#include "chip8.h"

#include <stdlib.h>

#include <SDL_events.h>
#include <SDL_log.h>
#include <SDL_render.h>
#include <SDL_timer.h>

#define SCREEN_WIDTH 32
#define SCREEN_HEIGHT 64
#define FPS 60.0f

enum Chip8InstructionError {
    CHIP8_ERROR_NONE = 0,
    CHIP8_INVALID_INSTRUCTION
};

struct Chip8 {
    uint8_t ram[4096];
    uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint8_t gfx[SCREEN_WIDTH][SCREEN_HEIGHT];
    uint16_t stack[32];
    uint8_t V[16];
    uint16_t I;
    uint16_t PC;
    uint16_t SP;
    uint8_t delay_timer;
    uint8_t sound_timer;
    SDL_bool draw_flag;
    SDL_bool running;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
};

static uint16_t fetch_opcode(const Chip8* chip) {
    uint8_t hb = chip->ram[chip->PC];
    uint8_t lb = chip->ram[chip->PC + 1];

    return hb << 8 | lb;
}

static void push(Chip8* chip, uint16_t value) {
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "PUSH %x", value);
    chip->stack[chip->SP++] = value;
}

static uint16_t pop(Chip8* chip) {
    uint16_t value = chip->stack[--chip->SP];
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "POP %x", value);
    return value;
}

static int exec0(Chip8* chip, uint16_t opcode) {
    switch (opcode)
	{
    case 0x00E0:
        SDL_memset(chip->gfx, 0, sizeof(chip->gfx));
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x00EE:
        chip->PC = pop(chip) + 2;
        return CHIP8_ERROR_NONE;
    default:
        return CHIP8_INVALID_INSTRUCTION;
    }
}

static int exec1(Chip8* chip, uint16_t opcode) {
    chip->PC = opcode & 0x0FFF;

    return CHIP8_ERROR_NONE;
}

static int exec2(Chip8* chip, uint16_t opcode) {
    push(chip, chip->PC);
    chip->PC = opcode & 0x0FFF;

    return CHIP8_ERROR_NONE;
}

static int exec3(Chip8* chip, uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint16_t value = opcode & 0x00FF;

    if (chip->V[reg] == value) {
        chip->PC += 4;
    } else {
        chip->PC += 2;
    }

    return CHIP8_ERROR_NONE;
}

static int exec4(Chip8* chip, uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint16_t value = opcode & 0x00FF;

    if (chip->V[reg] != value) {
        chip->PC += 4;
    } else {
        chip->PC += 2;
    }

    return CHIP8_ERROR_NONE;
}

static int exec5(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    if (chip->V[x] == chip->V[y]) {
        chip->PC += 4;
    } else {
        chip->PC += 2;
    }

    return CHIP8_ERROR_NONE;
}

static int exec6(Chip8* chip, uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t value = opcode & 0x00FF;

    chip->V[reg] = value;
    chip->PC += 2;

    return CHIP8_ERROR_NONE;
}

static int exec7(Chip8* chip, uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t value = opcode & 0x00FF;

    chip->V[reg] += value;
    chip->PC += 2;

    return CHIP8_ERROR_NONE;
}

static int exec8(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    switch (opcode & 0x000F)
    {
    case 0x0000:
        chip->V[x] = chip->V[y];
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0001:
        chip->V[x] |= chip->V[y];
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0002:
        chip->V[x] &= chip->V[y];
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0003:
        chip->V[x] ^= chip->V[y];
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0004:
        chip->V[0xF] = (chip->V[x] > 0xFF - chip->V[y] ? 0 : 1);
        chip->V[x] += chip->V[y];
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0005:
        chip->V[0xF] = (chip->V[y] > chip->V[x] ? 0 : 1);
        chip->V[x] -= chip->V[y];
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0006:
        chip->V[0xF] = chip->V[x] & 0x1;
        chip->V[x] >>= 1;
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0007:
        chip->V[0xF] = (chip->V[x] > chip->V[y] ? 0 : 1);
        chip->V[x] = chip->V[y] - chip->V[x];
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x000E:
        chip->V[0xF] = chip->V[x] >> 7;
        chip->V[x] <<= 1;
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    default:
        return CHIP8_INVALID_INSTRUCTION;
    }
}

static int exec9(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    if (chip->V[x] != chip->V[y]) {
        chip->PC += 4;
    } else {
        chip->PC += 2;
    }

    return CHIP8_ERROR_NONE;
}

static int execA(Chip8* chip, uint16_t opcode) {
    chip->I = opcode & 0x0FFF;
    chip->PC += 2;

    return CHIP8_ERROR_NONE;
}

static int execB(Chip8* chip, uint16_t opcode) {
    chip->PC = chip->V[0] + opcode & 0x0FFF;
    chip->PC += 2;

    return CHIP8_ERROR_NONE;
}

static int execC(Chip8* chip, uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t random = rand() % 256;
    uint8_t value = opcode & 0x00FF;

    chip->V[reg] = random & value;
    chip->PC += 2;

    return CHIP8_ERROR_NONE;
}

static int execD(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t height = opcode & 0x000F;

    chip->V[0xF] = 0;

    for (uint8_t i = 0; i < height; i++) {
        uint8_t pixel = chip->ram[chip->I + i];

        for (uint8_t j = 0; j < 8; j++) {
            if ((pixel & (0x80 >> j)) != 0) {
                if (chip->gfx[chip->V[y] + i][chip->V[x] + j] == 1) {
                    chip->V[0xF] = 1;
                }
                chip->gfx[chip->V[y] + i][chip->V[x] + j] ^= 1;
            }
        }
    }

    for (uint8_t i = 0; i < SCREEN_WIDTH; i++) {
        for (uint8_t j = 0; j < SCREEN_HEIGHT; j++) {
            chip->pixels[(i * SCREEN_HEIGHT) + j] = (0xFFFFFF00 * chip->gfx[i][j]) | 0x000000FF;
        }
    }

    chip->PC += 2;
    chip->draw_flag = SDL_TRUE;

    return CHIP8_ERROR_NONE;
}

static const int key_map[] = {
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_4,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_R,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_F,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_X,
    SDL_SCANCODE_C,
    SDL_SCANCODE_V
};

static int execE(Chip8* chip, uint16_t opcode) {
    const Uint8* state = SDL_GetKeyboardState(NULL);
    uint8_t x = (opcode & 0x0F00) >> 8;

    switch (opcode & 0x00FF)
    {
    case 0x009E:
        if (state[key_map[chip->V[x]]] == 1) {
            chip->PC += 4;
        } else {
            chip->PC += 2;
        }

        return CHIP8_ERROR_NONE;
    case 0x00A1:
        if (state[key_map[chip->V[x]]] == 0) {
            chip->PC += 4;
        } else {
            chip->PC += 2;
        }

        return CHIP8_ERROR_NONE;
    default:
        return CHIP8_INVALID_INSTRUCTION;
    }
}

static uint8_t wait_key_press() {
    SDL_Event event;

    while (1) {
        SDL_WaitEvent(&event);

        switch (event.type)
        {
        case SDL_KEYDOWN:
            for (uint8_t i = 0; i < 16; i++) {
                if (event.key.keysym.scancode == key_map[i]) {
                    return i;
                }
            }
        }
    }
}

static int execF(Chip8* chip, uint16_t opcode) {
    uint8_t x = (opcode & 0x0F00) >> 8;

    switch (opcode & 0x00FF)
    {
    case 0x0007:
        chip->V[x] = chip->delay_timer;
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x000A:
        chip->V[x] = wait_key_press();
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0015:
        chip->delay_timer = chip->V[x];
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0018:
        chip->sound_timer = chip->V[x];
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x001E:
        chip->I += chip->V[x];
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0029:
        chip->I = chip->V[x] * 0x05;
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0033:
        chip->ram[chip->I] = chip->V[x] / 100;
        chip->ram[chip->I + 1] = (chip->V[x] / 10) % 10;
        chip->ram[chip->I + 2] = (chip->V[x] % 100) % 10;
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0055:
        SDL_memcpy(&chip->ram[chip->I], chip->V, x + 1);
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    case 0x0065:
        SDL_memcpy(chip->V, &chip->ram[chip->I], x + 1);
        chip->PC += 2;
        return CHIP8_ERROR_NONE;
    default:
        return CHIP8_INVALID_INSTRUCTION;
    }
}

static int exec(Chip8* chip) {
    uint16_t opcode = fetch_opcode(chip);
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Opcode: %x", opcode);

    switch (opcode & 0xF000)
    {
    case 0x0000:
        return exec0(chip, opcode);
    case 0x1000:
        return exec1(chip, opcode);
    case 0x2000:
        return exec2(chip, opcode);
    case 0x3000:
        return exec3(chip, opcode);
    case 0x4000:
        return exec4(chip, opcode);
    case 0x5000:
        return exec5(chip, opcode);
    case 0x6000:
        return exec6(chip, opcode);
    case 0x7000:
        return exec7(chip, opcode);
    case 0x8000:
        return exec8(chip, opcode);
    case 0x9000:
        return exec9(chip, opcode);
    case 0xA000:
        return execA(chip, opcode);
    case 0xB000:
        return execB(chip, opcode);
    case 0xC000:
        return execC(chip, opcode);
    case 0xD000:
        return execD(chip, opcode);
    case 0xE000:
        return execE(chip, opcode);
    case 0xF000:
        return execF(chip, opcode);
    default:
        return CHIP8_INVALID_INSTRUCTION;
    }
}

static void display(Chip8* chip) {
    if (!chip->draw_flag) {
        return;
    }

    SDL_RenderClear(chip->renderer);
    SDL_UpdateTexture(chip->texture, NULL, chip->pixels, SCREEN_HEIGHT * sizeof(uint32_t));
    SDL_RenderCopy(chip->renderer, chip->texture, NULL, NULL);
    SDL_RenderPresent(chip->renderer);
    chip->draw_flag = SDL_FALSE;
}

static void handle_window_event(Chip8* chip, SDL_WindowEvent* event) {
    switch (event->event)
    {
    case SDL_WINDOWEVENT_RESIZED:
        chip->draw_flag = SDL_TRUE;
        break;
    }
}

static void handle_events(Chip8* chip) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type)
        {
        case SDL_QUIT:
            chip->running = SDL_FALSE;
            break;
        case SDL_WINDOWEVENT:
            handle_window_event(chip, &event.window);
            break;
        }
    }
}

Chip8* chip8_create() {
    Chip8* chip = NULL;

    if ((chip = SDL_malloc(sizeof(Chip8))) == NULL) {
        return NULL;
    }

    if ((chip->window = SDL_CreateWindow("CHIP8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE)) == NULL) {
        chip8_destroy(chip);
        return NULL;
    }

    if ((chip->renderer = SDL_CreateRenderer(chip->window, -1, SDL_RENDERER_ACCELERATED)) == NULL) {
        chip8_destroy(chip);
        return NULL;
    }

    if ((chip->texture = SDL_CreateTexture(chip->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 64, 32)) == NULL) {
        chip8_destroy(chip);
        return NULL;
    }

    return chip;
}

void chip8_destroy(Chip8* chip) {
    if (chip == NULL) {
        return;
    }

    SDL_DestroyTexture(chip->texture);
    SDL_DestroyRenderer(chip->renderer);
    SDL_DestroyWindow(chip->window);
    SDL_free(chip);
}

int chip8_load_rom(Chip8* chip, const char* path) {
    SDL_RWops* f = NULL;

    if ((f = SDL_RWFromFile(path, "r+b")) == NULL) {
        return 0;
    }

    chip->I = 0;
    chip->PC = 0x200;
    chip->SP = 0;
    chip->draw_flag = SDL_FALSE;
    chip->running = SDL_TRUE;

    SDL_RWread(f, chip->ram + chip->PC, sizeof(uint8_t), sizeof(chip->ram) - chip->PC);
    SDL_RWclose(f);

    return 1;
}

void chip8_run(Chip8* chip) {
    const float frame_duration = 1000.0f / FPS;
    Uint64 start, end;
    float elapsed;

    while (chip->running) {
        start = SDL_GetPerformanceCounter();

        exec(chip);
        handle_events(chip);
        display(chip);

        end = SDL_GetPerformanceCounter();
        elapsed = (float)(end - start) / (float)SDL_GetPerformanceFrequency();

        if (elapsed < frame_duration) {
            SDL_Delay((Uint32) (frame_duration - elapsed));
        }
    }
}