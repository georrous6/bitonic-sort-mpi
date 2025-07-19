#ifndef BITONIC_SORT_MPI_H
#define BITONIC_SORT_MPI_H

#include <stdbool.h>

typedef struct {
    double t_initial_sort;
    double t_comm_pairwise;
    double t_elbow_sort;
    double t_total;
} TimingInfo;


int cmp_asc(const void *a, const void *b);

int cmp_desc(const void *a, const void *b);

void initial_alternating_sort(int *local_row, int cols, int rank);

void pairwise_sort(int *local_row, int *recv_row, int cols, bool is_ascending);

void elbow_sort(int *local_row, int cols, bool is_ascending);

void distributed_bitonic_sort(int *local_row, int *recv_row, int cols, int rows, int rank);

#endif
