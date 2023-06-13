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
/* compress "AVR3s 24 bit capture full detail" 	720x248x32		Q=60	19k/f	res#1062
 */
/* Size = 720 x 248 x 32; Q Factor = 60; 19456 Bytes/frame (target);
   Resource #1062 */


#define AVR3s_NAME "AVR3s"

#define AVR3s_ID 56


{
  "AVR3s", /* avr string */
  56, /* TableID */
  60, /* Qfactor */
  720, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  19456, /* Target Frame Size */

  { /* 8bit Quantization tables */
    19, 14, 14, 17, 14, 17, 19, 16, 
    16, 19, 41, 23, 19, 23, 29, 48, 
    31, 29, 29, 42, 72, 85, 97, 67, 
    44, 48, 70, 61, 73, 77, 66, 67, 
    67, 162, 162, 162, 255, 255, 192, 115, 
    146, 116, 97, 134, 192, 252, 255, 255, 
    255, 255, 255, 255, 255, 204, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
    20, 22, 22, 29, 25, 29, 56, 31, 
    31, 56, 119, 79, 67, 79, 119, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
    20, 22, 22, 29, 25, 29, 56, 31, 
    31, 56, 119, 79, 67, 79, 119, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255
  },

  { /* 16bit Quantization Tables */
    0x1333, 0x0e66, 0x0e66, 0x10cc, 0x0e66, 0x10cc, 0x1333, 0x0f99, 
    0x0f99, 0x1333, 0x28cc, 0x16cc, 0x1333, 0x16cc, 0x1ccc, 0x3000, 
    0x1f33, 0x1ccc, 0x1ccc, 0x2a00, 0x4800, 0x5533, 0x6133, 0x4333, 
    0x2c66, 0x3000, 0x4599, 0x3d33, 0x4933, 0x4ccc, 0x4200, 0x4333, 
    0x4333, 0xa200, 0xa200, 0xa200, 0xff00, 0xff00, 0xc000, 0x7333, 
    0x9266, 0x7466, 0x6133, 0x8666, 0xc000, 0xfc00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xcc00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x1466, 0x1599, 0x1599, 0x1ccc, 0x1933, 0x1ccc, 0x3866, 0x1f33, 
    0x1f33, 0x3866, 0x76cc, 0x4f33, 0x4333, 0x4f33, 0x76cc, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x1466, 0x1599, 0x1599, 0x1ccc, 0x1933, 0x1ccc, 0x3866, 0x1f33, 
    0x1f33, 0x3866, 0x76cc, 0x4f33, 0x4333, 0x4f33, 0x76cc, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00

  }

}, 
