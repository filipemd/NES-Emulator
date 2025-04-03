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


#include "controller.h"
#include "gamepad.h"
#include "touchpad.h"


void init_joypad(struct JoyPad* joyPad, uint8_t player, bool multiple_controllers_in_one_keyboard) {
    joyPad->strobe = 0;
    joyPad->index = 0;
    joyPad->status = 0;
    joyPad->player = player;

    joyPad->multiple_controllers_in_one_keyboard=multiple_controllers_in_one_keyboard;
}


uint8_t read_joypad(struct JoyPad* joyPad){
    if(joyPad->index > 7)
        return 1;
    uint8_t val = (joyPad->status & (1 << joyPad->index)) != 0;
    if(!joyPad->strobe)
        joyPad->index++;
    return val;
}


void write_joypad(struct JoyPad* joyPad, uint8_t data){
    joyPad->strobe = data & 1;
    if(joyPad->strobe)
        joyPad->index = 0;
}


// Se apenas um controle for utilizado
static uint16_t generic_keyboard_mapper(SDL_Event* event) {
    switch (event->key.keysym.sym) {
        case SDLK_RIGHT:
            return RIGHT;
        case SDLK_LEFT:
            return LEFT;
        case SDLK_DOWN:
            return DOWN;
        case SDLK_UP:
            return UP;
        case SDLK_RETURN:
            return START;
        case SDLK_RSHIFT:
            return SELECT;
        case SDLK_j:
            return BUTTON_A;
        case SDLK_k:
            return BUTTON_B;
        case SDLK_l:
            return TURBO_B;
        case SDLK_h:
            return TURBO_A;
        default:
            return 0;
    }
    return 0;
}

static uint16_t player1_keyboard_mapper(SDL_Event* event) {
    switch (event->key.keysym.sym) {
        case SDLK_d:
            return RIGHT;
        case SDLK_a:
            return LEFT;
        case SDLK_s:
            return DOWN;
        case SDLK_w:
            return UP;
        case SDLK_TAB:
            return START;
        case SDLK_LSHIFT:
            return SELECT;
        case SDLK_q:
            return BUTTON_A;
        case SDLK_e:
            return BUTTON_B;
        case SDLK_z:
            return TURBO_A;
        case SDLK_c:
            return TURBO_B;
        default:
            return 0;
    }
    return 0;
}

static uint16_t player2_keyboard_mapper(SDL_Event* event) {
    switch (event->key.keysym.sym) {
        case SDLK_l:
            return RIGHT;
        case SDLK_j:
            return LEFT;
        case SDLK_k:
            return DOWN;
        case SDLK_i:
            return UP;
        case SDLK_RETURN:
            return START;
        case SDLK_RSHIFT:
            return SELECT;
        case SDLK_u:
            return BUTTON_A;
        case SDLK_o:
            return BUTTON_B;
        case SDLK_n:
            return TURBO_A;
        case SDLK_m:
            return TURBO_B;
        default:
            return 0;
    }
    return 0;
}

void keyboard_mapper(struct JoyPad* joyPad, SDL_Event* event){
    uint16_t key=0;
    if (joyPad->multiple_controllers_in_one_keyboard) {
        if (joyPad->player==0) key = player1_keyboard_mapper(event);
        else if (joyPad->player==1) key = player2_keyboard_mapper(event);
    } else {
        key = generic_keyboard_mapper(event);
    }

    if(event->type == SDL_KEYUP) {
        joyPad->status &= ~key;
        if(key == TURBO_A) {
            // clear button A
            joyPad->status &= ~BUTTON_A;
        }
        if(key == TURBO_B) {
            // clear button B
            joyPad->status &= ~BUTTON_B;
        }
    } else if(event->type == SDL_KEYDOWN) {
        joyPad->status |= key;
        if(key == TURBO_A) {
            // set button A
            joyPad->status |= BUTTON_A;
        }
        if(key == TURBO_B) {
            // set button B
            joyPad->status |= BUTTON_B;
        }
    }
}

void update_joypad(struct JoyPad* joyPad, SDL_Event* event){
#ifdef __ANDROID__
    ANDROID_TOUCHPAD_MAPPER(joyPad, event);
#else
    keyboard_mapper(joyPad, event);
    gamepad_mapper(joyPad, event);
#endif
}

void turbo_trigger(struct JoyPad* joyPad){
    // toggle BUTTON_A AND BUTTON_B if TURBO_A and TURBO_B are set respectively
    joyPad->status ^= joyPad->status >> 8;
}