#include "parallel_sort_omp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <omp.h>
#include <mpi.h>


static int cmp_asc(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}


static int cmp_desc(const void *a, const void *b) {
    return (*(int *)b - *(int *)a);
}


static void merge(int *arr, int l, int m, int r, bool ascending) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));
    if (!L || !R) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        fprintf(stderr, "Process %d: Memory allocation failed for merge\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }


    for (i = 0; i < n1; i++) L[i] = arr[l + i];
    for (j = 0; j < n2; j++) R[j] = arr[m + 1 + j];

    i = j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if ((ascending && L[i] <= R[j]) || (!ascending && L[i] >= R[j]))
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }

    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    free(L);
    free(R);
}


void parallel_merge_sort(int *arr, int l, int r, int depth, bool ascending) {
    if (l < r) {
        int m = l + (r - l) / 2;
        int sz = r - l + 1;

        if (depth <= 0 || sz < MIN_SIZE_THRESHOLD) {
            // Serial fallback using qsort
            qsort(arr + l, r - l + 1, sizeof(int), ascending ? cmp_asc : cmp_desc);
        }
        else {
            #pragma omp parallel sections
            {
                #pragma omp section
                parallel_merge_sort(arr, l, m, depth - 1, ascending);
                #pragma omp section
                parallel_merge_sort(arr, m + 1, r, depth - 1, ascending);
            }

            merge(arr, l, m, r, ascending);
        }
    }
}
