#!/bin/bash
#SBATCH --job-name=bitonic_benchmark
#SBATCH --partition=rome
#SBATCH --output=logs/slurm-%j.out
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=128
#SBATCH --cpus-per-task=1

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
python3 -m venv ~/myenv
source ~/myenv/bin/activate
pip install numpy pandas matplotlib

# Build the project with make
make

# Set executable path
EXECUTABLE="$PROJECT_DIR/build/bitonic_sort"

P_MIN=0
P_MAX=7
Q_MIN=20
Q_MAX=27

P_PLUS_Q=27

# Ensure logs directory exists
rm -f "$PROJECT_DIR/benchmarks/logs/bitonic_sort.log"
mkdir -p "$PROJECT_DIR/benchmarks/logs"

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

    srun -n $procs "$EXECUTABLE" $p $q $s --timing-file "$PROJECT_DIR/benchmarks/logs/bitonic_sort.log"
    status=$?
    
    if [ $status -ne 0 ]; then
        echo "An error occurred while running bitonic sort with p=${p}, q=${q}, s=${s}. Exiting."
        exit 1
    fi
done

python3 "$PROJECT_DIR/benchmarks/plot_benchmarks.py"
if [ $? -ne 0 ]; then
    echo "An error occurred while plotting benchmarks. Exiting."
    exit 1
fi

deactivate

echo "All benchmarks completed successfully."

