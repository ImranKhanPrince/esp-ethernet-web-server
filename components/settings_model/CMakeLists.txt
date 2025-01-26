set(SOURCES "src/settings.cpp"
            "src/settings_nvs.cpp"
            "src/settings_helper.cpp")

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "." 
                       REQUIRES esp_http_server j4210u-driver_esp5.3 global_status nvs_flash
)
