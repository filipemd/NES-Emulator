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


#include <stdlib.h>
#include <string.h>

#include "genie.h"
#include "mapper.h"
#include "utils.h"

static uint8_t read_genie_PRG(Mapper* mapper, uint16_t address);
static uint8_t PRG_passthrough(Mapper* mapper, uint16_t address);
static void write_PRG(Mapper*, uint16_t, uint8_t);
static uint8_t read_CHR(Mapper*, uint16_t);
static void write_CHR(Mapper*, uint16_t, uint8_t);
static void load_registers(const uint8_t* mem, uint16_t* addr, uint8_t* cmp, uint8_t* repl);

static void swap_mirroring(Genie* genie);


void load_genie(char* filename, Mapper* mapper){
    Genie* genie = calloc(1, sizeof(Genie));
    load_file(filename, NULL, NULL, &genie->g_mapper);
    mapper->genie = genie;
    genie->mapper = mapper;

    // store the mapper's actual function pointers on the game genie
    genie->g_mapper.read_PRG = mapper->read_PRG;
    genie->g_mapper.write_PRG = mapper->write_PRG;
    genie->g_mapper.read_CHR = mapper->read_CHR;
    genie->g_mapper.write_CHR = mapper->write_CHR;

    // intercept the mappers functions with the genie's
    mapper->read_PRG = read_genie_PRG;
    mapper->write_PRG = write_PRG;
    mapper->read_CHR = read_CHR;
    mapper->write_CHR = write_CHR;

    swap_mirroring(genie);
}


static void swap_mirroring(Genie* genie){
    Mirroring m;
    m = genie->mapper->mirroring;
    genie->mapper->mirroring = genie->g_mapper.mirroring;
    set_mirroring(genie->mapper, genie->g_mapper.mirroring);
    genie->g_mapper.mirroring = m;
    set_mirroring(&genie->g_mapper, m);

}


static uint8_t read_genie_PRG(Mapper* mapper, uint16_t address){
    // the game genie is only one bank
    return mapper->genie->g_mapper.PRG_ROM[(address - 0x8000) & 0x3fff];
}

static uint8_t PRG_passthrough(Mapper* mapper, uint16_t address){
    Genie* gg = mapper->genie;
    uint8_t value = gg->g_mapper.read_PRG(mapper, address);
    if(address == gg->address1 && !(gg->ctrl & BIT_4)){
        return gg->ctrl & BIT_1 ? (gg->cmp1 == value? gg->repl1 : value) : gg->repl1;
    }
    if(address == gg->address2 && !(gg->ctrl & BIT_5)){
        return gg->ctrl & BIT_2 ? (gg->cmp2 == value? gg->repl2 : value) : gg->repl2;
    }
    if(address == gg->address3 && !(gg->ctrl & BIT_6)){
        return gg->ctrl & BIT_3 ? (gg->cmp3 == value? gg->repl3 : value) : gg->repl3;
    }
    return value;
}

static void load_registers(const uint8_t* mem, uint16_t* addr, uint8_t* cmp, uint8_t* repl){
    uint8_t offset = 0;
    // fix bit 15 as 1
    *addr = 0x8000;
    *addr |= ((*(mem + offset++))<<8);
    *addr |= *(mem + offset++);
    *cmp = *(mem + offset++);
    *repl = *(mem + offset);

}

static void write_PRG(Mapper* mapper, uint16_t address, uint8_t value){
    Genie* genie = mapper->genie;
    genie->g_mapper.PRG_ROM[address - 0x8000] = value;
    if(address == 0x8000){
        if(value & BIT_0) {
            genie->ctrl = value;
            load_registers(genie->g_mapper.PRG_ROM + 1, &genie->address1, &genie->cmp1, &genie->repl1);
            load_registers(genie->g_mapper.PRG_ROM + 5, &genie->address2, &genie->cmp2, &genie->repl2);
            load_registers(genie->g_mapper.PRG_ROM + 9, &genie->address3, &genie->cmp3, &genie->repl3);
        } else {
            mapper->write_PRG = genie->g_mapper.write_PRG;
            mapper->write_CHR = genie->g_mapper.write_CHR;
            mapper->read_CHR = genie->g_mapper.read_CHR;
            if((genie->ctrl >> 4) == 0x7){
                // all codes disabled no passthrough needed connect directly to mapper
                mapper->read_PRG = genie->g_mapper.read_PRG;
            }else {
                // initialize PRG passthrough with targeted address override
                mapper->read_PRG = PRG_passthrough;
                LOG(INFO, "Game genie PRG passthrough engaged");
            }
            swap_mirroring(genie);
        }
    }
}


static uint8_t read_CHR(Mapper* mapper, uint16_t address){
    return mapper->genie->g_mapper.CHR_ROM[address];
}


static void write_CHR(Mapper* mapper, uint16_t address, uint8_t value){
    if(mapper->CHR_RAM_size){
        LOG(DEBUG, "Attempted to write to CHR-ROM");
        return;
    }
    mapper->genie->g_mapper.CHR_ROM[address] = value;
}