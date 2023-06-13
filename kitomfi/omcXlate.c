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

/*
 * Name: omcStd.c
 *
 * Function: The standard codec stream handlers for translating between various
 *			media formats.
 *
 * Audience: Toolkit users who want to create their own versions of the translators
 *			which handle more cases than the free version.
 */

#include "masterhd.h"
#include <string.h>
#include <stdio.h>

#include "omPublic.h"
#include "omPvt.h"
#include "omcStd.h" 


#define BITS_PER_UINT8		8

#define SCALEBITS	(16)
#define ONE_HALF	((int ) 1 << (SCALEBITS-1))
#define FIX(x)		((int ) ((x) * (1L<<SCALEBITS) + 0.5))
#define MAXJSAMPLE	(255)
#define RIGHT_SHIFT(x,shft)	((x) >> (shft))

typedef struct
{
	omfPixelFormat_t		pixFormat;
	omfFrameLayout_t		layout;
	omfRect_t				storedRect;
	omfBool					validMemFmt;
	omfInt16				pixelSize;
	omfCompSizeArray_t		compSize;
	omfCompArray_t			compLayout;
	omfFieldTop_t			topField;
	omfInt16				blackLevel;
	omfInt16				whiteLevel;
	omfInt16				colorRange;
	omfColorSiting_t		colorSiting;
	/* lineSize may be larger than frame width in a memory format, if preparing
	 * for display which has a line width which must be a power of 2
	 */
	omfInt32				fixedLineLength;
} omfVideoMemInfo_t;

typedef struct
{
	omfAudioSampleType_t	format;		/* OffsetBinary or SignedMagnitude */
	omfInt16           		sampleSize;
	omfRational_t   		sampleRate;
	omfInt16				numChannels;
} omfAudioMemInfo_t;

static void audioFormatFromOpcodeList(omfAudioMemOp_t *ops, omfAudioMemInfo_t *out);
static void videoFormatFromOpcodeList(omfVideoMemOp_t *ops, omfVideoMemInfo_t *out);

typedef enum { YUV422, YUV444, YUVOther } YUVType_t;

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfErr_t translateAudioSamples(omfCodecStream_t *stream,
				                 omfCStrmSwab_t * src,
				                 omfAudioMemInfo_t *srcInfo,
				                 omfCStrmSwab_t * dest,
				                 omfAudioMemInfo_t	*destInfo,
				                 omfBool			fileBigEndian)
{
	omfInt16			extraBits;
	omfUInt32			numSamples;
	omfInt32            r, idx;
	omfUInt32			n,srcBitOffset, destBitOffset, midpoint;
	omfUInt8			*out, *in;
	omfUInt32			samplesWritten;
	omfUInt8			aDestSample[5];
	omfUInt32			aSample, startSrcWord, endSrcWord, startDestWord, endDestWord, sampleMask;
	omfRational_t		multiplier;
	
	omfAssert((((omfInt32)src->buf & 0x03) == 0), stream->mainFile, OM_ERR_NOT_LONGWORD);
	omfAssert((((omfInt32)dest->buf & 0x03) == 0), stream->mainFile, OM_ERR_NOT_LONGWORD);
			
	XPROTECT(stream->mainFile)
	{
		XASSERT((srcInfo->sampleSize % 4) == 0, OM_ERR_TRANSLATE);
		XASSERT((destInfo->sampleSize % 4) == 0, OM_ERR_TRANSLATE);

		multiplier.numerator = srcInfo->sampleRate.numerator * destInfo->sampleRate.denominator;
		multiplier.denominator = srcInfo->sampleRate.denominator * destInfo->sampleRate.numerator;
		if(multiplier.numerator >= multiplier.denominator)
		{
			if(multiplier.numerator % multiplier.denominator != 0)	
				RAISE(OM_ERR_TRANSLATE_NON_INTEGRAL_RATE);
			multiplier.numerator /= multiplier.denominator;
			multiplier.denominator = 1;
		}
		else
		{
			if(multiplier.denominator % multiplier.numerator != 0)	
				RAISE(OM_ERR_TRANSLATE_NON_INTEGRAL_RATE);
			multiplier.denominator /= multiplier.numerator;
			multiplier.numerator = 1;
		}
	
		numSamples = (src->buflen * BITS_PER_UINT8)/srcInfo->sampleSize;
		extraBits = (omfInt16)((src->buflen * BITS_PER_UINT8)%srcInfo->sampleSize);
		XASSERT(extraBits == 0, OM_ERR_XFER_NOT_BYTES);
			
		in = (omfUInt8 *)src->buf;
		out = (omfUInt8 *)dest->buf;
		for(n = 0; n < dest->buflen; n++)
			out[n] = 0;					/* Clear the destination buffer */
			
		srcBitOffset = 0;
		destBitOffset = 0;
		samplesWritten = 0;
		while(samplesWritten < numSamples)
		{
			startSrcWord = srcBitOffset/BITS_PER_UINT8;
			endSrcWord = (srcBitOffset+srcInfo->sampleSize-1)/BITS_PER_UINT8;
			startDestWord = destBitOffset/BITS_PER_UINT8;
			endDestWord = (destBitOffset+destInfo->sampleSize-1)/BITS_PER_UINT8;
			sampleMask = (1L << srcInfo->sampleSize)-1;
			midpoint = (1L << (srcInfo->sampleSize-1));
			
			aSample = 0;
			for(n = startSrcWord; n <= endSrcWord; n++)
			{
				aSample <<= 8;
				aSample |= in[n];
			}
			aSample <<=  (srcBitOffset%BITS_PER_UINT8);
			
			if((srcInfo->sampleSize % 8) != 0)
			{
				aSample >>= (8 - (srcInfo->sampleSize % 8));
			}
			aSample &= sampleMask;
			
			if(destInfo->format != srcInfo->format)
			{
				if(destInfo->format == kOmfOffsetBinary)
					aSample = aSample + midpoint;
				if(destInfo->format == kOmfSignedMagnitude)
					aSample = aSample - midpoint;
			}
			
			for(r = 1; r <= multiplier.denominator; r++)
			{
				srcBitOffset += srcInfo->sampleSize;
			}

			aSample <<= 32 - destInfo->sampleSize;		/* Put the sample into the MS bits */
			
			for(r = 1; r <= multiplier.numerator; r++)
			{
				if(fileBigEndian)
				{
					aDestSample[0] = (omfUInt8) (aSample >> (24L + (destBitOffset % 8)));
					aDestSample[1] = (omfUInt8)(aSample >> (16L + (destBitOffset % 8)));
					aDestSample[2] = (omfUInt8)(aSample >> (8L + (destBitOffset % 8)));
					aDestSample[3] = (omfUInt8)(aSample >> (0L + (destBitOffset % 8)));
					aDestSample[4] = (omfUInt8)(aSample << (8L - (destBitOffset % 8)));
				}
				else
				{
					aDestSample[3] = (omfUInt8)(aSample >> (0L + (destBitOffset % 8)));
					aDestSample[2] = (omfUInt8)(aSample >> (8L + (destBitOffset % 8)));
					aDestSample[1] = (omfUInt8)(aSample >> (16L + (destBitOffset % 8)));
					aDestSample[0] = (omfUInt8)(aSample >> (24L + (destBitOffset % 8)));
				}

				for(n = startDestWord, idx = 0; n <= endDestWord; n++, idx++)
					out[n] |= aDestSample[idx];
				destBitOffset += destInfo->sampleSize;
				samplesWritten++;
			}
			if(samplesWritten >= numSamples)
				break;
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

static omfErr_t changePixelFormat(omfCodecStream_t	*stream,
				                 omfUInt8		 	*srcBuf,
				                 int 		 		srcBufLen,
				                 omfVideoMemInfo_t	*srcInfo,
				                 omfUInt8		 	*destBuf,
				                 int 				destBufLen,
				                 omfVideoMemInfo_t	*destInfo,
								omfInt32			*newLen)
{
	omfUInt8	srcVal, destVal, *srcPtr, *destPtr;
	omfUInt8   	*srcMax, *destMax;
	omfInt16	srcIter, destIter, srcSize, destSize, destOff, n;
	omfInt16	xformMatrix[MAX_NUM_RGBA_COMPS];
	omfUInt8	tmpBuf[MAX_NUM_RGBA_COMPS];
	omfInt16	fillMatrix[MAX_NUM_RGBA_COMPS];
	omfInt32	colorBuf[MAX_NUM_RGBA_COMPS];
	omfInt16	fillSize = 0;
	omfBool		found;
	YUVType_t	YUVType;
	omfInt32	src, dest;
	omfInt32	srcIncr, destIncr;
	omfInt32	intBufLen;		/* Intermediate */
	
	XPROTECT(NULL)
	{
		if(srcInfo->pixFormat == kOmfPixRGBA)
		{
			for(srcIter = 0, srcSize = 0; srcIter < MAX_NUM_RGBA_COMPS; srcIter++, srcSize++)
			{
				if(srcInfo->compSize[srcIter] == 0)
					break;				
				if(srcInfo->compSize[srcIter] != 8)
					RAISE(OM_ERR_TRANSLATE);
			}
		}
		else if(srcInfo->pixFormat == kOmfPixYUV)
		{
			srcSize = srcInfo->pixelSize / 8;
		}
		else
			RAISE(OM_ERR_TRANSLATE);

		if(destInfo->pixFormat == kOmfPixRGBA)
		{
			for(destIter = 0, destSize = 0; destIter < MAX_NUM_RGBA_COMPS; destIter++, destSize++)
			{
				if(destInfo->compSize[destIter] == 0)
					break;				
				if(destInfo->compSize[destIter] != 8)
					RAISE(OM_ERR_TRANSLATE);
			}
		}
		else if(destInfo->pixFormat == kOmfPixYUV)
		{
			destSize = destInfo->pixelSize / 8;
		}
		else
			RAISE(OM_ERR_TRANSLATE);
		
		if((srcInfo->pixFormat == kOmfPixYUV) && (destInfo->pixFormat == kOmfPixRGBA))
		{
			omfUInt16	y1, y2;
			omfInt16	Cb, Cr;
			omfInt32	CrToR[256], CbToG[256], CrToG[256], CbToB[256];
			omfUInt16	CCIRtoRGB[256];
			omfInt16	saveDestSize;
			omfInt32	x2;

			saveDestSize = destSize;
			destSize = 3;
			if(srcSize * 3 == destSize * 2)		/* If 4:2:2 */
				YUVType = YUV422;
			else if(srcSize == destSize)		/* If 4:4:4 */
				YUVType = YUV444;
			else
				RAISE(OM_ERR_TRANSLATE);
		
			if(YUVType == YUV422)		/* If 4:2:2 */
			{
				srcIncr = srcSize * 2;
				destIncr = destSize * 2;	/* Sizes of 2-pixel 4:2:2 blocks */
			}
			else
			{
				srcIncr = srcSize;
				destIncr = destSize;	/* Sizes of 2-pixel 4:4:4 blocks */
			}

			/* The following RGB->YCbCr->RGB code was lifted directly from 
			 * jdcolor.c, et. al.
			 */

			/*
			 *	R = Y                +  * Cr
			 *	G = Y - 0.34414 * Cb - 0.71414 * Cr
			 *	B = Y + 1.77200 * Cb
			 */

			for (n = 0; n <= MAXJSAMPLE; n++) 
			{
				/* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
				/* The Cb or Cr value we are thinking of is x = i - MAXJSAMPLE/2 */
				x2 = 2*n - MAXJSAMPLE;	/* twice x */
				/* Cr=>R value is nearest int to 1.40200 * x */
				CrToR[n] = (omfInt32)
						RIGHT_SHIFT(FIX(1.40200/2) * x2 + ONE_HALF, SCALEBITS);
				/* Cb=>B value is nearest int to 1.77200 * x */
				CbToB[n] = (omfInt32)
						RIGHT_SHIFT(FIX(1.77200/2) * x2 + ONE_HALF, SCALEBITS);
				/* Cr=>G value is scaled-up -0.71414 * x */
				CrToG[n] = (- FIX(0.71414/2)) * x2;
				/* Cb=>G value is scaled-up -0.34414 * x */
				/* We also add in ONE_HALF so that need not do it in inner loop */
				CbToG[n] = (- FIX(0.34414/2)) * x2 + ONE_HALF;
			}
			

 			for(n = 0; n < 256; n++)
			{
				if(n <= 16)
					CCIRtoRGB[n] = 0;
				else if(n >= 235) 
					CCIRtoRGB[n] = 255;
				else
					CCIRtoRGB[n] = (omfUInt8) (((n-16) * 255) / 219.0 + 0.5);
			}

 
 			intBufLen = (srcBufLen * destSize) / srcSize;
 			for(src = srcBufLen - srcIncr, dest = intBufLen - destIncr;
				 src >= 0 ;
				  src -= srcIncr, dest -= destIncr)
			{
				if(YUVType == YUV422)		/* If 4:2:2 */
				{
					Cb = srcBuf[src+0] ;
					y1 = srcBuf[src+1] ;
					Cr = srcBuf[src+2] ;
					y2 = srcBuf[src+3] ;
					colorBuf[0] = y1 + CrToR[Cr];
					colorBuf[1] = y1 +((int) RIGHT_SHIFT(CbToG[Cb] + CrToG[Cr], SCALEBITS));
					colorBuf[2] = y1 + CbToB[Cb];
					colorBuf[3] = y2 + CrToR[Cr];
					colorBuf[4] = y2 +((int) RIGHT_SHIFT(CbToG[Cb] + CrToG[Cr], SCALEBITS));
					colorBuf[5] = y2 + CbToB[Cb];
					for(n = 0; n < 6; n++)
					{
						if(colorBuf[n] < 0)
							destBuf[dest+n] = 0;
						else if(colorBuf[n] > 255)
							destBuf[dest+n] = 255;
						else
							destBuf[dest+n] = (omfUInt8) CCIRtoRGB[ colorBuf[n] ];
					}
				}
				else
				{
					y1 = srcBuf[src+0];
					Cb = srcBuf[src+1];
					Cr = srcBuf[src+2];
					colorBuf[0] = y1 + CrToR[Cr];
					colorBuf[1] = y1 +((int) RIGHT_SHIFT(CbToG[Cb] + CrToG[Cr], SCALEBITS));
					colorBuf[2] = y1 + CbToB[Cb];
					for(n = 0; n < 3; n++)
					{
						if(colorBuf[n] < 0)
							destBuf[dest+n] = 0;
						else if(colorBuf[n] > 255)
							destBuf[dest+n] = 255;
						else
							destBuf[dest+n] = (omfUInt8) CCIRtoRGB[ colorBuf[n] ];
					}
				}
			}

			srcInfo->pixFormat = kOmfPixRGBA;
			srcInfo->compSize[0] = 8;
			srcInfo->compSize[1] = 8;
			srcInfo->compSize[2] = 8;
			srcInfo->compSize[3] = 0;
			srcInfo->compLayout[0] = 'R';
			srcInfo->compLayout[1] = 'G';
			srcInfo->compLayout[2] = 'B';
			srcInfo->compLayout[3] = 0;
			srcInfo->pixelSize = 24;
			srcSize = 3;
			srcBufLen = intBufLen;
			srcBuf = destBuf;
			destSize = saveDestSize;
		} 

		if((srcInfo->pixFormat == kOmfPixRGBA) && (destInfo->pixFormat == kOmfPixYUV))
		{
			omfUInt16	r1, g1, b1, r2, g2, b2;
			omfInt32	RToY[256], GToY[256], BToY[256];
			omfInt32	RToCb[256], GToCb[256], BToCb[256];
			omfInt32	RToCr[256], GToCr[256], BToCr[256];
			omfUInt16	RGBToCCIR[256];
			
			if(srcSize * 2 == destSize * 3)		/* If 4:2:2 */
				YUVType = YUV422;
			else if(srcSize == destSize)		/* If 4:4:4 */
				YUVType = YUV444;
			else
				RAISE(OM_ERR_TRANSLATE);
		
			if(YUVType == YUV422)		/* If 4:2:2 */
			{
				srcIncr = srcSize * 2;
				destIncr = destSize * 2;	/* Sizes of 2-pixel 4:2:2 blocks */
			}
			else
			{
				srcIncr = srcSize;
				destIncr = destSize;	/* Sizes of 2-pixel 4:4:4 blocks */
			}


			/* The following RGB->YCbCr code was lifted directly from 
			 * jccolor.c, et. al.
			 */
			for(n = 0; n <= MAXJSAMPLE; n++)
			{
				RToY[n] =  FIX(0.29900) * n;
				GToY[n] =  FIX(0.58700) * n;
				BToY[n] =  FIX(0.11400) * n  + ONE_HALF;
				RToCb[n] = (-FIX(0.16874)) * n;
				GToCb[n] = (-FIX(0.33126)) * n;
				BToCb[n] = FIX(0.50000) * n  + ONE_HALF*(MAXJSAMPLE+1);
				RToCr[n] = FIX(0.50000) * n  + ONE_HALF*(MAXJSAMPLE+1);
				GToCr[n] = (-FIX(0.41869)) * n;
				BToCr[n] = (-FIX(0.08131)) * n;
			}
			
			/*
			 *	Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
			 *	Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B  + MAXJSAMPLE/2
			 *	Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B  + MAXJSAMPLE/2
			 */

 			intBufLen = (srcBufLen * destSize) / srcSize;
  			for(n = 0; n < 256; n++)
			{
				/* Maps colors in the 0-255 (RGB) range to the 16-235 (CCIR) range. */
				RGBToCCIR[n] = (omfUInt8) ( ((n * 219) / 255.0) + 16.5);
			}

			for(src = srcBufLen - srcIncr, dest = intBufLen - destIncr;
				 src >= 0 ;
				  src -= srcIncr, dest -= destIncr)
			{
				if(YUVType == YUV422)		/* If 4:2:2 */
				{
					r1 = RGBToCCIR[ srcBuf[src+0] ];
					g1 = RGBToCCIR[ srcBuf[src+1] ];
					b1 = RGBToCCIR[ srcBuf[src+2] ];  
					r2 = RGBToCCIR[ srcBuf[src+3] ];
					g2 = RGBToCCIR[ srcBuf[src+4] ];
					b2 = RGBToCCIR[ srcBuf[src+5] ];
					destBuf[dest+0] = (omfUInt8)((RToCb[r1] + GToCb[g1] + BToCb[b1]) >> SCALEBITS);	/* Cb */
					destBuf[dest+1] = (omfUInt8)((RToY[r1] + GToY[g1] + BToY[b1]) >> SCALEBITS );		/* Y1 */
					destBuf[dest+2] = (omfUInt8)((RToCr[r2] + GToCr[g2] + BToCr[b2]) >> SCALEBITS);	/* Cr */
					destBuf[dest+3] = (omfUInt8)((RToY[r2] + GToY[g2] + BToY[b2]) >> SCALEBITS );		/* Y2 */
				}
				else
				{
					r1 = RGBToCCIR[ srcBuf[src+0] ];
					g1 = RGBToCCIR[ srcBuf[src+1] ];
					b1 = RGBToCCIR[ srcBuf[src+2] ];
					destBuf[dest+0] = (omfUInt8)((RToY[r1] + GToY[g1] + BToY[b1]) >> SCALEBITS );		/* Y1 */
					destBuf[dest+1] = (omfUInt8)((RToCb[r1] + GToCb[g1] + BToCb[b1]) >> SCALEBITS);	/* Cb */
					destBuf[dest+2] = (omfUInt8)((RToCr[r1] + GToCr[g1] + BToCr[b1]) >> SCALEBITS);	/* Cr */
				}
		}

			srcInfo->pixFormat = kOmfPixYUV;
			srcSize = destSize;
			srcBufLen = intBufLen;
		} 

		if(srcSize != 0 && destSize != 0 && (destInfo->pixFormat == kOmfPixRGBA))
		{
			for(srcIter = 0; srcIter < srcSize; srcIter++)
			{
				srcVal = srcInfo->compLayout[srcIter];
				xformMatrix[srcIter] = -1;
				for(destIter = 0; destIter < destSize; destIter++)
				{
					if(srcVal == destInfo->compLayout[destIter])
					{
						xformMatrix[srcIter] = destIter;
						break;
					}
				}
			}
			for(destIter = 0; destIter < destSize; destIter++)
			{
				found = FALSE;
				destVal = destInfo->compLayout[destIter];
				for(srcIter = 0; srcIter < srcSize; srcIter++)
				{
					if(destVal == srcInfo->compLayout[srcIter])
					{
						found = TRUE;
						break;
					}
				}
				if(!found)
					fillMatrix[fillSize++] = destIter;
			}
	
			/* assert that buffer lengths are an integral number of samples*/
			XASSERT((srcBufLen % (srcInfo->pixelSize / 8)) == 0, OM_ERR_TRANSLATE);
			XASSERT((destBufLen % (destInfo->pixelSize / 8)) == 0, OM_ERR_TRANSLATE);
			if(srcBufLen/srcSize > destBufLen/destSize)
			  srcBufLen = (destBufLen / destSize) * srcSize;
			else if(destBufLen/destSize > srcBufLen/srcSize)
			  destBufLen = (srcBufLen / srcSize) * destSize;
			srcMax = srcBuf + (srcBufLen - srcSize);
			destMax = destBuf + (destBufLen - destSize);

			if(srcSize < destSize)
			{
				destPtr = destBuf + (destBufLen - destSize);
				for(srcPtr = srcMax; srcPtr >= srcBuf; srcPtr -= srcSize, destPtr-=destSize)
				{
					for(n = 0; n < srcSize; n++)
					{
						destOff = xformMatrix[n];
						if(destOff >= 0)
							tmpBuf[destOff] = srcPtr[n];
					}
					for(n = 0; n < fillSize; n++)
						tmpBuf[fillMatrix[n]] = 0;
					for(n = 0; n < destSize; n++)
						destPtr[n] = tmpBuf[n];
				}
			}
			else
			{
				destPtr = destBuf;
   				for(srcPtr = srcBuf; (srcPtr <= srcMax) && (destPtr < destMax);
					srcPtr += srcSize, destPtr+=destSize)
				{
					for(n = 0; n < srcSize; n++)
					{
						destOff = xformMatrix[n];
						if(destOff >= 0)
							tmpBuf[destOff] = srcPtr[n];
					}
					for(n = 0; n < fillSize; n++)
						tmpBuf[fillMatrix[n]] = 0;
					for(n = 0; n < destSize; n++)
						destPtr[n] = tmpBuf[n];
				}
			}
		}
		if(newLen)
			*newLen = destBufLen;
	}
	XEXCEPT
	XEND;
		
	return(OM_ERR_NONE);
}

static omfErr_t changeFrameLayout(omfCodecStream_t	*stream,
				                 omfUInt8			 *srcBuf,
				                 omfInt32			srcBuflen,
				                 omfFrameLayout_t	srcLayout,
				                 omfFieldTop_t		topField,
				                 omfInt32			numLines,
				                 omfInt32			lineSize,
								 omfUInt8			*destBuf,
				                 omfInt32			destBuflen,
				                 omfFrameLayout_t	destLayout,
								 omfInt32			*newLen,
								 omfInt32			*outNumLines)
{
	omfUInt8	*intBuf, *tmpBuf = NULL;
	omfUInt32	*transform = NULL;
	omfInt32	offsetField1, offsetField2;
	omfInt32	fieldLine, y, maxLine, newDestLen, newNumLines;
	omfUInt8	*srcRowPtr, *destRowPtr;
	omfUInt32	*transPtr;
	
	XPROTECT(NULL)
	{
		if(topField == kTopField1)
		{
			offsetField1 = 0;
			offsetField2 = 1;
		}
		else
		{
			offsetField1 = 1;
			offsetField2 = 0;
		}
		
		newDestLen = srcBuflen;		/* Default to the same size */
		if((srcLayout == kSeparateFields) && (destLayout == kOneField))
		{
			if(srcBuf != destBuf)
			{
				fieldLine = numLines / 2;
				memcpy(destBuf, srcBuf, fieldLine * lineSize);
			}
			newDestLen = srcBuflen / 2;
			newNumLines = numLines / 2;
		}
		else
		{
			if(srcBuf == destBuf)
			{
				tmpBuf = (omfUInt8 *)omOptMalloc(stream->mainFile, destBuflen);
				if(tmpBuf == NULL)
					RAISE(OM_ERR_NOMEMORY);
				intBuf = tmpBuf;
			}
			else
				intBuf = destBuf;

			if((srcLayout == kSeparateFields) && (destLayout == kFullFrame))
			{
				maxLine = numLines;
				fieldLine = numLines / 2;
				newNumLines = numLines;
				
				transform = (omfUInt32 *)omOptMalloc(stream->mainFile, maxLine * sizeof(*transform));
				if(transform == NULL)
					RAISE(OM_ERR_NOMEMORY);
	
				for(y = 0; y < fieldLine; y++)
					transform[y] = (y * 2) + offsetField1;
				for(y = fieldLine; y < maxLine; y++)
					transform[y] = ((y-fieldLine) * 2) + offsetField2;
	
				for(y = 0; y < maxLine; y++)
				{
					srcRowPtr = srcBuf + (lineSize * y);
					destRowPtr = intBuf + (lineSize * transform[y]);
					memcpy(destRowPtr, srcRowPtr, lineSize);
				}
			}
			else if((srcLayout == kFullFrame) && (destLayout == kSeparateFields))
			{
				maxLine = numLines;
				fieldLine = numLines/2;
				newNumLines = numLines;

				transform = (omfUInt32 *)omOptMalloc(stream->mainFile, maxLine * sizeof(*transform));
				if(transform == NULL)
					RAISE(OM_ERR_NOMEMORY);
	
				for(y = 0; y < fieldLine; y++)
					transform[y] = (y * 2) + offsetField1;
				for(y = fieldLine; y < maxLine; y++)
					transform[y] = ((y-fieldLine) * 2) + offsetField2;
	
				for(y = 0; y < maxLine; y++)
				{
					srcRowPtr = srcBuf + (lineSize * transform[y]);
					destRowPtr = intBuf + (lineSize * y);
					memcpy(destRowPtr, srcRowPtr, lineSize);
				}
			}
			else if((srcLayout == kOneField) && (destLayout == kSeparateFields))
			{
				maxLine = numLines*2;
				newNumLines = numLines * 2;
				transform = (omfUInt32 *)omOptMalloc(stream->mainFile, maxLine * sizeof(*transform));
				if(transform == NULL)
					RAISE(OM_ERR_NOMEMORY);
	
				for(y = 0, transPtr = transform; y < numLines; y++)
					*transPtr++ = y;
				for(y = 0; y < numLines; y++)
					*transPtr++ = y;
	
				for(y = 0; y < maxLine; y++)
				{
					srcRowPtr = srcBuf + (lineSize * transform[y]);
					destRowPtr = intBuf + (lineSize * y);
					memcpy(destRowPtr, srcRowPtr, lineSize);
				}
				newDestLen = srcBuflen * 2;
			}
			else if((srcLayout == kOneField) && (destLayout == kFullFrame))
			{
				newNumLines = numLines * 2;
				for(y = 0; y < numLines; y++)
				{
					srcRowPtr = srcBuf + (lineSize * y);
					destRowPtr = intBuf + (lineSize * (y * 2));
					memcpy(destRowPtr, srcRowPtr, lineSize);
					memcpy(destRowPtr+ lineSize, srcRowPtr, lineSize);
				}
				newDestLen = srcBuflen * 2;
			}
			else if((srcLayout == kFullFrame) && (destLayout == kOneField))
			{
				newNumLines = numLines / 2;
			  maxLine = numLines;
			  for(y = 0; y < maxLine; y += 2)
				{
					srcRowPtr = srcBuf + (lineSize * y);
					destRowPtr = intBuf + (lineSize * (y >> 1L));
					memcpy(destRowPtr, srcRowPtr, lineSize);
				}
				newDestLen = srcBuflen / 2;
			}
			else
				RAISE(OM_ERR_TRANSLATE);
				
			if(intBuf != destBuf)
				memcpy(destBuf, intBuf, destBuflen);
		}
		if(newLen)
		{
			*newLen = newDestLen;
		}
		if(outNumLines)
		{
			*outNumLines = newNumLines;
		}

		if(tmpBuf != NULL)
			omOptFree(stream->mainFile, tmpBuf);
		if(transform != NULL)
			omOptFree(stream->mainFile, transform);
	}
	XEXCEPT
	{
		if(tmpBuf != NULL)
			omOptFree(stream->mainFile, tmpBuf);
		if(transform != NULL)
			omOptFree(stream->mainFile, transform);
	}
	XEND;
	
	return(OM_ERR_NONE);
}

static omfErr_t translateVideoSamples(omfCodecStream_t *stream,
				                 omfCStrmSwab_t * src,
				                 omfVideoMemInfo_t *srcInfo,
				                 omfCStrmSwab_t * dest,
				                 omfVideoMemInfo_t *destInfo)
{
	omfInt32			destLineSize, imageWidthBytes, numLines, newBuflen, newNumLines;
	omfFrameLayout_t	srcLayout, destLayout;
	
	/*	<tss>+ */
	omfInt32 destImageWidthBytes;
	/*	<tss>- */
	
	XPROTECT(NULL)
	{
		imageWidthBytes = (srcInfo->pixelSize / 8) * srcInfo->storedRect.xSize;
		XASSERT(srcInfo->fixedLineLength == 0, OM_ERR_TRANSLATE); /* !!!Can't do this translate on write yet... */
			
		numLines = src->buflen / imageWidthBytes; 
		if(((srcInfo->pixelSize % 8) != 0) || ((srcInfo->pixelSize % 8) != 0))
			RAISE(OM_ERR_TRANSLATE);
		if((srcInfo->pixelSize == 0) || (srcInfo->pixelSize == 0))
			RAISE(OM_ERR_TRANSLATE);
			
/* Put this back, in order to increase testability
		srcSize = (srcInfo->pixelSize / 8) * srcInfo->storedRect.xSize * srcInfo->storedRect.ySize;
		if(srcInfo->layout == kSeparateFields)
			srcSize *= 2;
		destSize = (destInfo->pixelSize / 8) * destInfo->storedRect.xSize * destInfo->storedRect.ySize;
		if(destInfo->layout == kSeparateFields)
			destSize *= 2;

		if((src->buflen < srcSize) || (dest->buflen < destSize))
			RAISE(OM_ERR_TRANSLATE);
*/
		
		/*
		 * By turning kMixedFields into kFullFrame at this point, the number
		 * of possible transformations is reduced.  Since the actual pixel values
		 * are the same, there is no problem
		 */
		srcLayout = (srcInfo->layout == kMixedFields ? kFullFrame : srcInfo->layout);
		destLayout = (destInfo->layout == kMixedFields ? kFullFrame : destInfo->layout);

		if((srcInfo->pixFormat == destInfo->pixFormat) &&
		   (srcInfo->pixelSize == destInfo->pixelSize) &&
		   strncmp(srcInfo->compLayout, destInfo->compLayout, 8) == 0)
		{
			if(srcLayout != destLayout)
			{
				CHECK(changeFrameLayout(stream, (omfUInt8 *)src->buf, src->buflen, srcLayout,
								srcInfo->topField, numLines, imageWidthBytes,
								(omfUInt8 *)dest->buf, dest->buflen, destLayout, NULL, &newNumLines));
			}
			else
			{
				if (src->buflen <= dest->buflen)
					memcpy(dest->buf, src->buf, src->buflen);
				else
					memcpy(dest->buf, src->buf, dest->buflen);
				newNumLines = numLines;
			}
		}
		else
		{
			if(srcLayout == destLayout)
			{
				CHECK(changePixelFormat(stream, (omfUInt8 *)src->buf, src->buflen, srcInfo, (omfUInt8 *)dest->buf,
										dest->buflen, destInfo, &newBuflen));
				newNumLines = numLines;
			}
			else
			{
				/* Try to minimize the intermediate buffer size by doing the pixel
				 * translation wither before or after.
				 */
				if(srcInfo->pixelSize > destInfo->pixelSize)
				{
					CHECK(changePixelFormat(stream, (omfUInt8 *)src->buf, src->buflen, srcInfo, (omfUInt8 *)dest->buf,
											dest->buflen, destInfo, &newBuflen));
				
				/*	<tss>+	There is a problem which results in memory corruption.  The imageWidthBytes
				 *	is calculated using the source pixel size.  If the dest pixel size is smaller than the
				 *	source pixel size, the value for imageWidthBytes will be too large.  This will result
				 *	in the toolkit moving around more bytes than it is supposed to.  The fix is to calculate
				 *	the actual number fo bytes in a line in the dest buffer and use that value.
				 *
				 *	Old Code:
				 *	CHECK(changeFrameLayout(stream, (omfUInt8 *)dest->buf, newBuflen, srcLayout,
				 *					srcInfo->topField, numLines, imageWidthBytes,
				 *					(omfUInt8 *)dest->buf, dest->buflen, destLayout, NULL, &newNumLines));
				 *
				 *	New Code:			
				 */
					destImageWidthBytes = imageWidthBytes *destInfo->pixelSize / srcInfo->pixelSize;
					
					CHECK(changeFrameLayout(stream, (omfUInt8 *)dest->buf, newBuflen, srcLayout,
									srcInfo->topField, numLines, destImageWidthBytes,
									(omfUInt8 *)dest->buf, dest->buflen, destLayout, NULL, &newNumLines));
				/*	<tss>- */
				}
				else
				{
					CHECK(changeFrameLayout(stream, (omfUInt8 *)src->buf, src->buflen, srcLayout,
									srcInfo->topField, numLines, imageWidthBytes,
									(omfUInt8 *)dest->buf, dest->buflen, destLayout, &newBuflen, &newNumLines));
					CHECK(changePixelFormat(stream, (omfUInt8 *)dest->buf, newBuflen, srcInfo, (omfUInt8 *)dest->buf,
											dest->buflen, destInfo, &newBuflen));
				}
			}
			if(newNumLines != 0) /* This can be zero on EOF case */
			  imageWidthBytes = newBuflen / newNumLines;					
		}
			
		if(destInfo->fixedLineLength != 0)
		{
			omfUInt8	*srcPtr, *destPtr;
			omfInt32	y;
			destLineSize = destInfo->fixedLineLength;
			
			XASSERT(destLineSize > imageWidthBytes, OM_ERR_TRANSLATE);	/* Used to ADD padding on mem format only */
			srcPtr = (omfUInt8 *)dest->buf + ((newNumLines-1) * imageWidthBytes);
			destPtr = (omfUInt8 *)dest->buf + ((newNumLines-1) * destLineSize);
			for (y=newNumLines-1 ; y>=0 ; y--)
			{
				memcpy(destPtr, srcPtr, imageWidthBytes);
				srcPtr -= imageWidthBytes;
				destPtr -= destLineSize;
			}
		}
	}
	XEXCEPT
	XEND
		
	return(OM_ERR_NONE);
}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t        stdCodecSwabProc(omfCodecStream_t *stream,
				                 omfCStrmSwab_t	*src,
				                 omfCStrmSwab_t	*dest,
				                 omfBool		swapBytes)
{
	omfHdl_t        	main;
	omfAudioMemInfo_t	srcInfo, destInfo;
	omfVideoMemInfo_t	srcVideoInfo, destVideoInfo;
	omfBool				fileBigEndian;
	
	main = stream->mainFile;
	XPROTECT(main)
	{
		if(src->fmt.mediaKind == main->soundKind)
		{
			/* Src parameters are destination parms overloaded by source parms
			 * so that parms are equal if unspecified by source
			 */
			audioFormatFromOpcodeList(dest->fmt.afmt, &srcInfo);
			audioFormatFromOpcodeList(src->fmt.afmt, &srcInfo);

			/* Dest parameters are source parms overloaded by dest parms
			 * so that parms are equal if unspecified by dest
			 */
			audioFormatFromOpcodeList(src->fmt.afmt, &destInfo);
			audioFormatFromOpcodeList(dest->fmt.afmt, &destInfo);
	
			if((destInfo.sampleSize == srcInfo.sampleSize) &&
				 (destInfo.sampleRate.numerator == srcInfo.sampleRate.numerator) &&
				 (destInfo.sampleRate.denominator == srcInfo.sampleRate.denominator) &&
				 (destInfo.format == srcInfo.format))
			{
				memcpy(dest->buf, src->buf, dest->buflen);
				if (swapBytes)
				{
					if (destInfo.sampleSize == 16)
					{
						omfUInt32	i;
						omfInt16	*samp16bit = (omfInt16 *) dest->buf;
		
						for (i = 0; i < dest->buflen / sizeof(omfInt16); i++, samp16bit++)
							omfsFixShort(samp16bit);
					}
					else if (destInfo.sampleSize == 24)
					{
                  // for now copy without byte swap to test!
						omfUInt32	i;
						omfUInt8	*samp24bit = (omfUInt8 *) dest->buf;

                  // must be a multiple of 3
                  //if ( dest->buflen % 3 != 0 )
                  //   i = 1;

						for (i = 0; i < dest->buflen / 3*sizeof(omfUInt8); i++, samp24bit += 3*sizeof(omfUInt8))
                  {
                     register unsigned char *cp = (unsigned char *) samp24bit;
                     omfUInt8            t;

                     t = cp[2];
                     cp[2] = cp[0];
                     cp[0] = t;
                  }
					}
					else if (destInfo.sampleSize == 32)
					{
						omfUInt32	i;
						omfInt32	*samp32bit = (omfInt32 *) dest->buf;
		
						for (i = 0; i < dest->buflen / sizeof(omfInt32); i++, samp32bit++)
							omfsFixLong(samp32bit);
					}
					else if (destInfo.sampleSize != 8)
						RAISE(OM_ERR_NOAUDIOCONV);
				}
			}
			else
			{
				if(OMNativeByteOrder == MOTOROLA_ORDER)
					fileBigEndian = !swapBytes;
				else
					fileBigEndian = swapBytes;
				translateAudioSamples(stream, src, &srcInfo, dest, &destInfo, fileBigEndian);
			}
		}
		else if(src->fmt.mediaKind == main->pictureKind)
		{
			/* Src parameters are destination parms overloaded by source parms
			 * so that parms are equal if unspecified by source
			 */
			srcVideoInfo.pixelSize = 0;
			srcVideoInfo.compLayout[0] = 0;
			srcVideoInfo.compSize[0] = 0;
			srcVideoInfo.topField = kTopField1;
			videoFormatFromOpcodeList(dest->fmt.vfmt, &srcVideoInfo);
			/* fixedLineLength has NO equivilant in the file space, and it
			 * not currently allowed on write, so ignore it
			 */
			srcVideoInfo.fixedLineLength = 0;
			videoFormatFromOpcodeList(src->fmt.vfmt, &srcVideoInfo);

			/* Dest parameters are source parms overloaded by dest parms
			 * so that parms are equal if unspecified by dest
			 */
			destVideoInfo.pixelSize = 0;
			destVideoInfo.compLayout[0] = 0;
			destVideoInfo.compSize[0] = 0;
			destVideoInfo.topField = kTopField1;
			destVideoInfo.fixedLineLength = 0;
			videoFormatFromOpcodeList(src->fmt.vfmt, &destVideoInfo);
			videoFormatFromOpcodeList(dest->fmt.vfmt, &destVideoInfo);
	
			CHECK(translateVideoSamples(stream, src, &srcVideoInfo, dest, &destVideoInfo));
		}
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

omfErr_t stdCodecSwabLenProc(
			omfCodecStream_t 	*stream,	/* IN - On this media stream */
			omfUInt32			inSize,	/* IN - Given a # of bytes in the file */
			omfLengthCalc_t		direct,
			omfUInt32			*result)		/* OUT - Return the memory size */
{
	omfInt32           fileSamp, memSamp, rateNum, rateDenom, newPixSize;
	omfInt64			result64;
	omfRational_t   	fileRate, memRate;
	omfHdl_t       		main;
	omfAudioMemInfo_t	fileInfo, memInfo;
	omfVideoMemInfo_t	videoFileInfo, videoMemInfo;
	
	main = stream->mainFile;

	omfAssert((stream->memFormat.mediaKind != stream->mainFile->nilKind), main, OM_ERR_SWAP_SETUP);
	omfAssert((stream->fileFormat.mediaKind != stream->mainFile->nilKind), main, OM_ERR_SWAP_SETUP);

	XPROTECT(main)
	{
		if(stream->fileFormat.mediaKind == stream->mainFile->soundKind)
		{
			/* Src parameters are destination parms overloaded by source parms
			 * so that parms are equal if unspecified by source
			 */
			fileInfo.sampleSize = 0;
			fileInfo.sampleRate.numerator = 0;
			fileInfo.sampleRate.denominator = 0;
			fileInfo.numChannels = 0;
			memInfo.sampleSize = 0;
			memInfo.sampleRate.numerator = 0;
			memInfo.sampleRate.denominator = 0;
			memInfo.numChannels = 0;
			audioFormatFromOpcodeList(stream->fileFormat.afmt, &fileInfo);
			
			/* Memory format should be based upon file format with changes */
			audioFormatFromOpcodeList(stream->fileFormat.afmt, &memInfo);
			audioFormatFromOpcodeList(stream->memFormat.afmt, &memInfo);

			fileSamp = (fileInfo.sampleSize + 7)/ 8;
			fileRate = fileInfo.sampleRate;
			if (memInfo.sampleSize == 0)
				memSamp = fileSamp;
			else
				memSamp = (memInfo.sampleSize + 7) / 8;

			memSamp = (memInfo.sampleSize + 7) / 8;
			memRate = memInfo.sampleRate;
			rateNum = memRate.numerator * fileRate.denominator;
			rateDenom = memRate.denominator * fileRate.numerator;
			if(rateNum == rateDenom)
			{
				rateNum = 1;
				rateDenom = 1;
			}
			if(direct == omcComputeMemSize)
			{
				omfsCvtInt32toInt64(inSize, &result64);
				CHECK(omfsMultInt32byInt64(rateNum * memSamp, result64, &result64));
				if((fileSamp == 0) || (rateDenom == 0))
				{
					omfsCvtInt32toInt64(0, &result64);
				}
				else
				{
					CHECK(omfsDivideInt64byInt32(result64, (fileSamp * rateDenom), &result64, NULL));
				}
			}
			else
			{
				omfsCvtInt32toInt64(inSize, &result64);
				CHECK(omfsMultInt32byInt64(rateDenom * fileSamp, result64, &result64));
				if((memSamp == 0) || (rateNum == 0))
				{
					omfsCvtInt32toInt64(0, &result64);
				}
				else
				{
					CHECK(omfsDivideInt64byInt32(result64, (memSamp * rateNum), &result64, NULL));
				}
			}
			CHECK(omfsTruncInt64toUInt32(result64, result));	/* OK MAXREAD */
			
		}
		else if(stream->fileFormat.mediaKind == stream->mainFile->pictureKind)
		{
			videoMemInfo.pixelSize = 0;
			videoMemInfo.compLayout[0] = 0;
			videoMemInfo.compSize[0] = 0;
			videoMemInfo.fixedLineLength = 0;
			videoFileInfo.pixelSize = 0;
			videoFileInfo.compLayout[0] = 0;
			videoFileInfo.compSize[0] = 0;
			videoFileInfo.fixedLineLength = 0;
			videoFormatFromOpcodeList(stream->fileFormat.vfmt, &videoFileInfo);
			
			/* Memory format should be based upon file format with changes */
			videoFormatFromOpcodeList(stream->fileFormat.vfmt, &videoMemInfo);
			videoFormatFromOpcodeList(stream->memFormat.vfmt, &videoMemInfo);
			if(!videoMemInfo.validMemFmt)
				RAISE(OM_ERR_TRANSLATE);
				
			if(direct == omcComputeMemSize)
			{
				newPixSize = (inSize * videoMemInfo.pixelSize) / videoFileInfo.pixelSize;
				if(videoMemInfo.fixedLineLength != 0)
				{
					omfInt32	imageWidthBytes, padLineBytes, fileImageHeight;

					imageWidthBytes = (videoMemInfo.pixelSize / 8) * videoMemInfo.storedRect.xSize;
					padLineBytes = videoMemInfo.fixedLineLength - imageWidthBytes;
					fileImageHeight = videoFileInfo.storedRect.ySize;
					
					/* Used to ADD padding on mem format only */
					XASSERT(padLineBytes > 0, OM_ERR_TRANSLATE);

					newPixSize = newPixSize + (fileImageHeight * padLineBytes);
				}
				
				if((videoMemInfo.layout == kOneField) && (videoFileInfo.layout != kOneField))
					*result = newPixSize / 2;
				else if((videoFileInfo.layout == kOneField) && (videoMemInfo.layout != kOneField))
					*result = newPixSize * 2;
				else
					*result = newPixSize;
			}
			else
			{
				newPixSize = (inSize * videoFileInfo.pixelSize) / videoMemInfo.pixelSize;
				if((videoMemInfo.layout == kOneField) && (videoFileInfo.layout != kOneField))
					*result = newPixSize * 2;
				else if((videoFileInfo.layout == kOneField) && (videoMemInfo.layout != kOneField))
					*result = newPixSize / 2;
				else
					*result = newPixSize;
			}
		}
		else
			RAISE(OM_ERR_TRANSLATE);
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

omfErr_t stdCodecNeedsSwabProc(omfCodecStream_t *stream, omfBool *needsSwabbing)
{
	omfHdl_t				file;
	omfAudioMemInfo_t	audioFileInfo, audioMemInfo;
	omfVideoMemInfo_t	videoFileInfo, videoMemInfo;
	omfInt16			n;
	omfBool				layoutChanged = FALSE;
	
	file = stream->mainFile;
	XPROTECT(file)
	{
		XASSERT(stream->fileFormat.mediaKind == stream->memFormat.mediaKind, OM_ERR_TRANSLATE);
		*needsSwabbing = TRUE;
		if (stream->swapBytes)
			*needsSwabbing = TRUE;
		else if (stream->fileFormat.mediaKind == file->soundKind)
		{
			audioFileInfo.numChannels = 0;
			audioMemInfo.numChannels = 0;
			audioFormatFromOpcodeList(stream->memFormat.afmt, &audioFileInfo);
			audioFormatFromOpcodeList(stream->fileFormat.afmt, &audioFileInfo);
			audioFormatFromOpcodeList(stream->fileFormat.afmt, &audioMemInfo);
			audioFormatFromOpcodeList(stream->memFormat.afmt, &audioMemInfo);

			if((audioMemInfo.sampleSize != audioFileInfo.sampleSize) ||
				 (audioMemInfo.sampleRate.numerator != audioFileInfo.sampleRate.numerator) ||
				 (audioMemInfo.sampleRate.denominator != audioFileInfo.sampleRate.denominator) ||
				 (audioMemInfo.format != audioFileInfo.format) ||
				 (audioMemInfo.numChannels != audioFileInfo.numChannels))
				*needsSwabbing = TRUE;
			else
				*needsSwabbing = FALSE;	/* Pass other types through unaltered */
		}
		else if (stream->fileFormat.mediaKind == file->pictureKind)
		{
			videoMemInfo.pixelSize = 0;
			videoMemInfo.compLayout[0] = 0;
			videoMemInfo.compSize[0] = 0;
			videoMemInfo.fixedLineLength = 0;
			videoFileInfo.pixelSize = 0;
			videoFileInfo.compLayout[0] = 0;
			videoFileInfo.compSize[0] = 0;
			videoFileInfo.fixedLineLength = 0;
			videoFormatFromOpcodeList(stream->memFormat.vfmt, &videoFileInfo);
			videoFormatFromOpcodeList(stream->fileFormat.vfmt, &videoFileInfo);
			videoFormatFromOpcodeList(stream->fileFormat.vfmt, &videoMemInfo);
			videoFormatFromOpcodeList(stream->memFormat.vfmt, &videoMemInfo);
	
			if((videoMemInfo.pixFormat == kOmfPixRGBA) && (videoFileInfo.pixFormat == kOmfPixRGBA))
			{
				for(n = 0, layoutChanged = FALSE; n < MAX_NUM_RGBA_COMPS; n++)
				{
					if(videoMemInfo.compLayout[n] != videoFileInfo.compLayout[n])
						layoutChanged = TRUE;
					if(videoMemInfo.compLayout[n] == 0)
						break;
				}
			}	
			else if(videoMemInfo.pixFormat != videoFileInfo.pixFormat)
				layoutChanged = TRUE;
			
			if(	(videoFileInfo.layout != videoMemInfo.layout) ||
				(videoFileInfo.storedRect.xOffset != videoMemInfo.storedRect.xOffset) ||
				(videoFileInfo.storedRect.yOffset != videoMemInfo.storedRect.yOffset) ||
				(videoFileInfo.storedRect.xSize != videoMemInfo.storedRect.xSize) ||
				(videoFileInfo.storedRect.ySize != videoMemInfo.storedRect.ySize) ||
				(videoFileInfo.pixelSize != videoMemInfo.pixelSize) ||
				(videoMemInfo.fixedLineLength != 0) ||
				layoutChanged)
				*needsSwabbing = TRUE;
			else
				*needsSwabbing = FALSE;	/* Pass other types through unaltered */
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}
		
static void audioFormatFromOpcodeList(omfAudioMemOp_t *ops, omfAudioMemInfo_t *out)
{
	while((ops != NULL) && ops->opcode != kOmfAFmtEnd)
	{
		switch(ops->opcode)
		{
		case kOmfSampleSize:
			out->sampleSize = ops->operand.sampleSize;
			break;
		case kOmfSampleRate:
			out->sampleRate = ops->operand.sampleRate;
			break;
		case kOmfSampleFormat:
			out->format = ops->operand.format;
			break;
		case kOmfNumChannels:
			break;
		}
		ops++;
	}
}

static void videoFormatFromOpcodeList(omfVideoMemOp_t *ops, omfVideoMemInfo_t *out)
{
	omfInt16	n, padBits = 0;
	omfInt32				yuvCompSize = 0;
	omfUInt32			yuvSubsample = 0;
	omfBool		computePixSize = TRUE, computeTopLine = FALSE;
	omfVideoLineMap_t	lineMap;
	
	out->validMemFmt = TRUE;
	
	while((ops != NULL) && (ops->opcode != kOmfVFmtEnd))
	{
		switch(ops->opcode)
		{
		case kOmfPixelFormat:
			out->pixFormat = ops->operand.expPixelFormat;
			break;
		
		case kOmfFrameLayout:
			out->layout = ops->operand.expFrameLayout;
			break;
			
		case kOmfStoredRect:
			out->storedRect = ops->operand.expRect;
			out->validMemFmt = FALSE;
			break;
		case kOmfPixelSize:
			out->pixelSize = ops->operand.expInt16;
			computePixSize = FALSE;
			break;
			
		case kOmfVideoLineMap:
			computeTopLine = TRUE;
			lineMap[0] = ops->operand.expLineMap[0];
			lineMap[1] = ops->operand.expLineMap[1];
			break;

		case kOmfRGBCompLayout:
			memcpy(out->compLayout, ops->operand.expCompArray, sizeof(out->compLayout));
			break;

		case kOmfCDCICompWidth:
			yuvCompSize = ops->operand.expInt32;
			break;
			
		case kOmfCDCIHorizSubsampling:
			yuvSubsample = ops->operand.expUInt32;
			break;
		
		case kOmfRGBCompSizes:
			for(n = 0; n < sizeof(out->compSize); n++)
			{
				out->compSize[n] = ops->operand.expCompSizeArray[n];
				if(out->compSize[n] == 0)
					break;
			}
			
			out->pixelSize = 0;
			for(n = 0; n < MAX_NUM_RGBA_COMPS; n++)
			{
				if(out->compSize[n] != 0)
					out->pixelSize += out->compSize[n];
				else
					break;
			}
			break;

		case kOmfLineLength:
			out->fixedLineLength = ops->operand.expInt32;
			break;

		case kOmfCDCIPadBits:
			padBits = ops->operand.expInt16;

		case kOmfFieldDominance:
			break;

		case kOmfCDCIColorSiting:
			out->colorSiting = ops->operand.expColorSiting;
			break;

		case kOmfCDCIBlackLevel:
			out->blackLevel = (omfInt16)ops->operand.expInt32;
			break;

		case kOmfCDCIWhiteLevel:
			out->whiteLevel = (omfInt16)ops->operand.expInt32;
			break;

		case kOmfCDCIColorRange:
			out->colorRange = (omfInt16)ops->operand.expInt32;
			break;

		default:
			out->validMemFmt = FALSE;
			break;
		}
		ops++;
	}
	
	if(computeTopLine)
	{
		if (out->layout != kSeparateFields &&
			out->layout != kMixedFields)
		{
			out->topField = kTopField1;  /* dominance only relevant for multi-field */
		}
		else
		{
			/* The original OMF Toolkit used (OldStyle) incorrect line numbers
			 * we are now using correct line numbers, which requires coding to a
			 * particular specification, as we can no longer detect field-topness
			 * just by comparing the two numbers.  The NTSC and PAL vertical interval
			 * ranges don't seem to overlap, so we can just compare against constants
			 * without having to know the standard used (for now...)
			 */
			if(lineMap[1] == 278)			/* New Style map, new NTSC media */
				out->topField = kTopField2;
			else if(lineMap[1] == 279)		/* New Style, Old AVR NTSC media (field1) */
				out->topField = kTopField1;
			else if(lineMap[1] == 238)		/* New Style, PAL media (field1) */
				out->topField = kTopField1;
			else if(lineMap[0] < lineMap[1])
				out->topField = kTopField1;  /* Old Style! See note above */
			else
				out->topField = kTopField2;  /* The default is field2 dominant */
		}
	}
	
	if(computePixSize)
	{
		if(yuvSubsample == 1 && yuvCompSize != 0)
			out->pixelSize = (yuvCompSize * 3)+padBits;
		if(yuvSubsample == 2 && yuvCompSize != 0)
			out->pixelSize = (yuvCompSize * 2)+padBits;
	}
}

#ifdef OMFI_SELF_TEST

#define XFER_BUF_SIZE 10L*1024L

#include <stdlib.h>

/************************
 * name
 *
 * 		WhatIt(Internal)Does
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
void testSampleConversion(void)
{
	short				sampleSize, n;
	short				bytesUsed, extraBits;
	int 				multiplier = 1L, divisor = 1L, extraMask;
	unsigned int 		numSamples;
	char				*stdWriteBuf, *stdReadBuf;
	omfCStrmSwab_t		src, dest;
	omfAudioMemInfo_t	info;
	
	stdWriteBuf = (char *)omOptMalloc(NULL, XFER_BUF_SIZE);
	for(n = 0; n < XFER_BUF_SIZE; n++)
		stdWriteBuf[n] = n & 0xff;
	stdReadBuf = (char *)omOptMalloc(NULL, XFER_BUF_SIZE);
	if(stdReadBuf == NULL || stdWriteBuf == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}
		
	printf("Sample convertor test\n");
	for(sampleSize = 1; sampleSize <= 32; sampleSize++)
	{
		if((sampleSize % 4) != 0)
			continue;

		for(n = 0; n < XFER_BUF_SIZE; n++)
			stdReadBuf[n] = 0;

		info.format = kOmfSignedMagnitude;
		info.sampleSize = sampleSize;
		info.sampleRate.numerator = 44100;
		info.sampleRate.denominator = 1;
		info.numChannels = 1;

		src.buf = stdWriteBuf;
		src.buflen = XFER_BUF_SIZE;
		dest.buf = stdReadBuf;
		dest.buflen = XFER_BUF_SIZE;
		translateAudioSamples(NULL, &src, &info, &dest, &info, (OMNativeByteOrder == MOTOROLA_ORDER));
				
		numSamples = (XFER_BUF_SIZE * 8) / sampleSize;
		bytesUsed = (numSamples * sampleSize) / 8;

		/* Compare main buffer (not including fractional sample) */
		for(n = 0; n < bytesUsed; n++)
		{
			if(stdReadBuf[n] != stdWriteBuf[n])
			{
				printf("****Failed compare of byte %d at sample size: %d\n", n, sampleSize);
				exit(1);
			}
		}
		
		/* Compare the fractional sample */
		extraBits = (numSamples * sampleSize) % 8;
		if(extraBits != 0)
		{
			extraMask = ~((1L << (8-extraBits)) - 1L);
			if((stdReadBuf[bytesUsed] & extraMask) != (stdWriteBuf[bytesUsed] & extraMask))
			{
				printf("****Failed compare of fractional byte at sample size: %d\n", sampleSize);
				exit(1);
			}
		}
	}
	printf("Passed all sample size comversion tests!\n");
}
#endif

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
