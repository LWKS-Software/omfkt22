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
/* compress "AVR2s 24 bit capture full detail" 	720x248x32		Q=100	15k/f	res#1102
 */
/* Size = 720 x 248 x 32; Q Factor = 100; 15360 Bytes/frame (target);
   Resource #1102 */


#define AVR2s_NAME "AVR2s"

#define AVR2s_ID 70


{
  "AVR2s", /* avr string */
  70, /* TableID */
  100, /* Qfactor */
  720, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  15360, /* Target Frame Size */

  { /* 8bit Quantization tables */
     32,  24,  24,  28,  24,  28,  32,  26, 
     26,  32,  68,  38,  32,  38,  48,  80, 
     52,  48,  48,  70, 120, 142, 162, 112, 
     74,  80, 116, 102, 122, 128, 110, 112, 
    112, 255, 255, 255, 255, 255, 255, 192, 
    244, 194, 162, 224, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
     34,  36,  36,  48,  42,  48,  94,  52, 
     52,  94, 198, 132, 112, 132, 198, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
     34,  36,  36,  48,  42,  48,  94,  52, 
     52,  94, 198, 132, 112, 132, 198, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255 
  },

  { /* 16bit Quantization Tables */
    0x2000, 0x1800, 0x1800, 0x1c00, 0x1800, 0x1c00, 0x2000, 0x1a00, 
    0x1a00, 0x2000, 0x4400, 0x2600, 0x2000, 0x2600, 0x3000, 0x5000, 
    0x3400, 0x3000, 0x3000, 0x4600, 0x7800, 0x8e00, 0xa200, 0x7000, 
    0x4a00, 0x5000, 0x7400, 0x6600, 0x7a00, 0x8000, 0x6e00, 0x7000, 
    0x7000, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xc000, 
    0xf400, 0xc200, 0xa200, 0xe000, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x2200, 0x2400, 0x2400, 0x3000, 0x2a00, 0x3000, 0x5e00, 0x3400, 
    0x3400, 0x5e00, 0xc600, 0x8400, 0x7000, 0x8400, 0xc600, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x2200, 0x2400, 0x2400, 0x3000, 0x2a00, 0x3000, 0x5e00, 0x3400, 
    0x3400, 0x5e00, 0xc600, 0x8400, 0x7000, 0x8400, 0xc600, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00

  }

}, 
