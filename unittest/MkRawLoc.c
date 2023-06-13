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
 * This unittest creates a raw audio file using the native codec, and
 * a composition which references the raw file.  The raw file is then
 * opened from the composition and the data compared.
 */
#include "masterhd.h"
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include "omPublic.h"
#include "omMedia.h"

#include "UnitTest.h"

#define BUFFER_SIZE	20000
#define FatalError(msg) { printf(msg); RAISE(OM_ERR_TEST_FAILED); }


/****************
 * MAIN PROGRAM *
 ****************/
int MkRawLoc(char *compFileName)
{
	omfSessionHdl_t session;
	omfHdl_t        file = NULL, rawFile;
	omfMediaHdl_t   media;
	omfUInt32       n;
	omfUInt8        *stdWriteBuffer = NULL;
	omfUInt8        *stdReadBuffer = NULL;
	omfRational_t	audioRate;
	omfDDefObj_t	soundKind;
	omfErr_t        status;
	omfObject_t     fileMob, masterMob;
	omfLength_t		length;
	omfLength_t     numSamplesLong;
	omfUInt32		numSamples, bytesRead, samplesRead;
	char			*AIFCFileName = "aifc.raw";
	char			*WAVEFileName = "wave.raw";
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Raw Locator UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;
	
	XPROTECT(NULL)
	{
		audioRate.numerator = 44100;
		audioRate.denominator = 1;
		
		/* Start an OMF Session */
		CHECK(omfsBeginSession(&ProductInfo, &session));
		omfmInit(session);

		if (stdWriteBuffer == NULL)
			stdWriteBuffer = (omfUInt8 *) omfsMalloc(BUFFER_SIZE);
		if (stdReadBuffer == NULL)
			stdReadBuffer = (omfUInt8 *) omfsMalloc(BUFFER_SIZE);
		if ((stdWriteBuffer == NULL) || (stdReadBuffer == NULL))
			RAISE(OM_ERR_NOMEMORY);
	
		for (n = 0; n < BUFFER_SIZE; n++)
		{
			stdWriteBuffer[n] = (omfUInt8)(n & 0xFF);
			stdReadBuffer[n] = 0;
		}

		/* Create a raw AIFC file
		 */
		CHECK(omfsCreateRawFile(AIFCFileName, 
								(omfInt16)(strlen(AIFCFileName)+1), session,
								CODEC_AIFC_AUDIO, &rawFile));
		CHECK(omfmAudioMediaCreate(rawFile, NULL, 0, NULL, audioRate, 
								   kToolkitCompressionDisable,
								   8, 1, &media));
		CHECK(omfmWriteRawData(media, BUFFER_SIZE, stdWriteBuffer, 
							   BUFFER_SIZE));
		CHECK(omfmMediaClose(media));
		CHECK(omfsCloseFile(rawFile));


		/* Create the OMFI composition file
		 */
		CHECK(omfsCreateFile(compFileName, session, kOmfRev2x, &file));	
	
		omfiDatakindLookup(file, SOUNDKIND, &soundKind, &status);
		CHECK(status);
		
		/* make an AIFC Reference, read and compare.
		 */
		CHECK(omfmFileMobNew(file, "a file mob", audioRate,
				    CODEC_AIFC_AUDIO, &fileMob));
		omfsCvtInt32toLength(1, length);
	 	CHECK(omfmMobAddNilReference(file, fileMob, 1, length, 
							 soundKind, audioRate));
		CHECK(omfmMobAddUnixLocator(file, fileMob, kForeignMedia, 
						   AIFCFileName));
		CHECK(omfmMasterMobNew(file, "testMaster", TRUE, &masterMob));
		CHECK(omfmMobAddMasterTrack(file, masterMob, soundKind, 
						   1, 1, "testTrack", fileMob));
		CHECK(omfmImportRawRef(file, fileMob, AIFCFileName, 
							   (omfInt16)(strlen(AIFCFileName)+1)));
		CHECK(omfmMediaOpen(file, masterMob, 1, NULL, kMediaOpenReadOnly,
							kToolkitCompressionEnable, 
							&media));
		CHECK(omfmGetSampleCount(media, &numSamplesLong));
		CHECK(omfsTruncInt64toUInt32(numSamplesLong, &numSamples));	/* OK AIFC */
		if(numSamples != BUFFER_SIZE)
			FatalError("Wrong number of AIFC samples found\n");
		CHECK(omfmReadRawData(media, BUFFER_SIZE, BUFFER_SIZE, stdReadBuffer,
								&bytesRead, &samplesRead));
		if(memcmp(stdReadBuffer, stdWriteBuffer, BUFFER_SIZE) != 0)
			FatalError("AIFC buffers miscompared\n");
		CHECK(omfmMediaClose(media));
	

		/* Create a raw WAVE file
		 */
		CHECK(omfsCreateRawFile(WAVEFileName, 
								(omfInt16)(strlen(WAVEFileName)+1), session,
								CODEC_WAVE_AUDIO, &rawFile));
		CHECK(omfmAudioMediaCreate(rawFile, NULL, 0, NULL, audioRate, 
								   kToolkitCompressionDisable,
									8, 1, &media));
		CHECK(omfmWriteRawData(media, BUFFER_SIZE, stdWriteBuffer, 
							   BUFFER_SIZE));
		CHECK(omfmMediaClose(media));
		CHECK(omfsCloseFile(rawFile));

		/* make a WAVE Reference, read and compare.
		 */
		CHECK(omfmFileMobNew(file, "a file mob", audioRate,
				    CODEC_WAVE_AUDIO, &fileMob));
		omfsCvtInt32toLength(1, length);
	 	CHECK(omfmMobAddNilReference(file, fileMob, 1, length, 
							 soundKind, audioRate));
		CHECK(omfmMobAddUnixLocator(file, fileMob, kForeignMedia, 
						   WAVEFileName));
		CHECK(omfmMasterMobNew(file, "testMaster", TRUE, &masterMob));
		CHECK(omfmMobAddMasterTrack(file, masterMob, soundKind, 
						   1, 1, "testTrack", fileMob));
		CHECK(omfmImportRawRef(file, fileMob, WAVEFileName, 
							   (omfInt16)(strlen(WAVEFileName)+1)));
		CHECK(omfmMediaOpen(file, masterMob, 1, NULL, kMediaOpenReadOnly,
					   kToolkitCompressionEnable, 
					   &media));
		CHECK(omfmGetSampleCount(media, &numSamplesLong));
		CHECK(omfsTruncInt64toUInt32(numSamplesLong, &numSamples)); /* OK WAVE */
		if(numSamples != BUFFER_SIZE)
			FatalError("Wrong number of WAVE samples found\n");
		CHECK(omfmReadRawData(media, BUFFER_SIZE, BUFFER_SIZE, stdReadBuffer,
								&bytesRead, &samplesRead));
		if(memcmp(stdReadBuffer, stdWriteBuffer, BUFFER_SIZE) != 0)
			FatalError("WAVE buffers miscompared\n");
		CHECK(omfmMediaClose(media));

		/* Close the OMF File */
		CHECK(omfsCloseFile(file));

		if (stdWriteBuffer)
		  omfsFree(stdWriteBuffer);
		if (stdReadBuffer)
		  omfsFree(stdReadBuffer);
	}
	XEXCEPT
	{
		if (stdWriteBuffer)
		  omfsFree(stdWriteBuffer);
		if (stdReadBuffer)
		  omfsFree(stdReadBuffer);
		omfsCloseFile(file);
		fprintf(stderr, "<Fatal %1d>%s\n", XCODE(), 
				omfsGetErrorString(XCODE()));
	}
	XEND;
	
	/* Close the OMF Session */
	omfsEndSession(session);
	
	return(0);
}

/* This is example code for the Macintosh, to show how to get the 
 * volume name from the reference number that the Toolkit gives you.
 */
/* 	volname[0] = '\0';
	volParms.ioCompletion = 0;
	volParms.ioNamePtr = (StringPtr)&volname;
	volParms.ioVRefNum = fs.vRefNum;
	volParms.ioWDProcID = 0;
	volParms.ioWDIndex = 0;
	PBGetWDInfo(&volParms, FALSE);
	volNameLen = volname[0];
	strncpy(volname, volname+1, volNameLen);
	volname[volNameLen] = '\0';
	CHECK(omfmMobAddMacLocator(file, fileMob, kForeignMedia, 
	volname, volParms.ioWDDirID, printfname));
*/

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/

