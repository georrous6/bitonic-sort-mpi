#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <assert.h>
#include "bitonic_sort_mpi.h"

int main(int argc, char *argv[]) {
    int q, p, s, rows, rank, cols, buff_size;
    int *local_row = NULL, *recv_row = NULL;
    int status = EXIT_FAILURE;  // Local status

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &rows);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc != 4) {
        if (rank == 0) fprintf(stderr, "Usage: %s <q> <p> <s>\n", argv[0]);
        goto cleanup;
    }

    q = atoi(argv[1]);
    p = atoi(argv[2]);
    s = atoi(argv[3]);

    if (rows != (1 << p)) {
        if (rank == 0) fprintf(stderr, "Error: number of processes must be 2^p\n");
        goto cleanup;
    }

    if (s > q) {
        if (rank == 0) fprintf(stderr, "Error: s must be less than or equal to q\n");
        goto cleanup;
    }

    buff_size = 1 << s;
    cols = 1 << q;
    local_row = (int *)malloc(sizeof(int) * cols);
    recv_row = (int *)malloc(sizeof(int) * cols);
    if (!local_row || !recv_row) {
        fprintf(stderr, "Memory allocation failed on process %d\n", rank);
        goto cleanup;
    }

    srand(rank);  // Seed
    for (int i = 0; i < cols; i++) local_row[i] = rand();

    distributed_bitonic_sort(local_row, recv_row, cols, rows, buff_size, rank);

    for (int i = 0; i < cols - 1; i++) assert(local_row[i] <= local_row[i + 1]);

    int tmp;
    MPI_Request req;
    if (rank < rows - 1) {
        MPI_Isend(local_row + cols - 1, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &req);
    }
    if (rank > 0) {
        MPI_Recv(&tmp, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        assert(tmp <= local_row[0]);
    }
    if (rank < rows - 1) MPI_Wait(&req, MPI_STATUS_IGNORE);

    status = EXIT_SUCCESS;  // All checks passed

cleanup:
    if (local_row) free(local_row);
    if (recv_row) free(recv_row);

    // Reduce status across all processes to determine global success
    // If any process fails, the global status will be EXIT_FAILURE
    MPI_Barrier(MPI_COMM_WORLD);  // Ensure all processes reach this point
    int global_status;
    MPI_Reduce(&status, &global_status, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);  // Synchronize before exit
    if (rank == 0) {
        MPI_Finalize();
        return global_status;
    } 
    else {
        MPI_Finalize();
        return EXIT_SUCCESS;  // Other processes exit cleanly
    }
}
