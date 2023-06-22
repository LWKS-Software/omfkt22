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
 * MkCplx - This unittest creates a composition in either a 1.x or
 *            2.x file.  It only creates the objects that can be
 *            expressed in either revision.  It also creates master and 
 *            file mobs for video and audio to point to.  The video has real 
 *            data, and the audio doesn't.  It also creates a tape mob, to 
 *            derive the video data from (but not the audio data).
 ********************************************************************/

#include "masterhd.h"
#define TRUE 1
#define FALSE 0
#define PRIMARY 1
#define NULL 0
#define FADEINLEN 2
#define FADEOUTLEN 2
#define NAMESIZE 255

#include <stdio.h>
#include <string.h>

#include "omPublic.h"
#include "omMedia.h"
#include "omEffect.h"

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

#define STD_WIDTH	320L
#define STD_HEIGHT	240L
#define MEDIATRACKID    1

static omfErr_t CreateCompositionMob(omfHdl_t    fileHdl,
		  omfSourceRef_t vSource,
		  omfSourceRef_t aSource1,
		  omfSourceRef_t aSource2,
		  omfMobObj_t    *compMob);

static omfErr_t CreateVideoAudioMedia(omfHdl_t fileHdl, 
				    char *name, 
				    omfObject_t *returnMaster, 
				    omfObject_t *returnFileV,
				    omfObject_t *returnFileA1,
				    omfObject_t *returnFileA2);

static omfErr_t PatchTrackLength(omfHdl_t fileHdl,
			  omfMobObj_t mob,
			  omfTrackID_t trackID,
			  omfInt32 length32);

/* Please excuse the lack of error handling code! */
#ifdef MAKE_TEST_HARNESS
int MkCplx(char *filename, char *version)
{
	int argc;
	char *argv[3];
#else
int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t	session;
  omfHdl_t	fileHdl;
  omfSourceRef_t vSource, aSource1, aSource2;
  omfMobObj_t compMob;
#if 0
  omfMobObt_t masterMobV, masterMobA1, masterMobA2;
#endif
  omfMobObj_t masterMob;
  omfDDefObj_t pictureDef = NULL, soundDef = NULL;
  omfRational_t editRate;
  omfObject_t tapeMob, fileMobV, fileMobA1, fileMobA2;
  omfPosition_t	tcOffset; 
  omfLength_t         zeroLen;
  
  omfFileRev_t rev;
  omfErr_t omfError;
  omfPosition_t	zeroPos;
  omfPosition_t	clipPos5;
  omfPosition_t	clipPos8;
  omfPosition_t	clipPos13;
  omfPosition_t	clipPos17;
  omfLength_t	clipLen10;
  omfLength_t	clipLen42;
  omfLength_t	clipLen50;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Make Complex UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

  XPROTECT(NULL)
  {
#ifdef MAKE_TEST_HARNESS
	argc=3;
	argv[1] = filename;
	argv[2] = version;
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

    if (argc < 3)
      {
	printf("*** ERROR - WriteComp <filename> <1|2|IMA>\n");
	return(1);
      }

    omfsCvtInt32toPosition(0, zeroPos);
    omfsCvtInt32toPosition(5, clipPos5);
    omfsCvtInt32toPosition(8, clipPos8);
    omfsCvtInt32toPosition(13, clipPos13);
    omfsCvtInt32toPosition(17, clipPos17);
    omfsCvtInt32toLength(10, clipLen10);
    omfsCvtInt32toLength(42, clipLen42);
    omfsCvtInt32toLength(50, clipLen50);
    omfsCvtInt32toLength(0, zeroLen);

    CHECK(omfsBeginSession(&ProductInfo, &session));
    if (!strcmp(argv[2], "1"))
      rev = kOmfRev1x;
    else if (!strcmp(argv[2], "2"))
      rev = kOmfRev2x;
    else if (!strcmp(argv[2], "IMA"))
      rev = kOmfRevIMA;
    else	
      {
	printf("*** ERROR - Illegal file rev\n");
	return(1);
      }

    CHECK(omfmInit(session));
    CHECK(omfsCreateFile((fileHandleType) argv[1], session, 
			      rev, &fileHdl));

    /* Create a bunch of empty master mobs with the above mob ids, just for
     * completeness.
     */
    omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
    omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
    editRate.numerator = 2997;
    editRate.denominator = 100;

    /* Create a timecode track on a tapemob */
    printf("Making a TAPE MOB...\n");
    /* Create standard tape mob with timecode track */
    CHECK(omfmStandardTapeMobNew(fileHdl, editRate, /* video */ 
				 editRate, /* audio */
				 "WriteComp Tapemob", 
				 1, /* IN numVideoTracks */
				 2, /* IN numAudioTracks */
				 &tapeMob)); /* OUT TapeMob Returned object */

    /* Create Video Media */
    CHECK(CreateVideoAudioMedia(fileHdl, "Synchronized AV", &masterMob,
			       &fileMobV, &fileMobA1, &fileMobA2));

    /* Link up tape mob to newly created video file mob */
    omfsCvtInt32toPosition(153, tcOffset);
    CHECK(omfmMobAddPhysSourceRef(fileHdl, fileMobV, editRate,
				  1, /* From file mob's video trk */
				  pictureDef, 
				  tapeMob,	
				  tcOffset, 
				  2,  /* Video track in tape mob */
				  zeroLen)); /* SCLP already exists */
    omfsCvtInt32toPosition(153, tcOffset);
    CHECK(omfmMobAddPhysSourceRef(fileHdl, fileMobA1, editRate,
				  1, /* From file mob's video trk */
				  soundDef, 
				  tapeMob,	
				  tcOffset, 
				  3,  /* Video track in tape mob */
				  zeroLen)); /* SCLP already exists */
    omfsCvtInt32toPosition(153, tcOffset);
    CHECK(omfmMobAddPhysSourceRef(fileHdl, fileMobA2, editRate,
				  1, /* From file mob's video trk */
				  soundDef, 
				  tapeMob,	
				  tcOffset, 
				  4,  /* Video track in tape mob */
				  zeroLen)); /* SCLP already exists */

    /* ... Get source references from somewhere for Source Clips ... */
    CHECK(omfiMobGetMobID(fileHdl, masterMob, &vSource.sourceID));
    vSource.sourceTrackID = 1;
    vSource.startTime = clipPos5;
    CHECK(omfiMobGetMobID(fileHdl, masterMob, &aSource1.sourceID));
    aSource1.sourceTrackID = 2;
    aSource1.startTime = clipPos8;
    CHECK(omfiMobGetMobID(fileHdl, masterMob, &aSource2.sourceID));
    aSource2.sourceTrackID = 3;
    aSource2.startTime = clipPos13;

    CHECK(CreateCompositionMob(fileHdl, vSource, aSource1, aSource2,  
			       &compMob));

/*   if(rev == kOmfRev2x)
    {
    	CHECK(CreateHugeCompositionMob(fileHdl, vSource, aSource1, aSource2,  
			       &compMob));
    }
*/
			       
    CHECK(omfsCloseFile(fileHdl));
    CHECK(omfsEndSession(session));

    printf("MkCplx sucessfully created a %s.x OMF file\n", argv[2]);
  } /* XPROTECT */

  XEXCEPT
    {
      if (fileHdl)
	{
	  omfsCloseFile(fileHdl);
	  omfsEndSession(session);
	}
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
    omfFileRev_t fileRev;
    omfString	strBuf[64];
    omfRational_t	editRate, dissolveLevel;
    omfLength_t	trackLength;
    omfTimecode_t	timecode;
    omfEdgecode_t	edgecode;
    omfMSlotObj_t	vTrack1, aTrack2, aTrack3, tcTrack4, ecTrack5;
    omfSegObj_t	vSequence;
    omfSegObj_t	tcClip, aClip1, aClip2, vClip1, vClip2, ecClip;
    omfSegObj_t aSequence1, aSequence2;
    omfSegObj_t	vFiller1, vFiller2, vSelector;
    omfSegObj_t	vDissolve1 = NULL, vDissolve2 = NULL, vTransition, selValue;
    omfDDefObj_t	pictureDef, soundDef;
    omfTrackID_t	trackID;
    omfUID_t newMobID;
    omfErr_t omfError;
    omfPosition_t	zeroPos;
    omfPosition_t	clipOffset6;
    omfLength_t	clipLen10;
    omfLength_t	clipLen20;
    omfLength_t	clipLen60;

    XPROTECT(fileHdl)
      {
	editRate.numerator = 2997;
	editRate.denominator = 100;
	omfsCvtInt32toPosition(0, zeroPos);
	omfsCvtInt32toPosition(0, clipOffset6);
	omfsCvtInt32toLength(10, clipLen10);
	omfsCvtInt32toLength(20, clipLen20);
	omfsCvtInt32toLength(60, clipLen60);
	trackLength = clipLen60;
	
	omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
	omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);

	/* Create the empty composition mob */	
	strncpy((char *)strBuf, "sampMob", 8);	

	/* Get file revision */
	CHECK(omfsFileGetRev(fileHdl, &fileRev));

	printf("Making a COMPOSITION MOB...\n");
	CHECK(omfiCompMobNew(fileHdl, (omfString) strBuf, PRIMARY, compMob));

	CHECK(omfiMobIDNew(fileHdl, &newMobID));
	CHECK(omfiMobSetIdentity(fileHdl, *compMob, newMobID));

	CHECK(omfiMobSetName(fileHdl, *compMob, "Lisa Mob"));

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

	/* Create video track w/sequence: 
         *           FILL, SCLP, TRAN, SCLP, EFFI, NEST, SLCT, FILL 
         */
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen10, &vFiller1));
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen20, 
				vSource, &vClip1));
	CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen20, 
				vSource, &vClip2));
	CHECK(omfiSourceClipSetRef(fileHdl, vClip2, vSource));

	/* Create transition with dissolve effect, TrackA and TrackB implied */
	dissolveLevel.numerator = 2;
	dissolveLevel.denominator = 3;

	/* If 2x file, create the effect for the transition */
	if (fileRev == kOmfRev2x)
	  {
	    CHECK(omfeVideoDissolveNew(fileHdl, clipLen10, NULL, NULL, NULL,
				       &vDissolve1));
	  }
	omfError = omfiTransitionNew(fileHdl, pictureDef, clipLen10, clipOffset6, 
				     vDissolve1, &vTransition);

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
	if (vTransition)
	  {
	    CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vTransition));
	  }
	CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vClip2));
	CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vSelector));
	CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vFiller2));

	trackID = 1;
	CHECK(omfiMobAppendNewTrack(fileHdl, *compMob, editRate, vSequence, 
				    zeroPos, trackID, "VTrack1",
				    &vTrack1));
	trackID = 2;
	CHECK(omfiMobAppendNewTrack(fileHdl, *compMob, editRate, aSequence1,
				zeroPos, trackID, "ATrack2", &aTrack2));
	trackID = 3;
	CHECK(omfiMobAppendNewTrack(fileHdl, *compMob, editRate, aSequence2, 
				    zeroPos, trackID, "ATrack3",
				    &aTrack3));

	/* Create timecode Clip and append to timecode tracks */
	timecode.startFrame = 0;
	timecode.drop = kTcDrop;
	timecode.fps = 30;
	CHECK(omfiTimecodeNew(fileHdl, clipLen60, timecode, &tcClip));
	trackID = 4;
	CHECK(omfiMobAppendNewTrack(fileHdl, *compMob, editRate, tcClip,
				    zeroPos, trackID, "TCTrack4",
				    &tcTrack4));


	/* Create timecode Clip and append to timecode tracks */
	edgecode.startFrame = 0;
	edgecode.filmKind = kFt35MM;
	edgecode.codeFormat = kEtKeycode;
	strcpy( (char*) edgecode.header, "DevDesk");
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

static omfErr_t CreateVideoAudioMedia(omfHdl_t fileHdl, 
				    char *name, 
				    omfObject_t *returnMaster, 
				    omfObject_t *returnFileV,
				    omfObject_t *returnFileA1,
				    omfObject_t *returnFileA2)
{
  omfRational_t	videoRate;
  omfRational_t	imageAspectRatio;	
  omfObject_t		masterMob = NULL, vidFileMob = NULL;
  omfObject_t   	audFileMob1 = NULL, audFileMob2 = NULL;
  omfMediaHdl_t	mediaPtr = NULL;
  omfDDefObj_t 	pictureDef = NULL;
  omfLength_t 	clipLen;
  omfRational_t	audioRate;
  omfDDefObj_t 	soundDef = NULL;
  char			namebuf[64];
  char			*dataBuf;
  omfInt32		nFrames = 20, numAudioBytes, numAudioSamples, fillNum;
  omfLength_t		numSamples64;
  omfFileRev_t	rev;
  omfErr_t		omfError = OM_ERR_NONE;
  
  *returnFileV = NULL;
  *returnFileA1 = NULL;
  *returnFileA2 = NULL;
  XPROTECT(fileHdl)
    {
      /* set some constants */
      MakeRational(2997, 100, &videoRate);
      MakeRational(4, 3, &imageAspectRatio);
      omfsCvtInt32toLength(nFrames, clipLen);
      omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef,
			 &omfError); 
      if (omfError != OM_ERR_NONE) RAISE(omfError);
      MakeRational(44100, 1, &audioRate);
      omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef,
			 &omfError); 
      if (omfError != OM_ERR_NONE) RAISE(omfError);
      printf("Creating Master Mob\n");
      CHECK(omfmMasterMobNew(fileHdl, /* file (IN) */
			     name, /* name (IN) */
			     TRUE, /* isPrimary (IN) */
			     &masterMob)); /* result (OUT) */

      /* CREATE VIDEO MEDIA */
      printf("Creating File Mob\n");
      CHECK(omfmFileMobNew(fileHdl, /* file (IN) */
			   name, /* name (IN) */
			   videoRate,	/* editrate (IN) */
			   CODEC_TIFF_VIDEO, /* codec (IN) */
			   &vidFileMob)); /* result (OUT) */
      

      /* Create the digital data object, link to file and master mob */
      printf("Creating video media; linking data object, file mob, master mob\n");
      CHECK(omfmVideoMediaCreate(fileHdl,	/* in this file (IN) */
			 masterMob,          /* on this master mob (IN) */
			 1,		     /* and this master track (IN) */
			 vidFileMob,    /* and this file mob (IN),
					create video */
			 kToolkitCompressionEnable, /* compressing it (IN) */
			 videoRate, /* editRate (IN) */
			 STD_HEIGHT, /* height (IN) */
			 STD_WIDTH, /* width (IN) */
			 kFullFrame, /* frame layout (IN) */
			 imageAspectRatio, /* aspect ration (IN) */
			 &mediaPtr));	/* media object (OUT) */

#if 0
      
      /* Write all the frames one at at time */
      printf("Writing %d frames of video media data\n", nFrames);
      for(n = 0; n < nFrames; n++)
      {
      	CHECK(omfmWriteDataSamples(mediaPtr, /* for this media ref (IN) */
				 1,	/* write n samples (IN) */
				 stdWriteVBuffer, /* from a buffer (IN) */
				 FRAME_BYTES)); /* buf size (IN) */
	  }
#endif

      printf("closing media\n");
      CHECK(omfmMediaClose(mediaPtr));			

	CHECK(omfiConvertEditRate(videoRate, clipLen, audioRate, kRoundFloor, &numSamples64));
	CHECK(omfsTruncInt64toInt32(numSamples64, &numAudioSamples));	/* OK MAXREAD */
	numAudioBytes = numAudioSamples * 2;
      
      printf("Creating Audio File Mob 1\n");
      CHECK(omfmFileMobNew(fileHdl, /* file (IN) */
			   name, /* name (IN) */
			   videoRate,
			   CODEC_AIFC_AUDIO, /* codec (IN) */
			   &audFileMob1)); /* result (OUT) */

      omfsFileGetRev(fileHdl,  &rev);
      if (rev == kOmfRev1x)
	{
	  CHECK(omfsWriteExactEditRate(fileHdl, audFileMob1, 
				       OMCPNTEditRate, videoRate));
	}
   
      /* Create the digital data object, link to file and master mob */
      printf("Creating audio media...\n");
      CHECK(omfmAudioMediaCreate(fileHdl,	/* in this file (IN) */
				 masterMob,     /* on this master mob (IN) */
				2,		/* On this master track */
				audFileMob1,	/* and this file mob (IN),
						   create video */
				audioRate, /* editRate (IN) */
				kToolkitCompressionEnable, /* compress (IN) */
				16, /* sample size */
				1, /* num channels */
				&mediaPtr));	/* media object (OUT) */

	dataBuf = omfsMalloc(numAudioBytes);
	if(dataBuf == NULL)
		RAISE(OM_ERR_NOMEMORY);
		
	for(fillNum = 0; fillNum < numAudioBytes; fillNum++)
	{
		dataBuf[fillNum] = fillNum & 0xff;
	}
	CHECK(omfmWriteDataSamples(mediaPtr, numAudioSamples, dataBuf, numAudioBytes));
	omfsFree(dataBuf);

      printf("closing media\n");
      CHECK(omfmMediaClose(mediaPtr));			

      printf("Creating Audio File Mob 2\n");
      CHECK(omfmFileMobNew(fileHdl, /* file (IN) */
			   name, /* name (IN) */
			   videoRate,
			   CODEC_AIFC_AUDIO, /* codec (IN) */
			   &audFileMob2)); /* result (OUT) */

      omfsFileGetRev(fileHdl,  &rev);
      if (rev == kOmfRev1x)
	{
	  CHECK(omfsWriteExactEditRate(fileHdl, audFileMob2, 
				       OMCPNTEditRate, videoRate));
	}
   
      /* Create the digital data object, link to file and master mob */
      printf("Creating audio media...\n");
      CHECK(omfmAudioMediaCreate(fileHdl,	/* in this file (IN) */
				 masterMob,     /* on this master mob (IN) */
				3,		/* On this master track */
				audFileMob2,	/* and this file mob (IN),
						   create video */
				audioRate, /* editRate (IN) */
				kToolkitCompressionEnable, /* compress (IN) */
				16, /* sample size */
				1, /* num channels */
				&mediaPtr));	/* media object (OUT) */

	dataBuf = omfsMalloc(numAudioBytes);
	if(dataBuf == NULL)
		RAISE(OM_ERR_NOMEMORY);
		
	for(fillNum = 0; fillNum < numAudioBytes; fillNum++)
	{
		dataBuf[fillNum] = fillNum & 0xff;
	}
	CHECK(omfmWriteDataSamples(mediaPtr, numAudioSamples, dataBuf, numAudioBytes));
	omfsFree(dataBuf);

      printf("closing media\n");
      CHECK(omfmMediaClose(mediaPtr));			


      /* Patch length of file/master mob tracks, to pretend that that there 
       * is media for this example.
       */
      CHECK(PatchTrackLength(fileHdl, masterMob, 1, nFrames));
      CHECK(PatchTrackLength(fileHdl, masterMob, 2, nFrames));
      CHECK(PatchTrackLength(fileHdl, masterMob, 3, nFrames));
      CHECK(PatchTrackLength(fileHdl, vidFileMob, 1, nFrames));
      CHECK(PatchTrackLength(fileHdl, audFileMob1, 1, nFrames));
      CHECK(PatchTrackLength(fileHdl, audFileMob2, 1, nFrames));

      sprintf(namebuf, "fake.omf");
      CHECK(omfmMobAddUnixLocator(fileHdl, audFileMob1, kOmfiMedia, namebuf));
      sprintf(namebuf, "fake.omf");
      CHECK(omfmMobAddUnixLocator(fileHdl, audFileMob2, kOmfiMedia, namebuf));

      *returnFileV = vidFileMob;
      *returnFileA1 = audFileMob1;
      *returnFileA2 = audFileMob2;
      *returnMaster = masterMob;
    }
  XEXCEPT
  XEND
      
  return(OM_ERR_NONE);	   
}

static omfErr_t PatchTrackLength(omfHdl_t fileHdl,
			  omfMobObj_t mob,
			  omfTrackID_t trackID,
			  omfInt32 length32)
{
  omfLength_t   length;
  omfUInt32	tmp1xLength;
  omfIterHdl_t  trackIter = NULL;
  omfObject_t   track = NULL, trackSeg = NULL;
  omfInt32 numTracks, loop;
  omfFileRev_t  fileRev;

  XPROTECT(fileHdl)
    {
      omfsCvtInt32toLength(length32, length);
      CHECK(omfsFileGetRev(fileHdl, &fileRev));
	if ((fileRev == kOmfRev1x) || fileRev == kOmfRevIMA)
      	{
      		CHECK(omfsTruncInt64toUInt32(length, &tmp1xLength));		/* OK 1.x */
      	}
      CHECK(omfiMobGetNumTracks(fileHdl, mob, &numTracks));

      CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
      for (loop = 1; loop <= numTracks; loop++)
	{
	  CHECK(omfiMobGetNextTrack(trackIter, mob, NULL, &track));
	  CHECK(omfiTrackGetInfo(fileHdl, mob, track, NULL,
				 0, NULL, NULL, NULL, &trackSeg));
	  /* Assume that track component is a SCLP */
	  if ((fileRev == kOmfRev1x) || fileRev == kOmfRevIMA)
	    {
	      CHECK(omfsWriteInt32(fileHdl, trackSeg, OMCLIPLength, 
				   tmp1xLength));
	    }
	  else /* kOmfRev2x */
	    {
	      CHECK(omfsWriteLength(fileHdl, trackSeg, OMCPNTLength, length));
	    }
	}
      CHECK(omfiIteratorDispose(fileHdl, trackIter));
      trackIter = NULL;
    }

  XEXCEPT
    {
      if (trackIter)
	omfiIteratorDispose(fileHdl, trackIter);
    }
  XEND;

  return(OM_ERR_NONE);
}
