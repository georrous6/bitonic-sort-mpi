#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mpi.h>
#include <assert.h>


void parse_arguments(int argc, char *argv[], int *p, int *q, int *s, ProgramOptions *options) {

    int n_procs, rank;
    // Set defaults
    options->verbose = false;
    options->validate = true;
    options->timing_file = NULL;
    options->depth = 0;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <p> <q> <s> [--verbose] [--no-validation] [--timing-file <file>] [--depth <depth>]\n", argv[0]);
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    *p = atoi(argv[1]);
    *q = atoi(argv[2]);
    *s = atoi(argv[3]);

    if (n_procs != (1 << *p)) {
        if (rank == 0) fprintf(stderr, "Error: number of processes must be 2^%d (nprocs: %d)\n", *p, n_procs);
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    if (*q < 0 || *p < 0 || *s < 0 || *q > 31 || *p > 31 || *s > 31) {
        if (rank == 0)fprintf(stderr, "Error: q, p, and s must be in the range [0, 31].\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    if (*s > *q) {
        if (rank == 0) fprintf(stderr, "Error: s must be less than or equal to q.\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    // Parse additional options
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0) {
            options->verbose = true;  // Enable verbose output
        } 
        else if (strcmp(argv[i], "--timing-file") == 0) {
            if (i + 1 < argc) {
                options->timing_file = argv[++i];
            } 
            else {
                fprintf(stderr, "Error: --timing-file requires a filename.\n");
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[i], "--depth") == 0) {
            if (i + 1 < argc) {
                options->depth = atoi(argv[++i]) < 0 ? 0 : atoi(argv[i]);
            } 
            else {
                fprintf(stderr, "Error: --depth requires an integer value.\n");
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[i], "--no-validation") == 0) {
            options->validate = false;
        } 
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            MPI_Finalize();
            exit(EXIT_FAILURE);
        }
    }
}


void validate_sort(int *local_row, int rows, int cols, int rank) {
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
}


void save_timing_info(const char *filename, int p, int q, int s, TimingInfo *time_info) {
    if (filename == NULL) return;  // No file specified

    FILE *file = fopen(filename, "a");
    if (!file) {
        fprintf(stderr, "Error opening timing file '%s': %s\n", filename, strerror(errno));
        return;
    }

    fprintf(file, "%d %d %d %lf %lf %lf %lf\n", p, q, s,
            time_info->t_initial_sort,
            time_info->t_comm_pairwise,
            time_info->t_elbow_sort,
            time_info->t_total);

    fclose(file);
}
