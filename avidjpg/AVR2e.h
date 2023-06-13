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
/* compress "AVR2e capture" 	320x240x32		Q=52	10k/f	res#322
 */
/* Size = 320 x 240 x 32; Q Factor = 52; 10240 Bytes/frame (target);
   Resource #322 */


#define AVR2e_NAME "AVR2e"

#define AVR2e_ID 39


{
  "AVR2e", /* avr string */
  39, /* TableID */
  52, /* Qfactor */
  320, /* HSize */
  240, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  8, /* Leading Lines (NTSC) */
  8, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  10240, /* Target Frame Size */

  { /* 8bit Quantization tables */
     17,  11,  12,  15,  12,  10,  17,  15, 
     14,  15,  19,  18,  17,  20,  25,  42, 
     27,  25,  23,  23,  25,  51,  36,  38, 
     30,  42,  60,  53,  63,  62,  59,  53, 
     58,  57,  67,  75,  96,  81,  67,  71, 
     90,  72,  57,  58,  83, 113,  84,  90, 
     99, 102, 107, 108, 107,  64,  80, 118, 
    126, 116, 104, 125,  96, 105, 107, 103, 
    
     18,  19,  19,  25,  22,  25,  49,  27, 
     27,  49, 103,  69,  58,  69, 103, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
     18,  19,  19,  25,  22,  25,  49,  27, 
     27,  49, 103,  69,  58,  69, 103, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255 
  },

  { /* 16bit Quantization Tables */
    0x10a3, 0x0b70, 0x0c7a, 0x0e8f, 0x0c7a, 0x0a66, 0x10a3, 0x0e8f, 
    0x0d85, 0x0e8f, 0x12b8, 0x11ae, 0x10a3, 0x13c2, 0x18f5, 0x2999, 
    0x1b0a, 0x18f5, 0x16e1, 0x16e1, 0x18f5, 0x32f5, 0x2466, 0x267a, 
    0x1e28, 0x2999, 0x3c51, 0x350a, 0x3f70, 0x3e66, 0x3b47, 0x350a, 
    0x3a3d, 0x3933, 0x428f, 0x4ae1, 0x5fae, 0x511e, 0x428f, 0x46b8, 
    0x5a7a, 0x47c2, 0x3933, 0x3a3d, 0x5333, 0x715c, 0x543d, 0x5a7a, 
    0x62cc, 0x65eb, 0x6b1e, 0x6c28, 0x6b1e, 0x407a, 0x5014, 0x7585, 
    0x7dd7, 0x747a, 0x6800, 0x7ccc, 0x5fae, 0x690a, 0x6b1e, 0x66f5, 
    
    0x11ae, 0x12b8, 0x12b8, 0x18f5, 0x15d7, 0x18f5, 0x30e1, 0x1b0a, 
    0x1b0a, 0x30e1, 0x66f5, 0x44a3, 0x3a3d, 0x44a3, 0x66f5, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x11ae, 0x12b8, 0x12b8, 0x18f5, 0x15d7, 0x18f5, 0x30e1, 0x1b0a, 
    0x1b0a, 0x30e1, 0x66f5, 0x44a3, 0x3a3d, 0x44a3, 0x66f5, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00

  }

}, 
