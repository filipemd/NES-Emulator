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


#include <SDL.h>

#include "gamepad.h"
#include "utils.h"
#include "controller.h"

static GamePad* game_pads[MAX_PADS];
static int game_pad_c = 0;
static const uint16_t key_map[CONTROLLER_KEY_COUNT] = {
    TURBO_A,
    BUTTON_B,
    TURBO_B,
    BUTTON_A,
    BUTTON_A,
    BUTTON_B,
    TURBO_A,
    TURBO_B,
    SELECT,
    START,
};

static int pad_index(GamePad* pad);

void init_pads(){
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        GamePad* pad = SDL_JoystickOpen(i);
        if(pad != NULL) {
            game_pads[game_pad_c++] = pad;
            LOG(INFO, "Joypad connected");
        }
    }
}

void gamepad_mapper(struct JoyPad* joyPad, SDL_Event* event){
    uint16_t key = 0;
    switch (event->type) {
        case SDL_JOYDEVICEADDED: {
            if(game_pad_c < MAX_PADS) {
                GamePad* pad = SDL_JoystickOpen(event->jdevice.which);
                if(pad != NULL && pad_index(pad) < 0) {
                    game_pads[game_pad_c++] = pad;
                    LOG(INFO, "Joypad connected");
                }
            }
            break;
        }
        case SDL_JOYDEVICEREMOVED: {
            GamePad* pad = SDL_JoystickFromInstanceID(event->jdevice.which);
            for(int i = 0; i < game_pad_c; i++){
                if(pad == game_pads[i]){
                    for(int j = i; j < game_pad_c - 1; j++){
                        game_pads[j] = game_pads[j + 1];
                    }
                    game_pads[game_pad_c--] = NULL;
                    SDL_JoystickClose(pad);
                    LOG(INFO, "Joypad removed");
                    break;
                }
            }
            break;
        }
        default: {
            GamePad* pad = SDL_JoystickFromInstanceID(event->jdevice.which);
            if(joyPad->player != pad_index(pad))
                // the joypad is not interested in the active controller's input
                // this check allows management of multiple controllers
                return;
            switch (event->type) {
                case SDL_JOYBUTTONDOWN:
                case SDL_JOYBUTTONUP:
                    if (event->jbutton.button < 10) {
                        key = key_map[event->jbutton.button];
                    }

                    if(event->type == SDL_JOYBUTTONDOWN) {
                        joyPad->status |= key;
                        if(key == TURBO_A) {
                            // set button A
                            joyPad->status |= BUTTON_A;
                        }
                        if(key == TURBO_B) {
                            // set button B
                            joyPad->status |= BUTTON_B;
                        }
                    } else if(event->type == SDL_JOYBUTTONUP) {
                        joyPad->status &= ~key;
                        if(key == TURBO_A) {
                            // clear button A
                            joyPad->status &= ~BUTTON_A;
                        }
                        if(key == TURBO_B) {
                            // clear button B
                            joyPad->status &= ~BUTTON_B;
                        }
                    }
                    break;
                case SDL_JOYHATMOTION:
                    if (event->jhat.value & SDL_HAT_LEFT)
                        key |= LEFT;
                    else if (event->jhat.value & SDL_HAT_RIGHT)
                        key |= RIGHT;
                    if (event->jhat.value & SDL_HAT_UP)
                        key |= UP;
                    else if (event->jhat.value & SDL_HAT_DOWN)
                        key |= DOWN;

                    if (event->jhat.value == SDL_HAT_CENTERED) {
                        // Reset hat
                        key = RIGHT | LEFT | UP | DOWN;
                        joyPad->status &= ~key;
                    } else {
                        joyPad->status |= key;
                    }
                    break;
                case SDL_JOYAXISMOTION:
                    if (event->jaxis.value < -3200) {
                        if (event->jaxis.axis == 0)
                            key = LEFT;
                        else if (event->jaxis.axis == 1)
                            key = UP;
                        joyPad->status |= key;
                    } else if (event->jaxis.value > 3200) {
                        if (event->jaxis.axis == 0)
                            key = RIGHT;
                        else if (event->jaxis.axis == 1)
                            key = DOWN;
                        joyPad->status |= key;
                    } else {
                        // Reset axis
                        if (event->jaxis.axis == 0)
                            key = RIGHT | LEFT;
                        else if (event->jaxis.axis == 1)
                            key = UP | DOWN;

                        joyPad->status &= ~key;
                    }
                    break;
            }
        }
    }
}

static int pad_index(GamePad* pad){
    for(int i = 0; i < game_pad_c; i++){
        if(game_pads[i] == pad)
            return i;
    }
    return -1;
}