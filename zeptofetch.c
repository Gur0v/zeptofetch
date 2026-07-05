#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "config.h"

#define VERSION     "v2.0"
#define COPYRIGHT   "2024-2026"

#define MAX_PATH    4096
#define MAX_CHAIN   64
#define MAX_LINE    128
#define MAX_NAME    64
#define MAX_PID     4194304

#define ARRLEN(a)   (sizeof(a) / sizeof((a)[0]))

typedef struct {
    pid_t pid;
    pid_t ppid;
    char exe[MAX_PATH];
} proc_t;

typedef struct {
    const char *id;
    size_t len;
} match_t;

typedef struct {
    char user[MAX_NAME];
    char host[MAX_NAME];
    char os[MAX_LINE];
    char kernel[MAX_NAME];
    char shell[MAX_NAME];
    char wm[MAX_NAME];
    char term[MAX_NAME];
} info_t;

static const match_t shells[] = {
    {"bash", 4}, {"zsh", 3}, {"fish", 4}, {"dash", 4},
    {"sh", 2}, {"ksh", 3}, {"tcsh", 4}, {"csh", 3},
    {"elvish", 6}, {"nushell", 7}, {"xonsh", 5}, {"ion", 3},
    {"oil", 3}, {"murex", 5}, {"powershell", 10}, {"pwsh", 4},
    {"rc", 2}, {"es", 2}, {"yash", 4}, {"mksh", 4},
    {"oksh", 4}, {"pdksh", 5},
};

static const match_t terms[] = {
    {"alacritty", 9}, {"kitty", 5}, {"wezterm", 7}, {"gnome-terminal", 14},
    {"konsole", 7}, {"xfce4-terminal", 14}, {"foot", 4}, {"ghostty", 7},
    {"terminator", 10}, {"xterm", 5}, {"urxvt", 5}, {"st", 2},
    {"tilix", 5}, {"guake", 5}, {"yakuake", 7}, {"terminology", 11},
    {"mate-terminal", 13}, {"lxterminal", 10}, {"sakura", 6}, {"tilda", 5},
    {"termite", 7}, {"roxterm", 7}, {"hyper", 5}, {"tabby", 5},
    {"rio", 3}, {"contour", 7}, {"ptyxis", 6}, {"cosmic-term", 11},
    {"warp", 4}, {"wave", 4}, {"extraterm", 9}, {"zutty", 5},
    {"cool-retro-term", 15}, {"mlterm", 6}, {"aterm", 5}, {"eterm", 5},
    {"kterm", 5}, {"qterminal", 9}, {"lilyterm", 8}, {"evilvte", 7},
    {"mrxvt", 5}, {"fbterm", 6}, {"nxterm", 6}, {"pterm", 5},
    {"termine", 7}, {"wterm", 5}, {"xvt", 3}, {"yaft", 4},
};

static const match_t wms[] = {
    {"Hyprland", 8}, {"sway", 4}, {"kwin", 4}, {"mutter", 6},
    {"openbox", 7}, {"i3", 2}, {"bspwm", 5}, {"awesome", 7},
    {"dwm", 3}, {"xmonad", 6}, {"muffin", 6}, {"marco", 5},
    {"wayfire", 7}, {"river", 5}, {"labwc", 5}, {"niri", 4},
    {"xfwm4", 5}, {"fluxbox", 7}, {"icewm", 5}, {"jwm", 3},
    {"gnome-shell", 11}, {"cinnamon", 8}, {"mate-session", 12},
    {"enlightenment", 13}, {"qtile", 5}, {"leftwm", 6},
    {"herbstluftwm", 12}, {"spectrwm", 8}, {"ratpoison", 9},
    {"stumpwm", 7}, {"sawfish", 7}, {"fvwm", 4}, {"fvwm3", 5},
    {"fvwm-crystal", 12}, {"pekwm", 5}, {"windowmaker", 11},
    {"afterstep", 9}, {"blackbox", 8}, {"wmaker", 6}, {"cwm", 3},
    {"2bwm", 4}, {"berry", 5}, {"cage", 4}, {"catwm", 5},
    {"compiz", 6}, {"ctwm", 4}, {"dminiwm", 7}, {"echinus", 7},
    {"evilwm", 6}, {"frankenwm", 9}, {"goomwwm", 7}, {"ion", 3},
    {"lfwm", 4}, {"metacity", 8}, {"notion", 6}, {"olivetti", 8},
    {"plwm", 4}, {"snapwm", 6}, {"tinywm", 6}, {"trayer", 6},
    {"twm", 3}, {"vwm", 3}, {"waimea", 6}, {"wmii", 4},
    {"wmx", 3}, {"acme", 4}, {"mango", 5},
};

static void
str_cpy(char *dst, const char *src, size_t size)
{
    if (!dst || !src || size == 0) return;

    size_t i = 0;
    while (i + 1 < size && src[i]) {
        unsigned char c = (unsigned char)src[i];
        dst[i] = (c >= 32 && c <= 126) ? (char)c : '_';
        i++;
    }
    dst[i] = '\0';
}

static int
str_eq(const char *a, const char *b)
{
    return a && b && strcmp(a, b) == 0;
}

static int
valid_pid(pid_t pid)
{
    return pid > 0 && pid <= MAX_PID;
}

static unsigned long
fast_atoul(const char *s)
{
    unsigned long n = 0;
    while (*s >= '0' && *s <= '9')
        n = n * 10 + (unsigned long)(*s++ - '0');
    return n;
}

static void
proc_path(pid_t pid, const char *file, char *buf, size_t size)
{
    if (!valid_pid(pid) || !file || !*file || strchr(file, '/')) {
        if (size) buf[0] = '\0';
        return;
    }
    snprintf(buf, size, "/proc/%d/%s", (int)pid, file);
}

static int
read_link(const char *path, char *buf, size_t size)
{
    ssize_t len = readlink(path, buf, size - 1);
    if (len <= 0) return -1;
    buf[len] = '\0';
    return 0;
}

static int
read_first_line(const char *path, char *buf, size_t size)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;

    ssize_t n = read(fd, buf, size - 1);
    close(fd);

    if (n <= 0) return -1;
    buf[n] = '\0';

    size_t len = strcspn(buf, "\n");
    buf[len] = '\0';
    return 0;
}

static int
read_proc(pid_t pid, proc_t *p)
{
    char path[64], buf[4096];

    if (!valid_pid(pid) || !p) return -1;

    p->pid = pid;
    p->ppid = -1;
    p->exe[0] = '\0';

    proc_path(pid, "stat", path, sizeof(path));
    if (read_first_line(path, buf, sizeof(buf)) != 0) return -1;

    char *rparen = strrchr(buf, ')');
    if (!rparen || rparen[1] != ' ' || !rparen[2] || rparen[3] != ' ')
        return -1;

    p->ppid = (pid_t)fast_atoul(rparen + 4);
    if (!valid_pid(p->ppid)) p->ppid = -1;

    proc_path(pid, "exe", path, sizeof(path));
    (void)read_link(path, p->exe, sizeof(p->exe));

    return 0;
}

static size_t
build_chain(proc_t *chain, size_t max)
{
    size_t count = 0;
    pid_t pid = getpid();

    while (valid_pid(pid) && count < max) {
        if (read_proc(pid, &chain[count]) != 0) break;

        pid_t next = chain[count].ppid;
        count++;

        if (next == pid || !valid_pid(next)) break;
        pid = next;
    }

    return count;
}

static const char *
find_match(const char *name, const match_t *list, size_t count)
{
    if (!name || !*name) return NULL;

    for (size_t i = 0; i < count; i++) {
        if (name[0] != list[i].id[0]) continue;
        if (strncmp(name, list[i].id, list[i].len) != 0) continue;

        char sep = name[list[i].len];
        if (sep == '\0' || sep == '-' || sep == '.' || sep == '_')
            return list[i].id;
    }

    return NULL;
}

static void
base_name(const char *path, char *out, size_t size)
{
    const char *base = strrchr(path, '/');
    str_cpy(out, base ? base + 1 : path, size);
}

static void
fetch_user(char *buf, size_t size)
{
    struct passwd *pw = getpwuid(getuid());
    str_cpy(buf, (pw && pw->pw_name) ? pw->pw_name : "user", size);
}

static void
fetch_host(char *buf, size_t size)
{
    if (gethostname(buf, size) != 0)
        str_cpy(buf, "localhost", size);
    else
        buf[size - 1] = '\0';
}

static int
parse_os_value(const char *key, char *line, char *out, size_t size)
{
    size_t klen = strlen(key);
    if (strncmp(line, key, klen) != 0) return -1;

    char *value = line + klen;
    value[strcspn(value, "\n")] = '\0';

    if ((*value == '"' || *value == '\'') && value[1]) {
        char quote = *value++;
        char *end = strrchr(value, quote);
        if (end) *end = '\0';
    }

    if (!*value) return -1;
    str_cpy(out, value, size);
    return 0;
}

static void
fetch_os(char *buf, size_t size)
{
    FILE *f = fopen("/etc/os-release", "r");
    if (!f) {
        str_cpy(buf, "Linux", size);
        return;
    }

    char line[MAX_LINE], name[MAX_LINE] = "";
    buf[0] = '\0';

    while (fgets(line, sizeof(line), f)) {
        if (parse_os_value("PRETTY_NAME=", line, buf, size) == 0) break;
        if (!*name) parse_os_value("NAME=", line, name, sizeof(name));
    }

    fclose(f);

    if (!*buf)
        str_cpy(buf, *name ? name : "Linux", size);
}

static void
fetch_kernel(char *buf, size_t size)
{
    struct utsname un;
    if (uname(&un) == 0)
        str_cpy(buf, un.release, size);
    else
        str_cpy(buf, "unknown", size);
}

static void
fetch_shell(const proc_t *chain, size_t count, char *buf, size_t size)
{
    char name[MAX_NAME];

    for (size_t i = 0; i < count; i++) {
        if (!chain[i].exe[0]) continue;
        base_name(chain[i].exe, name, sizeof(name));
        if (find_match(name, shells, ARRLEN(shells))) {
            str_cpy(buf, name, size);
            return;
        }
    }

    char *env = getenv("SHELL");
    if (env && *env) {
        base_name(env, buf, size);
        return;
    }

    str_cpy(buf, "unknown", size);
}

static void
fetch_term(const proc_t *chain, size_t count, char *buf, size_t size)
{
    char *env = getenv("TERM_PROGRAM");
    if (env && *env) {
        str_cpy(buf, env, size);
        return;
    }

    env = getenv("TERMINAL");
    if (env && *env) {
        base_name(env, buf, size);
        return;
    }

    char name[MAX_NAME];
    for (size_t i = 1; i < count; i++) {
        if (!chain[i].exe[0]) continue;
        base_name(chain[i].exe, name, sizeof(name));
        if (find_match(name, shells, ARRLEN(shells))) continue;

        const char *match = find_match(name, terms, ARRLEN(terms));
        str_cpy(buf, match ? match : name, size);
        return;
    }

    env = getenv("TERM");
    str_cpy(buf, (env && *env) ? env : "unknown", size);
}

static int
detect_console(char *term, size_t term_size, char *wm, size_t wm_size)
{
    char path[MAX_PATH];
    if (read_link("/proc/self/fd/0", path, sizeof(path)) != 0) return 0;

    if (strncmp(path, "/dev/tty", 8) != 0 ||
        path[8] < '0' || path[8] > '9')
        return 0;

    base_name(path, term, term_size);
    str_cpy(wm, "none", wm_size);
    return 1;
}

static int
detect_container(void)
{
    char *env = getenv("CONTAINER_ID");
    if (env && *env) return 1;
    if (access("/.dockerenv", F_OK) == 0) return 1;

    struct stat st;
    if (stat("/run/.containerenv", &st) == 0) return 1;

    char buf[512];
    if (read_first_line("/proc/1/cgroup", buf, sizeof(buf)) != 0) return 0;

    return strstr(buf, "docker") || strstr(buf, "lxc") ||
           strstr(buf, "containerd") || strstr(buf, "podman");
}

static int
detect_ssh(void)
{
    return getenv("SSH_CLIENT") != NULL ||
           getenv("SSH_TTY") != NULL ||
           getenv("SSH_CONNECTION") != NULL;
}

static int
detect_wsl(void)
{
    char buf[512];

    if (getenv("WSLENV")) return 1;
    if (access("/mnt/wsl", F_OK) == 0) return 1;
    if (access("/proc/sys/fs/binfmt_misc/WSLInterop", F_OK) == 0) return 1;

    if (read_first_line("/proc/sys/kernel/osrelease", buf, sizeof(buf)) == 0 &&
        (strstr(buf, "WSL") || strstr(buf, "microsoft")))
        return 1;

    if (read_first_line("/proc/version", buf, sizeof(buf)) == 0 &&
        (strstr(buf, "WSL") || strstr(buf, "Microsoft")))
        return 1;

    return 0;
}

static void
fetch_wsl_term(char *buf, size_t size)
{
    if (getenv("WT_SESSION") || getenv("WT_PROFILE_ID")) {
        str_cpy(buf, "Windows Terminal", size);
        return;
    }

    char *env = getenv("TERM");
    str_cpy(buf, (env && *env) ? env : "", size);
}

static void
fetch_wsl_wm(char *buf, size_t size)
{
    char *wayland = getenv("WAYLAND_DISPLAY");
    char *display = getenv("DISPLAY");
    str_cpy(buf, ((wayland && *wayland) || (display && *display)) ? "WSLg" : "unknown", size);
}

static void
fetch_wm(char *buf, size_t size)
{
    DIR *dir = opendir("/proc");
    if (!dir) {
        str_cpy(buf, "unknown", size);
        return;
    }

    struct dirent *entry;
    char path[64], comm[MAX_NAME];

    while ((entry = readdir(dir))) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;

        proc_path((pid_t)fast_atoul(entry->d_name), "comm", path, sizeof(path));
        if (read_first_line(path, comm, sizeof(comm)) != 0) continue;

        const char *match = find_match(comm, wms, ARRLEN(wms));
        if (match) {
            str_cpy(buf, match, size);
            closedir(dir);
            return;
        }
    }

    closedir(dir);
    str_cpy(buf, "unknown", size);
}

static void
read_base_info(info_t *info)
{
    fetch_user(info->user, sizeof(info->user));
    fetch_host(info->host, sizeof(info->host));
    fetch_os(info->os, sizeof(info->os));
    fetch_kernel(info->kernel, sizeof(info->kernel));
}

static void
detect_session(info_t *info, const proc_t *chain, size_t count)
{
    fetch_shell(chain, count, info->shell, sizeof(info->shell));

    if (detect_console(info->term, sizeof(info->term), info->wm, sizeof(info->wm)))
        return;

    if (detect_container()) {
        str_cpy(info->term, "container", sizeof(info->term));
        str_cpy(info->wm, "none", sizeof(info->wm));
        return;
    }

    if (detect_ssh()) {
        str_cpy(info->term, "ssh", sizeof(info->term));
        str_cpy(info->wm, "none", sizeof(info->wm));
        return;
    }

    if (detect_wsl()) {
        fetch_wsl_term(info->term, sizeof(info->term));
        if (!info->term[0]) fetch_term(chain, count, info->term, sizeof(info->term));
        fetch_wsl_wm(info->wm, sizeof(info->wm));
        if (str_eq(info->wm, "unknown")) fetch_wm(info->wm, sizeof(info->wm));
        return;
    }

    fetch_term(chain, count, info->term, sizeof(info->term));
    fetch_wm(info->wm, sizeof(info->wm));
}

static void
kernel_short(char *dst, size_t size, const char *src)
{
    size_t i = 0;
    while (i + 1 < size && src[i] && src[i] != ' ') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void
print_sep(size_t len)
{
    while (len--) putchar('-');
    putchar('\n');
}

static void
display(const info_t *info, int color)
{
    const char *c1 = color ? C1 : "";
    const char *c2 = color ? C2 : "";
    const char *c3 = color ? C3 : "";
    const char *cr = color ? CR : "";

    char krel[MAX_NAME], userhost[MAX_NAME * 2 + 1];
    kernel_short(krel, sizeof(krel), info->kernel);

    int n = snprintf(userhost, sizeof(userhost), "%s@%s", info->user, info->host);
    size_t sep = (n > 0 && (size_t)n < sizeof(userhost)) ? (size_t)n : 0;

    printf("%s    ___ %s     %s%s@%s%s\n", c1, cr, c1, info->user, info->host, cr);
    printf("%s   (%s.· %s|%s     ", c1, c2, c1, cr);
    print_sep(sep);
    printf("%s   (%s<>%s %s|%s     %sOS:%s %s\n", c1, c3, cr, c1, cr, c3, cr, info->os);
    printf("%s  / %s__  %s\\%s    %sKernel:%s %s\n", c1, c2, c1, cr, c3, cr, krel);
    printf("%s ( %s/  \\ %s/|%s   %sShell:%s %s\n", c1, c2, c1, cr, c3, cr, info->shell);
    printf("%s_%s/\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n", c3, c1, c2, c1, c3, c1, cr, c3, cr, info->wm);
    printf("%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n", c1, c3, c1, c3, cr, c3, cr, info->term);
}

static void
print_version(void)
{
    printf("zeptofetch %s\n", VERSION);
    printf("Copyright (C) %s Gurov\n", COPYRIGHT);
    printf("Licensed under GPL-3.0\n\n");
    printf("BUILD: %s %s UTC\n", __DATE__, __TIME__);
    printf("CONFIG: CHAIN=%d PID=%d\n", MAX_CHAIN, MAX_PID);
}

static int
set_limits(void)
{
    struct rlimit rlim;

#ifndef __SANITIZE_ADDRESS__
    rlim.rlim_cur = rlim.rlim_max = 50 * 1024 * 1024;
    if (setrlimit(RLIMIT_AS, &rlim)) return -1;
#endif

    rlim.rlim_cur = rlim.rlim_max = 5;
    if (setrlimit(RLIMIT_CPU, &rlim)) return -1;

    rlim.rlim_cur = rlim.rlim_max = 256;
    if (setrlimit(RLIMIT_NOFILE, &rlim)) return -1;

    return 0;
}

static int
harden(void)
{
    (void)prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    (void)prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);

    if (getegid() != getgid() && setgid(getgid())) return -1;
    if (geteuid() != getuid() && setuid(getuid())) return -1;

    if (set_limits())
        fprintf(stderr, "Warn: limits failed\n");

    return 0;
}

int
main(int argc, char **argv)
{
    if (harden()) return 1;

    if (access("/proc/self/stat", R_OK) != 0) {
        fprintf(stderr, "Error: cannot read /proc (hidepid enabled?)\n");
        return 1;
    }

    if (argc > 1 && (str_eq(argv[1], "--version") || str_eq(argv[1], "-v"))) {
        print_version();
        return 0;
    }

    proc_t chain[MAX_CHAIN];
    info_t info;

    size_t count = build_chain(chain, ARRLEN(chain));
    read_base_info(&info);
    detect_session(&info, chain, count);
    display(&info, isatty(STDOUT_FILENO));

    return 0;
}
