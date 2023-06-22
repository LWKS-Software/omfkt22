/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                         <<<    CrptTest.c     >>>                         |
 |                                                                           |
 |                    Container Manager Crypt Value Tests                    |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 5/12/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file is only used to test encryption values using dynamic values.  Dynamic values
 are given their meaning through their handlers.  Handlers are generally user-written and
 not considered strictly part of the Container Manager.  So their tests are kepts separate
 (besides it's easier to debug them this way).

 See ExampleCryptHandlers.[ch] for full documentation on indirect file values.

 Note, we have other tests to test out the basic operations of dynamic values.  The main
 purpose of the tests in this file are to test subclassing types.  In particular we use the
 file types as a base class to crypt types to effect encrypted file I/O.  The tests in this
 file are basically the same as those in file tests (    IndFTest.c     ).  Indeed, we
 used that file as the basis for this one.
*/

/*#include <types.h>*/
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* The following is for testing under Think C. It allows us to generate a Unix or MPW		*/
/* command line argv vector. CAUTION about using Think C -- The standard I/O only works	*/
/* if you use 2-byte integers. 																													*/

#ifndef THINK_C
#define THINK_C 0
#endif
#ifndef THINK_CPLUS
#define THINK_CPLUS 0
#endif

#if THINK_C || THINK_CPLUS
#include <console.h>
#endif

#include "CMAPI.h"
#include "CMAPIDbg.h"    							/* assume the debugging support is there!					*/
#include "XHandlrs.h"
#include "XSession.h"
#include "XSubValu.h"
#include "XIndFile.h"
#include "XCrypt.h"

																	CM_CFUNCTIONS

#ifndef __TYPES__
enum {false,true};
typedef unsigned char Boolean;
#endif


static FILE 		*dbgFile = NULL;
static FILE			*containerHandlerDbgFile = NULL;
static int 			failures = 0, successes = 0;
static char 		whatWeGot[400], filename[256] = {0}, dataFilename[256] = {0};
static Boolean 	traceContainerHandlers = false,
								dontClose = false,
								showRaw = false,
								quiet = true;

#define SUBVALUES 1

#define KEY 		"Ira L. Ruben"
#define KEYSIZE 12


/* The following is a cheap and dirty way to register type names used in the various		*/
/* tests.  Each group of tests placed into a single property must be unique.  An 				*/
/* alternate scheme would be to use the same type by different properties.  Due to the	*/
/* historical (or hysterical) way the API evolved, changing the types is more 					*/
/* convenient.																																					*/

#define TestType(t) (CMRegisterType(container, (CMGlobalName)t))


/* In order to test and debug the crypt type example handlers we need some "inside"			*/
/* knowledge!  Specifically, the crypt type handlers write 16 bytes in front of the 		*/
/* "real" data.  The first 4 bytes are the key size and then the key follows.  We are 	*/
/* using a KEYSIZE-byte key, therefore...																								*/

#define KeyDataSize (4 + KEYSIZE)


/*------------------------------*
 | display - isolate all output |
 *------------------------------*

 In some environments it may be more desirable (or necessary) to handle I/O specially. To
 that end this routine is provided.  There is no input in this test (other than the
 command line -- if you got one).  So only output needs to be handled. It all goes through
 here.  It's parameters are identical to fprintf(), with the same meaning.  So, as
 delivered, in its simplest form, this routine maps into a fprintf().  Feel free to "warp"
 this routine into somthing appropriate to your system.
*/

static void CM_NEAR CM_C display(FILE *stream, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stream, format, ap);
	va_end(ap);
}


/*----------------*
 | processOptions |
 *----------------*/

static void CM_NEAR processOptions(int argc, char *argv[])
{
	int  i;
	char c, *arg;
	FILE *f;

	#define CmdLineOptionSyntax "[containerFile [dataFile]] [-t stderr|stdout] [-d] [-p] [-r] [-x stderr|stdout]"

	for (i = 1; i < argc; ++i) {
		arg = argv[i];
		if (*arg == '-')
			switch (c = *++arg) {
				case '?':
				case 'h':	display(stderr, "\n%s %s\n\n"
																	"   containerFile     Name of container file.  If omitted, the file\n"
																	"                     \"CrptCont\" is used.\n\n", argv[0], CmdLineOptionSyntax);
									display(stderr, "   dataFile          Name of data file to be indirectly created.  If\n"
																	"                     omitted, \"DataFile\" is used.\n\n");
									display(stderr, "   -t stderr|stdout  Send handler debugging output and input TOC display\n"
																	"                     to the specified file.  If omitted, no debugging\n"
																	"                     output is generated.\n\n");
									display(stderr, "   -d                Display the \"raw\" encrypted data file.\n\n");
									display(stderr, "   -p                Display test output to stderr.  The defualt is to run\n"
																	"                     quietly and self-check the results.  A summary is given\n"
																	"                     at the end of the program.\n\n");
									display(stderr, "   -r                The default is to treat each test separately so that the\n"
																	"                     container written to is closed and then reopened for\n"
																	"                     reading it.  This option suppresses that. The container\n"
																	"                     is opened, written, read, and then closed with -r.\n\n");
									display(stderr, "   -x stderr|stdout  Trace handlers to the specified file.\n");

									exit(EXIT_SUCCESS);

				case 'd':	showRaw = true;
									break;

				case 'p':	quiet = false;
									break;

				case 'r': dontClose = true;
									break;

				case 't':
				case 'x':	if ((arg = argv[++i]) != NULL)
										if (strcmp(arg, "stderr") == 0)
											f = stderr;
										else if (strcmp(arg, "stdout") == 0)
											f = stdout;
										else
											arg = NULL;
									if (arg == NULL) {
										display(stderr, "### Usage: %s %s\n"
																	  "###        Should have -t stdout | stderr on -%c\n", argv[0], CmdLineOptionSyntax, c);
										exit(EXIT_SUCCESS);
									}
									if (c == 'x') {
										traceContainerHandlers = true;
										containerHandlerDbgFile = f;
									} else
										dbgFile = f;
									break;

				default:	display(stderr, "### Usage: %s %s\n"
																	"###        Invalid option: \"%s\"\n", argv[0], CmdLineOptionSyntax, arg-1);
									exit(EXIT_FAILURE);
			}
		else if (*filename == 0)
			strcpy( (char*) filename, arg);
		else if (*dataFilename == 0)
			strcpy( (char*) dataFilename, arg);
		else {
			display(stderr, "### Usage: %s %s\n"
											"###        Too many filenames specified\n", argv[0], CmdLineOptionSyntax);
			exit(EXIT_FAILURE);
		}
	}
}


#if 0
/*----------------------*
 | Encrypt/Decrypt Data |
 *----------------------*

 This is the encryption algorithm used in the crypt type handler.  It's useful for looking
 at the "raw" encrypted data.
*/

static void CM_NEAR crypt(unsigned char *data, unsigned int  dataSize,
                                       unsigned char *key,  unsigned int  keySize,
                                       unsigned int  offset)
{
	unsigned char *keyStart = key;

	if (keySize == 0) return;									/* no key means no encryption								*/

	key += (offset % keySize);								/* starting key char is a function of offset*/

	while (dataSize--) {											/* process each character of the data...		*/
		*data++ ^= *key++;											/* ...encrypt/decrypt it										*/
		if (++offset % keySize == 0)						/* ...if we exhausted the key...						*/
			key = keyStart;												/* ...cycle it															*/
	}
}
#endif


/*----------------------------------------*
 | s2x - convert a string to a hex string |
 *----------------------------------------*

 Converts string s to hex and places the characters in x in the following format...

               xxxxxxxxxx...  "ccccc..."

 The string is the character equivalent to the hex.  Hex values les then ' ' and greater
 than '~' (i.e., most printable ASCII) are shown as '.'.
*/

static char *CM_NEAR s2x(char *x, char *s)
{
	char c, c1, c2, *x0 = x, *x1;

	x1 = x + (strlen(s) * 2);
	*x1++ = ' ';
	*x1++ = ' ';
	*x1++ = '"';

	while ((CMBoolean)(c = *s++)) {
		c1 = "0123456789ABCDEF"[(c >> 4) & 0x0F];
		c2 = "0123456789ABCDEF"[c & 0x0F];
		*x++ = c1;
		*x++ = c2;

		*x1++ = (char)((c < ' ' || c > '~') ? '.' : c);
	}

	*x1++ = '"';
	*x1		= '\0';

	return (x0);
}


/*---------*
 | checkIt |
 *---------*/

static void CM_NEAR checkIt(char *whatWeGot, char *whatItShouldBe)
{
	if (strcmp(whatWeGot, whatItShouldBe) != 0) {
		display(stderr, "### test failed! Expected \"%s\"\n"
										"                 Got      \"%s\"\n", whatItShouldBe, whatWeGot);
		++failures;
	} else {
		++successes;
		if (!quiet) display(stderr, "%s\n", whatWeGot);
	}
}


/*---------*
 | doTests |
 *---------*/

static void CM_NEAR doTests(void)
{
	CMContainer 	 container;
	CMRefCon			 myRefCon;
	CMObject			 o1, o2;
	CMProperty		 p1, p2, p3, p4;
	CMType				 t0, t1, t2, t3, t4, t5;
	CMValue				 v21, v22, v31, v32, v33;
	CMSize				 amountRead, fileSize;
	CMCount				 offset;
	CMGeneration 	 generation;
	CMSession			 session;
	unsigned char  buffer[256];

	if (*filename == 0) strcpy( (char*) filename, "CrptCont"      );
	if (*dataFilename == 0) strcpy( (char*) dataFilename, "DataFile");

	session = CMStartSession(sessionRoutinesMetahandler, NULL);
	CMDebugging(session, 256, dbgFile, 1);

	myRefCon = createRefConForMyHandlers(session, filename, NULL);
	setHandlersTrace(myRefCon, traceContainerHandlers, containerHandlerDbgFile);

	if (!quiet) display(stderr, "\nContainer we are using is: \"%s\"\n\n", filename);

	CMSetMetaHandler(session, "Debugging1", containerMetahandler);
	container = CMOpenNewContainer(session, myRefCon, "Debugging1", 0, 1, 0);
	if (container == NULL) {
		display(stderr, "### writing tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}

	p1 = CMRegisterProperty(container, "pointer");
	p2 = CMRegisterProperty(container, "Indirect file direct value tests");
	p3 = CMRegisterProperty(container, "Subvalue of crypt/file value tests");
	p4 = CMRegisterProperty(container, "Encryption/decryption tests");

	t1 = CMRegisterType(container, "pointer type");
	t2 = CMRegisterType(container, "The indirect value descriptor type");
	t3 = CMRegisterType(container, "The subvalue descriptor type");
	t4 = CMRegisterType(container, "The crypt/file descriptor type #1");
	t5 = CMRegisterType(container, "The crypt/file descriptor type #2");


	/*--------------------------*
	 | Create a crypt/file type |
	 *--------------------------*/

	CMAddBaseType(t4, RegisterFileValueType(container));		/* file type at lowest level 	*/
	CMAddBaseType(t4, RegisterCryptValueType(container));		/* crypt type next higher			*/


	/*-----------------------------*
	 | Create the crypt/file value |
	 *-----------------------------*/

	o1  = CMNewObject(container);
	v21 = CMNewValue(o1, p4, t4, dataFilename, "w", KEY); 	/* pathname, mode and key 		*/


	/*-----------------------------------------------------------------------*
	 | Write encrypted data to the data file through the indirect file value |
	 *-----------------------------------------------------------------------*/
																																						 /* offsets */
	CMWriteValueData(v21, (CMPtr)"line 1", 00, 6);														 /* 00 - 05 */
	CMWriteValueData(v21, (CMPtr)"line 2", 06, 6);														 /* 06 - 11 */
	CMWriteValueData(v21, (CMPtr)"line 3", 12, 6);														 /* 12 - 17 */
	CMWriteValueData(v21, (CMPtr)"line 4", 18, 6);														 /* 18 - 23 */
	CMWriteValueData(v21, (CMPtr)"line 5", 24, 6);														 /* 24 - 29 */
	CMWriteValueData(v21, (CMPtr)"line 6", 30, 6);														 /* 30 - 35 */
	CMWriteValueData(v21, (CMPtr)"line 7", 36, 6);														 /* 36 - 41 */
	CMWriteValueData(v21, (CMPtr)"line 8", 42, 6);														 /* 42 - 47 */
	CMWriteValueData(v21, (CMPtr)"line 9", 48, 6);														 /* 48 - 53 */
	CMWriteValueData(v21, (CMPtr)"line 10 ABCDEFGHIJKLMNOPQRSTUVWXYZ", 54,34); /* 54 - 87 */
	                           /* 1234567890123456789012345678901234 */

	/* try to overwrite line 9...																													*/

	CMWriteValueData(v21, (CMPtr)"line#9", 48, 6);														 /* 48 - 53 */


	/*--------------------------------------------------------------------------*
	 | Screw around with the type to test CMGetValueInfo() and CMSetValueXXXX() |
	 *--------------------------------------------------------------------------*/

	CMGetValueInfo(v21, NULL, NULL, NULL, &t0, NULL);
	if (t0 != t4) {
		display(stderr, "### got wrong type from CMGetValueInfo()\n");
		++failures;
	} else
		++successes;

	CMAddBaseType(t5, RegisterFileValueType(container));		/* file type at lowest level 	*/
	CMAddBaseType(t5, RegisterCryptValueType(container));		/* crypt type next higher			*/

	CMSetValueType(v21, t5);
	CMSetValueGeneration(v21, 3);

	CMGetValueInfo(v21, NULL, NULL, NULL, &t0, &generation);
	if (t0 != t5) {
		display(stderr, "### got wrong new type from CMGetValueInfo()\n");
		++failures;
	} else
		++successes;
	if (generation != 3) {
		display(stderr, "### got wrong new generation from CMGetValueInfo()\n");
		++failures;
	} else
		++successes;


	#if SUBVALUES
	/*------------------------------------------------------------*
	 | Sadistic test (part 1) - subvalue of a an crypt/file value |
	 *------------------------------------------------------------*

	The following shows the data for v21 (the indirect file value data) generated above.
	Repeating it here in the format we use here makes it easier to understand what we're
	taking the subvalue of and what the expected results should be.

	          1         2         3         4         5         6         7         8
	0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567
	line 1line 2line 3line 4line 5line 6line 7line 8line 9line 10 ABCDEFGHIJKLMNOPQRSTUVWXYZ
												  | v31|            | v32|	            |          v33           |
												 24    29          42    47             62                       87*/
	if (!quiet)
		display(stderr, "\nSadistic test - subvalue of a crypt/file value:\n"
										"    012345678901234567890123456789012345678901234567890\n");

	o2  = CMNewObject(container);
	v31 = newSubValue(o2, p3, TestType("t31"), v21, 24, 6);
	v32 = newSubValue(o2, p3, TestType("t32"), v21, 42, 6);
	v33 = newSubValue(o2, p3, TestType("t33"), v21, 62, 26);
	#endif


	/*---------------------*
	 | Close the data file |
	 *---------------------*/

	if (dbgFile) CMDumpTOCStructures(container, dbgFile);

	#if SUBVALUES
	CMReleaseValue(v31);
	CMReleaseValue(v32);
	CMReleaseValue(v33);
	#endif

	CMReleaseValue(v21);


	/*----------------------------------------------------------------------------*
	 | Create the indirect file value to check to see if data is really encrypted |
	 *----------------------------------------------------------------------------*/

	if (showRaw) {
		v22 = newIndirectFileValue(o1, p2, TestType("t22"), dataFilename, "rb+");

		for (offset = KeyDataSize;; offset += amountRead) {
			amountRead = CMReadValueData(v22, (CMPtr)buffer, offset, (offset <= 48 + KeyDataSize) ? 6 : 255);
			if (amountRead == 0) break;
			*(buffer+amountRead) = 0;
			s2x(whatWeGot, (char *)buffer);
			display(stderr, "%s\n", whatWeGot);
			/*
			crypt(buffer, amountRead, (unsigned char *)KEY, KEYSIZE, offset - KeyDataSize);
			fprintf(stderr, "%s\n", buffer);
			*/
			#if 0
			if (offset < 48 + KeyDataSize) {
				sprintf(whatWeGot, "    %s [%ld]", buffer, amountRead);
				sprintf((char *)buffer, "    line %ld [6]", 1 + (offset/6));
				checkIt(whatWeGot, (char *)buffer);
			} else if (offset == 48 + KeyDataSize) {
				sprintf(whatWeGot, "    %s [%ld]", buffer, amountRead);
				sprintf((char *)buffer, "    line#%ld [6]", 1 + (offset/6));
				checkIt(whatWeGot, (char *)buffer);
			} else {
				sprintf(whatWeGot, "    %s [%ld]", buffer, amountRead);
				checkIt(whatWeGot, "    line 10 ABCDEFGHIJKLMNOPQRSTUVWXYZ [34]");
			}
			#endif
		} /* for */

		CMReleaseValue(v22);
	}

	if (dontClose) goto readAsis;

	CMCloseContainer(container);

	/*------------------------------------------------------------------------------------*/

	if (dbgFile)
		display(dbgFile, "--------------------------------------------------------------------------\n");

	myRefCon = createRefConForMyHandlers(session, filename, NULL);
	setHandlersTrace(myRefCon, traceContainerHandlers, containerHandlerDbgFile);

	CMSetMetaHandler(session, "Debugging2", containerMetahandler);
	container = CMOpenContainer(session, myRefCon, "Debugging2", 0);
	if (container == NULL) {
		display(stderr, "### reading tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}

	if (dbgFile) CMDumpTOCStructures(container, dbgFile);

readAsis:
	p1 = CMRegisterProperty(container, "pointer");
	p2 = CMRegisterProperty(container, "Indirect file direct value tests");
	p3 = CMRegisterProperty(container, "Subvalue of crypt/file value tests");
	p4 = CMRegisterProperty(container, "Encryption/decryption tests");

	t1 = CMRegisterType(container, "pointer type");
	t2 = CMRegisterType(container, "The indirect value descriptor type");
	t3 = CMRegisterType(container, "The subvalue descriptor type");
	t4 = CMRegisterType(container, "The crypt/file descriptor type #1");
	t5 = CMRegisterType(container, "The crypt/file descriptor type #2");

	if (!quiet)
		display(stderr, "\nRead tests of indirect file value data:\n"
										"    012345678901234567890123456789012345678901234567890\n");


	/*--------------------------------*
	 | Indirect to the encrypted file |
	 *--------------------------------*/

	if ((o1 = CMGetNextObjectWithProperty(container, NULL, p4)) == NULL) {
		display(stderr, "    ### cannot find encryption/decryption property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;

	v21 = CMUseValue(o1, p4, t5);


	/*----------------------------*
	 | Read in the encrypted data |
	 *----------------------------*/

	CMGetValueInfo(v21, NULL, NULL, NULL, &t0, NULL);
	if (t0 != t5) {
		display(stderr, "### got wrong new type from CMGetValueInfo prior to reading()\n");
		++failures;
	} else
		++successes;

	for (offset = 0;; offset += amountRead) {
		amountRead = CMReadValueData(v21, (CMPtr)buffer, offset, (offset <= 48) ? 6 : 255);
		if (amountRead == 0) break;
		*(buffer+amountRead) = 0;
		if (offset < 48) {
			sprintf(whatWeGot, "    %s [%ld]", buffer, amountRead);
			sprintf((char *)buffer, "    line %ld [6]", 1 + (offset/6));
			checkIt(whatWeGot, (char *)buffer);
		} else if (offset == 48) {
			sprintf(whatWeGot, "    %s [%ld]", buffer, amountRead);
			sprintf((char *)buffer, "    line#%ld [6]", 1 + (offset/6));
			checkIt(whatWeGot, (char *)buffer);
		} else {
			sprintf(whatWeGot, "    %s [%ld]", buffer, amountRead);
			checkIt(whatWeGot, "    line 10 ABCDEFGHIJKLMNOPQRSTUVWXYZ [34]");
		}
	} /* for */

	if (offset != 88) {
		display(stderr, "    ### not all indirect data was read!\n");
		++failures;
	} else
		++successes;


	/*---------------------------------------------------*
	 | Miscellaneous tests of the indirect file handlers |
	 *---------------------------------------------------*/

	fileSize = CMGetValueSize(v21);
	sprintf(whatWeGot, "    fileSize = %ld", fileSize);
	checkIt(whatWeGot, "    fileSize = 88");


	#if SUBVALUES
	/*---------------------------------------------------*
	 | Sadistic test (part 2) - read of crypt/file value |
	 *---------------------------------------------------*/

	if ((o2 = CMGetNextObjectWithProperty(container, NULL, p3)) == NULL) {
		display(stderr, "### cannot find pointer property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;

	#if SUBVALUES
	v31 = CMUseValue(o2, p3, TestType("t31"));
	v32 = CMUseValue(o2, p3, TestType("t32"));
	v33 = CMUseValue(o2, p3, TestType("t33"));
	#endif

	amountRead = CMReadValueData(v31, (CMPtr)buffer, 0, 2550);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    line 5 [6]");

	amountRead = CMReadValueData(v32, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    line 8 [6]");

	amountRead = CMReadValueData(v33, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    ABCDEFGHIJKLMNOPQRSTUVWXYZ [26]");
	#endif


	/*---------------------*
	 | Close the data file |
	 *---------------------*/

	CMReleaseValue(v21);

	#if SUBVALUES
	CMReleaseValue(v31);
	CMReleaseValue(v32);
	CMReleaseValue(v33);
	#endif

	CMCloseContainer(container);

	CMEndSession(session, false);
}


/*------*
 | main |
 *------*/

void CM_C main(int argc, char *argv[])
{
	#if THINK_C || THINK_CPLUS
	console_options.nrows = 70;
	console_options.ncols = 93;
	argc = ccommand(&argv);
	#endif

	processOptions(argc, argv);

	doTests();

	if (failures)
		display(stderr, "\n### successful tests passed: %ld\n"
											"    failed tests:            %ld\n\n", successes, failures);
	else
		display(stderr, "\n### All tests passed!  Ship it!\n\n");

	exit(EXIT_SUCCESS);
}

															CM_END_CFUNCTIONS

