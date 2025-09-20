#ifndef CONFIG_H
#define CONFIG_H

// Terminal color codes
#define COLOR_RESET     "\033[0m"
#define COLOR_1         "\033[1;34m"       // Blue
#define COLOR_2         "\033[1;37m"       // White
#define COLOR_3         "\033[1;38;5;208m" // Orange (256-color)

// Fallback max path length if not defined by system
#ifndef PATH_MAX
#define PATH_MAX        4096
#endif

// Format string for /proc/<pid>/exe
#define PROC_PATH_FORMAT "/proc/%d/exe"

#endif // CONFIG_H
