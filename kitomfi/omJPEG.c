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

#include "masterhd.h"
#include <stdio.h>
#include <string.h>
#if PORT_COMP_MICROSOFT
#include <malloc.h>
#endif

#if PORT_INC_NEEDS_SYSTIME
#define	O_BINARY	0
#include <sys/time.h>
#include <sys/times.h>
#include <sys/param.h>
#endif

#if PORT_SYS_MAC
//#include <unix.h>
#include <time.h>
#include <stdlib.h>
#define INCLUDES_ARE_ANSI 1
#endif

#define OM_ENUM_VARS 1		/* defines variables for enumerated values to
				 * get addresses */

#include "omPublic.h"
#include "omJPEG.h"
#include "omMedia.h"
#include "omPvt.h"

omfUInt8           STD_QT[] =	/* From ISO JPEG spec, table 13.1.1, 13.1.2 */
{
	16, 11, 12, 14, 12, 10, 16, 14,
	13, 14, 18, 17, 16, 19, 24, 40,
	26, 24, 22, 22, 24, 49, 35, 37,
	29, 40, 58, 51, 61, 60, 57, 51,
	56, 55, 64, 72, 92, 78, 64, 68,
	87, 69, 55, 56, 80, 109, 81, 87,
	95, 98, 103, 104, 103, 62, 77, 113,
	121, 112, 100, 120, 92, 101, 103, 99,

	17, 18, 18, 24, 21, 24, 47, 26,
	26, 47, 99, 66, 56, 66, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,

	17, 18, 18, 24, 21, 24, 47, 26,
	26, 47, 99, 66, 56, 66, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
};

omfUInt8          *STD_QT_PTR[3];	/* pointers to components */


/* LF-W (12/18/96) - Supply AVR77 and AVR9s tables to get around Kansas bug */
omfUInt8 avr77Tables[] =	/* AVR77 */ 
{
  2, 1, 1, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 6, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4,
  4, 8, 7, 7, 10, 4, 4, 4,
  4, 4, 4, 5, 11, 7, 9, 14,
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
  12, 12, 12, 12, 12, 12, 12, 12,
};

omfUInt8 avr9sTables[] =
{
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
  12, 12, 12, 12, 12, 12, 12, 12,
};

omfUInt8 avr70Tables[] =
  { /* 8bit Quantization tables */
    5, 3, 4, 4, 4, 3, 5, 4, 
    4, 4, 5, 5, 5, 6, 7, 12, 
    8, 7, 7, 7, 7, 15, 11, 11, 
    9, 12, 17, 15, 18, 18, 17, 15, 
    17, 17, 19, 22, 28, 23, 19, 20, 
    26, 21, 17, 17, 24, 33, 24, 26, 
    29, 29, 31, 31, 31, 19, 23, 34, 
    36, 34, 30, 36, 28, 30, 31, 30, 
    
    5, 5, 5, 7, 6, 7, 14, 8, 
    8, 14, 30, 20, 17, 20, 30, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    
    5, 5, 5, 7, 6, 7, 14, 8, 
    8, 14, 30, 20, 17, 20, 30, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77, 
    77, 77, 77, 77, 77, 77, 77, 77
  };

#ifdef JPEG_QT_16

/****************************************************************************
 * 16-bit (128-byte) Quantization table for those applications 
 * which need them.  Values are the same as in the 8-bit table. 
 ***************************************************************************/

omfUInt16          STD_QT16[] =	/* From ISO JPEG spec, table 13.1.1, 13.1.2 */
{
	16, 11, 12, 14, 12, 10, 16, 14,
	13, 14, 18, 17, 16, 19, 24, 40,
	26, 24, 22, 22, 24, 49, 35, 37,
	29, 40, 58, 51, 61, 60, 57, 51,
	56, 55, 64, 72, 92, 78, 64, 68,
	87, 69, 55, 56, 80, 109, 81, 87,
	95, 98, 103, 104, 103, 62, 77, 113,
	121, 112, 100, 120, 92, 101, 103, 99,

	17, 18, 18, 24, 21, 24, 47, 26,
	26, 47, 99, 66, 56, 66, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,

	17, 18, 18, 24, 21, 24, 47, 26,
	26, 47, 99, 66, 56, 66, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
};

omfUInt8 avr75Tables[] =	/* AVR75 */ 
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
  };

omfUInt16         *STD_QT16_PTR[3];/* pointers to components */

#endif
/****************************************************************************
 * The standard DC tables have the following format : 
 *     o 16 bytes of "bits" indicating # of codes of length 0-15, followed by 
 *     o upto 17
 * bytes of "values" associated with those codes; in order of length 
 *     o
 * Note: we only have 12 
 * They are the same for chroma components 
 ***************************************************************************/

omfUInt8           STD_DCT[] =	/* From ISO JPEG spec, table 13.3.1.1,
								 * 13.3.1.2 */
{
	/* bits [0..15] - Luminance */
	0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* The values */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,

	/* bits [0..15] - chrominance_1 */
	0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* The values */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,

	/* bits [0..15] - chrominance_2 */
	0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* The values */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
};

omfUInt8          *STD_DCT_PTR[3];
omfInt32           STD_DCT_LEN[3];

/****************************************************************************
 * The standard AC tables have the following format : 
 *     o 16 bytes of "bits" indicating # of codes of length 0-15, followed by 
 *     o upto 256
 * bytes of "values" associated with those codes; in order of length 
 *     o
 * Note: we only have 162 ( total = 178 ) 
 * They are the same for each chroma component 
 ****************************************************************************/

omfUInt8           STD_ACT[] =	/* From ISO JPEG spec, table 13.3.2.1,
				 * 13.3.2.2 */
{
	/* Luminance bits */
	0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d,
	/* values */
	0x01, 0x02, 0x03, 0x00, 0x04,
	0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81,
	0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82,
	0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36,
	0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56,
	0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76,
	0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95,
	0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3,
	0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca,
	0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa,

	/* Chrominance_1 */
	0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
	/* values */
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61,
	0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52,
	0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a,
	0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86,
	0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4,
	0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2,
	0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa,

	/* Chrominance_2 */
	0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
	/* values */
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61,
	0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52,
	0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a,
	0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86,
	0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4,
	0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2,
	0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa,
};

omfUInt8          *STD_ACT_PTR[3];
omfInt32           STD_ACT_LEN[3];

/*
 * Only define JPEG software codec functions if specifically enabled. This
 * will allow audio-only applications to be a little smaller
 */

#ifdef OMFI_JPEG_CODEC


#include "jinclude.h"
#include "omcTIFF.h"

/*
 * <setjmp.h> is used for the optional error recovery mechanism shown in the
 * second part of the example.
 */

#include <setjmp.h>

/* These static variables are needed by the error routines. */
static jmp_buf  setjmp_buffer;	/* for return to caller */
static external_methods_ptr emethods;	/* needed for access to msg_parm */

static int      visited = 0;

static char            OMJPEGErrorString[512];
static int             OMJPEGErrorRaised = 0;
static omfErr_t        localJPEGStatus =	OM_ERR_NONE;

#if  PORT_MEM_DOS16
#define PIXEL unsigned char huge
#else
#define PIXEL unsigned char
#endif

METHODDEF void  jselromfi(decompress_info_ptr cinfo, omfCompatVideoCompr_t);
METHODDEF void  trace_message(const char *msgtext);
METHODDEF void  error_exit(const char *msgtext);
METHODDEF void  input_init(compress_info_ptr cinfo);
METHODDEF void  get_input_row(compress_info_ptr cinfo, JSAMPARRAY pixel_row);
METHODDEF void  input_term(compress_info_ptr cinfo);
METHODDEF void  c_ui_method_selection(compress_info_ptr cinfo);
METHODDEF void  output_init(decompress_info_ptr dinfo);
METHODDEF void  put_color_map(decompress_info_ptr dinfo, int num_colors, JSAMPARRAY colormap);
METHODDEF void  put_pixel_rows(decompress_info_ptr dinfo, int num_rows, JSAMPIMAGE pixel_data);
METHODDEF void  output_term(decompress_info_ptr dinfo);
METHODDEF void  d_ui_method_selection(decompress_info_ptr dinfo);
METHODDEF void  omjpeg_write_file_header(compress_info_ptr cinfo);
METHODDEF void  omjpeg_write_scan_header(compress_info_ptr cinfo);
METHODDEF void  omjpeg_write_jpeg_data(compress_info_ptr cinfo, char *dataptr, int datacount);
METHODDEF void  omjpeg_write_scan_trailer(compress_info_ptr cinfo);
METHODDEF void  omjpeg_write_file_trailer(compress_info_ptr cinfo);
METHODDEF void  jselwomfi(compress_info_ptr cinfo);
METHODDEF int   omjpeg_read_jpeg_data(decompress_info_ptr dinfo);
METHODDEF void  omjpeg_read_file_header(decompress_info_ptr dinfo);
METHODDEF boolean omjpeg_read_scan_header(decompress_info_ptr dinfo);
METHODDEF void  omjpeg_resync_to_restart(decompress_info_ptr dinfo, int marker);
METHODDEF void  omjpeg_read_scan_trailer(decompress_info_ptr dinfo);
METHODDEF void  omjpeg_read_file_trailer(decompress_info_ptr dinfo);
#endif

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t        omfsJPEGInit(void)
{

	STD_QT_PTR[0] = STD_QT;
	STD_QT_PTR[1] = STD_QT + 64;
	STD_QT_PTR[2] = STD_QT + 128;	/* pointers to components */

#ifdef JPEG_QT_16

	STD_QT16_PTR[0] = STD_QT16;
	STD_QT16_PTR[1] = STD_QT16 + 64;
	STD_QT16_PTR[2] = STD_QT16 + 128;	/* pointers to components */

#endif

	STD_DCT_PTR[0] = STD_DCT;
	STD_DCT_PTR[1] = STD_DCT + 28;
	STD_DCT_PTR[2] = STD_DCT + 56;

	STD_DCT_LEN[0] = 28;
	STD_DCT_LEN[1] = 28;
	STD_DCT_LEN[2] = 28;

	STD_ACT_PTR[0] = STD_ACT;
	STD_ACT_PTR[1] = STD_ACT + 178;
	STD_ACT_PTR[2] = STD_ACT + 356;

	STD_ACT_LEN[0] = 178;
	STD_ACT_LEN[1] = 178;
	STD_ACT_LEN[2] = 178;
	return (OM_ERR_NONE);
}


#ifdef OMFI_JPEG_CODEC

/*
 * These routines replace the default trace/error routines included with the
 * JPEG code.  The example trace_message routine shown here is actually the
 * same as the standard one, but you could modify it if you don't want
 * messages sent to stderr.  The example error_exit routine is set up to
 * return control to read_JPEG_file() rather than calling exit().  You can
 * use the same routines for both compression and decompression error
 * recovery.
 */

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * This routine is used for any and all trace, debug, or error printouts from
 * the JPEG code.  The parameter is a printf format string; up to 8 integer
 * data values for the format string have been stored in the msg_parm[] field
 * of the external_methods struct.
 */

METHODDEF void
                trace_message(const char *msgtext)
{
	sprintf(OMJPEGErrorString, msgtext,
		emethods->message_parm[0], emethods->message_parm[1],
		emethods->message_parm[2], emethods->message_parm[3],
		emethods->message_parm[4], emethods->message_parm[5],
		emethods->message_parm[6], emethods->message_parm[7]);

#ifdef JPEG_TRACE
	fprintf(stderr, "%s\n", OMJPEGErrorString);	/* there is no \n in the
							 * format string! */
#endif
}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * The error_exit() routine should not return to its caller.  The default
 * routine calls exit(), but here we assume that we want to return to
 * read_JPEG_file, which has set up a setjmp context for the purpose. You
 * should make sure that the free_all method is called, either within
 * error_exit or after the return to the outer-level routine.
 */

METHODDEF void
                error_exit(const char *msgtext)
{
	trace_message(msgtext);	/* report the error message */
	(*emethods->free_all) ();	/* clean up memory allocation & temp
					 * files */

	/*
	 * Set OMFI error code to signal a JPEG failure, unless set
	 * previously
	 */
	if (localJPEGStatus == OM_ERR_NONE)
		localJPEGStatus = OM_ERR_JPEGPROBLEM;

	OMJPEGErrorRaised = 1;	/* indicate that the message string is an
				 * error message */

	longjmp(setjmp_buffer, 1);	/* return control to outer routine */
}

typedef enum
{				/* JPEG marker codes */
	M_SOF0 = 0xc0,
	M_SOF1 = 0xc1,
	M_SOF2 = 0xc2,
	M_SOF3 = 0xc3,

	M_SOF5 = 0xc5,
	M_SOF6 = 0xc6,
	M_SOF7 = 0xc7,

	M_JPG = 0xc8,
	M_SOF9 = 0xc9,
	M_SOF10 = 0xca,
	M_SOF11 = 0xcb,

	M_SOF13 = 0xcd,
	M_SOF14 = 0xce,
	M_SOF15 = 0xcf,

	M_DHT = 0xc4,

	M_DAC = 0xcc,

	M_RST0 = 0xd0,
	M_RST1 = 0xd1,
	M_RST2 = 0xd2,
	M_RST3 = 0xd3,
	M_RST4 = 0xd4,
	M_RST5 = 0xd5,
	M_RST6 = 0xd6,
	M_RST7 = 0xd7,

	M_SOI = 0xd8,
	M_EOI = 0xd9,
	M_SOS = 0xda,
	M_DQT = 0xdb,
	M_DNL = 0xdc,
	M_DRI = 0xdd,
	M_DHP = 0xde,
	M_EXP = 0xdf,

	M_APP0 = 0xe0,
	M_APP15 = 0xef,

	M_JPG0 = 0xf0,
	M_JPG13 = 0xfd,
	M_COM = 0xfe,

	M_TEM = 0x01,

	M_ERROR = 0x100
} JPEG_MARKER;

/******************** JPEG COMPRESSION SAMPLE INTERFACE *******************/

/*
 * This half of the example shows how to feed data into the JPEG compressor.
 * We present a minimal version that does not worry about refinements such as
 * error recovery (the JPEG code will just exit() if it gets an error).
 */


/*
 * To supply the image data for compression, you must define three routines
 * input_init, get_input_row, and input_term.  These routines will be called
 * from the JPEG compressor via function pointer values that you store in the
 * cinfo data structure; hence they need not be globally visible and the
 * exact names don't matter.  (In fact, the "METHODDEF" macro expands to
 * "static" if you use the unmodified JPEG include files.)
 * 
 * The input file reading modules (jrdppm.c, jrdgif.c, jrdtarga.c, etc) may be
 * useful examples of what these routines should actually do, although each
 * of them is encrusted with a lot of specialized code for its own file
 * format.
 */


/************************
 * input_init
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
METHODDEF void
                input_init(compress_info_ptr cinfo)
/* Initialize for input; return image size and component data. */
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
	omfMediaHdl_t		media = parms->media;
	omfInt16      		bitsPerPixel /*, numFields Not needed ---ROGER */;
	omfInt32 			fieldHeight, fieldWidth;
	omfFrameLayout_t	layout;
	omfPixelFormat_t	memFmt;
	omfErr_t			functionJPEGStatus =	OM_ERR_NONE;

	functionJPEGStatus = omfmGetVideoInfo(media, &layout, 
										&fieldWidth, &fieldHeight, 
										NULL, &bitsPerPixel, &memFmt);
										
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		return;
	}
	
	/*
	 * This routine must return five pieces of information about the
	 * incoming image, and must do any SETUP needed for the get_input_row
	 * routine. The image information is returned in fields of the cinfo
	 * struct. (If you don't care about modularity, you could initialize
	 * these fields in the main JPEG calling routine, and make this
	 * routine be a no-op.) We show some example values here.
	 */
	cinfo->image_width = fieldWidth;	/* width in pixels */
	
	cinfo->image_height = fieldHeight;	/* height in pixels */
	/*
	 * JPEG views an image as being a rectangular array of pixels, with
	 * each pixel having the same number of "component" values (color
	 * channels). You must specify how many components there are and the
	 * colorspace interpretation of the components.  Most applications
	 * will use RGB data or grayscale data.  If you want to use something
	 * else, you'll need to study and perhaps modify jcdeflts.c,
	 * jccolor.c, and jdcolor.c.
	 */
	cinfo->input_components = 3;	/* or 1 for grayscale */
	if(media->pvt->pixType == kOmfPixRGBA)
	  	cinfo->in_color_space = CS_RGB;
	else
	  	cinfo->in_color_space = CS_YCbCr;	/* or CS_GRAYSCALE for grayscale */
	cinfo->data_precision = 8;	/* bits per pixel component value */
		
	if (cinfo->CCIR)
	{
		int  i;
		cinfo->CCIRLumaInMap = (JSAMPLE *) (*cinfo->emethods->alloc_small) (256 * SIZEOF(JCOEF));
		for (i = 0; i < 256; i++)
			cinfo->CCIRLumaInMap[i] = ((i * 220) / 256) + 16;
		
		cinfo->CCIRChromaInMap = (JSAMPLE *) (*cinfo->emethods->alloc_small) (256 * SIZEOF(JCOEF));
		for (i = 0; i < 256; i++)
			cinfo->CCIRChromaInMap[i] = ((i * 225) / 256) + 16;
	}

	/*
	 * In the current JPEG software, data_precision must be set equal to
	 * BITS_IN_JSAMPLE, which is 8 unless you twiddle jconfig.h.  Future
	 * versions might allow you to say either 8 or 12 if compiled with
	 * 12-bit JSAMPLEs, or up to 16 in lossless mode.  In any case, it is
	 * up to you to scale incoming pixel values to the range 0 ..
	 * (1<<data_precision)-1. If your image data format is fixed at a
	 * byte per component, then saying "8" is probably the best int -term
	 * solution.
	 */

	cinfo->write_JFIF_header = FALSE;

}


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * This function is called repeatedly and must supply the next row of pixels
 * on each call.  The rows MUST be returned in top-to-bottom order if you
 * want your JPEG files to be compatible with everyone else's.  (If you
 * cannot readily read your data in that order, you'll need an intermediate
 * array to hold the image.  See jrdtarga.c or jrdrle.c for examples of
 * handling bottom-to-top source data using the JPEG code's portable
 * mechanisms.) The data is to be returned into a 2-D array of JSAMPLEs,
 * indexed as JSAMPLE pixel_row[component][column] where component runs from
 * 0 to cinfo->input_components-1, and column runs from 0 to
 * cinfo->image_width-1 (column 0 is left edge of image).  Note that this is
 * actually an array of pointers to arrays rather than a true 2D array, since
 * C does not support variable-size multidimensional arrays. JSAMPLE is
 * typically typedef'd as "unsigned char".
 */


METHODDEF void
                get_input_row(compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* Read next row of pixels into pixel_row[][] */
{
	/*
	 * This example shows how you might read RGB data (3 components) from
	 * an input file in which the data is stored 3 bytes per pixel in
	 * left-to-right, top-to-bottom order.
	 */
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
	register JSAMPROW ptr0, ptr1, ptr2;
	register int    col;

	ptr0 = pixel_row[0];
	ptr1 = pixel_row[1];
	ptr2 = pixel_row[2];
	for (col = 0; col < cinfo->image_width; col++)
	{
		*ptr0++ = (JSAMPLE) ((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++];	/* red */
		*ptr1++ = (JSAMPLE) ((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++];	/* green */
		*ptr2++ = (JSAMPLE) ((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++];	/* blue */
	}
	
	ptr0 = pixel_row[0];		/* DEBUG: Keep these values around */
	ptr1 = pixel_row[1];
	ptr2 = pixel_row[2];
	
}


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
METHODDEF void
                input_term(compress_info_ptr cinfo)
/* Finish up at the end of the input */
{
	/* This termination routine will very often have no work to do, */
	/* but you must provide it anyway. */
	/* Note that the JPEG code will only call it during successful exit; */
	/*
	 * if you want it called during error exit, you gotta do that
	 * yourself.
	 */
}


/*
 * That's it for the routines that deal with reading the input image data.
 * Now we have overall control and parameter selection routines.
 */


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * This routine must determine what output JPEG file format is to be written,
 * and make any other compression parameter changes that are desirable. This
 * routine gets control after the input file header has been read (i.e.,
 * right after input_init has been called).  You could combine its functions
 * into input_init, or even into the main control routine, but if you have
 * several different input_init routines, it's a definite win to keep this
 * separate.  You MUST supply this routine even if it's a no-op.
 */

METHODDEF void
                c_ui_method_selection(compress_info_ptr cinfo)
{
	/* For now, always select OMFI-JPEG output format. */
	jselwomfi(cinfo);
}





/******************** JPEG DECOMPRESSION SAMPLE INTERFACE *******************/

/*
 * This half of the example shows how to read data from the JPEG
 * decompressor. It's a little more refined than the above in that we show
 * how to do your own error recovery.  If you don't care about that, you
 * don't need these next two routines.
 */


/*
 * To accept the image data from decompression, you must define four routines
 * output_init, put_color_map, put_pixel_rows, and output_term.
 * 
 * You must understand the distinction between full color output mode (N
 * independent color components) and colormapped output mode (a single output
 * component representing an index into a color map).  You should use
 * colormapped mode to write to a colormapped display screen or output file.
 * Colormapped mode is also useful for reducing grayscale output to a small
 * number of gray levels: when using the 1-pass quantizer on grayscale data,
 * the colormap entries will be evenly spaced from 0 to MAX_JSAMPLE, so you
 * can regard the indexes are directly representing gray levels at reduced
 * precision.  In any other case, you should not depend on the colormap
 * entries having any particular order. To get colormapped output, set
 * cinfo->quantize_colors to TRUE and set cinfo->desired_number_of_colors to
 * the maximum number of entries in the colormap.  This can be done either in
 * your main routine or in d_ui_method_selection.  For grayscale
 * quantization, also set cinfo->two_pass_quantize to FALSE to ensure the
 * 1-pass quantizer is used (presently this is the default, but it may not be
 * so in the future).
 * 
 * The output file writing modules (jwrppm.c, jwrgif.c, jwrtarga.c, etc) may be
 * useful examples of what these routines should actually do, although each
 * of them is encrusted with a lot of specialized code for its own file
 * format.
 */


METHODDEF void
                output_init(decompress_info_ptr dinfo)
/* This routine should do any SETUP required */
{
	/*
	 * This routine can initialize for output based on the data passed in
	 * dinfo. Useful fields include: image_width, image_height
	 * Pretty obvious, I hope. data_precision			bits
	 * per pixel value; typically 8. out_color_space
	 * utput colorspace previously requested color_out_comps
	 * umber of color components in same final_out_comps
	 * umber of components actually output final_out_comps is 1 if
	 * quantize_colors is true, else it is equal to color_out_comps.
	 * 
	 * If you have requested color quantization, the colormap is NOT yet
	 * set. You may wish to defer output initialization until
	 * put_color_map is called.
	 */

}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * This routine is called if and only if you have set dinfo->quantize_colors
 * to TRUE.  It is given the selected colormap and can complete any required
 * initialization.  This call will occur after output_init and before any
 * calls to put_pixel_rows.  Note that the colormap pointer is also placed in
 * a dinfo field, whence it can be used by put_pixel_rows or output_term.
 * num_colors will be less than or equal to desired_number_of_colors.
 * 
 * The colormap data is supplied as a 2-D array of JSAMPLEs, indexed as JSAMPLE
 * colormap[component][indexvalue] where component runs from 0 to
 * dinfo->color_out_comps-1, and indexvalue runs from 0 to num_colors-1.
 * Note that this is actually an array of pointers to arrays rather than a
 * true 2D array, since C does not support variable-size multidimensional
 * arrays. JSAMPLE is typically typedef'd as "unsigned char".  If you want
 * your code to be as portable as the JPEG code proper, you should always
 * access JSAMPLE values with the GETJSAMPLE() macro, which will do the right
 * thing if the machine has only signed chars.
 */

METHODDEF void
                put_color_map(decompress_info_ptr dinfo, int num_colors, JSAMPARRAY colormap)
/* Write the color map */
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)dinfo->input_file;
	omfMediaHdl_t		media = parms->media;
	/*
	 * You need not provide this routine if you always set
	 * dinfo->quantize_colors FALSE; but a safer practice is to provide
	 * it and have it just print an error message, like this:
	 * fprintf(stderr, "put_color_map called: there's a bug here
	 * somewhere!\n");
	 */
	XPROTECT(media->mainFile)
	{
		RAISE(OM_ERR_JPEGPCM);
	}
	XEXCEPT
	XEND_VOID
	
	return;
}


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * This function is called repeatedly, with a few more rows of pixels
 * supplied on each call.  With the current JPEG code, some multiple of 8
 * rows will be passed on each call except the last, but it is extremely bad
 * form to depend on this.  You CAN assume num_rows > 0. The data is supplied
 * in top-to-bottom row order (the standard order within a JPEG file).  If
 * you cannot readily use the data in that order, you'll need an intermediate
 * array to hold the image.  See jwrrle.c for an example of outputting data
 * in bottom-to-top order.
 * 
 * The data is supplied as a 3-D array of JSAMPLEs, indexed as JSAMPLE
 * pixel_data[component][row][column] where component runs from 0 to
 * dinfo->final_out_comps-1, row runs from 0 to num_rows-1, and column runs
 * from 0 to dinfo->image_width-1 (column 0 is left edge of image).  Note
 * that this is actually an array of pointers to pointers to arrays rather
 * than a true 3D array, since C does not support variable-size
 * multidimensional arrays. JSAMPLE is typically typedef'd as "unsigned
 * char".  If you want your code to be as portable as the JPEG code proper,
 * you should always access JSAMPLE values with the GETJSAMPLE() macro, which
 * will do the right thing if the machine has only signed chars.
 * 
 * If quantize_colors is true, then there is only one component, and its values
 * are indexes into the previously supplied colormap.  Otherwise the values
 * are actual data in your selected output colorspace.
 */

static int      numPixDisplay = 0;

METHODDEF void
                put_pixel_rows(decompress_info_ptr dinfo, int num_rows, JSAMPIMAGE pixel_data)
/* Write some rows of output data */
{
	/*
	 * This example shows how you might write full-color RGB data (3
	 * components) to an output file in which the data is stored 3 bytes
	 * per pixel.
	 */
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)dinfo->input_file;
	register JSAMPROW ptr0, ptr1, ptr2;
	register int    col;
	register int    row;

	for (row = 0; row < num_rows; row++)
	{
		ptr0 = pixel_data[0][row];
		ptr1 = pixel_data[1][row];
		ptr2 = pixel_data[2][row];
		for (col = 0; col < dinfo->image_width; col++)
		{

			((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++] = GETJSAMPLE(*ptr0);	/* red */
			ptr0++;

			((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++] = GETJSAMPLE(*ptr1);	/* green */
			ptr1++;

			((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++] = GETJSAMPLE(*ptr2);	/* blue */
			ptr2++;

		}
	}
}


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
METHODDEF void
                output_term(decompress_info_ptr dinfo)
/* Finish up at the end of the output */
{
	/* This termination routine may not need to do anything. */
	/* Note that the JPEG code will only call it during successful exit; */
	/*
	 * if you want it called during error exit, you gotta do that
	 * yourself.
	 */
}


/*
 * That's it for the routines that deal with writing the output image. Now we
 * have overall control and parameter selection routines.
 */


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * This routine gets control after the JPEG file header has been read; at
 * this point the image size and colorspace are known. The routine must
 * determine what output routines are to be used, and make any decompression
 * parameter changes that are desirable.  For example, if it is found that
 * the JPEG file is grayscale, you might want to do things differently than
 * if it is color.  You can also delay setting quantize_colors and associated
 * options until this point.
 * 
 * j_d_defaults initializes out_color_space to CS_RGB.  If you want grayscale
 * output you should set out_color_space to CS_GRAYSCALE.  Note that you can
 * force grayscale output from a color JPEG file (though not vice versa).
 */

METHODDEF void
                d_ui_method_selection(decompress_info_ptr dinfo)
{
}


/* Write a single byte */
#define emit_byte(cinfo,x) \
   { unsigned char c = (unsigned char)x; cinfo->methods->write_jpeg_data(cinfo, (char *)&c, 1);}

LOCAL void
                emit_marker(compress_info_ptr cinfo, JPEG_MARKER mark)
/* Emit a marker code */
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
	omfMediaHdl_t 		media = parms->media;
	/* if end of field, leave at a 4-byte boundary */
	if ((M_RST0 <= mark) && mark <= M_RST7)
	{
		omfErr_t functionJPEGStatus =	OM_ERR_NONE;
		omfUInt32 offset;
		omfInt32 remainder;
		omfInt32 i;
		unsigned char ff = 0xff, marker=(unsigned char)mark;
		
		if (localJPEGStatus != OM_ERR_NONE)
			error_exit("Problem in emit_marker.  File omJPEG.c line 1118");

		functionJPEGStatus =  omcGetStreamPos32(media->stream, &offset);
		if (functionJPEGStatus !=	OM_ERR_NONE)
		{
			if  (localJPEGStatus == OM_ERR_NONE)
				localJPEGStatus = functionJPEGStatus;
			error_exit("Bad alignment check after compression");
		}
		
		remainder = (offset+2) % 4;
		if (remainder)
		{
			for (i = 0; i < (4-remainder); i++)
			{
				functionJPEGStatus = omcWriteStream(media->stream, 1, &ff);
				if (functionJPEGStatus !=	OM_ERR_NONE)
				{
					if  (localJPEGStatus == OM_ERR_NONE)
						localJPEGStatus = functionJPEGStatus;
					error_exit("Bad alignment padding after compression");
				}
			}
		}
		
		functionJPEGStatus = omcWriteStream(media->stream, 1, &ff);
		if (functionJPEGStatus !=	OM_ERR_NONE)
		{
			if  (localJPEGStatus == OM_ERR_NONE)
				localJPEGStatus = functionJPEGStatus;
			error_exit("Bad alignment padding after compression");
		}
		
		functionJPEGStatus = omcWriteStream(media->stream, 1, &marker);
		if (functionJPEGStatus !=	OM_ERR_NONE)
		{
			if  (localJPEGStatus == OM_ERR_NONE)
				localJPEGStatus = functionJPEGStatus;
			error_exit("Bad alignment padding after compression");
		}
	}
	else
	{
		emit_byte(cinfo, 0xFF);
		emit_byte(cinfo, mark);
	}
}


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * Write the file header.
 */


METHODDEF void
                omjpeg_write_file_header(compress_info_ptr cinfo)
{
}


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * Write the start of a scan (everything through the SOS marker).
 */

METHODDEF void
                omjpeg_write_scan_header(compress_info_ptr cinfo)
{
}


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * Write some bytes of compressed data within a scan.
 */

METHODDEF void
                omjpeg_write_jpeg_data(compress_info_ptr cinfo, char *dataptr, int datacount)
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
	omfMediaHdl_t   	media = parms->media;

	if (localJPEGStatus == OM_ERR_NONE)
	{ /* optimize for if we have a problem, like a disk full */
		localJPEGStatus = omcWriteStream(media->stream, datacount, dataptr);
	}
}


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * Finish up after a compressed scan (series of write_jpeg_data calls).
 */

METHODDEF void
                omjpeg_write_scan_trailer(compress_info_ptr cinfo)
{
	/* no work needed in this format */
}


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * Finish up at the end of the file.
 */

METHODDEF void
                omjpeg_write_file_trailer(compress_info_ptr cinfo)
{
	emit_marker(cinfo, M_RST1);
}


/*
/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
 /* The method selection routine for standard JPEG header writing. This should
 * be called from c_ui_method_selection if appropriate.
 */

METHODDEF void
                jselwomfi(compress_info_ptr cinfo)
{
	cinfo->methods->write_file_header = omjpeg_write_file_header;
	cinfo->methods->write_scan_header = omjpeg_write_scan_header;
	cinfo->methods->write_jpeg_data = omjpeg_write_jpeg_data;
	cinfo->methods->write_scan_trailer = omjpeg_write_scan_trailer;
	cinfo->methods->write_file_trailer = omjpeg_write_file_trailer;
}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
METHODDEF int
                omjpeg_read_jpeg_data(decompress_info_ptr dinfo)
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)dinfo->input_file;
	omfMediaHdl_t		media = parms->media;
	omfUInt32          bytesRead, toRead;
	omfInt64		length, offset;
	omfErr_t			functionJPEGStatus =	OM_ERR_NONE;
	
	dinfo->next_input_byte = dinfo->input_buffer + MIN_UNGET;

	functionJPEGStatus = omcGetLength(media->stream, &length);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		error_exit("Problem with calling omcGetLength in omJPEG.c -- line 1340");
	}

	functionJPEGStatus = omcGetStreamPosition(media->stream, &offset);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		error_exit("Problem with calling omcGetStreamPosition in omJPEG.c -- line 1348");
	}

	functionJPEGStatus = omfsSubInt64fromInt64(offset, &length);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		error_exit("Problem with calling omfsSubInt64fromInt64 in omJPEG.c -- line 1356");
	}

	functionJPEGStatus = omfsTruncInt64toUInt32(length, &toRead);	/* OK MAXREAD */
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		error_exit("Problem with calling omfsTruncInt64toUInt32 in omJPEG.c -- line 1364");
	}
	if(toRead > JPEG_BUF_SIZE)
		toRead = JPEG_BUF_SIZE;

	functionJPEGStatus = omcReadStream(media->stream, toRead, (dinfo->next_input_byte), &bytesRead);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		error_exit("Problem with calling omcReadStream in omJPEG.c -- line 1374");
	}

	dinfo->bytes_in_buffer = bytesRead;

	if (dinfo->bytes_in_buffer <= 0)
	{
		WARNMS(dinfo->emethods, "Premature EOF in JPEG file");
		dinfo->next_input_byte[0] = (unsigned char) 0xFF;
		dinfo->next_input_byte[1] = (unsigned char) M_EOI;
		dinfo->bytes_in_buffer = 2;
	}
	return JGETC(dinfo);
}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
METHODDEF int
                omjpeg_read_LSIjpeg_data(decompress_info_ptr dinfo)
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)dinfo->input_file;
	omfMediaHdl_t		media = parms->media;
	omfUInt32       bytesRead, toRead, srcBytes, destBytes, n;
	char			*src, *dest;
	omfInt64		length, offset;
	char			rdBuf[JPEG_BUF_SIZE], t;
	omfErr_t		functionJPEGStatus =	OM_ERR_NONE;
	
	/* Note that the jpeg field data is formatted as follows:
	 *   
     *  4    bytes 			Bitstream length  in bytes
     * 						(Intel order, exclusive of headers and Q tabs)
     *  1    byte            Q table ID (for MCX, 00 denotes Y, 01 = Cx)
     * 64    bytes           Q table contents
     *  1    byte            Q table ID
     * 64    bytes           Q table
     *  2    bytes           padding
     * ??    bytes           mis-ordered JPEG bitstream
  	 *
	 * Bitstream complications include:
  	 *
     * 	1)	Bitstream is stored padded to 4-byte alignment.
     *
     * 	2)	Bitstream is stored as if it were 4-byte integers with Intel byte, 
     * 		ordering.
     *
     *	3) 	Bitstream contains no marker codes.  Every 'FF' found in the bitstream
     *		data must be followed by an '00' to indicate to our code that it is
     *		a valid 'FF' data value.
     *
     *	4)	Related to #3 above, a 0xFFD4 must be added when the Bitstream length
     *		(in bytes) is reached.
  	 */

		dinfo->next_input_byte = dinfo->input_buffer + MIN_UNGET;

	functionJPEGStatus = omcGetLength(media->stream, &length);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		error_exit("Problem with calling omcGetLength in omJPEG.c -- line 1447");
	}

	functionJPEGStatus = omcGetStreamPosition(media->stream, &offset);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		error_exit("Problem with calling omcGetStreamPosition in omJPEG.c -- line 1455");
	}

	functionJPEGStatus = omfsSubInt64fromInt64(offset, &length);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		error_exit("Problem with calling omfsSubInt64fromInt64 in omJPEG.c -- line 1463");
	}

	functionJPEGStatus = omfsTruncInt64toUInt32(length, &toRead);	/* OK MAXREAD */
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		error_exit("Problem with calling omfsTruncInt64toUInt32 in omJPEG.c -- line 1471");
	}

	if(toRead > parms->LSIbytesRemaining)
		toRead = parms->LSIbytesRemaining;
	if(toRead > JPEG_BUF_SIZE/2)
		toRead = JPEG_BUF_SIZE/2;

	functionJPEGStatus = omcReadStream(media->stream, toRead, rdBuf, &bytesRead);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		error_exit("Problem with calling omcReadStream in omJPEG.c -- line 1484");
	}

	for(n = 0; n < bytesRead; n += 4)
	{
		t = rdBuf[n+3];
		rdBuf[n+3] = rdBuf[n];
		rdBuf[n] = t;
		t = rdBuf[n+2];
		rdBuf[n+2] = rdBuf[n+1];
		rdBuf[n+1] = t;
	}
	parms->LSIbytesRemaining -= bytesRead;
	
	src = rdBuf;
	dest = dinfo->next_input_byte;
	srcBytes = 0;
	destBytes = 0;
	while(srcBytes < bytesRead)
	{
		if((omfUInt8)(*src) == 0xFF)
		{
			*dest++ = *src++;
			*dest++ = 0x00;
			destBytes += 2;
		}
		else
		{
			*dest++ = *src++;
			destBytes++;
		}
		srcBytes++;
	}
	if(parms->LSIbytesRemaining == 0)
	{
		*dest++ = (char)0xFF;
		*dest++ = (char)0xD4;
		destBytes += 2;
	}
	
	
	dinfo->bytes_in_buffer = destBytes;
	if (dinfo->bytes_in_buffer <= 0)
	{
		WARNMS(dinfo->emethods, "Premature EOF in JPEG file");
		dinfo->next_input_byte[0] = (unsigned char) 0xFF;
		dinfo->next_input_byte[1] = (unsigned char) M_EOI;
		dinfo->bytes_in_buffer = 2;
	}
	return JGETC(dinfo);
}

/************************
 * loadCompressionTable
 *
 * 		copies the compression tables from user space to JPEG space
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Set module static error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * Initialize and read the file header (everything through the SOF marker).
 */

static void loadCompressionTable(omfMediaHdl_t media, jpeg_tables * table, omfInt32 n)
{
	omfJPEGTables_t aTable;
	omfInt16 m;
	omfJPEGcomponent_t regComponent;
#if ! STANDARD_JPEG_Q
	omfJPEGcomponent_t extendComponent; /* RPS 4/8/96 FP16 extension */
	omfBool extendedQTables = FALSE;
#endif
	omfErr_t		functionJPEGStatus =	OM_ERR_NONE;

	/* RPS 4/8/96 don't use n directly, translate! */
	if (n == 0)
	{
		regComponent = kJcLuminance;
#if ! STANDARD_JPEG_Q
		extendComponent = kJcLuminanceFP16;
#endif
	}
	else
	{
		regComponent = kJcChrominance;
#if ! STANDARD_JPEG_Q
		extendComponent = kJcChrominanceFP16;
#endif
	}
	
#if ! STANDARD_JPEG_Q
	/* First, see if an FP16 table exists, otherwise, try for the normal 8-bit Q tables */
	aTable.JPEGcomp = extendComponent;
	
	functionJPEGStatus = codecGetInfo(media, kJPEGTables, media->pictureKind, sizeof(omfJPEGTables_t), &aTable);
/* EVEN if there is an error, do not set localJPEGStatus, first check the other tables! */
	if (functionJPEGStatus != OM_ERR_NONE || aTable.QTableSize == 0)
	{
#endif
		aTable.JPEGcomp = regComponent;

		functionJPEGStatus = codecGetInfo(media, kJPEGTables, media->pictureKind, sizeof(omfJPEGTables_t), &aTable);
		if (functionJPEGStatus !=	OM_ERR_NONE)
		{
			if  (localJPEGStatus == OM_ERR_NONE)
				localJPEGStatus = functionJPEGStatus;
			return;
		}

#if ! STANDARD_JPEG_Q
	}
	
	if (aTable.QTableSize == 128)
	{
		aTable.QTableSize = 64;
		extendedQTables = TRUE;
	}
#endif
		
	table->Qlen = aTable.QTableSize;
	table->Q16len = aTable.QTableSize;

#if ! STANDARD_JPEG_Q
	if (extendedQTables)
		table->Q16FPlen = aTable.QTableSize;
	else
		table->Q16FPlen = 0;
#endif

	table->DClen = aTable.DCTableSize;
	table->AClen = aTable.DCTableSize;
	
#if ! STANDARD_JPEG_Q
	if (extendedQTables)
	{
		omfUInt16 tval, rval, rem;
		
		for(m = 0; m < aTable.QTableSize; m++)
		{
			tval = ((omfUInt16 *)(aTable.QTables))[m];
			rval = tval >> 7;
			rem = rval & 1;
			rval = (rval >> 1) + rem; /* round the value, don't simply trunc */
	  		table->Q16FP[m] = tval;
	  		table->Q[m] = (omfUInt8)rval;
	  		table->Q16[m] = rval;
	  	}
	}
	else
#endif
	{
		memcpy(table->Q, aTable.QTables, aTable.QTableSize);
		for(m = 0; m < aTable.QTableSize; m++)
		  table->Q16[m] = aTable.QTables[m];
	}
	memcpy(table->DC, aTable.DCTables, aTable.DCTableSize);
	memcpy(table->AC, aTable.ACTables, aTable.ACTableSize);
	omOptFree(media->mainFile, aTable.QTables);
	omOptFree(media->mainFile, aTable.DCTables);
	omOptFree(media->mainFile, aTable.ACTables);
}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
METHODDEF void
                omjpeg_read_file_header(decompress_info_ptr dinfo)
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)dinfo->input_file;
	omfMediaHdl_t		media = parms->media;
 	omfTIFF_JPEGInfo_t   info;
	omfInt16           bitsPerPixel, n, numFields;
	omfInt32 fieldHeight, fieldWidth, tmpLen;
	omfFrameLayout_t layout;
	omfPixelFormat_t memFmt;
	jpeg_tables     compressionTables[3];
	omfUInt32		bytesRead;
	omfUInt8		tmpChar, tmpFill[128];
	static omfInt32		numFiller = 2;
	
	jpeg_component_info *compptr;
	int             i, count;
	short           ci;
	UINT8           bits[17];
	UINT8           huffval[256];
	HUFF_TBL      **htblptr;
	QUANT_TBL_PTR   quant_ptr;
	omfErr_t		functionJPEGStatus =	OM_ERR_NONE;
	
	memset(huffval, 0, 256);  /* initialize memset to 0 */

	functionJPEGStatus = omfmGetVideoInfo(media, &layout, &fieldWidth, 
										&fieldHeight, NULL, &bitsPerPixel, &memFmt);

	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		return;
	}

	/* RPS 3.29.96 call the codec to get the table ID.  If Avid, and studio, specify
	   CCIR translation */
	functionJPEGStatus = codecGetInfo(media, kCompressionParms, 
									media->pictureKind, 
									sizeof(omfTIFF_JPEGInfo_t), &info);

	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
		return;
	}
		
	if(info.compression == kLSIJPEG)
	{
		functionJPEGStatus = omcReadStream(media->stream, 4, &tmpLen, &bytesRead);
		if (functionJPEGStatus !=	OM_ERR_NONE)
		{
			if  (localJPEGStatus == OM_ERR_NONE)
				localJPEGStatus = functionJPEGStatus;
			return;
		}
#if PORT_BYTESEX_BIG_ENDIAN
		omfsFixLong(&tmpLen);
#endif
		functionJPEGStatus = omcReadStream(media->stream, 1, &tmpChar, &bytesRead);
		if (functionJPEGStatus !=	OM_ERR_NONE)
		{
			if  (localJPEGStatus == OM_ERR_NONE)
				localJPEGStatus = functionJPEGStatus;
			return;
		}
		functionJPEGStatus = omcReadStream(media->stream, 64, compressionTables[0].Q, &bytesRead);
		if (functionJPEGStatus !=	OM_ERR_NONE)
		{
			if  (localJPEGStatus == OM_ERR_NONE)
				localJPEGStatus = functionJPEGStatus;
			return;
		}
		functionJPEGStatus = omcReadStream(media->stream, 1, &tmpChar, &bytesRead);
		if (functionJPEGStatus !=	OM_ERR_NONE)
		{
			if  (localJPEGStatus == OM_ERR_NONE)
				localJPEGStatus = functionJPEGStatus;
			return;
		}
		functionJPEGStatus = omcReadStream(media->stream, 64, compressionTables[1].Q, &bytesRead);
		if (functionJPEGStatus !=	OM_ERR_NONE)
		{
			if  (localJPEGStatus == OM_ERR_NONE)
				localJPEGStatus = functionJPEGStatus;
			return;
		}
		if(numFiller != 0)
		{
			functionJPEGStatus = omcReadStream(media->stream, numFiller, tmpFill, &bytesRead);
			if (functionJPEGStatus !=	OM_ERR_NONE)
			{
				if  (localJPEGStatus == OM_ERR_NONE)
					localJPEGStatus = functionJPEGStatus;
				return;
			}
		}
		memcpy(compressionTables[2].Q, compressionTables[1].Q, 64);
		parms->LSIbytesRemaining = tmpLen;
		compressionTables[0].Qlen = 64;
		compressionTables[1].Qlen = 64;
		compressionTables[2].Qlen = 64;
		compressionTables[0].Q16len = 0;
		compressionTables[1].Q16len = 0;
		compressionTables[2].Q16len = 0;
		compressionTables[0].Q16FPlen = 0;
		compressionTables[1].Q16FPlen = 0;
		compressionTables[2].Q16FPlen = 0;
	}
	else
	{
		for (n = 0; n < 3; n++)
		{
			loadCompressionTable(media, compressionTables + n, n);
			if (localJPEGStatus != OM_ERR_NONE)  /* is set in loadCompressionTable */
				return;
		}	
	}
	
	dinfo->num_components = 3;
	dinfo->comps_in_scan = 3;
	if(media->pvt->pixType == kOmfPixRGBA)
	  	dinfo->out_color_space = CS_RGB;
	else
	  	dinfo->out_color_space = CS_YCbCr;
	dinfo->jpeg_color_space = CS_YCbCr;
	dinfo->color_out_comps = 3;
	dinfo->image_width = fieldWidth;
	numFields = ((layout == kSeparateFields) ? 2 : 1);
	dinfo->image_height = fieldHeight;
	dinfo->data_precision = 8;
	dinfo->arith_code = 0;
	dinfo->CCIR601_sampling = FALSE;
	dinfo->restart_interval = 0;
	
	dinfo->CCIR = FALSE;

#if ! STANDARD_JPEG_Q
	dinfo->quant16FP = FALSE;
#endif

	if (info.JPEGTableID > 0 && info.JPEGTableID < 256)
	{ 	 
		if (dinfo->out_color_space == CS_RGB && (fieldWidth == 720 || fieldWidth == 352))
			dinfo->CCIR = TRUE;
	}

	if (dinfo->CCIR)
	{
		int  i;
		dinfo->CCIRLumaOutMap = (JSAMPLE *) (*dinfo->emethods->alloc_small) (256 * SIZEOF(JCOEF));
		for (i = 0; i < 256; i++)
		{
			if(i <= 16) 
				dinfo->CCIRLumaOutMap[i] = 0;
			else if(i >= 235) 
				dinfo->CCIRLumaOutMap[i] = 255;
			else 
				dinfo->CCIRLumaOutMap[i] = (JSAMPLE)(((i-16) * 255) / 219.0 + .5);
		}
		
		dinfo->CCIRChromaOutMap = (JSAMPLE *) (*dinfo->emethods->alloc_small) (256 * SIZEOF(JCOEF));
		for (i = 0; i < 256; i++)
		{
			if(i <= 16) 
				dinfo->CCIRChromaOutMap[i] = 0;
			/* LF-W, change bound from 242 to 241 to fix overflow bug */
			else if(i >= 241) 
				dinfo->CCIRChromaOutMap[i] = 255;
			else 
				dinfo->CCIRChromaOutMap[i] = (JSAMPLE)(((i-16) * 256) / 225 + .5);
		}
	}

	for (i = 0; i < NUM_QUANT_TBLS; i++)
		dinfo->quant_tbl_ptrs[i] = NULL;

	for (i = 0; i < NUM_HUFF_TBLS; i++)
	{
		dinfo->dc_huff_tbl_ptrs[i] = NULL;
		dinfo->ac_huff_tbl_ptrs[i] = NULL;
	}

	dinfo->comp_info = (jpeg_component_info *) (*dinfo->emethods->alloc_small)
		(dinfo->num_components * SIZEOF(jpeg_component_info));

	/* luminance component */
	compptr = &dinfo->comp_info[0];
	compptr->component_index = 0;
	compptr->component_id = 1;
	compptr->h_samp_factor = 2;
	compptr->v_samp_factor = 1;
	compptr->quant_tbl_no = 0;
	compptr->dc_tbl_no = 0;
	compptr->ac_tbl_no = 0;

	/* two chrominance components, 2 & 3 */
	for (ci = 1; ci < dinfo->num_components; ci++)
	{
		compptr = &dinfo->comp_info[ci];
		compptr->component_index = ci;
		compptr->component_id = ci + 1;
		compptr->h_samp_factor = 1;
		compptr->v_samp_factor = 1;
		compptr->quant_tbl_no = 1;
		compptr->dc_tbl_no = 1;
		compptr->ac_tbl_no = 1;
	}

	dinfo->cur_comp_info[0] = &dinfo->comp_info[0];
	dinfo->cur_comp_info[1] = &dinfo->comp_info[1];
	dinfo->cur_comp_info[2] = &dinfo->comp_info[2];


	/* Set up two quantization tables */
	if (dinfo->quant_tbl_ptrs[0] == NULL)
		dinfo->quant_tbl_ptrs[0] = (QUANT_TBL_PTR)
			(*dinfo->emethods->alloc_small) (SIZEOF(QUANT_TBL));
	quant_ptr = dinfo->quant_tbl_ptrs[0];

#ifdef JPEG_TRACE
	fprintf(stderr, "JPEG: Using TIFF Quantization tables\n");
#endif

#if ! STANDARD_JPEG_Q
	if (compressionTables[0].Q16FPlen)
		dinfo->quant16FP = TRUE;
#endif

	for (i = 0; i < DCTSIZE2; i++)
	{
#if ! STANDARD_JPEG_Q
		if (dinfo->quant16FP && compressionTables[0].Q16FP != NULL)
			quant_ptr[i] = (QUANT_VAL) compressionTables[0].Q16FP[i];
		else
#endif
		    /* LF-W (12/18/96) Hack to get bad tables in Kansas 77&9s files */
		    if (info.JPEGTableID == 72)  /* AVR77 */
               quant_ptr[i] = avr77Tables[i];
			else if (info.JPEGTableID == 73) /* AVR9s */
			  quant_ptr[i] = avr9sTables[i];
			else if (info.JPEGTableID == 33 || info.JPEGTableID == 32) /* AVR70 & 70B */
			  quant_ptr[i] = avr70Tables[i];
		    else if (info.JPEGTableID == 50 || info.JPEGTableID == 51)  /* AVR75 & 75B */
               quant_ptr[i] = avr75Tables[i];
			else if (compressionTables[0].Q16len)
				quant_ptr[i] = (QUANT_VAL) compressionTables[0].Q16[i];
			else
				quant_ptr[i] = (QUANT_VAL) compressionTables[0].Q[i];
	}
	
	if (dinfo->quant_tbl_ptrs[1] == NULL)
		dinfo->quant_tbl_ptrs[1] = (QUANT_TBL_PTR)
			(*dinfo->emethods->alloc_small) (SIZEOF(QUANT_TBL));
	quant_ptr = dinfo->quant_tbl_ptrs[1];

	for (i = 0; i < DCTSIZE2; i++)
	{
#if ! STANDARD_JPEG_Q
		if (dinfo->quant16FP && compressionTables[1].Q16FP != NULL)
			quant_ptr[i] = (QUANT_VAL) compressionTables[1].Q16FP[i];
		else
#endif
		    /* LF-W (12/18/96) Hack to get bad tables in Kansas 77&9s files */
		    if (info.JPEGTableID == 72)  /* AVR77 */
               quant_ptr[i] = avr77Tables[i+DCTSIZE2];
			else if (info.JPEGTableID == 73) /* AVR9s */
			  quant_ptr[i] = avr9sTables[i+DCTSIZE2];
			else if (info.JPEGTableID == 33 || info.JPEGTableID == 32) /* AVR70 & 70B */
			  quant_ptr[i] = avr70Tables[i+DCTSIZE2];
		    else if (info.JPEGTableID == 50 || info.JPEGTableID == 51)  /* AVR75 & 75B */
               quant_ptr[i] = avr75Tables[i+DCTSIZE2];
			else if (compressionTables[1].Q16len)
				quant_ptr[i] = (QUANT_VAL) compressionTables[1].Q16[i];
			else
				quant_ptr[i] = (QUANT_VAL) compressionTables[1].Q[i];
	}

	/* Set up two Huffman tables, one for luminance, one for chrominance */
	for (ci = 0; ci < 2; ci++)
	{
		omfUInt8          *hv;

		/* DC table */
		bits[0] = 0;
		count = 0;
		
		/* ROGER-- I removed a check for JPEG table id, which, if found,
		    caused the use of STD_DCT_PTR[ci] instead of compressionTables.
		    This is no longer necessary.  It is the responsibility of the
		    user to set all of the tables in the file correctly.  It should
		    always be correct to use compressionTables.
		    
		 */
		if(info.compression == kLSIJPEG)
			hv = STD_DCT_PTR[ci];
		else
			hv = compressionTables[ci].DC;

		for (i = 1; i <= 16; i++)
		{
			bits[i] = (UINT8) * hv++;
			count += bits[i];
		}
		if (count > 16)
			ERREXIT(dinfo->emethods, "Bogus DC counts");

		for (i = 0; i < count; i++)
			huffval[i] = (UINT8) * hv++;

		htblptr = &dinfo->dc_huff_tbl_ptrs[ci];

		if (*htblptr == NULL)
			*htblptr = (HUFF_TBL *) (*dinfo->emethods->alloc_small) (SIZEOF(HUFF_TBL));

		MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
		MEMCOPY((*htblptr)->huffval, huffval, SIZEOF((*htblptr)->huffval));

		/* AC table */
		bits[0] = 0;
		count = 0;

		/* ROGER-- I removed a check for JPEG table id, which, if found,
		    caused the use of STD_ACT_PTR[ci] instead of compressionTables.
		    This is no longer necessary.  It is the responsibility of the
		    user to set all of the tables in the file correctly.  It should
		    always be correct to use compressionTables.
		    
		 */
		if(info.compression == kLSIJPEG)
			hv = STD_ACT_PTR[ci];
		else
			hv = compressionTables[ci].AC;

		for (i = 1; i <= 16; i++)
		{
			bits[i] = (UINT8) * hv++;
			count += bits[i];
		}

		if (count > 256)
			ERREXIT(dinfo->emethods, "Bogus AC count");

		for (i = 0; i < count; i++)
			huffval[i] = (UINT8) * hv++;

		htblptr = &dinfo->ac_huff_tbl_ptrs[ci];

		if (*htblptr == NULL)
			*htblptr = (HUFF_TBL *) (*dinfo->emethods->alloc_small) (SIZEOF(HUFF_TBL));

/*		MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
		MEMCOPY((*htblptr)->huffval, huffval, SIZEOF((*htblptr)->huffval)); 
    ROGER-- check this out! */

		for (i = 0; i < 17; i++)
			(*htblptr)->bits[i] = bits[i];
		for (i = 0; i < 256; i++)
			(*htblptr)->huffval[i] = huffval[i];
	}


}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * Read the start of a scan (everything through the SOS marker). Return TRUE
 * if find SOS, FALSE if find EOI.
 */


METHODDEF       boolean
                omjpeg_read_scan_header(decompress_info_ptr dinfo)
{
	/*
	 * this should only return true once.  If zero, increment and return
	 * 1
	 */
	if (visited)
		return (0);
	else
	{
		visited++;
		return (1);
	}
}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
METHODDEF void
                omjpeg_resync_to_restart(decompress_info_ptr dinfo, int marker)
{
}
/*
 * Finish up after a compressed scan (series of read_jpeg_data calls);
 * prepare for another read_scan_header call.
 */

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
METHODDEF void
                omjpeg_read_scan_trailer(decompress_info_ptr dinfo)
{
	/* no work needed */
}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * Finish up at the end of the file.
 */

METHODDEF void
                omjpeg_read_file_trailer(decompress_info_ptr dinfo)
{
	/* no work needed */
}


/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * The method selection routine for standard JPEG header reading. Note that
 * this must be called by the user interface before calling jpeg_decompress.
 * When a non-JFIF file is to be decompressed (TIFF, perhaps), the user
 * interface must discover the file type and call the appropriate method
 * selection routine.
 */

METHODDEF void
                jselromfi(decompress_info_ptr dinfo, omfCompatVideoCompr_t compress)
{
	dinfo->methods->read_file_header = omjpeg_read_file_header;
	dinfo->methods->read_scan_header = omjpeg_read_scan_header;
	if(compress == kLSIJPEG)
	{
		dinfo->methods->read_jpeg_data = omjpeg_read_LSIjpeg_data;
	}
	else
	{
		dinfo->methods->read_jpeg_data = omjpeg_read_jpeg_data;
	}
	dinfo->methods->resync_to_restart = omjpeg_resync_to_restart;
	dinfo->methods->read_scan_trailer = omjpeg_read_scan_trailer;
	dinfo->methods->read_file_trailer = omjpeg_read_file_trailer;
}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * OK, here is the main function that actually causes everything to happen.
 * We assume here that the JPEG filename is supplied by the caller of this
 * routine, and that all decompression parameters can be default values. The
 * routine returns 1 if successful, 0 if not.
 */

omfErr_t        omfmTIFFDecompressSample(JPEG_MediaParms_t *parms)
{
	omfMediaHdl_t		media = parms->media;
	
	/*
	 * These three structs contain JPEG parameters and working data. They
	 * must survive for the duration of parameter SETUP and one call to
	 * jpeg_decompress; typically, making them local data in the calling
	 * routine is the best strategy.
	 */
	struct Decompress_info_struct dinfo;
	struct Decompress_methods_struct dc_methods;
	struct External_methods_struct e_methods;
	omfTIFF_JPEGInfo_t   info;
	omfErr_t		functionJPEGStatus =	OM_ERR_NONE;

	omfAssertMediaHdl(media);
	XPROTECT(media->mainFile)
	{
		functionJPEGStatus = codecGetInfo(media, kCompressionParms, 
									media->pictureKind, 
									sizeof(omfTIFF_JPEGInfo_t), &info);
		if (functionJPEGStatus !=	OM_ERR_NONE)
		{
			if  (localJPEGStatus == OM_ERR_NONE)
				localJPEGStatus = functionJPEGStatus;
		}

		CHECK(localJPEGStatus);

		visited = 0;		/* clear the image-visited flag */
	
		/*
		 * Select the input and output files. In this example we want to open
		 * the input file before doing anything else, so that the setjmp()
		 * error recovery below can assume the file is open. Note that
		 * dinfo.output_file is only used if your output handling routines
		 * use it; otherwise, you can just make it NULL. VERY IMPORTANT: use
		 * "b" option to fopen() if you are on a machine that requires it in
		 * order to read binary files.
		 */
	
		dinfo.input_file = (FILE *) parms;
		dinfo.output_file = (FILE *) parms;
	
		/* Initialize the system-dependent method pointers. */
		dinfo.methods = &dc_methods;	/* links to method structs */
		dinfo.emethods = &e_methods;
		/*
		 * Here we supply our own error handler; compare to use of standard
		 * error handler in the previous write_JPEG_file example.
		 */
		emethods = &e_methods;	/* save struct addr for possible access */
		e_methods.error_exit = error_exit;	/* supply error-exit routine */
		e_methods.trace_message = trace_message;	/* supply trace-message
								 * routine */
		e_methods.trace_level = 0;	/* default = no tracing */
		e_methods.num_warnings = 0;	/* no warnings emitted yet */
		e_methods.first_warning_level = 0;	/* display first corrupt-data
							 * warning */
		e_methods.more_warning_level = 3;	/* but suppress additional
							 * ones */
		/* select output routines */
		dinfo.methods->output_init = output_init;
		dinfo.methods->put_color_map = put_color_map;
		dinfo.methods->put_pixel_rows = put_pixel_rows;
		dinfo.methods->output_term = output_term;
	
		/* prepare setjmp context for possible exit from error_exit */
		if (setjmp(setjmp_buffer))
		{
			/*
			 * If we get here, the JPEG code has signaled an error.
			 * Memory allocation has already been cleaned up (see
			 * free_all call in error_exit), but we need to close the
			 * input file before returning. You might also need to close
			 * an output file, etc.
			 */
			RAISE(OM_ERR_DECOMPRESS);
		}
		/*
		 * Here we use the standard memory manager provided with the JPEG
		 * code. In some cases you might want to replace the memory manager,
		 * or at least the system-dependent part of it, with your own code.
		 */
		jselmemmgr(&e_methods);	/* select std memory allocation routines */
	
		/*
		 * If the decompressor requires full-image buffers (for two-pass
		 * color quantization or a noninterleaved JPEG file), it will create
		 * temporary files for anything that doesn't fit within the
		 * maximum-memory setting. You can change the default maximum-memory
		 * setting by changing e_methods.max_memory_to_use after jselmemmgr
		 * returns. On some systems you may also need to set up a signal
		 * handler to ensure that temporary files are deleted if the program
		 * is interrupted. (This is most important if you are on MS-DOS and
		 * use the jmemdos.c memory manager back end; it will try to grab
		 * extended memory for temp files, and that space will NOT be freed
		 * automatically.) See jcmain.c or jdmain.c for an example signal
		 * handler.
		 */
	
		/*
		 * Here, set up the pointer to your own routine for
		 * post-header-reading parameter selection.  You could also
		 * initialize the pointers to the output data handling routines here,
		 * if they are not dependent on the image type.
		 */
		dc_methods.d_ui_method_selection = d_ui_method_selection;
	
		/* Set up default decompression parameters. */
		j_d_defaults(&dinfo, TRUE);
	
		/*
		 * TRUE indicates that an input buffer should be allocated. In
		 * unusual cases you may want to allocate the input buffer yourself;
		 * see jddeflts.c for commentary.
		 */
	
		/*
		 * At this point you can modify the default parameters set by
		 * j_d_defaults as needed; for example, you can request color
		 * quantization or force grayscale output.  See jdmain.c for examples
		 * of what you might change.
		 */
	
		/* Set up to write an OMFI  baseline-JPEG file. */
		jselromfi(&dinfo, info.compression);
	
		/* Here we go! */
		jpeg_decompress(&dinfo);
	
		/* back out the remaining compressed bytes */
		if(info.compression != kLSIJPEG)
			omcSeekStreamRelative(media->stream, -1 * dinfo.bytes_in_buffer);

		/* That's it, son.  Nothin' else to do, except close files. */
	}
	XEXCEPT
	XEND
	
	/*
	 * You might want to test e_methods.num_warnings to see if bad data
	 * was detected.  In this example, we just blindly forge ahead.
	 */
	return OM_ERR_NONE;	/* indicate success */

	/*
	 * Note: if you want to decompress more than one image, we recommend
	 * you repeat this whole routine.  You MUST repeat the
	 * j_d_defaults()/alter parameters/jpeg_decompress() sequence, as
	 * some data structures allocated in j_d_defaults are freed upon exit
	 * from jpeg_decompress.
	 */
}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
/*
 * OK, here is the main function that actually causes everything to happen.
 * We assume here that the target filename is supplied by the caller of this
 * routine, and that all JPEG compression parameters can be default values.
 */

omfErr_t        omfmTIFFCompressSample(JPEG_MediaParms_t *parms, omfBool customTables, omfInt32 * sampleSize)
{
	omfMediaHdl_t		media = parms->media;
	omfInt32           ci, i, count;
	omfUInt8           bits[17];
	omfUInt8           huffval[256];
	HUFF_TBL      **htblptr;
	QUANT_TBL_PTR   quant_ptr;
	omfUInt8          *table_ptr;
/*	omfJPEGInfo_t   info;
	ROGER--- delete this when I know it works
	*/

	omfInt16           bitsPerPixel, n;
	omfInt32 fieldHeight, fieldWidth;
	omfFrameLayout_t layout;
	omfPixelFormat_t memFmt;
	jpeg_tables     compressionTables[3];
	omfErr_t		functionJPEGStatus =	OM_ERR_NONE;

	/*
	 * These three structs contain JPEG parameters and working data. They
	 * must survive for the duration of parameter SETUP and one call to
	 * jpeg_compress; typically, making them local data in the calling
	 * routine is the best strategy.
	 */
	struct Compress_info_struct cinfo;
	struct Compress_methods_struct c_methods;
	struct External_methods_struct e_methods;

	memset(huffval, 0, 256);  /* initialize memset to 0 */
	
	omfAssertMediaHdl(media);
	XPROTECT(media->mainFile)
	{
		/* memset(&cinfo, 0, sizeof(cinfo)); */
		functionJPEGStatus = omfmGetVideoInfo(media, 
												&layout, 
												&fieldWidth, 
												&fieldHeight, 
												NULL, 
												&bitsPerPixel, 
												&memFmt);
	
		if (functionJPEGStatus !=	OM_ERR_NONE)
		{
			if  (localJPEGStatus == OM_ERR_NONE)
				localJPEGStatus = functionJPEGStatus;
		}
		CHECK(localJPEGStatus);

	/*	localJPEGStatus = codecGetInfo(media, kCompressionParms, media->pictureKind, sizeof(omfJPEGInfo_t), &info);
	    ROGER--- delete this when I know it works
	    */
		if(customTables)
		{
			for (n = 0; n < 3; n++)
			{
				loadCompressionTable(media, compressionTables + n, n);
				CHECK(localJPEGStatus); /* is set in loadCompressionTable */
			}
		}
	
		
		/* Initialize the system-dependent method pointers. */
		cinfo.methods = &c_methods;	/* links to method structs */
		cinfo.emethods = &e_methods;
	
		/*
		 * Here we use the default JPEG error handler, which will just print
		 * an error message on stderr and call exit().  See the second half
		 * of this file for an example of more graceful error recovery.
		 */
		/*
		 * Here we supply our own error handler; compare to use of standard
		 * error handler in the previous write_JPEG_file example.
		 */
		emethods = &e_methods;	/* save struct addr for possible access */
		e_methods.error_exit = error_exit;	/* supply error-exit routine */
		e_methods.trace_message = trace_message;	/* supply trace-message
								 * routine */
		e_methods.trace_level = 10;	/* default = no tracing */
		e_methods.num_warnings = 0;	/* no warnings emitted yet */
		e_methods.first_warning_level = 0;	/* display first corrupt-data
							 * warning */
		e_methods.more_warning_level = 3;	/* but suppress additional
							 * ones */
	
		/*
		 * jselerror(&e_methods);	/* select std error/trace message
		 * routines
		 */
		/*
		 * Here we use the standard memory manager provided with the JPEG
		 * code. In some cases you might want to replace the memory manager,
		 * or at least the system-dependent part of it, with your own code.
		 */
		jselmemmgr(&e_methods);	/* select std memory allocation routines */
	
		/*
		 * If the compressor requires full-image buffers (for entropy-coding
		 * optimization or a noninterleaved JPEG file), it will create
		 * temporary files for anything that doesn't fit within the
		 * maximum-memory setting. (Note that temp files are NOT needed if
		 * you use the default parameters.) You can change the default
		 * maximum-memory setting by changing e_methods.max_memory_to_use
		 * after jselmemmgr returns. On some systems you may also need to set
		 * up a signal handler to ensure that temporary files are deleted if
		 * the program is interrupted. (This is most important if you are on
		 * MS-DOS and use the jmemdos.c memory manager back end; it will try
		 * to grab extended memory for temp files, and that space will NOT be
		 * freed automatically.) See jcmain.c or jdmain.c for an example
		 * signal handler.
		 */
	
		/*
		 * Here, set up pointers to your own routines for input data handling
		 * and post-init parameter selection.
		 */
		c_methods.input_init = input_init;
		c_methods.get_input_row = get_input_row;
		c_methods.input_term = input_term;
		c_methods.c_ui_method_selection = c_ui_method_selection;
	
		/* Set up default JPEG parameters in the cinfo data structure. */
		j_c_defaults(&cinfo, 75, FALSE);

		if (parms->isCCIR)
			cinfo.CCIR = TRUE;

		/*
		 * Note: 75 is the recommended default quality level; you may instead
		 * pass a user-specified quality level.  Be aware that values below
		 * 25 will cause non-baseline JPEG files to be created (and a warning
		 * message to that effect to be emitted on stderr).  This won't
		 * bother our decoder, but some commercial JPEG implementations may
		 * choke on non-baseline JPEG files. If you want to force baseline
		 * compatibility, pass TRUE instead of FALSE. (If non-baseline files
		 * are fine, but you could do without that warning message, set
		 * e_methods.trace_level to -1.)
		 */
	
		/*
		 * At this point you can modify the default parameters set by
		 * j_c_defaults as needed.  For a minimal implementation, you
		 * shouldn't need to change anything.  See jcmain.c for some examples
		 * of what you might change.
		 */
	
		cinfo.comp_info[0].h_samp_factor = 2;	/* OMFI specifies 2h1v */
		cinfo.comp_info[0].v_samp_factor = 1;
	
	
		if (cinfo.quant_tbl_ptrs[0] == NULL)
			cinfo.quant_tbl_ptrs[0] = (QUANT_TBL_PTR) (*cinfo.emethods->alloc_small) (SIZEOF(QUANT_TBL));
		quant_ptr = cinfo.quant_tbl_ptrs[0];
		table_ptr = customTables ? compressionTables[0].Q : STD_QT_PTR[0];
	
#if ! STANDARD_JPEG_Q
		if (customTables && compressionTables[0].Q16FPlen)
			cinfo.quant16FP = TRUE;
#endif

		for (i = 0; i < DCTSIZE2; i++)
		{
#if ! STANDARD_JPEG_Q
			if (cinfo.quant16FP && compressionTables[0].Q16FP != NULL)
				quant_ptr[i] = (QUANT_VAL) compressionTables[0].Q16FP[i];
			else
#endif
				quant_ptr[i] = table_ptr[i];
		}
	
		if (cinfo.quant_tbl_ptrs[1] == NULL)
			cinfo.quant_tbl_ptrs[1] = (QUANT_TBL_PTR) (*cinfo.emethods->alloc_small) (SIZEOF(QUANT_TBL));
		quant_ptr = cinfo.quant_tbl_ptrs[1];
		table_ptr = customTables ? compressionTables[1].Q : STD_QT_PTR[1];
	
		for (i = 0; i < DCTSIZE2; i++)
		{
#if ! STANDARD_JPEG_Q
			if (cinfo.quant16FP && compressionTables[1].Q16FP != NULL)
				quant_ptr[i] = (QUANT_VAL) compressionTables[1].Q16FP[i];
			else
#endif
				quant_ptr[i] = table_ptr[i];
		}
	
		/* Set up two Huffman tables, one for luminance, one for chrominance */
		for (ci = 0; ci < 2; ci++)
		{
			omfUInt8          *hv;
	
			/* DC table */
			bits[0] = 0;
			count = 0;
			hv = customTables ? compressionTables[ci].DC : STD_DCT_PTR[ci];
	
			for (i = 1; i <= 16; i++)
			{
				bits[i] = *hv++;
				count += bits[i];
			}
			if (count > 16)
				ERREXIT(cinfo.emethods, "Bogus DC count");
	
			for (i = 0; i < count; i++)
				huffval[i] = *hv++;
	
			htblptr = &cinfo.dc_huff_tbl_ptrs[ci];
	
			if (*htblptr == NULL)
				*htblptr = (HUFF_TBL *) (*cinfo.emethods->alloc_small) (SIZEOF(HUFF_TBL));
	
			for (i = 0; i < 17; i++)
				(*htblptr)->bits[i] = bits[i];
			for (i = 0; i < 256; i++)
				(*htblptr)->huffval[i] = huffval[i];

/*			MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
			MEMCOPY((*htblptr)->huffval, huffval, SIZEOF((*htblptr)->huffval));
	ROGER -- check this out */

			/* AC table */
			bits[0] = 0;
			count = 0;
			hv = customTables ? compressionTables[ci].AC : STD_ACT_PTR[ci];
	
			for (i = 1; i <= 16; i++)
			{
				bits[i] = *hv++;
				count += bits[i];
			}
	
			if (count > 256)
				ERREXIT(cinfo.emethods, "Bogus AC count");
	
			for (i = 0; i < count; i++)
				huffval[i] = *hv++;
	
			htblptr = &cinfo.ac_huff_tbl_ptrs[ci];
	
			if (*htblptr == NULL)
				*htblptr = (HUFF_TBL *) (*cinfo.emethods->alloc_small) (SIZEOF(HUFF_TBL));
	
			for (i = 0; i < 17; i++)
				(*htblptr)->bits[i] = bits[i];
			for (i = 0; i < 256; i++)
				(*htblptr)->huffval[i] = huffval[i];

/*			MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
			MEMCOPY((*htblptr)->huffval, huffval, SIZEOF((*htblptr)->huffval));
     ROGER -- check this out */

		}
	
		/*
		 * Select the input and output files. Note that cinfo.input_file is
		 * only used if your input reading routines use it; otherwise, you
		 * can just make it NULL. VERY IMPORTANT: use "b" option to fopen()
		 * if you are on a machine that requires it in order to write binary
		 * files.
		 */
	
		cinfo.input_file = (FILE *) parms;
		cinfo.output_file = (FILE *) parms;
	
		/* Here we go! */
		if (setjmp(setjmp_buffer) == 0)
		{
			jpeg_compress(&cinfo);
		}
	
		/* That's it, son.  Nothin' else to do, except close files. */
		/* Here we assume only the output file need be closed. */
	
		/*
		 * Note: if you want to compress more than one image, we recommend
		 * you repeat this whole routine.  You MUST repeat the
		 * j_c_defaults()/alter parameters/jpeg_compress() sequence, as some
		 * data structures allocated in j_c_defaults are freed upon exit from
		 * jpeg_compress.
		 */
		if (localJPEGStatus == OM_ERR_NONE)
			*sampleSize = fieldWidth * fieldHeight * (bitsPerPixel / 8);
		else
			*sampleSize = 0;
			
		CHECK(localJPEGStatus);
	
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);	/* Return some real status here */
}

#else
/* OMFI_JPEG_CODEC */

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t        omfmTIFFCompressSample(JPEG_MediaParms_t *parms,  omfBool customTables, omfInt32 * sampleSize)
{
	return (OM_ERR_JPEGDISABLED);
}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t        omfmTIFFDecompressSample(JPEG_MediaParms_t *parms)
{
	return (OM_ERR_JPEGDISABLED);
}

#endif

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
