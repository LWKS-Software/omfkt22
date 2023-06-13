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
#include "omcCDCI.h"  
#include "omPvt.h"
#include "omcStd.h"

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
	omfProperty_t	omCDCIVerticalSubsampling;
	omfProperty_t	omAvJPEDResolutionID;
	omfProperty_t	omAvJPEDCompress;
	omfProperty_t	omAvDIDDFirstFrameOffset;
}				omcCDCIPersistent_t;

typedef struct
{
	omfInt32         imageWidth;
	omfInt32         imageLength;
	omfInt32			sampledWidth;
	omfInt32			sampledLength;
	omfInt32			sampledXOffset;
	omfInt32			sampledYOffset;
	omfInt32			displayWidth;
	omfInt32			displayLength;
	omfInt32			displayXOffset;
	omfInt32			displayYOffset;
	omfRational_t   	rate;
	omfInt32          fileBytesPerSample;
	omfFrameLayout_t 	fileLayout;	/* Frame of fields per sample */
	omfRational_t		imageAspectRatio;
	omfVideoMemOp_t	fmtOps[MAX_FMT_OPS+1];
	omfVideoMemOp_t	fileFmt[MAX_FMT_OPS+1];
	omfBool				descriptorFlushed;
	omfBool				dataWritten;
	omfInt32				alignment;
	omfInt32				saveAlignment;
	char 					*alignmentBuffer;
	omfInt32          numLines;
	omfFieldDom_t		fieldDominance;
	omfInt32				videoLineMap[2];
	omfInt32				componentWidth;
	/* RPS 3/28/96 added missing CDCI attributes */
	omfUInt32			horizontalSubsampling;
	omfUInt32			verticalSubsampling;
	omfColorSiting_t	colorSiting;
	omfUInt32			whiteLevel;
	omfUInt32			blackLevel;
	omfUInt32			colorRange;
	omfInt16				bitsPerPixelAvg;
	omfInt32				memBytesPerSample;
	omfInt16				padBits;				/* Bits per PIXEL */
	omfBool				avidUncompressed;
	omfInt32			clientFillStart;			/* Bytes at the start of every frame (non-pixels) */
	omfInt32			clientFillEnd;				/* Bytes at the end   of every frame (non-pixels) */
	omfCDCIInfo_t		CDCIResolutionID;			/* VCID -- Table ID */
	
	omfInt32			firstframeoffset;

}               userDataCDCI_t;


static omfErr_t codecPutCompressionParmsCDCI(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataCDCI_t * pdata);
static omfErr_t codecCDCIReadLines(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t omfmCDCIWriteLines(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t InitCDCICodec(omfCodecParms_t *info);
static omfErr_t codecOpenCDCI(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecCreateCDCI(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecGetCDCIFieldInfo(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecPutCDCIFieldInfo(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecGetRect(omfCodecParms_t * info);
static omfErr_t codecWriteSamplesCDCI(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecReadSamplesCDCI(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecGetSelectInfoCDCI(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecNumChannelsCDCI(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecSetRect(omfCodecParms_t * info);
static omfErr_t codecCDCIReadLines(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t omfmCDCIWriteLines(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t setupStream(omfMediaHdl_t media, userDataCDCI_t * pdata);
static omfErr_t codecCloseCDCI(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t omfmCDCISetFrameNumber(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t omfmCDCIGetFrameOffset(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t initUserData(userDataCDCI_t *pdata);
static omfErr_t postWriteOps(omfCodecParms_t * info, omfMediaHdl_t media);
static omfErr_t writeDescriptorData(omfCodecParms_t * info, 
									omfMediaHdl_t media, 
									omfHdl_t main, 
									userDataCDCI_t * pdata);
static omfErr_t loadDescriptorData(omfCodecParms_t * info, 
								   omfMediaHdl_t media, 
								   omfHdl_t main, 
								   userDataCDCI_t * pdata);
static omfErr_t InitMDESProps(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t myCodecSetMemoryInfo(omfCodecParms_t * parmblk,
				                      omfMediaHdl_t media,
				                      omfHdl_t file,
				                      userDataCDCI_t * pdata);
static omfErr_t codecPutInfoCDCI(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t codecGetInfoCDCI(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t RecomputePixelSampleSize(userDataCDCI_t *pdata, omfInt32 *bytePerSample, omfInt16 *avgBitsPerPixel);
static omfErr_t codecGetVarietyCount(omfCodecParms_t * info,
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

omfErr_t        omfCodecCDCI(omfCodecFunctions_t func, omfCodecParms_t * info)
{
	omfErr_t        	status;
	omcCDCIPersistent_t *pers = (omcCDCIPersistent_t *)info->pers;
	omfCodecOptDispPtr_t	*dispPtr;

	switch (func)
	{
	case kCodecInit:
		InitCDCICodec(info);
		return (OM_ERR_NONE);
		break;

	case kCodecGetMetaInfo:
		strncpy((char *)info->spc.metaInfo.name, "CDCI Codec (Uncompressed ONLY)", info->spc.metaInfo.nameLen);
		info->spc.metaInfo.info.mdesClassID = "CDCI";
		info->spc.metaInfo.info.dataClassID = "IDAT";
		info->spc.metaInfo.info.codecID = CODEC_CDCI_VIDEO;
		info->spc.metaInfo.info.rev = CODEC_REV_3;
		info->spc.metaInfo.info.dataKindNameList = PICTUREKIND;
		info->spc.metaInfo.info.minFileRev = kOmfRev2x;
		info->spc.metaInfo.info.maxFileRevIsValid = FALSE;		/* There is no maximum rev */
		return (OM_ERR_NONE);
			
	case kCodecLoadFuncPointers:
		dispPtr = info->spc.loadFuncTbl.dispatchTbl;

		dispPtr[ kCodecOpen ] = codecOpenCDCI;
		dispPtr[ kCodecCreate ] = codecCreateCDCI;
		dispPtr[ kCodecGetInfo ] = codecGetInfoCDCI;
		dispPtr[ kCodecPutInfo ] = codecPutInfoCDCI;
		dispPtr[ kCodecReadSamples ] = codecReadSamplesCDCI;
		dispPtr[ kCodecWriteSamples ] = codecWriteSamplesCDCI;
		dispPtr[ kCodecReadLines ] = codecCDCIReadLines;
		dispPtr[ kCodecWriteLines ] = omfmCDCIWriteLines;
		dispPtr[ kCodecClose ] = codecCloseCDCI;
		dispPtr[ kCodecSetFrame ] = omfmCDCISetFrameNumber;
		dispPtr[ kCodecGetFrameOffset ] = omfmCDCIGetFrameOffset;
		dispPtr[ kCodecGetNumChannels ] = codecNumChannelsCDCI;
		dispPtr[ kCodecInitMDESProps ] = InitMDESProps;
		dispPtr[ kCodecGetSelectInfo ] = codecGetSelectInfoCDCI;
/*		dispPtr[ kCodecImportRaw ] = NULL;
		dispPtr[ kCodecGetVarietyCount ] = NULL;
		dispPtr[ kCodecGetVarietyInfo ] = NULL;
		dispPtr[ kCodecSemanticCheck ] = NULL;
		dispPtr[ kCodecAddFrameIndexEntry ] = NULL;
*/
		return(OM_ERR_NONE);

	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}

	return (status);
}
	
static omfErr_t codecGetSelectInfoCDCI(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfInt32		storedWidth, storedHeight;
	omfUInt32		subSampling;
	omcCDCIPersistent_t *pers = (omcCDCIPersistent_t *)info->pers;
	omfInt32		compWidth, bitsPerPixel;
	omfInt16		padBits;
    omfHdl_t		file;
    omfErr_t		status;
	omfPosition_t	zeroPos;
	char			compress[128];
	omfBool			avidUncompressed;
	
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
			info->spc.selectInfo.info.willHandleMDES = (strcmp(compress, "AUNC") == 0);
			avidUncompressed = (info->spc.selectInfo.info.willHandleMDES);
			}
		else
		{
			info->spc.selectInfo.info.willHandleMDES = !omfsIsPropPresent(file,
										info->spc.selectInfo.mdes, OMDIDDCompression, OMString);
			avidUncompressed = FALSE;
		}
		info->spc.selectInfo.info.isNative = TRUE;
		info->spc.selectInfo.info.hwAssisted = FALSE;
		info->spc.selectInfo.info.relativeLoss = 0;
		CHECK(omfsReadInt32(file, info->spc.selectInfo.mdes, OMDIDDStoredWidth, &storedWidth));
		CHECK(omfsReadInt32(file, info->spc.selectInfo.mdes, OMDIDDStoredHeight, &storedHeight));
		/***/
		CHECK(omfsReadInt32(main, info->spc.selectInfo.mdes, pers->omCDCIComponentWidth, &compWidth));
		CHECK(omfsReadUInt32(main, info->spc.selectInfo.mdes, pers->omCDCIHorizontalSubsampling,
										&subSampling));

		status = OMReadProp(main, info->spc.selectInfo.mdes, pers->omCDCIPaddingBits,
							   zeroPos, kSwabIfNeeded, OMInt16,
							   sizeof(padBits), (void *)&padBits);
/*		if (status != OM_ERR_NONE)
		{
			padBits = compWidth
			if(avidUncompressed)
				padBits = 2;
			else
				padBits = 0;
		}
*/						 
		switch (subSampling)
		{
		/* 4:4:4 = 1 sample for each of luma and two chroma channels */   
		case 1:
			if (status != OM_ERR_NONE)
				padBits = (8 - ((compWidth * 3) % 8)) % 8;	/* Pad to byte boundry */
			bitsPerPixel = (compWidth * 3) + padBits;
  			break;
  			
  		/* 4:2:2 = two-pixels get 2 samples of luma, 1 of each chroma channel, avg == 2 */							
		case 2:
			if (status != OM_ERR_NONE)
				padBits = (8 - ((compWidth * 2) % 8)) % 8;	/* Pad to byte boundry */
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

static omfErr_t codecNumChannelsCDCI(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfDDefObj_t		dataKind;
	omfErr_t		status = OM_ERR_NONE;

	omfiDatakindLookup(info->spc.getChannels.file, PICTUREKIND, &dataKind, &status);
	if (info->spc.getChannels.mediaKind == dataKind)
		info->spc.getChannels.numCh = 1;
	else
		info->spc.getChannels.numCh = 0;
	
	return(OM_ERR_NONE);
}

static omfErr_t codecPutInfoCDCI(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	userDataCDCI_t * pdata;
	omfErr_t		status = OM_ERR_NONE;

	omfAssertMediaHdl(media);
	pdata = (userDataCDCI_t *) media->userData;

	switch (info->spc.mediaInfo.infoType)
	{
	case kVideoInfo:
		status = codecPutCDCIFieldInfo(info, media, main);
		break;
	case kCompressionParms:
		status = codecPutCompressionParmsCDCI(info, media, main, pdata);
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
omfErr_t        codecPutCompressionParmsCDCI(omfCodecParms_t * info,
					                omfMediaHdl_t media,
					                     omfHdl_t main,
				                     userDataCDCI_t * pdata)
{
/*	XPROTECT(main) */  /* RPS 11/24/99 don't need to use XPROTECT without inner CHECKs
/*	{ */
		pdata->CDCIResolutionID.CDCITableID = ((omfCDCIInfo_t *)info->spc.mediaInfo.buf)->CDCITableID;
/*	} */
/*	XEXCEPT */
/*	XEND */

	return (OM_ERR_NONE);
}

static omfErr_t codecGetInfoCDCI(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfMaxSampleSize_t	*ptr;
	omfErr_t		status = OM_ERR_NONE;
	userDataCDCI_t			*pdata;
	
	omfAssertMediaHdl(media);
	pdata = (userDataCDCI_t *) media->userData;

	switch (info->spc.mediaInfo.infoType)
	{
	case kMediaIsContiguous:
		{
		status = OMIsPropertyContiguous(main, media->dataObj, OMIDATImageData,
										OMDataValue, (omfBool *)info->spc.mediaInfo.buf);
		}
		break;
	case kVideoInfo:
		status = codecGetCDCIFieldInfo(info, media, main);
		break;
	case kCompressionParms:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	case kJPEGTables:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	case kMaxSampleSize:
		ptr = (omfMaxSampleSize_t *) info->spc.mediaInfo.buf;
		if(ptr->mediaKind == media->pictureKind)
			ptr->largestSampleSize = pdata->memBytesPerSample;
		else
			status = OM_ERR_CODEC_CHANNELS;
		break;
	
	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}
		
	return(status);
}

static omfErr_t codecCreateCDCI(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	userDataCDCI_t *pdata;
	omcCDCIPersistent_t *pers = (omcCDCIPersistent_t *)info->pers;
	
	omfAssertMediaHdl(media);
	pdata = (userDataCDCI_t *) media->userData;
	
	XPROTECT(main)
	{
	
		/* copy descriptor information into local structure */
		media->userData = (userDataCDCI_t *)omOptMalloc(main, sizeof(userDataCDCI_t));
		XASSERT(media->userData != NULL, OM_ERR_NOMEMORY);
	
		pdata = (userDataCDCI_t *) media->userData;
		
		CHECK(initUserData(pdata));
		
		/* there are no multiword entities (YET!) */
		media->stream->swapBytes = FALSE; 
	
		/* get sample rate from MDFL parent class of descriptor.  
		 * This assumes that the CDCI descriptor class is a descendant of 
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
		else									// SPIDY
			pdata->rate = media->channels[0].sampleRate;
		
		/* number of frames is unknown until close time */
		omfsCvtInt32toInt64(0, &media->channels[0].numSamples);
	
		pdata->fmtOps[0].opcode = kOmfVFmtEnd;
		
		MakeRational(4, 3, &(pdata->imageAspectRatio));
	
		CHECK(omcOpenStream(main, media->dataFile, media->stream, 
						media->dataObj, OMIDATImageData, OMDataValue));
		pdata->fileFmt[0].opcode = kOmfPixelFormat;	
		pdata->fileFmt[0].operand.expPixelFormat = kOmfPixYUV;	
		
		pdata->fileFmt[1].opcode = kOmfFrameLayout;	
		pdata->fileFmt[1].operand.expFrameLayout = pdata->fileLayout;	
		
		pdata->fileFmt[2].opcode = kOmfPixelSize;	
		if (pdata->horizontalSubsampling == 1)
			pdata->fileFmt[2].operand.expInt16 = (pdata->componentWidth * 3)+pdata->padBits;	
	  	else if (pdata->horizontalSubsampling == 2)						
			pdata->fileFmt[2].operand.expInt16 = (pdata->componentWidth * 2)+pdata->padBits;	

		pdata->fileFmt[3].opcode = kOmfStoredRect;
		pdata->fileFmt[3].operand.expRect.xOffset = 0;
		pdata->fileFmt[3].operand.expRect.xSize = pdata->imageWidth;
		pdata->fileFmt[3].operand.expRect.yOffset = 0;
		pdata->fileFmt[3].operand.expRect.ySize = pdata->imageLength;
		
		pdata->fileFmt[4].opcode = kOmfVideoLineMap;
		pdata->fileFmt[4].operand.expLineMap[0] = pdata->videoLineMap[0];
		pdata->fileFmt[4].operand.expLineMap[1] = pdata->videoLineMap[1];
		
		pdata->fileFmt[5].opcode = kOmfCDCIColorSiting;	
		pdata->fileFmt[5].operand.expColorSiting = pdata->colorSiting;
		pdata->fileFmt[6].opcode = kOmfCDCIBlackLevel;	
		pdata->fileFmt[6].operand.expInt32 = pdata->blackLevel;
		pdata->fileFmt[7].opcode = kOmfCDCIWhiteLevel;	
		pdata->fileFmt[7].operand.expInt32 = pdata->whiteLevel;
		pdata->fileFmt[8].opcode = kOmfCDCIColorRange;	
		pdata->fileFmt[8].operand.expInt32 = pdata->colorRange;
		
		pdata->fileFmt[9].opcode = kOmfVFmtEnd;	
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
static omfErr_t codecOpenCDCI(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	userDataCDCI_t * pdata;
	omfErr_t		status;
	char			compress[128];
	
	omfAssertMediaHdl(media);
	pdata = (userDataCDCI_t *) media->userData;

	/* pull out descriptor information BEFORE reading CDCI data */
	XPROTECT(main)
	{
		media->userData = (userDataCDCI_t *)omOptMalloc(main, sizeof(userDataCDCI_t));
		XASSERT(media->userData != NULL, OM_ERR_NOMEMORY);
			
		pdata = (userDataCDCI_t *) media->userData;
		
		CHECK(initUserData(pdata));
		if(omfsIsPropPresent(media->dataFile, media->mdes, OMDIDDCompression, OMString))
		{
			status = omfsReadString(media->dataFile, media->mdes, OMDIDDCompression, 
											compress, sizeof(compress));
			if(status != OM_ERR_NONE)
				return(status);
			pdata->avidUncompressed = (strcmp(compress, "AUNC") == 0);
			}
		else
		{
			pdata->avidUncompressed = FALSE;
		}
			
		pdata->fmtOps[0].opcode = kOmfVFmtEnd;
		
		CHECK(loadDescriptorData(info, media, main, pdata));
								
		CHECK(setupStream(media, pdata));
	
		/* prepare for data read */
		omfsCvtInt32toInt64( pdata->firstframeoffset, &media->channels[0].dataOffset );
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
static omfErr_t codecWriteSamplesCDCI(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfmMultiXfer_t 	*xfer = NULL;
	omfInt32		 	numFields, n, samp;
	omfUInt32			fileBytes, memBytes;
	char				tmpBuf[8];
	userDataCDCI_t		*pdata = NULL;
	unsigned char		*xferBuffer = NULL;
	
	omfAssertMediaHdl(media);
	
	XPROTECT(main)
	{
		pdata = (userDataCDCI_t	*)media->userData;
		XASSERT(pdata, OM_ERR_NULL_PARAM);
	
		xfer = info->spc.mediaXfer.xfer;
		XASSERT(xfer, OM_ERR_NULLBUF);
		
		/* this codec only allows one-channel media */
		XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
	
		/* set alignment just before first write.  This ensures that the first
		   byte of data in the stream will be aligned to the field alignment 
		   factor
		 */		 
		if ( ! pdata->dataWritten ) 
		{
			CHECK(omfsFileSetValueAlignment(main, pdata->alignment));
		}
		
		/* Here's where field alignment is handled on WRITE!  Since fields
		   are aligned seperately, (and we're assuming uncompressed in this
		   codec) if there are two fields, treat them as individual items. 
		   After writing out each item, check the stream position, and fill
		   out the block if necessary.  ASSUMPTION: setInfoArray will have
		   set up a block of memory (pdata->alignmentBuffer) of enough size.
		   Since alignment is expensive even if not needed, try to sift out
		   situations where alignment is not needed.
		 */
			if(pdata->fileLayout == kSeparateFields || pdata->fileLayout == kMixedFields)
				numFields = 2;
			else
				numFields = 1;
			fileBytes = pdata->fileBytesPerSample / numFields;
			CHECK(omcComputeBufferSize(media->stream, fileBytes, omcComputeMemSize, &memBytes));
			if(memBytes > xfer->buflen)
				RAISE(OM_ERR_SMALLBUF);
	
			CHECK(omfmSetSwabBufSize(media->stream, memBytes));
			xfer->bytesXfered = 0;
			xfer->samplesXfered = 0;
			xferBuffer = (unsigned char*) xfer->buffer;
			for(samp = 1; samp <= xfer->numSamples; samp++, xfer->samplesXfered++)
			{
				for(n = 1; n <= numFields; n++)
				{
					CHECK(omcWriteSwabbedStream(media->stream, fileBytes, memBytes, xferBuffer));
					xfer->bytesXfered += memBytes;
					if((pdata->clientFillEnd != 0) && (pdata->clientFillEnd < sizeof(tmpBuf)))
					{
						omfInt16	j;
				
						for(j = 0; j < pdata->clientFillEnd-1; j++)
							tmpBuf[j] = (char)0xFF;
						tmpBuf[pdata->clientFillEnd-1] = (char) 0xD9;
						CHECK(omcWriteStream(media->stream, pdata->clientFillEnd, tmpBuf));
					}

					if (pdata->alignment > 1)  /* only bother precomputing if necessary */
					{
						omfUInt32 position32;
						omfInt32 remainderBytes;

						CHECK(omcGetStreamPos32(media->stream, &position32));
						
						remainderBytes = (position32 % pdata->alignment);
						
						if (remainderBytes  != 0)
						{
							omfInt32 padAmount = pdata->alignment - remainderBytes;

							XASSERT(pdata->alignmentBuffer != NULL, OM_ERR_NULL_PARAM);
							 /* ROGER FIX THIS ERROR NUMBER, NULL_PARAM is not quite right */			

							CHECK(omcWriteStream(media->stream, padAmount,
												pdata->alignmentBuffer));
						}
					}
  					xferBuffer += memBytes;					
				}
			}

		pdata->dataWritten = TRUE;

		/* Don't forget to update the global frame counter */
		CHECK(omfsAddInt32toInt64(xfer->numSamples, &media->channels[0].numSamples));
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
static omfErr_t codecReadSamplesCDCI(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	userDataCDCI_t 	*pdata = NULL;
	omfUInt32			fileBytes = 0;
	omfUInt32			bytesRead = 0;
	omfmMultiXfer_t	*xfer = NULL;
	omfUInt32			rBytes, memBytes;
	omfInt32		 	n, numFields, samp;
   	unsigned char *xferBuffer = NULL;

	omfAssertMediaHdl(media);	
	
	XPROTECT(main)
	{
		pdata = (userDataCDCI_t *) media->userData;
		XASSERT(pdata, OM_ERR_NULL_PARAM);

		xfer = info->spc.mediaXfer.xfer;
		XASSERT(xfer, OM_ERR_NULLBUF);
			
		XASSERT(media->numChannels == 1, OM_ERR_CODEC_CHANNELS);
		XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
					
		/* Here's where field alignment is handled on READ!  Since fields
		   are aligned seperately, (and we're assuming uncompressed in this
		   codec) if there are two fields, treat them as individual items. 
		   After reading out each item, check the stream position, and skip
		   past the block if necessary.  Since alignment is expensive even 
		   if not needed, try to sift out situations where alignment is not 
		   needed.
		 */
			if(pdata->fileLayout == kSeparateFields || pdata->fileLayout == kMixedFields)
				numFields = 2;
			else
				numFields = 1;

			fileBytes = pdata->fileBytesPerSample / numFields;
			CHECK(omcComputeBufferSize(media->stream, fileBytes, omcComputeMemSize, &memBytes));
			if(memBytes > xfer->buflen)
				RAISE(OM_ERR_SMALLBUF);
	
			CHECK(omfmSetSwabBufSize(media->stream, memBytes));
			xfer->bytesXfered = 0;
			xfer->samplesXfered = 0;
			xferBuffer = (unsigned char*) xfer->buffer;
			for(samp = 1; samp <= xfer->numSamples; samp++, xfer->samplesXfered++)
			{
				for(n = 1; n <= numFields; n++)
				{
					CHECK(omcReadSwabbedStream(media->stream, fileBytes,
						memBytes, xferBuffer, &rBytes));
					xfer->bytesXfered += rBytes;
					if(pdata->clientFillEnd != 0)
					{
						CHECK(omcSeekStreamRelative(media->stream, pdata->clientFillEnd));	/* Skip over marker codes */
					}
					if (pdata->alignment > 1)  /* only bother precomputing if necessary */
					{
						omfUInt32 position32;
						omfInt32 remainderBytes;
						
						CHECK(omcGetStreamPos32(media->stream, &position32));
						remainderBytes = (position32 % pdata->alignment);
						if (remainderBytes != 0)
						{
							omfInt32 padAmount = pdata->alignment - remainderBytes;
							CHECK(omcSeekStreamRelative(media->stream, padAmount));
						}
					}
					xferBuffer += rBytes;
				}
			}
		}
	
	XEXCEPT
	{
		if((XCODE() == OM_ERR_EOF) || (XCODE() == OM_ERR_END_OF_DATA))
		{
			xfer->bytesXfered += rBytes;	/* Don't subtract off clientFillEnd, as we hit EOF	*/
			xfer->samplesXfered += rBytes / pdata->fileBytesPerSample;
		}
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
									userDataCDCI_t * pdata)
{
	omcCDCIPersistent_t *pers = (omcCDCIPersistent_t *)info->pers;

	/* this routine will be called after sample data is written */

	omfAssertMediaHdl(media);
 
	pdata = (userDataCDCI_t *) media->userData;
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
		
		
		CHECK(omfsWriteString(main, media->mdes, OMDIDDCompression, "AUNC"));
      	if ((main->setrev == kOmfRev1x) || main->setrev == kOmfRevIMA)
		{
			CHECK(omfsWriteObjectTag(main, media->mdes, pers->omAvJPEDCompress, "AUNC"));
		}
		
		/* OMFI:DIDD:ImageAspectRatio */		
		CHECK(omfsWriteRational(main, media->mdes, OMDIDDImageAspectRatio,
				 						 pdata->imageAspectRatio));
		
		CHECK(omfsWriteInt32(main, media->mdes, OMDIDDFieldAlignment, 
							pdata->alignment));

		CHECK(omfsWriteInt32(main, media->mdes, pers->omCDCIComponentWidth,
										pdata->componentWidth));
		
		CHECK(OMWriteBaseProp(main, media->mdes, pers->omCDCIColorSiting,
							   OMColorSitingType, sizeof(pdata->colorSiting),
							    (void *)&pdata->colorSiting));		/* Write as 2-bytes!!! */
							    
		CHECK(omfsWriteUInt32(main, media->mdes, pers->omCDCIHorizontalSubsampling,
										pdata->horizontalSubsampling));

		CHECK(omfsWriteUInt32(main, media->mdes, pers->omCDCIBlackReferenceLevel,
										pdata->blackLevel));

		CHECK(omfsWriteUInt32(main, media->mdes, pers->omCDCIWhiteReferenceLevel,
										pdata->whiteLevel));

		CHECK(omfsWriteUInt32(main, media->mdes, pers->omCDCIColorRange,
										pdata->colorRange));

		CHECK(omfsWriteInt16(main, media->mdes, pers->omCDCIPaddingBits,
										pdata->padBits));

		CHECK(omfsWriteInt32(main, media->mdes, OMDIDDClientFillStart, pdata->clientFillStart));
		CHECK(omfsWriteInt32(main, media->mdes, OMDIDDClientFillEnd, pdata->clientFillEnd));

		CHECK(omfsWriteInt32(main, media->mdes, pers->omAvJPEDResolutionID, pdata->CDCIResolutionID.CDCITableID));

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
									userDataCDCI_t * pdata)
{
	omfInt32 				sampleFrames = 0;
	omfUInt32 				layout = 0;
	omfUInt32 				bytesPerPixel = 0;
	omcCDCIPersistent_t *pers = NULL;
	omfPosition_t			zeroPos;
	omfErr_t					status;
	omfObject_t				mdes = media->mdes;
	omfInt32					numFields = 0;
	
	omfsCvtInt32toPosition(0, zeroPos);
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);
	
	XPROTECT(main)
	{
		pers = (omcCDCIPersistent_t *)info->pers;
	 
		if (info->fileRev == kOmfRev2x)
		{
	 		CHECK(omfsReadRational(main, mdes, OMMDFLSampleRate, 
								   &(pdata->rate)));
	 	}
	 	else
	 	{
	 		CHECK(omfsReadExactEditRate(main, mdes, OMMDFLSampleRate, 
										&(pdata->rate)));
		} 		
		CHECK(omfsReadLength(main, mdes, OMMDFLLength, 
							 &media->channels[0].numSamples));
	
		media->numChannels = 1;
	
		CHECK(omcOpenStream(main, media->dataFile, media->stream, 
						media->dataObj, OMIDATImageData, OMDataValue));
			
		CHECK(omfsReadInt32(main, mdes, OMDIDDStoredWidth, 
						   &pdata->imageWidth));
		CHECK(omfsReadInt32(main, mdes, OMDIDDStoredHeight, 
						   &pdata->imageLength));

		if ( OM_ERR_NONE != omfsReadInt32(main, mdes, pers->omAvDIDDFirstFrameOffset, &(pdata->firstframeoffset))) 
			pdata->firstframeoffset = 0;
		
		/* sampled dimensions default to stored dimensions.  If any calls fail
		 * later, default will refile untouched. 
		 */
	
		pdata->sampledWidth = pdata->imageWidth;
		pdata->sampledLength = pdata->imageLength;
		pdata->sampledXOffset = 0;
		pdata->sampledYOffset = 0;
		
		if (omfsReadInt32(main, mdes, OMDIDDSampledWidth, 
						 &pdata->sampledWidth) == OM_ERR_NONE &&
			omfsReadInt32(main, mdes, OMDIDDSampledHeight, 
						 &pdata->sampledLength) == OM_ERR_NONE)
		{
			/* ignore errors here, default to zero in both cases */		
			(void)omfsReadInt32(main, mdes, OMDIDDSampledXOffset, 
							   &pdata->sampledXOffset);
			(void)omfsReadInt32(main, mdes, OMDIDDSampledYOffset, 
							   &pdata->sampledYOffset);
		}
		
		pdata->displayWidth = pdata->imageWidth;
		pdata->displayLength = pdata->imageLength;
		pdata->displayXOffset = 0;
		pdata->displayYOffset = 0;
		
		if (omfsReadInt32(main, mdes, OMDIDDDisplayWidth, 
						 &pdata->displayWidth) == OM_ERR_NONE &&
			omfsReadInt32(main, mdes, OMDIDDDisplayHeight, 
						 &pdata->displayLength) == OM_ERR_NONE)
		{
			/* ignore errors here, default to zero in both cases */		
			(void)omfsReadInt32(main, mdes, OMDIDDDisplayXOffset, 
							   &pdata->displayXOffset);
			(void)omfsReadInt32(main, mdes, OMDIDDDisplayYOffset, 
							   &pdata->displayYOffset);
		}
			
		CHECK(omfsReadRational(main, mdes, OMDIDDImageAspectRatio, 
							   &pdata->imageAspectRatio))

		status = omfsReadInt32(main, mdes, OMDIDDFieldAlignment,
								&pdata->alignment);
		
		CHECK(omfsReadLayoutType(main, mdes, OMDIDDFrameLayout, 
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
			
			status = OMReadProp(main, mdes, OMDIDDVideoLineMap,
								  zeroPos, kSwabIfNeeded, OMInt32Array,
								  sizeof(omfInt32), (void *)&pdata->videoLineMap[0]);
			if (status != OM_ERR_NONE)
				pdata->videoLineMap[0] = 0;
			else
				mapFlags |= 1;
				
			if (pdata->fileLayout == kSeparateFields || pdata->fileLayout == kMixedFields)
			{
				omfsCvtInt32toPosition(sizeof(omfInt32), fourPos);
			
				status = OMReadProp(main, mdes, OMDIDDVideoLineMap,
									  fourPos, kSwabIfNeeded, OMInt32Array,
									  sizeof(omfInt32), (void *)&pdata->videoLineMap[1]);
				if (status != OM_ERR_NONE)
					pdata->videoLineMap[1] = 0;
				else
					mapFlags |= 2;
			}
		}

		CHECK(omfsReadInt32(main, mdes, pers->omCDCIComponentWidth, 
						 &(pdata->componentWidth)));
						 
		switch (pdata->componentWidth)
		{
		case 8:
		case 10:
		case 16:
			break;
			
		default:
			RAISE(OM_ERR_BADPIXFORM);
		}
				
		CHECK(omfsReadUInt32(main, mdes, pers->omCDCIHorizontalSubsampling,
										&(pdata->horizontalSubsampling)));
		
		status = OMReadProp(main, mdes, pers->omCDCIPaddingBits,
							   zeroPos, kSwabIfNeeded, OMInt16,
							   sizeof(pdata->padBits), (void *)&(pdata->padBits));
		if (status != OM_ERR_NONE)
		{
			switch (pdata->horizontalSubsampling)
			{
				/* 4:4:4 = 1 sample for each of luma and two chroma channels */   
				case 1:
					pdata->padBits = (8 - ((pdata->componentWidth * 3) % 8)) % 8;	/* Pad to byte boundry */
		  			break;
		  			
		  		/* 4:2:2 = two-pixels get 2 samples of luma, 1 of each chroma channel, avg == 2 */							
				case 2:
					pdata->padBits = (8 - ((pdata->componentWidth * 2) % 8)) % 8;	/* Pad to byte boundry */
					break;
			}
		}
		
		status = OMReadProp(main, mdes, pers->omCDCIColorSiting,		/* Read as 1 or 2-bytes!!! */
							   zeroPos, kSwabIfNeeded, OMColorSitingType,
							   sizeof(pdata->colorSiting), (void *)&(pdata->colorSiting));
		if (status != OM_ERR_NONE)
			pdata->colorSiting = kCoSiting;
							    
		if(media->fileMob != NULL)
		{
			omfUID_t	mobID;
			
			CHECK(omfiMobGetMobID(main, media->fileMob, &mobID));
			if(mobID.prefix == 42 && pdata->avidUncompressed)
				pdata->horizontalSubsampling = 2;	/* The file lies!! it's really 4:2:2 */
		}
		CHECK(RecomputePixelSampleSize(pdata, &pdata->fileBytesPerSample, &pdata->bitsPerPixelAvg));
		pdata->memBytesPerSample = pdata->fileBytesPerSample;
				

		status = omfsReadUInt32(main, mdes, pers->omCDCIBlackReferenceLevel,
										&(pdata->blackLevel));
		if (status != OM_ERR_NONE)
			pdata->blackLevel = 0;

		status = omfsReadUInt32(main, mdes, pers->omCDCIWhiteReferenceLevel,
										&(pdata->whiteLevel));
		if (status != OM_ERR_NONE)
			pdata->whiteLevel = powi(2, pdata->componentWidth) -1;

		status = omfsReadUInt32(main, mdes, pers->omCDCIColorRange,
										&(pdata->colorRange));
		if (status != OM_ERR_NONE)
			pdata->colorRange = powi(2, pdata->componentWidth) -2;			
			

		status = omfsReadUInt32(main, mdes, pers->omCDCIVerticalSubsampling,
										&(pdata->verticalSubsampling));
		if (status != OM_ERR_NONE)
			pdata->verticalSubsampling = 1;
				
		status = omfsReadInt32(main, mdes, OMDIDDClientFillStart,
										&(pdata->clientFillStart));
		if (status != OM_ERR_NONE)
			pdata->clientFillStart = 0;

		status = omfsReadInt32(main, mdes, OMDIDDClientFillEnd,
										&(pdata->clientFillEnd));/* Bytes at the end of every frame (non-pixels) */
		if (status != OM_ERR_NONE)
			{
			if (pdata->avidUncompressed)
				pdata->clientFillEnd = 4;			/* To fix a bug where MediaComposer doesn't export this field */
			else
				pdata->clientFillEnd = 0;
			}

		status = omfsReadInt32(main, mdes, pers->omAvJPEDResolutionID, &(pdata->CDCIResolutionID.CDCITableID));
		if (status != OM_ERR_NONE)
			pdata->CDCIResolutionID.CDCITableID = 0;



		pdata->fileFmt[0].opcode = kOmfPixelFormat;	
		pdata->fileFmt[0].operand.expPixelFormat = kOmfPixYUV;	
		
		pdata->fileFmt[1].opcode = kOmfFrameLayout;	
		pdata->fileFmt[1].operand.expFrameLayout = pdata->fileLayout;	
		
		pdata->fileFmt[2].opcode = kOmfPixelSize;	
		if (pdata->horizontalSubsampling == 1)
			pdata->fileFmt[2].operand.expInt16 = (pdata->componentWidth * 3)+pdata->padBits;	
	  	else if (pdata->horizontalSubsampling == 2)						
			pdata->fileFmt[2].operand.expInt16 = (pdata->componentWidth * 2)+pdata->padBits;	

		pdata->fileFmt[3].opcode = kOmfStoredRect;
		pdata->fileFmt[3].operand.expRect.xOffset = 0;
		pdata->fileFmt[3].operand.expRect.xSize = pdata->imageWidth;
		pdata->fileFmt[3].operand.expRect.yOffset = 0;
		pdata->fileFmt[3].operand.expRect.ySize = pdata->imageLength;
		
		pdata->fileFmt[4].opcode = kOmfVideoLineMap;
		pdata->fileFmt[4].operand.expLineMap[0] = pdata->videoLineMap[0];
		pdata->fileFmt[4].operand.expLineMap[1] = pdata->videoLineMap[1];
		
		pdata->fileFmt[5].opcode = kOmfCDCIColorSiting;	
		pdata->fileFmt[5].operand.expColorSiting = pdata->colorSiting;
		pdata->fileFmt[6].opcode = kOmfCDCIBlackLevel;	
		pdata->fileFmt[6].operand.expInt32 = pdata->blackLevel;
		pdata->fileFmt[7].opcode = kOmfCDCIWhiteLevel;	
		pdata->fileFmt[7].operand.expInt32 = pdata->whiteLevel;
		pdata->fileFmt[8].opcode = kOmfCDCIColorRange;	
		pdata->fileFmt[8].operand.expInt32 = pdata->colorRange;
		
		pdata->fileFmt[9].opcode = kOmfVFmtEnd;	
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
static omfErr_t omfmCDCISetFrameNumber(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfInt64          nBytes, zero;
	omfUInt32			bytesPerSample, remainder, numFields;
	userDataCDCI_t * pdata;


	omfAssertMediaHdl(media);
	pdata = (userDataCDCI_t *) media->userData;

	omfAssert(media->dataObj, main, OM_ERR_INTERNAL_MDO);
	XPROTECT(main)
	{
		omfsCvtInt32toInt64(0, &zero);

		nBytes = info->spc.setFrame.frameNumber;
		XASSERT(omfsInt64NotEqual(nBytes, zero), OM_ERR_BADFRAMEOFFSET);

		omfsSubInt32fromInt64(1, &nBytes);
		if ((pdata->fileLayout == kSeparateFields) ||
			    (pdata->fileLayout == kMixedFields))
			numFields = 2;
		else
			numFields = 1;				
		bytesPerSample = 
			pdata->fileBytesPerSample + (numFields * 
										(pdata->clientFillStart + pdata->clientFillEnd));
		
		if (pdata->alignment > 1)
		{
			if ((pdata->fileLayout == kSeparateFields) ||
			    (pdata->fileLayout == kMixedFields))
				bytesPerSample /= 2;
				
			remainder = bytesPerSample % pdata->alignment;
			if (remainder)
				bytesPerSample += pdata->alignment - remainder;
				
			if ((pdata->fileLayout == kSeparateFields) ||
			    (pdata->fileLayout == kMixedFields))
				bytesPerSample *= 2;				
		}
//		else /* DF FIX */		// JeffB: Find out who added this. Client fill end * 2 already added in for separate fields
//		{
//			int fill = pdata->clientFillEnd;
//			if ((pdata->fileLayout == kSeparateFields) ||
//				(pdata->fileLayout == kMixedFields))
//				{
//					fill *=2;
//				}
//			bytesPerSample += fill;
//		}
		CHECK(omfsMultInt32byInt64(bytesPerSample, nBytes, &nBytes));
		omfsAddInt64toInt64( media->channels[0].dataOffset, &nBytes ); // SPIDY
		CHECK(omcSeekStreamTo(media->stream, nBytes));
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * omfmCDCIGetFrameOffset
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
static omfErr_t omfmCDCIGetFrameOffset(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfInt64          nBytes, zero;
	omfUInt32			bytesPerSample, remainder, numFields;
	userDataCDCI_t * pdata;

	omfAssertMediaHdl(media);
	pdata = (userDataCDCI_t *) media->userData;

	omfAssert(media->dataObj, main, OM_ERR_INTERNAL_MDO);
	XPROTECT(main)
	{
		omfsCvtInt32toInt64(0, &zero);

		nBytes = info->spc.getFrameOffset.frameNumber;
		XASSERT(omfsInt64NotEqual(nBytes, zero), OM_ERR_BADFRAMEOFFSET);

		omfsSubInt32fromInt64(1, &nBytes);
		if ((pdata->fileLayout == kSeparateFields) ||
			    (pdata->fileLayout == kMixedFields))
			numFields = 2;
		else
			numFields = 1;				
		bytesPerSample = 
			pdata->fileBytesPerSample + (numFields * 
										(pdata->clientFillStart + pdata->clientFillEnd));
		
		if (pdata->alignment > 1)
		{
			if ((pdata->fileLayout == kSeparateFields) ||
			    (pdata->fileLayout == kMixedFields))
				bytesPerSample /= 2;
				
			remainder = bytesPerSample % pdata->alignment;
			if (remainder)
				bytesPerSample += pdata->alignment - remainder;
				
			if ((pdata->fileLayout == kSeparateFields) ||
			    (pdata->fileLayout == kMixedFields))
				bytesPerSample *= 2;				
		}
		CHECK(omfsMultInt32byInt64(bytesPerSample, nBytes, &nBytes));
		omfsAddInt64toInt64( media->channels[0].dataOffset, &nBytes ); // SPIDY
		info->spc.getFrameOffset.frameOffset = nBytes;
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
static omfErr_t codecGetCDCIFieldInfo(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfVideoMemOp_t 	*vparms;
	userDataCDCI_t * pdata;

	omfAssertMediaHdl(media);
	pdata = (userDataCDCI_t *) media->userData;
	
	XPROTECT(main)  /* added 3/28/96 RPS to catch error raised in switch default */
	{
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
			  if (pdata->horizontalSubsampling == 1)
			    vparms->operand.expInt16 = pdata->bitsPerPixelAvg;	
			  else if (pdata->horizontalSubsampling == 2)						
			    vparms->operand.expInt16 = pdata->bitsPerPixelAvg;	
			  break;
			  
			case kOmfAspectRatio:
			  vparms->operand.expRational = pdata->imageAspectRatio;
			  break;
			  
			case kOmfWillTransferLines:
				vparms->operand.expBoolean = TRUE;
				break;
				
			case kOmfIsCompressed:
				vparms->operand.expBoolean = FALSE;
				break;
				
			case 	kOmfCDCICompWidth:	    /* operand.expInt32 */
				vparms->operand.expInt32 = pdata->componentWidth;
				break;
				
			case kOmfCDCIPadBits:
				vparms->operand.expInt16 = pdata->padBits;
				break;

			case 	kOmfCDCIHorizSubsampling:/* operand.expUInt32 */
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

			default:
			  	RAISE(OM_ERR_ILLEGAL_FILEFMT);
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
static omfErr_t codecPutCDCIFieldInfo(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfVideoMemOp_t 	*vparms;
	omfVideoMemOp_t   oneVParm;
	omcCDCIPersistent_t *pers = (omcCDCIPersistent_t *)info->pers;
	omfRect_t			*aRect;
	omfUInt32			lineMapFlags = 0;
	userDataCDCI_t * pdata;

	omfAssertMediaHdl(media);
	pdata = (userDataCDCI_t *) media->userData;
	XPROTECT(main) /* RPS 3/28/96 added to catch exception raised in switch default */
	{
	  memset(&oneVParm, 0, sizeof(oneVParm));
		/* loop through the opcodes, looking for frame layout.  This value may be needed
		   to process another opcode, but order isn't guaranteed. */
		vparms = (omfVideoMemOp_t *)info->spc.mediaInfo.buf;
		for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
		{
			if(vparms->opcode == kOmfFrameLayout)
			{
				pdata->fileLayout = vparms->operand.expFrameLayout;
			}
		}

		vparms = (omfVideoMemOp_t *)info->spc.mediaInfo.buf;
		for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
		{
			switch(vparms->opcode)
			{
			case kOmfPixelFormat:
			case kOmfFrameLayout:	/* Frame layout handled above */
			  break;
			  
		  	case kOmfImageAlignmentFactor:
		  		pdata->alignment = vparms->operand.expInt32;
		  		pdata->alignmentBuffer = (char *)omOptMalloc(main, (size_t)pdata->alignment);
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

			case kOmfAspectRatio:
			  pdata->imageAspectRatio = vparms->operand.expRational;
			  break;

			/* RPS 3/28/96 added CDCI attributes and default exception */
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
					CHECK(RecomputePixelSampleSize(pdata, &pdata->fileBytesPerSample,
															&pdata->bitsPerPixelAvg));
		  			/* compute pixel size and put in the file fmt */
		  			oneVParm.opcode = kOmfPixelSize;
		  			oneVParm.operand.expInt16 = pdata->bitsPerPixelAvg; /* round up */
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
		
				/* RPS 2.28.96  Hmmm... a tough one here.  There is no rule that component
				   width and horizontal subsampling have to be specified at the same time.
				   However, fileBytesPerSample depends on both width and subsampling.  Recompute
				   here, but only if the fileBytesPerSample value already exists.   If it doesn't,
				   then the code following the switch will be executed. */
				
				if (pdata->fileBytesPerSample)
				{
					CHECK(RecomputePixelSampleSize(pdata, &pdata->fileBytesPerSample,
															&pdata->bitsPerPixelAvg));
		  			/* compute pixel size and put in the file fmt */
		  			oneVParm.opcode = kOmfPixelSize;
		  			oneVParm.operand.expInt16 = pdata->bitsPerPixelAvg;
		  			CHECK(omfmVideoOpAppend(main, kOmfForceOverwrite, oneVParm, pdata->fileFmt,
		  											MAX_FMT_OPS+1))
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
	  
		CHECK(omfmVideoOpMerge(main, kOmfForceOverwrite, 
					(omfVideoMemOp_t *) info->spc.mediaInfo.buf,
					pdata->fileFmt, MAX_FMT_OPS+1));
	
		/* filePixelFormat is FIXED for compressed data */
		
		/* START RPS 3/28/96 added default and synthetic data computation if needed and
		   available from component width */
		if (pdata->componentWidth)
		{
			if (! pdata->fileBytesPerSample)
			{
				omfInt16     		numFields = 0;
				
				if (pdata->fileLayout == kNoLayout)
					RAISE(OM_ERR_ILLEGAL_FILEFMT);
					
				numFields = ((pdata->fileLayout == kSeparateFields) ||
						 (pdata->fileLayout == kMixedFields) ? 2 : 1);		
				
				/* RPS 2.28.96  Hmmm... a tough one here.  There is no rule that component
				   width and horizontal subsampling have to be specified at the same time.
				   However, fileBytesPerSample depends on both width and subsampling.  One option
				   is to recompute at the point of receiving a horizontal subsampling value,
				   but be sure that the fileBytesPerSample value already exists.   If it doesn't,
				   then this code will be run */
				
				CHECK(RecomputePixelSampleSize(pdata, &pdata->fileBytesPerSample,
															&pdata->bitsPerPixelAvg));
	  			/* compute pixel size and put in the file fmt */
	  			oneVParm.opcode = kOmfPixelSize;
	  			oneVParm.operand.expInt16 = pdata->bitsPerPixelAvg;
	  			CHECK(omfmVideoOpAppend(main, kOmfForceOverwrite, oneVParm, pdata->fileFmt,
	  											MAX_FMT_OPS+1))

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

static omfErr_t codecCloseCDCI(omfCodecParms_t *info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	userDataCDCI_t *pdata;

	omfAssertMediaHdl(media);
	pdata = (userDataCDCI_t *)media->userData;

	XPROTECT(main)
	{
		CHECK(omcCloseStream(media->stream));
		if((media->openType == kOmfiCreated) || (media->openType == kOmfiAppended))
		{
			/* Roger-- restore normal alignment requirements */
			if(main->fmt == kOmfiMedia) // SPIDY
				CHECK(omfsFileSetValueAlignment(main, pdata->saveAlignment));
			if (pdata->alignmentBuffer)
			{
				omOptFree(main, (void *)pdata->alignmentBuffer);
				pdata->alignmentBuffer = NULL;
			}
			
			CHECK(postWriteOps(info, media));
			if(main->fmt == kOmfiMedia) // SPIDY
			{
				if ( ! pdata->descriptorFlushed )
					CHECK(writeDescriptorData(info, media, main, pdata));
			}
		}
	
		/* don't forget to cleanup allocated memory from open/create */
		if (pdata->alignmentBuffer)
		{
			omOptFree(main, (void *)pdata->alignmentBuffer);
		
			pdata->alignmentBuffer = NULL;
		}
		omOptFree(main, media->userData);

		media->userData = NULL;
	
		CHECK(omfmCodecRestoreDefaultStreamFuncs(main));
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

	userDataCDCI_t *pdata;
	omfHdl_t        main = NULL;
	omcCDCIPersistent_t *pers;

	/* this routine will be called after sample data is written */

	omfAssertMediaHdl(media);
	main = media->mainFile;
	
 	pers = (omcCDCIPersistent_t *)info->pers;
 
	pdata = (userDataCDCI_t *) media->userData;
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

static omfErr_t setupStream(omfMediaHdl_t media, userDataCDCI_t * pdata)
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
		
		/* RPS 3/28/96 removed setting of pdata->fileFmt, done in PutFileInfo */
				
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
static omfErr_t initUserData(userDataCDCI_t *pdata)
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
   pdata->fileBytesPerSample = 0;
	pdata->fileLayout = kNoLayout;
	pdata->imageAspectRatio.numerator = 0;
	pdata->imageAspectRatio.denominator = 0;
	pdata->fmtOps[0].opcode = kOmfVFmtEnd;
	pdata->fileFmt[0].opcode = kOmfVFmtEnd;
	pdata->descriptorFlushed = FALSE;
	pdata->dataWritten = FALSE;
	pdata->alignment = 1;
	pdata->saveAlignment = 1;
	pdata->alignmentBuffer = NULL;
	pdata->numLines = 0;
	pdata->fieldDominance = kNoDominant;
	pdata->videoLineMap[0] = 16;	/* Field 1 top also */
	pdata->videoLineMap[1] = 17;
	pdata->componentWidth = 8;
	/* RPS 3/28/96 added missing CDCI attributes */
	pdata->horizontalSubsampling = 2;  /* YCbCr 4:2:2 */
	pdata->verticalSubsampling = 1;  /* no subsampling */
	pdata->colorSiting = kCoSiting;
	pdata->whiteLevel = 235;
	pdata->blackLevel = 16;
	pdata->colorRange = 225;
	pdata->bitsPerPixelAvg = 0;
	pdata->memBytesPerSample = 0;
	pdata->padBits = 0;
	
	pdata->fileFmt[0].opcode = kOmfPixelFormat;
	pdata->fileFmt[0].operand.expPixelFormat = kOmfPixYUV;
	pdata->fileFmt[1].opcode = kOmfVFmtEnd;
	
	pdata->avidUncompressed = FALSE;
	pdata->clientFillStart = 0;
	pdata->clientFillEnd = 4;

	pdata->CDCIResolutionID.CDCITableID = 151;  /* AUNC */

	pdata->firstframeoffset = 0;
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
static omfErr_t InitCDCICodec(omfCodecParms_t *info)
	{
	omcCDCIPersistent_t	*persist;
	
	XPROTECT(NULL)
	{
		persist = (omcCDCIPersistent_t *)omOptMalloc(NULL, sizeof(omcCDCIPersistent_t));
		if(persist == NULL)
			RAISE(OM_ERR_NOMEMORY);
		
		/* LF-W: Moved dynamic registration of ColorSitingType to static
		 * registration in omFile.c 
		 */
					   
		/* Add the codec-related properties.  See the OMF Interchange 
		 * Specification 2.0 for details.
		 */
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"ComponentWidth", OMClassCDCI, 
					   OMInt32, kPropRequired, &persist->omCDCIComponentWidth));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"HorizontalSubsampling", OMClassCDCI, 
					   OMUInt32, kPropRequired, &persist->omCDCIHorizontalSubsampling));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"ColorSiting", OMClassCDCI, 
					   OMColorSitingType, kPropOptional, &persist->omCDCIColorSiting));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"BlackReferenceLevel", OMClassCDCI, 
					   OMUInt32, kPropOptional, &persist->omCDCIBlackReferenceLevel));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"WhiteReferenceLevel", OMClassCDCI, 
					   OMUInt32, kPropOptional, &persist->omCDCIWhiteReferenceLevel));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"ColorRange", OMClassCDCI, 
					   OMUInt32, kPropOptional, &persist->omCDCIColorRange));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"PaddingBits", OMClassCDCI, 
					   OMInt16, kPropOptional, &persist->omCDCIPaddingBits));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"VerticalSubsampling", OMClassCDCI, 
					   OMInt32, kPropOptional, &persist->omCDCIVerticalSubsampling));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"DIDResolutionID", "DIDD",
					   OMInt32, kPropRequired, &persist->omAvJPEDResolutionID));
		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"DIDCompressMethod", "DIDD",
					   OMObjectTag, kPropRequired, &persist->omAvJPEDCompress));

		CHECK(omfsRegisterDynamicProp(info->spc.init.session, kOmfTstRevEither,
						"FirstFrameOffset", "DIDD",
					   OMInt32, kPropOptional, &persist->omAvDIDDFirstFrameOffset));

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
static omfErr_t omfmCDCIWriteLines(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfInt32           nBytes, numFields, numLinesToNextField, numLinesToWrite;
	omfInt32           bIndex = 0;
	userDataCDCI_t * pdata;

	omfAssertMediaHdl(media);
	pdata = (userDataCDCI_t *) media->userData;

	XPROTECT(main)
	{
		if((pdata->fileLayout == kSeparateFields) ||
		   (pdata->fileLayout == kMixedFields))
			numFields = 2;
		else
			numFields = 1;
	

		/* set alignment just before first write.  This ensures that the first
		   byte of data in the stream will be aligned to the field alignment 
		   factor
		 */
		 
		if ( ! pdata->dataWritten ) 
		{
			CHECK(omfsFileSetValueAlignment(main, pdata->alignment));
		}
		
		/* Here's where field alignment is handled on WRITE!  Since fields
		   are aligned seperately, (and we're assuming uncompressed in this
		   codec) if there are two fields, treat them as individual items. 
		   After writing out each item, check the stream position, and fill
		   out the block if necessary.  ASSUMPTION: setInfoArray will have
		   set up a block of memory (pdata->alignmentBuffer) of enough size.
		   Since alignment is expensive even if not needed, try to sift out
		   situations where alignment is not needed.
		 */
		
		/* use pdata->numLines % imageLength to work correctly in second field */
		numLinesToNextField = pdata->imageLength - (pdata->numLines % pdata->imageLength);
		numLinesToWrite = info->spc.mediaLinesXfer.numLines;
		
		/* If field alignment is specified, see if the number of lines crosses the 
		   field boundary, if so, write whole fields, padding afterward, until the
		   number of lines won't cross a field boundary */
				
		bIndex = 0;

		while ((pdata->alignment > 1) && (numLinesToWrite  > numLinesToNextField) )
		{
			omfUInt32 position32;
			omfUInt32 remainderBytes;
			
			nBytes = pdata->imageWidth * ((pdata->bitsPerPixelAvg + 7) / 8) * numLinesToNextField;
			CHECK(omcWriteStream(media->stream, nBytes, 
								 (void *)&(((char *)info->spc.mediaLinesXfer.buf)[bIndex])));
			bIndex += nBytes;
			numLinesToWrite -= numLinesToNextField;
			numLinesToNextField = pdata->imageLength;

			CHECK(omcGetStreamPos32(media->stream, &position32));
			remainderBytes = (position32 % pdata->alignment);

			if (remainderBytes != 0)
			{
				omfInt32 padAmount = pdata->alignment - remainderBytes;
				CHECK(omcWriteStream(media->stream, padAmount, 
									 pdata->alignmentBuffer));
			}
		}
		
		/* at this point, the number of lines left to write won't cross a field
		   boundary, so just write that many lines. */
		   
		if (numLinesToWrite)
		{		
			nBytes = pdata->imageWidth * ((pdata->bitsPerPixelAvg + 7) / 8) * numLinesToWrite;
		
			CHECK(omcWriteStream(media->stream, nBytes, 
								 (void *)&(((char *)info->spc.mediaLinesXfer.buf)[bIndex])));
			bIndex += nBytes;
		}
		
		pdata->dataWritten = TRUE;
		
		pdata->numLines += info->spc.mediaLinesXfer.numLines;
		while (pdata->numLines >= (pdata->imageLength * numFields))
		{
			omfsAddInt32toInt64(1, &media->channels[0].numSamples);
			pdata->numLines -= (pdata->imageLength * numFields);
		}
		info->spc.mediaLinesXfer.bytesXfered = bIndex;
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
static omfErr_t codecCDCIReadLines(omfCodecParms_t * info,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfInt32           nBytes, numLinesToNextField, numLinesToRead;
	omfInt32           bIndex = 0;
	userDataCDCI_t * pdata;

	omfAssertMediaHdl(media);
	pdata = (userDataCDCI_t *) media->userData;

	XPROTECT(main)
	{
	
		nBytes = pdata->imageWidth * ((pdata->bitsPerPixelAvg + 7) / 8) *
					 	info->spc.mediaLinesXfer.numLines;
		XASSERT(nBytes <= info->spc.mediaLinesXfer.bufLen, OM_ERR_SMALLBUF);
	
		/* use pdata->numLines % imageLength to work correctly in second field */
		numLinesToNextField = pdata->imageLength - (pdata->numLines % pdata->imageLength);
		numLinesToRead = info->spc.mediaLinesXfer.numLines;
		
		/* If field alignment is specified, see if the number of lines crosses the 
		   field boundary, if so, write whole fields, padding afterward, until the
		   number of lines won't cross a field boundary */
				
		bIndex = 0;


		while ((pdata->alignment > 1) && (numLinesToRead  > numLinesToNextField) )
		{
			omfUInt32 position32;
			omfUInt32 remainderBytes;
			
			nBytes = pdata->imageWidth * ((pdata->bitsPerPixelAvg + 7) / 8) *
					 	numLinesToNextField;
			if (omcReadStream(media->stream, nBytes, 
						  (void *)&(((char *)info->spc.mediaLinesXfer.buf)[bIndex]), NULL) != OM_ERR_NONE)
				RAISE(OM_ERR_NODATA);
			
			bIndex += nBytes;
			numLinesToRead -= numLinesToNextField;
			numLinesToNextField = pdata->imageLength;

			CHECK(omcGetStreamPos32(media->stream, &position32));
			remainderBytes = (position32 % pdata->alignment);

			if (remainderBytes != 0)
			{
				omfInt32 padAmount = pdata->alignment - remainderBytes;
				CHECK(omcSeekStreamRelative(media->stream, padAmount));
			}
		}
		
		if (numLinesToRead)
		{
			nBytes = pdata->imageWidth * ((pdata->bitsPerPixelAvg + 7) / 8) *
					 	numLinesToRead;
			if (omcReadStream(media->stream, nBytes, 
						  (void *)&(((char *)info->spc.mediaLinesXfer.buf)[bIndex]), NULL) != OM_ERR_NONE)
				RAISE(OM_ERR_NODATA);
				
			bIndex += nBytes;
		}

		pdata->numLines += info->spc.mediaLinesXfer.numLines;

		/* leave pdata->numLines MOD the number of lines in a field */
		pdata->numLines = pdata->numLines % pdata->imageLength;
	
		info->spc.mediaLinesXfer.bytesXfered = bIndex;

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
										omfMediaHdl_t 		media,
										omfHdl_t				file)
{
	omfRational_t	aspect;
	char				lineMap[2] = { 0, 0 };
	omcCDCIPersistent_t	*pers;
	omfObject_t		obj = info->spc.initMDES.mdes;
	
	pers = (omcCDCIPersistent_t *)info->pers;
	
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
		
		CHECK(omfsWriteInt32(file, obj, pers->omCDCIComponentWidth, 10));
		CHECK(omfsWriteUInt32(file, obj, pers->omCDCIHorizontalSubsampling, 2));
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
				                      userDataCDCI_t * pdata)
{
	omfVideoMemOp_t 	*vparms;
	omfInt32				RGBFrom, memBitsPerPixel;
	omfPixelFormat_t	pixelFormat;
	
	omfAssertMediaHdl(media);
	
	XPROTECT(file)
	{

		/* validate opcodes individually.  Some don't apply */
		
		pixelFormat = kOmfPixYUV;
		vparms = ((omfVideoMemOp_t *)parmblk->spc.mediaInfo.buf);
		for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
		{
			if(vparms->opcode == kOmfPixelFormat)
				pixelFormat = vparms->operand.expPixelFormat;
		}
		
		vparms = ((omfVideoMemOp_t *)parmblk->spc.mediaInfo.buf);
		for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
		  {
			switch(vparms->opcode)
				{
			  /* put the good ones up front */
				case kOmfRGBCompLayout:
			  case kOmfPixelFormat:
			  case kOmfFrameLayout:		
			  case kOmfFieldDominance:
			  case kOmfStoredRect:
				case kOmfAspectRatio:
			  	
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


static omfErr_t RecomputePixelSampleSize(userDataCDCI_t *pdata, omfInt32 *bytesPerSample, omfInt16 *avgBitsPerPixel)
{
	omfInt32		bitsPerSample, numFields;
	
	XPROTECT(NULL) /* RPS 3/28/96 added to catch exception raised in switch default */
	{
		numFields = ((pdata->fileLayout == kSeparateFields) ||
				 (pdata->fileLayout == kMixedFields) ? 2 : 1);
		switch (pdata->horizontalSubsampling)
		{
		/* 4:4:4 = 1 sample for each of luma and two chroma channels */   
		case 1:
			*avgBitsPerPixel = (omfInt16)((pdata->componentWidth * 3) + pdata->padBits);
			bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
  									(*avgBitsPerPixel) * numFields;
  			break;
  			
  		/* 4:2:2 = two-pixels get 2 samples of luma, 1 of each chroma channel, avg == 2 */							
		case 2:
			*avgBitsPerPixel = (omfInt16)((pdata->componentWidth * 2) + pdata->padBits);					
			bitsPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
  									(*avgBitsPerPixel) * numFields;
			break;
			
		default:
			RAISE(OM_ERR_BADPIXFORM);
		}

		/* NOTE: Samples (frames) are always padded out to a byte boundry.  The 2.0
		 * spec was not clear on this, but the 2.1 spec should state that a frame is
		 * an integral number of bytes, even if the samples within are not an integral
		 * number of bytes
		 */
		*bytesPerSample = (bitsPerSample + 7) / 8;
   }
   XEXCEPT
   XEND;
   
   return(OM_ERR_NONE);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
