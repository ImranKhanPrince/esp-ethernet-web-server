set(SOURCES "api_info_view/api_json.c" 
            "settings_view/settings_view.c"
            "scan_view/scan_view.cpp"
            "scan_view/memory_view.cpp"
            )

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "." 
                       REQUIRES esp_http_server models_tasks json global_status
)


                      #  EMBED_FILES "html/index.html"
                      #  "html/assets/styles/style.css"
                      #  "html/assets/scripts/app.js"
                      #  "html/assets/images/favicon.ico"