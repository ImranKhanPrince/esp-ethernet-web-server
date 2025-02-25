#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "sdmmc_helper.h"

#define TAG "sdmmc"
#define MAX_RW_CHAR_SIZE 64
#define MOUNT_POINT "/sdcard"

sdmmc_card_t *card_;
char *mount_point_;

bool sd_init_mount(const char *mount_point)
{
  esp_err_t ret;
  ESP_LOGI(TAG, "Initializing SD card");
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};
  if (strlen(mount_point) <= 0)
  {
    strcpy(mount_point, MOUNT_POINT);
  }

  ESP_LOGI(TAG, "Configuring SDMMC peripheral");
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();                      // 20mhz frequency by default
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT(); // Default gpio config
  slot_config.width = 1;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP; // external pullup is in hardware available still using internal for safety

  ESP_LOGI(TAG, "Mounting filesystem");
  ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card_);
  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount filesystem. "
                    "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                    "Make sure SD card lines have pull-up resistors in place.",
               esp_err_to_name(ret));
    }
    return false;
  }
  ESP_LOGI(TAG, "Filesystem mounted");

  mount_point_ = strdup(mount_point);

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, card_);
  return true;
}

void sd_unmount(const char *mount_point)
{
  esp_vfs_fat_sdcard_unmount(mount_point, card_);
}

bool sd_write_file(const char *path, char *data)
{
  ESP_LOGI(TAG, "Opening file %s", path);
  FILE *f = fopen(path, "w");
  if (f == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return false;
  }
  fprintf(f, data);
  fclose(f);
  ESP_LOGI(TAG, "File written");

  return true;
}

bool sd_read_file(const char *path, char *buf, size_t buf_size)
{
  ESP_LOGI(TAG, "Reading file %s", path);
  FILE *f = fopen(path, "r");
  if (f == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return false;
  }

  size_t bytes_read = fread(buf, 1, buf_size - 1, f);

  if (bytes_read > 0)
  {
    buf[bytes_read] = '\0'; // Null-terminate the buffer
  }
  else
  {
    buf[0] = '\0'; // Ensure buffer is empty if nothing was read
  }
  fclose(f);
  ESP_LOGI(TAG, "Read %d bytes from file", bytes_read);
  // strip newline
  char *pos = strchr(buf, '\n');
  if (pos)
  {
    *pos = '\0';
  }
  ESP_LOGI(TAG, "Read from file: '%s'", buf);

  return true;
}
bool sd_read_json_file(const char *path, char *buf, size_t buf_size)
{
  ESP_LOGI(TAG, "Reading JSON file %s", path);
  FILE *f = fopen(path, "r");
  if (f == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return false;
  }

  char temp_buf[2]; // Temporary buffer to read one character at a time
  size_t bytes_read = 0;
  bool json_started = false;
  int brace_count = 0;

  while (bytes_read < buf_size - 1 && fread(temp_buf, 1, 1, f) == 1)
  {
    if (temp_buf[0] == '{')
    {
      json_started = true;
      brace_count++;
    }

    if (json_started)
    {
      buf[bytes_read++] = temp_buf[0];

      if (temp_buf[0] == '{')
      {
        brace_count++;
      }
      else if (temp_buf[0] == '}')
      {
        brace_count--;
      }

      if (brace_count == 0)
      {
        break; // JSON is complete
      }
    }
  }

  fclose(f);

  if (json_started)
  {
    buf[bytes_read] = '\0'; // Null-terminate the buffer
    ESP_LOGI(TAG, "Read JSON from file: '%s'", buf);
    return true;
  }
  else
  {
    ESP_LOGW(TAG, "No JSON found in file");
    buf[0] = '\0'; // Ensure buffer is empty
    return false;
  }
}