# sources
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(RQC_APP_SOURCE_FILES
    rqc_app.c
)
add_executable(rqc-app ${RQC_APP_SOURCE_FILES})
target_include_directories(rqc-app PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(rqc-app PRIVATE
                      cpprestsdk::cpprest
                      rexapis
)

set(IBMQ_APP_SOURCE_FILES
    ibmq_app.c
)
add_executable(ibmq-app ${IBMQ_APP_SOURCE_FILES})
target_include_directories(ibmq-app PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(ibmq-app PRIVATE
                      cpprestsdk::cpprest
                      rexapis
)

set(SLURM_CAL_APP_SOURCE_FILES
    slurm_cal_app.c
)
add_executable(slurm-cal-app ${SLURM_CAL_APP_SOURCE_FILES})
target_include_directories(slurm-cal-app PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(slurm-cal-app PRIVATE
                      cpprestsdk::cpprest
                      rexapis
)

set(SLURM_APP_SOURCE_FILES
    slurm_app.c
)
add_executable(slurm-app ${SLURM_APP_SOURCE_FILES})
target_include_directories(slurm-app PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(slurm-app PRIVATE
                      cpprestsdk::cpprest
                      rexapis
)

set(OQTOPUS_APP_SOURCE_FILES
    oqtopus_app.c
)
add_executable(oqtopus-app ${OQTOPUS_APP_SOURCE_FILES})
target_include_directories(oqtopus-app PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(oqtopus-app PRIVATE
                      cpprestsdk::cpprest
                      rexapis
)

