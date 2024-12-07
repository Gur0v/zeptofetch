#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
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

pid_t get_ppid(pid_t pid) {
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *stat_file = fopen(path, "r");
    if (!stat_file) {
        return -1;
    }

    pid_t ppid;
    fscanf(stat_file, "%*d %*s %*c %d", &ppid);
    fclose(stat_file);

    return ppid;
}

void get_process_name(pid_t pid, char *name, size_t size) {
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "/proc/%d/exe", pid);

    ssize_t len = readlink(path, name, size - 1);
    if (len != -1) {
        name[len] = '\0';
    } else {
        strncpy(name, "Unknown", size);
    }
}

void get_term(char *term, size_t size) {
    char *term_program = getenv("TERM_PROGRAM");
    char *ssh_connection = getenv("SSH_CONNECTION");
    char *wt_session = getenv("WT_SESSION");
    char *term_env = getenv("TERM");

    if (term_program) {
        if (strcmp(term_program, "iTerm.app") == 0) {
            strncpy(term, "iTerm2", size);
            return;
        } else if (strcmp(term_program, "Terminal.app") == 0) {
            strncpy(term, "Apple Terminal", size);
            return;
        } else if (strcmp(term_program, "Hyper") == 0) {
            strncpy(term, "HyperTerm", size);
            return;
        } else {
            strncpy(term, term_program, size);
            return;
        }
    }

    if (term_env) {
        if (strcmp(term_env, "tw52") == 0 || strcmp(term_env, "tw100") == 0) {
            strncpy(term, "TosWin2", size);
            return;
        }
    }

    if (ssh_connection) {
        char *ssh_tty = getenv("SSH_TTY");
        if (ssh_tty) {
            strncpy(term, ssh_tty, size);
            return;
        }
    }

    if (wt_session) {
        strncpy(term, "Windows Terminal", size);
        return;
    }

    pid_t parent = getppid();
    while (parent > 0) {
        char name[BUFFER_SIZE] = {0};
        get_process_name(parent, name, sizeof(name));

        if (strstr(name, "gnome-terminal")) {
            strncpy(term, "gnome-terminal", size);
            return;
        } else if (strstr(name, "urxvtd")) {
            strncpy(term, "urxvt", size);
            return;
        } else if (strstr(name, "konsole")) {
            strncpy(term, "Konsole", size);
            return;
        } else if (strstr(name, "alacritty")) {
            strncpy(term, "Alacritty", size);
            return;
        } else if (strstr(name, "xterm")) {
            strncpy(term, "xterm", size);
            return;
        } else if (strstr(name, "kitty")) {
            strncpy(term, "kitty", size);
            return;
        } else if (strstr(name, "wayland-terminal")) {
            strncpy(term, "Wayland Terminal", size);
            return;
        } else if (strstr(name, "ptyxis")) {
            strncpy(term, "ptyxis-agent", size);
            return;
        }

        if (strstr(name, "bash") || strstr(name, "zsh") || strstr(name, "sh")) {
            parent = get_ppid(parent);
            continue;
        }

        if (strlen(name) > 0) {
            strncpy(term, name, size);
            return;
        }

        parent = get_ppid(parent);
    }

    strncpy(term, "Unknown", size);
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
