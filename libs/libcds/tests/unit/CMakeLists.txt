# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests # libcds_flat_combining_priority_queue
    libcds_michael_map_hazard_pointer
    libcds_feldman_map_hazard_pointer
    libcds_split_list_map_hazard_pointer
    libcds_michael_map_hazard_pointer_stdthread
    libcds_feldman_map_hazard_pointer_stdthread
    libcds_split_list_map_hazard_pointer_stdthread
)

set(libcds_flat_combining_priority_queue THREADS_PER_LOCALITY 4)
set(libcds_michael_map_hazard_pointer THREADS_PER_LOCALITY 4)
set(libcds_feldman_map_hazard_pointer THREADS_PER_LOCALITY 4)
set(libcds_split_list_map_hazard_pointer THREADS_PER_LOCALITY 4)

foreach(test ${tests})

  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    DEPENDENCIES hpx_libcds
    FOLDER "Tests/Unit/Modules/LibCDS"
  )

  target_include_directories(${test}_test PRIVATE ${LIBCDS_SOURCE_DIR})

  add_hpx_unit_test("modules.libcds" ${test} ${${test}_PARAMETERS})

endforeach()
