#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <string.h>
#include <limits.h>
#include "config.h"
void get_username(char *u, size_t s) { char *e = getenv("USER"); strncpy(u, e ? e : (getpwuid(getuid())->pw_name ?: "Unknown"), s); }
void get_hostname(char *h, size_t s) { strncpy(h, gethostname(h, s) == 0 ? h : "Unknown", s); }
void get_shell(char *sh, size_t s, const char *distro) { if (strcmp(distro, "FreeBSD") == 0) { struct passwd *p = getpwuid(getuid()); strncpy(sh, p && p->pw_shell ? strrchr(p->pw_shell, '/') + 1 : "Unknown", s); } else { char path[PATH_MAX]; snprintf(path, sizeof(path), "/proc/%d/exe", getppid()); ssize_t len = readlink(path, sh, s - 1); strncpy(sh, len != -1 ? ({ sh[len] = '\0'; strrchr(sh, '/') + 1; }) : "Unknown", s); } }
void get_wm(char *wm, size_t s) { FILE *f = popen("ps -e | grep -E 'dwm|i3|xfwm4|openbox|kwin|sway|gnome-shell|mate-session|xfce4-session|lxqt|cinnamon|fluxbox|herbstluftwm|bspwm|awesome|spectrwm|wmii|xmonad|icewm|jwm' | awk '{print $4}'", "r"); strncpy(wm, f && fgets(wm, s, f) ? ({ wm[strcspn(wm, "\n")] = '\0'; wm; }) : "Unknown", s); if (f) pclose(f); }
void get_term(char *t, size_t s) { strncpy(t, getenv("TERM") ?: "Unknown", s); }
void get_distro(char *d, size_t s) { FILE *f = fopen("/etc/os-release", "r"); strncpy(d, f ? ({ char line[256]; char *res = "Unknown"; while (fgets(line, sizeof(line), f)) if (strncmp(line, "PRETTY_NAME=", 12) == 0) { char *start = strchr(line, '"'), *end = strrchr(line, '"'); res = (start && end && start != end) ? (*end = '\0', start + 1) : "Unknown"; break; } fclose(f); res; }) : "Unknown", s); }
int main() { char u[256], h[256], sh[256], wm[256], t[256], d[256]; struct utsname si; get_username(u, sizeof(u)), get_hostname(h, sizeof(h)), uname(&si), get_distro(d, sizeof(d)), get_shell(sh, sizeof(sh), strstr(d, "FreeBSD") ? "FreeBSD" : "Linux"), get_wm(wm, sizeof(wm)), get_term(t, sizeof(t)), printf("%s    ___ %s     %s%s@%s%s\n%s   (%s.· %s|%s     --------------\n%s   (%s<>%s %s|%s     %sOS:%s %s\n%s  / %s__  %s\\%s    %sKernel:%s %s\n%s ( %s/  \\ %s/|%s   %sShell:%s %s\n%s%s_%s/\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n", COLOR_1, COLOR_RESET, COLOR_1, u, h, COLOR_RESET, COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_1, COLOR_2, COLOR_RESET, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, d, COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, si.release, COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, sh, COLOR_1, COLOR_3, COLOR_1, COLOR_2, COLOR_1, COLOR_3, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, wm, COLOR_1, COLOR_3, COLOR_1, COLOR_3, COLOR_RESET, COLOR_3, COLOR_RESET, t); return 0; }
