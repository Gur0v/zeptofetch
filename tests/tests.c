#define ZEPTOFETCH_TEST 1
#define main zeptofetch_main
#include "../zeptofetch.c"
#undef main
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

int main(int argc, char **argv)
{
    int readers_only = argc == 2 && strcmp(argv[1], "--readers-only") == 0;
    assert(argc == 1 || readers_only);
#if !defined(__FreeBSD__)
    assert(parse_proc_stat("42 (name\nwith ) parens) S 1 0\n") == 1);
    assert(parse_proc_stat("42 (x) S 4194304 0\n") == MAX_PID);
    assert(parse_proc_stat("42 (x) S 4194305 0\n") == -1);
    assert(parse_proc_stat("42 (x) S 4294967297 0\n") == -1);
    assert(parse_proc_stat("42 (x) S 1junk 0\n") == -1);
    assert(parse_proc_stat("42 (x) S -1 0\n") == -1);
    assert(parse_proc_stat("42 (x) S 999999999999999999999999 0\n") == -1);
    assert(parse_proc_stat("42 (x)") == -1);
#endif
    char safe[32];
    copy_text(safe, "Jos\303\251\033[31m", sizeof(safe));
    assert(strcmp(safe, "Jos\303\251_[31m") == 0);
    copy_text(safe, "A\302\233B", sizeof(safe));
    assert(strcmp(safe, "A_B") == 0);
    copy_text(safe, "\360\237\230\200", 4);
    assert(!safe[0]);
    copy_text(safe, "\355\240\200", sizeof(safe));
    assert(strcmp(safe, "_") == 0);
    strcpy(safe, "Jos\303\251");
    copy_text(safe, safe, sizeof(safe));
    assert(strcmp(safe, "Jos\303\251") == 0);
    FILE *cg = tmpfile();
    assert(cg);
    assert(fputs("1:name=/user\n2:cpu=/docker/container\n", cg) >= 0);
    rewind(cg);
    assert(cgroup_container(cg));
    fclose(cg);
    if (setlocale(LC_CTYPE, "C.UTF-8")) {
        assert(text_width("Jos\303\251") == 4);
        assert(text_width("\344\270\255") == 2);
        assert(text_width("e\314\201") == 1);
    }
    SystemInfo launcher = {0};
    char candidate[MAX_NAME] = "";
    match_ancestor(&launcher, candidate, "/usr/bin/flatpak-spawn");
    match_ancestor(&launcher, candidate, "/tmp/launch.sh");
    assert(!candidate[0]);
    match_ancestor(&launcher, candidate, "/usr/bin/kitty");
    assert(strcmp(launcher.term, "kitty") == 0);
    setenv("SWAYSOCK", "sway", 1);
    setenv("HYPRLAND_CMD", "Hyprland", 1);
    fetch_wm(safe, sizeof(safe));
    assert(strcmp(safe, "sway") == 0);
    unsetenv("SWAYSOCK");
    fetch_wm(safe, sizeof(safe));
    assert(strcmp(safe, "Hyprland") == 0);
    unsetenv("HYPRLAND_CMD");
    char out[MAX_LINE];
    const char *hints[] = { "TERM_PROGRAM", "KITTY_PID", "KITTY_INSTALLATION_DIR", "ALACRITTY_SOCKET", "ALACRITTY_LOG", "ALACRITTY_WINDOW_ID", "WEZTERM_EXECUTABLE", "WEZTERM_PANE", "TERMUX_VERSION", "TERMUX_MAIN_PACKAGE_FORMAT", "KONSOLE_VERSION", "GNOME_TERMINAL_SCREEN", "GNOME_TERMINAL_SERVICE", "LC_TERMINAL" };
    for (size_t i = 0; i < ARRLEN(hints); i++) unsetenv(hints[i]);
    setenv("KITTY_PID", "", 1);
    setenv("TERM_PROGRAM", "example", 1);
    char terminal[MAX_NAME] = "";
    term_hint(terminal, sizeof(terminal));
    assert(strcmp(terminal, "example") == 0);
    setenv("KITTY_PID", "42", 1);
    term_hint(terminal, sizeof(terminal));
    assert(strcmp(terminal, "kitty") == 0);
    unsetenv("KITTY_PID");
    unsetenv("TERM_PROGRAM");
    setenv("TERMINAL", "generic", 1);
    SystemInfo info = {0};
    char fallback[MAX_NAME] = "";
    assert(find_match("osh", shells, ARRLEN(shells)));
    assert(find_match("ysh", shells, ARRLEN(shells)));
    assert(find_match("kgx", terms, ARRLEN(terms)));
    assert(!find_match("kgxfoo", terms, ARRLEN(terms)));
    assert(find_match("cosmic-comp", wms, ARRLEN(wms)));
    assert(find_match("dwl", wms, ARRLEN(wms)));
    assert(strcmp(find_match(".awesome-real", wms, ARRLEN(wms)), "awesome") == 0);
    assert(!find_match(".awesomefoo", wms, ARRLEN(wms)));
    assert(strcmp(find_match("oil.ovm", shells, ARRLEN(shells)), "oil") == 0);
    assert(strcmp(find_match("fvwm-crystal", wms, ARRLEN(wms)), "fvwm") == 0);
    match_ancestor(&info, fallback, "/bin/unknown-parent");
    assert(!info.term[0] && strcmp(fallback, "unknown-parent") == 0);
    match_ancestor(&info, fallback, "/bin/kitty");
    assert(strcmp(info.term, "kitty") == 0);
    match_ancestor(&info, fallback, "/bin/nu");
    assert(strcmp(info.shell, "nushell") == 0);
    assert(strcmp(info.term, "kitty") == 0);
    assert(strcmp(env_value("TERMINAL"), "generic") == 0);
    assert(setenv("SWAYSOCK", "", 1) == 0);
    assert(!env_value("SWAYSOCK"));
    unsetenv("SWAYSOCK");
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
#if !defined(__FreeBSD__)
    proc_path(getpid(), "", path, sizeof(path));
    struct stat st;
    assert(stat(path, &st) == 0);
    assert(S_ISDIR(st.st_mode) && st.st_uid == getuid());
    proc_path(0, "", path, sizeof(path));
    assert(!*path);
    proc_path(getpid(), "../stat", path, sizeof(path));
    assert(!*path);
#endif
    Process self;
    assert(read_proc(getpid(), &self, 1) == 0);
    assert(self.ppid == getppid());
    assert(self.exe[0]);
    assert(read_proc(getpid(), &self, 0) == 0);
    assert(self.ppid == getppid());
    assert(!self.exe[0]);
    assert(read_proc(0, &self, 1) == -1);
    assert(read_proc(getpid(), NULL, 1) == -1);
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
    assert(!out[0]);
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
