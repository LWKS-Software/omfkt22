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
 * Name: omMedia.c
 *
 * Function: The main API file for media layer operations.
 *
 * Audience: Clients writing or reading OMFI media data.
 *
 * General error codes returned:
 * 		OM_ERR_BAD_FHDL
 *				media referenced a bad omfHdl_t.
 * 		OM_ERR_BENTO_PROBLEM
 *				Bento returned an error, check BentoErrorNumber.
 */

#include "masterhd.h"
#include <string.h>
#include <stdio.h>

#include "omPublic.h"
#include "omMedia.h" 
#include "omPvt.h"
#include "omLocate.h"
#include "omJPEG.h" 

static omfErr_t omfsReconcileMobLength(omfHdl_t file, omfObject_t mob);
static omfErr_t InitMediaHandle(omfHdl_t file, omfMediaHdl_t media,
						omfObject_t fileMob);
static omfErr_t FindTrackByID(omfHdl_t file, omfObject_t masterMob,
										omfTrackID_t trackID,
										omfMSlotObj_t	*trackRtn);

#define MAX_DEF_AUDIO	8
/* A 75-column ruler
         111111111122222222223333333333444444444455555555556666666666777777
123456789012345678901234567890123456789012345678901234567890123456789012345
*/

omfErr_t PerFileMediaInit(omfHdl_t file);
omfErr_t DisposeCodecPersist(omfSessionHdl_t sess);

/************************************************************************
 *
 * Initialize the media subsystem.
 *
 ************************************************************************/


/************************
 * Function: omfmInit
 *
 * 	Initialized data for the media layer.  Most importantly, initializes
 *		the codecs for the required formats, and sets up two callbacks which
 *		handle per-file-open, and per-session global data.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BAD_SESSION - Session handle was NULL or invalid.
 */
omfErr_t        omfmInit(omfSessionHdl_t sess)
{
	if ((sess == NULL) || (sess->cookie != SESSION_COOKIE))
		return (OM_ERR_BAD_SESSION);

	XPROTECT(NULL)
	{
		CHECK(omfsJPEGInit());
		CHECK(codecMgrInit(sess));
		sess->mediaLayerInitComplete = TRUE;
		sess->openCB = PerFileMediaInit;
		sess->closeSessCB = DisposeCodecPersist;
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************************************************************
 *
 * Functions to write media data.
 *
 ************************************************************************/

/************************
 * Function: omfmAudioMediaCreate
 *
 * 		Creates either a single stream of audio media, or interleaved
 *		audio-only data.  A separate call (omfmMediaMultiCreate) exists 
 *		in order to create interleaved audio and video data.
 *
 *		The media handle from this call can be used with
 *		omfmWriteDataSamples or omfmWriteMultiSamples but NOT with 
 *		or omfmWriteDataLines.
 *
 * Argument Notes:
 *		If you are creating the media, and then attaching it to a master
 *		mob, then the "masterMob" field may be left NULL.
 *
 *		The numChannels field refers to the number of interleaved
 *		channels on a single data stream.  
 *
 *		The sample rate should be the actual samples per second, not the
 *		edit rate.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmAudioMediaCreate(
		omfHdl_t			file,			/* IN -- In this file */
		omfObject_t		masterMob,	/* IN -- on this master mob */
		omfTrackID_t	masterTrackID,
		omfObject_t		fileMob,		/* IN -- & this file mob, create audio */
		omfRational_t	samplerate,	/* IN -- with this sample rate */
		omfCompressEnable_t enable,	/* IN -- optionally compressing it */
		omfInt16				sampleSize,	/* IN -- with this sample size  */
		omfInt16				numChannels,/* IN -- and this many channels. */
		omfMediaHdl_t	*resultPtr)	/* OUT -- Return the object here. */
{
	omfMediaHdl_t		result;
	omfAudioMemOp_t	opList[4];
	omfmMultiCreate_t	init[MAX_DEF_AUDIO];
	omfInt16				n;
	omfUID_t				uid;
	mobTableEntry_t	*mobPtr;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	omfAssert(resultPtr != NULL, file, OM_ERR_NULL_PARAM);
	if(file->fmt == kOmfiMedia)
	{
		omfAssert(fileMob != NULL, file, OM_ERR_INVALID_FILE_MOB);
	}
	
	XPROTECT(file)
	{
		if(numChannels == 1)
		{
			CHECK(omfmMediaCreate(file, masterMob, masterTrackID, fileMob, 
										 file->soundKind,
										 samplerate, enable, &result));

			opList[0].opcode = kOmfSampleSize;
			opList[0].operand.sampleSize = sampleSize;
			opList[1].opcode = kOmfNumChannels;
			opList[1].operand.numChannels = numChannels;
			opList[2].opcode = kOmfSampleRate;
			opList[2].operand.sampleRate = samplerate;
			opList[3].opcode = kOmfAFmtEnd;
			CHECK(codecPutAudioInfo(result, opList));
		}
		else if(numChannels <= MAX_DEF_AUDIO)
		{
		  CHECK(omfsReadUID(file, fileMob, OMMOBJMobID, &uid));
			mobPtr = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, uid);
			if(mobPtr == NULL)
		  	{
				RAISE(OM_ERR_MISSING_MOBID);
		  	}

			for(n = 0; n < numChannels; n++)
			{
				init[n].mediaKind = file->soundKind;
				init[n].subTrackNum = n+1;	/* Physical Output Channel */
				init[n].trackID = n+1;
				init[n].sampleRate = samplerate;
			}
			CHECK(omfmMediaMultiCreate(file, masterMob, fileMob, numChannels,
										init, mobPtr->createEditRate, enable, &result));
			opList[0].opcode = kOmfSampleSize;
			opList[0].operand.sampleSize = sampleSize;
			opList[1].opcode = kOmfNumChannels;
			opList[1].operand.numChannels = numChannels;
			opList[2].opcode = kOmfSampleRate;
			opList[2].operand.sampleRate = samplerate;
			opList[3].opcode = kOmfAFmtEnd;
			CHECK(omfmPutAudioInfoArray(result, opList));
		}
		else
			RAISE(OM_ERR_USE_MULTI_CREATE);
	}
	XEXCEPT
	{
		*resultPtr = NULL;
	}
	XEND
	
	*resultPtr = result;
	return (OM_ERR_NONE);
}

/************************
 * Function: omfmVideoMediaCreate
 *
 * 		Creates a single stream of video media.  A separate call
 *		(omfmMediaMultiCreate) exists to create interleaved audio and
 *		video data.
 *
 *		The media handle from this call can be used with
 *		omfmWriteDataSamples or omfmWriteDataLines, but NOT with
 *		omfmWriteMultiSamples.
 *
 * Argument Notes:
 *		If you are creating the media, and then attaching it to a master
 *		mob, then the "masterMob" field may be left NULL.
 *
 *		The storedHeight and storedWidth are the dimensions of the frame
 *		as stored on disk (or as it should be restored by the codec.  The
 *		displayRect and sampledRect are set to:
 *			(0,0 @ sampledWidth, sampledHeight).
 *		If the displayed rectangle is not the same as the stored rectangle
 *		(as with the old leadingLines and trailingLines), then you should
 *		call omfmSetDisplayRect().
 *
 *		The frame layout contains the number of fields and whether they are
 *		interlaced, but does not specify field dominance.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmVideoMediaCreate(
			omfHdl_t			file,			/* IN -- In this file */
			omfObject_t		masterMob,	/* IN -- on this master mob */
			omfTrackID_t	masterTrackID,
			omfObject_t		fileMob,		/* IN -- & this file mob, create video */
			omfCompressEnable_t enable,	/* IN -- optionally compressing it */
			omfRational_t	editrate,
			omfUInt32			StoredHeight,/*IN -- with this height */
			omfUInt32			StoredWidth,/* IN -- and this width */
			omfFrameLayout_t layout,	/* IN -- and this frame layout. */
			omfRational_t	iRatio,		/* IN -- image aspect ratio */
			omfMediaHdl_t	*resultPtr)	/* OUT -- Return the object here. */
{
	omfMediaHdl_t   	result;
	omfVideoMemOp_t	opList[4], op;

	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	omfAssert(resultPtr != NULL, file, OM_ERR_NULL_PARAM);
	if(file->fmt == kOmfiMedia)
	{
		omfAssert(fileMob != NULL, file, OM_ERR_INVALID_FILE_MOB);
	}
	XPROTECT(file)
	{
		
		CHECK(omfmMediaCreate(file, masterMob, masterTrackID, fileMob, file->pictureKind,
					editrate, enable, &result));
				
		CHECK(omfmVideoOpInit(file, opList));
			
		op.opcode = kOmfFrameLayout;
		op.operand.expFrameLayout = layout;
		CHECK(omfmVideoOpAppend(file, kOmfAppendIfAbsent, op, opList, 4L));
		
		op.opcode = kOmfStoredRect;
		op.operand.expRect.xSize = StoredWidth;
		op.operand.expRect.ySize = StoredHeight;
		op.operand.expRect.xOffset = op.operand.expRect.yOffset = 0;
		CHECK(omfmVideoOpAppend(file, kOmfAppendIfAbsent, op, opList, 4L));
		
		op.opcode = kOmfAspectRatio;
		op.operand.expRational = iRatio;
		CHECK(omfmVideoOpAppend(file, kOmfAppendIfAbsent, op, opList, 4L));
		
		CHECK(codecPutVideoInfo(result, opList));
	}
	XEXCEPT
	{
		*resultPtr = NULL;
	}
	XEND
	
	*resultPtr = result;
	return (OM_ERR_NONE);
}

/************************
 * Function: omfmMediaCreate
 *
 * 		Creates a single channel stream of media.  Convenience functions
 *		exist to create audio or video media, and a separate call
 *		(omfmMediaMultiCreate) exists to create interleaved audio and
 *		video data.
 *
 *		The media handle from this call can be used with
 *		omfmWriteDataSamples  and possibly omfmWriteDataLines, but NOT with
 *		omfmWriteMultiSamples.
 *
 * Argument Notes:
 *		If you are creating the media, and then attaching it to a master
 *		mob, then the "masterMob" field may be left NULL.
 *		For video, the sampleRate should be the edit rate of the file mob.
 *		For audio, the sample rate should be the actual samples per second.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMediaCreate(
			omfHdl_t			file,			/* IN -- In this file */
			omfObject_t		masterMob,	/* IN -- on this master mob */
			omfTrackID_t	masterTrackID,
			omfObject_t		fileMob,		/* IN -- and this file mob */
			omfDDefObj_t  	mediaKind,	/* IN -- create media of this type */
			omfRational_t	samplerate,	/* IN -- with this sample rate */
			omfCompressEnable_t enable,	/* IN -- optionally compressing it */
			omfMediaHdl_t	*result)		/* OUT -- Return the object here. */
{
	omfUID_t        uid;
	omfMediaHdl_t   media;
	omfTimeStamp_t  create_timestamp;
	omfObjIndexElement_t elem;
	omfCodecMetaInfo_t info;
	omfLength_t		oneLength = omfsCvtInt32toLength(1, oneLength);
	char 			codecIDString[OMUNIQUENAME_SIZE]; /* reasonable enough */
	omfCodecID_t	codecID;
	omfBool			found;
	omfErr_t       omfError = OM_ERR_NONE;
	mobTableEntry_t	*mobPtr;
	char			*variety;
	omfObject_t		tmpTrack;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	omfAssert(result != NULL, file, OM_ERR_NULL_PARAM);
	if(file->fmt == kOmfiMedia)
	{
		omfAssert(fileMob != NULL, file, OM_ERR_INVALID_FILE_MOB);
	}

	*result = NULL;
	XPROTECT(file)
	{
		media = (omfMediaHdl_t) omOptMalloc(file, sizeof(struct omfiMedia));
		XASSERT((media != NULL), OM_ERR_NOMEMORY);
	
		/* Initialize the basic fields of the media handle
		 */
		CHECK(InitMediaHandle(file, media, fileMob));
		media->masterMob = masterMob;
		media->fileMob = fileMob;
		media->compEnable = enable;
		media->dataFile = file;
		media->channels = (omfSubChannel_t *) omOptMalloc(file, sizeof(omfSubChannel_t));
		XASSERT((media->channels != NULL), OM_ERR_NOMEMORY);
		media->numChannels = 1;
		omfsCvtInt32toLength(0, media->channels[0].numSamples);
		media->channels[0].dataOffset = media->dataStart;
		media->channels[0].mediaKind = mediaKind;
		media->channels[0].trackID = masterTrackID;
		media->channels[0].physicalOutChan = 1;
		media->channels[0].sampleRate = samplerate;
		omfsCvtInt32toLength(0, media->channels[0].numSamples);
		media->openType = kOmfiCreated;
		media->masterTrackID = masterTrackID;
		
		if(file->fmt == kOmfiMedia)
		{
			/* Initialize the fields which are derived from information in
			 * the file mob or media descriptor.
			 */
			CHECK(omfmMobGetMediaDescription(file, fileMob, &media->mdes));
	
			if(file->setrev == kOmfRev2x)
			{
				CHECK(omfsWriteRational(file, media->mdes, OMMDFLSampleRate,
												samplerate));
			}
			else
			{
				CHECK(omfsWriteExactEditRate(file, media->mdes,
											OMMDFLSampleRate, samplerate));
			}
			
			/* RPS-- don't use the 'best codec' method on WRITE. Instead,  */
			/*   the toolkit now stores the codec ID in omfmFileMobNew()   */
			/*   The string must be in the descriptor, assert as such.     */
			/* JeffB -- This doesn't work when adding media to a cloned File Mob */
			/* made by someone other than the toolkit.  So if the special string */
			/* isn't there, find the best codec to handle the media descriptor */
			/* */
	
			if(omfsReadString(file, media->mdes, OMMDESCodecID, codecIDString, 
										OMUNIQUENAME_SIZE) == OM_ERR_NONE)
			{
				variety = strchr(codecIDString, ':');
				if(variety != NULL)
				{
					*variety = '\0';
					variety++;			/* Skip over the separator */
					media->codecVariety = (char*)omOptMalloc(file, strlen(variety)+1);
					strcpy(media->codecVariety, variety);
				}
				codecID = (omfCodecID_t)codecIDString;
				omfsTableLookupBlock(file->session->codecID, codecID, 
									 sizeof(media->pvt->codecInfo), &media->pvt->codecInfo, &found);
									 
				if(!found)
				{
					RAISE(OM_ERR_CODEC_INVALID);
				}
			}
			else
			{
				CHECK(omfmFindCodecForMedia(file, media->mdes, &media->pvt->codecInfo));
			}
		}
		else
		{
			codecID = file->rawCodecID;
			omfsTableLookupBlock(file->session->codecID, codecID, 
							 sizeof(media->pvt->codecInfo), &media->pvt->codecInfo, &found);
		}
		
		if(file->fmt == kOmfiMedia)
		{
			/* RPS-- back to your regularly scheduled program              */

		  /* JEFF!! Changed masterTrackID to be 1 when creating mono 
			* audio media, so file mob track will be labeled correctly */
		  if (omfiIsSoundKind(file, mediaKind, kExactMatch, &omfError))
			 masterTrackID = 1;

		  CHECK(omfsReadUID(file, fileMob, OMMOBJMobID, &uid));
		  /* JeffB: Handle the case where an existing file=>tape mob connection exists
			*/
		  if(file->setrev == kOmfRev2x)
		  {
				mobPtr = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, uid);
				if(mobPtr == NULL)
			  	{
					RAISE(OM_ERR_MISSING_MOBID);
			  	}
			  	if(FindTrackByTrackID(file, fileMob, masterTrackID, &tmpTrack) == OM_ERR_TRACK_NOT_FOUND)
			  	{
					CHECK(omfmMobAddNilReference(file, fileMob, masterTrackID, 
													 oneLength, mediaKind, mobPtr->createEditRate));
				}
				CHECK(FindTrackByTrackID(file, fileMob, masterTrackID, &tmpTrack));
				CHECK(omfiTrackSetPhysicalNum(file, tmpTrack, masterTrackID));
			 }
			else
			{
				omfRational_t	fileEditRate;
				
				CHECK(omfsReadExactEditRate(file, fileMob, OMCPNTEditRate, &fileEditRate));
			  if(FindTrackByTrackID(file, fileMob, masterTrackID, &tmpTrack) == OM_ERR_TRACK_NOT_FOUND)
			  {
					CHECK(omfmMobAddNilReference(file, fileMob, masterTrackID, oneLength, mediaKind,
													fileEditRate));
				}
			}
			
			CHECK(codecGetMetaInfo(file->session, &media->pvt->codecInfo,media->codecVariety, NULL, 0,
									&info));
			CHECK(omfsObjectNew(file, info.dataClassID, &media->dataObj));
		}
			
		/* We now have a valid media handle, so tell the world so.  Then 
		 * fill in some of the more optional fields.
		 */
		*result = media;
	
		if(file->fmt == kOmfiMedia)
		{
			omfsGetDateTime(&create_timestamp);
		
			if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			  {
				 CHECK(omfsWriteTimeStamp(file, file->head, OMLastModified,
												  create_timestamp));
			  }
			else /* kOmf2x */
			  {
				 CHECK(omfsWriteTimeStamp(file, file->head, OMHEADLastModified,
												  create_timestamp));
			  }
		
			/* do this now, delay may mess up data contiguity
			 */
			if(file->setrev == kOmfRev2x)
			{
				CHECK(omfsWriteUID(file, media->dataObj, OMMDATMobID, uid));
				CHECK(omfsAppendObjRefArray(file, file->head, OMHEADMediaData,
											media->dataObj));
			}
			else
			{
				elem.Mob = media->dataObj;
				elem.ID = uid;
				CHECK(omfsAppendObjIndex(file, file->head, OMMediaData, &elem));
				CHECK(omfsAppendObjRefArray(file, file->head, OMObjectSpine,
											elem.Mob));
			}
			
			CHECK(omfsTableAddUID(file->dataObjs, uid, media->dataObj,
									kOmTableDupError));
		}
			
		/* Call the codec to create the actual media.
		 */
		CHECK(codecCreate(media));
  		omfmSetVideoLineMap(media, 0, kTopFieldNone);
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfmMediaMultiCreate
 *
 * 	Creates a multi-channel interleaved stream of media.
 *
 *		The media handle from this call can be used with
 *		omfmWriteDataSamples or omfmWriteMultiSamples but NOT with 
 *		or omfmWriteDataLines.
 *
 * Argument Notes:
 *		If you are creating the media, and then attaching it to a master
 *		mob, then the "masterMob" field may be left NULL.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMediaMultiCreate(
			omfHdl_t				file,				/* IN - In this file */
			omfObject_t			masterMob,		/* IN - on this master mob */
			omfObject_t			fileMob,			/* IN - and this file mob, create */
			omfInt16					arrayElemCount,/* IN - this many channels */
			omfmMultiCreate_t *mediaArray,	/* IN - using these definitions */
			omfRational_t		editRate,/* IN -  */
			omfCompressEnable_t	enable,			/* IN - optionally compressing it */
			omfMediaHdl_t		*result)			/* OUT - Return the object here. */
{
	omfClassID_t 	mdesTag;
	omfUID_t        uid;
	omfMediaHdl_t   media;
	omfTimeStamp_t  create_timestamp;
	omfInt16				n;
	omfmMultiCreate_t *initPtr;
	omfSubChannel_t	*destPtr;
	omfObjIndexElement_t elem;
	omfCodecMetaInfo_t info;
	omfBool			found;
	omfLength_t		oneLength = omfsCvtInt32toLength(1, oneLength);
	omfProperty_t	idProp;
	omfCodecID_t	CodecID;
	omfObject_t		tmpTrack;
	char 			codecIDString[OMUNIQUENAME_SIZE]; /* reasonable enough */
	char			*variety;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	omfAssert(result != NULL, file, OM_ERR_NULL_PARAM);
	if (file->fmt == kOmfiMedia)
	{
	   omfAssert(fileMob != NULL, file, OM_ERR_INVALID_FILE_MOB);
	}
	*result = NULL;
	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		media = (omfMediaHdl_t) omOptMalloc(file, sizeof(struct omfiMedia));
		if(media == NULL)
			RAISE(OM_ERR_NOMEMORY);

		media->masterTrackID = 1;
	
		/* Initialize the basic fields of the media handle
		 */
		CHECK(InitMediaHandle(file, media, fileMob));
		media->masterMob = masterMob;
		media->fileMob = fileMob;
		media->compEnable = enable;
		media->dataFile = file;
		if (file->fmt == kOmfiMedia)
		  {
		     CHECK(omfmMobGetMediaDescription(file, fileMob, &media->mdes));
			  /* JeffB-- copied Roger's change from the single-channel case to use */
			  /* 			a private variable to store the codec ID.  Also copied    */
			  /* My fix to Roger's change */
			  /* */
			  if(omfsReadString(file, media->mdes, OMMDESCodecID, codecIDString, 
											OMUNIQUENAME_SIZE) == OM_ERR_NONE)
				{
					variety = strchr(codecIDString, ':');
					if(variety != NULL)
					{
						*variety = '\0';
						variety++;			/* Skip over the separator */
						media->codecVariety = (char*)omOptMalloc(file, strlen(variety)+1);
						strcpy(media->codecVariety, variety);
					}
					CodecID = (omfCodecID_t)codecIDString;
					omfsTableLookupBlock(file->session->codecID, CodecID, 
										 sizeof(media->pvt->codecInfo), &media->pvt->codecInfo, &found);
				}
		      else
		      {
		     		CHECK(omfsReadClassID(file, media->mdes, idProp, mdesTag));
		     		CHECK(omfsTableClassIDLookup(file->session->codecMDES, mdesTag,
						sizeof(media->pvt->codecInfo), &media->pvt->codecInfo, &found));
				}
		  }
		else
		  {
		     CodecID = file->rawCodecID;
		     	
		     omfsTableLookupBlock(file->session->codecID, CodecID, 
							 sizeof(media->pvt->codecInfo), &media->pvt->codecInfo, &found);
		  }
		 if(!found)
			     RAISE(OM_ERR_CODEC_INVALID); 

		media->numChannels = arrayElemCount;
		media->channels = (omfSubChannel_t *)
			omOptMalloc(file, sizeof(omfSubChannel_t) * media->numChannels);
		XASSERT((media->channels != NULL), OM_ERR_NOMEMORY);
		

		for (n = 0; n < arrayElemCount; n++)
		{
			initPtr = mediaArray + n;
			destPtr = media->channels + n;
         if(file->fmt == kOmfiMedia)
         {
				/* JeffB: Handle the case where an existing file=>tape mob connection exists
				 */
				if(FindTrackByTrackID(file, fileMob, initPtr->trackID, &tmpTrack) == OM_ERR_TRACK_NOT_FOUND)
				{
				 	CHECK(omfmMobAddNilReference(file, fileMob,
					  initPtr->trackID, oneLength, initPtr->mediaKind, editRate));
				}
				CHECK(FindTrackByTrackID(file, fileMob, initPtr->trackID, &tmpTrack));
				if(file->setrev == kOmfRev2x)
				{
					CHECK(omfiTrackSetPhysicalNum(file, tmpTrack, initPtr->trackID));
				}
		   }
			destPtr->mediaKind = initPtr->mediaKind;
			destPtr->trackID = initPtr->trackID;
			destPtr->physicalOutChan = initPtr->subTrackNum;
			omfsCvtInt32toPosition(0, destPtr->dataOffset);
			destPtr->dataOffset = media->dataStart;
			omfsCvtInt32toLength(0, destPtr->numSamples);
			destPtr->sampleRate = initPtr->sampleRate;
			omfsCvtInt32toLength(0, destPtr->numSamples);
		}
	
		media->openType = kOmfiCreated;
	
		if(file->fmt == kOmfiMedia)
		{	
			/* Initialize the fields which are derived from information in
			 * the file mob or media descriptor.
			 */
			if(file->setrev == kOmfRev2x)
			{
				CHECK(omfsWriteRational(file, media->mdes, OMMDFLSampleRate,
					media->channels[0].sampleRate));
			}
			else
			{
				CHECK(omfsWriteExactEditRate(file, media->mdes, OMMDFLSampleRate,
					media->channels[0].sampleRate));
			}

			CHECK(codecGetMetaInfo(file->session, &media->pvt->codecInfo,media->codecVariety, NULL, 0,
											&info));
			CHECK(omfsObjectNew(file, info.dataClassID, &media->dataObj));
		}
	
		/* We now have a valid media handle, so tell the world so.  Then fill in
		 * some of the more optional fields.
		 */
		*result = media;
		
		if(file->fmt == kOmfiMedia)
		{
			omfsGetDateTime(&create_timestamp);
			if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			  {
				 CHECK(omfsWriteTimeStamp(file, file->head, OMLastModified,
												  create_timestamp));
			  }
			else /* kOmfRev2x */
			  {
				 CHECK(omfsWriteTimeStamp(file, file->head, OMHEADLastModified,
												  create_timestamp));
			  }
		
			/* do this now, delay may mess up data contiguity
			 */
			CHECK(omfsReadUID(file, fileMob, OMMOBJMobID, &uid));
			if(file->setrev == kOmfRev2x)
			{
				CHECK(omfsWriteUID(file, media->dataObj, OMMDATMobID, uid));
				CHECK(omfsAppendObjRefArray(file, file->head, OMHEADMediaData,
													media->dataObj));
			}
			else
			{
				/* Codecs set the UID of the data object for the 1.5 case */
				elem.Mob = media->dataObj;
				elem.ID = uid;
				CHECK(omfsAppendObjIndex(file, file->head, OMMediaData, &elem));
				CHECK(omfsAppendObjRefArray(file, file->head, OMObjectSpine,
													elem.Mob));
			}
		}
	
		/* Call the codec to create the actual media.
		 */
		CHECK(codecCreate(media));
  		omfmSetVideoLineMap(media, 0, kTopFieldNone);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfmSetBlockingSize
 *
 * 		Sets the size of chunks allocated on disk during writes (only).
 *		Allocating the space in this fashion ensures that the data will be
 *		contiguous on disk (for at least numBytes bytes) even if other
 *		disk operations allocate space on the disk.  If the data written
 *		exceeds numBytes, then another disk block of numBytes size will be
 *		allocated.
 *
 * Argument Notes:
 * 	Takes a media handle, so the media must have been opened or created.
 *		The space is allocated in terms of bytes.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmSetBlockingSize(
			omfMediaHdl_t	media,		/* IN -- For this media reference */
			omfInt32				numBytes)	/* IN -- Preallocate this many bytes*/
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(numBytes >= 0, media->mainFile, OM_ERR_BLOCKING_SIZE);

	media->stream->blockingSize = numBytes;
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmSetVideoLineMap
 *
 * 	Sets the video line map structure used by the codecs to deinterlace
 *		fields.
 *
 * Argument Notes:
 * 	Takes a media handle, so the media must have been opened or created.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t  omfmSetVideoLineMap(
			omfMediaHdl_t	media,		/* IN -- For this media reference */
			omfInt16			startLine,	/* IN -- set this starting video line */
			omfFieldTop_t	type)		/* IN -- and this top field */
{
	omfVideoMemOp_t		ops[3];
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	
	XPROTECT(media->mainFile)
	{
		ops[0] .opcode= kOmfFrameLayout;
		ops[1] .opcode= kOmfVFmtEnd;

		CHECK(omfmGetVideoInfoArray(media, ops));
		
		if (ops[0].operand.expFrameLayout == kFullFrame)
			RAISE(OM_ERR_INVALID_OBJ);

		/* even field dominance makes no sense for single field video */
		if (ops[0].operand.expFrameLayout == kOneField && type == kDominantField2)
			RAISE(OM_ERR_INVALID_OBJ);
			
		ops[0] .opcode= kOmfVideoLineMap;
		ops[1] .opcode= kOmfFieldDominance;
		ops[1] .operand.expFieldDom = (omfFieldDom_t)type;
		ops[2] .opcode= kOmfVFmtEnd;
		
		if (startLine == 0)  /* for GRAPHICS files, no analog mapping */
		{
			switch(type)
			{
			case kDominantField1:
				ops[0].operand.expLineMap[0] = 0;
				ops[0].operand.expLineMap[1] = 1;
				break;
				
			case kDominantField2:
				ops[0].operand.expLineMap[0] = 1;
				ops[0].operand.expLineMap[1] = 0;
				break;
				
			case kNoDominant:
			default:
				ops[0].operand.expLineMap[0] = 0;
				ops[0].operand.expLineMap[1] = 0;
				break;
			}
		}
		else  /* Video mapping, need to know format */
		{
			omfInt16 offset;
			omfVideoSignalType_t signalType;
			
			CHECK(omfmSourceGetVideoSignalType(media, &signalType));

			switch (signalType)
			{
			case kNTSCSignal:
				offset = 263;
				break;
				
			case kPALSignal:
				offset = 312;
				break;
			
			default:
			   RAISE(OM_ERR_INVALID_VIDEOSIGNALTYPE);
				break;
			}
			
			switch(type)
			{
			case kDominantField1:
				ops[0].operand.expLineMap[0] = (omfInt32) startLine;
				ops[0].operand.expLineMap[1] = (omfInt32) (startLine+offset+1);
				break;
				
			case kDominantField2:
				ops[0].operand.expLineMap[0] = (omfInt32) startLine;
				ops[0].operand.expLineMap[1] = (omfInt32) (startLine+offset);
				break;
				
			case kNoDominant:
			default:
			   RAISE(OM_ERR_INVALID_VIDEOSIGNALTYPE);
				break;
			}
			
		
		}
	
		CHECK(codecPutVideoInfo(media, ops));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmGetVideoTopField
 *
 * 	Gets the top video field used by the codecs to deinterlace
 *		fields.
 *
 * Argument Notes:
 * 	Takes a media handle, so the media must have been opened or created.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t  omfmGetVideoTopField(
			omfMediaHdl_t	media,		/* IN -- For this media reference */
			omfFieldTop_t	*type)		/* OUT -- and this top field */
{
	omfVideoMemOp_t		ops[3];

	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	
	XPROTECT(media->mainFile)
	{
		ops[0].opcode = kOmfVideoLineMap;
		ops[1].opcode = kOmfFrameLayout;
		ops[2].opcode = kOmfVFmtEnd;
		CHECK(codecGetVideoInfo(media, ops));

		if (ops[1].operand.expFrameLayout != kSeparateFields &&
			ops[1].operand.expFrameLayout != kMixedFields)
		{
			*type = kTopFieldNone;  /* dominance only relevant for multi-field */
			return(OM_ERR_NONE);
		}

		if(ops[0].operand.expLineMap[0] < ops[0].operand.expLineMap[1])
			*type = kTopField1;  /* dominance only relevant for multi-field */
		else
			*type = kTopField2;  /* dominance only relevant for multi-field */

	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmSetDisplayRect
 *
 *		Handles the case where the displayed rectangle is not the same as
 *		the stored rectangle (as with the old leadingLines and
 *		trailingLines).
 *		A positive "leadingLines" (from 1.5) becomes a positive yOffset, and
 *		decreases the display height.
 *		A positive "trailingLines" (from 1.5) also decreases the display
 *		height.
 *
 * Argument Notes:
 * 	Takes a media handle, so the media must have been opened or created.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmSetDisplayRect(
			omfMediaHdl_t	media,			/* IN -- For this media reference */
			omfRect_t		DisplayRect)	/* IN -- set sampled rect */
{
	omfHdl_t				main;
	omfVideoMemOp_t	ops[2];
	
	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);
	omfAssertMediaInitComplete(main);

	XPROTECT(main)
	{
		ops[0].opcode = kOmfDisplayRect;
		ops[0].operand.expRect = DisplayRect;
		ops[1].opcode = kOmfVFmtEnd;

		CHECK(codecPutVideoInfo(media, ops));
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}


/************************
 * Function: omfmSetSampledRect
 *
 *		Handles the case where the sampled rectangle is not the same as
 *		the stored rectangle .
 *
 * Argument Notes:
 * 	Takes a media handle, so the media must have been opened or created.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmSetSampledRect(
			omfMediaHdl_t	media,			/* IN -- For this media reference */
			omfRect_t		SampledRect)	/* IN -- set sampled rect */
{
	omfHdl_t				main;
	omfVideoMemOp_t	ops[2];

	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);
	omfAssertMediaInitComplete(main);

	XPROTECT(main)
	{
		ops[0].opcode = kOmfSampledRect;
		ops[0].operand.expRect = SampledRect;
		ops[1].opcode = kOmfVFmtEnd;

		CHECK(codecPutVideoInfo(media, ops));
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}


/************************
 * Function: omfmWriteDataSamples
 *
 * 		Writes data to a single-channel media stream.
 *
 * Argument Notes:
 * 	Takes a media handle, so the media must have been opened or created.
 *		A single video frame is ONE sample.
 *		Buflen must be large enough to hold
 *			nSamples * the maximum sample size.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_SINGLE_CHANNEL_OP -- Tried to write to an interleaved stream.
 *		OM_ERR_BADDATAADDRESS -- The buffer must not be a NULL pointer.
 */
omfErr_t omfmWriteDataSamples(
			omfMediaHdl_t	media,		/* IN -- For this media reference */
			omfInt32			nSamples,		/* IN -- write this many samples */
			void			*buffer,			/* IN -- to a buffer */
			omfInt32			buflen)			/* IN -- of this size */
{
	omfmMultiXfer_t xfer;

	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(media->numChannels == 1, media->mainFile,
		OM_ERR_SINGLE_CHANNEL_OP);
	omfAssert(buffer != NULL, media->mainFile, OM_ERR_BADDATAADDRESS);
	omfAssert((media->openType == kOmfiCreated) ||
				(media->openType == kOmfiAppended), media->mainFile, OM_ERR_MEDIA_OPENMODE);
	
	XPROTECT(media->mainFile)
	{
		xfer.subTrackNum = media->channels[0].physicalOutChan;
		xfer.numSamples = nSamples;
		xfer.buflen = buflen;
		xfer.buffer = buffer;
		xfer.bytesXfered = 0;
	
		CHECK (codecWriteBlocks(media, deinterleave, 1, &xfer));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmWriteRawData
 *
 * 		Writes pre-interleaved data to a media stream.  This function
 *		does what the 1.5 toolkit did for omfmWriteSamples.
 *
 * Argument Notes:
 *		A single video frame is ONE sample.
 *		Buflen must be large enough to hold
 *			nSamples * the maximum sample size.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADDATAADDRESS -- The buffer must not be a NULL pointer.
 */
omfErr_t omfmWriteRawData(
			omfMediaHdl_t	media,	/* IN -- For this media reference */
			omfInt32			nSamples,	/* IN -- write this many samples */
			void			*buffer,		/* IN -- to a buffer */
			omfInt32			buflen)		/* IN -- of this size */
{
	omfmMultiXfer_t xfer;

	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(buffer != NULL, media->mainFile, OM_ERR_BADDATAADDRESS);
	omfAssert((media->openType == kOmfiCreated) ||
				(media->openType == kOmfiAppended), media->mainFile,OM_ERR_MEDIA_OPENMODE);

	XPROTECT(media->mainFile)
	{
		xfer.subTrackNum = 1;
		xfer.numSamples = nSamples;
		xfer.buflen = buflen;
		xfer.buffer = buffer;
		xfer.bytesXfered = 0;

		CHECK(codecWriteBlocks(media, leaveInterleaved, 1, &xfer));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmReadRawData
 *
 * 	Read pre-interleaved data from a media stream.  This function
 *		does what the 1.5 toolkit did for omfmReadSamples.
 *
 * Argument Notes:
 *		A single video frame is ONE sample.
 *		Buflen must be large enough to hold
 *			nSamples * the maximum sample size.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADDATAADDRESS -- The buffer must not be a NULL pointer.
 */
omfErr_t omfmReadRawData(
			omfMediaHdl_t	media,	/* IN -- For this media reference */
			omfInt32			nSamples,	/* IN -- write this many samples */
			omfUInt32		buflen,		/* IN -- of this size */
			void			*buffer,		/* IN -- to a buffer */
			omfUInt32		*bytesRead,
			omfUInt32		*samplesRead)
{
	omfmMultiXfer_t xfer;

	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(buffer != NULL, media->mainFile, OM_ERR_BADDATAADDRESS);
	XPROTECT(media->mainFile)
	{
		xfer.subTrackNum = 1;
		xfer.numSamples = nSamples;
		xfer.buflen = buflen;
		xfer.buffer = buffer;
		xfer.bytesXfered = 0;
		xfer.samplesXfered = 0;
	
		CHECK(codecReadBlocks(media, leaveInterleaved, 1, &xfer));
		*bytesRead = xfer.bytesXfered;
		*samplesRead = xfer.samplesXfered;
	}
	XEXCEPT
	{
		*bytesRead = xfer.bytesXfered;
		*samplesRead = xfer.samplesXfered;
	}
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmWriteMultiSamples
 *
 * 		Writes multiple channels worth of sample data to an interleaved
 *		data stream, in the natural order for the CODEC.
 *
 * Argument Notes:
 *		arrayElemCount is the size of the array or transfer operations.
 *		xferArray points to an array of transfer parameters.  All fields
 *		in this array except for bytesXferred must be set up before
 *		doing the transfer.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmWriteMultiSamples(
			omfMediaHdl_t	media,			/* IN -- For this media reference */
			omfInt16				arrayElemCount,/* IN -- Do this many transfers */
			omfmMultiXfer_t *xferArray)	/* IN/OUT -- referencing this array */
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert((media->openType == kOmfiCreated) ||
				(media->openType == kOmfiAppended), media->mainFile,OM_ERR_MEDIA_OPENMODE);

	XPROTECT(media->mainFile)
	{
		CHECK (codecWriteBlocks(media, deinterleave, arrayElemCount, xferArray));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmWriteDataLines
 *
 * 		Writes single lines of video to a file.  This function allows writing
 *		video frames in pieces, for low-memory situations.  When enough lines
 *		have been written to constitute a frame, then the number of samples will
 *		be incremented by one.
 *		This function works only for video media.
 *
 * Argument Notes:
 *		The buffer must be large enough to hold an entire line of video. 
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADRWLINES -- This function only works for video media.
 */
omfErr_t omfmWriteDataLines(
			omfMediaHdl_t	media,		/* IN - For this media reference */
			omfInt32			nLines,			/* IN - write this many lines on video */
			void			*buffer,			/* IN - from a buffer */
			omfInt32			*bytesWritten)	/* OUT - of this size. */
{
	omfInt16		numVideo;
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert((media->openType == kOmfiCreated) ||
				(media->openType == kOmfiAppended), media->mainFile, OM_ERR_MEDIA_OPENMODE);
	
	XPROTECT(media->mainFile)
	{
		CHECK(codecGetNumChannels(media->mainFile, media->fileMob,
											media->pictureKind, &numVideo));
		if(numVideo <= 0)
			RAISE(OM_ERR_BADRWLINES);
		CHECK (codecWriteLines(media, nLines, buffer, bytesWritten));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmMediaClose
 *
 * 		Finish operations on media data, and close any external files
 *		opened or created to hold the media.  This function should be
 *		called whether the media was opened or created, and replaces
 *		omfmCleanup() from the 1.5 toolkit.
 *
 * Argument Notes:
 *		<none>
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMediaClose(
			omfMediaHdl_t	media)	/* IN -- Close this media */
{
	omfHdl_t       	main;
	omfMediaHdl_t	tstMedia;
	omfInt16       	n;
	omfTrackID_t	trackID;
	omfTrackID_t   	fileTrackID;
	omfErr_t		omfError = OM_ERR_NONE;
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	main = media->mainFile;

	XPROTECT(main)
	{
		/* Close the codec before creating more objects, in order to keep trailer data
		 * with the media in the file.
		 */
		if(media->pvt->codecInfo.rev != 0)		/* A codec was opened */
			CHECK(codecClose(media));

		if((media->openType == kOmfiCreated) || (media->openType == kOmfiAppended))
		{
			if(main->fmt == kOmfiMedia)
			{
				if(media->numChannels == 1)
					trackID = media->masterTrackID;
				else if(media->masterMob != NULL)
				{
					omfNumTracks_t		numMasterTracks;
					CHECK(omfiMobGetNumTracks(main, media->masterMob, &numMasterTracks));
					trackID = numMasterTracks + 1;
				}
				else
				  trackID = 1;
				for(n = 0; n < media->numChannels; n++)
				{
					CHECK(omfsWriteLength(main, media->mdes, OMMDFLLength,
														media->channels[n].numSamples));
					
					if((media->openType == kOmfiCreated) && (media->masterMob != NULL))
					{
					  /* JEFF!! Changed fileTrackID to be 1 when creating audio.
						* This should probably also be 1 for video?
						*/
					  if (omfiIsSoundKind(main, media->channels[n].mediaKind, 
												 kExactMatch, &omfError))
						 {
							fileTrackID = 1+n;
						 }
					  else
						 fileTrackID = trackID+n;

						CHECK(omfmMobAddMasterTrack(main, media->masterMob,
												 media->channels[n].mediaKind, trackID+n, 
/*															 trackID+n, */
															 fileTrackID,
													 NULL, media->fileMob));
					}
				}
				CHECK(omfsReconcileMobLength(main, media->fileMob));
				if(media->masterMob != NULL)
				{
					CHECK(omfsReconcileMobLength(main, media->masterMob));
				}
			}
		}
	
		/* Unlink this media from the list maintained by the omfHdl_t
		 */
		if (media == main->topMedia)
			main->topMedia = media->pvt->nextMedia;
		else
		{
			tstMedia = main->topMedia;
			while (tstMedia->pvt->nextMedia != NULL)
			{
				if (tstMedia->pvt->nextMedia == media)
					tstMedia->pvt->nextMedia = media->pvt->nextMedia;
				else
					tstMedia = tstMedia->pvt->nextMedia;
			}
		}
	
		/* If the data is kept in an external file, then close that file.
		 * If the data is in the current file, leave it open.
		 */
		if ((media->dataFile != main) && (media->dataFile != NULL))
			CHECK(omfsCloseFile(media->dataFile));

		if(media->channels != NULL)
		{
			omOptFree(main, media->channels);
			media->channels = NULL;
		}
		
		if(media->stream != NULL)
			omOptFree(main, media->stream);
		media->stream = NULL;
		if(media->pvt != NULL)
			omOptFree(main, media->pvt);
		media->pvt = NULL;
		
		if(media->codecVariety != NULL)
			omOptFree(main, media->codecVariety);
		media->codecVariety = NULL;
		
		/* Clear the cookie so that this handle will show up as invalid
		 * if the client continues to use it.
		 */
		media->cookie = 0;
		omOptFree(main, media);
	}
	XEXCEPT
	{
	}
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmSetJPEGTables
 *
 * 		Sets the Q, AC, & DC tables for the next frame to be compressed.
 *		This function needs to be called once for each component of the
 *		video.
 *
 * Argument Notes:
 *		<none>
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmSetJPEGTables(
			omfMediaHdl_t	media,			/* IN -- For this media ref */
			omfJPEGcomponent_t JPEGcomp,	/* IN -- change this component */
			omfUInt8				* QTables,		/* IN -- to have these Q-tables */
			omfUInt8				* ACTables,		/* IN -- and these AC-tables*/
			omfUInt8				* DCTables,		/* IN -- and these DC-tables*/
			omfInt16				QTableSize,		/* IN -- size in bytes */
			omfInt16				ACTableSize,	/* IN -- size in bytes */
			omfInt16				DCTableSize)	/* IN -- size in bytes */
{
	omfJPEGTables_t tables;

	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);

	tables.JPEGcomp = JPEGcomp;
	tables.QTables = QTables;
	tables.ACTables = ACTables;
	tables.DCTables = DCTables;
	tables.QTableSize = QTableSize;
	tables.ACTableSize = ACTableSize;
	tables.DCTableSize = DCTableSize;

	return (codecPutInfo(media, kJPEGTables, media->pictureKind,
								sizeof(tables), &tables));
}


/************************
 * Function: omfmSetAudioCompressParms / omfmGetAudioCompressParms
 *
 * 		Set common audio compression parms.
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
omfErr_t omfmSetAudioCompressParms(
			omfMediaHdl_t	media,			/* IN -- For this media, set */
			omfInt16				blockLength)	/* IN -- the block length. */
{
	omfAudioCompressParms_t cparms;

	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);

	cparms.blockLength = blockLength;
	return (codecPutInfo(media, kAudioCompressParms, media->soundKind, sizeof(cparms), &cparms));
}

/************************
 * Function: omfmGetAudioCompressParms
 */
omfErr_t omfmGetAudioCompressParms(
			omfMediaHdl_t media,			/* IN -- */
			omfInt16			*blockLength)	/* OUT -- */
{
	omfAudioCompressParms_t cparms;
	omfErr_t				status;
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(blockLength != NULL, media->mainFile, OM_ERR_NULL_PARAM);

	status = codecGetInfo(media, kAudioCompressParms, media->soundKind,
								sizeof(cparms), &cparms);
	*blockLength = cparms.blockLength;
	return(status);
}

/************************
 * omfmCodecSendPrivateData / omfmGetPrivateMediaData
 *
 * 		Sends a parameter block of private information to the CODEC for
 *		this media.
 *
 * Argument Notes:
 *		The parameter block should be defined in the
 *		"h" file of the codec, and must be included by the application in
 *		order to use this call.
 *
 *		NOTE: All CODECs should default to reasonable parameters,
 *		in case the application doesn't know about a given codec.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmCodecSendPrivateData(
			omfMediaHdl_t	media,			/* IN -- For this media, send */
			int 			parmBlockSize,		/* IN -- a parm block of this size */
			void			*ParameterBlock)	/* IN -- with these values. */
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);

	return (codecPutInfo(media, kCompressionParms, media->nilKind,
								parmBlockSize, ParameterBlock));
}

/************************
 * Function: omfmGetPrivateMediaData
 */
omfErr_t omfmGetPrivateMediaData(
			omfMediaHdl_t media,		/* IN -- */
			int 			blocksize,	/* IN -- */
			void			*result)		/* IN/OUT -- */
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(result != NULL, media->mainFile, OM_ERR_NULL_PARAM);

	XPROTECT(media->mainFile)
	{
		CHECK(codecGetInfo(media, kCompressionParms, media->nilKind, blocksize,
								result));
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************************************************************
 *
 * Routines to customize locator handling
 *
 ************************************************************************/

/************************
 * Function: omfmLocatorFailureCallback
 *
 * 		Sets a callback which gets called if none of the locators for
 *		media reference valid files, and the media is not in the current
 *		main file.  By supplying a callback, the application can continue
 *		the search for a valid media file without losing current context.
 *
 *		If the callback succeeds, then the media open process will continue
 *		as if nothing happened.
 *		If the callback returns NULL, then the media open process will fail.
 *
 * Argument Notes:
 *		The callback should have the prototype:
 *			omfHdl_t	failureCallback(omfHdl_t file, omfObject_t mdes);
 *
 *		and should return a valid file reference to the file containing
 *		the media data (if found) or NULL if the media could not be found.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmLocatorFailureCallback(
			omfHdl_t			file,		/* IN -- For this file */
			omfLocatorFailureCB	callback)	/* IN -- set this callback */
{
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);

	file->locatorFailureCallback = callback;
	return (OM_ERR_NONE);
}


/************************************************************************
 *
 * Routines for reading media data
 *
 ************************************************************************/


/************************
 * Function: omfmGetNumChannels
 *
 * 		Returns the number of interleaved media channels of a given
 *		type in the media stream referenced by the given file mob.
 *		If the data format is not interleaved, then the answer will
 *		always be zero or one.  This function correctly returns zero
 *		for media types not handled by a given codec, and handles codecs
 *		which work with multiple media types.
 *
 * Argument Notes:
 *		Takes an opaque handle, a master mob reference, and a track IDso that
 *		it may be called before the media is opened.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NULL_PARAM -- A return parameter was NULL.
 */
omfErr_t omfmGetNumChannels(
			omfHdl_t			file,			/* IN -- For this file */
			omfObject_t		masterMob,	/* IN -- In this master mob */
			omfTrackID_t	trackID,		/* IN -- On this track */
			omfMediaCriteria_t *mediaCrit,	/* IN -- using this media criteria */
			omfDDefObj_t  	mediaKind,	/* IN -- for this media type */
			omfInt16			*numCh)		/* OUT -- How many channels? */
{
	omfPosition_t		zeroPos;
	omfsCvtInt32toPosition(0, zeroPos);	
	return(omfmGetNumChannelsWithPosition(file, masterMob, trackID, mediaCrit, mediaKind, zeroPos, numCh));

}

omfErr_t omfmGetNumChannelsWithPosition(
			omfHdl_t			file,			/* IN -- For this file */
			omfObject_t		masterMob,	/* IN -- In this master mob */
			omfTrackID_t	trackID,		/* IN -- On this track */
			omfMediaCriteria_t *mediaCrit,	/* IN -- using this media criteria */
			omfDDefObj_t  	mediaKind,	/* IN -- for this media type */
			omfPosition_t	position,
			omfInt16			*numCh)		/* OUT -- How many channels? */
{
	omfFindSourceInfo_t	sourceInfo;
	omfErr_t			omfErr;
	omfObject_t			fileMob;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);

	omfAssert(numCh != NULL, file, OM_ERR_NULL_PARAM);
	XPROTECT(file)
	{
		if(omfiIsAFileMob(file, masterMob, &omfErr))
			fileMob = masterMob;
		else
		{
         // MC 01-Dec-03 -- fix for crash
         omfEffectChoice_t effectChoice = kFindEffectSrc1;
			CHECK(omfiMobSearchSource(file, masterMob, trackID, position, kFileMob,
									mediaCrit, &effectChoice, NULL, &sourceInfo));
			fileMob = sourceInfo.mob;
		}

		CHECK(codecGetNumChannels(file, fileMob, mediaKind, numCh));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: ompvtGetNumChannels
 *
 * 		Returns the number of interleaved media channels of a given
 *		type in the media stream referenced by the given file mob.
 *		If the data format is not interleaved, then the answer will
 *		always be zero or one.  This function correctly returns zero
 *		for media types not handled by a given codec, and handles codecs
 *		which work with multiple media types.
 *
 * Argument Notes:
 *		Takes an opaque handle, a master mob reference, and a track IDso that
 *		it may be called before the media is opened.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NULL_PARAM -- A return parameter was NULL.
 */
omfErr_t ompvtGetNumChannelsFromFileMob(
			omfHdl_t			file,			/* IN -- For this file */
			omfObject_t		fileMob,	/* IN -- In this master mob */
			omfDDefObj_t  	mediaKind,	/* IN -- for this media type */
			omfInt16			*numCh)		/* OUT -- How many channels? */
{
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);

	omfAssert(numCh != NULL, file, OM_ERR_NULL_PARAM);
	XPROTECT(file)
	{
		CHECK(codecGetNumChannels(file, fileMob, mediaKind, numCh));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmGetLargestSampleSize
 *
 * 		Returns the size in bytes of the largest sample for a given media
 *		type.  For uncompressed data, or the output of the software codec,
 *		the sample size will propably be a constant.
 *
 * Argument Notes:
 *		The media type parameter exists to support codecs with multiple
 *		interleaved media types.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NULL_PARAM -- A return parameter was NULL.
 */
omfErr_t omfmGetLargestSampleSize(
			omfMediaHdl_t	media,		/* IN -- For this media stream */
			omfDDefObj_t	mediaKind,		/* IN -- and this media type */
			omfInt32			*maxSize)	/* OUT -- the largest sample size */
{
	omfMaxSampleSize_t	parms;
	omfHdl_t			main;
	
	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);
	omfAssertMediaInitComplete(main);

	omfAssert(maxSize != NULL, main, OM_ERR_NULL_PARAM);
	*maxSize = 0;
	XPROTECT(main)
	{
		parms.mediaKind = mediaKind;
		CHECK(codecGetInfo(media, kMaxSampleSize, mediaKind, sizeof(parms),
								&parms));
		*maxSize = parms.largestSampleSize;
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmGetLargestSampleSize
 *
 * 		Returns the size in bytes of the largest sample for a given media
 *		type.  For uncompressed data, or the output of the software codec,
 *		the sample size will propably be a constant.
 *
 * Argument Notes:
 *		The media type parameter exists to support codecs with multiple
 *		interleaved media types.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NULL_PARAM -- A return parameter was NULL.
 */
omfErr_t omfmGetSampleFrameSize(
			omfMediaHdl_t	media,		/* IN -- For this media stream */
			omfDDefObj_t	mediaKind,	/* IN -- and this media type */
			omfPosition_t	frameNum,	/* IN -- for this (1-based) sample frame number */
			omfLength_t		*frameSize)	/* OUT -- How big is the sample frame? */
{
	omfFrameSizeParms_t	parms;
	omfHdl_t			main;
	
	omfAssertMediaHdl(media);
	main = media->mainFile;
	omfAssertValidFHdl(main);
	omfAssertMediaInitComplete(main);

	omfAssert(frameSize != NULL, main, OM_ERR_NULL_PARAM);
	omfsCvtInt32toInt64(0, frameSize);
	XPROTECT(main)
	{
		parms.mediaKind = mediaKind;
		parms.frameNum = frameNum;
		CHECK(codecGetInfo(media, kSampleSize, mediaKind, sizeof(parms),
								&parms));
		*frameSize = parms.frameSize;
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmMediaOpen
 *
 * 	Opens a single channel of a file mob.  If the media is interleaved,
 *		then it will be di-interleaved when samples are read.  This routine
 *		follows the locator, and may call the locator failure callback if
 *		the media can not be found.  If the failure callback finds the media,
 *		then this routine will return normally.
 *
 *		The media handle from this call can be used with
 *		omfmReadDataSamples  and possibly omfmReadDataLines, but NOT with
 *		omfmReadMultiSamples.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NOMEMORY -- couldn't allocate memory for the media handle
 *
 * NOTE: If a locator is followed, then mediaPtr may reference ANOTHER file
 * object, which must be closed on file close.
 */
omfErr_t omfmMediaOpen(
			omfHdl_t					file,			/* IN -- For this file */
			omfObject_t				masterMob,	/* IN -- In this master mob */
			omfTrackID_t			trackID,		/* IN -- On this track */
			omfMediaCriteria_t	*mediaCrit,	/* IN -- using this media criteria */
			omfMediaOpenMode_t	openMode,	/* IN -- ReadOnly or Append */
			omfCompressEnable_t	compEnable,	/* IN -- optionally decompressing */
			omfMediaHdl_t			*mediaPtr)	/* OUT -- and return a media handle */
{
	omfPosition_t	zeroPos;
	omfsCvtInt32toPosition(0, zeroPos);	

	return omfmMediaOpenFromOffset( file, masterMob, zeroPos, trackID,
									mediaCrit, openMode, compEnable, mediaPtr);
}


omfErr_t omfmMediaOpenFromOffset(
			omfHdl_t				file,			/* IN -- For this file */
			omfObject_t				masterMob,	/* IN -- In this master mob */
			omfPosition_t			pos,		/* IN -- offset in master mob */
			omfTrackID_t			trackID,		/* IN -- On this track */
			omfMediaCriteria_t	*mediaCrit,	/* IN -- using this media criteria */
			omfMediaOpenMode_t	openMode,	/* IN -- ReadOnly or Append */
			omfCompressEnable_t	compEnable,	/* IN -- optionally decompressing */
			omfMediaHdl_t			*mediaPtr)	/* OUT -- and return a media handle */
{
	return(omfmMediaOpenWithPosition(file, masterMob, trackID, mediaCrit, 
					 openMode, compEnable, pos, mediaPtr));

}


omfErr_t omfmMediaOpenWithPosition(
			omfHdl_t					file,			/* IN -- For this file */
			omfObject_t				masterMob,	/* IN -- In this master mob */
			omfTrackID_t			trackID,		/* IN -- On this track */
			omfMediaCriteria_t	*mediaCrit,	/* IN -- using this media criteria */
			omfMediaOpenMode_t	openMode,	/* IN -- ReadOnly or Append */
			omfCompressEnable_t	compEnable,	/* IN -- optionally decompressing */
			omfPosition_t		pos,
			omfMediaHdl_t			*mediaPtr)	/* OUT -- and return a media handle */
{
	omfTrackID_t	tmpTrackID;
	omfIterHdl_t	mobIter = NULL;
	omfInt32 		numTracks, loop;
	omfBool			foundTrack;
	omfObject_t		track = NULL, seg = NULL;
	omfDDefObj_t	mediaKind;
	omfLength_t masterMobLength, numSamples, one;
	omfFindSourceInfo_t	sourceInfo;
   omfEffectChoice_t effectChoice;

	XPROTECT(file)
	{
		omfsCvtInt32toLength(1, one);

      // MC 01-Dec-03 -- fix for crash
      effectChoice = kFindEffectSrc1;
		CHECK(omfiMobSearchSource(file, masterMob, trackID, pos, kFileMob,
									   mediaCrit, &effectChoice, NULL, &sourceInfo));
									   
		CHECK(omfiIteratorAlloc(file, &mobIter));
		CHECK(omfiMobGetNumTracks(file, masterMob, &numTracks));
		for (loop = 1; loop <= numTracks; loop++)
		{
			CHECK(omfiMobGetNextTrack(mobIter, masterMob, NULL, &track));
			CHECK(omfiTrackGetInfo(file, masterMob, track, NULL, 0, NULL, NULL, 
								   &tmpTrackID, &seg));
			if (tmpTrackID == trackID)
			{
				foundTrack = TRUE;
				break;
			}
		}
		if (!foundTrack)
		{
			RAISE(OM_ERR_TRACK_NOT_FOUND);
		}
		CHECK(omfiComponentGetInfo(file, seg, &mediaKind, &masterMobLength));
		/* !!! NOTE: Assumes that file trackID's are 1-N */
		CHECK(ompvtMediaOpenFromFileMob(file, sourceInfo.mob, sourceInfo.mobTrackID, openMode, mediaKind, 
												  (omfInt16)sourceInfo.mobTrackID, compEnable, mediaPtr));
		(*mediaPtr)->masterMob = masterMob;
		
		/* JeffB: This code is here because files imported into the
		 * Media Composer as PICT and then exported to OMF don't have
		 * a REPT effect to indicate the freeze frame.  Since the
		 * Media Composer code was written int  ago, and there are many
		 * files with this condition in the field, the toolkit will
		 * handle it for pre 2.0 files.
		 */
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		  {
			 CHECK(omfmGetSampleCount(*mediaPtr, &numSamples));
			 if(omfsInt64Equal(numSamples, one))
				{
				  if(omfsInt64NotEqual(masterMobLength, one))
					 (*mediaPtr)->pvt->repeatCount = masterMobLength; 
				}
		  }
		if(mobIter != NULL)
		  {
			 omfiIteratorDispose(file, mobIter);
			 mobIter = NULL;
		  }
	 }
	XEXCEPT
	{
		if (mobIter)
		  omfiIteratorDispose(file, mobIter);
	}
	XEND
	
	return (OM_ERR_NONE);
}
			
/* JeffB: Files opened directly with this function and appended to will not
 *  update the master mob length.  omfmOpenMedia() opened media will work.
 * This shouldn't be a problem, as the 1.x interface doesn't allow for appends
 * anyway.
 */
omfErr_t ompvtMediaOpenFromFileMob(
			omfHdl_t					file,		/* IN -- For this file */
			omfObject_t				fileMob,		/* IN -- In this master mob */
			omfTrackID_t				trackID,
			omfMediaOpenMode_t	openMode,	/* IN -- ReadOnly or Append */
			omfDDefObj_t			mediaKind,	/* IN -- and this media type */
			omfInt16					physicalOutChan,	/* IN -- open media at this track */
			omfCompressEnable_t	compEnable,	/* IN -- optionally decompressing */
			omfMediaHdl_t			*mediaPtr)	/* OUT -- and return a media handle */
{
	omfMediaHdl_t	media;
	omfInt16			numCh, n;
	omfUID_t			fileMobID;
	omfBool			isOMFI;
	omfPosition_t	numSamples;
	omfIterHdl_t	mobIter = NULL;
	omfObject_t		track = NULL, seg = NULL;
	omfInt32 rept;
	omfErr_t       status;

	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);

	omfAssert(fileMob != NULL, file, OM_ERR_INVALID_FILE_MOB);
	omfAssert(mediaPtr != NULL, file, OM_ERR_NULL_PARAM);
	*mediaPtr = NULL;
	XPROTECT(file)
	{
		media = (omfMediaHdl_t) omOptMalloc(file, sizeof(struct omfiMedia));
		memset (media, 0, sizeof(*media));	/* initialize media to NULL for BoundsChecker */
		if(media == NULL)
			RAISE(OM_ERR_NOMEMORY);
	
		CHECK(InitMediaHandle(file, media, fileMob));
		media->compEnable = compEnable;
		if(openMode == kMediaOpenAppend)
			media->openType = kOmfiAppended;
		else
			media->openType = kOmfiOpened;
	
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		 {
			CHECK(omfiIteratorAlloc(file, &mobIter));
			CHECK(omfiMobGetNextTrack(mobIter, fileMob, NULL, &track));
			CHECK(omfiTrackGetInfo(file, fileMob, track, NULL, 0, NULL, NULL, 
								   NULL, &seg));
			if(omfsIsTypeOf(file, seg, "REPT", &status))
			{
			  omfsReadInt32(file, seg, OMTRKGGroupLength, &rept);
			  omfsCvtInt32toInt64(rept, &media->pvt->repeatCount);
			}
		 }

		CHECK(omfmMobGetMediaDescription(file, fileMob, &media->mdes));
		CHECK(codecGetNumChannels(file, fileMob, mediaKind, &numCh));
		if (numCh == 0)
		  RAISE(OM_ERR_INVALID_DATAKIND);

		media->channels =
			(omfSubChannel_t *)omOptMalloc(file, sizeof(omfSubChannel_t) * numCh);
		if(media->channels == NULL)
			RAISE(OM_ERR_NOMEMORY);
		media->numChannels = numCh;
		for(n = 0; n < numCh; n++)
		{
			omfsCvtInt32toLength(0, media->channels[n].numSamples);
			media->channels[n].dataOffset = media->dataStart;
			media->channels[n].mediaKind = mediaKind;
			media->channels[n].physicalOutChan = n+1;
			media->channels[n].trackID = trackID+n;
		}
		media->physicalOutChanOpen = physicalOutChan;
		
		CHECK(LocateMediaFile(file, fileMob, &media->dataFile, &isOMFI));
		if(media->dataFile == NULL)
			RAISE(OM_ERR_MEDIA_NOT_FOUND);
		
		media->rawFile = media->dataFile->rawFile;			/* If non-omfi file */
		CHECK(omfmFindCodecForMedia(file, media->mdes, &media->pvt->codecInfo));
		if(isOMFI)
		{
			CHECK(omfsReadUID(file, fileMob, OMMOBJMobID, &fileMobID));
			/* MCX/NT Hack! */
			media->dataObj = (omfObject_t)omfsTableUIDLookupPtr(media->dataFile->dataObjs, 
																fileMobID);
			if((fileMobID.prefix == 444) && (media->dataObj == NULL))
			{
				fileMobID.prefix = 1;
				media->dataObj = (omfObject_t)omfsTableUIDLookupPtr(media->dataFile->dataObjs, 
																fileMobID);
			}
		}
		CHECK(codecOpen(media));
		if(openMode == kMediaOpenAppend)
		{
			CHECK(omfmGetSampleCount(media, &numSamples));										  
			CHECK(omfsAddInt32toInt64(1, &numSamples));
			CHECK(omfmGotoFrameNumber(media, numSamples));
		}
		if (mobIter)
		  omfiIteratorDispose(file, mobIter);
	}
	XEXCEPT
	{
		if (mobIter)
		  omfiIteratorDispose(file, mobIter);
	}
	XEND
	*mediaPtr = media;
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfmMediaMultiOpen
 *
 * 		Opens a all channels associated with a file mob.  This routine
 *		follows the locator, and may call the locator failure callback if
 *		the media can not be found.  If the failure callback finds the media,
 *		then this routine will return normally.
 *
 *		The media handle from this call can be used with
 *		omfmWriteDataSamples or omfmWriteMultiSamples but NOT with 
 *		or omfmWriteDataLines.
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NOMEMORY -- couldn't allocate memory for the media handle
 */
omfErr_t omfmMediaMultiOpen(
			omfHdl_t					file,			/* IN -- For this file */
			omfObject_t				masterMob,	/* IN -- In this master mob */
			omfTrackID_t			trackID,		/* IN -- On this track */
			omfMediaCriteria_t *mediaCrit,	/* IN -- using this media criteria */
			omfMediaOpenMode_t	openMode,	/* IN -- ReadOnly or Append */
			omfCompressEnable_t	compEnable,	/* IN -- optionally decompressing */
			omfMediaHdl_t			*mediaPtr)	/* OUT -- and return the media handle */
{
	omfPosition_t	zeroPos;
	omfFindSourceInfo_t	sourceInfo;
   omfEffectChoice_t effectChoice;

	XPROTECT(file)
	{
		
		omfsCvtInt32toPosition(0, zeroPos);	
      
      // MC 01-Dec-03 -- fix for crash
      effectChoice = kFindEffectSrc1;
		CHECK(omfiMobSearchSource(file, masterMob, trackID, zeroPos,kFileMob,
									mediaCrit, &effectChoice, NULL, &sourceInfo));

		CHECK(ompvtMediaMultiOpenFromFileMob(file, sourceInfo.mob, sourceInfo.mobTrackID, openMode, compEnable, mediaPtr));
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

omfErr_t ompvtMediaMultiOpenFromFileMob(
			omfHdl_t					file,			/* IN -- For this file */
			omfObject_t				fileMob,	/* IN -- In this master mob */
			omfTrackID_t			trackID,
			omfMediaOpenMode_t	openMode,	/* IN -- ReadOnly or Append */
			omfCompressEnable_t	compEnable,	/* IN -- optionally decompressing */
			omfMediaHdl_t			*mediaPtr)	/* OUT -- and return the media handle */
{
	omfMediaHdl_t   media;
	omfInt16           numVideo, numAudio, numOther = 0, n;
	omfUID_t			fileMobID;
	omfBool			isOMFI;
	omfPosition_t	numSamples;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	
	omfAssert(fileMob != NULL, file, OM_ERR_INVALID_FILE_MOB);
	omfAssert(mediaPtr != NULL, file, OM_ERR_NULL_PARAM);
	*mediaPtr = NULL;
	XPROTECT(file)
	{
		media = (omfMediaHdl_t) omOptMalloc(file, sizeof(struct omfiMedia));
		if(media == NULL)
			RAISE(OM_ERR_NOMEMORY);
	
		CHECK(InitMediaHandle(file, media, fileMob));
		media->compEnable = compEnable;
		if(openMode == kMediaOpenAppend)
			media->openType = kOmfiAppended;
		else
			media->openType = kOmfiOpened;
	
		CHECK(omfmMobGetMediaDescription(file, fileMob, &media->mdes));
		CHECK(codecGetNumChannels(file, fileMob, media->pictureKind,
											&numVideo));
		CHECK(codecGetNumChannels(file, fileMob, media->soundKind,
											&numAudio));

		media->numChannels = numVideo + numAudio + numOther;
		media->channels = (omfSubChannel_t *)
			omOptMalloc(file, sizeof(omfSubChannel_t) * media->numChannels);
		XASSERT((media->channels != NULL), OM_ERR_NOMEMORY);

		CHECK(LocateMediaFile(file, media->fileMob, &media->dataFile, &isOMFI));
		if(media->dataFile == NULL)
			RAISE(OM_ERR_MEDIA_NOT_FOUND);

		media->rawFile = media->dataFile->rawFile;			/* If non-omfi file */
		CHECK(omfmFindCodecForMedia(file, media->mdes, &media->pvt->codecInfo));
		if(isOMFI)
		{
			CHECK(omfsReadUID(file, fileMob, OMMOBJMobID, &fileMobID));
			/* MCX/NT Hack! */
			media->dataObj = (omfObject_t)omfsTableUIDLookupPtr(file->dataObjs, fileMobID);
			if((fileMobID.prefix == 444) && (media->dataObj == NULL))
			{
				fileMobID.prefix = 1;
				media->dataObj = (omfObject_t)omfsTableUIDLookupPtr(file->dataObjs, 
																fileMobID);
			}
		}
			
		for (n = 0; n < media->numChannels; n++)
		{
			if (n < numVideo)
			{
				media->channels[n].mediaKind = media->pictureKind;
				media->channels[n].physicalOutChan = n + 1;
				media->channels[n].trackID = trackID + n;
			} else if (n < numVideo + numAudio)
			{
				media->channels[n].mediaKind = media->soundKind;
				media->channels[n].physicalOutChan = n + 1 - (numVideo);
				media->channels[n].trackID = trackID + n;
			}

			omfsCvtInt32toPosition(0, media->channels[0].dataOffset);
			media->channels[n].dataOffset = media->dataStart;
		}
		CHECK(codecOpen(media));
		if(openMode == kMediaOpenAppend)
		{
			CHECK(omfmGetSampleCount(media, &numSamples));										  
			CHECK(omfsAddInt32toInt64(1, &numSamples));
			CHECK(omfmGotoFrameNumber(media, numSamples));
		}
	}
	XEXCEPT
	XEND
	
	*mediaPtr = media;
	return (OM_ERR_NONE);
}

/************************
 * Function: omfmIsMediaContiguous
 *
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
omfErr_t omfmIsMediaContiguous(
			omfMediaHdl_t	media,			/* IN -- Is this media */
			omfBool			*isContiguous)	/* OUT -- contiguous? */
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(isContiguous != NULL, media->mainFile, OM_ERR_NULL_PARAM);

	return(codecGetInfo(media, kMediaIsContiguous, media->nilKind,
								sizeof(omfBool), isContiguous));
}

/************************
 * Function: omfmAddVideoMemFormat
 *
 * 		Sets the video memory format and layout expected in memory.
 *		This is the format expected on writes and produced on reads.
 *
 *		On writes, the data will be written in this format, except
 *		where a software codec may be used.
 *		On reads, the data will be translated to this format.
 *
 * Argument Notes:
 *		The current CODECs should support rgb888 and YUV as formats
 *		and all of the standard layouts.  A special format of
 *		kVmFmtStd says to use the file's native format & layout.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmSetVideoMemFormat(
			omfMediaHdl_t		media,	/* IN -- For this media use */
			omfVideoMemOp_t	*op)
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);

	return (codecPutInfo(media, kVideoMemFormat, media->pictureKind,
								sizeof(omfVideoMemOp_t), op));
}

/************************
 * Function: omfmAddAudioMemFormat
 *
 * 		Sets the audio memory format expected in memory.
 *		This is the format expected on writes and produced on reads.
 *
 *		On writes, the data will be written in this format, except
 *		where a software codec may be used.
 *		On reads, the data will be translated to this format.
 *
 * Argument Notes:
 *		The current CODECs should support different sample sizes and rates
 *		A special format of kAmFmtStd says to use the file's native
 *		size and rate.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmSetAudioMemFormat(
			omfMediaHdl_t		media,		/* IN -- For this media use */
			omfAudioMemOp_t	*op)
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);

	return (codecPutInfo(media, kSetAudioMemFormat, media->soundKind,
								sizeof(op), op));
}

/************************
 * Function: omfmGetDisplayRect
 *
 * 		Returns the display rect for the current video media.
 *
 *		Handles the case where the displayed rectangle is not the same as
 *		the stored rectangle (as with the old leadingLines and
 *		trailingLines).
 *		A positive "leadingLines" (from 1.5) becomes a positive yOffset, and
 *		decreases the display height.
 *		A positive "trailingLines" (from 1.5) also decreases the display
 *		height.
 *
 * Argument Notes:
 * 	Takes a media handle, so the media must have been opened or
 *		created.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NULL_PARAM -- A NULL rectangle pointer.
 *		OM_ERR_INVALID_OP_CODEC -- This codec doesn't support display rect
 *									(may not be video media)
 */
omfErr_t omfmGetDisplayRect(
			omfMediaHdl_t	media,	/* IN -- For this media */
			omfRect_t		*result)	/* OUT -- Get the display rect */
{
	omfVideoMemOp_t	ops[2];

	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(result != NULL, media->mainFile, OM_ERR_NULL_PARAM);
	
	XPROTECT(media->mainFile)
	{
		ops[0].opcode = kOmfDisplayRect;
		ops[1].opcode = kOmfVFmtEnd;

		CHECK(codecGetVideoInfo(media, ops));
		*result = ops[0].operand.expRect;
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfmGetSampledRect
 *
 * 	Returns the sampled rect for the current video media.
 *
 * Argument Notes:
 * 	Takes a media handle, so the media must have been opened or created.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NULL_PARAM -- A NULL rectangle pointer.
 *		OM_ERR_INVALID_OP_CODEC -- This codec doesn't support display rect
 *									(may not be video media)
 */
omfErr_t omfmGetSampledRect(
			omfMediaHdl_t	media,		/* IN -- For this media */
			omfRect_t		*result)		/* OUT -- Get the sampled rect */
{
	omfVideoMemOp_t	ops[2];

	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(result != NULL, media->mainFile, OM_ERR_NULL_PARAM);
	
	XPROTECT(media->mainFile)
	{
		ops[0].opcode = kOmfSampledRect;
		ops[1].opcode = kOmfVFmtEnd;

		CHECK(codecGetVideoInfo(media, ops));
		*result = ops[0].operand.expRect;
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfmGetSampleCount
 *
 * 	Returns the number of samples on the given media stream.
 *		A video sample is one frame.
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
omfErr_t omfmGetSampleCount(
			omfMediaHdl_t media,			/* IN -- */
			omfLength_t		*result)	/* OUT -- */
{
	omfInt64		one;
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(result != NULL, media->mainFile, OM_ERR_NULL_PARAM);

	omfsCvtInt32toInt64(0, result);
	omfsCvtInt32toInt64(1, &one);
	
   if(omfsInt64Greater(media->pvt->repeatCount, one))
	{
	  *result = media->pvt->repeatCount;
	  return(OM_ERR_NONE);
	}

	omfsAddInt64toInt64(media->channels[0].numSamples, result);
		
	return (OM_ERR_NONE);
}

/************************
 * Function: omfmReadDataSamples
 *
 * 	Read a given number of samples from an opened media stream.  This
 *		call will only return a single channel of media from an interleaved
 *		stream.
 *
 * Argument Notes:
 *		A video sample is a frame.
 *		Buflen is in bytes, and should be large enough to hold the samples
 *		in the requested memory format.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmReadDataSamples(
			omfMediaHdl_t media,		/* IN -- */
			omfInt32			nSamples,	/* IN -- */
			omfInt32			buflen,		/* IN -- */
			void			*buffer,		/* IN/OUT -- */
			omfUInt32		*bytesRead)	/* OUT -- */
{
	omfmMultiXfer_t xfer;
	omfInt64		one;
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(buffer != NULL, media->mainFile, OM_ERR_NULL_PARAM);
	omfAssert(bytesRead != NULL, media->mainFile, OM_ERR_NULL_PARAM);

	omfsCvtInt32toInt64(1, &one);
	if(omfsInt64Greater(media->pvt->repeatCount, one))
	  omfmGotoShortFrameNumber(media, 1);

	XPROTECT(media->mainFile)
	{
		xfer.subTrackNum = media->physicalOutChanOpen;
		xfer.numSamples = nSamples;
		xfer.buflen = buflen;
		xfer.buffer = buffer;
		xfer.bytesXfered = 0;
	
		CHECK(codecReadBlocks(media, deinterleave, 1, &xfer));
	}
	XEXCEPT
	{
		*bytesRead = xfer.bytesXfered;
	}
	XEND
	
	*bytesRead = xfer.bytesXfered;

	return (OM_ERR_NONE);
}


/************************
 * Function: omfmReadMultiSamples
 *
 * 	Reads multiple channels worth of sample data from an interleaved
 *		data stream, in the natural order for the CODEC.
 *
 * Argument Notes:
 *		arrayElemCount is the size of the array or transfer operations.
 *		xferArray points to an array of transfer parameters.  All fields
 *		in this array except for bytesXferred must be set up before
 *		doing the transfer.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmReadMultiSamples(
			omfMediaHdl_t 		media,		/* IN -- */
			omfInt16					elemCount,	/* IN -- */
			omfmMultiXfer_t	*xferArray)	/* IN/OUT -- */
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(xferArray != NULL, media->mainFile, OM_ERR_NULL_PARAM);

	return (codecReadBlocks(media, deinterleave, elemCount, xferArray));
}

/************************
 * Function: omfmReadDataLines
 *
 * 	Reads single lines of video from a file.  This function allows reading
 *		video frames in pieces, for low-memory situations.  When enough lines
 *		have been read to constitute a frame, then the number of samples read
 *		be incremented by one.
 *		This function works only for video media.
 *
 * Argument Notes:
 *		The buffer must be large enough to hold an entire line of video.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADRWLINES -- This function only works for video media.
 */
omfErr_t omfmReadDataLines(
			omfMediaHdl_t media,	/* IN -- */
			omfInt32		nLines,		/* IN -- */
			omfInt32		bufLen,		/* IN -- */
			void		*buffer,		/* IN/OUT -- */
			omfInt32		*bytesRead)	/* OUT -- */
{
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	omfAssert(buffer != NULL, media->mainFile, OM_ERR_NULL_PARAM);

	return (codecReadLines(media, nLines, bufLen, buffer, bytesRead));
}

/************************
 * Function: omfmGotoShortFrameNumber
 *
 * 	The "seek" function for media.  Useful only on reading, you
 *		can't seek aound while writing media.  This version takes
 *		a 32-bit position.
 *
 *	See also
 *		omfmGotoFrameNumber takes a 64-bit position.
 *
 * Argument Notes:
 *		An audio frame is one sample across all open channels.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmGotoShortFrameNumber(
			omfMediaHdl_t media,		/* IN -- */
			omfInt32			frameNum)	/* IN -- */
{
	omfPosition_t          longFrameNum;

	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);

	omfsCvtInt32toPosition(frameNum, longFrameNum);
	return (codecSetFrameNum(media, longFrameNum));
}

/************************
 * Function: omfmGotoFrameNumber
 *
 * 	The "seek" function for media.  Useful only on reading, you
 *		can't seek aound while writing media.  This version takes
 *		a 64-bit position.
 *
 *	See also
 *		omfmGotoShortFrameNumber takes a 32-bit position.
 *
 * Argument Notes:
 *		An audio frame is one sample across all open channels.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmGotoFrameNumber(
			omfMediaHdl_t media,		/* IN -- */
			omfInt64		frameNum)	/* IN -- */
{
	omfInt64		one;
	omfErr_t		status;
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);

	omfsCvtInt32toInt64(1, &one);
	if(omfsInt64Greater(media->pvt->repeatCount, one))
		status = codecSetFrameNum(media, one);
	else
		status = codecSetFrameNum(media, frameNum);
	return (status);
}

/************************
 * Function: omfmGetFrameOffset
 *
 * 	The "tell" function for media.  Useful only on reading, you
 *		can't find position while writing media.  This version takes
 *		a 64-bit position and returns a 64-bit offset.
 *
 *
 * Argument Notes:
 *		An audio frame is one sample across all open channels.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmGetFrameOffset(
			omfMediaHdl_t media,		/* IN -- */
			omfInt64		frameNum,	/* IN -- */
			omfInt64      *frameOffset)	/* OUT - */
{
	omfInt64		one;
	omfErr_t		status;
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);

	omfsCvtInt32toInt64(1, &one);
	if(omfsInt64Greater(media->pvt->repeatCount, one))
		status = codecGetFrameOffset(media, one, frameOffset);
	else
		status = codecGetFrameOffset(media, frameNum, frameOffset);
	return (status);
}



/************************
 * Function: omfmGetVideoInfo
 *
 * 	Get video-related information about a given piece of media data.
 *
 * Argument Notes:
 *		Any parameters not required may have the pointers set to NULL.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_WRONG_MEDIATYPE -- Not video media.
 */
omfErr_t omfmGetVideoInfo(
			omfMediaHdl_t		media,
			omfFrameLayout_t	*layout,
			omfInt32				*fieldWidth,
			omfInt32				*fieldHeight,
			omfRational_t		*editrate,
			omfInt16				*bitsPerPixel,
			omfPixelFormat_t	*defaultMemFmt)
{
	omfFrameLayout_t	localLayout;
	omfInt16					localBitsPerPixel;
	omfInt32 localWidth, localHeight;
	omfRational_t		localEditrate;
	omfPixelFormat_t	localMemFmt;
	omfVideoMemOp_t		opList[6];

	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	if(layout == NULL)
		layout = &localLayout;
	if(fieldWidth == NULL)
		fieldWidth = &localWidth;
	if(fieldHeight == NULL)
		fieldHeight = &localHeight;
	if(editrate == NULL)
		editrate = &localEditrate;
	if(defaultMemFmt == NULL)
		defaultMemFmt = &localMemFmt;
	if(bitsPerPixel == NULL)
		bitsPerPixel = &localBitsPerPixel;

	XPROTECT(media->mainFile)
	{
		if(media->mainFile->setrev == kOmfRev2x)
		{
			CHECK(omfsReadRational(media->mainFile, media->mdes,
											OMMDFLSampleRate, editrate));
		}
		else
		{
			CHECK(omfsReadExactEditRate(media->mainFile, media->mdes,
												OMMDFLSampleRate, editrate));
		}
	
		opList[0].opcode = kOmfFrameLayout;
		opList[1].opcode = kOmfStoredRect;
		opList[2].opcode = kOmfPixelSize;
		opList[3].opcode = kOmfPixelFormat;
		opList[4].opcode = kOmfVFmtEnd;
		CHECK(codecGetVideoInfo(media, opList));
		*layout = opList[0].operand.expFrameLayout;
		*fieldWidth = opList[1].operand.expRect.xSize;
		*fieldHeight = opList[1].operand.expRect.ySize;
		*bitsPerPixel = opList[2].operand.expInt16;
		*defaultMemFmt = opList[3].operand.expPixelFormat;
	}
	XEXCEPT
	{
		if(XCODE() == OM_ERR_INVALID_OP_CODEC)
			RERAISE(OM_ERR_WRONG_MEDIATYPE);
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
omfErr_t omfmGetVideoInfoArray(
			omfMediaHdl_t	media,
			omfVideoMemOp_t *ops)
	{
		return(codecGetVideoInfo(media, ops));
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
omfErr_t omfmPutVideoInfoArray(
			omfMediaHdl_t	media,
			omfVideoMemOp_t *ops)
	{
		return(codecPutVideoInfo(media, ops));
	}
	

/************************************************************************
 *
 * Multiple media representations
 *
 * These functions are used when multiple representations may exist for
 *	a given piece of media.  This would happen if media was created in
 *	both AIFC and WAVE format to be easily translatable, or if both a high
 * and low quality compression is available.
 *
 ************************************************************************/


/************************
 * Function: omfmGetNumRepresentations
 *
 * 	Returns the number of media representations available for a given
 *		trackID on a given master mob.  This call is meant to work with
 *		omfmGetRepresentationSourceClip(), if you want to iterate through
 *		all of the choices yourself.
 *
 * See Also:
 *		A function called omfmGetCriteriaSourceClip() exist which selects
 *		a representation for you based upon supplied criteria.
 *
 * Argument Notes:
 *		trackID - A 2.0 track ID, unique for all slots in the Mob.  When
 *			writing a 1.x era file, the toolkit will map this into a 1.x
 *			trackNum in the file.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NOT_IN_15 - This functionality did not exist in rev 1.5.
 */
omfErr_t omfmGetNumRepresentations(
			omfHdl_t 		file,			/* IN -- */
			omfObject_t 	masterMob,	/* IN -- */
			omfTrackID_t	trackID,		/* IN -- */
			omfInt32				*numReps)	/* OUT -- */
{
	omfMSlotObj_t	track;
   	omfSegObj_t		segment;
	omfErr_t		status;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	omfAssert(numReps != NULL, file, OM_ERR_NULL_PARAM);

	omfAssertValidFHdl(file);
	*numReps = 0;
	
	XPROTECT(file)
	{
		CHECK(FindTrackByID( file, masterMob, trackID, &track));
		CHECK(omfiTrackGetInfo(file, masterMob, track, NULL,
						0, NULL, NULL, NULL, &segment));
		if(omfsIsTypeOf(file, segment, OMClassMGRP, &status))
			*numReps = omfsLengthObjRefArray(file, segment, OMMGRPChoices);
		else
			*numReps = 1;
	}
	XEXCEPT
	XEND

	return(OM_ERR_NONE);
}

/************************
 * Function: omfmGetRepresentationSourceClip
 *
 * 	Given a master mob, a track ID and an index, return the indexed
 *		media representation.  This call is meant to work with
 *		omfmGetNumRepresentations(), if you want to iterate through
 *		all of the choices yourself.
 *
 * Argument Notes:
 *		Index is in the range of 1 <= index <= numRepresentations.
 *		trackID - A 2.0 track ID, unique for all slots in the Mob.  When
 *			writing a 1.x era file, the toolkit will map this into a 1.x
 *			trackNum in the file.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NOT_IN_15 - This functionality did not exist in rev 1.5.
 */
omfErr_t omfmGetRepresentationSourceClip(
			omfHdl_t			file,			/* IN -- */
			omfObject_t 	masterMob,	/* IN -- */
			omfTrackID_t	trackID,		/* IN -- */
			omfInt32				index,		/* IN -- */
			omfObject_t		*sourceClip)	/* OUT -- */
{
	omfMSlotObj_t	track;
  	omfSegObj_t		segment;
	omfErr_t		status;
	omfNumSlots_t	numReps;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	omfAssert(sourceClip != NULL, file, OM_ERR_NULL_PARAM);
	*sourceClip = NULL;
		
	XPROTECT(file)
	{
		CHECK(FindTrackByID( file, masterMob, trackID, &track));
		CHECK(omfiTrackGetInfo(file, masterMob, track, NULL,
						0, NULL, NULL, NULL, &segment));
		if(omfsIsTypeOf(file, segment, OMClassMGRP, &status))
		{
			numReps = omfsLengthObjRefArray(file, segment, OMMGRPChoices);
			if(index >= 1 && index <= numReps)
			{
				CHECK(omfsGetNthObjRefArray(file, segment, OMMGRPChoices,
													sourceClip, index));
			}
			else
				RAISE(OM_ERR_MISSING_MEDIA_REP);
		}
		else
		  {
			 *sourceClip = segment;
		  }
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmGetCriteriaSourceClip
 *
 * 	Given a master mob and a track ID, return the file mob for
 *		the media which best fits the selection criteria.
 *
 * Argument Notes:
 *
 *		Available criteria are:
 *			kOmfAnyRepresentation,
 *			kOmfFastestRepresentation,
 *			kOmfBestFidelityRepresentation,
 *			kOmfSmallestRepresentation,
 *			kOmfUseRepresentationProc
 *		If the criteria is kOmfUseRepresentationProc, then a callback
 *		is called for every representation, and the best return calue
 *		is selected.  Otherwise, the proc field is not used.
 *
 *		trackID - A 2.0 track ID, unique for all slots in the Mob.  When
 *			writing a 1.x era file, the toolkit will map this into a 1.x
 *			trackNum in the file.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NOT_IN_15 - This functionality did not exist in rev 1.5.
 */
omfErr_t omfmGetCriteriaSourceClip(
			omfHdl_t file,	/* IN -- */
			omfObject_t masterMob,	/* IN -- */
			omfTrackID_t	trackID,	/* IN -- */
			omfMediaCriteria_t *criteria,		/* IN -- */
			omfObject_t		*retSrcClip)	/* OUT -- */
{
	omfInt32				n, numReps, score, highestScore;
	omfBool				more, isOMFI;
	omfHdl_t				dataFile;
	omfObject_t			mdes, fileMob, highestScoreSourceClip, sourceClip;
	omfClassID_t		mdesTag;
	omTableIterate_t	iter;
	codecTable_t		*codecPtr;
	omfCodecID_t		codecID;
	omfCodecSelectInfo_t selectInfo;
	omfCodecMetaInfo_t	meta;
	omfProperty_t		idProp;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	omfAssert(criteria != NULL, file, OM_ERR_NULL_PARAM);
	omfAssert(retSrcClip != NULL, file, OM_ERR_NULL_PARAM);
	highestScore = 0;
	highestScoreSourceClip = NULL;
	*retSrcClip = NULL;
		
	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		CHECK(omfmGetNumRepresentations(file, masterMob, trackID,&numReps));
		for(n = 1; n <= numReps; n++)
		{
			CHECK(omfmGetRepresentationSourceClip(file, masterMob, trackID, n,
															&sourceClip));
			if(numReps == 1)
			{
				highestScoreSourceClip = sourceClip;
				break;
			}
			CHECK(omfiSourceClipResolveRef(file, sourceClip, &fileMob));
				
			CHECK(omfsReadObjRef(file, fileMob, OMSMOBMediaDescription, &mdes));
			codecID = NULL;
	
			CHECK(omfsReadClassID(file, mdes, idProp, mdesTag));
			CHECK(omfsTableFirstEntryMatching(file->session->codecMDES, &iter, 
														 mdesTag, &more));
			while(more && (codecID == NULL))
			{
				codecPtr = (codecTable_t *)iter.valuePtr;
				CHECK(codecGetSelectInfo(file, codecPtr, mdes, &selectInfo));
				if(selectInfo.willHandleMDES)
				{
					CHECK(codecGetMetaInfo(file->session, codecPtr, NULL, NULL, 0,
													&meta));
					codecID = meta.codecID;
				}
			
				CHECK(omfsTableNextEntry(&iter, &more));
			}
			if(codecID == NULL)
				continue;
				
			/* Check for locator file existance & continue if not present
			 * A file which is supposed to be an OMFI file must be opened
			 * to check for the existance of the data object, so we must
			 * open the file here.
			 */
			CHECK(LocateMediaFile(file, fileMob, &dataFile, &isOMFI));
			if(dataFile == NULL)
				continue;
			if(dataFile != file)
				omfsCloseFile(dataFile);

			score = 0;
			switch(criteria->type)
			{
			case kOmfAnyRepresentation:
				break;
				
			case kOmfFastestRepresentation:
				if(selectInfo.hwAssisted)
					score += 10;
				if(selectInfo.isNative)
					score += 10;
				break;
				
			case kOmfBestFidelityRepresentation:
				score = (100 - selectInfo.relativeLoss);
				break;
				
			case kOmfSmallestRepresentation:
				score = -1 * selectInfo.avgBitsPerSec;
				break;
				
			case kOmfUseRepresentationProc:
				score = (*criteria->proc)(file, mdes, codecID);
				break;
			}
	
			if((score > highestScore) || (highestScoreSourceClip == NULL))
			{
				highestScore = score;
				highestScoreSourceClip = sourceClip;
			}
		}
	}
	XEXCEPT
	XEND
		
	*retSrcClip = highestScoreSourceClip;
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmGetAudioInfo
 *
 * 	Get audio-related information about a given piece of media data.
 *
 * Argument Notes:
 *		Any parameters not required may have the pointers set to NULL.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_WRONG_MEDIATYPE -- Not video media.
 */
omfErr_t omfmGetAudioInfo(
			omfMediaHdl_t	media,
			omfRational_t	*rate,
			omfInt32				*sampleSize,
			omfInt32				*numChannels)
{
	omfHdl_t        		file;
	omfRational_t			localRate;
	omfInt32						localSampleSize;
	omfInt32						localNumChannels;
	omfAudioMemOp_t		opList[3];
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	if(rate == NULL)
		rate = &localRate;
	if(sampleSize == NULL)
		sampleSize = &localSampleSize;
	if(numChannels == NULL)
		numChannels = &localNumChannels;

	file = media->mainFile;
	XPROTECT(file)
	{
		if(file->setrev == kOmfRev2x)
		{
			CHECK(omfsReadRational(file, media->mdes, OMMDFLSampleRate, rate));
		}
		else
		{
			CHECK(omfsReadExactEditRate(file, media->mdes, OMMDFLSampleRate, rate));
		}

		opList[0].opcode = kOmfSampleSize;
		opList[1].opcode = kOmfNumChannels;
		opList[2].opcode = kOmfAFmtEnd;
		CHECK(codecGetAudioInfo(media, opList));
		*sampleSize = opList[0].operand.sampleSize;
		*numChannels = opList[1].operand.numChannels;
	}
	XEXCEPT
	{
		if(XCODE() == OM_ERR_INVALID_OP_CODEC)
			RERAISE(OM_ERR_WRONG_MEDIATYPE);
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
omfErr_t omfmGetAudioInfoArray(
			omfMediaHdl_t	media,
			omfAudioMemOp_t *ops)
	{
		return(codecGetAudioInfo(media, ops));
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
omfErr_t omfmPutAudioInfoArray(
			omfMediaHdl_t	media,
			omfAudioMemOp_t *ops)
	{
		return(codecPutAudioInfo(media, ops));
	}
	
/************************
 * name
 *
 * 		WhatIt Does
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
omfErr_t omfmGetMediaFilePos(
			omfMediaHdl_t	media,
			omfInt64			*offset)	/* OUT - return file position through here */
{
	return(omcGetStreamFilePos(media->stream, offset));
}

/************************
 * omfmGetNumMediaSegments
 *
 * 		WhatIt Does
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
omfErr_t omfmGetNumMediaSegments(
			omfMediaHdl_t	media,
			omfInt32		*numSegments)	/* OUT - return file position through here */
{
	return(omcGetStreamNumSegments(media->stream, numSegments));
}

/************************
 * omfmGetMediaSegmentInfo
 *
 * 		WhatIt Does
 *
 * Argument Notes:
 *		if the start of the data is in the indexed segment, then
 *			dataPos returns the start of the data in this segment (startPos + offset).
 *		if the start of the data is in an ealier segment, then
 *			dataPos returns dataPos = startPos.
 *		If the start of the data is in a higher indexed segment (very unlikely)
 *			then dataPos returns a zero position.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmGetMediaSegmentInfo(
			omfMediaHdl_t	media,
			omfInt32		index,			/* IN -- for this segment */
			omfPosition_t	*startPos,		/* OUT -- where does the segment begin, */
			omfLength_t		*length,		/* OUT -- and how int  is it? */
			omfPosition_t	*dataPos,	/* OUT -- Where does media data begin */
			omfLength_t		*dataLen)	/* OUT -- and how int  is the data? */
{
	omfPosition_t	strmStart, strmEnd, initialPos, offset, segPos, localPos;
	omfLength_t		initialLen, segLen, localLen;
	omfInt32		n;
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	
	XPROTECT(media->mainFile)
	{
		CHECK(omcGetStreamSegmentInfo(media->stream, index, &initialPos, &initialLen));
		if(startPos != NULL)
			*startPos = initialPos;
		if(length != NULL)
			*length = initialLen;
		if(dataPos != NULL || dataLen != NULL)
		{
			if(dataPos == NULL)
				dataPos = &localPos;
			if(dataLen == NULL)
				dataLen = &localLen;
			*dataPos = initialPos;
			*dataLen = initialLen;
			if((index == 1) && (omfsInt64Less(media->dataStart, initialLen)))
			{
				omfsAddInt64toInt64(media->dataStart, dataPos);
				omfsSubInt64fromInt64(media->dataStart, dataLen);
			}
			else
			{
				omfsCvtInt32toInt64(0, dataPos);
				omfsCvtInt32toInt64(0, &strmStart);
				strmEnd = initialLen;
				for(n = 1; n <= index; n++)
				{
					CHECK(omcGetStreamSegmentInfo(media->stream, index, &segPos, &segLen));
					if(omfsInt64GreaterEqual(media->dataStart, strmStart) &&
						omfsInt64Less(media->dataStart, strmEnd))
					{
						*dataPos = segPos;
						if(n == index)
						{
							offset = media->dataStart;
							omfsSubInt64fromInt64(strmStart, &offset);
							omfsAddInt64toInt64(offset, dataPos);
							omfsSubInt64fromInt64(offset, dataLen);
						}
					}
					omfsAddInt64toInt64(segLen, &strmStart);
					omfsAddInt64toInt64(segLen, &strmEnd);
				}
			}
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmIsHardwareAssistedCodec
 *
 * 	Tells if the given codec is hardware-assisted.  That is, does
 *		it contain calls to a particular hardware device which speeds
 *		up transfer and compression/decompression.
 *
 *		If the hardware is not present in the system, this call should
 *		return FALSE.
 *
 *		If the parameters in the media descriptor are out-of-bounds
 *		for the hardware, then this call will return FALSE.
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
omfErr_t omfmIsHardwareAssistedCodec(
			omfHdl_t			file,
			omfCodecID_t	codecID,
			omfObject_t		mdes,
			omfBool			*result)
{
	omfCodecSelectInfo_t	select;
	codecTable_t			codec_data;
	omfBool					found;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	omfAssert(result != NULL, file, OM_ERR_NULL_PARAM);

	*result = FALSE;
	XPROTECT(file)
	{
		omfsTableLookupBlock(file->session->codecID, codecID,
									sizeof(codec_data), &codec_data, &found);
		if(!found)
			RAISE(OM_ERR_CODEC_INVALID);
	
		CHECK(codecGetSelectInfo(file, &codec_data, mdes, &select));
		*result = select.hwAssisted;
	}
	XEXCEPT
	{
		if(XCODE() == OM_ERR_INVALID_OP_CODEC)
			NO_PROPAGATE();
	}
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfmCodecGetName
 *
 * 	Returns the full name of the given codec, expanded for
 *		human consumption.  No other call uses this name, so it
 *		may be fully descriptive, esp. of limitations.
 *
 * Argument Notes:
 *		The name will be truncated to fit within "buflen" bytes.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_CODEC_INVALID - The given codec ID is not loaded.
 */
omfErr_t omfmCodecGetName(
			omfHdl_t			file,
			omfCodecID_t	codecID,
			omfInt32			namelen,
			char				*name)
{
	omfCodecMetaInfo_t	meta;
	codecTable_t		codec_data;
	omfBool				found;
	omfSessionHdl_t	session;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	omfAssert(name != NULL, file, OM_ERR_NULL_PARAM);
	session = file->session;
	
	XPROTECT(file)
	{
		omfsTableLookupBlock(session->codecID, codecID, sizeof(codec_data),
									&codec_data, &found);
		if(!found)
			RAISE(OM_ERR_CODEC_INVALID);
		
		CHECK(codecGetMetaInfo(session, &codec_data, NULL, name, namelen, &meta));
	}
	XEXCEPT
	{
		if(XCODE() == OM_ERR_INVALID_OP_CODEC)
		{
			strncpy(name, "<none supplied>", namelen);
			NO_PROPAGATE();
		}
	}
	XEND
	
	return (OM_ERR_NONE);
}		

/************************
 * Function: omfmMediaGetCodecID
 *
 * 	Returns the codec ID being used to handle the specified media.
 *		This will be required in order to send private data to the codec.
 *
 * Argument Notes:
 *		The name will be truncated to fit within "buflen" bytes.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMediaGetCodecID(
			omfMediaHdl_t	media,
			omfCodecID_t	*codecID)
{
	omfCodecMetaInfo_t	meta;
	omfSessionHdl_t	session;
	
	omfAssertMediaHdl(media);
	omfAssertValidFHdl(media->mainFile);
	omfAssertMediaInitComplete(media->mainFile);
	session = media->mainFile->session;
	
	XPROTECT(media->mainFile)
	{
		CHECK(codecGetMetaInfo(session, &media->pvt->codecInfo, NULL, NULL, 0, &meta));
		*codecID = meta.codecID;
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}		

/************************
 * Function: omfmImportRawRef
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
omfErr_t omfmImportRawRef(omfHdl_t main, omfObject_t fileMob, fileHandleType fh, omfInt16 fhSize)
{
	omfHdl_t	rawFile;
	
	XPROTECT(main)
	{
		CHECK(omfsOpenRawFile(fh, fhSize, main->session, &rawFile));
		CHECK(codecImportRaw(main, fileMob, rawFile));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmGetNumCodecs
 *
 * 		WhatItDoes
 *
 * Argument Notes:
 *		mediaKind - Find codecs which match this media kind
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmGetNumCodecs(omfSessionHdl_t sess,
 			omfString		dataKindString,
			omfFileRev_t	rev,
			omfInt32		*numCodecPtr)
{
	return(codecGetNumCodecs(sess, dataKindString, rev, numCodecPtr));
}

/************************
 * Function: omfmGetIndexedCodecID
 *
 * 		WhatItDoes
 *
 * Argument Notes:
 *		mediaKind - Find codecs which match this media kind
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmGetIndexedCodecInfo(omfSessionHdl_t sess,
 						omfString		dataKindString,
						omfFileRev_t	rev,
			 			omfInt32  		index,		/* IN -- At this index */
			 			omfInt32  		idLen,		/* IN -- Into a buffer of this size */
			 			omfCodecID_t  	codecid,	/* OUT -- return the codec ID */
			 			omfInt32  		nameLen,	/* IN -- Into a buffer of this size */
			 			char		  	*codecName	/* OUT -- return the codec name */
						)
{
	char				*idPtr;
	omfCodecMetaInfo_t	meta;
	codecTable_t		codec_data;
	omfBool				found;
	
	XPROTECT(NULL)
	{
		CHECK(codecGetIndexedCodecID(sess, dataKindString, rev, index, &idPtr));
		if((codecid != NULL) && (idLen != 0))
		{
			strncpy(codecid, idPtr, idLen);
			codecid[idLen-1] = '\0';
		}

		if((codecName != NULL) && (nameLen != 0))
		{
			omfsTableLookupBlock(sess->codecID, idPtr, sizeof(codec_data),
									&codec_data, &found);
			if(!found)
				RAISE(OM_ERR_CODEC_INVALID);
			
			CHECK(codecGetMetaInfo(sess, &codec_data, NULL, codecName, nameLen, &meta));
		}
	}
	XEXCEPT
	{
		if(XCODE() == OM_ERR_INVALID_OP_CODEC)
		{
			strncpy(codecName, "<none supplied>", nameLen);
			codecName[nameLen-1] = '\0';
			NO_PROPAGATE();
		}
	}
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfmGetNumCodecVarieties
 *
 * 		WhatItDoes
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmGetNumCodecVarieties(omfSessionHdl_t	sess,
								omfCodecID_t	codecID,
								omfFileRev_t    rev,
								char			*mediaKindString,
								omfInt32		*numVarietyPtr)
{
	XPROTECT(NULL)
	{
		CHECK(codecGetNumCodecVarieties(sess, codecID, rev, mediaKindString, numVarietyPtr));
	}
	XEXCEPT
	{
		if(XCODE() == OM_ERR_INVALID_OP_CODEC)
		{
			*numVarietyPtr = 0;
			NO_PROPAGATE();
		}
	}
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: codecGetIndexedCodecVariety
 *
 * 		WhatItDoes
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmGetIndexedCodecVariety(omfSessionHdl_t sess,
						omfCodecID_t	codecID,
						omfFileRev_t    rev,
						char			*mediaKindString,
			 			omfInt32  		index,		/* IN -- At this index */
			 			omfInt32  		bufLen,	/* IN -- Into a buffer of this size */
			 			char		  	*varietyName	/* OUT -- return the variety name */
						)
{
	char				*varietyPtr;
	
	XPROTECT(NULL)
	{
		CHECK(codecGetIndexedCodecVariety(sess, codecID, rev, mediaKindString, index, &varietyPtr));
		if((varietyName != NULL) && (bufLen != 0))
		{
			strncpy(varietyName, varietyPtr, bufLen);
			varietyName[bufLen-1] = '\0';
		}
	}
	XEXCEPT
	{
		if(XCODE() == OM_ERR_INVALID_OP_CODEC)
		{
			strncpy(varietyName, "Standard", bufLen);
			varietyName[bufLen-1] = '\0';
			NO_PROPAGATE();
		}
	}
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: codecAddFrameIndexEntry
 *
 * 		Adds a frame index entry for media which was added precompressed.
 *		This function should NOT be called when media is passed to the toolkit
 *		in an uncompressed format.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_INVALID_OP_CODEC -- This kind of media doesn't have a frame index
 *		OM_ERR_MEDIA_OPENMODE -- The media is open for read-only.
 */
omfErr_t omfmAddFrameIndexEntry(
			omfMediaHdl_t	media,			/* IN -- To this media*/
			omfInt64		frameOffset)	/* IN -- add a frame offset to it's frame index*/
{
	return(codecAddFrameIndexEntry(media, frameOffset));
}

/************************************************************************
 *
 * Codec stream functions
 *
 ************************************************************************/



/************************************************************************
 *
 * Internal support functions
 *
 ************************************************************************/

/************************
 * Function: InitMediaHandle (INTERNAL)
 *
 * 		Initialize an entire media handle structure so that fields are
 *			not left with random values.
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
static omfErr_t InitMediaHandle(
			omfHdl_t			file,
			omfMediaHdl_t	media,
			omfObject_t		fileMob)
{
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);

	XPROTECT(file)
	{
		media->masterMob = NULL;
		media->fileMob = NULL;
		media->mdes = NULL;
		media->dataObj = NULL;
		media->userData = NULL;
		media->numChannels = 0;
		omfsCvtInt32toPosition(0, media->dataStart);
		media->codecVariety = NULL;
		media->compEnable = kToolkitCompressionEnable;
		media->dataFile = NULL;
		media->mainFile = file;
		media->cookie = MEDIA_COOKIE;
		media->openType = kOmfiNone;
		media->channels = NULL;
		media->fileMob = fileMob;
		media->nilKind = file->nilKind;
		media->pictureKind = file->pictureKind;
		media->soundKind = file->soundKind;
		media->rawFile = NULL;			/* If non-omfi file */
		media->physicalOutChanOpen = 0;
		media->pvt = (omfMediaPvt_t *)
				omOptMalloc(file, sizeof(omfMediaPvt_t));
		media->pvt->codecInfo.rev = 0;
		omfsCvtInt32toInt64(1, &media->pvt->repeatCount);
		media->stream = (omfCodecStream_t *)
				omOptMalloc(file, sizeof(omfCodecStream_t));
		if(media->stream == NULL)
			RAISE(OM_ERR_NOMEMORY);
                media->stream->cookie = 0;
		
#ifdef OMFI_ENABLE_STREAM_CACHE
		media->stream->cachePtr = NULL;
#endif
		media->stream->procData = NULL;
		
		media->pvt->nextMedia = file->topMedia;
		file->topMedia = media;
	
		file->closeMediaProc = omfmMediaClose;
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: TrialOpenFile (INTERNAL)
 *
 * 	Open the file referenced by the given locator.  If an exception
 *		happens while opening the file, close the file handle and DON'T
 *		propagate the exception to the caller. 
 *
 * Argument Notes:
 *		refFile - The file holding the composition which has the locator.
 *		fileMobUid - This is required to find the correct media data object
 *			if the file is a Bento file.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfErr_t TrialOpenFile(
			omfHdl_t			refFile,		/* IN -- */
			omfObject_t		aLoc,			/* IN -- */
			omfUID_t			fileMobUid,	/* IN -- */
			omfFileFormat_t fmt,			/* IN -- */
			omfHdl_t			*dataFile,	/* OUT -- */
			omfBool			*found)		/* OUT -- */
{
	omfAssert(found != NULL, refFile, OM_ERR_NULL_PARAM);
	if (found)
	  *found = FALSE;
	if (dataFile)
	  *dataFile = NULL;

	XPROTECT(refFile)
	{
		CHECK(openLocator(refFile, aLoc, fmt, dataFile));
		if(omfmIsMediaDataPresent(*dataFile, fileMobUid, fmt))
			*found = TRUE;
	}
	XEXCEPT
	{
		if ((*dataFile != NULL) && (*dataFile != refFile))
			(void) omfsCloseFile(*dataFile);
		NO_PROPAGATE();
	}
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: LocateMediaFile (INTERNAL)
 *
 * 	Given a file handle and an initialized media handle, attempt
 *		to locate the media data, following all locators until the data
 *		is found.
 *
 *		This function sets up the dataFile pointer, and selects the codec
 *		but does NOT call codec open.
 *		If the media is not found by following a locator,
 *		the local file is searched for the media.  If the media is still
 *		not found, then the locatorFailureCallback routine is called.
 *			omfHdl_t FailureCallback(omfHdl_t file, omfObject_t mdes);
 *
 *		The locator failure callback should attempt to open the file by
 *		other means (such as putting up a dialog and asking the user).
 *		If the file is found by the callback, it should open the file
 *		and return the file handle (omfHdl_t).  If a valid file handle
 *		is returned from the callback, then the open will continue as
 *		if nothing happened, and no error will be returned from this
 *		function.
 *
 *		The locator failure callback is set by the function
 *			omfmLocatorFailureCallback().
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
omfErr_t     LocateMediaFile(
			omfHdl_t			file,		/* IN */
			omfObject_t		fileMob,	/* IN */
			omfHdl_t			*dataFile,
			omfBool			*isOMFI)
{
	omfUID_t        uid;
	omfFileFormat_t fmt;
	omfInt32           index, numItems;
	omfBool         found;
	omfObject_t     aLoc, mdes;
	omfHdl_t        refFile;
	omfErr_t			status;
	
	omfAssertValidFHdl(file);
	refFile = file;
	XPROTECT(file)
	{
		CHECK(omfmMobGetMediaDescription(file, fileMob, &mdes));
		CHECK(omfsReadUID(file, fileMob, OMMOBJMobID, &uid));
	
		found = FALSE;
		/* MCX/NT Hack! */
		if(omfsTableIncludesUID(file->dataObjs, uid))
		{
			*dataFile = file;
			*isOMFI = TRUE;
			found = omfmIsMediaDataPresent(file, uid, kOmfiMedia);
		}
		else if(uid.prefix == 444)
		{
			uid.prefix = 1;
			*dataFile = file;
			*isOMFI = TRUE;
			found = omfmIsMediaDataPresent(file, uid, kOmfiMedia);
		}
		
		if(!found)
		{
			status = omfsReadBoolean(file, mdes, OMMDFLIsOMFI, isOMFI);
			if(uid.prefix != 1)
 			{
 				fmt = (*isOMFI ? kOmfiMedia : kForeignMedia);
 				CHECK(status);
 			}
 			else
 			{
 				fmt = kOmfiMedia;			/* Hack for MCX/NT */
 			}
			numItems = omfsLengthObjRefArray(file, mdes, OMMDESLocator);
			for (index = 1; (index <= numItems) && !found; index++)
			{
				CHECK(omfsGetNthObjRefArray(file, mdes, OMMDESLocator,
														&aLoc, index));
				CHECK(TrialOpenFile(refFile, aLoc, uid, fmt, dataFile, &found));

				/* Release Bento reference, so the useCount is decremented */
				CMReleaseObject((CMObject)aLoc);
			}
		}
	
		if (!found &&
			(*dataFile == NULL) &&
			(file->locatorFailureCallback != NULL))
		{
			refFile = (*file->locatorFailureCallback) (file, mdes);
			if (refFile != NULL)
			{
				if(omfmIsMediaDataPresent(refFile, uid, fmt))
				  {
					 *dataFile = refFile;
					 found = TRUE;
				  }
				else
					*dataFile = NULL;
			}
		}
	}
	XEXCEPT
	{
		*dataFile = NULL;
	}
	XEND
	
	if(!found)
		*dataFile = NULL;
		
	return (OM_ERR_NONE);
}


/************************
 * Function: SetupCodecInfo (INTERNAL)
 *
 * 		Attempt to open media in the current file.  This involves
 *		searching for the correct mobID in an OMFI file.  The result
 *		is then passed to the correct codec (based upon the media
 *		descriptor) which determines if the media is valid.
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
omfBool omfmIsMediaDataPresent(
									omfHdl_t				file,	/* IN -- */
									omfUID_t				fileMobUid,	/* IN -- */
									omfFileFormat_t	fmt)
{
	omfObject_t	obj;
	omfBool		result;
	
	omfAssertValidFHdl(file);
	result = FALSE;
	
	if (fmt == kOmfiMedia)
	  {
		 obj = (omfObject_t)omfsTableUIDLookupPtr(file->dataObjs, fileMobUid);
		 if(obj != NULL)
			result = TRUE;
	  }
	else
	  result = TRUE;
	
	return (result);
}

/************************
 * Function: omfsReconcileMasterMobLength (INTERNAL)
 *
 * 	Called from omfsReconcileMobLength to handle the master mob case.
 *		Given a master mob, make sure that all fields which contain the
 *		length of the mob are in agreement.  Currently only makes sure
 *		that mob length references are >= each of the track lengths.
 *
 *		Since 2.0 does not contain a redundant mob length field, this
 *		function simply returns on 2.x era files.
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
static omfErr_t omfsReconcileMasterMobLength(
			omfHdl_t		file,
			omfObject_t mob)
{
	omfObject_t		trak, sclp, fileMob;
	omfInt32			fileMobLen, index, numTracks, masterLen, newLen, loop;
	omfInt32			numSlots, fileNumSlots;
	omfPosition_t	endPos;
	omfRational_t	fileRate, masterRate;
	omfIterHdl_t	slotIter = NULL, fileSlotIter = NULL;
	omfMSlotObj_t	fileSlot, slot;
	omfSegObj_t		fileSeg, seg;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);

	XPROTECT(file)
	{
		switch(file->setrev)
		{
		case kOmfRev1x:
		case kOmfRevIMA:
			if(omfsIsPropPresent(file, mob, OMTRKGGroupLength, OMInt32))
			{
				CHECK(omfsReadInt32(file, mob, OMTRKGGroupLength, &masterLen));
			}
			else
				masterLen = 0;
	
			numTracks = omfsLengthObjRefArray(file, mob, OMTRKGTracks);
			for(index = 1; index <= numTracks; index++)
			{
				CHECK(omfsGetNthObjRefArray(file, mob, OMTRKGTracks, &trak,
													index));
				CHECK(omfsReadObjRef(file, trak, OMTRAKTrackComponent, &sclp));
				CHECK(omfiSourceClipResolveRef(file, sclp, &fileMob));
				CHECK(omfsReadInt32(file, fileMob, OMTRKGGroupLength,
										&fileMobLen));

				CHECK(omfsCvtInt32toInt64(fileMobLen, &endPos));
				CHECK(omfsReadExactEditRate(file, fileMob, OMCPNTEditRate, &fileRate));
				CHECK(omfsReadExactEditRate(file, mob, OMCPNTEditRate, &masterRate));
				if((fileRate.numerator == masterRate.numerator) &&
				   (fileRate.denominator == masterRate.denominator))
				{
					newLen = fileMobLen;
				}
				else
				{
					CHECK(omfiConvertEditRate(	fileRate, endPos, masterRate, kRoundFloor, &endPos));
					CHECK(omfsTruncInt64toInt32(endPos, &newLen));	/* OK 1.x */
				}

				if (newLen > masterLen)
				{
					CHECK(omfsWriteInt32(file, mob, OMTRKGGroupLength,
												fileMobLen));
				}
				CHECK(omfsWriteInt32(file, sclp, OMCLIPLength, newLen));
			}
			break;
			

		case kOmfRev2x:
			/* Adjust the SCLP length from the master mob to the length of
			 * the file mob, in the units of the master mob
			 */
			CHECK(omfiIteratorAlloc(file, &slotIter));
			CHECK(omfiMobGetNumSlots(file, mob, &numSlots));
			for (loop = 1; loop <= numSlots; loop++)
			{
				CHECK(omfiMobGetNextSlot(slotIter, mob, NULL, &slot));
				CHECK(omfiMobSlotGetInfo(file, slot, NULL, &seg));
				CHECK(omfiSourceClipResolveRef(file, seg, &fileMob));
				CHECK(omfiMobGetNumSlots(file, fileMob, &fileNumSlots));
				if(fileNumSlots >= 1)
				{
					CHECK(omfiIteratorAlloc(file, &fileSlotIter));
					CHECK(omfiMobGetNextSlot(fileSlotIter, fileMob, NULL, &fileSlot));
					CHECK(omfiMobSlotGetInfo(file, fileSlot, NULL, &fileSeg));
					CHECK(omfsReadLength(file, fileSeg, OMCPNTLength, &endPos));
					omfiIteratorDispose(file, fileSlotIter);
				}
				
				CHECK(omfsReadRational(file, fileSlot, OMMSLTEditRate, &fileRate));
				CHECK(omfsReadRational(file, slot, OMMSLTEditRate, &masterRate));
				if((fileRate.numerator != masterRate.numerator) ||
				   (fileRate.denominator != masterRate.denominator))
				{
					CHECK(omfiConvertEditRate(	fileRate, endPos, masterRate, kRoundFloor, &endPos));
				}

				CHECK(omfsWriteLength(file, seg, OMCPNTLength, endPos));
			}			
			omfiIteratorDispose(file, slotIter);
			slotIter = NULL;
			break;
		}
	}
	XEXCEPT
	XEND
		
	return (OM_ERR_NONE);
}


/************************
 * Function: omfsReconcileMobLength (INTERNAL)
 *
 * 	Given a master mob or file mob, make sure that all fields
 *		which contain the length of the mob are in agreement.  Currently
 *		only makes sure that mob length references are >= each of
 *		the track lengths.
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
static omfErr_t omfsReconcileMobLength(
			omfHdl_t		file,
			omfObject_t mob)
{
	omfObject_t    physMedia, trak, sclp;
	omfInt32       dataLength, fileMobLen, newLen, numTracks, index;
	omfInt32			numSlots, loop;
	omfProperty_t	mdesProp;
	omfLength_t		len;
	omfMSlotObj_t	slot;
	omfSegObj_t		seg;
	omfIterHdl_t	slotIter = NULL;
	omfPosition_t	endPos;
	omfRational_t	srcRate, destRate;
	double			fpSrcRate, fpDestRate;
		
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);

	XPROTECT(file)
	{
		switch(file->setrev)
		{
		case kOmfRev1x:
		case kOmfRevIMA:
			mdesProp = OMMOBJPhysicalMedia;
			break;
			
		case kOmfRev2x:
			mdesProp = OMSMOBMediaDescription;
			break;
	
		default:
			RAISE(OM_ERR_FILEREV_NOT_SUPP);
		}
		
		if(omfsIsPropPresent(file, mob, mdesProp, OMObjRef))
		{				/* We are a physical MOB */
			CHECK(omfsReadObjRef(file, mob, mdesProp, &physMedia));
			if(file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
			{
				CHECK(omfsReadInt32(file, physMedia, OMMDFLLength, &dataLength));
				CHECK(omfsCvtInt32toInt64(dataLength, &endPos));
				CHECK(omfsReadExactEditRate(file, physMedia, OMMDFLSampleRate, &srcRate));
				CHECK(omfsReadExactEditRate(file, mob, OMCPNTEditRate, &destRate));
				if((srcRate.numerator == destRate.numerator) &&
				   (srcRate.denominator == destRate.denominator))
				{
					newLen = dataLength;
				}
				else
				{
					fpSrcRate = (double)srcRate.numerator / (double)srcRate.denominator;
					fpDestRate = (double)destRate.numerator / (double)destRate.denominator;
					if(fpSrcRate > fpDestRate)
					{
						CHECK(omfsAddInt32toInt64(1,&endPos));/* Round up only if
																		  fractional source sample */
					}
					CHECK(omfiConvertEditRate(	srcRate, endPos, destRate, kRoundFloor, &endPos));					
					CHECK(omfsTruncInt64toInt32(endPos, &newLen));	/* OK 1.x */
				}

				if(omfsIsPropPresent(file, mob, OMTRKGGroupLength, OMInt32))
				{
					CHECK(omfsReadInt32(file, mob, OMTRKGGroupLength, &fileMobLen));
					if (newLen > fileMobLen)
						CHECK(omfsWriteInt32(file, mob, OMTRKGGroupLength, newLen));
				}
				else
					CHECK(omfsWriteInt32(file, mob, OMTRKGGroupLength, dataLength));
					
				numTracks = omfsLengthObjRefArray(file, mob, OMTRKGTracks);
				for(index = 1; index <= numTracks; index++)
				{
					CHECK(omfsGetNthObjRefArray(file, mob, OMTRKGTracks, &trak,
														index));
					CHECK(omfsReadObjRef(file, trak, OMTRAKTrackComponent, &sclp));
					CHECK(omfsWriteInt32(file, sclp, OMCLIPLength, newLen));
				}
			}
			else
			{
				CHECK(omfiIteratorAlloc(file, &slotIter));
				CHECK(omfiMobGetNumSlots(file, mob, &numSlots));
				for (loop = 1; loop <= numSlots; loop++)
				{
					CHECK(omfiMobGetNextSlot(slotIter, mob, NULL, &slot));
					CHECK(omfiMobSlotGetInfo(file, slot, &destRate, &seg));
					CHECK(omfsReadLength(file, physMedia, OMMDFLLength, &len));
					CHECK(omfsReadRational(file, physMedia, OMMDFLSampleRate, &srcRate));
					if((srcRate.numerator != destRate.numerator) ||
					   (srcRate.denominator != destRate.denominator))
					{
						CHECK(omfiConvertEditRate(	srcRate, len, destRate, kRoundFloor, &len));
					}

					CHECK(omfsWriteLength(file, seg, OMCPNTLength, len));
				}			
				omfiIteratorDispose(file, slotIter);
				slotIter = NULL;
			}
				
		}
		else			/* Must be a master clip */
		{
			CHECK(omfsReconcileMasterMobLength(file, mob));
		}
	}
	XEXCEPT
	XEND
		
	return (OM_ERR_NONE);
}


/************************
 * Function: omfmMobGetMediaDescription
 *
 *    This function takes a file mob and returns and object reference
 *    to the associated Media Description object.
 *
 *    This function supports both 1.x and 2.x files.
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
omfErr_t omfmMobGetMediaDescription(
												omfHdl_t file,
												omfObject_t fileMob,
												omfObject_t *mdes)
{
   omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssert(fileMob != NULL, file, OM_ERR_NULL_PARAM);

	XPROTECT(file)
	{
	   if (!omfiIsASourceMob(file, fileMob, &omfError))
			 RAISE(OM_ERR_INVALID_MOBTYPE);

		if(file->setrev == kOmfRev2x)
		{
			CHECK(omfsReadObjRef(file, fileMob, OMSMOBMediaDescription, mdes));
		}
		else
		{
			CHECK(omfsReadObjRef(file, fileMob, OMMOBJPhysicalMedia, mdes));
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: FindTrackID
 *
 * 		Given a mob and a track ID, returns the mobSlot object.
 *
 * Argument Notes:
 *		trackID - A 2.0 track ID, unique for all slots in the Mob.  When
 *			writing a 1.x era file, the toolkit will map this into a 1.x
 *			trackNum in the file.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_MISSING_TRACKID - The given trackID is not present.
 */
static omfErr_t FindTrackByID(
			omfHdl_t 		file,			/* IN -- */
			omfObject_t 	masterMob,	/* IN -- */
			omfTrackID_t	trackID,		/* IN -- */
			omfMSlotObj_t	*trackRtn)	/* OUT -- */
{
	omfIterHdl_t 	iterHdl;
	omfSearchCrit_t	crit;
	omfTrackID_t		srchTrackID;
	omfBool			found;
	omfMSlotObj_t	track;

	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	omfAssert(trackRtn != NULL, file, OM_ERR_NULL_PARAM);

	XPROTECT(file)
	{
		*trackRtn = NULL;
		crit.searchTag = kNoSearch;
		CHECK(omfiIteratorAlloc(file,&iterHdl) );
	
		found = FALSE;
		CHECK(omfiMobGetNextTrack(iterHdl, masterMob, &crit, &track));
		while(*trackRtn == NULL)
		{
	
			CHECK(omfiTrackGetInfo(file, masterMob, track, NULL,
							0, NULL, NULL, &srchTrackID, NULL));
			if(trackID == srchTrackID)
			{
				*trackRtn = track;
				break;
			}
			
			CHECK(omfiMobGetNextTrack(iterHdl, masterMob, &crit, &track));
		}
		if (iterHdl)
		  {
			 omfiIteratorDispose(file, iterHdl);
			 iterHdl = NULL;
		  }
	}
	XEXCEPT
	{
	  if (iterHdl)
		 omfiIteratorDispose(file, iterHdl);
		if(XCODE() == OM_ERR_NO_MORE_OBJECTS)
			RERAISE(OM_ERR_MISSING_TRACKID);
	}
	XEND
		
	return(OM_ERR_NONE);
}

/************************
 * Function: PerFileMediaInit (INTERNAL)
 *
 * 		This function is a callback from the openFile and createFile
 *		group of functions.  This callback exists in order to allow the
 *		media layer to be independant, and yet have information of its
 *		own in the opaque file handle.
 *
 *    (NOTE: the data object cache needs to be set up whether or not
 *           the media layer exists, so the guts of this function were
 *           moved to omFile.c:BuildMediaCache().  This function shell was
 *           left for future expansion.)
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
omfErr_t PerFileMediaInit(omfHdl_t file)
{	
	return(OM_ERR_NONE);
}

/************************
 * Function: DisposeCodecPersist	(INTERNAL)
 *
 * 		This is a private callback from the omfsEndSession call.  Certain
 *		per-process data is allocated by the media layer, and needs to be
 *		released when the session ends.
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
omfErr_t DisposeCodecPersist(omfSessionHdl_t sess)
{
	omfBool				more;
	omTableIterate_t	iter;
	codecTable_t		codec_table;
	
	XPROTECT(NULL)
	{
		CHECK(omfsTableFirstEntry(sess->codecID, &iter, &more));
		while(more)
		{
			if(iter.valuePtr != NULL)
			{
			/*
				int temp = 0;
				memcpy (&(codec_table.initRtn),&(iter.valuePtr.initRtn), sizeof(iter.valuePtr.initRtn));
				for (temp = 0; temp < NUM_FUNCTION_CODES; temp++)
					{
					memcpy(&codec_table.dispTable[temp],&iter.valuePtr.dispTable[temp], sizeof(iter.valuePtr.dispTable[temp]));
					}
				codec_table.persist =	iter.valuePtr.persist;
				codec_table.rev =	iter.valuePtr.rev;
				codec_table.type =	iter.valuePtr.type;
				strcpy(codec_table.dataKindNameList, iter.valuePtr.dataKindNameList);
				codec_table.minFileRev =	iter.valuePtr.minFileRev;
				codec_table.maxFileRev =	iter.valuePtr.maxFileRev;
				codec_table.maxFileRevIsValid =	iter.valuePtr.maxFileRevIsValid;
			  */
				 memcpy(&codec_table, iter.valuePtr, sizeof(codec_table));
				if(codec_table.persist == NULL)
					omOptFree(NULL, codec_table.persist);
			}
			CHECK(omfsTableNextEntry(&iter, &more));
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}


/*************************************************************

          OPCODE LIST SUPPORT
          
 ************************************************************/
 
 #define incrementListPtr(lp, t) lp = (t *) ((char *)lp + sizeof(t))
 
/************************
 * omfmVideoOpCount
 *
 * 		Returns the length of an operation list.  Loop through
 *		the list, looking for the end marker, counting the 
 *		loops.
 *
 * Argument Notes:
 *		If list is NULL, count is zero.
 *
 * ReturnValue:
 *		Count of operations in list.
 *
 * Possible Errors:
 *		None.
 */

omfInt32 omfmVideoOpCount(
				omfHdl_t file, 
				omfVideoMemOp_t *list)
{
	omfInt32 count = 0;

	omfAssertValidFHdl(file);

	while (list && list->opcode != kOmfVFmtEnd)
	{
		incrementListPtr(list, omfVideoMemOp_t);		/* point to next operation */
		count++;	/* got another one! */
	}
	
	return(count);
}
	
/************************
 * omfmVideoOpInit
 *
 * 		Sets a list to be empty.
 *
 * Argument Notes:
 *		'list' is a pointer to the head of an operation list.
 *		Insert an end marker in the head of the list.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		OM_ERR_NONE
 *		OM_ERR_NULL_PARAM
 */
 
omfErr_t omfmVideoOpInit(
				omfHdl_t file, 
				omfVideoMemOp_t *list)
{
	omfInt32 count = 0;

	omfAssertValidFHdl(file);
	omfAssert(list, file, OM_ERR_NULL_PARAM);

	list->opcode = kOmfVFmtEnd;
	
	return(OM_ERR_NONE);
}

/************************
 * omfmVideoOpAppend
 *
 * 		Puts an operation in a list.
 *
 * Argument Notes:
 *		operation is a struct value, list is a pointer to
 *		the head of an operation list, force indicates whether 
 *		or not to overwrite a previously stored operation.  
 *		If force is kOmfForceOverwrite, then overwrite.
 *		If force is kOmfAppendIfAbsent, then don't do anything
 *		if the operation already exists.  maxLength is the
 *		number of items (including the end marker) that can
 *		be stored in 'list'
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		OM_ERR_NONE
 *		OM_ERR_NULL_PARAM
 *		OM_ERR_OPLIST_OVERFLOW
 */
 
omfErr_t omfmVideoOpAppend(
				omfHdl_t file, 
				omfAppendOption_t force,
				omfVideoMemOp_t item, 
				omfVideoMemOp_t *list,
				omfInt32 maxLength)
{
	omfVideoFmtOpcode_t savedOpcode;
	omfInt32 count = 0;

	omfAssertValidFHdl(file);
	omfAssert(list, file, OM_ERR_NULL_PARAM);

	while (list->opcode != kOmfVFmtEnd &&
		   list->opcode != item.opcode)
	{
		incrementListPtr(list, omfVideoMemOp_t);				/* point to next operation */
		count++;
	}
	
	/* if the number of items counted in the list plus
	   the end marker is greater than or equal to the 
	   maximum number of items allowed in the list, then
	   return an overflow error, since there's no way to
	   add the item to the list.  If the item matches a
	   previous entry, then this error should not be raised
	   unless the list is overflowing to begin with.
	*/
	
	if ((count + 1) >= maxLength)
		return(OM_ERR_OPLIST_OVERFLOW);
	
	savedOpcode = list->opcode; /* why did the loop terminate? */
	
	/* At this time, 'list' points to either a previously
	   stored operation, or the end marker.  If the force
	   code is kOmfForceOverwrite, then write the item into
	   the list regardless.  Otherwise, only write the item
	   if 'list' is pointing at the end marker */
	   
	if (force == kOmfForceOverwrite || 
		savedOpcode == kOmfVFmtEnd)
	{
		*list = item;
		incrementListPtr(list, omfVideoMemOp_t);
	}
	
	/* if the opcode is kOmfVFmtEnd, then the item wasn't found,
	   so it was appended, overwriting the end marker. Now restore
	   the end marker. We assume that 'list' has been incremented
	   past the old end marker, which is why we have to use 
	   'savedOpcode'. */
	if (savedOpcode == kOmfVFmtEnd)
		list->opcode = kOmfVFmtEnd;

	
	return(OM_ERR_NONE);
}
	
/************************
 * omfmVideoOpMerge
 *
 * 		Merges two lists.
 *
 * Argument Notes:
 *		source is a pointer to the head of the source list, 
 *		destination is a pointer to the head of a destination
 *		list. Force indicates whether 
 *		or not to overwrite previously stored operations.  
 *		If force is kOmfForceOverwrite, then overwrite.
 *		If force is kOmfAppendIfAbsent, then don't do anything
 *		if the operations already exist.  maxDestLength is the
 *		number of items (including the end marker) that can
 *		be stored in 'destination'
 *
 *		ASSUMPTION: destination points to enough memory to store
 *		the additional item!
 *
 * ReturnValue:
 *		'destination' will receive the merged operations.
 *
 *		The function returns an error code (see below).
 *
 * Possible Errors:
 *		OM_ERR_NONE
 *		OM_ERR_NULL_PARAM
 *		OM_ERR_OPLIST_OVERFLOW
 */
 
omfErr_t omfmVideoOpMerge(
				omfHdl_t file, 
				omfAppendOption_t force,
				omfVideoMemOp_t *source, 
				omfVideoMemOp_t *destination,
				omfInt32 maxDestLength)
{

	/* the algorithm will be simple for now (It's not efficient,
	   n-squared, but should be okay given the small magnitude
	   expected).  Loop through the source, appending the items
	   one at a time into the destination, passing along 'force'. */
	   
	omfAssertValidFHdl(file);
	omfAssert(source, file, OM_ERR_NULL_PARAM);
	omfAssert(destination, file, OM_ERR_NULL_PARAM);

	XPROTECT(file)
	{
		while (source->opcode != kOmfVFmtEnd)
		{
			CHECK(omfmVideoOpAppend(file, force, *source,
									destination, maxDestLength));
			incrementListPtr(source, omfVideoMemOp_t);
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

	
/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
