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

/* $Id: gdevsnfb.c,v 1.8 2008/03/23 15:27:47 Arabidopsis Exp $*/
/* Sony News frame buffer driver for GhostScript */
#include "gdevprn.h"
#define prn_dev ((gx_device_printer *)dev) /* needed in 5.31 et seq */
/*#include <sys/types.h> problems with ushort! */
typedef	char *	caddr_t;
typedef	long	off_t;
#include <sys/uio.h>
#include <newsiop/framebuf.h>

/* The device descriptor */
static dev_proc_open_device(sonyfb_open);
static dev_proc_output_page(sonyfb_output_page);
static dev_proc_close_device(sonyfb_close);
static gx_device_procs sonyfb_procs =
  prn_procs(sonyfb_open, sonyfb_output_page, sonyfb_close);
const gx_device_printer far_data gs_sonyfb_device =
  prn_device(sonyfb_procs, "sonyfb",
	102.4,				/* width_10ths */
	103.2,				/* height_10ths */
	100,				/* x_dpi */
	100,				/* y_dpi */
	0,0,0,0,			/* margins */
	1, 0);

static int fb_file = -1;
sPrimRect prect;

static int
sonyfb_open(gx_device *dev)
{
  sScrType stype;

  if(fb_file < 0)
    if((fb_file = open("/dev/fb", 2)) < 0)
      perror("open failed");
    else
      if(ioctl(fb_file, FBIOCGETSCRTYPE, &stype) < 0)
	perror("ioctl failed");
      else
	prect.rect = stype.visiblerect;

  return gdev_prn_open(dev);
}

static int
sonyfb_close(gx_device *dev)
{
  if(fb_file >= 0)
    {
      close(fb_file);
      fb_file = -1;
    }
  return gdev_prn_close(dev);
}

#define FRAME_WIDTH	1024

/* Send the page to the printer. */
static int
sonyfb_output_page(gx_device *dev, int num_copies, int flush)
{
  int l, i, byte_width, height;
  unsigned char *bm, *fbs, *fb;

  byte_width = (dev->width + 7) / 8;
  height = dev->height;
  bm	 = (typeof(bm))prn_dev->mem.base;

  prect.refPoint.x = 0;
  prect.refPoint.y = 0;
  prect.ptnRect = prect.rect;
  
  prect.ptnBM.type  = BM_MEM;
  prect.ptnBM.depth = 1;
  prect.ptnBM.width = (byte_width + 1) / 2;
  prect.ptnBM.rect.origin.x = 0;
  prect.ptnBM.rect.origin.y = 0;
  prect.ptnBM.rect.extent.x = byte_width * 8; /* width in 16bit words */
  prect.ptnBM.rect.extent.y = height;
  prect.ptnBM.base = (typeof(prect.ptnBM.base))bm;
  
  prect.fore_color = 1;
  prect.aux_color = 0;
  prect.planemask = FB_PLANEALL;
  prect.transp = 0;
  prect.func = BF_S;
  prect.clip = prect.rect;
  prect.drawBM.type  = BM_FB;
  prect.drawBM.depth = 1;
  prect.drawBM.width = (prect.rect.extent.x + 15) / 16;
  prect.drawBM.rect = prect.rect;
  prect.drawBM.base = 0;
  
  if(ioctl(fb_file, FBIOCRECTANGLE, &prect) < 0)
    perror("rect ioctl failed");

  return gx_finish_output_page(dev, num_copies, flush);
}
