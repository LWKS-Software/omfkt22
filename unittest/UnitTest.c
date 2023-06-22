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

/* TestCases can be added to this test harness, by modifying the 
   initialization routine to add the test to the Test Table. The test
   function is assumed to be of the format:

   test_function(char *inputfile, char *outputfile)

   if your test_function does not conform to that format, then it should be
   wrapped in a wrapper function.  Example:

   int WrapReadMedia(char *input, char *output)
   { return(ReadMedia(input,TRUE)); }

   The Wrapper function should be the one used to do the test in this case.

*/

/* Standard Include files */
#include "masterhd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* OMF required include files */
#define OMFI_ENABLE_SEMCHECK 1 /* For Semantic checking functions in omUtils.h */

#include "omErr.h"
#include "omDefs.h"
#include "omTypes.h"
#include "omUtils.h"
#include "omFile.h"
#include "omMedia.h" 

/* Unittest required include files */
#include "MkMed1x.h"
#include "MkMedia.h"
#include "UnitTest.h"
#include "TestPvt.h"
#include "MkTape.h"

#if PORT_SYS_MAC
#include <events.h>
#include <Console.h>
#include <Memory.h>
#endif
#if AVID_CODEC_SUPPORT
#include "omcAvJPED.h" 
#include "omcAvidJFIF.h" 
#endif

#ifdef MAC_PROFILER
#include <profiler.h>
#endif

/*************************/
/* local Func Prototypes */
/*************************/

static int init_table(void);
static void print_header(int testnum);
static int copy_file(char *source, char *dest);
static int usage(void);
static int print_table(void);
static int update_dependencies(void);
static int print_summary(void);
static int free_testdata(void);

/***********************/
/* Global Variables    */
/***********************/

static int NUMTESTS = 1;
omfBool gVerbose = FALSE, gWarnings = FALSE;
static int RunTheseTests[MAXTESTS];
static int TheseTestsFailed[MAXTESTS];
static int run_only_some_tests = 0;
static int gRunFailed = 0, gVerifyFailed = 0, gSemCheckFailed = 0;
static int gSemCheckFaults = 0;
omfInt32 TotalErrors = 0, TotalWarnings = 0;

/*********************/
/* Wrapper functions */
/*********************/
/*int WrapAttr1x(char *filename, char *out)
{ return (Attr1x(filename)); }*/
int WrapChgMob(char *srcName, char *out)
{ return (ChgMob(srcName)); }
int WrapCopyMob(char *filename, char *out)
{ return (CopyMob(filename));  }
int WrapDelMob(char *srcName, char *out)
{ return(DelMob(srcName)); }
int WrapFndSrc(char *srcName, char *out)
{ return(FndSrc(srcName)); }
int WrapObjIter(char *filename, char *out)
{ return(ObjIter(filename)); }
int WrapOpenMod(char *filename, char *out)
{ return(OpenMod(filename));  }
/*int WrapCheck1xMedia(char *filename)
{ Check1xMedia(filename,TRUE); return(0); }*/
/*int WrapCheck1xMediaNoWave(char *filename)
{ Check1xMedia(filename,FALSE); return(0); }*/
int WrapMkDesc2(char *filename, char *out)
{ return(MkDesc(filename,"2")); }
int WrapDumpDK(char *filename, char *out)
{ return(DumpDK(filename)); }
int WrapERCvt(char *in, char *out)
{ return(ERCvt()); }
int WrapDumpFXD(char *filename,char *out)
{ return(DumpFXD(filename)); }
int WrapIterSrch(char *filename,char *out) 
{ return (IterSrch(filename)); }
int WrapTCCvt(char *in, char *out)
{ return(TCCvt()); }
int Wrap1MkCplx(char *filename, char *out)
{ return(MkCplx(filename,"1")); }
int Wrap2MkCplx(char *filename, char *out)
{ return(MkCplx(filename,"2")); }
int WrapIMAMkCplx(char *filename, char *out)
{ return(MkCplx(filename,"IMA")); }
int WrapMkComp2x(char *filename, char *out)
{ return(MkComp2x(filename)); }
int WrapMkIlleg(char *filename, char *out)
{ return(MkIlleg(filename)); }
/*int WrapWrite1xMedia(char *filename, char *out)
{ Write1xMedia(filename,TRUE); return(0);}*/
/*(int WrapWrite1xMediaNoWave(char *filename, char *out)
{ Write1xMedia(filename,FALSE); return(0); }*/
int Wrap1MkSimpFX(char *filename, char *out)
{ return(MkSimpFX(filename,"1")); }
int Wrap2MkSimpFX(char *filename,char *out)
{ return(MkSimpFX(filename,"2")); }
int WrapIMAMkSimpFX(char *filename,char *out)
{ return(MkSimpFX(filename,"IMA"));}
int Wrap1MkSimple(char *filename, char *out)
{ return(MkSimple(filename,1)); }
int Wrap2MkSimple(char *filename, char *out)
{ return(MkSimple(filename,2)); }
int WrapWrite2xMedia(char *filename,char *out)
{ return(Write2xMedia(filename, kOmfRev2x)); }
int WrapWrite2xMedia1xFile(char *filename,char *out)
{ return(Write2xMedia(filename, kOmfRev1x)); }
int WrapWrite2xMediaIMAFile(char *filename,char *out)
{ return(Write2xMedia(filename, kOmfRevIMA)); }
int WrapCheck2xMedia(char *filename)
{ return(Check2xMedia(filename, kOmfRev2x)); }
int WrapCheck2xMedia1x(char *filename)
{ return(Check2xMedia(filename, kOmfRev1x)); }
int WrapCheck2xMediaIMA(char *filename)
{ return(Check2xMedia(filename, kOmfRevIMA)); }
int Wrap1VideoWriteTestFile(char *filename,char *out)
{ VideoWriteTestFile(filename,"1");  return(0);}
int Wrap2VideoWriteTestFile(char *filename,char *out)
{ VideoWriteTestFile(filename,"2");  return(0);}
int WrapIMAVideoWriteTestFile(char *filename,char *out)
{ VideoWriteTestFile(filename,"IMA");  return(0);}
int WrapVideoCheckTestFile(char *filename)
{ VideoCheckTestFile(filename); return(0); }
int WrapMkOMFLoc(char *input, char *output)
{ return(MkOMFLoc(input)); }
int Wrap1DupMobs(char *input, char *output)
{ return(DupMobs(input,"1")); }
int WrapIMADupMobs(char *input, char *output)
{ return(DupMobs(input,"IMA")); }
int Wrap2DupMobs(char *input, char *output)
{ return(DupMobs(input,"2")); }
int WrapRawLoc(char *input, char *output)
{ return(MkRawLoc(input)); }
int Wrap1ManyObjs(char *input, char *output)
{ return(ManyObjs(input,"1")); }
int WrapIMAManyObjs(char *input, char *output)
{ return(ManyObjs(input,"IMA")); }
int Wrap2ManyObjs(char *input, char *output)
{ return(ManyObjs(input,"2")); }
int Wrap1PrivReg(char *input, char *output)
{ return(PrivReg(input,"1")); }
int WrapIMAPrivReg(char *input, char *output)
{ return(PrivReg(input,"IMA")); }
int Wrap2PrivReg(char *input, char *output)
{ return(PrivReg(input,"2")); }
int Wrap1Patch1x(char *input, char *output)
{ return(Patch1x(input,"1")); }
int WrapIMAPatch1x(char *input, char *output)
{ return(Patch1x(input,"IMA")); }
int Wrap1MkFilm(char *filename, char *out)
{ return(MkFilm(filename,"1")); }
int Wrap2MkFilm(char *filename, char *out)
{ return(MkFilm(filename,"2")); }
int WrapIMAMkFilm(char *filename, char *out)
{ return(MkFilm(filename,"IMA")); }
int Wrap1TstAttr(char *filename, char *out)
{ return(TstAttr(filename,"1")); }
int Wrap2TstAttr(char *filename, char *out)
{ return(TstAttr(filename,"2")); }
int WrapIMATstAttr(char *filename, char *out)
{ return(TstAttr(filename,"IMA")); }
int WrapTestCodecs(char *filename, char *out)
{ return(TestCodecs()); }
int WrapCompIter(char *filename, char *out)
{ return(TestCompIter(filename)); }

/***************************/
/*   Utility Functions     */
/***************************/

/* Banners for the beginning of each test.... */
static void
print_header(int testnum)
{
  Test2Run_t *cur = &TestTable[testnum];

  printf("\n\n======================================================\n");
  printf("TEST %s, TEST NUMBER %d\n",cur->name,testnum);
  if (cur->comments)
    printf("Description: This test %s\n",cur->comments);
  else 
    printf("Description: None.\n");
  if (cur->results)
    printf("Expected Results: %s\n",cur->results);
  else
    printf("Expected Results: No Errors (may print some diag messages).\n");
  if (cur->input)
    printf("Input file: %s\n",cur->input);
  else
    printf("Input file: None \n");
  if (cur->output)
    printf("Output file: %s\n",cur->output);
  else
    printf("Output file: None\n");
  printf("=======================================================\n");
}

/* static int copy_file(char *source, char *dest)     */
/* utility routine for copying files on all platforms. */
/* The idea is that it is used as a test, but doesn't actually test OMF information */
static int
copy_file(char *source, char *dest)
{
   FILE *source_fp, *dest_fp;
   char *buf;
   int size=1, nitems=32000, result=1;

   source_fp = fopen(source, "rb");
   dest_fp = fopen(dest,"wb");
   buf = (char *) omfsMalloc ( nitems * sizeof(char *));
   if(buf == NULL)
     {
       printf("UnitTest: Out of memory for copy\n");
       return(1);
     }
   	
   if (source_fp == NULL)
     {
       printf("UnitTest: Couldn't open source file %s for reading\n",source);
       return(1);
     }
   if (dest_fp == NULL)
     {
       printf("UnitTest: Couldn't open destination file %s for writing\n",dest);
       return(1);
     }

   while (result)
     { 
       result = fread(buf, size, nitems, source_fp);
       if(result != 0)
	 if (fwrite(buf, size, result, dest_fp) == 0)
	   printf("UnitTest: Failed to write data to file %s\n",dest);
     }

   fclose(source_fp);
   fclose(dest_fp);
   omfsFree(buf);
  
   return(0);
}

static int lookup_test(char *name)
{
  int i, found = 0;

  if (NUMTESTS == 0) {
    printf("No files in Testing Table? Did you call init_table\n?");
    exit(1);
  }
  
  if (!name) {
    printf("lookup_test() called with null testname!\n");
    exit(1);
  }
  
  for (i=1; (i<=NUMTESTS && found == 0) ; i++) 
    if (!strcmp(TestTable[i].name,name)) found = i;
  
  return(found);
}

static int print_table()
{
  int i;

  printf("The following tests are available:\n");  
  for (i=1; i<=NUMTESTS-3; i+=3) {
    printf("[%2d] %-18.15s [%2d] %-18.15s [%2d] %-18.15s\n",i,TestTable[i].name,i+1,
	   TestTable[i+1].name,i+2,TestTable[i+2].name);
  }
 /* Now print the last few parts of the array....if needed */
  if ((NUMTESTS - i) == 2)
    printf("[%2d] %-18.15s [%2d] %-18.15s [%2d] %-18.15s\n",i,TestTable[i].name,i+1,
	   TestTable[i+1].name,i+2,TestTable[i+2].name);
  if ((NUMTESTS - i) == 1)
    printf("[%2d] %-18.15s [%2d] %-18.15s\n",i,TestTable[i].name,i+1,
	   TestTable[i+1].name);
  if ((NUMTESTS - i) == 0)
    printf("[%2d] %-18.15s\n",i,TestTable[i].name);
  
  printf("Please use the index numbers to select the tests you want to run.\n");
  exit(0);
  return(0);  /* obviously this is kinda bogus, but it shuts up CW */
}

/***************************/
/* Initialization Routines */
/***************************/

static int
usage()
{
  printf("UnitTest usage:  UnitTest [-vVrR][-uU][-pP][[-fF] Test Test1 ...]\n");
  printf("     UnitTest with no args runs & verifies all tests        \n");
  printf("     UnitTest with -u or -U prints this message             \n");
  printf("     UnitTest with -v,-V,-r,-R does the verify only phase of all tests \n");
  printf("     UnitTest with -p or -P prints the entire test table with index numbers \n");
  printf("     UnitTest with test numbers, will run the selected tests, after running \n");
  printf("          any tests that the ones that you've selected depend on.  \n");
  printf("     UnitTest with -f or -F followed by one or more test numbers will \n");
  printf("          only run the test(s) specified (don't run dependancies).\n"); 
  printf("\n");

  exit(0);
  return(0);  /* obviously this is kinda bogus, but it shuts up CW */
}

static int
update_dependencies()
{
  int i, tmp;
  int run_again = 1;
 
  while (run_again) 
    {
      run_again = 0;
      for (i = 1; i <= NUMTESTS; i++) 
	{
	  if (RunTheseTests[i])
	    {
	      if (TestTable[i].depend[0] != '\0') 
		{
		  tmp = lookup_test(TestTable[i].depend);
		  if (tmp > i) {
		    printf("***> ERROR: Dependencies out of order, Test %s depends on Test %s\n",
			   TestTable[i].name,TestTable[i].depend); 
		    exit(1);
		  }
		  if (!(RunTheseTests[tmp]))
		    {
		      RunTheseTests[tmp] = 1;
		      run_again = 1;
		    }
		}
	    }
	}
    }
    return(0);
}

static int
check_args(char **argv,int argc,int *run,int *verify,int *individual)
{
  int tmp,i,ForceNoDependancies = 0;

  if (argc == 1) 
    {
      *run = 1;
      return(0);
    }
  if (!strcmp(argv[1],"-v")) {
    *verify = 1;
    return(0);
  }
  else if (!strcmp(argv[1],"-V")) {
    *verify = 1;
    return(0);
  }
  else if (!strcmp(argv[1],"-r")) {
    *verify = 1;
    return(0);
  }
  else if (!strcmp(argv[1],"-R")) {
    *verify = 1;
    return(0);
  }
  else if (!strcmp(argv[1],"-u"))
    usage();
  else if (!strcmp(argv[1],"-U"))
    usage();
  else if (!strcmp(argv[1],"-p"))
    print_table();
  else if (!strcmp(argv[1],"-P"))
    print_table();
  else if (!strcmp(argv[1],"-f"))
    ForceNoDependancies = 1;
  else if (!strcmp(argv[1],"-F"))
    ForceNoDependancies = 1;

  /* We should only get here if there are numbers to get from the command line */
  for (i = 1; i < argc; i++)
    {
      if (strncmp(argv[i],"-f",2)) {
	if (strncmp(argv[i],"-F",2)) {
      	  if (!strncmp(argv[i],"-",1))
	    { 
	      printf("Unknown arg %s\n",argv[1]);
	      usage();
	    }
          tmp = atoi(argv[i]);
          if ((tmp > NUMTESTS) || (tmp <= 0)) 
          {
            printf("Test number requested (%d) is out of range.\n",tmp);
            print_table();
          }
          RunTheseTests[tmp] = 1;  /* turn on flag */
          *individual = 1;
	  run_only_some_tests = 1;
        } 
      }
    }
  if (!ForceNoDependancies)
    update_dependencies();
  return(0);
}

/* add2table */
/* This function creates a dynamically created array of tests to run */

static int add2table(char *name, char *depend, proc2_t runtest,
		     proc1_t verify, char *input, char *output,
		     char *comments, char *results, testType_t mode)
{
  Test2Run_t *cur;
  int i;

    i = NUMTESTS++;       /* When the Tests have all been added to the array, this is
			     the number of TESTS in the array */

  if (i >= MAXTESTS) 
    {
      printf("Maximum number of tests allowed in table exceeded.\n");
      printf("Up the number of MAXTESTS, recompile, and run again.\n");
      exit(1);
    }

  cur = &TestTable[i];    /* use local pointer for simplicity */

  if (!name)              /* Name for the test */
    {
      printf("Error test must have a name.\n");
      exit(1);
    }
  if ((strlen(name)) >= 25)
      name[24] = '\0';       /* null terminate to 25 length */
  strcpy( (char*) cur->name, name);
  
  if (depend)             /* Does this test depend on the output of another? which one? */
    {
      if ((strlen(depend)) >= 25)
	depend[24] = '\0';
      strcpy( (char*) cur->depend, depend);
    }
  
  if (runtest)               /* The function ptr that is the test function */
    cur->runtest = runtest;
  if (verify)               /* The function ptr that does the verify of the run */
    cur->verify = verify;
  
  if (input)                /* input filename for the runtest */
    {
      if (strlen(input) >= 64)
	{
	  printf("Input filename too int  on test %s\n",name);
	  exit(1);
	}
      strcpy( (char*) cur->input,input);
    }
  if (output)              /* output filename for the runtest */
    {
      if (strlen(output) >= 64)
	{
	  printf("Input filename too int  on test %s\n",name);
	  exit(1);
	}
      strcpy( (char*) cur->output,output);
    }

  cur->mode = mode;        /* mode used for telling what kind of test this is */

  if (comments) 
    {
      cur->comments = malloc(sizeof(char)*(strlen(comments)+1));
      strcpy( (char*) cur->comments,comments);
    }
  else
	  cur->comments = NULL;
  if (results)
    {
      cur->results = malloc(sizeof(char)*(strlen(results)+1));
      strcpy( (char*) cur->results,results);
    }
  else
	  cur->results = NULL;
      
  return(0);
}

/* init_table is where all the TestTable set up is done.  It creates an array */
/* of Test2Run_t structs */

static int init_table() 
{
  int i;
/* add2table(testname, depend, testrun, verifytest, 
	    inputfile, outputfile, 
	    comments,
	    results,mode) */

/* First we test the 1.x stuff, then IMA, then 2.x */

  /*** 1.x Tests ***/
/*  add2table("Write1xMedia",NULL,WrapWrite1xMedia,WrapCheck1xMedia,
	    "WrtMedia.omf",NULL,
	    "creates an OMF file with Media (1.x)",
	    NULL,kOmPosTest);*/
  add2table("MkSimple1x",NULL,Wrap1MkSimple,ReadComp,
	    "Simple1x.omf",NULL,
	    "creates a simple OMF comp file (ReadComp verifies) (1.x)",
	    NULL,kOmPosTest);
  add2table("WriteComplex1x",NULL,Wrap1MkCplx,ReadComp,
	    "Complx1x.omf",NULL,
	    "creates a complex OMF comp file (ReadComp verifies) (1.x)",
	    NULL,kOmPosTest);
  add2table("FndSrcComplex1x","WriteComplex1x",WrapFndSrc,NULL,
	   "Complx1x.omf",NULL,
	   "runs FndSrc on the file Complx1x.omf (1.x)",
           NULL,kOmNegTest);
  add2table("cp_openMod1x","WriteComplex1x",copy_file,NULL,
	    "Complx1x.omf","OpnMod1x.omf",
	    "copies Complx1x.omf to OpnMod1x.omf (1.x)",
	    NULL,kOmPosTest);
  add2table("OpenMod","cp_openMod1x",WrapOpenMod,NULL,
	    "OpnMod1x.omf",NULL,
	    "open for Modify (with the 2x Toolkit) a 1x OMF file (1.x)",
	    NULL,kOmPosTest); 
  add2table("MkSimpFX1x",NULL,Wrap1MkSimpFX,DumpFXD,
	    "SimpFX1x.omf",NULL,
	    "creates a simple effects file (1.x)",
	    "Verfication with DumpFXD",kOmPosTest);
  add2table("cp_DelMob1x","WriteComplex1x",copy_file, NULL,
	    "Complx1x.omf","DelMob1x.omf",
	    "copies file Complx1x.omf to DelMob1x.omf (1.x)",
	    NULL,kOmPosTest);
  add2table("DelMob1x","cp_DelMob1x",WrapDelMob,NULL,
	    "DelMob1x.omf",NULL,
            "deletes all the Mobs in the file (1.x)",
            NULL,kOmPosTest);
  /*add2table("Media1xNoWave",NULL,WrapWrite1xMediaNoWave,WrapCheck1xMediaNoWave,
	    "Med1xNoW.omf",NULL,
	    "creates a MC friendly media file (1.x)",
	    NULL,kOmPosTest);*/
  add2table("cp_copyMob1x","WriteComplex1x",copy_file,NULL,
	    "Complx1x.omf","CpMob1x.omf",
	    "copies file Complx1x.omf to file CpMob1x.omf (1.x)",
	    NULL,kOmPosTest);
  add2table("CopyMob1x","cp_copyMob1x",WrapCopyMob,NULL,
	    "CpMob1x.omf",NULL,
	    "creates a copy of a Mob inside the file (1.x)",
	    NULL,kOmPosTest);
  add2table("CopyMobX1x","WriteComplex1x",CopyMobX,NULL,
            "Complx1x.omf","CpMobX1x.omf",
            "copies a Mob and all of it's decendents to an external file (1.x)",
	    NULL,kOmPosTest);
  add2table("cp_changeMob1x","WriteComplex1x",copy_file,NULL,
	    "Complx1x.omf","ChgMob1x.omf",
	    "copies the file Complx1x.omf to ChgMob1x.omf (1.x)",
	    NULL,kOmPosTest);
  add2table("ChangeMob1x","cp_changeMob1x",WrapChgMob,NULL,
	    "ChgMob1x.omf",NULL,
	    "modifies one of the mob IDs in the file ChgMob1x.omf (1.x)",
	    NULL,kOmPosTest);
  add2table("CloneMobX1x","WriteComplex1x",CloneMobX,NULL,
	    "Complx1x.omf","ClnMbX1x.omf",
	    "clones mobs from a file, and places them in an external file (1.x)",
	    NULL,kOmPosTest);
  add2table("DumpDK1x","WriteComplex1x",WrapDumpDK,NULL,
	    "Complx1x.omf",NULL,
	    "verifies datakinds in an OMF file (1.x)",
	    NULL,kOmNegTest);
  add2table("ObjIter1x","WriteComplex1x",WrapObjIter,NULL,
	    "Complx1x.omf",NULL,
	    "uses the iterators to look at an OMF file (1.x)",
	    NULL,kOmNegTest);
  add2table("MkTape1x",NULL,Wrap1VideoWriteTestFile,WrapVideoCheckTestFile,
	    "MkTape1x.omf",NULL,
	    "creates a file with VideoTape references in it (1.x)",
	    NULL,kOmPosTest);
  add2table("TapeFndSrc1x","MkTape1x",WrapFndSrc,NULL,
	    "MkTape1x.omf",NULL,
	    "verifies MkTape1x with Find Source (1.x)",
	    NULL,kOmNegTest);
  add2table("DupMobs1x",NULL,Wrap1DupMobs,NULL,
	    "DupMob1x.omf",NULL,
            "creates a file with duplicate mobs in it, and verifies itself (1.x)",
	    NULL,kOmNegTest);
  /*add2table("Attr1x",NULL,WrapAttr1x,NULL,
	    "Attr1x.omf",NULL,
	    "creates a file with attributes (1.x API)",
	    "Attribute name is: Lisa's attribute",kOmPosTest);*/
  add2table("Media2xFile1x",NULL,WrapWrite2xMedia1xFile,WrapCheck2xMedia1x,
	    "M2xF1x.omf", NULL,
	    "Writes 2x media to a 1x file as private types",
	    NULL,kOmPosTest);
  add2table("ManyObjs1x",NULL,Wrap1ManyObjs,ReadComp,
	    "ManyO1x.omf",NULL,
	    "creates a 1.x file with with many objects (ReadComp verifies) (1.x)",
	    NULL,kOmPosTest);
  add2table("PrivReg1x",NULL,Wrap1PrivReg,ReadComp,
	    "PrivR1x.omf",NULL,
	    "creates a 1.x file with private props (ReadComp verifies) (1.x)",
	    NULL,kOmPosTest);
  add2table("Patch1x",NULL,Wrap1Patch1x,ReadComp,
	    "Patch1x.omf",NULL,
	    "tests omfsPatch1xMobs (ReadComp verifies) (1.x)",
	    NULL,kOmPosTest);
  add2table("MkFilm1x",NULL,Wrap1MkFilm,ReadComp,
	    "Film1x.omf",NULL,
	    "Generates a film composition with media (ReadComp verifies) (1.x)",
	    NULL,kOmPosTest);
  add2table("TstAttr1x",NULL,Wrap1TstAttr,NULL,
	    "TstAttr1x.omf",NULL,
	    "Generates a file of containing private attributes and mob comments (1.x)",
	    NULL,kOmPosTest);

/*** Test the IMA interface ***/

  add2table("WriteComplexIMA",NULL,WrapIMAMkCplx,ReadComp,
	   "CmplxIMA.omf",NULL,
	   "creates a complex OMF comp file (ReadComp verifies) (IMA)",
	   NULL,kOmPosTest);
  add2table("FndSrcComplexIMA","WriteComplexIMA",WrapFndSrc,NULL,
	   "CmplxIMA.omf",NULL,
	   "runs FndSrc on the file CmplxIMA.omf (IMA)",
           NULL,kOmNegTest);
  add2table("cp_openModIMA","WriteComplexIMA",copy_file,NULL,
	    "CmplxIMA.omf","OpModIMA.omf",
	    "copies CmplxIMA.omf to OpModIMA.omf (IMA)",
	    NULL,kOmPosTest);
  add2table("OpenModIMA","cp_openModIMA",WrapOpenMod,NULL,
	    "OpModIMA.omf",NULL,
	    "open for Modify (with the 2x Toolkit a IMA OMF file (IMA)",
	    NULL,kOmPosTest);
  add2table("MkSmpIMA",NULL,WrapIMAMkSimpFX,DumpFXD,
	    "MkSmpIMA.omf",NULL,
	    "creates a simple effects file (IMA)",
	    "Verfication with DumpFXD",kOmPosTest);
  add2table("cp_DelMobIMA","WriteComplexIMA",copy_file,NULL,
	    "CmplxIMA.omf","DelMbIMA.omf",
            "copies file CmplxIMA.omf to DelMbIMA.omf (IMA)",
	    NULL,kOmPosTest);
  add2table("DelMobIMA","cp_DelMobIMA",WrapDelMob,NULL,
	    "DelMbIMA.omf",NULL,
	    "deletes all of the Mobs in the file (IMA)",
	    NULL,kOmPosTest);
  add2table("cp_copyMobIMA","WriteComplexIMA",copy_file,NULL,
	    "CmplxIMA.omf","CpMobIMA.omf",
	    "copies the file CmplxIMA.omf to the file CpMobIMA.omf (IMA)",
	    NULL, kOmPosTest);
  add2table("CopyMobIMA","cp_copyMobIMA",WrapCopyMob,NULL,
	    "CpMobIMA.omf",NULL,
	    "creates a copy of a Mob inside the file (IMA)",
	    NULL, kOmPosTest);
  add2table("CopyMobXIMA","WriteComplex1x",CopyMobX,NULL,
	    "CmplxIMA.omf","CpMbXIMA.omf",
	    "copies a Mob and all of it's decendents to an external file (IMA)",
	    NULL,kOmPosTest);
  add2table("cp_changeMobIMA","WriteComplexIMA",copy_file,NULL,
	    "CmplxIMA.omf","ChgMbIMA.omf",
	    "copies the file CmplxIMA.omf to the file ChgMbIMA.omf",
	    NULL,kOmPosTest);
  add2table("ChangeMobIMA","cp_changeMobIMA",WrapChgMob,NULL,
	    "ChgMbIMA.omf",NULL,
	    "modifies on of the mob IDs in the file ChgMbIMA.omf (IMA)",
	    NULL,kOmPosTest);
  add2table("CloneMobXIMA","WriteComplexIMA",CloneMobX,NULL,
	    "CmplxIMA.omf","ClnMXIMA.omf",
	    "clones mobs from a file, and places them in an external file (IMA)",
	    NULL,kOmPosTest);
  add2table("DumpDKIMA","WriteComplexIMA",WrapDumpDK,NULL,
	    "CmplxIMA.omf",NULL,
	    "verifies datakinds in an OMF file (IMA)",
	    NULL,kOmNegTest);
  add2table("ObjIterIMA","WriteComplexIMA",WrapObjIter,NULL,
	    "CmplxIMA.omf",NULL,
	    "uses the iterators to look at an OMF file (IMA)",
	    NULL,kOmNegTest);
  add2table("MkTapeIMA",NULL,WrapIMAVideoWriteTestFile,WrapVideoCheckTestFile,
	    "MkTpeIMA.omf",NULL,
	    "creates a file with VideoTape references in it (IMA)",
	    NULL,kOmPosTest);
  add2table("TapeFndSrcIMA","MkTapeIMA",WrapFndSrc,NULL,
	    "MkTpeIMA.omf",NULL,
	    "verifies MkTapeIMA.omf with Find Source (IMA)",
	    NULL,kOmNegTest);
  add2table("DupMobsIMA",NULL,WrapIMADupMobs,NULL,
	    "DupMbIMA.omf",NULL,
            "creates a file with duplicate mobs in it, and verifies itself (IMA)",
	    NULL,kOmNegTest);
  add2table("Media2xFileIMA",NULL,WrapWrite2xMediaIMAFile,WrapCheck2xMediaIMA,
	    "M2xFIMA.omf", NULL,
	    "Writes 2x media to a IMA file as private types",
	    NULL,kOmPosTest);

  add2table("ManyObjsIMA",NULL,WrapIMAManyObjs,ReadComp,
	    "ManyOIMA.omf",NULL,
	    "creates an IMA file with many objects (ReadComp verifies) (IMA)",
	    NULL,kOmPosTest);
  add2table("PrivRegIMA",NULL,WrapIMAPrivReg,ReadComp,
	    "PrivRIMA.omf",NULL,
	    "creates an IMA file with private props (ReadComp verifies) (IMA)",
	    NULL,kOmPosTest);
  add2table("PatchIMA",NULL,WrapIMAPatch1x,ReadComp,
	    "PatchIMA.omf",NULL,
	    "tests omfsPatch1xMobs (ReadComp verifies) (IMA)",
	    NULL,kOmPosTest);
  add2table("MkFilmIMA",NULL,WrapIMAMkFilm,ReadComp,
	    "FilmIMA.omf",NULL,
	    "Generates a film composition with media (ReadComp verifies) (IMA)",
	    NULL,kOmPosTest);
  add2table("TstAttrIMA",NULL,WrapIMATstAttr,NULL,
	    "TstAttrIMA.omf",NULL,
	    "Generates a file of containing private attributes and mob comments (IMA)",
	    NULL,kOmPosTest);

/*** Now test all the 2.x stuff ***/

  add2table("MkComp2x",NULL,WrapMkComp2x,ReadComp,
	    "MkComp2x.omf",NULL,
	    "creates an OMF composition file (2.x)",
	    "ReadComp verifies",kOmPosTest);
  add2table("CopyMobX","MkComp2x",CopyMobX,NULL,
	    "MkComp2x.omf","CopyMobX.omf",
	    "copies a MOB and all it's decendents to an external file (2.x)",
	    NULL,kOmPosTest); 
  add2table("cp_copyMob","MkComp2x",copy_file,NULL,
	    "MkComp2x.omf","CopyMob.omf",
	    "copies MkComp2x.omf to CopyMob.omf (2.x)",
	    NULL,kOmPosTest);
  add2table("CopyMob","cp_copyMob",WrapCopyMob,NULL,
	    "CopyMob.omf",NULL,
	    "creates a copy of a Mob inside the file CopyMob.omf (2.x)",
	    NULL,kOmPosTest);
  add2table("CloneMobX","MkComp2x",CloneMobX,NULL,
	    "MkComp2x.omf","ClneMobX.omf",
	    "creates a clone of a Mob and places it in an external file (2.x)",
	    NULL,kOmPosTest);
  add2table("NgCloneX","MkComp2x",NgCloneX,NULL,
	    "MkComp2x.omf","NgCloneX.omf",
	    "is a negative test to verify OM_ERR_FILEREV_DIFF error code (2.x)",
	    NULL,kOmNegTest);
  add2table("cp_changeMob","MkComp2x",copy_file,NULL,
	    "MkComp2x.omf","ChngeMob.omf",
	    "copies MKComp2x.omf to ChngeMob.omf (2.x)",
	    NULL,kOmPosTest);
  add2table("ChangeMob","cp_changeMob",WrapChgMob,NULL,
	    "ChngeMob.omf",NULL,
	    "modify one of the Mobs found in the file ChngeMob.omf (2.x)",
	    NULL,kOmPosTest);
  add2table("cp_DelMob","MkComp2x",copy_file,NULL,
	    "MkComp2x.omf","DelMob.omf",
	    "copies MkComp2x.omf to DelMob.omf (2.x)",
	    NULL,kOmPosTest);
  add2table("DelMob","cp_DelMob",WrapDelMob,NULL,
	    "DelMob.omf",NULL,
	    "deletes all the Mobs found in the file DelMob.omf (2.x)",
	    NULL,kOmPosTest);
  add2table("FndSrc","MkComp2x",WrapFndSrc,NULL,
	    "MkComp2x.omf",NULL,
	    "prints out all a Tape Mob and it's offsets and TimeCode (2.x)",
	    NULL,kOmPosTest);
  add2table("IterSrch","MkComp2x",WrapIterSrch,NULL,
	    "MkComp2x.omf",NULL,
	    "exercises the iterators (2.x)",
	    NULL,kOmPosTest);
  add2table("ObjIter","MkComp2x",WrapObjIter,NULL,
	    "MkComp2x.omf",NULL,
	    "exercises the object iterators (2.x)",
	    NULL,kOmNegTest);
  add2table("MkIlleg",NULL,WrapMkIlleg,NgFndSrc,
	    "WriteIll.omf",NULL,
	    "writes an illegal OMF file!",
	    "NgFndSrc verifies that has MOBS that don't point to anything.\n  Semantic check is not run (2.x)",
	    kOmNegTest);
  add2table("DumpDK","MkComp2x",WrapDumpDK,NULL,
	    "MkComp2x.omf",NULL,
	    "verifies datakinds in an OMF file (2.x)",
	    "Can't find omfi:lisa datakind (and diags)",kOmNegTest);
  add2table("DumpFXD","MkComp2x",WrapDumpFXD,NULL,
	    "MkComp2x.omf",NULL,
	    "looks up and prints any effects in the specified file (2.x)",
	    NULL,kOmPosTest);
  add2table("ERCvt",NULL,WrapERCvt,NULL,
	    NULL,NULL,
	    "verifies the editrate conversions are correct (2.x Toolkit)",
	    "Prints out various editrates, the toolkit's conversion of them.",
	    kOmPosTest);
  add2table("TCCvt",NULL,WrapTCCvt,NULL,
	    NULL,NULL,
	    "verifies the time code conversions are correct (2.x Toolkit)",
	    "Prints out some time codes, and the toolkit's conversion of them.",
	    kOmPosTest);
  add2table("MkSimple2x",NULL,Wrap2MkSimple,ReadComp,
	    "Simple2x.omf",NULL,
	    "creates a simple OMF comp file (ReadComp verifies) (2.x)",
	    NULL,kOmPosTest);
  add2table("WriteComplex2x",NULL,Wrap2MkCplx,ReadComp,
	    "Complx2x.omf",NULL,
	    "creates a complex OMF comp file (ReadComp verifies) (2.x)",
	    NULL,kOmPosTest);
  add2table("FndSrcComplex2x","WriteComplex2x",WrapFndSrc,NULL,
	   "Complx2x.omf",NULL,
	   "runs FndSrc on the file Complx2x.omf (2.x)",
	    NULL,kOmNegTest);
  add2table("MkDesc2x",NULL,WrapMkDesc2,NULL,
	    "MkDesc2x.omf",NULL,
	    "writes a FILM and TAPE mob and verifies them (2.x)",
	    NULL,kOmPosTest);
  add2table("MkSimpFX2x",NULL,Wrap2MkSimpFX,DumpFXD,
	    "SimpFX2x.omf",NULL,
	    "creates a simple effects file (2.x)",
	    NULL,kOmPosTest);
  add2table("Write2xMedia",NULL,WrapWrite2xMedia,WrapCheck2xMedia,
	    "Media2x.omf",NULL,
	    "creates a file with Media in it (2.x)",
	    NULL,kOmPosTest);
  add2table("MkTape2x",NULL,Wrap2VideoWriteTestFile,WrapVideoCheckTestFile,
	    "MkTape2x.omf",NULL,
	    "creates a file with VideoTape references in it (2.x)",
	    NULL,kOmPosTest);
  add2table("TapeFndSrc2x","MkTape2x",WrapFndSrc,NULL,
	    "MkTape2x.omf",NULL,
	    "verifies MkTape2x.omf with Find Source (2.x)",
	    NULL,kOmNegTest);
  add2table("MediaLocators",NULL,WrapMkOMFLoc,NULL,
	    "MkOMFLoc.omf",NULL,
	    "creates a file with one of each kind of media locators (2.x)",
	    NULL,kOmPosTest);
  add2table("DupMobs2x",NULL,Wrap2DupMobs,NULL,
	    "DupMob2x.omf",NULL,
            "creates a file with duplicate mobs in it, and verifies itself (2.x)",
	    NULL,kOmNegTest);
  add2table("RawIO",NULL,WrapRawLoc,NULL,
	    "rawRef.omf", NULL,
	    "Write native media non-OMFI, then open and compare.",
	    NULL,kOmPosTest);

  add2table("ManyObjs2x",NULL,Wrap2ManyObjs,ReadComp,
	    "ManyO2x.omf",NULL,
	    "creates a 2.x file with with many objects (ReadComp verifies) (2.x)",
	    NULL,kOmPosTest);
  add2table("PrivReg2x",NULL,Wrap2PrivReg,ReadComp,
	    "PrivR2x.omf",NULL,
	    "creates a 2.x file with private props (ReadComp verifies) (2.x)",
	    NULL,kOmPosTest);
  add2table("MkFilm2x",NULL,Wrap2MkFilm,ReadComp,
	    "Film2x.omf",NULL,
	    "Generates a film composition with media (ReadComp verifies) (2.x)",
	    NULL,kOmPosTest);
  add2table("TstAttr2x",NULL,Wrap2TstAttr,NULL,
	    "TstAttr2x.omf",NULL,
	    "Generates a file of containing private attributes and mob comments (2.x)",
	    NULL,kOmPosTest);
  add2table("CompIter",NULL,WrapCompIter,NULL,
	    "CompIter.omf",NULL,
	    "Test the composition iterators with complex nested scopes",
	    NULL,kOmPosTest);

/*** Convert Tests, by their nature they are mixed 1.x and 2.x... ***/

  add2table("Convert_1x_to_2x","OpenMod1x",Convert,NULL,
	    "OpnMod1x.omf","CvtMod2x.omf",
	    "converts a 1x format file to 2x format (2.x)",
	    NULL,kOmPosTest);
  add2table("Convertback_2x_to_1x","Convert_1x_to_2x",Convert,NULL,
	    "CvtMod2x.omf","CvtBck1x.omf",
	    "converts the 2x file we just created back into a 1x file (1.x)",
	    NULL,kOmPosTest);
  add2table("Convert_2x_to_1x","WriteComplex2x",Convert,NULL,
	    "Complx2x.omf","CvtComp1.omf",
	    "converts a 2x complex composition into a 1x file (1.x)",
	    NULL,kOmPosTest);
  add2table("ConvertMedia_2x_to_1x","Write2xMedia",Convert,NULL,
	    "Media2x.omf","Cvt2xMed.omf",
	    "converts a 2x media file into a 1x media file (1.x)",
	    NULL,kOmPosTest);
  add2table("Cnvrt1xbck2x","ConvertMedia_2x_to_1x",Convert,NULL,
	    "Cvt2xMed.omf","CvtB2xMd.omf",
	    "converts the 1x media file back into a 2x media file (2.x)",
	    NULL,kOmPosTest);
  add2table("Cnvrt1xFX2x","MkSimpFX1x",Convert,NULL,
	    "SimpFX1x.omf","FX1xto2x.omf",
	    "converts a 1x FX file into a 2x FX file (2.x)",
	    NULL,kOmPosTest);
  add2table("Cnvt2xFX1x","MkSimpFX2x",Convert,NULL,
	    "SimpFX2x.omf","FX2xto1x.omf",
	    "converts a 2x FX file into a 1x FX file (1.x)",
	    NULL,kOmPosTest);            

/*** Other tests which mix revisions ***/
  add2table("TstCodec",NULL,WrapTestCodecs,NULL,
	    NULL,NULL,
	    "Veriifies the workings of the codec user selection functions",
	    "Prints out the complete list of codecs and their varieties.",
	    kOmPosTest);

  /* Now initialize the RunTheseTests & TheseTestsFailed */
  for (i=1; i<MAXTESTS; i++) 
    {
      RunTheseTests[i] = 0;
      TheseTestsFailed[i] = 0;
    }
  
  return(0);
}

/***************************/
/* Work Horse Routines     */
/***************************/

/*Semantic checking routine.  This is a boiled down version of the omfCheck routines */
static int semantic_check(char *filename,int i)
{
  omfSessionHdl_t session;
  omfHdl_t fp;
  omfErr_t omfError1 = OM_ERR_NONE;
  omfErr_t omfError2 = OM_ERR_NONE;
  omfFileCheckHdl_t hdl;
  omfInt32 err1=0,warn1=0,err2=0,warn2=0;
  
  XPROTECT(NULL)
    {
      CHECK(omfsBeginSession(NULL,&session));
      CHECK(omfmInit(session));
#if AVID_CODEC_SUPPORT
	  	CHECK(omfmRegisterCodec(session, omfCodecAvJPED, kOMFRegisterLinked));
	  	CHECK(omfmRegisterCodec(session, omfCodecAvidJFIF, kOMFRegisterLinked));
#endif
      CHECK(omfsOpenFile((fileHandleType) filename,session, &fp));
      CHECK(omfsFileCheckInit(fp,&hdl));
      omfError1 = 
	omfsFileCheckObjectIntegrity(
				    hdl,
				    (gVerbose ? kCheckVerbose : kCheckQuiet),
				    (gWarnings ? kCheckPrintWarnings : kCheckNoWarnings),
				     &err1, &warn1, stdout);
      if (omfError1)
	printf("***> ERROR: Object Integrity Check failed.\n");
      if (err1 || warn1)
	printf("***>Found %ld errors and %ld warnings in pass1 semantic check\n",
	       err1,warn1);
      omfError2 =
	omfsFileCheckMobData(hdl,
			     (gVerbose ? kCheckVerbose : kCheckQuiet),
			     (gWarnings ? kCheckPrintWarnings : kCheckNoWarnings),
			     &err2, &warn2, stdout);
      if (omfError2)
	printf("***> ERROR: Mob Data Check failed.\n");
      if (err2 || warn2)
	printf("***> Found %ld errors and %ld warnings in pass2 semantic check\n",
	       err2,warn2);
      TotalErrors = TotalErrors + err1 + err2;
      TotalWarnings = TotalWarnings + warn1 + warn2;
      CHECK(omfsFileCheckCleanup(hdl));
      CHECK(omfsCloseFile(fp));
      CHECK(omfsEndSession(session));
    
  } /* XPROTECT */
  XEXCEPT
    {
      if (XCODE() != OM_ERR_NO_MORE_OBJECTS){
	printf("***> ERROR: %d: %s\n",XCODE(),omfsGetErrorString(XCODE()));
	TheseTestsFailed[i] = 1;
	gSemCheckFaults++;
	return(1);
      }
      else
	printf("Semantic Check completed successfully\n");
    }
  XEND;

  if ((omfError1 != 0) || (omfError2 != 0))
    {
      TheseTestsFailed[i] = 1;
      gSemCheckFaults++;
      return(1);
    }

  if (err1 || warn1 || err2 || warn1)
    {
      TheseTestsFailed[i] = 1;
      gSemCheckFailed++;
      return(1);
    }

  return(0);
}

static int run_test(int i)
{
  int return_code = 0;

  printf("***> Running Test %s\n",TestTable[i].name);
  return_code = TestTable[i].runtest(TestTable[i].input, TestTable[i].output);
  if (return_code != 0)
    {
      gRunFailed++;
      TheseTestsFailed[i] = 1;
    }
  return(return_code);
}

static int verify_test(int i)
{
  Test2Run_t *cur;
  char *file_to_verify;
  int err = 0;

  cur = &TestTable[i];  /* create local ptr */

  if (cur->output[0] == '\0')       /* what is the file to verify? */
    file_to_verify = cur->input;
  else
    file_to_verify = cur->output;

  if (cur->mode == kOmPosTest) {
    if (file_to_verify[0] == '\0')   /* no file to verify here */
      {
        printf("***> No file to Verify.  Hope that's okay.\n");
        return(0);     
      }

    printf("***> Verifying Test %s, OMFI file %s\n",cur->name,file_to_verify);

    err = semantic_check(file_to_verify,i);  /* verify the semantics of the file */

    if (err) 
        printf("***>Semantic Check of verify FAILED.\n");
  }

  if (!cur->verify)
    return(err);  /* no verify function to use here,sem check is the return */

  err = cur->verify(file_to_verify);
  
  if (err != 0) 
    {
      TheseTestsFailed[i]=1;
      gVerifyFailed++;
      printf("***> Test Self-Verification phase FAILED.\n");
    }
      
  return(err);
}

static int
run_individual_tests(char **argv, int argc)
{
  int i, return_code;

  for (i=1; i<=NUMTESTS; i++)
    {
      if (RunTheseTests[i])
	{
	  print_header(i);
	  return_code = run_test(i);
	  if (return_code == 0)
	    printf("***> Test run Successful.\n");
	  return_code = verify_test(i);
	  if (return_code == 0)
	    printf("***> Verify Successful.\n");
	}
    }
    return(0);
}

static int
print_summary()
{
  int i, count=0;

  if (run_only_some_tests)
    {
      for (i=1; i<=NUMTESTS; i++)
	if (RunTheseTests[i]) count++;
    }
  else
    count = NUMTESTS;

  printf("\n\n==========================================================\n");
  printf("= UnitTest Summary:                                    \n");
  printf("=         Total Number of Tests Run: %d             \n",count);
  printf("=         Total Number of Failures: %d \n",
	gRunFailed+gSemCheckFaults+gSemCheckFailed+gVerifyFailed);
  printf("=            Test Run failures: %d           \n",gRunFailed);
  printf("=            Test Sem_check faults: %d    \n",gSemCheckFaults);
  printf("=            Test Sem_check failures: %d     \n",gSemCheckFailed);
  printf("=               Semantic Errors: %ld             \n",TotalErrors);
  printf("=               Semantic Warnings: %ld           \n",TotalWarnings);
  printf("=            Test Verify (self-check) failures %d\n",gVerifyFailed);
  if (gRunFailed || gSemCheckFailed || gVerifyFailed || gSemCheckFaults)
    {
      printf("=\n");
      printf("=         Tests that failed: \n");
      for (i=1; i<=NUMTESTS; i++)
	if (TheseTestsFailed[i])
	  printf("=              [%2d] %s\n",i,TestTable[i].name);
    }
  printf("=\n");
  printf("==========================================================\n");
return(0);
}

main(int argc, char **argv)
{
  int i;
  int run_all=0;
  int verify_all=0;
  int run_other=0;
  int return_code;
#if PORT_SYS_MAC
  int     startMem;
 
  MaxApplZone();
  startMem = FreeMem();
#if PORT_MAC_HAS_CCOMMAND
  argc = ccommand(&argv);
#endif
#endif
  
#ifdef MAC_PROFILER
  if(ProfilerInit(collectSummary, bestTimeBase, 10000, 32) != 0)
    return(2);
#endif

  init_table();
  NUMTESTS--;

  check_args(argv,argc,&run_all,&verify_all,&run_other);

  if (run_other)
    run_individual_tests(argv,argc);
  
  if (run_all || verify_all) 
    {
      for (i = 1 ; i <= NUMTESTS ; i++) 
	{
	  if (TestTable[i].name != NULL)
	    {
	      print_header(i);
	      if (run_all)
		{
		  return_code = run_test(i);
		  if (return_code == 0)
		    printf("***> Test reported successful run.\n");
		}
	      return_code = verify_test(i);
	      if (return_code == 0)
		printf("***> Verify Successful.\n");
		   #if !defined(_WIN32)
		   fflush(stdout);
		   #endif
	    }
	}
    }

  print_summary();
  free_testdata();
  exit(0);
  return(0);   /* CW insists that this return be here, even though you'll never get here.... */
}


static int free_testdata()
{
  omfInt32 i;

  for (i=1; i<=NUMTESTS; i++) 
    {
      if (TestTable[i].comments) 
		free(TestTable[i].comments);
      if (TestTable[i].results) 
		free(TestTable[i].results);
    }
    return(0);
}
