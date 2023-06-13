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
/* compress "AVR25 capture" 	640x240x32		Q=40	64k/f	res#262
 */
/* Size = 640 x 240 x 32; Q Factor = 40; 65536 Bytes/frame (target);
   Resource #262 */


#define AVR25_NAME "AVR25"

#define AVR25_ID 15


{
  "AVR25", /* avr string */
  15, /* TableID */
  40, /* Qfactor */
  640, /* HSize */
  240, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  8, /* Leading Lines (NTSC) */
  8, /* Leading Lines (PAL) */
  2, /* Number of Fields */
  65536, /* Target Frame Size */

  { /* 8bit Quantization tables */
    13, 9, 10, 11, 10, 8, 13, 11, 
    10, 11, 14, 14, 13, 15, 19, 32, 
    21, 19, 18, 18, 19, 39, 28, 30, 
    23, 32, 46, 41, 49, 48, 46, 41, 
    45, 44, 51, 58, 74, 62, 51, 54, 
    70, 55, 44, 45, 64, 87, 65, 70, 
    76, 78, 82, 83, 82, 50, 62, 90, 
    97, 90, 80, 96, 74, 81, 82, 79, 
    
    14, 14, 14, 19, 17, 19, 38, 21, 
    21, 38, 79, 53, 45, 53, 79, 204, 
    204, 204, 204, 204, 204, 204, 204, 204, 
    204, 204, 204, 204, 204, 204, 204, 204, 
    204, 204, 204, 204, 204, 204, 204, 204, 
    204, 204, 204, 204, 204, 204, 204, 204, 
    204, 204, 204, 204, 204, 204, 204, 204, 
    204, 204, 204, 204, 204, 204, 204, 204, 
    
    14, 14, 14, 19, 17, 19, 38, 21, 
    21, 38, 79, 53, 45, 53, 79, 204, 
    204, 204, 204, 204, 204, 204, 204, 204, 
    204, 204, 204, 204, 204, 204, 204, 204, 
    204, 204, 204, 204, 204, 204, 204, 204, 
    204, 204, 204, 204, 204, 204, 204, 204, 
    204, 204, 204, 204, 204, 204, 204, 204, 
    204, 204, 204, 204, 204, 204, 204, 204
  },

  { /* 16bit Quantization Tables */
    0x0ccc, 0x08cc, 0x0999, 0x0b33, 0x0999, 0x0800, 0x0ccc, 0x0b33, 
    0x0a66, 0x0b33, 0x0e66, 0x0d99, 0x0ccc, 0x0f33, 0x1333, 0x2000, 
    0x14cc, 0x1333, 0x1199, 0x1199, 0x1333, 0x2733, 0x1c00, 0x1d99, 
    0x1733, 0x2000, 0x2e66, 0x28cc, 0x30cc, 0x3000, 0x2d99, 0x28cc, 
    0x2ccc, 0x2c00, 0x3333, 0x3999, 0x4999, 0x3e66, 0x3333, 0x3666, 
    0x4599, 0x3733, 0x2c00, 0x2ccc, 0x4000, 0x5733, 0x40cc, 0x4599, 
    0x4c00, 0x4e66, 0x5266, 0x5333, 0x5266, 0x3199, 0x3d99, 0x5a66, 
    0x60cc, 0x5999, 0x5000, 0x6000, 0x4999, 0x50cc, 0x5266, 0x4f33, 
    
    0x0d99, 0x0e66, 0x0e66, 0x1333, 0x10cc, 0x1333, 0x2599, 0x14cc, 
    0x14cc, 0x2599, 0x4f33, 0x34cc, 0x2ccc, 0x34cc, 0x4f33, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 
    
    0x0d99, 0x0e66, 0x0e66, 0x1333, 0x10cc, 0x1333, 0x2599, 0x14cc, 
    0x14cc, 0x2599, 0x4f33, 0x34cc, 0x2ccc, 0x34cc, 0x4f33, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 
    0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00, 0xcc00

  }

},
