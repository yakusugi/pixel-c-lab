// droidstat.c - Compact system report for rooted Pixel/Android (read-only)
// Combines: uname + uptime (CLOCK_BOOTTIME) + /proc/meminfo + thermal + optional dmesg tail
//
// Build: clang -std=c11 -Wall -Wextra -O2 droidstat.c -o droidstat
// Run  : ./droidstat
//       ./droidstat --dmesg 30

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/utsname.h>

static void hr(void) { puts("----------------------------------------"); }

static void trim(char *s) {
    size_t n = strlen(s);
    if (n && s[n-1] == '\n') s[n-1] = '\0';
}

static int run_line(const char *cmd, char *out, size_t out_sz) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;
    if (!fgets(out, (int)out_sz, fp)) { pclose(fp); return -1; }
    pclose(fp);
    trim(out);
    return 0;
}

static void print_dur(double s) {
    long long t = (long long)s;
    long long d = t / 86400; t %= 86400;
    long long h = t / 3600;  t %= 3600;
    long long m = t / 60;
    long long sec = t % 60;
    if (d) printf("%lldd ", d);
    printf("%02lld:%02lld:%02lld", h, m, sec);
}

static void sec_uname(void) {
    puts("== uname ==");
    struct utsname u;
    if (uname(&u) == 0) {
        printf("sysname : %s\n", u.sysname);
        printf("release : %s\n", u.release);
        printf("version : %s\n", u.version);
        printf("machine : %s\n", u.machine);
    } else {
        puts("uname() failed");
    }
    hr();
}

static void sec_uptime(void) {
    puts("== uptime ==");
    struct timespec ts;
    if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
        double up = ts.tv_sec + ts.tv_nsec / 1e9;
        printf("uptime : "); print_dur(up); printf(" (%.2fs)\n", up);
        puts("note   : CLOCK_BOOTTIME (works even if /proc/uptime is blocked)");
    } else {
        puts("clock_gettime(CLOCK_BOOTTIME) failed");
    }
    hr();
}

static long long read_mem_kb(const char *key) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return -1;

    char k[64], unit[16];
    long long v = 0;
    while (fscanf(fp, "%63s %lld %15s", k, &v, unit) == 3) {
        if (strcmp(unit, "kB") != 0) continue;
        if (!strcmp(k, key)) { fclose(fp); return v; }
    }
    fclose(fp);
    return -1;
}

static void sec_mem(void) {
    puts("== mem ==");
    long long total = read_mem_kb("MemTotal:");
    long long free  = read_mem_kb("MemFree:");
    long long avail = read_mem_kb("MemAvailable:");
    long long cached= read_mem_kb("Cached:");

    if (total > 0) {
        printf("MemTotal     : %lld kB\n", total);
        printf("MemFree      : %lld kB\n", free);
        printf("MemAvailable : %lld kB\n", avail);
        printf("Cached       : %lld kB\n", cached);
        if (avail > 0) printf("Available%%   : %.1f%%\n", (double)avail * 100.0 / (double)total);
    } else {
        puts("note: /proc/meminfo blocked or unreadable");
        puts("hint: try running with root: su -c ./droidstat");
    }
    hr();
}

static int read_sys_line(const char *path, char *out, size_t out_sz) {
    FILE *fp = fopen(path, "r");
    if (fp) {
        if (!fgets(out, (int)out_sz, fp)) { fclose(fp); return -1; }
        fclose(fp);
        trim(out);
        return 0;
    }
    // su fallback (read-only)
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "su -c cat %s 2>/dev/null", path);
    if (run_line(cmd, out, out_sz) == 0) return 0;
    return -1;
}

static double to_celsius(const char *s) {
    long v = strtol(s, NULL, 10);
    if (v > 1000) return v / 1000.0;
    return (double)v;
}

static void sec_thermal(void) {
    puts("== thermal ==");
    int any = 0;
    for (int i = 0; i < 32; i++) {
        char type_p[128], temp_p[128];
        snprintf(type_p, sizeof(type_p), "/sys/class/thermal/thermal_zone%d/type", i);
        snprintf(temp_p, sizeof(temp_p), "/sys/class/thermal/thermal_zone%d/temp", i);

        char type[96] = {0}, temp[96] = {0};
        if (read_sys_line(type_p, type, sizeof(type)) != 0) continue;
        if (read_sys_line(temp_p, temp, sizeof(temp)) != 0) continue;

        printf("zone%-2d %-18s %6.1f C\n", i, type, to_celsius(temp));
        any = 1;
    }

    if (!any) {
        puts("no thermal zones readable (blocked?)");
        puts("hint: try: su -c ./droidstat");
    }
    hr();
}

static int tail_cmd(const char *cmd, int n) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;

    char **ring = calloc((size_t)n, sizeof(char *));
    if (!ring) { pclose(fp); return -1; }

    char buf[1024];
    int idx = 0, count = 0;

    while (fgets(buf, sizeof(buf), fp)) {
        free(ring[idx]);
        ring[idx] = strdup(buf);
        if (!ring[idx]) break;
        idx = (idx + 1) % n;
        if (count < n) count++;
    }
    pclose(fp);

    if (count == 0) {
        for (int i = 0; i < n; i++) free(ring[i]);
        free(ring);
        return -1;
    }

    int start = (count == n) ? idx : 0;
    for (int i = 0; i < count; i++) {
        int pos = (start + i) % n;
        if (ring[pos]) fputs(ring[pos], stdout);
    }

    for (int i = 0; i < n; i++) free(ring[i]);
    free(ring);
    return 0;
}

static void sec_dmesg_tail(int n) {
    puts("== dmesg tail ==");
    printf("lines: %d\n\n", n);

    if (tail_cmd("dmesg 2>/dev/null", n) == 0) { hr(); return; }
    if (tail_cmd("su -c dmesg 2>/dev/null", n) == 0) {
        puts("\n(note: used su -c dmesg)");
        hr();
        return;
    }

    puts("blocked: dmesg not readable on this build");
    puts("hint: try: su -c ./droidstat --dmesg 50");
    hr();
}

int main(int argc, char **argv) {
    int want_dmesg = 0;
    int dmesg_n = 30;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--dmesg")) {
            want_dmesg = 1;
            if (i + 1 < argc) dmesg_n = atoi(argv[i + 1]);
            if (dmesg_n <= 0) dmesg_n = 30;
            if (dmesg_n > 200) dmesg_n = 200;
        }
    }

    puts("droidstat - compact system report (read-only)");
    hr();

    sec_uname();
    sec_uptime();
    sec_mem();
    sec_thermal();
    if (want_dmesg) sec_dmesg_tail(dmesg_n);

    puts("done.");
    return 0;
}