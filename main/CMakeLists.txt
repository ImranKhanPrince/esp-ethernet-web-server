idf_component_register(SRCS "app_main.cpp" "server.c"
                       INCLUDE_DIRS "."
                       EMBED_FILES "html/index.html"
                                   "html/assets/styles/style.css"
                                   "html/assets/scripts/app.js"
                                   "html/assets/images/favicon.ico")