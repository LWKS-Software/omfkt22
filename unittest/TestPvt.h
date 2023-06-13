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

typedef int (*proc1_t)(char *in);
typedef int (*proc2_t)(char *in, char *out); 
typedef enum
{
	kOmPosTest, kOmNegTest
} testType_t;

typedef struct Test2Run
{
  char name[25];   /* name of Test2Run */
  char depend[25]; /* name of test to be run before this one */
  proc2_t runtest;  /* The function that needs to be run */
  proc1_t verify;   /* The function that needs to verify the run */
  char input[64];  /* input filename */
  char output[64]; /* output filename */
  char *comments; /* What does this test do? */
  char *results;  /* Expected results comments go here */
  testType_t mode;        
} Test2Run_t;

#define MAXTESTS 100

Test2Run_t TestTable[MAXTESTS];

int WrapAttr1x(char *input,char *output);
int WrapChgMob(char *srcName, char *out);
int WrapCheck1xMedia(char *filename);
int WrapCheck2xMedia(char *filename);
int WrapCheck1xMediaNoWave(char *filename);
int WrapCheck1xRGBATestFile(char *filename);
int WrapCopyMob(char *filename, char *out);
int WrapDelMob(char *srcName, char *out);
int WrapFndSrc(char *srcName, char *out);
int WrapNgFndSrc(char *srcName,char *out);
int WrapMkOMFLoc(char *input, char *output);
int WrapObjIter(char *filename, char *out);
int WrapOpenMod(char *filename, char *out);
int WrapReadComp(char *filename, char *out);
int WrapMkDesc(char *filename, char *out);
int WrapDumpDK(char *filename, char *out);
int WrapERCvt(char *in, char *out);
int WrapDumpFXD(char *filename,char *out);
int WrapIterSrch(char *filename,char *out);
int WrapTCCvt(char *in, char *out);
int WrapVideoCheckTestFile(char *filename);
int WrapVideoWriteTestFile(char *filename,char *out);
int Wrap1MkCplx(char *filename, char *out);
int WrapIMAMkCplx(char *filename, char *out);
int Wrap2MkCplx(char *filename, char *out);
int WrapMkComp2x(char *filename, char *out);
int WrapMkIlleg(char *filename, char *out);
int WrapWrite2xMedia(char *filename,char *out);
int WrapWrite1xMedia(char *filename, char *out);
int WrapWrite1xMediaNoWave(char *filename, char *out);
int WrapWrite1xRGBATestFile(char *filename, char *out);
int Wrap1MkSimpFX(char *filename, char *out);
int Wrap2MkSimpFX(char *filename,char *out);
int Wrap1MkSimple(char *filename, char *out);
int Wrap2MkSimple(char *filename, char *out);
int WrapWrite2xMedia1xFile(char *filename,char *out);
int WrapWrite2xMediaIMAFile(char *filename,char *out);
int WrapMkDesc1(char *filename, char *out);
int WrapMkDescIMA(char *filename, char *out);
int WrapMkDesc2(char *filename, char *out);
int WrapIMAMkSimpFX(char *filename,char *out);
int Wrap1VideoWriteTestFile(char *filename,char *out);
int Wrap2VideoWriteTestFile(char *filename,char *out);
int WrapIMAVideoWriteTestFile(char *filename,char *out);
int Wrap1DupMobs(char *input, char *output);
int WrapIMADupMobs(char *input, char *output);
int Wrap2DupMobs(char *input, char *output);
int WrapRawLoc(char *input, char *output);
int Wrap1ManyObjs(char *input, char *output);
int WrapIMAManyObjs(char *input, char *output);
int Wrap2ManyObjs(char *input, char *output);
int Wrap1PrivReg(char *input, char *output);
int WrapIMAPrivReg(char *input, char *output);
int Wrap2PrivReg(char *input, char *output);
int Wrap1Patch1x(char *input, char *output);
int WrapIMAPatch1x(char *input, char *output);
int WrapCheck2xMedia1x(char *filename);
int WrapCheck2xMediaIMA(char *filename);
int Wrap1MkFilm(char *filename, char *out);
int WrapIMAMkFilm(char *filename, char *out);
int Wrap2MkFilm(char *filename, char *out);
int Wrap1TstAttr(char *filename, char *out);
int Wrap2TstAttr(char *filename, char *out);
int WrapIMATstAttr(char *filename, char *out);
int WrapTestCodecs(char *filename, char *out);
int WrapCompIter(char *filename, char *out);

