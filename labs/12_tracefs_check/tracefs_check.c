// tracefs_check.c - Detect tracefs and read tiny status samples (read-only).
// Tries normal reads; falls back to `su -c cat` if blocked.

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static int exists(const char *p) {
    struct stat st;
    return (stat(p, &st) == 0);
}

static void trim(char *s) {
    size_t n = strlen(s);
    if (n && s[n - 1] == '\n') s[n - 1] = '\0';
}

static int read_line(const char *path, char *out, size_t out_sz) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    if (!fgets(out, (int)out_sz, fp)) { fclose(fp); return -1; }
    fclose(fp);
    trim(out);
    return 0;
}

static int read_line_su(const char *path, char *out, size_t out_sz) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "su -c cat %s 2>/dev/null", path);
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;
    if (!fgets(out, (int)out_sz, fp)) { pclose(fp); return -1; }
    pclose(fp);
    trim(out);
    return 0;
}

static int read_auto(const char *path, char *out, size_t out_sz, int *used_su) {
    if (read_line(path, out, out_sz) == 0) { *used_su = 0; return 0; }
    if (read_line_su(path, out, out_sz) == 0) { *used_su = 1; return 0; }
    return -1;
}

static void check_root(const char *root) {
    if (!exists(root)) {
        printf("trace root not found : %s\n", root);
        return;
    }

    printf("trace root exists    : %s\n", root);

    char p_current[256], p_avail[256], p_clock[256];
    snprintf(p_current, sizeof(p_current), "%s/current_tracer", root);
    snprintf(p_avail,   sizeof(p_avail),   "%s/available_tracers", root);
    snprintf(p_clock,   sizeof(p_clock),   "%s/trace_clock", root);

    int looks = exists(p_current) && exists(p_avail);
    printf("looks like tracefs   : %s\n", looks ? "yes" : "maybe");

    char buf[256] = {0};
    int used_su = 0;

    if (read_auto(p_current, buf, sizeof(buf), &used_su) == 0)
        printf("current_tracer       : %s%s\n", buf, used_su ? " (via su)" : "");
    else
        printf("current_tracer       : (not readable)\n");

    if (read_auto(p_clock, buf, sizeof(buf), &used_su) == 0)
        printf("trace_clock          : %s%s\n", buf, used_su ? " (via su)" : "");
    else
        printf("trace_clock          : (not readable)\n");

    puts("");
}

int main(void) {
    puts("== tracefs check ==");

    puts("\nTracing, in one line:");
    puts("- A structured event recorder for kernel activity (scheduler, I/O, syscalls).");
    puts("- We do NOT enable tracing here; we only detect and read tiny status files.\n");

    check_root("/sys/kernel/tracing");
    check_root("/sys/kernel/debug/tracing");

    puts("Note: If su is used, Magisk may show a toast. This is still read-only.");
    return 0;
}
