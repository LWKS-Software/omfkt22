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

/* OMJPEG.h  - Definitions for JPEG processing */

#ifndef OMJPEG_INCLUDED
#define OMJPEG_INCLUDED

#include "omErr.h"
#include "omTypes.h"

#if PORT_LANG_CPLUSPLUS
extern "C" 
  {
#endif

typedef struct jpeg_tables
{
	omfInt16           Qlen;	/* just for validation */
	omfInt16           Q16len;
#if ! STANDARD_JPEG_Q
	omfInt16		   Q16FPlen;
#endif
	omfInt16           DClen;
	omfInt16           AClen;
	omfUInt8           Q[64];
	omfUInt16          Q16[64];
#if ! STANDARD_JPEG_Q
	omfUInt16          Q16FP[64];
#endif
	omfUInt8           DC[33];	/* 16+17 */
	omfUInt8           AC[272];/* 16+256 */
}               jpeg_tables;

typedef struct
{
	omfJPEGTableID_t JPEGTableID;
}               omfJPEGInfo_t;

#define JPEG_QT_16 1

/* Default tables for JPEG Processing */

/****************************************************************************
 * NOTE: Quantization tables are in zig-zag order 
 * NOTE: These are no int  static to save some memory since they are 
 *       referenced by both
 * OMVideo.c and OMJPEG.c 
 ****************************************************************************/

extern omfUInt8    STD_QT[];	/* From ISO JPEG spec, table 13.1.1, 13.1.2 */

extern omfUInt8   *STD_QT_PTR[3];	/* pointers to components */

#ifdef JPEG_QT_16

/****************************************************************************
 * 16-bit (128-byte) Quantization table for those applications 
 * which need them.  Values are the same as in the 8-bit table. 
 ***************************************************************************/

extern omfUInt16   STD_QT16[];	/* From ISO JPEG spec, table 13.1.1, 13.1.2 */

extern omfUInt16  *STD_QT16_PTR[3];/* pointers to components */

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


extern omfUInt8    STD_DCT[];	/* From ISO JPEG spec, table 13.3.1.1,
								 * 13.3.1.2 */

extern omfUInt8   *STD_DCT_PTR[3];
extern omfInt32    STD_DCT_LEN[3];

/****************************************************************************
 * The standard AC tables have the following format : 
 *     o 16 bytes of "bits" indicating # of codes of length 0-15, followed by 
 *     o upto 256
 * bytes of "values" associated with those codes; in order of length 
 *     o
 * Note: we only have 162 ( total = 178 ) 
 * They are the same for each chroma component 
 ****************************************************************************/

extern omfUInt8    STD_ACT[];	/* From ISO JPEG spec, table 13.3.2.1, 
								   13.3.2.2 */

extern omfUInt8   *STD_ACT_PTR[3];
extern omfInt32    STD_ACT_LEN[3];

omfErr_t        omfmTIFFCompressSample(omfMediaHdl_t media,  omfBool customTables, omfInt32 * sampleSize);
omfErr_t        omfmTIFFDecompressSample(omfMediaHdl_t media);

omfErr_t		omfmJFIFCompressSample (omfMediaHdl_t media, omfBool customTables, omfInt32 * sampleSize);
omfErr_t        omfmJFIFDecompressSample(omfMediaHdl_t media);

omfErr_t        omfsJPEGInit(void);

#if PORT_LANG_CPLUSPLUS
  }
#endif
#endif

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
