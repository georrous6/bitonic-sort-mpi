# Bitonic Sort MPI

This project implements a **distributed hybrid sort using Bitonic interchanges**, leveraging 
**MPI (Message Passing Interface)** for inter-process communication. The project includes automated 
benchmarking and performance visualization, executed on the **Aristotelis HPC Cluster** of **ECE AUTH**.

---

## Problem Overview

The goal is to **sort `N = 2^(p + q)` integers in ascending order**, distributed among 
**`2^p` MPI processes**. Each process is initialized with **`2^q` random integers**. 
The Bitonic Sort algorithm guides the data exchange between processes using non-blocking 
MPI communication.

### Algorithm Summary:
- Each process **locally sorts** its data (ascending or descending based on rank parity).
- Processes **communicate in pairs** according to **Bitonic Sort stages** using non-blocking sends 
and receives.
- A **pairwise min-max exchange** is applied to move smaller elements towards lower ranks and larger 
elements towards higher ranks.
- After multiple log-scaled iterations, each process applies a **final elbow sort** to achieve a
globally sorted order.
- The result is **validated** using serial sorting for correctness.

---

## Requirements
- **CMake** (for building the project)
- **MPI implementation** (e.g., OpenMPI, MPICH)
- **Python 3** (with `pandas`, `matplotlib` for plotting)
- **Slurm workload manager** (for submitting jobs on Aristotelis)

## Project Structure
- **`.github`**: CI/CD pipelines (GitHub Actions)
- **`.vscode`**: VSCode development configuration
- **`benchmarks`**: Benchmarking scripts and logs
- **`docs`**: Documentation and generated figures
- **`include`**: Header files
- **`scr`**: Source code
- **`tests`**: Unit tests and validation scripts

## Build Instructions

Clone this repository
```bash
git clone https://github.com/georrous6/bitonic-sort-mpi.git
cd bitonic-sort-mpi
```

Build the project with CMake:
```bash
cmake -S . -B build
cmake --build build
```

## Run Tests
To run the tests type
```bash
cd tests
chmod +x run_tests.sh
./run_tests.sh
```

## Run Benchmarks (on Aristotelis HPC)
To run the benchmarks you must have access the the Aristotelis HPC Cluster.

### Step 1: Connect to Aristoteis HPC
Connect to Aristotelis HPC Cluster via ssh:
```bash
ssh [username]@aristotle.it.auth.gr
```
Replace `username` with your institutional username.

### Step 2: Upload Project
You can either:

- **Upload locally cloned project:**
```bash
scp -r bitonic-sort-mpi/ [username]@aristotle.it.auth.gr:path/to/destination/
```
Replace `username` with your institutional username and `path/to/destination` to the desired destination.

- **Or clone directly on Aristotelis:**
```bash
git clone https://github.com/georrous6/bitonic-sort-mpi.git
```

### Step 3: Submit Benchmark Job
```bash
cd bitonic-sort-mpi
sbatch benchmarks/run_benchmarks.sh </path/to/bitonic-sort-mpi>
```
Replace `/path/to/bitonic-sort-mpi` with the installation path of the repository.

You can check the status of the submitted job with
```bash
squeue -u $USER
```
