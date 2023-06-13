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
 * FndSrc - This unittest opens a file (either 1.x or 2.x), looks for
 *          composition mobs, and uses first omfiMobFindSource, and then
 *          omfmOffsetToTimecode (which also calls omfiMobFindSource)
 *          to find the timecode for a specific position in a specific
 *          track.  This unittest will only work on files with tape mobs,
 *          and that have the track and position searched for.  It 
 *          shouldn't be run on any file.
 ********************************************************************/

#include "masterhd.h"
#define TRUE 1
#define FALSE 0

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
#include "omMobMgt.h"
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

#define NODEPEND 0
#define YESDEPEND 1
#define NOMEDIA 0
#define YESMEDIA 1

#ifdef MAKE_TEST_HARNESS
int FndSrc(char *srcName)
{
    int	argc;
    char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t	session;
    omfHdl_t fileHdl = NULL;
    omfMobObj_t compMob, newMob, masterMob;
    omfInt32 loop, numMobs;
    omfIterHdl_t mobIter = NULL;
    omfSearchCrit_t searchCrit;
    omfUID_t mobID;
    omfTrackID_t mobTrack;
    char		mobName[128];
    omfPosition_t mobOffset, startOffset;
	char		mobOffbuf[32];
    omfMediaCriteria_t mediaCrit;
    omfLength_t minLength;
    omfErr_t omfError;
    omfTimecode_t timecode;
    omfBool		foundMob = FALSE;
	char	tcBuf[32];
	
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
	    exit(1);
	  }

	omfsCvtInt32toPosition(12, startOffset);

	CHECK(omfsBeginSession(NULL, &session));
	CHECK(omfmInit(session));
	CHECK(omfsOpenFile((fileHandleType) argv[1], session, &fileHdl));

	/* Iterate over all composition mobs in the file */
	CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	CHECK(omfiGetNumMobs(fileHdl, kCompMob, &numMobs));
	for (loop = 0; loop < numMobs; loop++)
	  {
	    searchCrit.searchTag = kByMobKind;
	    searchCrit.tags.mobKind = kCompMob;
	    omfError = omfiGetNextMob(mobIter, &searchCrit, &compMob);
	    if (omfError == OM_ERR_NO_MORE_OBJECTS)
	      break;
	    else if (omfError != OM_ERR_NONE)
	      {
		RAISE(omfError);
	      }

	    mediaCrit.type = kOmfAnyRepresentation;
		CHECK(omfiMobGetInfo(fileHdl, compMob, NULL, sizeof(mobName),    /* IN - Size of name buffer */
							mobName, NULL, NULL));/* OUT - Creation Time */
		if(strcmp(mobName, "Huge Offset Mob") != 0)
		{
		    CHECK(omfiMobFindSource(fileHdl, compMob, 1, startOffset, 
					    kTapeMob, &mediaCrit,
					    &mobID, &mobTrack,
					    &mobOffset, &minLength, &newMob));
	 	    CHECK(omfsInt64ToString(mobOffset, 10, sizeof(mobOffbuf), mobOffbuf));  
		    printf("MobID: %ld.%lu.%lu, MobTrack: %lu, mobOffset: %s\n",
			   mobID.prefix, mobID.major, mobID.minor,
			   mobTrack, mobOffbuf);
		    memset(&timecode, 0, sizeof(timecode));
		    
		    CHECK(omfiMobFindSource(fileHdl, compMob, 1, startOffset, 
					    kMasterMob, &mediaCrit,
					    &mobID, &mobTrack,
					    &mobOffset, &minLength, &masterMob));
		    CHECK(omfmOffsetToTimecode(fileHdl, masterMob, mobTrack, mobOffset, 
					&timecode));
		    CHECK(omfsTimecodeToString(timecode, (omfInt32)sizeof(tcBuf), 
					       tcBuf));
		    printf("%s\n",tcBuf);
		    foundMob = TRUE;
	    }
	  }

	CHECK(omfiIteratorDispose(fileHdl, mobIter));
	mobIter = NULL;

	if((numMobs != 0) &&!foundMob)
	{
		printf("Couldn't find correct mob to test\n");
		RAISE(OM_ERR_TEST_FAILED);
	}
	
	CHECK(omfsCloseFile(fileHdl));
	omfsEndSession(session);
	printf("FndSrc completed successfully.\n");

      } /* XPROTECT */

    XEXCEPT
      {
	if (mobIter)
	  omfiIteratorDispose(fileHdl, mobIter);
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
