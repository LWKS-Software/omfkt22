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
 * Name: omMedObj.c
 *
 * Function: API for manipulating master and source mobs.
 *
 * Audience: All clients.
 *
 * General error codes returned:
 * 		OM_ERR_BAD_FHDL
 *				media referenced a bad omfHdl_t.
 * 		OM_ERR_BENTO_PROBLEM
 *				Bento returned an error, check BentoErrorNumber.
 */

/* A 75-column ruler
         111111111122222222223333333333444444444455555555556666666666777777
123456789012345678901234567890123456789012345678901234567890123456789012345
*/

#include "masterhd.h"
#include <string.h>

#if defined(unix)
#include <sys/types.h>
#endif

#if PORT_SYS_MAC
#include <sys/stat.h>
//#include <CarbonCore/Files.h>
#elif PORT_INC_NEEDS_TIMEH
#include <time.h>
#elif PORT_INC_NEEDS_SYSTIME
#include <sys/time.h>
#include <sys/times.h>
#define HZ CLK_TCK
#endif

#include "omPublic.h"
#include "omMedia.h" 
#include "omPvt.h" 

/* Moved math.h down here to make NEXT's compiler happy */
#include <math.h>

typedef struct
	{
	omfUInt32	fpMinute;
	omfUInt32   fpHour;
	omfUInt32	dropFpMin;
	omfUInt32	dropFpMin10;
	omfUInt32	dropFpHour;
} frameTbl_t;

#define MAX_NUM_CODEC_NAME_CHARS 64
static frameTbl_t GetFrameInfo(omfInt32 fps);
static omfInt32 roundFrameRate(omfRational_t frameRate);

/************************
 * Function:  omfmMasterMobNew
 *
 * 	Create a master mob with no tracks.  Use omfmMobAddMasterTrack()
 *		to add individual file mobs on tracks. 
 *
 * Argument Notes:
 *		isPrimary -- If TRUE, adds the mob to the primary mobs index.  Set
 *			this for almob mobs which are to be used as starting points when
 *			parsing the OMFI file.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMasterMobNew(
				omfHdl_t			file,			/* IN - */
				char				*name,		/* IN - */
				omfBool			isPrimary,
				omfObject_t		*result)		/* OUT - Returned object */
{
	omfObject_t			mob;
	omfUID_t			uid;
	omfTimeStamp_t  mod_time;

	omfsGetDateTime(&mod_time);

	omfAssertValidFHdl(file);
	*result = NULL;

	XPROTECT(file)
	{
		switch (file->setrev)
		{
		case kOmfRev2x:
			CHECK(omfsObjectNew(file, "MMOB", &mob));
			CHECK(MobSetNewProps(file, mob, TRUE, /* is a MasterMob */
									 name, isPrimary));
			CHECK(omfsReadUID(file, mob, OMMOBJMobID, &uid));

			/* Add mobID to hash table */
			CHECK(omfsAppendObjRefArray(file, file->head, OMHEADMobs, mob));
			CHECK(omfsWriteTimeStamp(file, mob, OMMOBJCreationTime, mod_time));
			CHECK(omfsWriteTimeStamp(file, mob, OMMOBJLastModified, mod_time));
			*result = mob;
			break;

		case kOmfRev1x:
		case kOmfRevIMA:
			CHECK(omfsObjectNew(file, "MOBJ", &mob));

			CHECK(omfsWriteInt32(file, mob, OMMOBJStartPosition, 
								0));
			CHECK(omfsWriteUsageCodeType(file, mob, OMMOBJUsageCode, 
										 UC_MASTERMOB));
			/* SetNewProps last, as it relies on lots of checks to see if 
			 * this is a master mob. 
			 * Will write name, modificationTime and new UID
			 * Will add to mob hash table, and append to mob index 
			 * (doesn't add to ObjectSpine).
			 */
			CHECK(MobSetNewProps(file, mob, TRUE, /* Is a Master Mob */
									 name, isPrimary));

			/* Add to object spine */
			CHECK(omfsAppendObjRefArray(file, file->head, OMObjectSpine, 
										mob));
			*result = mob;
			break;
		
		default:
				RAISE(OM_ERR_FILEREV_NOT_SUPP);
		} /* switch */
	} /* XPROTECT */
	XEXCEPT
	XEND;

	return (OM_ERR_NONE);
}

/************************
 * Function:  omfmMobAddMasterTrack
 *
 * 	Add a reference to a particular track on a particular file mob as a track
 *		of a master mob.
 *
 * Argument Notes:
 *		masterMob -- A pre-existing master mob.  Make with omfmMasterMobNew().
 *		trackID - The trackID of the new track in the master mob, must be
 *						unique in the master mob.
 *		fileTrackID -- The trackID of the track referenced in the file mob.
 *		trackName -- A name assigned to the track in the master mob.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMobAddMasterTrack(
				omfHdl_t			file,				/* IN -- */
				omfObject_t		masterMob,		/* IN -- */
				omfDDefObj_t	mediaKind,		/* IN -- */
				omfTrackID_t	trackID,			/* IN -- */
				omfTrackID_t	fileTrackID,	/* IN -- */
				char				*trackName,		/* IN -- */
				omfObject_t		fileMob)			/* IN -- */
{
	omfObject_t    subSegment = NULL, sclp = NULL;
	omfRational_t	subEditRate;
	omfLength_t		trackLength;
	omfPosition_t	zeroPos;
	omfInt32			trackLength32;
	omfSourceRef_t	ref;
	omfMSlotObj_t	newTrack = NULL, fileTrack;
	
	omfsCvtInt32toPosition(0, zeroPos);
	omfAssertValidFHdl(file);

	XPROTECT(file)
	{
		switch (file->setrev)
		{
		case kOmfRev2x:
			CHECK(FindTrackByTrackID(file, fileMob, fileTrackID, &fileTrack));
			CHECK(omfiTrackGetInfo(file, fileMob, fileTrack, &subEditRate,
								   0, NULL, NULL, 
								   NULL, &subSegment));
			CHECK(omfiComponentGetLength(file, subSegment, &trackLength));

			/* Release Bento reference, so the useCount is decremented */
			if (subSegment)
			  CMReleaseObject((CMObject)subSegment);
			break;

		case kOmfRev1x:
		case kOmfRevIMA:
			CHECK(omfsReadExactEditRate(file, fileMob, OMCPNTEditRate, 
										&subEditRate) );
			if(omfsIsPropPresent(file, fileMob, OMTRKGGroupLength, OMInt32))
			  {
				CHECK(omfsReadInt32(file, fileMob, OMTRKGGroupLength, 
								   &trackLength32));
				omfsCvtInt32toLength(trackLength32, trackLength);
			  }
			else
			  omfsCvtInt32toLength(0, trackLength);
			break;
			
		default:
			  RAISE(OM_ERR_FILEREV_NOT_SUPP);
		}
		
		CHECK(omfsReadUID(file, fileMob, OMMOBJMobID, &ref.sourceID));
		ref.sourceTrackID = fileTrackID;
		ref.startTime = zeroPos;
		CHECK(omfiSourceClipNew(file, mediaKind, trackLength, ref, &sclp));
		CHECK(omfiMobAppendNewTrack(file, masterMob, subEditRate, sclp, zeroPos,
											trackID, trackName, &newTrack));

		/* Release Bento reference, so the useCount is decremented */
		if (sclp)
		  CMReleaseObject((CMObject)sclp);
		if (newTrack)
		  CMReleaseObject((CMObject)newTrack);
	} /* XPROTECT */
	XEXCEPT
	XEND;

	return (OM_ERR_NONE);
}


/****************************************/
/******	Reference an External File	*****/
/****************************************/

/************************
 * Function:  omfmMobAddDOSLocator
 *
 * 	Add a DOS locator to the media descriptor of a file mob.
 *
 * Argument Notes:
 *		filename - Should contain the complete path needed to
 *						reference the file on a DOS system.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMobAddDOSLocator(
				omfHdl_t				file,			/* IN -- */
				omfObject_t			fileMob,		/* IN -- */
				omfFileFormat_t	isOMFI,		/* IN -- */
				char					*filename)	/* IN -- */
{
	omfObject_t     dosl = NULL, mdes = NULL;

	omfAssertValidFHdl(file);
	XPROTECT(file)
	{
		CHECK(omfsObjectNew(file, "DOSL", &dosl));
		CHECK(omfsWriteString(file, dosl, OMDOSLPathName, filename));
		if(file->setrev == kOmfRev2x)
		{
			CHECK(omfsReadObjRef(file, fileMob, OMSMOBMediaDescription,
											&mdes));
		}
		else
		{
			CHECK(omfsReadObjRef(file,fileMob, OMMOBJPhysicalMedia, &mdes));
		}
		CHECK(omfsWriteBoolean(file, mdes, OMMDFLIsOMFI, 
							   (omfBool)(isOMFI == kOmfiMedia)));
		CHECK(omfsAppendObjRefArray(file, mdes, OMMDESLocator, dosl));

		/* Release Bento reference, so the useCount is decremented */
		if (dosl)
		  CMReleaseObject((CMObject)dosl);
		if (mdes)
		  CMReleaseObject((CMObject)mdes);
	}
	XEXCEPT
	XEND;
	
	return (OM_ERR_NONE);
}

/************************
 * Function:  omfmMobAddWindowsLocator
 *
 * 	Add a Windows locator to the media descriptor of a file mob.
 *
 * Argument Notes:
 *		shortcut - Should contain the complete path needed to
 *						reference the file on a Windows system.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_NOT_IN_15 - Can't make this call to a 1.5-era file, as this
 *			class didn't yet exist.
 */
omfErr_t omfmMobAddWindowsLocator(
				omfHdl_t		file,			/* IN -- */
				omfObject_t		fileMob,		/* IN -- */
				omfFileFormat_t	isOMFI,			/* IN -- */
				char			*path,			/* IN -- */
				char			*shortcut,		/* IN -- */
				omfInt32		shortCutLen)	/* IN -- */
{
	omfObject_t     winl = NULL, mdes = NULL;
	omfPosition_t	zeroPos;
	omfErr_t			omfError;
	omfDDefObj_t	datakind;
	
	omfAssertValidFHdl(file);
	XPROTECT(file)
	{
		XASSERT((path != NULL), OM_ERR_NULL_PARAM);
		XASSERT((file->setrev == kOmfRev2x), OM_ERR_NOT_IN_15);
		omfsCvtInt32toPosition(0, zeroPos);

		CHECK(omfsObjectNew(file, "WINL", &winl));
		omfiDatakindLookup(file, STRINGKIND, &datakind, &omfError);
		CHECK(omfError);
		CHECK(omfsWriteString(file, winl, OMWINLPathName, path));
		if((shortcut != NULL) && (shortCutLen != 0))
		{
			CHECK(omfsWriteDataValue(file, winl, OMWINLShortcut, datakind, shortcut, 
										zeroPos, shortCutLen));
		}
		CHECK(omfmMobGetMediaDescription(file, fileMob, &mdes));

		CHECK(omfsWriteBoolean(file, mdes, OMMDFLIsOMFI, 
							   (omfBool)(isOMFI == kOmfiMedia)));
		CHECK(omfsAppendObjRefArray(file, mdes, OMMDESLocator, winl));

		/* Release Bento reference, so the useCount is decremented */
		if (winl)
		  CMReleaseObject((CMObject)winl);
		if (mdes)
		  CMReleaseObject((CMObject)mdes);
	}
	XEXCEPT
	XEND;
	
	return (OM_ERR_NONE);
}


/************************
 * Function:  omfmMobAddTextLocator
 *
 * 	Additional information, which could be used by a locatorFailureCallback
 *		in order to help a user locate the correct file.  For instance, a file
 *		on a removable volume might contain the location of the volume in a
 *		human-readable form.
 *
 * Argument Notes:
 *		message - A zero-terminated human-readable string.  There is no enforced 
 *			formatting beyond that.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMobAddTextLocator(
				omfHdl_t		file,			/* IN -- */
				omfObject_t fileMob,		/* IN -- */
				char			*message)	/* IN -- */
{
	omfObject_t     txtl = NULL, mdes = NULL;

	omfAssertValidFHdl(file);
	XPROTECT(file)
	{
		CHECK(omfsObjectNew(file, "TXTL", &txtl));
		CHECK(omfsWriteString(file, txtl, OMTXTLName, message));
		CHECK(omfmMobGetMediaDescription(file, fileMob, &mdes));

		CHECK(omfsAppendObjRefArray(file, mdes, OMMDESLocator, txtl));

		/* Release Bento reference, so the useCount is decremented */
		if (txtl)
		  CMReleaseObject((CMObject)txtl);
		if (mdes)
		  CMReleaseObject((CMObject)mdes);
	}
	XEXCEPT
	XEND;
	
	return (OM_ERR_NONE);
}


/************************
 * Function:  omfmMobAddMacLocator
 *
 * 	Add a Mac locator to the media descriptor of a file mob.  If you're
 *		on a Mac, this information is relatively available. If not, you
 *		should probably use another locator type.
 *
 *		The Mac system call PBGetWDInfo() is useful to get the volume name and DirID
 *		if all you have is a working directory reference number, as from the
 *		old-style standard file dialog.
 *
 *		The Mac system call PBHGetVInfo() is useful to get the volume name if
 *		you already have the vref.
 *
 * Argument Notes:
 *		volumeName - is supplied instead of vref, so that this call
 *			may potentially by made on a non-Mac system.  The 2.0 spec
 *			requires that the volume name instead of vref be stored, as
 *			the vref is heavily system-dependant.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMobAddMacLocator(
				omfHdl_t				file,				/* IN -- */
				omfObject_t			fileMob,			/* IN -- */
				omfFileFormat_t	isOMFI,			/* IN -- */
				char					*volumeName,	/* IN -- */
				omfInt32				DirID,			/* IN -- */
				char					*filename)		/* IN -- */
{
	omfObject_t     macl = NULL, mdes = NULL;

	omfAssertValidFHdl(file);
	XPROTECT(file)
	{
		CHECK(omfsObjectNew(file, "MACL", &macl));
		if(file->setrev == kOmfRev2x)
		{
			CHECK(omfsWriteString(file, macl, OMMACLVName, volumeName));
		}
		else
		{
			CHECK(omfsWriteInt16(file, macl, OMMACLVRef, 0));
		}
		CHECK(omfmMobGetMediaDescription(file, fileMob, &mdes));
		CHECK(omfsWriteInt32(file, macl, OMMACLDirID, DirID));
		CHECK(omfsWriteString(file, macl, OMMACLFileName, filename));
		CHECK(omfsWriteBoolean(file, mdes, OMMDFLIsOMFI, 
									  (omfBool)(isOMFI == kOmfiMedia)));
		CHECK(omfsAppendObjRefArray(file, mdes, OMMDESLocator, macl));

		/* Release Bento reference, so the useCount is decremented */
		if (macl)
		  CMReleaseObject((CMObject)macl);
		if (mdes)
		  CMReleaseObject((CMObject)mdes);
	}
	XEXCEPT
	XEND;
	
	return (OM_ERR_NONE);
}

/************************
 * Function:  omfmMobAddUnixLocator
 *
 * 	Add a Unix locator to the media descriptor of a file mob.
 *
 * Argument Notes:
 *		pathname - Should contain the complete path needed to
 *						reference the file on a Unix system.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMobAddUnixLocator(
				omfHdl_t				file,			/* IN -- */
				omfObject_t			fileMob,		/* IN -- */
				omfFileFormat_t	isOMFI,		/* IN -- */
				char					*pathname)	/* IN -- */
{
	omfObject_t     unxl = NULL, mdes = NULL;

	omfAssertValidFHdl(file);
	XPROTECT(file)
	{
		CHECK(omfsObjectNew(file, "UNXL", &unxl));
		CHECK(omfsWriteString(file, unxl, OMUNXLPathName, pathname));
		CHECK(omfmMobGetMediaDescription(file, fileMob, &mdes));

		CHECK(omfsWriteBoolean(file, mdes, OMMDFLIsOMFI, 
									  (omfBool)(isOMFI == kOmfiMedia)));
		CHECK(omfsAppendObjRefArray(file, mdes, OMMDESLocator, unxl));

		/* Release Bento reference, so the useCount is decremented */
		if (unxl)
		  CMReleaseObject((CMObject)unxl);
		if (mdes)
		  CMReleaseObject((CMObject)mdes);
	}
	XEXCEPT
	XEND;
	
	return (OM_ERR_NONE);
}

/************************
 * Function:  omfmMobAddNetworkLocator
 *
 * 	Add a network locator to the media descriptor of a file mob.
 *
 * Argument Notes:
 *		pathname - Should contain the complete path needed to
 *						reference the file on a Unix system.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMobAddNetworkLocator(
				omfHdl_t		file,			/* IN -- */
				omfObject_t		fileMob,		/* IN -- */
				omfFileFormat_t	isOMFI,		/* IN -- */
				char			*url)	/* IN -- */
{
	omfObject_t     netl = NULL, mdes = NULL;

	omfAssertValidFHdl(file);
	XPROTECT(file)
	{
		CHECK(omfsObjectNew(file, "NETL", &netl));
		CHECK(omfsWriteString(file, netl, OMNETLURLString, url));
		CHECK(omfmMobGetMediaDescription(file, fileMob, &mdes));

		CHECK(omfsWriteBoolean(file, mdes, OMMDFLIsOMFI, 
									  (omfBool)(isOMFI == kOmfiMedia)));
		CHECK(omfsAppendObjRefArray(file, mdes, OMMDESLocator, netl));

		/* Release Bento reference, so the useCount is decremented */
		if (netl)
		  CMReleaseObject((CMObject)netl);
		if (mdes)
		  CMReleaseObject((CMObject)mdes);
	}
	XEXCEPT
	XEND;
	
	return (OM_ERR_NONE);
}

/************************
 * Function:  omfmFileMobNew
 *
 * 	Creates a file mob and media descriptor, initializing required
 *		fields to default values.
 *
 * Argument Notes:
 *		name -- File mob names are not required, as there should always
 *					be a master mob used to reference them.
 *		codec -- A codec ID (from omfcdcid.h), used to create the correct
 *					media descriptor class.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmFileMobNew(
				omfHdl_t			file,	      /* IN -- */
				char				*name,
				omfRational_t  editRate, /* IN -- Sample Rate */
				omfCodecID_t	codec,
				omfObject_t		*result)		/* OUT -- Returned object */
{
	omfObject_t			fileMob = NULL, mdes = NULL;
	codecTable_t		codec_data;
	omfBool				found;
	omfCodecMetaInfo_t	info;
	omfLength_t			zeroLen = omfsCvtInt32toLength(0, zeroLen);
	char              codecName[MAX_NUM_CODEC_NAME_CHARS];
	omfUID_t				id;
	mobTableEntry_t	*mobPtr;
	char 				codecIDString[OMUNIQUENAME_SIZE]; /* reasonable enough */
	char				*sep, *variant;
	
	omfAssertValidFHdl(file);
	omfAssertMediaInitComplete(file);
	*result = NULL;

	XPROTECT(file)
	{
		sep = strchr(codec, ':');
		if(sep != NULL)
		{
			omfInt32	len = sep-codec;
			
			strncpy(codecIDString, codec, len);
			codecIDString[len] = '\0';
			variant = sep+1;
		}
		else
		{
			strcpy( (char*) codecIDString, codec);
			variant = NULL;
		}

		/* Create the file mob and media descriptor
		 */
		omfsTableLookupBlock(file->session->codecID, codecIDString, sizeof(codec_data),
							 &codec_data, &found);
		if(!found)
		{
			RAISE(OM_ERR_CODEC_INVALID);
		}
		CHECK(codecGetMetaInfo(file->session, &codec_data, variant, codecName, 
									  MAX_NUM_CODEC_NAME_CHARS, &info));

		CHECK(omfmRawSourceMobNew(file, name, info.mdesClassID,
								  PT_FILE_MOB, &fileMob));
								  
		if(file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
		{
			CHECK(omfsWriteExactEditRate(file, fileMob, OMCPNTEditRate, 
										 editRate));
			CHECK(omfsReadObjRef(file, fileMob, OMMOBJPhysicalMedia, &mdes));
			CHECK(omfsWriteInt32(file, mdes, OMMDFLLength, 0));
			CHECK(omfsWriteBoolean(file, mdes, OMMDFLIsOMFI, TRUE));

			/* JeffB: This will be set to the sample rate at omfmMediaCreate time */
			CHECK(omfsWriteExactEditRate(file, mdes, OMMDFLSampleRate, 
										 editRate));
			CHECK(omfsWritePhysicalMobType(file, mdes, OMMDESMobKind, 
										   PT_FILE_MOB));
		}
		else
		{
			CHECK(omfsReadObjRef(file,fileMob, OMSMOBMediaDescription, &mdes));
			CHECK(omfsWriteLength(file, mdes, OMMDFLLength, zeroLen));
			CHECK(omfsWriteBoolean(file, mdes, OMMDFLIsOMFI, TRUE));
			
			/* JeffB: This will be set to the sample rate at omfmMediaCreate time */
			CHECK(omfsWriteRational(file, mdes, OMMDFLSampleRate, editRate));

		}
		CHECK(omfsReadUID(file, fileMob, OMMOBJMobID, &id));
		mobPtr = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, id);
		if(mobPtr == NULL)
	  	{
			RAISE(OM_ERR_MISSING_MOBID);
	  	}
	  	mobPtr->createEditRate = editRate;
		
/* RPS -- store the codec ID and name in the descriptor for a couple     */
/*   of reasons... 1) this is the only way to communicate the codec ID   */
/*   from this routine to omfmMediaCreate(), which sets the codec table  */
/*   in the media handle.  2) it will be useful to store the ID in the   */
/*   file so that if an app doesn't have access to the codec, it can     */
/*   figure out what to ask for.                                         */

		CHECK(omfsWriteString(file, mdes, OMMDESCodecID, codec));
		CHECK(omfsWriteString(file, mdes, OMMDESCodecDescription, codecName));
		
/* RPS-- end change */
		
		CHECK(codecInitMDESProps(file, mdes, codecIDString, variant));
		/* Release Bento reference, so the useCount is decremented */
		if (mdes)
		  CMReleaseObject((CMObject)mdes);	
	} /* XPROTECT */
	XEXCEPT
	XEND;

	*result = fileMob;
	return (OM_ERR_NONE);
}

/************************
 * Function:  omfsFindSourceMobByID
 *
 * 	Given a mobID and an opaque file handle, returns a pointer to the
 *		mob, or NULL and an error.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_MOB_NOT_FOUND - Mob does not exist in the given file.
 */
omfErr_t omfsFindSourceMobByID(
				omfHdl_t 	file, 
				omfUID_t 	mobID, 
				omfObject_t *result)
{
	mobTableEntry_t	*entry;
	
	XPROTECT(file)
	{
		entry = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, mobID);
		if (entry == NULL)
			RAISE(OM_ERR_MOB_NOT_FOUND);
	}
	XEXCEPT
	{
		*result = NULL;
	}
	XEND
	
	*result = entry->mob;
	return (OM_ERR_NONE);
}

/****************************************/
/******		Create Source Mobs		*****/
/****************************************/

/************************
 * Function:  omfmTapeMobNew
 *
 * 	Creates a new tape mob with no timecode or reference
 *		tracks.  There is generally one tape mob for all media
 *		digitized from the same physical source tape.
 *
 * Argument Notes:
 *		name -- The name of the tape mob is generally the name of
 *					the physical tape.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADRATE -- Illegal edit rate.
 */
omfErr_t omfmTapeMobNew(
				omfHdl_t			file,		/* IN -- */
				char				*name,	/* IN -- */
				omfObject_t		*result)	/* OUT -- Returned object */
{
	omfUInt32          u_zero = 0;
	omfObject_t     tmpResult;

	XPROTECT(file)
	{
		*result = NULL;
		/*
		 * Add into high-level timecode generation abstractClipLength =
		 * LengthConvertFilmToVideo(length);
		 */

		if (file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
		{
			CHECK(omfmRawSourceMobNew(file, name, "MDES", PT_TAPE_MOB,
									&tmpResult));
		}
		else
		{
			CHECK(omfmRawSourceMobNew(file, name, "MDTP", PT_TAPE_MOB,
									&tmpResult));
		}
	} /* XPROTECT */
	XEXCEPT
	XEND;

  *result = tmpResult;
  return (OM_ERR_NONE);
}

/************************
 * Function:  omfmFilmMobNew
 *
 * 	Creates a new film mob, with no timecode track, and no
 *		source references.
 *
 * Argument Notes:
 *		name -- The name of the tape mob is generally the name of
 *					the physical tape.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADRATE -- Illegal edit rate.
 */
omfErr_t omfmFilmMobNew(
				omfHdl_t			file,	/* IN -- */
				char				*name,	/* IN -- */
				omfObject_t		*result)	/* OUT -- Returned object */
{
	XPROTECT(file)
	{
		/*
		 * Add into high-level edgecode generation abstractClipLength =
		 * vi->totalClipLength;
		 */
		CHECK(omfmRawSourceMobNew(file, name, "MDFM", PT_FILM_MOB, result));
	}
	XEXCEPT
	XEND
	
	/*
	 * Add into high-level edgecode generation
	 * OMPutTRKGGroupLength(filmMob, &abstractClipLength);
	 */
	return (OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiFilmMobSetDescriptor()
 *
 *      Given a film mob, this function sets the optional properties
 *      on the film media descriptor (MDFM) that resides on the film
 *      mob.  Each argument is optional, and a NULL should be passed
 *      when not used.
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
omfErr_t omfmFilmMobSetDescriptor(
		  omfHdl_t file,             /* IN - File Handle */
		  omfMobObj_t filmMob,       /* IN - Film Mob */
		  omfFilmType_t *filmFormat, /* IN - Film Format (optional) 
												*      (kFt35MM, kFt16MM, kFt8MM, kFt65MM)
												*/
		  omfUInt32 *frameRate,      /* IN - Frame Rate (optional) */
		  omfUInt8 *perfPerFrame,    /* IN - Perforations per frame
												*      (optional)
												*/
		  omfRational_t *aspectRatio,/* IN - Film Aspect Ratio (optional) */
		  omfString manufacturer,    /* IN - Film Manufacturer (optional) */
		  omfString model)           /* IN - Manufacturer's Brand (optional) */
{
   omfObject_t descriptor = NULL;
   omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((filmMob != NULL), file, OM_ERR_NULLOBJECT);

  XPROTECT(file)
	 {
		if (!omfiIsAFilmMob(file, filmMob, &omfError))
		  {
			 RAISE(OM_ERR_INVALID_MOBTYPE);
		  }

		/* Get the descriptor */
      CHECK(omfsReadObjRef(file, filmMob, OMSMOBMediaDescription, 
									&descriptor));

		if (filmFormat)
		  {
			 CHECK(omfsWriteFilmType(file, descriptor, OMMDFMFilmFormat,
											 *filmFormat));
		  }
		if (frameRate)
		  {
			 CHECK(omfsWriteUInt32(file, descriptor, OMMDFMFrameRate, *frameRate));
		  }
		if (perfPerFrame)
		  {
			 CHECK(omfsWriteUInt8(file, descriptor, OMMDFMPerforationsPerFrame,
										 *perfPerFrame));
		  }
		if (aspectRatio)
		  {
			 CHECK(omfsWriteRational(file, descriptor, OMMDFMFilmAspectRatio,
											 *aspectRatio));
		  }
		if (manufacturer)
		  {
			 CHECK(omfsWriteString(file, descriptor, OMMDFMManufacturer, 
										  manufacturer));
		  }
		if (model)
		  {
			 CHECK(omfsWriteString(file, descriptor, OMMDFMModel, model));
		  }

		/* Release Bento reference, so the useCount is decremented */
		if (descriptor)
		  CMReleaseObject((CMObject)descriptor);	
	 }

  XEXCEPT
	 {
	 }
  XEND;

  return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: omfiTapeMobSetDescriptor()
 *
 *      Given a tape mob, this function sets the optional properties
 *      on the tape media descriptor (MDTP) that resides on the tape
 *      mob.  Each argument is optional, and a NULL should be passed
 *      when not used.
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
omfErr_t omfmTapeMobSetDescriptor(
			omfHdl_t file,                 /* IN - File Handle */
         omfMobObj_t tapeMob,           /* IN - Tape Mob */
         omfTapeCaseType_t *formFactor, /* IN - Form Factor (optional) */
			omfVideoSignalType_t *videoSignal, /* IN - Video Signal (optional) */
         omfTapeFormatType_t *tapeFormat, /* IN - Tape Format (optional) */
         omfLength_t *length,           /* IN - Length in minutes (optional) */
         omfString manufacturer,        /* IN - Manufacturer (optional) */
         omfString model)               /* IN - Model Brand Name (optional) */
{
   omfObject_t descriptor = NULL;
	omfUInt32 tmpLength;
   omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((tapeMob != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		if (!omfiIsATapeMob(file, tapeMob, &omfError))
		  {
			 RAISE(OM_ERR_INVALID_MOBTYPE);
		  }
		/* Get the descriptor */
      CHECK(omfsReadObjRef(file, tapeMob, OMSMOBMediaDescription, 
									&descriptor));

		 if (formFactor)
			{
			  CHECK(omfsWriteTapeCaseType(file, descriptor, OMMDTPFormFactor,
													*formFactor));
			}
		 if (videoSignal)
			{
			  CHECK(omfsWriteVideoSignalType(file, descriptor, OMMDTPVideoSignal,
														*videoSignal));
			}
		 if (tapeFormat)
			{
			  CHECK(omfsWriteTapeFormatType(file, descriptor, OMMDTPTapeFormat,
													  *tapeFormat));
			}
		 if (length)
			{
			  CHECK(omfsTruncInt64toUInt32(*length, &tmpLength));		/* OK FRAMEOFFSET */
			  CHECK(omfsWriteUInt32(file, descriptor, OMMDTPLength, tmpLength));
/*			  CHECK(omfsWriteLength(file, descriptor, OMMDTPLength, *length)); */
			}
		 if (manufacturer)
			{
			  CHECK(omfsWriteString(file, descriptor, OMMDTPManufacturer, 
											manufacturer));
			}
		 if (model)
			{
			  CHECK(omfsWriteString(file, descriptor, OMMDTPModel, model));
			}

		/* Release Bento reference, so the useCount is decremented */
		if (descriptor)
		  CMReleaseObject((CMObject)descriptor);	
	  }

	XEXCEPT
	  {
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiFilmMobGetDescriptor()
 *
 *      Given a film mob, this function gets the optional properties
 *      on the film media descriptor (MDFM) that resides on the film
 *      mob.  Each argument is optional, and a NULL should be passed
 *      when not desired.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *    NULL should be passed for any argument that is not requested.
 *    The string numbers (manufacturer and model) must be preallocated
 *    buffers of the specified sizes.  If the name is longer than the buffer
 *    provided, it will truncate the string to fit into the buffer.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfmFilmMobGetDescriptor(
		  omfHdl_t file,             /* IN - File Handle */
		  omfMobObj_t filmMob,       /* IN - Film Mob */
		  omfFilmType_t *filmFormat, /* OUT - Film Format 
												*      (kFt35MM, kFt16MM, kFt8MM, kFt65MM)
												*/
		  omfUInt32 *frameRate,      /* OUT - Frame Rate */
		  omfUInt8 *perfPerFrame,    /* OUT - Perforations per frame */
		  omfRational_t *aspectRatio,/* OUT - Film Aspect Ratio  */
		  omfInt32 manuSize,         /* IN - String size for manufact buffer */
		  omfString manufacturer,    /* OUT - Film Manufacturer */
		  omfInt32 modelSize,        /* IN - String size for model buffer */
		  omfString model)           /* OUT - Manufacturer's Brand */
{
   omfObject_t descriptor = NULL;
   omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((filmMob != NULL), file, OM_ERR_NULLOBJECT);

  XPROTECT(file)
	 {
		if (!omfiIsAFilmMob(file, filmMob, &omfError))
		  {
			 RAISE(OM_ERR_INVALID_MOBTYPE);
		  }

		/* Get the descriptor */
      CHECK(omfsReadObjRef(file, filmMob, OMSMOBMediaDescription, 
									&descriptor));

		if (filmFormat)
		  {
			 omfError = omfsReadFilmType(file, descriptor, OMMDFMFilmFormat,
											 filmFormat);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				*filmFormat = kFtNull;
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
		  }
		if (frameRate)
		  {
			 omfError = omfsReadUInt32(file, descriptor, OMMDFMFrameRate, 
											  frameRate);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				*frameRate = 0;
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
		  }
		if (perfPerFrame)
		  {
			 omfError = omfsReadUInt8(file, descriptor, 
											  OMMDFMPerforationsPerFrame, perfPerFrame);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				*perfPerFrame = 0;
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
		  }
		if (aspectRatio)
		  {
			 omfError = omfsReadRational(file, descriptor, OMMDFMFilmAspectRatio,
											 aspectRatio);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				{
				  (*aspectRatio).numerator = 0;
				  (*aspectRatio).denominator = 0;
				}
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
		  }
		if (manufacturer)
		  {
			 omfError = omfsReadString(file, descriptor, OMMDFMManufacturer, 
										  manufacturer, manuSize);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				{
				  strcpy( (char*) manufacturer, "\0");
				}
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
		  }
		if (model)
		  {
			 omfError = omfsReadString(file, descriptor, OMMDFMModel, model,
										 modelSize);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				{
				  strcpy( (char*) model, "\0");
				}
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
		  }

		/* Release Bento reference, so the useCount is decremented */
		if (descriptor)
		  CMReleaseObject((CMObject)descriptor);	
	 }

  XEXCEPT
	 {
	 }
  XEND;

  return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: omfiTapeMobGetDescriptor()
 *
 *      Given a tape mob, this function gets the optional properties
 *      on the tape media descriptor (MDTP) that resides on the tape
 *      mob.  Each argument is optional, and a NULL should be passed
 *      when not desired.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *    NULL should be passed for any argument that is not requested.
 *    The string numbers (manufacturer and model) must be preallocated
 *    buffers of the specified sizes.  If the name is longer than the buffer
 *    provided, it will truncate the string to fit into the buffer.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfmTapeMobGetDescriptor(
			omfHdl_t file,                 /* IN - File Handle */
         omfMobObj_t tapeMob,           /* IN - Tape Mob */
         omfTapeCaseType_t *formFactor, /* OUT - Form Factor */
			omfVideoSignalType_t *videoSignal, /* OUT - Video Signal */
         omfTapeFormatType_t *tapeFormat, /* OUT - Tape Format */
         omfLength_t *length,       /* OUT - Length in minutes */
			omfInt32 manuSize,         /* IN - String size for manufact buffer */
         omfString manufacturer,    /* OUT - Manufacturer */
		   omfInt32 modelSize,        /* IN - String size for model buffer */
         omfString model)           /* OUT - Model Brand Name */
{
   omfObject_t descriptor = NULL;
	omfUInt32 tmpLength;
   omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((tapeMob != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		if (!omfiIsATapeMob(file, tapeMob, &omfError))
		  {
			 RAISE(OM_ERR_INVALID_MOBTYPE);
		  }
		/* Get the descriptor */
      CHECK(omfsReadObjRef(file, tapeMob, OMSMOBMediaDescription, 
									&descriptor));

		 if (formFactor)
			{
			  omfError = omfsReadTapeCaseType(file, descriptor, OMMDTPFormFactor,
													formFactor);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				{
				  *formFactor = kTapeCaseNull;
				}
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
			}
		 if (videoSignal)
			{
			  omfError = omfsReadVideoSignalType(file, descriptor, 
															 OMMDTPVideoSignal, videoSignal);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				{
				  *videoSignal = kVideoSignalNull;
				}
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
			}
		 if (tapeFormat)
			{
			  omfError = omfsReadTapeFormatType(file, descriptor, 
															OMMDTPTapeFormat, tapeFormat);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				{
				  *tapeFormat = kTapeFormatNull;
				}
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
			}
		 if (length)
			{
/*			  omfError = omfsReadLength(file, descriptor, OMMDTPLength, length); */
			  omfError = omfsReadUInt32(file, descriptor, OMMDTPLength, 
												 &tmpLength);
			  omfsCvtUInt32toInt64(tmpLength, length);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				{
				  omfsCvtInt32toLength(0, (*length));
				}
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
			}
		 if (manufacturer)
			{
			  omfError = omfsReadString(file, descriptor, OMMDTPManufacturer, 
											manufacturer, manuSize);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				{
				  strcpy( (char*) manufacturer, "\0");
				}
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
			}
		 if (model)
			{
			  omfError = omfsReadString(file, descriptor, OMMDTPModel, model, 
										  modelSize);
			 if (omfError == OM_ERR_PROP_NOT_PRESENT)
				{
				  strcpy( (char*) model, "\0");
				}
			 else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
			}

		/* Release Bento reference, so the useCount is decremented */
		if (descriptor)
		  CMReleaseObject((CMObject)descriptor);	
	  }

	XEXCEPT
	  {
	  }
	XEND;

	return(OM_ERR_NONE);
}
			
/************************
 * Function:  omfmMobAddPhysSourceRef
 *
 * 	Adds a reference to a physical source mob to an existing mob.
 *		For instance, file mobs may have references to tracks on tape
 *		mobs, which reference tracks on film mobs.
 *
 * Argument Notes:
 *		aMob - The referencing mob (ie: file or tape mob).
 *		aMobTrack - The track ID to be added to the referencing mob.
 *		mediaKind - The media type data definition object.
 *		srcRefObj - The referenced mob (ie: tape of film mob)
 *		srcRefOffset - The position in the referenced mob which maps to
 *							zero in the referencing mob.
 *		srcRefTrack - The track ID being referenced.  Usuallly, this is
 *						the same value as aMobTrack.
 *		srcRefLength - The length of the media being referenced.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMobAddPhysSourceRef(
				omfHdl_t			file,
				omfObject_t		aSourceMob,
				omfRational_t	editrate,	/* IN -- */
				omfTrackID_t	aMobTrack,
				omfDDefObj_t	mediaKind,
				omfObject_t		sourceRefObj,
				omfPosition_t	srcRefOffset,
				omfInt32			srcRefTrack,
				omfLength_t		srcRefLength)
{
	omfObject_t	seg = NULL, track = NULL;
	omfErr_t	status = OM_ERR_NONE;
	omfSourceRef_t	ref;
	omfPosition_t	zeroPos;
	omfTrackID_t tmpTrackID;
	omfObject_t		sclp = NULL, trkd = NULL;
	
	omfAssertValidFHdl(file);
	
	XPROTECT(file)
	{
		if (sourceRefObj)
		{
			omfsCvtInt32toPosition(0, zeroPos);
			CHECK(omfsReadUID(file, sourceRefObj, OMMOBJMobID, &ref.sourceID));
			ref.sourceTrackID = srcRefTrack;
			ref.startTime = srcRefOffset;
				
			status = FindTrackByTrackID(file, aSourceMob, aMobTrack, &track);
			if (status == OM_ERR_NONE)
			{
				CHECK(omfiTrackGetInfo(file, aSourceMob, track, NULL, 0, NULL, NULL, 
									   &tmpTrackID, &seg));
				if(omfiIsASequence(file, seg, &status))
				{
					omfPosition_t	foundPos;
					omfLength_t		foundLen;
					
					CHECK(MobFindCpntByPosition(file, aSourceMob, aMobTrack, seg, zeroPos, zeroPos,
														NULL, &foundPos, &seg, &foundLen));
				}
				XASSERT(omfiIsASourceClip(file, seg, &status), OM_ERR_NOT_SOURCE_CLIP);
				CHECK(omfiSourceClipSetRef(file, seg, ref));
			}
			else
			{
				CHECK(omfiSourceClipNew(file, mediaKind, srcRefLength, ref, &sclp));
				CHECK(omfiMobAppendNewTrack(file, aSourceMob, editrate, sclp,
									zeroPos, aMobTrack, NULL, &trkd) );
			}

		}
		else
		{
			CHECK(omfmMobAddNilReference(file, aSourceMob, aMobTrack, 
										  srcRefLength, mediaKind,  editrate));
		}
	} /* XPROTECT */
	XEXCEPT
	XEND;

	return (OM_ERR_NONE);
}

/************************
 * Function:  omfmMobAddPulldownRef
 *
 * 	Adds a reference to a physical source mob to an existing mob.
 *		For instance, file mobs may have references to tracks on tape
 *		mobs, which reference tracks on film mobs.
 *
 * Argument Notes:
 *		aMob - The referencing mob (ie: file or tape mob).
 *		aMobTrack - The track ID to be added to the referencing mob.
 *		mediaKind - The media type data definition object.
 *		srcRefObj - The referenced mob (ie: tape of film mob)
 *		srcRefOffset - The position in the referenced mob which maps to
 *							zero in the referencing mob.
 *		srcRefTrack - The track ID being referenced.  Usuallly, this is
 *						the same value as aMobTrack.
 *		srcRefLength - The length of the media being referenced.
 *		pulldownKind - The kind of pulldown, one of the following values.
 *							kOMFTwoThreePD,	kOMFPALPD,	kOMFOneToOneNTSC,
 *							kOMFOneToOnePAL
 *		phaseFrame	- Specified which video frame in the pulldown the media
 *						starts with.
 *		phaseField - Can hold the values:
 *						kOMFPhaseFieldUnknown, kOMFPhaseField1,
 *						kOMFPhaseField2
 *		direction - Can hold the values:
 *						kOMFTapeToFilmSpeed,
 *						kOMFFilmToTapeSpeed
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMobAddPulldownRef(
				omfHdl_t			file,
				omfObject_t			aSourceMob,
				omfRational_t		editrate,			/* IN -- */
				omfTrackID_t		aMobTrack,
				omfDDefObj_t		mediaKind,
				omfObject_t			sourceRefObj,
				omfPosition_t		srcRefOffset,
				omfInt32			srcRefTrack,
				omfLength_t			srcRefLength,
				omfPulldownKind_t	pulldownKind,
				omfPhaseFrame_t		phaseFrame,
				omfPulldownDir_t	direction
				)
{
	omfObject_t		sclp = NULL, trkd = NULL;
	omfSourceRef_t	ref;
	omfPosition_t	zeroPos;
	omfIterHdl_t 	trackIter = NULL;
	omfTrackID_t 	tmpTrackID;
	omfObject_t		track = NULL;
	omfBool 		foundTrack = FALSE, isOneToOne;
	omfObject_t 	maskTrack = NULL, pdwn = NULL;
	omfDDefObj_t	datakind = NULL;
	omfLength_t 	length, outLength, zero;
	omfUInt32		maskbits; /* = 3623878656 for 3-2 */
	omfRational_t	tapeRate, filmRate;
	omfInt32 		type, patternLen;
	omfUInt32		tmp1xLength, vidLength32;
	omfLength_t		vidLength64;
	omfErr_t		status = OM_ERR_NONE;
	omfProperty_t	prop, subProp;
	omfUInt32 		mask;
	
	omfAssertValidFHdl(file);
	
	XPROTECT(file)
	{
		omfsCvtInt32toInt64(0, &zero);
		XASSERT(sourceRefObj != NULL, OM_ERR_NULLOBJECT);
		XASSERT(direction == kOMFFilmToTapeSpeed || direction == kOMFTapeToFilmSpeed,
				OM_ERR_PULLDOWN_DIRECTION);
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			XASSERT(pulldownKind == kOMFTwoThreePD ||
					pulldownKind == kOMFPALPD, OM_ERR_NOT_IN_15);
		}

		omfsCvtInt32toPosition(0, zeroPos);
		CHECK(omfsReadUID(file, sourceRefObj, OMMOBJMobID, &ref.sourceID));
		ref.sourceTrackID = srcRefTrack;
		ref.startTime = srcRefOffset;

		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			prop = OMSEQUSequence;
			subProp = OMCLIPLength;
		}
		else
		{
			prop = OMSEQUComponents;
			subProp = OMCPNTLength;
		}

		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			/* Add a mask object to the file mob to convert the edit rate to
			 * the video tape's rate.
			 */
			CHECK(omfPvtGetPulldownMask(file, pulldownKind, &maskbits,
										&patternLen, &isOneToOne));
			if (direction == kOMFFilmToTapeSpeed)
			{
				CHECK(omfsObjectNew(file, "MASK", &pdwn));
				CHECK(omfsWriteUInt32(file, pdwn, OMMASKMaskBits, 
												maskbits));
				CHECK(omfsWriteInt16(file, pdwn, OMWARPPhaseOffset, phaseFrame));
				CHECK(omfsWriteBoolean(file, pdwn, OMMASKIsDouble, 0));
				CHECK(omfsWriteString(file, pdwn, OMCPNTEffectID,
							  "EFF_TIMEWARP.CAPTURE_MASK"));
				CHECK(omfsWriteExactEditRate(file, pdwn, OMCPNTEditRate, 
														 editrate));
				/* Create the track for the MASK, so the embedded source
				 * clip can be added below.
				 */
				CHECK(omfsObjectNew(file, "TRAK", &maskTrack));
				CHECK(omfsWriteInt16(file, maskTrack, OMTRAKLabelNumber, 1));
				CHECK(omfsAppendObjRefArray(file, pdwn, OMTRKGTracks, 
														maskTrack));
				/* Write the length and datakind below */
			}

			else /* if (direction == kOMFTapeToFilmSpeed)	*/
			{
				if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			 	 {
					  CHECK(omfsObjectNew(file, "MASK", &pdwn));
					  CHECK(omfsWriteUInt32(file, pdwn, OMMASKMaskBits, 
													maskbits));
					  CHECK(omfsWriteInt16(file, pdwn, OMWARPPhaseOffset, phaseFrame));
					  CHECK(omfsWriteBoolean(file, pdwn, OMMASKIsDouble, 1));
					  CHECK(omfsWriteString(file, pdwn, OMCPNTEffectID,
								  "EFF_TIMEWARP.CAPTURE_MASK"));
					  CHECK(omfsWriteExactEditRate(file, pdwn, OMCPNTEditRate, 
															 editrate));
					  /* Create the track for the MASK, so the embedded source
						* clip can be added below.
						*/
					  CHECK(omfsObjectNew(file, "TRAK", &maskTrack));
					  CHECK(omfsWriteInt16(file, maskTrack, OMTRAKLabelNumber, 1));
					  CHECK(omfsAppendObjRefArray(file, pdwn, OMTRKGTracks, 
															maskTrack));
					/* Write the length and datakind below */
				}
			}
		}
		else
		{
			CHECK(omfsObjectNew(file, "PDWN", &pdwn));
			CHECK(omfsWriteObjRef(file, pdwn, OMCPNTDatakind, mediaKind));
			CHECK(omfsWritePulldownKindType(file, pdwn, OMPDWNPulldownKind, pulldownKind));
			CHECK(omfsWritePhaseFrameType(file, pdwn, OMPDWNPhaseFrame, phaseFrame));
			CHECK(omfsWritePulldownDirectionType(file, pdwn, OMPDWNDirection, direction));
			CHECK(omfPvtGetPulldownMask(file, pulldownKind,
										&mask,  &patternLen, &isOneToOne));

			if(isOneToOne)
			{
				CHECK(omfsWriteLength(file, pdwn, OMCPNTLength, srcRefLength));
			}
			else
			{
				/* Remember, this routine is given the OUTPUT length, and must determine
				 * the input length (so the ratios look backwards)
				 */
				CHECK(omfiPulldownMapOffset(file, pdwn, srcRefLength, TRUE, &outLength, NULL));
				CHECK(omfsWriteLength(file, pdwn, OMCPNTLength, outLength));
			}
		}
			
		/* If the track exists, and there is a SCLP, extract it so that it can be appended
		 * to the mask or pullown object later
		 */
		status = FindTrackByTrackID(file, aSourceMob, aMobTrack, &track);
		if (status == OM_ERR_NONE)
		{
			omfObject_t	seg;
			
			CHECK(omfiTrackGetInfo(file, aSourceMob, track, NULL, 0, NULL, NULL, 
								   &tmpTrackID, &seg));
			if(omfiIsASequence(file, seg, &status))
			{
				omfLength_t		foundLen;
				omfInt32		numSegments, n;
				omfObject_t		subSeg;
				
				numSegments = omfsLengthObjRefArray(file, seg, prop);
				if(numSegments == 0)
				{
					CHECK(omfsPutNthObjRefArray(file, seg, prop, pdwn, 1));
					CHECK(omfiSourceClipNew(file, mediaKind, srcRefLength, ref, &sclp));
				}
				for(n = 1; n <= numSegments; n++)
				{
					CHECK(omfsGetNthObjRefArray(file, seg, prop, &subSeg, n));
					CHECK(omfsReadLength(file, subSeg, subProp, &foundLen));

					if(omfsInt64NotEqual(foundLen, zero))
					{
						if ((file->setrev != kOmfRev1x) && (file->setrev != kOmfRevIMA))
						{
							CHECK(omfsWriteLength(file, seg, OMCPNTLength, outLength));
						}
						CHECK(omfsPutNthObjRefArray(file, seg, prop,
										pdwn, n));
						sclp = subSeg;
						break;
					}
				}
			}
			else
			{
				if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
					prop = OMTRAKTrackComponent;
				else
					prop = OMMSLTSegment;
				CHECK(omfsWriteObjRef(file, track, prop, pdwn));
				sclp = seg;
			}
			
			XASSERT(omfiIsASourceClip(file, sclp, &status), OM_ERR_NOT_SOURCE_CLIP);
			CHECK(omfiSourceClipSetRef(file, sclp, ref));
			CHECK(omfsWriteLength(file, sclp, subProp, srcRefLength));
		}
		else
		{
			CHECK(omfiSourceClipNew(file, mediaKind, srcRefLength, ref, &sclp));
			CHECK(omfiMobAppendNewTrack(file, aSourceMob, editrate, pdwn,
								zeroPos, aMobTrack, NULL, &trkd) );
		}

		/* Patch the MASK into the file mob if this is a Film Editrate */
		/* NOTE: This is assuming that there is a source clip on
		 * the file mob - if it is a nested structure (i.e., SEQU),
		 * this code is not patching the nested elements with the new
		 * editrate and length.
		 */
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		  {
			if (direction == kOMFTapeToFilmSpeed)
				{
				  /* Finish writing mask properties */
				  CHECK(omfiComponentGetInfo(file, sclp, &datakind, &length));
				  type = MediaKindToTrackType(file, datakind);
				  CHECK(omfsWriteTrackType(file, pdwn, OMCPNTTrackKind, 
							 (omfInt16)type));
				  CHECK(omfsTruncInt64toUInt32(length, &tmp1xLength));	/* OK 1.x */
				  CHECK(omfsWriteInt32(file, pdwn, OMTRKGGroupLength,
											  tmp1xLength));

				  /* Change the editrate and length on the sclp */
				  CHECK(omfsReadExactEditRate(file, sourceRefObj, OMCPNTEditRate, 
										&filmRate) );
				  CHECK(omfsWriteExactEditRate(file, sclp, OMCPNTEditRate, 
								 filmRate));
				  CHECK(omfiPulldownMapOffset(file, pdwn, length, FALSE, &vidLength64, NULL));
				  CHECK(omfsTruncInt64toUInt32(vidLength64, &vidLength32));	/* OK 1.x */
				  CHECK(omfsWriteInt32(file, sclp, OMCLIPLength, vidLength32));

				  /* Add sclp to the MASK's track */
				  CHECK(omfsWriteObjRef(file, maskTrack, 
												OMTRAKTrackComponent, sclp)); 
				}
		 	if (direction == kOMFFilmToTapeSpeed)
			{
				/* Finish writing mask properties */
				CHECK(omfiComponentGetInfo(file, sclp, &datakind, &length));
				type = MediaKindToTrackType(file, datakind);
				CHECK(omfsWriteTrackType(file, pdwn, OMCPNTTrackKind, 
							 (omfInt16)type));
				CHECK(omfsTruncInt64toUInt32(length, &tmp1xLength));	/* OK 1.x */
				CHECK(omfsWriteInt32(file, pdwn, OMTRKGGroupLength,
											  tmp1xLength));

				/* Change the editrate and length on the sclp */
				CHECK(omfsReadExactEditRate(file, sourceRefObj, OMCPNTEditRate, 
										&tapeRate) );
				CHECK(omfsWriteExactEditRate(file, sclp, OMCPNTEditRate, 
								 tapeRate));
				CHECK(omfiPulldownMapOffset(file, pdwn, length, FALSE, &vidLength64, NULL));
				CHECK(omfsTruncInt64toUInt32(vidLength64, &vidLength32));	/* OK 1.x */
				CHECK(omfsWriteInt32(file, sclp, OMCLIPLength, vidLength32));

				/* Add sclp to the MASK's track */
				CHECK(omfsWriteObjRef(file, maskTrack, 
												OMTRAKTrackComponent, sclp)); 
			}
		}
		else	/* 2.x case */
		{
			if(pdwn != NULL)
			{
				CHECK(omfsWriteObjRef(file, pdwn, OMPDWNInputSegment, sclp));
			}
		}

	} /* XPROTECT */
	XEXCEPT
	XEND;

	return (OM_ERR_NONE);
}

/************************
 * Function:  omfmRawSourceMobNew
 *
 * 	This is the routine upon which omfmTapeMobNew and omfmFilmMobNew
 *		are built.  If a tape mob or film mob are required, then these
 *		routines should be used instead.
 *
 *		If a new type of source mob is needed beyond those supplied with
 *		the toolkit, then a wrapper function should be written which calls
 *		this routine to build underlying data structures.
 *
 * Argument Notes:
 *		mdesClass - This class ID can be either registered or private.
 *		mobType - Required to write/read 1.5 types.  Set to PT_NULL for
 *					new classes.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmRawSourceMobNew(
				omfHdl_t 				file,			/* IN -- */
				char 						*name,		/* IN -- */
				omfClassIDPtr_t 		mdesClass,	/* IN -- */
				omfPhysicalMobType_t mobType,		/* IN -- */
				omfObject_t				*result)		/* OUT -- Returned object */
{
	omfInt32           zeroL = 0L;
	omfObject_t     srcMob = NULL, srcDesc = NULL;
	omfObjIndexElement_t ident;
	omfUID_t        id;
	omfTimeStamp_t  mod_time;

	XPROTECT(file)
	{
		*result = NULL;

		omfsGetDateTime(&mod_time);
		CHECK(omfiMobIDNew(file, &id));

		if (mdesClass != NULL)
		{
			CHECK(omfsObjectNew(file, mdesClass, &srcDesc));
		}

		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			CHECK(omfsObjectNew(file, "MOBJ", &srcMob));
			CHECK(omfsWriteTrackType(file, srcMob, OMCPNTTrackKind, 
									 TT_NULL));
			CHECK(omfsWriteUsageCodeType(file, srcMob, 
										 OMMOBJUsageCode, 0));
			CHECK(omfsWriteInt32(file, srcMob, OMMOBJStartPosition, 0));
	
	
			/* Done adding the SCLP reference */
			if (name != NULL)
			{
				CHECK(omfsWriteString(file, srcMob, OMCPNTName, name));
			}
			if(mdesClass != NULL)
			{
				CHECK(omfsWritePhysicalMobType(file, srcDesc, 
													OMMDESMobKind, mobType));
				CHECK(omfsWriteObjRef(file, srcMob, OMMOBJPhysicalMedia, 
										   srcDesc));
			}
			ident.ID = id;
			ident.Mob = srcMob;
			CHECK(omfsAppendObjIndex(file, file->head, OMSourceMobs, &ident));
			CHECK(omfsAppendObjRefArray(file, file->head, OMObjectSpine, 
										srcMob));
		}
		else
		{
			CHECK(omfsObjectNew(file, "SMOB", &srcMob));
			if (name != NULL)
			{
				CHECK(omfsWriteString(file, srcMob, OMMOBJName, name));
			}
			CHECK(omfsWriteTimeStamp(file, srcMob, OMMOBJCreationTime, 
										  mod_time));
			if(mdesClass != NULL)
			{
				CHECK(omfsWriteObjRef(file, srcMob, OMSMOBMediaDescription, 
									  srcDesc));
			}
			CHECK(omfsAppendObjRefArray(file, file->head, OMHEADMobs, srcMob));
		}
		/* Release Bento reference, so the useCount is decremented */
		if (srcDesc)
		  CMReleaseObject((CMObject)srcDesc);	

		CHECK(omfsWriteTimeStamp(file, srcMob, OMMOBJLastModified, 
								 mod_time));
		CHECK(omfsWriteUID(file, srcMob, OMMOBJMobID, id));
		CHECK(AddMobTableEntry(file, srcMob,id, kOmTableDupError));

	} /* XPROTECT */
	XEXCEPT
	XEND;

	*result = srcMob;
	return (OM_ERR_NONE);
}

/****************************************/
/******		Timecode Object			*****/
/****************************************/

/*************************************************************************
 * Function: omfmTimecodeToOffset()
 *
 *      This function takes a timecode and source mob and track as input
 *      and returns the position in the track that corresponds to the
 *      timecode value.  This is accomplished by searching for the tape
 *      mob with the timecode and calculating the offset in the given
 *      source mob.
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
omfErr_t omfmTimecodeToOffset( 
   omfHdl_t	file,
	omfTimecode_t timecode,          /* IN - The timecode value */
	omfMobObj_t	sourceMob,      /* IN - Source Mob */
    omfTrackID_t trackID,       /* IN - Track ID of track in source mob */
	omfFrameOffset_t *result)   /* OUT - Resulting offset in source track */
{
  omfIterHdl_t iterHdl = NULL, sequIter = NULL;
  omfObject_t seg = NULL, slot = NULL, tccp = NULL, pdwn = NULL, tmpSlot = NULL;
  omfObject_t	pdwnInput = NULL, subSegment;
  omfInt32 numSlots, loop, segStart, sequLoop, numSegs, start32, frameOffset;
  omfInt32		segLen32;
  omfPosition_t zero, sequPos, newStart, oldStart;
  omfFrameOffset_t	begPos, endPos;
  omfTimecode_t	startTC;
  omfErr_t	omfError = OM_ERR_NONE;
  omfBool found = FALSE;
  omfRational_t	editRate;
  omfLength_t	segLen, zeroLen, tcLen;
  omfFindSourceInfo_t	sourceInfo;

  omfAssertValidFHdl(file);
  
  omfsCvtInt32toPosition(0, zero);
  omfsCvtInt32toLength(0, zeroLen);
  sourceInfo.mob = NULL;
  
  XPROTECT(file)
	{
     // MC 01-Dec-03 -- fix for crash
     omfEffectChoice_t effectChoice = kFindEffectSrc1;
	  CHECK(omfiMobSearchSource(file, sourceMob, trackID, zero, kTapeMob,
							  NULL /* mediaCrit */, &effectChoice, NULL, &sourceInfo));

	  CHECK(omfiIteratorAlloc(file, &iterHdl));
	  CHECK(omfiMobGetNumSlots(file, sourceInfo.mob, &numSlots));
   for (loop = 1; (loop <= numSlots) & !found; loop++)
		{
			CHECK(omfiMobGetNextSlot(iterHdl, sourceInfo.mob, NULL, &slot));
			CHECK(omfiMobSlotGetInfo(file, slot, &editRate, &seg));
			segStart = 0;

			if (omfiIsATimecodeClip(file, seg, &omfError))
			  {
				 CHECK(omfiTimecodeGetInfo(file, seg, NULL, NULL, &startTC));
				CHECK(omfiComponentGetLength(file, seg, &tcLen));
				 found = TRUE;

			  } /* If timecode clip */
		  else if (omfiIsASequence(file, seg, &omfError))
			{
	  			CHECK(omfiIteratorAlloc(file, &sequIter));
	  			CHECK(omfiSequenceGetNumCpnts(file, seg, &numSegs));
				for (sequLoop=0; sequLoop < numSegs; sequLoop++)
				{
					CHECK(omfiSequenceGetNextCpnt(sequIter, seg,
													NULL, &sequPos, &subSegment));
					if (omfsIsTypeOf(file, subSegment, "MASK", &omfError))
					{
						pdwn = subSegment;
						CHECK(omfsGetNthObjRefArray(file, pdwn, OMTRKGTracks,
													  &tmpSlot, 1));
						CHECK(omfiMobSlotGetInfo(file, tmpSlot, NULL, &subSegment));
					}
					else if (omfsIsTypeOf(file, subSegment, "PDWN", &omfError))
					{
						pdwn = subSegment;
						CHECK(omfsReadObjRef(file, pdwn, OMPDWNInputSegment, &subSegment));
					}
					CHECK(omfiComponentGetLength(file, subSegment, &segLen));
		  			CHECK(omfsTruncInt64toInt32(segLen, &segLen32));	/* OK FRAMEOFFSET */
					/* Skip zero-length clips, sometimes found in MC files */
					if (segLen32 == 0)
						continue;
			 		CHECK(omfiTimecodeGetInfo(file, subSegment, NULL, NULL, &startTC));
					begPos = startTC.startFrame;
					endPos = startTC.startFrame + segLen32;
					if ((timecode.startFrame < endPos) && (begPos <= timecode.startFrame))
					{
		  				CHECK(omfsTruncInt64toInt32(sequPos, &segStart));	/* OK FRAMEOFFSET */
						CHECK(omfiComponentGetLength(file, subSegment, &tcLen));
						found = TRUE;
						break;
					}
				} /* for */
				CHECK(omfiIteratorDispose(file, sequIter));
				sequIter = NULL;
				
				/* Release Bento reference, so the useCount is decremented */
				if (subSegment)
				  {
					 CMReleaseObject((CMObject)subSegment);	
					 subSegment = NULL;
				  }
			  } /* If Sequence */
			  

			/* Release Bento reference, so the useCount is decremented */
			if (seg)
			  {
				 CMReleaseObject((CMObject)seg);	
				 seg = NULL;
			  }
			if (slot)
			  {
				 CMReleaseObject((CMObject)slot);	
				 slot = NULL;
			  }
		 } /* for */

	 if(!found)
	  {
		 RAISE(OM_ERR_TIMECODE_NOT_FOUND);
	  }

	  /* Assume found at this point, so finish generating result */
		omfsCvtInt32toInt64((timecode.startFrame - startTC.startFrame) + segStart , &oldStart);
		 CHECK(omfiConvertEditRate(editRate, oldStart,
		  							sourceInfo.editrate , kRoundFloor, &newStart));
		CHECK(omfsTruncInt64toInt32(sourceInfo.position, &frameOffset));	/* OK FRAMEOFFSET */
		CHECK(omfsTruncInt64toInt32(newStart, &start32));		/* OK FRAMEOFFSET */
		*result = start32 - frameOffset;

	   /* check for out of bound timecode */
	   if (timecode.startFrame < startTC.startFrame) 
	   {
		  /* out of left bound */
	    RAISE(OM_ERR_BADSAMPLEOFFSET);
	   }
	   else
	   {
	    omfUInt32 len;
	    CHECK(omfsTruncInt64toUInt32(tcLen, &len));
	    if (timecode.startFrame > (startTC.startFrame + len))
	    {
			/* out of right bound */
	     RAISE(OM_ERR_BADSAMPLEOFFSET);
	    }
	   }

	  /* Release Bento reference, so the useCount is decremented */
	  if (sourceInfo.mob)
		 CMReleaseObject((CMObject)sourceInfo.mob);	
		
	  if (iterHdl)
		{
		  CHECK(omfiIteratorDispose(file, iterHdl));
		  iterHdl = NULL;
		}
	}

  XEXCEPT
	{
	  if(iterHdl)
	  	omfiIteratorDispose(file, iterHdl);
	  if (sequIter)
		omfiIteratorDispose(file, sequIter);
	}
  XEND;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: GetFrameInfo()
 *
 *      This function is used by omfsTimecodeToString().  It pulls
 *      apart a frame rate into elements in different units, and
 *      returns a structure containing these units.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static frameTbl_t GetFrameInfo(
							   omfInt32 fps) /* IN - Frame Rate */
{
	frameTbl_t	result;
	
	result.dropFpMin = (60 * fps) - 2;
	result.dropFpMin10 = (10*result.dropFpMin+2);
	result.dropFpHour = 6 * result.dropFpMin10;

	result.fpMinute = 60 * fps;
	result.fpHour = 60 * result.fpMinute;

	return(result);
}

/*************************************************************************
 * Function: omfmOffsetToTimecode()
 *
 *      Given a master mob or source mob, track ID of a track in a mob
 *		and an offset into the track, this function searches for the
 *		tape mob and returns the associated timecode.
 *
 *      This function should work for 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfmOffsetToTimecode(
    omfHdl_t file,         /* IN - File Handle */
	omfObject_t mob,       /* IN - Input Mob */
	omfTrackID_t trackID,  /* IN - Track ID of the track in the input mob */
	omfPosition_t offset,  /* IN - Offset into the given track */
	omfTimecode_t	*result)		/* OUT - The resulting timecode */
{
  omfObject_t		seg = NULL, slot = NULL, subSegment = NULL;
  omfIterHdl_t 		iterHdl = NULL;
  omfInt32 			numSlots, loop;
  omfTimecode_t 	timecode;
  omfMediaCriteria_t mediaCrit;
  omfEffectChoice_t effectChoice;
  omfDDefObj_t 		datakind = NULL;
  omfErr_t 			omfError = OM_ERR_NONE;
  omfFindSourceInfo_t	sourceInfo;
  omfRational_t		editRate;
  omfPosition_t		frameOffset64;
  omfIterHdl_t		sequIter = NULL;
  omfLength_t		zeroLen = omfsCvtInt32toLength(0, zeroLen);
  
  sourceInfo.mob = NULL;
  memset(result, 0, sizeof(omfTimecode_t));
  memset(&timecode, 0, sizeof(omfTimecode_t));
  result->startFrame = 0;
  omfAssertValidFHdl(file);

  XPROTECT(file)
	{
	  mediaCrit.type = kOmfAnyRepresentation;

     // MC 01-Dec-03 -- fix for crash
     effectChoice = kFindEffectSrc1;
	  CHECK(omfiMobSearchSource(file, mob, trackID, offset, kTapeMob,
							  &mediaCrit, &effectChoice, NULL, &sourceInfo));

	  CHECK(omfiIteratorAlloc(file, &iterHdl));
	  CHECK(omfiMobGetNumSlots(file, sourceInfo.mob, &numSlots));
	  for (loop = 1; loop <= numSlots; loop++)
		{
		  CHECK(omfiMobGetNextSlot(iterHdl, sourceInfo.mob, NULL, &slot));
		  CHECK(omfiMobSlotGetInfo(file, slot, &editRate, &seg));

		  /* Verify that it's a timecode track by looking at the
			* datakind of the track segment.
			*/
		  CHECK(omfiComponentGetInfo(file, seg, &datakind, NULL));
		  if (omfiIsTimecodeKind(file, datakind, kExactMatch, &omfError))
			 {
				/* Assume found at this point, so finish generating result */
		  		CHECK(omfiConvertEditRate(sourceInfo.editrate, sourceInfo.position,
		  									editRate, kRoundCeiling, &frameOffset64));

				CHECK(omfiOffsetToMobTimecode(file, sourceInfo.mob, seg, frameOffset64,
												&timecode));
				break;
			}
		} /* for */

	  /* Release Bento reference, so the useCount is decremented */
	  if (sourceInfo.mob)
		 CMReleaseObject((CMObject)sourceInfo.mob);

	  *result = timecode;
	  if (iterHdl)
		{
		  CHECK(omfiIteratorDispose(file, iterHdl));
		  iterHdl = NULL;
		}
	} /* XPROTECT */

  XEXCEPT
	{
	  if (iterHdl)
		omfiIteratorDispose(file, iterHdl);
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);

}

/*************************************************************************
 * Private Function: OffsetToTimecode()
 *
 *      Given an offset into a track and a frame rate, this function
 *      calculates a timecode value.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t OffsetToTimecode(
	omfFrameOffset_t offset, /* IN - Offset into a track */
	omfInt16 frameRate,      /* IN - Frame rate */
	omfDropType_t drop,     /* OUT - Drop or non-drop Timecode */
	omfInt16 *hours,         /* OUT - Hours value of timecode */
	omfInt16 *minutes,       /* OUT - Minutes value of timecode */
	omfInt16 *seconds,       /* OUT - Seconds value of timecode */
	omfInt16 *frames)        /* OUT - Frames value of timecode */
{
  frameTbl_t info;
  omfUInt32		frames_day;
  omfInt32 min10, min1;
  omfBool frame_dropped;

  info = GetFrameInfo(frameRate);
  frames_day = (drop ? info.dropFpHour: info.fpHour) *24;

  if (offset < 0L)
	 offset += frames_day;
  if (offset >= frames_day)
	 offset -= frames_day;
  if (drop)
	 {
		*hours = (omfInt16)(offset / info.dropFpHour);
		offset = offset % info.dropFpHour;
		min10 = offset / info.dropFpMin10;
		offset = offset % info.dropFpMin10;
		if (offset < info.fpMinute)
		  {
			 frame_dropped = FALSE;
			 min1 = 0;
		  }
		else
		  {
			 frame_dropped = TRUE;
			 offset -= info.fpMinute;
			 min1 = (offset / info.dropFpMin) + 1;
			 offset = offset % info.dropFpMin;
		  }
		
		*minutes = (omfInt16)((min10 * 10) + min1);
		*seconds = (omfInt16)(offset / frameRate);
		*frames = (omfInt16)(offset % frameRate);
		if (frame_dropped)
		  {
			 (*frames) +=2;
			 if (*frames >= frameRate)
				{
				  (*frames) -= frameRate;
				  (*seconds)++;
				  if (*seconds > 60)
					 {
						(*seconds) -= 60;
						(*minutes)++;
						if (*minutes > 60)
						  {
							 (*minutes) -= 60;
							 (*hours)++;
						  }
					 }
				}
		  }
	 }
  else
	 {
		*hours = (omfInt16)(offset / info.fpHour);
		offset = offset % info.fpHour;
		*minutes = (omfInt16)(offset / info.fpMinute);
		offset = offset % info.fpMinute;
		*seconds = (omfInt16)(offset / frameRate);
		*frames = (omfInt16)(offset % frameRate);
	 }

  return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: TimecodeToOffset()
 *
 *      Given a timecode and a frame rate, this function returns a
 *      position relative to the beginning of a track.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t TimecodeToOffset(
	omfInt16 frameRate,  /* IN - Frame Rate */
	omfInt16 hours,      /* IN - Hours value of Timecode */
	omfInt16 minutes,    /* IN - Minutes value of Timecode */
	omfInt16 seconds,    /* IN - Seconds value of Timecode */
	omfInt16 frames,     /* IN - Frames value of Timecode */
	omfDropType_t drop,  /* IN - Drop of non-drop Timecode */
	omfFrameOffset_t	*result) /* OUT - resulting position */

{
   omfUInt32		val;
	frameTbl_t	info;
	
	info = GetFrameInfo(frameRate);
	if(drop)
		{
		val = (hours * info.dropFpHour);
		val += ((minutes / 10) * info.dropFpMin10);
		val += (minutes % 10) * info.dropFpMin;
		}
	else
		{
		val = hours * info.fpHour;
		val += minutes * info.fpMinute;
		}

	val += seconds * frameRate;
	val += frames;
	
	*result = val;

	return(OM_ERR_NONE);
}

/************************
 * Function:  FindTimecodeContents	(INTERNAL)
 *
 *    Search a track group for the timecode track, and return a TCCP object.
 *    This routine will search for the first nonzero length subcomponent
 *    of a SEQU, because MC5.0 has zero length filler clips and
 *    transitions on the timecode tracks.  NULL is returned if there is
 *    not a valid timecode object present.
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.labelNumber
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 * Standard errors (see top of file).  */
static omfErr_t FindTimecodeTrack(
				omfHdl_t			file, 
				omfObject_t		tapeMob, 
				omfObject_t		*result)
{
	omfObject_t			seg = NULL;
	omfIterHdl_t		slotIter = NULL;
	omfNumSlots_t		numSlots;
	omfInt32				loop;
	omfMSlotObj_t		slot = NULL;
	omfBool				found = FALSE;
	omfDDefObj_t      datakind = NULL;
	omfErr_t          tmpError = OM_ERR_NONE;
	
	XPROTECT(file)
	{
		CHECK(omfiIteratorAlloc(file, &slotIter));
		CHECK(omfiMobGetNumSlots(file, tapeMob, &numSlots));
		for (loop = 1; loop <= numSlots; loop++)
		{
			CHECK(omfiMobGetNextSlot(slotIter, tapeMob, NULL, &slot));
			CHECK(omfiMobSlotGetInfo(file, slot, NULL, &seg));
			CHECK(omfiComponentGetInfo(file, seg, &datakind, NULL));
			if (omfiIsTimecodeKind(file, datakind, kExactMatch, &tmpError))
			  {
				 *result = seg;
				 found = TRUE;
				 break;
			  }
			/* Release Bento reference, so the useCount is decremented */
			if (slot)
			  {
				 CMReleaseObject((CMObject)slot);	
				 slot = NULL;
			  }
		 } /* for */
		omfiIteratorDispose(file, slotIter);
		slotIter = NULL;
		if(!found)
			RAISE(OM_ERR_NO_TIMECODE);
	} /* XPROTECT */
	XEXCEPT
	{
		if(XCODE() == OM_ERR_NO_MORE_OBJECTS)
			RERAISE(OM_ERR_NO_TIMECODE);
		if(slotIter != NULL)
			omfiIteratorDispose(file, slotIter);
		*result = NULL;
	}
	XEND;

	return(OM_ERR_NONE);
}

/************************
 * Function:  FindTimecodeClip	(INTERNAL)
 */
static omfErr_t FindTimecodeClip(
				omfHdl_t			file, 
				omfObject_t			tapeMob,
				omfFrameOffset_t	position,
				omfObject_t			*result,
				omfFrameOffset_t	*tcStartPos,
				omfLength_t			*tcTrackLen)
{
	omfObject_t		seg = NULL, subSegment;
	omfIterHdl_t	sequIter = NULL;
	omfPosition_t	offset;
	omfErr_t		status = OM_ERR_NONE;
	omfInt32		numSegs, sequLoop;
	omfLength_t		segLen, zeroLen;
	omfPosition_t	begPos, endPos, sequPos;
	
	XPROTECT(file)
	{
		omfsCvtInt32toInt64(position, &offset);
		omfsCvtInt32toInt64(0, &zeroLen);
		*tcStartPos = 0;
		*result = NULL;
		CHECK(FindTimecodeTrack(file, tapeMob, &seg));
		if(tcTrackLen != NULL)
		{
			CHECK(omfiTimecodeGetInfo(file, seg, NULL, tcTrackLen, NULL));
		}
		if (omfiIsATimecodeClip(file, seg, &status))
		{
			*result = seg;
			*tcStartPos = 0;
		}
		else if(omfiIsASequence(file, seg, &status))
		{
			if(tcTrackLen != NULL)
			{
				CHECK(omfiComponentGetLength(file, seg, tcTrackLen));
			}
  			CHECK(omfiIteratorAlloc(file, &sequIter));
  			CHECK(omfiSequenceGetNumCpnts(file, seg, &numSegs));
			for (sequLoop=0; sequLoop < numSegs; sequLoop++)
			{
				CHECK(omfiSequenceGetNextCpnt(sequIter, seg,
												NULL, &sequPos, &subSegment));
				CHECK(omfiComponentGetLength(file, subSegment, &segLen));
				/* Skip zero-length clips, sometimes found in MC files */
				if (omfsInt64Equal(segLen, zeroLen))
					continue;
				begPos = sequPos;
				endPos = sequPos;
				CHECK(omfsAddInt64toInt64(segLen, &endPos));
				if (omfsInt64Less(offset, endPos) &&
					omfsInt64LessEqual(begPos, offset))
				{
		 			*result = subSegment;
		 			CHECK(omfsTruncInt64toUInt32(sequPos, tcStartPos)); /* OK FRAMEOFFSET */
					break;
				}
			} /* for */
			CHECK(omfiIteratorDispose(file, sequIter));
			sequIter = NULL;
			
			/* Release Bento reference, so the useCount is decremented */
			if (subSegment)
			  {
				 CMReleaseObject((CMObject)subSegment);	
				 subSegment = NULL;
			  }
		}
	} /* XPROTECT */
	XEXCEPT
	{
		if(XCODE() == OM_ERR_NO_MORE_OBJECTS)
			RERAISE(OM_ERR_NO_TIMECODE);
		if(sequIter != NULL)
		  {
			 omfiIteratorDispose(file, sequIter);
			 sequIter = NULL;
		  }
		*result = NULL;
	}
	XEND;

	return(OM_ERR_NONE);
}
/************************
 * Function: omfmMobAddNilReference
 *
 * 	Adds a nil reference to a track.  This is needed, for example
 *		on the tracks of a tape mob which did not come from film.  Some
 *		indication is still needed of which tracks are valid.
 *
 *		A placeholder track is a track containing a source clip with a
 *		source ID of 0.0.0, and an offset of 0.
 *
 * Argument Notes:
 *		trackID - A 2.0-style track ID, unique within a mob.
 *		length - Should be equal to the length of the mob.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADRATE -- Illegal edit rate.
 */
omfErr_t omfmMobAddNilReference(
				omfHdl_t 		file,			/* IN -- */
				omfObject_t 	mob,			/* IN -- */
				omfTrackID_t	trackID,		/* IN -- */
				omfLength_t		length,		/* IN -- */
				omfDDefObj_t 	dataKind, 
				omfRational_t	editRate)	/* IN -- */
{
	omfObject_t		sub = NULL;
	omfPosition_t	zeroPos = omfsCvtInt32toPosition(0, zeroPos);
	omfLength_t		zeroLen = omfsCvtInt32toLength(0, zeroLen);
	omfSourceRef_t	sourceRef;
	omfMSlotObj_t	newTrack = NULL;
	
	omfAssert(editRate.denominator != 0, file, OM_ERR_BADRATE);
	
	XPROTECT(file)
	{
		sourceRef.sourceID.prefix = 0;
		sourceRef.sourceID.major = 0;
		sourceRef.sourceID.minor = 0;
		sourceRef.sourceTrackID = 0;
		omfsCvtInt32toPosition(0, sourceRef.startTime);
		CHECK(omfiSourceClipNew(file, dataKind, length, sourceRef, &sub));	
		CHECK(omfiMobAppendNewTrack(file,mob, editRate, sub, zeroPos, trackID,
												NULL, &newTrack));
		/* Release Bento reference, so the useCount is decremented */
		if (sub)
		  CMReleaseObject((CMObject)sub);	
		if (newTrack)
		  CMReleaseObject((CMObject)newTrack);	
	}
	XEXCEPT
	XEND;

	return(OM_ERR_NONE);
}

/************************
 * Function: omfmMobAddTimecodeClip
 *
 * 	Adds a timecode clip to the given timecode track of a source mob.
 *
 *		This function does not add the filler to any of the other tracks to
 *		indicate that the timecode is valid for that channel, use
 *		omfmMobValidateTimecodeRange for that purpose.
 *
 * Argument Notes:
 *		startTC - This is frames since midnight.  Use the function
 *			TimecodeToOffset() to compute this number.
 *		length32 - May be the value FULL_RANGE, in which case the length
 *			is 24 hours given the current parameters.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMobAddTimecodeClip(
				omfHdl_t				file,			/* IN -- */
				omfObject_t			aSourceMob,	/* IN -- */
				omfRational_t		editrate,	/* IN -- */
				omfInt32				trackID,		/* IN -- */
				omfTimecode_t			startTC,		/* IN -- */
				omfFrameLength_t	length32)	/* IN -- */
{
	omfObject_t     trackComponent= NULL, tccp = NULL, fill = NULL;
	omfObject_t     aSubComponent = NULL, aSequ = NULL;
	omfObject_t     filler1 = NULL, filler2 = NULL;
	omfInt32         zeroL = 0L, sub, numSub;
	omfFrameLength_t maxLength;
	omfClassID_t	tag;
	omfDDefObj_t	timecodeKind = NULL;
	omfErr_t			omfError = OM_ERR_NONE;
	omfPosition_t	startPos, zeroPos;
	omfLength_t		length, zeroLen, sequLen;
	omfMSlotObj_t	newTrack = NULL;
	omfBool			fullLength = FALSE;
	omfProperty_t	idProp, sequProp;
	
	if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
	{
		idProp = OMObjID;
		sequProp = OMSEQUSequence;
	}
	else
	{
		idProp = OMOOBJObjClass;
		sequProp = OMSEQUComponents;
	}

	if(length32 == FULL_LENGTH)
	  {
		 fullLength = TRUE;
		 length32 = 1;
	  }
	else
	  fullLength = FALSE;
	
	omfsCvtInt32toPosition(0, zeroPos);
	omfsCvtInt32toLength(0, zeroLen);

	XPROTECT(file)
	  {
		 CHECK(omfsObjectNew(file, "TCCP", &tccp));
		 omfiDatakindLookup(file, TIMECODEKIND, &timecodeKind, &omfError);
		 if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			{
			  CHECK(omfsWriteInt32(file, tccp, OMTCCPFlags, 
										 (startTC.drop == kTcDrop ? 1 : 0)));
			  CHECK(omfsWriteInt16(file, tccp, OMTCCPFPS, startTC.fps));
			  CHECK(omfsWriteInt32(file, tccp, OMTCCPStartTC, startTC.startFrame));
			  CHECK(omfsWriteInt32(file, tccp, OMCLIPLength, length32));
			  CHECK(omfsWriteTrackType(file, tccp, OMCPNTTrackKind, TT_TIMECODE));
			}
		 else
			{
			  CHECK(omfsWriteUInt16(file, tccp, OMTCCPFPS, startTC.fps));
			  CHECK(omfsWriteBoolean(file, tccp, OMTCCPDrop, (omfBool)(startTC.drop == kTcDrop)));
			  omfsCvtInt32toPosition(startTC.startFrame, startPos);
			  omfsCvtInt32toLength(length32, length);
			  CHECK(omfsWritePosition(file, tccp, OMTCCPStart, startPos));
			  if (omfError != OM_ERR_NONE)
				 {
					RAISE(omfError);
				 }
			  CHECK(omfsWriteObjRef(file, tccp, OMCPNTDatakind, timecodeKind));
			  CHECK(omfsWriteLength(file, tccp, OMCPNTLength, length));
			}
		 
		 if (FindTimecodeTrack(file, aSourceMob, &trackComponent) == OM_ERR_NONE)
			{
			  CHECK(omfsReadClassID(file, trackComponent, idProp, tag));
			  if (streq(tag, "SEQU"))
				 {
					numSub = omfsLengthObjRefArray(file, trackComponent, 
															 sequProp);
					for (sub = numSub; sub >= 1; sub--)
					  {
						 CHECK(omfsGetNthObjRefArray(file, trackComponent, 
												  sequProp, &aSubComponent, sub));
						 CHECK(omfsReadClassID(file, aSubComponent, idProp, tag));
						 if (!streq(tag, "FILL"))
							{
								if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
								{
									CHECK(omfsWriteExactEditRate(file, tccp, OMCPNTEditRate, editrate));
								}
							  CHECK(omfsPutNthObjRefArray(file, trackComponent,
															sequProp, tccp, sub + 1));
								/* 1.x does not have a Sequence Length property */
								if (file->setrev == kOmfRev2x)
								  {
									CHECK(omfiComponentGetLength(file, trackComponent, &sequLen));
									omfsAddInt64toInt64(length, &sequLen);
									CHECK(omfsWriteLength(file, trackComponent,OMCPNTLength, sequLen));
								  }
							  if (sub != numSub)	/* At least one FILL
														 * found at the end */
								 {
									CHECK(omfiFillerNew(file, timecodeKind, zeroLen, 
															  &fill));	
									if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
									{
										CHECK(omfsWriteExactEditRate(file, fill, OMCPNTEditRate, editrate));
									}
									CHECK(omfsPutNthObjRefArray(file, trackComponent,
															 sequProp, fill, sub + 2));
								 }
							  break;
							}
					  }
				 }
			  else
				 RAISE(OM_ERR_NOT_IMPLEMENTED);
			} /* FindTimecodeTrack */
		 else
			{
			  CHECK(omfiSequenceNew(file, timecodeKind, &aSequ));
			  CHECK(omfiFillerNew(file, timecodeKind, zeroLen, &filler1));	
			  CHECK(omfiSequenceAppendCpnt(file, aSequ, filler1));
			  CHECK(omfiSequenceAppendCpnt(file, aSequ, tccp));
			  CHECK(omfiFillerNew(file, timecodeKind, zeroLen, &filler2));	
			  CHECK(omfiSequenceAppendCpnt(file, aSequ, filler2));
			  CHECK(omfiMobAppendNewTrack(file, aSourceMob, editrate, 
													aSequ, zeroPos,
													trackID, NULL, &newTrack));
			}

		 /* Release Bento reference, so the useCount is decremented */
		 if (trackComponent)
			{
			  CMReleaseObject((CMObject)trackComponent);	
			}

		 if(fullLength)
			{
			  CHECK(TimecodeToOffset(startTC.fps, 24, 0, 0, 0, 
											 startTC.drop, &maxLength));
			  if(file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
				 {
					CHECK(omfsWriteInt32(file, tccp, OMCLIPLength, maxLength) );
					/* NOTE: What if the sequence already existed? */
				 }
			  else	/* Check at start of routine winnows out bad enum values */
				 {
					omfsCvtInt32toLength(maxLength, length);
					CHECK(omfsWriteLength(file, tccp, OMCPNTLength, length) );
					/* NOTE: What if the sequence already existed? */
					CHECK(omfsWriteLength(file, aSequ, OMCPNTLength, length) );
				 }
			}
	  } /* XPROTECT */
	XEXCEPT
	XEND;
									
	return (OM_ERR_NONE);
}

/************************
 * Function: omfmMobAddEdgecodeClip
 *
 * 	Adds a edegecode clip to the given edegecode track of a source mob.
 *
 *		This function does not add the filler to any of the other tracks to
 *		indicate that the edgecode is valid for that channel, use
 *		omfmMobValidateTimecodeRange for that purpose.
 *
 * Argument Notes:
 *		startEC - This is frames since midnight.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMobAddEdgecodeClip(
				omfHdl_t				file,			/* IN -- */
				omfObject_t			filmMob,		/* IN -- */
				omfRational_t		editrate,	/* IN -- */
				omfInt32				trackID,	/* IN -- */
				omfFrameOffset_t	startEC,		/* IN -- */
				omfFrameLength_t	length32,	/* IN -- */
				omfFilmType_t		filmKind,	/* IN -- */
				omfEdgeType_t		codeFormat,
				char 					*header)		/* IN -- */
{
	omfObject_t     filler1, filler2, ecSequence, edgecodeClip;
	char            edgecodeName[8];
	omfDDefObj_t	defObj;
	omfErr_t			omfError = OM_ERR_NONE;
	omfPosition_t	startPos, zeroPos;
	omfLength_t		length, zeroLen;
	omfMSlotObj_t	newTrack;
	omfDDefObj_t	edgecodeKind;
	omfInt16		n;
	
	omfsCvtInt32toPosition(0, zeroPos);
	omfsCvtInt32toLength(0, zeroLen);
	XPROTECT(file)
	{
		omfiDatakindLookup(file, EDGECODEKIND, &edgecodeKind, &omfError);
		if (omfError != OM_ERR_NONE)
		{
			RAISE(omfError);
		}

		CHECK(omfiFillerNew(file, edgecodeKind, zeroLen, &filler1));	
		CHECK(omfiFillerNew(file, edgecodeKind, zeroLen, &filler2));	

		CHECK(omfiSequenceNew(file, edgecodeKind, &ecSequence));

		CHECK(omfsObjectNew(file, "ECCP", &edgecodeClip));
		CHECK(omfsWriteFilmType(file, edgecodeClip, OMECCPFilmKind, filmKind));
		CHECK(omfsWriteEdgeType(file, edgecodeClip, OMECCPCodeFormat, 
								codeFormat));
		for(n = 0; n < sizeof(edgecodeName); n++)
			edgecodeName[n] = '\0';
		strncpy((char *) edgecodeName, header, 8);
		if(file->setrev == kOmfRev2x)
		{
			omfsCvtInt32toLength(length32, length);
			omfsCvtInt32toPosition(startEC, startPos);
			CHECK(omfsWriteLength(file, edgecodeClip, OMCPNTLength, length));
			CHECK(omfsWritePosition(file, edgecodeClip, OMECCPStart, startPos));
			omfiDatakindLookup(file, EDGECODEKIND, &defObj, &omfError);
			if (omfError != OM_ERR_NONE)
			{
				RAISE(omfError);
			}
			
			CHECK(omfsWriteObjRef(file, edgecodeClip, OMCPNTDatakind, defObj));
			CHECK(omfsWriteDataValue(file, edgecodeClip, OMECCPHeader,
									   edgecodeKind, edgecodeName, zeroPos, 8));
		}
		else
		{
			CHECK(omfsWriteInt32(file, edgecodeClip, OMCLIPLength, length32));
			CHECK(omfsWriteExactEditRate(file, edgecodeClip, OMCPNTEditRate, 
										 editrate));
			CHECK(omfsWriteInt32(file, edgecodeClip, OMECCPStartEC, startEC));
			CHECK(omfsWriteTrackType(file, edgecodeClip, OMCPNTTrackKind, 
									 TT_EDGECODE));
			CHECK(omfsWriteVarLenBytes(file, edgecodeClip, OMECCPHeader,
									   zeroPos, 8, edgecodeName));
		}

		CHECK(omfiSequenceAppendCpnt(file, ecSequence, filler1));
		CHECK(omfiSequenceAppendCpnt(file, ecSequence, edgecodeClip));
		CHECK(omfiSequenceAppendCpnt(file, ecSequence, filler2));
		CHECK(omfiMobAppendNewTrack(file, filmMob, editrate, ecSequence, zeroPos,
											trackID, NULL, &newTrack));
	} /* XPROTECT */
	XEXCEPT
	XEND;

	return (OM_ERR_NONE);
}


/************************
 * Function: omfmMobValidateTimecodeRange
 *
 * 	Adds the filler to a track in a tape or film mob to
 *		indicate that the timecode is valid for that channel.
 *
 *		This function does not create the timecode track, but assumes
 *		that one is present.
 *
 * Argument Notes:
 *		startOffset - A reference relative to the start of the timecode
 *			track, indicating the start of the valid TC or EC range.
 *		length32 - May be the value FULL_RANGE, in which case the length
 *			is 24 hours given the current parameters.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfmMobValidateTimecodeRange(
				omfHdl_t				file,			/* IN -- */
				omfObject_t			sourceMob,	/* IN -- */
				omfDDefObj_t  		mediaKind,	/* IN -- */
				omfTrackID_t		trackID,		/* IN -- */
				omfRational_t 		editrate,	/* IN -- */
				omfFrameOffset_t	startOffset,/* IN -- */
				omfFrameLength_t	length32)	/* IN -- */
{
	omfObject_t   	sclp, timecodeClip, aSequ, filler1, filler2, seg, subSegment;
	omfPosition_t	pos, zeroPos, sequPos, begPos, endPos;
	omfLength_t		length, zeroLen, tcLen, endFillLen, firstFillLen, oldFillLen, segLen;
	omfLength_t		tcTrackLen, sequLen;
  	omfSourceRef_t sourceRef;
	omfMSlotObj_t	newTrack, track;
  	omfFrameOffset_t	tcStartPos;
  	omfTimecode_t	timecode;
  	omfProperty_t	prop;
  	omfInt32		sequLoop, numSegs;
  omfIterHdl_t		sequIter = NULL;
  	omfErr_t		omfError;
  
	XPROTECT(file)
	{
		CHECK(FindTimecodeClip(file, sourceMob, startOffset, &timecodeClip, &tcStartPos,
								&tcTrackLen));
		CHECK(omfiTimecodeGetInfo(file, timecodeClip, NULL, &tcLen, &timecode));
		if(length32 == FULL_LENGTH)
		{
			CHECK(TimecodeToOffset(timecode.fps, 24, 0, 0, 0,
			  						timecode.drop, &length32));
		}
		omfsCvtInt32toLength(length32, length);
		omfsCvtInt32toLength(0, zeroLen);
		
		omfsCvtInt32toPosition(startOffset, pos);
		omfsCvtInt32toPosition(0, zeroPos);
		endFillLen = tcTrackLen;
		CHECK(omfsSubInt64fromInt64(pos, &endFillLen));
		CHECK(omfsSubInt64fromInt64(length, &endFillLen));
		sourceRef.sourceID.prefix = 0;
		sourceRef.sourceID.major = 0;
		sourceRef.sourceID.minor = 0;
		sourceRef.sourceTrackID = 0;
		omfsCvtInt32toPosition(0, sourceRef.startTime);
		CHECK(omfiSourceClipNew(file, mediaKind, length, sourceRef, &sclp));

		if(FindTrackByTrackID(file,sourceMob, trackID, &track) != OM_ERR_NONE)
		{
			CHECK(omfiSequenceNew(file, mediaKind, &aSequ));
			CHECK(omfiFillerNew(file, mediaKind, pos, &filler1));	
			CHECK(omfiSequenceAppendCpnt(file, aSequ, filler1));
			CHECK(omfiSequenceAppendCpnt(file, aSequ, sclp));
			CHECK(omfiFillerNew(file, mediaKind, endFillLen, &filler2));	
			CHECK(omfiSequenceAppendCpnt(file, aSequ, filler2));

			/* (SPR#343) Change to validate multiple ranges */
			CHECK(omfiMobAppendNewTrack(file,sourceMob, editrate, aSequ, zeroPos, trackID,
												NULL, &newTrack));
		}
	  else
	  	{
		  CHECK(omfiMobSlotGetInfo(file, track, NULL, &seg));
		  	if (omfiIsASequence(file, seg, &omfError))
			{
	  			CHECK(omfiIteratorAlloc(file, &sequIter));
	  			CHECK(omfiSequenceGetNumCpnts(file, seg, &numSegs));
				for (sequLoop=0; sequLoop < numSegs; sequLoop++)
				{
					CHECK(omfiSequenceGetNextCpnt(sequIter, seg,
													NULL, &sequPos, &subSegment));
					CHECK(omfiComponentGetLength(file, subSegment, &segLen));
					/* Skip zero-length clips, sometimes found in MC files */
					if (omfsInt64Equal(segLen, zeroLen))
						continue;
					begPos = sequPos;
					endPos = sequPos;
					CHECK(omfsAddInt64toInt64(segLen, &endPos));
					if (omfsInt64Less(pos, endPos) &&
						omfsInt64LessEqual(begPos, pos))
					{
			 			if(omfiIsAFiller(file, subSegment, &omfError) &&
			 				(sequLoop == (numSegs-1)))
			 			{
							if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
								prop = OMCLIPLength;
							else
								prop = OMCPNTLength;
							
							firstFillLen = pos;
							CHECK(omfsSubInt64fromInt64(sequPos, &firstFillLen));
							CHECK(omfsReadLength(file, subSegment, prop, &oldFillLen));
							endFillLen = oldFillLen;
							CHECK(omfsSubInt64fromInt64(length, &endFillLen));
							CHECK(omfsSubInt64fromInt64(firstFillLen, &endFillLen));
							/****/
							CHECK(omfsWriteLength(file, subSegment, prop, firstFillLen));
							/* 1.x does not have a Sequence Length property */
							if (file->setrev == kOmfRev2x)
							  {
								CHECK(omfsReadLength(file, seg,prop, &sequLen));
								omfsSubInt64fromInt64(oldFillLen, &sequLen);
								omfsAddInt64toInt64(firstFillLen, &sequLen);
								CHECK(omfsWriteLength(file, seg,prop, sequLen));
							  }

							CHECK(omfiFillerNew(file, mediaKind, endFillLen, &filler2));	
							if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
							{
								CHECK(omfsWriteExactEditRate(file, sclp, OMCPNTEditRate, editrate));
								CHECK(omfsWriteExactEditRate(file, filler2, OMCPNTEditRate, editrate));
							}
							CHECK(omfiSequenceAppendCpnt(file, seg, sclp));
							CHECK(omfiSequenceAppendCpnt(file, seg, filler2));
							break;
						}
						else
							RAISE(OM_ERR_NOT_IMPLEMENTED);
					}
				} /* for */
				CHECK(omfiIteratorDispose(file, sequIter));
				sequIter = NULL;
				
				/* Release Bento reference, so the useCount is decremented */
				if (subSegment)
				  {
					 CMReleaseObject((CMObject)subSegment);	
					 subSegment = NULL;
				  }
			  } /* If Sequence */
			  else
				RAISE(OM_ERR_NOT_IMPLEMENTED);
		}
	}
	XEXCEPT
	XEND;

	return (OM_ERR_NONE);
}

/****************************************/
/******	High level Export wrapper	*****/
/****************************************/

/************************
 * Function: omfmStandardTapeMobNew
 *
 * 	Function to create a simple, standard tape mob.  This is a very
 *		restricted interface, useful mostly as an example.
 *
 *		This tape mob has a single timecode track, running from 0:0:0:0
 *		to 23:59:59:29 (non-drop only) with trackID 1.  The video tracks
 *		immediately follow the timecode track in ID numbering, and are
 *		marked valid for their entire length.  The audio tracks follow the
 *		video tracks, and are also marked as valid for their entire length.
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
omfErr_t omfmStandardTapeMobNew(
				omfHdl_t			file,				/* IN -- */
				omfRational_t	vEditRate,		/* IN -- */
				omfRational_t	aEditRate,		/* IN -- */
				char				*name,			/* IN -- */
				omfInt16			numVideoTracks,/* IN -- */
				omfInt16			numAudioTracks,/* IN -- */
				omfObject_t		*result)			/* OUT -- Returned object */
{
	omfObject_t		tmpResult;
	omfRational_t	primaryEditRate;
	omfInt16			n, trackID;
	omfTimecode_t	tc;
	

	XPROTECT(file)
	  {
		trackID = 1;
		primaryEditRate = (numVideoTracks != 0 ? vEditRate : aEditRate);
		tc.startFrame = 0;
		tc.drop = kTcNonDrop;
		tc.fps = 30;
		CHECK(omfmTapeMobNew(file, name, &tmpResult) );
		CHECK(omfmMobAddTimecodeClip(file, tmpResult, primaryEditRate,
									 trackID++, tc, FULL_LENGTH) );
				                    
		for(n = 0; n < numVideoTracks; n++)
		  CHECK(omfmMobValidateTimecodeRange(file, tmpResult, file->pictureKind,
									   trackID++, vEditRate, 0, FULL_LENGTH) );
	
		for(n = 0; n < numAudioTracks; n++)
		  CHECK(omfmMobValidateTimecodeRange(file, tmpResult, file->soundKind,
									   trackID++, aEditRate, 0, FULL_LENGTH) );
	  }

	XEXCEPT
	  {
	  }
	XEND;
				               			
	*result = tmpResult;
	return(OM_ERR_NONE);
}

/****************************************/
/******	 Locate next master mob		*****/
/****************************************/

/************************
 *  Function: omfmMobGetNextRefFileMob
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
static omfErr_t omfmMobGetNextRefFileMob(
				omfHdl_t file,						/* IN -- */
				omfObject_t masterMob,			/* IN -- */
				omfDDefObj_t  mediaKind,		/* IN -- */
				omfIterHdl_t iter,				/* IN/OUT -- */
				omfMediaCriteria_t *mediaCrit, /* IN - */
				omfObject_t *fileMob,			/* OUT -- */
				omfInt32 *masterTrackID,	/* OUT -- */
				omfPosition_t *masterOffset)	/* OUT -- */
{
	omfSearchCrit_t			criteria;
	omfMSlotObj_t		track;
	omfTrackID_t		trackID;
	omfSegObj_t			seg;
	omfObject_t			sourceClip;
	
	XPROTECT(file)
	{
		*fileMob = NULL;
		criteria.searchTag = kNoSearch;

		CHECK(omfiMobGetNextTrack(iter, masterMob, &criteria, &track));
		CHECK(omfiTrackGetInfo(file, masterMob, track, NULL, 0, NULL, masterOffset, &trackID, &seg));
		CHECK(omfmGetCriteriaSourceClip(file, masterMob, trackID, mediaCrit, &sourceClip));
		CHECK(omfiSourceClipResolveRef(file, sourceClip, fileMob));
	}
	XEXCEPT
	XEND;
	
	return(OM_ERR_NONE);	
}

/****************************************/
/******		High-Level Import		*****/
/****************************************/


/************************
 *  Function: omfmMobGetNumLocators
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
omfErr_t omfmMobGetNumLocators(
				omfHdl_t       file,          /* IN -- */
				omfMobObj_t		fileMob,	      /* IN -- */
				omfInt32		   *numLocators)	/* OUT -- */
{
   omfObject_t mdes = NULL;
	omfProperty_t prop;
	omfErr_t	status = OM_ERR_NONE;
	
	omfAssert(omfiIsAFileMob(file, fileMob, &status), file, OM_ERR_NOT_FILEMOB);

	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  prop = OMMOBJPhysicalMedia;
		else /* kOmfRev2x */
		  prop = OMSMOBMediaDescription;

		CHECK(omfsReadObjRef(file, fileMob, prop, &mdes));

		if (mdes && numLocators)
		  *numLocators = omfsLengthObjRefArray(file, mdes, OMMDESLocator);
	}

	XEXCEPT
	XEND;

	return(OM_ERR_NONE);
}


/************************
 *  Function: omfmMobGetNextLocator
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
omfErr_t omfmMobGetNextLocator(
				omfIterHdl_t	iterHdl,	/* IN/OUT -- */
				omfMobObj_t		fileMob,	/* IN -- */
				omfObject_t		*locator)	/* OUT -- */
{
	omfErr_t	status = OM_ERR_NONE;
	omfObject_t	tmpLocator;
	omfHdl_t	file;
	
	file = iterHdl->file;
	*locator = NULL;
	omfAssertIterHdl(iterHdl);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterLocator) ||
					(iterHdl->iterType == kIterNull), 
					file, OM_ERR_ITER_WRONG_TYPE);
	omfAssert(omfiIsAFileMob(iterHdl->file, fileMob, &status),
					file, OM_ERR_NOT_FILEMOB);

	XPROTECT(file)
	{
		/* Initialize iterator if first time through */
		if (iterHdl->currentIndex == 0)
		{
			CHECK(omfsReadObjRef(file, fileMob,
				((file->setrev == kOmfTstRev1x || file->setrev == kOmfRevIMA) 
					? OMMOBJPhysicalMedia
				   : OMSMOBMediaDescription),
				&iterHdl->mdes));
			iterHdl->maxIter = omfsLengthObjRefArray(file, iterHdl->mdes, 
													 OMMDESLocator);
			iterHdl->currentIndex = 1;
			iterHdl->iterType = kIterLocator;
		}

		if (iterHdl->currentIndex <= iterHdl->maxIter)
		{
			CHECK(omfsGetNthObjRefArray(file, iterHdl->mdes, OMMDESLocator,
										&tmpLocator, iterHdl->currentIndex));

			iterHdl->currentIndex++;
			*locator = tmpLocator;
		}
		else
		{
			RAISE(OM_ERR_NO_MORE_OBJECTS);
		}
	}
	XEXCEPT
	XEND;

	return(OM_ERR_NONE);
}

/************************
 *  Function: omfmMobGetTapeName
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
omfErr_t omfmMobGetTapeName(
				omfHdl_t 	file,					/* IN -- */
				omfObject_t	masterMob,			/* IN -- */
				omfInt32 	masterTrackID,	/* IN -- */
				omfInt32		buflen,				/* IN -- */
				char			*buffer)				/* OUT -- */
{
	omfPosition_t	zeroPos;
	omfFindSourceInfo_t	sourceInfo;
   omfEffectChoice_t effectChoice;
	
	XPROTECT(file)
	{
		omfsCvtInt32toPosition(0, zeroPos);

		CHECK(omfiMobGetInfo(file, masterMob, NULL, buflen, buffer, NULL, NULL));

      // MC 01-Dec-03 -- fix for crash
      effectChoice = kFindEffectSrc1;
		CHECK(omfiMobSearchSource(file, masterMob, masterTrackID,  zeroPos,
									kTapeMob, NULL, &effectChoice, NULL, &sourceInfo));

		CHECK(omfiMobGetInfo(file, sourceInfo.mob, NULL, buflen, buffer, NULL, NULL));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}


/************************
 * Function: omfmLocatorGetInfo
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
omfErr_t omfmLocatorGetInfo(
				omfHdl_t				file,			/* IN -- */
				omfObject_t			locator,		/* IN -- */
				omfClassIDPtr_t 	type,			/* OUT -- */
				omfInt32				pathBufSize,/* IN -- */
				char					*pathBuf)	/* OUT -- */
{
	omfClassID_t	locTag;
	char				filename[256];
	omfErr_t			omfError = OM_ERR_NONE;
	omfDDefObj_t	stringKind;
	omfUInt32		bytesRead;
	omfPosition_t	zeroPos;
	omfProperty_t	idProp;
	
	omfAssertValidFHdl(file);

	omfsCvtInt32toPosition(0, zeroPos);

	XPROTECT(file)
	{
		type[0] = 0;
		type[1] = 0;
		type[2] = 0;
		type[3] = 0;
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		pathBuf[0] = '\0';
		CHECK(omfsReadClassID(file, locator, idProp, locTag));

		strncpy(type, locTag, 4);
		if (streq(locTag, "MACL"))
		{
			CHECK(omfsReadString(file, locator, OMMACLFileName, filename, 
								 sizeof(filename)));
		}
		else if (streq(locTag, "DOSL"))
		{
			CHECK(omfsReadString(file, locator, OMDOSLPathName, filename, 
								 sizeof(filename)));
		} 
		else if (streq(locTag, "TXTL"))
		{
			CHECK(omfsReadString(file, locator, OMTXTLName, filename, 
								 sizeof(filename)));
		}
		else if (streq(locTag, "WINL"))
		{
			omfiDatakindLookup(file, STRINGKIND, &stringKind, &omfError);
			CHECK(omfError);
			omfError = omfsReadDataValue(file, locator, OMWINLShortcut, 
												  stringKind, filename, zeroPos, 
												  pathBufSize, &bytesRead);
			if ((omfError != OM_ERR_NONE) && (omfError != OM_ERR_END_OF_DATA))
			  RAISE(omfError);
		}
		else if (streq(locTag, "UNXL"))
		{
			CHECK(omfsReadString(file, locator, OMUNXLPathName, filename, 
								 sizeof(filename)));
		}
		else if (streq(locTag, "NETL"))
		{
			CHECK(omfsReadString(file, locator, OMNETLURLString, filename, 
								 sizeof(filename)));
		}
		else
		{
			RAISE(OM_ERR_INTERNAL_UNKNOWN_LOC);
		}

		strncpy(pathBuf, filename, pathBufSize);
	}
	XEXCEPT
	XEND;

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfmMacLocatorGetInfo
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
omfErr_t omfmMacLocatorGetInfo(
				omfHdl_t		file,		/* IN -- */
				omfObject_t locator,	/* IN -- */
				omfInt16		*vRef,	/* OUT -- */
				omfInt32		*DirID)	/* OUT -- */
{
	omfClassID_t  locTag;
	char			volname[256];
	omfProperty_t	idProp;

	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		*vRef = 0;
		*DirID = 0;
		CHECK(omfsReadClassID(file, locator, idProp, locTag));

		if (streq(locTag, "MACL"))
		{
			if(file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
			{
				CHECK(omfsReadInt16(file, locator, OMMACLVRef, vRef));
			}
			else
			{
				CHECK(omfsReadString(file, locator, OMMACLVName, volname, 
									 sizeof(volname)));
#if PORT_SYS_MAC
/*            HParamBlockRec	volParms;
            omfInt16			volNameLen, n;
				for(n = volNameLen; n >= 0; n--)
				  volname[n+1] = volname[n];
				volname[0] = volNameLen;
				volParms.volumeParam.ioCompletion = 0;
				volParms.volumeParam.ioNamePtr = (StringPtr)&volname;
				volParms.volumeParam.ioVRefNum = 0;
				volParms.volumeParam.ioVolIndex = -1;
				PBHGetVInfo(&volParms, FALSE);
				*vRef = volParms.volumeParam.ioVRefNum;*/
            struct stat st;
            int err = stat(volname, &st);
            *vRef = st.st_dev;
#endif
			}
			CHECK(omfsReadInt32(file, locator, OMMACLDirID, DirID));
		}
		else
		{
			RAISE(OM_ERR_INTERNAL_UNKNOWN_LOC);
		}

	}
	XEXCEPT
	XEND;

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfmWindowsLocatorGetInfo
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
omfErr_t omfmWindowsLocatorGetInfo(
				omfHdl_t		file,			/* IN -- */
				omfObject_t 	locator,		/* IN -- */
				omfInt32		shortCutLen,	/* IN -- */
				char			*shortCutBuf)			/* OUT -- */
{
	omfClassID_t	locTag;
	omfObject_t		datakind;
	omfInt64		zeroPos;
	omfUInt32		bytesRead;
	omfErr_t		omfError;
	
	XPROTECT(file)
	{
		omfsCvtInt32toInt64(0, &zeroPos);
		XASSERT((file->setrev == kOmfRev2x), OM_ERR_NOT_IN_15);

		CHECK(omfsReadClassID(file, locator, OMOOBJObjClass, locTag));
		if (streq(locTag, "WINL"))
		{
			omfiDatakindLookup(file, STRINGKIND, &datakind, &omfError);
			CHECK(omfError);
			
			CHECK(omfsReadDataValue(file, locator, OMWINLShortcut, datakind,
				shortCutBuf, zeroPos, shortCutLen, &bytesRead));					
		}
		else
		{
			RAISE(OM_ERR_INTERNAL_UNKNOWN_LOC);
		}

	}
	XEXCEPT
	XEND;

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfmSourceGetVideoSignalType
 *
 * 		If media has video, what is it's signal type
 *
 * Argument Notes:
 *		Get from tape if possible, if not, best guess.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */

omfErr_t   omfmSourceGetVideoSignalType(omfMediaHdl_t media,
										omfVideoSignalType_t *signalType)
{
	omfPosition_t	zeroPos;
	omfVideoSignalType_t tmpSignalType = kVideoSignalNull;
	omfMSlotObj_t mslot = NULL;
	omfIterHdl_t iterHdl;
	omfSearchCrit_t searchCrit;
	omfRational_t editRate;
	double editRateFloat;
	omfErr_t omfError = OM_ERR_NONE;
	omfInt32 trackID = -1;
	omfInt16 i = 0;
	omfDDefObj_t pictureKind = NULL;
	omfFindSourceInfo_t	sourceInfo;

	XPROTECT(media->mainFile)
	{
		omfsCvtInt32toPosition(0, zeroPos);

		/* find video channel in file mob */
		omfiDatakindLookup(media->mainFile, PICTUREKIND, &pictureKind, &omfError);
		CHECK(omfError);				
		
		i = media->numChannels;
		while (i > 0 && trackID < 0)
		{
			if (media->channels[i-1].mediaKind == pictureKind)
				trackID = media->channels[i-1].trackID;
			i--;
		}
		
		if (trackID >= 0)
		{
         // MC 01-Dec-03 -- fix for crash
         omfEffectChoice_t effectChoice = kFindEffectSrc1;
			omfError = omfiMobSearchSource(media->mainFile, media->fileMob, trackID,
							zeroPos, kTapeMob, NULL, &effectChoice, NULL, &sourceInfo);
		}
		

		if (omfError == OM_ERR_NONE && sourceInfo.mob != NULL)
		{
			omfError = omfmTapeMobGetDescriptor(media->mainFile, sourceInfo.mob, 
															NULL, &tmpSignalType,
															NULL,NULL,0,NULL,0,NULL);
			if (omfError == OM_ERR_NONE && tmpSignalType != kVideoSignalNull)
			{
				*signalType = tmpSignalType;
				return(OM_ERR_NONE);
			}
			else /* no tape descriptor with signal tape, check editRate */
			{
				CHECK(omfiIteratorAlloc(media->mainFile, &iterHdl));
				searchCrit.searchTag = kByDatakind;
				strcpy( (char*) searchCrit.tags.datakind, PICTUREKIND);
				CHECK(omfiMobGetNextSlot(iterHdl, sourceInfo.mob, &searchCrit, &mslot));
				
				CHECK(omfiIteratorDispose(media->mainFile, iterHdl));
				iterHdl = NULL;
				
				if (mslot != NULL)
				{
					CHECK(omfiMobSlotGetInfo(media->mainFile, mslot, &editRate, NULL));
					editRateFloat = (double)editRate.numerator / (double)editRate.denominator;
					if (fabs(editRateFloat - 29.97) < .1)
						tmpSignalType = kNTSCSignal;
					else if (fabs(editRateFloat - 25.0) < .01)
						tmpSignalType = kPALSignal;
					else
						tmpSignalType = kVideoSignalNull;

					if (tmpSignalType != kVideoSignalNull)
					{
						*signalType = tmpSignalType;
						return(OM_ERR_NONE);
					}
				}
			}
		}
		/* if this code is being executed, then the signal format wasn't 
		   found or deduced by the tape mob.  At this point, try guessing
		   from the sample rate? */
	    if(media->mainFile->setrev == kOmfRev2x)
		{
			CHECK(omfsReadRational(media->mainFile, media->mdes,
									OMMDFLSampleRate, &editRate));
		}
		else
		{
			CHECK(omfsReadExactEditRate(media->mainFile, media->mdes,
										OMMDFLSampleRate, &editRate));
		}
		editRateFloat = (double)editRate.numerator / (double)editRate.denominator;
		if (fabs(editRateFloat - 29.97) < .1)
			*signalType = kNTSCSignal;
		else if (fabs(editRateFloat - 25.0) < .01)
			*signalType = kPALSignal;
		else
			*signalType = kVideoSignalNull;

		return(OM_ERR_NONE);
 
	}
	XEXCEPT
		if (iterHdl)
		{
			iterHdl = NULL; /* do this first to prevent infinite loops */
			CHECK(omfiIteratorDispose(media->mainFile, iterHdl));
		}
	XEND
	
	*signalType = kVideoSignalNull;
	return(OM_ERR_NONE);
}


omfErr_t omfmFindCodec(
			omfHdl_t				file,
			omfObject_t			fileMob,
			omfCodecID_t		*result)
{
	omfObject_t				mdes;
	codecTable_t			tbl;
	omfCodecMetaInfo_t	meta;
	char						fullName[16];
	
	XPROTECT(file)
	{
		CHECK(omfmMobGetMediaDescription(file, fileMob, &mdes));
		CHECK(omfmFindCodecForMedia(file, mdes, &tbl));
		CHECK(codecGetMetaInfo(file->session, &tbl, NULL,
									fullName, sizeof(fullName), &meta));
		*result = meta.codecID;
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
