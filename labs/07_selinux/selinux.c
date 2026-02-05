// selinux.c - Quiet SELinux mode check (read-only), with su fallback

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void trim(char *s){
    size_t n=strlen(s);
    if(n && s[n-1]=='\n') s[n-1]='\0';
}

static int read_enforce_file(int *out){
    FILE *fp=fopen("/sys/fs/selinux/enforce","r");
    if(!fp) return -1;
    int v=-1;
    if(fscanf(fp,"%d",&v)!=1) v=-1;
    fclose(fp);
    if(v==0||v==1){ *out=v; return 0; }
    return -1;
}

static int run_line(const char *cmd, char *out, size_t out_sz){
    FILE *fp=popen(cmd,"r");
    if(!fp) return -1;
    if(!fgets(out,(int)out_sz,fp)){ pclose(fp); return -1; }
    pclose(fp);
    trim(out);
    return 0;
}

static int is_mode(const char *s){
    return !strcmp(s,"Enforcing") || !strcmp(s,"Permissive") || !strcmp(s,"Disabled");
}

int main(void){
    puts("== SELinux status ==");

    int v=-1;
    if(read_enforce_file(&v)==0){
        printf("mode : %s\n", v ? "Enforcing" : "Permissive");
        return 0;
    }

    char s[128]={0};

    // Try normal getenforce (silenced)
    if(run_line("getenforce 2>/dev/null", s, sizeof(s))==0 && is_mode(s)){
        printf("mode : %s\n", s);
        return 0;
    }

    // Try root fallback (silenced)
    if(run_line("su -c getenforce 2>/dev/null", s, sizeof(s))==0 && is_mode(s)){
        printf("mode : %s (via su)\n", s);
        return 0;
    }

    puts("mode : Unknown (blocked)");
    puts("hint : su -c ./selinux");
    return 1;
}