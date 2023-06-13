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
#include <string.h>

#include "omPublic.h"
#include "omMedia.h"
#include "omcStd.h" 
#include "omcAIFF.h" 
#include "omPvt.h"


typedef struct
{
	void					*buf;
	omfInt32				buflen;
	omfInt32				samplesLeft;
	omfInt32				bytesXfered;
	omfmMultiXfer_t	*xfer;
} interleaveBuf_t;

#define MAX_FMT_OPS	32

typedef struct
{
	omfUInt16          fileBitsPerSample;
	omfRational_t   	fileRate;
	omfInt64          formSizeOffset;
	omfInt64          dataSizeOffset;
	omfInt64          numSamplesOffset;
	omfAudioMemOp_t	fmtOps[MAX_FMT_OPS+1];
	omfAudioMemOp_t	fileFmt[8];
	interleaveBuf_t	*interleaveBuf;
}               userDataAIFF_t;

static omfErr_t codecGetNumChannelsAIFF(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t GetAIFCData(omfCodecStream_t *stream, omfInt32 maxsize, void *data);
static omfErr_t codecIsValidMDESAIFF(omfCodecParms_t * parmblk);
static omfErr_t codecOpenAIFF(omfCodecParms_t *parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecImportRawAIFF(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecCreateAIFF(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecGetAudioInfoAIFF(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecCloseAIFF(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecWriteBlocksAIFF(omfCodecParms_t * parmbl, omfMediaHdl_t media, omfHdl_t maink);
static omfErr_t codecReadBlocksAIFF(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t CreateAudioDataEnd(omfMediaHdl_t media, omfFileRev_t fileRev);
static omfErr_t omfmAudioSetFrameNumber(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t omfmAudioGetFrameOffset(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecPutAudioInfoAIFF(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecGetInfoAIFF(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecPutInfoAIFF(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t writeAIFCHeader(
			omfHdl_t				mainFile,
			omfCodecStream_t	*stream,
 			userDataAIFF_t		*pdata,
			omfInt16				numChPtr,
			omfInt32			numSamples,
			omfBool				isData);
static omfErr_t setupStream(omfCodecStream_t *stream, omfDDefObj_t dataKind, userDataAIFF_t * pdata);
static omfErr_t InitAIFFCodec(omfCodecParms_t *parmblk);
static void SplitBuffers(void *original, omfInt32 srcSamples, omfInt16 sampleSize, omfInt16 numDest, interleaveBuf_t *destPtr);
static omfErr_t readAIFCHeader(omfHdl_t mainFile, omfCodecStream_t *stream,
 			userDataAIFF_t *pdata,		/* OUT */
 			omfInt64		*dataStart,
			omfInt16		*numChPtr,
			omfUInt32		*sampleFrames);
static omfErr_t codecGetSelectInfoAIFF(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecSemCheckAIFF(omfCodecParms_t	*parmblk, omfMediaHdl_t media, omfHdl_t main);

/************************
 * omfCodecAIFF
 *
 * 		Main codec entry routine
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
#if PLUGIN
omfErr_t omfNewCodecAIFF(
			omfCodecFunctions_t	func,
			omfCodecParms_t		*parmblk)
#else
omfErr_t omfCodecAIFF(
			omfCodecFunctions_t	func,
			omfCodecParms_t		*parmblk)
#endif
{
	omfErr_t        	status = OM_ERR_NONE;
	omfCodecOptDispPtr_t	*dispPtr;
	
	switch (func)
	{
	case kCodecInit:
		InitAIFFCodec(parmblk);
		break;

	case kCodecGetMetaInfo:
		strncpy((char *)parmblk->spc.metaInfo.name, "AIFC Codec", parmblk->spc.metaInfo.nameLen);
		parmblk->spc.metaInfo.info.mdesClassID = "AIFD";
		parmblk->spc.metaInfo.info.dataClassID = "AIFC";
		parmblk->spc.metaInfo.info.codecID = CODEC_AIFC_AUDIO;
		parmblk->spc.metaInfo.info.rev = CODEC_REV_3;
		parmblk->spc.metaInfo.info.dataKindNameList = SOUNDKIND;
		parmblk->spc.metaInfo.info.minFileRev = kOmfRev1x;
		parmblk->spc.metaInfo.info.maxFileRevIsValid = FALSE;		/* There is no maximum rev */
		return (OM_ERR_NONE);
			
	case kCodecLoadFuncPointers:
		dispPtr = parmblk->spc.loadFuncTbl.dispatchTbl;

		dispPtr[ kCodecOpen ] = codecOpenAIFF;
		dispPtr[ kCodecCreate ] = codecCreateAIFF;
		dispPtr[ kCodecGetInfo ] = codecGetInfoAIFF;
		dispPtr[ kCodecPutInfo ] = codecPutInfoAIFF;
		dispPtr[ kCodecReadSamples ] = codecReadBlocksAIFF;
		dispPtr[ kCodecWriteSamples ] = codecWriteBlocksAIFF;
		dispPtr[ kCodecClose ] = codecCloseAIFF;
		dispPtr[ kCodecSetFrame ] = omfmAudioSetFrameNumber;
		dispPtr[ kCodecGetFrameOffset ] = omfmAudioGetFrameOffset;
		dispPtr[ kCodecGetNumChannels ] = codecGetNumChannelsAIFF;
		dispPtr[ kCodecGetSelectInfo ] = codecGetSelectInfoAIFF;
		dispPtr[ kCodecImportRaw ] = codecImportRawAIFF;
		dispPtr[ kCodecSemanticCheck ] = codecSemCheckAIFF;
/*
		dispPtr[ kCodecAddFrameIndexEntry ] = NULL;
		dispPtr[ kCodecReadLines ] = NULL;
		dispPtr[ kCodecWriteLines ] = NULL;
		dispPtr[ kCodecInitMDESProps ] = NULL;
		dispPtr[ kCodecGetVarietyInfo ] = NULL;
 		dispPtr[ kCodecGetVarietyCount ] = NULL;
*/
 		return(OM_ERR_NONE);
	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}

	return (status);
}

/************************
 * codecGetInfoAIFF	(INTERNAL)
 *
 */
static omfErr_t codecGetInfoAIFF(omfCodecParms_t * parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfErr_t			status = OM_ERR_NONE;
	omfType_t			dataType;
	omfMaxSampleSize_t	*maxSamplePtr;
	omfFrameSizeParms_t	*samplePtr;
	userDataAIFF_t 		*pdata;
	
	switch (parmblk->spc.mediaInfo.infoType)
	{
	case kMediaIsContiguous:
		dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
		status = OMIsPropertyContiguous(main, media->dataObj,OMAIFCData,
										dataType, (omfBool *)parmblk->spc.mediaInfo.buf);
		break;
	case kAudioInfo:
		status = codecGetAudioInfoAIFF(parmblk, media, main);
		break;
		
	case kMaxSampleSize:
		maxSamplePtr = (omfMaxSampleSize_t *) parmblk->spc.mediaInfo.buf;
		if(maxSamplePtr->mediaKind == media->soundKind)
		{
			pdata = (userDataAIFF_t *)media->userData;
			/* Round up to next byte as per AIFC Spec */
			maxSamplePtr->largestSampleSize = (pdata->fileBitsPerSample + 7) / 8;
		}
		else
			status = OM_ERR_CODEC_CHANNELS;
		break;

	case kSampleSize:
		samplePtr = (omfFrameSizeParms_t *) parmblk->spc.mediaInfo.buf;
		if(samplePtr->mediaKind == media->soundKind)
		{
			pdata = (userDataAIFF_t *)media->userData;
			omfsCvtInt32toInt64(((pdata->fileBitsPerSample + 7)/ 8), &samplePtr->frameSize);
		}
		else
			status = OM_ERR_CODEC_CHANNELS;
		break;

	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}
	
	return(status);
}
/************************
 * codecPutInfoAIFF	(INTERNAL)
 *
 */
static omfErr_t codecPutInfoAIFF(omfCodecParms_t * parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfErr_t			status = OM_ERR_NONE;
	omfAudioMemOp_t 	*aparms;
	omfInt16			numFmtOps, n;
	userDataAIFF_t 		*pdata;
	
	switch (parmblk->spc.mediaInfo.infoType)
	{
	case kAudioInfo:
		status = codecPutAudioInfoAIFF(parmblk, media, main);
		break;
	case kSetAudioMemFormat:
		pdata = (userDataAIFF_t *) media->userData;
		numFmtOps = 0;
		while((pdata->fmtOps[numFmtOps].opcode != kOmfAFmtEnd) && (numFmtOps <= MAX_FMT_OPS))
			numFmtOps++;
		
		if(numFmtOps >= MAX_FMT_OPS)
			status = OM_ERR_TOO_MANY_FMT_OPS;
		else
		{
			aparms = ((omfAudioMemOp_t *)parmblk->spc.mediaInfo.buf);
			for(n = 0 ; ; aparms++, n++)
			{
				pdata->fmtOps[n] =  *((omfAudioMemOp_t *)aparms);
				if(aparms->opcode == kOmfAFmtEnd)
					break;
			}
		}
		break;
	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}
	
	return(status);
}
/************************
 * codecGetNumChannelsAIFF	(INTERNAL)
 *
 *			Find the number of channels in the current media.
 */
static omfErr_t codecGetNumChannelsAIFF(
			omfCodecParms_t	*parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfHdl_t				file;
	omfCodecStream_t	streamData;
	omfType_t			dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
	userDataAIFF_t		pdataBlock;
	omfDDefObj_t		dataKind;
	omfInt16				numCh;
	omfErr_t				status;
	
#ifdef OMFI_ENABLE_STREAM_CACHE
	streamData.cachePtr = NULL;
#endif
	streamData.procData = NULL;

	file = parmblk->spc.getChannels.file;
	XPROTECT(file)
	{
		omfiDatakindLookup(parmblk->spc.getChannels.file, SOUNDKIND, &dataKind, &status);
		CHECK(status);
		if (parmblk->spc.getChannels.mediaKind == dataKind)
		{
			CHECK(omcOpenStream(file, file, &streamData, parmblk->spc.getChannels.mdes,
								OMAIFDSummary, dataType));
			CHECK(setupStream(&streamData, dataKind, &pdataBlock));
			CHECK(readAIFCHeader(parmblk->spc.getChannels.file, &streamData, NULL,
								NULL, &numCh, NULL));
			parmblk->spc.getChannels.numCh = numCh;
			CHECK(omcCloseStream(&streamData));
		}
		else
			parmblk->spc.getChannels.numCh = 0;
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}


/************************
 * codecGetSelectInfoAIFF	(INTERNAL)
 *
 *			Get the information required to select a codec.
 */
static omfErr_t codecGetSelectInfoAIFF(
			omfCodecParms_t	*parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
    omfHdl_t                        file;
    omfCodecStream_t        streamData;
    omfType_t               dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
    userDataAIFF_t          pdataBlock;
    omfDDefObj_t            dataKind;
    omfErr_t                status;

#ifdef OMFI_ENABLE_STREAM_CACHE
	streamData.cachePtr = NULL;
#endif
	streamData.procData = NULL;

   file = parmblk->spc.selectInfo.file;
    XPROTECT(file)
    {
		memset (&pdataBlock, 0, sizeof(pdataBlock)); /* fixes BoundsChecker errors */
        omfiDatakindLookup(file, SOUNDKIND, &dataKind, &status);
        CHECK(status);
        CHECK(omcOpenStream(file, file, &streamData, parmblk->spc.selectInfo.mdes,
                                                OMAIFDSummary, dataType));
        CHECK(setupStream(&streamData, dataKind, &pdataBlock));
        CHECK(readAIFCHeader(file, &streamData, &pdataBlock,
                                                NULL, NULL, NULL));
        parmblk->spc.selectInfo.info.willHandleMDES = TRUE;
#if PORT_BYTESEX_BIG_ENDIAN
        parmblk->spc.selectInfo.info.isNative = TRUE;
#else
        parmblk->spc.selectInfo.info.isNative = FALSE;
#endif
        parmblk->spc.selectInfo.info.hwAssisted = FALSE;
        parmblk->spc.selectInfo.info.relativeLoss = 0;
        parmblk->spc.selectInfo.info.avgBitsPerSec =
        									(pdataBlock.fileBitsPerSample *
        									pdataBlock.fileRate.numerator) /
        									pdataBlock.fileRate.denominator;
        CHECK(omcCloseStream(&streamData));
    }
    XEXCEPT
    XEND

    return(OM_ERR_NONE);
}

/************************
 * InitAIFFCodec	(INTERNAL)
 *
 * 		Initialize the persistant information for between codec calls.
 */
static omfErr_t InitAIFFCodec(
			omfCodecParms_t	*parmblk)
	{
	parmblk->spc.init.persistRtn = NULL;
	return(OM_ERR_NONE);
	}

/************************
 * codecGetAudioInfoAIFF	(INTERNAL)
 *
 * 		Get the audio information for the current media.
 */
static omfErr_t codecGetAudioInfoAIFF(
			omfCodecParms_t 	*parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	userDataAIFF_t *pdata;
	omfAudioMemOp_t *aparms;

	omfAssertMediaHdl(media);

	pdata = (userDataAIFF_t *) media->userData;

		aparms = *((omfAudioMemOp_t **)parmblk->spc.mediaInfo.buf);
	for( ; aparms->opcode != kOmfAFmtEnd; aparms++)
	{
		switch(aparms->opcode)
		{
		case kOmfSampleSize:
			aparms->operand.sampleSize = pdata->fileBitsPerSample;
			break;
		case kOmfSampleRate:
			aparms->operand.sampleRate = pdata->fileRate;
			break;

		case kOmfSampleFormat:
			aparms->operand.format = kOmfSignedMagnitude;
			break;

		case kOmfNumChannels:
			aparms->operand.numChannels = media->numChannels;
			break;
		}
	}
	
	return (OM_ERR_NONE);
}

/************************
 * codecPutAudioInfoAIFF	(INTERNAL)
 *
 * 		Sets the audio information for the current media.
 */
static omfErr_t codecPutAudioInfoAIFF(
			omfCodecParms_t	*parmblk,
		omfMediaHdl_t 			media,
		omfHdl_t				main)
{
	userDataAIFF_t		*pdata;
	omfAudioMemOp_t	*aparms;
	omfType_t			dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
	omfPosition_t		zero = omfsCvtInt32toLength(0, zero);

	omfAssertMediaHdl(media);

	XPROTECT(main)
	{
		pdata = (userDataAIFF_t *) media->userData;
	
		aparms = *((omfAudioMemOp_t **)parmblk->spc.mediaInfo.buf);
		for( ; aparms->opcode != kOmfAFmtEnd; aparms++)
		{
			switch(aparms->opcode)
			{
			case kOmfSampleSize:
				pdata->fileBitsPerSample = aparms->operand.sampleSize;
				break;
				
			case kOmfSampleRate:
				pdata->fileRate = aparms->operand.sampleRate;
				break;
	
			case kOmfSampleFormat:			/* Fixed sample format */
				RAISE(OM_ERR_INVALID_OP_CODEC);
				break;
	
			case kOmfNumChannels:
				media->numChannels = aparms->operand.numChannels;
				break;
			}
		}
		
		CHECK(omcCloseStream(media->stream));	/* Close the existing data
							 * stream */
		if(main->fmt == kOmfiMedia)
		{
			CHECK(omcOpenStream(main, media->dataFile, media->stream, media->mdes, OMAIFDSummary, dataType));
			CHECK(setupStream(media->stream, media->soundKind, pdata));
		
			CHECK(writeAIFCHeader(main, media->stream, pdata, media->numChannels, 0, FALSE));
			CHECK(omcCloseStream(media->stream));
		}
		
		CHECK(omcOpenStream(main, media->dataFile, media->stream, media->dataObj, OMAIFCData, dataType));
		CHECK(setupStream(media->stream, media->soundKind, pdata));
	
		CHECK(writeAIFCHeader(main, media->stream, pdata, media->numChannels, 0, TRUE));
		CHECK(omcGetStreamPosition(media->stream, &media->dataStart));
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * readAIFCHeader	(INTERNAL)
 *
 * 		Read the AIFC header for the current media, and return
 *			the parameters.
 */
static omfErr_t readAIFCHeader(
			omfHdl_t				mainFile,
			omfCodecStream_t	*stream,
 			userDataAIFF_t		*pdata,
 			omfInt64			*dataStart,
			omfInt16				*numChPtr,
			omfUInt32			*sampleFrames)
{
	omfInt64     	offset, chunkStart;
	omfUInt16     	bitsPerSample;
	omfInt16			numCh;
	char        	chunkID[4];
	omfBool       	commFound = FALSE, ssndFound = FALSE;
	char          	ieeerate[10];
	omfUInt32     	bytesPerFrame, chunkSize;
	omfInt32			ssndOffset, ssndBlocksize;
 	userDataAIFF_t	localPdata;
	omfUInt32		localSampleFrames;
 	

	if(sampleFrames == NULL)
		sampleFrames = &localSampleFrames;
	if(pdata == NULL)
		pdata = &localPdata;
	if(dataStart == NULL)
		ssndFound = TRUE;
			
	pdata->fmtOps[0].opcode = kOmfAFmtEnd;
	pdata->interleaveBuf = NULL;
	
	XPROTECT(mainFile)
	{
		omfsCvtInt32toInt64(0, &pdata->formSizeOffset);
		omfsCvtInt32toInt64(0, &pdata->dataSizeOffset);
		omfsCvtInt32toInt64(0, &pdata->numSamplesOffset);
	
		/*
		 * Start at offset 12 in the AIFC data, skipping past the FORM
		 * chunk's ckID, ckDataSize and formType.
		 */	
		omfsCvtInt32toInt64(12L, &offset);
		CHECK(omcSeekStreamTo(stream, offset));
	
		numCh = 0;
		while (omcReadStream(stream, 4L, chunkID, NULL) == OM_ERR_NONE)
		{
			CHECK(GetAIFCData(stream, 4L, (void *) &chunkSize));	
			CHECK(omcGetStreamPosition(stream, &chunkStart));
	
			if (strncmp(chunkID, "COMM", (size_t) 4) == 0)
			{
				CHECK(GetAIFCData(stream, 2L, (void *) &numCh));
				CHECK(GetAIFCData(stream, 4L, (void *) sampleFrames));
				
				CHECK(GetAIFCData(stream, 2L, (void *) &bitsPerSample));
				bytesPerFrame = ((bitsPerSample+7)/ 8) * numCh;
				pdata->fileBitsPerSample = bitsPerSample;
	
				CHECK(GetAIFCData(stream, 10L, (void *) ieeerate));
				pdata->fileRate = RationalFromFloat(
										ConvertFromIeeeExtended(ieeerate));
				commFound = TRUE;
			} else if (strncmp(chunkID, "SSND", (size_t) 4) == 0)
			{
				if(dataStart != NULL)
				{
					/* Positioned at beginning of audio data */
					CHECK(omcGetStreamPosition(stream, dataStart));
					CHECK(GetAIFCData(stream, 4L, (void *) &ssndOffset));
					CHECK(GetAIFCData(stream, 4L, (void *) &ssndBlocksize));
					if((ssndOffset < 0) || (ssndOffset > 128L*1024L))
						ssndOffset = 0;			/* Some apps wrote garbage here */
					/* Skip over ssndoffset and blocksize */
					CHECK(omfsAddInt32toInt64(ssndOffset+8, dataStart));
				}
				
				ssndFound = TRUE;
			}
			offset = chunkStart;
			CHECK(omfsAddInt32toInt64(chunkSize, &offset));
			CHECK(omcSeekStreamTo(stream, offset));
	
			if (commFound && ssndFound)	/* Do we have all information yet? */
				break;
		}
	}
	XEXCEPT
	{
		RERAISE(OM_ERR_BADAIFCDATA);
	}
	XEND
	
	if(numChPtr != NULL)
		*numChPtr = numCh;
	
	return(OM_ERR_NONE);
}

/************************
 * codecOpenAIFF	(INTERNAL)
 *
 * 		Open an AIFC file.
 */
static omfErr_t codecOpenAIFF(
			omfCodecParms_t	*parmblk,
		omfMediaHdl_t 			media,
		omfHdl_t				main)
{
	userDataAIFF_t 		*pdata;
	omfUInt16				index;
	omfUInt32				sampleFrames;
	omfType_t				dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);

	omfAssertMediaHdl(media);
	/* Find COMM and SSND chunks to extract data, skip other chunks */
	XPROTECT(main)
	{
		media->userData = (userDataAIFF_t *) omOptMalloc(main, sizeof(userDataAIFF_t));
		if (media->userData == NULL)
			RAISE(OM_ERR_NOMEMORY);
		
		pdata = (userDataAIFF_t *) media->userData;
		pdata->fmtOps[0].opcode = kOmfAFmtEnd;
		pdata->fileBitsPerSample = 0;
		pdata->interleaveBuf = NULL;
		
		CHECK(omcOpenStream(main, media->dataFile, media->stream, media->dataObj,
							OMAIFCData, dataType));
		CHECK(setupStream(media->stream, media->soundKind, pdata));

		CHECK(readAIFCHeader(main, media->stream, pdata,
							&media->dataStart, &media->numChannels, &sampleFrames));
		
		for (index = 0; index < media->numChannels; index++)
		{
			media->channels[index].dataOffset = media->dataStart;
			omfsCvtInt32toInt64(sampleFrames, &media->channels[index].numSamples);
		}
	
		CHECK(setupStream(media->stream, media->soundKind, pdata));
		CHECK(omcSeekStreamTo(media->stream, media->dataStart));
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * codecCloseAIFF	(INTERNAL)
 *
 * 		Close an AIFC file.
 */
static omfErr_t codecCloseAIFF(
			omfCodecParms_t	*parmblk,
		omfMediaHdl_t 			media,
		omfHdl_t				main)
{
	userDataAIFF_t *pdata;

	omfAssertMediaHdl(media);
	pdata = (userDataAIFF_t *) media->userData;

	XPROTECT(main)
	{
		if((media->openType == kOmfiCreated) || (media->openType == kOmfiAppended))
		{
			CHECK(CreateAudioDataEnd(media, parmblk->fileRev));
		}
		CHECK(omcCloseStream(media->stream));
	
		if(pdata->interleaveBuf != NULL)
			omOptFree(main, pdata->interleaveBuf);
		omOptFree(main, media->userData);
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * omfmAudioSetFrameNumber	(INTERNAL)
 *
 * 		Seek to a given read position.
 */
static omfErr_t omfmAudioSetFrameNumber(
			omfCodecParms_t	*parmblk,
		omfMediaHdl_t 			media,
		omfHdl_t				main)
{
	omfInt64    	nBytes, frame;
	omfInt64     	temp, offset, one;
	omfInt32       bytesPerFrame;
	userDataAIFF_t *pdata;

	omfsCvtInt32toInt64(1, &one);
	omfAssertMediaHdl(media);
	
	XPROTECT(main)
	{
		pdata = (userDataAIFF_t *) media->userData;

		temp = media->channels[0].numSamples;
		CHECK(omfsAddInt32toInt64(1, &temp));
		if(omfsInt64Greater(parmblk->spc.setFrame.frameNumber, temp))
			RAISE(OM_ERR_BADSAMPLEOFFSET);
	
		frame = parmblk->spc.setFrame.frameNumber;
		
		/* Make the result zero-based (& check for bad frame numbers as well). */
		if(omfsInt64Less(frame, one))
			RAISE(OM_ERR_BADSAMPLEOFFSET);
		CHECK(omfsSubInt64fromInt64(one, &frame));
		bytesPerFrame = ((pdata->fileBitsPerSample + 7) / 8) * media->numChannels;
		CHECK(omfsMultInt32byInt64(bytesPerFrame, frame, &nBytes));
		offset = media->dataStart;
		CHECK(omfsAddInt64toInt64(nBytes, &offset));
	
		omcSeekStreamTo(media->stream, offset);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * omfmAudioGetFrameOffset	(INTERNAL)
 *
 * 		find a given read position.
 */
static omfErr_t omfmAudioGetFrameOffset(
			omfCodecParms_t	*parmblk,
		omfMediaHdl_t 			media,
		omfHdl_t				main)
{
	omfInt64    	nBytes, frame;
	omfInt64     	temp, offset, one;
	omfInt32       bytesPerFrame;
	userDataAIFF_t *pdata;

	omfsCvtInt32toInt64(1, &one);
	omfAssertMediaHdl(media);
	
	XPROTECT(main)
	{
		pdata = (userDataAIFF_t *) media->userData;

		temp = media->channels[0].numSamples;
		CHECK(omfsAddInt32toInt64(1, &temp));
		if(omfsInt64Greater(parmblk->spc.getFrameOffset.frameNumber, temp))
			RAISE(OM_ERR_BADSAMPLEOFFSET);
	
		frame = parmblk->spc.getFrameOffset.frameNumber;
		
		/* Make the result zero-based (& check for bad frame numbers as well). */
		if(omfsInt64Less(frame, one))
			RAISE(OM_ERR_BADSAMPLEOFFSET);
		CHECK(omfsSubInt64fromInt64(one, &frame));
		bytesPerFrame = ((pdata->fileBitsPerSample + 7) / 8) * media->numChannels;
		CHECK(omfsMultInt32byInt64(bytesPerFrame, frame, &nBytes));
		offset = media->dataStart;
		CHECK(omfsAddInt64toInt64(nBytes, &offset));
	
		parmblk->spc.getFrameOffset.frameOffset = offset;
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * GetAIFCData	(INTERNAL)
 *
 * 		Read maxsize bytes from the stream.
 */
static omfErr_t GetAIFCData(
			omfCodecStream_t	*stream,
			omfInt32				maxsize,
			void					*data)
{
	XPROTECT(stream->mainFile)
	{
		CHECK(omcReadStream(stream, maxsize, data, NULL));
	
		if ((maxsize == sizeof(omfInt32)) && stream->swapBytes)
			omfsFixLong((omfInt32 *) data);
		else if ((maxsize == sizeof(omfInt16)) && stream->swapBytes)
			omfsFixShort((omfInt16 *) data);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * PutAIFCData	(INTERNAL)
 *
 * 		Write the maxsize bytes onto the stream.
 */
static omfErr_t PutAIFCData(
			omfCodecStream_t	*stream,
			omfInt32				maxsize,
			void					*data)
{
	omfInt32           dataL;
	omfInt16           dataS;

	XPROTECT(stream->mainFile)
	{
		if ((maxsize == sizeof(omfInt32)) && stream->swapBytes)
		{
			dataL = *((omfInt32 *) data);
			omfsFixLong(&dataL);
			data = &dataL;
		} else if ((maxsize == sizeof(omfInt16)) && stream->swapBytes)
		{
			dataS = *((omfInt16 *) data);
			omfsFixShort(&dataS);
			data = &dataS;
		}
		CHECK(omcWriteStream(stream, maxsize, data));
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * PutAIFCDataAt	(INTERNAL)
 *
 * 		Write the maxsize bytes at a given position on the stream.
 */
static omfErr_t PutAIFCDataAt(
			omfCodecStream_t	*stream,
			omfInt64			offset,
			omfInt32				maxsize,
			void					*data)
{
	XPROTECT(stream->mainFile)
	{
		CHECK(omcSeekStreamTo(stream, offset));
		CHECK(PutAIFCData(stream, maxsize, data));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * codecReadBlocksAIFF	(INTERNAL)
 *
 * 		Read blocks of data from the stream.
 */
static omfErr_t codecReadBlocksAIFF(
			omfCodecParms_t	*parmblk,
		omfMediaHdl_t 			media,
		omfHdl_t				main)
{
	omfUInt32       	nbytes, fileBytes, memBytes, startBuflen;
	omfInt32        	bytesPerSample, n, xferSamples, sub;
	omfUInt32			maxSamplesLeft;
	char					tmpBuf[256];
	char					*start;
	userDataAIFF_t		*pdata;
	omfmMultiXfer_t	*xfer;
	omfInt16				memBitsPerSample, ch, xf;
		
	omfAssertMediaHdl(media);
	XPROTECT(main)
	{
		pdata = (userDataAIFF_t *) media->userData;
		XASSERT(pdata->fileBitsPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		memBitsPerSample = pdata->fileBitsPerSample;
		for(n = 0; pdata->fmtOps[n].opcode != kOmfAFmtEnd; n++)
			if(pdata->fmtOps[n].opcode == kOmfSampleSize)
				memBitsPerSample = pdata->fmtOps[n].operand.sampleSize;

		for (n = 0; n < parmblk->spc.mediaXfer.numXfers; n++)
			parmblk->spc.mediaXfer.xfer[n].bytesXfered = 0;
	
		if(parmblk->spc.mediaXfer.inter == leaveInterleaved)
		{
			bytesPerSample = ((pdata->fileBitsPerSample + 7)/ 8);
			for (n = 0; n < parmblk->spc.mediaXfer.numXfers; n++)
			{
				xfer = parmblk->spc.mediaXfer.xfer + n;
				xfer->samplesXfered = 0;
	
				fileBytes = xfer->numSamples * bytesPerSample * media->numChannels;
				CHECK(omcComputeBufferSize(media->stream, fileBytes, omcComputeMemSize, &memBytes));
				if (memBytes > xfer->buflen)
					RAISE(OM_ERR_SMALLBUF);
		
				CHECK(omcReadSwabbedStream(media->stream, fileBytes, memBytes, xfer->buffer, &nbytes));
				xfer->bytesXfered = nbytes;
				xfer->samplesXfered = nbytes / (bytesPerSample * media->numChannels);
			}
		}
		else if(media->numChannels == 1)
		{
			bytesPerSample = ((pdata->fileBitsPerSample+7) / 8);
			for (n = 0; n < parmblk->spc.mediaXfer.numXfers; n++)
			{
				xfer = parmblk->spc.mediaXfer.xfer + n;
				xfer->samplesXfered = 0;
			
				fileBytes = xfer->numSamples * bytesPerSample;
				CHECK(omcComputeBufferSize(media->stream, fileBytes, omcComputeMemSize, &memBytes));
				if (memBytes > xfer->buflen)
					RAISE(OM_ERR_SMALLBUF);
		
				CHECK(omcReadSwabbedStream(media->stream, fileBytes, memBytes, xfer->buffer, &nbytes));
				xfer->bytesXfered = nbytes;
				xfer->samplesXfered = nbytes / bytesPerSample;
			}
		}
		else
		{
			if(pdata->interleaveBuf == NULL)
				pdata->interleaveBuf = (interleaveBuf_t *)omOptMalloc(main, media->numChannels * sizeof(interleaveBuf_t));
			if(pdata->interleaveBuf == NULL)
				RAISE(OM_ERR_NOMEMORY);
			bytesPerSample = ((pdata->fileBitsPerSample+7) / 8);
			for (n = 0; n < media->numChannels; n++)
			{
				pdata->interleaveBuf[n].buf = NULL;
				pdata->interleaveBuf[n].xfer = NULL;
			}
			
			maxSamplesLeft = 0;
			for (n = 0; n < parmblk->spc.mediaXfer.numXfers; n++)
			{
				xfer = parmblk->spc.mediaXfer.xfer + n;
				
				if((xfer->subTrackNum <= 0) || (xfer->subTrackNum > media->numChannels))
					RAISE(OM_ERR_CODEC_CHANNELS);
				sub = xfer->subTrackNum-1;
				if(pdata->interleaveBuf[sub].buf != NULL)
					RAISE(OM_ERR_XFER_DUPCH);
				pdata->interleaveBuf[sub].buf = xfer->buffer;
				pdata->interleaveBuf[sub].samplesLeft = xfer->numSamples;
				pdata->interleaveBuf[sub].buflen = xfer->buflen;
				pdata->interleaveBuf[sub].bytesXfered = 0;
				pdata->interleaveBuf[sub].xfer = xfer;
				xfer->bytesXfered  = 0;
				xfer->samplesXfered  = 0;
				if(xfer->numSamples > maxSamplesLeft)
					maxSamplesLeft = xfer->numSamples;
			}
					
			for (n = 0, start = NULL; (n < media->numChannels) && (start == NULL); n++)
			{
				if(pdata->interleaveBuf[n].buf != NULL)
				{
					start = (char *)pdata->interleaveBuf[n].buf;
					startBuflen = pdata->interleaveBuf[n].buflen;
				}
			}
			if(start == NULL)
				RAISE(OM_ERR_BADDATAADDRESS);
			
			while(maxSamplesLeft > 0)
			{
				if(maxSamplesLeft * bytesPerSample * media->numChannels >= sizeof(tmpBuf))
				{
					xferSamples = maxSamplesLeft / media->numChannels;
					/* !!! Make sure that xferSamples is a multiple of bitsPerSample */
				}
				else
				{
					xferSamples = maxSamplesLeft;
					start = tmpBuf;
				}
				fileBytes = xferSamples * bytesPerSample * media->numChannels;
				CHECK(omcComputeBufferSize(media->stream, fileBytes, omcComputeMemSize, &memBytes));
				if (memBytes > startBuflen)
					RAISE(OM_ERR_SMALLBUF);
		
				CHECK(omcReadSwabbedStream(media->stream, fileBytes, memBytes, start, &nbytes));
				
				SplitBuffers(start, xferSamples * media->numChannels, memBitsPerSample,
								media->numChannels, pdata->interleaveBuf);
				start += memBytes / media->numChannels;
				maxSamplesLeft -= xferSamples;

				for (ch = 0; ch < media->numChannels; ch++)
				{
					xfer = pdata->interleaveBuf[ch].xfer;
					if(xfer != NULL)
					{
						xfer->bytesXfered = pdata->interleaveBuf[ch].bytesXfered;
						xfer->samplesXfered = xfer->bytesXfered / bytesPerSample;
					}
				}
			}
		}
	}
	XEXCEPT
	{
		if((XCODE() == OM_ERR_EOF) || (XCODE() == OM_ERR_END_OF_DATA))
		{
			for (xf = 0; xf < parmblk->spc.mediaXfer.numXfers; xf++)
			{
				xfer = parmblk->spc.mediaXfer.xfer + xf;
				xfer->bytesXfered = nbytes;
				xfer->samplesXfered = nbytes / (bytesPerSample * media->numChannels);
			}
		}
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * codecWriteBlocksAIFF	(INTERNAL)
 *
 * 		Write blocks of data onto the stream.
 */
static omfErr_t codecWriteBlocksAIFF(
			omfCodecParms_t	*parmblk,
		omfMediaHdl_t 			media,
		omfHdl_t				main)
{
	omfUInt32      	fileBytes, memBytes, memBytesPerSample;
	omfInt32      		bytesPerSample, n, ch, xfers, samp;
	omfUInt32			maxSamplesLeft;
	userDataAIFF_t 	*pdata;
	omfmMultiXfer_t	*xfer;
	interleaveBuf_t	*inter;
	char					*destPtr;
	char					sampleBuf[256];
	
	omfAssertMediaHdl(media);
	XPROTECT(main)
	{
		pdata = (userDataAIFF_t *) media->userData;
		XASSERT(pdata->fileBitsPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		bytesPerSample = ((pdata->fileBitsPerSample+7) / 8);
		for (n = 0; n < parmblk->spc.mediaXfer.numXfers; n++)
			parmblk->spc.mediaXfer.xfer[n].bytesXfered = 0;
	
		if(parmblk->spc.mediaXfer.inter == leaveInterleaved)
		{
			for (n = 0; n < parmblk->spc.mediaXfer.numXfers; n++)
			{
				xfer = parmblk->spc.mediaXfer.xfer + n;
				XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
		
				fileBytes = xfer->numSamples * bytesPerSample * media->numChannels;
				CHECK(omcComputeBufferSize(media->stream, fileBytes, omcComputeMemSize, &memBytes));
				if (memBytes > xfer->buflen)
					RAISE(OM_ERR_SMALLBUF);
		
				CHECK(omcWriteSwabbedStream(media->stream, fileBytes, memBytes, xfer->buffer));
		
				for(ch = 0; ch < media->numChannels; ch++)
				{
					CHECK(omfsAddInt32toInt64(xfer->numSamples, &media->channels[ch].numSamples));
				}
			}
		}
		else if(media->numChannels == 1)
		{
			for (n = 0; n < parmblk->spc.mediaXfer.numXfers; n++)
			{
				xfer = parmblk->spc.mediaXfer.xfer + n;
		
				fileBytes = xfer->numSamples * bytesPerSample;
				CHECK(omcComputeBufferSize(media->stream, fileBytes, omcComputeMemSize, &memBytes));
				if (memBytes > xfer->buflen)
					RAISE(OM_ERR_SMALLBUF);
		
				CHECK(omcWriteSwabbedStream(media->stream, fileBytes, memBytes, xfer->buffer));
		
				CHECK(omfsAddInt32toInt64(xfer->numSamples, &media->channels[0].numSamples));
			}
		}
		else
		{
			if(pdata->interleaveBuf == NULL)
				pdata->interleaveBuf = (interleaveBuf_t *)omOptMalloc(main, media->numChannels * sizeof(interleaveBuf_t));
			if(pdata->interleaveBuf == NULL)
				RAISE(OM_ERR_NOMEMORY);
			bytesPerSample = ((pdata->fileBitsPerSample + 7) / 8);
			for (n = 0; n < media->numChannels; n++)
				pdata->interleaveBuf[n].buf = NULL;
			
			maxSamplesLeft = 0;
			for (n = 0; n < parmblk->spc.mediaXfer.numXfers; n++)
			{
				xfer = parmblk->spc.mediaXfer.xfer + n;
				xfer->bytesXfered = xfer->numSamples * bytesPerSample;
				xfer->samplesXfered = xfer->numSamples;
				
				if((xfer->subTrackNum <= 0) || (xfer->subTrackNum > media->numChannels))
					RAISE(OM_ERR_CODEC_CHANNELS);
				inter = pdata->interleaveBuf + (xfer->subTrackNum-1);
				XASSERT(inter->buf == NULL, OM_ERR_XFER_DUPCH);
				inter->buf = xfer->buffer;
				inter->samplesLeft = xfer->numSamples;
				inter->buflen = xfer->buflen;
				inter->bytesXfered = 0;
				if(maxSamplesLeft == 0)
					maxSamplesLeft = xfer->numSamples;
				else if(xfer->numSamples != maxSamplesLeft)
					RAISE(OM_ERR_MULTI_WRITELEN);
			}
			
	
			CHECK(omcComputeBufferSize(media->stream, bytesPerSample, omcComputeMemSize, &memBytesPerSample));

			while(maxSamplesLeft > 0)
			{
				xfers = sizeof(sampleBuf) / (memBytesPerSample * parmblk->spc.mediaXfer.numXfers);
				if((omfUInt32) xfers > maxSamplesLeft)
					xfers = (omfInt32) maxSamplesLeft;
					
				destPtr = sampleBuf;
				for (samp = 0; samp < xfers; samp++)
				{
					for (n = 0; n < parmblk->spc.mediaXfer.numXfers; n++)
					{
						inter = pdata->interleaveBuf+n;
						memcpy(destPtr, inter->buf, memBytesPerSample);
						inter->buf = (char *)inter->buf + memBytesPerSample;
						destPtr += memBytesPerSample;
						inter->samplesLeft--;
						inter->bytesXfered += memBytesPerSample;
					}
					maxSamplesLeft--;
				}
				
				memBytes = xfers * memBytesPerSample * parmblk->spc.mediaXfer.numXfers;
				CHECK(omcComputeBufferSize(media->stream, memBytes, omfComputeFileSize, &fileBytes));
				CHECK(omcWriteSwabbedStream(media->stream, fileBytes, memBytes, sampleBuf));
			}

			for (n = 0; n < parmblk->spc.mediaXfer.numXfers; n++)
			{
				xfer = parmblk->spc.mediaXfer.xfer + n;
				CHECK(omfsAddInt32toInt64(xfer->numSamples, &media->channels[xfer->subTrackNum-1].numSamples));
			}
		}
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * ComputeWriteChunkSize	(INTERNAL)
 *
 * 		Compute the size of the data chunk & patch the length field.
 */
static omfErr_t ComputeWriteChunkSize(
			omfHdl_t				main,
			omfCodecStream_t	*stream,
			omfInt64				sizeOff,
			omfInt64				end)
{
	omfInt64		tmpOffset, savePos, result;
	omfUInt32	size;

	XPROTECT(main)
	{
		CHECK(omcGetStreamPosition(stream, &savePos));
	
		tmpOffset = sizeOff;
		CHECK(omfsAddInt32toInt64(4L, &tmpOffset));
		result = end;
		CHECK(omfsSubInt64fromInt64(tmpOffset, &result));
		CHECK(omfsTruncInt64toUInt32(result, &size));		/* OK AIFC */
		CHECK(PutAIFCDataAt(stream, sizeOff, 4L, &size));
		CHECK(omcSeekStreamTo(stream, savePos));
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * CreateAudioDataEnd	(INTERNAL)
 *
 * 		Handle updating the fields after writing the data.
 */
static omfErr_t CreateAudioDataEnd(
			omfMediaHdl_t		media,
			omfFileRev_t		fileRev)
{
	omfUInt32		zero = 0, offsetLSW, nsamp, testLong;
	omfInt64		curOffset;
	userDataAIFF_t *pdata;
	omfHdl_t		main;
	omfType_t		dataType = (fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
	omfCodecStream_t localStream;	

	/* this routine will be called after sample data is written */
	/* Now set the patches for all the fields of the aifc data */

	pdata = (userDataAIFF_t *) media->userData;
	omfAssertMediaHdl(media);
	main = media->mainFile;
#ifdef OMFI_ENABLE_STREAM_CACHE
	localStream.cachePtr = NULL;
#endif
	localStream.procData = NULL;

	XPROTECT(main)
	{
		CHECK(omcGetStreamPosition(media->stream, &curOffset));
	
		/*
		 * Don't worrk about truncation errors, as we only need lo look at
		 * the LSB
		 */
		omfsTruncInt64toUInt32(curOffset, &offsetLSW);	/* OK AIFC */
	
		if (offsetLSW % 2 == 1)	/* if odd, write a pad byte */
			omcWriteStream(media->stream, 1, &zero);
	
		CHECK(ComputeWriteChunkSize(main, media->stream, pdata->formSizeOffset, curOffset));
		CHECK(ComputeWriteChunkSize(main, media->stream, pdata->dataSizeOffset, curOffset));
	
		CHECK(omfsTruncInt64toUInt32(media->channels[0].numSamples, &nsamp));	/* OK AIFC */
	
		if (omfsTruncInt64toUInt32(pdata->numSamplesOffset, &testLong) == OM_ERR_NONE && testLong != 0)	/* OK AIFC */
		{
			CHECK(PutAIFCDataAt(media->stream, pdata->numSamplesOffset, 4L, &nsamp));
		}
		if(main->fmt == kOmfiMedia)
		{
			CHECK(omcOpenStream(main, media->dataFile, &localStream, media->mdes, OMAIFDSummary, dataType));
			CHECK(setupStream(&localStream, media->soundKind, pdata));
		
			CHECK(writeAIFCHeader(main, &localStream, pdata, media->numChannels, nsamp, FALSE));
			CHECK(omcCloseStream(&localStream));
		}
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * writeAIFCHeader	(INTERNAL)
 *
 * 		Creates AIFC information in the given object. If the object is a
 * 		descriptor (AIFD), create the SSND chunk header but obviously don't write
 * 		the samples.
 */
static omfErr_t writeAIFCHeader(
			omfHdl_t				main,
			omfCodecStream_t	*stream,
 			userDataAIFF_t		*pdata,
			omfInt16				numCh,
			omfInt32			numSamples,
			omfBool				isData)
{
	char            	tempstr[12];
	double      		tempdbl;
	omfInt16      		bps;
	char         		ss;
	omfInt32          zero = 0;
	omfInt64			savePos, COMMsizeoffset;

	XPROTECT(main)
	{
		CHECK(omcSeekStream32(stream, 0));
	
		CHECK(omcWriteStream(stream, 4L, (void *) "FORM"));
		CHECK(omcGetStreamPosition(stream, &pdata->formSizeOffset));
		CHECK(omcWriteStream(stream, 4L, (void *) &zero));
		CHECK(omcWriteStream(stream, 4L, (void *) "AIFC"));
		CHECK(omcWriteStream(stream, 4L, (void *) "COMM"));
	
		/*
		 * Even though, currently, COMM size is constant, compression
		 * specification may be variable (pascal string).  Therefore, use an
		 * adjustable mechanism for size computation
		 */
	
		CHECK(omcGetStreamPosition(stream, &COMMsizeoffset));
		CHECK(omcWriteStream(stream, 4L, (void *) &zero));
	
		CHECK(PutAIFCData(stream, 2L, (void *) &numCh));
	
		CHECK(omcGetStreamPosition(stream, &pdata->numSamplesOffset));
		CHECK(omcWriteStream(stream, 4L, (void *) &zero));
	
		bps = pdata->fileBitsPerSample;
		CHECK(PutAIFCData(stream, 2L, (void *) &bps));
	
		tempdbl = FloatFromRational(pdata->fileRate);
		ConvertToIeeeExtended(tempdbl, tempstr);
	
		CHECK(omcWriteStream(stream, 10L, (void *) &tempstr));
	
		CHECK(omcWriteStream(stream, 4L, (void *) "NONE"));
	
		/* To write out a Pascal string, write a byte for the size, then */
		/* the characters, then, in this case, a pad byte to even out the */
		/* whole thing to 16 bytes. */
	
		ss = 14;
		CHECK(PutAIFCData(stream, 1L, (void *) &ss));
		CHECK(omcWriteStream(stream, 14L, (void *) "Not Compressed"));
	
		CHECK(omcWriteStream(stream, 1L, (void *) &zero));
	
		/* COMM chunk done, so patch up size field */
		CHECK(omcGetStreamPosition(stream, &savePos));
		CHECK(ComputeWriteChunkSize(main, stream, COMMsizeoffset, savePos));
	
		CHECK(omcWriteStream(stream, 4L, "SSND"));
	
		/* fill in SSND size later, unless Media Descriptor */
		CHECK(omcGetStreamPosition(stream, &pdata->dataSizeOffset));
		CHECK(omcWriteStream(stream, 4L, (void *) &zero));
	
		/* ssndoffset */
		CHECK(omcWriteStream(stream, 4L, (void *) &zero));
	
		/* blocksize */
		CHECK(omcWriteStream(stream, 4L, (void *) &zero));
	
		if (!isData)		/* No sample data, so patch FORM size here. */
		{
			CHECK(omcGetStreamPosition(stream, &savePos));
			CHECK(ComputeWriteChunkSize(main, stream, pdata->formSizeOffset, savePos));
		}
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * codecCreateAIFF	(INTERNAL)
 *
 * 		Create an AIFC file
 */
static omfErr_t codecCreateAIFF(
			omfCodecParms_t * parmblk,
		omfMediaHdl_t 			media,
		omfHdl_t				main)
{
	userDataAIFF_t 	*pdata;
	omfType_t			dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
	omfUID_t				uid;

	omfAssertMediaHdl(media);

	XPROTECT(main)
	{
		media->userData = (userDataAIFF_t *) omOptMalloc(main, sizeof(userDataAIFF_t));
		if (media->userData == NULL)
		{
			RAISE(OM_ERR_NOMEMORY);
		}
		pdata = (userDataAIFF_t *) media->userData;
		pdata->fileBitsPerSample = 0;
		pdata->interleaveBuf = NULL;
		pdata->fmtOps[0].opcode = kOmfAFmtEnd;
	
		if(main->fmt == kOmfiMedia)
		{
			if((parmblk->fileRev == kOmfRev1x) || (parmblk->fileRev == kOmfRevIMA))
			{
				CHECK(omfsReadUID(main, media->fileMob, OMMOBJMobID, &uid));
				CHECK(omfsWriteUID(main, media->dataObj, OMAIFCMobID, uid));
				CHECK(omfsReadExactEditRate(main, media->mdes, OMMDFLSampleRate, &pdata->fileRate));
			}
			else
			{
				CHECK(omfsReadRational(main, media->mdes, OMMDFLSampleRate, &pdata->fileRate));
			}
		}
		else
		  pdata->fileRate = media->channels[0].sampleRate;

		omfsCvtInt32toInt64(0, &pdata->formSizeOffset);
		omfsCvtInt32toInt64(0, &pdata->dataSizeOffset);
		omfsCvtInt32toInt64(0, &pdata->numSamplesOffset);
	
		if(main->fmt == kOmfiMedia)
		{
			CHECK(omcOpenStream(main, media->dataFile, media->stream, media->mdes, OMAIFDSummary, dataType));
			CHECK(setupStream(media->stream, media->soundKind, pdata));
		
			CHECK(writeAIFCHeader(main, media->stream, pdata, media->numChannels, 0, FALSE));
			CHECK(omcCloseStream(media->stream));
		}
		
		CHECK(omcOpenStream(main, media->dataFile, media->stream, media->dataObj,OMAIFCData, dataType));
		CHECK(setupStream(media->stream, media->soundKind, pdata));
		CHECK(writeAIFCHeader(main, media->stream, pdata, media->numChannels, 0, TRUE));
		CHECK(omcGetStreamPosition(media->stream, &media->dataStart));
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * codecImportRawAIFF	(INTERNAL)
 *
 * 	Open a raw AIFC file and create an MDES for it on the locator
 *		Don't add the locator, as that is the applications responsibility
 *		(We don't know what type of locator to add).
 */
static omfErr_t codecImportRawAIFF(omfCodecParms_t * parmblk,
		omfMediaHdl_t 			media,
		omfHdl_t				main)
{
	omfHdl_t				file, rawFile;
	omfCodecStream_t	readStream, writeStream;
	omfType_t			dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
	userDataAIFF_t		pdataBlock;
	omfInt16				numCh;
	omfUInt32			sampleFrames;
	omfObject_t			mdes;
	omfErr_t				status;
	omfDDefObj_t			dataKind;
	
#ifdef OMFI_ENABLE_STREAM_CACHE
	readStream.cachePtr = NULL;
	writeStream.cachePtr = NULL;
#endif
	readStream.procData = NULL;
	writeStream.procData = NULL;

	file = parmblk->spc.rawImportInfo.main;
	rawFile = parmblk->spc.rawImportInfo.rawFile;
	
	XPROTECT(file)
	{
		omfiDatakindLookup(parmblk->spc.rawImportInfo.main, SOUNDKIND, &dataKind, &status);
		CHECK(status);

		CHECK(omcOpenStream(file, rawFile, &readStream, NULL,
							OMNoProperty, OMNoType));
		CHECK(setupStream(&readStream, dataKind, &pdataBlock));
		CHECK(readAIFCHeader(parmblk->spc.getChannels.file, &readStream,
							&pdataBlock, NULL, &numCh, &sampleFrames));
		CHECK(omcCloseStream(&readStream));
		
		if(parmblk->fileRev == kOmfRev2x)
		{
			CHECK(omfsReadObjRef(file, parmblk->spc.rawImportInfo.fileMob,
										OMSMOBMediaDescription, &mdes));
			CHECK(omfsWriteRational(file, mdes, OMMDFLSampleRate, pdataBlock.fileRate));
		}
		else
		{
			CHECK(omfsReadObjRef(file, parmblk->spc.rawImportInfo.fileMob,
										OMMOBJPhysicalMedia, &mdes));
			CHECK(omfsWriteExactEditRate(file, mdes, OMMDFLSampleRate, pdataBlock.fileRate));
		}

		CHECK(omcOpenStream(file, file, &writeStream,  mdes, OMAIFDSummary, dataType));
		CHECK(setupStream(&writeStream, dataKind, &pdataBlock));
		CHECK(writeAIFCHeader(file, &writeStream, &pdataBlock, numCh, sampleFrames, FALSE));
		CHECK(omcCloseStream(&writeStream));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * codecSemCheckAIFF	(INTERNAL)
 *
 *			Semantic check the AIFF data at a higher level.
 */
static omfErr_t codecSemCheckAIFF(
			omfCodecParms_t	*parmblk,
		omfMediaHdl_t 			media,
		omfHdl_t				main)
{
	omfHdl_t			file;
	omfCodecStream_t	streamData;
	omfType_t			dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
	omfDDefObj_t		dataKind;
	omfInt16			mdesNumCh, dataNumCh;
	omfObject_t			mdes, dataObj;
	userDataAIFF_t		mdesPD, dataPD;
	omfUInt32			mdesSampleFrames, dataSampleFrames;
	omfErr_t			status;
	omfBool				getWarnings;
	
#ifdef OMFI_ENABLE_STREAM_CACHE
	streamData.cachePtr = NULL;
#endif
	streamData.procData = NULL;

	mdes = parmblk->spc.semCheck.mdes;
	dataObj = parmblk->spc.semCheck.dataObj;
	
	/* There is nothing to semantic check if just a media descriptor is poresent
	 */
	if(dataObj == NULL)
		return(OM_ERR_NONE);
		
	file = parmblk->spc.semCheck.file;
	parmblk->spc.semCheck.message = NULL;
	getWarnings = (parmblk->spc.semCheck.warn == kCheckPrintWarnings);
	XPROTECT(file)
	{
		omfiDatakindLookup(file, SOUNDKIND, &dataKind, &status);
		CHECK(status);
		/* First read the metadata out of the MDES Summary */
		CHECK(omcOpenStream(file, file, &streamData, mdes,
							OMAIFDSummary, dataType));
		CHECK(setupStream(&streamData, dataKind, &mdesPD));
		CHECK(readAIFCHeader(file, &streamData, &mdesPD,
							NULL, &mdesNumCh, &mdesSampleFrames));
		CHECK(omcCloseStream(&streamData));
		/* Next read the metadata out of the dataObject */
		CHECK(omcOpenStream(file, file, &streamData, dataObj,
							OMAIFCData, dataType));
		CHECK(setupStream(&streamData, dataKind, &dataPD));
		CHECK(readAIFCHeader(file, &streamData, &dataPD,
							NULL, &dataNumCh, &dataSampleFrames));
		CHECK(omcCloseStream(&streamData));
		/* Finally, make sure that the data agrees */
		if(mdesNumCh != dataNumCh)
		{
			parmblk->spc.semCheck.message = "Number of channels";
			RAISE(OM_ERR_DATA_MDES_DISAGREE);
		}
		if(getWarnings && (mdesSampleFrames != dataSampleFrames))
		{
			parmblk->spc.semCheck.message = "Sample Frames in summary don't agree with data";
			RAISE(OM_ERR_CODEC_SEMANTIC_WARN);		/* NOTE: This is ambiguous in the spec */
		}
		if(mdesPD.fileBitsPerSample != dataPD.fileBitsPerSample)
		{
			parmblk->spc.semCheck.message = "Bits Per Sample";
			RAISE(OM_ERR_DATA_MDES_DISAGREE);
		}
		if((mdesPD.fileRate.numerator != dataPD.fileRate.numerator) ||
		   (mdesPD.fileRate.denominator != dataPD.fileRate.denominator))
		{
			parmblk->spc.semCheck.message = "Frame Rate";
			RAISE(OM_ERR_DATA_MDES_DISAGREE);
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * setupStream	(INTERNAL)
 *
 * 	Sets up the memory & file formats of the codec stream based upon the
 *		variables in pdata.  This routine should be called whenever the file
 *		is opened to read or write data.
 */
static omfErr_t setupStream(
			omfCodecStream_t	*stream,
			omfDDefObj_t		datakind,
			userDataAIFF_t		*pdata)
{
	omfCStrmFmt_t   memFmt, fileFmt;
	omfUInt16			actualBitsPerSample;
	
	XPROTECT(stream->mainFile)
	{
		memFmt.mediaKind = datakind;
		memFmt.afmt = pdata->fmtOps;
		memFmt.vfmt = NULL;
		CHECK(omcSetMemoryFormat(stream, memFmt));
	
		/* Round sample size up to the next byte as per AIFC spec */
		actualBitsPerSample = (omfUInt16) pdata->fileBitsPerSample;
		if((actualBitsPerSample % 8) != 0)
			actualBitsPerSample +=  (8 - (actualBitsPerSample % 8));

		fileFmt.mediaKind = datakind;
		fileFmt.afmt = pdata->fileFmt;
		fileFmt.vfmt = NULL;
		pdata->fileFmt[0].opcode = kOmfSampleSize;
		pdata->fileFmt[0].operand.sampleSize = actualBitsPerSample;
		pdata->fileFmt[1].opcode = kOmfSampleRate;
		pdata->fileFmt[1].operand.sampleRate = pdata->fileRate;
		pdata->fileFmt[2].opcode = kOmfSampleFormat;
		pdata->fileFmt[2].operand.format = kOmfSignedMagnitude;
		pdata->fileFmt[3].opcode = kOmfAFmtEnd;
		CHECK(omcSetFileFormat(stream, fileFmt, (omfBool)(OMNativeByteOrder != MOTOROLA_ORDER)));
		
		// SPIDY - need to change this for anything BUT consolidate!
		//CHECK(omcSetFileFormat(stream, fileFmt, (omfBool)false) );

		CHECK(omfmSetSwabProc(stream, stdCodecSwabProc, stdCodecSwabLenProc,
								stdCodecNeedsSwabProc));		
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * SplitBuffers		(INTERNAL)
 *
 * 		Given a source buffer & an array of destination buffers, split the samples up in
 * 		round-robin fashion.  If a destination buffer in the array is NULL, then src is
 * 		incremented by one sample, but no data transfer occurs.  The first destination
 * 		buffer is allowed to be the same as the source buffer, but the other destination
 * 		buffers must be distinct to avoid corruption of the data.
 *
 * 		The number of entries in dest[] must be equal to numDest, and the dest array WILL
 * 		BE MODIFIED during execution, so it will have to be reloaded.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		<none>.
 *
 * Possible Errors:
 *		<none>.
 */
static void SplitBuffers(
			void					*original,
			omfInt32				srcSamples,
			omfInt16				sampleSize,
			omfInt16				numDest,
			interleaveBuf_t	*destPtr)
{
	omfInt16	*src16, *srcEnd16, *dest16;
	char	*src8, *srcEnd8, *dest8;
	omfInt16	n, x, sampleBytes;
	
	sampleBytes = (sampleSize + 7) / 8;
	if(sampleSize == 16)
	{
		src16 = (omfInt16 *)original;
		srcEnd16 = src16 + srcSamples;
		while(src16 < srcEnd16)
		{
			for(n = 0; (n < numDest) && (src16 < srcEnd16) ; n++)
			{
				dest16 = (omfInt16 *)destPtr[n].buf;
				if(dest16 != NULL)
				{
					destPtr[n].bytesXfered += sampleBytes;
					destPtr[n].samplesLeft--;
					*dest16++ = *src16++;
				}
				else
					src16++;
				destPtr[n].buf = (char *)dest16;
			}
		}
	}
	else
	{
		src8 = (char *)original;
		srcEnd8 = src8 + srcSamples;
		while(src8 < srcEnd8)
		{
			for(n = 0; (n < numDest) && (src8 < srcEnd8) ; n++, dest8++)
			{
				dest8 = (char *)destPtr[n].buf;
				if(dest8 != NULL)
				{
					destPtr[n].bytesXfered += sampleBytes;
					{	for(x = 0; x < sampleBytes; x++)
							*dest8++ = *src8++;
					}
				}
				else
					src8++;
				destPtr[n].buf = (char *)dest8;
			}
		}
	}
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
