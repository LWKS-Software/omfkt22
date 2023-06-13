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
/* compress "AVR4 capture" 	640x240x32		Q=54	24k/f	res#242
 */
/* Size = 640 x 240 x 32; Q Factor = 54; 24576 Bytes/frame (target);
   Resource #242 */


#define AVR4_NAME "AVR4"

#define AVR4_ID 30


{
  "AVR4", /* avr string */
  30, /* TableID */
  54, /* Qfactor */
  640, /* HSize */
  240, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  8, /* Leading Lines (NTSC) */
  8, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  24576, /* Target Frame Size */

  { /* 8bit Quantization tables */
    17, 12, 13, 15, 13, 11, 17, 15, 
    14, 15, 19, 18, 17, 21, 26, 43, 
    28, 26, 24, 24, 26, 53, 38, 40, 
    31, 43, 63, 55, 66, 65, 62, 55, 
    60, 59, 69, 78, 99, 84, 69, 73, 
    94, 75, 59, 60, 86, 118, 87, 94, 
    103, 106, 111, 112, 111, 67, 83, 122, 
    131, 121, 108, 130, 99, 109, 111, 107, 
    
    18, 19, 19, 26, 23, 26, 51, 28, 
    28, 51, 107, 71, 60, 71, 107, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
    18, 19, 19, 26, 23, 26, 51, 28, 
    28, 51, 107, 71, 60, 71, 107, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255
  },

  { /* 16bit Quantization Tables */
    0x1147, 0x0be1, 0x0cf5, 0x0f1e, 0x0cf5, 0x0acc, 0x1147, 0x0f1e, 
    0x0e0a, 0x0f1e, 0x1370, 0x125c, 0x1147, 0x1485, 0x19eb, 0x2b33, 
    0x1c14, 0x19eb, 0x17c2, 0x17c2, 0x19eb, 0x34eb, 0x25cc, 0x27f5, 
    0x1f51, 0x2b33, 0x3ea3, 0x3714, 0x41e1, 0x40cc, 0x3d8f, 0x3714, 
    0x3c7a, 0x3b66, 0x451e, 0x4dc2, 0x635c, 0x543d, 0x451e, 0x4970, 
    0x5df5, 0x4a85, 0x3b66, 0x3c7a, 0x5666, 0x75b8, 0x577a, 0x5df5, 
    0x6699, 0x69d7, 0x6f3d, 0x7051, 0x6f3d, 0x42f5, 0x5328, 0x7a0a, 
    0x82ae, 0x78f5, 0x6c00, 0x8199, 0x635c, 0x6d14, 0x6f3d, 0x6aeb, 
    
    0x125c, 0x1370, 0x1370, 0x19eb, 0x16ae, 0x19eb, 0x32c2, 0x1c14, 
    0x1c14, 0x32c2, 0x6aeb, 0x4747, 0x3c7a, 0x4747, 0x6aeb, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x125c, 0x1370, 0x1370, 0x19eb, 0x16ae, 0x19eb, 0x32c2, 0x1c14, 
    0x1c14, 0x32c2, 0x6aeb, 0x4747, 0x3c7a, 0x4747, 0x6aeb, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00

  }

}, 
