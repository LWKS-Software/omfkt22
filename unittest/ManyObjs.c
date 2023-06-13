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
/********************************************************************
 * ManyObjs - This unittest creates a file of the given revision, and
 *            creates a fake composition with many fill objects.
 *            It is attempting to test reading and writing many objects.
 ********************************************************************/
#include "masterhd.h"

#define TRUE 1
#define FALSE 0
#define LOTSOFOBJS 100

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
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

#ifdef MAKE_TEST_HARNESS
int ManyObjs(char *filename, char *version)
{
	int	argc;
	char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t	session;
    omfHdl_t fileHdl = NULL;
    omfFileRev_t rev;
    omfMobObj_t compMob = NULL;
    omfObject_t aSequence = NULL, vSequence = NULL, filler = NULL;
    omfTrackID_t trackID;
    omfObject_t aTrack = NULL, vTrack = NULL;
    omfRational_t editRate;
    omfDDefObj_t pictureDef = NULL;
    omfDDefObj_t soundDef = NULL;
    omfInt32 loop;
    omfPosition_t zeroPos;
    omfLength_t length10;
    omfErr_t omfError = OM_ERR_NONE;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Many Objects UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;
	
    XPROTECT(NULL)
      {
#ifdef MAKE_TEST_HARNESS
	argc = 3;
	argv[1] = filename;
	argv[2] = version;
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
	if (argc < 3)
	  {
	    printf("ManyObjs Usage: ManyObjs <omffile> <1|2|IMA>\n");
	    exit(1);
	  }

	if (!strcmp(version,"1"))
	  rev = kOmfRev1x;
	else if (!strcmp(version,"2"))
	  rev = kOmfRev2x;
	else if (!strcmp(version,"IMA"))
	  rev = kOmfRevIMA;
	else 
	  {
		printf("*** ERROR - Illegal file rev\n");
		return(1);
	  }

	CHECK(omfsBeginSession(&ProductInfo, &session));
	CHECK(omfsCreateFile((fileHandleType) argv[1], session, rev, 
			     &fileHdl));

	/* Initialize the editrate to NTSC */
	editRate.numerator = 2997;
	editRate.denominator = 100;
	omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
	omfiDatakindLookup(fileHdl, SOUNDKIND, &soundDef, &omfError);
	omfsCvtInt32toPosition(0, zeroPos);
	omfsCvtInt32toLength(10, length10);

	CHECK(omfiCompMobNew(fileHdl, "Simple Composition", TRUE, &compMob));
	CHECK(omfiSequenceNew(fileHdl, soundDef, &aSequence));
	CHECK(omfiSequenceNew(fileHdl, pictureDef, &vSequence));

	/* Create audio track with LOTSOFOBJS */
	for (loop = 1; loop <= LOTSOFOBJS; loop++)
	  {
	    CHECK(omfiFillerNew(fileHdl, soundDef, length10, &filler));
	    CHECK(omfiSequenceAppendCpnt(fileHdl, aSequence, filler));
	  }

	trackID = 1;
	CHECK(omfiMobAppendNewTrack(fileHdl, compMob, editRate, aSequence, 
				    zeroPos, trackID, NULL, /* name */
				    &aTrack));

	/* Create audio track with LOTSOFOBJS */
	for (loop = 1; loop <= LOTSOFOBJS; loop++)
	  {
	    CHECK(omfiFillerNew(fileHdl, pictureDef, length10, &filler));
	    CHECK(omfiSequenceAppendCpnt(fileHdl, vSequence, filler));
	  }

	trackID = 2;
	CHECK(omfiMobAppendNewTrack(fileHdl, compMob, editRate, vSequence, 
				    zeroPos, trackID, NULL, /* name */
				    &vTrack));

	CHECK(omfsCloseFile(fileHdl));
	omfsEndSession(session);
	printf("ManyObjs completed successfully.\n");

      } /* XPROTECT */

    XEXCEPT
      {
	if (fileHdl)
	  {
	    omfsCloseFile(fileHdl);
	    omfsEndSession(session);
	  }
	printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
	return(-1);
      }
    XEND;
    return(0);
}
