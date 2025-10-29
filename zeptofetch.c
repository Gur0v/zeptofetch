#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <limits.h>
#include <ctype.h>
#include <locale.h>
#include <dirent.h>
#include <errno.h>
#include "config.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_NAME          128
#define MAX_SMALL         64
#define MAX_LINE          64
#define PID_MAX           4194304
#define CACHE_SIZE        512
#define MAX_CHAIN_DEPTH   1000
#define VERSION           "v1.1"

typedef struct {
    pid_t pid;
    pid_t ppid;
    char exe[PATH_MAX];
    int has_exe;
    int has_ppid;
} proc_t;

static proc_t cache[CACHE_SIZE];
static size_t cache_cnt = 0;
static char wm_cached[MAX_SMALL] = {0};
static int wm_found = 0;

static void str_copy(char *dst, const char *src, size_t sz)
{
    if (!dst || sz == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t len = strlen(src);
    if (len < sz) {
        memcpy(dst, src, len + 1);
    } else {
        memcpy(dst, src, sz - 1);
        dst[sz - 1] = '\0';
    }
}

static int valid_pid(pid_t pid)
{
    return pid > 0 && pid <= PID_MAX;
}

static proc_t *cache_get(pid_t pid)
{
    for (size_t i = 0; i < cache_cnt; ++i) {
        if (cache[i].pid == pid) return &cache[i];
    }
    return NULL;
}

static proc_t *cache_add(pid_t pid)
{
    if (cache_cnt >= CACHE_SIZE) return NULL;
    cache[cache_cnt].pid = pid;
    cache[cache_cnt].ppid = -1;
    cache[cache_cnt].exe[0] = '\0';
    cache[cache_cnt].has_exe = 0;
    cache[cache_cnt].has_ppid = 0;
    return &cache[cache_cnt++];
}

static int read_ppid(pid_t pid, pid_t *out)
{
    char path[PATH_MAX];
    int r = snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    if (r < 0 || (size_t)r >= sizeof(path)) return -1;
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int ok = fscanf(f, "%*d %*s %*c %d", out);
    fclose(f);
    return (ok == 1) ? 0 : -1;
}

static int read_exe(pid_t pid, char *buf, size_t sz)
{
    char path[PATH_MAX];
    int r = snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    if (r < 0 || (size_t)r >= sizeof(path)) return -1;
    ssize_t len = readlink(path, buf, sz - 1);
    if (len <= 0) return -1;
    buf[len] = '\0';
    return 0;
}

static int get_proc(pid_t pid, proc_t *p)
{
    if (!valid_pid(pid) || !p) return -1;
    proc_t *cached = cache_get(pid);
    if (cached) {
        if (p != cached) *p = *cached;
        return 0;
    }
    proc_t *e = cache_add(pid);
    if (!e) return -1;
    e->pid = pid;
    if (read_ppid(pid, &e->ppid) == 0) {
        e->has_ppid = 1;
    } else {
        e->ppid = -1;
    }
    if (read_exe(pid, e->exe, sizeof(e->exe)) == 0) {
        e->has_exe = 1;
    } else {
        e->exe[0] = '\0';
        e->has_exe = 0;
    }
    *p = *e;
    return 0;
}

static size_t build_chain(pid_t start, proc_t *out, size_t max)
{
    size_t idx = 0;
    pid_t cur = start;
    while (valid_pid(cur) && idx < max && idx < MAX_CHAIN_DEPTH) {
        if (get_proc(cur, &out[idx]) != 0) break;
        pid_t ppid = out[idx].has_ppid ? out[idx].ppid : -1;
        if (ppid == cur || ppid <= 0) {
            idx++;
            break;
        }
        cur = ppid;
        idx++;
    }
    return idx;
}

static int str_eq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

static int str_prefix(const char *s, const char *pre, size_t pre_len)
{
    return strncmp(s, pre, pre_len) == 0;
}

static int is_shell(const char *n)
{
    if (!n) return 0;
    switch (n[0]) {
        case 'b': return str_eq(n, "bash");
        case 'z': return str_eq(n, "zsh");
        case 's': return str_eq(n, "sh");
        case 'f': return str_eq(n, "fish");
        case 'd': return str_eq(n, "dash");
        case 'k': return str_eq(n, "ksh");
        case 't': return str_eq(n, "tcsh");
        case 'c': return str_eq(n, "csh");
        case 'e': return str_eq(n, "elvish");
        case 'y': return str_eq(n, "yash");
        default:  return 0;
    }
}

static void basename_of(const char *path, char *out, size_t sz)
{
    if (!path || !out || sz == 0) return;
    const char *b = strrchr(path, '/');
    str_copy(out, b ? b + 1 : path, sz);
}

static void get_user(char *buf, size_t sz)
{
    struct passwd *pw = getpwuid(getuid());
    str_copy(buf, (pw && pw->pw_name) ? pw->pw_name : "user", sz);
}

static void get_host(char *buf, size_t sz)
{
    if (gethostname(buf, sz) != 0) {
        str_copy(buf, "localhost", sz);
        return;
    }
    buf[sz - 1] = '\0';
}

static void get_shell(proc_t *chain, size_t cnt, char *buf, size_t sz)
{
    char base[MAX_NAME];
    for (size_t i = 0; i < cnt; ++i) {
        if (!chain[i].has_exe) continue;
        basename_of(chain[i].exe, base, sizeof(base));
        if (is_shell(base)) {
            str_copy(buf, base, sz);
            return;
        }
    }
    str_copy(buf, "unknown", sz);
}

static void get_term(proc_t *chain, size_t cnt, char *buf, size_t sz)
{
    static const struct {
        const char *name;
        size_t len;
    } terms[] = {
        {"gnome-terminal", 14}, {"urxvt", 5}, {"konsole", 7}, {"alacritty", 9},
        {"xterm", 5}, {"kitty", 5}, {"foot", 4}, {"wezterm", 7}, {"st", 2},
        {"termite", 7}, {"terminator", 10}, {"tilix", 5}, {"terminology", 11},
        {"qterminal", 9}, {"lxterminal", 10}, {"mate-terminal", 13},
        {"xfce4-terminal", 14}, {"ptyxis", 6}, {"rio", 3}, {"ghostty", 7},
        {"contour", 7}, {"tabby", 5}, {"hyper", 5}, {"yakuake", 7},
        {"roxterm", 7}, {"sakura", 6}, {"tilda", 5}, {"guake", 5},
        {"cool-retro-term", 15}, {"eterm", 5}, {"mlterm", 6}, {"lilyterm", 8},
        {"aterm", 5}, {"evilvte", 7}, {"xvt", 3}, {"nxterm", 6}, {"kterm", 5},
        {"mrxvt", 5}, {"pterm", 5}, {"wterm", 5}, {"yaft", 4}, {"fbterm", 6}
    };

    char base[MAX_NAME];
    for (size_t i = 1; i < cnt; ++i) {
        if (!chain[i].has_exe) continue;
        basename_of(chain[i].exe, base, sizeof(base));
        if (is_shell(base)) continue;
        char fc = base[0];
        for (size_t j = 0; j < sizeof(terms) / sizeof(terms[0]); ++j) {
            if (fc != terms[j].name[0]) continue;
            if (str_eq(base, terms[j].name) ||
                str_prefix(base, terms[j].name, terms[j].len)) {
                str_copy(buf, terms[j].name, sz);
                return;
            }
        }
        if (base[0] != '\0') {
            str_copy(buf, base, sz);
            return;
        }
    }
    str_copy(buf, "unknown", sz);
}

static int read_comm(const char *pid_str, char *comm, size_t sz)
{
    char path[PATH_MAX];
    int r = snprintf(path, sizeof(path), "/proc/%s/comm", pid_str);
    if (r < 0 || (size_t)r >= sizeof(path)) return -1;
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(comm, sz, f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    comm[strcspn(comm, "\n")] = '\0';
    return 0;
}

static void get_wm(char *buf, size_t sz)
{
    if (wm_found) {
        str_copy(buf, wm_cached, sz);
        return;
    }

    static const struct {
        const char *name;
        size_t len;
    } wms[] = {
        {"acme", 4}, {"afterstep", 9}, {"awesome", 7}, {"berry", 5},
        {"bspwm", 5}, {"cage", 4}, {"catwm", 5}, {"compiz", 6},
        {"cinnamon", 8}, {"ctwm", 4}, {"dminiwm", 7}, {"dwm", 3},
        {"echinus", 7}, {"enlightenment", 13}, {"evilwm", 6}, {"fluxbox", 7},
        {"frankenwm", 9}, {"fvwm", 4}, {"fvwm-crystal", 12}, {"goomwwm", 7},
        {"herbstluftwm", 12}, {"hyprland", 8}, {"icewm", 5}, {"ion", 3},
        {"jwm", 3}, {"kwin", 4}, {"leftwm", 6}, {"labwc", 5}, {"lfwm", 4},
        {"marco", 5}, {"metacity", 8}, {"muffin", 6}, {"mutter", 6},
        {"notion", 6}, {"olivetti", 8}, {"openbox", 7}, {"pekwm", 5},
        {"plwm", 4}, {"ratpoison", 9}, {"river", 5}, {"sway", 4},
        {"spectrwm", 8}, {"stumpwm", 7}, {"sawfish", 7}, {"snapwm", 6},
        {"tinywm", 6}, {"trayer", 6}, {"twm", 3}, {"vwm", 3}, {"waimea", 6},
        {"wayfire", 7}, {"wmii", 4}, {"wmx", 3}, {"xfwm4", 5}, {"xmonad", 6},
        {"gnome-shell", 11}, {"mate-session", 12}, {"niri", 4}
    };

    DIR *proc = opendir("/proc");
    if (!proc) {
        str_copy(buf, "unknown", sz);
        return;
    }

    struct dirent *ent;
    char comm[MAX_SMALL];
    while ((ent = readdir(proc))) {
        if (ent->d_name[0] < '0' || ent->d_name[0] > '9') continue;
        if (read_comm(ent->d_name, comm, sizeof(comm)) != 0) continue;
        if (comm[0] == '\0') continue;

        char fc = comm[0];
        for (size_t i = 0; i < sizeof(wms) / sizeof(wms[0]); ++i) {
            if (fc != wms[i].name[0]) continue;
            if (str_eq(comm, wms[i].name) ||
                str_prefix(comm, wms[i].name, wms[i].len)) {
                if (str_eq(comm, "gnome-shell")) {
                    str_copy(buf, "mutter", sz);
                } else if (str_eq(comm, "cinnamon")) {
                    str_copy(buf, "muffin", sz);
                } else if (str_eq(comm, "mate-session")) {
                    str_copy(buf, "marco", sz);
                } else if (str_prefix(comm, "kwin", 4)) {
                    str_copy(buf, "kwin", sz);
                } else {
                    str_copy(buf, wms[i].name, sz);
                }
                str_copy(wm_cached, buf, sizeof(wm_cached));
                wm_found = 1;
                closedir(proc);
                return;
            }
        }
    }
    closedir(proc);
    str_copy(buf, "unknown", sz);
    str_copy(wm_cached, "unknown", sizeof(wm_cached));
    wm_found = 1;
}

static void get_os(char *buf, size_t sz)
{
    FILE *f = fopen("/etc/os-release", "r");
    if (!f) {
        str_copy(buf, "Linux", sz);
        return;
    }

    char line[MAX_LINE];
    char pretty_name[MAX_LINE] = {0};
    char name_buf[MAX_LINE] = {0};
    int found_pretty = 0;
    int found_name = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *start = strchr(line, '"');
            char *end = strrchr(line, '"');
            if (start && end && start < end) {
                *end = '\0';
                str_copy(pretty_name, start + 1, sizeof(pretty_name));
                found_pretty = 1;
                break;
            }
        } else if (strncmp(line, "NAME=", 5) == 0 && !found_name) {
            char *start = strchr(line, '"');
            char *end = strrchr(line, '"');
            if (start && end && start < end) {
                *end = '\0';
                str_copy(name_buf, start + 1, sizeof(name_buf));
                found_name = 1;
            } else {
                char *p = line + 5;
                p[strcspn(p, "\n")] = '\0';
                str_copy(name_buf, p, sizeof(name_buf));
                found_name = 1;
            }
        }
    }

    if (found_pretty) {
        str_copy(buf, pretty_name, sz);
    } else if (found_name) {
        str_copy(buf, name_buf, sz);
    } else {
        str_copy(buf, "Linux", sz);
    }

    fclose(f);
}

static void print_sep(size_t len)
{
    for (size_t i = 0; i < len; ++i) putchar('-');
    putchar('\n');
}

static void print_version(void)
{
    printf("zeptofetch %s\n", VERSION);
    printf("Copyright (C) 2025 Gurov\n");
    printf("Licensed under GPL-3.0\n");
    printf("\n");
    printf("BUILD: %s %s UTC | ", __DATE__, __TIME__);
    
#if defined(__clang__)
    printf("Clang %d.%d.%d | ", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
    #ifdef __GNUC_PATCHLEVEL__
    printf("GCC %d.%d.%d | ", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    #else
    printf("GCC %d.%d | ", __GNUC__, __GNUC_MINOR__);
    #endif
#else
    printf("Unknown | ");
#endif

#if defined(__x86_64__) || defined(_M_X64)
    printf("x86_64\n");
#elif defined(__i386__) || defined(_M_IX86)
    printf("i386\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
    printf("aarch64\n");
#elif defined(__arm__) || defined(_M_ARM)
    printf("arm\n");
#elif defined(__riscv)
    printf("riscv%d\n", __riscv_xlen);
#elif defined(__powerpc64__)
    printf("ppc64\n");
#elif defined(__powerpc__)
    printf("ppc\n");
#else
    printf("unknown\n");
#endif

    printf("CONFIG: CACHE=%d CHAIN=%d PATH=%d PID=%d\n", 
           CACHE_SIZE, MAX_CHAIN_DEPTH, PATH_MAX, PID_MAX);
}

static void display(const char *user, const char *host, const char *os,
                    const char *kern, const char *shell, const char *wm,
                    const char *term)
{
    size_t len = strlen(user) + strlen(host) + 1;
    printf("%s    ___ %s     %s%s@%s%s\n",
           COLOR_1, COLOR_RESET, COLOR_1, user, host, COLOR_RESET);
    printf("%s   (%s.· %s|%s     ", COLOR_1, COLOR_2, COLOR_1, COLOR_RESET);
    print_sep(len);
    printf("%s   (%s<>%s %s|%s     %sOS:%s %s\n",
           COLOR_1, COLOR_3, COLOR_RESET, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, os);
    printf("%s  / %s__  %s\\%s    %sKernel:%s %s\n",
           COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, kern);
    printf("%s ( %s/  \\ %s/|%s   %sShell:%s %s\n",
           COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, shell);
    printf("%s%s_/%s\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n",
           COLOR_1, COLOR_3, COLOR_1, COLOR_2, COLOR_1, COLOR_3, COLOR_1, COLOR_RESET,
           COLOR_3, COLOR_RESET, wm);
    printf("%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n",
           COLOR_1, COLOR_3, COLOR_1, COLOR_3, COLOR_RESET, COLOR_3, COLOR_RESET, term);
}

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    if (argc > 1) {
        if (str_eq(argv[1], "--version") || str_eq(argv[1], "-v")) {
            print_version();
            return 0;
        }
    }

    char user[MAX_SMALL];
    char host[MAX_SMALL];
    char shell[MAX_SMALL];
    char wm[MAX_SMALL];
    char term[MAX_SMALL];
    char os[MAX_NAME];
    struct utsname info;

    if (uname(&info) != 0) return 1;

    get_user(user, sizeof(user));
    get_host(host, sizeof(host));

    proc_t chain[CACHE_SIZE];
    cache_cnt = 0;
    size_t cnt = build_chain(getpid(), chain, sizeof(chain) / sizeof(chain[0]));

    get_shell(chain, cnt, shell, sizeof(shell));
    get_term(chain, cnt, term, sizeof(term));
    get_wm(wm, sizeof(wm));
    get_os(os, sizeof(os));

    display(user, host, os, info.release, shell, wm, term);
    return 0;
}
