<div align="center">

# ⚡ zeptofetch

*Blazingly fast, ultra-minimal system information tool for Linux*

[![License](https://img.shields.io/github/license/Gur0v/zeptofetch)](LICENSE)
[![Stars](https://img.shields.io/github/stars/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/stargazers)
[![Platform](https://img.shields.io/badge/platform-linux-blue)](#compatibility)
[![Version](https://img.shields.io/github/v/release/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/releases)
[![Issues](https://img.shields.io/github/issues/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/issues)
[![Pull Requests](https://img.shields.io/github/issues-pr/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/pulls)

![zeptofetch v1.1](https://roblo-x.com/6b9offkd.webp)

[Features](#features) • [Benchmarks](#benchmarks) • [Installation](#installation) • [Usage](#usage) • [Configuration](#configuration)

</div>

## 🎯 Why zeptofetch?

`zeptofetch` delivers system information in **under 2ms*** with a **28KB binary***. No scripts, no bloat, just pure C doing exactly what you need.

While tools like neofetch take over 400ms and fastfetch needs 200KB+ binaries, zeptofetch gives you information 200x faster* with a fraction of the size.

<sub>*_Performance varies by hardware and system configuration. See [benchmarks](#benchmarks) for details._

## ✨ Features

| **Performance**                                          | **Design**                                             |
| -------------------------------------------------------- | ------------------------------------------------------ |
| ⚡ **~1.9 ms runtime*** – up to 220× faster than Neofetch | 🎨 **Customizable colors** via a simple `config.h`     |
| 📦 **28 KB binary*** – minimal disk footprint            | 🐧 **Pure C implementation** – no shell scripts        |
| 🔒 **Safe caching** – optimized process chain            | 🔧 **Direct /proc parsing** – no `ps` or `popen` calls |
| 💾 **Zero dependencies** – supports static linking       | ✅ **Production ready** – tested and reliable           |

<sub>*Results may vary depending on hardware. See test system specs below.*</sub>

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
| **Kernel** | 6.17.5-2-cachyos |
| **OS** | Arch Linux |
| **Desktop** | KDE Plasma 6.5.1 (Wayland) |
| **Motherboard** | MSI B550-A PRO (MS-7C56) |

</details>

<table>
<thead>
<tr>
<th>Tool</th>
<th>Runtime</th>
<th>Binary Size</th>
<th>Speed vs neofetch</th>
</tr>
</thead>
<tbody>
<tr>
<td><strong>zeptofetch</strong></td>
<td><strong>1.9ms ± 0.1ms</strong></td>
<td><strong>28 KB</strong></td>
<td><code>218x faster</code></td>
</tr>
<tr>
<td>fastfetch</td>
<td>4.0ms ± 0.2ms</td>
<td>~200 KB</td>
<td><code>104x faster</code></td>
</tr>
<tr>
<td>neofetch</td>
<td>414.7ms ± 4.3ms</td>
<td>~50 KB</td>
<td><em>baseline</em></td>
</tr>
<tr>
<td>screenfetch</td>
<td>939.8ms ± 12.1ms</td>
<td>~100 KB</td>
<td><code>2.3x slower</code></td>
</tr>
</tbody>
</table>

> **Note**: Performance is highly dependent on hardware, system load, and configuration. Manual compilation yields significantly better performance than prebuilt releases. Results may vary on your system.

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
#define COLOR_1 "\033[1;34m"     // Primary (headers, ASCII art)
#define COLOR_2 "\033[1;37m"     // Secondary (ASCII details)
#define COLOR_3 "\033[38;5;208m" // Accent (labels)
#define COLOR_RESET "\033[0m"    // Reset to default
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

## 🐧 Compatibility

### Verified Distributions

<table>
<tr>
<td>

- ✅ Alpine Linux
- ✅ Arch Linux
- ✅ Artix Linux
- ✅ Debian
- ✅ Fedora
- ✅ Gentoo

</td>
<td>

- ✅ Linux Mint
- ✅ Manjaro
- ✅ NixOS
- ✅ openSUSE
- ✅ Pop!_OS
- ✅ Ubuntu

</td>
<td>

- ✅ Void Linux
- ✅ EndeavourOS
- ✅ Garuda Linux
- ✅ CachyOS
- ✅ Solus
- ✅ KDE neon

</td>
</tr>
</table>

### Requirements

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
