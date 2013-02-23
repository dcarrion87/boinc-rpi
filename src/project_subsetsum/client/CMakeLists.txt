include_directories(${BOINC_INCLUDE_DIR})
include_directories(${BOINC_INCLUDE_DIR}/api)
include_directories(${BOINC_INCLUDE_DIR}/lib)

find_package(CUDA)
find_package(BOINC)

include(FindCUDA)

message(STATUS "CUDA DIR: ${CUDA_SDK_ROOT_DIR}")

link_libraries("-L'${CUDA_SDK_ROOT_DIR}/lib' -lcutil_x86_64")

add_definitions( -D_BOINC_ -DENABLE_CHECKPOINTING -DVERBOSE -DFALSE_ONLY -D__STDC_LIMIT_MACROS)

add_executable(SubsetSum_${SUBSET_SUM_VERSION_MAJOR}.${SUBSET_SUM_VERSION_MINOR} subset_sum_main)
target_link_libraries(SubsetSum_${SUBSET_SUM_VERSION_MAJOR}.${SUBSET_SUM_VERSION_MINOR} ${BOINC_APP_LIBRARIES} sss_common)

if (CUDA_FOUND)
    #   cuda_add_executable(SubsetSum_${SUBSET_SUM_VERSION_MAJOR}.${SUBSET_SUM_VERSION_MINOR}_cuda subset_sum_main)
    #   target_link_libraries(SubsetSum_${SUBSET_SUM_VERSION_MAJOR}.${SUBSET_SUM_VERSION_MINOR}_cuda ${BOINC_APP_LIBRARIES} sss_common_cuda)
endif (CUDA_FOUND)
