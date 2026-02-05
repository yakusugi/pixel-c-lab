// mounts.c - Read mount table (/proc/*/mounts) and print key mount points.

#include <stdio.h>
#include <string.h>
#include <errno.h>

static int interesting_target(const char *t) {
    return (!strcmp(t, "/") ||
            !strncmp(t, "/data", 5) ||
            !strncmp(t, "/system", 7) ||
            !strncmp(t, "/vendor", 7) ||
            !strncmp(t, "/product", 8) ||
            !strncmp(t, "/odm", 4) ||
            !strncmp(t, "/apex", 5) ||
            !strncmp(t, "/storage", 8) ||
            !strncmp(t, "/sdcard", 7));
}

int main(void) {
    const char *paths[] = { "/proc/mounts", "/proc/self/mounts", "/proc/1/mounts" };
    FILE *fp = NULL;
    const char *used = NULL;

    for (int i = 0; i < 3; i++) {
        fp = fopen(paths[i], "r");
        if (fp) { used = paths[i]; break; }
    }
    if (!fp) {
        fprintf(stderr, "fopen failed: %s\n", strerror(errno));
        return 1;
    }

    printf("== mounts (%s) ==\n", used);
    printf("%-18s %-22s %-8s %s\n", "source", "target", "fstype", "options");

    char src[256], tgt[256], fstype[64], opts[256];
    int dump1, dump2;

    while (fscanf(fp, "%255s %255s %63s %255s %d %d\n",
                  src, tgt, fstype, opts, &dump1, &dump2) == 6) {
        if (!interesting_target(tgt)) continue;
        printf("%-18.18s %-22.22s %-8.8s %s\n", src, tgt, fstype, opts);
    }

    fclose(fp);
    puts("\nTip: remove the interesting_target() filter to print *everything*.");
    return 0;
}