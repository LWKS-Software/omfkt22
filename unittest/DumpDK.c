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
 * DumpDK - This unittest opens an existing OMF file (of either type
 *          1.x or 2.x), and tries to find all of the known datakinds
 *          for that revision.  It also looks for a bogus datakind to
 *          make sure that the correct error is returned.
 ********************************************************************/

#include "masterhd.h"
#include <stdio.h>
#include <string.h>

#include "omPublic.h"

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

static void printName(omfHdl_t	fileHdl,
          omfDDefObj_t  def);

/* Please excuse the lack of error handling code! */
#ifdef MAKE_TEST_HARNESS
int DumpDK(char *filename)
{
  int argc;
  char *argv[2];
#else
int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t	session;
  omfHdl_t	fileHdl;
  omfDDefObj_t def;
  omfFileRev_t fileRev;
  omfErr_t omfError;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "DumpDK UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

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
      CHECK(omfsBeginSession(&ProductInfo, &session));
      CHECK(omfsOpenFile((fileHandleType) argv[1], session, &fileHdl));

      /* Look up the bogus datakind */
      if (omfiDatakindLookup(fileHdl, "omfi:data:lisa", &def, &omfError))
	{
	  printName(fileHdl, def);
	  RAISE(OM_ERR_DATAKIND_EXIST);
	}
      else
	printf("***ERROR: Can't find %s datakind\n", "omfi:data:lisa");

      /* Look up the remaining datakinds that should exist in the file */
      if (omfiDatakindLookup(fileHdl, PICTUREKIND, &def, &omfError))
	printName(fileHdl, def);
      else
	{
	  RAISE(OM_ERR_INVALID_DATAKIND);
	}

      if (omfiDatakindLookup(fileHdl, SOUNDKIND, &def, &omfError))
	printName(fileHdl, def);
      else
	{
	  RAISE(OM_ERR_INVALID_DATAKIND);
	}

      if (omfiDatakindLookup(fileHdl, TIMECODEKIND, &def, &omfError))
	printName(fileHdl, def);
      else
	{
	  RAISE(OM_ERR_INVALID_DATAKIND);
	}
      
      if (omfiDatakindLookup(fileHdl, EDGECODEKIND, &def, &omfError))
	printName(fileHdl, def);
      else
	{
	  RAISE(OM_ERR_INVALID_DATAKIND);
	}

      /* The following datakinds should only appear in 2.x files */
      CHECK(omfsFileGetRev(fileHdl, &fileRev));
      if (fileRev == kOmfRev2x)
	{
	  if (omfiDatakindLookup(fileHdl, NODATAKIND, &def, &omfError))
	      printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }
     
	  if (omfiDatakindLookup(fileHdl, CHARKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, UINT8KIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, INT32KIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, RATIONALKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, BOOLEANKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, STRINGKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, COLORSPACEKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, COLORKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, DISTANCEKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, POINTKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, DIRECTIONKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, POLYKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }
      
	  if (omfiDatakindLookup(fileHdl, MATTEKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, PICTWITHMATTEKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	  if (omfiDatakindLookup(fileHdl, STEREOKIND, &def, &omfError))
	    printName(fileHdl, def);
	  else
	    {
	      RAISE(OM_ERR_INVALID_DATAKIND);
	    }

	} /* kOmfRef2x */

      CHECK(omfsCloseFile(fileHdl));
      CHECK(omfsEndSession(session));

      printf("DumpDK completed successfully\n");
    }

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
  omfUniqueName_t name;
  omfErr_t omfError;

  omfError = omfiDatakindGetName(fileHdl, def, 64, name);
  if (omfError == OM_ERR_NONE)
    printf("Datakind Name = %s\n", name);
  else
    {
      printf("***ERROR: %s\n", omfError, omfsGetErrorString(omfError));
    }
}
