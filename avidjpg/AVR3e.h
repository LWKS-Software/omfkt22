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
/* compress "AVR3e capture" 	320x240x32		Q=38	14k/f	res#332
 */
/* Size = 320 x 240 x 32; Q Factor = 38; 14336 Bytes/frame (target);
   Resource #332 */


#define AVR3e_NAME "AVR3e"

#define AVR3e_ID 37


{
  "AVR3e", /* avr string */
  37, /* TableID */
  38, /* Qfactor */
  320, /* HSize */
  240, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  8, /* Leading Lines (NTSC) */
  8, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  14336, /* Target Frame Size */

  { /* 8bit Quantization tables */
     12,   8,   9,  11,   9,   8,  12,  11, 
     10,  11,  14,  13,  12,  14,  18,  30, 
     20,  18,  17,  17,  18,  37,  27,  28, 
     22,  30,  44,  39,  46,  46,  43,  39, 
     43,  42,  49,  55,  70,  59,  49,  52, 
     66,  52,  42,  43,  61,  83,  62,  66, 
     72,  74,  78,  79,  78,  47,  59,  86, 
     92,  85,  76,  91,  70,  77,  78,  75, 
    
     13,  14,  14,  18,  16,  18,  36,  20, 
     20,  36,  75,  50,  43,  50,  75, 194, 
    194, 194, 194, 194, 194, 194, 194, 194, 
    194, 194, 194, 194, 194, 194, 194, 194, 
    194, 194, 194, 194, 194, 194, 194, 194, 
    194, 194, 194, 194, 194, 194, 194, 194, 
    194, 194, 194, 194, 194, 194, 194, 194, 
    194, 194, 194, 194, 194, 194, 194, 194, 
    
     13,  14,  14,  18,  16,  18,  36,  20, 
     20,  36,  75,  50,  43,  50,  75, 194, 
    194, 194, 194, 194, 194, 194, 194, 194, 
    194, 194, 194, 194, 194, 194, 194, 194, 
    194, 194, 194, 194, 194, 194, 194, 194, 
    194, 194, 194, 194, 194, 194, 194, 194, 
    194, 194, 194, 194, 194, 194, 194, 194, 
    194, 194, 194, 194, 194, 194, 194, 194 
  },

  { /* 16bit Quantization Tables */
    0x0c28, 0x085c, 0x091e, 0x0aa3, 0x091e, 0x0799, 0x0c28, 0x0aa3, 
    0x09e1, 0x0aa3, 0x0dae, 0x0ceb, 0x0c28, 0x0e70, 0x123d, 0x1e66, 
    0x13c2, 0x123d, 0x10b8, 0x10b8, 0x123d, 0x253d, 0x1a99, 0x1c1e, 
    0x160a, 0x1e66, 0x2c14, 0x26c2, 0x2e5c, 0x2d99, 0x2b51, 0x26c2, 
    0x2a8f, 0x29cc, 0x30a3, 0x36b8, 0x45eb, 0x3b47, 0x30a3, 0x33ae, 
    0x421e, 0x3470, 0x29cc, 0x2a8f, 0x3ccc, 0x52d7, 0x3d8f, 0x421e, 
    0x4833, 0x4a7a, 0x4e47, 0x4f0a, 0x4e47, 0x2f1e, 0x3a85, 0x55e1, 
    0x5bf5, 0x551e, 0x4c00, 0x5b33, 0x45eb, 0x4cc2, 0x4e47, 0x4b3d, 
    
    0x0ceb, 0x0dae, 0x0dae, 0x123d, 0x0ff5, 0x123d, 0x23b8, 0x13c2, 
    0x13c2, 0x23b8, 0x4b3d, 0x3228, 0x2a8f, 0x3228, 0x4b3d, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 
    
    0x0ceb, 0x0dae, 0x0dae, 0x123d, 0x0ff5, 0x123d, 0x23b8, 0x13c2, 
    0x13c2, 0x23b8, 0x4b3d, 0x3228, 0x2a8f, 0x3228, 0x4b3d, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 
    0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc, 0xc1cc

  }

}, 
