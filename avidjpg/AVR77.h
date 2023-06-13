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
/* compress "AVR77 capture" 	720x248x32		Q=6	301k/f	res#2882
 */
/* Size = 720 x 248 x 32; Q Factor = 6; 308224 Bytes/frame (target);
   Resource #2882 */


#define AVR77_NAME "AVR77"

#define AVR77_ID 72


{
  "AVR77", /* avr string */
  72, /* TableID */
  6, /* Qfactor */
  720, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  2, /* Number of Fields */
  308224, /* Target Frame Size */

  { /* 8bit Quantization tables */
    2, 1, 1, 4, 4, 4, 4, 4, 
    4, 4, 4, 4, 4, 4, 4, 4, 
    4, 4, 4, 4, 4, 4, 4, 4, 
    4, 4, 4, 4, 4, 6, 4, 4, 
    4, 4, 4, 4, 4, 4, 4, 4, 
    4, 8, 7, 7, 10, 4, 4, 4, 
    4, 4, 4, 5, 12, 7, 9, 14, 
    5, 4, 5, 14, 11, 12, 12, 12, 

    2, 1, 1, 4, 4, 4, 4, 5, 
    5, 4, 5, 5, 5, 6, 5, 12, 
    12, 6, 6, 6, 5, 6, 7, 7, 
    12, 12, 12, 12, 12, 12, 12, 12, 
    12, 12, 12, 6, 12, 12, 12, 12, 
    12, 12, 12, 12, 12, 12, 12, 12, 
    12, 12, 12, 12, 12, 12, 12, 12, 
    12, 12, 12, 12, 12, 12, 12, 12, 
    
    2, 1, 1, 4, 4, 4, 4, 5, 
    5, 4, 5, 5, 5, 6, 5, 12, 
    12, 6, 6, 6, 5, 6, 7, 7, 
    12, 12, 12, 12, 12, 12, 12, 12, 
    12, 12, 12, 6, 12, 12, 12, 12, 
    12, 12, 12, 12, 12, 12, 12, 12, 
    12, 12, 12, 12, 12, 12, 12, 12, 
    12, 12, 12, 12, 12, 12, 12, 12
  },

  { /* 16bit Quantization Tables */
    0x0200, 0x0100, 0x0100, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400,
    0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 
    0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 
    0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0600, 0x0400, 0x0400, 
    0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 
    0x0400, 0x0800, 0x0700, 0x0700, 0x0A00, 0x0400, 0x0400, 0x0400, 
    0x0400, 0x0400, 0x0400, 0x0500, 0x0C00, 0x0700, 0x0900, 0x0E00, 
    0x0500, 0x0400, 0x0500, 0x0E00, 0x0B00, 0x0C00, 0x0C00, 0x0C00, 

    0x0200, 0x0100, 0x0100, 0x0400, 0x0400, 0x0400, 0x0400, 0x0500,
    0x0500, 0x0400, 0x0500, 0x0500, 0x0500, 0x0600, 0x0500, 0x0C00, 
    0x0C00, 0x0600, 0x0600, 0x0600, 0x0500, 0x0600, 0x0700, 0x0700, 
    0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 
    0x0C00, 0x0C00, 0x0C00, 0x0600, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 
    0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 
    0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 
    0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 

    0x0200, 0x0100, 0x0100, 0x0400, 0x0400, 0x0400, 0x0400, 0x0500,
    0x0500, 0x0400, 0x0500, 0x0500, 0x0500, 0x0600, 0x0500, 0x0C00, 
    0x0C00, 0x0600, 0x0600, 0x0600, 0x0500, 0x0600, 0x0700, 0x0700, 
    0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 
    0x0C00, 0x0C00, 0x0C00, 0x0600, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 
    0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 
    0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 
    0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 

  }

}, 
