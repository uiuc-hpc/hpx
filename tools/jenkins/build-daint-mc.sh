#!/bin/bash -l
#SBATCH --job-name=build-mc-gpu
#SBATCH --nodes=1
#SBATCH --constraint=mc
#SBATCH --partition=cscsci
#SBATCH --time=06:00:00
#SBATCH --output=build-daint-mc.out
#SBATCH --error=build-daint-mc.err

set -eux

source ../tools/jenkins/env-daint-mc.sh

srcdir=$(pwd)
builddir=/dev/shm/hpx/build

mkdir -p $builddir
cd $builddir
rm -rf *
cmake $srcdir \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DHPX_WITH_MALLOC=system

ninja -j30 all tests

ctest --output-on-failure
