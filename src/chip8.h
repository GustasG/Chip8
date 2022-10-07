#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

struct Chip8;
typedef struct Chip8 Chip8;

Chip8* chip8_create();
void chip8_destroy(Chip8* chip);
int chip8_load_rom(Chip8* chip, const char* path);
void chip8_run(Chip8* chip);

#if defined(__cplusplus)
}
#endif