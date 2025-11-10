#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include "config.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define VERSION "v1.9"
#define CACHE_SZ 1024
#define MAX_CHAIN 1000
#define MAX_LINE 64
#define MAX_NAME 128
#define MAX_SMALL 64
#define PID_MAX 4194304
#define WM_TIMEOUT 1
#define MIN_WM_PID 300
#define MAX_WM_PID 100000
#define MAX_SCAN 50000
#define ARRLEN(a) (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(CACHE_SZ <= PID_MAX, "cache bigger than pid space");
#else
typedef char static_assert_cache_size[(CACHE_SZ <= PID_MAX) ? 1 : -1];
#endif

typedef struct {
	pid_t pid;
	pid_t ppid;
	char exe[PATH_MAX];
	time_t ct;
	uint8_t flg;
} proc_t;

#define F_EXE 1
#define F_PPID 2
#define F_VALID 4

static proc_t cache[CACHE_SZ];
static size_t cache_n = 0;
static char wm_cache[MAX_SMALL] = {0};
static int wm_ok = 0;
static char os_cache[MAX_NAME] = {0};
static int os_ok = 0;
static char host_cache[MAX_SMALL] = {0};
static int host_ok = 0;

static const struct {
	const char *nm;
	size_t ln;
} shells[] = {
	{"bash", 4}, {"zsh", 3}, {"fish", 4}, {"dash", 4},
	{"sh", 2}, {"ksh", 3}, {"tcsh", 4}, {"csh", 3},
	{"elvish", 6}, {"nushell", 7}, {"xonsh", 5}, {"ion", 3},
	{"oil", 3}, {"murex", 5}, {"powershell", 10}, {"pwsh", 4},
	{"rc", 2}, {"es", 2}, {"yash", 4}, {"mksh", 4},
	{"oksh", 4}, {"pdksh", 5},
};

static const struct {
	const char *nm;
	size_t ln;
} terms[] = {
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

static const struct {
	const char *nm;
	size_t ln;
} wms[] = {
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

static inline void
scpy(char *d, const char *s, size_t z)
{
	if (!d || !s || z == 0)
		return;
	size_t l = strnlen(s, z);
	if (l < z) {
		__builtin_memcpy(d, s, l + 1);
	} else {
		__builtin_memcpy(d, s, z - 1);
		d[z - 1] = '\0';
	}
}

static inline int
valid_pid(pid_t p)
{
	return p > 0 && p <= PID_MAX;
}

static int
ppath(pid_t p, const char *f, char *b, size_t z)
{
	int n = snprintf(b, z, "/proc/%d/%s", p, f);
	return (n < 0 || (size_t)n >= z) ? -1 : 0;
}

static inline int
pexists(pid_t p)
{
	char pt[64];
	struct stat st;
	if (ppath(p, "", pt, sizeof(pt)) != 0)
		return 0;
	return stat(pt, &st) == 0;
}

static proc_t *
cget(pid_t p)
{
	for (size_t i = 0; i < cache_n; ++i) {
		if (cache[i].pid == p) {
			if (!pexists(p))
				return NULL;
			return &cache[i];
		}
	}
	return NULL;
}

static proc_t *
cadd(pid_t p)
{
	if (cache_n >= CACHE_SZ)
		return NULL;
	size_t i = cache_n++;
	cache[i].pid = p;
	cache[i].ppid = -1;
	cache[i].exe[0] = '\0';
	cache[i].flg = 0;
	cache[i].ct = time(NULL);
	return &cache[i];
}

static int
rppid(pid_t p, pid_t *o)
{
	char pt[64];
	if (ppath(p, "stat", pt, sizeof(pt)) != 0)
		return -1;
	FILE *f = fopen(pt, "r");
	if (!f)
		return -1;
	int ok = fscanf(f, "%*d %*s %*c %d", o);
	fclose(f);
	return (ok == 1) ? 0 : -1;
}

static int
rexe(pid_t p, char *b, size_t z)
{
	char pt[PATH_MAX], tmp[PATH_MAX];
	int ret = -1;

	if (ppath(p, "exe", pt, sizeof(pt)) != 0)
		return -1;
	ssize_t l = readlink(pt, tmp, sizeof(tmp) - 1);
	if (l <= 0)
		return -1;
	tmp[l] = '\0';

	char *res = realpath(tmp, NULL);
	if (res) {
		struct stat st;
		if (stat(res, &st) == 0) {
			if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)) {
				if (strncmp(res, "/usr", 4) == 0 ||
				    strncmp(res, "/bin", 4) == 0 ||
				    strncmp(res, "/opt", 4) == 0 ||
				    strncmp(res, "/home", 5) == 0) {
					scpy(b, res, z);
					ret = 0;
				}
			}
		}
		free(res);
		if (ret == 0)
			return 0;
		return -1;
	}
	scpy(b, tmp, z);
	return 0;
}

static int
gproc(pid_t p, proc_t *pr)
{
	if (!valid_pid(p))
		return -1;
	if (!pexists(p))
		return -1;

	proc_t *c = cget(p);
	if (c) {
		if (pr != c)
			*pr = *c;
		return 0;
	}

	proc_t *e = cadd(p);
	if (!e)
		return -1;

	e->pid = p;
	if (rppid(p, &e->ppid) == 0) {
		e->flg |= F_PPID;
	} else {
		e->ppid = -1;
	}

	if (rexe(p, e->exe, sizeof(e->exe)) == 0) {
		e->flg |= F_EXE;
	} else {
		e->exe[0] = '\0';
	}

	*pr = *e;
	return 0;
}

static size_t
bchain(pid_t st, proc_t *o, size_t mx)
{
	size_t i = 0;
	pid_t c = st;

	while (valid_pid(c) && i < mx && i < MAX_CHAIN) {
		if (gproc(c, &o[i]) != 0)
			break;
		if (o[i].flg & F_PPID && o[i].ppid != c && o[i].ppid > 0) {
			c = o[i].ppid;
		} else {
			break;
		}
		i++;
	}
	return i;
}

static inline int
seq(const char *a, const char *b)
{
	return strcmp(a, b) == 0;
}

static inline int
spfx(const char *s, const char *p, size_t pl)
{
	return strncmp(s, p, pl) == 0;
}

static int
issh(const char *n)
{
	if (!n || !*n)
		return 0;
	char fc = n[0];
	for (size_t i = 0; i < ARRLEN(shells); ++i) {
		if (fc == shells[i].nm[0] && seq(n, shells[i].nm))
			return 1;
	}
	return 0;
}

static void
bname(const char *pt, char *o, size_t z)
{
	const char *b = strrchr(pt, '/');
	scpy(o, b ? b + 1 : pt, z);
}

static void
guser(char *b, size_t z)
{
	struct passwd *pw = getpwuid(getuid());
	scpy(b, (pw && pw->pw_name) ? pw->pw_name : "user", z);
}

static void
ghost(char *b, size_t z)
{
	if (host_ok) {
		scpy(b, host_cache, z);
		return;
	}
	if (gethostname(b, z) != 0) {
		scpy(b, "localhost", z);
	} else {
		b[z - 1] = '\0';
	}
	scpy(host_cache, b, sizeof(host_cache));
	host_ok = 1;
}

static void
gsh(proc_t *ch, size_t n, char *b, size_t z)
{
	char bs[MAX_NAME];
	for (size_t i = 0; i < n; ++i) {
		if (!(ch[i].flg & F_EXE))
			continue;
		bname(ch[i].exe, bs, sizeof(bs));
		if (issh(bs)) {
			scpy(b, bs, z);
			return;
		}
	}
	scpy(b, "unknown", z);
}

static void
gterm(proc_t *ch, size_t n, char *b, size_t z)
{
	char bs[MAX_NAME];
	for (size_t i = 1; i < n; ++i) {
		if (!(ch[i].flg & F_EXE))
			continue;
		bname(ch[i].exe, bs, sizeof(bs));
		if (issh(bs))
			continue;

		char fc = bs[0];
		for (size_t j = 0; j < ARRLEN(terms); ++j) {
			if (fc != terms[j].nm[0])
				continue;
			if (seq(bs, terms[j].nm) ||
			    spfx(bs, terms[j].nm, terms[j].ln)) {
				scpy(b, terms[j].nm, z);
				return;
			}
		}

		if (bs[0] != '\0') {
			scpy(b, bs, z);
			return;
		}
	}
	scpy(b, "unknown", z);
}

static int
rcomm(const char *ps, char *cm, size_t z)
{
	char pt[PATH_MAX];
	int n = snprintf(pt, sizeof(pt), "/proc/%s/comm", ps);
	if (n < 0 || (size_t)n >= sizeof(pt))
		return -1;

	FILE *f = fopen(pt, "r");
	if (!f)
		return -1;

	if (!fgets(cm, z, f)) {
		fclose(f);
		return -1;
	}
	fclose(f);

	size_t l = strnlen(cm, z);
	if (l > 0 && cm[l - 1] == '\n')
		cm[l - 1] = '\0';
	return 0;
}

static int
likely_wm(const char *nm)
{
	char *ep;
	errno = 0;
	unsigned long p = strtoul(nm, &ep, 10);
	if (errno != 0 || *ep != '\0' || ep == nm)
		return 0;
	return (p >= MIN_WM_PID && p <= MAX_WM_PID);
}

static void
gwm(char *b, size_t z)
{
	if (wm_ok) {
		scpy(b, wm_cache, z);
		return;
	}

	DIR *pr = opendir("/proc");
	if (!pr) {
		scpy(b, "unknown", z);
		scpy(wm_cache, "unknown", sizeof(wm_cache));
		wm_ok = 1;
		return;
	}

	time_t st = time(NULL);
	struct dirent *e;
	char cm[MAX_SMALL];
	size_t dc = 0;

	while ((e = readdir(pr))) {
		if (++dc > MAX_SCAN)
			break;
		if (time(NULL) - st >= WM_TIMEOUT)
			break;
		if (e->d_name[0] < '0' || e->d_name[0] > '9')
			continue;
		if (!likely_wm(e->d_name))
			continue;
		if (rcomm(e->d_name, cm, sizeof(cm)) != 0)
			continue;
		if (cm[0] == '\0')
			continue;

		char fc = cm[0];
		for (size_t i = 0; i < ARRLEN(wms); ++i) {
			if (fc != wms[i].nm[0])
				continue;
			if (seq(cm, wms[i].nm) || spfx(cm, wms[i].nm, wms[i].ln)) {
				scpy(b, wms[i].nm, z);
				scpy(wm_cache, b, sizeof(wm_cache));
				wm_ok = 1;
				closedir(pr);
				return;
			}
		}
	}

	closedir(pr);
	scpy(b, "unknown", z);
	scpy(wm_cache, "unknown", sizeof(wm_cache));
	wm_ok = 1;
}

static int
parse_os(const char *px, size_t pl, char *ln, char *o, size_t z)
{
	if (strncmp(ln, px, pl) != 0)
		return -1;

	char *st = strchr(ln, '"');
	char *en = strrchr(ln, '"');
	if (st && en && st < en) {
		*en = '\0';
		size_t rm = strnlen(st + 1, z);
		scpy(o, st + 1, MIN(rm + 1, z));
		return 0;
	}

	char *p = ln + pl;
	size_t l = strnlen(p, MAX_LINE - pl);
	if (l > 0 && p[l - 1] == '\n')
		p[l - 1] = '\0';
	scpy(o, p, z);
	return 0;
}

static void
gos(char *b, size_t z)
{
	if (os_ok) {
		scpy(b, os_cache, z);
		return;
	}

	FILE *f = fopen("/etc/os-release", "r");
	if (!f) {
		scpy(b, "Linux", z);
		scpy(os_cache, "Linux", sizeof(os_cache));
		os_ok = 1;
		return;
	}

	char ln[MAX_LINE];
	char pr[MAX_LINE] = {0};
	char nm[MAX_LINE] = {0};
	int fp = 0;
	int fn = 0;

	while (fgets(ln, sizeof(ln), f)) {
		if (!fp && parse_os("PRETTY_NAME=", 12, ln, pr, sizeof(pr)) == 0) {
			fp = 1;
			break;
		}
		if (!fn && parse_os("NAME=", 5, ln, nm, sizeof(nm)) == 0) {
			fn = 1;
		}
	}

	if (fp) {
		scpy(b, pr, z);
		scpy(os_cache, pr, sizeof(os_cache));
	} else if (fn) {
		scpy(b, nm, z);
		scpy(os_cache, nm, sizeof(os_cache));
	} else {
		scpy(b, "Linux", z);
		scpy(os_cache, "Linux", sizeof(os_cache));
	}

	os_ok = 1;
	fclose(f);
}

static void
psep(size_t l)
{
	for (size_t i = 0; i < l; ++i)
		putchar('-');
	putchar('\n');
}

static void
pver(void)
{
	printf("zeptofetch %s\n", VERSION);
	printf("Copyright (C) 2025 Gurov\n");
	printf("Licensed under GPL-3.0\n");
	printf("\n");
	printf("BUILD: %s %s UTC | ", __DATE__, __TIME__);

#if defined(__clang__)
	printf("Clang %d.%d.%d\n", __clang_major__, __clang_minor__,
	    __clang_patchlevel__);
#elif defined(__GNUC__)
#ifdef __GNUC_PATCHLEVEL__
	printf("GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__,
	    __GNUC_PATCHLEVEL__);
#else
	printf("GCC %d.%d\n", __GNUC__, __GNUC_MINOR__);
#endif
#else
	printf("Unknown\n");
#endif

	printf("CONFIG: CACHE=%d CHAIN=%d PATH=%d PID=%d TIMEOUT=%ds\n",
	    CACHE_SZ, MAX_CHAIN, PATH_MAX, PID_MAX, WM_TIMEOUT);
}

static void
san_rel(char *d, size_t z, const char *s)
{
	size_t i = 0;
	while (i < z - 1 && s[i] != '\0' && s[i] != ' ') {
		d[i] = s[i];
		i++;
	}
	d[i] = '\0';
}

static void
disp(const char *u, const char *h, const char *os, const char *k,
    const char *sh, const char *wm, const char *tm)
{
	char rel[64];
	san_rel(rel, sizeof(rel), k);

	char tmp[256];
	int n = snprintf(tmp, sizeof(tmp), "%s@%s", u, h);
	size_t l = (n > 0 && (size_t)n < sizeof(tmp)) ? (size_t)n : 0;

	printf("%s    ___ %s     %s%s@%s%s\n",
	    COLOR_1, COLOR_RESET, COLOR_1, u, h, COLOR_RESET);
	printf("%s   (%s.· %s|%s     ", COLOR_1, COLOR_2, COLOR_1,
	    COLOR_RESET);
	psep(l);
	printf("%s   (%s<>%s %s|%s     %sOS:%s %s\n",
	    COLOR_1, COLOR_3, COLOR_RESET, COLOR_1, COLOR_RESET,
	    COLOR_3, COLOR_RESET, os);
	printf("%s  / %s__  %s\\%s    %sKernel:%s %s\n",
	    COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET,
	    rel);
	printf("%s ( %s/  \\ %s/|%s   %sShell:%s %s\n",
	    COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET,
	    sh);
	printf("%s_%s/\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n",
	    COLOR_3, COLOR_1, COLOR_2, COLOR_1, COLOR_3, COLOR_1,
	    COLOR_RESET, COLOR_3, COLOR_RESET, wm);
	printf("%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n",
	    COLOR_1, COLOR_3, COLOR_1, COLOR_3, COLOR_RESET,
	    COLOR_3, COLOR_RESET, tm);
}

int
main(int argc, char **argv)
{
	prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);

	if (geteuid() != getuid() || getegid() != getgid()) {
		if (setuid(getuid()) != 0 || setgid(getgid()) != 0)
			return 1;
	}

	setlocale(LC_ALL, "");

	if (argc > 1) {
		if (seq(argv[1], "--version") || seq(argv[1], "-v")) {
			pver();
			return 0;
		}
	}

	char u[MAX_SMALL];
	char h[MAX_SMALL];
	char sh[MAX_SMALL];
	char wm[MAX_SMALL];
	char tm[MAX_SMALL];
	char os[MAX_NAME];
	struct utsname inf;

	if (uname(&inf) != 0)
		scpy(inf.release, "unknown", sizeof(inf.release));

	guser(u, sizeof(u));
	ghost(h, sizeof(h));

	proc_t ch[CACHE_SZ];
	cache_n = 0;
	size_t n = bchain(getpid(), ch, CACHE_SZ);

	gsh(ch, n, sh, sizeof(sh));
	gterm(ch, n, tm, sizeof(tm));
	gwm(wm, sizeof(wm));
	gos(os, sizeof(os));

	disp(u, h, os, inf.release, sh, wm, tm);

	return 0;
}
