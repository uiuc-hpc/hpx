export CRAYPE_LINK_TYPE=dynamic

module load daint-gpu
module load CMake
module load cudatoolkit

export BOOST_ROOT=/apps/daint/UES/simbergm/local/boost-1.69.0-gcc-8.3.0-c++17-release
export HWLOC_ROOT=/apps/daint/UES/simbergm/local/hwloc-2.0.3-gcc-8.3.0
export PATH=/apps/daint/UES/simbergm/spack/opt/spack/cray-cnl7-haswell/gcc-8.3.0/ninja-1.10.0-dcy5yzzldhss6wycy2ejjwj7o75dfddz/bin:$PATH
