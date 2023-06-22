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
 * FndSrc - This unittest opens a file (either 1.x or 2.x), looks for
 *          composition mobs, and uses first omfiMobFindSource, and then
 *          omfmOffsetToTimecode (which also calls omfiMobFindSource)
 *          to find the timecode for a specific position in a specific
 *          track.  This unittest will only work on files with tape mobs,
 *          and that have the track and position searched for.  It 
 *          shouldn't be run on any file.
 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
#include "omMobMgt.h"
#include "omMedia.h"
#include "omEffect.h"
#include "UnitTest.h"

#define NUM_MASTER_CLIPS		8

static omfErr_t CreateCompositionMob(
		omfHdl_t    	fileHdl,
		omfInt16		numSrcRef,
		omfSourceRef_t	*vSource,
		omfMobObj_t		*compMobPtr);
		
static omfErr_t CreateBadCompositionMob1(	/* Effect within effect */
		omfHdl_t    	fileHdl,
		omfInt16		numSrcRef,
		omfSourceRef_t	*vSource,
		omfMobObj_t		*compMobPtr);
		
static omfErr_t CreateBadCompositionMob2(	/* Effect within transition */
		omfHdl_t    	fileHdl,
		omfInt16		numSrcRef,
		omfSourceRef_t	*vSource,
		omfMobObj_t		*compMobPtr);
		
static omfErr_t CreateMasterClips(
		omfHdl_t    	fileHdl,
		omfInt16		numSrcRef,
		omfSourceRef_t	*vSource);

static omfErr_t CreateOneMasterClip(
		omfHdl_t    	fileHdl,
		omfObject_t		*master);

typedef struct tstCmpComponent
{
	char				*masterName;
	omfInt32			offset;
	omfInt32			length;
	struct tstCmpEffect	*eff;
} tstCmpComponent_t;

typedef struct tstCmpEffect
{
	char					*effectName;
	int 					numInputs;
	struct tstCmpComponent	inputs[2];
} tstCmpEffect_t;


tstCmpEffect_t seq1eff1 = { kOmfFxVSMPTEWipeGlobalName, 2, { 
							{ "Test Master 3", 0, 10, NULL },
							{ "Test Master 0", 10, 10, NULL }
							} };

tstCmpEffect_t seq1eff2 = { kOmfFxSpeedControlGlobalName, 1, {
							{ "Test Master 1", 0, 10, NULL }
							} };

tstCmpComponent_t sequence1[4] = {
	{ "Test Master 0", 0, 10, NULL },
	{ NULL, 0, 10, &seq1eff1 },
	{ NULL, 0, 10, &seq1eff2 },
	{ "Test Master 2", 0, 10, NULL }
	};

/*
 * The following composition structure is created for testing scopes using FindSource.
 *
 *								20				    10        10
 *	    	:--------:   :---------------------:----------:----------:
 * 	    	|        | 1 |   Master Mob0       | Nest/    | Master   |
 * 	  		|        |   |                     | EFFE     | Mob 2    |
 * 	    	|        |   |                     | SREF 0,1 |          |
 * 	  		|        |   |                     | Mob 1    |          |
 * Track 1	| NEST   |   :---------------------:----------:----------:
 * 	    	|        |   :----------:----------:---------------------:
 * 	    	|        | 2 | SREF 0,1 | Nest/    | SREF 0,1            |
 * 	    	|        |   |          | EFFE     |                     |
 * 	    	|        |   |          | SREF 1,1 |                     |
 * 	    	|        |   |          | Mob 3    |                     |
 * 	    	:--------:   :----------:----------:---------------------:
 *			                  10         10            20
 */

#ifdef MAKE_TEST_HARNESS
int TestCompIter(char *destName)
{
    int	argc;
    char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t		session;
    omfHdl_t 			fileHdl = NULL;
    omfMobObj_t 		compMob;
    omfSegObj_t 		trackSeg;
    omfInt32			loop, numTracks, effTracks, sub, cmpElem, numErrors;
    omfIterHdl_t		trackIter = NULL;
	omfErr_t			omfError = OM_ERR_NONE, srcError = OM_ERR_NONE;
    omfBool				foundMob = FALSE;
	char				lenBuf[32], *elemName;
	omfMobObj_t			track;
	omfTrackID_t		trackID;
	omfFindSourceInfo_t	foundSource;
	omfEffectChoice_t	effectChoice;
	omfPosition_t		currentPos, elemOffset;
	omfObject_t			currentCpnt = NULL, effDefine;
	omfMobFindHdl_t		mobFindHdl = NULL;
	omfBool				prevTransition = FALSE, foundOutgoing = FALSE;
	omfBool				foundTransition = FALSE, inEffect = FALSE;
	omfLength_t			srcLen, elemLen;
	omfRational_t		srcRate;
	char				tapeNameBuf[256], masterNameBuf[64], objClass[5];
	char				compositionName[64], fileMobName[64];
	char 				errBuf[256], tranName[128],recPosBuf[32];
#if DEBUG
	char				srcPosBuf[32];
#endif
	omfSourceRef_t		vSource[NUM_MASTER_CLIPS];
	
	omfProductIdentification_t ProductInfo;

	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "FindSource UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;
	
	XPROTECT(NULL)
	{
#ifdef MAKE_TEST_HARNESS
		argc = 2;
		argv[1] = destName;
#else
#if PORT_MAC_HAS_CCOMMAND
		argc = ccommand(&argv); 
#endif
#endif

		/*
		 *	Standard Toolbox stuff
		 */
		 
		if (argc < 2)
		  {
		    printf("*** ERROR - missing file name\n");
		    exit(1);
		  }

		CHECK(omfsBeginSession(&ProductInfo, &session));
		CHECK(omfmInit(session));
	    CHECK(omfsCreateFile((fileHandleType) argv[1], session, 
				 kOmfRev2x, &fileHdl));

		CHECK(CreateMasterClips(fileHdl, NUM_MASTER_CLIPS, vSource));

		CHECK(CreateCompositionMob(fileHdl, NUM_MASTER_CLIPS, vSource, &compMob));

		CHECK(omfiMobGetInfo(fileHdl, compMob, NULL, sizeof(compositionName),
							compositionName, NULL, NULL));
		numErrors = 0;
		printf("Validating composition '%s'\n", compositionName);
		CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
		CHECK(omfiMobGetNumTracks(fileHdl, compMob, &numTracks));
		for (loop = 1; loop <= numTracks; loop++)
		{
			CHECK(omfiMobGetNextTrack(trackIter, compMob, NULL, &track));
			CHECK(omfiTrackGetInfo(fileHdl, compMob, track, 
							   NULL, 0, NULL, NULL, 
							   &trackID, &trackSeg));

			CHECK(omfiComponentGetInfo(fileHdl, trackSeg, NULL, NULL));
			if(trackID == 1)
			{
				printf("    Validating video track\n");
				CHECK(omfiMobOpenSearch(fileHdl, compMob, track, &mobFindHdl));


				cmpElem = 0;
				/*** GET FIRST SOURCE ***/
				effectChoice = kFindIncoming; 
				srcError = omfiMobGetNextSource(mobFindHdl, 
												kMasterMob,
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
					CHECK(omfsInt64ToString(currentPos, 10, sizeof(recPosBuf), recPosBuf));
#if DEBUG
					printf("***** Position %s *****\n", recPosBuf);					
#endif
					tranName[0] = '\0';
					
					inEffect = (srcError == OM_ERR_PARSE_EFFECT_AMBIGUOUS);
					if(inEffect)
					{
						if(foundSource.effeObject != NULL)
						{
							CHECK(omfsReadClassID(fileHdl, foundSource.effeObject, OMOOBJObjClass, objClass));
							objClass[4] = '\0';
							CHECK(omfsReadObjRef(fileHdl, foundSource.effeObject,
												OMEFFEEffectKind, &effDefine));
							CHECK(omfsReadUniqueName(fileHdl, effDefine, OMEDEFEffectID,
												tranName, sizeof(tranName)));
#if DEBUG
							printf("Hit an '%s' effect:\n", tranName);
#endif
							if(strcmp(tranName, kOmfFxVSMPTEWipeGlobalName) == 0)
								effTracks = 2;		/* For this particular example */
							else
								effTracks = 1;		/* Default when we don't know */
						}
					}
					else
						effTracks = 1;

					for(sub = 1; sub  <= effTracks; sub++)
					{
						if(inEffect)
						{
#if DEBUG
							printf("   [ %ld ] ", sub);
#endif

							if(sub == 1)
								effectChoice = kFindEffectSrc1;
							else if(sub == 2)
								effectChoice = kFindEffectSrc2;
							srcError = omfiMobGetThisSource(mobFindHdl, 
															kMasterMob,
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

						CHECK(omfsInt64ToString(srcLen, 10, sizeof(lenBuf), lenBuf));
						if(foundSource.mob != NULL)
						{
							CHECK(omfiMobGetInfo(fileHdl, foundSource.mob, NULL,
											sizeof(masterNameBuf), masterNameBuf,
											NULL, NULL));
						}
						
#if DEBUG
						CHECK(omfsReadClassID(fileHdl, currentCpnt, OMOOBJObjClass, objClass));
						objClass[4] = '\0';
						CHECK(omfsInt64ToString(foundSource.minLength, 10, sizeof(lenBuf), lenBuf));
						CHECK(omfsInt64ToString(foundSource.position, 10, sizeof(srcPosBuf), srcPosBuf));
						printf("Pos = %s Len = %s mob = '%s', <%s>'\n",
											srcPosBuf, lenBuf, masterNameBuf, objClass);
#endif

						if(inEffect && (sequence1[cmpElem].eff == NULL))
						{
							printf("Missing effect at position %s\n", recPosBuf);
							numErrors++;
						}
						else if(!inEffect && (sequence1[cmpElem].eff != NULL))
						{
							printf("Extra effect at position %s\n", recPosBuf);
							numErrors++;
						}
						else if(inEffect &&(sequence1[cmpElem].eff != NULL))
						{
							if(sub == 1)
							{
								if(strcmp(sequence1[cmpElem].eff->effectName, tranName) != 0)
								{
									printf("Wrong effect at position %s.  Expected '%s' found '%s'\n",
											recPosBuf, sequence1[cmpElem].eff->effectName, tranName);
									numErrors++;
								}
								if(sequence1[cmpElem].eff->numInputs != effTracks)
								{
									printf("Wrong number of effect tracks  at position %s.  Expected %ld found %ld\n",
										recPosBuf, sequence1[cmpElem].eff->numInputs, effTracks);
									numErrors++;
								}
							}
							elemName   = sequence1[cmpElem].eff->inputs[sub-1].masterName;
							omfsCvtInt32toInt64(sequence1[cmpElem].eff->inputs[sub-1].offset, &elemOffset);
							omfsCvtInt32toInt64(sequence1[cmpElem].eff->inputs[sub-1].length, &elemLen);
						}
						else	/* Not in an effect */
						{
							elemName   = sequence1[cmpElem].masterName;
							omfsCvtInt32toInt64(sequence1[cmpElem].offset, &elemOffset);
							omfsCvtInt32toInt64(sequence1[cmpElem].length, &elemLen);
						}

						if(strcmp(elemName, masterNameBuf) != 0)
						{
							printf("Wrong component name at position %s.  Expected %s found %s\n",
									recPosBuf, elemName, masterNameBuf);
							numErrors++;
						}
						if(omfsInt64NotEqual(elemOffset, foundSource.position))
						{
							char	expected[32], actual[32];
							
							CHECK(omfsInt64ToString(elemOffset, 10,
													sizeof(expected), expected));
							CHECK(omfsInt64ToString(foundSource.position, 10, sizeof(actual), actual));
							printf("Wrong source offset at position %s.  Expected %s found %s\n",
									recPosBuf, expected, actual);
							numErrors++;
						}
						if(omfsInt64NotEqual(elemLen, foundSource.minLength))
						{
							char	expected[32], actual[32];
							
							CHECK(omfsInt64ToString(elemLen, 10,
													sizeof(expected), expected));
							CHECK(omfsInt64ToString(foundSource.minLength, 10, sizeof(actual), actual));
							printf("Wrong length at position %s.  Expected %s found %s\n",
									recPosBuf, expected, actual);
							numErrors++;
						}
						

#if 0
	//					/* If current object is a transition, remember 
	//					 * for next iteration.  
	//					 */
	//					if (!foundOutgoing && foundTransition)
	//					{
	//						prevTransition = TRUE;
	//						/* Get the length of the transition */
	//						if (srcError != OM_ERR_FILL_FOUND)
	//						{
	//							CHECK(omfiComponentGetInfo(fileHdl, currentCpnt, 
	//												 NULL, &srcLen));
	//						}
	///						srcRate = trackRate;
	//					}
	//					else
#endif
							foundOutgoing = FALSE;

						if (srcError == OM_ERR_FILL_FOUND)
						{
							strcpy( (char*) tapeNameBuf, "FILL FOUND");
						}
						else if (srcError != OM_ERR_NONE)
						{
							printf("***ERROR: %s\n", 
							   omfsGetExpandedErrorString(fileHdl, 
									  srcError, sizeof(errBuf), errBuf));
						}
						/* else No error from omfiMobGetNextSource */
						/* Investigate how to fit transition code in here later!!! */
					}
										
					cmpElem++;
					/*** GET NEXT SOURCE ***/
					/* If transition, process same transition once more 
					 * searching for outgoing clip information.
					 */
#if 0
					if (prevTransition)
					{
						effectChoice = kFindOutgoing;
						srcError = omfiMobGetThisSource(mobFindHdl, 
														kMasterMob,
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
#endif
					{
						srcError = omfiMobGetNextSource(mobFindHdl, 
													kMasterMob,
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
			}
		}
		CHECK(omfiIteratorDispose(fileHdl, trackIter));
		trackIter = NULL;
	
		/****************************************************************/
		/****** Create nested effects and check for expected error ******/
		/****************************************************************/
		CHECK(CreateBadCompositionMob1(fileHdl, NUM_MASTER_CLIPS, vSource, &compMob));	/* Effect within effect */
		CHECK(omfiMobGetInfo(fileHdl, compMob, NULL, sizeof(compositionName),
							compositionName, NULL, NULL));
		printf("Validating composition '%s'\n", compositionName);
		CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
		CHECK(omfiMobGetNumTracks(fileHdl, compMob, &numTracks));
		for (loop = 1; loop <= numTracks; loop++)
		{
			CHECK(omfiMobGetNextTrack(trackIter, compMob, NULL, &track));
			CHECK(omfiTrackGetInfo(fileHdl, compMob, track, 
							   NULL, 0, NULL, NULL, 
							   &trackID, &trackSeg));

			CHECK(omfiComponentGetInfo(fileHdl, trackSeg, NULL, NULL));
			if(trackID == 1)
			{
				printf("    Validating video track\n");
				CHECK(omfiMobOpenSearch(fileHdl, compMob, track, &mobFindHdl));
				effectChoice = kFindIncoming;
				srcError = omfiMobGetNextSource(mobFindHdl, 
											kMasterMob,
											NULL,
											&effectChoice,
											&currentCpnt,
											&currentPos,
											&foundSource,
											&foundTransition);
				if(srcError != OM_ERR_PARSE_EFFECT_AMBIGUOUS)
				{
					printf("Missed expected error (next source)\n");
					numErrors++;
				}
				effectChoice = kFindEffectSrc1;
				srcError = omfiMobGetThisSource(mobFindHdl, 
												kMasterMob,
												NULL,
												&effectChoice,
												&currentCpnt,
												&currentPos,
												&foundSource,
												&foundTransition);
				if(srcError != OM_ERR_PARSE_EFFECT_AMBIGUOUS)
				{
					printf("Missed expected error (this source)\n");
					numErrors++;
				}
				CHECK(omfiMobCloseSearch(fileHdl, mobFindHdl));
			}
		}
		CHECK(omfiIteratorDispose(fileHdl, trackIter));
		trackIter = NULL;
					
		/****************************************************************/
		/****** Create effects within a transition                 ******/
		/****** and check for expected error                       ******/
		/****************************************************************/
		CHECK(CreateBadCompositionMob2(fileHdl, NUM_MASTER_CLIPS, vSource, &compMob));	/* Effect within effect */
		CHECK(omfiMobGetInfo(fileHdl, compMob, NULL, sizeof(compositionName),
							compositionName, NULL, NULL));
		printf("Validating composition '%s'\n", compositionName);
		CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
		CHECK(omfiMobGetNumTracks(fileHdl, compMob, &numTracks));
		for (loop = 1; loop <= numTracks; loop++)
		{
			CHECK(omfiMobGetNextTrack(trackIter, compMob, NULL, &track));
			CHECK(omfiTrackGetInfo(fileHdl, compMob, track, 
							   NULL, 0, NULL, NULL, 
							   &trackID, &trackSeg));

			CHECK(omfiComponentGetInfo(fileHdl, trackSeg, NULL, NULL));
			if(trackID == 1)
			{
				printf("    Validating video track\n");
				CHECK(omfiMobOpenSearch(fileHdl, compMob, track, &mobFindHdl));
				/**** First two elements souuld be good (source 0) ****/
				effectChoice = kFindIncoming;
				srcError = omfiMobGetNextSource(mobFindHdl, 
										kMasterMob,
										NULL,
										&effectChoice,
										&currentCpnt,
										&currentPos,
										&foundSource,
										&foundTransition);
				if(srcError != OM_ERR_NONE)
				{
					printf("Problem reading comp1\n");
					numErrors++;
				}
				effectChoice = kFindIncoming;
				srcError = omfiMobGetNextSource(mobFindHdl, 
										kMasterMob,
										NULL,
										&effectChoice,
										&currentCpnt,
										&currentPos,
										&foundSource,
										&foundTransition);
				if(!foundTransition)
				{
					printf("Second element should be a transition\n");
					numErrors++;
				}
				if(srcError != OM_ERR_NONE)
				{
					printf("Problem reading comp1\n");
					numErrors++;
				}
				/**** Third element should fail (effect within transition) ****/
				effectChoice = kFindOutgoing;
				srcError = omfiMobGetThisSource(mobFindHdl, 
											kMasterMob,
											NULL,
											&effectChoice,
											&currentCpnt,
											&currentPos,
											&foundSource,
											&foundTransition);
				if(srcError != OM_ERR_PARSE_EFFECT_AMBIGUOUS)
				{
					printf("Missed expected error (next source)\n");
					numErrors++;
				}
				CHECK(omfiMobCloseSearch(fileHdl, mobFindHdl));
			}
		}
		CHECK(omfiIteratorDispose(fileHdl, trackIter));
		trackIter = NULL;
					
		/****************************************************************/
		/******               Try parsing a master clip            ******/
		/****************************************************************/
		CHECK(CreateOneMasterClip(fileHdl, &compMob));
		CHECK(omfiMobGetInfo(fileHdl, compMob, NULL, sizeof(compositionName),
							compositionName, NULL, NULL));
		printf("Validating composition '%s'\n", compositionName);
		CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
		CHECK(omfiMobGetNumTracks(fileHdl, compMob, &numTracks));
		for (loop = 1; loop <= numTracks; loop++)
		{
			CHECK(omfiMobGetNextTrack(trackIter, compMob, NULL, &track));
			CHECK(omfiTrackGetInfo(fileHdl, compMob, track, 
							   NULL, 0, NULL, NULL, 
							   &trackID, &trackSeg));

			CHECK(omfiComponentGetInfo(fileHdl, trackSeg, NULL, NULL));
			if(trackID == 1)
			{
				printf("    Validating video track\n");
				CHECK(omfiMobOpenSearch(fileHdl, compMob, track, &mobFindHdl));
				effectChoice = kFindIncoming;
				srcError = omfiMobGetNextSource(mobFindHdl, 
											kFileMob,
											NULL,
											&effectChoice,
											&currentCpnt,
											&currentPos,
											&foundSource,
											&foundTransition);
				if(srcError != OM_ERR_NONE)
				{
					printf("GetNextSource failed parsing a non-sequence\n");
					numErrors++;
				}
				CHECK(omfiMobGetInfo(fileHdl, foundSource.mob, NULL, sizeof(fileMobName),
							fileMobName, NULL, NULL));
				if(strcmp(fileMobName, "Test FileMob") != 0)
				{
					printf("Found the wrong file mob\n");
					numErrors++;
				}
				
				effectChoice = kFindIncoming;
				srcError = omfiMobGetNextSource(mobFindHdl, 
												kMasterMob,
												NULL,
												&effectChoice,
												&currentCpnt,
												&currentPos,
												&foundSource,
												&foundTransition);
				if(srcError != OM_ERR_NO_MORE_OBJECTS)
				{
					printf("Too many elements in the sequence\n");
					numErrors++;
				}
				CHECK(omfiMobCloseSearch(fileHdl, mobFindHdl));
			}
		}
		CHECK(omfiIteratorDispose(fileHdl, trackIter));
		trackIter = NULL;

		CHECK(omfsCloseFile(fileHdl));
		fileHdl = NULL;
		
		omfsEndSession(session);
		
		if(numErrors != 0)
		{
			RAISE(OM_ERR_TEST_FAILED);
		}
		
		printf("TstScope completed successfully.\n");

      } /* XPROTECT */

    XEXCEPT
      {
	if (trackIter)
	  omfiIteratorDispose(fileHdl, trackIter);
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

static omfErr_t CreateCompositionMob(
		omfHdl_t    	fileHdl,
		omfInt16		numSrcRef,
		omfSourceRef_t	*vSource,
		omfMobObj_t		*compMobPtr)
{
    omfRational_t	editRate;
    omfLength_t		trackLength;
    omfTimecode_t	timecode;
    omfMSlotObj_t	vTrack1, tcTrack4;
    omfSegObj_t		vSequence;
    omfSegObj_t		tcClip, vClip1, vClip2, trackScope, effectScope;
    omfDDefObj_t	pictureDef, soundDef, rationalDef;
    omfTrackID_t	trackID;
    omfErr_t 		omfError;
    omfLength_t		clipLen10, clipLen20;
    omfPosition_t	zeroPos;
	omfMobObj_t		compMob;
	omfRational_t	effectSpeed,wipePercent;
	omfSegObj_t		scopeRef1, scopeRef2, cval;
	omfEffObj_t		effect;
	omfWipeArgs_t	wipeArgs;
	
    XPROTECT(fileHdl)
	{
		editRate.numerator = 2997;
		editRate.denominator = 100;
		omfsCvtInt32toInt64(40, &trackLength);
		omfsCvtInt32toInt64(10, &clipLen10);
		omfsCvtInt32toInt64(20, &clipLen20);
		omfsCvtInt32toInt64(0, &zeroPos);
	
		omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
		omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
		omfiDatakindLookup(fileHdl, RATIONALKIND, &rationalDef, &omfError);
		
		/* Create the empty composition mob */	
		printf("Creating composition 'NestedScopeTest'\n");
		CHECK(omfiCompMobNew(fileHdl, (omfString) "NestedScopeTest", TRUE, &compMob));

		printf("    Creating Timecode Track\n");
		/* Create timecode Clip and append to timecode tracks */
		timecode.startFrame = 0;
		timecode.drop = kTcDrop;
		timecode.fps = 30;
		CHECK(omfiTimecodeNew(fileHdl, trackLength, timecode, &tcClip));
		trackID = 4;
		CHECK(omfiMobAppendNewSlot(fileHdl, compMob, editRate, tcClip,
					   &tcTrack4));

		/* ****** See comment at the top of the file for the test sequence ****** */
		trackID = 1;
		CHECK(omfiNestedScopeNew(fileHdl, pictureDef, trackLength, &trackScope));
		CHECK(omfiMobAppendNewTrack(fileHdl, compMob, editRate, trackScope, 
					    zeroPos, trackID, "VTrack", &vTrack1));

		printf("    Creating video layer 1\n");
		/* Create a nest containing an effect which references media within
		 * the current nest, but up one track.
		 */
		CHECK(omfiNestedScopeNew(fileHdl, pictureDef, clipLen10, &effectScope));
		CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen10, vSource[1], &vClip1));
		CHECK(omfiNestedScopeAppendSlot(fileHdl, effectScope, vClip1));

		CHECK(omfiScopeRefNew(fileHdl, pictureDef, clipLen10, 0, 1, &scopeRef1));
		effectSpeed.numerator = 2;
		effectSpeed.denominator = 1;
		CHECK(omfeVideoSpeedControlNew(fileHdl, clipLen10, scopeRef1, effectSpeed,
										0, &effect));
		CHECK(omfiNestedScopeAppendSlot(fileHdl, effectScope, effect));
		
		/* Build sequence with created segments */
		CHECK(omfiSequenceNew(fileHdl, pictureDef, &vSequence));
		CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen20, vSource[0], &vClip1));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vClip1));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, effectScope));
		CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen10, vSource[2], &vClip2));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vClip2));
		CHECK(omfiNestedScopeAppendSlot(fileHdl, trackScope, vSequence));

		printf("    Creating video layer 2\n");
		/* Create a nest containing an effect which references media both
		 * locally with a SCLP, and media in the containing scope (NOT the
		 * scurrent scope), and up one track.
		 */
		CHECK(omfiNestedScopeNew(fileHdl, pictureDef, clipLen10, &effectScope));

		CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen10, vSource[3], &vClip1));
		CHECK(omfiScopeRefNew(fileHdl, pictureDef, clipLen10, 1, 1, &scopeRef1));

		wipeArgs.reverse = FALSE;
		wipeArgs.soft = FALSE;
		wipeArgs.border = FALSE;
		wipeArgs.position = FALSE;
		wipeArgs.modulator = FALSE;
		wipeArgs.shadow = FALSE;
		wipeArgs.tumble = FALSE;
		wipeArgs.spotlight = FALSE;
		wipeArgs.replicationH = 1;
		wipeArgs.replicationV = 1;
		wipePercent.numerator = 1;
		wipePercent.denominator = 2;
		CHECK(omfiConstValueNew(fileHdl, rationalDef, clipLen10, sizeof(omfRational_t),
								&wipePercent, &cval));
		CHECK(omfeSMPTEVideoWipeNew(fileHdl, clipLen10, vClip1, scopeRef1, 
									cval, 1, &wipeArgs, &effect));
		CHECK(omfiNestedScopeAppendSlot(fileHdl, effectScope, effect));
		
		/* Build sequence with created segments */
		CHECK(omfiSequenceNew(fileHdl, pictureDef, &vSequence));
		CHECK(omfiScopeRefNew(fileHdl, pictureDef, clipLen10, 0, 1, &scopeRef1));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, scopeRef1));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, effectScope));
		CHECK(omfiScopeRefNew(fileHdl, pictureDef, clipLen20, 0, 1, &scopeRef2));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, scopeRef2));
		CHECK(omfiNestedScopeAppendSlot(fileHdl, trackScope, vSequence));

		*compMobPtr = compMob;
	} /* XPROTECT */
    XEXCEPT
	{
		return(XCODE());
	}
    XEND;

    return(OM_ERR_NONE);
}

static omfErr_t CreateBadCompositionMob1(	/* Effect within effect */
		omfHdl_t    	fileHdl,
		omfInt16		numSrcRef,
		omfSourceRef_t	*vSource,
		omfMobObj_t		*compMobPtr)
{
    omfRational_t	editRate;
    omfLength_t		trackLength;
    omfTimecode_t	timecode;
    omfMSlotObj_t	vTrack1, tcTrack4;
    omfSegObj_t		tcClip, vClip1, vSequence;
    omfDDefObj_t	pictureDef, soundDef, rationalDef;
    omfTrackID_t	trackID;
    omfErr_t 		omfError;
    omfLength_t		clipLen10;
    omfPosition_t	zeroPos;
	omfMobObj_t		compMob;
	omfRational_t	effectSpeed;
	omfEffObj_t		effect, effect2;
	
    XPROTECT(fileHdl)
	{
		editRate.numerator = 2997;
		editRate.denominator = 100;
		omfsCvtInt32toInt64(40, &trackLength);
		omfsCvtInt32toInt64(10, &clipLen10);
		omfsCvtInt32toInt64(0, &zeroPos);
	
		omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
		omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
		omfiDatakindLookup(fileHdl, RATIONALKIND, &rationalDef, &omfError);
		
		/* Create the empty composition mob */	
		printf("Creating composition 'NG Effect Within Effect' (Negative Test Case)\n");
		CHECK(omfiCompMobNew(fileHdl, (omfString) "NG Effect Within Effect", TRUE, &compMob));

		printf("    Creating Timecode Track\n");
		/* Create timecode Clip and append to timecode tracks */
		timecode.startFrame = 0;
		timecode.drop = kTcDrop;
		timecode.fps = 30;
		CHECK(omfiTimecodeNew(fileHdl, trackLength, timecode, &tcClip));
		trackID = 4;
		CHECK(omfiMobAppendNewTrack(fileHdl, compMob, editRate, tcClip,
					    zeroPos, 4, "TCTrack",  &tcTrack4));


		printf("    Creating video track\n");
		CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen10, vSource[1], &vClip1));

		effectSpeed.numerator = 2;
		effectSpeed.denominator = 1;
		CHECK(omfeVideoSpeedControlNew(fileHdl, clipLen10, vClip1, effectSpeed,
										0, &effect));
		effectSpeed.numerator = 2;
		effectSpeed.denominator = 1;
		CHECK(omfeVideoSpeedControlNew(fileHdl, clipLen10, effect, effectSpeed,
										0, &effect2));
		CHECK(omfiSequenceNew(fileHdl, pictureDef, &vSequence));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, effect2));
		
		/* Build sequence with created segments */
		CHECK(omfiMobAppendNewTrack(fileHdl, compMob, editRate, vSequence, 
					    zeroPos, 1, "VTrack", &vTrack1));

		*compMobPtr = compMob;
	} /* XPROTECT */
    XEXCEPT
	{
		return(XCODE());
	}
    XEND;

    return(OM_ERR_NONE);
}

static omfErr_t CreateBadCompositionMob2(	/* Effect within effect */
		omfHdl_t    	fileHdl,
		omfInt16		numSrcRef,
		omfSourceRef_t	*vSource,
		omfMobObj_t		*compMobPtr)
{
    omfRational_t	editRate;
    omfLength_t		trackLength;
    omfTimecode_t	timecode;
    omfMSlotObj_t	vTrack1, tcTrack4;
    omfSegObj_t		tcClip, vClip0, vClip1, vSequence, cval, vDissolve1, vTransition;
    omfDDefObj_t	pictureDef, soundDef, rationalDef;
    omfTrackID_t	trackID;
    omfErr_t 		omfError;
    omfLength_t		clipLen10, clipLen20;
    omfPosition_t	zeroPos, clipPos5;
	omfMobObj_t		compMob;
	omfRational_t	effectSpeed, dissolveLevel;
	omfEffObj_t		effect;
	
    XPROTECT(fileHdl)
	{
		editRate.numerator = 2997;
		editRate.denominator = 100;
		omfsCvtInt32toInt64(30, &trackLength);
		omfsCvtInt32toInt64(10, &clipLen10);
		omfsCvtInt32toInt64(20, &clipLen20);
		omfsCvtInt32toInt64(5, &clipPos5);
		omfsCvtInt32toInt64(0, &zeroPos);
	
		omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
		omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
		omfiDatakindLookup(fileHdl, RATIONALKIND, &rationalDef, &omfError);
		
		/* Create the empty composition mob */	
		printf("Creating composition 'NG Effect Within Transition' (Negative Test Case)\n");
		CHECK(omfiCompMobNew(fileHdl, (omfString) "NG Effect Within Transition", TRUE, &compMob));

		printf("    Creating Timecode Track\n");
		/* Create timecode Clip and append to timecode tracks */
		timecode.startFrame = 0;
		timecode.drop = kTcDrop;
		timecode.fps = 30;
		CHECK(omfiTimecodeNew(fileHdl, trackLength, timecode, &tcClip));
		trackID = 4;
		CHECK(omfiMobAppendNewTrack(fileHdl, compMob, editRate, tcClip,
					    zeroPos, 4, "TCTrack",  &tcTrack4));


		printf("    Creating video track\n");
		CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen20, vSource[0], &vClip0));
		CHECK(omfiSourceClipNew(fileHdl, pictureDef, clipLen20, vSource[1], &vClip1));

		effectSpeed.numerator = 2;
		effectSpeed.denominator = 1;
		CHECK(omfeVideoSpeedControlNew(fileHdl, clipLen20, vClip1, effectSpeed,
										0, &effect));
		
		/* Create transition with dissolve effect, TrackA and TrackB implied */
		dissolveLevel.numerator = 2;
		dissolveLevel.denominator = 3;
		CHECK(omfiConstValueNew(fileHdl, rationalDef, clipLen10, sizeof(omfRational_t),
								&dissolveLevel, &cval));
		CHECK(omfeVideoDissolveNew(fileHdl, clipLen10, NULL, NULL, cval, &vDissolve1));
		CHECK(omfiTransitionNew(fileHdl, pictureDef, clipLen10, clipPos5, vDissolve1,
					       &vTransition));
					       
		CHECK(omfiSequenceNew(fileHdl, pictureDef, &vSequence));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vClip0));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, vTransition));
		CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, effect));
		
		/* Build sequence with created segments */
		CHECK(omfiMobAppendNewTrack(fileHdl, compMob, editRate, vSequence, 
					    zeroPos, 1, "VTrack", &vTrack1));

		*compMobPtr = compMob;
	} /* XPROTECT */
    XEXCEPT
	{
		return(XCODE());
	}
    XEND;

    return(OM_ERR_NONE);
}

static omfErr_t CreateMasterClips(
		omfHdl_t    	fileHdl,
		omfInt16		numSrcRef,
		omfSourceRef_t	*vSource)
{
    omfRational_t	editRate;
    omfLength_t		trackLength;
    omfDDefObj_t	pictureDef, soundDef, rationalDef;
    omfErr_t 		omfError;
    omfPosition_t	zeroPos;
	omfMobObj_t		masterMob, fileMob;
	omfUID_t		mobID;
	char			masterMobName[64], fileMobName[64];
	omfInt32		n;
	
    XPROTECT(fileHdl)
	{
		editRate.numerator = 2997;
		editRate.denominator = 100;
		omfsCvtInt32toInt64(40, &trackLength);
		omfsCvtInt32toInt64(0, &zeroPos);
	
		omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
		omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
		omfiDatakindLookup(fileHdl, RATIONALKIND, &rationalDef, &omfError);

		printf("Creating Master/File Mob ");
		for(n = 0; n <= 3; n++)
		{
			printf("%ld...", n);
			sprintf(masterMobName, "Test Master %ld", n);
			sprintf(fileMobName, "Test FileMob %ld", n);
			CHECK(omfmMasterMobNew(fileHdl, /* file (IN) */
				     masterMobName, /* name (IN) */
				     TRUE, /* isPrimary (IN) */
				     &masterMob)); /* result (OUT) */
			
			CHECK(omfmFileMobNew(fileHdl, /* file (IN) */
				   fileMobName, /* name (IN) */
				   editRate,	/* editrate (IN) */
				   CODEC_RGBA_VIDEO, /* codec (IN) */
				   &fileMob)); /* result (OUT) */
			CHECK(omfmMobAddNilReference(fileHdl, fileMob, 1, trackLength,
										pictureDef, editRate));
			CHECK(omfmMobAddTextLocator(fileHdl, fileMob, "There's really no media here"));

			CHECK(omfiMobGetInfo(fileHdl, masterMob, &mobID, 0, NULL, NULL, NULL));
			CHECK(omfmMobAddPhysSourceRef(fileHdl, masterMob, editRate, 1, pictureDef,
											fileMob, zeroPos, 1, trackLength));
			
			vSource[n].sourceID = mobID;
			omfsCvtInt32toInt64(0, &vSource[n].startTime);
			vSource[n].sourceTrackID = 1;
		}
		printf("\n");
	} /* XPROTECT */
    XEXCEPT
	{
		return(XCODE());
	}
    XEND;

    return(OM_ERR_NONE);
}

static omfErr_t CreateOneMasterClip(
		omfHdl_t    	fileHdl,
		omfObject_t		*master)
{
    omfRational_t	editRate;
    omfLength_t		trackLength;
    omfDDefObj_t	pictureDef, soundDef, rationalDef;
    omfErr_t 		omfError;
    omfPosition_t	zeroPos;
	omfMobObj_t		masterMob, fileMob;
	omfUID_t		mobID;
	char			masterMobName[64], fileMobName[64];
	
    XPROTECT(fileHdl)
	{
		editRate.numerator = 2997;
		editRate.denominator = 100;
		omfsCvtInt32toInt64(40, &trackLength);
		omfsCvtInt32toInt64(0, &zeroPos);
	
		omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
		omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
		omfiDatakindLookup(fileHdl, RATIONALKIND, &rationalDef, &omfError);

		sprintf(masterMobName, "Test Master Mob");
		sprintf(fileMobName, "Test FileMob");
		CHECK(omfmMasterMobNew(fileHdl, /* file (IN) */
			     masterMobName, /* name (IN) */
			     TRUE, /* isPrimary (IN) */
			     &masterMob)); /* result (OUT) */
		
		CHECK(omfmFileMobNew(fileHdl, /* file (IN) */
			   fileMobName, /* name (IN) */
			   editRate,	/* editrate (IN) */
			   CODEC_RGBA_VIDEO, /* codec (IN) */
			   &fileMob)); /* result (OUT) */
		CHECK(omfmMobAddNilReference(fileHdl, fileMob, 1, trackLength,
									pictureDef, editRate));
		CHECK(omfmMobAddTextLocator(fileHdl, fileMob, "There's really no media here"));

		CHECK(omfiMobGetInfo(fileHdl, masterMob, &mobID, 0, NULL, NULL, NULL));
		CHECK(omfmMobAddPhysSourceRef(fileHdl, masterMob, editRate, 1, pictureDef,
										fileMob, zeroPos, 1, trackLength));
			
		*master = masterMob;
	} /* XPROTECT */
    XEXCEPT
	{
		return(XCODE());
	}
    XEND;

    return(OM_ERR_NONE);
}
