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


#include "timers.h"

#define G 1000000000L
#define M 1000000L

#ifdef _WIN // windows 64 and 32 bit
#include <windows.h>
#define SLEEP_RESOLUTION_MS 1

typedef struct Timer_t {
    LARGE_INTEGER start, diff;
    LARGE_INTEGER frequency;
    uint64_t period_ms;
} Timer_t;

static int timerPeriod = 0;

static void set_resolution();
static void reset_resolution();

void init_timer(Timer* timer, uint64_t sweep){
    Timer_t* t = calloc(1, sizeof(Timer_t));
    timer->timer = t;
    t->period_ms = sweep / M;
    if(!QueryPerformanceFrequency(&t->frequency)){
        LOG(ERROR, "Could not acquire timer resolution ");
        quit(EXIT_FAILURE);
    } else {
        LOG(DEBUG, "Performance counter frequency %lu Hz", t->frequency.QuadPart);
    }
    set_resolution();
}

void toggle_timer_resolution(){
    // toggles from high resolution to low to reduce CPU utilization during a pause
    if(timerPeriod) {
        reset_resolution();
        LOG(DEBUG, "Using Low resolution sleep");
    }
    else {
        set_resolution();
        LOG(DEBUG, "Using High resolution sleep");
    }
}

void mark_start(Timer* timer){
    Timer_t* t = timer->timer;
    QueryPerformanceCounter(&t->start);
}

void mark_end(Timer* timer){
    Timer_t* t = timer->timer;
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    // compute diff in milliseconds
    t->diff.QuadPart = end.QuadPart - t->start.QuadPart;
    t->diff.QuadPart *= 1000;
    t->diff.QuadPart /= t->frequency.QuadPart;
}

int adjusted_wait(Timer* timer){
    Timer_t* t = timer->timer;
    int64_t req_ms = t->period_ms - t->diff.QuadPart;
    if(req_ms < SLEEP_RESOLUTION_MS){
        return 0;
    }
    Sleep(req_ms);
    return 0;
}


int wait(uint64_t period_ms){
    Sleep(period_ms);
    return 0;
}


double get_diff_ms(Timer* timer){
    Timer_t* t = timer->timer;
    return t->diff.QuadPart;
}

void release_timer(Timer* timer){
    reset_resolution();
    if(timer->timer != NULL)
        free(timer->timer);
}

static void set_resolution(){
    // set only once
    if(!timerPeriod) {
        // 1 millisecond resolution
        timeBeginPeriod(SLEEP_RESOLUTION_MS);
        timerPeriod = SLEEP_RESOLUTION_MS;
    }
}

static void reset_resolution(){
    if(timerPeriod) {
        timeEndPeriod(SLEEP_RESOLUTION_MS);
        timerPeriod = 0;
    }
}

#else // linux
#include <unistd.h>
#include "time.h"


typedef struct Timer_t {
    struct timespec start, diff;
    uint64_t clock_res;
    uint64_t period_ns;
} Timer_t;

static inline void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *result);

void init_timer(Timer* timer, uint64_t period){
    Timer_t* t = calloc(1, sizeof(Timer_t));
    timer->timer = t;
    t->period_ns = period;
    struct timespec res;
    clock_getres(CLOCK_MONOTONIC, &res);
    t->clock_res = res.tv_sec * G + res.tv_nsec;
    LOG(DEBUG, "Clock resolution: %lu ns", t->clock_res);
}


void mark_start(Timer* timer){
    Timer_t* t = timer->timer;
    clock_gettime(CLOCK_MONOTONIC, &t->start);
}

void mark_end(Timer* timer){
    Timer_t* t = timer->timer;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    timespec_diff(&end, &t->start, &t->diff);
}

int adjusted_wait(Timer* timer){
    Timer_t* t = timer->timer;
    int64_t req_period_ns = (int64_t)(t->period_ns - (t->diff.tv_sec * G + t->diff.tv_nsec));
    if(req_period_ns <= t->clock_res)
        return 0;

    struct timespec req = {
            .tv_sec=req_period_ns / G,
            .tv_nsec=req_period_ns % G
    };
    return clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL);
}


int wait(uint64_t period_ms){
    int64_t req_period_ns = (int64_t)period_ms * M;
    struct timespec req = {
        .tv_sec=req_period_ns / G,
        .tv_nsec=req_period_ns % G
    };
    return clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL);
}


double get_diff_ms(Timer* timer){
    Timer_t* t = timer->timer;
    return ((double)t->diff.tv_sec * 1000 + (double)t->diff.tv_nsec / M);
}


void release_timer(Timer* timer){
    if(timer->timer != NULL)
        free(timer->timer);
}


static inline void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *result) {
    result->tv_sec  = a->tv_sec  - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0) {
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}

#endif // linux