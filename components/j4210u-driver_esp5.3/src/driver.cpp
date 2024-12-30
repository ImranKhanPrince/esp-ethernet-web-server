#include <stdio.h>
#include "platform.h"
#include "driver.h"
#include "910driver.h"
#include "903driver.h"
#include "common.h"

#include <iostream>
#include <iomanip> // For setw
#include <time.h>
#include <string>

#define PRESET_VALUE 0xFFFF
#define POLYNOMIAL 0x8408

#ifdef LINUX
#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif // CLK_TCK
#define CLK_PER_MS (CLK_TCK / 1000)
#else
#define CLK_PER_MS (CLK_TCK / 1000)
#endif // LINUX

bool Debug_ = true;
static string password_ = string(4, 0);
static Filter filter_; // MaskAdr*, MaskLen and MaskData*
vector<ScanData> scan_;
extern string error_;
int baud_;

static bool littlend_ = false;
static bool mu910_ = false;

unsigned char session_ = 0x00;
unsigned char q_ = 2;

HANDLE h_ = UART_NUM_MAX;

static void emptyserial()
{
    unsigned char buff[128];
    // empty the serial buffer
    platform_read(h_, buff, sizeof(buff), (unsigned short int *)buff, 0);
}

bool is910()
{
    unsigned char command[] = {0xCF, 0xFF, 0x00, 0x50, 0x00, 0x00, 0x00}; // RFM_MODULE_INIT
    unsigned char response[8];
    int retry = 3;
    bool ok;
    do
    {
        emptyserial();
        ok = mu910::transfer(command, sizeof(command), response, sizeof(response), 10, 20);
    } while (retry-- && !ok);

    if (ok && response[5] == 0x00)
    {
        mu910_ = true;
        if (Debug_)
        {
            printf("The Hardware is mu910...\n");
        }
        return true;
    }
    return false;
}

bool isLittleEndian()
{
    int n = 0x1234;
    int n1 = ((unsigned char *)&n)[0];
    int n2 = n & 0xff;
    if (n1 == n2)
        return true;
    else
        return false;
}

double elapsed(time_t t0)
{
    time_t t1 = clock();
    double e = (double)(t1 - t0) / ((double)CLK_TCK);
    return e;
}

void busywait(unsigned ms)
{
    if (ms <= 0)
        return;
    time_t t0;
    double e;
    t0 = clock();

    do
    {
        time_t t1 = clock();
        e = (double)(t1 - t0) / ((double)CLK_PER_MS);
        printf("e=%f\n", e);
    } while (e < ms);
    //    printf("Busiwait: %10.2f ms.\n", e);
}

bool OpenComPort(const char *port, int baud)
{
    h_ = platform_open(port, baud);
    if (!h_)
    {
        return false;
    }

    baud_ = baud;
    littlend_ = isLittleEndian();
    if (littlend_)
        printf("Hardware is Little Endian.\n");
    else
        printf("Hardware is Big Endian.\n");

    mu910::setHandle(h_);
    mu903::setHandle(h_);
    mu910::baud_ = baud;
    mu903::baud_ = baud;
    emptyserial();

    /* ===== CHECKS  mu910 only  =====
    // //    mu910_ = is910();
    // //
    // //    if(!mu910_) {
    // //        ReaderInfo ri;
    // //        if(GetSettings(&ri)) {
    // //            return true;
    // //        } else {
    // //            return false;
    // //        }
    // //    }
    // */

    ReaderInfo ri;
    if (is910())
    {
        mu910_ = true;
        return true;
    }
    else if (mu903::GetSettings(&ri))
    {
        mu910_ = false;
        return true;
    }
    else
    {
        platform_close(h_);
        return false;
    }
    return false;
}

void CloseComPort(void)
{
    if (h_)
    {
        platform_close(h_);
        h_ = UART_NUM_MAX;
    }
}

bool GetSettings(ReaderInfo *ri)
{
    if (mu910_)
    {
        return mu910::GetSettings(ri);
    }
    else
    {
        return mu903::GetSettings(ri);
    }
}

bool SetSettings(ReaderInfo *ri)
{
    if (mu910_)
    {
        return mu910::SetSettings(ri);
    }
    else
    {
        return mu903::SetSettings(ri);
    }
}

/** \brief
 *  Gets Inventory, applies filter if specified.
 * \param port port name in the form of COM1, COM2 ... Baudrate must be one of the following:
 *     9600, 19200, 38400, 57600(default), 115200.
 * \return array which is multiple of dataLength. {type,ant,RSSI,count,epc[]}, where type, RSSI
 *     and count are int. epc array size is (dataLength - 4*4). The total array size will be even.
 *     If the operation fails, returns 0;
 */
int Inventory(bool filter)
{
    if (mu910_)
    {
        return mu910::Inventory(filter);
    }
    else
    {
        return mu903::Inventory(filter);
    }
}

/** \brief
 *  Gets Single Tag Inventory.
 * \return 1, if a single tag found, otherwise returns 0;
 */

int InventoryOne()
{
    if (mu910_)
    {
        return mu910::InventoryOne();
    }
    else
    {
        return mu903::InventoryOne();
    }
}

bool GetResult(unsigned char *scanresult, int index)
{
    if (mu910_)
    {
        return mu910::GetResult(scanresult, index);
    }
    else
    {
        return mu903::GetResult(scanresult, index);
    }
}

bool InventoryB(vector<ScanData> &v, bool filter)
{
    if (mu910_)
    {
        return mu910::InventoryB(v, filter);
    }
    else
    {
        return mu903::InventoryB(v, filter);
    }
}

/**
 * Filter: x*, 2:x, x:2
 */
bool InventoryNB(vector<ScanData> &v, bool filter)
{
    if (mu910_)
    {
        return mu910::InventoryNB(v, filter);
    }
    else
    {
        return mu903::InventoryNB(v, filter);
    }
}

bool InventoryTID(vector<ScanData> &v)
{
    if (mu910_)
    {
        return mu910::InventoryTID(v);
    }
    else
    {
        return mu903::InventoryTID(v);
    }
}

bool Auth(unsigned char *password, unsigned char size)
{
    if (mu910_)
    {
        return mu910::Auth(password, size);
    }
    else
    {
        return mu903::Auth(password, size);
    }
}

/**
Writes data to an RFID tag.
@param epc(unsigned char*) Pointer to an array of unsigned chars representing the EPC of the tag.
@param epcLen(unsigned char) Length of the EPC array in bytes.
@param data(unsigned char*) Pointer to an array of unsigned chars containing the data to be written.
@param wsize(int) Size of each write operation in bytes.
@param windex(int) Starting index of the data to be written within the tag's memory.
@param memtype(int) Type of memory location on the tag where the data should be written in mu903 format.
@return True if the write operation was successful, False otherwise.
*/

void LibVersion(unsigned char *version)
{
    // version[0] = (LIB_VERSION >> 8) & 0x00FF;
    // version[1] = LIB_VERSION & 0x00FF;
    if (mu910_)
    {
        version[0] = 0x0000;
        version[1] = 0x0000;
    }
    else
    {
        version[0] = (LIB_VERSION >> 8) & 0x00FF;
        version[1] = LIB_VERSION & 0x00FF;
    }
}

bool Read(unsigned char *epc, unsigned epclen, unsigned char *data, int wnum, int windex, int memtype)
{
    if (mu910_)
    {
        return mu910::Read(epc, epclen, data, wnum, windex, memtype);
    }
    else
    {
        return mu903::Read(epc, epclen, data, wnum, windex, memtype);
    }
}

bool ReadWord(unsigned char *epc, unsigned char epclen, unsigned char *data, int windex, int memtype)
{
    if (mu910_)
    {
        return mu910::ReadWord(epc, epclen, data, windex, memtype);
    }
    else
    {
        return mu903::ReadWord(epc, epclen, data, windex, memtype);
    }
}

bool ReadMemWord(unsigned char *epc, unsigned char epclen, unsigned char *data, unsigned char windex)
{
    if (mu910_)
    {
        return mu910::ReadMemWord(epc, epclen, data, windex);
    }
    else
    {
        return mu903::ReadMemWord(epc, epclen, data, windex);
    }
}

bool GetTID(unsigned char *epc, unsigned char epclen, unsigned char *tid, unsigned char *tidlen)
{
    if (mu910_)
    {

        return mu910::GetTID(epc, epclen, tid, tidlen);
    }
    else
    {
        return mu903::GetTID(epc, epclen, tid, tidlen);
    }
}

/**
 * filterDesc = [~][-][.]aa>>[mm=]value[=nn]<<bb[~][-][.]
 *   if aa: is provided, then the value is right shifted this many bits.
 *   if :bb is provided, then the value is left shifted this many bits.
 *   both aa and bb cannot be specified together.
 *   if neither aa or bb is provided, then value or value is used as prefix.
 *   if *value is provided, then value is used as suffix.
 *   if ---value, where each - indicate 4 bit nibble.
 *   if value---, where each - indicate 4 bit nibble.
 *   value... where each . indicate a bit.
 *   ...value where each . indicate a bit.
 *   ~~~value where each ~ indicate a byte.
 *   value~~~ where each ~ indicate a byte.
 */
bool CreateFilter(char *filterDesc)
{
    return false;
}

bool SetFilter(int maskAdrInByte, int maskLenInByte, unsigned char *maskDataByte)
{
    // if_else
    if (mu910_)
    {

        return mu910::SetFilter(maskAdrInByte, maskLenInByte, maskDataByte);
    }
    else
    {
        return mu903::SetFilter(maskAdrInByte, maskLenInByte, maskDataByte);
    }
}

bool Write(unsigned char *epc, unsigned char epcLen, unsigned char *data, int wsize, int windex, int memtype)
{
    if (mu910_)
    {

        return mu910::Write(epc, epcLen, data, wsize, windex, memtype);
    }
    else
    {
        return mu903::Write(epc, epcLen, data, wsize, windex, memtype);
    }
}

bool WriteWord(unsigned char *epc, unsigned char epcLen, unsigned char *data, unsigned char windex, int memtype)
{
    if (mu910_)
    {

        return mu910::WriteWord(epc, epcLen, data, windex, memtype);
    }
    else
    {
        return mu903::WriteWord(epc, epcLen, data, windex, memtype);
    }
}

bool WriteMemWord(unsigned char *epc, unsigned char epclen, unsigned char *data, unsigned char windex)
{
    if (mu910_)
    {

        return mu910::WriteMemWord(epc, epclen, data, windex);
    }
    else
    {
        return mu903::WriteMemWord(epc, epclen, data, windex);
    }
}

bool WriteEpcWord(unsigned char *epc, unsigned char epclen, unsigned char *data, unsigned char windex)
{
    if (mu910_)
    {

        return mu910::WriteEpcWord(epc, epclen, data, windex);
    }
    else
    {
        return mu903::WriteEpcWord(epc, epclen, data, windex);
    }
}

bool WriteEpc(unsigned char *epc, unsigned char epcLen, unsigned char *data)
{
    if (mu910_)
    {
        return mu910::WriteEpc(epc, epcLen, data);
    }
    else
    {
        return mu903::WriteEpc(epc, epcLen, data);
    }
}

bool ReadEpcWord(unsigned char *epc, unsigned char epclen, unsigned char *data, int windex, int memtype)
{
    return Read(epc, epclen, data, 1, windex, MEM_EPC);
}

bool TagExists(unsigned char *epc, unsigned char epclen)
{
    Filter f = filter_; // backup filter
    bool success = SetFilter(0, epclen, epc);
    if (!success)
    {
        filter_ = f;
        return false;
    }
    int n = Inventory(true);
    // printf("n=%d\n", n);
    filter_ = f;
    return (n > 0) ? 1 : 0;
}

bool Kill()
{
    return false;
}

bool Lock()
{
    return false;
}

bool Erase()
{
    return false;
}

bool SetPassword(unsigned char *epc, unsigned char epcLen, unsigned char *pass, unsigned char size)
{
    if (mu910_)
    {
        return mu910::SetPassword(epc, epcLen, pass, size);
    }
    else
    {
        return mu903::SetPassword(epc, epcLen, pass, size);
    }
}

bool SetKillPassword(unsigned char *epc, unsigned char epcLen, unsigned char *pass, unsigned char size)
{
    if (mu910_)
    {
        return mu910::SetKillPassword(epc, epcLen, pass, size);
    }
    else
    {
        return mu903::SetKillPassword(epc, epcLen, pass, size);
    }
}

// bool GetKillPassword(unsigned char *epc, int epclen, unsigned char *pass, unsigned char *passlen) {
//     if(mu910_) {
//         return mu910::GetKillPassword1(epc, epcLen, pass, passlen);
//     } else {
//         return mu903::GetKillPassword2(epc, epcLen, pass, passlen);
//     }
// }

bool SetGPO(unsigned char gpoNumber)
{
    if (mu910_)
    {
        return mu910::SetGPO(gpoNumber);
    }
    else
    {
        return mu903::SetGPO(gpoNumber);
    }
}

char GetGPI(unsigned char gpiNumber)
{
    if (mu910_)
    {
        return mu910::GetGPI(gpiNumber);
    }
    else
    {
        return mu903::GetGPI(gpiNumber);
    }
}

void LastError(char *buffer)
{
    // if_else needed
    if (mu910_)
    {
        return mu910::LastError(buffer);
    }
    else
    {
        return mu903::LastError(buffer);
    }
}

int GetTagInfo(unsigned char *tid, TagInfo *info)
{
    unsigned short mfg = (tid[0] << 8) | tid[1];
    unsigned short chip = (tid[2] << 8) | tid[3];
    //    unsigned short model = (tid[4] << 8) | tid[5];

    info->pwdlen = 4;
    info->epclen = 12;
    info->tidlen = 12;
    info->userlen = 4;
    info->type = UNKNOWN;

    // printf("%04X, %04X\n", mfg, chip);
    // mfg &= 0xFF00;
    switch (mfg)
    {
    case 0xE280:
    {
        // MONZA
        switch (chip)
        {
        case 0x1130:
            strcpy(info->chip, "MONZA_5");
            info->type = MONZA_5;
            memcpy(info->tid, tid, info->tidlen);
            return MONZA_5;
        case 0x1105:
            strcpy(info->chip, "MONZA_4QT");
            info->type = MONZA_4QT;
            info->userlen = 512 / 8;
            info->epclen = 16;
            memcpy(info->tid, tid, info->tidlen);
            return MONZA_4QT;
        case 0x110C:
            strcpy(info->chip, "MONZA_4E");
            info->type = MONZA_4E;
            info->userlen = 128 / 8;
            // info->epclen = 496/8;
            memcpy(info->tid, tid, info->tidlen);
            return MONZA_4E;
        case 0x1100:
            strcpy(info->chip, "MONZA_4D");
            info->type = MONZA_4D;
            info->userlen = 32 / 8;
            info->epclen = 16;
            memcpy(info->tid, tid, info->tidlen);
            return MONZA_4D;
        case 0x1114:
            strcpy(info->chip, "MONZA_4I");
            info->type = MONZA_4I;
            info->userlen = 480 / 8;
            // info->epclen = 256/8;
            memcpy(info->tid, tid, info->tidlen);
            return MONZA_4I;
        case 0x1160:
            strcpy(info->chip, "MONZA_R6");
            info->type = MONZA_R6;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 0;
            return MONZA_R6;
        case 0x1170:
            strcpy(info->chip, "MONZA_R6P");
            info->type = MONZA_R6P;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 4;
            return MONZA_R6P;
        case 0x1150:
            strcpy(info->chip, "MONZA_X8K");
            info->type = MONZA_X8K;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 8192 / 8;
            return MONZA_X8K;
        case 0x1140: // TODO
            strcpy(info->chip, "MONZA_X2K");
            info->type = MONZA_X2K;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 0;
            info->pwdlen = 8;
            info->epclen = 12;
            return MONZA_X2K;
        case 0x1191:
            strcpy(info->chip, "IMPINJ_M730");
            info->type = IMPINJ_M730;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 0;
            info->pwdlen = 8;
            return IMPINJ_M730;
        case 0x1190:
            strcpy(info->chip, "IMPINJ_M750");
            info->type = IMPINJ_M750;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 4;
            info->pwdlen = 8;
            // info->epclen = 16;
            return IMPINJ_M750;
        case 0x11A0:
            strcpy(info->chip, "IMPINJ_M770");
            info->type = IMPINJ_M770;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 4;
            info->pwdlen = 8;
            // info->epclen = 16;
            return IMPINJ_M770;
        case 0x11C1: //??
            strcpy(info->chip, "IMPINJ_UNKNOWN");
            info->type = UNKNOWN;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 4;
            info->pwdlen = 8;
            return IMPINJ_M750;
            break;
        }
    }
    case 0xE200:
    {
        switch (chip)
        {
        // HIGGS
        case 0x3412: // TEST PASS
            strcpy(info->chip, "HIGGS_3");
            info->type = HIGGS_3;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 64;
            return HIGGS_3;
        case 0x3414:
            strcpy(info->chip, "HIGGS_4");
            info->type = HIGGS_4;
            memcpy(info->tid, tid, info->tidlen);
            return HIGGS_4;
        case 0x3415:
            strcpy(info->chip, "HIGGS_9");
            info->type = HIGGS_9;
            memcpy(info->tid, tid, info->tidlen);
            return HIGGS_9;
        case 0x3416: // TODO - This is incorrect. Find the correct one.
            strcpy(info->chip, "HIGGS_10");
            info->type = HIGGS_10;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 4;
            return HIGGS_10;
        //        case 0x3416:
        //            strcpy(info->chip,"HIGGS_EC");
        //            info->type = HIGGS_EC;
        //            info->tidlen = 16;
        //            memcpy(info->tid, tid, info->tidlen);
        //            info->userlen = 16;
        //            return HIGGS_EC;
        case 0x3811:
            strcpy(info->chip, "HIGGS_EC");
            info->type = HIGGS_EC;
            info->tidlen = 16;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 16;
            return HIGGS_EC;
        }

        unsigned short m = chip & 0x0FFF;
        switch (m)
        {
        // UCODE
        case 0x0890:
            strcpy(info->chip, "UCODE_7");
            info->type = UCODE_7;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 4;
            return UCODE_7;
        case 0x0894:
            strcpy(info->chip, "UCODE_8");
            info->type = UCODE_8;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 0;
            return UCODE_8;
        case 0x0994:
            strcpy(info->chip, "UCODE_8M");
            info->type = UCODE_8M;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 4;
            return UCODE_8M;
        case 0x0995:
            strcpy(info->chip, "UCODE_9");
            info->type = UCODE_9;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 0;
            return UCODE_9;
        }
        break;
    }

    case 0xE2C0:
    {
        switch (chip)
        {
        // UCODE
        case 0x6F92:
            strcpy(info->chip, "UCODE_DNA");
            info->type = UCODE_DNA;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 3072 / 8;
            // info->epclen = 224/8;
            return UCODE_DNA;
        case 0x6F93: // this code is incorrect
            strcpy(info->chip, "UCODE_DNA_CITY");
            info->type = UCODE_DNA_CITY;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 1024 / 8;
            // info->epclen = 224/8;
            return UCODE_DNA_CITY;
        case 0x6F94: // this code is incorrect
            strcpy(info->chip, "UCODE_DNA_TRACK");
            info->type = UCODE_DNA_TRACK;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 256 / 8;
            // info->epclen = 448/8;
            return UCODE_DNA_TRACK;
        case 0x11A2:
            strcpy(info->chip, "IMPINJ_M775");
            info->type = IMPINJ_M775;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 32 / 8;
            // info->epclen = 128/8;
            return UCODE_DNA_TRACK;
        }
    }
    case 0xE281:
    {
        switch (chip)
        {
        case 0xD011:
        {
            strcpy(info->chip, "KILOWAY_2005BL");
            info->type = KILOWAY_2005BL;
            memcpy(info->tid, tid, info->tidlen);
            info->userlen = 1312 / 8;
            return KILOWAY_2005BL;
        }
        }
    }
    }

    strcpy(info->chip, "UNKNOWN");
    memcpy(info->tid, tid, info->tidlen);
    return UNKNOWN;
}

int AvailablePorts(char *ports)
{
#ifndef ESP32
    vector<string> p = platform_list();
    char port[128];
    ports[0] = 0;
    for (unsigned i = 0; i < p.size(); i++)
    {
        snprintf(port, sizeof(port), "%s\n", p[i].c_str());
        strcat(ports, port);
    }
    printf("%s\n", ports);
    return p.size();
#else
    return 0;
#endif // not esp32
}

unsigned int little2big(unsigned int little)
{
    unsigned int b0 = little & 0x000000ff;
    unsigned int b1 = (little >> 8) & 0xff;
    unsigned int b2 = (little >> 16) & 0xff;
    unsigned int b3 = (little >> 24) & 0xff;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

/*
    Serial : 4 byte int
    VersionInfo : 2 byte {major, minor}
    Antenna : 1 byte
    ComAdr : 1 byte
    ReaderType : 1 byte
    Protocol : 1 byte
    Band : 1 byte
    Power : 1 byte
    ScanTime : 4 byte int
    BeepOn : 1 byte
    Reserved1 : 1 byte
    Reserved2 : 1 byte
    MaxFreq : 4 byte int
    MinFreq : 4 byte int
    BaudRate : 4 byte int


struct ReaderInfo {
    // the following constants are reader constants
    // they cannot be changed.
    int Serial = 0;
    char VersionInfo[2] = {0,0};
    unsigned char Antenna = 0;

    // the following variables are reader parameters
    // which can be change. Call SetSettings function
    // to change the settings.
    unsigned char ComAdr;
    unsigned char ReaderType;
    unsigned char Protocol;
    unsigned char Band;
    unsigned char Power;
    unsigned char ScanTime;
    unsigned char BeepOn;
    unsigned char Reserved1;
    unsigned char Reserved2;

    unsigned int MaxFreq  = 0;
    unsigned int MinFreq  = 0;
    unsigned int BaudRate = 0;
};

*/
bool LoadSettings(unsigned char *buff)
{
    ReaderInfo ri;
    bool success = GetSettings(&ri);
    if (!success)
        return 0;

    memcpy(buff, &ri, sizeof(ri));
    printarr(buff, sizeof(ri));
    if (0)
    {
        memcpy(buff, &ri, sizeof(ri));
        // printarr(buff, sizeof(ri));
    }
    else
    {
        unsigned char *p = buff;
        memcpy(p, &ri.Serial, 4);
        p += 4;
        *p++ = ri.VersionInfo[0];
        *p++ = ri.VersionInfo[1];
        *p++ = ri.Antenna;
        *p++ = ri.ComAdr;
        *p++ = ri.ReaderType;
        *p++ = ri.Protocol;
        *p++ = ri.Band;
        *p++ = ri.Power;
        *p++ = ri.ScanTime;
        *p++ = ri.BeepOn;
        *p++ = 0; // Reserved1
        *p++ = 0; // Reserved2
        unsigned int val = ri.MaxFreq;
        memcpy(p, &val, 4);
        p += 4;
        val = ri.MinFreq;
        memcpy(p, &val, 4);
        p += 4;
        val = ri.BaudRate;
        memcpy(p, &val, 4);
        // printarr(buff, sizeof(ri));
    }

    // printsettings(&ri);

    return 1;
}

bool SaveSettings(unsigned char *buff)
{
    ReaderInfo ri;
    // memcpy(&ri, buff, sizeof(ri));
    /*
        Serial : 4 byte int
        VersionInfo : 2 byte {major, minor}
        Antenna : 1 byte
        ComAdr : 1 byte
        ReaderType : 1 byte
        Protocol : 1 byte
        Band : 1 byte
        Power : 1 byte
        ScanTime : 4 byte int
        BeepOn : 1 byte
        Reserved1 : 1 byte
        Reserved2 : 1 byte
        MaxFreq : 4 byte int
        MinFreq : 4 byte int
        BaudRate : 4 byte int
    */
    unsigned char *p = buff;
    memcpy(&ri.Serial, p, 4);
    p += 4;
    ri.VersionInfo[0] = *p++;
    ri.VersionInfo[1] = *p++;
    ri.Antenna = *p++;
    ri.ComAdr = *p++;
    ri.ReaderType = *p++;
    ri.Protocol = *p++;
    ri.Band = *p++;
    ri.Power = *p++;
    ri.ScanTime = *p++;
    ri.BeepOn = *p++;
    ri.Reserved1 = *p++;
    ri.Reserved2 = *p++;
    memcpy(&ri.MaxFreq, p, 4);
    p += 4;
    memcpy(&ri.MinFreq, p, 4);
    p += 4;
    memcpy(&ri.BaudRate, p, 4);

    printsettings(&ri);
    bool success = SetSettings(&ri);
    //    return (unsigned char)success;
    return success;
}

bool SetQ(unsigned char q)
{
    // if_else
    if (mu910_)
    {

        return mu910::SetQ(q);
    }
    else
    {
        return mu903::SetQ(q);
    }
}

bool SetSession(unsigned char sess)
{
    // if_else
    if (mu910_)
    {
        return mu910::SetSession(sess);
    }
    else
    {
        return mu903::SetSession(sess);
    }
}

bool Is496Bits()
{
    return mu903::Is496Bits();
}

bool Set496Bits(bool bits496)
{
    return mu903::Set496Bits(bits496);
}
