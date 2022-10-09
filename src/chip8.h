#pragma once

#include <stdint.h>

#include <SDL_stdinc.h>

#define CHIP8_SCREEN_WIDTH 64
#define CHIP8_SCREEN_HEIGHT 32
#define CHIP8_KEY_COUNT 16

#if defined(__cplusplus)
extern "C" {
#endif

enum Chip8InstructionError {
    CHIP8_ERROR_NONE,
    CHIP8_INVALID_INSTRUCTION
};

struct Chip8;
typedef struct Chip8 Chip8;

Chip8* chip8_create(uint8_t* key_state);

void chip8_destroy(Chip8* chip);

int chip8_execute(Chip8* chip);

void chip8_reset(Chip8* chip);

int chip8_load_rom_file(Chip8* chip, const char* path);

SDL_bool chip8_draw_flag(const Chip8* chip);

void chip8_set_draw_flag(Chip8* chip, SDL_bool flag);

const uint8_t* chip8_pixels(Chip8* chip);

#if defined(__cplusplus)
}
#endif