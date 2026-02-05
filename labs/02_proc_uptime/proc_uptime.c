// proc_uptime.c - /proc/uptime (if allowed) + fallback to CLOCK_BOOTTIME

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

static void print_dur(double s) {
    long long t = (long long)s;
    long long d = t / 86400; t %= 86400;
    long long h = t / 3600;  t %= 3600;
    long long m = t / 60;
    long long sec = t % 60;
    if (d) printf("%lldd ", d);
    printf("%02lld:%02lld:%02lld", h, m, sec);
}

int main(void) {
    double up = 0.0, idle = 0.0;

    FILE *fp = fopen("/proc/uptime", "r");
    if (fp && fscanf(fp, "%lf %lf", &up, &idle) == 2) {
        fclose(fp);
        printf("uptime: "); print_dur(up); printf(" (%.2fs)\n", up);
        printf("idle  : %.2fs\n", idle);
        return 0;
    }
    if (fp) fclose(fp);

    // Fallback (no-root): CLOCK_BOOTTIME ~ “time since boot including sleep”
    struct timespec ts;
    if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
        up = ts.tv_sec + ts.tv_nsec / 1e9;
        printf("uptime: "); print_dur(up); printf(" (%.2fs)\n", up);
        puts("note  : /proc/uptime blocked; used CLOCK_BOOTTIME");
        return 0;
    }

    fprintf(stderr, "failed: /proc/uptime=%s, clock_gettime=%s\n",
            strerror(errno), strerror(errno));
    return 1;
}
