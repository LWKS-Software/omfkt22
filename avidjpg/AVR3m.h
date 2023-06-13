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
/* compress "AVR3m 24 bit capture full detail" 	360x248x32		Q=18	22k/f	res#2892
 */
/* Size = 360 x 248 x 32; Q Factor = 18; 22528 Bytes/frame (target);
   Resource #2892 */


#define AVR3m_NAME "AVR3m"

#define AVR3m_ID 74


{
  "AVR3m", /* avr string */
  74, /* TableID */
  18, /* Qfactor */
  352, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  22528, /* Target Frame Size */

  { /* 8bit Quantization tables */
    6, 4, 4, 5, 4, 5, 6, 5, 
    5, 6, 12, 7, 6, 7, 9, 14, 
    9, 9, 9, 13, 22, 26, 29, 20, 
    13, 14, 21, 18, 22, 23, 20, 20, 
    20, 49, 49, 49, 92, 92, 58, 35, 
    44, 35, 29, 40, 58, 76, 92, 92, 
    92, 92, 92, 92, 92, 61, 92, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    
    6, 6, 6, 9, 8, 9, 17, 9, 
    9, 17, 36, 24, 20, 24, 36, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    
    6, 6, 6, 9, 8, 9, 17, 9, 
    9, 17, 36, 24, 20, 24, 36, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    92, 92, 92, 92, 92, 92, 92, 92, 
    92, 92, 92, 92, 92, 92, 92, 92
  },

  { /* 16bit Quantization Tables */
    0x05C2, 0x0451, 0x0451, 0x050A, 0x0451, 0x050A, 0x05C2, 0x04AE,
    0x04AE, 0x05C2, 0x0C3D, 0x06D7, 0x05C2, 0x06D7, 0x08A3, 0x0E66,
    0x095C, 0x08A3, 0x08A3, 0x0C99, 0x1599, 0x198F, 0x1D28, 0x1428,
    0x0D51, 0x0E66, 0x14E1, 0x125C, 0x15F5, 0x170A, 0x13CC, 0x1428,
    0x1428, 0x3099, 0x3099, 0x3099, 0x5BCC, 0x5BCC, 0x3999, 0x228F,
    0x2BEB, 0x22EB, 0x1D28, 0x2851, 0x3999, 0x4B99, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x3D33, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,

    0x061E, 0x067A, 0x067A, 0x08A3, 0x078F, 0x08A3, 0x10EB, 0x095C,
    0x095C, 0x10EB, 0x23A3, 0x17C2, 0x1428, 0x17C2, 0x23A3, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,

    0x061E, 0x067A, 0x067A, 0x08A3, 0x078F, 0x08A3, 0x10EB, 0x095C,
    0x095C, 0x10EB, 0x23A3, 0x17C2, 0x1428, 0x17C2, 0x23A3, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC,
    0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC, 0x5BCC

  }

}, 
