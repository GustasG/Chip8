#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL.h>

#include "chip8_emulator.h"

static void fail() {
    fprintf(stderr, "%s\n", SDL_GetError());
    exit(1);
}

int main(int argc, char** argv) {
    Chip8Emulator* emulator = NULL;

    srand(time(NULL));

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fail();
    }

    if ((emulator = chip8_emulator_create()) == NULL) {
        fail();
    }

    if (chip8_emulator_load_rom_file(emulator, "roms/test_opcode.ch8") != 0) {
        fail();
    }

    chip8_emulator_run(emulator);
    chip8_emulator_destroy(emulator);

    SDL_Quit();

    return 0;
}