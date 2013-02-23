find_package(MySQL REQUIRED)
find_package(BOINC REQUIRED)

find_path(BOINC_INCLUDE_DIR config.h PATHS /boinc/src/boinc/ /boinc/)

include_directories(
    ${PROJECT_SOURCE_DIR}
	${BOINC_INCLUDE_DIR}
    ${BOINC_INCLUDE_DIR}/api
    ${BOINC_INCLUDE_DIR}/db
    ${BOINC_INCLUDE_DIR}/lib
    ${BOINC_INCLUDE_DIR}/sched
	${MYSQL_INCLUDE_DIR}
)

add_executable(sss_validator
    ${BOINC_INCLUDE_DIR}/sched/validator
    ${BOINC_INCLUDE_DIR}/sched/validate_util
    ${BOINC_INCLUDE_DIR}/sched/validate_util2
    sss_validation_policy)
target_link_libraries(sss_validator undvc_common ${BOINC_LIBRARIES} ${MYSQL_LIBRARIES})

add_executable(sss_assimilator
    ${BOINC_INCLUDE_DIR}/sched/assimilator
    ${BOINC_INCLUDE_DIR}/sched/validate_util
    sss_assimilation_policy)
target_link_libraries(sss_assimilator undvc_common ${BOINC_LIBRARIES} ${MYSQL_LIBRARIES} ${Boost_LIBRARIES})

#find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
    ${BOINC_INCLUDE_DIR}/tools/
)

add_executable(sss_work_generator sss_work_generator)
target_link_libraries(sss_work_generator undvc_common ${BOINC_LIBRARIES} ${MYSQL_LIBRARIES} ${OPENSSL_LIBRARIES})

add_definitions( -DHTML_OUTPUT -DENABLE_COLOR)
add_executable(sss_webpage_generator
    sss_webpage_generator
    ../common/generate_subsets
    ../common/n_choose_k
    ../common/binary_output)
target_link_libraries(sss_webpage_generator undvc_common ${BOINC_LIBRARIES} ${MYSQL_LIBRARIES})

