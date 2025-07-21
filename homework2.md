Parallel & Distributed Computer Systems

December 6, 2024

Write a distributed program that sorts `N` integers in ascending order, using MPI. The
inter-process communications should be defined by the Bitonic sort algorithm as presented 
in our class notes.

The program must perform the following tasks:

- The user specifies two positive integers $q$ and $p$.

- Start `2^p` processes with an array of `2^q` random integers is each processes.

- Sort all `N = 2^(q + p)` elements in ascending order.

- Check the correctness of the final result.

Your implementation should be based on the following steps:
- Start p processes, each process gets `n/p` data and sorts (ascending or descending depending 
on the process id) them using sort from any library. Use two buffers, to sort from one to the other,
to send from one and receive in the other.
- Repeat `O((log2(p))^2)`
- Exchange data with the corresponding partner and keep the min or max elements, depending on the
process id and phase of the computation
- Sort locally the bitonic sequence using a modification of merge sort, starting from the "elbow" 
of the bitonic.

You may use the C standard library function stdlib `qsort()` to check the correctness of your results and as the initial local sorting routine for each process.

You must deliver:
- A report (about 3-4 pages) that describes your parallel algorithm and implementation.

- Your comments on the speed of your parallel program compared to the serial sort, after trying you program on aristotelis for `p = [1:7]` and `q = [20:27]`.

- The source code of your program uploaded online.

Ethics: If you use code found on the web or by an LLM, you should mention your source and the changes you made. You may work in pairs; both partners must submit a single report with both names.
Deadline: 7 January, 2025.


