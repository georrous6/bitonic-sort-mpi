#include "distributed_sort_mpi.h"
#include "parallel_sort_omp.h"
#include <mpi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>


static TimingInfo time_info;


// Accessor function
TimingInfo get_timing_info() {
    return time_info;
}


static void pairwise_exchange(int *local_data, int *recv_data, int n_data_proc, bool is_ascending) {

    for (int i = 0; i < n_data_proc; i++) {
        if ((is_ascending && local_data[i] > recv_data[i]) ||
            (!is_ascending && local_data[i] < recv_data[i])) {
            int temp = local_data[i];
            local_data[i] = recv_data[i];
            recv_data[i] = temp;
        }
    }
}


static void alloc_memory(MPI_Request **send_reqs, MPI_Request **recv_reqs, int **buff, int *n_reqs, int n_data_proc, int buff_size) {

    *n_reqs = n_data_proc / buff_size;
    *send_reqs = (MPI_Request *)malloc(sizeof(MPI_Request) * (*n_reqs));
    *recv_reqs = (MPI_Request *)malloc(sizeof(MPI_Request) * (*n_reqs));
    if (!(*send_reqs) || !(*recv_reqs)) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        fprintf(stderr, "Process %d: Memory allocation failed for requests\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    *buff = (int *)malloc(sizeof(int) * n_data_proc);
    if (!(*buff)) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        fprintf(stderr, "Process %d: Memory allocation failed for buffer\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
}


static void elbow_sort(int *local_data, int n_data_proc, bool is_ascending, int *buff) {

    // Find elbow (min or max)
    int elbow = 0;
    int elbow_val = local_data[0];
    for (int i = 1; i < n_data_proc; i++) {
        if ((is_ascending && local_data[i] < elbow_val) ||
            (!is_ascending && local_data[i] > elbow_val)) {
            elbow = i;
            elbow_val = local_data[i];
        }
    }

    // Initialize left and right pointers
    int l = (elbow - 1 + n_data_proc) % n_data_proc;
    int r = (elbow + 1) % n_data_proc;

    // Start from elbow value
    int idx = 0;
    buff[idx++] = local_data[elbow];

    // Merge the rest
    while (idx < n_data_proc) {
        if ((is_ascending && local_data[l] < local_data[r]) ||
            (!is_ascending && local_data[l] > local_data[r])) {
            buff[idx++] = local_data[l];
            l = (l - 1 + n_data_proc) % n_data_proc;
        } else {
            buff[idx++] = local_data[r];
            r = (r + 1) % n_data_proc;
        }
    }

    // Copy back to local_row
    for (int i = 0; i < n_data_proc; i++) local_data[i] = buff[i];
}


void distributed_bitonic_sort(int *local_data, int *recv_data, int n_procs, int n_data_proc, int buff_size, int rank, int depth) {

    MPI_Request *send_reqs = NULL, *recv_reqs = NULL;
    int *buff = NULL;
    int n_reqs = 0;

    double t_start = MPI_Wtime();
    alloc_memory(&send_reqs, &recv_reqs, &buff, &n_reqs, n_data_proc, buff_size);

    time_info = (TimingInfo){0.0, 0.0, 0.0, 0.0};
    parallel_merge_sort(local_data, 0, n_data_proc - 1, depth, rank % 2 == 0);
    
    MPI_Barrier(MPI_COMM_WORLD);
    time_info.t_initial_sort = MPI_Wtime() - t_start;

    int stages = 0;
    int temp = n_procs;

    while (temp > 1) {
        temp = temp >> 1;
        stages++;
    }

    for (int stage = 1; stage <= stages; stage++) {
        int num_chunks = 1 << (stages - stage);
        int chunk_size = n_procs / num_chunks;
        int chunk = rank / chunk_size;
        bool is_ascending = (chunk % 2 == 0);

        for (int step = stage - 1; step >= 0; step--) {
            int partner = rank ^ (1 << step);

            double t_start_comm_pairwise = MPI_Wtime();
            if (rank >= partner) {

                // Post non-blocking sends and receives for all batches
                for (int i = 0; i < n_reqs; i++) {
                    int offset = i * buff_size;
                    MPI_Isend(local_data + offset, buff_size, MPI_INT, partner, i, MPI_COMM_WORLD, &send_reqs[i]);
                    MPI_Irecv(local_data + offset, buff_size, MPI_INT, partner, i, MPI_COMM_WORLD, &recv_reqs[i]);
                }

                // Wait for all sends and receives to complete
                MPI_Waitall(n_reqs, recv_reqs, MPI_STATUS_IGNORE);
                MPI_Waitall(n_reqs, send_reqs, MPI_STATUS_IGNORE);

            } 
            else {

                // Post non-blocking receives for all batches
                for (int i = 0; i < n_reqs; i++) {
                    int offset = i * buff_size;  
                    MPI_Irecv(recv_data + offset, buff_size, MPI_INT, partner, i, MPI_COMM_WORLD, &recv_reqs[i]);
                }

                int completed = 0;

                // Process each batch as it arrives
                while (completed < n_reqs) {
                    int idx;

                    // Wait for any receive to complete
                    MPI_Waitany(n_reqs, recv_reqs, &idx, MPI_STATUS_IGNORE);
                    int offset = idx * buff_size;

                    // Process each received batch
                    pairwise_exchange(local_data + offset, recv_data + offset, buff_size, is_ascending);

                    // Send the processed batch back
                    MPI_Isend(recv_data + offset, buff_size, MPI_INT, partner, idx, MPI_COMM_WORLD, &send_reqs[idx]);
                    completed++;
                }

                // Wait for all sends to complete
                MPI_Waitall(n_reqs, send_reqs, MPI_STATUS_IGNORE);
            }
            MPI_Barrier(MPI_COMM_WORLD);
            time_info.t_comm_pairwise += MPI_Wtime() - t_start_comm_pairwise;
        }
        double t_start_elbow_sort = MPI_Wtime();
        elbow_sort(local_data, n_data_proc, is_ascending, buff);
        MPI_Barrier(MPI_COMM_WORLD);
        time_info.t_elbow_sort += MPI_Wtime() - t_start_elbow_sort;
    }

    free(buff);
    free(send_reqs);
    free(recv_reqs);
    MPI_Barrier(MPI_COMM_WORLD);
    time_info.t_total = MPI_Wtime() - t_start;
}
