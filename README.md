# CHIP-8 Emulator

A CHIP-8 interpreter written in C, with a small SDL3 frontend for video, input, and audio.

The emulator core (`chip8.c` / `chip8.h`) has no SDL dependency, so it can be reused with any frontend. `main.c` is a thin SDL3 wrapper around it.

## Features

- Full original CHIP-8 instruction set (35 opcodes)
- Configurable execution speed (default: 700 instructions per second)
- 60 Hz delay and sound timers
- 64x32 display, resizable window with letterboxed, nearest-neighbour scaling
- Square-wave beep while the sound timer is active
- Standard QWERTY keypad mapping
- Selectable quirks for shift and jump behaviour (compile-time)
- Bounds-safe memory access, and stack overflow/underflow warnings instead of crashes

## Requirements

- A C99 compiler (GCC or Clang)
- [SDL3](https://github.com/libsdl-org/SDL)

## Building

There is no build system yet, so compile directly. With `pkg-config`:

```sh
gcc -std=c99 -O2 -Wall -Wextra main.c chip8.c -o chip8 $(pkg-config --cflags --libs sdl3)
```

Or, if SDL3 is installed in a standard location:

```sh
gcc -std=c99 -O2 main.c chip8.c -o chip8 -lSDL3
```

## Usage

```sh
./chip8 <rom.ch8> [ips]
```

| Argument | Description |
|----------|-------------|
| `rom.ch8` | Path to a CHIP-8 ROM (max 3584 bytes) |
| `ips` | Optional. Instructions per second (default `700`) |

Examples:

```sh
./chip8 roms/pong.ch8
./chip8 roms/pong.ch8 1000
```

Different ROMs expect different speeds. If a game feels too fast or too slow, adjust `ips`.

## Controls

The original CHIP-8 hex keypad is mapped onto the left side of a QWERTY keyboard:

```
CHIP-8 keypad        Keyboard
+---+---+---+---+    +---+---+---+---+
| 1 | 2 | 3 | C |    | 1 | 2 | 3 | 4 |
| 4 | 5 | 6 | D |    | Q | W | E | R |
| 7 | 8 | 9 | E |    | A | S | D | F |
| A | 0 | B | F |    | Z | X | C | V |
+---+---+---+---+    +---+---+---+---+
```

Press `Esc` to quit.

## Quirks and behaviour

CHIP-8 has several instructions whose behaviour differs between interpreters. This emulator makes the following choices:

| Behaviour | Setting |
|-----------|---------|
| `8XY6` / `8XYE` (shift) | Shifts `VX` in place (modern behaviour). Set `CHIP8_SHIFT_USES_VY` to `1` in `chip8.c` to shift `VY` into `VX` like the original COSMAC VIP |
| `BNNN` (jump with offset) | Jumps to `NNN + V0`. Set `CHIP8_JUMP_USES_VX` to `1` to use `BXNN` (`NNN + VX`) as in CHIP-48/SUPER-CHIP |
| `DXYN` (draw) | The starting position wraps, but sprites are clipped at the screen edges rather than wrapping |
| `FX55` / `FX65` | `I` is left unchanged |
| `FX1E` | Sets `VF` to 1 if `I` overflows past `0xFFF` |
| `FX0A` | Blocks until a key is pressed |
| Stack | 16 levels. Overflow and underflow print a warning to `stderr` once and the instruction is skipped |

If a ROM misbehaves, try flipping the quirk macros at the top of `chip8.c` and rebuilding.

## Project structure

```
.
├── chip8.h    # Emulator state, constants, and public API
├── chip8.c    # CPU core: fetch/decode/execute, memory, stack, timers
└── main.c     # SDL3 frontend: window, input, audio, main loop
```

## Using the core in your own frontend

The core exposes a small API. A minimal frontend only needs to do the following:

```c
#include "chip8.h"

Chip8 chip8;
chip8_init(&chip8);
chip8_load_program_from_file(&chip8, "game.ch8");

// In your main loop:
chip8_set_key(&chip8, key, pressed);   // feed input (key 0x0-0xF)
chip8_step(&chip8);                    // run one instruction (call ~700x/sec)
chip8_tick_timers(&chip8);             // call at 60 Hz

if (chip8.display_dirty) {
    // render chip8.display (64x32 array of bool)
    chip8.display_dirty = false;
}

if (chip8_sound_active(&chip8)) {
    // play a tone
}
```

| Function | Purpose |
|----------|---------|
| `chip8_init` | Reset state and load the built-in font |
| `chip8_load_program` / `chip8_load_program_from_file` | Load a ROM at `0x200` |
| `chip8_step` | Fetch, decode, and execute one instruction |
| `chip8_tick_timers` | Decrement the delay and sound timers (call at 60 Hz) |
| `chip8_set_key` | Update the state of a keypad key |
| `chip8_sound_active` | Whether the buzzer should be sounding |

`main.c` runs the CPU and timers using time accumulators, so emulation speed is independent of the frame rate. The display is refreshed on each 60 Hz tick.

## Testing

A good way to check correctness is to run the community test ROMs, such as [Timendus's chip8-test-suite](https://github.com/Timendus/chip8-test-suite), which covers opcodes, flags, and quirks.

## Limitations

- No SUPER-CHIP or XO-CHIP extensions (128x64 mode, scrolling, etc.)
- Quirks are compile-time options rather than runtime flags
- No built-in debugger, pause, or reset
- Pixel colours and window scale are set by constants in `main.c`

## Customisation

These constants at the top of `main.c` are easy to tweak:

| Constant | Default | Meaning |
|----------|---------|---------|
| `WINDOW_SCALE` | `15` | Initial window size multiplier (960x480) |
| `COLOR_ON` / `COLOR_OFF` | white / black | Pixel colours (RGBA8888) |

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).