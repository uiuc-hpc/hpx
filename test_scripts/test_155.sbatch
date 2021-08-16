#!/bin/bash
#
#SBATCH --time=00:50:00
#SBATCH --nodes=1
#SBATCH -p marvin
#SBATCH --output=HPX_test.o%j
#SBATCH --error=HPX_test.e%j
#

cd /home/elizabeth/hpx/build_MPIwithLCI
module load openmpi/3.1.6
export PATH=$PATH:/home/elizabeth/LC/build:/home/elizabeth/LC/include

#ctest -V
ctest -V -R "tests.unit.modules.async_colocated.distributed.mpi.apply_colocated"

