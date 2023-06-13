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
 * Name: omf2edl.c
 *
 * Function: Print out an edl-like dump from every composition in a given file.
 *
 * 			Currently only cuts-only!
 *
 *			An example of:
 *				Computin source & record timecodes.
 *				Finding tape names.
 *				Using omfsFindSource() to iterate over a track.
 *				Rretrieving iterator information.
 *				Converting edits to frame boundries.
 *
 */


#include "masterhd.h"
#if defined(THINK_C) || defined(__MWERKS__)
#include <console.h>
#include <StandardFile.h>
#include "MacSupport.h"
#endif
#include <stdio.h>
#include <string.h>
#include "omPublic.h"
#include "omMedia.h"

#include <math.h>

#define ExitIfNull(b, m) \
  if (!b) { \
      printf("%s: %s\n", cmdname, m); \
      return(-1); \
   }

static omfErr_t FormatTimecode(omfHdl_t file,
				   omfMobObj_t mob,
				   omfTrackID_t trackID,
				   omfPosition_t startPos,
				   omfLength_t length,
                   omfRational_t srcRate,
                   omfRational_t destRate,
				   omfInt16 startBufSize,
				   char *startTC,
				   omfInt16 endBufSize,
				   char *endTC);

static omfErr_t PrintTimecode(
							  omfHdl_t file,
							  omfMobObj_t mob,
							  omfObject_t object,
							  omfBool isTransition,
							  char *tapeName,
							  char *srcStartTC, 
							  char *srcEndTC, 
							  char *destStartTC, 
							  char *destEndTC);

static omfErr_t traverseMob(omfHdl_t fileHdl, omfMobObj_t compMob);

omfFileRev_t fileRev;

/*************************************************************************
 * Function: omf2edl()/Main()
 *
 * 	The main function of the module.  Iterates over all of the composition
 *		mobs in the file and calls traverseMob() for each.
 *
 */
int main(int argc, char *argv[])
{
	omfSessionHdl_t		session;
	omfHdl_t			fileHdl;
	omfMobObj_t			compMob;
	omfInt32			loop, numMobs;
	omfIterHdl_t		mobIter = NULL;
	omfErr_t			omfError = OM_ERR_NONE;
	omfSearchCrit_t		searchCrit;
	char        		*fname, *printfname;
	char 				*cmdname;
	omfUsageCode_t		usageCode;
	char 				errBuf[256];
	omfInt64			one;
#if PORT_SYS_MAC
	char            	namebuf[255];
   Point where;
	SFReply         	fs;
	SFTypeList      	fTypes;
#else
	int             	filearg;
#endif
	
	XPROTECT(NULL)
	{
#if PORT_SYS_MAC
		MacInit();
		where.h = 20;
		where.v = 40; 
#else
		if (argc < 2)
		{
			printf("*** ERROR - missing file name\n");
			return(1);
		}
#endif

		cmdname = argv[0];

		omfsCvtInt32toInt64(1, &one);
		CHECK(omfsBeginSession(NULL, &session));
		CHECK(omfmInit(session));

		/* BEGINNING OF WHILE LOOP */
#if defined(THINK_C) || defined(__MWERKS__)
		while (1)		/* Until the user chooses quit from the menu */
#else				
		for(filearg = 1; filearg < argc; filearg++)
#endif
		{
#if defined(THINK_C) || defined(__MWERKS__)
			/* Get the file name and open */
			fs.fName[0] = '\0';
			fTypes[0] = 'OMFI';
			SFGetFile(where, NULL, NULL, -1, fTypes, NULL, &fs);
			
			/* If the name is empty, exit */
			if (fs.fName[0] == '\0')
				break;
			if(!fs.good)
				break;
			
			fname = (char *) &fs;
			strncpy(namebuf, (char *) &(fs.fName[1]), (size_t) fs.fName[0]);
			namebuf[fs.fName[0]] = 0;
			printfname = namebuf;
#else				/* Not Macintosh */
			fname = argv[filearg];
			printfname = fname;
#endif

			printf("\n***** omf2edl: %s *****\n", printfname);
			CHECK(omfsOpenFile((fileHandleType)fname, session, &fileHdl));
			CHECK(omfsFileGetRev(fileHdl, &fileRev));
			
			/* Make sure tha tape mob is in the file, for source timecode */
			CHECK(omfiGetNumMobs(fileHdl, kTapeMob, &numMobs));
			if (numMobs == 0)
			{
				printf("No tape mobs were found in the OMF file (no source timecode)\n");
				CHECK(omfsCloseFile(fileHdl));
				continue;
			}
			
			/* Iterate over all composition mobs in the file */
			CHECK(omfiGetNumMobs(fileHdl, kCompMob, &numMobs));
			if (numMobs == 0)
			{
				printf("No composition mobs were found in the OMF file\n");
				CHECK(omfsCloseFile(fileHdl));
				continue;
			}

			/************************/
			/* Get Composition Mobs */
			/************************/
			CHECK(omfiIteratorAlloc(fileHdl, &mobIter));

	    /* Iterate over the composition mobs, if there are any */
			for (loop = 0; loop < numMobs; loop++)
			{
				searchCrit.searchTag = kByMobKind;
				searchCrit.tags.mobKind = kCompMob;
				CHECK(omfiGetNextMob(mobIter, &searchCrit, &compMob));
			 
				if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
        		{
                	usageCode = 0;
	       			omfError = omfsReadUsageCodeType(fileHdl,compMob,  OMMOBJUsageCode,
                                  &usageCode);
					if ((omfError != OM_ERR_NONE) || usageCode != UC_NULL)
				  		continue;
                }
  				CHECK(traverseMob(fileHdl, compMob))
              	
			} /* Mob iterator */
			
			omfiIteratorDispose(fileHdl, mobIter);
			mobIter = NULL;

			CHECK(omfsCloseFile(fileHdl));
		}
		CHECK(omfsEndSession(session));
		printf("omf2edl completed successfully.\n");
	} /* XPROTECT */

	XEXCEPT
	{	  
		if (mobIter)
			omfiIteratorDispose(fileHdl, mobIter);
		printf("***ERROR: %d: %s\n", XCODE(), 
				omfsGetExpandedErrorString(fileHdl, XCODE(), 
													sizeof(errBuf), errBuf));
		return(-1);
	}
	XEND;

	return(0);
}

static omfErr_t FormatTimecode(omfHdl_t file,
				   omfMobObj_t mob,
				   omfTrackID_t trackID,
				   omfPosition_t startPos,
				   omfLength_t length,
                   omfRational_t srcRate,
                   omfRational_t destRate,
				   omfInt16 startBufSize,
				   char *startTC,
				   omfInt16 endBufSize,
				   char *endTC)
{
  omfTimecode_t timecode;
  omfPosition_t endPos;
  omfLength_t convertLen;

  XPROTECT(file)
	{
	  endPos = startPos;						
	  if (srcRate.numerator != 0)
		{
		  CHECK(omfiConvertEditRate(srcRate,
									length,
									destRate,
									kRoundCeiling,
									&convertLen));
		}
	  else
		convertLen = length;
	  CHECK(omfsAddInt64toInt64(convertLen, &endPos));

	  CHECK(omfiOffsetToMobTimecode(file, mob, NULL, startPos, &timecode));
	  CHECK(omfsTimecodeToString(timecode, startBufSize, startTC));

	  /* Now get timecode for the end of the clip */
	  CHECK(omfiOffsetToMobTimecode(file, mob, NULL, endPos, &timecode));
	  CHECK(omfsTimecodeToString(timecode, endBufSize, endTC));

	} /* XPROTECT */

  XEXCEPT
	{
	}
  XEND;

  return(OM_ERR_NONE);
}

static omfErr_t PrintTimecode(
							  omfHdl_t file,
							  omfMobObj_t mob,
							  omfObject_t object,
							  omfBool isTransition,
							  char *tapeName,
							  char *srcStartTC, 
							  char *srcEndTC, 
							  char *destStartTC, 
							  char *destEndTC)
{
  char	eventType[8];
  char	tranName[128];
  omfErr_t omfError = OM_ERR_NONE;
  omfObject_t	effect, effDefine;
	omfFileRev_t	rev;
	
  XPROTECT(file)
	{

	  if (isTransition)
	  {
		CHECK(omfsFileGetRev(file, &rev));
		if ((rev == kOmfRev1x) || (rev == kOmfRevIMA))
		{
			CHECK(omfsReadString(file, object, OMCPNTEffectID, tranName, sizeof(tranName)));
			if(strcmp(tranName, "Blend:Dissolve") == 0)
				strcpy(eventType, "D");
			else
				strcpy(eventType, "T");
		}	
	  	else
	  	{
	  		if(omfsIsTypeOf(file, object, "TRAN", &omfError))
	  		{
				CHECK(omfsReadObjRef(file, object, OMTRANEffect, &effect));
				CHECK(omfsReadObjRef(file, effect, OMEFFEEffectKind, &effDefine));
				CHECK(omfsReadUniqueName(file, effDefine, OMEDEFEffectID, tranName, sizeof(tranName)));
				if(strcmp(tranName, "omfi:effect:SimpleVideoDissolve") == 0)
					strcpy(eventType, "D");
				else if(strcmp(tranName, "omfi:effect:SimpleMonoAudioDissolve") == 0)
					strcpy(eventType, "D");
				else
					strcpy(eventType, "T");
			}
			else
				strcpy(eventType, "T");
	  	}
	  }
	  else
		strcpy(eventType, "C");

	  printf("%s  %s %s   %s %s  '%s'\n",
			 eventType,
			 srcStartTC, srcEndTC,
			 destStartTC, destEndTC,
			 tapeName);
	} /* XPROTECT */

  XEXCEPT
	{
	}
  XEND;

  return(OM_ERR_NONE);
}
  
/************************
 * Function: traverseMob
 *
 * Traverse the tracks of a composition, finding the timecode track & video
 *	rate, and calling traverseMediaTrack() for each media track.
 *
 * Argument Notes:
 *		compEditRate - The frame rate of the composition, used to 
 *                   compute timecodes.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions, accessor, and Bento errors.
 *		See top of the file.
 */
static omfErr_t traverseMob(omfHdl_t fileHdl, omfMobObj_t compMob)
{
	char 				errBuf[256];
	omfBool				prevTransition = FALSE, foundOutgoing = FALSE;
	omfBool				foundTransition = FALSE;
	omfErr_t			omfError = OM_ERR_NONE, srcError = OM_ERR_NONE;
	omfObject_t			trackSeg = NULL;
	omfMobFindHdl_t		mobFindHdl = NULL;
	omfObject_t			currentCpnt = NULL;
	omfDDefObj_t		pictureKind = NULL, soundKind = NULL, datakind = NULL;
	omfInt32			nameSize = 32;
	omfMobObj_t			track = NULL;
	omfIterHdl_t		trackIter = NULL;
	omfString			name = NULL;
	omfInt32			numTracks, trackLoop;
	omfTrackID_t		trackID, printTrackID;
	omfInt16			tmp1xTrackID;
	char				srcStartTC[32], srcEndTC[32], destStartTC[32], destEndTC[32];
	omfFindSourceInfo_t	foundSource;
	omfEffectChoice_t	effectChoice;
	omfPosition_t		currentPos;
	char				tapeNameBuf[256];
	omfLength_t			srcLen;
	omfRational_t		trackRate, srcRate;

    XPROTECT(fileHdl)
    {
		omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureKind, &omfError);
		omfiDatakindLookup(fileHdl, SOUNDKIND, &soundKind, &omfError);

		name = (omfString)omfsMalloc((size_t) nameSize);
		if(name == NULL)
			RAISE(OM_ERR_NOMEMORY);
			
		CHECK(omfiMobGetInfo(fileHdl, compMob, NULL, nameSize, name, 
									 NULL, NULL));
		printf("MobName: %s\n", name);
		omfsFree(name);
		 

		CHECK(omfiMobGetNumTracks(fileHdl, compMob, &numTracks));

		/* Iterate over tracks in the mob */
		CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
		for (trackLoop = 0; trackLoop < numTracks; trackLoop++)
		{
			CHECK(omfiMobGetNextTrack(trackIter, compMob, NULL, &track));
			CHECK(omfiTrackGetInfo(fileHdl, compMob, track, 
							   &trackRate, 0, NULL, NULL, 
							   &trackID, &trackSeg));
			if (fileRev == kOmfRev1x)
			{
				CHECK(omfsReadInt16(fileHdl, track, OMTRAKLabelNumber,
								(omfInt16 *)&tmp1xTrackID));
				printTrackID = (omfTrackID_t)tmp1xTrackID;
		  	}
			else
				printTrackID = trackID;

			CHECK(omfiComponentGetInfo(fileHdl, trackSeg, &datakind,NULL));
			if ((datakind == pictureKind) || (datakind == soundKind))
			{
				if (datakind == pictureKind)
					printf("PICTURE TRACK: %d\n", printTrackID);
				else if (datakind == soundKind)
					printf("SOUND TRACK: %d\n", printTrackID);

				CHECK(omfiMobOpenSearch(fileHdl, compMob, track, &mobFindHdl));


				/*** GET FIRST SOURCE ***/
				effectChoice = kFindIncoming; 
				srcError = omfiMobGetNextSource(mobFindHdl, 
												kTapeMob,
												NULL,
												&effectChoice,
												&currentCpnt,
												&currentPos,
												&foundSource,
												&foundTransition);
				srcLen = foundSource.minLength;
				srcRate = foundSource.editrate;
				while (srcError != OM_ERR_NO_MORE_OBJECTS)
				{
					strcpy(srcStartTC, "00:00:00:00");
					strcpy(srcEndTC, "00:00:00:00");
					strcpy(destStartTC, "00:00:00:00");
					strcpy(destEndTC, "00:00:00:00");

					if(srcError == OM_ERR_PARSE_EFFECT_AMBIGUOUS)
					{
						effectChoice = kFindRender;
						srcError = omfiMobGetThisSource(mobFindHdl, 
														kTapeMob,
														NULL,
														&effectChoice,
														&currentCpnt,
														&currentPos,
														&foundSource,
														&foundTransition);
						srcLen = foundSource.minLength;
						srcRate = foundSource.editrate;
						effectChoice = kFindIncoming; /* Reset */
					}
					/* If current object is a transition, remember 
					 * for next iteration.  
					 */
#if 0
					if (!foundOutgoing && currentCpnt && 
						omfiIsATransition(fileHdl, currentCpnt, &omfError))
#endif
					if (!foundOutgoing && foundTransition)
					{
						prevTransition = TRUE;
						/* Get the length of the transition */
						if (srcError != OM_ERR_FILL_FOUND)
						{
							CHECK(omfiComponentGetInfo(fileHdl, currentCpnt, 
												 NULL, &srcLen));
						}
						srcRate = trackRate;
					}
					else
						foundOutgoing = FALSE;

					if (srcError == OM_ERR_FILL_FOUND)
					{
						strcpy(tapeNameBuf, "FILL FOUND");
					}
					else if (srcError != OM_ERR_NONE)
					{
						printf("***ERROR: %s\n", 
						   omfsGetExpandedErrorString(fileHdl, 
								  srcError, sizeof(errBuf), errBuf));
					}
					else
					{
						/*** Calculate the source timecode ***/
						CHECK(FormatTimecode(fileHdl, foundSource.mob,
								 foundSource.mobTrackID,
								 foundSource.position,
                                 srcLen, srcRate, foundSource.editrate,
								 sizeof(srcStartTC), srcStartTC, 
								 sizeof(srcEndTC), srcEndTC));
					} /* No error from omfiMobGetNextSource */

					/*** Calculate the record timecode ***/
					CHECK(FormatTimecode(fileHdl, 
							   compMob, trackID, currentPos,
							   srcLen, srcRate, trackRate,
							   sizeof(destStartTC), destStartTC,
							   sizeof(destEndTC), destEndTC));

					/*** Print the source and record timecode ***/
					if (foundSource.mob)
					{
						CHECK(omfiMobGetInfo(fileHdl,foundSource.mob, NULL,
										 256, tapeNameBuf, NULL,NULL));
					}
					PrintTimecode(fileHdl, compMob, currentCpnt, 
							  foundTransition, tapeNameBuf,
							  srcStartTC, srcEndTC,
							  destStartTC, destEndTC);

					/*** GET NEXT SOURCE ***/
					/* If transition, process same transition once more 
					 * searching for outgoing clip information.
					 */
					if (prevTransition)
					{
						effectChoice = kFindOutgoing;
						srcError = omfiMobGetThisSource(mobFindHdl, 
														kTapeMob,
														NULL,
														&effectChoice,
														&currentCpnt,
														&currentPos,
														&foundSource,
														&foundTransition);
						/* For outgoing transition media, get the length
						 * of the outgoing segment.
						 */
						prevTransition = FALSE;
						foundOutgoing = TRUE;
						srcLen = foundSource.minLength;
						srcRate = foundSource.editrate;
						effectChoice = kFindIncoming; /* Reset */
					}
					else
					{
						srcError = omfiMobGetNextSource(mobFindHdl, 
													kTapeMob,
													NULL,
													&effectChoice,
													&currentCpnt,
													&currentPos,
													&foundSource,
													&foundTransition);
						srcLen = foundSource.minLength;
						srcRate = foundSource.editrate;
					}
				}
				CHECK(omfiMobCloseSearch(fileHdl, mobFindHdl));
			} /* Not picture or sound kind */
		} /* Track iterator */
		CHECK(omfiIteratorDispose(fileHdl, trackIter));
		trackIter = NULL;
	} /* XPROTECT */
	XEXCEPT
	{
		if (name)
			omfsFree(name);
		if (trackIter)
			omfiIteratorDispose(fileHdl, trackIter);
	}
	XEND;

	return(OM_ERR_NONE);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
