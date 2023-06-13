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
/* compress "AVR4e capture" 	640x240x32		Q=69	64k/f	res#342
 */
/* Size = 640 x 240 x 32; Q Factor = 69; 65536 Bytes/frame (target);
   Resource #342 */


#define AVR4e_NAME "AVR4e"

#define AVR4e_ID 40


{
  "AVR4e", /* avr string */
  40, /* TableID */
  69, /* Qfactor */
  640, /* HSize */
  240, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  8, /* Leading Lines (NTSC) */
  8, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  65536, /* Target Frame Size */

  { /* 8bit Quantization tables */
    22, 15, 17, 19, 17, 14, 22, 19, 
    18, 19, 25, 23, 22, 26, 33, 55, 
    36, 33, 30, 30, 33, 68, 48, 51, 
    40, 55, 80, 70, 84, 83, 79, 70, 
    77, 76, 88, 99, 127, 108, 88, 94, 
    120, 95, 76, 77, 110, 150, 112, 120, 
    131, 135, 142, 144, 142, 86, 106, 156, 
    167, 155, 138, 166, 127, 139, 142, 137, 
    
    23, 25, 25, 33, 29, 33, 65, 36, 
    36, 65, 137, 91, 77, 91, 137, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
    23, 25, 25, 33, 29, 33, 65, 36, 
    36, 65, 137, 91, 77, 91, 137, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255
  },

  { /* 16bit Quantization Tables */
    0x1614, 0x0f2e, 0x108f, 0x1351, 0x108f, 0x0dcc, 0x1614, 0x1351, 
    0x11f0, 0x1351, 0x18d7, 0x1775, 0x1614, 0x1a38, 0x211e, 0x3733, 
    0x23e1, 0x211e, 0x1e5c, 0x1e5c, 0x211e, 0x439e, 0x304c, 0x330f, 
    0x2805, 0x3733, 0x500a, 0x4661, 0x542e, 0x52cc, 0x4ea8, 0x4661, 
    0x4d47, 0x4be6, 0x5851, 0x635c, 0x7ef5, 0x6ba3, 0x5851, 0x5dd7, 
    0x780f, 0x5f38, 0x4be6, 0x4d47, 0x6e66, 0x966b, 0x6fc7, 0x780f, 
    0x8319, 0x873d, 0x8e23, 0x8f85, 0x8e23, 0x558f, 0x6a42, 0x9bf0, 
    0xa6fa, 0x9a8f, 0x8a00, 0xa599, 0x7ef5, 0x8b61, 0x8e23, 0x889e, 
    
    0x1775, 0x18d7, 0x18d7, 0x211e, 0x1cfa, 0x211e, 0x40dc, 0x23e1, 
    0x23e1, 0x40dc, 0x889e, 0x5b14, 0x4d47, 0x5b14, 0x889e, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x1775, 0x18d7, 0x18d7, 0x211e, 0x1cfa, 0x211e, 0x40dc, 0x23e1, 
    0x23e1, 0x40dc, 0x889e, 0x5b14, 0x4d47, 0x5b14, 0x889e, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00

  }

}, 
