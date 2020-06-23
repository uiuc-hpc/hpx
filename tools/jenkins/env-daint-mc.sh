#export CRAYPE_LINK_TYPE=dynamic
#
#module load daint-mc
#module load CMake
#module load hwloc/.2.0.3
#module load Boost
#
#export PATH=/apps/daint/UES/simbergm/spack/opt/spack/cray-cnl7-haswell/gcc-8.3.0/ninja-1.10.0-dcy5yzzldhss6wycy2ejjwj7o75dfddz/bin:$PATH

#export CRAYPE_LINK_TYPE=dynamic
#
#module load daint-gpu
#module load CMake
#module load cudatoolkit
#
#export LOCAL_INSTALL=/apps/daint/UES/simbergm/local
#export BOOST_ROOT=$LOCAL_INSTALL/boost-1.69.0-gcc-8.3.0-c++17-release
#export HWLOC_ROOT=$LOCAL_INSTALL/hwloc-2.0.3-gcc-8.3.0
#export CXX=$(which CC)
#export CC=$(which cc)
#
#export PATH=/apps/daint/UES/simbergm/spack/opt/spack/cray-cnl7-haswell/gcc-8.3.0/ninja-1.10.0-dcy5yzzldhss6wycy2ejjwj7o75dfddz/bin:$PATH
export CRAYPE_LINK_TYPE=dynamic

export LOCAL_ROOT="/apps/daint/UES/simbergm/local"
export GCC_VER="10.1.0"
export CXX_STD="17"
export BOOST_VER="1.73.0"
export TCMALLOC_VER="2.7"
export HWLOC_VER="2.2.0"
export GCC_ROOT="${LOCAL_ROOT}/gcc-${GCC_VER}"
export BOOST_ROOT="${LOCAL_ROOT}/boost-${BOOST_VER}-gcc-${GCC_VER}-c++${CXX_STD}-debug"
export HWLOC_ROOT="${LOCAL_ROOT}/hwloc-${HWLOC_VER}-gcc-${GCC_VER}"
export TCMALLOC_ROOT="${LOCAL_ROOT}/gperftools-${TCMALLOC_VER}-gcc-${GCC_VER}"
export CFLAGS=""
export CXXFLAGS="-fdiagnostics-color=always -nostdinc++ -I${GCC_ROOT}/include/c++/${GCC_VER} -I${GCC_ROOT}/include/c++/${GCC_VER}/x86_64-unknown-linux-gnu -I${GCC_ROOT}/include/c++/${GCC_VER}/x86_64-pc-linux-gnu -L${GCC_ROOT}/lib64 -Wl,-rpath,${GCC_ROOT}/lib64"
export LDFLAGS="-L${GCC_ROOT}/lib64"
export LDFLAGS="-L${GCC_ROOT}/lib64"
export CXX=${GCC_ROOT}/bin/g++
export CC=${GCC_ROOT}/bin/gcc

module load daint-mc
module load CMake
