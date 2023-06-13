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

#ifndef _OMF_CODEC_STREAMS_
#define _OMF_CODEC_STREAMS_ 1

#include "omErr.h"
#include "omTypes.h"
#include "omFile.h"

#if PORT_LANG_CPLUSPLUS
extern          "C"
{
#endif

typedef struct
{
	omfDDefObj_t		mediaKind;
	omfVideoMemOp_t		*vfmt;
	omfAudioMemOp_t		*afmt;
}               omfCStrmFmt_t;

typedef struct
{
	omfCStrmFmt_t   fmt;
	void           *buf;
	omfUInt32         buflen;
}               omfCStrmSwab_t;

typedef struct omfCodecStream omfCodecStream_t;

/*
 * CODEC Stream Callback Functions
 */
typedef	omfErr_t (*omfStreamFileOpen_t)(fileHandleType fh, 
										omfHdl_t file);
typedef	omfErr_t (*omfStreamFileCreate_t)(fileHandleType fh, 
										  omfHdl_t file);
typedef	omfErr_t (*omfStreamFileClose_t)(omfHdl_t file);
typedef	omfErr_t(*omfCodecStreamOpen_t) (omfCodecStream_t *stream, 
										 omfObject_t obj, 
										 omfProperty_t prop, 
										 omfType_t type);
typedef	omfErr_t(*omfCodecStreamWrite_t) (omfCodecStream_t *stream, 
										  omfUInt32 ByteCount, 
										  void *Buffer);
typedef	omfErr_t(*omfCodecStreamRead_t) (omfCodecStream_t *stream, 
										 omfUInt32 ByteCount, 
										 void *Buffer, 
										 omfUInt32 * bytesRead);
typedef	omfErr_t(*omfCodecStreamSeek_t) (omfCodecStream_t *stream, 
										 omfPosition_t position);
typedef	omfErr_t(*omfCodecStreamGetpos_t) (omfCodecStream_t *stream, 
										   omfPosition_t * position);
typedef	omfErr_t(*omfCodecStreamLength_t) (omfCodecStream_t *stream, 
										   omfLength_t *length);
typedef	omfErr_t(*omfCodecStreamTrunc_t) (omfCodecStream_t *stream, 
										  omfLength_t length);
typedef	omfErr_t(*omfCodecStreamClose_t) (omfCodecStream_t *stream);
typedef	omfErr_t(*omfCStrmFileposProc_t)(omfCodecStream_t *stream, 
										 omfInt64 *result);
typedef	omfErr_t (*omfCStrmNumsegProc_t)(omfCodecStream_t *stream,
								omfInt32		*result);
typedef	omfErr_t (*omfCStrmSeginfoProc_t)(
			omfCodecStream_t *stream,
			omfInt32		index,
			omfPosition_t		*startPos,
			omfLength_t		*length);

struct omfCodecStreamFuncs
{
	omfStreamFileOpen_t		fileOpenFunc;
	omfStreamFileCreate_t	fileCreateFunc;
	omfStreamFileClose_t	fileCloseFunc;
	omfCodecStreamOpen_t 	openFunc;
	omfCodecStreamWrite_t 	writeFunc;
	omfCodecStreamRead_t 	readFunc;
	omfCodecStreamSeek_t 	seekFunc;
	omfCodecStreamLength_t 	lengthFunc;
	omfCodecStreamTrunc_t 	truncFunc;
	omfCodecStreamClose_t 	closeFunc;
	omfCStrmFileposProc_t		filePosFunc;
	omfCStrmNumsegProc_t	numsegFunc;
	omfCStrmSeginfoProc_t		seginfoFunc;
};

typedef enum { 
               omcCacheRead, omcCacheWrite, omcNone 
} omcCacheDir_t;

typedef enum {
		omcComputeMemSize, omfComputeFileSize
} omfLengthCalc_t;

typedef	omfErr_t(*omfCStrmSwabProc_t) (omfCodecStream_t *stream, 
									   omfCStrmSwab_t * src, 
									   omfCStrmSwab_t * dest, 
									   omfBool swapBytes);
typedef	omfErr_t(*omfCStrmSwabLenProc_t) (omfCodecStream_t *stream, 
										  omfUInt32 inSize, 
										  omfLengthCalc_t direct,
										  omfUInt32 *result);
typedef	omfErr_t(*omfCStrmSwabTstProc_t) (omfCodecStream_t *stream, 
										  omfBool *needsSwabbing);

typedef struct omfCodecSwabProc
{
    omfInt16				priority;
	omfCStrmSwabProc_t 		swabProc;
	omfCStrmSwabLenProc_t	swabLenProc;
	omfCStrmSwabTstProc_t	swabTestProc;	/* If swabbing will be required */
	omfUInt32          			swabBufSize;
	void           				*swabBuf;
	struct omfCodecSwabProc	*next;
} omfCodecSwabProc_t;
	
struct omfCodecStream
{
	omfHdl_t				mainFile;
	omfHdl_t				dataFile;
	struct omfCodecStreamFuncs funcs;
	omfCodecSwabProc_t		*swabProcs;
	/* */
		
	omfInt32          		cookie;
	void 					*procData;
	omfCStrmFmt_t   		memFormat;
	omfCStrmFmt_t   		fileFormat;
	omfBool         		swapBytes;
	omfPosition_t			fileOffset;
	omcCacheDir_t			direction;
	omfInt32				blockingSize;
#ifdef OMFI_ENABLE_STREAM_CACHE
	char					*cachePtr;
	omfUInt32				cachePhysSize;
	omfUInt32				cacheLogicalSize;
	omfPosition_t			cacheStartOffset;
	omfInt32				totalWrites;
	omfInt32				writeCacheHits;
	omfInt32				totalReads;
	omfInt32				readCacheHits;
#endif
};

OMF_EXPORT omfErr_t		omcFlushCache(omfCodecStream_t *stream);
	
OMF_EXPORT omfErr_t omcOpenStream(
			omfHdl_t		mainFile,
			omfHdl_t		dataFile,
			omfCodecStream_t *stream,
			omfObject_t		dataObj,	/* open for streaming to this obj */
			omfProperty_t	prop,		/* in this field. */
			omfType_t		type);

OMF_EXPORT omfErr_t omcReadStream(
			omfCodecStream_t *stream,
			omfUInt32		 bufLength,	/* IN - read this many bytes */
			void			 *buffer,	/* IN/OUT - into this buffer */
			omfUInt32		 *bytesRead);	/* OUT - on failure, bytes read */

OMF_EXPORT omfErr_t omcWriteStream(
			omfCodecStream_t *stream,
			omfUInt32 			bufLength,	/* IN - write this many bytes */
			void			*buffer);	/* IN - from this buffer. */

OMF_EXPORT omfErr_t omcSeekStreamTo(
			omfCodecStream_t	*stream,
			omfPosition_t		offset);		/* IN -- Seek to this offset */

OMF_EXPORT omfErr_t omcSeekStreamRelative(
			omfCodecStream_t *stream,
			omfInt32		 relPos);	/* IN - seek forward this many bytes */

OMF_EXPORT omfErr_t omcGetStreamPosition(
			omfCodecStream_t *stream,
			omfPosition_t	*offset); /* OUT - return position through here */

OMF_EXPORT omfErr_t omcSeekStream32(
			omfCodecStream_t *stream,
			omfUInt32			offset);	/* IN - seek to this offset */

OMF_EXPORT omfErr_t omcGetStreamPos32(
			omfCodecStream_t *stream,
			omfUInt32			*offset);	/* OUT - Find the 32-bitr offset */

OMF_EXPORT omfBool omcIsPosValid(
			omfCodecStream_t *stream,
			omfPosition_t			offset);	/* IN - check this position */

OMF_EXPORT omfErr_t omcGetLength(
			omfCodecStream_t *stream,		/* IN - On this media stream */
			omfLength_t		 *length);		/* OUT - return the length */
				
OMF_EXPORT omfErr_t omcCloseStream(
			omfCodecStream_t *stream);	/* IN - Close this codec stream */
	
OMF_EXPORT omfErr_t omfmSetStreamCacheSize(
			omfMediaHdl_t	media,
			omfUInt32			cacheSize);		/* make the cache this big */
	
OMF_EXPORT omfErr_t omcSetMemoryFormat(
			omfCodecStream_t *stream,
			omfCStrmFmt_t	fmt);		/* IN - set this memory format */
	
OMF_EXPORT omfErr_t omcSetFileFormat(
			omfCodecStream_t *stream,
			omfCStrmFmt_t	fmt,		/* IN - set this file format */
			omfBool			swapBytes);	/* IN - optionally swap the words */
	
OMF_EXPORT omfErr_t omcComputeBufferSize(
			omfCodecStream_t *stream,
			omfUInt32		inSize,	/* IN - Given # of bytes  */
			omfLengthCalc_t	direct,
			omfUInt32		*result);	/* OUT - Return the other size */
	
omfErr_t omcSwabData(
			omfCodecStream_t *stream,	/* IN - On this codec stream  */
			omfCStrmSwab_t	*src,		/* IN - munge data from this buf & fmt  */
			omfCStrmSwab_t	*dest,			/* IN - into this buf & fmt  */
			omfBool			swapBytes);		/* IN - optionally swapping words*/
	
omfErr_t omcReadSwabbedStream(
			omfCodecStream_t *stream,		/* IN - On this codec stream  */
			omfUInt32		fileXferLength,	/* IN - read this much from file */
			omfUInt32		bufferLength,	/* IN - translate into this size */
			void			*buffer,		/* IN - In this buffer */
			omfUInt32		*bytesRead);	/* OUT -- and tell how much read */
	
omfErr_t  omcWriteSwabbedStream(
			omfCodecStream_t *stream,		/* IN - On this codec stream  */
			omfUInt32			fileXferLength,	/* IN - write this much to file */
			omfUInt32			bufferLength,	/* IN - translated from size */
			void			*buffer);		/* IN - and from this buffer */

OMF_EXPORT omfBool		omcIsStreamOpen(omfCodecStream_t *stream);
	
OMF_EXPORT omfErr_t omfmSetStreamFuncs(
			omfHdl_t				file,
			omfCodecStreamFuncs_t	*def);

OMF_EXPORT omfErr_t omfmCodecRestoreDefaultStreamFuncs(
			omfHdl_t	 			file);	/* IN - On this codec stream  */

OMF_EXPORT omfErr_t omfmSetSwabProc(
			omfCodecStream_t	 	*stream,/* IN - On this codec stream  */
			omfCStrmSwabProc_t		swabProc,
			omfCStrmSwabLenProc_t	swabLenProc,
			omfCStrmSwabTstProc_t	swabTestProc);

OMF_EXPORT omfErr_t omfmSetSwabBufSize(
			omfCodecStream_t	 	*stream, /* IN - On this codec stream  */
			omfUInt32				newBufSize);

OMF_EXPORT omfErr_t omcGetStreamFilePos(
			omfCodecStream_t *stream,	  /* IN - On this media stream */
			omfInt64			*offset); /* OUT - return position here */
			
OMF_EXPORT omfErr_t omcGetStreamNumSegments(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfInt32			*numseg);	/* OUT - return position through here */

OMF_EXPORT omfErr_t omcGetStreamSegmentInfo(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfInt32		index,
			omfPosition_t		*startPos,	/* OUT -- where does the segment begin, */
			omfLength_t		*length);	/* OUT -- and how int  is it? */

#define STREAM_COOKIE			0x5354524D

#if PORT_LANG_CPLUSPLUS
}
#endif
#endif				/* _OMF_CODEC_STREAMS_ */

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
