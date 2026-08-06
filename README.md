<div align="center">

# zeptofetch

![zeptofetch v2.0](https://i.e-z.host/a4t4aoly.webp)

A system information tool for Linux. Written in C, no dependencies, runs in under 1ms.

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

**Shell:** walks `/proc/[pid]/exe` up the process tree. Falls back to `$SHELL`.

**Terminal:** checks common terminal env hints, then walks the process tree while skipping shells/wrappers.

**WM:** checks compositor env hints, then scans numeric `/proc` entries against a known list.

**OS:** reads `PRETTY_NAME` from `/etc/os-release` or `/usr/lib/os-release`, falls back to `NAME`, then `"Linux"`.

**WSL2:** checked via `WSLENV`, `/mnt/wsl`, `binfmt_misc`, and kernel version strings.

**Privileges:** `PR_SET_NO_NEW_PRIVS` and `PR_SET_DUMPABLE` set at startup. `RLIMIT_AS`, `RLIMIT_CPU`, `RLIMIT_NOFILE`, and `RLIMIT_CORE` enforced.

## Requirements
- Linux x86_64, kernel 2.6.32+
- glibc, musl, or compatible
- GCC or Clang

WSL2 support is limited. BSD, macOS, and Termux are not supported.

The Nix build skips `-march=native` (not reproducible) and patches out the `RLIMIT_AS` hardening limit (deadlocks under scudo).

## License
[GPL-3.0](LICENSE)
