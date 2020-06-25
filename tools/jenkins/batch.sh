#!/bin/bash -l

set -eux

orig_src_dir=$(pwd)
src_dir=/dev/shm/hpx/src
build_dir=/dev/shm/hpx/build

# Copy source directory to /dev/shm for faster builds
rm -rf $build_dir
rm -rf $src_dir
mkdir -p $build_dir
mkdir -p $src_dir
cp -r $orig_src_dir/. $src_dir

source ${src_dir}/tools/jenkins/env-${configuration_name}.sh

ctest \
    --verbose \
    -S ${src_dir}/tools/jenkins/ctest.cmake \
    -DCTEST_CONFIGURE_EXTRA_OPTIONS="${configure_extra_options}" \
    -DCTEST_BUILD_CONFIGURATION_NAME="${configuration_name}" \
    -DCTEST_SOURCE_DIRECTORY="${src_dir}" \
    -DCTEST_BINARY_DIRECTORY="${build_dir}"

