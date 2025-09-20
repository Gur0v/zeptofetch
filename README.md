<div align="center">

# zeptofetch

*An ultra-minimal system information tool for Linux, written in C.*

[![License](https://img.shields.io/github/license/Gur0v/zeptofetch)](LICENSE)
[![Stars](https://img.shields.io/github/stars/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/stargazers)
[![Platform](https://img.shields.io/badge/platform-linux-brightgreen)](#system-requirements)

</div>  

## Overview

`zeptofetch` provides fast, minimal system information without the overhead of larger tools.
Compared to neofetch’s \~378 ms runtime, zeptofetch completes in \~10 ms with a binary size of only 16 KB.

### Features

* **Fast** — \~10 ms runtime
* **Compact** — 16 KB binary, no dependencies
* **Pure C** — no scripts, no external libraries
* **Portable** — works across major Linux distributions
* **Configurable** — simple color definitions in `config.h`

## Benchmarks

Tested with [hyperfine](https://github.com/sharkdp/hyperfine) on identical hardware (`zeptofetch v1.0-rc5`).

| Tool       | Time       | Size      | Comparison               |
| ---------- | ---------- | --------- | ------------------------ |
| zeptofetch | **9.8 ms** | **16 KB** | 39× faster than neofetch |
| fastfetch  | 4.0 ms     | \~200 KB  | Faster, but 12× larger   |
| neofetch   | 378 ms     | \~50 KB   | Baseline                 |

## Installation

### From Source

```bash
git clone https://github.com/Gur0v/zeptofetch
cd zeptofetch
make
sudo make install
```

### Prebuilt Binaries

Available on the [Releases](https://github.com/Gur0v/zeptofetch/releases) page.

## Configuration

Colors are defined in `config.h`:

```c
#define COLOR_1 "\033[1;34m"     // headers
#define COLOR_2 "\033[1;37m"     // values
#define COLOR_3 "\033[38;5;208m" // accents
```

Rebuild after making changes:

```bash
make clean && make
```

## Build Options

| Mode    | Size  | Flags                         | Use case     |
| ------- | ----- | ----------------------------- | ------------ |
| Default | 16 KB | `-march=native`, dynamic link | General use  |
| Release | 68 KB | `-march=x86-64-v3`, static    | Distribution |

## System Requirements

* Linux (x86\_64)
* Verified on Alpine, Void, Gentoo, Arch, Ubuntu
* Android not supported

## Contributing

Contributions are welcome:

* Open [issues](https://github.com/Gur0v/zeptofetch/issues)
* Suggest improvements or features
* Help improve documentation

## License

[GPL-3.0](LICENSE)
