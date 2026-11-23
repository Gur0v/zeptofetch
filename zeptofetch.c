#if defined(__FreeBSD__) && defined(_POSIX_C_SOURCE)
#undef _POSIX_C_SOURCE
#endif
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#if defined(__FreeBSD__)
#include <sys/param.h>
#include <sys/procctl.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#define DEFAULT_OS "FreeBSD"
#else
#include <sys/prctl.h>
#define DEFAULT_OS "Linux"
#endif
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

#define VERSION     "v2.3"
#define COPYRIGHT   "2024-2026"

#define CR          "\033[0m"
#define C1          "\033[1;34m"
#define C2          "\033[1;37m"
#define C3          "\033[1;38;5;208m"

#define MAX_PATH    4096
#define MAX_CHAIN   64
#define MAX_LINE    128
#define MAX_NAME    64
#define MAX_PID     4194304

#define ARRLEN(a)   (sizeof(a) / sizeof((a)[0]))

typedef struct {
	pid_t ppid;
	char exe[MAX_PATH];
} Process;

typedef struct {
	char user[MAX_NAME];
	char host[MAX_NAME];
	char os[MAX_LINE];
	char kernel[MAX_NAME];
	char shell[MAX_NAME];
	char wm[MAX_NAME];
	char term[MAX_NAME];
} SystemInfo;

static const char *shells[] = {
	"bash", "sh", "zsh", "fish", "dash",
	"ash", "busybox", "ksh", "tcsh", "csh",
	"elvish", "nushell", "nu", "xonsh", "ion",
	"oil", "oil.ovm", "osh", "ysh", "murex", "powershell", "pwsh",
	"rc", "es", "yash", "mksh",
	"oksh", "pdksh",
};

static const char *terms[] = {
	"gnome-terminal", "konsole", "xfce4-terminal", "xterm",
	"kgx", "ptyxis", "mate-terminal", "qterminal", "lxterminal",
	"alacritty", "ghostty", "kitty", "wezterm", "foot",
	"terminator", "urxvt", "st",
	"tilix", "guake", "yakuake", "terminology",
	"sakura", "tilda",
	"termite", "roxterm", "hyper", "tabby",
	"rio", "contour", "cosmic-term",
	"warp", "wave", "extraterm", "zutty",
	"cool-retro-term", "mlterm", "aterm", "eterm",
	"kterm", "lilyterm", "evilvte",
	"mrxvt", "fbterm", "nxterm", "pterm",
	"termine", "wterm", "xvt", "yaft",
};

static const char *wrappers[] = {
	"bwrap", "sudo", "su", "doas", "strace",
	"ltrace", "gdb", "lldb", "perf", "time",
	"valgrind", "proot", "script", "run-parts",
};

static const char *wms[] = {
	"kwin", "mutter", "gnome-shell", "xfwm4",
	"Hyprland", "sway", "muffin", "marco",
	"openbox", "i3", "bspwm", "awesome",
	"dwm", "xmonad", "cinnamon", "mate-session",
	"labwc", "niri", "river", "wayfire",
	"cosmic-comp", "dwl", "mango", "melon",
	"fluxbox", "icewm", "jwm",
	"enlightenment", "qtile", "leftwm",
	"herbstluftwm", "spectrwm", "ratpoison",
	"stumpwm", "sawfish", "fvwm", "fvwm3",
	"fvwm-crystal", "pekwm", "windowmaker",
	"afterstep", "blackbox", "wmaker", "cwm",
	"2bwm", "berry", "cage", "catwm",
	"compiz", "ctwm", "dminiwm", "echinus",
	"evilwm", "frankenwm", "goomwwm", "ion",
	"lfwm", "metacity", "notion", "olivetti",
	"plwm", "snapwm", "tinywm", "trayer",
	"twm", "vwm", "waimea", "wmii",
	"wmx", "acme",
};

static void
copy_text(char *dst, const char *src, size_t size)
{
	const unsigned char *p = (const unsigned char *)src;
	unsigned int code, min;
	size_t n, i, used = 0;

	if (!dst || !src || !size)
		return;
	while (*p && used + 1 < size) {
		if (*p < 0x80) {
			dst[used++] = *p >= 32 && *p != 127 ? *p : '_';
			p++;
			continue;
		}
		if (*p >= 0xc2 && *p <= 0xdf) {
			n = 2;
			code = *p & 0x1f;
			min = 0x80;
		} else if (*p >= 0xe0 && *p <= 0xef) {
			n = 3;
			code = *p & 0x0f;
			min = 0x800;
		} else if (*p >= 0xf0 && *p <= 0xf4) {
			n = 4;
			code = *p & 0x07;
			min = 0x10000;
		} else {
			dst[used++] = '_';
			p++;
			continue;
		}
		for (i = 1; i < n && p[i] && (p[i] & 0xc0) == 0x80; i++)
			code = (code << 6) | (p[i] & 0x3f);
		if (i != n || code < min || code > 0x10ffff ||
		    (code >= 0xd800 && code <= 0xdfff) ||
		    (code >= 0x80 && code <= 0x9f)) {
			dst[used++] = '_';
			p += i;
			continue;
		}
		if (used + n >= size)
			break;
		for (i = 0; i < n; i++)
			dst[used++] = *p++;
	}
	dst[used] = '\0';
}

static size_t
text_width(const char *text)
{
	mbstate_t state = {0};
	wchar_t wc;
	size_t total = 0, n;
	int width;

	while (*text) {
		n = mbrtowc(&wc, text, MB_CUR_MAX, &state);
		if (n == (size_t)-1 || n == (size_t)-2) {
			memset(&state, 0, sizeof(state));
			text++;
			total++;
			continue;
		}
		if (!n)
			break;
		width = wcwidth(wc);
		total += width < 0 ? 1 : (size_t)width;
		text += n;
	}
	return total;
}

static const char *
find_match(const char *name, const char **names, size_t count)
{
	size_t i;

	if (!name || !*name)
		return NULL;

	for (i = 0; i < count; i++) {
		const char *id = names[i];
		size_t len;
		char sep;

		if (name[0] != id[0])
			continue;
		len = strlen(id);
		if (strncmp(name, id, len) != 0)
			continue;

		sep = name[len];
		if (sep == '\0' || sep == '-' || sep == '.' || sep == '_')
			return id;
	}

	return NULL;
}

static void
copy_basename(const char *path, char *out, size_t size)
{
	const char *base = strrchr(path, '/');
	copy_text(out, base ? base + 1 : path, size);
}

static const char *
shell_alias(const char *name)
{
	if (strcmp(name, "nu") == 0)
		return "nushell";
	if (strcmp(name, "oil.ovm") == 0)
		return "oil";
	if (strcmp(name, "busybox") == 0)
		return "ash";
	return name;
}

static const char *
env_value(const char *key)
{
	const char *value = getenv(key);

	return value && *value ? value : NULL;
}

static int
valid_pid(pid_t pid)
{
	return pid > 0 && pid <= MAX_PID;
}

#if !defined(__FreeBSD__)
static void
proc_path(pid_t pid, const char *file, char *buf, size_t size)
{
	if (!valid_pid(pid) || !file || strchr(file, '/')) {
		if (size) buf[0] = '\0';
		return;
	}
	snprintf(buf, size, "/proc/%d/%s", (int)pid, file);
}

#endif

#if !defined(__FreeBSD__) || defined(ZEPTOFETCH_TEST)
static int
read_link(const char *path, char *buf, size_t size)
{
	ssize_t len;

	if (!size)
		return -1;
	len = readlink(path, buf, size - 1);
	if (len <= 0)
		return -1;
	buf[len] = '\0';
	return 0;
}

static int
read_line_at(int base, const char *path, char *buf, size_t size)
{
	int fd;
	ssize_t n;
	size_t len;

	if (!size || (fd = openat(base, path, O_RDONLY)) < 0)
		return -1;
	n = read(fd, buf, size - 1);
	close(fd);

	if (n <= 0)
		return -1;
	buf[n] = '\0';

	len = strcspn(buf, "\n");
	buf[len] = '\0';
	return 0;
}

static int
read_first_line(const char *path, char *buf, size_t size)
{
	return read_line_at(AT_FDCWD, path, buf, size);
}

static int
read_record(const char *path, char *buf, size_t size)
{
	char extra;
	int fd;
	ssize_t got;
	size_t n = 0;

	if (!size || (fd = open(path, O_RDONLY)) < 0)
		return -1;
	do {
		if (n < size - 1)
			got = read(fd, buf + n, size - 1 - n);
		else
			got = read(fd, &extra, 1);
		if (got < 0 && errno == EINTR)
			continue;
		if (got <= 0 || n == size - 1)
			break;
		n += (size_t)got;
	} while (1);
	close(fd);
	buf[n] = '\0';
	return n && got == 0 ? 0 : -1;
}

#endif

#if !defined(__FreeBSD__)
static pid_t
parse_proc_stat(const char *buf)
{
	const char *p = strrchr(buf, ')');
	unsigned long parent = 0;

	if (!p || p[1] != ' ' || !p[2] || p[3] != ' ')
		return -1;
	p += 4;
	if (*p < '0' || *p > '9')
		return -1;
	while (*p >= '0' && *p <= '9') {
		parent = parent * 10 + (unsigned)(*p++ - '0');
		if (parent > MAX_PID)
			return -1;
	}
	if (*p != ' ' && *p != '\n' && *p != '\0')
		return -1;
	return valid_pid((pid_t)parent) ? (pid_t)parent : -1;
}

#endif

static int
read_proc(pid_t pid, Process *p, int want_exe)
{
	if (!valid_pid(pid) || !p)
		return -1;

	p->ppid = -1;
	p->exe[0] = '\0';

#if defined(__FreeBSD__)
	struct kinfo_proc kp;
	size_t len = sizeof(kp);
	int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
	if (sysctl(mib, ARRLEN(mib), &kp, &len, NULL, 0) != 0 ||
		len != sizeof(kp)) return -1;
	p->ppid = valid_pid(kp.ki_ppid) ? kp.ki_ppid : -1;
	if (want_exe) {
		int path_mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, pid };
		len = sizeof(p->exe);
		if (sysctl(path_mib, ARRLEN(path_mib), p->exe, &len, NULL, 0) != 0 ||
			len == 0 || !p->exe[0])
			copy_text(p->exe, kp.ki_comm, sizeof(p->exe));
		p->exe[sizeof(p->exe) - 1] = '\0';
	}
#else
	char path[64], buf[4096];
	proc_path(pid, "stat", path, sizeof(path));
	if (read_record(path, buf, sizeof(buf)) != 0)
		return -1;

	p->ppid = parse_proc_stat(buf);

	if (want_exe) {
		proc_path(pid, "exe", path, sizeof(path));
		(void)read_link(path, p->exe, sizeof(p->exe));
	}

#endif
	return 0;
}

static void
fetch_user(char *buf, size_t size)
{
	const char *env = getenv("USER");
	const struct passwd *pw;

	if (env && *env) {
		copy_text(buf, env, size);
		return;
	}

	pw = getpwuid(getuid());
	copy_text(buf, (pw && pw->pw_name) ? pw->pw_name : "user", size);
}

static void
fetch_host(char *buf, size_t size)
{
	if (!size)
		return;
	if (gethostname(buf, size) != 0)
		copy_text(buf, "localhost", size);
	else
		buf[size - 1] = '\0';
	copy_text(buf, buf, size);
}

static void
fetch_kernel(char *buf, size_t size)
{
	struct utsname un;
	if (uname(&un) == 0)
		copy_text(buf, un.release, size);
	else
		copy_text(buf, "unknown", size);
}

static int
parse_os_value(const char *line, const char *key, char *out, size_t n)
{
	size_t klen, used = 0;
	const char *v;
	char quote, tmp[MAX_LINE];
	int closed;

	if (!n)
		return 0;
	klen = strlen(key);
	if (strncmp(line, key, klen) != 0 || line[klen] != '=')
		return 0;
	v = line + klen + 1;
	quote = (*v == '\'' || *v == '"') ? *v++ : '\0';
	closed = !quote;
	while (*v && *v != '\n' && *v != '\r') {
		if (quote && *v == quote) {
			v++;
			closed = 1;
			break;
		}
		if (*v == '\\' && quote != '\'') {
			if (!v[1] || v[1] == '\n' || v[1] == '\r')
				return 0;
			if (!quote || v[1] == '$' || v[1] == '`' || v[1] == '"' || v[1] == '\\') v++;
		}
		if (used + 1 >= sizeof(tmp))
			return 0;
		tmp[used++] = *v++;
	}
	if (!closed)
		return 0;
	while (*v == ' ' || *v == '\t' || *v == '\r')
		v++;
	if (*v && *v != '\n')
		return 0;
	tmp[used] = '\0';
	copy_text(out, tmp, n);
	return out[0] != '\0';
}

static int
read_os(FILE *f, char *buf, size_t size)
{
	char line[MAX_LINE], name[MAX_LINE] = "";

	if (!size)
		return 0;
	buf[0] = '\0';

	while (fgets(line, sizeof(line), f)) {
		if (!strchr(line, '\n') && !feof(f)) {
			int c;
			while ((c = fgetc(f)) != '\n' && c != EOF) {}
			if (c != EOF)
				continue;
			if (strlen(line) == sizeof(line) - 1)
				break;
		}
		if (parse_os_value(line, "PRETTY_NAME", buf, size))
			break;
		if (!*name)
			parse_os_value(line, "NAME", name, sizeof(name));
	}

	if (!*buf && *name)
		copy_text(buf, name, size);
	return *buf != '\0';
}

static void
fetch_os(char *buf, size_t size)
{
	const char *paths[] = {
		"/etc/os-release",
#if defined(__FreeBSD__)
		"/var/run/os-release",
#endif
		"/usr/lib/os-release"
	};
	FILE *file;
	size_t i;
	int found;

	for (i = 0; i < ARRLEN(paths); i++) {
		file = fopen(paths[i], "r");
		if (!file)
			continue;
		found = read_os(file, buf, size);
		fclose(file);
		if (found)
			return;
	}
	copy_text(buf, DEFAULT_OS, size);
}

static int
console_name(char *name, size_t size)
{
#if defined(__FreeBSD__)
	const char *path = ttyname(STDIN_FILENO);

	if (!path || (strncmp(path, "/dev/ttyv", 9) &&
	    strcmp(path, "/dev/console")))
		return 0;
#else
	char link[MAX_PATH];
	const char *path = link;

	if (read_link("/proc/self/fd/0", link, sizeof(link)))
		path = ttyname(STDIN_FILENO);
	if (!path || strncmp(path, "/dev/tty", 8) ||
	    path[8] < '0' || path[8] > '9')
		return 0;
#endif
	copy_basename(path, name, size);
	return 1;
}

#if !defined(__FreeBSD__) || defined(ZEPTOFETCH_TEST)
static int
cgroup_container(FILE *file)
{
	const char *markers[] = { "docker", "lxc", "containerd", "podman" };
	size_t matched[ARRLEN(markers)] = {0};
	size_t i;
	int c;

	while ((c = fgetc(file)) != EOF) {
		for (i = 0; i < ARRLEN(markers); i++) {
			if (c == markers[i][matched[i]])
				matched[i]++;
			else
				matched[i] = c == markers[i][0];
			if (!markers[i][matched[i]])
				return 1;
		}
	}
	return 0;
}
#endif

static int
is_wslg(void)
{
#if defined(__FreeBSD__)
	return 0;
#else
	const char *wayland = env_value("WAYLAND_DISPLAY");
	const char *runtime = env_value("XDG_RUNTIME_DIR");
	const char *display = env_value("DISPLAY");
	char path[MAX_PATH], resolved[MAX_PATH];
	int n;

	if (wayland && (wayland[0] == '/' || runtime)) {
		if (wayland[0] == '/')
			n = snprintf(path, sizeof(path), "%s", wayland);
		else
			n = snprintf(path, sizeof(path), "%s/%s", runtime, wayland);
		if (n > 0 && (size_t)n < sizeof(path) && realpath(path, resolved) &&
		    !strncmp(resolved, "/mnt/wslg/", 10))
			return 1;
	}
	if (display && display[0] == ':' && display[1] >= '0' &&
	    display[1] <= '9') {
		char *end;
		unsigned long num = strtoul(display + 1, &end, 10);
		if ((*end == '\0' || *end == '.') && num <= 65535) {
			n = snprintf(path, sizeof(path), "/tmp/.X11-unix/X%lu", num);
			if (n > 0 && (size_t)n < sizeof(path) && realpath(path, resolved) &&
			    !strncmp(resolved, "/mnt/wslg/.X11-unix/", sizeof("/mnt/wslg/.X11-unix/") - 1))
				return 1;
		}
	}
	return 0;
#endif
}

static int
detect_container(void)
{
#if defined(__FreeBSD__)
	int jailed = 0;
	size_t len = sizeof(jailed);
	return sysctlbyname("security.jail.jailed", &jailed, &len, NULL, 0) == 0 && jailed;
#else
	const char *env = getenv("CONTAINER_ID");
	if (env && *env)
		return 1;
	if (access("/.dockerenv", F_OK) == 0)
		return 1;

	struct stat st;
	if (stat("/run/.containerenv", &st) == 0)
		return 1;

	FILE *file = fopen("/proc/1/cgroup", "r");
	int found;

	if (!file)
		return 0;
	found = cgroup_container(file);
	fclose(file);
	return found;
#endif
}

static int
detect_wsl(void)
{
#if defined(__FreeBSD__)
	return 0;
#else
	char buf[512];

	if (getenv("WSLENV"))
		return 1;
	if (access("/mnt/wsl", F_OK) == 0)
		return 1;
	if (access("/proc/sys/fs/binfmt_misc/WSLInterop", F_OK) == 0)
		return 1;

	if (read_first_line("/proc/sys/kernel/osrelease", buf, sizeof(buf)) == 0 &&
		(strstr(buf, "WSL") || strstr(buf, "microsoft")))
		return 1;

	if (read_first_line("/proc/version", buf, sizeof(buf)) == 0 &&
		(strstr(buf, "WSL") || strstr(buf, "Microsoft")))
		return 1;

	return 0;
#endif
}

static const char *
scan_wm(void)
{
	const char *match = NULL;
#if defined(__FreeBSD__)
	int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_RUID, (int)getuid() };
	struct kinfo_proc *procs;
	size_t i, len;
	int attempt;

	for (attempt = 0; attempt < 3; attempt++) {
		len = 0;
		if (sysctl(mib, ARRLEN(mib), NULL, &len, NULL, 0) || !len)
			break;
		if (!(procs = malloc(len)))
			break;
		if (sysctl(mib, ARRLEN(mib), procs, &len, NULL, 0)) {
			free(procs);
			continue;
		}
		for (i = 0; i < len / sizeof(*procs) && !match; i++)
			match = find_match(procs[i].ki_comm, wms, ARRLEN(wms));
		free(procs);
		break;
	}
#else
	DIR *dir;
	const struct dirent *entry;
	struct stat st;
	char path[64], comm[MAX_NAME];
	uid_t uid = getuid();
	int fd;
	size_t len;

	if (!(dir = opendir("/proc")))
		return NULL;
	fd = dirfd(dir);
	while (!match && (entry = readdir(dir))) {
		len = strspn(entry->d_name, "0123456789");
		if (!len || len > 7 || entry->d_name[len] ||
		    !valid_pid((pid_t)strtol(entry->d_name, NULL, 10)))
			continue;
		if (fstatat(fd, entry->d_name, &st, 0) || st.st_uid != uid)
			continue;
		memcpy(path, entry->d_name, len);
		memcpy(path + len, "/comm", sizeof("/comm"));
		if (!read_line_at(fd, path, comm, sizeof(comm)))
			match = find_match(comm, wms, ARRLEN(wms));
	}
	closedir(dir);
#endif
	return match;
}

static void
fetch_wm(char *buf, size_t size)
{
	const char *name;

	if (env_value("SWAYSOCK"))
		name = "sway";
	else if (env_value("HYPRLAND_INSTANCE_SIGNATURE") ||
	    env_value("HYPRLAND_CMD"))
		name = "Hyprland";
	else
		name = scan_wm();
	copy_text(buf, name ? name : "unknown", size);
}

static void
term_hint(char *buf, size_t size)
{
	const char *env = env_value("TERM_PROGRAM");
	if (env_value("KITTY_PID") || env_value("KITTY_INSTALLATION_DIR")) {
		copy_text(buf, "kitty", size);
		return;
	}
	if (env_value("ALACRITTY_SOCKET") || env_value("ALACRITTY_LOG") ||
		env_value("ALACRITTY_WINDOW_ID")) {
		copy_text(buf, "alacritty", size);
		return;
	}
	if (env_value("WEZTERM_EXECUTABLE") || env_value("WEZTERM_PANE")) {
		copy_text(buf, "wezterm", size);
		return;
	}
	if (env_value("TERMUX_VERSION") || env_value("TERMUX_MAIN_PACKAGE_FORMAT")) {
		copy_text(buf, "termux", size);
		return;
	}
	if (env_value("KONSOLE_VERSION")) {
		copy_text(buf, "konsole", size);
		return;
	}
	if (env_value("GNOME_TERMINAL_SCREEN") || env_value("GNOME_TERMINAL_SERVICE")) {
		copy_text(buf, "gnome-terminal", size);
		return;
	}
	if (env && *env) {
		copy_text(buf, env, size);
		return;
	}

	env = env_value("LC_TERMINAL");
	if (env && *env) {
		copy_text(buf, env, size);
		return;
	}

}

static int
probe_session(SystemInfo *info, const char **session)
{
	const char *label;
	int wsl;

	label = NULL;
	if (console_name(info->term, sizeof(info->term)))
		label = info->term;
	else if (detect_container())
		label = "container";
	else if (getenv("SSH_CLIENT") || getenv("SSH_TTY") ||
	    getenv("SSH_CONNECTION"))
		label = "ssh";
	if (label) {
		if (label == info->term ||
		    (!env_value("DISPLAY") && !env_value("WAYLAND_DISPLAY"))) {
			copy_text(info->term, label, sizeof(info->term));
			copy_text(info->wm, "none", sizeof(info->wm));
			return 0;
		}
		*session = label;
	}
	wsl = detect_wsl();
	if (wsl && (env_value("WT_SESSION") || env_value("WT_PROFILE_ID")))
		copy_text(info->term, "Windows Terminal", sizeof(info->term));
	else
		term_hint(info->term, sizeof(info->term));
	if (wsl && is_wslg()) {
		copy_text(info->wm, "WSLg", sizeof(info->wm));
		return 0;
	}
	return 1;
}

static void
match_ancestor(SystemInfo *info, char *fallback, const char *exe)
{
	char name[MAX_NAME];
	const char *shell, *term;

	if (!*exe)
		return;
	copy_basename(exe, name, sizeof(name));
	shell = find_match(name, shells, ARRLEN(shells));
	if (shell && !info->shell[0])
		copy_text(info->shell, shell_alias(shell), sizeof(info->shell));
	if (info->term[0] || shell ||
		find_match(name, wrappers, ARRLEN(wrappers)))
		return;
	term = find_match(name, terms, ARRLEN(terms));
	if (term)
		copy_text(info->term, term, sizeof(info->term));
	else if (!*fallback && strncmp(name, "flatpak-", 8) &&
	    !(strlen(name) > 3 && !strcmp(name + strlen(name) - 3, ".sh")))
		copy_text(fallback, name, MAX_NAME);
}

static void
walk_parents(SystemInfo *info, const char *session)
{
	Process parent;
	char fallback[MAX_NAME] = "";
	const char *hint;
	pid_t pid;
	size_t depth;

	pid = getpid();
	for (depth = 0; depth < MAX_CHAIN && valid_pid(pid); depth++) {
		if (read_proc(pid, &parent, depth != 0) != 0)
			break;
		match_ancestor(info, fallback, parent.exe);
		if ((info->shell[0] && info->term[0]) || parent.ppid == pid)
			break;
		pid = parent.ppid;
	}
	if (!info->shell[0]) {
		hint = getenv("SHELL");
		if (hint && *hint) {
			copy_basename(hint, info->shell, sizeof(info->shell));
			copy_text(info->shell, shell_alias(info->shell),
					sizeof(info->shell));
		} else {
			copy_text(info->shell, "unknown", sizeof(info->shell));
		}
	}
	if (!info->term[0] && session)
		copy_text(info->term, session, sizeof(info->term));
	if (!info->term[0]) {
		hint = env_value("TERMINAL");
		if (hint)
			copy_basename(hint, info->term, sizeof(info->term));
		if (!info->term[0]) {
			hint = env_value("TERM");
			copy_text(info->term, hint ? hint : *fallback ? fallback :
			    "unknown", sizeof(info->term));
		}
	}
}

static void
display(const SystemInfo *info, int color)
{
	const char *c1 = color ? C1 : "";
	const char *c2 = color ? C2 : "";
	const char *c3 = color ? C3 : "";
	const char *cr = color ? CR : "";

	size_t klen = strcspn(info->kernel, " ");
	size_t sep = text_width(info->user) + text_width(info->host) + 1;

	printf("%s    ___ %s     %s%s@%s%s\n",
	    c1, cr, c1, info->user, info->host, cr);
	printf("%s   (%s.· %s|%s     ", c1, c2, c1, cr);
	while (sep--) putchar('-');
	putchar('\n');
	printf("%s   (%s<>%s %s|%s     %sOS:%s %s\n",
	    c1, c3, cr, c1, cr, c3, cr, info->os);
	printf("%s  / %s__  %s\\%s    %sKernel:%s %.*s\n",
	    c1, c2, c1, cr, c3, cr, (int)klen, info->kernel);
	printf("%s ( %s/  \\ %s/|%s   %sShell:%s %s\n",
	    c1, c2, c1, cr, c3, cr, info->shell);
	printf("%s_%s/\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n",
	    c3, c1, c2, c1, c3, c1, cr, c3, cr, info->wm);
	printf("%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n",
	    c1, c3, c1, c3, cr, c3, cr, info->term);
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

static void
set_limits(void)
{
	const struct { int resource; rlim_t cap; const char *name; } limits[] = {
		{ RLIMIT_CPU, 5, "RLIMIT_CPU" },
		{ RLIMIT_NOFILE, 256, "RLIMIT_NOFILE" },
		{ RLIMIT_CORE, 0, "RLIMIT_CORE" }
	};
	size_t i;

	for (i = 0; i < ARRLEN(limits); i++) {
		struct rlimit r;
		if (getrlimit(limits[i].resource, &r) != 0) {
			fprintf(stderr, "Warn: %s get failed\n", limits[i].name);
			continue;
		}
		if (r.rlim_max > limits[i].cap)
			r.rlim_max = limits[i].cap;
		if (r.rlim_cur > r.rlim_max)
			r.rlim_cur = r.rlim_max;
		if (setrlimit(limits[i].resource, &r) != 0)
			fprintf(stderr, "Warn: %s set failed\n", limits[i].name);
	}
}

static int
harden(void)
{
#if defined(__FreeBSD__)
	int trace = PROC_TRACE_CTL_DISABLE;
	if (procctl(P_PID, 0, PROC_TRACE_CTL, &trace))
		fprintf(stderr, "Warn: PROC_TRACE_CTL failed\n");
#ifdef PROC_NO_NEW_PRIVS_CTL
	int no_new_privs = PROC_NO_NEW_PRIVS_ENABLE;
	if (procctl(P_PID, 0, PROC_NO_NEW_PRIVS_CTL, &no_new_privs))
		fprintf(stderr, "Warn: PROC_NO_NEW_PRIVS_CTL failed\n");
#endif
#else
	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0))
		fprintf(stderr, "Warn: PR_SET_NO_NEW_PRIVS failed\n");
	if (prctl(PR_SET_DUMPABLE, 0, 0, 0, 0))
		fprintf(stderr, "Warn: PR_SET_DUMPABLE failed\n");
#endif

	if (geteuid() != getuid() || getegid() != getgid()) {
		fprintf(stderr, "Error: refusing elevated execution\n");
		return -1;
	}

	set_limits();

	return 0;
}

int
main(int argc, char **argv)
{
	SystemInfo info = {0};
	int need_wm;
	const char *session = NULL;

	if (harden())
		return 1;

	if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
		print_version();
		return fflush(stdout) == EOF ? 1 : 0;
	}
	if (argc > 1) {
		fprintf(stderr, "Usage: zeptofetch [-v|--version]\n");
		return 1;
	}

	(void)setlocale(LC_CTYPE, "");
	fetch_user(info.user, sizeof(info.user));
	fetch_host(info.host, sizeof(info.host));
	fetch_os(info.os, sizeof(info.os));
	fetch_kernel(info.kernel, sizeof(info.kernel));
	need_wm = probe_session(&info, &session);
	walk_parents(&info, session);
	if (need_wm)
		fetch_wm(info.wm, sizeof(info.wm));
	display(&info, isatty(STDOUT_FILENO));

	return fflush(stdout) == EOF ? 1 : 0;
}
