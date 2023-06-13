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
 * CopyMob - This unittest opens an existing OMF file (of either type
 *          1.x or 2.x) for modify, finds the composition mobs, and
 *          copies them within the same file.
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


/* Please excuse the lack of error handling code! */
#ifdef MAKE_TEST_HARNESS
int CopyMob(char *filename)
{
	int		argc;
	char	*argv[3];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t	session;
    omfHdl_t fileHdl;
    omfMobObj_t compMob, newMob;
    omfInt32 loop, numMobs;
    omfIterHdl_t mobIter = NULL;
    omfSearchCrit_t searchCrit;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Copy UnitTest";
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
	argc = 2;
	argv[1] = filename;
#else
#if PORT_MAC_HAS_CCOMMAND
	argc = ccommand(&argv); 
#endif
#endif

	if (argc < 2)
	  {
	    printf("*** ERROR - missing file name\n");
	    exit(1);
	  }

	/* Open file for modification */
	CHECK(omfsBeginSession(&ProductInfo, &session));
	CHECK(omfsModifyFile((fileHandleType) argv[1], session, &fileHdl));

	/* Iterate over all composition mobs in the file */
	CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	CHECK(omfiGetNumMobs(fileHdl, kCompMob, &numMobs));
	for (loop = 0; loop < numMobs; loop++)
	  {
	    searchCrit.searchTag = kByMobKind;
	    searchCrit.tags.mobKind = kCompMob;
	    CHECK(omfiGetNextMob(mobIter, &searchCrit, &compMob));

	    CHECK(omfiMobCopy(fileHdl, compMob, "NewMob", &newMob));
	  }

	CHECK(omfiIteratorDispose(fileHdl, mobIter));
	mobIter = NULL;

	CHECK(omfsCloseFile(fileHdl));
	CHECK(omfsEndSession(session));
	printf("CopyMob completed successfully.\n");
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

