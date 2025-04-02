/*
 * NES Emulator
 * Copyright (C) 2025  filipemd
 * 
 * This file is part of NES Emulator.
 * 
 * NES Emulator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * NES Emulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with NES Emulator.  If not, see <https://www.gnu.org/licenses/>.
 */
/*
 * Copyright (c) 2023 Emmanuel Obara
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#pragma once

#include "cpu6502.h"
#include "ppu.h"
#include "mmu.h"
#include "apu.h"
#include "mapper.h"
#include "gfx.h"
#include "timers.h"


// frame rate in Hz
#define NTSC_FRAME_RATE 60
#define PAL_FRAME_RATE 50

// turbo keys toggle rate (Hz)
// value should be a factor of FRAME_RATE
// and should never exceed FRAME_RATE for best result
#define NTSC_TURBO_RATE 30
#define PAL_TURBO_RATE 25

// sleep time when emulator is paused in milliseconds
#define IDLE_SLEEP 50


typedef struct Emulator{
    c6502 cpu;
    PPU ppu;
    APU apu;
    Memory mem;
    Mapper mapper;
    GraphicsContext g_ctx;
    Timer timer;

    TVSystem type;

    double time_diff;

    uint8_t exit;
    uint8_t pause;
} Emulator;


void init_emulator(Emulator* emulator, int argc, char *argv[]);
void reset_emulator(Emulator* emulator);
void run_emulator(Emulator* emulator);
void run_NSF_player(Emulator* emulator);
void free_emulator(Emulator* emulator);
