/*
 * i_video.c
 *
 * Video system support code
 *
 * Copyright (C) 2022 National Cheng Kung University, Taiwan.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdint.h>
#include <string.h>

#include "doomdef.h"

#include "i_system.h"
#include "v_video.h"
#include "i_video.h"

static uint32_t buffer[SCREENWIDTH * SCREENHEIGHT];
static uint32_t video_pal[256];

// Dirty region tracking: mark which lines have changed
static byte dirty_lines[SCREENHEIGHT];
static int dirty_min_y = SCREENHEIGHT;
static int dirty_max_y = -1;

// Mark a range of lines as dirty (modified)
// This is called from rendering functions to track which parts of
// the screen have changed and need to be updated
void
I_MarkDirtyLines(int y_start, int y_end)
{
	if (y_start < 0) y_start = 0;
	if (y_end >= SCREENHEIGHT) y_end = SCREENHEIGHT - 1;
	if (y_start > y_end) return;

	// Update bounding box
	if (y_start < dirty_min_y) dirty_min_y = y_start;
	if (y_end > dirty_max_y) dirty_max_y = y_end;

	// Fast path for single line (common case for horizontal spans)
	if (y_start == y_end) {
		dirty_lines[y_start] = 1;
		return;
	}

	// Mark individual lines
	for (int y = y_start; y <= y_end; y++)
		dirty_lines[y] = 1;
}

void
I_InitGraphics(void)
{
	usegamma = 1;

	// Initialize dirty region tracking - mark entire screen as dirty
	I_MarkDirtyLines(0, SCREENHEIGHT - 1);

	register int a0 asm("a0") = (uintptr_t) buffer;
	register int a1 asm("a1") = SCREENWIDTH;
	register int a2 asm("a2") = SCREENHEIGHT;
	register int a7 asm("a7") = 0xbeef;

	asm volatile("ecall"
	             : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7));
}

void
I_ShutdownGraphics(void)
{
	/* Don't need to do anything really ... */
}


void
I_SetPalette(byte* palette)
{
	for (int i=0 ; i<256 ; i++) {
		byte r = gammatable[usegamma][*palette++];
		byte g = gammatable[usegamma][*palette++];
		byte b = gammatable[usegamma][*palette++];
		video_pal[i] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
	}
}


void
I_UpdateNoBlit(void)
{
}

void
I_FinishUpdate (void)
{
	// Only update dirty lines to reduce memory traffic.
	// In practice, Doom redraws most of the screen each frame, but this
	// still helps reduce cache pollution and syscall overhead

	int updated_lines = 0;

	// Quick check: if nothing is dirty, skip update
	if (dirty_max_y < 0) {
		// No changes this frame, still need to call ecall for frame pacing
		goto do_syscall;
	}

	// Copy only dirty lines from palette-indexed buffer to RGB buffer
	for (int y = dirty_min_y; y <= dirty_max_y; y++) {
		if (dirty_lines[y]) {
			int offset = y * SCREENWIDTH;
			for (int x = 0; x < SCREENWIDTH; x++)
				buffer[offset + x] = video_pal[screens[0][offset + x]];
			dirty_lines[y] = 0;
			updated_lines++;
		}
	}

	// Reset dirty tracking
	dirty_min_y = SCREENHEIGHT;
	dirty_max_y = -1;

do_syscall:
	// Always call ecall to present the frame (for timing and vsync)
	register int a0 asm("a0") = (uintptr_t) buffer;
	register int a1 asm("a1") = SCREENWIDTH;
	register int a2 asm("a2") = SCREENHEIGHT;
	register int a7 asm("a7") = 0xbeef;

	asm volatile("ecall"
	             : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7));

	/* Very crude FPS measure (time to render 100 frames */
#if 1
	static int frame_cnt = 0;
	static int tick_prev = 0;

	if (++frame_cnt == 100)
	{
		int tick_now = I_GetTime();
		printf("%d\n", tick_now - tick_prev);
		tick_prev = tick_now;
		frame_cnt = 0;
	}
#endif
}


void
I_WaitVBL(int count)
{
}


void
I_ReadScreen(byte* scr)
{
	memcpy(
		scr,
		screens[0],
		SCREENHEIGHT * SCREENWIDTH
	);
}
