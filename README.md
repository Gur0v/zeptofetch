# zeptofetch 🚀

<div align="center">

**The Ultra-Minimal System Information Tool**

[![GitHub License](https://img.shields.io/github/license/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/blob/main/LICENSE)
[![GitHub Stars](https://img.shields.io/github/stars/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/stargazers)
[![Linux Support](https://img.shields.io/badge/platform-linux-brightgreen)](https://github.com/Gur0v/zeptofetch#-compatibility)

[Key Features](#-key-features) • [Installation](#-installation) • [Configuration](#-configuration) • [Benchmarks](#-performance-benchmarks) • [Contributing](#-contributing)

</div>

## 🎯 Overview

zeptofetch is a fast, minimalist system information retrieval tool designed for users who appreciate performance and simplicity. With its microscopic footprint, zero external dependencies and a modular codebase, zeptofetch provides the fastest way to display essential system information while maintaining elegant output formatting.

## 📸 Preview

![zeptofetch Preview](https://monke.party/vsw3g1up.webp)

*Example output on Gentoo Linux with the stock color scheme.*

## ✨ Key Features

- **Microscopic Footprint**: Entire binary weighs less than 100KB
- **Zero Dependencies**: Completely self-contained implementation
- **Native Performance**: Leverages CPU-specific optimizations*
- **Instant Display**: Sub-20ms information retrieval
- **Customizable Output**: Easy color scheme modification
- **Resource Efficient**: Minimal CPU and memory usage

## 🛠 Installation

### Pre-built Binaries
Download the pre-built binary from the [releases page](https://github.com/Gur0v/zeptofetch/releases).
> *These are compiled with `march=x86-64 -mtune=generic` for maximum compatibility across different CPU architectures. Before you want to benefit from CPU-specific optimizations which you can get by compiling from source, check out the [benchmarks](#-performance-benchmarks).

### From Source
```bash
# Clone the repository
git clone https://github.com/Gur0v/zeptofetch
cd zeptofetch

# Build with optimal settings (includes CPU-specific optimizations)
make

# Install system-wide (optional)
sudo make install
```

## 🖥 Compatibility

### System Requirements
- **Operating System**: GNU/Linux & ~~GNU~~ Linux* (both are supported)
- **Architecture**: x86_64
> *Non-GNU Linux distros are supported (Void, Alpine, Gentoo, etc) but Android is **NOT supported.**

## 🎨 Configuration

### Color Customization
Edit `config.h` to personalize your output appearance:

```c
#define COLOR_RESET "\033[0m"     // Reset to terminal default
#define COLOR_1     "\033[1;34m"  // Bright blue (headers)
#define COLOR_2     "\033[1;37m"  // Bright white (values)
#define COLOR_3     "\033[1;38;5;208m"  // Bright orange (accents)
```

## 📊 Performance Benchmarks

### Test Environment
- **System**: Arch Linux, Alpine Linux Container (for Musl)
- **Kernel**: 6.12.6-2-[cachyos](https://cachyos.org/)
- **Processor**: AMD Ryzen 5 5600 (6C/12T @ 4.47 GHz)

### Compilation Configurations

| Config | Distribution | Avg Time | Binary Size |
|--------|---------------|-----------|-------------|
| 1 | From Source | 10.2 ms | 20K |
| 2 | Glibc Binary | 10.1 ms | 20K |
| 3 | Musl Binary | 12.7 ms | 16K |
| 4 | Static Binary | 9.9 ms | 68K |


*Benchmarks from [zeptofetch v1.0-rc2](https://github.com/Gur0v/zeptofetch/releases/tag/v1.0-rc2)*

## 🤔 Why "Zepto"?

The name "Zepto" comes from the [SI](https://en.wikipedia.org/wiki/International_System_of_Units) prefix for 10⁻²¹ (one sextillionth), reflecting our commitment to minimal resource usage:

- **~83x** smaller than fastfetch
- **~23x** smaller than neofetch

## 🤝 Contributing

While I'm not currently accepting direct code contributions, I always welcome:

- Bug reports
- Feature suggestions

Please use the GitHub issues system for all contributions.

## 📝 License

zeptofetch is licensed under the [GPL-3.0 License](LICENSE).
