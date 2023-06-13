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
/* compress "AVR4s 24 bit capture full detail" 	720x248x32		Q=37	30k/f	res#1072
 */
/* Size = 720 x 248 x 32; Q Factor = 37; 30720 Bytes/frame (target);
   Resource #1072 */


#define AVR4s_NAME "AVR4s"

#define AVR4s_ID 57


{
  "AVR4s", /* avr string */
  57, /* TableID */
  37, /* Qfactor */
  720, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  30720, /* Target Frame Size */

  { /* 8bit Quantization tables */
    12, 8, 9, 10, 9, 7, 12, 10, 
    10, 10, 13, 13, 12, 14, 18, 30, 
    19, 18, 16, 16, 18, 36, 26, 27, 
    21, 30, 43, 38, 45, 44, 42, 38, 
    41, 41, 47, 53, 68, 58, 47, 50, 
    64, 51, 41, 41, 59, 81, 60, 64, 
    70, 73, 76, 77, 76, 46, 57, 84, 
    90, 83, 74, 89, 68, 75, 76, 73, 
    
    13, 13, 13, 18, 16, 18, 35, 19, 
    19, 35, 73, 49, 41, 49, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73, 
    
    13, 13, 13, 18, 16, 18, 35, 19, 
    19, 35, 73, 49, 41, 49, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73, 
    73, 73, 73, 73, 73, 73, 73, 73
  },

  { /* 16bit Quantization Tables */
    0x0bd7, 0x0823, 0x08e1, 0x0a5c, 0x08e1, 0x0766, 0x0bd7, 0x0a5c, 
    0x099e, 0x0a5c, 0x0d51, 0x0c94, 0x0bd7, 0x0e0f, 0x11c2, 0x1d99, 
    0x133d, 0x11c2, 0x1047, 0x1047, 0x11c2, 0x2442, 0x19e6, 0x1b61, 
    0x1575, 0x1d99, 0x2aeb, 0x25bd, 0x2d23, 0x2c66, 0x2a2e, 0x25bd, 
    0x2970, 0x28b3, 0x2f5c, 0x3547, 0x4414, 0x39b8, 0x2f5c, 0x3251, 
    0x4061, 0x330f, 0x28b3, 0x2970, 0x3b33, 0x50a8, 0x3bf0, 0x4061, 
    0x464c, 0x4885, 0x4c38, 0x4cf5, 0x4c38, 0x2de1, 0x38fa, 0x539e, 
    0x598a, 0x52e1, 0x4a00, 0x58cc, 0x4414, 0x4abd, 0x4c38, 0x4942, 
    
    0x0c94, 0x0d51, 0x0d51, 0x11c2, 0x0f8a, 0x11c2, 0x22c7, 0x133d, 
    0x133d, 0x22c7, 0x4942, 0x30d7, 0x2970, 0x30d7, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 
    
    0x0c94, 0x0d51, 0x0d51, 0x11c2, 0x0f8a, 0x11c2, 0x22c7, 0x133d, 
    0x133d, 0x22c7, 0x4942, 0x30d7, 0x2970, 0x30d7, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 
    0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942, 0x4942

  }

}, 
