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


# pragma once

#include "gfx.h"
#include "mapper.h"
#include "apu.h"
#include "utils.h"
#include <SDL_rwops.h>
#include <SDL.h>

#define NSF_HEADER_SIZE 0x80
#define TEXT_FIELD_SIZE 32
// Should be greater than TEXT_FIELD_SIZE
#define MAX_TEXT_FIELD_SIZE 40
#define MAX_TRACK_NAME_SIZE 24

#define NSF_SENTINEL_ADDR 0x5FF5
#define NSF_DEFAULT_TRACK_DUR 180000 // ms

typedef enum NSFFormat{
    NSFE = 1,
    NSF2 = 2
}NSFFormat;

typedef struct NSF {
    uint8_t version;
    uint8_t total_songs;
    uint8_t starting_song;
    uint8_t current_song;
    uint16_t load_addr;
    uint16_t init_addr;
    uint16_t play_addr;
    char song_name[MAX_TEXT_FIELD_SIZE+1];
    char artist[MAX_TEXT_FIELD_SIZE+1];
    char copyright[MAX_TEXT_FIELD_SIZE+1];
    char ripper[MAX_TEXT_FIELD_SIZE+1];
    char** tlbls;
    int* times;
    int* fade;
    double tick;
    int tick_max;
    uint16_t speed;
    uint8_t bank_switch;
    uint8_t* bank_ptrs[8];
    uint8_t bank_init[8];
    uint8_t initializing;
    size_t prg_size;

    SDL_Texture* song_info_tx;
    SDL_Rect song_info_rect;
    SDL_Texture* song_num_tx;
    SDL_Rect song_num_rect;
    SDL_Texture* song_dur_tx;
    SDL_Rect song_dur_rect;
    SDL_Texture* song_dur_max_tx;
    SDL_Rect song_dur_max_rect;
    complx samples[AUDIO_BUFF_SIZE];
    complx temp[AUDIO_BUFF_SIZE];
} NSF;

void load_nsf(SDL_RWops* file, Mapper* mapper);
void load_nsfe(SDL_RWops* file, Mapper* mapper);
void free_NSF(NSF* nsf);
void next_song(struct Emulator* emulator, NSF* nsf);
void prev_song(struct Emulator* emulator, NSF* nsf);
void init_song(struct Emulator* emulator, size_t song_number);
void nsf_jsr(struct Emulator* emulator, uint16_t address);
void init_NSF_gfx(GraphicsContext* g_ctx, NSF* nsf);
void render_NSF_graphics(struct Emulator* emulator, NSF* nsf);
