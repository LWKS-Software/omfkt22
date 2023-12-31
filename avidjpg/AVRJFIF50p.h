/***********************************************************************
 *
 *              Copyright (c) 1999 Avid Technology, Inc.
 *
 * Permission to use, copy and modify this software and to distribute
 * and sublicense application software incorporating this software for
 * any purpose is hereby granted, provided that (i) the above
 * copyright notice and this permission notice appear in all copies of
 * the software and related documentation, and (ii) the name Avid
 * Technology, Inc. may not be used in any advertising or publicity
 * relating to the software without the specific, prior written
 * permission of Avid Technology, Inc.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL AVID TECHNOLOGY, INC. BE LIABLE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT, CONSEQUENTIAL OR OTHER DAMAGES OF
 * ANY KIND, OR ANY DAMAGES WHATSOEVER ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE, INCLUDING, 
 * WITHOUT  LIMITATION, DAMAGES RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, AND WHETHER OR NOT ADVISED OF THE POSSIBILITY OF
 * DAMAGE, REGARDLESS OF THE THEORY OF LIABILITY.
 *
 ************************************************************************/

#define AVRJFIF50p_ID 103


{
  "JFIF14:1p",					/* avr string */
  AVRJFIF50p_ID,				/* TableID */
  (int )(1.2*JFIF_QFACTOR_MULTIPLIER),	/* Render Min Scale Factor used to multiply out tables below */
  720,							/* HSize */
  486,							/* YSize(NTSC) */
  576,							/* YSize(PAL) */
  10,							/* Leading Lines (NTSC) */
  16,							/* Leading Lines (PAL) */
  1,							/* Number of Fields */
  50L*1024L,					/* Target Frame Size */

  { /* 8bit Quantization tables */ 
	0x07, 0x05, 0x05, 0x06, 0x05, 0x04, 0x07, 0x06,
	0x06, 0x06, 0x08, 0x07, 0x07, 0x06, 0x0B, 0x12,
	0x0B, 0x0B, 0x0A, 0x0A, 0x0B, 0x16, 0x0F, 0x10,
	0x0D, 0x12, 0x1A, 0x16, 0x1B, 0x1A, 0x19, 0x16,
	0x19, 0x18, 0x1C, 0x20, 0x28, 0x22, 0x1C, 0x1E,
	0x26, 0x1E, 0x18, 0x19, 0x23, 0x30, 0x24, 0x26,
	0x2A, 0x2B, 0x2D, 0x2E, 0x2D, 0x1B, 0x22, 0x32,
	0x35, 0x31, 0x2C, 0x35, 0x28, 0x2C, 0x2D, 0x2C,
	
	0x01, 0x07, 0x08, 0x08, 0x0B, 0x09, 0x0B, 0x15,
	0x0B, 0x0B, 0x15, 0x2C, 0x1D, 0x19, 0x1D, 0x2C,
	0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,
	0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,
	0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,
	0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,
	0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,
	0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,
	
	0x70, 0xFF, 0xC4, 0x01, 0xA2, 0x00, 0x00, 0x01,
	0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x0A, 0x0B, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
	0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x10
  },

  { /* 16bit Quantization Tables */
	0x0700, 0x0500, 0x0500, 0x0600, 0x0500, 0x0400, 0x0700, 0x0600,
	0x0600, 0x0600, 0x0800, 0x0700, 0x0700, 0x0600, 0x0B00, 0x1200,
	0x0B00, 0x0B00, 0x0A00, 0x0A00, 0x0B00, 0x1600, 0x0F00, 0x1000,
	0x0D00, 0x1200, 0x1A00, 0x1600, 0x1B00, 0x1A00, 0x1900, 0x1600,
	0x1900, 0x1800, 0x1C00, 0x2000, 0x2800, 0x2200, 0x1C00, 0x1E00,
	0x2600, 0x1E00, 0x1800, 0x1900, 0x2300, 0x3000, 0x2400, 0x2600,
	0x2A00, 0x2B00, 0x2D00, 0x2E00, 0x2D00, 0x1B00, 0x2200, 0x3200,
	0x3500, 0x3100, 0x2C00, 0x3500, 0x2800, 0x2C00, 0x2D00, 0x2C00,

	0x0100, 0x0700, 0x0800, 0x0800, 0x0B00, 0x0900, 0x0B00, 0x1500,
	0x0B00, 0x0B00, 0x1500, 0x2C00, 0x1D00, 0x1900, 0x1D00, 0x2C00,
	0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000,
	0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000,
	0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000,
	0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000,
	0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000,
	0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000,

	0x7000, 0xFF00, 0xC400, 0x0100, 0xA200, 0x0000, 0x0000, 0x0100,
	0x0500, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100,
	0x0200, 0x0300, 0x0400, 0x0500, 0x0600, 0x0700, 0x0800, 0x0900,
	0x0A00, 0x0B00, 0x0100, 0x0000, 0x0300, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0200, 0x0300, 0x0400,
	0x0500, 0x0600, 0x0700, 0x0800, 0x0900, 0x0A00, 0x0B00, 0x1000
  }

},
