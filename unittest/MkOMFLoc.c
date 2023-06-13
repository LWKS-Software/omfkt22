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

/*************************************************************************
*  MkOMFLoc - This unittest creates an OMF file that uses UNXL (and 
*             other locators) to point to another OMF file that contains 
*             RGBA and CDCI Media.  
**************************************************************************/

#include "masterhd.h"
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
#include "omMedia.h"
#include "omMobMgt.h"

#if PORT_SYS_MAC
#include <events.h>
#endif

#include "MkMed1x.h"
#include "UnitTest.h"

#define check(a)  { omfErr_t e; e = a; if (e != OM_ERR_NONE) \
					  FatalErrorCode(e, __LINE__, __FILE__); }

#define FatalErrorCode(ec,line,file){ \
  printf("Error '%s' returned at line %d in %s\n",\
		 omfsGetErrorString(ec), line, file); \
  RAISE(OM_ERR_TEST_FAILED); }

#define FatalError(msg){ printf(msg);RAISE(OM_ERR_TEST_FAILED); }


#define copyUIDs(a, b) \
 {a.prefix = b.prefix; a.major = b.major; a.minor = b.minor;}

#define STD_WIDTH	64L
#define STD_HEIGHT	48L
#define FRAME_SAMPLES	(STD_WIDTH * STD_HEIGHT)
#define FRAME_BYTES	(FRAME_SAMPLES * 3)

#define TIFF_UNCOMPRESSED_TEST	"TIFF Uncompressed"
#define RGBA_TEST				"RGBA"
#define CDCI_TEST				"CDCI"
#define JPEG_TEST				"JPEG"
#define JPEG_RAW_TEST			"JPEG_RAW"
#define JPEG_CMP_TEST			"JPEG_COMPRESSED"

#define NAMESIZE 255

static omfUInt8 red[] =	{ 3, 0x40, 0x00, 0x00 };
static omfUInt8 green[] =	{ 3, 0x00, 0x40, 0x00 };
static omfUInt8 blue[] =	{ 3, 0x00, 0x00, 0x40 };

static omfUInt8 *colorPatterns[] = { red, green, blue };
#define NUM_COLORS	(sizeof(colorPatterns) / sizeof(omfUInt8 *))
char           *stdWriteBuffer = NULL;
char           *stdColorBuffer = NULL;
char           *stdReadBuffer = NULL;
short           motoOrder = MOTOROLA_ORDER, intelOrder = INTEL_ORDER;

static int saveAsVersion1TOC = FALSE;

/**************/
/* PROTOTYPES */
/**************/
static omfErr_t WriteLocatorOMFfile(fileHandleType filename,
							 omfSessionHdl_t session);
static omfErr_t CheckLocatorOMFfile(fileHandleType filename,
					omfSessionHdl_t session);

static omfErr_t WriteRGBAMedia(omfHdl_t filePtr,
							   omfMobObj_t *fileMob,
							   omfMobObj_t *masterMob);

static omfErr_t WriteCDCIMedia(omfHdl_t filePtr,
						  omfMobObj_t *fileMob,
						  omfMobObj_t *masterMob);
static omfErr_t VerifyMedia(omfHdl_t filePtr, 
						  omfObject_t masterMob,
						  omfObject_t *fileMob);

static void     InitGlobals(void);
static void     FreeGlobals(void);


/****************************************************************/
static void InitGlobals(void)
{
  short           n;

  if (stdWriteBuffer == NULL)
	stdWriteBuffer = (char *) omfsMalloc(FRAME_BYTES);
  if (stdColorBuffer == NULL)
	stdColorBuffer = (char *) omfsMalloc(FRAME_BYTES);
  if (stdReadBuffer == NULL)
	stdReadBuffer = (char *) omfsMalloc(FRAME_BYTES);
  if ((stdWriteBuffer == NULL) || (stdColorBuffer == NULL) || 
	  (stdReadBuffer == NULL))
	exit(1);

  for (n = 0; n < FRAME_BYTES; n++)
	{
	  stdWriteBuffer[n] = n & 0xFF;
	  stdReadBuffer[n] = 0;
	}
}

static void     FreeGlobals(void)
{

	if (stdWriteBuffer != NULL)
	{
		omfsFree(stdWriteBuffer);
		stdWriteBuffer = NULL;
	}
	if (stdReadBuffer != NULL)
	{
		omfsFree(stdReadBuffer);
		stdReadBuffer = NULL;
	}
}

/****************************************************************/
static omfErr_t WriteRGBAMedia(omfHdl_t filePtr,
							   omfMobObj_t *fileMob,
							   omfMobObj_t *masterMob)
{
	omfRational_t	imageAspectRatio;	
	omfRational_t	videoRate;
	omfMediaHdl_t	mediaPtr;

	XPROTECT(filePtr)
	{
		printf("Writing RGBA Media\n");
			
		MakeRational(2997, 100, &videoRate);
		MakeRational(4, 3, &imageAspectRatio);
		
		CHECK(omfmMasterMobNew(	filePtr, 			/* file (IN) */
								RGBA_TEST, 			/* name (IN) */
								TRUE,				/* isPrimary (IN) */
								masterMob)); 		/* result (OUT) */
		
		CHECK(omfmFileMobNew(	filePtr,	 		/* file (IN) */
				                RGBA_TEST, 			/* name (IN) */
				                videoRate,   		/* editrate (IN) */
				                CODEC_RGBA_VIDEO, 	/* codec (IN) */
				                fileMob));   		/* result (OUT) */
	
		CHECK(omfmVideoMediaCreate(filePtr, *masterMob, 1, *fileMob, 
								   kToolkitCompressionEnable, 
								   videoRate, STD_HEIGHT, STD_WIDTH, 
								   kFullFrame, imageAspectRatio,
								   &mediaPtr));
							  
		CHECK(omfmWriteDataSamples(mediaPtr, 1, stdWriteBuffer, FRAME_BYTES));


		CHECK(omfmMediaClose(mediaPtr));
	}
	XEXCEPT 
	  {
	  }
	XEND;

	return(OM_ERR_NONE);
}

/****************************************************************/
static omfErr_t WriteCDCIMedia(omfHdl_t filePtr,
							   omfMobObj_t *fileMob,
							   omfMobObj_t *masterMob)
{
	omfRational_t	imageAspectRatio;	
	omfRational_t	videoRate;
	omfMediaHdl_t	mediaPtr;
	omfVideoMemOp_t yuvFileFmt[4];

	XPROTECT(filePtr)
	{
		printf("Writing CDCI Uncompressed Media\n");
			
		MakeRational(2997, 100, &videoRate);
		MakeRational(4, 3, &imageAspectRatio);
		
		CHECK(omfmMasterMobNew(	filePtr, 				/* file (IN) */
								CDCI_TEST, 				/* name (IN) */
								TRUE,					/* isPrimary (IN) */
								masterMob)); 			/* result (OUT) */
		
		CHECK(omfmFileMobNew(	filePtr,	 		/* file (IN) */
				                CDCI_TEST, 	/* name (IN) */
				                videoRate,   		/* editrate (IN) */
				                CODEC_CDCI_VIDEO, 	/* codec (IN) */
				                fileMob));   		/* result (OUT) */
	
		CHECK(omfmVideoMediaCreate(filePtr, *masterMob, 1, *fileMob, 
								   kToolkitCompressionDisable, 
								   videoRate, STD_HEIGHT, STD_WIDTH, 
								   kFullFrame, imageAspectRatio,
								   &mediaPtr));
							  
		yuvFileFmt[0].opcode = kOmfCDCICompWidth;
		yuvFileFmt[0].operand.expInt32 = 8;
		yuvFileFmt[1].opcode = kOmfCDCIHorizSubsampling;
		yuvFileFmt[1].operand.expInt32 = 1;
		yuvFileFmt[2].opcode = kOmfVFmtEnd;
		CHECK(omfmPutVideoInfoArray(mediaPtr, yuvFileFmt));

		CHECK(omfmWriteDataSamples(mediaPtr, 1, stdWriteBuffer, FRAME_BYTES));
		
	
		CHECK(omfmMediaClose(mediaPtr));
	}
	XEXCEPT
	  {
	  }
	XEND;
	
	return(OM_ERR_NONE);
}

/****************************************************************/
static omfErr_t WriteJPEGMedia(omfHdl_t filePtr,
							   omfMobObj_t *fileMob,
							   omfMobObj_t *masterMob)
{
	omfRational_t	imageAspectRatio;	
	omfRational_t	videoRate;
	omfMediaHdl_t	mediaPtr;
	omfVideoMemOp_t yuvFileFmt[4];

	XPROTECT(filePtr)
	{
		printf("Writing JPEG Uncompressed Media\n");
			
		MakeRational(2997, 100, &videoRate);
		MakeRational(4, 3, &imageAspectRatio);
		
		CHECK(omfmMasterMobNew(	filePtr, 				/* file (IN) */
								JPEG_TEST, 				/* name (IN) */
								TRUE,					/* isPrimary (IN) */
								masterMob)); 			/* result (OUT) */
		
		CHECK(omfmFileMobNew(	filePtr,	 		/* file (IN) */
				                JPEG_TEST, 	/* name (IN) */
				                videoRate,   		/* editrate (IN) */
				                CODEC_JPEG_VIDEO, 	/* codec (IN) */
				                fileMob));   		/* result (OUT) */
	
		CHECK(omfmVideoMediaCreate(filePtr, *masterMob, 1, *fileMob, 
								   kToolkitCompressionDisable, 
								   videoRate, STD_HEIGHT, STD_WIDTH, 
								   kFullFrame, imageAspectRatio,
								   &mediaPtr));
							  
		yuvFileFmt[0].opcode = kOmfCDCICompWidth;
		yuvFileFmt[0].operand.expInt32 = 8;
		yuvFileFmt[1].opcode = kOmfCDCIHorizSubsampling;
		yuvFileFmt[1].operand.expInt32 = 1;
		yuvFileFmt[2].opcode = kOmfVFmtEnd;
		CHECK(omfmPutVideoInfoArray(mediaPtr, yuvFileFmt));

		CHECK(omfmWriteDataSamples(mediaPtr, 1, stdWriteBuffer, FRAME_BYTES));
		
	
		CHECK(omfmMediaClose(mediaPtr));
	}
	XEXCEPT
	  {
	  }
	XEND;
	
	return(OM_ERR_NONE);
}

static omfErr_t VerifyMedia(omfHdl_t filePtr, 
						  omfObject_t masterMob,
						  omfObject_t *fileMob)
{
	omfSearchCrit_t	search;
	omfIterHdl_t	trackIter = NULL;
	omfMSlotObj_t	track;
	omfRational_t	editRate;
	omfSegObj_t		segment;
	omfInt16		numChannels;
	omfDDefObj_t	pictureKind, soundKind;
	omfErr_t		status;
	omfMediaHdl_t	media;
	omfUInt32		bytesRead;
	
	XPROTECT(filePtr)
	{
		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureKind, &status);
		CHECK(status);
		omfiDatakindLookup(filePtr, SOUNDKIND, &soundKind, &status);
		CHECK(status);

		search.searchTag = kByDatakind;
		strcpy(search.tags.datakind, PICTUREKIND);
		check(omfiIteratorAlloc(filePtr, &trackIter));
		CHECK(omfiMobGetNextTrack(trackIter, masterMob, &search, &track));
		CHECK(omfiIteratorDispose(filePtr, trackIter));
		trackIter = NULL;

		CHECK(omfiMobSlotGetInfo(filePtr, track, &editRate, &segment));
		if((editRate.numerator != 2997) || (editRate.denominator != 100))
			FatalError("Bad edit rate on TIFF uncompressed data\n");
		CHECK(omfiSourceClipResolveRef(filePtr, segment, fileMob));
			
		CHECK(omfmGetNumChannels(filePtr, masterMob, 1, NULL, soundKind,
								 &numChannels));
		if(numChannels != 0)
			FatalError("Found audio on video test result\n");
		CHECK(omfmGetNumChannels(filePtr, masterMob, 1, NULL,
								 pictureKind, &numChannels));
		if(numChannels == 0)
			FatalError("Missing video on video test result\n");
   	
		memset(stdReadBuffer, 0, FRAME_BYTES);
		CHECK(omfmMediaOpen(filePtr, masterMob, 1, NULL, kMediaOpenReadOnly,
							kToolkitCompressionDisable, &media));
		CHECK(omfmReadDataSamples(media, 1, FRAME_BYTES, stdReadBuffer, 
								  &bytesRead));
		if (memcmp(stdWriteBuffer, stdReadBuffer, FRAME_BYTES) != 0)
			FatalError("Video buffers did not compare\n");
		CHECK(omfmMediaClose(media));
	}
	XEXCEPT
	  {
		if (trackIter)
		  omfiIteratorDispose(filePtr, trackIter);
	  }
	XEND;
	
	return(OM_ERR_NONE);
}

/****************************************************************/
static omfErr_t WriteLocatorOMFfile(fileHandleType filename,
									omfSessionHdl_t session)
{
	omfHdl_t		fp, RGBAfp, CDCIfp, JPEGfp;
	omfObject_t		fileMob, masterMob;
	omfRational_t	editRate;
	char			RGBAnamebuf[NAMESIZE], CDCInamebuf[NAMESIZE], JPEGnamebuf[NAMESIZE];
	short		    motoOrder = MOTOROLA_ORDER, intelOrder = INTEL_ORDER;
	omfUID_t        RGBAfileMobID, RGBAmasterMobID;
	omfUID_t        CDCIfileMobID, CDCImasterMobID;
	omfUID_t        JPEGfileMobID, JPEGmasterMobID;
	omfMobObj_t     RGBAfileMob, RGBAmasterMob;
	omfMobObj_t     CDCIfileMob, CDCImasterMob;
	omfMobObj_t     JPEGfileMob, JPEGmasterMob;
	omfDDefObj_t    pictureDef = NULL;
	omfIterHdl_t    mobIter = NULL;
	omfSearchCrit_t searchCrit;
	char shortCutBuf[128];
	omfInt16 n;
	omfErr_t        status = OM_ERR_NONE;

	printf("Write out the OMF file with locators.\n");

	XPROTECT(NULL)
	{
	  /* Open the Base OMF file */
	  check(omfsCreateFile(filename, session, kOmfRev2x, &fp));

	  editRate.numerator = 2997;
	  editRate.denominator = 100;

	  omfiDatakindLookup(fp, PICTUREKIND, &pictureDef, &status);
	  for(n = 0; n < sizeof(shortCutBuf); n++)
		shortCutBuf[n] = (char)(n & 0xFF);

	  /* Create the RGBA.omf Media file */
	  memset(RGBAnamebuf, 0, NAMESIZE);
	  sprintf(RGBAnamebuf, "RGBA.omf");
	  sprintf(CDCInamebuf, "CDCI.omf");
	  CHECK(omfsCreateFile((fileHandleType)RGBAnamebuf, session, 
						   kOmfRev2x, &RGBAfp));
	  CHECK(WriteRGBAMedia(RGBAfp, &RGBAfileMob, &RGBAmasterMob));
	  CHECK(omfiMobGetMobID(RGBAfp, RGBAfileMob, &RGBAfileMobID));
	  CHECK(omfiMobGetMobID(RGBAfp, RGBAmasterMob, &RGBAmasterMobID));
	  
	  /* Create the CDCI.omf Media file */
	  sprintf(CDCInamebuf, "CDCI.omf");
	  CHECK(omfsCreateFile((fileHandleType)CDCInamebuf, session, 
						   kOmfRev2x, &CDCIfp));
	  CHECK(WriteCDCIMedia(CDCIfp, &CDCIfileMob, &CDCImasterMob));
	  CHECK(omfiMobGetMobID(CDCIfp, CDCIfileMob, &CDCIfileMobID));
	  CHECK(omfiMobGetMobID(CDCIfp, CDCImasterMob, &CDCImasterMobID));

	  /* Create the JPEG.omf Media file */
	  sprintf(JPEGnamebuf, "JPEG.omf");
	  CHECK(omfsCreateFile((fileHandleType)JPEGnamebuf, session, 
						   kOmfRev2x, &JPEGfp));
	  CHECK(WriteJPEGMedia(JPEGfp, &JPEGfileMob, &JPEGmasterMob));
	  CHECK(omfiMobGetMobID(JPEGfp, JPEGfileMob, &JPEGfileMobID));
	  CHECK(omfiMobGetMobID(JPEGfp, JPEGmasterMob, &JPEGmasterMobID));

	  /* Create the RGBA master and file mob copies, attach them, 
	   * and add locators that point to the RGBA media OMFI file.
	   */
	  CHECK(omfiMobCloneExternal(RGBAfp, RGBAmasterMob, 
								 kFollowDepend, kNoIncludeMedia,
								 fp, &masterMob));
	  
	  /* Find newly cloned RGBA file mob to add locators */
	  CHECK(omfiIteratorAlloc(fp, &mobIter));
	  searchCrit.searchTag = kByMobID;
	  copyUIDs(searchCrit.tags.mobID, RGBAfileMobID);
	  CHECK(omfiGetNextMob(mobIter, &searchCrit, &fileMob));
	  CHECK(omfiIteratorDispose(fp, mobIter));
	  mobIter = NULL;

	  CHECK(omfmMobAddUnixLocator(fp, fileMob, kOmfiMedia, RGBAnamebuf));
	  CHECK(omfmMobAddDOSLocator(fp, fileMob, kOmfiMedia, RGBAnamebuf));

	  CHECK(omfmMobAddWindowsLocator(fp, fileMob, kOmfiMedia, 
									 CDCInamebuf, shortCutBuf, sizeof(shortCutBuf)));
	  CHECK(omfmMobAddNetworkLocator(fp, fileMob, kOmfiMedia, RGBAnamebuf));
		
	  /* Create the CDCI master and file mob copies, attach them, 
	   * and add locators that point to the CDCI media OMFI file.
	   */
	  CHECK(omfiMobCloneExternal(CDCIfp, CDCImasterMob, 
								 kFollowDepend, kNoIncludeMedia,
								 fp, &masterMob));

	  /* Find newly cloned CDCI file mob to add locators */
	  CHECK(omfiIteratorAlloc(fp, &mobIter));
	  searchCrit.searchTag = kByMobID;
	  copyUIDs(searchCrit.tags.mobID, CDCIfileMobID);
	  CHECK(omfiGetNextMob(mobIter, &searchCrit, &fileMob));
	  CHECK(omfiIteratorDispose(fp, mobIter));
	  mobIter = NULL;
	  
	  CHECK(omfmMobAddUnixLocator(fp, fileMob, kOmfiMedia, CDCInamebuf));
	  CHECK(omfmMobAddTextLocator(fp, fileMob, CDCInamebuf));
	  CHECK(omfmMobAddWindowsLocator(fp, fileMob, kOmfiMedia, 
									 CDCInamebuf, shortCutBuf, sizeof(shortCutBuf)));
	  CHECK(omfmMobAddNetworkLocator(fp, fileMob, kOmfiMedia, CDCInamebuf));

	  /* Create the JPEG master and file mob copies, attach them, 
	   * and add locators that point to the JPEG media OMFI file.
	   */
	  CHECK(omfiMobCloneExternal(JPEGfp, JPEGmasterMob, 
								 kFollowDepend, kNoIncludeMedia,
								 fp, &masterMob));

	  /* Find newly cloned JPEG file mob to add locators */
	  CHECK(omfiIteratorAlloc(fp, &mobIter));
	  searchCrit.searchTag = kByMobID;
	  copyUIDs(searchCrit.tags.mobID, JPEGfileMobID);
	  CHECK(omfiGetNextMob(mobIter, &searchCrit, &fileMob));
	  CHECK(omfiIteratorDispose(fp, mobIter));
	  mobIter = NULL;
	  
	  CHECK(omfmMobAddUnixLocator(fp, fileMob, kOmfiMedia, JPEGnamebuf));
	  CHECK(omfmMobAddTextLocator(fp, fileMob, JPEGnamebuf));
	  CHECK(omfmMobAddWindowsLocator(fp, fileMob, kOmfiMedia, 
									 CDCInamebuf, shortCutBuf, sizeof(shortCutBuf)));
	  CHECK(omfmMobAddNetworkLocator(fp, fileMob, kOmfiMedia, JPEGnamebuf));

#if 0
	  CHECK(omfmMobAddMacLocator(fp, fileMob, kOmfiMedia, "CDCI.omf"));
#endif
	  
	  /* Close all OMF Files */
	  CHECK(omfsCloseFile(RGBAfp));
	  CHECK(omfsCloseFile(CDCIfp));	
	  CHECK(omfsCloseFile(JPEGfp));	
	  CHECK(omfsCloseFile(fp));
	}

	XEXCEPT
	{
	  if (mobIter)
		omfiIteratorDispose(fp, mobIter);
	  omfsCloseFile(fp);
	  fprintf(stderr, "<Fatal %1d>%s\n", XCODE(), omfsGetErrorString(XCODE()));
	}
	XEND;

	return(OM_ERR_NONE);
}

static omfErr_t CheckLocatorOMFfile(fileHandleType filename,
									omfSessionHdl_t session)
{
	omfHdl_t	fp;
	omfObject_t	locator = NULL;
	omfMobObj_t masterMob = NULL, fileMob = NULL;
	omfClassID_t	loctype;
	char 		locName[NAMESIZE], mobName[NAMESIZE];
	omfIterHdl_t	mobIter = NULL, locIter = NULL;
	omfInt16 vRef;
	omfInt32 dirID;
	char		shortcutBuf[128];
	omfSearchCrit_t searchCrit;

	printf("Verify the OMF file with Locators\n");
	
	XPROTECT(NULL) 
	  {
		check(omfsOpenFile(filename, session, &fp));

		check(omfiIteratorAlloc(fp, &mobIter));
		check(omfiIteratorAlloc(fp, &locIter));

		searchCrit.searchTag = kByMobKind;
		searchCrit.tags.mobKind = kMasterMob;
		while(omfiGetNextMob(mobIter, &searchCrit, &masterMob) == OM_ERR_NONE)
		  {
			check(omfiMobGetInfo(fp, masterMob, NULL, NAMESIZE, mobName,
								 NULL, NULL));
			printf("Mob Name: %s\n", mobName);
			if (strcmp(mobName, RGBA_TEST) == 0) 
			  {
				check(VerifyMedia(fp, masterMob, &fileMob));
			  }
			else if (strcmp(mobName, CDCI_TEST) == 0)
			  {
				check(VerifyMedia(fp, masterMob, &fileMob));
			  }
			else if (strcmp(mobName, JPEG_TEST) == 0)
			  {
				check(VerifyMedia(fp, masterMob, &fileMob));
			  }
			else
			  {
				printf("Found media from an unknown test\n");
				check(omfsCloseFile(fp));
				return(OM_ERR_BADMEDIATYPE);
			  }	
		
			printf("   LOCATORS on Mob %s...\n", mobName);
			while(omfmMobGetNextLocator(locIter, fileMob, &locator) == OM_ERR_NONE)
			  {
				check(omfmLocatorGetInfo(fp, locator, loctype, NAMESIZE, locName));
				printf("       Locator %.4s Name: %s\n", loctype, locName);
				if (strncmp(loctype, "MACL", (size_t)4) == 0)
				  {
					check(omfmMacLocatorGetInfo(fp, locator, &vRef, &dirID));
					printf("                    vRef: %d, DirID %d\n", 
						   vRef, dirID);
				  }
				else if (strncmp(loctype, "WINL", (size_t)4) == 0)
				  {
					check(omfmWindowsLocatorGetInfo(fp, locator, sizeof(shortcutBuf),
													shortcutBuf));
				  }
			  }
			check(omfiIteratorClear(fp, locIter));
		  }	

		check(omfiIteratorDispose(fp, mobIter));
		mobIter = NULL;
		check(omfiIteratorDispose(fp, locIter));
		locIter = NULL;
		check(omfsCloseFile(fp));
	  }

	XEXCEPT
	  {
		if (mobIter)
		  omfiIteratorDispose(fp, mobIter);
		if (locIter)
		  omfiIteratorDispose(fp, locIter);
	  }
    XEND;
	return(OM_ERR_NONE);
}
	
/****************
 * MAIN PROGRAM *
 ****************/
#ifdef MAKE_TEST_HARNESS
int MkOMFLoc(char *input)
{
	int argc;
	char *argv[2];
#else
int main(int argc, char *argv[])
{
#endif
	omfSessionHdl_t session;
	omfProductIdentification_t ProductInfo;
	
	/*	
	 * Standard Toolbox stuff
	 */
#if PORT_SYS_MAC
	int  	startMem;

	MaxApplZone();
	startMem = FreeMem();
#endif

	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Make OMF Locators UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;

#ifdef MAKE_TEST_HARNESS
	argc = 2;
	argv[1] = input;
#else
#if PORT_MAC_HAS_CCOMMAND
 	argc = ccommand(&argv);
#endif
#endif

	InitGlobals();

	if (argc < 2) {
	  printf ("*** ERROR - Usage: MkOMFLoc <filename.omf>\n");
	  return(1);
 	}

	XPROTECT(NULL)
	  {
		/* Start an OMF Session */
		check(omfsBeginSession(&ProductInfo, &session));
		check(omfmInit(session));

		/* Write then verify the file */
		check(WriteLocatorOMFfile((fileHandleType)argv[1], session));
		check(CheckLocatorOMFfile((fileHandleType)argv[1], session));

		/* Close the OMF Session */
		check(omfsEndSession(session));
	
	  }
	XEXCEPT
	  {
	  }
    XEND;
	FreeGlobals();
	return(0);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/


