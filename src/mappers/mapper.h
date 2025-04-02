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
#include <stdio.h>

#define INES_HEADER_SIZE 16

typedef enum TVSystem{
    NTSC = 0,
    DUAL,
    PAL,
    DENDY,
} TVSystem;

typedef enum{
    NO_MIRRORING,
    VERTICAL,
    HORIZONTAL,
    ONE_SCREEN,
    ONE_SCREEN_LOWER,
    ONE_SCREEN_UPPER,
    FOUR_SCREEN,
} Mirroring;

typedef enum MapperID {
    NROM = 0,
    MMC1,
    UXROM,
    CNROM,
    MMC3,
    AOROM = 7,
    COLORDREAMS = 11,
    COLORDREAMS46 = 46,
    GNROM = 66
} MapperID;

typedef enum MapperFormat {
    ARCHAIC_INES,
    INES,
    NES2,
} MapperFormat;

struct Genie;
struct Emulator;
struct NSF;

typedef struct Mapper{
    uint8_t* CHR_ROM;
    uint8_t* PRG_ROM;
    uint8_t* PRG_RAM;
    uint8_t* PRG_ptr;
    uint8_t* CHR_ptr;
    uint16_t PRG_banks;
    uint16_t CHR_banks;
    size_t CHR_RAM_size;
    uint8_t RAM_banks;
    size_t RAM_size;
    Mirroring mirroring;
    TVSystem type;
    MapperFormat format;
    uint16_t name_table_map[4];
    uint32_t clamp;
    uint16_t mapper_num;
    uint8_t submapper;
    uint8_t is_nsf;
    void (*on_scanline)(struct Mapper*);
    uint8_t (*read_ROM)(struct Mapper*, uint16_t);
    void (*write_ROM)(struct Mapper*, uint16_t, uint8_t);
    uint8_t (*read_PRG)(struct Mapper*, uint16_t);
    void (*write_PRG)(struct Mapper*, uint16_t, uint8_t);
    uint8_t (*read_CHR)(struct Mapper*, uint16_t);
    void (*write_CHR)(struct Mapper*, uint16_t , uint8_t);
    void (*reset)(struct Mapper*);

    // mapper extension structs would be attached here
    // memory should be allocated dynamically and should
    // not be freed since this is done by the generic mapper functions
    void* extension;
    // pointer to game genie if any
    struct Genie* genie;
    struct NSF* NSF;
    struct Emulator* emulator;
} Mapper;

void load_file(char* file_name, char* game_genie, Mapper* mapper);
void free_mapper(struct Mapper* mapper);
void set_mirroring(Mapper* mapper, Mirroring mirroring);

// mapper specifics

void load_UXROM(Mapper* mapper);
void load_MMC1(Mapper* mapper);
void load_CNROM(Mapper* mapper);
void load_GNROM(Mapper* mapper);
void load_AOROM(Mapper* mapper);
void load_MMC3(Mapper* mapper);
void load_colordreams(Mapper* mapper);
void load_colordreams46(Mapper* mapper);
