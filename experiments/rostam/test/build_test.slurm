#!/bin/bash
#SBATCH --partition=marvin
#SBATCH --time=1:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=build_test
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
module load python
export CC=gcc
export CXX=g++

PATH_TO_EXE=${1:-./init/build/}

cd ${PATH_TO_EXE}
time ninja tests
