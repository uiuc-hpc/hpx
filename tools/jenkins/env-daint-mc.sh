#export CRAYPE_LINK_TYPE=dynamic
#
#module load daint-mc
#module load CMake
#module load hwloc/.2.0.3
#module load Boost
#
#export PATH=/apps/daint/UES/simbergm/spack/opt/spack/cray-cnl7-haswell/gcc-8.3.0/ninja-1.10.0-dcy5yzzldhss6wycy2ejjwj7o75dfddz/bin:$PATH

export CRAYPE_LINK_TYPE=dynamic

module load daint-gpu
module load cudatoolkit

export BOOST_ROOT=$LOCAL_INSTALL/boost-1.69.0-gcc-8.3.0-c++17-release
export HWLOC_ROOT=$LOCAL_INSTALL/hwloc-2.0.3-gcc-8.3.0
export CXX=$(which CC)
export CC=$(which cc)

export PATH=/apps/daint/UES/simbergm/spack/opt/spack/cray-cnl7-haswell/gcc-8.3.0/ninja-1.10.0-dcy5yzzldhss6wycy2ejjwj7o75dfddz/bin:$PATH
