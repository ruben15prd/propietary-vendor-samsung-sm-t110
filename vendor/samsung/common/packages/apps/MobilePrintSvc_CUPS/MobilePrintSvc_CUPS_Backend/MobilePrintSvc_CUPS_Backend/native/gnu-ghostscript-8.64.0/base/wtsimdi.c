/* Copyright (C) 2005-2006 Artifex Software, Inc.
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

/* $Id: wtsimdi.c,v 1.3 2007/09/11 15:23:49 Arabidopsis Exp $ */

/*
 * Create an imdi memory device for page or band buffering,
 * possibly preceded by a plane extraction device.
 */
int
wtsimdi_create_buf_device(gx_device **pbdev, gx_device *target,
    const gx_render_plane_t *render_plane, gs_memory_t *mem, bool for_band)
{
    int plane_index = (render_plane ? render_plane->index : -1);
    int depth;
    const gx_device_memory *mdproto;
    gx_device_memory *mdev;

    if (plane_index >= 0)
	depth = render_plane->depth;
    else
	depth = target->color_info.depth;
    mdproto = gdev_mem_device_for_bits(depth);
    if (mdproto == 0)
	return_error(gs_error_rangecheck);
    if (mem) {
	mdev = gs_alloc_struct(mem, gx_device_memory, &st_device_memory,
			       "create_buf_device");
	if (mdev == 0)
	    return_error(gs_error_VMerror);
    } else {
	mdev = (gx_device_memory *)*pbdev;
    }
    if (target == (gx_device *)mdev) {
	/* The following is a special hack for setting up printer devices. */
	assign_dev_procs(mdev, mdproto);
        check_device_separable((gx_device *)mdev);
	gx_device_fill_in_procs((gx_device *)mdev);
    } else
	gs_make_mem_device(mdev, mdproto, mem, (for_band ? 1 : 0),
			   (target == (gx_device *)mdev ? NULL : target));
    mdev->width = target->width;
    /*
     * The matrix in the memory device is irrelevant,
     * because all we do with the device is call the device-level
     * output procedures, but we may as well set it to
     * something halfway reasonable.
     */
    gs_deviceinitialmatrix(target, &mdev->initial_matrix);
    /****** QUESTIONABLE, BUT BETTER THAN OMITTING ******/
    mdev->color_info = target->color_info;
    *pbdev = (gx_device *)mdev;
    return 0;
}

#define wtsimdi_prn_procs(print_page)\
	 { print_page,\
	   gx_default_print_page_copies,\
	   { wtsimdi_create_buf_device,\
	     gx_default_size_buf_device,\
	     gx_default_setup_buf_device,\
	     gx_default_destroy_buf_device\
	   },\
	   gdev_prn_default_get_space_params,\
	   gx_default_start_render_thread,\
	   gx_default_open_render_device,\
	   gx_default_close_render_device,\
	   gx_default_buffer_page\
	 }

