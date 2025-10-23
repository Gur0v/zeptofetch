#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <limits.h>
#include <ctype.h>
#include "config.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_PATH_LEN    256
#define MAX_LINE_LEN    256
#define MAX_ENV_LEN     64
#define PATH_BUF_SIZE   PATH_MAX

static void safe_copy(char *dest, const char *src, size_t size) {
    if (!dest || size == 0) 
        return;
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

static int is_valid_pid(pid_t pid) {
    return pid > 0 && pid <= 4194304;
}

static pid_t parent_pid(pid_t pid) {
    char path[MAX_PATH_LEN];
    FILE *file;
    pid_t ppid_val;
    int written;
    
    if (!is_valid_pid(pid)) 
        return -1;
    
    written = snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    if (written < 0 || (size_t)written >= sizeof(path)) 
        return -1;
    
    file = fopen(path, "r");
    if (!file) 
        return -1;
    
    if (fscanf(file, "%*d %*s %*c %d", &ppid_val) != 1) {
        fclose(file);
        return -1;
    }
    fclose(file);
    
    return is_valid_pid(ppid_val) ? ppid_val : -1;
}

static int process_executable(pid_t pid, char *buffer, size_t size) {
    char path[MAX_PATH_LEN];
    ssize_t len;
    int written;
    
    if (!is_valid_pid(pid) || !buffer || size == 0) 
        return -1;
    
    written = snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    if (written < 0 || (size_t)written >= sizeof(path)) 
        return -1;
    
    len = readlink(path, buffer, size - 1);
    if (len > 0) {
        buffer[len] = '\0';
        return 0;
    }
    return -1;
}

static void get_user(char *user, size_t size) {
    struct passwd *pw = getpwuid(getuid());
    const char *env_user = getenv("USER");
    
    if (env_user && strlen(env_user) < MAX_ENV_LEN) {
        int valid = 1;
        for (size_t i = 0; env_user[i] != '\0'; i++) {
            if (!isalnum(env_user[i]) && env_user[i] != '_' && env_user[i] != '-') {
                valid = 0;
                break;
            }
        }
        if (valid) {
            safe_copy(user, env_user, size);
            return;
        }
    }
    
    if (pw && pw->pw_name) {
        safe_copy(user, pw->pw_name, size);
    } else {
        safe_copy(user, "user", size);
    }
}

static void get_host(char *host, size_t size) {
    if (gethostname(host, size) != 0) {
        safe_copy(host, "localhost", size);
    }
    host[size - 1] = '\0';
}

static void get_shell(char *shell, size_t size) {
    char path[PATH_BUF_SIZE];
    char exe_path[PATH_BUF_SIZE];
    pid_t parent = parent_pid(getpid());
    ssize_t len;
    int written;
    
    if (parent <= 0) {
        safe_copy(shell, "unknown", size);
        return;
    }
    
    written = snprintf(path, sizeof(path), "/proc/%d/exe", parent);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        safe_copy(shell, "unknown", size);
        return;
    }
    
    len = readlink(path, exe_path, sizeof(exe_path) - 1);
    if (len > 0 && len < (ssize_t)sizeof(exe_path)) {
        exe_path[len] = '\0';
        char *base = strrchr(exe_path, '/');
        const char *result = base ? base + 1 : exe_path;
        safe_copy(shell, result, size);
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
        "river", "wayfire", "labwc", "cage", "leftwm", "frankenwm", "2bwm",
        "catwm", "echinus", "evilwm", "qtile", "stumpwm", "notion", "snapwm",
        "howm", "wingo", "monsterwm", "tinywm", "dminiwm", "dtwm", "ctwm",
        "vwm", "9wm", "w9wm", "pawm", "pekwm", "fvwm", "enlightenment",
        "compiz", "metacity", "waimea", "afterstep", "windowmaker", "blackbox",
        "fvwm-crystal", "mlvwm", "olivetti", "plwm", "pwm", "ratpoison", "sithwm",
        "uwm", "vtwm", "wm2", "wmx", "larswm", "goomwwm", "mwwm", "acme",
        "scrotwm", "subtle", "berry", "howm", "lwm", "mwm", "nwm"
    };
    
    FILE *ps = popen("ps -eo comm --no-headers", "r");
    if (!ps) {
        safe_copy(wm, "unknown", size);
        return;
    }
    
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), ps)) {
        line[strcspn(line, "\n")] = '\0';
        
        if (strlen(line) == 0) 
            continue;
        
        for (size_t i = 0; i < sizeof(known_wms) / sizeof(*known_wms); i++) {
            if (strcmp(line, known_wms[i]) == 0) {
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

static int is_shell_process(const char *name) {
    return (strcmp(name, "bash") == 0 || 
            strcmp(name, "zsh") == 0 || 
            strcmp(name, "sh") == 0 ||
            strcmp(name, "fish") == 0 ||
            strcmp(name, "dash") == 0);
}

static void get_terminal(char *term, size_t size) {
    pid_t parent = parent_pid(getpid());
    
    while (parent > 0) {
        char name[PATH_BUF_SIZE];
        if (process_executable(parent, name, sizeof(name)) != 0) {
            parent = parent_pid(parent);
            continue;
        }
        
        char *base = strrchr(name, '/');
        base = base ? base + 1 : name;
        
        if (is_shell_process(base)) {
            parent = parent_pid(parent);
            continue;
        }
        
        struct { 
            const char *match; 
            const char *name; 
        } terminals[] = {
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
            {"ghostty", "ghostty"},
            {"contour", "contour"},
            {"tabby", "tabby"},
            {"hyper", "hyper"},
            {"yakuake", "yakuake"},
            {"roxterm", "roxterm"},
            {"sakura", "sakura"},
            {"tilda", "tilda"},
            {"guake", "guake"},
            {"cool-retro-term", "cool-retro-term"},
            {"eterm", "eterm"},
            {"mlterm", "mlterm"},
            {"lilyterm", "lilyterm"},
            {"roxterm", "roxterm"},
            {"aterm", "aterm"},
            {"evilvte", "evilvte"},
            {"pangoterm", "pangoterm"},
            {"valaterm", "valaterm"},
            {"stterm", "stterm"},
            {"xvt", "xvt"},
            {"nxterm", "nxterm"},
            {"kterm", "kterm"},
            {"mlterm", "mlterm"},
            {"dtterm", "dtterm"},
            {"mrxvt", "mrxvt"},
            {"multi-gnome-terminal", "multi-gnome-terminal"},
            {"pterm", "pterm"},
            {"wterm", "wterm"},
            {"yaft", "yaft"},
            {"fbterm", "fbterm"},
            {"koi8rxterm", "koi8rxterm"}
        };
        
        for (size_t i = 0; i < sizeof(terminals) / sizeof(*terminals); i++) {
            if (strcmp(base, terminals[i].match) == 0) {
                safe_copy(term, terminals[i].name, size);
                return;
            }
        }
        
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
    
    char line[MAX_LINE_LEN];
    int found = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *start = strchr(line, '"');
            char *end = strrchr(line, '"');
            
            if (start && end && start < end) {
                *end = '\0';
                safe_copy(os, start + 1, size);
                found = 1;
            }
            break;
        }
    }
    
    if (!found) {
        safe_copy(os, "Linux", size);
    }
    
    fclose(file);
}

static void print_separator(size_t length) {
    for (size_t i = 0; i < length; i++) {
        putchar('-');
    }
    putchar('\n');
}

static void display_info(const char *user, const char *host, const char *os, 
                        const char *kernel, const char *shell, 
                        const char *wm, const char *terminal) {
    size_t user_host_length = strlen(user) + strlen(host) + 1;
    
    printf("%s    ___ %s     %s%s@%s%s\n", 
           COLOR_1, COLOR_RESET, COLOR_1, user, host, COLOR_RESET);
    printf("%s   (%s.· %s|%s     ", 
           COLOR_1, COLOR_2, COLOR_1, COLOR_RESET);
    print_separator(user_host_length);
    
    printf("%s   (%s<>%s %s|%s     %sOS:%s %s\n", 
           COLOR_1, COLOR_2, COLOR_RESET, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, os);
    printf("%s  / %s__  %s\\%s    %sKernel:%s %s\n", 
           COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, kernel);
    printf("%s ( %s/  \\ %s/|%s   %sShell:%s %s\n", 
           COLOR_1, COLOR_2, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, shell);
    printf("%s%s_/%s\\ %s__)%s/%s_%s)%s   %sWM:%s %s\n", 
           COLOR_1, COLOR_3, COLOR_1, COLOR_2, COLOR_1, COLOR_3, COLOR_1, COLOR_RESET, COLOR_3, COLOR_RESET, wm);
    printf("%s%s\\/%s-____%s\\/%s    %sTerminal:%s %s\n\n", 
           COLOR_1, COLOR_3, COLOR_1, COLOR_3, COLOR_RESET, COLOR_3, COLOR_RESET, terminal);
}

int main(void) {
    char user[MAX_ENV_LEN];
    char host[MAX_ENV_LEN];
    char shell[MAX_ENV_LEN];
    char wm[MAX_ENV_LEN];
    char terminal[MAX_ENV_LEN];
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
