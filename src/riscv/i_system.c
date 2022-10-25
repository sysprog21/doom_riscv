/*
 * i_system.c
 *
 * System support code
 *
 * Copyright (C) 1993-1996 by id Software, Inc.
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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "doomdef.h"
#include "doomstat.h"

#include "d_main.h"
#include "g_game.h"
#include "m_misc.h"
#include "i_sound.h"
#include "i_video.h"

#include "i_system.h"

#include "console.h"

enum {
	KEY_EVENT = 0,
	MOUSE_MOTION_EVENT = 1,
	MOUSE_BUTTON_EVENT = 2,
};

typedef struct {
	uint32_t keycode;
	uint8_t state;
} key_event_t;

typedef struct {
	int32_t xrel, yrel;
} mouse_motion_t;

typedef struct {
	uint8_t button;
	uint8_t state;
} mouse_button_t;

typedef struct {
	uint32_t type;
	union {
		key_event_t key_event;
		union {
			mouse_motion_t motion;
			mouse_button_t button;
		} mouse;
	};
} emu_event_t;

typedef struct {
	emu_event_t *base;
	size_t start;
} event_queue_t;

enum {
	RELATIVE_MODE_SUBMISSION = 0,
};

typedef struct {
	uint32_t type;
	union {
		union {
			uint8_t enabled;
		} mouse;
	};
} emu_submission_t;

typedef struct {
	emu_submission_t *base;
	size_t end;
} submission_queue_t;

/* Video Ticks tracking */
static uint16_t vt_last = 0;
static uint32_t vt_base = 0;

static event_queue_t event_queue = {
	.base = NULL,
	.start = 0,
};
static submission_queue_t submission_queue = {
	.base = NULL,
	.end = 0,
};
static unsigned int event_count = 0;
const int queues_capacity = 128;

void
I_SetRelativeMode(boolean enabled)
{
	emu_submission_t submission;
	submission.type = RELATIVE_MODE_SUBMISSION;
	submission.mouse.enabled = enabled;
	submission_queue.base[submission_queue.end++] = submission;
	submission_queue.end &= queues_capacity - 1;
	register int a0 asm("a0") = 1;
	register int a7 asm("a7") = 0xfeed;
	asm volatile("scall" : "+r"(a0) : "r"(a7));
}

void
I_Init(void)
{
	void *base = malloc(sizeof(emu_event_t) * queues_capacity + sizeof(emu_submission_t) * queues_capacity);
	event_queue.base = base;
	submission_queue.base = base + sizeof(emu_event_t) * queues_capacity;
	register int a0 asm("a0") = (uintptr_t) base;
	register int a1 asm("a1") = queues_capacity;
	register int a2 asm("a2") = (uintptr_t) &event_count;
	register int a7 asm("a7") = 0xc0de;
	asm volatile("scall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7));
	I_SetRelativeMode(true);
}


byte *
I_ZoneBase(int *size)
{
	/* Give 6M to DOOM */
	*size = 6 * 1024 * 1024;
	return (byte *) malloc (*size);
}


int
I_GetTime(void)
{
	uint16_t vt_now = (uint64_t) clock() / (CLOCKS_PER_SEC / 35.0f);

	if (vt_now < vt_last)
		vt_base += 65536;
	vt_last = vt_now;

	/* TIC_RATE is 35 in theory */
	return vt_base + vt_now;
}

static int PollEvent(emu_event_t* event)
{
	if (event_count <= 0)
		return 0;

	*event = event_queue.base[event_queue.start++];
	event_queue.start &= queues_capacity - 1;
	--event_count;

	return 1;
}

static void
I_GetRemoteEvent(void)
{
	event_t event;

	static byte s_btn = 0;

	boolean mupd = false;
	int mdx = 0;
	int mdy = 0;

	emu_event_t emu_event;
	while (PollEvent(&emu_event)) {
		if (emu_event.type == KEY_EVENT && emu_event.key_event.keycode & 0x40000000) {
			uint32_t keycode = emu_event.key_event.keycode;
			switch (keycode) {
				case 0x40000050:
					keycode = KEY_LEFTARROW;
					break;
				case 0x4000004F:
					keycode = KEY_RIGHTARROW;
					break;
				case 0x40000051:
					keycode = KEY_DOWNARROW;
					break;
				case 0x40000052:
					keycode = KEY_UPARROW;
					break;
				case 0x400000E5:
					keycode = KEY_RSHIFT;
					break;
				case 0x400000E4:
					keycode = KEY_RCTRL;
					break;
				case 0x400000E6:
					keycode = KEY_RALT;
					break;
				case 0x40000048:
					keycode = KEY_PAUSE;
					break;
				case 0x4000003A:
					keycode = KEY_F1;
					break;
				case 0x4000003B:
					keycode = KEY_F2;
					break;
				case 0x4000003C:
					keycode = KEY_F3;
					break;
				case 0x4000003D:
					keycode = KEY_F4;
					break;
				case 0x4000003E:
					keycode = KEY_F5;
					break;
				case 0x4000003F:
					keycode = KEY_F6;
					break;
				case 0x40000040:
					keycode = KEY_F7;
					break;
				case 0x40000041:
					keycode = KEY_F8;
					break;
				case 0x40000042:
					keycode = KEY_F9;
					break;
				case 0x40000043:
					keycode = KEY_F10;
					break;
				case 0x40000044:
					keycode = KEY_F11;
					break;
				case 0x40000045:
					keycode = KEY_F12;
					break;
			}
			emu_event.key_event.keycode = keycode;
		}

		switch (emu_event.type) {
			case KEY_EVENT:
				event.type = emu_event.key_event.state ? ev_keydown : ev_keyup;
				event.data1 = emu_event.key_event.keycode;
				D_PostEvent(&event);
				break;
			case MOUSE_BUTTON_EVENT:
				if (emu_event.mouse.button.state)
					s_btn |= (1 << (emu_event.mouse.button.button - 1));
				else
					s_btn &= ~(1 << (emu_event.mouse.button.button - 1));
				mupd = true;
				break;
			case MOUSE_MOTION_EVENT:
				mdx += emu_event.mouse.motion.xrel;
				mdy += emu_event.mouse.motion.yrel;
				mupd = true;
				break;
		}
	}

	if (mupd) {
		event.type = ev_mouse;
		event.data1 = s_btn;
		event.data2 =	mdx << 2;
		event.data3 = - mdy << 2;	/* Doom is sort of inverted ... */
		D_PostEvent(&event);
	}
}

void
I_StartFrame(void)
{
	/* Nothing to do */
}

void
I_StartTic(void)
{
	I_GetRemoteEvent();
}

ticcmd_t *
I_BaseTiccmd(void)
{
	static ticcmd_t emptycmd;
	return &emptycmd;
}


void
I_Quit(void)
{
	D_QuitNetGame();
	M_SaveDefaults();
	I_ShutdownGraphics();
	exit(0);
}


byte *
I_AllocLow(int length)
{
	/* FIXME: check if memory allocation succeeds */
	return calloc(1, length);
}


void
I_Tactile
( int on,
  int off,
  int total )
{
	// UNUSED.
	on = off = total = 0;
}


void
I_Error(char *error, ...)
{
	va_list	argptr;

	// Message first.
	va_start (argptr,error);
	fprintf (stderr, "Error: ");
	vfprintf (stderr,error,argptr);
	fprintf (stderr, "\n");
	va_end (argptr);

	fflush( stderr );

	// Shutdown. Here might be other errors.
	if (demorecording)
		G_CheckDemoStatus();

	D_QuitNetGame ();
	I_ShutdownGraphics();

	exit(-1);
}
