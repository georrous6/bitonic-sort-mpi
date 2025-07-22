#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "util.h"
#include "bitonic_sort_mpi.h"

int main(int argc, char *argv[]) {
    int q, p, s, rank;
    int *local_row = NULL, *recv_row = NULL;
    int status = EXIT_FAILURE;
    ProgramOptions options;

    parse_arguments(argc, argv, &q, &p, &s, &options);
    int rows = 1 << p;
    int cols = 1 << q;
    int buff_size = 1 << s;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    local_row = (int *)malloc(sizeof(int) * cols);
    if (!local_row) {
        fprintf(stderr, "Process %d: Error allocating memory for local_row.\n", rank);
        goto cleanup;
    }
    recv_row = (int *)malloc(sizeof(int) * cols);
    if (!recv_row) {
        fprintf(stderr, "Process %d: Error allocating memory for recv_row.\n", rank);
        goto cleanup;
    }

    // Initialize with random values
    srand(rank);
    for (int i = 0; i < cols; i++) local_row[i] = rand();

    distributed_bitonic_sort(local_row, recv_row, cols, rows, buff_size, rank);

    if (options.validate) {
        validate_sort(local_row, rows, cols, rank);
    }

    TimingInfo time_info = get_timing_info();
    if (options.verbose && rank == 0) {
        printf("Timing Information (p: %d, q: %d, s: %d)\n", p, q, s);
        printf("=========================\n");
        printf("Initial sort time: %lf seconds\n", time_info.t_initial_sort);
        printf("Pairwise communication time: %lf seconds\n", time_info.t_comm_pairwise);
        printf("Elbow sort time: %lf seconds\n", time_info.t_elbow_sort);
        printf("Total time: %lf seconds\n", time_info.t_total);
    }
    if (options.timing_file && rank == 0) {
        save_timing_info(options.timing_file, p, q, s, &time_info);
    }

    status = EXIT_SUCCESS;

cleanup:
    if (local_row) free(local_row);
    if (recv_row) free(recv_row);

    // Reduce status across all processes to determine global success
    // If any process fails, the global status will be EXIT_FAILURE
    MPI_Barrier(MPI_COMM_WORLD);  // Ensure all processes reach this point
    int global_status;
    MPI_Reduce(&status, &global_status, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0 && options.verbose) {
        if (global_status == EXIT_FAILURE) {
            fprintf(stderr, "\nOne or more processes failed.\n");
        } 
        else {
            printf("\nAll processes completed successfully.\n");
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);  // Synchronize before exit
    MPI_Finalize();
    return rank == 0 ? global_status : EXIT_SUCCESS;  // Return global status for rank 0, success for 
}
