/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                          <<<   UpdtTest.c    >>>                          |
 |                                                                           |
 |                 Container Manager Updating Container Tests                |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  6/3/92                                   |
 |                                                                           |
 |                     Copyright Apple Computer, Inc. 1992                   |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This file is only used to test and debug the updating of containers.  A piece of 
 MainTest.c  is used here as a convenient way to generate a target container to apply
 updates to.
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
#include "THandlrs.h"               
#include "XIndFile.h"              
#include "XSession.h"              
																	
																	CM_CFUNCTIONS

#ifndef __TYPES__
enum {false,true};
typedef unsigned char Boolean;
#endif

CMSession			 	session;
static FILE 		*dbgFile = NULL;
static FILE			*handlerDbgFile = NULL;
static FILE			*rawDataFile;
static short 		maxValueNbr = 0;
static int 			failures = 0, successes = 0;
static char 		whatWeGot[256], origFilename[256], targetFilename[256] = {0}, updatingFilename[256] = {0};
static Boolean 	traceHandlers  = false,
								reuseTestOnly  = false,
								ubaTestOnly 	 = false,
								dvTestsOnly 	 = false,
								updateRefsOnly = false,
								dontClose 		 = false,
								debugging			 = false,
								quiet 				 = true;
static Boolean	case1 = false, case2 = false, case3 = false;


/* The following is a cheap and dirty way to register type names used in the various		*/
/* tests.  Each group of tests placed into a single property must be unique.  An 				*/
/* alternate scheme would be to use the same type by different properties.  Due to the	*/
/* historical (or hysterical) way the API evolved, changing the types is more 					*/
/* convenient.																																					*/

#define TestType(t) (CMRegisterType(container, (CMGlobalName)(t)))


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
	
	#define CmdLineOptionSyntax "[targetFile] [updatingFile] [-t stderr|stdout] [-p] [-r] [-x stderr|stdout] [-n] [-s] [-a] [-d] [-R] [-z]"
	
	for (i = 1; i < argc; ++i) {
		arg = argv[i];
		if (*arg == '-') 
			switch (c = *++arg) {
				case '?':
				case 'h':	display(stderr, "\n%s %s\n\n"
																	"   targetFile        Name of target container file.  If omitted, the file\n"
																	"                     \"TargCont\" is used.\n\n", argv[0], CmdLineOptionSyntax);
									display(stderr, "   updatingFile      Name of separate container file to recieve recorded updates.\n"
																	"                     This is required only if doing the \"separate container\"\n"
																	"                     tests (-s or -u, -a, and -s omitted).  If omitted, the file\n"
																	"                     \"Upd\" is used.\n\n");
									display(stderr, "   -s                Do space reuse tests only.\n\n");
									display(stderr, "   -a                Do update-by-append tests only.\n\n");
									display(stderr, "   -d                Do update separate container tests only.\n\n");
									display(stderr, "   -R                Do reference update tests only (case sensitive option -- sorry).\n\n");
									display(stderr, "   -p                Display test output to stderr.  The defualt is to run\n"
																	"                     quietly and self-check the results.  A summary is given\n"
																	"                     at the end of the program.\n\n");
									display(stderr, "   -r                The default is to treat each test separately so that the\n"
																	"                     container written to is closed and then reopened for\n"
																	"                     reading it.  This option suppresses that. The container\n"
																	"                     is opened, written, read, and then closed with -r.\n\n");
									display(stderr, "   -t stderr|stdout  Send handler debugging output and input TOC display\n"
																	"                     to the specified file.  If omitted, no debugging\n"
																	"                     output is generated.\n\n");
									display(stderr, "   -x stderr|stdout  Trace handlers to the specified file.\n\n");
									display(stderr, "   -z                For debugging only.\n\n");
																												 
									exit(EXIT_SUCCESS);
									
				case 's':	reuseTestOnly = true;
									break;
				
				case 'a':	ubaTestOnly = true;
									break;
				
				case 'd': dvTestsOnly = true;
									break;
					
				case 'R': updateRefsOnly = true;
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
										traceHandlers = true;
										handlerDbgFile = f;
									} else
										dbgFile = f;
									break; 
		
				case 'z':	debugging = true;
									break;
				
				default:	display(stderr, "### Usage: %s %s\n"
																	"###        Invalid option: \"%s\"\n", argv[0], CmdLineOptionSyntax, arg-1);
									exit(EXIT_FAILURE);
			}
		else if (*targetFilename == 0)
			strcpy(targetFilename, arg);
		else if (*updatingFilename == 0)
			strcpy(updatingFilename, arg);
		else {
			display(stderr, "### Usage: %s %s\n"
											"###        Too many filenames specified\n", argv[0], CmdLineOptionSyntax);
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


/*-------------------------------*
 | createContainerWithReferences |
 *-------------------------------*
 
 This is basically the container creation portion of doReferenceTests() proc from 
 MainTests.c.  It's good enough for our purposes here.  This should be called only once.
 It creats a "flock" of references in a target container that are updated.  This is a
 separate test to make sure updates of references work correctly.
*/

static void CM_NEAR createContainerWithReferences(char *filename)
{
	CMContainer 	 container;
	CMRefCon			 myRefCon;
	CMType				 t1, t2;
	CMProperty		 p1, p2;
	CMObject			 testO, o[5];
	CMValue				 testV, v[5];
	CMReference		 refData[5];
	short					 i;
	unsigned char  buffer[256];
	 	
	myRefCon = createRefConForMyHandlers(session, filename, NULL);
	
	CMSetMetaHandler(session, "DebuggingRefs1", containerMetahandler);
	container = CMOpenNewContainer(session, myRefCon, "DebuggingRefs1", 0, 1, 0);
	if (container == NULL) {
		display(stderr, "### reference creation terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
		
	p1 = CMRegisterProperty(container, "target referenced property");
	t1 = CMRegisterType(container, "target referenced type");
	
	p2 = CMRegisterProperty(container, "referencing property");
	t2 = CMRegisterType(container, "referencing type");
	
	for (i = 0; i < 5; ++i) {												/* create 5 objects with 1 value each	*/
		o[i] = CMNewObject(container);								/* o[i] 															*/
		v[i] = CMNewValue(o[i], p1, t1);							/* o[i] contains value v[i]						*/
		sprintf((char *)buffer, "v[%d]", i);					/* value data for v[i] is "v[i]"			*/
		CMWriteValueData(v[i], (CMPtr)buffer, 0, 4);
	}
	
	/* Create the referencing object which will contain the references to the 7 objects...*/
	
	testO = CMNewObject(container);									/* object "owning" the references"		*/
	testV = CMNewValue(testO, p2, t2);							/* object's value to get refs as data	*/
		
	for (i = 0; i < 5; ++i)													/* create refs to 5 target objects		*/
		memcpy(refData[i], CMNewReference(testV, o[i], refData[i]), sizeof(CMReference));
		
	CMWriteValueData(testV, (CMPtr)"<<<<", 0, 4);
	CMWriteValueData(testV, (CMPtr)refData, 4, 5 * sizeof(CMReference));
	
	CMCloseContainer(container);
}


/*-----------------*
 | createContainer |
 *-----------------*
 
 This is basically the container creation portion of doBasicTests() proc from MainTests.c.
 It's good enough for our purposes here.  But additions have been made to test specific
 things.  This should be called only once.
*/

static void CM_NEAR createContainer(char *filename)
{
	CMContainer 	 container;
	CMRefCon			 myRefCon;
	CMType				 t1, t2, t3, base1, base2;
	CMProperty		 p1, p2, p3, p4, p5, p5a, p6, p7a, p7b;
	CMObject			 o1, o3, o4, o5, o6, o7;
	CMValue				 v1, v2, v3, v4, v5, v6, v7, v8, v9a, v9b, v10a, v10b, v11, v12, v13,
								 v17a, v17b, v17c, v18, v19a, v19b, v20a, v20b, v21a, 
								 v21b, v22a, v22b, v22c, v22d, v23, v24, v25a, v25b,
								 v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40,
								 v41, v50;
	
	myRefCon = createRefConForMyHandlers(session, filename, NULL);
	
	CMSetMetaHandler(session, "Debugging1", containerMetahandler);
	container = CMOpenNewContainer(session, myRefCon, "Debugging1", 0, 1, 0);
	
	if (container == NULL) {
		display(stderr, "### writing target container terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	p1 = CMRegisterProperty(container, "property #1");
	p2 = CMRegisterProperty(container, "property #2");
	p3 = CMRegisterProperty(container, "value tests");
	p4 = CMRegisterProperty(container, "move update tests");
	p5 = CMRegisterProperty(container, "insert tests");
	p5a= CMRegisterProperty(container, "insert tests2");
	p6 = CMRegisterProperty(container, "delete tests");
	p7a= CMRegisterProperty(container, "move tests (from)");
	p7b= CMRegisterProperty(container, "move tests (to)");
	
	t1 = CMRegisterType(container, "type #1");
	t2 = CMRegisterType(container, "type #2");
	t3 = CMRegisterType(container, "type #3");
	
	
	/*--------------------------*
	 | CMWriteValueData() tests |
	 *--------------------------*/

	o1 = CMNewObject(container);							/* test 1: immediate data	*/
	v1 = CMNewValue(o1, p3, TestType("t1"));
	CMSetValueType(v1, t1);										/* through in this test in as well */
	CMSetValueGeneration(v1, 2);
	CMWriteValueData(v1, (CMPtr)"123", 0, 3);
	
	v2 = CMNewValue(o1, p3, TestType("t2"));	/* test 2: nonimmediate data */
	CMWriteValueData(v2, (CMPtr)"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 26);
	
	v3 = CMNewValue(o1, p3, TestType("t3"));	/* test 3: pure overwrite of immediate */
	CMWriteValueData(v3, (CMPtr)"ABC", 0, 3);
	CMWriteValueData(v3, (CMPtr)"xy", 2, 2);
	
	v4 = CMNewValue(o1, p3, TestType("t4"));	/* test 4: pure overwrite of nonimmediate */
	CMWriteValueData(v4, (CMPtr)"123456789012345678901234567890", 0, 30);
	CMWriteValueData(v4, (CMPtr)"ABCDEFGHIJ", 10, 10);
	
	v5 = CMNewValue(o1, p3, TestType("t5"));	/* test 5: concat to immediate */
	CMWriteValueData(v5, (CMPtr)"ab", 0, 2);
	CMWriteValueData(v5, (CMPtr)"CD", 2, 2);
	
	v6 = CMNewValue(o1, p3, TestType("t6"));	/* test 6: concat to non-immediate */
	CMWriteValueData(v6, (CMPtr)"1234567890", 0, 10);
	CMWriteValueData(v6, (CMPtr)"ABCDE", 10, 5);
	
	v7 = CMNewValue(o1, p3, TestType("t7"));	/* test 7: extend immediate to nonimmediate */
	CMWriteValueData(v7, (CMPtr)"1234", 0, 4);
	CMWriteValueData(v7, (CMPtr)"ABCDE", 3, 5);

	v8 = CMNewValue(o1, p3, TestType("t8"));	/* test 8: extend nonimmediate */
	CMWriteValueData(v8, (CMPtr)"1234567890", 0, 10);
	CMWriteValueData(v8, (CMPtr)"****Test8****", 2, 13);
	
	v9a = CMNewValue(o1, p3, TestType("t9a"));/* test 9: continued value */
	v9b = CMNewValue(o1, p3, TestType("t9b"));
	CMWriteValueData(v9a, (CMPtr)"1234567890", 0, 10);
		CMWriteValueData(v9b, (CMPtr)"abcdefghij", 0, 10);
	CMWriteValueData(v9a, (CMPtr)"ABCDEFGHIJ", 10, 10);
		CMWriteValueData(v9b, (CMPtr)"1234567890", 10, 10);
	CMWriteValueData(v9a, (CMPtr)"abcdefghij", 20, 10);
		CMWriteValueData(v9b, (CMPtr)"ABCDEFGHIJ", 20, 10);
	CMWriteValueData(v9a, (CMPtr)"segment 4a", 30, 10);
		CMWriteValueData(v9b, (CMPtr)"segment 4b", 30, 10);
	CMWriteValueData(v9a, (CMPtr)"segment 5a", 40, 10);
		CMWriteValueData(v9b, (CMPtr)"segment 5b", 40, 10);
	CMWriteValueData(v9a, (CMPtr)"segment 6a", 50, 10);
		CMWriteValueData(v9b, (CMPtr)"segment 6b", 50, 10);
	
	v10a = CMNewValue(o1, p3, TestType("t10a"));/* test 10: overwrite of continued value */
	v10b = CMNewValue(o1, p3, TestType("t10b"));
	CMWriteValueData(v10a, (CMPtr)"ABCDE", 0, 5);
	  CMWriteValueData(v10b, (CMPtr)"abcde", 0, 5);
	CMWriteValueData(v10a, (CMPtr)"FGHIJ", 5, 5);
	  CMWriteValueData(v10b, (CMPtr)"fghij", 5, 5);
	CMWriteValueData(v10a, (CMPtr)"KLMNO", 10, 5);
	  CMWriteValueData(v10b, (CMPtr)"klmno", 10, 5);
	CMWriteValueData(v10a, (CMPtr)"PQRST", 15, 5);
	  CMWriteValueData(v10b, (CMPtr)"pqrst", 15, 5);
	
	CMWriteValueData(v10a, (CMPtr)"###TEST10###", 7, 12);
	CMWriteValueData(v10b, (CMPtr)"***test10***", 13, 12);
	
	v11 = CMNewValue(o1, p3, TestType("t11"));/* test 11: (over)write of data with size 0 */
	CMWriteValueData(v11, (CMPtr)"Size=0", 0, 0);
	CMWriteValueData(v11, (CMPtr)"12345", 0, 0); 
	
	v12 = CMNewValue(o1, p3, TestType("t12"));				
	CMWriteValueData(v12, (CMPtr)"test", 0, 4);
	CMWriteValueData(v12, (CMPtr)"12345", 2, 0); 

	v13 = CMNewValue(o1, p3, TestType("t13"));
	CMWriteValueData(v13, (CMPtr)"Ira Ruben", 0, 9);
	CMWriteValueData(v13, (CMPtr)"12345", 2, 0);


	/*---------------------------*
	 | CMInsertValueData() Tests |
	 *---------------------------*/
	
	o3 = CMNewObject(container);
	
	v17a = CMNewValue(o3, p5, TestType("t17a"));
	CMWriteValueData(v17a, (CMPtr)"wz", 0, 2);
	v17b = CMNewValue(o3, p5, TestType("t17b"));
	CMWriteValueData(v17b, (CMPtr)"ABGH", 0, 4);
	
	v18 = CMNewValue(o3, p5, TestType("t18"));
	CMWriteValueData(v18, (CMPtr)"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 26);
	
	v19a = CMNewValue(o3, p5, TestType("t19a"));
	v19b = CMNewValue(o3, p5, TestType("t19b"));
	CMWriteValueData(v19a, (CMPtr)"1234567890", 0, 10);
		CMWriteValueData(v19b, (CMPtr)"abcdefghij", 0, 10);
	CMWriteValueData(v19a, (CMPtr)"ABCDEFGHIJ", 10, 10);
		CMWriteValueData(v19b, (CMPtr)"1234567890", 10, 10);
	CMWriteValueData(v19a, (CMPtr)"abcdefghij", 20, 10);
		CMWriteValueData(v19b, (CMPtr)"ABCDEFGHIJ", 20, 10);

	CMInsertValueData(v17a, (CMPtr)"xy", 1, 2);					/* test 1: small insert into small immediate */
	CMInsertValueData(v17b, (CMPtr)"CDEF", 2, 4);				/* test 2: large insert into large immediate */
	CMInsertValueData(v18,  (CMPtr)"1234567890", 9, 10);/* test 3: insert into non-immediate */
	CMInsertValueData(v19a, (CMPtr)"<insert>", 15, 8);	/* test 4: insert into middle of cont'd seg */
	CMInsertValueData(v19b, (CMPtr)"<insert>", 20, 8);	/* test 5: insert into start of cont'd seg */

	v17c = CMNewValue(o3, p5a, TestType("t17c"));				/* just need a 2nd property for o3 */
	CMWriteValueData(v17c, (CMPtr)"", 0, 0);

	/*---------------------------*
	 | CMDeleteValueData() Tests |
	 *---------------------------*/

	o4 = CMNewObject(container);
	
	v20a = CMNewValue(o4, p6, TestType("t20a"));
	CMWriteValueData(v20a, (CMPtr)"ABCD", 0, 4);
	v20b = CMNewValue(o4, p6, TestType("t20b"));
	CMWriteValueData(v20b, (CMPtr)"abcd", 0, 4);
	
	v21a = CMNewValue(o4, p6, TestType("t21a"));
	CMWriteValueData(v21a, (CMPtr)"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 26);
	v21b = CMNewValue(o4, p6, TestType("t21b"));
	CMWriteValueData(v21b, (CMPtr)"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 26);
	
	v22a = CMNewValue(o4, p6, TestType("t22a"));
	v22b = CMNewValue(o4, p6, TestType("t22b"));
	v22c = CMNewValue(o4, p6, TestType("t22c"));
	v22d = CMNewValue(o4, p6, TestType("t22d"));
	CMWriteValueData(v22a, (CMPtr)"1234567890", 0, 10);
		CMWriteValueData(v22b, (CMPtr)"abcdefghij", 0, 10);
			CMWriteValueData(v22c, (CMPtr)"1234567890", 0, 10);
				CMWriteValueData(v22d, (CMPtr)"1234567890", 0, 10);
	CMWriteValueData(v22a, (CMPtr)"ABCDEFGHIJ", 10, 10);
		CMWriteValueData(v22b, (CMPtr)"1234567890", 10, 10);
			CMWriteValueData(v22c, (CMPtr)"ABCDEFGHIJ", 10, 10);
				CMWriteValueData(v22d, (CMPtr)"ABCDEFGHIJ", 10, 10);
	CMWriteValueData(v22a, (CMPtr)"abcdefghij", 20, 10);
		CMWriteValueData(v22b, (CMPtr)"ABCDEFGHIJ", 20, 10);
			CMWriteValueData(v22c, (CMPtr)"abcdefghij", 20, 10);
				CMWriteValueData(v22d, (CMPtr)"abcdefghij", 20, 10);

	CMDeleteValueData(v20a, 1, 2);									/* test 1: delete of immediate */
	CMDeleteValueData(v20b, 0, 4);									/* test 2: delete of entire immediate */
	CMDeleteValueData(v21a, 10, 9);									/* test 3: delete of non-immediate */
	CMDeleteValueData(v21b, 0, 100);								/* test 4: delete of entire non-immediate */
	CMDeleteValueData(v22a, 5, 19);									/* test 5: delete of cont'd */
	CMDeleteValueData(v22b, 23, 4);									/* test 6: delete of cont'd - part of last seg */
	CMDeleteValueData(v22c, 5, 100);								/* test 6: delete of cont'd - seg to end */
	CMDeleteValueData(v22d, 15, 100);								/* test 7: delete of cont'd - middle seg to end */
	

	/*---------------------*
	 | CMMoveValue() Tests |
	 *---------------------*/
	
	o5 = CMNewObject(container);

	v23 = CMNewValue(o5, p7a, TestType("t23"));			/* immediate value */
	CMWriteValueData(v23, (CMPtr)"dcba", 0, 4);						
	
	v24 = CMNewValue(o5, p7a, TestType("t24"));			/* non-immediate value */
	CMWriteValueData(v24, (CMPtr)"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 26);

	v25a = CMNewValue(o5, p7a, TestType("t25a"));		/* continued value */
	v25b = CMNewValue(o5, p7a, TestType("t25b"));
	CMWriteValueData(v25a, (CMPtr)"1234567890", 0, 10);
		CMWriteValueData(v25b, (CMPtr)"abcdefghij", 0, 10);
	CMWriteValueData(v25a, (CMPtr)"ABCDEFGHIJ", 10, 10);
		CMWriteValueData(v25b, (CMPtr)"1234567890", 10, 10);
	CMWriteValueData(v25a, (CMPtr)"abcdefghij", 20, 10);
		CMWriteValueData(v25b, (CMPtr)"ABCDEFGHIJ", 20, 10);
	
	o6 = CMNewObject(container);						
	CMMoveValue(v23, o6, p7b);											/* test 1: move of immediate */
	CMMoveValue(v24, o6, p7b);											/* test 2: move or non-immediate */
	CMMoveValue(v25a, o6, p7b);											/* test 3: move of cont'd */
	CMMoveValue(v25b, o6, p7b);											/* test 4: move of last cont'd */
	

	/*--------------------------------------------*
	 | Create "old" objects for move update tests |
	 *--------------------------------------------*/
	 
	o7 = CMNewObject(container);
	v26 = CMNewValue(o7, p4, TestType("t26"));
	CMWriteValueData(v26, (CMPtr)"v26", 0, 3);
	v27 = CMNewValue(o7, p4, TestType("t27"));
	CMWriteValueData(v27, (CMPtr)"v27", 0, 3);
	v28 = CMNewValue(o7, p4, TestType("t28"));
	CMWriteValueData(v28, (CMPtr)"v28", 0, 3);
	v29 = CMNewValue(o7, p4, TestType("t29"));
	CMWriteValueData(v29, (CMPtr)"v29", 0, 3);
	v30 = CMNewValue(o7, p4, TestType("t30"));
	CMWriteValueData(v30, (CMPtr)"v30", 0, 3);
	v31 = CMNewValue(o7, p4, TestType("t31"));
	CMWriteValueData(v31, (CMPtr)"v31", 0, 3);
	v32 = CMNewValue(o7, p4, TestType("t32"));
	CMWriteValueData(v32, (CMPtr)"v32", 0, 3);
	v33 = CMNewValue(o7, p4, TestType("t33"));
	CMWriteValueData(v33, (CMPtr)"v33", 0, 3);
	v34 = CMNewValue(o7, p4, TestType("t34"));
	CMWriteValueData(v34, (CMPtr)"v34", 0, 3);
	v35 = CMNewValue(o7, p4, TestType("t35"));
	CMWriteValueData(v35, (CMPtr)"v35", 0, 3);
	v36 = CMNewValue(o7, p4, TestType("t36"));
	CMWriteValueData(v36, (CMPtr)"v36", 0, 3);
 	v37 = CMNewValue(o7, p4, TestType("t37"));
	CMWriteValueData(v37, (CMPtr)"v37", 0, 3);
	v38 = CMNewValue(o7, p4, TestType("t38"));
	CMWriteValueData(v38, (CMPtr)"v38", 0, 3);
	v39 = CMNewValue(o7, p4, TestType("t39"));
	CMWriteValueData(v39, (CMPtr)"v39", 0, 3);
	v40 = CMNewValue(o7, p4, TestType("t40"));
	CMWriteValueData(v40, (CMPtr)"v40", 0, 3);
	v41 = CMNewValue(o7, p4, TestType("t41"));
	CMWriteValueData(v41, (CMPtr)"v41", 0, 3);
	
	v50 = CMNewValue(o1, p3, TestType("t50"));
	CMWriteValueData(v50, (CMPtr)"v50", 0, 3);
	
	
	/*------------------------*
	 | Create some base types |
	 *------------------------*/
	 
	base1 = CMRegisterType(container, "base #1"); /* modify base types */
	base2 = CMRegisterType(container, "base #2");
	CMAddBaseType(t3, base1);
	CMAddBaseType(t3, base2);

	CMCloseContainer(container);
}


/*-------------------*
 | createSomeUpdates |
 *-------------------*
 
 This is called somewhere in the open chain of tests to "putz" around with the data 
 created previously by createContainer().
*/

static void CM_NEAR createSomeUpdates(CMContainer container)
{
	CMObject			o1, o3, o4, o6, o7, o8, o9;
	CMProperty		p1, p2, p3, p4, p5, p5a, p6, p7a, p7b, p8, p9;
	CMType				t1, t2, t3, base1, base21, base22;
	CMValue				v1, v2, v3, v4, v5, v6, v7, v9a, v9b,
								v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40,
								v41, v50;
	
	/* Create some new objects...																													*/
	
	p1 = CMRegisterProperty(container, "property #21");
	t1 = CMRegisterType(container, "type #21");
	o1 = CMNewObject(container);
	v1 = CMNewValue(o1, p1, TestType("t1"));
	CMWriteValueData(v1, (CMPtr)"This belongs to 2nd container", 0, 29);
	v2 = CMNewValue(o1, p1, TestType("t2"));
	CMWriteValueData(v2, (CMPtr)"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 26);
	
	/* Manipulate old objects...																													*/
	
	p1 = CMRegisterProperty(container, "property #1");
	p2 = CMRegisterProperty(container, "property #2");
	p3 = CMRegisterProperty(container, "value tests");
	p4 = CMRegisterProperty(container, "move update tests");
	p5 = CMRegisterProperty(container, "insert tests");
	p5a= CMRegisterProperty(container, "insert tests2");
	p6 = CMRegisterProperty(container, "delete tests");
	p7a= CMRegisterProperty(container, "move tests (from)");
	p7b= CMRegisterProperty(container, "move tests (to)");
	p8 = CMRegisterProperty(container, "new object property #1");
	p9 = CMRegisterProperty(container, "new object property #2");
	
	t1 = CMRegisterType(container, "type #1");
	t2 = CMRegisterType(container, "type #2");
	t3 = CMRegisterType(container, "type #3");
	
	if ((o1 = CMGetNextObjectWithProperty(container, NULL, p3)) == NULL) {
		display(stderr, "### cannot find \"value tests\" property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	if ((o3 = CMGetNextObjectWithProperty(container, NULL, p5)) == NULL) {
		display(stderr, "### cannot find \"insert tests\" property from updating container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	if ((o4 = CMGetNextObjectWithProperty(container, NULL, p6)) == NULL) {
		display(stderr, "### cannot find \"delete tests\" property from updating container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	if ((o6 = CMGetNextObjectWithProperty(container, NULL, p7b)) == NULL) {
		display(stderr, "### cannot find \"move tests\" property from updating container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	if ((o7 = CMGetNextObjectWithProperty(container, NULL, p4)) == NULL) {
		display(stderr, "### cannot find \"move update tests\" property from updating container\n");
		exit(EXIT_FAILURE);
	}
	++successes;

	#if 1
	v1 = CMUseValue(o1, p3, t1);							/* immediate data */
	CMWriteValueData(v1, (CMPtr)"abcd", 0, 4);
	#endif
	
	#if 1
	v2 = CMUseValue(o1, p3, TestType("t2"));	/* data insert */
	CMInsertValueData(v2, "<insert>", 10, 8);
	#endif
	
	#if 1
	v4 = CMUseValue(o1, p3, TestType("t4"));	/* data delete */
	CMDeleteValueData(v4, 10, 10);
	#endif
	
	#if 1
	v6 = CMUseValue(o1, p3, TestType("t6"));	/* delete/insert (overwrite) */
	CMWriteValueData(v6, (CMPtr)"overwrite", 1, 9);
	#endif
	
	#if 1
	v9a = CMUseValue(o1, p3, TestType("t9a"));/* delete the middle of a segment */
	CMDeleteValueData(v9a, 12, 4);
	#endif
	
	#if 1
	v9b = CMUseValue(o1, p3, TestType("t9b"));/* multi-segment delete and insert */
	CMDeleteValueData(v9b, 15, 43);
	CMInsertValueData(v9b, (CMPtr)"<insert>", 15, 8);
	#endif
	
	#if 1
	v50 = CMUseValue(o1, p3, TestType("t50"));/* insert into immediate to make non-immediate */
	CMInsertValueData(v50, (CMPtr)"<insert>", 1, 8);
	#endif
	
	#if 1
	base1  = CMRegisterType(container, "base #1"); /* modify base types */
	base21 = CMRegisterType(container, "base #21"); 
	base22 = CMRegisterType(container, "base #22");
	CMRemoveBaseType(t3, base1);
	CMAddBaseType(t3, base21);
	CMAddBaseType(t3, base22);
	#endif
	
	#if 1
	v3 = CMUseValue(o1, p3, TestType("t3"));	/* set-infoed */
	CMSetValueType(v3, t2);
	CMSetValueGeneration(v3, 2);
	#endif
	
	#if 1
	v5 = CMUseValue(o1, p3, TestType("t5"));	/* deleted values */
	v7 = CMUseValue(o1, p3, TestType("t7"));
	CMDeleteValue(v5);
	CMDeleteValue(v7);
	CMDeleteObjectProperty(o3, p5);						/* delete properties */
	CMDeleteObjectProperty(o3, p5a);
	CMDeleteObject(o4);												/* deleted objects */
	CMDeleteObject(o6);
	#endif
	
	#if 1
	o8 = CMNewObject(container);							/* move tests */
	o9 = CMNewObject(container);
	v26 = CMUseValue(o7, p4, TestType("t26"));
	v27 = CMUseValue(o7, p4, TestType("t27"));
	v28 = CMUseValue(o7, p4, TestType("t28"));
	v29 = CMUseValue(o7, p4, TestType("t29"));
	v30 = CMUseValue(o7, p4, TestType("t30"));
	v31 = CMUseValue(o7, p4, TestType("t31"));
	v32 = CMUseValue(o7, p4, TestType("t32"));
	v33 = CMUseValue(o7, p4, TestType("t33"));
	v34 = CMUseValue(o7, p4, TestType("t34"));
	v35 = CMUseValue(o7, p4, TestType("t35"));
	v36 = CMUseValue(o7, p4, TestType("t36"));
	v37 = CMUseValue(o7, p4, TestType("t37"));
	v38 = CMUseValue(o7, p4, TestType("t38"));
	v39 = CMUseValue(o7, p4, TestType("t39"));
	v40 = CMUseValue(o7, p4, TestType("t40"));
	v41 = CMUseValue(o7, p4, TestType("t41"));

	CMMoveValue(v26, o7, p1);																		/* state 0 */
	CMMoveValue(v27, o8, p8);
	
	CMMoveValue(v28, o7, p1); CMMoveValue(v28, o7, p2);					/* state 1 */
	CMMoveValue(v29, o7, p1); CMMoveValue(v29, o7, p4);
	CMMoveValue(v30, o7, p1); CMMoveValue(v30, o7, p2); CMMoveValue(v30, o7, p4);
	CMMoveValue(v31, o7, p1); CMMoveValue(v31, o8, p8);
	
	CMMoveValue(v32, o8, p8); CMMoveValue(v32, o7, p1);					/* state 2 */
	CMMoveValue(v33, o8, p8); CMMoveValue(v33, o8, p2); 
	CMMoveValue(v34, o8, p8); CMMoveValue(v34, o9, p9); 
	CMMoveValue(v35, o8, p8); CMMoveValue(v35, o8, p1); CMMoveValue(v35, o7, p4);
	CMMoveValue(v36, o8, p8); CMMoveValue(v36, o9, p9); CMMoveValue(v36, o7, p4);
	CMMoveValue(v37, o8, p8); CMMoveValue(v37, o7, p1); CMMoveValue(v37, o9, p9);
	
	CMMoveValue(v38, o7, p1);																		/* state 0/1 + editing */
	CMInsertValueData(v38, (CMPtr)"<insert>", 1, 8);						/* +cvt imm to non-imm */
	CMMoveValue(v38, o7, p2);
	CMMoveValue(v38, o7, p4);
	
	CMMoveValue(v39, o8, p8);																		/* state 2/0 + editing */
	CMInsertValueData(v39, (CMPtr)"<insert>", 1, 8);
	CMMoveValue(v39, o7, p4);
	
	CMMoveValue(v40, o8, p8);																		/* state 2/1 + editing */
	CMInsertValueData(v40, (CMPtr)"<insert>", 1, 8);
	CMMoveValue(v40, o7, p1);
	
	CMMoveValue(v41, o8, p8);																		/* state 2 + editing */
	CMInsertValueData(v41, (CMPtr)"<insert>", 1, 8);
	CMMoveValue(v41, o9, p9);																		/* state 2 + editing */
	CMWriteValueData(v41, (CMPtr)"<append>", 11, 8);
	#endif
}


/*------------------*
 | updateTheUpdates |
 *------------------*
 
 This does further modifications on the stuff modified by createSomeUpdates().  For the
 self verifications to work (see verifyUpdates()), this can be called in the tests at
 least x times as follows:
 
 		updateTheUpdates(container, 1, 0);			
 		updateTheUpdates(container, 2, 0);			
 		updateTheUpdates(container, 3, 0);
		
 The index causes different groups of values to be additionally modified.  By using these
 calls at different "levels" of multi-layered containers, you (I) can see if the everyone
 knows "who's on first" with respect to which container owns what data.
 
 A fourth call variant is:
 
 		updateTheUpdates(container, 4, i);
		
 where i is 1, 2, 3, etc.  The order of calls should produce a closed set starting at 1
 and increasing by 1.  These add new values to a container in such a way so that the
 verifier can check them.  This can be called at any time to create the new values.
*/

static void CM_NEAR  updateTheUpdates(CMContainer container, short key, short newValueNbr)
{
	CMObject			o1, o7, o8, o9;
	CMProperty		p1, p2, p3, p4, p5, p5a, p6, p7a, p7b, p8, p9, p10;
	CMType				t1, t2, t3;
	CMValue				v1, v2, v3, v4, v9b, v27, v38, v41;
	unsigned char buffer[256];

	/* Set up standard properties, types, objects...																			*/
	
	if (key < 4) {
		p1 = CMRegisterProperty(container, "property #1");
		p2 = CMRegisterProperty(container, "property #2");
		p3 = CMRegisterProperty(container, "value tests");
		p4 = CMRegisterProperty(container, "move update tests");
		p5 = CMRegisterProperty(container, "insert tests");
		p5a= CMRegisterProperty(container, "insert tests2");
		p6 = CMRegisterProperty(container, "delete tests");
		p7a= CMRegisterProperty(container, "move tests (from)");
		p7b= CMRegisterProperty(container, "move tests (to)");
		p8 = CMRegisterProperty(container, "new object property #1");
		p9 = CMRegisterProperty(container, "new object property #2");
		
		t1 = CMRegisterType(container, "type #1");
		t2 = CMRegisterType(container, "type #2");
		t3 = CMRegisterType(container, "type #3");
		
		if ((o1 = CMGetNextObjectWithProperty(container, NULL, p3)) == NULL) {
			display(stderr, "### cannot find \"value tests\" property in container\n");
			exit(EXIT_FAILURE);
		}
		++successes;
		if ((o7 = CMGetNextObjectWithProperty(container, NULL, p4)) == NULL) {
			display(stderr, "### cannot find \"p4\" property from updating container\n");
			exit(EXIT_FAILURE);
		}
		++successes;
		if ((o8 = CMGetNextObjectWithProperty(container, NULL, p8)) == NULL) {
			display(stderr, "### cannot find \"p8\" property in container\n");
			exit(EXIT_FAILURE);
		}
		++successes;
		if ((o9 = CMGetNextObjectWithProperty(container, NULL, p9)) == NULL) {
			display(stderr, "### cannot find \"p9\" property in container\n");
			exit(EXIT_FAILURE);
		}
		++successes;
	}
	
	switch (key) {
		case 1:	v1 = CMUseValue(o1, p3, t1);							/* immediate data */
						CMWriteValueData(v1, (CMPtr)"12", 1, 2);
		
						v2 = CMUseValue(o1, p3, TestType("t2"));	/* data insert */
						CMInsertValueData(v2, "<insert2>", 26, 9);
		
						v4 = CMUseValue(o1, p3, TestType("t4"));	/* data delete */
						CMWriteValueData(v4, (CMPtr)"<v4 overwrite>", 14, 14);
						
						case1 = true;
						return;
						
		case 2:	v9b = CMUseValue(o1, p3, TestType("t9b"));
						CMDeleteValueData(v9b, 8, 4);
						CMWriteValueData(v9b, (CMPtr)"<v9b appended>", 21, 14);
					
						v3 = CMUseValue(o1, p3, t2);
						CMSetValueType(v3, t2);
						CMSetValueGeneration(v3, 3);
						
						v27 = CMUseValue(o8, p8, TestType("t27"));
						CMMoveValue(v27, o9, p9);
		
						case2 = true;
						return;
						
		case 3:	v38 = CMUseValue(o7, p4, TestType("t38"));
						CMWriteValueData(v38, (CMPtr)"<append>", CMGetValueSize(v38), 8);
						
						v41 = CMUseValue(o9, p9, TestType("t41"));
						CMInsertValueData(v41, (CMPtr)"<insert>", 0, 8);
						
						p10 = CMRegisterProperty(container, "new object property #3");
						CMMoveValue(v41, o9, p10);
		
						case3 = true;
						return;
						
		default:/* all new goodies */
						if (newValueNbr > maxValueNbr) maxValueNbr = newValueNbr;
		
						o1 = CMNewObject(container);
						
						sprintf((char *)buffer, "new property #p%d", newValueNbr);
						p1 = CMRegisterProperty(container, (CMGlobalName)buffer);
						
						sprintf((char *)buffer, "new type #t%d1", newValueNbr);
						v1 = CMNewValue(o1, p1, TestType(buffer));
						CMWriteValueData(v1, (CMPtr)"1234567890", 0, 10);
						
						sprintf((char *)buffer, "new type #t%d2", newValueNbr);
						v2 = CMNewValue(o1, p1, TestType(buffer));
						CMWriteValueData(v2, (CMPtr)"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 26);
						
						sprintf((char *)buffer, "new type #t%d3", newValueNbr);
						v3 = CMNewValue(o1, p1, TestType(buffer));
						CMWriteValueData(v3, (CMPtr)"Ira Ruben", 0, 9);
		
						return;
	} /* switch */
}


/*---------------*
 | verifyUpdates |
 *---------------*
 
 This is called before the last close to make sure all the data was manipulated correctly.
 It assumes that createContainer() was called first, then createSomeUpdates(), and 
 finally (and optionally) updateTheUpdates() in the order described in that routine's
 comments.
*/

static void CM_NEAR verifyUpdates(CMContainer container)
{
	CMObject			o1, o7, o8, o9;
	CMProperty		p1, p2, p3, p4, p5, p5a, p6, p7a, p7b, p8, p9, p10;
	CMType				t0, t1, t2, t3;
	CMValue				v1, v2, v3, v4, v6, v9a, v9b,
								v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40,
								v41, v50;
	CMGeneration  generation;
	CMSize				amountRead;
	short					i;
	unsigned char buffer[256];

	p1 = CMRegisterProperty(container, "property #1");
	p2 = CMRegisterProperty(container, "property #2");
	p3 = CMRegisterProperty(container, "value tests");
	p4 = CMRegisterProperty(container, "move update tests");
	p5 = CMRegisterProperty(container, "insert tests");
	p5a= CMRegisterProperty(container, "insert tests2");
	p6 = CMRegisterProperty(container, "delete tests");
	p7a= CMRegisterProperty(container, "move tests (from)");
	p7b= CMRegisterProperty(container, "move tests (to)");
	p8 = CMRegisterProperty(container, "new object property #1");
	p9 = CMRegisterProperty(container, "new object property #2");
	p10= CMRegisterProperty(container, "new object property #3");
	
	t1 = CMRegisterType(container, "type #1");
	t2 = CMRegisterType(container, "type #2");
	t3 = CMRegisterType(container, "type #3");
	
	if ((o1 = CMGetNextObjectWithProperty(container, NULL, p3)) == NULL) {
		display(stderr, "### cannot find \"value tests\" property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	
	#if 1
	v1 = CMUseValue(o1, p3, t1);							/* immediate data */
	amountRead = CMReadValueData(v1, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v1 = %s [%ld]", buffer, amountRead);
	if (case1)
		sprintf((char *)buffer, "    v1 = a12d [4]");
	else
		sprintf((char *)buffer, "    v1 = abcd [4]");
	checkIt(whatWeGot, (char *)buffer);
	#endif
	
	#if 1
	v2 = CMUseValue(o1, p3, TestType("t2"));	/* data insert */
	amountRead = CMReadValueData(v2, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v2 = %s [%ld]", buffer, amountRead);
	if (case1)
		checkIt(whatWeGot, "    v2 = ABCDEFGHIJ<insert>KLMNOPQR<insert2>STUVWXYZ [43]");
	else
		checkIt(whatWeGot, "    v2 = ABCDEFGHIJ<insert>KLMNOPQRSTUVWXYZ [34]");
	#endif
	
	#if 1
	v4 = CMUseValue(o1, p3, TestType("t4"));	/* data delete */
	amountRead = CMReadValueData(v4, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v4 = %s [%ld]", buffer, amountRead);
	if (case1)
		checkIt(whatWeGot, "    v4 = 12345678901234<v4 overwrite> [28]");
	else
		checkIt(whatWeGot, "    v4 = 12345678901234567890 [20]");
	#endif
	
	#if 1
	v6 = CMUseValue(o1, p3, TestType("t6"));	/* delete/insert (overwrite) */
	amountRead = CMReadValueData(v6, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v6 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v6 = 1overwriteABCDE [15]");
	#endif

	#if 1
	v9a = CMUseValue(o1, p3, TestType("t9a"));/* delete the middle of a segment */
	amountRead = CMReadValueData(v9a, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v9a = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v9a = 1234567890ABGHIJabcdefghijsegment 4asegment 5asegment 6a [56]");
	#endif

	#if 1
	v9b = CMUseValue(o1, p3, TestType("t9b"));/* multi-segment delete and insert */
	amountRead = CMReadValueData(v9b, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v9b = %s [%ld]", buffer, amountRead);
	if (case2)
		checkIt(whatWeGot, "    v9b = abcdefgh345<insert>6b<v9b appended> [35]");
	else
		checkIt(whatWeGot, "    v9b = abcdefghij12345<insert>6b [25]");
	#endif
	
	#if 1
	v50 = CMUseValue(o1, p3, TestType("t50"));/* multi-segment delete and insert */
	amountRead = CMReadValueData(v50, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v50 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v50 = v<insert>50 [11]");
	#endif
	
	#if 1
	v3 = CMUseValue(o1, p3, t2);							/* set-infoed */
	CMGetValueInfo(v3, NULL, NULL, NULL, &t0, &generation);
	if (t0 != t2) {
		display(stderr, "### got wrong new type from CMGetValueInfo() during update checking\n");
		++failures;
	} else
		++successes;
		
	if (generation != (case2 ? (CMGeneration)3 : (CMGeneration)2)) {
		display(stderr, "### got wrong generation (%d) from CMGetValueInfo() during update checking\n", generation);
		++failures;
	} else
		++successes;
	#endif

	if ((o7 = CMGetNextObjectWithProperty(container, NULL, p4)) == NULL) {
		display(stderr, "### cannot find \"p4\" property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	if ((o8 = CMGetNextObjectWithProperty(container, NULL, p8)) == NULL) {
		display(stderr, "### cannot find \"p8\" property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	if ((o9 = CMGetNextObjectWithProperty(container, NULL, p9)) == NULL) {
		display(stderr, "### cannot find \"p9\" property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;

	v26 = CMUseValue(o7, p1, TestType("t26"));
	v27 = case2 ? CMUseValue(o9, p9, TestType("t27")) : CMUseValue(o8, p8, TestType("t27"));
	v28 = CMUseValue(o7, p2, TestType("t28"));
	v29 = CMUseValue(o7, p4, TestType("t29"));
	v30 = CMUseValue(o7, p4, TestType("t30"));
	v31 = CMUseValue(o8, p8, TestType("t31"));
	v32 = CMUseValue(o7, p1, TestType("t32"));
	v33 = CMUseValue(o8, p2, TestType("t33"));
	v34 = CMUseValue(o9, p9, TestType("t34"));
	v35 = CMUseValue(o7, p4, TestType("t35"));
	v36 = CMUseValue(o7, p4, TestType("t36"));
	v37 = CMUseValue(o9, p9, TestType("t37"));
	v38 = CMUseValue(o7, p4, TestType("t38"));
	v39 = CMUseValue(o7, p4, TestType("t39"));
	v40 = CMUseValue(o7, p1, TestType("t40"));
	v41 = case3 ? CMUseValue(o9, p10,TestType("t41")) : CMUseValue(o9, p9,TestType("t41"));
	
	amountRead = CMReadValueData(v26, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v26 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v26 = v26 [3]");
	
	amountRead = CMReadValueData(v27, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v27 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v27 = v27 [3]");
	
	amountRead = CMReadValueData(v28, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v28 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v28 = v28 [3]");
	
	amountRead = CMReadValueData(v29, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v29 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v29 = v29 [3]");
	
	amountRead = CMReadValueData(v30, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v30 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v30 = v30 [3]");
	
	amountRead = CMReadValueData(v31, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v31 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v31 = v31 [3]");
	
	amountRead = CMReadValueData(v32, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v32 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v32 = v32 [3]");
	
	amountRead = CMReadValueData(v33, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v33 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v33 = v33 [3]");
	
	amountRead = CMReadValueData(v34, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v34 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v34 = v34 [3]");
	
	amountRead = CMReadValueData(v35, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v35 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v35 = v35 [3]");
		
	amountRead = CMReadValueData(v36, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v36 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v36 = v36 [3]");
	
	amountRead = CMReadValueData(v37, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v37 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v37 = v37 [3]");
	
	amountRead = CMReadValueData(v38, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v38 = %s [%ld]", buffer, amountRead);
	if (case3)
		checkIt(whatWeGot, "    v38 = v<insert>38<append> [19]");
	else
		checkIt(whatWeGot, "    v38 = v<insert>38 [11]");
	
	amountRead = CMReadValueData(v39, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v39 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v39 = v<insert>39 [11]");
	
	amountRead = CMReadValueData(v40, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v40 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v40 = v<insert>40 [11]");
	
	amountRead = CMReadValueData(v41, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v41 = %s [%ld]", buffer, amountRead);
	if (case3)
		checkIt(whatWeGot, "    v41 = <insert>v<insert>41<append> [27]");
	else
		checkIt(whatWeGot, "    v41 = v<insert>41<append> [19]");
	
	for (i = 1; i <= maxValueNbr; ++i) {
		sprintf((char *)buffer, "new property #p%d", i);
		p1 = CMRegisterProperty(container, (CMGlobalName)buffer);
		
		if ((o1 = CMGetNextObjectWithProperty(container, NULL, p1)) == NULL) {
			display(stderr, "### cannot find \"%s\" property in container\n", buffer);
			exit(EXIT_FAILURE);
		}
		++successes;
		
		sprintf((char *)buffer, "new type #t%d1", i);
		v1 = CMUseValue(o1, p1, TestType(buffer));
		amountRead = CMReadValueData(v1, (CMPtr)buffer, 0, 255);
		*(buffer+amountRead) = 0;
		sprintf(whatWeGot, "    v1(%d) = %s [%ld]", i, buffer, amountRead);
		sprintf((char *)buffer, "    v1(%d) = 1234567890 [10]", i);
		checkIt(whatWeGot, (char *)buffer);
		
		sprintf((char *)buffer, "new type #t%d2", i);
		v2 = CMUseValue(o1, p1, TestType(buffer));
		amountRead = CMReadValueData(v2, (CMPtr)buffer, 0, 255);
		*(buffer+amountRead) = 0;
		sprintf(whatWeGot, "    v2(%d) = %s [%ld]", i, buffer, amountRead);
		sprintf((char *)buffer, "    v2(%d) = ABCDEFGHIJKLMNOPQRSTUVWXYZ [26]", i);
		checkIt(whatWeGot, (char *)buffer);
		
		sprintf((char *)buffer, "new type #t%d3", i);
		v3 = CMUseValue(o1, p1, TestType(buffer));
		amountRead = CMReadValueData(v3, (CMPtr)buffer, 0, 255);
		*(buffer+amountRead) = 0;
		sprintf(whatWeGot, "    v3(%d) = %s [%ld]", i, buffer, amountRead);
		sprintf((char *)buffer, "    v3(%d) = Ira Ruben [9]", i);
		checkIt(whatWeGot, (char *)buffer);
	}
}


/*---------------*
 | getTargetType |
 *---------------*
 
 This creates a type that, when used by CMNewValue() or CMUseValue() will spawn a dynamic
 value for CMOpenNewContainer() to use when updating another container (i.e., the useFlags
 were kCMUpdateTarget). The type must be associated with a dynamic value handler package
 that will access the target container to be updated.
 
 A pointer to this routine is passed to createRefConForMyHandlers() which uses it for the
 returnTargetType_Handler().  This handler is only used by the Container Manager when
 opening a target container for updating.
*/

static CMType CM_FIXEDARGS getTargetType(CMContainer theUpdatingContainer)
{
	return (RegisterFileValueType(theUpdatingContainer));
}


/*------------------*
 | doDebuggingTests |
 *------------------*
 
 This is used for basic debugging of the updating code.  Here only one updater and its
 target are involved.  All the updating "touches" are excerised (hopefully).
*/

static void CM_NEAR doDebuggingTests(CMBoolean separateTarget, char *targetFilename,
																															 char *updatingFilename)
{
	CMContainer container;
	CMRefCon	  myRefCon;
	char 				*typeName = separateTarget ? "Separate Updater" : "UBA";
	
	createContainer(targetFilename);
	
	if (!separateTarget) strcpy(updatingFilename, targetFilename);
	
	myRefCon = createRefConForMyHandlers(session, updatingFilename, getTargetType);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, typeName, containerMetahandler);
	
	container = CMOpenNewContainer(session, myRefCon, typeName,
																 (CMContainerUseMode)(separateTarget ? kCMUpdateTarget : kCMUpdateByAppend),
																 2, 0, targetFilename, "rb");
	if (container == NULL) {
		display(stderr, "### Debugging tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	createSomeUpdates(container);
	
	if (dbgFile) CMDumpTOCStructures(container, dbgFile);

	CMCloseContainer(container);
	
	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");
	
	myRefCon = createRefConForMyHandlers(session, updatingFilename, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	
	CMSetMetaHandler(session, "Reading Test", containerMetahandler);
	container = CMOpenContainer(session, myRefCon, "Reading Test", 0);
	if (container == NULL) {
		display(stderr, "### reading tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	/*if (dbgFile) CMDumpTOCStructures(container, dbgFile);*/
	
	verifyUpdates(container);
	
	/*if (dbgFile) CMDumpTOCStructures(container, dbgFile);*/
	
	CMCloseContainer(container);
}


/*-------------------*
 | doSpaceReuseTests |
 *-------------------*
 
 This tests the basic space resue when opening an exiting container to update it.
*/

static void CM_NEAR doSpaceReuseTests(char *targetFileName)
{
	CMContainer 	container;
	CMRefCon			myRefCon;
	CMType				t1, t2;
	CMProperty		p1, p2, p3, p4, p5, p6, p7a, p7b;
	CMObject			o1, o3, o4, o6, o7;
	CMValue				v1, v2, v9a, v9b, v10a, v10b;
	CMSize				amountRead;
	unsigned char buffer[256];
	
	createContainer(targetFileName);
	
	CMDebugging(session, 256, NULL, 1);
	
	myRefCon = createRefConForMyHandlers(session, targetFileName, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);

	if (!quiet) display(stderr, "\nOpening \"%s\" for space reuse tests...\n\n", targetFileName);
	
	CMSetMetaHandler(session, "Reuse Space", containerMetahandler);
	container = CMOpenContainer(session, myRefCon, "Reuse Space", kCMReuseFreeSpace);
	
	if (container == NULL) {
		display(stderr, "### Space reuse tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}

	p1 = CMRegisterProperty(container, "property #1");
	p2 = CMRegisterProperty(container, "property #2");
	p3 = CMRegisterProperty(container, "value tests");
	p4 = CMRegisterProperty(container, "move update tests");
	p5 = CMRegisterProperty(container, "insert tests");
	p6 = CMRegisterProperty(container, "delete tests");
	p7a= CMRegisterProperty(container, "move tests (from)");
	p7b= CMRegisterProperty(container, "move tests (to)");
	
	t1 = CMRegisterType(container, "type #1");
	t2 = CMRegisterType(container, "type #2");

	if ((o1 = CMGetNextObjectWithProperty(container, NULL, p3)) == NULL) {
		display(stderr, "### cannot find \"value tests\" property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	
	/* v1 = 123 */
	/*      012 */
	
	v1 = CMUseValue(o1, p3, t1);												/* v1 = 23 												*/
	CMDeleteValueData(v1, 0, 1);												/*      01												*/
	CMWriteValueData(v1, (CMPtr)"ab", 1, 2);						/* v1 = 2ab											  */
	amountRead = CMReadValueData(v1, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v1 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v1 = 2ab [3]");
	
	/* v2 = ABCDEFGHIJKLMNOPQRSTUVWXYZ */
  /*      01234567890123456789012345 */
	
	v2 = CMUseValue(o1, p3, TestType("t2"));
	CMDeleteValueData(v2, 10, 100);											/* v2 = ABCDEFGHIJ 								*/ 
	CMWriteValueData(v2, (CMPtr)"1234567890", 10, 10);	/* v2 = ABCDEFGHIJ1234567890 			*/ 
	amountRead = CMReadValueData(v2, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v2 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v2 = ABCDEFGHIJ1234567890 [20]");
	
	/* v9a = 1234567890ABCDEFGHIJabcdefghij */
	/* v9b = abcdefghij1234567890ABCDEFGHIJ */
  /*      0123456789012345678901234567890 */
	
	v9a = CMUseValue(o1, p3, TestType("t9a"));
	v9b = CMUseValue(o1, p3, TestType("t9b"));
	
	/* Delete a bunch of stuff to make TOC smaller to force the double write algorithm...	*/
	
	CMDeleteValueData(v9a, 0, 100);
	CMDeleteValueData(v9b, 0, 100);
	
	/* v10a = ABCDEFG###TEST10###T 			 */
	/* v10b = abcdefghijklm***test10***  */
	/*        0123456789012345678901234  */
																												
	v10a = CMUseValue(o1, p3, TestType("t10a"));				/*        01234567890123456789		*/
	CMDeleteValueData(v10a, 5, 5);											/*        ABCDETEST10###T					*/
	CMDeleteValueData(v10a, 11, 100);										/*        ABCDETEST10							*/
	CMDeleteValueData(v10a, 0, 5);											/*        TEST10									*/
	CMInsertValueData(v10a, (CMPtr)"***", 0, 3);				/*        ***TEST10								*/
	CMInsertValueData(v10a, (CMPtr)"***", 9, 3);				/*        ***TEST10***						*/
																											/*        01234567890123456789		*/
	amountRead = CMReadValueData(v10a, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v10a = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v10a = ***TEST10*** [12]");
	
	v10b = CMUseValue(o1, p3, TestType("t10b"));
	CMDeleteValueData(v10b, 0, 100);
	
	if ((o3 = CMGetNextObjectWithProperty(container, NULL, p5)) == NULL) {
		display(stderr, "### cannot find \"insert tests\" property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	if ((o4 = CMGetNextObjectWithProperty(container, NULL, p6)) == NULL) {
		display(stderr, "### cannot find \"delete tests\" property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	if ((o6 = CMGetNextObjectWithProperty(container, NULL, p7b)) == NULL) {
		display(stderr, "### cannot find \"move tests\" property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	if ((o7 = CMGetNextObjectWithProperty(container, NULL, p4)) == NULL) {
		display(stderr, "### cannot find \"move update tests\" property from updating container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	
	CMDeleteObject(o3);
	CMDeleteObject(o4);
	CMDeleteObject(o6);
	CMDeleteObject(o7);

	if (dontClose) goto readIt;
	
	CMCloseContainer(container);	
	
	if (dbgFile) {
		display(dbgFile, "--------------------------------------------------------------------------\n");

		CMDebugging(session, 256, dbgFile, 1);
		myRefCon = createRefConForMyHandlers(session, targetFileName, NULL);
		setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
		
		CMSetMetaHandler(session, "Space reuse2", containerMetahandler);
		container = CMOpenContainer(session, myRefCon, "Space reuse2", 0);
		
		if (container == NULL) {
			display(stderr, "### Space reuse #2 container dump test terminated: container == NULL\n");
			exit(EXIT_FAILURE);
		}
		
		CMDumpTOCStructures(container, dbgFile);
		CMCloseContainer(container);	
	}
	
	/* Reopen container just to see if our update is still there...												*/
	
	CMDebugging(session, 256, NULL, 1);
	
	myRefCon = createRefConForMyHandlers(session, targetFileName, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	
	CMSetMetaHandler(session, "Space reuse3", containerMetahandler);
	container = CMOpenContainer(session, myRefCon, "Space reuse3", 0);
	
	if (container == NULL) {
		display(stderr, "### Space reuse #3 container reopen test terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}

	p3 = CMRegisterProperty(container, "value tests");
	t1 = CMRegisterType(container, "type #1");
	
	if ((o1 = CMGetNextObjectWithProperty(container, NULL, p3)) == NULL) {
		display(stderr, "### cannot find \"value tests\" property in container\n");
		exit(EXIT_FAILURE);
	}
	++successes;
	
readIt:

	v1 = CMUseValue(o1, p3, t1);
	amountRead = CMReadValueData(v1, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v1 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v1 = 2ab [3]");
	
	v2 = CMUseValue(o1, p3, TestType("t2"));
	amountRead = CMReadValueData(v2, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v2 = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v2 = ABCDEFGHIJ1234567890 [20]");
	
	v10a = CMUseValue(o1, p3, TestType("t10a"));
	amountRead = CMReadValueData(v10a, (CMPtr)buffer, 0, 255);
	*(buffer+amountRead) = 0;
	sprintf(whatWeGot, "    v10a = %s [%ld]", buffer, amountRead);
	checkIt(whatWeGot, "    v10a = ***TEST10*** [12]");
	
	CMCloseContainer(container);	
}


/*------------*
 | doUbaTests |
 *------------*
 
 This tests opening appended containers for updating.  It is used as an isolated test, or 
 as a separate target for doSeparateContainerUpdateTests().
 
 A base container and five (count 'em -- 5) appended updating containers are generated.
*/

static void CM_NEAR doUbaTests(CMBoolean isTarget, char *targetFilename)
{
	CMContainer container;
	CMRefCon	  myRefCon;
	
	createContainer(targetFilename);
	
	myRefCon = createRefConForMyHandlers(session, targetFilename, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, "UBA2", containerMetahandler);
	container = CMOpenNewContainer(session, myRefCon, "UBA2", kCMUpdateByAppend, 2, 0);
	if (container == NULL) {
		display(stderr, "### UBA tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	updateTheUpdates(container, 4, 1);
	
	CMCloseContainer(container);
	
	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");
	
	myRefCon = createRefConForMyHandlers(session, targetFilename, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, "UBA3", containerMetahandler);
	container = CMOpenNewContainer(session, myRefCon, "UBA3", kCMUpdateByAppend, 3, 0);
	if (container == NULL) {
		display(stderr, "### UBA tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	/*-------------------------*/
	createSomeUpdates(container); /* a bunch of updates to base container at this level */
	/*-------------------------*/
	
	updateTheUpdates(container, 4, 2);
	
	CMCloseContainer(container);
	
	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");
	
	myRefCon = createRefConForMyHandlers(session, targetFilename, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, "UBA4", containerMetahandler);
	container = CMOpenNewContainer(session, myRefCon, "UBA4", kCMUpdateByAppend, 4, 0);
	if (container == NULL) {
		display(stderr, "### UBA tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	updateTheUpdates(container, 1, 0);
	updateTheUpdates(container, 4, 3);
	
	CMCloseContainer(container);

	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");
	
	myRefCon = createRefConForMyHandlers(session, targetFilename, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, "UBA5", containerMetahandler);
	container = CMOpenNewContainer(session, myRefCon, "UBA5", kCMUpdateByAppend, 5, 0);
	if (container == NULL) {
		display(stderr, "### UBA tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	if (!isTarget) updateTheUpdates(container, 2, 0);
	updateTheUpdates(container, 4, 4);
	
	if (dbgFile) {
		fprintf(dbgFile, "UBA5 dump just after updateTheUpdates >>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		CMDumpTOCStructures(container, dbgFile);
	}
	
	CMCloseContainer(container);
	
	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");
	
	myRefCon = createRefConForMyHandlers(session, targetFilename, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, "UBA6", containerMetahandler);
	container = CMOpenNewContainer(session, myRefCon, "UBA6", kCMUpdateByAppend, 6, 0);
	if (container == NULL) {
		display(stderr, "### UBA tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	if (!isTarget) updateTheUpdates(container, 3, 0);
	updateTheUpdates(container, 4, 5);
	
	CMCloseContainer(container);

	if (isTarget) return;

	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");
	
	myRefCon = createRefConForMyHandlers(session, targetFilename, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	
	CMSetMetaHandler(session, "Reading Test", containerMetahandler);
	container = CMOpenContainer(session, myRefCon, "Reading Test", 0);
	
	if (container == NULL) {
		display(stderr, "### reading tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	if (dbgFile) CMDumpTOCStructures(container, dbgFile);
	
	verifyUpdates(container);
	
	if (failures && isTarget) 
		display(stderr, "\n----- above failures in \"open append tests\" -----\n\n");
	
	CMCloseContainer(container);
}


/*--------------------------------*
 | doSeparateContainerUpdateTests |
 *--------------------------------*
 
 This tests opening a separate target for updating.
*/

static void CM_NEAR doSeparateContainerUpdateTests(char *targetFilename,
																									 char *updatingFilename)
{
	CMContainer 	container;
	CMRefCon	  	myRefCon;
	char					updatingFilename1[256];
	
	/* Generate a target consisting of a "flock" (?) of appended containers...						*/
	
	doUbaTests(true, targetFilename);
	
	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");
	
	myRefCon = createRefConForMyHandlers(session, updatingFilename, getTargetType);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, "Separate Updater", containerMetahandler);
	
	container = CMOpenNewContainer(session, myRefCon, "Separate Updater", kCMUpdateTarget,
																 1, 0, targetFilename, "rb");
	if (container == NULL) {
		display(stderr, "### Remote updating tests #1 terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	updateTheUpdates(container, 2, 0);		
	updateTheUpdates(container, 4, 6);
	
	CMCloseContainer(container);
	
	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");
	
	myRefCon = createRefConForMyHandlers(session, updatingFilename, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, "Appended to Separate Updater", containerMetahandler);
	
	container = CMOpenNewContainer(session, myRefCon, "Appended to Separate Updater", kCMUpdateByAppend, 2, 0);
	if (container == NULL) {
		display(stderr, "### Dynamic append tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	updateTheUpdates(container, 4, 7);
	
	CMCloseContainer(container);
	
	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");
	
	strcat(strcpy((char *)updatingFilename1, (char *)updatingFilename), "1");
	
	myRefCon = createRefConForMyHandlers(session, updatingFilename1, getTargetType);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, "Separate Updater1", containerMetahandler);
	
	container = CMOpenNewContainer(session, myRefCon, "Separate Updater1", kCMUpdateTarget,
																 3, 0, updatingFilename, "rb");
	if (container == NULL) {
		display(stderr, "### Remote updating tests #2 terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	updateTheUpdates(container, 4, 8);
	updateTheUpdates(container, 3, 0);		
	
	CMCloseContainer(container);

	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");
	
	myRefCon = createRefConForMyHandlers(session, updatingFilename1, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	
	CMSetMetaHandler(session, "Reading Test", containerMetahandler);
	container = CMOpenContainer(session, myRefCon, "Reading Test", 0);
	
	if (container == NULL) {
		display(stderr, "### reading tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
			
	verifyUpdates(container);
	
	/*if (dbgFile) CMDumpTOCStructures(container, dbgFile);*/

	CMCloseContainer(container);
}


/*-------------------*
 | doUpdateRefsTests |
 *-------------------*
 
 This tests opening a an appended target for updating references.  Two appends are done.
 The net result of the two sets of updates are to produce a reference set exactly like
 the one used in MainTests.c.  It was used as the bases for these tests.
*/

static void CM_NEAR doUpdateRefsTests(char *targetFilename, char *updatingFilename)
{
	CMContainer 	 container;
	CMRefCon	  	 myRefCon;
	CMType				 t1, t2;
	CMProperty		 p1, p2;
	CMObject			 testO, o[9];
	CMValue				 testV, v[9];
	CMReference		 refData[7];
	CMSize				 amountRead;
	short					 i, j, n;
	unsigned char  buffer[256];
	
	createContainerWithReferences(targetFilename);

	myRefCon = createRefConForMyHandlers(session, targetFilename, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, "DebuggingRefs2", containerMetahandler);
	container = CMOpenNewContainer(session, myRefCon, "DebuggingRefs2", kCMUpdateByAppend, 2, 0);
	if (container == NULL) {
		display(stderr, "### reference appended updating tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	p1 = CMRegisterProperty(container, "target referenced property");
	t1 = CMRegisterType(container, "target referenced type");
	
	p2 = CMRegisterProperty(container, "referencing property");
	t2 = CMRegisterType(container, "referencing type");

	if ((testO = CMGetNextObjectWithProperty(container, NULL, p2)) == NULL) {
		display(stderr, "### cannot find \"referencing\" property in target container\n");
		exit(EXIT_FAILURE);
	}
	testV = CMUseValue(testO, p2, t2);

	/* Create 2 additional objects in this updating container using CMSetReference()...		*/
	
	for (i = 5; i < 7; ++i) {												/* create 2 more objs with 1 value		*/
		o[i] = CMNewObject(container);								/* o[i] 															*/
		v[i] = CMNewValue(o[i], p1, t1);							/* o[i] contains value v[i]						*/
		sprintf((char *)buffer, "v[%d]", i);					/* value data for v[i] is "v[i]"			*/
		CMWriteValueData(v[i], (CMPtr)buffer, 0, 4);
	}

	for (i = 5; i < 7; ++i)
		for (j = 0; j < sizeof(CMReference); ++j)
			refData[i][j] = 0x00;

	memcpy(refData[5], "ref5", sizeof(CMReference));
	CMSetReference(testV, o[5], refData[5]);
	memcpy(refData[6], "ref6", sizeof(CMReference));
	CMSetReference(testV, o[6], refData[6]);
		
	CMWriteValueData(testV, (CMPtr)&refData[5], 24, 2 * sizeof(CMReference));
	CMWriteValueData(testV, (CMPtr)">>>>", 4 + 7 * sizeof(CMReference), 4);

	CMCloseContainer(container);

	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");

	myRefCon = createRefConForMyHandlers(session, targetFilename, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, "DebuggingRefs2", containerMetahandler);
	container = CMOpenNewContainer(session, myRefCon, "DebuggingRefs2", kCMUpdateByAppend, 3, 0);
	if (container == NULL) {
		display(stderr, "### reference appended updating tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	p1 = CMRegisterProperty(container, "target referenced property");
	t1 = CMRegisterType(container, "target referenced type");
	
	p2 = CMRegisterProperty(container, "referencing property");
	t2 = CMRegisterType(container, "referencing type");

	if ((testO = CMGetNextObjectWithProperty(container, NULL, p2)) == NULL) {
		display(stderr, "### cannot find \"referencing\" property in target container\n");
		exit(EXIT_FAILURE);
	}
	testV = CMUseValue(testO, p2, t2);
		
	if (CMReadValueData(testV, (CMPtr)refData, 4, 7 * sizeof(CMReference)) != 7 * sizeof(CMReference)) {
		display(stderr, "### Reference value data was not fully read.\n");
		exit(EXIT_FAILURE);
	}
		
	/* Create 1 additional object in this updating container using CMSetReference()...		*/
	
	o[7] = CMNewObject(container);
	v[7] = CMNewValue(o[7], p1, t1);
	CMWriteValueData(v[7], (CMPtr)"v[7]", 0, 4);
	
	/* Change the 2nd reference to assoiciate to the 8th object.								 					*/

	CMSetReference(testV, o[7], refData[1]);

	CMCloseContainer(container);
	
	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");
	
	myRefCon = createRefConForMyHandlers(session, updatingFilename, getTargetType);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	CMSetMetaHandler(session, "Separate Reference Updater", containerMetahandler);
	
	container = CMOpenNewContainer(session, myRefCon, "Separate Reference Updater",
																 kCMUpdateTarget, 4, 0, targetFilename, "rb");
	if (container == NULL) {
		display(stderr, "### Remote reference updating tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	p1 = CMRegisterProperty(container, "target referenced property");
	t1 = CMRegisterType(container, "target referenced type");
	
	p2 = CMRegisterProperty(container, "referencing property");
	t2 = CMRegisterType(container, "referencing type");

	if ((testO = CMGetNextObjectWithProperty(container, NULL, p2)) == NULL) {
		display(stderr, "### cannot find \"referencing\" property in target container\n");
		exit(EXIT_FAILURE);
	}
	testV = CMUseValue(testO, p2, t2);
		
	if (CMReadValueData(testV, (CMPtr)refData, 4, 7 * sizeof(CMReference)) != 7 * sizeof(CMReference)) {
		display(stderr, "### Reference value data was not fully read.\n");
		exit(EXIT_FAILURE);
	}
		
	/* Create 1 additional object in this updating container using CMSetReference()...		*/
	
	o[8] = CMNewObject(container);
	v[8] = CMNewValue(o[8], p1, t1);
	CMWriteValueData(v[8], (CMPtr)"v[8]", 0, 4);
	
	/* Change the 4th reference to assoiciate to the 9th object.  We should end up with 7 */
	/* references to 7 objects and there will be no references to o[1] and o[3].					*/

	CMSetReference(testV, o[8], refData[3]);
	
	CMCloseContainer(container);

	if (dbgFile) 
		display(dbgFile, "==========================================================================\n");

	myRefCon = createRefConForMyHandlers(session, updatingFilename, NULL);
	setHandlersTrace(myRefCon, traceHandlers, handlerDbgFile);
	
	CMSetMetaHandler(session, "Reading References Test", containerMetahandler);
	container = CMOpenContainer(session, myRefCon, "Reading References Test", 0);
	
	if (container == NULL) {
		display(stderr, "### reading references tests terminated: container == NULL\n");
		exit(EXIT_FAILURE);
	}
	
	if (dbgFile) CMDumpTOCStructures(container, dbgFile);
	
	p1 = CMRegisterProperty(container, "target referenced property");
	t1 = CMRegisterType(container, "target referenced type");
	
	p2 = CMRegisterProperty(container, "referencing property");
	t2 = CMRegisterType(container, "referencing type");
	
	if ((testO = CMGetNextObjectWithProperty(container, NULL, p2)) == NULL) {
		display(stderr, "### cannot find \"referencing\" property in container\n");
		exit(EXIT_FAILURE);
	}
	testV = CMUseValue(testO, p2, t2);
	
	if (CMReadValueData(testV, (CMPtr)refData, 4, 7 * sizeof(CMReference)) != 7 * sizeof(CMReference)) {
		display(stderr, "### Reference value data was not fully read.\n");
		exit(EXIT_FAILURE);
	}

	n = (short)CMCountReferences(testV);
	
	if (n != 7) {
		display(stderr, "### CMCountReferences() returned %d references (should be 7)\n", n);
		exit(EXIT_FAILURE);
	}
	++successes;
		 
	/* The 2nd and 4th reference assoiciate to the 8th and 9th objects respectively.  		*/

	/* Get each reference and verify it...																								*/ 
	
	for (i = 0; i < n; ++i) {
		if (i == 1)
			j = 7;
		else if (i == 3)
			j = 8;
		else
			j = i;

		o[j] = CMGetReferencedObject(testV, refData[i]);
		if (o[j] == NULL) {
			display(stderr, "### CMGetReferencedObject() failed.\n");
			exit(EXIT_FAILURE);
		}
		++successes;
		
		v[j] = CMUseValue(o[j], p1, t1);
		if (v[j] == NULL) {
			display(stderr, "### Cannot get referenced object's value (%d)\n", j);
			exit(EXIT_FAILURE);
		}
		++successes;
		
		amountRead = CMReadValueData(v[j], (CMPtr)buffer, 0, 255);
		*(buffer+amountRead) = 0;
		sprintf(whatWeGot, "    v[%d] = \"%s\"", j, buffer);
		sprintf((char *)buffer,    "    v[%d] = \"v[%d]\"", j, j);
		checkIt(whatWeGot, (char *)buffer);
	}
	
	CMCloseContainer(container);
}
	

/*------*
 | main |
 *------*/

void CM_C main(int argc, char *argv[])
{
	#if THINK_C || THINK_CPLUS
	/*
	console_options.nrows = 70;
	console_options.ncols = 93;
	*/
	argc = ccommand(&argv);
	#endif
	
	processOptions(argc, argv);
	
	if (*targetFilename == 0) strcpy(targetFilename, "TargCont"       );
	strcpy(origFilename, targetFilename);
	
	if (*updatingFilename == 0) strcpy(updatingFilename, "Upd"              );
	
	session = CMStartSession(sessionRoutinesMetahandler, NULL);
	CMDebugging(session, 256, dbgFile, 1);
	
	CMSetMetaHandler(session, (CMGlobalName)CMTargetHandlersTypeName, targetContainerMetahandler);
	
	if (debugging)
		doDebuggingTests(dvTestsOnly, targetFilename, updatingFilename);
	else if (reuseTestOnly)
		doSpaceReuseTests(targetFilename);
	else if (ubaTestOnly)
		doUbaTests(false, targetFilename);
	else if (dvTestsOnly)
		doSeparateContainerUpdateTests(targetFilename, updatingFilename);
	else if (updateRefsOnly)
		doUpdateRefsTests(targetFilename, updatingFilename);
	else {
		doSpaceReuseTests(targetFilename);
		doSeparateContainerUpdateTests(targetFilename, updatingFilename);
		doUpdateRefsTests(targetFilename, updatingFilename);
	}
	
	CMEndSession(session, false);
	
	if (failures)
		display(stderr, "\n### successful tests passed: %ld\n"
											"    failed tests:            %ld\n\n", successes, failures);
	else
		display(stderr, "\n### All tests passed!  Ship it!\n\n");

	exit(EXIT_SUCCESS);
}

														  CM_END_CFUNCTIONS

