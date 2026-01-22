#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
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
#include "config.h"

#define VER         "v1.14-rc1"
#define CPR         "2026"

#ifndef PATH_MAX
#define PATH_MAX    4096
#endif

#define CSZ         256
#define MCHN        64
#define MLN         64
#define MNM         128
#define MSM         64
#define PIDMX       4194304
#define WMTO        1
#define WMPMIN      300
#define WMPMAX      100000
#define SCNMX       30000
#define PBUF        2048

#define F_EXE       1
#define F_PP        2
#define F_ST        4

#define ALEN(a)     (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b)   ((a) < (b) ? (a) : (b))

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(CSZ <= PIDMX, "cache > pid space");
#else
typedef char sz_chk[(CSZ <= PIDMX) ? 1 : -1];
#endif

typedef struct {
    pid_t p, pp;
    char exe[PATH_MAX];
    unsigned long long t;
    uint8_t f;
} proc_t;

typedef struct {
    const char *s;
    size_t l;
} lst_t;

static const lst_t shs[] = {
    {"bash", 4}, {"zsh", 3}, {"fish", 4}, {"dash", 4},
    {"sh", 2}, {"ksh", 3}, {"tcsh", 4}, {"csh", 3},
    {"elvish", 6}, {"nushell", 7}, {"xonsh", 5}, {"ion", 3},
    {"oil", 3}, {"murex", 5}, {"powershell", 10}, {"pwsh", 4},
    {"rc", 2}, {"es", 2}, {"yash", 4}, {"mksh", 4},
    {"oksh", 4}, {"pdksh", 5},
};

static const lst_t trms[] = {
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

static const lst_t wms[] = {
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

static proc_t *che = NULL;
static size_t che_n = 0;
static char wmb[MSM] = {0};
static int wmk = 0;
static char osb[MNM] = {0};
static int osk = 0;
static char hsb[MSM] = {0};
static int hsk = 0;
static int wslk = -1;
static volatile sig_atomic_t stop = 0;

static void
scpy(char *d, const char *s, size_t z)
{
    if (!d || !s || z == 0) return;
    size_t i = 0;
    while (i < z - 1 && s[i]) {
        d[i] = (s[i] >= 32 && s[i] <= 126) ? s[i] : '_';
        i++;
    }
    d[i] = '\0';
}

static int
vpid(pid_t p)
{
    return p > 0 && p <= PIDMX;
}

static int
ppath(pid_t p, const char *f, char *b, size_t z)
{
    int n = snprintf(b, z, "/proc/%d/%s", p, f);
    return (n < 0 || (size_t)n >= z) ? -1 : 0;
}

static int
gst(pid_t p, unsigned long long *t)
{
    char pt[64], b[PBUF];
    if (ppath(p, "stat", pt, sizeof(pt))) return -1;
    int fd = open(pt, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t r = read(fd, b, sizeof(b) - 1);
    close(fd);
    if (r <= 0) return -1;
    b[r] = '\0';
    char *x = strrchr(b, ')');
    if (!x) return -1;
    int c = 0;
    while (*x && c < 19) {
        if (*x == ' ') c++;
        x++;
    }
    if (c != 19 || sscanf(x, "%llu", t) != 1) return -1;
    return 0;
}

static proc_t *
cget(pid_t p)
{
    for (size_t i = 0; i < che_n; ++i)
        if (che[i].p == p) return &che[i];
    return NULL;
}

static proc_t *
cadd(pid_t p)
{
    if (che_n >= CSZ) return NULL;
    proc_t *e = &che[che_n++];
    e->p = p; e->pp = -1; e->exe[0] = 0; e->t = 0; e->f = 0;
    return e;
}

static int
gpp(pid_t p, pid_t *o)
{
    char pt[64], b[PBUF];
    if (ppath(p, "stat", pt, sizeof(pt))) return -1;
    int fd = open(pt, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t r = read(fd, b, sizeof(b) - 1);
    close(fd);
    if (r <= 0) return -1;
    b[r] = '\0';
    char *x = strchr(b, ')');
    if (!x || x[1] != ' ' || !x[2]) return -1;
    int pp;
    if (sscanf(x + 2, "%*c %d", &pp) != 1 || !vpid(pp)) {
        *o = -1; return -1;
    }
    *o = pp;
    return 0;
}

static int
gexe(pid_t p, char *b, size_t z)
{
    char pt[PATH_MAX], t[PATH_MAX];
    if (ppath(p, "exe", pt, sizeof(pt))) return -1;
    ssize_t l = readlink(pt, t, sizeof(t) - 1);
    if (l <= 0) return -1;
    t[l] = '\0';
    char *r = realpath(t, NULL);
    if (r) {
        if (*r == '/' && (!strncmp(r, "/usr", 4) || !strncmp(r, "/bin", 4) ||
            !strncmp(r, "/sbin", 5) || !strncmp(r, "/opt", 4) ||
            !strncmp(r, "/home", 5))) {
            scpy(b, r, z); free(r); return 0;
        }
        free(r); return -1;
    }
    scpy(b, t, z);
    return 0;
}

static int
gproc(pid_t p, proc_t *pr)
{
    if (!vpid(p)) return -1;
    proc_t *c = cget(p);
    if (c) { if (pr != c) *pr = *c; return 0; }
    proc_t *e = cadd(p);
    if (!e) return -1;
    if (!gst(p, &e->t)) e->f |= F_ST;
    if (!gpp(p, &e->pp)) e->f |= F_PP;
    if (!gexe(p, e->exe, sizeof(e->exe))) e->f |= F_EXE;
    *pr = *e;
    return 0;
}

static size_t
bchn(pid_t s, proc_t *l, size_t m)
{
    size_t i = 0;
    pid_t c = s;
    while (vpid(c) && i < m) {
        if (gproc(c, &l[i])) break;
        if ((l[i].f & F_PP) && l[i].pp != c && vpid(l[i].pp))
            c = l[i].pp;
        else break;
        i++;
    }
    return i;
}

static int
seq(const char *a, const char *b)
{
    return a && b && !strcmp(a, b);
}

static int
spfx(const char *s, const char *p, size_t l)
{
    return s && p && !strncmp(s, p, l);
}

static int
mlst(const char *n, const lst_t *l, size_t c)
{
    if (!n || !*n || !l || !c) return 0;
    for (size_t i = 0; i < c; ++i)
        if (n[0] == l[i].s[0] && (seq(n, l[i].s) || spfx(n, l[i].s, l[i].l)))
            return 1;
    return 0;
}

static int
issh(const char *n)
{
    return mlst(n, shs, ALEN(shs));
}

static void
bnm(const char *p, char *o, size_t z)
{
    if (!p || !o || !z) return;
    const char *b = strrchr(p, '/');
    scpy(o, b ? b + 1 : p, z);
}

static void
gusr(char *b, size_t z)
{
    struct passwd *pw = getpwuid(getuid());
    scpy(b, (pw && pw->pw_name) ? pw->pw_name : "user", z);
}

static void
ghst(char *b, size_t z)
{
    if (hsk) { scpy(b, hsb, z); return; }
    if (gethostname(b, z)) scpy(b, "localhost", z);
    else b[z - 1] = 0;
    scpy(hsb, b, sizeof(hsb)); hsk = 1;
}

static void
gsh(proc_t *c, size_t n, char *b, size_t z)
{
    char *e = getenv("SHELL");
    if (e && *e) { bnm(e, b, z); return; }
    char tmp[MNM];
    for (size_t i = 0; i < n; ++i) {
        if (!(c[i].f & F_EXE)) continue;
        bnm(c[i].exe, tmp, sizeof(tmp));
        if (issh(tmp)) { scpy(b, tmp, z); return; }
    }
    scpy(b, "unknown", z);
}

static void
gtm(proc_t *c, size_t n, char *b, size_t z)
{
    char *e = getenv("TERM_PROGRAM");
    if (e && *e) { scpy(b, e, z); return; }
    e = getenv("TERMINAL");
    if (e && *e) { bnm(e, b, z); return; }
    char tmp[MNM];
    for (size_t i = 1; i < n; ++i) {
        if (!(c[i].f & F_EXE)) continue;
        bnm(c[i].exe, tmp, sizeof(tmp));
        if (issh(tmp)) continue;
        if (mlst(tmp, trms, ALEN(trms))) {
            for (size_t j = 0; j < ALEN(trms); ++j)
                if (seq(tmp, trms[j].s) || spfx(tmp, trms[j].s, trms[j].l)) {
                    scpy(b, trms[j].s, z); return;
                }
        }
        if (*tmp) { scpy(b, tmp, z); return; }
    }
    scpy(b, "unknown", z);
}

static int
dwsl(void)
{
    if (wslk != -1) return wslk;
    wslk = 0;
    char b[512];
    int fd = open("/proc/sys/kernel/osrelease", O_RDONLY);
    if (fd >= 0) {
        ssize_t r = read(fd, b, sizeof(b) - 1);
        close(fd);
        if (r > 0) { b[r] = 0; if (strstr(b, "WSL") || strstr(b, "microsoft")) wslk = 1; }
    }
    if (wslk) return 1;
    fd = open("/proc/version", O_RDONLY);
    if (fd >= 0) {
        ssize_t r = read(fd, b, sizeof(b) - 1);
        close(fd);
        if (r > 0) { b[r] = 0; if (strstr(b, "Microsoft") || strstr(b, "WSL")) wslk = 1; }
    }
    return wslk;
}

static void
gwslwm(char *b, size_t z)
{
    char *e = getenv("WAYLAND_DISPLAY");
    if (e && *e) { scpy(b, "WSLg", z); return; }
    e = getenv("DISPLAY");
    if (e && *e) { scpy(b, "WSLg", z); return; }
    scpy(b, "unknown", z);
}

static void
gwsltm(char *b, size_t z)
{
    if (getenv("WT_SESSION") || getenv("WT_PROFILE_ID"))
        scpy(b, "Windows Terminal", z);
    else b[0] = 0;
}

static int
gcomm(const char *pid, char *o, size_t z)
{
    char pt[PATH_MAX];
    if (snprintf(pt, sizeof(pt), "/proc/%s/comm", pid) < 0) return -1;
    int fd = open(pt, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t r = read(fd, o, z - 1);
    close(fd);
    if (r <= 0) return -1;
    o[r] = 0;
    size_t l = strnlen(o, z);
    if (l > 0 && o[l - 1] == '\n') o[l - 1] = 0;
    return 0;
}

static int
iwm(const char *s)
{
    if (!s || !*s) return 0;
    char *e; errno = 0;
    unsigned long p = strtoul(s, &e, 10);
    return (errno == 0 && *e == 0 && e != s && p >= WMPMIN && p <= WMPMAX);
}

static void
salrm(int s)
{
    (void)s; stop = 1;
}

static int
dcmp(const struct dirent **a, const struct dirent **b)
{
    unsigned long na = strtoul((*a)->d_name, 0, 10);
    unsigned long nb = strtoul((*b)->d_name, 0, 10);
    return (nb < na) - (na < nb);
}

static int
dflt(const struct dirent *e)
{
    return e->d_name[0] >= '0' && e->d_name[0] <= '9' && iwm(e->d_name);
}

static void
gwm(char *b, size_t z)
{
    if (wmk) { scpy(b, wmb, z); return; }
    struct dirent **l = NULL;
    int n = scandir("/proc", &l, dflt, dcmp);
    if (n < 0) goto fail;
    struct sigaction sa = {0}, o;
    sa.sa_handler = salrm;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, &o)) {
        for (int i = 0; i < n; i++) free(l[i]);
        free(l); goto fail;
    }
    stop = 0; alarm(WMTO);
    char cm[MSM];
    int lm = (n < SCNMX) ? n : SCNMX;
    for (int i = 0; i < lm && !stop; i++) {
        if (gcomm(l[i]->d_name, cm, sizeof(cm))) continue;
        if (!*cm) continue;
        if (mlst(cm, wms, ALEN(wms))) {
            for (size_t j = 0; j < ALEN(wms); ++j)
                if (seq(cm, wms[j].s) || spfx(cm, wms[j].s, wms[j].l)) {
                    scpy(b, wms[j].s, z); scpy(wmb, b, sizeof(wmb)); wmk = 1;
                    goto end;
                }
        }
    }
    scpy(b, "unknown", z); scpy(wmb, "unknown", sizeof(wmb)); wmk = 1;
end:
    alarm(0); sigaction(SIGALRM, &o, NULL);
    for (int i = 0; i < n; i++) free(l[i]);
    free(l); return;
fail:
    scpy(b, "unknown", z); scpy(wmb, "unknown", sizeof(wmb)); wmk = 1;
}

static int
pos(const char *k, size_t kl, char *l, char *o, size_t z)
{
    if (strncmp(l, k, kl)) return -1;
    char *s = strchr(l, '"'), *e = strrchr(l, '"');
    if (s && e && s < e) {
        *e = 0; size_t r = strnlen(s + 1, z);
        scpy(o, s + 1, MIN(r + 1, z)); return 0;
    }
    char *p = l + kl;
    size_t x = strnlen(p, MLN - kl);
    if (x > 0 && p[x - 1] == '\n') p[x - 1] = 0;
    scpy(o, p, z); return 0;
}

static void
gos(char *b, size_t z)
{
    if (osk) { scpy(b, osb, z); return; }
    FILE *f = fopen("/etc/os-release", "r");
    if (!f) { scpy(b, "Linux", z); goto d; }
    char l[MLN]; b[0] = 0;
    while (fgets(l, sizeof(l), f)) {
        if (!pos("PRETTY_NAME=", 12, l, b, z)) break;
        if (!*b) pos("NAME=", 5, l, b, z);
    }
    if (!*b) scpy(b, "Linux", z);
    fclose(f);
d:  scpy(osb, b, sizeof(osb)); osk = 1;
}

static void
psep(size_t l)
{
    while (l--) putchar('-');
    putchar('\n');
}

static void
pver(void)
{
    printf("zeptofetch %s\n", VER);
    printf("Copyright (C) %s Gurov\n", CPR);
    printf("Licensed under GPL-3.0\n\n");
    printf("BUILD: %s %s UTC | ", __DATE__, __TIME__);
#if defined(__clang__)
    printf("Clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
#ifdef __GNUC_PATCHLEVEL__
    printf("GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
    printf("GCC %d.%d\n", __GNUC__, __GNUC_MINOR__);
#endif
#else
    printf("Unknown\n");
#endif
    printf("CFG: CHE=%d CHN=%d PATH=%d PID=%d TO=%ds\n", CSZ, MCHN, PATH_MAX, PIDMX, WMTO);
}

static void
san(char *d, size_t z, const char *s)
{
    if (!d || !s || !z) return;
    size_t i = 0;
    while (i < z - 1 && s[i] && s[i] != ' ') { d[i] = s[i]; i++; }
    d[i] = 0;
}

static void
draw(const char *u, const char *h, const char *os, const char *k,
     const char *sh, const char *wm, const char *tm)
{
    char kr[64], t[256];
    san(kr, sizeof(kr), k);
    int n = snprintf(t, sizeof(t), "%s@%s", u, h);
    size_t l = (n > 0 && (size_t)n < sizeof(t)) ? (size_t)n : 0;
    printf("%s    ___ %s     %s%s@%s%s\n", C1, CR, C1, u, h, CR);
    printf("%s   (%s.· %s|%s     ", C1, C2, C1, CR); psep(l);
    printf("%s   (%s<>%s %s|%s     %sOS:%s %s\n", C1, C3, CR, C1, CR, C3, CR, os);
    printf("%s  / %s__  %s\\%s    %sKernel:%s %s\n", C1, C2, C1, CR, C3, CR, kr);
    printf("%s ( %s/  \\ %s/|%s   %sShell:%s %s\n", C1, C2, C1, CR, C3, CR, sh);
    printf("%s_%s/\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n", C3, C1, C2, C1, C3, C1, CR, C3, CR, wm);
    printf("%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n", C1, C3, C1, C3, CR, C3, CR, tm);
}

static int
rlim(void)
{
    struct rlimit l;
    l.rlim_cur = l.rlim_max = 50 * 1024 * 1024;
    if (setrlimit(RLIMIT_AS, &l)) return -1;
    l.rlim_cur = l.rlim_max = 5;
    if (setrlimit(RLIMIT_CPU, &l)) return -1;
    l.rlim_cur = l.rlim_max = 128;
    if (setrlimit(RLIMIT_NOFILE, &l)) return -1;
    return 0;
}

static void
cln(void)
{
    if (che) munmap(che, CSZ * sizeof(proc_t));
}

int
main(int c, char **v)
{
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
    if (geteuid() != getuid() || getegid() != getgid())
        if (setuid(getuid()) || setgid(getgid())) return 1;
    if (rlim()) fprintf(stderr, "Warn: limits failed\n");
    che = mmap(NULL, CSZ * sizeof(proc_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (che == MAP_FAILED) { fprintf(stderr, "Err: alloc fail\n"); return 1; }
    atexit(cln);
    setlocale(LC_ALL, "");
    if (c > 1 && (seq(v[1], "--version") || seq(v[1], "-v"))) { pver(); return 0; }
    char u[MSM], h[MSM], sh[MSM], wm[MSM], tm[MSM], os[MNM];
    struct utsname un;
    if (uname(&un)) scpy(un.release, "unknown", sizeof(un.release));
    gusr(u, sizeof(u)); ghst(h, sizeof(h));
    proc_t ch[MCHN];
    che_n = 0;
    size_t n = bchn(getpid(), ch, MCHN);
    gsh(ch, n, sh, sizeof(sh)); gos(os, sizeof(os));
    if (dwsl()) {
        gwsltm(tm, sizeof(tm));
        if (!*tm) gtm(ch, n, tm, sizeof(tm));
        gwslwm(wm, sizeof(wm));
        if (seq(wm, "unknown")) gwm(wm, sizeof(wm));
    } else {
        gtm(ch, n, tm, sizeof(tm)); gwm(wm, sizeof(wm));
    }
    draw(u, h, os, un.release, sh, wm, tm);
    return 0;
}
