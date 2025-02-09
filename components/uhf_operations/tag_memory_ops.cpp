#include <string.h>
#include "driver.h"

#include "tag_memory_ops.h"
#include "helper_func.h"
#include "global_status.h"

#define MIN_EPC_STR_SIZE 24
#define WORD_TO_BYTE 2
#define BYTE_TO_STR 2

char *get_tid_memory(char *epc_num)
{
  int epc_size = strlen(epc_num);
  if (epc_size < MIN_EPC_STR_SIZE)
  {
    return strdup("{\"error\": \"EPC Size is wrong\"}");
  }

  unsigned char epc_byte_array[MIN_EPC_STR_SIZE / 2];
  unsigned char tid_byte_array[MIN_EPC_STR_SIZE / 2];

  int result = hexstr_to_byte_array(epc_num, epc_byte_array, sizeof(epc_byte_array));
  if (result != 0)
  {
    return strdup("{\"error\": \"Invalid EPC format\"}");
  }
  print_byte_array(epc_byte_array, sizeof(epc_byte_array));

  unsigned char tid_size = 0xFF;
  //---- Critical Section
  if (xSemaphoreTake(xUhfUartMutex, portMAX_DELAY) == pdTRUE)
  {
    GetTID(epc_byte_array, (unsigned char)sizeof(epc_byte_array), tid_byte_array, &tid_size);
    xSemaphoreGive(xUhfUartMutex);
  }
  // ---- END Critical Section
  if (tid_size == 0xFF)
  {
    return strdup("{\"error\": \"can't find the tag nearby\"}");
  }
  print_byte_array(tid_byte_array, tid_size);

  char tid_hex_string[MIN_EPC_STR_SIZE + 1];
  result = byte_array_to_hexstr(tid_byte_array, sizeof(epc_byte_array), tid_hex_string, sizeof(tid_hex_string));
  if (result == 0)
  {
    printf("Hex String : %s\n", tid_hex_string);
  }
  else
  {
    return strdup("{\"error\": \"corrupted tid value\"}");
  }
  char response[50];
  sprintf(response, "{\"tid\":\"%s\"}", tid_hex_string);
  return strdup(response);
}

char *get_usr_memory(char *epc_num, int wnum, int windex)

{

  int epc_size = strlen(epc_num);
  if (epc_size < MIN_EPC_STR_SIZE)
  {
    return strdup("{\"error\": \"EPC Size is wrong\"}");
  }
  unsigned char epc_byte_array[MIN_EPC_STR_SIZE / 2];

  int result = hexstr_to_byte_array(epc_num, epc_byte_array, sizeof(epc_byte_array));
  if (result != 0)
  {
    return strdup("{\"error\": \"Invalid EPC format\"}");
  }
  print_byte_array(epc_byte_array, sizeof(epc_byte_array));
  unsigned char data[wnum * WORD_TO_BYTE];
  bool ok = Read(epc_byte_array, sizeof(epc_byte_array), data, wnum, windex, MEM_USER);
  if (!ok)
  {
    return strdup("{\"error\": \"failed to read memory\"}");
  }
  char data_str[wnum * WORD_TO_BYTE * BYTE_TO_STR + 1];
  byte_array_to_hexstr(data, sizeof(data), data_str, sizeof(data_str));
  printf("DATA:%s\n", data_str);
  char buf[sizeof(data_str) + 20];
  sprintf(buf, "{\"user_memory\":\"%s\"}", data_str);
  return strdup(buf);
}

MEM_WRITE_STAUTUS change_epc(char *old_epc, char *new_epc)
{
  unsigned char new_epc_byte_array[MIN_EPC_STR_SIZE / 2];
  unsigned char old_epc_byte_array[MIN_EPC_STR_SIZE / 2];

  hexstr_to_byte_array(new_epc, new_epc_byte_array, sizeof(new_epc_byte_array));
  hexstr_to_byte_array(old_epc, old_epc_byte_array, sizeof(old_epc_byte_array));
  if (!TagExists(old_epc_byte_array, sizeof(old_epc_byte_array)))
  {
    return TAG_NOT_FOUND;
  }

  bool ok = WriteEpc(old_epc_byte_array, sizeof(old_epc_byte_array), new_epc_byte_array);
  if (!ok)
  {
    return MEM_WRITE_FAILED;
  }
  return MEM_WRITE_SUCCESSFUL;
}

MEM_WRITE_STAUTUS change_user_mem(char *epc_num, char *data, int wnum, int windex)
{
  unsigned char epc_byte_array[MIN_EPC_STR_SIZE / 2];
  unsigned char data_byte_array[strlen(data) / 2];
  printf("EPC: %s\n", epc_num);
  hexstr_to_byte_array(epc_num, epc_byte_array, sizeof(epc_byte_array));
  hexstr_to_byte_array(data, data_byte_array, sizeof(data_byte_array));
  print_byte_array(epc_byte_array, sizeof(epc_byte_array));

  if (!TagExists(epc_byte_array, sizeof(epc_byte_array)))
  {
    return TAG_NOT_FOUND;
  }
  bool ok = Write(epc_byte_array, sizeof(epc_byte_array), data_byte_array, sizeof(data_byte_array), windex, MEM_USER);
  if (!ok)
  {
    return MEM_WRITE_FAILED;
  }
  return MEM_WRITE_SUCCESSFUL;
}