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
 * Name: PlayAudio.c
 *
 * Function: Example program to iterate over all file mobs in a file, playing
 *			 the audio.
 *
 *			Currently restricted to playing mono or the left channel of
 *			interleaved stereo.
 */

#include "masterhd.h"
#if defined(THINK_C) || defined(__MWERKS__)
#include <QuickDraw.h>
#include <StandardFile.h>
#endif
#include <stdlib.h>
#include <strings.h>

#include "omPublic.h"
#include "omMedia.h"
#include "omCodec.h"
#include "omPvt.h"

#if defined(THINK_C) || defined(__MWERKS__)
#include "MacSupport.h"
#include "MacAudioIO.h"
#include "omcSD2.h"
#else
#include "SGIAudioIO.h"
#endif

omfHdl_t *MediaLocate (omfHdl_t file, 
					omfObject_t mdes);

int main(int argc, char *argv[])
{
   omfSessionHdl_t	session;
   omfHdl_t		fileHdl;
	fileHandleType fh;
	omfFileRev_t fileRev;
	omfProperty_t idProp;
   omfMobObj_t	compMob, masterMob;
	omfObject_t track = NULL, trackSeg = NULL, tmpCpnt = NULL;
   omfIterHdl_t	mobIter = NULL, trackIter = NULL, sequIter = NULL;
   omfSearchCrit_t search;
   omfInt16		   numCh;
	char			   errBuf[256];
	omfDDefObj_t 	soundKind = NULL, datakind = NULL;
	omfMediaHdl_t	media;
	omfInt64			clipSamples;
	omfUInt32		bytes, bufsize, samplesRead, bytesRead;
	omfRational_t	rate, editrate;
	omfInt32 		sampleSize, numChannels, numCpnts;
	omfCodecID_t	codecID;
	omfInt16			bytesPerSample, interfaceWidth;
	char			*buffer, repeatBuf[64];
	omfBool		doRepeat, foundAudio;
	omfInt32 numTracks, loop, sequLoop;
	omfLength_t foundMinLength, length, zeroLen, clipLength;
	omfInt32 length32, clipSamples32;
	omfTrackID_t trackID, masterMobTrackID;
	audioInfoPtr_t hdl;
	omfPosition_t offset, masterMobOffset, audioOffset;
	omfUID_t masterMobID;
	char			nameBuf[128];
	char classID[5];
   omfErr_t 		omfError = OM_ERR_NONE, tmpError = OM_ERR_NONE;
#if defined(THINK_C) || defined(__MWERKS__)
	Point 			where;
	SFReply 		   fs;
	SFTypeList 		fTypes;
#endif
	
    XPROTECT(NULL)
	{
#if defined(THINK_C) || defined(__MWERKS__)
		MacInit();
	
		where.h = 20;
		where.v = 40;
	
		/*****
		 * Get the file name and open
		 */
		fTypes[0] = 'OMFI';
		SFGetFile(where,  NULL, NULL, -1, fTypes, NULL, &fs);
		if (! fs.good)
			exit(0);
		fh = (fileHandleType)&fs;
#else
		fh = argv[1];
#endif
		interfaceWidth =  MyGetInterfaceSampleWidth();
			
		/*****
		 * Open the OMF file for read
		 */
		CHECK(omfsBeginSession(NULL, &session));
		CHECK(omfmInit(session));
#if defined(THINK_C) || defined(__MWERKS__)
		CHECK(omfmRegisterCodec(session, omfCodecSD2));
#endif

		CHECK(omfsOpenFile(fh, session, &fileHdl));
		CHECK(omfsFileGetRev(fileHdl, &fileRev));
		if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
		  idProp = OMObjID;
		else
		  idProp = OMOOBJObjClass;
		
		/*****
		 * Media calls take a DDEF object for media type, so make one here.
		 */
		omfiDatakindLookup(fileHdl, SOUNDKIND, &soundKind, &omfError);
		CHECK(omfError);
		omfsCvtInt32toPosition(0, offset);
		omfsCvtInt32toInt64(0, &zeroLen);

/*		omfmLocatorFailureCallback(fileHdl, MediaLocate); */
	
		CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	
		do {
		  /*****
			* Iterate over all file mobs in the file.
			*/
		  search.searchTag = kByMobKind;
		  search.tags.mobKind = kCompMob;
		  omfError = omfiGetNextMob(mobIter, &search, &compMob);
		  while((omfError == OM_ERR_NONE) && (compMob != NULL))
			 {
				CHECK(omfiMobGetNumTracks(fileHdl, compMob, &numTracks));
				CHECK(omfiMobGetInfo(fileHdl, compMob, NULL, sizeof(nameBuf), 
											nameBuf, NULL, NULL));
				printf("Mob '%s'\n", nameBuf);
				foundAudio = FALSE;
				
				/* Iterate through master mob tracks, looking for audio */
				CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
				for (loop = 0; loop < numTracks; loop++)
				  {
					 CHECK(omfiMobGetNextTrack(trackIter, compMob, NULL,&track));
					 CHECK(omfiTrackGetInfo(fileHdl, compMob, track, 
													&editrate, 0, NULL, NULL, 
													&trackID, &trackSeg));
					 CHECK(omfiComponentGetInfo(fileHdl, trackSeg, &datakind, 
														 NULL));
					 
					 if (omfiIsSoundKind(fileHdl, datakind, kExactMatch, 
												&omfError))
						{
						  /* Make sure the that composition mob's track contains
							* a sequence.
							*/
						  if (!omfiIsASequence(fileHdl, trackSeg, &omfError))
							 {
								printf("    <Track component not a sequence>\n");
								RAISE(OM_ERR_INVALID_OBJ);
							 }

						  foundAudio = TRUE;

						  /* Get each component in the sequence */
						  CHECK(omfiIteratorAlloc(fileHdl, &sequIter));
						  CHECK(omfiSequenceGetNumCpnts(fileHdl, trackSeg,
																  &numCpnts));
						  for (sequLoop = 1; sequLoop <= numCpnts; sequLoop++)
							 {
								CHECK(omfiSequenceGetNextCpnt(sequIter, trackSeg, NULL,
																		&offset, &tmpCpnt));
								CHECK(omfiComponentGetInfo(fileHdl, tmpCpnt, NULL,
																	&length));
								/* If 0 length fill, skip */
								if (omfiIsAFiller(fileHdl, tmpCpnt, &tmpError) &&
									 omfsInt64Equal(length, zeroLen))
								  {
									 continue;
								  }
								omfsReadClassID(fileHdl, tmpCpnt, idProp, classID);
								classID[4] = '\0';
								CHECK(omfsTruncInt64toInt32(length, &length32));
								printf("    Playing %4s length %d\n", classID, 
										 length32);
								omfError = omfiMobFindSource(fileHdl, compMob, 
																	  trackID, offset, 
																	  kMasterMob,
																	  NULL, &masterMobID, 
																	  &masterMobTrackID,
																	  &masterMobOffset, 
																	  &foundMinLength, 
																	  &masterMob);
								/* If a master mob was found */
								if (omfError == OM_ERR_NONE)
								  {
									 /*****
									  * See if this master mob has audio
									  */
									 CHECK(omfmGetNumChannels(	
																	  fileHdl,
																	  masterMob,
																	  masterMobTrackID, 
																	  NULL, 
																	  soundKind, 
																	  &numCh));
									 if(numCh != 0)
										{
										  omfError = omfmMediaOpen(
																			fileHdl,
																			masterMob,	
																			masterMobTrackID,
																			NULL, 
																			kMediaOpenReadOnly,
																	kToolkitCompressionDisable,
																			&media);	
										  if(omfError == OM_ERR_MEDIA_NOT_FOUND)
											 {
												printf("    <Media not found>\n");
												continue;
											 }
										  else if(omfError != OM_ERR_NONE)
											 RAISE(omfError);
										 
										  CHECK(omfmGetAudioInfo(	
																		 media,
																		 &rate,
																		 &sampleSize,
																		 &numChannels));

										  if (omfsInt64Less(foundMinLength, length))
											 clipLength = foundMinLength;
										  else
											 clipLength = length;
										  CHECK(omfiConvertEditRate(editrate, 
																			 clipLength,
																			 rate, 
																			 kRoundFloor, 
																			 &clipSamples));
										  CHECK(omfiConvertEditRate(editrate, 
																			 masterMobOffset,
																			 rate, 
																			 kRoundFloor, 
																			 &audioOffset));

										  CHECK(omfsTruncInt64toInt32(clipSamples,
																				 &clipSamples32));
										  CHECK(omfmMediaGetCodecID(media, &codecID));
										  printf("    Found %ld %s samples of audio on track %ld.\n",
													clipSamples32,
													(numCh == 1 ? "mono" : "interleaved"),
													masterMobTrackID);
										 
										  if(MyGetInterfaceSampleWidth() == 8)
											 {
												omfAudioMemOp_t	memFmt[3];
												memFmt[0].opcode = kOmfSampleSize;	 
												memFmt[0].operand.sampleSize = 8;
												memFmt[1].opcode = kOmfSampleFormat;
												memFmt[1].operand.format = kOmfOffsetBinary;
												memFmt[2].opcode = kOmfAFmtEnd;
												CHECK(omfmSetAudioMemFormat(media, memFmt));
												sampleSize = 8;
											 }
										 
										  bytesPerSample = (sampleSize+7) / 8;
										  bufsize = bytesPerSample * clipSamples32 * numChannels;
										  buffer = (char *)omfsMalloc(bufsize);
										  if(buffer == NULL)
											 RAISE(OM_ERR_NOMEMORY);
										  
										  MyInitAudio(rate, sampleSize, numChannels, &hdl);

										  CHECK(omfmGotoFrameNumber(media, audioOffset));
										  CHECK(omfmReadRawData(media, 
																			 clipSamples32,
																			 bufsize,	
																			 buffer,		
																			 &bytesRead,
																			 &samplesRead));
										 
										 CHECK(MyPlaySamples(
																	hdl,
																	buffer,
																	clipSamples32));
										 
										 MyFinishAudio(hdl);
										 omfsFree(buffer);
										 
										 CHECK(omfmMediaClose(media));
									  }
								 } /* if no error */
							} /* for each component */
						 omfiIteratorDispose(fileHdl, sequIter);
					  } /* If sound kind */
				 } /* for each master mob track */
				
				if(!foundAudio)
				  printf("Mob contains no audio\n");
				CHECK(omfiIteratorDispose(fileHdl, trackIter));
				trackIter = NULL;
				omfError = omfiGetNextMob(mobIter, &search, &compMob);
			 } /* master mob loop */
			
			omfiIteratorClear(fileHdl, mobIter);
			printf("repeat (y or n + hit return)?");
			fflush(stdout);
			gets(repeatBuf);
			doRepeat = (repeatBuf[0] == 'y' || repeatBuf[0] == 'Y');
		 } while(doRepeat);
		omfiIteratorDispose(fileHdl, mobIter);
		mobIter = NULL;
		omfError = omfsCloseFile(fileHdl);
		omfsEndSession(session);
	 }
    XEXCEPT
	{
	  if (trackIter)
		omfiIteratorDispose(fileHdl, trackIter);
	  if (mobIter)
		omfiIteratorDispose(fileHdl, mobIter);
	  if (sequIter)
		omfiIteratorDispose(fileHdl, sequIter);

		omfsGetExpandedErrorString(fileHdl, XCODE(), sizeof(errBuf), errBuf);
		printf("***ERROR: %d: %s\n", XCODE(), errBuf);
	}
    XEND;

	return(0);
}

omfHdl_t *MediaLocate (omfHdl_t file, 
					omfObject_t mdes)
{	
#if defined(THINK_C) || defined(__MWERKS__)	
   SFReply 		   fs;
	SFTypeList 		fTypes;
	Point where;
#endif
	fileHandleType fh;
	omfHdl_t fileHdl = NULL;
	char pathname[256];
	omfErr_t omfError = OM_ERR_NONE;

#if defined(THINK_C) || defined(__MWERKS__)	
	/*****
	 * Get the file name and open
	 */
	where.h = 20;
	where.v = 40;
	
	fTypes[0] = 'OMFI';
	SFGetFile(where,  NULL, NULL, -1, fTypes, NULL, &fs);
	if (! fs.good)
		exit(0);
	fh = (fileHandleType)&fs;
#else
	printf("Media Missing... provide a pathname: ");
	if (gets(pathname))
	  fh = pathname;
	else
	  fh = NULL;
#endif
	/* NOTE: Check for isOMFI to decide how to open */
	if (fh == NULL)
	  return(NULL);

	omfError = omfsOpenFile(fh, file->session, &fileHdl);
	if (omfError == OM_ERR_NONE)
		return(&fileHdl);
	else
		return(NULL);
}
/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
