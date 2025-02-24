#pragma once

typedef enum
{
  MEM_WRITE_FAILED,
  TAG_NOT_FOUND,
  MEM_WRITE_SUCCESSFUL
} MEM_WRITE_STAUTUS;

#ifdef __cplusplus
extern "C"
{
#endif

  char *get_tid_memory(char *epc_num);
  char *get_usr_memory(char *epc_num, int wnum, int windex);
  MEM_WRITE_STAUTUS change_epc(char *old_epc, char *new_epc);
  MEM_WRITE_STAUTUS change_user_mem(char *epc_num, char *data, int wnum, int windex);

#ifdef __cplusplus
}
#endif