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
#include "masterhd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(THINK_C) || defined(__MWERKS__)
#include "MacSupport.h"
#include <console.h>
#endif

#include "omErr.h"
#include "omDefs.h"
#include "omTypes.h"
#include "omUtils.h"
#include "omMedia.h"
#include "omCodId.h"
#include "omCodec.h"
#include "omcAVR.h"
#include "omMobGet.h"
#include "omMobBld.h"
#include "omcAvidJFIF.h"
#include "omcAvidJFIF.h"

#define ARG_NTSC 1
#define ARG_PAL  2
#define ARG_IN	 4
#define ARG_AVR  8
#define ARG_FILM 16


#define MakeRational(num, den, resPtr) \
        {(resPtr)->numerator = num; (resPtr)->denominator = den;}

static void     FatalErrorCode(omfErr_t errcode, int line, char *file)
{
  printf("Error '%s' returned at line %d in %s\n",
	 omfsGetErrorString(errcode), line, file);
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
     FatalErrorCode(moduleErrorTmp, __LINE__, __FILE__);\
}

#define assert(b, msg) \
  if (!(b)) {fprintf(stderr, "ASSERT: %s\n\n", msg); exit(1);}

 /* The following #ifndef was added by a customer who said:
    The type definitions (ulong, ushort and ushort) starting in line
    41 are already included in the <sys/types.h> file <on the AIX R6000
    platform,> therefore you need to conditionally compile the code so
    it will not include these.
*/

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
void usage(void);

#define NUM_COLUMNS	5

void usage()
{
	omfSessionHdl_t	sessionPtr;
  omfInt32		v, numVariety;
  char			varietyName[64];

	fprintf(stderr,
    	"Usage: MkAVR  [-pal | -ntsc] [-film] -input <input file name> [AVRxx]\n");
	fprintf(stderr, "  AVRxx := ( ");

/*	AVR1 | AVR2 | AVR3 | AVR4 | AVR5\n\
         AVR1e | AVR2e | AVR3e | AVR4e | AVR5e | AVR6e\n\
         AVR25 | AVR26 | AVR27\n\
         AVR2s | AVR3s | AVR4s | AVR6s | AVR8s | AVR9s\n\
         AVR2m | AVR3m | AVR4m | AVR5m | AVR6m\n\
         AVR70 | AVR71 | AVR75 AVR77 ) \n");
*/
   check(omfsBeginSession(NULL, &sessionPtr));

  check(omfmInit(sessionPtr));
  check(omfmRegisterCodec(sessionPtr, omfCodecAJPG,kOMFRegisterLinked));
  check(omfmRegisterCodec(sessionPtr, omfCodecAvidJFIF, kOMFRegisterLinked));

	check(omfmGetNumCodecVarieties(sessionPtr,
								CODEC_AJPG_VIDEO,
								kOmfRev1x,
								PICTUREKIND,
								&numVariety));

  for(v = 1; v <= numVariety; v++)
	{
		check(omfmGetIndexedCodecVariety(sessionPtr,
						CODEC_AJPG_VIDEO,
						kOmfRev1x,
						PICTUREKIND,
			 			v,
			 			sizeof(varietyName),
			 			varietyName));
   		fprintf(stderr, "%s", varietyName);
		if(v == numVariety)
			fprintf(stderr, " )\n");
		if((v % NUM_COLUMNS) == 0)
			fprintf(stderr, "\n             ");
		else
			fprintf(stderr, " | ");
	}
	fprintf(stderr, "\n");
  	check(omfsEndSession(sessionPtr));

	exit(1);
}

main(int argc, char *argv[])
{
  int  i, j, nframes, nBytes;

  InfinitFileRec_t 	fileHeader;
  InfinitMsgRec_t 	msgHeader;
  InfinitDataRec_t 	dataHeader;

  int  	width, height;
  ulong length;
  const ulong  step = 2;  /* interlaced works on alternate lines */
  unsigned char	*PixelPtr = NULL;
  unsigned char	*PixBuffer = NULL;
  unsigned char	*ScaledPixels = NULL;
  int  		bIndex, pIndex;
  int  		nPixels, pixelsToRead;

  omfSessionHdl_t 	sessionPtr;
  omfHdl_t      filePtr;
  omfObject_t	masterMob, fileMob;
  omfRational_t	videoRate;
  omfMediaHdl_t	mediaPtr;
  omfRational_t	imageAspectRatio;

  FILE 				*inFP;
  char          outFileName[128];
  char          inFileName[128];

  char			*progressiveptr;
  int			tmplen;


  avidAVRdata_t avrData;
  omfBool	twoField = FALSE;
  omfBool	TIFF_codec = FALSE;
  char 		mobName[64];
  char		codec[64], *ptr;
  omfInt32 	aWidth, omfWidth;
  omfInt32 	aHeight, omfHeight, omfSize;
  omfVideoSignalType_t videoSignal = kNTSCSignal;
  omfUInt32	argMask;
  omfInt32	argIndex;
	omfDDefObj_t pictureKind;
	omfErr_t	status;
  omfProductIdentification_t ProductInfo;

  ProductInfo.companyName = "OMF Developers Desk";
  ProductInfo.productName = "Make AVR Example";
  ProductInfo.productVersion = omfiToolkitVersion;
  ProductInfo.productVersionString = NULL;
  ProductInfo.productID = -1;
  ProductInfo.platform = NULL;

#if defined(THINK_C) || defined(__MWERKS__)
  MacInit();
  argc = ccommand(&argv);
#endif

  argMask = 0;
  argIndex = 1;

  while (argIndex < argc)
    {
      if (argv[argIndex][0] == '-')
    	{
	  if (strcmp(argv[argIndex],"-pal") == 0)
	    {
	      argMask |= ARG_PAL;
	      videoSignal = kPALSignal;
	    }
	  else if (strcmp(argv[argIndex],"-ntsc") == 0)
	    {
	      argMask |= ARG_NTSC;
	      videoSignal = kNTSCSignal;
	    }
	  else if (strcmp(argv[argIndex],"-film") == 0)
	    {
	      argMask |= ARG_FILM;
	    }
	  else if (strcmp(argv[argIndex],"-input") == 0)
	    {
	      argMask |= ARG_IN;
	      argIndex++;
	      strcpy(inFileName, argv[argIndex]);
	    }
	  else
	    usage();
	}
      else if (argv[argIndex][0] == 'A')
	{
	  avrData.avr = argv[argIndex];
	  avrData.product = avidComposerMac;
	  argMask |= ARG_AVR;
	}
      else if (argv[argIndex][0] == 'J')
	{
	  avrData.avr = argv[argIndex];
	  avrData.product = avidComposerMac;
	  argMask |= ARG_AVR;
	}
      else if (argv[argIndex][0] == 'U')
	{
	  avrData.avr = argv[argIndex];
	  avrData.product = avidComposerMac;
	  argMask |= ARG_AVR;
	}
      else
	usage();

      argIndex++;
    }

  /* if both NTSC and PAL, error */
  if ((argMask & ARG_NTSC) && (argMask & ARG_PAL))
    usage();
  /* if none, default NTSC */
  else if (! ((argMask & ARG_NTSC) || (argMask & ARG_PAL)))
    {
      argMask |= ARG_NTSC;
      videoSignal = kNTSCSignal;
    }


  if (! (argMask & ARG_IN))  /* input file required */
    usage();

#if 1
  sprintf(codec, "%s:%s:%s", CODEC_AJPG_VIDEO, avrData.avr,
	videoSignal == kNTSCSignal ? "NTSC" : "PAL");
#else
  sprintf(codec, "%s", CODEC_AJPG_VIDEO);
#endif

  if (! (argMask & ARG_AVR))
    {
      if (TIFF_codec)
		strcpy(codec, CODEC_TIFF_VIDEO);
      else
		strcpy(codec, CODEC_RGBA_VIDEO);
    }


  if (argMask & ARG_PAL)
    strcpy(mobName, "p");
  else
    strcpy(mobName,"n");

  if (argMask & ARG_FILM)
    strcat(mobName, "f");

  if (! (argMask & ARG_AVR))
    strcat(mobName, "uncompressed");
  else
    strcat(mobName, avrData.avr);

  strcpy(outFileName, mobName);
  strcat(outFileName, ".omf");

  for(ptr = outFileName; *ptr != '\0'; ptr++)
  	if(*ptr == ':')
  		*ptr = '-';

  check(omfsBeginSession(&ProductInfo, &sessionPtr));

  check(omfmInit(sessionPtr));

  inFP = fopen(inFileName, "rb");

  if (inFP == NULL)
    {
      check(OM_ERR_BADOPEN);
    }

  nframes = 3;  /* arbitrary number */

  /* read in the pixels, converting from 32bit ABGR to 24bit RGB.  When
     the toolkit can automatically convert, set the memory format to
     32bit ABGR, and the rest is done behind the scenes.
     */

  check(getChyronData(inFP, &fileHeader, &msgHeader, &dataHeader));

  width = dataHeader.Hpixels;
  height = dataHeader.Vpixels;
  nPixels = width * height;
  length = nPixels * 3; /* 24bit RGB */

  check(omfmRegisterCodec(sessionPtr, omfCodecAJPG,kOMFRegisterLinked));
  check(omfmRegisterCodec(sessionPtr, omfCodecAvidJFIF, kOMFRegisterLinked));

  check(omfsCreateFile ((fileHandleType)outFileName, sessionPtr, kOmfRev1x,
							&filePtr));

  /* check film first!!! Film can co-exist with NTSC or PAL */
  if (argMask & ARG_FILM)
  {
    MakeRational(24, 1, &videoRate);
  }
  else if (argMask & ARG_PAL)
  {
    MakeRational(25, 1, &videoRate);
  }
  else if (argMask & ARG_NTSC)
  {
    MakeRational(2997, 100, &videoRate);
  }

  MakeRational(4, 3, &imageAspectRatio);

  check(omfmMasterMobNew(filePtr,	/* file (IN) */
			 mobName, 	/* name (IN) */
			 TRUE,		/* isPrimary (IN) */
			 &masterMob)); 	/* result (OUT) */

  check(omfmFileMobNew(filePtr,	/* file (IN) */
		       mobName, 	/* name (IN) */
		       videoRate,	/* editrate (IN) */
		       codec, 		/* codec (IN) */
		       &fileMob)); /* result (OUT) */

  if ( argMask & ARG_AVR )
    {
      omfFrameLayout_t aLayout;
      omfRational_t aEditRate;
      omfInt16 aBitsPerPixel;
      omfPixelFormat_t aPixelFmt;

		omfiDatakindLookup(filePtr, PICTUREKIND, &pictureKind, &status);
		check(status);

      status = avrVideoMediaCreate(filePtr, masterMob,1, fileMob,
				videoRate, avrData, videoSignal, &mediaPtr);
	  if(status == OM_ERR_NOT_IMPLEMENTED)
		{
			printf("Unknown AVR specified.\n");
			usage();
		}
		else
			check(status);

      /* avr codec sets the image parameters.  To find out what they are */
      /* set to, call GetVideoInfo. */

      check(omfmGetVideoInfo(mediaPtr, &aLayout, &omfWidth,
			     &omfHeight, &aEditRate, &aBitsPerPixel,
			     &aPixelFmt));

	  if(strncmp(avrData.avr, "Unc", 3) == 0)
	  	aBitsPerPixel = 24;
	  if(strncmp(avrData.avr, "JFI", 3) == 0)
	  	aBitsPerPixel = 24;
      twoField = (aLayout == kSeparateFields || aLayout == kMixedFields ? TRUE : FALSE);
      omfSize = (omfWidth * omfHeight * (aBitsPerPixel/8))*(twoField ? 2 : 1);
      ScaledPixels = (unsigned char *) omfsMalloc((size_t)omfSize);
	  memset(ScaledPixels, 0, ((size_t)omfSize));
    }
  else
    check(omfmVideoMediaCreate(filePtr, masterMob, 1, fileMob,
			       kToolkitCompressionDisable,
			       videoRate,
			       height,
			       width,
			       kFullFrame,
			       imageAspectRatio, &mediaPtr));



  tmplen = strlen(avrData.avr);
  progressiveptr = avrData.avr;
  progressiveptr += tmplen - 1;
  if ((*progressiveptr != 'P') && (*progressiveptr != 'p'))
	{/* progressive resolutions are FullFrame, not fielded */
		check(omfmSetVideoLineMap(mediaPtr, 0,kTopField1));
	}


  PixBuffer = (unsigned char *) omfsMalloc((size_t)(((ulong)omfSize > length) ? omfSize : length));
  memset(PixBuffer,0,((size_t)(((ulong)omfSize > length) ? omfSize : length)));
  PixelPtr = (unsigned char *) omfsMalloc((size_t)(nPixels/32)*4);
  memset(PixelPtr,0,((size_t)(nPixels/32)*4));


  if(PixBuffer == NULL || PixelPtr == NULL)
    check(OM_ERR_NOMEMORY);

  bIndex = 0;

  /* deal with 1/32 of the pixels in one shot.  This avoids having to have */
  /* both a large 32bit pixel buffer *and* a large 24bit pixel buffer */
  for (i = 0; i < 33; i++)
    {
      if (i < 32)
	pixelsToRead = nPixels / 32;
      else
	pixelsToRead = nPixels % 32;

      nBytes = pixelsToRead * 4;

      nBytes = fread(PixelPtr, 1, nBytes, inFP);

      pIndex = 0;
      for (j = 0; j < pixelsToRead; j++)
	{
	  /* chyron format is Alpha BGR */
	  PixBuffer[bIndex++] = PixelPtr[pIndex+3];  /* R */
	  PixBuffer[bIndex++] = PixelPtr[pIndex+2];  /* G */
	  PixBuffer[bIndex++] = PixelPtr[pIndex+1];  /* B */
	  pIndex += 4;
	}
    }

  omfsFree(PixelPtr); PixelPtr = NULL;

  fclose(inFP);

  if (argMask & ARG_AVR)
    {
      /* if avr data, then consider that the output image might be larger */
      /* than the input image.  If so, center the input picture in the */
      /* output picture, surrounded by black.  Also, check the display */
      /* rectangle for more clues. */

      omfInt32 leftover, left, right, top, bottom, adjHeight, h_skip;
      omfInt32 omfDisplayHeight;
      omfRect_t displayRect;

      check(omfmGetDisplayRect(mediaPtr, &displayRect));

      /* aHeight is Height of input buffer copied to output */

      tmplen = strlen(avrData.avr);
      progressiveptr = avrData.avr;
      progressiveptr += tmplen - 1;
      if ((*progressiveptr == 'P') || (*progressiveptr == 'p'))
      {/* progressive resolutions are FullFrame, not fielded */
		adjHeight = height;
	  }
	  else
	  {/* since mc is always field based, only use half */
		adjHeight = height / 2;
	  }

      omfDisplayHeight = omfHeight - displayRect.yOffset;
      if (omfDisplayHeight  > adjHeight)
	{
	  leftover = omfDisplayHeight - adjHeight;
	  top = leftover / 2;
	  bottom = leftover - top;
	  aHeight = adjHeight;
	}
      else
	{
	  top = bottom = 0;
	  aHeight = omfDisplayHeight;
	}

      /* aWidth is width of input buffer copied to output */
      if (omfWidth > width)
	{
	  left = (omfWidth - width) / 2;
	  right = (omfWidth - width) - left;
	  aWidth = width;
	  h_skip = 1;
	}
      else if (omfWidth * 2 <= width)
	{
	  left = right = 0;
	  aWidth = omfWidth;
	  h_skip = 2;
	}
      else
	{
	  left = right = 0;
	  aWidth = omfWidth;
	  h_skip = 1;
	}

      bIndex = 0;

      if (twoField)
	{
	  /* do second field first*/
	  /* put black in first displayRect.yOffset lines plus extra top*/
	  for (i = 0; i < (aWidth * (displayRect.yOffset + top)); i++)
	    {
	      ScaledPixels[bIndex++] = 0; /* R */
	      ScaledPixels[bIndex++] = 0; /* G */
	      ScaledPixels[bIndex++] = 0; /* B */
	    }

	  for (i = 0; i < aHeight; i++)
	    {
	      for (j = 0; j < left; j++)
		{
		  ScaledPixels[bIndex++] = 0; /* R */
		  ScaledPixels[bIndex++] = 0; /* G */
		  ScaledPixels[bIndex++] = 0; /* B */
		}
	      for (j = 0; j < aWidth; j++)
		{
		  /*                rows      +  column + color offset */
		  ScaledPixels[bIndex++] =
		    PixBuffer[(i*step*width*3) +  (j*h_skip*3) +   0 ]; /* R */

		  ScaledPixels[bIndex++] =
		    PixBuffer[(i*step*width*3) +  (j*h_skip*3) +   1 ]; /* G */

		  ScaledPixels[bIndex++] =
		    PixBuffer[(i*step*width*3) +  (j*h_skip*3) +   2 ]; /* B */
		}
	      for (j = 0; j < right; j++)
		{
		  ScaledPixels[bIndex++] = 0; /* R */
		  ScaledPixels[bIndex++] = 0; /* G */
		  ScaledPixels[bIndex++] = 0; /* B */
		}
	    }
	  /* put black in any bottom black lines */
	  for (i = 0; i < (aWidth * bottom); i++)
	    {
	      ScaledPixels[bIndex++] = 0; /* R */
	      ScaledPixels[bIndex++] = 0; /* G */
	      ScaledPixels[bIndex++] = 0; /* B */
	    }

	}
      /* put black in first displayRect.yOffset lines plus extra top*/
      for (i = 0; i < (aWidth * (displayRect.yOffset + top)); i++)
	{
	  ScaledPixels[bIndex++] = 0; /* R */
	  ScaledPixels[bIndex++] = 0; /* G */
	  ScaledPixels[bIndex++] = 0; /* B */
	}

      for (i = 0; i < aHeight; i++)
	{
	  for (j = 0; j < left; j++)
	    {
	      ScaledPixels[bIndex++] = 0; /* R */
	      ScaledPixels[bIndex++] = 0; /* G */
	      ScaledPixels[bIndex++] = 0; /* B */
	    }
	  for (j = 0; j < aWidth; j++)
	    {
	      /* for Avid media, if two field, use as is.  If one field,
	       * average the even and odd lines.  if "quarter size"
	       * (AVR 1, 2, 1-3e, multicam),
	       * average both horizontally and vertically.
		   */
	      if ((*progressiveptr == 'P') || (*progressiveptr == 'p'))
		  {/* progressive resolutions are FullFrame, not fielded */
		  /*                     rows      +  column + color offset */
		  ScaledPixels[bIndex++] =
		    PixBuffer[((i+1)*width*3) + (j*h_skip*3)+0]; /* R */

		  ScaledPixels[bIndex++] =
		    PixBuffer[((i+1)*width*3) + (j*h_skip*3)+1]; /* G */

		  ScaledPixels[bIndex++] =
		    PixBuffer[((i+1)*width*3) + (j*h_skip*3)+2]; /* B */
		}

	      else if (twoField)
		{
		  /*                     rows      +  column + color offset */
		  ScaledPixels[bIndex++] =
		    PixBuffer[(((i*step)+1)*width*3) + (j*h_skip*3)+0]; /* R */

		  ScaledPixels[bIndex++] =
		    PixBuffer[(((i*step)+1)*width*3) + (j*h_skip*3)+1]; /* G */

		  ScaledPixels[bIndex++] =
		    PixBuffer[(((i*step)+1)*width*3) + (j*h_skip*3)+2]; /* B */
		}
	      else if (h_skip == 1)
		{
		  ScaledPixels[bIndex++] =
		    (PixBuffer[(((i*step)+1)*width*3)+(j*h_skip*3)] +
		     PixBuffer[(((i*step))*width*3)+(j*h_skip*3)])/2; /* R */

		  ScaledPixels[bIndex++] =
		    (PixBuffer[(((i*step)+1)*width*3)+(j*h_skip*3)+1] +
		     PixBuffer[(((i*step))*width*3)+(j*h_skip*3)+1])/2; /* G */

		  ScaledPixels[bIndex++] =
		    (PixBuffer[(((i*step)+1)*width*3)+(j*h_skip*3)+2] +
		     PixBuffer[(((i*step))*width*3)+(j*h_skip*3)+2])/2; /* B */
		}
	      else
		{
		  ScaledPixels[bIndex++] =
		    (PixBuffer[(((i*step)+1)*width*3)+(j*h_skip*3)] +
		     PixBuffer[(((i*step))*width*3)+(j*h_skip*3)] +
		     PixBuffer[(((i*step)+1)*width*3)+(((j*h_skip)+1)*3)] +
		     PixBuffer[(((i*step))*width*3)+(((j*h_skip)+1)*3)])/4; /* R */

		  ScaledPixels[bIndex++] =
		    (PixBuffer[(((i*step)+1)*width*3)+(j*h_skip*3)+1] +
		     PixBuffer[(((i*step))*width*3)+(j*h_skip*3)+1] +
		     PixBuffer[(((i*step)+1)*width*3)+(((j*h_skip)+1)*3)+1] +
		     PixBuffer[(((i*step))*width*3)+(((j*h_skip)+1)*3)+1])/4; /* G */

		  ScaledPixels[bIndex++] =
		    (PixBuffer[(((i*step)+1)*width*3)+(j*h_skip*3)+2] +
		     PixBuffer[(((i*step))*width*3)+(j*h_skip*3)+2] +
		     PixBuffer[(((i*step)+1)*width*3)+(((j*h_skip)+1)*3)+2] +
		     PixBuffer[(((i*step))*width*3)+(((j*h_skip)+1)*3)+2])/4; /* B */
		}
	    }
	  for (j = 0; j < right; j++)
	    {
	      ScaledPixels[bIndex++] = 0; /* R */
	      ScaledPixels[bIndex++] = 0; /* G */
	      ScaledPixels[bIndex++] = 0; /* B */
	    }
	}

      /* put black in any bottom black lines */
      for (i = 0; i < (aWidth * bottom); i++)
	{
	  ScaledPixels[bIndex++] = 0; /* R */
	  ScaledPixels[bIndex++] = 0; /* G */
	  ScaledPixels[bIndex++] = 0; /* B */
	}
    }

  for (i = 0; i < nframes; i++)
    {
      fprintf(stdout, ".");
      fflush(stdout);
      if (argMask & ARG_AVR)
	{
	  check(omfmWriteDataSamples(	mediaPtr,
				     1, /* number of frames */
				     (void *)ScaledPixels, /* buffer */
				     (int )omfSize));
	}
      else
	{
	  check(omfmWriteDataSamples(	mediaPtr,
				     1, /* number of frames */
				     (void *)PixBuffer, /* buffer */
				     (int )(height*width*3L)));
	}
    }

#if OMF_CODEC_DIAGNOSTICS
  /* run diags before closing.  If media is closed,
     codec will ASSERT-fail */
  check(codecRunDiagnostics(mediaPtr, stdout));
#endif

  check(omfmMediaClose(mediaPtr));

  if (argMask & ARG_AVR)
    {
      omfsFree(ScaledPixels); ScaledPixels = NULL;
    }
  omfsFree(PixBuffer); PixBuffer = NULL;

  check(omfsCloseFile(filePtr));
  check(omfsEndSession(sessionPtr));
  fprintf(stdout, "\nDone\n");
  return(0);
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
