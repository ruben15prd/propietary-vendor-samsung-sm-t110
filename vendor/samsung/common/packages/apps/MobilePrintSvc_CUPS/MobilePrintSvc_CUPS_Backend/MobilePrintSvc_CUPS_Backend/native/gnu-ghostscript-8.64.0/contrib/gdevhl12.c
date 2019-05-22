/*
   Contributors:
     Created by Marek Michalkiewicz <marekm@amelek.gda.pl>

   THIS SOFTWARE IS NOT COPYRIGHTED

   This source code is offered for use in the public domain.  You may
   use, modify or distribute it freely.

   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY.  ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
   DISCLAIMED.  This includes but is not limited to warranties of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   (Legalese borrowed from the Mingw32 runtime libraries.)
 */

/*$Id: gdevhl12.c,v 1.2 2008/03/23 15:28:31 Arabidopsis Exp $ */
/* gdevhl12.c - Brother HL-1250 driver for Ghostscript */

/*
   This driver handles the 1200x600 dpi mode, and some settings like
   paper source and toner save mode.  600x600 dpi and below is handled
   by gdevdljm.c (basically as in the ljet4 device, with minor changes
   in initialization and margins, and PCL wrapped in PJL).

   There are two device names made available by this driver, which
   differ only in the default (maximum) resolution:

     -sDEVICE=hl1240
	600x600 dpi, for HL-1030/1240 (uses standard PCL modes 2 and 3)

     -sDEVICE=hl1250
	1200x600 dpi, for HL-1050/1070/1250/1270N
	(uses special compression mode 1027 for 1200x600 dpi,
	otherwise modes 2 and 3 as above)

   Tested with a HL-1250, both 1200x600 and 600x600 dpi.  Please test
   with other models and report results (both problems and success).
   Some notes on the other models:

     HL-1070 and HL-1270N
	can also print Postscript directly, at maximum 600x600 dpi
	(this driver can print 1200x600 dpi using mode 1027).

     HL-1240
	should no longer be limited to 300 dpi - this driver sends
	the recommended "@PJL SET RESOLUTION=..." command.  PLEASE TEST.
	There have been some vague problem reports (wrong margins? not
	clear for what paper size, quite possibly Letter, not A4?) with
	this printer, if you have it and some Ghostscript hacking skills,
	_please_ try to help, fix it and send me a patch - thanks!

     HL-1030
	should no longer be "Paperweight" despite the "for Windows only"
	warning :-) - it knows enough PCL to print raster graphics, but
	*requires* "@PJL ENTER LANGUAGE=PCL", so it does not work with
	the ljet4 driver sending raw PCL.  The windows driver uses an
	undocumented compression mode 1030, but modes 2 and 3 work too.
	PLEASE TEST.

   New models, still need to be tested:
     HL-1230 (GDI printer)
     HL-1440 (GDI printer)
     HL-1450 (PCL6 and PS2 emulation)
     HL-1470N (PCL6 and PS2 emulation)

   Additional settings (gs command line options) for this driver:

     -dEconoMode=0...2
	toner save mode: 0 = disabled (default), 1 = 25%, 2 = 50%

     -dPaperType=0...4
	paper type: 0 = plain (default), 1 = thin, 2 = thick, 3 = thicker,
	4 = transparency

     -dSourceTray=0...4
	paper source: 0 = auto (default), 1 = dual, 2 = upper, 3 = lower,
	4 = manual (not all models support the optional lower tray)

   The 1200x600 dpi mode may require a lot of printer memory - the manual
   recommends at least 10 MB; for me it seems to work well with 8 MB: 4 MB
   standard + 4 MB "finally found some good use for that old PS/2 SIMM" :-).
   Even at just 600x600 dpi (normal PCL 5 control codes) print quality is
   very good, at 1200x600 dpi it is difficult to see the difference (more
   visible on grayscale images).  So 600x600 dpi is best for general use.

   The HL-1250 also supports PCL 6 (PCL XL) but
   - I don't have the documentation for that
   - the pxlmono driver works, but the output looks a bit different
     from that generated by the ljet4 or (this) 1200x600 dpi driver
     (lines appear very thin - not yet sure if this is a driver bug
     or differences in PCL 6 implementation, someone should check on
     a HP printer, does tiger.ps printed using the ljet4 and pxlmono
     drivers, both at 600x600 dpi, look the same?)
   so let's use the good old ljet4 (PCL 5) driver for now.
   (PCL 6 seems to offer better compression though.)

   To do: the undocumented mode 1030 may offer better compression
   (may help print complex graphics at 600 dpi - the HL-1030/1240 have
   only 2 MB of non-expandable RAM, which may not be enough for data
   that doesn't compress well).  The windows driver uses this mode for
   all models, not just HL-1030.  Documentation may be available later.

   Many thanks to Richard Thomas <richardt@brother.co.uk> for help
   (sending me a HL-1250, documentation, answering my questions).

   Thanks to L. Peter Deutsch <ghost@aladdin.com> for making gdevdjet.c
   and gdevdljm.[ch] available under a dual AFPL/GPL license.
 */

#include "gdevprn.h"
#include "gdevdljm.h"

#ifdef X_DPI
#  define X_DPI2 X_DPI
#else
#  define X_DPI 600
#  define X_DPI2 1200
#endif

#ifndef Y_DPI
#  define Y_DPI 600
#endif

typedef enum {
    ECONO_OFF = 0,
    ECONO_25,
    ECONO_50
} hl1250_econo_t;

typedef enum {
    PTYPE_PLAIN = 0,
    PTYPE_THIN,
    PTYPE_THICK,
    PTYPE_THICK2,
    PTYPE_TRANS
} hl1250_paper_t;

typedef enum {
    TRAY_AUTO = 0,
    TRAY_DUAL,
    TRAY_UPPER,
    TRAY_LOWER,
    TRAY_MANUAL
} hl1250_tray_t;

typedef struct gx_device_hl1250_s {
    gx_device_common;
    gx_prn_device_common;
    hl1250_econo_t econo_mode;
    hl1250_paper_t paper_type;
    hl1250_tray_t source_tray;
} gx_device_hl1250;

static dev_proc_open_device(hl1250_open);
static dev_proc_close_device(hl1250_close);
static dev_proc_print_page_copies(hl1250_print_page_copies);
static dev_proc_get_params(hl1250_get_params);
static dev_proc_put_params(hl1250_put_params);

static const gx_device_procs prn_hl1250_procs =
prn_params_procs(hl1250_open, gdev_prn_output_page, hl1250_close,
		 hl1250_get_params, hl1250_put_params);

#define hl1250_device_copies(dname, xdpi, ydpi)\
{   prn_device_std_body_copies(gx_device_hl1250, prn_hl1250_procs,\
			       dname,\
			       DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,\
			       xdpi, ydpi,\
			       0, 0, 0, 0,\
			       1, hl1250_print_page_copies),\
    ECONO_OFF, PTYPE_PLAIN, TRAY_AUTO\
}

const gx_device_hl1250 gs_hl1250_device =
hl1250_device_copies("hl1250", X_DPI2, Y_DPI);

const gx_device_hl1250 gs_hl1240_device =
hl1250_device_copies("hl1240", X_DPI, Y_DPI);

/* defines for the 1200x600 dpi mode (mode 1027) */

/* max number of 16-bit words in one 1200 dpi scan line */
#define SCAN_WORDS 640

/* number of scan lines per band */
#define BAND_SCANS 64

/* max number of 16-bit words in one 64-line band after compression,
   limited by 16-bit length field - 7 bytes header + 0xFFF8 bytes data -
   note that we may have to lose some data if it can't compress well
   (maximum possible uncompressed data length is about 80K).  */
#define MAX_BAND_WORDS 0x7FFC

/* codes (16-bit words) to send in the 1200x600 dpi data block: */

/* vertical compression (copy COUNT words from the previous line) */
#define HL_VERT(count) (0xE000 | (count))

/* 16-bit horizontal compression (repeat the next word COUNT times) */
#define HL_HOR16(count) (0x8000 | (count))

/* 8-bit horiz. compression (repeat DATA byte COUNT*2 times, COUNT < 32) */
#define HL_HOR8(count, data) (0xC000 | ((count) << 8) | ((data) & 0xFF))

/* 4-bit horiz. compression (repeat DATA nibble COUNT*4 times, COUNT < 512) */
#define HL_HOR4(count, data) (0xA000 | (count) | (((data) & 0x0F) << 9))

/* no compression, COUNT words of data follow */
#define HL_COPY(count) ((count) << 4)

typedef unsigned short u16;

/* The printer expects 16-bit data words in big endian order.  */
#if arch_is_big_endian
#define cpu_to_be16(x) (x)
#else
static u16
cpu_to_be16(u16 x)
{
    return (x >> 8) | (x << 8);
}
#endif

typedef struct hl1250_state_s {
    /* smallest area that contains all black dots of the band */
    /* x = xl..xr-1, y = yt..yb-1, width=xr-xl, height=yb-yt */
    unsigned int xl, xr, yt, yb;

    /* number of words in the output buffer */
    unsigned int out_count;

    /* output buffer for compressed data */
    u16 out_buf[MAX_BAND_WORDS];

    /* input buffer for one 64-line band */
    u16 in_buf[BAND_SCANS][SCAN_WORDS];
} hl1250_state_t;

/*
   find the smallest rectangle that contains all black dots of the
   band (in other words, the outside of that rectangle is all white)
   returns 0 if the band is completely white, else 1
   XXX - it should be faster to scan 32-bit (instead of 16-bit) words
 */
static int
hl1250_check_area(hl1250_state_t * s)
{
    unsigned int x, y, xleft, xright, ytop, ybottom;
    int line_not_white, band_not_white;

    band_not_white = 0;
    xright = s->xl;
    xleft = s->xr;
    ybottom = s->yt;
    ytop = s->yb;
    for (y = s->yt; y < s->yb; y++) {
	line_not_white = 0;
	for (x = s->xl; x < s->xr; x++) {
	    if (s->in_buf[y][x]) {
		if (xleft > x)
		    xleft = x;
		if (xright <= x)
		    xright = x + 1;
		line_not_white = 1;
	    }
	}
	if (line_not_white) {
	    if (ytop > y)
		ytop = y;
	    if (ybottom <= y)
		ybottom = y + 1;
	    band_not_white = 1;
	}
    }
    if (band_not_white) {
	s->xl = xleft;
	s->xr = xright;
	s->yt = ytop;
	s->yb = ybottom;
    } else {
	s->xr = s->xl;
	s->yb = s->yt;
    }
    return band_not_white;
}

/*
   Compressed data of this band is larger than MAX_BAND_WORDS.
   Repeat each line using vertical compression, effectively reducing
   resolution to 1200x300 dpi.  This shouldn't happen too often.
 */
static void
hl1250_lose_resolution(hl1250_state_t * s)
{
    unsigned int x, y;
    u16 data0, data1, mask;
    u16 *out;
    const u16 *line0;
    const u16 *line1;
    unsigned int width = s->xr - s->xl;

    out = s->out_buf;
    for (y = s->yt; y < s->yb; y += 2) {
	line0 = &s->in_buf[y][s->xl];
	line1 = line0;
	if (y + 1 < s->yb)
	    line1 = &s->in_buf[y + 1][s->xl];
	mask = (y & 2) ? 0x5555 : 0xAAAA;
	*out++ = cpu_to_be16(HL_COPY(width));
	for (x = 0; x < width; x++) {
	    data0 = line0[x];
	    data1 = line1[x];
	    *out++ = (data0 & data1) | (data0 & mask) | (data1 & mask);
	}
	*out++ = cpu_to_be16(HL_VERT(width));
    }
    s->out_count = out - s->out_buf;
}

/*
   Try to compress a single scan line.
   Return 1 on success, 0 on failure (data doesn't compress
   well enough to fit in MAX_BAND_WORDS words, truncated).
 */
static int
hl1250_compress_line(hl1250_state_t * s, unsigned int y)
{
    unsigned int total_len;	/* total data words after compression */
    unsigned int copy_len;	/* number of words to copy uncompressed */
    unsigned int hlen, vlen;	/* length for hor. and vert. compression */
    unsigned int clen, ulen;	/* compressed and uncompressed words */
    u16 data;			/* word currently being processed */
    u16 hl_code;		/* code to send */
    int x, i;
    u16 *out;
    unsigned int width = s->xr - s->xl;
    const u16 *line = &s->in_buf[y][s->xl];
    const u16 *prev = (y > s->yt ? &s->in_buf[y - 1][s->xl] : NULL);

    out = &s->out_buf[s->out_count];
    x = 0;
    copy_len = 0;
    total_len = s->out_count;
    while (x < width) {
	data = line[x];

	/* how many words (hlen >= 1) to repeat */
	for (i = x + 1; i < width; i++) {
	    if (line[i] != data)
		break;
	}
	hlen = i - x;
	/* if not the first line, try vertical compression */
	vlen = 0;
	if (prev) {
	    for (i = x; i < width; i++) {
		if (line[i] != prev[i])
		    break;
	    }
	    vlen = i - x;
	}
	/* find the best method to encode the data */
	ulen = hlen;
	clen = 1;
	if (vlen >= hlen) {
	    /* vertical compression */
	    ulen = vlen;
	    hl_code = HL_VERT(vlen);
	} else if ((data ^ (data >> 8)) & 0xFF) {
	    /* 16-bit horizontal compression */
	    hl_code = HL_HOR16(hlen);
	    clen = 2;
	} else if ((data ^ (data >> 4)) & 0x0F) {
	    /* 8-bit horizontal compression (if possible) */
	    if (hlen < 32) {
		hl_code = HL_HOR8(hlen, data);
	    } else {
		hl_code = HL_HOR16(hlen);
		clen = 2;
	    }
	} else {
	    /* 4-bit horizontal compression (if possible) */
	    if (hlen < 512) {
		hl_code = HL_HOR4(hlen, data);
	    } else {
		hl_code = HL_HOR16(hlen);
		clen = 2;
	    }
	}
	/*
	   now see if we should copy the word without compression
	   (the cost of leaving/re-entering copy mode is one word).
	 */
	if (ulen < clen + (copy_len > 0)) {
	    /* yes, copy */
	    copy_len += ulen;
	} else {
	    if (copy_len) {
		/* first, copy data (if any) */
		total_len += copy_len + 1;
		if (total_len > MAX_BAND_WORDS)
		    goto error_out;
		*out++ = cpu_to_be16(HL_COPY(copy_len));
		memcpy(out, &line[x - copy_len], copy_len * sizeof(u16));
		out += copy_len;
		copy_len = 0;
	    }
	    /* now send the current code */
	    total_len += clen;
	    if (total_len > MAX_BAND_WORDS)
		goto error_out;
	    *out++ = cpu_to_be16(hl_code);
	    if (clen == 2) {
		/* 16-bit compression */
		*out++ = data;
	    }
	}
	/* move to the next word to process */
	x += ulen;
    }
    /* copy anything left at the end */
    if (copy_len) {
	total_len += copy_len + 1;
	if (total_len > MAX_BAND_WORDS)
	    goto error_out;
	*out++ = cpu_to_be16(HL_COPY(copy_len));
	memcpy(out, &line[width - copy_len], copy_len * sizeof(u16));
	out += copy_len;
    }
  error_out:
    s->out_count = out - s->out_buf;
    return (total_len <= MAX_BAND_WORDS);
}

/*
   send a 16-bit big endian value
 */
static void
put_be16(FILE * fp, u16 data)
{
    putc(data >> 8, fp);
    putc(data & 0xFF, fp);
}

/*
   prn_stream = data sent to the printer
   s->xl,xr,yt,yb,in_buf = band (max 64 lines), coordinates band-relative
   band = Y coordinate of the band from top of page
 */
static void
hl1250_compress_band(FILE * prn_stream, hl1250_state_t * s, unsigned int band)
{
    unsigned int y, ytop, ybottom;

    s->out_count = 0;

    if (!hl1250_check_area(s))
	return;

    /*
       It seems HL-1250 1200x600 dpi mode has an undocumented feature -
       it rounds the X left coordinate of the image up to next 32 dots.
       Probably the RISC inside likes 32-bit alignment.  No big problem,
       we just have to round it down (it's in 16-bit words here).
     */
    s->xl &= ~1;

    ytop = s->yt;
    ybottom = s->yb;
    for (y = ytop; y < ybottom; y++) {
	if (hl1250_compress_line(s, y))
	    continue;
#if 1
	/* XXX - please report if you ever see this, then change to #if 0 */
	dprintf1("hl1250: compression failed (y=%u), doing 1200x300 dpi\n",
		band + y);
#endif
	hl1250_lose_resolution(s);
	break;
    }
    if (s->out_count) {
	fprintf(prn_stream, "\033*b%uW", s->out_count * sizeof(u16) + 9);
	put_be16(prn_stream, s->out_count * sizeof(u16) + 7);
	put_be16(prn_stream, s->xl * 16);
	put_be16(prn_stream, band + ytop);
	putc(ybottom - ytop, prn_stream);
	put_be16(prn_stream, s->xr - s->xl);
	fwrite(s->out_buf, sizeof(u16), s->out_count, prn_stream);
    }
}


static int
hl1250_print_1200dpi(gx_device_printer * pdev, FILE * prn_stream,
		     int num_copies, const char *page_init)
{
    int band, lnum;
    int code = 0;
    hl1250_state_t *s;
    int line_size = gdev_prn_raster(pdev);
    int num_rows = dev_print_scan_lines(pdev);
    int paper_size = gdev_pcl_paper_size((gx_device *) pdev);

    s = (hl1250_state_t *) gs_alloc_byte_array(pdev->memory, 1, sizeof(*s),
					       "hl1250_print_1200dpi");
    if (!s)
	return_error(gs_error_VMerror);

    memset(s, 0, sizeof(*s));

    if (line_size > sizeof(s->in_buf[0]))
	line_size = sizeof(s->in_buf[0]);

    if (pdev->PageCount == 0) {
	/* reset, set paper size */
	fprintf(prn_stream, "\033E\033&l%dA", paper_size);
    }
    fputs("\033&u1200D", prn_stream);
    fputs("\033&l0o0l0E", prn_stream);
    fputs(page_init, prn_stream);
    fprintf(prn_stream, "\033&l%dX", num_copies);
    fputs("\033*rB\033*p0x0Y", prn_stream);

    /* set 600dpi (vertical) resolution */
    fputs("\033*t600R", prn_stream);

    /* start raster graphics */
    fputs("\033*r1A", prn_stream);

    /* transfer data in the special 1200x600 dpi format */
    fputs("\033*b1027M", prn_stream);

    for (band = 0; band < num_rows; band += BAND_SCANS) {
	int next_band = band + BAND_SCANS;

	if (next_band > num_rows)
	    next_band = num_rows;
	for (lnum = band; lnum < next_band; lnum++) {
	    code = gdev_prn_copy_scan_lines(pdev, lnum,
			      (byte *) (s->in_buf[lnum - band]), line_size);
	    if (code < 0)
		goto error_out;
	}
	s->yt = 0;
	s->yb = next_band - band;
	s->xl = 0;
	s->xr = 635;		/* max width */
	hl1250_compress_band(prn_stream, s, band);
    }
  error_out:
    fputs("\033*rB\f", prn_stream);
    gs_free_object(pdev->memory, s, "hl1250_print_1200dpi");
    return code;
}


static int
hl1250_open(gx_device * pdev)
{
    /* margins: left, bottom, right, top */
    /* XXX - only A4 tested, letter size margins likely need to be tweaked */
    static const float margins_a4_600[4] = {0.05, 0.15, 0.25, 0.15};
    static const float margins_a4_1200[4] = {0.05, 0.2, 0.1, 0.15};
    static const float margins_letter_600[4] = {0.05, 0.15, 0.25, 0.15};
    static const float margins_letter_1200[4] = {0.05, 0.2, 0.1, 0.15};

    int x_dpi = (int)((gx_device_printer *) pdev)->x_pixels_per_inch;
    int paper_size = gdev_pcl_paper_size(pdev);

    if (x_dpi == 1200) {
	if (paper_size == PAPER_SIZE_A4)
	    gx_device_set_margins(pdev, margins_a4_1200, true);
	else
	    gx_device_set_margins(pdev, margins_letter_1200, true);
    } else {
	if (paper_size == PAPER_SIZE_A4)
	    gx_device_set_margins(pdev, margins_a4_600, false);
	else
	    gx_device_set_margins(pdev, margins_letter_600, false);
    }
    return gdev_prn_open(pdev);
}

#define UEL "\033%-12345X"

/* XXX - how to get the name of the job (PS file) being printed? */
#define PJL_JOB_NAME "Ghost"

static int
hl1250_close(gx_device * pdev)
{
    int code = gdev_prn_open_printer(pdev, 1);
    FILE *fp;

    if (code < 0)
	return code;
    fp = ((gx_device_printer *) pdev)->file;
    /* job separation, reset, UEL */
    fputs("\033&l1T\033E", fp);
    fputs(UEL, fp);
    fprintf(fp, "@PJL EOJ NAME=\"%s\"\r\n", PJL_JOB_NAME);
    fputs(UEL, fp);
    return gdev_prn_close(pdev);
}


static int
hl1250_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			 int num_copies)
{
    int x_dpi = (int)pdev->x_pixels_per_inch;
    int y_dpi = (int)pdev->y_pixels_per_inch;
    const char *tray_pjl;
    const char *tray_pcl;
    const gx_device_hl1250 *hl1250 = (gx_device_hl1250 *) pdev;
    hl1250_econo_t econo_mode = hl1250->econo_mode;
    hl1250_paper_t paper_type = hl1250->paper_type;
    hl1250_tray_t source_tray = hl1250->source_tray;
    static const char * const onoff[2] = { "OFF", "ON" };
    static const char * const papertypes[5] =
	{ "REGULAR", "THIN", "THICK", "THICK2", "TRANSPARENCY" };

    tray_pjl = "AUTO";
    tray_pcl = "";
    switch (source_tray) {
	case TRAY_DUAL:
	    tray_pcl = "\033&l0H";
	    break;
	case TRAY_UPPER:
	    tray_pjl = "TRAY1";
	    tray_pcl = "\033&l1H";
	    break;
	case TRAY_LOWER:
	    tray_pjl = "TRAY2";
	    tray_pcl = "\033&l5H";
	    break;
	case TRAY_MANUAL:
	    tray_pcl = "\033&l2H";
	    break;
	default:
	    break;
    }
    if (pdev->PageCount == 0) {
	/* initialize printer */
	fputs(UEL, prn_stream);
	fputs("@PJL \r\n", prn_stream);
	fprintf(prn_stream, "@PJL JOB NAME=\"%s\"\r\n", PJL_JOB_NAME);
	fprintf(prn_stream, "@PJL SET ECONOMODE=%s\n", onoff[econo_mode != 0]);
	if (econo_mode)
	    fprintf(prn_stream, "@PJL SET ECONOLEVEL=%d\n", 3 - (int)econo_mode);
	fprintf(prn_stream, "@PJL SET MEDIATYPE=%s\n", papertypes[paper_type]);
	fprintf(prn_stream, "@PJL SET SOURCETRAY=%s\n", tray_pjl);
	fprintf(prn_stream, "@PJL SET RESOLUTION=%d\n", y_dpi);
	fprintf(prn_stream, "@PJL SET RAS1200MODE=%s\n", onoff[x_dpi == 1200]);

	fputs("@PJL ENTER LANGUAGE=PCL\n", prn_stream);
    }
    if (x_dpi != 1200) {
	char page_init[100];

	/* We get the printer to translate the coordinate system for us.
	   It seems the HL-1250 needs a slightly different translation
	   (-120, 0) compared to the one in the ljet4 driver (-180, 36)
	   (X, Y coordinates here are specified in 1/720-inch units).  */

	sprintf(page_init, "\033&l-120U\033*r0F\033&u%dD%s", y_dpi, tray_pcl);
	return dljet_mono_print_page_copies(pdev, prn_stream, num_copies,
					    y_dpi, PCL_LJ4_FEATURES, 
                                            page_init, page_init, false);
    }
    return hl1250_print_1200dpi(pdev, prn_stream, num_copies, tray_pcl);
}

static int
hl1250_put_param_int(gs_param_list * plist, gs_param_name pname,
		     int *pvalue, int minval, int maxval, int ecode)
{
    int code, value;

    code = param_read_int(plist, pname, &value);
    switch (code) {
	case 0:
	    if (value < minval || value > maxval)
		param_signal_error(plist, pname, gs_error_rangecheck);
	    *pvalue = value;
	    return (ecode < 0 ? ecode : 1);
	case 1:
	    return ecode;
	default:
	    return code;
    }
}

static int
hl1250_get_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_hl1250 *hl1250 = (gx_device_hl1250 *) pdev;
    int code, val;

    code = gdev_prn_get_params(pdev, plist);
    if (code < 0)
	return code;
    val = hl1250->econo_mode;
    code = param_write_int(plist, "EconoMode", &val);
    if (code < 0)
	return code;
    val = hl1250->paper_type;
    code = param_write_int(plist, "PaperType", &val);
    if (code < 0)
	return code;
    val = hl1250->source_tray;
    code = param_write_int(plist, "SourceTray", &val);
    return code;
}

static int
hl1250_put_params(gx_device * pdev, gs_param_list * plist)
{
    int code = 0;
    gx_device_hl1250 *hl1250 = (gx_device_hl1250 *) pdev;
    int econo_mode = hl1250->econo_mode;
    int paper_type = hl1250->paper_type;
    int source_tray = hl1250->source_tray;

    code = hl1250_put_param_int(plist, "EconoMode", &econo_mode,
				ECONO_OFF, ECONO_50, code);
    code = hl1250_put_param_int(plist, "PaperType", &paper_type,
				PTYPE_PLAIN, PTYPE_TRANS, code);
    code = hl1250_put_param_int(plist, "SourceTray", &source_tray,
				TRAY_AUTO, TRAY_MANUAL, code);
    if (code < 0)
	return code;
    code = gdev_prn_put_params(pdev, plist);
    if (code < 0)
	return code;
    hl1250->econo_mode = econo_mode;
    hl1250->paper_type = paper_type;
    hl1250->source_tray = source_tray;
    return 0;
}
