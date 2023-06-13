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
/* compress "AVR12 capture" 	720x248x32		Q=90	33k/f	res#2872
 */
/* Size = 720 x 248 x 32; Q Factor = 90; 33792 Bytes/frame (target);
   Resource #2872 */


#define AVR12_NAME "AVR12"

#define AVR12_ID 52


{
  "AVR12", /* avr string */
  52, /* TableID */
  90, /* Qfactor */
  720, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  2, /* Number of Fields */
  33792, /* Target Frame Size */

  { /* 8bit Quantization tables */
     29,  22,  22,  25,  22,  25,  29,  23, 
     23,  29,  61,  34,  29,  34,  43,  72, 
     47,  43,  43,  63, 108, 128, 146, 101, 
     67,  72, 104,  92, 110, 115,  99, 101, 
    101, 243, 243, 243, 255, 255, 255, 173, 
    220, 175, 146, 202, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
     31,  32,  32,  43,  38,  43,  85,  47, 
     47,  85, 178, 119, 101, 119, 178, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
     31,  32,  32,  43,  38,  43,  85,  47, 
     47,  85, 178, 119, 101, 119, 178, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255
  },

  { /* 16bit Quantization Tables */
    0x1ccc, 0x1599, 0x1599, 0x1933, 0x1599, 0x1933, 0x1ccc, 0x1766, 
    0x1766, 0x1ccc, 0x3d33, 0x2233, 0x1ccc, 0x2233, 0x2b33, 0x4800, 
    0x2ecc, 0x2b33, 0x2b33, 0x3f00, 0x6c00, 0x7fcc, 0x91cc, 0x64cc, 
    0x4299, 0x4800, 0x6866, 0x5bcc, 0x6dcc, 0x7333, 0x6300, 0x64cc, 
    0x64cc, 0xf300, 0xf300, 0xf300, 0xff00, 0xff00, 0xff00, 0xaccc, 
    0xdb99, 0xae99, 0x91cc, 0xc999, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x1e99, 0x2066, 0x2066, 0x2b33, 0x25cc, 0x2b33, 0x5499, 0x2ecc, 
    0x2ecc, 0x5499, 0xb233, 0x76cc, 0x64cc, 0x76cc, 0xb233, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x1e99, 0x2066, 0x2066, 0x2b33, 0x25cc, 0x2b33, 0x5499, 0x2ecc, 
    0x2ecc, 0x5499, 0xb233, 0x76cc, 0x64cc, 0x76cc, 0xb233, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00

  }

}, 
