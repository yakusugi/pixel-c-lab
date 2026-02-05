// props.c - Read Android system properties via `getprop` (Termux-friendly)

#include <stdio.h>
#include <string.h>

static int run_getprop(const char *key, char *out, size_t out_sz) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "getprop %s", key);

    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;

    if (!fgets(out, (int)out_sz, fp)) {
        pclose(fp);
        return -1;
    }
    pclose(fp);

    // trim trailing newline
    size_t n = strlen(out);
    if (n && out[n - 1] == '\n') out[n - 1] = '\0';
    return 0;
}

static void print_prop(const char *key, const char *label) {
    char val[512] = {0};
    if (run_getprop(key, val, sizeof(val)) == 0 && val[0]) {
        printf("%-18s %s\n", label, val);
    } else {
        printf("%-18s (not available)\n", label);
    }
}

int main(void) {
    puts("== Android system properties ==");

    print_prop("ro.product.model",        "Model:");
    print_prop("ro.product.manufacturer", "Manufacturer:");
    print_prop("ro.product.device",       "Device:");
    print_prop("ro.build.version.release","Android:");
    print_prop("ro.build.version.sdk",    "SDK:");
    print_prop("ro.build.fingerprint",    "Fingerprint:");

    return 0;
}