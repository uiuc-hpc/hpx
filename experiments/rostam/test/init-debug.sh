#!/usr/bin/bash

# exit when any command fails
set -e
# import the the script containing common functions
source ../../include/scripts.sh

# get the ibvBench source path via environment variable or default value
HPX_SOURCE_PATH=$(realpath "${HPX_SOURCE_PATH:-../../../}")

if [[ -f "${HPX_SOURCE_PATH}/libs/full/include/include/hpx/hpx.hpp" ]]; then
  echo "Found HPX at ${HPX_SOURCE_PATH}"
else
  echo "Did not find HPX at ${HPX_SOURCE_PATH}!"
  exit 1
fi

# create the ./init directory
mkdir_s ./init
# move to ./init directory
cd init

# setup module environment
module purge
module load gcc
module load cmake
module load boost
module load hwloc
module load openmpi
module load papi
export CC=gcc
export CXX=g++

# record build status
record_env

mkdir -p log
mv *.log log

# build FB
mkdir -p build
cd build
echo "Running cmake..."
HPX_INSTALL_PATH=$(realpath "../install")
cmake -GNinja \
      -DCMAKE_INSTALL_PREFIX=${HPX_INSTALL_PATH} \
      -DCMAKE_BUILD_TYPE=Debug \
      -DHPX_WITH_MALLOC=system \
      -DHPX_WITH_PARCELPORT_MPI=ON \
      -DHPX_WITH_FETCH_ASIO=ON \
      -L \
      ${HPX_SOURCE_PATH} | tee init-cmake.log 2>&1 || { echo "cmake error!"; exit 1; }
cmake -LAH . >> init-cmake.log
echo "Running make..."
ninja | tee init-make.log 2>&1 || { echo "make error!"; exit 1; }
echo "Installing HPX to ${HPX_INSTALL_PATH}"
mkdir -p ${HPX_INSTALL_PATH}
ninja install > init-install.log 2>&1 || { echo "install error!"; exit 1; }
mv *.log ../log