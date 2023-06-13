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
 * MkIll - This unittest creates an illegal 2.x OMF file to be used
 *         by other applications that do negative testing.  It should
 *         especially help test the semantic checking functionality.
 ********************************************************************/

#include "masterhd.h"
#define EFFATRACK -1
#define EFFBTRACK -2
#define EFFLEVEL -3
#define EFFARG 4
#define TRUE 1
#define FALSE 0
#define PRIMARY 1
#define NULL 0
#define FADEINLEN 2
#define FADEOUTLEN 2

#include <stdio.h>
#include <string.h>

#include "omPublic.h"
#include "omMedia.h"

#include "UnitTest.h"

#if PORT_SYS_MAC
#include <console.h>
#include <stdlib.h>
#include <Types.h>
#include <Fonts.h>
#include <QuickDraw.h>
#include <Windows.h>
#include <Menus.h>
#include <Dialogs.h>
#include <StandardFile.h>
extern GrafPtr thePort; 
#endif

static omfErr_t CreateCompositionMob(omfHdl_t    fileHdl,
				     omfSourceRef_t vSource,
				     omfSourceRef_t aSource1,
				     omfSourceRef_t aSource2,
				     omfMobObj_t    *compMob);
static omfErr_t LisaDissolveNew(omfHdl_t    fileHdl,
				omfSegObj_t   effInput1,
				omfSegObj_t   effInput2,
				omfRational_t *effInput3,
				omfLength_t  effLength,
				omfSourceRef_t effSource,
				omfEffObj_t   *newEffect);

/* Please excuse the lack of error handling code! */
#ifdef MAKE_TEST_HARNESS
int MkIlleg(char *filename)
{
    int argc;
    char *argv[2];
#else
int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t	session;
  omfHdl_t	fileHdl;
  omfSourceRef_t vSource, aSource1, aSource2, sourceRef;
  omfMobObj_t compMob, masterMob, fileMob;
  omfObject_t newClip, tcClip;
  omfObject_t filler, track, tapeMob;
  omfDDefObj_t pictureDef;
  omfTimecode_t timecode;
  omfRational_t editRate;
  omfUID_t mobID;
  omfErr_t omfError;
  omfPosition_t	zeroPos;
  omfPosition_t	clipPos5;
  omfPosition_t	clipPos8;
  omfPosition_t	clipPos13;
  omfPosition_t	clipPos17;
  omfLength_t	clipLen10;
  omfLength_t	clipLen50;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Make Illegal UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
  	ProductInfo.platform = NULL;

  XPROTECT(NULL)
  {
#ifdef MAKE_TEST_HARNESS
    argc = 2;
    argv[1] = filename;
#else
#if PORT_MAC_HAS_CCOMMAND
    argc = ccommand(&argv); 

    /*
     *	Standard Toolbox stuff
     */
#if PORT_MAC_QD_SEPGLOBALS
    InitGraf	(&thePort);
#else
    InitGraf	(&qd.thePort);
#endif
    InitFonts	();
    InitWindows	();
    InitMenus	();
    TEInit();
    InitDialogs	((int )NULL);
    InitCursor	();
    
    where.h = 20;
    where.v = 40;
#endif
#endif
    
    if (argc < 2)
      {
	printf("*** ERROR - missing file name\n");
	return(1);
      }

    omfsCvtInt32toPosition(0, zeroPos);
    omfsCvtInt32toPosition(5, clipPos5);
    omfsCvtInt32toPosition(8, clipPos8);
    omfsCvtInt32toPosition(13, clipPos13);
    omfsCvtInt32toPosition(17, clipPos17);
    omfsCvtInt32toLength(10, clipLen10);
    omfsCvtInt32toLength(50, clipLen50);

    CHECK(omfsBeginSession(&ProductInfo, &session));
    CHECK(omfsCreateFile((fileHandleType) argv[1], session, 
			      kOmfRev2x, &fileHdl));

    CHECK(omfmInit(session));

    /* ... Get source references from somewhere for Source Clips ... */
    CHECK(omfiMobIDNew(fileHdl, &vSource.sourceID));
    vSource.sourceTrackID = 10;
    vSource.startTime = clipPos5;
    CHECK(omfiMobIDNew(fileHdl, &aSource1.sourceID));
    aSource1.sourceTrackID = 20;
    aSource1.startTime = clipPos8;
    CHECK(omfiMobIDNew(fileHdl, &aSource2.sourceID));
    aSource2.sourceTrackID = 30;
    aSource2.startTime = clipPos13;

    /* Create a bunch of empty master mobs with the above mob ids, just for
     * completeness.
     */
    omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
    editRate.numerator = 2997;
    editRate.denominator = 100;


    /* Create a timecode track on a tapemob */
    CHECK(omfmTapeMobNew(fileHdl, "TapeMob", &tapeMob));

    timecode.startFrame = 0;
    timecode.drop = kTcDrop;
    timecode.fps = 30;
    CHECK(omfiTimecodeNew(fileHdl, clipLen50, timecode, &tcClip));
    CHECK(omfiMobAppendNewTrack(fileHdl, tapeMob, editRate, tcClip,
				zeroPos, 3, NULL, &track));


    /* Create a file mob for the video master mob to point to */
    CHECK(omfmFileMobNew(fileHdl, "FileMob", editRate, "TIFF", &fileMob));

    CHECK(omfiMobGetMobID(fileHdl, fileMob, &mobID));
    printf("MobID: %d.%d.%d\n", mobID.prefix, mobID.major, mobID.minor);
    sourceRef.sourceTrackID = 3;
    sourceRef.startTime = clipPos17;
    sourceRef.sourceID.prefix = 0;
    sourceRef.sourceID.major = 0;
    sourceRef.sourceID.minor = 0;
    CHECK(omfiSourceClipNew(fileHdl, pictureDef,  clipLen50, sourceRef, 
			    &newClip));
    CHECK(omfiMobAppendNewTrack(fileHdl, fileMob, editRate, newClip,
				zeroPos, 3, NULL, &track));

    /* Create a video master mob with a media group */

    CHECK(omfmMasterMobNew(fileHdl, "MasterMob", FALSE, &masterMob));
    CHECK(omfiMobSetIdentity(fileHdl, masterMob, aSource1.sourceID));
    CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen10, &filler));
    CHECK(omfiMobAppendNewTrack(fileHdl, masterMob, editRate, filler,
				zeroPos, 20, NULL, &track));

    CHECK(omfmMasterMobNew(fileHdl, "MasterMob", FALSE, &masterMob));
    CHECK(omfiMobSetIdentity(fileHdl, masterMob, aSource2.sourceID));
    CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen10, &filler));
    CHECK(omfiMobAppendNewTrack(fileHdl, masterMob, editRate, filler,
				zeroPos, 20, NULL, &track));

    CHECK(CreateCompositionMob(fileHdl, vSource, aSource1, aSource2,  
			       &compMob));
    CHECK(omfsCloseFile(fileHdl));
    CHECK(omfsEndSession(session));

    printf("MkIlleg completed successfully.\n");
  } /* XPROTECT */

  XEXCEPT
    {
      printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
      return(-1);
    }
  XEND;
  return(0);
}

static omfErr_t CreateCompositionMob(omfHdl_t    fileHdl,
				     omfSourceRef_t vSource,
				     omfSourceRef_t aSource1,
				     omfSourceRef_t aSource2,
				     omfMobObj_t    *compMob)
{
    omfString	strBuf[64];
    omfRational_t	editRate, dissolveLevel, rationalValue;
    omfLength_t	trackLength;
    omfTimecode_t	timecode;
    omfEdgecode_t	edgecode;
    omfMSlotObj_t	vTrack1, aTrack2, aTrack3, tcTrack4, ecTrack5;
    omfSegObj_t	vSequence, dissolveTrackA, dissolveTrackB;
    omfSegObj_t	tcClip, aClip1, aClip2, vClip1, vClip2, ecClip;
    omfSegObj_t	vFiller1, vFiller2, vScope, scopeValue, vSelector;
    omfSegObj_t	vDissolve1, vDissolve2, vTransition, selValue;
    omfDDefObj_t	pictureDef, soundDef, rationalDef;
    omfTrackID_t	trackID;
    omfUID_t newMobID;
    omfErr_t omfError;
    omfPosition_t	zeroPos;
    omfPosition_t	clipOffset6;
    omfLength_t	clipLen10;
    omfLength_t	clipLen15;
    omfLength_t	clipLen50;
    omfLength_t	clipLen60;

    XPROTECT(fileHdl)
      {
	editRate.numerator = 2997;
	editRate.denominator = 100;
	omfsCvtInt32toPosition(0, zeroPos);
	omfsCvtInt32toLength(10, clipLen10);
	omfsCvtInt32toLength(15, clipLen15);
	omfsCvtInt32toLength(50, clipLen50);
	omfsCvtInt32toLength(60, clipLen60);
	omfsCvtInt32toPosition(60, clipOffset6);
	trackLength = clipLen60;
	
	omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
	omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
	omfiDatakindLookup(fileHdl, RATIONALKIND, &rationalDef, &omfError);

	/* Create the empty composition mob */	
	strncpy((char *)strBuf, "sampMob", 8);	
	CHECK(omfiCompMobNew(fileHdl, (omfString) strBuf, PRIMARY, compMob));

	CHECK(omfiMobSetPrimary(fileHdl, *compMob, PRIMARY));
	CHECK(omfiMobIDNew(fileHdl, &newMobID));
	CHECK(omfiMobSetIdentity(fileHdl, *compMob, newMobID));
	CHECK(omfiMobSetName(fileHdl, *compMob, "Lisa Mob"));

	/* Create timecode Clip and append to timecode tracks */
	timecode.startFrame = 0;
	timecode.drop = kTcDrop;
	timecode.fps = 30;
	CHECK(omfiTimecodeNew(fileHdl, clipLen60, timecode, &tcClip));
	trackID = 4;
	CHECK(omfiMobAppendNewSlot(fileHdl, *compMob, editRate, tcClip,
		&tcTrack4));

	/* Create audio source clips and append to audio tracks */
	CHECK(omfiSourceClipNew(fileHdl, soundDef, trackLength, 
		aSource1, &aClip1));
	CHECK(omfiSourceClipSetFade(fileHdl, aClip1, FADEINLEN, kFadeLinearAmp,
				FADEOUTLEN, kFadeLinearAmp));

	CHECK(omfiSourceClipNew(fileHdl, soundDef, trackLength, 
		aSource2, &aClip2));

	trackID = 2;
	CHECK(omfiMobAppendNewTrack(fileHdl, *compMob, editRate, aClip1,
				zeroPos, trackID, "ATrack2", &aTrack2));

	trackID = 3;
	CHECK(omfiMobAppendNewSlot(fileHdl, *compMob, editRate, aClip2, 
		&aTrack3));

	/* Create video track w/sequence: 
         *           FILL, SCLP, TRAN, SCLP, EFFI, NEST, SLCT, FILL 
         */
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen10, &vFiller1));
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen15, vSource, 
				&vClip1));
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen15, vSource, 
				&vClip2));
	CHECK(omfiSourceClipSetRef(fileHdl, vClip2, vSource));

	/* Create transition with dissolve effect, TrackA and TrackB implied */
	dissolveLevel.numerator = 2;
	dissolveLevel.denominator = 3;
	CHECK(LisaDissolveNew(fileHdl, NULL, NULL, &dissolveLevel, clipLen10, 
				     vSource, &vDissolve1));
	CHECK(omfiTransitionNew(fileHdl, pictureDef, clipLen10, clipOffset6, vDissolve1,
				       &vTransition));

	/* Create effect, use same video sources as clips in transition */
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen10, vSource, 
				       &dissolveTrackA));
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen10, vSource, 
				       &dissolveTrackB));
	CHECK(LisaDissolveNew(fileHdl, dissolveTrackA, dissolveTrackB, 
				     &dissolveLevel, clipLen10, vSource, 
				     &vDissolve2));

	/* Create nested scope */
	CHECK(omfiNestedScopeNew(fileHdl, pictureDef, clipLen10, &vScope));
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen10, vSource, 
				       &scopeValue));
	CHECK(omfiNestedScopeAppendSlot(fileHdl, vScope, scopeValue));
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen10, &scopeValue));
	CHECK(omfiNestedScopeAppendSlot(fileHdl, vScope, scopeValue));
	rationalValue.numerator = 1;
	rationalValue.denominator = 2;
	CHECK(omfiConstValueNew(fileHdl, rationalDef, clipLen10, 
					     sizeof(rationalValue),
					     (void *)&rationalValue, 
					     &scopeValue));
	CHECK(omfiNestedScopeAppendSlot(fileHdl, vScope, scopeValue));

	/* Create selector */
	CHECK(omfiSelectorNew(fileHdl, pictureDef, clipLen10, &vSelector));
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen10, vSource, 
				&selValue));
	CHECK(omfiSelectorSetSelected(fileHdl, vSelector, selValue));
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen10, &selValue));
	CHECK(omfiSelectorAddAlt(fileHdl, vSelector, selValue));

	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen10, &vFiller2));

	/* Build sequence with created segments */
	CHECK(omfiSequenceNew(fileHdl, pictureDef, &vSequence));
	CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vFiller1));
	CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vClip1));
	CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vTransition));
	CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vClip2));
	CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vDissolve2));
	CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vScope));
	CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vSelector));
	CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vFiller2));

	trackID = 1;
	CHECK(omfiMobAppendNewSlot(fileHdl, *compMob, editRate,
					  vSequence, &vTrack1));

	/* Create timecode Clip and append to timecode tracks */
	edgecode.startFrame = 0;
	edgecode.filmKind = kFt35MM;
	edgecode.codeFormat = kEtKeycode;
	strcpy(edgecode.header, "DevDesk");
	CHECK(omfiEdgecodeNew(fileHdl, clipLen60, edgecode, &ecClip));
	trackID = 5;
	CHECK(omfiMobAppendNewTrack(fileHdl, *compMob, editRate, ecClip,
				zeroPos, trackID, "ECTrack5", &ecTrack5));
      } /* XPROTECT */

    XEXCEPT
      {
	return(XCODE());
      }
    XEND;

    return(OM_ERR_NONE);

}


static omfErr_t LisaDissolveNew(omfHdl_t    fileHdl,
			 omfSegObj_t   effInput1,
			 omfSegObj_t   effInput2,
			 omfRational_t *effInput3,
			 omfLength_t  effLength,
			 omfSourceRef_t effSource,
			 omfEffObj_t   *newEffect)
{
    omfString	effName[64], effDesc[64];
    omfUniqueName_t	effID;
    omfEDefObj_t	effDef;
    omfArgIDType_t	bypass;
    omfESlotObj_t	effSlot1, effSlot2, effSlot3, effSlot4;
    omfDDefObj_t	rationalDef, pictureDef, int32Def;
    omfSegObj_t	constValueSeg, varyValueSeg, renderClip1, renderClip2;
    omfRational_t   pointTime;
    omfInt32           pointValue;
    omfEditHint_t   editHint;
    omfErr_t        omfError;

    XPROTECT(fileHdl)
      {
	omfiDatakindLookup(fileHdl, RATIONALKIND, &rationalDef, &omfError);
	omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
	omfiDatakindLookup(fileHdl, INT32KIND, &int32Def, &omfError);

	/* Set up the Effect Definition for Lisa's Dissolve Effect */
	strncpy((char *)effID, "omfi:effect:LisaDissolve", 25);
	strncpy((char *)effName, "Lisa's Segment Dissolve Effect", 32);
	strncpy((char *)effDesc, 
		"This is a very special effect. Blah Blah Blah", 64);
	bypass = EFFATRACK; /* The bypass is ATrack for this Dissolve effect */
	if (!omfiEffectDefLookup(fileHdl, effID, &effDef, &omfError))
	  {
	    CHECK(omfiEffectDefNew(fileHdl, effID, 	
				 (omfString)effName, (omfString)effDesc, 
				 &bypass, FALSE, &effDef));
	  }
	
	/* Create the effect instance, it returns an omfi:data:Picture */
	CHECK(omfiEffectNew(fileHdl, pictureDef, effLength, effDef, 
			      newEffect));

	/* Create and set final&working renderings and bypass override */
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, effLength, 
		effSource, &renderClip1));
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, effLength, 
		effSource, &renderClip2));
	CHECK(omfiEffectSetFinalRender(fileHdl, *newEffect, renderClip1));
	CHECK(omfiEffectSetWorkingRender(fileHdl, *newEffect, renderClip2));
	CHECK(omfiEffectSetBypassOverride(fileHdl, *newEffect, 0));

	if (effInput1)
	  CHECK(omfiEffectAddNewSlot(fileHdl, *newEffect, EFFATRACK, 
				       effInput1, &effSlot1));
	if (effInput2)
	  CHECK(omfiEffectAddNewSlot(fileHdl, *newEffect, EFFBTRACK, 
				       effInput2, &effSlot2));
	if (effInput3)
	{
	  CHECK(omfiConstValueNew(fileHdl, rationalDef, effLength, 
		    sizeof(*effInput3),
		    (void *)effInput3, &constValueSeg));
	  CHECK(omfiEffectAddNewSlot(fileHdl, *newEffect, EFFLEVEL,
					 constValueSeg, &effSlot3));
	}

	/* Add 4th slot with varying value */
	CHECK(omfiVaryValueNew(fileHdl, rationalDef, effLength, kLinearInterp,
				 &varyValueSeg));
	/* First control Point */
	pointTime.numerator = 2;
	pointTime.denominator = 3;
	pointValue = 20;
	editHint = kNoEditHint;
	CHECK(omfiVaryValueAddPoint(fileHdl, varyValueSeg, pointTime,
					editHint,
					int32Def, 
					sizeof(pointValue),
					(void *)&pointValue));
	/* Second control Point */
	pointTime.numerator = 1;
	pointTime.denominator = 3;
	pointValue = 10;
	editHint = kNoEditHint;
	CHECK(omfiVaryValueAddPoint(fileHdl, varyValueSeg, pointTime,
					editHint,
					int32Def, 
					sizeof(pointValue),
					(void *)&pointValue));

	CHECK(omfiEffectAddNewSlot(fileHdl, *newEffect, EFFARG,
					 varyValueSeg, &effSlot4));
	  
      } /* XPROTECT */
    
    XEXCEPT
      {
	return(XCODE());
      }
    XEND;

    return(OM_ERR_NONE);
}

