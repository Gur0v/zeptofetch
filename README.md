# zeptofetch 🚀

**An Ultra-Minimal System Information Fetch Tool**

![GitHub License](https://img.shields.io/github/license/Gur0v/zeptofetch)
![GitHub Stars](https://img.shields.io/github/stars/Gur0v/zeptofetch)
![Linux Support](https://img.shields.io/badge/platform-linux-brightgreen)

## Overview

zeptofetch is a lightning-fast, minimalist system information retrieval tool designed for users who appreciate performance and simplicity. With its tiny footprint and zero external dependencies, zeptofetch provides a quick and elegant way to display system information.

## 📸 Preview

![zeptofetch Preview](https://monke.party/m9cx715g.png)

## ✨ Key Features

- **Microscopic Size**: Weighing less than 50KB
- **Zero Dependencies**: Clean, self-contained implementation
- **Native Performance**: Optimized with native CPU instructions
- **Rapid Information Retrieval**: Instant system details display [(Benchmark of zeptofetch-v1.0-rc1)](https://github.com/Gur0v/zeptofetch/commit/56daa35b5c15c132aef669d5ed29f9edda08c4e7#commitcomment-150075748)

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

## 🤝 Contributing

While direct contributions are not currently being accepted, feedback and suggestions are always welcome.

## 📄 License

This project is licensed under the GPL-3.0 License.
