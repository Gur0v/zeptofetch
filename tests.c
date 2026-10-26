#define main zeptofetch_main
#include "zeptofetch.c"
#undef main
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

int main(int argc, char **argv)
{
    int readers_only = argc == 2 && strcmp(argv[1], "--readers-only") == 0;
    assert(argc == 1 || readers_only);
    assert(parse_proc_stat("42 (name\nwith ) parens) S 1 0\n") == 1);
    assert(parse_proc_stat("42 (x) S 4194304 0\n") == MAX_PID);
    assert(parse_proc_stat("42 (x) S 4194305 0\n") == -1);
    assert(parse_proc_stat("42 (x) S 4294967297 0\n") == -1);
    assert(parse_proc_stat("42 (x) S 1junk 0\n") == -1);
    assert(parse_proc_stat("42 (x) S -1 0\n") == -1);
    assert(parse_proc_stat("42 (x) S 999999999999999999999999 0\n") == -1);
    assert(parse_proc_stat("42 (x)") == -1);
    char out[MAX_LINE];
    const char *hints[] = { "TERM_PROGRAM", "KITTY_PID", "KITTY_INSTALLATION_DIR", "ALACRITTY_SOCKET", "ALACRITTY_LOG", "ALACRITTY_WINDOW_ID", "WEZTERM_EXECUTABLE", "WEZTERM_PANE", "TERMUX_VERSION", "TERMUX_MAIN_PACKAGE_FORMAT", "KONSOLE_VERSION", "GNOME_TERMINAL_SCREEN", "GNOME_TERMINAL_SERVICE", "LC_TERMINAL" };
    for (size_t i = 0; i < ARRLEN(hints); i++) unsetenv(hints[i]);
    setenv("TERMINAL", "generic", 1);
    proc_t chain[3] = { { .exe = "" }, { .exe = "/bin/unknown-parent" }, { .exe = "/bin/kitty" } };
    fetch_term(chain, 3, out, sizeof(out));
    assert(strcmp(out, "kitty") == 0);
    fetch_term(chain, 2, out, sizeof(out));
    assert(strcmp(out, "unknown-parent") == 0);
    fetch_term(chain, 1, out, sizeof(out));
    assert(strcmp(out, "generic") == 0);
    assert(parse_os_value("NAME=\"Test \\\"OS\\\"\"\n", "NAME", out, sizeof(out)));
    assert(strcmp(out, "Test \"OS\"") == 0);
    assert(parse_os_value("NAME='Test OS'\n", "NAME", out, sizeof(out)));
    assert(strcmp(out, "Test OS") == 0);
    assert(!parse_os_value("NAME=\"unterminated\n", "NAME", out, sizeof(out)));
    char dir[] = "/tmp/zeptofetch-test-XXXXXX";
    assert(mkdtemp(dir));
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/record", dir);
    int fd = open(path, O_CREAT | O_EXCL | O_RDWR, 0600);
    assert(fd >= 0);
    const char record[] = "42 (name\nwith ) parens) S 1 0\n";
    assert(write(fd, record, sizeof(record) - 1) == sizeof(record) - 1);
    close(fd);
    char buf[128];
    assert(read_record(path, buf, sizeof(buf)) == 0);
    assert(strcmp(buf, record) == 0);
    assert(read_record(path, buf, 8) == -1);
    assert(read_record(path, buf, sizeof(record)) == 0);
    assert(read_record(path, buf, sizeof(record) - 1) == -1);
    assert(read_record(path, buf, 1) == -1);
    assert(read_record(path, NULL, 0) == -1);
    assert(read_first_line(path, NULL, 0) == -1);
    assert(read_link(path, NULL, 0) == -1);
    assert(!parse_os_value("NAME=Linux", "NAME", NULL, 0));
    assert(!parse_os_value("NAME=Linux", "NAME", buf, 1));
    assert(parse_os_value("NAME=L", "NAME", buf, 2));
    assert(strcmp(buf, "L") == 0);
    assert(read_record(dir, buf, sizeof(buf)) == -1);
    proc_path(getpid(), "", path, sizeof(path));
    struct stat st;
    assert(stat(path, &st) == 0);
    assert(S_ISDIR(st.st_mode) && st.st_uid == getuid());
    proc_path(0, "", path, sizeof(path));
    assert(!*path);
    proc_path(getpid(), "../stat", path, sizeof(path));
    assert(!*path);
    snprintf(path, sizeof(path), "%s/record", dir);
    unlink(path);
    FILE *f = fopen(path, "w+");
    assert(f);
    assert(fputs("NAME=Fallback\nPRETTY_NAME=", f) >= 0);
    for (size_t i = 0; i < MAX_LINE * 4; i++) assert(fputc('x', f) != EOF);
    assert(fputs("\nPRETTY_NAME='Good OS'\n", f) >= 0);
    rewind(f);
    read_os(f, out, sizeof(out));
    assert(strcmp(out, "Good OS") == 0);
    fclose(f);
    unlink(path);
    f = fopen(dir, "r");
    assert(f);
    read_os(f, out, sizeof(out));
    assert(ferror(f));
    assert(strcmp(out, "Linux") == 0);
    fclose(f);
    assert(rmdir(dir) == 0);
    if (readers_only) return 0;
    struct rlimit before, after;
    assert(getrlimit(RLIMIT_NOFILE, &before) == 0);
    if (before.rlim_max > 64) before.rlim_max = 64;
    if (before.rlim_cur > 32) before.rlim_cur = 32;
    if (before.rlim_cur > before.rlim_max) before.rlim_cur = before.rlim_max;
    assert(setrlimit(RLIMIT_NOFILE, &before) == 0);
    set_limits();
    assert(getrlimit(RLIMIT_NOFILE, &after) == 0);
    assert(after.rlim_cur == before.rlim_cur);
    assert(after.rlim_max == before.rlim_max);
    return 0;
}
