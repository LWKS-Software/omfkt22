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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(THINK_C) || defined(__MWERKS__)
#include <Memory.h>
#include <events.h>
#include <Files.h>
#endif
#include <time.h>
#if defined(__MWERKS__) && __profile__
#include <profiler.h>
#define CLOCKS_PER_SECOND	CLOCKS_PER_SEC
#endif

#include "omPublic.h"
#include "omMedia.h"

#include "MkMedia.h"
#include "UnitTest.h"

#define STD_WIDTH		640L
#define STD_HEIGHT		480L
#define FRAME_SAMPLES	(STD_WIDTH * STD_HEIGHT)
#define FRAME_BYTES		(FRAME_SAMPLES * 3L)
#define NUM_FRAMES		50L

#define TIFF_UNCOMPRESSED_TEST	"TIFF Uncompressed"
#define RGBA_TEST				"RGBA"
#define CDCI_TEST				"CDCI"
#define JPEG_RAW_TEST			"JPEG_RAW"
#define JPEG_CMP_TEST			"JPEG_COMPRESSED"
#define AIFC_MONO_TEST			"AIFC_MONO"
#define AIFC_STEREO_RAW_TEST	"AIFC_STEREO_RAW"

static omfUInt8 red[] =	{ 3, 0x80, 0x00, 0x00 };
static omfUInt8 green[] =	{ 3, 0x00, 0x80, 0x00 };
static omfUInt8 blue[] =	{ 3, 0x00, 0x00, 0x80 };

static omfUInt8 *colorPatterns[] = { red, green, blue };
#define NUM_COLORS	(sizeof(colorPatterns) / sizeof(omfUInt8 *))

typedef struct
{
	omfUInt8	*buf;
	omfInt32	bufLen;
} testBuf_t;

static testBuf_t  logoWriteBuffer;
static testBuf_t  videoReadBuffer;

static short motoOrder = MOTOROLA_ORDER, intelOrder = INTEL_ORDER;

static int saveAsVersion1TOC = FALSE;

#define check(a)  { omfErr_t e; e = a; \
			  if (e != OM_ERR_NONE) FatalErrorCode(e, __LINE__, __FILE__); }

static void FatalErrorCode(omfErr_t errcode, int line, char *file)
{
	printf("Error '%s' returned at line %d in %s\n",
	       omfsGetErrorString(errcode), line, file);
	exit(1);
}

static void FatalError(char *message)
{
	printf(message);
	exit(1);
}

/****************/
/* PixelCompare */
/****************/
static omfBool PixelCompare(omfInt16 slop, testBuf_t *buf1, testBuf_t *buf2)
{
	omfInt32	n, diff;
	omfUInt8	*ptr1, *ptr2;
	
	if(buf1->bufLen != buf2->bufLen)
		return(FALSE);
		
	ptr1 = buf1->buf;
	ptr2 = buf2->buf;
	for(n = 0; n < buf1->bufLen; n++)
	{
		diff = *ptr1++ - *ptr2++;
		if((diff < -slop) || (diff > slop))
			return(FALSE);
	}
	
	return(TRUE);		
}

/****************/
/* allocTestBuf */
/****************/
static omfErr_t allocTestBuf(omfInt32 bufsize, testBuf_t *result)
{
	result->buf = omfsMalloc(bufsize);
	if(result->buf != NULL)
	{
		result->bufLen = bufsize;
		return(OM_ERR_NONE);
	}
	else
	{
		result->bufLen = 0;
		return(OM_ERR_NOMEMORY);
	}
}

/****************/
/* freeTestBuf  */
/****************/
static omfErr_t freeTestBuf(testBuf_t *buf)
{
	if(buf->buf != NULL)
		omfsFree(buf->buf);
	return(OM_ERR_NONE);
}

/****************/
/* ClearBuffer  */
/****************/
static omfErr_t ClearBuffer(testBuf_t *buf)
{
	omfInt32	n;
	
	for (n = 0; n < buf->bufLen; n++)
		buf->buf[n] = 0;
	return(OM_ERR_NONE);
}

/****************/
/* FillBufLogo */
/****************/
static void FillBufLogo(testBuf_t *buf, 
						omfInt32 height, 
						omfInt32 width, 
						omfInt32 numFrames)
{
	int 	bit, x, y, n;
	omfUInt8	*dest, *src, val;
	static omfUInt8	writeBits[] =
	{	0x00, 0x00, 0x1F, 0x80, 0x00, 0x00, 0x38, 0xC0,
		0x00, 0x00, 0xE0, 0x40, 0x7F, 0xFF, 0xFF, 0xFE,
		0x40, 0x03, 0x80, 0x42, 0x40, 0x07, 0x00, 0x42,
		0x40, 0x04, 0x01, 0xA2, 0x40, 0x0C, 0x03, 0xB2,
		0x40, 0x18, 0x07, 0xBA, 0x40, 0x10, 0x07, 0xFA,
		0x4F, 0xB8, 0x07, 0xFA, 0x4F, 0xF8, 0x07, 0xFA,
		0x4F, 0xF8, 0x03, 0xF2, 0x4F, 0xF8, 0x00, 0xC2,
		0x4F, 0xF8, 0x00, 0xC2, 0x4F, 0xF8, 0x00, 0x82,
		0x4F, 0xF8, 0x01, 0x82, 0x4F, 0xF0, 0x01, 0x82,
		0x41, 0x00, 0x03, 0x02, 0x41, 0x00, 0x03, 0x02,
		0x41, 0x00, 0x06, 0x02, 0x41, 0x00, 0x66, 0x02,
		0x41, 0x00, 0x6C, 0x02, 0x41, 0x00, 0xF8, 0x02,
		0x41, 0x00, 0xD8, 0x02, 0x41, 0x81, 0xF8, 0x02,
		0x40, 0x83, 0xFC, 0x02, 0x40, 0xC3, 0xFE, 0x02,
		0x40, 0x62, 0x00, 0x02, 0x40, 0x3C, 0x00, 0x02,
		0x40, 0x00, 0x00, 0x02, 0x7F, 0xFF, 0xFF, 0xFE
	};
	
	dest = (omfUInt8 *)buf->buf;
	src = writeBits;
	for(n = 0; n < numFrames; n++)
	{
	  for (y = 0; y < height; y++)
		{
		  for (x = 0; x < width / 8; x++)
			{
			  if(x < (32/8) && y < 32)
				{
				  for (bit = 7; bit >= 0; bit--)
					{
					  val = ((*src >> bit) & 0x01 ? 255 : 0);
					  *dest++ = val;
					  *dest++ = val;
					  *dest++ = val;
					}
				  src++;
				}
			  else
				{
				  for (bit = 7; bit >= 0; bit--)
					{
					  *dest++ = 0;
					  *dest++ = 0;
					  *dest++ = 0;
					}
				}
			}
		}
	}
}

/******************************/
/* WriteTIFFUncompressedMedia */
/******************************/
static omfErr_t WriteTIFFUncompressedMedia(omfHdl_t filePtr, char *filename)
{
	omfRational_t	imageAspectRatio;	
	omfRational_t	videoRate;
	omfObject_t		masterMob, fileMob;
	omfMediaHdl_t	mediaPtr;
	omfInt32		n;
	FILE *file;
	
	XPROTECT(filePtr)
	{
	  printf("Writing TIFF Uncompressed Media\n");
			
	  MakeRational(2997, 100, &videoRate);
	  MakeRational(4, 3, &imageAspectRatio);
		
	  CHECK(omfmMasterMobNew(	filePtr, 					/* file (IN) */
							 TIFF_UNCOMPRESSED_TEST, 	/* name (IN) */
							 TRUE,						/* isPrimary (IN) */
							 &masterMob)); 				/* result (OUT) */
		
	  CHECK(omfmFileMobNew(	filePtr,	 		/* file (IN) */
						   TIFF_UNCOMPRESSED_TEST, 	/* name (IN) */
						   videoRate,   		/* editrate (IN) */
						   CODEC_TIFF_VIDEO, 	/* codec (IN) */
						   &fileMob));   		/* result (OUT) */
	
	  CHECK(omfmVideoMediaCreate(filePtr, masterMob, fileMob, 
								 kToolkitCompressionDisable, 
								 videoRate, STD_HEIGHT, STD_WIDTH, kFullFrame, 
								 imageAspectRatio,
								 &mediaPtr));
							  
	  for(n = 1; n <= NUM_FRAMES; n++)
		{
		  CHECK(omfmWriteDataSamples(mediaPtr, 1, logoWriteBuffer.buf, 
									 logoWriteBuffer.bufLen));
		}
	  CHECK(omfmMediaClose(mediaPtr));
	
	  printf("ReReading data to fill read caches\n");
	  file = fopen(filename, "rb");
	  for(n = 1; n <= NUM_FRAMES; n++)
		{
		  fread(videoReadBuffer.buf, 1, videoReadBuffer.bufLen, file);
		}
	  fclose(file);
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

static omfErr_t CheckTIFFUncompressedOMF(omfHdl_t filePtr, 
										 omfObject_t masterMob, 
										 char *filename, double *overhead)
{
    omfSearchCrit_t	search;
	omfIterHdl_t	trackIter = NULL;
	omfMSlotObj_t	track;
	omfRational_t	editRate;
	omfSegObj_t		segment;
	omfObject_t		fileMob;
	omfInt16		numChannels;
	omfDDefObj_t	pictureKind, soundKind;
	omfErr_t		status;
	omfMediaHdl_t	media;
	omfUInt32		frameBytes, bytesRead;
	clock_t			start, end;
	omfInt32		elapsedOMF, elapsedRaw, cps, offset32, n;
	double			elapsedSec, rate;
	omfBool			isContig;
	omfInt64		offset;
	FILE			*file;
	
	*overhead = 0.0;
	printf("Found the TIFF uncompressed test result\n");

	XPROTECT(filePtr)
	{
		CHECK(ClearBuffer(&videoReadBuffer));
		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureKind, &status);
		CHECK(status);
		omfiDatakindLookup(filePtr, SOUNDKIND, &soundKind, &status);
		CHECK(status);

		search.searchTag = kByDatakind;
		strcpy( (char*) search.tags.datakind, PICTUREKIND);
		check(omfiIteratorAlloc(filePtr, &trackIter));
		CHECK(omfiMobGetNextTrack(trackIter, masterMob, &search, &track));

		CHECK(omfiMobSlotGetInfo(filePtr, track, &editRate, &segment));
		if((editRate.numerator != 2997) || (editRate.denominator != 100))
			FatalError("Bad edit rate on TIFF uncompressed data\n");
		CHECK(omfiSourceClipResolveRef(filePtr, segment, &fileMob));
			
		CHECK(omfmGetNumChannels(filePtr, fileMob, soundKind, &numChannels));
		if(numChannels != 0)
			FatalError("Found audio on video test result\n");
		CHECK(omfmGetNumChannels(filePtr, fileMob, pictureKind, &numChannels));
		if(numChannels == 0)
			FatalError("Missing video on video test result\n");
   	

		/**** Test read using ANSI stream routines
		 */
		CHECK(omfmMediaOpen(filePtr, fileMob, kMediaOpenReadOnly, 
							pictureKind, 0, kToolkitCompressionDisable, 
							&media));
		CHECK(omfmGetMediaFilePos(media, &offset));
		CHECK(omfsTruncInt64toInt32(offset, &offset32));
		CHECK(omfmIsMediaContiguous(media, &isContig));
		CHECK(omfmMediaClose(media));

		if(!isContig)
			FatalError("Can't read using raw handle, not contiguous\n");
		
		file = fopen(filename, "rb");
		if(file == NULL)
			FatalError("Can't open file again for raw read\n");
		
		/* enlarge the I/O buffer  (just like the ANSI file handler)     */
		setvbuf(file, NULL, _IOFBF, 6000);		
		fseek(file, offset32, SEEK_SET);
		
		bytesRead = 0;
		start = clock();
		for(n = 1; n <= NUM_FRAMES; n++)
		{
			bytesRead += fread(videoReadBuffer.buf, 1, videoReadBuffer.bufLen,
							   file);
		}
		end = clock();

		fclose(file);
		elapsedRaw = end - start;
		cps = CLOCKS_PER_SEC;
		elapsedSec = (double)elapsedRaw / (double)cps;
		rate = (double)bytesRead / elapsedSec;
		printf("elapsed %ld (read = %ld)\n", elapsedRaw, (int )bytesRead);
		printf("elapsed time (stdio) = %8.2f seconds (%8.2f bytes per second)\n", 
			   elapsedSec, rate);

		/**** Test read using OMF routines
		 */
		CHECK(omfmMediaOpen(filePtr, fileMob, kMediaOpenReadOnly, pictureKind,
							0, kToolkitCompressionDisable, &media));
		bytesRead = 0;

#if defined(__MWERKS__) && __profile__
		ProfilerSetStatus(TRUE);
#endif
		start = clock();
		for(n = 1; n <= NUM_FRAMES; n++)
		{
			CHECK(omfmReadDataSamples(media, 1, videoReadBuffer.bufLen, 
									  videoReadBuffer.buf, &frameBytes));
			bytesRead += frameBytes;
		}
		end = clock();

#if defined(__MWERKS__) && __profile__
		ProfilerSetStatus(FALSE);
#endif
/*		if (!PixelCompare(0, &logoWriteBuffer, &videoReadBuffer))
			FatalError("Video buffers did not compare\n");
*/		CHECK(omfmMediaClose(media));

		elapsedOMF = end - start;
		cps = CLOCKS_PER_SEC;
		elapsedSec = (double)elapsedOMF / (double)cps;
		rate = (double)bytesRead / elapsedSec;
		
		printf("elapsed time (Through OMF) = %8.2f seconds (%8.2f bytes per second)\n", 
			   elapsedSec, rate);


		if(isContig)
		{
		  *overhead = ((double)elapsedOMF * 100.0 /(double)elapsedRaw) - 100.0;
		  printf("OMF overhead = %8.2f percent\n", *overhead);
		}
		
		if(trackIter != NULL)
		{
			CHECK(omfiIteratorDispose(filePtr, trackIter));
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}


static void BenchmarkWriteOMF(char *name)
{
	omfSessionHdl_t sessionPtr;
	omfHdl_t        filePtr;
#if defined(THINK_C) || defined(__MWERKS__)
	int 		startMem;

	MaxApplZone();
	startMem = FreeMem();
#endif

	check(omfsBeginSession(1, &sessionPtr));
	check(omfmInit(sessionPtr));
	
	check(omfsCreateFile((fileHandleType)name, sessionPtr, 
						 kOmfRev2x, &filePtr));
		
	XPROTECT(filePtr)
	{
		CHECK(allocTestBuf(FRAME_BYTES, &logoWriteBuffer));
		CHECK(allocTestBuf(FRAME_BYTES, &videoReadBuffer));
		FillBufLogo(&logoWriteBuffer, STD_HEIGHT, STD_WIDTH, 1);
		
		CHECK(WriteTIFFUncompressedMedia(filePtr,name));
		
		CHECK(omfsCloseFile(filePtr));
		CHECK(omfsEndSession(sessionPtr));

		CHECK(freeTestBuf(&logoWriteBuffer));
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

static void BenchmarkReadOMF(char *name, double *overhead)
{

	omfSessionHdl_t sessionPtr = NULL;
	omfHdl_t        filePtr = NULL;
	omfObject_t		masterMob = NULL;
	omfIterHdl_t	mobIter = NULL;
	omfIterHdl_t	refIter = NULL;
	omfMediaHdl_t	mediaPtr = NULL;
	char			mobName[64];
	omfSearchCrit_t	search;
	omfErr_t		status;
	omfDDefObj_t	nilKind, pictureKind, soundKind, curKind;
	omfMediaCriteria_t	mediaCrit;
	omfObject_t			fileMob;
	omfInt32			masterTrack;
	omfPosition_t		masterOffset;
	
	check(allocTestBuf(FRAME_BYTES, &logoWriteBuffer));
	check(allocTestBuf(FRAME_BYTES, &videoReadBuffer));
	FillBufLogo(&logoWriteBuffer, STD_HEIGHT, STD_WIDTH, 1);

	check(omfsBeginSession(1, &sessionPtr));
	
	check(omfmInit(sessionPtr));
	
	check(omfsOpenFile((fileHandleType)name, sessionPtr, &filePtr));
	
	omfiDatakindLookup(filePtr, NODATAKIND, &nilKind, &status);
	check(status);
	omfiDatakindLookup(filePtr, PICTUREKIND, &pictureKind, &status);
	check(status);
	omfiDatakindLookup(filePtr, SOUNDKIND, &soundKind, &status);
	check(status);

	check(omfiIteratorAlloc(filePtr, &mobIter));
	search.searchTag = kByMobKind;
	search.tags.mobKind = kMasterMob;

	while (omfiGetNextMob(mobIter, &search, &masterMob) == OM_ERR_NONE)
	{
		check(omfiMobGetInfo(filePtr, masterMob, NULL, sizeof(mobName), 
							 mobName, NULL, NULL));
		curKind = nilKind;
		
		if(strcmp(mobName, TIFF_UNCOMPRESSED_TEST) == 0)
		{
			check(CheckTIFFUncompressedOMF(filePtr, masterMob, name, 
										   overhead));
			curKind = pictureKind;
		}
		else
			printf("Found media from an unknown test\n");
			
		if(curKind != nilKind)
		{
			check(omfiIteratorAlloc(filePtr, &refIter));
			mediaCrit.type = kOmfFastestRepresentation;
			mediaCrit.proc = NULL;
			check(omfmMobGetNextRefFileMob(filePtr, masterMob, curKind, 
										   refIter,
											&mediaCrit, &fileMob, &masterTrack,
											&masterOffset));
			check(omfiIteratorDispose(filePtr, refIter));
		}
	}
	
	if(mobIter != NULL)
	{
		check(omfiIteratorDispose(filePtr, mobIter));
	}
	printf("File checks out OK.\n");

	check(omfsCloseFile(filePtr));
	check(omfsEndSession(sessionPtr));

/*	check(CheckTIFFUncompressedRaw(name, dataOffset));	*/
	
	check(freeTestBuf(&logoWriteBuffer));
	check(freeTestBuf(&videoReadBuffer));
}


#define NUM_PASSES	5

int main(void)
{
	omfInt16	pass;
	double		totalOverhead = 0.0, overhead;
	
#if defined(__MWERKS__) && __profile__
	if(ProfilerInit(collectSummary, bestTimeBase, 10000, 32) != 0)
		return(2);
	ProfilerSetStatus(FALSE);
#endif

	printf("OMFI Read benchmarking\n");
	printf("********* Write Pass\n");
	BenchmarkWriteOMF("jeffBench.omf");

	for(pass = 1; pass <= NUM_PASSES; pass++)
	{
	  printf("********* Read Pass %d of %d\n", pass, NUM_PASSES);
	  BenchmarkReadOMF("jeffBench.omf", &overhead);	/* Validate the result */
	  totalOverhead += overhead;
	}
	printf("******* DONE *******\n");

	printf("Average percent overhead = %8.2f\n", totalOverhead / NUM_PASSES);
	
#if defined(__MWERKS__) && __profile__
	ProfilerDump("\pbenchmark.profile");
	ProfilerTerm();
#endif

	return (0);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/


