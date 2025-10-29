

# Distributed Systems - Homework 2

This document provides the compilation and execution instructions for the three questions in this assignment.

## Q1) MPI Program

### Execution Details

The program can be compiled and executed using the following commands. The input is read from `input.txt` and the output is redirected to `output.txt`.

**Compile:** 
>mpic++ -o q1 q1.cpp


**Execution:**
> mpirun -np <num_processes> ./q1 < input.txt > output.txt


## Q2) Optimized MPI on Slurm Cluster

### Execution Details

The program was compiled and executed on a Slurm-managed cluster.

**Compilation Command:**
The code was compiled into an executable named `q2_optimized_mpi`.

> mpic++ -std=c++11 -o q2_optimized_mpi q2_optimized.cpp


**Slurm Batch Script (`run_job.sh`):**
This script requests 16 tasks on a single node and runs for a maximum of 10 minutes. The output is saved to `resultsq2.txt` and errors are logged to `benchmarkq2.txt`.

>* #!/bin/bash
>* #SBATCH --job-name=4-cycle-optimized
>* #SBATCH --partition=debug
>* #SBATCH --nodes=1
>* #SBATCH --ntasks=16
>* #SBATCH --time=00:10:00
>* #SBATCH --output=resultsq2.txt
>* #SBATCH --error=benchmarkq2.txt

>* module purge
>* module load openmpi

>* echo "Running on $SLURM_NTASKS cores..."
>* mpirun -np $SLURM_NTASKS ./q2_optimized_mpi


**Submission Command:**

> sbatch run_job.sh


## Q3) gRPC Client-Server

### Execution Details

The following commands were used for generating protocol buffer files and running the client-server application.

**Protocol Buffer Generation:**
This command generates the necessary Python files from the `.proto` definition.

> python3 -m grpc_tools.protoc --python_out=. --grpc_python_out=. gRPC.proto


**Server Startup:**
This command starts the gRPC server.

> python3 server.py

**Client Execution:**
To run the clients, open separate terminal windows and execute the following commands.

*   **Client 1 Execution:**
    
    > python3 client.py 1
    
*   **Client 2 Execution:**
    
    > python3 client.py 2
    
