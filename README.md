<div align="center">

# zeptofetch

![zeptofetch v2.0](https://i.e-z.host/a4t4aoly.webp)

A system information tool for Linux. Written in C, no dependencies, runs in under 1ms.

</div>

## Benchmarks
Tested with [hyperfine](https://github.com/sharkdp/hyperfine).

| Tool | Mean | Binary |
|---|---|---|
| **zeptofetch** | **571.7 µs ± 44.8 µs** | 27 KB |
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

Run locally with:
```bash
make clean && make
hyperfine -N --warmup 5 ./zeptofetch
```

<sub>Results vary by system.</sub>

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

## How it works

**Shell:** walks `/proc/[pid]/exe` up the process tree. Falls back to `$SHELL`.

**Terminal:** checks common terminal env hints, then walks the process tree while skipping shells/wrappers.

**WM:** checks compositor env hints, then scans numeric `/proc` entries against a known list.

**OS:** reads `PRETTY_NAME` from `/etc/os-release` or `/usr/lib/os-release`, falls back to `NAME`, then `"Linux"`.

**WSL2:** checked via `WSLENV`, `/mnt/wsl`, `binfmt_misc`, and kernel version strings.

**Privileges:** `PR_SET_NO_NEW_PRIVS` and `PR_SET_DUMPABLE` set at startup. `RLIMIT_AS`, `RLIMIT_CPU`, `RLIMIT_NOFILE`, and `RLIMIT_CORE` enforced.

> For accuracy/speed tradeoffs, see [zeptofetch-u](https://github.com/Gur0v/zeptofetch-u), a more stripped-down variant that runs in ~200 µs.

## Requirements
- Linux x86_64, kernel 2.6.32+
- glibc, musl, or compatible
- GCC or Clang

WSL2 support is limited. BSD, macOS, and Termux are not supported.

## License
[GPL-3.0](LICENSE)
