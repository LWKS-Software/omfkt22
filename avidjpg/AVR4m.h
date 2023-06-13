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
/* compress "AVR4m capture" 	360x248x32		Q=12	28k/f	res#282
 */
/* Size = 360 x 248 x 32; Q Factor = 12; 28672 Bytes/frame (target);
   Resource #282 */


#define AVR4m_NAME "AVR4m"

#define AVR4m_ID 44


{
  "AVR4m", /* avr string */
  44, /* TableID */
  12, /* Qfactor */
  352, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  28672, /* Target Frame Size */

  { /* 8bit Quantization tables */
    4, 3, 3, 3, 3, 2, 4, 3, 
    3, 3, 4, 4, 4, 5, 6, 10, 
    6, 6, 5, 5, 6, 12, 8, 9, 
    7, 10, 14, 12, 15, 14, 14, 12, 
    13, 13, 15, 17, 22, 19, 15, 16, 
    21, 17, 13, 13, 19, 26, 19, 21, 
    23, 24, 25, 25, 25, 15, 18, 27, 
    29, 27, 24, 29, 22, 24, 25, 24, 
    
    4, 4, 4, 6, 5, 6, 11, 6, 
    6, 11, 24, 16, 13, 16, 24, 61, 
    61, 61, 61, 61, 61, 61, 61, 61, 
    61, 61, 61, 61, 61, 61, 61, 61, 
    61, 61, 61, 61, 61, 61, 61, 61, 
    61, 61, 61, 61, 61, 61, 61, 61, 
    61, 61, 61, 61, 61, 61, 61, 61, 
    61, 61, 61, 61, 61, 61, 61, 61, 
    
    4, 4, 4, 6, 5, 6, 11, 6, 
    6, 11, 24, 16, 13, 16, 24, 61, 
    61, 61, 61, 61, 61, 61, 61, 61, 
    61, 61, 61, 61, 61, 61, 61, 61, 
    61, 61, 61, 61, 61, 61, 61, 61, 
    61, 61, 61, 61, 61, 61, 61, 61, 
    61, 61, 61, 61, 61, 61, 61, 61, 
    61, 61, 61, 61, 61, 61, 61, 61
  },

  { /* 16bit Quantization Tables */
    0x03d7, 0x02a3, 0x02e1, 0x035c, 0x02e1, 0x0266, 0x03d7, 0x035c, 
    0x031e, 0x035c, 0x0451, 0x0414, 0x03d7, 0x048f, 0x05c2, 0x0999, 
    0x063d, 0x05c2, 0x0547, 0x0547, 0x05c2, 0x0bc2, 0x0866, 0x08e1, 
    0x06f5, 0x0999, 0x0deb, 0x0c3d, 0x0ea3, 0x0e66, 0x0dae, 0x0c3d, 
    0x0d70, 0x0d33, 0x0f5c, 0x1147, 0x1614, 0x12b8, 0x0f5c, 0x1051, 
    0x14e1, 0x108f, 0x0d33, 0x0d70, 0x1333, 0x1a28, 0x1370, 0x14e1, 
    0x16cc, 0x1785, 0x18b8, 0x18f5, 0x18b8, 0x0ee1, 0x127a, 0x1b1e, 
    0x1d0a, 0x1ae1, 0x1800, 0x1ccc, 0x1614, 0x183d, 0x18b8, 0x17c2, 
    
    0x0414, 0x0451, 0x0451, 0x05c2, 0x050a, 0x05c2, 0x0b47, 0x063d, 
    0x063d, 0x0b47, 0x17c2, 0x0fd7, 0x0d70, 0x0fd7, 0x17c2, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 
    
    0x0414, 0x0451, 0x0451, 0x05c2, 0x050a, 0x05c2, 0x0b47, 0x063d, 
    0x063d, 0x0b47, 0x17c2, 0x0fd7, 0x0d70, 0x0fd7, 0x17c2, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 
    0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33, 0x3d33

  }

}, 
