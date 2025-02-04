#include "api_json.h"

char *view_handle_memory_command(const char *data)
{
  return strdup("{status: \"success\"}");
}

char *get_tid_memory(const char *data)
{
  return strdup("{\"tid\": \"0x12345678\"}");
}

char *get_usr_memory(const char *data)
{
  return strdup("{\"usr_mem\": \"0x12345678\"}");
}