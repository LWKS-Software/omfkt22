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
/********************************************************************
 * ReadComp - This unittest parses a 1.x or 2.x composition and 
 *            "dumps" it to the screen.  Its purpose is to test out
 *            as much of the the mob traversal functionality in the 
 *            Toolkit as possible.  Right now, it is not very sophiticated
 *            about errors - if it finds one, it exits instead of
 *            trying to continue.
 ********************************************************************/

#include "masterhd.h"
#define TRUE 1
#define FALSE 0

#define PATHSIZE 256

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "omErr.h"
#include "omUtils.h"
#include "omFile.h"
#include "omDefs.h"
#include "omMobBld.h"
#include "omMobGet.h"
#include "omMedia.h"
#include "omCvt.h"

#include "UnitTest.h"

#if PORT_SYS_MAC
#include "MacSupport.h"
#include <console.h>
#endif

omfErr_t processComponent(omfHdl_t file, omfObject_t containingMob, omfRational_t editRate, omfSegObj_t segment, omfInt32 tabLevel);
omfErr_t processEffect(omfHdl_t file, omfSegObj_t segment, omfRational_t editRate, omfInt32 tabLevel);
omfErr_t processControlPoints(omfHdl_t file, omfCpntObj_t cpnt,omfInt32 tabLevel);
omfErr_t processMediaDescription(omfHdl_t file, 
			     omfMobObj_t mob,
			     omfInt32 tabLevel);
void printInfo(omfInt32  tabLevel, char *string);
omfErr_t traverseMob(omfHdl_t file, omfMobObj_t mob, omfInt32 tabLevel);
void printDatakind(omfHdl_t fileHdl,
		   omfDDefObj_t  def,
		   omfInt32 tabLevel);
void printDataValue(omfHdl_t fileHdl,
		    omfDDefObj_t def,
		    omfInt32 siz,
		    void *value,
		    omfInt32 tabLevel);

#ifdef MAKE_TEST_HARNESS
int	gMakeReadCompSilent = 1;
#else
int     gMakeReadCompSilent = 0;
#endif

/* Please excuse the lack of error handling code! */
#ifdef MAKE_TEST_HARNESS
int ReadComp(char *filename)
{
	int		argc;
	char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
	omfSessionHdl_t	session;
	omfHdl_t fileHdl;
	omfMobObj_t compMob;
	omfInt32 loop, numMobs, nameSize = 32;
	omfIterHdl_t mobIter = NULL;
	omfInt32 tabLevel = 0;
	omfUID_t ID;
	char printString[128];
	omfString name = NULL;
	omfTimeStamp_t createTime, lastModified;
	omfErr_t omfError = OM_ERR_NONE;

	XPROTECT(NULL)
	  {
#if PORT_SYS_MAC
		MacInit();
#endif

#ifdef MAKE_TEST_HARNESS
	argc = 2;
	argv[1] = filename;
#else
#if PORT_MAC_HAS_CCOMMAND
	    argc = ccommand(&argv); 
#endif
#endif

	    if (argc < 2)
	      {
		printf("*** ERROR - missing file name\n");
		return(1);
	      }

	    CHECK(omfsBeginSession(NULL, &session));
	    CHECK(omfmInit(session));
	    CHECK(omfsOpenFile((fileHandleType)argv[1], session, &fileHdl));

	    /* Iterate over all composition mobs in the file */
	    CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	    CHECK(omfiGetNumMobs(fileHdl, kAllMob, &numMobs)); 
	    for (loop = 0; loop < numMobs; loop++)
	      {
		CHECK(omfiGetNextMob(mobIter, NULL, &compMob));
	    
		CHECK(omfiMobGetMobID(fileHdl, compMob, &ID));
		sprintf(printString, "MOB: MobID %ld.%ld.%ld\n", 
			ID.prefix, ID.major, ID.minor);
		printInfo(tabLevel, printString);
		name = (omfString)omfsMalloc((size_t) nameSize);
		CHECK(omfiMobGetInfo(fileHdl, compMob, &ID, nameSize, 
					  name, &lastModified, &createTime));
		sprintf(printString, "MobName: %s, modTime: %ld, createTime: %ld\n",
			name, lastModified.TimeVal, createTime.TimeVal);
		omfsFree(name);
		name = NULL;
		
		printInfo(tabLevel, printString);
		if (omfiIsAPrimaryMob(fileHdl, compMob, &omfError))
		  sprintf(printString, "Mob is a Primary Mob\n");
		else
		  sprintf(printString, "Mob is NOT a Primary Mob\n");
		printInfo(tabLevel, printString);
		if (omfiIsACompositionMob(fileHdl, compMob, &omfError))
		  sprintf(printString, "COMPOSITION MOB\n");
		else if (omfiIsAMasterMob(fileHdl, compMob, &omfError))
		  sprintf(printString, "MASTER MOB\n");
		else if (omfiIsAFileMob(fileHdl, compMob, &omfError))
		    sprintf(printString, "FILE MOB\n");
		else if (omfiIsATapeMob(fileHdl, compMob, &omfError))
		  sprintf(printString, "TAPE MOB\n");
		else if (omfiIsAFilmMob(fileHdl, compMob, &omfError))
		  sprintf(printString, "FILM MOB\n");
		printInfo(tabLevel, printString);
	    
		/* If file mob, print media descriptor/locator information */
		if (omfiIsAFileMob(fileHdl, compMob, &omfError))
		  {
		    CHECK(processMediaDescription(fileHdl, compMob, 
						  tabLevel));
		  }

		CHECK(traverseMob(fileHdl, compMob, tabLevel));
	      }

	    omfiIteratorDispose(fileHdl, mobIter);
	    mobIter = NULL;
	    CHECK(omfsCloseFile(fileHdl));
	    CHECK(omfsEndSession(session));
	    printf("ReadComp completed successfully.\n");
	  } /* XPROTECT */

	XEXCEPT
	  {	  
	    if (name)
	      omfsFree(name);
	    if (mobIter)
	      omfiIteratorDispose(fileHdl, mobIter);
	    omfsCloseFile(fileHdl);
	    omfsEndSession(session);
	    printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
	    return(-1);
	  }
	XEND;
	return(0);
}

omfErr_t traverseMob(omfHdl_t file,
		omfMobObj_t mob,
		omfInt32 tabLevel)
{
    omfInt32 numSlots, nameSize, numTracks;
    omfInt32 loop;
    char trackName[32];
    omfMSlotObj_t track;
    omfSegObj_t trackSeg;
    omfIterHdl_t trackIter = NULL;
    omfPosition_t origin;
    char	 originBuf[32];
    omfTrackID_t trackID;
    omfRational_t editRate;
    char printString[128];
    omfErr_t omfError = OM_ERR_NONE;

    XPROTECT(file)
      {
	tabLevel++;

	CHECK(omfiIteratorAlloc(file, &trackIter));

	CHECK(omfiMobGetNumSlots(file, mob, &numSlots));
	CHECK(omfiMobGetNumTracks(file, mob, &numTracks));
	sprintf(printString, "Num Mob Slots: %ld, Num Mob Tracks: %ld\n",
		numSlots, numTracks);
	printInfo(tabLevel, printString);

	for (loop = 0; loop < numSlots; loop++)
	  {
	    CHECK(omfiMobGetNextSlot(trackIter, mob, NULL, &track));
	    CHECK(omfiMobSlotGetInfo(file, track, &editRate, &trackSeg));
	    sprintf(printString, "MOBSLOT - EditRate %ld/%ld\n", 
		    editRate.numerator, editRate.denominator);
	    printInfo(tabLevel, printString);
	    tabLevel++;
	    if (omfiMobSlotIsTrack(file, track, &omfError))
	      {
		nameSize = 32;
		CHECK(omfiTrackGetInfo(file, mob, track, &editRate,
				       nameSize, (omfString)trackName, 
				       &origin, &trackID, &trackSeg));
  		CHECK(omfsInt64ToString(origin, 10, sizeof(originBuf), originBuf));  
		if (trackID == 0)
		  sprintf(printString, "Name %s, Origin %s, NO trackID\n",
			  trackName, originBuf, trackID);
		else
		  sprintf(printString, "Name %s, Origin %ld, trackID %ld\n",
			  trackName, originBuf, trackID);
		printInfo(tabLevel, printString);
	      }
	    CHECK(processComponent(file, mob, editRate, trackSeg, tabLevel));
	    tabLevel--;
	  }

	CHECK(omfiIteratorDispose(file, trackIter));
	trackIter = NULL;

	tabLevel--;
      } /* XPROTECT */

    XEXCEPT
      {
	if (trackIter)
	  omfiIteratorDispose(file, trackIter);
	return(XCODE());
      } 
    XEND;
    return(OM_ERR_NONE);
}

omfErr_t processComponent(omfHdl_t file, 
					omfObject_t containingMob,
					omfRational_t editRate,
	                 omfCpntObj_t cpnt,
	                 omfInt32 tabLevel)
{
    omfInt32 loop, numSegs, numSlots, valueSize32;
    omfLength_t	valueSize;
    omfPosition_t cutPoint;
    char printString[128];
    omfLength_t length, bytesRead;
    omfPosition_t sequPos;
    omfCpntObj_t sequCpnt;
    omfSegObj_t slotSeg, selected;
    omfEffObj_t effect;
    omfIterHdl_t segIter = NULL, scopeIter = NULL, selIter = NULL;
    omfTimecode_t timecode;
    omfEdgecode_t edgecode;
    omfSourceRef_t sourceRef;
    omfDDefObj_t datakind;
    omfInterpKind_t interpolation = (omfInterpKind_t)-1;
    omfInt32 fadeInLen, fadeOutLen;
    omfFadeType_t fadeInType, fadeOutType;
    void *value = NULL;
    char		lengthBuf[32], sequPosBuf[32], posBuf[32], cutBuf[32];
    omfUInt32 relScope, relSlot;
    omfErr_t omfError = OM_ERR_NONE;
   	omfFileRev_t fileRev;
  	omfProperty_t idProp;
	omfBool		fadeInPresent, fadeOutPresent;
	omfDefaultFade_t	defaultFade;
	omfPosition_t		fadeLenpos;
	omfErr_t			status;
	
    XPROTECT(file)
      {
		CHECK(omfsFileGetRev(file,  &fileRev));
		if ((fileRev == kOmfRev1x) ||(fileRev == kOmfRevIMA))
		{
			idProp = OMObjID;
		}
		else /* kOmfRev2x */
		{
			idProp = OMOOBJObjClass;
		}

	tabLevel++;
	if (omfiIsASequence(file, cpnt, &omfError))
	  {
	    CHECK(omfiIteratorAlloc(file, &segIter));
	    CHECK(omfiSequenceGetInfo(file, cpnt, &datakind, &length));
 		CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
	    sprintf(printString, "SEQUENCE - Length %s\n", lengthBuf);
	    printInfo(tabLevel, printString);
	    tabLevel++;
	    printDatakind(file, datakind, tabLevel);

	    CHECK(omfiSequenceGetNumCpnts(file, cpnt, &numSegs));
	    for (loop = 0; loop < numSegs; loop++)
	      {
		CHECK(omfiSequenceGetNextCpnt(segIter, cpnt, NULL, &sequPos, 
					  &sequCpnt));
 		CHECK(omfsInt64ToString(sequPos, 10, sizeof(sequPosBuf), sequPosBuf));  
		sprintf(printString, "Sequence current position %s\n",
			sequPosBuf);
		printInfo(tabLevel, printString);
		CHECK(processComponent(file, containingMob, editRate, sequCpnt, tabLevel));
	      }
	    omfiIteratorDispose(file, segIter);
	    segIter = NULL;
	    tabLevel--;
	  }
	else if (omfiIsASourceClip(file, cpnt, &omfError))
	{
/*	  CHECK(omfiSourceClipGetRef(file, cpnt, &sourceRef)); */
	  CHECK(omfiSourceClipGetInfo(file, cpnt, &datakind, &length, 
				      &sourceRef));
 	  CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
	  sprintf(printString, "SOURCECLIP - length %s\n", lengthBuf);
	  printInfo(tabLevel, printString);
	  tabLevel++;
	  printDatakind(file, datakind, tabLevel);
 	  CHECK(omfsInt64ToString(sourceRef.startTime, 10, sizeof(posBuf), posBuf));  
	  sprintf(printString, "MobID %ld.%ld.%ld, Track %ld, Position %s\n",
		  sourceRef.sourceID.prefix,
		  sourceRef.sourceID.major, sourceRef.sourceID.minor,
		  sourceRef.sourceTrackID, posBuf);
	  printInfo(tabLevel, printString);
	  omfError = omfiSourceClipGetFade(file, cpnt,
			   &fadeInLen, &fadeInType, &fadeInPresent,
			   &fadeOutLen, &fadeOutType, &fadeOutPresent);
	  if ((omfError == OM_ERR_NONE) &&
	      (fileRev != kOmfRev1x) &&
	      (fileRev != kOmfRevIMA))
	  {
	  	if(!fadeInPresent || !fadeOutPresent)
	  	{
	  		if(containingMob != NULL && omfiIsACompositionMob(file, containingMob, &status))
	  		{
				CHECK(omfiMobGetDefaultFade(file, containingMob, &defaultFade));
				if(defaultFade.valid)
				{
/* !!! Need macros for where it's OK to xlate between length and position */
#if PORT_USE_NATIVE64
					fadeLenpos = (omfPosition_t)defaultFade.fadeLength;
#else
					fadeLenpos.words[0] = defaultFade.fadeLength.words[0];
					fadeLenpos.words[1] = defaultFade.fadeLength.words[1];
					fadeLenpos.words[2] = defaultFade.fadeLength.words[2];
					fadeLenpos.words[3] = defaultFade.fadeLength.words[3];
#endif
					CHECK(omfiConvertEditRate(defaultFade.fadeEditUnit,
										fadeLenpos,
										editRate, kRoundCeiling,
										(omfPosition_t *)&fadeInLen));
					fadeOutLen = fadeInLen;
					fadeInType = defaultFade.fadeType;
					fadeOutType = defaultFade.fadeType;
			  		fadeInPresent = TRUE;
			  		fadeOutPresent = TRUE;
				}
			}
	  	}
	  
	    if(fadeInPresent && fadeOutPresent)
	    	sprintf(printString, "FadeInfo - In: %ld %ld Out: %ld %ld\n",
		   		 fadeInLen, fadeInType, fadeOutLen, fadeOutType);
	    else if(fadeInPresent)
	    	sprintf(printString, "FadeInfo - In: %ld %ld\n",
		   		 fadeInLen, fadeInType);
	    else if(fadeOutPresent)
	    	sprintf(printString, "FadeInfo - Out: %ld %ld\n",
		   		 fadeOutLen, fadeOutType);
	  }
	  else if (omfError == OM_ERR_PROP_NOT_PRESENT)
	    {
	      sprintf(printString, "Fade Info does not exist.\n");
	      omfError = OM_ERR_NONE;
	    }
	  else
	    {
	      RAISE(omfError);
	    }
	  printInfo(tabLevel, printString);
	  tabLevel--;
	}
	else if (omfiIsATimecodeClip(file, cpnt, &omfError))
	  {
	    CHECK(omfiTimecodeGetInfo(file, cpnt, &datakind, &length,
					      &timecode));
 	  	CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
	    sprintf(printString, 
		    "TIMECODE - Length %s, StartFrame %ld, drop %d, fps %d\n",
		    lengthBuf, timecode.startFrame, timecode.drop, timecode.fps);
	    printInfo(tabLevel, printString);
	    tabLevel++;
	    printDatakind(file, datakind, tabLevel);
	    tabLevel--;
	  }
	else if (omfiIsAnEdgecodeClip(file, cpnt, &omfError))
	  {
	    CHECK(omfiEdgecodeGetInfo(file, cpnt,
					&datakind, &length, &edgecode));
 	  	CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
	    sprintf(printString,
		    "EDGECODE - Length %s, StartFrame %ld, filmKind %d, codeFormat %d\n",
		    lengthBuf, edgecode.startFrame, edgecode.filmKind, 
		    edgecode.codeFormat);
	    printInfo(tabLevel, printString);
	    tabLevel++;
	    printDatakind(file, datakind, tabLevel);
	    tabLevel--;
	}
	else if (omfiIsAFiller(file, cpnt, &omfError))
	 {
	   CHECK(omfiFillerGetInfo(file, cpnt, &datakind, &length));
 	   CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
	   sprintf(printString, "FILLER - length %s\n", lengthBuf);
	   printInfo(tabLevel, printString);
	   tabLevel++;
	   printDatakind(file, datakind, tabLevel);
	   tabLevel--;
	 }
	else if (omfiIsAnEffect(file, cpnt, &omfError))
	  {
	    CHECK(processEffect(file, cpnt, editRate, tabLevel));
	  }
	else if (omfiIsATransition(file, cpnt, &omfError))
	  {
	    CHECK(omfiTransitionGetInfo(file, cpnt, &datakind,
					     &length, &cutPoint, &effect));
 	    CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
 	    CHECK(omfsInt64ToString(cutPoint, 10, sizeof(cutBuf), cutBuf));  

	    sprintf(printString, "TRANSITION - length %s, cutPoint %s\n", lengthBuf, cutBuf);
	    printInfo(tabLevel, printString);
	    tabLevel++;
	    printDatakind(file, datakind, tabLevel);
	    tabLevel--;
	  }
	else if (omfiIsAConstValue(file, cpnt, &omfError))
	  {
	    CHECK(omfiDataValueGetSize(file, cpnt, &valueSize));
	    if(omfsTruncInt64toInt32(valueSize, &valueSize32) == OM_ERR_NONE)	/* OK EFFECTPARM */
	    {
		    value = omfsMalloc(valueSize32);
		    CHECK(omfiConstValueGetInfo(file, cpnt, &datakind, &length, 
						     valueSize, &bytesRead, value));
 	    	CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
		    sprintf(printString, "CONSTVAL - length %s\n", lengthBuf);
		    printInfo(tabLevel, printString);
		    tabLevel++;
		    printDatakind(file, datakind, tabLevel);
		    printDataValue(file, datakind, valueSize32, value, tabLevel);
		    omfsFree(value);
		    tabLevel--;
	    }
	    else
	    	printInfo(tabLevel, "Data value greater than 4 gigabytes int .\n");
	  }
	else if (omfiIsAVaryValue(file, cpnt, &omfError))
	  {
	    CHECK(omfiVaryValueGetInfo(file, cpnt, &datakind, &length,
					     &interpolation));
 	    CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
	    sprintf(printString, "VARYVAL - length %s, interpolation %ld\n", 
		    lengthBuf, interpolation);
	    printInfo(tabLevel, printString);
	    tabLevel++;
	    printDatakind(file, datakind, tabLevel);
	    CHECK(processControlPoints(file, cpnt, tabLevel));
	    tabLevel--;
	  }
	else if (omfiIsANestedScope(file, cpnt, &omfError))
	  {
	    CHECK(omfiNestedScopeGetInfo(file, cpnt, &datakind, &length));
 	    CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
	    sprintf(printString, "SCOPE - length %s\n", lengthBuf);
	    printInfo(tabLevel, printString);
	    tabLevel++;
	    printDatakind(file, datakind, tabLevel);
	    CHECK(omfiIteratorAlloc(file, &scopeIter));
	    CHECK(omfiNestedScopeGetNumSlots(file, cpnt, &numSlots));
	    for (loop = 0; loop < numSlots; loop++)
	      {
		CHECK(omfiNestedScopeGetNextSlot(scopeIter, cpnt, NULL, &slotSeg));
		CHECK(processComponent(file, containingMob, editRate, slotSeg, tabLevel));
	      }
	    omfiIteratorDispose(file, scopeIter);
	    scopeIter = NULL;
	    tabLevel--;
	  }
	else if (omfiIsAScopeRef(file, cpnt, &omfError))
	  {
	    CHECK(omfiScopeRefGetInfo(file, cpnt, &datakind, &length,
				      &relScope, &relSlot));
 	    CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
	    sprintf(printString, "SCOPE REFERENCE - length %s, RelScope %ld, RelSlot %ld\n", lengthBuf, relScope, relSlot);
	    printInfo(tabLevel, printString);
	    tabLevel++;
	    printDatakind(file, datakind, tabLevel);
	    tabLevel--;
	  }
	else if (omfiIsASelector(file, cpnt, &omfError))
	  {
	    CHECK(omfiSelectorGetInfo(file, cpnt, &datakind, &length, 
					   &selected));
 	    CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
	    sprintf(printString, "SELECTOR - length %s\n", lengthBuf);
	    printInfo(tabLevel, printString);
	    tabLevel++;
	    printDatakind(file, datakind, tabLevel);
	    sprintf(printString, "Selected:\n");
	    printInfo(tabLevel, printString);
	    CHECK(processComponent(file, containingMob, editRate, selected, tabLevel));

	    sprintf(printString, "Alternates:\n");
	    printInfo(tabLevel, printString);
	    CHECK(omfiIteratorAlloc(file, &selIter));
	    CHECK(omfiSelectorGetNumAltSlots(file, cpnt, &numSlots));
	    for (loop = 0; loop < numSlots; loop++)
	      {
		CHECK(omfiSelectorGetNextAltSlot(selIter, cpnt, NULL, 
						 &slotSeg));
		CHECK(processComponent(file, containingMob, editRate, slotSeg, tabLevel));
	      }	    
	    omfiIteratorDispose(file, selIter);
	    selIter = NULL;
	    tabLevel--;
	  }
	else if (omfiIsAMediaGroup(file, cpnt, &omfError))
	 {
	   CHECK(omfiMediaGroupGetInfo(file, cpnt, &datakind, &length));
 	   CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
	   sprintf(printString, "MediaGroup - length %s\n", lengthBuf);
	   printInfo(tabLevel, printString);
	   tabLevel++;
	   printDatakind(file, datakind, tabLevel);
	   tabLevel--;
	 }
	else 
	  {
	    char data[5];
	    CHECK(omfsReadClassID(file, cpnt, idProp, data));
	    data[4] = '\0';
	    sprintf(printString, "UNKNOWN OBJECT: %s\n", data);
	    printInfo(tabLevel, printString);
	  }

	tabLevel--;
	if (!gMakeReadCompSilent)
	  printf("\n");
      } /* XPROTECT */

    XEXCEPT
      {
	if (segIter)
	  omfiIteratorDispose(file, segIter);
	if (scopeIter)
	  omfiIteratorDispose(file, scopeIter);
	if (selIter)
	  omfiIteratorDispose(file, selIter);
	if (value)
	  omfsFree(value);
	return(XCODE());
      }
    XEND;

    return(OM_ERR_NONE);
}

omfErr_t processEffect(omfHdl_t file, 
		       omfSegObj_t effect,
		       omfRational_t	editRate,
		       omfInt32 tabLevel)
{
    omfInt32 idSize = 64, nameSize = 64, descSize = 120, numRender;
    omfUID_t ID;
    omfLength_t length;
    omfEDefObj_t effectDef;
    omfDDefObj_t datakind;
    char printString[128], nameBuf[64], descBuf[120];
    omfUniqueName_t effectID;
    omfArgIDType_t bypass, argID;
    omfBool isTimeWarp;
    omfIterHdl_t effectIter = NULL, renderIter = NULL;
    omfInt32 numSlots, loop;
    omfESlotObj_t slot;
    omfSegObj_t argValue, renderClip;
    omfSourceRef_t sourceRef;
    omfObject_t fileMob;
    omfErr_t omfError = OM_ERR_NONE;
    char	lengthBuf[32], posBuf[32];

    XPROTECT(file)
      {
	CHECK(omfiEffectGetInfo(file, effect, &datakind, &length, &effectDef));
 	CHECK(omfsInt64ToString(length, 10, sizeof(lengthBuf), lengthBuf));  
	sprintf(printString, "EFFECT - length %s\n", lengthBuf);
	printInfo(tabLevel, printString);
	tabLevel++;
	printDatakind(file, datakind, tabLevel);

	/* Get Effect Definition Information */
	CHECK(omfiEffectDefGetInfo(file, effectDef, idSize, effectID,
					    nameSize, nameBuf,
					    descSize, descBuf,
					    &bypass, &isTimeWarp));
	sprintf(printString, "EDEF - ID: %s\n", effectID);
	printInfo(tabLevel, printString);
	tabLevel++;
	sprintf(printString, "Name: %s\n", nameBuf);
	printInfo(tabLevel, printString);
	sprintf(printString, "Desc: %s\n", descBuf);
	printInfo(tabLevel, printString);
	sprintf(printString, "Bypass: %ld\n", bypass);
	printInfo(tabLevel, printString);
	tabLevel--;
	if (omfiEffectIsATimeWarp(file, effect, &omfError))
	  sprintf(printString, "Effect IS a Timewarp\n");
	else
	  sprintf(printString, "Effect IS NOT a Timewarp\n");
	printInfo(tabLevel, printString);

	/* Get Final Render, if exists */
	omfError = omfiEffectGetFinalRender(file, effect, &renderClip);
	if (omfError == OM_ERR_NONE)
	  {
	    CHECK(omfiEffectGetNumFinalRenders(file, effect, &numRender));
	    sprintf(printString, "There are %ld Final renderings\n",
		    numRender);
	    printInfo(tabLevel, printString);
	    if (numRender)
	      {
		CHECK(omfiIteratorAlloc(file, &renderIter));
		omfiEffectGetNextFinalRender(renderIter, effect, NULL, 
					     &fileMob); 
		CHECK(omfiMobGetMobID(file, fileMob, &ID));
		sprintf(printString, "Final Render File MOB: MobID %ld.%ld.%ld\n",
			ID.prefix, ID.major, ID.minor);
		printInfo(tabLevel, printString);
		CHECK(omfiSourceClipGetInfo(file, renderClip, NULL, NULL, 
					    &sourceRef));
/*		CHECK(omfiSourceClipGetRef(file, renderClip, &sourceRef)); */

		CHECK(omfsInt64ToString(sourceRef.startTime, 10, sizeof(posBuf), posBuf));  
		sprintf(printString,
		    "Final Render -  MobID %ld.%ld.%ld, Track %ld, Position %s\n",
			sourceRef.sourceID.prefix, 
			sourceRef.sourceID.major, 
			sourceRef.sourceID.minor, 
			sourceRef.sourceTrackID, 
			posBuf);
	      }
	  }
	else if (omfError == OM_ERR_PROP_NOT_PRESENT)
	  {
	    sprintf(printString, "Final Render does not exist.\n");
	    omfError = OM_ERR_NONE;
	  }
	else
	  {
	    RAISE(omfError);
	  }
	printInfo(tabLevel, printString);

	/* Get Working Render, if exists */
	omfError = omfiEffectGetWorkingRender(file, effect, &renderClip);
	if (omfError == OM_ERR_NONE)
	  {
	    CHECK(omfiEffectGetNumWorkingRenders(file, effect, &numRender));
	    sprintf(printString, "There are %ld Working renderings\n",
		    numRender);
	    printInfo(tabLevel, printString);
	    if (numRender)
	      {
		CHECK(omfiIteratorClear(file, renderIter));
		omfiEffectGetNextWorkingRender(renderIter, effect, NULL, 
					       &fileMob);
		CHECK(omfiMobGetMobID(file, fileMob, &ID));
		sprintf(printString, "Working  Render File MOB: MobID %ld.%ld.%ld\n", 
			ID.prefix, ID.major, ID.minor);
		printInfo(tabLevel, printString);

		CHECK(omfiSourceClipGetInfo(file, renderClip, NULL, NULL, 
					    &sourceRef));
/*		CHECK(omfiSourceClipGetRef(file, renderClip, &sourceRef)); */
 		CHECK(omfsInt64ToString(sourceRef.startTime, 10, sizeof(posBuf), posBuf));  
		sprintf(printString,
		    "Working Render - MobID %ld.%ld.%ld, Track %ld, Position %s\n",
			sourceRef.sourceID, sourceRef.sourceTrackID, 
			posBuf);
	      }
	  }
	else if (omfError == OM_ERR_PROP_NOT_PRESENT)
	  {
	    sprintf(printString, "Working Render does not exist.\n");
	    omfError = OM_ERR_NONE;
	  }
	else
	  {
	    RAISE(omfError);
	  }

	if (renderIter)
	  {
	    omfiIteratorDispose(file, renderIter);
	    renderIter = NULL;
	  }

	printInfo(tabLevel, printString);

	/* Get bypass override, if exists */
	omfError = omfiEffectGetBypassOverride(file, effect, &bypass);
	if (omfError == OM_ERR_NONE)
	    sprintf(printString, "Bypass Override - %ld\n", bypass);
	else if (omfError == OM_ERR_PROP_NOT_PRESENT)
	  {
	    sprintf(printString, "Bypass Override does not exist.\n");
	    omfError = OM_ERR_NONE;
	  }
	else
	  {
	    RAISE(OM_ERR_NONE);
	  }
	printInfo(tabLevel, printString);

	/* Iterate through Effect Slots and get information */
	CHECK(omfiIteratorAlloc(file, &effectIter));
	CHECK(omfiEffectGetNumSlots(file, effect, &numSlots));
	for (loop = 0; loop < numSlots; loop++)
	  {
	    CHECK(omfiEffectGetNextSlot(effectIter, effect,NULL, &slot));
	    CHECK(omfiEffectSlotGetInfo(file, slot, &argID, &argValue));
	    sprintf(printString, "ArgID: %ld\n", argID);
	    printInfo(tabLevel, printString);

	    CHECK(processComponent(file, NULL, editRate, argValue, tabLevel));
	  }
	omfiIteratorDispose(file, effectIter);
	effectIter = NULL;
	tabLevel--;
      } /* XPROTECT */

    XEXCEPT
      {
	if (renderIter)
	  omfiIteratorDispose(file, renderIter);
	if (effectIter)
	  omfiIteratorDispose(file, effectIter);
	return(XCODE());
      }
    XEND;

    return(OM_ERR_NONE);
}

omfErr_t processControlPoints(omfHdl_t file, 
			      omfCpntObj_t vValue, 
			      omfInt32 tabLevel)
{
    omfIterHdl_t pointIter = NULL;
    omfInt32 numPoints, loop, valueSize32, bytesRead;
    omfLength_t	valueSize;
    omfCntlPtObj_t control;
    omfDDefObj_t datakind;
    omfRational_t time;
    void *value = NULL;
    char printString[128];
    omfEditHint_t editHint;
    omfErr_t omfError = OM_ERR_NONE;

    XPROTECT(file)
      {
	CHECK(omfiIteratorAlloc(file, &pointIter));
	CHECK(omfiVaryValueGetNumPoints(file, vValue, &numPoints));
	for (loop = 0; loop < numPoints; loop++)
	  {
	    CHECK(omfiVaryValueGetNextPoint(pointIter, vValue, NULL, 
					    &control));
	    CHECK(omfiDataValueGetSize(file, control, &valueSize));
	    if(omfsTruncInt64toInt32(valueSize, &valueSize32) == OM_ERR_NONE)	/* OK EFFECTPARM */
	    {
		    value = omfsMalloc(valueSize32);
		    CHECK(omfiControlPtGetInfo(file, control, &time,
						 &editHint, &datakind, 
						 valueSize32, &bytesRead, value));
		    sprintf(printString, "CONTROLPT: Time: %ld/%ld\n",
			    time.numerator, time.denominator);
		    printInfo(tabLevel, printString);
		    printDatakind(file, datakind, tabLevel);
		    printDataValue(file, datakind, valueSize32, value, tabLevel);
		    omfsFree(value);
		   }
		  else
		  	printInfo(tabLevel, "Data value greater than 4 gigabytes\n");
	  }
	omfiIteratorDispose(file, pointIter);
	pointIter = NULL;
      } /* XPROTECT */

    XEXCEPT
      {
	if (pointIter)
	  omfiIteratorDispose(file, pointIter);
	if (value)
	  omfsFree(value);
	return(XCODE());
      }
    XEND;

    return(OM_ERR_NONE);
}

omfErr_t processMediaDescription(omfHdl_t file, 
				 omfMobObj_t mob,
				 omfInt32 tabLevel)
{
  omfObject_t mdes = NULL;
  omfIterHdl_t locatorIter = NULL;
  omfObject_t locator = NULL;
  char pathBuf[PATHSIZE];
  char printString[128], typeStr[5];
  omfClassID_t locType, mdesType;
  omfInt32 loop, numLocators;
   omfFileRev_t fileRev;
  omfProperty_t idProp;

  XPROTECT(file)
    {
		CHECK(omfsFileGetRev(file,  &fileRev));
		if ((fileRev == kOmfRev1x) ||(fileRev == kOmfRevIMA))
		{
			idProp = OMObjID;
		}
		else /* kOmfRev2x */
		{
			idProp = OMOOBJObjClass;
		}
      tabLevel++;

      CHECK(omfmMobGetMediaDescription(file, mob, &mdes));

      CHECK(omfsReadClassID(file, mdes, idProp, mdesType));
      strncpy(typeStr, mdesType, 4);
      typeStr[4] = '\0';
      sprintf(printString, "Media Descriptor %.4s\n", typeStr);
      printInfo(tabLevel, printString);

      if (mdes)
	{
	  tabLevel++;
	  CHECK(omfiIteratorAlloc(file, &locatorIter));
	  CHECK(omfmMobGetNumLocators(file, mob, &numLocators));

	  for (loop = 0; loop < numLocators; loop++)
	    {
	      CHECK(omfmMobGetNextLocator(locatorIter, mob, &locator));
	      CHECK(omfmLocatorGetInfo(file, locator, locType, PATHSIZE, 
				       pathBuf));
	      sprintf(printString, "Locator %.4s, Path %s\n", 
		      locType, pathBuf);
	      printInfo(tabLevel, printString);
	    }

	  omfiIteratorDispose(file, locatorIter);
	  locatorIter = NULL;
	  tabLevel--;
	}
      tabLevel--;
    }
  
  XEXCEPT
    {
      if (locatorIter)
	omfiIteratorDispose(file, locatorIter);
      tabLevel--;
    }
  XEND;

  return(OM_ERR_NONE);
}

void printInfo(omfInt32  tabLevel,
	    char *string)
{
	omfInt32 loop;

	if (!gMakeReadCompSilent) {
	  for (loop=1;loop<=tabLevel;loop++) printf("  ");
	    printf("%s", string);
        }
}

void printDatakind(omfHdl_t fileHdl,
	      omfDDefObj_t  def,
	      omfInt32 tabLevel)
{
  omfUniqueName_t name;
  char printString[128];
  omfErr_t omfError = OM_ERR_NONE;

  omfError = omfiDatakindGetName(fileHdl, def, OMUNIQUENAME_SIZE, name);
  if (omfError == OM_ERR_NONE)
    sprintf(printString, "Datakind Name = %s\n", name);
  else
    sprintf(printString, "***ERROR: %ld: %s\n", 
	    omfError, omfsGetErrorString(omfError));
  printInfo(tabLevel, printString);
}


void printDataValue(omfHdl_t fileHdl,
	      omfDDefObj_t def,
	      omfInt32 siz,
	      void *value,
	      omfInt32 tabLevel)
{
  char printString[128];
  char charValue;
  omfUInt8 uint8Value;
  omfUInt8 int8Value;
  omfUInt8 uint16Value;
  omfUInt8 int16Value;
  omfInt32 int32Value;
  omfUInt32 uint32Value;
  omfRational_t ratValue;
  omfBool boolValue;
  omfString stringValue;
  omfErr_t omfError = OM_ERR_NONE;

  printString[0] = '\0';
  if (!omfError && omfiIsCharKind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&charValue, value, siz);
      sprintf(printString, "DataValue = %c\n", charValue); 
    }

  if (!omfError && omfiIsUInt8Kind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&uint8Value, value, siz);
      sprintf(printString, "DataValue = %ld\n", uint8Value);
    }

  if (!omfError && omfiIsInt8Kind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&int8Value, value, siz);
      sprintf(printString, "DataValue = %ld\n", int8Value);
    }

  if (!omfError && omfiIsInt16Kind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&int16Value, value, siz);
      sprintf(printString, "DataValue = %ld\n", int16Value);
    }

  if (!omfError && omfiIsUInt16Kind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&uint16Value, value, siz);
      sprintf(printString, "DataValue = %ld\n", uint16Value);
    }

  if (!omfError && omfiIsInt32Kind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&int32Value, value, siz); 
      sprintf(printString, "DataValue = %ld\n", int32Value);
    }

  if (!omfError && omfiIsUInt32Kind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&uint32Value, value, siz); 
      sprintf(printString, "DataValue = %lu\n", uint32Value);
    }

  if (!omfError && omfiIsRationalKind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&ratValue, value, siz);
      sprintf(printString, "DataValue = %ld/%ld\n", ratValue.numerator,
	                                        ratValue.denominator);
    }

  if (!omfError && omfiIsBooleanKind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&boolValue, value, siz);
      sprintf(printString, "DataValue = %ld\n", boolValue);
    }

  if (!omfError && omfiIsStringKind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&stringValue, value, siz);
      sprintf(printString, "DataValue = %s\n", stringValue);
    }

  /* Enum: RGB, YUV, YIQ, HSI, HSV, YCrCb, YDrDb, CMYK */
  if (!omfError && omfiIsColorSpaceKind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&int32Value, value, siz);
      sprintf(printString, "DataValue = %ld\n", int32Value);
    }

  /* NOTE: colorspace identified followed by n rational color components,
     dependent on the colorspace */
  if (!omfError && omfiIsColorKind(fileHdl, def, kExactMatch, &omfError))
    {
      sprintf(printString, "DataValue is a colorspace value.\n");
    }


  if (!omfError && omfiIsDistanceKind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&ratValue, value, siz);
      sprintf(printString, "DataValue = %ld.%ld\n", ratValue.numerator,
	                                        ratValue.denominator);
    }

  /* Shouldn't be a rational */
  if (!omfError && omfiIsPointKind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&ratValue, value, siz);
      sprintf(printString, "DataValue = %ld.%ld\n", ratValue.numerator,
	                                        ratValue.denominator);
    }

  if (!omfError && omfiIsDirectionCodeKind(fileHdl, def, kExactMatch, &omfError))
    {
      memcpy(&int32Value, value, siz);
      sprintf(printString, "DataValue = %ld\n", int32Value);
    }

  /* NOTE: */
  /* UInt8 representing number of terms (N) followed by N rational
   * numbers representing the coefficients an an order N-1 poly,
   * listed in increasing order without omission of zeros 
   */
  if (!omfError && omfiIsPolynomialKind(fileHdl, def, kExactMatch, &omfError))
    {
/*    sprintf(printString, "DataValue = %ld\n", (omfInt32) *value); */
      sprintf(printString, "DataValue is a poly\n");
    }

  /* These kinds won't be encountered in compositions */
  /* Displayable picture */
  if (!omfError && omfiIsPictureKind(fileHdl, def, kExactMatch, &omfError))
    sprintf(printString, "DataValue is a displayable picture\n");

  if (!omfError && omfiIsMatteKind(fileHdl, def, kExactMatch, &omfError))
    sprintf(printString, "DataValue is a pixel array\n");

  if (!omfError && omfiIsPictureWithMatteKind(fileHdl, def, kExactMatch, &omfError))
    sprintf(printString, "DataValue is a picture with a matte key channel");

  if (!omfError && omfiIsSoundKind(fileHdl, def, kExactMatch, &omfError))
    sprintf(printString, "DataValue is a monaural audio stream");

  if (!omfError && omfiIsStereoSoundKind(fileHdl, def, kExactMatch, &omfError))
    sprintf(printString, "DataValue is a stereo audio stream");

  if (!omfError && omfiIsTimecodeKind(fileHdl, def, kExactMatch, &omfError))
    sprintf(printString, "DataValue is a stream of timecode values");

  if (!omfError && omfiIsEdgecodeKind(fileHdl, def, kExactMatch, &omfError))
    sprintf(printString, "DataValue is a stream of edgecode values");

  printInfo(tabLevel, printString);
}

