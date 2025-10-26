# Contributing to zeptofetch

Thank you for your interest in contributing to zeptofetch! This document provides guidelines and instructions for contributing.

## 🎯 Ways to Contribute

### Reporting Bugs

If you find a bug, please create an issue with:

- **Clear title**: Describe the problem concisely
- **System information**: OS, kernel version, architecture
- **Steps to reproduce**: How to trigger the bug
- **Expected behavior**: What should happen
- **Actual behavior**: What actually happens
- **Terminal output**: Include any relevant error messages
- **Build information**: Compilation flags, compiler version

### Suggesting Features

Feature suggestions are welcome! Please include:

- **Use case**: Why this feature would be useful
- **Implementation ideas**: How it might work (optional)
- **Compatibility concerns**: Any potential issues with existing systems
- **Performance impact**: Whether it might affect runtime

### Testing on New Distributions

Help us verify compatibility:

1. Test zeptofetch on your distribution
2. Report results (working/broken)
3. Include distro name, version, and any issues
4. Open an issue or PR to add it to the verified list

**Tip**: Use [distrobox](https://github.com/89luca89/distrobox) or VMs to easily test on multiple distributions without dual-booting:

```bash
# Example: Test on Ubuntu using distrobox
distrobox create --name ubuntu --image ubuntu:latest
distrobox enter ubuntu
# Build and test zeptofetch inside the container
```

### Documentation Improvements

Documentation can always be better:

- Fix typos or unclear explanations
- Add examples or clarifications
- Improve installation instructions
- Translate to other languages (future)

## 🔧 Development Setup

### Prerequisites

```bash
# Debian/Ubuntu
sudo apt install build-essential git

# Arch Linux
sudo pacman -S base-devel git

# Fedora
sudo dnf install gcc make git
```

### Building from Source

```bash
git clone https://github.com/Gur0v/zeptofetch
cd zeptofetch
make
./zeptofetch
```

### Development Build

For debugging with sanitizers:

```bash
make debug
./zeptofetch
```

This enables:
- `-O0`: No optimization for easier debugging
- `-g`: Debug symbols for GDB/LLDB
- `-fsanitize=address,undefined`: Catches memory errors and undefined behavior

The debug build is larger and slower but helps catch:
- Buffer overflows
- Use-after-free errors
- Memory leaks
- Null pointer dereferences
- Integer overflows

### Manual Debug Build

Alternatively, build with custom flags:

```bash
make clean
CFLAGS="-g -O0 -Wall -Wextra" make
```

## 📝 Code Style

### General Guidelines

- **No comments**: Code should be self-explanatory
- **Short names**: Use concise but readable variable names (e.g., `cnt`, `buf`, `sz`)
- **Function names**: Descriptive verbs (e.g., `get_shell`, `read_ppid`, `cache_add`)
- **Constants**: Use `#define` for magic numbers
- **Error handling**: Always check return values

### Code Structure

```c
// Good: Short, clear function
static int read_exe(pid_t pid, char *buf, size_t sz) {
    char path[PATH_MAX];
    int r = snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    if (r < 0 || (size_t)r >= sizeof(path)) return -1;
    ssize_t len = readlink(path, buf, sz - 1);
    if (len <= 0) return -1;
    buf[len] = '\0';
    return 0;
}

// Bad: Overly verbose
static int read_executable_path(pid_t process_id, char *buffer, size_t buffer_size) {
    // Construct the path to /proc/[pid]/exe
    char proc_path[PATH_MAX];
    // ... etc
}
```

### Optimization Guidelines

- **Profile first**: Use `perf` or `valgrind` to identify bottlenecks
- **Avoid premature optimization**: Readability matters
- **Cache wisely**: Only cache data that's expensive to fetch
- **Minimize syscalls**: Batch operations where possible
- **Stack over heap**: Prefer stack allocation for small buffers

## 🔍 Pull Request Process

### Before Submitting

1. **Test thoroughly**: Run on multiple systems if possible
2. **Check performance**: Use `hyperfine` to verify no regressions
3. **Verify compilation**: Test with GCC and Clang
4. **Test edge cases**: Invalid PIDs, missing files, etc.
5. **Keep it focused**: One feature/fix per PR

### PR Template

```markdown
## Description
Brief description of changes

## Motivation
Why is this change needed?

## Testing
- [ ] Tested on x86_64
- [ ] Verified no performance regression
- [ ] Tested on multiple distributions

## Benchmark Results (if applicable)
Before: X.XXms
After: X.XXms
```

### Review Process

1. Maintainers will review your PR
2. Address any feedback or requested changes
3. Once approved, your PR will be merged
4. You'll be credited in release notes

## 🐛 Bug Fix Guidelines

### Critical Bugs (Security/Crashes)

- **Priority**: Fix immediately
- **Testing**: Extensive verification required
- **Documentation**: Clear explanation of the issue and fix

### Non-Critical Bugs

- **Testing**: Verify the fix works and doesn't break anything else
- **Scope**: Keep changes minimal and focused

## ✨ Feature Guidelines

### Adding New Features

Consider:

- **Performance impact**: Will it slow down execution?
- **Binary size**: How much will it increase the binary?
- **Maintenance**: Is it worth the added complexity?
- **User value**: How many users will benefit?

### Acceptable Features

- Detection improvements (new terminals, WMs, etc.)
- Performance optimizations
- Better error handling
- Compatibility fixes

### Features to Avoid

- Network requests
- External dependencies
- Slow operations (>1ms)
- Features that significantly increase binary size

## 📊 Benchmarking

Always benchmark changes that might affect performance:

```bash
# Install hyperfine
sudo pacman -S hyperfine  # Arch
sudo apt install hyperfine  # Debian/Ubuntu

# Build both versions
make clean && make
cp zeptofetch zeptofetch-new
git checkout main
make clean && make
cp zeptofetch zeptofetch-old

# Compare
hyperfine --warmup 10 --runs 1000 './zeptofetch-old' './zeptofetch-new'
```

## 📄 License

By contributing, you agree that your contributions will be licensed under the GPL-3.0 License.

## ❓ Questions?

- Open a [discussion](https://github.com/Gur0v/zeptofetch/discussions)
- Create an [issue](https://github.com/Gur0v/zeptofetch/issues)
- Reach out to maintainers

## 🙏 Recognition

All contributors are valued! Significant contributions will be:

- Credited in release notes
- Listed in the README contributors section
- Mentioned in the project history

Thank you for helping make zeptofetch better!
