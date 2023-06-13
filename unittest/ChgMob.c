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
 * ChgMob - This unittest opens an existing OMF file for modify,
 *          and uses iterators and the omfiMobMatchAndExecute() function 
 *          (via the omfiMobChangeRef() function) to change all of the
 *          source clips in the found composition mob to point to
 *          a mob with UID 	<999999, 999999, 999999>.  The purpose
 *          of this test is to verify the mob traversal functionality.
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
int ChgMob(char *srcName)
{
  int	argc;
  char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t	session;
  omfHdl_t fileHdl;
  omfMobObj_t compMob, masterMob;
  omfInt32 loop, numMobs;
  omfIterHdl_t mobIter = NULL, masterMobIter = NULL;
  omfUID_t changeMobID, matchMobID;
  omfSearchCrit_t criteria;
	omfProductIdentification_t ProductInfo;

	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Change Mob UnitTest";
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
#if PORT_MAC_HAS_CONSOLE
	  argc = ccommand(&argv); 
	  /*
	   *	Standard Toolbox stuff
	   */
#if PORT_QD_SEPGLOBALS
	  InitGraf(&thePort);
#else
	  InitGraf(&qd.thePort);
#endif
	  InitFonts();
	  InitWindows();
	  InitMenus();
	  TEInit();
	  InitDialogs((int )NULL);
	  InitCursor();
	
	  where.h = 20;
	  where.v = 40;
#endif
#endif
	  if (argc < 2)
		{
		  printf("*** ERROR - missing file name\n");
		  exit(1);
		}

	  /* Open the file for modification */
	  CHECK(omfsBeginSession(&ProductInfo, &session));
	  CHECK(omfsModifyFile((fileHandleType) argv[1], session, &fileHdl));

	  /* New MobID */
	  changeMobID.prefix = 999999;
	  changeMobID.major = 999999;
	  changeMobID.minor = 999999;

	  /*
	   * First find a master mob in the file and get its mobID 
	   * If no master mobs exist, return error and exit.
	   */
	  CHECK(omfiIteratorAlloc(fileHdl, &masterMobIter));
	  CHECK(omfiGetNumMobs(fileHdl, kMasterMob, &numMobs));
	  if (numMobs == 0)
		{
		  printf("***Error: No Master Mobs were found in the file\n");
		  exit(-1);
		}

	  /* Retrieve the first master mob that we find, and its MobID */
	  criteria.searchTag = kByMobKind;
	  criteria.tags.mobKind = kMasterMob;
	  CHECK(omfiGetNextMob(masterMobIter, &criteria, &masterMob));
	  CHECK(omfiMobGetMobID(fileHdl, masterMob, &matchMobID));
	  CHECK(omfiMobSetIdentity(fileHdl, masterMob, changeMobID));

	  /*
	   * Iterate over all composition mobs in the file, and change
	   * any source clips that reference the given master mob, so that
	   * they refer to the correct (new) mob ID.
	   */
	  CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	  CHECK(omfiGetNumMobs(fileHdl, kCompMob, &numMobs));
	  for (loop = 0; loop < numMobs; loop++)
		{
		  criteria.searchTag = kByMobKind;
		  criteria.tags.mobKind = kCompMob;
		  CHECK(omfiGetNextMob(mobIter, &criteria, &compMob));
		  
		  CHECK(omfiMobChangeRef(fileHdl, compMob, matchMobID, changeMobID));
		}

	  CHECK(omfiIteratorDispose(fileHdl, masterMobIter));
	  CHECK(omfiIteratorDispose(fileHdl, mobIter));
	  masterMobIter = NULL;
	  mobIter = NULL;
	  
	  CHECK(omfsCloseFile(fileHdl));
	  CHECK(omfsEndSession(session));
	  printf("ChangeMob completed successfully.\n");
	} /* XPROTECT */

  XEXCEPT
	{
	  if (masterMobIter)
		omfiIteratorDispose(fileHdl, masterMobIter);
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

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
