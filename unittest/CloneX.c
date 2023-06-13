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
 * CloneX - This unittest opens an existing OMF file and clones
 *          (maintains mob IDs) the composition mob, all mobs that 
 *          it references and all associated media to an external file.  
 *          It will copy a 1.x file to another 1.x file, or a 2.x file 
 *          to another 2.x file.  It will not clone between file revs.
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

/********* MAIN PROGRAM *********/
#ifdef MAKE_TEST_HARNESS
int CloneMobX(char *srcName, char *destName)
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
	ProductInfo.productName = "Clone External UnitTest";
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
#ifdef PORT_QD_SEPGLOBALS
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

	  /* Open given file, and create file with same OMF revision */
	  CHECK(omfsBeginSession(&ProductInfo, &session));
	  CHECK(omfsOpenFile((fileHandleType) argv[1], session, &fileHdl));
	  CHECK(omfsFileGetRev(fileHdl, &fileRev));
	  CHECK(omfsCreateFile((fileHandleType) argv[2], session,
						   fileRev, &fileHdl2));

	  /* Iterate over all composition mobs in the file */
	  CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	  CHECK(omfiGetNumMobs(fileHdl, kCompMob, &numMobs));
	  for (loop = 0; loop < numMobs; loop++)
		{
		  searchCrit.searchTag = kByMobKind;
		  searchCrit.tags.mobKind = kCompMob;
		  CHECK(omfiGetNextMob(mobIter, &searchCrit, &compMob));

		  /* Clone the mob and all referenced mobs and media */
		  CHECK(omfiMobCloneExternal(fileHdl, compMob, kFollowDepend, 
									 kIncludeMedia,
									 fileHdl2, &newMob));

		}

	  CHECK(omfiIteratorDispose(fileHdl, mobIter));
	  mobIter = NULL;

	  /* Close both files */
	  CHECK(omfsCloseFile(fileHdl));
	  CHECK(omfsCloseFile(fileHdl2));
	  CHECK(omfsEndSession(session));
	  printf("CloneMobX completed successfully.\n");
	}

  XEXCEPT
	{
	  if (mobIter)
		omfiIteratorDispose(fileHdl, mobIter);
	  omfsCloseFile(fileHdl);
	  omfsCloseFile(fileHdl2);
	  printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
	  omfsEndSession(session);
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
