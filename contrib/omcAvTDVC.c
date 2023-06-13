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
#include <stdio.h>

#include "omPublic.h"
#include "omMedia.h"
#include "omcStd.h"
#include "omcAvTDVC.h" 
#include "omPvt.h"
#include "omUtils.h"

#define MAX_FMT_OPS	32

#define DVC_COMPRESSION_METHOD "DV/C"
typedef struct
{
	omfProperty_t	omCDCIComponentWidth;
	omfProperty_t	omCDCIHorizontalSubsampling;
	omfProperty_t	omCDCIColorSiting;
	omfProperty_t	omCDCIBlackReferenceLevel;
	omfProperty_t	omCDCIWhiteReferenceLevel;
   omfProperty_t  omDIDDFrameSampleSize;
	omfProperty_t	omAvDIDDResolutionID;
	omfProperty_t	omAvTDVCImageData;		/* !!! */
}				omcTDVCPersistent_t;

typedef struct
{
	omfInt32           imageWidth;
	omfInt32           imageLength;
	omfInt32			sampledWidth;
	omfInt32			sampledLength;
	omfInt32			sampledXOffset;
	omfInt32			sampledYOffset;
	omfInt32			displayWidth;
	omfInt32			displayLength;
	omfInt32			displayXOffset;
	omfInt32			displayYOffset;
	omfRational_t   	rate;
	omfInt32			componentWidth;
	omfUInt32           fileBytesPerSample;
	omfFrameLayout_t fileLayout;	/* Frame of fields per sample */
	omfRational_t	imageAspectRatio;
	omfVideoMemOp_t		fmtOps[MAX_FMT_OPS+1];
	omfVideoMemOp_t		fileFmt[10];
	omfBool				descriptorFlushed;
	omfInt32				currentIndex;
	omfFieldDom_t	fieldDominance;
	omfInt32				videoLineMap[2];
	omfUInt32          maxIndex;
	omfUInt32		horizontalSubsampling;
	omfColorSiting_t	colorSiting;
	omfUInt32			whiteLevel;
	omfUInt32			blackLevel;
	omfUInt32			colorRange;
	omfInt32				bitsPerPixelAvg;
	omfInt32				memBytesPerSample;
    omfInt32          fixedFrameSize;
    omfUInt32          numSamples32;
	omfUInt32         *frameIndex;
}               userDataTDVC_t;

static omfErr_t codecGetCompressionParmsTDVC(omfCodecParms_t * info, omfHdl_t main, userDataTDVC_t * pdata);
static omfErr_t codecPutCompressionParmsTDVC(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataTDVC_t * pdata);

static omfErr_t InitTDVCCodec(omfCodecParms_t *info);
static omfErr_t codecOpenTDVC(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecCreateTDVC(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecGetTDVCFieldInfo(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataTDVC_t * pdata);
static omfErr_t codecPutTDVCFieldInfo(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataTDVC_t * pdata);
static omfErr_t codecGetRect(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataTDVC_t * pdata);
static omfErr_t codecWriteSamplesTDVC(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecReadSamplesTDVC(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecSetRect(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataTDVC_t * pdata);
static omfErr_t setupStream(omfMediaHdl_t media, userDataTDVC_t * pdata);
static omfErr_t codecCloseTDVC(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t omfmTDVCSetFrameNumber(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t omfmTDVCGetFrameOffset(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t initUserData(userDataTDVC_t *pdata);
static omfErr_t postWriteOps(omfCodecParms_t * info, omfMediaHdl_t media);
static omfErr_t writeDescriptorData(omfCodecParms_t * info, 
									omfMediaHdl_t media, 
									omfHdl_t main, 
									userDataTDVC_t * pdata);
static omfErr_t loadDescriptorData(omfCodecParms_t * info, 
								   omfMediaHdl_t media, 
								   omfHdl_t main, 
								   userDataTDVC_t * pdata);

static omfErr_t omfmTDVCGetMaxSampleSize(omfCodecParms_t * info,
					                 omfMediaHdl_t media,
					                 omfHdl_t main,
				                     userDataTDVC_t * pdata,
					                 omfInt32 * sampleSize);

static omfErr_t InitMDESProps(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				file);
static omfErr_t myCodecSetMemoryInfo(omfCodecParms_t * parmblk,
				                      omfMediaHdl_t media,
				                      omfHdl_t file,
				                      userDataTDVC_t * pdata);
static omfErr_t codecGetInfoTDVC(omfCodecParms_t * parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main);
static omfErr_t codecPutInfoTDVC(omfCodecParms_t * parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main);
static omfErr_t codecGetNumChannelsTDVC(
			omfCodecParms_t	*parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main);
static omfErr_t codecGetSelectInfoTDVC(
			omfCodecParms_t	*parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main);


static omfInt32 powi(omfInt32 base, omfInt32 exp)
{
	omfInt32 temp = 1;
	int  i;
	
	if (exp < 0)
		return -1;
		
	for (i = 0; i < exp; i++)
		temp *= base;
		
	return temp;
}

omfErr_t        omfCodecAvTDVC(omfCodecFunctions_t func, omfCodecParms_t * info)
{
	omfErr_t        	status;
	omfCodecOptDispPtr_t	*dispPtr;
	
	switch (func)
	{
	case kCodecInit:
		InitTDVCCodec(info);
		return (OM_ERR_NONE);
		break;

	case kCodecGetMetaInfo:
		strncpy((char *)info->spc.metaInfo.name, "Avid TDVC Codec (Compressed ONLY)", info->spc.metaInfo.nameLen);
		info->spc.metaInfo.info.mdesClassID = "CDCI";
		info->spc.metaInfo.info.dataClassID = "TDVC";
		info->spc.metaInfo.info.codecID = CODEC_AVID_TARGA_DVC_VIDEO;
		info->spc.metaInfo.info.rev = CODEC_REV_3;
		info->spc.metaInfo.info.dataKindNameList = PICTUREKIND;
		info->spc.metaInfo.info.minFileRev = kOmfRev1x;
		info->spc.metaInfo.info.maxFileRevIsValid = FALSE;		/* There is no maximum rev */
		return (OM_ERR_NONE);
	case kCodecLoadFuncPointers:
		dispPtr = info->spc.loadFuncTbl.dispatchTbl;

		dispPtr[ kCodecOpen ] = codecOpenTDVC;
		dispPtr[ kCodecCreate ] = codecCreateTDVC;
		dispPtr[ kCodecGetInfo ] = codecGetInfoTDVC;
		dispPtr[ kCodecPutInfo ] = codecPutInfoTDVC;
		dispPtr[ kCodecReadSamples ] = codecReadSamplesTDVC;
		dispPtr[ kCodecWriteSamples ] = codecWriteSamplesTDVC;
		dispPtr[ kCodecClose ] = codecCloseTDVC;
		dispPtr[ kCodecSetFrame ] = omfmTDVCSetFrameNumber;
		dispPtr[ kCodecGetFrameOffset ] = omfmTDVCGetFrameOffset;
		dispPtr[ kCodecGetNumChannels ] = codecGetNumChannelsTDVC;
		dispPtr[ kCodecGetSelectInfo ] = codecGetSelectInfoTDVC;
		dispPtr[ kCodecInitMDESProps ] = InitMDESProps;

 		return(OM_ERR_NONE);
			
	default:
	case kCodecReadLines:		/* This can't work through the soft codec */
	case kCodecWriteLines:		/* As JPEG works in 8x8 blocks */
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}

	return (status);
}
	
/************************
 * codecGetNumChannelsTDVC	(INTERNAL)
 *
 *			Find the number of channels in the current media.
 */
static omfErr_t codecGetNumChannelsTDVC(
			omfCodecParms_t	*info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfDDefObj_t		dataKind;
	omfErr_t			status = OM_ERR_NONE;

	omfiDatakindLookup(info->spc.getChannels.file, PICTUREKIND, &dataKind, &status);
	if (info->spc.getChannels.mediaKind == dataKind)
		info->spc.getChannels.numCh = 1;
	else
		info->spc.getChannels.numCh = 0;
	
	return(status);
}


/************************
 * codecGetSelectInfoTDVC	(INTERNAL)
 *
 *			Get the information required to select a codec.
 */
static omfErr_t codecGetSelectInfoTDVC(
			omfCodecParms_t	*info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfErr_t			status = OM_ERR_NONE;
	char				compress[256];

		if(omfsIsPropPresent(info->spc.selectInfo.file, info->spc.selectInfo.mdes, OMDIDDCompression, OMString))
		{
			status = omfsReadString(info->spc.selectInfo.file, info->spc.selectInfo.mdes, OMDIDDCompression, 
											compress, sizeof(compress));
			if(status != OM_ERR_NONE)
				return(status);
			info->spc.selectInfo.info.willHandleMDES = (strcmp(compress, DVC_COMPRESSION_METHOD) == 0);
		}
		else
			info->spc.selectInfo.info.willHandleMDES = FALSE;
			
	info->spc.selectInfo.info.isNative = TRUE;
	info->spc.selectInfo.info.hwAssisted = FALSE;
	info->spc.selectInfo.info.relativeLoss = 0;	/* !!! Need to read MDES header here */
	info->spc.selectInfo.info.avgBitsPerSec = 4;	/* !!! Need to read MDES header here */
	
	return(status);
}

/************************
 * codecGetInfoTDVC	(INTERNAL)
 *
 */
static omfErr_t codecGetInfoTDVC(omfCodecParms_t *info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfErr_t			status = OM_ERR_NONE;
	omfMaxSampleSize_t	*ptr;
	omfFrameSizeParms_t	*singlePtr;
	userDataTDVC_t 		*pdata = (userDataTDVC_t *) media->userData;
//	omfUInt32			frameSize, frameOff;

	switch (info->spc.mediaInfo.infoType)
	{
	case kMediaIsContiguous:
		{
		status = OMIsPropertyContiguous(media->mainFile, media->dataObj, OMIDATImageData,
										OMDataValue, (omfBool *)info->spc.mediaInfo.buf);
		}
		break;
	case kVideoInfo:
		status = codecGetTDVCFieldInfo(info, media, main, pdata);
		break;
	case kCompressionParms:
		status = codecGetCompressionParmsTDVC(info, main, pdata);
		break;
	case kJPEGTables:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	case kMaxSampleSize:
		ptr = (omfMaxSampleSize_t *) info->spc.mediaInfo.buf;
		if(ptr->mediaKind == media->pictureKind)
		{
			ptr->largestSampleSize = pdata->fixedFrameSize;
		}
		else
			status = OM_ERR_CODEC_CHANNELS;
		break;
	
	case kSampleSize:
		singlePtr = (omfFrameSizeParms_t *)info->spc.mediaInfo.buf;
		if(singlePtr->mediaKind == media->pictureKind)
		{
			omfsCvtUInt32toInt64(pdata->fixedFrameSize, &singlePtr->frameSize);
			status = OM_ERR_NONE;
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
 * codecPutInfoTDVC	(INTERNAL)
 *
 */
static omfErr_t codecPutInfoTDVC(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfErr_t			status = OM_ERR_NONE;
	userDataTDVC_t 		*pdata = (userDataTDVC_t *) media->userData;

	switch (info->spc.mediaInfo.infoType)
	{
	case kVideoInfo:
		status = codecPutTDVCFieldInfo(info, media, main, pdata);
		break;
	case kCompressionParms:
		status = codecPutCompressionParmsTDVC(info, media, main, pdata);
		break;
	case kVideoMemFormat:
		status = myCodecSetMemoryInfo(info, media, main, pdata);
		break;
	case kJPEGTables:
		status = OM_ERR_INVALID_OP_CODEC;
		break;

	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}
	
	return(status);
}

static omfErr_t codecCreateTDVC(omfCodecParms_t * info,
								omfMediaHdl_t media,
				                omfHdl_t main)
{
	userDataTDVC_t *pdata;
	omcTDVCPersistent_t *pers = (omcTDVCPersistent_t *)info->pers;

	omfAssertMediaHdl(media);
	omfAssert(info, main, OM_ERR_NULL_PARAM);
	
	XPROTECT(main)
	{
media->pvt->pixType	= kOmfPixRGBA;		 // JeffB: Was YUV
		/* copy descriptor information into local structure */
		media->userData = (userDataTDVC_t *)omOptMalloc(main, sizeof(userDataTDVC_t));
		XASSERT(media->userData != NULL, OM_ERR_NOMEMORY);
	
		pdata = (userDataTDVC_t *) media->userData;
		
		CHECK(initUserData(pdata));
		
		/* there are no multiword entities (YET!) */
		media->stream->swapBytes = FALSE; 
	
		/* get sample rate from MDFL parent class of descriptor.  
		 * This assumes that the TDVC descriptor class is a descendant of 
		 * MDFL. 
		 */
		if(main->fmt == kOmfiMedia) // SPIDY
		{
			if(info->fileRev == kOmfRev1x || info->fileRev == kOmfRevIMA)
			{
				if (omfsReadExactEditRate(main, media->mdes, OMMDFLSampleRate,
				  		   &(pdata->rate)) != OM_ERR_NONE)
					RAISE(OM_ERR_DESCSAMPRATE);
			}
			else
			{
				if (omfsReadRational(main, media->mdes, OMMDFLSampleRate,
							 &(pdata->rate)) != OM_ERR_NONE)
					RAISE(OM_ERR_DESCSAMPRATE);
			}
		}
		else
			pdata->rate = media->channels[0].sampleRate;		// SPIDY
		
		/* number of frames is unknown until close time */
		omfsCvtInt32toInt64(0, &media->channels[0].numSamples);
	
		pdata->fmtOps[0].opcode = kOmfVFmtEnd;
		
		MakeRational(4, 3, &(pdata->imageAspectRatio));
		CHECK(omcOpenStream(media->mainFile, media->dataFile, media->stream, 
						media->dataObj, pers->omAvTDVCImageData, OMDataValue));	/* !!! */
		CHECK(setupStream(media, pdata));
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
static omfErr_t codecOpenTDVC(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
			                      omfHdl_t main)
{
	userDataTDVC_t 		*pdata = (userDataTDVC_t *) media->userData;

	omfAssertMediaHdl(media);

	/* pull out descriptor information BEFORE reading TDVC data */
	XPROTECT(main)
	{
media->pvt->pixType	= kOmfPixRGBA;		 // JeffB: Was YUV

		media->userData = (userDataTDVC_t *)omOptMalloc(main, sizeof(userDataTDVC_t));
		XASSERT(media->userData != NULL, OM_ERR_NOMEMORY);
		pdata = media->userData;
				
		CHECK(initUserData(pdata));
			
		pdata->fmtOps[0].opcode = kOmfVFmtEnd;
		
		CHECK(loadDescriptorData(info, media, main, pdata));
								
		CHECK(setupStream(media, pdata));
	
		/* prepare for data read */
		omfsCvtInt32toInt64(0, &media->channels[0].dataOffset);	
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
static omfErr_t codecWriteSamplesTDVC(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
				                      omfHdl_t main)
{
	omfmMultiXfer_t 	*xfer = NULL;
	omfUInt32			offset;
//	omfInt32 			fieldLen;
//	omfUInt32			n;
	omcTDVCPersistent_t *pers = (omcTDVCPersistent_t *)info->pers;
	userDataTDVC_t 		*pdata = (userDataTDVC_t *) media->userData;
	
	omfAssertMediaHdl(media);
	
	XPROTECT(main)
	{
		XASSERT(pdata, OM_ERR_NULL_PARAM);
	
		XASSERT(pdata->componentWidth != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->fileBytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		xfer = info->spc.mediaXfer.xfer;
		XASSERT(xfer, OM_ERR_NULLBUF);
		
		/* this codec only allows one-channel media */
		XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
	
		if (media->compEnable == kToolkitCompressionEnable)
		{
		  RAISE(OM_ERR_JPEGDISABLED);
		} else			/* already compressed by user */
		{	
			XASSERT(xfer->numSamples == 1, OM_ERR_ONESAMPLEWRITE);
			CHECK(omcWriteStream(media->stream, xfer->buflen, xfer->buffer));
			xfer->bytesXfered = xfer->buflen;
			xfer->samplesXfered = xfer->numSamples;

			/* Don't forget to update the global frame counter */
			CHECK(omfsAddInt32toInt64(1, &media->channels[0].numSamples));
			CHECK(omcGetStreamPos32(media->stream, &offset));
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
static omfErr_t codecReadSamplesTDVC(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
				                     omfHdl_t main)
{
	userDataTDVC_t 	*pdata = NULL;
	omfUInt32			fileBytes = 0;
	omfUInt32			bytesRead = 0;
	omfmMultiXfer_t	*xfer = NULL;
	omfUInt32			rBytes, startPos, nBytes;
	omcTDVCPersistent_t *pers = (omcTDVCPersistent_t *)info->pers;
//	omfCStrmSwab_t		src, dest;
	char					*tempBuf = NULL;
//	omfUInt32			n;
#ifdef JPEG_TRACE
	char              errmsg[256];
	omfUInt32			offset;
#endif
	omfErr_t				status;
//	omfInt32 frameIndexLen;
	
	omfAssertMediaHdl(media);
	
	XPROTECT(main)
	{
		pdata = (userDataTDVC_t *) media->userData;
		XASSERT(pdata, OM_ERR_NULL_PARAM);
	
		XASSERT(pdata->componentWidth != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->fileBytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		xfer = info->spc.mediaXfer.xfer;
		XASSERT(xfer, OM_ERR_NULLBUF);
	
		xfer->bytesXfered = 0;
		xfer->samplesXfered = 0;
		
		XASSERT(media->numChannels == 1, OM_ERR_CODEC_CHANNELS);
		//XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
						
		if (media->compEnable == kToolkitCompressionEnable)
		{
		    RAISE(OM_ERR_JPEGDISABLED);
		}
		else		/* give back a compressed frame */
		{	

			if (pdata->frameIndex == NULL)
				RAISE(OM_ERR_NOFRAMEINDEX);
			if ((omfUInt32)(pdata->currentIndex + 1) >= pdata->maxIndex)
				RAISE(OM_ERR_BADFRAMEOFFSET);
	
			nBytes = pdata->frameIndex[pdata->currentIndex + 1] - pdata->frameIndex[pdata->currentIndex];
			startPos = pdata->frameIndex[pdata->currentIndex];
			if(nBytes > xfer->buflen)
				RAISE(OM_ERR_SMALLBUF);
			
			/* skip the IFH */
			CHECK(omcSeekStream32(media->stream, startPos));	
			pdata->currentIndex++;

			status = omcReadStream(media->stream,
				nBytes, xfer->buffer, &rBytes);
			xfer->bytesXfered = rBytes;
			xfer->samplesXfered = 1;
			CHECK(status);
			CHECK(status);
			/* Check for this last so that we at least read one frames worth */
			XASSERT(xfer->numSamples == 1, OM_ERR_ONESAMPLEREAD);
		}
	}
	XEXCEPT
	{
		if(tempBuf != NULL)
			omOptFree(main, tempBuf);
	}
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
static omfErr_t writeDescriptorData(omfCodecParms_t * info, 
									omfMediaHdl_t media, 
									omfHdl_t main, 
									userDataTDVC_t * pdata)
{
	omcTDVCPersistent_t *pers;
	
	/* this routine will be called after sample data is written */

	omfAssertMediaHdl(media);
	main = media->mainFile;
	
	omfAssert(info, main, OM_ERR_NULL_PARAM);
 	pers = (omcTDVCPersistent_t *)info->pers;
 
	pdata = (userDataTDVC_t *) media->userData;
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
		/* OMFI:DIDD:StoredHeight and OMFI:DIDD:StoredWidth */	
		CHECK(omfsWriteInt32(main, media->mdes, OMDIDDStoredHeight, 
							pdata->imageLength));
		CHECK(omfsWriteInt32(main, media->mdes, OMDIDDStoredWidth, 
							pdata->imageWidth));
		
		/* display and sampled coordinates  */
		if (pdata->displayLength && pdata->displayWidth)
		{
			CHECK(omfsWriteInt32(main, media->mdes, OMDIDDDisplayYOffset, 
								pdata->displayYOffset));
			CHECK(omfsWriteInt32(main, media->mdes, OMDIDDDisplayXOffset, 
								pdata->displayXOffset));
			CHECK(omfsWriteInt32(main, media->mdes, OMDIDDDisplayHeight, 
								pdata->displayLength));
			CHECK(omfsWriteInt32(main, media->mdes, OMDIDDDisplayWidth, 
								pdata->displayWidth));
		}
		if (pdata->sampledLength && pdata->sampledWidth)
		{
			CHECK(omfsWriteInt32(main, media->mdes, OMDIDDSampledYOffset, 
								pdata->sampledYOffset));
			CHECK(omfsWriteInt32(main, media->mdes, OMDIDDSampledXOffset, 
								pdata->sampledXOffset));
			CHECK(omfsWriteInt32(main, media->mdes, OMDIDDSampledHeight, 
								pdata->sampledLength));
			CHECK(omfsWriteInt32(main, media->mdes, OMDIDDSampledWidth, 
								pdata->sampledWidth));
		}
		/* OMFI:DIDD:FrameLayout */
		CHECK(omfsWriteLayoutType(main, media->mdes, OMDIDDFrameLayout, pdata->fileLayout));
			
		/* OMFI:DIDD:AlphaTransparency DEFER IMPLEMENTATION TO A LATER TIME!!*/
		
		/* OMFI:DIDD:VideoLineMap */
		if (pdata->fileLayout != kFullFrame &&
			(pdata->videoLineMap[0] != 0 || pdata->videoLineMap[1] != 0))
		{
		
			CHECK(OMWriteBaseProp(main, media->mdes, OMDIDDVideoLineMap,
							   OMInt32Array, sizeof(pdata->videoLineMap),
							   (void *)pdata->videoLineMap));
		}
		
		/* OMFI:DIDD:ImageAspectRatio */
		
		CHECK(omfsWriteRational(main, media->mdes, OMDIDDImageAspectRatio,
				 						 pdata->imageAspectRatio));
		
		CHECK(omfsWriteInt32(main, media->mdes, pers->omCDCIComponentWidth, pdata->componentWidth));
		CHECK(omfsWriteUInt32(main, media->mdes, pers->omCDCIHorizontalSubsampling, pdata->horizontalSubsampling));

		CHECK(omfsWriteInt32(main, media->mdes, pers->omDIDDFrameSampleSize, pdata->fixedFrameSize));
		pdata->descriptorFlushed = TRUE;
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
static omfErr_t loadDescriptorData(omfCodecParms_t * info, 
								   omfMediaHdl_t media, 
								   omfHdl_t main, 
								   userDataTDVC_t * pdata)
{
	omfInt32 				sampleFrames = 0;
	omfUInt32 				layout = 0;
	omfUInt32 				bytesPerPixel = 0;
	omfUInt32 				numFields = 0;
	omfUInt32				indexSize, n, indexVal;
	omcTDVCPersistent_t *pers = NULL;
	omfPosition_t			zeroPos;
	omfErr_t					status;
//	omfInt32				bytesProcessed, len;
//	omfInt16				tableType, tableindex, elem16, tableBytes, i;
#if OMFI_ENABLE_SEMCHECK
	omfBool					saveCheck;
#endif
//	unsigned char			*ptr, *src;
	
	omfsCvtInt32toPosition(0, zeroPos);
	omfAssertMediaHdl(media);
	omfAssert(info, main, OM_ERR_NULL_PARAM);
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
		pers = (omcTDVCPersistent_t *)info->pers;
		pdata->frameIndex = NULL;
	 
		if (info->fileRev == kOmfRev2x)
		{
	 		CHECK(omfsReadRational(main, media->mdes, OMMDFLSampleRate, 
								   &(pdata->rate)));
			CHECK(omfsReadLength(main, media->mdes, OMMDFLLength, 
								 &media->channels[0].numSamples));
	 	}
	 	else
	 	{
	 		CHECK(omfsReadExactEditRate(main, media->mdes, OMMDFLSampleRate, 
										&(pdata->rate)));
			CHECK(omfsReadInt32(main, media->mdes,OMMDFLLength, &sampleFrames));
			omfsCvtInt32toInt64(sampleFrames, &media->channels[0].numSamples);
		} 		
		
		CHECK(omfsTruncInt64toUInt32(media->channels[0].numSamples, &pdata->numSamples32));
	
		media->numChannels = 1;
	
		CHECK(omcOpenStream(media->mainFile, media->dataFile, media->stream, 
						media->dataObj, pers->omAvTDVCImageData, OMDataValue));	/* !!! */
			
		CHECK(omfsReadInt32(main, media->mdes, OMDIDDStoredWidth, 
						   &pdata->imageWidth));
		CHECK(omfsReadInt32(main, media->mdes, OMDIDDStoredHeight, 
						   &pdata->imageLength));
		
		/* sampled dimensions default to stored dimensions.  If any calls fail
		 * later, default will refile untouched. 
		 */
	
		pdata->sampledWidth = pdata->imageWidth;
		pdata->sampledLength = pdata->imageLength;
		pdata->sampledXOffset = 0;
		pdata->sampledYOffset = 0;
		
		if (omfsReadInt32(main, media->mdes, OMDIDDSampledWidth, 
						 &pdata->sampledWidth) == OM_ERR_NONE &&
			omfsReadInt32(main, media->mdes, OMDIDDSampledHeight, 
						 &pdata->sampledLength) == OM_ERR_NONE)
		{
			/* ignore errors here, default to zero in both cases */		
			(void)omfsReadInt32(main, media->mdes, OMDIDDSampledXOffset, 
							   &pdata->sampledXOffset);
			(void)omfsReadInt32(main, media->mdes, OMDIDDSampledYOffset, 
							   &pdata->sampledYOffset);
		}
		
		pdata->displayWidth = pdata->imageWidth;
		pdata->displayLength = pdata->imageLength;
		pdata->displayXOffset = 0;
		pdata->displayYOffset = 0;
		
		if (omfsReadInt32(main, media->mdes, OMDIDDDisplayWidth, 
						 &pdata->displayWidth) == OM_ERR_NONE &&
			omfsReadInt32(main, media->mdes, OMDIDDDisplayHeight, 
						 &pdata->displayLength) == OM_ERR_NONE)
		{
			/* ignore errors here, default to zero in both cases */		
			(void)omfsReadInt32(main, media->mdes, OMDIDDDisplayXOffset, 
							   &pdata->displayXOffset);
			(void)omfsReadInt32(main, media->mdes, OMDIDDDisplayYOffset, 
							   &pdata->displayYOffset);
		}
			
		CHECK(omfsReadRational(main, media->mdes, OMDIDDImageAspectRatio, 
							   &pdata->imageAspectRatio))
		CHECK(omfsReadLayoutType(main, media->mdes, OMDIDDFrameLayout, 
							 &pdata->fileLayout));
	
		switch (pdata->fileLayout)
		{
	
			case kFullFrame:
			case kOneField:
				numFields = 1;
				break;
		
			case kSeparateFields:
			case kMixedFields:
				numFields = 2;
				break;
		
			default:
				break;
		
		} /* end switch */
												
		/* OMFI:DIDD:VideoLineMap */
		if (pdata->fileLayout != kFullFrame)
		{
			omfUInt32 mapFlags = 0;
			omfPosition_t			fourPos;
	
			omfsCvtInt32toPosition(sizeof(omfInt32), fourPos);
			
			status = OMReadProp(main, media->mdes, OMDIDDVideoLineMap,
								  zeroPos, kSwabIfNeeded, OMInt32Array,
								  sizeof(omfInt32), (void *)&pdata->videoLineMap[0]);
			if (status != OM_ERR_NONE)
				pdata->videoLineMap[0] = 0;
			else
				mapFlags |= 1;
				
			if (pdata->fileLayout == kSeparateFields || pdata->fileLayout == kMixedFields)
			{
				omfsCvtInt32toPosition(sizeof(omfInt32), fourPos);
			
				status = OMReadProp(main, media->mdes, OMDIDDVideoLineMap,
									  fourPos, kSwabIfNeeded, OMInt32Array,
									  sizeof(omfInt32), (void *)&pdata->videoLineMap[1]);
				if (status != OM_ERR_NONE)
					pdata->videoLineMap[1] = 0;
				else
					mapFlags |= 2;
			}
		}

		pdata->fileBytesPerSample = 0;
		omfsReadInt32(main, media->mdes, pers->omCDCIComponentWidth, 
						 &pdata->componentWidth);
				
		CHECK(omfsReadUInt32(main, media->mdes, pers->omCDCIHorizontalSubsampling,
							&pdata->horizontalSubsampling));

		if (! pdata->fileBytesPerSample)
		{
			omfUInt32    		bitsPerSample;
			
			/* 4:2:2 = two-pixels get 2 samples of luma, 1 of each chroma channel, avg == 2 */							
	  		if (pdata->horizontalSubsampling == 2)						
			{
				pdata->bitsPerPixelAvg = (pdata->componentWidth * 2);
				bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
	  									pdata->bitsPerPixelAvg * numFields;
	  		}							
	  		/* 4:1:1 = four-pixels get four samples of luma, 1 of each chroma channel, avg == 1.5 */							
	  		else if (pdata->horizontalSubsampling == 4)						
			{
				pdata->bitsPerPixelAvg = ((pdata->componentWidth * 6) / 4);
				bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
	  									pdata->bitsPerPixelAvg * numFields;
	  		}							
	  		else
	  			RAISE(OM_ERR_ILLEGAL_FILEFMT);
	  									
			pdata->fileBytesPerSample = (bitsPerSample + 7) / 8;
			pdata->memBytesPerSample = pdata->fileBytesPerSample;
		}

		CHECK(omfsReadInt32(main, media->mdes, pers->omDIDDFrameSampleSize, 
						 &pdata->fixedFrameSize));
				
#if OMFI_ENABLE_SEMCHECK
		saveCheck = omfsIsSemanticCheckOn(media->dataFile);
		omfsSemanticCheckOff(media->dataFile);
#endif
		indexSize = pdata->numSamples32 + 1;
		pdata->frameIndex = (omfUInt32 *) omOptMalloc(main, (size_t)indexSize * sizeof(omfInt32));
		if (pdata->frameIndex == NULL)
			RAISE(OM_ERR_NOMEMORY);

		pdata->maxIndex = indexSize;
		for(n = 0, indexVal = 0; n < indexSize; n++,indexVal+=pdata->fixedFrameSize)
		{
			pdata->frameIndex[n] = indexVal;
		}
#if OMFI_ENABLE_SEMCHECK
		if(saveCheck)
			omfsSemanticCheckOn(media->dataFile);
#endif
	}
	XEXCEPT
	{
#if OMFI_ENABLE_SEMCHECK
		if(saveCheck)
			omfsSemanticCheckOn(media->dataFile);
#endif
	}
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
static omfErr_t omfmTDVCSetFrameNumber(omfCodecParms_t * info,
				                       omfMediaHdl_t media,
				                       omfHdl_t main)
{
	omfUInt32          frameNumber;
	userDataTDVC_t 		*pdata = (userDataTDVC_t *) media->userData;

	omfAssert(media->dataObj, main, OM_ERR_INTERNAL_MDO);
	XPROTECT(main)
	{
		CHECK(omfsTruncInt64toUInt32(info->spc.setFrame.frameNumber, &frameNumber)); /* OK FRAMEOFFSET */
		XASSERT(frameNumber != 0, OM_ERR_BADFRAMEOFFSET);

		XASSERT(pdata->frameIndex != NULL, OM_ERR_NOFRAMEINDEX);
		if ((omfUInt32)pdata->currentIndex >= pdata->maxIndex)
			RAISE(OM_ERR_BADFRAMEOFFSET);

		pdata->currentIndex = frameNumber - 1;
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * omfmTDVCGetFrameOffset
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
static omfErr_t omfmTDVCGetFrameOffset(omfCodecParms_t * info,
				                       omfMediaHdl_t media,
				                       omfHdl_t main)
{
	omfUInt32          frameNumber;
	userDataTDVC_t 		*pdata = (userDataTDVC_t *) media->userData;

	omfAssert(media->dataObj, main, OM_ERR_INTERNAL_MDO);
	XPROTECT(main)
	{
		CHECK(omfsTruncInt64toUInt32(info->spc.getFrameOffset.frameNumber, &frameNumber)); /* OK FRAMEOFFSET */
		XASSERT(frameNumber != 0, OM_ERR_BADFRAMEOFFSET);

		XASSERT(pdata->frameIndex != NULL, OM_ERR_NOFRAMEINDEX);
		if (frameNumber >= pdata->maxIndex)
			RAISE(OM_ERR_BADFRAMEOFFSET);

		omfsCvtInt32toInt64(pdata->frameIndex[frameNumber - 1], &info->spc.getFrameOffset.frameOffset);
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
static omfErr_t codecGetTDVCFieldInfo(omfCodecParms_t * info,
				                      omfMediaHdl_t media,
				                      omfHdl_t main,
				                      userDataTDVC_t * pdata)
{
	omfVideoMemOp_t 	*vparms;
	
//	XPROTECT(main)
	{
		vparms = ((omfVideoMemOp_t *)info->spc.mediaInfo.buf);
		for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
		  {
			 switch(vparms->opcode)
				{
				case kOmfPixelFormat:
					vparms->operand.expPixelFormat = kOmfPixRGBA;		 // JeffB: Was YUV
				  break;
				case kOmfFrameLayout:
				  vparms->operand.expFrameLayout = pdata->fileLayout;
				  break;
		
				case kOmfFieldDominance:
				  vparms->operand.expFieldDom = pdata->fieldDominance;
				  break;
				  
				case kOmfVideoLineMap:
				 	vparms->operand.expLineMap[0] = pdata->videoLineMap[0];
				 	vparms->operand.expLineMap[1] = pdata->videoLineMap[1];
				  	break;
				  	
				case kOmfCDCICompWidth:
					vparms->operand.expInt32 = pdata->componentWidth;
					break;
				case kOmfSampledRect:
					vparms->operand.expRect.xOffset = pdata->sampledXOffset;
					vparms->operand.expRect.yOffset = pdata->sampledYOffset;
					vparms->operand.expRect.xSize = pdata->sampledWidth;
					vparms->operand.expRect.ySize = pdata->sampledLength;
					break;
					
				case kOmfDisplayRect:
					vparms->operand.expRect.xOffset = pdata->displayXOffset;
					vparms->operand.expRect.yOffset = pdata->displayYOffset;
					vparms->operand.expRect.xSize = pdata->displayWidth;
					vparms->operand.expRect.ySize = pdata->displayLength;
					break;

				case kOmfStoredRect:
				  vparms->operand.expRect.xSize = pdata->imageWidth;
				  vparms->operand.expRect.ySize = pdata->imageLength;
				  vparms->operand.expRect.xOffset = 0;
				  vparms->operand.expRect.yOffset = 0;
				  break;
				case kOmfPixelSize:
				  vparms->operand.expInt16 = pdata->componentWidth * 3;
				  break;
				case kOmfAspectRatio:
				  vparms->operand.expRational = pdata->imageAspectRatio;
				  break;
				case kOmfWillTransferLines:
					vparms->operand.expBoolean = FALSE;
					break;
				case kOmfIsCompressed:
					vparms->operand.expBoolean = TRUE;
					break;
				case kOmfCDCIHorizSubsampling:
					vparms->operand.expUInt32 = pdata->horizontalSubsampling;
					break;
				case kOmfRGBCompLayout:
//					CHECK(GetFilePixelFormat(main, pdata, &format));
//					if (format == kOmfPixRGBA)
					{	
						vparms->operand.expCompArray[0] = 'R';
						vparms->operand.expCompArray[1] = 'G';
						vparms->operand.expCompArray[2] = 'B';
						vparms->operand.expCompArray[3] = 0; /* terminating zero */
					}
//					else
//						RAISE(OM_ERR_INVALID_OP_CODEC);

					break;
		
				case kOmfRGBCompSizes:
//					CHECK(GetFilePixelFormat(main, pdata, &format));
//					if (format == kOmfPixRGBA)
					{	
						vparms->operand.expCompSizeArray[0] = 8;
						vparms->operand.expCompSizeArray[1] = 8;
						vparms->operand.expCompSizeArray[2] = 8;
						vparms->operand.expCompSizeArray[3] = 0; /* terminating zero */
					}
//					else
//						RAISE(OM_ERR_INVALID_OP_CODEC);

					break;
		
				}
		  }
	}
//	XEXCEPT
//	XEND
	
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
static omfErr_t codecPutTDVCFieldInfo(omfCodecParms_t * info,
				                      omfMediaHdl_t media,
				                      omfHdl_t main,
				                      userDataTDVC_t * pdata)
{
	omfInt16     		numFields;
	omfVideoMemOp_t 	*vparms, oneVParm;
	omcTDVCPersistent_t *pers = (omcTDVCPersistent_t *)info->pers;
	omfRect_t			*aRect;
	omfUInt32			lineMapFlags = 0;

	XPROTECT(main) /* RPS 4/5/96 added to catch exception raised in switch default */
	{
	  memset(&oneVParm, 0, sizeof(oneVParm));
	  vparms = (omfVideoMemOp_t *)info->spc.mediaInfo.buf;
	for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
	{
		if(vparms->opcode == kOmfFrameLayout)
		{
			pdata->fileLayout = vparms->operand.expFrameLayout;
			numFields = ((pdata->fileLayout == kSeparateFields) ||
						 (pdata->fileLayout == kMixedFields) ? 2 : 1);
		}
	}
	
	vparms = (omfVideoMemOp_t *)info->spc.mediaInfo.buf;
	for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
	  {
		 switch(vparms->opcode)
			{
			case kOmfPixelFormat:
			  break;
			case kOmfFrameLayout:
			  pdata->fileLayout = vparms->operand.expFrameLayout;
			  break;
	
			case kOmfFieldDominance:
			 	pdata->fieldDominance = vparms->operand.expFieldDom;
			 	lineMapFlags |= 1;
			  	break;
	
			case kOmfVideoLineMap:
			 	pdata->videoLineMap[0] = vparms->operand.expLineMap[0];
			 	pdata->videoLineMap[1] = vparms->operand.expLineMap[1];
			 	lineMapFlags |= 2;
			  	break;
			  
			case kOmfSampledRect:
				aRect = &vparms->operand.expRect;
				pdata->sampledLength = aRect->ySize;
				pdata->sampledWidth = aRect->xSize;
				pdata->sampledXOffset = aRect->xOffset;
				pdata->sampledYOffset = aRect->yOffset;			
				pdata->descriptorFlushed = FALSE;
				break;
				
			case kOmfDisplayRect:
				aRect = &vparms->operand.expRect;
				if ((aRect->yOffset < 0) || (aRect->yOffset + aRect->ySize > 
											 pdata->imageLength))
					return(OM_ERR_INVALID_OP_CODEC);
		
				pdata->displayLength = aRect->ySize;
				pdata->displayWidth = aRect->xSize;
				pdata->displayXOffset = aRect->xOffset;
				pdata->displayYOffset = aRect->yOffset;			
				pdata->descriptorFlushed = FALSE;
				break;
				
			case kOmfStoredRect:
			  pdata->imageWidth = vparms->operand.expRect.xSize;
			  pdata->imageLength = vparms->operand.expRect.ySize;
			  break;

/*       RPS 4/5/96   pixel size is computed, not passed in...
			case kOmfPixelSize:
			  pdata->componentWidth = vparms->operand.expInt16 / 3;
			  break;
 */
			case kOmfAspectRatio:
			  pdata->imageAspectRatio = vparms->operand.expRational;
			  break;
			/* RPS 4/5/96 added CDCI attributes and default exception */
			case 	kOmfCDCICompWidth:	    /* operand.expInt32 */
				pdata->componentWidth = vparms->operand.expInt32;
				
				switch (pdata->componentWidth)
				{
				case 8:
				case 10:
				case 16:
					break;
					
				default:
					RAISE(OM_ERR_BADPIXFORM);
				}
				break;
								
			case 	kOmfCDCIHorizSubsampling:/* operand.expUInt32 */
				pdata->horizontalSubsampling = vparms->operand.expUInt32;
				
				switch (pdata->horizontalSubsampling)
				{
				case 1:
				case 2:
					break;
					
				default:
					RAISE(OM_ERR_BADPIXFORM);
				}
		
				/* RPS 4.5.96  Hmmm... a tough one here.  There is no rule that component
				   width and horizontal subsampling have to be specified at the same time.
				   However, fileBytesPerSample depends on both width and subsampling.  Recompute
				   here, but only if the fileBytesPerSample value already exists.   If it doesn't,
				   then the code following the switch will be executed. */
				
				if (pdata->fileBytesPerSample)
				{
					omfUInt32    		bitsPerSample;
					
					/* 4:4:4 = 1 sample for each of luma and two chroma channels */   
					if (pdata->horizontalSubsampling == 1)
					{
						pdata->bitsPerPixelAvg = (pdata->componentWidth * 3);
						bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
			  									pdata->bitsPerPixelAvg * numFields;
			  									
			  			/* compute pixel size and put in the file fmt */
			  			oneVParm.opcode = kOmfPixelSize;
			  			oneVParm.operand.expInt16 = (pdata->bitsPerPixelAvg + 7) / 8; /* round up */
			  			CHECK(omfmVideoOpAppend(main, kOmfForceOverwrite, oneVParm, pdata->fileFmt,
			  											MAX_FMT_OPS+1))
			  		}
			  		/* 4:2:2 = two-pixels get 2 samples of luma, 1 of each chroma channel, avg == 2 */							
			  		else if (pdata->horizontalSubsampling == 2)						
					{
						pdata->bitsPerPixelAvg = (pdata->componentWidth * 2);
						bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
			  									pdata->bitsPerPixelAvg * numFields;
			  									
			  			/* compute pixel size and put in the file fmt */
			  			oneVParm.opcode = kOmfPixelSize;
			  			oneVParm.operand.expInt16 = (pdata->bitsPerPixelAvg + 7) / 8; /* round up */
			  			CHECK(omfmVideoOpAppend(main, kOmfForceOverwrite, oneVParm, pdata->fileFmt,
			  											MAX_FMT_OPS+1))
			  		}										  									
			  		else
			  			RAISE(OM_ERR_ILLEGAL_FILEFMT);
			  									
					pdata->fileBytesPerSample = (bitsPerSample + 7) / 8;
				}

				break;
				
			case 	kOmfCDCIColorSiting:    /* operand.expColorSiting */
				pdata->colorSiting = vparms->operand.expColorSiting;
				break;
				
			case 	kOmfCDCIBlackLevel:     /* operand.expUInt32 */
				pdata->blackLevel = vparms->operand.expUInt32;
				break;
				
			case 	kOmfCDCIWhiteLevel:     /* operand.expUInt32 */
				pdata->whiteLevel = vparms->operand.expUInt32;
				break;
				
			case 	kOmfCDCIColorRange:     /* operand.expUInt32 */
				pdata->colorRange = vparms->operand.expUInt32;
				break;

			default:
			  	RAISE(OM_ERR_ILLEGAL_FILEFMT);
			}
	  }

		numFields = ((pdata->fileLayout == kSeparateFields) || 
						 (pdata->fileLayout == kMixedFields) ? 2 : 1);
		/* filePixelFormat is FIXED for compressed data */
		
		CHECK(omfmVideoOpMerge(main, kOmfForceOverwrite, 
					(omfVideoMemOp_t *) info->spc.mediaInfo.buf,
					pdata->fileFmt, MAX_FMT_OPS+1));
	
		/* filePixelFormat is FIXED for compressed data */
		
		/* START RPS 4/5/96 added default and synthetic data computation if needed and
		   available from component width */
		if (pdata->componentWidth)
		{
			if (! pdata->fileBytesPerSample)
			{
				omfUInt32    		bitsPerSample;
				
				/* RPS 2.28.96  Hmmm... a tough one here.  There is no rule that component
				   width and horizontal subsampling have to be specified at the same time.
				   However, fileBytesPerSample depends on both width and subsampling.  One option
				   is to recompute at the point of receiving a horizontal subsampling value,
				   but be sure that the fileBytesPerSample value already exists.   If it doesn't,
				   then this code will be run */
				
				/* 4:4:4 = 1 sample for each of luma and two chroma channels */   
				if (pdata->horizontalSubsampling == 1)
				{
					pdata->bitsPerPixelAvg = (pdata->componentWidth * 3);
					bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
		  									pdata->bitsPerPixelAvg * numFields;
		  									
		  			/* compute pixel size and put in the file fmt */
		  			oneVParm.opcode = kOmfPixelSize;
		  			oneVParm.operand.expInt16 = (pdata->bitsPerPixelAvg + 7) / 8; /* round up */
		  			CHECK(omfmVideoOpAppend(main, kOmfForceOverwrite, oneVParm, pdata->fileFmt,
		  											MAX_FMT_OPS+1))
		  		}
		  		/* 4:2:2 = two-pixels get 2 samples of luma, 1 of each chroma channel, avg == 2 */							
		  		else if (pdata->horizontalSubsampling == 2)						
				{
					pdata->bitsPerPixelAvg = (pdata->componentWidth * 2);
					bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
		  									pdata->bitsPerPixelAvg * numFields;
		  									
		  			/* compute pixel size and put in the file fmt */
		  			oneVParm.opcode = kOmfPixelSize;
		  			oneVParm.operand.expInt16 = (pdata->bitsPerPixelAvg + 7) / 8; /* round up */
		  			CHECK(omfmVideoOpAppend(main, kOmfForceOverwrite, oneVParm, pdata->fileFmt,
		  											MAX_FMT_OPS+1))
		  		}										  									
		  		else
		  			RAISE(OM_ERR_ILLEGAL_FILEFMT);
		  									
				pdata->fileBytesPerSample = (bitsPerSample + 7) / 8;
				pdata->memBytesPerSample = pdata->fileBytesPerSample;
			}
			
			if (! pdata->whiteLevel) /* maxint */
				pdata->whiteLevel = powi(2, pdata->componentWidth) -1;
				
			if (! pdata->colorRange )
				pdata->colorRange = powi(2, pdata->componentWidth) -2;
				
		}
		/* END RPS 3/28/96 */

		if (lineMapFlags && lineMapFlags != 3)
			RAISE(OM_ERR_ILLEGAL_FILEFMT);
	}

   XEXCEPT
   XEND
   
	return (setupStream(media, pdata));
}

static omfErr_t codecCloseTDVC(omfCodecParms_t *info,
									omfMediaHdl_t media,
				                      omfHdl_t main)
{
	userDataTDVC_t *pdata = (userDataTDVC_t *)media->userData;

	XPROTECT(media->mainFile)
	{
		CHECK(omcCloseStream(media->stream));
		if((media->openType == kOmfiCreated) || (media->openType == kOmfiAppended))
		{
			CHECK(postWriteOps(info, media));
			if(main->fmt == kOmfiMedia) // SPIDY
			{
				if ( ! pdata->descriptorFlushed )
					CHECK(writeDescriptorData(info, media, media->mainFile, pdata));
			}
		}
		
	  	if(pdata->frameIndex != NULL)
			omOptFree(media->mainFile, pdata->frameIndex);
		omOptFree(media->mainFile, media->userData);
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
/*

postWriteOps:

This routine is called by myCodecClose, but only if media was created.
It's purpose is to write non-media data to the descriptor, and
possibly to the data object itself.  This is the place to add any
post-write activities.

*/
static omfErr_t postWriteOps(omfCodecParms_t * info, omfMediaHdl_t media)
{
/*	omfInt32           padbytes = 0; */

	userDataTDVC_t *pdata;
	omfUInt32          frames;
	omfHdl_t        main = NULL;
	omcTDVCPersistent_t *pers;

	/* this routine will be called after sample data is written */

	omfAssertMediaHdl(media);
	main = media->mainFile;
	
	omfAssert(info, main, OM_ERR_NULL_PARAM);
 	pers = (omcTDVCPersistent_t *)info->pers;
 
	pdata = (userDataTDVC_t *) media->userData;
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
	 	/* fix the clip and track group lengths,they weren't known until now */
		if(main->fmt == kOmfiMedia) // SPIDY
		{
			XASSERT(media->fileMob != NULL, OM_ERR_INTERNAL_CORRUPTVINFO);
			XASSERT(media->mdes != NULL, OM_ERR_INTERNAL_CORRUPTVINFO);
				
	 		/* patch descriptor length */
	 		if(info->fileRev == kOmfRev1x || info->fileRev == kOmfRevIMA)
	 		{
				CHECK(omfsTruncInt64toUInt32(media->channels[0].numSamples, &frames)); /* OK 1.x */
	 			CHECK(omfsWriteInt32(main, media->mdes, OMMDFLLength, frames));
	 		}
	 		else
	 		{
	 			CHECK(omfsWriteLength(main, media->mdes, OMMDFLLength,
	 									media->channels[0].numSamples));
	 		}
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
static omfErr_t omfmTDVCGetMaxSampleSize(omfCodecParms_t * info,
					                 omfMediaHdl_t media,
					                 omfHdl_t main,
				                     userDataTDVC_t * pdata,
					                 omfInt32 * sampleSize)
{
//	omfInt32          *maxlen = (omfInt32 *) info->spc.mediaInfo.buf;
//	omfUInt32          numSamples;
	omcTDVCPersistent_t *pers = (omcTDVCPersistent_t *)info->pers;

	XPROTECT(main)
	{	
		/*
		 * if data is compressed, and will not be software decompressed, find
		 * the largest frame by scanning the frame index.  This may take a
		 * while
		 */
	
		if (media->compEnable == kToolkitCompressionDisable)
		{
//			omfUInt32            maxlen, i, thislen;

			*sampleSize = pdata->fixedFrameSize;
		} else
			RAISE (OM_ERR_JPEGDISABLED);
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

static omfErr_t setupStream(omfMediaHdl_t media, userDataTDVC_t * pdata)
{
	omfCStrmFmt_t   memFmt, fileFmt;

	XPROTECT(media->mainFile)
	{
		memFmt.mediaKind = media->pictureKind;
		memFmt.vfmt = pdata->fmtOps;
		memFmt.afmt = NULL;
		CHECK(omcSetMemoryFormat(media->stream, memFmt));
	
		fileFmt.mediaKind = media->pictureKind;
		fileFmt.vfmt = pdata->fileFmt;
		fileFmt.afmt = NULL;
		/* If compressed, use memory format here, as the compressor/decompressor will
		 * translate to the memory format
		 */
		pdata->fileFmt[0].opcode = kOmfFrameLayout;
		pdata->fileFmt[0].operand.expFrameLayout = pdata->fileLayout;
		
		pdata->fileFmt[1].opcode = kOmfFieldDominance;
		pdata->fileFmt[1].operand.expFieldDom = pdata->fieldDominance;

		pdata->fileFmt[2].opcode = kOmfPixelSize;
		pdata->fileFmt[2].operand.expInt16 = (omfInt16)pdata->bitsPerPixelAvg;

		pdata->fileFmt[3].opcode = kOmfStoredRect;
		pdata->fileFmt[3].operand.expRect.xOffset = 0;
		pdata->fileFmt[3].operand.expRect.xSize = pdata->imageWidth;
		pdata->fileFmt[3].operand.expRect.yOffset = 0;
		pdata->fileFmt[3].operand.expRect.ySize = pdata->imageLength;
		
		if(media->compEnable == kToolkitCompressionEnable)
		{
		  RAISE(OM_ERR_JPEGDISABLED);
		}
		else
		{
			pdata->fileFmt[4].opcode = kOmfPixelFormat;
pdata->fileFmt[4].operand.expPixelFormat = kOmfPixRGBA;		 // JeffB: Was YUV
			pdata->fileFmt[5].opcode = kOmfVFmtEnd;
		}
		
		CHECK(omcSetFileFormat(media->stream, fileFmt, 
							   media->stream->swapBytes));
							   
		CHECK(omfmSetSwabProc(media->stream, stdCodecSwabProc, stdCodecSwabLenProc,
								stdCodecNeedsSwabProc));		
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
static omfErr_t initUserData(userDataTDVC_t *pdata)
{
	omfHdl_t main = NULL;
	
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);
	
	pdata->imageWidth = 0;
	pdata->imageLength = 0;
	pdata->sampledWidth = 0;
	pdata->sampledLength = 0;
	pdata->sampledXOffset = 0;
	pdata->sampledYOffset = 0;
	pdata->displayWidth = 0;
	pdata->displayLength = 0;
	pdata->displayXOffset = 0;
	pdata->displayYOffset = 0;
	pdata->componentWidth = 0;
	pdata->rate.numerator = 0;
	pdata->rate.denominator = 0;

	pdata->fmtOps[0].opcode = kOmfVFmtEnd;
	pdata->imageAspectRatio.numerator = 0;
	pdata->imageAspectRatio.denominator = 0;
	pdata->fileLayout = kNoLayout;
	pdata->descriptorFlushed = FALSE;
	pdata->componentWidth = 0;
	pdata->fileBytesPerSample = 0;
	pdata->currentIndex = 0;
	pdata->fieldDominance = kNoDominant;
	pdata->videoLineMap[0] = 0;
	pdata->videoLineMap[1] = 0;
	pdata->maxIndex = 0;
	pdata->horizontalSubsampling = 1;
	pdata->colorSiting = kCoSiting;
	pdata->whiteLevel = 0;
	pdata->blackLevel = 0;
	pdata->colorRange = 0;
	pdata->bitsPerPixelAvg = 0;
	pdata->memBytesPerSample = 0;
	pdata->fixedFrameSize = 0;
	pdata->frameIndex = 0; // SPIDY
		
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
static omfErr_t InitTDVCCodec(omfCodecParms_t *info)
	{
	omcTDVCPersistent_t	*persist;
	
	XPROTECT(NULL)
	{
		persist = (omcTDVCPersistent_t *)omOptMalloc(NULL, sizeof(omcTDVCPersistent_t));
		if(persist == NULL)
			RAISE(OM_ERR_NOMEMORY);
		
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"ComponentWidth", OMClassCDCI, 
					   OMInt32, kPropRequired, &persist->omCDCIComponentWidth));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"HorizontalSubsampling", OMClassCDCI, 
					   OMUInt32, kPropRequired, &persist->omCDCIHorizontalSubsampling));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"ColorSiting", OMClassCDCI, 
					   OMBoolean, kPropOptional, &persist->omCDCIColorSiting));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"BlackReferenceLevel", OMClassCDCI, 
					   OMInt32, kPropOptional, &persist->omCDCIBlackReferenceLevel));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"WhiteReferenceLevel", OMClassCDCI, 
					   OMInt32, kPropOptional, &persist->omCDCIWhiteReferenceLevel));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"FrameSampleSize", OMClassDIDD, 
					   OMInt32, kPropRequired, &persist->omDIDDFrameSampleSize));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"ImageData", OMClassIDAT, 
					   OMDataValue, kPropRequired, &persist->omAvTDVCImageData));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"DIDResolutionID", "DIDD",
					   OMInt32, kPropRequired, &persist->omAvDIDDResolutionID));
	}
	XEXCEPT
	XEND
		
	info->spc.init.persistRtn = persist;
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
static omfErr_t codecGetCompressionParmsTDVC(omfCodecParms_t * info,
					                     omfHdl_t main,
				                     userDataTDVC_t * pdata)

{

	 return(OM_ERR_INVALID_OP_CODEC);
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
omfErr_t        codecPutCompressionParmsTDVC(omfCodecParms_t * info,
					                omfMediaHdl_t media,
					                     omfHdl_t main,
				                     userDataTDVC_t * pdata)
{
	return (OM_ERR_INVALID_OP_CODEC);
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
static omfErr_t InitMDESProps(omfCodecParms_t * info,
								omfMediaHdl_t 			media,
								omfHdl_t				file)
{
	omfRational_t	aspect;
	char				lineMap[2] = { 0, 0 };
	omfObject_t		obj = info->spc.initMDES.mdes ;
	omcTDVCPersistent_t *pers = (omcTDVCPersistent_t *)info->pers;
	
	XPROTECT(file)
	{
		CHECK(omfsWriteInt32(file, obj, OMDIDDStoredHeight, 0));
		CHECK(omfsWriteInt32(file, obj, OMDIDDStoredWidth, 0));
		
		CHECK(omfsWriteLayoutType(file, obj, OMDIDDFrameLayout, kOneField)); 
				
		CHECK(OMWriteBaseProp(file, obj, OMDIDDVideoLineMap,
						   OMInt32Array, sizeof(lineMap), lineMap));
				
		aspect.numerator = 0;
		aspect.denominator = 1;
		CHECK(omfsWriteRational(file, obj, OMDIDDImageAspectRatio, aspect));
		
		CHECK(omfsWriteInt32(file, obj, pers->omCDCIComponentWidth, 8));
		CHECK(omfsWriteUInt32(file, obj, pers->omCDCIHorizontalSubsampling, 2));
		CHECK(omfsWriteString(file, obj, OMDIDDCompression, DVC_COMPRESSION_METHOD));
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
static omfErr_t myCodecSetMemoryInfo(omfCodecParms_t * parmblk,
				                      omfMediaHdl_t media,
				                      omfHdl_t file,
				                      userDataTDVC_t *pdata)
{
	omfVideoMemOp_t 	*vparms;
	omfInt32				RGBFrom, memBitsPerPixel;

	omfAssertMediaHdl(media);
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);
	omfAssert(pdata, file, OM_ERR_NULL_PARAM);
	
	XPROTECT(file)
	{

		/* validate opcodes individually.  Some don't apply */
		
		vparms = ((omfVideoMemOp_t *)parmblk->spc.mediaInfo.buf);
		for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
		  {
			switch(vparms->opcode)
			  {
			  /* put the good ones up front */
			  case kOmfPixelFormat:
				  media->pvt->pixType =  vparms->operand.expPixelFormat;
				  CHECK(setupStream(media, pdata));		/* This can affect logical file format for swabbing */
			     break;
			  case kOmfFrameLayout:		
			  case kOmfFieldDominance:
			  case kOmfStoredRect:
			  case kOmfAspectRatio:
			  case kOmfRGBCompLayout:
			  case kOmfLineLength:
			  		break;
			  		
			  case kOmfRGBCompSizes:
			  		memBitsPerPixel = 0;
			  		RGBFrom = 0;
			  		while (vparms->operand.expCompSizeArray[RGBFrom])
			  		{
			  			memBitsPerPixel += vparms->operand.expCompSizeArray[RGBFrom];
			  			RGBFrom++;
			  		}
			  		pdata->memBytesPerSample = (pdata->fileBytesPerSample * memBitsPerPixel) / pdata->bitsPerPixelAvg;
			  	break;
			  	
			  default:
			  	RAISE(OM_ERR_ILLEGAL_MEMFMT);
			  }
		  }
	
		CHECK(omfmVideoOpInit(file, pdata->fmtOps));
		CHECK(omfmVideoOpMerge(file, kOmfForceOverwrite, 
					(omfVideoMemOp_t *) parmblk->spc.mediaInfo.buf,
					pdata->fmtOps, MAX_FMT_OPS+1));
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
