#include "bitonic_sort_mpi.h"
#include <mpi.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>


static TimingInfo time_info;

// Accessor function
TimingInfo get_timing_info() {
    return time_info;
}

static int cmp_asc(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

static int cmp_desc(const void *a, const void *b) {
    return (*(int*)b - *(int*)a);
}

static void initial_alternating_sort(int *local_row, int cols, int rank) {
    if (rank % 2 == 0) {
        qsort(local_row, cols, sizeof(int), cmp_asc);
    } else {
        qsort(local_row, cols, sizeof(int), cmp_desc);
    }
}

static void pairwise_sort(int *local_row, int *recv_row, int cols, bool is_ascending) {

    for (int i = 0; i < cols; i++) {
        if ((is_ascending && local_row[i] > recv_row[i]) ||
            (!is_ascending && local_row[i] < recv_row[i])) {
            int temp = local_row[i];
            local_row[i] = recv_row[i];
            recv_row[i] = temp;
        }
    }
}


static void alloc_memory(MPI_Request **send_reqs, MPI_Request **recv_reqs, int **buff, int *n_reqs, int cols, int buff_size) {

    *n_reqs = cols / buff_size;
    *send_reqs = (MPI_Request *)malloc(sizeof(MPI_Request) * (*n_reqs));
    *recv_reqs = (MPI_Request *)malloc(sizeof(MPI_Request) * (*n_reqs));
    if (!(*send_reqs) || !(*recv_reqs)) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        fprintf(stderr, "Process %d: Memory allocation failed for requests\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    *buff = (int *)malloc(sizeof(int) * cols);
    if (!(*buff)) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        fprintf(stderr, "Process %d: Memory allocation failed for buffer\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
}


static void elbow_sort(int *local_row, int cols, bool is_ascending, int *buff) {

    // Find elbow (min or max)
    int elbow = 0;
    int elbow_val = local_row[0];
    for (int i = 1; i < cols; i++) {
        if ((is_ascending && local_row[i] < elbow_val) ||
            (!is_ascending && local_row[i] > elbow_val)) {
            elbow = i;
            elbow_val = local_row[i];
        }
    }

    // Initialize left and right pointers
    int l = (elbow - 1 + cols) % cols;
    int r = (elbow + 1) % cols;

    // Start from elbow value
    int idx = 0;
    buff[idx++] = local_row[elbow];

    // Merge the rest
    while (idx < cols) {
        if ((is_ascending && local_row[l] < local_row[r]) ||
            (!is_ascending && local_row[l] > local_row[r])) {
            buff[idx++] = local_row[l];
            l = (l - 1 + cols) % cols;
        } else {
            buff[idx++] = local_row[r];
            r = (r + 1) % cols;
        }
    }

    // Copy back to local_row
    for (int i = 0; i < cols; i++) local_row[i] = buff[i];
}


void distributed_bitonic_sort(int *local_row, int *recv_row, int cols, int rows, int buff_size, int rank) {

    MPI_Request *send_reqs = NULL, *recv_reqs = NULL;
    int *buff = NULL;
    int n_reqs = 0;

    double t_start = MPI_Wtime();
    alloc_memory(&send_reqs, &recv_reqs, &buff, &n_reqs, cols, buff_size);

    time_info = (TimingInfo){0.0, 0.0, 0.0, 0.0};
    initial_alternating_sort(local_row, cols, rank);
    
    MPI_Barrier(MPI_COMM_WORLD);
    time_info.t_initial_sort = MPI_Wtime() - t_start;

    int stages = (int) log2(rows);
    for (int stage = 1; stage <= stages; stage++) {
        int num_chunks = 1 << (stages - stage);
        int chunk_size = rows / num_chunks;
        int chunk = rank / chunk_size;
        bool is_ascending = (chunk % 2 == 0);

        for (int step = stage - 1; step >= 0; step--) {
            int partner = rank ^ (1 << step);

            double t_start_comm_pairwise = MPI_Wtime();
            if (rank >= partner) {

                // Post non-blocking sends and receives for all batches
                for (int i = 0; i < n_reqs; i++) {
                    int offset = i * buff_size;
                    MPI_Isend(local_row + offset, buff_size, MPI_INT, partner, i, MPI_COMM_WORLD, &send_reqs[i]);
                    MPI_Irecv(local_row + offset, buff_size, MPI_INT, partner, i, MPI_COMM_WORLD, &recv_reqs[i]);
                }

                // Wait for all sends and receives to complete
                MPI_Waitall(n_reqs, recv_reqs, MPI_STATUS_IGNORE);
                MPI_Waitall(n_reqs, send_reqs, MPI_STATUS_IGNORE);

            } 
            else {

                // Post non-blocking receives for all batches
                for (int i = 0; i < n_reqs; i++) {
                    int offset = i * buff_size;  
                    MPI_Irecv(recv_row + offset, buff_size, MPI_INT, partner, i, MPI_COMM_WORLD, &recv_reqs[i]);
                }

                int completed = 0;

                // Process each batch as it arrives
                while (completed < n_reqs) {
                    int idx;

                    // Wait for any receive to complete
                    MPI_Waitany(n_reqs, recv_reqs, &idx, MPI_STATUS_IGNORE);
                    int offset = idx * buff_size;

                    // Process each received batch
                    pairwise_sort(local_row + offset, recv_row + offset, buff_size, is_ascending);

                    // Send the processed batch back
                    MPI_Isend(recv_row + offset, buff_size, MPI_INT, partner, idx, MPI_COMM_WORLD, &send_reqs[idx]);
                    completed++;
                }

                // Wait for all sends to complete
                MPI_Waitall(n_reqs, send_reqs, MPI_STATUS_IGNORE);
            }
            MPI_Barrier(MPI_COMM_WORLD);
            time_info.t_comm_pairwise += MPI_Wtime() - t_start_comm_pairwise;
        }
        double t_start_comm_elbow_sort = MPI_Wtime();
        elbow_sort(local_row, cols, is_ascending, buff);
        MPI_Barrier(MPI_COMM_WORLD);
        time_info.t_elbow_sort += MPI_Wtime() - t_start_comm_elbow_sort;
    }

    free(buff);
    free(send_reqs);
    free(recv_reqs);
    MPI_Barrier(MPI_COMM_WORLD);
    time_info.t_total = MPI_Wtime() - t_start;
}
