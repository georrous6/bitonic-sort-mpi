#ifndef PARALLEL_SORT_OMP_H
#define PARALLEL_SORT_OMP_H

#include <stdbool.h>

void parallel_merge_sort(int *arr, int l, int r, int depth, bool ascending);

#endif