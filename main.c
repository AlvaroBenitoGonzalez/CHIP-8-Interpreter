#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "chip8.h"

#define WINDOW_SCALE 15
#define COLOR_ON 0xFFFFFFFF
#define COLOR_OFF 0x000000FF

#define AUDIO_FREQUENCY 44100
#define SAMPLES_PER_FRAME (AUDIO_FREQUENCY / CHIP8_TIMER_FREQUENCY)
#define BYTES_PER_SAMPLE 2

// keyboard
static const SDL_Scancode CHIP8_KEYMAP[CHIP8_NUM_KEYS] = {
    SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, // 0, 1, 2, 3
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A, // 4, 5, 6, 7
    SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C, // 8, 9, A, B
    SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V  // C, D, E, F
};

static void beep(SDL_AudioStream* stream, bool on) {
    static unsigned int phase = 0;
    if (!stream) return;
    if (!on) { SDL_ClearAudioStream(stream); return; }
    if (SDL_GetAudioStreamQueued(stream) < SAMPLES_PER_FRAME * BYTES_PER_SAMPLE * 2) {
        int16_t buffer[SAMPLES_PER_FRAME];
        for (int i = 0; i < SAMPLES_PER_FRAME; i++) {
            buffer[i] = ((phase++ / 50) % 2) ? 3000 : -3000;
        }
        SDL_PutAudioStreamData(stream, buffer, sizeof(buffer));
    }
}

int main (int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s rom.ch8 [ips]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int ips = CHIP8_DEFAULT_IPS;
    if (argc >= 3) {
        ips = atoi(argv[2]);
        if (ips <= 0) {
            fprintf(stderr, "Invalid IPS value: %s\n", argv[2]);
            return EXIT_FAILURE;
        }
    }

    Chip8 chip8;
    chip8_init(&chip8);
    srand((unsigned)time(NULL));

    if (!chip8_load_program_from_file(&chip8, argv[1])) {
        fprintf(stderr, "Failed to load program: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    if (!SDL_CreateWindowAndRenderer("Chip8 Emulator", CHIP8_DISPLAY_WIDTH * WINDOW_SCALE,
        CHIP8_DISPLAY_HEIGHT * WINDOW_SCALE, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        fprintf(stderr, "Failed to create window and renderer: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_SetRenderLogicalPresentation(renderer, CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING, CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT);
    
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, 44100 };
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);

    if (stream) SDL_ResumeAudioStreamDevice(stream);
    else fprintf(stderr, "Failed to open audio stream: %s\n", SDL_GetError());

    static uint32_t pixels[CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT];
    double cpu_acc = 0.0, timer_acc = 0.0;
    uint64_t last_time = SDL_GetTicksNS();
    bool running = true;

    while (running) {
        // input
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) running = false;
                for (int i = 0; i < CHIP8_NUM_KEYS; i++) {
                    if (event.key.scancode == CHIP8_KEYMAP[i])
                        chip8_set_key(&chip8, i, event.type == SDL_EVENT_KEY_DOWN);
                }
            }
        }

        // time passed
        uint64_t current_time = SDL_GetTicksNS();
        double delta_time = (current_time - last_time) / 1e9;
        last_time = current_time;
        if (delta_time > 0.1) delta_time = 0.1;

        // ips
        cpu_acc += delta_time * ips;
        while (cpu_acc >= 1.0) { chip8_step(&chip8); cpu_acc -= 1.0; }

        // timers
        timer_acc += delta_time * CHIP8_TIMER_FREQUENCY;
        int ticks = 0;
        while (timer_acc >= 1.0) {
            chip8_tick_timers(&chip8);
            timer_acc -= 1.0; ticks++;
        }

        // display
        if (ticks > 0) {
            if (chip8.display_dirty) {
                for (int i = 0; i < CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT; i++) {
                    pixels[i] = chip8.display[i] ? COLOR_ON : COLOR_OFF;
                }
                SDL_UpdateTexture(texture, NULL, pixels, CHIP8_DISPLAY_WIDTH * (int) sizeof(uint32_t));
                chip8.display_dirty = false;
            }

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);

            beep(stream, chip8_sound_active(&chip8));
        }

        SDL_Delay(1);
    }

    // end
    if (stream) {
        SDL_DestroyAudioStream(stream);
        stream = NULL;
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}