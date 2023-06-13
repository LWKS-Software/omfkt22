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
 * Patch1x - This unittest test the omfsPatch1xMobs() function which
 *           fixes editrates, etc. on mobs that weren't built bottom up.
 ********************************************************************/

#include "masterhd.h"
#include <stdio.h>
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

/* Global Variables */
static omfSourceRef_t nullSource;

#ifdef MAKE_TEST_HARNESS
int Patch1x(char *filename, char *version)
{
	int	argc;
	char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t session;
    omfHdl_t fileHdl;
    omfMobObj_t compMob = NULL;
    omfLength_t clipLen;
    omfObject_t sequence, vFiller1, vFiller2, vTrack1;
    omfDDefObj_t pictureDef;
    omfPosition_t         zeroPos;
    omfTrackID_t trackID;
    omfRational_t editRate;
    omfErr_t omfError = OM_ERR_NONE;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Patch Mob UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

    XPROTECT(NULL)
      {
#ifdef MAKE_TEST_HARNESS
	argc = 3;
	argv[1] = filename;
#else
#if PORT_MAC_HAS_CCOMMAND
	argc = ccommand(&argv); 

	/*
	 *	Standard Toolbox stuff
	 */
#if PORT_QD_SEPGLOBALS
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
	    printf("Patch1x Usage: Patch1x <omffile>\n");
	    exit(1);
	  }

	CHECK(omfsBeginSession(&ProductInfo, &session));
	CHECK(omfsCreateFile((fileHandleType) argv[1], session, kOmfRev1x, 
			     &fileHdl));

	/* Initialize the editrate to NTSC */
	editRate.numerator = 2997;
	editRate.denominator = 100;

	omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
	omfsCvtInt32toInt64(512, &clipLen);
	omfsCvtInt32toPosition(0, zeroPos);

	/* create the composition */
	CHECK(omfiCompMobNew(fileHdl, "Simple Composition", TRUE, &compMob));

	/* create the picture track */
	CHECK(omfiSequenceNew(fileHdl, pictureDef, &sequence));
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen, &vFiller1));
	CHECK(omfiSequenceAppendCpnt(fileHdl, sequence, vFiller1));
	trackID = 1;
	CHECK(omfiMobAppendNewTrack(fileHdl, compMob, editRate, sequence, 
				    zeroPos, trackID, "VTrack", &vTrack1));

	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen, &vFiller2));
	CHECK(omfiSequenceAppendCpnt(fileHdl, sequence, vFiller2));


	CHECK(omfsPatch1xMobs(fileHdl));
	CHECK(omfsCloseFile(fileHdl));
	CHECK(omfsEndSession(session));
      }

    XEXCEPT
      {
	printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
	return(-1);
      }
    XEND;
    return(0);
}


