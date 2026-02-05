// thermal.c - Read thermal zones from /sys/class/thermal (read-only)
// Tries direct read; falls back to `su -c cat` if blocked.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_first_line(const char *path, char *out, size_t out_sz) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    if (!fgets(out, (int)out_sz, fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    size_t n = strlen(out);
    if (n && out[n - 1] == '\n') out[n - 1] = '\0';
    return 0;
}

static int read_first_line_su(const char *path, char *out, size_t out_sz) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "su -c cat %s 2>/dev/null", path);

    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;

    if (!fgets(out, (int)out_sz, fp)) {
        pclose(fp);
        return -1;
    }
    pclose(fp);

    size_t n = strlen(out);
    if (n && out[n - 1] == '\n') out[n - 1] = '\0';
    return 0;
}

static int read_line_auto(const char *path, char *out, size_t out_sz, int *used_su) {
    if (read_first_line(path, out, out_sz) == 0) { *used_su = 0; return 0; }
    if (read_first_line_su(path, out, out_sz) == 0) { *used_su = 1; return 0; }
    return -1;
}

static double to_celsius(const char *s) {
    long v = strtol(s, NULL, 10);
    // Many Android kernels report millidegrees Celsius
    if (v > 1000) return v / 1000.0;
    return (double)v;
}

int main(void) {
    puts("== thermal zones ==");

    int any = 0;
    int used_su_any = 0;

    for (int i = 0; i < 64; i++) {
        char type_path[128], temp_path[128];
        snprintf(type_path, sizeof(type_path), "/sys/class/thermal/thermal_zone%d/type", i);
        snprintf(temp_path, sizeof(temp_path), "/sys/class/thermal/thermal_zone%d/temp", i);

        char type[96] = {0}, temp_s[96] = {0};
        int used_su_type = 0, used_su_temp = 0;

        if (read_line_auto(type_path, type, sizeof(type), &used_su_type) != 0) continue;
        if (read_line_auto(temp_path, temp_s, sizeof(temp_s), &used_su_temp) != 0) continue;

        double c = to_celsius(temp_s);
        printf("zone%-2d %-18s %6.1f C\n", i, type, c);

        any = 1;
        if (used_su_type || used_su_temp) used_su_any = 1;
    }

    if (!any) {
        puts("no thermal zones readable (blocked?)");
        puts("hint: try: su -c ./thermal");
        return 1;
    }

    if (used_su_any) {
        puts("\n(note: some reads required su)");
    }

    puts("\nTip: temps over ~45-50C on skin/battery often correlate with throttling.");
    return 0;
}