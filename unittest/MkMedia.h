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

#include "omPublic.h"

typedef enum
{
	kMultiChannel, kPreInterleaved, kSingleChannel, kLineXfer, kANSIRead, kSingleChannelChunks
} testXferType_t;

typedef enum
{
	kFramesSameSize, kFramesDifferentSize
} frameVary_t;

typedef struct
{
	omfUInt8	*buf;
	omfInt32	bufLen;
} testBuf_t;

typedef struct
{
	char				*testName;
	testXferType_t		writeXferType;
	testXferType_t		readXferType;
	omfCodecID_t		codecID;
	omfUniqueNamePtr_t	mediaKind;
	omfInt32			numSamples;
	omfInt32			numChannels;
	omfVideoMemOp_t		*videoWriteMemFormat;
	omfVideoMemOp_t		*videoWriteFileFormat;
	omfVideoMemOp_t		*videoReadMemFormat;
	omfAudioMemOp_t		*audioWriteMemFormat;
	omfAudioMemOp_t		*audioWriteFileFormat;
	omfAudioMemOp_t		*audioReadMemFormat;
	testBuf_t			*writeBuf;
	testBuf_t			*writeBuf2;
	testBuf_t			*readBuf;
	testBuf_t			*readBuf2;
 	testBuf_t			*checkBuf;
 	testBuf_t			*checkBuf2;
	omfFileRev_t		fileRev;
	omfCompressEnable_t	writeCompress;
	omfCompressEnable_t	readCompress;
	void				*codecPrivateData;
	omfInt16			codecPrivateDataLen;
	frameVary_t			frameVary;
	omfInt32			alignment;
} mediaTestDef_t;

int Write2xMedia(char *filename, omfFileRev_t rev);
int Check2xMedia(char *filename, omfFileRev_t rev);
int ExecMediaWriteTests(char *filename, mediaTestDef_t *tests, omfInt32 numTests, omfFileRev_t rev);
int ExecMediaCheckTests(char *filename, mediaTestDef_t *tests, omfInt32 numTests);


omfErr_t setupStandardTests(mediaTestDef_t **resultPtr, omfInt32 *numTests,
			    omfFileRev_t rev);
omfErr_t cleanupStandardTests(mediaTestDef_t **resultPtr, omfInt32 *numTests);

