#!/bin/bash -l

set -eux

orig_src_dir="$(pwd)"
src_dir="/dev/shm/hpx/src"
build_dir="/dev/shm/hpx/build"

# Copy source directory to /dev/shm for faster builds
mkdir -p "${build_dir}"
cp -r "${orig_src_dir}/." "${src_dir}"

source ${src_dir}/tools/jenkins/env-${configuration_name}.sh

ctest \
    --verbose \
    -S ${src_dir}/tools/jenkins/ctest.cmake \
    -DCTEST_CONFIGURE_EXTRA_OPTIONS="${configure_extra_options}" \
    -DCTEST_BUILD_CONFIGURATION_NAME="${configuration_name}" \
    -DCTEST_SOURCE_DIRECTORY="${src_dir}" \
    -DCTEST_BINARY_DIRECTORY="${build_dir}"
ctest_status=$?
echo "${ctest_status}" > "jenkins-hpx-${configuration_name}-ctest-status.txt"
