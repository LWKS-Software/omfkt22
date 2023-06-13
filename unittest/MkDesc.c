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
 * MkDesc - This unittest creates a 2.x file with a tape and film
 *          mob in it.  It then writes and reads back (to verify) 
 *          tape and film descriptor information.
 ********************************************************************/

#include "masterhd.h"
#include "stdio.h"
#include "string.h"

#include "omDefs.h" 
#include "omTypes.h"
#include "omUtils.h"
#include "omFile.h"
#include "omMobBld.h"
#include "omMobGet.h"
#include "omMedia.h"
#include "omCvt.h"

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

#define NAMESIZE 32

#ifdef MAKE_TEST_HARNESS
int MkDesc(char *filename, char *version)
{
	int argc;
	char *argv[3];
#else
int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t	session;
  omfHdl_t	fileHdl;
  omfMobObj_t filmMob, tapeMob;
  omfTapeCaseType_t formFactor, formFactorRes;
  omfVideoSignalType_t videoSignal, videoSignalRes;
  omfTapeFormatType_t tapeFormat, tapeFormatRes;
  omfLength_t length, lengthRes;
  char manufacturer[NAMESIZE], model[NAMESIZE]; 
  char manufacturerRes[NAMESIZE], modelRes[NAMESIZE];
  omfFilmType_t filmFormat, filmFormatRes;
  omfUInt32 frameRate;
  omfUInt8 perfPerFrame, perfPerFrameRes;
  omfRational_t aspectRatio, aspectRatioRes;
  omfFileRev_t rev;
  omfDDefObj_t pictureDef = NULL;
  omfRational_t editRate;
  omfLength_t clipLen0;
  omfPosition_t zeroPos;
  omfObject_t fillerTape = NULL, fillerFilm = NULL;
  omfObject_t slotTape = NULL, slotFilm = NULL;
  omfErr_t omfError = OM_ERR_NONE;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "MkDesc UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
 	ProductInfo.platform = NULL;

  XPROTECT(NULL)
    {
#ifdef MAKE_TEST_HARNESS
      argc = 3;
      argv[1] = filename;
      argv[2] = version;
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

      if (argc < 3)
	{
	  printf("*** ERROR - MkDesc <filename> <2>\n");
	  return(1);
	}
      
      if (!strcmp(argv[2], "2"))
	rev = kOmfRev2x;
      else 
	{ 
	  printf("*** ERROR - Illegal file rev\n");
	  return(1);
	}

      CHECK(omfsBeginSession(&ProductInfo, &session));
      CHECK(omfmInit(session));
      printf("Creating new OMF file...\n");
      CHECK(omfsCreateFile((fileHandleType) argv[1], session, 
			   rev, &fileHdl));

      /* Initialize stuff */
      omfiDatakindLookup(fileHdl, PICTUREKIND, &pictureDef, &omfError);
      omfsCvtInt32toPosition(0, zeroPos);
      omfsCvtInt32toInt64(10, &clipLen0);
      editRate.numerator = 2997;
      editRate.denominator = 100;

      printf("Creating tape mob...\n");
      CHECK(omfmTapeMobNew(fileHdl, "TapeMob", &tapeMob));
      CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen0, &fillerTape));
      CHECK(omfiMobAppendNewTrack(fileHdl, tapeMob, editRate, fillerTape,
				    zeroPos, 1, "Tape Mob VTrack", &slotTape));
      formFactor = kBetacamVideoTape;
      videoSignal = kPALSignal;
      tapeFormat = kHi8Format;
      omfsCvtInt32toInt64(500, &length);
      strcpy(manufacturer, "Some Tape Manufacturer");
      strcpy(model, "Some Tape Model");
      if (rev == kOmfRev2x) {
	printf("Adding descriptor info to tape mob...\n");
	CHECK(omfmTapeMobSetDescriptor(fileHdl, tapeMob, &formFactor, 
				       &videoSignal, &tapeFormat, &length, 
				       manufacturer, model));
      }

      printf("Creating film mob...\n");
      CHECK(omfmFilmMobNew(fileHdl, "FilmMob", &filmMob));
      CHECK(omfiFillerNew(fileHdl, pictureDef, clipLen0, &fillerFilm));
      CHECK(omfiMobAppendNewTrack(fileHdl, filmMob, editRate, fillerFilm,
				    zeroPos, 1, "Film Mob VTrack", &slotFilm));
      if (rev == kOmfRev2x) 
	{
	  filmFormat = kFt35MM;
	  perfPerFrame = 8;
	  aspectRatio.numerator = 1;
	  aspectRatio.denominator = 2;
	  strcpy(manufacturer, "Some Film Manufacturer");
	  strcpy(model, "Some Film Model");
	  printf("Adding descriptor info to film mob...\n");
	  CHECK(omfmFilmMobSetDescriptor(fileHdl, filmMob, &filmFormat,
					 NULL /* frame rate */, 
					 &perfPerFrame, &aspectRatio,
					 manufacturer, model));
	}
      if (rev == kOmfRev2x) 
	{
	  printf("Reading descriptor information from tape mob...\n");
	  CHECK(omfmTapeMobGetDescriptor(fileHdl, tapeMob,
					 &formFactorRes,
					 &videoSignalRes,
					 &tapeFormatRes,
					 &lengthRes,
					 NAMESIZE, manufacturerRes,
					 NAMESIZE, modelRes));
	  if (formFactorRes != kBetacamVideoTape)
	    printf("ERROR -- Form Factor not Betacam on tape mob\n");
	  if (videoSignalRes != kPALSignal)
	    printf("ERROR -- Video Signal not PAL on tape mob\n");
	  if (tapeFormatRes != kHi8Format)
	    printf("ERROR -- Tape format not Hi* on tape mob\n");
	  if (omfsInt64NotEqual(length, lengthRes))
	    printf("ERROR -- Length not 500 in tape mob\n");
	  if (strcmp("Some Tape Manufacturer", manufacturerRes))
	    printf("ERROR -- Manufacturer not correct in tape mob\n");
	  if (strcmp("Some Tape Model", modelRes))
	    printf("ERROR -- Model not correct in tape mob\n");

	  printf("Reading descriptor information from film mob...\n");
	  CHECK(omfmFilmMobGetDescriptor(fileHdl, filmMob, &filmFormatRes,
					 &frameRate,
					 &perfPerFrameRes,
					 &aspectRatioRes,
					 NAMESIZE, manufacturerRes,
					 NAMESIZE, modelRes));
	  if (filmFormatRes != kFt35MM)
	    printf("ERROR -- Film Format no 35MM in Film Mob\n");
	  if (perfPerFrameRes != 8)
	    printf("ERROR -- Perforatnions Per Frame not 8 in Film Mob\n");
	  if ((aspectRatioRes.numerator != 1) && 
	      (aspectRatioRes.denominator != 2))
	    printf("ERROR -- Aspect Ratio not correct in Film Mob\n");
	  if (strcmp("Some Film Manufacturer", manufacturerRes))
	    printf("ERROR -- Manufacturer not correct in Film Mob\n");
	  if (strcmp("Some Film Model", modelRes))
	    printf("ERROR -- Film Model not correct in Film Mob\n");
	}
	
      CHECK(omfsCloseFile(fileHdl));
      CHECK(omfsEndSession(session));
    }

  XEXCEPT
    {
      printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
      return(-1);
    }
  XEND;

  return(0);
}
