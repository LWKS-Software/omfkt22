/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                           <<<   SubVTest.c    >>>                         |
 |                                                                           |
 |               Container Manager Dynamic Value SubValue Tests              |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  3/13/92                                  |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file is only used to test subvalues using dynamic values.  Dynamic values are given
 their meaning through their handlers.  Handlers are generally user-written and not
 considered strictly part of the Container Manager.  So their tests are kepts separate
 (besides it's easier to debug them this way).
 
 See ExampleSubValueHandlers.[ch] for full documentation on subvalues.
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
																	
																	CM_CFUNCTIONS

#ifndef __TYPES__
enum {false,true};
typedef unsigned char Boolean;
#endif


static FILE 		*dbgFile = NULL;
static FILE			*containerHandlerDbgFile = NULL;
static int 			failures = 0, successes = 0;
static char 		whatWeGot[256], origFilename[256], filename[256] = {0};
static Boolean 	traceContainerHandlers = false,
								dontClose = false,
								quiet = true;


/* The following is a cheap and dirty way to register type names used in the various		*/
/* tests.  Each group of tests placed into a single property must be unique.  An 				*/
/* alternate scheme would be to use the same type by different properties.  Due to the	*/
/* historical (or hysterical) way the API evolved, changing the types is more 					*/
/* convenient.																																					*/

#define TestType(t) (CMRegisterType(container, (CMGlobalName)t))
	

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
	
	#define CmdLineOptionSyntax "[containerFile] [-t stderr|stdout] [-p] [-r] [-x stderr|stdout]"
	
	for (i = 1; i < argc; ++i) {
		arg = argv[i];
		if (*arg == '-') 
			switch (c = *++arg) {
				case '?':
				case 'h':	display(stderr, "\n%s %s\n\n"
																	"   containerFile     Name of container file.  If omitted, the file\n"
																	"                     \"SubVCont\" is used.\n\n", argv[0], CmdLineOptionSyntax);
									display(stderr, "   -t stderr|stdout  Send handler debugging output and input TOC display\n"
																	"                     to the specified file.  If omitted, no debugging\n"
																	"                     output is generated.\n\n");
									display(stderr, "   -p                Display test output to stderr.  The defualt is to run\n"
																	"                     quietly and self-check the results.  A summary is given\n"
																	"                     at the end of the program.\n\n");
									display(stderr, "   -r                The default is to treat each test separately so that the\n"
																	"                     container written to is closed and then reopened for\n"
																	"                     reading it.  This option suppresses that. The container\n"
																	"                     is opened, written, read, and then closed with -r.\n\n");
									display(stderr, "   -x stderr|stdout  Trace handlers to the specified file.\n");
																												 
									exit(EXIT_SUCCESS);
								
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
		else {
			display(stderr, "### Usage: %s %s\n"
											"###        More than one filename specified\n", argv[0], CmdLineOptionSyntax);
			exit(EXIT_FAILURE);
		}
	}
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
	CMObject			 o1, o2, o3, o4, o5, o6, o7;
	CMProperty		 p1, p2, p3, p4, p5, p6;
	CMType				 t0, t1, t2;
	CMValue				 v11, v12, v13, v14,
								 v21, v22, v23, v24, v25, v26, v27, v28,
								 v31, v32, v33, v34,
								 v41, v42, v43,
								 v61, v62,
								 v71, v72;
	CMSize				 amountRead;
	CMSession			 session;
	unsigned char  buffer[256];

	if (*filename == 0) strcpy( (char*) filename, "SubVCont"         );
	strcpy( (char*) origFilename, filename);
	
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
	
	p1 = CMRegisterProperty(container, "property #1");
	p2 = CMRegisterProperty(container, "property #2");
	p3 = CMRegisterProperty(container, "Subvalue dynamic value tests");
	p4 = CMRegisterProperty(container, "move tests (from)");
	p5 = CMRegisterProperty(container, "move tests (to)");
	
	t1 = CMRegisterType(container, "type #1");
	t2 = CMRegisterType(container, "The Subvalue Descriptor Type");
	

	/*------------------------------------------------------*
	 | Create some data for which we will take the subvalue |
	 *------------------------------------------------------*/
	
	o1  = CMNewObject(container);
	v11 = CMNewValue(o1, p1, TestType("t11"));	/* immediate data													*/
	CMWriteValueData(v11, (CMPtr)"Ira", 0, 3);

	v12 = CMNewValue(o1, p1, TestType("t12"));	/* nonimmediate data						 					*/
	CMWriteValueData(v12, (CMPtr)"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 26);
	
	v13 = CMNewValue(o1, p1, TestType("t13"));	/* continued value 												*/
	v14 = CMNewValue(o1, p1, TestType("t14"));
	CMWriteValueData(v13, (CMPtr)"1234567890", 0, 10);
		CMWriteValueData(v14, (CMPtr)"abcdefghij", 0, 10);
	CMWriteValueData(v13, (CMPtr)"ABCDEFGHIJ", 10, 10);
		CMWriteValueData(v14, (CMPtr)"1234567890", 10, 10);
	CMWriteValueData(v13, (CMPtr)"abcdefghij", 20, 10);
		CMWriteValueData(v14, (CMPtr)"ABCDEFGHIJ", 20, 10);
	

	/*---------------------*
	 | Subvalue read tests |
	 *---------------------*/

	o2 = CMNewObject(container);
	v21 = newSubValue(o2, p3, TestType("t21"), v11,  1,  1);/* subvalue(immediate) 				*/
	v22 = newSubValue(o2, p3, TestType("t22"), v12, 10, 10);/* subvalue(noncont'd nonimm)	*/
	v23 = newSubValue(o2, p3, TestType("t23"), v13,  6, 18);/* subvalue(continued) 				*/
	v24 = newSubValue(o2, p3, TestType("t24"), v13, 16,  8);/* subvalue(continued) 				*/
	v25 = newSubValue(o2, p3, TestType("t25"), v13, 23,  4);/* subvalue(continued) 				*/
	v26 = newSubValue(o2, p3, TestType("t26"), v23,  7,  9);/* subvalue(subvalue)					*/
	v27 = newSubValue(o2, p3, TestType("t27"), v26,  7,100);/* subvalue(subvalue(subvalu))*/
	
	/*----------------------*
	 | Subvalue write tests |
	 *----------------------*/
																													/*      01234567890123456789				*/
	v28 = newSubValue(o2, p3, TestType("t28"), v14, 5, 20);	/* abcdefghij1234567890ABCDEFGHIJ		*/
	CMWriteValueData(v28, (CMPtr)" Testing Write ", 2, 15);	/*      fg Testing Write CDE				*/
	CMDeleteValueData(v28, 10, 100);												/*      fg Testing									*/
	CMInsertValueData(v28, (CMPtr)"Write ", 3, 6);					/*      fg Write Testing				   	*/	


	/*------------------------------*
	 | Subvalue CMMoveValue() tests |
	 *------------------------------*/
	
	o3 = CMNewObject(container);

	v31 = CMNewValue(o3, p4, TestType("t31"));				/* immediate value 									*/
	CMWriteValueData(v31, (CMPtr)"dcba", 0, 4);						
	
	v32 = CMNewValue(o3, p4, TestType("t32"));				/* non-immediate value 							*/
	CMWriteValueData(v32, (CMPtr)"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 26);

	v33 = CMNewValue(o3, p4, TestType("t33"));				/* continued value 									*/
	v34 = CMNewValue(o3, p4, TestType("t34"));
	CMWriteValueData(v33, (CMPtr)"1234567890", 0, 10);
		CMWriteValueData(v34, (CMPtr)"abcdefghij", 0, 10);
	CMWriteValueData(v33, (CMPtr)"ABCDEFGHIJ", 10, 10);
		CMWriteValueData(v34, (CMPtr)"1234567890", 10, 10);
	CMWriteValueData(v33, (CMPtr)"abcdefghij", 20, 10);
		CMWriteValueData(v34, (CMPtr)"ABCDEFGHIJ", 20, 10);
	
	o4 = CMNewObject(container);
	v41 = newSubValue(o4, p3, TestType("t41"), v31,  2,  2);/* subvalue(immediate) 				*/
	v42 = newSubValue(o4, p3, TestType("t42"), v32, 10, 10);/* subvalue(noncont'd nonimm)	*/
	v43 = newSubValue(o4, p3, TestType("t43"), v33,  6, 18);/* subvalue(continued) 				*/

	o5 = CMNewObject(container);
	CMMoveValue(v41, o5, p5);													/* move of immediate subvalue				*/
	CMMoveValue(v42, o5, p5);													/* move or non-immediate subvalue		*/
	CMMoveValue(v43, o5, p5);													/* move of continued subvalue			  */


	/*--------------------------------*
	 | Subvalue CMDeleteValue() tests |
	 *--------------------------------*/

	o6 = CMNewObject(container);
	p6 = CMRegisterProperty(container, "CMDeleteValue() tests");

	v61 = CMNewValue(o6, p6, TestType("t61"));				/* immediate value 									*/
	CMWriteValueData(v61, (CMPtr)"abcd", 0, 4);						
	
	v62 = CMNewValue(o6, p6, TestType("t62"));				/* non-immediate value 							*/
	CMWriteValueData(v62, (CMPtr)"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 26);
	
	o7 = CMNewObject(container);
	v71 = newSubValue(o7, p3, TestType("t71"), v61,  1,  2);/* subvalue(immediate) 				*/
	v72 = newSubValue(o7, p3, TestType("t72"), v62, 10, 10);/* subvalue(noncont'd nonimm)	*/

	CMDeleteValue(v71);																/* only v72 should be on o7					*/
																										/* v71 doesn't need another release	*/
																										
	if (dbgFile) CMDumpTOCStructures(container, dbgFile);
	
	CMReleaseValue(v21);
	CMReleaseValue(v22);
	CMReleaseValue(v23);
	CMReleaseValue(v24);
	CMReleaseValue(v25);
	CMReleaseValue(v26);
	CMReleaseValue(v27);
	CMReleaseValue(v28);
	CMReleaseValue(v41);
	CMReleaseValue(v42);
	CMReleaseValue(v43);
	CMReleaseValue(v72);
	
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
	p1 = CMRegisterProperty(container, "property #1");
	p2 = CMRegisterProperty(container, "property #2");
	p3 = CMRegisterProperty(container, "Subvalue dynamic value tests");
	p4 = CMRegisterProperty(container, "move tests (from)");
	p5 = CMRegisterProperty(container, "move tests (to)");
	p6 = CMRegisterProperty(container, "CMDeleteValue() tests");
	
	t1 = CMRegisterType(container, "type #1");
	t2 = CMRegisterType(container, "The Subvalue Descriptor Type");
	
	if (!quiet) 
		display(stderr, "\nRead tests of subvalues:\n"
										"    012345678901234567890123456789012345678901234567890\n");
	if ((o2 = CMGetNextObjectWithProperty(container, NULL, p3)) == NULL) {
		display(stderr, "### cannot find property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	
	v21 = CMUseValue(o2, p3, TestType("t21"));
	v22 = CMUseValue(o2, p3, TestType("t22"));
	v23 = CMUseValue(o2, p3, TestType("t23"));
	v24 = CMUseValue(o2, p3, TestType("t24"));
	v25 = CMUseValue(o2, p3, TestType("t25"));
	v26 = CMUseValue(o2, p3, TestType("t26"));
	v27 = CMUseValue(o2, p3, TestType("t27"));
	v28 = CMUseValue(o2, p3, TestType("t28"));
	
	amountRead = CMReadValueData(v21, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v21 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v21 = r [1]");
	
	CMGetValueInfo(v21, NULL, NULL, NULL, &t0, NULL);
	if (t0 != TestType("t21")) {
		display(stderr, "    ### got wrong subvalue type during CMReadValueData() tests\n");
		++failures;
	} else
		++successes;
	
	amountRead = CMReadValueData(v22, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld] (subvalue of non-continued non-immediate)", buffer, amountRead);
	checkIt(whatWeGot, "    KLMNOPQRST [10] (subvalue of non-continued non-immediate)");
	
	amountRead = CMReadValueData(v23, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld] (subvalue of continued non-immediate)", buffer, amountRead);
	checkIt(whatWeGot, "    7890ABCDEFGHIJabcd [18] (subvalue of continued non-immediate)");

	amountRead = CMReadValueData(v24, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld] (subvalue of continued non-immediate)", buffer, amountRead);
	checkIt(whatWeGot, "    GHIJabcd [8] (subvalue of continued non-immediate)");

	amountRead = CMReadValueData(v25, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld] (subvalue of continued non-immediate)", buffer, amountRead);
	checkIt(whatWeGot, "    defg [4] (subvalue of continued non-immediate)");
	
	amountRead = CMReadValueData(v26, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld] (subvalue of a subvalue)", buffer, amountRead);
	checkIt(whatWeGot, "    DEFGHIJab [9] (subvalue of a subvalue)");
	
	amountRead = CMReadValueData(v27, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld] (subvalue of a subvalue of a subvalue)", buffer, amountRead);
	checkIt(whatWeGot, "    ab [2] (subvalue of a subvalue of a subvalue)");	
	
	amountRead = CMReadValueData(v28, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld] (write/delete/insert)", buffer, amountRead);
	checkIt(whatWeGot, "    fg Write Testing [16] (write/delete/insert)");
	
	/*-----------------------------------------------------------------------------------*/
	
	if (!quiet) 
		display(stderr, "\nRead tests of moved subvalues:\n"
										"    012345678901234567890123456789012345678901234567890\n");
	if ((o5 = CMGetNextObjectWithProperty(container, NULL, p5)) == NULL) {
		display(stderr, "### cannot find \"move tests\" property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	
	v41 = CMUseValue(o5, p5, TestType("t41"));
	v42 = CMUseValue(o5, p5, TestType("t42"));
	v43 = CMUseValue(o5, p5, TestType("t43"));
	/*v44 = CMUseValue(o5, p5, 4);*/
	
	amountRead = CMReadValueData(v41, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld] (move of immediate subvalue)", buffer, amountRead);
	checkIt(whatWeGot, "    ba [2] (move of immediate subvalue)");

	CMGetValueInfo(v41, NULL, NULL, NULL, &t0, NULL);
	if (t0 != TestType("t41")) {
		display(stderr, "    ### got wrong subvalue type from moved subvalue\n");
		++failures;
	} else
		++successes;

	amountRead = CMReadValueData(v42, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld] (move of non-immediate subvalue)", buffer, amountRead);
	checkIt(whatWeGot, "    KLMNOPQRST [10] (move of non-immediate subvalue)");
	
	amountRead = CMReadValueData(v43, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld] (move of subvalue of continued non-immediate)", buffer, amountRead);
	checkIt(whatWeGot, "    7890ABCDEFGHIJabcd [18] (move of subvalue of continued non-immediate)");
	
	#if 0
	amountRead = CMReadValueData(v44, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    %s [%ld] (move of a subvalue of a subvalue)", buffer, amountRead);
	checkIt(whatWeGot, "    DEFGHIJab [9] (move of a subvalue of a subvalue)");
	#endif
	
	CMReleaseValue(v21);
	CMReleaseValue(v22);
	CMReleaseValue(v23);
	CMReleaseValue(v24);
	CMReleaseValue(v25);
	CMReleaseValue(v26);
	CMReleaseValue(v27);
	CMReleaseValue(v28);
	CMReleaseValue(v41);
	CMReleaseValue(v42);
	CMReleaseValue(v43);
	/*CMReleaseValue(v44);*/
	
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

