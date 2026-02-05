// loadavg.c - /proc/loadavg (if allowed) + fallback to sysinfo()

#include <stdio.h>
#include <sys/sysinfo.h>

int main(void) {
    double a1=0, a5=0, a15=0;
    int running=0, total=0, lastpid=0;

    FILE *fp = fopen("/proc/loadavg", "r");
    if (fp && fscanf(fp, "%lf %lf %lf %d/%d %d", &a1, &a5, &a15, &running, &total, &lastpid) == 6) {
        fclose(fp);
        puts("== /proc/loadavg ==");
        printf("load avg (1m)  : %.2f\n", a1);
        printf("load avg (5m)  : %.2f\n", a5);
        printf("load avg (15m) : %.2f\n", a15);
        printf("processes      : %d running / %d total\n", running, total);
        printf("last PID       : %d\n", lastpid);
        return 0;
    }
    if (fp) fclose(fp);

    // Fallback: sysinfo() load averages (scaled integers)
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        const double scale = (double)(1 << SI_LOAD_SHIFT);
        a1  = si.loads[0] / scale;
        a5  = si.loads[1] / scale;
        a15 = si.loads[2] / scale;

        puts("== sysinfo() fallback ==");
        printf("load avg (1m)  : %.2f\n", a1);
        printf("load avg (5m)  : %.2f\n", a5);
        printf("load avg (15m) : %.2f\n", a15);
        puts("note           : /proc/loadavg blocked; used sysinfo()");
        return 0;
    }

    puts("failed");
    return 1;
}