#!/bin/bash
#SBATCH --partition=marvin
#SBATCH --time=1:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=16
#SBATCH --job-name=test
#SBATCH --output=slurm_output.%x-o%j
#SBATCH --error=slurm_error.%x-o%j

# setup module environment
module purge
module load gcc
module load cmake
module load boost
module load hwloc
module load openmpi
module load papi
export CC=gcc
export CXX=g++

PATH_TO_EXE=${1:-./init/build/}

cd ${PATH_TO_EXE}
time ctest --verbose --timeout 300
