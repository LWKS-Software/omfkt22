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
 * Name: omcStd.c
 *
 * Function: The standard codec stream handlers which handle transfers to and from
 *			 both Bento objects and raw files.
 *
 * Audience: Toolkit users who need specialized codec streams, and need examples
 *			of working stream handlers.
 */

#include "masterhd.h"
#include <string.h>
#include <stdio.h>

#include "omPublic.h"
#include "omPvt.h" 		/* Needed to check if file is an OMFI file */
#include "omcStd.h" 

struct omfRawStream
{
	CMRefCon		theRefCon;
	HandlerOps	 	hnd;
};
	

static Boolean CM_NEAR buildHandlerVector(MetaHandlerPtr mh,
                                          CMHandlerAddr *handlerAddress, char *operationType)
{
  *handlerAddress = (CMHandlerAddr)(*mh->metaHandler)(NULL, (CMGlobalName)operationType);
  return(*handlerAddress == NULL);
}


typedef struct
{
	omfObject_t     obj;
	omfProperty_t   prop;
	omfType_t		type;
	omfInt64          offset;
}               streamRefData_t;


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
omfErr_t openStdCodecStream(	omfCodecStream_t *stream,
								omfObject_t		obj,
								omfProperty_t	prop,
								omfType_t		type)
{
	streamRefData_t *ref;
	omfRawStream_t	*rs;
	MetaHandlerPtr	metaHandler;
	omfHdl_t		file;
	
	XPROTECT(stream->mainFile)
	{
		if (stream->dataFile->fmt == kOmfiMedia)
		{
			XASSERT(obj != NULL, OM_ERR_NULL_PARAM);
			
			stream->procData = omOptMalloc(stream->mainFile, sizeof(omfCodecStream_t));
			XASSERT(stream->procData != NULL, OM_ERR_NOMEMORY);
	
			ref = (streamRefData_t *) stream->procData;
			ref->obj = obj;
			ref->prop = prop;
			ref->type = type;
			omfsCvtInt32toInt64(0, &ref->offset);
		}
		else
		{
			file = stream->dataFile;
			metaHandler = cmLookupMetaHandler((unsigned char *)"OMFI", (SessionGlobalDataPtr)file->session->BentoSession);
			XASSERT(metaHandler != NULL, OM_ERR_BENTO_HANDLER);
			
			file->rawFile = (struct omfRawStream *)omOptMalloc(file, sizeof(struct omfRawStream));
			rs = file->rawFile;
			stream->procData = file->rawFile;
			rs->theRefCon = createRefConForMyHandlers(file->session->BentoSession, 
												 (char *)file->rawFileDesc, NULL, file);
	
			buildHandlerVector(metaHandler, (CMHandlerAddr *)&rs->hnd.cmfopen, CMOpenOpType);
			buildHandlerVector(metaHandler, (CMHandlerAddr *)&rs->hnd.cmfclose, CMCloseOpType);
			buildHandlerVector(metaHandler, (CMHandlerAddr *)&rs->hnd.cmfflush, CMFlushOpType);
			buildHandlerVector(metaHandler, (CMHandlerAddr *)&rs->hnd.cmfseek, CMSeekOpType);
			buildHandlerVector(metaHandler, (CMHandlerAddr *)&rs->hnd.cmftell, CMTellOpType);
			buildHandlerVector(metaHandler, (CMHandlerAddr *)&rs->hnd.cmfread, CMReadOpType);
			buildHandlerVector(metaHandler, (CMHandlerAddr *)&rs->hnd.cmfwrite, CMWriteOpType);
			buildHandlerVector(metaHandler, (CMHandlerAddr *)&rs->hnd.cmfeof, CMEofOpType);
			buildHandlerVector(metaHandler, (CMHandlerAddr *)&rs->hnd.cmftrunc, CMTruncOpType);
			buildHandlerVector(metaHandler, (CMHandlerAddr *)&rs->hnd.cmgetContainerSize, CMSizeOpType);
		    
		   	if(file->openType == kOmCreate)
				rs->theRefCon = (*rs->hnd.cmfopen)(rs->theRefCon,"wb+"); /* ...open update & trunc */
		   	else if(file->openType == kOmOpenRead)
				rs->theRefCon = (*rs->hnd.cmfopen)(rs->theRefCon,"rb"); /* ...open update & trunc */
			if(rs->theRefCon == NULL)
				RAISE(OM_ERR_BADOPEN);
		}
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
omfErr_t writeStdCodecStream(	omfCodecStream_t *stream,
								omfUInt32			ByteCount,
								void			*Buffer)
{
	streamRefData_t *ref;
	omfUInt32           count;
	
	omfAssert(Buffer, stream->mainFile, OM_ERR_BADDATAADDRESS);

	XPROTECT(stream->mainFile)
	{
		if (stream->dataFile->fmt == kOmfiMedia)
		{
			ref = (streamRefData_t *) stream->procData;
			CHECK(OMWriteProp(stream->dataFile, ref->obj, ref->prop, ref->offset,
								ref->type, ByteCount, Buffer));
			CHECK(omfsAddInt32toInt64(ByteCount, &ref->offset));
			count = ByteCount;
		} else			/* Non-OMFI case */
		{
			count = (*stream->dataFile->rawFile->hnd.cmfwrite)(stream->dataFile->rawFile->theRefCon, Buffer, 1, ByteCount);
		}
	
		XASSERT(count == ByteCount, OM_ERR_EOF);
	}
	XEXCEPT
	XEND

	return(OM_ERR_NONE);
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
omfErr_t readStdCodecStream(	omfCodecStream_t *stream,
								omfUInt32			ByteCount,
								void			*Buffer,
								omfUInt32			*bytesRead )
{
	streamRefData_t	*ref;
	omfUInt32			rsize;
	omfErr_t		status = OM_ERR_NONE, lenStat;
	omfInt32			count;
	omfLength_t           bytes;

	omfAssert(Buffer, stream->mainFile, OM_ERR_BADDATAADDRESS);
	omfAssert(bytesRead, stream->mainFile, OM_ERR_NULL_PARAM);

	XPROTECT(stream->mainFile)
	{
		if(stream->dataFile->fmt == kOmfiMedia)
		{
			ref = (streamRefData_t *)stream->procData;
			status = OMReadProp(stream->dataFile, ref->obj, ref->prop, ref->offset,
								kNeverSwab, ref->type, ByteCount, Buffer);
			if (status == OM_ERR_NONE)
				rsize = ByteCount;
			else
			{
				lenStat = OMLengthProp(stream->dataFile, ref->obj, ref->prop,
										ref->type, 0, &bytes);
				if (lenStat == OM_ERR_NONE)
				{
					CHECK(omfsSubInt64fromInt64(ref->offset, &bytes));
					if(bytes >= 0)
					{
						CHECK(omfsTruncInt64toUInt32(bytes, &rsize));	/* OK MAXREAD */
					}
					else
						rsize = 0;
				}
				else
					rsize = 0;
			}
		
			*bytesRead = rsize;
			XASSERT(rsize == ByteCount, OM_ERR_END_OF_DATA);
			CHECK(omfsAddInt32toInt64(ByteCount, &ref->offset));
		} else		/* Non-OMFI case */
		{
			count = (*stream->dataFile->rawFile->hnd.cmfread)(stream->dataFile->rawFile->theRefCon, Buffer, 1, ByteCount);
			*bytesRead = count;
		}
	
		XASSERT(*bytesRead == ByteCount, OM_ERR_EOF);
	}
	XEXCEPT
	XEND

	return(OM_ERR_NONE);
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
omfErr_t seekStdCodecStream(	omfCodecStream_t *stream,
								omfInt64			position)
{
	streamRefData_t	*ref;

	if(stream->dataFile->fmt == kOmfiMedia)
	{
		ref = (streamRefData_t *)stream->procData;
		ref->offset = position;
	} else
	{
		(*stream->dataFile->rawFile->hnd.cmfseek)(stream->dataFile->rawFile->theRefCon, position, kCMSeekSet);
	}

	return(OM_ERR_NONE);
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
omfErr_t lengthStdCodecStream(	omfCodecStream_t *stream,
								omfInt64			*bytesLeft)
{
	streamRefData_t	*ref;
	omfInt64		savePos;
	
	XPROTECT(stream->mainFile)
	{
		if(stream->dataFile->fmt == kOmfiMedia)
		{
			ref = (streamRefData_t *)stream->procData;
			CHECK(OMLengthProp(stream->dataFile, ref->obj,
								ref->prop, ref->type, 0, bytesLeft));
		} else
		{
			savePos = (*stream->dataFile->rawFile->hnd.cmftell)(stream->dataFile->rawFile->theRefCon);
			*bytesLeft = (*stream->dataFile->rawFile->hnd.cmgetContainerSize)(stream->dataFile->rawFile->theRefCon);
			(*stream->dataFile->rawFile->hnd.cmfseek)(stream->dataFile->rawFile->theRefCon, savePos, kCMSeekSet);
		}
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
omfErr_t truncStdCodecStream(	omfCodecStream_t *stream,
								omfInt64			newSize)
{
	omfRawStream_t	*rawStream;
	CMValue         val;
	CMProperty      cprop;
	CMType          ctype;
	streamRefData_t	*ref;
	CMSize			deleteSize;

	clearBentoErrors(stream->mainFile);
	XPROTECT(stream->mainFile)
	{
		if(stream->dataFile->fmt == kOmfiMedia)
		{
			ref = (streamRefData_t *)stream->procData;
			cprop = CvtPropertyToBento(stream->dataFile, ref->prop);
			ctype = CvtTypeToBento(stream->dataFile, ref->type, NULL);
			XASSERT(cprop != NULL, OM_ERR_BAD_PROP);
			XASSERT(ctype != NULL, OM_ERR_BAD_TYPE);

			if (CMCountValues((CMObject) ref->obj, cprop, ctype))
			{
				val = CMUseValue((CMObject) ref->obj, cprop, ctype);
				if (stream->dataFile->BentoErrorRaised)
					RAISE(OM_ERR_BENTO_PROBLEM);
			} else
				RAISE(OM_ERR_PROP_NOT_PRESENT);
	
			deleteSize =  CMGetValueSize(val);
			omfsSubInt64fromInt64(newSize, &deleteSize);
			CMDeleteValueData(val, newSize,deleteSize);
		}
		else
		{
			rawStream = stream->dataFile->rawFile;
			(*rawStream->hnd.cmftrunc)(rawStream->theRefCon, newSize);
		}
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
omfErr_t fileposStdCodecStream(omfCodecStream_t *stream,
								omfInt64		*result)
{
	streamRefData_t	*ref;
	
	XPROTECT(stream->mainFile)
	{
		if(stream->dataFile->fmt == kOmfiMedia)
		{
			ref = (streamRefData_t *)stream->procData;
			CHECK(OMGetPropertyFileOffset(stream->dataFile, ref->obj,
								ref->prop, ref->offset, ref->type, result));
		} else
		{
			*result = (*stream->dataFile->rawFile->hnd.cmftell)(stream->dataFile->rawFile->theRefCon);
		}
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * numsegStdCodecStream
 *
 * 		Returns the number of physical segments in the underlying implementation.
 *		Used when an application needs to bypass the toolkit read/write functions
 *		of files where the media stream may be fragmented.
 *		For all simple stream implementations (non-Bento) this function will return 1.
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
omfErr_t numsegStdCodecStream(omfCodecStream_t *stream,
								omfInt32		*result)
{
	streamRefData_t	*ref;
	
	XPROTECT(stream->mainFile)
	{
		if(stream->dataFile->fmt == kOmfiMedia)
		{
			ref = (streamRefData_t *)stream->procData;
			CHECK(OMGetNumPropertySegments(stream->dataFile, ref->obj,
						ref->prop, ref->type, result));
		} else
		{
			*result = 1;
		}
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * seginfoStdCodecStream
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
omfErr_t seginfoStdCodecStream(
			omfCodecStream_t *stream,
			omfInt32		index,
			omfPosition_t	*startPos,	/* OUT -- where does the segment begin, */
			omfLength_t		*length)	/* OUT -- and how int  is it? */
{
	streamRefData_t	*ref;
	omfInt64		savePos;
	
	XPROTECT(stream->mainFile)
	{
		if(stream->dataFile->fmt == kOmfiMedia)
		{
			ref = (streamRefData_t *)stream->procData;
			CHECK(OMGetPropertySegmentInfo(stream->dataFile, ref->obj,
								ref->prop,ref->type, index, startPos, length));
		}
		else
		{
	
			omfsCvtInt32toInt64(0, startPos);
			/***/
			savePos = (*stream->dataFile->rawFile->hnd.cmftell)(stream->dataFile->rawFile->theRefCon);
			*length = (*stream->dataFile->rawFile->hnd.cmgetContainerSize)(stream->dataFile->rawFile->theRefCon);
			(*stream->dataFile->rawFile->hnd.cmfseek)(stream->dataFile->rawFile->theRefCon, savePos, kCMSeekSet);
		}
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
omfErr_t closeStdCodecStream(omfCodecStream_t *stream)
{
	omfHdl_t        file, dataFile;

	file = stream->mainFile;
	omfAssertValidFHdl(file);
	if (stream->dataFile != NULL)
	{
		dataFile = stream->dataFile;
		if(dataFile->fmt == kForeignMedia)
		   {
			(*dataFile->rawFile->hnd.cmfclose)(dataFile->rawFile->theRefCon);
		   }
	   	omOptFree(dataFile, stream->procData);
		stream->procData = NULL;
	}
		
	return (OM_ERR_NONE);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
