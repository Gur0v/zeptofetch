#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include "system_info.h"
#include "ui.h"
#include "config.h"

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
    get_shell(shell, sizeof(shell), strstr(distro, "FreeBSD") ? "FreeBSD" : "Linux");
    get_wm(wm, sizeof(wm));
    get_term(terminal, sizeof(terminal));

    print_ui(username, hostname, distro, sys_info.release, shell, wm, terminal);

    return EXIT_SUCCESS;
}
