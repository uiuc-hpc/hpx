#!/bin/bash -l
#SBATCH --job-name=build-daint-gpu
#SBATCH --nodes=1
#SBATCH --constraint=gpu
#SBATCH --partition=cscsci
#SBATCH --time=00:30:00
#SBATCH --output=hpx-daint-gpu-performance-tests.out
#SBATCH --error=hpx-daint-gpu-performance-tests.err

set -eux

source ../tools/jenkins/env-daint-gpu.sh

ctest -R tests.performance
