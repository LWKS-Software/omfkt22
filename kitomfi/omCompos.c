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

/* Name: omCompos.c
 * 
 * Overall Function: The high level API for building simple (flat) compositions
 *
 * Audience: Clients writing OMFI compositions.
 *
 * Public Functions:
 *    omfcSimpleTrackNew(), omfcSimpleTrackClose()
 *    omfcSimpleAppendFiller(), omfcSimpleAppendSourceClip()
 *    omfcSimpleTimecodeTrackNew(), omfcSimpleEdgecodeTrackNew()
 *    omfcSimpleTrackGetLength()
 *    omfcSimpleAppendTransition(), omfcSimpleAppendEffect()
 *    omfcSimpleAppendXXX() - functions for appending specific effects
 * 
 * General error codes returned:
 *    OM_ERR_TRACK_EXISTS
 *        A Track with this trackID already exists
 *    OM_ERR_NOMEMORY
 *        Memory allocation failed, no more heap memory
 *    OM_ERR_STRACK_APPEND_ILLEGAL
 *        This track does not contain a sequence, appending is illegal
 *    OM_ERR_BAD_SLOTLENGTH
 *        Bad Slot Length
 *    OM_ERR_INVALID_DATAKIND
 *        The track datakind does not match the appended component's datakind.
 *    OM_ERR_INVALID_EFFECT
 *        The effect is invalid or non-existent.
 */

#include "masterhd.h"
#include "omPublic.h"
#include "omPvt.h" 
#include "omEffect.h"
#include "omCompos.h"


#define NOT_TIMEWARP 0
#define IS_TIMEWARP 1

/* Static Function Definitions */
static omfErr_t InitTrackHdl(
    omfHdl_t file, 
    omfMobObj_t mob,
    omfTrackID_t trackID,
    omfTrackHdl_t trackHdl);

static omfBool DoesTrackExist(
    omfHdl_t      file,
    omfMobObj_t   mob,
    omfTrackID_t  trackID,
    omfErr_t      *omfError);

/************************/
/* Function Definitions */
/************************/

/*************************************************************************
 * Private Function: DoesTrackExist()
 *
 *      This internal function returns whether or not a track with
 *      the given track ID already exists.  It is called by the functions
 *      that create new tracks to make sure that a duplicate track isn't
 *      being created.
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *      If an error occurs, it is returned as the last argument, and the
 *      return value will be FALSE.
 *
 * ReturnValue:
 *		Boolean (as described above)
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfBool DoesTrackExist(
    omfHdl_t      file,       /* IN - File Handle */
    omfMobObj_t   mob,        /* IN - Mob object */
    omfTrackID_t  trackID,    /* IN - Track ID */
    omfErr_t      *omfError)  /* OUT - Error code */
{
    omfInt32 numTracks, loop;
    omfIterHdl_t mobIter;
	omfObject_t trackSeg;
	omfMSlotObj_t tmpTrack;
	omfInt16 tmp1xTrackType;
	omfTrackID_t tmpTrackID, tmpMapTrackID;
	omfUID_t mobID;
    omfBool found = FALSE;

	*omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE);

    XPROTECT(file)
	  {
		CHECK(omfiIteratorAlloc(file, &mobIter));
		CHECK(omfiMobGetNumTracks(file, mob, &numTracks));
		for (loop = 1; loop < numTracks; loop++)
		  {
			CHECK(omfiMobGetNextTrack(mobIter, mob, NULL, 
									  &tmpTrack));
			CHECK(omfiTrackGetInfo(file, mob, tmpTrack, NULL, 0, NULL, NULL, 
								   &tmpTrackID, &trackSeg));
			/* If 1.x file, get track mapping */
			if (file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
			  {
				/* Get the tracktype of the track's component */
				CHECK(omfsReadTrackType(file, trackSeg, 
										OMCPNTTrackKind, &tmp1xTrackType));
				/* Get the MobID from the track's mob */
				CHECK(omfiMobGetMobID(file, mob, &mobID));
				CHECK(CvtTrackNumtoID(file, mobID, (omfInt16)tmpTrackID, 
									  tmp1xTrackType, &tmpMapTrackID));
				tmpTrackID = tmpMapTrackID;
			  }
			if (tmpTrackID == trackID)
			  {
				found = TRUE;
				break;
			  }
		  }

		if (mobIter)
		  {
			CHECK(omfiIteratorDispose(file, mobIter));
			mobIter = NULL;
		  }
		if (found)
		  return(TRUE);
		else
		  return(FALSE);
	  }
	
	XEXCEPT
	  {
		if (mobIter)
		  omfiIteratorDispose(file, mobIter);
		*omfError = XCODE();
	  }
	XEND_SPECIAL(FALSE);

	return(FALSE);
}

/*************************************************************************
 * Function: omfcSimpleTrackNew()
 *
 *      This function creates a new track in the given mob.  The track is
 *      identified with the given trackID and trackName.  Its attributes are 
 *      defined in the datakindName and editRate properties.  A track handle 
 *      is returned which should be used in future calls to the simple 
 *      composition interface.  A sequence is automatically created as the 
 *      track component, and the track is appended to the mob.  The origin of
 *      the track will be set to 0. 
 *
 *      This function assumes that a simple composition will be created.
 *      The datakind, editrate and other attributes will be remembered
 *      and enforced for components added to the track.  For 2.x, a mob
 *      slot is created and a track descriptor is added to it.  For
 *      1.x, a TRAK object is created.
 *
 *      This function supports 1.x and 2.x files.
 *
 * Argument Notes:
 *      The track handle is returned and should be passed into future
 *      simple composition functions.  trackName is optional, and a NULL
 *      should be passed if not desired.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleTrackNew(
	omfHdl_t      file,              /* IN - File Handle */
	omfMobObj_t   mob,               /* IN - Mob object */
	omfTrackID_t  trackID,           /* IN - Track ID */
	omfString     trackName,         /* IN - Track Name (optional) */
	omfRational_t editRate,          /* IN - Editrate */
    omfUniqueNamePtr_t datakindName, /* IN - Datakind Name */
	omfTrackHdl_t *trackHdl)         /* OUT - Track Handle */
{
    omfTrackHdl_t tmpHdl = NULL;
	omfDDefObj_t datakind = NULL;
	omfErr_t omfError = OM_ERR_NONE;
	omfPosition_t	zeroPos;
	
	*trackHdl = NULL;
	omfAssertValidFHdl(file);

	omfsCvtInt32toPosition(0, zeroPos);

	XPROTECT(file)
	  {
		if (DoesTrackExist(file, mob, trackID, &omfError))
		  {
			RAISE(OM_ERR_TRACK_EXISTS);
		  }

		tmpHdl = (omfTrackHdl_t)omfsMalloc(sizeof(struct omfiSimpleTrack));
		if (tmpHdl == NULL)
		  {
			RAISE(OM_ERR_NOMEMORY);
		  }
		CHECK(InitTrackHdl(file, mob, trackID, tmpHdl));

		/* Create a new track and add an empty sequence as the component */
		if (omfiDatakindLookup(file, datakindName, &datakind, &omfError))
		  {
			tmpHdl->datakind = datakind;
			CHECK(omfiSequenceNew(file, datakind, &tmpHdl->sequence));
			CHECK(omfiMobAppendNewTrack(file, mob, editRate, tmpHdl->sequence,
									zeroPos/* origin */, trackID, trackName,
									&tmpHdl->track));
		  }

		/* If false, return an error */
		else
		  {
			RAISE(OM_ERR_INVALID_DATAKIND);
		  }
	  }

	XEXCEPT
	  {
		/* Assume don't have to free if malloc failed */
		if ((XCODE() != OM_ERR_NOMEMORY) && tmpHdl)
		  omfsFree(tmpHdl);
		return(XCODE());
	  }
	XEND;

	*trackHdl = tmpHdl;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: InitTrackHdl()
 *
 *      This internal function initializes the fields in a track handle
 *      data structure.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t InitTrackHdl(
    omfHdl_t file,           /* IN - File Handle */
    omfMobObj_t mob,         /* IN - Mob object */
    omfTrackID_t trackID,    /* IN - Track ID */
    omfTrackHdl_t trackHdl)  /* IN/OUT - Track Handle */
{
    omfAssertValidFHdl(file);

	trackHdl->cookie = SIMPLETRAK_COOKIE;
	trackHdl->file = file;
	trackHdl->mob = mob;
	trackHdl->trackID = trackID;
	omfsCvtInt32toPosition(1, trackHdl->currentPos);
	trackHdl->track = NULL;
	trackHdl->sequence = NULL;
	trackHdl->datakind = NULL;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleTrackClose()
 *
 *      This function ends a simple composition "session".  If cleans
 *      up the track, and frees the track handle.  The track handle should
 *      not be used after this function is called.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleTrackClose(
    omfTrackHdl_t trackHdl)       /* IN/OUT - Track Handle */
{
    omfObject_t segment = NULL;
	omfRational_t editRate;
	omfInt32 matches, segLength1x, groupLength;
	omfLength_t length;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);

	XPROTECT(trackHdl->file)
	  {
		/* If 1.x file, need to patch a few things */
		if (trackHdl->file->setrev == kOmfRev1x || 
			trackHdl->file->setrev == kOmfRevIMA)	
		  {
			/* 1) Patch each component to make sure that it has an 
			 *    editrate property.
			 */
			CHECK(omfiMobSlotGetInfo(trackHdl->file, trackHdl->track, NULL,
									 &segment));
			CHECK(omfsReadExactEditRate(trackHdl->file, segment, OMCPNTEditRate,
										&editRate)); 
			CHECK(omfiMobMatchAndExecute(trackHdl->file, segment,
										 0, /* level */
										 isObjFunc, NULL,
										 set1xEditrate, &editRate,
										 &matches));
			/* 
			 * 2) Patch the Mob's TRKG:GroupLength, if necessary.
			 */
			CHECK(omfsReadInt32(trackHdl->file, trackHdl->mob, 
							   OMTRKGGroupLength, &groupLength));
			CHECK(omfiComponentGetLength(trackHdl->file, segment, &length));
			/* Truncation not a problem since 1.x file */
			CHECK(omfsTruncInt64toInt32(length, &segLength1x));	/* OK 1.X */
			/* If length of segment is greater, patch mob group length */
			if (segLength1x > groupLength)
			  {
				CHECK(omfsWriteInt32(trackHdl->file, trackHdl->mob, 
									OMTRKGGroupLength, segLength1x));
			  }
		  }

		if (trackHdl)
		  omfsFree(trackHdl);

		trackHdl = NULL;
	  }

	XEXCEPT
	  {
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleAppendFiller()
 *
 *      This function appends a filler clip of the given length to
 *      a track that was created with the simple composition interface.
 *      It will figure out the datakind of the filler from the sequence.
 *
 *      This function will support 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendFiller(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
	omfLength_t length)       /* IN - Fill Length */
{
    omfObject_t newFiller;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		CHECK(omfiFillerNew(trackHdl->file, trackHdl->datakind, length, 
							&newFiller));
		CHECK(omfiSequenceAppendCpnt(trackHdl->file, trackHdl->sequence,
									 newFiller));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}	

/*************************************************************************
 * Function: omfcSimpleAppendSourceClip
 *
 *      This function appends a source clip of the given length to
 *      a track that was created with the simple composition interface.
 *      it will figure out the datakind of the filler from the sequence.
 *
 *      This function will support 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendSourceClip(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
	omfLength_t length,       /* IN - Length of source clip */
	omfSourceRef_t sourceRef) /* IN - Source Reference (sourceID, 
							   *      sourceTrackID, startTime)
							   */
{
    omfSegObj_t newSourceClip;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		CHECK(omfiSourceClipNew(trackHdl->file, trackHdl->datakind, length,
								sourceRef, &newSourceClip));
		CHECK(omfiSequenceAppendCpnt(trackHdl->file, trackHdl->sequence,
									 newSourceClip));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
} 

/*************************************************************************
 * Function: omfcSimpleTimecodeTrackNew()
 *
 *      This function creates a new timecode track in the given mob.  
 *      The track is identified with the given trackID and trackName.
 *      This function will create a single timecode clip and append
 *      it as the track's component.  The length, timecode and editrate are
 *      specified as arguments.  The datakind of the track is initialized
 *      to be omfi:data:Timecode.  The origin of the track will be set to 0.
 *
 *      A track handle representing the track is returned, and should be
 *      passed into other simple composition functions.
 *
 *      For 2.x, a mob slot is created and a track descriptor is added to it.
 *      For 1.x, a TRAK object is created.
 *
 * Argument Notes:
 *      The track handle is returned and should be passed into future
 *      simple composition functions.  trackName is optional, and a NULL
 *      should be passed if not desired.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleTimecodeTrackNew(
    omfHdl_t      file,        /* IN - File Handle */
    omfMobObj_t   mob,         /* IN - Mob object */
    omfTrackID_t  trackID,     /* IN - Track ID */
    omfString     trackName,   /* IN - Track Name (optional) */
    omfRational_t editRate,    /* IN - Editrate */
    omfLength_t   length,      /* IN - Length of timecode */
    omfTimecode_t timecode,    /* IN - Initial timecode (startFrame, drop,
								*      fps) 
								*/
    omfTrackHdl_t *trackHdl)   /* OUT - Track Handle */
{
    omfTrackHdl_t tmpHdl;
	omfSegObj_t   tcClip;
	omfDDefObj_t  datakind;
	omfErr_t      omfError;
	omfPosition_t	zeroPos;
	
	omfAssertValidFHdl(file);

	omfsCvtInt32toPosition(0, zeroPos);

	XPROTECT(file)
	  {
		if (DoesTrackExist(file, mob, trackID, &omfError))
		  {
			RAISE(OM_ERR_TRACK_EXISTS);
		  }
		
		tmpHdl = (omfTrackHdl_t)omfsMalloc(sizeof(struct omfiSimpleTrack));
		if (tmpHdl == NULL)
		  {
			RAISE(OM_ERR_NOMEMORY);
		  }
		CHECK(InitTrackHdl(file, mob, trackID, tmpHdl));
		/* Create a new track and add a timecode clip as the component */
		if (omfiDatakindLookup(file, TIMECODEKIND, &datakind, &omfError))
		  {
			tmpHdl->datakind = datakind;
			/* NOTE: how does origin relate to startTC? */
			CHECK(omfiTimecodeNew(file, length, timecode, &tcClip));
			CHECK(omfiMobAppendNewTrack(file, mob, editRate, tcClip,
										zeroPos /* origin */, trackID, trackName,
										&tmpHdl->track));
		  }
	  }

	XEXCEPT
	  {
		if ((XCODE() != OM_ERR_NOMEMORY) && tmpHdl)
		  omfsFree(tmpHdl);
		return(XCODE());
	  }
	XEND;

	*trackHdl = tmpHdl;
	return(OM_ERR_NONE);
}	

/*************************************************************************
 * Function: omfcSimpleEdgecodeTrackNew()
 *
 *      This function creates a new edgecode track in the given mob.
 *      The track is identified with the given trackID and trackName.
 *      This function will create a single edgecode clip and append it as
 *      the track's component.  The length, edgecode and editrate are
 *      specified as arguments.  The datakind of the track is initialized
 *      to be omfi:data:Edgecode.  The origin of the track will be set to 0.
 *
 *      A track handle representing the track is returned, and should be
 *      passed into other simple composition functions.
 *
 *      For 2.x, a mob slot is created and a track descriptor is added to it.
 *      For 1.x, a TRAK object is created.
 *
 * Argument Notes:
 *      The track handle is returned and should be passed into future
 *      simple composition functions.  trackName is optional, and a NULL
 *      should be passed if not desired.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleEdgecodeTrackNew(
    omfHdl_t      file,        /* IN - File Handle */
    omfMobObj_t   mob,         /* IN - Mob object */
    omfTrackID_t  trackID,     /* IN - Track ID */
    omfString     trackName,   /* IN - Track Name (optional) */
    omfRational_t editRate,    /* IN - Editrate */
    omfLength_t   length,      /* IN - Length of edgecode */
	omfEdgecode_t edgecode,    /* IN - Initial edgecode (startFrame, filmKind,
								*      codeFormat)
								*/
    omfTrackHdl_t *trackHdl)   /* OUT - Track Handle */
{
	omfTrackHdl_t tmpHdl;
	omfSegObj_t   ecClip;
	omfDDefObj_t datakind;
	omfErr_t omfError;
	omfPosition_t	zeroPos;
	
	omfAssertValidFHdl(file);

	omfsCvtInt32toPosition(0, zeroPos);

	XPROTECT(file)
	  {
		if (DoesTrackExist(file, mob, trackID, &omfError))
		  {
			RAISE(OM_ERR_TRACK_EXISTS);
		  }

		tmpHdl = (omfTrackHdl_t)omfsMalloc(sizeof(struct omfiSimpleTrack));
		if (tmpHdl == NULL)
		  {
			RAISE(OM_ERR_NOMEMORY);
		  }
		CHECK(InitTrackHdl(file, mob, trackID, tmpHdl));
		/* Create a new track and add an edgecode clip as the component */
		if (omfiDatakindLookup(file, EDGECODEKIND, &datakind, &omfError))
		  {
			tmpHdl->datakind = datakind;
			/* NOTE: how does origin relate to startEC? */
			CHECK(omfiEdgecodeNew(file, length, edgecode, &ecClip));
			CHECK(omfiMobAppendNewTrack(file, mob, editRate, ecClip,
										zeroPos /* origin */, trackID, trackName,
										&tmpHdl->track));
		  }
	  }

	XEXCEPT
	  {
		if ((XCODE() != OM_ERR_NOMEMORY) && tmpHdl)
		  omfsFree(tmpHdl);
		return(XCODE());
	  }
	XEND;

	*trackHdl = tmpHdl;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleTrackGetLength()
 *
 *      This function returns the length of a track in a simple
 *      composition.
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleTrackGetLength(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
	omfLength_t *length)      /* OUT - Length of the track */
{
    omfObject_t segment = NULL;

 	omfAssertSTrackHdl(trackHdl);

	XPROTECT(trackHdl->file)
	  {
		CHECK(omfiTrackGetInfo(trackHdl->file, trackHdl->mob, trackHdl->track,
						 NULL, 0, NULL, NULL, NULL, &segment));
		if (segment)
		  {
			CHECK(omfiComponentGetLength(trackHdl->file, segment, length));
		  }
	  }

	XEXCEPT
	  {
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleAppendTransition()
 *
 *      This function appends a transition to a track in a simple
 *      composition.  The datakind is assumed to be the datakind of
 *      the track, established at creation time.  The effect applied
 *      to the composition must be created with the mob creation
 *      interface and passed in as an argument.
 *
 *      This function supports 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendTransition(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
	omfLength_t length,       /* IN - Length of transition */
	omfPosition_t cutPoint,       /* IN - Cut point */
	omfEffObj_t effect)       /* IN - Effect object */
{
    omfCpntObj_t newTransition;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify effect object is transition effect */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!IsValidTranEffect(trackHdl->file, effect, &omfError))
			  {
				RAISE(OM_ERR_INVALID_EFFECT);
			  }
		  }

		CHECK(omfiTransitionNew(trackHdl->file, trackHdl->datakind, length,
								cutPoint, effect, &newTransition));
		
		CHECK(omfiSequenceAppendCpnt(trackHdl->file, trackHdl->sequence,
									 newTransition));		
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}	

/*************************************************************************
 * Function: omfcSimpleAppendEffect()
 *
 *      This function appends an effect to a track in a simple composition.
 *      The datakind is assumed to be the datakind of the track, established
 *      at creation time.  The effect applied to the composition must be
 *      created with the mob creation interface an dpassed in as an
 *      argument.
 *
 *      There are additional simple composition effect wrappers below,
 *      that will append a specific effect to a track.  These functions
 *      abstract away the existence of the effect object.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendEffect(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
	omfLength_t length,       /* IN - Length of effect */
	omfEffObj_t effect)       /* IN - Effect object */
{
    omfLength_t effLen;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* omfiIsAnEffect will return FALSE for 1.x segment effects 
		 * (SPED, MASK, REPT), so just check on 2.x files.
		 */
		if (trackHdl->file->semanticCheckEnable && 
			(trackHdl->file->setrev == kOmfRev2x))
		  {
			if (!omfiIsAnEffect(trackHdl->file, effect, &omfError))
			  {
				RAISE(OM_ERR_INVALID_EFFECT);
			  }
		  }

		CHECK(omfiComponentGetLength(trackHdl->file, effect, &effLen));
		if (omfsInt64NotEqual(effLen, length))
		  {
			RAISE(OM_ERR_BAD_SLOTLENGTH);
		  }

		CHECK(omfiSequenceAppendCpnt(trackHdl->file, trackHdl->sequence,
									 effect));				
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleAppendMonoAudioDissolve()
 *
 *      A mono audio dissolve combines 2 mono audio streams into
 *      a resulting mono audio stream.
 *
 *      This function calls omfeMonoAudioDissolveNew() to create a new
 *      effect object.  It appends a transition to the track associated
 *      with the input track handle, and inserts the new effect object
 *      into the transition.  This function only applies to a track
 *      of datakind omfi:data:Sound.  
 *
 *      The default value for level in a transition is assumed -- a VVAL
 *      with 2 control points: Value 0 at time 0, and value 1 at time 1.
 *      If the default value is not desired, use omfeMonoAudioDissolveNew()
 *      to create an effect object with the desired controls, and then
 *      append it to the track with omfcSimpleAppendTransition().
 *
 *      The default value for cutPoint is half of the input length of the
 *      effect.
 *
 *      This function supports 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendMonoAudioDissolve(
    omfTrackHdl_t trackHdl,     /* IN - Track Handle */
	omfLength_t length)         /* IN - Length of transition */
{
	omfEffObj_t newEffect = NULL;
	omfInt32	junk;
	omfPosition_t cutPoint;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind Sound ,
		 * which is the output of the MonoAudioDissolve effect.
		 */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsSoundKind(trackHdl->file, trackHdl->datakind, 
								 kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		CHECK(omfeMonoAudioDissolveNew(trackHdl->file, length, NULL,
									   NULL, NULL, &newEffect));
		if (newEffect)
		  {
			/* Default value for cutpoint is half of effect length truncated */
			CHECK(omfsDivideInt64byInt32(length, 2, &cutPoint, &junk));
			CHECK(omfcSimpleAppendTransition(trackHdl, length, 
											 cutPoint, newEffect));

			/* If 1.x file, delete effect (TRKG) object, since don't need it,
			 * after the transition is created.
			 */
			if (trackHdl->file->setrev == kOmfRev1x || 
				trackHdl->file->setrev == kOmfRevIMA)	
			  {
				CHECK(omfsObjectDelete(trackHdl->file, newEffect));
			  }
		  }		
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleAppendStereoAudioDissolve()
 *
 *      A stereo audio dissolve combines 2 stereo audio streams into a
 *      resulting stereo audio stream.
 *
 *      This function calls omfeStereoAudioDissolveNew() to create a new
 *      effect object.  It appends a transition to the track associated
 *      with the input track handle, and inserts the new effect object
 *      into the transition.  This function only applies to a track
 *      of datakind omfi:data:StereoSound.
 *      
 *      The default value for level in a transition is assumed -- a VVAL
 *      with 2 control points: Value 0 at time 0, and value 1 at time 2.
 *      If the default value is not desired, use omfeStereoAudioDissolveNew()
 *      to create an effect object with the desired controls, and then
 *      append it to the track with omfcSimpleAppendTransition().
 *
 *      The default value for cutPoint is half of the input length of the
 *      effect.
 *
 *      This function only supports 2.x files. 
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendStereoAudioDissolve(
    omfTrackHdl_t trackHdl,     /* IN - Track Handle */
	omfLength_t length)         /* IN - Length of transition */
{
	omfEffObj_t newEffect = NULL;
	omfInt32	junk32;
	omfPosition_t cutPoint;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertNot1x(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind StereoSound 
		 * which is the output from a StereoAudioDissolve effect.
		 */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsStereoSoundKind(trackHdl->file, trackHdl->datakind, 
									   kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		CHECK(omfeStereoAudioDissolveNew(trackHdl->file, length, NULL,
										 NULL, NULL, &newEffect));
		if (newEffect)
		  {
			/* Default value for cutpoint is half of effect length truncated */
			CHECK(omfsDivideInt64byInt32(length, 2, &cutPoint, &junk32));
			CHECK(omfcSimpleAppendTransition(trackHdl, length, 
											 cutPoint, newEffect));
		  }		
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: omfcSimpleAppendVideoDissolve()
 *
 *      A simple video dissolve effect combines 2 video streams using
 *      a simple linear equation.
 *
 *      This function calls omfeVideoDissolveNew() to create a new
 *      effect object.  It appends a transition to the track associated
 *      with the input track handle, and inserts the new effect object
 *      into the transition.  This function only applies to a track
 *      of datakind omfi:data:Picture or a convertible datakind.
 *
 *      The default value for level in a transition is assumed -- a VVAL
 *      with 2 control points: Value 0 at time 0, and value 1 at time 1.
 *      If the default value is not desired, use omfeVideoDissolveNew()
 *      to create an effect object with the desired controls, and then
 *      append it to the track with omfcSimpleAppendTransition().
 *
 *      The default value for cutPoint is half of the input length of the
 *      effect.
 *
 *      This function supports 1.x and 2.x files. 
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendVideoDissolve(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
	omfLength_t length)       /* IN - Effect length */
{
	omfEffObj_t newEffect = NULL;
	omfInt32	junk32;
	omfPosition_t cutPoint;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind PictureWithMatte,
		 * which is the output of a VideoDissolve effect.
		 */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsPictureWithMatteKind(trackHdl->file, trackHdl->datakind,
											kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		CHECK(omfeVideoDissolveNew(trackHdl->file, length, NULL,
										 NULL, NULL, &newEffect));
		if (newEffect)
		  {
			/* Default value for cutpoint is half of effect length truncated */
			CHECK(omfsDivideInt64byInt32(length, 2, &cutPoint, &junk32));
			CHECK(omfcSimpleAppendTransition(trackHdl, length,
											 cutPoint, newEffect));
			/* If 1.x file, delete effect (TRKG) object, since don't need it,
			 * after the transition is created.
			 */
			if (trackHdl->file->setrev == kOmfRev1x || 
				trackHdl->file->setrev == kOmfRevIMA)	
			  {
				CHECK(omfsObjectDelete(trackHdl->file, newEffect));
			  }
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleAppendVideoSpeedControl()
 *
 *      A video speed control effect changes the speed at which the media
 *      is to be played by duplicating or eliminating edit units by using a
 *      constant ratio.
 *
 *      This function calls omfeVideoSpeedControlNew(), and appends
 *      the resulting effect to the track associated with the input
 *      track handle.  It only operates on tracks of datakind 
 *      omfi:data:Picture, or a convertible datakind.
 *
 *      The phaseOffset specifies that the first edit unit of the output
 *      is actually offset from the theoretical output.
 *      The default value for phaseOffset (0) is assumed.  
 *      If phaseOffset is not specified, the default is 0.  speedRatio 
 *      defines the ratio of output length to input length.  The range
 *      is 0 to +infinity.  This is a required control.
 * 
 *      This function supports 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendVideoSpeedControl(
    omfTrackHdl_t trackHdl,            /* IN - Track Handle */
	omfLength_t length,                /* IN - Effect Length */
	const omfSegObj_t	inputSegment,  /* IN - Input Segment */
	const omfRational_t	speedRatio)    /* IN - Speed Ratio */
{
    omfEffObj_t newEffect = NULL;
	omfUInt32 phaseOffset;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind PictureWithMatte 
		 * which is the output of a VideoSpeedControl effect.
		 */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsPictureWithMatteKind(trackHdl->file, trackHdl->datakind,
											kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		phaseOffset = 0;

		CHECK(omfeVideoSpeedControlNew(trackHdl->file, length, 
					 inputSegment, speedRatio, phaseOffset, &newEffect));
		if (newEffect)
		  {
			CHECK(omfcSimpleAppendEffect(trackHdl, length, newEffect));
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleAppendVideoWipe()
 *
 *      This function combines 2 video streams using a SMPTE video
 *      wipe with default controls.
 *
 *      This function calls omfeSMPTEVideoWipeNew() to create a new
 *      effect object.  It appends a transition to the track associated
 *      with the input track handle, and inserts the new effect object
 *      into the transition.  This function only applies to a track of
 *      datakind omfi:data:Picture or a convertible datakind.
 *
 *      Required controls:
 *         SMPTE Wipe Number
 *
 *      Default values (from SMPTE 258M) for optional SMPTE controls 
 *      are assumed:
 *         Level: range 0 to 1
 *         Reverse: FALSE
 *         Soft: FALSE
 *         Border: FALSE
 *         Position: FALSE
 *         Modulator: FALSE
 *         Shadow: FALSE
 *         Tumble: FALSE
 *         Spotlight: FALSE
 *         ReplicationH: not specified
 *         ReplicationV: not specified
 *         Checkerboard: FALSE
 *
 *      The default value for cutPoint is half of the input length of the
 *      effect.
 *
 *      This function supports 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendVideoWipe(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
    omfLength_t length,       /* IN - Effect Length */
    omfInt32 wipeNumber)      /* IN - SMPTE Wipe Number */
{
	omfEffObj_t newEffect = NULL;
	omfInt32		junk32;
    	omfPosition_t		cutPoint;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind Picture 
		 * which is the output of a VideoWipe effect. 
		 */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsPictureKind(trackHdl->file, trackHdl->datakind, 
								   kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		CHECK(omfeSMPTEVideoWipeNew(trackHdl->file, length, NULL, NULL,
									NULL, wipeNumber, NULL, &newEffect))
		if (newEffect)
		  {
			/* Default value for cutpoint is half of effect length truncated */
			CHECK(omfsDivideInt64byInt32(length, 2, &cutPoint, &junk32));
			CHECK(omfcSimpleAppendTransition(trackHdl, length, 
											 cutPoint, newEffect));
			/* If 1.x file, delete effect (TRKG) object, since don't need it,
			 * after the transition is created.
			 */
			if (trackHdl->file->setrev == kOmfRev1x || 
				trackHdl->file->setrev == kOmfRevIMA)	
			  {
				CHECK(omfsObjectDelete(trackHdl->file, newEffect));
			  }
		  }

	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}	

/*************************************************************************
 * Function: omfcSimpleAppendMonoAudioPan()
 *
 *      A mono audio pan effect converts a mono audio segment into
 *      stereo audio. The amount of data sent to each channel is 
 *      determined by the pan control.
 *
 *      This function calls omfeMonoAudioPanNew(), and appends
 *      the resulting effect to the track associated with the input
 *      track handle.  It only operates on tracks of datakind
 *      omfi:data:StereoSound.
 *
 *      This function only supports 2.x files. 
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendMonoAudioPan(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
	omfLength_t length,       /* IN - Effect Length */
    omfSegObj_t inputSegment, /* IN - Input segment */
    omfRational_t ratio)      /* IN - Ratio */
{
	omfEffObj_t newEffect = NULL;
	omfSegObj_t ratioCVAL = NULL;
	omfDDefObj_t	rationalDataKind = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertNot1x(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind StereoSound,
		 * which is the output of a MonoAudioPan effect. */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsStereoSoundKind(trackHdl->file, trackHdl->datakind, 
									   kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		/* Create ratio CVAL to pass into omfeMonoAudioPanNew() */
		omfiDatakindLookup(trackHdl->file, RATIONALKIND, 
						   &rationalDataKind, &omfError);
	  	CHECK(omfiConstValueNew(trackHdl->file, rationalDataKind, length,
	  							(omfInt32)sizeof(omfRational_t),
	  							(void *)&ratio, &ratioCVAL));

		CHECK(omfeMonoAudioPanNew(trackHdl->file, length, inputSegment, 
								   ratioCVAL, &newEffect));
		if (newEffect)
		  {
			CHECK(omfcSimpleAppendEffect(trackHdl, length, newEffect));
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleAppendVideoRepeat()
 *
 *      A video repeat timewarp effect repeats a segment of video for
 *      a given amount of time.
 *      
 *      This function calls omfeVideoRepeatNew(), and appends the
 *      resulting effect to the track associated with the input
 *      track handle.  It only operates on tracks of datakind
 *      omfi:data:Picture or a convertible datakind.
 *
 *      The phase offset indicates where (as an offset) to start
 *      reading the input segment on the first loop.  The default 
 *      value (0) for phase offset is assumed.
 *    
 *      This function supports 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendVideoRepeat(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
    omfLength_t length,       /* IN - Effect length */
    omfSegObj_t inputSegment) /* IN - Input segment */
{
    omfEffObj_t newEffect = NULL;
	omfUInt32 phaseOffset;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind PictureWithMatte 
		 * which is the output of a VideoRepeat effect. 
		 */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsPictureWithMatteKind(trackHdl->file, trackHdl->datakind,
											kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		phaseOffset = 0;

		CHECK(omfeVideoRepeatNew(trackHdl->file, length, inputSegment,
								 phaseOffset, &newEffect));
		if (newEffect)
		  {
			CHECK(omfcSimpleAppendEffect(trackHdl, length, newEffect));
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleAppendVideoFadeToBlack()
 *
 *      This effect combines a video stream with black using a simple
 *      linear equation.  It starts with the input video and finishes
 *      with black.
 *
 *      This function calls omfeVideoFadeToBlackNew(), specifying FALSE
 *      for the reverse argument.  The resulting effect is appended
 *      to the track associated with the input track handle.  
 *      It only operates on tracks of datakind omfi:data:Picture
 *      or a convertible datakind.
 *
 *      The default value for level is assumed -- a VVAL with 2 control
 *      points: Value 0 at time 0, and value 1 at time 1.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendVideoFadeToBlack(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
    omfLength_t length,       /* IN - Effect length */
    omfSegObj_t inputSegment) /* IN - Input segment */
{
    omfEffObj_t newEffect = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertNot1x(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind PictureWithMatte
		 * which is the output of a Video FadeTo/FromBlack effect. 
		 */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsPictureWithMatteKind(trackHdl->file, trackHdl->datakind,
											kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		CHECK(omfeVideoFadeToBlackNew(trackHdl->file, length, inputSegment,
								 NULL, FALSE, &newEffect));
		if (newEffect)
		  {
			CHECK(omfcSimpleAppendEffect(trackHdl, length, newEffect));
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: omfcSimpleAppendVideoFadeFromBlack()
 *
 *      This effect combines a video stream with black using a simple
 *      linear equation.  It starts with the black and finishes
 *      with the input video.
 *
 *      This function calls omfeVideoFadeToBlackNew(), specifying TRUE
 *      for the reverse argument.  The resulting effect is appended
 *      to the track associated with the input track handle.  
 *      It only operates on tracks of datakind omfi:data:Picture
 *      or a convertible datakind.
 *
 *      The default value for level is assumed -- a VVAL with 2 control
 *      points: Value 0 at time 0, and value 1 at time 1.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendVideoFadeFromBlack(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
    omfLength_t length,       /* IN - Effect length */
    omfSegObj_t inputSegment) /* IN - Input segment */
{
    omfEffObj_t newEffect = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertNot1x(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind PictureWithMatte 
		 * which is the output of a VideoFadeTo/FromBlack effect. 
		 */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsPictureWithMatteKind(trackHdl->file, trackHdl->datakind,
											kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		CHECK(omfeVideoFadeToBlackNew(trackHdl->file, length, inputSegment,
								 NULL, TRUE, &newEffect));
		if (newEffect)
		  {
			CHECK(omfcSimpleAppendEffect(trackHdl, length, newEffect));
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleAppendMonoAudioGain()
 *
 *      A mono audio gain effect adjusts the volume of a mono audio
 *      segment.
 *
 *      This function calls omfeMonoAudioGainNew(), and appends
 *      the resulting effect to the track associated with the input
 *      track handle.  It only operates on tracks of datakind
 *      omfi:data:StereoSound.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendMonoAudioGain(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
	omfLength_t length,       /* IN - Effect Length */
    omfSegObj_t inputSegment, /* IN - Input segment */
    omfRational_t multiplier) /* IN - Amplitude Multiplier */
{
	omfEffObj_t newEffect = NULL;
	omfSegObj_t multiplierCVAL = NULL;
	omfDDefObj_t	rationalDataKind = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertNot1x(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind Sound,
		 * which is the output of a MonoAudioGain effect.
		 */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsSoundKind(trackHdl->file, trackHdl->datakind, 
								 kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		/* Create ratio CVAL to pass into omfeMonoAudioGainNew() */
		omfiDatakindLookup(trackHdl->file, RATIONALKIND, 
						   &rationalDataKind, &omfError);
	  	CHECK(omfiConstValueNew(trackHdl->file, rationalDataKind, length,
	  							(omfInt32)sizeof(omfRational_t),
	  							(void *)&multiplier, &multiplierCVAL));

		CHECK(omfeMonoAudioGainNew(trackHdl->file, length, inputSegment, 
								   multiplierCVAL, &newEffect));
		if (newEffect)
		  {
			CHECK(omfcSimpleAppendEffect(trackHdl, length, newEffect));
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleAppendStereoAudioGain()
 *
 *      A stereo audio gain effect adjusts the volume of a stereo audio
 *      segment.
 *
 *      This function calls omfeStereoAudioGainNew(), and appends
 *      the resulting effect to the track associated with the input
 *      track handle.  It only operates on tracks of datakind
 *      omfi:data:StereoSound.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendStereoAudioGain(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
	omfLength_t length,       /* IN - Effect Length */
    omfSegObj_t inputSegment, /* IN - Input segment */
    omfRational_t multiplier) /* IN - Multiplier */
{
	omfEffObj_t newEffect = NULL;
	omfSegObj_t multiplierCVAL = NULL;
	omfDDefObj_t	rationalDataKind = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertNot1x(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind StereoSound 
		 * which is the output of a StereoAudioGain effect. 
		 */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsStereoSoundKind(trackHdl->file, trackHdl->datakind, 
									   kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		/* Create multiplier CVAL to pass into omfeMonoAudioGainNew() */
		omfiDatakindLookup(trackHdl->file, RATIONALKIND, 
						   &rationalDataKind, &omfError);
	  	CHECK(omfiConstValueNew(trackHdl->file, rationalDataKind, length,
	  							(omfInt32)sizeof(omfRational_t),
	  							(void *)&multiplier, &multiplierCVAL));

		/* Create multiplier CVAL to pass into omfeMonoAudioGainNew() */
		omfiDatakindLookup(trackHdl->file, RATIONALKIND, 
						   &rationalDataKind, &omfError);
	  	CHECK(omfiConstValueNew(trackHdl->file, rationalDataKind, length,
	  							(omfInt32)sizeof(omfRational_t),
	  							(void *)&multiplier, &multiplierCVAL));

		CHECK(omfeStereoAudioGainNew(trackHdl->file, length, inputSegment, 
								   multiplierCVAL, &newEffect));
		if (newEffect)
		  {
			CHECK(omfcSimpleAppendEffect(trackHdl, length, newEffect));
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfcSimpleAppendVideoMask()
 *
 *      The video mask timewarp effect maps an input video segment to an 
 *      output video segment according to a cyclical pattern.
 *
 *      This function calls omfeVideoFrameMaskNew(), and appends
 *      the resulting effect to the track associated with the input 
 *      track handle. It only operates on tracks of datakind omfi:data:Picture
 *      or a convertible datakind.
 *
 *      The mask bits are required and must not be 0.  They are processed
 *      in order, from the most-significant bit down toward the least-
 *      significant bit as follows: any time all of the remaining bits
 *      in the mask a re 0, then processing resets to the first bit in
 *      the mask.
 *
 *      The phase offset indicates where (as an offset in bits from the
 *      most-significant bit) to start reading the mask on the first cycle.
 *      The default value is assumed (0).
 *
 *      The add/drop indicator is required.  TRUE means that when a 1-bit
 *      is reached in the mask, hold the previous frame.  FALSE means
 *      when a 1-bit is reached in the mask, skip to the next frame.
 *      In both cases, a 0-bit indicates that the input frame should be
 *      copied to the output segment.
 *
 *      This function supports 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfcSimpleAppendVideoMask(
    omfTrackHdl_t trackHdl,   /* IN - Track Handle */
    omfLength_t length,       /* IN - Effect length */
    omfSegObj_t inputSegment, /* IN - Input segment */
    omfUInt32 mask,           /* IN - Mask Bits */
    omfFxFrameMaskDirection_t addOrDrop) /* IN - kOmfFxFrameMaskDirNone,
										*      kOmfFxFrameMaskDirAdd,
										*      kOmfFxFrameMaskDirDrop
										*/
{
    omfEffObj_t newEffect = NULL;
	omfUInt32 phaseOffset;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(trackHdl->file);
	omfAssertSTrackHdl(trackHdl);
	omfAssert((trackHdl->sequence != NULL), trackHdl->file, 
					OM_ERR_STRACK_APPEND_ILLEGAL);

	XPROTECT(trackHdl->file)
	  {
		/* Verify that track is convertible from datakind PictureWithMatte 
		 * which is the output of a VideoFrameMaskEffect.
		 */
		if (trackHdl->file->semanticCheckEnable)
		  {
			if (!omfiIsPictureWithMatteKind(trackHdl->file, trackHdl->datakind,
											kConvertFrom, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  }

		phaseOffset = 0;
		CHECK(omfeVideoFrameMaskNew(trackHdl->file, length, inputSegment,
								 mask, addOrDrop, phaseOffset, &newEffect));
		if (newEffect)
		  {
			CHECK(omfcSimpleAppendEffect(trackHdl, length, newEffect));
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
