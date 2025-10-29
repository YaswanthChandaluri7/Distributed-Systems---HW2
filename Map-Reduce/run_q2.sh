#!/bin/bash
#SBATCH --job-name=4-cycle-mapreduce # Name for your job
#SBATCH --partition=debug            # IMPORTANT: Use a valid partition from your cluster
#SBATCH --nodes=1                    # Run all processes on a single machine
#SBATCH --ntasks=16                  # Request 16 cores in total
#SBATCH --time=00:10:00              # Set a short time for testing
#SBATCH --output=output.txt       # File for all standard output (cycle results)
#SBATCH --error=bench.txt      # File for all standard error (benchmark data)

# --- Preamble ---
echo "Job running on ${SLURM_NTASKS} cores."

# --- Load Modules ---
# Load your cluster's specific OpenMPI module. Find the name with 'module avail'.
module purge
module load openmpi/4.1.5 # Or whatever the correct name is on your system

# --- Compile Code ---
# This command compiles your C++ source file into an executable named 'q2_mpi'.
echo "Compiling q2.cpp..."
cd ~/Q2
mpic++ -std=c++17 -o q2_mpi q2.cpp

# --- Run Program ---
# This is the step that actually runs your code. The print statements
# inside your C++ program will execute now.
echo "Running MPI program..."
mpirun --mca pml ^ucx --mca osc ^ucx -np $SLURM_NTASKS ./q2_mpi
echo "Job finished."
