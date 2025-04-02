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

static uint8_t read_PRG(Mapper*, uint16_t);
static void write_PRG(Mapper*, uint16_t, uint8_t);

void load_AOROM(Mapper* mapper){
    mapper->write_PRG = write_PRG;
    mapper->read_PRG = read_PRG;
    mapper->PRG_ptr = mapper->PRG_ROM;
}

static void write_PRG(Mapper* mapper, uint16_t address, uint8_t value){
    mapper->PRG_ptr = mapper->PRG_ROM + (value & 0x7) * 0x8000;
    if((value >> 4) & 0x1)
        set_mirroring(mapper, ONE_SCREEN_UPPER);
    else
        set_mirroring(mapper, ONE_SCREEN_LOWER);
}

static uint8_t read_PRG(Mapper* mapper, uint16_t address){
    // PRG bank determined by bit 0 - 2
    return *(mapper->PRG_ptr + (address - 0x8000));
}