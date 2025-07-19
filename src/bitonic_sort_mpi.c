#include "bitonic_sort_mpi.h"
#include <mpi.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

int cmp_asc(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

int cmp_desc(const void *a, const void *b) {
    return (*(int*)b - *(int*)a);
}

void initial_alternating_sort(int *local_row, int cols, int rank) {
    if (rank % 2 == 0) {
        qsort(local_row, cols, sizeof(int), cmp_asc);
    } else {
        qsort(local_row, cols, sizeof(int), cmp_desc);
    }
}

void pairwise_sort(int *local_row, int *recv_row, int cols, bool is_ascending) {
    
    for (int i = 0; i < cols; i++) {
        if ((is_ascending && local_row[i] > recv_row[i]) ||
            (!is_ascending && local_row[i] < recv_row[i])) {
            int temp = local_row[i];
            local_row[i] = recv_row[i];
            recv_row[i] = temp;
        }
    }
}


void elbow_sort(int *local_row, int cols, bool is_ascending) {
    int *buff = (int *)malloc(sizeof(int) * cols);
    if (!buff) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        fprintf(stderr, "Process %d: Memory allocation failed in elbow_sort\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

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
    free(buff);
}


void distributed_bitonic_sort(int *local_row, int *recv_row, int cols, int rows, int rank) {

    TimingInfo time_info = {0};
    double t_start = MPI_Wtime();
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
                MPI_Send(local_row, cols, MPI_INT, partner, 0, MPI_COMM_WORLD);
                MPI_Recv(local_row, cols, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else if (partner < rows) {
                MPI_Recv(recv_row, cols, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                pairwise_sort(local_row, recv_row, cols, is_ascending);
                MPI_Send(recv_row, cols, MPI_INT, partner, 0, MPI_COMM_WORLD);
            }
            MPI_Barrier(MPI_COMM_WORLD);
            time_info.t_comm_pairwise += MPI_Wtime() - t_start_comm_pairwise;
        }
        double t_start_comm_elbow_sort = MPI_Wtime();
        elbow_sort(local_row, cols, is_ascending);
        MPI_Barrier(MPI_COMM_WORLD);
        time_info.t_elbow_sort += MPI_Wtime() - t_start_comm_elbow_sort;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    time_info.t_total = MPI_Wtime() - t_start;
    if (rank == 0) {
        printf("Timing Information:\n");
        printf("Initial Sort: %f seconds\n", time_info.t_initial_sort);
        printf("Communication (Pairwise): %f seconds\n", time_info.t_comm_pairwise);
        printf("Elbow Sort: %f seconds\n", time_info.t_elbow_sort);
        printf("Total: %f seconds\n", time_info.t_total);
    }
}