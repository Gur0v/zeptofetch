#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <fcntl.h>
#include <limits.h>
#include "config.h"

void scpy(char *d, const char *s, size_t n) {
    if (!d || !n) return;
    strncpy(d, s, n - 1);
    d[n - 1] = '\0';
}

pid_t ppid(pid_t pid) {
    char p[BUFFER_SIZE];
    FILE *f;
    pid_t pp;
    snprintf(p, sizeof(p), "/proc/%d/stat", pid);
    if (!(f = fopen(p, "r"))) return -1;
    fscanf(f, "%*d %*s %*c %d", &pp);
    fclose(f);
    return pp;
}

void pexe(pid_t pid, char *b, size_t n) {
    char p[BUFFER_SIZE];
    ssize_t l;
    snprintf(p, sizeof(p), "/proc/%d/exe", pid);
    if ((l = readlink(p, b, n - 1)) != -1)
        b[l] = '\0';
    else
        scpy(b, "Unknown", n);
}

void uname_s(char *u, size_t n) {
    struct passwd *pw = getpwuid(getuid());
    scpy(u, getenv("USER") ?: (pw && pw->pw_name ? pw->pw_name : "Unknown"), n);
}

void hname(char *h, size_t n) {
    if (gethostname(h, n))
        scpy(h, "Unknown", n);
}

void shname(char *s, size_t n) {
    char p[PATH_MAX], b[PATH_MAX];
    pid_t par = ppid(getpid());
    snprintf(p, sizeof(p), PROC_PATH_FORMAT, par);
    ssize_t l = readlink(p, b, sizeof(b) - 1);
    if (l > 0) {
        b[l] = '\0';
        scpy(s, strrchr(b, '/') + 1, n);
    } else {
        scpy(s, "Unknown", n);
    }
}

void wmname(char *w, size_t n) {
    static const char *list[] = {
        "dwm", "i3", "xfwm4", "openbox", "kwin", "sway", "gnome-shell",
        "mate-session", "xfce4-session", "lxqt", "cinnamon", "fluxbox",
        "herbstluftwm", "bspwm", "awesome", "spectrwm", "wmii", "xmonad", "icewm", "jwm"
    };
    FILE *fp = popen(
        "ps -e | grep -E 'dwm|i3|xfwm4|openbox|kwin|sway|gnome-shell|mate-session|xfce4-session|lxqt|cinnamon|fluxbox|herbstluftwm|bspwm|awesome|spectrwm|wmii|xmonad|icewm|jwm' | awk '{print $4}'",
        "r"
    );
    if (!fp || !fgets(w, n, fp)) goto fail;
    w[strcspn(w, "\n")] = '\0';
    for (size_t i = 0; i < sizeof(list) / sizeof(*list); ++i)
        if (strstr(w, list[i])) {
            scpy(w, list[i], n);
            pclose(fp);
            return;
        }
fail:
    scpy(w, "Unknown", n);
    if (fp) pclose(fp);
}

void termname(char *t, size_t n) {
    if (getenv("TERM_PROGRAM")) {
        const char *tp = getenv("TERM_PROGRAM");
        if      (!strcmp(tp, "iTerm.app"))       scpy(t, "iTerm2", n);
        else if (!strcmp(tp, "Terminal.app"))    scpy(t, "Apple Terminal", n);
        else if (!strcmp(tp, "Hyper"))           scpy(t, "HyperTerm", n);
        else                                     scpy(t, tp, n);
        return;
    }
    if (getenv("WT_SESSION"))            { scpy(t, "Windows Terminal", n); return; }
    if (getenv("SSH_CONNECTION") && getenv("SSH_TTY")) {
        scpy(t, getenv("SSH_TTY"), n);
        return;
    }
    if (getenv("TERM")) {
        const char *te = getenv("TERM");
        if (!strcmp(te, "tw52") || !strcmp(te, "tw100")) {
            scpy(t, "TosWin2", n);
            return;
        }
    }
    pid_t par = ppid(getpid());
    while (par > 0) {
        char name[BUFFER_SIZE];
        pexe(par, name, sizeof(name));
        char *base = strrchr(name, '/');
        base = base ? base + 1 : name;
        struct { const char *m, *p; } map[] = {
            {"gnome-terminal", "gnome-terminal"},
            {"urxvtd", "urxvt"},
            {"konsole", "Konsole"},
            {"alacritty", "Alacritty"},
            {"xterm", "xterm"},
            {"kitty", "kitty"},
            {"wayland-terminal", "Wayland Terminal"},
            {"ptyxis", "ptyxis-agent"}
        };
        for (size_t i = 0; i < sizeof(map) / sizeof(*map); ++i)
            if (strstr(base, map[i].m)) {
                scpy(t, map[i].p, n);
                return;
            }
        if (strstr(base, "bash") || strstr(base, "zsh") || strstr(base, "sh")) {
            par = ppid(par);
            continue;
        }
        if (*base) {
            scpy(t, base, n);
            return;
        }
        par = ppid(par);
    }
    scpy(t, "Unknown", n);
}

void distro(char *d, size_t n) {
    FILE *f = fopen("/etc/os-release", "r");
    if (!f) {
        scpy(d, "Unknown", n);
        return;
    }
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), f)) {
        if (!strncmp(line, "PRETTY_NAME=", 12)) {
            char *s = strchr(line, '"'), *e = strrchr(line, '"');
            if (s && e && s < e) {
                *e = '\0';
                scpy(d, s + 1, n);
            } else {
                scpy(d, "Unknown", n);
            }
            fclose(f);
            return;
        }
    }
    scpy(d, "Unknown", n);
    fclose(f);
}

void print_info(
    const char *u,
    const char *h,
    const char *d,
    const char *k,
    const char *s,
    const char *w,
    const char *t
) {
    size_t sl = strlen(u) + strlen(h) + 1;
    char *sep = malloc(sl + 1);
    if (!sep) return;
    memset(sep, '-', sl);
    sep[sl] = '\0';

    printf(
        "%s    ___ %s     %s%s@%s%s\n"
        "%s   (%s.· %s|%s     %s\n"
        "%s   (%s<>%s %s|%s     %sOS:%s %s\n"
        "%s  / %s__  %s\\%s    %sKernel:%s %s\n"
        "%s ( %s/  \\ %s/|%s   %sShell:%s %s\n"
        "%s%s_/%s\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n"
        "%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n",
        COLOR_1, COLOR_RESET, COLOR_1, u, h, COLOR_RESET,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, sep,
        COLOR_1, COLOR_2, COLOR_RESET, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, d,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, k,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, s,
        COLOR_1, COLOR_3, COLOR_1, COLOR_2, COLOR_1, COLOR_3, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, w,
        COLOR_1, COLOR_3, COLOR_1, COLOR_3, COLOR_RESET, COLOR_3, COLOR_RESET, t
    );

    free(sep);
}

int main() {
    char u[BUFFER_SIZE];
    char h[BUFFER_SIZE];
    char s[BUFFER_SIZE];
    char w[BUFFER_SIZE];
    char t[BUFFER_SIZE];
    char d[BUFFER_SIZE];
    struct utsname info;

    if (uname(&info)) {
        perror("uname");
        return 1;
    }

    uname_s(u, sizeof(u));
    hname(h, sizeof(h));
    shname(s, sizeof(s));
    wmname(w, sizeof(w));
    termname(t, sizeof(t));
    distro(d, sizeof(d));

    print_info(
        u,
        h,
        d,
        info.release,
        s,
        w,
        t
    );

    return 0;
}
