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


#include "debugtools.h"
#include "ppu.h"

static uint8_t* get_palette(struct PPU* ppu, size_t tile_x, size_t tile_y);

void render_name_tables(struct PPU* ppu, uint32_t* screen) {
    // renders all four name_tables to screen
    // screen should have resolution 512x480
    uint16_t bank = ppu->ctrl & (BIT_4) ? 0x1000 : 0;
    for(int k = 0; k < 4; k++) {
        size_t y_off = k > 1? VISIBLE_SCANLINES: 0;
        size_t x_off = k % 2? VISIBLE_DOTS: 0;
        for (int n = k * 0x400, i = 0; n < (k * 0x400) + 0x3C0; n++, i++) {
            size_t tile_x = i % 32;
            size_t tile_y = i / 32;
            uint8_t *palette = get_palette(ppu, tile_x, tile_y);

            for (int y = 0; y < 8; y++) {
                uint8_t lo = read_vram(ppu, bank + read_vram(ppu, 0x2000 + n) * 16 + y);
                uint8_t hi = read_vram(ppu, bank + read_vram(ppu, 0x2000 + n) * 16 + y + 8);;

                for (int x = 7; x >= 0; x--) {
                    uint8_t value = ((hi & BIT_0) << 1) | (lo & BIT_0);
                    uint32_t color = value == 0 ? nes_palette[ppu->palette[0]] : nes_palette[*(palette + value)];
                    hi >>= 1;
                    lo >>= 1;
                    screen[(y + tile_y * 8 + y_off) * VISIBLE_DOTS * 2 + (x + x_off + tile_x * 8)] = color;
                }
            }
        }
    }
}

static uint8_t* get_palette(struct PPU* ppu, size_t tile_x, size_t tile_y){
    uint8_t index = (tile_y / 4) * 8 + tile_x / 4;
    uint8_t attr = ppu->V_RAM[0x3C0 + index];

    uint8_t palette_index = (attr >> ((((tile_y % 4) / 2) << 1) | ((tile_x % 4) / 2)) * 2) & 0x3 ;
    return ppu->palette + palette_index * 4;
}
