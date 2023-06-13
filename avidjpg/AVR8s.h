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
/* compress "AVR8s 24 bit capture full detail" 	720x248x32		Q=9	130k/f	res#1092
 */
/* Size = 720 x 248 x 32; Q Factor = 9; 133120 Bytes/frame (target);
   Resource #1092 */


#define AVR8s_NAME "AVR8s"

#define AVR8s_ID 59


{
  "AVR8s", /* avr string */
  59, /* TableID */
  9, /* Qfactor */
  720, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  133120, /* Target Frame Size */

  { /* 8bit Quantization tables */
    3, 2, 2, 3, 2, 2, 3, 3, 
    2, 3, 3, 3, 3, 3, 4, 7, 
    5, 4, 4, 4, 4, 9, 6, 7, 
    5, 7, 10, 9, 11, 11, 10, 9, 
    10, 10, 12, 13, 17, 14, 12, 12, 
    16, 12, 10, 10, 14, 20, 15, 16, 
    17, 18, 19, 19, 19, 11, 14, 20, 
    22, 20, 18, 22, 17, 18, 19, 18, 
    
    3, 3, 3, 4, 4, 4, 8, 5, 
    5, 8, 18, 12, 10, 12, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18, 
    
    3, 3, 3, 4, 4, 4, 8, 5, 
    5, 8, 18, 12, 10, 12, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18, 
    18, 18, 18, 18, 18, 18, 18, 18
  },

  { /* 16bit Quantization Tables */
    0x02e1, 0x01fa, 0x0228, 0x0285, 0x0228, 0x01cc, 0x02e1, 0x0285, 
    0x0257, 0x0285, 0x033d, 0x030f, 0x02e1, 0x036b, 0x0451, 0x0733, 
    0x04ae, 0x0451, 0x03f5, 0x03f5, 0x0451, 0x08d1, 0x064c, 0x06a8, 
    0x0538, 0x0733, 0x0a70, 0x092e, 0x0afa, 0x0acc, 0x0a42, 0x092e, 
    0x0a14, 0x09e6, 0x0b85, 0x0cf5, 0x108f, 0x0e0a, 0x0b85, 0x0c3d, 
    0x0fa8, 0x0c6b, 0x09e6, 0x0a14, 0x0e66, 0x139e, 0x0e94, 0x0fa8, 
    0x1119, 0x11a3, 0x128a, 0x12b8, 0x128a, 0x0b28, 0x0ddc, 0x1457, 
    0x15c7, 0x1428, 0x1200, 0x1599, 0x108f, 0x122e, 0x128a, 0x11d1, 
    
    0x030f, 0x033d, 0x033d, 0x0451, 0x03c7, 0x0451, 0x0875, 0x04ae, 
    0x04ae, 0x0875, 0x11d1, 0x0be1, 0x0a14, 0x0be1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 
    
    0x030f, 0x033d, 0x033d, 0x0451, 0x03c7, 0x0451, 0x0875, 0x04ae, 
    0x04ae, 0x0875, 0x11d1, 0x0be1, 0x0a14, 0x0be1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 
    0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1, 0x11d1

  }

}, 
