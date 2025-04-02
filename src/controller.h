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

#include <stdint.h>
#include <SDL.h>

typedef enum KeyPad{
    TURBO_B     = 1 << 9,
    TURBO_A     = 1 << 8,
    RIGHT       = 1 << 7,
    LEFT        = 1 << 6,
    DOWN        = 1 << 5,
    UP          = 1 << 4,
    START       = 1 << 3,
    SELECT      = 1 << 2,
    BUTTON_B    = 1 << 1,
    BUTTON_A    = 1
} KeyPad;

typedef struct JoyPad{
    uint8_t strobe;
    uint8_t index;
    uint16_t status;
    uint8_t player;
} JoyPad;


void init_joypad(struct JoyPad* joyPad, uint8_t player);
uint8_t read_joypad(struct JoyPad* joyPad);
void write_joypad(struct JoyPad* joyPad, uint8_t data);
void update_joypad(struct JoyPad* joyPad, SDL_Event* event);
void turbo_trigger(struct JoyPad* joyPad);
void keyboard_mapper(struct JoyPad* joyPad, SDL_Event* event);
