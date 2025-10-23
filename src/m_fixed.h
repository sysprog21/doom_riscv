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
#include <limits.h>  /* For INT_MAX used in FixedDiv */

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
 *
 * IMPORTANT: abs(INT_MIN) is undefined behavior in C. We use a safe
 * bit-twiddling implementation that works correctly for all int32_t values
 * including INT_MIN (which becomes INT_MIN in unsigned interpretation).
 */

inline static int CONSTFUNC D_abs(fixed_t x)
{
    /* Safe absolute value without UB:
     * For negative x: mask = 0xFFFFFFFF, result = (~x + 1) = -x
     * For positive x: mask = 0x00000000, result = x
     * For INT_MIN: returns INT_MIN (as an int), which in fixed-point context
     * represents the magnitude correctly when cast to unsigned.
     */
    uint32_t ux = (uint32_t)x;
    uint32_t mask = ux >> 31;  /* 0 if positive, 1 if negative */
    return (int)((ux ^ -mask) + mask);
}

/*
 * Fixed Point Multiplication
 *
 * RISC-V M extension optimization:
 * - With M extension (rv32im): Direct use of mul/mulh instructions (2-3 cycles)
 * - Without M extension (rv32i): Falls back to libgcc __muldi3 (~100+ cycles)
 *
 * The 64-bit multiply is necessary for correct 16.16 fixed-point arithmetic.
 * With M extension, we use inline assembly to avoid libgcc overhead.
 *
 * Detection strategy:
 * We check for __riscv_mul (standard GCC/Clang macro) OR __riscv_m (M extension version).
 * This provides maximum compatibility across toolchain versions.
 *
 * TODO: __riscv_zmmul (multiply-only subset) could also be checked, but doom_riscv
 * targets full RV32IM which includes both multiply and divide.
 */

/* Detect RISC-V M extension multiply support */
#if defined(__riscv) && (defined(__riscv_mul) || defined(__riscv_m))
#define HAVE_RISCV_HW_MUL 1
#else
#define HAVE_RISCV_HW_MUL 0
#endif

/* Force inlining to eliminate function call overhead even at -O0 */
#ifdef __GNUC__
#define FIXED_INLINE __attribute__((always_inline)) inline static
#else
#define FIXED_INLINE inline static
#endif

FIXED_INLINE fixed_t CONSTFUNC FixedMul(fixed_t a, fixed_t b)
{
#if HAVE_RISCV_HW_MUL
    /* Use RV32M mul/mulh instructions directly for optimal performance.
     * This avoids libgcc __muldi3 call and compiles to just 2 instructions.
     *
     * Math: (a * b) >> 16
     * Implementation:
     * - mul  computes lower 32 bits of (a * b)
     * - mulh computes upper 32 bits of (a * b), signed
     * - We extract bits [16..47] of the 64-bit product by:
     *   1. Shifting high left by 16 to get bits [32..47] → bits [16..31]
     *   2. Shifting low right by 16 to get bits [16..31] → bits [0..15]
     *   3. OR'ing them together to reconstruct the middle 32 bits
     *
     * Correctness: All shifts done in unsigned domain to avoid UB,
     * then cast back to signed for proper two's complement interpretation.
     */
    int32_t high, low;
    __asm__ (
        "mul  %0, %2, %3\n\t"   /* low = a * b (lower 32 bits) */
        "mulh %1, %2, %3"       /* high = a * b (upper 32 bits, signed) */
        : "=&r" (low), "=&r" (high)  /* early-clobber to prevent input overlap */
        : "r" (a), "r" (b)
    );
    /* Avoid UB from signed left shift: cast to unsigned, shift, then back to signed */
    return (fixed_t)(((uint32_t)high << (32 - FRACBITS)) | ((uint32_t)low >> FRACBITS));
#else
    /* Fallback to 64-bit multiply for non-RISC-V or rv32i targets */
    return (fixed_t)((int64_t) a * b >> FRACBITS);
#endif
}

/*
 * Fixed Point Multiply with Right Shift
 *
 * Optimized version of: (a * b) >> shift
 * Common pattern in sprite projection and rendering calculations.
 *
 * This function safely handles arithmetic right shift of the 64-bit signed
 * product without invoking libgcc __ashrdi3 helper on RV32M.
 *
 * IMPORTANT: shift must be in range [0..63]. Values outside this range
 * produce undefined results. Current call sites use constant shifts (8, 16).
 *
 * Performance: On RV32M, this uses hardware mul/mulh and avoids libgcc calls.
 * The benefit is eliminating the 64-bit shift helper, not register reduction.
 */
FIXED_INLINE fixed_t CONSTFUNC FixedMulShift(fixed_t a, fixed_t b, unsigned shift)
{
#if HAVE_RISCV_HW_MUL
    int32_t high, low;
    __asm__ (
        "mul  %0, %2, %3\n\t"   /* low = a * b (lower 32 bits) */
        "mulh %1, %2, %3"       /* high = a * b (upper 32 bits, signed) */
        : "=&r" (low), "=&r" (high)
        : "r" (a), "r" (b)
    );

    /* Safe arithmetic right shift of 64-bit signed product, avoiding UB:
     * - shift == 0: return low 32 bits
     * - shift < 32: extract middle 32 bits via shifts and OR
     * - shift < 64: arithmetic shift of high part
     * - shift >= 64: sign-extend to infinity
     *
     * This avoids UB from shifting by >= word size and libgcc __ashrdi3.
     */
    if (shift == 0)
        return low;
    if (shift < 32)
        return (fixed_t)(((uint32_t) high << (32 - shift)) | ((uint32_t) low >> shift));
    if (shift < 64)
        return (int32_t) high >> (shift - 32);
    return (high < 0) ? -1 : 0;
#else
    /* Fallback for non-RISC-V or rv32i targets */
    return (fixed_t)((int64_t) a * b >> shift);
#endif
}

/*
 * Fixed Point Multiply with Pre-Shifted First Operand
 *
 * Semantic helper for: (a >> shift) * b
 * Common pattern in collision detection where operands are pre-shifted
 * to prevent overflow (e.g., line->dy >> FRACBITS).
 *
 * IMPORTANT: This is a readability wrapper, NOT a performance optimization.
 * The shift still occurs; this function exists to make the intent clear at
 * call sites and to ensure consistent semantics (arithmetic right shift before
 * multiplication, not combining shifts with the multiply).
 *
 * Note: shifta must be in range [0..31] to avoid UB. Current call sites
 * use constant values (FRACBITS=16, 8).
 */
FIXED_INLINE fixed_t CONSTFUNC FixedMulPreShift(fixed_t a, unsigned shifta, fixed_t b)
{
    /* Perform shift then multiply using existing FixedMul */
    return FixedMul(a >> shifta, b);
}

/*
 * Fixed Point Multiply with Negation
 *
 * Semantic helper for: -(a * b)
 * Common pattern in wall rendering for calculating negative step values.
 *
 * This is implemented as -FixedMul(a, b) to avoid duplicating the bit
 * assembly logic and to allow the compiler to optimize the negation.
 * On RV32M, GCC typically emits a single 'sub rd, x0, rd' instruction
 * after the multiply.
 *
 * IMPORTANT: This preserves the original behavior including potential UB
 * when FixedMul returns INT_MIN (negation overflow). On two's complement
 * RISC-V targets, this wraps to INT_MIN as expected, but it's technically UB.
 *
 * For strict UB-avoidance, one could use:
 *   return (fixed_t)(0u - (uint32_t)FixedMul(a, b));
 */
FIXED_INLINE fixed_t CONSTFUNC FixedMulNeg(fixed_t a, fixed_t b)
{
    return -FixedMul(a, b);
}

/*
 * Fixed Point Division
 *
 * RISC-V M extension detection:
 * - With M extension (rv32im): Uses hardware div instruction, fast
 * - Without M extension (rv32i): Falls back to libgcc software emulation, very slow
 *
 * For rv32i targets, consider using FixedApproxDiv() instead, which uses
 * a reciprocal table and is much faster than software division.
 */

FIXED_INLINE fixed_t CONSTFUNC FixedDiv(fixed_t a, fixed_t b)
{
    return ((unsigned)D_abs(a)>>14) >= (unsigned)D_abs(b) ? ((a^b)>>31) ^ INT_MAX :
                                                            (fixed_t)(((int64_t) a << FRACBITS) / b);
}

/*
 * FixedMod - returns a % b, guaranteeing 0<=a<b
 * (notice that the C standard for % does not guarantee this)
 */

FIXED_INLINE fixed_t CONSTFUNC FixedMod(fixed_t a, fixed_t b)
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

FIXED_INLINE CONSTFUNC fixed_t FixedReciprocal(fixed_t v)
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
 *
 * This function uses a reciprocal table and FixedMul, avoiding division entirely.
 * On rv32i (without M extension), this is significantly faster than FixedDiv.
 */

FIXED_INLINE CONSTFUNC fixed_t FixedApproxDiv(fixed_t a, fixed_t b)
{
    return FixedMul(a, FixedReciprocal(b));
}

#endif
