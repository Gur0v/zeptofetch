<div align="center">

# zeptofetch

A small, fast system information tool for Linux and FreeBSD.

![zeptofetch screenshot](https://i.e-z.host/a4t4aoly.webp)

Written in C. No runtime dependencies beyond libc.

</div>

## Benchmarks

Measured with [hyperfine](https://github.com/sharkdp/hyperfine): 200 runs per tool after 20 warmups, with output redirected.

| Tool | Average time | File size |
|---|---:|---:|
| **zeptofetch** | **0.93 ms** | **36 KB** |
| fastfetch | 14.29 ms | 2.2 MB |
| neofetch | 212.32 ms | 387 KB |

<sub>The tools show different information, so they do different amounts of work. Sizes exclude libraries and interpreters; neofetch is a shell script.</sub>

<details>
<summary>Test system and method</summary>

| | |
|---|---|
| CPU | AMD Ryzen 5 5600 |
| Memory | 32 GB |
| System | Void Linux musl, kernel 7.3.5_1 |
| Desktop | Sway/Wayland |
| Zeptofetch | Revision `ac8bc8f`; GCC 14.2.1, `-O2 -flto -march=native`, stripped |
| Comparison | Fastfetch 2.71.0-debug and neofetch 7.1.0, both with `--config none` |

</details>

## Installation

### From source

```sh
git clone https://github.com/Gur0v/zeptofetch.git
cd zeptofetch
make NATIVE=-march=native
sudo make install
```

Omit `NATIVE=-march=native` for a portable build.

On FreeBSD, the base-system Clang and `make` are sufficient. No procfs mount is required.

### Arch Linux

```sh
paru -S zeptofetch
```

For the latest Git version:

```sh
paru -S zeptofetch-git
```

### Nix

```sh
nix profile install github:Gur0v/zeptofetch
```

## Usage

```sh
zeptofetch
```

Show version and build information:

```sh
zeptofetch --version
```

## How detection works

- **Shell:** Walks the parent process tree, then falls back to `$SHELL`.
- **Terminal:** Checks terminal-specific environment hints and known parent processes, then falls back to `$TERMINAL`, `$TERM`, or an unrecognized parent.
- **Window manager:** Checks compositor hints, then scans processes owned by the current user. Multiple sessions under one user can make the result ambiguous.
- **OS:** Reads `PRETTY_NAME` or `NAME` from available `os-release` files.
- **Sessions:** Detects Linux virtual consoles and containers, FreeBSD virtual consoles and jails, SSH, and WSL. SSH and containers with display hints also attempt graphical-session detection.

Linux uses procfs for process detection. If procfs is unavailable, zeptofetch still prints the information it can obtain. FreeBSD uses native `sysctl` interfaces and does not require procfs.

## Requirements

- Linux x86_64 with kernel 2.6.32 or newer; glibc, musl, or compatible libc
- FreeBSD 13 or newer
- GCC or Clang to build

Other BSDs, macOS, and Termux are not supported.

## License

[GPL-3.0](LICENSE)
