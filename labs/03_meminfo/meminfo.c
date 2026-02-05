// meminfo.c - Minimal /proc/meminfo summary (kB)

#include <stdio.h>
#include <string.h>

int main(void) {
    long long total=-1, free=-1, avail=-1, cached=-1, buffers=-1;
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) { perror("fopen"); return 1; }

    char key[64], unit[16];
    long long val;
    while (fscanf(fp, "%63s %lld %15s", key, &val, unit) == 3) {
        if (strcmp(unit, "kB") != 0) continue;
        if (!strcmp(key,"MemTotal:")) total = val;
        else if (!strcmp(key,"MemFree:")) free = val;
        else if (!strcmp(key,"MemAvailable:")) avail = val;
        else if (!strcmp(key,"Cached:")) cached = val;
        else if (!strcmp(key,"Buffers:")) buffers = val;
    }
    fclose(fp);

    puts("== /proc/meminfo ==");
    printf("MemTotal     : %lld kB\n", total);
    printf("MemFree      : %lld kB\n", free);
    printf("MemAvailable : %lld kB\n", avail);
    printf("Cached       : %lld kB\n", cached);
    printf("Buffers      : %lld kB\n", buffers);

    if (total > 0 && avail > 0)
        printf("Available%%   : %.1f%%\n", (double)avail * 100.0 / (double)total);

    return 0;
}
