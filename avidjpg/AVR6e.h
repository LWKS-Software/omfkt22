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
/* compress "AVR6e capture" 	640x240x32		Q=10	64k/f	res#362
 */
/* Size = 640 x 240 x 32; Q Factor = 10; 65536 Bytes/frame (target);
   Resource #362 */


#define AVR6e_NAME "AVR6e"

#define AVR6e_ID 42


{
  "AVR6e", /* avr string */
  42, /* TableID */
  10, /* Qfactor */
  640, /* HSize */
  240, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  8, /* Leading Lines (NTSC) */
  8, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  65536, /* Target Frame Size */

  { /* 8bit Quantization tables */
    3, 2, 2, 3, 2, 2, 3, 3, 
    3, 3, 4, 3, 3, 4, 5, 8, 
    5, 5, 4, 4, 5, 10, 7, 7, 
    6, 8, 12, 10, 12, 12, 11, 10, 
    11, 11, 13, 14, 18, 16, 13, 14, 
    17, 14, 11, 11, 16, 22, 16, 17, 
    19, 20, 21, 21, 21, 12, 15, 23, 
    24, 22, 20, 24, 18, 20, 21, 20, 
    
    3, 4, 4, 5, 4, 5, 9, 5, 
    5, 9, 20, 13, 11, 13, 20, 51, 
    51, 51, 51, 51, 51, 51, 51, 51, 
    51, 51, 51, 51, 51, 51, 51, 51, 
    51, 51, 51, 51, 51, 51, 51, 51, 
    51, 51, 51, 51, 51, 51, 51, 51, 
    51, 51, 51, 51, 51, 51, 51, 51, 
    51, 51, 51, 51, 51, 51, 51, 51, 
    
    3, 4, 4, 5, 4, 5, 9, 5, 
    5, 9, 20, 13, 11, 13, 20, 51, 
    51, 51, 51, 51, 51, 51, 51, 51, 
    51, 51, 51, 51, 51, 51, 51, 51, 
    51, 51, 51, 51, 51, 51, 51, 51, 
    51, 51, 51, 51, 51, 51, 51, 51, 
    51, 51, 51, 51, 51, 51, 51, 51, 
    51, 51, 51, 51, 51, 51, 51, 51
  },

  { /* 16bit Quantization Tables */
    0x0333, 0x0233, 0x0266, 0x02cc, 0x0266, 0x0200, 0x0333, 0x02cc, 
    0x0299, 0x02cc, 0x0399, 0x0366, 0x0333, 0x03cc, 0x04cc, 0x0800, 
    0x0533, 0x04cc, 0x0466, 0x0466, 0x04cc, 0x09cc, 0x0700, 0x0766, 
    0x05cc, 0x0800, 0x0b99, 0x0a33, 0x0c33, 0x0c00, 0x0b66, 0x0a33, 
    0x0b33, 0x0b00, 0x0ccc, 0x0e66, 0x1266, 0x0f99, 0x0ccc, 0x0d99, 
    0x1166, 0x0dcc, 0x0b00, 0x0b33, 0x1000, 0x15cc, 0x1033, 0x1166, 
    0x1300, 0x1399, 0x1499, 0x14cc, 0x1499, 0x0c66, 0x0f66, 0x1699, 
    0x1833, 0x1666, 0x1400, 0x1800, 0x1266, 0x1433, 0x1499, 0x13cc, 
    
    0x0366, 0x0399, 0x0399, 0x04cc, 0x0433, 0x04cc, 0x0966, 0x0533, 
    0x0533, 0x0966, 0x13cc, 0x0d33, 0x0b33, 0x0d33, 0x13cc, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 
    
    0x0366, 0x0399, 0x0399, 0x04cc, 0x0433, 0x04cc, 0x0966, 0x0533, 
    0x0533, 0x0966, 0x13cc, 0x0d33, 0x0b33, 0x0d33, 0x13cc, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 
    0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300, 0x3300

  }

}, 
