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
 * DumpFXD - This unittest opens an existing OMF file (of either type
 *           1.x or 2.x), and tries to find all of the effectdefs
 *           in the file.
 *
 *           NOTE: This should NOT be used as example code.  It uses
 *           internal data structures.  A better example that uses
 *           public APIs will be provided in the future.
 ********************************************************************/

#include "masterhd.h"
#include <stdio.h>
#include <string.h>

#define OMFI_ENABLE_SEMCHECK 1  /* for types defined in omPvt.h and omUtils.h */

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
#include "omTable.h"
#include "omPvt.h"

#include "UnitTest.h"

typedef enum
{
	valueIsPtr, valueIsBlock
} valueType_t;

/* typedef struct tableLink_t *tableLinkPtr_t; */


static void printName(omfHdl_t	fileHdl,
          omfDDefObj_t  def);

#ifdef MAKE_TEST_HARNESS
int DumpFXD(char *filename)
{
    int argc;
    char *argv[2];
#else
int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t	session;
  omfHdl_t	fileHdl;
  omfClassID_t classID;
  omTableIterate_t iter;
  omfBool more;

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
      if (argc < 2)
	{
	  printf("*** ERROR - missing file name\n");
	  return(1);
	}

      /* Open the given file */
      CHECK(omfsBeginSession(NULL, &session));
      CHECK(omfsOpenFile((fileHandleType)argv[1], session, &fileHdl));

      if (fileHdl->effectDefs == NULL)
	{
	  printf("There are no registered Effect Definitions\n");
          CHECK(omfsCloseFile(fileHdl));
          CHECK(omfsEndSession(session));
	  return(0);
	}

     CHECK(omfsTableFirstEntry(fileHdl->effectDefs, &iter, &more));
      while (more)
	{
	  CHECK(omfsReadClassID(fileHdl, (omfObject_t)iter.valuePtr,
				 OMOOBJObjClass, classID));
	  if (streq(classID, "EDEF"))
	    printName(fileHdl, (omfObject_t)iter.valuePtr);
	  CHECK(omfsTableNextEntry(&iter, &more));
	}

      CHECK(omfsCloseFile(fileHdl));
      CHECK(omfsEndSession(session));
      printf("DumpFXD completed successfully.\n");

    } /* XPROTECT */

  XEXCEPT
    {
      printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
      return(-1);
    }
  XEND;
  return(0);
}


static void printName(omfHdl_t	fileHdl,
          omfDDefObj_t  def)
{
  char name[64];
  omfErr_t omfError;

  omfError = omfsReadUniqueName(fileHdl, def, OMEDEFEffectID, name, 64);
  if (omfError == OM_ERR_NONE)
    printf("Datakind Name = %s\n", name);
  else
    printf("***ERROR: %d: %s\n", omfError, omfsGetErrorString(omfError));

}
