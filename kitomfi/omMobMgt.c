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

/* Name: omMobMgt.c
 * 
 * Overall Function: An API of management functions for Mobs
 *
 * Audience: Client applications that manipulate mobs
 *
 * Public Functions:
 *    omfiMobCopy()
 *    omfiMobCopyExternal()
 *    omfiMobCloneExternal()
 *    omfiMobDelete()
 *    omfiObjectDeleteTree()
 *    omfiObjectCopyTree()
 *    omfiObjectCopyTreeExternal()
 *    omfiIsMobInFile()
 *    omfiFileGetNextDupMobs()
 *    omfiMobChangeRef()
 *
 * General error codes returned:
 *    OM_ERR_NULLOBJECT
 *       A null object is not allowed (probably passed in as an argument)
 *    OM_ERR_WRONG_OPENMODE
 *       File opened in wrong mode (readonly vs. readwrite)
 *    OM_ERR_INVALID_OBJ
 *       Invalid object for this operation
 *    OM_ERR_BAD_FHDL
 *       Bad File Handle
 *    OM_ERR_MOB_NOT_FOUND
 *       Mob does not exist in this file
 *    OM_ERR_BENTO_PROBLEM
 *       Unspecified Bento container error
 */

#include "masterhd.h"
#include "omPublic.h"
#include "omPvt.h"
#include "omMobMgt.h"

#define NO_COPY_PRIVATE FALSE
#define COPY_PRIVATE TRUE
#define BUFSIZE 1024

/* Static function prototypes */
static omfErr_t MakeNewDatakind(
						 omfHdl_t srcFile, 
						 omfObject_t objRef, 
						 omfHdl_t destFile,
						 omfObject_t *datakind);

static omfErr_t ProcessDatakindExternal(
	omfHdl_t srcFile,
    omfObject_t objRef,
    CMGlobalName propName,
    omfHdl_t destFile,
    omfProperty_t *destProp,
    omfDDefObj_t *datakind);

static omfErr_t ProcessEffectExternal(
    omfHdl_t srcFile,
    omfObject_t objRef,
    CMGlobalName propName,
    omfHdl_t destFile,
    omfProperty_t *destProp,
    omfDDefObj_t *effectDef);

static omfErr_t DeleteFromObjectSpine(
    omfHdl_t file,
    omfObject_t head,
    omfMobObj_t mob);

/* Functions to support omfiMobChangeRef() */
static omfBool IsThisSCLP(omfHdl_t file,
		  omfObject_t obj,
		  void *data);

static omfErr_t ChangeRef(omfHdl_t file,
		  omfObject_t obj,
		  omfInt32 level,
          void *data);

static omfErr_t Write1xMediaDataMobID(omfHdl_t file,
							   omfObject_t mediaObj,
							   omfUID_t mobID);

OMF_EXPORT  omfErr_t omfiCopyPropertyExternal(
					  omfHdl_t srcFile,       /* IN - Src File Handle */
					  omfHdl_t destFile,      /* IN - Dest File Handle */
					  omfObject_t srcObj,     /* IN - Object to copy from */	
					  omfObject_t destObj,    /* IN - Object to copy to */
					  omfProperty_t srcProp,  /* IN - Src Property Name */
					  omfType_t srcType,      /* IN - Src Property Type Name */
					  omfProperty_t destProp, /* IN - Dest Property Name */
					  omfType_t destType);    /* IN - Src Property Type Name */

/*************************************************************************
 * Function: omfiMobCopy()
 *
 *      This function makes a copy of the given source mob into a destination
 *      mob in the same file that is returned through the destMob parameter.
 *      The destination mob is given a newly created mob ID, and the name
 *      specified in the destMobName parameter.  All private data is also
 *      copied.
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *      The destMobName argument is optional.  A NULL should be specified
 *      if no destination mob name is desired.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobCopy(
	omfHdl_t file,          /* IN - File Handle */
	omfMobObj_t srcMob,     /* IN - Source Mob */
	omfString destMobName,  /* IN - Destination Mob Name (optional) */
	omfMobObj_t *destMob)   /* OUT - Destination Mob */
{
	omfMobObj_t newMob;
	omfBool isPrimary, isMaster;
	omfObject_t head;
	omfErr_t omfError = OM_ERR_NONE;
	
	*destMob = NULL;
	omfAssertValidFHdl(file);
	omfAssert((srcMob != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((file->openType != kOmOpenRead), file, 
					OM_ERR_WRONG_OPENMODE);

	XPROTECT(file)
	  {
		if (!omfiIsAMob(file, srcMob, &omfError))
		  {
			RAISE(OM_ERR_INVALID_OBJ);
		  }

		CHECK(omfiObjectCopyTree(file, srcMob, &newMob));

		/* MobSetNewProps will create a new mob ID, set name, and add
		 * to the Primary mobs index if needed
		 */

		isPrimary = omfiIsAPrimaryMob(file, srcMob, &omfError);
		isMaster = omfiIsAMasterMob(file, srcMob, &omfError);
		if (omfError == OM_ERR_NONE)
		  {
			CHECK(MobSetNewProps(file, newMob, isMaster, destMobName, 
									 isPrimary));
		  }
		else
		  {
			RAISE(omfError);
		  }

		/* Store the new mob in the HEAD Mobs */
		CHECK(omfsGetHeadObject(file, &head));
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
		  	/* SetObjectIdentity already added this to the compmob oo source mob list */
			CHECK(omfsAppendObjRefArray(file, head, OMObjectSpine, newMob));
		  }
		else
		  {
			CHECK(omfsAppendObjRefArray(file, head, OMHEADMobs, newMob));
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		/* NOTE: This function needs more cleanup (delete mob, out of index) */
		return(XCODE());
	  }
	XEND;

	*destMob = newMob;
	return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: Write1xMediaDataMobID (INTERNAL)
 *
 *      This function determines the type of 1.x media, and writes the 
 *      MobID with the correct property name (e.g., OMTIFFMobID)
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t Write1xMediaDataMobID(omfHdl_t file,
									  omfObject_t mediaObj,
									  omfUID_t mobID)
{
  omfClassID_t classID;
  omfProperty_t prop;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(file)
	{
	  CHECK(omfsReadClassID(file, mediaObj, OMObjID, classID));
	  if (!strncmp(classID, "TIFF", (size_t)4))
		{
		  prop = OMTIFFMobID;
		}
	  else if (!strncmp(classID, "AIFC", (size_t)4))
		{
		  prop = OMAIFCMobID;
		}
	  else if (!strncmp(classID, "WAVE", (size_t)4))
		{
		  prop = OMWAVEMobID;
		}
	  /* In case there is 2.0 media written to a 1.0 file */
	  else if (omfsIsTypeOf(file, mediaObj, "IDAT", &omfError))
		{
		  prop = OMMDATMobID;
		}
	  CHECK(omfsWriteUID(file, mediaObj, prop, mobID));
	}
  XEXCEPT
	{
	}
  XEND;

  return(OM_ERR_NONE);
}
  
/*************************************************************************
 * Function: omfiMobCopyExternal()
 *
 *      This function copies the given source mob in the source file
 *      into a destination mob in the destination file with the given
 *      destMobName.  If resolveDependencies is TRUE, it will also clone
 *      all mobs referenced by the given source mob.  A new mob ID will be
 *      created for the source mob, but all other dependent mobs will be 
 *      cloned (retain the same mob ID).
 *    
 *      If includeMedia is TRUE, it will also copy the media data 
 *      associated with the source mob.  The destination mob (and its
 *      destination media data object, if requested) are returned.  
 *      All private data is also copied.
 *
 *      If the media data is not in this file, it will not attempt to
 *      try and find it in another file and copy it.
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
omfErr_t omfiMobCopyExternal(
    omfHdl_t srcFile,                /* IN - Source File Handle */
	omfMobObj_t srcMob,              /* IN - Source Mob */
	omfDepend_t resolveDependencies, /* IN - Whether to copy dependent mobs */
	omfIncMedia_t includeMedia,      /* IN - Whether to include media data */
	omfString destMobName,           /* IN - Destination Mob Name */
	omfHdl_t destFile,               /* IN - Destination File Handle */
	omfObject_t *destMedia,          /* OUT - Destination Media Data */
	omfMobObj_t *destMob)            /* OUT - Destination Mob */
{
    omfMobObj_t newMob = NULL;
	omfBool isPrimary, isMaster;
	omfObject_t destHead = NULL, srcMediaData = NULL, tmpDestMedia = NULL;
	omfUID_t srcMobID, newMobID;
	omfObjIndexElement_t	elem;
	omfFileRev_t srcRev, destRev;
	omfErr_t omfError = OM_ERR_NONE;
	omfNumTracks_t	numTracks, tkIndex;
	omfIterHdl_t	srcTrackIter = NULL;
	omfMSlotObj_t	srcTrack;
	omfTrackID_t	trackID;
	omfInt16		trackNum;
	omfTrackType_t	trackType;
	omfDDefObj_t	destMediaKind;
		

	*destMob = NULL;
	*destMedia = NULL;
	omfAssertValidFHdl(srcFile);
	omfAssertValidFHdl(destFile);
	omfAssert((destFile->openType != kOmOpenRead), destFile, 
					OM_ERR_WRONG_OPENMODE);
	omfAssert((srcMob != NULL), srcFile, OM_ERR_NULLOBJECT);

	XPROTECT(srcFile)
	  {
		CHECK(omfsFileGetRev(srcFile, &srcRev));
		CHECK(omfsFileGetRev(destFile, &destRev));
		if (srcRev != destRev)
		  {
			RAISE(OM_ERR_FILEREV_DIFF);
		  }

		if (!omfiIsAMob(srcFile, srcMob, &omfError))
		  {
			RAISE(OM_ERR_INVALID_OBJ);
		  }

		CHECK(omfiObjectCopyTreeExternal(srcFile, srcMob, 
									 includeMedia, resolveDependencies,
									 destFile, &newMob));

		/* MobSetNewProps will create a new mob ID, set name, and add
		 * to the Primary mobs index if needed.  It will also add and entry
		 * to the mob hash table.
		 */
		isPrimary = omfiIsAPrimaryMob(srcFile, srcMob, &omfError);
		isMaster = omfiIsAMasterMob(srcFile, srcMob, &omfError);
		if (omfError == OM_ERR_NONE)
		  {
			CHECK(MobSetNewProps(destFile, newMob, isMaster, destMobName, 
									 isPrimary));
		  }
		else
		  {
			RAISE(omfError);
		  }

		/* Store the new mob in the HEAD Mobs index of the destFile */
		CHECK(omfsGetHeadObject(destFile, &destHead));
		/* MobSetIdentity already added it to the appropriate 1.x MobIndex */
		if ((destFile->setrev == kOmfRev1x) || destFile->setrev == kOmfRevIMA)
		  {
			CHECK(omfsAppendObjRefArray(destFile, destHead, OMObjectSpine,
										newMob));
		  }
		else /* kOmfRev2x */
		  {
			CHECK(omfsAppendObjRefArray(destFile, destHead, OMHEADMobs, 
										newMob));
		  }

		CHECK(omfiMobGetMobID(srcFile, srcMob, &srcMobID));
		CHECK(omfiMobGetMobID(destFile, newMob, &newMobID));
//		CHECK(AddMobTableEntry(destFile, newMob, mobID, kOmTableDupError));
		CHECK(omfiMobGetNumTracks(destFile, newMob, &numTracks));
		if ((destFile->setrev == kOmfRev1x) || destFile->setrev == kOmfRevIMA)
		{
			CHECK(omfiIteratorAlloc(srcFile, &srcTrackIter));
			for(tkIndex = 1; tkIndex <= numTracks; tkIndex++)
			{
				CHECK(omfiMobGetNextTrack(srcTrackIter, srcMob, NULL, &srcTrack));
				CHECK(omfiTrackGetInfo(srcFile, srcMob, srcTrack, NULL, 0, NULL, NULL, &trackID, NULL));
				CHECK(CvtTrackIDtoNum(srcFile, srcMobID, trackID, &trackNum));
				CHECK(CvtTrackIDtoTrackType1X(srcFile, srcMobID, trackID, &trackType));
				destMediaKind = TrackTypeToMediaKind(srcFile, trackType);
				CHECK(AddTrackMappingExact(destFile, newMobID, trackID, destMediaKind, trackNum));
			}
			CHECK(omfiIteratorDispose(srcFile, srcTrackIter));
			srcTrackIter = NULL;
		}

		/* Also copyExternal the MediaData object associated with the
		 * Mob, if it is in this file. 
		 */
		if (includeMedia && omfiIsAFileMob(srcFile, srcMob, &omfError))
		  {
			/* Get the srcMob's MobID */
			CHECK(omfiMobGetMobID(srcFile, srcMob, &srcMobID));
			CHECK(omfiMobGetMobID(destFile, newMob, &newMobID));

			/* Retrieve the data object from the srcFile's hash table */
			srcMediaData = (omfObject_t)omfsTableUIDLookupPtr(
									  srcFile->dataObjs, srcMobID);

			/* If the media data is not in this file, don't report an error */
			if (srcMediaData)
			  {
				CHECK(omfiObjectCopyTreeExternal(srcFile, srcMediaData,
											 includeMedia,
											 resolveDependencies,
											 destFile, &tmpDestMedia));

				if ((destFile->setrev == kOmfRev1x) || 
					destFile->setrev == kOmfRevIMA)
				  {
					/* Change the mob ID to be the new mob ID */
					CHECK(Write1xMediaDataMobID(destFile, tmpDestMedia,
												newMobID));
					copyUIDs(elem.ID, newMobID);
					elem.Mob = tmpDestMedia;
					CHECK(omfsAppendObjIndex(destFile, destHead,
												OMMediaData, &elem));
					CHECK(omfsAppendObjRefArray(destFile, destHead,
												OMObjectSpine, tmpDestMedia));
				  }
				else /* kOmfRev2x */
				  {
					/* Change the mob ID to be the new mob ID */
					CHECK(omfsWriteUID(destFile, tmpDestMedia, OMMDATMobID,
									   newMobID));

					CHECK(omfsAppendObjRefArray(destFile, destHead,
											OMHEADMediaData, tmpDestMedia));
				  }
				/* Add to the MediaData hash table */
				CHECK(omfsTableAddUID(destFile->dataObjs, newMobID,
									  tmpDestMedia, kOmTableDupAddDup));
				*destMedia = tmpDestMedia;
			  }
		  } /* includeMedia */
	  } /* XPROTECT */

	XEXCEPT
	  {
	  	if(srcTrackIter != NULL)
	  	{
			CHECK(omfiIteratorDispose(srcFile, srcTrackIter));
			srcTrackIter = NULL;
	  	}
	  	
		/* NOTE: This function needs more cleanup (delete mob, out of index) */
		if (XCODE() == OM_ERR_NO_MORE_OBJECTS)
		  {
			*destMob = newMob;
			return(OM_ERR_NONE);
		  }
		else if (XCODE() == OM_ERR_TABLE_DUP_KEY)
		  return(OM_ERR_DUPLICATE_MOBID);

		return(XCODE());
	  }
	XEND;

	*destMob = newMob;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobCloneExternal()
 *
 *      This function clones the given source mob in the source file into
 *      a destination mob in the destination file with the same Mob ID.
 *      If resolveDependencies is TRUE, it will also clone all mobs 
 *      referenced by the given source mob. 
 *
 *      If includeMedia is TRUE, it will also copy the media data 
 *      associated with the source mob.  The destination mob is
 *      returned. All private data is also cloned.
 *
 *      If the media data is not in the file, it will not attempt to
 *      try and find it in another file and clone it.
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
omfErr_t omfiMobCloneExternal(
    omfHdl_t srcFile,                 /* IN - Source File Handle */
	omfMobObj_t srcMob,               /* IN - Source Mob */
	omfDepend_t resolveDependencies,  /* IN - Whether to clone dependent mobs*/
	omfIncMedia_t includeMedia,       /* IN - Whether to include media data */
	omfHdl_t destFile,                /* IN - Destination File Handle */
	omfMobObj_t *destMob)             /* OUT - Destination Mob */
{
    omfUID_t saveMobID, newMobID;
	omfString destMobName = NULL;
    omfMobObj_t tmpDestMob = NULL;
	omfObject_t tmpDestMedia = NULL;
	omfObject_t destHead = NULL;
	omfTimeStamp_t saveCreationTime, saveModTime;
	omfInt32 loop, len;
	omfObjIndexElement_t	elem;
	omfFileRev_t srcRev, destRev;
	omfErr_t omfError = OM_ERR_NONE;

	*destMob = NULL;
	omfAssert((srcFile != NULL) && (srcFile->cookie == FILE_COOKIE), 
		     srcFile, OM_ERR_BAD_FHDL);
	omfAssert((srcMob != NULL), srcFile, OM_ERR_NULLOBJECT);

	XPROTECT(srcFile)
	  {
		CHECK(omfsFileGetRev(srcFile, &srcRev));
		CHECK(omfsFileGetRev(destFile, &destRev));
		if (srcRev != destRev)
		  {
			RAISE(OM_ERR_FILEREV_DIFF);
		  }

		/* Copy the Mob, then change the MobID back to its original */
		/* Remember the creation and modification times, to restore later */
		CHECK(omfiMobGetInfo(srcFile, srcMob, &saveMobID, 0, NULL, 
							 &saveModTime, &saveCreationTime));

		destMobName = (omfString)omOptMalloc(srcFile, OMMOBNAME_SIZE);
		strncpy(destMobName, "\0", 1);
		if ((srcFile->setrev == kOmfRev1x) || srcFile->setrev == kOmfRevIMA)
		  {
			omfError = omfsReadString(srcFile, srcMob, OMCPNTName, 
									  (char *)destMobName, OMMOBNAME_SIZE);
		  }
		else
		  {
			omfError = omfsReadString(srcFile, srcMob, OMMOBJName, 
									  (char *)destMobName, OMMOBNAME_SIZE);
		  }

		CHECK(omfiMobCopyExternal(srcFile, srcMob, resolveDependencies,
								  includeMedia, destMobName, destFile,
								  &tmpDestMedia, &tmpDestMob));
		CHECK(omfiMobGetMobID(destFile, tmpDestMob, &newMobID));

		 /* omfiMobSetIdentity will take out the existing hash table
		 * entry and add an entry for the original cloned mobID.
		 */
		CHECK(omfiMobSetIdentity(destFile, tmpDestMob, saveMobID));

		/* Restore creation and modification times */
		if (destFile->setrev == kOmfRev2x) /* Only 2.x has creationTime */
		  {
			CHECK(omfsWriteTimeStamp(destFile, tmpDestMob, OMMOBJCreationTime,
									 saveCreationTime));
		  }
		CHECK(omfsWriteTimeStamp(destFile, tmpDestMob, OMMOBJLastModified,
								 saveModTime)); 

		/* If it is a primary mob, put it in the primary mob index */
		if (omfiIsAPrimaryMob(srcFile, srcMob, &omfError))
		  {
			if (omfError == OM_ERR_NONE)
			  {
				CHECK(omfiMobSetPrimary(destFile, tmpDestMob, TRUE));
			  }
			else
			  {
				RAISE(omfError);
			  }
		  }
		if (destMobName)
		  omOptFree(srcFile, destMobName);
		destMobName = NULL;

		if (includeMedia)
		  {
			/* The media data object has already been copied.  Now, we have
			 * to change the MobID back to the original, since we're cloning 
			 */
			if (tmpDestMedia)
			  {
				/* Change the mob ID to be the original mob ID */
				if ((destFile->setrev == kOmfRev1x) || 
					destFile->setrev == kOmfRevIMA)
				  {
					CHECK(Write1xMediaDataMobID(destFile, tmpDestMedia,
												saveMobID));
				  }
				else /* kOmfRev2x */
				  {
					CHECK(omfsWriteUID(destFile, tmpDestMedia, OMMDATMobID,
									   saveMobID));
				  }

				CHECK(omfsTableRemoveUID(destFile->dataObjs, newMobID));
				/* Add to the MediaData hash table */
				CHECK(omfsTableAddUID(destFile->dataObjs, saveMobID,
									  tmpDestMedia, kOmTableDupAddDup));

				/* If 1.x file, patch the MediaData index */
				if ((destFile->setrev == kOmfRev1x) || 
					destFile->setrev == kOmfRevIMA)
				  {
					CHECK(omfsGetHeadObject(destFile, &destHead));
					len = omfsLengthObjIndex(destFile, destHead, OMMediaData);
					for (loop = 1; loop <= len; loop++)
					  {
						CHECK(omfsGetNthObjIndex(destFile, destHead, 
												 OMMediaData, &elem, loop));
						if (equalUIDs(elem.ID, newMobID))
						  {
							CHECK(omfsRemoveNthObjIndex(destFile, destHead, 
														OMMediaData, loop));
							break;
						  }
					  }
					copyUIDs(elem.ID, saveMobID);
					elem.Mob = tmpDestMedia;
					CHECK(omfsAppendObjIndex(destFile, destHead, OMMediaData, 
											 &elem));
				  }
			  }

			/* If the media data is not in this file, don't report an error */
		  }

	  }
	XEXCEPT
	  {
		if (destMobName)
		  omOptFree(srcFile, destMobName);
		/* NOTE: This function needs more cleanup (delete mob, out of index) */
		if (XCODE() == OM_ERR_TABLE_DUP_KEY)
		  return(OM_ERR_DUPLICATE_MOBID);
		return(XCODE());
	  }
	XEND;

	*destMob = tmpDestMob;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: DeleteFromObjectSpine()
 *
 *      This internal function deletes the given mob from the 1.x file 
 *      HEAD's Object Spine.
 *
 *      This function only supports 1.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t DeleteFromObjectSpine(
    omfHdl_t file,     /* IN - File Handle */
    omfObject_t head,  /* IN - File HEAD object */
    omfMobObj_t mob)   /* IN - Mob to delete */
{
    omfInt32 loop, len;
    omfObject_t tmpMob = NULL;
    omfUID_t mobID, tmpMobID;
    omfErr_t omfError;

	XPROTECT(file)
	  {
		len = omfsLengthObjRefArray(file, head, OMObjectSpine);
		CHECK(omfiMobGetMobID(file, mob, &mobID));
		for (loop = 1; loop <= len; loop++)
		  {
			CHECK(omfsGetNthObjRefArray(file, head, OMObjectSpine, &tmpMob, 
										loop));
			omfError = omfiMobGetMobID(file, tmpMob, &tmpMobID);
			/* Release Bento reference, so the useCount is decremented */
			if (tmpMob)
			  CMReleaseObject((CMObject)tmpMob);

			if ((omfError == OM_ERR_NONE) && equalUIDs(mobID, tmpMobID))
			  {
				CHECK(omfsRemoveNthObjRefArray(file, head, OMObjectSpine, 
											   loop));
				break;
			  }
		  }


	  } /* XPROTECT */

	XEXCEPT
	  {
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobDelete()
 *
 *      This function deletes the mob with the given mobID from the
 *      file specified by the file handle.  It will delete all objects
 *      contained in the mob, and remove the mob from all indices and
 *      internal data structures.
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
omfErr_t omfiMobDelete(
	omfHdl_t file,   /* IN - File Handle */
	omfUID_t mobID)  /* IN - Mob ID of mob to delete */
{
    omfObject_t mob = NULL, head;
	omfInt32 index;
	omfProperty_t prop;
	omfBool isComp = FALSE;
	mobTableEntry_t *entry;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			prop = OMSourceMobs;
			omfError = FindMobInIndex(file, mobID, prop, &index, &mob);
			/* If its not in the Source index, look in the Compostion index */
			if (omfError == OM_ERR_MOB_NOT_FOUND)
			  {
				omfError = OM_ERR_NONE;
				prop = OMCompositionMobs;
				CHECK(FindMobInIndex(file, mobID, prop, &index, &mob));
				isComp = TRUE;
			  }
			else if (omfError != OM_ERR_NONE)
			  {
				RAISE(omfError);
			  }
		  }
		else /* kOmfRev2x */
		  {
			prop = OMHEADMobs;
			CHECK(FindMobInIndex(file, mobID, prop, &index, &mob));
		  }

		if (mob)
		  {
			if (omfError != OM_ERR_NONE)
			  {
				RAISE(omfError);
			  }
			else
			  {
				/* Take out of Primary Mob index, if necessary */
				if (omfiIsAPrimaryMob(file, mob, &omfError))
				  {
					if (omfError == OM_ERR_NONE)
					  {
						CHECK(omfiMobSetPrimary(file, mob, FALSE));
					  }
					else
					  {
						RAISE(omfError);
					  }
				  }

				/* Delete out of mob cache */
				entry = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, mobID);
				/* Tell Bento that cache's object reference has been deleted */
				if (entry)
				  CMReleaseObject((CMObject)(entry->mob));
				CHECK(omfsTableRemoveUID(file->mobs, mobID));

				/* Remove from Mobs index */
				CHECK(omfsGetHeadObject(file, &head));
				if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
				  {
					if (isComp)
					  prop = OMCompositionMobs;
					else
					  prop = OMSourceMobs;
					CHECK(omfsRemoveNthObjIndex(file, head, prop, index));
					CHECK(DeleteFromObjectSpine(file, head, mob));
				  }
				else /* kOmfRev2x */
				  {
					CHECK(omfsRemoveNthObjRefArray(file, head, OMHEADMobs, 
												   index));
				  }

				/* Delete Object */
				CHECK(omfiObjectDeleteTree(file, mob));
			  }
		  }
	  }

	XEXCEPT
	  {
		/* NOTE: This function needs more cleanup (delete mob, out of index) */
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiObjectDeleteTree()
 *
 *      This function recursively deletes all objects in the subtree rooted
 *      at the object identified by the 'obj' parameter.  It is used by
 *      the omfiMobDelete() function.
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
omfErr_t omfiObjectDeleteTree(
    omfHdl_t file,    /* IN - File Handle */
	omfObject_t obj)  /* IN - Object to delete */
{
    omfIterHdl_t propIter = NULL;
	omfProperty_t prop;
	omfType_t type;
	CMProperty cprop;
	CMType ctype;
	CMValue cvalue;
	omfInt32 arrayLen, loop;
	omfObject_t objRef;
	omfErr_t omfError = OM_ERR_NONE;

	clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiIteratorAlloc(file, &propIter));

		CHECK(omfiGetNextProperty(propIter, obj, &prop, &type));
		while (prop)
		  {
			cprop = CvtPropertyToBento(file, prop);
			ctype = CvtTypeToBento(file, type, NULL);
			if (file->BentoErrorRaised)
			  {
				RAISE(OM_ERR_BENTO_PROBLEM);
			  }
			if (CMCountValues((CMObject)obj, cprop, ctype))
			  {
				cvalue = CMGetNextValue((CMObject)obj, cprop, NULL);
				if (type == OMObjRefArray)
				  {
					arrayLen = omfsLengthObjRefArray(file, obj,
												 (omfProperty_t)prop);
					for (loop=1; loop<=arrayLen; loop++)
					  {
						CHECK(omfsGetNthObjRefArray(file, obj, prop, &objRef, 
												loop));
						  /* If not Datakind or Effectdef, delete object */
						  if (objRef)
							{
							  if (!omfiIsADatakind(file, objRef, &omfError) && 
								  !omfiIsAnEffectDef(file, objRef, &omfError))
								{
								  if (omfError == OM_ERR_NONE)
									{
									  CHECK(omfiObjectDeleteTree(file, objRef));
									}
								  else
									{
									  RAISE(omfError);
									}
								}
							}
					  } /* for */
				  } /* If ObjRefArray */
				else if (type == OMObjRef)
				  {
					CHECK(omfsReadObjRef(file, obj, prop, &objRef));
					  if (objRef)
						{
						  /* If not Datakind or Effectdef, delete object */
						  if (!omfiIsADatakind(file, objRef, &omfError) && 
							  !omfiIsAnEffectDef(file, objRef, &omfError))
							{
							  if (omfError == OM_ERR_NONE)
								{
								  CHECK(omfiObjectDeleteTree(file, objRef));
								}
							  else
								{
								  RAISE(omfError);
								}
							}
						}
				  } /* If ObjRef */
			  } /* if Count Values */
			CHECK(omfiGetNextProperty(propIter, obj, &prop, &type));
		  } /* while */
		
		/* After all subtrees are deleted, delete obj */
		/* NOTE: Shouldn't reach this point, deletion should happen
		 * in the exception handler when OM_ERR_NO_MORE_OBJECTS is handled */
		CHECK(omfsObjectDelete(file, obj));

	  } /* XPROTECT */

	XEXCEPT
	  {
		/* After all subtrees are deleted, delete obj */
		if (propIter)
		  omfiIteratorDispose(file, propIter);
		if (XCODE() == OM_ERR_NO_MORE_OBJECTS)
		  {
			omfError = omfsObjectDelete(file, obj);
			return(omfError); 
		  }
		return(XCODE()); 
	  }
    XEND;

	if (propIter)
	  omfiIteratorDispose(file, propIter);
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: CopyProperty() (ORIG)
 *
 *      This internal function is called by the public clone and
 *      copy functions to copy the given property values to the 
 *      specified object.
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
omfErr_t CopyProperty(
							omfHdl_t file,     /* IN - File Handle */
							omfObject_t obj,   /* IN - Object to copy to */
							CMProperty cprop,  /* IN - Bento property to copy*/
							CMValue cpropValue, /* IN - Bento value to copy */
							CMType ctype)      /* IN - Bento type to copy */
{
  CMValue val;
  unsigned char buffer[BUFSIZE];
  omfUInt32 	valSize32, remSize32;
  omfInt32	readSize = 0;
  CMCount	offset;
  CMSize valueSize, remSize;
  omfInt64	zero;
  omfErr_t	status;

  omfsCvtInt32toInt64(0, &zero);
  offset = zero;
  clearBentoErrors(file);
  omfAssertValidFHdl(file);
  omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);

  XPROTECT(file)
	{
	  if (CMCountValues((CMObject)obj, cprop, ctype))
		val = CMUseValue((CMObject)obj, cprop, ctype);
	  else
		val = CMNewValue((CMObject)obj, cprop, ctype);
        
	  if (file->BentoErrorRaised)
		{
		  RAISE(OM_ERR_BENTO_PROBLEM);
		}

	  valueSize = CMGetValueSize(cpropValue);
	  status = omfsTruncInt64toUInt32(valueSize, &valSize32);	/* OK HANDLED */
	  if ((status != OM_ERR_NONE) || valSize32 > BUFSIZE)
		{
		  /* Calculate number of total reads, and copy a BUFSIZE data 
		   * buffer at a time.
		   */
		  remSize = valueSize;
		  while (omfsInt64Greater(remSize, zero))
			{
	  	 	status = omfsTruncInt64toUInt32(remSize, &remSize32);		/* OK HANDLED */
			  readSize = (((status == OM_ERR_NONE) && (remSize32 < BUFSIZE)) ? remSize32 : BUFSIZE);
			  if (readSize)
				{
				  CMReadValueData(cpropValue, (CMPtr)buffer, offset, readSize);
				  CMWriteValueData(val, (CMPtr)buffer, offset, readSize);
				  omfsAddInt32toInt64(readSize, &offset);
				  omfsSubInt32fromInt64(readSize, &remSize);
				}
			}
		}
	  else /* Read/write < BUFSIZE bytes */
		{
		  CMReadValueData(cpropValue, (CMPtr)buffer, zero, valSize32);
		  CMWriteValueData(val, (CMPtr)buffer, zero, valSize32);
		}

	  if (file->BentoErrorRaised)
		{
		  RAISE(OM_ERR_BENTO_PROBLEM);  
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
 * Private Function: CopyProperty()
 *
 *      This internal function is called by the public clone and
 *      copy external functions to copy the given property values to the 
 *      specified object.  This calls the accessors which will take care
 *      of byte ordering differences for types on different platforms.
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
omfErr_t omfiCopyPropertyExternal(
					  omfHdl_t srcFile,       /* IN - Src File Handle */
					  omfHdl_t destFile,      /* IN - Dest File Handle */
					  omfObject_t srcObj,     /* IN - Object to copy from */	
					  omfObject_t destObj,    /* IN - Object to copy to */
					  omfProperty_t srcProp,  /* IN - Src Property Name */
					  omfType_t srcType,      /* IN - Src Property Type Name */
					  omfProperty_t destProp, /* IN - Dest Property Name */
					  omfType_t destType)     /* IN - Src Property Type Name */
{
  CMProperty cprop, newcProp;
  CMType ctype, newcType;
  CMValue cvalue;
  omfProperty_t newProp;
  omfType_t newType;
  omfBool found = FALSE;

  clearBentoErrors(srcFile);
  omfAssertValidFHdl(srcFile);
  omfAssert((srcObj != NULL), srcFile, OM_ERR_NULLOBJECT);  
  omfAssert((destObj != NULL), destFile, OM_ERR_NULLOBJECT);

  XPROTECT(srcFile)
	{
	  /* Register property/type with new container */
#if 1
		CMGlobalName propName, propTypeName;

	  /* Get the omfProperty_t and omfType_t from the Bento equivalents 
	   * that were just created.
	   */
		cprop = CvtPropertyToBento(srcFile, destProp);
		if(cprop != NULL)
		{
			propName = (char *)CMGetGlobalName((CMObject)cprop);
			newcProp = CMRegisterProperty(destFile->container, propName);
			newProp = CvtPropertyFromBento(destFile, newcProp, &found);
		}
		else
			newProp = destProp;

		ctype = CvtTypeToBento(srcFile, destType, NULL);
		if(ctype != NULL)
		{
			propTypeName = (char *)CMGetGlobalName((CMObject)ctype);
			newcType = CMRegisterType(destFile->container, propTypeName);
		 	newType = CvtTypeFromBento(destFile, newcType, &found);
		}
	  	else
	  		newType = destType;

#else
		  newProp = destProp;
		  newType = destType;
#endif

	  switch (newType) 
		{
		case OMAttrKind: {
		  omfAttributeKind_t attrKind;

		  CHECK(omfsReadAttrKind(srcFile, srcObj, srcProp, &attrKind));
		  CHECK(omfsWriteAttrKind(destFile, destObj, newProp, attrKind));
		  break;
		}      
		case OMBoolean: {
		  omfBool boolData;

		  CHECK(omfsReadBoolean(srcFile, srcObj, srcProp, &boolData));
		  CHECK(omfsWriteBoolean(destFile, destObj, newProp, boolData));
		  break;
		}
		case OMInt8: {
		  char int8Data;

		  CHECK(omfsReadInt8(srcFile, srcObj, srcProp, &int8Data));
		  CHECK(omfsWriteInt8(destFile, destObj, newProp, int8Data));
		  break;
		}
		case OMCharSetType: {
		  omfCharSetType_t charSetData;

		  CHECK(omfsReadCharSetType(srcFile, srcObj, srcProp, &charSetData));
		  CHECK(omfsWriteCharSetType(destFile, destObj, newProp, charSetData));
		  break;
		}
		case OMEdgeType: {
		  omfEdgeType_t edgeData;

		  CHECK(omfsReadEdgeType(srcFile, srcObj, srcProp, &edgeData));
		  CHECK(omfsWriteEdgeType(destFile, destObj, newProp, edgeData));
		  break;
		}
		case OMExactEditRate: {
		  omfRational_t editRateData;

		  if(srcType == OMExactEditRate)
			  {
				CHECK(omfsReadExactEditRate(srcFile, srcObj, srcProp,&editRateData));
			  }
		  else
			  {
				CHECK(omfsReadRational
					(srcFile, srcObj, srcProp,&editRateData));
			  }
		  CHECK(omfsWriteExactEditRate(destFile, destObj, newProp, 
									   editRateData));
		  break;
		}
		case OMFilmType: {
		  omfFilmType_t filmData;

		  CHECK(omfsReadFilmType(srcFile, srcObj, srcProp, &filmData));
		  CHECK(omfsWriteFilmType(destFile, destObj, newProp, filmData));
		  break;
		}
		case OMJPEGTableIDType: {
		  omfJPEGTableID_t jpegIDData;

		  CHECK(omfsReadJPEGTableIDType(srcFile, srcObj, srcProp, 
										&jpegIDData));
		  CHECK(omfsWriteJPEGTableIDType(destFile, destObj, newProp, 
										 jpegIDData));
		  break;
		}
		case OMInt32: {
		  omfInt32 int32Data;
		  if (srcType == OMLength32)
			  {
			  omfLength_t lengthData;
			  CHECK(omfsReadLength(srcFile, srcObj, srcProp, &lengthData));
			  CHECK(omfsTruncInt64toInt32(lengthData, &int32Data));
			  }
		  else
			  {
			  CHECK(omfsReadInt32(srcFile, srcObj, srcProp, &int32Data));
			  }
		  CHECK(omfsWriteInt32(destFile, destObj, newProp, int32Data));
		  break;
		}

		case OMObjectTag: {
		  omfObjectTag_t tagData;

		  CHECK(omfsReadObjectTag(srcFile, srcObj, srcProp, 
								  (omfTagPtr_t)&tagData));
		  CHECK(omfsWriteObjectTag(destFile, destObj, newProp, tagData));
		  break;
		}
		case OMPhysicalMobType: {
		  omfPhysicalMobType_t physMobData;

		  CHECK(omfsReadPhysicalMobType(srcFile, srcObj, srcProp, 
										&physMobData));
		  CHECK(omfsWritePhysicalMobType(destFile, destObj, newProp, 
										 physMobData));
		  break;
		}
		case OMInt16: {
		  omfInt16 int16Data;

		  CHECK(omfsReadInt16(srcFile, srcObj, srcProp, &int16Data));
		  CHECK(omfsWriteInt16(destFile, destObj, newProp, int16Data));
		  break;
		}
		case OMString: {
		  omfInt32 strsize;
		  char *stringData;

		  strsize = omfsLengthString(srcFile, srcObj, srcProp);
		  stringData = (char*)omfsMalloc(strsize+1);
		  /* The last parameter to omfsReadString is the BUFFER size */
		  CHECK(omfsReadString(srcFile, srcObj, srcProp, stringData, strsize+1));
		  stringData[strsize] = '\0';
		  CHECK(omfsWriteString(destFile, destObj, newProp, stringData));
		  if (stringData)
			omfsFree(stringData);
		  break;
		}
		case OMTimeStamp: {
		  omfTimeStamp_t timeData;

		  CHECK(omfsReadTimeStamp(srcFile, srcObj, srcProp, &timeData));
		  CHECK(omfsWriteTimeStamp(destFile, destObj, newProp, timeData));
		  break;
		}
		case OMTrackType: {
		  omfInt16 trackType;

		  CHECK(omfsReadTrackType(srcFile, srcObj, srcProp, &trackType));
		  CHECK(omfsWriteTrackType(destFile, destObj, newProp, trackType));
		  break;
		}
		case OMUID: {
		  omfUID_t uid;

		  CHECK(omfsReadUID(srcFile, srcObj, srcProp, &uid));
		  CHECK(omfsWriteUID(destFile, destObj, newProp, uid));
		  break;
		}
		case OMUInt8: {
		  omfUInt8 uint8Data;

		  CHECK(omfsReadUInt8(srcFile, srcObj, srcProp, &uint8Data));
		  CHECK(omfsWriteUInt8(destFile, destObj, newProp, uint8Data));
		  break;
		}
		case OMUInt32: {
		  omfUInt32 uint32Data;

		  CHECK(omfsReadUInt32(srcFile, srcObj, srcProp, &uint32Data));
		  CHECK(omfsWriteUInt32(destFile, destObj, newProp, uint32Data));
		  break;
		}
		case OMUInt16: {
		  omfUInt16 uint16Data;
		  CHECK(omfsReadUInt16(srcFile, srcObj, srcProp, &uint16Data));
		  CHECK(omfsWriteUInt16(destFile, destObj, newProp, uint16Data));
		  break;
		}
		case OMUsageCodeType: {
		  omfUsageCode_t usageCode;

		  CHECK(omfsReadUsageCodeType(srcFile, srcObj, srcProp, &usageCode));
		  CHECK(omfsWriteUsageCodeType(destFile, destObj, newProp, usageCode));
		  break;
		}
		case OMVarLenBytes: {
		  /* NOTE: For now, use CopyProperty() since we're not worrying
		   * about byte swapping.
		   */
			cprop = CvtPropertyToBento(srcFile, srcProp);
			ctype = CvtTypeToBento(srcFile, srcType, NULL);
			newcProp = CvtPropertyToBento(destFile, destProp);
			newcType = CvtTypeToBento(destFile, destType, NULL);
		  if (CMCountValues((CMObject)srcObj, cprop, ctype))
			{
			  cvalue = CMGetNextValue((CMObject)srcObj, cprop, NULL);
			  CHECK(CopyProperty(destFile, destObj, newcProp, 
								 cvalue, newcType));
			}
		  break;
		}
		case OMVersionType: {
		  omfVersionType_t vers;

		  CHECK(omfsReadVersionType(srcFile, srcObj, srcProp, &vers));
		  CHECK(omfsWriteVersionType(destFile, destObj, newProp, vers));
		  break;
		}
		case OMPosition32:
		case OMPosition64: {
		  omfPosition_t pos;

		  CHECK(omfsReadPosition(srcFile, srcObj, srcProp, &pos));
		  CHECK(omfsWritePosition(destFile, destObj, newProp, pos));
		  break;
		}
		case OMLength32:
		case OMLength64: {
		  omfLength_t len;

		  CHECK(omfsReadLength(srcFile, srcObj, srcProp, &len));
		  CHECK(omfsWriteLength(destFile, destObj, newProp, len));
		  break;
		}
		case OMClassID: {
		  omfClassID_t classID;

		  CHECK(omfsReadClassID(srcFile, srcObj, srcProp, classID));
		  CHECK(omfsWriteClassID(destFile, destObj, newProp, classID));
		  break;
		}
		case OMUniqueName: {
		  omfUniqueName_t name;

		  CHECK(omfsReadUniqueName(srcFile, srcObj, srcProp, name, 
								   OMUNIQUENAME_SIZE));
		  CHECK(omfsWriteUniqueName(destFile, destObj, newProp, name));
		  break;
		}
		case OMFadeType: {
		  omfFadeType_t fade;

		  CHECK(omfsReadFadeType(srcFile, srcObj, srcProp, &fade));
		  CHECK(omfsWriteFadeType(destFile, destObj, newProp, fade));
		  break;
		}
		case OMInterpKind: {
		  omfInterpKind_t interpKind;

		  CHECK(omfsReadInterpKind(srcFile, srcObj, srcProp, &interpKind));
		  CHECK(omfsWriteInterpKind(destFile, destObj, newProp, interpKind));
		  break;
		}      
		case OMArgIDType: {
		  omfArgIDType_t argID;

		  CHECK(omfsReadArgIDType(srcFile, srcObj, srcProp, &argID));
		  CHECK(omfsWriteArgIDType(destFile, destObj, newProp, argID));
		  break;
		}
		case OMEditHintType: {
		  omfEditHint_t editHint;

		  CHECK(omfsReadEditHint(srcFile, srcObj, srcProp, &editHint));
		  CHECK(omfsWriteEditHint(destFile, destObj, newProp, editHint));
		  break;
		}
		case OMDataValue: {
		  /* NOTE: For now, use CopyProperty() since we're not worrying
		   * about byte swapping.
		   */
			cprop = CvtPropertyToBento(srcFile, srcProp);
			ctype = CvtTypeToBento(srcFile, srcType, NULL);
			newcProp = CvtPropertyToBento(destFile, destProp);
			newcType = CvtTypeToBento(destFile, destType, NULL);
		  if (CMCountValues((CMObject)srcObj, cprop, ctype))
			{
			  cvalue = CMGetNextValue((CMObject)srcObj, cprop, NULL);
			  CHECK(CopyProperty(destFile, destObj, newcProp, 
								 cvalue, newcType));
			}
		  break;
		}
		case OMTapeCaseType: {
		  omfTapeCaseType_t tapeCase;

		  CHECK(omfsReadTapeCaseType(srcFile, srcObj, srcProp, &tapeCase));
		  CHECK(omfsWriteTapeCaseType(destFile, destObj, newProp, tapeCase));
		  break;
		}
		case OMVideoSignalType: {
		  omfVideoSignalType_t sig;

		  CHECK(omfsReadVideoSignalType(srcFile, srcObj, srcProp, &sig));
		  CHECK(omfsWriteVideoSignalType(destFile, destObj, newProp, sig));
		  break;
		}
		case OMTapeFormatType: {
		  omfTapeFormatType_t format;

		  CHECK(omfsReadTapeFormatType(srcFile, srcObj, srcProp, &format));
		  CHECK(omfsWriteTapeFormatType(destFile, destObj, newProp, format));
		  break;
		}
		case OMRational: {
		  omfRational_t rat;

		  if(srcType == OMRational)
			  {
				CHECK(omfsReadRational(srcFile, srcObj, srcProp, &rat));
			  }
		  else
			  {
				 CHECK(omfsReadExactEditRate(srcFile, srcObj, srcProp,&rat));
			  }
		  CHECK(omfsWriteRational(destFile, destObj, newProp, rat));
		  break;
		}
		case OMLayoutType: {
		  omfFrameLayout_t layout;

		  CHECK(omfsReadLayoutType(srcFile, srcObj, srcProp, &layout));
		  CHECK(omfsWriteLayoutType(destFile, destObj, newProp, layout));
		  break;
		}
		case OMCompCodeArray: 
		case OMCompSizeArray: {
		  /* NOTE: Use CopyProperty() its just a stream of bytes which
		   * aren't swapped.
		   */
			cprop = CvtPropertyToBento(srcFile, srcProp);
			ctype = CvtTypeToBento(srcFile, srcType, NULL);
			newcProp = CvtPropertyToBento(destFile, destProp);
			newcType = CvtTypeToBento(destFile, destType, NULL);
		  if (CMCountValues((CMObject)srcObj, cprop, ctype))
			{
			  cvalue = CMGetNextValue((CMObject)srcObj, cprop, NULL);
			  CHECK(CopyProperty(destFile, destObj, newcProp, 
								 cvalue, newcType));
			}
		  break;
		}
		case OMColorSitingType: {
		  short colorSiting;
		  omfPosition_t			zeroPos;
		  omfLength_t			one, oldLen;
		  char					aByte;
		  
		  omfsCvtInt32toPosition(0, zeroPos);
		  omfsCvtInt32toLength(1, one);
			CHECK(OMLengthProp(srcFile, srcObj, srcProp,		/* Fix this (write to short) */
						   srcType,
						   0, &oldLen));
			if(omfsInt64Equal(oldLen, one))
		  {
		  	CHECK(OMReadProp(srcFile, srcObj, srcProp,		/* Fix this (write to short) */
						   zeroPos, kSwabIfNeeded, srcType,
						   sizeof(aByte), (void *)&aByte));
			colorSiting = aByte;
		  }
		  else
		  {
		  	CHECK(OMReadProp(srcFile, srcObj, srcProp,		/* Fix this (write to short) */
						   zeroPos, kSwabIfNeeded, srcType,
						   sizeof(colorSiting), (void *)&colorSiting));
		  }
		  CHECK(OMWriteBaseProp(destFile, destObj, srcProp,
								srcType, sizeof(colorSiting),
								(void *)&colorSiting));
		  break;
		}
		case OMPulldownKindType: {
			omfPulldownKind_t	kind;
			CHECK(omfsReadPulldownKindType(srcFile, srcObj, srcProp, &kind));
			CHECK(omfsWritePulldownKindType(destFile, destObj, srcProp, kind));
		}
		case OMPhaseFrameType: {
			omfPhaseFrame_t	frame;
			CHECK(omfsReadPhaseFrameType(srcFile, srcObj, srcProp, &frame));
			CHECK(omfsWritePhaseFrameType(destFile, destObj, srcProp, frame));
		}
		case OMPulldownDirectionType: {
			omfPulldownDir_t	dir;
			CHECK(omfsReadPulldownDirectionType(srcFile, srcObj, srcProp, &dir));
			CHECK(omfsWritePulldownDirectionType(destFile, destObj, srcProp, dir));
		}

		// SPIDY default: {
		case OMInt64: {
			omfInt64		value;
			CHECK(OMReadBaseProp(srcFile, srcObj, srcProp, srcType, sizeof(value), &value));
			CHECK(OMWriteBaseProp(destFile, destObj, srcProp, newType, sizeof(value), &value));
		  break;
		}

		/* Don't have to worry about the following since they either won't
		 * appear in mobs, or are handled elsewhere.
		 *        OMMobIndex, OMObjRefArray, OMObjRef 
		 * Don't support (yet):
		 *        OMPosition32Array, OMInt32Array
		 */
		} /* switch */
	} /* XPROTECT */

  XEXCEPT
	{
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
 }

/*************************************************************************
 * Function: omfiObjectCopyTree()
 *
 *      This function recursively copies all objects in the subtree rooted
 *      at the object identified by the 'obj' parameter.  It is used by the
 *      copy and clone functions.  All private data is copied.  The
 *      destination object is created and returned.
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
omfErr_t omfiObjectCopyTree(
	omfHdl_t file,        /* IN - File Handle */
	omfObject_t srcObj,   /* IN - Object to copy */
	omfObject_t *destObj) /* OUT - Destination object */
{
    omfIterHdl_t propIter = NULL;
	omfProperty_t prop;
	omfType_t type;
	CMProperty cprop;
	CMType ctype;
	CMValue cvalue;
	omfObject_t newObject;
	omfClassID_t classID;
	omfInt32 arrayLen, loop;
	omfObject_t newRef, objRef;
	omfErr_t omfError = OM_ERR_NONE;
	omfProperty_t	idProp;
	
	clearBentoErrors(file);
	*destObj = NULL;
	omfAssertValidFHdl(file);
	omfAssert((srcObj != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((file->openType != kOmOpenRead), file, 
					OM_ERR_WRONG_OPENMODE);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		/* Create the new destination object */
		CHECK(omfsReadClassID(file, srcObj, idProp, classID));
		CHECK(omfsObjectNew(file, classID, &newObject));
	  
		CHECK(omfiIteratorAlloc(file, &propIter));
	
		/* Iterate over properties and copy.If ObjRef or ObjRefArray,recurse */
		CHECK(omfiGetNextProperty(propIter, srcObj, &prop, &type));

		while ((omfError == OM_ERR_NONE) && prop)
		  {
			cprop = CvtPropertyToBento(file, prop);
			ctype = CvtTypeToBento(file, type, NULL);
			if (file->BentoErrorRaised)
			  {
				RAISE(OM_ERR_BENTO_PROBLEM);
			  }

			if (CMCountValues((CMObject)srcObj, cprop, ctype))
			  {
				cvalue = CMGetNextValue((CMObject)srcObj, cprop, NULL);

				if (type == OMObjRefArray)
				  {
					arrayLen = omfsLengthObjRefArray(file, srcObj, 
													 (omfProperty_t)prop);
					for (loop=1; loop<=arrayLen; loop++)
					  {
						CHECK(omfsGetNthObjRefArray(file, srcObj, 
													   prop, &objRef, loop));

						if (objRef)
						  {
							/* If not Datakind or Effectdef, copy object */
							if (!omfiIsADatakind(file, objRef, &omfError) && 
								!omfiIsAnEffectDef(file, objRef, &omfError))
							  {
								CHECK(omfiObjectCopyTree(file, objRef, &newRef));
								if (newRef)
								  omfsAppendObjRefArray(file, newObject, prop, 
														newRef);
							  }
							/* Append ObjRef of existing datakd or effdef */
							else 
							  {
								CHECK(omfsAppendObjRefArray(file, newObject,
															 prop, objRef));
							  }
						  }
					  }
				  } /* If ObjRefArray */
				else if (type == OMObjRef)
				  {
					CHECK(omfsReadObjRef(file, srcObj, prop, &objRef));

					if (objRef)
					  {
						/* If not Datakind or Effectdef, copy object */
						if (!omfiIsADatakind(file, objRef, &omfError) && 
						!omfiIsAnEffectDef(file, objRef, &omfError))
						  {	
							CHECK(omfiObjectCopyTree(file, objRef, &newRef));
							if (newRef)
							  {
								CHECK(omfsWriteObjRef(file, newObject, prop, 
													   newRef));
							  }
						  }
						/* Write ObjRef of existing datakind or effectdef */
						else 
						  {
							CHECK(omfsWriteObjRef(file, newObject, prop, 
												  objRef));
						  }
					  }
				  } /* If ObjRef */
				else
				  {
					CHECK(CopyProperty(file, newObject, cprop, cvalue, ctype));
				  }
			  } /* if Count Values */
			CHECK(omfiGetNextProperty(propIter, srcObj, &prop, &type));
		  } /* while */

	  } /* XPROTECT */

	XEXCEPT
	  {
		if (propIter)
		  omfiIteratorDispose(file, propIter);
		if (XCODE() == OM_ERR_NO_MORE_OBJECTS)
		  {
			*destObj = newObject;
			return(OM_ERR_NONE);
		  }

		return(XCODE());
	  }
	XEND;

	*destObj = newObject;
	if (propIter)
	  omfiIteratorDispose(file, propIter);
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: MakeNewDatakind()
 *
 *      This function creates makes a copy of the source datakind object,
 *      and creates a new datakind object for the given destination file.
 *      After the new object is created, it adds it to the destination
 *      file's HEAD Definitions index, and to the internal caches.
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
static omfErr_t MakeNewDatakind(
			 omfHdl_t srcFile,        /* IN - Source File Handle */
			 omfObject_t srcDatakind, /* IN - Source Datakind object */
			 omfHdl_t destFile,       /* IN - Destination File Handle */
			 omfObject_t *datakind)   /* OUT - Destination Datakind object */
{
  omfUniqueName_t datakindName;
  omfObject_t newDatakind, tmpDatakind;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(srcFile)
	{
	  CHECK(omfsReadUniqueName(srcFile, srcDatakind, 
								OMDDEFDatakindID, datakindName, 
								OMUNIQUENAME_SIZE));
	  if (!omfiDatakindLookup(destFile, datakindName, &tmpDatakind, &omfError))
		{
		  CHECK(omfiObjectCopyTreeExternal(srcFile, srcDatakind, 
							   kNoIncludeMedia, /* No includeMedia */
							   kNoFollowDepend, /* No resolveDependencies */
							   destFile, &newDatakind));
		  CHECK(omfiDatakindRegister(destFile, datakindName, newDatakind));
		  *datakind = newDatakind;
		}
	  else
		*datakind = tmpDatakind;
	}

  XEXCEPT
	{
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: ProcessDatakindExternal()
 *
 *      Given a source datakind, this function creates a destination datakind
 *      object for a destination file and registers the new information
 *      with the destination Bento container.
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
omfErr_t static ProcessDatakindExternal(
	omfHdl_t srcFile,        /* IN - Source File Handle */
    omfObject_t objRef,      /* IN - Source object */
    CMGlobalName propName,   /* IN - Property Name */
    omfHdl_t destFile,       /* IN - Destination File Handle */
    omfProperty_t *destProp, /* OUT - Destination Property */
    omfDDefObj_t *datakind)  /* OUT - Destination Datakind object */
{
  CMProperty newProp;
  omfDDefObj_t tmpDatakind;
  omfBool		found;

  XPROTECT(srcFile)
	{
	  *datakind = NULL;
	  *destProp = OMNoProperty;
	  /* Register property name with Bento */
	  newProp = CMRegisterProperty(destFile->container, propName);
	  *destProp = CvtPropertyFromBento(destFile, newProp, &found);

	  /* Register datakind with new file */
	  CHECK(MakeNewDatakind(srcFile, objRef, destFile, &tmpDatakind));
	  *datakind = tmpDatakind;
	}

  XEXCEPT
	{
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: ProcessEffectExternal()
 *
 *      Given a source effect definition, this function creates a
 *      destination effect defintion object for a destination file and
 *      registers the new information with the destination Bento container.
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
omfErr_t static ProcessEffectExternal(
    omfHdl_t srcFile,         /* IN - Source File Handle */
    omfObject_t objRef,       /* IN - Source Object */
    CMGlobalName propName,    /* IN - Property Name */
    omfHdl_t destFile,        /* IN - Destination File Handle */
    omfProperty_t *destProp,  /* OUT - Destination property */
    omfDDefObj_t *effectDef)  /* OUT - Destination Effect Definition */
{
  CMProperty newProp;
  omfUniqueName_t effectName;
  omfDDefObj_t defObject, tmpEffectDef;
  omfErr_t omfError;
  omfBool	found;
    
  XPROTECT(srcFile)
	{
	  *effectDef = NULL;
	  *destProp = OMNoProperty;

	  /* Register property name with Bento */
	  newProp = CMRegisterProperty(destFile->container, propName);
	  *destProp = CvtPropertyFromBento(destFile, newProp, &found);

	  CHECK(omfsReadUniqueName(srcFile, objRef, OMEDEFEffectID, effectName, 
							   OMUNIQUENAME_SIZE));
	  if (!omfiEffectDefLookup(destFile, effectName, &defObject, &omfError))
	  {
		CHECK(omfiObjectCopyTreeExternal(srcFile, objRef, 
								 kNoIncludeMedia, /* No includeMedia */
								 kNoFollowDepend, /* No resolveDependencies */
								 destFile, &tmpEffectDef));
			CHECK(omfiEffectDefRegister(destFile, effectName, tmpEffectDef));
			*effectDef = tmpEffectDef;
	  }
	  else
		*effectDef = defObject;
	} /* XPROTECT */

  XEXCEPT
	{
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiObjectCopyTreeExternal()
 *
 *      This function recursively copies all objects in the subtree rooted
 *      at the object identified by the 'obj' parameter into the
 *      specified destination file.  It is used by the copy and clone
 *      external functions.  All privatre data is copied.
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
omfErr_t omfiObjectCopyTreeExternal(
    omfHdl_t srcFile,      /* IN - Source File Handle */
	omfObject_t srcObj,    /* IN - Source object */
	omfIncMedia_t includeMedia,  /* IN - Whether to include media data */
    omfDepend_t resolveDependencies, /* IN - Whether to resolve mob dependencies */
	omfHdl_t destFile,     /* IN - Destination File Handle */
	omfObject_t *destObj)  /* OUT - Destination object */
{
    omfObject_t newObject, newMob;
	omfDDefObj_t datakind;
	omfClassID_t classID;
    omfIterHdl_t propIter = NULL;
	omfProperty_t srcProp, destProp;
	omfType_t srcType;
	CMProperty cprop, newProp;
	CMType ctype;
	CMGlobalName propTypeName, propName;
	mobTableEntry_t	*entry;
	omfUID_t sourceID, srcMobID, nullID = {0,0,0};
	omfInt32 loop, arrayLen;
	omfObject_t newRef, objRef;
	omfFileRev_t srcRev, destRev;
	omfBool	found;
    omfErr_t omfError = OM_ERR_NONE;
    	omfProperty_t	idProp;
	
	clearBentoErrors(srcFile);
	clearBentoErrors(destFile);
	*destObj = NULL;
	omfAssertValidFHdl(srcFile);

	omfAssertValidFHdl(destFile);
	omfAssert((destFile->openType != kOmOpenRead), destFile, 
					OM_ERR_WRONG_OPENMODE);
	omfAssert((srcObj != NULL), srcFile, OM_ERR_NULLOBJECT);

	XPROTECT(srcFile)
	  {
		if ((srcFile->setrev == kOmfRev1x) || (srcFile->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		CHECK(omfsFileGetRev(srcFile, &srcRev));
		CHECK(omfsFileGetRev(destFile, &destRev));
		if (srcRev != destRev)
		  {
			RAISE(OM_ERR_FILEREV_DIFF);
		  }

		/* Create the new destination object */
		CHECK(omfsReadClassID(srcFile, srcObj, idProp, classID));
		CHECK(omfsObjectNew(destFile, classID, &newObject));

		/* NOTE: The mobID has to be added to the hash table before
		 * copying the rest of the mob, to handle the resolveDependencies 
		 * recursive case.  If it isn't added first, recursing through mob 
		 * references will never detect that a mob already exists
		 * in the file, since the hash table entry wouldn't be there.
		 */
		if (omfsIsTypeOf(srcFile, srcObj, "MOBJ", &omfError))
		  {
			CHECK(omfsReadUID(srcFile, srcObj, OMMOBJMobID, &srcMobID));
			CHECK(AddMobTableEntry(destFile, newObject, srcMobID, kOmTableDupError));
			/* The MobID will be written to the destMob below */
		  }

		CHECK(omfiIteratorAlloc(srcFile, &propIter));

		/* Iterate over properties and copy. If ObjRef or ObjRefArray, 
		 * recurse 
		 */
		CHECK(omfiGetNextProperty(propIter, srcObj, &srcProp, &srcType));

		while ((omfError == OM_ERR_NONE) && srcProp)
		  {
			cprop = CvtPropertyToBento(srcFile, srcProp);
			ctype = CvtTypeToBento(srcFile, srcType, NULL);
			if (srcFile->BentoErrorRaised)
			  {
				RAISE(OM_ERR_BENTO_PROBLEM);
			  }
			if (CMCountValues((CMObject)srcObj, cprop, ctype))
			  {
				propTypeName = CMGetGlobalName((CMObject)ctype);
				propName = CMGetGlobalName((CMObject)cprop);

				/* If OMSCLPSourceID && resolveDependencies, copy the mob that
				 * the source clip references.
				 */
				if ((srcProp == OMSCLPSourceID) && resolveDependencies)
				  {
					CHECK(omfsReadUID(srcFile, srcObj, OMSCLPSourceID, 
									  &sourceID));
					/* Is the sourceRef null?  If not...
					 * Is the mobID in the src file's hash table?  (It should
					 * be if it is a valid OMF file.)  If so, is it in the
					 * destination file yet?  If not, clone it.  If so,
					 * don't do anything since it was already cloned.
					 */
					if (!equalUIDs(sourceID, nullID) &&
						omfsTableIncludesKey(srcFile->mobs, &sourceID) &&
						!omfsTableIncludesKey(destFile->mobs, &sourceID))
					  {
						 entry = (mobTableEntry_t *)omfsTableUIDLookupPtr(
											srcFile->mobs, sourceID);
						 if (entry == NULL)
						   {
							 RAISE(OM_ERR_MOB_NOT_FOUND);
						   }
						 CHECK(omfiMobCloneExternal(srcFile, entry->mob,
 						   (resolveDependencies?kFollowDepend:kNoFollowDepend),
								(includeMedia ? kIncludeMedia:kNoIncludeMedia),
							    destFile, &newMob));
							/* NOTE: The property is copied as usual below,
							 * so we don't need to copy/assign anything here.
							 */
					  }
				  } /* If OMSCLPSourceID && resolveDependencies */

				/* If Array of Object References */
				if (srcType == OMObjRefArray)
				  {
					arrayLen = omfsLengthObjRefArray(srcFile, srcObj, 
													 (omfProperty_t)srcProp);
					/* Iterate through ObjRefArray */
					for (loop = 1; loop <=arrayLen; loop++)
					  {
						omfError = omfsGetNthObjRefArray(srcFile, srcObj, 
													 srcProp, &objRef, loop);
						if (objRef)
						  {
							/* If this is a datakind object... */
							if (omfiIsADatakind(srcFile, objRef, &omfError))
							  {
								CHECK(ProcessDatakindExternal(srcFile, objRef,
									propName, destFile, &destProp, &datakind));
								if (datakind)
								  {
									CHECK(omfsAppendObjRefArray(destFile, 
											newObject, destProp, datakind));
								  }
							  }
							else if (omfiIsAnEffectDef(srcFile, objRef, 
													   &omfError))
							  {
								CHECK(ProcessEffectExternal(srcFile, objRef, 
									propName, destFile, &destProp, &newRef));
								if (newRef)
								  {
									CHECK(omfsAppendObjRefArray(destFile, 
										   newObject, destProp, newRef));
								  }
							  }
							else /* Not datakind or effectdef */
							  {
								CHECK(omfiObjectCopyTreeExternal(srcFile, 
														 objRef, 
														 includeMedia,
														 resolveDependencies,
														 destFile, &newRef));
								/* Retrieve new property from destFile (should
								 * have already been created by ObjectCopyX
								 */
								newProp = CMRegisterProperty(
										 destFile->container, propName);
								destProp = CvtPropertyFromBento(destFile,
															newProp, &found);
								if (newRef)
								  omfsAppendObjRefArray(destFile, newObject, 
														destProp, newRef);
							  }
						  } /* if objRef */
					  } /* for loop to iterate through ObjRefArray */
				  } /* If ObjRefArray */
				/* If Object Reference */
				else if (srcType == OMObjRef)
				  {
					CHECK(omfsReadObjRef(srcFile, srcObj, srcProp, &objRef));

					if (objRef)
					  {
						/* If this is a Datakind object */
						if (omfiIsADatakind(srcFile, objRef, &omfError))
						  {
							CHECK(ProcessDatakindExternal(srcFile, objRef,
								  propName, destFile, &destProp, &datakind));
							if (datakind)
							  {
								CHECK(omfsWriteObjRef(destFile, newObject,
													  destProp, datakind));
							  }
						  }
						else if (omfiIsAnEffectDef(srcFile, objRef, &omfError))
						  {
							CHECK(ProcessEffectExternal(srcFile, objRef, 
									propName, destFile, &destProp, &newRef));
							if (newRef)
							  {
								CHECK(omfsWriteObjRef(destFile, newObject, 
													 destProp, newRef));
							  }
						  }
						else /* Not a datakind or effectdef object, copy */
						  {
							CHECK(omfiObjectCopyTreeExternal(srcFile, objRef,
											  includeMedia,
											  resolveDependencies,
                                              destFile, &newRef));
							/* Retrieve new property from destFile (should
							 * have already been created by ObjectCopyX
							 */
							newProp = CMRegisterProperty(destFile->container,
													 propName);
							destProp = CvtPropertyFromBento(destFile, newProp,
															&found);
							if (newRef)
							  {
								CHECK(omfsWriteObjRef(destFile, newObject, 
													 destProp, newRef));
							  }
						  }
					  }
				  } /* If ObjRef */
				else /* If not ObjRef/ObjRefArray, copy property */
				  {
					/* This will take care of byte swapping if necessary */
					CHECK(omfiCopyPropertyExternal(srcFile, destFile, 
											   srcObj, newObject, 
											   srcProp, srcType, srcProp, srcType));
				  }
			  } /* if Count Values */
			CHECK(omfiGetNextProperty(propIter, srcObj, &srcProp, &srcType));
		  } /* while */

		if (omfError == OM_ERR_NO_MORE_OBJECTS)
		  omfError = OM_ERR_NONE;

	  } /* XPROTECT */

	XEXCEPT
	  {
		if (propIter)
		  omfiIteratorDispose(srcFile, propIter);
		if (XCODE() == OM_ERR_NO_MORE_OBJECTS)
		  {
			*destObj = newObject;
			return(OM_ERR_NONE);
		  }
		return(XCODE());
	  }
	XEND;

	*destObj = newObject;
	if (propIter)
	  omfiIteratorDispose(srcFile, propIter);
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiIsMobInFile()
 *
 *      This boolean function returns whether or not a mob with the
 *      given mobID is in the file. If it is, it returns a pointer to
 *      the mob.  The error code is returned as the last argument as 
 *      with other boolean functions.
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
omfBool omfiIsMobInFile(
	omfHdl_t file,       /* IN - File Handle */
	omfUID_t mobID,      /* IN - Mob ID */
	omfObject_t *mob,    /* OUT - Found mob */
	omfErr_t *omfError)  /* OUT - Error status */
{
	mobTableEntry_t	*entry;

	*mob = NULL;
	*omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE);

	/* Get the mob out of the mob hash table */
	entry = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, mobID);
	if (entry != NULL)
	  {
		*mob = entry->mob; 
		return(TRUE);
	  }

	return(FALSE);  /* Not found */
}

/*************************************************************************
 * Function: omfiFileGetNextDupMobs()
 *
 *      This function iterates through the file looking for duplicate
 *      mobs.  It returns when it finds a set of mobs with the same
 *      mob ID.  It returns the number of matches, and a list of
 *      object references that point to the duplicate mobs.  
 *
 *      An iterator handle must be created by calling omfiIteratorAlloc()
 *      before calling this function.  The iterator does not take 
 *      search criteria.  When all of the duplicate mobs have been
 *      iterated over, the function returns the error status 
 *      OM_ERR_NO_MORE_OBJECTS.  When the iteration is complete, the
 *      iterator should be destroyed by calling omfiIteratorDispose().
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *      mobList is allocated by this function and will need to be
 *      freed by the caller.  This is the only call in the Toolkit
 *      that allocates memory for the caller.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiFileGetNextDupMobs(
    omfIterHdl_t iterHdl,   /* IN - Iterator Handle */
	omfUID_t *mobID,        /* OUT - Mob ID of duplicate mobs */
    omfInt32 *numMatches,   /* OUT - Number of duplicate mobs with mobID */
	omfMobObj_t **mobList)   /* OUT - List of Object References to mobs */
{
    omfInt32 loop, dupCount = 0;
	omfBool foundEntry = FALSE, foundDup = FALSE;
	omfBool initialized = FALSE, firstTime = FALSE, more = FALSE;
	omTableIterate_t dupIter;
	mobTableEntry_t *mobEntry;
	omfUID_t *tmpMobID;

	*numMatches = 0;
	omfAssertIterHdl(iterHdl);
	omfAssertIterMore(iterHdl);

	XPROTECT(iterHdl->file)
	  {
		/* Initialize iterator if first time through */
		if (iterHdl->iterType == kIterNull)
		  {
			iterHdl->iterType = kIterDupMob;
			iterHdl->tableIter = 
			  (omTableIterate_t *)omOptMalloc(iterHdl->file, sizeof(omTableIterate_t));
			firstTime = TRUE;
		  }

		initialized = TRUE;
		if (firstTime)
		{
		  CHECK(omfsTableFirstEntryUnique(iterHdl->file->mobs, 
												 iterHdl->tableIter,
												 &foundEntry));
		}
		else
		{
		  CHECK(omfsTableNextEntry(iterHdl->tableIter,
												 &foundEntry));
		}
		
		/* While haven't found a duplicate yet, and there are more entries */
		while (!foundDup && foundEntry)
		  {
			dupCount = omfmTableNumEntriesMatching(iterHdl->file->mobs, 
												   iterHdl->tableIter->key);
			/* Were there any duplicates for this MobID entry? */
			if (dupCount > 1)
			  {
				/* Malloc NOT using a file handle, because omfsFree() doean't take a file
				 * handle
				 */
				*mobList = 
				  (omfObject_t *)omOptMalloc(NULL, dupCount * (sizeof(omfMobObj_t)));
				/* Loop to get all matching entries */
				loop = 0;
				CHECK(omfsTableFirstEntryMatching(iterHdl->file->mobs, 
								   &dupIter, iterHdl->tableIter->key, &more));
				while (more)
				  {
					mobEntry = (mobTableEntry_t *)dupIter.valuePtr;
					(*mobList)[loop] = (omfObject_t)mobEntry->mob;
					loop++;
					CHECK(omfsTableNextEntry(&dupIter, &more));
				  }
				*numMatches = dupCount;
				tmpMobID = (omfUID_t *)iterHdl->tableIter->key;
				copyUIDs((*mobID), (*tmpMobID)); 

				foundDup = TRUE;
			  } /* If dupCount > 1 */
			if (!foundDup)
			{
			  CHECK(omfsTableNextEntry(iterHdl->tableIter, &foundEntry));
			}
		  } /* while (found) */

		/* If no more entries were found in the table, finished iterating */
		if (!foundEntry)
		  {
			RAISE(OM_ERR_NO_MORE_OBJECTS);
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (!initialized)
		  omfiIteratorClear(iterHdl->file, iterHdl);
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: IsThisSCLP()
 *
 *      This function is the Match callback used by omfiMobChangeRef().
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
static omfBool IsThisSCLP(
						  omfHdl_t file,    /* IN - File Handle */
						  omfObject_t obj,  /* IN - Object to match */
						  void *data)       /* IN/OUT - Match Data */
{
  omfSourceRef_t *matchRef = (omfSourceRef_t *)data;
  omfSourceRef_t sourceRef;
  omfErr_t omfError = OM_ERR_NONE;

  if (omfiIsASourceClip(file, obj, &omfError))
    {
      omfError = SourceClipGetRef(file, obj, &sourceRef);
      if (equalUIDs(sourceRef.sourceID, (*matchRef).sourceID))
		return(TRUE);
      else 
		return(FALSE);
    }
  else
    return(FALSE);
}

/*************************************************************************
 * Private Function: ChangeRef()
 *
 *      This function is the Execute callback used by omfiMobChangeRef().
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
static omfErr_t ChangeRef(
						  omfHdl_t file,    /* IN - File Handle */
						  omfObject_t obj,  /* IN - Object to execute */
						  omfInt32 level,   /* IN - Depth level */
						  void *data)       /* IN/OUT - Execute data */
{
  omfUID_t *newMobID = (omfUID_t *)data;
  omfSourceRef_t sourceRef;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(file)
	{
	  if (omfiIsASourceClip(file, obj, &omfError))
		{
		  if (omfError == OM_ERR_NONE)
			{
			  CHECK(omfiSourceClipGetInfo(file, obj, NULL, NULL, &sourceRef));
			  copyUIDs(sourceRef.sourceID, (*newMobID));
			  CHECK(omfiSourceClipSetRef(file, obj, sourceRef));
			}
		  else
			{
			  RAISE(omfError);
			}
		}
    } /* XPROTECT */

  XEXCEPT
	{
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobChangeRef()
 *
 *      This function traverses through the entire structure of the
 *      input mob looking for source clips, and changes the sourceID
 *      property on all source clips with oldMobID to newMobID.  It
 *      uses the omfiMobMatchAndExecute() function to traverse
 *      the mob structure.
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
omfErr_t omfiMobChangeRef(
    omfHdl_t file,        /* IN - File Handle */
	omfMobObj_t mob,      /* IN - Mob to traverse */
	omfUID_t oldMobID,    /* IN - Old Mob ID reference in source clip */
	omfUID_t newMobID)    /* IN - New Mob ID reference in source clip */
{
    omfInt32 matches;

	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiMobMatchAndExecute(file, mob, 0, /* level */
		      IsThisSCLP, &oldMobID,
		      ChangeRef, &newMobID, 
		      &matches));
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
