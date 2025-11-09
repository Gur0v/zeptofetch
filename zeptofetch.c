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

#define VERSION "v1.8"
#define CACHE_SIZE 1024
#define MAX_CHAIN 1000
#define MAX_LINE 64
#define MAX_NAME 128
#define MAX_SMALL 64
#define PID_MAX 4194304
#define WM_SCAN_TIMEOUT 1
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(CACHE_SIZE <= PID_MAX, "cache bigger than pid space");
#else
typedef char static_assert_cache_size[(CACHE_SIZE <= PID_MAX) ? 1 : -1];
#endif

typedef struct
{
	pid_t pid;
	pid_t ppid;
	char exe[PATH_MAX];
	time_t ct;
	uint8_t flags;
} proc_t;

#define PROC_HAS_EXE 1
#define PROC_HAS_PPID 2
#define PROC_VALIDATED 4

static proc_t cache[CACHE_SIZE];
static size_t cache_cnt = 0;
static char wm_cache[MAX_SMALL] = {0};
static int wm_cached = 0;
static char os_cache[MAX_NAME] = {0};
static int os_cached = 0;
static char host_cache[MAX_SMALL] = {0};
static int host_cached = 0;

static const struct {
	const char *name;
	size_t len;
} shells[] = {
	{"bash", 4}, {"zsh", 3}, {"fish", 4}, {"dash", 4},
	{"sh", 2}, {"ksh", 3}, {"tcsh", 4}, {"csh", 3},
	{"elvish", 6}, {"nushell", 7}, {"xonsh", 5}, {"ion", 3},
	{"oil", 3}, {"murex", 5}, {"powershell", 10}, {"pwsh", 4},
	{"rc", 2}, {"es", 2}, {"yash", 4}, {"mksh", 4},
	{"oksh", 4}, {"pdksh", 5},
};

static const struct {
	const char *name;
	size_t len;
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
	const char *name;
	size_t len;
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
str_copy(char *dst, const char *src, size_t sz)
{
	if (sz == 0)
		__builtin_trap();
	size_t len = strnlen(src, sz);
	if (len < sz) {
		__builtin_memcpy(dst, src, len + 1);
	} else {
		__builtin_memcpy(dst, src, sz - 1);
		dst[sz - 1] = '\0';
	}
}

static inline int
valid_pid(pid_t pid)
{
	return pid > 0 && pid <= PID_MAX;
}

static inline int
proc_exists(pid_t pid)
{
	char path[64];
	struct stat st;
	int n = snprintf(path, sizeof(path), "/proc/%d", pid);
	if (n < 0 || (size_t)n >= sizeof(path))
		return 0;
	return stat(path, &st) == 0;
}

static proc_t *
cache_get(pid_t pid)
{
	for (size_t i = 0; i < cache_cnt; ++i) {
		if (cache[i].pid == pid) {
			if (!proc_exists(pid))
				return NULL;
			return &cache[i];
		}
	}
	return NULL;
}

static proc_t *
cache_add(pid_t pid)
{
	if (cache_cnt >= CACHE_SIZE)
		return NULL;
	size_t idx = cache_cnt++;
	cache[idx].pid = pid;
	cache[idx].ppid = -1;
	cache[idx].exe[0] = '\0';
	cache[idx].flags = 0;
	cache[idx].ct = time(NULL);
	return &cache[idx];
}

static int
read_ppid(pid_t pid, pid_t *out)
{
	char path[64];
	int n = snprintf(path, sizeof(path), "/proc/%d/stat", pid);
	if (n < 0 || (size_t)n >= sizeof(path))
		return -1;

	FILE *f = fopen(path, "r");
	if (!f)
		return -1;

	int ok = fscanf(f, "%*d %*s %*c %d", out);
	fclose(f);
	return (ok == 1) ? 0 : -1;
}

static int
read_exe(pid_t pid, char *buf, size_t sz)
{
	char path[PATH_MAX];
	char tmp[PATH_MAX];
	int n = snprintf(path, sizeof(path), "/proc/%d/exe", pid);
	if (n < 0 || (size_t)n >= sizeof(path))
		return -1;

	ssize_t len = readlink(path, tmp, sizeof(tmp) - 1);
	if (len <= 0)
		return -1;
	tmp[len] = '\0';

	char *res = realpath(tmp, NULL);
	if (res) {
		struct stat st;
		if (stat(res, &st) != 0) {
			free(res);
			return -1;
		}

		if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode)) {
			free(res);
			return -1;
		}

		if (strncmp(res, "/usr", 4) == 0 ||
		    strncmp(res, "/bin", 4) == 0 ||
		    strncmp(res, "/opt", 4) == 0 ||
		    strncmp(res, "/home", 5) == 0) {
			str_copy(buf, res, sz);
			free(res);
			return 0;
		}
		free(res);
		return -1;
	}

	str_copy(buf, tmp, sz);
	return 0;
}

static int
get_proc(pid_t pid, proc_t *p)
{
	if (!valid_pid(pid))
		return -1;
	if (!proc_exists(pid))
		return -1;

	proc_t *cached = cache_get(pid);
	if (cached) {
		if (p != cached)
			*p = *cached;
		return 0;
	}

	proc_t *e = cache_add(pid);
	if (!e)
		return -1;

	e->pid = pid;
	if (read_ppid(pid, &e->ppid) == 0) {
		e->flags |= PROC_HAS_PPID;
	} else {
		e->ppid = -1;
	}

	if (read_exe(pid, e->exe, sizeof(e->exe)) == 0) {
		e->flags |= PROC_HAS_EXE;
	} else {
		e->exe[0] = '\0';
	}

	*p = *e;
	return 0;
}

static size_t
build_chain(pid_t start, proc_t *out, size_t max)
{
	size_t idx = 0;
	pid_t cur = start;

	while (valid_pid(cur) && idx < max && idx < MAX_CHAIN) {
		if (get_proc(cur, &out[idx]) != 0)
			break;

		if (out[idx].flags & PROC_HAS_PPID && out[idx].ppid != cur &&
		    out[idx].ppid > 0) {
			cur = out[idx].ppid;
		} else {
			break;
		}

		idx++;
	}

	return idx;
}

static inline int
str_eq(const char *a, const char *b)
{
	return strcmp(a, b) == 0;
}

static inline int
str_prefix(const char *s, const char *pre, size_t plen)
{
	return strncmp(s, pre, plen) == 0;
}

static int
is_shell(const char *n)
{
	if (!n)
		return 0;
	char fc = n[0];
	for (size_t i = 0; i < ARRAY_LEN(shells); ++i) {
		if (fc != shells[i].name[0])
			continue;
		if (str_eq(n, shells[i].name))
			return 1;
	}
	return 0;
}

static void
basename_of(const char *path, char *out, size_t sz)
{
	const char *b = strrchr(path, '/');
	str_copy(out, b ? b + 1 : path, sz);
}

static void
get_user(char *buf, size_t sz)
{
	struct passwd *pw = getpwuid(getuid());
	str_copy(buf, (pw && pw->pw_name) ? pw->pw_name : "user", sz);
}

static void
get_host(char *buf, size_t sz)
{
	if (host_cached) {
		str_copy(buf, host_cache, sz);
		return;
	}

	if (gethostname(buf, sz) != 0) {
		str_copy(buf, "localhost", sz);
	} else {
		buf[sz - 1] = '\0';
	}

	str_copy(host_cache, buf, sizeof(host_cache));
	host_cached = 1;
}

static void
get_shell(proc_t *chain, size_t cnt, char *buf, size_t sz)
{
	char base[MAX_NAME];
	for (size_t i = 0; i < cnt; ++i) {
		if (!(chain[i].flags & PROC_HAS_EXE))
			continue;
		basename_of(chain[i].exe, base, sizeof(base));
		if (is_shell(base)) {
			str_copy(buf, base, sz);
			return;
		}
	}
	str_copy(buf, "unknown", sz);
}

static void
get_term(proc_t *chain, size_t cnt, char *buf, size_t sz)
{
	char base[MAX_NAME];
	for (size_t i = 1; i < cnt; ++i) {
		if (!(chain[i].flags & PROC_HAS_EXE))
			continue;
		basename_of(chain[i].exe, base, sizeof(base));
		if (is_shell(base))
			continue;

		char fc = base[0];
		for (size_t j = 0; j < ARRAY_LEN(terms); ++j) {
			if (fc != terms[j].name[0])
				continue;
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

static int
read_comm(const char *pid_str, char *comm, size_t sz)
{
	char path[PATH_MAX];
	int n = snprintf(path, sizeof(path), "/proc/%s/comm", pid_str);
	if (n < 0 || (size_t)n >= sizeof(path))
		return -1;

	FILE *f = fopen(path, "r");
	if (!f)
		return -1;

	if (!fgets(comm, sz, f)) {
		fclose(f);
		return -1;
	}
	fclose(f);

	size_t len = strnlen(comm, sz);
	if (len > 0 && comm[len - 1] == '\n')
		comm[len - 1] = '\0';

	return 0;
}

static int
likely_wm_pid(const char *name)
{
	unsigned long pid = strtoul(name, NULL, 10);
	if (pid < 300)
		return 0;
	if (pid > 100000)
		return 0;
	return 1;
}

static void
get_wm(char *buf, size_t sz)
{
	if (wm_cached) {
		str_copy(buf, wm_cache, sz);
		return;
	}

	DIR *proc = opendir("/proc");
	if (!proc) {
		str_copy(buf, "unknown", sz);
		str_copy(wm_cache, "unknown", sizeof(wm_cache));
		wm_cached = 1;
		return;
	}

	time_t start = time(NULL);
	struct dirent *ent;
	char comm[MAX_SMALL];
	size_t dcnt = 0;

	while ((ent = readdir(proc))) {
		if (++dcnt > 50000)
			break;
		if (time(NULL) - start >= WM_SCAN_TIMEOUT)
			break;
		if (ent->d_name[0] < '0' || ent->d_name[0] > '9')
			continue;
		if (!likely_wm_pid(ent->d_name))
			continue;
		if (read_comm(ent->d_name, comm, sizeof(comm)) != 0)
			continue;
		if (comm[0] == '\0')
			continue;

		char fc = comm[0];
		for (size_t i = 0; i < ARRAY_LEN(wms); ++i) {
			if (fc != wms[i].name[0])
				continue;
			if (str_eq(comm, wms[i].name) ||
			    str_prefix(comm, wms[i].name, wms[i].len)) {
				str_copy(buf, wms[i].name, sz);
				str_copy(wm_cache, buf, sizeof(wm_cache));
				wm_cached = 1;
				closedir(proc);
				return;
			}
		}
	}

	closedir(proc);
	str_copy(buf, "unknown", sz);
	str_copy(wm_cache, "unknown", sizeof(wm_cache));
	wm_cached = 1;
}

static void
get_os(char *buf, size_t sz)
{
	if (os_cached) {
		str_copy(buf, os_cache, sz);
		return;
	}

	FILE *f = fopen("/etc/os-release", "r");
	if (!f) {
		str_copy(buf, "Linux", sz);
		str_copy(os_cache, "Linux", sizeof(os_cache));
		os_cached = 1;
		return;
	}

	char line[MAX_LINE];
	char pretty[MAX_LINE] = {0};
	char name[MAX_LINE] = {0};
	int found_pretty = 0;
	int found_name = 0;

	while (fgets(line, sizeof(line), f)) {
		if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
			char *start = strchr(line, '"');
			char *end = strrchr(line, '"');
			if (start && end && start < end) {
				*end = '\0';
				str_copy(pretty, start + 1, sizeof(pretty));
				found_pretty = 1;
				break;
			}
		} else if (strncmp(line, "NAME=", 5) == 0 && !found_name) {
			char *start = strchr(line, '"');
			char *end = strrchr(line, '"');
			if (start && end && start < end) {
				*end = '\0';
				str_copy(name, start + 1, sizeof(name));
				found_name = 1;
			} else {
				char *p = line + 5;
				size_t len = strnlen(p, sizeof(line) - 5);
				if (len > 0 && p[len - 1] == '\n')
					p[len - 1] = '\0';
				str_copy(name, p, sizeof(name));
				found_name = 1;
			}
		}
	}

	if (found_pretty) {
		str_copy(buf, pretty, sz);
		str_copy(os_cache, pretty, sizeof(os_cache));
	} else if (found_name) {
		str_copy(buf, name, sz);
		str_copy(os_cache, name, sizeof(os_cache));
	} else {
		str_copy(buf, "Linux", sz);
		str_copy(os_cache, "Linux", sizeof(os_cache));
	}

	os_cached = 1;
	fclose(f);
}

static void
print_sep(size_t len)
{
	for (size_t i = 0; i < len; ++i)
		putchar('-');
	putchar('\n');
}

static void
print_version(void)
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
	    CACHE_SIZE, MAX_CHAIN, PATH_MAX, PID_MAX, WM_SCAN_TIMEOUT);
}

static void
sanitise_rel(char *dst, size_t sz, const char *src)
{
	size_t i = 0;
	while (i < sz - 1 && src[i] != '\0' && src[i] != ' ') {
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
}

static void
display(const char *user, const char *host, const char *os, const char *kern,
    const char *shell, const char *wm, const char *term)
{
	char rel[64];
	sanitise_rel(rel, sizeof(rel), kern);

	char tmp[256];
	int n = snprintf(tmp, sizeof(tmp), "%s@%s", user, host);
	size_t len = (n > 0 && (size_t)n < sizeof(tmp)) ? (size_t)n : 0;

	printf("%s    ___ %s     %s%s@%s%s\n",
	    COLOR_1, COLOR_RESET, COLOR_1, user, host, COLOR_RESET);
	printf("%s   (%s.· %s|%s     ", COLOR_1, COLOR_2, COLOR_1,
	    COLOR_RESET);
	print_sep(len);
	printf("%s   (%s<>%s %s|%s     %sOS:%s %s\n",
	    COLOR_1, COLOR_3, COLOR_RESET, COLOR_1, COLOR_RESET,
	    COLOR_3, COLOR_RESET, os);
	printf("%s  / %s__  %s\\%s    %sKernel:%s %s\n",
	    COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET,
	    rel);
	printf("%s ( %s/  \\ %s/|%s   %sShell:%s %s\n",
	    COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET,
	    shell);
	printf("%s_%s/\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n",
	    COLOR_3, COLOR_1, COLOR_2, COLOR_1, COLOR_3, COLOR_1,
	    COLOR_RESET, COLOR_3, COLOR_RESET, wm);
	printf("%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n",
	    COLOR_1, COLOR_3, COLOR_1, COLOR_3, COLOR_RESET,
	    COLOR_3, COLOR_RESET, term);
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

	if (uname(&info) != 0)
		return 1;

	get_user(user, sizeof(user));
	get_host(host, sizeof(host));

	proc_t chain[CACHE_SIZE];
	cache_cnt = 0;
	size_t cnt = build_chain(getpid(), chain, CACHE_SIZE);

	get_shell(chain, cnt, shell, sizeof(shell));
	get_term(chain, cnt, term, sizeof(term));
	get_wm(wm, sizeof(wm));
	get_os(os, sizeof(os));

	display(user, host, os, info.release, shell, wm, term);

	return 0;
}
