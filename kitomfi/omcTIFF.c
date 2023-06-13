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
#include "omcTIFF.h" 
#include "tiff.h"
#include "omJPEG.h" 
#include "omPvt.h"


#define MAX_FMT_OPS	32

static omfErr_t codecGetInfoTIFF(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
			                      omfHdl_t main);

omfErr_t codecPutInfoTIFF(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
			                      omfHdl_t main);
omfErr_t codecNumChannelsTIFF(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
			                      omfHdl_t main);
omfErr_t codecGetSelectInfoTIFF(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
			                      omfHdl_t main);
omfErr_t codecOpenTIFF(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
omfErr_t codecCreateTIFF(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
omfErr_t codecCloseTIFF(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
omfErr_t codecImportRawTIFF(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
omfErr_t codecSemCheckTIFF(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
omfErr_t codecWriteSamplesTIFF(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
omfErr_t codecReadSamplesTIFF(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
omfErr_t codecTIFFReadLines(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
omfErr_t omfmTIFFWriteLines(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
omfErr_t omfmTIFFSetFrameNumber(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);
omfErr_t omfmTIFFGetFrameOffset(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main);


#if ! STANDARD_JPEG_Q
typedef struct
{
	omfProperty_t omFP16QTables;
}		omcTIFFPersistent_t;
#endif

typedef struct
{
	jpeg_tables     compressionTables[3];
	omfJPEGInfo_t   jpeg;
	omfInt32           imageWidth;
	omfInt32           imageLength;
	omfInt32           bytesPerSample;
	omfRational_t   rate;
	omfFrameLayout_t fileLayout;	/* Frame of fields per sample */
	omfPixelFormat_t filePixelFormat;
	omfInt16           bitsPerPixel;	/* Number of bits per pixel */
	omfCompatVideoCompr_t compression;
	omfUInt32         *frameIndex;
	omfUInt32          maxIndex;
	omfUInt32          currentIndex;
	omfInt32           videoOffset;
	omfInt32           leadingLines;
	omfInt32           trailingLines;
	omfInt16           tiffFrameLayout;
	omfInt16           tiffPixelFormat;
	omfInt32           tiffCompressionType;
	omfInt32          numLines;
	omfUInt32          firstIFD;
	omfVideoMemOp_t		fmtOps[MAX_FMT_OPS+1];
	omfVideoMemOp_t		fileFmt[16];
	omfBool				swapBytes; 
	omfBool				customTables;
	omfFieldDom_t	fieldDominance;
	omfBool			codecCompression;
	omfInt32			videoLineMap[2];
}               userDataTIFF_t;	

static omfErr_t readDataShort(omfCodecStream_t *stream, omfUInt16 * data);
static omfErr_t readDataLong(omfCodecStream_t *stream, omfUInt32 * data);
static omfErr_t readDataFrom(omfCodecStream_t *stream, omfInt32 offset, void *buffer, int  bufLength);
static omfErr_t ReadIFD(omfHdl_t main, omfCodecStream_t *stream, userDataTIFF_t *pdata);
static omfErr_t codecIsValidMDESTIFF(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataTIFF_t * pdata);
static omfErr_t codecGetTIFFFieldInfo(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataTIFF_t * pdata);
static omfErr_t codecPutTIFFFieldInfo(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataTIFF_t * pdata);
static omfErr_t codecGetCompressionParmsTIFF(omfCodecParms_t * info, omfHdl_t main, userDataTIFF_t * pdata);
static omfErr_t codecPutCompressionParmsTIFF(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataTIFF_t * pdata);
static omfErr_t AddFrameOffset(omfMediaHdl_t media, omfPosition_t pos);
static omfErr_t CreateTIFFdata_End(omfMediaHdl_t media);
static omfErr_t WriteIFD(omfHdl_t main, omfCodecStream_t *stream, userDataTIFF_t *pdata, omfLength_t numSamples, omfBool has_data);
static omfErr_t Put_TIFFDirEntry(omfHdl_t main, omfCodecStream_t *stream, TIFFDirEntry entry, void *data, omfUInt32 * outOffset);
static omfErr_t codecSetRect(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataTIFF_t * pdata);
static omfErr_t codecSetJPEGTables(omfCodecParms_t * info, omfHdl_t main, userDataTIFF_t * pdata);
static omfErr_t codecGetJPEGTables(omfCodecParms_t * info, omfHdl_t main, userDataTIFF_t * pdata);
static omfErr_t codecGetRect(omfCodecParms_t * info, omfMediaHdl_t media, omfHdl_t main, userDataTIFF_t * pdata);
static omfErr_t ReconcileTIFFInfo(omfMediaHdl_t media, userDataTIFF_t * pdata);
static omfErr_t setupStream(omfCodecStream_t *stream, omfDDefObj_t datakind,
							userDataTIFF_t * pdata, omfBool swapBytes);
static omfErr_t omfmTIFFGetMaxSampleSize(omfCodecParms_t * info, omfMediaHdl_t media,
					                 omfHdl_t main, userDataTIFF_t * pdata, omfInt32 * sampleSize);
static omfErr_t InitTIFFCodec(omfCodecParms_t *parmblk);
static omfErr_t GetFilePixelFormat(omfHdl_t main, userDataTIFF_t * pdata, omfPixelFormat_t * fmt);
static omfErr_t initUserData(userDataTIFF_t *pdata);
static omfErr_t myCodecSetMemoryInfo(omfCodecParms_t * parmblk,
				                      omfMediaHdl_t media,
				                      omfHdl_t file,
				                      userDataTIFF_t * pdata);
static omfErr_t InitMDESProps(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
			                      omfHdl_t main);

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
/* WriteDirEntry is an ease-of-use covering of Put_TIFFDirEntry */

static omfErr_t WriteDirEntry(omfHdl_t main, omfCodecStream_t *stream, omfInt16 tag, omfInt16 type, omfInt32 value)
{
	TIFFDirEntry    entry;
	entry.tdir_tag = tag;
	entry.tdir_type = type;
	entry.tdir_count = 1;
	entry.tdir_offset = value;
	return (Put_TIFFDirEntry(main, stream, entry, NULL, NULL));
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
omfErr_t        omfCodecTIFF(omfCodecFunctions_t func, omfCodecParms_t * parmblk)
{
	omfErr_t			status;
	omfCodecOptDispPtr_t	*dispPtr;
	
	switch (func)
	{
	case kCodecInit:
		InitTIFFCodec(parmblk);
		return (OM_ERR_NONE);
		break;

	case kCodecGetMetaInfo:
		strncpy((char *)parmblk->spc.metaInfo.name, "TIFF Codec (1.5 Compatability)", parmblk->spc.metaInfo.nameLen);
		parmblk->spc.metaInfo.info.mdesClassID = "TIFD";
		parmblk->spc.metaInfo.info.dataClassID = "TIFF";
		parmblk->spc.metaInfo.info.codecID = CODEC_TIFF_VIDEO;
		parmblk->spc.metaInfo.info.rev = CODEC_REV_3;
		parmblk->spc.metaInfo.info.dataKindNameList = PICTUREKIND;
		parmblk->spc.metaInfo.info.minFileRev = kOmfRev1x;
		parmblk->spc.metaInfo.info.maxFileRevIsValid = FALSE;		/* There is no maximum rev */
		return (OM_ERR_NONE);
			
	case kCodecLoadFuncPointers:
		dispPtr = parmblk->spc.loadFuncTbl.dispatchTbl;

		dispPtr[ kCodecOpen ] = codecOpenTIFF;
		dispPtr[ kCodecCreate ] = codecCreateTIFF;
		dispPtr[ kCodecGetInfo ] = codecGetInfoTIFF;
		dispPtr[ kCodecPutInfo ] = codecPutInfoTIFF;
		dispPtr[ kCodecReadSamples ] = codecReadSamplesTIFF;
		dispPtr[ kCodecWriteSamples ] = codecWriteSamplesTIFF;
		dispPtr[ kCodecReadLines ] = codecTIFFReadLines;
		dispPtr[ kCodecWriteLines ] = omfmTIFFWriteLines;
		dispPtr[ kCodecClose ] = codecCloseTIFF;
		dispPtr[ kCodecSetFrame ] = omfmTIFFSetFrameNumber;
		dispPtr[ kCodecGetFrameOffset ] = omfmTIFFGetFrameOffset;
		dispPtr[ kCodecGetNumChannels ] = codecNumChannelsTIFF;
		dispPtr[ kCodecInitMDESProps ] = InitMDESProps;
		dispPtr[ kCodecGetSelectInfo ] = codecGetSelectInfoTIFF;
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

static omfErr_t codecGetInfoTIFF(omfCodecParms_t * parmblk,
			                      omfMediaHdl_t media,
			                      omfHdl_t main)
{
	omfErr_t			status = OM_ERR_NONE;
	omfUInt32			frameSize, frameOff;
	omfFrameSizeParms_t	*singlePtr;
	omfMaxSampleSize_t	*ptr;
	userDataTIFF_t 	*pdata = (userDataTIFF_t *) media->userData;
	
	switch (parmblk->spc.mediaInfo.infoType)
	{
	case kMediaIsContiguous:
		{
		omfType_t	dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
		status = OMIsPropertyContiguous(main, media->dataObj, OMTIFFData,
										dataType, (omfBool *)parmblk->spc.mediaInfo.buf);
		}
		break;
	case kVideoInfo:
		status = codecGetTIFFFieldInfo(parmblk, media, main, pdata);
		break;
	case kCompressionParms:
		status = codecGetCompressionParmsTIFF(parmblk, main, pdata);
		break;
	case kJPEGTables:
		status = codecGetJPEGTables(parmblk, main, pdata);
		break;
	case kMaxSampleSize:
		ptr = (omfMaxSampleSize_t *) parmblk->spc.mediaInfo.buf;
		if(ptr->mediaKind == media->pictureKind)
		{
			status = omfmTIFFGetMaxSampleSize(parmblk, media, main,
								pdata, &ptr->largestSampleSize);
		}
		else
			status = OM_ERR_CODEC_CHANNELS;
		break;
	
	case kSampleSize:
		singlePtr = (omfFrameSizeParms_t *)parmblk->spc.mediaInfo.buf;
		if(pdata->compression == knoComp)
			status = OM_ERR_NOFRAMEINDEX;
		else if(singlePtr->mediaKind == media->pictureKind)
		{
			status = omfsTruncInt64toUInt32(singlePtr->frameNum, &frameOff);	/* OK FRAMECOUNT */
			if(status == OM_ERR_NONE)
			{
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

omfErr_t codecPutInfoTIFF(omfCodecParms_t * parmblk,
			                      omfMediaHdl_t media,
			                      omfHdl_t main)
{
	omfErr_t	status = OM_ERR_NONE;
	userDataTIFF_t 	*pdata = (userDataTIFF_t *) media->userData;
	
	switch (parmblk->spc.mediaInfo.infoType)
	{
	case kVideoInfo:
		status = codecPutTIFFFieldInfo(parmblk, media, main, pdata);
		break;
	case kCompressionParms:
		status = codecPutCompressionParmsTIFF(parmblk, media, main, pdata);
		break;
	case kVideoMemFormat:
		status = myCodecSetMemoryInfo(parmblk, media, main, pdata);
		break;
	case kJPEGTables:
		status = codecSetJPEGTables(parmblk, main, pdata);
		break;

	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}

	return(status);
}

omfErr_t codecNumChannelsTIFF(omfCodecParms_t * parmblk,
			                      omfMediaHdl_t media,
			                      omfHdl_t main)
{
	omfErr_t		status = OM_ERR_NONE;
	omfDDefObj_t	dataKind;
	
	omfiDatakindLookup(parmblk->spc.getChannels.file, PICTUREKIND, &dataKind, &status);
	if (parmblk->spc.getChannels.mediaKind == dataKind)
		parmblk->spc.getChannels.numCh = 1;
	else
		parmblk->spc.getChannels.numCh = 0;

	return(status);
}

omfErr_t codecGetSelectInfoTIFF(omfCodecParms_t * parmblk,
			                      omfMediaHdl_t media,
			                      omfHdl_t main)
{
	omfErr_t		status = OM_ERR_NONE;
	omfInt64        ifdPosition;
    omfCodecStream_t streamData;
    omfType_t        dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
    userDataTIFF_t  pdataBlock;
	omfPosition_t	zeroPos;
	omcTIFFPersistent_t *pers = (omcTIFFPersistent_t *)parmblk->pers;
    omfHdl_t		file;
    omfUInt32		ifdShort;
	omfUInt16		byteOrder;
	omfCStrmFmt_t   fileFmt;
	omfVideoMemOp_t	fmtOps[1];
	omfBool			swapBytes;
	
#ifdef OMFI_ENABLE_STREAM_CACHE
	streamData.cachePtr = NULL;
#endif
	streamData.procData = NULL;

	file = parmblk->spc.selectInfo.file;
    XPROTECT(file)
    {
		omfsCvtInt32toPosition(0, zeroPos);
		parmblk->spc.selectInfo.info.willHandleMDES = TRUE;
		parmblk->spc.selectInfo.info.isNative = TRUE;
		parmblk->spc.selectInfo.info.hwAssisted = FALSE;
		/***/

		if(omfsIsPropPresent(file, parmblk->spc.selectInfo.mdes, OMTIFDSummary, dataType))
		{
			CHECK(omcOpenStream(file, file, &streamData, parmblk->spc.selectInfo.mdes,
													OMTIFDSummary, dataType));
			CHECK(readDataShort(&streamData, &byteOrder));
			swapBytes = (byteOrder != OMNativeByteOrder);
			fmtOps[0].opcode = kOmfVFmtEnd;
			fileFmt.mediaKind = file->pictureKind;
			fileFmt.afmt = NULL;
			fileFmt.vfmt = fmtOps;
			CHECK(omcSetFileFormat(&streamData, fileFmt, swapBytes));
		
			CHECK(omcSeekStream32(&streamData, 4));
			/* go to the IFD */
			CHECK(readDataLong(&streamData, &ifdShort));
			omfsCvtInt32toInt64(ifdShort, &ifdPosition);
			CHECK(omcSeekStreamTo(&streamData, ifdPosition));
		
			CHECK(initUserData(&pdataBlock));
			ReadIFD(file, &streamData, &pdataBlock);
			if(parmblk->fileRev == kOmfRev2x)
			  {
				CHECK(omfsReadRational(file, parmblk->spc.selectInfo.mdes, OMMDFLSampleRate, &(pdataBlock.rate)));
			  }
			else
			  {
				CHECK(omfsReadExactEditRate(file, parmblk->spc.selectInfo.mdes, OMMDFLSampleRate, &(pdataBlock.rate)));
			  }
			if(pdataBlock.rate.denominator != 0)
			  {
				parmblk->spc.selectInfo.info.avgBitsPerSec = (pdataBlock.bytesPerSample *
															pdataBlock.rate.numerator) /
															pdataBlock.rate.denominator;
			  }
			else
			  {
				parmblk->spc.selectInfo.info.avgBitsPerSec = 0;
			  }
			if(pdataBlock.compression == knoComp)
				parmblk->spc.selectInfo.info.relativeLoss = 0;
			else
				parmblk->spc.selectInfo.info.relativeLoss = 10;
			CHECK(omcCloseStream(&streamData));
		}
		else
		{
			parmblk->spc.selectInfo.info.avgBitsPerSec = 0;
			parmblk->spc.selectInfo.info.relativeLoss = 0;
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
static omfErr_t InitMDESProps(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
			                      omfHdl_t main)
{
	omfObject_t		obj = info->spc.initMDES.mdes;
	omfType_t		dataType = (info->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
	TIFFHeader      hdr;
	userDataTIFF_t	initialPData;
    omfCodecStream_t streamData;
    omfLength_t		zero;
#ifdef OMFI_ENABLE_STREAM_CACHE
	streamData.cachePtr = NULL;
#endif
	streamData.procData = NULL;
	omfsCvtInt32toInt64(0, &zero);
	
	XPROTECT(main)
	{
		CHECK(omfsWriteBoolean(main, obj, OMTIFDIsContiguous, FALSE));
		CHECK(omfsWriteBoolean(main, obj, OMTIFDIsUniform, FALSE));
		CHECK(omcOpenStream(main, main, &streamData,
							  info->spc.initMDES.mdes, OMTIFDSummary, dataType));
		hdr.tiff_magic = OMNativeByteOrder;
		hdr.tiff_version = TIFF_VERSION;
		hdr.tiff_diroff = 0; /* Stop an uninitialized memory reference */
		CHECK(omcWriteStream(&streamData, sizeof(TIFFHeader), &hdr));
		CHECK(initUserData(&initialPData));
		CHECK(WriteIFD(main, &streamData, &initialPData, zero, FALSE));
		CHECK(omcCloseStream(&streamData));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * InitTIFFCodec
 *
 * 		Initializes values that need to live across all codec invocations
 *
 * Argument Notes:
 *		Allocate a persistent memory block, and store the address in the parameter
 *      block pointer.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfErr_t InitTIFFCodec(omfCodecParms_t *parmblk)
{	
#if ! STANDARD_JPEG_Q
	omcTIFFPersistent_t *persist;
	omfHdl_t			file = NULL;
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);
	/* first, allocate a persistent "cookie" that will hold codec-dynamic 
	 * values, like property and type handles 
	 */
	   
	XPROTECT(NULL)
	{
		persist = (omcTIFFPersistent_t *)
		  omOptMalloc(NULL, sizeof(omcTIFFPersistent_t));
		XASSERT(persist != NULL, OM_ERR_NOMEMORY);
	
		/* put the cookie pointer in the parameter block for return */
		parmblk->spc.init.persistRtn = persist;
						
		/* add the codec-related properties.  See the OMF Interchange 
		 * Specification 2.0 for details.
		 */
		CHECK(omfsRegisterDynamicProp(parmblk->spc.init.session, kOmfTstRev1x,
						"FP16QTables", OMClassTIFD, 
					   OMVarLenBytes, kPropOptional, &persist->omFP16QTables));
		CHECK(omfsRegisterProp(parmblk->spc.init.session, persist->omFP16QTables,
						kOmfTstRev2x,
						"FP16QTables", OMClassTIFD, 
					   OMDataValue, kPropOptional));
					   
	}
	XEXCEPT
	XEND
#else
	parmblk->spc.init.persistRtn = NULL;

#endif

	return(OM_ERR_NONE);
}

/************************
 * codecOpenTIFF
 *
 * 		Opens an existing TIFF media stream
 *
 * Argument Notes:
 *		Media descriptor and data object are passed in via "media".
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t codecOpenTIFF(omfCodecParms_t * info,
			                      omfMediaHdl_t media,
			                      omfHdl_t main)
{
	omfInt64          ifdPosition;
	omfUInt32          ifdShort;
	omfUInt16          byteOrder, tiffVersion;
	omfCStrmFmt_t   fileFmt;
	userDataTIFF_t 	*pdata = (userDataTIFF_t *) media->userData;
	omfType_t		dataType;
	omfInt32		numFields;
	
	/* pull out descriptor information BEFORE reading TIFF data */
	XPROTECT(main)
	{
		media->pvt->pixType = kOmfPixRGBA;
		media->userData = (userDataTIFF_t *) omOptMalloc(main, sizeof(userDataTIFF_t));
		if(media->userData == NULL)
		{
			RAISE(OM_ERR_NOMEMORY);
		}
		pdata = (userDataTIFF_t *) media->userData;
		CHECK(initUserData(pdata));		/* Set to some reasonable default values */
		pdata->codecCompression = (media->compEnable == kToolkitCompressionEnable);
		CHECK(ReconcileTIFFInfo(media, pdata));
		
		pdata->fmtOps[0].opcode = kOmfVFmtEnd;
	
		if(info->fileRev == kOmfRev2x)
		{
			CHECK(omfsReadRational(main, media->mdes, OMMDFLSampleRate, &(pdata->rate)));
			dataType = OMDataValue;
		}
		else
		{
			CHECK(omfsReadExactEditRate(main, media->mdes, OMMDFLSampleRate, &(pdata->rate)));
			dataType = OMVarLenBytes;
		}
		CHECK(omfsReadLength(main, media->mdes, OMMDFLLength,
							&media->channels[0].numSamples));
		
		media->numChannels = 1;
	
#if CHECK_TIFF_SUMMARY
		{
		char		debugBuf[16];
		omfUInt32	rbytes;
		
		CHECK(omcOpenStream(main, main, media->stream, media->mdes, OMTIFDSummary, dataType));
		CHECK(omcReadStream(media->stream, sizeof(debugBuf), debugBuf, &rbytes));
		CHECK(omcCloseStream(media->stream));
		}
#endif
		CHECK(omcOpenStream(main, media->dataFile, media->stream, media->dataObj, OMTIFFData, dataType));
	
		CHECK(readDataShort(media->stream, &byteOrder));
		pdata->swapBytes = (byteOrder != OMNativeByteOrder);
		fileFmt.mediaKind = media->pictureKind;
		fileFmt.afmt = NULL;
		fileFmt.vfmt = pdata->fmtOps;
		CHECK(omcSetFileFormat(media->stream, fileFmt, pdata->swapBytes));
		
		CHECK(readDataShort(media->stream, &tiffVersion));
		if (tiffVersion != TIFF_VERSION)
			RAISE(OM_ERR_TIFFVERSION);
	
		pdata->numLines = 0;
	
		/* go to the IFD */
		CHECK(readDataLong(media->stream, &ifdShort));
		omfsCvtInt32toInt64(ifdShort, &ifdPosition);
		CHECK(omcSeekStreamTo(media->stream, ifdPosition));
	
		CHECK(ReadIFD(main, media->stream, pdata));
	
		if (omfsReadJPEGTableIDType(main, media->mdes, OMTIFDJPEGTableID, &pdata->jpeg.JPEGTableID) != OM_ERR_NONE)
		{
			pdata->jpeg.JPEGTableID = 0;
		}

		/* The leading/trailing lines will always be 13/16 for CCIR in TIFF.  We don't
		 * need to handle the 5/8 case, because JFIF can't be embedded into TIFF
		 */
		if(pdata->jpeg.JPEGTableID != 0 && pdata->imageWidth > 640)
		{
			omfVideoSignalType_t	signal;
			CHECK(omfmSourceGetVideoSignalType(media, &signal));
			if(signal == kPALSignal)
			{
				pdata->leadingLines = 16;
			}
			else
			{
				pdata->leadingLines = 13;
			}
		}
		else
		{
			if (omfsReadInt32(main, media->mdes, OMTIFDLeadingLines, &pdata->leadingLines) != OM_ERR_NONE)
			{
				pdata->leadingLines = 0;
			}
		}
		if (omfsReadInt32(main, media->mdes, OMTIFDTrailingLines,
			      &pdata->trailingLines) != OM_ERR_NONE)
		{
			pdata->trailingLines = 0;
		}
		numFields = (pdata->tiffFrameLayout == FRAMELAYOUT_MIXEDFIELDS ? 2 : 1);
		pdata->bytesPerSample = (omfInt32) pdata->imageWidth * (omfInt32) pdata->imageLength *
			(omfInt32) (pdata->bitsPerPixel / 8) * numFields;

#if ! STANDARD_JPEG_Q
		if(pdata->compression == kJPEG)
		{
			omfPosition_t      offset;
			omfErr_t			status = OM_ERR_NONE;
			omfLength_t			tableLen;
			omfInt32			tableLen32;
			omcTIFFPersistent_t *persist = (omcTIFFPersistent_t *)info->pers;
			
			XASSERT(persist != NULL, OM_ERR_BADQTABLE);

			CHECK(omfsCvtInt32toInt64(0, &offset));
			
			if(info->fileRev == kOmfRev2x)
				tableLen = omfsLengthDataValue(main, media->mdes, persist->omFP16QTables);
			else
				tableLen = omfsLengthVarLenBytes(main, media->mdes, persist->omFP16QTables);
			CHECK(omfsTruncInt64toInt32(tableLen, &tableLen32));	/* OK BOUNDS */
			if (tableLen32 == 256)
			{
				omfInt32 i;
				omfBool swab;
				
				swab = (main->byteOrder != OMNativeByteOrder);
				for (i = 0; i < 64; i++)
				{
				    /* LF-W: 12/9/96 changed kSwabIfNeeded to kNeverSwab */
					status = OMReadProp(main, media->mdes, 
										persist->omFP16QTables, offset, 
										kNeverSwab, dataType, sizeof(omfUInt16), 
										(void *)&pdata->compressionTables[0].Q16FP[i]);
					if (status != OM_ERR_NONE)
						goto recover;
					if (swab)
					  omfsFixShort((short *)(&pdata->compressionTables[0].Q16FP[i]));
					CHECK(omfsAddInt32toInt64(sizeof(omfUInt16), &offset));
				}
				for (i = 0; i < 64; i++)
				{
					status = OMReadProp(main, media->mdes, 
										persist->omFP16QTables, offset, 
										kNeverSwab, dataType, sizeof(omfUInt16), 
										(void *)&pdata->compressionTables[1].Q16FP[i]);
					if (status != OM_ERR_NONE)
						goto recover;
					if (swab)
					  omfsFixShort((short *)(&pdata->compressionTables[1].Q16FP[i]));
					CHECK(omfsAddInt32toInt64(sizeof(omfUInt16), &offset));
				}
				pdata->compressionTables[0].Q16FPlen = 64;
				pdata->compressionTables[1].Q16FPlen = 64;
			}
recover:
			status = status; /* Need SOME line to hold place here */
		}
#endif		

	#ifdef JPEG_TRACE
		fprintf(stderr, "JPEGtableID (import) = %1ld\n", pdata->jpeg.JPEGTableID);
	#endif
		CHECK(GetFilePixelFormat(main, pdata, &pdata->filePixelFormat));

		CHECK(ReconcileTIFFInfo(media, pdata));
		CHECK(setupStream(media->stream, media->pictureKind, pdata, pdata->swapBytes));
	
		omfsCvtInt32toInt64(pdata->videoOffset, &media->channels[0].dataOffset);	/* prepare for data
								 * read, skip IFH */
		CHECK(omcSeekStreamTo(media->stream, media->channels[0].dataOffset));
		
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
omfErr_t codecCreateTIFF(omfCodecParms_t * info,
								omfMediaHdl_t media,
				                omfHdl_t main)
{
	TIFFHeader      hdr;
	omfInt16           n;
	userDataTIFF_t *pdata;
	omfErr_t		status;
	omfUID_t		uid;
	jpeg_tables		*destPtr;
	omfType_t		dataType;
	omfPosition_t	frameOffset;
	
	/* pull out descriptor information BEFORE reading TIFF data */
	XPROTECT(main)
	{
		media->pvt->pixType = kOmfPixRGBA;
		media->userData = (userDataTIFF_t *) omOptMalloc(main, sizeof(userDataTIFF_t));
		if(media->userData == NULL)
		{
			RAISE(OM_ERR_NOMEMORY);
		}
		pdata = (userDataTIFF_t *) media->userData;
		CHECK(initUserData(pdata));		/* Set to some reasonable default values */
		pdata->codecCompression = (media->compEnable == kToolkitCompressionEnable);
	
		omfsCvtInt32toInt64(0, &media->channels[0].numSamples);

		CHECK(omfsReadUID(main, media->fileMob, OMMOBJMobID, &uid));
		if((info->fileRev == kOmfRev1x) || (info->fileRev == kOmfRevIMA))
		{
			CHECK(omfsWriteUID(main, media->dataObj, OMTIFFMobID, uid));
			status = omfsReadExactEditRate(main, media->mdes, OMMDFLSampleRate,
				   			  &(pdata->rate));
			if (status != OM_ERR_NONE)
				RAISE(OM_ERR_DESCSAMPRATE);
			if (omfsReadLength(main, media->mdes, OMMDFLLength, &media->channels[0].numSamples) != OM_ERR_NONE)
				RAISE(OM_ERR_DESCLENGTH);
			dataType = OMVarLenBytes;
		}
		else
		{
			CHECK(omfsWriteUID(main, media->dataObj, OMMDATMobID, uid));
			status = omfsReadRational(main, media->mdes, OMMDFLSampleRate, &(pdata->rate));
			if (status != OM_ERR_NONE)
				RAISE(OM_ERR_DESCSAMPRATE);
			CHECK(omfsReadLength(main, media->mdes, OMMDFLLength,
							&media->channels[0].numSamples));
			dataType = OMDataValue;
		}
		
		/* 
		   software compression is specified when media object is created 
		   this assignment must be made before ReconcileTIFFInfo!
		 */
		pdata->compression = (media->compEnable == kToolkitCompressionEnable) ? kJPEG : knoComp;
	
		CHECK(ReconcileTIFFInfo(media, pdata));

		for (n = 0; n < 3; n++)
		{

			destPtr = &pdata->compressionTables[n];
			destPtr->Qlen = 0;
			destPtr->Q16len = 0;
			destPtr->DClen = 0;
			destPtr->AClen = 0;
		}
		
		pdata->videoOffset = sizeof(TIFFHeader);
		pdata->firstIFD = 0;
		pdata->fmtOps[0].opcode = kOmfVFmtEnd;
		
		media->numChannels = 1;
	
		CHECK(omcOpenStream(main, media->dataFile, media->stream, media->dataObj, OMTIFFData, dataType));
		CHECK(setupStream(media->stream, media->pictureKind, pdata, FALSE));
	
		hdr.tiff_magic = OMNativeByteOrder;
		hdr.tiff_version = TIFF_VERSION;
		hdr.tiff_diroff = 0; /* Stop an uninitialized memory reference */

		CHECK(omcWriteStream(media->stream, sizeof(TIFFHeader), &hdr));
		/*
		 * Create the first entry of the frameIndex, even if not JPEG. Don't
		 * write this out if not JPEG
		 */
		
		CHECK(omcGetStreamPosition(media->stream, &frameOffset));	
		CHECK(omfsSubInt32fromInt64(8, &frameOffset));	/* minus 8 for IFH */
		CHECK(AddFrameOffset(media, frameOffset));
		CHECK(omcSeekStream32(media->stream, 8));	/* prepare for data
								 * write, skip IFH */
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
omfErr_t codecCloseTIFF(omfCodecParms_t * info, 
								omfMediaHdl_t media,
			                       omfHdl_t main)
{	
  userDataTIFF_t *pdata = (userDataTIFF_t *) media->userData;
  TIFFHeader      hdr;
  omfUInt32	numSamples;
  omfType_t			dataType = (info->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
	omfProperty_t	summary = OMTIFDSummary;
	
  XPROTECT(main)
	omfBool			isContig;
	
	{
		if((media->openType == kOmfiCreated) || (media->openType == kOmfiAppended))
		{
		  CHECK(CreateTIFFdata_End(media));
  		}
	  CHECK(omcCloseStream(media->stream));           
          
	  if (media->openType == kOmfiCreated)    /* fill in summary information */
		{
		  CHECK(omcOpenStream(main, main, media->stream,
							  media->mdes, summary, dataType));
		  hdr.tiff_magic = OMNativeByteOrder;
		  hdr.tiff_version = TIFF_VERSION;
		  hdr.tiff_diroff = 0; /* Stop an uninitialized memory reference */
		  CHECK(omcWriteStream(media->stream, sizeof(TIFFHeader), &hdr));
		  CHECK(WriteIFD(main, media->stream, pdata, media->channels[0].numSamples, FALSE));
		  CHECK(omcCloseStream(media->stream));
		  CHECK(omfsWriteLength(main, media->mdes, OMMDFLLength, media->channels[0].numSamples));

		  CHECK(omfmIsMediaContiguous(media, &isContig));
		  CHECK(omfsWriteBoolean(main,media->mdes, 
								OMTIFDIsContiguous, isContig));
								
		  if (omfsTruncInt64toUInt32(media->channels[0].numSamples, &numSamples) == OM_ERR_NONE)	/* OK HANDLED */
		  {
			CHECK(omfsWriteBoolean(main, media->mdes, OMTIFDIsUniform, 
								   (omfBool)(numSamples == 1)));
		  } else
		  {
			CHECK(omfsWriteBoolean(main, media->mdes, OMTIFDIsUniform, FALSE));
		  }
			omfsWriteInt32(main, media->mdes, OMTIFDLeadingLines, pdata->leadingLines);
			omfsWriteInt32(main, media->mdes, OMTIFDTrailingLines,
			      pdata->trailingLines);
			CHECK(omfsWriteJPEGTableIDType(main, media->mdes,
			       OMTIFDJPEGTableID, pdata->jpeg.JPEGTableID));
#if ! STANDARD_JPEG_Q
			if (pdata->compressionTables[0].Q16FPlen)
			{
				omfPosition_t      offset;
				omcTIFFPersistent_t *persist = (omcTIFFPersistent_t *)info->pers;
			
				XASSERT(persist != NULL, OM_ERR_BADQTABLE);
	
				CHECK(omfsCvtInt32toInt64(0, &offset));
	
				CHECK(OMWriteProp(main, media->mdes, persist->omFP16QTables, offset,
									dataType,128L,
									(void *)(pdata->compressionTables[0].Q16FP)));
									
				CHECK(omfsAddInt32toInt64(128, &offset));
				
				CHECK(OMWriteProp(main, media->mdes, persist->omFP16QTables, offset,
									dataType,128L,
									(void *)(pdata->compressionTables[1].Q16FP)));
				
			}
#endif		
		}
	  if(pdata->frameIndex != NULL)
		omOptFree(main, pdata->frameIndex);
	  omOptFree(main, pdata);
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
static omfErr_t GetFilePixelFormat(omfHdl_t main, userDataTIFF_t * pdata, omfPixelFormat_t * fmt)
{
	omfPixelFormat_t format;

	XPROTECT(main)
	{
		switch (pdata->tiffPixelFormat)
		{
		case PHOTOMETRIC_RGB:	/* only allow 24-bit RGB  */
			if (pdata->bitsPerPixel == 24)
				format = kOmfPixRGBA;
			else
				RAISE(OM_ERR_BADPIXFORM);
			break;
	
		case PHOTOMETRIC_YCBCR:
			/* if decompressing JPEG, output will be RGB */
			if ((pdata->compression == kJPEG || pdata->compression == kLSIJPEG)
			  && pdata->codecCompression)
				format = kOmfPixRGBA;
			else
				format = kOmfPixYUV;
			break;
	
		default:
			RAISE(OM_ERR_BADPIXFORM);
		}

	}
	XEXCEPT
	XEND

	*fmt = format;
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
static omfErr_t codecGetTIFFFieldInfo(omfCodecParms_t * info,
				                      omfMediaHdl_t media,
				                      omfHdl_t main,
				                      userDataTIFF_t * pdata)
{
	omfVideoMemOp_t 	*vparms;
	omfPixelFormat_t	format;
	
	XPROTECT(main)
	{
		vparms = ((omfVideoMemOp_t *)info->spc.mediaInfo.buf);
		for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
		{
			switch(vparms->opcode)
			{
			case kOmfPixelFormat:
				CHECK(GetFilePixelFormat(main, pdata, &format));
				vparms->operand.expPixelFormat = format;
				break;
			case kOmfFrameLayout:
				vparms->operand.expFrameLayout = pdata->fileLayout;
				break;
	
			case kOmfFieldDominance:
				vparms->operand.expFieldDom = pdata->fieldDominance;
				break;
	
			case kOmfDisplayRect:
				vparms->operand.expRect.xOffset = 0;
				vparms->operand.expRect.yOffset = 0;
				vparms->operand.expRect.xSize = pdata->imageWidth;
				vparms->operand.expRect.yOffset = pdata->leadingLines;
				vparms->operand.expRect.ySize = pdata->imageLength -
							pdata->leadingLines - pdata->trailingLines;
				break;

			case kOmfSampledRect:
				vparms->operand.expRect.xOffset = 0;
				vparms->operand.expRect.yOffset = 0;
				vparms->operand.expRect.xSize = pdata->imageWidth;
				vparms->operand.expRect.ySize = pdata->imageLength;
				break;

			case kOmfStoredRect:
				vparms->operand.expRect.xOffset = 0;
				vparms->operand.expRect.yOffset = 0;
				vparms->operand.expRect.xSize = pdata->imageWidth;
				vparms->operand.expRect.ySize = pdata->imageLength;
				break;
			case kOmfPixelSize:
				vparms->operand.expInt16 = pdata->bitsPerPixel;
				break;
			case kOmfAspectRatio:
				RAISE(OM_ERR_INVALID_OP_CODEC);
				break;
			
			case kOmfRGBCompLayout:
				CHECK(GetFilePixelFormat(main, pdata, &format));
				if (format == kOmfPixRGBA)
				{	
					vparms->operand.expCompArray[0] = 'R';
					vparms->operand.expCompArray[1] = 'G';
					vparms->operand.expCompArray[2] = 'B';
					vparms->operand.expCompArray[3] = 0; /* terminating zero */
				}
				else
					RAISE(OM_ERR_INVALID_OP_CODEC);

				break;
	
			case kOmfRGBCompSizes:
				CHECK(GetFilePixelFormat(main, pdata, &format));
				if (format == kOmfPixRGBA)
				{	
					vparms->operand.expCompSizeArray[0] = 8;
					vparms->operand.expCompSizeArray[1] = 8;
					vparms->operand.expCompSizeArray[2] = 8;
					vparms->operand.expCompSizeArray[3] = 0; /* terminating zero */
				}
				else
					RAISE(OM_ERR_INVALID_OP_CODEC);

				break;
	
			case kOmfWillTransferLines:
				vparms->operand.expBoolean = (pdata->compression == knoComp);
				break;
			case kOmfIsCompressed:
				vparms->operand.expBoolean = (pdata->compression != knoComp);;
				break;
				
			default:
				RAISE(OM_ERR_INVALID_OP_CODEC);
				
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
static omfErr_t codecPutTIFFFieldInfo(omfCodecParms_t * info,
				                      omfMediaHdl_t media,
				                      omfHdl_t main,
				                      userDataTIFF_t * pdata)
{
	omfInt16     		numFields;
	omfVideoMemOp_t 	*vparms;

	XPROTECT(main)
	{
		vparms = (omfVideoMemOp_t*)info->spc.mediaInfo.buf;
		for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
		{
			switch(vparms->opcode)
			{
			case kOmfPixelFormat:
				pdata->filePixelFormat = vparms->operand.expPixelFormat;
				break;
			case kOmfFrameLayout:
				if(vparms->operand.expFrameLayout == kMixedFields)
					RAISE(OM_ERR_INVALID_OP_CODEC);
				pdata->fileLayout = vparms->operand.expFrameLayout;
				break;
	
			case kOmfFieldDominance:
				pdata->fieldDominance = vparms->operand.expFieldDom;
				break;
	
			case kOmfVideoLineMap:
			 	pdata->videoLineMap[0] = vparms->operand.expLineMap[0];
			 	pdata->videoLineMap[1] = vparms->operand.expLineMap[1];
			  	break;
				  	
			case kOmfSampledRect:
				if ((vparms->operand.expRect.xOffset != 0) ||
				    (vparms->operand.expRect.xSize != pdata->imageWidth))
					RAISE(OM_ERR_INVALID_OP_CODEC);
				/* Don't allow this size to change at all */
				if ((vparms->operand.expRect.yOffset != 0) ||
					(vparms->operand.expRect.ySize != pdata->imageLength))
					RAISE(OM_ERR_INVALID_OP_CODEC);
				break;

			case kOmfDisplayRect:
				if ((vparms->operand.expRect.xOffset != 0) ||
				    (vparms->operand.expRect.xSize != pdata->imageWidth))
					RAISE(OM_ERR_INVALID_OP_CODEC);
				if ((vparms->operand.expRect.yOffset < 0) ||
				    (vparms->operand.expRect.yOffset + vparms->operand.expRect.ySize
				    > pdata->imageLength))
					RAISE(OM_ERR_INVALID_OP_CODEC);
				pdata->leadingLines = (omfInt32)vparms->operand.expRect.yOffset;
				pdata->trailingLines = (omfInt32)(pdata->imageLength -
							(vparms->operand.expRect.yOffset +
							vparms->operand.expRect.ySize));
				break;

			case kOmfStoredRect:
				pdata->imageWidth = vparms->operand.expRect.xSize;
				pdata->imageLength = vparms->operand.expRect.ySize;
				break;
				
			case kOmfPixelSize:
				pdata->bitsPerPixel = vparms->operand.expInt16;
				break;
				
			case kOmfAspectRatio:
				break;

			default:
				RAISE(OM_ERR_INVALID_OP_CODEC);
			}
		}

		numFields = ((pdata->fileLayout == kSeparateFields) ? 2 : 1);
		/* filePixelFormat is FIXED for compressed data */
	
		CHECK(ReconcileTIFFInfo(media, pdata));
	
		pdata->bytesPerSample = (omfInt32)pdata->imageWidth * 
	                        (omfInt32)pdata->imageLength *
							(omfInt32)(pdata->bitsPerPixel / 8) * numFields;

		CHECK(setupStream(media->stream, media->pictureKind, pdata, pdata->swapBytes));
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
omfErr_t omfmTIFFSetFrameNumber(omfCodecParms_t * info,
				                       omfMediaHdl_t media,
				                       omfHdl_t main)
{
	omfUInt32          frameNumber;
	omfInt64          nBytes;
	userDataTIFF_t 	*pdata = (userDataTIFF_t *) media->userData;

	omfAssert(media->dataObj, main, OM_ERR_INTERNAL_MDO);
	XPROTECT(main)
	{
		CHECK(omfsTruncInt64toUInt32(info->spc.setFrame.frameNumber, &frameNumber));	/* OK FRAMEOFFSET */
		XASSERT(frameNumber != 0, OM_ERR_BADFRAMEOFFSET);

		if (pdata->tiffCompressionType == COMPRESSION_JPEG)
		{
			XASSERT(pdata->frameIndex != NULL, OM_ERR_NOFRAMEINDEX);
			XASSERT((0 < frameNumber && frameNumber - 1 < pdata->maxIndex),
					OM_ERR_BADFRAMEOFFSET);
	
			pdata->currentIndex = frameNumber - 1;
			CHECK(omcSeekStream32(media->stream, pdata->frameIndex[pdata->currentIndex] + 8));
		} else
		{
			omfsCvtInt32toInt64(frameNumber - 1, &nBytes);
			CHECK(omfsMultInt32byInt64(pdata->bytesPerSample, nBytes, &nBytes));
			CHECK(omfsAddInt32toInt64(sizeof(TIFFHeader), &nBytes));
			CHECK(omcSeekStreamTo(media->stream, nBytes));
		}
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}


/************************
 * omfmTIFFGetFrameOffset
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
omfErr_t omfmTIFFGetFrameOffset(omfCodecParms_t * info,
				                       omfMediaHdl_t media,
				                       omfHdl_t main)
{
	omfUInt32          frameNumber;
	omfInt64          nBytes;
	userDataTIFF_t 	*pdata = (userDataTIFF_t *) media->userData;

	omfAssert(media->dataObj, main, OM_ERR_INTERNAL_MDO);
	XPROTECT(main)
	{
		CHECK(omfsTruncInt64toUInt32(info->spc.getFrameOffset.frameNumber, &frameNumber));	/* OK FRAMEOFFSET */
		XASSERT(frameNumber != 0, OM_ERR_BADFRAMEOFFSET);

		if (pdata->tiffCompressionType == COMPRESSION_JPEG)
		{
			XASSERT(pdata->frameIndex != NULL, OM_ERR_NOFRAMEINDEX);
			XASSERT((0 < frameNumber && frameNumber - 1 < pdata->maxIndex),
					OM_ERR_BADFRAMEOFFSET);
	
			omfsCvtInt32toInt64(pdata->frameIndex[frameNumber - 1],
								&info->spc.getFrameOffset.frameOffset);
			
		} else
		{
			omfsCvtInt32toInt64(frameNumber - 1, &nBytes);
			CHECK(omfsMultInt32byInt64(pdata->bytesPerSample, nBytes, &nBytes));
			CHECK(omfsAddInt32toInt64(sizeof(TIFFHeader), &nBytes));

			info->spc.getFrameOffset.frameOffset = nBytes;
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
static omfErr_t codecSetJPEGTables(omfCodecParms_t * info,
				                   omfHdl_t main,
				                   userDataTIFF_t * pdata)
{
	omfJPEGTables_t *tables = (omfJPEGTables_t *) info->spc.mediaInfo.buf;
	omfInt32           compIndex, i;
	omfInt16			Qtabsize;

#if ! STANDARD_JPEG_Q
	omfBool 			extendedQ = FALSE;
#endif

	XPROTECT(main)
	{
		if (info->spc.mediaInfo.bufLen != sizeof(omfJPEGTables_t))
			RAISE(OM_ERR_INTERN_TOO_SMALL);
			
		if (tables->JPEGcomp != kJcLuminance && tables->JPEGcomp != kJcChrominance)
			RAISE(OM_ERR_JPEGPROBLEM);
	
		compIndex = (tables->JPEGcomp == kJcLuminance ? 0 : 1);
		pdata->customTables = TRUE;
		
		Qtabsize = tables->QTableSize;
		switch (Qtabsize) {
		case 64:
#if ! STANDARD_JPEG_Q
			extendedQ = FALSE;
			pdata->compressionTables[compIndex].Q16FPlen = 0;
#endif
			break;
		
#if ! STANDARD_JPEG_Q
		case 128:
			extendedQ = TRUE;
			Qtabsize = 64;
			pdata->compressionTables[compIndex].Q16FPlen = 64;

			break;
#endif
			
		default:
			RAISE(OM_ERR_BADQTABLE);
		}

		pdata->compressionTables[compIndex].Qlen = tables->QTableSize;
		pdata->compressionTables[compIndex].Q16len = tables->QTableSize;
		pdata->compressionTables[compIndex].DClen = tables->DCTableSize;
		pdata->compressionTables[compIndex].AClen = tables->ACTableSize;
		for (i = 0; i < Qtabsize; i++)
		{
#if ! STANDARD_JPEG_Q
			if (extendedQ)
			{
				omfUInt16 tval, rval, rem;
				tval = ((omfUInt16 *)(tables->QTables))[i];
				rval = tval >> 7;
				rem = rval & 1;
				rval = (rval >> 1) + rem; /* round the value, don't simply trunc */
		  		pdata->compressionTables[compIndex].Q16FP[i] = tval;
		  		pdata->compressionTables[compIndex].Q[i] = (omfUInt8)rval;
		  		pdata->compressionTables[compIndex].Q16[i] = rval;
			}
			else
#endif
			{
				pdata->compressionTables[compIndex].Q[i] = tables->QTables[i];
				pdata->compressionTables[compIndex].Q16[i] = tables->QTables[i];
			}
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
				                   userDataTIFF_t * pdata)
{
	omfJPEGTables_t *tables = (omfJPEGTables_t *) info->spc.mediaInfo.buf;
	omfInt32           compIndex, i;
	omfUInt8           *Qptr;
#if ! STANDARD_JPEG_Q
	omfBool				extendedQ = FALSE;
#endif

	XPROTECT(main)
	{
		if(pdata->compression == kLSIJPEG)
			RAISE(OM_ERR_NOT_IMPLEMENTED);			/* (SPR#345) S/B able to get this */
		if (info->spc.mediaInfo.bufLen != sizeof(omfJPEGTables_t))
			RAISE(OM_ERR_INTERN_TOO_SMALL);
		
		switch (tables->JPEGcomp) {
		case kJcLuminance:
			compIndex = 0;
			break;
			
		case kJcChrominance:
			compIndex = 1;
			break;
			
#if ! STANDARD_JPEG_Q
		case kJcLuminanceFP16:
			compIndex = 0;
			if (! pdata->compressionTables[compIndex].Q16FPlen)
				RAISE(OM_ERR_BADQTABLE);
			extendedQ = TRUE;			
			break;

		case kJcChrominanceFP16:
			compIndex = 1;
			if (! pdata->compressionTables[compIndex].Q16FPlen)
				RAISE(OM_ERR_BADQTABLE);
			extendedQ = TRUE;
			break;			
#endif
			
		default:
			RAISE(OM_ERR_BADQTABLE);
		}
	
#if ! STANDARD_JPEG_Q
		if (extendedQ)
		{
			tables->QTableSize = 128;
			tables->QTables = (omfUInt8 *) omOptMalloc(main, (size_t) tables->QTableSize);
		}
		else
#endif
		{
			tables->QTableSize = pdata->compressionTables[compIndex].Qlen;
			tables->QTables = (omfUInt8 *) omOptMalloc(main, (size_t) tables->QTableSize);
		}

		XASSERT(tables->QTables != NULL, OM_ERR_NOMEMORY);

		tables->ACTableSize = pdata->compressionTables[compIndex].AClen;	
		tables->ACTables = (omfUInt8 *) omOptMalloc(main, (size_t) tables->ACTableSize);
		XASSERT(tables->ACTables != NULL, OM_ERR_NOMEMORY);
	
		tables->DCTableSize = pdata->compressionTables[compIndex].DClen;	
		tables->DCTables = (omfUInt8 *) omOptMalloc(main, (size_t) tables->DCTableSize);
		XASSERT(tables->DCTables != NULL, OM_ERR_NOMEMORY);
	
		XASSERT((pdata->compressionTables[compIndex].Q || !tables->QTableSize), OM_ERR_BADQTABLE);
		XASSERT((pdata->compressionTables[compIndex].AC || !tables->ACTableSize), OM_ERR_BADACTABLE);
		XASSERT((pdata->compressionTables[compIndex].DC || !tables->DCTableSize), OM_ERR_BADDCTABLE);
	
#if ! STANDARD_JPEG_Q
		if (extendedQ)
			Qptr = (omfUInt8 *)pdata->compressionTables[compIndex].Q16FP;
		else
#endif
			Qptr = (omfUInt8 *)pdata->compressionTables[compIndex].Q;
			
		for (i = 0; i < tables->QTableSize; i++)
			(tables->QTables)[i] = Qptr[i];
		for (i = 0; i < tables->ACTableSize; i++)
			(tables->ACTables)[i] = pdata->compressionTables[compIndex].AC[i];
		for (i = 0; i < tables->DCTableSize; i++)
			(tables->DCTables)[i] = pdata->compressionTables[compIndex].DC[i];
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
static omfErr_t omfmTIFFGetMaxSampleSize(omfCodecParms_t * info,
					                 omfMediaHdl_t media,
					                 omfHdl_t main,
				                     userDataTIFF_t * pdata,
					                 omfInt32 * sampleSize)
{
	omfInt32          *maxlen = (omfInt32 *) info->spc.mediaInfo.buf;
	omfUInt32          numSamples;

	XPROTECT(main)
	{
		/*
		 * if data is compressed, and will not be software decompressed, find
		 * the largest frame by scanning the frame index.  This may take a
		 * while
		 */
	
		if (pdata->tiffCompressionType == COMPRESSION_JPEG &&
		    (media->compEnable == kToolkitCompressionDisable))
		{
			if (pdata->frameIndex)
			{
				omfUInt32	i, maxlen, thislen;
	
				CHECK(omfsTruncInt64toUInt32(media->channels[0].numSamples, &numSamples));	/* OK FRAMEOFFSET */
				for (i = 0, maxlen = 0; i < numSamples; i++)
				{
					thislen = pdata->frameIndex[i + 1] - pdata->frameIndex[i];
					maxlen = (maxlen > thislen) ? maxlen : thislen;
				}
				*sampleSize = maxlen;
			} else
				*sampleSize = pdata->bytesPerSample;	/* be pessimistic, it
									 * can't be bigger than
									 * this */
		} else
			*sampleSize = pdata->bytesPerSample;
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
 *		The GetCompressionParms call needs a different data type than the Put
 *      call because at creation time, compression is signified by the
 *      CreateVideoMedia call.  Since there is no equivalent returning of the
 *      compression type from the codec, the Get call needs a compression field,
 *      hence the 'TIFF-JPEG' info type, which adds compression.  This is part
 *      of the private interface to the TIFF codec.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfErr_t codecGetCompressionParmsTIFF(omfCodecParms_t * info,
					                     omfHdl_t main,
				                     userDataTIFF_t * pdata)

{
	omfTIFF_JPEGInfo_t  *returnParms = (omfTIFF_JPEGInfo_t *) info->spc.mediaInfo.buf;

	XPROTECT(main)
	{
		if (info->spc.mediaInfo.bufLen != sizeof(omfTIFF_JPEGInfo_t))
			RAISE(OM_ERR_INTERN_TOO_SMALL);
	
		returnParms->JPEGTableID = pdata->jpeg.JPEGTableID;
		returnParms->compression = pdata->compression;
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
omfErr_t        codecPutCompressionParmsTIFF(omfCodecParms_t * info,
					                omfMediaHdl_t media,
					                     omfHdl_t main,
				                     userDataTIFF_t * pdata)
{
	omfTIFF_JPEGInfo_t  *parms = (omfTIFF_JPEGInfo_t *) info->spc.mediaInfo.buf;

	XPROTECT(main)
	{
		if (info->spc.mediaInfo.bufLen != sizeof(omfTIFF_JPEGInfo_t))
			RAISE(OM_ERR_INTERN_TOO_SMALL);
	
		pdata->jpeg.JPEGTableID = parms->JPEGTableID;
		pdata->compression = parms->compression;
		CHECK(ReconcileTIFFInfo(media, pdata));
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
static omfErr_t readDataShort(omfCodecStream_t *stream, omfUInt16 * data)
{
	omfUInt32          rbytes;

	XPROTECT(stream->mainFile)
	{
		*data = 0;
		CHECK(omcReadStream(stream, sizeof(omfUInt16), data, &rbytes));
		if (stream->swapBytes)
			omfsFixShort((omfInt16 *) data);
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
static omfErr_t readDataLong(omfCodecStream_t *stream, omfUInt32 * data)
{
	XPROTECT(stream->mainFile)
	{
		*data = 0L;
		CHECK(omcReadStream(stream, sizeof(omfUInt32), data, NULL));
		if (stream->swapBytes)
			omfsFixLong((omfInt32 *) data);
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
static omfErr_t readDataFrom(omfCodecStream_t *stream, omfInt32 offset, void *buffer, int  bufLength)
{
	XPROTECT(stream->mainFile)
	{
		CHECK(omcSeekStream32(stream, offset));
		CHECK(omcReadStream(stream, bufLength, buffer, NULL));
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
static omfErr_t writeDataShort(omfCodecStream_t *stream, omfUInt16 data)
{
	XPROTECT(stream->mainFile)
	{
		CHECK(omcWriteStream(stream, sizeof(omfUInt16), &data));
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
static omfErr_t writeDataLong(omfCodecStream_t *stream, omfUInt32 data)
{
	XPROTECT(stream->mainFile)
	{
		CHECK(omcWriteStream(stream, sizeof(omfUInt32), &data));
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
static omfErr_t writeDataTo(omfCodecStream_t *stream, omfInt32 offset, void *buffer, int  bufLength)
{
	XPROTECT(stream->mainFile)
	{
		CHECK(omcSeekStream32(stream, offset));
		CHECK(omcWriteStream(stream, bufLength, buffer));
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
static omfErr_t codecReadBlockTIFF(omfMediaHdl_t media, void *buffer, omfUInt32 bufsize, omfInt32 samples, omfUInt32 * bytesRead, omfUInt32 *samplesRead)
{
	omfUInt32          nBytes, rBytes, fileBytes, memBytes;
	omfInt32		n;
	userDataTIFF_t *pdata;
	omfHdl_t        main;
	omfCStrmSwab_t	src, dest;
	char			*tempBuf = NULL;
	omfErr_t		status;
	omfUInt32		curPos, endPos, lastValid;
	JPEG_MediaParms_t	compressParms;
	
	omfAssertMediaHdl(media);
	main = media->mainFile;
	pdata = (userDataTIFF_t *) media->userData;
	XPROTECT(main)
	{
		XASSERT(pdata->bitsPerPixel != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->bytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		*bytesRead = 0;
		*samplesRead = 0;
		if ((pdata->tiffCompressionType == COMPRESSION_JPEG) ||
		    (pdata->tiffCompressionType == COMPRESSION_LSIJPEG))
		{
			if (media->compEnable == kToolkitCompressionEnable)
			{
				omfInt32           compressOkay;
		
				for(n = 0; n < samples; n++)
				{
					if (pdata->frameIndex == NULL)
						RAISE(OM_ERR_NOFRAMEINDEX);
					if (pdata->currentIndex + 1 >= pdata->maxIndex)
						RAISE(OM_ERR_BADFRAMEOFFSET);
			
					nBytes = pdata->frameIndex[pdata->currentIndex + 1] - pdata->frameIndex[pdata->currentIndex];
					compressParms.media = media;
					compressParms.LSIbytesRemaining = nBytes;
					
					CHECK(omcSeekStream32(media->stream, pdata->frameIndex[pdata->currentIndex] + 8));
					pdata->currentIndex++;
					
					src.fmt = media->stream->fileFormat;
					src.buflen = pdata->bytesPerSample;
					dest.fmt = media->stream->memFormat;
					dest.buf = (char *)buffer +  (n * pdata->bytesPerSample);
					omcComputeBufferSize(media->stream, pdata->bytesPerSample, omcComputeMemSize, &dest.buflen);
					if(dest.buflen * samples > bufsize)
						RAISE(OM_ERR_SMALLBUF);
	
					if((omfUInt32)pdata->bytesPerSample > dest.buflen)
					{
						tempBuf = (char *)omOptMalloc(main, pdata->bytesPerSample);
						if(tempBuf == NULL)
							RAISE(OM_ERR_NOMEMORY);
						src.buf = tempBuf;
						compressParms.pixelBuffer = (char *)tempBuf;
						compressParms.pixelBufferIndex = 0;
						compressParms.pixelBufferLength = pdata->bytesPerSample;
					}
					else
					{
						src.buf = dest.buf;
						src.buflen = pdata->bytesPerSample;
						compressParms.pixelBuffer = (char *)dest.buf;
						compressParms.pixelBufferIndex = 0;
						compressParms.pixelBufferLength = bufsize;
					}
					
					compressOkay = (omfmTIFFDecompressSample(&compressParms) == OM_ERR_NONE);
					if (!compressOkay)
						RAISE(OM_ERR_DECOMPRESS);
		
					if(compressOkay && (pdata->tiffFrameLayout == FRAMELAYOUT_MIXEDFIELDS))
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
	
						compressOkay = (omfmTIFFDecompressSample(&compressParms) == OM_ERR_NONE);
						if (!compressOkay)
							RAISE(OM_ERR_DECOMPRESS);
					}
	
					CHECK(omcSwabData(media->stream, &src, &dest, media->stream->swapBytes));
					if(tempBuf != NULL)
					{
						omOptFree(main, tempBuf);
						tempBuf = NULL;
					}
					(*samplesRead)++;
					(*bytesRead) += dest.buflen;
				}
			}
			else		/* give back a compressed frame */
			{	
				if (pdata->frameIndex == NULL)
					RAISE(OM_ERR_NOFRAMEINDEX);
				if (pdata->currentIndex + 1 >= pdata->maxIndex)
					RAISE(OM_ERR_BADFRAMEOFFSET);
		
				nBytes = pdata->frameIndex[pdata->currentIndex + 1] - pdata->frameIndex[pdata->currentIndex];
	
				CHECK(omcSeekStream32(media->stream, pdata->frameIndex[pdata->currentIndex] + 8));
				pdata->currentIndex++;
	
				XASSERT(nBytes <= bufsize, OM_ERR_SMALLBUF);
				status = omcReadStream(media->stream, nBytes, buffer, &rBytes);
				*bytesRead = rBytes;
				*samplesRead = rBytes / pdata->bytesPerSample;
				CHECK(status);
				XASSERT(samples == 1, OM_ERR_ONESAMPLEREAD);
			}
		}
		else
		{
			omfInt32	actualSamples;
			
			CHECK(omcGetStreamPos32(media->stream, &curPos));
			endPos = curPos + (pdata->bytesPerSample * samples);
			CHECK(omfsTruncInt64toInt32(media->channels[0].numSamples, &actualSamples));
			lastValid = pdata->videoOffset + (actualSamples * pdata->bytesPerSample);

			CHECK(omfmSetSwabBufSize(media->stream, pdata->bytesPerSample));
			fileBytes = samples * pdata->bytesPerSample;
			CHECK(omcComputeBufferSize(media->stream,
				fileBytes, omcComputeMemSize, &memBytes));
			if(memBytes > bufsize)
				RAISE(OM_ERR_SMALLBUF);
	
			status = omcReadSwabbedStream(media->stream, fileBytes,
				memBytes, buffer, &rBytes);
			*bytesRead = rBytes;
			*samplesRead = rBytes / pdata->bytesPerSample;
			if(endPos > lastValid)
			{
				/* There will be IFD bytes at the end */
				CHECK(omcComputeBufferSize(media->stream,
									(lastValid - curPos), omcComputeMemSize, bytesRead));
				*samplesRead = (lastValid - curPos) / pdata->bytesPerSample;
			}
			CHECK(status);
		}
	}
	XEXCEPT
	{
		if(tempBuf != NULL)
			omOptFree(main, tempBuf);
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
static omfErr_t codecWriteBlockJPEG(omfMediaHdl_t media,
									void *buffer,
									omfUInt32 bufsize,
									omfInt32 samples,
									omfUInt32 *writeLen,
									omfUInt32 *writeSamples)
{
	omfInt32 			fieldLen, n;
	omfUInt32			fileBytes, memBytes;
	userDataTIFF_t		*pdata;
	omfHdl_t			main;
	JPEG_MediaParms_t	compressParms;
	omfPosition_t		frameOffset;
	char				*tempBuf = NULL;
	omfCStrmSwab_t		src, dest;
	
	omfAssertMediaHdl(media);
	main = media->mainFile;
	XPROTECT(main)
	{
		compressParms.media = media;

		pdata = (userDataTIFF_t *)media->userData;
		XASSERT(pdata->bitsPerPixel != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->bytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

	  compressParms.isCCIR = FALSE;
	  if(pdata->jpeg.JPEGTableID >0 && pdata->jpeg.JPEGTableID < 256)
		{
		  if(pdata->imageWidth == 720 || pdata->imageWidth == 352)
			compressParms.isCCIR = TRUE;
		}
		*writeLen = 0;
		*writeSamples = 0;

	    if (pdata->tiffCompressionType == COMPRESSION_JPEG)
	    {
			if (media->compEnable == kToolkitCompressionEnable)
			{
				for(n = 0; n < samples; n++)
				{
					src.fmt = media->stream->memFormat;
					omcComputeBufferSize(media->stream, pdata->bytesPerSample, omcComputeMemSize, &memBytes);
					src.buf = (char *)buffer +  (n * memBytes);
					src.buflen = memBytes;
					dest.fmt = media->stream->fileFormat;
					dest.buflen = pdata->bytesPerSample;
					if(src.buflen * samples > bufsize)
						RAISE(OM_ERR_SMALLBUF);

					if((omfUInt32)pdata->bytesPerSample > src.buflen)
					{
						tempBuf = (char *)omOptMalloc(main, pdata->bytesPerSample);
						if(tempBuf == NULL)
							RAISE(OM_ERR_NOMEMORY);
						dest.buf = tempBuf;
						dest.buflen = pdata->bytesPerSample;
					}
					else
					{
						dest.buf = src.buf;
						tempBuf = NULL;
					}
					
					compressParms.pixelBuffer = (char *)dest.buf;
					compressParms.pixelBufferIndex = 0;
					compressParms.pixelBufferLength = pdata->bytesPerSample;
					CHECK(omcSwabData(media->stream, &src, &dest, media->stream->swapBytes));
	
					CHECK(omfmTIFFCompressSample(&compressParms, pdata->customTables, &fieldLen));
			
					*writeLen += fieldLen;
					*writeSamples += fieldLen / pdata->bytesPerSample;
					if (writeLen && (pdata->tiffFrameLayout == FRAMELAYOUT_MIXEDFIELDS))
					{
						CHECK(omfmTIFFCompressSample(&compressParms, pdata->customTables, &fieldLen));
						*writeLen += fieldLen;
					}

					if(tempBuf != NULL)
					{
						omOptFree(main, tempBuf);
						tempBuf = NULL;
					}

					CHECK(omcGetStreamPosition(media->stream, &frameOffset));	
					CHECK(omfsSubInt32fromInt64(8, &frameOffset));	/* minus 8 for IFH */
					CHECK(AddFrameOffset(media, frameOffset));
				}
			} else			/* already compressed by user */
			{
				if(samples != 1)
					RAISE(OM_ERR_ONESAMPLEWRITE);
	
				CHECK(omcWriteStream(media->stream, bufsize, buffer));
				*writeLen = bufsize;
				*writeSamples = bufsize / pdata->bytesPerSample;
				CHECK(omcGetStreamPosition(media->stream, &frameOffset));	
				CHECK(omfsSubInt32fromInt64(8, &frameOffset));	/* minus 8 for IFH */
				CHECK(AddFrameOffset(media, frameOffset));
			} 
		} else
		{
			CHECK(omfmSetSwabBufSize(media->stream, pdata->bytesPerSample));
			fileBytes = samples * pdata->bytesPerSample;
			CHECK(omcComputeBufferSize(media->stream, fileBytes, omcComputeMemSize, &memBytes));
			if(memBytes > bufsize)
				RAISE(OM_ERR_SMALLBUF);
	
			CHECK(omcWriteSwabbedStream(media->stream, fileBytes, memBytes, buffer));
			*writeLen = memBytes;
			*writeSamples = memBytes / pdata->bytesPerSample;
		}
	
		CHECK(omfsAddInt32toInt64(samples, &media->channels[0].numSamples));
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
omfErr_t codecWriteSamplesTIFF(omfCodecParms_t * info,
				                      omfMediaHdl_t	media,
				                      omfHdl_t main)
{
	omfInt32           n;
	omfmMultiXfer_t *xfer;
	
	XPROTECT(main)
	{
	for (n = 0; n < info->spc.mediaXfer.numXfers; n++)
		info->spc.mediaXfer.xfer[n].bytesXfered = 0;

	for (n = 0; n < info->spc.mediaXfer.numXfers; n++)
	{
		xfer = info->spc.mediaXfer.xfer + n;

		XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
		XASSERT(media->numChannels == 1, OM_ERR_CODEC_CHANNELS);

		CHECK(codecWriteBlockJPEG(media, xfer->buffer, xfer->buflen,
				     xfer->numSamples, &xfer->bytesXfered, &xfer->samplesXfered));
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
omfErr_t codecReadSamplesTIFF(omfCodecParms_t * info,
				                     omfMediaHdl_t	media,
				                     omfHdl_t main)
{
	omfmMultiXfer_t *xfer;
	omfInt32           n;

	XPROTECT(main)
	{
		for (n = 0; n < info->spc.mediaXfer.numXfers; n++)
			info->spc.mediaXfer.xfer[n].bytesXfered = 0;
	
		for (n = 0; n < info->spc.mediaXfer.numXfers; n++)
		{
			xfer = info->spc.mediaXfer.xfer + n;
			XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
			XASSERT(media->numChannels == 1, OM_ERR_CODEC_CHANNELS);
			CHECK(codecReadBlockTIFF(media, xfer->buffer, xfer->buflen,
					     xfer->numSamples, &xfer->bytesXfered, &xfer->samplesXfered));
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
omfErr_t omfmTIFFWriteLines(omfCodecParms_t * info,
				                   omfMediaHdl_t media,
				                   omfHdl_t main)
{
	omfInt32           nBytes, numFields;
	userDataTIFF_t 	*pdata = (userDataTIFF_t *) media->userData;
	
	omfAssert(pdata->compression == knoComp, main, OM_ERR_COMPRLINESWR);

	XPROTECT(main)
	{
		XASSERT(pdata->bitsPerPixel != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->bytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		nBytes = pdata->imageWidth * (pdata->bitsPerPixel / 8) * info->spc.mediaLinesXfer.numLines;
		if(pdata->fileLayout == kSeparateFields)
			numFields = 2;
		else
			numFields = 1;
	
		CHECK(omcWriteStream(media->stream, nBytes, info->spc.mediaLinesXfer.buf));
		pdata->numLines += info->spc.mediaLinesXfer.numLines;
		while (pdata->numLines >= (pdata->imageLength * numFields))
		{
			omfsAddInt32toInt64(1, &media->channels[0].numSamples);
			pdata->numLines -= (pdata->imageLength * numFields);
		}
		info->spc.mediaLinesXfer.bytesXfered = nBytes;
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
omfErr_t codecTIFFReadLines(omfCodecParms_t * info,
				                   omfMediaHdl_t media,
				                   omfHdl_t main)
{
	omfInt32           nBytes;
	userDataTIFF_t 	*pdata = (userDataTIFF_t *) media->userData;

	omfAssert(pdata->compression == knoComp, main, OM_ERR_COMPRLINESRD);

	XPROTECT(main)
	{
		XASSERT(pdata->bitsPerPixel != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->bytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		nBytes = pdata->imageWidth * (pdata->bitsPerPixel / 8) * info->spc.mediaLinesXfer.numLines;
	
		XASSERT(nBytes <= info->spc.mediaLinesXfer.bufLen, OM_ERR_SMALLBUF);
		if (omcReadStream(media->stream, nBytes, info->spc.mediaLinesXfer.buf, NULL) != OM_ERR_NONE)
			RAISE(OM_ERR_NODATA);
	
		info->spc.mediaLinesXfer.bytesXfered = nBytes;
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
static omfErr_t ReadIFD(omfHdl_t main, omfCodecStream_t *stream, userDataTIFF_t *pdata)
{
	omfInt64          offset, savePos;
	omfInt64          n;	/* next available offset */
	omfInt32           samplesPerPixel;
	omfUInt16          bitsPerPixelArray[4];
	omfUInt16          numEnts;/* magic number, minimum for RGB */
	omfUInt16          entries, tmp;
	omfUInt32           j, k, intRate;
	omfUInt16          Subsampling[2];
	omfUInt8          *buffer;
	omfInt32           numFields = 1;	/* Until told otherwise */
	TIFFDirEntry    	td;
	omfBool				avidTIFF;
	
	XPROTECT(main)
	{
		pdata->frameIndex = NULL;
		/* Default to full frame if raw TIFF file */
		pdata->tiffFrameLayout = FRAMELAYOUT_FULLFRAME;
		pdata->maxIndex = 1;
	
		CHECK(readDataShort(stream, &numEnts));

		/* Prescan the file for any avid format extensions.  TIFF files
		 * written by the toolkit or media composer will have all short data
		 * fields written in the wrong half of the longword (LSW whre it should be MSW).
		 *
		 * The toolkit will read short fields from the LSW if it is an avid file, and
		 * from the MSW if it is a baselije TIFF.
		 */
		avidTIFF = FALSE;
		CHECK(omcGetStreamPosition(stream, &savePos));
		for (entries = 0; entries < numEnts; entries++)
		{
			CHECK(readDataShort(stream, &td.tdir_tag));
			CHECK(readDataShort(stream, &td.tdir_type));
			CHECK(readDataLong(stream, &td.tdir_count));
			CHECK(readDataLong(stream, &td.tdir_offset));
			switch (td.tdir_tag)
			{
			case TIFFTAG_VIDEOFRAMEOFFSETS:
			case TIFFTAG_VIDEOFRAMEBYTECOUNTS:
			case TIFFTAG_VIDEOFRAMELAYOUT:
			case TIFFTAG_VIDEOFRAMERATE:
			case TIFFTAG_JPEGQTABLES16:
				avidTIFF = TRUE;
				break;
			
			default:
				break;
			}
		}
		CHECK(omcSeekStreamTo(stream, savePos));
		
		for (entries = 0; entries < numEnts; entries++)
		{
			CHECK(readDataShort(stream, &td.tdir_tag));
			CHECK(readDataShort(stream, &td.tdir_type));
			CHECK(readDataLong(stream, &td.tdir_count));
			CHECK(readDataLong(stream, &td.tdir_offset));
			CHECK(omcGetStreamPosition(stream, &savePos));

			switch (td.tdir_tag)
			{
			case TIFFTAG_IMAGEWIDTH:
				if(avidTIFF || (td.tdir_type == TIFF_LONG) || (td.tdir_type == TIFF_SLONG))
					pdata->imageWidth = (omfInt16) td.tdir_offset;
				else
					pdata->imageWidth = (omfInt16) (td.tdir_offset >> 16L);
				break;
	
			case TIFFTAG_IMAGELENGTH:
				if(avidTIFF || (td.tdir_type == TIFF_LONG) || (td.tdir_type == TIFF_SLONG))
					pdata->imageLength = (omfInt16) td.tdir_offset;
				else
					pdata->imageLength = (omfInt16) (td.tdir_offset >> 16L);
				break;
	
			case TIFFTAG_BITSPERSAMPLE:
				if (td.tdir_count != 3)
					RAISE(OM_ERR_3COMPONENT);
				omfsCvtInt32toInt64(td.tdir_offset, &offset);
				CHECK(omcSeekStreamTo(stream, offset));
	
				for (j = 0; j < td.tdir_count; j++)
				{
					CHECK(readDataShort(stream, &bitsPerPixelArray[j]));
				}
				if (bitsPerPixelArray[0] != 8 ||
				    bitsPerPixelArray[1] != 8 ||
				    bitsPerPixelArray[2] != 8)
					return (OM_ERR_24BITVIDEO);
				pdata->bitsPerPixel = 24;
				break;
	
			case TIFFTAG_COMPRESSION:
				if(avidTIFF)
					pdata->tiffCompressionType = td.tdir_offset;
				else
					pdata->tiffCompressionType = (td.tdir_offset >> 16L);
				break;
	
			case TIFFTAG_PHOTOMETRIC:
				if(avidTIFF)
					pdata->tiffPixelFormat = (omfInt16)td.tdir_offset;
				else
					pdata->tiffPixelFormat = (omfInt16)(td.tdir_offset >> 16L);
				break;
	
			case TIFFTAG_SAMPLESPERPIXEL:
				if(avidTIFF)
					samplesPerPixel = td.tdir_offset;
				else
					samplesPerPixel = (td.tdir_offset >> 16L);
				break;
	
			case TIFFTAG_ROWSPERSTRIP:
				/*if(avidTIFF || (td.tdir_type == TIFF_LONG) || (td.tdir_type == TIFF_SLONG)) */
				break;
	
			case TIFFTAG_STRIPOFFSETS:
				if ((avidTIFF) || (td.tdir_count == 1))
				{
				  if(avidTIFF || (td.tdir_type == TIFF_LONG) || (td.tdir_type == TIFF_SLONG))
					pdata->videoOffset = td.tdir_offset;
				  else
					pdata->videoOffset = (td.tdir_offset >> 16L);
				}
				else
				  {
					omfsCvtInt32toInt64(td.tdir_offset, &offset);
					CHECK(omcSeekStreamTo(stream, offset));
	
					/*!!!				for (j = 0; j < td.tdir_count; j++)*/
					{
					  if((td.tdir_type == TIFF_LONG) || (td.tdir_type == TIFF_SLONG))
						{
						  CHECK(readDataLong(stream, (omfUInt32 *)&pdata->videoOffset));
						}
					  else
						{
						  CHECK(readDataShort(stream, &tmp));
						  pdata->videoOffset = tmp;
						} 
					}
				  }
				break;
	
			case TIFFTAG_STRIPBYTECOUNTS:
				/*if(avidTIFF || (td.tdir_type == TIFF_LONG) || (td.tdir_type == TIFF_SLONG)) */
				/* vi->desc.bytesPerSample = td.tdir_offset; */
				break;
	
			case TIFFTAG_XRESOLUTION:
			case TIFFTAG_YRESOLUTION:
			case TIFFTAG_RESOLUTIONUNIT:
				break;
	
			case TIFFTAG_JPEGPROC:
				if (pdata->tiffCompressionType != COMPRESSION_JPEG)
					RAISE(OM_ERR_BADJPEGINFO);
				if (td.tdir_offset != JPEGPROC_BASELINE)
					RAISE(OM_ERR_JPEGBASELINE);
				break;
	
			case TIFFTAG_JPEGQTABLES:
				if (td.tdir_count != 3)
					RAISE(OM_ERR_3COMPONENT);
	
				omfsCvtInt32toInt64(td.tdir_offset, &offset);
				CHECK(omcSeekStreamTo(stream, offset));
	
				for (j = 0; j < td.tdir_count; j++)
				{
					omfUInt32          table_offset;
	
					buffer = pdata->compressionTables[j].Q;
	
					CHECK(omcSeekStreamTo(stream, offset));
					CHECK(readDataLong(stream, &table_offset));
					CHECK(omcGetStreamPosition(stream, &offset));
	
					CHECK(readDataFrom(stream, table_offset, buffer, 64));
	
					pdata->compressionTables[j].Qlen = 64;
	
	#ifdef JPEG_TRACE
					{
						fprintf(stderr, "Q table %1ld\n", j);
						for (k = 0; k < 8; k++)
							fprintf(stderr, "   %3d %3d %3d %3d %3d %3d %3d %3d\n",
								pdata->compressionTables[j].Q[k * 8 + 0],
								pdata->compressionTables[j].Q[k * 8 + 1],
								pdata->compressionTables[j].Q[k * 8 + 2],
								pdata->compressionTables[j].Q[k * 8 + 3],
								pdata->compressionTables[j].Q[k * 8 + 4],
								pdata->compressionTables[j].Q[k * 8 + 5],
								pdata->compressionTables[j].Q[k * 8 + 6],
								pdata->compressionTables[j].Q[k * 8 + 7]);
						fprintf(stderr, "\n");
					}
	#endif
				}
	
				break;
	
			case TIFFTAG_JPEGDCTABLES:
				if (td.tdir_count != 3)
					RAISE(OM_ERR_3COMPONENT);
	
				omfsCvtInt32toInt64(td.tdir_offset, &n);
				CHECK(omcSeekStreamTo(stream, n));
	
				for (j = 0; j < td.tdir_count; j++)
				{
					omfUInt32          table_offset;
					omfInt32           len;
	
					buffer = pdata->compressionTables[j].DC;
	
					CHECK(omcSeekStreamTo(stream, n));
	
					CHECK(readDataLong(stream, &table_offset));
					CHECK(omcGetStreamPosition(stream, &n));
	
					CHECK(readDataFrom(stream, table_offset, buffer, 16));
	
					for (k = 0, len = 0; k < 16; k++)
						len += buffer[k];
	
	#ifdef JPEG_TRACE
					fprintf(stderr, "DC table %1ld length is %1ld\n  ", j, len);
	#endif
	
					table_offset += 16;
					buffer += 16;
					pdata->compressionTables[j].DClen = len + 16;
	
					CHECK(readDataFrom(stream, table_offset, buffer, len));
	#ifdef JPEG_TRACE
					{
						omfInt16           nitems;
	
						buffer = &pdata->compressionTables[j].DC[16];
						for (k = 0, nitems = 0; k < len; k++)
						{
							fprintf(stderr, "%1d ", buffer[k]);
							if (++nitems > 15)
							{
								fprintf(stderr, "\n  ");
								nitems = 0;
							}
						}
						fprintf(stderr, "\n\n");
					}
	#endif
				}
				break;
	
			case TIFFTAG_JPEGACTABLES:
				if (td.tdir_count != 3)
					RAISE(OM_ERR_3COMPONENT);
	
				omfsCvtInt32toInt64(td.tdir_offset, &offset);
				CHECK(omcSeekStreamTo(stream, offset));
	
				for (j = 0; j < td.tdir_count; j++)
				{
					omfUInt32          table_offset;
					omfInt32           len;
	
					CHECK(omcSeekStreamTo(stream, offset));
					CHECK(readDataLong(stream, &table_offset));
					CHECK(omcGetStreamPosition(stream, &offset));
	
					buffer = pdata->compressionTables[j].AC;
	
					CHECK(readDataFrom(stream, table_offset, buffer, 16));
	
					for (k = 0, len = 0; k < 16; k++)
						len += buffer[k];
	
	#ifdef JPEG_TRACE
					fprintf(stderr, "AC table %1ld length is %1ld\n  ", j, len);
	#endif
	
					table_offset += 16;
					buffer += 16;
					pdata->compressionTables[j].AClen = len + 16;
	
					CHECK(readDataFrom(stream, table_offset, buffer, len));
	
	#ifdef JPEG_TRACE
					{
						omfInt16           nitems;
						buffer = &pdata->compressionTables[j].AC[16];
						for (k = 0, nitems = 0; k < len; k++)
						{
							fprintf(stderr, "%1d ", buffer[k]);
							if (++nitems > 15)
							{
								fprintf(stderr, "\n  ");
								nitems = 0;
							}
						}
						fprintf(stderr, "\n\n");
					}
	#endif
				}
				break;
	
			case TIFFTAG_YCBCRSUBSAMPLING:
				Subsampling[0] = (omfInt16) ((td.tdir_offset & 0xffff0000) >> 16);
				Subsampling[1] = (omfInt16) (td.tdir_offset & 0x0000ffff);
				/*
				 * assert(Subsampling[0] == 2 && Subsampling[1] == 1,
				 * "2h1v subsampling only");
				 */
				break;
	
				/* read in the frame index */
			case TIFFTAG_VIDEOFRAMEOFFSETS:
				/* Free first if used before */
				if (pdata->frameIndex)
					omOptFree(main, (void *) pdata->frameIndex);
				pdata->frameIndex = (omfUInt32 *) omOptMalloc(main, (size_t) td.tdir_count * sizeof(omfInt32));
				if (pdata->frameIndex == NULL)
					RAISE(OM_ERR_NOMEMORY);
	
				pdata->maxIndex = td.tdir_count;
				pdata->currentIndex = 0;
				omfsCvtInt32toInt64(td.tdir_offset, &offset);
				CHECK(omcSeekStreamTo(stream, offset));
	
				for (j = 0; j < td.tdir_count; j++)
				{
					/*
					 * Bug fix (LF-W): Need to mask out high bit
					 * that is set by Avid Media Suite Pro for
					 * exact frame animation images
					 */
					CHECK(readDataLong(stream, &pdata->frameIndex[j]));
					pdata->frameIndex[j] &= 0x7FFFFFFF;
	#ifdef JPEG_TRACE
					fprintf(stderr, "Frame index entry %1ld/%1ld = %1ld\n",
					j + 1, td.tdir_count, pdata->frameIndex[j]);
	#endif
				}
				break;
	
			case TIFFTAG_VIDEOFRAMELAYOUT:
				pdata->tiffFrameLayout = (omfInt16) td.tdir_offset;
				if (td.tdir_offset == FRAMELAYOUT_MIXEDFIELDS)
					numFields = 2;
				else
					numFields = 1;
				break;
	
			case TIFFTAG_VIDEOFRAMERATE:
				break;
	
	#ifdef  JPEG_QT_16
			case TIFFTAG_JPEGQTABLES16:
				if (td.tdir_count != 3)
					RAISE(OM_ERR_3COMPONENT);
	
				omfsCvtInt32toInt64(td.tdir_offset, &offset);	/* Where is old Offset set? */
	
				for (j = 0; j < 3; j++)
				{
					omfUInt32          table_offset;
					omfUInt16         *sbuffer;
					omfInt64          saveQOffset;
	
					if (!omcIsPosValid(stream, offset))
						break;	/* M/C Sometimes forgets the third component */
					CHECK(omcSeekStreamTo(stream, offset));
					CHECK(readDataLong(stream, &table_offset));
					CHECK(omcGetStreamPosition(stream, &saveQOffset));
	
					sbuffer = pdata->compressionTables[j].Q16;
	
					omfsCvtInt32toInt64(table_offset, &offset);
	
					for (k = 0; k < 64; k++)
					{
						CHECK(readDataShort(stream, &sbuffer[k]));
					}
	
					pdata->compressionTables[j].Q16len = 64;
	
					CHECK(omcSeekStreamTo(stream, saveQOffset));
	
	#ifdef JPEG_TRACE
					fprintf(stderr, "Q(16) table %1ld\n", j);
					for (k = 0; k < 8; k++)
						fprintf(stderr, "   %3d %3d %3d %3d %3d %3d %3d %3d\n",
							pdata->compressionTables[j].Q16[k * 8 + 0],
							pdata->compressionTables[j].Q16[k * 8 + 1],
							pdata->compressionTables[j].Q16[k * 8 + 2],
							pdata->compressionTables[j].Q16[k * 8 + 3],
							pdata->compressionTables[j].Q16[k * 8 + 4],
							pdata->compressionTables[j].Q16[k * 8 + 5],
							pdata->compressionTables[j].Q16[k * 8 + 6],
							pdata->compressionTables[j].Q16[k * 8 + 7]);
					fprintf(stderr, "\n");
	#endif
				}
				break;
	#endif
	
			default:
				break;
			}
	
			CHECK(omcSeekStreamTo(stream, savePos));
		}

		switch (pdata->tiffCompressionType)
		{
		case 0x00010000:
		case COMPRESSION_NONE:
			pdata->compression = knoComp; 
			break;
		case COMPRESSION_JPEG:
			pdata->compression = kJPEG;
			break;
		case COMPRESSION_LSIJPEG:
			pdata->compression = kLSIJPEG;
			intRate = pdata->rate.numerator / pdata->rate.denominator;
			if((intRate == 29) || (intRate == 30))
			{
				pdata->fieldDominance = kDominantField2;		/* Different for LSI JPEG */
				pdata->videoLineMap[0] = 17;	/* Field 2 top */
				pdata->videoLineMap[1] = 16;
			}

			break;
		default:
			RAISE(OM_ERR_BADCOMPR);
		}
	
		switch (pdata->tiffFrameLayout)
		{
		case FRAMELAYOUT_FULLFRAME:
			pdata->fileLayout = kFullFrame;
			break;
		case FRAMELAYOUT_SINGLEFIELDODD:
			pdata->fileLayout = kOneField;
			break;
		case FRAMELAYOUT_MIXEDFIELDS:
			pdata->fileLayout = kSeparateFields;
			break;
		default:
			RAISE(OM_ERR_BADLAYOUT);
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
static omfErr_t CreateTIFFdata_End(omfMediaHdl_t media)
{
	omfInt32           padbytes = 0;
	double          outputRate;
	userDataTIFF_t *pdata;
	omfHdl_t        main;

	/* this routine will be called after sample data is written */

	omfAssertMediaHdl(media);
	pdata = (userDataTIFF_t *) media->userData;
	main = media->mainFile;
	XPROTECT(main)
	{
	CHECK(WriteIFD(media->mainFile, media->stream, pdata, media->channels[0].numSamples, TRUE));

	outputRate = (double) pdata->rate.numerator / (double) pdata->rate.denominator;

	if (media->fileMob == NULL)
		RAISE(OM_ERR_INTERNAL_CORRUPTVINFO);

	if (media->mdes == NULL)
		RAISE(OM_ERR_INTERNAL_CORRUPTVINFO);

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
static omfErr_t WriteIFD(omfHdl_t main, omfCodecStream_t *stream, userDataTIFF_t *pdata,
						omfLength_t numSamples, omfBool has_data)
{
	omfInt32           samplesPerPixel;
	omfUInt16          bitsPerPixelArray[3];
	omfUInt16          numEnts = 13;	/* magic number, minimum for RGB */
	omfUInt32          n;				/* next available offset */
	omfInt32           i, j, len;
	omfInt32           frameRate[2];
	TIFFDirEntry    entry;
	omfUInt8          *buffer;
	omfUInt8          *databuf;
	omfUInt16         *sbuffer;
	omfUInt32          saveOffset, savePos, numSamp32;
	TIFFHeader      hdr;

	static omfInt32    Xresolution[2] = {4, 0};
	static omfInt32    Yresolution[2] = {3, 0};
	static omfInt16    Subsampling[2] = {2, 1};	/* equivalent to 2h1v, see TIFF 6.0 */

	XPROTECT(main)
	{
		samplesPerPixel = 3;
	
		bitsPerPixelArray[0] = 8;
		bitsPerPixelArray[1] = 8;
		bitsPerPixelArray[2] = 8;
	
		CHECK(omcGetStreamPos32(stream, &pdata->firstIFD));
	
		frameRate[0] = pdata->rate.numerator;
		frameRate[1] = pdata->rate.denominator;
	
		if (pdata->tiffCompressionType == COMPRESSION_JPEG)
		{
			if (has_data)
				numEnts += 6;	/* extra entries for JPEG  */
			else
				numEnts += 1;
	
	#ifdef JPEG_QT_16
			if (has_data)
				numEnts += 1;	/* 16-bit Quantization tables */
	#endif
	
		}
	
		CHECK(omfsTruncInt64toUInt32(numSamples, &numSamp32));	/* OK FRAMEOFFSET */
		if (numSamp32 > 1)
			numEnts += 1;	/* add entry for frame rate */
	
		/* Bento doesn't like holes, so write a blank ifd first.  This will 
		 * allow add-on data (like BITSPERSAMPLE) to be appended before IFD
		 * is finished.  4 for nextIFD + 2 for numEnts
		 */
		databuf = (omfUInt8 *) omOptMalloc(main, (size_t) numEnts * 12 + 6);
		if (databuf == NULL)
			RAISE(OM_ERR_NOMEMORY);
		for(i = (numEnts * 12 + 6) - 1; i >= 0; i--)
		  databuf[i] = 0; /* stop uninitialized memory references */

		CHECK(omcGetStreamPos32(stream, &saveOffset));
		CHECK(omcWriteStream(stream, (omfInt32) (numEnts * 12 + 6), databuf));
		CHECK(omcSeekStream32(stream, saveOffset));
	
		CHECK(writeDataShort(stream, numEnts));
		CHECK(omcGetStreamPos32(stream, &saveOffset));
		n = saveOffset + numEnts * 12 + 4;	/* point to the next available space */
	
		CHECK(WriteDirEntry(main, stream, TIFFTAG_IMAGEWIDTH, TIFF_LONG, pdata->imageWidth));
		CHECK(WriteDirEntry(main, stream, TIFFTAG_IMAGELENGTH, TIFF_LONG, pdata->imageLength));
	
		/* write out the bitsPerPixel array  */
		entry.tdir_tag = TIFFTAG_BITSPERSAMPLE;
		entry.tdir_type = TIFF_SHORT;
		entry.tdir_count = 3;
		entry.tdir_offset = n;
		CHECK(Put_TIFFDirEntry(main, stream, entry, bitsPerPixelArray, &n));
		CHECK(WriteDirEntry(main, stream, TIFFTAG_COMPRESSION, TIFF_SHORT,
					     pdata->tiffCompressionType));
	
		CHECK(WriteDirEntry(main, stream, TIFFTAG_PHOTOMETRIC, TIFF_SHORT, pdata->tiffPixelFormat));
		CHECK(WriteDirEntry(main, stream, TIFFTAG_STRIPOFFSETS, TIFF_LONG, pdata->videoOffset));
		CHECK(WriteDirEntry(main, stream, TIFFTAG_SAMPLESPERPIXEL, TIFF_SHORT, samplesPerPixel));
		CHECK(WriteDirEntry(main, stream, TIFFTAG_ROWSPERSTRIP, TIFF_LONG, pdata->imageLength));
	
		CHECK(WriteDirEntry(main, stream, TIFFTAG_STRIPBYTECOUNTS, TIFF_LONG,
					     pdata->bytesPerSample));
	
		/* write out the Xresolution rational  */
		entry.tdir_tag = TIFFTAG_XRESOLUTION;
		entry.tdir_type = TIFF_RATIONAL;
		entry.tdir_count = 1;
		entry.tdir_offset = n;
		CHECK(Put_TIFFDirEntry(main, stream, entry, Xresolution, &n));
	
		/* Write out the Yresolution rational  */
		entry.tdir_tag = TIFFTAG_YRESOLUTION;
		entry.tdir_type = TIFF_RATIONAL;
		entry.tdir_count = 1;
		entry.tdir_offset = n;
		CHECK(Put_TIFFDirEntry(main, stream, entry, Yresolution, &n));
	
		CHECK(WriteDirEntry(main, stream, TIFFTAG_RESOLUTIONUNIT, TIFF_SHORT, RESUNIT_INCH));
	
		if (pdata->tiffCompressionType == COMPRESSION_JPEG && has_data)
		{
			omfInt32           offsets[3] = {0, 0, 0};
			omfInt32           array_offset;
	
			CHECK(WriteDirEntry(main, stream, TIFFTAG_JPEGPROC, TIFF_SHORT, JPEGPROC_BASELINE));
	
			/* write out the JPEG Quantization tables */
			CHECK(writeDataShort(stream, TIFFTAG_JPEGQTABLES));
			CHECK(writeDataShort(stream, TIFF_LONG));
			CHECK(writeDataLong(stream, samplesPerPixel));
			CHECK(writeDataLong(stream, n));
	
			/* write array of pointers to tables */
			CHECK(omcGetStreamPos32(stream, &savePos));
			array_offset = n;
			CHECK(writeDataTo(stream, n, (void *) &offsets, sizeof(omfInt32) * 3));
			n += sizeof(omfInt32) * 3;
	
			/*
			 * use the 0 table for luminance, duplicate the 1 table for
			 * both chrominance
			 */
			for (i = 0; i < samplesPerPixel; i++)
			{
				CHECK(writeDataTo(stream, array_offset + (i * sizeof(omfInt32)), (void *) &n,
							   sizeof(omfInt32)));
	
				if (pdata->customTables && pdata->compressionTables[(i == 0) ? 0 : 1].Qlen)
					buffer = pdata->compressionTables[(i == 0) ? 0 : 1].Q;
				else
					buffer = STD_QT_PTR[(i == 0) ? 0 : 1];
	
				CHECK(writeDataTo(stream, n, (void *) buffer, 64 * sizeof(omfUInt8)));
				n += 64 * sizeof(omfUInt8);
			}
	
			CHECK(omcSeekStream32(stream, savePos));
	
			CHECK(writeDataShort(stream, TIFFTAG_JPEGDCTABLES));
			CHECK(writeDataShort(stream, TIFF_LONG));
			CHECK(writeDataLong(stream, samplesPerPixel));
			CHECK(writeDataLong(stream, n));
	
			/* write array of pointers to tables */
			CHECK(omcGetStreamPos32(stream, &savePos));
			array_offset = n;
			CHECK(writeDataTo(stream, n, (void *) &offsets, sizeof(omfInt32) * 3));
			n += sizeof(omfInt32) * 3;
	
			/*
			 * use the 0 table for luminance, duplicate the 1 table for
			 * both chrominance
			 */
			for (i = 0; i < samplesPerPixel; i++)
			{
				CHECK(writeDataTo(stream, n, (void *) &offsets, sizeof(omfInt32) * 3));
				CHECK(writeDataTo(stream, array_offset + (i * sizeof(omfInt32)), (void *) &n,
							   sizeof(omfInt32)));
	
				if (pdata->customTables && pdata->compressionTables[(i == 0) ? 0 : 1].DClen)
				{
					buffer = pdata->compressionTables[(i == 0) ? 0 : 1].DC;
					len = 16;
					for (j = 0; j < 16; j++)
						len += buffer[j];
				} else
				{
					buffer = STD_DCT_PTR[(i == 0) ? 0 : 1];
					len = STD_DCT_LEN[(i == 0) ? 0 : 1];
				}
	
				CHECK(writeDataTo(stream, n, (void *) buffer, len));
				n += len;
			}
			CHECK(omcSeekStream32(stream, savePos));
	
			/* write out the JPEG AC Huffman tables */
	
			CHECK(writeDataShort(stream, TIFFTAG_JPEGACTABLES));
			CHECK(writeDataShort(stream, TIFF_LONG));
			CHECK(writeDataLong(stream, samplesPerPixel));
			CHECK(writeDataLong(stream, n));
	
			/* write array of pointers to tables */
			CHECK(omcGetStreamPos32(stream, &savePos));
			array_offset = n;
			CHECK(writeDataTo(stream, n, (void *) &offsets, sizeof(omfInt32) * 3));
			n += sizeof(omfInt32) * 3;
	
			/*
			 * use the 0 table for luminance, duplicate the 1 table for
			 * both chrominance
			 */
			for (i = 0; i < samplesPerPixel; i++)
			{
				CHECK(writeDataTo(stream, array_offset + (i * sizeof(omfInt32)), (void *) &n,
							   sizeof(omfInt32)));
				if (pdata->customTables && pdata->compressionTables[(i == 0) ? 0 : 1].AClen)
				{
					buffer = pdata->compressionTables[(i == 0) ? 0 : 1].AC;
					len = 16;
					for (j = 0; j < 16; j++)
						len += buffer[j];
				} else
				{
					buffer = STD_ACT_PTR[(i == 0) ? 0 : 1];
					len = STD_ACT_LEN[(i == 0) ? 0 : 1];
				}
	
				CHECK(writeDataTo(stream, n, (void *) buffer, len));
				n += len;
			}
			CHECK(omcSeekStream32(stream, savePos));
		}
		if (pdata->tiffCompressionType == COMPRESSION_JPEG)
		{
			/* write out the YCbCr sumsampling ratio */
			entry.tdir_tag = TIFFTAG_YCBCRSUBSAMPLING;
			entry.tdir_type = TIFF_SHORT;
			entry.tdir_count = 2;
			entry.tdir_offset = Subsampling[0] << 16 | Subsampling[1];
			CHECK(Put_TIFFDirEntry(main, stream, entry, Subsampling, &n));
		}
		if (pdata->tiffCompressionType == COMPRESSION_JPEG && has_data)
		{
			/* write out the frame index */
			entry.tdir_tag = TIFFTAG_VIDEOFRAMEOFFSETS;
			entry.tdir_type = TIFF_LONG;
			entry.tdir_count = pdata->currentIndex;
			entry.tdir_offset = n;
			CHECK(Put_TIFFDirEntry(main, stream, entry, pdata->frameIndex, &n));
		}
		CHECK(WriteDirEntry(main, stream, (omfInt16)TIFFTAG_VIDEOFRAMELAYOUT, 
							(omfInt16)TIFF_SHORT, pdata->tiffFrameLayout));
	
		if (numSamp32 > 1)
		{
			entry.tdir_tag = TIFFTAG_VIDEOFRAMERATE;
			entry.tdir_type = TIFF_RATIONAL;
			entry.tdir_count = 1;
			entry.tdir_offset = n;
			CHECK(Put_TIFFDirEntry(main, stream, entry, frameRate, &n));
		}
	#ifdef  JPEG_QT_16
		if (pdata->tiffCompressionType == COMPRESSION_JPEG && has_data)
			/* write out the JPEG Quantization tables */
		{
			omfInt32           offsets[3] = {0, 0, 0};
			omfInt32           array_offset;
			omfInt32           i, j;
	
			CHECK(writeDataShort(stream, (omfUInt16) TIFFTAG_JPEGQTABLES16));
			CHECK(writeDataShort(stream, TIFF_LONG));
			CHECK(writeDataLong(stream, samplesPerPixel));
			CHECK(writeDataLong(stream, n));
	
			/* write array of pointers to tables */
			CHECK(omcGetStreamPos32(stream, &savePos));
			array_offset = n;
			CHECK(writeDataTo(stream, n, (void *) &offsets, sizeof(omfInt32) * 3));
			n += sizeof(omfInt32) * 3;
	
			/*
			 * use the 0 table for luminance, duplicate the 1 table for
			 * both chrominance
			 */
			for (i = 0; i < samplesPerPixel; i++)
			{
				omfUInt32          savePos2;
	
				CHECK(writeDataTo(stream, array_offset + (i * sizeof(omfInt32)), (void *) &n,
							   sizeof(omfInt32)));
	
				if (pdata->customTables && pdata->compressionTables[(i == 0) ? 0 : 1].Q16len)
					sbuffer = pdata->compressionTables[(i == 0) ? 0 : 1].Q16;
				else
					sbuffer = STD_QT16_PTR[(i == 0) ? 0 : 1];
	
				CHECK(omcGetStreamPos32(stream, &savePos2));
				CHECK(omcSeekStream32(stream, n));
	
				for (j = 0; j < 64; j++)
				{
					CHECK(writeDataShort(stream, sbuffer[j]));
				}
	
				CHECK(omcGetStreamPos32(stream, &n));
				CHECK(omcSeekStream32(stream, savePos2));
			}
	
			CHECK(omcSeekStream32(stream, savePos));
		}
	#endif
	
		hdr.tiff_magic = OMNativeByteOrder;
		hdr.tiff_version = TIFF_VERSION;
		hdr.tiff_diroff = pdata->firstIFD;
		CHECK(writeDataTo(stream, 0, (void *) &hdr, sizeof(hdr)));
		CHECK(omcSeekStream32(stream, pdata->firstIFD + 2 + (numEnts * 12)));	/* go to "next IFD"
												 * field */
	
		CHECK(writeDataLong(stream, 0L));	/* next IFD field is
								 * null, no more IFDs */
		CHECK(omcSeekStream32(stream, n));	/* resume writing at end
								 * of auxiliary data. */
	
		if (databuf)
			omOptFree(main, (void *) databuf);
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
static omfErr_t Put_TIFFDirEntry(omfHdl_t main, omfCodecStream_t *stream, TIFFDirEntry entry, void *data, omfUInt32 * offset)
{
	omfInt32           numData = 0;
	omfUInt32          endPos;

	XPROTECT(main)
	{
		CHECK(writeDataShort(stream, entry.tdir_tag));
		CHECK(writeDataShort(stream, entry.tdir_type));
		CHECK(writeDataLong(stream, entry.tdir_count));
	
		if (entry.tdir_count < 1)
			return (OM_ERR_BADTIFFCOUNT);
	
		switch (entry.tdir_type)
		{
		case TIFF_BYTE:
		case TIFF_SBYTE:
		case TIFF_UNDEFINED:
		case TIFF_ASCII:
			{
				omfUInt32          savePos;
	
				if (entry.tdir_count < 5)
				{
					CHECK(writeDataLong(stream, entry.tdir_offset));
					break;
				}
				CHECK(writeDataLong(stream, entry.tdir_offset));
				CHECK(omcGetStreamPos32(stream, &savePos));
				CHECK(writeDataTo(stream, entry.tdir_offset, data, entry.tdir_count));
				CHECK(omcGetStreamPos32(stream, &endPos));
				numData = endPos - entry.tdir_offset;
	
				CHECK(omcSeekStream32(stream, savePos));	/* return to next ifd
											 * entry */
				break;
			}
	
		case TIFF_SHORT:
		case TIFF_SSHORT:
			{
				CHECK(writeDataLong(stream, entry.tdir_offset));
	
				if (entry.tdir_count < 3)	/* you are done */
					break;
				else	/* have an array of shorts to write out */
				{
					omfUInt32 i,savePos;
	
					CHECK(omcGetStreamPos32(stream, &savePos));
					CHECK(omcSeekStream32(stream, entry.tdir_offset));
	
					for ( i = 0 ; i < entry.tdir_count; i++ )
					{
						CHECK(writeDataShort(stream, ((omfUInt16*)data)[i]));
					}
					
					CHECK(omcGetStreamPos32(stream, &endPos));
					numData = endPos - entry.tdir_offset;
					CHECK(omcSeekStream32(stream, savePos)); /* return to next ifd entry */
					break;
				}
			}
	
		case TIFF_LONG:
		case TIFF_SLONG:
		case TIFF_FLOAT:
			{
				omfUInt32          i, savePos;
		
				CHECK(writeDataLong(stream, entry.tdir_offset));
		
				if (entry.tdir_count == 1)
					break;
				/* write out an array of longs */
		
				CHECK(omcGetStreamPos32(stream, &savePos));
				CHECK(omcSeekStream32(stream, entry.tdir_offset));
		
				for (i = 0; i < entry.tdir_count; i++)
				{
					CHECK(writeDataLong(stream, ((omfUInt32 *) data)[i]));
				}
		
				CHECK(omcGetStreamPos32(stream, &endPos));
				numData = endPos - entry.tdir_offset;
				CHECK(omcSeekStream32(stream, savePos));	/* return to next ifd
											 * entry */
				break;
			}
	
		case TIFF_DOUBLE:
		case TIFF_SRATIONAL:
		case TIFF_RATIONAL:
			{
				omfUInt32          i, savePos;
		
				CHECK(writeDataLong(stream, entry.tdir_offset));
		
				CHECK(omcGetStreamPos32(stream, &savePos));
				CHECK(omcSeekStream32(stream, entry.tdir_offset));
		
				for (i = 0; i < entry.tdir_count * 2; i++)
				{
					CHECK(writeDataLong(stream, ((omfUInt32 *) data)[i]));
				}
		
				CHECK(omcGetStreamPos32(stream, &endPos));
				numData = endPos - entry.tdir_offset;
				CHECK(omcSeekStream32(stream, savePos));	/* return to next ifd
											 * entry */
				break;
			}
		}
	}
	XEXCEPT
	XEND
	
	if (offset != NULL)
		*offset += numData;
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
static omfErr_t ReconcileTIFFInfo(omfMediaHdl_t media, userDataTIFF_t * pdata)
{
	XPROTECT(media->mainFile)
	{
		switch (pdata->compression)
		{
		case kJPEG:
		case kLSIJPEG:
			pdata->tiffCompressionType = COMPRESSION_JPEG;
			{
				pdata->tiffPixelFormat = PHOTOMETRIC_YCBCR;
				pdata->filePixelFormat = kOmfPixYUV;
			}
			break;

		case knoComp:
			pdata->tiffCompressionType = COMPRESSION_NONE;
			if ((pdata->filePixelFormat == kOmfPixRGBA))
				pdata->tiffPixelFormat = PHOTOMETRIC_RGB;
			else if (pdata->filePixelFormat == kOmfPixYUV)
				pdata->tiffPixelFormat = PHOTOMETRIC_YCBCR;
			else
				RAISE(OM_ERR_BADEXPORTPIXFORM);
			break;
	
		default:
			RAISE(OM_ERR_BADEXPORTPIXFORM);
		}
	
		switch (pdata->fileLayout)
		{
		case kFullFrame:
			pdata->tiffFrameLayout = FRAMELAYOUT_FULLFRAME;
			break;
	
		case kOneField:
			pdata->tiffFrameLayout = FRAMELAYOUT_SINGLEFIELDODD;
			break;
	
		case kSeparateFields:
			pdata->tiffFrameLayout = FRAMELAYOUT_MIXEDFIELDS;
			break;
	
		default:
			RAISE(OM_ERR_BADEXPORTLAYOUT);
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
static omfErr_t setupStream(omfCodecStream_t *stream, omfDDefObj_t dataKind,
							userDataTIFF_t * pdata, omfBool swapBytes)
{
	omfCStrmFmt_t   memFmt, fileFmt;
	omfPixelFormat_t	format;
	omfVideoMemOp_t		*memOp;
	
	XPROTECT(stream->mainFile)
	{
		memFmt.mediaKind = dataKind;
		memFmt.vfmt = pdata->fmtOps;
		memFmt.afmt = NULL;
		CHECK(omcSetMemoryFormat(stream, memFmt));
		
		GetFilePixelFormat(stream->mainFile, pdata, &format);
		if(pdata->codecCompression)		/* JPEG compress/decompress will accept either	*/
		{								/* RGB or YUV, so let IT do the translation.	*/
			for(memOp = pdata->fmtOps; memOp->opcode != kOmfVFmtEnd; memOp++)
			{
				if(memOp->opcode == kOmfPixelFormat)
				{
					format = memOp->operand.expPixelFormat;
					break;
				}
			}
		}
		pdata->fileFmt[0].opcode = kOmfPixelFormat;
		pdata->fileFmt[0].operand.expPixelFormat = format;
		
		pdata->fileFmt[1].opcode = kOmfFrameLayout;
		pdata->fileFmt[1].operand.expFrameLayout = pdata->fileLayout;
		
		pdata->fileFmt[2].opcode = kOmfPixelSize;
		pdata->fileFmt[2].operand.expInt16 = pdata->bitsPerPixel;
		
		pdata->fileFmt[3].opcode = kOmfStoredRect;
		pdata->fileFmt[3].operand.expRect.xOffset = 0;
		pdata->fileFmt[3].operand.expRect.xSize = pdata->imageWidth;
		pdata->fileFmt[3].operand.expRect.yOffset = 0;
		pdata->fileFmt[3].operand.expRect.ySize = pdata->imageLength - 
										(pdata->leadingLines + pdata->trailingLines);
		
		pdata->fileFmt[4].opcode = kOmfVideoLineMap;
		pdata->fileFmt[4].operand.expLineMap[0] = pdata->videoLineMap[0];
		pdata->fileFmt[4].operand.expLineMap[1] = pdata->videoLineMap[1];

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
		
		fileFmt.mediaKind = dataKind;
		fileFmt.vfmt = pdata->fileFmt;
		fileFmt.afmt = NULL;
		CHECK(omcSetFileFormat(stream, fileFmt, swapBytes));
		CHECK(omfmSetSwabProc(stream, stdCodecSwabProc, stdCodecSwabLenProc,
								stdCodecNeedsSwabProc));		
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

static omfErr_t initUserData(userDataTIFF_t *pdata)
{
	omfHdl_t main = NULL;
	
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);
	
	pdata->filePixelFormat = kOmfPixRGBA;	/* As a default */
	pdata->compression = knoComp;	/* As a default (goes with tiff version) */
	pdata->fileLayout = kOneField;
	pdata->frameIndex = NULL;
	pdata->maxIndex = 0;
	pdata->currentIndex = 0;
	pdata->imageWidth = 0;
	pdata->imageLength = 0;
	pdata->bitsPerPixel = 24;
	pdata->bytesPerSample = 0;
	pdata->jpeg.JPEGTableID = 0;
	pdata->customTables = FALSE;
	pdata->numLines = 0;
	pdata->rate.numerator = 0;
	pdata->rate.denominator = 0;
	pdata->videoOffset = 0;
	pdata->firstIFD = 0;
	pdata->fmtOps[0].opcode = kOmfVFmtEnd;
	pdata->fileFmt[0].opcode = kOmfVFmtEnd;
	pdata->swapBytes = FALSE;
	pdata->leadingLines = 0;
	pdata->trailingLines = 0;
	pdata->fieldDominance = kDominantField1;
	pdata->videoLineMap[0] = 16;	/* Field 1 top also */
	pdata->videoLineMap[1] = 17;
	pdata->tiffFrameLayout = FRAMELAYOUT_FULLFRAME;
	pdata->tiffPixelFormat = PHOTOMETRIC_RGB;
	pdata->tiffCompressionType = COMPRESSION_NONE;
	pdata->codecCompression = FALSE;
#if ! STANDARD_JPEG_Q			
	pdata->compressionTables[0].Q16FPlen = 0;
	pdata->compressionTables[1].Q16FPlen = 0;
#endif
		
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
				                      userDataTIFF_t * pdata)
{
	omfVideoMemOp_t 	*vparms;

	omfAssertMediaHdl(media);
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);
	omfAssert(pdata, file, OM_ERR_NULL_PARAM);

	pdata = (userDataTIFF_t *) media->userData;
	
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
			     break;
			  case kOmfFrameLayout:		
			  case kOmfFieldDominance:
			  case kOmfStoredRect:
			  case kOmfAspectRatio:
			  case kOmfRGBCompLayout:
			  case kOmfRGBCompSizes:
			  case kOmfLineLength:
			  	break;
			  	
			  default:
			  	RAISE(OM_ERR_ILLEGAL_MEMFMT);
			  }
		  }
	
		CHECK(omfmVideoOpInit(file, pdata->fmtOps));
		CHECK(omfmVideoOpMerge(file, kOmfForceOverwrite, 
					(omfVideoMemOp_t *) parmblk->spc.mediaInfo.buf,
					pdata->fmtOps, MAX_FMT_OPS+1));
		CHECK(setupStream(media->stream, media->pictureKind, pdata, pdata->swapBytes));
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * codecImportRawTIFF	(INTERNAL)
 *
 * 	Open a raw TIFF file and create an MDES for it on the locator
 *		Don't add the locator, as that is the applications responsibility
 *		(We don't know what type of locator to add).
 */
omfErr_t codecImportRawTIFF(omfCodecParms_t * parmblk,
									omfMediaHdl_t 			media,
									omfHdl_t				main)
{
	omfHdl_t				file, rawFile;
	omfCodecStream_t	readStream, writeStream;
	omfType_t			dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
	omfObject_t			mdes;
	omfErr_t			status;
	omfDDefObj_t		dataKind;
	userDataTIFF_t		tmpPData;
	omfUInt16			byteOrder, tiffVersion;
	omfUInt32			ifdPos32, len32;
	omfPosition_t		ifdPosition;
	omfLength_t			len;
	
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
		omfiDatakindLookup(parmblk->spc.getChannels.file, SOUNDKIND, &dataKind, &status);
		CHECK(status);

		CHECK(omcOpenStream(file, rawFile, &readStream, NULL,
							OMNoProperty, OMNoType));
		CHECK(readDataShort(&readStream, &byteOrder));
		tmpPData.swapBytes = (byteOrder != OMNativeByteOrder);
		CHECK(setupStream(&readStream, dataKind, &tmpPData, tmpPData.swapBytes)); 
		
		CHECK(readDataShort(&readStream, &tiffVersion));
		if (tiffVersion != TIFF_VERSION)
			RAISE(OM_ERR_TIFFVERSION);
		
		/* go to the IFD */
		CHECK(readDataLong(&readStream, &ifdPos32));
		omfsCvtInt32toInt64(ifdPos32, &ifdPosition);
		CHECK(omcSeekStreamTo(&readStream, ifdPosition));

		CHECK(ReadIFD(parmblk->spc.getChannels.file, &readStream, &tmpPData));
		CHECK(omcCloseStream(&readStream));
		
		if(parmblk->fileRev == kOmfRev2x)
		{
			CHECK(omfsReadObjRef(file, parmblk->spc.rawImportInfo.fileMob,
										OMSMOBMediaDescription, &mdes));
		}
		else
		{
			CHECK(omfsReadObjRef(file, parmblk->spc.rawImportInfo.fileMob,
										OMMOBJPhysicalMedia, &mdes));
		}

		CHECK(omcOpenStream(file, file, &writeStream,  mdes, OMTIFDSummary, dataType));
		CHECK(setupStream(&writeStream, dataKind, &tmpPData, tmpPData.swapBytes)); 

		/* Assumes single frame if uncompressed raw TIFF */
		len32 = (tmpPData.maxIndex > 1 ? tmpPData.maxIndex : 1);
		omfsCvtInt32toInt64(len32, &len);
		CHECK(WriteIFD(file, &writeStream, &tmpPData, len, FALSE));
		CHECK(omcCloseStream(&writeStream));


		/*
		 * Fill in some of the MDES fields
		 */
		CHECK(omfsWriteLength(file, mdes, OMMDFLLength, len));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * codecSemCheckTIFF	(INTERNAL)
 *
 *			Semantic check the TIFF data at a higher level.
 */
omfErr_t codecSemCheckTIFF(
			omfCodecParms_t	*parmblk,
			omfMediaHdl_t 	media,
			omfHdl_t		main)
{
	omfHdl_t			file;
	omfCodecStream_t	streamData;
	omfType_t			dataType = (parmblk->fileRev == kOmfRev2x ? OMDataValue : OMVarLenBytes);
	omfDDefObj_t		dataKind;
	omfObject_t			mdes, dataObj;
	userDataTIFF_t		mdesPD, dataPD;
	omfErr_t			status;
	omfBool				getWarnings;
	omfUInt32			ifdPosition;
	omfUInt16      		mdesByteOrder, dataByteOrder;
	omfUInt16      		mdesTiffVersion, dataTiffVersion;
	
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
		omfiDatakindLookup(file, PICTUREKIND, &dataKind, &status);
		CHECK(status);

		/* First read the metadata out of the MDES Summary */
		CHECK(omcOpenStream(file, file, &streamData, mdes,
							OMTIFDSummary, dataType));
		/* Read the header */
		CHECK(readDataShort(&streamData, &mdesByteOrder));
		CHECK(readDataShort(&streamData, &mdesTiffVersion));	
		 /* go to the IFD */
		CHECK(readDataLong(&streamData, &ifdPosition));
		CHECK(omcSeekStream32(&streamData, ifdPosition));
		CHECK(ReadIFD(file, &streamData, &mdesPD));
		CHECK(omcCloseStream(&streamData));
		/* Next read the metadata out of the dataObject */
		CHECK(omcOpenStream(file, file, &streamData, dataObj,
							OMTIFFData, dataType));
		/* Read the header */
		CHECK(readDataShort(&streamData, &dataByteOrder));
		CHECK(readDataShort(&streamData, &dataTiffVersion));	
		 /* go to the IFD */
		CHECK(readDataLong(&streamData, &ifdPosition));
		CHECK(omcSeekStream32(&streamData, ifdPosition));
		CHECK(ReadIFD(file, &streamData, &dataPD));
		CHECK(omcCloseStream(&streamData));
		/* Finally, make sure that the data agrees */
		if(mdesByteOrder != dataByteOrder)
		{
			parmblk->spc.semCheck.message = "TIFF Byte Order";
			RAISE(OM_ERR_DATA_MDES_DISAGREE);
		}
		if(mdesTiffVersion != dataTiffVersion)
		{
			parmblk->spc.semCheck.message = "TIFF Version";
			RAISE(OM_ERR_DATA_MDES_DISAGREE);
		}
		if(mdesPD.tiffFrameLayout != dataPD.tiffFrameLayout)
		{
			parmblk->spc.semCheck.message = "TIFF Frame Layout";
			RAISE(OM_ERR_DATA_MDES_DISAGREE);
		}
		if(mdesPD.imageWidth != dataPD.imageWidth)
		{
			parmblk->spc.semCheck.message = "TIFF Image Width";
			RAISE(OM_ERR_DATA_MDES_DISAGREE);
		}
		if(mdesPD.imageLength != dataPD.imageLength)
		{
			parmblk->spc.semCheck.message = "TIFF Image Length";
			RAISE(OM_ERR_DATA_MDES_DISAGREE);
		}
		if(mdesPD.tiffCompressionType != dataPD.tiffCompressionType)
		{
			parmblk->spc.semCheck.message = "TIFF Compression Type";
			RAISE(OM_ERR_DATA_MDES_DISAGREE);
		}
		if(mdesPD.tiffPixelFormat != dataPD.tiffPixelFormat)
		{
			parmblk->spc.semCheck.message = "TIFF Pixel Format";
			RAISE(OM_ERR_DATA_MDES_DISAGREE);
		}
		/* Now check the compression tables (Need a numTables arg?) */
/*		for(n = 0; n <= 2; n++)
		{
			if(mdesPD.compressionTables[n].Qlen != dataPD.compressionTables[n].Qlen)
			{
				parmblk->spc.semCheck.message = "TIFF Q Table Size";
				RAISE(OM_ERR_DATA_MDES_DISAGREE);
			}
			if(mdesPD.compressionTables[n].DClen != dataPD.compressionTables[n].DClen)
			{
				parmblk->spc.semCheck.message = "TIFF DC Table Size";
				RAISE(OM_ERR_DATA_MDES_DISAGREE);
			}
			if(mdesPD.compressionTables[n].AClen != dataPD.compressionTables[n].AClen)
			{
				parmblk->spc.semCheck.message = "TIFF AC Table Size";
				RAISE(OM_ERR_DATA_MDES_DISAGREE);
			}
			if(mdesPD.compressionTables[n].Q16len != dataPD.compressionTables[n].Q16len)
			{
				parmblk->spc.semCheck.message = "TIFF 16-Bit Q Table Size";
				RAISE(OM_ERR_DATA_MDES_DISAGREE);
			}
			if(mdesPD.compressionTables[n].Qlen != 0)
			{
				if(memcmp(mdesPD.compressionTables[n].Q, dataPD.compressionTables[n].Q,
					mdesPD.compressionTables[n].Qlen) != 0)
				{
					parmblk->spc.semCheck.message = "TIFF Q Table Data";
					RAISE(OM_ERR_DATA_MDES_DISAGREE);
				}
			}
			if(mdesPD.compressionTables[n].DClen != 0)
			{
				if(memcmp(mdesPD.compressionTables[n].Q, dataPD.compressionTables[n].Q,
					mdesPD.compressionTables[n].DClen) != 0)
				{
					parmblk->spc.semCheck.message = "TIFF DC Table Data";
					RAISE(OM_ERR_DATA_MDES_DISAGREE);
				}
			}
			if(mdesPD.compressionTables[n].AClen != 0)
			{
				if(memcmp(mdesPD.compressionTables[n].Q, dataPD.compressionTables[n].Q,
					mdesPD.compressionTables[n].AClen) != 0)
				{
					parmblk->spc.semCheck.message = "TIFF AC Table Data";
					RAISE(OM_ERR_DATA_MDES_DISAGREE);
				}
			}
			if(mdesPD.compressionTables[n].Q16len != 0)
			{
				if(memcmp(mdesPD.compressionTables[n].Q, dataPD.compressionTables[n].Q,
					mdesPD.compressionTables[n].Q16len) != 0)
				{
					parmblk->spc.semCheck.message = "TIFF 16-Bit Q Table Data";
					RAISE(OM_ERR_DATA_MDES_DISAGREE);
				}
			}
		}
*/
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/****************************************************************************
 *
 *		FRAME INDEX MANAGEMENT
 *
 ****************************************************************************/

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
static omfErr_t AddFrameOffset(omfMediaHdl_t media, omfPosition_t pos)
{
	omfInt32           mni;
	omfUInt32         i, ci;
	omfUInt32         *tmp_array;
	omfHdl_t        main;
	userDataTIFF_t *pdata;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	XPROTECT(main)
	{
		pdata = (userDataTIFF_t *) media->userData;
		ci = ++pdata->currentIndex;
	
		if (ci > pdata->maxIndex)
		{
	
			tmp_array = pdata->frameIndex;
			pdata->maxIndex += 1024;
			mni = pdata->maxIndex;
	
			pdata->frameIndex = (omfUInt32 *) omOptMalloc(main, (size_t) mni * sizeof(omfInt32));
			if (pdata->frameIndex == NULL)
				RAISE(OM_ERR_NOMEMORY);
	
			for (i = 0; i < ci - 1; i++)	/* copy old storage into new */
				pdata->frameIndex[i] = tmp_array[i];
	
			if (tmp_array)
				omOptFree(main, (void *) tmp_array);
		}
		CHECK(omfsTruncInt64toUInt32(pos, &pdata->frameIndex[ci - 1]));	/* OK 1.x */
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
