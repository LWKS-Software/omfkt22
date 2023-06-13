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
 * DelMob - This unittest opens an existing OMF file (of either type
 *          1.x or 2.x) for modify, and attempts to delete all of
 *          the mobs in the file.  The resulting file should have no
 *          mobs in it.
 *
 *          NOTE: There are some bugs with the MobDelete function.
 *          If any Bento references to any objects are still in use
 *          (not "released), Bento will not release an object.
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
int DelMob(char *srcName)
{
	int	argc;
	char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t	session;
    omfHdl_t fileHdl;
    omfMobObj_t compMob;
    omfInt32 loop, numMobs;
    omfIterHdl_t mobIter = NULL;
    omfUID_t tmpMobID;
    omfErr_t omfError = OM_ERR_NONE;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Delete UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

    XPROTECT(NULL)
      {
#ifdef MAKE_TEST_HARNESS
	argc = 2;
	argv[1] = srcName;
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

	/* Open file for modification */
	CHECK(omfsBeginSession(&ProductInfo, &session));
	CHECK(omfsModifyFile((fileHandleType) argv[1], session, &fileHdl));

	/* Iterate over all composition mobs in the file */
	CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	CHECK(omfiGetNumMobs(fileHdl, kAllMob, &numMobs));
	for (loop = 0; loop < numMobs; loop++)
	  {
	    CHECK(omfiGetNextMob(mobIter, NULL, &compMob));
	    if (omfiIsAFilmMob(fileHdl, compMob, &omfError))
	      printf("Deleting a Film mob...\n");
	    else if (omfiIsATapeMob(fileHdl, compMob, &omfError))
	      printf("Deleting a Tape mob...\n");
	    else if (omfiIsAFileMob(fileHdl, compMob, &omfError))
	      printf("Deleting a File mob...\n");
	    else if (omfiIsAMasterMob(fileHdl, compMob, &omfError))
	      printf("Deleting a Master mob...\n");
	    else if (omfiIsACompositionMob(fileHdl, compMob, &omfError))
	      printf("Deleting a Composition mob...\n");
	    CHECK(omfiMobGetMobID(fileHdl, compMob, &tmpMobID));
	    CHECK(omfiMobDelete(fileHdl, tmpMobID));
	  }

	CHECK(omfiIteratorDispose(fileHdl, mobIter));
	mobIter = NULL;

	CHECK(omfsCloseFile(fileHdl));
	CHECK(omfsEndSession(session));
	printf("DelMob completed successfully.\n");
      } /* XPROTECT */

    XEXCEPT
      {
	if (mobIter)
	  omfiIteratorDispose(fileHdl, mobIter);
	omfsCloseFile(fileHdl);
	printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
	omfsEndSession(session);
	return(-1);
      }
    XEND;
    return(0);
}

