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
/* compress "AVR1e 24 bit capture full detail" 	320x240x32		Q=95	7k/f	res#312
 */
/* Size = 320 x 240 x 32; Q Factor = 95; 7168 Bytes/frame (target);
   Resource #312 */


#define AVR1e_NAME "AVR1e"

#define AVR1e_ID 38


{
  "AVR1e", /* avr string */
  38, /* TableID */
  95, /* Qfactor */
  320, /* HSize */
  240, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  8, /* Leading Lines (NTSC) */
  8, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  7168, /* Target Frame Size */

  { /* 8bit Quantization tables */
     30,  21,  23,  27,  23,  19,  30,  27, 
     25,  27,  34,  32,  30,  36,  46,  76, 
     49,  46,  42,  42,  46,  93,  67,  70, 
     55,  76, 110,  97, 116, 114, 108,  97, 
    106, 105, 122, 137, 175, 148, 122, 129, 
    165, 131, 105, 106, 152, 207, 154, 165, 
    181, 186, 196, 198, 196, 118, 146, 215, 
    230, 213, 190, 228, 175, 192, 196, 188, 
    
     32,  34,  34,  46,  40,  46,  89,  49, 
     49,  89, 188, 125, 106, 125, 188, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
     32,  34,  34,  46,  40,  46,  89,  49, 
     49,  89, 188, 125, 106, 125, 188, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255
  },

  { /* 16bit Quantization Tables */
    0x1e66, 0x14e6, 0x16cc, 0x1a99, 0x16cc, 0x1300, 0x1e66, 0x1a99, 
    0x18b3, 0x1a99, 0x2233, 0x204c, 0x1e66, 0x2419, 0x2d99, 0x4c00, 
    0x3166, 0x2d99, 0x29cc, 0x29cc, 0x2d99, 0x5d19, 0x4280, 0x464c, 
    0x3719, 0x4c00, 0x6e33, 0x60e6, 0x73e6, 0x7200, 0x6c4c, 0x60e6, 
    0x6a66, 0x6880, 0x7999, 0x88cc, 0xaecc, 0x9433, 0x7999, 0x8133, 
    0xa54c, 0x8319, 0x6880, 0x6a66, 0x9800, 0xcf19, 0x99e6, 0xa54c, 
    0xb480, 0xba33, 0xc3b3, 0xc599, 0xc3b3, 0x75cc, 0x924c, 0xd6b3, 
    0xe5e6, 0xd4cc, 0xbe00, 0xe400, 0xaecc, 0xbfe6, 0xc3b3, 0xbc19, 
    
    0x204c, 0x2233, 0x2233, 0x2d99, 0x27e6, 0x2d99, 0x594c, 0x3166, 
    0x3166, 0x594c, 0xbc19, 0x7d66, 0x6a66, 0x7d66, 0xbc19, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x204c, 0x2233, 0x2233, 0x2d99, 0x27e6, 0x2d99, 0x594c, 0x3166, 
    0x3166, 0x594c, 0xbc19, 0x7d66, 0x6a66, 0x7d66, 0xbc19, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00

  }

}, 
