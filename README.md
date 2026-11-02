<div align="center">

# zeptofetch

![zeptofetch v2.0](https://i.e-z.host/a4t4aoly.webp)

A system information tool for Linux and FreeBSD. Written in C, no dependencies, runs in under 1ms.

</div>

## Benchmarks
Tested with [hyperfine](https://github.com/sharkdp/hyperfine).

| Tool | Mean | Binary |
|---|---|---|
| **zeptofetch** | **506.9 µs ± 53.3 µs** | 32 KB |
| fastfetch | 5.6 ms ± 0.4 ms | ~200 KB |
| neofetch | 194.3 ms ± 24.8 ms | ~50 KB |

<details>
<summary>Test system</summary>

| | |
|---|---|
| CPU | AMD Ryzen 5 5600 (12) @ 4.47 GHz |
| RAM | 32 GB (31.26 GiB) |
| Kernel | 7.0.14-lqx2-2-lqx |
| OS | Artix Linux |
| Desktop | Sway 1.12 (Wayland) |

</details>

Run locally with:
```bash
make clean && make NATIVE=-march=native
hyperfine -N --warmup 5 ./zeptofetch
```

<sub>Results vary by system.</sub>

## Installation
**From source:**
```bash
git clone https://gitlab.archlinux.org/gurov/zeptofetch.git
cd zeptofetch
make NATIVE=-march=native
sudo make install
```
**FreeBSD:** the base-system Clang and `make` are sufficient; use the same source build commands above. No procfs mount or extra libraries are needed.

**AUR:**
```bash
paru -S zeptofetch      # stable
paru -S zeptofetch-git  # git HEAD
```
**Nix (flake):**
```bash
nix profile install github:Gur0v/zeptofetch
```

## How it works

**Shell:** walks `/proc/[pid]/exe` up the process tree. On FreeBSD, uses `sysctl(KERN_PROC_PID)` and `KERN_PROC_PATHNAME`, falling back to the process name when its executable path is unavailable. Falls back to `$SHELL`.

**Terminal:** checks specific terminal env hints, then known terminal ancestors, then nearest unknown non-shell/non-wrapper ancestor, then generic `$TERMINAL`.

**WM:** checks compositor env hints, then scans numeric `/proc` entries against a known list, restricted to current UID. FreeBSD uses `sysctl(KERN_PROC_RUID)` instead of `/proc`. Multiple compositors belonging to the same user, including those in separate sessions, remain ambiguous.

**OS:** reads `PRETTY_NAME` from `/etc/os-release` or `/usr/lib/os-release`, falls back to `NAME`, then `"Linux"` (or `"FreeBSD"` on FreeBSD). FreeBSD also checks `/var/run/os-release`.

**WSL2:** checked via `WSLENV`, `/mnt/wsl`, `binfmt_misc`, and kernel version strings.

**FreeBSD sessions:** detects virtual consoles via `ttyname` and jails via `security.jail.jailed` (displayed as `container`).

**Privileges:** `PR_SET_NO_NEW_PRIVS` and `PR_SET_DUMPABLE` set at startup on Linux. FreeBSD uses `procctl(PROC_TRACE_CTL)` and, when available, `PROC_NO_NEW_PRIVS_CTL`. `RLIMIT_CPU`, `RLIMIT_NOFILE`, and `RLIMIT_CORE` capped independently without increasing inherited limits. Failures produce named warnings. Address space is not limited, allowing sanitizer and allocator reservations.

## Requirements
- Linux x86_64, kernel 2.6.32+
- FreeBSD 13+ (native libc; no procfs required; tested on 15.1)
- On Linux: glibc, musl, or compatible
- GCC or Clang

WSL2 support is limited. Other BSDs, macOS, and Termux are not supported.

The Nix build skips `-march=native` (not reproducible) and patches out the `RLIMIT_AS` hardening limit (deadlocks under scudo).

## License
[GPL-3.0](LICENSE)
