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
/* compress "AVR1 24 bit capture full detail" 	320x240x32		Q=80	9k/f	res#202
 */
/* Size = 320 x 240 x 32; Q Factor = 80; 9216 Bytes/frame (target);
   Resource #202 */


#define AVR1_NAME "AVR1"

#define AVR1_ID 28


{
  "AVR1", /* avr string */
  28, /* TableID */
  80, /* Qfactor */
  320, /* HSize */
  240, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  8, /* Leading Lines (NTSC) */
  8, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  9216, /* Target Frame Size */

  { /* 8bit Quantization tables */
     26,  19,  19,  22,  19,  22,  26,  21, 
     21,  26,  54,  30,  26,  30,  38,  64, 
     42,  38,  38,  56, 255, 255, 255,  90, 
     59,  64,  93,  82,  98, 102,  88,  90, 
     90, 255, 255, 255, 255, 255, 255, 154, 
    195, 155, 130, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
     27,  29,  29,  38,  34,  38,  75,  42, 
     42,  75, 158, 106,  90, 106, 158, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
     27,  29,  29,  38,  34,  38,  75,  42, 
     42,  75, 158, 106,  90, 106, 158, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255
  },

  { /* 16bit Quantization Tables */
    0x1999, 0x1333, 0x1333, 0x1666, 0x1333, 0x1666, 0x1999, 0x14cc, 
    0x14cc, 0x1999, 0x3666, 0x1e66, 0x1999, 0x1e66, 0x2666, 0x4000, 
    0x2999, 0x2666, 0x2666, 0x3800, 0xff00, 0xff00, 0xff00, 0x5999, 
    0x3b33, 0x4000, 0x5ccc, 0x5199, 0x6199, 0x6666, 0x5800, 0x5999, 
    0x5999, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0x9999, 
    0xc333, 0x9b33, 0x8199, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x1b33, 0x1ccc, 0x1ccc, 0x2666, 0x2199, 0x2666, 0x4b33, 0x2999, 
    0x2999, 0x4b33, 0x9e66, 0x6999, 0x5999, 0x6999, 0x9e66, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x1b33, 0x1ccc, 0x1ccc, 0x2666, 0x2199, 0x2666, 0x4b33, 0x2999, 
    0x2999, 0x4b33, 0x9e66, 0x6999, 0x5999, 0x6999, 0x9e66, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00
    
  }

}, 
