# Chip8

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/91d006ad79cf4d86b532ec6ec24f1658)](https://www.codacy.com/gh/GustasG/Chip8/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=GustasG/Chip8&amp;utm_campaign=Badge_Grade)

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
