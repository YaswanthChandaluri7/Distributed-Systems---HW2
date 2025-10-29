#!/bin/bash
# Usage: ./run_experiments.sh N   (where N is number of iterations)

if [ $# -ne 1 ]; then
    echo "Usage: $0 <num_iterations>"
    exit 1
fi

ITERATIONS=$1

# Ensure output directories exist
mkdir -p outputs benchmark testcases

# Compile and run generate.cpp once
echo "Compiling generate.cpp..."
g++ -O2 -std=c++17 generate.cpp -o generate
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi


# Iteration loop
for (( j=1; j<=ITERATIONS; j++ ))
do
echo "Running generate..."
./generate
cp input.txt "./testcases/tc_${j}.txt"
    echo "========== Iteration $j =========="

    for cores in 2 6 12 24
    do
        echo "[Iteration $j] Submitting job with $cores cores..."

        # Submit job
        JOBID=$(sbatch --wait --ntasks=$cores run_q2.sh | awk '{print $4}')
        echo "Submitted job $JOBID"

        # Wait for completion
       # while squeue -j "$JOBID" >/dev/null 2>&1; do
   	 sleep 3
	#done


        # Copy results
        cp output.txt "./outputs/output_${j}_${cores}.txt"
        cp bench.txt "./benchmark/bench_${j}_${cores}.txt"
        echo "[Iteration $j] Results saved for $cores cores."
    done
done

echo "All iterations completed."

