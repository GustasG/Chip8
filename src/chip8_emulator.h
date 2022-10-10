#pragma once

#include "chip8.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct Chip8Emulator;
typedef struct Chip8Emulator Chip8Emulator;

Chip8Emulator* chip8_emulator_create();

void chip8_emulator_destroy(Chip8Emulator* emulator);

int chip8_emulator_load_rom_file(Chip8Emulator* emulator, const char* path);

void chip8_emulator_run(Chip8Emulator* emulator);

#if defined(__cplusplus)
}
#endif