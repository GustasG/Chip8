#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL.h>

#include "chip8_emulator.h"

static void usage() {
    fprintf(stdout, "CHIP-8 Emulator\n");
    fprintf(stdout, "\t USAGE: [fps] <file>\n");
}

static void fail() {
    fprintf(stderr, "%s\n", SDL_GetError());
    exit(1);
}

static void valid_argc_count(int argc, int pos, const char* descriptipn) {
    if (pos >= argc) {
        fprintf(stderr, "Expected \"%s\" value\n", descriptipn);
        exit(2);
    }
}

int main(int argc, char** argv) {
    int args = 1;
    Chip8Emulator* emulator = NULL;
    float fps = 60.0f;
    char* file = NULL;

    srand(time(NULL));

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fail();
    }

    for (; args < argc && SDL_strncmp(argv[args], "--", 2) == 0; args += 2) {
        if (SDL_strcmp(argv[args], "--fps") == 0) {
            valid_argc_count(argc, args + 1, "fps");
            fps = SDL_atof(argv[args + 1]);
        } else if (SDL_strcmp(argv[args], "--file") == 0) {
            valid_argc_count(argc, args + 1, "file");
            file = argv[args + 1];
        } else if (SDL_strcmp(argv[args], "--help") == 0) {
            usage();
            return 0;
        } else {
            fprintf(stderr, "Invalid argument: %s\n", argv[args]);
            return 2;
        }
    }

    if (file == NULL) {
        if (args >= argc) {
            usage();
            return 2;
        } else {
            file = argv[args];
        }
    }

    if ((emulator = chip8_emulator_create()) == NULL) {
        fail();
    }

    if (chip8_emulator_load_rom_file(emulator, file) != 0) {
        fail();
    }

    chip8_emulator_set_fps(emulator, fps);
    chip8_emulator_run(emulator);
    chip8_emulator_destroy(emulator);

    SDL_Quit();

    return 0;
}