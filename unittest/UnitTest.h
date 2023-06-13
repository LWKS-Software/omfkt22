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

#define MakeRational(num, den, resPtr) \
        {(resPtr)->numerator = num; (resPtr)->denominator = den;}

#ifdef MAKE_TEST_HARNESS
int Attr1x(char *filename);
int ChgMob(char *srcName);
int CloneMobX(char *srcName, char *destName);
int NgCloneX(char *srcName, char *destName);
int Convert(char *srcName, char *destname);
int CopyMob(char *filename);
int CopyMobX(char *srcName, char *destName);
int DupMobs(char *infile, char *version);
int DelMob(char *srcName);
int FndSrc(char *srcName);
int NgFndSrc(char *srcName);
int MkOMFLoc(char *input);
int ObjIter(char *filename);
int OpenMod(char *filename);
int ReadComp(char *filename);
int MkDesc(char *filename,char *version);
int DumpDK(char *filename);
int ERCvt(void);
int DumpFXD(char *filename);
int IterSrch(char *filename);
int TCCvt(void);
int MkCplx(char *filename, char *version);
int MkFilm(char *filename, char *version);
int MkComp2x(char *filename);
int MkIlleg(char *filename);
int MkSimpFX(char *filename,char *version);
int MkSimple(char *filename, int version);
int MkRawLoc(char *compFileName);
int ManyObjs(char *filename, char *version);
int PrivReg(char *filename, char *version);
int TstAttr(char *filename, char *version);
int Patch1x(char *filename, char *version);
int TestCodecs(void);
int TestCompIter(char *filename);
#endif


