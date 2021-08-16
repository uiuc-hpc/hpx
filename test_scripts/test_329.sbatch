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

#ctest --repeat until-fail:100 --timeout 15000 -V -R "tests.examples.modules.resiliency_distributed.distributed.lci.async_replicate_distributed"
#ctest --repeat until-fail:100 --timeout 60 -V -R "tests.examples.modules.resiliency_distributed.distributed.mpi.async_replicate_distributed"
ctest --repeat until-fail:100 --timeout 60 -V -R "tests.examples.modules.resiliency_distributed.distributed.lci.async_replicate_distributed"

