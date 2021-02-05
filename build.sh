#!/bin/bash

module load boost/1.71.0
module load cmake/.3.18.4
module load hwloc/1.7.2
#module load openmpi/3.1.1-intel-18.0
module load mvapich2/.2.3.4-intel-18.0

echo $PATH

if [[ ! $PATH == *"HPXLCI/LC"* ]]; then
	export PATH=$PATH:/home/ekoning2/scratch/HPXLCI/LC/install/usr/local
	export PATH=$PATH:/home/ekoning2/scratch/HPXLCI/LC/include
fi

if [[ ! $PATH == *"boost/1.71.0"* ]]; then
	export PATH=$PATH:/usr/local/boost/1.71.0/include/boost
fi

if [ -n CPLUS_INCLUDE_PATH ]; then
	export CPLUS_INCLUDE_PATH=/usr/local/hwloc/1.7.2/include:/usr/local/hwloc/1.7.2/include/hwloc:/usr/local/boost/1.71.0/include
	# not sure if boost needs to be in PATH or not --must test
	#export PATH=$PATH:/usr/local/boost/1.71.0/include/boost
fi

mkdir -p build && cd build

cmake -DCMAKE_INSTALL_PREFIX=/home/ekoning2/scratch/HPXLCI/replace_mpi/hpx/build/install -DBOOST_ROOT=/usr/local/boost/1.71.0 -DHPX_WITH_MALLOC=system -DHWLOC_ROOT=/usr/local/hwloc/1.7.2 -DMPI_ROOT=/usr/local/mpi/mvapich2/2.3.4/intel/18.0 -MPI_CXX_COMPILER=/usr/local/mpi/mvapich2/2.3.4/intel/18.0/bin/mpicxx -DHPX_WITH_PARCELPORT_MPI=On -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DMPI_C_COMPILER=/usr/local/mpi/mvapich2/2.3.4/intel/18.0/bin/mpicc -DMPI_CXX_COMPILER=/usr/local/mpi/mvapich2/2.3.4/intel/18.0/bin/mpic++ ..

make -j 16 install

make -j 16 tests

make test

