#!/bin/bash -l
#SBATCH --job-name=jenkins-hpx-mc
#SBATCH --nodes=1
#SBATCH --constraint=mc
#SBATCH --partition=cscsci
#SBATCH --time=02:00:00
#SBATCH --output=jenkins-hpx-log.out
#SBATCH --error=jenkins-hpx-log.err

set -eu

orig_src_dir=$(pwd)
src_dir=/dev/shm/hpx/src
build_dir=/dev/shm/hpx/build

# Copy source directory to /dev/shm for faster builds
mkdir -p $build_dir
mkdir -p $src_dir
cp -r $orig_src_dir/* $src_dir/

cd $builddir
rm -rf *

source ${src_dir}/tools/jenkins/env-${configuration_name}.sh

ctest \
    -S ${src_dir}/tools/jenkins/ctest.cmake \
    -DCTEST_CONFIGURE_EXTRA_OPTIONS="${configure_extra_options}" \
    -DCTEST_BUILD_CONFIGURATION_NAME="${configuration_name}" \
    -DCTEST_SOURCE_DIRECTORY="${src_dir}" \
    -DCTEST_BINARY_DIRECTORY="${build_dir}"
