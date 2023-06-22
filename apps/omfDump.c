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

/*******************
 * VERSION HISTORY *
 *******************/
/* JeffB: Rev 2 changes
 * 		Added printing of:
 *			user attributes (Media Composer bin comments)
 * 			IDNT audit trail information
 *			Q table information (optional)
 *			Frame size information (optional)
 */

#include "masterhd.h"
#if PORT_SYS_MAC
#include <Types.h>
#include <console.h>
#include <StandardFile.h>
#include "MacSupport.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
#include "omMobMgt.h"
#include "omMedia.h"
#include "tiff.h"
#include "omPvt.h"
#include "InfoSupp.h"

#ifdef MAC_DRAG_DROP
#include "Main.h"
#endif

/***********
 * DEFINES *
 ***********/
#define TRUE 1
#define FALSE 0
#define DEBUG_DUMP 0
#define STRINGSIZE 1024

/********************/
/* GLOBAL VARIABLES */
/********************/
const omfInt32 omfiAppVersion = 2;
omfBool omfiPrintDefines = 0;
omfBool omfiSemCheckOn = 0;
omfBool omfiPrintTypes = 0;
omfBool omfiVerboseMode = 0;
omfBool omfiDumpData = 0;

static char *typestring[] = {
  "NONE    ",
  "BYTE    ",
  "ASCII   ",
  "SHORT   ",
  "LONG    ",
  "RATIONAL" };

/**************
 * PROTOTYPES *
 **************/
static omfErr_t dumpObject(omfHdl_t    fileHdl,
						   omfObject_t obj,
						   omfInt32       level,
                           void        *data);
static omfErr_t dumpHead(omfHdl_t    fileHdl,
						 omfObject_t head);
static omfErr_t dumpObjProps(omfHdl_t    file,
							 omfObject_t obj,
							 omfInt32	  level);
static void dumpProp(omfHdl_t        file,
                     omfObject_t     obj,
                     omfProperty_t   prop,
                     omfType_t       type,
					 char            *printName,
					 omfInt32        level);
static omfBool isPrintableObj(omfHdl_t file,
							  omfObject_t obj,
                              void *data);
static void usage(char *cmdName, 
				  omfProductVersion_t toolkitVers,
				  omfInt32 appVers);
static void printHeader(char *name, 
						omfProductVersion_t toolkitVers, 
						omfInt32 appVers);
void printSpace(omfInt32 level);
void printPropSpace(omfInt32 level);
static omfErr_t DumpClassDictionary(omfHdl_t file,
								omfObject_t head);
static omfErr_t DumpMobs(omfHdl_t fileHdl,
						 omfObject_t head,
						 omfInt32 numMobs,
						 omfMobKind_t mobKind);

static omfErr_t DumpMediaData(omfHdl_t fileHdl,
							  omfObject_t head,
							  omfInt32 numMedia);
static omfErr_t DumpDefinitions(omfHdl_t fileHdl,
								omfObject_t head);
static omfErr_t getMobString(omfHdl_t fileHdl,
							 omfObject_t obj,
							 char *mobString);

static char *GetTiffTagName(omfUInt16 code);
static omfInt16 GetUShort(omfUInt16 byteorder,
						  char *data);
static omfUInt32 GetULong(omfUInt16 byteorder,
						  char *data);
static void DumpTIFFData(omfHdl_t file,
						 omfInt32 level, 
						 omfObject_t obj,
						 omfProperty_t mediaProp);
static omfErr_t dumpMDES(omfHdl_t    file,
							 omfObject_t obj,
							 omfInt32	  level,
							 char		*prefixTxt);

/****************
 * MAIN PROGRAM *
 ****************/
int main(int argc, char *argv[])
{
    omfSessionHdl_t	session;
    omfHdl_t fileHdl;
	omfObject_t head;
    omfMobObj_t mob;
    omfInt32 loop, numMobs, numMedia;
    omfIterHdl_t mobIter = NULL;
    omfInt32 matches;
    omfInt32 i;
#if PORT_SYS_MAC && !defined(MAC_DRAG_DROP)
#if OMFI_MACSF_STREAM
	Point where;
	SFReply fs;
	SFTypeList fTypes;
	char namebuf[255];
#endif  
#endif
	fileHandleType fh;
	char *fname = NULL, *printfname = NULL, *cmdname = NULL;
	omfFileRev_t fileRev;
	char errBuf[256];
	omfObject_t mediaObj = NULL;
	omfClassID_t objClass;
	omfProperty_t idProp, mediaIndex, tiffProp;
	omfObjIndexElement_t objIndex;
	omfErr_t omfError = OM_ERR_NONE;


#if PORT_SYS_MAC && !defined(MAC_DRAG_DROP)
	argc = ccommand(&argv); 
	strcpy( (char*) argv[0],"omfDump");
	MacInit();  
#if OMFI_MACSF_STREAM
	where.h = 20;
	where.v = 40;
#endif
#endif
	for (i=1 ; i < argc; i++)
	  {
		if (!strncmp("-def",argv[i],3))
		  omfiPrintDefines = 1;
		else if (!strncmp("-types",argv[i],2))
		  omfiPrintTypes = 1;
		else if (!strncmp("-check", argv[i],2))
		  omfiSemCheckOn = 1;
		else if (!strncmp("-dumpdata", argv[i], 3))
		  omfiDumpData = 1;
		else if (!strncmp("-verbose", argv[i],2))
		  {
			omfiPrintTypes = 1;
			omfiPrintDefines = 1;
			omfiVerboseMode = 1;
			omfiDumpData = 1;
		  }
		else if (!strncmp("-", argv[i],1))
		  usage(argv[0], omfiToolkitVersion, omfiAppVersion);
		else
		  fname = argv[i];			/* This will not be used on Macintosh */
	  }

#if OMFI_MACSF_STREAM
	/* Get the file name and open */
	fTypes[0] = 'OMFI';
	SFGetFile(where,  NULL, NULL, -1, fTypes, NULL, &fs);
	if (! fs.good)
	  exit(0);
	fh = (fileHandleType)&fs;

	strncpy(namebuf, (char *)&(fs.fName[1]), (size_t)fs.fName[0]);
	printfname = namebuf;
#else
	if ((argc < 2) || (argc >= 6))
	  usage(argv[0], omfiToolkitVersion, omfiAppVersion);

	fh = (fileHandleType)fname;
	printfname = fname;
#endif

    XPROTECT(NULL)
      {
		cmdname = argv[0];    

		/* print OMF version */
		printHeader(cmdname, omfiToolkitVersion, omfiAppVersion);

		CHECK(omfsBeginSession(0, &session));
		CHECK(omfmInit(session));

	    printf("\n***** OMF File: %s *****\n", printfname);
		CHECK(omfsOpenFile(fh, session, &fileHdl));
		omfsFileGetRev(fileHdl, &fileRev);

#if OMFI_ENABLE_SEMCHECK
		if (omfiSemCheckOn)
		  {
			omfsSemanticCheckOn(fileHdl);
		  }
		else /* default - semantic checking off */
		  {
			omfsSemanticCheckOff(fileHdl);
		  }
#endif

		if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
		  {
			printf("OMFI File Revision: 1.0\n\n");
			mediaIndex = OMMediaData;
			tiffProp = OMTIFFData;
			idProp = OMObjID;
		  }
		else if (fileRev == kOmfRev2x)
		  {
			printf("OMFI File Revision: 2.0\n\n");
			mediaIndex = OMHEADMediaData;
			tiffProp = OMTIFFData;
			idProp = OMOOBJObjClass;
		  }
		else
		  printf("OMFI File Revision: UNKNOWN\n\n");

	    /* Print the IDNT audit trail */
		omfPrintAuditTrail(fileHdl);

	    /* Dump the rest of the HEAD object */
		CHECK(omfsGetHeadObject(fileHdl, &head));
		CHECK(dumpHead(fileHdl, head));

		/* Iterate over all mobs in the file */
		CHECK(omfiIteratorAlloc(fileHdl, &mobIter));
		CHECK(omfiGetNumMobs(fileHdl, kAllMob, &numMobs));
		for (loop = 0; loop < numMobs; loop++) 
		  {
			CHECK(omfiGetNextMob(mobIter, NULL, &mob));

			CHECK(omfiMobMatchAndExecute(fileHdl, mob, 1,
										 isPrintableObj, NULL,
										 dumpObject, NULL,
										 &matches));
			printf("\n"); 
		  } /* for */

		omfiIteratorDispose(fileHdl, mobIter);
		mobIter = NULL;

		/* Dump Media Data Objects */
		if (omfiDumpData)
		  {
			if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
			  numMedia = omfsLengthObjIndex(fileHdl, head, mediaIndex);
			else
			  numMedia = omfsLengthObjRefArray(fileHdl, head, mediaIndex);
			for (loop = 1; loop <= numMedia; loop++) 
			  {
				if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
				  {
					CHECK(omfsGetNthObjIndex(fileHdl, head, mediaIndex, 
											 &objIndex, loop));
					mediaObj = objIndex.Mob;
				  }
				else
				  {
					CHECK(omfsGetNthObjRefArray(fileHdl, head, mediaIndex, 
												&mediaObj, loop));
				  }
				CHECK(dumpObject(fileHdl, mediaObj, 1, NULL));
				CHECK(omfsReadClassID(fileHdl, mediaObj, idProp, objClass));
				if (!strncmp(objClass, "TIFF", (size_t)4))
				  DumpTIFFData(fileHdl, 2, mediaObj, tiffProp);
			  }
		  }

		omfError = omfsCloseFile(fileHdl);
		omfsEndSession(session);
		printf("omfDump completed successfully.\n");
      } /* XPROTECT */

    XEXCEPT
      {
		printf("***ERROR: %d: %s\n", XCODE(), 
			   omfsGetExpandedErrorString(fileHdl, XCODE(), 
										  sizeof(errBuf), errBuf));
		return(-1);
      }
    XEND;
    return(0);
}

/*************
 * FUNCTIONS *
 *************/
static omfBool isPrintableObj(omfHdl_t file,
							  omfObject_t obj,
                              void *data)
{
  if (obj)
    return(TRUE);
  else
    return(FALSE);
}

static void dumpProp(omfHdl_t        file,
                     omfObject_t     obj,
                     omfProperty_t   prop,
                     omfType_t       type,
					 char            *printName,
					 omfInt32        level)
{
  omfErr_t omfError = OM_ERR_NONE;
  char     printString[STRINGSIZE];
  char     printString2[STRINGSIZE];
  omfBool printProp = TRUE;
  omfUniqueName_t typeString;
  omfAttributeKind_t	attrKind;
  strncpy(printString2, "\0", 1);
  strncpy(printString, "\0", 1);
  strncpy(typeString, "\0", 1);

  omfiGetPropertyTypename(file, type, OMUNIQUENAME_SIZE, typeString);

  switch(type) {
      case OMNoType: {
         sprintf(printString, typeString);
         sprintf(printString2, "\n");
         break;
      }
      case OMAttrKind: {
        sprintf(printString, typeString);
		 omfError = omfsReadAttrKind(file, obj, prop, &attrKind);
		 if(attrKind == kOMFIntegerAttribute)
         	sprintf(printString2, " : Integer\n");
		 else if(attrKind == kOMFStringAttribute)
         	sprintf(printString2, " : String\n");
		 else if(attrKind == kOMFObjectAttribute)
         	sprintf(printString2, " : Object\n");
         else
         	sprintf(printString2, "\n");
         break;
      }
      case OMBoolean: {
		omfBool data;
         sprintf(printString, typeString);
         omfError = omfsReadBoolean(file, obj, prop, &data);
         if (data)
             sprintf(printString2, " : TRUE\n");
         else
             sprintf(printString2, " : FALSE\n");
         break;
      }
      case OMInt8: {
		 char data;
         sprintf(printString, typeString);
         omfError = omfsReadInt8(file, obj, prop, &data);
         if(isprint(data))
         	sprintf(printString2, " : %d = '%c'\n", data,data); 
         else
         	sprintf(printString2, " : %d\n", data); 
         break;
      }
      case OMCharSetType: {
         omfCharSetType_t data;
         omfError = omfsReadCharSetType(file, obj, prop, &data);
         sprintf(printString, typeString);
         sprintf(printString2, " : %hd\n", data);
         break;
      }
      case OMEdgeType: {
         omfEdgeType_t  data;
         omfError = omfsReadEdgeType(file, obj, prop, &data);
         sprintf(printString, typeString);
         if (data == kEtNull)
           sprintf(printString2, " : kEtNull\n");
         else if (data == kEtKeycode)
           sprintf(printString2, " : kEtKeycode\n");
         else if (data == kEtEdgenum4)
           sprintf(printString2, " : kEtEdgenum4\n");
         else if (data == kEtEdgenum5)
           sprintf(printString2, " : kEtEdgenum5\n");
         else
           sprintf(printString2, " : PRIVATE value %d \n",data);
         break;
      }
      case OMExactEditRate: {
         omfRational_t  data;	
         omfError = omfsReadExactEditRate( file, obj, prop, &data);
         sprintf(printString, typeString);
         sprintf(printString2, " : %ld/%ld\n", data.numerator, data.denominator);
         break;
      }
      case OMFilmType: {
		omfFilmType_t data;
		omfError = omfsReadFilmType(file, obj, prop, &data);
		sprintf(printString, typeString);
		if (data == kFtNull)
		  sprintf(printString2, " : kFtNull\n");
		else if (data == kFt35MM)
		  sprintf(printString2, " : kFt35MM\n");
		else if (data == kFt16MM)
		  sprintf(printString2, " : kFt16MM\n");
		else if (data == kFt8MM)
		  sprintf(printString2, " : kFt8MM\n");
		else if (data == kFt65MM)
		  sprintf(printString2, " : kFt65MM\n");
		else
		  sprintf(printString2, " : PRIVATE value %d\n",data);
		break;
      }
      case OMJPEGTableIDType: {
		omfJPEGTableID_t  data;
		omfError = omfsReadJPEGTableIDType(file, obj, prop, &data);
		sprintf(printString, typeString);
		sprintf(printString2, " : %ld\n", data);
		break;
      }
      case OMInt32: {
         omfInt32 data;
         omfError = omfsReadInt32(file, obj, prop, &data);
         sprintf(printString, typeString);
         sprintf(printString2, " : %ld\n", data);
         break;
      }
      case OMInt64: {
         omfInt64	data;
         char		posStr[64];
         
		 omfError = OMReadBaseProp(file, obj, prop, OMInt64, sizeof(data), &data);
		 omfsInt64ToString(data, 10, sizeof(posStr), posStr);
         sprintf(printString, typeString);
         sprintf(printString2, " : %s\n", posStr);
         break;
      }
      case OMMobIndex: {
         omfInt32 data = omfsLengthObjIndex(file, obj, prop);
         sprintf(printString, typeString);
         sprintf(printString2, "\n");
         break;
      }
      case OMObjRefArray: {
         omfInt32        i = 1;
         omfObject_t  data; 
         char         tmpString[128];
		 omfInt32     strCount = 0;

         omfInt32 numObjRef = omfsLengthObjRefArray(file, obj, prop);

         sprintf(printString, typeString);
		 if (numObjRef > 0)
		   {
			 omfError = omfsGetNthObjRefArray(file, obj, prop, &data, 1);
			 sprintf(printString2, " : (%1ld", 
					 omfsGetBentoID(file, data) & 0x0000ffff);
			 strCount = strlen(printString2);
		   }
		 for (i = 2; i <= numObjRef; i++) {
            omfError = omfsGetNthObjRefArray(file, obj, prop, &data, i);
            sprintf(tmpString, ", %1ld", 
					omfsGetBentoID(file, data) & 0x0000ffff);
			strCount += (strlen(tmpString)-1);  /* Don't count null */
			if (strCount >= (STRINGSIZE-5))
			  {
				strcat(printString2, "...");
				break;
			  }
			else
			  strcat(printString2, tmpString);
         }
         sprintf(tmpString, ")\n");
         strcat(printString2, tmpString);
/*!!!		{
 //        char		debug[256];
//			omfiGetPropertyName(file, prop, sizeof(debug), debug);
//			if(strstr(debug, "SessionAttrs") != 0 || strstr(debug, "ObjAttributes") || strstr(debug, "AttrRefs"))
//				dumpObjProps(file, data, level+1);
//		}
*/
         break;
      }
      case OMObjRef: {
         omfObject_t data;

         omfError = omfsReadObjRef(file, obj, prop, &data);
         sprintf(printString, typeString);
         sprintf(printString2, " : (%1ld)\n", 
				 omfsGetBentoID(file, data) & 0x0000ffff);
         break;
      }
      case OMObjectTag: {
         char data[5];
         omfError = omfsReadObjectTag(file, obj, prop, (omfTagPtr_t)data);
         data[4] = '\0';
         sprintf(printString, typeString);
         sprintf(printString2, " : %s\n", data);
         break;
      }
      case OMPhysicalMobType: {
         omfPhysicalMobType_t  data;
         omfError = omfsReadPhysicalMobType(file, obj, prop, &data);
         sprintf(printString, typeString);
		 if (data == PT_NULL)
		   sprintf(printString2, " : PT_NULL (%hd)\n", data);
		 else if (data == PT_FILE_MOB)
		   sprintf(printString2, " : PT_FILE_MOB (%hd)\n", data);
		 else if (data == PT_TAPE_MOB)
		   sprintf(printString2, " : PT_TAPE_MOB (%hd)\n", data);
		 else if (data == PT_FILM_MOB)
		   sprintf(printString2, " : PT_FILM_MOB (%hd)\n", data);
		 else if (data == PT_NAGRA_MOB)
		   sprintf(printString2, " : PT_NAGRA_MOB (%hd)\n", data);
         sprintf(printString2, " : %hd\n", data);
         break;
      }
      case OMInt16: {
         omfInt16 data;
         omfError = omfsReadInt16(file, obj, prop, &data);
         sprintf(printString, typeString);
         sprintf(printString2, " : %hd\n", data);
         break;
      }
      case OMString: {
         char data[256]; 
         omfInt32 maxsize = sizeof(data);

         omfError = omfsReadString(file, obj, prop, (char *)data, maxsize);
         if (omfError == OM_ERR_PROP_NOT_PRESENT) {
            strncpy(data, "\0", 1);
         }
         sprintf(printString, typeString);
         sprintf(printString2, " : %s\n", data);
         break;
      }
      case OMTimeStamp: {
         omfTimeStamp_t  data;
		 if (omfiVerboseMode)
		   {
			 omfError = omfsReadTimeStamp(file, obj, prop, &data);
			 sprintf(printString, typeString);
			 sprintf(printString2, " : (TimeVal = %lu , IsGMT = %ld)\n", 
					 data.TimeVal, data.IsGMT);
		   }
		 else
		   printProp = FALSE;
         break;
      }
      case OMTrackType: {
		 omfTrackType_t data;
         omfError = omfsReadTrackType(file, obj, prop, &data);
         sprintf(printString, typeString);
		 if (data == TT_NULL)
		   sprintf(printString2, " : TT_NULL (%hd)\n", data);
		 else if (data == TT_PICTURE)
		   sprintf(printString2, " : TT_PICTURE (%hd)\n", data);
		 else if (data == TT_SOUND)
		   sprintf(printString2, " : TT_SOUND (%hd)\n", data);
		 else if (data == TT_TIMECODE)
		   sprintf(printString2, " : TT_TIMECODE (%hd)\n", data);
		 else if (data == TT_EDGECODE)
		   sprintf(printString2, " : TT_EDGECODE (%hd)\n", data);
		 else if (data == TT_ATTRIBUTE)
		   sprintf(printString2, " : TT_ATTRIBUTE (%hd)\n", data);
		 else if (data == TT_EFFECTDATA)
		   sprintf(printString2, " : TT_EFFECTDATA (%hd)\n", data);
         break;
      }
      case OMUID: {
         omfUID_t  data;
		 /* Already printed out the MobID */
		 if (prop != OMMOBJMobID)
		   {
			 omfError = omfsReadUID(file, obj, prop, &data);
			 sprintf(printString, typeString);
			 sprintf(printString2, " : %ld.%lu.%lu\n", data.prefix, 
					 data.major, data.minor);
		   }
         break;
      }
      case OMUInt8: {
         unsigned char data;
         omfError = omfsReadUInt8(file, obj, prop, &data);
         sprintf(printString, typeString);
         if(isprint(data))
         	sprintf(printString2, " : %d = '%c'\n", data,data); 
         else
         	sprintf(printString2, " : %d\n", data); 
         break;
      }
      case OMUInt32: {
         omfUInt32 data;
         omfError = omfsReadUInt32(file, obj, prop, &data);
         sprintf(printString, typeString);
         sprintf(printString2, " : %lu\n", data);
         break;
      }
      case OMUInt16: {
         omfUInt16 data;
         omfError = omfsReadUInt16(file, obj, prop, &data);
         sprintf(printString, typeString);
         sprintf(printString2, " : %hu\n", data);
         break;
      }
      case OMUsageCodeType: {
         omfUsageCode_t  data;
         omfError = omfsReadUsageCodeType(file, obj, prop, &data);
         sprintf(printString, typeString);
		 if (data == UC_NULL)
		   sprintf(printString2, " : UC_NULL (%ld)\n", data);
		 else if (data == UC_MASTERMOB)
		   sprintf(printString2, " : UC_MASTERMOB (%ld)\n", data);
		 else if (data == UC_PRECOMPUTE)
		   sprintf(printString2, " : UC_PRECOMPUTE (%ld)\n", data);
		 else if (data == UC_SUBCLIP)
		   sprintf(printString2, " : UC_SUBCLIP (%ld)\n", data);
		 else if (data == UC_EFFECT)
		   sprintf(printString2, " : UC_EFFECT (%ld)\n", data);
		 else if (data == UC_GROUP)
		   sprintf(printString2, " : UC_GROUP (%ld)\n", data);
		 else if (data == UC_GROUPOOFTER)
		   sprintf(printString2, " : UC_GROUPOOFTER (%ld)\n", data);
		 else if (data == UC_MOTION)
		   sprintf(printString2, " : UC_MOTION (%ld)\n", data);
		 else if (data == UC_PRECOMPUTE_FILE)
		   sprintf(printString2, " : UC_PRECOMPUT_FILE (%ld)\n", data);
         break;
      }
      case OMVarLenBytes: {
         char data;
         omfUInt32 bytesRead;
         omfInt32 maxsize = 64;
         omfPosition_t offset;

		 omfsCvtInt32toInt64(0, &offset);
         omfError = omfsReadVarLenBytes(file, obj, prop, offset, maxsize, 
										&data, &bytesRead);
         sprintf(printString, typeString);
         sprintf(printString2, "\n");
         break;
      }
      case OMVersionType: {
         omfVersionType_t  data;
         omfError = omfsReadVersionType(file, obj, prop, &data);
         sprintf(printString, typeString);
         if ((data.major == 2) && (data.minor == 0))
            sprintf(printString2, " : 2.0\n");
         else if (data.major == 1)
            sprintf(printString2, " : 1.0\n");
         else if ((data.major == 2) && (data.minor > 0)
                  || (data.major > 2))
            sprintf(printString2, " : (>2) \n");
         break;
      }
      case OMPosition32:
	  case OMPosition64: {
         omfPosition_t data;
		 char tmpBuf[48];
         
         omfError = omfsReadPosition(file, obj, prop, &data);
         sprintf(printString, typeString);
         omfsInt64ToString(data, 10, sizeof(tmpBuf), tmpBuf);
		 sprintf(printString2, " : %s\n", tmpBuf);
         break;
      }
      case OMLength32:  {	/* omfsReadLength will choose which type to read.  Dump should
      						 * always print the exact type, so call OMReadBaseProp
      						 */
 		 omfInt32 	data;
		 omfError = OMReadBaseProp(file, obj, prop, OMLength32, 
							sizeof(data), &data);
         sprintf(printString, typeString);
         sprintf(printString2, " : %ld\n", data);
         break;
      }

	  case OMLength64: {	/* omfsReadLength will choose which type to read.  Dump should
      						 * always print the exact type, so call OMReadBaseProp
      						 */
 		 omfLength_t 	data;
		 char tmpBuf[48];

		 omfError = OMReadBaseProp(file, obj, prop, OMLength64, 
							sizeof(data), &data);
         sprintf(printString, typeString);
         omfsInt64ToString(data, 10, sizeof(tmpBuf), tmpBuf);
         sprintf(printString2, " : %s\n", tmpBuf);
         break;
      }
      case OMClassID: {
         omfClassID_t data;
         char         tmpClass[5];
		 omfFileRev_t	fileRev;
		 omfProperty_t	idProp;

		 if (omfiVerboseMode)
		   {
			 tmpClass[4] = '\0';

			 omfError = omfsFileGetRev(file, &fileRev);
			 if(omfError != OM_ERR_NONE)
			   break;
			 
			 if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
			   idProp = OMObjID;
			 else
			   idProp = OMOOBJObjClass;
			 omfError = omfsReadClassID(file, obj, idProp, data);
			 strncpy(tmpClass, data, 4);
			 sprintf(printString, typeString);
			 sprintf(printString2, " : %4s\n", (char *)tmpClass);
		   }
		 else
		   printProp = FALSE;
         break;
      }
      case OMUniqueName: {
         omfUniqueName_t data;
         omfError = omfsReadUniqueName(file, obj, prop, data, 
									   OMUNIQUENAME_SIZE);
         sprintf(printString, typeString);
         sprintf(printString2, " : %s\n", data);
         break;
      }
      case OMFadeType: {
         omfFadeType_t  data;
         omfError = omfsReadFadeType(file, obj, prop, &data);
         sprintf(printString, typeString);
         if (data == kFadeNone)
           sprintf(printString2, " : kFadeNone\n");
         else if (data == kFadeLinearAmp)
           sprintf(printString2, " : kFadeLinearAmp\n");
         else if (data == kFadeLinearPower)
           sprintf(printString2, " : kFadeLinearPower\n");
         else
           sprintf(printString2, " : PRIVATE value %d\n",data);
         break;
      }
      case OMInterpKind: {
         omfInterpKind_t  data;
         omfError = omfsReadInterpKind(file, obj, prop, &data);
         sprintf(printString, typeString);
         if (data == kConstInterp)
           sprintf(printString2, " : kConstInterp\n");
         else if (data == kLinearInterp)
           sprintf(printString2, " : kLinearInterp\n");
         else
           sprintf(printString2, " : PRIVATE value %d\n",data);
         break;
      }
      case OMArgIDType: {
         omfArgIDType_t data;
         omfError = omfsReadArgIDType(file, obj, prop, &data);
         sprintf(printString, typeString);
         sprintf(printString2, " : %ld\n", data);
         break;
      }
      case OMEditHintType: {
         omfEditHint_t  data;
         omfError = omfsReadEditHint(file, obj, prop, &data);
         sprintf(printString, typeString);
         if (data == kNoEditHint)
           sprintf(printString2, " : kNoEditHint\n");
         else if (data == kProportional)
           sprintf(printString2, " : kProportional\n");
         else if (data == kRelativeLeft)
           sprintf(printString2, " : kRelativeLeft\n");
         else if (data == kRelativeRight)
           sprintf(printString2, " : kRelativeRight\n");
         else if (data == kRelativeFixed)
           sprintf(printString2, " : kRelativeFixed\n");
         else
           sprintf(printString2, " : PRIVATE value %d\n",data);
         break;
      }
      case OMDataValue: {
         omfLength_t maxsize;
         char		*dkData = NULL;
         int 		dkData32;
         char		dkData8, *ptr;
         omfUInt32 bytesRead, maxPrint, n;
         omfUInt32	maxSize32, fileSize32;
         omfPosition_t offset;
		 omfRational_t	*ratPtr;
		 omfErr_t		errStatus;
		 
		 omfsCvtInt32toInt64(0, &offset);
         maxsize = omfsLengthDataValue(file, obj, prop);
         omfsTruncInt64toUInt32(maxsize, &maxSize32);
         if((prop == OMCVALValue) || (prop == OMCTLPValue))		/* Read MC Private dkData for effects */
         {
          	omfDDefObj_t ddef;
          	char		ddefName[64];
         	
			fileSize32 = maxSize32;
			omfError = omfsReadObjRef(file, obj,
								(prop == OMCVALValue ? OMCPNTDatakind: OMCTLPDatakind),
								&ddef);
			omfError = omfsReadUniqueName(file, ddef, OMDDEFDatakindID, ddefName, sizeof(ddefName));
  			if(strcmp(ddefName, "omfi:data:Rational") == 0 && (maxSize32 == 4))
			{
				omfiDatakindLookup(file,
				       (omfUniqueNamePtr_t) "omfi:data:Int32",
				       &ddef,
				       &errStatus);
			}
							
        	if(maxSize32 > 4)
      			dkData = omfsMalloc(maxSize32);
       		else if(maxSize32 == 1)
       			dkData = (char *)&dkData8;
       		else
        		dkData = (char *)&dkData32;
      		
       		omfError = omfsReadDataValue(file, obj, prop, ddef, dkData, offset, 
				     maxSize32, &bytesRead);
				
			if(strcmp(ddefName, "omfi:data:Int32") == 0)
       			sprintf(printString2, " : %ld\n", dkData32);
			else if(strcmp(ddefName, "omfi:data:UInt32") == 0)
       			sprintf(printString2, " : %lu\n", dkData32);
  			else if(strcmp(ddefName, "omfi:data:Boolean") == 0)
       			sprintf(printString2, " : %s\n", (dkData32 ? "TRUE" : "FALSE"));
  			else if(strcmp(ddefName, "omfi:data:Rational") == 0)
       		{
       			if(fileSize32 == 4)	/* Composer bug */
        			sprintf(printString2, " : %ld/100\n", dkData32);
        		else
        		{
      				ratPtr = (omfRational_t *)dkData;
       				sprintf(printString2, " : %ld/%ld\n", ratPtr->numerator, ratPtr->denominator);
       			}
       		}
  			else if((strcmp(ddefName, "omfi:data:EffectGlobals") == 0) || 
  			        (strcmp(ddefName, "omfi:data:KeyFrame") == 0))
  			{
   				sprintf(printString2, " : %ld bytes (", bytesRead);
				ptr = printString2 + strlen(printString2);
     			maxPrint = (bytesRead < 32 ? bytesRead : 32);
     			for(n = 0; n < maxPrint; n++)
     			{
     				if((n % 16) == 0)
     				{
     					sprintf(ptr, "\n                        ");
     					ptr += strlen(ptr);
     				}
     				sprintf(ptr, "%02x ", dkData[n] & 0xFF);
     				ptr += strlen(ptr);
     			}
     			if(bytesRead != maxPrint)
     				sprintf(ptr, "...)\n");
     			else
     				sprintf(ptr, ")\n");
  			}
      		else
        		sprintf(printString2, "\n");
	   
       		if(maxSize32 > 4)
				omfsFree(dkData);
 	    }
        else
        	sprintf(printString2, "\n");
/* NOTE: omfsReadDataValue needs a datakind argument.  We don't
 *       know what the datakind is at this point!
 */
#if 0
        omfError = omfsReadDataValue(file, obj, prop, &data, offset, 
				     maxSize32, &bytesRead);
#endif
         sprintf(printString, typeString);
         break;

      }
      case OMTapeCaseType: {
		omfTapeCaseType_t data;
		omfError = omfsReadTapeCaseType(file, obj, prop, &data);
		sprintf(printString, typeString);
		switch (data)
		  {
		  case kTapeCaseNull:
			sprintf(printString2, " : kTapeCaseNull\n");
			break;
		  case kThreeFourthInchVideoTape: 
			sprintf(printString2, " : kThreeFourthInchVideoTape\n");
			break;
		  case kVHSVideoTape:
			sprintf(printString2, " : kVHSVideoTape\n");
			break;
		  case k8mmVideoTape:
			sprintf(printString2, " : k8mmVideoTape\n");
			break;
		  case kBetacamVideoTape:
			sprintf(printString2, " : kBetacamVideoTape\n");
			break;
		  case kCompactCassette:
			sprintf(printString2, " : kCompactCassette\n");
			break;
		  case kDATCartridge:
			sprintf(printString2, " : kDATCartridge\n");
			break;
		  case kNagraAudioTape:
			sprintf(printString2, " : kNagraAudioTape\n");
			break;
		  default:
			sprintf(printString2, " : PRIVATE value %d\n",data);
			break;
		  }
		break;
      }
      case OMVideoSignalType: {
		omfVideoSignalType_t data;
		omfError = omfsReadVideoSignalType(file, obj, prop, &data);
		sprintf(printString, typeString);
		switch (data)
		  {
		  case kVideoSignalNull:
			sprintf(printString2, " : kVideoSignalNull\n");
			break;
		  case kNTSCSignal:
			sprintf(printString2, " : kNTSCSignal\n");
			break;
		  case kPALSignal:
			sprintf(printString2, " : kPALSignal\n");
			break;
		  case kSECAMSignal:
			sprintf(printString2, " : kSECAMSignal\n");
			break;
		  default:
			sprintf(printString2, " : PRIVATE value %d\n",data);
			break;
		  }
		break;
      }
      case OMTapeFormatType: {
		omfTapeFormatType_t data;
		omfError = omfsReadTapeFormatType(file, obj, prop, &data);
		sprintf(printString, typeString);
		switch (data)
		  {
		  case kTapeFormatNull:
			sprintf(printString2, " : kTapeFormatNull\n");
			break;
		  case kBetacamFormat:
			sprintf(printString2, " : kBetacamFormat\n");
			break;
		  case kBetacamSPFormat:
			sprintf(printString2, " : kBetacamSPFormat\n");
			break;
		  case kVHSFormat:
			sprintf(printString2, " : kVHSFormat\n");
			break;
		  case kSVHSFormat:
			sprintf(printString2, " : kSVHSFormat\n");
			break;
		  case k8mmFormat:
			sprintf(printString2, " : k8mmFormat\n");
			break;
		  case kHi8Format:
			sprintf(printString2, " : kHi8Format\n");
			break;
		  default:
			sprintf(printString2, " : PRIVATE value %d\n",data);
			break;
		  }
		break;
      }
      case OMCompCodeArray: {
		 char pixComps[MAX_NUM_RGBA_COMPS];
		 omfPosition_t			zeroPos;

         sprintf(printString, typeString);
		 omfsCvtInt32toPosition(0, zeroPos);
		 OMReadProp(file, obj, prop, zeroPos,
					   kNeverSwab, type, 
					   MAX_NUM_RGBA_COMPS, (void *)pixComps);
		 sprintf(printString2, " : %s\n", pixComps);
         break;
      }
      case OMCompSizeArray: {
         sprintf(printString, typeString);
         sprintf(printString2, "\n");
         break;
      }
      case OMColorSitingType: {
         omfInt16		siting;
  		 omfPosition_t	zeroPos;
       
         sprintf(printString, typeString);
		 omfsCvtInt32toPosition(0, zeroPos);
		 OMReadProp(file, obj, prop,
							   zeroPos, kSwabIfNeeded, type,
							   sizeof(omfInt16), (void *)&siting);
         sprintf(printString2, " : %ld\n", siting);
         break;
      }
      case OMRational: {
         omfRational_t  data;
         sprintf(printString, typeString);
         omfError = omfsReadRational(file, obj, prop, &data);
         sprintf(printString2, " : %ld/%ld\n", data.numerator, data.denominator);
         break;
      }
      case OMCompressionType: {
         sprintf(printString, typeString);
         sprintf(printString2, "\n");
         break;
      }
      case OMLayoutType: {
		 omfFrameLayout_t data = kNoLayout;
		 omfError = omfsReadLayoutType(file, obj, prop, &data);
         sprintf(printString, typeString);
         if (data == kNoLayout)
           sprintf(printString2, " : kNoLayout\n");
         else if (data == kFullFrame)
           sprintf(printString2, " : kFullFrame\n");
         else if (data == kOneField)
           sprintf(printString2, " : kOneField\n");
         else if (data == kSeparateFields)
           sprintf(printString2, " : kSeparateFields\n");
         else
           sprintf(printString2, " : PRIVATE value %d\n",data);
         break;
      }
      case OMPulldownKindType: {
		 omfPulldownKind_t data;
		 omfError = omfsReadPulldownKindType(file, obj, prop, &data);
         sprintf(printString, typeString);
         if (data == kOMFTwoThreePD)
           sprintf(printString2, " : 3-2 Pulldown\n");
         else if (data == kOMFPALPD)
           sprintf(printString2, " : PAL Pulldown\n");
         else if (data == kOMFOneToOneNTSC)
           sprintf(printString2, " : One-to-One (NTSC)\n");
         else if (data == kOMFOneToOnePAL)
           sprintf(printString2, " : One-to-One (NTSC)\n");
         else
           sprintf(printString2, " : PRIVATE value %d\n",data);
         break;
      }
      case OMPulldownDirectionType: {
		 omfPulldownDir_t data;
		 omfError = omfsReadPulldownDirectionType(file, obj, prop, &data);
         sprintf(printString, typeString);
         if (data == kOMFTapeToFilmSpeed)
           sprintf(printString2, " : Videotape to Film Pulldown\n");
         else if (data == kOMFFilmToTapeSpeed)
           sprintf(printString2, " : Film to Videotape Pulldown\n");
         else
           sprintf(printString2, " : PRIVATE value %d\n",data);
         break;
      }
      case OMInt32Array:
      case OMPosition32Array:
	  case OMPosition64Array:  {
         omfInt32       i = 1;
         omfObject_t 	data; 
         char     		tmpString[128];
		 omfInt32  		strCount = 0;
         omfInt32		numEntries, entrySize;
		 omfPosition_t	offset;
		 
		 entrySize = (type == OMPosition64Array ? sizeof(omfInt64) : sizeof(omfUInt32));
		 omfError = omfsGetArrayLength(file, obj, prop, type,
		 								entrySize, &numEntries);

         sprintf(printString, typeString);
		 if (numEntries > 0)
		   {
			 omfError = OMGetNthPropHdr(file, obj, prop, 1, type,
										entrySize, &offset);
			 omfError = OMReadProp(file, obj, prop, offset, kSwabIfNeeded,
										type, entrySize, &data);
			 sprintf(printString2, " : (%ld", data);
			 strCount = strlen(printString2);
		   }
		 for (i = 2; i <= numEntries; i++) {
			 omfError = OMGetNthPropHdr(file, obj, prop, i, type,
										entrySize, &offset);
			 omfError = OMReadProp(file, obj, prop, offset, kSwabIfNeeded,
										type, entrySize, &data);
            sprintf(tmpString, ", %1ld", data);
			strCount += (strlen(tmpString));
			if (strCount >= (STRINGSIZE-5))
			  {
				strcat(printString2, "...");
				break;
			  }
			else
			  strcat(printString2, tmpString);
         }
         sprintf(tmpString, ")\n");
         strcat(printString2, tmpString);
         break;
      }
      case OMDirectionCode:
      case OMColorSpace:
      case OMPhaseFrameType: {
         omfInt16 data;
         omfError = OMReadBaseProp(file, obj, prop, type,  sizeof(omfInt16), &data);
         sprintf(printString, typeString);
         sprintf(printString2, " : %hd\n", data);
         break;
      }
      case OMProductVersion: {
         omfProductVersion_t vers;
         char				*typeName;
         
         omfError = omfsReadProductVersionType(file, obj, prop,
											&vers);
         sprintf(printString, typeString);	
		if(vers.type == kVersionUnknown)
			typeName = "Unknown";
		else if(vers.type == kVersionReleased)
			typeName = "Released";
		else if(vers.type == kVersionDebug)
			typeName = "Pre-Release Debug";
		else if(vers.type == kVersionPatched)
			typeName = "Patched";
		else if(vers.type == kVersionBeta)
			typeName = "Pre-Release";
		else if(vers.type == kVersionPatched)
			typeName = "Private Build";
	
		if(vers.type == kVersionReleased)
			sprintf(printString2, "(%d, %d, %d)\n",
				vers.major, vers.minor, vers.tertiary);
		else
			sprintf(printString2, "(%d, %d, %d) %s %d\n",
				vers.major, vers.minor, vers.tertiary,
				typeName, vers.patchLevel);
         break;
      }
      default:
		if (omfiVerboseMode)
		  {
			omfiGetPropertyTypename(file, type,  sizeof(printString),  
									(omfUniqueNamePtr_t)printString);
			sprintf(printString2, " not handled by this program. \n");
		  }
		else
		  printProp = FALSE;
        break;
  }

  if (printProp)
	{
	  printPropSpace(level);
	  if (omfiPrintTypes)
	  {
		printf("%s", printName);
	  	if(strlen(printString) < 20)
	  		printf("%-20.20s  %s", printString, printString2);
	  	else if(strlen(printString) < 28)
	  		printf("%-28.28s  %s", printString, printString2);
	  	else
	  		printf("  %-64.64s  %s", printString, printString2);
	  }
	  else
	  {
		  printf("%-16.16s", printName);
	  	  printf("%s",printString2);
	  }
	}

  return;

}

static omfErr_t dumpObjProps(omfHdl_t    file,
							 omfObject_t obj,
							 omfInt32	  level)
{
  omfProperty_t   prop = OMNoProperty, mProp = OMNoProperty; 
  omfProperty_t attrProp = OMNoProperty;
  omfIterHdl_t    propIter = NULL, attrIter = NULL;
  omfType_t       type = OMNoType;
  omfType_t attrType = OMNoType;
  omfUniqueName_t datakindName, currName;
  omfInt32           i, n, numObjRef, numAttrs, numAttrNest;
  omfDDefObj_t datakind = NULL;
  char *namePtr;
  omfObject_t mdes = NULL, attr = NULL, attb = NULL, attrNest = NULL;
  omfProperty_t slotProp = OMNoProperty, mdesProp = OMNoProperty;
  omfProperty_t segProp = OMNoProperty, cpntProp = OMNoProperty;
  omfProperty_t idProp = OMNoProperty;
  omfType_t slotType = OMNoType, mdesType = OMNoType;
  omfPosition_t origin;
  omfInt32 nameSize = 64, origin32;
  char name[64], printName[64];
  omfFileRev_t	fileRev;
  omfTrackID_t trackID;
  omfClassID_t data;
  omfErr_t        omfError = OM_ERR_NONE;
  omfAttributeKind_t	attrKind;
  
  omfError = omfsFileGetRev(file, &fileRev);
  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
	idProp = OMObjID;
  else
	idProp = OMOOBJObjClass;

  omfiIteratorAlloc(file, &propIter);
  omfiGetNextProperty(propIter, obj, &prop, &type);
  while ((omfError == OM_ERR_NONE) && prop) 
	{
      omfiGetPropertyName(file, prop, OMUNIQUENAME_SIZE, currName);
      if (currName) 
		{
		  if (!strncmp("OMFI:",currName,5))
			{
			  namePtr = &currName[5];
			  /* If not verbose, don't print the Class prefix to prop names */
			  if (!omfiVerboseMode)
				namePtr += 5;
			}
		  else
			namePtr = currName;

		  /* Print the Datakind name if datakind object */
		  switch (prop)
			{
			case OMCPNTDatakind:
			  omfError = omfiComponentGetInfo(file, obj, &datakind, NULL);
			  omfError = omfiDatakindGetName(file, datakind,
											 OMUNIQUENAME_SIZE, datakindName);
			  printPropSpace(level);
			  if (omfiVerboseMode)
				printf("%-18.18s", "CPNT:Datakind");
			  else 
				printf("%-18.18s", "Datakind");
			  printf(" : %s\n", datakindName);
			  break;

			case OMMOBJMobID:
			case OMOOBJObjClass:
			case OMObjID:
			  break;

 			case OMCPNTAttributes:
			case OMMOBJUserAttributes:
			  attrProp = prop;
			  attrType = type;
			  break;

			case OMMOBJSlots:
			case OMTRKGTracks:
			case OMEFFEEffectSlots:
			case OMNESTSlots:
			  slotProp = prop;
			  slotType = type;
			  break;

			case OMMSLTSegment:
			  segProp = prop;
			  break;

			case OMTRAKTrackComponent:
			  cpntProp = prop;
			  break;

			case OMSMOBMediaDescription:
			case OMMOBJPhysicalMedia:
			  mdesProp = prop;
			  mdesType = type;
			  break;

			case OMMSLTTrackDesc:
			  /* Print out the Track descriptor on a 2.0 Mob slot now */
			  /* NULL for Mob since 2.0 file */
			  omfError = omfiTrackGetInfo(file, NULL /* mob */, obj, 
										  NULL, nameSize, name,
										  &origin, &trackID, NULL);
			  omfsTruncInt64toInt32(origin, &origin32);
			  printPropSpace(level);
			  if (strncmp(name, "\0", 1))
				{
				  if (omfiVerboseMode)
					printf("%-18.18s", "TRKD:Name");
				  else
					printf("%-18.18s", "Name");
				  printf(" : %s\n", name);
				  printPropSpace(level);
				}
			  
			  if (omfiVerboseMode)
				printf("%-18.18s", "TRKD:Origin");
			  else
				printf("%-18.18s", "Origin");
			  printf(" : %ld\n", origin32);
			  printPropSpace(level);
			  if (omfiVerboseMode)
				printf("%-18.18s", "TRKD:TrackID");
			  else
				printf("%-18.18s", "TrackID");
			  printf(" : %ld\n", trackID);
			  break;

			default:
				{
				char	debug[256];
				omfiGetPropertyName(file, prop, sizeof(debug), debug);
				if(strstr(debug, "SessionAttrs"))
			 	{
			 		 attrProp = prop;
			  		attrType = type;
			  		break;
			  	}
			  	}
			  if(strlen(namePtr) < 24)
			  	sprintf(printName, "%-24.24s ", namePtr);
			  else if(strlen(namePtr) < 32)
			  	sprintf(printName, "%-32.32s ", namePtr);
			  else if(strlen(namePtr) < (sizeof(printName)-1))
			  	sprintf(printName, "%s ", namePtr);
			  else
			  	sprintf(printName, "%-64.64s ", namePtr);
			  dumpProp(file, obj, prop, type, printName, level);
			  break;
			} /* switch */
		}
	  omfError = omfiGetNextProperty(propIter, obj, &prop, &type);
	} /* Property loop */

  /*** Print the child objects after the other properties ***/

  /* Process Attributes now */
  if (attrProp != OMNoProperty)
	{
	  omfiIteratorAlloc(file, &attrIter);
	  omfError = omfsReadObjRef(file, obj, attrProp, &attr);
	  numAttrs = omfsLengthObjRefArray(file, attr, OMATTRAttrRefs);
	  if (numAttrs > 0)
		{
		  printPropSpace(level);
		  if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		  	printf("Attributes ->\n");
		  else
		  	printf("UserAttributes ->\n");
		}
	  for (i=1; i<=numAttrs; i++)
		{
		  printSpace(level+1);
		  omfsGetNthObjRefArray(file, attr, OMATTRAttrRefs, &attb, i);
		  omfError = omfsReadClassID(file, attb, idProp, data);
		  printf("%-4.4s [BentoID: %1ld]\n", (char *)data,
				 omfsGetBentoID(file, attb) & 0x0000ffff);
		  dumpObjProps(file, attb, level+2);
		  omfError = omfsReadAttrKind(file, attb, OMATTBKind, &attrKind);
		  if(attrKind == kOMFObjectAttribute)
		  {
		  	numAttrNest = omfsNumAttributes(file, attb, OMATTBObjAttribute);
		  	printSpace(level+1);
		  	printf("num nested attributes = %ld\n", numAttrNest);
		  	for(n = 1; n <= numAttrNest; n++)
		  	{
				omfsGetNthAttribute(file, attb, OMATTBObjAttribute, &attrNest, n);
				printSpace(level+2);
			    omfError = omfsReadClassID(file, attrNest, idProp, data);
			    printf("%-4.4s [BentoID: %1ld]\n", (char *)data,
					 omfsGetBentoID(file, attrNest) & 0x0000ffff);
				dumpObjProps(file, attrNest, level+3);
				printf("\n");
		  	}
		  }
		}
	  printf("\n");
	  omfiIteratorDispose(file, attrIter);
	  attrIter = NULL;
	}

  /* Process Media Descriptor now */
  if (mdesProp != OMNoProperty)
	{
	  	omfError = omfmMobGetMediaDescription(file, obj, &mdes);
		dumpMDES(file,mdes, level+1, "MediaDescription");
	}

  if (segProp != OMNoProperty)
	{
	  printPropSpace(level);
	  if (omfiVerboseMode)
		printf("%-18.18s\n", "MSLT:Segment ->");
	  else
		printf("%-18.18s\n", "Segment ->");
	}

  if (cpntProp != OMNoProperty)
	{
	  printPropSpace(level);
	  if (omfiVerboseMode)
		printf("TRAK:TrackComponent ->\n");
	  else
		printf("TrackComponent ->\n");
	}

  /* Process slots now */
  if (slotProp != OMNoProperty)
	{
	  numObjRef = omfsLengthObjRefArray(file, obj, slotProp);
	  printPropSpace(level);
	  if ((slotProp == OMMOBJSlots) || slotProp == OMTRKGTracks)
		printf("%-18.18s", "Num subtracks");
	  else
		printf("%-18.18s", "Num subslots");
	  printf(" : %ld\n", numObjRef);
	}

  omfiIteratorDispose(file, propIter);
  propIter = NULL;

  return(omfError);
}

static omfErr_t dumpMDES(omfHdl_t    file,
							 omfObject_t mdes,
							 omfInt32	  level,
							 char *prefixTxt)
	{
		char mName[64], *mNamePtr;
		omfIterHdl_t mdesIter = NULL;
		omfErr_t	omfError;
  		omfProperty_t   mProp = OMNoProperty; 
 		omfClassID_t data;
	  	omfFileRev_t	fileRev;
 		omfProperty_t locProp = OMNoProperty, idProp = OMNoProperty;
	  	omfInt32		numLocs, i;
  		omfUniqueName_t propName;
 		omfType_t       mType = OMNoType, locType = OMNoType;
 		omfObject_t		loc = NULL;
	  	
	  omfError = omfsFileGetRev(file, &fileRev);
	  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
		idProp = OMObjID;
	  else
		idProp = OMOOBJObjClass;

	  omfiIteratorAlloc(file, &mdesIter);

	  omfError = omfsReadClassID(file, mdes, idProp, data);
	  printPropSpace(level);
	  printf("%s ->\n", prefixTxt);
	  printf("\n");
	  printSpace(level+1);
	  printf("%-4.4s [BentoID: %1ld]\n", (char *)data,
			 omfsGetBentoID(file, mdes) & 0x0000ffff);

	  omfiGetNextProperty(mdesIter, mdes, &mProp, &mType);
	  while ((omfError == OM_ERR_NONE) && mProp) 
		{
		  omfiGetPropertyName(file, mProp, OMUNIQUENAME_SIZE, propName);
		  if (omfiVerboseMode)
			mNamePtr = propName+5;
		  else
			mNamePtr = propName+10;
		  if(strlen(mNamePtr) < 24)
		  	sprintf(mName, "%-24.24s ", mNamePtr);
		  else if(strlen(mNamePtr) < 32)
		  	sprintf(mName, "%-32.32s ", mNamePtr);
		  else if(strlen(mNamePtr) < (sizeof(mName)-1))
		  	sprintf(mName, "%s ", mNamePtr);
		  else
		  	sprintf(mName, "%-64.64s ", mNamePtr);

		  switch (mProp)
			{
			case OMOOBJObjClass:
			case OMObjID:
			  break;
			  
			case OMMDESLocator:
			  locProp = mProp;
			  locType = mType;
			  break;

			case OMTIFDSummary:
			  if (omfiDumpData)
				DumpTIFFData(file, level+2, mdes, mProp);
			  break;

			default:
			  dumpProp(file, mdes, mProp, mType, mName, level+1);
			  if(mType == OMObjRef)
			  {
			  	omfObject_t	nestedMDES;
			  	
			  	printPropSpace(level+1);
			 	omfError = omfsReadObjRef(file, mdes, mProp, &nestedMDES);
				if (omfError == OM_ERR_NONE)
					dumpMDES(file,  nestedMDES, level+1, "NestedMDES");
			  }
			  break;
			}

		  omfiGetNextProperty(mdesIter, mdes, &mProp, &mType);
		}

	  /* Process locators on Media Descriptors */
	  if (locProp)
		{
		  omfiIteratorClear(file, mdesIter);
		  numLocs = omfsLengthObjRefArray(file, mdes, OMMDESLocator);
		  if (numLocs > 0)
			{
			  printPropSpace(level+1);
			  printf("Locators ->\n");
			}
		  for (i=1; i<=numLocs; i++)
			{
			  printSpace(level+2);
			  omfsGetNthObjRefArray(file, mdes, OMMDESLocator, &loc, i);
			  omfError = omfsReadClassID(file, loc, idProp, data);
			  printf("%-4.4s [BentoID: %1ld]\n", (char *)data,
					 omfsGetBentoID(file, loc) & 0x0000ffff);
			  dumpObjProps(file, loc, level+2);
			  printf("\n");
			}
		}
	  omfiIteratorDispose(file, mdesIter);
	  printf("\n");

  return(omfError);
}
  
static omfErr_t dumpHead(omfHdl_t    fileHdl,
						 omfObject_t head)
{
   omfInt32	       numMobs, numMedia;
   omfClassID_t    objClass;
   char            tmpClass[5];
   char errBuf[256];
   omfUInt16 byteOrder;
   omfVersionType_t fileVersion;
   omfProperty_t   idProp, mediaProp;
   omfFileRev_t	   rev;
   omfErr_t        omfError = OM_ERR_NONE;
	
   tmpClass[4] = '\0';

   XPROTECT(fileHdl) 
	 {
	   /* get File Revision, and check type of head object */
	   omfsFileGetRev(fileHdl, &rev);
	   if ((rev == kOmfRev1x) || (rev == kOmfRevIMA))
		 {
		   idProp = OMObjID;
		   mediaProp = OMMediaData;
		 }
	   else
		 {
		   idProp = OMOOBJObjClass;
		   mediaProp = OMHEADMediaData;
		  }
	   omfError = omfsReadClassID(fileHdl, head, idProp, objClass);
	   if (omfError == OM_ERR_NONE) 
		 {
		   strncpy(tmpClass, objClass, 4);
		   printf("* %4s [BentoID: %1ld]\n", (char *)tmpClass,
				  omfsGetBentoID(fileHdl, head) & 0x0000ffff);
		 } 
	   else if (omfError == OM_ERR_PROP_NOT_PRESENT) /* Not OMF object */ 
		 {
		   printf("UNKNOWN CLASS\n");
		   omfError = OM_ERR_NONE;
		 }

	   /* Print Byte Order */
	   omfsGetByteOrder(fileHdl, head, (omfInt16 *) & byteOrder);
	   printSpace(1);
	   if (byteOrder == MOTOROLA_ORDER) 
		 printf("ByteOrder: BigEndian (MOTOROLA_ORDER)\n");
	   else if (byteOrder == INTEL_ORDER)
		 printf("ByteOrder: LittleEndian (INTEL_ORDER)\n");
	   else 
		 printf("ByteOrder: UNKNOWN_ORDER\n");


	   /* Print Version Number */
	   if (omfsIsPropPresent(fileHdl, head, OMVersion, OMVersionType)) 
		 {
		   omfsReadVersionType(fileHdl, head, OMVersion, &fileVersion);
		   printSpace(1);
		   if ((fileVersion.major == 2) && (fileVersion.minor == 0))
			 printf("Version: 2.0\n");
		   else if (fileVersion.major == 1)
			 printf("Version: 1.0\n");
		   else if ((fileVersion.major == 2) && (fileVersion.minor > 0)
					|| (fileVersion.major > 2))
			 printf("Version: (>2) \n");
		 }
	   else
		 {
		   printf("File Version Property could not be found. \n");
		 }


	   /* If 2.x file, print out the Primary Mob index */
	   if (rev == kOmfRev2x)
		 {
		   CHECK(omfiGetNumMobs(fileHdl, kPrimaryMob, &numMobs)); 
		   printf("\n* OMFI Primary Mobs: %ld\n", numMobs);
		   CHECK(DumpMobs(fileHdl, head, numMobs, kPrimaryMob));
		 }


	   /* Now print all Mobs */
	   CHECK(omfiGetNumMobs(fileHdl, kAllMob, &numMobs)); 
	   printf("\n* OMFI Mobs: %ld\n", numMobs);
	   CHECK(DumpMobs(fileHdl, head, numMobs, kAllMob));


	   if (omfiPrintDefines && (rev == kOmfRev2x))
		 {
		   CHECK(DumpDefinitions(fileHdl, head));
		 }

	   /* Print media data objects in the file */
	   if ((rev == kOmfRev1x) || (rev == kOmfRevIMA))
		 numMedia = omfsLengthObjIndex(fileHdl, head, mediaProp); 
	   else
		 numMedia = omfsLengthObjRefArray(fileHdl, head, mediaProp); 
	   printf("\n* OMFI Media: %ld\n", numMedia);
	   CHECK(DumpMediaData(fileHdl, head, numMedia));

	   /* Now, dump the class dictionary */
	   DumpClassDictionary(fileHdl, head);
	 } /* XPROTECT */

   XEXCEPT 
	 {
	   printf("***ERROR: %ld: %s\n", XCODE(), 
			  omfsGetExpandedErrorString(fileHdl, 
										 XCODE(), sizeof(errBuf), errBuf));
	 }
   XEND;
   return(OM_ERR_NONE);
}  /* dumpHead() */

static omfErr_t dumpObject(omfHdl_t    fileHdl,
						   omfObject_t obj,
						   omfInt32       level,
                           void        *data)
{
   omfClassID_t    objClass;
   char            tmpClass[5];
   omfFileRev_t		fileRev;
   omfProperty_t	idProp;
   omfBool skip = FALSE;
   omfUID_t mobID;
   char mobString[32];
   omfErr_t omfError = OM_ERR_NONE;

   omfsFileGetRev(fileHdl, &fileRev);
   if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
		idProp = OMObjID;
   else
		idProp = OMOOBJObjClass;

   omfError = omfsReadClassID(fileHdl, obj, idProp, objClass);

   if (omfError == OM_ERR_NONE) 
	 {
	   strncpy(tmpClass, objClass, 4);
	   if (omfsIsTypeOf(fileHdl, obj, "MOBJ", &omfError))
		 {
		   printf("* %-4.4s [BentoID: %1ld]\n", (char *)tmpClass,
				  omfsGetBentoID(fileHdl, obj) & 0x0000ffff);
		   omfError = getMobString(fileHdl, obj, mobString);
		   printPropSpace(level);
		   printf("%-18.18s : %s\n", "MobKind", mobString);
		   omfError = omfiMobGetMobID(fileHdl, obj, &mobID);
		   printPropSpace(level);
		   if (omfiVerboseMode)
			 printf("%-18.18s", "MOBJ:MobID");
		   else
			 printf("%-18.18s", "MobID");
		   printf(" : %ld.%lu.%lu\n", mobID.prefix, mobID.major, mobID.minor);
		 }
	   else if (!strncmp(tmpClass, "TIFF", 4) ||
				!strncmp(tmpClass, "AIFC", 4) ||
				!strncmp(tmpClass, "WAVE", 4) ||
				!strncmp(tmpClass, "IDAT", 4) ||
				!strncmp(tmpClass, "JPEG", 4))
		 {
		   printf("* %-4.4s [BentoID: %1ld]\n", (char *)tmpClass,
				  omfsGetBentoID(fileHdl, obj) & 0x0000ffff);
		 }
	   else if (!strncmp(tmpClass, "DDEF", 4))
		 skip = TRUE; /* Skip DDEFs - print out the name when dumping props */
	   else if (!strncmp(tmpClass, "TRKD", 4))
		 skip = TRUE; /* Skip TRKD - print out with MSLT */
	   else if (omfsIsTypeOf(fileHdl, obj, "LOCR", &omfError) ||
				!strncmp(tmpClass, "UNXL", 4) ||
				!strncmp(tmpClass, "MACL", 4) ||
				!strncmp(tmpClass, "DOSL", 4) ||
				!strncmp(tmpClass, "TXTL", 4))
		 skip = TRUE; /* Skip locator objects, print out with MDES */

	   else if (!strncmp(tmpClass, "ATTR", 4) ||
				!strncmp(tmpClass, "ATTB", 4))
		 skip = TRUE; /* Print out attribute arrays with components */

	   else if (omfsIsTypeOf(fileHdl, obj, "MDES", &omfError))
		 skip = TRUE;
	   else if (!strncmp(tmpClass, "RLED", 4))
		 skip = TRUE;
	   else
		 {
		   if (!strncmp(tmpClass, "MSLT", 4) ||
			 (!strncmp(tmpClass, "TRAK", 4)))
			 printf("\n");

		   printSpace(level);
		   printf("%-4.4s [BentoID: %1ld]\n", (char *)tmpClass,
				  omfsGetBentoID(fileHdl, obj) & 0x0000ffff);
		 }
	 } 
   else if (omfError == OM_ERR_PROP_NOT_PRESENT) /* Not OMF object */ 
	 {
	   printf("UNKNOWN CLASS\n");
	   omfError = OM_ERR_NONE;
	 }

   if (!skip)
	 dumpObjProps(fileHdl, obj, level);
   return(omfError);
}

/*********/
/* USAGE */
/*********/
static void usage(char *cmdName, 
		  omfProductVersion_t toolkitVers,
		  omfInt32 appVers)
{
  fprintf(stderr, "%s %ld.%ld.ldr%ld\n", cmdName, toolkitVers.major,
		  toolkitVers.minor, toolkitVers.tertiary,
		  appVers);
  fprintf(stderr, "Usage: %s [-defs]|[-types]|[-verbose][-check][-dumpdata] <filename>\n", cmdName);
  exit(1);
}

/***************/
/* printHeader */
/****************/
static void printHeader(char *name,
			omfProductVersion_t toolkitVers,
			omfInt32 appVers)
{
  fprintf(stderr, "%s %ld.%ld.%ldr%ld\n", name, toolkitVers.major,
		  toolkitVers.minor, toolkitVers.tertiary,
		  appVers);
}


void printSpace(omfInt32 level)
{
  omfInt32 loop;

  for (loop=0; loop<level; loop++)
	{
	  printf("    ");
	}
}

void printPropSpace(omfInt32 level)
{
  omfInt32 loop;

  for (loop=0; loop<level; loop++)
	{
	  printf("    ");
	}
  printf("  ");
}


static omfErr_t getMobString(omfHdl_t fileHdl,
							 omfObject_t obj,
							 char *mobString)
{
  omfErr_t omfError = OM_ERR_NONE;
  omfFileRev_t	fileRev;
  omfPhysicalMobType_t mobKind;
  omfObject_t	mDesc = NULL;

  XPROTECT(fileHdl)
	{
	  if (omfiIsACompositionMob(fileHdl, obj, &omfError))
		sprintf(mobString, "%s\0", "COMPOSITION MOB");
	  else if (omfiIsAMasterMob(fileHdl, obj, &omfError))
		sprintf(mobString, "%s\0", "MASTER MOB");
	  else if (omfiIsAFileMob(fileHdl, obj, &omfError))
		sprintf(mobString, "%s\0", "FILE MOB");
	  else if (omfiIsATapeMob(fileHdl, obj, &omfError))
		sprintf(mobString, "%s\0", "TAPE MOB");
	  else if (omfiIsAFilmMob(fileHdl, obj, &omfError))
		sprintf(mobString, "%s\0", "FILM MOB");
	  else
	  {
		omfsFileGetRev(fileHdl, &fileRev);
		if((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
		{
			omfError = omfsReadObjRef(fileHdl, obj, OMMOBJPhysicalMedia, &mDesc);
			if ((omfError != OM_ERR_NONE) && 
				(omfError != OM_ERR_PROP_NOT_PRESENT))
			  RAISE(omfError);

			if (mDesc)
			  {
				omfError = omfsReadPhysicalMobType(fileHdl, mDesc, OMMDESMobKind,
											  &mobKind);
				/* Release Bento reference, so the useCount is decremented	*/
				/* Actually we don't need to do this here, since we are not	*/
				/* deleting objects in the dumper!							*/
				/*		CMReleaseObject((CMObject)mDesc);					*/
			  }
			else
				mobKind = 0;

			if(mobKind == 5)
				sprintf(mobString, "%s", "SOURCE FILE MOB");
			else
				sprintf(mobString, "%s", "UNKNOWN MOB %d",(int)mobKind );
		}
		else
			sprintf(mobString, "%s", "UNKNOWN MOB");
		}
		
	  if (omfError != OM_ERR_NONE)
		{
		  RAISE(omfError);
		}
	}
  XEXCEPT
	{
	}
  XEND;
  return(OM_ERR_NONE);
}

static omfErr_t DumpMobs(omfHdl_t fileHdl,
								omfObject_t head,
								omfInt32 numMobs,
								omfMobKind_t mobKind)
{
  omfInt32 i;
  omfObject_t tmpMob = NULL;
  omfUID_t mobID;
  omfClassID_t    objClass;
  char            tmpClass[5];
  omfUniqueName_t name;
  omfSearchCrit_t searchCrit;
  omfFileRev_t	   rev;
  omfProperty_t idProp;
  omfIterHdl_t    mobIter = NULL;
  char mobString[32];
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(fileHdl)
	{
	  omfsFileGetRev(fileHdl, &rev);
	  if ((rev == kOmfRev1x) || (rev == kOmfRevIMA))
		idProp = OMObjID;
	  else
		idProp = OMOOBJObjClass;
	  searchCrit.searchTag = kByMobKind;
	  searchCrit.tags.mobKind = mobKind;
	  CHECK(omfiIteratorAlloc(fileHdl, &mobIter));

	  for (i = 1; i <= numMobs; i++) 
		{
		  CHECK(omfiGetNextMob(mobIter, &searchCrit, &tmpMob));
		  omfError = omfsReadClassID(fileHdl, tmpMob, idProp, objClass);
		  omfError = omfiMobGetMobID(fileHdl, tmpMob, &mobID);
		  if (objClass && (omfError == OM_ERR_NONE)) 
			{
			  strncpy(tmpClass, objClass, 4);
			  tmpClass[4] = 0;
			  omfError = omfiMobGetMobID(fileHdl, tmpMob, &mobID);
			  omfError = getMobString(fileHdl, tmpMob, mobString);
			  if (omfiVerboseMode)
				{
				  printSpace(1); 
				  printf("%-16s [BentoID: %1ld]\n", 
						 mobString, 
						 omfsGetBentoID(fileHdl, tmpMob) & 0x0000ffff);
				   
				  omfiGetPropertyName(fileHdl, OMMOBJMobID, 
									  OMUNIQUENAME_SIZE, name);
				  printSpace(1);
				  printf("%-26.26s  : %ld.%lu.%lu\n", (char *)name,
						 mobID.prefix, mobID.major, mobID.minor);
				} 
			  else
				{
				  printSpace(1);
				  printf("%-16s [BentoID: %1ld]", 
						 mobString, 
						 omfsGetBentoID(fileHdl, tmpMob) & 0x0000ffff);
				  printSpace(1); 
				  printf(" : %ld.%lu.%lu\n", 
						 mobID.prefix, mobID.major, mobID.minor);
				}					   
			}
		}

	  CHECK(omfiIteratorDispose(fileHdl, mobIter));
	  mobIter = NULL;

	} /* XPROTECT */

  XEXCEPT
	{
	}
  XEND;

  return(OM_ERR_NONE);
}


static omfErr_t DumpMediaData(omfHdl_t fileHdl,
							  omfObject_t head,
							  omfInt32 numMedia)
{
  omfInt32 i;
  omfProperty_t mediaProp, idProp;
  omfFileRev_t fileRev;
  omfUID_t        tmpMediaID;
  omfObject_t obj;
  omfObjIndexElement_t objIndex;
  omfClassID_t    objClass;
  char            tmpClass[5];
  omfUniqueName_t name;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(fileHdl)
	{
	  CHECK(omfsFileGetRev(fileHdl, &fileRev));
	  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
		{
		  mediaProp = OMMediaData;
		  idProp = OMObjID;
		}
	  else 
		{
		  mediaProp = OMHEADMediaData;
		  idProp = OMOOBJObjClass;
		}

	  for (i = 1; i <= numMedia; i++) 
		{
		  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
			{
			  CHECK(omfsGetNthObjIndex(fileHdl, head, mediaProp, 
									   &objIndex, i));
			  tmpMediaID = objIndex.ID;
			  obj = objIndex.Mob;
			}
		  else
			{
			  CHECK(omfsGetNthObjRefArray(fileHdl, head, mediaProp, &obj, i));
			  CHECK(omfsReadUID(fileHdl, obj, OMMDATMobID, &tmpMediaID));
			}
		  omfError = omfsReadClassID(fileHdl, obj, idProp, objClass);
		  if (objClass && (omfError == OM_ERR_NONE)) 
			{
			  strncpy(tmpClass, objClass, 4);
			  tmpClass[4] = 0;
			  if (omfiVerboseMode)
				{
				  printSpace(1);
				  printf("%-18.18s [BentoID: %1ld]\n", (char *)tmpClass,
						 omfsGetBentoID(fileHdl, obj) & 0x0000ffff);
				  omfiGetPropertyName(fileHdl, OMMDATMobID, 
									  OMUNIQUENAME_SIZE, name);
				  printSpace(1);
				  printf("%-26.26s  ", (char *)name);
				}
			  else
				{
				  printSpace(1);
				  printf("%-18.18s [BentoID: %1ld]", (char *)tmpClass,
						 omfsGetBentoID(fileHdl, obj) & 0x0000ffff);
				  printSpace(1);
				}
			  printf(" : %ld.%lu.%lu\n", tmpMediaID.prefix, 
					  tmpMediaID.major, tmpMediaID.minor);
/*			  dumpProp(fileHdl, obj, OMMDATMobID, OMUID, "\0", 1); */
			}
		}
	} /* XPROTECT */

  XEXCEPT
	{
	}
  XEND;

  return(OM_ERR_NONE);
}

static omfErr_t DumpDefinitions(omfHdl_t fileHdl,
							 omfObject_t head)
{
  omfInt32 numDefs, i;
  omfObject_t obj;
  omfClassID_t    objClass;
  omfUniqueName_t name;
  char            tmpClass[5];
  char            printName[64];
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(fileHdl)
	{
	  numDefs = omfsLengthObjRefArray(fileHdl, head, 
									  OMHEADDefinitionObjects);
	  printf("\n* Definition Objects: %ld\n", numDefs);
	  
	  for (i = 1; i <= numDefs; i++) 
		{
		  CHECK(omfsGetNthObjRefArray(fileHdl, head, OMHEADDefinitionObjects, 
								&obj, i));
		  omfError = omfsReadClassID(fileHdl, obj, OMOOBJObjClass, objClass);
		  if (objClass && (omfError == OM_ERR_NONE)) 
			{
			  strncpy(tmpClass, objClass, 4);
			  tmpClass[4] = '\0';
			  printSpace(1);
			  printf("%4s [BentoID: %1ld]\n", (char *)tmpClass,
					 omfsGetBentoID(fileHdl, obj) & 0x0000ffff);
			}
		  if (omfiIsADatakind(fileHdl, obj, &omfError)) 
			{
			  omfiGetPropertyName(fileHdl, OMDDEFDatakindID, 
								  OMUNIQUENAME_SIZE, name);
			  printSpace(1);
			  sprintf(printName, "%-26.26s  ", (char *)name); 	      
			  dumpProp(fileHdl, obj, OMDDEFDatakindID, OMUniqueName,
					   printName, 1);
			} 
		  else if (omfiIsAnEffectDef(fileHdl, obj, &omfError)) 
			{
			  omfiGetPropertyName(fileHdl, OMEDEFEffectID, 
								  OMUNIQUENAME_SIZE, name);
			  printSpace(1);
			  sprintf(printName, "%-26.26s  ", (char *)name);
			  dumpProp(fileHdl, obj, OMEDEFEffectID, OMUniqueName, 
					   printName, 1);
				   
			  omfiGetPropertyName(fileHdl, OMEDEFEffectName, 
								  OMUNIQUENAME_SIZE, name);
			  printSpace(1);
			  sprintf(printName, "%-26.26s  ", (char *)name); 	      
			  dumpProp(fileHdl, obj, OMEDEFEffectName, OMString, 
					   printName, 1);
			}
		} /* for */
	} /* XPROTECT */
  XEXCEPT
	{
	}
  XEND;
  return(OM_ERR_NONE);
}

static omfErr_t DumpClassDictionary(omfHdl_t file,
									omfObject_t head)
{
  omfObject_t dictObj, dictParent;
  char		 id[5];		/* Changed when noted by brooks */
  omfInt32 i, numEntries;
  char classString[512];   
  omfProperty_t   indexProp, idProp, classProp, parentProp;
  omfFileRev_t	  fileRev;
  omfBool wasSemCheckOn = FALSE;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(file)
	{
	  omfsFileGetRev(file, &fileRev);
	  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
		{
		  idProp = OMObjID;
		  classProp = OMCLSDClassID;
		  indexProp = OMClassDictionary;
		}
	  else
		{
		  idProp = OMOOBJObjClass;
		  classProp = OMCLSDClass;
		  indexProp = OMHEADClassDictionary;
		}
	  parentProp = OMCLSDParentClass;

	  numEntries = omfsLengthObjRefArray(file, head, indexProp);
	  printf("\n* OMFI Class Dictionary: %ld\n", numEntries);
	  
	  for (i=0; i<numEntries; i++)
		{
		  omfsGetNthObjRefArray(file, head, indexProp, &dictObj, i+1);
		  
		  omfError = omfsReadClassID(file, dictObj, classProp, id);
		  strncpy(classString, id, 4);
		  classString[4] = 0;
		  
		  while (omfsIsPropPresent(file, dictObj, parentProp, OMObjRef))
			{
			  /* Turn off semantic checking to deal with older broken Avid
			   * Media Composer OMF files (missing ClassID property)
			   */
#if OMFI_ENABLE_SEMCHECK
			  wasSemCheckOn = omfsIsSemanticCheckOn(file);
			  if (wasSemCheckOn)
				omfsSemanticCheckOff(file);
#endif
			  CHECK(omfsReadObjRef(file, dictObj, parentProp, &dictParent));
			  dictObj = dictParent;
			  omfError = omfsReadClassID(file, dictObj, classProp, id);
#if OMFI_ENABLE_SEMCHECK
			  if (wasSemCheckOn) /* If it was on, turn it back on */
				omfsSemanticCheckOn(file);
#endif
			  id[4] = 0;
		  
			  strcat(classString, " -> ");
			  strcat(classString, id);
			} /* while */
		  printSpace(1);
		  printf("%s\n", classString);
		}
	  
	  printf("\n");
	} /* XPROTECT */

  XEXCEPT
	{
	}
  XEND;

  return(OM_ERR_NONE);
}

static omfUInt32 GetULong(omfUInt16 byteorder,
						  char *data)
{
  omfUInt32 result;
  
#if BYTE_SWAP_TEST && BYTE_ALIGN_DATA
  if (byteorder == OMNativeByteOrder)
    {
      result = *(omfUInt32 *)data;
      printf("*********TEST  getUlong casts into %d\n",result);
      return(result);
    }
#endif
  
  if (byteorder == MOTOROLA_ORDER)
    {       
      result = ((((omfUInt32)data[0] & 0x000000ff) << 24) |
                (((omfUInt32)data[1] & 0x000000ff) << 16) |
                (((omfUInt32)data[2] & 0x000000ff) << 8)  | 
                (((omfUInt32)data[3])& 0x000000ff));
    }
  else
    {       
      result = ((((omfUInt32)data[0])& 0x000000ff)        |
                (((omfUInt32)data[1] & 0x000000ff) << 8)  |
                (((omfUInt32)data[2] & 0x000000ff) << 16) | 
                (((omfUInt32)data[3] & 0x000000ff) << 24));
    }
  
  return(result);
}

static omfInt16 GetUShort(omfUInt16 byteorder,
						  char *data)
{
  
  omfInt16 result;
  
  if (byteorder == MOTOROLA_ORDER)
    result = ((((omfInt16)data[0] & 0x00ff)<< 8) | 
              (((omfInt16)data[1])& 0x00ff));
  else
    result = ((((omfInt16)data[0])& 0x00ff) | 
              (((omfInt16)data[1] & 0x00ff)<< 8));
  return(result);
}

static char *GetTiffTagName(omfUInt16 code)
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

static void DumpTIFFData(omfHdl_t file,
						 omfInt32 level, 
						 omfObject_t obj,
						 omfProperty_t mediaProp) 

{
  omfUInt32 first_IFD, offset, bytesRead;
  omfUInt16 tiff_id, byteorder;
  omfInt16 num_entries;
  omfInt32 i, n, data_offset;
  omfUInt16 code, type;
  omfInt32 nvals, val, val2, valoffset32;
  char buffer[128];
  omfType_t mediaType;
  omfFileRev_t fileRev;
  omfDDefObj_t pictureDef = NULL;
  omfPosition_t zeroOffset, fileOffset, tiffOffset, valoffset;
  omfErr_t omfError = OM_ERR_NONE;
  char *strbuffer;

  omfsFileGetRev(file, &fileRev);
  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
	{
	  mediaType = OMVarLenBytes;
	}
  else
	{
	  mediaType = OMDataValue;
	  omfiDatakindLookup(file, PICTUREKIND, &pictureDef, &omfError);
	}

  printSpace(level);
  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
	printf("Length of Data block: %1ld\n", 
		   omfsLengthVarLenBytes(file, obj, mediaProp));
  else
	printf("Length of Data block: %1ld\n", 
		   omfsLengthDataValue(file, obj, mediaProp));

  omfsCvtInt32toInt64(0, &zeroOffset);
  OMGetPropertyFileOffset(file, obj, mediaProp, zeroOffset, 
						  mediaType, &fileOffset);
  omfsTruncInt64toUInt32(fileOffset, &offset);
  printSpace(level);
  printf("Media Data file offset : %1lu\n", offset);

  if (omfiDumpData)
    {
	  /* Get byte order and TIFF ID */
	  omfsCvtInt32toInt64(0, &tiffOffset);
	  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
		omfsReadVarLenBytes(file, obj, mediaProp, tiffOffset/* offset */, 4,
							(void *)buffer, &bytesRead);
	  else
		omfsReadDataValue(file, obj, mediaProp, pictureDef, 
						  (omfDataValue_t)buffer, tiffOffset/* offset */, 4,
						  &bytesRead);
      byteorder = GetUShort(file->byteOrder, &buffer[0]); /* check the byteorder
															 of the media stream
															 in this file.... */
      tiff_id = GetUShort(byteorder, &buffer[2]);

      printSpace(level);
      printf("TIFF id: %2d, ByteOrder: %s\n", tiff_id,
	      (byteorder == MOTOROLA_ORDER ? "MM" : "II"));
      

	  /* Get IFD information */
	  omfsCvtInt32toInt64(4, &tiffOffset);
	  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
		omfsReadVarLenBytes(file, obj, mediaProp, tiffOffset/* offset */, 
							sizeof(omfUInt32), (void *)buffer, &bytesRead);
	  else
		omfsReadDataValue(file, obj, mediaProp, pictureDef, 
						  (omfDataValue_t)buffer, tiffOffset/* offset */, 
						  sizeof(omfUInt32),
						  &bytesRead);
      first_IFD = GetULong(byteorder, buffer);
      printSpace(level);
      printf("Location of IFD entries = %1ld\n", first_IFD);
      
	  omfsCvtInt32toInt64(first_IFD, &tiffOffset);
	  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
		omfsReadVarLenBytes(file, obj, mediaProp, tiffOffset/* offset */, 
							sizeof(omfUInt16), (void *)buffer, &bytesRead);
	  else
		omfsReadDataValue(file, obj, mediaProp, pictureDef, 
						  (omfDataValue_t)buffer, tiffOffset/* offset */, 
						  sizeof(omfUInt16),
						  &bytesRead);
      num_entries = GetUShort(byteorder, buffer);
      if ((num_entries < 10) || (25 < num_entries))
		{
		  printf("Error reading num_entries in TIFF IFD: %1d\n", num_entries);
		}
      
      n = num_entries;
      printSpace(level);
      printf("Number of IFD entries = %1ld\n", n);
      
      data_offset = first_IFD + 2;
      
      for (i = 0; i < n; i++)
		{
		  omfsCvtInt32toInt64(data_offset, &tiffOffset);
		  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
			omfsReadVarLenBytes(file, obj, mediaProp, tiffOffset /* offset */, 
								12, (void *)buffer, &bytesRead);
		  else
			omfsReadDataValue(file, obj, mediaProp, pictureDef, 
							  (omfDataValue_t)buffer, tiffOffset /* offset */, 
							  12, &bytesRead);
			
		  data_offset += 12;
		  code = GetUShort(byteorder, buffer);
		  type = GetUShort(byteorder, &buffer[2]);
		  if (type > 5) 
			type = 0; /* bad type value, don't read past string table */
		  nvals = GetULong(byteorder, &buffer[4]);
		  printSpace(level+1);
		  printf("%s\n", GetTiffTagName(code));
		  printSpace(level+2);
		  
		  switch (type)
			{
			case 1:  /* byte */
			  val = buffer[8];
			  printf("code = %1u type = %s N = %1ld V = %1ld\n",
					 code, typestring[type], nvals, val);
			  break;
			case 2:  /* ascii */
			  valoffset32 = GetULong(byteorder, &buffer[8]);
			  omfsCvtInt32toInt64(valoffset32, &valoffset);
			  if (nvals == 0)
				break;
			  strbuffer = omfsMalloc(nvals);
			  if (strbuffer == NULL)
				break;
			  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
				omfsReadVarLenBytes(file, obj, mediaProp, valoffset /* offset */, 
									nvals, (void *)strbuffer, &bytesRead);
			  else
				omfsReadDataValue(file, obj, mediaProp, pictureDef, 
								  (omfDataValue_t)strbuffer, valoffset,  
								  nvals, &bytesRead);
			  printf("code = %1u type = %s N = %1ld V = '%s'\n",
					 code, typestring[type], nvals, strbuffer);
			  omfsFree(strbuffer);
			  strbuffer=0;
			  break;	
			case 3: /* short */
			  val = GetUShort(byteorder, &buffer[8]);
			  printf("code = %1u type = %s N = %1ld V = %1ld\n",
					 code, typestring[type], nvals, val);
			  break;
			case 4: /* int  */
			  val = GetULong(byteorder, &buffer[8]);			  
			  printf("code = %1u type = %s N = %1ld V = %1ld\n",
					 code, typestring[type], nvals, val);
			  break;
			case 5: /* rational */
			  valoffset32 = GetULong(byteorder, &buffer[8]);
			  omfsCvtInt32toInt64(valoffset32, &valoffset);
			  if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
				omfsReadVarLenBytes(file, obj, mediaProp, valoffset /* offset */, 
									8, (void *)buffer, &bytesRead);
			  else
				omfsReadDataValue(file, obj, mediaProp, pictureDef, 
								  (omfDataValue_t)buffer, valoffset /* offset */, 
								  8, &bytesRead);
			  val = GetULong(byteorder, &buffer[0]);			  
			  val2 = GetULong(byteorder, &buffer[4]);
			  printf("code = %1u type = %s N = %1ld V = %1ld/%1ld\n",
					 code, typestring[type], nvals, val, val2);
			  break;
			} /* switch on type */
		} /* for */
	} /* if omfiDumpData */
}


/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/

