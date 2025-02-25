#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <limits.h>
#include <fcntl.h>
#include "config.h"

static pid_t get_ppid(pid_t pid) {
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *stat_file = fopen(path, "r");
    if (!stat_file) return -1;

    pid_t ppid;
    int result = fscanf(stat_file, "%*d %*s %*c %d", &ppid);
    fclose(stat_file);

    return (result == 1) ? ppid : -1;
}

static void get_process_name(pid_t pid, char *name, size_t size) {
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    ssize_t len = readlink(path, name, size - 1);
    if (len != -1) {
        name[len] = '\0';
    } else {
        strncpy(name, "Unknown", size);
    }
}

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

void get_shell(char *shell, size_t size) {
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
    char *term_program = getenv("TERM_PROGRAM");
    char *ssh_connection = getenv("SSH_CONNECTION");
    char *wt_session = getenv("WT_SESSION");
    char *term_env = getenv("TERM");

    if (term_program) {
        if (strcmp(term_program, "iTerm.app") == 0) {
            strncpy(term, "iTerm2", size - 1);
        } else if (strcmp(term_program, "Terminal.app") == 0) {
            strncpy(term, "Apple Terminal", size - 1);
        } else if (strcmp(term_program, "Hyper") == 0) {
            strncpy(term, "HyperTerm", size - 1);
        } else {
            strncpy(term, term_program, size - 1);
        }
        term[size - 1] = '\0';
        return;
    }

    if (term_env) {
        if (strcmp(term_env, "tw52") == 0 || strcmp(term_env, "tw100") == 0) {
            strncpy(term, "TosWin2", size - 1);
            term[size - 1] = '\0';
            return;
        }
    }

    if (ssh_connection) {
        char *ssh_tty = getenv("SSH_TTY");
        if (ssh_tty) {
            strncpy(term, ssh_tty, size - 1);
            term[size - 1] = '\0';
            return;
        }
    }

    if (wt_session) {
        strncpy(term, "Windows Terminal", size - 1);
        term[size - 1] = '\0';
        return;
    }

    pid_t parent = getppid();
    while (parent > 0) {
        char name[BUFFER_SIZE] = {0};
        get_process_name(parent, name, sizeof(name));
        char *base_name = strrchr(name, '/');
        if (base_name) base_name++;
        else base_name = name;

        if (strstr(base_name, "gnome-terminal")) {
            strncpy(term, "gnome-terminal", size - 1);
        } else if (strstr(base_name, "urxvtd")) {
            strncpy(term, "urxvt", size - 1);
        } else if (strstr(base_name, "konsole")) {
            strncpy(term, "Konsole", size - 1);
        } else if (strstr(base_name, "alacritty")) {
            strncpy(term, "Alacritty", size - 1);
        } else if (strstr(base_name, "xterm")) {
            strncpy(term, "xterm", size - 1);
        } else if (strstr(base_name, "kitty")) {
            strncpy(term, "kitty", size - 1);
        } else if (strstr(base_name, "wayland-terminal")) {
            strncpy(term, "Wayland Terminal", size - 1);
        } else if (strstr(base_name, "ptyxis")) {
            strncpy(term, "ptyxis-agent", size - 1);
        }

        if (strstr(base_name, "bash") || strstr(base_name, "zsh") || strstr(base_name, "sh")) {
            parent = get_ppid(parent);
            continue;
        }

        if (strlen(base_name) > 0) {
            strncpy(term, base_name, size - 1);
            term[size - 1] = '\0';
            return;
        }
        parent = get_ppid(parent);
    }

    if (term_env) {
        strncpy(term, term_env, size - 1);
        term[size - 1] = '\0';
    } else {
        strncpy(term, "Unknown", size - 1);
        term[size - 1] = '\0';
    }
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

void print_ui(const char *username, const char *hostname, const char *distro, const char *kernel,
              const char *shell, const char *wm, const char *terminal) {
    size_t line_length = strlen(username) + 1 + strlen(hostname);
    char *separator = malloc(line_length + 1);
    if (!separator) {
        perror("Error allocating memory for separator");
        return;
    }
    memset(separator, '-', line_length);
    separator[line_length] = '\0';

    printf(
        "%s    ___ %s     %s%s@%s%s\n"
        "%s   (%s.· %s|%s     %s\n"
        "%s   (%s<>%s %s|%s     %sOS:%s %s\n"
        "%s  / %s__  %s\\%s    %sKernel:%s %s\n"
        "%s ( %s/  \\ %s/|%s   %sShell:%s %s\n"
        "%s%s_%s/\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n"
        "%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n",
        COLOR_1, COLOR_RESET, COLOR_1, username, hostname, COLOR_RESET,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, separator,
        COLOR_1, COLOR_2, COLOR_RESET, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, distro,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, kernel,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, shell,
        COLOR_1, COLOR_3, COLOR_1, COLOR_2, COLOR_1, COLOR_3, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, wm,
        COLOR_1, COLOR_3, COLOR_1, COLOR_3, COLOR_RESET, COLOR_3, COLOR_RESET, terminal
    );

    free(separator);
}

int main() {
    char username[BUFFER_SIZE], hostname[BUFFER_SIZE];
    char shell[BUFFER_SIZE], wm[BUFFER_SIZE];
    char terminal[BUFFER_SIZE], distro[BUFFER_SIZE];
    struct utsname sys_info;

    if (uname(&sys_info) == -1) {
        perror("Error getting system information");
        return EXIT_FAILURE;
    }

    get_username(username, sizeof(username));
    get_hostname(hostname, sizeof(hostname));
    get_distro(distro, sizeof(distro));
    get_shell(shell, sizeof(shell));
    get_wm(wm, sizeof(wm));
    get_term(terminal, sizeof(terminal));

    print_ui(username, hostname, distro, sys_info.release, shell, wm, terminal);

    return EXIT_SUCCESS;
}
