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

/* $Id: write_t1.c,v 1.7 2007/09/11 15:24:41 Arabidopsis Exp $ */

/*
Functions to serialize a type 1 font as PostScript code that can then be
passed to FreeType via the FAPI FreeType bridge.
Started by Graham Asher, 26th July 2002.
*/

#include "wrfont.h"
#include "write_t1.h"

/*
Public structures and functions in this file are prefixed with FF_ because they are part of
the FAPI FreeType implementation.
*/

static void write_word_entry(FAPI_font* a_fapi_font,WRF_output* a_output,const char* a_name,int a_index,int a_divisor)
	{
	short x;
	WRF_wbyte(a_output,'/');
	WRF_wstring(a_output,a_name);
	WRF_wbyte(a_output,' ');
	/* Get the value and convert it from unsigned to signed by assigning it to a short. */
	x = a_fapi_font->get_word(a_fapi_font,a_index,0);
	/* Divide by the divisor to bring it back to font units. */
	x = (short)(x / a_divisor);
	WRF_wint(a_output,x);
	WRF_wstring(a_output," def\n");
	}

static void write_array_entry_with_count(FAPI_font* a_fapi_font,WRF_output* a_output,const char* a_name,int a_index,int a_count,int a_divisor)
	{
	int i;

	if (a_count <= 0)
		return;

	WRF_wbyte(a_output,'/');
	WRF_wstring(a_output,a_name);
	WRF_wstring(a_output," [");
	for (i = 0; i < a_count; i++)
		{
		/* Get the value and convert it from unsigned to signed by assigning it to a short. */
		short x = a_fapi_font->get_word(a_fapi_font,a_index,i);
		/* Divide by the divisor to bring it back to font units. */
		x = (short)(x / a_divisor);
		WRF_wint(a_output,x);
		WRF_wbyte(a_output,(byte)(i == a_count - 1 ? ']' : ' '));
		}
	WRF_wstring(a_output," def\n");
	}


static void write_array_entry(FAPI_font* a_fapi_font,WRF_output* a_output,const char* a_name,int a_index,int a_divisor)
	{
	/* NOTE that the feature index must be preceded by the count index for this to work. */
	int count = a_fapi_font->get_word(a_fapi_font,a_index - 1,0);
	write_array_entry_with_count(a_fapi_font,a_output,a_name,a_index,count,a_divisor);
	}

static void write_subrs(FAPI_font* a_fapi_font,WRF_output* a_output)
	{
	int i;
	int count = a_fapi_font->get_word(a_fapi_font,FAPI_FONT_FEATURE_Subrs_count,0);
	if (count <= 0)
		return;

	WRF_wstring(a_output,"/Subrs ");
	WRF_wint(a_output,count);
	WRF_wstring(a_output," array\n");

	for (i = 0; i < count; i++)
		{
		long length = a_fapi_font->get_subr(a_fapi_font,i,0,0);
		long buffer_size;
		WRF_wstring(a_output,"dup ");
		WRF_wint(a_output,i);
		WRF_wbyte(a_output,' ');
		WRF_wint(a_output,length);
		WRF_wstring(a_output," RD ");
		
		/* Get the subroutine into the buffer and encrypt it in place. */
		buffer_size = a_output->m_limit - a_output->m_count;
		if (buffer_size >= length)
			{
			a_fapi_font->get_subr(a_fapi_font,i,a_output->m_pos,(ushort)length);
			WRF_wtext(a_output,a_output->m_pos,length);
			}
		else
			a_output->m_count += length;

		WRF_wstring(a_output," NP\n");
		}

	WRF_wstring(a_output,"ND\n");
	}

static void write_private_dictionary(FAPI_font* a_fapi_font,WRF_output* a_output)
	{
	a_output->m_encrypt = true;

	/* Write 4 bytes that must encrypt to at least one character that cannot be a valid hexadecimal character. */
	WRF_wstring(a_output,"XXXX");

	/*+ to do: correct size of dictionary from 8. */	
	WRF_wstring(a_output,"dup /Private 8 dict dup begin\n");

	WRF_wstring(a_output,"/MinFeature {16 16} def\n");
	WRF_wstring(a_output,"/password 5839 def\n");
	WRF_wstring(a_output,"/lenIV -1 def\n"); /* indicate that /subrs are not encoded. */
	write_word_entry(a_fapi_font,a_output,"BlueFuzz",FAPI_FONT_FEATURE_BlueFuzz,16);

	WRF_wstring(a_output,"/BlueScale ");
	WRF_wfloat(a_output,a_fapi_font->get_long(a_fapi_font,FAPI_FONT_FEATURE_BlueScale,0) / 65536.0);
	WRF_wstring(a_output," def\n");

	write_word_entry(a_fapi_font,a_output,"BlueShift",FAPI_FONT_FEATURE_BlueShift,16);
	write_array_entry(a_fapi_font,a_output,"BlueValues",FAPI_FONT_FEATURE_BlueValues,16);
	write_array_entry(a_fapi_font,a_output,"OtherBlues",FAPI_FONT_FEATURE_OtherBlues,16);
	write_array_entry(a_fapi_font,a_output,"FamilyBlues",FAPI_FONT_FEATURE_FamilyBlues,16);
	write_array_entry(a_fapi_font,a_output,"FamilyOtherBlues",FAPI_FONT_FEATURE_FamilyOtherBlues,16);
	write_word_entry(a_fapi_font,a_output,"ForceBold",FAPI_FONT_FEATURE_ForceBold,1);
	write_array_entry_with_count(a_fapi_font,a_output,"StdHW",FAPI_FONT_FEATURE_StdHW,1,16);
	write_array_entry_with_count(a_fapi_font,a_output,"StdVW",FAPI_FONT_FEATURE_StdVW,1,16);
	write_array_entry(a_fapi_font,a_output,"StemSnapH",FAPI_FONT_FEATURE_StemSnapH,16);
	write_array_entry(a_fapi_font,a_output,"StemSnapV",FAPI_FONT_FEATURE_StemSnapV,16);

	write_subrs(a_fapi_font,a_output);
	}

static void write_main_dictionary(FAPI_font* a_fapi_font,WRF_output* a_output)
	{
	int i;
	WRF_wstring(a_output,"5 dict begin\n");

    WRF_wstring(a_output,"/FontType 1 def\n");

	WRF_wstring(a_output,"/FontMatrix [");
	for (i = 0; i < 6; i++)
		{
		WRF_wfloat(a_output,a_fapi_font->get_float(a_fapi_font,FAPI_FONT_FEATURE_FontMatrix,i));
		WRF_wbyte(a_output,(byte)(i == 5 ? ']' : ' '));
		}
	WRF_wbyte(a_output,'\n');

	/* For now, specify standard encoding - I think GS will pass glyph indices so doesn't matter. */
	WRF_wstring(a_output,"/Encoding StandardEncoding def\n");

	WRF_wstring(a_output,"/FontBBox {");
	for (i = 0; i < 4; i++)
		{
		short x = a_fapi_font->get_word(a_fapi_font,FAPI_FONT_FEATURE_FontBBox,i);
		WRF_wint(a_output,x);
		WRF_wbyte(a_output,(byte)(i == 3 ? '}' : ' '));
		}
	WRF_wbyte(a_output,'\n');
	WRF_wstring(a_output,"currentdict end\ncurrentfile eexec\n");
	write_private_dictionary(a_fapi_font,a_output);
	}
	
/**
Write a Type 1 font in textual format and return its length in bytes.
If a_buffer_size is less than the total length, only a_buffer_size bytes are written, but the total
length is returned correctly.

The PostScript is non-standard. The main dictionary contains no /Charstrings dictionary. This
is supplied to FreeType using the incremental interface, There is also no /PaintType entry. This is required
by PostScript but FreeType doesn't use it.
*/
long FF_serialize_type1_font(FAPI_font* a_fapi_font,unsigned char* a_buffer,long a_buffer_size)
	{
	WRF_output output;
	WRF_init(&output,a_buffer,a_buffer_size);
	
	/* Leading comment identifying a Type 1 font. */
	WRF_wstring(&output,"%!PS-AdobeFont-1\n");

	write_main_dictionary(a_fapi_font,&output);
	return output.m_count;
	}
