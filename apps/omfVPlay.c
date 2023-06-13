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
/*------------------------------------------------------------------------------
#
#
------------------------------------------------------------------------------*/

#include "masterhd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AVID_CODEC_SUPPORT 1

#if defined(THINK_C) || defined(__MWERKS__)
#include <console.h>
 #include <Types.h>
 #include <StandardFile.h>
#else
#include <unistd.h>
#endif

#include "omPublic.h"
#include "omMedia.h"
#if AVID_CODEC_SUPPORT
#include "omCodec.h"
#include "omcAvJPED.h"
#include "omcAvidJFIF.h"
#endif

#if defined(THINK_C) || defined(__MWERKS__)
#include "MacSupport.h"
#include "MacPictureIO.h"
static omfBool doEventLoop( void );
#else
#include "SGIPictureIO.h"
#endif
#if defined(__MWERKS__)		/* Metrowerks only */
#include <SIOUX.h>
#endif



/* Prototypes */
void Initialize(void);
omfErr_t DisplayImage(omfInt32 width, omfInt32 height, omfInt32 displayHeight, omfInt32 lineLen,
						omfMediaHdl_t mediaPtr, omfHdl_t file, winDataPtr_t windowData);


void main(int argc, char *argv[])
{
	
	omfSessionHdl_t sessionPtr = NULL;
	omfHdl_t        filePtr = NULL;
	omfObject_t		masterMob = NULL;
	omfIterHdl_t	mobIter = NULL, trackIter = NULL;
	omfMediaHdl_t	mediaPtr = NULL;
	omfFrameLayout_t frameLayout;
	omfInt32		bufWidth;
	omfInt32		bufHeight, displayHeight;
	omfRational_t	editRate;
	omfPixelFormat_t pixelFormat;
	omfInt16        bitsPerPixel;
	omfDDefObj_t	pictureKind;
    omfErr_t     	status;
	omfSearchCrit_t search;
	omfVideoMemOp_t	memFmt[6];
	omfFrameLayout_t outputFrameLayout;
    omfInt16		numCh;
    omfNumTracks_t	n, numTracks;
	char			name[64];
	omfSearchCrit_t	searchCrit;
	omfMSlotObj_t	track;
	omfTrackID_t	trackID;
    char			errBuf[256];
	omfRect_t displayRect;
	omfBool			stripBlank = TRUE;
	winDataPtr_t	windowData;
	omfInt32		lineLen;
	
#if defined(THINK_C) || defined(__MWERKS__)
   	Point where;	
  	SFReply inFS;
	
 	argc = ccommand(&argv);
	MacInit();
#if defined(__MWERKS__)
	SIOUXSettings.standalone = FALSE;
	SIOUXSettings.autocloseonquit = TRUE;
	SIOUXSettings.setupmenus = FALSE;
#endif
#else
   char *filename;
#endif

	XPROTECT(NULL)
	{
	    outputFrameLayout = kFullFrame;
#if defined(THINK_C) || defined(__MWERKS__)
		if(argc >= 2)
#else
		if(argc >= 3)
#endif
		{
			stripBlank = FALSE;
			if(strcmp(argv[1], "-field") == 0)
				outputFrameLayout = kOneField;
			else if(strcmp(argv[1], "-frame") == 0)
				outputFrameLayout = kFullFrame;
			else if(strcmp(argv[1], "-separate") == 0)
				outputFrameLayout = kSeparateFields;
			else if(strcmp(argv[1], "-mixed") == 0)
				outputFrameLayout = kMixedFields;
			else
				printf("Illegal	flag\n");
#if !defined(THINK_C) && !defined(__MWERKS__)
			filename = argv[2];
#endif
		}
		else
		{
#if !defined(THINK_C) && !defined(__MWERKS__)
		  filename = argv[1];
		  stripBlank = TRUE;
#endif
		}
		
		pctInit();
		
		CHECK(omfsBeginSession(NULL, &sessionPtr));
		
		CHECK(omfmInit(sessionPtr));
#if AVID_CODEC_SUPPORT
	  	CHECK(omfmRegisterCodec(sessionPtr, omfCodecAvJPED, kOMFRegisterLinked));
	  	CHECK(omfmRegisterCodec(sessionPtr, omfCodecAvidJFIF, kOMFRegisterLinked));
#endif
				
#if defined(THINK_C) || defined(__MWERKS__)
		where.h = 20;
		where.v = 40;
	
	  /* Get the file name and open */
		SFGetFile(where,  NULL, NULL, -1, NULL, NULL,  &inFS);
		if (!inFS.good)
		{
			CHECK(OM_ERR_BADOPEN); /* force error */
		}
	
		CHECK(omfsOpenFile((fileHandleType)&inFS, sessionPtr, &filePtr));
#else
		CHECK(omfsOpenFile((fileHandleType)filename, sessionPtr, &filePtr));
#endif
#if OMFI_ENABLE_SEMCHECK
		omfsSemanticCheckOff(filePtr);
#endif		
		CHECK(omfiIteratorAlloc(filePtr, &mobIter));
		
		search.searchTag = kByMobKind;
		search.tags.mobKind = kMasterMob;
		status = omfiGetNextMob(mobIter, &search, &masterMob);
		while((status == OM_ERR_NONE) && (masterMob != NULL))
		{
			omfiDatakindLookup(filePtr, PICTUREKIND, &pictureKind, &status);

			CHECK(omfiMobGetNumTracks(filePtr, masterMob, &numTracks));
			CHECK(omfiIteratorAlloc(filePtr, &trackIter));
			searchCrit.searchTag = kByDatakind;
			strcpy(searchCrit.tags.datakind, PICTUREKIND);
			for(n = 1; n <= numTracks; n++)
			{
			
				if(omfiMobGetNextTrack(trackIter, masterMob, &searchCrit, 
									   &track) == OM_ERR_NO_MORE_OBJECTS)
					break;
					
				CHECK(omfiTrackGetInfo(filePtr, masterMob, track, NULL, 0, 
									   NULL, NULL, &trackID,
										NULL));
				CHECK(omfmGetNumChannels(	
							filePtr,		/* IN -- For this file */
							masterMob,		/* IN -- In this fileMob */
							trackID,
							NULL,
							pictureKind,	/* IN -- for this media type */
							&numCh));		/* OUT -- How many channels? */
				if(numCh != 0)
				{
					CHECK(omfiMobGetInfo(filePtr, masterMob, NULL, 
										 sizeof(name), name, NULL, NULL));
	
					status = omfmMediaOpen(filePtr, masterMob, 
							    trackID, NULL, 
								kMediaOpenReadOnly,
							    kToolkitCompressionEnable, 
								&mediaPtr);
					if(status == OM_ERR_MEDIA_NOT_FOUND)
					{
						printf("    <Media not found>\n");
						continue;
					}
					else if(status != OM_ERR_NONE)
						RAISE(status);
							
					CHECK(omfmGetVideoInfo(mediaPtr,
								     &frameLayout,
								     &bufWidth,
								     &bufHeight,
								     &editRate,
								     &bitsPerPixel,
								     &pixelFormat));
				
					if(stripBlank)
					{
						CHECK(omfmGetDisplayRect(mediaPtr, &displayRect));
						displayHeight = bufHeight - displayRect.yOffset;
						}
					else
						displayHeight = bufHeight;
					
				    if(outputFrameLayout != kOneField)
					{
						if((frameLayout == kSeparateFields) ||
						   (frameLayout == kMixedFields) ||
						   (frameLayout == kOneField))
						  {
							bufHeight *= 2;
							displayHeight *= 2;
						  }
					}
					else if(frameLayout == kFullFrame)
					  {
						bufHeight /= 2;
						displayHeight /= 2;
					  }

					CHECK(pctOpenWindow(bufWidth, displayHeight, 32, name, &windowData));
					/* Ask for the media as one frame (re-interlace if needed)
					 */
					memFmt[0].opcode = 	kOmfFrameLayout;
					memFmt[0].operand.expFrameLayout = outputFrameLayout;
					
					memFmt[1].opcode = 	kOmfPixelFormat;
					memFmt[1].operand.expPixelFormat = kOmfPixRGBA;
										
					memFmt[2].opcode = kOmfRGBCompLayout;
#if defined(THINK_C) || defined(__MWERKS__)
					memFmt[2].operand.expCompArray[0] = 'A';
					memFmt[2].operand.expCompArray[1] = 'R';
					memFmt[2].operand.expCompArray[2] = 'G';
					memFmt[2].operand.expCompArray[3] = 'B';
#else
					memFmt[2].operand.expCompArray[0] = 'A';
					memFmt[2].operand.expCompArray[1] = 'B';
					memFmt[2].operand.expCompArray[2] = 'G';
					memFmt[2].operand.expCompArray[3] = 'R';
#endif
					memFmt[2].operand.expCompArray[4] = 0;
					memFmt[3].opcode = kOmfRGBCompSizes;
					memFmt[3].operand.expCompSizeArray[0] = 8;
					memFmt[3].operand.expCompSizeArray[1] = 8;
					memFmt[3].operand.expCompSizeArray[2] = 8;
					memFmt[3].operand.expCompSizeArray[3] = 8;
					memFmt[3].operand.expCompSizeArray[4] = 0;
					
#if defined(THINK_C) || defined(__MWERKS__)
					if(pctHandlesDirectFrameBuf(windowData))
					{
						CHECK(pctFrameBufLineLen(windowData, &lineLen));
						memFmt[4].opcode = kOmfLineLength;
						memFmt[4].operand.expInt32 = lineLen;
						
						memFmt[5].opcode = 	kOmfVFmtEnd;
					}
					else
					{
						memFmt[4].opcode = 	kOmfVFmtEnd;
						lineLen = bufWidth * 4;
					}
#else
					memFmt[4].opcode = 	kOmfVFmtEnd;
					lineLen = bufWidth * 4;
#endif
			
					CHECK(omfmSetVideoMemFormat(mediaPtr, memFmt));
					CHECK(DisplayImage(bufWidth, bufHeight, displayHeight, lineLen, mediaPtr, filePtr, windowData));
				}
			  }
			CHECK(omfiIteratorDispose(filePtr, trackIter));
			trackIter = NULL;
			status = omfiGetNextMob(mobIter, &search, &masterMob);
		}

#if defined(THINK_C) || defined(__MWERKS__)
		while(doEventLoop())
			;
#else
		sginap(600);
#endif

		CHECK(pctCloseAllWindows());
		CHECK(omfiIteratorDispose(filePtr, mobIter));
		mobIter = NULL;
		CHECK(omfsCloseFile(filePtr));
		CHECK(omfsEndSession(sessionPtr));
	}
	XEXCEPT
	{
	  if (trackIter)
		omfiIteratorDispose(filePtr, trackIter);
	  if (mobIter)
		omfiIteratorDispose(filePtr, mobIter);

		printf("***ERROR: %d: %s\n", XCODE(), 
		       omfsGetExpandedErrorString(filePtr, XCODE(), 
						  sizeof(errBuf), errBuf));
	}
	XEND_VOID;
}

omfErr_t DisplayImage(omfInt32 width, omfInt32 height, omfInt32 displayHeight, omfInt32 lineSize,
					omfMediaHdl_t mediaPtr, omfHdl_t file, winDataPtr_t windowData)
{
	/* now comes the moment you've all been waiting for */
	unsigned char	*buffer = NULL;
	int  			bsize;
	omfUInt32 		bytesRead;
	omfErr_t		status;
	omfLength_t 	numSamples;
	omfInt32 		numSamples32, loop, startOffset;
	omfBool			keepRunning = TRUE;
#if defined(THINK_C) || defined(__MWERKS__)
	omfBool			directFrameBufWrites;
#endif
    char			errBuf[256];
	
	XPROTECT(NULL)
	{
#if defined(THINK_C) || defined(__MWERKS__)
		directFrameBufWrites = pctHandlesDirectFrameBuf(windowData);
#endif
		bsize = height * lineSize;
#if defined(THINK_C) || defined(__MWERKS__)
		if(!directFrameBufWrites)
#endif
		{
			buffer = (unsigned char *) omfsMalloc(bsize);
			if(buffer == NULL)
				RAISE(OM_ERR_NOMEMORY);
		}
		
#if DM_TRACE
		printf("Allocated %ld bytes.\n",bsize);
#endif
		status = omfmGetSampleCount(mediaPtr, &numSamples);
		omfsTruncInt64toInt32(numSamples, &numSamples32);

		
		startOffset = (height - displayHeight) * lineSize;

		for (loop = 1; (loop <= numSamples32) && keepRunning; loop++)
		  {
			printf("Read frame %ld of %ld\n", loop, numSamples32);
#if defined(THINK_C) || defined(__MWERKS__)
			keepRunning = doEventLoop();
#endif
#if defined(THINK_C) || defined(__MWERKS__)
			if(directFrameBufWrites)
			{
				CHECK(pctFrameBufStartWrite(windowData, &buffer));
			}
#endif

			status = omfmReadDataSamples(mediaPtr,	/* IN -- */
										 1,	/* IN -- */
										 bsize,	/* IN -- */
										 (void *)buffer,	/* IN/OUT -- */
										 &bytesRead);	/* OUT -- */
			if(status == OM_ERR_NONE)
			  {
#if DM_TRACE
				printf("Read %ld bytes from disk.\n",bytesRead);
#endif
#if defined(THINK_C) || defined(__MWERKS__)
				keepRunning = doEventLoop();
#endif
#if defined(THINK_C) || defined(__MWERKS__)
				if(directFrameBufWrites)
				{
					CHECK(pctFrameBufEndWrite(windowData));
				}
				else
#endif
				{
					CHECK(pctDisplayRGB(width, displayHeight, buffer+startOffset, windowData));
				}
				printf("    Displaying frame %ld\n", loop);
			  }
			else
			{
				printf("***ERROR reading data: %d: %s\n", status, 
		       omfsGetExpandedErrorString(file, status, 
						  sizeof(errBuf), errBuf));
		    }
		  }
		omfsFree(buffer);
	}
	XEXCEPT
	XEND;
	
	return(OM_ERR_NONE);
}



#if defined(THINK_C) || defined(__MWERKS__)
static omfBool doEventLoop( void )
{
	EventRecord event;
	WindowPtr   window;
	short       clickArea;
	Rect        screenRect;
	char		msg;
	
	while(GetNextEvent( everyEvent, &event))
	{
		if (event.what == mouseDown){
			clickArea = FindWindow( event.where, &window );
			
			if (clickArea == inDrag){
				screenRect = (**GetGrayRgn()).rgnBBox;
				DragWindow( window, event.where, &screenRect );
			}
			else if (clickArea == inContent){
				if (window != FrontWindow())
					SelectWindow( window );
			}
			else if (clickArea == inGoAway)
				if (TrackGoAway( window, event.where ))
					return(FALSE);
		}
		else if(event.what == keyDown)
		{
			msg = event.message & charCodeMask;
			if(event.modifiers & cmdKey)
			{
				if((msg == 'q') || (msg == 'Q') || (msg == '.'))
					return(FALSE);
			}
		}
		else if (event.what == updateEvt){
			window = (WindowPtr)event.message;	
			SetPort( window );				
			BeginUpdate( window );
			drawWindow(window);
			EndUpdate( window );		
		}
#if defined(__MWERKS__)
		else
		{
			SIOUXHandleOneEvent(&event);
		}
#endif
	}
	
	return(TRUE);
}
#endif

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
