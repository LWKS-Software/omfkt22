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
 * Attr1x -   This unittest creates a 1.x file with the API
 *            compatibility layer, creates an attribute array
 *            object, and appends 3 new attribute objects to it 
 *            (an integer attribute, a string attribute and an
 *            object reference attribute).  The attribute array
 *            is appended to the HEAD object.
 *
 *            Then it reads back the attributes and verifies their values.
 ********************************************************************/

#include "masterhd.h"
#ifdef unix
#include <sys/types.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "omPublic.h"
#include "omMedia.h"
#include "UnitTest.h"

#if PORT_SYS_MAC
#include "MacSupport.h"
#endif

#define FatalErrorCode(ec,line,file) { \

#define NAMESIZE 32
#define COMMENT_NAME	"comments"
#define COMMENT_VALUE	"a user comment"

#ifdef MAKE_TEST_HARNESS
int TstAttr(char *filename, char *version)
{
	int		argc;
	char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
	omfObject_t			anATTB = NULL, masterMob = NULL, theHeadObj = NULL;
	char				nameBuf[NAMESIZE];
	omfInt32			intValue;
	char				stringValue[NAMESIZE];
	omfObject_t			objValue;
	omfInt32			attrLength, loop;
	omfAttributeKind_t	attrKind;
	char				objID[5];
	omfHdl_t			fileHdl;
	omfProperty_t		attrProp;
	omfIterHdl_t		mobIter = NULL,commentIter = NULL;
    omfInt32			numMobs, numComments;
    omfSearchCrit_t 	searchCrit;
    char				commentName[64], commentValue[128];
    omfFileRev_t		rev;
    omfSessionHdl_t		session;
    omfLength_t			zeroLen;
	omfDDefObj_t		pictureDef = NULL;
	omfRational_t		editRate = { 2997, 100 };
	omfErr_t			omfError;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Test Attribute Mechanism UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

#if PORT_SYS_MAC
    MacInit();
#endif
#ifdef MAKE_TEST_HARNESS
    argc = 3;
    argv[1] = filename;
    argv[2] = version;
#else
#if PORT_MAC_HAS_CONSOLE
    argc = ccommand(&argv); 
#endif
#endif

    if (argc < 3)
	{
		printf("*** ERROR - TstAttr <filename><1|2|IMA>\n");
		return(1);
	}

    if (!strcmp(argv[2], "1"))
		rev = kOmfRev1x;
    else if (!strcmp(argv[2], "2"))
		rev = kOmfRev2x;
    else if (!strcmp(argv[2], "IMA"))
		rev = kOmfRevIMA;
    else	
	{
		printf("*** ERROR - Illegal file rev\n");
		return(1);
	}

    XPROTECT(NULL)
    { 
    	omfsCvtInt32toInt64(0, &zeroLen);
		/***************************/
		/* Create a file with attributes */
		/***************************/
		CHECK(omfsBeginSession(&ProductInfo, &session));
		CHECK(omfsCreateFile((fileHandleType) argv[1], session, 
			      rev, &fileHdl));

  		omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);

		CHECK(omfmMasterMobNew(fileHdl, "Attribute test (no channels)", TRUE, &masterMob));
		CHECK(omfmMobAddNilReference(fileHdl, masterMob, 1, zeroLen, pictureDef, editRate));
		CHECK(omfiMobAppendComment(fileHdl, masterMob, COMMENT_NAME, COMMENT_VALUE));

		CHECK(omfsGetHeadObject(fileHdl, &theHeadObj));
		if(rev == kOmfRev2x)
		{
			CHECK(omfsRegisterDynamicProp(session, kOmfTstRev2x,
					   "PvtTstAttributes", OMClassHEAD, OMObjRef, kPropOptional, &attrProp));
		}
		else
			attrProp = OMAttributes;

		/* Create an integer attribute */
		CHECK(omfsObjectNew(fileHdl, "ATTB", &anATTB));
		CHECK(omfsWriteAttrKind(fileHdl, anATTB, OMATTBKind, AK_INT));
		CHECK(omfsWriteString(fileHdl, anATTB, OMATTBName, "An Int attribute"));
		CHECK(omfsWriteInt32(fileHdl, anATTB, OMATTBIntAttribute, 32));
		CHECK(omfsAppendAttribute(fileHdl, theHeadObj, attrProp, anATTB));

		/* Create a string attribute */
		CHECK(omfsObjectNew(fileHdl, "ATTB", &anATTB));
		attrKind = AK_STRING;
		CHECK(omfsWriteAttrKind(fileHdl, anATTB, OMATTBKind, attrKind));
		CHECK(omfsWriteString(fileHdl, anATTB, OMATTBName, "An Int attribute"));
		strcpy( (char*) stringValue, "A String");
		CHECK(omfsWriteString(fileHdl, anATTB, OMATTBStringAttribute, stringValue));
		CHECK(omfsAppendAttribute(fileHdl, theHeadObj, attrProp, anATTB));

	  	/* Create an object attribute */
		CHECK(omfsObjectNew(fileHdl, "ATTB", &anATTB));
		CHECK(omfsWriteAttrKind(fileHdl, anATTB, OMATTBKind, AK_OBJECT));
		CHECK(omfsWriteString(fileHdl, anATTB, OMATTBName, "An Int attribute"));

		CHECK(omfiFillerNew(fileHdl, pictureDef, zeroLen, &objValue));
		if((rev == kOmfRev1x) || (rev == kOmfRevIMA))
		{
			CHECK(omfsWriteExactEditRate(fileHdl, objValue, OMCPNTEditRate, editRate));
		}
		CHECK(omfsWriteObjRef(fileHdl, anATTB, OMATTBObjAttribute, objValue));
		CHECK(omfsAppendAttribute(fileHdl, theHeadObj, attrProp, anATTB));
    	  
		CHECK(omfsCloseFile(fileHdl));
		CHECK(omfsEndSession(session));

		/***************************/
		/* Now try to read it back */
		/***************************/
		anATTB = NULL;
		fileHdl = NULL;
		masterMob = NULL;
		CHECK(omfsBeginSession(&ProductInfo, &session));
		CHECK(omfsOpenFile((fileHandleType)argv[1], session, &fileHdl));
		CHECK(omfsGetHeadObject(fileHdl, &theHeadObj));

	    CHECK(omfiGetNumMobs(fileHdl, kMasterMob, &numMobs)); 
	    if (numMobs != 1)
		{
			printf("ERROR -- Wrong number of master mobs\n");
			return(-1);
		}

	    CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
		searchCrit.searchTag = kByMobKind;
		searchCrit.tags.mobKind = kMasterMob;
		CHECK(omfiGetNextMob(mobIter, &searchCrit, &masterMob));

		CHECK(omfiMobGetNumComments(fileHdl, masterMob, &numComments));
	    if (numComments != 1)
		{
			printf("ERROR -- Wrong number of comments\n");
			return(-1);
		}
    	CHECK(omfiIteratorAlloc(fileHdl, &commentIter));
    	CHECK(omfiMobGetNextComment(commentIter, masterMob,
    									sizeof(commentName), commentName,
    									sizeof(commentValue), commentValue));
    	if(strcmp(commentName, COMMENT_NAME) != 0)
    	{
			printf("ERROR -- Incorrect comment name\n");
			return(-1);
    	}
    	if(strcmp(commentValue, COMMENT_VALUE) != 0)
    	{
			printf("ERROR -- Incorrect comment value\n");
			return(-1);
    	}
    	CHECK(omfiIteratorDispose(fileHdl, commentIter));
    	CHECK(omfiIteratorDispose(fileHdl, mobIter));

		attrLength = omfsNumAttributes(fileHdl, theHeadObj, attrProp);
		for (loop = 1; loop <= attrLength; loop++)
		{
			CHECK(omfsGetNthAttribute(fileHdl, theHeadObj, attrProp, &anATTB, loop));

			CHECK(omfsReadString(fileHdl, anATTB, OMATTBName, nameBuf, NAMESIZE)); 
			printf("Attribute name is: %s\n", nameBuf); 
			CHECK(omfsReadAttrKind(fileHdl, anATTB, OMATTBKind, &attrKind));
			switch (attrKind)
			{
				case AK_NULL:
				printf("Attribute kind is AK_NULL\n");
				break;
				
				case AK_INT:
				CHECK(omfsReadInt32(fileHdl, anATTB, OMATTBIntAttribute, &intValue));
				printf("Attribute kind is AK_INT: %d\n", intValue);
				if (intValue != 32)
				{
					printf("ERROR -- Int Value is incorrect\n");
					return(-1);
				}
				break;
				case AK_STRING:
				CHECK(omfsReadString(fileHdl, anATTB, OMATTBStringAttribute,
									stringValue, NAMESIZE));
				printf("Attribute kind is AK_STRING: %s\n", stringValue);
				if (strcmp(stringValue, "A String"))
				{
					printf("ERROR -- String Value is incorrect\n");
					return(-1);
				}
				break;
				case AK_OBJECT:
				CHECK(omfsReadObjRef(fileHdl, anATTB, OMATTBObjAttribute, &objValue));
				if((rev == kOmfRev1x) || (rev == kOmfRevIMA))
				{
					CHECK(omfsReadClassID(fileHdl, objValue, OMObjID, (omfClassIDPtr_t)objID));
				}
				else
				{
					CHECK(omfsReadClassID(fileHdl, objValue, OMOOBJObjClass, (omfClassIDPtr_t)objID));
				}
				objID[4] = '\0';
				printf("Attribute kind is AK_OBJECT: %.4s\n", objID);
				if (strncmp(objID, "FILL", (size_t)4))
				{
					printf("ERROR -- ObjRef Value is incorrect\n");
					return(-1);
				}
				break;
			}
		}
		CHECK(omfsCloseFile(fileHdl));
		CHECK(omfsEndSession(session));
    }
    XEXCEPT
    {
		printf("Error '%s' returned\n",
              omfsGetErrorString(XCODE()));
		RERAISE(OM_ERR_TEST_FAILED);
   }
   XEND

    return(0);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
