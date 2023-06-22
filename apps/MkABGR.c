#include "portvars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "masterhd.h"
#include "omErr.h"
#include "omDefs.h"
#include "omTypes.h"
#include "omUtils.h"
#include "omMedia.h"
#include "omCodId.h"
#include "omCodec.h"
#include "omMobGet.h"
#ifdef PORTKEY_OS_MACSYS
#include <Memory.h>
#endif

#define MakeRational(num, den, resPtr) \
{(resPtr)->numerator = num; (resPtr)->denominator = den; }

static void     FatalErrorCode(omfHdl_t filePtr, omfErr_t errcode, int line, char *file)
{
  char	errMsg[256];

  omfsGetExpandedErrorString(filePtr, errcode, sizeof(errMsg), errMsg);
  printf("Error '%s' returned at line %d in %s\n", errMsg, line, file);
  exit(1);
}

static void     FatalError(char *message)
{
  printf(message);
  exit(1);
}

static omfErr_t moduleErrorTmp = OM_ERR_NONE;/* note usage in macro */
#define check(a)  \
{ moduleErrorTmp = a; \
  if (moduleErrorTmp != OM_ERR_NONE) \
     FatalErrorCode(filePtr, moduleErrorTmp, __LINE__, __FILE__);\
}

#define assert(b, msg) \
  if (!(b)) {fprintf(stderr, "ASSERT: %s\n\n", msg); exit(1);}

/* The following #ifndef was added by a customer who said:
   The type definitions (ulong, ushort and ushort) starting in line
   41 are already included in the <sys/types.h> file <on the AIX R6000
   platform,> therefore you need to conditionally compile the code so
   it will not include these. */

#ifndef _AIX
typedef unsigned int  ulong;
typedef unsigned short ushort;
typedef unsigned short uchar;
#endif

typedef struct
{
    ulong      FType;         /* set to 0x2CA       */
    ulong      Reserved1;     /* set to 0xFFFFFFFF  */
    char		Name[60];		/* Null terminated name */
    char		Name2[60];	        /* Not used */
    char		Reserved2[8];
} InfinitFileRec_t;

typedef struct
{
  ushort	Vformat;		/* bits 0-14 set to 0;
					   (bit 15 == 1 ? PAL : NTSC)   */
  ushort	Reserved1;		/* set to 0xABCD  */
  ushort	Reserved2;		/* set to 0xABCD  */
  ushort	Vrate;			/* bits 0-1, 4-15 set to 0;
					   bit2 == 1 for 13.5 Mhz;
					   bit3 set to 1  */
  ushort	Reserved3;		/* set to 0xABCD  */
  ushort	Reserved4;		/* set to 0xABCD  */
  ushort	Reserved5;		/* set to 0xABCD  */
  ushort	Reserved6;		/* set to 0xABCD  */
  } InfinitMsgRec_t;

typedef struct
  {
  ulong     RGBlength;     /* size of RGB data (in bytes)  */
  ushort	status;
  ushort	Xorg;			/* starting X coordinate  */
  ushort	Yorg;			/* starting Y coordinate  */
  ushort	Hpixels;		/* # of pixels per scan line  */
  ushort	Vpixels;		/*# of scanlines  */
} InfinitDataRec_t;

#define MAX_NUM_OPS 16

static void swapLong(ulong *lptr)
{
ulong tmp;
unsigned char *ucptr;

tmp = *lptr;
ucptr = (unsigned char *)&tmp;
*lptr = ucptr[0]<<24 | ucptr[1]<<16 | ucptr[2]<<8 | ucptr[3];
}

static void swapShort(ushort *sptr)
{
ushort tmp;
unsigned char *ucptr;

tmp = *sptr;
ucptr = (unsigned char *)&tmp;
*sptr = ucptr[0]<<8 | ucptr[1];
}

omfErr_t getChyronData(FILE *inFP,
						InfinitFileRec_t *fileHeader,
						InfinitMsgRec_t *msgHeader,
						InfinitDataRec_t *dataHeader);


main(int argc, char *argv[])

{
	int  i, j, nframes, nBytes;

	InfinitFileRec_t 	fileHeader;
	InfinitMsgRec_t 	msgHeader;
	InfinitDataRec_t 	dataHeader;

	int  				width, height;
	unsigned char		*OutBuffer = NULL;
	unsigned char		*InBuffer = NULL;
	int  				bIndex;
	int  				nPixels, pixelsToRead;
	int 				InBufferSize, OutBufferSize;
	int                     rowOffsetField1, rowOffsetField2;

	omfSessionHdl_t 	sessionPtr;
	omfHdl_t        	filePtr = NULL;
	omfObject_t			masterMob, fileMob;
	omfRational_t		videoRate;
	omfMediaHdl_t		mediaPtr;
	omfRational_t		spectrumAspectRatio;

	char 				mobName[64];
	omfCodecID_t		codec = CODEC_RGBA_VIDEO;
	omfFrameLayout_t	spectrumFrameLayout = kSeparateFields;
	omfInt32			spectrumHeight = 243;
	omfInt32			spectrumWidth = 720;
	omfVideoMemOp_t		fileOps[MAX_NUM_OPS];
	omfVideoSignalType_t signalType;
	omfVideoMemOp_t		op;
	FILE 				*inFP;
	char                fname[128];
	omfProductIdentification_t ProductInfo;

	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Make ABGR Example";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = 1;
	ProductInfo.platform = NULL;

#ifdef PORTKEY_OS_MACSYS
	MaxApplZone();
#endif

	strcpy( (char*) mobName, "ABGR");
	strcpy( (char*) fname, mobName);
	strcat(fname, ".omf");

	inFP = fopen("chyron.inp", "rb");

	if (inFP == NULL)
	  {
	    FatalError("Missing file 'chyron.inp' in current directory");
	  }

	nframes = 3;
	if(argc == 2)
	  {
	    if(strcmp(argv[1], "-big") == 0)
	      nframes = 1000;  /* Should be > 1gig with current settings */
	    if(strcmp(argv[1], "-huge") == 0)
	      nframes = 3200;  /* Should be > 4 gig with current settings */
	  }

	check(getChyronData(inFP, &fileHeader, &msgHeader, &dataHeader));
	/*    sprintf(errstr, "Cannot open input file: %s", inFS.fName);*/


	width = dataHeader.Hpixels;
	height = dataHeader.Vpixels;
	nPixels = height * width;
	InBufferSize = nPixels*4;

	OutBufferSize = (spectrumHeight * 2) * spectrumWidth * 4;

	if (width < spectrumWidth || height < (spectrumHeight * 2))
	{
		check(OM_ERR_NULL_PARAM);
	}


	check(omfsBeginSession(&ProductInfo, &sessionPtr));
	check(omfmInit(sessionPtr));

	check(omfsCreateFile ((fileHandleType)&fname, sessionPtr, kOmfRev1x,
							&filePtr));

	MakeRational(2997, 100, &videoRate);

	MakeRational(4, 3, &spectrumAspectRatio);
	spectrumFrameLayout = kSeparateFields;

	check(omfmMasterMobNew(filePtr,	/* file (IN) */
							mobName, 	/* name (IN) */
							TRUE,		/* isPrimary (IN) */
							&masterMob)); 	/* result (OUT) */

	check(omfmFileMobNew(filePtr,	/* file (IN) */
						mobName, 	/* name (IN) */
						videoRate,	/* editrate (IN) */
						codec, 		/* codec (IN) */
		   				&fileMob)); /* result (OUT) */

	check(omfmVideoMediaCreate(filePtr, masterMob, 1, fileMob,
		     kToolkitCompressionDisable,
		     videoRate,
		     spectrumHeight,
		     spectrumWidth,
		     spectrumFrameLayout,
		     spectrumAspectRatio, &mediaPtr));

/* set up the auxiliary file format details */
	check(omfmVideoOpInit(filePtr, fileOps));

	op.opcode = kOmfImageAlignmentFactor; /* 4K */
	op.operand.expInt32 = 4*1024L;
	check(omfmVideoOpAppend(filePtr, kOmfForceOverwrite,
							op, fileOps, MAX_NUM_OPS));

	op.opcode = kOmfRGBCompLayout;
	op.operand.expCompArray[0] = 'A';
	op.operand.expCompArray[1] = 'B';
	op.operand.expCompArray[2] = 'G';
	op.operand.expCompArray[3] = 'R';
	op.operand.expCompArray[4] = 0;
	check(omfmVideoOpAppend(filePtr, kOmfForceOverwrite,
							op, fileOps, MAX_NUM_OPS));

    op.opcode = kOmfRGBCompSizes;
	op.operand.expCompSizeArray[0] = 8;
	op.operand.expCompSizeArray[1] = 8;
	op.operand.expCompSizeArray[2] = 8;
	op.operand.expCompSizeArray[3] = 8;
	op.operand.expCompSizeArray[4] = 0;
	check(omfmVideoOpAppend(filePtr, kOmfForceOverwrite,
							op, fileOps, MAX_NUM_OPS));

	check(omfmPutVideoInfoArray(mediaPtr, fileOps));

	check(omfmSourceGetVideoSignalType(mediaPtr, &signalType));

	if (signalType == kNTSCSignal) {
	  check(omfmSetVideoLineMap(mediaPtr, 20, kDominantField2))
	  rowOffsetField1 = 1;
	  rowOffsetField2 = 0;  /* field 2 dominant means first line of frame */
				/* is stored in the SECOND field */
        }
	else {
	  check(omfmSetVideoLineMap(mediaPtr, 23, kDominantField1));
	  rowOffsetField1 = 0;  /* field 1 dominant means first line of frame */
	  rowOffsetField2 = 1;  /* is stored in the FIRST field */
        }

	InBuffer = (unsigned char *) malloc((size_t)InBufferSize);

	if(InBuffer == NULL)
		fprintf(stderr, "memory full");

	OutBuffer = (unsigned char *) malloc((size_t)OutBufferSize);

	if(OutBuffer == NULL)
		fprintf(stderr, "memory full");

  	/* read 1/32 of the pixels in one shot.  Not important, ThinkC libs had
	   a hard time trying to load the whole buffer in one chunk.   */
	for (i = 0, bIndex = 0; i < 33; i++)
	{
		/*	fprintf(stdout, "reading part %1d of 33\n", i+1); */

		if (i < 32)
			pixelsToRead = nPixels / 32;
		else
			pixelsToRead = nPixels % 32;

		if (pixelsToRead == 0)
			continue;

		nBytes = pixelsToRead * 4;

		nBytes = fread(&InBuffer[bIndex], 1, nBytes, inFP);
		bIndex += nBytes;
	}

	fclose(inFP);

	bIndex = 0;  /* reset bIndex for next run */

	for (i = 0; i < spectrumHeight; i++)
	{
		for (j = 0; j < spectrumWidth; j++)
		{
			/*                          rows      +   column */
			*(int  *)(&OutBuffer[bIndex]) =
      			*(int  *)(&InBuffer[(((i*2)+rowOffsetField1)*width*4) +   (j*4)]);
      		bIndex += 4;

    	}
	}

	/* do second field */

	for (i = 0; i < spectrumHeight; i++)
	{
		for (j = 0; j < spectrumWidth; j++)
		{
			/*                                rows      +   column */
			*(int  *)(&OutBuffer[bIndex]) =
      			*(int  *)(&InBuffer[(((i*2)+rowOffsetField2)*width*4) +  (j*4)]);
      		bIndex += 4;
    	}
	}


	for (i = 0; i < nframes; i++)
	{
		fprintf(stdout, "Writing frame %1ld of %1ld\n", i+1, nframes);
		check(omfmWriteDataSamples(	mediaPtr,
									1, /* number of frames */
									(void *)OutBuffer, /* buffer */
									(int )OutBufferSize));
	}

#if OMF_CODEC_DIAGNOSTICS
	/* run diags before closing.  If media is closed,
		codec will ASSERT-fail */
	check(codecRunDiagnostics(mediaPtr, stdout));
#endif

	check(omfmMediaClose(mediaPtr));

	if (InBuffer)
	  {
	    omfsFree(InBuffer);
	    InBuffer = NULL;
	  }
	if (OutBuffer)
	  {
	    omfsFree(OutBuffer);
	    OutBuffer = NULL;
	  }

	check(omfsCloseFile(filePtr));
	check(omfsEndSession(sessionPtr));

	fprintf(stdout, "Done\n");
	return(OM_ERR_NONE);
}

omfErr_t getChyronData(FILE *inFP, InfinitFileRec_t *fileHeader,
		       InfinitMsgRec_t *msgHeader,
		       InfinitDataRec_t *dataHeader)
{
  int  nBytes;
  int  bytesRead;

  nBytes = 136;
  bytesRead = fread(fileHeader, nBytes, 1, inFP);

#if PORT_BYTESEX_LITTLE_ENDIAN
  swapLong(&(fileHeader->FType));
#endif

  assert((fileHeader->FType == 0x2ca), "Graphic is not a Chyron file");

  nBytes = 16;
  bytesRead = fread(msgHeader, 1, nBytes, inFP);

#if PORT_BYTESEX_LITTLE_ENDIAN
  swapShort(&(msgHeader->Vformat));
#endif

  assert((bytesRead == nBytes), "Error reading chyron msg header");

  nBytes = 14;
  bytesRead = fread(dataHeader, 1, nBytes, inFP);

#if PORT_BYTESEX_LITTLE_ENDIAN
  swapLong(&(dataHeader->RGBlength));
  swapShort(&(dataHeader->status));
  swapShort(&(dataHeader->Xorg));
  swapShort(&(dataHeader->Yorg));
  swapShort(&(dataHeader->Hpixels));
  swapShort(&(dataHeader->Vpixels));
#endif

  assert((bytesRead == nBytes), "Error reading chyron msg header");

  return(OM_ERR_NONE);

}
