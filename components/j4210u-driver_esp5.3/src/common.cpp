#include "common.h"
#include <stdio.h>
#include <string>
#include "driver.h"

void printsettings(ReaderInfo *ri) {
    printf("\n>>Settings>>\n");
    printf("Version: %d.%d\n", ri->VersionInfo[0], ri->VersionInfo[1]);
    printf("Serial: %u\n", ri->Serial);
    printf("ComAdr: %d\n", ri->ComAdr);
    printf("Reader Type: %d\n", ri->ReaderType);
    printf("Protocol: %X\n", ri->Protocol);
    printf("Band: %c\n", ri->Band);
    printf("Min Freq: %fMHz\n", ri->MinFreq / 1000.0);
    printf("Max Freq: %fMHz\n", ri->MaxFreq / 1000.0);
    printf("Power: %ddB\n", ri->Power);
    printf("Scan Time: %dms\n", ri->ScanTime * 100);
    printf("Antenna: %d\n", ri->Antenna);
    printf("BeepOn: %d\n", ri->BeepOn);
    printf("Baud Rate: %d\n", ri->BaudRate);
}


void printarr(unsigned char *arr, int size) {
    for (int i = 0; i < size; i++) {
        printf("%02X ", arr[i]);
    }
    printf("\n");
    fflush(stdout);
}



double  stopwatch(bool start) {
    /*
        static time_t t0;
        if (start) {
            t0 = clock();
            return 0.0;
        }
        time_t t1 = clock();
        double e = (double)(t1-t0)/((double)CLK_TCK);
        return e;
    */
    struct timeval tt;
    static timeval st;
    double secs = 0;

    if (start) {
        gettimeofday(&st, NULL);
        return 0.0;
    }

    gettimeofday(&tt, NULL);
    // Do stuff  here
    secs = (double)(tt.tv_usec - st.tv_usec) / 1000000.0 + (double)(tt.tv_sec - st.tv_sec);;
    printf("time taken %f\n",secs);
    return secs;
}
