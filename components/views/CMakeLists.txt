set(SOURCES "api_info_view/api_json.c" 
            "settings_view/settings.c"
            "scan_view/scan.c")

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "." "include" 
                       REQUIRES esp_http_server settings_model json
)


                      #  EMBED_FILES "html/index.html"
                      #  "html/assets/styles/style.css"
                      #  "html/assets/scripts/app.js"
                      #  "html/assets/images/favicon.ico"