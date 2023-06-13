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
 * ObjIter - This unittest uses the object and property iterators
 *           to locate all of the objs in the file and dump them
 *           to the screen.  It will also find (floating) objects that 
 *           are not linked to a mob.
 ********************************************************************/

#include "masterhd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
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

#ifdef MAKE_TEST_HARNESS
int gMakeObjIterateSilent = 1;
#else
int gMakeObjIterateSilent = 0;
#endif

#ifdef MAKE_TEST_HARNESS
int ObjIter(char *filename)
{
   int argc;
   char *argv[2];
#else
int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t session;
  omfHdl_t fileHdl;
  omfIterHdl_t objIter = NULL, propIter = NULL;
  omfObject_t obj;
  omfProperty_t prop;
  omfType_t type;
  omfClassID_t objClass, parentClass;
  char tmpClass[5];
  omfUniqueName_t datakindName, currName;
  omfFileRev_t fileRev;
  omfBool found = FALSE;
  omfErr_t omfError = OM_ERR_NONE;
  omfProperty_t idProp;
  
  XPROTECT(NULL)
    {
#ifdef MAKE_TEST_HARNESS
      argc = 2;
      argv[1] = filename;
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
      tmpClass[4] = '\0';

      if (argc < 2)
	{
	  printf("*** ERROR - missing file name\n");
	  exit(1);
	}
      
      CHECK(omfsBeginSession(NULL, &session));
      CHECK(omfmInit(session));
      CHECK(omfsOpenFile((fileHandleType) argv[1], session, &fileHdl));
      CHECK(omfiIteratorAlloc(fileHdl, &objIter));
      CHECK(omfiIteratorAlloc(fileHdl, &propIter));

      CHECK(omfsFileGetRev(fileHdl,  &fileRev));
      if ((fileRev == kOmfRev1x) ||(fileRev == kOmfRevIMA))
	  idProp = OMObjID;
      else /* kOmfRev2x */
	idProp = OMOOBJObjClass;

      /* Loop over all objects in the file */
      CHECK(omfiGetNextObject(objIter, &obj));
      while (obj)
	{
	  omfError = omfsReadClassID(fileHdl, obj, idProp, objClass);
	  if (omfError == OM_ERR_NONE)
	    {
	      strncpy(tmpClass, objClass, 4);
	      if (!gMakeObjIterateSilent)
	        printf("ClassName = %4s, (Label: %1ld)\n", (char *)tmpClass,
		     omfsGetBentoID(fileHdl, obj) & 0x0000ffff);
	    }
	  else if (omfError == OM_ERR_PROP_NOT_PRESENT) /* Not OMF object */
	    {
	      printf("UNKNOWN CLASS\n");
	      omfError = OM_ERR_NONE;
	    }

	  CHECK(omfsClassFindSuperClass(fileHdl, objClass, parentClass, 
					&found));

	  strncpy(tmpClass, parentClass, 4);
	  if (!gMakeObjIterateSilent)
	    printf("    Parent ClassName = %s\n", (char *)tmpClass);

	  CHECK(omfiGetNextProperty(propIter, obj, &prop, &type));
	  while ((omfError == OM_ERR_NONE) && prop)
	    {
	      CHECK(omfiGetPropertyName(fileHdl, prop, OMUNIQUENAME_SIZE,
					currName));
	      if (currName)
	        if (!gMakeObjIterateSilent)
		  printf("*** PropertyName = %s\n", (char *)currName); 	      
	      if (!strncmp(currName, "OMFI:DatakindID", (size_t)15))
		{
		  CHECK(omfsReadUniqueName(fileHdl, obj, OMDDEFDatakindID, 
					   datakindName, OMUNIQUENAME_SIZE));
	          if (!gMakeObjIterateSilent)
		    printf("    DDEFName = %s\n", datakindName);
		}
	      omfError = omfiGetNextProperty(propIter, obj, &prop, &type);
	    } /* Property loop */

	  if (omfError == OM_ERR_NO_MORE_OBJECTS)
	    {
	      omfiIteratorClear(fileHdl, propIter);
	      omfError = OM_ERR_NONE;
	    }
	  CHECK(omfiGetNextObject(objIter, &obj));
	} /* Object loop */
  
      CHECK(omfiIteratorDispose(fileHdl, objIter));
      CHECK(omfiIteratorDispose(fileHdl, propIter));
      objIter = NULL;
      propIter = NULL;

      CHECK(omfsCloseFile(fileHdl));
      CHECK(omfsEndSession(session));
      printf("ObjIter completed successfully.\n");
    }

  XEXCEPT
    {
      if (objIter)
	omfiIteratorDispose(fileHdl, objIter);
      if (propIter)
	omfiIteratorDispose(fileHdl, propIter);
      omfsCloseFile(fileHdl);
      omfsEndSession(session);
      if (XCODE() != OM_ERR_NO_MORE_OBJECTS)
	{
	  printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
	  return(-1);
	}
      else /* OM_ERR_NO_MORE_OBJECT */
	return(0);
    }
  XEND;

  return(0);

}
