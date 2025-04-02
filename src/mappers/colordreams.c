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


#include "mapper.h"
#include "utils.h"

static uint8_t read_PRG(Mapper*, uint16_t);
static void write_PRG(Mapper*, uint16_t, uint8_t);
static uint8_t read_CHR(Mapper*, uint16_t);
static void write_ROM(Mapper* mapper, uint16_t address, uint8_t value);
static void reset(Mapper* mapper);

static void select_banks(Mapper* mapper);

typedef struct {
    uint8_t CHR;
    uint8_t PRG;
} reg_t;

void load_colordreams(Mapper* mapper){
    mapper->read_PRG = read_PRG;
    mapper->write_PRG = write_PRG;
    mapper->read_CHR = read_CHR;
    mapper->PRG_ptr = mapper->PRG_ROM;
    mapper->CHR_ptr = mapper->CHR_ROM;
}

void load_colordreams46(Mapper* mapper) {
    // two store the two registers
    // CHR = 0, PRG = 1
    mapper->extension = calloc(1, sizeof(reg_t));
    mapper->write_ROM = write_ROM;
    mapper->read_PRG = read_PRG;
    mapper->read_CHR = read_CHR;
    mapper->PRG_ptr = mapper->PRG_ROM;
    mapper->CHR_ptr = mapper->CHR_ROM;
    mapper->reset = reset;
}

void reset(Mapper* mapper) {
    write_ROM(mapper, 0x6000, 0);
    write_ROM(mapper, 0x8000, 0);
}

static void write_ROM(Mapper* mapper, uint16_t address, uint8_t value){
    if(address < 0x6000){
        LOG(DEBUG, "Attempted to write to unavailable expansion ROM");
        return;
    }

    if(address < 0x8000){
        reg_t* reg = mapper->extension;
        reg->CHR &= 0x7;
        reg->CHR |= (value & 0xf0) >> 1;
        reg->PRG &= 0x1;
        reg->PRG |= (value & 0xf) << 1;
        select_banks(mapper);
        return;
    }

    /*
    7  bit  0
    ---- ----
    .CCC ...P
     |||    |
     |||    +- PRG LOW
     +++------ CHR LOW
    */
    reg_t* reg = mapper->extension;
    reg->CHR &= ~0x7;
    reg->CHR |= (value & 0x70) >> 4;
    reg->PRG &= ~0x1;
    reg->PRG |= value & 0x1;
    select_banks(mapper);
}

static void select_banks(Mapper* mapper) {
    const reg_t* reg = mapper->extension;
    mapper->PRG_ptr = mapper->PRG_ROM + reg->PRG * 0x8000;
    mapper->CHR_ptr = mapper->CHR_ROM + reg->CHR * 0x2000;
}

static uint8_t read_PRG(Mapper* mapper, uint16_t address){
    return *(mapper->PRG_ptr + (address - 0x8000));
}

static void write_PRG(Mapper* mapper, uint16_t address, uint8_t value){
    /*
    7  bit  0
    ---- ----
    CCCC LLPP
    |||| ||||
    |||| ||++- Select 32 KB PRG ROM bank for CPU $8000-$FFFF
    |||| ++--- Used for lockout defeat
    ++++------ Select 8 KB CHR ROM bank for PPU $0000-$1FFF
    */
    mapper->PRG_ptr = mapper->PRG_ROM + (value & 0x3) * 0x8000;
    mapper->CHR_ptr = mapper->CHR_ROM + 0x2000 * ((value >> 4) & 0xf);
}


static uint8_t read_CHR(Mapper* mapper, uint16_t address){
    return *(mapper->CHR_ptr + address);
}

