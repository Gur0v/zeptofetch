#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <string.h>
#include <limits.h>
void get_username(char *u, size_t s) { char *e = getenv("USER"); if (e) strncpy(u, e, s); else { struct passwd *p = getpwuid(getuid()); strncpy(u, p ? p->pw_name : "Unknown", s); } }
void get_hostname(char *h, size_t s) { if (gethostname(h, s) != 0) strncpy(h, "Unknown", s); }
void get_shell(char *sh, size_t s, const char *distro) { if (strcmp(distro, "FreeBSD") == 0) { struct passwd *p = getpwuid(getuid()); if (p) { const char *shell = strrchr(p->pw_shell, '/'); strncpy(sh, shell ? shell + 1 : "Unknown", s); } else strncpy(sh, "Unknown", s); } else { char p[PATH_MAX]; snprintf(p, sizeof(p), "/proc/%d/exe", getppid()); ssize_t l = readlink(p, sh, s - 1); if (l != -1) { sh[l] = '\0'; const char *n = strrchr(sh, '/'); strncpy(sh, n ? n + 1 : "Unknown", s); } else strncpy(sh, "Unknown", s); } }
void get_wm(char *wm, size_t s) { FILE *f = popen("ps -e | grep -E 'dwm|i3|xfwm4|openbox|kwin|sway|gnome-shell|mate-session|xfce4-session|lxqt|cinnamon|fluxbox|herbstluftwm|bspwm|awesome|spectrwm|wmii|xmonad|icewm|jwm' | awk '{print $4}'", "r"); if (f) { if (fgets(wm, s, f)) wm[strcspn(wm, "\n")] = '\0'; else strncpy(wm, "Unknown", s); pclose(f); } else strncpy(wm, "Unknown", s); }
void get_term(char *t, size_t s) { char *e = getenv("TERM"); strncpy(t, e ? e : "Unknown", s); }
void get_distro(char *d, size_t s) { FILE *f = fopen("/etc/os-release", "r"); if (f) { char line[256]; while (fgets(line, sizeof(line), f)) { if (strncmp(line, "PRETTY_NAME=", 12) == 0) { char *start = strchr(line, '"'); char *end = strrchr(line, '"'); if (start && end && start != end) { *end = '\0'; strncpy(d, start + 1, s); fclose(f); return; } } } fclose(f); } strncpy(d, "Unknown", s); }
int main() { char u[256], h[256], sh[256], wm[256], t[256], d[256]; get_username(u, sizeof(u)); get_hostname(h, sizeof(h)); struct utsname si; uname(&si); get_distro(d, sizeof(d)); get_shell(sh, sizeof(sh), strstr(d, "FreeBSD") ? "FreeBSD" : "Linux"); get_wm(wm, sizeof(wm)); get_term(t, sizeof(t)); printf("    ___      %s@%s\n   (.· |     --------------\n   (<> |     OS: %s\n  / __  \\    Kernel: %s\n ( /  \\ /|   Shell: %s\n_/\\ __)/_)   WM: %s\n\\/-____\\/    Terminal: %s\n", u, h, d, si.release, sh, wm, t); return 0; }
