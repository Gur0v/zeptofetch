#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <limits.h>
#include "config.h"

// Safe string copy utility
static void safe_copy(char *dest, const char *src, size_t size) {
    if (!dest || size == 0) return;
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

// Process utilities
static pid_t parent_pid(pid_t pid) {
    char path[256];
    FILE *file;
    pid_t ppid_val;
    
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (!file) return -1;
    
    fscanf(file, "%*d %*s %*c %d", &ppid_val);
    fclose(file);
    return ppid_val;
}

static void process_executable(pid_t pid, char *buffer, size_t size) {
    char path[256];
    ssize_t len;
    
    snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    len = readlink(path, buffer, size - 1);
    if (len != -1) {
        buffer[len] = '\0';
    } else {
        safe_copy(buffer, "unknown", size);
    }
}

// System information collection
static void get_user(char *user, size_t size) {
    struct passwd *pw = getpwuid(getuid());
    const char *env_user = getenv("USER");
    
    if (env_user) {
        safe_copy(user, env_user, size);
    } else if (pw && pw->pw_name) {
        safe_copy(user, pw->pw_name, size);
    } else {
        safe_copy(user, "user", size);
    }
}

static void get_host(char *host, size_t size) {
    if (gethostname(host, size) != 0) {
        safe_copy(host, "localhost", size);
    }
}

static void get_shell(char *shell, size_t size) {
    char path[PATH_MAX], exe_path[PATH_MAX];
    pid_t parent = parent_pid(getpid());
    
    snprintf(path, sizeof(path), PROC_PATH_FORMAT, parent);
    ssize_t len = readlink(path, exe_path, sizeof(exe_path) - 1);
    
    if (len > 0) {
        exe_path[len] = '\0';
        char *base = strrchr(exe_path, '/');
        safe_copy(shell, base ? base + 1 : exe_path, size);
    } else {
        safe_copy(shell, "unknown", size);
    }
}

static void get_wm(char *wm, size_t size) {
    static const char *known_wms[] = {
        "dwm", "i3", "xfwm4", "openbox", "kwin", "kwin_x11", "kwin_wayland",
        "sway", "gnome-shell", "mutter", "mate-session", "marco", "xfce4-session",
        "lxqt", "cinnamon", "muffin", "fluxbox", "herbstluftwm", "bspwm",
        "awesome", "spectrwm", "wmii", "xmonad", "icewm", "jwm", "hyprland",
        "river", "wayfire", "labwc", "cage"
    };
    
    FILE *ps = popen("ps -eo comm --no-headers", "r");
    if (!ps) {
        safe_copy(wm, "unknown", size);
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), ps)) {
        line[strcspn(line, "\n")] = '\0';
        
        for (size_t i = 0; i < sizeof(known_wms) / sizeof(*known_wms); i++) {
            if (strcmp(line, known_wms[i]) == 0) {
                // Handle special cases
                if (strcmp(line, "gnome-shell") == 0) {
                    safe_copy(wm, "mutter", size);
                } else if (strcmp(line, "mate-session") == 0) {
                    safe_copy(wm, "marco", size);
                } else if (strcmp(line, "cinnamon") == 0) {
                    safe_copy(wm, "muffin", size);
                } else if (strcmp(line, "kwin_x11") == 0 || strcmp(line, "kwin_wayland") == 0) {
                    safe_copy(wm, "kwin", size);
                } else {
                    safe_copy(wm, known_wms[i], size);
                }
                pclose(ps);
                return;
            }
        }
    }
    
    pclose(ps);
    safe_copy(wm, "unknown", size);
}

static void get_terminal(char *term, size_t size) {
    pid_t parent = parent_pid(getpid());
    
    while (parent > 0) {
        char name[256];
        process_executable(parent, name, sizeof(name));
        
        char *base = strrchr(name, '/');
        base = base ? base + 1 : name;
        
        // Common terminal mappings
        struct { const char *match; const char *name; } terminals[] = {
            {"gnome-terminal", "gnome-terminal"},
            {"urxvtd", "urxvt"},
            {"konsole", "konsole"},
            {"alacritty", "alacritty"},
            {"xterm", "xterm"},
            {"kitty", "kitty"},
            {"foot", "foot"},
            {"wezterm", "wezterm"},
            {"st", "st"},
            {"termite", "termite"},
            {"terminator", "terminator"},
            {"tilix", "tilix"},
            {"terminology", "terminology"},
            {"qterminal", "qterminal"},
            {"lxterminal", "lxterminal"},
            {"mate-terminal", "mate-terminal"},
            {"xfce4-terminal", "xfce4-terminal"},
            {"wayland-terminal", "Wayland Terminal"},
            {"ptyxis", "ptyxis"},
            {"rio", "rio"},
            {"ghostty", "ghostty"}
        };
        
        for (size_t i = 0; i < sizeof(terminals) / sizeof(*terminals); i++) {
            if (strstr(base, terminals[i].match)) {
                safe_copy(term, terminals[i].name, size);
                return;
            }
        }
        
        // Skip shell processes
        if (strstr(base, "bash") || strstr(base, "zsh") || strstr(base, "sh")) {
            parent = parent_pid(parent);
            continue;
        }
        
        // Return process name if found
        if (strlen(base) > 0) {
            safe_copy(term, base, size);
            return;
        }
        
        parent = parent_pid(parent);
    }
    
    safe_copy(term, "unknown", size);
}

static void get_os(char *os, size_t size) {
    FILE *file = fopen("/etc/os-release", "r");
    if (!file) {
        safe_copy(os, "Linux", size);
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *start = strchr(line, '"');
            char *end = strrchr(line, '"');
            
            if (start && end && start < end) {
                *end = '\0';
                safe_copy(os, start + 1, size);
            } else {
                safe_copy(os, "Linux", size);
            }
            fclose(file);
            return;
        }
    }
    
    safe_copy(os, "Linux", size);
    fclose(file);
}

// Display functions
static void print_separator(size_t length) {
    for (size_t i = 0; i < length; i++) {
        putchar('-');
    }
    putchar('\n');
}

static void display_info(const char *user, const char *host, const char *os, 
                        const char *kernel, const char *shell, 
                        const char *wm, const char *terminal) {
    // Calculate the length for the separator under user@host
    size_t user_host_length = strlen(user) + strlen(host) + 1; // +1 for @
    
    printf(
        "%s    ___ %s     %s%s@%s%s\n"
        "%s   (%s.· %s|%s     ",
        COLOR_1, COLOR_RESET, COLOR_1, user, host, COLOR_RESET,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET
    );
    print_separator(user_host_length);
    
    printf(
        "%s   (%s<>%s %s|%s     %sOS:%s %s\n"
        "%s  / %s__  %s\\%s    %sKernel:%s %s\n"
        "%s ( %s/  \\ %s/|%s   %sShell:%s %s\n"
        "%s%s_/%s\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n"
        "%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n",
        COLOR_1, COLOR_2, COLOR_RESET, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, os,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, kernel,
        COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, shell,
        COLOR_1, COLOR_3, COLOR_1, COLOR_2, COLOR_1, COLOR_3, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, wm,
        COLOR_1, COLOR_3, COLOR_1, COLOR_3, COLOR_RESET, COLOR_3, COLOR_RESET, terminal
    );
}

int main(void) {
    char user[64];
    char host[64];
    char shell[64];
    char wm[64];
    char terminal[64];
    char os[128];
    struct utsname info;
    
    if (uname(&info) != 0) {
        perror("uname");
        return 1;
    }
    
    get_user(user, sizeof(user));
    get_host(host, sizeof(host));
    get_shell(shell, sizeof(shell));
    get_wm(wm, sizeof(wm));
    get_terminal(terminal, sizeof(terminal));
    get_os(os, sizeof(os));
    
    display_info(user, host, os, info.release, shell, wm, terminal);
    
    return 0;
}
