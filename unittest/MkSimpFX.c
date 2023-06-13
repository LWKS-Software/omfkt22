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

/******************************************************************/
/* WriteSimple.c - Uses the OMF Simple Composition Interface to   */
/*                 write a simple (flat) composition with a       */
/*                 single video track and a timecode track.       */
/*                 The video track contains the Roger and Paul    */
/*                 clips from the OMF Movie example in the OMF    */
/*                 2.0 Training class.                            */
/*                 To keep the example simple, the file mobs, etc.*/
/*                 are not included in the file.                  */
/******************************************************************/
/* INCLUDES */
#include "masterhd.h"
#include <stdlib.h>
#include <string.h>

#include "omPublic.h"
#include "omMedia.h"
#include "omCompos.h"

#include "UnitTest.h"

/* MACROS */
#define initUID(id, pref, maj, min) \
{ (id).prefix = pref; (id).major = maj; (id).minor = min; }
#define copyUIDs(a, b) \
 {a.prefix = b.prefix; a.major = b.major; a.minor = b.minor;}

/* STATIC FUNCTIONS */
static omfErr_t CreateMasterMob(
				omfHdl_t fileHdl,
				omfMobObj_t *masterMob,
				omfUniqueNamePtr_t datakindName);
static omfErr_t VerifyEffects(
			      omfSessionHdl_t session,
			      char *filename,
			      omfFileRev_t rev);

static omfErr_t EffectGetName(omfHdl_t fileHdl,
		       omfObject_t effect,
		       omfInt32 nameSize,
		       omfUniqueNamePtr_t effectName);

/* Global Variables */
static omfRational_t editRate;
static omfSourceRef_t nullSource;

/* MAIN PROGRAM */
#ifdef MAKE_TEST_HARNESS
int MkSimpFX(char *filename,char *version)
{
    int argc;
    char *argv[3];
#else
int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t	session;
  omfHdl_t	fileHdl = NULL;
  omfMobObj_t compMob, videoMasterMob, imageMasterMob, audioMasterMob;
  omfObject_t newClip;
  omfDDefObj_t pictureDef, soundDef, stereoSoundDef;
  omfTimecode_t timecode;
  omfUID_t nullID = {0,0,0};
  omfTrackHdl_t pictureTrackHdl = NULL, tcTrackHdl = NULL;
  omfTrackHdl_t soundTrackHdl = NULL;
  omfSourceRef_t videoSource, imageSource, audioSource;
  omfFileRev_t rev;
  omfRational_t speedRatio, ratio, multiplier;
  omfUInt32 mask;
  omfLength_t pictTrackLen, soundTrackLen, trackLen, clipLen, fillLen;
  omfErr_t omfError = OM_ERR_NONE;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Make Simple Effects UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

  XPROTECT(NULL)
  {
#ifdef MAKE_TEST_HARNESS
    argc = 3;
    argv[1] = filename;
    argv[2] = version;
#endif
    /* This function will write either a 1.x or 2.x file */
    if (argc != 3)
      {
	printf("Usage: MkSimpFX <filename>.omf <1|2|IMA>\n");
	return(-1);
      }
    if (!strcmp(argv[2], "1"))
      rev = kOmfRev1x;
    else if (!strcmp(argv[2], "2"))
      rev = kOmfRev2x;
    else if (!strcmp(argv[2],"IMA"))
      rev = kOmfRevIMA;
    else
      {
	printf("*** ERROR - Illegal file revision\n");
	return(-1);
      }

    CHECK(omfsBeginSession(&ProductInfo, &session));
    printf("Creating file...\n");
    CHECK(omfsCreateFile((fileHandleType) argv[1], session, rev, &fileHdl));

    /* Initialize the editrate to NTSC */
    editRate.numerator = 2997;
    editRate.denominator = 100;

    /* Initialize a null source reference */
    nullSource.sourceTrackID = 0;
    omfsCvtInt32toPosition(0, nullSource.startTime);
    initUID(nullSource.sourceID, 0, 0, 0);

    /* Create datakind objects */
    omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
    omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
    omfiDatakindLookup(fileHdl, SOUNDKIND, &stereoSoundDef, &omfError);

    /* Create a master mob that the composition mob source clips will
     * point to for their source media.
     */
    CHECK(CreateMasterMob(fileHdl, &videoMasterMob, PICTUREKIND));
    CHECK(CreateMasterMob(fileHdl, &imageMasterMob, PICTUREKIND));
    CHECK(CreateMasterMob(fileHdl, &audioMasterMob, SOUNDKIND));

    /* Get the newly created master mobs' mobIDs to reference in the 
     * composition.
     */
    CHECK(omfiMobGetMobID(fileHdl, videoMasterMob, &videoSource.sourceID));
    videoSource.sourceTrackID = 1;

    CHECK(omfiMobGetMobID(fileHdl, imageMasterMob, &imageSource.sourceID));
    imageSource.sourceTrackID = 1;

    CHECK(omfiMobGetMobID(fileHdl, audioMasterMob, &audioSource.sourceID));
    audioSource.sourceTrackID = 1;

    /* Create a new Composition Mob */
    printf("Creating Composition Mob with effects\n");
    CHECK(omfiCompMobNew(fileHdl, 
			 "Simple Composition", /* Mob Name */
			 TRUE,                 /* Primary mob */
			 &compMob));

    /**********************************************************/
    /* Create picture new track */
    /* Length is not specified until components are added */
    CHECK(omfcSimpleTrackNew(fileHdl, compMob, 
			     1,           /* TrackID */
			     "Picture Track 1",   /* TrackName */
			     editRate,    /* Track editrate */
			     PICTUREKIND, /* Datakind: omfi:data:Picture */
			     &pictureTrackHdl));

    /*** Start appending components to the new track ***/
    /* All segments will be of the datakind specified during track creation */

    /* Append source clips that point to the created master mob */
    omfsCvtInt32toPosition(100, videoSource.startTime);
    omfsCvtInt32toLength(500, clipLen);
    CHECK(omfcSimpleAppendSourceClip(pictureTrackHdl, clipLen, videoSource));

    omfsCvtInt32toPosition(200, videoSource.startTime);
    omfsCvtInt32toLength(100, clipLen);
    CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen, videoSource,
			    &newClip));
    speedRatio.numerator = 2;
    speedRatio.denominator = 1;
    CHECK(omfcSimpleAppendVideoSpeedControl(pictureTrackHdl, clipLen,
					    newClip, speedRatio));

    omfsCvtInt32toLength(15, clipLen);
    CHECK(omfcSimpleAppendVideoWipe(pictureTrackHdl, clipLen, 15));


    omfsCvtInt32toPosition(700, videoSource.startTime);
    omfsCvtInt32toLength(300, clipLen);
    CHECK(omfcSimpleAppendSourceClip(pictureTrackHdl, clipLen, videoSource));

    /* Use "transition defaults" for the arguments to the effect */
    omfsCvtInt32toLength(15, clipLen);
    CHECK(omfcSimpleAppendVideoDissolve(pictureTrackHdl, 
					clipLen));    /* length */

    omfsCvtInt32toPosition(200, videoSource.startTime);
    omfsCvtInt32toLength(100, clipLen);
    CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen, videoSource,
			    &newClip));
    CHECK(omfcSimpleAppendVideoRepeat(pictureTrackHdl, clipLen, newClip));

    /* The following effects only apply to 2.x:
     *    FadeToBlack
     *    FadeFromBlack
     */
    if (rev == kOmfRev2x)
      {
	omfsCvtInt32toPosition(200, videoSource.startTime);
	omfsCvtInt32toLength(100, clipLen);
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen, videoSource,
				&newClip));
	CHECK(omfcSimpleAppendVideoFadeToBlack(pictureTrackHdl, clipLen, 
					       newClip));

	omfsCvtInt32toPosition(200, videoSource.startTime);
	omfsCvtInt32toLength(100, clipLen);
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen, videoSource,
				&newClip));
	CHECK(omfcSimpleAppendVideoFadeFromBlack(pictureTrackHdl, clipLen, 
						 newClip));

      }

    omfsCvtInt32toPosition(200, videoSource.startTime);
    omfsCvtInt32toLength(100, clipLen);
    CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen, videoSource,
			    &newClip));
    mask = 0;
    CHECK(omfcSimpleAppendVideoMask(pictureTrackHdl, clipLen, newClip,
				    mask, kOmfFxFrameMaskDirAdd));

    omfsCvtInt32toPosition(0, imageSource.startTime);
    omfsCvtInt32toLength(15, clipLen);
    CHECK(omfcSimpleAppendSourceClip(pictureTrackHdl, clipLen, imageSource));

    /**********************************************************/
    /* Create sound new track */
    /* Length is not specified until components are added */
    CHECK(omfcSimpleTrackNew(fileHdl, compMob, 
			     2,           /* TrackID */
			     "Audio Track 2",   /* TrackName */
			     editRate,    /* Track editrate */
			     SOUNDKIND, /* Datakind: omfi:data:Picture */
			     &soundTrackHdl));

    /*** Start appending components to the new track ***/
    /* All segments will be of the datakind specified during track creation */

    /* NOTE: add MonoAudioMix */

    /* Append source clips that point to the created master mob */
    omfsCvtInt32toPosition(100, audioSource.startTime);
    omfsCvtInt32toLength(100, clipLen);
    CHECK(omfcSimpleAppendSourceClip(soundTrackHdl, clipLen, audioSource));

    omfsCvtInt32toLength(25, clipLen);
    CHECK(omfcSimpleAppendMonoAudioDissolve(soundTrackHdl, clipLen));

    omfsCvtInt32toPosition(700, audioSource.startTime);
    omfsCvtInt32toLength(100, clipLen);
    CHECK(omfcSimpleAppendSourceClip(soundTrackHdl, clipLen, audioSource));

    /* The following effects only apply to 2.x:
     *    MonoAudioPan
     *    MonoAudioGain
     *    StereoAudioGain
     */
    if (rev == kOmfRev2x)
      {
	omfsCvtInt32toPosition(200, audioSource.startTime);
	omfsCvtInt32toLength(100, clipLen);
	CHECK(omfiSourceClipNew(fileHdl, soundDef, clipLen, audioSource,
				&newClip));
	ratio.numerator = 1;
	ratio.denominator = 3;
	CHECK(omfcSimpleAppendMonoAudioPan(soundTrackHdl, clipLen, newClip,
					   ratio));
	omfsCvtInt32toPosition(200, audioSource.startTime);
	omfsCvtInt32toLength(100, clipLen);
	CHECK(omfiSourceClipNew(fileHdl, soundDef, clipLen, audioSource,
				&newClip));
	multiplier.numerator = 1;
	multiplier.denominator = 3;
	CHECK(omfcSimpleAppendMonoAudioGain(soundTrackHdl, clipLen, newClip, 
					    multiplier));

	omfsCvtInt32toPosition(200, audioSource.startTime);
	omfsCvtInt32toLength(100, clipLen);
	CHECK(omfiSourceClipNew(fileHdl, stereoSoundDef, clipLen, audioSource,
				&newClip));
	multiplier.numerator = 1;
	multiplier.denominator = 3;
	CHECK(omfcSimpleAppendStereoAudioGain(soundTrackHdl, clipLen, newClip, 
					      multiplier));


      }

    omfsCvtInt32toPosition(700, audioSource.startTime);
    omfsCvtInt32toLength(100, clipLen);
    CHECK(omfcSimpleAppendSourceClip(soundTrackHdl, clipLen, audioSource));

    if (rev == kOmfRev2x)
      {
	omfsCvtInt32toLength(25, clipLen);
	CHECK(omfcSimpleAppendStereoAudioDissolve(soundTrackHdl, clipLen));
      }

    omfsCvtInt32toPosition(700, audioSource.startTime);
    omfsCvtInt32toLength(100, clipLen);
    CHECK(omfcSimpleAppendSourceClip(soundTrackHdl, clipLen, audioSource));

    /**********************************************************/
    /* Create a new timecode track */
    /* Get picture track length for timecode track */	       
    CHECK(omfcSimpleTrackGetLength(pictureTrackHdl, &pictTrackLen));
    CHECK(omfcSimpleTrackGetLength(soundTrackHdl, &soundTrackLen));

    /* Pad out the tracks so that they're the same length, for this
     * unittest example.
     */
    if (omfsInt64Greater(pictTrackLen, soundTrackLen))
      {
	fillLen = pictTrackLen;
	CHECK(omfsSubInt64fromInt64(soundTrackLen, &fillLen));
	CHECK(omfcSimpleAppendFiller(soundTrackHdl, fillLen));
	trackLen = pictTrackLen;
      }
    else if (omfsInt64Greater(soundTrackLen, pictTrackLen))
      {
	fillLen = soundTrackLen;
	CHECK(omfsSubInt64fromInt64(pictTrackLen, &fillLen));
	CHECK(omfcSimpleAppendFiller(pictureTrackHdl, fillLen));
	trackLen = soundTrackLen;
      }
    else /* equal */
      trackLen = pictTrackLen;

    CHECK(omfsStringToTimecode("01:00:00:00", editRate, &timecode));
    CHECK(omfcSimpleTimecodeTrackNew(fileHdl, compMob, 
					3,                /* TrackID */
					"Timecode Track", /* TrackName */
					editRate,         /* Track Editrate */
					trackLen,         /* Track Length */
					timecode,         /* Timecode desc */
					&tcTrackHdl));

    /* Close the 2 tracks */
    CHECK(omfcSimpleTrackClose(pictureTrackHdl));
    pictureTrackHdl = NULL;
    CHECK(omfcSimpleTrackClose(soundTrackHdl));
    soundTrackHdl = NULL;
    CHECK(omfcSimpleTrackClose(tcTrackHdl));
    tcTrackHdl = NULL;

    CHECK(omfsCloseFile(fileHdl));
    fileHdl = NULL;

    printf("Verifying effect creation...\n");
    CHECK(VerifyEffects(session, argv[1], rev));

    omfsEndSession(session);

    printf("MkSimpFX completed successfully.\n");
  } /* XPROTECT */

  XEXCEPT
    {
      if (fileHdl)
	{
	  if (pictureTrackHdl)
	    omfcSimpleTrackClose(pictureTrackHdl);
	  if (soundTrackHdl)
	    omfcSimpleTrackClose(soundTrackHdl);
	  if (tcTrackHdl)
	    omfcSimpleTrackClose(tcTrackHdl);
	  if (fileHdl)
	    omfsCloseFile(fileHdl);
	  omfsEndSession(session);
	}
      printf("*** ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
      return(-1);
    }
  XEND;
  return(0);
}


/* CreateMasterMob - creates a master mob with one track.  The datakind of the
 *                   track is passed in.  The track contains a single SCLP
 *                   with a null reference.  In reality, this would probably
 *                   point to a file mob with digital data associated with
 *                   it.  For this example, we are simply providing a null
 *                   mastermob to give the composition mob something to
 *                   reference for media.
 */
static omfErr_t CreateMasterMob(omfHdl_t fileHdl,
				omfMobObj_t *masterMob,
				omfUniqueNamePtr_t datakindName)
{
  omfTrackHdl_t masterTrackHdl = NULL;
  omfLength_t clipLen;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(fileHdl)
    {
      CHECK(omfmMasterMobNew(fileHdl, 
			     "Null Master Mob", /* Mob Name */
			     FALSE,             /* Not a primary mob */
			     masterMob));

      /* Create picture new track */
      /* Length is not specified until components are added */
      CHECK(omfcSimpleTrackNew(fileHdl, *masterMob, 
			       1,           /* TrackID */
			       "Track 1",   /* TrackName */
			       editRate,    /* Track editrate */
			       datakindName, /* Datakind name */
			       &masterTrackHdl));
      omfsCvtInt32toLength(2000, clipLen);
      CHECK(omfcSimpleAppendSourceClip(masterTrackHdl, clipLen, nullSource));
      CHECK(omfcSimpleTrackClose(masterTrackHdl));
    }

  XEXCEPT
    {
      if (masterTrackHdl)
	omfcSimpleTrackClose(masterTrackHdl);
      return(XCODE());
    }
  XEND;

  return(OM_ERR_NONE);
}

static omfErr_t VerifyEffects(
			      omfSessionHdl_t session,
			      char *filename,
			      omfFileRev_t rev)
{
  omfHdl_t fileHdl;
  omfIterHdl_t mobIter = NULL, trackIter = NULL, sequIter = NULL;
  omfMobObj_t compMob = NULL;
  omfMSlotObj_t track = NULL;
  omfSegObj_t trackSeg = NULL;
  omfSegObj_t sequCpnt = NULL;
  omfSegObj_t inputSegment = NULL, inputSegmentA = NULL, inputSegmentB = NULL;
  omfSegObj_t level = NULL;
  omfSegObj_t ratio = NULL;
  omfSegObj_t multiplier = NULL;
  omfSegObj_t effect = NULL;
  omfLength_t length;
  omfRational_t speedRatio;
  omfUInt32 phaseOffset, mask;
  omfFxFrameMaskDirection_t addOrDrop;
  omfInt32 wipeNumber;
  omfBool reverse = FALSE;
  omfSearchCrit_t searchCrit;
  omfUniqueName_t effectID;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(NULL)
    {
      CHECK(omfsOpenFile((fileHandleType)filename, session, &fileHdl));

      CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
      searchCrit.searchTag = kByMobKind;
      searchCrit.tags.mobKind = kCompMob;
      CHECK(omfiGetNextMob(mobIter, &searchCrit, &compMob));

      CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
      
      /* Read expected segments off of video track */
      printf("Verifying video track effects...\n");
      CHECK(omfiMobGetNextTrack(trackIter, compMob, NULL, &track));
      CHECK(omfiTrackGetInfo(fileHdl, compMob, track, NULL,
			     0, NULL, NULL, NULL, &trackSeg));

      if (!omfiIsASequence(fileHdl, trackSeg, &omfError))
	{
	  printf("*** ERROR - No sequence in this track!\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}

      CHECK(omfiIteratorAlloc(fileHdl, &sequIter));

      CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL, 
				    &sequCpnt));
      if (!omfiIsASourceClip(fileHdl, sequCpnt, &omfError))
	{
	  printf("*** ERROR - Source clip expected\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}

      CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,&sequCpnt));
      CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
      if (!strcmp(effectID, kOmfFxSpeedControlGlobalName))
	{
	  CHECK(omfeVideoSpeedControlGetInfo(fileHdl, sequCpnt, 
					     &length, &inputSegment, 
					     &speedRatio, &phaseOffset));
	}
	else
	{
	  printf("*** ERROR - Video Speed Control expected\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}


      CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,&sequCpnt));
      CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
      if (!strcmp(effectID, kOmfFxVSMPTEWipeGlobalName))
	{
	  if (rev == kOmfRev2x)
	    {
	      CHECK(omfiTransitionGetInfo(fileHdl, sequCpnt, NULL, NULL, 
					  NULL, &effect));
	    }
	  else /* kOmfRev1x or kOmfRevIMA */
	    effect = sequCpnt;

	  /* NOTE: get wipe controls too... */
	  CHECK(omfeSMPTEVideoWipeGetInfo(fileHdl, effect, &length,
				  &inputSegmentA, &inputSegmentB, &level, 
				  &wipeNumber, NULL));
	}
      else
	{
	  printf("*** ERROR - Video Wipe expected\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}

      CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,&sequCpnt));
      if (!omfiIsASourceClip(fileHdl, sequCpnt, &omfError))
	{
	  printf("*** ERROR - Source clip expected\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}

      CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,&sequCpnt));
      CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
      if (!strcmp(effectID, kOmfFxSVDissolveGlobalName))
	{
	  if (rev == kOmfRev2x)
	    {
	      CHECK(omfiTransitionGetInfo(fileHdl, sequCpnt, NULL, NULL, 
					  NULL, &effect));
	    }
	  else /* kOmfRev1x or kOmfRevIMA */
	    effect = sequCpnt;

	  CHECK(omfeVideoDissolveGetInfo(fileHdl, effect, &length,
					 &inputSegmentA, &inputSegmentB,
					 &level));
	}
      else
	{
	  printf("*** ERROR - Video Dissolve expected\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}

      CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,&sequCpnt));
      CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
      if (!strcmp(effectID, kOmfFxRepeatGlobalName))
	{
	  CHECK(omfeVideoRepeatGetInfo(fileHdl, sequCpnt, &length,
				       &inputSegment, &phaseOffset));
	}
      else
	{
	  printf("*** ERROR - Video Repeat expected\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}

      if (rev == kOmfRev2x)
	{
	  CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,
					&sequCpnt));
	  CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
	  if (!strcmp(effectID, kOmfFxVFadeToBlackGlobalName))
	    {
	      CHECK(omfeVideoFadeToBlackGetInfo(fileHdl, sequCpnt, &length,
					&inputSegment, &level, &reverse, 
					&phaseOffset));
	      if (reverse != FALSE)
		{
		  printf("*** ERROR - Video Fade TO Black expected\n");
		  RAISE(OM_ERR_INVALID_OBJ);
		}
	    }
	  else
	    {
	      printf("*** ERROR - Video Fade To Black expected\n");
	      RAISE(OM_ERR_INVALID_OBJ);
	    }

	  CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,
					&sequCpnt));
	  CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
	  if (!strcmp(effectID, kOmfFxVFadeToBlackGlobalName))
	    {
	      CHECK(omfeVideoFadeToBlackGetInfo(fileHdl, sequCpnt, &length,
					&inputSegment, &level, &reverse, 
					&phaseOffset));
	      if (reverse != TRUE)
		{
		  printf("*** ERROR - Video Fade FROM Black expected\n");
		  RAISE(OM_ERR_INVALID_OBJ);
		}
	    }
	  else
	    {
	      printf("*** ERROR - Video FadeFromBlack expected\n");
	      RAISE(OM_ERR_INVALID_OBJ);
	    }
	} /* kOmfRev2x */

      CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,&sequCpnt));
      CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
      if (!strcmp(effectID, kOmfFxFrameMaskGlobalName))
	{
	  CHECK(omfeVideoFrameMaskGetInfo(fileHdl, sequCpnt, &length,
				     &inputSegment, &mask, 
				     &addOrDrop, &phaseOffset));
	}
      else
	{
	  printf("*** ERROR - Video Mask expected\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}

      CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,&sequCpnt));
      if (!omfiIsASourceClip(fileHdl, sequCpnt, &omfError))
	{
	  printf("*** ERROR - Source clip expected\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}


      /* Read expected segments off of audio track */
      printf("Verifying audio track effects...\n");
      CHECK(omfiMobGetNextTrack(trackIter, compMob, NULL, &track));
      CHECK(omfiTrackGetInfo(fileHdl, compMob, track, NULL,
			     0, NULL, NULL, NULL, &trackSeg));

      CHECK(omfiIteratorClear(fileHdl, sequIter));
      CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,&sequCpnt));
      if (!omfiIsASourceClip(fileHdl, sequCpnt, &omfError))
	{
	  printf("*** ERROR - Source clip expected\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}

      CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,&sequCpnt));
      CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
      if (!strcmp(effectID, kOmfFxSMADissolveGlobalName))
	{
	  if (rev == kOmfRev2x)
	    {
	      CHECK(omfiTransitionGetInfo(fileHdl, sequCpnt, NULL, NULL, 
					  NULL, &effect));
	    }
	  else /* kOmfRev1x or kOmfRevIMA */
	    effect = sequCpnt;

	  CHECK(omfeMonoAudioDissolveGetInfo(fileHdl, effect,
					     &length, &inputSegmentA, 
					     &inputSegmentB, &level));
	}
      else
	{
	  printf("*** ERROR - Mono Audio Dissolve expected\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}

      CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,&sequCpnt));
      if (!omfiIsASourceClip(fileHdl, sequCpnt, &omfError))
	{
	  printf("*** ERROR - Source clip expected\n");
	  RAISE(OM_ERR_INVALID_OBJ);
	}

    if (rev == kOmfRev2x)
      {
	CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,
				      &sequCpnt));
	CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
	if (!strcmp(effectID, kOmfFxMAPanGlobalName))
	  {
	    CHECK(omfeMonoAudioPanGetInfo(fileHdl, sequCpnt, &length,
					  &inputSegment, &ratio));
	  }
	else
	  {
	    printf("*** ERROR - Mono Audio Pan expected\n");
	    RAISE(OM_ERR_INVALID_OBJ);
	  }

	CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,
				      &sequCpnt));
	CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
	if (!strcmp(effectID, kOmfFxMAGainGlobalName))
	  {
	    CHECK(omfeMonoAudioGainGetInfo(fileHdl, sequCpnt, &length,
					   &inputSegment, &multiplier));
	  }
	else
	  {
	    printf("*** ERROR - Mono Audio Gain expected\n");
	    RAISE(OM_ERR_INVALID_OBJ);
	  }

	CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,
				      &sequCpnt));
	CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
	if (!strcmp(effectID, kOmfFxSAGainGlobalName))
	  {
	    CHECK(omfeStereoAudioGainGetInfo(fileHdl, sequCpnt, &length,
					     &inputSegment, &multiplier));
	  }
	else
	  {
	    printf("*** ERROR - Stereo Audio Gain expected\n");
	    RAISE(OM_ERR_INVALID_OBJ);
	  }

	CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,
				      &sequCpnt));
	if (!omfiIsASourceClip(fileHdl, sequCpnt, &omfError))
	  {
	    printf("*** ERROR - Source clip expected\n");
	    RAISE(OM_ERR_INVALID_OBJ);
	  }

	CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,
				      &sequCpnt));
	CHECK(EffectGetName(fileHdl, sequCpnt, OMUNIQUENAME_SIZE, effectID));
	if (!strcmp(effectID, kOmfFxSSADissolveGlobalName))
	  {
	  if (rev == kOmfRev2x)
	    {
	      CHECK(omfiTransitionGetInfo(fileHdl, sequCpnt, NULL, NULL, 
					  NULL, &effect));
	    }
	  else /* kOmfRev1x or kOmfRevIMA */
	    effect = sequCpnt;

	  CHECK(omfeStereoAudioDissolveGetInfo(fileHdl, effect,
					       &length, &inputSegmentA,
					       &inputSegmentB, &level));
	  }
	else
	  {
	    printf("*** ERROR - Stereo Audio Dissolve expected\n");
	    RAISE(OM_ERR_INVALID_OBJ);
	  }

	CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL, NULL,
				      &sequCpnt));
	if (!omfiIsASourceClip(fileHdl, sequCpnt, &omfError))
	  {
	    printf("*** ERROR - Source clip expected\n");
	    RAISE(OM_ERR_INVALID_OBJ);
	  }
      } /* kOmfRev2x */

      if (mobIter)
	omfiIteratorDispose(fileHdl, mobIter);
      if (trackIter)
	omfiIteratorDispose(fileHdl, trackIter);
      if (sequIter)
	omfiIteratorDispose(fileHdl, sequIter);
      CHECK(omfsCloseFile(fileHdl));
    } /* XPROTECT */

  XEXCEPT
    {
      if (mobIter)
	omfiIteratorDispose(fileHdl, mobIter);
      if (trackIter)
	omfiIteratorDispose(fileHdl, trackIter);
      if (sequIter)
	omfiIteratorDispose(fileHdl, sequIter);
      omfsCloseFile(fileHdl);
    }
  XEND;

  return(OM_ERR_NONE);
}

static omfErr_t EffectGetName(omfHdl_t fileHdl,
		       omfObject_t component,
		       omfInt32 nameSize,
		       omfUniqueNamePtr_t effectID)
{
  omfEDefObj_t effectDef = NULL;
  omfEffObj_t effect = NULL;
  omfDDefObj_t datakind = NULL;
  omfClassID_t classID;
  omfFileRev_t rev;
  omfUniqueName_t buf;
  omfErr_t omfError = OM_ERR_NONE;
  omfProperty_t idProp;
  
  XPROTECT(NULL)
    {
      CHECK(omfsFileGetRev(fileHdl, &rev));
      if (rev == kOmfRev2x)
	{
	  if (omfiIsATransition(fileHdl, component, &omfError))
	    {
	      CHECK(omfiTransitionGetInfo(fileHdl, component, NULL, NULL, 
					  NULL, &effect));
	    }
	  else if (!omfiIsAnEffect(fileHdl, component, &omfError))
	    {
	      printf("*** ERROR - Not a effect");
	      RAISE(OM_ERR_INVALID_OBJ);
	    }
	  else /* is an effect object */
		  effect = component;

	  CHECK(omfiEffectGetInfo(fileHdl, effect, NULL, NULL, &effectDef));
	  CHECK(omfiEffectDefGetInfo(fileHdl, effectDef, nameSize,
				     effectID, 0, NULL, 0, NULL, NULL, NULL));
	}
      else /* kOmfRev1x or kOmfRevIMA */
	{
	  if (omfiIsATransition(fileHdl, component, &omfError))
	    {
	      CHECK(omfsReadString(fileHdl, component, OMCPNTEffectID, buf, 
				   OMUNIQUENAME_SIZE));
	      if (!strncmp(buf, "Wipe:SMPTE:", 11))
		{
		  strncpy(effectID, kOmfFxVSMPTEWipeGlobalName, 
			  (size_t)nameSize);
		}
	      else if (!strncmp(buf, "Blend:Dissolve", (size_t)14))
		{
		  CHECK(omfiComponentGetInfo(fileHdl, component, &datakind,NULL));
		  if (omfiIsPictureWithMatteKind(fileHdl, datakind, 
						 kConvertTo, &omfError))
		    {
		      strncpy(effectID, kOmfFxSVDissolveGlobalName, 
			      (size_t)nameSize);
		    }
		  else if (omfiIsSoundKind(fileHdl, datakind, kConvertTo,
					   &omfError))
		    {
		      strncpy(effectID, kOmfFxSMADissolveGlobalName,
			      (size_t)nameSize);
		    }
		}
	    }
	  else /* is an effect */
	    {
		if ((rev == kOmfRev1x) ||(rev == kOmfRevIMA))
		{
			idProp = OMObjID;
		}
		else /* kOmfRev2x */
		{
			idProp = OMOOBJObjClass;
		}
		
	      CHECK(omfsReadClassID(fileHdl, component, idProp, classID));
	      if (!strncmp(classID, "MASK", (size_t)4))
		{
		  strncpy(effectID, kOmfFxFrameMaskGlobalName, 
			  (size_t)nameSize);
		}
	      else if (!strncmp(classID, "REPT", (size_t)4))
		{
		  strncpy(effectID, kOmfFxRepeatGlobalName, (size_t)nameSize);
		}
	      else if (!strncmp(classID, "SPED", (size_t)4))
		{
		  strncpy(effectID, kOmfFxSpeedControlGlobalName,
			  (size_t)nameSize);
		}
	      else
		{
		  RAISE(OM_ERR_INVALID_EFFECT);
		}
	    } /* effect */
	}
    }

  XEXCEPT
    {
    }
  XEND;

  return(OM_ERR_NONE);
}
