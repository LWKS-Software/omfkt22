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
 * PrivReg - This unittest test creates private data and properties,
 *           and appends them to a newly created mob object.  Besides
 *           testing the creation of private data, it also tests the
 *           remaining accessor functions that haven't been tested in
 *           other unittests - by creating properties of those types.
 ********************************************************************/

#include "masterhd.h"
#define TRUE 1
#define FALSE 0

/* As in Winnie! */
#define OMClassPOOH	"POOH"
#define BYTESSIZE 128

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
#include "UnitTest.h"

#if PORT_SYS_MAC
#include "MacSupport.h"
#endif

#ifdef MAKE_TEST_HARNESS
int PrivReg(char *filename, char *version)
{
	int	argc;
	char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
    omfSessionHdl_t	session;
    omfHdl_t fileHdl = NULL;
    omfFileRev_t rev;
    omfObject_t poohMob = NULL;
    omfProperty_t OMColorThing, OMDirectionThing, OMHintThing,
                  OMFadeThing, OMObjectThing, OMCharThing, OMCharSetThing,
                  OMBytesThing;
    omfUID_t mobID;
    omfUsageCode_t usageCode = UC_NULL;
    char charValue;
    char bunchOfBytes[BYTESSIZE], outputBytes[BYTESSIZE];
    omfIterHdl_t mobIter = NULL;
    omfInt32 numMobs, loop;
    omfUInt32 bytesRead;
    omfColorSpace_t colorValue;
    omfDirectionCode_t directionValue;
    omfEditHint_t hintValue;
    omfFadeType_t fadeValue;
    omfObjectTag_t objID;
    omfCharSetType_t charSetValue;
	omfTimeStamp_t	create_timestamp;
    omfLength_t length10;
    omfPosition_t zeroPos;
    omfDDefObj_t pictureDef = NULL;
    omfRational_t editRate;
    omfObject_t track, filler;
    omfTrackID_t trackID;
    omfErr_t omfError = OM_ERR_NONE;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "PrivReg UnitTest";
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
	argv[1] = filename;
	argv[2] = version;
#else
#if PORT_MAC_HAS_CCOMMAND
	argc = ccommand(&argv); 
#endif
#endif
	if (argc < 3)
	  {
	    printf("PrivReg Usage: PrivReg <omffile> <1|2|IMA>\n");
	    exit(1);
	  }

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

	CHECK(omfsBeginSession(&ProductInfo, &session));
	CHECK(omfsCreateFile((fileHandleType) argv[1], session, rev, &fileHdl));

	/* Initialize the editrate to NTSC */
	editRate.numerator = 2997;
	editRate.denominator = 100;
	omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
	omfsCvtInt32toPosition(0, zeroPos);
	omfsCvtInt32toLength(10, length10);

	/* Register a new class that is a subclass of CMOB (for 1.x, it
	 * is a MOBJ class with a usageCode = UC_NULL written later).
	 * Then write a new type and a bunch of private properties.
	 */
	if ((rev == kOmfRev1x) || rev == kOmfRevIMA)
	  {
	    CHECK(omfsNewClass(session, kClsRegistered, kOmfTstRevEither, 
			       OMClassPOOH, OMClassMOBJ)); 
	  }
	else /* kOmfRev2x */
	  {
	    CHECK(omfsNewClass(session, kClsRegistered, kOmfTstRevEither, 
			       OMClassPOOH, OMClassCMOB)); 
	  }

	/* Properties with 2.x types */
	if (rev == kOmfRev2x)
	  {
	    CHECK(omfsRegisterDynamicProp(session, kOmfTstRevEither,
					  "Color", OMClassPOOH,
					  OMColorSpace, kPropOptional,
					  &OMColorThing));
	    CHECK(omfsRegisterDynamicProp(session, kOmfTstRevEither,
					  "Direction", OMClassPOOH,
					  OMDirectionCode, kPropOptional,
					  &OMDirectionThing));
	    CHECK(omfsRegisterDynamicProp(session, kOmfTstRevEither,
					  "EditHint", OMClassPOOH,
					  OMEditHintType, kPropOptional,
					  &OMHintThing));
	    CHECK(omfsRegisterDynamicProp(session, kOmfTstRevEither,
					  "Fade", OMClassPOOH,
					  OMFadeType, kPropOptional,
					  &OMFadeThing));
	  }

	/* Properties with 1.x types */
	else if ((rev == kOmfRev1x) || rev == kOmfRevIMA)
	  {
	    CHECK(omfsRegisterDynamicProp(session, kOmfTstRevEither,
					  "ObjectThing", OMClassPOOH,
					  OMObjectTag, kPropOptional,
					  &OMObjectThing));
	  }

	/* Properties with either 1.x or 2.x types */
	CHECK(omfsRegisterDynamicProp(session, kOmfTstRevEither,
								  "AChar", OMClassPOOH,
								  OMInt8, kPropOptional,
								  &OMCharThing));
	CHECK(omfsRegisterDynamicProp(session, kOmfTstRevEither,
								  "charSet", OMClassPOOH,
								  OMCharSetType, kPropOptional,
								  &OMCharSetThing));
	CHECK(omfsRegisterDynamicProp(session, kOmfTstRevEither,
								  "SomeStuff", OMClassPOOH,
								  OMVarLenBytes, kPropOptional,
								  &OMBytesThing));

	/* Create a POOH mob */
	CHECK(omfsObjectNew(fileHdl, OMClassPOOH, &poohMob)); 

	/* Write the other properties (creation time, etc.) needed to have POOH
	 * be a semantically valid Mob.  If 1.x file, make it a composition mob by 
	 * writing UC_NULL.
	 */
	omfsGetDateTime(&create_timestamp);
	create_timestamp.IsGMT = 0;
	if ((rev == kOmfRev1x) || rev == kOmfRevIMA)
	  {
	    usageCode = UC_NULL;
	    CHECK(omfsWriteUsageCodeType(fileHdl, poohMob, OMMOBJUsageCode, 
					 usageCode));
		CHECK(omfsWriteTrackType(fileHdl, poohMob, OMCPNTTrackKind, TT_NULL));
		CHECK(omfsWriteTimeStamp(fileHdl, poohMob, OMMOBJLastModified,
								 create_timestamp));
	  }
	else /* kOmfRev2x */
	  {
		CHECK(omfsWriteTimeStamp(fileHdl, poohMob, OMMOBJLastModified,
								 create_timestamp));
		CHECK(omfsWriteTimeStamp(fileHdl, poohMob, OMMOBJCreationTime,
								 create_timestamp));
	  }

	/* Set the new mob's identity */
	CHECK(omfiMobIDNew(fileHdl, &mobID));
	CHECK(omfiMobSetIdentity(fileHdl, poohMob, mobID)); 

	/* Add a track with a filler clip to the new mob */
	CHECK(omfiFillerNew(fileHdl, pictureDef, length10, &filler));
	trackID = 1;
	CHECK(omfiMobAppendNewTrack(fileHdl, poohMob, editRate, filler,
				    zeroPos, trackID, NULL, &track));

	/* Attach a bunch of private properties to it */	
	/* Write 2.x Properties */
	if (rev == kOmfRev2x)
	  {
	    CHECK(omfsWriteColorSpace(fileHdl, poohMob, OMColorThing, 
				      kColorSpaceCMYK));
	    CHECK(omfsWriteDirectionCode(fileHdl, poohMob, OMDirectionThing, 
					 kDirCodeLeft));
	    CHECK(omfsWriteEditHint(fileHdl, poohMob, OMHintThing, 
				    kProportional));
	    CHECK(omfsWriteFadeType(fileHdl, poohMob, OMFadeThing, 
				    kFadeLinearPower));
	  }

	/* Write 1.x Properties */
	else if ((rev == kOmfRev1x) || rev == kOmfRevIMA)
	{
	  CHECK(omfsWriteObjectTag(fileHdl, poohMob, OMObjectThing, "PIGY"));
	  CHECK(omfsWriteCharSetType(fileHdl, poohMob, OMCharSetThing, 
					   1));
	  memset(bunchOfBytes, 0, sizeof(bunchOfBytes));
	  strcpy(bunchOfBytes, "A Pooh is a wonderful thing...\n");
	  CHECK(omfsWriteVarLenBytes(fileHdl, poohMob, OMBytesThing, 
				     zeroPos, /* offset */
				     sizeof(bunchOfBytes),
				     (void *)bunchOfBytes));
	}

	/* Write the rest of the Properties */
	charValue = 'p';
	CHECK(omfsWriteInt8(fileHdl, poohMob, OMCharThing, charValue));
	CHECK(omfsCloseFile(fileHdl));

	/***************************/
	/* Now try to read it back */
	/***************************/
	CHECK(omfsOpenFile((fileHandleType)argv[1], session, &fileHdl));

	CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
	CHECK(omfiGetNumMobs(fileHdl, kAllMob, &numMobs)); 
	for (loop = 0; loop < numMobs; loop++)
	  {
	    CHECK(omfiGetNextMob(mobIter, NULL, &poohMob));

	    /* Read back the 2.x properties */
	    if (rev == kOmfRev2x)
	      {
			CHECK(omfsReadColorSpace(fileHdl, poohMob, OMColorThing,
									 &colorValue));
			if (colorValue != kColorSpaceCMYK)
			  {
				printf("ERROR -- Wrong color space value\n");
				return(-1);
			  }
			CHECK(omfsReadDirectionCode(fileHdl, poohMob, OMDirectionThing,
										&directionValue));
			if (directionValue != kDirCodeLeft)
			  {
				printf("ERROR -- Wrong direction code value\n");
				return(-1);
			  }
			CHECK(omfsReadEditHint(fileHdl, poohMob, OMHintThing,
								   &hintValue));
			if (hintValue != kProportional)
			  {
				printf("ERROR -- Wrong edit hint value\n");
				return(-1);
			  }
			CHECK(omfsReadFadeType(fileHdl, poohMob, OMFadeThing,
								   &fadeValue));
			if (fadeValue != kFadeLinearPower)
			  {
				printf("ERROR -- Wrong fade type value\n");
				return(-1);
			  }
	      }

	    /* Read back the 1.x Properties */
	    else if ((rev == kOmfRev1x) || rev == kOmfRevIMA)
	      {
			CHECK(omfsReadObjectTag(fileHdl, poohMob, OMObjectThing,
						objID));
			if (strncmp(objID, "PIGY", (size_t)4))
			  {
			    printf("ERROR -- Wrong Obj ID value\n");
			    return(-1);
			  }
		    CHECK(omfsReadCharSetType(fileHdl, poohMob, OMCharSetThing,
					      &charSetValue));
		    if (charSetValue != 1)
		      {
				printf("ERROR -- Wrong char set value\n");
				return(-1);
		      }
		    CHECK(omfsReadVarLenBytes(fileHdl, poohMob, OMBytesThing,
						 zeroPos, BYTESSIZE, 
						 outputBytes, &bytesRead));
		    if (strcmp(outputBytes, bunchOfBytes))
		      {
				printf("ERROR -- Wrong var len value\n");
				return(-1);
		      }
	      }

	    /* Read the rest of the Properties */
	    CHECK(omfsReadInt8(fileHdl, poohMob, OMCharThing, &charValue));
	    if (charValue != 'p')
	      {
			printf("ERROR -- Wrong char value\n");
			return(-1);
	      }
	  }

	if (mobIter)
	  CHECK(omfiIteratorDispose(fileHdl, mobIter));
	mobIter = NULL;

	CHECK(omfsCloseFile(fileHdl));
	omfsEndSession(session);
	printf("PrivReg completed successfully.\n");

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

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
