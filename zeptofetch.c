#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include "config.h"

#define VERSION     "v1.17-rc1"
#define COPYRIGHT   "2026"

#define MAX_PATH    4096
#define MAX_CACHE   256
#define MAX_CHAIN   64
#define MAX_LINE    128
#define MAX_NAME    64
#define MAX_PID     4194304
#define MAX_SCAN    512
#define MAX_WM_PID  100000
#define MIN_WM_PID  300
#define WM_MAX_CHK  1000

#define FL_EXE      1
#define FL_PPID     2
#define FL_TIME     4

#define ARRLEN(a)   (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b)   ((a) < (b) ? (a) : (b))

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(MAX_CACHE <= MAX_PID, "cache > pid space");
#else
typedef char sz_chk[(MAX_CACHE <= MAX_PID) ? 1 : -1];
#endif

typedef struct {
    pid_t pid;
    pid_t ppid;
    char exe[MAX_PATH];
    unsigned long long time;
    uint8_t flags;
} proc_t;

typedef struct {
    const char *id;
    size_t len;
} match_t;

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
    {"wmx", 3}, {"acme", 4},
};

static proc_t *cache = NULL;
static size_t cache_len = 0;
static char wm_buf[MAX_NAME] = {0};
static int wm_ok = 0;
static char os_buf[MAX_LINE] = {0};
static int os_ok = 0;
static char host_buf[MAX_NAME] = {0};
static int host_ok = 0;
static int wsl_ok = -1;

static inline void
str_cpy(char *dst, const char *src, size_t max)
{
    if (!dst || !src || !max) return;
    size_t i = 0;
    while (i < max - 1 && src[i]) {
        dst[i] = (src[i] >= 32 && src[i] <= 126) ? src[i] : '_';
        i++;
    }
    dst[i] = '\0';
}

static inline int
valid_pid(pid_t pid)
{
    return pid > 0 && pid <= MAX_PID;
}

static void
secure_zero(void *ptr, size_t len)
{
    volatile unsigned char *p = ptr;
    while (len--) *p++ = 0;
}

static void
make_path(pid_t pid, const char *file, char *buf, size_t bufsize)
{
    if (!valid_pid(pid) || !file || !*file || strchr(file, '/')) {
        buf[0] = '\0';
        return;
    }
    snprintf(buf, bufsize, "/proc/%d/%s", (int)pid, file);
}

static unsigned long long
fast_atoi(const char *str)
{
    unsigned long long res = 0;
    while (*str >= '0' && *str <= '9')
        res = res * 10 + (*str++ - '0');
    return res;
}

static int
parse_stat(pid_t pid, pid_t *ppid, unsigned long long *time)
{
    char path[64], buf[4096];
    make_path(pid, "stat", path, sizeof(path));
    if (!path[0]) return -1;

    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;

    ssize_t r = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (r <= 0) return -1;
    buf[r] = '\0';

    char *ptr = strrchr(buf, ')');
    if (!ptr || ptr[1] != ' ') return -1;
    ptr += 2;

    if (!*ptr || !*(++ptr)) return -1;
    ptr++;

    *ppid = (pid_t)fast_atoi(ptr);
    if (!valid_pid(*ppid)) *ppid = -1;

    for (int i = 0; i < 18; ++i) {
        while (*ptr && *ptr != ' ') ptr++;
        if (!*ptr) return -1;
        ptr++;
    }

    *time = fast_atoi(ptr);
    return 0;
}

static proc_t *
cache_get(pid_t pid)
{
    for (size_t i = 0; i < cache_len; ++i)
        if (cache[i].pid == pid) return &cache[i];
    return NULL;
}

static proc_t *
cache_add(pid_t pid)
{
    if (cache_len >= MAX_CACHE) return NULL;
    proc_t *entry = &cache[cache_len++];
    entry->pid = pid;
    entry->ppid = -1;
    entry->exe[0] = '\0';
    entry->time = 0;
    entry->flags = 0;
    return entry;
}

static int
get_exe(pid_t pid, char *buf, size_t size)
{
    char path[64];
    make_path(pid, "exe", path, sizeof(path));
    if (!path[0]) return -1;
    ssize_t len = readlink(path, buf, size - 1);
    if (len <= 0) return -1;
    buf[len] = '\0';
    return 0;
}

static int
get_proc(pid_t pid, proc_t *out)
{
    if (!valid_pid(pid)) return -1;

    proc_t *cached = cache_get(pid);
    if (cached) {
        if (out != cached) *out = *cached;
        return 0;
    }

    proc_t *entry = cache_add(pid);
    if (!entry) return -1;

    if (parse_stat(pid, &entry->ppid, &entry->time) == 0)
        entry->flags |= (FL_TIME | FL_PPID);

    if (get_exe(pid, entry->exe, sizeof(entry->exe)) == 0)
        entry->flags |= FL_EXE;

    *out = *entry;
    return 0;
}

static size_t
build_chain(pid_t start, proc_t *list, size_t max)
{
    size_t count = 0;
    pid_t curr = start;

    while (valid_pid(curr) && count < max) {
        if (get_proc(curr, &list[count]) != 0) break;
        if ((list[count].flags & FL_PPID) && list[count].ppid != curr && valid_pid(list[count].ppid))
            curr = list[count].ppid;
        else
            break;
        count++;
    }
    return count;
}

static inline int
str_eq(const char *a, const char *b)
{
    return a && b && strcmp(a, b) == 0;
}

static inline int
str_pfx(const char *s, const char *p, size_t len)
{
    return s && p && strncmp(s, p, len) == 0;
}

static int
match_list(const char *name, const match_t *list, size_t count)
{
    if (!name || !*name) return 0;
    for (size_t i = 0; i < count; ++i)
        if (name[0] == list[i].id[0] && (str_eq(name, list[i].id) || str_pfx(name, list[i].id, list[i].len)))
            return 1;
    return 0;
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
    if (host_ok) {
        str_cpy(buf, host_buf, size);
        return;
    }
    if (gethostname(buf, size) != 0)
        str_cpy(buf, "localhost", size);
    else
        buf[size - 1] = '\0';
    str_cpy(host_buf, buf, sizeof(host_buf));
    host_ok = 1;
}

static void
fetch_shell(proc_t *chain, size_t count, char *buf, size_t size)
{
    char *env = getenv("SHELL");
    if (env && *env) {
        base_name(env, buf, size);
        return;
    }

    char tmp[MAX_NAME];
    for (size_t i = 0; i < count; ++i) {
        if (!(chain[i].flags & FL_EXE)) continue;
        base_name(chain[i].exe, tmp, sizeof(tmp));
        if (match_list(tmp, shells, ARRLEN(shells))) {
            str_cpy(buf, tmp, size);
            return;
        }
    }
    str_cpy(buf, "unknown", size);
}

static void
fetch_term(proc_t *chain, size_t count, char *buf, size_t size)
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

    char tmp[MAX_NAME];
    for (size_t i = 1; i < count; ++i) {
        if (!(chain[i].flags & FL_EXE)) continue;
        base_name(chain[i].exe, tmp, sizeof(tmp));
        if (match_list(tmp, shells, ARRLEN(shells))) continue;

        if (match_list(tmp, terms, ARRLEN(terms))) {
            for (size_t j = 0; j < ARRLEN(terms); ++j) {
                if (str_eq(tmp, terms[j].id) || str_pfx(tmp, terms[j].id, terms[j].len)) {
                    str_cpy(buf, terms[j].id, size);
                    return;
                }
            }
        }
        if (tmp[0]) {
            str_cpy(buf, tmp, size);
            return;
        }
    }
    str_cpy(buf, "unknown", size);
}

static int
detect_wsl(void)
{
    if (wsl_ok != -1) return wsl_ok;
    wsl_ok = 0;

    if (getenv("WSLENV")) {
        wsl_ok = 1;
        return 1;
    }

    struct stat st;
    if (stat("/mnt/wsl", &st) == 0) {
        wsl_ok = 1;
        return 1;
    }

    if (access("/proc/sys/fs/binfmt_misc/WSLInterop", F_OK) == 0) {
        wsl_ok = 1;
        return 1;
    }

    char buf[512];
    int fd = open("/proc/sys/kernel/osrelease", O_RDONLY);
    if (fd >= 0) {
        ssize_t r = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (r > 0) {
            buf[r] = '\0';
            if (strstr(buf, "WSL") || strstr(buf, "microsoft")) wsl_ok = 1;
        }
    }
    if (wsl_ok) return 1;

    fd = open("/proc/version", O_RDONLY);
    if (fd >= 0) {
        ssize_t r = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (r > 0) {
            buf[r] = '\0';
            if (strstr(buf, "Microsoft") || strstr(buf, "WSL")) wsl_ok = 1;
        }
    }
    return wsl_ok;
}

static void
fetch_wsl_wm(char *buf, size_t size)
{
    char *env = getenv("WAYLAND_DISPLAY");
    if (env && *env) {
        str_cpy(buf, "WSLg", size);
        return;
    }
    env = getenv("DISPLAY");
    if (env && *env) {
        str_cpy(buf, "WSLg", size);
        return;
    }
    str_cpy(buf, "unknown", size);
}

static void
fetch_wsl_term(char *buf, size_t size)
{
    if (getenv("WT_SESSION") || getenv("WT_PROFILE_ID"))
        str_cpy(buf, "Windows Terminal", size);
    else
        buf[0] = '\0';
}

static int
get_comm(pid_t pid, char *buf, size_t size)
{
    char path[64];
    make_path(pid, "comm", path, sizeof(path));
    if (!path[0]) return -1;

    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;

    ssize_t r = read(fd, buf, size - 1);
    close(fd);

    if (r <= 0) return -1;
    buf[r] = '\0';

    size_t len = 0;
    while (buf[len]) len++;
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';

    return 0;
}

static void
fetch_wm(char *buf, size_t size)
{
    if (wm_ok) {
        str_cpy(buf, wm_buf, size);
        return;
    }

    DIR *dir = opendir("/proc");
    if (!dir) goto fail;

    pid_t candidates[MAX_SCAN];
    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) && count < MAX_SCAN) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;
        pid_t pid = (pid_t)fast_atoi(entry->d_name);
        if (pid >= MIN_WM_PID && pid <= MAX_WM_PID)
            candidates[count++] = pid;
    }
    closedir(dir);

    char comm[MAX_NAME];
    int checked = 0;
    for (int i = 0; i < count && checked < WM_MAX_CHK; i++, checked++) {
        if (get_comm(candidates[i], comm, sizeof(comm))) continue;
        if (!*comm) continue;

        if (match_list(comm, wms, ARRLEN(wms))) {
            for (size_t j = 0; j < ARRLEN(wms); ++j) {
                if (str_eq(comm, wms[j].id) || str_pfx(comm, wms[j].id, wms[j].len)) {
                    str_cpy(buf, wms[j].id, size);
                    str_cpy(wm_buf, buf, sizeof(wm_buf));
                    wm_ok = 1;
                    return;
                }
            }
        }
    }

fail:
    str_cpy(buf, "unknown", size);
    str_cpy(wm_buf, "unknown", sizeof(wm_buf));
    wm_ok = 1;
}

static int
parse_os_field(const char *key, size_t klen, char *line, char *out, size_t size)
{
    if (strncmp(line, key, klen)) return -1;
    char *start = strchr(line, '"');
    char *end = strrchr(line, '"');

    if (start && end && start < end) {
        *end = '\0';
        size_t rem = strnlen(start + 1, size);
        str_cpy(out, start + 1, MIN(rem + 1, size));
        return 0;
    }

    char *ptr = line + klen;
    size_t len = strnlen(ptr, MAX_LINE - klen);
    if (len > 0 && ptr[len - 1] == '\n') ptr[len - 1] = '\0';
    str_cpy(out, ptr, size);
    return 0;
}

static void
fetch_os(char *buf, size_t size)
{
    if (os_ok) {
        str_cpy(buf, os_buf, size);
        return;
    }

    FILE *f = fopen("/etc/os-release", "r");
    if (!f) {
        str_cpy(buf, "Linux", size);
        goto done;
    }

    char line[MAX_LINE];
    buf[0] = '\0';

    while (fgets(line, sizeof(line), f)) {
        if (!parse_os_field("PRETTY_NAME=", 12, line, buf, size)) break;
        if (!*buf) parse_os_field("NAME=", 5, line, buf, size);
    }

    if (!*buf) str_cpy(buf, "Linux", size);
    fclose(f);

done:
    str_cpy(os_buf, buf, sizeof(os_buf));
    os_ok = 1;
}

static int
get_tty(char *buf, size_t size, char *wm, size_t wm_size)
{
    ssize_t len = readlink("/proc/self/fd/0", buf, size - 1);
    if (len <= 0) return 0;
    buf[len] = '\0';

    if (strncmp(buf, "/dev/tty", 8) == 0 && buf[8] >= '0' && buf[8] <= '9') {
        str_cpy(wm, "none", wm_size);
        return 1;
    }
    return 0;
}

static void
print_sep(size_t len)
{
    while (len--) putchar('-');
    putchar('\n');
}

static void
print_version(void)
{
    printf("zeptofetch %s\n", VERSION);
    printf("Copyright (C) %s Gurov\n", COPYRIGHT);
    printf("Licensed under GPL-3.0\n\n");
    printf("BUILD: %s %s UTC\n", __DATE__, __TIME__);
    printf("CONFIG: CACHE=%d CHAIN=%d PID=%d MAX_CHK=%d\n",
           MAX_CACHE, MAX_CHAIN, MAX_PID, WM_MAX_CHK);
}

static void
sanitize(char *dst, size_t size, const char *src)
{
    if (!dst || !src || !size) return;
    size_t i = 0;
    while (i < size - 1 && src[i] && src[i] != ' ') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void
display(const char *user, const char *host, const char *os, const char *kern,
        const char *shell, const char *wm, const char *term)
{
    char krel[64], temp[256];
    sanitize(krel, sizeof(krel), kern);

    int n = snprintf(temp, sizeof(temp), "%s@%s", user, host);
    size_t len = (n > 0 && (size_t)n < sizeof(temp)) ? (size_t)n : 0;

    printf("%s    ___ %s     %s%s@%s%s\n", C1, CR, C1, user, host, CR);
    printf("%s   (%s.· %s|%s     ", C1, C2, C1, CR);
    print_sep(len);
    printf("%s   (%s<>%s %s|%s     %sOS:%s %s\n", C1, C3, CR, C1, CR, C3, CR, os);
    printf("%s  / %s__  %s\\%s    %sKernel:%s %s\n", C1, C2, C1, CR, C3, CR, krel);
    printf("%s ( %s/  \\ %s/|%s   %sShell:%s %s\n", C1, C2, C1, CR, C3, CR, shell);
    printf("%s_%s/\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n", C3, C1, C2, C1, C3, C1, CR, C3, CR, wm);
    printf("%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n", C1, C3, C1, C3, CR, C3, CR, term);
}

static int
set_limits(void)
{
    struct rlimit rlim;
    rlim.rlim_cur = rlim.rlim_max = 50 * 1024 * 1024;
    if (setrlimit(RLIMIT_AS, &rlim)) return -1;

    rlim.rlim_cur = rlim.rlim_max = 5;
    if (setrlimit(RLIMIT_CPU, &rlim)) return -1;

    rlim.rlim_cur = rlim.rlim_max = 128;
    if (setrlimit(RLIMIT_NOFILE, &rlim)) return -1;

    return 0;
}

static int
can_read_proc(void)
{
    return access("/proc/self/stat", R_OK) == 0;
}

static void
cleanup(void)
{
    if (cache) {
        secure_zero(cache, cache_len * sizeof(proc_t));
        munmap(cache, MAX_CACHE * sizeof(proc_t));
    }
}

int
main(int argc, char **argv)
{
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);

    if (geteuid() != getuid() || getegid() != getgid())
        if (setuid(getuid()) || setgid(getgid())) return 1;

    if (set_limits()) fprintf(stderr, "Warn: limits failed\n");

    if (!can_read_proc()) {
        fprintf(stderr, "Error: cannot read /proc (hidepid enabled?)\n");
        return 1;
    }

    cache = mmap(NULL, MAX_CACHE * sizeof(proc_t),
                 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (cache == MAP_FAILED) return 1;

    atexit(cleanup);

    if (argc > 1 && (str_eq(argv[1], "--version") || str_eq(argv[1], "-v"))) {
        print_version();
        return 0;
    }

    char user[MAX_NAME], host[MAX_NAME];
    char shell[MAX_NAME], wm[MAX_NAME], term[MAX_NAME];
    char os[MAX_LINE];
    struct utsname un;

    if (uname(&un)) str_cpy(un.release, "unknown", sizeof(un.release));

    fetch_user(user, sizeof(user));
    fetch_host(host, sizeof(host));

    proc_t chain[MAX_CHAIN];
    cache_len = 0;
    size_t count = build_chain(getpid(), chain, MAX_CHAIN);

    fetch_shell(chain, count, shell, sizeof(shell));
    fetch_os(os, sizeof(os));

    if (get_tty(term, sizeof(term), wm, sizeof(wm))) {
    } else if (detect_wsl()) {
        fetch_wsl_term(term, sizeof(term));
        if (!*term) fetch_term(chain, count, term, sizeof(term));
        fetch_wsl_wm(wm, sizeof(wm));
        if (str_eq(wm, "unknown")) fetch_wm(wm, sizeof(wm));
    } else {
        fetch_term(chain, count, term, sizeof(term));
        fetch_wm(wm, sizeof(wm));
    }

    display(user, host, os, un.release, shell, wm, term);
    return 0;
}
