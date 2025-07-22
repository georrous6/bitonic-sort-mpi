#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <assert.h>
#include "bitonic_sort_mpi.h"

int main(int argc, char *argv[]) {
    int q, p, s, rows, rank, cols, buff_size;
    int *local_row = NULL, *recv_row = NULL;
    int status = EXIT_FAILURE;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &rows);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc != 4) {
        if (rank == 0) fprintf(stderr, "Usage: %s <q> <p> <s>\n", argv[0]);
        MPI_Finalize();
        exit(status);
    }

    q = atoi(argv[1]);
    p = atoi(argv[2]);
    s = atoi(argv[3]);

    if (rows != (1 << p)) {
        if (rank == 0) fprintf(stderr, "Error: number of processes must be 2^p\n");
        MPI_Finalize();
        exit(status);
    }

    if (s > q) {
        if (rank == 0) fprintf(stderr, "Error: s must be less than or equal to q\n");
        MPI_Finalize();
        exit(status);
    }

    cols = 1 << q;
    buff_size = 1 << s;
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

    distributed_bitonic_sort(local_row, recv_row, cols, rows, buff_size, rank);

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
    TimingInfo time_info = get_timing_info();
    if (rank == 0) {
        printf("Initial sort time: %f seconds\n", time_info.t_initial_sort);
        printf("Pairwise communication time: %f seconds\n", time_info.t_comm_pairwise);
        printf("Elbow sort time: %f seconds\n", time_info.t_elbow_sort);
        printf("Total time: %f seconds\n", time_info.t_total);
    }

cleanup:
    if (local_row) free(local_row);
    if (recv_row) free(recv_row);
    MPI_Finalize();
    return status;
}
