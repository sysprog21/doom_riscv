/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      Fixed point arithemtics, implementation.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __M_FIXED__
#define __M_FIXED__

#include <stdint.h>
#include <stdlib.h>

#include "doomtype.h"
#include "m_recip.h"

/*
 * Fixed point, 32bit as 16.16.
 */

#define FRACBITS 16
#define FRACUNIT (1 << FRACBITS)


typedef int fixed_t;

/*
 * Absolute Value
 */

#if 0
inline static int CONSTFUNC D_abs(fixed_t x)
{
    fixed_t _t = (x), _s;
    _s = _t >> (8 * sizeof _t - 1);
    return (_t ^ _s) - _s;
}
#else
/* let compilers optimize the calls of abs */
#define D_abs abs
#endif

/*
 * Fixed Point Multiplication
 */

inline static fixed_t CONSTFUNC FixedMul(fixed_t a, fixed_t b)
{
    return (fixed_t)((int64_t) a * b >> FRACBITS);
}

/*
 * Fixed Point Division
 */

inline static fixed_t CONSTFUNC FixedDiv(fixed_t a, fixed_t b)
{
    return ((unsigned)D_abs(a)>>14) >= (unsigned)D_abs(b) ? ((a^b)>>31) ^ INT_MAX :
                                                            (fixed_t)(((int64_t) a << FRACBITS) / b);
}

/*
 * FixedMod - returns a % b, guaranteeing 0<=a<b
 * (notice that the C standard for % does not guarantee this)
 */

inline static fixed_t CONSTFUNC FixedMod(fixed_t a, fixed_t b)
{
    if (!a)
        return 0;

    if (b & (b-1))
    {
        fixed_t r = a % b;
        return ((r<0) ? r+b : r);
    }
    return (a & (b-1));
}

/*
 * Approximate Reciprocal of v
 */

inline static CONSTFUNC fixed_t FixedReciprocal(fixed_t v)
{
    unsigned int val = v < 0 ? -v : v;

    unsigned int shift = 0;

    while (val > (1 << FRACBITS))
    {
        val = (val >> 1u);
        shift++;
    }

    fixed_t result = (reciprocalTable[val] >> shift);

    return v < 0 ? -result : result;
}


/*
 * Approximate fixed point divide of a/b using reciprocal. -> a * (1/b).
 */

inline static CONSTFUNC fixed_t FixedApproxDiv(fixed_t a, fixed_t b)
{
    return FixedMul(a, FixedReciprocal(b));
}

#endif
