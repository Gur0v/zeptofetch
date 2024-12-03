#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <string.h>
#include <limits.h>
#include "config.h"

// Helper functions
void get_username(char *username, size_t size) {
    char *env_user = getenv("USER");
    struct passwd *pwd = getpwuid(getuid());
    strncpy(username, env_user ? env_user : (pwd && pwd->pw_name ? pwd->pw_name : "Unknown"), size);
}

void get_hostname(char *hostname, size_t size) {
    gethostname(hostname, size) == 0 ? hostname : strncpy(hostname, "Unknown", size);
}

void get_shell(char *shell, size_t size, const char *distro) {
    if (strcmp(distro, "FreeBSD") == 0) {
        struct passwd *pwd = getpwuid(getuid());
        strncpy(shell, pwd && pwd->pw_shell ? strrchr(pwd->pw_shell, '/') + 1 : "Unknown", size);
    } else {
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "/proc/%d/exe", getppid());
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
    FILE *proc = popen("ps -e | grep -E 'dwm|i3|xfwm4|openbox|kwin|sway|gnome-shell|mate-session|xfce4-session|lxqt|cinnamon|fluxbox|herbstluftwm|bspwm|awesome|spectrwm|wmii|xmonad|icewm|jwm' | awk '{print $4}'", "r");
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
        char line[256];
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

// Main function
int main() {
    char username[256], hostname[256], shell[256], wm[256], terminal[256], distro[256];
    struct utsname sys_info;

    get_username(username, sizeof(username));
    get_hostname(hostname, sizeof(hostname));
    uname(&sys_info);
    get_distro(distro, sizeof(distro));
    get_shell(shell, sizeof(shell), strstr(distro, "FreeBSD") ? "FreeBSD" : "Linux");
    get_wm(wm, sizeof(wm));
    get_term(terminal, sizeof(terminal));

    printf(
        "%s    ___ %s     %s%s@%s%s\n"
        "%s   (%s.· %s|%s     --------------\n"
        "%s   (%s<>%s %s|%s     %sOS:%s %s\n"
        "%s  / %s__  %s\\%s    %sKernel:%s %s\n"
        "%s ( %s/  \\ %s/|%s   %sShell:%s %s\n"
        "%s%s_%s/\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n"
        "%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n",
        COLOR_1, COLOR_RESET, COLOR_1, username, hostname, COLOR_RESET,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET,
        COLOR_1, COLOR_2, COLOR_RESET, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, distro,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, sys_info.release,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, shell,
        COLOR_1, COLOR_3, COLOR_1, COLOR_2, COLOR_1, COLOR_3, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, wm,
        COLOR_1, COLOR_3, COLOR_1, COLOR_3, COLOR_RESET, COLOR_3, COLOR_RESET, terminal
    );

    return 0;
}
