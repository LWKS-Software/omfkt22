/***********************************************************************
 *
 *              Copyright (c) 1998 Avid Technology, Inc.
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

#define AVRJFIF2_1_NAME " AVRJFIF2_1"

#define  AVRJFIF2_1_ID 76


{
  "JFIF2:1", /* avr string */
  76, /* TableID */
  (int )(1.5*JFIF_QFACTOR_MULTIPLIER), /* Qfactor */
  720, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  5, /* Leading Lines (NTSC) */
  8, /* Leading Lines (PAL) */
  2, /* Number of Fields */
  300L*1024L, /* Target Frame Size */

  { /* 8bit Quantization tables */ 

0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
0x02, 0x02, 0x02, 0x02, 0x03, 0x04, 0x03, 0x02,
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
0x03, 0x06, 0x04, 0x04, 0x06, 0x03, 0x03, 0x02,
0x02, 0x02, 0x03, 0x03, 0x08, 0x05, 0x06, 0x09,
0x03, 0x02, 0x03, 0x0A, 0x07, 0x08, 0x08, 0x08,

0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x03,
0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x03, 0x08,
0x08, 0x04, 0x04, 0x04, 0x03, 0x04, 0x04, 0x04,
0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
0x08, 0x08, 0x08, 0x04, 0x08, 0x08, 0x08, 0x08,
0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,

0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x03,
0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x03, 0x08,
0x08, 0x04, 0x04, 0x04, 0x03, 0x04, 0x04, 0x04,
0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
0x08, 0x08, 0x08, 0x04, 0x08, 0x08, 0x08, 0x08,
0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
  },

  { /* 16bit Quantization Tables */
0x0100, 0x0100, 0x0100, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
0x0200, 0x0200, 0x0200, 0x0200, 0x0300, 0x0400, 0x0300, 0x0200,
0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
0x0300, 0x0600, 0x0400, 0x0400, 0x0600, 0x0300, 0x0300, 0x0200,
0x0200, 0x0200, 0x0300, 0x0300, 0x0800, 0x0500, 0x0600, 0x0900,
0x0300, 0x0200, 0x0300, 0x0A00, 0x0700, 0x0800, 0x0800, 0x0800,

0x0100, 0x0100, 0x0100, 0x0300, 0x0300, 0x0300, 0x0300, 0x0300,
0x0300, 0x0300, 0x0300, 0x0300, 0x0300, 0x0400, 0x0300, 0x0800,
0x0800, 0x0400, 0x0400, 0x0400, 0x0300, 0x0400, 0x0400, 0x0400,
0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800,
0x0800, 0x0800, 0x0800, 0x0400, 0x0800, 0x0800, 0x0800, 0x0800,
0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800,
0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800,
0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800,

0x0100, 0x0100, 0x0100, 0x0300, 0x0300, 0x0300, 0x0300, 0x0300,
0x0300, 0x0300, 0x0300, 0x0300, 0x0300, 0x0400, 0x0300, 0x0800,
0x0800, 0x0400, 0x0400, 0x0400, 0x0300, 0x0400, 0x0400, 0x0400,
0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800,
0x0800, 0x0800, 0x0800, 0x0400, 0x0800, 0x0800, 0x0800, 0x0800,
0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800,
0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800,
0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800
  }

},
