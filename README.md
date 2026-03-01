<div align="center">

# ⚡ zeptofetch

*Blazingly fast, ultra-minimal system information tool for Linux*

![zeptofetch v1.17-rc1](https://i.e-z.host/a4t4aoly.webp)

[Features](#features) • [Benchmarks](#benchmarks) • [Installation](#installation) • [Usage](#usage) • [Configuration](#configuration)

</div>

## 🎯 Why zeptofetch?
`zeptofetch` is a minimal system information tool written in C. It executes in under 1ms* with a 32KB binary—no dependencies, no shell scripts.

Compared to neofetch (~400ms) and fastfetch (~5ms, 200KB+ binary), zeptofetch prioritizes speed and minimal footprint.

<sub>*Performance varies by hardware and system configuration. See [benchmarks](#benchmarks) for details.</sub>

## ✨ Features
**Performance**
- ⚡ ~0.73 ms execution time (553× faster than neofetch)
- 📦 32 KB binary size
- 💾 Zero external dependencies

**Design**
- 🎨 Customizable colors via `config.h` at compile time
- 🔒 Security hardening with memory protection and resource limits
- 🐧 Pure C99 with efficient process scanning
- ⚙️ Memory-mapped process cache with secure cleanup

## 📊 Benchmarks
Tested with [hyperfine](https://github.com/sharkdp/hyperfine) on the following system:

<details>
<summary><b>Test System Specifications</b></summary>

| Component | Specification |
|-----------|--------------|
| **CPU** | AMD Ryzen 5 5600 (6-core, 12-thread) |
| **RAM** | 32 GB DDR4 @ 3200MHz |
| **Kernel** | 6.17.7-lqx1-1-lqx |
| **OS** | Arch Linux |
| **Desktop** | KDE Plasma 6.5.2 (Wayland) |

</details>

| Tool           | Runtime*              | Binary Size | Speed vs neofetch     |
|----------------|-----------------------|-------------|-----------------------|
| **zeptofetch** | **732.4µs ± 77.7µs**  | **32 KB**   | `553x faster`         |
| fastfetch      | 6.7ms ± 1.0ms         | ~200 KB     | `61x faster`          |
| neofetch       | 405.1ms ± 21.0ms      | ~50 KB      | *baseline*            |

<sub>*Performance varies based on hardware, system load, kernel, desktop environment, and terminal emulator. Benchmarks shown are from a statically-linked zeptofetch v1.9 binary.</sub>

## 🚀 Installation

### Manual Compilation

To build the latest version from source:
```bash
git clone https://gitlab.archlinux.org/gurov/zeptofetch.git
cd zeptofetch
make
sudo make install
```

This builds from the main branch, which may include release candidates. For production use, download a tagged release from the [releases page](https://gitlab.archlinux.org/gurov/zeptofetch/-/releases).

### Arch Linux (AUR)
```bash
paru -S zeptofetch        # Latest stable release
paru -S zeptofetch-git    # Development version
```

## 📖 Usage

```bash
# Display system information
zeptofetch

# Show version and build information
zeptofetch --version
zeptofetch -v
```

The `--version` flag displays:
- Version number and license information
- Build date, time, and compiler used
- Compact configuration stats (Cache size, chain depth, PID max, timeouts)

## ⚙️ Configuration

Customize colors by editing `config.h`.
*Note: Macros use concise naming to match the source code.*

```c
#define CR "\033[0m"        // Reset
#define C1 "\033[1;34m"     // Primary Color (Bold Blue)
#define C2 "\033[1;37m"     // Secondary Color (Bold White)
#define C3 "\033[38;5;208m" // Accent Color (Bold Orange)
```

Rebuild after changes:

```bash
make clean && make
sudo make install
```

## 🔍 Technical Details

### Implementation

- Compact source with minimal symbol overhead
- mmap-based cache for process information (256 entry limit)
- Direct `/proc` reads without subprocess spawning
- Pre-computed string lengths for O(1) prefix matching
- Stack allocations with compile-time size limits

### Detection

- **OS**: Single-pass `/etc/os-release` parsing with `PRETTY_NAME` fallback
- **Shell**: Process tree walk via `/proc/[pid]/exe` symlinks with `$SHELL` fallback
- **Terminal**: Ancestor scanning with `TERM_PROGRAM`/`TERMINAL` environment checks
- **WM**: `/proc` scan (PIDs 300-100000, max 1000 checks) with result caching
- **WSL**: Multi-method detection (`WSLENV`, `/mnt/wsl`, `binfmt_misc`, kernel strings)
- **Kernel**: `uname()` syscall with error handling

## 🐧 Requirements

- **Platform**: Linux (x86_64)
- **Kernel**: 2.6.32+ (any modern kernel)
- **libc**: glibc, musl, or compatible
- **Build**: GCC or Clang

**Supported**: Linux, WSL1, WSL2.
**Not supported**: Termux (Android)*, BSD, macOS.

<sub>*Termux lacks full `/proc` support and imposes strict memory mapping restrictions that make porting non-trivial. While the program may compile, it will not run correctly.</sub>

## 🤝 Contributing

This project is developed on the **Arch Linux GitLab**:

👉 https://gitlab.archlinux.org/gurov/zeptofetch

The GitHub repository is a read-only mirror for visibility.

**How to contribute:**
- Report bugs or suggest features
- Improve documentation
- Submit merge requests
- Test on different distributions

All issues and merge requests should be submitted on the Arch Linux GitLab.

## 📜 License

Licensed under [GPL-3.0](LICENSE)

---

<div align="center">

**Made with ⚡ by [Gurov](https://gitlab.archlinux.org/gurov)**

[⬆ Back to top](#-zeptofetch)

</div>
