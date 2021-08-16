#!/bin/bash
#
#SBATCH --time=02:50:00
#SBATCH --nodes=1
#SBATCH -p medusa
##SBATCH -w marvin08
#SBATCH --output=HPX_test.o%j
#SBATCH --error=HPX_test.e%j
#

cd /home/elizabeth/hpx/build_MPIwithLCI
module load openmpi/3.1.6
export PATH=$PATH:/home/elizabeth/LC/build:/home/elizabeth/LC/include

#ctest -V
for i in {1..5}
do
#ctest --timeout 300 -V -R "mpi.partitioned_vector" 
ctest --timeout 300 -V -R "lci.partitioned_vector" 
done

