/*
 * console.c
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
#include <stdio.h>
#include <stdarg.h>


void
console_init(void)
{
}

void
console_putchar(char c)
{
	putchar(c);
}

char
console_getchar(void)
{
	return getchar();
}

int
console_getchar_nowait(void)
{
	/* TODO: implement this */
	return -1;
}

void
console_puts(const char *p)
{
	puts(p);
}

int
console_printf(const char *fmt, ...)
{
	va_list va;
	int l;

	va_start(va, fmt);
	l = vprintf(fmt, va);
	va_end(va);

	return l;
}
