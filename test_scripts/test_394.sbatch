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

P=4
for i in {1..1}
do
#ctest -V -R "tests.unit.modules.segmented_algorithms.distributed.mpi.partitioned_vector_exclusive_scan"
srun --mpi=pmi2 -n $P /home/elizabeth/hpx/build_MPIwithLCI/bin/partitioned_vector_exclusive_scan_test --hpx:ini=hpx.parcel.mpi.priority=1000 --hpx:ini=hpx.parcel.mpi.enable=1 --hpx:ini=hpx.parcel.bootstrap=mpi --hpx:threads=1 --hpx:localities=$P
done

