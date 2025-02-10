set(SOURCES "scan_task.cpp"
            "helper_formatter.cpp"
            "helper_func.cpp"
            "tag_memory_ops.cpp")

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS "." "include"
                        REQUIRES j4210u-driver_esp5.3 settings_model global_status json messaging driver
)