# Chip8

CHIP-8 emulator

## Installation

This project uses [Conan](https://conan.io/) for package manager and [CMake](https://cmake.org/) as build system. To build project you can use:

```bash
mkdir build
conan install .. --build=missing -s build_type=Debug
cmake ..
cmake --build .
```

After this both emulator binary and project files should be in /build folder.
