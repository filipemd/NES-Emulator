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

#ifdef __ANDROID__

#include <stdint.h>
#include <SDL2/SDL.h>

#include "gfx.h"
#include "controller.h"

typedef struct TouchAxis{
    int x;
    int y;
    int r;
    int inner_x;
    int inner_y;
    int origin_x;
    int origin_y;
    uint8_t state;
    uint8_t active;
    uint8_t h_latch;
    uint8_t v_latch;
    SDL_FingerID finger;
    SDL_Rect bg_dest;
    SDL_Rect joy_dest;
    SDL_Texture* bg_tx;
    SDL_Texture* joy_tx;
} TouchAxis;


typedef struct  TouchButton{
    SDL_Texture * texture;
    SDL_Rect dest;
    KeyPad id;
    int x;
    int y;
    int r;
    uint8_t active;
    uint8_t auto_render;
    SDL_FingerID finger;
} TouchButton;

#define TOUCH_BUTTON_COUNT 6

typedef struct TouchPad{
    uint16_t status;
    TouchButton A;
    TouchButton turboA;
    TouchButton B;
    TouchButton turboB;
    TouchButton select;
    TouchButton start;
    TouchButton* buttons[TOUCH_BUTTON_COUNT];
    TouchAxis axis;
    GraphicsContext* g_ctx;
    TTF_Font * font;
} TouchPad;

// forward declaration
struct JoyPad;

void init_touch_pad(GraphicsContext* ctx);
void free_touch_pad();
void render_touch_controls(GraphicsContext* ctx);
void touchpad_mapper(struct JoyPad* joyPad, SDL_Event* event);

#define ANDROID_INIT_TOUCH_PAD(CTX) init_touch_pad(CTX)
#define ANDROID_FREE_TOUCH_PAD() free_touch_pad()
#define ANDROID_RENDER_TOUCH_CONTROLS(CTX) render_touch_controls(CTX)
#define ANDROID_TOUCHPAD_MAPPER(JOYPAD, EVENT) touchpad_mapper(JOYPAD, EVENT)

#else

#define ANDROID_INIT_TOUCH_PAD(CTX)
#define ANDROID_FREE_TOUCH_PAD()
#define ANDROID_RENDER_TOUCH_CONTROLS(CTX)
#define ANDROID_TOUCHPAD_MAPPER(JOYPAD, EVENT)

#endif