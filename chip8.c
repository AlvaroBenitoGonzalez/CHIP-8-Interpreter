#include "chip8.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CHIP8_SHIFT_USES_VY 0
#define CHIP8_JUMP_USES_VX 0

// font
static const uint8_t CHIP8_FONT[CHIP8_FONT_SIZE] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// initialization
void chip8_init(Chip8 *chip8) {
	memset(chip8, 0, sizeof(Chip8));
	memcpy(&chip8->memory[CHIP8_FONT_START], CHIP8_FONT, CHIP8_FONT_SIZE);
	chip8->pc = CHIP8_PROGRAM_START;
	chip8->display_dirty = true;
}

// memory
uint8_t chip8_read_memory(const Chip8* chip8, uint16_t addr)
{
	return chip8->memory[addr & CHIP8_ADDR_MASK];
}

void chip8_write_memory(Chip8* chip8, uint16_t addr, uint8_t value)
{
	chip8->memory[addr & CHIP8_ADDR_MASK] = value;
}

// pc and i
uint16_t chip8_get_pc(const Chip8* chip8)
{
	return chip8->pc;
}

void chip8_set_pc(Chip8* chip8, uint16_t pc)
{
	chip8->pc = pc & CHIP8_ADDR_MASK;
}

uint16_t chip8_get_i(const Chip8* chip8)
{
	return chip8->I;
}

void chip8_set_i(Chip8* chip8, uint16_t i)
{
	chip8->I = i & CHIP8_ADDR_MASK;
}

// stack
bool chip8_push_stack(Chip8* chip8, uint16_t addr)
{
	if (chip8->sp >= CHIP8_STACK_SIZE) {
		return false;
	}
	chip8->stack[chip8->sp++] = addr;
	return true;
}

bool chip8_pop_stack(Chip8* chip8, uint16_t* addr)
{
	if (chip8->sp <= 0) {
		return false;
	}
	*addr = chip8->stack[--chip8->sp];
	return true;
}

// display
void chip8_clear_display(Chip8* chip8) 
{
	memset(chip8->display, 0, sizeof(chip8->display));
	chip8->display_dirty = true;
}

// keyboard
void chip8_set_key(Chip8* chip8, uint8_t key, bool pressed)
{
	chip8->keys[key] = pressed;
}

// timers
void chip8_tick_timers(Chip8* chip8)
{
	if (chip8->delay_timer > 0) chip8->delay_timer--;
	if (chip8->sound_timer > 0) chip8->sound_timer--;
}

bool chip8_sound_active(const Chip8* chip8)
{
	return chip8->sound_timer > 0;
}

// program loading
bool chip8_load_program(Chip8* chip8, const uint8_t* program, size_t size)
{
	if (size > CHIP8_MEMORY_SIZE - CHIP8_PROGRAM_START) {
		return false;
	}
	memcpy(&chip8->memory[CHIP8_PROGRAM_START], program, size);
	return true;
}

bool chip8_load_program_from_file(Chip8* chip8, const char* path)
{
	FILE* file = fopen(path, "rb");
	if (!file) {
		return false;
	}

	uint8_t buffer[CHIP8_MEMORY_SIZE - CHIP8_PROGRAM_START + 1];
	size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);
	bool ok = !ferror(file);
	fclose(file);

	return ok && chip8_load_program(chip8, buffer, bytesRead);
}

// fetch, decode and execute loop
uint16_t chip8_fetch(Chip8* chip8) {
	uint16_t op = (chip8_read_memory(chip8, chip8->pc) << 8) | (chip8_read_memory(chip8, chip8->pc + 1));

	chip8_set_pc(chip8, chip8->pc + 2);
	return op;
}

Instruction chip8_decode(uint16_t op) {
	Instruction instr = {
		.raw = op,
		.type = CHIP8_OP_TYPE(op),
		.x = CHIP8_OP_X(op),
		.y = CHIP8_OP_Y(op),
		.n = CHIP8_OP_N(op),
		.nn = CHIP8_OP_NN(op),
		.nnn = CHIP8_OP_NNN(op)
	};

	return instr;
}

static void chip8_execute_draw(Chip8* chip8, Instruction instr);
static void chip8_execute_arithmetic(Chip8* chip8, Instruction instr);
void chip8_execute(Chip8* chip8, Instruction instr) {
	switch (instr.type) {
		case 0x0: {
			if (instr.raw == 0x00E0) {
				chip8_clear_display(chip8);
			} else if (instr.raw == 0x00EE) {
				uint16_t addr;
				if (!chip8_pop_stack(chip8, &addr)) {
				static bool warned_under = false;
				if (!warned_under) {
					fprintf(stderr, "Stack underflow on return\n");
					warned_under = true;
				}
				break;
				}
				chip8_set_pc(chip8, addr);
			}
			break;
		}
		case 0x1:
			chip8_set_pc(chip8, instr.nnn);
			break;
		case 0x2: {
			if (!chip8_push_stack(chip8, chip8_get_pc(chip8))) {
				static bool warned_over = false;
				if (!warned_over) {
					fprintf(stderr, "Stack overflow on call\n");
					warned_over = true;
				}
				break;
			}
			chip8_set_pc(chip8, instr.nnn);
			break;
		}
		case 0x3:
			if (chip8->V[instr.x] == instr.nn)
				chip8_set_pc(chip8, chip8_get_pc(chip8) + 2);
			break;
		case 0x4:
			if (chip8->V[instr.x] != instr.nn)
				chip8_set_pc(chip8, chip8_get_pc(chip8) + 2);
			break;
		case 0x5:
			if (chip8->V[instr.x] == chip8->V[instr.y])
				chip8_set_pc(chip8, chip8_get_pc(chip8) + 2);
			break;
		case 0x6:
			chip8->V[instr.x] = instr.nn;
			break;
		case 0x7:
			chip8->V[instr.x] += instr.nn;
			break;
		case 0x8:
			chip8_execute_arithmetic(chip8, instr);
			break;
		case 0x9:
			if (chip8->V[instr.x] != chip8->V[instr.y])
					chip8_set_pc(chip8, chip8_get_pc(chip8) + 2);
			break;
		case 0xA:
			chip8_set_i(chip8, instr.nnn);
			break;
		case 0xB:
			if (CHIP8_JUMP_USES_VX) {
				chip8_set_pc(chip8, instr.nnn + chip8->V[instr.x]);
			} else {
				chip8_set_pc(chip8, instr.nnn + chip8->V[0]);
			}
			break;
		case 0xC:
			chip8->V[instr.x] = (rand() % 256) & instr.nn;
			break;
		case 0xD:
			chip8_execute_draw(chip8, instr);
			break;
		case 0xE:
			if (instr.nn == 0x9E) {
				if (chip8->keys[chip8->V[instr.x] & 0xF]) {
					chip8_set_pc(chip8, chip8_get_pc(chip8) + 2);
				}
			} else if (instr.nn == 0xA1) {
				if (!chip8->keys[chip8->V[instr.x] & 0xF]) {
					chip8_set_pc(chip8, chip8_get_pc(chip8) + 2);
				}
			}
			break;
		case 0xF:
			switch (instr.nn) {
				case 0x07:
					chip8->V[instr.x] = chip8->delay_timer;
					break;
				case 0x15:
					chip8->delay_timer = chip8->V[instr.x];
					break;
				case 0x18:
					chip8->sound_timer = chip8->V[instr.x];
					break;
				case 0x1E: {
					uint16_t result = chip8_get_i(chip8) + chip8->V[instr.x];
					chip8->V[CHIP8_VF] = result > 0xFFF;
					chip8_set_i(chip8, result);
					break;
				}
				case 0x0A: {
					bool key_pressed = false;
					for (uint8_t i = 0; i < CHIP8_NUM_KEYS; i++) {
						if (chip8->keys[i]) {
							chip8->V[instr.x] = i;
							key_pressed = true;
							break;
						}
					}
					
					if (!key_pressed) {
						chip8_set_pc(chip8, chip8_get_pc(chip8) - 2);
					}
					break;
				}
				case 0x29:
					chip8_set_i(chip8, CHIP8_FONT_START + (chip8->V[instr.x] & 0xF) * CHIP8_FONT_CHAR_BYTES);
					break;
				case 0x33: {
					uint8_t value = chip8->V[instr.x];
					
					chip8_write_memory(chip8, chip8_get_i(chip8), value / 100);
					chip8_write_memory(chip8, chip8_get_i(chip8) + 1, (value / 10) % 10);
					chip8_write_memory(chip8, chip8_get_i(chip8) + 2, value % 10);
					break;
				}
				case 0x55:
					for (uint8_t i = 0; i <= instr.x; i++) {
						chip8_write_memory(chip8, chip8_get_i(chip8) + i, chip8->V[i]);
					}
					break;
				case 0x65:
					for (uint8_t i = 0; i <= instr.x; i++) {
						chip8->V[i] = chip8_read_memory(chip8, chip8_get_i(chip8) + i);
					}
					break;
			}
		break;
	}
}

void chip8_step(Chip8* chip8) {
	uint16_t op = chip8_fetch(chip8);
	Instruction instr = chip8_decode(op);
	chip8_execute(chip8, instr);
}



static void chip8_execute_draw(Chip8* chip8, Instruction instr) {
	uint8_t start_x = chip8->V[instr.x] % CHIP8_DISPLAY_WIDTH;
	uint8_t start_y = chip8->V[instr.y] % CHIP8_DISPLAY_HEIGHT;
	chip8->V[CHIP8_VF] = 0;

	for (uint8_t row = 0; row < instr.n; row++) {
		uint8_t y = (start_y + row);
		if (y >= CHIP8_DISPLAY_HEIGHT) break;

		uint8_t sprite_byte = chip8_read_memory(chip8, chip8->I + row);
		for (uint8_t col = 0; col < 8; col++) {
			uint8_t x = start_x + col;
			if (x >= CHIP8_DISPLAY_WIDTH) break;

			if (sprite_byte & (0x80 >> col)) {
				bool* pixel = &chip8->display[y * CHIP8_DISPLAY_WIDTH + x];
				if (*pixel) chip8->V[CHIP8_VF] = 1;
				*pixel = !*pixel;
			}
		}
	}

	chip8->display_dirty = true;
}

static void chip8_execute_arithmetic(Chip8* chip8, Instruction instr) {
	uint8_t vx = chip8->V[instr.x];
	uint8_t vy = chip8->V[instr.y];

	switch (instr.n) {
		case 0x0:
			chip8->V[instr.x] = chip8->V[instr.y];
			break;
		case 0x1:
			chip8->V[instr.x] |= chip8->V[instr.y];
			break;
		case 0x2:
			chip8->V[instr.x] &= chip8->V[instr.y];
			break;
		case 0x3:
			chip8->V[instr.x] ^= chip8->V[instr.y];
			break;
		case 0x4: {
			uint16_t sum = vx + vy;
			chip8->V[instr.x] = (uint8_t)(sum & 0xFF);
			chip8->V[CHIP8_VF] = sum > 0xFF;
			break;
		}
		case 0x5:
			chip8->V[instr.x] = vx - vy;
			chip8->V[CHIP8_VF] = vx >= vy;
			break;
		case 0x6: {
			uint8_t valueR = CHIP8_SHIFT_USES_VY ? vy : vx;

			chip8->V[instr.x] = valueR >> 1;
			chip8->V[CHIP8_VF] = valueR & 0x1;
			break;
		}
		case 0x7:
			chip8->V[instr.x] = vy - vx;
			chip8->V[CHIP8_VF] = (vy >= vx) ? 1 : 0;
			break;
		case 0xE: {
			uint8_t valueL = CHIP8_SHIFT_USES_VY ? vy : vx;

			chip8->V[instr.x] = valueL << 1;
			chip8->V[CHIP8_VF] = valueL >> 7;
			break;
		}
	}
}