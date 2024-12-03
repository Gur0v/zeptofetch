#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <stddef.h>

void get_username(char *username, size_t size);
void get_hostname(char *hostname, size_t size);
void get_shell(char *shell, size_t size, const char *distro);
void get_wm(char *wm, size_t size);
void get_term(char *term, size_t size);
void get_distro(char *distro, size_t size);

#endif
