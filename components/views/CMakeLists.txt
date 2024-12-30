set(SOURCES "api/api_json.c" 
            "settings/settings.c")

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "." "include" 
                       REQUIRES esp_http_server settings json
)


                      #  EMBED_FILES "html/index.html"
                      #  "html/assets/styles/style.css"
                      #  "html/assets/scripts/app.js"
                      #  "html/assets/images/favicon.ico"