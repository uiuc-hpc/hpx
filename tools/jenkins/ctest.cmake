#  Copyright (c) 2020 ETH Zurich
#  Copyright (c) 2017 John Biddiscombe
#
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

find_package(Git REQUIRED)

set(CTEST_TEST_TIMEOUT 200)
set(CTEST_BUILD_PARALLELISM 36)
set(CTEST_TEST_PARALLELISM 8)
set(CTEST_CMAKE_GENERATOR Ninja)
set(CTEST_SITE "cscs(daint)")
set(CTEST_BUILD_NAME "$ENV{ghprbPullId}-$ENV{CTEST_BUILD_CONFIGURATION_NAME}")
set(CTEST_GIT_COMMAND "${GIT_EXECUTABLE}")

# TODO: Can we build master?
set(CTEST_SUBMISSION_TRACK "Pull_Requests")
set(CTEST_CONFIGURE_COMMAND "${CMAKE_COMMAND} ${CTEST_SOURCE_DIRECTORY}")
set(CTEST_CONFIGURE_COMMAND "${CTEST_CONFIGURE_COMMAND} -G${CTEST_CMAKE_GENERATOR}")
set(CTEST_CONFIGURE_COMMAND "${CTEST_CONFIGURE_COMMAND} -B${CTEST_BINARY_DIRECTORY}")
set(CTEST_CONFIGURE_COMMAND "${CTEST_CONFIGURE_COMMAND} ${CTEST_CONFIGURE_EXTRA_OPTIONS}")

ctest_start(Experimental TRACK "${CTEST_SUBMISSION_TRACK}")
ctest_update()
ctest_submit(PARTS Update)
ctest_configure()
ctest_submit(PARTS Configure)
#ctest_build(TARGET tests.unit.modules.algorithms.adjacentdifference)
#ctest_submit(PARTS Build)
#ctest_test(
  #PARALLEL_LEVEL "${CTEST_TEST_PARALLELISM}"
  #INCLUDE tests.unit.modules.algorithms.adjacentdifference)
ctest_submit(PARTS Test BUILD_ID CTEST_BUILD_ID)
file(WRITE "ctest_build_id.txt" "${CTEST_BUILD_ID}")
