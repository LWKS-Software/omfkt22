/***********************************************************************
 *
 *              Copyright (c) 1996 Avid Technology, Inc.
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
/* compress "AVR5 capture" 	640x240x32		Q=15	41k/f	res#252
 */
/* Size = 640 x 240 x 32; Q Factor = 15; 41984 Bytes/frame (target);
   Resource #252 */


#define AVR5_NAME "AVR5"

#define AVR5_ID 31


{
  "AVR5", /* avr string */
  31, /* TableID */
  15, /* Qfactor */
  640, /* HSize */
  240, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  8, /* Leading Lines (NTSC) */
  8, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  41984, /* Target Frame Size */

  { /* 8bit Quantization tables */
    5, 3, 4, 4, 4, 3, 5, 4, 
    4, 4, 5, 5, 5, 6, 7, 12, 
    8, 7, 7, 7, 7, 15, 11, 11, 
    9, 12, 17, 15, 18, 18, 17, 15, 
    17, 17, 19, 22, 28, 23, 19, 20, 
    26, 21, 17, 17, 24, 33, 24, 26, 
    29, 29, 31, 31, 31, 19, 23, 34, 
    36, 34, 30, 36, 28, 30, 31, 30, 
    
    5, 5, 5, 7, 6, 7, 14, 8, 
    8, 14, 30, 20, 17, 20, 30, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    
    5, 5, 5, 7, 6, 7, 14, 8, 
    8, 14, 30, 20, 17, 20, 30, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77
  },

  { /* 16bit Quantization Tables */
    0x04cc, 0x034c, 0x0399, 0x0433, 0x0399, 0x0300, 0x04cc, 0x0433, 
    0x03e6, 0x0433, 0x0566, 0x0519, 0x04cc, 0x05b3, 0x0733, 0x0c00, 
    0x07cc, 0x0733, 0x0699, 0x0699, 0x0733, 0x0eb3, 0x0a80, 0x0b19, 
    0x08b3, 0x0c00, 0x1166, 0x0f4c, 0x124c, 0x1200, 0x1119, 0x0f4c, 
    0x10cc, 0x1080, 0x1333, 0x1599, 0x1b99, 0x1766, 0x1333, 0x1466, 
    0x1a19, 0x14b3, 0x1080, 0x10cc, 0x1800, 0x20b3, 0x184c, 0x1a19, 
    0x1c80, 0x1d66, 0x1ee6, 0x1f33, 0x1ee6, 0x1299, 0x1719, 0x21e6, 
    0x244c, 0x2199, 0x1e00, 0x2400, 0x1b99, 0x1e4c, 0x1ee6, 0x1db3, 
    
    0x0519, 0x0566, 0x0566, 0x0733, 0x064c, 0x0733, 0x0e19, 0x07cc, 
    0x07cc, 0x0e19, 0x1db3, 0x13cc, 0x10cc, 0x13cc, 0x1db3, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 
    
    0x0519, 0x0566, 0x0566, 0x0733, 0x064c, 0x0733, 0x0e19, 0x07cc, 
    0x07cc, 0x0e19, 0x1db3, 0x13cc, 0x10cc, 0x13cc, 0x1db3, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 
    0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80, 0x4c80

  }

}, 
