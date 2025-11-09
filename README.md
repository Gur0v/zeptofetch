<div align="center">

# ⚡ zeptofetch

*Blazingly fast, ultra-minimal system information tool for Linux*

[![License](https://img.shields.io/github/license/Gur0v/zeptofetch)](LICENSE)
[![Stars](https://img.shields.io/github/stars/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/stargazers)
[![Platform](https://img.shields.io/badge/platform-linux-blue)](#compatibility)
[![Version](https://img.shields.io/github/v/release/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/releases)
[![Issues](https://img.shields.io/github/issues/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/issues)
[![Pull Requests](https://img.shields.io/github/issues-pr/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/pulls)

![zeptofetch v1.5](https://roblo-x.com/1kvwoscj.webp)

[Features](#features) • [Benchmarks](#benchmarks) • [Installation](#installation) • [Usage](#usage) • [Configuration](#configuration)

</div>

## 🎯 Why zeptofetch?
`zeptofetch` delivers system information in **under 1ms*** with a **28KB binary***. No scripts, no bloat, just pure C doing exactly what you need.
While tools like neofetch take over 400ms and fastfetch needs 200KB+ binaries, zeptofetch gives you information 480x faster* with a fraction of the size.

<sub>*_Performance varies by hardware and system configuration. See [benchmarks](#benchmarks) for details._</sub>

## ✨ Features
**Fast & Lightweight**
- ⚡ Runs in ~0.8 ms (445× faster than neofetch)
- 📦 Only 28 KB in size
- 💾 No dependencies needed

**Customizable & Reliable**
- 🎨 Easy color customization via `config.h`
- 🐧 Written in pure C
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

| Tool           | Runtime*            | Binary Size | Speed vs neofetch     |
|----------------|---------------------|-------------|-----------------------|
| **zeptofetch** | **0.80ms ± 0.09ms** | **28 KB**   | `480x faster`         |
| fastfetch      | 3.6ms ± 0.4ms       | ~200 KB     | `107x faster`         |
| neofetch       | 384.1ms ± 6.7ms     | ~50 KB      | *baseline*            |

<sub>*_Runtime performance is highly dependent on hardware, system load, and configuration. The kernel, scheduler, desktop environment (DE), window manager (WM), and terminal emulator can all influence speed, leading to different results on your system. The benchmark results shown are from a prebuilt binary version v1.8. While prebuilt releases may sometimes offer competitive runtime speeds, it often feels like a 50/50 chance. Manual compilation is generally safer, yielding more consistent performance optimized for your specific configuration._</sub>

## 🚀 Installation

### Recommended: Manual Compilation

For best performance and smallest binary size, compile manually with your system's native optimizations:

```bash
git clone https://github.com/Gur0v/zeptofetch
cd zeptofetch
make
sudo make install
```

This builds with `-march=native` for maximum performance on your specific CPU.

### Arch Linux (AUR)

For Arch Linux users, zeptofetch is available in the AUR:

```bash
# Using paru
paru -S zeptofetch        # Build from source
paru -S zeptofetch-bin    # Prebuilt binary
paru -S zeptofetch-git    # Latest from git

# Using yay
yay -S zeptofetch         # Build from source
yay -S zeptofetch-bin     # Prebuilt binary
yay -S zeptofetch-git     # Latest from git
```

### Alternative: Prebuilt Releases

Prebuilt binaries are available on the [Releases](https://github.com/Gur0v/zeptofetch/releases) page. These are primarily intended for packaging and distribution, compiled with `-march=x86-64-v3` for broader compatibility.

**Manual compilation is strongly recommended** for optimal performance.

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
- Target architecture
- Configuration constants (cache size, limits)

## ⚙️ Configuration

Customize colors by editing `config.h`:

```c
#define COLOR_1 "\033[1;34m"     // Bold Blue
#define COLOR_2 "\033[1;37m"     // Bold White
#define COLOR_3 "\033[38;5;208m" // Bold Orange
#define COLOR_RESET "\033[0m"    // Normal
```

Rebuild after changes:

```bash
make clean && make
sudo make install
```

## 🔍 Technical Details

### Architecture Highlights

- **Smart caching**: Process chain information cached during runtime
- **Direct /proc access**: No subprocess spawning or shell execution
- **Safe string handling**: All operations bounds-checked
- **Optimized lookups**: First-character filtering before string comparisons
- **Memory efficient**: Fixed-size stack allocations, predictable memory usage

### Detection Methods

- **OS**: Parses `/etc/os-release` in single pass
- **Shell**: Walks parent process chain via `/proc/[pid]/exe`
- **Terminal**: Scans process ancestry for known terminal emulators
- **WM**: Direct `/proc` scanning with result caching
- **Kernel**: Uses `uname()` syscall

## 🐧 Requirements

- **Platform**: Linux (x86_64)
- **Kernel**: 2.6.32+ (any modern kernel)
- **libc**: glibc, musl, or compatible
- **Build**: GCC or Clang

**Partial support**: WSL (supported but may provide inaccurate information as zeptofetch is not specifically designed for it)

**Not supported**: Android, BSD, macOS

## 🤝 Contributing

Contributions are welcome! Here's how you can help:

- 🐛 [Report bugs](https://github.com/Gur0v/zeptofetch/issues)
- 💡 [Suggest features](https://github.com/Gur0v/zeptofetch/issues)
- 📝 Improve documentation
- 🔧 Submit pull requests
- 🧪 Test on new distributions

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting PRs.

## 📜 License

Licensed under [GPL-3.0](LICENSE)

---

<div align="center">

**Made with ⚡ by [Gur0v](https://github.com/Gur0v)**

[⬆ Back to top](#-zeptofetch)

</div>
