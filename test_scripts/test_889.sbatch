#!/bin/bash
#
#SBATCH --time=01:30:00
#SBATCH --nodes=1
#SBATCH --output=HPX_test.o%j
#SBATCH --error=HPX_test.e%j
#

cd /home/elizabeth/hpx/build_MPIwithLCI
module load openmpi/3.1.6
export PATH=$PATH:/home/elizabeth/LC/build:/home/elizabeth/LC/include

for i in {1..1000}
do
    ctest -V -R "tests.regressions.components.distributed.mpi.create_remote_component_2334"
done

