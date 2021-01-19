#!/bin/bash

module load boost/1.71.0
module load cmake/.3.18.4
module load hwloc/1.7.2
#module load openmpi/3.1.1-intel-18.0
module load mvapich2/.2.3.4-intel-18.0

echo $PATH
#export PATH=$PATH:/home/ekoning2/scratch/HPXLCI/LC/install/usr/local
#export PATH=$PATH:/home/ekoning2/scratch/HPXLCI/LC/include

if [ -n CPLUS_INCLUDE_PATH ]; then
	#export CPLUS_INCLUDE_PATH=/usr/local/hwloc/1.7.2/include:/usr/local/hwloc/1.7.2/include/hwloc
	export CPLUS_INCLUDE_PATH=/usr/local/hwloc/1.7.2/include:/usr/local/hwloc/1.7.2/include/hwloc:/usr/local/boost/1.71.0/include
	# not sure if boost needs to be in PATH or not --must test
	export PATH=$PATH:/usr/local/boost/1.71.0/include/boost
fi

# At one point, PATH worked for openmpi with:
# /usr/local/mpi/openmpi/3.1.1/intel/18.0/bin:/usr/local/mpi/mvapich2/2.3.4/intel/18.0/bin:/usr/local/intel/parallel_studio_xe_2018/advisor_2018.1.1.535164/bin64:/usr/local/intel/parallel_studio_xe_2018/vtune_amplifier_2018.1.0.535340/bin64:/usr/local/intel/parallel_studio_xe_2018/inspector_2018.1.1.535159/bin64:/usr/local/intel/parallel_studio_xe_2018/itac/2018.1.017/intel64/bin:/usr/local/intel/parallel_studio_xe_2018/itac/2018.1.017/intel64/bin:/usr/local/intel/parallel_studio_xe_2018/clck/2018.1/bin/intel64:/usr/local/intel/parallel_studio_xe_2018/compilers_and_libraries_2018.1.163/linux/bin/intel64:/usr/local/intel/parallel_studio_xe_2018/compilers_and_libraries_2018.1.163/linux/mpi/intel64/bin:/usr/local/hwloc/1.7.2/bin:/usr/local/hwloc/1.7.2/sbin:/usr/local/cmake/3.18.4/bin:/usr/local/python/2.7.15/bin:/usr/local/python/3.7.0/bin:/usr/local/gcc/7.2.0/bin:/usr/local/vim/8.1-el7/bin:/usr/local/src/git/2.19.0/bin:/usr/local/slurm/current/bin:/usr/lib64/qt-3.3/bin:/usr/local/perlbrew/bin:/usr/local/moab-releases/moab-9.1.2-el7/bin:/usr/local/bin:/usr/bin:/usr/local/sbin:/usr/sbin:/sbin:/opt/ibutils/bin:/bin:/usr/local/slurm/20.02.3/bin:/usr/local/slurm/20.02.3/sbin:/usr/local/intel/parallel_studio_xe_2018/parallel_studio_xe_2018.1.038/bin:/home/ekoning2/bin:/usr/local/hwloc/1.7.2/include:/home/ekoning2/scratch/HPXLCI/LC/include:/home/ekoning2/scratch/HPXLCI/LC/install/usr/local:/usr/local/boost/1.71.0/include/boost


mkdir -p build && cd build

#cmake -DCMAKE_INSTALL_PREFIX=/home/ekoning2/scratch/HPXLCI/replace_mpi/hpx/build/install -DBOOST_ROOT=/usr/local/boost/1.71.0 -DHPX_WITH_MALLOC=system -DHWLOC_ROOT=/usr/local/hwloc/1.7.2 -DMPI_ROOT=/usr/local/openmpi/3.1.3/gcc/8.2.0/ -MPI_CXX_COMPILER=/usr/local/openmpi/3.1.3/gcc/8.2.0/bin/mpicxx -DHPX_WITH_PARCELPORT_MPI=On -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DMPI_C_COMPILER=/usr/local/openmpi/3.1.3/gcc/8.2.0/bin/mpicc -DMPI_CXX_COMPILER=/usr/local/openmpi/3.1.3/gcc/8.2.0/bin/mpic++ ..
cmake -DCMAKE_INSTALL_PREFIX=/home/ekoning2/scratch/HPXLCI/replace_mpi/hpx/build/install -DBOOST_ROOT=/usr/local/boost/1.71.0 -DHPX_WITH_MALLOC=system -DHWLOC_ROOT=/usr/local/hwloc/1.7.2 -DMPI_ROOT=/usr/local/mpi/mvapich2/2.3.4/intel/18.0 -MPI_CXX_COMPILER=/usr/local/mpi/mvapich2/2.3.4/intel/18.0/bin/mpicxx -DHPX_WITH_PARCELPORT_MPI=On -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DMPI_C_COMPILER=/usr/local/mpi/mvapich2/2.3.4/intel/18.0/bin/mpicc -DMPI_CXX_COMPILER=/usr/local/mpi/mvapich2/2.3.4/intel/18.0/bin/mpic++ ..

make -j 16 install

make -j 16 tests

make test

