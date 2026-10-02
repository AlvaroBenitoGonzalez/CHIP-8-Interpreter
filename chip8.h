#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// memory
#define CHIP8_MEMORY_SIZE 4096
#define CHIP8_ADDR_MASK 0xFFF
#define CHIP8_PROGRAM_START 0x200

// font
#define CHIP8_FONT_START 0x050
#define CHIP8_FONT_CHAR_BYTES 5
#define CHIP8_FONT_SIZE (CHIP8_FONT_CHAR_BYTES * 16)

// display
#define CHIP8_DISPLAY_WIDTH 64
#define CHIP8_DISPLAY_HEIGHT 32

// registers and stack
#define CHIP8_NUM_REGS 16
#define CHIP8_VF 0xF
#define CHIP8_STACK_SIZE 16

// keyboard and timers
#define CHIP8_NUM_KEYS 16
#define CHIP8_TIMER_FREQUENCY 60
#define CHIP8_DEFAULT_IPS 700

// decoding
#define CHIP8_OP_TYPE(op)   (((op) >> 12) & 0xF)
#define CHIP8_OP_X(op)      (((op) >> 8) & 0xF)
#define CHIP8_OP_Y(op)      (((op) >> 4) & 0xF)
#define CHIP8_OP_N(op)      ((op) & 0xF)
#define CHIP8_OP_NN(op)     ((op) & 0xFF)
#define CHIP8_OP_NNN(op)    ((op) & 0xFFF)

typedef struct {
    uint16_t raw;
    uint8_t type;
    uint8_t x, y, n;
    uint8_t nn;
    uint16_t nnn;
} Instruction;

typedef struct {
    uint8_t memory[CHIP8_MEMORY_SIZE];
    uint16_t pc;

    bool display[CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT];
    bool display_dirty;

    uint8_t V[CHIP8_NUM_REGS];
    uint16_t I;

    uint16_t stack[CHIP8_STACK_SIZE];
    uint8_t sp;

    uint8_t delay_timer;
    uint8_t sound_timer;
    
    bool keys[CHIP8_NUM_KEYS];
} Chip8;

// initialization
void chip8_init(Chip8 *chip8);

// memory
uint8_t chip8_read_memory(const Chip8* chip8, uint16_t addr);
void chip8_write_memory(Chip8* chip8, uint16_t addr, uint8_t value);

// pc and i
uint16_t chip8_get_pc(const Chip8* chip8);
void chip8_set_pc(Chip8* chip8, uint16_t pc);
uint16_t chip8_get_i(const Chip8* chip8);
void chip8_set_i(Chip8* chip8, uint16_t i);

// stack
bool chip8_push_stack(Chip8* chip8, uint16_t addr);
bool chip8_pop_stack(Chip8* chip8, uint16_t* addr);

// display
void chip8_clear_display(Chip8* chip8);

// keyboard
void chip8_set_key(Chip8* chip8, uint8_t key, bool pressed);

// timers
void chip8_tick_timers(Chip8* chip8);
bool chip8_sound_active(const Chip8* chip8);

// program loading
bool chip8_load_program(Chip8* chip8, const uint8_t* program, size_t size);
bool chip8_load_program_from_file(Chip8* chip8, const char* path);

// fetch, decode and execute loop
uint16_t chip8_fetch(Chip8* chip8);
Instruction chip8_decode(uint16_t op);
void chip8_execute(Chip8* chip8, Instruction instr);
void chip8_step(Chip8* chip8);

#endif // CHIP8_H