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
/* compress "AVR2m 24 bit capture full detail" 	360x248x32		Q=51	13k/f	res#207
 */
/* Size = 360 x 248 x 32; Q Factor = 51; 13312 Bytes/frame (target);
   Resource #207 */


#define AVR2m_NAME "AVR2m"

#define AVR2m_ID 47


{
  "AVR2m", /* avr string */
  47, /* TableID */
  51, /* Qfactor */
  352, /* HSize */
  243, /* YSize(NTSC) */
  288, /* YSize(PAL) */
  13, /* Leading Lines (NTSC) */
  16, /* Leading Lines (PAL) */
  1, /* Number of Fields */
  13312, /* Target Frame Size */

  { /* 8bit Quantization tables */
     16,  12,  12,  14,  12,  14,  16,  13, 
     13,  16,  35,  19,  16,  19,  24,  41, 
     27,  24,  24,  36,  61,  72,  83,  57, 
     38,  41,  59,  52,  62,  65,  56,  57, 
     57, 138, 138, 138, 255, 255, 163,  98, 
    124,  99,  83, 114, 163, 214, 255, 255, 
    255, 255, 255, 255, 255, 173, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
     17,  18,  18,  24,  21,  24,  48,  27, 
     27,  48, 101,  67,  57,  67, 101, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    
     17,  18,  18,  24,  21,  24,  48,  27, 
     27,  48, 101,  67,  57,  67, 101, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255
  },

  { /* 16bit Quantization Tables */
    0x1051, 0x0c3d, 0x0c3d, 0x0e47, 0x0c3d, 0x0e47, 0x1051, 0x0d42, 
    0x0d42, 0x1051, 0x22ae, 0x1361, 0x1051, 0x1361, 0x187a, 0x28cc, 
    0x1a85, 0x187a, 0x187a, 0x23b3, 0x3d33, 0x486b, 0x529e, 0x391e, 
    0x25bd, 0x28cc, 0x3b28, 0x3405, 0x3e38, 0x4147, 0x3819, 0x391e, 
    0x391e, 0x89b3, 0x89b3, 0x89b3, 0xff00, 0xff00, 0xa333, 0x61eb, 
    0x7c70, 0x62f0, 0x529e, 0x723d, 0xa333, 0xd633, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xad66, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x1157, 0x125c, 0x125c, 0x187a, 0x156b, 0x187a, 0x2ff0, 0x1a85, 
    0x1a85, 0x2ff0, 0x64fa, 0x4351, 0x391e, 0x4351, 0x64fa, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    
    0x1157, 0x125c, 0x125c, 0x187a, 0x156b, 0x187a, 0x2ff0, 0x1a85, 
    0x1a85, 0x2ff0, 0x64fa, 0x4351, 0x391e, 0x4351, 0x64fa, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 
    0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00

  }

}, 
