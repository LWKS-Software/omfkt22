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

/******************************************************************/
/* WriteSimple.c - Uses the OMF Simple Composition Interface to   */
/*                 write a simple (flat) composition with a       */
/*                 single video track and a timecode track.       */
/*                 The video track contains the Roger and Paul    */
/*                 clips from the OMF Movie example in the OMF    */
/*                 2.0 Training class.                            */
/*                 To keep the example simple, the file mobs, etc.*/
/*                 are not included in the file.                  */
/******************************************************************/
/* INCLUDES */
#include "masterhd.h"
#include <stdlib.h>
#include <string.h>

#include "omPublic.h"
#include "omMedia.h"
#include "omCompos.h"

#include "UnitTest.h"

/* MACROS */
#define initUID(id, pref, maj, min) \
{ (id).prefix = pref; (id).major = maj; (id).minor = min; }
#define copyUIDs(a, b) \
 {a.prefix = b.prefix; a.major = b.major; a.minor = b.minor;}

/* STATIC FUNCTIONS */
static omfErr_t CreateMasterMob(omfHdl_t fileHdl,
				omfMobObj_t *masterMob);

/* Global Variables */
static omfRational_t editRate;
static omfSourceRef_t nullSource;

/* MAIN PROGRAM */
#ifdef MAKE_TEST_HARNESS
int MkSimple(char *filename, int version)
{
  int argc;
  char *argv[3];
#else
int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t	session;
  omfHdl_t	fileHdl;
  omfMobObj_t compMob = NULL, videoMasterMob = NULL, imageMasterMob = NULL;
  omfTimecode_t timecode;
  omfUID_t nullID = {0,0,0};
  omfTrackHdl_t pictureTrackHdl = NULL, tcTrackHdl = NULL;
  omfSourceRef_t videoSource, imageSource;
  omfFileRev_t rev;
  omfLength_t trackLen, clipLen;
  omfErr_t omfError = OM_ERR_NONE;
  char revstr[16];
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Simple Composition Interface UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
  	ProductInfo.platform = NULL;

  XPROTECT(NULL)
  {
#ifdef MAKE_TEST_HARNESS
    argc = 3;
    argv[1] = filename;
    sprintf(revstr,"%d",version);
    argv[2] = revstr;
#endif
    /* This function will write either a 1.x or 2.x file */
    if (argc != 3)
      {
	printf("Usage: MkSimple <filename>.omf <1|2>\n");
	return(-1);
      }
    if (!strcmp(argv[2], "1"))
      rev = kOmfRev1x;
    else if (!strcmp(argv[2], "2"))
      rev = kOmfRev2x;
    else
      {
	printf("*** ERROR - Illegal file revision\n");
      }

    CHECK(omfsBeginSession(&ProductInfo, &session));
    CHECK(omfsCreateFile((fileHandleType) argv[1], session, rev, &fileHdl));

    /* Initialize the editrate to NTSC */
    editRate.numerator = 2997;
    editRate.denominator = 100;

    /* Initialize a null source reference */
    nullSource.sourceTrackID = 0;
    omfsCvtInt32toPosition(0, nullSource.startTime);
    initUID(nullSource.sourceID, 0, 0, 0);

    /* Create a master mob that the composition mob source clips will
     * point to for their source media.
     */
    CHECK(CreateMasterMob(fileHdl, &videoMasterMob));
    CHECK(CreateMasterMob(fileHdl, &imageMasterMob));

    /* Get the newly created master mobs' mobIDs to reference in the 
     * composition.
     */
    CHECK(omfiMobGetMobID(fileHdl, videoMasterMob, &videoSource.sourceID));
    videoSource.sourceTrackID = 1;

    CHECK(omfiMobGetMobID(fileHdl, imageMasterMob, &imageSource.sourceID));
    imageSource.sourceTrackID = 1;

    /* Create a new Composition Mob */
    CHECK(omfiCompMobNew(fileHdl, 
			 "Simple Composition", /* Mob Name */
			 TRUE,                 /* Primary mob */
			 &compMob));

    /**********************************************************/
    /* Create picture new track */
    /* Length is not specified until components are added */
    CHECK(omfcSimpleTrackNew(fileHdl, compMob, 
			     1,           /* TrackID */
			     "Track 1",   /* TrackName */
			     editRate,    /* Track editrate */
			     PICTUREKIND, /* Datakind: omfi:data:Picture */
			     &pictureTrackHdl));

    /*** Start appending components to the new track ***/
    /* All segments will be of the datakind specified during track creation */

    /* Append source clips that point to the created master mob */
    omfsCvtInt32toPosition(100, videoSource.startTime);
    omfsCvtInt32toLength(500, clipLen);
    CHECK(omfcSimpleAppendSourceClip(pictureTrackHdl, clipLen, videoSource));
    omfsCvtInt32toPosition(700, videoSource.startTime);
    omfsCvtInt32toLength(300, clipLen);
    CHECK(omfcSimpleAppendSourceClip(pictureTrackHdl, clipLen, videoSource));

    /* Use "transition defaults" for the arguments to the effect */
    omfsCvtInt32toLength(15, clipLen);
    CHECK(omfcSimpleAppendVideoDissolve(pictureTrackHdl, 
					clipLen));    /* length */

    omfsCvtInt32toPosition(0, imageSource.startTime);
    omfsCvtInt32toLength(15, clipLen);
    CHECK(omfcSimpleAppendSourceClip(pictureTrackHdl, clipLen, imageSource));

    /**********************************************************/
    /* Create a new timecode track */
    /* Get picture track length for timecode track */	       
    CHECK(omfcSimpleTrackGetLength(pictureTrackHdl, &trackLen));

    CHECK(omfsStringToTimecode("01:00:00:00", editRate, &timecode));
    CHECK(omfcSimpleTimecodeTrackNew(fileHdl, compMob, 
					2,                /* TrackID */
					"Timecode Track", /* TrackName */
					editRate,         /* Track Editrate */
					trackLen,         /* Track Length */
					timecode,         /* Timecode desc */
					&tcTrackHdl));

    /* Close the 2 tracks */
    CHECK(omfcSimpleTrackClose(pictureTrackHdl));
    CHECK(omfcSimpleTrackClose(tcTrackHdl));

    CHECK(omfsCloseFile(fileHdl));
    CHECK(omfsEndSession(session));

    printf("MkSimple completed successfully.\n");
  } /* XPROTECT */

  XEXCEPT
    {
      if (pictureTrackHdl)
	omfcSimpleTrackClose(pictureTrackHdl);
      if (tcTrackHdl)
	omfcSimpleTrackClose(tcTrackHdl);
      omfsCloseFile(fileHdl);
      omfsEndSession(session);
      printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
      return(-1);
    }
  XEND;
  return(0);
}


/* CreateMasterMob - creates a master mob with one video track.  The
 *                   video track contains a single source clip with
 *                   a null reference.  In reality, this would probably
 *                   point to a file mob with digital data associated with
 *                   it.  For this example, we are simply providing a null
 *                   mastermob to give the composition mob something to
 *                   reference for media.
 */
static omfErr_t CreateMasterMob(omfHdl_t fileHdl,
				omfMobObj_t *masterMob)
{
  omfTrackHdl_t masterTrackHdl = NULL;
  omfLength_t clipLen;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(fileHdl)
    {
      CHECK(omfmMasterMobNew(fileHdl, 
			     "Null Master Mob", /* Mob Name */
			     FALSE,             /* Not a primary mob */
			     masterMob));

      /* Create picture new track */
      /* Length is not specified until components are added */
      CHECK(omfcSimpleTrackNew(fileHdl, *masterMob, 
			       1,           /* TrackID */
			       "Track 1",   /* TrackName */
			       editRate,    /* Track editrate */
			       PICTUREKIND, /* Datakind: omfi:data:Picture */
			       &masterTrackHdl));
      omfsCvtInt32toLength(2000, clipLen);
      CHECK(omfcSimpleAppendSourceClip(masterTrackHdl, clipLen, nullSource));

      CHECK(omfcSimpleTrackClose(masterTrackHdl));
    }

  XEXCEPT
    {
      if (masterTrackHdl)
	omfcSimpleTrackClose(masterTrackHdl);
      return(XCODE());
    }
  XEND;

  return(OM_ERR_NONE);
}
