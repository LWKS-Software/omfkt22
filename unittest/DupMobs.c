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
 * DupMobs - This unittest creates an OMF file of the specified revision,
 *           creates duplicate mobs, and then verifies that the duplicate
 *           mob iterator finds them.
 ********************************************************************/

#include "masterhd.h"
#include <string.h>
#include "omPublic.h"
#include "omMobMgt.h"
#include "omMedia.h"
#ifdef MAKE_TEST_HARNESS
#include "UnitTest.h"
#endif

#if PORT_MAC_HAS_CCOMMAND
#include <console.h>
#endif

/* two UIDs are equal if all of the fields are equal */
#define equalUIDs(a, b) \
    ((a.prefix == b.prefix) && (a.major == b.major) && (a.minor == b.minor))

/* Global Variables */
static omfRational_t editRate;
static omfSourceRef_t nullSource;

#ifdef MAKE_TEST_HARNESS
int DupMobs(char *input, char *version)
{
    int argc;
    char *argv[3];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t session;
    omfHdl_t fileHdl;
    omfMobObj_t compMob1 = NULL, compMob2 = NULL, compMob3 = NULL;
    omfMobObj_t compMob4 = NULL, compMob5 = NULL;
    omfMobObj_t masterMob1 = NULL, masterMob2 = NULL;
    omfSegObj_t filler1 = NULL, filler2 = NULL, filler3 = NULL;
    omfSegObj_t filler4 = NULL, filler5 = NULL;
    omfMSlotObj_t slot1 = NULL, slot2 = NULL, slot3 = NULL;
    omfMSlotObj_t slot4 = NULL, slot5 = NULL;
    omfMSlotObj_t mSlot1 = NULL, mSlot2 = NULL;
    omfSegObj_t mFiller1 = NULL, mFiller2 = NULL;
    omfIterHdl_t dupIter = NULL;
    omfUID_t compMobID, masterMobID, mobID;
    omfInt32 numMatches = 0, loop;
    omfMobObj_t *mobList, *mobListStart, mob;
    fileHandleType filename;
    omfFileRev_t   rev;
    omfDDefObj_t pictureDef = NULL;
    omfRational_t editRate;
    omfLength_t clipLen0;
    omfPosition_t zeroPos;
    omfErr_t omfError = OM_ERR_NONE;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Duplicate UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

    XPROTECT(NULL)
      {

#ifdef MAKE_TEST_HARNESS
	argc = 3;
	argv[1] = input;
	argv[2] = version;
#else
#if PORT_SYS_MAC
	MacInit();
#if PORT_MAC_HAS_CCOMMAND
	argc = ccomand(&argv);
#endif
#endif
#endif
	if (argc < 2) 
	  { 
            printf("DupMobs Usage: DupMobs <infile> <outfile> <1|2|IMA>\n");
	    return(1);
 	  }
	filename = (fileHandleType) argv[1];

	if (!strcmp(version,"1"))
	  rev = kOmfRev1x;
	else if (!strcmp(version,"2"))
	  rev = kOmfRev2x;
	else if (!strcmp(version,"IMA"))
	  rev = kOmfRevIMA;
	else 
	  {
	    printf("*** ERROR - Illegal file rev\n");
	    return(1);
	  }

	/* Create a file of the specified revision */
	CHECK(omfsBeginSession(&ProductInfo, &session));
	CHECK(omfsCreateFile(filename, session, rev, &fileHdl));

	/* Initialize the editrate to NTSC */
	editRate.numerator = 2997;
	editRate.denominator = 100;

	/* Create duplicate composition mobs */
	CHECK(omfiCompMobNew(fileHdl, "Simple Composition", TRUE, &compMob1));
	CHECK(omfiMobGetMobID(fileHdl, compMob1, &compMobID));
	printf("Creating duplicate composition mobs... %ld.%lu.%lu\n", 
	       compMobID);

	omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
	editRate.numerator = 2997;
	editRate.denominator = 100;
	omfsCvtInt32toInt64(10, &clipLen0);
	omfsCvtInt32toPosition(0, zeroPos);

	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen0, &filler1));
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen0, &filler2));
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen0, &filler3));
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen0, &filler4));
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen0, &filler5));

	CHECK(omfiCompMobNew(fileHdl, "Simple Composition", TRUE, &compMob2));
	CHECK(omfiCompMobNew(fileHdl, "Simple Composition", TRUE, &compMob3));
	CHECK(omfiCompMobNew(fileHdl, "Simple Composition", TRUE, &compMob4));
	CHECK(omfiCompMobNew(fileHdl, "Simple Composition", TRUE, &compMob5));

	CHECK(omfiMobAppendNewTrack(fileHdl, compMob1, editRate, filler1,
				    zeroPos, 1, "Dup Mob 1", &slot1));
	CHECK(omfiMobAppendNewTrack(fileHdl, compMob2, editRate, filler2,
				    zeroPos, 1, "Dup Mob 2", &slot2));
	CHECK(omfiMobAppendNewTrack(fileHdl, compMob3, editRate, filler3,
				    zeroPos, 1, "Dup Mob 3", &slot3));
	CHECK(omfiMobAppendNewTrack(fileHdl, compMob4, editRate, filler4,
				    zeroPos, 1, "Dup Mob 4", &slot4));
	CHECK(omfiMobAppendNewTrack(fileHdl, compMob5, editRate, filler5,
				    zeroPos, 1, "Dup Mob 5", &slot5));

	CHECK(omfsWriteUID(fileHdl, compMob2, OMMOBJMobID, compMobID));
	CHECK(omfsWriteUID(fileHdl, compMob3, OMMOBJMobID, compMobID));
	CHECK(omfsWriteUID(fileHdl, compMob4, OMMOBJMobID, compMobID));
	CHECK(omfsWriteUID(fileHdl, compMob5, OMMOBJMobID, compMobID));

	/* Create duplicate master mobs */
	CHECK(omfmMasterMobNew(fileHdl, "Master Mob", FALSE, &masterMob1));
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen0, &mFiller1));
	CHECK(omfiMobAppendNewTrack(fileHdl, masterMob1, editRate, mFiller1,
				    zeroPos, 1, NULL, &mSlot1));
	CHECK(omfiMobGetMobID(fileHdl, masterMob1, &masterMobID));
	printf("Creating duplicate master mobs... %ld.%lu.%lu\n", 
	       masterMobID);
	CHECK(omfmMasterMobNew(fileHdl, "Master Mob", FALSE, &masterMob2));
	CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen0, &mFiller2));
	CHECK(omfiMobAppendNewTrack(fileHdl, masterMob2, editRate, mFiller2,
				    zeroPos, 1, NULL, &mSlot2));
	CHECK(omfsWriteUID(fileHdl, masterMob2, OMMOBJMobID, masterMobID));

	/* Close the file */
	CHECK(omfsCloseFile(fileHdl));

	/* Open the file again to reset the cache */
	CHECK(omfsOpenFile(filename, session, &fileHdl));

	printf("\nLooking for duplicate mobs...\n");
	CHECK(omfiIteratorAlloc(fileHdl, &dupIter));
	omfError = omfiFileGetNextDupMobs(dupIter, &mobID, 
					  &numMatches, &mobList);
	while (omfError == OM_ERR_NONE)
	  {
	    if (numMatches > 0)
	      {
		mobListStart = mobList;
		printf("There are %d duplicate mobs with MobID: %ld.%lu.%lu\n",
		       numMatches, mobID);
		for (loop = 1; loop <= numMatches; loop++)
		  {
		    mob = *mobList;
		    CHECK(omfiMobGetMobID(fileHdl, mob, &mobID));
		    if (omfiIsACompositionMob(fileHdl, mob, &omfError) && 
			!equalUIDs(mobID, compMobID))
		      {
			RAISE(OM_ERR_MISSING_MOBID);
		      }
		    else if (omfiIsAMasterMob(fileHdl, mob, &omfError) &&
			     !equalUIDs(mobID, masterMobID))
		      {
			RAISE(OM_ERR_MISSING_MOBID);
		      }
		    printf("%d MobID: %ld.%lu.%lu\n", 
				loop,mobID.prefix, mobID.major, mobID.minor);
		    mobList++;
		  }
		omfsFree(mobListStart);
	      }
	    printf("\n");
	    omfError = omfiFileGetNextDupMobs(dupIter, &mobID, 
					      &numMatches, &mobList);
	  }

	if (omfError != OM_ERR_NO_MORE_OBJECTS)
	  {
	    RAISE(omfError);
	  }

	CHECK(omfiIteratorDispose(fileHdl, dupIter));
	dupIter = NULL;

	/* NOTE: Don't close the file, since it will generate an error (?) */
	CHECK(omfsEndSession(session));

        printf("DupMobs completed successfully.\n");
      }

    XEXCEPT
      {
	if (dupIter)
	  {
	    omfiIteratorDispose(fileHdl, dupIter);
	  }
	if (XCODE() == OM_ERR_MISSING_MOBID)
	  printf("***ERROR: Expected duplicate mob, test failed\n");
	printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
	return(-1);
      }
    XEND;

    return(0);
}


