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


#include "nsf.h"
#include "utils.h"
#include "emulator.h"

#ifdef __ANDROID__
#include "touchpad.h"
#endif

#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PRG_ROM_SIZE 0x8000
#define PRG_RAM_SIZE 0x2000
#define BAR_COUNT 128
#define MAX_SILENCE 150 // frames

static uint8_t read_PRG(const Mapper*, uint16_t);
static void write_PRG(Mapper*, uint16_t, uint8_t);
static uint8_t read_CHR(Mapper*, uint16_t);
static void write_CHR(Mapper*, uint16_t, uint8_t);
static uint8_t read_ROM(Mapper*, uint16_t);
static void write_ROM(const Mapper*, uint16_t, uint8_t);

static void read_text_stream(char** list, const char* buf, size_t list_len, size_t buf_len, size_t max_str_len);
static void load_info_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file);
static void load_data_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file);
static void load_bank_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file);
static void load_rate_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file);
static void load_auth_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file);
static void load_time_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file);
static void load_fade_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file);
static void load_tlbl_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file);

void read_text_stream(char** list, const char* buf, size_t list_len, size_t buf_len, size_t max_str_len) {
    if(list == NULL || buf == NULL)
        return;
    size_t offset = 0, str = 0, idx = 0;
    while(offset < buf_len) {
        char c = buf[offset++];
        if(c == 0) {
            str++;
            if(str >= list_len)
                break;
            idx = 0;
        } else if(idx < max_str_len) {
            list[str][idx++] = c;
        } else {
            // string is too long
            while (c != 0 && offset < buf_len) {
                c = buf[offset++];
            }
            // add ellipsis at the end to show content is truncated
            strncpy(list[str] + max_str_len - 3, "..", 3);
            // reset to just before null byte if there is still stuff to be read
            if(offset < buf_len)
                offset--;
        }
    }
}

void load_info_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file) {
    uint8_t chunk[10] = {0};
    if(len < 9) {
        LOG(ERROR, "INFO chunk too short");
        quit(EXIT_FAILURE);
    }
    SDL_RWread(file, chunk, len > 10 ? 10: len, 1);
    NSF* nsf = mapper->NSF;
    nsf->load_addr = (chunk[1] << 8) | chunk[0];
    if(nsf->load_addr < 0x8000) {
        LOG(ERROR, "Load address ox%04x too low", nsf->load_addr);
        quit(EXIT_FAILURE);
    }

    nsf->init_addr = (chunk[3] << 8) | chunk[2];
    nsf->play_addr = (chunk[5] << 8) | chunk[4];

    if(chunk[0x6] & BIT_1) {
        // PAL/NTSC
        LOG(INFO, "ROM type: Dual Compatible (using NTSC)");
        mapper->type = NTSC;
    }else if(chunk[0x6] & BIT_0) {
        // PAL
        LOG(INFO, "ROM type: PAL");
        mapper->type = PAL;
    }else {
        // NTSC
        LOG(INFO, "ROM type: NTSC");
        mapper->type = NTSC;
    }

    if(mapper->type == PAL) {
        nsf->speed = 19997;
    }else {
        nsf->speed = 16666;
    }

    if(chunk[0x7]) {
        LOG(ERROR, "Extra Sound Chip support required");
        quit(EXIT_FAILURE);
    }

    nsf->total_songs = chunk[8];
    nsf->starting_song = nsf->current_song = chunk[9] + 1;
}

void load_data_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file) {
    NSF* nsf = mapper->NSF;
    if(nsf->bank_switch) {
        uint16_t padding = nsf->load_addr & 0xfff;
        size_t prg_size = len + padding;
        if(prg_size % 0x1000) {
            // ensure size is a multiple of 4kb
            prg_size = (prg_size / 0x1000 + 1) * 0x1000;
        }
        mapper->PRG_banks = prg_size / 0X1000;
        LOG(INFO, "PRG banks: %llu", mapper->PRG_banks);
        mapper->PRG_ROM = malloc(prg_size);
        memset(mapper->PRG_ROM, 0, prg_size);
        SDL_RWread(file, mapper->PRG_ROM + padding, 1, len);

        // compute bank pointers
        for(size_t i = 0; i < 8; i++) {
            mapper->write_ROM(mapper, 0x5ff8 + i, nsf->bank_init[i]);
        }

    } else {
        mapper->PRG_ROM = malloc(PRG_ROM_SIZE);
        memset(mapper->PRG_ROM, 0, PRG_ROM_SIZE);
        size_t read_len = MIN(len, 0x10000 - nsf->load_addr);
        SDL_RWread(file, mapper->PRG_ROM + (nsf->load_addr - 0x8000), 1, read_len);
    }
}

void load_bank_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file) {
    NSF* nsf = mapper->NSF;
    nsf->bank_switch = 1;
    uint8_t bank_data[8] = {0};
    SDL_RWread(file, bank_data, len, 1);

    for(size_t i = 0; i < 8; i++) {
        nsf->bank_init[i] = bank_data[i];
    }
}

void load_rate_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file) {
    NSF* nsf = mapper->NSF;
    if(len < 2 || (len & 1 && len < 8)) {
        LOG(ERROR, "Invalid RATE chunk");
        quit(EXIT_FAILURE);
    }

    uint8_t chunk[6];
    SDL_RWread(file, chunk, len > 6 ? 6 : len, 1);

    if(len > 0 && mapper->type == NTSC) {
        nsf->speed = (chunk[1] << 8) | chunk[0];
    }
    if(len > 2 && mapper->type == PAL) {
        nsf->speed = (chunk[3] << 8) | chunk[2];
    }
    if(len > 4 && mapper->type == DENDY) {
        nsf->speed = (chunk[5] << 8) | chunk[4];
    }
}

void load_auth_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file) {
    NSF* nsf = mapper->NSF;

    uint8_t* chunk = malloc(len);
    memset(chunk, 0, len);
    SDL_RWread(file, chunk, len, 1);

    char* fields[4] = {
        nsf->song_name,
        nsf->artist,
        nsf->copyright,
        nsf->ripper
    };

    read_text_stream(fields, (char*)chunk, 4, len, MAX_TEXT_FIELD_SIZE);
    LOG(INFO, "SONG_NAME: %s", nsf->song_name);
    LOG(INFO, "ARTIST: %s", nsf->artist);
    LOG(INFO, "COPYRIGHT: %s", nsf->copyright);
    LOG(INFO, "RIPPER: %s", nsf->ripper);
    free(chunk);
}

void load_time_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file) {
    NSF* nsf = mapper->NSF;

    nsf->times = malloc(4 * nsf->total_songs);
    memset(nsf->times, 0, 4 * nsf->total_songs);

    SDL_RWread(file, nsf->times, 4, len / 4);
    for(int i = len / 4; i < nsf->total_songs; i++) {
        nsf->times[i] = NSF_DEFAULT_TRACK_DUR;
    }
}

void load_fade_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file) {
    NSF* nsf = mapper->NSF;

    nsf->fade = malloc(4 * nsf->total_songs);
    memset(nsf->fade, 0, 4 * nsf->total_songs);

    SDL_RWread(file, nsf->fade, 4, len / 4);
}

void load_tlbl_chunk(uint32_t len, Mapper* mapper, SDL_RWops* file) {
    NSF* nsf = mapper->NSF;

    uint8_t* chunk = malloc(len);
    memset(chunk, 0, len);
    SDL_RWread(file, chunk, len, 1);

    nsf->tlbls = malloc(nsf->total_songs * sizeof(char*));
    memset(nsf->tlbls, 0, nsf->total_songs * sizeof(char*));

    for(int i = 0; i < nsf->total_songs; i++) {
        nsf->tlbls[i] = (char*)malloc(MAX_TRACK_NAME_SIZE + 1);
        memset(nsf->tlbls[i], 0, MAX_TRACK_NAME_SIZE + 1);
    }
    read_text_stream(nsf->tlbls, (char*)chunk, nsf->total_songs, len, MAX_TRACK_NAME_SIZE);
    free(chunk);
}

void load_nsfe(SDL_RWops* file, Mapper* mapper) {
    // PRG RAM
    mapper->PRG_RAM = malloc(PRG_RAM_SIZE);
    memset(mapper->PRG_RAM, 0, PRG_RAM_SIZE);

    // mapper R/W redirects
    mapper->read_PRG = (uint8_t(*)(struct Mapper*, uint16_t))read_PRG;
    mapper->write_PRG = write_PRG;
    mapper->read_CHR = read_CHR;
    mapper->write_CHR = write_CHR;
    mapper->read_ROM = read_ROM;
    mapper->write_ROM = (void(*)(struct Mapper*, uint16_t, uint8_t))write_ROM;

    // skip the header
    int64_t offset = 4;
    SDL_RWseek(file, offset, RW_SEEK_SET);

    mapper->is_nsf = 1;

    NSF* nsf = calloc(1, sizeof(NSF));
    mapper->NSF = nsf;

    // defaults
    strncpy(nsf->song_name, "<?>", 4);
    strncpy(nsf->artist, "<?>", 4);
    strncpy(nsf->copyright, "<?>", 4);
    strncpy(nsf->ripper, "<?>", 4);

    uint32_t len;
    int has_info = 0;

    while(1) {
        char id[5];
        if(!SDL_RWread(file, &len, 4, 1)) {
            LOG(ERROR, "Error loading NSFe");
            quit(EXIT_FAILURE);
        }
        memset(id, 0, 5);
        SDL_RWread(file, id, 4, 1);
        offset += 8 + len;

        LOG(DEBUG, "Chunk: %s (%d)", id, len);

        if(strncmp(id, "INFO", 4) == 0) {
            load_info_chunk(len, mapper, file);
            has_info = 1;
        } else if(strncmp(id, "DATA", 4) == 0) {
            if(!has_info) {
                LOG(ERROR, "Missing INFO chunk before DATA");
                quit(EXIT_FAILURE);
            }
            load_data_chunk(len, mapper, file);
        } else if(strncmp(id, "BANK", 4) == 0) {
            if(!has_info) {
                LOG(ERROR, "Missing INFO chunk before BANK");
                quit(EXIT_FAILURE);
            }
            load_bank_chunk(len, mapper, file);
        } else if (strncmp(id, "NEND", 4) == 0) {
            break;
        } else if(strncmp(id, "RATE", 4) == 0) {
            load_rate_chunk(len, mapper, file);
        } else if(strncmp(id, "auth", 4) == 0) {
            load_auth_chunk(len, mapper, file);
        } else if(strncmp(id, "time", 4) == 0) {
            if(has_info) {
                load_time_chunk(len, mapper, file);
            }
        } else if(strncmp(id, "fade", 4) == 0) {
            if(has_info) {
                load_fade_chunk(len, mapper, file);
            }
        } else if(strncmp(id, "tlbl", 4) == 0) {
            if(has_info) {
                load_tlbl_chunk(len, mapper, file);
            }
        } else if(strncmp(id, "text", 4) == 0) {
            char* chunk = malloc(len);
            SDL_RWread(file, chunk, len, 1);
            LOG(INFO, "TEXT: \n %s \n", chunk);
            free(chunk);
        } else if(id[0] > 65 && id[0] < 90) {
            LOG(ERROR, "Required chunk %s not implemented", id);
            quit(EXIT_FAILURE);
        } else {
            LOG(DEBUG, "Skipping chunk %s", id);
        }

        // move to start of next chunk
        if(SDL_RWseek(file, offset, RW_SEEK_SET) < 0) {
            LOG(ERROR, "Error loading NSFe");
            quit(EXIT_FAILURE);
        }
    }
    LOG(DEBUG, "Bank switching: %s", nsf->bank_switch ? "ON": "OFF");
}

void load_nsf(SDL_RWops* file, Mapper* mapper) {
    SDL_RWseek(file, 0, RW_SEEK_SET);

    uint8_t header[NSF_HEADER_SIZE];
    SDL_RWread(file, header, NSF_HEADER_SIZE, 1);

    mapper->is_nsf = 1;

    NSF* nsf = calloc(1, sizeof(NSF));
    mapper->NSF = nsf;

    nsf->version = header[5];
    nsf->total_songs = header[6];
    nsf->starting_song = nsf->current_song = header[7];

    nsf->load_addr = (header[9] << 8) | header[8];
    if(nsf->load_addr < 0x8000) {
        LOG(ERROR, "Load address ox%04x too low", nsf->load_addr);
        quit(EXIT_FAILURE);
    }

    nsf->init_addr = (header[0xb] << 8) | header[0xa];
    nsf->play_addr = (header[0xd] << 8) | header[0xc];

    strncpy(nsf->song_name, (char*)(header + 0xe), TEXT_FIELD_SIZE);
    nsf->song_name[TEXT_FIELD_SIZE-1]='\0';
    strncpy(nsf->artist, (char*)(header + 0x2e), TEXT_FIELD_SIZE);
    nsf->artist[TEXT_FIELD_SIZE-1]='\0';
    strncpy(nsf->copyright, (char*)(header + 0x4e), TEXT_FIELD_SIZE);
    nsf->copyright[TEXT_FIELD_SIZE-1]='\0';

    LOG(INFO, "SONG_NAME: %s", nsf->song_name);
    LOG(INFO, "ARTIST: %s", nsf->artist);
    LOG(INFO, "COPYRIGHT: %s", nsf->copyright);

    if(header[0x7a] & BIT_1) {
        // PAL/NTSC
        LOG(INFO, "ROM type: Dual Compatible (using NTSC)");
        mapper->type = NTSC;
    }else if(header[0x7a] & BIT_0) {
        // PAL
        LOG(INFO, "ROM type: PAL");
        mapper->type = PAL;
    }else {
        // NTSC
        LOG(INFO, "ROM type: NTSC");
        mapper->type = NTSC;
    }

    if(mapper->type == PAL) {
        nsf->speed = (header[0x79] << 8) | header[0x78];
    }else {
        nsf->speed = (header[0x6f] << 8) | header[0x6e];
    }

    if(header[0x7b]) {
        LOG(ERROR, "Extra Sound Chip support required");
        quit(EXIT_FAILURE);
    }

    size_t data_len = (header[0x7f] << 16) | (header[0x7e] << 8) | header[0x7d];
    if(!data_len || nsf->version == 1) {
        long long size = SDL_RWseek(file, 0, RW_SEEK_END);
        if(size < 0) {
            LOG(ERROR, "Error reading ROM");
            quit(EXIT_FAILURE);
        }
        data_len = size - NSF_HEADER_SIZE;
        // reset file ptr
        SDL_RWseek(file, NSF_HEADER_SIZE, RW_SEEK_SET);
    }
    LOG(DEBUG, "Program data length: %llu", data_len);

    nsf->bank_switch = 0;
    for(size_t i = 0x70; i < 0x78; i++) {
        if(header[i] != 0) {
            nsf->bank_switch = 1;
            break;
        }
    }
    LOG(DEBUG, "Bank switching: %s", nsf->bank_switch ? "ON": "OFF");

    // PRG RAM
    mapper->PRG_RAM = malloc(PRG_RAM_SIZE);
    memset(mapper->PRG_RAM, 0, PRG_RAM_SIZE);

    // mapper R/W redirects
    mapper->read_PRG = (uint8_t(*)(struct Mapper*, uint16_t))read_PRG;
    mapper->write_PRG = write_PRG;
    mapper->read_CHR = read_CHR;
    mapper->write_CHR = write_CHR;
    mapper->read_ROM = read_ROM;
    mapper->write_ROM = (void(*)(struct Mapper*, uint16_t, uint8_t))write_ROM;


    if(nsf->bank_switch) {
        uint16_t padding = nsf->load_addr & 0xfff;
        size_t prg_size = data_len + padding;
        if(prg_size % 0x1000) {
            // ensure size is a multiple of 4kb
            prg_size = (prg_size / 0x1000 + 1) * 0x1000;

        }
        mapper->PRG_banks = prg_size / 0X1000;
        LOG(DEBUG, "PRG banks: %llu", mapper->PRG_banks);
        mapper->PRG_ROM = malloc(prg_size);
        memset(mapper->PRG_ROM, 0, prg_size);
        SDL_RWread(file, mapper->PRG_ROM + padding, 1, data_len);

        // init banks
        for(size_t i =0; i < 8; i++) {
            mapper->write_ROM(mapper, 0x5ff8 + i, header[0x70+i]);
            nsf->bank_init[i] = header[0x70+i];
        }

    } else {
        mapper->PRG_ROM = malloc(PRG_ROM_SIZE);
        memset(mapper->PRG_ROM, 0, PRG_ROM_SIZE);
        size_t read_len = MIN(data_len, 0x10000 - nsf->load_addr);
        SDL_RWread(file, mapper->PRG_ROM + (nsf->load_addr - 0x8000), 1, read_len);
    }
}

static uint8_t read_ROM(Mapper* mapper, uint16_t addr) {
    if(addr < 0x6000)
        // open bus
        return mapper->emulator->mem.bus;

    if(addr < 0x8000)
        return mapper->PRG_RAM[addr - 0x6000];

    return mapper->read_PRG(mapper, addr);
}

static void write_ROM(const Mapper* mapper, uint16_t addr, uint8_t val) {
    if(addr < 0x6000) {
        if(addr > 0x5ff7) {
            mapper->NSF->bank_ptrs[addr - 0x5ff8] = mapper->PRG_ROM + val * 0x1000;
        }
    } else if(addr < 0x8000) {
        mapper->PRG_RAM[addr - 0x6000] = val;
    }
}

static uint8_t read_PRG(const Mapper* mapper, uint16_t addr) {
    if(!mapper->NSF->bank_switch) {
        return mapper->PRG_ROM[addr - 0x8000];
    }
    size_t bank_index = (addr - 0x8000) / 0x1000;
    return *(mapper->NSF->bank_ptrs[bank_index] + (addr & 0xfff));
}

static void write_PRG(Mapper* mapper, uint16_t addr, uint8_t val) {
    // can't write to PRG ROM
}

static uint8_t read_CHR(Mapper* mapper, uint16_t addr) {
    // CHR not required
    return 0;
}

static void write_CHR(Mapper* mapper, uint16_t addr, uint8_t val) {
    // CHR not required
}

void init_song(Emulator* emulator, size_t song_number) {
    memset(emulator->mem.RAM, 0, RAM_SIZE);
    init_cpu(emulator);
    emulator->apu.audio_start = 0;
    emulator->apu.sampler.index = 0;
    SDL_PauseAudioDevice(emulator->g_ctx.audio_device, 1);

    for(size_t i = 0; i < 14; i++) {
        write_mem(&emulator->mem, 0x4000 + i, 0);
    }
    write_mem(&emulator->mem, 0x4015, 0);
    write_mem(&emulator->mem, 0x4015, 0xf);
    write_mem(&emulator->mem, 0x4017, 0x40);

    NSF* nsf = emulator->mapper.NSF;
    if(nsf->bank_switch) {
        for(size_t i = 0; i < 8; i++) {
            write_mem(&emulator->mem, 0x5ff8 + i, nsf->bank_init[i]);
        }
    }
    emulator->cpu.ac = song_number > 0 ? song_number - 1: song_number;
    emulator->cpu.x = emulator->type == PAL? 1: 0;
    nsf->initializing = 1;
    if(nsf->times != NULL) {
        nsf->tick_max = nsf->times[nsf->current_song == 0 ? 0 : nsf->current_song - 1];
        if(nsf->fade != NULL)
            nsf->tick_max += nsf->fade[nsf->current_song == 0 ? 0 : nsf->current_song - 1];
    }
    emulator->apu.volume = 1;
    nsf_jsr(emulator, nsf->init_addr);
    LOG(DEBUG, "Initializing tune %d", nsf->current_song);
}

void nsf_jsr(Emulator* emulator, uint16_t address) {
    // approximate JSR
    uint16_t ret_addr = NSF_SENTINEL_ADDR - 1;
    write_mem(&emulator->mem, STACK_START + emulator->cpu.sp--, ret_addr >> 8);
    write_mem(&emulator->mem, STACK_START + emulator->cpu.sp--, ret_addr & 0xFF);
    emulator->cpu.pc = address;
}

void next_song(Emulator* emulator, NSF* nsf) {
    nsf->current_song = nsf->current_song >= nsf->total_songs ? 1 : nsf->current_song + 1;
    init_song(emulator, nsf->current_song);
}

void prev_song(Emulator* emulator, NSF* nsf) {
    nsf->current_song = nsf->current_song <= 1 ? nsf->total_songs : nsf->current_song - 1;
    init_song(emulator, nsf->current_song);
}

static double bin_boundaries[BAR_COUNT + 1] = {0};

void init_NSF_gfx(GraphicsContext* g_ctx, NSF* nsf) {
#ifdef __ANDROID__
    int offset_x = g_ctx->dest.x, offset_y = g_ctx->dest.y, width = g_ctx->dest.w, height = g_ctx->dest.h;
#else
    int offset_x = 0, offset_y = 0;
#endif
    // pre-compute logarithmic binning boundaries
    for(size_t i = 0; i < BAR_COUNT; i++) {
        bin_boundaries[i] = exp((log(20000) - log(20))*i/(double)BAR_COUNT) * 20;
    }
    bin_boundaries[BAR_COUNT] = 20000;
    char buf[256] = {0};
    snprintf(buf, sizeof(buf)/sizeof(buf[0]), "song: %s \nartist: %s \ncopyright: %s", nsf->song_name, nsf->artist, nsf->copyright);
    SDL_Color color = {192, 0x30, 0x0, 0xff};
    SDL_Surface* text_surf = TTF_RenderUTF8_Solid_Wrapped(g_ctx->font, buf, color, 0);
    nsf->song_info_tx = SDL_CreateTextureFromSurface(g_ctx->renderer, text_surf);
    nsf->song_info_rect.w = text_surf->w;
    nsf->song_info_rect.h = text_surf->h;
    nsf->song_info_rect.x = 10 + offset_x;
    nsf->song_info_rect.y = 10 + offset_y;

    SDL_FreeSurface(text_surf);
}

static float bins[BAR_COUNT] = {0};
static int amps[BAR_COUNT] = {0};

void render_NSF_graphics(Emulator* emulator, NSF* nsf) {
    static int song_num = -1, silent_frames = 0;
    static int minutes = -1, seconds = -1;
    GraphicsContext* g_ctx = &emulator->g_ctx;
#ifdef __ANDROID__
    int offset_x = g_ctx->dest.x, offset_y = g_ctx->dest.y, width = g_ctx->dest.w, height = g_ctx->dest.h;
#else
    int offset_x = 0, offset_y = 0, width = g_ctx->width, height = g_ctx->height;
#endif

    APU* apu = &emulator->apu;
    complx* v = nsf->samples;
    int silent = 1;
    // convert audio buffer to complex values for FFT
    for(size_t i =0; i < AUDIO_BUFF_SIZE; i++) {
        v[i].Re = apu->buff[i];
        v[i].Im = 0;
        if(silent && apu->buff[i] != 0)
            silent = 0;
    }
    if(silent)
        silent_frames++;
    else
        silent_frames = 0;

    if(silent_frames > MAX_SILENCE) {
        next_song(emulator, nsf);
        silent_frames = 0;
        return;
    }
    // FFT to extract frequency spectrum
    fft(nsf->samples, AUDIO_BUFF_SIZE, nsf->temp);

    // Place frequencies into their respective frequency bins
    double end = bin_boundaries[0], step = 20000.0f / AUDIO_BUFF_SIZE / 2, index = 0;
    size_t j = step/20;
    for(size_t i = 0; i < BAR_COUNT; i++) {
        double total = 0;
        size_t bin_count = 0;
        while(index < end) {
            total+=sqrt(v[j].Re * v[j].Re + v[j].Im * v[j].Im);
            index += step;
            j++;
            bin_count++;
        }
        end = bin_boundaries[i + 1];
        bins[i] = 0;
        if(!bin_count) {
            // distribute to empty bins for better visual effect
            int target = i > 0? i - 1 : 0;
            bins[i] = bins[target] / 2;
            if(target != i)
                bins[target] /= 2;
        } else {
            bins[i] = total / bin_count;
        }
    }

    // compute normalization factor for spectrum values for better visualization
    float min_v = FLT_MAX, max_v = FLT_MIN;
    for(size_t i = 0; i < BAR_COUNT; i++) {
        min_v = MIN(min_v, bins[i]);
        max_v = MAX(max_v, bins[i]);
    }
    float factor = 1.0f / (max_v - min_v);

    SDL_RenderClear(g_ctx->renderer);
    SDL_Rect dest;
    int max_bar_h = 0.4f * height, min_bar_h = 0.02f * height, bar_step = min_bar_h / 2;
    for(int i = 0; i < BAR_COUNT; i++) {
        int amp = factor * bins[i] * max_bar_h;
        // animate the visualization bars
        if(amp > amps[i]) {
            amps[i] += bar_step;
        }else {
            amps[i] -= bar_step;
        }
        amps[i] = amps[i] < min_bar_h ? min_bar_h : amps[i] > max_bar_h ? max_bar_h : amps[i];
        dest.y = (height - amps[i]) / 2 + offset_y;
        dest.x = i * width/BAR_COUNT + offset_x;
        dest.w = width/BAR_COUNT - 1;
        dest.h = amps[i];
        SDL_SetRenderDrawColor(
            g_ctx->renderer, i*(g_ctx->width/BAR_COUNT), 0x0,
            g_ctx->width - (i * g_ctx->width/BAR_COUNT), 255);
        SDL_RenderFillRect(g_ctx->renderer, &dest);

    }

    int current_song = nsf->current_song == 0 ? 0 : nsf->current_song - 1;

    if(song_num != nsf->current_song) {
        SDL_DestroyTexture(nsf->song_num_tx);
        char str[32 + MAX_TRACK_NAME_SIZE] = {0};
        SDL_Color color = {0x62, 0x30, 152, 0xff};
        if(nsf->tlbls != NULL) {
            snprintf(
                str, 32 + MAX_TRACK_NAME_SIZE, "%d / %d: %s", nsf->current_song, nsf->total_songs,
                nsf->tlbls[current_song]
            );
        } else {
            snprintf(str, 32, "%d / %d", nsf->current_song, nsf->total_songs);
        }
        SDL_Surface* text_surf = TTF_RenderText_Solid(g_ctx->font, str, color);
        nsf->song_num_tx = SDL_CreateTextureFromSurface(g_ctx->renderer, text_surf);
        nsf->song_num_rect.h = text_surf->h;
        nsf->song_num_rect.w = text_surf->w;
        nsf->song_num_rect.x = 10 + offset_x;
        song_num = nsf->current_song;

        if(nsf->times != NULL) {
            SDL_FreeSurface(text_surf);
            SDL_DestroyTexture(nsf->song_dur_max_tx);
            snprintf(str, 24, "%02d : %02ld", nsf->tick_max / 60000, ((long)nsf->tick_max % 60000) / 1000);

            SDL_Color color1 = {0x0, 0x30, 192, 0xff};
            text_surf = TTF_RenderText_Solid(g_ctx->font, str, color1);
            nsf->song_dur_max_tx = SDL_CreateTextureFromSurface(g_ctx->renderer, text_surf);
            nsf->song_dur_max_rect.h = text_surf->h;
            nsf->song_dur_max_rect.w = text_surf->w;
            nsf->song_dur_max_rect.x = (width - text_surf->w - 10) + offset_x;
            nsf->song_dur_max_rect.y = height - 15 - text_surf->h + offset_y;
        }else {
            nsf->song_num_rect.x = (width - text_surf->w) / 2 + offset_x;;
            nsf->song_num_rect.y = height - 15 - text_surf->h + offset_y;
        }
        nsf->song_num_rect.y = nsf->song_dur_max_rect.y - 0.12*height - text_surf->h;
        SDL_FreeSurface(text_surf);
    }

    if(nsf->times != NULL) {
        int cur_min = nsf->tick / 60000;
        int cur_sec = ((long)nsf->tick % 60000) / 1000;

        dest.x = offset_x + 10;
        dest.y = nsf->song_dur_max_rect.y - 0.06*height;
        dest.w = width - 20;
        dest.h = 0.01 * height;
        SDL_SetRenderDrawColor(g_ctx->renderer, 30, 30, 30, 0x1f);
        SDL_RenderFillRect(g_ctx->renderer, &dest);

        dest.w = (width - 20) * nsf->tick / nsf->tick_max;
        SDL_SetRenderDrawColor(g_ctx->renderer, 60, 0x30, 192, 0xff);
        SDL_RenderFillRect(g_ctx->renderer, &dest);

        if(cur_min != minutes || cur_sec != seconds) {
            SDL_DestroyTexture(nsf->song_dur_tx);
            char str[12];
            SDL_Color color = {0x0, 0x30, 192, 0xff};
            snprintf(str, sizeof(str)/sizeof(str[0]), "%02d : %02d", cur_min, cur_sec);
            SDL_Surface* text_surf = TTF_RenderText_Solid(g_ctx->font, str, color);
            nsf->song_dur_tx = SDL_CreateTextureFromSurface(g_ctx->renderer, text_surf);
            nsf->song_dur_rect.h = text_surf->h;
            nsf->song_dur_rect.w = text_surf->w;
            nsf->song_dur_rect.x = offset_x + 10;
            nsf->song_dur_rect.y = height - 15 - text_surf->h + offset_y;
            SDL_FreeSurface(text_surf);
            minutes = cur_min;
            seconds = cur_sec;
        }
    }

    SDL_RenderCopy(g_ctx->renderer, nsf->song_info_tx, NULL, &nsf->song_info_rect);
    SDL_RenderCopy(g_ctx->renderer, nsf->song_num_tx, NULL, &nsf->song_num_rect);
    SDL_RenderCopy(g_ctx->renderer, nsf->song_dur_tx, NULL, &nsf->song_dur_rect);
    SDL_RenderCopy(g_ctx->renderer, nsf->song_dur_max_tx, NULL, &nsf->song_dur_max_rect);
#ifdef __ANDROID__
    ANDROID_RENDER_TOUCH_CONTROLS(g_ctx);
#endif
    SDL_SetRenderDrawColor(g_ctx->renderer, 0, 0, 0, 255);
    SDL_RenderPresent(g_ctx->renderer);
}

void free_NSF(NSF* nsf) {
    if(nsf == NULL)
        return;
    SDL_DestroyTexture(nsf->song_num_tx);
    SDL_DestroyTexture(nsf->song_info_tx);
    SDL_DestroyTexture(nsf->song_dur_tx);
    SDL_DestroyTexture(nsf->song_dur_max_tx);
    if(nsf->times != NULL)
        free(nsf->times);
    if(nsf->fade)
        free(nsf->fade);
    if(nsf->tlbls != NULL) {
        for(int i = 0; i < nsf->total_songs; i++) {
            if(nsf->tlbls[i] != NULL)
                free(nsf->tlbls[i]);
        }
        free(nsf->tlbls);
    }
    free(nsf);
}