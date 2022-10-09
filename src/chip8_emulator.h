#pragma once

#include <SDL_render.h>

#include "chip8.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct Chip8Emulator {
    Chip8* chip;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint8_t key_state[CHIP8_KEY_COUNT];
    SDL_bool running;
} Chip8Emulator;

Chip8Emulator* chip8_emulator_create();

void chip8_emulator_destroy(Chip8Emulator* emulator);

void chip8_emulator_run(Chip8Emulator* emulator);

#if defined(__cplusplus)
}
#endif