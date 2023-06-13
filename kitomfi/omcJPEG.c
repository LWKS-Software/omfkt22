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
#include "omcJPEG.h" 
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
	omfProperty_t	omCDCIColorRange;
	omfProperty_t	omCDCIPaddingBits;
	
	omfProperty_t	omJPEGFrameIndex;
	omfProperty_t	omJPEGFrameIndexExt;
	omfProperty_t	omAvJPEDResolutionID;
	omfProperty_t	omAvJPEDCompressMeth;
	omfProperty_t	omAvJPEDFrameIndex;
	omfProperty_t	omAvJPEDIndexByteOrder;
}				omcJPEGPersistent_t;

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
	omfPosition_t      *frameIndex;
	omfUInt32          maxIndex;
	omfUInt32		horizontalSubsampling;
	omfColorSiting_t	colorSiting;
	omfUInt32			whiteLevel;
	omfUInt32			blackLevel;
	omfUInt32			colorRange;
	omfInt16				memBitsPerPixel;
	omfInt16				bitsPerPixelAvg;
	omfInt32				memBytesPerSample;
	omfInt16				padBits;				/* pad Bits per PIXEL */
}               userDataJPEG_t;

static omfErr_t codecSetJPEGTables(omfCodecParms_t * info, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecGetJPEGTables(omfCodecParms_t * info, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecGetCompressionParmsJPEG(omfCodecParms_t * info, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecPutCompressionParmsJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);

static omfErr_t InitJPEGCodec(omfCodecParms_t *info);
static omfErr_t codecOpenJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecCreateJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecGetJPEGFieldInfo(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecPutJPEGFieldInfo(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecGetRect(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecWriteSamplesJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecReadSamplesJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecSetRect(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t codecJPEGReadLines(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t omfmJPEGWriteLines(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataJPEG_t * pdata);
static omfErr_t setupStream(omfMediaHdl_t media, userDataJPEG_t * pdata);
static omfErr_t codecCloseJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t omfmJPEGSetFrameNumber(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t omfmJPEGGetFrameOffset(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
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
omfErr_t omfsGetNthFrameIndex(omfHdl_t file, omcJPEGPersistent_t *pers, 
								omfObject_t obj, omfLength_t *data, omfInt32 i);

omfErr_t omfsAppendFrameIndex(omfHdl_t file, omcJPEGPersistent_t *pers, 
								omfObject_t obj, omfPosition_t data);

omfInt32 omfsLengthFrameIndex( omfHdl_t file, omcJPEGPersistent_t *pers, omfObject_t obj);
static omfErr_t omfmJPEGGetMaxSampleSize(omfCodecParms_t * info,
					                 omfMediaHdl_t media,
					                 omfHdl_t main,
				                     userDataJPEG_t * pdata,
					                 omfInt32 * sampleSize);

static omfErr_t InitMDESProps(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t myCodecSetMemoryInfo(omfCodecParms_t * parmblk,
				                      omfMediaHdl_t media,
				                      omfHdl_t file,
				                      userDataJPEG_t * pdata);
static omfErr_t AddFrameOffset(omfMediaHdl_t media, omfPosition_t offset);
static omfErr_t codecPutInfoJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecGetInfoJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecSelectInfoJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecNumChannelsJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);

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

omfErr_t        omfCodecJPEG(omfCodecFunctions_t func, omfCodecParms_t * info)
{
	omfErr_t        	status;
	omcJPEGPersistent_t *pers = (omcJPEGPersistent_t *)info->pers;
	omfCodecOptDispPtr_t	*dispPtr;

	switch (func)
	{
	case kCodecInit:
		InitJPEGCodec(info);
		return (OM_ERR_NONE);
		break;

	case kCodecGetMetaInfo:
		strncpy((char *)info->spc.metaInfo.name, "JPEG Codec (Compressed ONLY)", info->spc.metaInfo.nameLen);
		info->spc.metaInfo.info.mdesClassID = "CDCI";
		info->spc.metaInfo.info.dataClassID = "JPEG";
		info->spc.metaInfo.info.codecID = CODEC_JPEG_VIDEO;
		info->spc.metaInfo.info.rev = CODEC_REV_3;
		info->spc.metaInfo.info.dataKindNameList = PICTUREKIND;
		info->spc.metaInfo.info.minFileRev = kOmfRev2x;
		info->spc.metaInfo.info.maxFileRevIsValid = FALSE;		/* There is no maximum rev */
		return (OM_ERR_NONE);
			
	case kCodecLoadFuncPointers:
		dispPtr = info->spc.loadFuncTbl.dispatchTbl;

		dispPtr[ kCodecOpen ] = codecOpenJPEG;
		dispPtr[ kCodecCreate ] = codecCreateJPEG;
		dispPtr[ kCodecGetInfo ] = codecGetInfoJPEG;
		dispPtr[ kCodecPutInfo ] = codecPutInfoJPEG;
		dispPtr[ kCodecReadSamples ] = codecReadSamplesJPEG;
		dispPtr[ kCodecWriteSamples ] = codecWriteSamplesJPEG;
		dispPtr[ kCodecClose ] = codecCloseJPEG;
		dispPtr[ kCodecSetFrame ] = omfmJPEGSetFrameNumber;
		dispPtr[ kCodecGetFrameOffset ] = omfmJPEGGetFrameOffset;
		dispPtr[ kCodecGetNumChannels ] = codecNumChannelsJPEG;
		dispPtr[ kCodecInitMDESProps ] = InitMDESProps;
		dispPtr[ kCodecGetSelectInfo ] = codecSelectInfoJPEG;
/*		dispPtr[ kCodecReadLines ] = NULL;
		dispPtr[ kCodecWriteLines ] = NULL;
		dispPtr[ kCodecImportRaw ] = NULL;
		dispPtr[ kCodecGetVarietyInfo ] = NULL;
		dispPtr[ kCodecSemanticCheck ] = NULL;
		dispPtr[ kCodecAddFrameIndexEntry ] = NULL;
 		dispPtr[ kCodecGetVarietyCount ] = NULL;
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
	
static omfErr_t codecPutInfoJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main)
{
	omfErr_t		status = OM_ERR_NONE;
	userDataJPEG_t * pdata;

	omfAssertMediaHdl(media);
	pdata = (userDataJPEG_t *) media->userData;
	
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

static omfErr_t codecGetInfoJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main)
{
	omfErr_t		status = OM_ERR_NONE;
	userDataJPEG_t * pdata;
	omfMaxSampleSize_t*ptr;
	omfFrameSizeParms_t	*singlePtr;
	omfLength_t			frameSize;
	omfUInt32			frameOff;

	omfAssertMediaHdl(media);
	pdata = (userDataJPEG_t *) media->userData;
	
	switch (info->spc.mediaInfo.infoType)
	{
	case kMediaIsContiguous:
		{
		status = OMIsPropertyContiguous(main, media->dataObj, OMIDATImageData,
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
			/* Fix when 4 billion frame count limit becomes a problem */
			status = omfsTruncInt64toUInt32(singlePtr->frameNum, &frameOff);
			if(status == OM_ERR_NONE)
			{
				frameOff--;		/* pdata->frameIndex is 0-based */
				if(pdata->frameIndex == NULL)
					status = OM_ERR_NOFRAMEINDEX;
				else if(frameOff < 0 || frameOff >= pdata->maxIndex)
					status = OM_ERR_BADFRAMEOFFSET;
				else
				{
					frameSize = pdata->frameIndex[frameOff + 1];
					status = omfsSubInt64fromInt64(pdata->frameIndex[frameOff], &frameSize);
					singlePtr->frameSize = frameSize;
				}
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

static omfErr_t codecNumChannelsJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main)
{
	omfErr_t				status = OM_ERR_NONE;
	omfDDefObj_t		dataKind;

	omfiDatakindLookup(info->spc.getChannels.file, PICTUREKIND, &dataKind, &status);
	if (info->spc.getChannels.mediaKind == dataKind)
		info->spc.getChannels.numCh = 1;
	else
		info->spc.getChannels.numCh = 0;
			
	return(status);
}

static omfErr_t codecSelectInfoJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main)
{
	omfErr_t		status = OM_ERR_NONE;
	char			compress[128];
	omfInt32		storedWidth, storedHeight;
	omfInt16		padBits;
	omfInt32		compWidth, bitsPerPixel;
	omfUInt32	subSampling;
	omfHdl_t		file;
	omcJPEGPersistent_t *pers = (omcJPEGPersistent_t *)info->pers;
	omfPosition_t	zeroPos;

	omfsCvtInt32toPosition(0, zeroPos);
	file = info->spc.selectInfo.file;
    XPROTECT(file)
    {
		if(omfsIsPropPresent(info->spc.selectInfo.file, info->spc.selectInfo.mdes, OMDIDDCompression, OMString))
		{
			status = omfsReadString(info->spc.selectInfo.file, info->spc.selectInfo.mdes, OMDIDDCompression, 
											compress, sizeof(compress));
			if(status != OM_ERR_NONE)
				return(status);
			info->spc.selectInfo.info.willHandleMDES = (strcmp(compress, "JPEG") == 0) || (strcmp(compress, "JFIF") == 0); 

		}
		else
			info->spc.selectInfo.info.willHandleMDES = FALSE;
				
		info->spc.selectInfo.info.isNative = TRUE;
		info->spc.selectInfo.info.hwAssisted = FALSE;
		info->spc.selectInfo.info.relativeLoss = 10;	/* !!! Need to read MDES header here */
		CHECK(omfsReadInt32(file, info->spc.selectInfo.mdes, OMDIDDStoredWidth, &storedWidth));
		CHECK(omfsReadInt32(file, info->spc.selectInfo.mdes, OMDIDDStoredHeight, &storedHeight));
		/***/
		CHECK(omfsReadInt32(main, info->spc.selectInfo.mdes, pers->omCDCIComponentWidth, &compWidth));
		status = OMReadProp(main, info->spc.selectInfo.mdes, pers->omCDCIPaddingBits,
							   zeroPos, kSwabIfNeeded, OMInt16,
							   sizeof(padBits), (void *)&padBits);
		if (status != OM_ERR_NONE)
			padBits = 0;
		CHECK(omfsReadUInt32(main, info->spc.selectInfo.mdes, pers->omCDCIHorizontalSubsampling,
										&subSampling));
						 
		switch (subSampling)
		{
		/* 4:4:4 = 1 sample for each of luma and two chroma channels */   
		case 1:
			bitsPerPixel = (compWidth * 3) + padBits;
			break;
			
		/* 4:2:2 = two-pixels get 2 samples of luma, 1 of each chroma channel, avg == 2 */							
		case 2:
			bitsPerPixel = (compWidth * 2) + padBits;					
			break;
		}
		/****/				
		info->spc.selectInfo.info.avgBitsPerSec = storedWidth * storedHeight * bitsPerPixel;
    }
    XEXCEPT
    XEND
	
	return(OM_ERR_NONE); 
}

static omfErr_t codecCreateJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main)
{
	userDataJPEG_t *pdata;
	omfLength_t		zero;

	omfAssertMediaHdl(media);
	pdata = (userDataJPEG_t *) media->userData;
	
	XPROTECT(main)
	{
		omfsCvtInt32toInt64(0, &zero);
		media->pvt->pixType	= kOmfPixYUV;
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
		else												// SPIDY
			pdata->rate = media->channels[0].sampleRate;

		
		/* number of frames is unknown until close time */
		omfsCvtInt32toInt64(0, &media->channels[0].numSamples);
	
		pdata->fmtOps[0].opcode = kOmfVFmtEnd;
		
		MakeRational(4, 3, &(pdata->imageAspectRatio));
		CHECK(AddFrameOffset(media, zero));
		CHECK(omcOpenStream(main, media->dataFile, media->stream, 
						media->dataObj, OMIDATImageData, OMDataValue));
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
static omfErr_t codecOpenJPEG(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main)
{
	userDataJPEG_t * pdata;

	omfAssertMediaHdl(media);
	pdata = (userDataJPEG_t *) media->userData;

	/* pull out descriptor information BEFORE reading JPEG data */
	XPROTECT(main)
	{
		media->pvt->pixType	= kOmfPixYUV;

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
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfmMultiXfer_t 	*xfer = NULL;
	omfPosition_t		offset;
	userDataJPEG_t		*pdata = NULL;
	omfInt32 			fieldLen;
	omfUInt32			n;
	omcJPEGPersistent_t *pers = (omcJPEGPersistent_t *)info->pers;
	JPEG_MediaParms_t	compressParms;
	omfCStrmSwab_t		src, dest;
	char				*tempBuf = NULL;
	omfUInt32			xlateBytesPerSample;
	
	omfAssertMediaHdl(media);
	
	XPROTECT(main)
	{
		compressParms.media = media;		
	
		pdata = (userDataJPEG_t	*)media->userData;
		XASSERT(pdata, OM_ERR_NULL_PARAM);
	
		XASSERT(pdata->componentWidth != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->fileBytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		xfer = info->spc.mediaXfer.xfer;
		XASSERT(xfer, OM_ERR_NULLBUF);
		
		/* this codec only allows one-channel media */
		XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
	
		if (media->compEnable == kToolkitCompressionEnable)
		{
			compressParms.blackLevel = pdata->blackLevel;
			compressParms.whiteLevel = pdata->whiteLevel;
			compressParms.colorRange = pdata->colorRange;
			compressParms.JPEGTableID= pdata->jpeg.JPEGTableID;

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
		 * OMF_EXT_CHECK_ERR_RETURN(main);
		 * omfErr_t omcSwabData(omfMediaHdl_t media,
		 * omfCStrmSwab_t *src, omfCStrmSwab_t *dest, omfBool swapBytes);
		 * OMF_EXT_CHECK_ERR_RETURN(main);
          */
			xfer->bytesXfered = 0;
			xfer->samplesXfered = 0;
			for(n = 0; n < xfer->numSamples; n++)
			{
/*!!!				compressParms.pixelBuffer = (char *)xfer->buffer + (n * pdata->memBytesPerSample);
				compressParms.pixelBufferIndex = 0;
				compressParms.pixelBufferLength = pdata->memBytesPerSample;
;--- */
				omcComputeBufferSize(media->stream, pdata->memBytesPerSample, omfComputeFileSize, &xlateBytesPerSample);
				src.fmt = media->stream->memFormat;
				src.buf = (char *)xfer->buffer +  (n * pdata->memBytesPerSample);
				src.buflen = pdata->memBytesPerSample;
				dest.fmt = media->stream->fileFormat;
				dest.buflen = xlateBytesPerSample;
				if(src.buflen * xfer->numSamples > xfer->buflen)
					RAISE(OM_ERR_SMALLBUF);

				if((omfUInt32)xlateBytesPerSample > src.buflen)
				{
					tempBuf = (char *)omOptMalloc(main, xlateBytesPerSample);
					if(tempBuf == NULL)
						RAISE(OM_ERR_NOMEMORY);
					dest.buf = tempBuf;
					dest.buflen = xlateBytesPerSample;
				}
				else
				{
					dest.buf = src.buf;
					tempBuf = NULL;
				}
				
				compressParms.pixelBuffer = (char *)dest.buf;
				compressParms.pixelBufferIndex = 0;
				compressParms.pixelBufferLength = xlateBytesPerSample;
				compressParms.isAvidJFIF = (pdata->jpeg.JPEGTableID != 0);

				CHECK(omcSwabData(media->stream, &src, &dest, media->stream->swapBytes));
	
				CHECK(omfmJFIFCompressSample(&compressParms, pdata->customTables, &fieldLen));
		
				xfer->bytesXfered += fieldLen;
				if (xfer->bytesXfered && (pdata->fileLayout == kSeparateFields))
				{
					CHECK(omfmJFIFCompressSample(&compressParms, pdata->customTables, &fieldLen));
					xfer->bytesXfered += fieldLen;
				}
				if(tempBuf != NULL)
				{
					omOptFree(main, tempBuf);
					tempBuf = NULL;
				}
				xfer->samplesXfered++;
				/* Don't forget to update the global frame counter */
				CHECK(omfsAddInt32toInt64(1, &media->channels[0].numSamples));
				CHECK(omcGetStreamPosition(media->stream, &offset));
/*!!!				if(pdata->jpeg.JPEGTableID != 0)
				{
					omfPosition_t	dataStartPos;
					
					CHECK(omfmGetMediaSegmentInfo(media, 1, &dataStartPos, NULL, NULL, NULL));
					CHECK(omfsAddInt64toInt64(dataStartPos, &offset));
				}
*/				
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
			CHECK(omcGetStreamPosition(media->stream, &offset));
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
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	userDataJPEG_t 	*pdata = NULL;
	omfUInt32			fileBytes = 0;
	omfUInt32			bytesRead = 0;
	omfmMultiXfer_t	*xfer = NULL;
	omfUInt32			rBytes, nBytes32;
	omfPosition_t		startPos;
	omfLength_t			nBytes;
	omcJPEGPersistent_t *pers = (omcJPEGPersistent_t *)info->pers;
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
		memset (&compressParms, 0, sizeof(compressParms)); /* fixes BoundsChecker errors */
		compressParms.media = media;
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
				frameIndexLen = omfsLengthFrameIndex(main, pers, media->dataObj);
				if ((pdata->currentIndex+1) >= frameIndexLen)
					RAISE(OM_ERR_BADFRAMEOFFSET);
		
				nBytes = pdata->frameIndex[pdata->currentIndex + 1];
				omfsSubInt64fromInt64(pdata->frameIndex[pdata->currentIndex], &nBytes);
				startPos = pdata->frameIndex[pdata->currentIndex];
				
				/* skip the IFH */
				CHECK(omcSeekStreamTo(media->stream, startPos));	
				pdata->currentIndex++;
	
				src.fmt = media->stream->fileFormat;
				src.buflen = pdata->fileBytesPerSample;
				dest.fmt = media->stream->memFormat;
				dest.buf = (char *)xfer->buffer + (n * pdata->memBytesPerSample);
				dest.buflen = pdata->memBytesPerSample;
				if((dest.buflen * xfer->numSamples) > xfer->buflen)
					RAISE(OM_ERR_SMALLBUF);

				if(media->pvt->pixType == kOmfPixRGBA)
					decompBytesPerSample = 	(omfInt32)pdata->imageWidth *
										(omfInt32) pdata->imageLength *
										(omfInt32) 3 * 	/*!!!*/
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
	
				compressOkay = (omfmJFIFDecompressSample(&compressParms) == OM_ERR_NONE);
				if (!compressOkay)
					RAISE(OM_ERR_DECOMPRESS);
					
				/* copyout JPEGTableID assuming it was set */
				pdata->jpeg.JPEGTableID = compressParms.JPEGTableID;
	
				if (compressOkay && pdata->fileLayout == kSeparateFields)
				{
					omfUInt8           data;
					omfPosition_t      saveOffset;

					CHECK(omcGetStreamPosition(media->stream, &saveOffset));
	
					/*
					 * look for a restart marker separating the
					 * two fields
					 */
					CHECK(omcSeekStreamRelative(media->stream, -2));
					CHECK(omcReadStream(media->stream, 1, &data, NULL));
	
					while (data == 0xFF)
					{
						CHECK(omcReadStream(media->stream, 1, &data, NULL));
					}
					if ((0xD0 <= data && data <= 0xD7) || (data == 0xD9))	/* skip restart marker */
					{
						CHECK(omcGetStreamPosition(media->stream, &saveOffset));
					}
					CHECK(omcSeekStreamTo(media->stream, saveOffset));
					
					compressOkay = (omfmJFIFDecompressSample(&compressParms) == OM_ERR_NONE);
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
	
			nBytes = pdata->frameIndex[pdata->currentIndex + 1];
			omfsSubInt64fromInt64(pdata->frameIndex[pdata->currentIndex], &nBytes);
			CHECK(omfsTruncInt64toUInt32(nBytes, &nBytes32));	/* OK FRAMESIZE */
			startPos = pdata->frameIndex[pdata->currentIndex];
			if(nBytes32 > xfer->buflen)
				RAISE(OM_ERR_SMALLBUF);
			
			/* skip the IFH */
			CHECK(omcSeekStreamTo(media->stream, startPos));	
			pdata->currentIndex++;

			status = omcReadStream(media->stream,
				nBytes32, xfer->buffer, &rBytes);
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
	omcJPEGPersistent_t *pers;
	omfInt32				n;
	
	/* this routine will be called after sample data is written */

	omfAssertMediaHdl(media);
	
	omfAssert(info, main, OM_ERR_NULL_PARAM);
 	pers = (omcJPEGPersistent_t *)info->pers;
 
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

		CHECK(OMWriteBaseProp(main, media->mdes, pers->omCDCIColorSiting,
							   OMColorSitingType, sizeof(pdata->colorSiting),
							    (void *)&pdata->colorSiting));
							    
		CHECK(omfsWriteUInt32(main, media->mdes, pers->omCDCIBlackReferenceLevel,
										pdata->blackLevel));

		CHECK(omfsWriteUInt32(main, media->mdes, pers->omCDCIWhiteReferenceLevel,
										pdata->whiteLevel));

		CHECK(omfsWriteUInt32(main, media->mdes, pers->omCDCIColorRange,
										pdata->colorRange));

		CHECK(omfsWriteInt16(main, media->mdes, pers->omCDCIPaddingBits,
										pdata->padBits));

		for(n = 1; n <= pdata->currentIndex; n++)
		{
			CHECK(omfsAppendFrameIndex(main, pers, media->dataObj, pdata->frameIndex[n-1]));
		}

		if(pdata->jpeg.JPEGTableID != 0)
		{
			omfPosition_t	offset, dataOffset;
			omfLength_t		length;
			omfErr_t		status;
			
			CHECK(omfsWriteInt32(main, media->mdes, pers->omAvJPEDResolutionID, pdata->jpeg.JPEGTableID));

			CHECK(omfsWriteString(main, media->mdes, pers->omAvJPEDCompressMeth, "JFIF"));
			CHECK(omfsWriteString(main, media->mdes, OMDIDDCompression, "JFIF"));
			CHECK(omfsWriteString(main, media->mdes, OMDIDDCompression, "JFIF"));
			
			status = OMGetPropertySegmentInfo(main, media->dataObj, pers->omJPEGFrameIndex,
											OMPosition32Array, 1, &offset, &length);
			if(status != OM_ERR_NONE)
			{
				CHECK(OMGetPropertySegmentInfo(main, media->dataObj, pers->omJPEGFrameIndexExt,
											OMPosition64Array, 1, &offset, &length));
			}
			CHECK(OMGetPropertySegmentInfo(main, media->dataObj, OMIDATImageData,
											OMDataValue, 1, &dataOffset, &length));
			CHECK(omfsAddInt32toInt64(2, &offset));
			CHECK(omfsSubInt64fromInt64(dataOffset, &offset));
			CHECK (OMWriteBaseProp(main, media->mdes, pers->omAvJPEDFrameIndex, OMInt64,sizeof(omfPosition_t), 
				  &offset));
		CHECK(omfsWriteInt16(main, media->mdes, pers->omAvJPEDIndexByteOrder,
										main->byteOrder));
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
	omfUInt32				indexSize, n;
	omfPosition_t			indexVal;
	omcJPEGPersistent_t *pers = NULL;
	omfPosition_t			zeroPos;
	omfErr_t					status;
	
	omfsCvtInt32toPosition(0, zeroPos);
	omfAssertMediaHdl(media);
	omfAssert(info, main, OM_ERR_NULL_PARAM);
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
		pers = (omcJPEGPersistent_t *)info->pers;
		pdata->frameIndex = NULL;
	 
		if (info->fileRev == kOmfRev2x)
		{
	 		CHECK(omfsReadRational(main, media->mdes, OMMDFLSampleRate, 
								   &(pdata->rate)));
	 	}
	 	else
	 	{
	 		CHECK(omfsReadExactEditRate(main, media->mdes, OMMDFLSampleRate, 
										&(pdata->rate)));
		} 		
		CHECK(omfsReadLength(main, media->mdes, OMMDFLLength, 
							 &media->channels[0].numSamples));
	
		media->numChannels = 1;
	
		CHECK(omcOpenStream(main, media->dataFile, media->stream, 
						media->dataObj, OMIDATImageData, OMDataValue));
			
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
				pdata->bitsPerPixelAvg = (omfInt16)((pdata->componentWidth * 3) + pdata->padBits);
				bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
	  									(pdata->componentWidth * 3) * numFields;
	  		}							
	  		/* 4:2:2 = two-pixels get 2 samples of luma, 1 of each chroma channel, avg == 2 */							
	  		else if (pdata->horizontalSubsampling == 2)						
			{
				pdata->bitsPerPixelAvg = (omfInt16)((pdata->componentWidth * 2) + pdata->padBits);
				bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
	  									(pdata->componentWidth * 2) * numFields;
	  		}							
	  		else
	  			RAISE(OM_ERR_ILLEGAL_FILEFMT);
	  									
			pdata->fileBytesPerSample = (bitsPerSample + 7) / 8;
			pdata->memBytesPerSample = pdata->fileBytesPerSample;
		}
		
		status = OMReadProp(main, media->mdes, pers->omCDCIPaddingBits,
							   zeroPos, kSwabIfNeeded, OMInt16,
							   sizeof(pdata->padBits), (void *)&(pdata->padBits));
		if (status != OM_ERR_NONE)
			pdata->padBits = 0;
		

		status = OMReadProp(main, media->mdes, pers->omCDCIColorSiting,
							   zeroPos, kSwabIfNeeded, OMColorSitingType,
							   sizeof(pdata->colorSiting), (void *)&(pdata->colorSiting));
		if (status != OM_ERR_NONE)
			pdata->colorSiting = kCoSiting;
							    

		status = omfsReadUInt32(main, media->mdes, pers->omCDCIBlackReferenceLevel,
										&(pdata->blackLevel));
		if (status != OM_ERR_NONE)
			pdata->blackLevel = 0;

		status = omfsReadUInt32(main, media->mdes, pers->omCDCIWhiteReferenceLevel,
										&(pdata->whiteLevel));
		if (status != OM_ERR_NONE)
			pdata->whiteLevel = powi(2, pdata->componentWidth) -1;

		status = omfsReadUInt32(main, media->mdes, pers->omCDCIColorRange,
										&(pdata->colorRange));
		if (status != OM_ERR_NONE)
			pdata->colorRange = powi(2, pdata->componentWidth) -2;			

		status = omfsReadInt32(main, media->mdes, pers->omAvJPEDResolutionID, &pdata->jpeg.JPEGTableID);
		if (status != OM_ERR_NONE)
			pdata->jpeg.JPEGTableID = 0;			

		indexSize = omfsLengthFrameIndex(media->dataFile, pers, media->dataObj);
		pdata->frameIndex = (omfPosition_t *) omOptMalloc(main, (size_t)indexSize * sizeof(omfPosition_t));
		if (pdata->frameIndex == NULL)
			RAISE(OM_ERR_NOMEMORY);

		pdata->maxIndex = indexSize;
		for(n = 1; n <= indexSize; n++)
		{
			CHECK(omfsGetNthFrameIndex(media->dataFile, pers, media->dataObj,
											&indexVal, n));
			pdata->frameIndex[n-1] = indexVal;
		}
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
static omfErr_t omfmJPEGSetFrameNumber(omfCodecParms_t * info,
				                       omfMediaHdl_t media,
				                       omfHdl_t main)
{
	omfUInt32         frameNumber;
	userDataJPEG_t 	*pdata = (userDataJPEG_t *) media->userData;

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
 * omfmJPEGGetFrameOffset
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
static omfErr_t omfmJPEGGetFrameOffset(omfCodecParms_t * info,
				                       omfMediaHdl_t media,
				                       omfHdl_t main)
{
	omfUInt32         frameNumber;
	/* omfInt64          frameOffset; */
	userDataJPEG_t 	*pdata = (userDataJPEG_t *) media->userData;

	omfAssert(media->dataObj, main, OM_ERR_INTERNAL_MDO);
	XPROTECT(main)
	{
		CHECK(omfsTruncInt64toUInt32(info->spc.getFrameOffset.frameNumber, &frameNumber)); /* OK FRAMEOFFSET */
		XASSERT(frameNumber != 0, OM_ERR_BADFRAMEOFFSET);

		XASSERT(pdata->frameIndex != NULL, OM_ERR_NOFRAMEINDEX);
		if ((omfUInt32)pdata->currentIndex >= pdata->maxIndex)
			RAISE(OM_ERR_BADFRAMEOFFSET);

		info->spc.getFrameOffset.frameOffset = pdata->frameIndex[frameNumber - 1];
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
			  vparms->operand.expPixelFormat = kOmfPixYUV;
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
			  vparms->operand.expInt16 = pdata->bitsPerPixelAvg;
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
			case kOmfCDCIPadBits:
				vparms->operand.expInt16 = pdata->padBits;
				break;

			case kOmfCDCIHorizSubsampling:
				vparms->operand.expUInt32 = pdata->horizontalSubsampling;
				break;
			case 	kOmfCDCIColorSiting:    /* operand.expColorSiting */
				vparms->operand.expColorSiting = pdata->colorSiting;
				break;
				
			case 	kOmfCDCIBlackLevel:     /* operand.expUInt32 */
				vparms->operand.expUInt32 = pdata->blackLevel;
				break;
				
			case 	kOmfCDCIWhiteLevel:     /* operand.expUInt32 */
				vparms->operand.expUInt32 = pdata->whiteLevel;
				break;
				
			case 	kOmfCDCIColorRange:     /* operand.expUInt32 */
				vparms->operand.expUInt32 = pdata->colorRange;
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
	omcJPEGPersistent_t *pers = (omcJPEGPersistent_t *)info->pers;
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
								
			case kOmfCDCIPadBits:
				pdata->padBits = vparms->operand.expInt16;
				/* JeffB June/1997: Essentially the same as Rogers comment below.  fileBytesPerSample
					Depends on componentWidth, horizontalSubsampling, and now... padBits...
				 */
				
				if (pdata->fileBytesPerSample)
				{
					pdata->bitsPerPixelAvg = (omfInt16)((pdata->componentWidth * 3) + pdata->padBits);
		  			/* compute pixel size and put in the file fmt */
		  			oneVParm.opcode = kOmfPixelSize;
		  			oneVParm.operand.expInt16 = pdata->bitsPerPixelAvg;
		  			CHECK(omfmVideoOpAppend(main, kOmfForceOverwrite, oneVParm, pdata->fileFmt,
		  											MAX_FMT_OPS+1))
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
						pdata->bitsPerPixelAvg = (omfInt16)((pdata->componentWidth * 3) + pdata->padBits);
						bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
			  									pdata->bitsPerPixelAvg * numFields;
			  									
			  			/* compute pixel size and put in the file fmt */
			  			oneVParm.opcode = kOmfPixelSize;
			  			oneVParm.operand.expInt16 = pdata->bitsPerPixelAvg;
			  			CHECK(omfmVideoOpAppend(main, kOmfForceOverwrite, oneVParm, pdata->fileFmt,
			  											MAX_FMT_OPS+1))
			  		}
			  		/* 4:2:2 = two-pixels get 2 samples of luma, 1 of each chroma channel, avg == 2 */							
			  		else if (pdata->horizontalSubsampling == 2)						
					{
						pdata->bitsPerPixelAvg = (omfInt16)((pdata->componentWidth * 2) + pdata->padBits);
						bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
			  									pdata->bitsPerPixelAvg * numFields;
			  									
			  			/* compute pixel size and put in the file fmt */
			  			oneVParm.opcode = kOmfPixelSize;
			  			oneVParm.operand.expInt16 = pdata->bitsPerPixelAvg;
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
					pdata->bitsPerPixelAvg = (omfInt16)((pdata->componentWidth * 3) + pdata->padBits);
					bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
		  									pdata->bitsPerPixelAvg * numFields;
		  									
		  			/* compute pixel size and put in the file fmt */
		  			oneVParm.opcode = kOmfPixelSize;
		  			oneVParm.operand.expInt16 = pdata->bitsPerPixelAvg;
		  			CHECK(omfmVideoOpAppend(main, kOmfForceOverwrite, oneVParm, pdata->fileFmt,
		  											MAX_FMT_OPS+1))
		  		}
		  		/* 4:2:2 = two-pixels get 2 samples of luma, 1 of each chroma channel, avg == 2 */							
		  		else if (pdata->horizontalSubsampling == 2)						
				{
					pdata->bitsPerPixelAvg = (omfInt16)((pdata->componentWidth * 2) + pdata->padBits);
					bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
		  									pdata->bitsPerPixelAvg * numFields;
		  									
		  			/* compute pixel size and put in the file fmt */
		  			oneVParm.opcode = kOmfPixelSize;
		  			oneVParm.operand.expInt16 = pdata->bitsPerPixelAvg;
		  			CHECK(omfmVideoOpAppend(main, kOmfForceOverwrite, oneVParm, pdata->fileFmt,
		  											MAX_FMT_OPS+1))
		  		}										  									
		  		else
		  			RAISE(OM_ERR_ILLEGAL_FILEFMT);
		  									
				pdata->fileBytesPerSample = (bitsPerSample + 7) / 8;
				pdata->memBytesPerSample = pdata->fileBytesPerSample;
		  		if(pdata->bitsPerPixelAvg != 0 && pdata->memBitsPerPixel != 0)
		  			pdata->memBytesPerSample = (pdata->fileBytesPerSample * pdata->memBitsPerPixel) / pdata->bitsPerPixelAvg;
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
										omfMediaHdl_t 		media,
										omfHdl_t				main)
{
	userDataJPEG_t *pdata = (userDataJPEG_t *)media->userData;

	XPROTECT(main)
	{
		CHECK(omcCloseStream(media->stream));
		if((media->openType == kOmfiCreated) || (media->openType == kOmfiAppended))
		{
			CHECK(postWriteOps(info, media));
			if(main->fmt == kOmfiMedia) // SPIDY
			{
				if ( ! pdata->descriptorFlushed )
					CHECK(writeDescriptorData(info, media, main, pdata));
			}
		}
		
	  	if(pdata->frameIndex != NULL)
			omOptFree(main, pdata->frameIndex);
		omOptFree(main, media->userData);
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
	omfHdl_t        main = NULL;
	omcJPEGPersistent_t *pers;

	/* this routine will be called after sample data is written */

	omfAssertMediaHdl(media);
	main = media->mainFile;
	
	omfAssert(info, main, OM_ERR_NULL_PARAM);
 	pers = (omcJPEGPersistent_t *)info->pers;
 
	pdata = (userDataJPEG_t *) media->userData;
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
	 	/* fix the clip and track group lengths,they weren't known until now */
		if(main->fmt == kOmfiMedia) // SPIDY
		{
			XASSERT(media->fileMob != NULL, OM_ERR_INTERNAL_CORRUPTVINFO);
			XASSERT(media->mdes != NULL, OM_ERR_INTERNAL_CORRUPTVINFO);
				
	 		/* patch descriptor length */
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
	omcJPEGPersistent_t *pers = (omcJPEGPersistent_t *)info->pers;

	XPROTECT(main)
	{	
		/*
		 * if data is compressed, and will not be software decompressed, find
		 * the largest frame by scanning the frame index.  This may take a
		 * while
		 */
	
		if (media->compEnable == kToolkitCompressionDisable)
		{
			omfUInt32         maxlen, i, thisLen32;
			omfLength_t			thisLen;

			CHECK(omfsTruncInt64toUInt32(media->channels[0].numSamples, &numSamples)); /* OK FRAMEOFFSET */
			for (i = 0, maxlen = 0; i < numSamples; i++)
			{
				thisLen = pdata->frameIndex[i + 1];
				CHECK(omfsSubInt64fromInt64(pdata->frameIndex[i], &thisLen));
				CHECK(omfsTruncInt64toUInt32(thisLen, &thisLen32));	/* OK FRAMESIZE */
				maxlen = (maxlen > thisLen32) ? maxlen : thisLen32;
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
		
		pdata->fileFmt[1].opcode = kOmfVideoLineMap;
		pdata->fileFmt[1].operand.expLineMap[0] = pdata->videoLineMap[0];
		pdata->fileFmt[1].operand.expLineMap[1] = pdata->videoLineMap[1];

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
	pdata->padBits = 0;
	pdata->memBitsPerPixel = 0;
		
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
static omfErr_t InitJPEGCodec(omfCodecParms_t *info)
	{
	omcJPEGPersistent_t	*persist;
	
	XPROTECT(NULL)
	{
		persist = (omcJPEGPersistent_t *)omOptMalloc(NULL, sizeof(omcJPEGPersistent_t));
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
						"ColorRange", OMClassCDCI, 
					   OMUInt32, kPropOptional, &persist->omCDCIColorRange));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"FrameIndex", OMClassJPEG, 
					   OMPosition32Array, kPropRequired, &persist->omJPEGFrameIndex));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"FrameIndexExt", OMClassJPEG, 
					   OMPosition64Array, kPropRequired, &persist->omJPEGFrameIndexExt));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"PaddingBits", OMClassCDCI, 
					   OMInt16, kPropOptional, &persist->omCDCIPaddingBits));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"DIDResolutionID", "DIDD",
					   OMInt32, kPropOptional, &persist->omAvJPEDResolutionID));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"DIDCompressMethod", "DIDD",
					   OMInt32, kPropOptional, &persist->omAvJPEDCompressMeth));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"OffsetToFrameIndexes", "JPED",
					   OMInt64, kPropOptional, &persist->omAvJPEDFrameIndex));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"FrameIndexByteOrder", "DIDD",
					   OMInt16, kPropOptional, &persist->omAvJPEDIndexByteOrder));
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
	
		compIndex = (tables->JPEGcomp == kJcLuminance ? 0 : 1);
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
		/* (SPR#344) S/B Getting Q-tables from the current JFIF frame */
		if (info->spc.mediaInfo.bufLen != sizeof(omfJPEGTables_t))
			RAISE(OM_ERR_INTERN_TOO_SMALL);
	
		compIndex = (tables->JPEGcomp == kJcLuminance ? 0 : 1);
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
omfErr_t omfsGetNthFrameIndex(
			omfHdl_t			file,		/* IN - From this file */
			omcJPEGPersistent_t *pers,
			omfObject_t		obj,		/* IN - and this object */
			omfLength_t		*data,	/* OUT - Get the data */
			omfInt32			i)			/* IN - at this index. */
{
	omfPosition_t	offset;
	omfInt32			baseIndexLen;
	omfInt32			len;
	
	XPROTECT(file)
	{
#if OMFI_ENABLE_SEMCHECK
		XASSERT(i >= 1 && i <= omfsLengthFrameIndex(file, pers, obj), OM_ERR_BADINDEX)
#endif
		CHECK(omfsGetArrayLength(file, obj, pers->omJPEGFrameIndex,
										OMPosition32Array, sizeof(omfUInt32), &baseIndexLen));
		if(i <= baseIndexLen)
		{
		
			CHECK(OMGetNthPropHdr(file, obj, pers->omJPEGFrameIndex, i,
										OMPosition32Array,
										sizeof(omfUInt32), &offset));
			CHECK(OMReadProp(file, obj, pers->omJPEGFrameIndex, offset, kSwabIfNeeded,
								OMPosition32Array, sizeof(omfUInt32), &len));
			CHECK(omfsCvtUInt32toInt64(len, data));
		}
		else
		{
			CHECK(OMGetNthPropHdr(file, obj, pers->omJPEGFrameIndexExt, i,
										OMPosition64Array, sizeof(omfInt64), &offset));
			CHECK(OMReadProp(file, obj, pers->omJPEGFrameIndexExt, offset,
										kSwabIfNeeded, OMPosition64Array, sizeof(omfInt64),
										data));
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
omfErr_t omfsAppendFrameIndex(
			omfHdl_t				file,	/* IN - From this file */
			omcJPEGPersistent_t *pers,
			omfObject_t			obj,	/* IN - and this object */
			omfPosition_t		data)	/* IN - append this value */
{
	omfInt32			length = 0;
	omfErr_t			status;
	omfPosition_t	offset;
	omfInt32			upper, lower;
	
	XPROTECT(file)
	{
		CHECK(omfsDecomposeInt64(data, &upper, &lower));
		if(upper == 0)
		{
			status = omfsGetArrayLength(file, obj, pers->omJPEGFrameIndex, OMPosition32Array,
												sizeof(omfUInt32), &length);
			if ((status == OM_ERR_NONE) || (status == OM_ERR_PROP_NOT_PRESENT))
			{
				CHECK(OMPutNthPropHdr(file, obj, pers->omJPEGFrameIndex, length+1,
											OMPosition32Array,
											sizeof(omfUInt32), &offset));
											
				CHECK(OMWriteProp(file, obj, pers->omJPEGFrameIndex, offset, OMPosition32Array,
											sizeof(omfUInt32), (omfUInt32 *)&lower));
			}
		}
		else		/* Frame offset has passed 32-bits */
		{
			status = omfsGetArrayLength(file, obj, pers->omJPEGFrameIndexExt, OMPosition64Array,
											sizeof(omfInt64), &length);
			if ((status == OM_ERR_NONE) || (status == OM_ERR_PROP_NOT_PRESENT))
			{
				CHECK(OMPutNthPropHdr(file, obj, pers->omJPEGFrameIndexExt, length+1,
											OMPosition64Array,
											sizeof(omfInt64), &offset));
											
				CHECK(OMWriteProp(file, obj, pers->omJPEGFrameIndexExt,
											offset, OMPosition64Array,
											sizeof(omfInt64), &data));
			}
		}
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * omfsLengthFrameIndex
 */
omfInt32 omfsLengthFrameIndex(
			omfHdl_t			file,	/* IN - From this file */
			omcJPEGPersistent_t *pers,
			omfObject_t		obj)	/* IN - # of of elements in this array */
{
	omfInt32           length1 = 0, length2 = 0;

	XPROTECT(file)
	{
		CHECK(omfsGetArrayLength(file, obj, pers->omJPEGFrameIndex,
										OMPosition32Array, sizeof(omfUInt32), &length1));
		CHECK(omfsGetArrayLength(file, obj, pers->omJPEGFrameIndexExt,
										OMPosition64Array, sizeof(omfInt64), &length2));
	}
	XEXCEPT
	{
		return(0);
	}
	XEND

	return (length1+length2);
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
static omfErr_t AddFrameOffset(omfMediaHdl_t media, omfPosition_t offset)
{
	omfInt32           mni;
	omfUInt32         i, ci;
	omfPosition_t     *tmp_array;
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
	
			pdata->frameIndex = (omfPosition_t *) omOptMalloc(main, (size_t) mni * sizeof(omfPosition_t));
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
	omcJPEGPersistent_t *pers = (omcJPEGPersistent_t *)info->pers;
	omfObject_t		obj = info->spc.initMDES.mdes;
	
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
		CHECK(omfsWriteString(file, obj, OMDIDDCompression, "JPEG"));
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
				                      userDataJPEG_t * pdata)
{
	omfVideoMemOp_t 	*vparms;
	omfInt32			RGBFrom;
	omfInt32			fileFieldSize, memFieldSize, accumBytesPerSample;

	omfAssertMediaHdl(media);
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);
	omfAssert(pdata, file, OM_ERR_NULL_PARAM);

	pdata = (userDataJPEG_t *) media->userData;
	
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
			  		pdata->memBitsPerPixel = 0;
			  		RGBFrom = 0;
			  		while (vparms->operand.expCompSizeArray[RGBFrom])
			  		{
			  			pdata->memBitsPerPixel += vparms->operand.expCompSizeArray[RGBFrom];
			  			RGBFrom++;
			  		}
			  		if(pdata->bitsPerPixelAvg != 0)
			  			accumBytesPerSample = (accumBytesPerSample * pdata->memBitsPerPixel) / pdata->bitsPerPixelAvg;
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
