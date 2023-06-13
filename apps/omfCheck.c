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
#include "masterhd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define OMFI_ENABLE_SEMCHECK 1 /* For semantic checking fns in omUtils.h */
#include "omPublic.h"
#include "omMedia.h"

#if PORT_SYS_MAC && !defined(MAC_DRAG_DROP)
#include <Types.h>
#include <console.h>
#include <StandardFile.h>
#endif
#if PORT_SYS_MAC 
#include "MacSupport.h"
#include "omcSD2.h" 
#endif

#ifdef MAC_DRAG_DROP
#include "Main.h"
#endif
#if AVID_CODEC_SUPPORT
#include "omcAvJPED.h" 
#include "omcAvidJFIF.h"
#endif

/***********
 * DEFINES *
 ***********/
#define TRUE 1
#define FALSE 0
/*#define NULL 0	*/

/**********
 * MACROS *
 **********/
#define ExitIfNull(b, m) \
  if (!b) { \
      printf("%s: %s\n", cmdname, m); \
      return(-1); \
    }

#define streq(a,b) (strncmp(a, b, (size_t)4) == 0)

/********************/
/* STATIC FUNCTIONS */
/********************/
static void usage(char *cmdName, omfProductVersion_t toolkitVers,
				  omfInt32 appVers);
static void printHeader(char *name, omfProductVersion_t toolkitVers, 
						            omfInt32 appVers);

/********************/
/* GLOBAL VARIABLES */
/********************/
omfBool gVerbose = FALSE, gWarnings = FALSE;
const omfInt32 omfiAppVersion = 1;

/****************
 * MAIN PROGRAM *
 ****************/
int main(int argc, char *argv[])
{
    omfSessionHdl_t session;
    omfHdl_t	file;
    char *cmdname;
    omfInt32	tstErr, tstWarn;
    omfInt32	fileErrors, fileWarnings;
    omfInt32	totalErrors, totalWarnings;
	omfFileCheckHdl_t	hdl;
	omfFileRev_t fileRev;
	char        *fname, *printfname;
    char errBuf[256];
    omfErr_t omfError = OM_ERR_NONE;
#if PORT_SYS_MAC && !defined(MAC_DRAG_DROP)
	char            namebuf[255];
    Point where;
	SFReply         fs;
	SFTypeList      fTypes;
#else
	int             filearg;
#endif

#if PORT_SYS_MAC 
	MacInit();  
#endif

	XPROTECT(NULL)
	  {
#if PORT_SYS_MAC && !defined(MAC_DRAG_DROP)
	    argc = ccommand(&argv); 

	    where.h = 20;
	    where.v = 40;
#else
	    if (argc < 2)
		{
			usage(argv[0], omfiToolkitVersion, omfiAppVersion);
			return(-1);
		}
#endif
		/* Process command line switches */	
	    cmdname = argv[0];
		argc--, argv++;
		for (; argc > 0 && argv[0][0] == '-'; argc--, argv++) 
		{ 
			if (streq(argv[0], "-h")) 
				usage(cmdname, omfiToolkitVersion, omfiAppVersion);
			else if (streq(argv[0], "-a"))
			{
				gWarnings = TRUE;
				gVerbose = TRUE;
			}
			else if (streq(argv[0], "-v")) 
				gVerbose = TRUE;
			else if (streq(argv[0], "-w"))  
				gWarnings = TRUE;
		}

		totalErrors = 0;
		totalWarnings = 0;

		/* Initialize OMF Session */
		printHeader("OMFI Semantic Checker", omfiToolkitVersion, 
					omfiAppVersion);
		CHECK(omfsBeginSession(0, &session));
		CHECK(omfmInit(session));
#if AVID_CODEC_SUPPORT
		CHECK(omfmRegisterCodec(session, omfCodecAvJPED, kOMFRegisterLinked));
		CHECK(omfmRegisterCodec(session, omfCodecAvidJFIF, kOMFRegisterLinked));
#if PORT_SYS_MAC 
		CHECK(omfmRegisterCodec(session, omfCodecSD2, kOMFRegisterLinked));
#endif
#endif

		/* BEGINNING OF WHILE LOOP */
#if PORT_SYS_MAC && !defined(MAC_DRAG_DROP)
		while (1)			/* Until the user chooses quit from the menu */
#else				
		for(filearg = 0; filearg < argc; filearg++)
#endif
		  {
		    fileErrors = 0;
		    fileWarnings = 0;

#if PORT_SYS_MAC && !defined(MAC_DRAG_DROP)
			/* Get the file name and open */
			fs.fName[0] = '\0';
			fTypes[0] = 'OMFI';
			SFGetFile(where, NULL, NULL, -1, fTypes, NULL, &fs);
			
			/* If the name is empty, exit */
			if (fs.fName[0] == '\0')
			  return(0);
			ExitIfNull(fs.good, "Exiting omfCheck...\n");
			
			fname = (char *) &fs;
			strncpy(namebuf, (char *) &(fs.fName[1]), (size_t) fs.fName[0]);
			namebuf[fs.fName[0]] = 0;
			printfname = namebuf;
#else				/* Not Macintosh */
			fname = argv[filearg];
			printfname = fname;
#endif

			printf("\n***** OMF File: %s *****\n", printfname);
		        CHECK(omfsOpenFile((fileHandleType) fname, session, &file));
			CHECK(omfsFileGetRev(file, &fileRev));
			if(fileRev == kOmfRev1x)
				printf("File is being checked as an OMFI 1.0 file\n");
			else if (fileRev == kOmfRevIMA)
				printf("File is being checked as an OMFI 1.0 file (IMA)\n");
			else
				printf("File is being checked as an OMFI 2.0 file\n");
			CHECK(omfsFileCheckInit(file, &hdl));

		    /***************************************************************/
		    printf("PASS 1.... Verify individual objects and their properties\n");
		    printf("**********************************************\n");
		    CHECK(omfsFileCheckObjectIntegrity(hdl,
	    				(gVerbose ? kCheckVerbose : kCheckQuiet),
	    				(gWarnings ? kCheckPrintWarnings : kCheckNoWarnings),
						&tstErr, &tstWarn, stdout));
			fileErrors += tstErr;
			fileWarnings += tstWarn;
		
		    /***************************************************************/
		    printf("PASS 2.... Verify mob structures\n");
		    printf("********************************\n");
			omfsSemanticCheckOff(file);
		    CHECK(omfsFileCheckMobData(hdl,
	    				(gVerbose ? kCheckVerbose : kCheckQuiet),
	    				(gWarnings ? kCheckPrintWarnings : kCheckNoWarnings),
						&tstErr, &tstWarn, stdout));
			omfsSemanticCheckOn(file);
			fileErrors += tstErr;
			fileWarnings += tstWarn;

		    /***************************************************************/
		    printf("PASS 3.... Verify Local Media Data Integrity\n");
		    printf("********************************\n");
			CHECK(omfsFileCheckMediaIntegrity(hdl,
	    				(gVerbose ? kCheckVerbose : kCheckQuiet),
	    				(gWarnings ? kCheckPrintWarnings : kCheckNoWarnings),
					&tstErr, &tstWarn, stdout));

			/* Cleanup and close file */
		    CHECK(omfsFileCheckCleanup(hdl));
		    CHECK(omfsCloseFile(file));

		    totalErrors += fileErrors;
		    totalWarnings += fileWarnings;
		    if((fileErrors == 0) && (fileWarnings == 0))
		    	printf("File %s has no errors.\n", printfname);
		    else
		    	printf("File %s has %ld errors and %ld warnings\n",
		    		printfname, fileErrors, fileWarnings);
		  } /* End of loop */

	    CHECK(omfsEndSession(session));
	
	    if((totalErrors == 0) && (totalWarnings == 0))
	  	  printf("SemCheck completed successfully\n");
	 	else
		    printf("Found a total of %ld errors and %ld warnings in all files.\n",
		    		totalErrors, totalWarnings);
	} /* XPROTECT */
	XEXCEPT
    {
		printf("*** ERROR: %d: %s\n", XCODE(), 
                                      omfsGetExpandedErrorString(file, 
									  XCODE(), sizeof(errBuf), errBuf));
		return(-1);
	}
	XEND;
	
    return(0);
}


/*********/
/* USAGE */
/*********/
static void usage(char *cmdName, 
				  omfProductVersion_t toolkitVers,
				  omfInt32 appVers)
{
	fprintf(stderr, "%s %ld.%ld.%ldr%ld\n", cmdName, 
			toolkitVers.major, toolkitVers.minor, toolkitVers.tertiary,
			appVers);
	fprintf(stderr, "Usage: %s [-h] [-v] [-w] [-a] <filename>\n", cmdName);
	fprintf(stderr, "%s semantically verifies an OMFI file\n", cmdName);
}

/***************/
/* printHeader */
/****************/
static void printHeader(char *name,
						omfProductVersion_t toolkitVers,
						omfInt32 appVers)
{
	fprintf(stderr, "%s %ld.%ld.%ldr%ld\n\n", name, 
			toolkitVers.major, toolkitVers.minor, toolkitVers.tertiary,
			appVers);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
