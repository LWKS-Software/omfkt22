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
 * TstCodec.c - This unittest tests the codec selection functions.
 ********************************************************************/

#include "masterhd.h"
#include <stdio.h>
#include <string.h>

#include "omPublic.h"
#include "omMedia.h"
#include "UnitTest.h"
#include "omcAVR.h"

static omfErr_t	TestOneCombination(omfSessionHdl_t session, omfFileRev_t rev, char *datakind);

#ifdef MAKE_TEST_HARNESS
int TestCodecs(void)
{
#else
int main(int argc, char *argv[])
{
#endif
	omfSessionHdl_t	session;
	
	XPROTECT(NULL)
    {
		/* Start an OMF Session */
		CHECK(omfsBeginSession(NULL, &session));
		omfmInit(session);
		CHECK(omfmRegisterCodec(session, omfCodecAJPG, kOMFRegisterLinked));

		CHECK(TestOneCombination(session, kOmfRev1x, PICTUREKIND));
		CHECK(TestOneCombination(session, kOmfRev1x, SOUNDKIND));
		CHECK(TestOneCombination(session, kOmfRevIMA, PICTUREKIND));
		CHECK(TestOneCombination(session, kOmfRevIMA, SOUNDKIND));
		CHECK(TestOneCombination(session, kOmfRev2x, PICTUREKIND));
		CHECK(TestOneCombination(session, kOmfRev2x, SOUNDKIND));

		/* Close the OMF Session */
		omfsEndSession(session);
	}
	XEXCEPT
    {
      printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
      return(-1);
    }
	XEND;
	return(0);
}

static omfErr_t	TestOneCombination(omfSessionHdl_t session, omfFileRev_t rev, char *datakind)
{
	omfInt32		numCodecs, codec, numVarieties, variety, lineLen, nameLen;
	char			codecID[64], codecName[256], varietyName[256], *revString, *kindName;

	XPROTECT(NULL)
    {
		if(rev == kOmfRev1x)
			revString = "1.x";
		else if(rev == kOmfRev2x)
			revString = "1.x";
		else
			revString = "IMA";
		kindName = strrchr(datakind, ':');
		if(kindName != NULL)
			kindName++;				/* Skip over the colon */
		else
			kindName = datakind;
			
		printf("Searching for %s-capable, rev %s codecs\n", kindName, revString);
		CHECK(omfmGetNumCodecs(session, datakind, rev, &numCodecs));
		for(codec = 1; codec <= numCodecs; codec++)
		{
			CHECK(omfmGetIndexedCodecInfo(session, datakind, rev, codec,
					sizeof(codecID), codecID, sizeof(codecName), codecName));
			printf("    found %s (codec: ID = %s)\n",
						codecName, codecID);
			CHECK(omfmGetNumCodecVarieties(session,codecID, rev, datakind, &numVarieties));
			if(numVarieties != 0)
			{
				printf("    found %ld varieties:\n", numVarieties);
				for(variety = 1, lineLen = 0; variety <= numVarieties; variety++)
				{
					CHECK(omfmGetIndexedCodecVariety(session,codecID, rev, datakind,
										variety, sizeof(varietyName), varietyName));
					nameLen = strlen(varietyName);
					if(variety == 1)
					{
						printf("        %s", varietyName);
						lineLen = nameLen + 8;	/* +8 For spaces */
					}
					else if(nameLen + lineLen + 2 <= 70)
					{
						printf(", %s", varietyName);
						lineLen += nameLen + 2;	/* +2 For comma and space */
					}
					else
					{
						printf("\n        %s", varietyName);
						lineLen = nameLen + 8;	/* +8 For spaces */
					}
				}
				printf("\n");
			}
		}
	}
	XEXCEPT
    {
    }
	XEND;

	return(OM_ERR_NONE);
}
/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/

