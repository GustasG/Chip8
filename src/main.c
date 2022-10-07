#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include <SDL.h>

#include "chip8.h"

int main(int argc, char** argv) {
    Chip8* chip = NULL;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "SDL init failed\n");
        return 1;
    }

    srand(time(NULL));
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    if ((chip = chip8_create()) == NULL) {
        fprintf(stderr, "Failed to create chip\n");
        return 1;
    }

    if (chip8_load_rom(chip, "roms/IBM Logo.ch8") == 0) {
        fprintf(stderr, "Failed to load rom\n");
        return 1;
    }

    chip8_run(chip);
    chip8_destroy(chip);

    SDL_Quit();

    return 0;
}