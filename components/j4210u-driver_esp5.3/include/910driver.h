#ifndef DRIVER_910_H_INCLUDED
#define DRIVER_910_H_INCLUDED

#include "common.h"
#include "dtypes.h"

void printsettings(ReaderInfo *ri);

namespace mu910
{
  extern int baud_;

  extern bool transfer(unsigned char *command, int cmdsize, unsigned char *response, int responseSize, int sleepms, int pause); // needed for reader module detection
  extern void setHandle(HANDLE h);                                                                                              // temporary workaround to shift the functions one by one

  extern bool GetSettings(ReaderInfo *ri);
  extern bool SetSettings(ReaderInfo *ri);

  extern bool SetFilter(int maskAdrInByte, int maskLenInByte, unsigned char *maskDataByte);
  extern int Inventory(bool filter);
  extern int InventoryOne();
  extern bool InventoryNB(vector<ScanData> &v, bool filter);
  extern bool InventoryB(vector<ScanData> &v, bool filter);
  extern bool InventoryTID(vector<ScanData> &v);

  extern bool GetResult(unsigned char *scanresult, int index);

  extern bool GetTID(unsigned char *epc, unsigned char epclen, unsigned char *tid, unsigned char *tidlen);
  extern bool ReadMemWord(unsigned char *epc, unsigned char epclen, unsigned char *data, unsigned char windex);
  extern bool Read(unsigned char *epc, unsigned epclen, unsigned char *data, int wnum, int windex, int memtype);
  extern bool ReadWord(unsigned char *epc, unsigned char epclen, unsigned char *data, int windex, int memtype);

  extern bool Write(unsigned char *epc, unsigned char epcLen, unsigned char *data, int Wsize, int windex, int memtype);
  extern bool WriteWord(unsigned char *epc, unsigned char epcLen, unsigned char *data, unsigned char windex, int memtype);
  extern bool WriteMemWord(unsigned char *epc, unsigned char epclen, unsigned char *data, unsigned char windex);
  extern bool WriteEpc(unsigned char *epc, unsigned char epclen, unsigned char *data);
  extern bool WriteEpcWord(unsigned char *epc, unsigned char epclen, unsigned char *data, unsigned char windex);

  extern bool Auth(unsigned char *password, unsigned char size);
  extern bool SetPassword(unsigned char *epc, unsigned char epcLen, unsigned char *pass, unsigned char size);
  extern bool SetKillPassword(unsigned char *epc, unsigned char epcLen, unsigned char *pass, unsigned char size);

  extern char GetGPI(unsigned char gpiNumber); // extern c is missing cz of namespace
  extern bool SetGPO(unsigned char gpiNumber);

  extern bool SetQ(unsigned char q);
  extern bool SetSession(unsigned char sess);

  extern void LastError(char *buffer);
}

#endif // 910DRIVER_H_INCLUDED
