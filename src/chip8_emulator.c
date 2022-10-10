#include "chip8_emulator.h"

#include <SDL_events.h>
#include <SDL_render.h>
#include <SDL_timer.h>

struct Chip8Emulator {
    Chip8* chip;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint8_t key_state[CHIP8_KEY_COUNT];
    float fps;
    bool running;
};

static const int key_map[] = {
    SDL_SCANCODE_X,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_C,
    SDL_SCANCODE_4,
    SDL_SCANCODE_R,
    SDL_SCANCODE_F,
    SDL_SCANCODE_V
};

static void handle_window_event(Chip8Emulator* emulator, SDL_WindowEvent* event) {
    switch (event->event)
    {
    case SDL_WINDOWEVENT_RESIZED:
        chip8_set_draw_flag(emulator->chip, true);
        break;
    }
}

static void handle_key_down_event(Chip8Emulator* emulator, SDL_KeyboardEvent* event) {
    for (uint8_t i = 0; i < CHIP8_KEY_COUNT; i++) {
        if (event->keysym.scancode == key_map[i]) {
            emulator->key_state[i] = 1;
            break;
        }
    }
}

static void handle_key_up_event(Chip8Emulator* emulator, SDL_KeyboardEvent* event) {
    for (uint8_t i = 0; i < CHIP8_KEY_COUNT; i++) {
        if (event->keysym.scancode == key_map[i]) {
            emulator->key_state[i] = 0;
            break;
        }
    }
}

static void handle_events(Chip8Emulator* emulator) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type)
        {
        case SDL_QUIT:
            emulator->running = false;
            break;
        case SDL_WINDOWEVENT:
            handle_window_event(emulator, &event.window);
            break;
        case SDL_KEYDOWN:
            handle_key_down_event(emulator, &event.key);
            break;
        case SDL_KEYUP:
            handle_key_up_event(emulator, &event.key);
            break;
        }
    }
}

static void display(Chip8Emulator* emulator) {
    uint32_t data[CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT];
    const uint8_t* pixels = chip8_pixels(emulator->chip);

    if (!chip8_draw_flag(emulator->chip)) {
        return;
    }

    for (size_t i = 0; i < CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT; i++) {
        data[i] = (0xFFFFFF00 * pixels[i]) | 0x000000FF;
    }

    SDL_RenderClear(emulator->renderer);
    SDL_UpdateTexture(emulator->texture, NULL, data, CHIP8_SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderCopy(emulator->renderer, emulator->texture, NULL, NULL);
    SDL_RenderPresent(emulator->renderer);
    chip8_set_draw_flag(emulator->chip, false);
}

Chip8Emulator* chip8_emulator_create() {
    Chip8Emulator* emulator = NULL;

    if ((emulator = SDL_malloc(sizeof(Chip8Emulator))) == NULL) {
        return NULL;
    }

    if ((emulator->chip = chip8_create(emulator->key_state)) == NULL) {
        SDL_free(emulator);
        return NULL;
    }

    if ((emulator->window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE)) == NULL) {
        chip8_destroy(emulator->chip);
        SDL_free(emulator);
        return NULL;
    }

    if ((emulator->renderer = SDL_CreateRenderer(emulator->window, -1, 0)) == NULL) {
        chip8_destroy(emulator->chip);
        SDL_DestroyWindow(emulator->window);
        SDL_free(emulator);
        return NULL;
    }

    if ((emulator->texture = SDL_CreateTexture(emulator->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, CHIP8_SCREEN_WIDTH, CHIP8_SCREEN_HEIGHT)) == NULL) {
        chip8_destroy(emulator->chip);
        SDL_DestroyWindow(emulator->window);
        SDL_DestroyRenderer(emulator->renderer);
        SDL_free(emulator);
        return NULL;
    }

    SDL_memset(emulator->key_state, 0, sizeof(emulator->key_state));
    emulator->fps = 60.0f;
    emulator->running = true;

    return emulator;
}

void chip8_emulator_destroy(Chip8Emulator* emulator) {
    if (emulator == NULL) {
        return;
    }

    chip8_destroy(emulator->chip);
    SDL_DestroyTexture(emulator->texture);
    SDL_DestroyRenderer(emulator->renderer);
    SDL_DestroyWindow(emulator->window);
}

int chip8_emulator_load_rom_file(Chip8Emulator* emulator, const char* path) {
    if (emulator == NULL) {
        return SDL_InvalidParamError("emulator");
    }

    return chip8_load_rom_file(emulator->chip, path);
}

void chip8_emulator_run(Chip8Emulator* emulator) {
    const float frame_duration = 1000.0f / chip8_emulator_fps(emulator);
    const int timer_update_fps = (int)(chip8_emulator_fps(emulator) / 60.0);
    int frames = 0;

    while (emulator->running) {
        Uint64 start = SDL_GetPerformanceCounter();

        chip8_execute(emulator->chip);

        if (frames == timer_update_fps) {
            chip8_update_timers(emulator->chip);
            frames = 0;
        }

        frames += 1;

        handle_events(emulator);
        display(emulator);

        Uint64 end = SDL_GetPerformanceCounter();
        float elapsed = (float)(end - start) / (float)SDL_GetPerformanceFrequency();

        if (elapsed < frame_duration) {
            SDL_Delay((Uint32) (frame_duration - elapsed));
        }
    }
}

float chip8_emulator_fps(const Chip8Emulator* emulator) {
    return emulator->fps;
}

void chip8_emulator_set_fps(Chip8Emulator* emulator, float fps) {
    if (fps <= 0.0) {
        SDL_SetError("Non positive FPS is invalid");
        return;
    }

    emulator->fps = fps;
}