# zeptofetch 🚀

**An Ultra-Minimal System Information Fetch Tool**

![GitHub License](https://img.shields.io/github/license/Gur0v/zeptofetch)
![GitHub Stars](https://img.shields.io/github/stars/Gur0v/zeptofetch)
![Linux Support](https://img.shields.io/badge/platform-linux-brightgreen)

## Overview

zeptofetch is a lightning-fast, minimalist system information retrieval tool designed for users who appreciate performance and simplicity. With its tiny footprint and zero external dependencies, zeptofetch provides a quick and elegant way to display system information.

## 📸 Preview

![zeptofetch Preview](https://monke.party/sish07tp.webp)

## ✨ Key Features

- **Microscopic Size**: Weighing less than 50KB
- **Zero Dependencies**: Clean, self-contained implementation
- **Native Performance**: Optimized with native CPU instructions
- **Rapid Information Retrieval**: Instant system details display

## 🛠 Installation

```bash
# Clone the repository
git clone https://github.com/Gur0v/zeptofetch

# Navigate to the project directory
cd zeptofetch

# Build and install
make && sudo make install
```

## 🖥 Compatibility

- **Primary Platform**: Linux
- **Minimum Kernel Version**: 4.19+
- **Experimental Support**: FreeBSD (requires Makefile modification)

## 🎨 Color Customization

zeptofetch allows easy color customization through the `config.h` file. You can modify the following color definitions:

```c
#define COLOR_RESET "\033[0m"     // Resets text to default terminal color
#define COLOR_1     "\033[1;34m"  // Bright blue
#define COLOR_2     "\033[1;37m"  // Bright white/light gray
#define COLOR_3     "\033[1;38;5;208m"  // Bright orange (256-color palette)
```

Color codes explained:
- `COLOR_RESET`: Restores default terminal text color
- `COLOR_1`: Bold bright blue
- `COLOR_2`: Bold bright white/light gray
- `COLOR_3`: Bold bright orange (color index 208 in 256-color palette)

Simply edit these definitions in `config.h` to personalize your zeptofetch appearance.

## 🤔 Why "Zepto"?

The name "Zepto" derives from the SI prefix representing 10⁻²¹ (one quintillionth), symbolizing the tool's incredibly minimal footprint:
- Approximately 83.2 times smaller than fastfetch
- Approximately 23.25 times smaller than neofetch


## 📊 Performance Benchmarks

### Test Environment
- **System**: Bazzite 41, Linux 6.11.9
- **Processor**: Intel Core i5-6600 (4 cores, 3.90 GHz)

### Compilation Configurations

| Config | Compiler Flags | Avg Time | Size |
|--------|----------------|----------|--------|
| 1 | `-march=native -O2 -s` | **19.0 ms** | **16K** |
| 2 | `-march=native -Os -s` | 20.3 ms | 16K |
| 3 | `-march=native -O2` | 20.0 ms | 24K |
| 4 | `-march=native -O3 -s` | 21.7 ms | 16K |
| 5 | `-march=native -O2 -static` | 23.1 ms | 68K |

**Key Findings** (zeptofetch v1.0-rc1): 
- Fastest config: #1 at 19.0 ms
- Most consistent performance
- Minimal memory overhead

## 🤝 Contributing

While direct contributions are not currently being accepted, feedback and suggestions are always welcome.

## 📄 License

This project is licensed under the GPL-3.0 License.

## 🌟 Star History

<a href="https://star-history.com/#Gur0v/zeptofetch&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=Gur0v/zeptofetch&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=Gur0v/zeptofetch&type=Date" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=Gur0v/zeptofetch&type=Date" />
 </picture>
</a>
