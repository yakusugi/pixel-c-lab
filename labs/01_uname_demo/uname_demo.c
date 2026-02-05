#include <stdio.h>
#include <sys/utsname.h>

int main(void) {
    struct utsname u;
    if (uname(&u) == 0) {
        printf("sysname : %s\n", u.sysname);
        printf("release : %s\n", u.release);
        printf("version : %s\n", u.version);
        printf("machine : %s\n", u.machine);
        return 0;
    }
    perror("uname");
    return 1;
}