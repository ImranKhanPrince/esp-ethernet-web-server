set(COMPONENT_INCLUDE_DIRS 
  "settings_model"
  "uhf_operations/include"
  "messaging"
)

set(SETTINGS_MODEL_SRCS
  "settings.cpp"
  "settings_nvs.cpp"
  "settings_helper.cpp"
)

set(SCAN_TASK_SRCS
  "scan_task.cpp"
  "helper_formatter.cpp"
  "helper_func.cpp"
  "tag_memory_ops.cpp"
)

set(MESSAGING_TASK_SRCS
  "socket_message.cpp"
)

set(COMPONENT_REQUIRES 
  esp_http_server 
  j4210u-driver_esp5.3
  global_status 
  nvs_flash
  json
  driver
  esp_timer 
  esp_http_client
)

# function to prepend the sources
macro(prepend_srcs PREFIX SRC_LIST)
  foreach(src IN LISTS ${SRC_LIST})
    list(APPEND COMPONENT_SRCS "${PREFIX}/${src}")
  endforeach()
endmacro()

set(COMPONENT_SRCS "")
prepend_srcs("settings_model/src" SETTINGS_MODEL_SRCS)
prepend_srcs("uhf_operations/src" SCAN_TASK_SRCS)
prepend_srcs("messaging" MESSAGING_TASK_SRCS)

idf_component_register(SRCS ${COMPONENT_SRCS}
                       INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}  
                       REQUIRES ${COMPONENT_REQUIRES}
)