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
 * Name: Raw2omf.c
 *
 * Function: Example program to read in a raw AUDIO and/or raw VIDEO
 *           create a 2.x OMF file.
 *			 
 */

#include "masterhd.h"
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
#include "omMedia.h"
#include "omMobMgt.h"
#include "omcTIFF.h"

typedef enum
{
	inAIFC, inWAVE, inTIFF, inSD2
} inputFormat_t;

typedef enum
{
	outNone, outTIFFCompressed, outTIFFUncompressed, outJPEG, outRGBA, outAIFC, outWAVE
} outputFormat_t;

#if defined(THINK_C) || defined(__MWERKS__)
#include "MacSupport.h"
#include <console.h>
#include "omcSD2.h"
#endif

#define MakeRational(num, den, resPtr) \
        {(resPtr)->numerator = num; (resPtr)->denominator = den;}

#define MAX_BUFSIZE		1048576L
#define BITS_PER_BYTE		8

static omfRational_t audioRate,frameRate;
static omfLength_t length;
static omfDDefObj_t soundKind, pictureKind;

/*********************
 */
static
void MyGetInputMediaInfo(inputFormat_t inFmt, omfCodecID_t *codec, omfDDefObj_t *mediaKind,
						omfRational_t *rate)
{
	switch(inFmt)
	{
	  case inAIFC:
	  	*codec = CODEC_AIFC_AUDIO;
	  	*mediaKind = soundKind;
	  	*rate = frameRate;   /* really the edit rate, not sample rate.
								MC can't edit at 44.1k. */
	  	break;
	  
	  case inWAVE:
	  	*codec = CODEC_WAVE_AUDIO;
	  	*mediaKind = soundKind;
	  	*rate = frameRate;   /* really the edit rate, not sample rate.
								MC can't edit at 44.1k. */
	  	break;
	  
	  case inSD2:
	  	*codec = CODEC_SD2_AUDIO;
	  	*mediaKind = soundKind;
	  	*rate = frameRate;   /* really the edit rate, not sample rate.
								MC can't edit at 44.1k. */
	  	break;
	  
	  case inTIFF:
	  	*codec = CODEC_TIFF_VIDEO;
	  	*mediaKind = pictureKind;
	  	*rate = frameRate;
	  	break;
	  }
}

/*********************
 */
static
void MyGetOutputMediaInfo(outputFormat_t outFmt, omfCodecID_t *codec, omfDDefObj_t *mediaKind,
						omfRational_t *rate)
{
	switch(outFmt)
	{
	  case outAIFC:
	  	*codec = CODEC_AIFC_AUDIO;
	  	*mediaKind = soundKind;
	  	*rate = audioRate;
	  	break;
	  
	  case outWAVE:
	  	*codec = CODEC_WAVE_AUDIO;
	  	*mediaKind = soundKind;
	  	*rate = audioRate;
	  	break;
	  
	  case outJPEG:
	  	*codec = CODEC_JPEG_VIDEO;
	  	*mediaKind = pictureKind;
	  	*rate = frameRate;
	  	break;
	  
	  case outRGBA:
	  	*codec = CODEC_RGBA_VIDEO;
	  	*mediaKind = pictureKind;
	  	*rate = frameRate;
	  	break;
	  
	  case outTIFFCompressed:
	  case outTIFFUncompressed:
	  	*codec = CODEC_TIFF_VIDEO;
	  	*mediaKind = pictureKind;
	  	*rate = frameRate;
	  	break;
	  }
}


/*********************
 */
static 
omfErr_t CreateLocatorRefToMedia(omfHdl_t omfFileHdl, inputFormat_t inFmt,  outputFormat_t outFmt,
							fileHandleType rawfile,
							omfMobObj_t *rawMasterMob)
{
	omfMobObj_t   rawFileMob;
	omfCodecID_t	codec;
	omfDDefObj_t	mediaKind;
	omfRational_t	rate;
	omfInt16		numCh, n;
  
	XPROTECT(omfFileHdl) 
	{
		MyGetInputMediaInfo(inFmt, &codec, &mediaKind, &rate);
	  
		CHECK(omfmFileMobNew(omfFileHdl, "Raw File Mob", rate, 
							 codec, &rawFileMob))

		/* Generate the locator */
		CHECK(omfmMobAddUnixLocator(omfFileHdl, rawFileMob, kForeignMedia, rawfile));
         
		/* Create the Master Mob */
		CHECK(omfmMasterMobNew(omfFileHdl, "MMob Loc for raw", TRUE, rawMasterMob));
	  
		/* Connect the File Mob and the Master Mob */

		/* Create the reference */
		CHECK(omfmImportRawRef(omfFileHdl, rawFileMob, rawfile, strlen(rawfile)+1));
		CHECK(omfmGetNumChannels(omfFileHdl, rawFileMob, 0, NULL, mediaKind, &numCh));
		for(n = 1; n <= numCh; n++)
		{
			CHECK(omfmMobAddNilReference(omfFileHdl, rawFileMob, n,
										length, mediaKind, rate));
			CHECK(omfmMobAddMasterTrack(omfFileHdl, *rawMasterMob, mediaKind,
								  n, n, "Master Track", rawFileMob));
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;

	return(OM_ERR_NONE);
}

/*********************
 */
static 
omfErr_t OpenReadandWriteMedia(omfHdl_t omfFileHdl, omfMobObj_t rawMasterMob,
							   outputFormat_t outFmt, omfMobObj_t *localFileMob, 
							   omfMobObj_t *localMasterMob, omfMobObj_t *tapeMob, 
							   omfLength_t *numSamplesLong)
{
	omfMediaHdl_t   media, local_media;
	omfUInt8        *MediaBuffer = NULL;
	omfUInt32       numSamples, bytesRead, samplesRead,bufsize;
	omfInt16        numChannels;
	omfInt32        maxsize;
	omfInt32        allbytes, samplesToRead, totalSamplesLeft;
	omfRational_t	sampleRate;
	omfTimecode_t   mediaStartTC;
	omfPosition_t   tcOffset;

  	XPROTECT(omfFileHdl)
	{
		/* Now let's open the data up for reading */
		CHECK(omfmMediaOpen(omfFileHdl, rawMasterMob, 1, NULL, 
						  kMediaOpenReadOnly, kToolkitCompressionEnable,
						  &media));
		CHECK(omfmGetAudioInfo(media, &sampleRate, NULL, NULL));
		  
		/* See how much data we need to read in, and allocate it */ 
		CHECK(omfmGetSampleCount(media, numSamplesLong));
		CHECK(omfsTruncInt64toUInt32(*numSamplesLong, &numSamples));
		CHECK(omfmGetLargestSampleSize(media,soundKind,&maxsize));
		CHECK(omfmGetNumChannels(omfFileHdl,rawMasterMob,1,NULL,
							   soundKind,&numChannels));
		CHECK(omfsStringToTimecode("01:00:00:00",frameRate,&mediaStartTC));
		omfsCvtInt32toPosition(mediaStartTC.startFrame,tcOffset);
	  
		if (numChannels == 1) 
		{
		  	CHECK(omfmAudioMediaCreate(omfFileHdl, *localMasterMob, 1, *localFileMob,
									 sampleRate, kToolkitCompressionEnable, 
									 maxsize*BITS_PER_BYTE, 1, &local_media));

			allbytes = maxsize*numSamples;
			bufsize = (allbytes < MAX_BUFSIZE ? allbytes : MAX_BUFSIZE);
			MediaBuffer = (omfUInt8 *) omfsMalloc(bufsize);
			if (MediaBuffer == NULL)
				RAISE(OM_ERR_NOMEMORY);

			for (totalSamplesLeft =  numSamples; totalSamplesLeft > 0 ; totalSamplesLeft-=samplesRead) 
			{
				samplesToRead = bufsize/maxsize;
				if(samplesToRead > totalSamplesLeft)
					samplesToRead = totalSamplesLeft;
			 
				/* Read the Data in, then close down the reference to that media */	
				CHECK(omfmReadRawData(media, samplesToRead, bufsize, MediaBuffer,
									&bytesRead, &samplesRead));
			  
				CHECK(omfmWriteRawData(local_media, samplesRead, MediaBuffer,
									 bufsize));
			}
		}
		else if (numChannels == 2)
		{
			omfmMultiXfer_t   Xfer[2];

		  	/* This does an omfmMediaMultiCreate() if numChannels != 1 */
		  	CHECK(omfmAudioMediaCreate(omfFileHdl, *localMasterMob, 1, *localFileMob,
									 sampleRate, kToolkitCompressionEnable, 
									 maxsize*BITS_PER_BYTE, 2, &local_media));

			Xfer[0].mediaKind = soundKind;
			Xfer[0].subTrackNum = 1;
			Xfer[1].mediaKind = soundKind;
			Xfer[1].subTrackNum = 2;
		  
			allbytes = numChannels*maxsize*numSamples;
			Xfer[0].buflen = (allbytes < MAX_BUFSIZE ? allbytes : MAX_BUFSIZE);
			Xfer[1].buflen = Xfer[0].buflen;
			Xfer[0].buffer = (omfUInt8 *) omfsMalloc(Xfer[0].buflen);
			Xfer[1].buffer = (omfUInt8 *) omfsMalloc(Xfer[1].buflen);
			if ((Xfer[0].buffer == NULL) || (Xfer[1].buffer == NULL))
				RAISE(OM_ERR_NOMEMORY);

/*!!!! There is a problem here.  It is possible that the read can fail for one
track but not the other.  Or the write could fail but not for the other.  I'm 
not sure how to deal with this properly....   */

			for (totalSamplesLeft =  numSamples; totalSamplesLeft > 0 ; totalSamplesLeft-=Xfer[0].samplesXfered) 
			{
				samplesToRead = Xfer[0].buflen/maxsize;
				if(samplesToRead > totalSamplesLeft)
				 	samplesToRead = totalSamplesLeft;
				Xfer[0].numSamples = samplesToRead;
				Xfer[1].numSamples = samplesToRead;

				/* Read the Data in, then close down the reference to that media */	
				CHECK(omfmReadMultiSamples(media,2,Xfer)); 
				
				CHECK(omfmWriteMultiSamples(local_media,2,Xfer));
			}
			if (Xfer[0].buffer != NULL)
				omfsFree(Xfer[0].buffer);
			if (Xfer[1].buffer != NULL)
				omfsFree(Xfer[1].buffer);

		}
		else
			RAISE(OM_ERR_CODEC_CHANNELS);
	
		CHECK(omfmMediaClose(media));
		CHECK(omfmMediaClose(local_media));
		/* Hook it up to the tape Mob */
		/* Link file mob to tape mob starting at TC 01:00:00:00 */
		CHECK(omfmMobAddPhysSourceRef(omfFileHdl,
									  *localFileMob,  
									  frameRate,     
									  1,             
									  soundKind,
									  *tapeMob,
									  tcOffset,
									  3,
									  *numSamplesLong)); 

	}
	XEXCEPT
	{	  
		if (MediaBuffer)
			omfsFree(MediaBuffer);
		return(XCODE());
	}
	XEND;
	
	if (MediaBuffer)
		omfsFree(MediaBuffer);
  
	return(OM_ERR_NONE);
}

/*********************
 */
static  omfErr_t OpenReadandWriteVideoMedia(omfHdl_t omfFileHdl,
											omfMobObj_t rawMasterMob, 
											outputFormat_t outFmt,
											omfMobObj_t *localFileMob, 
											omfMobObj_t *localMasterMob,
											omfMobObj_t *tapeMob, 
											omfLength_t *numSamplesLong)
{
	omfMediaHdl_t   		media, local_media;
	omfUInt8        		*MediaBuffer = NULL;
	omfUInt32       		numSamples, bytesRead, samplesRead,bufsize;
	omfInt16        		numChannels;
	omfInt32        		maxsize;
	omfInt32        		allbytes, samplesToRead, totalSamplesLeft;
	omfFrameLayout_t		layout;
	omfInt32			StoredHeight, StoredWidth;
	omfRational_t		iRatio;
	omfVideoMemOp_t		fmtOps[2];
	omfTimecode_t   mediaStartTC;
	omfPosition_t   tcOffset;

	
	XPROTECT(omfFileHdl)
	{
		MakeRational(4, 3, &iRatio);

		/* Now let's open the data up for reading */
		CHECK(omfmMediaOpen(omfFileHdl, rawMasterMob, 1, NULL, 
						  kMediaOpenReadOnly, kToolkitCompressionEnable,
						  &media));
		  
		CHECK(omfmGetVideoInfo(media, &layout,  &StoredWidth, &StoredHeight,
								NULL, NULL, NULL));

		/* See how much data we need to read in, and allocate it */ 
		CHECK(omfmGetSampleCount(media, numSamplesLong));
		CHECK(omfsTruncInt64toUInt32(*numSamplesLong, &numSamples));
		CHECK(omfmGetLargestSampleSize(media,pictureKind,&maxsize));
		CHECK(omfmGetNumChannels(omfFileHdl,rawMasterMob,1,NULL,
							   pictureKind,&numChannels));
		CHECK(omfsStringToTimecode("01:00:00:00",frameRate,&mediaStartTC));
		omfsCvtInt32toPosition(mediaStartTC.startFrame,tcOffset);
	  
		CHECK(omfmVideoMediaCreate(
			omfFileHdl,
			*localMasterMob, 1, *localFileMob,
			kToolkitCompressionEnable,
			frameRate, 
			StoredHeight, StoredWidth, layout, iRatio,
			&local_media));	

		if(outFmt == outJPEG)
		{
			fmtOps[0].opcode = kOmfCDCICompWidth;
			fmtOps[0].operand.expInt32 = 8;
			fmtOps[1].opcode = kOmfVFmtEnd;
			CHECK(omfmPutVideoInfoArray(local_media, fmtOps));
		}

		if(outFmt == outTIFFCompressed)
		{
			omfTIFF_JPEGInfo_t	TIFFPrivate;

			TIFFPrivate.JPEGTableID = 0;
			TIFFPrivate.compression = kJPEG;
			CHECK(omfmCodecSendPrivateData(local_media, sizeof(TIFFPrivate), &TIFFPrivate));
		}

		allbytes = maxsize*numSamples;
		bufsize = (allbytes < MAX_BUFSIZE ? allbytes : MAX_BUFSIZE);
		MediaBuffer = (omfUInt8 *) omfsMalloc(bufsize);
		if (MediaBuffer == NULL)
			RAISE(OM_ERR_NOMEMORY);

		for (totalSamplesLeft =  numSamples; totalSamplesLeft > 0 ; totalSamplesLeft-=samplesRead) 
		{
			samplesToRead = bufsize/maxsize;
			if(samplesToRead > totalSamplesLeft)
				samplesToRead = totalSamplesLeft;
		 
			/* Read the Data in, then close down the reference to that media */	
			CHECK(omfmReadRawData(media, samplesToRead, bufsize, MediaBuffer,
								&bytesRead, &samplesRead));
		  
			CHECK(omfmWriteRawData(local_media, samplesRead, MediaBuffer,
								 bufsize));
		}
		/* Link file mob to tape mob starting at TC 01:00:00:00 */
		CHECK(omfmMobAddPhysSourceRef(omfFileHdl,
									  *localFileMob,  
									  frameRate,     
									  1,             
									  pictureKind,
									  *tapeMob,
									  tcOffset,
									  2,
									  *numSamplesLong)); 

		CHECK(omfmMediaClose(media));
		CHECK(omfmMediaClose(local_media));
	}
	XEXCEPT
	{	  
		if (MediaBuffer)
			omfsFree(MediaBuffer);
	}
	XEND;
	if (MediaBuffer)
		omfsFree(MediaBuffer);
  
	return(OM_ERR_NONE);
}

/*********************
 */
static
omfErr_t CreateTapeMob(omfHdl_t omfFileHdl, omfMobObj_t *tapeMob)
{

	XPROTECT(omfFileHdl)
	{
		CHECK(omfmStandardTapeMobNew(omfFileHdl,
								   frameRate,
								   audioRate,
								   "Fake Tape Mob Descriptor",
								   1, /* IN numVideoTracks */
								   4, /* IN numAudioTracks*/
								   tapeMob)); 

	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;
	return(OM_ERR_NONE);
}

/*********************
 */
static int CreateOMFfile(fileHandleType rawfile,fileHandleType omffile,inputFormat_t inFmt,
							outputFormat_t outFmt, omfFileRev_t outRev)
{
	omfSessionHdl_t session;
	omfMobObj_t     rawMasterMob, localMasterMob, localFileMob, tapeMob;
	omfHdl_t        omfFileHdl;
	omfErr_t        status;
	omfLength_t     numSamples;
	omfDDefObj_t	mediaKind;
	omfCodecID_t	codec;
	omfRational_t	rate;
	omfProductIdentification_t ProductInfo;

	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "AIFC/WAVE/TIFF to OMF Example";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = 0;
	ProductInfo.platform = NULL;

  XPROTECT(NULL)
	{
		/* CreateSession */
		CHECK(omfsBeginSession(&ProductInfo, &session));
		CHECK(omfmInit(session));
#if defined(THINK_C) || defined(__MWERKS__)
		CHECK(omfmRegisterCodec(session, omfCodecSD2, registerLinked));
#endif
  
		/* Open file for create */
		CHECK(omfsCreateFile(omffile,session,outRev,&omfFileHdl));
  
		omfiDatakindLookup(omfFileHdl, SOUNDKIND, &soundKind, &status);/*Sets global*/
		CHECK(status);
		omfiDatakindLookup(omfFileHdl, PICTUREKIND, &pictureKind, &status);/*Sets global*/
		CHECK(status);
  
		/* First step sets up a locator, pointing to the first raw file.*/
		/*                                                              */
		/* Then we want to read the data from each of the the Raw files */
		/* and write them into the OMF file.  To do this, we need to    */
		/* create a second Master Mob structure that will point to the  */
		/* copy of media that is local to the omf file.                 */
		/*                                                              */
		/* This process is repeated for each of the input raw files.    */
	  
		/* Step one, create a reference to the Media.  In this case     */
		/* We're going to create a MMOB -> FMOB (with locator) reference*/

		CHECK(CreateLocatorRefToMedia(omfFileHdl,inFmt, outFmt,
							  rawfile,&rawMasterMob));

		/* Create a new File Mob for the Media Data with the appropriate*/
		/* CODEC referenced. This is the time to ask what CODEC they    */
		/* want to write out...                                         */

		MyGetOutputMediaInfo(outFmt, &codec, &mediaKind, &rate);
	  
		CHECK(omfmFileMobNew(omfFileHdl, "Local File Mob", rate, 
							 codec, &localFileMob));
	  
		/* Create the Master Mob for the local media */
		CHECK(omfmMasterMobNew(omfFileHdl, "MMOB for Local Data", TRUE,
							 &localMasterMob));

		/* Create a TapeMob as reference to the data source...          */

		CHECK(CreateTapeMob(omfFileHdl,&tapeMob));

		/* Now we can read in the data via the Reference copy, and      */
		/* write it out into the local media reference                  */
      
		switch(inFmt)
		{
			case inTIFF:
	  			CHECK(OpenReadandWriteVideoMedia(omfFileHdl,rawMasterMob,outFmt,
	  						&localFileMob,&localMasterMob,&tapeMob,
							&numSamples));
			break;
		
			default:
				CHECK(OpenReadandWriteMedia(omfFileHdl,rawMasterMob,outFmt,
	  						&localFileMob,&localMasterMob,&tapeMob,
							&numSamples));
			break;
		}

		/* Close file, clean up*/
		CHECK(omfsCloseFile(omfFileHdl));
	}
	XEXCEPT
	{
	  printf("*** Fatal Error %1d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
      
      CHECK(omfsCloseFile(omfFileHdl));
	}
	XEND;
  
	/* EndSession */
	omfsEndSession(session);
	
	return(OM_ERR_NONE);
}

/*********************
 */
static 
int usage(void)
{
	printf("*** ERROR - Raw2omf needs 2 or 4 args \n");
	printf(" Usage: Raw2omf [-out <output format>] [-rev1] <input raw file> <output omf file>\n");
	printf(" where output format may be:\n");
	printf("      tiffjpeg    -- TIFF with JPEG compression\n");
	printf("      tiff        -- TIFF RGB 24-bit\n");
	printf("      rgba        -- RGBA 2.0 format\n");
	printf("      jpeg        -- JPEG 2.0 format\n");
	printf("      aifc\n");
	printf("      wave\n");
	exit(1);
	return(0);  /* code warrior insists on this... */
}

/*********************
 */
int main(int argc, char *argv[])
{
  fileHandleType	rawfile, omffile;
  inputFormat_t 	inFmt;
  outputFormat_t	outFmt;
  omfInt16			n;
  omfFileRev_t		outRev = kOmfRev2x;
  char            	*str;
    
#if defined(THINK_C) || defined(__MWERKS__)
  MacInit();
  argc = ccommand(&argv);
#endif

 outFmt = outNone;
  /* arg parsing */
  if (argc < 3)
	usage();
  if (argc == 3)
	{
	  rawfile = argv[1];
	  omffile = argv[2];
	}
  else if (argc >= 4)
	{
	  if((argv[1])[0] == '-')
	  {
		  for(n = 1; n < argc-2; n++)
		  {
		  	if(strncmp(argv[n], "-out", 4) == 0)
		  	{
			  	n++;
			  	if(strcmp(argv[n], "tiffjpeg") == 0)
			  		outFmt = outTIFFCompressed;
			  	else if(strcmp(argv[n], "tiff") == 0)
			  		outFmt = outTIFFUncompressed;
			  	else if(strcmp(argv[n], "jpeg") == 0)
			  		outFmt = outJPEG;
			  	else if(strcmp(argv[n], "rgba") == 0)
			  		outFmt = outRGBA;
			  	else if(strcmp(argv[n], "aifc") == 0)
			  		outFmt = outAIFC;
			  	else if(strcmp(argv[n], "wave") == 0)
			  		outFmt = outWAVE;
			  	else
	     				usage();
		  	}
		  	else if(strncmp(argv[n], "-rev1", 4) == 0)
		  		outRev = kOmfRev1x;
		  	else
     				usage();
		  }
		  rawfile = argv[argc-2];
		  omffile = argv[argc-1];
	  }
	  else
	     usage();
	}

	if((outRev == kOmfRev1x) &&
		((outFmt == outRGBA) || (outFmt == outJPEG)))
	{
		printf("WARNING: 2.0 media type being created in OMFI 1.0 file\n");
	}
	
  /* Check media Type AIFC or WAV based on file extension */
  str = strrchr(rawfile,'.');
  if (!strncmp(str,".aif",3))
	inFmt = inAIFC;
  else 
	if(!strncmp(str,".wav",3))
		inFmt = inWAVE;
	else 
	  {
		if(!strncmp(str,".tif",3))
			inFmt = inTIFF;
		else
		{	
			if(!strncmp(str,".sd2",3))
				inFmt = inSD2;
			else
			{	printf("*** ERROR - First arg needs to be an aifc, wav, sd2, or tiff\n");
				usage();
			}
		}
	  }

  if(inFmt == inAIFC || inFmt == inWAVE || inFmt == inSD2)
  {
  	if(outFmt == outNone)
  		outFmt = outAIFC;
  	else if(outFmt != outAIFC && outFmt != outWAVE)
  	{
  		printf("Incompatible input & output formats\n");
  		exit(1);
  	}
  }
  else
  {
  	if(outFmt == outNone)
  		outFmt = outTIFFCompressed;
  	else if(outFmt == outAIFC || outFmt == outWAVE)
  	{
  		printf("Incompatible input & output formats\n");
  		exit(1);
  	}
  }
  
  /* Initialize some globals (applications will set them as needed) */

/*	if(outRev == kOmfRev1x)
 	{
*/
 	 MakeRational(2997,100,&audioRate);
/* 	}
 	else
 	{
 	 MakeRational(44100,1,&audioRate);
 	}
*/
  MakeRational(2997,100,&frameRate);
  omfsCvtInt32toLength(1, length);

  
  CreateOMFfile(rawfile,omffile,inFmt, outFmt, outRev);
  return(OM_ERR_NONE);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
