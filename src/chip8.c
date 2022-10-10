#include "chip8.h"

#include <stdlib.h>

#include <SDL_error.h>
#include <SDL_rwops.h>

struct Chip8 {
    uint8_t ram[4096];
    uint8_t pixels[CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT];
    uint16_t stack[32];
    uint8_t V[16];
    uint8_t* key_state;
    uint16_t I;
    uint16_t PC;
    uint16_t SP;
    uint8_t delay_timer;
    uint8_t sound_timer;
    bool draw_flag;
};

static const uint8_t fontset[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80
};

static uint16_t fetch(const Chip8* chip) {
    uint8_t hb = chip->ram[chip->PC];
    uint8_t lb = chip->ram[chip->PC + 1];

    return hb << 8 | lb;
}

static void push(Chip8* chip, uint16_t value) {
    chip->stack[chip->SP++] = value;
}

static uint16_t pop(Chip8* chip) {
    return chip->stack[--chip->SP];
}

static int execute0(Chip8* chip, uint16_t opcode) {
    switch (opcode)
    {
    case 0x00E0:
        SDL_memset(chip->pixels, 0, sizeof(chip->pixels));
        chip8_set_draw_flag(chip, true);
        return CHIP8_ERROR_NONE;
    case 0x00EE:
        chip->PC = pop(chip);
        return CHIP8_ERROR_NONE;
    default:
        return CHIP8_INVALID_INSTRUCTION;
    }
}

static int execute1(Chip8* chip, uint16_t opcode) {
    uint16_t value = opcode & 0x0FFF;
    chip->PC = value;

    return CHIP8_ERROR_NONE;
}

static int execute2(Chip8* chip, uint16_t opcode) {
    uint16_t value = opcode & 0x0FFF;
    push(chip, chip->PC);
    chip->PC = value;

    return CHIP8_ERROR_NONE;
}

static int execute3(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint16_t value = opcode & 0x00FF;

    if (chip->V[x] == value) {
        chip->PC += 2;
    }

    return CHIP8_ERROR_NONE;
}

static int execute4(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint16_t value = opcode & 0x00FF;

    if (chip->V[x] != value) {
        chip->PC += 2;
    }

    return CHIP8_ERROR_NONE;
}

static int execute5(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    if (chip->V[x] == chip->V[y]) {
        chip->PC += 2;
    }

    return CHIP8_ERROR_NONE;
}

static int execute6(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t value = opcode & 0x00FF;
    chip->V[x] = value;

    return CHIP8_ERROR_NONE;
}

static int execute7(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t value = opcode & 0x00FF;
    chip->V[x] += value;

    return CHIP8_ERROR_NONE;
}

static int execute8(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    switch (opcode & 0x000F)
    {
    case 0x0000:
        chip->V[x] = chip->V[y];
        return CHIP8_ERROR_NONE;
    case 0x0001:
        chip->V[x] |= chip->V[y];
        return CHIP8_ERROR_NONE;
    case 0x0002:
        chip->V[x] &= chip->V[y];
        return CHIP8_ERROR_NONE;
    case 0x0003:
        chip->V[x] ^= chip->V[y];
        return CHIP8_ERROR_NONE;
    case 0x0004:
        chip->V[0xF] = (chip->V[x] > 0xFF - chip->V[y] ? 0 : 1);
        chip->V[x] += chip->V[y];
        return CHIP8_ERROR_NONE;
    case 0x0005:
        chip->V[0xF] = (chip->V[y] > chip->V[x] ? 0 : 1);
        chip->V[x] -= chip->V[y];
        return CHIP8_ERROR_NONE;
    case 0x0006:
        chip->V[0xF] = chip->V[x] & 0x1;
        chip->V[x] >>= 1;
        return CHIP8_ERROR_NONE;
    case 0x0007:
        chip->V[0xF] = (chip->V[x] > chip->V[y] ? 0 : 1);
        chip->V[x] = chip->V[y] - chip->V[x];
        return CHIP8_ERROR_NONE;
    case 0x000E:
        chip->V[0xF] = chip->V[x] >> 7;
        chip->V[x] <<= 1;
        return CHIP8_ERROR_NONE;
    default:
        return CHIP8_INVALID_INSTRUCTION;
    }
}

static int execute9(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    if (chip->V[x] != chip->V[y]) {
        chip->PC += 2;
    }

    return CHIP8_ERROR_NONE;
}

static int executeA(Chip8* chip, uint16_t opcode) {
    uint16_t value = opcode & 0x0FFF;
    chip->I = value;

    return CHIP8_ERROR_NONE;
}

static int executeB(Chip8* chip, uint16_t opcode) {
    uint16_t value = opcode & 0x0FFF;
    chip->PC = chip->V[0] + value;

    return CHIP8_ERROR_NONE;
}

static int executeC(Chip8* chip, uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t random = rand() % 256;
    uint8_t value = opcode & 0x00FF;
    chip->V[reg] = random & value;

    return CHIP8_ERROR_NONE;
}

static int executeD(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t height = opcode & 0x000F;

    chip->V[0xF] = 0;

    for (uint8_t i = 0; i < height; i++) {
        uint8_t pixel = chip->ram[chip->I + i];

        for (uint8_t j = 0; j < 8; j++) {
            uint16_t location = ((chip->V[y] + i) * CHIP8_SCREEN_WIDTH) + chip->V[x] + j;

            if ((pixel & (0x80 >> j)) != 0) {
                if (chip->pixels[location] == 1) {
                    chip->V[0xF] = 1;
                }

                chip->pixels[location] ^= 1;
            }
        }
    }

    chip8_set_draw_flag(chip, true);

    return CHIP8_ERROR_NONE;
}

static int executeE(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;

    switch (opcode & 0x00FF)
    {
    case 0x009E:
        if (chip->key_state[chip->V[x]] == 1) {
            chip->PC += 2;
        }

        return CHIP8_ERROR_NONE;
    case 0x00A1:
        if (chip->key_state[chip->V[x]] == 0) {
            chip->PC += 2;
        }

        return CHIP8_ERROR_NONE;
    default:
        return CHIP8_INVALID_INSTRUCTION;
    }
}

static int executeF(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;

    switch (opcode & 0x00FF)
    {
    case 0x0007:
        chip->V[x] = chip->delay_timer;
        return CHIP8_ERROR_NONE;
    case 0x000A:
        bool pressed = false;

        for (uint8_t i = 0; i < CHIP8_KEY_COUNT; i++) {
            if (chip->key_state[i] == 1) {
                chip->V[x] = i;
                pressed = true;
                break;
            }
        }

        if (!pressed) {
            chip->PC -= 2;
        }

        return CHIP8_ERROR_NONE;
    case 0x0015:
        chip->delay_timer = chip->V[x];
        return CHIP8_ERROR_NONE;
    case 0x0018:
        chip->sound_timer = chip->V[x];
        return CHIP8_ERROR_NONE;
    case 0x001E:
        chip->I += chip->V[x];
        return CHIP8_ERROR_NONE;
    case 0x0029:
        chip->I = chip->V[x] * 0x05;
        return CHIP8_ERROR_NONE;
    case 0x0033:
        chip->ram[chip->I] = chip->V[x] / 100;
        chip->ram[chip->I + 1] = (chip->V[x] / 10) % 10;
        chip->ram[chip->I + 2] = (chip->V[x] % 100) % 10;
        return CHIP8_ERROR_NONE;
    case 0x0055:
        SDL_memcpy(&chip->ram[chip->I], chip->V, x + 1);
        return CHIP8_ERROR_NONE;
    case 0x0065:
        SDL_memcpy(chip->V, &chip->ram[chip->I], x + 1);
        return CHIP8_ERROR_NONE;
    default:
        return CHIP8_INVALID_INSTRUCTION;
    }
}

static int execute(Chip8* chip) {
    uint16_t opcode = fetch(chip);
    chip->PC += 2;

    switch (opcode & 0xF000)
    {
    case 0x0000:
        return execute0(chip, opcode);
    case 0x1000:
        return execute1(chip, opcode);
    case 0x2000:
        return execute2(chip, opcode);
    case 0x3000:
        return execute3(chip, opcode);
    case 0x4000:
        return execute4(chip, opcode);
    case 0x5000:
        return execute5(chip, opcode);
    case 0x6000:
        return execute6(chip, opcode);
    case 0x7000:
        return execute7(chip, opcode);
    case 0x8000:
        return execute8(chip, opcode);
    case 0x9000:
        return execute9(chip, opcode);
    case 0xA000:
        return executeA(chip, opcode);
    case 0xB000:
        return executeB(chip, opcode);
    case 0xC000:
        return executeC(chip, opcode);
    case 0xD000:
        return executeD(chip, opcode);
    case 0xE000:
        return executeE(chip, opcode);
    case 0xF000:
        return executeF(chip, opcode);
    default:
        return CHIP8_INVALID_INSTRUCTION;
    }
}

Chip8* chip8_create(uint8_t* key_state) {
    Chip8* chip = NULL;

    if (key_state == NULL) {
        SDL_InvalidParamError("key_state");
        return NULL;
    }

    if ((chip = SDL_malloc(sizeof(Chip8))) == NULL) {
        return NULL;
    }

    chip->key_state = key_state;

    return chip;
}

void chip8_destroy(Chip8* chip) {
    if (chip == NULL) {
        return;
    }

    SDL_free(chip);
}

int chip8_execute(Chip8* chip) {
    return execute(chip);
}

void chip8_update_timers(Chip8* chip) {
    if (chip->delay_timer > 0) {
        chip->delay_timer -= 1;
    }

    if (chip->sound_timer > 0) {
        chip->sound_timer -= 1;
    }
}

void chip8_reset(Chip8* chip) {
    if (chip == NULL) {
        return;
    }

    chip->I = 0;
    chip->PC = 0x200;
    chip->SP = 0;
    chip->delay_timer = 0;
    chip->sound_timer = 0;
    chip->draw_flag = true;
    SDL_memset(chip->pixels, 0, sizeof(chip->pixels));
    SDL_memcpy(chip->ram, fontset, sizeof(fontset) / sizeof(fontset[0]));
}

int chip8_load_rom_file(Chip8* chip, const char* path) {
    SDL_RWops* f = NULL;

    if (chip == NULL) {
        return SDL_InvalidParamError("chip");
    }

    if ((f = SDL_RWFromFile(path, "r+b")) == NULL) {
        return -1;
    }

    chip8_reset(chip);
    SDL_RWread(f, chip->ram + chip->PC, sizeof(uint8_t), sizeof(chip->ram) - 0x200);
    SDL_RWclose(f);

    return 0;
}

bool chip8_draw_flag(const Chip8* chip) {
    return chip->draw_flag;
}

void chip8_set_draw_flag(Chip8* chip, bool flag) {
    chip->draw_flag = flag;
}

const uint8_t* chip8_pixels(const Chip8* chip) {
    return chip->pixels;
}