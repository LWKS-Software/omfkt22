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
 * MkComp2x - This unittest creates a fairly complicated composition
 *            in a 2.x file, that attempts to have one of every possible
 *            2.x object in it.  It also creates master and file mobs
 *            for video and audio to point to.  The video has real data,
 *            and the audio doesn't.  It also creates a tape mob, to derive
 *            the video data from (but not the audio data).  The composition
 *            will have effects, scopes, media groups, selectors, etc.
 ********************************************************************/
#include "masterhd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
#include "omMedia.h"

#include "UnitTest.h"

#if PORT_SYS_MAC
#include <Memory.h>
#include "MacSupport.h"
#endif

/***********/
/* DEFINES */
/***********/
#define EFFATRACK -1
#define EFFBTRACK -2
#define EFFLEVEL -3
#define EFFARG 4
#define TRUE 1
#define FALSE 0
#define PRIMARY 1
#define FADEINLEN 2
#define FADEOUTLEN 2
#define NAMESIZE 255

#define STD_WIDTH	320L
#define STD_HEIGHT	240L
#define FRAME_SAMPLES	(STD_WIDTH * STD_HEIGHT)
#define FRAME_BYTES	(FRAME_SAMPLES * 3)
#define MEDIATRACKID    1
#define AUDIO1TRACKID    1

/* GLOBALS */
static char           *stdWriteVBuffer = NULL;
static char           *stdReadVBuffer = NULL;
int		      nFrames = 20;
omfPosition_t         zeroPos;
omfLength_t         zeroLen;
omfRational_t         editRate;

/* STATIC PROTOTYPES */
static void     InitGlobals(void);
static omfErr_t CreateVideoMedia(omfHdl_t fileHdl, 
				 char *name, 
				 omfObject_t *returnMaster, 
				 omfObject_t *returnFile);
static omfErr_t CreateFakeAudioMedia(omfHdl_t fileHdl, 
				    char *name, 
				    omfObject_t *returnMaster, 
				    omfObject_t *returnFile);
static omfErr_t PatchTrackLength(omfHdl_t fileHdl,
				 omfMobObj_t mob,
				 omfInt32 length32);
static omfErr_t CreateCompositionMob(omfHdl_t    fileHdl,
				     omfSourceRef_t vSource,
				     omfSourceRef_t aSource1,
				     omfSourceRef_t aSource2,
				     omfMobObj_t    *compMob);
static omfErr_t CreateHugeCompositionMob(omfHdl_t    fileHdl,
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

/* MAIN PROGRAM */
#ifdef MAKE_TEST_HARNESS
int MkComp2x(char *filename)
{
	int		argc;
	char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t	session;
  omfHdl_t	fileHdl;
  omfSourceRef_t vSource, aSource1, aSource2;
  omfMobObj_t compMob = NULL, masterMob = NULL, fileMob = NULL, tapeMob = NULL, filmMob = NULL;
  omfDDefObj_t pictureDef, soundDef;
  omfLength_t	clipLen10;
  omfPosition_t	tcOffset; 
  omfErr_t omfError = OM_ERR_NONE;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Make Complex2x UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

  XPROTECT(NULL);
  {
#if PORT_SYS_MAC
    MaxApplZone();
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
	printf("*** ERROR - MkComp2x <filename>\n");
	return(1);
      }

    InitGlobals();

    /* Create file */
    CHECK(omfsBeginSession(&ProductInfo, &session));
    CHECK(omfmInit(session));
    CHECK(omfsCreateFile((fileHandleType) argv[1], session, 
			 kOmfRev2x, &fileHdl));

    /* Create a video and 2 audio file/master mobs for comps to reference */
    omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
    omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);

    /* Create standard tape mob with timecode track */
    CHECK(omfmStandardTapeMobNew(fileHdl, editRate, /* video */ 
				 editRate, /* audio */
				 "MkComp2x Tapemob", 
				 1, /* IN numVideoTracks */
				 2, /* IN numAudioTracks */
				 &tapeMob)); /* OUT TapeMob Returned object */

/*	CHECK(omfmFilmMobNew(fileHdl, "filmtest", &filmMob));
    omfsCvtInt32toPosition(200, tcOffset);
   	CHECK(omfmMobAddPhysSourceRef(fileHdl, tapeMob, editRate, 2,  
   					pictureDef, filmMob, tcOffset, 2, zeroLen));     
*/

    /* Create Video Media */
    CHECK(CreateVideoMedia(fileHdl, "Video Media", &masterMob, &fileMob));

    /* Link up tape mob to newly created video file mob */
    omfsCvtInt32toPosition(153, tcOffset);
    CHECK(omfmMobAddPhysSourceRef(fileHdl, fileMob, editRate,
				  MEDIATRACKID, /* From file mob's video trk */
				  pictureDef, 
				  tapeMob,	
				  tcOffset, 
				  2,  /* Video track in tape mob */
				  zeroLen)); /* SCLP already exists */

    /* Get video source reference from newly created video mobs */
    CHECK(omfiMobGetMobID(fileHdl, masterMob, &vSource.sourceID));
    vSource.sourceTrackID = MEDIATRACKID;
    omfsCvtInt32toInt64(5, &vSource.startTime);

    omfsCvtInt32toInt64(10, &clipLen10);

    /* Create fake audio media */
    CHECK(CreateFakeAudioMedia(fileHdl, "Audio Media 1", &masterMob,&fileMob));
    CHECK(omfiMobGetMobID(fileHdl, masterMob, &aSource1.sourceID));
    aSource1.sourceTrackID = AUDIO1TRACKID;
    omfsCvtInt32toInt64(8, &aSource1.startTime);

    CHECK(CreateFakeAudioMedia(fileHdl, "Audio Media 2", &masterMob,&fileMob));
    CHECK(omfiMobGetMobID(fileHdl, masterMob, &aSource2.sourceID));
    aSource2.sourceTrackID = AUDIO1TRACKID;
    omfsCvtInt32toInt64(5, &aSource2.startTime);

    printf("Creating composition\n");
    CHECK(CreateCompositionMob(fileHdl, vSource, aSource1, aSource2,  
			       &compMob));

    printf("Creating composition with 64-bit offsets\n");
    CHECK(CreateHugeCompositionMob(fileHdl, vSource, aSource1, aSource2,  
			       &compMob));

    CHECK(omfsCloseFile(fileHdl));
    CHECK(omfsEndSession(session));

    if (stdWriteVBuffer)
      omfsFree(stdWriteVBuffer);
    stdWriteVBuffer = NULL;
    if (stdReadVBuffer)
      omfsFree(stdReadVBuffer);
    stdReadVBuffer = NULL;

    printf("MkComp2x completed successfully.\n");
  } /* XPROTECT */

  XEXCEPT
    {
      if (stdWriteVBuffer)
	omfsFree(stdWriteVBuffer);
      if (stdReadVBuffer)
	omfsFree(stdReadVBuffer);
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
    omfSegObj_t aSequence1, aSequence2;
    omfSegObj_t	vFiller1, vFiller2, vScope, scopeValue, vSelector;
    omfSegObj_t	vDissolve1, vDissolve2, vTransition, selValue;
    omfDDefObj_t	pictureDef, soundDef, rationalDef;
    omfTrackID_t	trackID;
    omfUID_t newMobID;
    omfErr_t omfError;
    omfLength_t	clipLen10, clipLen15;
    omfPosition_t	clipOffset6;
	
    XPROTECT(fileHdl)
      {
	editRate.numerator = 2997;
	editRate.denominator = 100;
	omfsCvtInt32toInt64(70, &trackLength);
	omfsCvtInt32toInt64(10, &clipLen10);
	omfsCvtInt32toInt64(15, &clipLen15);
	omfsCvtInt32toInt64(6, &clipOffset6);
	
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

	CHECK(omfiMobSetDefaultFade(fileHdl, *compMob, clipLen10, kFadeLinearPower, editRate));

	/* Create timecode Clip and append to timecode tracks */
	timecode.startFrame = 0;
	timecode.drop = kTcDrop;
	timecode.fps = 30;
	CHECK(omfiTimecodeNew(fileHdl, trackLength, timecode, &tcClip));
	trackID = 4;
	CHECK(omfiMobAppendNewSlot(fileHdl, *compMob, editRate, tcClip,
				   &tcTrack4));

	/* Create audio source clips and append to audio tracks */
	CHECK(omfiSourceClipNew(fileHdl, soundDef, trackLength, 
				aSource1, &aClip1));
	CHECK(omfiSourceClipSetFade(fileHdl, aClip1, FADEINLEN, kFadeLinearAmp,
				FADEOUTLEN, kFadeLinearAmp));
	CHECK(omfiSequenceNew(fileHdl, soundDef, &aSequence1));
	CHECK(omfiSequenceAppendCpnt(fileHdl, aSequence1, aClip1));

	CHECK(omfiSourceClipNew(fileHdl, soundDef, trackLength, 
				aSource2, &aClip2));
	CHECK(omfiSequenceNew(fileHdl, soundDef, &aSequence2));
	CHECK(omfiSequenceAppendCpnt(fileHdl, aSequence2, aClip2));

	trackID = 2;
	CHECK(omfiMobAppendNewTrack(fileHdl, *compMob, editRate, aSequence1,
				zeroPos, trackID, "ATrack2", &aTrack2));

	trackID = 3;
	CHECK(omfiMobAppendNewSlot(fileHdl, *compMob, editRate, aSequence2, 
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
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen10, &scopeValue));
	CHECK(omfiNestedScopeAppendSlot(fileHdl, vScope, scopeValue));
	rationalValue.numerator = 1;
	rationalValue.denominator = 2;
	CHECK(omfiConstValueNew(fileHdl, rationalDef, clipLen10, 
					     sizeof(rationalValue),
					     (void *)&rationalValue, 
					     &scopeValue));
	CHECK(omfiNestedScopeAppendSlot(fileHdl, vScope, scopeValue));
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen10, vSource, 
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
	CHECK(omfiMobAppendNewTrack(fileHdl, *compMob, editRate, vSequence, 
				    zeroPos, trackID, "VTrack", &vTrack1));

	/* Create timecode Clip and append to timecode tracks */
	edgecode.startFrame = 0;
	edgecode.filmKind = kFt35MM;
	edgecode.codeFormat = kEtKeycode;
	strcpy( (char*) edgecode.header, "DevDesk");
	CHECK(omfiEdgecodeNew(fileHdl, trackLength, edgecode, &ecClip));
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

/* Create a composition mob with 64-bit offsets and lengths */
static omfErr_t CreateHugeCompositionMob(omfHdl_t    fileHdl,
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
    omfSegObj_t aSequence1, aSequence2;
    omfSegObj_t	vFiller1, vFiller2, vScope, scopeValue, vSelector;
    omfSegObj_t	vDissolve1, vDissolve2, vTransition, selValue;
    omfDDefObj_t	pictureDef, soundDef, rationalDef;
    omfTrackID_t	trackID;
    omfUID_t newMobID;
    omfErr_t omfError;
    omfLength_t	clipLen10, clipLen15;
    omfPosition_t	clipOffset6;
	
    XPROTECT(fileHdl)
      {
	editRate.numerator = 2997;
	editRate.denominator = 100;
	omfsMakeInt64(6, 70, &trackLength);
	omfsMakeInt64(1, 10, &clipLen10);
	omfsMakeInt64(2, 15, &clipLen15);
	omfsMakeInt64(3, 6, &clipOffset6);
	
	omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
	omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
	omfiDatakindLookup(fileHdl, RATIONALKIND, &rationalDef, &omfError);

	/* Create the empty composition mob */	
	strncpy((char *)strBuf, "sampMob", 8);	
	CHECK(omfiCompMobNew(fileHdl, (omfString) strBuf, PRIMARY, compMob));

	CHECK(omfiMobSetPrimary(fileHdl, *compMob, PRIMARY));
	CHECK(omfiMobIDNew(fileHdl, &newMobID));
	CHECK(omfiMobSetIdentity(fileHdl, *compMob, newMobID));
	CHECK(omfiMobSetName(fileHdl, *compMob, "Huge Offset Mob"));

	/* Create timecode Clip and append to timecode tracks */
	timecode.startFrame = 0;
	timecode.drop = kTcDrop;
	timecode.fps = 30;
	CHECK(omfiTimecodeNew(fileHdl, trackLength, timecode, &tcClip));
	trackID = 4;
	CHECK(omfiMobAppendNewSlot(fileHdl, *compMob, editRate, tcClip,
				   &tcTrack4));

	/* Create audio source clips and append to audio tracks */
	CHECK(omfiSourceClipNew(fileHdl, soundDef, trackLength, 
				aSource1, &aClip1));
	CHECK(omfiSourceClipSetFade(fileHdl, aClip1, FADEINLEN, kFadeLinearAmp,
				FADEOUTLEN, kFadeLinearAmp));
	CHECK(omfiSequenceNew(fileHdl, soundDef, &aSequence1));
	CHECK(omfiSequenceAppendCpnt(fileHdl, aSequence1, aClip1));

	CHECK(omfiSourceClipNew(fileHdl, soundDef, trackLength, 
				aSource2, &aClip2));
	CHECK(omfiSequenceNew(fileHdl, soundDef, &aSequence2));
	CHECK(omfiSequenceAppendCpnt(fileHdl, aSequence2, aClip2));

	trackID = 2;
	CHECK(omfiMobAppendNewTrack(fileHdl, *compMob, editRate, aSequence1,
				zeroPos, trackID, "ATrack2", &aTrack2));

	trackID = 3;
	CHECK(omfiMobAppendNewSlot(fileHdl, *compMob, editRate, aSequence2, 
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
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen10, &scopeValue));
	CHECK(omfiNestedScopeAppendSlot(fileHdl, vScope, scopeValue));
	rationalValue.numerator = 1;
	rationalValue.denominator = 2;
	CHECK(omfiConstValueNew(fileHdl, rationalDef, clipLen10, 
					     sizeof(rationalValue),
					     (void *)&rationalValue, 
					     &scopeValue));
	CHECK(omfiNestedScopeAppendSlot(fileHdl, vScope, scopeValue));
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen10, vSource, 
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
	CHECK(omfiMobAppendNewTrack(fileHdl, *compMob, editRate, vSequence, 
				    zeroPos, trackID, "VTrack", &vTrack1));

	/* Create timecode Clip and append to timecode tracks */
	edgecode.startFrame = 0;
	edgecode.filmKind = kFt35MM;
	edgecode.codeFormat = kEtKeycode;
	strcpy( (char*) edgecode.header, "DevDesk");
	CHECK(omfiEdgecodeNew(fileHdl, trackLength, edgecode, &ecClip));
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


static omfErr_t CreateVideoMedia(omfHdl_t fileHdl, 
				    char *name, 
				    omfObject_t *returnMaster, 
				    omfObject_t *returnFile)
{
  omfRational_t	videoRate;
  omfRational_t	imageAspectRatio;	
  omfErr_t		omfError = OM_ERR_NONE;
  omfObject_t		masterMob = NULL, fileMob = NULL;
  omfMediaHdl_t	mediaPtr = NULL;
  omfDDefObj_t 	pictureDef = NULL;
  omfObject_t mediaGroup;
  omfObject_t newClip;
  omfObject_t newTrack;
  omfSourceRef_t ref;
  omfLength_t clipLen;
  omfPosition_t	otherPos;
  omfInt16	n;
  
  *returnFile = NULL;
  XPROTECT(fileHdl)
    {
      /* set some constants */
      MakeRational(2997, 100, &videoRate);
      MakeRational(4, 3, &imageAspectRatio);
      omfsCvtInt32toLength(nFrames, clipLen);
      omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef,
			 &omfError); 
      if (omfError != OM_ERR_NONE) RAISE(omfError);

      printf("Creating Master Mob\n");
      CHECK(omfmMasterMobNew(fileHdl, /* file (IN) */
			     name, /* name (IN) */
			     TRUE, /* isPrimary (IN) */
			     &masterMob)); /* result (OUT) */
		
      printf("Creating File Mob\n");
      CHECK(omfmFileMobNew(fileHdl, /* file (IN) */
			   name, /* name (IN) */
			   videoRate,	/* editrate (IN) */
			   CODEC_TIFF_VIDEO, /* codec (IN) */
			   &fileMob)); /* result (OUT) */
      
     
      CHECK(omfiMediaGroupNew(fileHdl, pictureDef, clipLen, &mediaGroup));
      CHECK(omfiMobGetMobID(fileHdl, fileMob, &ref.sourceID));
      ref.sourceTrackID = MEDIATRACKID;
      omfsCvtInt32toPosition(5, otherPos);
      ref.startTime = otherPos;
      CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen, ref, &newClip));
      CHECK(omfiMediaGroupAddChoice(fileHdl, mediaGroup, newClip));
      CHECK(omfiMobAppendNewTrack(fileHdl, masterMob, editRate, mediaGroup, 
				  zeroPos, MEDIATRACKID, NULL, &newTrack));

      /* Create the digital data object, link to file and master mob */
      printf("Creating video media; linking data object, file mob, master mob\n");
      CHECK(omfmVideoMediaCreate(fileHdl,	/* in this file (IN) */
				 NULL,          /* on this master mob (IN) */
				 1,	/* and this master track (IN) */
				 fileMob,	/* and this file mob (IN),
						   create video */
				 kToolkitCompressionEnable, /* compressing it*/
				 videoRate, /* editRate (IN) */
				 STD_HEIGHT, /* height (IN) */
				 STD_WIDTH, /* width (IN) */
				 kFullFrame, /* frame layout (IN) */
				 imageAspectRatio, /* aspect ration (IN) */
				 &mediaPtr));	/* media object (OUT) */

      
      /* Write all the frames one at at time */
      printf("Writing %d frames of video media data\n", nFrames);
      for(n = 0; n < nFrames; n++)
      {
      	CHECK(omfmWriteDataSamples(mediaPtr, /* for this media ref (IN) */
				 1,	/* write n samples (IN) */
				 stdWriteVBuffer, /* from a buffer (IN) */
				 FRAME_BYTES)); /* buf size (IN) */
	  }

      printf("closing media\n");
      CHECK(omfmMediaClose(mediaPtr));			
      
      *returnFile = fileMob;
      *returnMaster = masterMob;
    }
  XEXCEPT
  XEND
      
  return(OM_ERR_NONE);	   
}

static void     InitGlobals(void)
{
  short           frame;
  int             n;
  int		  r, g, b;
  char			*bp;

  omfsCvtInt32toPosition(0, zeroPos);
  omfsCvtInt32toLength(0, zeroLen);
  editRate.numerator = 2997;
  editRate.denominator = 100;

  if (stdWriteVBuffer == NULL)
    stdWriteVBuffer = (char *) omfsMalloc(FRAME_BYTES * 1);
  if (stdReadVBuffer == NULL)
    stdReadVBuffer = (char *) omfsMalloc(FRAME_BYTES);
  if ((stdWriteVBuffer == NULL) || (stdReadVBuffer == NULL))
    exit(1);

  bp = stdWriteVBuffer;
  for ( frame = 0; frame < 1; ++frame )
    {
      /* Flood the image to a frame-unique value */
      r = ( frame & 0x1 ) ? 0:255;
/*    g = ( frame & 0x2 ) ? 0:255;
      b = ( frame & 0x4 ) ? 0:255;
*/
      g = ( frame & 0x2 ) ? 0:0;
      b = ( frame & 0x4 ) ? 0:0;
      
      for (n = 0; n < FRAME_SAMPLES; n++)
	{
	  *bp++ = r;
	  *bp++ = g;
	  *bp++ = b;
	}
    }
  for (n = 0; n < FRAME_BYTES; n++)
    stdReadVBuffer[n] = 0;
}

static omfErr_t CreateFakeAudioMedia(omfHdl_t fileHdl, 
				    char *name, 
				    omfObject_t *returnMaster, 
				    omfObject_t *returnFile)
{
  omfRational_t	audioRate;
  omfErr_t	omfError = OM_ERR_NONE;
  omfObject_t	masterMob = NULL, fileMob = NULL, mdes = NULL;
  omfMediaHdl_t	mediaPtr = NULL;
  omfDDefObj_t 	soundDef = NULL;
  char			namebuf[NAMESIZE], *dataBuf;
  omfInt32		fillNum;
  
  *returnFile = NULL;
  XPROTECT(fileHdl)
    {
      /* set some constants */
      MakeRational(44100, 1, &audioRate);
      omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef,
			 &omfError); 
      if (omfError != OM_ERR_NONE) RAISE(omfError);

      printf("Creating Master Mob\n");
      CHECK(omfmMasterMobNew(fileHdl, /* file (IN) */
			     name, /* name (IN) */
			     TRUE, /* isPrimary (IN) */
			     &masterMob)); /* result (OUT) */
		
      printf("Creating File Mob\n");
      CHECK(omfmFileMobNew(fileHdl, /* file (IN) */
			   name, /* name (IN) */
			   audioRate,	/* editrate (IN) */
			   CODEC_AIFC_AUDIO, /* codec (IN) */
			   &fileMob)); /* result (OUT) */

    
      /* Create the digital data object, link to file and master mob */
      printf("Creating audio media (20000 samples @ 16 bit)...\n");
      CHECK(omfmAudioMediaCreate(fileHdl,	/* in this file (IN) */
				 masterMob,     /* on this master mob (IN) */
				AUDIO1TRACKID,		/* On this master track */
				fileMob,	/* and this file mob (IN),
						   create video */
				audioRate, /* editRate (IN) */
				kToolkitCompressionEnable, /* compress (IN) */
				16, /* sample size */
				1, /* num channels */
				&mediaPtr));	/* media object (OUT) */

	dataBuf = omfsMalloc(40000);
	if(dataBuf == NULL)
		RAISE(OM_ERR_NOMEMORY);
		
	for(fillNum = 0; fillNum < 40000; fillNum++)
	{
		dataBuf[fillNum] = fillNum & 0xff;
	}
	CHECK(omfmWriteDataSamples(mediaPtr, 20000, dataBuf, 40000));
	omfsFree(dataBuf);
	 
      printf("closing media\n");
      CHECK(omfmMediaClose(mediaPtr));			

      sprintf(namebuf, "fake.omf");
      CHECK(omfmMobAddUnixLocator(fileHdl, fileMob, kOmfiMedia, namebuf));
      
      *returnFile = fileMob;
      *returnMaster = masterMob;
    }
  XEXCEPT
  XEND
      
  return(OM_ERR_NONE);	   
}

static omfErr_t PatchTrackLength(omfHdl_t fileHdl,
			  omfMobObj_t mob,
			  omfInt32 length32)
{
  omfLength_t   length;
  omfIterHdl_t  trackIter = NULL;
  omfObject_t   track = NULL, trackSeg = NULL;
  omfFileRev_t  fileRev;

  XPROTECT(fileHdl)
    {
      CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
      CHECK(omfiMobGetNextTrack(trackIter, mob, NULL, &track));
      CHECK(omfiIteratorDispose(fileHdl, trackIter));
      trackIter = NULL;
      CHECK(omfiTrackGetInfo(fileHdl, mob, track, NULL,
			     0, NULL, NULL, NULL, &trackSeg));

      omfsCvtInt32toLength(length32, length);

      /* Assume that track component is a SCLP */
      CHECK(omfsFileGetRev(fileHdl, &fileRev));
      if ((fileRev == kOmfRev1x) || fileRev == kOmfRevIMA)
	{
	  CHECK(omfsWriteLength(fileHdl, trackSeg, OMCLIPLength, length));
	}
      else /* kOmfRev2x */
	{
	  CHECK(omfsWriteLength(fileHdl, trackSeg, OMCPNTLength, length));
	}
    }

  XEXCEPT
    {
      if (trackIter)
	omfiIteratorDispose(fileHdl, trackIter);
    }
  XEND;

  return(OM_ERR_NONE);
}
