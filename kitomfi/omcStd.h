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
#ifndef _OMF_STDC_
#define _OMF_STDC_ 1

#include "omCodec.h"

#if PORT_LANG_CPLUSPLUS
extern "C" 
  {
#endif

/*
 * Name: omcStd.h
 *
 * Function: The standard codec stream handlers which handle transfers to and
 *			 from both Bento objects and raw files.
 *
 * Audience: Toolkit users who need specialized codec streams, and need
 *			examples of working stream handlers.
 */

OMF_EXPORT omfErr_t openStdCodecStream(omfCodecStream_t *stream, 
							omfObject_t obj, 
							omfProperty_t prop, 
							omfType_t type);
OMF_EXPORT omfErr_t writeStdCodecStream(omfCodecStream_t *stream, 
							 omfUInt32 ByteCount, 
							 void *Buffer);
OMF_EXPORT omfErr_t readStdCodecStream(omfCodecStream_t *stream, 
							omfUInt32 ByteCount, 
							void *Buffer, 
							omfUInt32 *bytesRead );
OMF_EXPORT omfErr_t seekStdCodecStream(omfCodecStream_t *stream, 
							omfPosition_t position);
OMF_EXPORT omfErr_t lengthStdCodecStream(omfCodecStream_t *stream, 
							  omfLength_t *len);
OMF_EXPORT omfErr_t truncStdCodecStream(omfCodecStream_t *stream, 
							 omfLength_t len);
OMF_EXPORT omfErr_t closeStdCodecStream(omfCodecStream_t *stream);
OMF_EXPORT omfErr_t fileposStdCodecStream(omfCodecStream_t *stream, 
							   omfInt64 *result);

OMF_EXPORT omfErr_t stdCodecSwabProc(omfCodecStream_t *stream, 
						  omfCStrmSwab_t *src, 
						  omfCStrmSwab_t *dest, 
						  omfBool swapBytes);
OMF_EXPORT omfErr_t stdCodecSwabLenProc(omfCodecStream_t *stream, 
							 omfUInt32 inSize, 
							omfLengthCalc_t		direct,
							 omfUInt32 *result);
OMF_EXPORT omfErr_t stdCodecNeedsSwabProc(omfCodecStream_t *stream, 
							   omfBool *needsSwabbing);
OMF_EXPORT omfErr_t numsegStdCodecStream(omfCodecStream_t *stream,
								omfInt32		*result);
OMF_EXPORT omfErr_t seginfoStdCodecStream(
			omfCodecStream_t *stream,
			omfInt32		index,
			omfPosition_t	*startPos,	/* OUT -- where does the segment begin, */
			omfLength_t		*length);	/* OUT -- and how int  is it? */

#ifdef OMFI_SELF_TEST
void testSampleConversion(void);
#endif

#if PORT_LANG_CPLUSPLUS
  }
#endif
#endif /* _OMF_STDC_ */


/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
