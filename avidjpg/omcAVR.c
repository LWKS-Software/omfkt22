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

#include "omErr.h"
#include "omTypes.h"
#include "omMedia.h"
#include "omCodId.h"
#include "omcTIFF.h" 
#include "omcCDCI.h" 
#include "omJPEG.h"
#include "omcAVR.h"  
#include "omMobBld.h"
#include "omCvt.h"
#include "omPvt.h" 
#include "omcAvidJFIF.h" 

/* Moved math.h down here to make NEXT's compiler happy */
#include <math.h>

#include "avrPvt.h"

#include "avrs.h" /* get the automatically-generated avr headers */

#define OMFS_FREE_AND_NULL_IF_SET(x) if (x) { omfsFree(x); x=NULL;}
#define CLIP(minVal,maxVal,val) if ((val)<(minVal)) 	\
									val=(minVal); 		\
								else if ((val)>(maxVal))\
									val=(maxVal);		
									
void ScaleQTables(omfUInt8* qTable, double scaleFactor);
void ScaleQTables16(omfUInt16* qTable, double scaleFactor);

#define MAX_FMT_OPS	32

static const double NTSC_rate = 29.97;
static const double FILM_rate = 24.0;
static const double PAL_rate = 25.0;
static const double EPSILON = 0.001;

typedef enum
{
	OriginalAVR, JFIF, Uncompressed422
} AVRType_t;

/* These need to go in an include file -rps */
#define OMFI_DIDD_LAYOUT_FULL 0
#define OMFI_DIDD_LAYOUT_SEPARATE 1
#define OMFI_DIDD_LAYOUT_SINGLE 2
#define OMFI_DIDD_LAYOUT_MIXED 3

/**************************************************************************

              PERSISTENT DATA STRUCTURE DEFINITION
              Holds dynamic Bento property and type handles across
                invocations of the codec (different media streams)
                Not important to codec users, only to codec functions

**************************************************************************/

typedef struct
	{
		omfProperty_t	omCDCIComponentWidth;
		omfProperty_t	omCDCIHorizontalSubsampling;
		omfProperty_t	omCDCIColorSiting;
		omfProperty_t	omCDCIBlackReferenceLevel;
		omfProperty_t	omCDCIWhiteReferenceLevel;
	}				omcAJPGPersistent_t;
	
/**************************************************************************

              PER-MEDIA-STREAM DATA STRUCTURE DEFINITION
              Holds stream data across invocations of the codec 
                methods on the same media streams
                Not important to codec users, only to codec functions

**************************************************************************/

typedef struct
{
	AVRType_t			avrType;
	codecTable_t		clientCodec;
	void				*clientPData;
}   userDataAJPG_t;

typedef struct
{
	omfObject_t     obj;
	omfProperty_t   prop;
	omfInt64          offset;
}               streamRefData_t;

typedef enum {NTSC, PAL} signalType_t;

/**************************************************************************

                   LOCAL VARIABLE DECLARATIONS

**************************************************************************/

static omfSessionHdl_t thisSession = NULL;

/**************************************************************************

                   LOCAL FUNCTION PROTOTYPES

**************************************************************************/

static omfErr_t myCodecInit(omfCodecParms_t * parmblk);

static omfErr_t initUserData(userDataAJPG_t *pdata);

static omfErr_t myCodecSetPrivateData(omfCodecParms_t *parmblk, 
									  omfHdl_t file, 
									  omfMediaHdl_t media, 
									  userDataAJPG_t *pdata);
									  
static omfErr_t avrGetWidth(avrInfoStruct_t *avrData, omfUInt32 *result);

static omfErr_t avrGetHeight(omfVideoSignalType_t videoType, avrInfoStruct_t *avrData,
							 omfUInt32 *result);

static omfErr_t avrGetFrameLayout(	avrInfoStruct_t *avrData, 
									omfFrameLayout_t *result);

static omfErr_t avrGetAspectRatio(avrInfoStruct_t *avrData, 
									omfRational_t *result);

static omfBool	avrNeedsDisplayRect(omfVideoSignalType_t videoType,
									avrInfoStruct_t *avrData,
									omfRect_t *rect);

static omfErr_t avrGetInfoStruct(avidAVRdata_t avrData, 
								 avrInfoStruct_t **info);
static omfErr_t codecGetSelectInfoAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecOpenAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecCreateAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecCloseAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecVarietyCountAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecVarietyInfoAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecGetInfoAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecPutInfoAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecNumChannelsAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecSetFrameAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecReadSamplesAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecWriteSamplesAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecReadLinesAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecWriteLinesAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t codecInitMDESPropsAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);
static omfErr_t createPassThroughAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main);

/************************
 * omfCodecAJPG
 *
 * 		Handles main codec interface implementation.  Responds to all legal
 *        codec methods.
 *
 * Argument Notes:
 *		Subfunctions will be called based on formal arg 'func'.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
 
omfErr_t omfCodecAJPG(omfCodecFunctions_t func, 
					  omfCodecParms_t * parmblk)
{
	omfErr_t        status = OM_ERR_NONE, tmpErr = OM_ERR_NONE;
	omfMediaHdl_t   media = NULL;
	omfHdl_t        file = NULL;
	userDataAJPG_t *pdata = NULL;
	omfCodecOptDispPtr_t	*dispPtr;
	
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);


	
	switch (func)
	{
	
	case kCodecInit:
	  /* if codec-persistent data exists, then init has been called */
		if (parmblk->pers) 
			return(OM_ERR_NONE); /* let it slide, like guarded includes */
		else
			return(myCodecInit(parmblk));
			
    case kCodecGetMetaInfo:
	
		tmpErr = OM_ERR_NONE;

		strncpy((char *)parmblk->spc.metaInfo.name, "Avid AVR Codec", 
				parmblk->spc.metaInfo.nameLen);
		parmblk->spc.metaInfo.info.codecID = CODEC_AJPG_VIDEO;
		parmblk->spc.metaInfo.info.rev = CODEC_REV_3;
	   	parmblk->spc.metaInfo.info.mdesClassID = NULL;
		parmblk->spc.metaInfo.info.dataClassID = NULL;
		parmblk->spc.metaInfo.info.dataKindNameList = PICTUREKIND;
		parmblk->spc.metaInfo.info.minFileRev = kOmfRev1x;
		parmblk->spc.metaInfo.info.maxFileRevIsValid = TRUE;		/* There is a maximum rev */
	    parmblk->spc.metaInfo.info.maxFileRev = kOmfRev1x;
		if(parmblk->spc.metaInfo.variant != NULL)
		{
			if(parmblk->spc.metaInfo.variant[0] == 'A' || parmblk->spc.metaInfo.variant[0] == 'a')
			{
				tmpErr = omfCodecTIFF(func, parmblk);

				parmblk->spc.metaInfo.info.mdesClassID = "TIFD";
				parmblk->spc.metaInfo.info.dataClassID = "TIFF";
				parmblk->spc.metaInfo.info.dataKindNameList = PICTUREKIND;
				parmblk->spc.metaInfo.info.minFileRev = kOmfRev1x;
				parmblk->spc.metaInfo.info.maxFileRevIsValid = TRUE;		/* There is a maximum rev */
				parmblk->spc.metaInfo.info.maxFileRev = kOmfRev1x;
			}
			else if(parmblk->spc.metaInfo.variant[0] == 'U' || parmblk->spc.metaInfo.variant[0] == 'u')
			{
				tmpErr = omfCodecCDCI(func, parmblk);
			}
			else if(parmblk->spc.metaInfo.variant[0] == 'J' || parmblk->spc.metaInfo.variant[0] == 'j')
			{
				tmpErr = omfCodecAvidJFIF(func, parmblk);
			}
		}
		
		return (tmpErr);

	case kCodecLoadFuncPointers:
		dispPtr = parmblk->spc.loadFuncTbl.dispatchTbl;

		dispPtr[ kCodecOpen ] = codecOpenAVR;
		dispPtr[ kCodecCreate ] = codecCreateAVR;
		dispPtr[ kCodecGetInfo ] = codecGetInfoAVR;
		dispPtr[ kCodecPutInfo ] = codecPutInfoAVR;
		dispPtr[ kCodecReadSamples ] = codecReadSamplesAVR;
		dispPtr[ kCodecWriteSamples ] = codecWriteSamplesAVR;
		dispPtr[ kCodecReadLines ] = codecReadLinesAVR;
		dispPtr[ kCodecWriteLines ] = codecWriteLinesAVR;
		dispPtr[ kCodecClose ] = codecCloseAVR;
		dispPtr[ kCodecSetFrame ] = codecSetFrameAVR;
		dispPtr[ kCodecGetNumChannels ] = codecNumChannelsAVR;
		dispPtr[ kCodecGetSelectInfo ] = codecGetSelectInfoAVR;
		dispPtr[ kCodecGetVarietyCount ] = codecVarietyCountAVR;
		dispPtr[ kCodecGetVarietyInfo ] = codecVarietyInfoAVR;
		dispPtr[ kCodecInitMDESProps ] = codecInitMDESPropsAVR;
/*		dispPtr[ kCodecImportRaw ] = NULL;
		dispPtr[ kCodecSemanticCheck ] = NULL;
		dispPtr[ kCodecAddFrameIndexEntry ] = NULL;
*/		return(OM_ERR_NONE);
		
	default:
		status = OM_ERR_INVALID_OP_CODEC;
		break;
	}
	
	return (status);
}

static omfErr_t codecGetSelectInfoAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	return(OM_ERR_INVALID_OP_CODEC);
}

static omfErr_t codecOpenAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	omfErr_t	status = OM_ERR_NONE;

	return(OM_ERR_INVALID_OP_CODEC);
}
 
static omfErr_t codecCreateAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	char					*sep, *ptr, avr[16], signal[16];
	avidAVRdata_t			avrData;
	omfVideoSignalType_t	signalType, videoType;
	avrInfoStruct_t			*info;
	omfRational_t			editRate;
	int						tmplen;
	double          		rate;
	omfRect_t				displayRect;
	omfUInt32				height, width;
	omfFrameLayout_t		layout;
	omfRational_t			aspectRatio;
	omfVideoMemOp_t			opList[5], op;
	userDataAJPG_t			*pdata;
	
	XPROTECT(main)
	{
		media->userData = (userDataAJPG_t *)omOptMalloc(main, sizeof(userDataAJPG_t));
		XASSERT(media->userData != NULL, OM_ERR_NOMEMORY);
	
		pdata = (userDataAJPG_t *)media->userData;

		/* FORMAT of AVR variety must be one of:
		 * 		'<product>:<Resolution>:<signalType>' or
		 * 		'<Resolution>:<signalType>' or
		 * 		'<Resolution>'
		 *
		 * Where <resolution> is one of:
		 *		'AVR<#>'
		 * 		'JFIF<ratio>' or
		 * 		'Uncomp422'
		 *
		 * 		Currently supported JFIF ratios are:
		 *			15-1s	(single field)
		 *			2-1s	(single field)
		 *			2-1s	(single field)
		 *			20-1
		 *			3-1
		 *			2-1
		 *
		 * Signal type is one of:
		 *		NTSC
		 *		PAL
		 *		SECAM
		 */
		if(strncmp(media->codecVariety, "AVR", 3) == 0)
		{
			ptr = media->codecVariety;
			pdata->avrType = OriginalAVR;
		}
		else if(strncmp(media->codecVariety, "JFIF", 4) == 0)
		{
			ptr = media->codecVariety;
			pdata->avrType = JFIF;
		}
		else if(strncmp(media->codecVariety, "Uncomp422", 3) == 0)
		{
			ptr = media->codecVariety;
			pdata->avrType = Uncompressed422;
		}
		else
		{
			sep = strchr(media->codecVariety, ':');
			if(sep == NULL)
				RAISE(OM_ERR_BAD_VARIETY);
			ptr = sep+1;
		}

		if(strncmp(media->codecVariety, "JFIF", 4) == 0)
		{
			sep = strchr(ptr, ':');		/* Skip over the colon which is part of the name */
			if(sep != NULL)
				sep = strchr(sep+1, ':');
		}
		else
			sep = strchr(ptr, ':');
		if(sep != NULL)
		{
			XASSERT(sep-ptr > 0 && sep-ptr < sizeof(avr), OM_ERR_BAD_VARIETY);
			strncpy(avr, ptr, sep-ptr);
			avr[sep-ptr] = '\0';
			ptr = sep+1;

			strncpy(signal, ptr, sizeof(signal));
			if(strcmp(signal, "NTSC") == 0)
				signalType = kNTSCSignal;
			else if(strcmp(signal, "PAL") == 0)
				signalType = kPALSignal;
			else if(strcmp(signal, "SECAM") == 0)
				signalType = kSECAMSignal;
			else
				signalType = kVideoSignalNull;
		}
		else
		{
			signalType = kVideoSignalNull;
			XASSERT(strlen(ptr) < sizeof(avr), OM_ERR_BAD_VARIETY);
			strcpy(avr, ptr);
		}

		

		avrData.product = avidComposerMac;
		avrData.avr = avr;
		CHECK(avrGetInfoStruct(avrData, &info));
		
		if(parmblk->fileRev == kOmfRev2x)
		{
			CHECK(omfsReadRational(main, media->mdes, OMMDFLSampleRate, &editRate));
		}
		else
		{
			CHECK(omfsReadExactEditRate(main, media->mdes, OMMDFLSampleRate, &editRate));
		}

		rate = (double)editRate.numerator / (double)editRate.denominator;
		switch (signalType)
		{
		case kVideoSignalNull:
			/* figure out whether the video is PAL or NTSC */
			
			if (fabs(rate - NTSC_rate) < EPSILON)
			  videoType = kNTSCSignal;
			else if (fabs(rate - PAL_rate) < EPSILON)
			  videoType = kPALSignal;
			else
			  RAISE(OM_ERR_BADRATE);
			break;
			
		case kNTSCSignal:
			if (fabs(rate - NTSC_rate) < EPSILON ||
				fabs(rate - FILM_rate) < EPSILON)
				videoType = kNTSCSignal;
			else
			  RAISE(OM_ERR_BADRATE);
			break;
			
		case kPALSignal:
		case kSECAMSignal:
			if (fabs(rate - PAL_rate) < EPSILON ||
				fabs(rate - FILM_rate) < EPSILON)
				videoType = kPALSignal;
			else
			  RAISE(OM_ERR_BADRATE);
			break;
			
		default:
			RAISE(OM_ERR_INVALID_VIDEOSIGNALTYPE);
			break;
		}

		
		CHECK(avrGetHeight(videoType, info, &height));
		CHECK(avrGetWidth(info, &width));
		CHECK(avrGetFrameLayout(info, &layout));
		CHECK(avrGetAspectRatio(info, &aspectRatio));

		CHECK(omfsFileSetValueAlignment(main, (omfUInt32)4));
		
		CHECK(createPassThroughAVR(parmblk, media, main));
		
		CHECK(omfmVideoOpInit(main, opList));
			
		op.opcode = kOmfFrameLayout;
		op.operand.expFrameLayout = layout;
		CHECK(omfmVideoOpAppend(main, kOmfAppendIfAbsent, op, opList, 5L));
		
		op.opcode = kOmfStoredRect;
		op.operand.expRect.xSize = width;
		op.operand.expRect.ySize = height;
		op.operand.expRect.xOffset = op.operand.expRect.yOffset = 0;
		CHECK(omfmVideoOpAppend(main, kOmfAppendIfAbsent, op, opList, 5L));
		
		op.opcode = kOmfAspectRatio;
		op.operand.expRational = aspectRatio;
		CHECK(omfmVideoOpAppend(main, kOmfAppendIfAbsent, op, opList, 5L));
		
		CHECK(codecPutVideoInfo(media, opList));

		if (avrNeedsDisplayRect(videoType, info, &displayRect))
		{
			CHECK(omfmSetDisplayRect(media, displayRect));
			
	#if SAMPLED_NEEDED
			/* AVID AVR media has 8 black lines for CCube purposes,
			   and optionally some vertical interval lines.  If the
			   display is more than 8 lines away from the top of the
			   stored region, then subtract the VI from the offset,
			   and add it to the sampled YSize. */
			if (displayRect.yOffset > 8)
			{
				displayRect.ySize += (displayRect.yOffset - 8);
				displayRect.yOffset = 8;
				CHECK(omfmSetSampledRect(media, displayRect));
			}
	#endif			     
		}
			     
		if(pdata->avrType == OriginalAVR)
		{
			CHECK(omfmCodecSendPrivateData(media,
	                               sizeof(avrInfoStruct_t),
	                               (void *)info));
	    }
		else if((pdata->avrType == Uncompressed422) || 
		        (pdata->avrType == JFIF))
		{
			tmplen = strlen(info->avrName);
			ptr = info->avrName;
			ptr += tmplen - 1;
			if ((*ptr == 'P') || (*ptr == 'p'))
			{/* we are using a progressive resolution */
				omfVideoMemOp_t	yuvFileFmtMixed[8], rgbVideoMemFmt[4];
				
				yuvFileFmtMixed[0].opcode = kOmfCDCICompWidth;
				yuvFileFmtMixed[0].operand.expInt32 = 8;
				yuvFileFmtMixed[1].opcode = kOmfCDCIHorizSubsampling;
				if(pdata->avrType == Uncompressed422)
					yuvFileFmtMixed[1].operand.expInt32 = 2;
				else
					yuvFileFmtMixed[1].operand.expInt32 = 1;

				yuvFileFmtMixed[2].opcode = kOmfFrameLayout;
				yuvFileFmtMixed[2].operand.expFrameLayout = kFullFrame;

				yuvFileFmtMixed[3].opcode = kOmfCDCIBlackLevel;
				yuvFileFmtMixed[3].operand.expUInt32 = 16;
				yuvFileFmtMixed[4].opcode = kOmfCDCIWhiteLevel;
				yuvFileFmtMixed[4].operand.expUInt32 = 235;
				yuvFileFmtMixed[5].opcode = kOmfCDCIColorRange;
				yuvFileFmtMixed[5].operand.expUInt32 = 225;

				yuvFileFmtMixed[6].opcode = kOmfVFmtEnd;
				CHECK(omfmPutVideoInfoArray(media, yuvFileFmtMixed));
			
				rgbVideoMemFmt[0].opcode = kOmfPixelFormat;
				rgbVideoMemFmt[0].operand.expPixelFormat = kOmfPixRGBA;
				rgbVideoMemFmt[1].opcode = kOmfRGBCompSizes;
				rgbVideoMemFmt[1].operand.expCompSizeArray[0] = 8;
				rgbVideoMemFmt[1].operand.expCompSizeArray[1] = 8;
				rgbVideoMemFmt[1].operand.expCompSizeArray[2] = 8;
				rgbVideoMemFmt[1].operand.expCompSizeArray[3] = 0; /* terminating zero */
				rgbVideoMemFmt[2].opcode = kOmfRGBCompLayout;
				rgbVideoMemFmt[2].operand.expCompArray[0] = 'R';
				rgbVideoMemFmt[2].operand.expCompArray[1] = 'G';
				rgbVideoMemFmt[2].operand.expCompArray[2] = 'B';
				rgbVideoMemFmt[2].operand.expCompArray[3] = 0; /* terminating zero */




				rgbVideoMemFmt[3].opcode = kOmfVFmtEnd;
				CHECK(omfmSetVideoMemFormat(media,rgbVideoMemFmt));
			}
			else
			{
				omfVideoMemOp_t	yuvFileFmtMixed[5], rgbVideoMemFmt[4];
				
				yuvFileFmtMixed[0].opcode = kOmfCDCICompWidth;
				yuvFileFmtMixed[0].operand.expInt32 = 8;
				yuvFileFmtMixed[1].opcode = kOmfCDCIHorizSubsampling;
				if(pdata->avrType == Uncompressed422)
					yuvFileFmtMixed[1].operand.expInt32 = 2;
				else
					yuvFileFmtMixed[1].operand.expInt32 = 1;
				yuvFileFmtMixed[2].opcode = kOmfFrameLayout;
				if(info->NumberOfFields == 2)
					yuvFileFmtMixed[2].operand.expFrameLayout = kMixedFields;
				else
					yuvFileFmtMixed[2].operand.expFrameLayout = kOneField;
				yuvFileFmtMixed[3].opcode = kOmfVFmtEnd;
				CHECK(omfmPutVideoInfoArray(media, yuvFileFmtMixed));
			
				rgbVideoMemFmt[0].opcode = kOmfPixelFormat;
				rgbVideoMemFmt[0].operand.expPixelFormat = kOmfPixRGBA;
				rgbVideoMemFmt[1].opcode = kOmfRGBCompSizes;
				rgbVideoMemFmt[1].operand.expCompSizeArray[0] = 8;
				rgbVideoMemFmt[1].operand.expCompSizeArray[1] = 8;
				rgbVideoMemFmt[1].operand.expCompSizeArray[2] = 8;
				rgbVideoMemFmt[1].operand.expCompSizeArray[3] = 0; /* terminating zero */
				rgbVideoMemFmt[2].opcode = kOmfRGBCompLayout;
				rgbVideoMemFmt[2].operand.expCompArray[0] = 'R';
				rgbVideoMemFmt[2].operand.expCompArray[1] = 'G';
				rgbVideoMemFmt[2].operand.expCompArray[2] = 'B';
				rgbVideoMemFmt[2].operand.expCompArray[3] = 0; /* terminating zero */
				rgbVideoMemFmt[3].opcode = kOmfVFmtEnd;
				CHECK(omfmSetVideoMemFormat(media,rgbVideoMemFmt));
			}
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

static omfErr_t codecCloseAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	userDataAJPG_t	*pdata = (userDataAJPG_t *)media->userData;
			
	return(codecPassThrough(&pdata->clientCodec, kCodecClose, parmblk, media, main, pdata->clientPData));
}

static omfErr_t codecVarietyCountAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	if ((strcmp(parmblk->spc.varietyCount.mediaKindString, PICTUREKIND) == 0))
	{
		parmblk->spc.varietyCount.count = sizeof(avrTable) / sizeof(avrInfoStruct_t);
	}
	else
	{
		parmblk->spc.varietyCount.count = 0;
	}
	return(OM_ERR_NONE);
}

static omfErr_t createPassThroughAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	userDataAJPG_t			*saveUserData;
	codecTable_t   			saveCodecInfo;
	omfBool					found;
	userDataAJPG_t			*pdata = (userDataAJPG_t *) media->userData;

	XPROTECT(main)
	{
		saveUserData = (userDataAJPG_t *) media->userData;
		saveCodecInfo = media->pvt->codecInfo;
		switch(pdata->avrType)
		{
			case OriginalAVR:
				omfsTableLookupBlock(main->session->codecID, CODEC_TIFF_VIDEO, sizeof(pdata->clientCodec),
						 &pdata->clientCodec, &found);
				if(!found)
					RAISE(OM_ERR_CODEC_INVALID);
				break;
			case JFIF:
				omfsTableLookupBlock(main->session->codecID, CODEC_AVID_JFIF_VIDEO, sizeof(pdata->clientCodec),
						 &pdata->clientCodec, &found);
				if(!found)
					RAISE(OM_ERR_CODEC_INVALID);
				break;
			case Uncompressed422:
				omfsTableLookupBlock(main->session->codecID, CODEC_CDCI_VIDEO, sizeof(pdata->clientCodec),
						 &pdata->clientCodec, &found);
				if(!found)
					RAISE(OM_ERR_CODEC_INVALID);
				break;
			default:
				RAISE(OM_ERR_CODEC_INVALID);
		}
		CHECK(codecPassThrough(&pdata->clientCodec, kCodecCreate, parmblk, media, main, NULL));
		pdata->clientPData = media->userData;
		media->userData = saveUserData;
		media->pvt->codecInfo = saveCodecInfo;
	}
	XEXCEPT
	{
		media->userData = saveUserData;
		media->pvt->codecInfo = saveCodecInfo;
	}
	XEND
	
	return(OM_ERR_NONE);
}

static omfErr_t codecVarietyInfoAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	char			*namePtr;

	namePtr = avrTable[parmblk->spc.varietyInfo.index-1].avrName;
	parmblk->spc.varietyInfo.varietyName = namePtr;
	return(OM_ERR_NONE);
}

static omfErr_t codecGetInfoAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	omfErr_t	status = OM_ERR_NONE;
	userDataAJPG_t	*pdata = (userDataAJPG_t *)media->userData;

	switch (parmblk->spc.mediaInfo.infoType)
	{
	case kCompressionParms:
		status = OM_ERR_BADCOMPR;
		break;
    /* ROGER-- let kJPEGTables through.  It's not a bad idea 
        for the user to get this stuff, even though the purpose
        of this avr codec is to hide details.  More importantly,
        the omJPEG.ccode calls kCodecGetInfo:kJPEGTables to extract 
        the tables for the software codec.
        
	case kJPEGTables:
		status = OM_ERR_BADCOMPR;
		break;
		
	   ROGER-- end comment */
	   
	default:
		status = codecPassThrough(&pdata->clientCodec, kCodecGetInfo, parmblk, media, main, pdata->clientPData);
		break;
	}

	return(status);
}

static omfErr_t codecPutInfoAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	omfErr_t		status = OM_ERR_NONE;
	userDataAJPG_t	*pdata = (userDataAJPG_t *)media->userData;

	switch (parmblk->spc.mediaInfo.infoType)
	{
	case kCompressionParms:
		status = myCodecSetPrivateData(parmblk, main, media, pdata);
		break;
	case kJPEGTables:
		status = OM_ERR_BADCOMPR;
		break;

	default:
		status = codecPassThrough(&pdata->clientCodec, kCodecPutInfo, parmblk, media, main, pdata->clientPData);
		break;
	}

	return(status);
}

static omfErr_t codecNumChannelsAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	userDataAJPG_t	*pdata = (userDataAJPG_t *)media->userData;

	return(codecPassThrough(&pdata->clientCodec, kCodecGetNumChannels, parmblk, media, main, pdata->clientPData));
}

static omfErr_t codecSetFrameAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	omfErr_t	status = OM_ERR_NONE;
	userDataAJPG_t	*pdata = (userDataAJPG_t *)media->userData;

	status = codecPassThrough(&pdata->clientCodec, kCodecSetFrame, parmblk, media, main, pdata->clientPData);
	
	return(status);
}

static omfErr_t codecReadSamplesAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	omfErr_t	status = OM_ERR_NONE;
	userDataAJPG_t	*pdata = (userDataAJPG_t *)media->userData;
	
	status = codecPassThrough(&pdata->clientCodec, kCodecReadSamples, parmblk, media, main, pdata->clientPData);
		
	return(status);
}

static omfErr_t codecWriteSamplesAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	omfErr_t	status = OM_ERR_NONE;
	userDataAJPG_t	*pdata = (userDataAJPG_t *)media->userData;
	
	status = codecPassThrough(&pdata->clientCodec, kCodecWriteSamples, parmblk, media, main, pdata->clientPData);

	return(status);
}

static omfErr_t codecReadLinesAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	omfErr_t	status = OM_ERR_NONE;
	userDataAJPG_t	*pdata = (userDataAJPG_t *)media->userData;
	
	status = codecPassThrough(&pdata->clientCodec, kCodecReadLines, parmblk, media, main, pdata->clientPData);

	return(status);
}

static omfErr_t codecWriteLinesAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	omfErr_t	status = OM_ERR_NONE;
	userDataAJPG_t	*pdata = (userDataAJPG_t *)media->userData;
	
	status = codecPassThrough(&pdata->clientCodec, kCodecWriteLines, parmblk, media, main, pdata->clientPData);

	return(status);
}

static omfErr_t codecInitMDESPropsAVR(omfCodecParms_t * parmblk,
								omfMediaHdl_t 			media,
								omfHdl_t				main)
{
	omfErr_t		status = OM_ERR_NONE;
	codecTable_t	clientCodec;
	omfBool			found = FALSE;
	if(parmblk->spc.initMDES.variant != NULL)
	{
		if(strncmp(parmblk->spc.initMDES.variant, "AVR", 3) == 0)
		{
					omfsTableLookupBlock(main->session->codecID, CODEC_TIFF_VIDEO, sizeof(clientCodec),
							 &clientCodec, &found);
		}
		else if(strncmp(parmblk->spc.initMDES.variant, "JFIF", 4) == 0)
		{
					omfsTableLookupBlock(main->session->codecID, CODEC_AVID_JFIF_VIDEO, sizeof(clientCodec),
							 &clientCodec, &found);
		}
		else if(strncmp(parmblk->spc.initMDES.variant, "Uncomp422", 3) == 0)
		{
					omfsTableLookupBlock(main->session->codecID, CODEC_CDCI_VIDEO, sizeof(clientCodec),
							 &clientCodec, &found);
		}
		if(found)
			status = codecPassThrough(&clientCodec, kCodecInitMDESProps, parmblk, media, main, NULL);
		else
			status = OM_ERR_CODEC_INVALID;
		
	}
			
	return(status);
}

/************************
 * myCodecInit
 *
 * 		Registers dynamic properties and types.  Will not check
 *         for previous calls, since it doesn't hurt.
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

	omcAJPGPersistent_t	*persist;
	omfHdl_t			file = NULL;
	
	omfAssert(parmblk, file, OM_ERR_NULL_PARAM);
	/* first, allocate a persistent "cookie" that will hold codec-dynamic 
	 * values, like property and type handles 
	 */
	   
	XPROTECT(NULL)
	{
		persist = (omcAJPGPersistent_t *)
		  omfsMalloc(sizeof(omcAJPGPersistent_t));
		XASSERT(persist != NULL, OM_ERR_NOMEMORY);
	
		/* for this codec only! Save the session so that a codec
		   lookup can be done later (in omfCodecAJPG) */
		thisSession = parmblk->spc.init.session;
		
		/* put the cookie pointer in the parameter block for return */
		parmblk->spc.init.persistRtn = persist;
					
		/* add the codec-related properties.  See the OMF Interchange 
		 * Specification 2.0 for details.
		 */
		CHECK(omfsRegisterDynamicProp(parmblk->spc.init.session, 
						kOmfTstRevEither, "ComponentWidth", OMClassCDCI, 
					    OMVersionType, kPropRequired, 
					    &persist->omCDCIComponentWidth));
					    
		CHECK(omfsRegisterDynamicProp(parmblk->spc.init.session, 
						kOmfTstRevEither, "HorizontalSubsampling",
						OMClassCDCI, OMBoolean, kPropRequired,
					   	&persist->omCDCIHorizontalSubsampling));
					   	
		CHECK(omfsRegisterDynamicProp(parmblk->spc.init.session, 
						kOmfTstRevEither, "ColorSiting", OMClassCDCI, 
					   	OMBoolean, kPropRequired, 
					   	&persist->omCDCIColorSiting));
					   	
		CHECK(omfsRegisterDynamicProp(parmblk->spc.init.session, 
						kOmfTstRevEither, "BlackReferenceLevel",
						OMClassCDCI, OMInt32, kPropOptional, 
						&persist->omCDCIBlackReferenceLevel));
						
		CHECK(omfsRegisterDynamicProp(parmblk->spc.init.session, 
						kOmfTstRevEither, "WhiteReferenceLevel",
						OMClassCDCI, OMInt32, kPropOptional, 
						&persist->omCDCIWhiteReferenceLevel));
 
 	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * initUserData
 *
 * 		Initializes mediaPtr-persistent local codec data structure.
 *
 * Argument Notes:
 *		
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfErr_t initUserData(userDataAJPG_t *pdata)
{
	omfHdl_t file = NULL;
	
	omfAssert(pdata, file, OM_ERR_NULL_PARAM);
	
/*	pdata->imageWidth = 0;
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
	pdata->bitsPerPixel = 0;
	pdata->rate.numerator = 0;
	pdata->rate.denominator = 0;
	pdata->fileLayout = kNoLayout;	/ Frame of fields per sample
	pdata->filePixelFormat = kOmfPixYUV;

	pdata->numLines = 0;
	pdata->fmtOps[0].opcode = kOmfVFmtEnd;
	pdata->imageAspectRatio.numerator = 0;
	pdata->imageAspectRatio.denominator = 0;
	pdata->descriptorFlushed = FALSE;
*/	
	pdata->clientPData = NULL;

	return(OM_ERR_NONE);
}


/************************
 * myCodecSetPrivateData
 *
 * 		Takes an AVR setting and sends the right tables to the 
 *        TIFF codec (1.x)
 *
 * Argument Notes:
 *		
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfErr_t myCodecSetPrivateData(omfCodecParms_t *parmblk, 
									  omfHdl_t file, 
									  omfMediaHdl_t media, 
									  userDataAJPG_t *pdata)
{
	omfUInt16 tableByteSize;
	omfUInt8 * 	jpegModifiedQTableLum=NULL;
	omfUInt8 *	jpegModifiedQTableChr=NULL;
	omfUInt16 * jpegModifiedQTableLum16=NULL;
	omfUInt16 *	jpegModifiedQTableChr16=NULL;
	avrInfoStruct_t *myData = (avrInfoStruct_t *) parmblk->spc.mediaInfo.buf;
	omfTIFF_JPEGInfo_t  TIFFparms;
	omfJPEGInfo_t		JPEGParms;
	omfCDCIInfo_t		CDCIParms;
	omfJPEGTables_t jpegTables;
	Boolean isJFIF=(strncmp("JFIF", myData->avrName, 4) == 0);

	
	XPROTECT(file)
	{
		if (parmblk->spc.mediaInfo.bufLen != sizeof(avrInfoStruct_t))
			RAISE(OM_ERR_INTERN_TOO_SMALL);
	
		if(pdata->avrType == OriginalAVR)
		{
			TIFFparms.JPEGTableID = myData->TableID;
			TIFFparms.compression = kJPEG;
			parmblk->spc.mediaInfo.buf = (void *)&TIFFparms;
			parmblk->spc.mediaInfo.bufLen = sizeof(TIFFparms);
			CHECK(codecPassThrough(&pdata->clientCodec, kCodecPutInfo, parmblk, media, media->mainFile, pdata->clientPData));
		}
		else if(pdata->avrType == JFIF)
		{
			JPEGParms.JPEGTableID = myData->TableID;
			parmblk->spc.mediaInfo.buf = (void *)&JPEGParms;
			parmblk->spc.mediaInfo.bufLen = sizeof(JPEGParms);
			CHECK(codecPassThrough(&pdata->clientCodec, kCodecPutInfo, parmblk, media, media->mainFile, pdata->clientPData));
		}
		else if(pdata->avrType == Uncompressed422)
		{
			CDCIParms.CDCITableID = myData->TableID;
			parmblk->spc.mediaInfo.buf = (void *)&CDCIParms;
			parmblk->spc.mediaInfo.bufLen = sizeof(CDCIParms);
			CHECK(codecPassThrough(&pdata->clientCodec, kCodecPutInfo, parmblk, media, media->mainFile, pdata->clientPData));
		}

		parmblk->spc.mediaInfo.buf = (void *)&jpegTables;
		parmblk->spc.mediaInfo.bufLen = sizeof(jpegTables);		
		parmblk->spc.mediaInfo.infoType = kJPEGTables;
		
		jpegTables.JPEGcomp = kJcLuminance;
#if ! STANDARD_JPEG_Q
		if (myData->QT16[0] & 0x00ff)
		{
			if(isJFIF)
			{
				/* we are dealing with JFIF media - so therefore we need to scale the table */
				tableByteSize=JPEG_QSIZE16*sizeof(omfUInt8);
				jpegModifiedQTableLum16=(omfUInt16*)omfsMalloc(tableByteSize);
				memcpy(jpegModifiedQTableLum16, myData->QT16, tableByteSize);
				ScaleQTables16(jpegModifiedQTableLum16,((double)myData->QFactor)/JFIF_QFACTOR_MULTIPLIER);
				jpegTables.QTables = (omfUInt8 *)jpegModifiedQTableLum16;
			}
			else
				jpegTables.QTables = (omfUInt8 *)(myData->QT16);
			jpegTables.QTableSize = JPEG_QSIZE16;
		}
		else
#endif
		{
			if(isJFIF)
			{
				/* we are dealing with JFIF media - so therefore we need to scale the table */
				tableByteSize=JPEG_QSIZE8*sizeof(omfUInt8);
				jpegModifiedQTableLum=(omfUInt8 *)omfsMalloc(tableByteSize);
				memcpy(jpegModifiedQTableLum,myData->QT,tableByteSize);
				ScaleQTables(jpegModifiedQTableLum,((double)myData->QFactor)/JFIF_QFACTOR_MULTIPLIER);
				jpegTables.QTables = jpegModifiedQTableLum;
			}
			else
				jpegTables.QTables = myData->QT;
			jpegTables.QTableSize = JPEG_QSIZE8;
		}
		jpegTables.ACTables = STD_ACT_PTR[0];
		jpegTables.ACTableSize = (omfInt16) STD_ACT_LEN[0];
		jpegTables.DCTables = STD_DCT_PTR[0];
		jpegTables.DCTableSize = (omfInt16) STD_DCT_LEN[0];
		
		CHECK(codecPassThrough(&pdata->clientCodec, kCodecPutInfo, parmblk, media, media->mainFile, pdata->clientPData));
		
		jpegTables.JPEGcomp = kJcChrominance;
#if ! STANDARD_JPEG_Q
		if ( myData->QT16[0] & 0x00ff)
		{
			if(isJFIF)
			{
				/* we are dealing with JFIF media - so therefore we need to scale the table */
				tableByteSize=JPEG_QSIZE16*sizeof(omfUInt8);
				jpegModifiedQTableChr16=(omfUInt16 *)omfsMalloc(tableByteSize);
				memcpy(jpegModifiedQTableChr16, (omfUInt8 *)&(myData->QT16[64]), tableByteSize);
				ScaleQTables16(jpegModifiedQTableChr16,((double)myData->QFactor)/JFIF_QFACTOR_MULTIPLIER);
				jpegTables.QTables = (omfUInt8 *)jpegModifiedQTableChr16;
			}
			else
				jpegTables.QTables = (omfUInt8 *)&(myData->QT16[64]);
			jpegTables.QTableSize = JPEG_QSIZE16;
		}
		else
#endif
		{			
			if(isJFIF)
			{
				/* we are dealing with JFIF media - so therefore we need to scale the table */
				tableByteSize=JPEG_QSIZE8*sizeof(omfUInt8);
				jpegModifiedQTableChr=(omfUInt8 *)omfsMalloc(tableByteSize);
				memcpy(jpegModifiedQTableChr, &(myData->QT[JPEG_QSIZE8]), tableByteSize);
				ScaleQTables(jpegModifiedQTableChr,((double)myData->QFactor)/JFIF_QFACTOR_MULTIPLIER);
				jpegTables.QTables = jpegModifiedQTableChr;
			}
			else
				jpegTables.QTables = &(myData->QT[JPEG_QSIZE8]);
			jpegTables.QTableSize = JPEG_QSIZE8;
		}
		jpegTables.ACTables = STD_ACT_PTR[1];
		jpegTables.ACTableSize = (omfInt16) STD_ACT_LEN[1];
		jpegTables.DCTables = STD_DCT_PTR[1];
		jpegTables.DCTableSize = (omfInt16) STD_DCT_LEN[1];
		
		CHECK(codecPassThrough(&pdata->clientCodec, kCodecPutInfo, parmblk, media, media->mainFile, pdata->clientPData));

		OMFS_FREE_AND_NULL_IF_SET(jpegModifiedQTableChr);
		OMFS_FREE_AND_NULL_IF_SET(jpegModifiedQTableChr16);
		OMFS_FREE_AND_NULL_IF_SET(jpegModifiedQTableLum);
		OMFS_FREE_AND_NULL_IF_SET(jpegModifiedQTableLum16);
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: avrVideoMediaCreate
 *
 * 		Creates a single stream of Avid JPEG-compatible video media.
 *
 *		The media handle from this call can be used with
 *		omfmWriteDataSamples or omfmWriteDataLines, but NOT with
 *		omfmWriteMultiSamples.
 *
 * Argument Notes:
 *		If you are creating the media, and then attaching it to a master
 *		mob, then the "masterMob" field may be left NULL.
 *
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */

omfErr_t avrVideoMediaCreate(
			omfHdl_t		filePtr,   /* IN -- In this file */
			omfObject_t		masterMob, /* IN -- on this master mob */
			omfTrackID_t    trackID,
			omfObject_t		fileMob,   /* IN -- on this file mob  */
			omfRational_t	editrate,
			avidAVRdata_t	avrData,   /* avr settings data */
			omfVideoSignalType_t signalType,
			omfMediaHdl_t	*resultPtr)	/* OUT -- Return object here. */
{

	omfRect_t			displayRect;
	omfUInt32			height, width;
	omfFrameLayout_t	layout;
	omfRational_t		aspectRatio;
	avrInfoStruct_t		*info;
	double              rate;
	omfVideoSignalType_t videoType;
	
	XPROTECT(filePtr)
	{	
		CHECK(avrGetInfoStruct(avrData, &info));

		rate = (double)editrate.numerator / (double)editrate.denominator;
		switch (signalType)
		{
		case kVideoSignalNull:
			/* figure out whether the video is PAL or NTSC */
			
			if (fabs(rate - NTSC_rate) < EPSILON)
			  videoType = kNTSCSignal;
			else if (fabs(rate - PAL_rate) < EPSILON)
			  videoType = kPALSignal;
			else
			  RAISE(OM_ERR_BADRATE);
			break;
			
		case kNTSCSignal:
			if (fabs(rate - NTSC_rate) < EPSILON ||
				fabs(rate - FILM_rate) < EPSILON)
				videoType = kNTSCSignal;
			else
			  RAISE(OM_ERR_BADRATE);
			break;
			
		case kPALSignal:
		case kSECAMSignal:
			if (fabs(rate - PAL_rate) < EPSILON ||
				fabs(rate - FILM_rate) < EPSILON)
				videoType = kPALSignal;
			else
			  RAISE(OM_ERR_BADRATE);
			break;
			
		default:
			RAISE(OM_ERR_INVALID_VIDEOSIGNALTYPE);
			break;
		}

		
		CHECK(avrGetHeight(videoType, info, &height));
		CHECK(avrGetWidth(info, &width));
		CHECK(avrGetFrameLayout(info, &layout));
		CHECK(avrGetAspectRatio(info, &aspectRatio));

		CHECK(omfsFileSetValueAlignment(filePtr, (omfUInt32)4));

		{
			omfObject_t	mdes;
			char		codec[128];
			
			sprintf(codec, "%s:%s:%s", CODEC_AJPG_VIDEO, avrData.avr,
				signalType == kNTSCSignal ? "NTSC" : "PAL");
		  	CHECK(omfmMobGetMediaDescription(filePtr, fileMob, &mdes));
			CHECK(omfsWriteString(filePtr, mdes, OMMDESCodecID, codec));
		}
				
		CHECK(omfmVideoMediaCreate(filePtr, masterMob, trackID, fileMob, 
			     kToolkitCompressionEnable,
			     editrate, 
			     height, 
			     width,
			     layout, 
			     aspectRatio, resultPtr));
			     
		if (avrNeedsDisplayRect(videoType, info, &displayRect))
		{
			CHECK(omfmSetDisplayRect(*resultPtr, displayRect));
			
#if SAMPLED_NEEDED
			/* AVID AVR media has 8 black lines for CCube purposes,
			   and optionally some vertical interval lines.  If the
			   display is more than 8 lines away from the top of the
			   stored region, then subtract the VI from the offset,
			   and add it to the sampled YSize. */
			if (displayRect.yOffset > 8)
			{
				displayRect.ySize += (displayRect.yOffset - 8);
				displayRect.yOffset = 8;
				CHECK(omfmSetSampledRect(*resultPtr, displayRect));
			}
#endif			     
		}
			     
		CHECK(omfmCodecSendPrivateData(*resultPtr,
                                   sizeof(avrInfoStruct_t),
                                   (void *)info));
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}
static omfErr_t   avrGetWidth(avrInfoStruct_t *data, omfUInt32 *result)
{
	*result = data->XSize;
	
	return(OM_ERR_NONE);

}

static omfErr_t   avrGetHeight(omfVideoSignalType_t videoType, avrInfoStruct_t *data,
							   omfUInt32 *result)
{

	/* assume that the editrate is either PAL or NTSC */
    switch (videoType)
    {
    case kNTSCSignal:
	   *result = data->YSizeNTSC + data->LeadingLinesNTSC;
	   break;

	case kPALSignal:
	   *result = data->YSizePAL + data->LeadingLinesPAL;
	   break;
	
	default:
	   return OM_ERR_INVALID_VIDEOSIGNALTYPE;
	}

	return OM_ERR_NONE;
}
static omfErr_t   avrGetFrameLayout(avrInfoStruct_t *data, 
									omfFrameLayout_t *result)
{
	char	*ptr;
	int		tmplen;

	tmplen = strlen(data->avrName);
	ptr = data->avrName;
	ptr += tmplen - 1;
	if ((*ptr == 'P') || (*ptr == 'p'))
		*result = kFullFrame;	/* we are using a progressive resolution */
	else if (data->NumberOfFields == 2)
		*result = kSeparateFields;
	else
		*result = kOneField;
	
	return(OM_ERR_NONE);
}

static omfErr_t   avrGetAspectRatio(avrInfoStruct_t *data, 
									omfRational_t *result)
{
	MakeRational(4,3,result);
	return(OM_ERR_NONE);
}

static omfBool   avrNeedsDisplayRect(omfVideoSignalType_t videoType,
									 avrInfoStruct_t *data, 
									 omfRect_t *result)
{
	omfInt32 leadingLines;

	/* assume that the editrate is either PAL or NTSC */
    switch (videoType)
    {
    case kNTSCSignal:
	   leadingLines = data->LeadingLinesNTSC;
	   break;

	case kPALSignal:
	   leadingLines = data->LeadingLinesPAL;
	   break;
	
	default:
	   return FALSE; /* this should never be executed */
	}

	result->xOffset = 0;
	result->yOffset = 0;
	result->xSize = 0;
	result->ySize = 0;
	
	if (leadingLines == 0)
		return(FALSE);
	
	result->xOffset = 0;
	result->yOffset = leadingLines;
	
	    
	result->xSize = data->XSize;
	
	if (videoType == kNTSCSignal)
	  result->ySize = data->YSizeNTSC;
	else
	  result->ySize = data->YSizePAL;
	
	return(TRUE);
}

static omfErr_t avrGetInfoStruct(avidAVRdata_t avrData, 
								 avrInfoStruct_t **info)
{
	omfInt32 i = 0;
	
	while (i < MAX_NUM_AVRS && avrTable[i].avrName != NULL)
	{
	/* If two underbars start the resolution name, it's reserved */
		if (strcmp(avrData.avr, avrTable[i].avrName) == 0 &&
		    strncmp(avrData.avr, "__", 2) != 0)
		{
			*info = &avrTable[i];
			return(OM_ERR_NONE);
		}
		i++;
	}
	
	return(OM_ERR_NOT_IMPLEMENTED);
}


omfErr_t avrGetFormatData( omfInt32 index, /* IN - index */
						  int is_pal,  /* IN - true if we want pal height */
						  char **name, /* OUT - name of AVR */
						  int *width,  /* OUT - width of image (field) */
						  int *height, /* OUT - height of image (field) */
						  int *nfields, /* OUT - no of fields */
						  int *black_line_count) /* OUT - no. of black lines at top of field */
{
  if (index>=0 && index < MAX_NUM_AVRS)
    {
	  *name = avrTable[index].avrName;
	  *width = avrTable[index].XSize;
	  *nfields = avrTable[index].NumberOfFields;
	  if (is_pal)
		{
		  *height = avrTable[index].YSizePAL;
		  *black_line_count = avrTable[index].LeadingLinesPAL;
		}
	  else
		{
		  *height = avrTable[index].YSizeNTSC;
		  *black_line_count = avrTable[index].LeadingLinesNTSC;
		}
	  return(OM_ERR_NONE);
    }
  *name = NULL;
    
  return(OM_ERR_NOT_IMPLEMENTED);
}



void ScaleQTables(omfUInt8* qTable, double scaleFactor) 
{
	unsigned char 	i;
	double			tableElem;

	for (i=0;i<JPEG_QSIZE8;i++)
	{
		/*Scale Luma Table*/
		tableElem = ((double)(*qTable) * scaleFactor) + 0.5;
		CLIP(1, 255, tableElem); 
		*qTable++ = (omfUInt8)tableElem;

	}
}
void ScaleQTables16(omfUInt16* qTable, double scaleFactor) 
{
	unsigned char 	i;
	double			tableElem;

	for (i=0;i<JPEG_QSIZE8;i++)
	{
		/*Scale Luma Table*/
		tableElem = ((double)(*qTable) * scaleFactor) + 0.5;
		CLIP(1, 255, tableElem); 
		*qTable++ = (omfUInt16)tableElem;

	}
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
