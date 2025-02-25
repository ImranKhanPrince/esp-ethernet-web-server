set(SOURCES 
  "ethernet_init/ethernet_init.c"
  "sdmmc/sdmmc_helper.c"
)

idf_component_register(SRCS ${SOURCES} 
                      INCLUDE_DIRS "ethernet_init" "sdmmc"
                      REQUIRES driver esp_eth fatfs
)