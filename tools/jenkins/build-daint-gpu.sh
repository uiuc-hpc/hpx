#!/bin/bash -l
#SBATCH --job-name=build-daint-gpu
#SBATCH --nodes=1
#SBATCH --constraint=mc
#SBATCH --partition=cscsci
#SBATCH --time=06:00:00
#SBATCH --output=build-daint-gpu.out
#SBATCH --error=build-daint-gpu.err

set -eux

source ../tools/jenkins/env-daint-gpu.sh

mkdir -p build
cd build
export CXX=CC
export CC=cc
cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBOOST_ROOT=$BOOST_ROOT \
    -DHPX_PROGRAM_OPTIONS_WITH_BOOST_PROGRAM_OPTIONS_COMPATIBILITY=OFF \
    -DHWLOC_ROOT=$HWLOC_ROOT \
    -DHPX_WITH_MALLOC=system \
    -DHPX_WITH_CUDA=ON \
    -DHPX_WITH_CUDA_CLANG=ON \
    -DHPX_CUDA_CLANG_FLAGS"=--cuda-gpu-arch=sm_60"
ninja all tests examples
