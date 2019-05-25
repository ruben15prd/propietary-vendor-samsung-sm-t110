/* Copyright (C) 2001-2006 Artifex Software, Inc.
   All Rights Reserved.
  
  This file is part of GNU ghostscript

  GNU ghostscript is free software; you can redistribute it and/or
  modify it under the terms of the version 2 of the GNU General Public
  License as published by the Free Software Foundation.

  GNU ghostscript is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  ghostscript; see the file COPYING. If not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

/* $Id: math_.h,v 1.8 2007/09/11 15:24:19 Arabidopsis Exp $ */
/* Generic substitute for math.h */

#ifndef math__INCLUDED
#  define math__INCLUDED

/* We must include std.h before any file that includes sys/types.h. */
#include "std.h"

#if defined(VMS) && defined(__GNUC__)
/*  DEC VAX/VMS C comes with a math.h file, but GNU VAX/VMS C does not. */
#  include "vmsmath.h"
#else
#  include <math.h>
#endif

/* math.h is different for Turbo and Unix.... */
#ifndef M_PI
#  ifdef PI
#    define M_PI PI
#  else
#    define M_PI 3.14159265358979324
#  endif
#endif

/* Factors for converting between degrees and radians */
#define degrees_to_radians (M_PI / 180.0)
#define radians_to_degrees (180.0 / M_PI)

/*
 * Define the maximum value of a single-precision float.
 * This doesn't seem to be defined in any standard place,
 * and we need an exact value for it.
 */
#undef MAX_FLOAT		/* just in case */
#if defined(vax) || defined(VAX) || defined(__vax) || defined(__VAX)
/* Maximum exponent is +127, 23 bits of fraction. */
#  define MAX_FLOAT\
     ((0x800000 - 1.0) * 0x1000000 * 0x1000000 * 0x10000000 * 0x10000000)
#else
/* IEEE, maximum exponent is +127, 23 bits of fraction + an implied '1'. */
#  define MAX_FLOAT\
     ((0x1000000 - 1.0) * 0x1000000 * 0x1000000 * 0x10000000 * 0x10000000)
#endif

/* Define the hypot procedure on those few systems that don't provide it. */
#if defined(_IBMR2)
/* The RS/6000 has hypot, but math.h doesn't declare it! */
extern double hypot(double, double);
#elif defined(_MSC_VER)
#  define hypot(x,y) _hypot(x,y)
#elif !defined(__TURBOC__) && !defined(BSD4_2) && !defined(VMS) && !defined(__MWERKS__) && !defined(HAVE_HYPOT)
#  define hypot(x,y) sqrt((double)(x)*(x)+(double)(y)*(y))
#endif

#ifdef OSK
/* OSK has atan2 and ldexp, but math.h doesn't declare them! */
extern double atan2(), ldexp();
#endif

/* Intercept calls on sqrt for debugging. */
extern double gs_sqrt(double, const char *, int);
#ifdef DEBUG
#undef sqrt
#define sqrt(x) gs_sqrt(x, __FILE__, __LINE__)
#endif /* DEBUG */

#endif /* math__INCLUDED */
