#!/bin/bash
#SBATCH --job-name=bitonic_benchmark
#SBATCH --partition=rome
#SBATCH --output=logs/slurm-%j.out
#SBATCH --time=07:00:00
#SBATCH --nodes=8
#SBATCH --ntasks=128
#SBATCH --cpus-per-task=4

set -e  # Exit on error immediately

export UCX_WARN_UNUSED_ENV_VARS=n

# First command-line argument: project dir
PROJECT_DIR="$1"
if [ -z "$PROJECT_DIR" ]; then
    echo "Usage: $0 <project_dir>"
    exit 1
fi

# Set working directory to project dir
cd "$PROJECT_DIR" || { echo "Cannot cd to $PROJECT_DIR"; exit 1; }

# Install modules
module purge
module load gcc/14.2.0
module load python/3.13.0
module load openmpi/5.0.5

# Create python environment
if [ ! -d ~/grousenv ]; then
    python3 -m venv ~/grousenv
    source ~/grousenv/bin/activate
    pip install numpy pandas matplotlib
else
    source ~/grousenv/bin/activate
fi

# Build the project with make
make

# Set executable path
EXECUTABLE="$PROJECT_DIR/build/bitonic_sort"
LOG_FILE="$PROJECT_DIR/benchmarks/logs/bitonic_sort.log"
FIGURES_DIR="$PROJECT_DIR/docs/figures"
DATA_DIR="$PROJECT_DIR/docs/data"

P_MIN=0
P_MAX=7
Q_MIN=20
Q_MAX=27

log2_cpustask=$(awk -v n="$SLURM_CPUS_PER_TASK" 'BEGIN { print int(log(n)/log(2) + 0.5) }')

# Ensure logs directory exists
mkdir -p "$PROJECT_DIR/benchmarks/logs"
rm -f "$LOG_FILE"

# Store benchmark start time and SLURM environment variables
echo "Benchmark started at $(date)" | tee -a "$LOG_FILE"
echo "SLURM_NODES=$SLURM_JOB_NUM_NODES, SLURM_NTASKS=$SLURM_NTASKS, SLURM_CPUS_PER_TASK=$SLURM_CPUS_PER_TASK" | tee -a "$LOG_FILE"


# Check if bitonic sort executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Executable not found at $EXECUTABLE"
    exit 1
fi

for p in $(seq $P_MIN $P_MAX); do
    procs=$((2 ** p))
    for q in $(seq $Q_MIN $Q_MAX); do
        for s in $(seq $q -1 $((q - 2))); do
            for d in $(seq 0 $log2_cpustask); do

                # Set environment variables for the run
                export OMP_NUM_THREADS=$((2 ** d))

                # Log the run parameters
                echo "Running bitonic sort with p=${p}, q=${q}, s=${s}, depth=${d} ..."

                # Run the executable with srun
                srun -n "$procs" "$EXECUTABLE" "$p" "$q" "$s" --depth "$d" --timing-file "$LOG_FILE"
            done
        done
    done
done

python3 "$PROJECT_DIR/benchmarks/plot_benchmarks.py" "$LOG_FILE" "$FIGURES_DIR" "$DATA_DIR"
deactivate

echo "All benchmarks completed successfully."

