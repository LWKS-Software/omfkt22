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
#include "omcRGBA.h"
#include "omPvt.h"

#define MAX_FMT_OPS	32

typedef struct
{
	omfProperty_t	OMRGBAPixelLayout;
	omfProperty_t	OMRGBAPixelStructure;
	omfProperty_t	OMRGBAPalette;
	omfProperty_t	OMRGBAPaletteLayout;
	omfProperty_t	OMRGBAPaletteStructure;
}				omcRGBAPersistent_t;

typedef struct
{
	omfInt32           	imageWidth;
	omfInt32           	imageLength;
	omfInt32			sampledWidth;
	omfInt32			sampledLength;
	omfInt32			sampledXOffset;
	omfInt32			sampledYOffset;
	omfInt32			displayWidth;
	omfInt32			displayLength;
	omfInt32			displayXOffset;
	omfInt32			displayYOffset;
	omfRational_t   	rate;
	omfFrameLayout_t 	fileLayout;	/* Frame of fields per sample */
	omfPixelFormat_t 	filePixelFormat;
	omfUInt32           bytesPerSample;
	omfInt16			bitsPerPixel;
	omfInt32          	numLines;
	omfVideoMemOp_t		memFmt[MAX_FMT_OPS+1];
	omfVideoMemOp_t		fileFmt[MAX_FMT_OPS+1];
	omfRational_t		imageAspectRatio;
	omfBool				descriptorFlushed;
	omfBool				dataWritten;
	omfInt32			alignment;
	omfInt32			saveAlignment;
	char 				*alignmentBuffer;
	omfFieldDom_t		fieldDominance;
	unsigned char 		filePixSizes[MAX_NUM_RGBA_COMPS];
	char 				filePixComps[MAX_NUM_RGBA_COMPS];
	unsigned char 		memPixSizes[MAX_NUM_RGBA_COMPS];
	char 				memPixComps[MAX_NUM_RGBA_COMPS];
	omfInt32			videoLineMap[2];
}   userDataRGBA_t;

typedef struct
{
	omfObject_t     obj;
	omfProperty_t   prop;
	omfInt64          offset;
}               streamRefData_t;

static omfErr_t myCodecInit(omfCodecParms_t * parmblk);
static omfErr_t initUserData(omfHdl_t file, userDataRGBA_t *pdata);
static omfErr_t myCodecIsValidMDES(omfCodecParms_t * parmblk, 
								   omfMediaHdl_t media, 
								   omfHdl_t file, 
								   userDataRGBA_t * pdata);
static omfErr_t myCodecOpen(omfCodecParms_t * parmblk, 
							omfMediaHdl_t media, omfHdl_t main);
static omfErr_t myCodecCreate(omfCodecParms_t * parmblk, 
							  omfMediaHdl_t media, omfHdl_t main);
static omfErr_t myCodecClose(omfCodecParms_t * parmblk, 
							 omfMediaHdl_t media, omfHdl_t main);
static omfErr_t myCodecGetFileInfo(omfCodecParms_t * parmblk, 
									omfMediaHdl_t media, 
									omfHdl_t file);
static omfErr_t myCodecPutFileInfo(omfCodecParms_t * parmblk, 
									omfMediaHdl_t media, 
									omfHdl_t file);
static omfErr_t myCodecSetMemoryInfo(omfCodecParms_t * parmblk, 
									omfMediaHdl_t media, 
									omfHdl_t file);
static omfErr_t myCodecWriteSamples(omfCodecParms_t * parmblk, 
									omfMediaHdl_t media, omfHdl_t main);
static omfErr_t myCodecReadSamples(omfCodecParms_t * parmblk, 
								   omfMediaHdl_t media, omfHdl_t main);
static omfErr_t myCodecReadLines(omfCodecParms_t * parmblk, 
								 omfMediaHdl_t media, omfHdl_t main);
static omfErr_t myCodecWriteLines(omfCodecParms_t * parmblk, 
								  omfMediaHdl_t media, 
								  omfHdl_t file);
static omfErr_t myCodecSetFrameNumber(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t myCodecGetFrameOffset(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t myCodecGetMaxSampleSize(omfCodecParms_t * parmblk,
					                 omfMediaHdl_t media,
					                 omfHdl_t file);
				                     
static omfErr_t myCodecSetRect(omfCodecParms_t * parmblk, 
							   omfMediaHdl_t media, 
							   omfHdl_t file, 
							   userDataRGBA_t * pdata);
static omfErr_t myCodecGetRect(omfCodecParms_t * parmblk, 
							   omfMediaHdl_t media, 
							   omfHdl_t file, 
							   userDataRGBA_t * pdata);
static omfErr_t ReconcileInfo(omfMediaHdl_t media, 
							  userDataRGBA_t * pdata);
static omfErr_t postWriteOps(omfCodecParms_t * parmblk, 
							 omfMediaHdl_t media);
static omfErr_t writeDescriptorData(omfCodecParms_t * parmblk, 
									omfMediaHdl_t media, 
									omfHdl_t file, 
									userDataRGBA_t * pdata);
static omfErr_t loadDescriptorData(omfCodecParms_t * parmblk, 
								   omfMediaHdl_t media, 
								   omfHdl_t file, 
								   userDataRGBA_t * pdata);
static omfErr_t setupStream(omfMediaHdl_t media, 
							userDataRGBA_t * pdata);


#if OMFI_CODEC_DIAGNOSTICS
static omfErr_t myCodecDiags(omfCodecParms_t * parmblk, 
							 omfMediaHdl_t media, omfHdl_t main);
#endif

static omfErr_t openMyCodecStream(omfCodecStream_t *stream, 
								  omfObject_t obj, 
								  omfProperty_t prop, 
								  omfType_t type);
static omfErr_t writeMyCodecStream(omfCodecStream_t *stream, 
								   omfUInt32 ByteCount, 
								   void *Buffer);
static omfErr_t readMyCodecStream(omfCodecStream_t *stream, 
								  omfUInt32 ByteCount, 
								  void *Buffer, 
								  omfUInt32 *bytesRead );
static omfErr_t seekMyCodecStream(omfCodecStream_t *stream, 
								  omfInt64 position);
static omfErr_t lengthMyCodecStream(omfCodecStream_t *stream, 
									omfInt64 *bytesLeft);
static omfErr_t truncMyCodecStream(omfCodecStream_t *stream, 
								   omfInt64 newSize);
static omfErr_t closeMyCodecStream(omfCodecStream_t *stream);
static omfErr_t myCodecSwabProc(omfCodecStream_t *stream, 
								omfCStrmSwab_t * src,
				                 omfCStrmSwab_t * dest, 
								omfBool swapBytes);
static omfErr_t InitMDESProps(omfCodecParms_t * parmblk, omfMediaHdl_t media, omfHdl_t main);
static omfErr_t myCodecGetInfo(omfCodecParms_t * parmblk,
							omfMediaHdl_t 			media,
							omfHdl_t				main);

static omfErr_t myCodecPutInfo(omfCodecParms_t * parmblk,
							omfMediaHdl_t 			media,
							omfHdl_t				main);
static omfErr_t myCodecNumChannels(omfCodecParms_t * parmblk,
							omfMediaHdl_t 			media,
							omfHdl_t				main);

static omfErr_t myCodecSelectInfo(omfCodecParms_t * parmblk,
							omfMediaHdl_t 			media,
							omfHdl_t				main);

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
 *   omfCodecRGBA is the main entry point, subfunctions will be called based on formal arg
 *   func.
 */
 
omfErr_t omfCodecRGBA(omfCodecFunctions_t func, 
					  omfCodecParms_t * parmblk)
{
	omfErr_t        status = OM_ERR_NONE;
	omfMediaHdl_t   media = NULL;
	omfHdl_t        file = NULL;
	userDataRGBA_t *pdata = NULL;
	omcRGBAPersistent_t *pers = (omcRGBAPersistent_t *)parmblk->pers;
	omfCodecOptDispPtr_t	*dispPtr;
	
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);

	switch (func)
	{
	
	case kCodecInit:
	  /* if codec-persistent data exists, then init has been called */
		if (parmblk->pers) 
			return(OM_ERR_NONE); /* let it slide, much like guarded includes */
		else
			return(myCodecInit(parmblk));

	case kCodecGetMetaInfo:
		strncpy((char *)parmblk->spc.metaInfo.name, "RGBA Codec", 
				parmblk->spc.metaInfo.nameLen);
		parmblk->spc.metaInfo.info.mdesClassID = "RGBA";
		parmblk->spc.metaInfo.info.dataClassID = "IDAT";
		parmblk->spc.metaInfo.info.codecID = CODEC_RGBA_VIDEO;
		parmblk->spc.metaInfo.info.rev = CODEC_REV_3;
		parmblk->spc.metaInfo.info.dataKindNameList = PICTUREKIND;
		parmblk->spc.metaInfo.info.minFileRev = kOmfRev2x;
		parmblk->spc.metaInfo.info.maxFileRevIsValid = FALSE;		/* There is no maximum rev */
		return (OM_ERR_NONE);

	case kCodecLoadFuncPointers:
		dispPtr = parmblk->spc.loadFuncTbl.dispatchTbl;

		dispPtr[ kCodecOpen ] = myCodecOpen;
		dispPtr[ kCodecCreate ] = myCodecCreate;
		dispPtr[ kCodecGetInfo ] = myCodecGetInfo;
		dispPtr[ kCodecPutInfo ] = myCodecPutInfo;
		dispPtr[ kCodecReadSamples ] = myCodecReadSamples;
		dispPtr[ kCodecWriteSamples ] = myCodecWriteSamples;
		dispPtr[ kCodecReadLines ] = myCodecReadLines;
		dispPtr[ kCodecWriteLines ] = myCodecWriteLines;
		dispPtr[ kCodecClose ] = myCodecClose;
		dispPtr[ kCodecSetFrame ] = myCodecSetFrameNumber;
		dispPtr[ kCodecGetFrameOffset ] = myCodecGetFrameOffset;
		dispPtr[ kCodecGetNumChannels ] = myCodecNumChannels;
		dispPtr[ kCodecInitMDESProps ] = InitMDESProps;
		dispPtr[ kCodecGetSelectInfo ] = myCodecSelectInfo;
/*		dispPtr[ kCodecImportRaw ] = NULL;
		dispPtr[ kCodecGetVarietyInfo ] = NULL;
		dispPtr[ kCodecSemanticCheck ] = NULL;
		dispPtr[ kCodecAddFrameIndexEntry ] = NULL;
		dispPtr[ kCodecGetVarietyCount ] = NULL;
*/
		return(OM_ERR_NONE);
	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}

	return (status);
}

static omfErr_t myCodecGetInfo(omfCodecParms_t * parmblk,
							omfMediaHdl_t 			media,
							omfHdl_t				main)
{
	omfErr_t		status = OM_ERR_NONE;
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;
	
	switch (parmblk->spc.mediaInfo.infoType)
	{
	case kMediaIsContiguous:
		status = OMIsPropertyContiguous(main, media->dataObj,OMIDATImageData,
										OMDataValue, (omfBool *)parmblk->spc.mediaInfo.buf);
		break;
		
	case kVideoInfo:
		status = myCodecGetFileInfo(parmblk, media, main);
		break;
		
	case kCompressionParms:
		status = OM_ERR_BADCOMPR;
		break;
		
	case kJPEGTables:
		status = OM_ERR_BADCOMPR;
		break;
		
	case kMaxSampleSize:
		status = myCodecGetMaxSampleSize(parmblk, media, main);
		break;
					
	case kVideoMemFormat:
	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}
	
	return(status);
}
static omfErr_t myCodecPutInfo(omfCodecParms_t * parmblk,
							omfMediaHdl_t 			media,
							omfHdl_t				main)
{
	omfErr_t		status = OM_ERR_NONE;
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;

	switch (parmblk->spc.mediaInfo.infoType)
	{
	case kVideoInfo:
		status = myCodecPutFileInfo(parmblk, media, main);
		break;
	case kCompressionParms:
		status = OM_ERR_BADCOMPR;
		break;
	case kVideoMemFormat:
		status = myCodecSetMemoryInfo(parmblk, media, main);
		break;
	case kJPEGTables:
		status = OM_ERR_BADCOMPR;
		break;

	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}
	
	return(status);
}
static omfErr_t myCodecNumChannels(omfCodecParms_t * parmblk,
							omfMediaHdl_t 			media,
							omfHdl_t				main)
{
	omfErr_t		status = OM_ERR_NONE;
	omfHdl_t		file;
	omfDDefObj_t	dataKind;
	
	file = parmblk->spc.getChannels.file;
	omfiDatakindLookup(file, PICTUREKIND, &dataKind, &status);
	if (parmblk->spc.getChannels.mediaKind == dataKind)
		parmblk->spc.getChannels.numCh = 1;
	else
		parmblk->spc.getChannels.numCh = 0;
	
	return(status);
}

static omfErr_t myCodecSelectInfo(omfCodecParms_t * parmblk,
							omfMediaHdl_t 			media,
							omfHdl_t				main)
{
	omfErr_t		status = OM_ERR_NONE;
	unsigned char 	pixSizes[MAX_NUM_RGBA_COMPS];
	omfPosition_t	zeroPos;
	omfInt32		storedWidth, storedHeight, i, bitsPerPixel;
	omcRGBAPersistent_t *pers = (omcRGBAPersistent_t *)parmblk->pers;
    omfHdl_t		file;

	file = parmblk->spc.selectInfo.file;
    XPROTECT(file)
    {
		omfsCvtInt32toPosition(0, zeroPos);
		parmblk->spc.selectInfo.info.willHandleMDES = TRUE;
		parmblk->spc.selectInfo.info.isNative = TRUE;
		parmblk->spc.selectInfo.info.hwAssisted = FALSE;
		parmblk->spc.selectInfo.info.relativeLoss = 0;
		/***/
		CHECK(omfsReadInt32(file, parmblk->spc.selectInfo.mdes, OMDIDDStoredWidth, &storedWidth));
		CHECK(omfsReadInt32(file, parmblk->spc.selectInfo.mdes, OMDIDDStoredHeight, &storedHeight));
		for (i = 0; i < MAX_NUM_RGBA_COMPS; i++)
			pixSizes[i] = 0;
		status = OMReadProp(file, parmblk->spc.selectInfo.mdes, pers->OMRGBAPixelStructure,
						  zeroPos, 
						  kNeverSwab, OMCompSizeArray,  
						  MAX_NUM_RGBA_COMPS, (void *)pixSizes);

		/* allow end of data because compsizearray is variable length */
		if (status != OM_ERR_NONE && status != OM_ERR_END_OF_DATA) 
		  RAISE(status);
								
		bitsPerPixel = 0;
		{
		    i = 0;
			while (i < MAX_NUM_RGBA_COMPS && pixSizes[i])
				bitsPerPixel += pixSizes[i++];
		}
		/****/				
		parmblk->spc.selectInfo.info.avgBitsPerSec = storedWidth * storedHeight * bitsPerPixel;
    }
    XEXCEPT
    XEND

	return(OM_ERR_NONE);
}
/*
	codecInit initializes storage for this module.  It should only be called
	once, the main dispatch routine should check for multiple calls...
 */
 
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
static omfErr_t myCodecInit(omfCodecParms_t * parmblk)
{

	omcRGBAPersistent_t	*persist;
	omfHdl_t			file = NULL;
	
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);
	/* first, allocate a persistent "cookie" that will hold codec-dynamic 
	 * values, like property and type handles 
	 */
	   
	XPROTECT(NULL)
	{
		persist = (omcRGBAPersistent_t *)
		  omOptMalloc(NULL, sizeof(omcRGBAPersistent_t));
		XASSERT(persist != NULL, OM_ERR_NOMEMORY);
	
		/* put the cookie pointer in the parameter block for return */
		parmblk->spc.init.persistRtn = persist;
					
		/* LF-W: Moved dynamic registration of CompCodeArray & CompSizeArray
		 * to static registration in omFile.c 
		 */
	
		/* add the codec-related properties.  See the OMF Interchange 
		 * Specification 2.0 for details.
		 */
	    CHECK(omfsRegisterDynamicProp(parmblk->spc.init.session, 
				  kOmfTstRevEither, "PixelLayout", OMClassRGBA, 
				  OMCompCodeArray, kPropRequired,
				  &persist->OMRGBAPixelLayout));	
	   	CHECK(omfsRegisterDynamicProp(parmblk->spc.init.session, 
					kOmfTstRevEither, "PixelStructure", OMClassRGBA, 
					OMCompSizeArray, kPropOptional,
					&persist->OMRGBAPixelStructure));
		CHECK(omfsRegisterDynamicProp(parmblk->spc.init.session, 
					  kOmfTstRevEither, "Palette", OMClassRGBA, OMDataValue, 
					  kPropOptional, &persist->OMRGBAPalette));
	    CHECK(omfsRegisterDynamicProp(parmblk->spc.init.session, 
                  kOmfTstRevEither, "PaletteLayout", OMClassRGBA, 
				  OMCompCodeArray,
				  kPropOptional, &persist->OMRGBAPaletteLayout));
	    CHECK(omfsRegisterDynamicProp(parmblk->spc.init.session, 
					  kOmfTstRevEither, "PaletteStructure", OMClassRGBA, 
                      OMCompSizeArray, kPropOptional,
	                  &persist->OMRGBAPaletteStructure));
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
static omfErr_t initUserData(omfHdl_t file, userDataRGBA_t *pdata)
{
	omfErr_t err;
	
	omfAssert(pdata, file, OM_ERR_NULL_PARAM);
	
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
	pdata->bytesPerSample = 0;
	pdata->bitsPerPixel = 24;
	pdata->rate.numerator = 0;
	pdata->rate.denominator = 1;
	pdata->fileLayout = kNoLayout;	/* Frame of fields per sample */
	pdata->filePixelFormat = kOmfPixRGBA;
	pdata->filePixComps[0] = 'R';
	pdata->filePixComps[1] = 'G';
	pdata->filePixComps[2] = 'B';	
	pdata->filePixComps[3] = 0;
	pdata->filePixSizes[0] = 8;
	pdata->filePixSizes[1] = 8;
	pdata->filePixSizes[2] = 8;
	pdata->filePixSizes[3] = 0;
	pdata->fieldDominance = kNoDominant;
	pdata->videoLineMap[0] = 0;
	pdata->videoLineMap[1] = 0;

	pdata->numLines = 0;
	
	err = omfmVideoOpInit(file, pdata->memFmt);
	if (err != OM_ERR_NONE)
		return(err);
		
	err = omfmVideoOpInit(file, pdata->fileFmt);
	if (err != OM_ERR_NONE)
		return(err);
		
	pdata->fileFmt[0].opcode = kOmfFieldDominance;
	pdata->fileFmt[0].operand.expFieldDom = pdata->fieldDominance;
	pdata->fileFmt[1].opcode = kOmfRGBCompLayout;
	memcpy(pdata->fileFmt[1].operand.expCompArray, pdata->filePixComps,
			sizeof(pdata->fileFmt[1].operand.expCompArray));
	pdata->fileFmt[2].opcode = kOmfRGBCompSizes;
	memcpy(pdata->fileFmt[2].operand.expCompSizeArray, pdata->filePixSizes,
			sizeof(pdata->fileFmt[2].operand.expCompSizeArray));
	pdata->fileFmt[3].opcode = kOmfPixelFormat;
	pdata->fileFmt[3].operand.expPixelFormat = kOmfPixRGBA;
	pdata->fileFmt[4].opcode = kOmfVFmtEnd;
	
	pdata->imageAspectRatio.numerator = 0;
	pdata->imageAspectRatio.denominator = 0;
	pdata->descriptorFlushed = FALSE;
	pdata->dataWritten = FALSE;
	pdata->alignment = 1;
	pdata->saveAlignment = 1;
	pdata->alignmentBuffer = NULL;
	
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
static omfErr_t myCodecOpen(omfCodecParms_t * parmblk,
							omfMediaHdl_t 			media,
							omfHdl_t				main)
{
	userDataRGBA_t 	*pdata;

	omfAssertValidFHdl(main);
	omfAssertMediaHdl(media);

	/* pull out descriptor information BEFORE reading RGBA data */
	XPROTECT(main)
	{
		media->userData = (userDataRGBA_t *)omOptMalloc(main, sizeof(userDataRGBA_t));
		XASSERT(media->userData != NULL, OM_ERR_NOMEMORY);
			
		pdata = (userDataRGBA_t *) media->userData;
		
		CHECK(initUserData(main, pdata));
			
		CHECK(loadDescriptorData(parmblk, media, main, pdata));
								
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
static omfErr_t myCodecCreate(	omfCodecParms_t * parmblk, 
							 	omfMediaHdl_t media,
				                omfHdl_t main)
{
	omcRGBAPersistent_t *pers;
	userDataRGBA_t *pdata;

	omfAssertValidFHdl(main);
	omfAssertMediaHdl(media);
	
	XPROTECT(main)
	{
		pers  = (omcRGBAPersistent_t *)parmblk->pers;
		
		/* copy descriptor information into local structure */
		media->userData = (userDataRGBA_t *)omOptMalloc(main, sizeof(userDataRGBA_t));
		XASSERT(media->userData != NULL, OM_ERR_NOMEMORY);
	
		pdata = (userDataRGBA_t *) media->userData;
		
		CHECK(initUserData(main, pdata));
		
		/* there are no multiword entities (YET!) */
		media->stream->swapBytes = FALSE; 
	
		/* get sample rate from MDFL parent class of descriptor.  
		 * This assumes that the RGBA descriptor class is a descendant of 
		 * MDFL. 
		 */
		if(parmblk->fileRev == kOmfRev1x || parmblk->fileRev == kOmfRevIMA)
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
	
		pdata->filePixelFormat = kOmfPixRGBA;	/* As a default */
		pdata->fileLayout = kOneField;
		pdata->bytesPerSample = 3;			/* As a default */
		
		MakeRational(4, 3, &(pdata->imageAspectRatio));
		
		CHECK(omfsFileGetValueAlignment(main,(omfUInt32*)&(pdata->saveAlignment)));
	
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
static omfErr_t myCodecClose(omfCodecParms_t * parmblk, 
							 omfMediaHdl_t media,
							 omfHdl_t main)
{
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;

	omfAssertValidFHdl(main);
	omfAssertMediaHdl(media);

	XPROTECT(main)
	{
		/* If the media was written, perform post-write operations 
		 */
		CHECK(omcCloseStream(media->stream));
		if((media->openType == kOmfiCreated) || (media->openType == kOmfiAppended))
		{
			/* Roger-- restore normal alignment requirements */
			CHECK(omfsFileSetValueAlignment(media->dataFile, pdata->saveAlignment));
			if (pdata->alignmentBuffer)
			{
				omOptFree(main, (void *)pdata->alignmentBuffer);
				pdata->alignmentBuffer = NULL;
			}
			
			CHECK(postWriteOps(parmblk, media));
			if ( ! pdata->descriptorFlushed )
				CHECK(writeDescriptorData(parmblk, media, main, pdata));
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
 * 		myCodecGetFileInfo
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
static omfErr_t myCodecGetFileInfo(omfCodecParms_t * parmblk,
				                      omfMediaHdl_t media,
				                      omfHdl_t main)
{
	omfVideoMemOp_t *vparms;
	omfInt32 		RGBFrom, RGBTo;
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;

	omfAssertValidFHdl(main);
	omfAssertMediaHdl(media);
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
	vparms = ((omfVideoMemOp_t *)parmblk->spc.mediaInfo.buf);
	for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
	  {
		switch(vparms->opcode)
		  {
			case kOmfPixelFormat:
				vparms->operand.expPixelFormat = pdata->filePixelFormat;
				break;
			case kOmfFrameLayout:
				vparms->operand.expFrameLayout = pdata->fileLayout;
				break;
		
		  	case kOmfImageAlignmentFactor:
		  		vparms->operand.expInt32 = pdata->alignment;
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
				vparms->operand.expRect.xOffset = 0;
				vparms->operand.expRect.yOffset = 0;
				vparms->operand.expRect.xSize = pdata->imageWidth;
				vparms->operand.expRect.ySize = pdata->imageLength;
				break;
				
			  case kOmfPixelSize:
				vparms->operand.expInt16 = pdata->bitsPerPixel;
				break;
				
			  case kOmfAspectRatio:
				vparms->operand.expRational = pdata->imageAspectRatio;
				break;
				
			  case kOmfRGBCompLayout:
				RGBFrom = RGBTo = 0;
	
				while (pdata->filePixComps[RGBFrom])
					vparms->operand.expCompArray[RGBTo++] = pdata->filePixComps[RGBFrom++];
				vparms->operand.expCompArray[RGBTo] = 0; /* terminating zero */
				break;

			  case kOmfRGBCompSizes:
				RGBFrom = RGBTo = 0;
		
				while (pdata->filePixSizes[RGBFrom])
					vparms->operand.expCompSizeArray[RGBTo++] = pdata->filePixSizes[RGBFrom++];
				vparms->operand.expCompSizeArray[RGBTo] = 0; /* terminating zero */
				break;
			
			case kOmfWillTransferLines:
				vparms->operand.expBoolean = TRUE;
				break;
			
			case kOmfFieldDominance:
				vparms->operand.expFieldDom = pdata->fieldDominance;
				break;
			  
			case kOmfVideoLineMap:
			 	vparms->operand.expLineMap[0] = pdata->videoLineMap[0];
			 	vparms->operand.expLineMap[1] = pdata->videoLineMap[1];
			  	break;
			  	
			case kOmfIsCompressed:
				vparms->operand.expBoolean = FALSE;
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
 * 		myCodecPutFileInfo
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
static omfErr_t myCodecPutFileInfo(omfCodecParms_t * parmblk,
				                      omfMediaHdl_t media,
				                      omfHdl_t main)
{
	omfVideoMemOp_t 	*vparms;
	omfInt32			RGBFrom, RGBTo;
	omfUInt32			numFields;
	omfRect_t			*aRect;
	omfUInt32			lineMapFlags = 0;
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;

	omfAssertValidFHdl(main);
	omfAssertMediaHdl(media);
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
		vparms = ((omfVideoMemOp_t *)parmblk->spc.mediaInfo.buf);
		for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
		  {
			switch(vparms->opcode)
			  {
			  	case kOmfPixelFormat:
					pdata->filePixelFormat = vparms->operand.expPixelFormat;
					break;
				
			  	case kOmfFrameLayout:
					pdata->fileLayout = vparms->operand.expFrameLayout;
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
				  	
			  	case kOmfRGBCompLayout:
			   		RGBFrom = RGBTo = 0;
				  	while (vparms->operand.expCompArray[RGBFrom])
				  		pdata->filePixComps[RGBTo++] = 
						  vparms->operand.expCompArray[RGBFrom++];
				  	pdata->filePixComps[RGBTo] = 0; /* terminating zero */
				  	break;
				
			  	case kOmfRGBCompSizes:
					RGBFrom = RGBTo = 0;
		
			  		pdata->bitsPerPixel = 0;
			  		while (vparms->operand.expCompSizeArray[RGBFrom])
			  		{
			  			pdata->bitsPerPixel += 
						  vparms->operand.expCompSizeArray[RGBFrom];
			  			pdata->filePixSizes[RGBTo++] = 
						  vparms->operand.expCompSizeArray[RGBFrom++];
			  		}
			 	 	pdata->filePixSizes[RGBTo] = 0; /* terminating zero */
			 	 	break;
				
			  	default:
			  		RAISE(OM_ERR_ILLEGAL_FILEFMT);
			  }
		  }
		  
		CHECK(omfmVideoOpMerge(main, kOmfForceOverwrite, 
					(omfVideoMemOp_t *) parmblk->spc.mediaInfo.buf,
					pdata->fileFmt, MAX_FMT_OPS+1));

		switch (pdata->fileLayout)
		{
	
			case kFullFrame:
			case kOneField:
				numFields = 1;
				break;
		
			case kSeparateFields:
			case kMixedFields:
				if(lineMapFlags == 0)
				{
					pdata->videoLineMap[0] = 0;
					pdata->videoLineMap[1] = 1;
				}	
				numFields = 2;
				break;
		
			default:
				break;
		
		} /* end switch */
		pdata->bytesPerSample = (pdata->bitsPerPixel/8) * pdata->imageWidth * 
		  pdata->imageLength * numFields;
		  
		if (lineMapFlags && lineMapFlags != 3)
			RAISE(OM_ERR_ILLEGAL_FILEFMT);
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
static omfErr_t myCodecSetMemoryInfo(omfCodecParms_t * parmblk,
				                      omfMediaHdl_t media,
				                      omfHdl_t main)
{
	omfVideoMemOp_t 	*vparms;
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;

	omfAssertValidFHdl(main);
	omfAssertMediaHdl(media);
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);
	
	XPROTECT(main)
	{

		/* validate opcodes individually.  Some don't apply */
		
		vparms = ((omfVideoMemOp_t *)parmblk->spc.mediaInfo.buf);
		for( ; vparms->opcode != kOmfVFmtEnd; vparms++)
		  {
			switch(vparms->opcode)
			  {
			  /* put the good ones up front */
			  case kOmfPixelFormat:
			  case kOmfFrameLayout:		
			  case kOmfFieldDominance:
			  case kOmfStoredRect:
			  case kOmfAspectRatio:
			  case kOmfRGBCompLayout:
			  case kOmfRGBCompSizes:
			  case kOmfRGBPalette:
			  case kOmfRGBPaletteLayout:
			  case kOmfRGBPaletteSizes:
			  case kOmfLineLength:
			  	break;
			  	
			  default:
			  	RAISE(OM_ERR_ILLEGAL_MEMFMT);
			  }
		  }
	
		CHECK(omfmVideoOpInit(main, pdata->memFmt));
		CHECK(omfmVideoOpMerge(main, kOmfForceOverwrite, 
					(omfVideoMemOp_t *) parmblk->spc.mediaInfo.buf,
					pdata->memFmt, MAX_FMT_OPS+1));
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
static omfErr_t myCodecSetFrameNumber(omfCodecParms_t * parmblk,
				                       omfMediaHdl_t media,
				                       omfHdl_t main)
{
	omfInt64          nBytes, zero;
	omfUInt32			bytesPerSample, remainder;
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;

	XPROTECT(main)
	{
		omfsCvtInt32toInt64(0, &zero);
	
		omfAssertValidFHdl(main);
		omfAssertMediaHdl(media);
		XASSERT(parmblk != NULL, OM_ERR_NULL_PARAM);
		XASSERT(pdata != NULL, OM_ERR_NULL_PARAM);
	
		XASSERT(media->dataObj != NULL, OM_ERR_INTERNAL_MDO);
	
		nBytes = parmblk->spc.setFrame.frameNumber;
	
		XASSERT(omfsInt64NotEqual(nBytes, zero), OM_ERR_BADFRAMEOFFSET);
		omfsSubInt32fromInt64(1, &nBytes);
		bytesPerSample = pdata->bytesPerSample;
		
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
		omfsMultInt32byInt64(bytesPerSample, nBytes, &nBytes);
		CHECK(omcSeekStreamTo(media->stream, nBytes));
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * myCodecGetFrameOffset
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
static omfErr_t myCodecGetFrameOffset(omfCodecParms_t * parmblk,
				                       omfMediaHdl_t media,
				                       omfHdl_t main)
{
	omfInt64          nBytes, zero;
	omfUInt32			bytesPerSample, remainder;
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;

	XPROTECT(main)
	{
		omfsCvtInt32toInt64(0, &zero);
	
		omfAssertValidFHdl(main);
		omfAssertMediaHdl(media);
		XASSERT(parmblk != NULL, OM_ERR_NULL_PARAM);
		XASSERT(pdata != NULL, OM_ERR_NULL_PARAM);
	
		XASSERT(media->dataObj != NULL, OM_ERR_INTERNAL_MDO);
	
		nBytes = parmblk->spc.getFrameOffset.frameNumber;
	
		XASSERT(omfsInt64NotEqual(nBytes, zero), OM_ERR_BADFRAMEOFFSET);
		omfsSubInt32fromInt64(1, &nBytes);
		bytesPerSample = pdata->bytesPerSample;
		
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
		omfsMultInt32byInt64(bytesPerSample, nBytes, &nBytes);

		parmblk->spc.getFrameOffset.frameOffset = nBytes;
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
static omfErr_t myCodecGetMaxSampleSize(omfCodecParms_t * parmblk,
					                 omfMediaHdl_t media,
					                 omfHdl_t main)
{
	omfMaxSampleSize_t *maxlenstruct = (omfMaxSampleSize_t *) parmblk->spc.mediaInfo.buf;
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;

	omfAssertValidFHdl(main);
	omfAssertMediaHdl(media);
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
		XASSERT(parmblk->spc.mediaInfo.bufLen != sizeof(omfInt32), 
				OM_ERR_INTERN_TOO_SMALL)	
		maxlenstruct->largestSampleSize = pdata->bytesPerSample;
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
static omfErr_t myCodecWriteSamples(omfCodecParms_t * parmblk,
										omfMediaHdl_t 			media,
										omfHdl_t				main)
{
	omfmMultiXfer_t *xfer = NULL;
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;
	omfInt32		 blockSize;
	omfUInt32		fileBytes, memBytes;
	
	
	omfAssertValidFHdl(main);
	omfAssertMediaHdl(media);
	
	XPROTECT(main)
	{
		XASSERT(pdata, OM_ERR_NULL_PARAM);
	
		XASSERT(pdata->bitsPerPixel != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->bytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		xfer = parmblk->spc.mediaXfer.xfer;
		XASSERT(xfer, OM_ERR_NULLBUF);
		
		/* this codec only allows one-channel media */
		XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
	
		/* set alignment just before first write.  This ensures that the first
		   byte of data in the stream will be aligned to the field alignment 
		   factor
		 */
		 
		if ( ! pdata->dataWritten ) 
		{
			CHECK(omfsFileSetValueAlignment(media->dataFile, pdata->alignment));
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
		 
		if (pdata->alignment > 1)  /* only bother precomputing if necessary */
		{
			blockSize = pdata->bytesPerSample;  /* size per frame */
			if ((pdata->fileLayout == kSeparateFields) ||
			    (pdata->fileLayout == kMixedFields))
				blockSize /=2;   /* A block is half-size - equal for each field */
		}
		
		/* align if blockSize not already an even multiple of alignment factor */		
		if ((pdata->alignment > 1) && (blockSize % pdata->alignment != 0))
		{
			omfInt32 i, bufIndex;
			omfInt32 numLoops;
			omfInt32 remainderBytes;
			omfInt64 position;
			omfInt32 pos32;
			
			XASSERT(pdata->alignmentBuffer != NULL, OM_ERR_NULL_PARAM);
			 /* ROGER FIX THIS ERROR NUMBER, NULL_PARAM is not quite right */			

			/* work one item at a time.  An item is a field for kSeparateFields,
			   a frame otherwise.
			 */
			numLoops = xfer->numSamples;
			
			if ((pdata->fileLayout == kSeparateFields) ||
			    (pdata->fileLayout == kMixedFields))
				numLoops *= 2;   /* Twice as many items */
			
			for (i = 0, bufIndex = 0; i < numLoops; i++, bufIndex += blockSize)
			{
				CHECK(omcWriteStream(media->stream, blockSize, 
									 &(((char *)xfer->buffer)[bufIndex])));
									 									 
				CHECK(omcGetStreamPosition(media->stream, &position));
				
				if(pdata->alignment == 4096)
				{
				  omfsDecomposeInt64(position, NULL, &pos32);
				  remainderBytes = ((omfUInt32)pos32 % pdata->alignment);
				}
				else
				  omfsDivideInt64byInt32(position, pdata->alignment,
										 NULL, &remainderBytes);
				if (remainderBytes  != 0)
				{
					omfInt32 padAmount = pdata->alignment - remainderBytes;
					CHECK(omcWriteStream(media->stream, padAmount,
										pdata->alignmentBuffer));
				}
			}

			/* return the number of bytes transferred for consistency check.  
			 * xfer->buflen is okay, if there were any problems, the writeStream 
			 * call would have failed.
			 * Trust me... :-)   */
			xfer->bytesXfered = xfer->buflen;
			xfer->samplesXfered = xfer->buflen / pdata->bytesPerSample;
		}
		else /* byte-aligned, simply write out the data */
		{
			CHECK(omfmSetSwabBufSize(media->stream, pdata->bytesPerSample));
			fileBytes = xfer->numSamples * pdata->bytesPerSample;
			CHECK(omcComputeBufferSize(media->stream, fileBytes, omcComputeMemSize, &memBytes));
			if(memBytes > xfer->buflen)
				RAISE(OM_ERR_SMALLBUF);
	
			CHECK(omcWriteSwabbedStream(media->stream, fileBytes,
										memBytes, xfer->buffer));
			xfer->bytesXfered = memBytes;
			xfer->samplesXfered = memBytes / pdata->bytesPerSample;
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
static omfErr_t myCodecReadSamples(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;
	omfUInt32		fileBytes = 0, memBytes;
	omfUInt32		bytesRead = 0;
	omfmMultiXfer_t *xfer = NULL;
	omfInt32 		blockSize, srcFields;
	omfCStrmSwab_t	src, dest;
	char			*srcPtr, *basePtr, *tmpPtr = NULL;
	
	omfAssertValidFHdl(main);
	omfAssertMediaHdl(media);
	
	XPROTECT(main)
	{
		XASSERT(pdata, OM_ERR_NULL_PARAM);
	
		XASSERT(pdata->bitsPerPixel != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->bytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		xfer = parmblk->spc.mediaXfer.xfer;
		XASSERT(xfer, OM_ERR_NULLBUF);
	
		xfer->bytesXfered = 0;
		xfer->samplesXfered = 0;
		
		XASSERT(media->numChannels == 1, OM_ERR_CODEC_CHANNELS);
		XASSERT(xfer->subTrackNum == 1, OM_ERR_CODEC_CHANNELS);
		
		fileBytes = xfer->numSamples * pdata->bytesPerSample;
		
		
		/* Here's where field alignment is handled on READ!  Since fields
		   are aligned seperately, (and we're assuming uncompressed in this
		   codec) if there are two fields, treat them as individual items. 
		   After reading out each item, check the stream position, and skip
		   past the block if necessary.  Since alignment is expensive even 
		   if not needed, try to sift out situations where alignment is not 
		   needed.
		 */
		 
		if (pdata->alignment > 1)  /* only bother precomputing if necessary */
		{
			blockSize = pdata->bytesPerSample;  /* size per frame */
			if ((pdata->fileLayout == kSeparateFields) ||
			    (pdata->fileLayout == kMixedFields))
				blockSize /=2;   /* A block is half-size - equal for each field */
		}
		
		/* align if blockSize not already an even multiple of alignment factor */		
		if ((pdata->alignment > 1) && (blockSize % pdata->alignment != 0))
		{
			omfInt32	i, bufIndex, field;
			omfInt32	remainderBytes;
			omfUInt32	bytesRead;
			omfInt32	pos32;
			omfInt64 	position;
			
			if ((pdata->fileLayout == kSeparateFields) ||
			    (pdata->fileLayout == kMixedFields))
			{
				srcFields = 2;
			}
			else
				srcFields = 1;
			
			CHECK(omcComputeBufferSize(media->stream,
				blockSize, omcComputeMemSize, &memBytes));
			
			/* !!! Check for destFields later */
			if((memBytes * xfer->numSamples * srcFields) > xfer->buflen)
				RAISE(OM_ERR_SMALLBUF);
				
			if((blockSize * srcFields * xfer->numSamples) > xfer->buflen)
			{
				tmpPtr = (char *)omOptMalloc(main, blockSize * srcFields);
				if(tmpPtr == NULL)
					RAISE(OM_ERR_NOMEMORY);
			}
				
			for (i = 0, bufIndex = 0; i < (omfInt32)xfer->numSamples; i++, bufIndex += blockSize * srcFields)
			{
				if(tmpPtr != NULL)
					srcPtr = tmpPtr;
				else
					srcPtr = (char *)xfer->buffer + bufIndex;
				basePtr = srcPtr;
						
				for(field = 0; field < srcFields; field++)
				{
					CHECK(omcReadStream(media->stream, blockSize,
										srcPtr, 
										&bytesRead));
					xfer->bytesXfered += bytesRead;
																			 									 
					CHECK(omcGetStreamPosition(media->stream, &position));
					if(pdata->alignment == 4096)
					  {
						omfsDecomposeInt64(position, NULL, &pos32);
						remainderBytes = ((omfUInt32)pos32 % pdata->alignment);
					  }
					else
					  {
						omfsDivideInt64byInt32(position, pdata->alignment, NULL, &remainderBytes);
					  }
					if (remainderBytes != 0)
					{
						omfInt32 padAmount = pdata->alignment - remainderBytes;
						CHECK(omcSeekStreamRelative(media->stream, padAmount));
					}
					srcPtr += blockSize;
				}
				
				src.fmt = media->stream->fileFormat;
				src.buf = basePtr;
				src.buflen = blockSize * srcFields;
				dest.fmt = media->stream->memFormat;
				dest.buf = (char *)xfer->buffer + bufIndex;
				dest.buflen = memBytes * srcFields;
				CHECK(omcSwabData(media->stream, &src, &dest, media->stream->swapBytes));
			}
			
			if(tmpPtr != NULL)
			{
				omOptFree(main, tmpPtr);
			}
		}
		else
		{
			CHECK(omfmSetSwabBufSize(media->stream, pdata->bytesPerSample));
			fileBytes = xfer->numSamples * pdata->bytesPerSample;
			CHECK(omcComputeBufferSize(media->stream,
				fileBytes, omcComputeMemSize, &memBytes));
			if(memBytes > xfer->buflen)
				RAISE(OM_ERR_SMALLBUF);
	
			CHECK(omcReadSwabbedStream(media->stream, fileBytes,
				memBytes, xfer->buffer, &bytesRead));
			xfer->bytesXfered = bytesRead;
		}
		xfer->samplesXfered = xfer->bytesXfered / pdata->bytesPerSample;
	}
	XEXCEPT
	{
		if((XCODE() == OM_ERR_EOF) || (XCODE() == OM_ERR_END_OF_DATA))
		{
			xfer->bytesXfered += bytesRead;
			xfer->samplesXfered = xfer->bytesXfered / pdata->bytesPerSample;
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
static omfErr_t myCodecWriteLines(	omfCodecParms_t * parmblk,
				            		omfMediaHdl_t media,
				            		omfHdl_t main)
{
	omfInt32           nBytes, numFields, numLinesToNextField, numLinesToWrite;
	omfInt32           bIndex = 0;
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;

	omfAssertValidFHdl(main);
	omfAssertMediaHdl(media);
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
		XASSERT(pdata->bitsPerPixel != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->bytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

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
			CHECK(omfsFileSetValueAlignment(media->dataFile, pdata->alignment));
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
		numLinesToWrite = parmblk->spc.mediaLinesXfer.numLines;
		
		/* If field alignment is specified, see if the number of lines crosses the 
		   field boundary, if so, write whole fields, padding afterward, until the
		   number of lines won't cross a field boundary */
				
		bIndex = 0;

		while ((pdata->alignment > 1) && (numLinesToWrite  > numLinesToNextField) )
		{
			omfUInt32 position32;
			omfUInt32 remainderBytes;
			
			nBytes = pdata->imageWidth * (pdata->bitsPerPixel / 8) *
					 	numLinesToNextField;
			CHECK(omcWriteStream(media->stream, nBytes, 
								 (void *)&(((char *)parmblk->spc.mediaLinesXfer.buf)[bIndex])));
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
			nBytes = pdata->imageWidth * (pdata->bitsPerPixel / 8) * 
			          numLinesToWrite;
		
			CHECK(omcWriteStream(media->stream, nBytes, 
								 (void *)&(((char *)parmblk->spc.mediaLinesXfer.buf)[bIndex])));
			bIndex += nBytes;
		}
		
		pdata->dataWritten = TRUE;
		
		pdata->numLines += parmblk->spc.mediaLinesXfer.numLines;
		while (pdata->numLines >= (pdata->imageLength * numFields))
		{
			omfsAddInt32toInt64(1, &media->channels[0].numSamples);
			pdata->numLines -= (pdata->imageLength * numFields);
		}
		
		parmblk->spc.mediaLinesXfer.bytesXfered = bIndex;
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
static omfErr_t myCodecReadLines(omfCodecParms_t * parmblk,
				                   omfMediaHdl_t media,
				                   omfHdl_t main)
{
	userDataRGBA_t 	*pdata = (userDataRGBA_t *) media->userData;
	omfInt32           nBytes, numLinesToNextField, numLinesToRead;
	omfInt32           bIndex = 0;

	omfAssertValidFHdl(main);
	omfAssertMediaHdl(media);
	omfAssert(pdata, main, OM_ERR_NULL_PARAM);

	XPROTECT(main)
	{
		XASSERT(pdata->bitsPerPixel != 0, OM_ERR_ZERO_PIXELSIZE);
		XASSERT(pdata->bytesPerSample != 0, OM_ERR_ZERO_SAMPLESIZE);

		nBytes = pdata->imageWidth * (pdata->bitsPerPixel / 8) *
					 	parmblk->spc.mediaLinesXfer.numLines;
	
		XASSERT(nBytes <= parmblk->spc.mediaLinesXfer.bufLen, OM_ERR_SMALLBUF);
		
		/* use pdata->numLines % imageLength to work correctly in second field */
		numLinesToNextField = pdata->imageLength - (pdata->numLines % pdata->imageLength);
		numLinesToRead = parmblk->spc.mediaLinesXfer.numLines;
		
		/* If field alignment is specified, see if the number of lines crosses the 
		   field boundary, if so, write whole fields, padding afterward, until the
		   number of lines won't cross a field boundary */
				
		bIndex = 0;


		while ((pdata->alignment > 1) && (numLinesToRead  > numLinesToNextField) )
		{
			omfUInt32 position32;
			omfUInt32 remainderBytes;
			
			nBytes = pdata->imageWidth * (pdata->bitsPerPixel / 8) *
					 	numLinesToNextField;
			if (omcReadStream(media->stream, nBytes, 
						  (void *)&(((char *)parmblk->spc.mediaLinesXfer.buf)[bIndex]), NULL) != OM_ERR_NONE)
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
			nBytes = pdata->imageWidth * (pdata->bitsPerPixel / 8) *
					 	numLinesToRead;
			if (omcReadStream(media->stream, nBytes, 
						  (void *)&(((char *)parmblk->spc.mediaLinesXfer.buf)[bIndex]), NULL) != OM_ERR_NONE)
				RAISE(OM_ERR_NODATA);
				
			bIndex += nBytes;
		}

		pdata->numLines += parmblk->spc.mediaLinesXfer.numLines;

		/* leave pdata->numLines MOD the number of lines in a field */
		pdata->numLines = pdata->numLines % pdata->imageLength;
	
		parmblk->spc.mediaLinesXfer.bytesXfered = bIndex;
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
static omfErr_t postWriteOps(omfCodecParms_t * parmblk, omfMediaHdl_t media)
{
/*	omfInt32           padbytes = 0; */

	userDataRGBA_t *pdata;
	omfHdl_t        file = NULL;
	omcRGBAPersistent_t *pers;

	/* this routine will be called after sample data is written */

	omfAssertMediaHdl(media);
	file = media->mainFile;
	omfAssertValidFHdl(file);
	
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);
 	pers = (omcRGBAPersistent_t *)parmblk->pers;
 
	pdata = (userDataRGBA_t *) media->userData;
	omfAssert(pdata, file, OM_ERR_NULL_PARAM);

	XPROTECT(file)
	{
	 	/* fix the clip and track group lengths,they weren't known until now */
	
		XASSERT(media->fileMob != NULL, OM_ERR_INTERNAL_CORRUPTVINFO);
		XASSERT(media->mdes != NULL, OM_ERR_INTERNAL_CORRUPTVINFO);
			
	 	/* patch descriptor length */
	 	CHECK(omfsWriteLength(file, media->mdes, OMMDFLLength,
	 								media->channels[0].numSamples));
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
static omfErr_t writeDescriptorData(omfCodecParms_t * parmblk, 
									omfMediaHdl_t media, 
									omfHdl_t file, 
									userDataRGBA_t * pdata)
{
	omcRGBAPersistent_t *pers;

	/* this routine will be called after sample data is written */

	omfAssertMediaHdl(media);
	file = media->mainFile;
	omfAssertValidFHdl(file);
	
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);
 	pers = (omcRGBAPersistent_t *)parmblk->pers;
 
	pdata = (userDataRGBA_t *) media->userData;
	omfAssert(pdata, file, OM_ERR_NULL_PARAM);

	XPROTECT(file)
	{
		/* OMFI:DIDD:StoredHeight and OMFI:DIDD:StoredWidth */	
		CHECK(omfsWriteInt32(file, media->mdes, OMDIDDStoredHeight, 
							pdata->imageLength));
		CHECK(omfsWriteInt32(file, media->mdes, OMDIDDStoredWidth, 
							pdata->imageWidth));
		
		/* display and sampled coordinates  */
		if (pdata->displayLength && pdata->displayWidth)
		{
			CHECK(omfsWriteInt32(file, media->mdes, OMDIDDDisplayYOffset, 
								pdata->displayYOffset));
			CHECK(omfsWriteInt32(file, media->mdes, OMDIDDDisplayXOffset, 
								pdata->displayXOffset));
			CHECK(omfsWriteInt32(file, media->mdes, OMDIDDDisplayHeight, 
								pdata->displayLength));
			CHECK(omfsWriteInt32(file, media->mdes, OMDIDDDisplayWidth, 
								pdata->displayWidth));
		}
		if (pdata->sampledLength && pdata->sampledWidth)
		{
			CHECK(omfsWriteInt32(file, media->mdes, OMDIDDSampledYOffset, 
								pdata->sampledYOffset));
			CHECK(omfsWriteInt32(file, media->mdes, OMDIDDSampledXOffset, 
								pdata->sampledXOffset));
			CHECK(omfsWriteInt32(file, media->mdes, OMDIDDSampledHeight, 
								pdata->sampledLength));
			CHECK(omfsWriteInt32(file, media->mdes, OMDIDDSampledWidth, 
								pdata->sampledWidth));
		}
		/* OMFI:DIDD:FrameLayout */
		CHECK(omfsWriteLayoutType(file, media->mdes, OMDIDDFrameLayout,
								 pdata->fileLayout)); 
			
		/* OMFI:DIDD:AlphaTransparency DEFER IMPLEMENTATION TO A LATER TIME!!*/
		
		/* OMFI:DIDD:VideoLineMap */
		if (pdata->fileLayout != kFullFrame &&
			(pdata->videoLineMap[0] != 0 || pdata->videoLineMap[1] != 0))
		{
		
			CHECK(OMWriteBaseProp(file, media->mdes, OMDIDDVideoLineMap,
							   OMInt32Array, sizeof(pdata->videoLineMap),
							   (void *)pdata->videoLineMap));
		}
		
		/* OMFI:DIDD:ImageAspectRatio */
		
		CHECK(omfsWriteRational(file, media->mdes, OMDIDDImageAspectRatio,
				 						 pdata->imageAspectRatio));
		
		CHECK(omfsWriteInt32(file, media->mdes, OMDIDDFieldAlignment, 
							pdata->alignment));
		/* OMFI:RGBA:PixelLayout */
		
		{
			char pixComps[MAX_NUM_RGBA_COMPS];
			int  compCount = 0, p = 0;
			
			memset(pixComps, 0, sizeof(pixComps));
			while (pdata->filePixComps[compCount])
				pixComps[p++] = pdata->filePixComps[compCount++];
			pixComps[p] = 0;
			
			CHECK(OMWriteBaseProp(file, media->mdes, pers->OMRGBAPixelLayout,
							   OMCompCodeArray, MAX_NUM_RGBA_COMPS,
							    (void *)pixComps));
		}
		
		/* OMFI:RGBA:PixelStructure */
		
		{
			unsigned char pixSizes[MAX_NUM_RGBA_COMPS];
			int  compCount = 0, p = 0;
			
			memset(pixSizes, 0, sizeof(pixSizes));
			while (pdata->filePixSizes[compCount])
				pixSizes[p++] = pdata->filePixSizes[compCount++];
			pixSizes[p] = 0;
						
			CHECK(OMWriteBaseProp(file, media->mdes,pers->OMRGBAPixelStructure,
						   OMCompSizeArray, MAX_NUM_RGBA_COMPS,
						    (void *)pixSizes));
		}
		
		
		/* OMFI:RGBA:Palette, OMFI:RGBA:PaletteLayout & OMFI:RGBA:PaletteStructure
		   not yet needed... */
		
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
static omfErr_t loadDescriptorData(omfCodecParms_t * parmblk, 
								   omfMediaHdl_t media, 
								   omfHdl_t file, 
								   userDataRGBA_t * pdata)
{
	omfInt32 				sampleFrames = 0;
	omfUInt32 				layout = 0;
	omfUInt32 				bytesPerPixel = 0;
	omfUInt32 				numFields = 0;
	omfUInt32 				i = 0;
	omcRGBAPersistent_t *pers = NULL;
	omfPosition_t			zeroPos;
	omfErr_t                omfError = OM_ERR_NONE;
	
	omfsCvtInt32toPosition(0, zeroPos);
	omfAssertValidFHdl(file);
	omfAssertMediaHdl(media);
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);
	omfAssert(pdata, file, OM_ERR_NULL_PARAM);

	XPROTECT(file)
	{
		pers = (omcRGBAPersistent_t *)parmblk->pers;
	 
		if (parmblk->fileRev == kOmfRev2x)
		{
	 		CHECK(omfsReadRational(file, media->mdes, OMMDFLSampleRate, 
								   &(pdata->rate)));
	 	}
	 	else
	 	{
	 		CHECK(omfsReadExactEditRate(file, media->mdes, OMMDFLSampleRate, 
										&(pdata->rate)));
		} 		
		CHECK(omfsReadLength(file, media->mdes, OMMDFLLength, 
							 &media->channels[0].numSamples));
	
		media->numChannels = 1;
	
		CHECK(omcOpenStream(media->mainFile, media->dataFile, media->stream, 
						media->dataObj, OMIDATImageData, OMDataValue));
	
		pdata->numLines = 0;
		
		CHECK(omfsReadInt32(file, media->mdes, OMDIDDStoredWidth, 
						   &pdata->imageWidth));
		CHECK(omfsReadInt32(file, media->mdes, OMDIDDStoredHeight, 
						   &pdata->imageLength));
		
		/* sampled dimensions default to stored dimensions.  If any calls fail
		 * later, default will refile untouched. 
		 */
	
		pdata->sampledWidth = pdata->imageWidth;
		pdata->sampledLength = pdata->imageLength;
		pdata->sampledXOffset = 0;
		pdata->sampledYOffset = 0;
		
		if (omfsReadInt32(file, media->mdes, OMDIDDSampledWidth, 
						 &pdata->sampledWidth) == OM_ERR_NONE &&
			omfsReadInt32(file, media->mdes, OMDIDDSampledHeight, 
						 &pdata->sampledLength) == OM_ERR_NONE)
		{
			/* ignore errors here, default to zero in both cases */		
			(void)omfsReadInt32(file, media->mdes, OMDIDDSampledXOffset, 
							   &pdata->sampledXOffset);
			(void)omfsReadInt32(file, media->mdes, OMDIDDSampledYOffset, 
							   &pdata->sampledYOffset);
		}
		
		pdata->displayWidth = pdata->imageWidth;
		pdata->displayLength = pdata->imageLength;
		pdata->displayXOffset = 0;
		pdata->displayYOffset = 0;
		
		if (omfsReadInt32(file, media->mdes, OMDIDDDisplayWidth, 
						 &pdata->displayWidth) == OM_ERR_NONE &&
			omfsReadInt32(file, media->mdes, OMDIDDDisplayHeight, 
						 &pdata->displayLength) == OM_ERR_NONE)
		{
			/* ignore errors here, default to zero in both cases */		
			(void)omfsReadInt32(file, media->mdes, OMDIDDDisplayXOffset, 
							   &pdata->displayXOffset);
			(void)omfsReadInt32(file, media->mdes, OMDIDDDisplayYOffset, 
							   &pdata->displayYOffset);
		}
			
		CHECK(omfsReadRational(file, media->mdes, OMDIDDImageAspectRatio, 
							   &pdata->imageAspectRatio));
							   
		omfError = omfsReadInt32(file, media->mdes, OMDIDDFieldAlignment,
								&pdata->alignment);
		
		CHECK(omfsReadLayoutType(file, media->mdes, OMDIDDFrameLayout, &pdata->fileLayout));
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
			
			omfError = OMReadProp(file, media->mdes, OMDIDDVideoLineMap,
								  zeroPos, kSwabIfNeeded, OMInt32Array,
								  sizeof(omfInt32), (void *)&pdata->videoLineMap[0]);
			if (omfError != OM_ERR_NONE)
				pdata->videoLineMap[0] = 0;
			else
				mapFlags |= 1;
				
			if (pdata->fileLayout == kSeparateFields || pdata->fileLayout == kMixedFields)
			{
				omfsCvtInt32toPosition(sizeof(omfInt32), fourPos);
			
				omfError = OMReadProp(file, media->mdes, OMDIDDVideoLineMap,
									  fourPos, kSwabIfNeeded, OMInt32Array,
									  sizeof(omfInt32), (void *)&pdata->videoLineMap[1]);
				if (omfError != OM_ERR_NONE)
					pdata->videoLineMap[1] = 0;
				else
					mapFlags |= 2;
			}
		}
			
		
		/* OMFI:RGBA:PixelLayout */
		
		for (i = 0; i < MAX_NUM_RGBA_COMPS; i++)
			pdata->filePixComps[i] = 0;
		omfError = OMReadProp(file, media->mdes, pers->OMRGBAPixelLayout,
							  zeroPos, 
							  kNeverSwab, OMCompCodeArray, 
							  MAX_NUM_RGBA_COMPS,
							  (void *)pdata->filePixComps); 

		/* allow end of data because compcodearray is variable length */
		if (omfError != OM_ERR_NONE && omfError != OM_ERR_END_OF_DATA)
		  RAISE(omfError);

		/* check layout for legal characters */
		i = 0;
		while (i < MAX_NUM_RGBA_COMPS && pdata->filePixComps[i])
		{
		  switch (pdata->filePixComps[i])
		  {
		  case 'A':
		  case 'B':
		  case 'G':
		  case 'R':
		  case 'P':
		  case 'F':
			break;
		  default:
			RAISE(OM_ERR_BADEXPORTPIXFORM);
		  }
		  i++;
		}

		/* OMFI:RGBA:PixelStructure */
		for (i = 0; i < MAX_NUM_RGBA_COMPS; i++)
			pdata->filePixSizes[i] = 0;
		omfError = OMReadProp(file, media->mdes, pers->OMRGBAPixelStructure,
							  zeroPos, 
							  kNeverSwab, OMCompSizeArray,  
							  MAX_NUM_RGBA_COMPS, (void *)pdata->filePixSizes);

		/* allow end of data because compsizearray is variable length */
		if (omfError != OM_ERR_NONE && omfError != OM_ERR_END_OF_DATA) 
		  RAISE(omfError);
								
		pdata->bytesPerSample = 0;
		pdata->bitsPerPixel = 0;
		{
		    i = 0;
			while (i < MAX_NUM_RGBA_COMPS && pdata->filePixSizes[i])
				pdata->bitsPerPixel += pdata->filePixSizes[i++];
		}
		
		/* make sure the size is an even multiple of 8 for now */
		if ((pdata->bitsPerPixel == 0) || (pdata->bitsPerPixel % 8))
			RAISE(OM_ERR_24BITVIDEO);
		
		pdata->bytesPerSample = (pdata->bitsPerPixel/8) * pdata->imageWidth * 
		  pdata->imageLength * numFields;
		  
		pdata->fileFmt[0].opcode = kOmfPixelFormat;
		pdata->fileFmt[0].operand.expPixelFormat = kOmfPixRGBA;
		
		pdata->fileFmt[1].opcode = kOmfFrameLayout;
		pdata->fileFmt[1].operand.expFrameLayout = pdata->fileLayout;
		
		pdata->fileFmt[2].opcode = kOmfPixelSize;
		pdata->fileFmt[2].operand.expInt16 = pdata->bitsPerPixel;
		
		pdata->fileFmt[3].opcode = kOmfStoredRect;
		pdata->fileFmt[3].operand.expRect.xOffset = 0;
		pdata->fileFmt[3].operand.expRect.xSize = pdata->imageWidth;
		pdata->fileFmt[3].operand.expRect.yOffset = 0;
		pdata->fileFmt[3].operand.expRect.ySize = pdata->imageLength;
		
		pdata->fileFmt[4].opcode = kOmfVideoLineMap;
		pdata->fileFmt[4].operand.expLineMap[0] = pdata->videoLineMap[0];
		pdata->fileFmt[4].operand.expLineMap[1] = pdata->videoLineMap[1];

		pdata->fileFmt[5].opcode = kOmfRGBCompLayout;
		memcpy(pdata->fileFmt[5].operand.expCompArray, pdata->filePixComps,
				sizeof(pdata->fileFmt[5].operand.expCompArray));
		
		pdata->fileFmt[6].opcode = kOmfRGBCompSizes;
		memcpy(pdata->fileFmt[6].operand.expCompSizeArray, pdata->filePixSizes,
				sizeof(pdata->fileFmt[6].operand.expCompSizeArray));

		pdata->fileFmt[7].opcode = kOmfVFmtEnd;
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
static omfErr_t setupStream(omfMediaHdl_t media, userDataRGBA_t * pdata)
{
	omfCStrmFmt_t   memFmt, fileFmt;
	
	XPROTECT(media->mainFile)
	{
		memFmt.mediaKind = media->pictureKind;
		memFmt.vfmt = pdata->memFmt;
		memFmt.afmt = NULL;
		CHECK(omcSetMemoryFormat(media->stream, memFmt));
	
		fileFmt.mediaKind = media->pictureKind;
		fileFmt.vfmt = pdata->fileFmt;
		fileFmt.afmt = NULL;
		
		CHECK(omcSetFileFormat(media->stream, fileFmt, 
							   media->stream->swapBytes));
		CHECK(omfmSetSwabProc(media->stream, myCodecSwabProc, stdCodecSwabLenProc,
								stdCodecNeedsSwabProc));		
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

#if OMFI_CODEC_DIAGNOSTICS
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
static omfErr_t myCodecDiags(omfCodecParms_t * parmblk, 
							 omfMediaHdl_t media, 
							 omfHdl_t file)
{
	FILE *logFile;
	omcRGBAPersistent_t *pers = (omcRGBAPersistent_t *)parmblk->pers;
	omfPosition_t			zeroPos;
	char					lenBuf[32];
	
	omfsCvtInt32toPosition(0, zeroPos);

	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);
	omfAssert(media, file, OM_ERR_BAD_MDHDL);
	
	logFile = (FILE *)parmblk->spc.diag.logFile;
	omfAssert(logFile, file, OM_ERR_NULL_PARAM);
	
	fprintf(logFile,"\n\n*************** BEGIN RGBA DIAGNOSTICS ******************\n\n");
	
	fprintf(logFile, "In-memory data:\n\n");
	
	if (pdata == NULL)
	{
		fprintf(logFile, "WARNING: User data pointer is NULL\n");
	}
	else
	{
		if (pdata->imageWidth)
			fprintf(logFile, "\tStored Width: %1ld\n", pdata->imageWidth);
			
		if (pdata->imageLength)
			fprintf(logFile, "\tStored Height: %1ld\n", pdata->imageLength);
			
		if (pdata->sampledWidth)
			fprintf(logFile, "\tSampled Width: %1ld\n", pdata->sampledWidth);
			
		if (pdata->sampledLength)
			fprintf(logFile, "\tSampled Height: %1ld\n", pdata->sampledLength);
			
		if (pdata->sampledXOffset)
			fprintf(logFile, "\tSampled X Offset: %1ld\n", 
					pdata->sampledXOffset);
			
		if (pdata->sampledYOffset)
			fprintf(logFile, "\tSampled Y Offset: %1ld\n", 
					pdata->sampledYOffset);
			
		if (pdata->displayWidth)
			fprintf(logFile, "\tDisplay Width: %1ld\n", pdata->displayWidth);
			
		if (pdata->displayLength)
			fprintf(logFile, "\tDisplay Height: %1ld\n", pdata->displayLength);
			
		if (pdata->displayXOffset)
			fprintf(logFile, "\tDisplay X Offset: %1ld\n", 
					pdata->displayXOffset);
			
		if (pdata->displayYOffset)
			fprintf(logFile, "\tDisplay Y Offset: %1ld\n", 
					pdata->displayYOffset);
						
		fprintf(logFile, "\tSample Rate: %1ld/%1ld\n", 
				pdata->rate.numerator, pdata->rate.denominator);
		
		switch (pdata->fileLayout)
			{
	
				case kFullFrame:
					fprintf(logFile, "\tFile frame layout: kFullFrame\n");
					break;
		
				case kOneField:
					fprintf(logFile, "\tFile frame layout: kOneField\n");
					break;
		
				case kSeparateFields:
					fprintf(logFile, "\tFile frame layout: kSeparateFields\n");
					break;
		
				case kMixedFields:
					fprintf(logFile, "\tFile frame layout: kMixedFields\n");
					break;
		
				default:
					fprintf(logFile, "\tUnrecognized file frame layout: %1ld\n", 
							(omfInt32)pdata->fileLayout);
					break;
		
			} /* end switch */
			
		fprintf(logFile, "\tFile pixel format: ");
		switch (pdata->filePixelFormat)
			{
	
				case kOmfPixRGBA:
					fprintf(logFile, "kOmfPixRGBA\n");
					break;
		
				case kOmfPixYUV:
					fprintf(logFile, "kOmfPixYUV\n");
					break;
		
				default:
					fprintf(logFile, "?Unrecognized?: %1ld\n", 
							(omfInt32)pdata->filePixelFormat);
					break;
		
			} /* end switch */
			
		fprintf(logFile, "\tAspect Ratio: %1ld/%1ld\n", 
				pdata->imageAspectRatio.numerator, 
				pdata->imageAspectRatio.denominator);
	
	}
	
	if (media && (media->mdes == NULL))
	{
		fprintf(logFile, "\nDescriptor pointer is NULL, no in-file data:\n\n");
	}
	else
	{
		omfInt32 lvar;
		omfInt32 layout;
		omfRational_t ratvar;
		
		fprintf(logFile, "\nIn-File data:\n\n");
		
		if ((omfsReadInt32(file, media->mdes, OMDIDDStoredWidth, &lvar) 
			 == OM_ERR_NONE) && lvar != 0)
			fprintf(logFile, "\tStored Width: %1ld\n", lvar);
			
		if ((omfsReadInt32(file, media->mdes, OMDIDDStoredHeight, &lvar) 
			 == OM_ERR_NONE) && lvar != 0)
			fprintf(logFile, "\tStored Height: %1ld\n", lvar);
			
		if ((omfsReadInt32(file, media->mdes, OMDIDDSampledWidth, &lvar) 
			 == OM_ERR_NONE) && lvar != 0)
		{
			fprintf(logFile, "\tSampled Width: %1ld\n", lvar);
			
			if (omfsReadInt32(file, media->mdes, OMDIDDSampledHeight, &lvar) 
				== OM_ERR_NONE)
				fprintf(logFile, "\tSampled Height: %1ld\n", lvar);
			
			if (omfsReadInt32(file, media->mdes, OMDIDDSampledXOffset, &lvar) 
				== OM_ERR_NONE)
				fprintf(logFile, "\tSampled X Offset: %1ld\n", lvar);
			
			if (omfsReadInt32(file, media->mdes, OMDIDDSampledYOffset, &lvar) 
				== OM_ERR_NONE)
				fprintf(logFile, "\tSampled Y Offset: %1ld\n", lvar);
		}
			
		if ((omfsReadUInt32(file, media->mdes, OMDIDDDisplayWidth, &lvar) 
			 == OM_ERR_NONE) && lvar != 0)
		{
			fprintf(logFile, "\tDisplay Width: %1ld\n", lvar);
				
			if (omfsReadInt32(file, media->mdes, OMDIDDDisplayHeight, &lvar) 
				== OM_ERR_NONE)
				fprintf(logFile, "\tDisplay Height: %1ld\n", lvar);
				
			if (omfsReadInt32(file, media->mdes, OMDIDDDisplayXOffset, &lvar) 
				== OM_ERR_NONE)
				fprintf(logFile, "\tDisplay X Offset: %1ld\n", lvar);
				
			if (omfsReadInt32(file, media->mdes, OMDIDDDisplayYOffset, &lvar) 
				== OM_ERR_NONE)
				fprintf(logFile, "\tDisplay Y Offset: %1ld\n", lvar);
		}
		
		if (omfsReadRational(file, media->mdes, OMDIDDImageAspectRatio, 
							 &ratvar) == OM_ERR_NONE)
			fprintf(logFile, "\tAspect Ratio: %1ld/%1ld\n", ratvar.numerator, 
					ratvar.denominator);
		
		if (OMReadBaseProp(file, media->mdes, OMDIDDFrameLayout,
						   OMLayoutType, sizeof(omfInt32), (void *)&layout) 
			== OM_ERR_NONE)
		{
			fprintf(logFile, "\tDescriptor frame layout: ");
		
			switch (layout)
			{
		
				case OMFI_DIDD_LAYOUT_FULL:
					fprintf(logFile, "kFullFrame\n");
					break;
			
				case OMFI_DIDD_LAYOUT_SINGLE:
					fprintf(logFile, "kOneField\n");
					break;
			
				case OMFI_DIDD_LAYOUT_SEPARATE:
					fprintf(logFile, "kSeparateFields\n");
					break;
			
				case OMFI_DIDD_LAYOUT_MIXED:
					fprintf(logFile, "kMixedFields\n");
					break;
				
				default:
					fprintf(logFile, "?Unrecognized?: %1ld\n", 
							(omfInt32)pdata->fileLayout);
					break;
			
			} /* end switch */
		}
							
			
		/* OMFI:RGBA:PixelLayout */
		
		{
			char pixComps[MAX_NUM_RGBA_COMPS];
			register omfInt32 i = 0;			
			
			if( OMReadProp(file, media->mdes, pers->OMRGBAPixelLayout, zeroPos, 
					   kNeverSwab, OMCompCodeArray, 
					   MAX_NUM_RGBA_COMPS, (void *)pixComps) == OM_ERR_NONE)
			{
				fprintf(logFile, "\tPixel Layout: ");
				while (i < MAX_NUM_RGBA_COMPS && pixComps[i])
					fprintf(logFile, "%c ", pixComps[i++]);
				
				fprintf(logFile, "\n");
			}
		}
		
		/* OMFI:RGBA:PixelStructure */
		
		{
			unsigned char pixSizes[MAX_NUM_RGBA_COMPS];
			register omfInt32 i = 0;
			
			if (OMReadProp(file, media->mdes, pers->OMRGBAPixelStructure, zeroPos, 
					   kNeverSwab, OMCompSizeArray,  
					   MAX_NUM_RGBA_COMPS, (void *)pixSizes) == OM_ERR_NONE)
			{
				fprintf(logFile, "\tPixel Layout: ");
				/* Metrowerks has trouble printing char */
				while (i < MAX_NUM_RGBA_COMPS && pixSizes[i])  
					fprintf(logFile, "%1ld ", (omfInt32)pixSizes[i++]);
				
				fprintf(logFile, "\n");
			}
		}
		
		
		if (media->dataObj)
		{
			omfInt32 len;
			omfInt64 imageDataLength = omfsLengthDataValue(file, media->dataObj, 
														OMIDATImageData);
			omfsInt64ToString(imageDataLength, 10, sizeof(lenBuf), lenBuf);
			fprintf(logFile, "\nMedia Data:\n\n");
			fprintf(logFile, "\tImage Data length: %s\n", lenBuf);
			
		}
					
	}
	
		
	fprintf(logFile,  "\n***************  END RGBA DIAGNOSTICS  ******************\n");	
	
	return (OM_ERR_NONE);
}
#endif

static omfErr_t myCodecSwabProc( omfCodecStream_t *stream,
				                 omfCStrmSwab_t * src,
				                 omfCStrmSwab_t * dest,
				                 omfBool swapBytes)
{
	return(stdCodecSwabProc(stream, src, dest, swapBytes));
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
static omfErr_t InitMDESProps(omfCodecParms_t * parmblk, 
								   omfMediaHdl_t media, 
								   omfHdl_t file)
{
	omfRational_t	aspect;
	char 			pixComps[MAX_NUM_RGBA_COMPS];
	omfUInt8        pixStruct[MAX_NUM_RGBA_COMPS];
	char			lineMap[2] = { 0, 0 };
	omfObject_t		obj = parmblk->spc.initMDES.mdes;
	omcRGBAPersistent_t *pers = (omcRGBAPersistent_t *)parmblk->pers;
	
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
		
		pixComps[0] = 'R';
		pixComps[1] = 'G';
		pixComps[2] = 'B';
		pixComps[3] = 0;
		
		CHECK(OMWriteBaseProp(file, obj, pers->OMRGBAPixelLayout,
						   OMCompCodeArray, 4, (void *)pixComps));

		pixStruct[0] = 8;
		pixStruct[1] = 8;
		pixStruct[2] = 8;
		pixStruct[3] = 0;

		CHECK(OMWriteBaseProp(file, obj, pers->OMRGBAPixelStructure,
							  OMCompSizeArray, 4, (void *)pixStruct));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}


/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
