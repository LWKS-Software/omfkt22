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
/* compress "AVR6s 24 bit capture full detail" 	720x248x32		Q=27	55k/f	res#1082
 */
/* Size = 720 x 248 x 32; Q Factor = 27; 56320 Bytes/frame (target);
   Resource #1082 */


#define AVR6s_NAME "AVR6s"

#define AVR6s_ID 58


{
  "AVR6s", /* avr string */
  58, /* TableID */
  27, /* Qfactor */
  720, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  56320, /* Target Frame Size */

  { /* 8bit Quantization tables */
    9, 6, 6, 8, 6, 5, 9, 8, 
    7, 8, 10, 9, 9, 10, 13, 22, 
    14, 13, 12, 12, 13, 26, 19, 20, 
    16, 22, 31, 28, 33, 32, 31, 28, 
    30, 30, 35, 39, 50, 42, 35, 37, 
    47, 37, 30, 30, 43, 59, 44, 47, 
    51, 53, 56, 56, 56, 33, 42, 61, 
    65, 60, 54, 65, 50, 55, 56, 53, 
    
    9, 10, 10, 13, 11, 13, 25, 14, 
    14, 25, 53, 36, 30, 36, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53, 
    
    9, 10, 10, 13, 11, 13, 25, 14, 
    14, 25, 53, 36, 30, 36, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53, 
    53, 53, 53, 53, 53, 53, 53, 53
  },

  { /* 16bit Quantization Tables */
    0x08a3, 0x05f0, 0x067a, 0x078f, 0x067a, 0x0566, 0x08a3, 0x078f, 
    0x0705, 0x078f, 0x09b8, 0x092e, 0x08a3, 0x0a42, 0x0cf5, 0x1599, 
    0x0e0a, 0x0cf5, 0x0be1, 0x0be1, 0x0cf5, 0x1a75, 0x12e6, 0x13fa, 
    0x0fa8, 0x1599, 0x1f51, 0x1b8a, 0x20f0, 0x2066, 0x1ec7, 0x1b8a, 
    0x1e3d, 0x1db3, 0x228f, 0x26e1, 0x31ae, 0x2a1e, 0x228f, 0x24b8, 
    0x2efa, 0x2542, 0x1db3, 0x1e3d, 0x2b33, 0x3adc, 0x2bbd, 0x2efa, 
    0x334c, 0x34eb, 0x379e, 0x3828, 0x379e, 0x217a, 0x2994, 0x3d05, 
    0x4157, 0x3c7a, 0x3600, 0x40cc, 0x31ae, 0x368a, 0x379e, 0x3575, 
    
    0x092e, 0x09b8, 0x09b8, 0x0cf5, 0x0b57, 0x0cf5, 0x1961, 0x0e0a, 
    0x0e0a, 0x1961, 0x3575, 0x23a3, 0x1e3d, 0x23a3, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 
    
    0x092e, 0x09b8, 0x09b8, 0x0cf5, 0x0b57, 0x0cf5, 0x1961, 0x0e0a, 
    0x0e0a, 0x1961, 0x3575, 0x23a3, 0x1e3d, 0x23a3, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 
    0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575, 0x3575

  }

}, 
