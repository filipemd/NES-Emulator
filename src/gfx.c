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


#include <SDL2/SDL.h>

#include "gfx.h"
#include "utils.h"
#include "font.h"

#ifdef __ANDROID__
#include "touchpad.h"
#endif

void get_graphics_context(GraphicsContext* ctx){

    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
#ifdef __ANDROID__
    ctx->font = TTF_OpenFont("asap.ttf", (int)(ctx->screen_height * 0.05));
    if(ctx->font == NULL){
        LOG(ERROR, SDL_GetError());
    }
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    ctx->window = SDL_CreateWindow(
        "NES Emulator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        // width and height not used in FULLSCREEN
        0,
        0,
        SDL_WINDOW_FULLSCREEN_DESKTOP
        | SDL_WINDOW_ALLOW_HIGHDPI
    );
#else
    SDL_RWops* rw = SDL_RWFromMem(font_data, sizeof(font_data));
    ctx->font = TTF_OpenFontRW(rw, 1, 11);
    if(ctx->font == NULL){
        LOG(ERROR, SDL_GetError());
    }
    ctx->window = SDL_CreateWindow(
        "NES Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        ctx->width * (int)ctx->scale,
        ctx->height * (int)ctx->scale,
        SDL_WINDOW_SHOWN
        | SDL_WINDOW_OPENGL
        | SDL_WINDOW_RESIZABLE
        | SDL_WINDOW_ALLOW_HIGHDPI
    );
#endif

    if(ctx->window == NULL){
        LOG(ERROR, SDL_GetError());
        quit(EXIT_FAILURE);
    }
    SDL_SetWindowMinimumSize(ctx->window, ctx->width, ctx->height);

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED);
    if(ctx->renderer == NULL){
        LOG(ERROR, SDL_GetError());
        quit(EXIT_FAILURE);
    }

#ifdef __ANDROID__
    ctx->dest.h = ctx->screen_height;
    ctx->dest.w = (ctx->width * ctx->dest.h) / ctx->height;
    ctx->dest.x = (ctx->screen_width - ctx->dest.w) / 2;
    ctx->dest.y = 0;
#else
    SDL_RenderSetLogicalSize(ctx->renderer, ctx->width, ctx->height);
    SDL_RenderSetIntegerScale(ctx->renderer, 1);
    SDL_RenderSetScale(ctx->renderer, ctx->scale, ctx->scale);
#endif

    ctx->texture = SDL_CreateTexture(
        ctx->renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_TARGET,
        ctx->width,
        ctx->height
    );

    if(ctx->texture == NULL){
        LOG(ERROR, SDL_GetError());
        quit(EXIT_FAILURE);
    }

    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx->renderer);
    SDL_RenderPresent(ctx->renderer);

    LOG(DEBUG, "Initialized SDL subsystem");
}

void render_graphics(GraphicsContext* g_ctx, const uint32_t* buffer){
    SDL_RenderClear(g_ctx->renderer);
    SDL_UpdateTexture(g_ctx->texture, NULL, buffer, (int)(g_ctx->width * sizeof(uint32_t)));
#ifdef __ANDROID__
    SDL_RenderCopy(g_ctx->renderer, g_ctx->texture, NULL, &g_ctx->dest);
    ANDROID_RENDER_TOUCH_CONTROLS(g_ctx);
#else
    SDL_RenderCopy(g_ctx->renderer, g_ctx->texture, NULL, NULL);
#endif
    SDL_SetRenderDrawColor(g_ctx->renderer, 0, 0, 0, 255);
    SDL_RenderPresent(g_ctx->renderer);
}

void free_graphics(GraphicsContext* ctx){
    TTF_CloseFont(ctx->font);
    TTF_Quit();
    SDL_DestroyTexture(ctx->texture);
    SDL_DestroyRenderer(ctx->renderer);
    SDL_DestroyWindow(ctx->window);
    SDL_CloseAudioDevice(ctx->audio_device);
    SDL_Quit();
    LOG(DEBUG, "Graphics clean up complete");
}
