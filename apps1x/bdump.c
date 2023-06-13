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
 * apps/bdump: An OMFI (Bento) file dumper                                 *
 *             Designed to be an example of how to navigate OMFI files,    *
 *             and a start at a verifier of conformance to the OMFI        *
 *             standard.                                                   *
 ***************************************************************************/
#include "masterhd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef unix
#ifndef NEXT
#include <malloc.h>
#endif
#include <sys/types.h>
#include <time.h>
#endif

#if defined(THINK_C) || defined(__MWERKS__)
#include <console.h>
#include "MacSupport.h"
#include <Types.h>
#include <StandardFile.h>
#endif

#define OMFI_BENTO_ACCESSORS 1 /* To include prototypes for low level calls */

#include "OMApi.h"
#include "OMFI.h"
#include "omUtils.h"
#include "tiff.h"
#include "OMFIdef.h"
#include "CMTypes.h"
#include "omCodec.h"

#ifdef MSDOS
#include <stdlib.h>
#endif

#define streq(a,b) (!strcmp(a,b))

#define FILE_BUFFER_SIZE 5120
#define MAXSTR 512

typedef char indexType;
#define PMOB 1
#define LMOB 2
#define MDAT 3

static void assert(Boolean cond, char *message);
static void usage(void);
static void fatal(char *message);
static void display(short tab_level, char *string);
static u_long GetULong(char *input);
static u_short GetUShort(char *input);
static void DisplayObjectHeader(short tab_level, OMObject obj);
static void DumpClassDictionary(short tab_level, OMObject obj);
static void DumpUnknownObject(short tab_level, char *id, OMObject obj);
static void DumpAttrKind(short tab_level, AttrKind ak);
static void DumpTrackType(short tab_level, TrackType tt);
static void DumpUID(short tab_level, UID id);
static void DumpXlong(short tab_level, char *label, Ulong val);
static void DumpUlong(short tab_level, char *label, Ulong val);
static void DumpLong(short tab_level, char *label, Long val);
static void DumpShort(short tab_level, char *label, Short val);
static void DumpBoolean(short tab_level, char *label, Boolean val);
static void DumpBytes(short tab_level, char *label, char *val, Long numvals);
static void DumpObjHandle(short tab_level, OMObject obj);
static void DumpUsageCode(short tab_level, UsageCodeType uc);
static void DumpPhysMobType(short tab_level, PhysicalMobType pmt);
static void DumpTime(short tab_level, TimeStamp time);
static void DumpExactEditRate(short tab_level, ExactEditRate er);
static void DumpString(short tab_level, char *label, char *strdata);
static void DumpObjectByType(short tab_level, OMObject obj, OMFI_chunks type, char *ident);
static void DumpObject(short tab_level, OMObject obj);
static void DumpMOBIndex(short tab_level, OMObject obj, indexType Index);
static void DumpATTRIBUTEObject(short tab_level, OMObject obj);
static void DumpATTRLISTObject(short tab_level, OMObject obj );
static void DumpCPNTProperties(short tab_level, OMObject obj);
static void DumpSLCTProperties(short tab_level, OMObject obj);
static void DumpSELECTORObject(short tab_level, OMObject obj);
static void DumpTRKGProperties(short tab_level, OMObject obj);
static void DumpTRACKGROUPObject(short tab_level, OMObject obj);
static void DumpTRANProperties(short tab_level, OMObject obj);
static void DumpTRANSITIONObject(short tab_level, OMObject obj);
static void DumpCLIPProperties(short tab_level, OMObject obj);
static void DumpCLIPObject(short tab_level, OMObject obj);
static void DumpFILLProperties(short tab_level, OMObject obj);
static void DumpFILLCLIPObject(short tab_level, OMObject obj);
static void DumpWARPProperties(short tab_level, OMObject obj);
static void DumpREPTProperties(short tab_level, OMObject obj);
static void DumpREPEATCLIPObject(short tab_level, OMObject obj);
static void DumpSPEDProperties(short tab_level, OMObject obj);
static void DumpMOTIONCONTROLObject(short tab_level, OMObject obj);
static void DumpMASKProperties(short tab_level, OMObject obj);
static void DumpCAPTUREMASKObject(short tab_level, OMObject obj);
static void DumpTRKRProperties(short tab_level, OMObject obj);
static void DumpTRACKREFObject(short tab_level, OMObject obj);
static void DumpSCLPProperties(short tab_level, OMObject obj);
static void DumpSOURCECLIPObject(short tab_level, OMObject obj);
static void DumpECCPProperties(short tab_level, OMObject obj);
static void DumpTCCPProperties(short tab_level, OMObject obj);
static void DumpTIMECODECLIPObject(short tab_level, OMObject obj);
static void DumpEDGECODECLIPObject(short tab_level, OMObject obj);
static void DumpSEQUENCEObject(short tab_level, OMObject obj);
static void DumpMOBJProperties(short tab_level, OMObject obj);
static void DumpMEDIAOBJECTObject(short tab_level, OMObject obj);
static void DumpMEDIADESCObject(short tab_level, OMObject obj);
static void DumpMDESProperties(short tab_level, OMObject obj);
static void DumpMDFLProperties(short tab_level, OMObject obj);
static void DumpFILEMEDIADESCObject(short tab_level, OMObject obj);
static void DumpWAVEDATAObject(short tab_level, OMObject obj);
static void DumpAIFCDATAObject(short tab_level, OMObject obj);
static char *GetTiffTagName(u_short code);
static void DumpTIFFDATAObject(short tab_level, OMObject obj);
static void DumpWAVEDescriptor(short tab_level, OMObject obj);
static void DumpAIFDProperties(short tab_level, OMObject obj);
static void DumpAIFCDescriptor(short tab_level, OMObject obj);
static void DumpTIFDProperties(short tab_level, OMObject obj);
static void DumpTIFFDescriptor(short tab_level, OMObject obj);
static void DumpUNXLProperties(short tab_level, OMObject obj);
static void DumpUNIXFILELOCATORObject(short tab_level, OMObject obj);
static void DumpMACLProperties(short tab_level, OMObject obj);
static void DumpMACFILELOCATORObject(short tab_level, OMObject obj);
static void DumpDOSLProperties(short tab_level, OMObject obj);
static void DumpDOSFILELOCATORObject(short tab_level, OMObject obj);
static void DumpTNFXProperties(short tab_level, OMObject obj);
static void DumpTRANSEFFECTObject(short tab_level, OMObject obj);
static void DumpPVOLProperties(short tab_level, OMObject obj);
static void DumpPANVOLEFFECTObject(short tab_level, OMObject obj);
static void DumpTKFXProperties(short tab_level, OMObject obj);
static void DumpTRACKEFFECTObject(short tab_level, OMObject obj);

/* LRC not supported in 1.x Compatibility Mode for 2.0 Toolkit */
#if 0
static void DumpLRCMDATAObject(short tab_level, OMObject obj);
static void DumpLRCDDescriptor(short tab_level, OMObject obj);
#endif

/* Utility routine */

OMFI_chunks GetOMFIChunkType(char *chunk);


static ByteOrder_t ByteOrder;            /* Global byte ordering of file */
static char errstr[256], outstr[256];    /* Formatting strings */
static u_long filepos= 0, curpos = 0;
static char *typestring[] = {
  "NONE    ",
  "BYTE    ",
  "ASCII   ",
  "SHORT   ",
  "LONG    ",
  "RATIONAL" };

#if MOTOROLA && INTEL
  --------------  Both INTEL and MOTOROLA byte order specified  !!!!!!!!!!!!!!!!!!!!!!!
#endif

#if MOTOROLA
  #define PlatformByteOrder Motorola
#else
  #if INTEL
    #define PlatformByteOrder Intel
  #else
--------------  Neither INTEL nor MOTOROLA byte order specified !!!!!!!!!!!!!!!!!!!!!!!
  #endif
#endif

static OMObject Head = NULL;
static Boolean dumpdata = FALSE;
static Boolean dumpdesc = FALSE;

void main(int argc, char *argv[])
{

  OMObject spobj;
  char ident[5];
  int  i;
  VersionType Version;
  Short sex;

#if defined(THINK_C) || defined(__MWERKS__)
  Point where;
  SFReply fs;
  SFTypeList fTypes;
#else
  char *fname;
#endif

  fileHandleType fh;
  omfFileRev_t fileRev;

  /* Process command line */

#if defined(THINK_C)
  /*    console_options.nrows = 54;*/
  /*    console_options.ncols = 132;*/
  argc = ccommand(&argv);
#endif
  argc--, argv++;

#if !defined(THINK_C) && !defined(__MWERKS__)
  if (argc < 1)
    usage();
#endif

  /* Process command line switches */
  for (; argc > 0 && argv[0][0] == '-'; argc--, argv++)
    {
      if (streq(argv[0], "-help"))
        usage();
      else if (streq(argv[0], "-ddata"))
        dumpdata = TRUE;
      else if (streq(argv[0], "-ddesc"))
        dumpdesc = TRUE;
    }

  /* Get the file name and open */

#if OMFI_MACSF_STREAM
	/*
	*	Standard Toolbox stuff
	*/
	MacInit();

	where.h = 20;
	where.v = 40;

  /* Get the file name and open */
	fTypes[0] = 'OMFI';
	SFGetFile(where,  NULL, NULL, -1, fTypes, NULL, &fs);
	if (! fs.good)
		exit(0);
        fh = (fileHandleType)&fs;
#else

  fname = argv[0];

  if (fname[0] == '-')
    fatal("No filename given");

  fh = (fileHandleType)fname;

#endif

  /* Open the input file */

  omfiBeginSession();

  Head = omfiOpenFile(fh);

  if (Head == NULL)
    {
      sprintf(errstr, "Error opening input file\n");
      fatal(errstr);
    }

  omfsFileGetRev(GetFileHdl(Head), &fileRev);
  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
    printf("OMFI File Revision: 1.0\n");
  else if (fileRev == kOmfRev2x)
    {
      printf("OMFI File Revision: 2.0 - NOT SUPPORTED by bdump\n");
      omfiCloseFile(Head);
      omfiEndSession();
      exit(1);
    }
  else
    {
      printf("OMFI File Revision: UNKNOWN - NOT SUPPORTED by bdump\n");
      omfiCloseFile(Head);
      omfiEndSession();
      exit(1);
    }


  /* Now process the file */
  /* Make sure it is an OMFI file */

  assert(OMGetObjID(Head, (ObjectTag)ident), "no ID on Head object");

  if (strncmp(ident, ChunkNameTable[HEAD].code, (size_t)4))
      fatal("HEAD chunk not present");

  /* Get the byte ordering */

  assert(OMGetByteOrder(Head, &sex), "No byte order specified for file");
  if (sex == 0x4d4d)
    ByteOrder = Motorola;
  else if (sex == 0x4949)
    ByteOrder = Intel;
  else
    fatal("Unknown byte ordering specified");

  OMSetDefaultByteOrder(Head, sex);

  /* Now we know the byte ordering, we can begin to dump */


  OMGetVersion(Head, &Version);

  display(1, "OMFI");
  if (ByteOrder == Motorola)
    display(2, "MM");
  else
    display(2, "II");

  sprintf(outstr, "Version %d.%d", Version.major, Version.minor);
  display(2, outstr);

  DumpClassDictionary(1, Head);

  for (i = 1; i <= OMLengthObjectSpine(Head); i++)
    {
      display(2,"");
      OMGetNthObjectSpine(Head, &spobj, i);
      DumpObject(1, spobj);
    }

  DumpMOBIndex(1, Head, PMOB);

  DumpMOBIndex(1, Head, LMOB);

  DumpMOBIndex(1, Head, MDAT);

  display(2,"");

  omfiCloseFile(Head);
  omfiEndSession();
}


static void usage(void)
{
  printf("Usage:  bdump [options] <omf-file>\n\n");
  printf("No options currently specified\n\n");
  printf("Produces a formatted dump of an input OMFI file\n");
  exit(0);
}

static void assert(Boolean cond, char *message)

{
  if (! cond) fatal(message);
}


static void fatal(char *message)

{

  char *estr;

  if (OMErrorCode)
    {
      estr = OMGetErrorString(OMErrorCode);
      printf("%s\n\n", estr);
    }
  printf("bdump Error: %s\n\n", message);

  if (OMBentoErrorRaised)
    {
      printf("Bento error %1ld: %s\n", OMBentoErrorNumber, OMBentoErrorString);
    }

  if (Head)
    {
      omfiCloseFile(Head);
      omfiEndSession();
    }

  exit(OMErrorCode ? (int)OMErrorCode : 1);
}


static void display(short tab_level, char *string)

{
  short i, j = 0;

  if (tab_level == 1)
    {
      printf("* ");
      j = 1;
    }

  for (i = j; i < tab_level; i++)
    printf("  ");

  printf("%s\n", string);

}

static u_long GetULong(char *data)
{
  u_long result;


#if BYTE_SWAP_TEST && BYTE_ALIGN_DATA
  if (ByteOrder == PlatformByteOrder)
    {
      result = *(u_long *)data;
      printf("*********TEST  getUlong casts into %d\n",result);
      return(result);
    }
#endif

  if (ByteOrder == Motorola)
    {
      result = ((((u_long)data[0] & 0x000000ff) << 24) |
                (((u_long)data[1] & 0x000000ff) << 16) |
                (((u_long)data[2] & 0x000000ff) << 8)  |
                (((u_long)data[3])& 0x000000ff));
    }
  else
    {
      result = ((((u_long)data[0])& 0x000000ff)        |
                (((u_long)data[1] & 0x000000ff) << 8)  |
                (((u_long)data[2] & 0x000000ff) << 16) |
                (((u_long)data[3] & 0x000000ff) << 24));
    }

  return(result);
}


static u_short GetUShort(char *data)
{

  u_short result;

  if (ByteOrder == Motorola)
    result = ((((u_short)data[0] & 0x00ff)<< 8) |
              (((u_short)data[1])& 0x00ff));
  else
    result = ((((u_short)data[0])& 0x00ff) |
              (((u_short)data[1] & 0x00ff)<< 8));


  return(result);
}

static void DisplayObjectHeader(short tab_level, OMObject obj)

{
  char id[8];

  if (OMGetObjID(obj, (ObjectTag)id))
    {
      id[4]=0;
      sprintf(outstr,"%4s, [Label: %1ld ]", id, OMGetBentoID(obj) & 0x0000ffff);
    }
  else
    sprintf(outstr,"Warning: Bogus ID!!!");
  display(tab_level, outstr);

}

OMFI_chunks GetOMFIChunkType(char *chunk)
{
  int  i;

  if (chunk[0] == 0 && chunk[1] == 0 && chunk[2] == 0 && chunk[3] == 0)
    return(CHUNK_NULL);


  for (i = 0; i < OMFI_CHUNK_UNKNOWN; i++)
    {
      if (!strncmp(chunk, ChunkNameTable[i].code, (size_t)4))
        return(ChunkNameTable[i].number);
    }
  return(OMFI_CHUNK_UNKNOWN);

}

static void DumpClassDictionary(short tab_level, OMObject obj)
{
  OMObject dictObj, dictParent;
  char id[5];
  int  i, numEntries;
  char classString[512];

  numEntries = OMLengthClassDictionary(obj);

  display(tab_level, "OMFI Class Dictionary");

  for (i=0; i<numEntries; i++)
    {
      OMGetNthClassDictionary(obj, &dictObj, i+1);

      OMGetCLSDClassID(dictObj, (ObjectTag)id);
      strncpy(classString, id, 4);
      classString[4] = 0;

      while (OMGetCLSDParentClass(dictObj, &dictParent))
	{
	  dictObj = dictParent;

	  OMGetCLSDClassID(dictObj, (ObjectTag)id);
	  id[4] = 0;

	  strcat(classString, " -> ");
	  strcat(classString, id);

	}

      display(tab_level+1, classString);
    }

  display(tab_level+1, "");
}

static void DumpUnknownObject(short tab_level, char *id, OMObject obj)

{
  int  i;
  int  numEntries;
  char classID[5];
  OMFI_chunks type;
  OMObject dictObj, dictParent;

  sprintf(outstr, "(Unknown structure tagged '%4s') ", id);
  display(tab_level, outstr);

  numEntries = OMLengthClassDictionary(Head);

  for (i=0; i<numEntries; i++)
    {
      OMGetNthClassDictionary(Head, &dictObj, i+1);

      OMGetCLSDClassID(dictObj, (ObjectTag)classID);
      if (strncmp(id, classID, (size_t)4) != 0)
      	continue;

      while (OMGetCLSDParentClass(dictObj, &dictParent))
	    {
	  	dictObj = dictParent;

	  	OMGetCLSDClassID(dictObj, (ObjectTag)classID);
	  	id[4] = 0;

	    if ((type = GetOMFIChunkType(classID)) == OMFI_CHUNK_UNKNOWN)
	    	continue;
	    else
	    	{
	    	DumpObjectByType(tab_level, obj, type, classID);
	    	return;
	    	}
		}
    }

  display(tab_level+1, "");


}

static void DumpAttrKind(short tab_level, AttrKind ak)

{

  switch (ak)

    {
    case AK_NULL:
      sprintf(outstr, "Attribute Kind: AK_NULL");
      break;

    case AK_INT:
      sprintf(outstr, "Attribute Kind: AK_INT");
      break;

    case AK_STRING:
      sprintf(outstr, "Attribute Kind: AK_STRING");
      break;

    case AK_OBJECT:
      sprintf(outstr, "Attribute Kind: AK_OBJECT");
      break;

    default:
      sprintf(outstr, "Unknown Attr Kind \"%1d\"", ak);
      break;
    }

  display(tab_level, outstr);
}

static void DumpTrackType(short tab_level, TrackType tt)

{

  switch (tt)
    {
    case TT_NULL:
      sprintf(outstr, "Track Type: TT_NULL");
      break;

    case TT_PICTURE:
      sprintf(outstr, "Track Type: TT_PICTURE");
      break;

    case TT_SOUND:
      sprintf(outstr, "Track Type: TT_SOUND");
      break;

    case TT_TIMECODE:
      sprintf(outstr, "Track Type: TT_TIMECODE");
      break;

    case TT_EDGECODE:
      sprintf(outstr, "Track Type: TT_EDGECODE");
      break;

    case TT_ATTRIBUTE:
      sprintf(outstr, "Track Type: TT_ATTRIBUTE");
      break;

    case TT_EFFECTDATA:
      sprintf(outstr, "Track Type: TT_EFFECTDATA");
      break;

    default:
      sprintf(outstr, "Unknown Track Type \"%1d\"", tt);
      break;
    }

  display(tab_level, outstr);
}

static void DumpUID(short tab_level, UID id)

{
  sprintf(outstr, "UID : (%1ld, %1ld, %1ld)", id.prefix, id.major, id.minor);
  display(tab_level, outstr);
}

static void DumpXlong(short tab_level, char *label, Ulong val)

{
  sprintf(outstr, "%s : 0x%08lx", label, val);
  display(tab_level, outstr);
}

static void DumpUlong(short tab_level, char *label, Ulong val)

{
  sprintf(outstr, "%s : %1lu", label, val);
  display(tab_level, outstr);
}

static void DumpLong(short tab_level, char *label, Long val)

{
  sprintf(outstr, "%s : %1ld", label, val);
  display(tab_level, outstr);
}

static void DumpShort(short tab_level, char *label, Short val)

{
  sprintf(outstr, "%s : %1d", label, val);
  display(tab_level, outstr);
}


static void DumpBoolean(short tab_level, char *label, Boolean val)

{
  sprintf(outstr, "%s : %s", label, (val ? "T" : "F"));
  display(tab_level, outstr);
}


static void DumpBytes(short tab_level, char *label, char *val, Long numvals)
{
  Long i, j, lines;
  Long rem;
  char *c = val;
  char *s = outstr;
  unsigned int  fourbytes[4];

  sprintf(outstr, "%s : ", label);
  display(tab_level, outstr);

  lines = numvals / 16;
  rem = numvals % 16;

  for (i = 0; i < lines; i++)
    {
      for (j = 0; j < 4; j++)
	{
	  fourbytes[j] = *c++ << 24;
	  fourbytes[j] |= *c++ << 16;
	  fourbytes[j] |= *c++ << 8;
	  fourbytes[j] |= *c++;
	}
      sprintf(outstr, "0x%8x 0x%8x 0x%8x 0x%8x", fourbytes[0], fourbytes[1], fourbytes[2], fourbytes[3]);
      display(tab_level+1, outstr);
    }

  for (j = 0; j < rem; j++)
    {
      fourbytes[j] = *c++ << 24;
      fourbytes[j] |= *c++ << 16;
      fourbytes[j] |= *c++ << 8;
      fourbytes[j] |= *c++;
      sprintf(s, "0x%8x ", fourbytes[j]);
      s = &s[strlen(s)];
    }
  display(tab_level+1, outstr);

}


static void DumpObjHandle(short tab_level, OMObject obj)

{
  sprintf(outstr,"Label = %1ld", OMGetBentoID(obj) & 0x0000ffff);
  display(tab_level, outstr);
}

static void DumpUsageCode(short tab_level, UsageCodeType uc)

{

  strcpy(outstr, "Usage Code: ");

  switch (uc)
    {
    case UC_NULL:
      sprintf(&outstr[strlen(outstr)], "UC_NULL");
      break;

    case UC_MASTERMOB:
      sprintf(&outstr[strlen(outstr)], "UC_MASTERMOB");
      break;

#ifdef UC_PRECOMPUTE
    case UC_PRECOMPUTE:
      sprintf(&outstr[strlen(outstr)], "UC_PRECOMPUTE");
      break;
#endif

#ifdef UC_SUBCLIP
    case UC_SUBCLIP:
      sprintf(&outstr[strlen(outstr)], "UC_SUBCLIP");
      break;
#endif

#ifdef UC_EFFECT
    case UC_EFFECT:
      sprintf(&outstr[strlen(outstr)], "UC_EFFECT");
      break;
#endif

#ifdef UC_GROUP
    case UC_GROUP:
      sprintf(&outstr[strlen(outstr)], "UC_GROUP");
      break;
#endif

#ifdef UC_GROUPOOFTER
    case UC_GROUPOOFTER:
      sprintf(&outstr[strlen(outstr)], "UC_GROUPOOFTER");
      break;
#endif

#ifdef UC_MOTION
    case UC_MOTION:
      sprintf(&outstr[strlen(outstr)], "UC_MOTION");
      break;
#endif

#ifdef UC_STUB
    case UC_STUB:
      sprintf(&outstr[strlen(outstr)], "UC_STUB");
      break;
#endif

#ifdef UC_PRECOMPUTE_FILE
    case UC_PRECOMPUTE_FILE:
      sprintf(&outstr[strlen(outstr)], "UC_PRECOMPUTE_FILE");
      break;
#endif

    default:
      sprintf(&outstr[strlen(outstr)], "Unknown Usage Code \"%1ld\"", uc);
      break;
    }

  display(tab_level, outstr);
}

static void DumpPhysMobType(short tab_level, PhysicalMobType pmt)

{
  switch (pmt)
    {
    case PT_NULL:
      sprintf(outstr, "NULL_MOB");
      break;

    case PT_FILE_MOB:
      sprintf(outstr, "FILE_MOB");
      break;

    case PT_TAPE_MOB:
      sprintf(outstr, "TAPE_MOB");
      break;

    case PT_FILM_MOB:
      sprintf(outstr, "FILM_MOB");
      break;

    case PT_NAGRA_MOB:
      sprintf(outstr, "NAGRA_MOB");
      break;

    default:
      sprintf(outstr, "Unknown PhysMob Type \"%1d\"",pmt);
      break;
    }

  display(tab_level, outstr);
}

static void DumpTime(short tab_level, TimeStamp time)

{

  char gmt[32];

#ifdef unix
  char timestr[48];
#endif

#if ! DEBUG
  if (! time.TimeVal)
    return;
#endif

  if (time.IsGMT)
    strcpy(gmt, "(GMT-based)");
  else
    strcpy(gmt, "(Unknown timezone)");

#ifdef unix
  strcpy(timestr, ctime((const time_t *)&(time.TimeVal)));
  timestr[strlen(timestr)-1] = 0;
  sprintf(outstr, "Time : %s %s", timestr, gmt);
#else
  sprintf(outstr, "Time: %ld %s", time.TimeVal, gmt);
#endif
  display(tab_level, outstr);
}

static void DumpExactEditRate(short tab_level, ExactEditRate er)

{
  sprintf(outstr, "Rate: %1ld/%1ld", er.numerator, er.denominator);
  display(tab_level, outstr);
}

static void DumpString(short tab_level, char *label, char *strdata)

{

  if (strlen(strdata) == 0)
    {

      /* Null string, don't print                         */
      /*                                                  */
      /*      sprintf(outstr, "%s : (NULL)", label);      */
      /*      display(tab_level, outstr);                 */

    }
  else
    {
      sprintf(outstr, "%s : %s", label, strdata);
      display (tab_level, outstr);
    }
}


static void DumpObjectByType(short tab_level, OMObject obj, OMFI_chunks type, char *ident)
{
  switch (type)
    {

    case ATTRLIST:
      DumpATTRLISTObject(tab_level, obj);
      break;

    case ATTRIBUTE:
      DumpATTRIBUTEObject(tab_level, obj);
      break;

    case TRACKGROUP:
      DumpTRACKGROUPObject(tab_level, obj);
      break;

    case SELECTOR:
      DumpSELECTORObject(tab_level, obj);
      break;

    case TRANSITION:
      DumpTRANSITIONObject(tab_level, obj);
      break;

    case MEDIAOBJECT:
      DumpMEDIAOBJECTObject(tab_level, obj);
      break;

    case MEDIADESC:
      DumpMEDIADESCObject(tab_level, obj);
      break;

    case FILEMEDIADESC:
      DumpFILEMEDIADESCObject(tab_level, obj);
      break;

    case UNIXFILELOCATOR:
      DumpUNIXFILELOCATORObject(tab_level, obj);
      break;

    case MACFILELOCATOR:
      DumpMACFILELOCATORObject(tab_level, obj);
      break;

    case DOSFILELOCATOR:
      DumpDOSFILELOCATORObject(tab_level, obj);
      break;

    case CLIP:
      DumpCLIPObject(tab_level, obj);
      break;

    case FILLCLIP:
      DumpFILLCLIPObject(tab_level, obj);
      break;

    case SOURCECLIP:
      DumpSOURCECLIPObject(tab_level, obj);
      break;

    case TRACKREF:
      DumpTRACKREFObject(tab_level, obj);
      break;

    case TIMECODECLIP:
      DumpTIMECODECLIPObject(tab_level, obj);
      break;

    case EDGECODECLIP:
      DumpEDGECODECLIPObject(tab_level, obj);
      break;

    case REPEATCLIP:
      DumpREPEATCLIPObject(tab_level, obj);
      break;

    case MOTIONCONTROL:
      DumpMOTIONCONTROLObject(tab_level, obj);
      break;

    case CAPTUREMASK:
      DumpCAPTUREMASKObject(tab_level, obj);
      break;

    case SEQUENCE:
      DumpSEQUENCEObject(tab_level, obj);
      break;

    case TIFFDATA:
      DumpTIFFDATAObject(tab_level, obj);
      break;

    case TIFFMEDIADESC:
      DumpTIFFDescriptor(tab_level, obj);
      break;

/* LRC not supported in 1.x Compatibility Mode for 2.0 Toolkit */
#if 0
    case LRCMDATA:
      DumpLRCMDATAObject(tab_level, obj);
      break;

    case LRCDMEDIADESC:
      DumpLRCDDescriptor(tab_level, obj);
      break;
#endif

    case AIFCDATA:
      DumpAIFCDATAObject(tab_level, obj);
      break;

    case AIFCMEDIADESC:
      DumpAIFCDescriptor(tab_level, obj);
      break;

    case WAVEDATA:
      DumpWAVEDATAObject(tab_level, obj);
      break;

    case WAVEMEDIADESC:
      DumpWAVEDescriptor(tab_level, obj);
      break;

    case AVIDTRANSEFFECT:
      DumpTRANSEFFECTObject(tab_level, obj);
      break;

    case AVIDPANVOLEFFECT:
      DumpPANVOLEFFECTObject(tab_level, obj);
      break;

    case AVIDTRACKEFFECT:
      DumpTRACKEFFECTObject(tab_level, obj);
      break;

    default:
      DumpUnknownObject(tab_level, ident, obj);
      break;
    }
}
static void DumpObject(short tab_level, OMObject obj)

{
  char ident[5];
  OMFI_chunks type;

  ident[4] = 0;

  if (OMGetObjID(obj, (ObjectTag)ident))
    type = GetOMFIChunkType(ident);
  else
    type = CHUNK_NULL;

  DumpObjectByType(tab_level, obj, type, ident);

}


static void DumpMOBIndex(short tab_level, OMObject obj, indexType Index)

{
  u_long num_spec, i;
  char mdesc[20];
  MobIndexElement mi;
  OMProperty prop;

  switch (Index)
    {
    case LMOB:
      strcpy(mdesc,"Composition Mobs");
      prop = OMCompositionMobs;
      break;

    case PMOB:
      strcpy(mdesc,"Source Mobs");
      prop = OMSourceMobs;
      break;

    case MDAT:
      strcpy(mdesc,"Media Data Objects");
      prop = OMMediaData;
      break;

    default:
      return;
    };

  num_spec = OMLengthMobIndexWP(obj, prop);
  if (num_spec < 1)
    return;

  display(tab_level+1, "");

  sprintf(outstr, "Number of %s = %ld", mdesc, num_spec);
  display(tab_level, outstr);

  for (i = 0; i < num_spec; i++)
    {
      display(tab_level+1, "");
      OMGetNthMobIndexWP(obj, prop, &mi, i+1);
      DumpUID(tab_level+1, mi.ID);
      DumpObjHandle(tab_level+1, mi.Mob);
    }

}

static void DumpATTRIBUTEObject(short tab_level, OMObject obj)

{
  AttrKind ak;
  char name[512];
  Long lv;
  char sv[512];
  OMObject ov;

  DisplayObjectHeader(tab_level, obj);

  ak = AK_NULL;

  if (OMGetATTBKind(obj, &ak))
    DumpAttrKind(tab_level+1, ak);

  if (OMGetATTBName(obj, name, 512))
    DumpString(tab_level+1, "Name", name);

  switch (ak)
    {
    case AK_INT:
      if (OMGetATTBIntAttribute(obj, &lv))
        DumpLong(tab_level+1, "Int Attribute", lv);
      break;

    case AK_STRING:
      if (OMGetATTBStringAttribute(obj, sv, 512))
        DumpString(tab_level+1, "String Attribute", sv);
      break;

    case AK_OBJECT:
      if (OMGetATTBObjAttribute(obj, &ov))
        {
          display (tab_level+1, "Object Attribute");
          DumpObject(tab_level+2, ov);
        }
      break;

    default:
      break;
    }

}
static void DumpATTRLISTObject(short tab_level, OMObject obj )

{
  OMObject comp;
  int  ncomps, i;

  DisplayObjectHeader(tab_level, obj);

  ncomps = OMLengthATTRAttrRef(obj);
  sprintf(outstr, "Number of Attributes : %1ld", ncomps);
  display(tab_level+1, outstr);

  display(tab_level+1, "");
  for (i = 0; i < ncomps; i++)
    {
      OMGetNthATTRAttrRef(obj, &comp, i+1);
      DumpObject(tab_level+2, comp);
      display(tab_level+2, "");
    }
}

static void DumpCPNTProperties(short tab_level, OMObject obj)

{
  TrackType tt;
  ExactEditRate er;
  char name[MAXSTR];
  char effectID[MAXSTR];
  OMObject attr, prec;

  if (OMGetCPNTTrackKind(obj, &tt))
    DumpTrackType(tab_level, tt);

  if (OMGetCPNTEditRate(obj, &er))
    DumpExactEditRate(tab_level, er);

  if (OMGetCPNTName(obj, name, MAXSTR))
    DumpString(tab_level, "Name", name);

  if (OMGetCPNTEffectID(obj, effectID, MAXSTR))
    DumpString(tab_level, "EffectID", effectID);

  if (OMGetCPNTAttributes(obj, &attr))
    {
      display(tab_level+1, "Attributes");
      DumpObject(tab_level+2, attr);
    }
  if (OMGetCPNTPrecomputed(obj, &prec))
    {
      display(tab_level+1, "Precomputed");
      DumpObject(tab_level+2, prec);
    }

}

static void DumpSLCTProperties(short tab_level, OMObject obj)

{
  Short sh;
  omfBool bo;

  DumpTRKGProperties(tab_level, obj);

  if (OMGetSLCTIsGanged(obj, &bo))
    DumpBoolean(tab_level, "SLCT Is Ganged", bo);

  if (OMGetSLCTSelectedTrack(obj, &sh))
    DumpShort(tab_level, "SLCT Selected Track", sh);

}


static void DumpSELECTORObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpSLCTProperties(tab_level+1, obj);

}


static void DumpTRKGProperties(short tab_level, OMObject obj)

{
  int  i;
  Long gl;
  OMObject track, attr, fillproxy, trackcomp;
  Short ln;
  int  num_tracks;

  DumpCPNTProperties(tab_level, obj);

  if(OMGetTRKGGroupLength(obj, &gl))
    DumpLong(tab_level, "Group Length", gl);

  num_tracks = OMLengthTRKGTrack(obj);
  DumpLong(tab_level+1, "Num subtracks", (Long)num_tracks);
  display(tab_level+1, "");

  for (i = 0; i < num_tracks; i++)
    {

      OMGetNthTRKGTrack(obj, &track, i+1);

      DumpLong(tab_level+1, "Track number", (Long)i);

      if (OMGetTRAKLabelNumber(track, &ln))
        DumpLong(tab_level+1, "Track label number", ln);

      if (OMGetTRAKAttributes(track, &attr))
        {
          display(tab_level+1, "Attributes");
          DumpObject(tab_level+2, attr);
        }

      if (OMGetTRAKTrackComponent(track, &trackcomp))
        {
          display(tab_level+1, "Track Component");
          DumpObject(tab_level+2, trackcomp);
        }

      if (OMGetTRAKFillerProxy(track, &fillproxy))
        {
          display(tab_level+1, "Filler Proxy");
          DumpObject(tab_level+2, fillproxy);
        }

      display(tab_level+1, "");

    }

}

static void DumpTRACKGROUPObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpTRKGProperties(tab_level+1, obj);

}

static void DumpTRANProperties(short tab_level, OMObject obj)

{
  Long len;

  if (OMGetTRANCutPoint(obj, &len))
    DumpLong(tab_level+1, "CutPoint", len);

  DumpTRKGProperties(tab_level, obj);

}

static void DumpTRANSITIONObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpTRANProperties(tab_level+1, obj);

}

static void DumpCLIPProperties(short tab_level, OMObject obj)

{
  Long ln;

  if (OMGetCLIPLength(obj, &ln))
    DumpLong(tab_level, "CLIP Length", ln);

  DumpCPNTProperties(tab_level, obj);

}

static void DumpCLIPObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpCLIPProperties(tab_level+1, obj);

}

static void DumpFILLProperties(short tab_level, OMObject obj)

{

  DumpCLIPProperties(tab_level, obj);

}

static void DumpFILLCLIPObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpFILLProperties(tab_level+1, obj);

}

static void DumpWARPProperties(short tab_level, OMObject obj)
{
  ExactEditRate er;
  Short po;

  if (OMGetWARPEditRate(obj, &er))
  	DumpExactEditRate(tab_level, er);

  if (OMGetWARPPhaseOffset(obj, &po))
  	DumpShort(tab_level, "Phase Offset", po);

  DumpTRKGProperties(tab_level, obj);

}
static void DumpREPTProperties(short tab_level, OMObject obj)

{

  DumpWARPProperties(tab_level, obj);

}

static void DumpREPEATCLIPObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpREPTProperties(tab_level+1, obj);

}

static void DumpSPEDProperties(short tab_level, OMObject obj)

{
  Long ln;

  if (OMGetSPEDNumerator(obj, &ln))
    DumpLong(tab_level, "SPED Numerator", ln);

  if (OMGetSPEDDenominator(obj, &ln))
    DumpLong(tab_level, "SPED Denominator", ln);

  DumpWARPProperties(tab_level, obj);

}

static void DumpMOTIONCONTROLObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpSPEDProperties(tab_level+1, obj);

}

static void DumpMASKProperties(short tab_level, OMObject obj)

{
  Ulong ln;
  omfBool isDouble;

  if (OMGetMASKMaskBits(obj, &ln))
    DumpUlong(tab_level, "Mask Bits", ln);

  if (OMGetMASKIsDouble(obj, &isDouble))
    DumpLong(tab_level, "Is Double", isDouble);

  DumpWARPProperties(tab_level, obj);

}

static void DumpCAPTUREMASKObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpMASKProperties(tab_level+1, obj);

}

static void DumpTRKRProperties(short tab_level, OMObject obj)

{
  Short tn;

  DumpCLIPProperties(tab_level, obj);

  if (OMGetTRKRRelativeScope(obj, &tn))
    DumpShort(tab_level, "Relative Scope", tn);

  if (OMGetTRKRRelativeTrack(obj, &tn))
    DumpLong(tab_level, "Relative Track", tn);

}

static void DumpTRACKREFObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpTRKRProperties(tab_level+1, obj);

}
static void DumpSCLPProperties(short tab_level, OMObject obj)

{
  Short tn;
  UID id;
  Long ln;

  DumpCLIPProperties(tab_level, obj);

  if (OMGetSCLPSourceID(obj, &id))
    DumpUID(tab_level, id);

  if (OMGetSCLPSourceTrack(obj, &tn))
    DumpShort(tab_level, "Track Number", tn);

  if (OMGetSCLPSourcePosition(obj, &ln))
    DumpLong(tab_level, "Source Position", ln);

}

static void DumpSOURCECLIPObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpSCLPProperties(tab_level+1, obj);

}

static void DumpTCCPProperties(short tab_level, OMObject obj)

{
  Short sn;
  Ulong un;
  Long ln;


  if (OMGetTCCPFlags(obj, &un))
    DumpXlong(tab_level, "Flags", un);

  if (OMGetTCCPFPS(obj, &sn))
    DumpShort(tab_level, "Frames per second", sn);

  if (OMGetTCCPStartTC(obj, &ln))
    DumpLong(tab_level, "Start Timecode", ln);

  DumpCLIPProperties(tab_level, obj);

}

static void DumpTIMECODECLIPObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpTCCPProperties(tab_level+1, obj);

}

static void DumpECCPProperties(short tab_level, OMObject obj)

{
  FilmType sn;
  Long ln;


  if (OMGetECCPFilmKind(obj, &sn))
    DumpLong(tab_level, "Film Kind", sn);

  if (OMGetECCPCodeFormat(obj, (EdgeType *)&sn))
    DumpShort(tab_level, "Code Format", sn);

  if (OMGetECCPStartEC(obj, &ln))
    DumpLong(tab_level, "Start Edgecode", ln);

  DumpCLIPProperties(tab_level, obj);

}

static void DumpEDGECODECLIPObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpECCPProperties(tab_level+1, obj);

}

static void DumpSEQUENCEObject(short tab_level, OMObject obj)

{
  OMObject comp;
  int  ncomps, i;

  DisplayObjectHeader(tab_level, obj);

  DumpCPNTProperties(tab_level+1, obj);

  ncomps = OMLengthSEQUSequence(obj);
  DumpLong(tab_level+1, "Number of SubComps", (Long)ncomps);

  display(tab_level+1, "");
  for (i = 0; i < ncomps; i++)
    {
      OMGetNthSEQUSequence(obj, &comp, i+1);
      DumpObject(tab_level+2, comp);
      display(tab_level+2, "");
    }
}

static void DumpMOBJProperties(short tab_level, OMObject obj)

{
  UID id;
  TimeStamp ts;
  UsageCodeType uc;
  Long st;
  OMObject media;

  DumpTRKGProperties(tab_level, obj);

  assert(OMGetMOBJMobID(obj, &id), "no mob id on MOBJ");
  DumpUID(tab_level, id);

  if (OMGetMOBJLastModified(obj, &ts))
    DumpTime(tab_level, ts);

  if (OMGetMOBJUsageCode(obj, &uc))
    DumpUsageCode(tab_level, uc);

  if (OMGetMOBJStartPosition(obj, &st))
    DumpLong(tab_level, "Start", st);

  if (OMGetMOBJPhysicalMedia(obj, &media))
    {
      display(tab_level, "Physical Media:");
      DumpObject(tab_level+1, media);
    }

}

static void DumpMEDIAOBJECTObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpMOBJProperties(tab_level+1, obj);

}

static void DumpMEDIADESCObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpMDESProperties(tab_level+1, obj);

}

static void DumpMDESProperties(short tab_level, OMObject obj)

{
  int  i;
  int  len;
  PhysicalMobType pmt;

  if (OMGetMDESMobKind(obj, &pmt))
    DumpPhysMobType(tab_level, pmt);

  /* Dump the file locator */
  len = OMLengthMDESLocator(obj);
  if (len)
    {
      OMObject locator;

      display(tab_level, "Locators:");
      display(tab_level+1, "");
      for (i = 1; i <= len; i++)
        {
          if (OMGetNthMDESLocator(obj, &locator, (Long)i))
            {
              DumpObject(tab_level+1, locator);
              display(tab_level+1, "");
            }
        }
    }
}

static void DumpMDFLProperties(short tab_level, OMObject obj)

{
  ExactEditRate er;
  Long len;
  omfBool IsOMFI;

  if (OMGetMDFLSampleRate(obj, &er))
    DumpExactEditRate(tab_level, er);

  if (OMGetMDFLLength(obj, &len))
    DumpLong(tab_level, "Length", len);

  if (OMGetMDFLIsOMFI(obj, &IsOMFI))
    {
      sprintf(outstr, "IsOMFI? : %s", (IsOMFI ? "True" : "False"));
      display(tab_level, outstr);
    }

  DumpMDESProperties(tab_level, obj);

}

static void DumpFILEMEDIADESCObject(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpMDFLProperties(tab_level+1, obj);

}


static void DumpWAVEDATAObject(short tab_level, OMObject obj)

{
#ifdef WAVEDATA_IMPLEMENTED
  int  size;
  int  form, chunk, nsampframes;
  short nchannels;
  short samplesize;
  char aifc[4], comm[4], ssnd[4];
  char ieeerate[10];
  char found;
  double r;
#endif
  UID id;

  DisplayObjectHeader(tab_level, obj);

  if (OMGetWAVEMobID(obj, &id))
    DumpUID(tab_level+1, id);

  display(tab_level+1, "WAVE Form data not yet handled");

  if (dumpdata)
    {

    }

}

/* LRC not supported in 1.x Compatibility Mode for 2.0 Toolkit */
#if 0
static void DumpLRCMDATAObject(short tab_level, OMObject obj)

{
  short frameLength, wordLength, dataByteOrder;
  ExactEditRate sampleRate;
  Long length;
  unsigned int  compressedLength;
  UID id;

  DisplayObjectHeader(tab_level, obj);

  if (OMGetLRCMMobID(obj, &id))
    DumpUID(tab_level+1, id);

  display(tab_level+1, "Lossless Realtime Compression (LRC) data");

  if (OMGetLRCMFrameLength(obj, &frameLength))
    DumpShort(tab_level+2, "Block Size", frameLength);

  if (OMGetLRCMWordLength(obj, &wordLength))
    DumpShort(tab_level+2, "Bits per sample", wordLength);

  OMGetLRCMSampleRate(obj, &sampleRate);
  DumpExactEditRate(tab_level+2, sampleRate);

  if (OMGetLRCMLength(obj, &length))
    DumpLong(tab_level+2, "Length of Media in edit units", length);

  if (OMGetLRCMCompressedLength(obj, &compressedLength))
    DumpLong(tab_level+2, "Length of compressed bit stream", compressedLength);

  if (OMGetLRCMDataByteOrder(obj, &dataByteOrder))
    {
      sprintf(outstr, "Data Byte Order: %s",
	      (ByteOrder == Motorola ? "MM" : "II"));
      display(tab_level+2, outstr);
    }
}

static void DumpLRCDDescriptor(short tab_level, OMObject obj)

{
  short frameLength, wordLength;
  unsigned int  compressedLength;
  Boolean indexPresent;

  DisplayObjectHeader(tab_level, obj);

  DumpMDFLProperties(tab_level+1, obj);

  if (OMGetLRCDFrameLength(obj, &frameLength))
    DumpShort(tab_level+2, "Block Size", frameLength);

  if (OMGetLRCDWordLength(obj, &wordLength))
    DumpShort(tab_level+2, "Bits per sample", wordLength);

  if (OMGetLRCDCompressedLength(obj, &compressedLength))
    DumpLong(tab_level+2, "Length of compressed bit stream", compressedLength);

  if (OMGetLRCDIndexPresent(obj, &indexPresent) && indexPresent)
    {
      sprintf(outstr, "Block Index: exists");
      display(tab_level+2, outstr);
    }
}
#endif

static void DumpAIFCDATAObject(short tab_level, OMObject obj)

{
  int  nsampframes;
  short nchannels;
  short samplesize;
  char comm[4];
  char ieeerate[10];
  double r;
  UID id;
  unsigned int  offset;

  DisplayObjectHeader(tab_level, obj);

  if (OMGetAIFCMobID(obj, &id))
    DumpUID(tab_level+1, id);

  display(tab_level+1, "AIFC Form data");

  offset = OMGetValueFileOffset(obj, OMAIFCData, OMVarLenBytes);
  sprintf(outstr,"Media Data file offset : %1lu", offset);
  display(tab_level+2, outstr);


  if (OMGetAIFCData(obj, (void *)comm, 12L, 4L) != 4)
    fatal("Error reading COMM chunk tag");

  if (dumpdata)
    {

      assert((Boolean)(comm[0] == 'C' && comm[1] == 'O' && comm[2] == 'M' && comm[3] == 'M'),
	     "COMM chunk not found");

      display(tab_level+1, "COMM");

      if (OMGetAIFCData(obj, (void *)&nchannels, 20L, 2L) != 2)
	fatal("Error reading nchannels");
      sprintf(outstr,"NumChannels : %1d", nchannels);
      display(tab_level+2, outstr);

      if (OMGetAIFCData(obj, (void *)&nsampframes, 22L, 4L) != 4)
	fatal("Error reading nsampframes");
      sprintf(outstr,"NumSampleFrames : %1ld", nsampframes);
      display(tab_level+2, outstr);

      if (OMGetAIFCData(obj, (void *)&samplesize, 26L, 2L) != 2)
	fatal("Error reading samplesize");
      sprintf(outstr,"Sample Size : %1d", samplesize);
      display(tab_level+2, outstr);

      if (OMGetAIFCData(obj, (void *)ieeerate, 28L, 10L) != 10)
	fatal("Error reading ieeerate");

      r =  ConvertFromIeeeExtended(ieeerate);
      sprintf(outstr,"Rate : %f", r);
      display(tab_level+2, outstr);
    }

}

static char *GetTiffTagName(u_short code)
{
  switch (code) {
  case TIFFTAG_IMAGEWIDTH:    return("IMAGEWIDTH");
  case TIFFTAG_IMAGELENGTH:    return("IMAGELENGTH");
  case TIFFTAG_BITSPERSAMPLE:    return("BITSPERSAMPLE");
  case TIFFTAG_COMPRESSION:    return("COMPRESSION");
  case TIFFTAG_PHOTOMETRIC:    return("PHOTOMETRIC");
  case TIFFTAG_THRESHHOLDING:    return("THRESHHOLDING");
  case TIFFTAG_CELLWIDTH:    return("CELLWIDTH");
  case TIFFTAG_CELLLENGTH:    return("CELLLENGTH");
  case TIFFTAG_FILLORDER:    return("FILLORDER");
  case TIFFTAG_DOCUMENTNAME:    return("DOCUMENTNAME");
  case TIFFTAG_IMAGEDESCRIPTION:    return("IMAGEDESCRIPTION");
  case TIFFTAG_MAKE:    return("MAKE");
  case TIFFTAG_MODEL:    return("MODEL");
  case TIFFTAG_STRIPOFFSETS:    return("STRIPOFFSETS");
  case TIFFTAG_ORIENTATION:    return("ORIENTATION");
  case TIFFTAG_SAMPLESPERPIXEL:    return("SAMPLESPERPIXEL");
  case TIFFTAG_ROWSPERSTRIP:    return("ROWSPERSTRIP");
  case TIFFTAG_STRIPBYTECOUNTS:    return("STRIPBYTECOUNTS");
  case TIFFTAG_MINSAMPLEVALUE:    return("MINSAMPLEVALUE");
  case TIFFTAG_MAXSAMPLEVALUE:    return("MAXSAMPLEVALUE");
  case TIFFTAG_XRESOLUTION:    return("XRESOLUTION");
  case TIFFTAG_YRESOLUTION:    return("YRESOLUTION");
  case TIFFTAG_PLANARCONFIG:    return("PLANARCONFIG");
  case TIFFTAG_PAGENAME:    return("PAGENAME");
  case TIFFTAG_XPOSITION:    return("XPOSITION");
  case TIFFTAG_YPOSITION:    return("YPOSITION");
  case TIFFTAG_FREEOFFSETS:    return("FREEOFFSETS");
  case TIFFTAG_FREEBYTECOUNTS:    return("FREEBYTECOUNTS");
  case TIFFTAG_GRAYRESPONSEUNIT:    return("GRAYRESPONSEUNIT");
  case TIFFTAG_GRAYRESPONSECURVE:    return("GRAYRESPONSECURVE");
  case TIFFTAG_GROUP3OPTIONS:    return("GROUP3OPTIONS");
  case TIFFTAG_GROUP4OPTIONS:    return("GROUP4OPTIONS");
  case TIFFTAG_RESOLUTIONUNIT:    return("RESOLUTIONUNIT");
  case TIFFTAG_PAGENUMBER:    return("PAGENUMBER");
  case TIFFTAG_COLORRESPONSEUNIT:    return("COLORRESPONSEUNIT");
  case TIFFTAG_TRANSFERFUNCTION:    return("TRANSFERFUNCTION");
  case TIFFTAG_SOFTWARE:    return("SOFTWARE");
  case TIFFTAG_DATETIME:    return("DATETIME");
  case TIFFTAG_ARTIST:    return("ARTIST");
  case TIFFTAG_HOSTCOMPUTER:    return("HOSTCOMPUTER");
  case TIFFTAG_PREDICTOR:    return("PREDICTOR");
  case TIFFTAG_WHITEPOINT:    return("WHITEPOINT");
  case TIFFTAG_PRIMARYCHROMATICITIES:    return("PRIMARYCHROMATICITIES");
  case TIFFTAG_COLORMAP:    return("COLORMAP");
  case TIFFTAG_HALFTONEHINTS:    return("HALFTONEHINTS");
  case TIFFTAG_TILEWIDTH:    return("TILEWIDTH");
  case TIFFTAG_TILELENGTH:    return("TILELENGTH");
  case TIFFTAG_TILEOFFSETS:    return("TILEOFFSETS");
  case TIFFTAG_TILEBYTECOUNTS:    return("TILEBYTECOUNTS");
  case TIFFTAG_BADFAXLINES:    return("BADFAXLINES");
  case TIFFTAG_CLEANFAXDATA:    return("CLEANFAXDATA");
  case TIFFTAG_CONSECUTIVEBADFAXLINES:    return("CONSECUTIVEBADFAXLINES");
  case TIFFTAG_INKSET:    return("INKSET");
  case TIFFTAG_INKNAMES:    return("INKNAMES");
  case TIFFTAG_DOTRANGE:    return("DOTRANGE");
  case TIFFTAG_TARGETPRINTER:    return("TARGETPRINTER");
  case TIFFTAG_EXTRASAMPLES:    return("EXTRASAMPLES");
  case TIFFTAG_SAMPLEFORMAT:    return("SAMPLEFORMAT");
  case TIFFTAG_SMINSAMPLEVALUE:    return("SMINSAMPLEVALUE");
  case TIFFTAG_SMAXSAMPLEVALUE:    return("SMAXSAMPLEVALUE");
  case TIFFTAG_JPEGPROC:    return("JPEGPROC");
  case TIFFTAG_JPEGIFOFFSET:    return("JPEGIFOFFSET");
  case TIFFTAG_JPEGIFBYTECOUNT:    return("JPEGIFBYTECOUNT");
  case TIFFTAG_JPEGRESTARTINTERVAL:    return("JPEGRESTARTINTERVAL");
  case TIFFTAG_JPEGLOSSLESSPREDICTORS:    return("JPEGLOSSLESSPREDICTORS");
  case TIFFTAG_JPEGPOINTTRANSFORM:    return("JPEGPOINTTRANSFORM");
  case TIFFTAG_JPEGQTABLES:    return("JPEGQTABLES");
  case TIFFTAG_JPEGDCTABLES:    return("JPEGDCTABLES");
  case TIFFTAG_JPEGACTABLES:    return("JPEGACTABLES");
  case TIFFTAG_YCBCRCOEFFICIENTS:    return("YCBCRCOEFFICIENTS");
  case TIFFTAG_YCBCRSUBSAMPLING:    return("YCBCRSUBSAMPLING");
  case TIFFTAG_YCBCRPOSITIONING:    return("YCBCRPOSITIONING");
  case TIFFTAG_REFERENCEBLACKWHITE:    return("REFERENCEBLACKWHITE");
  case TIFFTAG_MATTEING:    return("MATTEING");
  case TIFFTAG_DATATYPE:    return("DATATYPE");
  case TIFFTAG_IMAGEDEPTH:    return("IMAGEDEPTH");
  case TIFFTAG_TILEDEPTH:    return("TILEDEPTH");
  case 34432:  return("FRAMEOFFSETS");
  case 34433:  return("FRAMEBYTECOUNTS");
  case 34434:  return("FRAMELAYOUT");
  case 34435:  return("FRAMERATE");
  case 34436:  return("JPEGQTABLES16");
  default:
    return("Unknown TIFF tag");
  }
}

static void DumpTIFFDATAObject(short tab_level, OMObject obj)

{
  u_long first_IFD;
  u_short tiff_id, byteorder;
  short num_entries;
  int  i, n, data_offset;
  u_short code, type;
  int  nvals, val;
  UID id;
  ByteOrder_t SaveByteOrder;
  char buffer[128];
  unsigned int  offset;


#define t_assert(c) \
  {if (!c) {printf("Error reading tiff header\n"); \
	      ByteOrder = SaveByteOrder; return;}}

  SaveByteOrder = ByteOrder;
  DisplayObjectHeader(tab_level, obj);

  tab_level++;

  if (OMGetTIFFMobID(obj, &id))
    DumpUID(tab_level, id);

  SaveByteOrder = ByteOrder;

  sprintf(outstr, "Length of Data block: %1ld", OMLengthVarLenBytesWP(obj, OMTIFFData));
  display(tab_level, outstr);

  offset = OMGetValueFileOffset(obj, OMTIFFData, OMVarLenBytes);
  sprintf(outstr,"Media Data file offset : %1lu", offset);
  display(tab_level, outstr);

  if (dumpdata)
    {
      t_assert(OMGetTIFFData(obj, &buffer, 0/* offset */, 4));

      byteorder = GetUShort(&buffer[0]);

      if (byteorder == 0x4d4d)
	ByteOrder = Motorola;
      else if (byteorder == 0x4949)
	ByteOrder = Intel;
      else
	fatal("Unknown byte ordering specified");

      tiff_id = GetUShort(&buffer[2]);

      sprintf(outstr, "TIFF id: %2d, ByteOrder: %s", tiff_id,
	      (ByteOrder == Motorola ? "MM" : "II"));
      display(tab_level, outstr);

      t_assert(OMGetTIFFData(obj, buffer, 4/* offset */, sizeof(u_long)));
      first_IFD = GetULong(buffer);
      sprintf(outstr, "Location of IFD entries = %1ld", first_IFD);
      display(tab_level, outstr);

      t_assert(OMGetTIFFData(obj, &buffer, first_IFD, sizeof(u_short)));
      num_entries = GetUShort(buffer);
      if ((num_entries < 10) || (25 < num_entries))
	{
	  printf("Error reading num_entries in TIFF IFD: %1d\n", num_entries);
	}

      n = num_entries;
      sprintf(outstr, "Number of IFD entries = %1ld", n);
      display(tab_level, outstr);

      data_offset = first_IFD + 2;

      for (i = 0; i < n; i++)
	{
	  t_assert(OMGetTIFFData(obj, &buffer, data_offset, 12));
	  data_offset += 12;
	  code = GetUShort(buffer);
	  type = GetUShort(&buffer[2]);
	  if (type > 5) type = 0; /* bad type value, don't read past string table */
	  nvals = GetULong(&buffer[4]);
	  val = GetULong(&buffer[8]);

	  sprintf(outstr, "%s", GetTiffTagName(code));
	  display(tab_level+1, outstr);

	  sprintf(outstr, "code = %1u type = %s N = %1ld V = %1ld",
		  code, typestring[type], nvals, val);
	  display(tab_level+2, outstr);
	}
      ByteOrder = SaveByteOrder;
    }
}


static void DumpWAVEDescriptor(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpMDFLProperties(tab_level+1, obj);

}

static void DumpAIFDProperties(short tab_level, OMObject obj)
{
  int  nsampframes;
  short nchannels;
  short samplesize;
  char comm[4];
  char ieeerate[10];
  double r;

  DumpMDFLProperties(tab_level+1, obj);

  if (OMGetAIFDSummary(obj, (void *)comm, 12L, 4L) != 4)
    fatal("Error reading COMM chunk tag");

  if (dumpdesc)
    {
      int  offset;

  	  offset = OMGetValueFileOffset(obj, OMAIFDSummary, OMVarLenBytes);
  	  sprintf(outstr,"Media Descriptor file offset : %1lu", offset);
  	  display(tab_level+2, outstr);

      assert((Boolean)(comm[0] == 'C' && comm[1] == 'O' && comm[2] == 'M' && comm[3] == 'M'),
	     "COMM chunk not found");

      display(tab_level+1, "COMM");

      if (OMGetAIFDSummary(obj, (void *)&nchannels, 20L, 2L) != 2)
	fatal("Error reading nchannels");
      sprintf(outstr,"NumChannels : %1d", nchannels);
      display(tab_level+2, outstr);

      if (OMGetAIFDSummary(obj, (void *)&nsampframes, 22L, 4L) != 4)
	fatal("Error reading nsampframes");
      sprintf(outstr,"NumSampleFrames : %1ld", nsampframes);
      display(tab_level+2, outstr);

      if (OMGetAIFDSummary(obj, (void *)&samplesize, 26L, 2L) != 2)
	fatal("Error reading samplesize");
      sprintf(outstr,"Sample Size : %1d", samplesize);
      display(tab_level+2, outstr);

      if (OMGetAIFDSummary(obj, (void *)ieeerate, 28L, 10L) != 10)
	fatal("Error reading ieeerate");

      r =  ConvertFromIeeeExtended(ieeerate);
      sprintf(outstr,"Rate : %f", r);
      display(tab_level+2, outstr);
    }
}
static void DumpAIFCDescriptor(short tab_level, OMObject obj)

{

  DisplayObjectHeader(tab_level, obj);

  DumpAIFDProperties(tab_level, obj);
}

static void DumpTIFDProperties(short tab_level, OMObject obj)
{

  u_long first_IFD;
  u_short tiff_id, byteorder;
  short num_entries;
  int  i, n, data_offset;
  u_short code, type;
  int  nvals, val;
  ByteOrder_t SaveByteOrder;
  char buffer[128];

  omfBool iu, ic;
  JPEGTableIDType jt;
  Long ll, tl;

  tab_level++;

#ifdef t_assert
  #undef t_assert
  #endif
#define t_assert(c) \
  {if (!c) {printf("Error reading tiff descriptor header\n"); \
      ByteOrder = SaveByteOrder; return;}}

  DumpMDFLProperties(tab_level+1, obj);

  if (OMGetTIFDIsUniform(obj, &iu))
    {
      sprintf(outstr, "Is Uniform : %s", (iu ? "T" : "F"));
      display(tab_level+1, outstr);
    }

  if (OMGetTIFDIsContiguous(obj, &ic))
    {
      sprintf(outstr, "Is Contiguous : %s", (ic ? "T" : "F"));
      display(tab_level+1, outstr);
    }

  if (OMGetTIFDLeadingLines(obj, &ll))
    {
      sprintf(outstr, "Leading Lines : %1ld", ll);
      display(tab_level+1, outstr);
    }

  if (OMGetTIFDTrailingLines(obj, &tl))
    {
      sprintf(outstr, "Trailing Lines : %1ld", tl);
      display(tab_level+1, outstr);
    }

  if (OMGetTIFDJPEGTableID(obj, &jt))
    {
      sprintf(outstr, "JPEG Table ID : %1ld", jt);
      display(tab_level+1, outstr);
    }

  tab_level++;
  SaveByteOrder = ByteOrder;

  sprintf(outstr, "Length of Summary block: %1ld", OMLengthVarLenBytesWP(obj, OMTIFDSummary));
  display(tab_level, outstr);

  t_assert(OMGetTIFDSummary(obj, &buffer, 0/* offset */, 4));

  if (dumpdesc)
    {
      byteorder = GetUShort(&buffer[0]);

      if (byteorder == 0x4d4d)
	ByteOrder = Motorola;
      else if (byteorder == 0x4949)
	ByteOrder = Intel;
      else
	fatal("Unknown byte ordering specified");

      tiff_id = GetUShort(&buffer[2]);

      sprintf(outstr, "TIFF id: %2d, ByteOrder: %s", tiff_id,
	      (ByteOrder == Motorola ? "MM" : "II"));
      display(tab_level, outstr);

      t_assert(OMGetTIFDSummary(obj, buffer, 4/* offset */, sizeof(u_long)));
      first_IFD = GetULong(buffer);
      sprintf(outstr, "Location of IFD entries = %1ld", first_IFD);
      display(tab_level, outstr);

      t_assert(OMGetTIFDSummary(obj, &buffer, /*first_IFD*/8, sizeof(u_short)));
      num_entries = GetUShort(buffer);
      if ((num_entries < 10) || (25 < num_entries))
	{
	  printf("Error reading num_entries in TIFF IFD: %1d\n", num_entries); \
	  }

      n = num_entries;
      sprintf(outstr, "Number of IFD entries = %1ld", n);
      display(tab_level, outstr);

      data_offset = 10;

      for (i = 0; i < n; i++)
	{
	  t_assert(OMGetTIFDSummary(obj, &buffer, data_offset, 12));
	  data_offset += 12;
	  code = GetUShort(buffer);
	  type = GetUShort(&buffer[2]);
	  if (type > 5) type = 0; /* bad type value, don't read past string table */
	  nvals = GetULong(&buffer[4]);
	  val = GetULong(&buffer[8]);

	  sprintf(outstr, "%s", GetTiffTagName(code));
	  display(tab_level, outstr);

	  sprintf(outstr, "code = %1u type = %s N = %1ld V = %1ld",
		  code, typestring[type], nvals, val);
	  display(tab_level+1, outstr);
	}

      ByteOrder = SaveByteOrder;

    }

}

static void DumpTIFFDescriptor(short tab_level, OMObject obj)
{

  DisplayObjectHeader(tab_level, obj);
  DumpTIFDProperties(tab_level, obj);

}

static void DumpUNXLProperties(short tab_level, OMObject obj)

{
  char path[512];

  if (OMGetUNXLPathName(obj, path, (Long)512))
    DumpString(tab_level+1, "Path Name", path);
}

static void DumpUNIXFILELOCATORObject(short tab_level, OMObject obj)

{
  DisplayObjectHeader(tab_level, obj);

  DumpUNXLProperties(tab_level, obj);

}

static void DumpMACLProperties(short tab_level, OMObject obj)

{
  char path[512];
  Short vref;
  Long  dirID;

  if (OMGetMACLVRef(obj, &vref))
    DumpShort(tab_level+1, "VRef", vref);

  if (OMGetMACLDirID(obj, &dirID))
    DumpLong(tab_level+1, "Dir ID", dirID);

  if (OMGetMACLFileName(obj, path, 512L))
    DumpString(tab_level+1, "Path Name", path);

}

static void DumpMACFILELOCATORObject(short tab_level, OMObject obj)

{
  DisplayObjectHeader(tab_level, obj);

  DumpDOSLProperties(tab_level, obj);

}

static void DumpDOSLProperties(short tab_level, OMObject obj)

{
  char path[512];

  if (OMGetDOSLPathName(obj, path, (Long)512))
    DumpString(tab_level+1, "Path Name", path);

}
static void DumpDOSFILELOCATORObject(short tab_level, OMObject obj)

{
  DisplayObjectHeader(tab_level, obj);

  DumpDOSLProperties(tab_level, obj);

}


#define TKFXLeftLengthProperty "OMFI:TKFX:MC:LeftLength"
#define TKFXRightLengthProperty "OMFI:TKFX:MC:RightLength"
#define TKFXGlobalInfoProperty "OMFI:TKFX:MC:GlobalInfo"
#define TKFXkfListProperty "OMFI:TKFX:MC:KeyFrameList"

#define TNFXLeftLengthProperty "OMFI:TNFX:MC:LeftLength"
#define TNFXRightLengthProperty "OMFI:TNFX:MC:RightLength"
#define TNFXGlobalInfoProperty "OMFI:TNFX:MC:GlobalInfo"

#define kfListProperty "OMFI:TNFX:MC:KeyFrameList"
#define kfCurrentProperty "OMFI:TNFX:MC:GlobalInfo.kfCurrent"
#define kfSmoothProperty "OMFI:TNFX:MC:GlobalInfo.kfSmooth"
#define colorItemProperty "OMFI:TNFX:MC:GlobalInfo.colorItem"
#define qualityProperty "OMFI:TNFX:MC:GlobalInfo.quality"
#define isReversedProperty "OMFI:TNFX:MC:GlobalInfo.isReversed"
#define aspectOnProperty "OMFI:TNFX:MC:GlobalInfo.aspectOn"

static void DumpTNFXProperties(short tab_level, OMObject obj)

{
  int  lval;
  short sval;
  omfBool bval;

  DumpTRANProperties(tab_level+1, obj);

  if (OMReadLongWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),TNFXLeftLengthProperty),&lval))
    DumpLong(tab_level+1, "Left Length", lval);

  if (OMReadLongWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),TNFXRightLengthProperty), &lval))
    DumpLong(tab_level+1, "Right Length", lval);

  if (OMReadLongWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),kfCurrentProperty), &lval))
    DumpLong(tab_level+1, "kfCurrent", lval);

  if (OMReadLongWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),kfSmoothProperty), &lval))
    DumpLong(tab_level+1, "kfSmooth", lval);

  if (OMReadShortWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),colorItemProperty), &sval))
    DumpShort(tab_level+1, "color item", sval);

  if (OMReadShortWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),qualityProperty), &sval))
    DumpShort(tab_level+1, "quality", sval);

  if (OMReadBooleanWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),isReversedProperty), &bval))
    DumpBoolean(tab_level+1, "isReversed", bval);

  if (OMReadBooleanWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),aspectOnProperty), &bval))
    DumpBoolean(tab_level+1, "aspectOn", bval);

}

static void DumpTRANSEFFECTObject(short tab_level, OMObject obj)

{
  DisplayObjectHeader(tab_level, obj);

  DumpTNFXProperties(tab_level+1, obj);
}

static void DumpPVOLProperties(short tab_level, OMObject obj)

{
#define LevelProperty "OMFI:PVOL:Level"
#define PanProperty "OMFI:PVOL:Pan"
#define SuppressValProperty "OMFI:PVOL:SuppressValidation"

  int  lval;
  omfBool bval;

  DumpTRKGProperties(tab_level+1, obj);

  if (OMReadLongWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),LevelProperty),&lval))
    DumpLong(tab_level+1, "Level", lval);

  if (OMReadLongWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),PanProperty), &lval))
    DumpLong(tab_level+1, "Pan", lval);

  if (OMReadBooleanWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),SuppressValProperty), &bval))
    DumpLong(tab_level+1, "suppressValidation", bval);
}


static void DumpPANVOLEFFECTObject(short tab_level, OMObject obj)

{
  DisplayObjectHeader(tab_level, obj);

  DumpPVOLProperties(tab_level, obj);

}

static void DumpTKFXProperties(short tab_level, OMObject obj)

{
  int  lval;
  short sval;
  omfBool bval;

  DumpTRKGProperties(tab_level+1, obj);

  if (OMReadLongWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),TKFXLeftLengthProperty),&lval))
    DumpLong(tab_level+1, "Left Length", lval);

  if (OMReadLongWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),TKFXRightLengthProperty), &lval))
    DumpLong(tab_level+1, "Right Length", lval);

  if (OMReadLongWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),kfCurrentProperty), &lval))
    DumpLong(tab_level+1, "kfCurrent", lval);

  if (OMReadLongWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),kfSmoothProperty), &lval))
    DumpLong(tab_level+1, "kfSmooth", lval);

  if (OMReadShortWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),colorItemProperty), &sval))
    DumpShort(tab_level+1, "color item", sval);

  if (OMReadShortWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),qualityProperty), &sval))
    DumpShort(tab_level+1, "quality", sval);

  if (OMReadBooleanWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),isReversedProperty), &bval))
    DumpBoolean(tab_level+1, "isReversed", bval);

  if (OMReadBooleanWP(obj, (OMProperty)CMRegisterProperty(OMGetObjectContainer(obj),aspectOnProperty), &bval))
    DumpBoolean(tab_level+1, "aspectOn", bval);

}

static void DumpTRACKEFFECTObject(short tab_level, OMObject obj)

{
  DisplayObjectHeader(tab_level, obj);

  DumpTKFXProperties(tab_level+1, obj);
}


