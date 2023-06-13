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
 * Name: omFndSrc.c
 *
 * Overall Function: Contains public and supporting function for
 *                   functionality to iterate through complex sequences
 *
 * Audience: Clients writing or reading OMFI compositions.
 *
 * Public Functions:
 *     omfiMobOpenSearch() -  Open a search iterator on a particular track of a sequence
 *     omfiMobCloseSearch() -  Close a search iterator
 *     omfiMobGetNextSource() - Go to the next value using a search iterator
 *     omfiMobGetThisSource() - Go more information from the current value
 *     omfiMobSearchSource() - The replacement for omfiMobFindSource().  No longer
 *								accepts sequences
 *
 * Obsoleted functions, still present but will be removed:
 *     omfiMobFindSource()
 *
 * General error codes returned:
 *      OM_ERR_NONE
 *             Success
 * 		OM_ERR_BAD_FHDL
 *				Bad file handle passed in
 * 		OM_ERR_BENTO_PROBLEM
 *				Bento returned an error, check BentoErrorNumber.
 *      OM_ERR_NULLOBJECT
 *              Null object passed in as an argument
 *      OM_ERR_INVALID_DATAKIND
 *              An invalid datakind was passed in
 *      OM_ERR_NULL_PARAM
 *              A required parameter was NULL
 *      OM_ERR_NOT_IN_20
 *              This function is not supported in 2.0 (GetSequLength)
 *      OM_ERR_INVALID_OBJ
 *              An object of the wrong class was passed in
 *      OM_ERR_INTERN_TOO_SMALL
 *              Buffer size is too small (GetDataValue)
 *      OM_ERR_MOB_NOT_FOUND
 *              A source clip references a mob that is not in the file
 *      OM_ERR_REQUIRED_POSITIVE
 *              The value (e.g., length) must be positive)
 *      OM_ERR_TRAVERSAL_NOT_POSS
 *              An attempt to traverse a mob failed
 *      OM_ERR_TRACK_NOT_FOUND
 *              A source clip references a track that does not exist in a mob
 *      OM_ERR_NO_MORE_MOBS
 *              The end of the mob chain was reached
 *      OM_ERR_INVALID_MOBTYPE
 *              This operation does not apply to this mob type
 *      OM_ERR_NOMEMORY
 *              We ran out of memory (omfsMalloc failed)
 *      OM_ERR_ITER_WRONG_TYPE
 *              An iterator of the wrong type was passed in
 *      OM_ERR_INVALID_SEARCH_CRIT
 *              This search criteria does not apply to this iterator
 *      OM_ERR_INTERNAL_ITERATOR
 *              An internal iterator error occurred
 */

#include "masterhd.h"
#include <stdlib.h> /* for abs() */

#include "omPublic.h"
#include "omPvt.h"
#include "omMedia.h" 

/* Moved math.h down here to make NEXT's compiler happy */
#include <math.h>

/* NOTE: this should have its own error code */
#define omfAssertMobFindHdl(hdl) \
	if((hdl == NULL) || (hdl->cookie != MOBFIND_COOKIE)) \
           return(OM_ERR_BAD_SRCH_ITER)

/*********/
/* Types */
/*********/
#define MOBFIND_COOKIE		0x4D4F4246	/* 'MOBF' */
#define TRACE 0
#define SCOPEBLOCKS 10

/*******************************/
/* Static Function Definitions */
/*******************************/
static omfErr_t omfiMobFindSourceRecurs(
    omfHdl_t file,           /* IN - File Handle */
	omfMobObj_t rootMob,     /* IN - Root Mob to search */
	omfTrackID_t trackID,    /* IN - Track ID to search */
	omfPosition_t offset,    /* IN - Offset into Track */
	omfMobKind_t mobKind,    /* IN - Kind of mob to look for */
	omfMediaCriteria_t *mediaCrit, /* IN - Media Criteria, if find media group*/
	omfUID_t *mobID,         /* OUT - Found mob's Mob ID */
	omfTrackID_t *mobTrack,  /* OUT - Found Mob's Track */
	omfPosition_t *mobOffset,/* OUT - Found Mob's Track offset */
	omfLength_t *minLength,  /* OUT - Minimum clip length found in chain */
	omfMobObj_t *mob,        /* OUT - Pointer to found mob */
	omfBool		*foundLength);

omfErr_t MobFindCpntByPosition(
    omfHdl_t    file,       /* IN - File Handle */
	omfObject_t mob,        /* IN - Mob object */
	omfTrackID_t trackID,   /* IN - Track ID of searched track */
    omfObject_t rootObj,    /* IN - Current Root of tree in the track */
	omfPosition_t startPos, /* IN - Current Start Position in track */
	omfPosition_t position, /* IN - Desired Position */
    omfMediaCriteria_t *mediaCrit, /* IN - Media criteria (if mediagroup) */
	omfPosition_t     *diffPos, /* OUT - Difference between the beginning
								 *       of the segment and the desired pos 
								 */
    omfObject_t *foundObj,      /* OUT - Object found that desired position */
    omfLength_t *foundLen);     /* OUT - Length of found object, provided
                                         in case segment was incoming or
                                         outgoing segment of a transition */

omfErr_t SequencePeekNextCpnt(omfIterHdl_t iterHdl, 
							  omfObject_t sequence,
							  omfObject_t *component,
							  omfPosition_t *offset);

static omfErr_t IteratorCopy(
	omfIterHdl_t inIter,   /* IN - Iterator to copy */ 
    omfIterHdl_t *outIter);  /* IN - Iterator to copy to */

static omfErr_t MobFindLeaf( 
					 omfHdl_t file,
					 omfMobObj_t mob,
					 omfMSlotObj_t track,
					 omfMediaCriteria_t *mediaCrit,
					 omfEffectChoice_t *effectChoice,
					 omfObject_t rootObj,
					 omfPosition_t rootPos,
					 omfLength_t rootLen,
					 omfObject_t prevObject,
					 omfObject_t nextObject,
					 omfScopeStack_t scopeStack,
					 omfPosition_t currentObjPos,
					 omfObject_t *foundObj,
					 omfLength_t *minLength,
					 omfBool *isTransition,
					 omfObject_t *effeObject,
					 omfInt32	*nestDepth,
					 omfPosition_t *diffPos);

static omfErr_t FindNextMob(omfHdl_t file, 
					 omfMobObj_t mob, 
					 omfObject_t track, 
					 omfObject_t segment,
					 omfLength_t length,
					 omfPosition_t diffPos,
					 omfMobObj_t *retMob,
					 omfTrackID_t *retTrackID,
					 omfPosition_t *retPos,
					 omfObject_t *pulldownObj,
					 omfInt32 *pulldownPhase,
					 omfLength_t *retLen);

static omfErr_t MobFindSource(
					   omfHdl_t file,
					   omfMobObj_t mob,
					   omfTrackID_t trackID,
					   omfPosition_t offset,
					   omfLength_t length,
					   omfMobKind_t mobKind,
					   omfMediaCriteria_t *mediaCrit,
					   omfEffectChoice_t *effectChoice,
					   omfFindSourceInfo_t *sourceInfo,
					   omfBool *foundSource);

static omfErr_t SetupTransitionInfo(omfMobFindHdl_t hdl);

static omfErr_t ScopeStackAlloc(omfHdl_t file,
						omfScopeStack_t *scopeStack);
static omfErr_t ScopeStackDispose(omfHdl_t file,
						omfScopeStack_t scopeStack);
static omfErr_t ScopeStackPop(omfHdl_t file,
					   omfScopeStack_t scopeStack,
					   omfInt32 numPops,
					   omfScopeInfo_t *scopeInfo);
static omfErr_t ScopeStackPeek(omfHdl_t file,
					   omfScopeStack_t scopeStack,
					   omfInt32 numPops,
					   omfScopeInfo_t *scopeInfo);
static omfErr_t ScopeStackPush(omfHdl_t file,
						omfScopeStack_t scopeStack,
						omfScopeInfo_t scopeInfo);

static omfErr_t IteratorClone(
	omfIterHdl_t inIter,   /* IN - Iterator to copy */ 
    omfIterHdl_t *outIter);  /* IN - Iterator to copy to */

static omfErr_t IteratorDisposeClone(
	omfHdl_t file,        /* IN - File Handle */
	omfIterHdl_t iterHdl); /* IN/OUT - Iterator handle */

static omfErr_t SequPeekNextCpnt(omfIterHdl_t iterHdl,
							  omfObject_t sequence,
							  omfObject_t *component,
							  omfPosition_t *offset,
							  omfPosition_t *length);

static omfBool IsANagraMob(omfHdl_t file,
						omfMobObj_t mob,
						omfErr_t *omfError);

static omfErr_t FindTrackAndSegment(omfHdl_t file,
							 omfMobObj_t mob,
							 omfTrackID_t trackID,
							 omfPosition_t offset,
							 omfObject_t *track,
							 omfObject_t *segment,
							 omfRational_t *srcRate,
							 omfPosition_t *diffPos);

/*******************************/
/* Functions                   */
/*******************************/

omfErr_t omfiMobOpenSearch(omfHdl_t file,
	omfMobObj_t mob,    	
	omfObject_t track,   
	omfMobFindHdl_t *mobFindHdl)
{
  omfMobFindHdl_t tmpHdl = NULL;
  omfInt32 loop, numSlots, currSlotIndex = 0;
  omfObject_t tmpTrack = NULL, tmpSlot = NULL;
  omfScopeInfo_t scopeInfo;
  omfPosition_t zeroPos = omfsCvtInt32toPosition(0, zeroPos);
  omfErr_t omfError = OM_ERR_NONE;

  *mobFindHdl = NULL;
  omfAssertValidFHdl(file);

  XPROTECT(file)
	{
	  tmpHdl = (omfMobFindHdl_t)omOptMalloc(file, sizeof(struct omfiMobFind));
	  if (tmpHdl == NULL)
		{
		  RAISE(OM_ERR_NOMEMORY);
		}
	  tmpHdl->cookie = MOBFIND_COOKIE;
	  tmpHdl->file = file;
	  CHECK(omfsGetHeadObject(tmpHdl->file, &tmpHdl->head));
	  tmpHdl->mob = mob;
	  tmpHdl->track = track;

	  tmpHdl->isSequence = FALSE;  /* Initialize this, it's checked later */
	  tmpHdl->sequence = NULL;

	  tmpHdl->currentIndex = 1;
	  tmpHdl->currentObj = NULL;
	  omfsCvtInt32toInt64(0, &tmpHdl->currentPos);
	  omfsCvtInt32toInt64(0, &tmpHdl->currentObjLen);
	  omfsCvtInt32toInt64(0, &tmpHdl->currentObjPos);
 	  tmpHdl->searchCrit = NULL;
 	  tmpHdl->mediaCrit = NULL;

	  tmpHdl->prevObject = NULL;
	  tmpHdl->prevTransition = FALSE;
	  omfsCvtInt32toInt64(0, &tmpHdl->prevOverlap);

	  tmpHdl->nextObject = NULL;
	  omfsCvtInt32toInt64(0, &tmpHdl->nextLength);
	  omfsCvtInt32toInt64(0, &tmpHdl->nextPosition);
	  omfsCvtInt32toInt64(0, &tmpHdl->effeNextPos);
		tmpHdl->prevNestDepth = 0;

	  CHECK(omfiTrackGetInfo(tmpHdl->file, 
							 tmpHdl->mob, 
							 tmpHdl->track, 
							 NULL, 0, NULL, NULL, NULL,
							 &tmpHdl->segment));

	  /*** If 2.0, push the mob onto the scope stack as the first scope ***/
	  if (file->setrev == kOmfRev2x)
		{
		  ScopeStackAlloc(file, &tmpHdl->scopeStack);
		  CHECK(omfiMobGetNumTracks(file, mob, &numSlots));
		  for (loop = 1; loop <= numSlots; loop++)
			{
			  CHECK(omfsGetNthObjRefArray(file, mob, OMMOBJSlots,
										  &tmpTrack, loop));
			  if (tmpTrack == track)	/* JeffB: Fixed = to == */
				{
				  currSlotIndex = loop;
				  break;
				}
			}
		  scopeInfo.scope = mob;
		  scopeInfo.numSlots = numSlots;
		  scopeInfo.scopeOffset = zeroPos;
		  scopeInfo.currSlotIndex = currSlotIndex;
		  ScopeStackPush(file, tmpHdl->scopeStack, scopeInfo);
		}
	  else
		tmpHdl->scopeStack = NULL;

	  /* FIND FIRST COMPONENT ON TRACK */
	  if (omfiIsASequence(tmpHdl->file, tmpHdl->segment, &omfError))
		{
		  tmpHdl->isSequence = TRUE;
		  tmpHdl->sequence = tmpHdl->segment;
		  CHECK(omfiSequenceGetNumCpnts(tmpHdl->file,
										tmpHdl->sequence,
										&tmpHdl->numSegs));
		  CHECK(omfiIteratorAlloc(tmpHdl->file, &tmpHdl->sequIter));
		}

	  /* Also treat a top level nested scope as a sequence, if it contains
	   * a sequence.  This is a common Media Composer OMF 2.0 structure.
	   */
	  else if (omfiIsANestedScope(tmpHdl->file, tmpHdl->segment, &omfError))
		{
		  CHECK(omfiNestedScopeGetNumSlots(file, tmpHdl->segment, &numSlots));
		  CHECK(omfsGetNthObjRefArray(file, tmpHdl->segment, OMNESTSlots,
									  &tmpSlot, numSlots));
		  if (omfiIsASequence(tmpHdl->file, tmpSlot, &omfError))
			{
			  tmpHdl->isSequence = TRUE;
			  tmpHdl->sequence = tmpSlot;

			  if (file->setrev == kOmfRev2x)
				{
				  scopeInfo.scope = tmpHdl->segment;
				  scopeInfo.numSlots = numSlots;
				  scopeInfo.scopeOffset = zeroPos;
				  scopeInfo.currSlotIndex = numSlots;
				  ScopeStackPush(file, tmpHdl->scopeStack, scopeInfo);
				}

			  CHECK(omfiSequenceGetNumCpnts(tmpHdl->file,
											tmpHdl->sequence,
											&tmpHdl->numSegs));
			  CHECK(omfiIteratorAlloc(tmpHdl->file, &tmpHdl->sequIter));
			}

		  /* Else, no nested sequence so handle below */
		}

	  /* If not a sequence */
	  if (!tmpHdl->isSequence)
		{
		  tmpHdl->isSequence = FALSE;
		  tmpHdl->numSegs = 1;
		  tmpHdl->sequIter = NULL;
		  tmpHdl->currentObj = tmpHdl->segment;
		}
	} /* XPROTECT */
  XEXCEPT
	{
		/* Assume don't have to free if malloc failed */
		if ((XCODE() != OM_ERR_NOMEMORY) && tmpHdl)
		  omOptFree(file, tmpHdl);
		return(XCODE());
	  }
  XEND;
  *mobFindHdl = tmpHdl;
  return(OM_ERR_NONE);
}

omfErr_t omfiMobCloseSearch(omfHdl_t file,
							omfMobFindHdl_t mobFindHdl)
{
  if (mobFindHdl)
	{
	  if (mobFindHdl->scopeStack)
		ScopeStackDispose(file, mobFindHdl->scopeStack);

	  if (mobFindHdl->sequIter)
		omfiIteratorDispose(file, mobFindHdl->sequIter);
	  mobFindHdl->cookie = 0;
	  omOptFree(file, mobFindHdl);
	}
  mobFindHdl = NULL;
  return(OM_ERR_NONE);
}

static omfErr_t IteratorClone(
	omfIterHdl_t inIter,   /* IN - Iterator to copy */ 
    omfIterHdl_t *outIter)  /* IN - Iterator to copy to */
{
	(*outIter)->cookie = inIter->cookie;
	(*outIter)->iterType = inIter->iterType;
	(*outIter)->currentIndex = inIter->currentIndex;
	(*outIter)->file = inIter->file;
	(*outIter)->maxIter = inIter->maxIter;
	(*outIter)->maxMobs = inIter->maxMobs;
	(*outIter)->head = inIter->head;
	(*outIter)->searchCrit.searchTag = inIter->searchCrit.searchTag;
	(*outIter)->mdes = inIter->mdes;
	(*outIter)->currentObj = inIter->currentObj;
	(*outIter)->currentProp = inIter->currentProp;
	(*outIter)->nextIterator = inIter->nextIterator;
	(*outIter)->masterMob = inIter->masterMob;
	(*outIter)->masterTrack = inIter->masterTrack;
	(*outIter)->prevTransition = inIter->prevTransition;
	(*outIter)->prevLength = inIter->prevLength;
	(*outIter)->prevOverlap = inIter->prevOverlap;
	(*outIter)->position = inIter->position;
	(*outIter)->tableIter = inIter->tableIter;

	return(OM_ERR_NONE);
}

static omfErr_t IteratorDisposeClone(
	omfHdl_t file,        /* IN - File Handle */
	omfIterHdl_t iterHdl) /* IN/OUT - Iterator handle */
{
	omfAssertValidFHdl(file);

	/* Be careful not to destroy links into original mob that this was
	 * cloned from.  Just delete the iterator, not the tables and other
	 * iterators that it refers to.
	 */
	
	/* Free iterHdl itself */
	if (iterHdl)
		omOptFree(file, iterHdl);
	iterHdl = NULL;

	return(OM_ERR_NONE);
}

omfErr_t SequPeekNextCpnt(omfIterHdl_t iterHdl,
							  omfObject_t sequence,
							  omfObject_t *component,
							  omfPosition_t *offset,
							  omfPosition_t *length)
{
  omfIterHdl_t tmpIter = NULL;

  XPROTECT(iterHdl->file)
	{
	  CHECK(omfiIteratorAlloc(iterHdl->file, &tmpIter));
	  IteratorClone(iterHdl, &tmpIter);
	  CHECK(omfiSequenceGetNextCpnt(tmpIter, sequence, NULL, 
									offset, component));
	  if (*component)
		omfiComponentGetLength(iterHdl->file, *component, length);
	  else
		omfsCvtInt32toLength(0, (*length));
	  IteratorDisposeClone(iterHdl->file, tmpIter);
	} /* XPROTECT */
  XEXCEPT
	{
	  if(tmpIter != NULL)
		IteratorDisposeClone(iterHdl->file, tmpIter);
	}
  XEND;
  return(OM_ERR_NONE);
}

omfErr_t omfiMobGetNextSource(
    omfMobFindHdl_t hdl,       /* IN */
	omfMobKind_t mobKind,             /* IN */
	omfMediaCriteria_t *mediaCrit,    /* IN */
	omfEffectChoice_t *effectChoice,  /* IN */
	omfCpntObj_t *thisCpnt,           /* OUT */
	omfPosition_t *thisPosition,      /* OUT */
	omfFindSourceInfo_t *sourceInfo, /* OUT */
	omfBool *foundTransition)        /* OUT */
{
  omfLength_t zeroLen = omfsCvtInt32toLength(0, zeroLen);
  omfPosition_t zeroPos = omfsCvtInt32toPosition(0, zeroPos);
  omfLength_t cpntLen = omfsCvtInt32toLength(0, cpntLen);
  omfLength_t diffPos = omfsCvtInt32toPosition(0, diffPos);
  omfObject_t leafObj = NULL;
  omfMobObj_t nextMob = NULL;
  omfFindSourceInfo_t tmpSourceInfo;
  omfBool sourceFound = FALSE, sameSegment = FALSE, foundTran = FALSE;
  omfPosition_t nextPos, endPos;
  omfTrackID_t nextTrackID;
  omfLength_t nextLen, newLen, minLength;
  omfErr_t omfError = OM_ERR_NONE;
  omfObject_t	pulldownObj, effeObject;
  omfInt32		nestDepth = 0, pulldownPhase;

	if(omfsInt64NotEqual(zeroPos, hdl->effeNextPos))
	{
		hdl->currentPos = hdl->effeNextPos;
	}
	omfsCvtInt32toInt64(0, &hdl->effeNextPos);

  /* Initialize outputs */
  if (thisCpnt)
	*thisCpnt = NULL;
  if (thisPosition)
	omfsCvtInt32toPosition(0, (*thisPosition));
  if (sourceInfo)
	{
	  (*sourceInfo).mob = NULL;
	  (*sourceInfo).mobTrackID = 0;
	  (*sourceInfo).editrate.numerator = 0;
	  (*sourceInfo).editrate.denominator = 1;
	  (*sourceInfo).filmTapePdwn = NULL;
	  (*sourceInfo).tapeFilmPdwn = NULL;
	  (*sourceInfo).effeObject = NULL;
	  omfsCvtInt32toLength(0, (*sourceInfo).position);
	  omfsCvtInt32toLength(0, (*sourceInfo).minLength);
	}
  if (foundTransition)
	*foundTransition = FALSE;

  omfAssertMobFindHdl(hdl); 

  XPROTECT(hdl->file)
	{
	  /* NOTE: Save current state in case of error? */

	  /* If we're not finished processing this segment, continue */
	  endPos = hdl->currentObjPos;
	  CHECK(omfsAddInt64toInt64(hdl->currentObjLen, &endPos));
 	  if (omfsInt64Less(hdl->currentObjPos, hdl->currentPos) &&
		  omfsInt64Less(hdl->currentPos, endPos))
		{
		  sameSegment = TRUE;
		}

	  /* Only step through the sequence, if we're not finished with
	   * this segment.
	   */
	  if (!sameSegment)
		{
		  if (hdl->isSequence)
			{
			  /* Initialize previous pointers */
			  hdl->prevObject = hdl->currentObj;
			  if (hdl->prevObject &&
				  omfiIsATransition(hdl->file, hdl->prevObject, &omfError))
				hdl->prevTransition = TRUE;
			  else
				hdl->prevTransition = FALSE;  /* Reset */

			  /* Skip zero-length clips, sometimes found in MC files */
			  while (omfsInt64Equal(cpntLen, zeroLen))
				{
				  CHECK(omfiSequenceGetNextCpnt(hdl->sequIter,
												hdl->sequence,
												NULL, /* search criteria */
												&hdl->currentPos,
												&hdl->currentObj));
				hdl->currentObjPos = hdl->currentPos;
				  if (thisCpnt)
					*thisCpnt = hdl->currentObj;
				  CHECK(omfiComponentGetLength(hdl->file, hdl->currentObj, 
										   &cpntLen));
				}
			  if (!hdl->prevTransition)
				hdl->currentObjLen = cpntLen;
			  else /* previous transition */
				{
				  /* If a transition preceeds the clip, the offset points to 
				   * the beginning of the exposed clip.
				   */
				  diffPos = cpntLen;
				  omfsSubInt64fromInt64(hdl->nextLength, &diffPos);
				  hdl->currentObjLen = hdl->nextLength;
				}
				
			  /** Peek ahead at the next object **/
			  omfError = SequPeekNextCpnt(hdl->sequIter, hdl->sequence,
										  &hdl->nextObject, &hdl->nextPosition,
										  &hdl->nextLength);
			  if (omfError == OM_ERR_NONE)
				{
				  /* If this object, the previous, or the next object are 
				   * transitions, figure out new segment lengths, overlaps...
				   */
				  SetupTransitionInfo(hdl);
				}
			  else if (omfError != OM_ERR_NO_MORE_OBJECTS)
				{
				  RAISE(omfError);
				}
			}
		  else /* Only one component */
			{
			  /* If object was returned last time, we're out of objects */
			  if (hdl->prevObject)
				{
				  RAISE(OM_ERR_NO_MORE_OBJECTS);
				}
			  hdl->prevObject = hdl->currentObj;
		      CHECK(omfiComponentGetLength(hdl->file, hdl->currentObj, 
										  &hdl->currentObjLen));
			}

		}  /* If getting next segment in sequence */


		/* !!! Change this to detect change in nesting
		 * This only works if we only walked down ONE level
		 */
		if(hdl->prevNestDepth != 0)
		{
		 	 omfScopeInfo_t		info;
		 	 
		 	 CHECK(ScopeStackPop(hdl->file, hdl->scopeStack, 1, &info));
		}

	  /*** Find leaf object in this track that points to the next mob ***/
	  CHECK(MobFindLeaf(hdl->file, hdl->mob, hdl->track, 
						hdl->mediaCrit, effectChoice,
						hdl->currentObj, hdl->currentPos, hdl->currentObjLen,
						hdl->prevObject, hdl->nextObject,
						hdl->scopeStack, hdl->currentObjPos,
						&leafObj, &minLength, foundTransition, &effeObject,
						&nestDepth, &diffPos));

 	  if (omfsInt64Less(minLength, hdl->currentObjLen))
		{
		  newLen = minLength;
		}
	  else
		newLen = hdl->currentObjLen;

	  /*** Find the next mob, factoring in mask object edit rate conversions,
	   *** and 1.0 track mappings.
	   ***/
	  CHECK(FindNextMob(hdl->file, hdl->mob, hdl->track, leafObj, 
						newLen, diffPos,
						&nextMob, &nextTrackID, &nextPos, &pulldownObj, &pulldownPhase, &nextLen));

	  if(pulldownObj != NULL)
	  {
		omfPulldownDir_t	direction;

	  	if ((hdl->file->setrev == kOmfRev1x) || (hdl->file->setrev == kOmfRevIMA))
		{
			omfBool		isDouble;
			
			CHECK(omfsReadBoolean(hdl->file, pulldownObj, OMMASKIsDouble, &isDouble));
			direction = (isDouble == 0 ? kOMFFilmToTapeSpeed : kOMFTapeToFilmSpeed);
		}
	  	else
		{
			CHECK(omfsReadPulldownDirectionType(hdl->file, pulldownObj, OMPDWNDirection, &direction));
		}
		if(direction == kOMFFilmToTapeSpeed)	/* kOMFFilmToTapeSpeed */
		{
			(*sourceInfo).filmTapePdwn = pulldownObj;
			(*sourceInfo).filmTapePhase = pulldownPhase;
		}
		else				/* kOMFTapeToFilmSpeed */
		{
			(*sourceInfo).tapeFilmPdwn = pulldownObj;
			(*sourceInfo).tapeFilmPhase = pulldownPhase;
		}
	 } 

	  /*** Find component at referenced position in new mob ***/
	  CHECK(MobFindSource(hdl->file, 
						  nextMob, nextTrackID, nextPos, nextLen,
						  mobKind, mediaCrit, effectChoice,
						  &tmpSourceInfo, &sourceFound));
	  if (sourceFound)
		{
		  (*sourceInfo).mob = tmpSourceInfo.mob;
		  (*sourceInfo).mobTrackID = tmpSourceInfo.mobTrackID;
		  (*sourceInfo).position = tmpSourceInfo.position;
		  (*sourceInfo).editrate = tmpSourceInfo.editrate;
		  (*sourceInfo).minLength = tmpSourceInfo.minLength;
	  	  if(tmpSourceInfo.filmTapePdwn != NULL)
	  	  	(*sourceInfo).filmTapePdwn = tmpSourceInfo.filmTapePdwn;
	  	  if(tmpSourceInfo.tapeFilmPdwn != NULL)
		  	  (*sourceInfo).tapeFilmPdwn = tmpSourceInfo.tapeFilmPdwn;
		  if (thisCpnt)
			*thisCpnt = hdl->currentObj;
		  if (thisPosition)
			*thisPosition = hdl->currentPos;

		  /* Now increment currentPos for next iteration */
		  CHECK(omfsAddInt64toInt64(minLength, &hdl->currentPos));
			if(!sameSegment)
				hdl->currentObjPos = hdl->currentPos;
		}
	  else /* Failure - null  out return values */
		{
		  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
		}
		hdl->prevNestDepth = nestDepth;
	}
  XEXCEPT
	{
	  /* If fill, increment currentPos for next iteration */
	  if(XCODE() == OM_ERR_FILL_FOUND)
		{
		  omfsAddInt64toInt64(minLength, &hdl->currentPos);
			if(!sameSegment)
				hdl->currentObjPos = hdl->currentPos;
		  /* Roger add propagation of length here*/
		  (*sourceInfo).minLength = newLen;  
		}
	  if(XCODE() == OM_ERR_PARSE_EFFECT_AMBIGUOUS)
		{
		    omfError = omfiComponentGetLength(hdl->file, effeObject, &minLength);
		    if(omfError != OM_ERR_NONE)
		    	RERAISE(omfError);
		   	hdl->effeNextPos = hdl->currentPos;
			hdl->prevNestDepth = nestDepth;
		  	omfsAddInt64toInt64(minLength, &hdl->effeNextPos);
		  	if(sourceInfo != NULL)
		  	{
		  		(*sourceInfo).effeObject = effeObject;
				(*sourceInfo).minLength = minLength;
	  		}
	  	}
	if (thisPosition)
		*thisPosition = hdl->currentPos;
	}
  XEND;

  return(OM_ERR_NONE);
}

static omfErr_t SetupTransitionInfo(omfMobFindHdl_t hdl)
{
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(hdl->file)
	{
	  /* Is the next component a transition? */
	  if (hdl->nextObject &&
		  omfiIsATransition(hdl->file, hdl->nextObject, &omfError))
		{
		  /* Subtract the length of the transition from the length of
		   * the current object, to get the exposed clip length
		   */
		  CHECK(omfsSubInt64fromInt64(hdl->nextLength, &hdl->currentObjLen));
		}

	  /* Is the current component a transtion? */
	  else if (hdl->nextObject && 
			   omfiIsATransition(hdl->file, hdl->currentObj, &omfError))
		{
		  /* Subtract the length of the transition from the length of
		   * the following object, to get the exposed clip length
		   */
		  CHECK(omfsSubInt64fromInt64(hdl->currentObjLen, &hdl->nextLength));
		}
	} /* XPROTECT */
  XEXCEPT
	{
	}
  XEND;

  return(OM_ERR_NONE);
}

omfErr_t omfiMobGetThisSource(
    omfMobFindHdl_t hdl,       /* IN */
	omfMobKind_t mobKind,             /* IN */
	omfMediaCriteria_t *mediaCrit,    /* IN */
	omfEffectChoice_t *effectChoice,  /* IN */
	omfCpntObj_t *thisCpnt,           /* OUT */
	omfPosition_t *thisPosition,      /* OUT */
	omfFindSourceInfo_t *sourceInfo, /* OUT */
    omfBool *foundTransition)        /* OUT */
{
  omfObject_t leafObj = NULL, effeObject = NULL;
  omfMobObj_t nextMob = NULL;
  omfFindSourceInfo_t tmpSourceInfo;
  omfBool sourceFound = FALSE;
  omfPosition_t diffPos, nextPos;
  omfLength_t minLength, newLen;
  omfTrackID_t nextTrackID;
  omfLength_t nextLen;
  omfInt32		nestDepth;

  /* Initialize outputs */
  if (thisCpnt)
	*thisCpnt = NULL;
  if (thisPosition)
	omfsCvtInt32toPosition(0, (*thisPosition));
  if (sourceInfo)
	{
	  (*sourceInfo).mob = NULL;
	  (*sourceInfo).mobTrackID = 0;
	  (*sourceInfo).editrate.numerator = 0;
	  (*sourceInfo).editrate.denominator = 1;
	  (*sourceInfo).filmTapePdwn = NULL;
	  (*sourceInfo).tapeFilmPdwn = NULL;
	  (*sourceInfo).effeObject = NULL;
	  omfsCvtInt32toLength(0, (*sourceInfo).position);
	  omfsCvtInt32toLength(0, (*sourceInfo).minLength);
	}

  omfAssertMobFindHdl(hdl); 

  XPROTECT(hdl->file)
	{
	  if (thisCpnt)
		*thisCpnt = hdl->currentObj;

	  /*** Find leaf object in this track that points to the next mob ***/

	  CHECK(MobFindLeaf(hdl->file, hdl->mob, hdl->track, 
						hdl->mediaCrit, effectChoice,
						hdl->currentObj, hdl->currentPos, hdl->currentObjLen,
						hdl->prevObject, hdl->nextObject, 
						hdl->scopeStack, hdl->currentObjPos,
						&leafObj, &minLength, foundTransition, &effeObject, &nestDepth,
						&diffPos));

 	  if (omfsInt64Less(minLength, hdl->currentObjLen))
		{
		  newLen = minLength;
		}
	  else
		{
		  newLen = hdl->currentObjLen;
		}

	  /*** Find the next mob, factoring in mask object edit rate conversions,
	   *** and 1.0 track mappings.
	   ***/
	  CHECK(FindNextMob(hdl->file, hdl->mob, hdl->track, leafObj, 
						newLen, diffPos,
						&nextMob, &nextTrackID, &nextPos, NULL, NULL, &nextLen));

	  /*** Find component at referenced position in new mob ***/
	  CHECK(MobFindSource(hdl->file, 
						  nextMob, nextTrackID, nextPos, nextLen,
						  mobKind, mediaCrit, effectChoice,
						  &tmpSourceInfo, &sourceFound));
	  if (sourceFound)
		{
		  (*sourceInfo).mob = tmpSourceInfo.mob;
		  (*sourceInfo).mobTrackID = tmpSourceInfo.mobTrackID;
		  (*sourceInfo).position = tmpSourceInfo.position;
		  (*sourceInfo).editrate = tmpSourceInfo.editrate;
		  (*sourceInfo).minLength = tmpSourceInfo.minLength;
	  	  if(tmpSourceInfo.filmTapePdwn != NULL)
	  	  	(*sourceInfo).filmTapePdwn = tmpSourceInfo.filmTapePdwn;
	  	  if(tmpSourceInfo.tapeFilmPdwn != NULL)
		  	  (*sourceInfo).tapeFilmPdwn = tmpSourceInfo.tapeFilmPdwn;
		  if (thisCpnt)
			*thisCpnt = hdl->currentObj;
		  if (thisPosition)
			*thisPosition = hdl->currentPos;
		}
	  else /* Failure - null  out return values */
		{
		  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
		}
	}
  XEXCEPT
	{
	  if(XCODE() == OM_ERR_PARSE_EFFECT_AMBIGUOUS)
		{
	  		(*sourceInfo).effeObject = effeObject;
	  	}
	}
  XEND;

  return(OM_ERR_NONE);
}


/* This function will resolve mask objects to find the correct offset,
 * and will map track IDs for 1.0 files.
 */
static omfErr_t FindNextMob(omfHdl_t file, 
					 omfMobObj_t mob, 
					 omfObject_t track, 
					 omfObject_t segment,
					 omfLength_t length,
					 omfPosition_t diffPos,
					 omfMobObj_t *retMob,
					 omfTrackID_t *retTrackID,
					 omfPosition_t *retPos,
					 omfObject_t *pulldownObj,
					 omfInt32 *pulldownPhase,
					 omfLength_t *retLen)
{
  omfBool isMask = FALSE, reverse = FALSE;
  omfObject_t tmpSlot = NULL, sclp = NULL, maskObj = NULL;
  omfSourceRef_t sourceRef;
  omfUID_t nullUID = {0,0,0};
  omfUID_t MCnullUID = {1,0,1};
  omfInt16 tmp1xTrackType = 0;
  omfInt32	phase;
  omfTrackID_t tmpTrackID, nextTrackID;
  omfObject_t nextTrack = NULL, nextMob = NULL;
  omfRational_t srcRate, destRate;
  omfPosition_t tmpPos, convertPos;
  omfLength_t tmpLen, sclpLen;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(file)
	{
	  /* Initialize return parameters */
	  if (retMob)
		*retMob = NULL;
	  else 
		{
		  RAISE(OM_ERR_NULL_PARAM);
		}
	  if (retTrackID)
		*retTrackID = 0;
	  else
		{
		  RAISE(OM_ERR_NULL_PARAM);
		}
	  if (!retPos)
		{
		  RAISE(OM_ERR_NULL_PARAM);
		}
	  if (!retLen)
		{
		  RAISE(OM_ERR_NULL_PARAM);
		}
	if(pulldownObj)
		*pulldownObj = NULL;
		
	  /* Get source editrate from track, for later conversion */
	  CHECK(omfiTrackGetInfo(file, mob, track, 
							 &srcRate, 0, NULL, NULL, NULL, NULL));

	  /* If 1.0 MASK object - prepare to apply to convert editrate */
	  if (omfsIsTypeOf(file, segment, "MASK", &omfError))
		{
		  isMask = TRUE;
		  /* Get the (assumed) source clip out of the mask */
		  maskObj = segment;
		  CHECK(omfsGetNthObjRefArray(file, maskObj, OMTRKGTracks,
									  &tmpSlot, 1));
		  CHECK(omfiMobSlotGetInfo(file, tmpSlot, NULL, &sclp));
		  if (!omfiIsASourceClip(file, sclp, &omfError))
			{
			  RAISE(OM_ERR_NO_MORE_MOBS);
			}
		  tmpLen = length;
		  CHECK(omfiComponentGetLength(file, sclp, &sclpLen));
		  reverse = FALSE;
		  CHECK(omfiPulldownMapOffset(file, maskObj, tmpLen, reverse, &length, &phase));
		  if(pulldownObj != NULL)
		  	*pulldownObj = maskObj;
		  if(pulldownPhase != NULL)
		  	*pulldownPhase = phase;
		}
	  else if (omfsIsTypeOf(file, segment, "PDWN", &omfError))
		{
		  isMask = TRUE;
		  /* Get the (assumed) source clip out of the mask */
		  maskObj = segment;
		  CHECK(omfsReadObjRef(file, segment, OMPDWNInputSegment, &sclp));
		  if (!omfiIsASourceClip(file, sclp, &omfError))
			{
			  RAISE(OM_ERR_NO_MORE_MOBS);
			}
		  tmpLen = length;
		  CHECK(omfiComponentGetLength(file, sclp, &sclpLen));
		  reverse = FALSE;
		  CHECK(omfiPulldownMapOffset(file, maskObj, tmpLen, reverse, &length, &phase));
		  if(pulldownObj != NULL)
		  	*pulldownObj = maskObj;
		  if(pulldownPhase != NULL)
		  	*pulldownPhase = phase;
		}
	  else if (omfiIsAFiller(file, segment, &omfError))
		{
		  RAISE(OM_ERR_FILL_FOUND);
		}
	  else if (!omfiIsASourceClip(file, segment, &omfError))
		{
		  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
		}
	  else
		{
		  sclp = segment;
		  CHECK(omfiComponentGetLength(file, sclp, &sclpLen));
		}

 	  if (omfsInt64Less(length, sclpLen))
		{
		  sclpLen = length;
		}

	  CHECK(omfiSourceClipGetInfo(file, sclp, NULL, NULL, &sourceRef));
	  if (equalUIDs(nullUID, sourceRef.sourceID) || 
		  equalUIDs(MCnullUID, sourceRef.sourceID))
		{
		  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
		}
	  /* Get next mob */
	  CHECK(omfiSourceClipResolveRef(file, sclp, &nextMob));

	  /* Map 1.x trackID to 2.x, and get source editrate from sclp */
	  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		{
		  CHECK(omfsReadTrackType(file, sclp,
								  OMCPNTTrackKind, &tmp1xTrackType));
		  /* Get the MobID from the track's mob */
		  CHECK(CvtTrackNumtoID(file, 
								sourceRef.sourceID, 
								(omfInt16)sourceRef.sourceTrackID,
								tmp1xTrackType, &tmpTrackID));
		}
	  else
		tmpTrackID = sourceRef.sourceTrackID;

	  /* Get destination track's edit rate */
	  CHECK(FindTrackByTrackID(file, nextMob, tmpTrackID, &nextTrack));
	  CHECK(omfiTrackGetInfo(file, nextMob, nextTrack, &destRate, 
							 0, NULL, NULL, &nextTrackID, NULL));
	  
	  /* If its a MASK, apply the mask bits to the offset difference
	   * (into the MASK object).  Then add this to the startTime in the
	   * Source Clip.  Then do an editrate conversion in case it is
	   * needed (24/1 -> 96/1 for Film Composer).  Then follow this 
	   * offset to the next mob in the mob chain.
	   */
	  if (isMask)
		{
		  reverse = FALSE;
		  /* !!!Check out if we need phase returned from here */
		  CHECK(omfiPulldownMapOffset(file, maskObj, diffPos, reverse, &tmpPos, NULL));
		}
	  else
		tmpPos = diffPos;

	  CHECK(omfsAddInt64toInt64(sourceRef.startTime, &tmpPos));
	  if (!isMask)
		{
		  CHECK(omfiConvertEditRate(srcRate, tmpPos,
									destRate, kRoundNormally,   // NPW, was kRoundFloor,
									&convertPos));
		}
	  else
		convertPos = tmpPos;

	  *retMob = nextMob;
	  *retTrackID = nextTrackID;
	  *retPos = convertPos;
	  *retLen = sclpLen;
	}
  XEXCEPT
	{
	}
  XEND;
  
  return(OM_ERR_NONE);
}


static omfErr_t FindTrackAndSegment(omfHdl_t file,
							 omfMobObj_t mob,
							 omfTrackID_t trackID,
							 omfPosition_t offset,
							 omfObject_t *track,
							 omfObject_t *segment,
							 omfRational_t *srcRate,
							 omfPosition_t *diffPos)
{
  omfIterHdl_t sequIter = NULL, trackIter = NULL;
  omfInt32 trackLoop, sequLoop, numTracks, numSegs;
  omfObject_t tmpTrack = NULL, tmpSegment = NULL;
  omfBool foundTrack = FALSE, foundClip = FALSE;
  omfRational_t tmpSrcRate;
  omfPosition_t origin, sequPos;
  omfPosition_t begPos = omfsCvtInt32toPosition(0, begPos);
  omfPosition_t endPos = omfsCvtInt32toPosition(0, endPos);
  omfLength_t segLen;
  omfTrackID_t tmpTrackID;
  omfObject_t subSegment = NULL, nextSeg = NULL;
  omfLength_t zeroLen = omfsCvtInt32toLength(0, zeroLen);
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(file)
	{
	  /* Initialize return parameters */
	  if (track)
		*track = NULL;
	  else
		{
		  RAISE(OM_ERR_NULL_PARAM);
		}
	  if (segment)
		*segment = NULL;
	  else
		{
		  RAISE(OM_ERR_NULL_PARAM);
		}
	  if (!srcRate)
		{
		  RAISE(OM_ERR_NULL_PARAM);
		}

	  CHECK(omfiIteratorAlloc(file, &trackIter));
	  CHECK(omfiMobGetNumTracks(file, mob, &numTracks));
	  for (trackLoop = 0; trackLoop < numTracks; trackLoop++)
		{
		  CHECK(omfiMobGetNextTrack(trackIter, mob, NULL, &tmpTrack))
		  CHECK(omfiTrackGetInfo(file, mob, tmpTrack, 
								 &tmpSrcRate, 0, NULL, &origin,
								 &tmpTrackID, &tmpSegment));
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
	  else
		{
		  *track = tmpTrack;
		  *srcRate = tmpSrcRate;
		}
	omfiIteratorDispose(file, trackIter);

	  /* Normalize the requested position on the track by adding
	   * the StartPosition (1.x) or Origin (2.x) to it.  The origin
	   * was acquired above with omfiTrackGetInfo().
	   * The StartPosition/Origin will usually be 0.  It may be 
	   * negative, if more data before the original "origin" was 
	   * digitized later.
	   */
	  if (!omfsIsInt64Positive(origin))
		omfsMultInt32byInt64(-1, origin, &origin);
	  omfsAddInt64toInt64(origin, &offset);

	  if (omfiIsASequence(file, tmpSegment, &omfError))
		{
		  CHECK(omfiIteratorAlloc(file, &sequIter));
		  CHECK(omfiSequenceGetNumCpnts(file, tmpSegment, &numSegs));
		  for (sequLoop=0; sequLoop < numSegs; sequLoop++)
			{
			  CHECK(omfiSequenceGetNextCpnt(sequIter, tmpSegment,
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
#if 0
				  /* If following tran subsumes segment, skip this segment */
				  CHECK(SequPeekNextCpnt(sequIter, tmpSegment, &nextSeg,
										 &nextOffset, &nextLength));
				  if (omfiIsATransition(file, nextSeg, &omfError) &&
					  omfsInt64GreaterEqual(offset, nextOffset))
					{
					  continue;
					}
				  else
#endif
					{
					  foundClip = TRUE;
					  *segment = subSegment;
					  break;
					}
				}
			} /* for */
		  CHECK(omfiIteratorDispose(file, sequIter));
		  sequIter = NULL;
		} 
	  else /* Is not a sequence */
		{
		  CHECK(omfiComponentGetLength(file, tmpSegment, &segLen));
		  omfsCvtInt32toPosition(0, begPos);
		  CHECK(omfsAddInt64toInt64(segLen, &endPos));
		  if (omfsInt64LessEqual(begPos, offset) &&
			  omfsInt64Less(offset, endPos))
			{
			  foundClip = TRUE;
			  *segment = tmpSegment;
			}
		}
	  /* Calculate diffPos - difference between requested offset and
	   * the beginning of clip that contains it. 
	   */
	  (*diffPos) = offset;
	  CHECK(omfsSubInt64fromInt64(begPos, diffPos));

	} /* XPROTECT */
  XEXCEPT
	{
	  if (sequIter)
		omfiIteratorDispose(file, sequIter);
	  if (trackIter)
		omfiIteratorDispose(file, trackIter);
	}
  XEND;

  return(OM_ERR_NONE);
}

/* This is stretching it a little bit... but, this function guesses
 * that a mob is a Nagra mob if it satisfies the following:
 *       1) It has usage code UC_NULL
 *       2) It has a media descriptor (either MDES, NDNG, ??)
 *       3) It has a timecode track
 */
static omfBool IsANagraMob(omfHdl_t file,
						omfMobObj_t mob,
						omfErr_t *omfError)
{
  omfUsageCode_t usageCode = UC_NULL;
  omfObject_t mdesc = NULL;
  omfIterHdl_t mobIter = NULL;
  omfInt32 loop, numTracks;
  omfObject_t track = NULL, trackSeg = NULL;
  omfDDefObj_t datakind = NULL;
  omfErr_t tmpError = OM_ERR_NONE;
  omfBool		result = FALSE;

  XPROTECT(file)
	{
	  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		{
		  if (!omfsIsTypeOf(file, mob, "MOBJ", omfError))
			return(FALSE);
		  if(omfsIsPropPresent(file, mob, OMMOBJUsageCode, OMUsageCodeType))
			{
			  CHECK(omfsReadUsageCodeType(file, mob, OMMOBJUsageCode, 
										  &usageCode));
			  if (usageCode != UC_NULL)
				return(FALSE);
			}

		  tmpError = omfsReadObjRef(file, mob, OMMOBJPhysicalMedia, &mdesc);
		  if ((tmpError != OM_ERR_NONE) && 
			  (tmpError != OM_ERR_PROP_NOT_PRESENT))
			RAISE(tmpError);

		  CHECK(omfiIteratorAlloc(file, &mobIter));
		  CHECK(omfiMobGetNumTracks(file, mob, &numTracks));
		  for (loop = 1; loop <= numTracks; loop++)
			{
			  CHECK(omfiMobGetNextTrack(mobIter, mob, NULL, &track));
			  CHECK(omfiTrackGetInfo(file, mob, track, 
									 NULL, 0, NULL, NULL, 
									 NULL, &trackSeg));
			  CHECK(omfiComponentGetInfo(file, trackSeg, &datakind, NULL));
			  if (omfiIsTimecodeKind(file, datakind, kExactMatch, &tmpError))
				result = TRUE;
			}
			omfiIteratorDispose(file, mobIter);
			result = FALSE;
		}
	  else /* kOmfRev2x */
		result = FALSE;
	} /* XPROTECT */

  XEXCEPT
	{
		if(mobIter)
			omfiIteratorDispose(file, mobIter);
	  *omfError = XCODE();
	}
  XEND_SPECIAL(FALSE);

  *omfError = OM_ERR_NONE;
  return(result);
}

static omfErr_t MobFindSource(
					   omfHdl_t file,
					   omfMobObj_t mob,
					   omfTrackID_t trackID,
					   omfPosition_t offset, /* offset in referenced units */
					   omfLength_t length,   /* expected length of clip */
					   omfMobKind_t mobKind,
					   omfMediaCriteria_t *mediaCrit,
					   omfEffectChoice_t *effectChoice,
					   omfFindSourceInfo_t *sourceInfo,
					   omfBool *foundSource)
{
  omfObject_t track = NULL, pulldownObj = NULL;
  omfObject_t rootObj = NULL, leafObj = NULL, effeObject = NULL;
  omfMobObj_t nextMob = NULL;
  omfTrackID_t foundTrackID;
  omfBool nextFoundSource = FALSE, foundTransition = FALSE;
  omfPosition_t foundPos, diffPos, zeroPos;
  omfFindSourceInfo_t tmpSourceInfo;
  omfRational_t srcRate;
  omfLength_t tmpLength, foundLen, minLength, newLen;
  omfErr_t omfError = OM_ERR_NONE;
  omfInt32		nestDepth, pulldownPhase;

  omfsCvtInt32toInt64(0, &zeroPos);

  XPROTECT(file)
	{
	  /* Initialize return values */
	  if (foundSource)
		*foundSource = FALSE;
	  else
		{
		  RAISE(OM_ERR_NULL_PARAM);
		}
	  if (sourceInfo)
		{
		  (*sourceInfo).mob = NULL;
		  (*sourceInfo).mobTrackID = 0;
		  (*sourceInfo).editrate.numerator = 0;
		  (*sourceInfo).editrate.denominator = 1;
		(*sourceInfo).filmTapePdwn = NULL;
		  (*sourceInfo).tapeFilmPdwn = NULL;
		  (*sourceInfo).effeObject = NULL;
		  omfsCvtInt32toPosition(0, ((*sourceInfo).position));
		  omfsCvtInt32toLength(0, ((*sourceInfo).minLength));
		}
	  else
		{
		  RAISE(OM_ERR_NULL_PARAM);
		}

	  /* Verify that track and position are valid */
	  CHECK(FindTrackAndSegment(file, mob, trackID, offset, 
								&track, &rootObj, &srcRate, &diffPos));
	  CHECK(omfiComponentGetLength(file, rootObj, &tmpLength));
 	  if (omfsInt64Less(length, tmpLength))
		{
		  tmpLength = length;
		}

	  /* 1) Is this the mob that we're looking for? */
	  if (mobKind == kCompMob)
		{
		  if (omfiIsACompositionMob(file, mob, &omfError))
			*foundSource = TRUE;
		}
	  else if (mobKind == kMasterMob)
		{
		  if (omfiIsAMasterMob(file, mob, &omfError))
			*foundSource = TRUE;
		}
	  else if (mobKind == kFileMob)
		{
		  if (omfiIsAFileMob(file, mob, &omfError))
			*foundSource = TRUE;
		}
	  else if (mobKind == kTapeMob)
		{
		  if (omfiIsATapeMob(file, mob, &omfError))
			*foundSource = TRUE;

		  /* Also check for special Media Composer Nagra Mob with timecode */
		  if (IsANagraMob(file, mob, &omfError))
			*foundSource = TRUE;
		}
	  else if (mobKind == kFilmMob)
		{
		  if (omfiIsAFilmMob(file, mob, &omfError))
			*foundSource = TRUE;
		}
	  else if (mobKind == kAllMob)
		*foundSource = TRUE;
	  else
		{
		  RAISE(OM_ERR_INVALID_MOBTYPE);
		}

	  if (*foundSource)
		{
		  (*sourceInfo).mob = mob;
		  (*sourceInfo).mobTrackID = trackID;
		  (*sourceInfo).position = offset;
		  (*sourceInfo).editrate = srcRate;
		  (*sourceInfo).minLength = tmpLength;
		  return(OM_ERR_NONE);
		}

  	  /* 2) If not right mob type, find component at referenced position 
	   * in new mob.
	   */

	  /* Found the clip on the top level track - now, traverse down
	   * to find the leaf source clip if it exists.  We are assuming
	   * that there will not be nested sequences.
	   *
	   * NOTE: Media Composer uses nested sequences to embed 0-length 
	   * fills in compositions.
	   * We are also assuming that this function will probably be called
	   * on the master mob and down - so, we shouldn't run into transitions.
	   * So, passing NULL for prevObject and nextObject is probably alright.
	   */
	  CHECK(MobFindLeaf(file, mob, track, mediaCrit, effectChoice, 
						rootObj, offset, tmpLength,
						NULL, NULL, 
						NULL, /* Shouldn't be scopes here */
						zeroPos,
						&leafObj, &minLength, &foundTransition,
						&effeObject, &nestDepth, NULL));

 	  if (omfsInt64Less(minLength, length))
		{
		  /* Figure out diffPos!!! (changed newDIffPos -> tmpLength */
		  newLen = minLength;
		}
	  else
		{
		  newLen = length;
		}

	  /*** Find the next mob, factoring in mask object edit rate conversions,
	   *** and 1.0 track mappings.
	   ***/
	  CHECK(FindNextMob(file, mob, track, leafObj, 
						tmpLength, diffPos,
						&nextMob, &foundTrackID, &foundPos, &pulldownObj, &pulldownPhase, &foundLen));

	  if(pulldownObj != NULL)
	  {
			omfPulldownDir_t	direction;

		  	if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			{
				omfBool		isDouble;
				
				CHECK(omfsReadBoolean(file, pulldownObj, OMMASKIsDouble, &isDouble));
				direction = (isDouble == 0 ? kOMFFilmToTapeSpeed : kOMFTapeToFilmSpeed);
			}
		 	else
			{
				CHECK(omfsReadPulldownDirectionType(file, pulldownObj, OMPDWNDirection, &direction));
			}
			if(direction == kOMFFilmToTapeSpeed)	/* kOMFFilmToTapeSpeed */
			{
				(*sourceInfo).filmTapePdwn = pulldownObj;
				(*sourceInfo).filmTapePhase = pulldownPhase;
			}
			else				/* kOMFTapeToFilmSpeed */
			{
				(*sourceInfo).tapeFilmPdwn = pulldownObj;
				(*sourceInfo).tapeFilmPhase = pulldownPhase;
			}
	 } 
	  /* Find component at referenced position in new mob */
	  CHECK(MobFindSource(file, nextMob, foundTrackID,
						  foundPos, foundLen,
						  mobKind, mediaCrit, effectChoice,
						  &tmpSourceInfo, &nextFoundSource));
	  if (nextFoundSource)
		{
		  (*sourceInfo).mob = tmpSourceInfo.mob;
		  (*sourceInfo).mobTrackID = tmpSourceInfo.mobTrackID;
		  (*sourceInfo).position = tmpSourceInfo.position;
		  (*sourceInfo).editrate = tmpSourceInfo.editrate;
		  (*sourceInfo).minLength = tmpSourceInfo.minLength;
		  if(tmpSourceInfo.filmTapePdwn != NULL)
	  	  	(*sourceInfo).filmTapePdwn = tmpSourceInfo.filmTapePdwn;
	  	  if(tmpSourceInfo.tapeFilmPdwn != NULL)
		  	  (*sourceInfo).tapeFilmPdwn = tmpSourceInfo.tapeFilmPdwn;

		  *foundSource = nextFoundSource;
		}
	  else /* Failure - null  out return values */
		{
		  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
		}
	}
  XEXCEPT
	{
	  if(XCODE() == OM_ERR_PARSE_EFFECT_AMBIGUOUS)
	  	(*sourceInfo).effeObject = effeObject;
	}
  XEND;

  return(OM_ERR_NONE);
}

/* NOTE: the assumption is that this function should be used primarily 
 * for master mob and down 
 */
omfErr_t omfiMobSearchSource(	
    omfHdl_t file,                    /* IN */
    omfMobObj_t mob,                  /* IN */
    omfTrackID_t trackID,             /* IN */
	omfPosition_t offset,             /* IN */
	omfMobKind_t mobKind,             /* IN */
	omfMediaCriteria_t *mediaCrit,    /* IN */
	omfEffectChoice_t *effectChoice,  /* IN */  /* NOTE: take this arg out? */
	omfCpntObj_t *thisCpnt,           /* OUT */
	omfFindSourceInfo_t *sourceInfo)  /* OUT */
{
  omfLength_t cpntLen, nextLen, minLength, newLen;
  omfTrackID_t nextTrackID;
  omfObject_t track = NULL, rootObj = NULL, leafObj = NULL, pulldownObj = NULL;
  omfObject_t	effeObject;
  omfRational_t srcRate;
  omfPosition_t diffPos, nextPos;
  omfMobObj_t nextMob = NULL;
  omfFindSourceInfo_t tmpSourceInfo;
  omfBool sourceFound = FALSE, foundTransition = FALSE;
  omfErr_t omfError = OM_ERR_NONE;
  omfInt32	nestDepth, pulldownPhase;
  omfPosition_t zeroPos = omfsCvtInt32toPosition(0, zeroPos);
  omfScopeStack_t scopeStack = NULL;

  /* Initialize outputs */
  if (thisCpnt)
	*thisCpnt = NULL;
  if (sourceInfo)
	{
	  (*sourceInfo).mob = NULL;
	  (*sourceInfo).mobTrackID = 0;
	  (*sourceInfo).editrate.numerator = 0;
	  (*sourceInfo).editrate.denominator = 1;
	  (*sourceInfo).filmTapePdwn = NULL;
	  (*sourceInfo).tapeFilmPdwn = NULL;
	  (*sourceInfo).effeObject = NULL;
	  omfsCvtInt32toLength(0, (*sourceInfo).position);
	  omfsCvtInt32toLength(0, (*sourceInfo).minLength);
	}

  XPROTECT(file)
	{
	  /* omfiMobSearchSource should only be called for Master Mobs
	   * and Source Mobs.
	   */
	  if (omfiIsACompositionMob(file, mob, &omfError))
		{
		  RAISE(OM_ERR_INVALID_MOBTYPE);
		}

	  /* Find segment at offset */
	  CHECK(FindTrackAndSegment(file, mob, trackID, offset,
								&track, &rootObj, &srcRate, &diffPos));
	  CHECK(omfiComponentGetLength(file, rootObj, &cpntLen));

	  /*** Find leaf object in this track that points to the next mob ***/
	  ScopeStackAlloc(file, &scopeStack);
	  CHECK(MobFindLeaf(file, mob, track, 
						mediaCrit, effectChoice,
						rootObj, offset, cpntLen,
						NULL, NULL, 
						scopeStack, /* Shouldn't be scopes here?? */
						zeroPos,
						&leafObj, &minLength, &foundTransition, &effeObject,
						&nestDepth, NULL));
	  if (omfsInt64Less(minLength, cpntLen))
		{
		  /* Figure out diffPos */
		  newLen = minLength;
		  /* NOTE: What should diffPos be in this case? */
		}
	  else
		{
		  newLen = cpntLen;
		}

	  /*** Find the next mob, factoring in mask object edit rate conversions,
	   *** and 1.0 track mappings.
	   ***/
	  CHECK(FindNextMob(file, mob, track, leafObj, 
						cpntLen, diffPos,
						&nextMob, &nextTrackID, &nextPos, &pulldownObj, &pulldownPhase, &nextLen));
	  if(pulldownObj != NULL)
	  {
			omfPulldownDir_t	direction;

		  	if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			{
				omfBool		isDouble;
				
				CHECK(omfsReadBoolean(file, pulldownObj, OMMASKIsDouble, &isDouble));
				direction = (isDouble == 0 ? kOMFFilmToTapeSpeed : kOMFTapeToFilmSpeed);
			}
		 	else
			{
				CHECK(omfsReadPulldownDirectionType(file, pulldownObj, OMPDWNDirection, &direction));
			}
			if(direction == kOMFFilmToTapeSpeed)	/* kOMFFilmToTapeSpeed */
			{
				(*sourceInfo).filmTapePdwn = pulldownObj;
				(*sourceInfo).filmTapePhase = pulldownPhase;
			}
			else				/* kOMFTapeToFilmSpeed */
			{
				(*sourceInfo).tapeFilmPdwn = pulldownObj;
				(*sourceInfo).tapeFilmPhase = pulldownPhase;
			}
	 } 

	  /*** Find component at referenced position in new mob ***/
	  CHECK(MobFindSource(file, 
						  nextMob, nextTrackID, nextPos, nextLen,
						  mobKind, mediaCrit, effectChoice,
						  &tmpSourceInfo, &sourceFound));
	  if (sourceFound)
		{
		  (*sourceInfo).mob = tmpSourceInfo.mob;
		  (*sourceInfo).mobTrackID = tmpSourceInfo.mobTrackID;
		  (*sourceInfo).position = tmpSourceInfo.position;
		  (*sourceInfo).editrate = tmpSourceInfo.editrate;
		  (*sourceInfo).minLength = tmpSourceInfo.minLength;
	  	  if(tmpSourceInfo.filmTapePdwn != NULL)
	  	  	(*sourceInfo).filmTapePdwn = tmpSourceInfo.filmTapePdwn;
	  	  if(tmpSourceInfo.tapeFilmPdwn != NULL)
		  	  (*sourceInfo).tapeFilmPdwn = tmpSourceInfo.tapeFilmPdwn;
		  if (thisCpnt)
			*thisCpnt = rootObj;
		}
	  else /* Failure - null  out return values */
		{
		  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
		}
	} /* XPROTECT */

  XEXCEPT
	{
	  if(XCODE() == OM_ERR_PARSE_EFFECT_AMBIGUOUS)
	  	(*sourceInfo).effeObject = effeObject;
	}
  XEND;

  if( scopeStack != NULL )
  {
	  omOptFree(file, scopeStack );
  }

  return(OM_ERR_NONE);
}


static omfErr_t ScopeStackAlloc(omfHdl_t file,
						 omfScopeStack_t *scopeStack)
{
  omfInt32 loop;
  omfScopeStack_t tmpStack = NULL;

  *scopeStack = NULL;
  omfAssertValidFHdl(file);
  
  XPROTECT(file)
	{
	  tmpStack = (omfScopeStack_t)omOptMalloc(file, 
											sizeof(struct omfiScopeStack));
	  if (tmpStack == NULL)
		{
		  RAISE(OM_ERR_NOMEMORY);
		}
	  tmpStack->stackSize = 0;
	  tmpStack->maxSize = SCOPEBLOCKS;
	  tmpStack->scopeList = (omfScopeInfo_t **)omOptMalloc(NULL, 
									   sizeof(omfScopeInfo_t *)*SCOPEBLOCKS);
	  if (tmpStack->scopeList == NULL)
		{
		  RAISE(OM_ERR_NOMEMORY);
		}
	  /* Initialize scope pointers */
	  for (loop = 0; loop < SCOPEBLOCKS; loop++)
		{
		  tmpStack->scopeList[loop] = NULL;
		}
	} /* XPROTECT */

  XEXCEPT
	{
	  if ((XCODE() != OM_ERR_NOMEMORY) && tmpStack)
		ScopeStackDispose(file, tmpStack);/* changed from omOptFree to remove memory leak */
	}
  XEND;

  *scopeStack = tmpStack;
  return(OM_ERR_NONE);
}

static omfErr_t ScopeStackDispose(omfHdl_t file,
						   omfScopeStack_t scopeStack)
{
  omfInt32 loop;

  omfAssertValidFHdl(file);
  if (!scopeStack)
	{
	  return(OM_ERR_NULL_PARAM);
	}

  for (loop = 0; loop <= scopeStack->stackSize; loop++)
	{
	  if (scopeStack->scopeList[loop])
		omOptFree(file, scopeStack->scopeList[loop]);
	}

  if (scopeStack->scopeList)
	omOptFree(NULL, scopeStack->scopeList);
  if (scopeStack)
	omOptFree(file, scopeStack);

  scopeStack = NULL;

  return(OM_ERR_NONE);
}

static omfErr_t ScopeStackPop(omfHdl_t file,
					   omfScopeStack_t scopeStack,
					   omfInt32 numPops,
					   omfScopeInfo_t *scopeInfo)
{
  omfInt32 loop, numScopes, stopPop;

  numScopes = scopeStack->stackSize;
  stopPop = numScopes - numPops + 1;
  for (loop = numScopes; loop >= stopPop; loop--)
	{
	  scopeStack->stackSize--;
	  if (loop == stopPop)
		{
		  (*scopeInfo).scope = (*scopeStack->scopeList[loop-1]).scope;
		  (*scopeInfo).numSlots =(*scopeStack->scopeList[loop-1]).numSlots;
		  (*scopeInfo).scopeOffset = 
			           (*scopeStack->scopeList[loop-1]).scopeOffset;
		  (*scopeInfo).currSlotIndex = 
                       (*scopeStack->scopeList[loop-1]).currSlotIndex;
		  break;
		}
		omOptFree(file, scopeStack->scopeList[loop-1]);
	}

  return(OM_ERR_NONE);
}

static omfErr_t ScopeStackPeek(omfHdl_t file,
					   omfScopeStack_t scopeStack,
					   omfInt32 numPops,
					   omfScopeInfo_t *scopeInfo)
{
  omfInt32		numScopes, stopPop;

  numScopes = scopeStack->stackSize;
  stopPop = numScopes - numPops + 1;
  (*scopeInfo).scope = (*scopeStack->scopeList[stopPop-1]).scope;
  (*scopeInfo).numSlots =(*scopeStack->scopeList[stopPop-1]).numSlots;
  (*scopeInfo).scopeOffset = 
	           (*scopeStack->scopeList[stopPop-1]).scopeOffset;
  (*scopeInfo).currSlotIndex = 
               (*scopeStack->scopeList[stopPop-1]).currSlotIndex;

  return(OM_ERR_NONE);
}

static omfErr_t ScopeStackPush(omfHdl_t file,
						omfScopeStack_t scopeStack,
						omfScopeInfo_t scopeInfo)
{
  omfScopeInfo_t *tmpInfo = NULL;
  omfBool		uniqueScope;

  XPROTECT(file)
	{
		if(scopeStack->stackSize == 0)
			uniqueScope = TRUE;
		else
			uniqueScope = (scopeInfo.scope !=
						   (*scopeStack->scopeList[scopeStack->stackSize-1]).scope);
		if(uniqueScope)
		{
			tmpInfo = (omfScopeInfo_t *)omOptMalloc(file, sizeof(omfScopeInfo_t));
			if (tmpInfo == NULL)
			{
				RAISE(OM_ERR_NOMEMORY);
			}

			if (scopeStack->maxSize == scopeStack->stackSize)
				;  /* NOTE: allocate memory for more scope pointers, and 
					* set maxSize+10 */

			scopeStack->scopeList[scopeStack->stackSize] = tmpInfo;
			scopeStack->stackSize++;
		}
		else
			tmpInfo = scopeStack->scopeList[scopeStack->stackSize-1];
			
		tmpInfo->scope = scopeInfo.scope;
		tmpInfo->numSlots = scopeInfo.numSlots;
		tmpInfo->scopeOffset = scopeInfo.scopeOffset;
		tmpInfo->currSlotIndex = scopeInfo.currSlotIndex;
	}
  XEXCEPT
	{
	  /* Assume don't have to free if malloc failed */
	  if ((XCODE() != OM_ERR_NOMEMORY) && tmpInfo)
		omOptFree(file, tmpInfo);
	}
  XEND;

  return(OM_ERR_NONE);
}


static omfErr_t MobFindLeaf(
					 omfHdl_t file,
					 omfMobObj_t mob,
					 omfMSlotObj_t track,
					 omfMediaCriteria_t *mediaCrit,
					 omfEffectChoice_t *effectChoice,
					 omfObject_t rootObj,
					 omfPosition_t rootPos,
					 omfLength_t rootLen,
					 omfObject_t prevObject,
					 omfObject_t nextObject,
					 omfScopeStack_t scopeStack,
					 omfPosition_t	currentObjPos,
					 omfObject_t *foundObj,
					 omfLength_t *minLength,
					 omfBool *foundTransition,
					 omfObject_t *effeObject,
					 omfInt32	*nestDepth,
					 omfPosition_t *diffPos)
{
  omfObject_t critClip = NULL, seg = NULL, selected = NULL, effeSlotClip = NULL;
  omfObject_t tmpSlot = NULL, cpnt = NULL;
  omfObject_t effectObj = NULL, renderClip = NULL;
  omfObject_t tmpFound = NULL;
  omfMediaCriteria_t criteriaStruct, *ptrCriteriaStruct;
  omfIterHdl_t sequIter = NULL, slotIter = NULL, effeIter = NULL;
  omfPosition_t sequOffset, begPos, endPos;
  omfLength_t cpntLen, tmpMinLen;
  omfInt32 numSlots, loop, numCpnt, slotLoop, numEffSlots, argID;
  omfTrackID_t trackID;
  omfDDefObj_t datakind = NULL;
  omfUInt32 relScope, relSlot;
  omfScopeInfo_t scopeInfo;
  omfPosition_t zeroPos = omfsCvtInt32toPosition(0, zeroPos);
  omfErr_t omfError = OM_ERR_NONE, tmpError = OM_ERR_NONE;
  omfBool		wasEnabled;
  omfEffectChoice_t	localEffChoice;
	omfESlotObj_t effSlot;
	omfArgIDType_t effArgID;
	
  /* Parameter initialization and checking */
  if(nestDepth)
  	*nestDepth = 0;
  if (foundObj)
	*foundObj = NULL;
  else
	  return(OM_ERR_NULL_PARAM);
  if (effeObject)
	*effeObject = NULL;
  else
	  return(OM_ERR_NULL_PARAM);
  if (minLength)
	omfsCvtInt32toLength(0, (*minLength));
  else
	return(OM_ERR_NULL_PARAM);
  omfAssertValidFHdl(file);
  omfAssert((rootObj != NULL), file, OM_ERR_NULLOBJECT);

  XPROTECT(file)
	{
	  CHECK(omfiTrackGetInfo(file, mob, track,
							 NULL, 0, NULL, NULL, &trackID, NULL));
	  /*** FILLER, SOURCECLIP, TIMECODECLIP, EDGECODECLIP, MASK ***/
	  if (omfiIsAFiller(file, rootObj, &omfError) ||
		  omfiIsASourceClip(file, rootObj, &omfError) ||
		  omfiIsATimecodeClip(file, rootObj, &omfError) ||
		  omfiIsAnEdgecodeClip(file, rootObj, &omfError) ||
		  omfsIsTypeOf(file, rootObj, "MASK", &omfError) ||
		  omfsIsTypeOf(file, rootObj, "PDWN", &omfError))

		{
		  *foundObj = rootObj;
		  CHECK(omfiComponentGetLength(file, rootObj, &tmpMinLen));
		  if (omfsInt64Less(tmpMinLen, rootLen))
			{
				*minLength = tmpMinLen;
				if(diffPos != NULL)
				{
				  /* Figure out diffPos */
				  *diffPos = rootPos;
				  omfsSubInt64fromInt64(currentObjPos, diffPos);
				}
			}
		  else
			{
				*minLength = rootLen;
				if(diffPos != NULL)
				  omfsCvtInt32toInt64(0, diffPos);
			}
		}
	  else if (omfsIsTypeOf(file, rootObj, "MASK", &omfError))
		{
			printf("***** FOUND MASK OBJECT *****\n");
		}
	  /*** SELECTOR - follow the selected track ***/
	  else if (omfiIsASelector(file, rootObj, &omfError))
		{
		  CHECK(omfiSelectorGetInfo(file, rootObj, NULL, NULL, &selected));
		  if (selected)
			{
			  CHECK(MobFindLeaf(file, mob, track, mediaCrit, effectChoice,
								selected, rootPos, rootLen,
								prevObject, nextObject, 
								scopeStack, 
								currentObjPos, &tmpFound, &tmpMinLen, foundTransition,
								effeObject, nestDepth, diffPos));
			  if (tmpFound)
				{
				  *foundObj = tmpFound;
				  if (omfsInt64Less(tmpMinLen, rootLen))
					*minLength = tmpMinLen;
				  else
					*minLength = rootLen;
				}
			  else
				{
				  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
				}
			}
		  else
			{
			  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
			}
		}

	  /*** MEDIA GROUP - pass media search criteria to retrieve choice ***/
	  else if (omfiIsAMediaGroup(file, rootObj, &omfError))
		{
		  criteriaStruct.proc = NULL;
		  if (mediaCrit)
			criteriaStruct = *mediaCrit; 
		  else
			criteriaStruct.type = kOmfAnyRepresentation;
		  ptrCriteriaStruct = &criteriaStruct;
		  CHECK(omfmGetCriteriaSourceClip(file, mob, trackID,
											  ptrCriteriaStruct, &critClip));

		  if (critClip)
			{
			  CHECK(MobFindLeaf(file, mob, track, mediaCrit, effectChoice,
								critClip, rootPos, rootLen,
								prevObject, nextObject, 
								scopeStack, 
								currentObjPos, &tmpFound, &tmpMinLen, foundTransition,
								effeObject, nestDepth, diffPos));
			  if (tmpFound)
				{
				  *foundObj = tmpFound;
				  if (omfsInt64Less(tmpMinLen, rootLen))
					*minLength = tmpMinLen;
				  else
					*minLength = rootLen;
				}
			  else
				{
				  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
				}
			}
		}

	  /*** TRANSITION - return the requested path ***/
	  else if (omfiIsATransition(file, rootObj, &omfError))
		{
		  omfBool tmpFoundTran = FALSE;
		/* This next line (localEffChoice) and the references to it within this
		 * if statement prevent nesting transitions, or effects
		 * within transitions.
		 */
		localEffChoice = kFindNull;

		  /*** If requesting INCOMING media, need prevObject ***/
		  *foundTransition = TRUE;
		  if (*effectChoice && (*effectChoice == kFindIncoming) && prevObject)
			{
			  CHECK(MobFindLeaf(file, mob, track,  mediaCrit, &localEffChoice,
								prevObject, rootPos, rootLen,
								NULL, NULL, 
								scopeStack, 
								currentObjPos, &tmpFound, &tmpMinLen, &tmpFoundTran,
								effeObject, nestDepth, diffPos));
			  if (tmpFound)
				{
				  *foundObj = tmpFound;
				  if (omfsInt64Less(tmpMinLen, rootLen))
					*minLength = tmpMinLen;
				  else
					*minLength = rootLen;
				}
			  else
				{
				  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
				}
			}
		  /*** If requesting OUTGOING media, need nextObject ***/
		  else if (*effectChoice && (*effectChoice == kFindOutgoing) 
				   && nextObject)
			{
			  CHECK(MobFindLeaf(file, mob, track, mediaCrit, &localEffChoice,
								nextObject, rootPos, rootLen,
								NULL, NULL, 
								scopeStack, 
								currentObjPos,
								&tmpFound, &tmpMinLen, &tmpFoundTran, effeObject,
								nestDepth, diffPos));
			  if (tmpFound)
				{
				  *foundObj = tmpFound;
				  if (omfsInt64Less(tmpMinLen, rootLen))
					*minLength = tmpMinLen;
				  else
					*minLength = rootLen;
				}
			  else
				{
				  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
				}
			}

		  /*** Return RENDER SOURCE CLIP ***/
		  else /* No effectChoice (default), or effectChoice == kFindRender */
			{
			  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
				{
				  omfError = omfsReadObjRef(file, rootObj, OMCPNTPrecomputed, 
											&renderClip);
				  if (omfError != OM_ERR_NONE)
					{
					  RAISE(OM_ERR_RENDER_NOT_FOUND);
					}
				  else
					{
					  *foundObj = renderClip;
					  CHECK(omfiComponentGetLength(file, renderClip, &tmpMinLen));
					  if (omfsInt64Less(tmpMinLen, rootLen))
						*minLength = tmpMinLen;
					  else
						*minLength = rootLen;
					}
				}
			  else /* kOmfRev2x */
				{
				  CHECK(omfiTransitionGetInfo(file, rootObj, NULL, NULL, NULL,
										  &effectObj));
				  if (effectObj)
					{
					  omfError = omfiEffectGetFinalRender(file, effectObj,
														  &renderClip);
					}
				  if (!effectObj || (omfError != OM_ERR_NONE))
					{
					  RAISE(OM_ERR_RENDER_NOT_FOUND);
					}
				  else
					{
					  *foundObj = renderClip;
					  CHECK(omfiComponentGetLength(file, renderClip, &tmpMinLen));
					  if (omfsInt64Less(tmpMinLen, rootLen))
						*minLength = tmpMinLen;
					  else
						*minLength = rootLen;
					}
				} /* kOmfRev2x */
			} /* if return render */
		} /* if transition */

	  /*** EFFECT - based on what is passed in ***/
	  /* NOTE: should this take user defined options? Right now, it
	   * looks for the first non-control track and tries to follow it.
	   * Maybe it should follow the render instead?
	   */
	  else if (omfiIsAnEffect(file, rootObj, &omfError))
		{
		  if(*effectChoice == kFindRender)
		  {
			  if ((file->setrev == kOmfRev1x) || 
				  file->setrev == kOmfRevIMA)
				{
				  omfError = omfsReadObjRef(file, rootObj,
											OMCPNTPrecomputed, &renderClip);
				  if (omfError != OM_ERR_NONE)
					RAISE(OM_ERR_RENDER_NOT_FOUND);
				}
			  else /* kOmfRev2x */
				{
				  omfError = omfiEffectGetFinalRender(file, rootObj, &renderClip);
				  if (omfError != OM_ERR_NONE)
					RAISE(OM_ERR_RENDER_NOT_FOUND);
				}
			  CHECK(MobFindLeaf(file, mob, track, mediaCrit, effectChoice,
								renderClip, rootPos, rootLen,
								NULL, NULL, 
								scopeStack, currentObjPos, &tmpFound, &tmpMinLen, 
								foundTransition,
								effeObject, nestDepth, diffPos));
		  }
		else if ((*effectChoice == kFindNull) ||
				 (*effectChoice == kFindIncoming) ||
				 (*effectChoice == kFindOutgoing))
		{
			*effeObject = rootObj;
			RAISE(OM_ERR_PARSE_EFFECT_AMBIGUOUS);
		}
		else if (file->setrev == kOmfRev2x)
		  {
		  	argID = ((omfInt32)(*effectChoice)) - ((omfInt32)kFindEffectSrc1);
		  	argID = (argID+1) * -1;

			CHECK(omfiEffectGetNumSlots(file, rootObj, &numEffSlots));
			CHECK(omfiIteratorAlloc(file, &effeIter));
			for(loop = 1; loop <= numEffSlots; loop++)
			{
				CHECK(omfiEffectGetNextSlot(effeIter, rootObj, NULL, &effSlot));
				CHECK(omfiEffectSlotGetInfo(file, effSlot, &effArgID, &effeSlotClip));
				if(effArgID == argID)
				{
					/* This next line (localEffChoice) and the references to it within this
					 * if statement prevents nesting effects, or transitions
					 * within effects.
					 */
					localEffChoice = kFindNull;
			  		CHECK(MobFindLeaf(file, mob, track, mediaCrit, &localEffChoice,
								effeSlotClip, rootPos, rootLen,
								NULL, NULL, 
								scopeStack, currentObjPos, &tmpFound, &tmpMinLen, 
								foundTransition,
								effeObject, nestDepth, diffPos));
					break;
				}
			}
			omfiIteratorDispose(file, effeIter);
		  }
		  if (tmpFound)
			{
			  *foundObj = tmpFound;
			  if (omfsInt64Less(tmpMinLen, rootLen))
				*minLength = tmpMinLen;
			  else
				*minLength = rootLen;
			}
		}
	  /*** NESTED SCOPE (2.0 only) ***/
	  else if (omfiIsANestedScope(file, rootObj, &omfError))
		{
		  CHECK(omfiNestedScopeGetNumSlots(file, rootObj, &numSlots));
		  CHECK(omfsGetNthObjRefArray(file, rootObj, OMNESTSlots,
									  &tmpSlot, numSlots));
		  /* Push another scope stack */
		  scopeInfo.scope = rootObj;
		  scopeInfo.numSlots = numSlots;
		  scopeInfo.scopeOffset = rootPos;
		  scopeInfo.currSlotIndex = numSlots;
		  ScopeStackPush(file, scopeStack, scopeInfo);

		  omfError = MobFindLeaf(file, mob, track, mediaCrit, effectChoice,
							tmpSlot, rootPos, rootLen,
							prevObject, nextObject, 
							scopeStack, currentObjPos, &tmpFound, &tmpMinLen,
							foundTransition, effeObject, nestDepth, diffPos);

		  if(nestDepth)
		  {
		  	(*nestDepth)++;
		  }
		  CHECK(omfError);
		  
		  /* Release Bento reference, so the useCount is decremented */
		  CMReleaseObject((CMObject)tmpSlot);

		  if (tmpFound)
			{
			  *foundObj = tmpFound;
			  if (omfsInt64Less(tmpMinLen, rootLen))
				*minLength = tmpMinLen;
			  else
				*minLength = rootLen;
			}
		  else
			{
			  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
			}
		}

	  /*** 1.0 TRACKGROUP ***/
	  else if(omfsIsTypeOf(file, rootObj, "TRKG", &omfError))
	  {
		/* Note: a trackgroup isn't really a mob, but these functions
		 * will work, since MOBJ is a subclass of TRKG in 1.0.
		 */
		/* Turn off semantic checking, since Media Composer writes wacky
		 * stuff in private effects.
		 */
#if OMFI_ENABLE_SEMCHECK
		wasEnabled = omfsIsSemanticCheckOn(file);
		omfsSemanticCheckOff(file);
#endif
	  	CHECK(omfiMobGetNumSlots(file, rootObj, &numSlots));
		CHECK(omfiIteratorAlloc(file, &slotIter));
		for (slotLoop = 0; slotLoop < numSlots; slotLoop++)
		  {
			CHECK(omfiMobGetNextSlot(slotIter, rootObj, NULL, &tmpSlot));
			CHECK(omfiMobSlotGetInfo(file, tmpSlot, NULL, &seg));
			CHECK(omfiComponentGetInfo(file, seg, &datakind, NULL));
			if (omfiIsPictureKind(file, datakind, kExactMatch, &omfError) ||
				omfiIsSoundKind(file, datakind, kExactMatch, &omfError))
			  {
				CHECK(MobFindLeaf(file, mob, track, mediaCrit, effectChoice,
								  seg, rootPos, rootLen,
								  prevObject, nextObject, 
								  scopeStack, currentObjPos, &tmpFound, &tmpMinLen,
								  foundTransition, effeObject, nestDepth, diffPos));
				if (tmpFound)
				  {
					*foundObj = tmpFound;
					if (omfsInt64Less(tmpMinLen, rootLen))
					  *minLength = tmpMinLen;
					else
					  *minLength = rootLen;
				  }
				break;
			  }
		  } /* slot for loop */
		CHECK(omfiIteratorDispose(file, slotIter));
#if OMFI_ENABLE_SEMCHECK
		if(wasEnabled)
		{
			omfsSemanticCheckOn(file);
		}
#endif
		if (!tmpFound)
		  {
			RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
		  }
	  } /* If TRKG */

	  /*** SCOPE REFERENCE (2.0 only) ***/
	  else if (omfiIsAScopeRef(file, rootObj, &omfError))
		{
		  /* If scope information is missing, then return an error */
		  if (!scopeStack)
			{
			  RAISE(OM_ERR_TRAVERSAL_NOT_POSS); 
			}

		  CHECK(omfiScopeRefGetInfo(file, rootObj, NULL, NULL,
									&relScope, &relSlot));
		  tmpError = ScopeStackPeek(file, scopeStack, relScope+1, &scopeInfo);

		  /* NOTE: Should this be scopeInfo.currSlotIndex? */
		  tmpError = omfsGetNthObjRefArray(file, scopeInfo.scope, 
									   OMNESTSlots,  &tmpSlot, 
									   scopeInfo.currSlotIndex - relSlot);
		  if (tmpError == OM_ERR_NONE)
			{
			  CHECK(MobFindLeaf(file, mob, track, mediaCrit, effectChoice,
								tmpSlot, rootPos, rootLen,
								prevObject, nextObject, 
								scopeStack, currentObjPos, &tmpFound, &tmpMinLen,
								foundTransition, effeObject, nestDepth, diffPos));
			}

		  if (tmpFound)
			{
			  *foundObj = tmpFound;
			  if (omfsInt64Less(tmpMinLen, rootLen))
				*minLength = tmpMinLen;
			  else
				*minLength = rootLen;
			}
		  else
			{
			  RAISE(OM_ERR_TRAVERSAL_NOT_POSS); 
			}
		}

	  /* NOTE: We're making an assumption that we won't run into
	   * nested sequences.  There are some known cases where 
	   * sequences are used to surround a clip with 0-length
	   * fills.
	   */
	  else if (omfiIsASequence(file, rootObj, &omfError))
		{
		  omfBool rememberTran = FALSE;
		  omfObject_t pObj = NULL, nObj = NULL;
		  omfLength_t rememberTranLen, nextLen;
		  omfPosition_t nextPos;
		  omfErr_t tmpError = OM_ERR_NONE;

		  /* NOTE: A good optimization might be to remember all sequences 
		   * that were come across in a composition, and maintain statistics
		   * and information about them.  Currently with no optimization,
		   * parsing a nested sequence means starting from the beginning
		   * each time!
		   */

		  CHECK(omfiIteratorAlloc(file, &sequIter));
		  CHECK(omfiSequenceGetNumCpnts(file, rootObj, &numCpnt));
		  for (loop = 1; loop <= numCpnt; loop++)
			{
			  CHECK(omfiSequenceGetNextCpnt(sequIter, rootObj, NULL, 
											&sequOffset, &cpnt));

			  /* NOTE: For nested tracks in 1.0 files, the component offset
			   * could be relative to a nested track in an effect, etc.
			   * This needs to be accounted for.
			   */
			  if (file->setrev == kOmfRev1x)
				{
				  CHECK(omfsAddInt64toInt64(rootPos, &sequOffset));
				}


			  CHECK(omfiComponentGetLength(file, cpnt, &cpntLen));
			  /* Handle zero-length clips */
			  if(omfsInt64Equal(cpntLen, zeroPos))	
			  	continue;

			  /* Was previous component a transition? */
			  if (rememberTran)
				{
				  omfsSubInt64fromInt64(rememberTranLen, &cpntLen);
				  rememberTran = FALSE;
				}

			  /* Is the next component a transition? */
			  tmpError = SequPeekNextCpnt(sequIter, rootObj, &nObj,
										  &nextPos, &nextLen);
			  if (omfiIsATransition(file, nObj, &tmpError))
				{
				  omfsSubInt64fromInt64(nextLen, &cpntLen);
				}

			  /* Is this component a transition? */
			  if (omfiIsATransition(file, cpnt, &tmpError))
				{
				  rememberTran = TRUE;
				  rememberTranLen = cpntLen;
				}

			  begPos = sequOffset;
			  endPos = sequOffset;
			  CHECK(omfsAddInt64toInt64(cpntLen, &endPos));
			  if (omfsInt64LessEqual(begPos, rootPos) &&
				  omfsInt64Less(rootPos, endPos))
				{
				  CHECK(MobFindLeaf(file, mob, track, mediaCrit, effectChoice,
									cpnt, sequOffset, cpntLen,
									pObj, nObj, 
									scopeStack, currentObjPos, &tmpFound, &tmpMinLen,
									foundTransition, effeObject, nestDepth, NULL));
									
				  if(diffPos != NULL)
				  {
				  	*diffPos = rootPos;
				  	omfsSubInt64fromInt64(begPos, diffPos);
				  }	
				  if (tmpFound)
					{
					  *foundObj = tmpFound;
					  if (omfsInt64Less(tmpMinLen, rootLen))
						*minLength = tmpMinLen;
					  else
						*minLength = rootLen;
					  break;
					}
				  else
					{
					  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
					}
				}
			  pObj = cpnt;
			  /* else, continue iterating through sequence */
			} /* for loop */
		  CHECK(omfiIteratorDispose(file, sequIter));
		  sequIter = NULL;
		}
	  /* NOTE: Should not hit ConstantValue, VaryValue... */
	  else
		{
		  RAISE(OM_ERR_TRAVERSAL_NOT_POSS); 
		}

	} /* XPROTECT */

  XEXCEPT
	{
#if OMFI_ENABLE_SEMCHECK
		if(wasEnabled)
		{
			omfsSemanticCheckOn(file);
		}
#endif
	  if (sequIter)
		omfiIteratorDispose(file, sequIter);
	  if (effeIter)
		omfiIteratorDispose(file, effeIter);
	  /* At least try and return length if we can */
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: IteratorCopy()
 *************************************************************************/
static omfErr_t IteratorCopy(
	omfIterHdl_t inIter,   /* IN - Iterator to copy */ 
    omfIterHdl_t *outIter)  /* IN - Iterator to copy to */
{
	(*outIter)->cookie = inIter->cookie;
	(*outIter)->iterType = inIter->iterType;
	(*outIter)->currentIndex = inIter->currentIndex;
	(*outIter)->file = inIter->file;
	(*outIter)->maxIter = inIter->maxIter;
	(*outIter)->maxMobs = inIter->maxMobs;
	(*outIter)->head = inIter->head;
	(*outIter)->mdes = inIter->mdes;
	(*outIter)->currentObj = inIter->currentObj;
	(*outIter)->currentProp = inIter->currentProp;
	(*outIter)->nextIterator = inIter->nextIterator;
	(*outIter)->masterMob = inIter->masterMob;
	(*outIter)->masterTrack = inIter->masterTrack;
	(*outIter)->prevTransition = inIter->prevTransition;
	(*outIter)->prevLength = inIter->prevLength;
	(*outIter)->prevOverlap = inIter->prevOverlap;
	(*outIter)->position = inIter->position;
	(*outIter)->tableIter = inIter->tableIter;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: SequencePeekNextCpnt()
 *************************************************************************/
omfErr_t SequencePeekNextCpnt(omfIterHdl_t iterHdl, 
							  omfObject_t sequence,
							  omfObject_t *component,
							  omfPosition_t *offset)
{
  omfObject_t tmpCpnt = NULL;
  omfProperty_t prop;
  omfPosition_t tmpOffset;
  omfIterHdl_t saveIter = NULL;
  omfErr_t omfError = OM_ERR_NONE;

  omfAssertIterHdl(iterHdl);
  omfAssert((iterHdl->iterType == kIterSequence) ||
			(iterHdl->iterType == kIterNull), 
			iterHdl->file, OM_ERR_ITER_WRONG_TYPE);

  *component = NULL;

  XPROTECT(iterHdl->file)
	{
	  if (!omfiIsASequence(iterHdl->file, sequence, &omfError))
		{
		  RAISE(OM_ERR_INVALID_OBJ);
		}

	  if ((iterHdl->file->setrev == kOmfRev1x) || 
		  iterHdl->file->setrev == kOmfRevIMA)
		prop = OMSEQUSequence;
	  else
		prop = OMSEQUComponents;

	  /* Remember the current iterator settings, and replace after getNext */
	  CHECK(omfiIteratorAlloc(iterHdl->file, &saveIter));
	  IteratorCopy(iterHdl, &saveIter);

	  CHECK(omfiSequenceGetNextCpnt(iterHdl, sequence, NULL, 
									&tmpOffset, &tmpCpnt));

	  IteratorCopy(saveIter, &iterHdl);
	  omfiIteratorDispose(iterHdl->file, saveIter);
	}
  XEXCEPT
	{
	  if (saveIter)
		{
		  IteratorCopy(saveIter, &iterHdl);
		  omfiIteratorDispose(iterHdl->file, saveIter);
		}
	}
  XEND;

  *component = tmpCpnt;
  *offset = tmpOffset;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: MobFindCpntByPosition()
 *
 * 	    Given a mob, a trackID, an object in the track, and a desired
 *      position, this function recursively finds the object at that
 *      position.  For effects and transitions, it follows the final 
 *      rendering if it exists.  For selectors, it follows the selected slot.
 *      For a media group, it uses the media criteria to select a slot.
 *      For nested scopes, it follows the "value slot" (last slot).
 *      NOTE: this function currently does not handle scope references.
 *
 *      If the object is not found, the error OM_ERR_TRAVERSAL_NOT_POSS will 
 *      be returned.
 *
 * Argument Notes:
 *      Since this routine is recursive, it needs to remember the current
 *      state of where it is searching.  This is described with the rootObj
 *      and startPos parameters.  
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t MobFindCpntByPosition(
    omfHdl_t    file,       /* IN - File Handle */
	omfObject_t mob,        /* IN - Mob object */
	omfTrackID_t trackID,   /* IN - Track ID of searched track */
    omfObject_t rootObj,    /* IN - Current Root of tree in the track */
	omfPosition_t startPos, /* IN - Current Start Position in track */
	omfPosition_t position, /* IN - Desired Position */
    omfMediaCriteria_t *mediaCrit, /* IN - Media criteria (if mediagroup) */
	omfPosition_t     *diffPos, /* OUT - Difference between the beginning
								 *       of the segment and the desired pos 
								 */
    omfObject_t *foundObj,      /* OUT - Object found that desired position */
    omfLength_t *foundLen)      /* OUT - Length of found object, provided
                                         in case segment was incoming or
                                         outgoing segment of a transition */
{
  omfIterHdl_t sequIter = NULL;
  omfInt32 loop, numCpnt;
  omfLength_t tranLen, shortLen, zeroLen, nextLen;
  omfPosition_t sequOffset, tranPos, segEnd;
  omfObject_t cpnt = NULL, render = NULL, effect = NULL;
  omfBool prevTran = FALSE;
  omfPosition_t sclpTranOffset; /* Position into clip if tran before */

  omfInt32 numSlots;
  omfLength_t cpntLen, tmpLength;
  omfPosition_t currPos, endPos, zeroPos;
  omfObject_t seg = NULL, selected = NULL;
  omfObject_t sourceClip = NULL, tmpFound = NULL;
  omfObject_t tmpSlot = NULL, nextCpnt = NULL;
  omfMediaCriteria_t criteriaStruct, *ptrCriteriaStruct;
  omfErr_t omfError = OM_ERR_NONE;
	
  *foundObj = NULL;
  omfsCvtInt32toInt64(0, diffPos);
  omfsCvtInt32toPosition(0, zeroPos);
  omfsCvtInt32toPosition(0, sclpTranOffset); 
  omfsCvtInt32toInt64(0, &tranLen);
  omfsCvtInt32toInt64(0, &zeroLen);
  omfsCvtInt32toInt64(0, &shortLen);
  omfAssertValidFHdl(file);
  omfAssert((rootObj != NULL), file, OM_ERR_NULLOBJECT);

  XPROTECT(file)
	{
	  currPos = startPos;

	  CHECK(omfiComponentGetLength(file, rootObj, &cpntLen));
	  endPos = startPos;
	  CHECK(omfsAddInt64toInt64(cpntLen, &endPos));
	  /* If this component isn't at least as int  to contain pos, error */
 	  if (omfsInt64Less(endPos, position))
		{
		  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
		}

	  /* Remember the difference between the beginning of the segment
	   * and the desired position.  
	   */
	  *diffPos = position;
	  CHECK(omfsSubInt64fromInt64(startPos, diffPos));
	  
	  /* If leaf node, return the object */
	  if (omfiIsAFiller(file, rootObj, &omfError))
		{
		  *foundObj = rootObj;
		  CHECK(omfiComponentGetLength(file, rootObj, foundLen));
		  RAISE(OM_ERR_FILL_FOUND);
		}
	  else if (omfiIsASourceClip(file, rootObj, &omfError) ||
		  omfiIsATimecodeClip(file, rootObj, &omfError) ||
		  omfiIsAnEdgecodeClip(file, rootObj, &omfError) ||
		  omfiIsAConstValue(file, rootObj, &omfError) ||
		  omfiIsAVaryValue(file, rootObj, &omfError))
		{
		  *foundObj = rootObj;
		  CHECK(omfiComponentGetLength(file, rootObj, foundLen));
		}

	  /* If sequence, iterate through until find the CPNT at offset */
	  else if (omfiIsASequence(file, rootObj, &omfError))
		{
		  CHECK(omfiIteratorAlloc(file, &sequIter));
		  CHECK(omfiSequenceGetNumCpnts(file, rootObj, &numCpnt));
		  for (loop = 1; loop <= numCpnt; loop++)
			{
			  CHECK(omfiSequenceGetNextCpnt(sequIter, rootObj, NULL, 
											&sequOffset, &cpnt));

			  CHECK(omfiComponentGetLength(file, cpnt, &cpntLen));
			  /* Handle zero-length clips */
			  if(omfsInt64Equal(cpntLen, zeroPos))	
			  	continue;

			  /* Set the current position to the startPos (position of 
			   * the beginning of the sequence in the track), then add
			   * the position of sequence's current component 
			   */
			  currPos = startPos;
			  omfsAddInt64toInt64(sequOffset, &currPos);

			  /* Is there was a previous transition, calculate the new
			   * length for the outgoing segment.
			   */
			  if (prevTran)
				{
				  CHECK(omfsSubInt64fromInt64(tranLen, &cpntLen));
				  prevTran = FALSE;

				  /* If the transition completely consumes the outgoing 
				   * segment, get the next segment.
				   */
				  if (omfsInt64Equal(cpntLen, zeroLen))
					{
					  omfsCvtInt32toInt64(0, &tranLen); /* Reset */
					  continue;
					}
				  else 	/* Else, continue searching through sequence below */
					{
					  CHECK(omfsAddInt64toInt64(tranLen, &sclpTranOffset));
					  omfsCvtInt32toInt64(0, &tranLen); /* Reset */
					}
				}
			  else
				{
				  omfsCvtInt32toPosition(0, sclpTranOffset);  /* Reset */
				}

			  /* If this is a transition, skip.  The case where the requested
			   * position falls in a transition is handled below (by looking
			   * ahead, if the position falls in a segment that may have
			   * an overlapping transition in it.  
			   */
			  if (omfiIsATransition(file, cpnt, &omfError))
				{
				  tranLen = cpntLen;
				  prevTran = TRUE;
				  continue;  /* Skip to next component in sequence */
				}

			  segEnd = currPos;
			  CHECK(omfsAddInt64toInt64(cpntLen, &segEnd));
			  shortLen = cpntLen;
			  if (omfsInt64Less(position, segEnd))
				{
				  /* If the next component is a transition, see if the
				   * requested position points into the Transition.
				   */
				  nextCpnt = NULL;
				  omfError = SequencePeekNextCpnt(sequIter, rootObj, &nextCpnt,
											 &tranPos);
				  if (omfiIsATransition(file, nextCpnt, &omfError))
					{
					  CHECK(omfiTransitionGetInfo(file, nextCpnt, NULL, 
											  &nextLen, NULL, &effect));

					  /* omfiSequenceGetNextCpnt() already figured out the 
					   * transition's offset into the track for us.
					   * If position falls in shortened segment, return
					   * segment, else return rendered source clip.
					   */
					  if (omfsInt64Less(position, tranPos))
						{
						  CHECK(omfsSubInt64fromInt64(nextLen, &shortLen));
						}
					  else /* Falls within TRAN */
						{
						  /* The next component will be the final rendering */
						  if ((file->setrev == kOmfRev1x) || 
							  file->setrev == kOmfRevIMA)
							{
							  omfError = omfsReadObjRef(file, nextCpnt, 
											   OMCPNTPrecomputed, &render);
							  if (omfError != OM_ERR_NONE)
								{
								  cpntLen = nextLen;
								  RAISE(OM_ERR_RENDER_NOT_FOUND);
								}
							}
						  else /* kOmfRev2x */
							{
							  CHECK(omfiEffectGetFinalRender(file, effect, 
															 &render));
							}
						  if (render)
							{
							  cpnt = render;
							  currPos = tranPos;
							}
						} /* Position falls within a transition */
					} /* If Transition */

				  /* If in this segment, we're found it */
				  CHECK(MobFindCpntByPosition(file, mob, trackID, cpnt, 
											  currPos, position, mediaCrit, 
											  diffPos, &tmpFound, &tmpLength));
				  if (tmpFound)
					{
					  CHECK(omfsAddInt64toInt64(sclpTranOffset, diffPos));
					  *foundObj = tmpFound;
					  /* If segment shortened by transition, return
					   * shortened length 
					   */
					  if (omfsInt64Less(shortLen, tmpLength))
						*foundLen = shortLen;
					  else
						*foundLen = tmpLength;
					  break;
					}
				  else
					{
					  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
					}
				} /* Not in this segment */
			  else 
				{
				  omfsAddInt64toInt64(cpntLen, &currPos);
				}

			} /* for loop */
		  CHECK(omfiIteratorDispose(file, sequIter));
		  sequIter = NULL;
		}

	  /* If 1.0 MASK object, return this object.  FindSource will
	   * have to make sure that it correctly converts the position
	   * taking the mask into account if we need to keep traversing
	   * down the mob chain.
	   */
	  else if (omfsIsTypeOf(file, rootObj, "MASK", &omfError))
		{
		  *foundObj = rootObj;
		  CHECK(omfiComponentGetLength(file, rootObj, foundLen));
		}

	  /* If Selector, follow the selected track */
	  else if (omfiIsASelector(file, rootObj, &omfError))
		{
		  CHECK(omfiSelectorGetInfo(file, rootObj, NULL, NULL, &selected));
		  if (selected)
			{
			  CHECK(MobFindCpntByPosition(file, mob, trackID, selected, 
						  currPos, position, mediaCrit, diffPos, 
						  &tmpFound, &tmpLength));
			  if (tmpFound)
				{
				  *foundObj = tmpFound;
				  *foundLen = tmpLength;
				}
			  else
				{
				  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
				}
			}
		  else
			{
			  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
			}
		}

	  /* If Media Group, pass media search criteria to retrieve choice */
	  else if (omfiIsAMediaGroup(file, rootObj, &omfError))
		{
		  criteriaStruct.proc = NULL;
		  if (mediaCrit)
			criteriaStruct = *mediaCrit; 
		  else
			criteriaStruct.type = kOmfAnyRepresentation;
		  ptrCriteriaStruct = &criteriaStruct;
		  CHECK(omfmGetCriteriaSourceClip(file, mob, trackID,
											  ptrCriteriaStruct, &seg));

		  if (seg)
			{
			  CHECK(MobFindCpntByPosition(file, mob, trackID, seg, currPos, 
								   position, mediaCrit, diffPos, 
                                   &tmpFound, &tmpLength));
			  if (tmpFound)
				{
				  *foundObj = tmpFound;
				  *foundLen = tmpLength;
				}
			  else
				{
				  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
				}
			}
		}

	  /* If Effect, return rendered media source clip */
	  else if (omfiIsAnEffect(file, rootObj, &omfError))
		{
		  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			{
			  omfError = omfsReadObjRef(file, nextCpnt, 
										OMCPNTPrecomputed, &sourceClip);
			  if (omfError != OM_ERR_NONE)
				RAISE(OM_ERR_RENDER_NOT_FOUND);
			}
		  else /* kOmfRev2x */
			{
			  CHECK(omfiEffectGetFinalRender(file, rootObj, &sourceClip));
			}
		  CHECK(MobFindCpntByPosition(file, mob, trackID, sourceClip, currPos, 
									  position, mediaCrit, diffPos, 
									  &tmpFound, &tmpLength));
		  if (tmpFound)
			{
			  *foundObj = sourceClip;
			  *foundLen = tmpLength;
			}
		}

	  /* Get the last track of the nested scope */
	  else if (omfiIsANestedScope(file, rootObj, &omfError))
		{
		  CHECK(omfiNestedScopeGetNumSlots(file, rootObj, &numSlots));
		  CHECK(omfsGetNthObjRefArray(file, rootObj, OMNESTSlots,
									  &tmpSlot, numSlots));
		  CHECK(MobFindCpntByPosition(file, mob, trackID, tmpSlot, currPos, 
									  position, mediaCrit, diffPos, 
									  &tmpFound, &tmpLength));
		  /* Release Bento reference, so the useCount is decremented */
		  CMReleaseObject((CMObject)tmpSlot);

		  if (tmpFound)
			{
			  *foundObj = tmpFound;
			  *foundLen = tmpLength;
			}
		  else
			{
			  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
			}
		}
	  else if(omfsIsTypeOf(file, rootObj, "TRKG", &omfError))
	  {
	  	CHECK(omfiMobGetNumSlots(file, rootObj, &numSlots));
	  	if(numSlots == 1)		/* It's a simple 1-source effect like pan/vol */
	  	{
		  CHECK(omfsGetNthObjRefArray(file, rootObj, OMTRKGTracks,
									  &tmpSlot, 1));
		  CHECK(omfiMobSlotGetInfo(file, tmpSlot, NULL, &seg));
		  CHECK(MobFindCpntByPosition(file, mob, trackID, seg, currPos, 
									  position, mediaCrit, diffPos, 
									  &tmpFound, &tmpLength));
		  /* Release Bento reference, so the useCount is decremented */
		  CMReleaseObject((CMObject)tmpSlot);
		  if (tmpFound)
			{
			  *foundObj = tmpFound;
			  *foundLen = tmpLength;
			}
		  else
			{
			  RAISE(OM_ERR_TRAVERSAL_NOT_POSS);
			}
	  	}
		else
		  RAISE(OM_ERR_NOT_IMPLEMENTED); 
	  }
	  else 	  /* Else, if Scope Reference, return err */
		{
		  /* NOTE: How should scope references be handled? */
		  RAISE(OM_ERR_TRAVERSAL_NOT_POSS); 
		}
	} /* XPROTECT */

  XEXCEPT
	{
	  if (sequIter)
		omfiIteratorDispose(file, sequIter);
	  /* At least try and return length if we can */
	  *foundLen = cpntLen;
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobFindSource()
 *
 * 		Given a mob, trackID, offset into the track, and mob kind to
 *      search for, this function finds the corresponding mob and 
 *      track/position.  This function would be useful to implement
 *      a function that returned the relative timecode on a tape mob
 *      for a given position in a composition mob.  The minLength argument
 *      specifies the smallest length of a clip that was found somewhere
 *      in the mob chain.  The caller should compare this with the length
 *      of the clip that he/she is starting with to see if there is
 *      enough media/data.
 *
 *      This function will work for 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobFindSource(
    omfHdl_t file,           /* IN - File Handle */
	omfMobObj_t rootMob,     /* IN - Root Mob to search */
	omfTrackID_t trackID,    /* IN - Track ID to search */
	omfPosition_t offset,    /* IN - Offset into Track */
	omfMobKind_t mobKind,    /* IN - Kind of mob to look for */
	omfMediaCriteria_t *mediaCrit, /* IN - Media Criteria, if find media group*/
	omfUID_t *mobID,         /* OUT - Found mob's Mob ID */
	omfTrackID_t *mobTrack,  /* OUT - Found Mob's Track */
	omfPosition_t *mobOffset,/* OUT - Found Mob's Track offset */
	omfLength_t *minLength,  /* OUT - Minimum clip length found in chain */
	omfMobObj_t *mob)        /* OUT - Pointer to found mob */
{
	omfBool	foundLen = FALSE;
	
	if (minLength)
	  omfsCvtInt32toInt64(0, minLength);

	return(omfiMobFindSourceRecurs(file, rootMob, trackID, offset, 
								   mobKind, mediaCrit, 
								   mobID, mobTrack, mobOffset, minLength,
								   mob, &foundLen));
}



	
/* Added lower-level function in order to add "foundLength" parm */
static omfErr_t omfiMobFindSourceRecurs(
    omfHdl_t file,           /* IN - File Handle */
	omfMobObj_t rootMob,     /* IN - Root Mob to search */
	omfTrackID_t trackID,    /* IN - Track ID to search */
	omfPosition_t offset,    /* IN - Offset into Track */
	omfMobKind_t mobKind,    /* IN - Kind of mob to look for */
	omfMediaCriteria_t *mediaCrit, /* IN - Media Criteria, if find media group*/
	omfUID_t *mobID,         /* OUT - Found mob's Mob ID */
	omfTrackID_t *mobTrack,  /* OUT - Found Mob's Track */
	omfPosition_t *mobOffset,/* OUT - Found Mob's Track offset */
	omfLength_t *minLength,  /* OUT - Minimum clip length found in chain */
	omfMobObj_t *mob,        /* OUT - Pointer to found mob */
	omfBool		*foundLength)
{
    omfIterHdl_t mobIter = NULL;
	omfPosition_t diffPos;
	omfPosition_t zeroPos = omfsCvtInt32toPosition(0, zeroPos);
	omfPosition_t origin, foundPos, convertPos;
	omfInt32 numTracks, loop;
	omfObject_t track = NULL, seg = NULL, nextMob = NULL, foundObj = NULL;
	omfObject_t destTrack = NULL, maskObj = NULL, sclp = NULL, tmpSlot = NULL;
	omfTrackID_t tmpTrackID, foundTrackID;
	omfBool foundTrack = FALSE, foundMob = FALSE, isMask = FALSE;
	omfSourceRef_t sourceRef;
	omfLength_t	refLength, sclpLen;
	omfInt16 tmp1xTrackType = 0;
	omfRational_t srcRate, destRate;
	omfBool reverse = FALSE;
	omfErr_t omfError = OM_ERR_NONE;

	if (mob)
	  *mob = NULL;
	omfAssertValidFHdl(file);
	omfAssert((rootMob != NULL), file, OM_ERR_NULLOBJECT);
	XPROTECT(file)
	  {
		/* Get the Track's Component */
		CHECK(omfiIteratorAlloc(file, &mobIter));
		CHECK(omfiMobGetNumTracks(file, rootMob, &numTracks));
		for (loop = 1; loop <= numTracks; loop++)
		  {
			CHECK(omfiMobGetNextTrack(mobIter, rootMob, NULL, &track));
			CHECK(omfiTrackGetInfo(file, rootMob, track, &srcRate, 0, NULL, 
								   &origin, &tmpTrackID, &seg));
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

		/* Normalize the requested position on the track by adding
		 * the StartPosition (1.x) or Origin (2.x) to it.  The origin
		 * was acquired above with omfiTrackGetInfo().
		 * The StartPosition/Origin will usually be 0.  It may be 
		 * negative, if more data before the original "origin" was 
		 * digitized later.
		 */
		/* LF-W (12/18/96) - This was discovered by Brooks.  We have to
         *                   take the absolute value of the origin before
		 *                   adding it to get the correct offset.
		 */
		if (!omfsIsInt64Positive(origin))
		  omfsMultInt32byInt64(-1, origin, &origin);
		omfsAddInt64toInt64(origin, &offset);

		/* Search through the mob until the object at offset is found */
		CHECK(MobFindCpntByPosition(file, rootMob, trackID, seg, 
									zeroPos, offset, mediaCrit, &diffPos, 
									&foundObj, &refLength)); 

		/* If this object is a MASK, then we need to calculate the
		 * offset of the source clip into the next mob taking
		 * dropped/doubled frames into account.
		 */
		if (omfsIsTypeOf(file, foundObj, "MASK", &omfError))
		  {
			isMask = TRUE;
			/* Get the (assumed) source clip out of the mask */
			maskObj = foundObj;
			CHECK(omfsGetNthObjRefArray(file, maskObj, OMTRKGTracks,
										&tmpSlot, 1));
			CHECK(omfiMobSlotGetInfo(file, tmpSlot, NULL, &sclp));
			if (!omfiIsASourceClip(file, sclp, &omfError))
			  {
				if (minLength != NULL)
				  *minLength = refLength;
				RAISE(OM_ERR_NO_MORE_MOBS);
			  }
		  }

		/* NOTE: 2.0 ERAT objects are currently not supported.  They
		 *       need to be handled similar to 1.0 MASK objects.
		 */

		/* If the object is not a source clip, we've reached the end
	     * of the mob chain.
		 */
		else if (!omfiIsASourceClip(file, foundObj, &omfError))
		  {
			if (minLength != NULL)
			  *minLength = refLength;
			RAISE(OM_ERR_NO_MORE_MOBS);
		  }
		else /* Assuming source clip */
		  sclp = foundObj;

		/* If it is a source clip, resolve the reference */
		CHECK(omfiSourceClipGetInfo(file, sclp, NULL, &sclpLen,
									&sourceRef));

		CHECK(omfiSourceClipResolveRef(file, sclp, &nextMob));

		/* Map 1.x trackID to 2.x, and get source editrate from sclp */
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(omfsReadExactEditRate(file, sclp, OMCPNTEditRate,
										&srcRate));

			CHECK(omfsReadTrackType(file, sclp, 
									OMCPNTTrackKind, &tmp1xTrackType));
			/* Get the MobID from the track's mob */
			CHECK(CvtTrackNumtoID(file, 
								  sourceRef.sourceID, 
								  (omfInt16)sourceRef.sourceTrackID,
								  tmp1xTrackType, &foundTrackID));
		  }
		else
		  foundTrackID = sourceRef.sourceTrackID;

		/* Get destination track's edit rate */
		CHECK(FindTrackByTrackID(file, nextMob, foundTrackID, &destTrack));
		CHECK(omfiTrackGetInfo(file, nextMob, destTrack, &destRate, 0, NULL,
							   NULL, NULL, NULL));

		/* If its a MASK, apply the mask bits to the offset difference
		 * (into the MASK object).  Then add this to the startTime in the
		 * Source Clip.  Then do an editrate conversion in case it is
		 * needed (24/1 -> 96/1 for Film Composer).  Then follow this 
		 * offset to the next mob in the mob chain.
		 */
		if (isMask)
		  {
			reverse = FALSE;
			CHECK(omfiPulldownMapOffset(file, maskObj, diffPos,
									reverse, &foundPos, NULL));
		  }
		else
		  foundPos = diffPos;

		CHECK(omfsAddInt64toInt64(sourceRef.startTime, &foundPos));
		CHECK(omfiConvertEditRate(srcRate, foundPos,
								  destRate, kRoundNormally, // NPW, was: kRoundFloor,
								  &convertPos));
		foundPos = convertPos;

		if(minLength != NULL)
		{
		  if(!(*foundLength) || omfsInt64Less(refLength, *minLength))
			{
			  *minLength = refLength;
			  *foundLength = TRUE;
			}
		}

		/* If this is the mobKind that we're looking for, we're done */
		if (mobKind == kCompMob) 
		  {
			if (omfiIsACompositionMob(file, nextMob, &omfError))
			  foundMob = TRUE;
		  }
		else if (mobKind == kMasterMob) 
		  {
			if (omfiIsAMasterMob(file, nextMob, &omfError))
			  foundMob = TRUE;
		  }
		else if (mobKind == kFileMob) 
		  {
			if (omfiIsAFileMob(file, nextMob, &omfError))
			  foundMob = TRUE;
		  }
		else if (mobKind == kTapeMob) 
		  {
			if (omfiIsATapeMob(file, nextMob, &omfError))
			  foundMob = TRUE;
		  }
		else if (mobKind == kFilmMob) 
		  {
			if (omfiIsAFilmMob(file, nextMob, &omfError))
			  foundMob = TRUE;
		  }
		else if (mobKind == kAllMob)
		  foundMob = TRUE;
		else
		  {
			RAISE(OM_ERR_INVALID_MOBTYPE);
		  }

		if (foundMob)
		  {
			if (mobID)
			  copyUIDs((*mobID), sourceRef.sourceID);
			if (mobTrack)
			  *mobTrack = foundTrackID;
			if (mobOffset)
			 {
				*mobOffset = foundPos;
		     }
			if (mob)
			  *mob = nextMob;
			if (mobIter)
			  {
				omfiIteratorDispose(file, mobIter);
				mobIter = NULL;
			  }
			return(OM_ERR_NONE);
		  }

		/* If mob of desired kind is not found, continue recursion down
		 * mob chain.
		 */

		/* Recursively follow mob chain, passing new mob information */
		CHECK(omfiMobFindSourceRecurs(file, nextMob, 
								foundTrackID,
								foundPos,
								mobKind, mediaCrit,
							    mobID, mobTrack, mobOffset, minLength, 
								mob, foundLength));

		if (mobIter)
		  {
			omfiIteratorDispose(file, mobIter);
			mobIter = NULL;
		  }
	  }

	XEXCEPT
	  {
		if (mob)
		  *mob = NULL;
		if (mobID)
		  initUID((*mobID), 0, 0, 0);
		if (mobTrack)
		  *mobTrack = 0;
		if (mobOffset)
		  omfsCvtInt32toInt64(0, mobOffset);
		if (mobIter)
		  omfiIteratorDispose(file, mobIter);
		/* Try and return length if we can */
		if (minLength)
		  *minLength = refLength;
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
