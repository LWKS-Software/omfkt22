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
 * IterSrch - This unittest tests the search criteria on iterators,
 *            by searching for a track with a specific datakind (picture)
 *            in a mob.  If a picture track does not exist, it will
 *            return an error.
 ********************************************************************/
#include "masterhd.h"
#define TRUE 1
#define FALSE 0

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

#define NODEPEND 0
#define YESDEPEND 1
#define NOMEDIA 0
#define YESMEDIA 1

/* Please excuse the lack of error handling code! */
#ifdef MAKE_TEST_HARNESS
int IterSrch(char *filename)
{
    int argc;
    char *argv[2];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t	session;
    omfHdl_t fileHdl;
    omfMobObj_t mob;
    omfInt32 loop, numMobs, numTracks, loop2;
    omfInt32 nameSize = 32;
    omfObject_t track;
    omfIterHdl_t mobIter = NULL, trackIter = NULL;
    omfSearchCrit_t mobCrit, trackCrit;
    omfErr_t omfError;

    XPROTECT(NULL)
      {

#ifdef MAKE_TEST_HARNESS
        argc=2;
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
	    exit(1);
	  }

	CHECK(omfsBeginSession(NULL, &session));
	CHECK(omfsOpenFile((fileHandleType) argv[1], session, &fileHdl));

	/* Iterate over all master mobs in the file */
	CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	CHECK(omfiGetNumMobs(fileHdl, kCompMob, &numMobs));
	for (loop = 0; loop < numMobs; loop++)
	  {
	    mobCrit.searchTag = kByMobKind;
	    mobCrit.tags.mobKind = kCompMob;
	    omfError = omfiGetNextMob(mobIter, &mobCrit, &mob);
	    if (omfError == OM_ERR_NO_MORE_OBJECTS)
	      break;
	    else if (omfError != OM_ERR_NONE)
	      {
		RAISE(omfError);
	      }

	    CHECK(omfiIteratorAlloc(fileHdl, &trackIter));
	    trackCrit.searchTag = kByDatakind;
	    strncpy(trackCrit.tags.datakind, PICTUREKIND, OMUNIQUENAME_SIZE);
	    CHECK(omfiMobGetNumSlots(fileHdl, mob, &numTracks));
	    for (loop2 = 0; loop2 < numTracks; loop2++)
	      {
		omfError = omfiMobGetNextSlot(trackIter, mob, &trackCrit, 
					       &track);
		if (omfError == OM_ERR_NO_MORE_OBJECTS)
		  break;
		printf("Found a mobslot of type picturekind\n");

	      }
	    CHECK(omfiIteratorDispose(fileHdl, trackIter));
	    trackIter = NULL;
	  }

	CHECK(omfiIteratorDispose(fileHdl, mobIter));
	mobIter = NULL;

	CHECK(omfsCloseFile(fileHdl));
	omfsEndSession(session);
	printf("IterSrch completed successfully.\n");

      } /* XPROTECT */

    XEXCEPT
      {
	if (mobIter)
	  omfiIteratorDispose(fileHdl, mobIter);
	if (trackIter)
	  omfiIteratorDispose(fileHdl, trackIter);
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
