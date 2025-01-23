idf_component_register(SRCS "settings.cpp" "settings_nvs.cpp"
                       INCLUDE_DIRS "." 
                       REQUIRES esp_http_server j4210u-driver_esp5.3 global_status nvs_flash
)
