#!/bin/bash
#
#SBATCH --time=00:10:00
#SBATCH --nodes=1
#SBATCH --output=HPX_test.o%j
#SBATCH --error=HPX_test.e%j
#

cd /home/elizabeth/hpx/build_MPIwithLCI
module load openmpi/3.1.6
export PATH=$PATH:/home/elizabeth/LC/build:/home/elizabeth/LC/include

ctest --repeat until-fail:100 -V -R "tests.performance.modules.resiliency_distributed.replay.distributed...i.1d_stencil_distributed" 

