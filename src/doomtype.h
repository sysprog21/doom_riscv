// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//      Simple basic typedefs, isolated here to make it easier
//       separating modules.
//
//-----------------------------------------------------------------------------


#ifndef __DOOMTYPE__
#define __DOOMTYPE__


#ifndef __BYTEBOOL__
#define __BYTEBOOL__
// DOOM uses int-sized boolean for struct layout compatibility.
// C23 makes true/false keywords, so we can't use enum {false, true}.
// Solution: Use int for boolean type, define true/false if needed.
#if defined(__cplusplus) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L)
// C++/C23: true, false are keywords; use int to preserve struct sizes
typedef int boolean;
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
// C99/C11: true, false available via stdbool.h; use int for size
#include <stdbool.h>
typedef int boolean;
#else
// Pre-C99: define the enum and macros
typedef enum {qfalse, qtrue} boolean;
#define false qfalse
#define true qtrue
#endif
typedef unsigned char byte;
#endif


// Limits alias
#include <limits.h>

#define MAXCHAR         ((char)CHAR_MAX)
#define MINCHAR         ((char)CHAR_MIN)

#define MAXSHORT        ((short)SHRT_MAX)
#define MINSHORT        ((short)SHRT_MIN)

#define MAXINT          ((int)INT_MAX)
#define MININT          ((int)INT_MIN)

#define MAXLONG         ((long)LONG_MAX)
#define MINLONG         ((long)LONG_MIN)


#if defined(__GNUC__) || defined(__clang__)
#define CONSTFUNC __attribute__((const))
#define PUREFUNC __attribute__((pure))
#else
#define CONSTFUNC
#define PUREFUNC
#endif

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
