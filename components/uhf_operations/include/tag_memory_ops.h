#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

  char *get_tid_memory(char *epc_num);
  char *get_usr_memory(char *epc_num, int wnum, int windex);

#ifdef __cplusplus
}
#endif