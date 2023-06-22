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
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
#include "omMedia.h"

#if PORT_SYS_MAC
#include <Memory.h>
#include <events.h>
#endif

#include "MkMedia.h"
#include "UnitTest.h"
#include "omcTIFF.h"
#include "omcAVR.h"
#if AVID_CODEC_SUPPORT
#include "omcAvJPED.h" 
#include "omcAvidJFIF.h" 
#endif

#define STD_TRACK_ID		1
#define STD_WIDTH		128L
#define STD_HEIGHT		96L
#define FRAME_SAMPLES	(STD_WIDTH * STD_HEIGHT)
#define FRAME_BYTES				(FRAME_SAMPLES * 3L)
#define FRAME_BYTES_YUV10		(FRAME_SAMPLES * 4L)
#define LINE_BYTES		(FRAME_BYTES / STD_HEIGHT)
#define AVRNAME_SIZE 8L
#define BUF_SEGMENTS	3

/* Size of compressed data (below) when decompressed */
#define COMPRESS_FRAME_BYTES	(128L * 96L * 3L)

#define SOUND_BYTES		100000L
#define COLOR_SEQ_LEN		3

static omfUInt8 red[] =			{ 3, 0x80, 0x00, 0x00 };
static omfUInt8 green[] =		{ 3, 0x00, 0x80, 0x00 };
static omfUInt8 blue[] =		{ 3, 0x00, 0x00, 0x80 };
/*static omfUInt8 red[] =			{ 3, 235, 16, 16 };
static omfUInt8 green[] =		{ 3, 16, 235, 16 };
static omfUInt8 blue[] =		{ 3, 16, 16, 235 };
*/

static omfUInt8 *RGBColorPatterns[] = { red, green, blue };
#define NUM_RGB_COLORS	(sizeof(RGBColorPatterns) / sizeof(omfUInt8 *))

static omfUInt8 redYUV[] =		{ 3,  38, 106, 192 };
static omfUInt8 greenYUV[] =	{ 3,  75,  86,  74 };
static omfUInt8 blueYUV[] =		{ 3,  15, 192, 118 };
/*!!!static omfUInt8 redYUV[] =		{ 3,  78, 92, 238 };
static omfUInt8 greenYUV[] =	{ 3,  78,  57,  37 };
static omfUInt8 blueYUV[] =		{ 3,  78, 238, 111 };
*/
static omfUInt8 *YUVColorPatterns[] = { redYUV, greenYUV, blueYUV };
#define NUM_YUV_COLORS	(sizeof(RGBColorPatterns) / sizeof(omfUInt8 *))

/* The above numbers, translated by hand into 10-bit padded YUV
 * in binary are:
 * 	00 0010 0110 00 0110 1010 00 1100 0000 00
 *  00 0100 1011 00 0101 0110 00 0100 1010 00
 *  00 0000 1111 00 1100 0000 00 0111 0110 00
 * which translates into 4 hex bytes as follows:
 */
static omfUInt8 redYUV10[] =		{ 4, 0x09, 0x86, 0xa3, 0x00 };
static omfUInt8 greenYUV10[] =		{ 4, 0x12, 0xc5, 0x61, 0x28 };
static omfUInt8 blueYUV10[] =		{ 4, 0x03, 0xcc, 0x01, 0xd8 };
static omfUInt8 *YUV10ColorPatterns[] = { redYUV10, greenYUV10, blueYUV10 };
#define NUM_YUV10_COLORS	(sizeof(YUV10ColorPatterns) / sizeof(omfUInt8 *))

static omfUInt8 compressedJFIF[] = 
{
	0xFF,0xD8,0xFF,0xE0,0x00,0x10,0x4A,0x46,0x49,0x46,0x00,0x01,0x01,0x00,0x00,0x01,
	0x00,0x01,0x00,0x00,0xFF,0xDB,0x00,0x43,0x00,0x08,0x06,0x06,0x07,0x06,0x05,0x08,
	0x07,0x07,0x07,0x09,0x09,0x08,0x0A,0x0C,0x14,0x0D,0x0C,0x0B,0x0B,0x0C,0x19,0x12,
	0x13,0x0F,0x14,0x1D,0x1A,0x1F,0x1E,0x1D,0x1A,0x1C,0x1C,0x20,0x24,0x2E,0x27,0x20,
	0x22,0x2C,0x23,0x1C,0x1C,0x28,0x37,0x29,0x2C,0x30,0x31,0x34,0x34,0x34,0x1F,0x27,
	0x39,0x3D,0x38,0x32,0x3C,0x2E,0x33,0x34,0x32,0xFF,0xDB,0x00,0x43,0x01,0x09,0x09,
	0x09,0x0C,0x0B,0x0C,0x18,0x0D,0x0D,0x18,0x32,0x21,0x1C,0x21,0x32,0x32,0x32,0x32,
	0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
	0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
	0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0xFF,0xC0,
	0x00,0x11,0x08,0x00,0x60,0x00,0x80,0x03,0x01,0x22,0x00,0x02,0x11,0x01,0x03,0x11,
	0x01,0xFF,0xC4,0x00,0x1F,0x00,0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
	0x0A,0x0B,0xFF,0xC4,0x00,0xB5,0x10,0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,
	0x05,0x04,0x04,0x00,0x00,0x01,0x7D,0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,
	0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,
	0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,
	0x18,0x19,0x1A,0x25,0x26,0x27,0x28,0x29,0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,
	0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,
	0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,
	0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,
	0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,
	0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,
	0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,
	0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFF,0xC4,0x00,0x1F,0x01,0x00,0x03,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
	0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0xFF,0xC4,0x00,0xB5,0x11,0x00,
	0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,0x02,0x77,0x00,
	0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,
	0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,0xF0,0x15,
	0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,0x26,0x27,
	0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
	0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
	0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,0x86,0x87,0x88,
	0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,
	0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,
	0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE2,
	0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,
	0xFA,0xFF,0xDA,0x00,0x0C,0x03,0x01,0x00,0x02,0x11,0x03,0x11,0x00,0x3F,0x00,0x28,
	0xA2,0x8A,0xFC,0x34,0xFC,0x34,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,0x8A,0x00,0x28,0xA2,
	0x8A,0x00,0xFF,0xD9
};

static omfUInt8 compressedTIFF[] = 
{
	0xE4,0xA8,0xAE,0x53,0xDE,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,
	0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,
	0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,
	0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,
	0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,
	0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,
	0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,
	0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,
	0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,
	0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,
	0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,
	0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,
	0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,
	0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,
	0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,0x0A,0x28,0x00,0xA2,0x80,
	0x0A,0x28,0x03,0xFF,0xD0
};

static testBuf_t           logoWriteBuffer;
static testBuf_t           RGBColorWriteBuffer;
static testBuf_t           YUVColorWriteBuffer;
static testBuf_t           RGBsingleFrameColorBuf;
static testBuf_t           JFIFWriteBuffer;
static testBuf_t           JPEGTIFFWriteBuffer;
static testBuf_t           JFIFReadBuffer;
static testBuf_t           JPEGTIFFReadBuffer;
static testBuf_t           colorReadBuffer;
static testBuf_t           soundWriteBuffer;
static testBuf_t           sndLeftWriteBuffer;
static testBuf_t           sndRightWriteBuffer;
static testBuf_t           sndLeftReadBuffer;
static testBuf_t           sndRightReadBuffer;
static testBuf_t           videoReadBuffer;
static testBuf_t           audioReadBuffer;

static testBuf_t           colorWriteBufferYUV10;
static testBuf_t           videoReadBufferYUV10;

static omfTIFF_JPEGInfo_t	tiffPrivate;
static omfVideoMemOp_t	stdVideoFmt[2], rgbVideoMemFmt[4], yuvVideoMemFmt[2], mixedFieldFmt[2], separateFieldFmt[2];
static omfVideoMemOp_t	fullFrameFmt[3], singleFieldFmt[2], yuvFileFmt[4], yuvFileFmtSubsample[4];
static omfVideoMemOp_t	yuvFileFmtMixed[5], yuvFileFmtSeparate[5], yuvFileFmt10[5], yuvFileFmtPad10[5];
static omfVideoMemOp_t	fullFrameMemFmt[3], separateFieldFileFmt[4];
static omfAudioMemOp_t	byteAudioMemFmt[2];

static short        motoOrder = MOTOROLA_ORDER, intelOrder = INTEL_ORDER;

static int saveAsVersion1TOC = FALSE;

#define FatalErrorCode(ec, line, file) { printf("Error '%s' returned at line %d in %s\n", \
				omfsGetErrorString(ec), line, file); \
				RAISE(OM_ERR_TEST_FAILED); }

#define FatalError(msg) { printf(msg); RAISE(OM_ERR_TEST_FAILED); }

static omfErr_t allocTestBuf(omfInt32 bufsize, testBuf_t *result);
static omfErr_t freeTestBuf(testBuf_t *buf);
static void     FillBufLogo(testBuf_t *buf, omfInt32 height, omfInt32 width);
static void fillColorBuf(omfUInt8 **patternList, omfInt32 numFrames,
						omfInt32 frameBytes, testBuf_t *buf);

#define MAX_STD_TESTS		512

static omfErr_t addTestsForCodec(omfCodecID_t codecID,
								char				*mediaKindStr, 
								omfInt32 			maxTests,
								omfPixelFormat_t	nativePixelFormat,
								omfBool				hasCompression,
                        void              *privateData,
								mediaTestDef_t 		*result,
								omfInt32 			*numTestsPtr)
{
	omfInt32	cnt = *numTestsPtr;
   avidAVRdata_t *avrData = (avidAVRdata_t *)privateData;
	avidAVRdata_t *privateAVR;
	omfInt32 avrSize;
	
	/********/
	XPROTECT(NULL)
	{
		/* !!! Remove the codec-specific checks where possible;	*/
		if(strcmp(mediaKindStr, PICTUREKIND) == 0)
		{
		  if (strcmp(codecID, CODEC_AJPG_VIDEO) == 0)
			 {
				privateAVR = omfsMalloc(sizeof(avidAVRdata_t));
				result[cnt].codecPrivateData = privateAVR;
				(*privateAVR).product = (*avrData).product;
				avrSize = sizeof((*avrData).avr);
				(*privateAVR).avr = omfsMalloc(avrSize);
				strcpy( (char*) (*privateAVR).avr, (*avrData).avr);
				result[cnt].codecPrivateDataLen = sizeof(avidAVRdata_t) + avrSize;
			 } /* private AVR data */

			if(nativePixelFormat == kOmfPixRGBA)
			{
			  /* Set up private data if AJPG */
				if(hasCompression)
				{
					/********/
				   /* Compresses (if possible) RGB and writes, decompresses 
					 * (if necessary) and compares uncompressed 
					 */
					result[cnt].testName = "RGB Native Compressed media";
					result[cnt].writeXferType = kSingleChannel;
					result[cnt].readXferType = kSingleChannel;
					result[cnt].codecID = codecID;
					result[cnt].mediaKind = PICTUREKIND;
					result[cnt].numSamples = NUM_RGB_COLORS; 	
					result[cnt].numChannels = 1;
					result[cnt].videoWriteMemFormat = NULL;
					result[cnt].videoWriteFileFormat = NULL;
					result[cnt].videoReadMemFormat = NULL;
					result[cnt].writeBuf = &RGBColorWriteBuffer;
					result[cnt].readBuf = &colorReadBuffer;
					result[cnt].checkBuf = &RGBColorWriteBuffer;
					result[cnt].writeCompress = kToolkitCompressionEnable;
					result[cnt].readCompress = kToolkitCompressionEnable;
					if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
					  result[cnt].codecPrivateData = NULL;
					result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 1;

					XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)
				} /* hasCompression && RGBA */
				else
				{
					/********/
				   /* Writes RGB uncompressed (non-TIFF codec) as lines,
	            	 * reads back as lines, and compares against expected RGB
					 */
					result[cnt].testName = "Line Mode Video";
					result[cnt].writeXferType = kLineXfer;
					result[cnt].readXferType = kLineXfer;
					result[cnt].codecID = codecID;
					result[cnt].mediaKind = PICTUREKIND;
					result[cnt].numSamples = 1;			/* Only write 1 sample */
					result[cnt].numChannels = 1;
					result[cnt].videoWriteMemFormat = NULL;
					result[cnt].videoWriteFileFormat = NULL;
					result[cnt].videoReadMemFormat = NULL;
					result[cnt].writeBuf = &logoWriteBuffer;
					result[cnt].readBuf = &videoReadBuffer;
					result[cnt].checkBuf = &logoWriteBuffer;
					result[cnt].writeCompress = kToolkitCompressionDisable;
					result[cnt].readCompress = kToolkitCompressionDisable;
					result[cnt].codecPrivateData = NULL;
					result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 1;
					XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)
				 } /* No compression */


				/* Compresses (if possible) RGB and writes as single field, decompresses 
				 * (if necessary) and compares uncompressed 
				 */
				  result[cnt].testName = "Uniform RGB Media (single field)";
				  result[cnt].writeXferType = kSingleChannel;
				  result[cnt].readXferType = kSingleChannel;
				  result[cnt].codecID = codecID;
				  result[cnt].mediaKind = PICTUREKIND;
				  result[cnt].numSamples = NUM_RGB_COLORS;
				  result[cnt].numChannels = 1;
				  result[cnt].videoWriteMemFormat = NULL;
				  result[cnt].videoWriteFileFormat = singleFieldFmt;
				  result[cnt].videoReadMemFormat = NULL;
				  result[cnt].writeBuf = &RGBColorWriteBuffer;
				  result[cnt].readBuf = &colorReadBuffer;
				  result[cnt].checkBuf = &RGBColorWriteBuffer;
				  result[cnt].writeCompress = kToolkitCompressionEnable;
				  result[cnt].readCompress = kToolkitCompressionEnable;
				  if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
					 result[cnt].codecPrivateData = NULL;
				  result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 1;
				  XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY);

			  	if (strcmp(codecID, CODEC_TIFF_VIDEO) != 0)		/* if Not TIFF */
		  		{
					/* Compresses (if possible) RGB and writes as mixed fields, decompresses 
					 * (if necessary) and compares uncompressed 
					 */
				  result[cnt].testName = "Uniform RGB Media (mixed fields)";
				  result[cnt].writeXferType = kSingleChannel;
				  result[cnt].readXferType = kSingleChannelChunks;
				  result[cnt].codecID = codecID;
				  result[cnt].mediaKind = PICTUREKIND;
				  result[cnt].numSamples = NUM_RGB_COLORS;
				  result[cnt].numChannels = 1;
				  result[cnt].videoWriteMemFormat = NULL;
				  result[cnt].videoWriteFileFormat = mixedFieldFmt;
				  result[cnt].videoReadMemFormat = NULL;
				  result[cnt].writeBuf = &RGBColorWriteBuffer;
				  result[cnt].readBuf = &colorReadBuffer;
				  result[cnt].checkBuf = &RGBColorWriteBuffer;
				  result[cnt].writeCompress = kToolkitCompressionEnable;
				  result[cnt].readCompress = kToolkitCompressionEnable;
				  if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
					 result[cnt].codecPrivateData = NULL;
				  result[cnt].frameVary = kFramesSameSize;
				result[cnt].alignment = 1;
				  XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY);
	
					/* Compresses (if possible) RGB and writes as separate fields, decompresses 
					 * (if necessary) and compares uncompressed 
					 */
				  result[cnt].testName = "Uniform RGB Media (separate fields)";
				  result[cnt].writeXferType = kSingleChannel;
				  result[cnt].readXferType = kSingleChannelChunks;
				  result[cnt].codecID = codecID;
				  result[cnt].mediaKind = PICTUREKIND;
				  result[cnt].numSamples = NUM_RGB_COLORS;
				  result[cnt].numChannels = 1;
				  result[cnt].videoWriteMemFormat = NULL;
				  result[cnt].videoWriteFileFormat = separateFieldFmt;
				  result[cnt].videoReadMemFormat = NULL;
				  result[cnt].writeBuf = &RGBColorWriteBuffer;
				  result[cnt].readBuf = &colorReadBuffer;
				  result[cnt].checkBuf = &RGBColorWriteBuffer;
				  result[cnt].writeCompress = kToolkitCompressionEnable;
				  result[cnt].readCompress = kToolkitCompressionEnable;
				  if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
					 result[cnt].codecPrivateData = NULL;
				  result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 1;
				  XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY);
				}
				
				/* Compresses (if possible) RGB and writes, seeks, decompresses 
				 * (if necessary) and compares uncompressed 
				 */
			  result[cnt].testName = "Uniform RGB Media with seeks";
			  result[cnt].writeXferType = kSingleChannel;
			  result[cnt].readXferType = kSingleChannelChunks;
			  result[cnt].codecID = codecID;
			  result[cnt].mediaKind = PICTUREKIND;
			  result[cnt].numSamples = NUM_RGB_COLORS;
			  result[cnt].numChannels = 1;
			  result[cnt].videoWriteMemFormat = NULL;
			  result[cnt].videoWriteFileFormat = stdVideoFmt;
			  result[cnt].videoReadMemFormat = NULL;
			  result[cnt].writeBuf = &RGBColorWriteBuffer;
			  result[cnt].readBuf = &colorReadBuffer;
			  result[cnt].checkBuf = &RGBColorWriteBuffer;
			  result[cnt].writeCompress = kToolkitCompressionEnable;
			  result[cnt].readCompress = kToolkitCompressionEnable;
			  if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				 result[cnt].codecPrivateData = NULL;
			  result[cnt].frameVary = kFramesSameSize;
			result[cnt].alignment = 1;
			  XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY);


				/* Compresses (if possible) RGB full frame and writes, 
				 * decompresses (if necessary) and converts to single field
				 * and compares uncompressed 
				 */
				result[cnt].testName = "Single field memory & full frame file format";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannel;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = 1;
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = singleFieldFmt;
				result[cnt].videoWriteFileFormat = fullFrameFmt;
				result[cnt].videoReadMemFormat = singleFieldFmt;
				result[cnt].writeBuf = &logoWriteBuffer;
				result[cnt].readBuf = &videoReadBuffer;
				result[cnt].checkBuf = &logoWriteBuffer;
				result[cnt].writeCompress = kToolkitCompressionDisable;
				result[cnt].readCompress = kToolkitCompressionDisable;
				if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				  result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = (hasCompression ? 
												 kFramesDifferentSize : kFramesSameSize);
					result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

				/* Compresses (if possible) RGB single field and writes, 
				 * decompresses (if necessary) and converts to full frame
				 * and compares uncompressed 
				 */
				result[cnt].testName = "full frame memory & separate fields file format";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannel;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = 1;
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = fullFrameMemFmt;
				result[cnt].videoWriteFileFormat = separateFieldFileFmt;
				result[cnt].videoReadMemFormat = fullFrameMemFmt;
				result[cnt].writeBuf = &logoWriteBuffer;
				result[cnt].readBuf = &videoReadBuffer;
				result[cnt].checkBuf = &logoWriteBuffer;
				result[cnt].writeCompress = kToolkitCompressionDisable;
				result[cnt].readCompress = kToolkitCompressionDisable;
				if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				  result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = (hasCompression ? 
												 kFramesDifferentSize : kFramesSameSize);
					result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

			  	if (strcmp(codecID, CODEC_RGBA_VIDEO) == 0)		/* if RGBA */
				{
					/********/
				   /* Writes RGB uncompressed (non-TIFF codec) ,
	            	 * reads back, and compares against expected RGB (w/alignment)(
					 */
					result[cnt].testName = "Uniform RGB Media with alignment";
					result[cnt].writeXferType = kSingleChannel;
					result[cnt].readXferType = kSingleChannel;
					result[cnt].codecID = codecID;
					result[cnt].mediaKind = PICTUREKIND;
					result[cnt].numSamples = 1;			/* Only write 1 sample */
					result[cnt].numChannels = 1;
					result[cnt].videoWriteMemFormat = NULL;
					result[cnt].videoWriteFileFormat = NULL;
					result[cnt].videoReadMemFormat = NULL;
					result[cnt].writeBuf = &logoWriteBuffer;
					result[cnt].readBuf = &videoReadBuffer;
					result[cnt].checkBuf = &logoWriteBuffer;
					result[cnt].writeCompress = kToolkitCompressionDisable;
					result[cnt].readCompress = kToolkitCompressionDisable;
					result[cnt].codecPrivateData = NULL;
					result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 4096;
					XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

					/* Compresses (if possible) RGB and writes, seeks, decompresses 
					 * (if necessary) and compares uncompressed 
					 */
				  	result[cnt].testName = "Uniform RGB Media with seeks & alignment";
				  	result[cnt].writeXferType = kSingleChannel;
				  	result[cnt].readXferType = kSingleChannelChunks;
				  	result[cnt].codecID = codecID;
				  	result[cnt].mediaKind = PICTUREKIND;
				  	result[cnt].numSamples = NUM_RGB_COLORS;
				  	result[cnt].numChannels = 1;
				  	result[cnt].videoWriteMemFormat = NULL;
				  	result[cnt].videoWriteFileFormat = stdVideoFmt;
				  	result[cnt].videoReadMemFormat = NULL;
				  	result[cnt].writeBuf = &RGBColorWriteBuffer;
				  	result[cnt].readBuf = &colorReadBuffer;
				  	result[cnt].checkBuf = &RGBColorWriteBuffer;
				 	result[cnt].writeCompress = kToolkitCompressionEnable;
				  	result[cnt].readCompress = kToolkitCompressionEnable;
				  	if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
					 	result[cnt].codecPrivateData = NULL;
				  	result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 4096;
				  	XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY);
				}
			 } /* If not Avid JPEG */
			
			if(nativePixelFormat == kOmfPixYUV)
			{
				/********/
			   /* Compresses (if possible) YUV and writes,
             * decompresses (if necessary) and compares uncompressed
             */
				result[cnt].testName = "YUV Native Media";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannel;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = NUM_YUV_COLORS;
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = NULL;
				result[cnt].videoWriteFileFormat = yuvFileFmt;
				result[cnt].videoReadMemFormat = NULL;
				result[cnt].writeBuf = &YUVColorWriteBuffer;
				result[cnt].readBuf = &colorReadBuffer;
				result[cnt].checkBuf = &YUVColorWriteBuffer;
				result[cnt].writeCompress = kToolkitCompressionEnable;
				result[cnt].readCompress = kToolkitCompressionEnable;
				if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				  result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)
			} /* if YUV and no compression */
				
			if(nativePixelFormat == kOmfPixYUV && !hasCompression)
			{
            	/* Compresses (if possible) YUV and writes, seeks,
            	 * decompresses (if necessary) and compares uncompressed
            	 */
				result[cnt].testName = "Uniform YUV Media with seeks (4:4:4)";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannelChunks;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = NUM_RGB_COLORS;
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = NULL;
				result[cnt].videoWriteFileFormat = yuvFileFmt;
				result[cnt].videoReadMemFormat = NULL;
				result[cnt].writeBuf = &YUVColorWriteBuffer;
				result[cnt].readBuf = &colorReadBuffer;
				result[cnt].checkBuf = &YUVColorWriteBuffer;
				result[cnt].writeCompress = kToolkitCompressionEnable;
				result[cnt].readCompress = kToolkitCompressionEnable;
				if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = kFramesSameSize;
				result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

             	/* Compresses (if possible) YUV and writes, seeks,
            	 * decompresses (if necessary) and compares uncompressed
            	 */
				result[cnt].testName = "Uniform YUV Media with alignment (YUV 4:4:4)";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannelChunks;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = NUM_RGB_COLORS;
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = NULL;
				result[cnt].videoWriteFileFormat = yuvFileFmt;
				result[cnt].videoReadMemFormat = NULL;
				result[cnt].writeBuf = &YUVColorWriteBuffer;
				result[cnt].readBuf = &colorReadBuffer;
				result[cnt].checkBuf = &YUVColorWriteBuffer;
				result[cnt].writeCompress = kToolkitCompressionEnable;
				result[cnt].readCompress = kToolkitCompressionEnable;
				if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = kFramesSameSize;
				result[cnt].alignment = 4096;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

            	/* Compresses (if possible) YUV and writes, seeks,
            	 * decompresses (if necessary) and compares uncompressed (w/alignment)
            	 */
				result[cnt].testName = "Uniform YUV Media with seeks & alignment(YUV 4:4:4 )";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannelChunks;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = NUM_RGB_COLORS;
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = NULL;
				result[cnt].videoWriteFileFormat = yuvFileFmt;
				result[cnt].videoReadMemFormat = NULL;
				result[cnt].writeBuf = &YUVColorWriteBuffer;
				result[cnt].readBuf = &colorReadBuffer;
				result[cnt].checkBuf = &YUVColorWriteBuffer;
				result[cnt].writeCompress = kToolkitCompressionEnable;
				result[cnt].readCompress = kToolkitCompressionEnable;
				if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = kFramesSameSize;
				result[cnt].alignment = 4096;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

           	/* Compresses (if possible) YUV and writes as separateFields,
            	 * decompresses (if necessary) and compares uncompressed
            	 */
				result[cnt].testName = "Uniform YUV Media (Separate Fields, YUV 4:4:4)";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannelChunks;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = NUM_RGB_COLORS;
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = NULL;
				result[cnt].videoWriteFileFormat = yuvFileFmtSeparate;
				result[cnt].videoReadMemFormat = NULL;
				result[cnt].writeBuf = &YUVColorWriteBuffer;
				result[cnt].readBuf = &colorReadBuffer;
				result[cnt].checkBuf = &YUVColorWriteBuffer;
				result[cnt].writeCompress = kToolkitCompressionEnable;
				result[cnt].readCompress = kToolkitCompressionEnable;
				if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				  result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

            	/* Compresses (if possible) YUV and writes as separateFields,
            	 * decompresses (if necessary) and compares uncompressed
            	 */
				result[cnt].testName = "Uniform YUV Media (Mixed Fields, YUV 4:4:4)";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannelChunks;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = NUM_RGB_COLORS;
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = NULL;
				result[cnt].videoWriteFileFormat = yuvFileFmtMixed;
				result[cnt].videoReadMemFormat = NULL;
				result[cnt].writeBuf = &YUVColorWriteBuffer;
				result[cnt].readBuf = &colorReadBuffer;
				result[cnt].checkBuf = &YUVColorWriteBuffer;
				result[cnt].writeCompress = kToolkitCompressionEnable;
				result[cnt].readCompress = kToolkitCompressionEnable;
				if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				  result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

				/********/
			   /* Writes YUV uncompressed as lines,
            	 * reads back as lines, and compares against expected YUV
				 */
				result[cnt].testName = "Line Mode Video (YUV 4:4:4)";
				result[cnt].writeXferType = kLineXfer;
				result[cnt].readXferType = kLineXfer;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = 1;			/* Only write 1 sample */
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = NULL;
				result[cnt].videoWriteFileFormat = yuvFileFmt;
				result[cnt].videoReadMemFormat = NULL;
				result[cnt].writeBuf = &logoWriteBuffer;
				result[cnt].readBuf = &videoReadBuffer;
				result[cnt].checkBuf = &logoWriteBuffer;
				result[cnt].writeCompress = kToolkitCompressionDisable;
				result[cnt].readCompress = kToolkitCompressionDisable;
				result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

				/* !!! We need a 10-bit packed test, but first a fill-color-bytes
				 * which can pack the components in any size field is needed.  The
				 * result will be useful for RGBA too.
				 */
				 
				/********/
			   /* Writes YUV 10-bit per component 4:4:4 padded to 32-bits
            	 * reads back and compares against expected YUV
				 */
				result[cnt].testName = "10-bit per component 4:4:4 Padded to 32";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannel;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = 3;			/* Only write 3 samples */
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = NULL;
				result[cnt].videoWriteFileFormat = yuvFileFmtPad10;
				result[cnt].videoReadMemFormat = NULL;
				result[cnt].writeBuf = &colorWriteBufferYUV10;
				result[cnt].readBuf = &videoReadBufferYUV10;
				result[cnt].checkBuf = &colorWriteBufferYUV10;
				result[cnt].writeCompress = kToolkitCompressionDisable;
				result[cnt].readCompress = kToolkitCompressionDisable;
				result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)
			} /* if YUV and no compression, seek test */
				
#if 0
		/*/// !!!!Should be able to put this back */
		  /********/
        /* Compresses (optional) YUV and writes, decompresses (optional)
         * into RGB format, and compares result to expected RGB
         */
			result[cnt].testName = "Write YUV Read RGB";
			result[cnt].writeXferType = kSingleChannel;
			result[cnt].readXferType = kSingleChannel;
			result[cnt].codecID = codecID;
			result[cnt].mediaKind = PICTUREKIND;
			result[cnt].numSamples = NUM_RGB_COLORS; 	
			result[cnt].numChannels = 1;
			result[cnt].videoWriteMemFormat = yuvVideoMemFmt;
			if(strcmp(codecID, CODEC_JPEG_VIDEO) == 0)
				result[cnt].videoWriteFileFormat = yuvFileFmt;
#if AVID_CODEC_SUPPORT
			else if(strcmp(codecID, CODEC_AVID_JFIF_VIDEO) == 0)
				result[cnt].videoWriteFileFormat = yuvFileFmt;
#endif
			else
				result[cnt].videoWriteFileFormat = NULL;		
			result[cnt].videoReadMemFormat = rgbVideoMemFmt;
			result[cnt].writeBuf = &YUVColorWriteBuffer;
			result[cnt].readBuf = &colorReadBuffer;
			result[cnt].checkBuf = &RGBColorWriteBuffer;
			result[cnt].writeCompress = kToolkitCompressionEnable;
			result[cnt].readCompress = kToolkitCompressionEnable;
			if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
			  result[cnt].codecPrivateData = NULL;
			result[cnt].frameVary = kFramesSameSize;
				result[cnt].alignment = 1;
			XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)
#endif

		  /********/
        /* Compresses (optional) RGB and writes, decompresses (optional)
         * into YUV format, and compares result to expected YUV
         */
			if ((strcmp(codecID, CODEC_RGBA_VIDEO) != 0)		/* problems w/RGBA codec here!!! */
			 && (strcmp(codecID, CODEC_CDCI_VIDEO) != 0)	/* problems w/CDCI codec here!!! */
#if AVID_CODEC_SUPPORT
			 && (strcmp(codecID, CODEC_AVID_JPEG_VIDEO) != 0)
#endif
		  	)
		  	{												/* Looks like checkbuf being corrupted */
				result[cnt].testName = "Write RGB Read YUV";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannel;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = NUM_RGB_COLORS; 	
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = rgbVideoMemFmt;
				if(strcmp(codecID, CODEC_JPEG_VIDEO) == 0)
					result[cnt].videoWriteFileFormat = yuvFileFmt;
#if AVID_CODEC_SUPPORT
				else if((strcmp(codecID, CODEC_AVID_JFIF_VIDEO) == 0) || 
						(strcmp(codecID, CODEC_AVID_JPEG_VIDEO) == 0))
					result[cnt].videoWriteFileFormat = yuvFileFmt;
#endif
				else
					result[cnt].videoWriteFileFormat = NULL;		
				result[cnt].videoReadMemFormat = yuvVideoMemFmt;
				result[cnt].writeBuf = &RGBColorWriteBuffer;
				result[cnt].readBuf = &colorReadBuffer;
				result[cnt].checkBuf = &YUVColorWriteBuffer;
				result[cnt].writeCompress = kToolkitCompressionEnable;
				result[cnt].readCompress = kToolkitCompressionEnable;
				if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				  result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = kFramesSameSize;
					result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY);
			}

			if(strcmp(codecID, CODEC_JPEG_VIDEO) == 0)
			{
			   /* Writes pre-compressed YUV, reads back raw, and compares 
				 * compressed 
				 */
				 result[cnt].testName = "Precompressed YUV Media";
				 result[cnt].writeXferType = kSingleChannel;
				 result[cnt].readXferType = kSingleChannel;
				 result[cnt].codecID = codecID;
				 result[cnt].mediaKind = PICTUREKIND;
				 result[cnt].numSamples = 1;
				 result[cnt].numChannels = 1;
				 result[cnt].videoWriteMemFormat = NULL;
				 result[cnt].videoWriteFileFormat = yuvFileFmt;
				 result[cnt].videoReadMemFormat = NULL;
				 result[cnt].writeBuf = &JFIFWriteBuffer;
				 result[cnt].readBuf = &JFIFReadBuffer;
				 result[cnt].checkBuf = &JFIFWriteBuffer;
				 result[cnt].writeCompress = kToolkitCompressionDisable;
				 result[cnt].readCompress = kToolkitCompressionDisable;
				 if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
					result[cnt].codecPrivateData = NULL;
				 result[cnt].frameVary = kFramesDifferentSize;
				result[cnt].alignment = 1;
				 XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

				/********/
           /* Writes pre-compressed data (JFIF - JPEG codec ONLY), 
            * decompresses, and compares against expected RGB
				 */
				result[cnt].testName = "Compressed Video (Write raw, read decompress)";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannel;
				result[cnt].codecID = codecID;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = 1;			/* Only write 1 sample */
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = NULL;
				result[cnt].videoWriteFileFormat = yuvFileFmt;
				result[cnt].videoReadMemFormat = NULL;
				result[cnt].writeBuf = &JFIFWriteBuffer;
				result[cnt].readBuf = &videoReadBuffer;
				result[cnt].checkBuf = &RGBsingleFrameColorBuf;
				result[cnt].writeCompress = kToolkitCompressionDisable;
				result[cnt].readCompress = kToolkitCompressionEnable;
				if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				  result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = kFramesSameSize;
				result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)
			}
				
			if(strcmp(codecID, CODEC_TIFF_VIDEO) == 0)
			{
			   /* Writes pre-compressed RGB, reads back raw, and compares 
				 * compressed 
				 */
				 result[cnt].testName = "Precompressed RGB Media";
				 result[cnt].writeXferType = kSingleChannel;
				 result[cnt].readXferType = kSingleChannel;
				 result[cnt].codecID = codecID;
				 result[cnt].mediaKind = PICTUREKIND;
				 result[cnt].numSamples = 1;
				 result[cnt].numChannels = 1;
				 result[cnt].videoWriteMemFormat = NULL;
				 result[cnt].videoWriteFileFormat = stdVideoFmt;
				 result[cnt].videoReadMemFormat = NULL;
				 result[cnt].writeBuf = &JPEGTIFFWriteBuffer;
				 result[cnt].readBuf = &JPEGTIFFReadBuffer;
				 result[cnt].checkBuf = &JPEGTIFFWriteBuffer;
				 result[cnt].writeCompress = kToolkitCompressionDisable;
				 result[cnt].readCompress = kToolkitCompressionDisable;
				result[cnt].codecPrivateData = &tiffPrivate;
				result[cnt].codecPrivateDataLen = sizeof(omfTIFF_JPEGInfo_t);
				 result[cnt].frameVary = kFramesDifferentSize;
				result[cnt].alignment = 1;
				 XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

				/********/
           /* Writes pre-compressed data (JPEG - TIFF codec ONLY), 
            * decompresses, and compares against expected RGB
				 */
				result[cnt].testName = "Compressed Video (Write raw, read decompress)";
				result[cnt].writeXferType = kSingleChannel;
				result[cnt].readXferType = kSingleChannel;
				result[cnt].codecID = CODEC_TIFF_VIDEO;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = 1;			/* Only write 1 sample */
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = NULL;
				result[cnt].videoWriteFileFormat = NULL;
				result[cnt].videoReadMemFormat = NULL;
				result[cnt].writeBuf = &JPEGTIFFWriteBuffer;
				result[cnt].readBuf = &videoReadBuffer;
				result[cnt].checkBuf = &RGBsingleFrameColorBuf;
				result[cnt].writeCompress = kToolkitCompressionDisable;
				result[cnt].readCompress = kToolkitCompressionEnable;
				result[cnt].codecPrivateData = &tiffPrivate;
				result[cnt].codecPrivateDataLen = sizeof(omfTIFF_JPEGInfo_t);
				result[cnt].frameVary = kFramesSameSize;
				result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)

				/********/
           /* Writes RGB uncompressed (TIFF codec ONLY) as lines,
            * reads back as lines, and compares against expected RGB
				 */
				result[cnt].testName = "Line Mode TIFF Video";
				result[cnt].writeXferType = kLineXfer;
				result[cnt].readXferType = kLineXfer;
				result[cnt].codecID = CODEC_TIFF_VIDEO;
				result[cnt].mediaKind = PICTUREKIND;
				result[cnt].numSamples = 1;			/* Only write 1 sample */
				result[cnt].numChannels = 1;
				result[cnt].videoWriteMemFormat = NULL;
				result[cnt].videoWriteFileFormat = NULL;
				result[cnt].videoReadMemFormat = NULL;
				result[cnt].writeBuf = &logoWriteBuffer;
				result[cnt].readBuf = &videoReadBuffer;
				result[cnt].checkBuf = &logoWriteBuffer;
				result[cnt].writeCompress = kToolkitCompressionDisable;
				result[cnt].readCompress = kToolkitCompressionDisable;
				if (strcmp(codecID, CODEC_AJPG_VIDEO) != 0)
				  result[cnt].codecPrivateData = NULL;
				result[cnt].frameVary = kFramesSameSize;
				result[cnt].alignment = 1;
				XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)
			 } /* if TIFF */
		 } /* if pictureKind */
				
		if(strcmp(mediaKindStr, SOUNDKIND) == 0)
		{
			/********/
			result[cnt].testName = "single-channel audio Media";
			result[cnt].writeXferType = kSingleChannel;
			result[cnt].readXferType = kSingleChannel;
			result[cnt].codecID = codecID;
			result[cnt].mediaKind =SOUNDKIND;
			result[cnt].numSamples = SOUND_BYTES/2;
			result[cnt].numChannels = 1;
			result[cnt].audioWriteMemFormat = NULL;
			result[cnt].audioWriteFileFormat = NULL;
			result[cnt].audioReadMemFormat = NULL;
			result[cnt].writeBuf = &soundWriteBuffer;
			result[cnt].readBuf = &audioReadBuffer;
			result[cnt].checkBuf = &soundWriteBuffer;
			result[cnt].writeCompress = kToolkitCompressionDisable;
			result[cnt].readCompress = kToolkitCompressionDisable;
			result[cnt].codecPrivateData = NULL;
			result[cnt].alignment = 1;
			XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)
				
			/********/
			result[cnt].testName = "Stereo audio Media (pre-interleaved)";
			result[cnt].writeXferType = kPreInterleaved;
			result[cnt].readXferType = kPreInterleaved;
			result[cnt].codecID = codecID;
			result[cnt].mediaKind =SOUNDKIND;
			result[cnt].numSamples = SOUND_BYTES/4;
			result[cnt].numChannels = 2;
			result[cnt].audioWriteMemFormat = NULL;
			result[cnt].audioWriteFileFormat = NULL;
			result[cnt].audioReadMemFormat = NULL;
			result[cnt].writeBuf = &soundWriteBuffer;
			result[cnt].readBuf = &audioReadBuffer;
			result[cnt].checkBuf = &soundWriteBuffer;
			result[cnt].writeCompress = kToolkitCompressionDisable;
			result[cnt].readCompress = kToolkitCompressionDisable;
			result[cnt].codecPrivateData = NULL;
			result[cnt].alignment = 1;
			XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)
												
			/********/
			result[cnt].testName = "Multi-channel calls (read interleaved)";
			result[cnt].writeXferType = kMultiChannel;
			result[cnt].readXferType = kPreInterleaved;
			result[cnt].codecID = codecID;
			result[cnt].mediaKind = SOUNDKIND;
			result[cnt].numSamples = SOUND_BYTES/4;
			result[cnt].numChannels = 2;
			result[cnt].audioWriteMemFormat = NULL;
			result[cnt].audioWriteFileFormat = NULL;
			result[cnt].audioReadMemFormat = NULL;
			result[cnt].writeBuf = &sndLeftWriteBuffer;
			result[cnt].writeBuf2 = &sndRightWriteBuffer;
			result[cnt].readBuf = &audioReadBuffer;
			result[cnt].checkBuf = &soundWriteBuffer;
			result[cnt].writeCompress = kToolkitCompressionDisable;
			result[cnt].readCompress = kToolkitCompressionDisable;
			result[cnt].codecPrivateData = NULL;
			result[cnt].alignment = 1;
			XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY);
							
			/********/
			result[cnt].testName = "Write Interleaved Read Multi-channel calls";
			result[cnt].writeXferType = kPreInterleaved;
			result[cnt].readXferType = kMultiChannel;
			result[cnt].codecID = codecID;
			result[cnt].mediaKind = SOUNDKIND;
			result[cnt].numSamples = SOUND_BYTES/4;
			result[cnt].numChannels = 2;
			result[cnt].audioWriteMemFormat = NULL;
			result[cnt].audioWriteFileFormat = NULL;
			result[cnt].audioReadMemFormat = NULL;
			result[cnt].writeBuf = &soundWriteBuffer;
			result[cnt].readBuf = &sndLeftReadBuffer;
			result[cnt].readBuf2 = &sndRightReadBuffer;
			result[cnt].checkBuf = &sndLeftWriteBuffer;
			result[cnt].checkBuf2 = &sndRightWriteBuffer;
			result[cnt].writeCompress = kToolkitCompressionDisable;
			result[cnt].readCompress = kToolkitCompressionDisable;
			result[cnt].codecPrivateData = NULL;
			result[cnt].alignment = 1;
			XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY);
			
			/********/
			result[cnt].testName = "single channel with seeks";
			result[cnt].writeXferType = kSingleChannel;
			result[cnt].readXferType = kSingleChannelChunks;
			result[cnt].codecID = codecID;
			result[cnt].mediaKind =SOUNDKIND;
			result[cnt].numSamples = SOUND_BYTES/2;
			result[cnt].numChannels = 1;
			result[cnt].audioWriteMemFormat = NULL;
			result[cnt].audioWriteFileFormat = NULL;
			result[cnt].audioReadMemFormat = NULL;
			result[cnt].writeBuf = &soundWriteBuffer;
			result[cnt].readBuf = &audioReadBuffer;
			result[cnt].checkBuf = &soundWriteBuffer;
			result[cnt].writeCompress = kToolkitCompressionDisable;
			result[cnt].readCompress = kToolkitCompressionDisable;
			result[cnt].codecPrivateData = NULL;
			result[cnt].alignment = 1;
			XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY)
											
			/********/
			result[cnt].testName = "8-bit memory, 16-bit file format";
			result[cnt].writeXferType = kSingleChannel;
			result[cnt].readXferType = kSingleChannel;
			result[cnt].codecID = codecID;
			result[cnt].mediaKind = SOUNDKIND;
			result[cnt].numSamples = SOUND_BYTES;
			result[cnt].numChannels = 1;
			result[cnt].audioWriteMemFormat = byteAudioMemFmt;
			result[cnt].audioWriteFileFormat = NULL;
			result[cnt].audioReadMemFormat = byteAudioMemFmt;
			result[cnt].writeBuf = &soundWriteBuffer;
			result[cnt].readBuf = &audioReadBuffer;
			result[cnt].checkBuf = &soundWriteBuffer;
			result[cnt].writeCompress = kToolkitCompressionDisable;
			result[cnt].readCompress = kToolkitCompressionDisable;
			result[cnt].codecPrivateData = NULL;
			result[cnt].alignment = 1;
			XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY);

			/********/
			if((strcmp(codecID, CODEC_AIFC_AUDIO) == 0)/* ||
			   (strcmp(codecID, CODEC_WAVE_AUDIO) == 0) */)
			{
			/* !!! Add for wave codec too !!! */
			result[cnt].testName = "8-bit write, read using ANSI";
			result[cnt].writeXferType = kSingleChannel;
			result[cnt].readXferType = kANSIRead;
			result[cnt].codecID = codecID;
			result[cnt].mediaKind = SOUNDKIND;
			result[cnt].numSamples = SOUND_BYTES;
			result[cnt].numChannels = 1;
			result[cnt].audioWriteMemFormat = NULL;
			result[cnt].audioWriteFileFormat = byteAudioMemFmt;
			result[cnt].audioReadMemFormat = NULL;
			result[cnt].writeBuf = &soundWriteBuffer;
			result[cnt].readBuf = &audioReadBuffer;
			result[cnt].checkBuf = &soundWriteBuffer;
			result[cnt].writeCompress = kToolkitCompressionDisable;
			result[cnt].readCompress = kToolkitCompressionDisable;
			result[cnt].codecPrivateData = NULL;
			result[cnt].alignment = 1;
			XASSERT(cnt++ < maxTests, OM_ERR_NOMEMORY);
			}

			/* !!! Add pixel format swabbing.
			 *	(same here, have BOTH mem formats be different from the file format)
			 * !!! Add reverse versions of tests (RGB => YUV)
			 */
		}
		*numTestsPtr = cnt;
	}
	XEXCEPT
	XEND
			
	return(OM_ERR_NONE);
}

omfErr_t setupStandardTests(mediaTestDef_t **resultPtr, 
									 omfInt32 *numTests,
									 omfFileRev_t rev)
{
	mediaTestDef_t	*result;
	omfInt32		n;
#if 0
	avidAVRdata_t 		avrData;
#endif
	
	XPROTECT(NULL)
	{
		*numTests = 0;
		
		/****** Buffers required by the standard tests ******/
#if 0
		avrData.avr = omfsMalloc(AVRNAME_SIZE);
#endif
		CHECK(allocTestBuf(FRAME_BYTES, &logoWriteBuffer));
		CHECK(allocTestBuf(FRAME_BYTES * NUM_RGB_COLORS, &RGBColorWriteBuffer));
		CHECK(allocTestBuf(FRAME_BYTES * NUM_YUV_COLORS, &YUVColorWriteBuffer));
		CHECK(allocTestBuf(COMPRESS_FRAME_BYTES, &RGBsingleFrameColorBuf));
		CHECK(allocTestBuf(FRAME_BYTES_YUV10 * NUM_YUV_COLORS, &colorWriteBufferYUV10));
		CHECK(allocTestBuf(FRAME_BYTES_YUV10 * NUM_YUV_COLORS, &videoReadBufferYUV10));

		CHECK(allocTestBuf(sizeof(compressedJFIF), &JFIFWriteBuffer));
		CHECK(allocTestBuf(sizeof(compressedJFIF), &JFIFReadBuffer));
		CHECK(allocTestBuf(sizeof(compressedTIFF), &JPEGTIFFWriteBuffer));
		CHECK(allocTestBuf(sizeof(compressedTIFF), &JPEGTIFFReadBuffer));
		CHECK(allocTestBuf(FRAME_BYTES * NUM_RGB_COLORS, &colorReadBuffer));
		
		CHECK(allocTestBuf(SOUND_BYTES, &soundWriteBuffer));
		CHECK(allocTestBuf(FRAME_BYTES, &videoReadBuffer));
		CHECK(allocTestBuf(SOUND_BYTES, &audioReadBuffer));
		CHECK(allocTestBuf(SOUND_BYTES/2, &sndLeftWriteBuffer));
		CHECK(allocTestBuf(SOUND_BYTES/2, &sndRightWriteBuffer));
		CHECK(allocTestBuf(SOUND_BYTES/2, &sndLeftReadBuffer));
		CHECK(allocTestBuf(SOUND_BYTES/2, &sndRightReadBuffer));

		FillBufLogo(&logoWriteBuffer, STD_HEIGHT, STD_WIDTH);
		fillColorBuf(RGBColorPatterns, NUM_RGB_COLORS, FRAME_BYTES, 
						 &RGBColorWriteBuffer);
		fillColorBuf(RGBColorPatterns, 1, COMPRESS_FRAME_BYTES, 
						 &RGBsingleFrameColorBuf);
		fillColorBuf(YUVColorPatterns, NUM_YUV_COLORS, FRAME_BYTES, 
						 &YUVColorWriteBuffer);
		fillColorBuf(YUV10ColorPatterns, NUM_YUV10_COLORS, FRAME_BYTES_YUV10, 
						 &colorWriteBufferYUV10);

		memcpy(JFIFWriteBuffer.buf, compressedJFIF, sizeof(compressedJFIF));
		memcpy(JPEGTIFFWriteBuffer.buf, compressedTIFF, sizeof(compressedTIFF));
		
		for(n = 0; n < SOUND_BYTES; n++)
			soundWriteBuffer.buf[n] = (omfUInt8)(n & 0xFF);
		for(n = 0; n < SOUND_BYTES/2; n++)
		{
			if((n & 0x01) == 0)
				sndLeftWriteBuffer.buf[n] = (omfUInt8)((n*2) & 0xFF);
			else
				sndLeftWriteBuffer.buf[n] = (omfUInt8)((((n & ~01) *2)+1) & 0xFF);
		}
		for(n = 0; n < SOUND_BYTES/2; n++)
		{
			if((n & 0x01) == 0)
				sndRightWriteBuffer.buf[n] = (omfUInt8)(((n*2)+2) & 0xFF);
			else
				sndRightWriteBuffer.buf[n] = (omfUInt8)((((n & ~01) *2)+3) & 0xFF);
		}

		/****** Formats required by the standard tests ******/
		stdVideoFmt[0].opcode = kOmfStoredRect;
		stdVideoFmt[0].operand. expRect.xOffset = 0;
		stdVideoFmt[0].operand. expRect.yOffset = 0;
		stdVideoFmt[0].operand. expRect.xSize = STD_WIDTH;
		stdVideoFmt[0].operand. expRect.ySize = STD_HEIGHT;
		stdVideoFmt[1].opcode = kOmfVFmtEnd;
			
		rgbVideoMemFmt[0].opcode = kOmfPixelFormat;
		rgbVideoMemFmt[0].operand.expPixelFormat = kOmfPixRGBA;
		rgbVideoMemFmt[1].opcode = kOmfRGBCompLayout;
		rgbVideoMemFmt[1].operand.expCompArray[0] = 'R';
		rgbVideoMemFmt[1].operand.expCompArray[1] = 'G';
		rgbVideoMemFmt[1].operand.expCompArray[2] = 'B';
		rgbVideoMemFmt[1].operand.expCompArray[3] = 0;
		rgbVideoMemFmt[2].opcode = kOmfRGBCompSizes;
		rgbVideoMemFmt[2].operand.expCompSizeArray[0] = 8;
		rgbVideoMemFmt[2].operand.expCompSizeArray[1] = 8;
		rgbVideoMemFmt[2].operand.expCompSizeArray[2] = 8;
		rgbVideoMemFmt[2].operand.expCompArray[3] = 0;
		rgbVideoMemFmt[3].opcode = kOmfVFmtEnd;
		
		yuvVideoMemFmt[0].opcode = kOmfPixelFormat;
		yuvVideoMemFmt[0].operand.expPixelFormat = kOmfPixYUV;
		yuvVideoMemFmt[1].opcode = kOmfVFmtEnd;

		byteAudioMemFmt[0].opcode = kOmfSampleSize;
		byteAudioMemFmt[0].operand.sampleSize = 8;
		byteAudioMemFmt[1].opcode = kOmfAFmtEnd;

		singleFieldFmt[0].opcode = kOmfFrameLayout;
		singleFieldFmt[0].operand.expFrameLayout = kOneField;
		singleFieldFmt[1].opcode = kOmfVFmtEnd;

		fullFrameFmt[0].opcode = kOmfFrameLayout;
		fullFrameFmt[0].operand.expFrameLayout = kFullFrame;
		fullFrameFmt[1].opcode = kOmfStoredRect;
		fullFrameFmt[1].operand.expRect.xOffset = 0;
		fullFrameFmt[1].operand.expRect.yOffset = 0;
		fullFrameFmt[1].operand.expRect.xSize = STD_WIDTH;
		fullFrameFmt[1].operand.expRect.ySize = STD_HEIGHT  * 2;
		fullFrameFmt[2].opcode = kOmfVFmtEnd;
		
		separateFieldFileFmt[0].opcode = kOmfFrameLayout;
		separateFieldFileFmt[0].operand.expFrameLayout = kSeparateFields;
		separateFieldFileFmt[1].opcode = kOmfStoredRect;
		separateFieldFileFmt[1].operand.expRect.xOffset = 0;
		separateFieldFileFmt[1].operand.expRect.yOffset = 0;
		separateFieldFileFmt[1].operand.expRect.xSize = STD_WIDTH;
		separateFieldFileFmt[1].operand.expRect.ySize = STD_HEIGHT/2;
		separateFieldFileFmt[2].opcode = kOmfVFmtEnd;
		
		fullFrameMemFmt[0].opcode = kOmfFrameLayout;
		fullFrameMemFmt[0].operand.expFrameLayout = kFullFrame;
		fullFrameMemFmt[1].opcode = kOmfVFmtEnd;
		
		yuvFileFmt[0].opcode = kOmfCDCICompWidth;
		yuvFileFmt[0].operand.expInt32 = 8;
		yuvFileFmt[1].opcode = kOmfCDCIHorizSubsampling;
		yuvFileFmt[1].operand.expInt32 = 1;
		yuvFileFmt[2].opcode = kOmfVFmtEnd;
		
		yuvFileFmt10[0].opcode = kOmfCDCICompWidth;
		yuvFileFmt10[0].operand.expInt32 = 10;
		yuvFileFmt10[1].opcode = kOmfCDCIHorizSubsampling;
		yuvFileFmt10[1].operand.expInt32 = 1;
		yuvFileFmt10[2].opcode = kOmfVFmtEnd;

		yuvFileFmtPad10[0].opcode = kOmfCDCICompWidth;
		yuvFileFmtPad10[0].operand.expInt32 = 10;
		yuvFileFmtPad10[1].opcode = kOmfCDCIHorizSubsampling;
		yuvFileFmtPad10[1].operand.expInt32 = 1;
		yuvFileFmtPad10[2].opcode = kOmfCDCIPadBits;
		yuvFileFmtPad10[2].operand.expInt16 = 2;
		yuvFileFmtPad10[3].opcode = kOmfVFmtEnd;

		yuvFileFmtSubsample[0].opcode = kOmfCDCICompWidth;
		yuvFileFmtSubsample[0].operand.expInt32 = 8;
		yuvFileFmtSubsample[1].opcode = kOmfCDCIHorizSubsampling;
		yuvFileFmtSubsample[1].operand.expInt32 = 2;
		yuvFileFmtSubsample[2].opcode = kOmfVFmtEnd;
		
		mixedFieldFmt[0].opcode = kOmfFrameLayout;
		mixedFieldFmt[0].operand.expFrameLayout = kMixedFields;
		mixedFieldFmt[1].opcode = kOmfVFmtEnd;
		
		separateFieldFmt[0].opcode = kOmfFrameLayout;
		separateFieldFmt[0].operand.expFrameLayout = kSeparateFields;
		separateFieldFmt[1].opcode = kOmfVFmtEnd;

		yuvFileFmtMixed[0].opcode = kOmfCDCICompWidth;
		yuvFileFmtMixed[0].operand.expInt32 = 8;
		yuvFileFmtMixed[1].opcode = kOmfCDCIHorizSubsampling;
		yuvFileFmtMixed[1].operand.expInt32 = 1;
		yuvFileFmtMixed[2].opcode = kOmfFrameLayout;
		yuvFileFmtMixed[2].operand.expFrameLayout = kMixedFields;
		yuvFileFmtMixed[3].opcode = kOmfVFmtEnd;

		yuvFileFmtSeparate[0].opcode = kOmfCDCICompWidth;
		yuvFileFmtSeparate[0].operand.expInt32 = 8;
		yuvFileFmtSeparate[1].opcode = kOmfCDCIHorizSubsampling;
		yuvFileFmtSeparate[1].operand.expInt32 = 1;
		yuvFileFmtSeparate[2].opcode = kOmfFrameLayout;
		yuvFileFmtSeparate[2].operand.expFrameLayout = kSeparateFields;
		yuvFileFmtSeparate[3].opcode = kOmfVFmtEnd;

		/****** Codec private data ******/
		tiffPrivate.JPEGTableID = 0;
		tiffPrivate.compression = kJPEG;

		/****** The standard tests ******/
		result = omfsMalloc(sizeof(mediaTestDef_t) * MAX_STD_TESTS);
		if(result == NULL)
			RAISE(OM_ERR_NOMEMORY);

		CHECK(addTestsForCodec(CODEC_TIFF_VIDEO, PICTUREKIND, MAX_STD_TESTS, 
									  kOmfPixRGBA, TRUE, NULL, result, numTests));

		CHECK(addTestsForCodec(CODEC_RGBA_VIDEO, PICTUREKIND, MAX_STD_TESTS, 
									  kOmfPixRGBA, FALSE, NULL, result, numTests));
		CHECK(addTestsForCodec(CODEC_CDCI_VIDEO, PICTUREKIND, MAX_STD_TESTS, 
									  kOmfPixYUV, FALSE, NULL, result, numTests));
		CHECK(addTestsForCodec(CODEC_JPEG_VIDEO, PICTUREKIND, MAX_STD_TESTS, 
									  kOmfPixYUV, TRUE, NULL, result, numTests)); 
		CHECK(addTestsForCodec(CODEC_AIFC_AUDIO, SOUNDKIND, MAX_STD_TESTS, 
									  kOmfPixNone, FALSE, NULL, result, numTests));
		CHECK(addTestsForCodec(CODEC_WAVE_AUDIO, SOUNDKIND, MAX_STD_TESTS, 
									  kOmfPixNone, FALSE, NULL, result, numTests));
#if AVID_CODEC_SUPPORT
		CHECK(addTestsForCodec(CODEC_AVID_JFIF_VIDEO, PICTUREKIND, MAX_STD_TESTS, 
									  kOmfPixYUV, TRUE, NULL, result, numTests)); 
		CHECK(addTestsForCodec(CODEC_AVID_JPEG_VIDEO, PICTUREKIND, MAX_STD_TESTS, 
									  kOmfPixRGBA, TRUE, NULL, result, numTests)); 
#endif

#if 0
		/* AVR Media Tests */
		if (rev != kOmfRev2x)
		  {
			 avrData.product = avidComposerMac;

			 strcpy( (char*) avrData.avr, "AVR5");
			 CHECK(addTestsForCodec(CODEC_AJPG_VIDEO, PICTUREKIND, MAX_STD_TESTS,
									kOmfPixRGBA, TRUE, &avrData, result, numTests));
		  }

		omfsFree(avrData.avr);
#endif

		*resultPtr = result;
	 }
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

omfErr_t cleanupStandardTests(mediaTestDef_t **resultPtr, omfInt32 *numTests)
{
	XPROTECT(NULL)
	{
		omfsFree(*resultPtr);
		CHECK(freeTestBuf(&colorReadBuffer));
		CHECK(freeTestBuf(&logoWriteBuffer));
		CHECK(freeTestBuf(&RGBColorWriteBuffer));
		CHECK(freeTestBuf(&YUVColorWriteBuffer));
		CHECK(freeTestBuf(&RGBsingleFrameColorBuf));
		CHECK(freeTestBuf(&JFIFWriteBuffer));
		CHECK(freeTestBuf(&JPEGTIFFWriteBuffer));
		CHECK(freeTestBuf(&JFIFReadBuffer));
		CHECK(freeTestBuf(&JPEGTIFFReadBuffer));
		CHECK(freeTestBuf(&soundWriteBuffer));
		CHECK(freeTestBuf(&videoReadBuffer));
		CHECK(freeTestBuf(&audioReadBuffer))
		CHECK(freeTestBuf(&sndLeftWriteBuffer))
		CHECK(freeTestBuf(&sndRightWriteBuffer))
		CHECK(freeTestBuf(&sndLeftReadBuffer))
		CHECK(freeTestBuf(&sndRightReadBuffer))
		CHECK(freeTestBuf(&colorWriteBufferYUV10))
		CHECK(freeTestBuf(&videoReadBufferYUV10))
	}
	XEXCEPT
	XEND

	return(OM_ERR_NONE);
}

int Write2xMedia(char *filename, omfFileRev_t rev)
{
	mediaTestDef_t	*stdTests;
	omfInt32		numStdTests, testStat;
	omfErr_t		status;
	
	status = setupStandardTests(&stdTests, &numStdTests, rev);
	if(status != OM_ERR_NONE)
	{
		printf("Error loading test\n");
		return(3);
	}
	testStat = ExecMediaWriteTests(filename, stdTests, numStdTests, rev);
	cleanupStandardTests(&stdTests, &numStdTests);
	return(testStat);
}

int Check2xMedia(char *filename, omfFileRev_t rev)
{
	mediaTestDef_t	*stdTests;
	omfInt32		numStdTests, testStat;
	omfErr_t		status;

	status = setupStandardTests(&stdTests, &numStdTests, rev);
	if(status != OM_ERR_NONE)
	{
		printf("Error loading test\n");
		return(3);
	}
	/* Validate the result */
	testStat = ExecMediaCheckTests(filename, stdTests, numStdTests);	
	cleanupStandardTests(&stdTests, &numStdTests);
	return(testStat);
}

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

static omfErr_t freeTestBuf(testBuf_t *buf)
{
	if(buf->buf != NULL)
		omfsFree(buf->buf);
	return(OM_ERR_NONE);
}

static omfBool SoundCompare(testBuf_t *buf1, 
									 testBuf_t *buf2, 
									 omfInt32 lenSamples, 
									 omfInt32 numChannels,
									 omfUInt16 byteOrder,
									 omfInt16 sampleSize)
{
	omfInt32	n;
	omfInt16	val;
	omfInt16	*ptr1, *ptr2;
	
/*	!!! Take this out until we can also get "bitsPerSample"
	if(buf1->bufLen != lenSamples * 2 * numChannels)
		return(FALSE);
	if(buf2->bufLen != lenSamples * 2 * numChannels)
		return(FALSE);
*/
		
   /* NOTE that with the synthetic test patterns used by this test, we have to 
    * byte swap on the compare in order to make things come out correctly when 
    * transferring between defierently-endian machines.  This is because the 
    * test patterns are sent and tested as bytes, but are interpreted by the 
	 * codecs as words.  The pattern 0x00 0x01 would have been the word 0x0100 
	 * on a PC, and would have been byte-swapped to 0x01 0x00
	 * for correct transference to a Mac as AIFC data.  The mac codec would 
    * not byte-swap the data again as it would be a native format.  So we 
    * have to do the byte swap on the compare.
	 */
	lenSamples = buf1->bufLen / 2;	/* !!!Take this out until we can also get "bitsPerSample" */
	if((byteOrder == OMNativeByteOrder) || (sampleSize == 8))
	{
		ptr1 = (omfInt16 *)buf1->buf;
		ptr2 = (omfInt16 *)buf2->buf;
		for(n = 0; n < lenSamples; n++)
		{
			if(*ptr1++ != *ptr2++)
			{
				printf("%0lx %0lx %ld\n", ptr1-1, ptr2-1, n);
				return(FALSE);
			}
		}
		
		return(TRUE);
	}
	else
	{
		ptr1 = (omfInt16 *)buf1->buf;
		ptr2 = (omfInt16 *)buf2->buf;
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

static void fillColorBuf(omfUInt8 **patternList, omfInt32 numFrames, omfInt32 frameBytes, testBuf_t *buf)
{
	omfInt32	f, n, offset, ptnSize;
	omfUInt8	*pattern;
	
	for(f = 0; f < numFrames; f++)
	{
		pattern = patternList[f];
		ptnSize = pattern[0];
		offset = f * frameBytes;
		for(n = 0; n < frameBytes; n++)
		{
			buf->buf[n+offset] = pattern[ (n % ptnSize) + 1 ];
		}
	}
}

static omfErr_t ClearBuffer(testBuf_t *buf)
{
	omfInt32	n;
	
	for (n = 0; n < buf->bufLen; n++)
		buf->buf[n] = 0;
	return(OM_ERR_NONE);
}

static void     FillBufLogo(testBuf_t *buf, omfInt32 height, omfInt32 width)
{
	int 				bit, x, y;
	omfUInt8			*dest, *src, val;
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

static omfErr_t ValidateSoundKind(omfHdl_t filePtr, omfObject_t masterMob)
{
	omfDDefObj_t	pictureKind, soundKind;
	omfErr_t		status;
	omfInt16		numChannels;

	XPROTECT(filePtr)
	{
		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureKind, &status);
		CHECK(status);
		omfiDatakindLookup(filePtr, SOUNDKIND, &soundKind, &status);
		CHECK(status);

		CHECK(omfmGetNumChannels(filePtr, masterMob, 1, NULL, pictureKind, &numChannels));
		if(numChannels != 0)
			FatalError("Found video on audio test result\n");
		CHECK(omfmGetNumChannels(filePtr, masterMob, 1, NULL, soundKind, &numChannels));
		if(numChannels == 0)
			FatalError("Missing audio on audio test result\n");
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

static omfErr_t CompareVideoFileFormat(omfMediaHdl_t media, omfVideoMemOp_t *expected)
{
	omfVideoMemOp_t	*actual, *src, *dest, *exp, *act;
	omfInt32		numOpcodes;
	
	XPROTECT(media->mainFile)
	{
		numOpcodes = omfmVideoOpCount(media->mainFile, expected) + 1;

		actual = (omfVideoMemOp_t *)omfsMalloc(numOpcodes * sizeof(*actual));
		if(actual == NULL)
			RAISE(OM_ERR_NOMEMORY);

		for(src = expected, dest = actual; ; src++, dest++)
		{
			dest->opcode = src->opcode;
			dest->operand.expRational.numerator = 0;
			dest->operand.expRational.denominator = 0;
			if(src->opcode == kOmfVFmtEnd)
				break;
		}
			
		CHECK(omfmGetVideoInfoArray(media, actual));
		for(exp = expected, act = actual; exp->opcode != kOmfVFmtEnd; exp++, act++)
		{
			switch(exp->opcode)
			{
			case kOmfStoredRect:
			case kOmfDisplayRect:
			case kOmfSampledRect:
				if((exp->operand.expRect.xOffset != act->operand.expRect.xOffset) ||
				   (exp->operand.expRect.yOffset != act->operand.expRect.yOffset) ||
				   (exp->operand.expRect.xSize != act->operand.expRect.xSize) ||
				   (exp->operand.expRect.ySize != act->operand.expRect.ySize))
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;
				
			case kOmfPixelFormat:
				if(exp->operand.expPixelFormat != act->operand.expPixelFormat)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;

			case kOmfFrameLayout:
				if(exp->operand.expFrameLayout != act->operand.expFrameLayout)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;

			case kOmfAlphaTransparency:
			case kOmfImageAlignmentFactor:
			case kOmfCDCICompWidth:
				if(exp->operand.expInt32 != act->operand.expInt32)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;

			case kOmfCDCIHorizSubsampling:
			case kOmfCDCIBlackLevel:
			case kOmfCDCIWhiteLevel:
			case kOmfCDCIColorRange:
				if(exp->operand.expUInt32 != act->operand.expUInt32)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;

			case kOmfFieldDominance:
				if(exp->operand.expFieldDom != act->operand.expFieldDom)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;

			case kOmfPixelSize:
				if(exp->operand.expInt16 != act->operand.expInt16)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;

			case kOmfWillTransferLines:
			case kOmfIsCompressed:
				if(exp->operand.expBoolean != act->operand.expBoolean)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;

			case kOmfAspectRatio:
			case kOmfGamma:
				if((exp->operand.expRational.numerator != act->operand.expRational.numerator) ||
				   (exp->operand.expRational.denominator != act->operand.expRational.denominator))
				{
					RAISE(OM_ERR_TEST_FAILED);
				}

			case kOmfRGBCompLayout:
			case kOmfRGBPaletteLayout:
				if(strcmp((char *)&exp->operand.expCompArray,
				  (char *)&act->operand.expCompArray) != 0)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;

			case kOmfRGBCompSizes:
			case kOmfRGBPaletteSizes:
				if(strcmp((char *)&exp->operand.expCompSizeArray,
				  (char *)&act->operand.expCompSizeArray) != 0)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;
		
			case kOmfCDCIColorSiting:
				if(exp->operand.expColorSiting != act->operand.expColorSiting)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;
				
			case kOmfVideoLineMap:		/* operand.expLineMap */
			default:
				break;
			}
		}
		omfsFree(actual);
	}
	XEXCEPT
	{
		if(XCODE() == OM_ERR_TEST_FAILED)
			printf("Failed video format opcode # %d\n", (short)exp->opcode);
	}
	XEND
	
	return(OM_ERR_NONE);
}

static omfErr_t CompareAudioFileFormat(omfMediaHdl_t media, omfAudioMemOp_t *expected)
{
	omfAudioMemOp_t	*actual, *src, *dest, *exp, *act;
	omfInt32		numOpcodes;
	
	XPROTECT(media->mainFile)
	{
		for(src = expected, numOpcodes = 0; ; src++)
		{
			numOpcodes++;
			if(src->opcode == kOmfAFmtEnd)
				break;
		}
		actual = (omfAudioMemOp_t *)omfsMalloc(numOpcodes * sizeof(*actual));
		if(actual == NULL)
			RAISE(OM_ERR_NOMEMORY);

		for(src = expected, dest = actual; ; src++, dest++)
		{
			dest->opcode = src->opcode;
			dest->operand.expLong = 0;
			dest->operand.expRational.numerator = 0;
			dest->operand.expRational.denominator = 0;
			if(src->opcode == kOmfAFmtEnd)
				break;
		}
			
		CHECK(omfmGetAudioInfoArray(media, actual));
		for(exp = expected, act = actual; exp->opcode != kOmfAFmtEnd; exp++, act++)
		{
			switch(exp->opcode)
			{
			case kOmfSampleSize:
				if(exp->operand.sampleSize != act->operand.sampleSize)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;
				
			case kOmfSampleFormat:
				if(exp->operand.format != act->operand.format)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;

			case kOmfNumChannels:
				if(exp->operand.numChannels != act->operand.numChannels)
				{
					RAISE(OM_ERR_TEST_FAILED);
				}
				break;

			case kOmfSampleRate:
				if((exp->operand.sampleRate.numerator != act->operand.sampleRate.numerator) ||
				   (exp->operand.sampleRate.denominator != act->operand.sampleRate.denominator))
				{
					RAISE(OM_ERR_TEST_FAILED);
				}

			default:
				break;
			}
		}
		omfsFree(actual);
	}
	XEXCEPT
	{
		if(XCODE() == OM_ERR_TEST_FAILED)
			printf("Failed audio format opcode # %d\n", (short)exp->opcode);
	}
	XEND
	
	return(OM_ERR_NONE);
}

static omfErr_t WriteMedia(omfHdl_t filePtr, mediaTestDef_t *testDef)
{
	omfRational_t		imageAspectRatio;	
	omfRational_t		videoRate;
	omfRational_t		audioRate;
	omfDDefObj_t		pictureKind, soundKind, mediaKind;
	omfObject_t			masterMob, fileMob;
	omfMediaHdl_t		mediaPtr;
	omfErr_t			status;
	omfmMultiCreate_t	mult[2];
	omfAudioMemOp_t		audioOp[3];
	omfmMultiXfer_t		writeXfer[2];
	omfInt32			line, lineLen, height;
	char				mobName[256], codecName[128];
	omfUInt8			*linePtr;
	omfRect_t			sampledRect;
	avidAVRdata_t 		*avrData;
	omfVideoMemOp_t		*videoOps, alignOp[2];
	omfFrameLayout_t	writeLayout;
	
	XPROTECT(filePtr)
	{
		CHECK(omfmCodecGetName(filePtr, testDef->codecID, sizeof(codecName), 
									  codecName));
		printf("Writing %s\n    '%s'.\n", codecName, testDef->testName);
			
		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureKind, &status);
		CHECK(status);
		omfiDatakindLookup(filePtr, SOUNDKIND, &soundKind, &status);
		CHECK(status);
		omfiDatakindLookup(filePtr, testDef->mediaKind, &mediaKind, &status);
		CHECK(status);

		MakeRational(2997, 100, &videoRate);
		MakeRational(4, 3, &imageAspectRatio);
		MakeRational(44100, 1, &audioRate);
		
		strcpy( (char*) mobName, testDef->codecID);
		strcat(mobName, testDef->testName);
		
		CHECK(omfmMasterMobNew(	filePtr, 					/* file (IN) */
								mobName, 					/* name (IN) */
								TRUE,						/* isPrimary (IN) */
								&masterMob)); 				/* result (OUT) */
		
		if(mediaKind == pictureKind)
		{
			CHECK(omfmFileMobNew(	filePtr,	 		/* file (IN) */
				                mobName, 			/* name (IN) */
				                videoRate,   		/* editrate (IN) */
				                testDef->codecID, 	/* codec (IN) */
				                &fileMob));   		/* result (OUT) */

			writeLayout = kFullFrame;
			videoOps = testDef->videoWriteFileFormat;
			if(videoOps != NULL)
			{
				while(videoOps->opcode != kOmfVFmtEnd)
				{
					if(videoOps->opcode == kOmfFrameLayout)
					{
						writeLayout = videoOps->operand.expFrameLayout;
						break;
					}
				videoOps++;
				}
			}

			if (strcmp(testDef->codecID, CODEC_AJPG_VIDEO) == 0)
			  {
				 avrData = (avidAVRdata_t *)testDef->codecPrivateData;
				 CHECK(avrVideoMediaCreate(filePtr, masterMob, 1, fileMob,
													videoRate, 
													*avrData,
													kNTSCSignal,
													&mediaPtr));
			  }
			else
			  {
			  	 if((writeLayout == kSeparateFields) || (writeLayout == kMixedFields))
			  	 	height = STD_HEIGHT / 2;
			  	 else
			  	 	height = STD_HEIGHT;
				 CHECK(omfmVideoMediaCreate(filePtr, masterMob, 1, fileMob, 
													 testDef->writeCompress, 
													 videoRate, height, STD_WIDTH, 
													 writeLayout, 
													 imageAspectRatio, &mediaPtr));
  				omfmSetVideoLineMap(mediaPtr, 16, kTopField1);
			  }

			if((strcmp(testDef->codecID, CODEC_TIFF_VIDEO) != 0) &&
				(strcmp(testDef->codecID, CODEC_AJPG_VIDEO) != 0))
			{
				sampledRect.xOffset = 0;
				sampledRect.yOffset = 0;
				sampledRect.xSize = STD_WIDTH+1;
				sampledRect.ySize = STD_HEIGHT+1;
				CHECK(omfmSetSampledRect(mediaPtr, sampledRect));
			}

			if(testDef->videoWriteMemFormat != NULL)
			{
				CHECK(omfmSetVideoMemFormat(mediaPtr,
													 testDef->videoWriteMemFormat)); 
			}
			if(testDef->alignment > 1)
			  {
				 alignOp[0].opcode = kOmfImageAlignmentFactor;
				 alignOp[0].operand.expInt32 = testDef->alignment;
				 alignOp[1].opcode = kOmfVFmtEnd;
				 CHECK(omfmPutVideoInfoArray(mediaPtr, alignOp));
			  }
			if(testDef->videoWriteFileFormat != NULL)
			{
				CHECK(omfmPutVideoInfoArray(mediaPtr, 
													 testDef->videoWriteFileFormat));
			}
		}
		else if(mediaKind == soundKind)
		{
			CHECK(omfmFileMobNew(	filePtr,	 		/* file (IN) */
					                testDef->testName, 	/* name (IN) */
					                audioRate,   		/* editrate (IN) */
					                testDef->codecID, 	/* codec (IN) */
					                &fileMob));   		/* result (OUT) */
		
			switch(testDef->writeXferType)
			{
			case kMultiChannel:
			case kPreInterleaved:
				mult[0].mediaKind = soundKind;
				mult[0].subTrackNum = 1;
				mult[0].trackID = 1;
				mult[0].sampleRate = audioRate;
				mult[1].mediaKind = soundKind;
				mult[1].subTrackNum = 2;
				mult[1].trackID = 2;
				mult[1].sampleRate = audioRate;
				CHECK(omfmMediaMultiCreate(filePtr, masterMob, fileMob,
										sizeof(mult)/sizeof(mult[0]), mult,
										audioRate, kToolkitCompressionDisable, 
													&mediaPtr));
				audioOp[0].opcode = kOmfSampleSize;
				audioOp[0].operand.sampleSize = 16;
				audioOp[1].opcode = kOmfAFmtEnd;
				CHECK(omfmPutAudioInfoArray(mediaPtr, audioOp));
				break;
			
			case kSingleChannel:
			case kLineXfer:
			default:
				CHECK(omfmAudioMediaCreate(filePtr, masterMob, 1, fileMob, 
													audioRate, 
													kToolkitCompressionDisable, 
													16, 1, &mediaPtr));
				break;
			}
			
			if(testDef->audioWriteMemFormat != NULL)
			{
				CHECK(omfmSetAudioMemFormat(mediaPtr,
													 testDef->audioWriteMemFormat)); 
			}
			if(testDef->audioWriteFileFormat != NULL)
			{
				CHECK(omfmPutAudioInfoArray(mediaPtr, 
													 testDef->audioWriteFileFormat));
			}
		}
		else
		{
			printf("Illegal test type on write\n");
			return((omfErr_t)2);
		}

		/* If there is private data, but not the Avid JPEG codec (since
		 * the avrVideoMediaCreate() function already takes care of the
		 * private data.
		 */
		if((testDef->codecPrivateData != NULL) && 
			strcmp(testDef->codecID, CODEC_AJPG_VIDEO) != 0)
		{
			CHECK(omfmCodecSendPrivateData(mediaPtr, 
													 testDef->codecPrivateDataLen, 
													 testDef->codecPrivateData));
		 }
		
		switch(testDef->writeXferType)
		{
		case kSingleChannel:
			CHECK(omfmWriteDataSamples(mediaPtr, testDef->numSamples, 
												testDef->writeBuf->buf, 
												testDef->writeBuf->bufLen));
			break;
			
		case kMultiChannel:
			writeXfer[0].mediaKind = mediaKind;
			writeXfer[0].subTrackNum = 1;
			writeXfer[0].numSamples = testDef->numSamples;
			writeXfer[0].buflen = testDef->writeBuf->bufLen;
			writeXfer[0].buffer = testDef->writeBuf->buf;
			writeXfer[1].mediaKind = mediaKind;
			writeXfer[1].subTrackNum = 2;
			writeXfer[1].numSamples = testDef->numSamples;
			writeXfer[1].buflen = testDef->writeBuf2->bufLen;
			writeXfer[1].buffer = testDef->writeBuf2->buf;
			CHECK(omfmWriteMultiSamples(mediaPtr, 2, writeXfer));
			break;
		
		case kLineXfer:
			linePtr = testDef->writeBuf->buf;
			for(line = 0; line < STD_HEIGHT; line++, linePtr += LINE_BYTES)
			{
				CHECK(omfmWriteDataLines(mediaPtr, 1, linePtr, &lineLen));
			}
			break;

		case kPreInterleaved:
			CHECK(omfmWriteRawData(mediaPtr, testDef->numSamples,
									 testDef->writeBuf->buf, 
									testDef->writeBuf->bufLen));	
			break;
/*	kOmfWillTransferLines,	 operand.expBoolean */
/*	kOmfIsCompressed,		 operand.expBoolean */
		default:
			printf("Illegal xfer type on write\n");
			return((omfErr_t)2);
		}
														
		CHECK(omfmMediaClose(mediaPtr));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

static omfErr_t CheckMedia(omfHdl_t filePtr, 
									omfObject_t masterMob, 
									mediaTestDef_t *testDef, 
									char *fileName)
{
	omfInt16		numChannels, sampleSize;
	omfDDefObj_t	pictureKind, soundKind, mediaKind;
	omfErr_t		status;
	omfMediaHdl_t	media;
	omfInt16		byteOrder;
	omfUInt32		bytesRead, samplesRead, offset32, line;
	omfmMultiXfer_t	readXfer[2];
	omfUInt8		*linePtr;
	omfInt64		offset;
	omfInt32		n, lineLen, maxSize, xferOffset, xferLen;
	omfBool			isContig;
	FILE			*ansiPtr;
	char			*bufPtr, codecName[128];
	omfRect_t		sampledRect;
	omfAudioMemOp_t	*audioOps;
	omfLength_t		sampleCount64, frameCount64;
	omfInt32		sampleCount, frameCount, samplesLeft, segmentSize, segBytes;
	
	XPROTECT(filePtr)
	{
		CHECK(omfmCodecGetName(filePtr, testDef->codecID, sizeof(codecName), 
									  codecName));
		printf("Checking %s\n    '%s'.\n", codecName, testDef->testName);

		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureKind, &status);
		CHECK(status);
		omfiDatakindLookup(filePtr, SOUNDKIND, &soundKind, &status);
		CHECK(status);
		omfiDatakindLookup(filePtr, testDef->mediaKind, &mediaKind, &status);
		CHECK(status);
		omfsFileGetDefaultByteOrder(filePtr, &byteOrder);

		CHECK(ClearBuffer(testDef->readBuf));
			
		if(mediaKind == pictureKind)
		{
			CHECK(omfmGetNumChannels(filePtr, masterMob, 1, NULL, soundKind, 
											 &numChannels));
			if(numChannels != 0)
				FatalError("Found audio on video test result\n");
			CHECK(omfmGetNumChannels(filePtr, masterMob, 1, NULL, pictureKind, 
											 &numChannels));
			if(numChannels == 0)
				FatalError("Missing video on video test result\n");
			   	
			CHECK(omfmMediaOpen(filePtr, masterMob, 1, NULL, kMediaOpenReadOnly, 
								testDef->readCompress, 
								&media));

			if((strcmp(testDef->codecID, CODEC_TIFF_VIDEO) != 0) &&
				(strcmp(testDef->codecID, CODEC_AJPG_VIDEO) != 0))
			{
				CHECK(omfmGetSampledRect(media, &sampledRect));
				if((sampledRect.xOffset != 0) || (sampledRect.yOffset != 0) ||
				   (sampledRect.xSize != STD_WIDTH+1) || 
					(sampledRect.ySize != STD_HEIGHT+1))
				{
					FatalError("Incorrect values on sampled rect\n");
				}
			}
			
			if(testDef->videoReadMemFormat != NULL)
			{
				CHECK(omfmSetVideoMemFormat(media,testDef->videoReadMemFormat)); 
			}
			if(testDef->videoWriteFileFormat != NULL)
			{
				CHECK(CompareVideoFileFormat(media, testDef->videoWriteFileFormat));
			}
		
			if((testDef->frameVary == kFramesSameSize) && 
				(testDef->videoReadMemFormat == NULL) &&
				(testDef->readCompress == kToolkitCompressionEnable)) 
			{
				CHECK(omfmGetLargestSampleSize(media, pictureKind, &maxSize));
				if((maxSize * testDef->numSamples) != testDef->readBuf->bufLen)
				{
					FatalError("Wrong largest sample size\n");
				}
			}
			
			CHECK(omfmIsMediaContiguous(media, &isContig));
			if(!isContig)
			{
				FatalError("Media is not contiguous\n");
			}

			CHECK(omfmGetSampleCount(media, &sampleCount64));
			CHECK(omfsTruncInt64toInt32(sampleCount64, &sampleCount));	/* OK TEST */
			if(sampleCount != testDef->numSamples)
			{
				FatalError("wrong number of samples returned\n");
			}

			if(testDef->readXferType == kLineXfer)
			{
				linePtr = testDef->readBuf->buf;
				for(line = 0; line < STD_HEIGHT; line++, linePtr += LINE_BYTES)
				{
					CHECK(omfmReadDataLines(media, 1, LINE_BYTES, linePtr, 
													&lineLen));
				}
			}
			else if(testDef->readXferType == kSingleChannelChunks)
			{
				CHECK(omfmGetLargestSampleSize(media, pictureKind, &maxSize));
				for(n = testDef->numSamples; n >= 1; n--)
				{
					CHECK(omfmGotoShortFrameNumber(media, n));
					bufPtr = ((char *)testDef->readBuf->buf) + ((n-1) * maxSize);
					CHECK(omfmReadDataSamples(media, 1, maxSize, 
									  bufPtr, &bytesRead));
				}
				if (!PixelCompare(4, testDef->checkBuf, testDef->readBuf))
					FatalError("Video buffers did not compare\n");
			}
			else
			{
				CHECK(omfmReadDataSamples(media, testDef->numSamples, 
												  testDef->readBuf->bufLen, 
									  testDef->readBuf->buf, &bytesRead));
			}
			if (!PixelCompare(4, testDef->checkBuf, testDef->readBuf))
				FatalError("Video buffers did not compare\n");

			if(testDef->readXferType == kSingleChannel)
			{
				printf("Checking %s\n    '%s' (EOF Case).\n", codecName, testDef->testName);
				CHECK(omfmGetSampleCount(media, &frameCount64));
				CHECK(omfsTruncInt64toInt32(frameCount64, &frameCount));	/* OK FRAMEOFFSET */
				CHECK(omfmGetLargestSampleSize(media, pictureKind, &maxSize));
				CHECK(omfmGotoShortFrameNumber(media, 2));
				status = omfmReadDataSamples(media, frameCount, 
												  (frameCount) * maxSize, 
									  testDef->readBuf->buf, &bytesRead);
				if(status == OM_ERR_NONE)
				{
					FatalError("Missing expected EOF error\n");
				}
				else if((status == OM_ERR_EOF) || (status == OM_ERR_BADFRAMEOFFSET) ||
						(status == OM_ERR_END_OF_DATA))
				{
					if((omfInt32)bytesRead != (frameCount-1) * maxSize)
						FatalError("BytesRead incorrect on EOF\n");
				}
				else if((status == OM_ERR_TRANSLATE) || (status == OM_ERR_DECOMPRESS))
				{
					/* Try to clean up these error returns */
				}
				else if(status == OM_ERR_ONESAMPLEREAD)
				{		
					if((frameCount == 1) && ((omfInt32)bytesRead != maxSize))
						FatalError("BytesRead incorrect on EOF\n");
				}
				else
				{
					CHECK(status);
				}
			}
		}
		else if(mediaKind == soundKind)
		{
			CHECK(ClearBuffer(testDef->readBuf));
	
			CHECK(ValidateSoundKind(filePtr, masterMob));
	   	

			switch(testDef->readXferType)
			{
			case kMultiChannel:
				CHECK(omfmMediaMultiOpen(filePtr, masterMob, 1, NULL, 
												 kMediaOpenReadOnly, 
									  kToolkitCompressionDisable, 
									  &media));
				break;
			
			case kSingleChannel:
			case kSingleChannelChunks:
			default:
				CHECK(omfmMediaOpen(filePtr, masterMob, 1, NULL, 
										  kMediaOpenReadOnly, 
									  kToolkitCompressionDisable, 
									  &media));
				break;
			}

			if(testDef->audioReadMemFormat != NULL)
			{
				CHECK(omfmSetAudioMemFormat(media,testDef->audioReadMemFormat)); 
			}
			if(testDef->audioWriteFileFormat != NULL)
			{
				CHECK(CompareAudioFileFormat(media, testDef->audioWriteFileFormat));
			}

			CHECK(omfmGetLargestSampleSize(media, soundKind, &maxSize));
#if 0
			if(maxSize != 2)		/* Fixed size for now */
			{
				FatalError("Wrong largest sample size\n");
			}
#endif

			CHECK(omfmIsMediaContiguous(media, &isContig));
			if(!isContig)
			{
				FatalError("Media is not contiguous\n");
			}
			
			sampleSize = 16;
			audioOps = testDef->audioReadMemFormat;
			if(audioOps != NULL)
			{
				while(audioOps->opcode != kOmfAFmtEnd)
				{
					if(audioOps->opcode == kOmfSampleSize)
					{
						sampleSize = audioOps->operand.sampleSize;
						break;
					}
				audioOps++;
				}
			}
			
			
			switch(testDef->readXferType)
			{
			case kSingleChannel:
				CHECK(omfmReadDataSamples(media, testDef->numSamples, 
												  testDef->readBuf->bufLen, 
												  testDef->readBuf->buf, &bytesRead));
				if (!SoundCompare(testDef->checkBuf, testDef->readBuf, 
										testDef->numSamples, 
										testDef->numChannels, byteOrder, sampleSize))
					FatalError("Audio buffers did not compare\n");
				break;

			case kSingleChannelChunks:
				CHECK(omfmGetLargestSampleSize(media, mediaKind, &maxSize));
				xferOffset = (testDef->numSamples / 2);		/* Do in two pieces */
				xferLen = (testDef->readBuf->bufLen / maxSize) - xferOffset;
				/* Make one based */
				CHECK(omfmGotoShortFrameNumber(media, xferOffset+1));	
				bufPtr = ((char *)testDef->readBuf->buf) + (xferOffset * maxSize);
				CHECK(omfmReadDataSamples(media, xferLen, (xferLen*maxSize), 
												  bufPtr, &bytesRead));
				
				xferLen = xferOffset;
				CHECK(omfmGotoShortFrameNumber(media, 1));
				bufPtr = ((char *)testDef->readBuf->buf);
				CHECK(omfmReadDataSamples(media, xferLen, (xferLen*maxSize), 
												  bufPtr, &bytesRead));
				
				
				if (!SoundCompare(testDef->checkBuf, testDef->readBuf, 
										testDef->numSamples, 
										testDef->numChannels, byteOrder, sampleSize))
					FatalError("Audio buffers did not compare\n");
				break;

			case kMultiChannel:
				segmentSize = (testDef->numSamples / BUF_SEGMENTS) + 1;
				samplesLeft = testDef->numSamples;
				readXfer[0].buflen = testDef->readBuf->bufLen;
				readXfer[0].buffer = testDef->readBuf->buf;
				readXfer[1].buflen = testDef->readBuf2->bufLen;
				readXfer[1].buffer = testDef->readBuf2->buf;
				for(n = 1; n <= (BUF_SEGMENTS+1); n++)
				{
					if(samplesLeft < segmentSize)
					{
						segmentSize = samplesLeft;
					}
					segBytes = (segmentSize * sampleSize) / 8;
					readXfer[0].mediaKind = mediaKind;
					readXfer[0].subTrackNum = 1;
					readXfer[0].numSamples = segmentSize;
					readXfer[1].mediaKind = mediaKind;
					readXfer[1].subTrackNum = 2;
					readXfer[1].numSamples = segmentSize;
					CHECK(omfmReadMultiSamples(media, 2, readXfer));
	
					if((omfInt32)readXfer[0].samplesXfered != segmentSize)
						FatalError("Num samples xfered incorrect (ch 1)\n");
					if((omfInt32)readXfer[1].samplesXfered != segmentSize)
						FatalError("Num samples xfered incorrect (ch 2)\n");
					if((omfInt32)readXfer[0].bytesXfered != segBytes)
						FatalError("Num bytes xfered incorrect (ch 1)\n");
					if((omfInt32)readXfer[1].bytesXfered != segBytes)
						FatalError("Num bytes xfered incorrect (ch 2)\n");
					samplesLeft -= segmentSize;
					readXfer[0].buflen -= segBytes;
					readXfer[0].buffer = ((char *)(readXfer[0].buffer)) + segBytes;
					readXfer[1].buflen -= segBytes;
					readXfer[1].buffer = ((char *)(readXfer[1].buffer)) + segBytes;
				}
				if (!SoundCompare(testDef->checkBuf, testDef->readBuf, 
										testDef->numSamples, 1, byteOrder, sampleSize))
					FatalError("Audio buffers did not compare (left)\n");
				if (!SoundCompare(testDef->checkBuf2, testDef->readBuf2, 
										testDef->numSamples, 1, byteOrder, sampleSize))
					FatalError("Audio buffers did not compare (right)\n");
				break;
		
			case kPreInterleaved:
				CHECK(omfmReadRawData(media, testDef->numSamples, 
											 testDef->readBuf->bufLen, 
											 testDef->readBuf->buf, 
											 &bytesRead, &samplesRead));
				if (!SoundCompare(testDef->checkBuf, testDef->readBuf, 
										testDef->numSamples, 
										testDef->numChannels, byteOrder, sampleSize))
					FatalError("Audio buffers did not compare\n");
				break;
			
			case kANSIRead:	/* only works with 8-bit samples cross platform */
				CHECK(omfmGetMediaFilePos(media, &offset));

				{
					omfInt32		numSegments;
					omfPosition_t	tstPos;
					omfLength_t		tstLen;
				
					CHECK(omfmGetNumMediaSegments(media, &numSegments));
					CHECK(omfmGetMediaSegmentInfo(media, 1, NULL, NULL, &tstPos, &tstLen));
					if(numSegments != 1)
						FatalError("media is in segments\n");
					if(omfsInt64NotEqual(tstPos, offset))
						FatalError("omfmGetMediaSegmentInfo tstPos is wrong\n");
				}
				CHECK(omfsTruncInt64toUInt32(offset, &offset32));	/* OK */
				ansiPtr = fopen(fileName, "rb");
				fseek(ansiPtr, offset32, SEEK_SET);
				bytesRead = fread(testDef->readBuf->buf,
										testDef->readBuf->bufLen,1,ansiPtr);
				fclose(ansiPtr);
				for(n = 0; n < testDef->readBuf->bufLen; n++)
				{
					if(testDef->readBuf->buf[n] != testDef->checkBuf->buf[n])
					{
						printf("miscompare @ %ld\n", n);
						FatalError("Audio buffers did not compare");
					}
				}
				break;
			
			default:
				printf("Illegal xfer type on read\n");
				return((omfErr_t)2);
			}

			if(testDef->readXferType == kSingleChannel)
			{
				printf("Checking %s\n    '%s' (EOF Case).\n", codecName, testDef->testName);
				CHECK(omfmGotoShortFrameNumber(media, 1));
				status = omfmReadDataSamples(media, testDef->numSamples+1, 
												  testDef->readBuf->bufLen + (sampleSize/8), 
												  testDef->readBuf->buf, &bytesRead);
				if(status == OM_ERR_NONE)
				{
					FatalError("Missing expected EOF error\n");
				}
				else if((status != OM_ERR_EOF) && (status != OM_ERR_END_OF_DATA))
				{
					CHECK(status);
				}
				if((omfInt32)bytesRead != testDef->readBuf->bufLen)
					FatalError("BytesRead incorrect on EOF\n");
			}
		}
		else
		{
			printf("Illegal test type on check\n");
			return((omfErr_t)2);
		}

		CHECK(omfmMediaClose(media));

/*		printf("Checking Video (TIFF) Data\n");
		START_TIMER(ticksSpent);
		omfmGetNthMediaDesc(head, 1, &mt, &readDesc, &readUID);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		if (mt != imageMedia)
		{
			printf("Wrong media type on video media\n");
			return((omfErr_t)1);
		}
		if ((readDesc.image.format != imageStruct.format) ||
		    (readDesc.image.layout != imageStruct.layout) ||
		    (readDesc.image.width != imageStruct.width) ||
		    (readDesc.image.height != imageStruct.height) ||
		    (readDesc.image.editrate.numerator != 
                                     imageStruct.editrate.numerator) ||
		    (readDesc.image.editrate.denominator != 
			                            imageStruct.editrate.denominator) ||
		    (readDesc.image.bitsPerPixel != imageStruct.bitsPerPixel) ||
		    (readDesc.image.compression != imageStruct.compression) ||
		    (readDesc.image.JPEGtableID != imageStruct.JPEGtableID) ||
		    (readDesc.image.leadingLines != imageStruct.leadingLines) ||
		    (readDesc.image.trailingLines != imageStruct.trailingLines) ||
		    ((readDesc.image.name != imageStruct.name) 
			           && strcmp(readDesc.image.name, imageStruct.name)) ||
		    ((readDesc.image.filename != imageStruct.filename) 
                    && strcmp(readDesc.image.filename, imageStruct.filename)))
			FatalError("Bad image descriptor values\n");
		omfmSoftwareCodec(head, FALSE);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		omfmReadSamples(head, 1, stdReadBuffer, FRAME_BYTES, &samplesRead);
		if (OMErrorCode != OM_ERR_NONE)
			FatalErrorCode(__LINE__, __FILE__);
		if (memcmp(stdWriteBuffer, stdReadBuffer, FRAME_BYTES) != 0)
			FatalError("Video buffers did not compare\n");
		STOP_TIMER(ticksSpent);
*/	
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}


int ExecMediaWriteTests(char *filename, 
								mediaTestDef_t *testDefs, 
								omfInt32 numTests, 
								omfFileRev_t rev)
{

	omfSessionHdl_t sessionPtr;
	omfHdl_t        filePtr;
	omfInt32		test;
	omfProductIdentification_t ProductInfo;
#if PORT_SYS_MAC
	int 		startMem;

	MaxApplZone();
	startMem = FreeMem();
#endif
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Media UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

	XPROTECT(NULL)
	{		
		CHECK(omfsBeginSession(&ProductInfo, &sessionPtr));
		CHECK(omfmInit(sessionPtr));
		if (rev != kOmfRev2x)
		  CHECK(omfmRegisterCodec(sessionPtr, omfCodecAJPG, kOMFRegisterLinked));
#if AVID_CODEC_SUPPORT
	  	CHECK(omfmRegisterCodec(sessionPtr, omfCodecAvJPED, kOMFRegisterLinked));
	  	CHECK(omfmRegisterCodec(sessionPtr, omfCodecAvidJFIF, kOMFRegisterLinked));
#endif
	
		CHECK(omfsCreateFile((fileHandleType)filename, sessionPtr, rev, 
									&filePtr));
		
		for(test = 0; test < numTests; test++)
			CHECK(WriteMedia(filePtr, testDefs+test));

		printf("Closing write file\n");
		CHECK(omfsCloseFile(filePtr));
		CHECK(omfsEndSession(sessionPtr));
	}
	XEXCEPT
	{
#ifdef OMFI_ERROR_TRACE
		omfPrintStackTrace(filePtr);
#endif
	}
	XEND_SPECIAL(1)


	return (0);	
}

int ExecMediaCheckTests(char *name, 
								mediaTestDef_t *testDefs, 
								omfInt32 numTests)
{

	omfSessionHdl_t sessionPtr = NULL;
	omfHdl_t        filePtr = NULL;
	omfObject_t		masterMob = NULL;
	omfIterHdl_t	mobIter = NULL;
	omfMediaHdl_t	mediaPtr = NULL;
	char			mobName[64];
	omfSearchCrit_t	search;
	omfErr_t		status;
	omfDDefObj_t	nilKind, pictureKind, soundKind;
	omfInt32			test, offset;
	omfBool			found;
	
	XPROTECT(NULL)
	{
		CHECK(omfsBeginSession(NULL, &sessionPtr));
	
		CHECK(omfmInit(sessionPtr));
#if AVID_CODEC_SUPPORT
	  	CHECK(omfmRegisterCodec(sessionPtr, omfCodecAvJPED, kOMFRegisterLinked));
	  	CHECK(omfmRegisterCodec(sessionPtr, omfCodecAvidJFIF, kOMFRegisterLinked));
#endif
		
		CHECK(omfsOpenFile((fileHandleType)name, sessionPtr, &filePtr));
		
		printf("Opened read file\n");
		omfiDatakindLookup(filePtr, NODATAKIND, &nilKind, &status);
		CHECK(status);
		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureKind, &status);
		CHECK(status);
		omfiDatakindLookup(filePtr, SOUNDKIND, &soundKind, &status);
		CHECK(status);
	
		CHECK(omfiIteratorAlloc(filePtr, &mobIter));
		search.searchTag = kByMobKind;
		search.tags.mobKind = kMasterMob;
	
		while (omfiGetNextMob(mobIter, &search, &masterMob) == OM_ERR_NONE)
		{
			CHECK(omfiMobGetInfo(filePtr, masterMob, NULL, sizeof(mobName), 
										mobName, NULL, NULL));
													
			for(test = 0, found = FALSE; (test < numTests) && !found; test++)
			{
				offset = strlen(testDefs[test].codecID);
				if(strncmp(mobName, testDefs[test].codecID, offset) == 0)
				{
					if(strcmp(mobName+offset, testDefs[test].testName) == 0)
					{
						CHECK(CheckMedia(filePtr, masterMob, testDefs+test, name));
						found = TRUE;
					}
				}
			}		
			if(!found)
				printf("Found media from an unknown test\n");
		 }
		
		if(mobIter != NULL)
		{
			CHECK(omfiIteratorDispose(filePtr, mobIter));
		}
		printf("File checks out OK.\n");
	
		CHECK(omfsCloseFile(filePtr));
		CHECK(omfsEndSession(sessionPtr));
	}
	XEXCEPT
	{
#ifdef OMFI_ERROR_TRACE
		omfPrintStackTrace(filePtr);
#endif
	}
	XEND_SPECIAL(1)

	return(0);	
}


/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
