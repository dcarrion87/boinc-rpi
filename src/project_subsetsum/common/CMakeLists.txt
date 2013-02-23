add_library(sss_common binary_output generate_subsets n_choose_k)

include(FindCUDA)

if (CUDA_FOUND)
    #   cuda_add_library(sss_common_cuda generate_subsets_cuda)
    #   target_link_libraries(SubsetSum_${SUBSET_SUM_VERSION_MAJOR}.${SUBSET_SUM_VERSION_MINOR}_cuda ${BOINC_APP_LIBRARIES} ${SSS_COMMON_LIBRARY})
endif (CUDA_FOUND)
