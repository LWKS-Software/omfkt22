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

#ifndef _OMF_CODEC_SPT_
#define _OMF_CODEC_SPT_ 1

#if OMFI_CODEC_DIAGNOSTICS
#include <stdio.h>
#endif

#include "omFile.h"
#include "omCodCmn.h"
#include "omcStrm.h" 

#if PORT_LANG_CPLUSPLUS
extern          "C"
{
#endif

/* values for omfi:LayoutType enumeration*/
#define OMFI_DIDD_LAYOUT_FULL 0
#define OMFI_DIDD_LAYOUT_SEPARATE 1
#define OMFI_DIDD_LAYOUT_SINGLE 2
#define OMFI_DIDD_LAYOUT_MIXED 3

/************************************************************
 *
 * omfMediaHandle and related types
 *
 * These are opaque to users of the toolkit, but not to codec developers.
 *
 *************************************************************/

typedef enum
{
	kOmfiOpened, kOmfiCreated, kOmfiAppended, kOmfiNone
}               omfMediaOpenType_t;
	
typedef struct
{
	omfPosition_t		dataOffset;
	omfDDefObj_t		mediaKind;
	omfInt32		trackID;
	omfInt16		physicalOutChan;	/* 1->N */
	omfRational_t	sampleRate;
	omfLength_t		numSamples;
}               omfSubChannel_t;

	
#define MEDIA_COOKIE	0x4D444941	/* 'MDIA' */



/************************************************************
 *
 * Toolkit codec types and functions to call codecs.  These types and
 * functions are toolkit internals
 *
 *************************************************************/

OMF_EXPORT void     ConvertToIeeeExtended(double, char *);
OMF_EXPORT double   ConvertFromIeeeExtended(char *bytes);
OMF_EXPORT omfErr_t codecMgrInit(omfSessionHdl_t sess);
	
#if OMFI_CODEC_DIAGNOSTICS
OMF_EXPORT omfErr_t		codecRunDiagnostics(omfMediaHdl_t media, FILE *logFile);
#endif

typedef struct omfiMediaPrivate omfMediaPvt_t;

struct omfiMedia
{
	omfObject_t     masterMob;
	omfTrackID_t	masterTrackID;
	omfObject_t     fileMob;
	omfObject_t     mdes;
	omfObject_t     dataObj;
	void       		*rawFile;		/* If non-omfi file */
	void         	*userData;
	omfInt16		numChannels;
	omfSubChannel_t *channels;
	omfInt16	physicalOutChanOpen;
	omfPosition_t	dataStart;
	char           	*codecVariety;
	omfCompressEnable_t compEnable;
	omfHdl_t        dataFile;
	omfHdl_t        mainFile;
	omfInt32           cookie;
	omfMediaOpenType_t openType;

	/* */
	omfDDefObj_t		nilKind;
	omfDDefObj_t		pictureKind;
	omfDDefObj_t		soundKind;
		
	/* */
	omfCodecStream_t *stream;	
	omfMediaPvt_t	*pvt;
	};


OMF_EXPORT void *omfmGetRawFileDescriptor(omfHdl_t file);

#if PORT_LANG_CPLUSPLUS
}
#endif
#endif				/* _OMF_CODEC_API_ */

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/

