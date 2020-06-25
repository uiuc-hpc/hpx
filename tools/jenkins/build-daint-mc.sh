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

# Copy source directory to /dev/shm for faster builds
#orig_src_dir=$(pwd)
#src_dir=/dev/shm/hpx/src
#build_dir=/dev/shm/hpx/build
#
#mkdir -p $build_dir
#mkdir -p $src_dir
#
#cp -r $orig_src_dir/* $src_dir/
#
#cd $builddir
#rm -rf *

src_dir=$(pwd)/..
build_dir=$(pwd)

configure_extra_options="-DCMAKE_BUILD_TYPE=Debug"
configure_extra_options+=" -DHPX_WITH_MAX_CPU_COUNT=128"
configure_extra_options+=" -DHPX_WITH_MALLOC=system"
configuration_name="gcc-newest"

ctest \
    -S ${src_dir}/tools/jenkins/ctest.cmake \
    -DCTEST_CONFIGURE_EXTRA_OPTIONS="${configure_extra_options}" \
    -DCTEST_BUILD_CONFIGURATION_NAME="${configuration_name}" \
    -DCTEST_SOURCE_DIRECTORY="${src_dir}" \
    -DCTEST_BINARY_DIRECTORY="${build_dir}"
