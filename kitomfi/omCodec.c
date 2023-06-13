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

#if defined(THINK_C) || defined(__MWERKS__)
#include <MixedMode.h>
#endif
#include "masterhd.h"
#include "omPublic.h"
#include "omCodec.h"
#include "omCntPvt.h"
#include "omPvt.h"
#include "omMedia.h"
#include "omcAIFF.h"
#include "omcWAVE.h"
#include "omcTIFF.h"
#include "omcRGBA.h"
#include "omcCDCI.h"
#include "omcJPEG.h"

#define DEFAULT_NUM_CODECS	32

#if defined(PORTKEY_OS_MACSYS) && PORTKEY_CPU_PPC
ProcInfoType uppCodecProcInfo = kPascalStackBased
		 | RESULT_SIZE(SIZE_CODE(sizeof(omfErr_t)))
		 | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(omfCodecParms_t *)))
		 | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(omfMediaHdl_t)))
		 | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(omfHdl_t)));
		 
ProcInfoType uppInitProcInfo = kPascalStackBased
		 | RESULT_SIZE(SIZE_CODE(sizeof(omfErr_t)))
		 | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(omfInt32)))
		 | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(omfCodecParms_t *)));
		 
#define CallBaseCodec(codecPtr, func, parmblk)		\
		((codecPtr)->type == kOMFRegisterPlugin \
			? (omfErr_t)CallUniversalProc((UniversalProcPtr)((codecPtr)->initRtn), uppCodecProcInfo, (func), (parmblk) )		\
			: (*((codecPtr)->initRtn))((func), (parmblk)))
#define CallExtendCodec(codecPtr, func, parmblk,media,main)		\
		((codecPtr)->type == kOMFRegisterPlugin \
			? (omfErr_t)CallUniversalProc((UniversalProcPtr)((codecPtr)->dispTable[func]), uppCodecProcInfo, (parmblk),(media),(main))		\
			: (*((codecPtr)->dispTable[func]))((parmblk),(media),(main)))
#else
#define CallBaseCodec(codecPtr, func, parmblk)		\
			(*((codecPtr)->initRtn))((func), (parmblk))
#define CallExtendCodec(codecPtr, func, parmblk,media,main)		\
			(*((codecPtr)->dispTable[func]))((parmblk),(media),(main))
#endif

#ifdef OMF_CODEC_COMPAT
#define correctCodevRev(rev)	((rev) <= CODEC_CURRENT_REV)
#else
#define correctCodevRev(rev)	((rev) == CODEC_CURRENT_REV)
#endif

static void SetupDefaultParmblk(
			omfCodecParms_t	*parms,
			codecTable_t	*codecPtr,
			omfMediaHdl_t	media);

static omfErr_t codecDefaultFunction(
			omfCodecParms_t * parmblk,
			omfMediaHdl_t	media,
			omfHdl_t		main)
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
omfErr_t codecMgrInit(
			omfSessionHdl_t sess)
{
	omfHdl_t file = NULL;
	
	omfAssert(sess, file, OM_ERR_BAD_SESSION);
	
	XPROTECT(NULL)
	{
		if((sess->codecMDES == NULL) || (sess->codecID == NULL))
		{
			CHECK(omfsNewClassIDTable(NULL, DEFAULT_NUM_CODECS, &sess->codecMDES));
			CHECK(omfsNewStringTable(NULL, TRUE, NULL, DEFAULT_NUM_CODECS, &sess->codecID));
#ifndef OMFI_NO_AUDIO
			CHECK(omfmRegisterCodec(sess, omfCodecAIFF, kOMFRegisterLinked));
			CHECK(omfmRegisterCodec(sess, omfCodecWAVE, kOMFRegisterLinked));
#endif

#ifndef OMFI_NO_IMAGE
			CHECK(omfmRegisterCodec(sess, omfCodecTIFF, kOMFRegisterLinked));
			CHECK(omfmRegisterCodec(sess, omfCodecRGBA, kOMFRegisterLinked));
			CHECK(omfmRegisterCodec(sess, omfCodecCDCI, kOMFRegisterLinked));
			CHECK(omfmRegisterCodec(sess, omfCodecJPEG, kOMFRegisterLinked));
#endif
		}
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * name
 *
 * 		WhatItDoes
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
omfErr_t omfmRegisterCodec(
			omfSessionHdl_t		session,
			omfCodecDispPtr_t	pluginRoutine,
			omfCodecType_t		type)
{
	codecTable_t		codecBlock;
	omfCodecMetaInfo_t	info;
	omfCodecParms_t		parms;
	omfInt32			n;
	
	XPROTECT(NULL)
	{
		memset(&info, 0, sizeof(info));		/* clear struct padding for BoundsChecker */
		codecBlock.rev = 0;
		codecBlock.type = type;
		codecBlock.persist = NULL;
		codecBlock.initRtn = pluginRoutine;
		CHECK(codecInit(session, &codecBlock, &codecBlock.persist));

		info.dataKindNameList = "";			/* Just in case old codecs fail to set this */
		info.minFileRev = kOmfRev2x;		/* Just in case old codecs fail to set this */
		info.maxFileRevIsValid = FALSE;		/* Just in case old codecs fail to set this */
		info.maxFileRev = kOmfRev2x;		/* Just in case old codecs fail to set this */
		CHECK(codecGetMetaInfo(session, &codecBlock, NULL, NULL, 0, &info));
		codecBlock.rev = info.rev;
		codecBlock.dataKindNameList = info.dataKindNameList;
		codecBlock.minFileRev = info.minFileRev;
		codecBlock.maxFileRevIsValid = info.maxFileRevIsValid;
		codecBlock.maxFileRev = info.maxFileRev;
		XASSERT(correctCodevRev(info.rev), OM_ERR_BAD_CODEC_REV);
		
#ifdef OMF_CODEC_COMPAT
 		if(info.rev >= CODEC_REV_3)
#endif
 		{
			SetupDefaultParmblk(&parms, &codecBlock, NULL);	
			parms.spc.loadFuncTbl.dispatchTbl = codecBlock.dispTable;
			parms.spc.loadFuncTbl.tableSize = NUM_FUNCTION_CODES;
		
		
		
			for(n = 0; n < NUM_FUNCTION_CODES; n++)
			{
#if defined(PORTKEY_OS_MACSYS) && PORTKEY_CPU_PPC
				if(type == kOMFRegisterPlugin)
				{
					parms.spc.loadFuncTbl.dispatchTbl[n] = (omfCodecOptDispPtr_t)
							NewRoutineDescriptor((ProcPtr)codecDefaultFunction,
							uppCodecProcInfo, GetCurrentArchitecture());
				}
				else
#endif
					parms.spc.loadFuncTbl.dispatchTbl[n] = codecDefaultFunction;
			}

			CHECK(CallBaseCodec(&codecBlock,kCodecLoadFuncPointers, &parms));
		}
 		if(info.mdesClassID != NULL)
		  {
			CHECK(omfsTableAddValueBlock(session->codecMDES, info.mdesClassID, sizeof(omfClassID_t),
  									&codecBlock, sizeof(codecBlock), kOmTableDupAddDup));
		  }
		CHECK(omfsTableAddStringBlock(session->codecID, info.codecID, &codecBlock, sizeof(codecBlock), kOmTableDupError));

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
/*
	codecInit allows the codec to dynamically set up custom properties, types, etc...
	should be the FIRST function applied to the codec!  See omfmNewFileMob in omMedObj.c
	If the media handle has been allocated, init will not do anything.  
	
	TBD: how to detect multiple inits with different media handles, same file handle in 
	a thread-safe way?
*/
static void SetupDefaultParmblk(
			omfCodecParms_t	*parms,
			codecTable_t	*codecPtr,
			omfMediaHdl_t	media)
{
	if(codecPtr != NULL)
		parms->pers = codecPtr->persist;
	else
		parms->pers = NULL;
#if ACCEPT_PLUGINS
	parms->dispatchPtr = xxxxx;	/* Fill this in later */
#endif
#ifdef OMF_CODEC_COMPAT
	parms->media = media;
#endif
	if(media != NULL)
		parms->fileRev = media->mainFile->setrev;
	else
		parms->fileRev = kOmfRev2x;
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
omfErr_t codecInit(
			omfSessionHdl_t		session,
			codecTable_t		*codecPtr,
			void				**persistentData)
{
	static omfCodecParms_t parms;
	
	SetupDefaultParmblk(&parms, NULL, NULL);
	parms.spc.init.session = session;
	CallBaseCodec(codecPtr,kCodecInit, &parms);
	*persistentData = parms.spc.init.persistRtn;

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
void *codecGetPersistentData(
			omfSessionHdl_t	sess,
			omfCodecID_t	id,
			omfErr_t		*errPtr)
{
	void			*result;
	codecTable_t	codec_data;
	omfBool			found;
	
	XPROTECT(NULL)
	{
		CHECK(omfsTableLookupBlock(sess->codecID, id, sizeof(codec_data), &codec_data, &found));
		if(found)
			result = codec_data.persist;
		else
			RAISE(OM_ERR_CODEC_INVALID);
	}
	XEXCEPT
	{
		*errPtr = XCODE();
	}
	XEND_SPECIAL(NULL);
	
	return(result);
}

/************************************************************************
 *
 * CODEC Basic Information Functions
 *
 * These functions to not require an open media handle, and are used prior
 * to opening the media handle.
 *
 ************************************************************************/

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
omfErr_t codecGetNumChannels(
			omfHdl_t main,
			omfObject_t fileMob,
			omfDDefObj_t  mediaKind,
			omfInt16 * numCh)
{
	omfCodecParms_t parms;
	codecTable_t	codecInfo;
	omfObject_t		mdes;
	
	omfAssertValidFHdl(main);
	XPROTECT(main)
	{
		CHECK(omfmMobGetMediaDescription(main, fileMob, &mdes));

		CHECK(omfmFindCodecForMedia(main, mdes, &codecInfo));
		XASSERT(correctCodevRev(codecInfo.rev), OM_ERR_BAD_CODEC_REV);
	
		SetupDefaultParmblk(&parms, &codecInfo, NULL);
		parms.fileRev = main->setrev;
		parms.spc.getChannels.file = main;
		parms.spc.getChannels.mdes = mdes;
		parms.spc.getChannels.mediaKind = mediaKind;
		parms.spc.getChannels.numCh = 0;
#ifdef OMF_CODEC_COMPAT
		if(codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&codecInfo,kCodecGetNumChannels, &parms, NULL, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&codecInfo,kCodecGetNumChannels, &parms));
		}
#endif
		*numCh = parms.spc.getChannels.numCh;
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
omfErr_t codecInitMDESProps(
			omfHdl_t main,
			omfObject_t mdes,
			omfCodecID_t codecID,
			char *variant)
{
	omfCodecParms_t parms;
	codecTable_t	codecInfo;
	omfBool			found;
	omfErr_t		status;

	omfAssertValidFHdl(main);
	XPROTECT(main)
	{
		CHECK(omfsTableLookupBlock(main->session->codecID, codecID, sizeof(codecInfo), &codecInfo, &found));
		if(!found)
			RAISE(OM_ERR_CODEC_INVALID);
			
		XASSERT(correctCodevRev(codecInfo.rev), OM_ERR_BAD_CODEC_REV);
	
		SetupDefaultParmblk(&parms, &codecInfo, NULL);
		parms.fileRev = main->setrev;
		parms.spc.initMDES.file = main;
		parms.spc.initMDES.mdes = mdes;
		parms.spc.initMDES.variant = variant;
#ifdef OMF_CODEC_COMPAT
		if(codecInfo.rev >= CODEC_REV_3)
#endif
		{
			status = CallExtendCodec(&codecInfo,kCodecInitMDESProps, &parms, NULL, main);
			if(status != OM_ERR_NONE && status != OM_ERR_INVALID_OP_CODEC)
				RAISE(status);
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&codecInfo,kCodecInitMDESProps, &parms));
		}
#endif
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}


/************************************************************************
 *
 * CODEC Media Functions
 *
 * These functions create, close, or use a media handle.
 *
 ************************************************************************/

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
omfErr_t codecOpen(
			omfMediaHdl_t media)
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);

	omfAssert(correctCodevRev(media->pvt->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecOpen, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecOpen, &parms));
		}
#endif
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
omfErr_t codecCreate(
			omfMediaHdl_t media)
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);
	
	omfAssert(correctCodevRev(media->pvt->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecCreate, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecCreate, &parms));
		}
#endif
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
omfErr_t codecGetInfo(
			omfMediaHdl_t	media,
			omfInfoType_t	infoType,
			omfDDefObj_t	mediaKind, 
			omfInt32			infoBufSize,
			void			*info)
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);

	omfAssert(correctCodevRev(media->pvt->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
		parms.spc.mediaInfo.buf = info;
		parms.spc.mediaInfo.bufLen = infoBufSize;
		parms.spc.mediaInfo.infoType = infoType;
		parms.spc.mediaInfo.mediaKind = mediaKind;
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecGetInfo, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecGetInfo, &parms));
		}
#endif
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
omfErr_t codecPutInfo(
			omfMediaHdl_t	media,
			omfInfoType_t	infoType,
			omfDDefObj_t	mediaKind, 
			omfInt32		infoBufSize,
			void			*info)
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);

	omfAssert(correctCodevRev(media->pvt->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
		parms.spc.mediaInfo.buf = info;
		parms.spc.mediaInfo.bufLen = infoBufSize;
		parms.spc.mediaInfo.infoType = infoType;
		parms.spc.mediaInfo.mediaKind = mediaKind;
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecPutInfo, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecPutInfo, &parms));
		}
#endif
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
omfErr_t codecGetAudioInfo(
			omfMediaHdl_t	media,
			omfAudioMemOp_t *ops)
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);

	XPROTECT(media->mainFile)
	{
		CHECK(codecGetInfo(media, kAudioInfo, media->soundKind, sizeof(omfAudioMemOp_t *), &ops));
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
omfErr_t codecPutAudioInfo(
			omfMediaHdl_t	media,
			omfAudioMemOp_t *ops)
{
	omfInt32		n;
	omfSubChannel_t	*oldChannel, *src;
	omfAudioMemOp_t	*ptr;
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);

	XPROTECT(media->mainFile)
	{
		CHECK(codecPutInfo(media, kAudioInfo, media->soundKind, sizeof(omfAudioMemOp_t *), &ops));
		oldChannel = media->channels;
		
		for(ptr = ops; ptr->opcode != kOmfAFmtEnd; ptr++)
		{
			if(ptr->opcode == kOmfNumChannels)
			{
				media->channels = (omfSubChannel_t *)
						omOptMalloc(media->mainFile, sizeof(omfSubChannel_t) * ptr->operand.numChannels);
				XASSERT((media->channels != NULL), OM_ERR_NOMEMORY);
		
				src = oldChannel;
				for(n = 0; n < ptr->operand.numChannels; n++, src++)
				{
					media->channels[n].numSamples = src->numSamples;
					media->channels[n].dataOffset = src->dataOffset;
					media->channels[n].mediaKind = src->mediaKind;
					media->channels[n].trackID = n+1;	/* !!! Ties trackID to physical chan */
					media->channels[n].physicalOutChan = n+1;
					media->channels[n].sampleRate = src->sampleRate;
				}
			}
		}
		if(media->channels != oldChannel)
			omOptFree(media->mainFile, oldChannel);
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
omfErr_t codecGetVideoInfo(
			omfMediaHdl_t		media,
			omfVideoMemOp_t 	*ops)
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);

	XPROTECT(media->mainFile)
	{
		CHECK(codecGetInfo(media, kVideoInfo, media->pictureKind,
									sizeof(omfVideoMemOp_t), ops));
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
omfErr_t codecPutVideoInfo(
			omfMediaHdl_t		media,
			omfVideoMemOp_t 	*ops)
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);

	XPROTECT(media->mainFile)
	{
		CHECK(codecPutInfo(media, kVideoInfo, media->pictureKind,
							sizeof(omfVideoMemOp_t), ops));
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
omfErr_t codecClose(
			omfMediaHdl_t media)
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);

	omfAssert(correctCodevRev(media->pvt->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecClose, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecClose, &parms));
		}
#endif
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
omfErr_t codecWriteBlocks(
			omfMediaHdl_t	media,
			omfDeinterleave_t inter,
			omfInt16			xferBlockCount,
			omfmMultiXfer_t	*xferBlock)
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);

	omfAssert(correctCodevRev(media->pvt->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
		parms.spc.mediaXfer.xfer = xferBlock;
		parms.spc.mediaXfer.numXfers = xferBlockCount;
		parms.spc.mediaXfer.inter = inter;
	
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecWriteSamples, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecWriteSamples, &parms));
		}
#endif
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
omfErr_t codecReadBlocks(
			omfMediaHdl_t	media,
			omfDeinterleave_t inter,
			omfInt16			xferBlockCount,
			omfmMultiXfer_t *xferBlock)
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);

	omfAssert(correctCodevRev(media->pvt->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
		parms.spc.mediaXfer.xfer = xferBlock;
		parms.spc.mediaXfer.numXfers = xferBlockCount;
		parms.spc.mediaXfer.inter = inter;
		
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecReadSamples, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecReadSamples, &parms));
		}
#endif
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
omfErr_t codecWriteLines(
			omfMediaHdl_t	media,	/* IN -- */
			omfInt32			nLines,	/* IN -- */
			void			*buffer,	/* IN -- */
			omfInt32			*bytesWritten)	/* OUT -- */
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);

	omfAssert(correctCodevRev(media->pvt->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
		parms.spc.mediaLinesXfer.buf = buffer;
		parms.spc.mediaLinesXfer.numLines = nLines;
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecWriteLines, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecWriteLines, &parms));
		}
#endif
	
		*bytesWritten = parms.spc.mediaLinesXfer.bytesXfered;
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
omfErr_t codecReadLines(
			omfMediaHdl_t	media,	/* IN -- */
			omfInt32			nLines,	/* IN -- */
			omfInt32			bufLen,	/* IN -- */
			void			*buffer,	/* IN/OUT -- */
			omfInt32			*bytesRead)	/* OUT -- */
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);

	omfAssert(correctCodevRev(media->pvt->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
		parms.spc.mediaLinesXfer.buf = buffer;
		parms.spc.mediaLinesXfer.bufLen = bufLen;
		parms.spc.mediaLinesXfer.numLines = nLines;
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecReadLines, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecReadLines, &parms));
		}
#endif
	
		*bytesRead = parms.spc.mediaLinesXfer.bytesXfered;
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
omfErr_t codecSetFrameNum(
			omfMediaHdl_t	media,	/* IN -- */
			omfInt64			frameNum)	/* IN -- */
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);
	XPROTECT(main)
	{
		if (media->openType == kOmfiCreated)
			RAISE(OM_ERR_MEDIA_OPENMODE);
	
		omfAssert(correctCodevRev(media->pvt->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);
	
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
		parms.spc.setFrame.frameNumber = frameNum;
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecSetFrame, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecSetFrame, &parms));
		}
#endif
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * codecGetFrameOffset
 *
 * 		Returns the offset of the given frame from the start of the data.
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
omfErr_t codecGetFrameOffset(
			omfMediaHdl_t	media,	/* IN -- */
			omfInt64		frameNum,	/* IN -- */
			omfInt64		*frameOffset ) /* OUT -- */
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);
	XPROTECT(main)
	{
		if (media->openType == kOmfiCreated)
			RAISE(OM_ERR_MEDIA_OPENMODE);
	
		omfAssert(correctCodevRev(media->pvt->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);
	
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
		parms.spc.getFrameOffset.frameNumber = frameNum;
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecGetFrameOffset, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecGetFrameOffset, &parms));
		}
#endif
		*frameOffset = parms.spc.getFrameOffset.frameOffset;
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
omfErr_t codecGetMetaInfo(
			omfSessionHdl_t		session,
			codecTable_t		*codecPtr,
			char				*variant,
			char				*nameBuf,
			omfInt32				nameBufLen,
			omfCodecMetaInfo_t	*info)
{
	omfCodecParms_t	parms;
	char			buf[32];
	
	if(nameBuf == NULL)
	{
		nameBuf = buf;
		nameBufLen = 0;		/* Stave off bus errors & scribbles */
	}
	XPROTECT(NULL)
	{
		if(codecPtr == NULL)
			RAISE(OM_ERR_CODEC_INVALID);

		SetupDefaultParmblk(&parms, codecPtr, NULL);	
		parms.spc.metaInfo.variant = variant;
		parms.spc.metaInfo.name = nameBuf;
		parms.spc.metaInfo.nameLen = nameBufLen;
		CHECK(CallBaseCodec(codecPtr,kCodecGetMetaInfo, &parms));
	}
	XEXCEPT
	XEND
	
	*info = parms.spc.metaInfo.info;
	
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
omfErr_t codecGetSelectInfo(
			omfHdl_t				file,
			codecTable_t			*codecPtr,
			omfObject_t				mdes,
			omfCodecSelectInfo_t	*info)
{
	omfCodecParms_t	parms;

	omfAssertValidFHdl(file);

	omfAssert((codecPtr != NULL), file, OM_ERR_CODEC_INVALID);

	XPROTECT(file)
	{
		SetupDefaultParmblk(&parms, codecPtr, NULL);	
		parms.fileRev = file->setrev;
		parms.spc.selectInfo.file = file;
		parms.spc.selectInfo.mdes = mdes;
#ifdef OMF_CODEC_COMPAT
		if(codecPtr->rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(codecPtr,kCodecGetSelectInfo, &parms, NULL, file));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(codecPtr,kCodecGetSelectInfo, &parms));
		}
#endif
		*info = parms.spc.selectInfo.info;
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
omfErr_t codecImportRaw(omfHdl_t main, omfObject_t fileMob, omfHdl_t rawFile)
{
	omfCodecParms_t	parms;
	codecTable_t	codecInfo;
	omfBool			found;
	omfClassID_t	classID;
	omfObject_t		mdes = NULL;
	omfProperty_t	idProp;
	
	omfAssertValidFHdl(main);

	XPROTECT(main)
	{
		if ((main->setrev == kOmfRev1x) || (main->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		CHECK(omfmMobGetMediaDescription(main, fileMob, &mdes));
		CHECK(omfsReadClassID(main, mdes, idProp, classID));
		/* Release Bento reference, so the useCount is decremented */
		if (mdes)
		  CMReleaseObject((CMObject)mdes);	
		CHECK(omfsTableClassIDLookup(main->session->codecMDES, classID, sizeof(codecInfo), &codecInfo, &found));
		XASSERT(found, OM_ERR_CODEC_INVALID);
		XASSERT(correctCodevRev(codecInfo.rev), OM_ERR_BAD_CODEC_REV);

		SetupDefaultParmblk(&parms, &codecInfo, NULL);
		parms.fileRev = main->setrev;
		parms.spc.rawImportInfo.main = main;
		parms.spc.rawImportInfo.fileMob = fileMob;
		parms.spc.rawImportInfo.rawFile = rawFile;
#ifdef OMF_CODEC_COMPAT
		if(codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&codecInfo,kCodecImportRaw, &parms, NULL, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&codecInfo,kCodecImportRaw, &parms));
		}
#endif
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * codecGetNumCodecs
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
omfErr_t codecGetNumCodecs(
			omfSessionHdl_t sess,
			omfString		dataKindString,
			omfFileRev_t	rev,
			omfInt32		*numCodecPtr)
{
	omTableIterate_t	iter;
	omfInt32			numCodecs = 0;
	omfBool				found;
	codecTable_t		*tablePtr, tableRef;
	
	XPROTECT(NULL)
	{
		XASSERT(numCodecPtr != NULL, OM_ERR_NULL_PARAM);
		*numCodecPtr = 0;
		
		CHECK(omfsTableFirstEntry(sess->codecID, &iter, &found));
		while(found)
		{
			tablePtr = (codecTable_t *)iter.valuePtr;
			memcpy(&tableRef, tablePtr, sizeof(codecTable_t));
			if((rev >= tableRef.minFileRev) &&
				((!tableRef.maxFileRevIsValid) || (rev <= tableRef.maxFileRev)) &&
				(strstr(tableRef.dataKindNameList, dataKindString) != NULL))
			{
				numCodecs++;
			}
			CHECK(omfsTableNextEntry(&iter, &found));
		}
		*numCodecPtr = numCodecs;
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * codecGetIndexedCodecID
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		Returns a pointer to static data within the codec.  Should make a copy
 *		of this at the external API level.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t codecGetIndexedCodecID(
			omfSessionHdl_t sess,
			omfString		dataKindString,
			omfFileRev_t	rev,
			omfInt32		index,
			char			**codecIDPtr)
{
	omTableIterate_t	iter;
	omfInt32			count = 1;
	omfBool				found;
	codecTable_t		*tablePtr, tableRef;
	
	XPROTECT(NULL)
	{
		XASSERT(codecIDPtr != NULL, OM_ERR_NULL_PARAM);
		*codecIDPtr = NULL;
		
		CHECK(omfsTableFirstEntry(sess->codecID, &iter, &found));
		while(found)
		{
			tablePtr = (codecTable_t *)iter.valuePtr;
			memcpy(&tableRef, tablePtr, sizeof(codecTable_t));
			if((rev >= tableRef.minFileRev) &&
				((!tableRef.maxFileRevIsValid) || (rev <= tableRef.maxFileRev)) &&
				(strstr(tableRef.dataKindNameList, dataKindString) != NULL))
			{
				if(count == index)
				{
					*codecIDPtr = (char *)iter.key;
					break;
				}
				count++;
			}
			CHECK(omfsTableNextEntry(&iter, &found));
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * codecGetNumCodecs
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
omfErr_t codecGetNumCodecVarieties(
			omfSessionHdl_t sess,
			omfCodecID_t	codecID,
			omfFileRev_t    setrev,
			char			*mediaKindString,
			omfInt32		*varietyCountPtr)
{
	omfInt32			numCodecs = 0;
	omfBool				found;
	codecTable_t		codecInfo;
	omfCodecParms_t 	parms;
	omfErr_t			status;
		
	XPROTECT(NULL)
	{
		CHECK(omfsTableClassIDLookup(sess->codecID, codecID, sizeof(codecInfo), &codecInfo, &found));
		XASSERT(found, OM_ERR_CODEC_INVALID);
		XASSERT(correctCodevRev(codecInfo.rev), OM_ERR_BAD_CODEC_REV);
		XASSERT(varietyCountPtr != NULL, OM_ERR_NULL_PARAM);

		SetupDefaultParmblk(&parms, &codecInfo, NULL);
		parms.fileRev = setrev;
		parms.spc.varietyCount.mediaKindString = mediaKindString;
#ifdef OMF_CODEC_COMPAT
		if(codecInfo.rev >= CODEC_REV_3)
#endif
		{
			status = CallExtendCodec(&codecInfo,kCodecGetVarietyCount, &parms, NULL, NULL);
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			status = CallBaseCodec(&codecInfo,kCodecGetVarietyCount, &parms);
		}
#endif
		if(status == OM_ERR_INVALID_OP_CODEC)
			*varietyCountPtr = 0;
		else if(status == OM_ERR_NONE)
			*varietyCountPtr = parms.spc.varietyCount.count;
		else
		{
			CHECK(status);
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * codecGetIndexedCodecVariety
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		Returns a pointer to static data within the codec.  Should make a copy
 *		of this at the external API level.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t codecGetIndexedCodecVariety(
			omfSessionHdl_t sess,
			omfCodecID_t	codecID,
			omfFileRev_t    setrev,
			char			*mediaKindString,
			omfInt32		index,
			char			**varietyPtr)
{
	omfInt32			numCodecs = 0;
	omfBool				found;
	codecTable_t		codecInfo;
	omfCodecParms_t 	parms;
		
	XPROTECT(NULL)
	{
		CHECK(omfsTableClassIDLookup(sess->codecID, codecID, sizeof(codecInfo), &codecInfo, &found));
		XASSERT(found, OM_ERR_CODEC_INVALID);
		XASSERT(correctCodevRev(codecInfo.rev), OM_ERR_BAD_CODEC_REV);
		XASSERT(varietyPtr != NULL, OM_ERR_NULL_PARAM);

		SetupDefaultParmblk(&parms, &codecInfo, NULL);
		parms.fileRev = setrev;
		parms.spc.varietyInfo.mediaKindString = mediaKindString;
		parms.spc.varietyInfo.index = index;

#ifdef OMF_CODEC_COMPAT
		if(codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&codecInfo,kCodecGetVarietyInfo, &parms, NULL, NULL));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&codecInfo,kCodecGetVarietyInfo, &parms));
		}
#endif
		*varietyPtr = parms.spc.varietyInfo.varietyName;
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
omfErr_t codecSemanticCheck(
			omfHdl_t	main,
			omfObject_t	fileMob,
			omfCheckVerbose_t verbose,
			omfCheckWarnings_t warn, 
			omfInt32	msgBufSize,
			char		*msgBuf)	/* OUT - Which property (if any) failed */
{
	omfCodecParms_t parms;
	codecTable_t	codecInfo;
	omfErr_t		status;
	omfObject_t		mdes;
	omfUID_t		fileMobUID;
	omfCodecMetaInfo_t	meta;
	char			name[64], classID[5];
	omfProperty_t	idProp;
	
	omfAssertValidFHdl(main);

	XPROTECT(main)
	{
		XASSERT(msgBuf != NULL, OM_ERR_NULL_PARAM);
		CHECK(omfmMobGetMediaDescription(main, fileMob, &mdes));

		CHECK(omfmFindCodecForMedia(main, mdes, &codecInfo));
		XASSERT(correctCodevRev(codecInfo.rev), OM_ERR_BAD_CODEC_REV);
	
		SetupDefaultParmblk(&parms, &codecInfo, NULL);
		parms.fileRev = main->setrev;

		msgBuf[0] = '\0';
		parms.spc.semCheck.file = main;
		parms.spc.semCheck.mdes = mdes;
		parms.spc.semCheck.verbose = verbose;
		parms.spc.semCheck.warn = warn;
		parms.spc.semCheck.message = NULL;

		/* Only semantic check the dataObj on local media
		 * If the media is NOT local, then NULL will be passed in as the data object
		 * pointer.
		 */
		CHECK(omfsReadUID(main, fileMob, OMMOBJMobID, &fileMobUID));
		parms.spc.semCheck.dataObj = (omfObject_t)omfsTableUIDLookupPtr(main->dataObjs,
																	fileMobUID);

		CHECK(codecGetMetaInfo(main->session, &codecInfo, NULL, name, sizeof(name), &meta));
		if(parms.spc.semCheck.dataObj != NULL)
		{
			if ((main->setrev == kOmfRev1x) ||(main->setrev == kOmfRevIMA))
			{
				idProp = OMObjID;
			}
			else /* kOmfRev2x */
			{
				idProp = OMOOBJObjClass;
			}
			CHECK(omfsReadClassID(main, parms.spc.semCheck.dataObj, idProp, classID));
			if(strncmp(classID, meta.dataClassID, 4) != 0)
			{
				printf("MISMATCHED CLASS ID IN DATA OBJECT\n");
			}
		}

#ifdef OMF_CODEC_COMPAT
		if(codecInfo.rev >= CODEC_REV_3)
#endif
		{
			status = CallExtendCodec(&codecInfo,kCodecSemanticCheck, &parms, NULL, main);
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			status = CallBaseCodec(&codecInfo,kCodecSemanticCheck, &parms);
		}
#endif
		if(parms.spc.semCheck.message != NULL)
		{
			strncpy(msgBuf, parms.spc.semCheck.message, msgBufSize);
			msgBuf[msgBufSize-1] = '\0';
		}
		if((status != OM_ERR_NONE) && (status != OM_ERR_INVALID_OP_CODEC))
			RAISE(status);
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
omfErr_t codecAddFrameIndexEntry(
			omfMediaHdl_t	media,			/* IN -- */
			omfInt64		frameOffset)	/* IN -- */
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);
	XPROTECT(main)
	{
		if (media->openType == kOmfiOpened)
			RAISE(OM_ERR_MEDIA_OPENMODE);
	
		XASSERT(correctCodevRev(media->pvt->codecInfo.rev), OM_ERR_BAD_CODEC_REV);
	
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
		parms.spc.addIndex.frameOffset = frameOffset;
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(&(media->pvt->codecInfo),kCodecAddFrameIndexEntry, &parms, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(&(media->pvt->codecInfo),kCodecAddFrameIndexEntry, &parms));
		}
#endif
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

omfErr_t codecPassThrough(
			codecTable_t		*subCodec,	/* IN -- */
			omfCodecFunctions_t	func,
			omfCodecParms_t 	*parmblk,
			omfMediaHdl_t 		media,
			omfHdl_t			main,
			void				*clientPData)
{
	void			*savePdata;
	codecTable_t   	saveCodecInfo;
	
	omfAssertValidFHdl(main);

	omfAssert(correctCodevRev(subCodec->rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		savePdata = media ? media->userData : NULL;
		if( media )
			saveCodecInfo = media->pvt->codecInfo;
		if(clientPData != NULL)
			if( media )
		{
				media->userData = clientPData;	/* Special-case the create case */
				media->pvt->codecInfo = *subCodec;
		}
/*!!!		parmblk->dispatchPtr = subCodec->dispTable;	*/
		parmblk->pers = subCodec->persist;
#ifdef OMF_CODEC_COMPAT
		if(subCodec->rev >= CODEC_REV_3)
#endif
		{
			CHECK(CallExtendCodec(subCodec,func, parmblk, media, main));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			CHECK(CallBaseCodec(subCodec,func, parmblk));
		}
#endif
		if(clientPData != NULL && media != NULL )
		{
			media->userData = savePdata;
			media->pvt->codecInfo = saveCodecInfo;
		}
	}
	XEXCEPT
	{
		if(clientPData != NULL && media !=	NULL )
		{
			media->userData = savePdata;
			media->pvt->codecInfo = saveCodecInfo;
		}
	}
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
omfErr_t codecRunDiagnostics(
			omfMediaHdl_t	media,
			FILE			*logFile)
{
	omfHdl_t        main;
	omfCodecParms_t parms;

	omfAssertMediaHdl(media);
	main = media->mainFile;

	omfAssertValidFHdl(main);
	omfAssert(correctCodevRev(media->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		SetupDefaultParmblk(&parms, &media->pvt->codecInfo, media);
		parms.spc.diag.mdes = media->mdes;
		parms.spc.diag.logFile = logFile;
#ifdef OMF_CODEC_COMPAT
		if(media->pvt->codecInfo.rev >= CODEC_REV_3)
#endif
		{
			status = CallExtendCodec(&media->pvt->codecInfo,kCodecRunDiagnostics, &parms));
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			status = CallBaseCodec(&media->pvt->codecInfo,kCodecRunDiagnostics, &parms));
		}
#endif
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

#endif

/* INDENT OFF */
/* TEMPLATE CALL
omfErr_t codecClose(omfMediaHdl_t media)
{
	omfHdl_t		main;
	omfCodecParms_t	parms;
	codecTable_t	*codecPtr;

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);

	codecPtr = &media->pvt->codecInfo;
	omfAssert(correctCodevRev(media->codecInfo.rev), main, OM_ERR_BAD_CODEC_REV);

	XPROTECT(main)
	{
		SetupDefaultParmblk(&parms, codecPtr, media);
		***Set up other input parms***
		
#ifdef OMF_CODEC_COMPAT
		if(codecPtr->rev >= CODEC_REV_3)
#endif
		{
			status = CallExtendCodec(&codecInfo,op, &parms);
		}
#ifdef OMF_CODEC_COMPAT
		else
		{
			status = CallBaseCodec(&codecInfo,op, &parms);
		}
#endif
		***Retrieve output parms***
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}
 */
 
void *omfmGetRawFileDescriptor(omfHdl_t file)
{
	return(file->rawFileDesc);
}



/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
