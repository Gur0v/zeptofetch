#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <string.h>
#include <limits.h>
#include "system_info.h"
#include "config.h"

void get_username(char *username, size_t size) {
    char *env_user = getenv("USER");
    struct passwd *pwd = getpwuid(getuid());
    strncpy(username, env_user ? env_user : (pwd && pwd->pw_name ? pwd->pw_name : "Unknown"), size);
}

void get_hostname(char *hostname, size_t size) {
    if (gethostname(hostname, size) != 0) {
        strncpy(hostname, "Unknown", size);
    }
}

void get_shell(char *shell, size_t size, const char *distro) {
    if (strcmp(distro, "FreeBSD") == 0) {
        struct passwd *pwd = getpwuid(getuid());
        strncpy(shell, pwd && pwd->pw_shell ? strrchr(pwd->pw_shell, '/') + 1 : "Unknown", size);
    } else {
        char path[PATH_MAX];
        snprintf(path, sizeof(path), PROC_PATH_FORMAT, getppid());
        ssize_t len = readlink(path, shell, size - 1);
        if (len != -1) {
            shell[len] = '\0';
            strncpy(shell, strrchr(shell, '/') + 1, size);
        } else {
            strncpy(shell, "Unknown", size);
        }
    }
}

void get_wm(char *wm, size_t size) {
    FILE *proc = popen(
        "ps -e | grep -E 'dwm|i3|xfwm4|openbox|kwin|sway|gnome-shell|mate-session|xfce4-session|lxqt|cinnamon|fluxbox|herbstluftwm|bspwm|awesome|spectrwm|wmii|xmonad|icewm|jwm' | awk '{print $4}'",
        "r"
    );
    if (proc && fgets(wm, size, proc)) {
        wm[strcspn(wm, "\n")] = '\0';
    } else {
        strncpy(wm, "Unknown", size);
    }
    if (proc) pclose(proc);
}

void get_term(char *term, size_t size) {
    strncpy(term, getenv("TERM") ? getenv("TERM") : "Unknown", size);
}

void get_distro(char *distro, size_t size) {
    FILE *os_release = fopen("/etc/os-release", "r");
    if (os_release) {
        char line[BUFFER_SIZE];
        while (fgets(line, sizeof(line), os_release)) {
            if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                char *start = strchr(line, '"');
                char *end = strrchr(line, '"');
                if (start && end && start != end) {
                    *end = '\0';
                    strncpy(distro, start + 1, size);
                } else {
                    strncpy(distro, "Unknown", size);
                }
                break;
            }
        }
        fclose(os_release);
    } else {
        strncpy(distro, "Unknown", size);
    }
}
