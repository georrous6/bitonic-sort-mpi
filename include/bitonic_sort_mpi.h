#ifndef BITONIC_SORT_MPI_H
#define BITONIC_SORT_MPI_H

#include <stdbool.h>

typedef struct {
    double t_initial_sort;
    double t_comm_pairwise;
    double t_elbow_sort;
    double t_total;
} TimingInfo;


TimingInfo get_timing_info();


void distributed_bitonic_sort(int *local_row, int *recv_row, int cols, int rows, int buff_size, int rank);

#endif
