# zeptofetch 🚀

<div align="center">

**The Ultra-Minimal System Information Fetch Tool**

[![GitHub License](https://img.shields.io/github/license/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/blob/main/LICENSE)
[![GitHub Stars](https://img.shields.io/github/stars/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/stargazers)
[![Linux Support](https://img.shields.io/badge/platform-linux-brightgreen)](https://github.com/Gur0v/zeptofetch#-compatibility)

[Key Features](#-key-features) • [Installation](#-installation) • [Configuration](#-configuration) • [Benchmarks](#-performance-benchmarks) • [Contributing](#-contributing)

</div>

## 🎯 Overview

zeptofetch is a lightning-fast, minimalist system information retrieval tool designed for users who appreciate performance and simplicity. With its microscopic footprint and zero external dependencies, zeptofetch provides the fastest way to display essential system information while maintaining elegant output formatting.

## 📸 Preview

![zeptofetch Preview](https://monke.party/sish07tp.webp)

*Example output on Bazzite Linux with the stock color scheme*

## ✨ Key Features

- **Microscopic Footprint**: Entire binary weighs less than 50KB
- **Zero Dependencies**: Completely self-contained implementation
- **Native Performance**: Leverages CPU-specific optimizations*
- **Instant Display**: Sub-20ms information retrieval
- **Customizable Output**: Easy color scheme modification
- **Resource Efficient**: Minimal CPU and memory usage

## 🛠 Installation

### Pre-built Binaries
Download the pre-built binary from the [releases page](https://github.com/Gur0v/zeptofetch/releases).
> *These are compiled with `-mtune=generic` for maximum compatibility across different CPU architectures. To benefit from CPU-specific optimizations, compile from source instead.

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
- **Operating System**: Linux (primary), FreeBSD (requires source patching)
- **Kernel Version**: 4.19 or newer
- **Architecture**: x86_64
- **Memory**: < 1MB RAM

## 🎨 Configuration

### Color Customization
Edit `config.h` to personalize your output appearance:

```c
// Default color scheme
#define COLOR_RESET "\033[0m"     // Reset to terminal default
#define COLOR_1     "\033[1;34m"  // Bright blue (headers)
#define COLOR_2     "\033[1;37m"  // Bright white (values)
#define COLOR_3     "\033[1;38;5;208m"  // Bright orange (accents)
```

## 📊 Performance Benchmarks

### Test Environment
- **System**: Bazzite 41, Linux 6.11.9
- **Processor**: Intel Core i5-6600 (4C/4T @ 3.90 GHz)

### Compilation Configurations

| Config | Compiler Flags | Avg Time | Binary Size |
|--------|---------------|-----------|-------------|
| 1 | `-march=native -O2 -s` | **19.0 ms** | **16K** |
| 2 | `-march=native -Os -s` | 20.3 ms | 16K |
| 3 | `-march=native -O2` | 20.0 ms | 24K |
| 4 | `-march=native -O3 -s` | 21.7 ms | 16K |
| 5 | `-march=native -O2 -static` | 23.1 ms | 68K |

*Benchmarks from zeptofetch v1.0-rc1*

## 🤔 Why "Zepto"?

The name "Zepto" comes from the SI prefix for 10⁻²¹ (one sextillionth), reflecting our commitment to minimal resource usage:

- **~83x** smaller than fastfetch
- **~23x** smaller than neofetch

## 🤝 Contributing

While I'm not currently accepting direct code contributions, I welcome:

- Bug reports
- Feature suggestions

Please use the GitHub issues system for all contributions.

## 📝 License

zeptofetch is licensed under the [GPL-3.0 License](LICENSE).
