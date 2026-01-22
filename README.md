<div align="center">

# ⚡ zeptofetch

*Blazingly fast, ultra-minimal system information tool for Linux*

![zeptofetch v1.14](https://roblo-x.com/3fa4j3ar.webp)

[Features](#features) • [Benchmarks](#benchmarks) • [Installation](#installation) • [Usage](#usage) • [Configuration](#configuration)

</div>

## 🎯 Why zeptofetch?
`zeptofetch` delivers system information in **under 1ms*** with a **28KB binary**. No scripts, no bloat, just pure C doing exactly what you need.
While tools like neofetch take over 400ms and fastfetch needs 200KB+ binaries, zeptofetch gives you information 553x faster* with a fraction of the size.

<sub>*_Performance varies by hardware and system configuration. See [benchmarks](#benchmarks) for details._</sub>

## ✨ Features
**Fast & Lightweight**
- ⚡ Runs in ~0.73 ms (553× faster than neofetch)
- 📦 Only 28 KB in size
- 💾 No dependencies needed

**Modern & Reliable**
- 🎨 Easy color customization via `config.h`
- 🪟 **Native WSL Support** (Windows Terminal & WSLg detection)
- 🐧 Written in pure, minified C (C99)
- ✅ Production ready and tested

## 📊 Benchmarks
Tested with [hyperfine](https://github.com/sharkdp/hyperfine) on the following system:

<details>
<summary><b>Test System Specifications</b></summary>

| Component | Specification |
|-----------|--------------|
| **CPU** | AMD Ryzen 5 5600 (6-core, 12-thread) |
| **GPU** | AMD Radeon RX 7600 |
| **RAM** | 32 GB DDR4 @ 3200MHz |
| **Storage** | 931.51 GB SSD |
| **Kernel** | 6.17.7-lqx1-1-lqx |
| **OS** | Arch Linux |
| **Desktop** | KDE Plasma 6.5.2 (Wayland) |
| **Motherboard** | MSI B550-A PRO (MS-7C56) |

</details>

| Tool           | Runtime*              | Binary Size | Speed vs neofetch     |
|----------------|-----------------------|-------------|-----------------------|
| **zeptofetch** | **732.4µs ± 77.7µs**  | **28 KB**   | `553x faster`         |
| fastfetch      | 6.7ms ± 1.0ms         | ~200 KB     | `61x faster`          |
| neofetch       | 405.1ms ± 21.0ms      | ~50 KB      | *baseline*            |

<sub>*_Performance varies based on hardware, system load, CPU scheduler, kernel, desktop environment, and terminal emulator._</sub>

## 🚀 Installation

### Manual Compilation

For best performance and smallest binary size, compile manually with your system's native optimizations:

```bash
git clone https://gitlab.archlinux.org/gurov/zeptofetch.git
cd zeptofetch
make
sudo make install
```

### Arch Linux (AUR)

For Arch Linux users, zeptofetch is available in the AUR:

```bash
paru -S zeptofetch        # Build from source
paru -S zeptofetch-git    # Latest from git
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

### Architecture Highlights

- **Minified Source**: Aggressive symbol reduction for cleaner, smaller code.
- **Smart caching**: `mmap` based allocation for process chain information.
- **Direct /proc access**: No subprocess spawning or shell execution.
- **Optimized lookups**: Unified list matchers with length pre-checks.
- **Memory efficient**: Fixed-size stack allocations, predictable memory usage.

### Detection Methods

- **OS**: Parses `/etc/os-release` in a single pass.
- **Shell**: Walks parent process chain via `/proc/[pid]/exe`.
- **Terminal**: Scans ancestry for known terminals; detects `WT_SESSION` for Windows Terminal.
- **WM**: Direct `/proc` scanning with caching; detects `WAYLAND_DISPLAY` for WSLg.
- **Kernel**: Uses `uname()` syscall.

## 🐧 Requirements

- **Platform**: Linux (x86_64)
- **Kernel**: 2.6.32+ (any modern kernel)
- **libc**: glibc, musl, or compatible
- **Build**: GCC or Clang

**Supported**: Linux, WSL1, WSL2 (Native detection).
**Not supported**: Android, BSD, macOS.

## 🤝 Contributing

Contributions are welcome! Here's how you can help:

- 🐛 [Report bugs](https://gitlab.archlinux.org/gurov/zeptofetch/-/issues)
- 💡 [Suggest features](https://gitlab.archlinux.org/gurov/zeptofetch/-/issues)
- 📝 Improve documentation
- 🔧 Submit pull requests
- 🧪 Test on new distributions

## 📜 License

Licensed under [GPL-3.0](LICENSE)

---

<div align="center">

**Made with ⚡ by [Gurov](https://gitlab.archlinux.org/gurov)**

[⬆ Back to top](#-zeptofetch)

</div>