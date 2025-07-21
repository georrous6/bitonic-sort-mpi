#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <assert.h>
#include "bitonic_sort_mpi.h"

int main(int argc, char *argv[]) {
    int q, p, rows, rank, cols;
    int *local_row = NULL, *recv_row = NULL;
    int status = EXIT_FAILURE;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &rows);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc != 3) {
        if (rank == 0) fprintf(stderr, "Usage: %s <q> <p>\n", argv[0]);
        MPI_Finalize();
        exit(status);
    }

    q = atoi(argv[1]);
    p = atoi(argv[2]);

    if (rows != (1 << p)) {
        if (rank == 0) fprintf(stderr, "Error: number of processes must be 2^p\n");
        MPI_Finalize();
        exit(status);
    }

    cols = 1 << q;
    local_row = (int *)malloc(sizeof(int) * cols); 
    if (!local_row) {
        fprintf(stderr, "Memory allocation failed for local_row in process %d\n", rank);
        goto cleanup;
    }
    recv_row = (int *)malloc(sizeof(int) * cols);
    if (!recv_row) {
        fprintf(stderr, "Memory allocation failed for recv_row in process %d\n", rank);
        goto cleanup;
    }

    srand(rank + time(NULL));
    for (int i = 0; i < cols; i++) local_row[i] = rand() % 100;

    distributed_bitonic_sort(local_row, recv_row, cols, rows, rank);

    // Verify local sorting
    for (int i = 0; i < cols - 1; i++) {
       assert(local_row[i] <= local_row[i + 1]);
    }

    int tmp;
    MPI_Request req;
    if (rank < rows - 1) {  // Send the last element to the next process
        MPI_Isend(local_row + cols - 1, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &req);
    }
    if (rank > 0) { // Receive the last element of the previous process
        MPI_Recv(&tmp, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        assert(tmp <= local_row[0]);
    }
    if (rank < rows - 1) MPI_Wait(&req, MPI_STATUS_IGNORE);

    status = EXIT_SUCCESS;

cleanup:
    if (local_row) free(local_row);
    if (recv_row) free(recv_row);
    MPI_Finalize();
    return status;
}
