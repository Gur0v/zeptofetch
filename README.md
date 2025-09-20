# zeptofetch

**zeptofetch** — a 16KB system info tool that runs in ~10ms.

![zeptofetch](https://i.e-z.host/ml5dtgia.png)

## Overview

Shows basic system information fast and without bloat. Think neofetch, but tiny.

[![GitHub License](https://img.shields.io/github/license/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/blob/main/LICENSE)
[![GitHub Stars](https://img.shields.io/github/stars/Gur0v/zeptofetch)](https://github.com/Gur0v/zeptofetch/stargazers)
[![Linux Support](https://img.shields.io/badge/platform-linux-brightgreen)](https://github.com/Gur0v/zeptofetch#-compatibility)

## Installation

**From source**
```bash
git clone https://github.com/Gur0v/zeptofetch
cd zeptofetch
make
sudo make install
````

**Prebuilt binaries**
Download from [Releases](https://github.com/Gur0v/zeptofetch/releases).

## Configuration

Colors are set in `config.h`:

```c
#define COLOR_1  "\033[1;34m"    // Headers
#define COLOR_2  "\033[1;37m"    // Values
#define COLOR_3  "\033[38;5;208m"// Accents
```

## Benchmarks

Measured with [hyperfine](https://github.com/sharkdp/hyperfine):

| Tool           | Time      | Binary size | Notes                    |
| -------------- | --------- | ----------- | ------------------------ |
| fastfetch      | 4.0ms     | \~200KB     | fastest overall          |
| **zeptofetch** | **9.8ms** | **16KB**    | 39× faster than neofetch |
| neofetch       | 378ms     | \~50KB      | baseline                 |

## Build configurations

| Build   | Size | Notes                                    |
| ------- | ---- | ---------------------------------------- |
| Default | 16KB | `-march=native`, dynamic, size-optimized |
| Release | 68KB | `-march=x86-64-v3`, static (musl/NixOS)  |

## Requirements

* Linux (x86\_64)
* Works on GNU/Linux (tested on Alpine, Void, Gentoo, etc.)
* Not Android

## Contributing

Bug reports and small patches welcome via [Issues](https://github.com/Gur0v/zeptofetch/issues).

## License

Licensed under [GPL-3.0](LICENSE).
