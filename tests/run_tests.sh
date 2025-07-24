#!/bin/bash

# Resolve script directory to make EXECUTABLE path relative to script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXECUTABLE="$SCRIPT_DIR/../build/bitonic_sort"

P_MIN=0
P_MAX=2
Q_MIN=0
Q_MAX=19
Q_THRESHOLD=10

# ANSI color codes
BOLD_BLUE="\033[1;34m"
BOLD_RED="\033[1;31m"
BOLD_GREEN="\033[1;32m"
COLOR_RESET="\033[0m"

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "Executable not found at $EXECUTABLE"
    exit 1
fi

total_tests=$(( (P_MAX + 1) * (Q_MAX + 1) ))
passed_tests=0
failed_tests=0

export OMP_NUM_THREADS=4

for p in $(seq $P_MIN $P_MAX); do
    procs=$((2 ** p))
    for q in $(seq $Q_MIN $Q_MAX); do

        # Calculate s based on q
        if [ $q -gt $Q_THRESHOLD ]; then
            s=$((q - 2))
        else
            s=$q
        fi


        echo -e "${BOLD_BLUE}Running with p=${p}, q=${q}, s=${s} ...${COLOR_RESET}"

        mpirun --oversubscribe -np "$procs" "$EXECUTABLE" "$p" "$q" "$s" --depth 2
        status=$?
        
        if [ $status -ne 0 ]; then
            echo -e "${BOLD_RED}Failed${COLOR_RESET}"
            ((failed_tests++))
        else
            echo -e "${BOLD_GREEN}Passed${COLOR_RESET}"
            ((passed_tests++))
        fi
    done
done

echo
if [ $failed_tests -eq 0 ]; then
    echo -e "${BOLD_GREEN}All tests completed successfully ($passed_tests/$total_tests) ${COLOR_RESET}"
    exit 0
else
    echo -e "${BOLD_RED}Tests passed: $passed_tests/$total_tests, Tests failed: $failed_tests${COLOR_RESET}"
    exit 1
fi