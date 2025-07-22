#!/bin/bash
#SBATCH --job-name=bitonic_benchmark
#SBATCH --partition=batch
#SBATCH --output=logs/slurm-%j.out
#SBATCH --time=01:00:00
#SBATCH --nodes=4
#SBATCH --ntasks=128
#SBATCH --cpus-per-task=1
#SBATCH --partition=compute

# Resolve script directory to make executable paths relative to script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXECUTABLE="$SCRIPT_DIR/../build/bitonic_sort"

P_MIN=0
P_MAX=7
Q_MIN=20
Q_MAX=27

P_PLUS_Q=30

# Ensure logs directory exists
rm -rf "$SCRIPT_DIR/logs"
mkdir -p "$SCRIPT_DIR/logs"

# Check if bitonic sort executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Executable not found at $EXECUTABLE"
    exit 1
fi


for p in $(seq $P_MIN $P_MAX); do
    procs=$((2 ** p))

    q=$(($P_PLUS_Q - p))
    s=$q

    echo "Running with p=${p}, q=${q}, s=${s} ..."

    mpirun -np $procs "$EXECUTABLE" $p $q $s --timing-file "$SCRIPT_DIR/logs/bitonic_sort.log"
    status=$?
    
    if [ $status -ne 0 ]; then
        echo "An error occurred while running bitonic sort with p=${p}, q=${q}, s=${s}. Exiting."
        exit 1
    fi
done

python3 "$SCRIPT_DIR/plot_benchmarks.py"
if [ $? -ne 0 ]; then
    echo "An error occurred while plotting benchmarks. Exiting."
    exit 1
fi

echo "All benchmarks completed successfully."
