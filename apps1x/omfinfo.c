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

/***************************************************************************
 * apps/omfinfo: This application finds the media data objects in the OMF  *
 *               file (audio and video) and prints the descriptor          *
 *               information.                                              *
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef MSDOS
#include <stdlib.h>
#endif

#if defined(THINK_C) || defined(__MWERKS__)
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

#include "OMFI.h"


/*
 *  Avid MediaComposer/FilmComposer magic numbers
 */
#define avr1_id 28
#define avr1e_id 38
#define avr2_id 29
#define avr2e_id 39
#define avr3_id 27
#define avr3e_id 37
#define avr4_id 30
#define avr4e_id 40
#define avr5_id 31
#define avr5e_id 41
#define avr6e_id 42
#define avr23_id 12
#define avr24_id 13
#define avr24e_id 24
#define avr25_id 15
#define avr25b_id 14
#define avr25e_id 25
#define avr26_id 16
#define avr27_id 34
#define mspi_id 26    /* Media Suite Pro Indigo, same id as Media Composer's
			 unused AVR26e */

/*
 *  	Display OMFI information on screen
 *
 */
static char *layoutStr[] = {
  "no layout",
  "full frame",
  "one field",
  "interlaced fields"
};

static char *compressionStr[] = {
  "none",
  "JPEG"
};

static char *audioCompressionStr[] = {
  "none",
  "LRC"
};

static char *formatStr[] = {
  "no format",
  "rgb",
  "yCbCr"
};

static char outstr[256];    /* Formatting strings */

/* ERROR MACROS */
/* ExitOnOMFError macro to exit w/ a message if the toolkit reports an error */
#define ExitOnOMFError(f) \
{ int  errcode = 0; \
    errcode = omfiGetErrorCode(); \
    if (errcode) \
      printf("%s\n", omfiGetErrorString(OMErrorCode)); \
    if (omfiGetBentoErrorRaised()) \
      printf("Bento error %1ld: %s\n",omfiGetBentoErrorNumber(), \
	     omfiGetBentoErrorString()); \
    if (errcode) { \
      omfiCloseFile(f); \
      omfiEndSession(); \
      exit(OMErrorCode ? (int)OMErrorCode : 1); \
     } \
}

#define ContinueOnOMFError(f, m) \
{ int  errcode = 0; \
    errcode = omfiGetErrorCode(); \
    if (errcode) \
      printf("%s\n", OMGetErrorString(OMErrorCode)); \
    if (omfiGetBentoErrorRaised()) \
      printf("Bento error %1ld: %s\n",omfiGetBentoErrorNumber(),\
	     omfiGetBentoErrorString()); \
    if (errcode) { \
      printf("%s: %s\n", argv[0], m); \
      continue; \
    } \
}

#define ExitIfNull(b, m) \
  if (!b) { \
      printf("%s: %s\n", argv[0], m); \
      exit(-1); \
    }

/****************
 * MAIN PROGRAM *
 ****************/
void main(int argc , char *argv[])
{
  filePtr f;
  mediaType mtype;
  mediaDesc_t mdesc;
  int  n, nObjects;    
  int  numFrames;
  omfMobID mobID;
  int  filearg;
  char *fname, *printfname;
  omfFileRev_t fileRev;
#if defined(THINK_C) || defined(__MWERKS__)
  char namebuf[255];
#endif
  double eRate;
    
#if defined(THINK_C) || defined(__MWERKS__)
  Point where;
  SFReply fs;
  SFTypeList fTypes;
#endif  

#if defined(THINK_C) || defined(__MWERKS__)
  /* console_options.nrows = 54; */
  /* console_options.ncols = 132; */
  argc = ccommand(&argv); 

  /*
   *	Standard Toolbox stuff
   */
#ifdef THINK_C
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
#else
  ExitIfNull((argc >= 2), "Usage:  omfinfo <omf-file>\n");
#endif

  /* Start an OMF Session */
  omfiBeginSession();
  
/* BEGINNING OF WHILE LOOP */
#if defined(THINK_C) || defined(__MWERKS__)
  while (1)  /* Until the user chooses quit from the menu */

#else        /* Command line interface */
  for (filearg=1; filearg<argc; filearg++)
#endif
  {

    /* Get the OMF filename */
#if defined(THINK_C) || defined(__MWERKS__)
    /* Get the file name and open */
    fs.fName[0] = '\0';
    fTypes[0] = 'OMFI';
    SFGetFile(where,  NULL, NULL, -1, fTypes, NULL, &fs);
    
    /* If the name is empty, exit */
    if (fs.fName[0] == '\0')
        exit(0);
    ExitIfNull(fs.good, "Exiting omfinfo...");

    fname = (char *)&fs;
    strncpy(namebuf, (char *)&(fs.fName[1]), (size_t)fs.fName[0]);
    namebuf[fs.fName[0]] = 0;

    printfname = namebuf;

#else  /* Not Macintosh */
    fname = argv[filearg];
    printfname = fname;
#endif

    /* Open the OMF file */
    printf("\n***** OMF File: %s *****\n", printfname);

    f = omfiOpenFile(fname);
    ContinueOnOMFError(f, "Open Failed: Skipping Bad OMFI File");

    /**************************/
    /* NOTE: This is a diversion to the 2.0 API in order to be able
     *       to print out the file revision...
     */
    omfsFileGetRev(GetFileHdl(f), &fileRev);

    if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
      printf("%s: OMFI File Revision: 1.0\n", printfname);
    else if (fileRev == kOmfRev2x)
      printf("%s: OMFI File Revision: 2.0\n", printfname);
    else
      printf("%s: OMFI File Revision: UNKNOWN\n", printfname);
    /**************************/
   
    nObjects = omfmGetNumMediaObjects(f);
    ContinueOnOMFError(f, "GetNumMediaObjects Failed: Skipping Bad OMFI File");
    
    if (!nObjects)
        printf("No Media Objects in the OMF file: %s\n", printfname);

    for (n = 1; n <= nObjects; n++)
      {
	omfmGetNthMediaDesc(f, n, &mtype, &mdesc, &mobID);
	ContinueOnOMFError(f, "Skipping media object");
	    
	if (mtype == imageMedia)
	  {
	    printf("\nIMAGE OBJECT    ( %1ld , %1ld , %1ld )\n",
		   mobID.prefix, 
		   mobID.major, 
		   mobID.minor);

	    if (! mdesc.image.name)
	      mdesc.image.name = "  ";
	    printf("   Name         %s\n", mdesc.image.name);
	    
	    if (! mdesc.image.filename)
	      mdesc.image.filename = "  ";
	    printf("   Filename     %s\n", mdesc.image.filename);
	    
	    eRate = OMFloatFromExactEditRate(mdesc.image.editrate);
	    printf("   Edit Rate    %f\n", eRate);
	    
	    printf("   Bits/Pixel   %1d\n", mdesc.image.bitsPerPixel);
	    
	    printf("   Image Size   %1d x %1d\n", 
		   mdesc.image.width, 
		   mdesc.image.height);

	    printf("   Lead Lines   %ld\n", mdesc.image.leadingLines );
	    printf("   Trail Lines  %ld\n", mdesc.image.trailingLines);

	    printf("   Layout       %s\n", layoutStr[mdesc.image.layout]);
	    
	    printf("   Compression  %s\n", 
		   compressionStr[mdesc.image.compression]);
	    if (mdesc.image.compression == JPEG && mdesc.image.JPEGtableID)
	      {
		switch (mdesc.image.JPEGtableID)
		  {
		  case avr1_id: sprintf(outstr, "JPEG Tbl ID  AVR1"); break;
		  case avr1e_id: sprintf(outstr, "JPEG Tbl ID  AVR1e");  break;
		  case avr2_id: sprintf(outstr, "JPEG Tbl ID  AVR2"); break;
		  case avr2e_id: sprintf(outstr, "JPEG Tbl ID  AVR2e"); break;
		  case avr3_id: sprintf(outstr, "JPEG Tbl ID  AVR3"); break;
		  case avr3e_id: sprintf(outstr, "JPEG Tbl ID  AVR3e"); break;
		  case avr4_id: sprintf(outstr, "JPEG Tbl ID  AVR4"); break;
		  case avr4e_id: sprintf(outstr, "JPEG Tbl ID  AVR4e"); break;
		  case avr5_id: sprintf(outstr, "JPEG Tbl ID  AVR5"); break;
		  case avr5e_id: sprintf(outstr, "JPEG Tbl ID  AVR5e"); break;
		  case avr6e_id: sprintf(outstr, "JPEG Tbl ID  AVR6e"); break;
		  case avr23_id: sprintf(outstr, "JPEG Tbl ID  AVR23"); break;
		  case avr24_id: sprintf(outstr, "JPEG Tbl ID  AVR24"); break;
		  case avr24e_id: sprintf(outstr,"JPEG Tbl ID  AVR24e"); break;
		  case avr25_id: sprintf(outstr, "JPEG Tbl ID  AVR25"); break;
		  case avr25e_id: sprintf(outstr,"JPEG Tbl ID  AVR25e"); break;
		  case avr25b_id: sprintf(outstr,"JPEG Tbl ID  AVR25b"); break;
		  case avr26_id: sprintf(outstr, "JPEG Tbl ID  AVR26"); break;
		  case avr27_id: sprintf(outstr, "JPEG Tbl ID  AVR27"); break;
		  case mspi_id: sprintf(outstr,"JPEG Tbl ID  MSPIndigo");break;

		  default:
		    sprintf(outstr, "JPEG Tbl ID  %ld", 
			    mdesc.image.JPEGtableID);
		    break;
		  }
		printf("   %s\n", outstr);
	      }
	  }

	else if (mtype == audioMedia)
	  {
	    printf("\nSOUND OBJECT    ( %1ld , %1ld , %1ld )\n",
		   mobID.prefix, 
		   mobID.major, 
		   mobID.minor);

	    if (! mdesc.audio.name) mdesc.audio.name = "  ";
	    printf("   Name         %s\n", mdesc.audio.name);

	    if (! mdesc.audio.filename)mdesc.audio.filename = "  ";
	    printf("   Filename     %s\n", mdesc.audio.filename);

	    eRate = OMFloatFromExactEditRate(mdesc.audio.editrate);
	    printf("   Edit Rate    %f\n", eRate);

	    printf("   Sample Size  %1ld\n", mdesc.audio.sampleSize);

	    printf("   No. Channels %1d\n", mdesc.audio.numChannels);

	    printf("   Compression  %s\n", 
		   audioCompressionStr[mdesc.audio.compression]);
	  }

	numFrames = omfmGetNumSamples(f);
	ContinueOnOMFError(f, "Skipping media object");

	printf("   No. Samples  %ld\n\n",numFrames);
      }

    /* Close the OMF File */
    omfmCleanup(f);
    omfiCloseFile(f);
  }

  /* Close the OMF Session */
  omfiEndSession();
}
