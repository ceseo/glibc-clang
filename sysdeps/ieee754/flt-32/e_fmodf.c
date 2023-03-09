/* Floating-point remainder function.
   Copyright (C) 2023 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <libm-alias-finite.h>
#include <math.h>
#include "math_config.h"

/* With x = mx * 2^ex and y = my * 2^ey (mx, my, ex, ey being integers), the
   simplest implementation is:

   mx * 2^ex == 2 * mx * 2^(ex - 1)

   while (ex > ey)
     {
       mx *= 2;
       --ex;
       mx %= my;
     }

   With mx/my being mantissa of double floating pointer, on each step the
   argument reduction can be improved 11 (which is sizeo of uint64_t minus
   MANTISSA_WIDTH plus the signal bit):

   while (ex > ey)
     {
       mx << 11;
       ex -= 11;
       mx %= my;
     }  */

float
__ieee754_fmodf (float x, float y)
{
  uint32_t hx = asuint (x);
  uint32_t hy = asuint (y);

  uint32_t sx = hx & SIGN_MASK;
  /* Get |x| and |y|.  */
  hx ^= sx;
  hy &= ~SIGN_MASK;

  /* Special cases:
     - If x or y is a Nan, NaN is returned.
     - If x is an inifinity, a NaN is returned.
     - If y is zero, Nan is returned.
     - If x is +0/-0, and y is not zero, +0/-0 is returned.  */
  if (__glibc_unlikely (hy == 0	|| hx >= EXPONENT_MASK || hy > EXPONENT_MASK))
    return (x * y) / (x * y);

  if (__glibc_unlikely (hx <= hy))
    {
      if (hx < hy)
	return x;
      return sx ? -0.0 : 0.0;
    }

  int ex = get_unbiased_exponent (hx);
  int ey = get_unbiased_exponent (hy);

  /* Common case where exponents are close: ey >= -103 and |x/y| < 2^8,  */
  if (__glibc_likely (ey > MANTISSA_WIDTH && ex - ey <= EXPONENT_WIDTH))
    {
      uint32_t mx = get_explicit_mantissa (hx);
      uint32_t my = get_explicit_mantissa (hy);

      uint32_t d = (ex == ey) ? (mx - my) : (mx << (ex - ey)) % my;
      if (d == 0)
	return 0.0;
      return make_float (d, ey - 1, sx);
    }

  /* Special case, both x and y are subnormal.  */
  if (__glibc_unlikely (ex == 0 && ey == 0))
    return asfloat (hx % hy);

  /* Convert |x| and |y| to 'mx + 2^ex' and 'my + 2^ey'.  Assume that hx is
     not subnormal by conditions above.  */
  uint32_t mx = get_explicit_mantissa (hx);
  ex--;

  uint32_t my = get_explicit_mantissa (hy);
  int lead_zeros_my = EXPONENT_WIDTH;
  if (__glibc_likely (ey > 0))
    ey--;
  else
    {
      my = get_mantissa (hy);
      lead_zeros_my = __builtin_clz (my);
    }

  int tail_zeros_my = __builtin_ctz (my);
  int sides_zeroes = lead_zeros_my + tail_zeros_my;
  int exp_diff = ex - ey;

  int right_shift = exp_diff < tail_zeros_my ? exp_diff : tail_zeros_my;
  my >>= right_shift;
  exp_diff -= right_shift;
  ey += right_shift;

  int left_shift = exp_diff < EXPONENT_WIDTH ? exp_diff : EXPONENT_WIDTH;
  mx <<= left_shift;
  exp_diff -= left_shift;

  mx %= my;

  if (__glibc_unlikely (mx == 0))
    return sx ? -0.0 : 0.0;

  if (exp_diff == 0)
    return make_float (my, ey, sx);

  /* Assume modulo/divide operation is slow, so use multiplication with invert
     values.  */
  uint32_t inv_hy = UINT32_MAX / my;
  while (exp_diff > sides_zeroes) {
    exp_diff -= sides_zeroes;
    uint32_t hd = (mx * inv_hy) >> (BIT_WIDTH - sides_zeroes);
    mx <<= sides_zeroes;
    mx -= hd * my;
    while (__glibc_unlikely (mx > my))
      mx -= my;
  }
  uint32_t hd = (mx * inv_hy) >> (BIT_WIDTH - exp_diff);
  mx <<= exp_diff;
  mx -= hd * my;
  while (__glibc_unlikely (mx > my))
    mx -= my;

  return make_float (mx, ey, sx);
}
libm_alias_finite (__ieee754_fmodf, __fmodf)
