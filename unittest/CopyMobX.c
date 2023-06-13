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
 * CopyMobX - This unittest opens an existing OMF file (of either type
 *          1.x or 2.x), finds the mobs, and copies them (including
 *          their references mobs and media) to another file of the
 *          same type.  It will help to test that duplicate mobs are
 *          not copied.
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
#include "MacSupport.h"
#endif

#define NODEPEND 0
#define YESDEPEND 1
#define NOMEDIA 0
#define YESMEDIA 1

/* Please excuse the lack of error handling code! */
#ifdef MAKE_TEST_HARNESS
int CopyMobX(char *srcName, char *destName)
{
    int		argc;
    char	*argv[3];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t	session;
    omfHdl_t fileHdl = NULL, fileHdl2 = NULL;
    omfMobObj_t mob = NULL, newMob = NULL, foundMob = NULL;
    omfObject_t newMedia;
    omfInt32 numMobs, loop;
    omfIterHdl_t mobIter = NULL;
    omfSearchCrit_t searchCrit;
    omfFileRev_t fileRev;
    omfUID_t mobID;
    omfErr_t omfError;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Copy External UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

    XPROTECT(NULL)
      {
#if PORT_SYS_MAC
	MacInit();
#endif

#ifdef MAKE_TEST_HARNESS
	argc = 3;
	argv[1] = srcName;
	argv[2] = destName;
#else
#if PORT_MAC_HAS_CCOMMAND
	argc = ccommand(&argv); 
#endif
#endif

	if (argc < 3)
	  {
	    printf("*** ERROR - missing <copyin> <copyout> file name\n");
	    exit(1);
	  }

	/* Open the given file, and create a file of the same OMF revision */
	CHECK(omfsBeginSession(&ProductInfo, &session));
	CHECK(omfsOpenFile((fileHandleType) argv[1], session, &fileHdl));
	CHECK(omfsFileGetRev(fileHdl, &fileRev));
	CHECK(omfsCreateFile((fileHandleType) argv[2], session,
				  fileRev, &fileHdl2));

	/* Find a composition mob in the file, and copy it and its
	 * references and media.
	 */
	CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	CHECK(omfiGetNumMobs(fileHdl, kAllMob, &numMobs));
	if (numMobs == 0)
	  {
	    printf("***ERROR: No composition mobs in the file\n");
	    exit(-1);
	  }

	for (loop = 0; loop < numMobs; loop++)
	  {
	    searchCrit.searchTag = kByMobKind;
	    searchCrit.tags.mobKind = kAllMob;
	    CHECK(omfiGetNextMob(mobIter, &searchCrit, &mob));
	    CHECK(omfiMobGetMobID(fileHdl, mob, &mobID));

	    /* If the mob wasn't already copied with kFollowDepend, copy 
	     * to destination file.
	     */
	    if (mob && !omfiIsMobInFile(fileHdl2, mobID, &foundMob, &omfError))
	      {
		omfError = omfiMobCopyExternal(fileHdl, mob, 
					       kFollowDepend, kIncludeMedia, 
					       "NewMob", fileHdl2,
					       &newMedia, &newMob);
		if ((omfError != OM_ERR_DUPLICATE_MOBID) &&
		    (omfError != OM_ERR_NONE))
		  {
		    RAISE(omfError);
		  }

	      }
	  }

	omfiIteratorDispose(fileHdl, mobIter);
	mobIter = NULL;

	/* Close both files */
	omfError = omfsCloseFile(fileHdl);
	omfError = omfsCloseFile(fileHdl2);

	omfsEndSession(session);
	printf("CopyMobX completed successfully.\n");
      } /* XPROTECT */

    XEXCEPT
      {
	if (mobIter)
	  omfiIteratorDispose(fileHdl, mobIter);
	if(fileHdl != NULL)
	  omfError = omfsCloseFile(fileHdl);
	if(fileHdl2 != NULL)
	  omfError = omfsCloseFile(fileHdl2);
	
	omfsEndSession(session);
	printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
	return(-1);
      }
    XEND;

    return(0);
}

