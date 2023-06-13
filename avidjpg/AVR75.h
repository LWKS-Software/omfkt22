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
/* compress "AVR75 capture" 	720x248x32		Q=11	200k/f	res#2762
 */
/* Size = 720 x 248 x 32; Q Factor = 11; 204800 Bytes/frame (target);
   Resource #2762 */


#define AVR75_NAME "AVR75"

#define AVR75_ID 50


{
  "AVR75", /* avr string */
  50, /* TableID */
  11, /* Qfactor */
  720, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  2, /* Number of Fields */
  204800, /* Target Frame Size */

  { /* 8bit Quantization tables */
    3, 3, 3, 7, 7, 7, 7, 7, 
    7, 7, 7, 7, 7, 7, 7, 7, 
    7, 7, 7, 7, 7, 7, 7, 7, 
    7, 7, 7, 7, 8, 10, 8, 7, 
    7, 7, 7, 7, 7, 7, 7, 7, 
    8, 15, 12, 12, 18, 8, 8, 7, 
    7, 7, 8, 9, 23, 14, 17, 25, 
    9, 7, 9, 26, 20, 22, 23, 22, 
    
    3, 3, 3, 8, 8, 8, 8, 9, 
    9, 8, 9, 9, 9, 10, 9, 56, 
    56, 11, 10, 10, 9, 10, 12, 12, 
    56, 56, 56, 56, 56, 56, 56, 56, 
    56, 56, 56, 10, 56, 56, 56, 56, 
    56, 56, 56, 56, 56, 56, 56, 56, 
    56, 56, 56, 56, 56, 56, 56, 56, 
    56, 56, 56, 56, 56, 56, 56, 56, 
    
    3, 3, 3, 8, 8, 8, 8, 9, 
    9, 8, 9, 9, 9, 10, 9, 56, 
    56, 11, 10, 10, 9, 10, 12, 12, 
    56, 56, 56, 56, 56, 56, 56, 56, 
    56, 56, 56, 10, 56, 56, 56, 56, 
    56, 56, 56, 56, 56, 56, 56, 56, 
    56, 56, 56, 56, 56, 56, 56, 56, 
    56, 56, 56, 56, 56, 56, 56, 56
  },

  { /* 16bit Quantization Tables */
    0x02A3, 0x02A3, 0x02A3, 0x0699, 0x0699, 0x0699, 0x0699, 0x0699,
    0x0699, 0x0699, 0x0699, 0x0699, 0x0699, 0x0699, 0x0699, 0x0699,
    0x0699, 0x0699, 0x0699, 0x0699, 0x0699, 0x0699, 0x0699, 0x0699,
    0x0699, 0x0699, 0x0699, 0x0699, 0x07B3, 0x0A1E, 0x07EB, 0x0699,
    0x0699, 0x0699, 0x0699, 0x0699, 0x0699, 0x0699, 0x0699, 0x0699,
    0x07EB, 0x0F2E, 0x0C19, 0x0C51, 0x1199, 0x07EB, 0x07EB, 0x0699,
    0x0699, 0x0699, 0x07EB, 0x0905, 0x16A8, 0x0DA3, 0x10F0, 0x18DC,
    0x0905, 0x0699, 0x0905, 0x1A66, 0x143D, 0x1638, 0x16A8, 0x15C7,

    0x02A3, 0x02A3, 0x02A3, 0x07EB, 0x07EB, 0x07EB, 0x07EB, 0x0905,
    0x0905, 0x07EB, 0x0905, 0x0905, 0x0905, 0x0A1E, 0x0905, 0x3819,
    0x3819, 0x0B38, 0x0A1E, 0x0A1E, 0x0905, 0x0A1E, 0x0C51, 0x0C51,
    0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819,
    0x3819, 0x3819, 0x3819, 0x0A1E, 0x3819, 0x3819, 0x3819, 0x3819,
    0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819,
    0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819,
    0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819,

    0x02A3, 0x02A3, 0x02A3, 0x07EB, 0x07EB, 0x07EB, 0x07EB, 0x0905,
    0x0905, 0x07EB, 0x0905, 0x0905, 0x0905, 0x0A1E, 0x0905, 0x3819,
    0x3819, 0x0B38, 0x0A1E, 0x0A1E, 0x0905, 0x0A1E, 0x0C51, 0x0C51,
    0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819,
    0x3819, 0x3819, 0x3819, 0x0A1E, 0x3819, 0x3819, 0x3819, 0x3819,
    0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819,
    0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819,
    0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819, 0x3819

  }

}, 
