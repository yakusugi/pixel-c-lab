// threads.c - List processes + thread count via /proc/[pid]/status (read-only)
// Android note: some /proc entries may be Permission denied; we skip those.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

static int is_number(const char *s) {
    for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

static int read_status_fields(int pid, char *name, size_t name_sz,
                              int *tgid, int *threads, char *state, size_t state_sz) {
    char path[128];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    char line[256];
    int got_name = 0, got_tgid = 0, got_threads = 0, got_state = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (!got_name && sscanf(line, "Name:%63s", name) == 1) got_name = 1;
        else if (!got_state && sscanf(line, "State:%15[^\n]", state) == 1) got_state = 1;
        else if (!got_tgid && sscanf(line, "Tgid:%d", tgid) == 1) got_tgid = 1;
        else if (!got_threads && sscanf(line, "Threads:%d", threads) == 1) got_threads = 1;

        if (got_name && got_state && got_tgid && got_threads) break;
    }
    fclose(fp);

    if (!got_name) snprintf(name, name_sz, "?");
    if (!got_state) snprintf(state, state_sz, "?");
    if (!got_tgid) *tgid = -1;
    if (!got_threads) *threads = -1;

    return 0;
}

int main(int argc, char **argv) {
    int limit = 30;
    if (argc >= 2) {
        limit = atoi(argv[1]);
        if (limit <= 0) limit = 30;
        if (limit > 200) limit = 200;
    }

    DIR *d = opendir("/proc");
    if (!d) { perror("opendir(/proc)"); return 1; }

    puts("== threads (/proc/[pid]/status) ==");
    printf("showing up to %d readable entries\n\n", limit);
    printf("%-6s %-6s %-7s %-18s %s\n", "PID", "TGID", "THR", "NAME", "STATE");

    struct dirent *e;
    int shown = 0;

    while ((e = readdir(d)) != NULL) {
        if (!is_number(e->d_name)) continue;
        int pid = atoi(e->d_name);

        char name[64] = {0};
        char state[32] = {0};
        int tgid = -1, thr = -1;

        if (read_status_fields(pid, name, sizeof(name), &tgid, &thr, state, sizeof(state)) != 0)
            continue; // permission denied or disappeared

        printf("%-6d %-6d %-7d %-18.18s %s\n", pid, tgid, thr, name, state);

        if (++shown >= limit) break;
    }

    closedir(d);

    if (shown == 0) {
        puts("no readable /proc/[pid]/status entries (blocked?)");
        puts("hint: try: su -c ./threads 30");
        return 1;
    }

    puts("\nTip: Threads: shows how many kernel threads exist inside a process.");
    return 0;
}