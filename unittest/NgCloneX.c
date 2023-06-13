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
 * NgCloneX - This negative unittest takes a file as input, creates a 
 *            file of the opposite type and makes sure that 
 *            omfiMobCloneExternal returns the correct error if it tries 
 *            to clone mobs from a file of one type to another.
 ********************************************************************/

#include "masterhd.h"
#define TRUE 1
#define FALSE 0

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
#include "omMobMgt.h"

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
int NgCloneX(char *srcName, char *destName)
{
	int	argc;
	char	*argv[3];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t	session;
    omfHdl_t fileHdl, fileHdl2;
    omfMobObj_t compMob, newMob;
    omfInt32 loop, numMobs;
    omfIterHdl_t mobIter = NULL;
    omfSearchCrit_t searchCrit;
    omfFileRev_t fileRev;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Negative Clone External UnitTest (Should fail import)";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;
        
    XPROTECT(NULL)
      {
#ifdef MAKE_TEST_HARNESS
	argc = 3;
	argv[1] = srcName;
	argv[2] = destName;
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
	    printf("*** ERROR - missing <copyin> <copyout> file name\n");
	    exit(1);
	  }

	CHECK(omfsBeginSession(&ProductInfo, &session));
	CHECK(omfsOpenFile((fileHandleType) argv[1], session, &fileHdl));

	CHECK(omfsFileGetRev(fileHdl, &fileRev));
	if (fileRev == kOmfRev2x)
	  {
		CHECK(omfsCreateFile((fileHandleType) argv[2], session, kOmfRev1x, 
							 &fileHdl2));
	  }
	else /* kOmfRev1x */
	  {
		CHECK(omfsCreateFile((fileHandleType) argv[2], session, kOmfRev2x, 
							 &fileHdl2));
	  }

	/* Iterate over all composition mobs in the file */
	CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	CHECK(omfiGetNumMobs(fileHdl, kCompMob, &numMobs));
	for (loop = 0; loop < numMobs; loop++)
	  {
	    searchCrit.searchTag = kByMobKind;
	    searchCrit.tags.mobKind = kCompMob;
	    CHECK(omfiGetNextMob(mobIter, &searchCrit, &compMob));

	    CHECK(omfiMobCloneExternal(fileHdl, compMob, kFollowDepend, 
				       kIncludeMedia,
				       fileHdl2, &newMob));

	  }

	CHECK(omfiIteratorDispose(fileHdl, mobIter));
	mobIter = NULL;
	CHECK(omfsCloseFile(fileHdl));
	CHECK(omfsCloseFile(fileHdl2));
	CHECK(omfsEndSession(session));
	printf("CloneMobX completed successfully.\n");
      }

    XEXCEPT
      {
		if (mobIter)
		  omfiIteratorDispose(fileHdl, mobIter);
		mobIter = NULL;
		omfsCloseFile(fileHdl);
		omfsCloseFile(fileHdl2);
		omfsEndSession(session);
		/* Success - returned correct error code */
		if (XCODE() == OM_ERR_FILEREV_DIFF)
		  return(0);

		/* Else, error */
		printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
		return(-1);
      }
    XEND;
    return(0);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
