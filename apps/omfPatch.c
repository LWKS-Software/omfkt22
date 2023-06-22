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
#if defined(THINK_C) || defined(__MWERKS__)
#include <Memory.h>
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
#include "MacSupport.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "masterhd.h"
#include "omPublic.h"
#include "omMobMgt.h"
#include "omMedia.h"
#include "omPvt.h"

/***********
 * DEFINES *
 ***********/

/********************/
/* GLOBAL VARIABLES */
/********************/
const omfInt32 omfiAppVersion = 1;
omfBool omfiVerboseMode = 0;
omfBool omfiYesMode = 0;

/**************
 * PROTOTYPES *
 **************/
static void printHeader(char *name,
			omfProductVersion_t toolkitVers,
			omfInt32 appVers);
static void usage(char *cmdName,
		  omfProductVersion_t toolkitVers,
		  omfInt32 appVers);
static omfBool isObj(omfHdl_t file,
		     omfObject_t obj,
		     void *data);
static omfErr_t fixObject(omfHdl_t    file,
			   omfObject_t obj,
			   omfInt32       level,
                           void        *data);
omfErr_t RepairClassDict(omfHdl_t file);

static int ProcessOneFile(fileHandleType fh, char *printName);

/**********************************************************************
 * Fixes include:
 *      - Adding "CLSD" ObjID to class dictionary objects.
 **********************************************************************/

omfSessionHdl_t	session;

/****************
 * MAIN PROGRAM *
 ****************/
int main(int argc, char *argv[])
{
	int	rtnCode = 0;
    fileHandleType fh;
    char *fname = NULL, *printfname = NULL;
    omfBool		foundOne;
	char	*cmdname = NULL;

#if defined(THINK_C) || defined(__MWERKS__)
#if OMFI_MACSF_STREAM
    Point where;
    SFReply fs;
    SFTypeList fTypes;
    char namebuf[255];
#endif
#else
    omfInt32  i;
#endif
	omfProductIdentification_t ProductInfo;

	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Old File Patcher";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = 1;
	ProductInfo.platform = NULL;

#if defined(THINK_C) || defined(__MWERKS__)
    argc = ccommand(&argv);
    strcpy( (char*) argv[0],"omfPatch");
    MacInit();
#if OMFI_MACSF_STREAM
    where.h = 20;
    where.v = 40;
#endif
#endif
	foundOne = FALSE;

	cmdname = argv[0];
	printHeader(cmdname, omfiToolkitVersion, omfiAppVersion);
	omfsBeginSession(&ProductInfo, &session);
	omfmInit(session);

#if OMFI_MACSF_STREAM
    /* Get the file name and open */
    fTypes[0] = 'OMFI';
    SFGetFile(where,  NULL, NULL, -1, fTypes, NULL, &fs);
    if (! fs.good)
      exit(0);
    fh = (fileHandleType)&fs;

    strncpy(namebuf, (char *)&(fs.fName[1]), (size_t)fs.fName[0]);
	namebuf[fs.fName[0]] = 0;
    printfname = namebuf;
	if((argc == 2) && (!strncmp("-verbose", argv[1],2)))
	  {
	    omfiVerboseMode = 1;
	  }
	rtnCode = ProcessOneFile(fh, printfname);
#else
    if (argc < 2)
      usage(argv[0], omfiToolkitVersion, omfiAppVersion);

    for (i=1 ; i < argc; i++)
      {
	if (!strncmp("-verbose", argv[i],2))
	  {
	    omfiVerboseMode = 1;
	  }
	else if (!strncmp("-yes", argv[i],2))
	  {
	    omfiYesMode = 1;
	  }
	else if (!strncmp("-", argv[i], 1))
	  {
	    usage(argv[0], omfiToolkitVersion, omfiAppVersion);
	  }
	else
	{
	  	fname = argv[i];
    	if(fname)
    	{
    		printfname = fname;
    		fh = (fileHandleType)fname;
			rtnCode = ProcessOneFile(fh, fname);
			if(rtnCode != 0)
			  {
			    omfsEndSession(session);
			    return(rtnCode);
			  }
			foundOne = TRUE;
		}
	}
     }

	if (!foundOne)
      usage(argv[0], omfiToolkitVersion, omfiAppVersion);

#endif
	omfsEndSession(session);
	printf("\nomfPatch completed successfully.\n");

	return(0);
}

static int ProcessOneFile(fileHandleType fh, char *printName)
{
    omfHdl_t file;
    omfFileRev_t fileRev;
    char errBuf[256];
    char ch;
     omfInt32 loop, numMobs;
   omfInt32 matches;
    omfMobObj_t mob;
    omfIterHdl_t mobIter = NULL;
    omfErr_t omfError = OM_ERR_NONE;

   XPROTECT(NULL)
      {
	if (!omfiYesMode)
	  {
	    /* print OMF version */
	    printf("\n***** OMF File: %s *****\n", printName);
	    printf("\n***** NOTE: This utility WILL modify your file \n");
	    printf("            and will moderately increase the file size...\n");
	    printf("            You may want to make a copy of the file before proceeding.\n");
	    fflush(stdin);
	    printf("\n      Do you want to continue? [y|n]: ");
	    ch = getchar();
	    if ((ch != 'y') && (ch != 'Y'))
	      {
		printf("\nExiting omfPatch...\n");
		exit(1);
	      }
	    printf("      Continuing...\n\n");
	  }
	CHECK(omfsModifyFile(fh, session, &file));
	omfsFileGetRev(file, &fileRev);
	if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
	  printf("OMFI File Revision: 1.0\n\n");
	else if (fileRev == kOmfRev2x)
	  {
	    printf("ERROR: Only supports OMF 1.0 files\n");
	    RAISE(OM_ERR_NOT_IMPLEMENTED);
	  }
	else
	  {
	    printf("ERROR: Only supports OMF 1.0 files\n");
	    RAISE(OM_ERR_NOT_IMPLEMENTED);
	  }

	/* NOTE - inquire about changing file */

	/* Fix Class Dictionary if necessary */
	CHECK(RepairClassDict(file));

#if OMFI_ENABLE_SEMCHECK
	omfsSemanticCheckOff(file);		/* JeffB: Added this so that we will handle MCX Composition Files */
#endif

        /* Iterate over all mobs in the file */
	CHECK(omfiIteratorAlloc(file, &mobIter));
	CHECK(omfiGetNumMobs(file, kAllMob, &numMobs));
	(file);
	for (loop = 0; loop < numMobs; loop++)
	  {
	    CHECK(omfiGetNextMob(mobIter, NULL, &mob));
	    CHECK(omfiMobMatchAndExecute(file, mob, 1,
					 isObj, NULL,
					 fixObject, NULL,
					 &matches));
	  } /* for */

	omfiIteratorDispose(file, mobIter);
	mobIter = NULL;
	omfError = omfsCloseFile(file);
      } /* XPROTECT */

    XEXCEPT
      {
	if (mobIter)
	  omfiIteratorDispose(file, mobIter);
	omfError = omfsCloseFile(file);
	printf("***ERROR: %d: %s\n", XCODE(),
	       omfsGetExpandedErrorString(file, XCODE(),
					  sizeof(errBuf), errBuf));
	return(-1);
      }
    XEND;
    return(0);
}

omfErr_t RepairClassDict(omfHdl_t file)
{
  omfFileRev_t  fileRev;
  omfProperty_t idProp, classProp, indexProp;
  omfType_t     classType;
  omfClassID_t  objClass, classID;
  omfObject_t   head = NULL, dictObj = NULL;
  omfInt32      numEntries, loop;
  omTable_t	*table;
  OMClassDef	superEntry;
  omfBool       found = FALSE;
  omfErr_t      omfError = OM_ERR_NONE;
  omfInt16		defaultByteOrder;

  XPROTECT(file)
    {
      omfsFileGetRev(file, &fileRev);
      CHECK(omfsGetHeadObject(file, &head));
      if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
	{
	  idProp = OMObjID;
	  classProp = OMCLSDClassID;
	  indexProp = OMClassDictionary;
	  classType = OMObjectTag;
	}

      /* Add the private MC classes to the class dictionary */
      table = file->session->classDefs1X;
      CHECK(omfsTableClassIDLookup(table, "MFMB",
				   sizeof(superEntry),
				   &superEntry, &found));
      if (!found)
	{
	  if (omfiVerboseMode)
	    printf("... Adding MFMB to Class Dictionary ...\n");
	  CHECK(omfsNewClass(file->session, kClsPrivate,
			     kOmfTstRev1x, "MFMB", OMClassNone));
	}

      CHECK(omfsTableClassIDLookup(table, "MFML",
				   sizeof(superEntry),
				   &superEntry, &found));
      if (!found)
	{
	  if (omfiVerboseMode)
	    printf("... Adding MFML to Class Dictionary ...\n");
	  CHECK(omfsNewClass(file->session, kClsPrivate,
			     kOmfTstRev1x, "MFML", OMClassNone));
	}

      CHECK(omfsTableClassIDLookup(table, "MDAU",
				   sizeof(superEntry),
				   &superEntry, &found));
      if (!found)
	{
	  if (omfiVerboseMode)
	    printf("... Adding MDAU to Class Dictionary ...\n");
	  CHECK(omfsNewClass(file->session, kClsPrivate,
			     kOmfTstRev1x, "MDAU", OMClassMDES));
	}

      CHECK(omfsTableClassIDLookup(table, "SD2D",
				   sizeof(superEntry),
				   &superEntry, &found));
      if (!found)
	{
	  if (omfiVerboseMode)
	    printf("... Adding SD2D to Class Dictionary ...\n");
	  CHECK(omfsNewClass(file->session, kClsPrivate,
			     kOmfTstRev1x, "SD2D", OMClassMDFL));
	}

      CHECK(omfsTableClassIDLookup(table, "LITM",
				   sizeof(superEntry),
				   &superEntry, &found));
      if (!found)
	{
	  if (omfiVerboseMode)
	    printf("... Adding LITM to Class Dictionary ...\n");
	  CHECK(omfsNewClass(file->session, kClsPrivate,
			     kOmfTstRev1x, "LITM", OMClassNone));
	}

      /* LF-W 1/16/97 - Control Clip is a subclass of Clip (Kansas) */
      /*                Also, made up a control point classID for   */
      /*                their control points called "CTPT".         */
      table = file->session->classDefs1X;
      CHECK(omfsTableClassIDLookup(table, "CTRL",
				   sizeof(superEntry),
				   &superEntry, &found));
      if (!found)
	{
	  if (omfiVerboseMode)
	    printf("... Adding CTRL to Class Dictionary ...\n");
	  CHECK(omfsNewClass(file->session, kClsPrivate,
			     kOmfTstRev1x, "CTRL", OMClassCLIP));
	}
      CHECK(omfsTableClassIDLookup(table, "CTPT",
				   sizeof(superEntry),
				   &superEntry, &found));
      if (!found)
	{
	  if (omfiVerboseMode)
	    printf("... Adding CTPT to Class Dictionary ...\n");
	  CHECK(omfsNewClass(file->session, kClsPrivate,
			     kOmfTstRev1x, "CTPT", OMClassNone));
	}


	/***** JeffB: Added McX/NT classes if was written using little-endian order */
	CHECK(omfsFileGetDefaultByteOrder(file, &defaultByteOrder));
	if(defaultByteOrder == INTEL_ORDER)
	{
	    CHECK(omfsTableClassIDLookup(table, "CNSC",
					   sizeof(superEntry),
					   &superEntry, &found));
	    if (!found)
		{
		  	if (omfiVerboseMode)
		    	printf("... Adding CNSC to Class Dictionary ...\n");
		  	CHECK(omfsNewClass(file->session, kClsPrivate,
				kOmfTstRev1x, "CNSC", OMClassNone));
		}

	    CHECK(omfsTableClassIDLookup(table, "CTRL",
					   sizeof(superEntry),
					   &superEntry, &found));
	    if (!found)
		{
		  	if (omfiVerboseMode)
		    	printf("... Adding CTRL to Class Dictionary ...\n");
		  	CHECK(omfsNewClass(file->session, kClsPrivate,
				kOmfTstRev1x, "CTRL", OMClassNone));
		}

	    CHECK(omfsTableClassIDLookup(table, "CTRI",
					   sizeof(superEntry),
					   &superEntry, &found));
	    if (!found)
		{
		  	if (omfiVerboseMode)
		    	printf("... Adding CTRI to Class Dictionary ...\n");
		  	CHECK(omfsNewClass(file->session, kClsPrivate,
				kOmfTstRev1x, "CTRI", OMClassNone));
		}
	}

      /* Fix the 1.x CLSD "ClassID missing" bug */
      numEntries = omfsLengthObjRefArray(file, head, indexProp);
      for (loop=1; loop<=numEntries; loop++)
	{
	  omfsGetNthObjRefArray(file, head, indexProp, &dictObj, loop);
	  omfError = omfsReadClassID(file, dictObj, idProp, objClass);
	  if (!omfsIsPropPresent(file, dictObj, idProp, classType))
	    {
	      if (omfiVerboseMode)
		printf("... Adding ObjID Property to CLSD ...\n");
	      strncpy(classID, "CLSD", (size_t)4);
	      classID[4] = 0;
	      omfError = omfsWriteClassID(file, dictObj, idProp, classID);
	    }
	}
    } /* XPROTECT */

  XEXCEPT
    {
      /* Will cleanup and close file at next level */
    }
  XEND;

  return(OM_ERR_NONE);
}

/*************
 * FUNCTIONS *
 *************/
static omfBool isObj(omfHdl_t file,
		     omfObject_t obj,
		     void *data)
{
  if (obj)
    return(TRUE);
  else
    return(FALSE);
}

static omfErr_t fixObject(omfHdl_t    file,
			   omfObject_t obj,
			   omfInt32       level,
                           void        *data)
{
   omfFileRev_t  fileRev;
   omfProperty_t idProp, prop;
   omfType_t     classType, longType, ulongType, boolType, shortType;
   omfClassID_t  objClass;
   omfObject_t   head = NULL, child = NULL;
   omfInt32      numEntries, loop;
   CMProperty    bentoProp;
   omfBool       found = FALSE;
   omfBool       isOMFI = FALSE, fixIsOMFI = FALSE;
   omfBool		 wasEnabled;
   omfUID_t      nullUID = {0,0,0};
   char          bentoID[32];
   omfErr_t      omfError = OM_ERR_NONE;

   XPROTECT(file)
     {
       /* NOTE: Key off of AME version? */

       omfsFileGetRev(file, &fileRev);
       CHECK(omfsGetHeadObject(file, &head));
       if ((fileRev == kOmfRev1x) || (fileRev == kOmfRevIMA))
	 {
	   idProp = OMObjID;
	   longType = OMInt32;
      ulongType = OMUInt32;
	   boolType = OMBoolean;
	   shortType = OMInt16;
	   classType = OMObjectTag;
	 }
       omfError = omfsReadClassID(file, obj, idProp, objClass);

       /* Get Bento ID for object */
       sprintf(bentoID, "[BentoID: %1ld]", omfsGetBentoID(file, obj) & 0x0000ffff);

       /* If TCCP:Flags is missing altogether, add a 0 value,
   * Else, if the type is wrong (ulong), write a (int ) value
	*/
       if (streq(objClass, "TCCP"))
	 {

	   prop = OMTCCPFlags;
	   if (!omfsIsPropPresent(file, obj, prop, longType))
	     {
	       /* TCCP:Flags is missing altogether */
          if (!omfsIsPropPresent(file, obj, prop, ulongType))
		 {
		   CHECK(omfsWriteInt32(file, obj, prop, 0));
		   /* Also write one that MC will understand */
		   CHECK(omfsWriteUInt32(file, obj, prop, 0));
		 }
          else /* TCCP:flags (as a ulong) is present */
		 {
		   omfUInt32 flags;

		   if (omfiVerboseMode)
		     printf("... Fixing type of TCCP:Flags %s ...\n",
			    bentoID);

		   wasEnabled = file->semanticCheckEnable;
#if OMFI_ENABLE_SEMCHECK
		   omfsSemanticCheckOff(file);
#endif
		   CHECK(omfsReadUInt32(file, obj, prop, &flags));
#if OMFI_ENABLE_SEMCHECK
		   if(wasEnabled)
		   	  omfsSemanticCheckOn(file);
#endif
		   CHECK(omfsWriteInt32(file, obj, prop, (omfInt32)flags));
		 }
	     }
	 }
       /* If SMLS Class, check to see if children LITM objects have
	* classID.
	*/
       else if (omfsIsTypeOf(file, obj, "SMLS", &omfError))
	 {
	   /* Bento Property ID for OMFI:SMLS:MC:ListItems */
	   bentoProp = CMRegisterProperty(file->container,
					  "OMFI:SMLS:MC:ListItems");
	   prop = CvtPropertyFromBento(file, bentoProp, &found);
	   numEntries = omfsLengthObjRefArray(file, obj, prop);
	   for (loop=1; loop<=numEntries; loop++)
	     {
	       omfsGetNthObjRefArray(file, obj, prop, &child, loop);
	       if (!omfsIsPropPresent(file, child, idProp, classType))
		 {
		   if (omfiVerboseMode)
		     printf("... Adding ObjID Property to LITM %s ...\n",
			    bentoID);
		   omfError = omfsWriteClassID(file, child, idProp, "LITM");
		 }
	     }
	 }

       /* LF-W 1/24/97 - patched Kansas private PVOL control points with
	*                classID called "CTPT".
	*/
       else if (omfsIsTypeOf(file, obj, "CTRL", &omfError))
	 {
	   /* Bento Property ID for OMFI:CTRL:ControlPoints */
	   bentoProp = CMRegisterProperty(file->container,
					  "OMFI:CTRL:ControlPoints");
	   prop = CvtPropertyFromBento(file, bentoProp, &found);
	   numEntries = omfsLengthObjRefArray(file, obj, prop);
	   for (loop=1; loop<=numEntries; loop++)
	     {
	       omfsGetNthObjRefArray(file, obj, prop, &child, loop);
	       if (!omfsIsPropPresent(file, child, idProp, classType))
		 {
		   if (omfiVerboseMode)
		     printf("... Adding ObjID Property to CTPT %s ...\n",
			    bentoID);
		   omfError = omfsWriteClassID(file, child, idProp, "CTPT");
		 }
	     }
	 }

       /* If SCLP with 1.0.1 Mob reference, change to 0.0.0 */
       else if (omfsIsTypeOf(file, obj, "SCLP", &omfError))
	 {
	   omfSourceRef_t sourceRef;
	   omfUID_t mcUID = {1,0,1};
	   omfUID_t kansasUID = {42,0,1};
#if 0
	   omfError = omfiSourceClipGetInfo(file, obj, NULL, NULL, &sourceRef);
	   if ((omfError == OM_ERR_NONE) &&
	       equalUIDs(sourceRef.sourceID, mcUID))
	     {
	      if (omfiVerboseMode)
		printf("... Fixing SCLP %s 1.0.1 Mob reference ...\n", bentoID);
	       initUID(sourceRef.sourceID, 0, 0, 0);
	       sourceRef.sourceTrackID = 0;
	       CHECK(omfiSourceClipSetRef(file, obj, sourceRef));
	     }
#endif
	   /* Fix SourceID */
	   prop = OMSCLPSourceID;
	   if (!omfsIsPropPresent(file, obj, prop, OMUID))
	     {
	       if (omfiVerboseMode)
		 printf("... Adding SourceID property to SCLP %s ...\n",
			bentoID);
	       CHECK(omfsWriteUID(file, obj, prop, nullUID));
	     }
	   else /* Property is present */
	     {
	       CHECK(omfsReadUID(file, obj, OMSCLPSourceID,
				 &sourceRef.sourceID));
	       if (equalUIDs(sourceRef.sourceID, mcUID))
		 {
		   if (omfiVerboseMode)
		     printf("... Fixing SCLP %s 1.0.1 Mob reference ...\n",
			    bentoID);
		   initUID(sourceRef.sourceID, 0, 0, 0);
		   sourceRef.sourceTrackID = 0;
		   omfsCvtInt32toPosition(0, sourceRef.startTime);
		   CHECK(omfiSourceClipSetRef(file, obj, sourceRef));
		 }
	       else if (equalUIDs(sourceRef.sourceID, kansasUID))
		 {
		   if (omfiVerboseMode)
		     printf("... Fixing SCLP %s 42.0.1 Mob reference ...\n",
			    bentoID);
		   initUID(sourceRef.sourceID, 0, 0, 0);
		   sourceRef.sourceTrackID = 0;
		   omfsCvtInt32toPosition(0, sourceRef.startTime);
		   CHECK(omfiSourceClipSetRef(file, obj, sourceRef));
		 }
	     }

	   /* Fix SourceTrack */
	   prop = OMSCLPSourceTrack;
	   if (!omfsIsPropPresent(file, obj, prop, OMInt16))
	     {
	       if (omfiVerboseMode)
		 printf("... Adding SourceTrack property to SCLP %s ...\n",
			bentoID);
	       CHECK(omfsWriteInt16(file, obj, prop, 0));
	     }

	   /* Fix SourcePosition */
	   prop = OMSCLPSourcePosition;
	   if (!omfsIsPropPresent(file, obj, prop, OMInt32))
	     {
	       if (omfiVerboseMode)
		 printf("... Adding SourcePosition property to SCLP %s ...\n",
			bentoID);
	       CHECK(omfsWriteInt32(file, obj, prop, 0));
	     }
	 }

       /* Add the TrackKind property for MOBJ's missing it */
       else if (omfsIsTypeOf(file, obj, "MOBJ", &omfError))
	 {
	   prop = OMCPNTTrackKind;
	   if (!omfsIsPropPresent(file, obj, prop, OMTrackType))
	     {
	       if (omfiVerboseMode)
		 printf("... Adding TrackType Property to MOBJ %s ...\n",
			bentoID);
	       CHECK(omfsWriteTrackType(file, obj, prop, 0));
	     }

	   /* Fix SourcePosition */
	   prop = OMMOBJStartPosition;
	   if (!omfsIsPropPresent(file, obj, prop, OMInt32))
	     {
	       if (omfiVerboseMode)
		 printf("... Adding StartPosition property to MOBJ %s ...\n",
			bentoID);
	       CHECK(omfsWriteInt32(file, obj, prop, 0));
	     }
	 }

       /* Add the missing IsUniform property to a TIFD object */
       else if (omfsIsTypeOf(file, obj, "MDFL", &omfError))
	 {
	   prop = OMMDFLIsOMFI;
	   /* Always TRUE for TIFF, AIFC and WAVE */
	   if (streq(objClass, "TIFD") || streq(objClass, "AIFD")
	       || streq(objClass, "WAVD"))
	     {
	       if (!omfsIsPropPresent(file, obj, prop, boolType))
		 {
		   isOMFI = TRUE;
		   fixIsOMFI = TRUE;
		 }
	       else /* isOMFI present */
		 {
		   CHECK(omfsReadBoolean(file, obj, prop, &isOMFI));
		   if (isOMFI == FALSE)
		     {
		       isOMFI = TRUE;
		       fixIsOMFI = TRUE;
		     }
		 }
	     }
	   else /* SDII */
	     if (!omfsIsPropPresent(file, obj, prop, boolType))
	       {
		 isOMFI = FALSE;
		 fixIsOMFI = TRUE;
	       }

	   if (fixIsOMFI)
	     {
	       if (omfiVerboseMode)
		 printf("... Fixing IsOMFI Property on MDFL %s ...\n", bentoID);
	       CHECK(omfsWriteBoolean(file, obj, prop, isOMFI));
	     }

	   if (streq(objClass, "TIFD"))
	     {
	       prop = OMTIFDIsUniform;
	       if (!omfsIsPropPresent(file, obj, prop, boolType))
		 {
		   if (omfiVerboseMode)
		     printf("... Adding IsUniform Property to TIFD %s ...\n",
			    bentoID);
		   CHECK(omfsWriteBoolean(file, obj, prop, FALSE));
		 }
	     }

	 } /* MDFL */

     } /* XPROTECT */

   XEXCEPT
     {
#if OMFI_ENABLE_SEMCHECK
		   if(wasEnabled)
		   	  omfsSemanticCheckOn(file);
#endif
     }
   XEND;

   return(OM_ERR_NONE);
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
  fprintf(stderr, "Usage: %s [-verbose] <filename>\n", cmdName);
  exit(1);
}
