<div align="center">

# zeptofetch

![zeptofetch v1.18](https://i.e-z.host/a4t4aoly.webp)

A system information tool for Linux. Written in C, no dependencies, runs in under 1ms.

</div>

---

## Benchmarks

Tested with [hyperfine](https://github.com/sharkdp/hyperfine).

| Tool | Mean | Binary |
|---|---|---|
| **zeptofetch** | **732.4 µs ± 77.7 µs** | 32 KB |
| fastfetch | 6.7 ms ± 1.0 ms | ~200 KB |
| neofetch | 405.1 ms ± 21.0 ms | ~50 KB |

<details>
<summary>Test system</summary>

| | |
|---|---|
| CPU | AMD Ryzen 5 5600 |
| RAM | 32 GB DDR4 @ 3200 MHz |
| Kernel | 6.17.7-lqx1-1-lqx |
| OS | Arch Linux |
| Desktop | KDE Plasma 6.5.2 (Wayland) |

</details>

<sub>Benchmarks from a statically-linked v1.9 binary. Results vary by system.</sub>

---

## Installation

**From source:**
```bash
git clone https://gitlab.archlinux.org/gurov/zeptofetch.git
cd zeptofetch
make
sudo make install
```

**AUR:**
```bash
paru -S zeptofetch      # stable
paru -S zeptofetch-git  # git HEAD
```

---

## Configuration

Edit `config.h` before building:

```c
#define CR "\033[0m"         // reset
#define C1 "\033[1;34m"      // primary   (bold blue)
#define C2 "\033[1;37m"      // secondary (bold white)
#define C3 "\033[38;5;208m"  // accent    (bold orange)
```

```bash
make clean && make && sudo make install
```

---

## How it works

**Shell** — walks `/proc/[pid]/exe` up the process tree. Falls back to `$SHELL` if nothing matches.

**Terminal** — same tree walk, skipping known shells. Checks `TERM_PROGRAM` and `TERMINAL` first.

**WM** — single-pass scan of `/proc` across PIDs 300–100000, matched against a known list. Result is cached.

**OS** — reads `PRETTY_NAME` from `/etc/os-release`, falls back to `NAME`, then `"Linux"`.

**WSL** — checked via `WSLENV`, `/mnt/wsl`, `binfmt_misc`, and kernel version strings.

**Privileges** — `PR_SET_NO_NEW_PRIVS` and `PR_SET_DUMPABLE` set at startup. `RLIMIT_AS`, `RLIMIT_CPU`, and `RLIMIT_NOFILE` enforced. Process cache is `mmap`-allocated and zeroed on exit.

~700 lines of C99.

---

## Requirements

- Linux x86_64, kernel 2.6.32+
- glibc, musl, or compatible
- GCC or Clang

WSL1 and WSL2 are supported. BSD, macOS, and Termux are not.

---

## Contributing

Primary repo: https://gitlab.archlinux.org/gurov/zeptofetch

GitHub is a read-only mirror. Issues and merge requests go on GitLab.

---

## License

[GPL-3.0](LICENSE)