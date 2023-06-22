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

#include "OMApi.h"
#include "UnitTest.h"

#if PORT_SYS_MAC
#include "MacSupport.h"
#endif

#define check(a)	if(!a) { FatalErrorCode(a,__LINE__, __FILE__); }
#define FatalErrorCode(ec,line,file) { \
       printf("Error '%s' returned at line %d in %s\n", \
              omfGetErrorString(ec), line, file); \
       RAISE(OM_ERR_TEST_FAILED); }

#define NAMESIZE 32

#ifdef MAKE_TEST_HARNESS
int Attr1x(char *filename)
{
	int		argc;
	char	*argv[2];
#else
int main(int argc, char *argv[])
{
#endif
  OMObject head = NULL, anATTR = NULL, anATTB = NULL;
  char nameBuf[NAMESIZE];
  omfInt32 intValue;
  char stringValue[NAMESIZE];
  omfObject_t objValue;
  omfInt32 attrLength, loop;
  AttrKind attrKind;
  char objID[5];
  omfInt32 length;
  TrackType trackKind;
  omfRational_t editRate;

#if PORT_SYS_MAC
    MacInit();
#endif
#ifdef MAKE_TEST_HARNESS
    argc = 2;
    argv[1] = filename;
#else
#if PORT_MAC_HAS_CONSOLE
    argc = ccommand(&argv); 
#endif
#endif

    if (argc < 2)
      {
		printf("*** ERROR - Attr1x <filename>\n");
		return(1);
      }

    XPROTECT(NULL) { 
  
	  /***************************/
	  /* Create a file with attributes */
	  /***************************/
	  omfiBeginSession();
	  head = omfiCreateFile((fileHandleType)argv[1]);

	  /* Create an attribute list */
	  anATTR = OMNewObject(head, "ATTR");

	  /* Create an integer attribute */
	  anATTB = OMNewObject(head, "ATTB");
	  attrKind = AK_INT;
	  check(OMPutATTBKind(anATTB, &attrKind));
	  check(OMPutATTBName(anATTB, "An Int attribute"));
	  intValue = 32;
	  check(OMPutATTBIntAttribute(anATTB, &intValue));
	  check(OMPutNthATTRAttrRef(anATTR, &anATTB, 1));

	  /* Create a string attribute */
	  anATTB = OMNewObject(head, "ATTB");
	  attrKind = AK_STRING;
	  check(OMPutATTBKind(anATTB, &attrKind));
	  check(OMPutATTBName(anATTB, "A String attribute"));
	  strcpy( (char*) stringValue, "A String");
	  check(OMPutATTBStringAttribute(anATTB, stringValue));
	  check(OMPutNthATTRAttrRef(anATTR, &anATTB, 2));

	  /* Create an object attribute */
	  anATTB = OMNewObject(head, "ATTB");
	  attrKind = AK_OBJECT;
	  check(OMPutATTBKind(anATTB, &attrKind));
	  check(OMPutATTBName(anATTB, "An Object attribute"));

	  objValue = OMNewObject(head, "FILL");
	  trackKind = TT_PICTURE;
	  check(OMPutCPNTTrackKind(objValue, &trackKind));
	  length = 0;
	  check(OMPutCLIPLength(objValue, &length)); 
	  editRate.numerator = 2997;
	  editRate.denominator = 100;
	  check(OMPutCPNTEditRate(objValue, &editRate));
	  check(OMPutATTBObjAttribute(anATTB, &objValue));
	  check(OMPutNthATTRAttrRef(anATTR, &anATTB, 3));
    
	  /* Put the attribute array on the HEAD object */
	  check(OMPutAttributes(head, &anATTR));
	  
	  check(omfiCloseFile(head));
	  check(omfiEndSession());

    /***************************/
    /* Now try to read it back */
    /***************************/
	  anATTR = NULL;
	  anATTB = NULL;
	  check(omfiBeginSession());
	  head = omfiOpenFile((fileHandleType)argv[1]);

	  check(OMGetAttributes(head, &anATTR));
	  attrLength = OMLengthATTRAttrRef(anATTR);
	  for (loop = 1; loop <= attrLength; loop++)
		{
		  check(OMGetNthATTRAttrRef(anATTR, &anATTB, loop));
		  check(OMGetATTBName(anATTB, nameBuf, NAMESIZE)); 
		  printf("Attribute name is: %s\n", nameBuf); 
		  check(OMGetATTBKind(anATTB, &attrKind));
		  switch (attrKind)
			{
			case AK_NULL:
			  printf("Attribute kind is AK_NULL\n");
			  break;
			case AK_INT:
			  check(OMGetATTBIntAttribute(anATTB, &intValue));
			  printf("Attribute kind is AK_INT: %d\n", intValue);
			  if (intValue != 32)
				{
				  printf("ERROR -- Int Value is incorrect\n");
				  return(-1);
				}
			  break;
			case AK_STRING:
			  check(OMGetATTBStringAttribute(anATTB, stringValue, NAMESIZE));
			  printf("Attribute kind is AK_STRING: %s\n", stringValue);
			  if (strcmp(stringValue, "A String"))
				{
				  printf("ERROR -- String Value is incorrect\n");
				  return(-1);
				}
			  break;
			case AK_OBJECT:
			  check(OMGetATTBObjAttribute(anATTB, &objValue));
			  check(OMGetObjID(objValue, (ObjectTag)objID));
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

	  check(omfiCloseFile(head));
	  check(omfiEndSession());
    }
    XEXCEPT
    XEND

    return(0);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
