#pragma once

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

  extern sdmmc_card_t *card_;
  extern char *mount_point_;
  extern bool sd_init_mount(const char *mount_point);
  extern void sd_unmount(const char *mount_point);

  extern bool sd_write_file(const char *path, char *data);
  extern bool sd_read_file(const char *path, char *buf, size_t buf_size);
  extern bool sd_read_json_file(const char *path, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif // __cplusplus