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

/* $Id: gxdhtserial.h,v 1.6 2007/09/11 15:24:30 Arabidopsis Exp $ */
/* Interface to [de-]serialization for (traditional) halftones */

#ifndef gxdhtserial_INCLUDED
#  define gxdhtserial_INCLUDED

#ifndef gs_memory_DEFINED
#  define gs_memory_DEFINED
typedef struct gs_memory_s  gs_memory_t;
#endif

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s  gx_device;
#endif

#ifndef gx_device_halftone_DEFINED
#  define gx_device_halftone_DEFINED
typedef struct gx_device_halftone_s gx_device_halftone;
#endif

#ifndef gs_imager_state_DEFINED
#  define gs_imager_state_DEFINED
typedef struct gs_imager_state_s    gs_imager_state;
#endif


/*
 * Serialize a halftone.
 *
 * Returns:
 *
 *    0, with *psize set the the amount of space required, if successful
 *
 *    gs_error_rangecheck, with *psize set to the size required, if the
 *        original *psize was not large enough
 *
 *    some other error code, with *psize unchange, in the event of an
 *        error other than lack of space
 */
extern  int     gx_ht_write( const gx_device_halftone * pdht,
                             const gx_device *          dev,
                             byte *                     data,
                             uint *                     psize );

/*
 * Reconstruct a halftone from its serial representation, and install it
 * as the current halftone. The buffer provided is expected to be large
 * enough to hold the entire halftone.
 *
 * The reading and installation phases are combined in this routine so as
 * to avoid unnecessarily allocating a device halftone and its component
 * array, just to release them immediately after installation is complete.
 * There is also not much reason to reconstuct a halftone except to make
 * it the current halftone.
 *
 * Returns the number of bytes read, or < 0 in the event of an error.
 */
extern  int     gx_ht_read_and_install( gs_imager_state *   pis,
                                        const gx_device *   dev,
                                        const byte *        data,
                                        uint                size,
                                        gs_memory_t *       mem );

#endif  /* gxdhtserail_INCLUDED */
