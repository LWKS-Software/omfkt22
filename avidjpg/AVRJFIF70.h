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

#define AVRJFIF10_1_NAME "AVRJFIF10-1"

#define AVRJFIF10_1_ID 75


{
  "JFIF10:1", /* avr string */
  75, /* TableID */
  (int )(1.2*JFIF_QFACTOR_MULTIPLIER), /* Qfactor */
  720, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  5, /* Leading Lines (NTSC) */
  8, /* Leading Lines (PAL) */
  2, /* Number of Fields */
  84L*1024L, /* Target Frame Size */

  { /* 8bit Quantization tables */ 

	0x05, 0x04, 0x04, 0x04, 0x04, 0x03, 0x05, 0x04,
	0x04, 0x04, 0x06, 0x06, 0x05, 0x06, 0x08, 0x0D,
	0x09, 0x08, 0x07, 0x07, 0x08, 0x10, 0x0B, 0x0C,
	0x09, 0x0D, 0x13, 0x11, 0x14, 0x14, 0x13, 0x11,
	0x12, 0x12, 0x15, 0x17, 0x1E, 0x1A, 0x15, 0x16,
	0x1C, 0x17, 0x12, 0x12, 0x1B, 0x24, 0x1B, 0x1C,
	0x1F, 0x20, 0x22, 0x22, 0x22, 0x14, 0x19, 0x25,
	0x28, 0x25, 0x21, 0x27, 0x1E, 0x22, 0x22, 0x20,
	
	0x01, 0x06, 0x06, 0x06, 0x08, 0x07, 0x08, 0x0F,
	0x09, 0x09, 0x0F, 0x20, 0x16, 0x12, 0x16, 0x20,
	0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
	0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
	0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
	0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
	0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
	0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54,
	
	0x54, 0xFF, 0xC4, 0x01, 0xA2, 0x00, 0x00, 0x01,
	0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x0A, 0x0B, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
	0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D,
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07	
  },

  { /* 16bit Quantization Tables */
	0x0500, 0x0400, 0x0400, 0x0400, 0x0400, 0x0300, 0x0500, 0x0400,
	0x0400, 0x0400, 0x0600, 0x0600, 0x0500, 0x0600, 0x0800, 0x0D00,
	0x0900, 0x0800, 0x0700, 0x0700, 0x0800, 0x1000, 0x0B00, 0x0C00,
	0x0900, 0x0D00, 0x1300, 0x1100, 0x1400, 0x1400, 0x1300, 0x1100,
	0x1200, 0x1200, 0x1500, 0x1700, 0x1E00, 0x1A00, 0x1500, 0x1600,
	0x1C00, 0x1700, 0x1200, 0x1200, 0x1B00, 0x2400, 0x1B00, 0x1C00,
	0x1F00, 0x2000, 0x2200, 0x2200, 0x2200, 0x1400, 0x1900, 0x2500,
	0x2800, 0x2500, 0x2100, 0x2700, 0x1E00, 0x2200, 0x2200, 0x2000,
	
	0x0100, 0x0600, 0x0600, 0x0600, 0x0800, 0x0700, 0x0800, 0x0F00,
	0x0900, 0x0900, 0x0F00, 0x2000, 0x1600, 0x1200, 0x1600, 0x2000,
	0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400,
	0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400,
	0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400,
	0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400,
	0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400,
	0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400,
	
	0x5400, 0xFF00, 0xC400, 0x0100, 0xA200, 0x0000, 0x0000, 0x0100,
	0x0500, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100,
	0x0200, 0x0300, 0x0400, 0x0500, 0x0600, 0x0700, 0x0800, 0x0900,
	0x0A00, 0x0B00, 0x0100, 0x0300, 0x0300, 0x0200, 0x0400, 0x0300,
	0x0500, 0x0500, 0x0400, 0x0400, 0x0000, 0x0000, 0x0100, 0x7D00,
	0x0100, 0x0200, 0x0300, 0x0000, 0x0400, 0x1100, 0x0500, 0x1200,
	0x2100, 0x3100, 0x4100, 0x0600, 0x1300, 0x5100, 0x6100, 0x0700	
  }

},
