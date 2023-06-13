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

   /*********************************************************/
   /*                                                       */
   /* OMF 2.0 Toolkit sample program for copying 1.x video  */
   /* from an omf file and writing it out to a separate 2.0 */
   /* file containing just a video master mob and copies of */
   /* every video file mob with the chains to copies of the */
   /* corresponding video data.                             */
   /*                                                       */
   /*********************************************************/

#include "masterhd.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(THINK_C) || defined(__MWERKS__)
#include <console.h>
#include <stdlib.h>
#include <Types.h>
#include <Fonts.h>
#include <QuickDraw.h>
#include <Windows.h>
#include <Menus.h>
#include <Dialogs.h>
#include <StandardFile.h>
#include "MacSupport.h"
extern GrafPtr thePort; 
#endif

#include "omPublic.h"
#include "omMedia.h"


#define check(a)  { omfErr_t e; e = a; if (e != OM_ERR_NONE) \
                    FatalErrorCode(e, __LINE__, __FILE__); }

#define ExitIfNull(b, m) \
  if (!b) { \
      printf("%s: %s\n", cmdname, m); \
      return(-1); \
   }

#define MakeRational(num, den, resPtr) \
        {(resPtr)->numerator = num; (resPtr)->denominator = den;}

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

int main(int argc, char *argv[])
{
   omfSessionHdl_t sessionPtr;
   omfHdl_t        InfilePtr, OutfilePtr;
   fileHandleType fh;
   omfObject_t     masterMob, OutfileMob, InMasterMob;
   omfRational_t   editRate;
   omfMediaHdl_t   InmediaPtr, OutmediaPtr;
   omfErr_t        omfError = OM_ERR_NONE;
   omfInt16        bitsPerPixel;
   omfInt32        omfWidth, omfHeight;
   char            *stdWriteBuffer = NULL;
   omfInt64        numSamplesLong;
   omfUInt32       numSamples, bytesRead, NeededSize;
   omfUInt32       stdWriteBufferSize = 0;
   char            Outname[256];
   omfIterHdl_t    masterMobIter = NULL, trackIter = NULL;
   omfUInt32        i;
   omfBool         isPrimary = 1;
   omfSearchCrit_t search;
   omfRational_t imageAspectRatio; 
   omfFrameLayout_t frameLayout;
   omfPixelFormat_t pixelFormat;
   omfInt32 numTracks, loop;
   omfObject_t track = NULL, trackSeg = NULL;
   omfDDefObj_t datakind = NULL;
   omfTrackID_t trackID;
   omfCodecID_t codecID;
   omfBool masterMobCreated = FALSE;
   char        *printfname;
   char *cmdname;
#if defined(THINK_C) || defined(__MWERKS__)
   Point where;
#endif
	omfProductIdentification_t ProductInfo;

	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Video Copy Example";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = 1;
	ProductInfo.platform = NULL;

#if defined(THINK_C) || defined(__MWERKS__)
   argc = ccommand(&argv);

   /*
	*	Standard Toolbox stuff
	*/
   MacInit();
   
   where.h = 20;
   where.v = 40;
#endif

   cmdname = argv[0];
   if (argc != 2) 
	 {
	   printf("Usage: %s <filename>\nWrites <filename>.vid\n", cmdname);
	   exit(-1);
	 }

   /* Initialize the session handle */
   check(omfsBeginSession(&ProductInfo, &sessionPtr));
	
   /* Initialize the media subsystem */
   check(omfmInit(sessionPtr));

   /* Get the OMF filename */
   fh = (fileHandleType)argv[1];
   printfname = argv[1];
   
   /* Open input file for reading media */
   check(omfsOpenFile(fh, sessionPtr, &InfilePtr));

   /* Create output file for writing media, and create master mob for it */
   sprintf(Outname, argv[1]);
   strcat(Outname, ".vid");
   check(omfsCreateFile((fileHandleType)Outname, sessionPtr, 
						kOmfRev2x, &OutfilePtr)); 

   XPROTECT(OutfilePtr) 
	 {
	   MakeRational(2997, 100, &editRate);
   
	   /* Prepare to iterate over all of the master mobs in the input file*/
	   CHECK(omfiIteratorAlloc(InfilePtr, &masterMobIter));
	   search.searchTag = kByMobKind;
	   search.tags.mobKind = kMasterMob;

	   CHECK(omfiGetNextMob(masterMobIter, &search, &InMasterMob));
   
	   /* Loop over all input master mobs */
	   while (InMasterMob) 
		 {
		   /* Iterate through master mob tracks, looking for video */
		   CHECK(omfiIteratorAlloc(InfilePtr, &trackIter));
		   CHECK(omfiMobGetNumTracks(InfilePtr, InMasterMob, &numTracks));
		   for (loop = 0; loop < numTracks; loop++)
			 {
			   CHECK(omfiMobGetNextTrack(trackIter, InMasterMob, NULL,&track));
			   CHECK(omfiTrackGetInfo(InfilePtr, InMasterMob, track, 
									  NULL, 0, NULL, NULL, 
									  &trackID, &trackSeg));
			   CHECK(omfiComponentGetInfo(InfilePtr, trackSeg, &datakind, 
										  NULL));
			   /* If this is a video track, copy the data */
			   if (omfiIsPictureKind(InfilePtr, datakind, kExactMatch, 
									 &omfError))
				 {
				   if (!masterMobCreated)
					 {
					   /* Create master mob in new file */
					   CHECK(omfmMasterMobNew(OutfilePtr, 
									  "The Master Mob", /* mob name        */
									  isPrimary,        /* isPrimary       */ 
									  &masterMob));
					 }
				   omfError = omfmMediaOpen(InfilePtr, InMasterMob, 
											trackID, /* Track ID */
											NULL, /* Media criteria */
											kMediaOpenReadOnly,
											kToolkitCompressionEnable, 
											&InmediaPtr);
				   if (InmediaPtr) 
					 {
					   printf("Found Video Track -- Copying Video Media...\n");
					   /* Duplicate the frame information */
					   CHECK(omfmGetVideoInfo(InmediaPtr, 
											  &frameLayout, 
											  &omfWidth, &omfHeight, 
											  &editRate, 
											  &bitsPerPixel, &pixelFormat));
					   NeededSize = omfWidth * omfHeight * 3;
					   /* For kSeparateFields & kMixedFields, height will 
						* be for one field.
						*/
					   if (frameLayout == kSeparateFields ||
						   frameLayout == kMixedFields)
						 NeededSize *= 2;

					   CHECK(omfmGetSampleCount(InmediaPtr, 
												&numSamplesLong));
					   CHECK(omfsTruncInt64toUInt32(numSamplesLong, 
													&numSamples));
					   CHECK(omfmMediaGetCodecID(InmediaPtr, &codecID));
					   if (numSamples <= 0) continue;

					   if (!stdWriteBuffer) 
						 {
						   stdWriteBuffer = (char *) omfsMalloc(NeededSize);
						   stdWriteBufferSize = NeededSize;
						 } 
					   else if (stdWriteBufferSize < NeededSize) 
						 {
						   stdWriteBuffer = (char *) realloc(stdWriteBuffer, 
															 NeededSize);
						   stdWriteBufferSize = NeededSize;
						 }
					   if (stdWriteBuffer == NULL) 
						 {
						   printf("Out of memory, sorry.\n");
						   return(1);
						 }

					   /* Create a corresponding file mob for output file */
					   CHECK(omfmFileMobNew(OutfilePtr, 
											"File Mob", 
											editRate, 
											codecID,
											&OutfileMob)); 
         
					   /* Create the media object for the output data */
					   MakeRational(4, 3, &imageAspectRatio);
					   CHECK(omfmVideoMediaCreate(OutfilePtr,  masterMob, 
												  trackID,/* master track id */
												  OutfileMob,   
												  kToolkitCompressionEnable, 
												  editRate, 
												  omfHeight, omfWidth, 
												  kFullFrame, 
												  imageAspectRatio, 
												  &OutmediaPtr));   

					   for (i=0; i<numSamples; i++) 
						 {
						   /* Read the data to be copied to the Outfile */
						   CHECK(omfmReadDataSamples(InmediaPtr, 1, 
													 NeededSize, 
													 stdWriteBuffer, 
													 &bytesRead));
             
						   /* Write the video data */
						   CHECK(omfmWriteDataSamples(OutmediaPtr, 1,
													  stdWriteBuffer, 
													  bytesRead));
						 } /* for loop to read samples */

					   /* Clean up for reuse */
					   if (OutmediaPtr)
						 CHECK(omfmMediaClose(OutmediaPtr));

					 } /* If media on track */
				 
				   /* Clean up for reuse */
				   if (InmediaPtr)
					 CHECK(omfmMediaClose(InmediaPtr));

				 } /* If Video Media */
			 } /* For number of tracks */

		   CHECK(omfiIteratorDispose(InfilePtr, trackIter));
		   trackIter = NULL;
		   /* Iterate to the next file mob */
		   masterMobCreated = FALSE;
		   omfError = omfiGetNextMob(masterMobIter, &search, &InMasterMob);
		   if ((omfError == OM_ERR_NO_MORE_OBJECTS) && (InMasterMob == NULL))
			 {
			   omfError = OM_ERR_NONE;
			   break;
			 } 
		   else if (omfError != OM_ERR_NONE) 
			 {
			   RAISE(omfError);
			 }
		 } /* While master mob */

	   CHECK(omfiIteratorDispose(InfilePtr, masterMobIter));
	   masterMobIter = NULL;
	 }  /* XPROTECT */

   XEXCEPT 
	 {
	   printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
	   printf("CopyVideo failed, sorry.\n");
	   if (trackIter)
		 omfiIteratorDispose(InfilePtr, trackIter);
	   if (masterMobIter)
		 omfiIteratorDispose(InfilePtr, masterMobIter);
	   check(omfsCloseFile(InfilePtr));
	   check(omfsCloseFile(OutfilePtr));
	   check(omfsEndSession(sessionPtr));
	   if (stdWriteBuffer)
		 omfsFree(stdWriteBuffer);
	   stdWriteBuffer = NULL;
	   return(1);
	 } XEND;
   
   /* Close out files and session */
   check(omfsCloseFile(InfilePtr));
   check(omfsCloseFile(OutfilePtr));
   check(omfsEndSession(sessionPtr));

   if (stdWriteBuffer)
	 omfsFree(stdWriteBuffer);
   stdWriteBuffer = NULL;

   printf("CopyVideo completed.\n");
   return(0);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
