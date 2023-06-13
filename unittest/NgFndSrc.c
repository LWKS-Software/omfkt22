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
 * NgFndSrc - This negative unittest tests omfiMobMatchAndExecute
 *            and looks for SCLPs in mobs that point to mobs that
 *            don't exist in the file.  It needs to be run on an illegal
 *            file in order to report problems.
 ********************************************************************/

/* INCLUDES */
#include "masterhd.h"
#include <stdlib.h>

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

#include "omPublic.h"
#include "omMobMgt.h"

#include "UnitTest.h"

/* DEFINES */
#define TRUE 1
#define FALSE 0

/* MACROS */
#define equalUIDs(a, b) \
 ((a.prefix == b.prefix) && (a.major == b.major) && (a.minor == b.minor))

/* STATIC PROTOTYPES */
static omfBool IsSCLPFunc(omfHdl_t file,
		  omfObject_t obj,
		  void *data);

static omfErr_t FindSourceRef(omfHdl_t file,
		  omfObject_t obj,
		  omfInt32 level,
                  void *data);

/*************************************************************************/
/* IsSCLPFunc - match function for omfiMobMatchAndExecute() function */
/*************************************************************************/
static omfBool IsSCLPFunc(omfHdl_t file,
			  omfObject_t obj,
                          void *data)
{
  omfErr_t omfError;

  if (omfiIsASourceClip(file, obj, &omfError))
    return(TRUE);
  else 
    return(FALSE);
}

/***************************************************************************/
/* FindSourceRef - callbackFunc for omfiMobMatchAndExecute() function. */
/*               Resolves source reference and checks to see if it is      */
/*               in the file.                                              */
/***************************************************************************/
static omfErr_t FindSourceRef(omfHdl_t file,
			      omfObject_t obj,
			      omfInt32 level,
			      void *data)
{
  omfSourceRef_t sourceRef;
  omfObject_t mob;
  omfUID_t nullID = {0,0,0};
  omfErr_t omfError = OM_ERR_NONE;

  if (omfiIsASourceClip(file, obj, &omfError))
    {
      /* Only return the source reference from the SCLP */
      omfError = omfiSourceClipGetInfo(file, obj, NULL, NULL, &sourceRef);

      /* If it is not a nullID (end of mob chain) and no error... */
      if ((omfError == OM_ERR_NONE) && !equalUIDs(nullID, sourceRef.sourceID))
	{
	  /* Is the mob in the file?  If not, print error */
	  if (!omfiIsMobInFile(file, sourceRef.sourceID, &mob, &omfError))
	    {
	      printf("MOB: MobID %d.%d.%d is NOT in the OMF file\n", 
		     sourceRef.sourceID.prefix, 
		     sourceRef.sourceID.major, 
		     sourceRef.sourceID.minor);
	    }
	}
    }
  return(omfError);
}

#ifdef MAKE_TEST_HARNESS
int NgFndSrc(char *srcName)
{
	int	argc;
	char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t	session;
    omfHdl_t fileHdl;
    omfMobObj_t mob;
    omfInt32 loop, numMobs;
    omfIterHdl_t mobIter = NULL;
    omfInt32 matches;

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
	    printf("Usage: NgFndSrc <filename>.omf\n");
	    exit(1);
	  }

	CHECK(omfsBeginSession(NULL, &session));
	CHECK(omfsOpenFile((fileHandleType) argv[1], session, &fileHdl));

	/* Iterate over all mobs in the file */
	CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	CHECK(omfiGetNumMobs(fileHdl, kAllMob, &numMobs));
	for (loop = 0; loop < numMobs; loop++)
	  {
	    /* No search criteria means find all mobs */
	    CHECK(omfiGetNextMob(mobIter, NULL, &mob));

	    CHECK(omfiMobMatchAndExecute(fileHdl, mob, 0, /* level */
					     IsSCLPFunc, NULL,
					     FindSourceRef, NULL,
					     &matches));
	    printf("Found %d Total Source Clips in this mob\n", matches); 
	  }

	CHECK(omfiIteratorDispose(fileHdl, mobIter));
	mobIter = NULL;
	CHECK(omfsCloseFile(fileHdl));
	CHECK(omfsEndSession(session));
	printf("NgFndSrc completed.\n");
      } /* XPROTECT */

    XEXCEPT
      {
	if (mobIter)
	  omfiIteratorDispose(fileHdl, mobIter);
	printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
	return(-1);
      }
    XEND;

    return(0);
}

