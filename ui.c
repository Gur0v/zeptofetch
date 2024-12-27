#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ui.h"
#include "config.h"

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
