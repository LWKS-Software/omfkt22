/***************************************************************************
 * apps/omfinfo: This application finds the media data objects in the OMF  *
 *               file (audio and video) and prints the descriptor          *
 *               information.                                              *
 ***************************************************************************/
#include "masterhd.h"
#include <stdio.h>
#include <string.h>

#ifdef MSDOS
#include <stdlib.h>
#endif

#if PORT_SYS_MAC
#include <Types.h>
#include <OSUtils.h>
#if !defined(MAC_DRAG_DROP)
#include <console.h>
#include <StandardFile.h>
#endif
#include "MacSupport.h"
#endif

#ifdef MAC_DRAG_DROP
#include "Main.h"
#endif

#include "omPublic.h"
#include "omMedia.h"
#include "omCodec.h"
#include "omPvt.h"
#include "omcTIFF.h"
#include "avrPvt.h"
#include "omJPEG.h"
#include "avrs.h"
#if AVID_CODEC_SUPPORT
#include "omCodec.h"
#include "omcAvJPED.h"
#include "omcAvidJFIF.h"
#endif
#include "InfoSupp.h"

/***********
 * DEFINES *
 ***********/
#define NAMESIZE 256

/**********
 * MACROS *
 **********/
#define ContinueOnOMFError(e, m) \
     if (e != OM_ERR_NONE) \
       { \
	 printf("\n%s\n", omfsGetErrorString(e)); \
	 printf("%s\n", m); \
         continue; \
       }

#define ExitIfNull(b, m) \
  if (!b) { \
      printf("%s: %s\n", argv[0], m); \
      return(-1); \
    }

/**************
 * PROTOTYPES *
 **************/
static omfErr_t PrintAudioInfo(omfHdl_t fileHdl,
			       omfMediaHdl_t mediaHdl);
static omfErr_t PrintVideoInfo(omfHdl_t fileHdl,
			       omfMediaHdl_t mediaHdl);
static omfErr_t GetAVRString(omfInt32 tableID, char *outstr);
static omfErr_t PrintMediaLocation(omfHdl_t fileHdl,
				   omfMobObj_t mob,
				   omfTrackID_t trackID,
				   omfMediaHdl_t mediaHdl);
static omfErr_t PrintTapeMobInfo(omfHdl_t fileHdl,
				 omfMobObj_t mob);
static omfErr_t PrintFilmMobInfo(omfHdl_t fileHdl,
				 omfMobObj_t mob);
static void usage(char *cmdName,
		  omfProductVersion_t toolkitVers,
		  omfInt32 appVers);
static void printHeader(char *name,
			omfProductVersion_t toolkitVers,
			omfInt32 appVers);
static omfHdl_t locatorFailure(omfHdl_t file, omfObject_t mdes);
static omfErr_t PrintPulldownInfo(omfHdl_t fileHdl,
			       omfObject_t pulldown);

/********************/
/* GLOBAL VARIABLES */
/********************/
const omfInt32 omfiAppVersion = 4;
omfBool		printQTables = FALSE;
omfInt32	frameSizeCount = 0;

omfPosition_t	sourcePos;

/*******************
 * VERSION HISTORY *
 *******************/
/* JeffB: Rev 2 changes
 * 		Better handling of missing field dominance & line map (from TIFF)
 * 		Add display rect printing for video
 * JeffB: Rev 3 changes
 * 		Added printing of:
 *			user attributes (Media Composer bin comments)
 * 			IDNT audit trail information
 *			Q table information (optional)
 *			Frame size information (optional)
 */

/****************
 * MAIN PROGRAM *
 ****************/
int main(int argc, char *argv[])
{
    omfSessionHdl_t session;
    omfHdl_t fileHdl = NULL;
    omfMediaHdl_t mediaHdl = NULL;
    char *cmdname, *fname, *printfname;
    omfFileRev_t fileRev;
    omfIterHdl_t mobIter = NULL, trackIter = NULL;
    omfIterHdl_t mediaIter = NULL, sourceIter = NULL, commentIter = NULL;
    omfInt32 numMobs, numTracks, numComments;
    char name[NAMESIZE], trackType[OMUNIQUENAME_SIZE];
	char prtBuf[64];
    omfTimeStamp_t createTime, lastModified;
    omfInt32 loop, trackLoop, skipLoop, cmnt;
    omfSearchCrit_t searchCrit;
    omfUID_t mobID;
    omfRational_t editRate;
    omfTrackID_t trackID;
    omfInt16 tmp1xTrackID = 0;
    omfLength_t length;
    omfMobObj_t mob = NULL;
    omfMSlotObj_t track = NULL;
    omfSegObj_t segment = NULL;
    omfDDefObj_t datakind = NULL, pictureDef = NULL, soundDef = NULL;
    omfBool isPrimary = TRUE;
    omfInt16 numVChannels, numAChannels;
    omfTimecode_t startTC;
    omfInt32 numTapeMobs = 0, numFilmMobs = 0;
    char errBuf[256];
    char	commentName[64], commentValue[256];
    omfErr_t omfError = OM_ERR_NONE;
    int  	filearg;
    omfPosition_t	zero;
	omfFindSourceInfo_t		srcInfo;
   enum omfEffectChoice_t effectChoice = kFindEffectSrc1;


#if PORT_SYS_MAC && !defined(MAC_DRAG_DROP)
    char            namebuf[255];
    Point           where;
    SFReply         fs;
    SFTypeList      fTypes;
#endif

	omfsCvtInt32toPosition(0, sourcePos);

#if PORT_SYS_MAC && !defined(MAC_DRAG_DROP)
    argc = ccommand(&argv);

    MacInit();

    where.h = 20;
    where.v = 40;
#else
    /* Process command line switches */
    if (argc < 2)
      {
	usage(argv[0], omfiToolkitVersion, omfiAppVersion);
	return(-1);
      }
#endif

    omfsCvtInt32toInt64(0, &zero);

    XPROTECT(NULL)
      {
	cmdname = argv[0];

	/* Start an OMF Session */
	printHeader(cmdname, omfiToolkitVersion, omfiAppVersion);
	CHECK(omfsBeginSession(NULL, &session));
	CHECK(omfmInit(session));
#if AVID_CODEC_SUPPORT
	CHECK(omfmRegisterCodec(session, omfCodecAvJPED, kOMFRegisterLinked));
	CHECK(omfmRegisterCodec(session, omfCodecAvidJFIF, kOMFRegisterLinked));
#endif

	printQTables = FALSE;
	frameSizeCount = 0;
	for (filearg = 1; filearg < argc; filearg++)
	{
		if(argv[filearg][0] == '-')
		{
			if(strcmp(argv[filearg], "qtab") == 0)
				printQTables = TRUE;
			if(strcmp(argv[filearg], "-fs") == 0)
			{
				filearg++;
				sscanf(argv[filearg], "%ld", &frameSizeCount);
			}
			if(strcmp(argv[filearg], "-o") == 0)
			{
            unsigned int  lPos;
				filearg++;
				sscanf(argv[filearg], "%ld", &lPos);
				omfsCvtInt32toPosition(lPos, sourcePos);
			}

		}
	}

	/* BEGINNING OF WHILE LOOP */
#if PORT_SYS_MAC && !defined(MAC_DRAG_DROP)
	while (1)	/* Until the user chooses quit from the menu */
#else
	  /* Command line interface */
	for (filearg = 1; filearg < argc; filearg++)
#endif
	  {
		if(argv[filearg][0] == '-')
			continue;
	    /* Get the OMF filename */
#if PORT_SYS_MAC && !defined(MAC_DRAG_DROP)
	    fs.fName[0] = '\0';
	    fTypes[0] = 'OMFI';
	    SFGetFile(where, NULL, NULL, -1, fTypes, NULL, &fs);

	    /* If the name is empty, exit */
	    if (fs.fName[0] == '\0')
	      return(0);
	    ExitIfNull(fs.good, "\nomfInfo completed successfully\n");

	    fname = (char *) &fs;
	    strncpy(namebuf, (char *) &(fs.fName[1]), (size_t) fs.fName[0]);
	    namebuf[fs.fName[0]] = 0;
	    printfname = namebuf;
#else				/* Not Macintosh */
	    fname = argv[filearg];
	    printfname = fname;
#endif

	    /* Open the OMF file */
	    printf("\n***** OMF File: %s *****\n", printfname);

	    omfError = omfsOpenFile(fname, session, &fileHdl);

		if(omfError != OM_ERR_NONE)
		{
			printf("***ERROR: %s\nSkipping file...\n",
	      	 omfsGetExpandedErrorString(fileHdl, omfError,
					  sizeof(errBuf), errBuf));
			continue;
		}
#if AVID_CODEC_SUPPORT
		omfsSemanticCheckOff(fileHdl);
#endif

	    /* Print out the file revision */
	    CHECK(omfsFileGetRev(fileHdl, &fileRev));
	    if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
	      printf("OMF File Revision 1.0\n");
	    else if (fileRev == kOmfRev2x)
	      printf("OMF File Revision 2.0\n");
	    else
	      printf("OMF File Revision UNKNOWN\n");

/*	    omfmLocatorFailureCallback(fileHdl, locatorFailure);	*/

	    omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
	    omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);

	    /* Print the IDNT audit trail */
		omfPrintAuditTrail(fileHdl);

	    /* Get Composition Mobs */
	    printf("\nCompositions\n");
	    printf(  "------------\n");
	    CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	    CHECK(omfiGetNumMobs(fileHdl, kCompMob, &numMobs));
	    if (numMobs == 0)
	      printf("No Compositions found.\n");

	    /* Iterate over the composition mobs, if there are any */
	    for (loop = 0; loop < numMobs; loop++)
	      {
		searchCrit.searchTag = kByMobKind;
		searchCrit.tags.mobKind = kCompMob;
		CHECK(omfiGetNextMob(mobIter, &searchCrit, &mob));
		/* NOTE: Print create/modified times */
		strcpy( (char*) name, "\0");
		omfError = omfiMobGetInfo(fileHdl, mob, &mobID,
					  NAMESIZE, name,
					  &lastModified, &createTime);
		if (omfError != OM_ERR_NONE)
		  {
		    printf("ERROR: Retrieving information from mob\n");
		    continue;
		  }

		CHECK(omfiMobGetNumTracks(fileHdl, mob, &numTracks));
		isPrimary = omfiIsAPrimaryMob(fileHdl, mob, &omfError);
		printf("COMPOSITION MOB ID ( %1ld, %1ld, %1ld )\n", mobID);
		printf("    Name: %s", name);
		if (isPrimary)
		  printf(" (PRIMARY Mob)\n");
		else
		  printf("\n");

	    CHECK(omfiMobGetNumComments(fileHdl, mob, &numComments));
	    if(numComments != 0)
	    {
		    printf("        Comments:\n", trackID);
	    	CHECK(omfiIteratorAlloc(fileHdl, &commentIter));
	    	for(cmnt = 1; cmnt <= numComments; cmnt++)
	    	{
	    		CHECK(omfiMobGetNextComment(commentIter, mob, sizeof(commentName), commentName,
	    									sizeof(commentValue), commentValue));
	    		printf("            %s = '%s'\n", commentName, commentValue);
	    	}
	    	CHECK(omfiIteratorDispose(fileHdl, commentIter));
	    	printf("\n");
	    }

		/* Iterate over tracks in the mob */
		CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
		for (trackLoop = 0; trackLoop < numTracks; trackLoop++)
		  {
		    CHECK(omfiMobGetNextTrack(trackIter, mob, NULL, &track));
		    /* NOTE: Also get origin, etc.? */
		    strcpy( (char*) name, "\0");
		    omfError = omfiTrackGetInfo(fileHdl, mob, track, &editRate,
						NAMESIZE, name,
						NULL, &trackID, &segment);
		    if (omfError != OM_ERR_NONE)
		      {
			printf("       ERROR: Incomplete Track\n");
		      }
		    else /* Track is fine */
		      {
			omfError = omfiComponentGetInfo(fileHdl, segment,
							&datakind, &length);
			if (omfError != OM_ERR_NONE)
			  {
			    printf("ERROR: Retrieving information from track component\n");
			    continue;
			  }
			if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
			  {
			    CHECK(omfsReadInt16(fileHdl, track,
						OMTRAKLabelNumber,
						(omfInt16 *)&tmp1xTrackID));
			    trackID = (omfTrackID_t)tmp1xTrackID;
			  }

			CHECK(omfsInt64ToString(length, 10, sizeof(prtBuf), prtBuf));
			CHECK(omfiDatakindGetName(fileHdl, datakind,
					  OMUNIQUENAME_SIZE, trackType));
			printf("       Track %ld -- %s\n", trackID, trackType);
			printf("             Name: %s \n", name);
			printf("             Length: %s \n", prtBuf);
			printf("             Editrate: %d/%d\n", editRate);
			if (omfiIsTimecodeKind(fileHdl, datakind, kExactMatch,
					       &omfError))
			  {
			    omfError = omfiTimecodeGetInfo(fileHdl, segment,
						   NULL, NULL, &startTC);
			    /* NOTE: some Avid FC timecode tracks have masks
			     * and other things in them, ignore this case
			     * for now
			     */
			    if (omfError == OM_ERR_NONE)
			      {
				switch (startTC.drop)
				  {
				  case kTcNonDrop:
				    printf("             Non-Drop Timecode\n");
				    break;
				  case kTcDrop:
				    printf("             Drop Timecode\n");
				    break;
				  default:
				    break;
				  }
				printf("             Start Frame: %d\n",
				       startTC.startFrame);
				printf("             Frames/Sec - %d\n",
				       startTC.fps);
			      }
			  }
		      } /* if track error */
		  }
		printf("\n");
		CHECK(omfiIteratorDispose(fileHdl, trackIter));
		trackIter = NULL;
	      }
	    CHECK(omfiIteratorDispose(fileHdl, mobIter));
	    mobIter = NULL;

	    printf("\nMedia Data\n");
	    printf("----------\n");
	    /* Get to the media data via master mobs */
	    CHECK(omfiIteratorAlloc(fileHdl, &mediaIter));
	    CHECK(omfiGetNumMobs(fileHdl, kMasterMob, &numMobs));
	    if (numMobs == 0)
	      printf("No Media Data found.\n");

	    /* Iterate over the media data, via their master mobs */
	    for (loop = 0; loop < numMobs; loop++)
	      {
		searchCrit.searchTag = kByMobKind;
		searchCrit.tags.mobKind = kMasterMob;
		omfError = omfiGetNextMob(mediaIter, &searchCrit, &mob);
		ContinueOnOMFError(omfError, "Skipping media object");
		strcpy( (char*) name, "\0");
		CHECK(omfiMobGetInfo(fileHdl, mob, &mobID,
				     NAMESIZE, name,
				     &lastModified, &createTime));

		printf("MASTER MOB ID ( %1ld, %1ld, %1ld )\n", mobID);
		printf("    Name: %s\n", name);


	    CHECK(omfiMobGetNumComments(fileHdl, mob, &numComments));
	    if(numComments != 0)
	    {
		    printf("        Comments:\n", trackID);
	    	CHECK(omfiIteratorAlloc(fileHdl, &commentIter));
	    	for(cmnt = 1; cmnt <= numComments; cmnt++)
	    	{
	    		CHECK(omfiMobGetNextComment(commentIter, mob, sizeof(commentName), commentName,
	    									sizeof(commentValue), commentValue));
	    		printf("            %s = '%s'\n", commentName, commentValue);
	    	}
	    	CHECK(omfiIteratorDispose(fileHdl, commentIter));
	    	printf("\n");
	    }

		CHECK(omfiMobGetNumTracks(fileHdl, mob, &numTracks));
		CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
		trackLoop = numTracks;
		while (trackLoop > 0)
		  {
		    CHECK(omfiMobGetNextTrack(trackIter, mob, NULL, &track));
		    strcpy( (char*) name, "\0");
		    omfError = omfiTrackGetInfo(fileHdl, mob, track, &editRate,
						NAMESIZE, name,
						NULL, &trackID, &segment);
		    if (omfError != OM_ERR_NONE)
		      {
			printf("ERROR: Retrieving information from track\n");
			trackLoop--;
			continue;
		      }
		    printf("        Track %d", trackID);

		    omfError = omfmGetNumChannels(fileHdl, mob, trackID,
						  NULL, /* media criteria */
						  pictureDef,
						  &numVChannels);
		    if (omfError != OM_ERR_NONE)
		      {
			printf("\n        OMFI_ERR: Missing Media -- Skipping master mob track\n\n");
			trackLoop--;
			continue;
		      }

		    CHECK(omfmGetNumChannels(fileHdl, mob, trackID,
					     NULL, /* media criteria */
					     soundDef,
					     &numAChannels));

		    /* NOTE: there could potentially be video/audio interleaved
		     *       data.  In this case, both numVChannels and
		     *       numAChannels will be non-0.
		     */

		    if ((numVChannels > 0) && (numAChannels > 0))
		      printf(" -- INTERLEAVED MEDIA: %d V channels, %d A channels\n",
			     numVChannels, numAChannels);
		    else if (numVChannels > 0)
		      {
			if (numVChannels == 1)
			  printf(" -- PICTURE\n");
			else
			  printf(" -- ILLEGAL PICTURE TRACK: 2 or more channels\n");
		      }
		    else if (numAChannels > 0)
		      {
			if (numAChannels == 1)
			  printf(" -- MONO SOUND\n");
			else
			  {
			    printf(" -- INTERLEAVED SOUND: %d channels\n",
				   numAChannels);
			    /* Reset Number of Audio Channels for 1.x, since
			     * only one track is used to represent stereo
			     * in the master mob (unlike 2.0)
			     */
			    if ((fileRev == kOmfRev1x) ||(fileRev == kOmfRevIMA))
			      {
				numAChannels = 1;
			      }
			  }
		      }
		    else
		      printf(" -- ILLEGAL TRACK:  no picture or sound data\n");

		    /* Is there video information? */
		    if (numVChannels > 0)
		      {
			omfError = omfmMediaOpenFromOffset(fileHdl, mob, sourcePos, trackID, NULL,
						 kMediaOpenReadOnly,
						 kToolkitCompressionDisable,
						 &mediaHdl);
			if (omfError == OM_ERR_MEDIA_NOT_FOUND)
			  {
			    printf("            Could not locate media\n");
			    CHECK(PrintMediaLocation(fileHdl, mob, trackID,
						     mediaHdl));
			  }
			else if (omfError != OM_ERR_NONE)
			  {
			    printf("            ERROR trying to open media\n");
			  }
			else
			  {
			    CHECK(PrintMediaLocation(fileHdl, mob, trackID,
						     mediaHdl));
			    CHECK(PrintVideoInfo(fileHdl, mediaHdl));
			    CHECK(omfmMediaClose(mediaHdl));
			  }
		      }

		    /* Is there audio information? */
		    if (numAChannels > 0)
		      {
			omfError = omfmMediaOpen(fileHdl, mob, trackID, NULL,
						 kMediaOpenReadOnly,
						 kToolkitCompressionDisable,
						 &mediaHdl);
			if (omfError == OM_ERR_MEDIA_NOT_FOUND)
			  {
			    printf("            Could not locate media\n");
			    CHECK(PrintMediaLocation(fileHdl, mob, trackID,
						     mediaHdl));
			  }
			else if (omfError != OM_ERR_NONE)
			  {
			    printf("            ERROR trying to open media\n");
			  }
			else
			  {
			    CHECK(PrintMediaLocation(fileHdl, mob, trackID,
						     mediaHdl));
			    CHECK(PrintAudioInfo(fileHdl, mediaHdl));
			    CHECK(omfmMediaClose(mediaHdl));
			  }
		      }

			/* See if there are any pulldown objects present on the chain */
         /* MC 01-Dec-03 -- fix for crash */
			omfError = omfiMobSearchSource(fileHdl, mob, trackID, zero,
				kFilmMob, NULL, &effectChoice, NULL, &srcInfo);
			if(omfError != OM_ERR_NONE)
			{
				omfError = omfiMobSearchSource(fileHdl, mob, trackID, zero,
					kFilmMob, NULL, &effectChoice, NULL, &srcInfo);
			}
			if(omfError == OM_ERR_NONE)
			{
				if(srcInfo.filmTapePdwn != NULL)
				{
					PrintPulldownInfo(fileHdl, srcInfo.filmTapePdwn);
				}
				if(srcInfo.tapeFilmPdwn != NULL)
				{
					PrintPulldownInfo(fileHdl, srcInfo.tapeFilmPdwn);
				}
			}
     		printf("\n");

		    /* If interleaved data, skip past subchannels */
		    trackLoop -= (numVChannels + numAChannels);
		    for (skipLoop = 1; skipLoop < numVChannels + numAChannels;
			 skipLoop++)
		      {
			CHECK(omfiMobGetNextTrack(trackIter, mob, NULL, &track));
		      }
		  }
		CHECK(omfiIteratorDispose(fileHdl, trackIter));
		trackIter = NULL;
	      }
	    CHECK(omfiIteratorDispose(fileHdl, mediaIter));
	    mediaIter = NULL;

	    /* Get Physical Source Mobs */
	    printf("\nPhysical Source Info\n");
	    printf(  "--------------------\n");
	    CHECK(omfiIteratorAlloc(fileHdl, &sourceIter));
	    CHECK(omfiGetNumMobs(fileHdl, kAllMob, &numMobs));
	    for (loop = 0; loop < numMobs; loop++)
	      {
		searchCrit.searchTag = kByMobKind;
		searchCrit.tags.mobKind = kAllMob;
		CHECK(omfiGetNextMob(sourceIter, &searchCrit, &mob));
		strcpy( (char*) name, "\0");
		CHECK(omfiMobGetInfo(fileHdl, mob, &mobID,
				     NAMESIZE, name,
				     &lastModified, &createTime));
		if (omfiIsATapeMob(fileHdl, mob, &omfError))
		  {
		    printf("TAPE MOB ID ( %1ld, %1ld, %1ld )\n", mobID);
		    printf("    Name: %s\n", name);
		    numTapeMobs++;
		    /* Print out descriptive information, if 2.x file */
		    if (fileRev == kOmfRev2x)
		      {
			CHECK(PrintTapeMobInfo(fileHdl, mob));
			printf("\n");
		      }
		  }
		else if (omfiIsAFilmMob(fileHdl, mob, &omfError))
		  {
		    printf("FILM MOB ID ( %1ld, %1ld, %1ld )\n", mobID);
		    printf("    Name: %s\n", name);
		    numFilmMobs++;
		    /* Print out descriptive information, if 2.x file */
		    if (fileRev == kOmfRev2x)
		      {
			CHECK(PrintFilmMobInfo(fileHdl, mob));
			printf("\n");
		      }
		  }
	      } /* for loop */
	    CHECK(omfiIteratorDispose(fileHdl, sourceIter));
	    sourceIter = NULL;

	    if ((numTapeMobs + numFilmMobs) == 0)
	      printf("No Physical Source info found.\n");

	    /* Close the OMF File */
	    CHECK(omfsCloseFile(fileHdl));
	  } /* file iteration loop */

	/* Close the OMF Session */
	CHECK(omfsEndSession(session));

	printf("\nomfInfo completed successfully\n");
      } /* XPROTECT */

    XEXCEPT
      {
	if (mobIter)
	  omfiIteratorDispose(fileHdl, mobIter);
	if (mediaIter)
	  omfiIteratorDispose(fileHdl, mediaIter);
	if (sourceIter)
	  omfiIteratorDispose(fileHdl, sourceIter);

	printf("***ERROR: %d: %s\n", XCODE(),
	       omfsGetExpandedErrorString(fileHdl, XCODE(),
					  sizeof(errBuf), errBuf));
	return(-1);
      }
    XEND;

    return(0);
}

#define MAX_NUM_VID_OPS 5

static omfErr_t PrintVideoInfo(omfHdl_t fileHdl,
			       omfMediaHdl_t mediaHdl)
{
  omfTIFF_JPEGInfo_t TIFFInfo;
  omfCodecID_t codecID;
  char codecName[NAMESIZE];
  omfFrameLayout_t layout;
  char layoutString[NAMESIZE];
  omfInt32 fieldWidth;
  omfInt32 fieldHeight;
  omfInt16 bitsPerPixel;
  omfPixelFormat_t defaultMemFmt;
  omfRational_t editRate;
  char tableIDStr[NAMESIZE];
  omfLength_t numSamples;
  omfInt32 n;
  char prtBuf[64];
  omfVideoSignalType_t signalType;
  omfFieldTop_t fieldTop;
  omfVideoMemOp_t vidOpList[MAX_NUM_VID_OPS];
  char		errBuf[256];
  char		lenTxt[64];
  omfErr_t status;
  omfPosition_t	frameNum64;
	omfRect_t	dispRect;
	omfLength_t	frameSize;
	omfInt32	printFrames;

  XPROTECT(NULL)
    {
      CHECK(omfmMediaGetCodecID(mediaHdl, &codecID));
      CHECK(omfmCodecGetName(fileHdl, codecID, NAMESIZE, codecName));
      CHECK(omfmGetVideoInfo(mediaHdl,
			     &layout,
			     &fieldWidth,
			     &fieldHeight,
			     &editRate,
			     &bitsPerPixel,
			     &defaultMemFmt));

      printf("            Video Format  %s\n", codecName);
      printf("            Edit Rate     %d/%d\n", editRate.numerator,
			                                      editRate.denominator);

      printf("            Bits/Pixel    %d\n", bitsPerPixel);
      printf("            Image Size    %ld x %ld (stored)\n",
			                     fieldWidth, fieldHeight);
	  CHECK(omfmGetDisplayRect(mediaHdl, &dispRect));
      printf("                          %ld x %ld (displayed)\n",
			                     dispRect.xSize, dispRect.ySize);
      switch (layout)
		{
		case kFullFrame:
		  strcpy( (char*) layoutString, "Full Frame");
		  break;
		case kSeparateFields:
		  strcpy( (char*) layoutString, "Separate Fields");
		  break;
		case kOneField:
		  strcpy( (char*) layoutString, "One Field");
		  break;
		case kMixedFields:
		  strcpy( (char*) layoutString, "Mixed Fields");
		  break;
		default:
		  strcpy( (char*) layoutString, "Null Layout");
		  break;
		}
      printf("            Layout        %s\n", layoutString);

	status = omfmSourceGetVideoSignalType(mediaHdl, &signalType);
	if (status == OM_ERR_NONE)
	{
		switch(signalType) {
		case kNTSCSignal:
			printf("            Signal        NTSC\n");
			break;
		case kPALSignal:
			printf("            Signal        PAL\n");
			break;
		case kSECAMSignal:
			printf("            Signal        SECAM\n");
			break;
      	default:
			printf("            Signal        Not known\n");
			break;
		}
	}
    else
		printf("%s\n",omfsGetExpandedErrorString(fileHdl, status,
					 sizeof(errBuf), errBuf));

	status = omfmGetVideoTopField(mediaHdl, &fieldTop);
	if (status == OM_ERR_NONE)
	{
		switch(fieldTop) {
		case kTopField1:
			printf("            Top Field:    Field 1\n");
			break;
		case kTopField2:
			printf("            Top Field:    Field 2\n");
			break;
		case kTopFieldNone:
			printf("            Top Field:    None\n");
			break;
      	default:
			printf("            Top Field:    Not known\n");
			break;
		}
	}
	else if(status == OM_ERR_INVALID_OP_CODEC)
	  {
		printf("            Top Field:    Not known\n");
	  }
	else
		printf("%s\n",omfsGetExpandedErrorString(fileHdl, status,
						 sizeof(errBuf), errBuf));

	vidOpList[0].opcode = kOmfVideoLineMap;
	vidOpList[1].opcode = kOmfVFmtEnd;
	status = omfmGetVideoInfoArray(mediaHdl, vidOpList);

	if (status == OM_ERR_NONE)
	{
		omfInt32 field1, field2;
		field1 = vidOpList[0].operand.expLineMap[0];
		field2 = vidOpList[0].operand.expLineMap[1];
		if (field1 !=0 || field2 != 0)
		{
			printf("            LineMap       %1ld, %1ld\n",
					field1, field2);
		}
	}
	else if(status == OM_ERR_INVALID_OP_CODEC)
	  {
		printf("            LineMap       Not known\n");
	  }
	else
		printf("%s\n",omfsGetExpandedErrorString(fileHdl, status,
						 sizeof(errBuf), errBuf));

      /* This function returns private codec data.  In this
       * case it is returning the AVR information from
       * a TIFF object.
       */
      if (!strcmp(codecID, CODEC_TIFF_VIDEO))
	{
	  CHECK(omfmGetPrivateMediaData(mediaHdl,
					sizeof(omfTIFF_JPEGInfo_t),
					&TIFFInfo));
	  switch (TIFFInfo.compression)
	    {
	    case knoComp:
	      printf("            Compression   No Compression\n");
	      break;
	    case kJPEG:
	      printf("            Compression   JPEG\n");
	      CHECK(GetAVRString(TIFFInfo.JPEGTableID, tableIDStr));
	      printf("            JPEG Tbl ID   %s\n", tableIDStr);
	      break;
	    case kLSIJPEG:
	      printf("            Compression   LSI JPEG\n");
	      break;
	    }
	  }
#if AVID_CODEC_SUPPORT
      else if (!strcmp(codecID,  CODEC_AVID_JPEG_VIDEO))
		{
		omfJPEGInfo_t	JPEDInfo;
	  CHECK(omfmGetPrivateMediaData(mediaHdl,
					sizeof(omfJPEGInfo_t),
					&JPEDInfo));
	   CHECK(GetAVRString(JPEDInfo.JPEGTableID, tableIDStr));
	   printf("            JPEG Tbl ID   %s\n", tableIDStr);
	  }
#endif

      CHECK(omfmGetSampleCount(mediaHdl, &numSamples));
	  CHECK(omfsInt64ToString(numSamples, 10, sizeof(prtBuf), prtBuf));
      printf("            No. Samples   %s\n", prtBuf);

	if(printQTables)
	{
	    omfJPEGTables_t JPEGtables;
		int  i;

	    JPEGtables.JPEGcomp = kJcLuminance;
		status = codecGetInfo(mediaHdl, kJPEGTables, mediaHdl->pictureKind,
								sizeof(JPEGtables), &JPEGtables);
		if(status == OM_ERR_NONE)
		{
			printf("\n            Luminance Q Tables:\n");
			printf("                ");
			for (i=0; i<64; i++)
			{
				printf("%3d", JPEGtables.QTables[i]);
				if ((i+1) % 8)
					printf(", ");
				else
					printf("\n                ");
			}
			omfsFree(JPEGtables.QTables);
			omfsFree(JPEGtables.ACTables);
			omfsFree(JPEGtables.DCTables);
		}

	    JPEGtables.JPEGcomp = kJcChrominance;
		status = codecGetInfo(mediaHdl, kJPEGTables, mediaHdl->pictureKind,
								sizeof(JPEGtables), &JPEGtables);
		if(status == OM_ERR_NONE)
		{
			printf("\n            Chrominance Q Tables:\n");
			printf("                ");
			for (i=0; i<64; i++)
			{
				printf("%3d", JPEGtables.QTables[i]);
				if ((i+1) % 8)
					printf(", ");
				else
					printf("\n                ");
			}
			omfsFree(JPEGtables.QTables);
			omfsFree(JPEGtables.ACTables);
			omfsFree(JPEGtables.DCTables);
		}

		printf("\n");
	  }

		if(frameSizeCount != 0)
		{
		  omfInt32 numSamples32;

			printf("            Frame Sizes:\n");
			printFrames = frameSizeCount;
			omfsTruncInt64toInt32(numSamples, &numSamples32);
			if(printFrames > numSamples32)
				printFrames = numSamples32;
			for(n = 1; n <= printFrames; n++)
			{
				omfsCvtInt32toInt64(n, &frameNum64);
				status = omfmGetSampleFrameSize(mediaHdl, mediaHdl->pictureKind,
											frameNum64, &frameSize);
				if(status != OM_ERR_NONE)
				{
					if((status == OM_ERR_NOFRAMEINDEX) ||
						(status == OM_ERR_INVALID_OP_CODEC))
					{
						printf("                No frame Index\n");
					}
					else
					{
						printf("                ** %s\n",
							omfsGetErrorString(status));
					}
					break;
				}
				CHECK(omfsInt64ToString(frameSize, 10, sizeof(lenTxt), lenTxt));
				printf("                frame %ld size: %s\n", n, lenTxt);
			}
		}
    }

  XEXCEPT
    {
    }
  XEND;

  return(OM_ERR_NONE);
}

static omfErr_t GetAVRString(omfInt32 tableID, char *outstr)
{
  switch (tableID)
    {
    case AVR1_ID:
      sprintf(outstr, "%s", AVR1_NAME);
      break;
    case AVR1e_ID:
      sprintf(outstr, "%s", AVR1e_NAME);
      break;
    case AVR2_ID:
      sprintf(outstr, "%s", AVR2_NAME);
      break;
    case AVR2e_ID:
      sprintf(outstr, "%s", AVR2e_NAME);
      break;
    case AVR2s_ID:
      sprintf(outstr, "%s", AVR2s_NAME);
      break;
    case AVR3_ID:
      sprintf(outstr, "%s", AVR3_NAME);
      break;
    case AVR3e_ID:
      sprintf(outstr, "%s", AVR3e_NAME);
      break;
    case AVR3s_ID:
      sprintf(outstr, "%s", AVR3s_NAME);
      break;
    case AVR4_ID:
      sprintf(outstr, "%s", AVR4_NAME);
      break;
    case AVR4e_ID:
      sprintf(outstr, "%s", AVR4e_NAME);
      break;
    case AVR4m_ID:
      sprintf(outstr, "%s", AVR4m_NAME);
      break;
    case AVR4s_ID:
      sprintf(outstr, "%s", AVR4s_NAME);
      break;
    case AVR5_ID:
      sprintf(outstr, "%s", AVR5_NAME);
      break;
    case AVR5e_ID:
      sprintf(outstr, "%s", AVR5e_NAME);
      break;
    case AVR6e_ID:
      sprintf(outstr, "%s", AVR6e_NAME);
      break;
    case AVR6m_ID:
      sprintf(outstr, "%s", AVR6m_NAME);
      break;
    case AVR6s_ID:
      sprintf(outstr, "%s", AVR6s_NAME);
      break;
    case AVR8s_ID:
      sprintf(outstr, "%s", AVR8s_NAME);
      break;
    case AVR12_ID:
      sprintf(outstr, "%s", AVR12_NAME);
      break;
    case AVR25_ID:
      sprintf(outstr, "%s", AVR25_NAME);
      break;
    case AVR26_ID:
      sprintf(outstr, "%s", AVR26_NAME);
      break;
    case AVR27_ID:
      sprintf(outstr, "%s", AVR27_NAME);
      break;
#if 0
    case AVR27f_ID:
      sprintf(outstr, "%s", AVR27f_NAME);
      break;
#endif
    case AVR70_ID:
      sprintf(outstr, "%s", AVR70_NAME);
      break;
    case AVR71_ID:
      sprintf(outstr, "%s", AVR71_NAME);
      break;
    case AVR75_ID:
      sprintf(outstr, "%s", AVR75_NAME);
      break;
#if 0
    case mspi_id:
      sprintf(outstr, "MSPIndigo");
      break;
#endif

    default:
      sprintf(outstr, "JPEG Tbl ID  %ld",
	      tableID);
      break;
    }

 return(OM_ERR_NONE);
}


static omfErr_t PrintAudioInfo(omfHdl_t fileHdl,
			omfMediaHdl_t mediaHdl)
{
  omfInt32 sampleSize;
  omfInt32 numChannels;
  omfCodecID_t codecID;
  char codecName[NAMESIZE];
  omfLength_t numSamples;
  char prtBuf[64];
  omfRational_t editRate;

  XPROTECT(NULL)
    {
      CHECK(omfmMediaGetCodecID(mediaHdl, &codecID));
      CHECK(omfmCodecGetName(fileHdl, codecID, NAMESIZE, codecName));
      CHECK(omfmGetAudioInfo(mediaHdl,
			     &editRate,
			     &sampleSize,
			     &numChannels));

      printf("            Audio Format  %s\n", codecName);
      printf("            Edit Rate     %ld/%ld\n", editRate.numerator,
			                                        editRate.denominator);
      printf("            Sample Size   %ld\n", sampleSize);
      printf("            No. Channels  %ld\n", numChannels);

      CHECK(omfmGetSampleCount(mediaHdl, &numSamples));
	  CHECK(omfsInt64ToString(numSamples, 10, sizeof(prtBuf), prtBuf));
      printf("            No. Samples   %s\n", prtBuf);
    }
  XEXCEPT
    {
    }
  XEND;

  return(OM_ERR_NONE);
}

static omfErr_t PrintMediaLocation(omfHdl_t fileHdl,
				   omfMobObj_t mob,
				   omfTrackID_t trackID,
				   omfMediaHdl_t mediaHdl)
{
  omfUID_t fileMobID;
  omfInt32 numLocators, loop;
  omfIterHdl_t locIter = NULL;
  omfObject_t locator;
  omfClassID_t	loctype;
  omfMobObj_t fileMob = NULL;
  char locName[NAMESIZE];
  omfPosition_t offset, segPos, dataPos, zero;
  omfBool	isContig;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(fileHdl)
    {
      /* Find the file mob associated with the master mob */

      omfsCvtInt32toPosition(0, offset);
      CHECK(omfiMobFindSource(fileHdl, mob, trackID, offset, kFileMob,
			   NULL, NULL, NULL, NULL, NULL, &fileMob));
      CHECK(omfiMobGetMobID(fileHdl, fileMob, &fileMobID));

      CHECK(omfmMobGetNumLocators(fileHdl, fileMob, &numLocators));
      if (numLocators)
	{
	  CHECK(omfiIteratorAlloc(fileHdl, &locIter));
	  for (loop = 1; loop <= numLocators; loop++)
	    {
 	      CHECK(omfmMobGetNextLocator(locIter, fileMob, &locator));
	      omfError = omfmLocatorGetInfo(fileHdl, locator, loctype,
					    NAMESIZE, locName);
	      if (omfError == OM_ERR_INTERNAL_UNKNOWN_LOC)
		printf("            Unknown Locator Type\n");
	      else if (omfError != OM_ERR_NONE)
		printf("            ERROR retrieving locator\n");
	      else
		printf("            %.4s Locator  %s\n", loctype,locName);
	    }
	  CHECK(omfiIteratorDispose(fileHdl, locIter));
	  locIter = NULL;
	}
    else if (omfmIsMediaDataPresent(fileHdl, fileMobID, kOmfiMedia))
    {
		printf("            Media Locator Internal\n");
		CHECK(omfmIsMediaContiguous(mediaHdl, &isContig));
		if(!isContig)
		{
			printf("            ***Some media is not contiguous within the file.***\n");
			printf("            This could cause problems with some applications.\n");
			CHECK(omfmGetMediaSegmentInfo(mediaHdl, 1, &segPos, NULL, &dataPos, NULL));
			omfsCvtInt32toInt64(0, &zero);
			if(omfsInt64Equal(dataPos, zero) && omfsInt64NotEqual(dataPos, segPos))
			{
				printf("            ***In addition, the data header is fragmented.***\n");
				printf("            This could also cause additional problems with some applications.\n");
			}
		}
	}
    else
		printf("            Media Locator NOT FOUND\n");
    }
  XEXCEPT
    {
      if (locIter)
	omfiIteratorDispose(fileHdl, locIter);
    }
  XEND;

  return(OM_ERR_NONE);
}

static omfErr_t PrintTapeMobInfo(omfHdl_t fileHdl,
				 omfMobObj_t mob)
{
  omfTapeCaseType_t formFactor;
  omfVideoSignalType_t videoSignal;
  omfTapeFormatType_t tapeFormat;
  omfLength_t length;
  char prtBuf[64];
  char manufacturer[NAMESIZE];
  char model[NAMESIZE];

  XPROTECT(fileHdl)
    {
      CHECK(omfmTapeMobGetDescriptor(fileHdl, mob,
				     &formFactor,
				     &videoSignal,
				     &tapeFormat,
				     &length,
				     NAMESIZE,
				     manufacturer,
				     NAMESIZE,
				     model));
      switch (formFactor)
	{
	case kTapeCaseNull:
	  printf("    Form Factor: TapeCaseNull\n");
	  break;
	case kThreeFourthInchVideoTape:
	  printf("    Form Factor: ThreeFourthInchVideoTape\n");
	  break;
	case kVHSVideoTape:
	  printf("    Form Factor: VHSVideoTape\n");
	  break;
	case k8mmVideoTape:
	  printf("    Form Factor: 8mmVideoTape\n");
	  break;
	case kBetacamVideoTape:
	  printf("    Form Factor: BetacamVideoTape\n");
	  break;
	case kCompactCassette:
	  printf("    Form Factor: CompactCasette\n");
	  break;
	case kDATCartridge:
	  printf("    Form Factor: DATCartridge\n");
	  break;
	case kNagraAudioTape:
	  printf("    Form Factor: NagraAudioTape\n");
	  break;
	}

      switch (videoSignal)
	{
	case kVideoSignalNull:
	  printf("    Video Signal: VideoSignalNull\n");
	  break;
	case kNTSCSignal:
	  printf("    Video Signal: NTSCSignal\n");
	  break;
	case kPALSignal:
	  printf("    Video Signal: PALSignal\n");
	  break;
	case kSECAMSignal:
	  printf("    Video Signal: SECAMSignal\n");
	  break;
	}

      switch (tapeFormat)
	{
	case kTapeFormatNull:
	  printf("    Tape Format: BetacamFormat\n");
	  break;
	case kBetacamSPFormat:
	  printf("    Tape Format: BetacamSPFormat\n");
	  break;
	case kVHSFormat:
	  printf("    Tape Format: VHSFormat\n");
	  break;
	case kSVHSFormat:
	  printf("    Tape Format: SVHSFormat\n");
	  break;
	case k8mmFormat:
	  printf("    Tape Format: 8mmFormat\n");
	  break;
	case kHi8Format:
	  printf("    Tape Format: Hi8Format\n");
	  break;
	}

	  CHECK(omfsInt64ToString(length, 10, sizeof(prtBuf), prtBuf));
      printf("    Length: %s\n", prtBuf);
      printf("    Manufacturer: %s\n", manufacturer);
      printf("    Model: %s\n", model);
    }
  XEXCEPT
    {
    }
  XEND;

  return(OM_ERR_NONE);
}

static omfErr_t PrintFilmMobInfo(omfHdl_t fileHdl,
				 omfMobObj_t mob)

{
  omfFilmType_t filmFormat;
  omfUInt32 frameRate;
  omfUInt8 perfPerFrame;
  omfRational_t aspectRatio;
  char manufacturer[NAMESIZE];
  char model[NAMESIZE];

  XPROTECT(fileHdl)
    {
      CHECK(omfmFilmMobGetDescriptor(fileHdl, mob,
				     &filmFormat,
				     &frameRate,
				     &perfPerFrame,
				     &aspectRatio,
				     NAMESIZE,
				     manufacturer,
				     NAMESIZE,
				     model));

      /* Film Format */
      switch (filmFormat)
	{
	case kFtNull:
	  printf("    Film Type: kFtNull\n");
	  break;
	case kThreeFourthInchVideoTape:
	  printf("    Film Type: kFt35MM\n");
	  break;
	case kVHSVideoTape:
	  printf("    Film Type: kFt16MM\n");
	  break;
	case k8mmVideoTape:
	  printf("    Film Type: kFt8MM\n");
	  break;
	case kBetacamVideoTape:
	  printf("    Film Type: kFt65MM\n");
	  break;
	}
      printf("    Frame Rate: %d\n", frameRate);
      printf("    Perfs Per Frame: %d\n", perfPerFrame);
      printf("    Aspect Ratio: %d/%d\n", aspectRatio.numerator,
	                                  aspectRatio.denominator);
      printf("    Manufacturer: %s\n", manufacturer);
      printf("    Model: %s\n", model);
    }
  XEXCEPT
    {
    }
  XEND;

  return(OM_ERR_NONE);
}

static omfErr_t PrintPulldownInfo(omfHdl_t fileHdl,
			       omfObject_t pdwn)
{
	omfUInt32			maskbits;
	omfInt16			phaseOffset;
	omfBool				isDouble;
	omfPulldownKind_t	pulldownKind;
	omfPhaseFrame_t		phaseFrame;
	omfPulldownDir_t	direction;
	omfErr_t			status;

	XPROTECT(fileHdl)
	{
      	printf("            Found Pulldown:\n");
		if ((fileHdl->setrev == kOmfRev1x) || (fileHdl->setrev == kOmfRevIMA))
		{
			omfsIsTypeOf(fileHdl, pdwn, "PDWN", &status);	/* !!!Assert this */

			CHECK(omfsReadUInt32(fileHdl, pdwn, OMMASKMaskBits,
												&maskbits));
      		printf("                Mask = %0lx\n", maskbits);
			CHECK(omfsReadInt16(fileHdl, pdwn, OMWARPPhaseOffset, &phaseOffset));
      		printf("                PhaseOffset = %d\n", phaseOffset);
			CHECK(omfsReadBoolean(fileHdl, pdwn, OMMASKIsDouble, &isDouble));
      		if(isDouble)
      			printf("                Tape => Film\n");
      		else
      			printf("                Film => Tape\n");
		}
		else
		{
			CHECK(omfsReadPulldownKindType(fileHdl, pdwn, OMPDWNPulldownKind, &pulldownKind));
			CHECK(omfsReadPhaseFrameType(fileHdl, pdwn, OMPDWNPhaseFrame, &phaseFrame));
			CHECK(omfsReadPulldownDirectionType(fileHdl, pdwn, OMPDWNDirection, &direction));
      		if(pulldownKind == kOMFTwoThreePD)
      			printf("                3-2 Pulldown\n");
      		else if(pulldownKind == kOMFPALPD)
      			printf("                24-1 Pulldown\n");
      		else if(pulldownKind == kOMFOneToOneNTSC)
      			printf("                1-1 Mapping (NTSC)\n");
      		else if(pulldownKind == kOMFOneToOnePAL)
      			printf("                1-1 Mapping (PAL)\n");
      		else
      			printf("                Unspecified (%d)\n", pulldownKind);
      		printf("                PhaseOffset = %ld\n", phaseFrame);
      		if(direction == kOMFTapeToFilmSpeed)
      			printf("                Tape => Film\n");
      		else
      			printf("                Film => Tape\n");
		}
    }
	XEXCEPT
    {
    }
	XEND;

	return(OM_ERR_NONE);
}

/*********/
/* USAGE */
/*********/
static void usage(char *cmdName,
		  omfProductVersion_t toolkitVers,
		  omfInt32 appVers)
{
  fprintf(stderr, "%s %ld.%ld.%ldr%ld\n", cmdName,
	  toolkitVers.major, toolkitVers.minor, toolkitVers.tertiary,
	  appVers);
  fprintf(stderr, "Usage: %s [-qtab] [-fs <count>] <filename>\n", cmdName);
  fprintf(stderr, "%s prints general info on an OMFI file\n", cmdName);
  fprintf(stderr, "-qtab means to print the Q tables (if any)\n", cmdName);
  fprintf(stderr, "-fs <count> means to print <count> frame sizes\n", cmdName);
}

/***************/
/* printHeader */
/****************/
static void printHeader(char *name,
			omfProductVersion_t toolkitVers,
			omfInt32 appVers)
{
  fprintf(stderr, "%s %ld.%ld.%ldr%ld\n\n", name,
	  toolkitVers.major, toolkitVers.minor, toolkitVers.tertiary,
	  appVers);
}

static omfHdl_t locatorFailure(omfHdl_t file, omfObject_t mdes)
{
  char fname[256];
  omfHdl_t fileHdl = NULL;
  omfBool isOMFI = TRUE;
  omfErr_t omfError = OM_ERR_NONE;

  printf("Media not found... please enter filename with media: ");
  scanf("%s", fname);
  printf("\n");

  omfError = omfsReadBoolean(file, mdes, OMMDFLIsOMFI, &isOMFI);
  if (isOMFI)
  {
	omfError = omfsOpenFile(fname, file->session, &fileHdl);
  }
  else
  {
	omfError = omfsOpenRawFile(fname, strlen(fname)+1,
							   file->session, &fileHdl);
  }
  if (omfError == OM_ERR_NONE)
	return (fileHdl);
  else
	return(NULL);
}


/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
