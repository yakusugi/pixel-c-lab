// netpeek.c - Read-only overview of /proc/net/tcp and /proc/net/udp
// If /proc is blocked, fall back to `su -c cat` (still read-only).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* tcp_state(unsigned int st) {
    switch (st) {
        case 0x01: return "ESTABLISHED";
        case 0x02: return "SYN_SENT";
        case 0x03: return "SYN_RECV";
        case 0x04: return "FIN_WAIT1";
        case 0x05: return "FIN_WAIT2";
        case 0x06: return "TIME_WAIT";
        case 0x07: return "CLOSE";
        case 0x08: return "CLOSE_WAIT";
        case 0x09: return "LAST_ACK";
        case 0x0A: return "LISTEN";
        case 0x0B: return "CLOSING";
        default:   return "UNKNOWN";
    }
}

static void ip_from_hex(unsigned int hex, char out[16]) {
    // /proc/net/tcp uses little-endian hex for IPv4
    unsigned int b1 = (hex) & 0xFF;
    unsigned int b2 = (hex >> 8) & 0xFF;
    unsigned int b3 = (hex >> 16) & 0xFF;
    unsigned int b4 = (hex >> 24) & 0xFF;
    snprintf(out, 16, "%u.%u.%u.%u", b1, b2, b3, b4);
}

static FILE* open_source(const char *path) {
    FILE *fp = fopen(path, "r");
    if (fp) return fp;

    // Root fallback: read file via su (still read-only)
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "su -c cat %s 2>/dev/null", path);
    fp = popen(cmd, "r");
    return fp; // may be NULL
}

static void close_source(FILE *fp, int via_popen) {
    if (via_popen) pclose(fp);
    else fclose(fp);
}

static void dump_table(const char *label, const char *path, int limit) {
    FILE *fp = fopen(path, "r");
    int via_popen = 0;
    if (!fp) {
        fp = open_source(path);
        via_popen = 1;
    }
    if (!fp) {
        printf("== %s ==\n", label);
        printf("blocked: cannot read %s (try running with root)\n\n", path);
        return;
    }

    char line[512];
    // skip header
    if (!fgets(line, sizeof(line), fp)) { close_source(fp, via_popen); return; }

    printf("== %s ==\n", label);
    printf("%-12s %-22s %-22s\n", "state", "local", "remote");

    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        unsigned int lip=0, rip=0, lport=0, rport=0, st=0;

        // Example fields: local_address rem_address st ...
        // We'll parse only the parts we need:
        // "%*d: %8X:%4X %8X:%4X %2X"
        if (sscanf(line, " %*d: %8X:%4X %8X:%4X %2X",
                   &lip, &lport, &rip, &rport, &st) != 5) {
            continue;
        }

        char lip_s[16], rip_s[16];
        ip_from_hex(lip, lip_s);
        ip_from_hex(rip, rip_s);

        char local[32], remote[32];
        snprintf(local, sizeof(local), "%s:%u", lip_s, lport);
        snprintf(remote, sizeof(remote), "%s:%u", rip_s, rport);

        printf("%-12s %-22s %-22s\n",
               tcp_state(st), local, remote);

        if (++count >= limit) break;
    }

    if (count == 0) puts("(no entries)\n");
    else puts("");

    close_source(fp, via_popen);
}

int main(int argc, char **argv) {
    int limit = 20;
    if (argc >= 2) {
        limit = atoi(argv[1]);
        if (limit <= 0) limit = 20;
        if (limit > 200) limit = 200;
    }

    puts("== netpeek ==");
    printf("showing up to %d entries per table\n\n", limit);

    dump_table("TCP sockets (/proc/net/tcp)", "/proc/net/tcp", limit);
    dump_table("UDP sockets (/proc/net/udp)", "/proc/net/udp", limit);

    puts("note: this is read-only and does NOT capture packets.");
    return 0;
}