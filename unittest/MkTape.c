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

/* MkTape - shows 3 different ways to make a video tape mob in the
 *          OMF file.  
 */
#include "masterhd.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if PORT_SYS_MAC
#include <events.h>
#endif

#include "omPublic.h"
#include "omMedia.h"

#include "UnitTest.h"
#include "MkTape.h"

#define STD_WIDTH	64L
#define STD_HEIGHT	48L
#define FRAME_SAMPLES	(STD_WIDTH * STD_HEIGHT)
#define FRAME_BYTES	(FRAME_SAMPLES * 3)

static char  *stdWriteBuffer = NULL;
static char  *stdReadBuffer = NULL;
static short motoOrder = MOTOROLA_ORDER, intelOrder = INTEL_ORDER;

static int   saveAsVersion1TOC = FALSE;
static int	 nFrames = 8;
static omfErr_t CreateTapeMobHigh(omfHdl_t filePtr, 
								  omfObject_t fileMob, 
								  omfLength_t clipLen);
static omfErr_t CreateTapeMobMedium(omfHdl_t filePtr, 
									omfObject_t fileMob, 
									omfLength_t clipLen);
static omfErr_t CreateTapeMobLow(omfHdl_t filePtr, 
								 omfObject_t fileMob, 
								 omfLength_t clipLen);
static omfErr_t CreateStandardMedia(omfHdl_t filePtr, 
									char *name, 
									omfObject_t *returnMaster, 
									omfObject_t *returnFile);
static omfErr_t CreateDiscontiguousTapeMobTypeA(omfHdl_t filePtr, 
									omfObject_t fileMob, 
									omfLength_t clipLen);
static omfErr_t CreateDiscontiguousTapeMobTypeB(omfHdl_t filePtr, 
									omfObject_t fileMob, 
									omfLength_t clipLen);

#define FatalErrorCode(ec,line,file) { \
					  printf("ERROR '%s' returned at line %d in %s\n", \
					  omfsGetErrorString(ec),line,file); \
					  RAISE(OM_ERR_TEST_FAILED); }

#define FatalError(msg) { printf(msg); RAISE(OM_ERR_TEST_FAILED); }

#define check(a)  { omfErr_t e; e = a; \
                if (e != OM_ERR_NONE) FatalErrorCode(e, __LINE__, __FILE__); }

static void InitGlobals(void)
{
	short   n, frame;
	int		r, g, b;
	char	*bp;

	if (stdWriteBuffer == NULL)
		stdWriteBuffer = (char *) omfsMalloc(FRAME_BYTES * nFrames);
	if (stdReadBuffer == NULL)
		stdReadBuffer = (char *) omfsMalloc(FRAME_BYTES);
	if ((stdWriteBuffer == NULL) || (stdReadBuffer == NULL))
		exit(1);

    bp = stdWriteBuffer;
    for ( frame = 0; frame < nFrames; ++frame )
		{
		/* Flood the image to a frame-unique value */
		r = ( frame & 0x1 ) ? 0:255;
		g = ( frame & 0x2 ) ? 0:255;
		b = ( frame & 0x4 ) ? 0:255;

		for (n = 0; n < FRAME_SAMPLES; n++)
		{
			*bp++ = r;
			*bp++ = g;
			*bp++ = b;
		}
	}
	for (n = 0; n < FRAME_BYTES; n++)
		stdReadBuffer[n] = 0;
}

static void FreeGlobals(void)
{
	if (stdWriteBuffer != NULL)
		{
		omfsFree(stdWriteBuffer);
		stdWriteBuffer = NULL;
		}
	if (stdReadBuffer != NULL)
		{
		omfsFree(stdReadBuffer);
		stdReadBuffer = NULL;
		}
	}

void VideoWriteTestFile(char *name, char *version)
{
	omfSessionHdl_t sessionPtr = NULL;
	omfHdl_t        filePtr = NULL;
	omfObject_t		fileMob = NULL;
	omfObject_t		masterMob = NULL;
	omfLength_t		clipLen;
	char			tapeName[256];
	omfFileRev_t    rev;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Tape Mob UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

	InitGlobals();

	if (!strcmp(version,"1"))
	  rev = kOmfRev1x;
	else if (!strcmp(version,"2"))
	  rev = kOmfRev2x;
	else if (!strcmp(version,"IMA"))
	  rev = kOmfRevIMA;
	else 
	  {
		printf("*** ERROR - Illegal file rev\n");
		return;
	  }

	XPROTECT(NULL) 
	  {
		CHECK(omfsBeginSession(&ProductInfo, &sessionPtr));
		CHECK(omfmInit(sessionPtr));
		CHECK(omfsCreateFile((fileHandleType)name, sessionPtr, rev,
						 &filePtr)); 
		
		omfsCvtInt32toLength(nFrames, clipLen);
		CHECK(CreateStandardMedia(filePtr, "Low-level", &masterMob, &fileMob));
		CHECK(CreateTapeMobLow(filePtr, fileMob, clipLen));
		
		CHECK(CreateStandardMedia(filePtr, "Medium-level", &masterMob, 
								  &fileMob));
		CHECK(CreateTapeMobMedium(filePtr, fileMob, clipLen));
 		CHECK(omfmMobGetTapeName(filePtr, fileMob, 1,  sizeof(tapeName), 
								 tapeName));
		CHECK(CreateStandardMedia(filePtr, "High-level", &masterMob, 
								  &fileMob));
		CHECK(CreateTapeMobHigh(filePtr, fileMob, clipLen));
		CHECK(CreateStandardMedia(filePtr, "Discontiguous (Continuous TC Track)", &masterMob, 
								  &fileMob));
		CHECK(CreateDiscontiguousTapeMobTypeA(filePtr, fileMob,  clipLen));
		CHECK(CreateStandardMedia(filePtr, "Discontiguous (Mixed TC Track)", &masterMob, 
								  &fileMob));
		CHECK(CreateDiscontiguousTapeMobTypeB(filePtr, fileMob,  clipLen));

		printf("closing file\n");
		CHECK(omfsCloseFile(filePtr));
		printf("ending session\n");
		CHECK(omfsEndSession(sessionPtr));
		printf("End of WriteTestFile\n");
	  }
	XEXCEPT
	{
#ifdef OMFI_ERROR_TRACE
		omfPrintStackTrace(filePtr);
		omfsCloseFile(filePtr);
		omfsEndSession(sessionPtr);
#endif
	}
	XEND_VOID

	return;	
}

static omfErr_t CreateStandardMedia(omfHdl_t filePtr, 
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

	*returnFile = NULL;
	XPROTECT(filePtr)
	{
		/* set some constants */
		MakeRational(2997, 100, &videoRate);
		MakeRational(4, 3, &imageAspectRatio);
		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureDef,
						   &omfError); 
		if (omfError != OM_ERR_NONE) RAISE(omfError);

		printf("Creating Master Mob\n");
		CHECK(omfmMasterMobNew(filePtr, /* file (IN) */
							   name, /* name (IN) */
							   TRUE, /* isPrimary (IN) */
							   &masterMob)); /* result (OUT) */
		
		printf("Creating File Mob\n");
		CHECK(omfmFileMobNew(filePtr, /* file (IN) */
							 name, /* name (IN) */
							 videoRate,	/* editrate (IN) */
							 CODEC_RGBA_VIDEO, /* codec (IN) */
							 &fileMob)); /* result (OUT) */
	
		/* Create the digital data object, link to file and master mob */
		printf("Creating video media; linking data object, file mob, master mob\n");
		CHECK(omfmVideoMediaCreate(filePtr,	/* in this file (IN) */
								   NULL, /* on this master mob (IN) */
								   1, 		/* and this master track (IN) */
								   fileMob,	/* and this file mob (IN),
											   create video */
								   kToolkitCompressionEnable, 
								   videoRate, /* editRate (IN) */
								   STD_HEIGHT, /* height (IN) */
								   STD_WIDTH, /* width (IN) */
								   kFullFrame, /* frame layout (IN) */
								   imageAspectRatio, /* aspect ration (IN) */
								   &mediaPtr));	/* media object (OUT) */

									  /* Write all the frames at once */
		printf("Writing %d frames of video media data\n", nFrames);
		CHECK(omfmWriteDataSamples(mediaPtr, /* for this media ref (IN) */
								   nFrames,	/* write n samples (IN) */
								   stdWriteBuffer, /* from a buffer (IN) */
								   FRAME_BYTES * nFrames));	/* buf size (IN) */
		
		printf("closing media\n");
		CHECK(omfmMediaClose(mediaPtr));			

		CHECK(omfmMobAddMasterTrack(filePtr, /* file (IN) */
									masterMob, /* master mob (IN) */
									pictureDef, /* media kind (IN) */
									1, /* master mob track id */
									1, /* file mob track id */
									"Picture Track #1", /* track name */
									fileMob)); /* file mob (IN) */

		*returnFile = fileMob;
		*returnMaster = masterMob;
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);											
}

static omfErr_t CreateTapeMobHigh(omfHdl_t filePtr, 
								  omfObject_t fileMob, 
								  omfLength_t clipLen)
{
	omfObject_t	tapeMob = NULL;
	omfDDefObj_t pictureDef = NULL;
	omfErr_t		omfError = OM_ERR_NONE;
	omfRational_t	videoRate;
	omfTimecode_t	mediaStartTC;
	omfTimecode_t	tapeStartTC;
	omfPosition_t	tcOffset;
	
	/* Create the tape mob with 24 hrs TC track id 0
	 * starting at 00:00:00:00, picture track id 1 
	 */ 
	XPROTECT(filePtr)
	{
		MakeRational(2997, 100, &videoRate);
		CHECK(omfsStringToTimecode("01:00:00:00", videoRate,
								   &mediaStartTC)); 
		omfsCvtInt32toPosition(mediaStartTC.startFrame, tcOffset);
		CHECK(omfsStringToTimecode("00:00:00:00", videoRate,
								   &tapeStartTC)); 
		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureDef,
						   &omfError); 
		if (omfError != OM_ERR_NONE) RAISE(omfError);

		printf("Creating Tape Mob: 24 hr TC track id 0, picture track id 1\n");
		CHECK(omfmStandardTapeMobNew(filePtr, /* IN file */
								  videoRate, /* IN videoEditRate */
								  videoRate, /* IN audioEditRate */
								  "Created with high-level calls", /* IN name*/
								  1, /* IN numVideoTracks */
								  0, /* IN numAudioTracks */
								  &tapeMob)); /* OUT TapeMob Returned object */
		
		/* Link file mob to tape mob, showing source starting at
		 * timecode 01:00:00:00 (non-drop) 
		 */
		printf("Linking file mob to tape mob starting at TC 01:00:00:00\n");
		CHECK(omfmMobAddPhysSourceRef(filePtr, /* file (IN) */
									  fileMob, /* file mob (IN) */
									  videoRate, /* editRate (IN) */
									  1, /* trackLabel of file mob (IN) */
									  pictureDef, /* mediaKind (IN) */
									  tapeMob,	/* sourceRef (IN) */
									  tcOffset, 
									  /* srcRefOffset in tape mob (IN) */
									  2, /* srcRefTrack in tape mob (IN) */
									  clipLen)); /* srcRefLength (IN) */
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);											
}

static omfErr_t CreateTapeMobMedium(omfHdl_t filePtr, 
									omfObject_t fileMob, 
									omfLength_t clipLen)
{	
	omfObject_t		tapeMob = NULL;
	omfDDefObj_t 	pictureDef = NULL;
	omfErr_t		omfError = OM_ERR_NONE;
	omfRational_t	videoRate;
	omfTimecode_t	mediaStartTC;
	omfTimecode_t	tapeStartTC;
	omfPosition_t	tcOffset, zeroOffset;

	/* Create the tape mob with 24 hrs TC track id 0
	 * starting at 00:00:00:00, picture track id 1 
	 */ 
	XPROTECT(filePtr)
	{
		omfsCvtInt32toPosition(0, zeroOffset);
		MakeRational(2997, 100, &videoRate);
		CHECK(omfsStringToTimecode("01:00:00:00", videoRate,
								   &mediaStartTC)); 
		omfsCvtInt32toPosition(mediaStartTC.startFrame, tcOffset);
		CHECK(omfsStringToTimecode("00:00:00:00", videoRate,
								   &tapeStartTC)); 
		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureDef,
						   &omfError); 
		if (omfError != OM_ERR_NONE) RAISE(omfError);

		printf("Creating Tape Mob\n");
		CHECK(omfmTapeMobNew(filePtr,
							 "Created with medium-level calls",
							 &tapeMob));
	
		printf("Add to tape mob 24 hr TC track id 1, picture track id 2\n");

		CHECK(omfmMobAddTimecodeClip(filePtr,
									 tapeMob,
									 videoRate,
									 1,	/* track label */
									 tapeStartTC, /* start TC */
									 FULL_LENGTH));	/* length */

		/* Make empty source clip on picture track of tape mob, link
		   file mob picture track to this track */
	
		/* Make picture track with empty source clip in tape mob */
		printf("Creating picture track id 1 on tape mob, empty source clip\n");
		CHECK(omfmMobValidateTimecodeRange(filePtr,	/* IN -- */
				                     tapeMob,	/* IN -- */
					                 pictureDef,	/* IN -- */
					                 2,	/* Video Track ID IN -- */
				            	     videoRate,	/* IN -- */
					                 mediaStartTC.startFrame,	/* IN -- */
					                 nFrames));	/* IN -- */
	
		/* Link file mob picture track to tape mob picture track */
		printf("Linking file mob picture track 1 to tape mob picture track 1\n");
		CHECK(omfmMobAddPhysSourceRef(filePtr,
									  fileMob,
				                      			videoRate,	/* IN -- */
									  1, /* file mob track id */
									  pictureDef, /* mediaKind */
									  tapeMob, /* source ref */
									  tcOffset, /* srcRefOffset */
									  2,	/* srcRefTrack ID (tape mob) */
									  clipLen)); /* srcRefLength */
		
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);											
}

static omfErr_t CreateDiscontiguousTapeMobTypeA(omfHdl_t filePtr, 
									omfObject_t fileMob, 
									omfLength_t clipLen)
{	
	omfObject_t		tapeMob = NULL;
	omfDDefObj_t 	pictureDef = NULL;
	omfErr_t		omfError = OM_ERR_NONE;
	omfRational_t	videoRate;
	omfTimecode_t	mediaStartTC1, mediaStartTC2;
	omfTimecode_t	tapeStartTC;
	omfPosition_t		tcOffset1, tcOffset2, zeroOffset;

	/* Create the tape mob with 24 hrs TC track id 0
	 * starting at 00:00:00:00, picture track id 1 
	 */ 
	XPROTECT(filePtr)
	{
		omfsCvtInt32toPosition(0, zeroOffset);
		MakeRational(2997, 100, &videoRate);
		CHECK(omfsStringToTimecode("01:00:00:00", videoRate,
								   &mediaStartTC1)); 
		omfsCvtInt32toPosition(mediaStartTC1.startFrame, tcOffset1);
		CHECK(omfsStringToTimecode("02:00:00:00", videoRate,
								   &mediaStartTC2)); 
		omfsCvtInt32toPosition(mediaStartTC2.startFrame, tcOffset2);
		CHECK(omfsStringToTimecode("00:00:00:00", videoRate,
								   &tapeStartTC)); 
		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureDef,
						   &omfError); 
		if (omfError != OM_ERR_NONE) RAISE(omfError);

		printf("Creating Tape Mob\n");
		CHECK(omfmTapeMobNew(filePtr,
							 "Created discontiguous (Type A) with medium-level calls",
							 &tapeMob));
	
		printf("Add to tape mob 24 hr TC track id 1, picture track id 2\n");

		CHECK(omfmMobAddTimecodeClip(filePtr,
									 tapeMob,
									 videoRate,
									 1,	/* track label */
									 tapeStartTC, /* start TC */
									 FULL_LENGTH));	/* length */

		/* Make empty source clip on picture track of tape mob, link
		   file mob picture track to this track */
	
		/* Make picture track with empty source clip in tape mob */
		printf("Creating picture track id 1 on tape mob, empty source clip\n");
		CHECK(omfmMobValidateTimecodeRange(filePtr,	/* IN -- */
				                     tapeMob,	/* IN -- */
					                 pictureDef,	/* IN -- */
					                 2,	/* Video Track ID IN -- */
				            	     videoRate,	/* IN -- */
					                 mediaStartTC1.startFrame,	/* IN -- */
					                 nFrames));	/* IN -- */
		printf("Creating discontiguous section on picture track id 1 on tape mob, empty source clip\n");
		CHECK(omfmMobValidateTimecodeRange(filePtr,	/* IN -- */
				                     tapeMob,	/* IN -- */
					                 pictureDef,	/* IN -- */
					                 2,	/* Video Track ID IN -- */
				            	     videoRate,	/* IN -- */
					                 mediaStartTC2.startFrame,	/* IN -- */
					                 nFrames));	/* IN -- */
	
		/* Link file mob picture track to tape mob picture track */
		printf("Linking file mob picture track 1 to tape mob picture track 1\n");
		CHECK(omfmMobAddPhysSourceRef(filePtr,
									  fileMob,
				                      			videoRate,	/* IN -- */
									  1, /* file mob track id */
									  pictureDef, /* mediaKind */
									  tapeMob, /* source ref */
									  tcOffset1, /* srcRefOffset */
									  2,	/* srcRefTrack ID (tape mob) */
									  clipLen)); /* srcRefLength */
		
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);											
}

static omfErr_t CreateDiscontiguousTapeMobTypeB(omfHdl_t filePtr, 
									omfObject_t fileMob, 
									omfLength_t clipLen)
{	
	omfObject_t		tapeMob = NULL;
	omfDDefObj_t 	pictureDef = NULL;
	omfErr_t		omfError = OM_ERR_NONE;
	omfRational_t	videoRate;
	omfTimecode_t	mediaStartTC1, mediaStartTC2;
	omfTimecode_t	tapeStartTC;
	omfPosition_t		tcOffset1, tcOffset2, zeroOffset;
	omfFrameOffset_t	clipLen32;

	/* Create the tape mob with 24 hrs TC track id 0
	 * starting at 00:00:00:00, picture track id 1 
	 */ 
	XPROTECT(filePtr)
	{
		omfsTruncInt64toUInt32(clipLen, &clipLen32);
		omfsCvtInt32toPosition(0, zeroOffset);
		MakeRational(2997, 100, &videoRate);
		CHECK(omfsStringToTimecode("01:00:00:00", videoRate,
								   &mediaStartTC1)); 
		omfsCvtInt32toPosition(mediaStartTC1.startFrame, tcOffset1);
		CHECK(omfsStringToTimecode("02:00:00:00", videoRate,
								   &mediaStartTC2)); 
		omfsCvtInt32toPosition(mediaStartTC2.startFrame, tcOffset2);
		CHECK(omfsStringToTimecode("00:00:00:00", videoRate,
								   &tapeStartTC)); 
		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureDef,
						   &omfError); 
		if (omfError != OM_ERR_NONE) RAISE(omfError);

		printf("Creating Tape Mob\n");
		CHECK(omfmTapeMobNew(filePtr,
							 "Created discontiguous (Type B) with medium-level calls",
							 &tapeMob));
	
		printf("Add to tape mob 24 hr TC track id 1, picture track id 2\n");

		CHECK(omfmMobAddTimecodeClip(filePtr,
									 tapeMob,
									 videoRate,
									 1,	/* track label */
									 mediaStartTC1, /* start TC */
									 clipLen32));	/* length */
		CHECK(omfmMobAddTimecodeClip(filePtr,
									 tapeMob,
									 videoRate,
									 1,	/* track label */
									 mediaStartTC2, /* start TC */
									 clipLen32));	/* length */

		/* Make empty source clip on picture track of tape mob, link
		   file mob picture track to this track */
	
		/* Make picture track with empty source clip in tape mob */
		printf("Creating picture track id 1 on tape mob, empty source clip\n");
		CHECK(omfmMobValidateTimecodeRange(filePtr,	/* IN -- */
				                     tapeMob,	/* IN -- */
					                 pictureDef,	/* IN -- */
					                 2,	/* Video Track ID IN -- */
				            	     videoRate,	/* IN -- */
					                 0,	/* IN -- */
					                 nFrames * 2));	/* IN -- */
	
		/* Link file mob picture track to tape mob picture track */
		printf("Linking file mob picture track 1 to tape mob picture track 1\n");
		CHECK(omfmMobAddPhysSourceRef(filePtr,
									  fileMob,
				                      			videoRate,	/* IN -- */
									  1, /* file mob track id */
									  pictureDef, /* mediaKind */
									  tapeMob, /* source ref */
									  zeroOffset, /* srcRefOffset */
									  2,	/* srcRefTrack ID (tape mob) */
									  clipLen)); /* srcRefLength */
		
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);											
}

static omfErr_t CreateTapeMobLow(omfHdl_t filePtr, 
								 omfObject_t fileMob, 
								 omfLength_t clipLen)
{
	omfObject_t		tapeMob = NULL, timecodeClip = NULL;
	omfDDefObj_t 	pictureDef = NULL;
	omfErr_t		omfError = OM_ERR_NONE;
	omfRational_t	videoRate;
	omfTimecode_t	mediaStartTC, tapeStartTC;
	omfPosition_t	zeroPos, tcOffset;
	omfLength_t		tcLen;
	omfFrameOffset_t	clipLen32;
	omfMSlotObj_t	track = NULL;
	
	XPROTECT(filePtr)
	{
		MakeRational(2997, 100, &videoRate);
		CHECK(omfsStringToTimecode("01:00:00:00", videoRate,
								   &mediaStartTC)); 
		CHECK(omfsStringToTimecode("00:00:00:00", videoRate,
								   &tapeStartTC)); 
		omfsCvtInt32toPosition(0, zeroPos);
		omfsCvtInt32toPosition(mediaStartTC.startFrame, tcOffset);
		omfsCvtInt32toLength(mediaStartTC.startFrame, tcLen);
		omfsAddInt64toInt64(clipLen, &tcLen);
		omfsTruncInt64toUInt32(clipLen, &clipLen32);
		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureDef,
						   &omfError); 
		if (omfError != OM_ERR_NONE) RAISE(omfError);

		/* Create the tape mob with 8 frames TC track id 1
		   starting at 01:00:00:00, picture track id 2 */ 
		printf("Creating Tape Mob\n");
		CHECK(omfmTapeMobNew(filePtr,
							"Created with low-level calls",
							 &tapeMob));
	
		/* create timecode clip, append to new track in tape mob */
		printf("Creating timecode clip\n");
		CHECK(omfiTimecodeNew(filePtr,
								  tcLen,
								  tapeStartTC,
								  &timecodeClip));
	
		printf("Appending timecode clip to new track id 0 in tape mob\n");
		CHECK(omfiMobAppendNewTrack(filePtr,
									tapeMob,
									videoRate,
									timecodeClip,
									zeroPos,
									1, /* trackID */
									"Timecode Track id 0", /* trackName */
									&track));
									
		/* Make empty source clip on picture track of tape mob, link
		   file mob picture track to this track */
	
		/* Make picture track with empty source clip in tape mob */
		printf("Creating picture track id 1 on tape mob, empty source clip\n");
		CHECK(omfmMobValidateTimecodeRange(filePtr,	/* IN -- */
				                     tapeMob,	/* IN -- */
					                 pictureDef,	/* IN -- */
					                 2,	/* Track ID IN -- */
				            	     videoRate,	/* IN -- */
					                 mediaStartTC.startFrame,	/* IN -- */
					                 nFrames));	/* IN -- */
	
		/* Link file mob picture track to tape mob picture track */
		printf("Linking file mob picture track 1 to tape mob picture track 1\n");
		CHECK(omfmMobAddPhysSourceRef(filePtr,
									  fileMob,
				                      videoRate,	/* IN -- */
									  1, /* file mob track id */
									  pictureDef, /* mediaKind */
									  tapeMob, /* source ref */
									  tcOffset, /* srcRefOffset */
									  2,	/* srcRefTrack ID (tape mob) */
									  clipLen)); /* srcRefLength */
			
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);											
}

void VideoCheckTestFile(char *name)
{

	omfSessionHdl_t sessionPtr = NULL;
	omfHdl_t        filePtr = NULL;
	omfObject_t		masterMob = NULL;
	omfIterHdl_t	masterMobIter = NULL;
	omfMediaHdl_t	mediaPtr = NULL;
	omfSearchCrit_t search;
	omfPosition_t testPos;
	omfUInt32		resultPos;
	omfTimecode_t	resultTC;
	char				mobName[128];
	
    XPROTECT(NULL)
	  {
	  	omfsCvtInt32toInt64(5, &testPos);
		CHECK(omfsBeginSession(NULL, &sessionPtr));
	
		CHECK(omfmInit(sessionPtr));
	
		CHECK(omfsOpenFile((fileHandleType)name, sessionPtr, &filePtr));
	
		CHECK(omfiIteratorAlloc(filePtr, &masterMobIter));
	
		search.searchTag = kByMobKind;
		search.tags.mobKind = kMasterMob;
		while(omfiGetNextMob(masterMobIter, &search, &masterMob) == OM_ERR_NONE)
		{
			CHECK(omfiMobGetInfo(filePtr,  masterMob, NULL,  sizeof(mobName),
								mobName, NULL, NULL));

			CHECK(omfmOffsetToTimecode(filePtr, masterMob,  1, testPos, &resultTC));
			if(resultTC.startFrame != 108005)
				RAISE(OM_ERR_TEST_FAILED);

			CHECK(omfmTimecodeToOffset( filePtr, resultTC, masterMob, 1, &resultPos));
			if(5 != resultPos)
				RAISE(OM_ERR_TEST_FAILED);
		}
		
		CHECK(omfiIteratorDispose(filePtr, masterMobIter));
		printf("File checks out OK.\n");

		CHECK(omfsCloseFile(filePtr));
		CHECK(omfsEndSession(sessionPtr));

		FreeGlobals();

	  }
	XEXCEPT
	  {
#ifdef OMFI_ERROR_TRACE
		omfPrintStackTrace(filePtr);
#endif
	  }
  	XEND_VOID

	  return;
  }

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/



