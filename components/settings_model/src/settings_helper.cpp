#include "settings.h"

void print_device_func_settings(device_func_status_t *func_settings)
{
  printf("\n{\n");
  printf("    data_output_loc: %s,\n", func_settings->data_output_loc);
  printf("    can_interval: %d,\n", func_settings->scan_interval);
  printf("    trigger: %d,\n", func_settings->trigger);
  printf("}\n");
}