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

/* $Id: gsfont.h,v 1.10 2007/09/11 15:24:10 Arabidopsis Exp $ */
/* Generic font and font cache interface */

#ifndef gsfont_INCLUDED
#  define gsfont_INCLUDED

#ifndef gs_matrix_DEFINED
#  define gs_matrix_DEFINED
typedef struct gs_matrix_s gs_matrix;
#endif

/* A 'font directory' object (to avoid making fonts global). */
/* 'directory' is something of a misnomer: this structure */
/* just keeps track of the defined fonts, and the scaled font and */
/* rendered character caches. */
#ifndef gs_font_dir_DEFINED
#  define gs_font_dir_DEFINED
typedef struct gs_font_dir_s gs_font_dir;
#endif

/* Font objects */
#ifndef gs_font_DEFINED
#  define gs_font_DEFINED
typedef struct gs_font_s gs_font;
#endif

/* Initialization */
/* These procedures return 0 if they fail. */
gs_font_dir *gs_font_dir_alloc2(gs_memory_t * struct_mem,
				gs_memory_t * bits_mem);
gs_font_dir *gs_font_dir_alloc2_limits(gs_memory_t * struct_mem,
				       gs_memory_t * bits_mem,
				       uint smax, uint bmax, uint mmax,
				       uint cmax, uint upper);

/* Backward compatibility */
#define gs_font_dir_alloc(mem) gs_font_dir_alloc2(mem, mem)
#define gs_font_dir_alloc_limits(mem, smax, bmax, mmax, cmax, upper)\
  gs_font_dir_alloc2_limits(mem, mem, smax, bmax, mmax, cmax, upper)

/* Font manipulations */
/* Use gs_definefont only with original (unscaled) fonts! */
int gs_definefont(gs_font_dir *, gs_font *);

/* Find a sililar registered font of same type. */
int gs_font_find_similar(const gs_font_dir * pdir, const gs_font **ppfont, 
			   int (*similar)(const gs_font *, const gs_font *));

/* gs_scalefont and gs_makefont return 0 if the scaled font */
/* was already in the cache, 1 if a new font was created. */
int gs_scalefont(gs_font_dir *, const gs_font *, floatp, gs_font **);
int gs_makefont(gs_font_dir *, const gs_font *, const gs_matrix *, gs_font **);
int gs_setfont(gs_state *, gs_font *);
gs_font *gs_currentfont(const gs_state *);
gs_font *gs_rootfont(const gs_state *);
void gs_set_currentfont(gs_state *, gs_font *);
int  gs_purge_font(gs_font *);
/* Locate a gs_font by gs_id. */
gs_font *gs_find_font_by_id(gs_font_dir *pdir, gs_id id, gs_matrix *FontMatrix);

/* Font cache parameter operations */
void gs_cachestatus(const gs_font_dir *, uint[7]);

#define gs_setcachelimit(pdir,limit) gs_setcacheupper(pdir,limit)
uint gs_currentcachesize(const gs_font_dir *);
int gs_setcachesize(gs_font_dir *, uint);
uint gs_currentcachelower(const gs_font_dir *);
int gs_setcachelower(gs_font_dir *, uint);
uint gs_currentcacheupper(const gs_font_dir *);
int gs_setcacheupper(gs_font_dir *, uint);
uint gs_currentaligntopixels(const gs_font_dir *);
int gs_setaligntopixels(gs_font_dir *, uint);
uint gs_currentgridfittt(const gs_font_dir *);
int gs_setgridfittt(gs_font_dir *, uint);

#endif /* gsfont_INCLUDED */
