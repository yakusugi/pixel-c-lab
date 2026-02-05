// dmesg_tail.c - Print last N lines of kernel log (read-only), with su fallback.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_last_lines_cmd(const char *cmd, int n) {
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

    // Print in correct order (oldest -> newest)
    int start = (count == n) ? idx : 0;
    for (int i = 0; i < count; i++) {
        int pos = (start + i) % n;
        if (ring[pos]) fputs(ring[pos], stdout);
    }

    for (int i = 0; i < n; i++) free(ring[i]);
    free(ring);
    return 0;
}

int main(int argc, char **argv) {
    int n = 50;
    if (argc >= 2) {
        n = atoi(argv[1]);
        if (n <= 0) n = 50;
        if (n > 500) n = 500; // keep it sane on phones
    }

    puts("== dmesg tail ==");
    printf("lines: %d\n\n", n);

    // Try non-root first
    if (read_last_lines_cmd("dmesg 2>/dev/null", n) == 0) return 0;
    if (read_last_lines_cmd("/system/bin/dmesg 2>/dev/null", n) == 0) return 0;

    // Root fallback (still read-only)
    if (read_last_lines_cmd("su -c dmesg 2>/dev/null", n) == 0) {
        puts("\n(note: used su -c dmesg)");
        return 0;
    }

    puts("failed: dmesg is blocked (try: su -c ./dmesg_tail 50)");
    return 1;
}