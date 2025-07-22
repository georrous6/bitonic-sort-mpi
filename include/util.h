#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include "bitonic_sort_mpi.h"


typedef struct {
    bool verbose;       // Optional: enable verbose output
    bool validate;      // Optional: enable validation of the sort
    char *timing_file;  // Optional: file to save timing information
} ProgramOptions;


void parse_arguments(int argc, char *argv[], int *p, int *q, int *s, ProgramOptions *options);


void validate_sort(int *local_row, int rows, int cols, int rank);


void save_timing_info(const char *filename, int p, int q, int s, TimingInfo *time_info);

#endif
