#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <omp.h>


static void merge(int *arr, int l, int m, int r, bool ascending) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));

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

        if (depth <= 0) {
            parallel_merge_sort(arr, l, m, 0, ascending);
            parallel_merge_sort(arr, m + 1, r, 0, ascending);
        } else {
            #pragma omp parallel sections
            {
                #pragma omp section
                parallel_merge_sort(arr, l, m, depth - 1, ascending);
                #pragma omp section
                parallel_merge_sort(arr, m + 1, r, depth - 1, ascending);
            }
        }

        merge(arr, l, m, r, ascending);
    }
}
