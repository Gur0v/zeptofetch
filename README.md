# zeptofetch

**An ultra-minimal system information tool for Linux, written in C.**

[![GitHub License](https://img.shields.io/github/license/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/blob/main/LICENSE)
[![GitHub Stars](https://img.shields.io/github/stars/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/stargazers)
[![Linux Support](https://img.shields.io/badge/platform-linux-brightgreen)](https://github.com/Gur0v/zeptofetch#-compatibility)

## Why zeptofetch?

If you've ever waited for neofetch to crawl through your system info, you'll appreciate zeptofetch. While neofetch takes 378ms to display your system details, zeptofetch does the same job in just 10ms — that's 39 times faster.

But speed isn't everything. What makes zeptofetch special is the balance: it's incredibly fast while remaining tiny. The entire binary is just 16KB, smaller than most configuration files you have on your system.

**Key advantages:**
- **Lightning fast execution**: 10ms vs 378ms for neofetch
- **Minimal footprint**: 16KB binary with zero dependencies  
- **Pure C implementation**: No scripts, no external libraries
- **Highly portable**: Works across all major Linux distributions
- **Easily customizable**: Simple color configuration

![zeptofetch demo](https://i.e-z.host/ml5dtgia.png)

## Benchmarks

Measured using [hyperfine](https://github.com/sharkdp/hyperfine) on identical hardware (Ryzen 5 5600):

| Tool        | Execution Time | Binary Size | Performance vs neofetch |
|-------------|----------------|-------------|-------------------------|
| zeptofetch  | **9.8ms**      | **16KB**    | **39× faster**         |
| fastfetch   | 4.0ms          | ~200KB      | 95× faster              |
| neofetch    | 378ms          | ~50KB       | baseline                |

While fastfetch is technically faster, zeptofetch delivers 95% of that performance in just 8% of the binary size. For most users, this represents the sweet spot between speed and efficiency.

## Installation

**From source:**
```bash
git clone https://github.com/Gur0v/zeptofetch
cd zeptofetch
make
sudo make install
```

**Pre-built binaries:**  
Download from [Releases](https://github.com/Gur0v/zeptofetch/releases)

## Configuration

Customize colors by editing `config.h` before building:

```c
#define COLOR_1  "\033[1;34m"     // Headers (blue)
#define COLOR_2  "\033[1;37m"     // Values (white)  
#define COLOR_3  "\033[38;5;208m" // Accents (orange)
```

Rebuild after making changes: `make clean && make`

## Build Options

| Configuration | Binary Size | Optimization | Purpose |
|---------------|-------------|--------------|---------|
| Default       | 16KB        | `-march=native`, dynamic linking | General use |
| Release       | 68KB        | `-march=x86-64-v3`, static linking | Distribution |

## System Requirements

- Linux (x86_64 architecture)
- Compatible with all major GNU/Linux distributions
- Tested on Alpine, Void, Gentoo, Ubuntu, Arch, and others
- Does not support Android

## Contributing

Contributions are welcome! Feel free to:
- Report bugs through [GitHub Issues](https://github.com/Gur0v/zeptofetch/issues)
- Suggest new features
- Help with documentation

## License

Licensed under [GPL-3.0](LICENSE).
