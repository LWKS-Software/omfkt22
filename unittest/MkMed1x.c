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

#include "masterhd.h"
#ifdef unix
#include <sys/types.h>
#endif

#include <string.h>
#include <stdlib.h>
#if PORT_SYS_MAC
#include <Memory.h>
#include <events.h>
#endif

#include "OMApi.h"
#include "OMFI.h"

#include "MkMed1x.h"

#define STD_WIDTH	320L
#define STD_HEIGHT	240L
#define FRAME_SAMPLES	(STD_WIDTH * STD_HEIGHT)
#define FRAME_BYTES	(FRAME_SAMPLES * 3)

#define TIFF_COMPRESSED_TEST	"TIFF Compressed"
#define TIFF_PRECOMPRESSED_TEST	"TIFF PreCompressed"
#define AIFC_MONO_TEST			"AIFC_MONO"
#define AIFC_STEREO_TEST		"AIFC_STEREO"
#define WAVE_MONO_TEST			"WAVE_MONO"
#define WAVE_STEREO_TEST		"WAVE_STEREO"

#if PORT_SYS_MAC
#define	RESET_TIMER(x)	x=0
#define	START_TIMER(x)	x-= TickCount();
#define	STOP_TIMER(x)	x+= TickCount();
#else
#define	RESET_TIMER(x)
#define	START_TIMER(x)
#define	STOP_TIMER(x)
#endif

static omfUInt8 red[] =	{ 3, 0x80, 0x00, 0x00 };
static omfUInt8 green[] =	{ 3, 0x00, 0x80, 0x00 };
static omfUInt8 blue[] =	{ 3, 0x00, 0x00, 0x80 };

static omfUInt8 *colorPatterns[] = { red, green, blue };
#define NUM_COLORS	(sizeof(colorPatterns) / sizeof(omfUInt8 *))

static omfUInt8        *stdWriteBuffer = NULL;
static omfUInt8        *stdColorBuffer = NULL;
static omfUInt8        *stdReadBuffer = NULL;
static short           motoOrder = MOTOROLA_ORDER, intelOrder = INTEL_ORDER;
static TimeStamp       stdTimeStamp = {42, TRUE};
static UsageCodeType   stdUsage = 0;
static UID             stdUID1 = {1, 2, 3};
static UID             stdUID2 = {4, 5, 6};
static UID             stdUID3 = {7, 8, 9};
static UID             stdUID4 = {10, 11, 12};
static UID             stdUID5 = {13, 14, 15};
static ExactEditRate   stdEditRate = {2997, 100};
static MobIndexElement stdElement1, stdElement2;
static JPEGTableIDType stdJPEGTable = 28;
static ExactEditRate   editRate = {2997, 100};
static imageStruct_t   imageStruct = {RGB888, kFullFrame, STD_WIDTH, STD_HEIGHT, {2997, 100}, 24, JPEG, 0, 0, 0, "test", NULL};
static audioStruct_t   audio1Struct = {{44100, 1}, 16, 1, noAudioComp, "testA", NULL};
static audioStruct_t   audio2Struct = {{44100, 1}, 16, 1, noAudioComp, "testA", NULL};
static audioStruct_t   audio3Struct = {{44100, 1}, 16, 2, noAudioComp, "testA", NULL};
static audioStruct_t   audio4Struct = {{44100, 1}, 16, 2, noAudioComp, "testA", NULL};

#define check(a)	if(!a) { FatalErrorCode(__LINE__, __FILE__); }

static void     FatalErrorCode(int line, char *file)
{
	printf("Error '%s' returned at line %d in %s\n",
	       omfsGetErrorString(OMErrorCode), line, file);
	exit(1);
}

static void     FatalError(char *message)
{
	printf(message);
	exit(1);
}

static omfBool PixelCompare(omfUInt8 *buf1, omfUInt8 *buf2, omfInt32 len, omfInt16 slop)
{
	omfInt32	n, diff;
	
	for(n = 0; n < len; n++)
	{
		diff = *buf1++ - *buf2++;
		if((diff < -slop) || (diff > slop))
			return(FALSE);
	}
	
	return(TRUE);		
}

static omfBool SoundCompare(void *buf1, void *buf2, omfInt32 lenSamples, omfUInt16 byteOrder)
{
	omfInt32	n;
	omfInt16	val;
	omfInt16	*ptr1, *ptr2;
	
	/* NOTE that with the synthetic test patterns used by this test, we have to byte swap
	 * on the compare in order to make things come out correctly when transferring
	 * between defierently-endian machines.  This is because the test patterns are sent and
	 * tested as bytes, but are interpreted by the codecs as words.  The pattern 0x00 0x01
	 * would have been the word 0x0100 on a PC, and would have been byte-swapped to 0x01 0x00
	 * for correct transference to a Mac as AIFC data.  The mac codec would not byte-swap the data
	 * again as it would be a native format.  So we have to do the byte swap on the compare.
	 */
	if(byteOrder == OMNativeByteOrder)
	{
		return(memcmp(buf1, buf2, lenSamples * 2) == 0);
	}
	else
	{
		ptr1 = (omfInt16 *)buf1;
		ptr2 = (omfInt16 *)buf2;
		for(n = 0; n < lenSamples; n++)
		{
			val = *ptr2++;
			omfsFixShort(&val);
			if(*ptr1++ != val)
				return(FALSE);
		}
		
		return(TRUE);
	}	
}

static void fillColorBuf(omfUInt8 *pattern)
{
	omfInt32	n, ptnSize;
	
	ptnSize = pattern[0];
	for(n = 0; n < FRAME_BYTES; n++)
	{
		stdColorBuffer[n] = pattern[ (n % ptnSize) + 1 ];
	}
}

static void     InitGlobals(void)
{
	int            n;

	stdElement1.ID = stdUID1;
	stdElement2.ID = stdUID2;
	if (stdWriteBuffer == NULL)
		stdWriteBuffer = (omfUInt8 *) OMMalloc(FRAME_BYTES);
	if (stdColorBuffer == NULL)
		stdColorBuffer = (omfUInt8 *) OMMalloc(FRAME_BYTES);
	if (stdReadBuffer == NULL)
		stdReadBuffer = (omfUInt8 *) OMMalloc(FRAME_BYTES);
	if ((stdWriteBuffer == NULL) || (stdReadBuffer == NULL) || (stdColorBuffer == NULL))
		exit(1);

	for (n = 0; n < FRAME_BYTES; n++)
	{
		stdWriteBuffer[n] = n & 0xFF;
		stdReadBuffer[n] = 0;
	}
}

void            Write1xMedia(char *name, omfBool handleWAVE)
{
	OMObject        head, fileMob;
	omfMobID        fileMobID;
	omfInt32			zeroL = 0;
	omfInt16			zeroS = 0;
	omfInt16			color;
	omfInt32		frameSize;
	mediaType       mt;
	mediaDesc_t     readDesc;
	omfUID_t        readUID;
	int             samplesRead, largest, frameNum;
	
#if PORT_SYS_MAC
	MaxApplZone();
#endif

	InitGlobals();

	check(omfiBeginSession());
	
	head = omfiCreateFile(name);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);

	for(color = 0; color < NUM_COLORS; color++)
	{
		imageStruct.name = TIFF_COMPRESSED_TEST;
		omfmSetMediaDesc(head, imageMedia, (mediaDescPtr) &imageStruct, NULL, &fileMobID);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		fillColorBuf(colorPatterns[color]);
		printf("Writing TIFF Compressed JPEG pattern Data #%d\n", color+1);
		omfmSoftwareCodec(head, TRUE);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		for(frameNum = 1; frameNum <= 4; frameNum++)
		{
			omfmWriteSamples(head, 1, stdColorBuffer, FRAME_BYTES);
			if (OMErrorCode != OM_ERR_NONE)
				FatalErrorCode(__LINE__, __FILE__);
		}
	}


	printf("Extracting TIFF precompressed data for next test\n");
	omfmGetNthMediaDesc(head, 1, &mt, &readDesc, &readUID);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	omfmSoftwareCodec(head, FALSE);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	largest = omfmGetMaxSampleSize(head);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	frameSize = omfmReadSamples(head, 1, stdReadBuffer, largest, &samplesRead);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
   	if(readDesc.image.name != NULL)
	   	omfsFree(readDesc.image.name);
	if(readDesc.image.filename != NULL)
	  	omfsFree(readDesc.image.filename);
  	readDesc.image.name = NULL;
	readDesc.image.filename = NULL;

	/* Create the video fileMob */
	printf("Writing Video (TIFF) Data\n");
	imageStruct.name = TIFF_PRECOMPRESSED_TEST;
	omfmSetMediaDesc(head, imageMedia, (mediaDescPtr) & imageStruct, &stdUID1, &fileMobID);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	omfmSoftwareCodec(head, FALSE);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	for(frameNum = 1; frameNum <= 4; frameNum++)
	{
		omfmWriteSamples(head, 1, stdReadBuffer, frameSize);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
	}
	omfmCleanup(head);
	
	omfsFindSourceMobByID(GetFileHdl(head), fileMobID, &fileMob);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	check(OMPutMOBJLastModified(fileMob, &stdTimeStamp));
	
	/* Create the single-channel AIFC fileMob */
	gForceAIFC = TRUE;
	printf("Writing Audio (AIFC) Data\n");
	audio1Struct.name = AIFC_MONO_TEST;
	omfmSetMediaDesc(head, audioMedia, (mediaDescPtr) & audio1Struct, &stdUID2, &fileMobID);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	omfmWriteSamples(head, FRAME_BYTES/2, stdWriteBuffer, FRAME_BYTES);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	omfmCleanup(head);
	gForceAIFC = FALSE;

	/* Create the 2-channel AIFC fileMob */
	gForceAIFC = TRUE;
	printf("Writing AIFC Stereo Data\n");
	audio3Struct.name = AIFC_STEREO_TEST;
	omfmSetMediaDesc(head, audioMedia, (mediaDescPtr) & audio3Struct, &stdUID4, &fileMobID);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	omfmWriteSamples(head, FRAME_BYTES / 4, stdWriteBuffer, FRAME_BYTES);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	omfmCleanup(head);
	gForceAIFC = FALSE;

	if(handleWAVE)
	{
		/* Create the single-channel WAVE fileMob */
		gForceWAVE = TRUE;
		printf("Writing Audio (WAVE) Data\n");
		audio2Struct.name = WAVE_MONO_TEST;
		omfmSetMediaDesc(head, audioMedia, (mediaDescPtr) & audio2Struct, &stdUID3, &fileMobID);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		omfmWriteSamples(head, FRAME_BYTES / 2, stdWriteBuffer, FRAME_BYTES);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		omfmCleanup(head);
		gForceWAVE = FALSE;
		
		/* Create the Stereo WAVE fileMob */
		gForceWAVE = TRUE;
		printf("Writing WAVE Stereo Data\n");
		audio4Struct.name = WAVE_STEREO_TEST;
		omfmSetMediaDesc(head, audioMedia, (mediaDescPtr) & audio4Struct, &stdUID5, &fileMobID);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		omfmWriteSamples(head, FRAME_BYTES / 4, stdWriteBuffer, FRAME_BYTES);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		omfmCleanup(head);
		gForceWAVE = FALSE;
	}

	
	omfsFree(stdColorBuffer);
	stdColorBuffer = NULL;
	omfsFree(stdWriteBuffer);
	stdWriteBuffer = NULL;
	omfsFree(stdReadBuffer);
	stdReadBuffer = NULL;

	check(omfiCloseFile(head));
	check(omfiEndSession());
}

void            Check1xMedia(char *name, omfBool handleWAVE)
{
	OMObject        head;
	omfInt16        color;
	omfUID_t        readUID;
	int             samplesRead;
	mediaDesc_t     readDesc;
	mediaType       mt;
	int 			largest;
	omfUInt16		byteOrder;
	
#if PORT_SYS_MAC
	int             ticksSpent, compressTicksSpent;
	MaxApplZone();
#endif

	InitGlobals();

	RESET_TIMER(ticksSpent);
	RESET_TIMER(compressTicksSpent);
	START_TIMER(ticksSpent);
	check(omfiBeginSession());
	head = omfiOpenFile(name);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	STOP_TIMER(ticksSpent);
	byteOrder = OMGetDefaultByteOrder(head);
	
	for(color = 0; color < NUM_COLORS; color++)
	{
		imageStruct.name = TIFF_COMPRESSED_TEST;
		fillColorBuf(colorPatterns[color]);
		InitGlobals();
		printf("Reading TIFF Compressed JPEG pattern Data #%d\n", color+1);
		START_TIMER(compressTicksSpent);
		omfmGetNthMediaDesc(head, 1+color, &mt, &readDesc, &readUID);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		omfmSoftwareCodec(head, TRUE);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		omfmReadSamples(head, 1, stdReadBuffer, FRAME_BYTES, &samplesRead);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		if (!PixelCompare(stdColorBuffer, stdReadBuffer, FRAME_BYTES, 3))
			FatalError("Video buffers did not compare\n");
		if(readDesc.image.name != NULL)
			omfsFree(readDesc.image.name);
		if(readDesc.image.filename != NULL)
			omfsFree(readDesc.image.filename);
		readDesc.image.name = NULL;
		readDesc.image.filename = NULL;
		STOP_TIMER(compressTicksSpent);
	}

	/* Check the video fileMob */
	printf("Checking Video (TIFF) Data\n");
	imageStruct.name = TIFF_PRECOMPRESSED_TEST;
	START_TIMER(ticksSpent);
	InitGlobals();
	omfmGetNthMediaDesc(head, NUM_COLORS+1, &mt, &readDesc, &readUID);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	if (mt != imageMedia)
	{
		printf("Wrong media type on video media\n");
		exit(1);
	}
	if ((readDesc.image.format != imageStruct.format) ||
	    (readDesc.image.layout != imageStruct.layout) ||
	    (readDesc.image.width != imageStruct.width) ||
	    (readDesc.image.height != imageStruct.height) ||
	    (readDesc.image.editrate.numerator != imageStruct.editrate.numerator) ||
	    (readDesc.image.editrate.denominator != imageStruct.editrate.denominator) ||
	    (readDesc.image.bitsPerPixel != imageStruct.bitsPerPixel) ||
	    (readDesc.image.compression != imageStruct.compression) ||
	    (readDesc.image.JPEGtableID != imageStruct.JPEGtableID) ||
	    (readDesc.image.leadingLines != imageStruct.leadingLines) ||
	    (readDesc.image.trailingLines != imageStruct.trailingLines) ||
	    ((readDesc.image.name != imageStruct.name) && strcmp(readDesc.image.name, imageStruct.name)) ||
	    ((readDesc.image.filename != imageStruct.filename) && strcmp(readDesc.image.filename, imageStruct.filename)))
		FatalError("Bad image descriptor values\n");
	omfmSoftwareCodec(head, FALSE);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	largest = omfmGetMaxSampleSize(head);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	omfmReadSamples(head, 1, stdReadBuffer, largest, &samplesRead);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	if(readDesc.image.name != NULL)
		omfsFree(readDesc.image.name);
	if(readDesc.image.filename != NULL)
		omfsFree(readDesc.image.filename);
	readDesc.image.name = NULL;
	readDesc.image.filename = NULL;
	STOP_TIMER(ticksSpent);

	/* Check the single-channel AIFC fileMob */
	printf("Checking Audio (AIFC) Data\n");
	audio1Struct.name = AIFC_MONO_TEST;
	START_TIMER(ticksSpent);
	InitGlobals();
	omfmGetNthMediaDesc(head, NUM_COLORS+2, &mt, &readDesc, &readUID);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	if (mt != audioMedia)
		FatalError("Wrong media type on single-channel AIFC\n");
		
	if ((readDesc.audio.editrate.numerator != audio1Struct.editrate.numerator) ||
	    (readDesc.audio.editrate.denominator != audio1Struct.editrate.denominator) ||
	    (readDesc.audio.sampleSize != audio1Struct.sampleSize) ||
	    (readDesc.audio.numChannels != audio1Struct.numChannels) ||
	    (readDesc.audio.compression != audio1Struct.compression) ||
	    ((readDesc.audio.name != audio1Struct.name) && strcmp(readDesc.audio.name, audio1Struct.name)) ||
	    ((readDesc.audio.filename != audio1Struct.filename) && strcmp(readDesc.audio.filename, audio1Struct.filename)))
		FatalError("Bad audio 1 descriptor values\n");
	omfmReadSamples(head, FRAME_BYTES / 2, stdReadBuffer, FRAME_BYTES, &samplesRead);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	if (!SoundCompare(stdWriteBuffer, stdReadBuffer, FRAME_BYTES / 2, byteOrder))
		FatalError("single-channel AIFC buffers did not compare\n");
	if(readDesc.audio.name != NULL)
		omfsFree(readDesc.audio.name);
	if(readDesc.audio.filename != NULL)
		omfsFree(readDesc.audio.filename);
	readDesc.audio.name = NULL;
	readDesc.audio.filename = NULL;
	STOP_TIMER(ticksSpent);

	/* Check the Stereo AIFC fileMob */
	printf("Checking AIFC Stereo Data\n");
	audio3Struct.name = AIFC_STEREO_TEST;
	START_TIMER(ticksSpent);
	InitGlobals();
	omfmGetNthMediaDesc(head, NUM_COLORS+3, &mt, &readDesc, &readUID);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	if (mt != audioMedia)
		FatalError("Wrong media type on 2-channel AIFC\n");
		
	if ((readDesc.audio.editrate.numerator != audio3Struct.editrate.numerator) ||
	    (readDesc.audio.editrate.denominator != audio3Struct.editrate.denominator) ||
	    (readDesc.audio.sampleSize != audio3Struct.sampleSize) ||
	    (readDesc.audio.numChannels != audio3Struct.numChannels) ||
	    (readDesc.audio.compression != audio3Struct.compression) ||
	    ((readDesc.audio.name != audio3Struct.name) && strcmp(readDesc.audio.name, audio3Struct.name)) ||
	    ((readDesc.audio.filename != audio3Struct.filename) && strcmp(readDesc.audio.filename, audio3Struct.filename)))
		FatalError("Bad audio 1 descriptor values\n");
	omfmReadSamples(head, FRAME_BYTES / 4, stdReadBuffer, FRAME_BYTES, &samplesRead);
	if (OMErrorCode != OM_ERR_NONE)
		FatalErrorCode(__LINE__, __FILE__);
	if (!SoundCompare(stdWriteBuffer, stdReadBuffer, FRAME_BYTES / 2, byteOrder))
		FatalError("2-channel AIFC buffers did not compare\n");
	if(readDesc.audio.name != NULL)
		omfsFree(readDesc.audio.name);
	if(readDesc.audio.filename != NULL)
		omfsFree(readDesc.audio.filename);
	readDesc.audio.name = NULL;
	readDesc.audio.filename = NULL;
	STOP_TIMER(ticksSpent);

	if(handleWAVE)
	{
		/* Check the single-channel WAVE fileMob */
		printf("Checking Audio (WAVE) Data\n");
		audio2Struct.name = WAVE_MONO_TEST;
		START_TIMER(ticksSpent);
		InitGlobals();
		omfmGetNthMediaDesc(head, NUM_COLORS+4, &mt, &readDesc, &readUID);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		if (mt != audioMedia)
			FatalError("Wrong media type on single-channel WAVE\n");
		if ((readDesc.audio.editrate.numerator != audio2Struct.editrate.numerator) ||
		    (readDesc.audio.editrate.denominator != audio2Struct.editrate.denominator) ||
		    (readDesc.audio.sampleSize != audio2Struct.sampleSize) ||
		    (readDesc.audio.numChannels != audio2Struct.numChannels) ||
		    (readDesc.audio.compression != audio2Struct.compression) ||
		    ((readDesc.audio.name != audio2Struct.name) && strcmp(readDesc.audio.name, audio2Struct.name)) ||
		    ((readDesc.audio.filename != audio2Struct.filename) && strcmp(readDesc.audio.filename, audio2Struct.filename)))
			FatalError("Bad audio 2 descriptor values\n");
		omfmReadSamples(head, FRAME_BYTES / 2, stdReadBuffer, FRAME_BYTES, &samplesRead);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		if (!SoundCompare(stdWriteBuffer, stdReadBuffer, FRAME_BYTES / 2, byteOrder))
			FatalError("single-channel WAVE buffers did not compare\n");
		if(readDesc.audio.name != NULL)
			omfsFree(readDesc.audio.name);
		if(readDesc.audio.filename != NULL)
			omfsFree(readDesc.audio.filename);
		readDesc.audio.name = NULL;
		readDesc.audio.filename = NULL;
		STOP_TIMER(ticksSpent);
	
		/* Check the Stereo WAVE fileMob */
		printf("Checking WAVE Stereo Data\n");
		audio4Struct.name = WAVE_STEREO_TEST;
		START_TIMER(ticksSpent);
		InitGlobals();
		omfmGetNthMediaDesc(head, NUM_COLORS+5, &mt, &readDesc, &readUID);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		if (mt != audioMedia)
			FatalError("Wrong media type on Stereo WAVE\n");
		if ((readDesc.audio.editrate.numerator != audio4Struct.editrate.numerator) ||
		    (readDesc.audio.editrate.denominator != audio4Struct.editrate.denominator) ||
		    (readDesc.audio.sampleSize != audio4Struct.sampleSize) ||
		    (readDesc.audio.numChannels != audio4Struct.numChannels) ||
		    (readDesc.audio.compression != audio4Struct.compression) ||
		    ((readDesc.audio.name != audio4Struct.name) && strcmp(readDesc.audio.name, audio4Struct.name)) ||
		    ((readDesc.audio.filename != audio4Struct.filename) && strcmp(readDesc.audio.filename, audio4Struct.filename)))
			FatalError("Bad WAVE Stereo descriptor values\n");
		omfmReadSamples(head, FRAME_BYTES / 4, stdReadBuffer, FRAME_BYTES, &samplesRead);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		if (!SoundCompare(stdWriteBuffer, stdReadBuffer, FRAME_BYTES / 2, byteOrder))
			FatalError("Stereo WAVE buffers did not compare\n");
		if(readDesc.audio.name != NULL)
			omfsFree(readDesc.audio.name);
		if(readDesc.audio.filename != NULL)
			omfsFree(readDesc.audio.filename);
		readDesc.audio.name = NULL;
		readDesc.audio.filename = NULL;
		STOP_TIMER(ticksSpent);
	}


	/*
	 * check( OMGetcharacterSet(head, CharSetType *data) );
	 */
	START_TIMER(ticksSpent);
	omfmCleanup(head);
	check(omfiCloseFile(head));
	check(omfiEndSession());
	STOP_TIMER(ticksSpent);
	
	omfsFree(stdColorBuffer);
	stdColorBuffer = NULL;
	omfsFree(stdWriteBuffer);
	stdWriteBuffer = NULL;
	omfsFree(stdReadBuffer);
	stdReadBuffer = NULL;

#if PORT_SYS_MAC
	printf("File checks out OK. (%ld ticks+%ld ticks for Compresses data)\n",
			 ticksSpent, compressTicksSpent);
#endif
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
