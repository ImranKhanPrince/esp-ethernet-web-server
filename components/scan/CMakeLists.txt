set(SOURCES "scan_task.cpp")

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS "."
                        REQUIRES j4210u-driver_esp5.3 settings_model
)