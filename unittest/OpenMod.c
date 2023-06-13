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
#define TRUE 1
#define FALSE 0

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
#include "omMedia.h"

#include "UnitTest.h"

#if PORT_SYS_MAC
#include <console.h>
#include <stdlib.h>
#include <Types.h>
#include <Fonts.h>
#include <QuickDraw.h>
#include <Windows.h>
#include <Menus.h>
#include <Dialogs.h>
#include <StandardFile.h>
extern GrafPtr thePort; 
#endif

#define STD_WIDTH		128L
#define STD_HEIGHT		96L
#define FRAME_SAMPLES	(STD_WIDTH * STD_HEIGHT)
#define FRAME_BYTES		(FRAME_SAMPLES * 3L)
#define NUM_FRAMES		6

#define OPEN_MODIFY_TEST	"OPEN_MODIFY_TEST"

typedef struct
{
	omfUInt8	*buf;
	omfInt32	bufLen;
} testBuf_t;

static testBuf_t           logoWriteBuffer;
static testBuf_t           videoReadBuffer;


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

#ifdef MAKE_TEST_HARNESS
int OpenMod(char *filename)
{
    int argc;
    char *argv[2];
#else
int main(int argc, char *argv[])
{
#endif
	omfSessionHdl_t		session = NULL;
	omfHdl_t 			fileHdl = NULL;
	omfInt32 			nameSize = 32;
	omfInt32 			tabLevel = 0, n;
	omfErr_t 			omfError = OM_ERR_NONE;
	omfRational_t		imageAspectRatio;	
	omfIterHdl_t		mobIter = NULL, refIter = NULL;
	omfDDefObj_t		pictureKind;
	omfRational_t		videoRate;
	omfObject_t			masterMob, fileMob;
	omfMediaHdl_t		mediaPtr;
	omfSearchCrit_t		search;
	omfMediaCriteria_t	mediaCrit;
	char				mobName[64];
	omfPosition_t		masterOffset;
	omfSearchCrit_t			criteria;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Open For Modify UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

	XPROTECT(NULL)
	  {
#ifdef MAKE_TEST_HARNESS
	    argc = 2;
	    argv[1] = filename;
#else
#if PORT_MAC_HAS_CCOMMAND
	    argc = ccommand(&argv); 

	    /*
	     *	Standard Toolbox stuff
	     */
#if PORT_MAC_QD_SEPGLOBALS
	    InitGraf	(&thePort);
#else
	    InitGraf	(&qd.thePort);
#endif
	    InitFonts	();
	    InitWindows	();
	    InitMenus	();
	    TEInit();
	    InitDialogs	((int )NULL);
	    InitCursor	();
	    
	    where.h = 20;
	    where.v = 40;
#endif
#endif

		if (argc < 2)
	 	{
		printf("*** ERROR - missing file name\n");
		return(1);
	  	}

		MakeRational(4, 3, &imageAspectRatio);
		MakeRational(2997, 100, &videoRate);

		CHECK(allocTestBuf(FRAME_BYTES, &logoWriteBuffer));
		CHECK(allocTestBuf(FRAME_BYTES, &videoReadBuffer));
		FillBufLogo(&logoWriteBuffer, STD_HEIGHT, STD_WIDTH);

		CHECK(omfsBeginSession(&ProductInfo, &session));
		CHECK(omfmInit(session));
		
		CHECK(omfsModifyFile((fileHandleType)argv[1], session, &fileHdl));

		omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureKind, &omfError);
		CHECK(omfError);

		CHECK(omfmMasterMobNew(	fileHdl, 					/* file (IN) */
								OPEN_MODIFY_TEST, 	/* name (IN) */
								TRUE,						/* isPrimary (IN) */
								&masterMob)); 				/* result (OUT) */
		
		CHECK(omfmFileMobNew(	fileHdl,	 		/* file (IN) */
				                OPEN_MODIFY_TEST, 	/* name (IN) */
				                videoRate,   		/* editrate (IN) */
				                CODEC_TIFF_VIDEO, 	/* codec (IN) */
				                &fileMob));   		/* result (OUT) */
	
		CHECK(omfmVideoMediaCreate(fileHdl, masterMob, 1, fileMob, 
											kToolkitCompressionDisable, 
											videoRate, STD_HEIGHT, STD_WIDTH, kFullFrame, 
											imageAspectRatio,
											&mediaPtr));
							  
		CHECK(omfmWriteDataSamples(mediaPtr, 1, logoWriteBuffer.buf, 
											logoWriteBuffer.bufLen));
			
		CHECK(omfmMediaClose(mediaPtr));

		CHECK(omfsCloseFile(fileHdl));
	    	CHECK(omfsEndSession(session));

		for(n = 2; n <= NUM_FRAMES; n++)
		{
			printf("Appending frame number %ld\n", n);
			CHECK(omfsBeginSession(&ProductInfo, &session));
			CHECK(omfmInit(session));
			
			CHECK(omfsModifyFile((fileHandleType)argv[1], session, &fileHdl));

			omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureKind, &omfError);
			CHECK(omfError);

			CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
			search.searchTag = kByMobKind;
			search.tags.mobKind = kMasterMob;
		
			while (omfiGetNextMob(mobIter, &search, &masterMob) == OM_ERR_NONE)
			{
				CHECK(omfiMobGetInfo(fileHdl, masterMob, NULL, sizeof(mobName),
									 mobName, NULL, NULL));
				
				if(strcmp(mobName, OPEN_MODIFY_TEST) == 0)
				{
					omfMSlotObj_t	track;
					omfTrackID_t		trackID;
					omfObject_t		sourceClip;
					omfSegObj_t		seg;

					CHECK(omfiIteratorAlloc(fileHdl, &refIter));

					mediaCrit.type = kOmfFastestRepresentation;
					fileMob = NULL;
					criteria.searchTag = kNoSearch;
					CHECK(omfiMobGetNextTrack(refIter, masterMob, &criteria, &track));
					CHECK(omfiTrackGetInfo(fileHdl, masterMob, track, NULL, 0, 
												  NULL, &masterOffset, &trackID, &seg));
					CHECK(omfmGetCriteriaSourceClip(fileHdl, masterMob, trackID, 
															  &mediaCrit, &sourceClip));
					CHECK(omfiSourceClipResolveRef(fileHdl, sourceClip, &fileMob));
					
					CHECK(omfiIteratorDispose(fileHdl, refIter));
					refIter = NULL;

					CHECK(omfmMediaOpen(fileHdl, masterMob, 1, &mediaCrit, kMediaOpenAppend,
									kToolkitCompressionDisable,
									&mediaPtr));
					CHECK(omfmWriteDataSamples(mediaPtr, 1, 
							   logoWriteBuffer.buf, logoWriteBuffer.bufLen));
					CHECK(omfmMediaClose(mediaPtr));
					break;
				}
					
			}

			if (mobIter)
			  CHECK(omfiIteratorDispose(fileHdl, mobIter));
			mobIter = NULL;
	
			CHECK(omfsCloseFile(fileHdl));
		    CHECK(omfsEndSession(session));
	    	}
	    	
		/* !!! Add verify pass to make sure that all data is present */
		CHECK(freeTestBuf(&logoWriteBuffer));
		CHECK(freeTestBuf(&videoReadBuffer));
		printf("OpenMod completed successfully.\n");
	} /* XPROTECT */
	XEXCEPT
	{	  
	    printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));

		 if (mobIter)
			omfiIteratorDispose(fileHdl, mobIter);
		 if (refIter)
			omfiIteratorDispose(fileHdl, refIter);

	    if(fileHdl != NULL)
	    {
	    	CHECK(omfsCloseFile(fileHdl));
	    }
	    if(session != NULL)
	    {
	    	CHECK(omfsEndSession(session));
	    }
	    return(-1);
	  }
	XEND;
	return(0);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
