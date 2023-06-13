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

/*
 * Name: omcStrm.c
 *
 * Function: Dispatcher for codec streams, and provides higher-level functions which
 *			call the dispatcher functions.  Codec streams are provided to media codecs
 *			in order to provide a common interface, regardless of whether the data
 *			is embedded in an OMFI file, or separate.  Thi smodule also includes
 *			functions which swab the data being processed by the stream, including
 *			operations which may change effective sample rates or sizes.  By providing
 *			a swabProc to the stream, the stream buffering may be used for temporary
 *			buffers in order to avoid having to allocate extra buffers
 *			to hold intermediate data.
 *
 * Audience: CODEC Developers
 *
 * General error codes returned:
 * 		OM_ERR_BAD_FHDL -- media referenced a bad omfHdl_t.
 * 		OM_ERR_BENTO_PROBLEM -- Bento returned an error, check BentoErrorNumber.
 * 		OM_ERR_NULL_STREAMPROC -- The streamProc for that operation was NULL.
 */

#include "masterhd.h"
#include <string.h>

#include "omPublic.h"
#include "omcStd.h" 
#include "omPvt.h"

#define DEFAULT_SWAB_SIZE		(64L*1024L)
#define DEFAULT_STREAMBUF_SIZE	(64L*1024L)

/*#define PERFORMANCE_TEST		1	*/

/************************
 * omfmCodecSetStreamFuncs
 *
 * 	Replaces the default stream functions provided with the toolkit
 *		with new functions.  This replacement takes place only for the
 *		given media handle.
 *
 *		The default stream functions handle transfers on Bento files,
 *		and on non-Bento files using the ANSI or Mac filesystem calls,
 *		depending on platform.
 *		
 *		clients whose systems don't supprort the ANSI file calls will
 *		have to replace the stream functions to read/write raw media,
 *		and write their own bento handler to do bento I/O.
 *
 *		If you only need to replace one call, link the rest of them to
 *		the correct function (see omfmCodecRestoreDefaultStreamFuncs().)
 *		The standard functions are defined in omcStd.c.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmSetStreamFuncs(
			omfHdl_t						file,
			omfCodecStreamFuncs_t	*def)
{
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	omfAssert(def != NULL, file, OM_ERR_NULL_PARAM);
	file->streamFuncs = *def;
	file->customStreamFuncsExist = TRUE;
	
	return (OM_ERR_NONE);
}

/************************
 * omfmCodecRestoreDefaultStreamFuncs
 *
 * 	Restore the default stream functions.  See omfmCodecSetStreamFuncs
 *		for the details.  This replacement takes place only for the
 *		given media handle.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmCodecRestoreDefaultStreamFuncs(
			omfHdl_t file)
{
	omfCodecStreamFuncs_t streamDef;

	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	
	XPROTECT(file)
	{
		streamDef.openFunc = openStdCodecStream;
		streamDef.writeFunc = writeStdCodecStream;
		streamDef.readFunc = readStdCodecStream;
		streamDef.seekFunc = seekStdCodecStream;
		streamDef.lengthFunc = lengthStdCodecStream;
		streamDef.truncFunc = truncStdCodecStream;
		streamDef.closeFunc = closeStdCodecStream;
		streamDef.filePosFunc = fileposStdCodecStream;
		streamDef.numsegFunc = numsegStdCodecStream;
		streamDef.seginfoFunc = seginfoStdCodecStream;
		CHECK(omfmSetStreamFuncs(file, &streamDef));
		file->customStreamFuncsExist = FALSE;
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

omfErr_t omfmSetSwabProc(
			omfCodecStream_t	 	*stream,	/* IN - On this codec stream  */
			omfCStrmSwabProc_t		swabProc,
			omfCStrmSwabLenProc_t	swabLenProc,
			omfCStrmSwabTstProc_t	swabTestProc)
{
	omfCodecSwabProc_t	*entry;
	
	XPROTECT(NULL)
	{
		entry = stream->swabProcs;
		if(entry == NULL)
		{
			entry = (omfCodecSwabProc_t *)omOptMalloc(stream->mainFile, sizeof(omfCodecSwabProc_t));
			entry->swabBuf = omOptMalloc(stream->mainFile, DEFAULT_SWAB_SIZE);
			if(entry->swabBuf == NULL)
				RAISE(OM_ERR_NOMEMORY);
			entry->swabBufSize = DEFAULT_SWAB_SIZE;
		}
		entry->priority = 1;
		entry->swabProc = swabProc;
		entry->swabLenProc = swabLenProc;
		entry->swabTestProc = swabTestProc;
		/* Init the swab buffer.
		 */
		entry->next = NULL;
		stream->swabProcs = entry;
		}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

omfErr_t omfmSetSwabBufSize(
			omfCodecStream_t	 	*stream,	/* IN - On this codec stream  */
			omfUInt32				newBufSize)
{
	omfCodecSwabProc_t	*entry;

	XPROTECT(NULL)
	{
		newBufSize = ((newBufSize + 3) & (~0x03));
		entry = stream->swabProcs;
		if(entry == NULL)
			RAISE(OM_ERR_NULL_PARAM);
		
		if(entry->swabBuf != NULL)
			omOptFree(stream->mainFile, entry->swabBuf);
		entry->swabBuf = omOptMalloc(stream->mainFile, newBufSize);
		if(entry->swabBuf == NULL)
			RAISE(OM_ERR_NOMEMORY);
		entry->swabBufSize = newBufSize;
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************************************************************
 *
 * Stream cache setup routines
 *
 * The stream read/write routines can use a cache buffer in order to minimize
 * calls to the supporting file system (Bento or streams).  Setting the
 * stream cache size to 0 effectively disables the cache, and not defining
 * ENABLE_STREAM CACHE turns the cache off entirely.  This cache obviously
 * does NOT affect the speed of composition calls.
 *
 ************************************************************************/

/************************
 * omfmSetStreamCacheSize
 *
 * 		Sets the size of the buffer used when caching read and
 *
 * Argument Notes:
 *		Zero is a calid cache size, although negative numbers are not
 *		allowed.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_INVALID_CACHE_SIZE -- Cache size must be >= 0.
 */
omfErr_t omfmSetStreamCacheSize(
			omfMediaHdl_t	media,			/* on this media reference */
			omfUInt32			cacheSize)		/* make the cache this big */
{
	omfCodecStream_t *stream;
	
	omfAssertMediaHdl(media);
	omfAssert(cacheSize >= 0, media->mainFile, OM_ERR_INVALID_CACHE_SIZE);
	stream = media->stream;
	
#if OMFI_ENABLE_STREAM_CACHE
	XPROTECT(media->mainFile)
	{
		if((stream->cachePtr == NULL) || (cacheSize != stream->cachePhysSize))
		{
			if(stream->cachePtr != NULL)
				omOptFree(media->mainFile, stream->cachePtr);
			
			stream->cachePtr = (char *)omOptMalloc(media->mainFile, cacheSize);
			if(stream->cachePtr != NULL)
				stream->cachePhysSize = cacheSize;
			else
			{
				stream->cachePhysSize = 0;
				RAISE(OM_ERR_NOMEMORY);
			}
		}
	}
	XEXCEPT
	XEND
#endif

	return(OM_ERR_NONE);
}

#if OMFI_ENABLE_STREAM_CACHE
/************************
 * omcFlushCache
 *
 * 		Invalidates the cached data after a read, or writes the cache
 *		to secondary storage and invalidates after a write.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NULL_STREAMPROC -- The writeFunc is NULL
 */
omfErr_t		omcFlushCache(omfCodecStream_t *stream)
{
	omfHdl_t	main;
	
	main = stream->mainFile;
	omfAssert((stream->cookie == STREAM_COOKIE), main, OM_ERR_STREAM_CLOSED);

	XPROTECT(main)
	{
		if(stream->direction == omcCacheWrite)
		{
			XASSERT(stream->funcs.writeFunc != NULL, OM_ERR_NULL_STREAMPROC);
			CHECK((*stream->funcs.writeFunc) (stream, stream->cacheLogicalSize,
															stream->cachePtr));
		}
		omfsCvtInt32toInt64(0, &stream->cacheStartOffset);
		stream->cacheLogicalSize = 0;
		stream->direction = omcNone;
	}
	XEXCEPT
	XEND
	return(OM_ERR_NONE);
}
#endif

/************************************************************************
 *
 * CODEC stream callback wrappers
 *
 ************************************************************************/

/************************
 * omcOpenStream
 *
 * 		Opens a pre-existing stream for write/read operations.
 *
 *		This function also allocates a buffer of DEFAULT_SWAB_SIZE to be used when
 *		data swabbing needs to be performed which may grow or shrink the data.
 *
 * Argument Notes:
 *		The arguments "dataObj" and "prop" need to be supplied if the stream could be
 *		a Bento stream (using the default codec stream handler).
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omcOpenStream(
			omfHdl_t		mainFile,
			omfHdl_t		dataFile,
			omfCodecStream_t *stream,
			omfObject_t		dataObj,	/* open for streaming to this obj */
			omfProperty_t	prop,		/* in this field. */
			omfType_t		type)
{	
	XPROTECT(mainFile)
	{
		if(!mainFile->customStreamFuncsExist)
		{
			CHECK(omfmCodecRestoreDefaultStreamFuncs(mainFile));
		}
		stream->funcs = mainFile->streamFuncs;
		XASSERT(stream->funcs.openFunc != NULL, OM_ERR_NULL_STREAMPROC);
#if OMFI_ENABLE_STREAM_CACHE
		XASSERT(stream->cachePtr == NULL, OM_ERR_STREAM_REOPEN);
#endif
		XASSERT(stream->procData == NULL, OM_ERR_STREAM_REOPEN);

		stream->mainFile = mainFile;
		stream->dataFile = dataFile;
		stream->blockingSize = 0;
		/*
		 * Reset some fields which the openFunc should set up for real
		 */
		stream->procData = NULL;
		stream->memFormat.mediaKind = mainFile->nilKind;
		stream->fileFormat.mediaKind = mainFile->nilKind;
		stream->swapBytes = FALSE;
		stream->swabProcs = NULL;
		stream->cookie = STREAM_COOKIE;
		omfsCvtInt32toInt64(0, &stream->fileOffset);

#if OMFI_ENABLE_STREAM_CACHE
		stream->cacheLogicalSize = 0;
		stream->direction = omcNone;
		stream->cachePtr = NULL;
		stream->totalWrites = 0;
		stream->writeCacheHits = 0;
		stream->totalReads = 0;
		stream->readCacheHits = 0;
		stream->cachePtr = (char *)omOptMalloc(mainFile, DEFAULT_STREAMBUF_SIZE);
		if(stream->cachePtr != NULL)
			stream->cachePhysSize = DEFAULT_STREAMBUF_SIZE;
		CHECK(omcFlushCache(stream));
#endif
	
		CHECK((*stream->funcs.openFunc) (stream, dataObj, prop, type));
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}



/************************
 * omcReadStream
 *
 * 		Read data from an opened CODEC stream.
 *
 *		This is the root function which reads the data without any swabbing.
 *
 * Argument Notes:
 *		BytesRead does not need to be checked unless OM_ERR_EOF is returned, and
 *		may be NULL if you don't want incomplete buffers.
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_EOF -- Hit the end of available data.  Data will be valid up to
 *						"bytesRead" bytes.
 *		OM_ERR_NULLBUF - NULL transfer buffer
 */
omfErr_t omcReadStream(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfUInt32			bufLength,	/* IN - read this many bytes */
			void			*buffer,	/* IN/OUT - into this buffer */
			omfUInt32			*bytesRead)	/* OUT - on failure, bytes read */
{
	omfHdl_t        		main;
	omfUInt32          	junk;
#if OMFI_ENABLE_STREAM_CACHE
	omfUInt32          	actualCacheFillSize;
	omfUInt32			cacheOffset, bytesLeft;
	omfInt64			tmp, streamBytesLeft;
	omfBool			validOffset;
#endif
	
	main = stream->mainFile;
	omfAssert(stream->funcs.readFunc != NULL, main, OM_ERR_NULL_STREAMPROC);
	omfAssert(stream->funcs.lengthFunc != NULL, main, OM_ERR_NULL_STREAMPROC);
	omfAssert(buffer != NULL, main, OM_ERR_NULLBUF);
	omfAssert((stream->cookie == STREAM_COOKIE), main, OM_ERR_STREAM_CLOSED);

	XPROTECT(main)
	{

		if (bytesRead == NULL)
			bytesRead = &junk;
		*bytesRead = 0;
		
#if OMFI_ENABLE_STREAM_CACHE
		stream->totalReads++;
		if(stream->direction == omcCacheWrite)
			CHECK(omcFlushCache(stream));
#endif
		stream->direction = omcCacheRead;
#if OMFI_ENABLE_STREAM_CACHE
		tmp = stream->fileOffset;
		validOffset = omfsInt64LessEqual(stream->cacheStartOffset, tmp);
		CHECK(omfsSubInt64fromInt64(stream->cacheStartOffset, &tmp));
		CHECK(omfsTruncInt64toUInt32(tmp, &cacheOffset));	/* OK MAXREAD */
		if(bufLength > stream->cachePhysSize)
		{	/* read directly */
			CHECK((*stream->funcs.seekFunc) (stream, stream->fileOffset));
#endif
			CHECK((*stream->funcs.readFunc) (stream, bufLength, buffer, bytesRead));
#if OMFI_ENABLE_STREAM_CACHE
		}
		else if(validOffset && (cacheOffset > 0) &&
				cacheOffset + bufLength <= stream->cacheLogicalSize)
		{
			stream->readCacheHits++;
			memcpy(buffer, stream->cachePtr + cacheOffset, bufLength);
			*bytesRead = bufLength;
		}
		else
		{
			CHECK(omcFlushCache(stream));
			stream->cacheLogicalSize = stream->cachePhysSize;
			(*stream->funcs.lengthFunc)(stream, &streamBytesLeft);
			CHECK(omfsSubInt64fromInt64(stream->fileOffset, &streamBytesLeft));
			CHECK(omfsTruncInt64toUInt32(streamBytesLeft, &bytesLeft));	/* OK MAXREAD */
	
			if(bytesLeft < stream->cacheLogicalSize)
				stream->cacheLogicalSize = bytesLeft;
				
			CHECK((*stream->funcs.seekFunc) (stream, stream->fileOffset));
			CHECK((*stream->funcs.readFunc) (stream, stream->cacheLogicalSize,
										stream->cachePtr, &actualCacheFillSize));
			stream->cacheStartOffset = stream->fileOffset;
			if(bufLength <= stream->cacheLogicalSize)
			{
				memcpy(buffer, stream->cachePtr, bufLength);
				*bytesRead = bufLength;
			}
			else
			{
				XASSERT(actualCacheFillSize < bufLength, OM_ERR_SMALLBUF);
				memcpy(buffer, stream->cachePtr, actualCacheFillSize);
				*bytesRead = actualCacheFillSize;
				RAISE(OM_ERR_EOF);
			}
		}
#endif
		CHECK(omfsAddInt32toInt64(bufLength, &stream->fileOffset));
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
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
static omfErr_t padStream(omfCodecStream_t *stream, omfUInt32 padLength)
{
	omfInt32			fillLeft, fillSize, fillBufLen, tryLen;
	char			*zeroPtr;
	omfHdl_t        main;
	
	main = stream->mainFile;
	omfAssert((stream->cookie == STREAM_COOKIE), main, OM_ERR_STREAM_CLOSED);

	XPROTECT(main)
	{
#if OMFI_ENABLE_STREAM_CACHE
		CHECK(omcFlushCache(stream));
		if(stream->cachePhysSize != 0)
		{
			zeroPtr = stream->cachePtr;
			fillBufLen = stream->cachePhysSize;
		}
		else
#endif
		{
			tryLen = 32L*1024L;		/* A good starting size */
			do
			{
			fillBufLen = tryLen;
			zeroPtr = (char *)omOptMalloc(main, fillBufLen);
			tryLen /= 2;
			} while((zeroPtr == NULL) && (tryLen > 0));
			if(zeroPtr == NULL)
				RAISE(OM_ERR_NOMEMORY);
		}
		memset(zeroPtr, 0, fillBufLen);
		fillLeft = padLength;
		while(fillLeft > 0)
		{
			fillSize = fillBufLen;
			if(fillSize > fillLeft)
				fillSize = fillLeft;
			CHECK((*stream->funcs.writeFunc) (stream, fillSize, zeroPtr));
			fillLeft -= fillSize;
		}
#if OMFI_ENABLE_STREAM_CACHE
		if((stream->cachePhysSize == 0) && (zeroPtr != NULL))
#endif
			omOptFree(main, zeroPtr);
		}
	XEXCEPT
	XEND
		
	return(OM_ERR_NONE);
}

/************************
 * omcWriteStream
 *
 * 		Write data to an opened CODEC stream.
 *
 *		This is the root function which writes the data without any swabbing.
 *
 * Argument Notes:
 *		<none>
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NULLBUF - NULL transfer buffer
 */
omfErr_t omcWriteStream(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfUInt32 			bufLength,	/* IN - write this many bytes */
			void			*buffer)	/* IN - from this buffer. */
{
	omfHdl_t        main;
	omfInt64			endpos, newEndpos;

	main = stream->mainFile;
	omfAssert(stream->funcs.writeFunc != NULL, main, OM_ERR_NULL_STREAMPROC);
	omfAssert(buffer != NULL, main, OM_ERR_NULLBUF);
	omfAssert((stream->cookie == STREAM_COOKIE), main, OM_ERR_STREAM_CLOSED);

	XPROTECT(main)
	{
#if OMFI_ENABLE_STREAM_CACHE
		if(stream->direction == omcCacheRead)
			CHECK(omcFlushCache(stream));
#endif
		stream->direction = omcCacheWrite;
		if(stream->blockingSize != 0)
		{
			endpos = stream->fileOffset;
			newEndpos = endpos;
			omfsAddInt32toInt64(bufLength, &newEndpos);
			if(!omcIsPosValid(stream, newEndpos))
			{
			CHECK(padStream(stream, stream->blockingSize));
			CHECK(omcSeekStreamTo(stream, endpos));
			}
		}
#if OMFI_ENABLE_STREAM_CACHE
		stream->totalWrites++;
		
		if(stream->cacheLogicalSize + bufLength > stream->cachePhysSize)
		{
			CHECK(omcFlushCache(stream));
			stream->direction = omcCacheWrite;
		}
		
		if(bufLength > stream->cachePhysSize)		/* Write directly */
		{
#endif
			CHECK((*stream->funcs.writeFunc) (stream, bufLength, buffer));
#if OMFI_ENABLE_STREAM_CACHE
		}
		else
		{
			stream->writeCacheHits++;
			XASSERT(stream->cachePtr != NULL, OM_ERR_NULLBUF);
			memcpy(stream->cachePtr + stream->cacheLogicalSize, buffer, bufLength);
			stream->cacheLogicalSize += bufLength;
		}
#endif
		CHECK(omfsAddInt32toInt64(bufLength, &stream->fileOffset));
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * omcSeekStreamTo
 *
 * 		Seek to a given position on an opened CODEC stream.
 *
 *		This function takes a 64-bit argument.  A 32-bit version is available below
 *		for convenience.
 *
 * Argument Notes:
 *		Offset is a 64-bit number of bytes from the start of the stream.
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omcSeekStreamTo(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfInt64			offset)		/* IN -- Seek to this offset */
{
	omfHdl_t       	 main;
#if OMFI_ENABLE_STREAM_CACHE
	omfBool			validStart, validEnd;
	omfInt64			tmp;
#endif
	
	main = stream->mainFile;
	omfAssert(stream->funcs.seekFunc != NULL, main, OM_ERR_NULL_STREAMPROC);
	omfAssert((stream->cookie == STREAM_COOKIE), main, OM_ERR_STREAM_CLOSED);
	stream->fileOffset = offset;

	XPROTECT(main)
	{
#if OMFI_ENABLE_STREAM_CACHE
		if(stream->direction == omcCacheRead)
		{
			validStart = omfsInt64LessEqual(stream->cacheStartOffset, stream->fileOffset);
			tmp = stream->cacheStartOffset;
			CHECK(omfsAddInt32toInt64(stream->cacheLogicalSize, &tmp));
			validEnd = omfsInt64LessEqual(stream->cacheStartOffset, tmp);
			if(!validStart || !validEnd)
			{
				CHECK(omcFlushCache(stream));
			}
		}
		else
		{
			CHECK(omcFlushCache(stream));
		}
#endif
			 
		CHECK((*stream->funcs.seekFunc) (stream, offset));
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * omcIsPosValid
 *
 * 		Tells whether a given position is valid on an opened CODEC stream.
 *
 * Argument Notes:
 *		Offset is a 64-bit number of bytes from the start of the stream.
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfBool omcIsPosValid(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfInt64			offset)		/* IN - check this position */
{
	omfInt64		length;
	

	omfAssert((stream->cookie == STREAM_COOKIE), stream->mainFile, OM_ERR_STREAM_CLOSED);
	
	XPROTECT(stream->mainFile)
	{
		XASSERT(stream->funcs.lengthFunc != NULL, OM_ERR_NULL_STREAMPROC);

		CHECK(omcGetLength(stream, &length));

		omfsCvtInt32toInt64(0, &length);
		(*stream->funcs.lengthFunc) (stream,  &length);
	}
	XEXCEPT
		return(FALSE);
	XEND_SPECIAL(FALSE);
	
	return(omfsInt64Less(offset, length));
}

/************************
 * omcGetLength
 *
 * 		Returns the 64-bit length of a stream.
 *
 * Argument Notes:
 *		length is a 64-bit number of bytes.
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omcGetLength(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfInt64		 *length)	/* OUT - return the length */
{	
	omfAssert(stream->funcs.lengthFunc != NULL, stream->mainFile, OM_ERR_NULL_STREAMPROC);
	omfAssert((stream->cookie == STREAM_COOKIE), stream->mainFile, OM_ERR_STREAM_CLOSED);
	
	omfsCvtInt32toInt64(0, length);
	(*stream->funcs.lengthFunc) (stream, length);
	
	return(OM_ERR_NONE);
}

/************************
 * omcGetStreamPosition
 *
 * 		Returns the current position on an opened CODEC stream.
 *
 *		This function returns a 64-bit result.  A 32-bit version is available below
 *		for convenience.
 *
 * Argument Notes:
 *		Offset is a pointer to a 64-bit value, and will contain the number of bytes
 *		from the start of the stream.
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omcGetStreamPosition(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfInt64			*offset)	/* OUT - return position through here */
{
	omfAssert((stream->cookie == STREAM_COOKIE), stream->mainFile, OM_ERR_STREAM_CLOSED);
	*offset = stream->fileOffset;
	
	return (OM_ERR_NONE);
}

/************************
 * omcGetStreamFilePos
 *
 * 		Returns the current file position (as opposed to the stream position)
 *		on an opened CODEC stream.
 *
 *		This function returns a 64-bit result.
 *
 * Argument Notes:
 *		Offset is a pointer to a 64-bit value, and will contain the number of bytes
 *		from the start of the file, which is probably greater than the number of bytes
 *		into the stream.
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omcGetStreamFilePos(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfInt64			*offset)	/* OUT - return position through here */
{
	omfAssert((stream->cookie == STREAM_COOKIE), stream->mainFile, OM_ERR_STREAM_CLOSED);
	(*stream->funcs.filePosFunc) (stream,  offset);
	
	return (OM_ERR_NONE);
}

/************************
 * omcGetStreamNumSegments
 *
 * 		Returns the number of physical segments in the underlying implementation.
 *		Used when an application needs to bypass the toolkit read/write functions
 *		of files where the media stream may be fragmented.
 *		For all simple stream implementations (non-Bento) this function will return 1.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omcGetStreamNumSegments(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfInt32		*numseg)	/* OUT - return position through here */
{
	omfAssert((stream->cookie == STREAM_COOKIE), stream->mainFile, OM_ERR_STREAM_CLOSED);
	(*stream->funcs.numsegFunc) (stream, numseg);
	
	return (OM_ERR_NONE);
}

/************************
 * omcGetStreamSegmentInfo
 *
 * 		Returns the current file position (as opposed to the stream position)
 *		and length of a given physical segment of the underlying implementation.
 *		For all simple stream implementations (non-Bento) this function will return 0
 *		for the offset, and fileLength for the length.
 *
 *		This function returns two 64-bit results.
 *
 * Argument Notes:
 *		StartPos is a pointer to a 64-bit value, and will contain the number of bytes
 *		from the start of the file, which is probably greater than the number of bytes
 *		into the stream.
 *		Length is the number of contiguous bytes in the data.
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omcGetStreamSegmentInfo(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfInt32		index,
			omfPosition_t	*startPos,	/* OUT -- where does the segment begin, */
			omfLength_t		*length)	/* OUT -- and how int  is it? */
{
	omfAssert((stream->cookie == STREAM_COOKIE), stream->mainFile, OM_ERR_STREAM_CLOSED);
	(*stream->funcs.seginfoFunc) (stream,  index, startPos, length);
	
	return (OM_ERR_NONE);
}

/************************
 * omcCloseStream
 *
 * 		Closes the currently opened CODEC stream.
 *
 * Argument Notes:
 *		<none>
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omcCloseStream(
			omfCodecStream_t *stream)	/* IN - Close this codec stream */
{
	omfHdl_t        	main;
	omfCodecSwabProc_t	*tmp;
	
	main = stream->mainFile;
	omfAssert(stream->funcs.closeFunc != NULL, main, OM_ERR_NULL_STREAMPROC);
	omfAssert(stream->funcs.truncFunc != NULL, main, OM_ERR_NULL_STREAMPROC);
	omfAssert((stream->cookie == STREAM_COOKIE), main, OM_ERR_STREAM_CLOSED);
	
	XPROTECT(main)
	{
#if OMFI_ENABLE_STREAM_CACHE
#ifdef PERFORMANCE_TEST
		if(stream->totalWrites != 0)
			printf("Write Cache percentage = %ld\n", (stream->writeCacheHits * 100 / stream->totalWrites));
		if(stream->totalReads != 0)
			printf("Read Cache percentage = %ld\n", (stream->readCacheHits * 100 / stream->totalReads));
#endif

		CHECK(omcFlushCache(stream));
		if(stream->cachePtr != NULL)
			omOptFree(main, stream->cachePtr);
		stream->cachePtr = NULL;
		stream->cachePhysSize = 0;
#endif
		
		while(stream->swabProcs != NULL)
		{
			if(stream->swabProcs->swabBuf != NULL)
				omOptFree(main, stream->swabProcs->swabBuf);
			stream->swabProcs->swabBuf = NULL;
			stream->swabProcs->swabBufSize = 0;
			tmp = stream->swabProcs;
			stream->swabProcs = stream->swabProcs->next;
			omOptFree(main, tmp);
		}

		if((stream->blockingSize != 0) && (stream->direction == omcCacheWrite))
		{
			CHECK((*stream->funcs.truncFunc)(stream, stream->fileOffset));
		}
		CHECK((*stream->funcs.closeFunc) (stream));
		stream->cookie = 0;
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************************************************************
 *
 * Convenience functions
 *
 * These call toolkit routines, and may handle status normally
 *
 ************************************************************************/

/************************
 * omcSeekStreamRelative
 *
 * 		Seeks to a relative position on the codec stream.
 *
 * Argument Notes:
 *		Only takes a 32-bit signed offset, which is a number of bytes.  A positive
 *		offset seeks forward in the file, a negative offset seeks towards the start
 *		of the file.
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omcSeekStreamRelative(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfInt32			relPos)	/* IN - seek forward this many bytes */
{
	omfInt64          pos, tmp;

	omfAssert((stream->cookie == STREAM_COOKIE), stream->mainFile, OM_ERR_STREAM_CLOSED);

	XPROTECT(stream->mainFile)
	{
		pos = stream->fileOffset;
		if(relPos < 0)
		{
			omfsCvtInt32toInt64(-1 * relPos , &tmp);
			CHECK(omfsSubInt64fromInt64(tmp, &pos));
		}
		else
		{
			CHECK(omfsAddInt32toInt64(relPos, &pos));
		}
	
		CHECK (omcSeekStreamTo(stream, pos));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * omcSeekStream32
 *
 * 		Seeks to an absolute 32-bit position on the codec stream.
 *		Many codecs (such as TIFF) have 32-bit limitations on the sizes of data
 *		chunks, and may use this function.
 *
 * Argument Notes:
 *		<none>
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omcSeekStream32(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfUInt32			offset)	/* IN - seek to this offset */
{
	omfInt64          pos;

	omfAssert((stream->cookie == STREAM_COOKIE), stream->mainFile, OM_ERR_STREAM_CLOSED);

	XPROTECT(stream->mainFile)
	{
		omfsCvtInt32toInt64(offset, &pos);
		CHECK (omcSeekStreamTo(stream, pos));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * omcGetStreamPos32
 *
 * 		Returns the current position on the codec stream, as a 32-bit number, or
 *		returns an error if the current position is beyond the range of a omfUInt32.
 *		Many codecs (such as TIFF) have 32-bit limitations on the sizes of data
 *		chunks, and may use this function.
 *
 * Argument Notes:
 *		<none>
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_OFFSET_SIZE - The current offset is too large for a omfUInt32.
 */
omfErr_t omcGetStreamPos32(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfUInt32			*offset)	/* OUT - Find the 32-bitr offset */
{
	omfHdl_t        main;
	omfInt64          pos;


	main = stream->mainFile;
	omfAssert((stream->cookie == STREAM_COOKIE), main, OM_ERR_STREAM_CLOSED);
	XPROTECT(main)
	{
		CHECK(omcGetStreamPosition(stream, &pos));
		CHECK(omfsTruncInt64toUInt32(pos, offset));		/* OK API */
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************************************************************
 *
 * Swabbing versions of write/read
 *
 * These functions call a swabbing procedure, which may change the size of
 * a sample, translate samples, and/or byteSwap samples.
 *
 * The standard usage of the swabbing functions would be to open a stream, set up the
 * memory and file formats (sample size, sample rate, format, etc), and then repeatedly
 * compute the number of bytes which may be transfered (due to changes in sample size or rate)
 * and write or read the data.  For example:
 *
 *		omcOpenStream(stream, media->dataObj, OMAIFCData);
 *		omcSetMemoryFormat(stream, memFmt);
 *		omcSetFileFormat(stream, fileFmt, media->swapBytes);
 *
 *		while(moreData)
 *			{
 *			omcComputeBufferSize(stream, fileBytes, &memBytes);
 *			omcWriteSwabbedStream(stream, fileBytes, memBytes, xfer->buffer);
 *			}
 *
 * Alternatively, the data may already be in a buffer.  For example when processing the input
 * to the software codec.  In that case omcSwabData()	can be called directly on the buffer,
 * which is then further processed and then written out with standard codec stream calls.
 *		omcSwabData(stream, &src, &dest, TRUE);
 *
 ************************************************************************/

/************************
 * omcSetMemoryFormat
 *
 * 		Sets the expected in-memory representation of the data for
 *		the swabbing function.
 *
 * Argument Notes:
 *		Fmt takes a media type and audio & video formatting information.  Only
 *		the formatting information for the requested media type need be supplied.
 *		Formatting currently only works for audio & video streams.
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADMEDIATYPE -  (see below).
 */
omfErr_t omcSetMemoryFormat(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfCStrmFmt_t	fmt)		/* IN - set this memory format */
{
	stream->memFormat = fmt;
	return (OM_ERR_NONE);
}

/************************
 * omcSetFileFormat
 *
 * 		Sets the expected file representation of the data for
 *		the swabbing function.
 *
 * Argument Notes:
 *		Fmt takes a media type and audio & video formatting information.  Only
 *		the formatting information for the requested media type need be supplied.
 *		Formatting currently only works for audio & video streams.
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADMEDIATYPE -  (see below).
 */
omfErr_t omcSetFileFormat(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfCStrmFmt_t	fmt,		/* IN - set this file format */
			omfBool			swapBytes)	/* IN - optionally swap the words */
{
	stream->fileFormat = fmt;
	stream->swapBytes = swapBytes;
	return (OM_ERR_NONE);
}

/************************
 * omcComputeBufferSize
 *
 * 		Sets the expected file representation of the data for
 *		the swabbing function.
 *
 * Argument Notes:
 *		Fmt takes a media type and audio & video formatting information.  Only
 *		the formatting information for the requested media type need be supplied.
 *		Formatting currently only works for audio & video streams.
 *
 * ReturnValue:
 *		Error code (see below).  
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_SWAP_SETUP - omcSetMemoryFormat and/or omcSetFileFormat
 *							haven't been called.
 */
omfErr_t omcComputeBufferSize(
			omfCodecStream_t 	*stream,	/* IN - On this media stream */
			omfUInt32			inBufSize,	/* IN - Given a # of bytes in the file */
			omfLengthCalc_t		direct,
			omfUInt32			*result)		/* OUT - Return the memory size */
{
	omfHdl_t     	   	main;
	omfCodecSwabProc_t	*entry;
	
	main = stream->mainFile;

	omfAssert((stream->memFormat.mediaKind != stream->mainFile->nilKind), main, OM_ERR_SWAP_SETUP);
	omfAssert((stream->fileFormat.mediaKind != stream->mainFile->nilKind), main, OM_ERR_SWAP_SETUP);
	omfAssert((stream->cookie == STREAM_COOKIE), main, OM_ERR_STREAM_CLOSED);

	XPROTECT(main)
	{
		/* JEFF2.x:  Find out out to do this correctly */
		if(stream->swabProcs == NULL)
			*result = inBufSize;
		for(entry = stream->swabProcs; entry != NULL; entry = entry->next)
		{
			CHECK((*entry->swabLenProc) (stream, inBufSize, direct, result));
			inBufSize = *result;
		}
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * swabbingNeeded		(INTERNAL)
 *
 * 		Returns TRUE if no translation is required between the file and
 *		memory format.  This allows the routines to short-circuit the
 *		somewhat time intensive swab process and allows the client to
 *		always call the swab versions of the calls when writing data.
 *
 * Argument Notes:
 *		Takes a stream definition, which is internal.
 *
 * ReturnValue:
 *		Boolean indicating need for swabbing.
 *
 * Possible Errors:
 *		<none>.
 */
static omfBool swabbingNeeded(
	omfCodecStream_t	*stream)
{
	omfHdl_t     	   	main;
	omfCodecSwabProc_t	*entry;
	omfErr_t			status;
	omfBool			result;
	
	main = stream->mainFile;

	if(stream->memFormat.mediaKind == stream->mainFile->nilKind)
		return(FALSE);
	if(stream->fileFormat.mediaKind == stream->mainFile->nilKind)
		return(FALSE);
	if(stream->cookie != STREAM_COOKIE)
		return(FALSE);

	result = FALSE;
	for(entry = stream->swabProcs; entry != NULL; entry = entry->next)
	{
		status = (*entry->swabTestProc) (stream, &result);
		if(result == TRUE)
			break;
	}

	return (result);
}

/************************
 * omcSwabData
 *
 * 		Performs the swab functions (including word size & sample
 *		rate changes if specified) between two in-memory buffers.
 *		Used instead of the swabbing write/read functions if the
 *		software codec is being used.  The data needs to be swabbed
 *		before the codec on write, and after the codec on read.
 *
 * Argument Notes:
 *		Takes all of the parameters set up in omcSetMemoryFormat()
 *		and omcSetFileFormat().  Set up all fields in both src and dest
 *		and indicate whether the data needs to be word-swapped.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omcSwabData(
			omfCodecStream_t *stream,		/* IN - On this media stream */
			omfCStrmSwab_t	*src,			/* IN - munge data from this buf & fmt  */
			omfCStrmSwab_t	*dest,			/* IN - into this buf & fmt  */
			omfBool			swapBytes)		/* IN - optionally swapping words */
{
	omfCodecSwabProc_t	*entry;

	omfAssert((stream->cookie == STREAM_COOKIE), stream->mainFile, OM_ERR_STREAM_CLOSED);

	XPROTECT(stream->mainFile)
	{
		for(entry = stream->swabProcs; entry != NULL; entry = entry->next)
		{
			CHECK((*entry->swabProc) (stream, src, dest, swapBytes));
		}
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * omcReadSwabbedStream
 *
 * 		Read bytes from the disk, and translate them using the current
 *		translation, as defined by the memory and disk formats, and by the
 *		swapBytes flag.
 *
 * Argument Notes:
 * 		fileXferLength is the number of bytes in the file.
 *		bufferLength should the number of bytes after translation
 *		to the in-memory format.
 *		bytesRead is the number of bytes read from disk before
 *		translation.  The memory size may be found by using
 *		omcComputeBufferSize() on bytesRead. 
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_SWAP_SETUP -- You must set up file and memory formats before calling
 *							this function.
 *		OM_ERR_MISSING_SWAP_PROC -- Missing a swab routine.
 *		OM_ERR_SMALLBUF -- The buffer given will not hold the number of bytes from
 *							the disk.
 *		OM_ERR_SWABBUFSIZE -- The swap buffer must be a multiple of 4-bytes.
 *		OM_ERR_TRANSLATE -- The swab routine can't handle this translation.
 *		OM_ERR_NOAUDIOCONV -- Attempt to translate to a nonstandard audio sample size.
 */
omfErr_t omcReadSwabbedStream(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfUInt32			fileXferLength,	/* IN - read this much from the file */
			omfUInt32			bufferLength,	/* IN - translate it into this size */
			void			*buffer,		/* IN - In this buffer */
			omfUInt32			*bytesRead)		/* OUT -- and tell how much was read */
{
	omfUInt32          readLen, bytesToXfer, blockSize;
	omfUInt32          actualRead;
	omfCStrmSwab_t  src, dest;
	omfHdl_t        main;
	omfCodecSwabProc_t	*entry;
	omfErr_t		readStat;
	
	main = stream->mainFile;
	omfAssert((stream->memFormat.mediaKind != main->nilKind), main, OM_ERR_SWAP_SETUP);
	omfAssert((stream->fileFormat.mediaKind != main->nilKind), main, OM_ERR_SWAP_SETUP);
	omfAssert((stream->cookie == STREAM_COOKIE), main, OM_ERR_STREAM_CLOSED);

	XPROTECT(main)
	{
		*bytesRead = 0;
		CHECK(omcComputeBufferSize(stream, fileXferLength, omcComputeMemSize, &bytesToXfer));
		
		if (swabbingNeeded(stream))
		{
			/* JEFF2.x: Find out out to do this correctly */
			for(entry = stream->swabProcs; entry != NULL; entry = entry->next)
			{
				XASSERT(entry->swabProc != NULL, OM_ERR_MISSING_SWAP_PROC);
				XASSERT(bufferLength >= bytesToXfer, OM_ERR_SMALLBUF);
				XASSERT((entry->swabBufSize & 0x00000003) == 0, OM_ERR_SWABBUFSIZE);
				CHECK(omcComputeBufferSize(stream, entry->swabBufSize, omfComputeFileSize, &blockSize));
				if(blockSize > entry->swabBufSize)
					blockSize = entry->swabBufSize;
				while (fileXferLength > 0)
				{
					readLen = fileXferLength;
					if (readLen > blockSize)
						readLen = blockSize;
					readStat = omcReadStream(stream, readLen, entry->swabBuf, &actualRead);
					if((readStat != OM_ERR_EOF) && (readStat != OM_ERR_END_OF_DATA) && (readStat != OM_ERR_NONE))
						RAISE(readStat);
						
					src.fmt = stream->fileFormat;
					src.buf = entry->swabBuf;
					src.buflen = actualRead;
					dest.fmt = stream->memFormat;
					dest.buf = buffer;
					CHECK(omcComputeBufferSize(stream, actualRead, omcComputeMemSize,&dest.buflen));
		
					CHECK((*entry->swabProc) (stream, &src,
								     &dest, stream->swapBytes));
		
					buffer = (char *) buffer + dest.buflen;
					fileXferLength -= src.buflen;
					*bytesRead += dest.buflen;
					if(readStat != OM_ERR_NONE)
						RAISE(readStat);
				}
			}
		} else
		{
			CHECK(omcReadStream(stream, bufferLength, buffer, bytesRead));
		}
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * omcWriteSwabbedStream
 *
 * 		Translate them using the current translation, as defined by the memory
 *		and disk formats, and write the bytes to the disk.
 *
 * Argument Notes:
 * 		fileXferLength is the number of bytes in the file.
 *		bufferLength should the number of bytes after translation
 *		to the in-memory format.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_SWAP_SETUP -- You must set up file and memory formats before calling
 *							this function.
 *		OM_ERR_MISSING_SWAP_PROC -- Missing a swab routine.
 *		OM_ERR_SMALLBUF -- The buffer given will not hold the number of bytes from
 *							the disk.
 *		OM_ERR_SWABBUFSIZE -- The swap buffer must be a multiple of 4-bytes.
 *		OM_ERR_TRANSLATE -- The swab routine can't handle this translation.
 *		OM_ERR_NOAUDIOCONV -- Attempt to translate to a nonstandard audio sample size.
 */
omfErr_t  omcWriteSwabbedStream(
			omfCodecStream_t *stream,	/* IN - On this media stream */
			omfUInt32			fileXferLength,	/* IN - write this much to the file */
			omfUInt32			bufferLength,	/* IN - translated from this size */
			void			*buffer)		/* IN - and from this buffer */
{
	omfUInt32          writeLen, memXferLen, blockSize;
	omfCStrmSwab_t  src, dest;
	omfHdl_t        main;
	omfCodecSwabProc_t	*entry;

	main = stream->mainFile;
	omfAssert((stream->memFormat.mediaKind != main->nilKind), main, OM_ERR_SWAP_SETUP);
	omfAssert((stream->fileFormat.mediaKind != main->nilKind), main, OM_ERR_SWAP_SETUP);
	omfAssert((stream->cookie == STREAM_COOKIE), main, OM_ERR_STREAM_CLOSED);

	XPROTECT(main)
	{
		CHECK(omcComputeBufferSize(stream, fileXferLength, omcComputeMemSize, &memXferLen));
	
		if (swabbingNeeded(stream))
		{
			/* JEFF2.x: Find out out to do this correctly */
			for(entry = stream->swabProcs; entry != NULL; entry = entry->next)
			{
				XASSERT(entry->swabProc != NULL, OM_ERR_MISSING_SWAP_PROC);
				XASSERT(bufferLength >= memXferLen, OM_ERR_SMALLBUF);
				XASSERT((entry->swabBufSize & 0x00000003) == 0, OM_ERR_SWABBUFSIZE);
				CHECK(omcComputeBufferSize(stream, entry->swabBufSize, omfComputeFileSize, &blockSize));
				if(blockSize > entry->swabBufSize)
					blockSize = entry->swabBufSize;
				while (fileXferLength > 0)
				{
					writeLen = fileXferLength;
					if (writeLen > blockSize)
						writeLen = blockSize;
		
					src.fmt = stream->memFormat;
					dest.fmt = stream->fileFormat;
					src.buf = buffer;
					CHECK(omcComputeBufferSize(stream, writeLen, omcComputeMemSize, &src.buflen));
					dest.buf = entry->swabBuf;
					dest.buflen = writeLen;
		
					CHECK((*entry->swabProc) (stream, &src,
								     &dest, stream->swapBytes));
			
					CHECK(omcWriteStream(stream, writeLen, entry->swabBuf));
		
					buffer = (char *) buffer + src.buflen;
					fileXferLength -= writeLen;
				}
			}
		} else
		{
			CHECK(omcWriteStream(stream, fileXferLength, buffer));
		}
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
