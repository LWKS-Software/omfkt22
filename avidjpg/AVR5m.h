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
/* compress "AVR5m 24 bit capture full detail" 	360x248x32		Q=11	36k/f	res#1112
 */
/* Size = 360 x 248 x 32; Q Factor = 11; 36864 Bytes/frame (target);
   Resource #1112 */


#define AVR5m_NAME "AVR5m"

#define AVR5m_ID 71


{
  "AVR5m", /* avr string */
  71, /* TableID */
  11, /* Qfactor */
  352, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  36864, /* Target Frame Size */

  { /* 8bit Quantization tables */
    4, 2, 3, 3, 3, 2, 4, 3, 
    3, 3, 4, 4, 4, 4, 5, 9, 
    6, 5, 5, 5, 5, 11, 8, 8, 
    6, 9, 13, 11, 13, 13, 13, 11, 
    12, 12, 14, 16, 20, 17, 14, 15, 
    19, 15, 12, 12, 18, 24, 18, 19, 
    21, 22, 23, 23, 23, 14, 17, 25, 
    27, 25, 22, 26, 20, 22, 23, 22, 
    
    4, 4, 4, 5, 5, 5, 10, 6, 
    6, 10, 22, 15, 12, 15, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 
    
    4, 4, 4, 5, 5, 5, 10, 6, 
    6, 10, 22, 15, 12, 15, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22
  },

  { /* 16bit Quantization Tables */
    0x0385, 0x026B, 0x02A3, 0x0314, 0x02A3, 0x0233, 0x0385, 0x0314,
    0x02DC, 0x0314, 0x03F5, 0x03BD, 0x0385, 0x042E, 0x0547, 0x08CC,
    0x05B8, 0x0547, 0x04D7, 0x04D7, 0x0547, 0x0AC7, 0x07B3, 0x0823,
    0x0661, 0x08CC, 0x0CC2, 0x0B38, 0x0D6B, 0x0D33, 0x0C8A, 0x0B38,
    0x0C51, 0x0C19, 0x0E14, 0x0FD7, 0x143D, 0x1128, 0x0E14, 0x0EF5,
    0x1323, 0x0F2E, 0x0C19, 0x0C51, 0x1199, 0x17FA, 0x11D1, 0x1323,
    0x14E6, 0x158F, 0x16A8, 0x16E1, 0x16A8, 0x0DA3, 0x10F0, 0x18DC,
    0x1A9E, 0x18A3, 0x1600, 0x1A66, 0x143D, 0x1638, 0x16A8, 0x15C7,

    0x03BD, 0x03F5, 0x03F5, 0x0547, 0x049E, 0x0547, 0x0A57, 0x05B8,
    0x05B8, 0x0A57, 0x15C7, 0x0E85, 0x0C51, 0x0E85, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7,

    0x03BD, 0x03F5, 0x03F5, 0x0547, 0x049E, 0x0547, 0x0A57, 0x05B8,
    0x05B8, 0x0A57, 0x15C7, 0x0E85, 0x0C51, 0x0E85, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7,
    0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7, 0x15C7

  }

}, 
