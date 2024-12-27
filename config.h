#ifndef CONFIG_DEF_H
#define CONFIG_DEF_H

// Default color scheme
#define COLOR_RESET "\033[0m"     // Reset to terminal default
#define COLOR_1     "\033[1;34m"  // Bright blue (headers)
#define COLOR_2     "\033[1;37m"  // Bright white (values)
#define COLOR_3     "\033[1;38;5;208m"  // Bright orange (accents)

#define BUFFER_SIZE 256
#define PROC_PATH_FORMAT "/proc/%d/exe"

#endif
