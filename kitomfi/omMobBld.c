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

/* Name: omMobBld.c
 *
 * Overall Function: The API for building compositions.
 *
 * Audience: Clients writing OMFI compositions.
 *
 * Public Functions:
 *     MobID Creation: omfiMobIDNew()
 *     Mob Root Creation: omfiCompMobNew(), omfiMobSetModTime(), 
 *              omfiMobSetPrimary(), omfiMobSetIdentity(), omfiMobSetName()
 *              omfiMobAppendNewSlot(), omfiMobAppendNewTrack()
 *     Clip Creation: omfiSourceClipNew(), omfiSourceClipSetRef(), 
 *              omfiSourceClipSetFade(), omfiTimecodeNew(),
 *              omfiEdgecodeNew(), omfiFillerNew()
 *     Const/Vary Value Creation: omfiConstValueNew(), omfiVaryValueNew(),
 *              omfiVaryValueAddPoint()
 *     Sequence Creation: omfiSequenceNew(), omfiSequenceAppendCpnt()
 *     Scope Creation: omfiNestedScopeNew(), omfiNestedScopeAppendSlot(), 
 *              omfiScopeRefNew()
 *     Selector Creation: omfiSelectorNew(), omfiSelectorSetSelected(),
 *              omfiSelectorAddAlt()
 *     Transition Creation: omfiTransitionNew()
 *     Effect Creation: omfiEffectDefNew(), omfiEffectAddNewSlot(),
 *              omfiEffectNew()
 *              omfiEffectSetFinalRender(), omfiEffectSetWorkingRender()
 *              omfiEffectSetBypassOverride(), omfiEffectDefLookup()
 *     Media Group Creation: omfiMediaGroupNew(), omfiMediaGroupSetStillFrame()
 *              omfiMediaGroupAddChoice()
 *     Definition Creation: omfiDatakindNew(), omfiDatakindLookup()
 *
 *
 * General error codes returned:
 *      OM_ERR_INVALID_OBJ
 *              An object of the wrong class was passed in
 *      OM_ERR_DUPLICATE_MOBID
 *              MobID already exists in the file
 *      OM_ERR_INVALID_MOBTYPE
 *              This operation does not apply to this mob type
 *      OM_ERR_NULLOBJECT
 *              An invalid null object was passed in as a parameter
 *      OM_ERR_INVALID_DATAKIND
 *              An invalid datakind was passed in
 *      OM_ERR_INVALID_EFFECTDEF
 *              An invalid effectdef was passed in
 *      OM_ERR_INVALID_TRAN_EFFECT
 *              The effect is not a transition effect
 *      OM_ERR_ADJACENT_TRAN
 *              Adjacent transitions in a sequence are illegal
 *      OM_ERR_LEADING_TRAN
 *              Beginning a sequence with a transition is illegal
 *      OM_ERR_INSUFF_TRAN_MATERIAL
 *              Not enough material as input to transition
 *      OM_ERR_BAD_SLOTLENGTH
 *              The length of the slot is not valid
 *      OM_ERR_EFFECTDEF_EXIST
 *              An effectdef with this ID already exists
 *      OM_ERR_DATAKIND_EXIST
 *              A datakind with this ID already exists
 *      OM_ERR_NOT_SOURCE_CLIP
 *              The property must be a source clip.
 */

#include "masterhd.h"
#if PORT_SYS_MAC
#if !LW_MAC
#include <CarbonCore/OSUtils.h>
#endif
#include <HIToolbox/Events.h>
#elif PORT_INC_NEEDS_TIMEH
#include <time.h>
#define HZ CLK_TCK
#endif

/*
 * Doug Cooper - 04-04-96
 * Added proper includes for NeXTStep to define HZ:
 */
#ifdef NEXT
#include <architecture/ARCH_INCLUDE.h>
#import ARCH_INCLUDE(bsd/, param.h)
#endif

#if defined(PORTKEY_OS_UNIX) || defined(PORTKEY_OS_ULTRIX)
#if PORT_INC_NEEDS_SYSTIME
#include <sys/time.h>
#include <sys/times.h>
#endif
#include <sys/param.h>
#endif

#ifdef sun
#include <sys/resource.h>
#endif

#include <string.h>

#include "omPublic.h"
#include "omPvt.h" 


/*******************************/
/* Static Function Definitions */
/*******************************/
static omfErr_t InsertPoint(omfHdl_t file,
				     omfSegObj_t varyValue,
					 omfRational_t time,
					 omfCntlPtObj_t point);

static omfErr_t MobSlotSetTrackDesc(
	omfHdl_t file,
	omfMSlotObj_t slot,
	omfPosition_t origin,
	omfTrackID_t trackID,
	omfString trackName,
	omfTDescObj_t *trackDesc);

/* Callback functions used by omfiMobAppendNewSlot() to recursively attach 
 * the CPNT:Editrate property to 1.x components 
*/
/*******************************/
/* Functions                   */
/*******************************/
/*************************************************************************
 * Function: omfiMobIDNew()
 *
 *      This function can be used to create a new mob ID.  The mob ID
 *      consists of the company specific prefix specified when 
 *      omfsBeginSession() is called.  The major number is the time of day,
 *      and the minor number is the accumulated cpu cycles of the
 *      application.
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
omfErr_t omfiMobIDNew(
        omfHdl_t file,       /* IN - File Handle */
        omfUID_t *mobID)     /* OUT - Newly created Mob ID */
{
	static omfUInt32 last_part2 = 0;
#ifdef sun
	struct rusage rusage_struct;
	int status;
#endif
	omfTimeStamp_t	timestamp;
	
	omfAssertValidFHdl(file);

	mobID->prefix = file->session->prefix;
	/* Code Resources - UNIX time() uses statically initialized strings, 
	 * so the UNIX call can't be used with Code Resources on the Mac 
	 */

	omfsGetDateTime(&timestamp);
	mobID->major = timestamp.TimeVal;
#if PORT_SYS_MAC
	mobID->minor = TickCount();
#else
#if PORT_INC_NEEDS_TIMEH
	mobID->minor = ((unsigned int )(time(NULL)*60/CLK_TCK));

#else 
#if defined(sun)
	status = getrusage(RUSAGE_SELF, &rusage_struct);

	/* On the Sun, add system and user time */
	mobID->minor = rusage_struct.ru_utime.tv_sec*60 + 
	      rusage_struct.ru_utime.tv_usec*60/1000000 +
		  rusage_struct.ru_stime.tv_sec*60 +
		  rusage_struct.ru_stime.tv_usec*60/1000000;
#else
	{
	  static struct tms timebuf;
	  mobID->minor = ((unsigned int )(times(&timebuf)*60/HZ));
	}	
#endif
#endif
#endif

#if OMF_ID_SIG_OFFSET
	mobID->minor += file->session->ident->idGenerationSig;
#endif
	
	if (last_part2 >= mobID->minor)
	  mobID->minor = last_part2 + 1;
		
	last_part2 = mobID->minor;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiCompMobNew()
 *
 *      This function creates a new composition mob, sets it base
 *      properties, adds the mob to the hash table in the file handle,
 *      adds the mob to the HEAD mob index(s), and adds the mob to the
 *      HEAD primary mob index if requested.  The Creation and Modification
 *      times will automatically be set to the time of day.  A mobID
 *      will be created for the mob.  To change the mobID, call
 *      the function omfiMobSetIdentity().  This function calls 
 *      MobSetNewProps() to create most of the properties.  To change
 *      the modification time, call omfiMobSetModTime().
 *
 *      This function will create 1.x and 2.x composition mobs.  For
 *      a 1.x file, it places the mob in the HEAD Composition Mob
 *      index, and ignores the primary mob request.
 *
 * Argument Notes:
 *      The name argument is optional.  Specify a NULL if no name is
 *      requested.  The created mob will be passed back as the final argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiCompMobNew(
	omfHdl_t file,       /* IN - File Handle */
	omfString name,      /* IN - Mob name (optional) */
	omfBool	isPrimary,   /* IN - Whether or not the mob is a primary mob */
	omfMobObj_t *compMob)/* OUT - Created composition mob */
{
	omfObject_t tmpMob = NULL;
	omfObject_t head = NULL;
	omfUID_t mobID;
	omfBool primary = FALSE, hashed = FALSE;
	omfInt32 numMobs;

	*compMob = NULL;
	omfAssertValidFHdl(file);

	XPROTECT(file)
	  {
		CHECK(omfsGetHeadObject(file, &head));

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(omfsObjectNew(file, "MOBJ", &tmpMob));
			/* Set base props, add mobID to hash table, add to mobIndex */
			CHECK(MobSetNewProps(file, tmpMob, FALSE, /* Not a MasterMob */
								 name, FALSE /* Not a Primary Mob */));
			hashed = TRUE;

			/* Append new comp mob to ObjectSpine */
			CHECK(omfsAppendObjRefArray(file, head, OMObjectSpine, tmpMob));
		  }
		else /* kOmfRev2x */
		  {
			CHECK(omfsObjectNew(file, "CMOB", &tmpMob));
			/* Set base props, add to PrimaryMob index, add to hash table */
			CHECK(MobSetNewProps(file, tmpMob, FALSE, /* Not a MasterMob */
								 name, isPrimary));
			primary = TRUE;
			hashed = TRUE;

			/* Append new composition mob to HEAD mob index */
			CHECK(omfiMobGetMobID(file, tmpMob, &mobID));
			CHECK(omfsAppendObjRefArray(file, head, OMHEADMobs, tmpMob));
		  }
	  }

	XEXCEPT
	{
	    omfsObjectDelete(file, tmpMob);
		if (hashed)
		  omfsTableRemove(file->mobs, &mobID);
		if (primary)
		  {
			omfsGetHeadObject(file, &head);
			numMobs = omfsLengthObjRefArray(file, head, OMHEADPrimaryMobs);
			if (numMobs > 0)
			  omfsRemoveNthObjRefArray(file, head, OMHEADPrimaryMobs, numMobs);
		  }
		/* Assume removal from OMHEADMobs index on failure */

		return(XCODE());
	}
	XEND;

    *compMob = tmpMob;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: MobSetNewProps()
 *
 *      This function is called by omfiCompMobNew, omfmMasterMobNew, etc.
 *      to create the new properties on a mob.  It sets the name to the 
 *      requested name, and adds the mob to the HEAD primary mob index if 
 *      requested.  This function also sets the creation and modification
 *      times to the time of day.  A mobID is created for the mob.
 *      This function also puts the mob in the appropriate mob index
 *      and into the file handle's mob hash table.
 *
 *      This function will create 1.x and 2.x composition mobs.  For the
 *      1.x file, it places the mob in the HEAD Composition Mob index, and
 *      igmores the primary mob request.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t MobSetNewProps(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - New Mob */
	omfBool isMasterMob, /* IN - Whether or not this is a Master Mob */
	omfString name,      /* IN - Mob Name (optional) */
	omfBool isPrimary)   /* IN - Whether or not this is a primary mob */
{
	omfTimeStamp_t	create_timestamp;
	omfUID_t ID;
	omfBool appended = FALSE, hashed = FALSE;

	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_INVALID_OBJ);

	XPROTECT(file)
	  {
		/* Create new Mob ID */
		CHECK(omfiMobIDNew(file, &ID));

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			/* First write Component properties */
			if (name)
			  CHECK(omfsWriteString(file, mob, OMCPNTName, name));

			CHECK(omfsWriteTrackType(file, mob, OMCPNTTrackKind, TT_NULL));

			/* Initialize TrackGroup length */
			/* Note: only write if it isn't already present.  The Copy
			 * and clone mob calls also use this function.
			 */
			if (!omfsIsPropPresent(file, mob, OMTRKGGroupLength, OMInt32))
			  {
				CHECK(omfsWriteInt32(file, mob, OMTRKGGroupLength, 0));
			  }

			/* Create timestamp from current time, and set lastModified
			 */
			omfsGetDateTime(&create_timestamp);
			
			CHECK(omfsWriteTimeStamp(file, mob, OMMOBJLastModified,
									 create_timestamp));

			/* Set Usage Code.  All mobs have UC_NULL, except Master Mobs
			 * which have UC_MASTERMOB.
			 */
			if (isMasterMob == FALSE)
			  {
				CHECK(omfsWriteUsageCodeType(file, mob, OMMOBJUsageCode, 
											 UC_NULL));
			  }
			else
			  {
				CHECK(omfsWriteUsageCodeType(file, mob, OMMOBJUsageCode, 
											 UC_MASTERMOB));
			  }

			/* No PhysicalMedia property since it is a composition mob */
		  }

		else /* kOmfRev2x */
		  {
			/* Name is optional */
			if (name)
			  CHECK(omfsWriteString(file, mob, OMMOBJName, name));

			/* Create timestamp from current time, and set lastModified and
			 * creationTime for mob 
			 */
			omfsGetDateTime(&create_timestamp);
			
			CHECK(omfsWriteTimeStamp(file, mob, OMMOBJLastModified,
									 create_timestamp));
			CHECK(omfsWriteTimeStamp(file, mob, OMMOBJCreationTime,
									 create_timestamp));

			if (isPrimary)
			  {
				CHECK(omfiMobSetPrimary(file, mob, isPrimary));
			  }
		  } /* kOmfRev2x */

		/* Set the identity of the Mob, whether 1.x or 2.x */
		CHECK(omfiMobSetIdentity(file, mob, ID));

	  } /* XPROTECT */

	XEXCEPT
	  {
		/* Assume removal from OMHEADPrimaryMobs index on failure */
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobSetModTime()
 *
 *      This function sets the modification time on a mob.  The
 *      modification time is initially set to the time that the mob
 *      was created.  The Toolkit does not maintain the modification
 *      time every time that a mob has been updated.  Therefore, this
 *      function should be called explicitly to change the modification
 *      time.
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
omfErr_t omfiMobSetModTime(	
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t	mob,     /* IN - Mob object */
	omfTimeStamp_t modTime) /* IN - New Modification Time */
{
	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_INVALID_OBJ);

	XPROTECT(file)
	  {
		CHECK(omfsWriteTimeStamp(file, mob, OMMOBJLastModified, modTime));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobSetPrimary()
 *
 *      This function is used to either add a mob to the HEAD Primary Mob
 *      index or to take it out of the index.
 *
 *      This function can be called on either 1.x or 2.x files, but the 
 *      result for 1.x files will be to do nothing.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobSetPrimary(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob object */
	omfBool	isPrimary)   /* IN - Whether or not the mob is a primary mob */
{
	omfObject_t head = NULL;
	omfInt32 loop, numMobs;
	omfUID_t mobID, tmpMobID;
	omfMobObj_t tmpMob = NULL;
	omfBool found = FALSE;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_INVALID_OBJ);

	XPROTECT(file)
	  {
		/* For 1.x, don't do anything.  Just return OM_ERR_NONE */
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  return(OM_ERR_NONE);

		CHECK(omfsGetHeadObject(file, &head));

		/* If primary mob, add to "Primary Mob" index */
		if (isPrimary)
		  {
			/* If it isn't already in the index */
			if (!omfiIsAPrimaryMob(file, mob, &omfError))
			  {
				CHECK(omfsAppendObjRefArray(file, head, OMHEADPrimaryMobs, 
											mob));
			  }
		  }

		else /* isPrimary == FALSE */
		  {
			CHECK(omfsReadUID(file, mob, OMMOBJMobID, &mobID));
			numMobs = omfsLengthObjRefArray(file, head, OMHEADPrimaryMobs);
			for (loop = 1; loop <= numMobs; loop++)
			  {
				CHECK(omfsGetNthObjRefArray(file, head, OMHEADPrimaryMobs,
											&tmpMob, loop));
				CHECK(omfsReadUID(file, tmpMob, OMMOBJMobID, &tmpMobID));
				if (equalUIDs(mobID, tmpMobID))
				  {
					CHECK(omfsRemoveNthObjRefArray(file, head, 
												 OMHEADPrimaryMobs, loop));
					found = TRUE;
				  }
				/* Release Bento reference, so the useCount is decremented */
				CMReleaseObject((CMObject)tmpMob);
				if (found)
				  break;
			  }
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
 		/* Assume removal from OMHEADPrimaryMobs index on append failure,
		 * and corrupt index on remove failure.
		 */
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobSetIdentity()
 *
 *      When a mob is initially created, the Toolkit internally creates 
 *      a mobID for the new mob.  This function should be used to change
 *      the mob's identity to an explicit mobID.
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
omfErr_t omfiMobSetIdentity(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob Object */
	omfUID_t newMobID)   /* IN - New Mob ID */
{
    omfUID_t oldMobID;
    omfBool hasMobID = FALSE;
	omfObject_t head = NULL, foundMob = NULL;
	omfInt32 index = 0;
	omfProperty_t prop = OMNoProperty;
	omfObjIndexElement_t elem;
	omfErr_t omfError = OM_ERR_NONE;
	mobTableEntry_t *mobPtr, *oldMobPtr;
	
	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_INVALID_OBJ);

	XPROTECT(file)
	  {
		/* Remember the old mob ID so it can be removed from the hash table */
		hasMobID = omfsIsPropPresent(file, mob, OMMOBJMobID, OMUID);
		if(hasMobID)
		{
			CHECK(omfiMobGetMobID(file, mob, &oldMobID));
			oldMobPtr = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, oldMobID);
		}
		else
			oldMobPtr = NULL;

		/* Does a mob with the new ID already exist?  If so, return error */
		mobPtr = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, newMobID);
		if(mobPtr != NULL)
		  {
			if(mobPtr->mob == NULL)
				{
				CHECK(omfsWriteUID(file, mob, OMMOBJMobID, newMobID));
				mobPtr->mob = mob;
				}
			else
			{
				RAISE(OM_ERR_DUPLICATE_MOBID);
			}
		  }
		else
		{
			CHECK(omfsWriteUID(file, mob, OMMOBJMobID, newMobID));

			/* Remove the hash table entry for the old mobID, and add new one */
			CHECK(AddMobTableEntry(file, mob, newMobID, kOmTableDupError));
			mobPtr = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, newMobID);
		}
		
		/* Remove it last, so the old hash entry is still there on error */

		/* For 1.x, append mob to HEAD mob index */
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			if(oldMobPtr != NULL)
			{
				mobPtr->map1x = oldMobPtr->map1x;
				oldMobPtr->map1x = NULL;
			}

			CHECK(omfsGetHeadObject(file, &head));
			if (omfiIsASourceMob(file, mob, &omfError))
			  prop = OMSourceMobs;
			else if (omfiIsACompositionMob(file, mob, &omfError) ||
					 omfiIsAMasterMob(file, mob, &omfError))
			  prop = OMCompositionMobs;
			else /* The mob must be a source or composition at this point */
			  {
				RAISE(OM_ERR_INVALID_MOBTYPE);
			  }
			elem.Mob = mob;
			copyUIDs(elem.ID, newMobID);
			CHECK(omfsAppendObjIndex(file, head, prop, &elem));
		  }

		/* If there was a previous MobID, delete from hash table.  Also,
		 * delete from the mob index if it is a 1.x file.
		 */
		if (hasMobID)
		  {
			CHECK(omfsTableRemoveUID(file->mobs, oldMobID));

			/* If 1.x, take the old Mob out of the mob index, and add
			 * a new entry with a new mob ID
			 */
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				omfError = FindMobInIndex(file, oldMobID, prop, &index, 
										  &foundMob);
				if (foundMob)
				  {
					CHECK(omfsRemoveNthObjIndex(file, head, prop, index));
				  }
			  } /* kOmfRev1x or kOmfRevIMA */
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
 * Function: omfiMobSetName()
 *
 *      This function sets the name property on a mob.
 *
 *      This function supports both 1.x and 2.x files.  For 1.x files,
 *      the CPNTName property is set.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobSetName(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob Object */
	omfString name)      /* IN - Mob Name */
{
	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_INVALID_OBJ);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(omfsWriteString(file, mob, OMCPNTName, name));
		  }
		else
		  {
			CHECK(omfsWriteString(file, mob, OMMOBJName, name));
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
 * Private Function: isObjFunc() and set1xEditrate()
 *
 *      These are callback functions used by omfiMobAppendNewSlot()
 *      to recursively attach the CPNT:Editrate property to 1.x
 *      components.  The callback functions are input to the
 *      omfiMobMatchAndExecute() function which traverses a
 *      tree of objects depth first and executes the callbacks.
 *      They will only be called on 1.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfBool isObjFunc(omfHdl_t file,       /* IN - File Handle */
				  omfObject_t obj,     /* IN - Object to match */
				  void *data)          /* IN/OUT - Match Data */
{
  /* Match all objects in the subtree */
  return(TRUE);
}

omfErr_t set1xEditrate(omfHdl_t file,       /* IN - File Handle */
					   omfObject_t obj,       /* IN - Object to execute */
					   omfInt32 level,        /* IN - Level of the tree */
					   void *data)            /* IN/OUT - Execute Data */
{
  omfRational_t *editrate = (omfRational_t *)data;
  omfClassID_t classID;
  omfErr_t omfError = OM_ERR_NONE;
	omfProperty_t	idProp;
	
	if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		idProp = OMObjID;
	else
		idProp = OMOOBJObjClass;

  /* Write 1.x editrate property if it is not a TRAK object */
  omfError = omfsReadClassID(file, obj, idProp, classID);
  if (!omfsIsPropPresent(file, obj, OMCPNTEditRate, OMExactEditRate))
	{
	  if (!streq(classID, "TRAK") && (omfError == OM_ERR_NONE))
		{
/* NOTE: This function should change the editrate to WARP's inputEditRate
 *       for objects below a WARP.  It needs to take level into account,
 *       so we know when to go back to the old editrate.
 */
#if 0
		  if (omfsIsTypeOf(file, obj, "WARP", &omfError))
			{
			  /* If it is a WARP,change the editrate to WARP's inputEditRate */
			  if (omfsIsPropPresent(file, obj, OMWARPEditRate, 
									 OMExactEditRate))
				{
				  omfError = omfsReadExactEditRate(file, obj, OMWARPEditRate,
												   editRate);
				}
			}
#endif
		  omfError = omfsWriteExactEditRate(file, obj, OMCPNTEditRate, 
											*editrate);
		}
	}

  return(omfError);
}


/*************************************************************************
 * Function: omfiMobAppendNewSlot()
 *
 *      This function creates a new mob slot with the given property
 *      values, and appends it to the input mob.  Since the Toolkit
 *      attempts to enforce bottom up creation of mobs, the slot segment
 *      must be passed in as an argument.
 *
 *      This function will support both 1.x and 2.x files.  For 1.x
 *      files, a TRAK object without a label will be created. 
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobAppendNewSlot(
	omfHdl_t file,             /* IN - File Handle */
	omfMobObj_t mob,           /* IN - Input Mob */
	omfRational_t editRate,    /* IN - Edit rate property value */
	omfSegObj_t segment,       /* IN - Segment to append as slot component */
	omfMSlotObj_t *newSlot)    /* OUT - Newly created slot */
{
	omfObject_t tmpSlot = NULL;
	omfInt32 matches, numSlots;
	omfInt32	tmp1xLen;
	omfLength_t length = omfsCvtInt32toLength(0, length);
	omfLength_t	mobLength = omfsCvtInt32toLength(0, mobLength);
	omfErr_t omfError = OM_ERR_NONE;

	*newSlot = NULL;
	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((segment != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(omfsObjectNew(file, "TRAK", &tmpSlot));

			CHECK(omfsWriteObjRef(file, tmpSlot, OMTRAKTrackComponent, 
								  segment));

			/* If this is the first slot/track, set the editrate
			 * on the mob.  NOTE: it will not verify that all editrates
			 * are the same for all tracks in this release.
			 */
			CHECK(omfiMobGetNumSlots(file, mob, &numSlots));
			if (numSlots == 0)
			  {
				CHECK(omfsWriteExactEditRate(file, mob, OMCPNTEditRate,
											 editRate)); 
			  }

			/* Check to see if the length of the trackcomponent is
			 * greater than the length of the mob.  If so, set the
			 * new mob length to the component length.
			 */
			CHECK(omfiComponentGetLength(file, segment, &length));
			omfError = omfiComponentGetLength(file, mob, &mobLength);
			if (omfsInt64Greater(length, mobLength))
			  {
				CHECK(omfsTruncInt64toInt32(length, &tmp1xLen));	/* OK 1.x */
				CHECK(omfsWriteInt32(file, mob, OMTRKGGroupLength,
									tmp1xLen));
			  }

			/* Write editrate to component, and recursively add to 
			 * rest of component tree.
			 */
			CHECK(omfiMobMatchAndExecute(file, segment, 0, /* level */
											 isObjFunc, NULL,
											 set1xEditrate, &editRate,
											 &matches));

			/* Append new track to mob */
			CHECK(omfsAppendObjRefArray(file, mob, OMTRKGTracks, tmpSlot));
		  }
		else /* xOmfRev2x */
		  {
			CHECK(omfsObjectNew(file, "MSLT", &tmpSlot));
			CHECK(omfsWriteRational(file, tmpSlot, OMMSLTEditRate, editRate));
			CHECK(omfsWriteObjRef(file, tmpSlot, OMMSLTSegment, segment));
		
			/* Append new slot to mob */
			CHECK(omfsAppendObjRefArray(file, mob, OMMOBJSlots, tmpSlot));
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
	    omfsObjectDelete(file, tmpSlot);
		return(XCODE());
	  }
	XEND;

	*newSlot = tmpSlot;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: MobSlotSetTrackDesc()
 *
 *      This function is called to create a track descriptor object
 *      and append it to a mob slot.  A track descriptor gives the mob
 *      slot identity outside of the mob.
 *
 *      This internal function will only be called on 2.x files.
 *
 * Argument Notes:
 *      The track name is optional.  The newly created track descriptor
 *      object is returned as the last argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t MobSlotSetTrackDesc(
	omfHdl_t file,         /* IN - File Handle */
	omfMSlotObj_t slot,    /* IN - Input Mob Slot */
	omfPosition_t origin,  /* IN - Track Origin */
	omfTrackID_t trackID,  /* IN - Track ID */
	omfString trackName,   /* IN - Track Name (optional) */
	omfTDescObj_t *trackDesc) /* OUT - Newly created track descriptor */
{
	omfObject_t tmpDesc = NULL;
	omfBool trackDescExists = FALSE;
	omfErr_t omfError = OM_ERR_NONE;

	*trackDesc = NULL;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((slot != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* See if trackDesc already exists.  If so, simply set new
		 * properties.  If not, create new object 
		 */
		omfError = omfsReadObjRef(file, slot, OMMSLTTrackDesc, &tmpDesc);
		if (!tmpDesc)
		  {
			CHECK(omfsObjectNew(file, "TRKD", &tmpDesc));
			trackDescExists = TRUE;
		  }
		else if (omfError != OM_ERR_NONE)
		  {
			RAISE(omfError);
		  }

		CHECK(omfsWritePosition(file, tmpDesc, OMTRKDOrigin, origin));
		CHECK(omfsWriteUInt32(file, tmpDesc, OMTRKDTrackID, trackID));
		/* Name is optional, only write if specified */
		if (trackName)
		  {
			CHECK(omfsWriteString(file, tmpDesc, OMTRKDTrackName, trackName));
		  }

		/* Append new object */
		if (trackDescExists)
		  {
			CHECK(omfsWriteObjRef(file, slot, OMMSLTTrackDesc, tmpDesc));
		  }

	  } /* XPROTECT */

	XEXCEPT
	  {
	    omfsObjectDelete(file, tmpDesc);
		return(XCODE());
	  }
	XEND;

	*trackDesc = tmpDesc;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobAppendNewTrack()
 *
 *      This function creates a new mob slot with the given property
 *      values, and appends it to the input mob.  For 2.x, a mob slot
 *      becomes a track when a track descriptor object is appended to
 *      give it identity outside of the mob.  This function creates the
 *      track descriptor with the given property values.  It attempts to
 *      abstract away the construction of a "track" object.  Since the 
 *      Toolkit attempts to enforce bottom up creation of mobs, the slot
 *      segment must be passed in as an argument.
 *
 *      For 1.x files, this function will create a TRAK object with
 *      the given property values.
 *
 *
 * Argument Notes:
 *      trackName is optional.  The newly created track will be returned
 *      as the last argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobAppendNewTrack(
	omfHdl_t file,           /* IN - File Handle */
	omfMobObj_t mob,         /* IN - Input Mob Object */
	omfRational_t editRate,  /* IN - Edit rate property value */
	omfSegObj_t segment,     /* IN - Segment to append as track component */
	omfPosition_t origin,    /* IN - Track Origin */
	omfTrackID_t trackID,    /* IN - Track ID */
	omfString trackName,     /* IN - Track Name (optional) */
	omfMSlotObj_t *newTrack) /* OUT - Newly created track */
{
	omfTDescObj_t trackDesc = NULL;
	omfObject_t tmpTrack = NULL;
	omfInt32 numSlots, matches;
	omfInt32	tmp1xStartPosition;
	omfInt32	tmp1xLength;
	omfLength_t length = omfsCvtInt32toLength(0, length);
	omfLength_t mobLength = omfsCvtInt32toLength(0, mobLength);
	omfBool hasDesc = FALSE;
	omfUID_t mobID;
	omfInt16 trackID1x;
	omfDDefObj_t datakind = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	*newTrack = NULL;
	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((segment != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* NOTE: write unique labels, return error if not unique */
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(omfsObjectNew(file, "TRAK", &tmpTrack));

			CHECK(omfiMobGetMobID(file, mob, &mobID));
			CHECK(omfiComponentGetInfo(file, segment, &datakind, NULL));
			CHECK(AddTrackMapping(file, mobID, trackID, datakind));
			CHECK(CvtTrackIDtoNum(file, mobID, trackID, &trackID1x));
			CHECK(omfsWriteInt16(file, tmpTrack, OMTRAKLabelNumber,
								 trackID1x));
			CHECK(omfsWriteObjRef(file, tmpTrack, OMTRAKTrackComponent, 
								  segment));
			/* Origin and TrackName are not 1.x properties */

			/* NOTE: as add tracks, should make sure that start positions
			 * are same after adjusting for editrate differences
			 */

			/* If this is the first slot/track, set the editrate
			 * on the mob.  NOTE: it will not verify that all editrates
			 * are the same for all tracks in this release.
			 */
			CHECK(omfiMobGetNumSlots(file, mob, &numSlots));
			if (numSlots == 0)
			  {
				CHECK(omfsWriteExactEditRate(file, mob, OMCPNTEditRate,
											 editRate)); 
			  }

			/* There will always be at least one track on a mob.  When
			 * the first track (not slot) is added, take the StartPosition
			 * from the origin passed in.
			 */
			if (!omfsIsPropPresent(file, mob, OMMOBJStartPosition, OMInt32))
			  {
				CHECK(omfsTruncInt64toInt32(origin, &tmp1xStartPosition));	/* OK 1.x */
				CHECK(omfsWriteInt32(file, mob, OMMOBJStartPosition, 
									tmp1xStartPosition));
			  }

			/* Check to see if the length of the trackcomponent is
			 * greater than the length of the mob.  If so, set the
			 * new mob length to the component length.
			 */
			CHECK(omfiComponentGetLength(file, segment, &length));
			omfError = omfiComponentGetLength(file, mob, &mobLength);
			if (omfsInt64Greater(length, mobLength))
			  {
				CHECK(omfsTruncInt64toInt32(length, &tmp1xLength));	/* OK 1.x */
				CHECK(omfsWriteInt32(file, mob, OMTRKGGroupLength,
									tmp1xLength));
			  }

			/* Write editrate to component, and recursively add to 
			 * rest of component tree.
			 */
			CHECK(omfiMobMatchAndExecute(file, segment, 0, /* level */
											 isObjFunc, NULL,
											 set1xEditrate, &editRate,
											 &matches));

			/* Append new track to mob */
			CHECK(omfsAppendObjRefArray(file, mob, OMTRKGTracks, tmpTrack));
		  }
		else /* kOmfRev2x */
		  {
			CHECK(omfsObjectNew(file, "MSLT", &tmpTrack));
			CHECK(omfsWriteRational(file, tmpTrack, OMMSLTEditRate, editRate));
			
			CHECK(omfsWriteObjRef(file, tmpTrack, OMMSLTSegment, segment));
			
			CHECK(MobSlotSetTrackDesc(file, tmpTrack, origin, 
										  trackID, trackName, &trackDesc));
			hasDesc = TRUE;
			
			/* Append new slot to mob */
			CHECK(omfsAppendObjRefArray(file, mob, OMMOBJSlots, tmpTrack));
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (hasDesc)
		  omfsObjectDelete(file, trackDesc);
	    omfsObjectDelete(file, tmpTrack);
		return(XCODE());
	  }
	XEND;

	*newTrack = tmpTrack;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiTrackSetPhysicalNum()
 *
 *      Adds physical track num property to the specified track.
 *
 *      This function works only for 2.x files.
 *
 * Argument Notes:
 *		PhysicalTrackNum should be >= 1
 *		For audio:
 *			1 = left
 *			2 = right
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiTrackSetPhysicalNum(
	omfHdl_t		file,      			/* IN - File Handle */
	omfMSlotObj_t	track,				/* IN - An existing track */
	omfUInt32		physicalTrackNum) 	/* IN - A physical track number */
{
    omfObject_t 	trackDesc = NULL;
	omfErr_t		status;
	
	omfAssertValidFHdl(file);
	omfAssert((track != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((file->setrev != kOmfRev1x) && (file->setrev != kOmfRevIMA), file, OM_ERR_NOT_IN_15);
	XPROTECT(file)
	{
		XASSERT(omfiMobSlotIsTrack(file, track, &status), OM_ERR_NOT_A_TRACK);
		
		CHECK(omfsReadObjRef(file, track, OMMSLTTrackDesc, &trackDesc));
		CHECK(omfsWriteUInt32(file, trackDesc, OMTRKDPhysTrack, physicalTrackNum));
	} /* XPROTECT */
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobAppendComment()
 *
 *      Creates a user-defined comment and appends it to the specified Mob.
 *
 *      For 1.x files, this function will create a TRAK object with
 *      the given property values.
 *
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobAppendComment(
	omfHdl_t		file,      	/* IN - File Handle */
	omfMobObj_t		mob,        /* IN - Input Mob Object */
	char			*category,  /* IN - Comment heading */
	char			*comment)	/* IN - Comment value */
{
	omfProperty_t		prop;
	omfInt32			n, cnt;
	omfObject_t			attb, head;
    char				CategoryName[64], attbName[64];
    omfBool				found, commentFound;
    omfErr_t			omfError;
    omfAttributeKind_t	kind;
	
	omfAssertValidFHdl(file);
	omfAssert((category != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((comment != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	{
		if (!omfiIsAMob(file, mob, &omfError))
		{
			RAISE(OM_ERR_INVALID_OBJ);
		}
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			found = FALSE;
			cnt = omfsNumAttributes(file, mob, OMCPNTAttributes);
			for(n = 1; n <= cnt; n++)
			{
				CHECK(omfsGetNthAttribute(file, mob, OMCPNTAttributes, &attb, n));
	 			CHECK(omfsReadAttrKind(file, attb, OMATTBKind, &kind));
				CHECK(omfsReadString(file, attb, OMATTBName, attbName, sizeof(attbName)));
				if((kind == kOMFObjectAttribute) &&
				   (strcmp(attbName, "_USER") == 0))
				{
	  				found = TRUE;
	  				break;
	  			}
			}
			if(!found)
			{
				CHECK(omfsObjectNew(file, "ATTB", &attb));
				CHECK(omfsWriteAttrKind(file, attb, OMATTBKind, kOMFObjectAttribute));
				CHECK(omfsWriteString(file, attb, OMATTBName, "_USER"));
				CHECK(omfsAppendAttribute(file, mob, OMCPNTAttributes, attb));
			}
			head = attb;
		}
		else
			head = mob;
		
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			prop = OMATTBObjAttribute;
		}
		else
		{
			prop = OMMOBJUserAttributes;
		}

		commentFound = FALSE;
		cnt = omfsNumAttributes(file, head, prop);
		for(n = 1; n <= cnt; n++)
		{
			CHECK(omfsGetNthAttribute(file, head, prop, &attb, n));
			CHECK(omfsReadAttrKind(file, attb, OMATTBKind, &kind));
			CHECK(omfsReadString(file, attb, OMATTBName, CategoryName,
								sizeof(CategoryName)));

			/* comments are string attributes whose name matches the category */

			if((kind == kOMFStringAttribute) &&
			   (strcmp(CategoryName, category) == 0))
			{
				/* comment exists, so overwrite string value. */
				commentFound = TRUE;
				CHECK(omfsWriteString(file, attb, OMATTBStringAttribute, comment));
				break;
			}
		}
			
		if(!commentFound)
		{
			CHECK(omfsObjectNew(file, "ATTB", &attb));
			CHECK(omfsWriteAttrKind(file, attb, OMATTBKind, kOMFStringAttribute));
			CHECK(omfsWriteString(file, attb, OMATTBName, category));
			CHECK(omfsWriteString(file, attb, OMATTBStringAttribute, comment));
			CHECK(omfsAppendAttribute(file, head, prop, attb));
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
 * Function: omfiMobSetDefaultFade()
 *
 *      Adds the default crossfade properties to the specified Mob.
 *
 *      This function works only for 2.x files.
 *
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobSetDefaultFade(
	omfHdl_t		file,      		/* IN - File Handle */
	omfMobObj_t		mob,        	/* IN - Input Mob Object */
	omfLength_t		fadeLength, 	/* IN - Default fade length */
	omfFadeType_t	fadeType,		/* IN - default fade type */
	omfRational_t	fadeEditUnit)	/* IN - default fade type */
{
	omfErr_t		status;
	
	omfAssert((file->setrev != kOmfRev1x) && (file->setrev != kOmfRevIMA), file, OM_ERR_NOT_IN_15);
	omfAssert(omfiIsACompositionMob(file, mob, &status), file, OM_ERR_NOT_COMPOSITION);
	
	XPROTECT(file)
	{
		CHECK(omfsWriteLength(file, mob, OMCMOBDefFadeLength, fadeLength));
		CHECK(omfsWriteFadeType(file, mob, OMCMOBDefFadeType, fadeType));
		CHECK(omfsWriteRational(file, mob, OMCMOBDefFadeEditUnit, fadeEditUnit));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;
	
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: ComponentSetNewProps()
 *
 *      This is an internal function that is called by functions that
 *      create new components.  It sets the length and datakind property
 *      values on the input component object.
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
omfErr_t ComponentSetNewProps(
        omfHdl_t file,           /* IN - File Handle */
        omfCpntObj_t component,  /* IN - Component object */
        omfLength_t length,      /* IN - Length property value */
        omfDDefObj_t datakind)   /* IN - Datakind property value */
{
    omfInt32 type;
    omfInt32	tmp1xLength;
    omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssert((component != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			/* Convert datakind to tracktype, and write to component */
			type = MediaKindToTrackType(file, datakind);
			CHECK(omfsWriteTrackType(file, component, OMCPNTTrackKind, 
									 (omfInt16)type));

			/* If 1.5, set length for TRKG, CLIP */
			CHECK(omfsTruncInt64toInt32(length, &tmp1xLength));	/* OK 1.x */
			if (omfsIsTypeOf(file, component, "TRKG", &omfError))
			  {
				CHECK(omfsWriteInt32(file, component, OMTRKGGroupLength,
									tmp1xLength));
			  }
			else if (omfsIsTypeOf(file, component, "CLIP", &omfError))
			  {
				CHECK(omfsWriteInt32(file, component, OMCLIPLength,
									tmp1xLength));
			  }
			/* Sequences don't have length in 1.x, so 0 length is implied */
		  }
		else /* kOmfRev2x */
		  {
			CHECK(omfsWriteObjRef(file, component, OMCPNTDatakind, datakind));
			CHECK(omfsWriteLength(file, component, OMCPNTLength, length));
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
 * Function: omfiSourceClipNew()
 *
 *      This function creates a new source clip object with the
 *      given required properties.  Optional properties are added with
 *      separate functions.
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
omfErr_t omfiSourceClipNew(
	omfHdl_t file,              /* IN - File Handle */
	omfDDefObj_t datakind,      /* IN - Datakind object */
	omfLength_t length,         /* IN - Length property value */
	omfSourceRef_t sourceRef,   /* IN - Source Reference (sourceID, 
								 *      sourceTrackID, startTime)
								 */
	omfSegObj_t *newSourceClip) /* OUT - New Source Clip object */
{
	omfObject_t tmpClip = NULL;

	*newSourceClip = NULL;
	omfAssertValidFHdl(file);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		/* These functions will work for both 1.x and 2.x */
		CHECK(omfsObjectNew(file, "SCLP", &tmpClip));

		CHECK(ComponentSetNewProps(file, tmpClip, length, datakind));
		CHECK(omfiSourceClipSetRef(file, tmpClip, sourceRef));
	  }
	
	XEXCEPT
	  {
		omfsObjectDelete(file, tmpClip);
		return(XCODE());
	  }
	XEND;

	*newSourceClip = tmpClip;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSourceClipSetRef()
 *
 *      This function changes a source reference on the input source
 *      clip object.  A source reference is the information needed for
 *      a source clip to refer to another mob, including the sourceID,
 *      sourceTrackID and startTime.
 *
 *      This function will support both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiSourceClipSetRef(
	omfHdl_t file,             /* IN - File Handle */
	omfSegObj_t sourceClip,    /* IN - Source Clip object */
	omfSourceRef_t sourceRef)  /* IN - Source Reference (sourceID,
								*      sourceTrackID, startTime)
								*/
{
    omfInt32	tmp1xSourcePosition = 0;
	omfInt16	tmp1xTrackNum = 0;
	omfUID_t	nilUID;
	omfDDefObj_t mediaKind = NULL;
	omfErr_t    omfError = OM_ERR_NONE;
	
	omfAssertValidFHdl(file);
	omfAssert((sourceClip != NULL), file, OM_ERR_NULLOBJECT);
	NULLMobID(nilUID);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(omfsWriteUID(file, sourceClip, OMSCLPSourceID, 
							   sourceRef.sourceID));
			CHECK(omfsTruncInt64toInt32(sourceRef.startTime, &tmp1xSourcePosition));	/* OK 1.x */

			if(!equalUIDs(sourceRef.sourceID, nilUID))
			{
			  omfError = CvtTrackIDtoNum(file, sourceRef.sourceID, 
								 sourceRef.sourceTrackID, &tmp1xTrackNum);
			  if (omfError == OM_ERR_MISSING_MOBID)
				{
				  CHECK(AddMobTableEntry(file, NULL, sourceRef.sourceID,
										 kOmTableDupError));
				  CHECK(omfiComponentGetInfo(file, sourceClip, &mediaKind, NULL));
				  CHECK(AddTrackMapping(file, sourceRef.sourceID, 
								  sourceRef.sourceTrackID, mediaKind));
			 	  CHECK(CvtTrackIDtoNum(file, sourceRef.sourceID, 
								 sourceRef.sourceTrackID, &tmp1xTrackNum));
				}
			  else if (omfError == OM_ERR_MISSING_TRACKID)
				{
				  CHECK(omfiComponentGetInfo(file, sourceClip, &mediaKind, NULL));
				  CHECK(AddTrackMapping(file, sourceRef.sourceID, 
								  sourceRef.sourceTrackID, mediaKind));
			 	  CHECK(CvtTrackIDtoNum(file, sourceRef.sourceID, 
								 sourceRef.sourceTrackID, &tmp1xTrackNum));
				}
			  else if (omfError != OM_ERR_NONE)
				{
				  RAISE(omfError);
				}
			}
			else
				tmp1xTrackNum = 0;
			CHECK(omfsWriteInt16(file, sourceClip, OMSCLPSourceTrack, 
								 tmp1xTrackNum));
			CHECK(omfsWriteInt32(file, sourceClip, OMSCLPSourcePosition,
								   tmp1xSourcePosition));
		  }
		else
		  {
			/* If UID is 0,0,0 - make the rest of the fields 0 too. */
			if(equalUIDs(sourceRef.sourceID, nilUID))
			  {
				sourceRef.sourceTrackID = 0;
				omfsCvtInt32toPosition(0, sourceRef.startTime);	
			  }
			CHECK(omfsWriteUID(file, sourceClip, OMSCLPSourceID, 
							   sourceRef.sourceID));
			CHECK(omfsWriteUInt32(file, sourceClip, OMSCLPSourceTrackID, 
								 sourceRef.sourceTrackID));
			CHECK(omfsWritePosition(file, sourceClip, OMSCLPStartTime, 
								sourceRef.startTime));
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
 * Function: omfiSourceClipSetFade()
 *
 *      This function sets the optional fade properties on the input 
 *      source clip object.  The fade properties only apply to a source
 *      clip of datakind (or convertible to a datakind) of type Sound.
 *      All arguments should be specified.
 *
 *      This function will support both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiSourceClipSetFade(
	omfHdl_t file,              /* IN - File Handle */
	omfSegObj_t sourceClip,     /* IN - Source Clip object */
	omfInt32 fadeInLen,         /* IN - Fade In Length */
	omfFadeType_t fadeInType,   /* IN - Fade In Type */
	omfInt32 fadeOutLen,        /* IN - Fade Out Length */
	omfFadeType_t fadeOutType)  /* IN - Fade Out Type */
{
    omfDDefObj_t datakind = NULL;
	omfErr_t omfError = OM_ERR_NONE;
	omfPosition_t	inLen64, outLen64;
	
	omfAssertValidFHdl(file);
	omfAssert((sourceClip != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* Verify that this is a sound or stereo sound clip */
		CHECK(omfiComponentGetInfo(file, sourceClip, &datakind, NULL));
		if (!omfiIsSoundKind(file, datakind, kConvertTo, &omfError))
		  {
			RAISE(OM_ERR_INVALID_DATAKIND);
		  }

		/* These functions will support both 1.x and 2.x */
		if(file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
		{
			CHECK(omfsWriteInt32(file, sourceClip, OMSCLPFadeInLength, 
								  fadeInLen));
			CHECK(omfsWriteInt32(file, sourceClip, OMSCLPFadeInType, 
									fadeInType));
			CHECK(omfsWriteInt32(file, sourceClip, OMSCLPFadeOutLength, 
								  fadeOutLen));
			CHECK(omfsWriteInt32(file, sourceClip, OMSCLPFadeOutType, 
								  fadeOutType));
		}
		else
		{
			omfsCvtInt32toInt64(fadeInLen, &inLen64);
			omfsCvtInt32toInt64(fadeOutLen, &outLen64);
			CHECK(omfsWriteLength(file, sourceClip, OMSCLPFadeInLength, 
								  inLen64));
			CHECK(omfsWriteFadeType(file, sourceClip, OMSCLPFadeInType, 
									fadeInType));
			CHECK(omfsWriteLength(file, sourceClip, OMSCLPFadeOutLength, 
								  outLen64));
			CHECK(omfsWriteFadeType(file, sourceClip, OMSCLPFadeOutType, 
								  fadeOutType));
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
 * Function: omfiTimecodeNew()
 *
 *      This function creates a new timecode clip with the given
 *      property values.  The timecode value is represented with an
 *      omfTimecode_t struct consisting of startFrame, drop, and fps.
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
omfErr_t omfiTimecodeNew(
    omfHdl_t file,               /* IN - File Handle */
	omfLength_t length,          /* IN - Length Property Value */
	omfTimecode_t timecode,      /* IN - Timecode Value (startFrame,
								  *      drop, fps)
								  */
	omfSegObj_t *timecodeClip)   /* OUT - New Timecode Clip */
{
	omfObject_t tmpTCClip = NULL;
	omfDDefObj_t datakind = NULL;
	omfInt32 tmp1xLength;
	omfInt32	tmp1xStartTC, tmp1xFlags;
	omfInt16 tmp1xFps;
	omfErr_t omfError = OM_ERR_NONE;
	omfPosition_t	startPos;
	
	*timecodeClip = NULL;
	omfAssertValidFHdl(file);

	XPROTECT(file)
	  {
		CHECK(omfsObjectNew(file, "TCCP", &tmpTCClip));

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(omfsWriteTrackType(file, tmpTCClip, OMCPNTTrackKind,
									 TT_TIMECODE));

			CHECK(omfsTruncInt64toInt32(length, &tmp1xLength));	/* OK 1.x */
			CHECK(omfsWriteInt32(file, tmpTCClip, OMCLIPLength, tmp1xLength));
			tmp1xStartTC = (omfInt32)timecode.startFrame;
			CHECK(omfsWriteInt32(file, tmpTCClip, OMTCCPStartTC, tmp1xStartTC));
			tmp1xFps = (omfInt16)timecode.fps;
			CHECK(omfsWriteInt16(file, tmpTCClip, OMTCCPFPS, tmp1xFps));
			tmp1xFlags = timecode.drop;
			CHECK(omfsWriteInt32(file, tmpTCClip, OMTCCPFlags, tmp1xFlags));
		  }
		else /* kOmfRev2x */
		  {
			/* Set datakind object for timecode from cache */
			if (!omfiDatakindLookup(file, TIMECODEKIND, &datakind, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }

			CHECK(ComponentSetNewProps(file, tmpTCClip, length, datakind));
			omfsCvtInt32toInt64(timecode.startFrame, &startPos);
			CHECK(omfsWritePosition(file, tmpTCClip, OMTCCPStart, 
								startPos));
			CHECK(omfsWriteUInt16(file, tmpTCClip, OMTCCPFPS, timecode.fps));
			CHECK(omfsWriteBoolean(file, tmpTCClip, OMTCCPDrop,
									(omfBool)(timecode.drop == kTcDrop)));
		  } /* kOmfRev2x */
	  }

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpTCClip);
		return(XCODE());
	  }
	XEND;

	*timecodeClip = tmpTCClip;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEdgecodeNew()
 *
 *      This function creates a new edgecode clip with the given 
 *      property values.  The edgecode value is represented with an
 *      omfEdgecode_t struct consisting of startFrame, filmKind, and
 *      codeFormat.
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
omfErr_t omfiEdgecodeNew(
    omfHdl_t file,             /* IN - File Handle */
	omfLength_t length,        /* IN - Length Property Value */
	omfEdgecode_t edgecode,    /* IN - Edgecode Value (startFrame,
							    *      filmKind, codeFormat)
							    */
	omfSegObj_t *edgecodeClip) /* OUT - New Edgecode Clip */
{
    omfDDefObj_t datakind = NULL;
	omfObject_t tmpECClip;
	omfInt32 tmp1xStartEC, n;
	omfInt32	tmp1xLength;
	omfPosition_t	startPos, zeroPos;
	omfBool		doFill;
	omfErr_t omfError = OM_ERR_NONE;

	*edgecodeClip = NULL;
	omfAssertValidFHdl(file);

	XPROTECT(file)
	  {
	  	omfsCvtInt32toInt64(0, &zeroPos);
		CHECK(omfsObjectNew(file, "ECCP", &tmpECClip));

		doFill = FALSE;
		for(n = 0; n < 8; n++)
		{
			if(doFill)
				edgecode.header[n] = '\0';
			else if(edgecode.header[n] == '\0')
				doFill = TRUE;
		}
		
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(omfsWriteTrackType(file, tmpECClip, OMCPNTTrackKind,
									 TT_EDGECODE));

			CHECK(omfsTruncInt64toInt32(length, &tmp1xLength));	/* OK 1.x */
			CHECK(omfsWriteInt32(file, tmpECClip, OMCLIPLength, tmp1xLength));
			tmp1xStartEC = (omfInt32)edgecode.startFrame;
			CHECK(omfsWriteInt32(file, tmpECClip, OMECCPStartEC, tmp1xStartEC));
			CHECK(omfsWriteFilmType(file, tmpECClip, OMECCPFilmKind,
								   edgecode.filmKind));
			CHECK(omfsWriteEdgeType(file, tmpECClip, OMECCPCodeFormat,
								   edgecode.codeFormat));
			CHECK(omfsWriteVarLenBytes(file, tmpECClip, OMECCPHeader,
									   zeroPos, 8, edgecode.header));
		  }
		else /* kOmfRev2x */
		  {
			/* Set datakind object for edgecode from cache */
			if (!omfiDatakindLookup(file, EDGECODEKIND,
									&datakind, &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
			
			CHECK(ComponentSetNewProps(file, tmpECClip, length, datakind));
			omfsCvtInt32toInt64(edgecode.startFrame, &startPos);
			CHECK(omfsWritePosition(file, tmpECClip, OMECCPStart, 
								startPos));
			CHECK(omfsWriteFilmType(file, tmpECClip, OMECCPFilmKind,
									edgecode.filmKind));
			CHECK(omfsWriteEdgeType(file, tmpECClip, OMECCPCodeFormat,
									edgecode.codeFormat));
			CHECK(omfsWriteDataValue(file, tmpECClip, OMECCPHeader,
									   datakind, edgecode.header, zeroPos, 8));
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpECClip);
		return(XCODE());
	  }
	XEND;

	*edgecodeClip = tmpECClip;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiFillerNew()
 *
 *      This function will create a new filler object with the
 *      given property values.
 *
 *      This function will support both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiFillerNew(	
	omfHdl_t file,          /* IN - File Handle */
	omfDDefObj_t datakind,  /* IN - Datakind Object */
	omfLength_t length,     /* IN - Length Property Value */
	omfSegObj_t *newFiller) /* OUT - New Filler Object */
{
	omfObject_t tmpFiller = NULL;

	*newFiller = NULL;
	omfAssertValidFHdl(file);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		/* This should work for both 1.x and 2.x */
		CHECK(omfsObjectNew(file, "FILL", &tmpFiller));
		CHECK(ComponentSetNewProps(file, tmpFiller, length, datakind));
	  }
	
	XEXCEPT
	  {
		omfsObjectDelete(file, tmpFiller);
		return(XCODE());
	  }
	XEND;

	*newFiller = tmpFiller;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiConstValueNew()
 *
 *      This function creates a new constant value object with the
 *      given property values.
 *
 *      This function only supports 2.x files.  Constant value objects
 *      don't exist in 1.x and cannot be translated into an equivalent
 *      1.x structure.
 *
 * Argument Notes:
 *      The value data is passed in as a void * through the "value"
 *      argument.  The size of the value must be passed through the
 *      valueSize argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiConstValueNew(
	omfHdl_t file,          /* IN - File Handle */
	omfDDefObj_t datakind,  /* IN - Datakind object */
	omfLength_t length,     /* IN - Length property value */
	omfInt32 valueSize,     /* IN - Size of the value data */
	void *value,            /* IN - Value data */
	omfSegObj_t *constValueSeg) /* OUT - Constant value object */
{
	omfObject_t tmpCVal = NULL;
	omfPosition_t	zeroPos;
	
	omfsCvtInt32toPosition(0, zeroPos);	
	*constValueSeg = NULL;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		CHECK(omfsObjectNew(file, "CVAL", &tmpCVal));
		CHECK(ComponentSetNewProps(file, tmpCVal, length, datakind));
		CHECK(omfsWriteDataValue(file, tmpCVal, OMCVALValue, datakind, value, 
								 zeroPos, (omfUInt32)valueSize));
	  }

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpCVal);
		return(XCODE());
	  }
	XEND;

	*constValueSeg = tmpCVal;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiVaryValueNew()
 *
 *      This function creates a new varying value object with the given
 *      property values.  To add control points to the varying value,
 *      use the function omfiVaryValueAddPoint().
 *
 *      This function only supports 2.x files.  Varying value objects
 *      don't exist in 1.x and cannot be translated into an equivalent
 *      1.x structure.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiVaryValueNew(
    omfHdl_t file,                 /* IN - File Handle */
	omfDDefObj_t datakind,         /* IN - Datakind object */
	omfLength_t length,            /* IN - Length property value */
	omfInterpKind_t interpolation, /* IN - Interpolation method */
	omfSegObj_t *varyValueSeg)     /* OUT - Varying value object */
{
    omfObject_t tmpVVal = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	*varyValueSeg = NULL;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		if (file->semanticCheckEnable)
		  {
			/* Verify that the point value's datakind is not a media stream */
			if (IsDatakindMediaStream(file, datakind, &omfError))
			  {
				RAISE(OM_ERR_MEDIASTREAM_NOTALLOWED);
			  }
		  }

		CHECK(omfsObjectNew(file, "VVAL", &tmpVVal));
		CHECK(ComponentSetNewProps(file, tmpVVal, length, datakind));
		CHECK(omfsWriteInterpKind(file, tmpVVal, OMVVALInterpolation,
				     interpolation));
	  }

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpVVal);
		return(XCODE());
	  }
	XEND;

	*varyValueSeg = tmpVVal;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: InsertPoint()
 *
 *      This internal function inserts a control point in time order
 *      into the input varying value object.  It is called by the
 *      public function omfiVaryValueAddPoint().
 *
 *      This function only supports 2.x files.  Varying value objects
 *      and control points don't exist in 1.x and cannot be translated 
 *      into an equivalent 1.x structure.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t InsertPoint(omfHdl_t file,     /* IN - File Handle */
				     omfSegObj_t varyValue,    /* IN - Varying value object */
					 omfRational_t time,       /* IN - Time of control point */
					 omfCntlPtObj_t point)     /* IN - Control Point object */
{
  omfInt32 loop, numPoints, pos = 0;
  omfObject_t tmpPoint = NULL;
  omfRational_t tmpTime;
  double fTime, fTmpTime;

  omfAssertValidFHdl(file);
  omfAssert((varyValue != NULL), file, OM_ERR_NULLOBJECT);
  omfAssert((point != NULL), file, OM_ERR_NULLOBJECT);

  XPROTECT(file)
	{
	  fTime = FloatFromRational(time);
	  numPoints = omfsLengthObjRefArray(file, varyValue, OMVVALPointList);

	  /* If this is the first point, add it to the list */
	  if (numPoints == 0)
		{
		  CHECK(omfsAppendObjRefArray(file, varyValue, OMVVALPointList,
											  point));
		}
	  /* Else, insert it into the list */								
	  else  /* not first point in the list, search to find where to insert */
		{
		  for (loop=1; loop <= numPoints; loop++)
			{
			  CHECK(omfsGetNthObjRefArray(file, varyValue, 
									  OMVVALPointList, &tmpPoint, loop));
			  CHECK(omfiControlPtGetInfo(file, tmpPoint, &tmpTime, NULL,
											  NULL, 0, NULL, NULL));
			  fTmpTime = FloatFromRational(tmpTime);
			  if (fTmpTime > fTime)
				{
				  pos = loop;
				  break;
				}
			  /* Release Bento reference, so the useCount is decremented */
			  CMReleaseObject((CMObject)tmpPoint);
			} /* for */
		  /* If the point time is greater than the whole list, append */
		  if (pos == 0)
			{
			  CHECK(omfsAppendObjRefArray(file, varyValue, OMVVALPointList, 
										  point));
			}
		  /* Insert at pos */
		  else
			{
			  CHECK(omfsInsertObjRefArray(file, varyValue, OMVVALPointList, 
										  point, pos));
			}
		} /* else insert into list */
	} /* XPROTECT */

  XEXCEPT
	{
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiVaryValueAddPoint()
 *
 *      This function adds a control point in time order into a varying
 *      value object.
 *
 *      This function only supports 2.x files.  Constant value objects
 *      don't exist in 1.x and cannot be translated into an equivalent
 *      1.x structure.
 *
 * Argument Notes:
 *      The value data is passed in as a void * through the "value"
 *      argument.  The size of the value must be passed through the
 *      valueSize argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiVaryValueAddPoint(
    omfHdl_t file,          /* IN - File Handle */
	omfSegObj_t varyValue,  /* IN - VaryValue object */
	omfRational_t time,     /* IN - Time for control Point */
	omfEditHint_t editHint, /* IN - Edit Hint */
	omfDDefObj_t datakind,  /* IN - Datakind object */
	omfInt32 valueSize,     /* IN - Size of value data */
	void *value)            /* IN - Value data */
{
    omfCntlPtObj_t tmpPoint = NULL;
	omfPosition_t	zeroPos;
	omfDDefObj_t vvDatakind = NULL;
	omfUniqueName_t datakindName;
	omfErr_t omfError = OM_ERR_NONE;
	
	omfsCvtInt32toPosition(0, zeroPos);

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((varyValue != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		if (file->semanticCheckEnable)
		  {
			/* Verify that the point value's datakind is not a media stream */
			if (IsDatakindMediaStream(file, datakind, &omfError))
			  {
				RAISE(OM_ERR_MEDIASTREAM_NOTALLOWED);
			  }

			/* Verify that point's datakind converts to varyVal's datakind */
			CHECK(omfiComponentGetInfo(file, varyValue, &vvDatakind, NULL));
			CHECK(omfiDatakindGetName(file, vvDatakind, OMUNIQUENAME_SIZE, 
									  datakindName));
			if (!DoesDatakindConvertTo(file, datakind, datakindName, 
									   &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  } /* semanticCheckEnable */

		CHECK(omfsObjectNew(file, "CTLP", &tmpPoint));
		CHECK(omfsWriteRational(file, tmpPoint, OMCTLPTime, time));
		if (editHint != kNoEditHint)
		  CHECK(omfsWriteEditHint(file, tmpPoint, OMCTLPEditHint, editHint));
		CHECK(omfsWriteObjRef(file, tmpPoint, OMCTLPDatakind, datakind));
		CHECK(omfsWriteDataValue(file, tmpPoint, OMCTLPValue, datakind,
								 value, zeroPos, (omfUInt32)valueSize));
		CHECK(InsertPoint(file, varyValue, time, tmpPoint));
	  }

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpPoint);
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSequenceNew()
 *
 *      This function creates a new sequence object with the given
 *      property values.  The length of the sequence is initially set
 *      to 0.  When components are appended to the sequence with the
 *      omfiSequenceAppendCpnt() call, the length of the appended component
 *      is added to the length of the sequence.
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
omfErr_t omfiSequenceNew(
	omfHdl_t file,             /* IN - File Handle */
	omfDDefObj_t datakind,     /* IN - Datakind object */
	omfSegObj_t *newSequence)  /* OUT - Sequence object */
{
	omfObject_t tmpSequence = NULL;
	omfLength_t	zeroLen = omfsCvtInt32toLength(0, zeroLen);
	
	*newSequence = NULL;
	omfAssertValidFHdl(file);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		/* This function should work for both 1.x and 2.x */
		CHECK(omfsObjectNew(file, "SEQU", &tmpSequence));

		/* Length should be 0 for newly created sequence */
		CHECK(ComponentSetNewProps(file, tmpSequence, zeroLen, datakind));
	  }

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpSequence);
		return(XCODE());
	  }
	XEND;

	*newSequence = tmpSequence;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: IsValidTranEffect()
 *
 *      This internal boolean function verifies that the input effect 
 *      object can be used in a transition.  
 *
 *      NOTE: For the time being this function simply makes sure that
 *      the effect object is an EFFE.  There isn't a property on the
 *      effect that designates it to "possibly be used in a transition",
 *      so there is no way to validate whether this is true for privately
 *      defined effects.  This function will stay for now if we decide
 *      that it is possible to semantically validate "transition effects"
 *      in the future.
 *
 *      This function only supports 2.x files.
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
omfBool IsValidTranEffect(
    omfHdl_t    file,       /* IN - File Handle */
    omfEffObj_t effect,     /* IN - Effect object */
    omfErr_t    *omfError)  /* OUT - Error code */
{
  *omfError = OM_ERR_NONE;

  omfAssertValidFHdlBool(file, omfError, FALSE);

  XPROTECT(file)
	{
	  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		{
		  return(TRUE);
		}
	  else /* kOmfRev2x */
		{
		  if (!omfiIsAnEffect(file, effect, omfError))
			{
			  RAISE(OM_ERR_INVALID_EFFECT);
			}
		  return(TRUE);
		}
	}

  XEXCEPT
	{
	  *omfError = XCODE();
	}
  XEND_SPECIAL(FALSE);

  return(FALSE);
}

/*************************************************************************
 * Function: omfiTransitionNew()
 *
 *      This function creates a new transition object with the given
 *      property values.  It expects a complete effect object as input,
 *      and verifies that this is a valid transition effect.
 *
 *      This function supports both 1.x and 2.x files.  For 1.x files,
 *      it creates 2 tracks and appends them to the transition object
 *      (which was formerly a subclass of TRKG.)
 *      
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiTransitionNew(
	omfHdl_t file,             /* IN - File Handle */
	omfDDefObj_t datakind,     /* IN - Datakind object */
	omfLength_t length,        /* IN - Length property value */
	omfPosition_t cutPoint,    /* IN - Cut Point */
	omfEffObj_t effect,        /* IN - Effect object */
	omfCpntObj_t *newTransition) /* OUT - New Transition */
{
	omfObject_t tmpTransition = NULL;
	omfObject_t track1 = NULL, track2 = NULL;
	omfInt16 trackID;
	omfInt32 tmp1xCutPoint;
	omfInt32 len = 0;
	char buf[32]; /* Big enough to hold 1.x effect ID */
	omfErr_t omfError = OM_ERR_NONE;

	*newTransition = NULL;
	omfAssertValidFHdl(file);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		CHECK(omfsObjectNew(file, "TRAN", &tmpTransition));

		/* Will write TRKGGroupLength and CPNTTrackKind for 1.x */
		CHECK(ComponentSetNewProps(file, tmpTransition, length, datakind));

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(omfsTruncInt64toInt32(cutPoint, &tmp1xCutPoint));	/* OK 1.x */
			CHECK(omfsWriteInt32(file, tmpTransition,OMTRANCutPoint, 
								tmp1xCutPoint));

			/* If EffectID string exists on effect, write to transition */
			len = omfsLengthString(file, effect, OMCPNTEffectID);
			if (len)
			  {
				CHECK(omfsReadString(file, effect, OMCPNTEffectID, buf, sizeof(buf)));
				CHECK(omfsWriteString(file, tmpTransition, OMCPNTEffectID,
									  buf));
			  }

			/* NOTE: Should the tracks be pulled off of the effect object? */
			CHECK(omfsObjectNew(file, "TRAK", &track1));
			CHECK(omfsObjectNew(file, "TRAK", &track2));

			/* Tracks will be numbered sequentially in a transition since
			 * all tracks are of the same type, so no 1.x track mapping
			 * is necessary.
			 */
			trackID = 1;
			CHECK(omfsWriteInt16(file, track1, OMTRAKLabelNumber, trackID));
			trackID = 2;
			CHECK(omfsWriteInt16(file, track2, OMTRAKLabelNumber, trackID));

			CHECK(omfsAppendObjRefArray(file, tmpTransition, OMTRKGTracks, 
										track1));
			CHECK(omfsAppendObjRefArray(file, tmpTransition, OMTRKGTracks, 
										track2));
		  }
		else /* xOmfRev2x */
		  {
			CHECK(omfsWritePosition(file, tmpTransition,OMTRANCutPoint, cutPoint));

			/* Make sure that the effect is a valid transition effect */
			if (!IsValidTranEffect(file, effect, &omfError))
			  {
				RAISE(OM_ERR_INVALID_TRAN_EFFECT);
			  }

			if (effect == NULL)
			  {
				RAISE(OM_ERR_INVALID_EFFECT);
			  }
			else
			  {
				CHECK(omfsWriteObjRef(file, tmpTransition, OMTRANEffect, 
									  effect));
			  }
		  } 
	  } /* XPROTECT */

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpTransition);
		return(XCODE());
	  }
	XEND;

	*newTransition = tmpTransition;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSequenceAppendCpnt()
 *
 *      This function appends the input component to the given
 *      sequence, enforcing bottom up creation of mobs.  The length
 *      of the sequence is incremented by the size of the component,
 *      unless the component is a transition.  If the component is
 *      a transition, it verifies that it is not the first object
 *      in a transition, and that it is not neighboring another transition.
 *      It also verifies that there is enough source material on either
 *      side of the transition.  The function also verifies that the datakinds 
 *      are compatible.
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
omfErr_t omfiSequenceAppendCpnt(
	omfHdl_t file,             /* IN - File Handle */
	omfSegObj_t sequence,      /* IN - Sequence object */
	omfCpntObj_t component)    /* IN - Component to append to the sequence */
{
	omfInt32 numCpnts;
	omfLength_t sequLen, cpntLen, prevLen;
	omfProperty_t prop;
	omfDDefObj_t sequDatakind = NULL, cpntDatakind = NULL;
	omfObject_t prevCpnt = NULL;
	omfUniqueName_t datakindName;
	omfBool isPrevTran = FALSE;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssert((sequence != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((component != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, sequence, &sequDatakind, &sequLen));
		CHECK(omfiComponentGetInfo(file, component, &cpntDatakind, &cpntLen));

		if (file->semanticCheckEnable)
		  {
			/* Verify that cpnt's datakind converts to sequence's datakind */
			CHECK(omfiDatakindGetName(file, sequDatakind, OMUNIQUENAME_SIZE, 
									  datakindName));
			if (!DoesDatakindConvertTo(file, cpntDatakind, datakindName,
									   &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }
		  } /* semanticCheckEnable */

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  prop = OMSEQUSequence;
		else /* xOmfRev2x */
		  prop = OMSEQUComponents;

		/* Get the previous element in the sequence to verify neighboring
		 * transitions and source clip lengths.
		 */
		numCpnts = omfsLengthObjRefArray(file, sequence, prop);
		if (numCpnts)
		  {
			CHECK(omfsGetNthObjRefArray(file, sequence, prop,
										&prevCpnt, numCpnts));
			if (omfiIsATransition(file, prevCpnt, &omfError))
			  {
				isPrevTran = TRUE;
			  }
			CHECK(omfiComponentGetLength(file, prevCpnt, &prevLen));
			/* Release Bento reference, so the useCount is decremented */
			CMReleaseObject((CMObject)prevCpnt);
		  }

		/* Is the newly appended component a transition? */
	    if (omfiIsATransition(file, component, &omfError))
		  {
			if (isPrevTran) 
			  {
				RAISE(OM_ERR_ADJACENT_TRAN);
			  }
			else if (numCpnts == 0) /* It is the first cpnt in the sequ */
			  {
				RAISE(OM_ERR_LEADING_TRAN);
			  }
			else
			  {
				/* Verify that previous SCLP is at least as int  as the tran */
				if (omfsInt64Less(prevLen, cpntLen))
				  {
					RAISE(OM_ERR_INSUFF_TRAN_MATERIAL);
				  }
			  }
			omfsSubInt64fromInt64(cpntLen, &sequLen);
			/* 1.x does not have a Sequence Length property */
			if (file->setrev == kOmfRev2x)
			  {
				CHECK(omfsWriteLength(file, sequence,OMCPNTLength, sequLen));
			  }
		  }
		else /* Not a transition */
		  {
			if (isPrevTran)
			  {
				/* Verify that length is at least as int  as the prev tran */
				if (omfsInt64Less(cpntLen, prevLen))
				  {
					RAISE(OM_ERR_INSUFF_TRAN_MATERIAL);
				  }
			  }
			/* Add length of component to sequence, if not transition */
			omfsAddInt64toInt64(cpntLen, &sequLen);
			/* 1.x does not have a Sequence Length property */
			if (file->setrev == kOmfRev2x)
			  {
				CHECK(omfsWriteLength(file, sequence,OMCPNTLength, sequLen));
			  }
		  }

		/* If it all checks out, append the component to the sequence */
		CHECK(omfsAppendObjRefArray(file, sequence, prop, component));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiNestedScopeNew()
 *
 *     This function creates a new empty nested scope object with the given
 *     property values.  To appends slots to the scope, use the 
 *     omfiNestedScopeAppendSlot() function.
 *
 *     This function is only supported for 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiNestedScopeNew(
	omfHdl_t file,          /* IN - File Handle */
	omfDDefObj_t datakind,  /* IN - Datakind object */
	omfLength_t length,     /* IN - Length property value */
	omfSegObj_t *newScope)  /* OUT - New nested scope object */
{
	omfObject_t tmpScope = NULL;

	*newScope = NULL;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		CHECK(omfsObjectNew(file, "NEST", &tmpScope));
		CHECK(ComponentSetNewProps(file, tmpScope, length, datakind));
	  }

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpScope);
		return(XCODE());
	  }
	XEND;
	
	*newScope = tmpScope;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiNestedScopeAppendSlot()
 *
 *      This function appends a given segment into the slots property
 *      of a scope.  Slots in a scope are simply represented as an array of
 *      segments (i.e., there is no explicit "scope slot" object).
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiNestedScopeAppendSlot(
	omfHdl_t file,       /* IN - File Handle */
	omfSegObj_t scope,   /* IN - Scope object */
	omfSegObj_t value)   /* IN - Segment to place in scope slot */
{
	omfLength_t scopeLen, valueLen;
	omfDDefObj_t scopeDatakind = NULL, valueDatakind = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((scope != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((value != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, scope, &scopeDatakind, &scopeLen));
		CHECK(omfiComponentGetInfo(file, value, &valueDatakind, &valueLen));

		if (file->semanticCheckEnable)
		  {
			/* The datakind of the last segment in the scope (its value) must
			 * be the same as or convertible to the datakind of the nested
			 * scope object.  The rest of the slots do not have to
			 * match the scope's datakind.  Since these routines cannot 
			 * determine whether this is the last slot, they will not perform 
			 * any semantic checking.  The checker tool will need to confirm
			 * the correctness once the entire file is built.
			 */

			/* Verify that scope slot value is same length as scope */
			if (omfsInt64NotEqual(scopeLen, valueLen))
			  {
				RAISE(OM_ERR_BAD_SLOTLENGTH);
			  }
		  } /* semanticCheckEnable */

		CHECK(omfsAppendObjRefArray(file, scope, OMNESTSlots, value));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiScopeRefNew()
 *
 *      This function creates a new scope reference object with the
 *      given property values.
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
omfErr_t omfiScopeRefNew(
	omfHdl_t file,          /* IN - File Handle */
	omfDDefObj_t datakind,  /* IN - Datakind object */
	omfLength_t length,     /* IN - Length property value */
	omfUInt32 relScope,     /* IN - Relative Scope */
	omfUInt32 relSlot,      /* IN - Relative slot */
	omfSegObj_t *newScopeRef) /* OUT - Scope Reference */
{
    omfObject_t tmpScopeRef = NULL;

	*newScopeRef = NULL;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		/* NOTE: how can we semantic check this?  We need the mob object
		 * to determine the context
		 */
		CHECK(omfsObjectNew(file, "SREF", &tmpScopeRef));
		CHECK(ComponentSetNewProps(file, tmpScopeRef, length, datakind));
		CHECK(omfsWriteUInt32(file, tmpScopeRef, OMSREFRelativeScope, 
							 relScope));
		CHECK(omfsWriteUInt32(file, tmpScopeRef, OMSREFRelativeSlot, relSlot));
	  }

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpScopeRef);
		return(XCODE());
	  }
	XEND;

	*newScopeRef = tmpScopeRef;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSelectorNew()
 *
 *      This function creates a new empty selector object with the given
 *      property values.  Selector slots should be added with the
 *      omfiSelectorSetSelected() and omfiSelectorAddAlt() functions.
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
omfErr_t omfiSelectorNew(
	omfHdl_t file,              /* IN - File Handle */
	omfDDefObj_t datakind,      /* IN - Datakind object */
	omfLength_t length,         /* IN - Length property value */
	omfSegObj_t *newSelector)   /* OUT - New Selector object */
{
	omfObject_t tmpSelector = NULL;

	*newSelector = NULL;
	omfAssertValidFHdl(file);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		CHECK(omfsObjectNew(file, "SLCT", &tmpSelector));

		/* Will set CPNTTrackType and TRKGGroupLength for 1.x */
		CHECK(ComponentSetNewProps(file, tmpSelector, length, datakind));

		/* For 1.x, all tracks will have the same length */
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			/* Editrate is added when appending to mob track.  Building
			 * bottom up is assumed.
			 */
			CHECK(omfsWriteBoolean(file, tmpSelector, OMSLCTIsGanged, TRUE));
		  }
	  }

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpSelector);
		return(XCODE());
	  }
	XEND;

	*newSelector = tmpSelector;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSelectorSetSelected()
 *
 *      This function sets the Selected slot on the given selector
 *      to be the segment object that is passed in the "value" parameter.
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
omfErr_t omfiSelectorSetSelected(
    omfHdl_t file,        /* IN - File Handle */
	omfSegObj_t selector, /* IN - Selector object */
	omfSegObj_t value)    /* IN - Selected segment in selector */
{
    omfLength_t selLen, valLen;
	omfObject_t tmpTrack = NULL;
	omfDDefObj_t selDatakind= NULL, valDatakind = NULL;
	omfUniqueName_t datakindName;
	omfInt16 numTracks, trackLabel;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssert((selector != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((value != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, selector, &selDatakind, &selLen));
		CHECK(omfiComponentGetInfo(file, value, &valDatakind, &valLen));

		if (file->semanticCheckEnable)
		  {
			/* Verify that value's datakind converts to selector's datakind */
			CHECK(omfiDatakindGetName(file, selDatakind, OMUNIQUENAME_SIZE, 
									  datakindName));
			if (!DoesDatakindConvertTo(file, valDatakind, datakindName, 
									   &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }

			/* Verify that value is same length as selector */
			if (omfsInt64NotEqual(selLen,  valLen))
			  {
				RAISE(OM_ERR_BAD_SLOTLENGTH);
			  }
		  } /* semanticCheckEnable */

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			/* Create the new TRAK, create new label, append track to 
			 * selector, then set selector Selected property to new label.
			 */
			CHECK(omfsObjectNew(file, "TRAK", &tmpTrack));
			numTracks = (omfInt16)omfsLengthObjRefArray(file, selector, 
														OMTRKGTracks);

			/* Tracks will be numbered sequentially in a selector since
			 * all tracks are of the same type, so no 1.x track mapping
			 * is necessary.
			 */
			trackLabel = (short)(numTracks+1);
			CHECK(omfsWriteInt16(file, tmpTrack, OMTRAKLabelNumber,
								 trackLabel));
			CHECK(omfsWriteObjRef(file, tmpTrack, OMTRAKTrackComponent, 
								  value));
			CHECK(omfsAppendObjRefArray(file, selector, OMTRKGTracks, 
										tmpTrack));

			CHECK(omfsWriteInt16(file, selector, OMSLCTSelectedTrack, 
								 trackLabel));
		  }
		else /* kOmfRev2x */
		  {
			CHECK(omfsWriteObjRef(file, selector, OMSLCTSelected, value));
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
 * Function: omfiSelectorAddAlt()
 *
 *
 *      This function adds an alternate slot to the given selector.
 *
 *      This function suppots both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiSelectorAddAlt(
	omfHdl_t file,         /* IN - File Handle */
	omfSegObj_t selector,  /* IN - Selector object */
	omfSegObj_t value)     /* IN - Alternate segment in selector */
{
	omfLength_t selLen, valLen;
	omfObject_t tmpTrack = NULL;
	omfDDefObj_t selDatakind = NULL, valDatakind = NULL;
	omfUniqueName_t datakindName;
	omfInt16 numTracks, trackLabel;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssert((selector != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((value != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, selector, &selDatakind, &selLen));
		CHECK(omfiComponentGetInfo(file, value, &valDatakind, &valLen));

		if (file->semanticCheckEnable)
		  {
			/* Verify that value's datakind converts to selector's datakind */
			CHECK(omfiDatakindGetName(file, selDatakind, OMUNIQUENAME_SIZE, 
									  datakindName));
			if (!DoesDatakindConvertTo(file, valDatakind, datakindName,
									   &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }

			/* Verify that scope slot value is same length as scope */
			if (omfsInt64NotEqual(selLen, valLen))
			  {
				RAISE(OM_ERR_BAD_SLOTLENGTH);
			  }
		  } /* semanticCheckEnable */

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			/* Create a new TRAK for the selector trackgroup, assign
			 * a tracklabel, add the value segment, and append to the 
			 * selector.
			 */
			CHECK(omfsObjectNew(file, "TRAK", &tmpTrack));
			numTracks = (omfInt16)omfsLengthObjRefArray(file, selector, 
														OMTRKGTracks);

			/* Tracks will be numbered sequentially in a selector since
			 * all tracks are of the same type, so no 1.x track mapping
			 * is necessary.
			 */
			trackLabel = (omfInt16)(numTracks+1);
			CHECK(omfsWriteInt16(file, tmpTrack, OMTRAKLabelNumber,
								 trackLabel));
			CHECK(omfsWriteObjRef(file, tmpTrack, OMTRAKTrackComponent, 
								  value));
			CHECK(omfsAppendObjRefArray(file, selector, OMTRKGTracks, 
										tmpTrack));
		  }
		else /* kOmfRev2x */
		  {
			CHECK(omfsAppendObjRefArray(file, selector, OMSLCTAlternates, 
										value));
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
 * Function: omfiEffectDefNew()
 *
 *      This function creates a new effect definition object with the
 *      given property values.  This function will add the effect definition
 *      to the effect definition cache and the HEAD Definitions index
 *      if it is not already there.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *      effectName, effectDesc are optional, and should be NULL if
 *      not desired.  bypass is also optional and should have a value
 *      of 0, if not desired.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiEffectDefNew(
	omfHdl_t file,               /* IN - File Handle */
	omfUniqueNamePtr_t effectID, /* IN - Effect identifier */
	omfString effectName,        /* IN - Effect Name (optional) */
	omfString effectDesc,        /* IN - Effect Description (optional) */
	omfArgIDType_t *bypass,      /* IN - Bypass (optional) */
	omfBool isTimeWarp,          /* IN - Is the effect a timewarp */
	omfEDefObj_t *newEffectDef)  /* OUT - New effect definition object */
{
	omfObject_t tmpEffectDef = NULL, lookupDef = NULL;
	omfBool created = FALSE;
	omfErr_t omfError = OM_ERR_NONE;

	*newEffectDef = NULL;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);

	XPROTECT(file)
	  {
		if (omfiEffectDefLookup(file, effectID, &lookupDef, &omfError))
		  {
			RAISE(OM_ERR_EFFECTDEF_EXIST);
		  }

		CHECK(omfsObjectNew(file, "EDEF", &tmpEffectDef));
		created = TRUE;
		CHECK(omfsWriteUniqueName(file, tmpEffectDef, 
										   OMEDEFEffectID, effectID));
		if (effectName)
		  {
			CHECK(omfsWriteString(file, tmpEffectDef, 
								  OMEDEFEffectName, effectName));
		  }
		if (effectDesc)
		  {
			CHECK(omfsWriteString(file, tmpEffectDef, 
								  OMEDEFEffectDescription, effectDesc));
		  }

		if (bypass)
		  {
			CHECK(omfsWriteArgIDType(file, tmpEffectDef, OMEDEFBypass, *bypass));
		  }
		CHECK(omfsWriteBoolean(file, tmpEffectDef, OMEDEFIsTimeWarp,
					isTimeWarp));
		CHECK(omfiEffectDefRegister(file, effectID, tmpEffectDef));
	  }

	XEXCEPT
	  {
		if (created)
		  omfsObjectDelete(file, tmpEffectDef);
		return(XCODE());
	  }
	XEND;

	*newEffectDef = tmpEffectDef;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiDatakindNew()
 *
 *      This function creates a new datakind object with the given
 *      unique name.  It will add the object to the HEAD Definitions
 *      index and the file handle cache if it does not already exist.
 *      If a datakind object with the same name already exists, the
 *      function will return a OM_ERR_DATAKIND_EXIST error.
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
omfErr_t omfiDatakindNew(
	omfHdl_t file,                   /* IN - File Handle */
	omfUniqueNamePtr_t datakindName, /* IN - Datakind Unique Name */
    omfDDefObj_t *newDatakind)       /* OUT - New Datakind object */
{
    omfObject_t lookupDef = NULL, tmpDatakind = NULL;
	omfBool created = FALSE;
	omfErr_t omfError = OM_ERR_NONE;

	*newDatakind = NULL;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);

	XPROTECT(file)
	  {
		if (omfiDatakindLookup(file, datakindName, &lookupDef, &omfError))
		  {
			RAISE(OM_ERR_DATAKIND_EXIST);
		  }
		CHECK(omfsObjectNew(file, "DDEF", &tmpDatakind));
		created = TRUE;
		CHECK(omfsWriteUniqueName(file, tmpDatakind, OMDDEFDatakindID,
								  datakindName));
		CHECK(omfiDatakindRegister(file, datakindName, tmpDatakind));
	  }

	XEXCEPT
	  {
		if (created)
		  omfsObjectDelete(file, tmpDatakind);
		return(XCODE());
	  }
	XEND;

	*newDatakind = tmpDatakind;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectNew()
 *
 *      This function creates a new empty effect object with the given 
 *      property values.  It takes an already created effect definition
 *      object as an argument.  To add slots to the effect, call
 *      omfiEffectAddNewSlot().  To add renderings, call 
 *      omfiEffectSetFinalRender() or omfiEffectSetWorkingRender().
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
omfErr_t omfiEffectNew(
	omfHdl_t file,          /* IN - File Handle */
	omfDDefObj_t datakind,  /* IN - Datakind object */
	omfLength_t length,     /* IN - Length property value */
	omfEDefObj_t effectDef, /* IN - Effect Definition object */
	omfEffObj_t *newEffect) /* OUT - New Effect object */
{
	omfObject_t tmpEffect = NULL;

	*newEffect = NULL;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effectDef != NULL), file, OM_ERR_INVALID_EFFECTDEF);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		CHECK(omfsObjectNew(file, "EFFE", &tmpEffect));
		CHECK(ComponentSetNewProps(file, tmpEffect, length, datakind));
		CHECK(omfsWriteObjRef(file, tmpEffect, OMEFFEEffectKind, effectDef));
	  }

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiDatakindLookup()
 *
 *      Given a datakind uniquename (e.g., omfi:data:Picture), this function
 *      looks up the name in the file handle's datakind cache and returns
 *      a pointer to the datakind object.  If an object is not found, a
 *      NULL is returned through defObject argument and the function returns a
 *      FALSE.  The function will return TRUE when the object is found.
 *
 *      If an object is not found, omfiDatakindRegister() can be called to
 *      register a new datakind.
 *
 *      This function supports both 1.x and 2.x files.  For 1.x files, it
 *      will create a "dummy" datakind object if the requested datakind
 *      is either Picture, Sound, Edgecode or Timecode.  Otherwise, a
 *      NULL datakind object and FALSE will be returned.
 *
 * Argument Notes:
 *      defObject will be NULL and the function return value will be
 *      FALSE if a datakind object with the given name is not found.
 *      The error status will be returned through the omfError argument.
 *
 * ReturnValue:
 *		Boolean (as described above)
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfBool omfiDatakindLookup(
	omfHdl_t file,           /* IN - File Handle */
    omfUniqueNamePtr_t name, /* IN - Datakind Unique Name */
	omfDDefObj_t *defObject, /* OUT - Datakind object */
    omfErr_t     *omfError)  /* OUT - Error code */
{
    omfDDefObj_t tmpDef = NULL;
	omfUniqueName_t datakindName;
	char *header = "omfi:data:";
	static const int  fakeDDefs[3] = { 1, 2, 3 };
	
	*defObject = NULL;
	*omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE); 

	if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
	{
		if((strncmp(name, PICTUREKIND, OMUNIQUENAME_SIZE) == 0) ||
		   (strncmp(name, PICTWITHMATTEKIND, OMUNIQUENAME_SIZE) == 0) ||
		   (strncmp(name, COLORKIND, OMUNIQUENAME_SIZE) == 0) ||
		   (strncmp(name, MATTEKIND, OMUNIQUENAME_SIZE) == 0))
			*defObject = (omfDDefObj_t)&fakeDDefs[0];

		else if((strncmp(name, SOUNDKIND, OMUNIQUENAME_SIZE) == 0) ||
				(strncmp(name, STEREOKIND, OMUNIQUENAME_SIZE) == 0))
			*defObject = (omfDDefObj_t)&fakeDDefs[1];

		else if (strncmp(name, TIMECODEKIND, OMUNIQUENAME_SIZE) == 0)
			*defObject = (omfDDefObj_t)&fakeDDefs[2];

		else if (strncmp(name, EDGECODEKIND, OMUNIQUENAME_SIZE) == 0)
			*defObject = (omfDDefObj_t)&fakeDDefs[3];

		if (*defObject)
		  return(TRUE);
		else
		  {
/* NOTE: do we want to return an error here? */
#if 0
			*omfError = OM_ERR_INVALID_DATAKIND;
#endif
			return(FALSE);
		  }
	}
	
	/* Append "omfi:data" if name doesn't have it */
	if (strncmp(name, "omfi:data:", 10))
	  {
		strcpy(datakindName, header);
		strcat(datakindName, name);
	  }
	else
	  strcpy(datakindName, name);

	tmpDef = (omfDDefObj_t)omfsTableDefLookup(file->datakinds, 
											  datakindName);
	if (tmpDef)
	  {
		*defObject = tmpDef;
		return(TRUE);
	  }

/* NOTE: Should we return an error here? */
#if 0
	*omfError = OM_ERR_INVALID_DATAKIND;
#endif
	return(FALSE);
}

/*************************************************************************
 * Function: omfiEffectDefLookup()
 *
 *      Given an effect uniquename (e.g., omfi:effect:Dissolve), this 
 *      function looks up the name in the file handle's effectdef cache and
 *      returns a pointer to the effectdef object.  If an object is not
 *      found, a NULL is returned through the defObject argument and the
 *      function returns a FALSE.  The function will return TRUE when the
 *      object is found.
 *
 *      If an object is not found, omfiEffectDefRegister() can be called
 *      to register a new effect definition.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Boolean (as described above)
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfBool omfiEffectDefLookup(
	omfHdl_t file,           /* IN - File Handle */
    omfUniqueNamePtr_t name, /* IN - Effect Definition Unique Name */
	omfEDefObj_t *defObject, /* OUT - Effect Definition object */
	omfErr_t     *omfError)  /* OUT - Error code */
{
    omfEDefObj_t tmpDef = NULL;
	char *header = "omfi:effect:";
	omfUniqueName_t effectName;

	*defObject = NULL;
	*omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE);

	if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
	  {
		*omfError = OM_ERR_NOT_IN_15;
		return(FALSE);
	  }

	/* Append "omfi:effect" if name doesn't have it */
	if (strncmp(name, "omfi:effect:", 12))
	  {
		strcpy(effectName, header);
		strcat(effectName, name);
	  }
	else
	  strcpy(effectName, name);

	tmpDef = (omfEDefObj_t)omfsTableDefLookup(file->effectDefs, 
											  effectName);
	if (tmpDef)
	  {
		*defObject = tmpDef;
		return(TRUE);
	  }

	return(FALSE);
}


/*************************************************************************
 * Function: omfiEffectAddNewSlot()
 *
 *      This function creates a new effect slot object, inserts the
 *      segment passed as the "value" argument to the new slot, and
 *      adds the slot to the effect object.
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
omfErr_t omfiEffectAddNewSlot(
	omfHdl_t file,              /* IN - File Handle */
	omfEffObj_t effect,         /* IN - Effect object */
	omfArgIDType_t argID,       /* IN - Argument ID for the slot */
	omfSegObj_t value,          /* IN - Segment to place in effect slot */
	omfESlotObj_t *effectSlot)  /* OUT - Effect slot object */
{
	omfObject_t tmpSlot = NULL;
	omfLength_t effLen, valLen;
	omfBool objCreated = FALSE;
	omfDDefObj_t effDatakind = NULL, valDatakind = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	*effectSlot = NULL;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		if (file->semanticCheckEnable)
		  {
			CHECK(omfiComponentGetInfo(file, effect, NULL, &effLen));
			CHECK(omfiComponentGetInfo(file, value, NULL, &valLen));

			/* If !timewarp, verify that value is same length as the effect */
			if (!omfiEffectIsATimeWarp(file, effect, &omfError))
			  {
				if (omfError == OM_ERR_NONE)
				  {
					if (omfsInt64NotEqual(effLen, valLen))
						RAISE(OM_ERR_BAD_SLOTLENGTH);
				  }
				else
				  RAISE(omfError);
			  }
		  } /* semanticCheckEnable */

		CHECK(omfsObjectNew(file, "ESLT", &tmpSlot));
		objCreated = TRUE;

		CHECK(omfsWriteArgIDType(file, tmpSlot, OMESLTArgID, argID));
		CHECK(omfsWriteObjRef(file, tmpSlot, OMESLTArgValue, value));

		CHECK(omfsAppendObjRefArray(file, effect, OMEFFEEffectSlots, 
					   tmpSlot));
	  }

	XEXCEPT
	  {
		if (objCreated)
		  omfsObjectDelete(file, tmpSlot);
		return(XCODE());
	  }
	XEND;

	*effectSlot = tmpSlot;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectSetFinalRender()
 *
 *      This function sets the final rendering for the given effect
 *      to the input source clip.  Multiple renderings may exist if the
 *      source clip refers to a master mob that contains a media group.
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
omfErr_t omfiEffectSetFinalRender(
	omfHdl_t file,       /* IN - File Handle */
	omfEffObj_t effect,  /* IN - Effect object */
	omfSegObj_t sourceClip) /* IN - Source clip object */
{
	omfLength_t effLen, clipLen;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((sourceClip != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* Verify that render is a SCLP */
		if (!omfiIsASourceClip(file, sourceClip, &omfError))
		{
		  return(OM_ERR_NOT_SOURCE_CLIP);
		}

		/* Verify that the sourceClip is same length as the effect */
		CHECK(omfiComponentGetLength(file, effect, &effLen));
		CHECK(omfiComponentGetLength(file, sourceClip, &clipLen));
		if (omfsInt64NotEqual(effLen, clipLen))
		  {
			RAISE(OM_ERR_BAD_SLOTLENGTH);
		  }

		CHECK(omfsWriteObjRef(file, effect, OMEFFEFinalRendering, sourceClip));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectSetWorkingRender()
 *
 *      This function sets the final rendering for the given effect
 *      to the input source clip.  Multiple renderings may exist if the
 *      source clip refers to a master mob that contains a media group.
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
omfErr_t omfiEffectSetWorkingRender(
	omfHdl_t file,       /* IN - File Handle */
	omfEffObj_t effect,  /* IN - Effect object */
	omfSegObj_t sourceClip) /* IN - Source clip object */
{
	omfLength_t effLen, clipLen;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((sourceClip != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* Verify that render is a SCLP */
		if (!omfiIsASourceClip(file, sourceClip, &omfError))
		{
		  return(OM_ERR_NOT_SOURCE_CLIP);
		}

		/* Verify that the sourceClip is same length as the effect */
		CHECK(omfiComponentGetLength(file, effect, &effLen));
		CHECK(omfiComponentGetLength(file, sourceClip, &clipLen));
		if (omfsInt64NotEqual(effLen, clipLen))
		  {
			RAISE(OM_ERR_BAD_SLOTLENGTH);
		  }

		CHECK(omfsWriteObjRef(file, effect, OMEFFEWorkingRendering, 
				     sourceClip));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectSetBypassOverride()
 *
 *      This function sets the optional bypass override property on
 *      the given effect object.
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
omfErr_t omfiEffectSetBypassOverride(
	omfHdl_t file,       /* IN - File Handle */
	omfEffObj_t effect,  /* IN - Effect Object */
	omfArgIDType_t bypassOverride) /* IN - Bypass override */
{
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfsWriteArgIDType(file, effect, OMEFFEBypassOverride,
				    bypassOverride));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMediaGroupNew()
 *
 *      This function creates a new empty media group object with the given
 *      property values.  Media group choices should be added with the
 *      omfiMediaGroupAddChoice() function.
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
omfErr_t omfiMediaGroupNew(
    omfHdl_t file,            /* IN - File Handle */
	omfDDefObj_t datakind,    /* IN - Datakind object */
    omfLength_t length,       /* IN - Length property value */
    omfObject_t *mediaGroup)  /* OUT - New Media Group */
{
    omfObject_t tmpGroup = NULL;

    *mediaGroup = NULL;
  	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((datakind != NULL), file, OM_ERR_INVALID_DATAKIND);

	XPROTECT(file)
	  {
		CHECK(omfsObjectNew(file, "MGRP", &tmpGroup));
		CHECK(ComponentSetNewProps(file, tmpGroup, length, datakind));
	  }

	XEXCEPT
	  {
		omfsObjectDelete(file, tmpGroup);
		return(XCODE());
	  }
	XEND;

	*mediaGroup = tmpGroup;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMediaGroupStillFrame()
 *
 *      This function sets the still frame property on a media group
 *      to be the source clip passed as the stillFrame argument.
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
omfErr_t omfiMediaGroupSetStillFrame(
    omfHdl_t file,             /* IN - File Handle */
    omfObject_t mediaGroup,    /* IN - Media Group object */
    omfSegObj_t stillFrame)    /* IN - Still Frame source clip */
{
    omfLength_t groupLength, stillLength, oneLength;
	omfDDefObj_t stillDatakind = NULL, groupDatakind = NULL;
	omfUniqueName_t datakindName;
    omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((mediaGroup != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((stillFrame != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* Verify that stillFrame is a SCLP */
		if (!omfiIsASourceClip(file, stillFrame, &omfError))
		{
		  return(OM_ERR_NOT_SOURCE_CLIP);
		}

		if (file->semanticCheckEnable)
		  {
			CHECK(omfiComponentGetInfo(file, stillFrame, &stillDatakind,
									&stillLength));
			CHECK(omfiComponentGetInfo(file, mediaGroup, &groupDatakind,
									&groupLength));
			
			/* Verify that groups's datakind converts to still's datakind */
			CHECK(omfiDatakindGetName(file, groupDatakind, OMUNIQUENAME_SIZE, 
									  datakindName));
			if (!DoesDatakindConvertTo(file, stillDatakind, datakindName,
									   &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }

			/* Verify that length of still frame is 1 */
			omfsCvtInt32toInt64(1, &oneLength);
			if (omfsInt64NotEqual(oneLength, stillLength))
			  {
				RAISE(OM_ERR_STILLFRAME_BADLENGTH);
			  }
		  } /* semanticCheckEnable */

	    CHECK(omfsWriteObjRef(file, mediaGroup, OMMGRPStillFrame, 
								stillFrame));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMediaGroupAddChoice()
 *
 *      This function adds the source clip specified in the choice argument
 *      as a choice to the given media group object.
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
omfErr_t omfiMediaGroupAddChoice(
    omfHdl_t file,          /* IN - File Handle */
    omfObject_t mediaGroup, /* IN - Media Group object */
    omfSegObj_t choice)     /* IN - Source clip to add as a choice */
{
    omfLength_t groupLength, choiceLength;
	omfDDefObj_t groupDatakind = NULL, choiceDatakind = NULL;
	omfUniqueName_t datakindName;
    omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((mediaGroup != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((choice != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* Verify that choice is a SCLP */
		if (!omfiIsASourceClip(file, choice, &omfError))
		{
		  return(OM_ERR_NOT_SOURCE_CLIP);
		}

		if (file->semanticCheckEnable)
		  {
			CHECK(omfiComponentGetInfo(file, choice, &choiceDatakind, 
									&groupLength));
			CHECK(omfiComponentGetInfo(file, mediaGroup, &groupDatakind,
									&choiceLength));
			
			/* Verify that groups's datakind converts to still's datakind */
			CHECK(omfiDatakindGetName(file, groupDatakind, OMUNIQUENAME_SIZE, 
									  datakindName));
			if (!DoesDatakindConvertTo(file, choiceDatakind, datakindName,
									   &omfError))
			  {
				RAISE(OM_ERR_INVALID_DATAKIND);
			  }

			/* Verify that length of choice matches length of group */
			if (omfsInt64NotEqual(groupLength, choiceLength))
			  {
				RAISE(OM_ERR_BAD_LENGTH);
			  }
		  } /* semanticCheckEnable */

		CHECK(omfsAppendObjRefArray(file, mediaGroup, OMMGRPChoices, 
					   choice));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: AddMobTableEntry()
 *                   AddTrackMapping()
 *                   AddTrackMappingExact()
 *                   CvtTrackIDtoNum()
 *                   CvtTrackIDtoTrackType1X()
 *                   CvtTrackNumtoID()
 *                   MediaKindToTrackType()
 *                   TrackTypeToMediaKind()
 *
 *      These functions are used to support the internal mapping of track
 *      identifiers between 1.x and 2.x files.  With 1.x, the combination
 *      of the track label and the track kind (e.g., TT_PICTURE) uniquely
 *      identify the track within a mob.  In other words, it is legal to 
 *      have 2 tracks in the same mob with the following labels: V1, A1.
 *      With 2.x, the track labels must be unique: V1, A2.  To support
 *      backward compatibility a mapping of track labels is maintained
 *      internally in the file handle, and the correct label is written
 *      depending on the file version.  The public function interfaces
 *      support 2.x files, and the Toolkit assumes that track labels 
 *      starting with 1 are desired if a 1.x file is being written.
 *
 *      MediaKindToTrackType() and TrackTypeToMediaKind() support the
 *      backward compatibility of datakind objects.  Datakinds do not
 *      exist in 1.x, but they roughly correspond to track kinds.  Since
 *      the 2.x API needs to also support reading and writing 1.x files,
 *      these functions create dummy datakind objects for the known
 *      track kinds in 1.x (TT_PICTURE, TT_SOUND, TT_TIMECODE, and 
 *      TT_EDGECODE).  These dummy objects can then be passed in and
 *      out of the 2.x API calls.  The toolkit recognizes them to be
 *      1.x datakind objects and handles them accordingly.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
typedef struct
{
	omfInt16		trackNum1X;
	omfTrackType_t	trackType1X;
} trackMapEntry_t;

/*************************************************************************
 * Private Function: AddMobTableEntry()
 *************************************************************************/
omfErr_t AddMobTableEntry(
						  omfHdl_t file,       /* IN - File Handle */ 
						  omfObject_t mob,     /* IN - Input Mob */ 
						  omfUID_t mobID,      /* IN - Mob ID */
						  omTableDuplicate_t dup) /* IN - Allow duplicates */
{
	mobTableEntry_t	*entry;
	
	XPROTECT(file)
	  {
		entry = (mobTableEntry_t *)omOptMalloc(file, sizeof(mobTableEntry_t));
		entry->mob = mob;
		entry->map1x = NULL;
		if(file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
		  {
			CHECK(omfsNewTrackIDTable(file, 32, &entry->map1x));
		  }
	
		CHECK(omfsTableAddUID(file->mobs, mobID, entry, dup));
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: AddTrackMapping()
 *************************************************************************/
omfErr_t AddTrackMapping(
						 omfHdl_t file,          /* IN - File Handle */ 
						 omfUID_t mobID,         /* IN - Mob ID */
						 omfTrackID_t trackID,   /* IN - Track ID */
						 omfDDefObj_t mediaKind) /* IN - Media Kind */
{
	omfInt16				trackNum1X;
	omfInt32				trackType1X;
	omTableIterate_t	iter;
	omfBool				more;
	trackMapEntry_t		entry;
	mobTableEntry_t		*mobPtr;
	
	XPROTECT(file)
	{
		trackType1X = MediaKindToTrackType(file, mediaKind);
		mobPtr = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, mobID);
		if(mobPtr == NULL)
		  {
			RAISE(OM_ERR_MISSING_MOBID);
		  }
		
		if(omfmTableNumEntriesMatching(mobPtr->map1x, &trackID) == 0)
		{
			trackNum1X = 1;
			CHECK(omfsTableFirstEntry(mobPtr->map1x, &iter, &more));
			while(more)
			{
				memcpy(&entry, iter.valuePtr, sizeof(entry));
				if((entry.trackType1X == trackType1X) && 
				   (trackNum1X <= entry.trackNum1X))
					trackNum1X = (omfInt16)(entry.trackNum1X + 1);
				CHECK(omfsTableNextEntry(&iter, &more));
			}
			CHECK(AddTrackMappingExact(file, mobID, trackID, mediaKind, 
									   trackNum1X));
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: AddTrackMappingExact()
 *************************************************************************/
omfErr_t AddTrackMappingExact(
							  omfHdl_t file,        /* IN - File Handle */ 
							  omfUID_t mobID,       /* IN - Mob ID */
							  omfTrackID_t trackID, /* IN - 2.x Track ID */
							  omfDDefObj_t mediaKind,/* IN - Media Kind */
							  omfInt16	trackNum1X) /* IN - 1.x Track Label */
{
	trackMapEntry_t	mapEntry, oldEntry;
	omfBool			present;
	mobTableEntry_t	*mobPtr;

	XPROTECT(file)
	  {
		mobPtr = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, mobID);
		if(mobPtr == NULL)
		  {
			RAISE(OM_ERR_MISSING_MOBID);
		  }
		  
		CHECK(omfsTableTrackIDLookup(mobPtr->map1x, trackID, &oldEntry, 
									 sizeof(oldEntry), &present));
		mapEntry.trackNum1X = trackNum1X;
		mapEntry.trackType1X = (omfTrackType_t)MediaKindToTrackType(file, 
																	mediaKind);
		
		if(!present)
		  {
			CHECK(omfsTableAddTrackID(mobPtr->map1x, trackID, &mapEntry,
									  sizeof(mapEntry)));
		  }
		else
		  {
			if(oldEntry.trackNum1X != mapEntry.trackNum1X)
			  return(OM_ERR_TABLE_DUP_KEY);
			if(oldEntry.trackType1X != mapEntry.trackType1X)
			  return(OM_ERR_TABLE_DUP_KEY);
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
 * Private Function: CvtTrackIDtoNum()
 *************************************************************************/
omfErr_t CvtTrackIDtoNum(
						 omfHdl_t file,        /* IN - File Handle */ 
						 omfUID_t mobID,       /* IN - Mob ID */
						 omfTrackID_t trackID, /* IN - 2.x Track ID */
						 omfInt16 *trackNum1X) /* OUT - 1.x Track Label */
{
	mobTableEntry_t	*mobPtr;
	trackMapEntry_t	oldEntry;
	omfBool			present;

	XPROTECT(file)
	  {

		mobPtr = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, mobID);
		if(mobPtr == NULL)
		  {
			RAISE(OM_ERR_MISSING_MOBID);
		  }
		
		CHECK(omfsTableTrackIDLookup(mobPtr->map1x, trackID, &oldEntry, 
									 sizeof(oldEntry), &present));
		if(!present)
		  {
			RAISE(OM_ERR_MISSING_TRACKID);
		  }
	
		*trackNum1X = oldEntry.trackNum1X;

	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);

}

/*************************************************************************
 * Private Function: CvtTrackIDtoTrackType1X()
 *************************************************************************/
omfErr_t CvtTrackIDtoTrackType1X(
                    omfHdl_t file,               /* IN - File Handle */ 
					omfUID_t mobID,              /* IN - Mob ID */
					omfTrackID_t trackID,        /* IN - 2.x Track ID */
					omfTrackType_t *trackType1X) /* OUT - 1.x track type */
{
	mobTableEntry_t	*mobPtr;
	trackMapEntry_t	oldEntry;
	omfBool			present;

	XPROTECT(file)
	  {
		mobPtr = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, mobID);
		if(mobPtr == NULL)
		  {
			RAISE(OM_ERR_MISSING_MOBID);
		  }
		
		CHECK(omfsTableTrackIDLookup(mobPtr->map1x, trackID, &oldEntry, 
									 sizeof(oldEntry), &present));
		if(!present)
		  {
			RAISE(OM_ERR_MISSING_TRACKID);
		  }

		*trackType1X = oldEntry.trackType1X;
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: CvtTrackNumtoID()
 *************************************************************************/
omfErr_t CvtTrackNumtoID(
						 omfHdl_t file,        /* IN - File Handle */ 
						 omfUID_t mobID,       /* IN - Mob ID */
						 omfInt16 trackNum1X,  /* IN - 1.x Track Label */
						 omfInt16 trackType1X, /* IN - 1.x Track Type */
						 omfTrackID_t *trackID)/* OUT - 2.x Track ID */
{
    mobTableEntry_t	*mobPtr;
	trackMapEntry_t	mapEntry;
	omfBool			found;
	
	XPROTECT(file)
	  {
		mobPtr = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, mobID);
		if(mobPtr == NULL)
		  {
			RAISE(OM_ERR_MISSING_MOBID);
		  }
		
		mapEntry.trackNum1X = trackNum1X;
		mapEntry.trackType1X = trackType1X;	
		CHECK(omfsTableSearchDataValue(mobPtr->map1x, sizeof(mapEntry), 
					   &mapEntry, sizeof(omfTrackID_t), trackID, &found));
		if(!found)
			RAISE(OM_ERR_MISSING_TRACKID);
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;
 		
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: MediaKindToTrackType()
 *************************************************************************/
omfInt32 MediaKindToTrackType(
							  omfHdl_t file,       /* IN - File Handle */ 
							  omfDDefObj_t mediaKind) /* IN - Data Kind */
{
	omfInt32			type;
	omfErr_t		status = OM_ERR_NONE;
	omfDDefObj_t	timecodeKind, edgecodeKind;
	
	if (mediaKind == file->pictureKind)
	  type = TT_PICTURE;
	else if (mediaKind == file->soundKind)
	  type = TT_SOUND;
	else
	  {
		omfiDatakindLookup(file, TIMECODEKIND, &timecodeKind, &status);
		omfiDatakindLookup(file, EDGECODEKIND, &edgecodeKind, &status);
		if (mediaKind == timecodeKind)
		  type = TT_TIMECODE;
		else if (mediaKind == edgecodeKind)
		  type = TT_EDGECODE;
		else
		  type = TT_NULL;
	  }
		
	return(type);
}

/*************************************************************************
 * Private Function: TrackTypeToMediaKind()
 *************************************************************************/
omfDDefObj_t TrackTypeToMediaKind(
								  omfHdl_t file,       /* IN - File Handle */ 
								  omfInt32 trackType)  /* OUT - Track Type */
{
	omfDDefObj_t	mediaKind;
	omfErr_t		status = OM_ERR_NONE;
	omfDDefObj_t	timecodeKind, edgecodeKind;

	if (trackType == TT_PICTURE)
	  mediaKind = file->pictureKind;
	else if (trackType == TT_SOUND)
	  mediaKind = file->soundKind;
	else
	  {
		omfiDatakindLookup(file, TIMECODEKIND, &timecodeKind, &status);
		omfiDatakindLookup(file, EDGECODEKIND, &edgecodeKind, &status);

		if (trackType == TT_TIMECODE)
		  mediaKind = timecodeKind;
		else if (trackType == TT_EDGECODE)
		  mediaKind = edgecodeKind;
		else
		  mediaKind = file->nilKind;
	  }
		
	return(mediaKind);
}



/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
