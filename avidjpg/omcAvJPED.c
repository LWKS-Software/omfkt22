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
#include "omcAvJPED.h" 
#include "omJPEG.h"
#include "omPvt.h"

#define MAX_FMT_OPS	32

typedef struct
{
	omfProperty_t	omCDCIComponentWidth;
	omfProperty_t	omCDCIHorizontalSubsampling;
	omfProperty_t	omCDCIColorSiting;
	omfProperty_t	omCDCIBlackReferenceLevel;
	omfProperty_t	omCDCIWhiteReferenceLevel;
	omfProperty_t	omAvJPEDResolutionID;
	omfProperty_t	omJPEGFrameIndex;
	omfProperty_t	omAvJPEDImageData;
	omfProperty_t	omAvJPEDQTab;
	omfProperty_t	omAvJPEDQTab2;
	omfProperty_t	omAvJPEDCompress;
}				omcAvJPEDPersistent_t;

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
	jpeg_tables     compressionTables[3];
	omfBool			 customTables;
	omfJPEGInfo_t   jpeg;
	omfFieldDom_t	fieldDominance;
	omfInt32				videoLineMap[2];
	omfUInt32         *frameIndex;
	omfUInt32          maxIndex;
	omfUInt32		horizontalSubsampling;
	omfColorSiting_t	colorSiting;
	omfUInt32			whiteLevel;
	omfUInt32			blackLevel;
	omfUInt32			colorRange;
	omfInt32				bitsPerPixelAvg;
	omfInt32				memBytesPerSample;
}               userDataJPEG_t;

static omfErr_t codecSetJPEGTables(omfCodecParms_t * info, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecGetJPEGTables(omfCodecParms_t * info, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecGetCompressionParmsJPEG(omfCodecParms_t * info, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecPutCompressionParmsJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);

static omfErr_t InitAvJPEGCodec(omfCodecParms_t *info);
static omfErr_t codecOpenJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecCreateJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecGetJPEGFieldInfo(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecPutJPEGFieldInfo(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecGetRect(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecWriteSamplesJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecReadSamplesJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecSetRect(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t setupStream(omfMediaHdl_t media, userDataJPEG_t * pdata);
static omfErr_t codecCloseJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t omfmJPEGSetFrameNumber(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t initUserData(userDataJPEG_t *pdata);
static omfErr_t postWriteOps(omfCodecParms_t * info, omfMediaHdl_t media);
static omfErr_t writeDescriptorData(omfCodecParms_t * info, 
									omfMediaHdl_t media, 
									omfHdl_t main, 
									userDataJPEG_t * pdata);
static omfErr_t loadDescriptorData(omfCodecParms_t * info, 
								   omfMediaHdl_t media, 
								   omfHdl_t main, 
								   userDataJPEG_t * pdata);
static omfErr_t omfsGetNthFrameIndex(omfHdl_t file, omfObject_t obj,omfProperty_t prop,
								omfUInt32 *data, omfInt32 i);

static omfErr_t omfsAppendFrameIndex(omfHdl_t file, omfObject_t obj, omfProperty_t prop,
								omfUInt32 data);

static omfInt32 omfsLengthFrameIndex( omfHdl_t file, omfObject_t obj, omfProperty_t prop);
static omfErr_t omfmJPEGGetMaxSampleSize(omfCodecParms_t * info,
					                 omfMediaHdl_t media,
					                 omfHdl_t main,
				                     userDataJPEG_t * pdata,
					                 omfInt32 * sampleSize);

static omfErr_t InitMDESProps(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				file);
static omfErr_t myCodecSetMemoryInfo(omfCodecParms_t * parmblk,
				                      omfMediaHdl_t media,
				                      omfHdl_t file,
				                      userDataJPEG_t * pdata);
static omfErr_t AddFrameOffset(omfMediaHdl_t media, omfUInt32 offset);
static omfErr_t codecGetInfoJPED(omfCodecParms_t * parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main);
static omfErr_t codecPutInfoJPED(omfCodecParms_t * parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main);
static omfErr_t codecGetNumChannelsJPEG(
			omfCodecParms_t	*parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main);
static omfErr_t codecGetSelectInfoJPEG(
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

omfErr_t        omfCodecAvJPED(omfCodecFunctions_t func, omfCodecParms_t * info)
{
	omfErr_t        	status;
/*	omfMediaHdl_t   	media;
	omfHdl_t        	main;
	omfDDefObj_t		dataKind;
	omcAvJPEDPersistent_t *pers = (omcAvJPEDPersistent_t *)info->pers;
*/	omfCodecOptDispPtr_t	*dispPtr;
	
	switch (func)
	{
	case kCodecInit:
		InitAvJPEGCodec(info);
		return (OM_ERR_NONE);
		break;

	case kCodecGetMetaInfo:
		strncpy((char *)info->spc.metaInfo.name, "Avid JPEG Codec (Compressed ONLY)", info->spc.metaInfo.nameLen);
		info->spc.metaInfo.info.mdesClassID = "JPED";
		info->spc.metaInfo.info.dataClassID = "JPEG";
		info->spc.metaInfo.info.codecID = CODEC_AVID_JPEG_VIDEO;
		info->spc.metaInfo.info.rev = CODEC_REV_3;
		info->spc.metaInfo.info.dataKindNameList = PICTUREKIND;
		info->spc.metaInfo.info.minFileRev = kOmfRev1x;
		info->spc.metaInfo.info.maxFileRevIsValid = FALSE;		/* There is no maximum rev */
		return (OM_ERR_NONE);
	case kCodecLoadFuncPointers:
		dispPtr = info->spc.loadFuncTbl.dispatchTbl;

		dispPtr[ kCodecOpen ] = codecOpenJPEG;
		dispPtr[ kCodecCreate ] = codecCreateJPEG;
		dispPtr[ kCodecGetInfo ] = codecGetInfoJPED;
		dispPtr[ kCodecPutInfo ] = codecPutInfoJPED;
		dispPtr[ kCodecReadSamples ] = codecReadSamplesJPEG;
		dispPtr[ kCodecWriteSamples ] = codecWriteSamplesJPEG;
		dispPtr[ kCodecClose ] = codecCloseJPEG;
		dispPtr[ kCodecSetFrame ] = omfmJPEGSetFrameNumber;
		dispPtr[ kCodecGetNumChannels ] = codecGetNumChannelsJPEG;
		dispPtr[ kCodecGetSelectInfo ] = codecGetSelectInfoJPEG;
		dispPtr[ kCodecInitMDESProps ] = InitMDESProps;
/*
		dispPtr[ kCodecAddFrameIndexEntry ] = NULL;
		dispPtr[ kCodecReadLines ] = NULL;
		dispPtr[ kCodecImportRaw ] = NULL;
		dispPtr[ kCodecWriteLines ] = NULL;
		dispPtr[ kCodecGetVarietyInfo ] = NULL;
 		dispPtr[ kCodecGetVarietyCount ] = NULL;
		dispPtr[ kCodecSemanticCheck ] = NULL;
*/
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
 * codecGetNumChannelsJPEG	(INTERNAL)
 *
 *			Find the number of channels in the current media.
 */
static omfErr_t codecGetNumChannelsJPEG(
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
 * codecGetSelectInfoJPEG	(INTERNAL)
 *
 *			Get the information required to select a codec.
 */
static omfErr_t codecGetSelectInfoJPEG(
			omfCodecParms_t	*info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfErr_t			status = OM_ERR_NONE;
	char				compress[256];
	omcAvJPEDPersistent_t *pers = (omcAvJPEDPersistent_t *)info->pers;

		if(omfsIsPropPresent(info->spc.selectInfo.file, info->spc.selectInfo.mdes, OMDIDDCompression, OMString))
		{
			status = omfsReadString(info->spc.selectInfo.file, info->spc.selectInfo.mdes, OMDIDDCompression, 
											compress, sizeof(compress));
			if(status != OM_ERR_NONE)
				return(status);
			info->spc.selectInfo.info.willHandleMDES = (strcmp(compress, "AJPG") == 0) || (strcmp(compress, "1095389255") == 0);
		}
		else if(omfsIsPropPresent(info->spc.selectInfo.file, info->spc.selectInfo.mdes, pers->omAvJPEDCompress, OMObjectTag))
		{
			omfClassID_t		pvtClass;
			status = omfsReadClassID(info->spc.selectInfo.file, info->spc.selectInfo.mdes, pers->omAvJPEDCompress, 
											pvtClass);
			if(status != OM_ERR_NONE)
				return(status);
			info->spc.selectInfo.info.willHandleMDES = (strncmp(pvtClass, "AJPG", 4) == 0) ||
													   (strncmp(pvtClass, "1095389255", 4) == 0);
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
 * codecGetInfoJPED	(INTERNAL)
 *
 */
static omfErr_t codecGetInfoJPED(omfCodecParms_t *info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfErr_t			status = OM_ERR_NONE;
	omfMaxSampleSize_t	*ptr;
	omfFrameSizeParms_t	*singlePtr;
	userDataJPEG_t 		*pdata = (userDataJPEG_t *) media->userData;
	omfUInt32			frameSize, frameOff;

	switch (info->spc.mediaInfo.infoType)
	{
	case kMediaIsContiguous:
		{
		status = OMIsPropertyContiguous(media->mainFile, media->dataObj, OMIDATImageData,
										OMDataValue, (omfBool *)info->spc.mediaInfo.buf);
		}
		break;
	case kVideoInfo:
		status = codecGetJPEGFieldInfo(info, media, main, pdata);
		break;
	case kCompressionParms:
		status = codecGetCompressionParmsJPEG(info, main, pdata);
		break;
	case kJPEGTables:
		status = codecGetJPEGTables(info, main, pdata);
		break;
	case kMaxSampleSize:
		ptr = (omfMaxSampleSize_t *) info->spc.mediaInfo.buf;
		if(ptr->mediaKind == media->pictureKind)
		{
			status = omfmJPEGGetMaxSampleSize(info, media, main, pdata, (omfInt32 *)&pdata->memBytesPerSample);
			ptr->largestSampleSize = pdata->memBytesPerSample;
		}
		else
			status = OM_ERR_CODEC_CHANNELS;
		break;
	
	case kSampleSize:
		singlePtr = (omfFrameSizeParms_t *)info->spc.mediaInfo.buf;
		if(singlePtr->mediaKind == media->pictureKind)
		{
			status = omfsTruncInt64toUInt32(singlePtr->frameNum, &frameOff);
			if(status != OM_ERR_NONE)
				break;
				
			frameOff--;		/* pdata->frameIndex is 0-based */
			if(pdata->frameIndex == NULL)
				status = OM_ERR_NOFRAMEINDEX;
			else if(frameOff < 0 || frameOff >= pdata->maxIndex)
				status = OM_ERR_BADFRAMEOFFSET;
			else
			{
				frameSize = pdata->frameIndex[frameOff + 1] -
							pdata->frameIndex[frameOff];
				omfsCvtUInt32toInt64(frameSize, &singlePtr->frameSize);
				status = OM_ERR_NONE;
			}
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
 * codecPutInfoJPED	(INTERNAL)
 *
 */
static omfErr_t codecPutInfoJPED(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfErr_t			status = OM_ERR_NONE;
	userDataJPEG_t 		*pdata = (userDataJPEG_t *) media->userData;

	switch (info->spc.mediaInfo.infoType)
	{
	case kVideoInfo:
		status = codecPutJPEGFieldInfo(info, media, main, pdata);
		break;
	case kCompressionParms:
		status = codecPutCompressionParmsJPEG(info, media, main, pdata);
		break;
	case kVideoMemFormat:
		status = myCodecSetMemoryInfo(info, media, main, pdata);
		break;
	case kJPEGTables:
		status = codecSetJPEGTables(info, main, pdata);
		break;

	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}
	
	return(status);
}

static omfErr_t codecCreateJPEG(omfCodecParms_t * info,
								omfMediaHdl_t media,
				                omfHdl_t main)
{
	userDataJPEG_t *pdata;
	omcAvJPEDPersistent_t *pers = (omcAvJPEDPersistent_t *)info->pers;

	omfAssertMediaHdl(media);
	omfAssert(info, main, OM_ERR_NULL_PARAM);
	
	XPROTECT(main)
	{
	  media->pvt->pixType	= kOmfPixRGBA;		 /* JeffB: Was YUV */
		/* copy descriptor information into local structure */
		media->userData = (userDataJPEG_t *)omOptMalloc(main, sizeof(userDataJPEG_t));
		XASSERT(media->userData != NULL, OM_ERR_NOMEMORY);
	
		pdata = (userDataJPEG_t *) media->userData;
		
		CHECK(initUserData(pdata));
		
		/* there are no multiword entities (YET!) */
		media->stream->swapBytes = FALSE; 
	
		/* get sample rate from MDFL parent class of descriptor.  
		 * This assumes that the JPEG descriptor class is a descendant of 
		 * MDFL. 
		 */
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
		
		/* number of frames is unknown until close time */
		omfsCvtInt32toInt64(0, &media->channels[0].numSamples);
	
		pdata->fmtOps[0].opcode = kOmfVFmtEnd;
		
		MakeRational(4, 3, &(pdata->imageAspectRatio));
		CHECK(AddFrameOffset(media, 0));
		CHECK(omcOpenStream(media->mainFile, media->dataFile, media->stream, 
						media->dataObj, pers->omAvJPEDImageData, OMDataValue));
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
static omfErr_t codecOpenJPEG(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
			                      omfHdl_t main)
{
	userDataJPEG_t 		*pdata = (userDataJPEG_t *) media->userData;

	omfAssertMediaHdl(media);

	/* pull out descriptor information BEFORE reading JPEG data */
	XPROTECT(main)
	{
	  media->pvt->pixType	= kOmfPixRGBA;		 /* JeffB: Was YUV */

		media->userData = (userDataJPEG_t *)omOptMalloc(main, sizeof(userDataJPEG_t));
		XASSERT(media->userData != NULL, OM_ERR_NOMEMORY);
		pdata = (userDataJPEG_t *) media->userData;
				
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
static omfErr_t codecWriteSamplesJPEG(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
				                      omfHdl_t main)
{
	omfmMultiXfer_t 	*xfer = NULL;
	omfUInt32			offset;
	omfInt32 			fieldLen;
	omfUInt32			n;
	omcAvJPEDPersistent_t *pers = (omcAvJPEDPersistent_t *)info->pers;
	userDataJPEG_t 		*pdata = (userDataJPEG_t *) media->userData;
	JPEG_MediaParms_t	compressParms;
	
	omfAssertMediaHdl(media);
	
	XPROTECT(main)
	{
		compressParms.media = media;		
		XASSERT(pdata, OM_ERR_NULL_PARAM);
	
		XASSERT(pdata->componentWidth != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->fileBytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

	  compressParms.isCCIR = FALSE;
	  if(pdata->jpeg.JPEGTableID >0 && pdata->jpeg.JPEGTableID < 256)
		{
		  if(pdata->imageWidth == 720 || pdata->imageWidth == 352)
			compressParms.isCCIR = TRUE;
		}
		xfer = info->spc.mediaXfer.xfer;
		XASSERT(xfer, OM_ERR_NULLBUF);
		
		/* this codec only allows one-channel media */
		XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
	
		if (media->compEnable == kToolkitCompressionEnable)
		{
			compressParms.blackLevel = pdata->blackLevel;
			compressParms.whiteLevel = pdata->whiteLevel;
			compressParms.colorRange = pdata->colorRange;
		/*
		 * !!! Currently we don't swab on writes, change this for all codecs
		 * Later: alloc a temp buffer is we are to swab
		 * the data then swab the data and free the buffer
		 * return(def->fileFormat.vfmt.format != def->memFormat.vfmt.format);
		 * src.fmt = def->fileFormat;
		 * src.buf = def->swabBuf;
		 * src.buflen = readLen; dest.fmt = def->memFormat;
		 * dest.buf = buffer;
		 * omcComputeBufferSize(media, readLen, &dest.buflen);
		 * OMF_EXT_CHECK_ERR_RETURN(media->mainFile);
		 * omfErr_t omcSwabData(omfMediaHdl_t media,
		 * omfCStrmSwab_t *src, omfCStrmSwab_t *dest, omfBool swapBytes);
		 * OMF_EXT_CHECK_ERR_RETURN(media->mainFile);
          */
			xfer->bytesXfered = 0;
			xfer->samplesXfered = 0;
			for(n = 0; n < xfer->numSamples; n++)
			{
				compressParms.pixelBuffer = (char *)xfer->buffer + (n * pdata->memBytesPerSample);
				compressParms.pixelBufferIndex = 0;
				compressParms.pixelBufferLength = pdata->memBytesPerSample;
	
				CHECK(omfmJPEDCompressSample(&compressParms, pdata->customTables, &fieldLen));
		
				xfer->bytesXfered += fieldLen;
				if (xfer->bytesXfered && (pdata->fileLayout == kSeparateFields))
				{
					CHECK(omfmJPEDCompressSample(&compressParms, pdata->customTables, &fieldLen));
					xfer->bytesXfered += fieldLen;
				}
				xfer->samplesXfered++;
				/* Don't forget to update the global frame counter */
				CHECK(omfsAddInt32toInt64(1, &media->channels[0].numSamples));
				CHECK(omcGetStreamPos32(media->stream, &offset));
				CHECK(AddFrameOffset(media, offset));
			}
		} else			/* already compressed by user */
		{	
			XASSERT(xfer->numSamples == 1, OM_ERR_ONESAMPLEWRITE);
			CHECK(omcWriteStream(media->stream, xfer->buflen, xfer->buffer));
			xfer->bytesXfered = xfer->buflen;
			xfer->samplesXfered = xfer->numSamples;

			/* Don't forget to update the global frame counter */
			CHECK(omfsAddInt32toInt64(1, &media->channels[0].numSamples));
			CHECK(omcGetStreamPos32(media->stream, &offset));
			CHECK(AddFrameOffset(media, offset));
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
static omfErr_t codecReadSamplesJPEG(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
				                     omfHdl_t main)
{
	userDataJPEG_t 	*pdata = NULL;
	omfUInt32			fileBytes = 0;
	omfUInt32			bytesRead = 0;
	omfmMultiXfer_t	*xfer = NULL;
	omfUInt32			rBytes, startPos, nBytes;
	omcAvJPEDPersistent_t *pers = (omcAvJPEDPersistent_t *)info->pers;
	omfCStrmSwab_t		src, dest;
	char					*tempBuf = NULL;
	omfUInt32			n;
#ifdef JPEG_TRACE
	char              errmsg[256];
	omfUInt32			offset;
#endif
	omfErr_t				status;
	omfInt32 frameIndexLen, decompBytesPerSample;
	JPEG_MediaParms_t	compressParms;
	
	omfAssertMediaHdl(media);
	
	XPROTECT(main)
	{
		pdata = (userDataJPEG_t *) media->userData;
		XASSERT(pdata, OM_ERR_NULL_PARAM);
	
		XASSERT(pdata->componentWidth != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->fileBytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		xfer = info->spc.mediaXfer.xfer;
		XASSERT(xfer, OM_ERR_NULLBUF);
	
		xfer->bytesXfered = 0;
		xfer->samplesXfered = 0;
		
		XASSERT(media->numChannels == 1, OM_ERR_CODEC_CHANNELS);
		XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
					
		compressParms.media = media;		
	
		if (media->compEnable == kToolkitCompressionEnable)
		{
			omfInt32           compressOkay;

			compressParms.blackLevel = pdata->blackLevel;
			compressParms.whiteLevel = pdata->whiteLevel;
			compressParms.colorRange = pdata->colorRange;

			for(n = 0; n < xfer->numSamples; n++)
			{
				if (pdata->frameIndex == NULL)
					RAISE(OM_ERR_NOFRAMEINDEX);
				CHECK(omfsTruncInt64toInt32(media->channels[0].numSamples, &frameIndexLen));
				if ((pdata->currentIndex+1) > frameIndexLen)
					RAISE(OM_ERR_BADFRAMEOFFSET);
		
				nBytes = pdata->frameIndex[pdata->currentIndex + 1] -
				  pdata->frameIndex[pdata->currentIndex];
				startPos = pdata->frameIndex[pdata->currentIndex];
								
				/* skip the IFH */
				CHECK(omcSeekStream32(media->stream, startPos));	
				pdata->currentIndex++;
	
				src.fmt = media->stream->fileFormat;
				src.buflen = pdata->memBytesPerSample;
				dest.fmt = media->stream->memFormat;
				dest.buf = (char *)xfer->buffer + (n * pdata->memBytesPerSample);
				dest.buflen = pdata->memBytesPerSample;
				if((dest.buflen * xfer->numSamples) > xfer->buflen)
					RAISE(OM_ERR_SMALLBUF);

				if(media->pvt->pixType == kOmfPixRGBA)
					decompBytesPerSample = 	(omfInt32)pdata->imageWidth *
										(omfInt32) pdata->imageLength *
										(omfInt32) 3 * 
										(pdata->fileLayout == kSeparateFields ? 2 : 1);
				else
					decompBytesPerSample = pdata->fileBytesPerSample;

				if(decompBytesPerSample > pdata->memBytesPerSample)
				{
					tempBuf = (char *)omOptMalloc(main, decompBytesPerSample);
					if(tempBuf == NULL)
						RAISE(OM_ERR_NOMEMORY);
					src.buf = tempBuf;
					src.buflen = decompBytesPerSample;
					compressParms.pixelBuffer = (char *)tempBuf;
					compressParms.pixelBufferIndex = 0;
					compressParms.pixelBufferLength = decompBytesPerSample;
				}
				else
				{
					src.buf = dest.buf;
					src.buflen = decompBytesPerSample;
					compressParms.pixelBuffer = (char *)src.buf;
					compressParms.pixelBufferIndex = 0;
					compressParms.pixelBufferLength = decompBytesPerSample;
				}
	
				compressOkay = (omfmJPEDDecompressSample(&compressParms) == OM_ERR_NONE);
				if (!compressOkay)
					RAISE(OM_ERR_DECOMPRESS);
	
				if (compressOkay && (pdata->fileLayout == kSeparateFields))
				{
					omfUInt8           data;
					omfUInt32          saveOffset;
	
					CHECK(omcGetStreamPos32(media->stream, &saveOffset));
	
					/*
					 * look for a restart marker separating the
					 * two fields
					 */
					CHECK(omcReadStream(media->stream, 1, &data, NULL));
	
	/*				if (data == 0xFF)	*/
					{
						while (data == 0xFF)
						{
							CHECK(omcReadStream(media->stream, 1, &data, NULL));
						}
						if (0xD0 <= data && data <= 0xD7)	/* skip restart marker */
						{
							CHECK(omcGetStreamPos32(media->stream, &saveOffset));
						}
						/*
						 * Avid Media Suite Pro on the SGI
						 * inserts an EOI marker between
						 * fields.  Skip past it if it
						 * exists.
						 */
						if (data == 0xD9)
						{
							CHECK(omcGetStreamPos32(media->stream, &saveOffset));
						}
					}
					CHECK(omcSeekStream32(media->stream, saveOffset));
	
					compressOkay = (omfmJPEDDecompressSample(&compressParms) == OM_ERR_NONE);
					if (!compressOkay)
						RAISE(OM_ERR_DECOMPRESS);
				}
				
				CHECK(omcSwabData(media->stream, &src, &dest, media->stream->swapBytes));
				if(tempBuf != NULL)
				{
					omOptFree(main, tempBuf);
					tempBuf = NULL;
				}
				xfer->bytesXfered += pdata->memBytesPerSample;
				xfer->samplesXfered++;
			}
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
									userDataJPEG_t * pdata)
{
	omcAvJPEDPersistent_t *pers;
	omfUInt8			JFIFHeader[133+3], *hdrPtr;	/* SOI + DQT */
	omfInt32			i, n;
	omfErr_t			status;
	omfPosition_t		zeroPos;
	
	/* this routine will be called after sample data is written */

	omfsCvtInt32toPosition(0, zeroPos);
	omfAssertMediaHdl(media);
	main = media->mainFile;
	
	omfAssert(info, main, OM_ERR_NULL_PARAM);
 	pers = (omcAvJPEDPersistent_t *)info->pers;
 
	pdata = (userDataJPEG_t *) media->userData;
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
		CHECK(omfsWriteInt32(main, media->mdes, pers->omAvJPEDResolutionID, pdata->jpeg.JPEGTableID));

		/**** Build a small JFIF header & write */
		hdrPtr = JFIFHeader;
		*hdrPtr++ = 0xFF;
		*hdrPtr++ = 0xD8;
		*hdrPtr++ = 0xFF;
		*hdrPtr++ = 0xDB;
		*hdrPtr++ = 0x00;	/* Table Length (part 1)	*/
		*hdrPtr++ = 0x84;	/* Table Length (part 2)	*/
		*hdrPtr++ = 0x00;	/* 8 bit luma	*/
		if(pdata->compressionTables[0].Qlen != 0)
		{
			for (i = 0; i < 64; i++)
			{
				*hdrPtr++ = pdata->compressionTables[0].Q[i];
			}
			*hdrPtr++ = 0x01;	/* 8 bit chroma	*/
			for (i = 0; i < 64; i++)
			{
				*hdrPtr++ = pdata->compressionTables[1].Q[i];
			}
		}
		else
		{
			memcpy(hdrPtr, STD_QT_PTR[0], 64L);
			hdrPtr += 64;
			*hdrPtr++ = 0x01;	/* 8 bit chroma	*/
			memcpy(hdrPtr, STD_QT_PTR[1], 64L);
			hdrPtr += 64;
		}

		if(info->fileRev == kOmfRev2x)
		{
			omfObject_t	pictureKind;
			omfiDatakindLookup(main, PICTUREKIND, &pictureKind, &status);
			CHECK(omfsWriteDataValue(main, media->mdes, pers->omAvJPEDQTab2,
							pictureKind, JFIFHeader, zeroPos, sizeof(JFIFHeader)));
		}
		else
		{
			CHECK(omfsWriteVarLenBytes(main, media->mdes, pers->omAvJPEDQTab,
							zeroPos, sizeof(JFIFHeader), JFIFHeader));
		}

		for(n = 1; n <= pdata->currentIndex; n++)
		{
			CHECK(omfsAppendFrameIndex(main, media->dataObj,  pers->omJPEGFrameIndex,
												pdata->frameIndex[n-1]));
		}

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
								   userDataJPEG_t * pdata)
{
	omfInt32 				sampleFrames = 0;
	omfUInt32 				layout = 0;
	omfUInt32 				bytesPerPixel = 0;
	omfUInt32 				numFields = 0;
	omfUInt32				indexSize, n, indexVal;
	omcAvJPEDPersistent_t *pers = NULL;
	omfPosition_t			zeroPos;
	omfErr_t					status;
	omfLength_t				qtabLen64;
	omfInt32				qtabLen32, len;
	omfUInt32				bytesProcessed;
	omfInt16				tableType, tableindex, elem16, tableBytes, i;
#if OMFI_ENABLE_SEMCHECK
	omfBool					saveCheck;
#endif
	char					*JFIFHeader;
	unsigned char			*ptr, *src;
	omfUInt16				src16;
	
	omfsCvtInt32toPosition(0, zeroPos);
	omfAssertMediaHdl(media);
	omfAssert(info, main, OM_ERR_NULL_PARAM);
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
		pers = (omcAvJPEDPersistent_t *)info->pers;
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
	
		media->numChannels = 1;
	
		CHECK(omcOpenStream(media->mainFile, media->dataFile, media->stream, 
						media->dataObj, pers->omAvJPEDImageData, OMDataValue));
			
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
			
			/* 4:4:4 = 1 sample for each of luma and two chroma channels */   
			if (pdata->horizontalSubsampling == 1)
			{
				pdata->bitsPerPixelAvg = (pdata->componentWidth * 3);
				bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
	  									(pdata->componentWidth * 3) * numFields;
	  		}							
	  		/* 4:2:2 = two-pixels get 2 samples of luma, 1 of each chroma channel, avg == 2 */							
	  		else if (pdata->horizontalSubsampling == 2)						
			{
				pdata->bitsPerPixelAvg = (pdata->componentWidth * 2);
				bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
	  									(pdata->componentWidth * 2) * numFields;
	  		}							
	  		else
	  			RAISE(OM_ERR_ILLEGAL_FILEFMT);
	  									
			pdata->fileBytesPerSample = (bitsPerSample + 7) / 8;

	  		if(pdata->bitsPerPixelAvg != 0 && media->pvt->pixType == kOmfPixRGBA)		/* defaults to RGB888 */
	 	 		pdata->memBytesPerSample = (pdata->fileBytesPerSample * 24L) / pdata->bitsPerPixelAvg;
			else
				pdata->memBytesPerSample = pdata->fileBytesPerSample;
		}
				
		CHECK(omfsReadInt32(main, media->mdes, pers->omAvJPEDResolutionID, &pdata->jpeg.JPEGTableID));
		
		/* For MC files, they made the display yOffset relative to the sampledRectangle instead of the stored
		 * fix it.
		 */
		/* The displayYOffset will always be 13/16 for CCIR in AvJPED.  We don't
		 * need to handle the 5/8 case, because JFIF can't be embedded into AvJPED
		 */
		if(pdata->jpeg.JPEGTableID != 0 && pdata->imageWidth > 640)
		{
			omfVideoSignalType_t	signal;
			CHECK(omfmSourceGetVideoSignalType(media, &signal));
			if(signal == kPALSignal)
			{
				pdata->displayYOffset = 16;
			}
			else
			{
				pdata->displayYOffset = 13;
			}
			pdata->imageLength = pdata->displayLength + pdata->displayYOffset;
		}
		
		if(info->fileRev == kOmfRev2x)
			qtabLen64 = omfsLengthDataValue(main, media->mdes, pers->omAvJPEDQTab2);
		else
			qtabLen64 = omfsLengthVarLenBytes(main, media->mdes, pers->omAvJPEDQTab);
		CHECK(omfsTruncInt64toInt32(qtabLen64, &qtabLen32));
		JFIFHeader = (char *) omOptMalloc(main, (size_t)qtabLen32);
		if((qtabLen32 != 0) && (JFIFHeader != NULL))
		{
			omfUInt32		bytesRead;
			
			if(info->fileRev == kOmfRev2x)
			{
				omfObject_t	nilKind;
				omfiDatakindLookup(main, PICTUREKIND, &nilKind, &status);
				CHECK(omfsReadDataValue(main, media->mdes, pers->omAvJPEDQTab2,
								nilKind, JFIFHeader, zeroPos, qtabLen32, &bytesRead));
			}
			else
			{
				CHECK(omfsReadVarLenBytes(main, media->mdes, pers->omAvJPEDQTab,
								zeroPos, qtabLen32, JFIFHeader, &bytesRead));
			}
			bytesProcessed = 0;
			ptr = (unsigned char *)JFIFHeader;
			while(bytesProcessed < bytesRead)
			{
				if(*ptr != 0xFF)
					break;				/* Error readign header!!! */
				ptr++;
				switch(*ptr)
				{
					case 0xD8:
						ptr++;
						break;
						
					case 0xDB:	
						ptr++;
						len = (ptr[0] << 8L) | ptr[1];
						tableBytes = 2;				/* account for 2 byte length field */
						src = ptr + 2;
						while(tableBytes < len)
						{
							tableType = *src++;
							tableBytes++;			/* account for table type */
							tableindex = tableType & 0x0F;
							elem16 = ((tableType & 0xF0) == 0x10);
							if(elem16)
							{
								for (i = 0; i < 128; i+=2)
								{
									src16 = (src[i] << 8) | src[i+1];
									pdata->compressionTables[tableindex].Q[i] = (omfUInt8)src16;
									pdata->compressionTables[tableindex].Q16[i] = src16;
								}
								tableBytes += 128;
								src += 128;
							}
							else
							{
								for (i = 0; i < 64; i++)
								{
									pdata->compressionTables[tableindex].Q[i] = src[i];
									pdata->compressionTables[tableindex].Q16[i] = src[i];
								}
								tableBytes += 64;
								src += 64;
								pdata->compressionTables[tableindex].Qlen = 64;
								pdata->compressionTables[tableindex].Q16len = 128;
							}
						}
						ptr += len;
						break;

					case 0xC4:
						ptr++;
						len = (ptr[0] << 8L) | ptr[1];
						tableBytes = 2;				/* account for 2 byte length field */
						src = ptr + 2;
						while(tableBytes < len)
						{
							tableType = *src++;
							tableBytes++;
							tableindex = (tableType & 0x01 ? 1 : 0);
							if((tableType & 0x10) != 0)
							{
								for (i = 0; i < 178; i++)
									pdata->compressionTables[tableindex].AC[i] = src[i];
								tableBytes += 178;
								src += 178;
								pdata->compressionTables[tableindex].AClen = 178;
							}
							else
							{
								for (i = 0; i < 28; i++)
									pdata->compressionTables[tableindex].DC[i] = src[i];
								tableBytes += 28;
								src += 28;
								pdata->compressionTables[tableindex].DClen = 28;
							}
						}
						ptr += len;
						break;

					case 0xE0:
					case 0xFE:
					case 0xDA:
					default:
						ptr++;
						len = (ptr[0] << 8L) | ptr[1];
						ptr += len;
						break;
					
				}
				bytesProcessed = (char *)ptr - JFIFHeader;
			}
			omOptFree(main, JFIFHeader);
		}
		
		for(n = 0; n < 2; n++)
		{
			if(pdata->compressionTables[n].AClen == 0)
			{
				memcpy(pdata->compressionTables[n].AC, STD_ACT, 178);
				pdata->compressionTables[n].AClen = 178;
			}
			
			if(pdata->compressionTables[n].DClen == 0)
			{
				memcpy(pdata->compressionTables[n].DC, STD_DCT, 28);
				pdata->compressionTables[n].DClen = 28;
			}
		}

#if OMFI_ENABLE_SEMCHECK
		saveCheck = omfsIsSemanticCheckOn(media->dataFile);
		omfsSemanticCheckOff(media->dataFile);
#endif
		indexSize = omfsLengthFrameIndex(media->dataFile, media->dataObj, pers->omJPEGFrameIndex);
		pdata->frameIndex = (omfUInt32 *) omOptMalloc(main, (size_t)indexSize * sizeof(omfInt32));
		if (pdata->frameIndex == NULL)
			RAISE(OM_ERR_NOMEMORY);

		pdata->maxIndex = indexSize;
		for(n = 1; n <= indexSize; n++)
		{
			CHECK(omfsGetNthFrameIndex(media->dataFile, media->dataObj, pers->omJPEGFrameIndex,
											&indexVal, n));
			pdata->frameIndex[n-1] = indexVal;
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
static omfErr_t omfmJPEGSetFrameNumber(omfCodecParms_t * info,
				                       omfMediaHdl_t media,
				                       omfHdl_t main)
{
	omfUInt32          frameNumber;
	userDataJPEG_t 		*pdata = (userDataJPEG_t *) media->userData;

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
static omfErr_t codecGetJPEGFieldInfo(omfCodecParms_t * info,
				                      omfMediaHdl_t media,
				                      omfHdl_t main,
				                      userDataJPEG_t * pdata)
{
	omfVideoMemOp_t 	*vparms;
	
	vparms = ((omfVideoMemOp_t *)info->spc.mediaInfo.buf);
		for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
	  {
		 switch(vparms->opcode)
			{
			case kOmfPixelFormat:
				vparms->operand.expPixelFormat = kOmfPixRGBA;		 /* JeffB: Was YUV */
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
				vparms->operand.expCompArray[0] = 'R';
				vparms->operand.expCompArray[1] = 'G';
				vparms->operand.expCompArray[2] = 'B';
				vparms->operand.expCompArray[3] = 0; /* terminating zero */
				break;
	
			case kOmfRGBCompSizes:
				vparms->operand.expCompSizeArray[0] = 8;
				vparms->operand.expCompSizeArray[1] = 8;
				vparms->operand.expCompSizeArray[2] = 8;
				vparms->operand.expCompSizeArray[3] = 0; /* terminating zero */
				break;
	
			}
	  }
	
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
static omfErr_t codecPutJPEGFieldInfo(omfCodecParms_t * info,
				                      omfMediaHdl_t media,
				                      omfHdl_t main,
				                      userDataJPEG_t * pdata)
{
	omfInt16     		numFields;
	omfVideoMemOp_t 	*vparms, oneVParm;
	omcAvJPEDPersistent_t *pers = (omcAvJPEDPersistent_t *)info->pers;
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
		  		if(pdata->bitsPerPixelAvg != 0 && media->pvt->pixType == kOmfPixRGBA)		/* defaults to RGB888 */
		 	 		pdata->memBytesPerSample = (pdata->fileBytesPerSample * 24L) / pdata->bitsPerPixelAvg;
				else
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

static omfErr_t codecCloseJPEG(omfCodecParms_t *info,
									omfMediaHdl_t media,
				                      omfHdl_t main)
{
	userDataJPEG_t *pdata = (userDataJPEG_t *)media->userData;

	XPROTECT(media->mainFile)
	{
		CHECK(omcCloseStream(media->stream));
		if((media->openType == kOmfiCreated) || (media->openType == kOmfiAppended))
		{
			CHECK(postWriteOps(info, media));
			if ( ! pdata->descriptorFlushed )
				CHECK(writeDescriptorData(info, media, media->mainFile, pdata));
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

	userDataJPEG_t *pdata;
	omfUInt32          frames;
	omfHdl_t        main = NULL;
	omcAvJPEDPersistent_t *pers;

	/* this routine will be called after sample data is written */

	omfAssertMediaHdl(media);
	main = media->mainFile;
	
	omfAssert(info, main, OM_ERR_NULL_PARAM);
 	pers = (omcAvJPEDPersistent_t *)info->pers;
 
	pdata = (userDataJPEG_t *) media->userData;
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
	 	/* fix the clip and track group lengths,they weren't known until now */
	
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
static omfErr_t omfmJPEGGetMaxSampleSize(omfCodecParms_t * info,
					                 omfMediaHdl_t media,
					                 omfHdl_t main,
				                     userDataJPEG_t * pdata,
					                 omfInt32 * sampleSize)
{
	omfInt32          *maxlen = (omfInt32 *) info->spc.mediaInfo.buf;
	omfUInt32          numSamples;
	omcAvJPEDPersistent_t *pers = (omcAvJPEDPersistent_t *)info->pers;

	XPROTECT(main)
	{	
		/*
		 * if data is compressed, and will not be software decompressed, find
		 * the largest frame by scanning the frame index.  This may take a
		 * while
		 */
	
		if (media->compEnable == kToolkitCompressionDisable)
		{
			omfUInt32            maxlen, i, thislen;

			CHECK(omfsTruncInt64toUInt32(media->channels[0].numSamples, &numSamples)); /* OK FRAMEOFFSET */
			for (i = 0, maxlen = 0; i < numSamples; i++)
			{
				thislen = pdata->frameIndex[i + 1] - pdata->frameIndex[i];
				maxlen = (maxlen > thislen) ? maxlen : thislen;
			}
			if(maxlen == 0)
				*sampleSize = pdata->memBytesPerSample;
			else
				*sampleSize = maxlen;
		} else
			*sampleSize = pdata->memBytesPerSample;
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

static omfErr_t setupStream(omfMediaHdl_t media, userDataJPEG_t * pdata)
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
		
		if((media->compEnable == kToolkitCompressionEnable) && (media->pvt->pixType == kOmfPixRGBA))
		{
			pdata->fileFmt[4].opcode = kOmfPixelFormat;
			pdata->fileFmt[4].operand.expPixelFormat = media->pvt->pixType;
			pdata->fileFmt[5].opcode = kOmfRGBCompLayout;
			pdata->fileFmt[5].operand.expCompArray[0] = 'R';
			pdata->fileFmt[5].operand.expCompArray[1] = 'G';
			pdata->fileFmt[5].operand.expCompArray[2] = 'B';
			pdata->fileFmt[5].operand.expCompArray[3] = 0;
			
			pdata->fileFmt[6].opcode = kOmfRGBCompSizes;
			pdata->fileFmt[6].operand.expCompSizeArray[0] = 8;
			pdata->fileFmt[6].operand.expCompSizeArray[1] = 8;
			pdata->fileFmt[6].operand.expCompSizeArray[2] = 8;
			pdata->fileFmt[6].operand.expCompSizeArray[3] = 0;
			pdata->fileFmt[7].opcode = kOmfVFmtEnd;
		}
		else
		{
			pdata->fileFmt[4].opcode = kOmfPixelFormat;
			pdata->fileFmt[4].operand.expPixelFormat = kOmfPixYUV;
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
static omfErr_t initUserData(userDataJPEG_t *pdata)
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
	pdata->rate.numerator = 0;
	pdata->rate.denominator = 0;

	pdata->fmtOps[0].opcode = kOmfVFmtEnd;
	pdata->imageAspectRatio.numerator = 0;
	pdata->imageAspectRatio.denominator = 0;
	pdata->fileLayout = kSeparateFields;
	pdata->descriptorFlushed = FALSE;
	pdata->componentWidth = 8;
	pdata->fileBytesPerSample = 0;
	pdata->currentIndex = 0;
	pdata->jpeg.JPEGTableID = 0;
	pdata->customTables = FALSE;
	pdata->fieldDominance = kNoDominant;
	pdata->videoLineMap[0] = 0;
	pdata->videoLineMap[1] = 0;
	pdata->frameIndex = NULL;
	pdata->maxIndex = 0;
	pdata->horizontalSubsampling = 1;
	pdata->colorSiting = kCoSiting;
	pdata->whiteLevel = 0;
	pdata->blackLevel = 0;
	pdata->colorRange = 0;
	pdata->bitsPerPixelAvg = 0;
	pdata->memBytesPerSample = 0;
	pdata->compressionTables[0].Qlen = 0;
	pdata->compressionTables[0].Q16len = 0;
	pdata->compressionTables[0].DClen = 0;
	pdata->compressionTables[0].AClen = 0;
	pdata->compressionTables[1].Qlen = 0;
	pdata->compressionTables[1].Q16len = 0;
	pdata->compressionTables[1].DClen = 0;
	pdata->compressionTables[1].AClen = 0;
	pdata->compressionTables[2].Qlen = 0;
	pdata->compressionTables[2].Q16len = 0;
	pdata->compressionTables[2].DClen = 0;
	pdata->compressionTables[2].AClen = 0;
		
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
static omfErr_t InitAvJPEGCodec(omfCodecParms_t *info)
	{
	omcAvJPEDPersistent_t	*persist;
	
	XPROTECT(NULL)
	{
		persist = (omcAvJPEDPersistent_t *)omOptMalloc(NULL, sizeof(omcAvJPEDPersistent_t));
		if(persist == NULL)
			RAISE(OM_ERR_NOMEMORY);
		
		omfsNewClass(info->spc.init.session, kClsRegistered, kOmfTstRevEither,
			"JPED", "CDCI");	/* This may fail if already registered */
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
						"FrameIndex", OMClassJPEG, 
					   OMDataValue, kPropRequired, &persist->omJPEGFrameIndex));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"ImageData", OMClassIDAT, 
					   OMDataValue, kPropRequired, &persist->omAvJPEDImageData));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRev1x,
						"QuantizationTables", "JPED", 
					   OMVarLenBytes, kPropRequired, &persist->omAvJPEDQTab));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRev2x,
						"QntznTbl", "JPED", 
					   OMDataValue, kPropRequired, &persist->omAvJPEDQTab2));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"DIDResolutionID", "DIDD",
					   OMInt32, kPropRequired, &persist->omAvJPEDResolutionID));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"DIDCompressMethod", "DIDD",
					   OMObjectTag, kPropRequired, &persist->omAvJPEDCompress));
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
static omfErr_t codecGetCompressionParmsJPEG(omfCodecParms_t * info,
					                     omfHdl_t main,
				                     userDataJPEG_t * pdata)

{
	omfJPEGInfo_t  *returnParms = (omfJPEGInfo_t *) info->spc.mediaInfo.buf;

	XPROTECT(main)
	{
		if (info->spc.mediaInfo.bufLen != sizeof(omfJPEGInfo_t))
			RAISE(OM_ERR_INTERN_TOO_SMALL);
	
		returnParms->JPEGTableID = pdata->jpeg.JPEGTableID;
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
omfErr_t        codecPutCompressionParmsJPEG(omfCodecParms_t * info,
					                omfMediaHdl_t media,
					                     omfHdl_t main,
				                     userDataJPEG_t * pdata)
{
	omfJPEGInfo_t  *parms = (omfJPEGInfo_t *) info->spc.mediaInfo.buf;

	XPROTECT(main)
	{
		if (info->spc.mediaInfo.bufLen != sizeof(omfJPEGInfo_t))
			RAISE(OM_ERR_INTERN_TOO_SMALL);
	
		pdata->jpeg.JPEGTableID = parms->JPEGTableID;
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
static omfErr_t codecSetJPEGTables(omfCodecParms_t * info,
				                   omfHdl_t main,
				                   userDataJPEG_t * pdata)
{
	omfJPEGTables_t *tables = (omfJPEGTables_t *) info->spc.mediaInfo.buf;
	omfInt32           compIndex, i;

	XPROTECT(main)
	{
		if (info->spc.mediaInfo.bufLen != sizeof(omfJPEGTables_t))
			RAISE(OM_ERR_INTERN_TOO_SMALL);
	
		if(tables->JPEGcomp == kJcLuminance || tables->JPEGcomp == kJcLuminanceFP16)
			compIndex = 0;
		else
			compIndex = 1;
		pdata->customTables = TRUE;
		pdata->compressionTables[compIndex].Qlen = tables->QTableSize;
		pdata->compressionTables[compIndex].Q16len = tables->QTableSize;
		pdata->compressionTables[compIndex].DClen = tables->DCTableSize;
		pdata->compressionTables[compIndex].AClen = tables->ACTableSize;
		for (i = 0; i < tables->QTableSize; i++)
		{
			pdata->compressionTables[compIndex].Q[i] = tables->QTables[i];
			pdata->compressionTables[compIndex].Q16[i] = tables->QTables[i];
		}
		for (i = 0; i < tables->ACTableSize; i++)
			pdata->compressionTables[compIndex].AC[i] = tables->ACTables[i];
		for (i = 0; i < tables->DCTableSize; i++)
			pdata->compressionTables[compIndex].DC[i] = tables->DCTables[i];
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
static omfErr_t codecGetJPEGTables(omfCodecParms_t * info,
				                   omfHdl_t main,
				                   userDataJPEG_t * pdata)
{
	omfJPEGTables_t *tables = (omfJPEGTables_t *) info->spc.mediaInfo.buf;
	omfInt32           compIndex, i;

	XPROTECT(main)
	{
		if (info->spc.mediaInfo.bufLen != sizeof(omfJPEGTables_t))
			RAISE(OM_ERR_INTERN_TOO_SMALL);
	
		if(tables->JPEGcomp == kJcLuminance || tables->JPEGcomp == kJcLuminanceFP16)
			compIndex = 0;
		else
			compIndex = 1;
		tables->QTableSize = pdata->compressionTables[compIndex].Qlen;
		tables->ACTableSize = pdata->compressionTables[compIndex].AClen;
		tables->DCTableSize = pdata->compressionTables[compIndex].DClen;
	
		tables->QTables = (omfUInt8 *) omOptMalloc(main, (size_t) tables->QTableSize);
		XASSERT(tables->QTables != NULL, OM_ERR_NOMEMORY);
	
		tables->ACTables = (omfUInt8 *) omOptMalloc(main, (size_t) tables->ACTableSize);
		XASSERT(tables->ACTables != NULL, OM_ERR_NOMEMORY);
	
		tables->DCTables = (omfUInt8 *) omOptMalloc(main, (size_t) tables->DCTableSize);
		XASSERT(tables->DCTables != NULL, OM_ERR_NOMEMORY);
	
		XASSERT((pdata->compressionTables[compIndex].Q || !tables->QTableSize), OM_ERR_BADQTABLE);
		XASSERT((pdata->compressionTables[compIndex].AC || !tables->ACTableSize), OM_ERR_BADACTABLE);
		XASSERT((pdata->compressionTables[compIndex].DC || !tables->DCTableSize), OM_ERR_BADDCTABLE);
	
		for (i = 0; i < tables->QTableSize; i++)
				(tables->QTables)[i] = pdata->compressionTables[compIndex].Q[i];
		for (i = 0; i < tables->ACTableSize; i++)
			(tables->ACTables)[i] = pdata->compressionTables[compIndex].AC[i];
		for (i = 0; i < tables->DCTableSize; i++)
			(tables->DCTables)[i] = pdata->compressionTables[compIndex].DC[i];
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

	/************************************************************
	 *
	 * Position32Array functions (for the frame index)
	 *
	 *************************************************************/

/************************
 * omfsGetNthFrameIndex
 * omfsAppendFrameIndex
 * omfsLengthFrameIndex
 *
 * 	Routines to handle FrameIndexes, which are used to hold lists of related
 *		objects or mobs.
 *
 * Argument Notes:
 *		i - Index is 1-based.
 *
 * ReturnValue:
 *		For omfsLengthFrameIndex:
 *			The length of the FrameIndex property, or 0 on error.
 *		Others:
 *			Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfErr_t omfsGetNthFrameIndex(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - and this property */
			omfUInt32		*data,	/* OUT - Get the data */
			omfInt32			i)			/* IN - at this index. */
{
	omfPosition_t	offset;
	omfErr_t		status;
	
	XPROTECT(file)
	{
#if OMFI_ENABLE_SEMCHECK
		XASSERT(i >= 1 && i <= omfsLengthFrameIndex(file, obj, prop), OM_ERR_BADINDEX)
#endif
		CHECK(OMGetNthPropHdr(file, obj, prop, i, OMPosition32Array,
										sizeof(omfUInt32), &offset));
		status = OMReadProp(file, obj, prop, offset, kSwabIfNeeded,
								OMPosition32Array, sizeof(omfUInt32), data);
		if(status != OM_ERR_NONE)
		{
			CHECK(OMReadProp(file, obj, prop, offset, kNeverSwab,
								OMDataValue, sizeof(omfUInt32), data));
			if(ompvtIsForeignByteOrder(file, obj))
				omfsFixLong((omfInt32 *) data);
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * omfsAppendFrameIndex
 */
static omfErr_t omfsAppendFrameIndex(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN - and this property */
			omfUInt32			data)	/* IN - append this value */
{
	omfInt32		length = 0;
	omfErr_t	status;
	omfPosition_t	offset;
	
	XPROTECT(file)
	{
		status = omfsGetArrayLength(file, obj, prop, OMPosition32Array, sizeof(omfUInt32), &length);
		if ((status == OM_ERR_NONE) || (status == OM_ERR_PROP_NOT_PRESENT))
		{
			CHECK(OMPutNthPropHdr(file, obj, prop, length+1, OMPosition32Array,
										sizeof(omfUInt32), &offset));
										
			CHECK(OMWriteProp(file, obj, prop, offset, OMPosition32Array,
										sizeof(omfUInt32), &data));
		}
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * omfsLengthFrameIndex
 */
static omfInt32 omfsLengthFrameIndex(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object, return the  */
			omfProperty_t	prop)	/* IN - # of of elements in this array */
{
	omfInt32          length = 0;

	XPROTECT(file)
	{
		CHECK(omfsGetArrayLength(file, obj, prop, OMPosition32Array,
									sizeof(omfUInt32), &length));
	}
	XEXCEPT
	{
		return(0);
	}
	XEND

	return (length);
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
static omfErr_t AddFrameOffset(omfMediaHdl_t media, omfUInt32 offset)
{
	omfInt32           mni;
	omfUInt32         i, ci;
	omfUInt32         *tmp_array;
	omfHdl_t        main;
	userDataJPEG_t *pdata;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	XPROTECT(main)
	{
		pdata = (userDataJPEG_t *) media->userData;
		ci = ++pdata->currentIndex;
	
		if (ci > pdata->maxIndex)
		{
	
			tmp_array = pdata->frameIndex;
			pdata->maxIndex += 1024;
			mni = pdata->maxIndex;
	
			pdata->frameIndex = (omfUInt32 *) omOptMalloc(main, (size_t) mni * sizeof(omfInt32));
			if (pdata->frameIndex == NULL)
				RAISE(OM_ERR_NOMEMORY);
	
			if(tmp_array != NULL)
			  {
				 for (i = 0; i < ci - 1; i++)	/* copy old storage into new */
				pdata->frameIndex[i] = tmp_array[i];
	
				omOptFree(main, (void *) tmp_array);
			  }
		}
		pdata->frameIndex[ci - 1] = offset;
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
static omfErr_t InitMDESProps(omfCodecParms_t * info,
								omfMediaHdl_t 			media,
								omfHdl_t				file)
{
	omfRational_t	aspect;
	char				lineMap[2] = { 0, 0 };
	omfObject_t		obj = info->spc.initMDES.mdes;
	omcAvJPEDPersistent_t *pers = (omcAvJPEDPersistent_t *)info->pers;
	
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
		CHECK(omfsWriteString(file, obj, OMDIDDCompression, "AJPG"));
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
				                      userDataJPEG_t *pdata)
{
	omfVideoMemOp_t 	*vparms;
	omfInt32			RGBFrom, memBitsPerPixel;
	omfInt32			fileFieldSize, memFieldSize, accumBytesPerSample;
	
	omfAssertMediaHdl(media);
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);
	omfAssert(pdata, file, OM_ERR_NULL_PARAM);
	
	XPROTECT(file)
	{

		/* validate opcodes individually.  Some don't apply */
		
		accumBytesPerSample = pdata->fileBytesPerSample;
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
			  case kOmfFieldDominance:
			  case kOmfStoredRect:
			  case kOmfAspectRatio:
			  case kOmfRGBCompLayout:
			  case kOmfLineLength:
			  		break;
			  		
			  case kOmfFrameLayout:		
			  		fileFieldSize = (pdata->fileLayout == kOneField ? 1 : 2);
			  		memFieldSize = (vparms->operand.expFrameLayout == kOneField ? 1 : 2);
			  		accumBytesPerSample = (accumBytesPerSample * memFieldSize) / fileFieldSize;
			  		break;

			  case kOmfRGBCompSizes:
			  		memBitsPerPixel = 0;
			  		RGBFrom = 0;
			  		while (vparms->operand.expCompSizeArray[RGBFrom])
			  		{
			  			memBitsPerPixel += vparms->operand.expCompSizeArray[RGBFrom];
			  			RGBFrom++;
			  		}
			  		if(pdata->bitsPerPixelAvg != 0)
			 	 		accumBytesPerSample = (accumBytesPerSample * memBitsPerPixel) / pdata->bitsPerPixelAvg;
			  	break;
			  	
			  default:
			  	RAISE(OM_ERR_ILLEGAL_MEMFMT);
			  }
		  }
		pdata->memBytesPerSample = 	accumBytesPerSample;

		CHECK(omfmVideoOpInit(file, pdata->fmtOps));
		CHECK(omfmVideoOpMerge(file, kOmfForceOverwrite, 
					(omfVideoMemOp_t *) parmblk->spc.mediaInfo.buf,
					pdata->fmtOps, MAX_FMT_OPS+1));
		CHECK(setupStream(media, pdata));
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
