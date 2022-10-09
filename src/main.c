#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL.h>

#include "chip8_emulator.h"

int main(int argc, char** argv) {
    Chip8Emulator* emulator = NULL;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "SDL init failed\n");
        return 1;
    }

    srand(time(NULL));

    if ((emulator = chip8_emulator_create()) == NULL) {
        fprintf(stderr, "Failed to create chip\n");
        return 1;
    }

    if (chip8_load_rom_file(emulator->chip, "roms/octojam2title.ch8") == 0) {
        fprintf(stderr, "%s\n", SDL_GetError());
        return 1;
    }

    chip8_emulator_run(emulator);
    chip8_emulator_destroy(emulator);

    SDL_Quit();

    return 0;
}