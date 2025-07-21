#!/bin/bash

# Resolve script directory to make EXECUTABLE path relative to script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXECUTABLE="$SCRIPT_DIR/../build/test"

P_MAX=2
Q_MAX=19

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

for p in $(seq 0 $P_MAX); do
    procs=$((2 ** p))
    for q in $(seq 0 $Q_MAX); do
        echo -e "${BOLD_BLUE}Running with p=${p}, q=${q} ...${COLOR_RESET}"

        mpirun -np $procs "$EXECUTABLE" $q $p
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