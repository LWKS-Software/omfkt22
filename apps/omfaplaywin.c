
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

#include "omPublic.h"
#include "omMedia.h"
#include "omCodec.h"

#if defined(THINK_C) || defined(__MWERKS__)
#include "MacSupport.h"
#include "MacAudioIO.h"
#endif
//#include "omcSD2.h"
#if defined(_WIN32)
#include "WinAudIO.h"
#else
#include "SGIAudioIO.h"
#endif
#include "omPlugin.h"
#if AVID_CODEC_SUPPORT
#include "omCodec.h"
#include "omcAvJPED.h"
#include "omcAvidJFIF.h"
#endif

/* !!! Change this to a runtime check based upon return value from macaudio.c
 */
#if defined(THINK_C) || defined(__MWERKS__)
#define NO_STEREO	1
#endif

int main(int argc, char *argv[])
{
   omfSessionHdl_t	session;
   omfHdl_t		fileHdl;
   omfMobObj_t	masterMob;
   omfIterHdl_t	mobIter = NULL, trackIter = NULL;
   omfInt16		   numCh;
   omfErr_t 		omfError = OM_ERR_NONE;
#if defined(THINK_C) || defined(__MWERKS__)
	Point 			where;
#if OMFI_MACFSSPEC_STREAM
	StandardFileReply  fs;
#else
	SFReply 		   fs;
#endif
	SFTypeList 		fTypes;
#endif
	char			errBuf[256];
	fileHandleType fh;
	omfDDefObj_t 	soundKind;
	omfMediaHdl_t	media;
	omfInt64			numSamplesLong;
	omfUInt32		numSamples, bytesRead, bufsize;
#ifndef NO_STEREO
	omfUInt32		samplesRead;
#endif
	omfRational_t	rate;
	omfInt32 		sampleSize, numChannels;
	omfCodecID_t	codecID;
	omfInt16			bytesPerSample;
	omfInt32		interfaceWidth;
	char			*buffer, repeatBuf[64];
	omfBool		doRepeat, foundAudio;
   omfSearchCrit_t search;
	omfInt32 numTracks, loop;
	omfObject_t track = NULL, trackSeg = NULL;
	omfDDefObj_t datakind;
	omfTrackID_t trackID;
	audioInfoPtr_t hdl;
	char			nameBuf[128];
	
    XPROTECT(NULL)
	{
#if defined(THINK_C) || defined(__MWERKS__)
		MacInit();
	
		where.h = 20;
		where.v = 40;
		fTypes[0] = 'OMFI';
	
		/*****
		 * Get the file name and open
		 */
#if OMFI_MACFSSPEC_STREAM
		StandardGetFile(NULL, 1, fTypes, &fs);
		if (! fs.sfGood)
			exit(0);
	    fh = (fileHandleType)&fs.sfFile;
#else
		SFGetFile(where,  NULL, NULL, -1, fTypes, NULL, &fs);
		if (! fs.good)
			exit(0);
	    fh = (fileHandleType)&fs;
#endif
#else
		fh = argv[1];
#endif
		interfaceWidth =  MyGetInterfaceSampleWidth();
			
		/*****
		 * Open the OMF file for read
		 */
		CHECK(omfsBeginSession(0, &session));
		CHECK(omfmInit(session));
#if AVID_CODEC_SUPPORT
	  	CHECK(omfmRegisterCodec(sessionPtr, omfCodecAvJPED, kOMFRegisterLinked));
	  	CHECK(omfmRegisterCodec(sessionPtr, omfCodecAvidJFIF, kOMFRegisterLinked));
#endif

#if TEST_PLUGINS
		CHECK(omfmLoadMediaPlugins(session));
#else
#if defined(THINK_C) || defined(__MWERKS__)
	 	CHECK(omfmRegisterCodec(session, omfCodecSD2));
#endif
#endif
		CHECK(omfsOpenFile(fh, session, &fileHdl));
		
		/*****
		 * Media calls take a DDEF object for media type, so make one here.
		 */
		omfiDatakindLookup(fileHdl, SOUNDKIND, &soundKind, &omfError);
		CHECK(omfError);

		CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	
		do {
			/*****
			 * Iterate over all file mobs in the file.
			 */
			search.searchTag = kByMobKind;
			search.tags.mobKind = kMasterMob;
			omfError = omfiGetNextMob(mobIter, &search, &masterMob);
			while((omfError == OM_ERR_NONE) && (masterMob != NULL))
			{
				CHECK(omfiMobGetNumTracks(fileHdl, masterMob, &numTracks));
				
				CHECK(omfiMobGetInfo(fileHdl, masterMob, NULL, sizeof(nameBuf), nameBuf,
									NULL, NULL));
				printf("Mob '%s'\n", nameBuf);
				foundAudio = FALSE;
				
			  /* Iterate through master mob tracks, looking for audio */
			  CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
			  for (loop = 0; loop < numTracks; loop++)
				 {
					CHECK(omfiMobGetNextTrack(trackIter, masterMob, NULL,&track));
					CHECK(omfiTrackGetInfo(fileHdl, masterMob, track, 
												  NULL, 0, NULL, NULL, 
												  &trackID, &trackSeg));
					CHECK(omfiComponentGetInfo(fileHdl, trackSeg, &datakind, 
														NULL));

					if (omfiIsSoundKind(fileHdl, datakind, kExactMatch, 
											  &omfError))
					  {
						 foundAudio = TRUE;
						 /*****
						  * See if this master mob has audio
						  */
						 CHECK(omfmGetNumChannels(	
											  fileHdl,		/* IN -- For this file */ 
											  masterMob,	/* IN -- In this masterMob */
											  trackID,    /* IN - on is track */
											  NULL,       /* Media criteria */
											  soundKind,  /* IN -- for this media type */
											  &numCh));	  /* OUT -- How many channels? */
						 if(numCh != 0)
							{
							  omfError = omfmMediaOpen(
												 fileHdl,		/* IN -- For this file */
												 masterMob,		/* IN -- This file mob */
												 trackID,
												 NULL, /* media criteria */
												 kMediaOpenReadOnly,
												 kToolkitCompressionDisable,
												 &media);		/* OUT -- media handle */
								if(omfError == OM_ERR_MEDIA_NOT_FOUND)
								{
									printf("    <Media not found>\n");
									continue;
								}
								else if(omfError != OM_ERR_NONE)
									RAISE(omfError);
						
							  CHECK(omfmGetSampleCount(media,	&numSamplesLong));
							  CHECK(omfsTruncInt64toUInt32(numSamplesLong, 
																	 &numSamples));
							  CHECK(omfmMediaGetCodecID(media, &codecID));
							  printf("    Found %ld %s samples of audio on track %ld.\n",
							  			numSamples,
							  			(numCh == 1 ? "mono" : "interleaved"),
							  			trackID);

							  CHECK(omfmGetAudioInfo(	
															 media,
															 &rate,
															 &sampleSize,
															 &numChannels));
					
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
							  bufsize = bytesPerSample * numSamples * numChannels;
							  buffer = (char *)omfsMalloc(bufsize);
							  if(buffer == NULL)
								 RAISE(OM_ERR_NOMEMORY);
						

#ifdef NO_STEREO
							  CHECK(omfmReadDataSamples(
																 media,
																 numSamples,	
																 bufsize,
																 buffer,
																 &bytesRead));
	
							  MyInitAudio(rate, sampleSize, 1, &hdl);
#else
							  MyInitAudio(rate, sampleSize, numChannels, &hdl);
							  CHECK(omfmReadRawData(
																 media,
																 numSamples,	
																 bufsize,
																 buffer,
																 &bytesRead,
																 &samplesRead));
#endif
							  CHECK(MyPlaySamples(
														 hdl,
														 buffer,
														 numSamples));
	
							  MyFinishAudio(hdl);
							  omfsFree(buffer);

							  CHECK(omfmMediaClose(media));
							}
					  } /* If sound kind */
				 } /* for each master mob track */
				 
			  if(!foundAudio)
			  	printf("Mob contains no audio\n");
			  CHECK(omfiIteratorDispose(fileHdl, trackIter));
			  trackIter = NULL;
			  omfError = omfiGetNextMob(mobIter, &search, &masterMob);
			} /* master mob loop */
			
			omfiIteratorClear(fileHdl, mobIter);
			printf("repeat (y or n + hit return)?");
			//fflush(stdout);
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

		omfsGetExpandedErrorString(fileHdl, XCODE(), sizeof(errBuf), errBuf);
		printf("***ERROR: %d: %s\n", XCODE(), errBuf);
	}
    XEND;

	return(0);
}

