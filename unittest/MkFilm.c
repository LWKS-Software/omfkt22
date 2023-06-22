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
 * MkFilm - This unittest creates a composition in either a 1.x or
 *            2.x file.  It only creates the objects that can be
 *            expressed in either revision.  It also creates master and 
 *            file mobs for video and audio to point to.  It also creates a tape mob,
 *			  and a film mob for both audio and video data.
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
#include <stdlib.h>
#include <math.h>

#include "omPublic.h"
#include "omMedia.h"
#include "omEffect.h"

#include "UnitTest.h"

#if PORT_SYS_MAC
#include <console.h>
#endif

#define STD_WIDTH	320L
#define STD_HEIGHT	240L
#define FRAME_SAMPLES	(STD_WIDTH * STD_HEIGHT)
#define FRAME_BYTES	(FRAME_SAMPLES * 3)
#define MEDIATRACKID    1

static char           *stdWriteVBuffer = NULL;

static omfErr_t CreateCompositionMob(omfHdl_t    fileHdl,
 		  omfMobObj_t masterMob,
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

static void InitGlobals(void);

#define M_PI 3.1415

/* Please excuse the lack of error handling code! */
#ifdef MAKE_TEST_HARNESS
int MkFilm(char *filename, char *version)
{
	int argc;
	char *argv[3];
#else
int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t	session;
  omfHdl_t	fileHdl;
  omfMobObj_t compMob;
  omfMobObj_t masterMob;
  omfDDefObj_t pictureDef = NULL, soundDef = NULL;
  omfRational_t tapeEditRate, filmEditRate;
  omfObject_t tapeMob, fileMobV, fileMobA1, fileMobA2, filmMob;
  omfPosition_t	tcOffset, filmOffset; 
  omfLength_t         zeroLen, filmLen, videoLen;
  omfInt32				filmLen32;
  
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
	ProductInfo.productName = "Make Film UnitTest";
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
#else
#error Must have some form of console
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

    InitGlobals();
    CHECK(omfmInit(session));
    CHECK(omfsCreateFile((fileHandleType) argv[1], session, 
			      rev, &fileHdl));

    /* Create a bunch of empty master mobs with the above mob ids, just for
     * completeness.
     */
    omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
    omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
    tapeEditRate.numerator = 30;
    tapeEditRate.denominator = 1;
    filmEditRate.numerator = 24;
    filmEditRate.denominator = 1;

    /* Create a timecode track on a tapemob */
    printf("Making a TAPE MOB...\n");
    /* Create standard tape mob with timecode track */
    CHECK(omfmStandardTapeMobNew(fileHdl, tapeEditRate, /* video */ 
				 tapeEditRate, /* audio */
				 "WriteComp Tapemob", 
				 1, /* IN numVideoTracks */
				 2, /* IN numAudioTracks */
				 &tapeMob)); /* OUT TapeMob Returned object */


    printf("Making a FILM MOB...\n");
	CHECK(omfmFilmMobNew(fileHdl, "Film Mob", &filmMob));
    omfsCvtInt32toPosition(101, filmOffset);
    omfsCvtInt32toPosition(40000, filmLen);
    omfsCvtInt32toPosition(50000, videoLen);
	omfsTruncInt64toInt32(filmLen, &filmLen32);
	CHECK(omfmMobAddEdgecodeClip(fileHdl, filmMob, filmEditRate, 1,
								200, filmLen32, FT_35MM, ET_KEYCODE, "MySource"));
	CHECK(omfmMobAddNilReference(fileHdl, filmMob,
								 2,  /* Video track in film mob */
								 filmLen,
				 				 pictureDef,
				 				 filmEditRate));

	CHECK(omfmMobAddPulldownRef(fileHdl, tapeMob, tapeEditRate,
				  2, /* From file mob's video trk */
				  pictureDef, 
				  filmMob,	
				  filmOffset, 
				  2,  /* Video track in film mob */
				  filmLen,
				  kOMFTwoThreePD,
				  0,
				  kOMFTapeToFilmSpeed));

    /* Create Video Media */
    CHECK(CreateVideoAudioMedia(fileHdl, "Film Master Mob", &masterMob,
			       &fileMobV, &fileMobA1, &fileMobA2));

    /* Link up tape mob to newly created video file mob */
    omfsCvtInt32toPosition(153, tcOffset);
	CHECK(omfmMobAddPulldownRef(fileHdl, fileMobV, filmEditRate,
				  1, /* From file mob's video trk */
				  pictureDef, 
				  tapeMob,	
				  tcOffset, 
				  2,  /* Video track in tape mob */
				  videoLen,
				  kOMFTwoThreePD,
				  0,
				  kOMFFilmToTapeSpeed));

    /* Link up tape mob to newly created audio file mob */
    omfsCvtInt32toPosition(153, tcOffset);
	CHECK(omfmMobAddPulldownRef(fileHdl, fileMobA1, filmEditRate,
				  1, /* From file mob's trk */
				  soundDef, 
				  tapeMob,	
				  tcOffset, 
				  3,  /* Audio1 track in tape mob */
				  videoLen,
				  kOMFTwoThreePD,
				  0,
				  kOMFFilmToTapeSpeed));
				  
    /* Link up tape mob to newly created audio file mob */
    omfsCvtInt32toPosition(153, tcOffset);
	CHECK(omfmMobAddPulldownRef(fileHdl, fileMobA2, filmEditRate,
				  1, /* From file mob's trk */
				  soundDef, 
				  tapeMob,	
				  tcOffset, 
				  4,  /* Audio2 track in tape mob */
				  videoLen,
				  kOMFTwoThreePD,
				  0,
				  kOMFFilmToTapeSpeed));

    CHECK(CreateCompositionMob(fileHdl, masterMob, &compMob));

    CHECK(omfsCloseFile(fileHdl));
    CHECK(omfsEndSession(session));

    if (stdWriteVBuffer)
      omfsFree(stdWriteVBuffer);
    stdWriteVBuffer = NULL;

    printf("MkCplx sucessfully created a %s.x OMF file\n", argv[2]);
  } /* XPROTECT */

  XEXCEPT
    {
      	if (fileHdl)
		{
		  omfsCloseFile(fileHdl);
		  omfsEndSession(session);
		}
		
	    if (stdWriteVBuffer)
	      omfsFree(stdWriteVBuffer);
	    stdWriteVBuffer = NULL;

    	printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
   		return(-1);
    }
  XEND;
  return(0);
}

#define LARGE_SUBLEN	20
#define SMALL_SUBLEN	10
#define	TRACK_LEN		(LARGE_SUBLEN+SMALL_SUBLEN)

static omfErr_t CreateCompositionMob(omfHdl_t    fileHdl,
 		 			 omfMobObj_t masterMob,
				     omfMobObj_t    *compMob)
{
	omfSourceRef_t vSource;
	omfSourceRef_t aSource1;
	omfSourceRef_t aSource2;
    omfRational_t	editRate;
    omfTimecode_t	timecode;
    omfMSlotObj_t	vTrack1, aTrack2, aTrack3, tcTrack4, ecTrack5;
    omfSegObj_t	vSequence;
    omfSegObj_t	aClip1, aClip2, vClip1, vClip2, tcClip, ecClip;
    omfSegObj_t aSequence1, aSequence2;
    omfSegObj_t	vDissolve1 = NULL, vDissolve2 = NULL;
    omfDDefObj_t	pictureDef, soundDef;
    omfTrackID_t	trackID;
    omfErr_t omfError;
    omfPosition_t	zeroPos;
    omfPosition_t	clipOffset5;
    omfLength_t	clipLen10;
    omfLength_t	clipLen20;
    omfLength_t	clipLen30;
   	omfEdgecode_t	edgecode;
	char            edgecodeName[8];
	omfFileRev_t	rev;

    XPROTECT(fileHdl)
      {
		editRate.numerator = 24;
		editRate.denominator = 1;
		omfsCvtInt32toPosition(0, zeroPos);
		omfsCvtInt32toPosition(5, clipOffset5);
		omfsCvtInt32toLength(SMALL_SUBLEN, clipLen10);
		omfsCvtInt32toLength(LARGE_SUBLEN, clipLen20);
		omfsCvtInt32toLength(TRACK_LEN, clipLen30);
		
		omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
		omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
     	omfsFileGetRev(fileHdl,  &rev);

	    /* ... Get source references from somewhere for Source Clips ... */
	    CHECK(omfiMobGetMobID(fileHdl, masterMob, &vSource.sourceID));
	    vSource.sourceTrackID = 1;
	    CHECK(omfiMobGetMobID(fileHdl, masterMob, &aSource1.sourceID));
	    aSource1.sourceTrackID = 2;
	    CHECK(omfiMobGetMobID(fileHdl, masterMob, &aSource2.sourceID));
	    aSource2.sourceTrackID = 3;

		printf("Making a COMPOSITION MOB...\n");
		CHECK(omfiCompMobNew(fileHdl, "Film Composition", PRIMARY, compMob));

		/* Create audio source clips and append to audio tracks */
	    aSource1.startTime = clipOffset5;
		CHECK(omfiSourceClipNew(fileHdl, soundDef, clipLen10, aSource1, &aClip1)); 
	    aSource1.startTime = zeroPos;
		CHECK(omfiSourceClipNew(fileHdl, soundDef, clipLen20, aSource1, &aClip2));
		CHECK(omfiSequenceNew(fileHdl, soundDef, &aSequence1));
		CHECK(omfiSequenceAppendCpnt(fileHdl, aSequence1, aClip1));
		CHECK(omfiSequenceAppendCpnt(fileHdl, aSequence1, aClip2));

	    aSource2.startTime = clipOffset5;
		CHECK(omfiSourceClipNew(fileHdl, soundDef, clipLen10, aSource2, &aClip1)); 
	    aSource2.startTime = zeroPos;
		CHECK(omfiSourceClipNew(fileHdl, soundDef, clipLen20, aSource2, &aClip2));
		CHECK(omfiSequenceNew(fileHdl, soundDef, &aSequence2));
		CHECK(omfiSequenceAppendCpnt(fileHdl, aSequence2, aClip1));
		CHECK(omfiSequenceAppendCpnt(fileHdl, aSequence2, aClip2));

		/* Create video track w/sequence: 
	         *           SCLP, SCLP
	         */
	    vSource.startTime = clipOffset5;
		CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen10, 
					vSource, &vClip1));
	    vSource.startTime = zeroPos;
		CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen20, 
					vSource, &vClip2));

		/* Build sequence with created segments */
		CHECK(omfiSequenceNew(fileHdl, pictureDef, &vSequence));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vClip1));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vClip2));

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
		trackID = 4;
		CHECK(omfiTimecodeNew(fileHdl, clipLen30, timecode, &tcClip));
		CHECK(omfiMobAppendNewTrack(fileHdl, *compMob, editRate, tcClip,
					    zeroPos, trackID, "TCTrack4",
					    &tcTrack4));

		/* Create timecode Clip and append to timecode tracks */
		edgecode.startFrame = 0;
		edgecode.filmKind = kFt35MM;
		edgecode.codeFormat = kEtKeycode;
		strcpy( (char*) edgecode.header, "DevDesk");
		CHECK(omfiEdgecodeNew(fileHdl, clipLen30, edgecode, &ecClip));
		if((rev == kOmfRev1x) || (rev == kOmfRevIMA))
		{
			strncpy((char *) edgecodeName, "Dest", 8);
			CHECK(omfsWriteVarLenBytes(fileHdl, ecClip, OMECCPHeader,
									   zeroPos, 8, edgecodeName));
		}
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
  omfObject_t	masterMob = NULL, vidFileMob = NULL;
  omfObject_t   audFileMob1 = NULL, audFileMob2 = NULL;
  omfMediaHdl_t	mediaPtr = NULL;
  omfDDefObj_t 	pictureDef = NULL;
  omfLength_t 	clipLen;
  omfRational_t	audioRate;
  omfDDefObj_t 	soundDef = NULL;
  short			*dataBuf;
  omfInt32		nFrames = 20, numAudioBytes, numAudioSamples, fillNum, n, cnt;
  omfFileRev_t	rev;
  omfLength_t	numSamples64;
  omfErr_t		omfError = OM_ERR_NONE;
  
  *returnFileV = NULL;
  *returnFileA1 = NULL;
  *returnFileA2 = NULL;
  XPROTECT(fileHdl)
    {
      /* set some constants */
      MakeRational(24, 1, &videoRate);
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

      /* Write all the frames one at at time */
      printf("Writing %d frames of video media data\n", nFrames);
      for(n = 0; n < nFrames; n++)
      {
      	printf("%ld..", n+1);
		#if !defined(_WIN32)
		fflush(stdout);
		#endif
      	if(nFrames-n >= 3)
      		cnt = 3;
      	else
      		cnt = nFrames-n;
      	CHECK(omfmWriteDataSamples(mediaPtr, /* for this media ref (IN) */
				 cnt,	/* write n samples (IN) */
				 stdWriteVBuffer, /* from a buffer (IN) */
				 FRAME_BYTES * 3)); /* buf size (IN) */
	  }
	  printf("\n");

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
		
	for(fillNum = 0; fillNum < numAudioBytes/2; fillNum++)
	{
		dataBuf[fillNum] = (short)(10000.0 * sin(2.0 * M_PI * (float)fillNum / 100.0));
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
		
	for(fillNum = 0; fillNum < numAudioBytes/2; fillNum++)
	{
		dataBuf[fillNum] = (short)(10000.0 * sin(2.0 * M_PI * (float)fillNum / 200.0));
	}
	CHECK(omfmWriteDataSamples(mediaPtr, numAudioSamples, dataBuf, numAudioBytes));
	omfsFree(dataBuf);

      printf("closing media\n");
      CHECK(omfmMediaClose(mediaPtr));			


      *returnFileV = vidFileMob;
      *returnFileA1 = audFileMob1;
      *returnFileA2 = audFileMob2;
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
	omfPosition_t	 zeroPos;
	omfLength_t		 zeroLen;

  omfsCvtInt32toPosition(0, zeroPos);
  omfsCvtInt32toLength(0, zeroLen);

  if (stdWriteVBuffer == NULL)
    stdWriteVBuffer = (char *) omfsMalloc(FRAME_BYTES * 3);
  if (stdWriteVBuffer == NULL)
    exit(1);

  bp = stdWriteVBuffer;
  for ( frame = 0; frame < 3; ++frame )
    {
      /* Flood the image to a frame-unique value */
      r = 0;
      g = 0;
      b = 0;
      if((frame % 3) == 0)
      	r = 255;
      if((frame % 3) == 1)
      	g = 255;
      if((frame % 3) == 2)
      	b = 255;
      
      for (n = 0; n < FRAME_SAMPLES; n++)
		{
		  *bp++ = r;
		  *bp++ = g;
		  *bp++ = b;
		}
    }
}
