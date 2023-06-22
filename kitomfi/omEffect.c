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

/* Name: omEffect.c
 *
 * Overall Function: The building block functions to create and get
 *                   information about effects in a composition.
 *
 * Audience: Clients writing OMFI compositions with effects
 *
 * Public Functions:
 *      omfeVideoSpeedControlNew(), omfeVideoSpeedControlGetInfo()
 *      omfeVideoFrameMaskNew(), omfeVideoFrameMaskGetInfo()
 *      omfeVideoRepeatNew(), omfeVideoRepeatGetInfo()
 *      omfeVideoDissolveNew(), omfeVideoDissolveGetInfo()
 *      omfeMonoAudioDissolveNew(), omfeMonoAudioDissolveGetInfo()
 *      omfeStereoAudioDissolveNew(), omfeStereoAudioDissolveGetInfo()
 *      omfeMonoAudioGainNew(), omfeMonoAudioGainGetInfo()
 *      omfeStereoAudioGainNew(), omfeStereoAudioGainGetInfo()
 *      omfeMonoAudioPanNew(), omfeMonoAudioPanGetInfo()
 *      omfeVideoFadeToBlackNew(), omfeVideoFadeToBlackGetInfo()
 *      omfeMonoAudioMixdownNew(), omfeMonoAudioMixdownGetInfo()
 *      omfeSMPTEVideoWipeNew(), omfeSMPTEVideoWipeGetInfo()
 *      
 * General error codes returned:
 *      OM_ERR_NONE
 *             Success
 *      OM_ERR_NULLOBJECT
 *              Null object passed in as an argument
 *      OM_ERR_INVALID_EFFECTDEF
 *              An invalid effectdef was specified
 *      OM_ERR_BAD_LENGTH
 *              Segment has an illegal length
 *      OM_ERR_INTERN_TOO_SMALL
 *              Buffer size is too small
 *      OM_ERR_INVALID_EFFECTARG
 *              Effect argument is invalid (wrong datakind, etc.)
 *
 */

#include "masterhd.h"
#include <stdlib.h>

#include "omPublic.h"
#include "omPvt.h" 
#include "omEffect.h" 

#define NOT_TIMEWARP 0
#define IS_TIMEWARP 1

static omfErr_t LookupCreateDatakind(	
									 const omfHdl_t file,
									 const omfUniqueNamePtr_t name,
									 omfDDefObj_t *defObject);
							  		  
static void ExtractValueData(
							 const omfHdl_t	file,
							 void		*destination,
							 const void		*source,
							 const omfInt32		size);
							  		
static omfErr_t GetConstUint32(	
							   const omfHdl_t		file,
							   const omfSegObj_t	cval,
							   const omfUInt32		*value);


static omfErr_t GetConstInt32(	
							  const omfHdl_t		file,
							  const omfSegObj_t	cval,
							  const omfInt32			*value);


static omfErr_t GetConstBoolean(
								const omfHdl_t		file,
								const omfSegObj_t	cval,
								const omfBool		*value);


static omfErr_t GetConstRational(
								 const omfHdl_t		file,
								 const omfSegObj_t	cval,
								 const omfRational_t	*value);

/* reverse: reverse string s in place */
static void reverse(char s[]);

/* itoaLocal: Convert n to characters in s */
static void itoaLocal(omfInt32 n, omfInt32 bufLen, char buf[]);

/*************************************************************************
 * Function: omfeNewVideoSpeedControl()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:VideoSpeedControl.  If the effect definition object (EDEF)
 *      does not already exist, it will be created, and registered with the
 *      file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      The VideoSpeedControl timewarp effect changes the speed at which
 *      the media is to be played by duplicating or eliminating edit units
 *      by using a constant ratio.
 *
 *      Effect Inputs:
 *          ArgID -1: The input video segment.  This control is required.
 *          ArgID 1: Edit ratio. (CVAL) Defines the ratio of output length to
 *                   input length.  Range is 0 to +infinity.  This control
 *                   is required.
 *          ArgID 2: Phase Offset. (CVAL) Specifies that the first edit unit of
 *                   the output is actually offset from the theoritical 
 *                   output.  This control is optional, the default value
 *                   is constant 0.
 *
 *      The input segment and resulting datakind must be 
 *      omfi:data:PictureWithMatte.
 *
 *      This is a single-video-input effect, and is not legal as a
 *      transition effect.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *      Pass a 0 for phaseOffset, to specify the default value.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeVideoSpeedControlNew(
    const omfHdl_t		file,          /* IN - File Handle */
    const omfLength_t	length,        /* IN - Effect Length */
	const omfSegObj_t	inputSegment,  /* IN - Input Segment */
	const omfRational_t	speedRatio,    /* IN - Speed Ratio */
	const omfUInt32		phaseOffset,   /* IN - Phase Offset */
    omfEffObj_t 	    *newEffect)    /* OUT - New effect object */
{
	omfDDefObj_t	uint32Datakind = NULL;
	omfDDefObj_t	rationalDatakind = NULL;
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t    inputDatakind = NULL;
	omfDDefObj_t    pictDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfSegObj_t		speedRatioCVAL = NULL;
	omfSegObj_t		phaseOffsetCVAL = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfObject_t     tmpEffect = NULL;
	omfObject_t     tmpTrack = NULL;
	omfEDefObj_t	effectDef = NULL;
	
	omfAssertValidFHdl(file);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((inputSegment != NULL), file, OM_ERR_NULLOBJECT);
	*newEffect = NULL;
	
    XPROTECT(file)
	  {
		/* Verify that inputSegment is convertible to datakind 
		 * PictureWithMatte which is the arg type for VideoFrameMask
		 */
		if (file->semanticCheckEnable)
		  {
			CHECK(omfiComponentGetInfo(file, inputSegment, &inputDatakind,
									NULL));
			if (!omfiIsPictureWithMatteKind(file, inputDatakind, kConvertTo, 
											&omfErr))
			  {
				RAISE(OM_ERR_INVALID_EFFECTARG);
			  }
		  }

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(LookupCreateDatakind(file, PICTUREKIND, &pictDatakind));

			CHECK(omfsObjectNew(file, "SPED", &tmpEffect));
			CHECK(ComponentSetNewProps(file, tmpEffect, length, pictDatakind));
			CHECK(omfsWriteInt16(file, tmpEffect, OMWARPPhaseOffset,
								 (omfInt16)phaseOffset));
			CHECK(omfsWriteInt32(file, tmpEffect, OMSPEDNumerator, 
								speedRatio.numerator));
			CHECK(omfsWriteInt32(file, tmpEffect, OMSPEDDenominator,
								speedRatio.denominator));                       // FIX NPW 

			CHECK(omfsObjectNew(file, "TRAK", &tmpTrack));
			CHECK(omfsWriteInt16(file, tmpTrack, OMTRAKLabelNumber, 1));
			CHECK(omfsWriteObjRef(file, tmpTrack, OMTRAKTrackComponent,
								  inputSegment));
			CHECK(omfsAppendObjRefArray(file, tmpEffect, OMTRKGTracks,
										tmpTrack));
		  }

		else /* kOmfRev2x */
		  {
			CHECK(LookupCreateDatakind(file, UINT32KIND, &uint32Datakind));
			CHECK(LookupCreateDatakind(file, RATIONALKIND, &rationalDatakind));
			CHECK(LookupCreateDatakind(file, PICTWITHMATTEKIND, 
									   &outputDatakind));
					
			omfiEffectDefLookup(file, kOmfFxSpeedControlGlobalName,
								&effectDef, &omfErr);

			if (effectDef == NULL && omfErr == OM_ERR_NONE)
			  {
				CHECK(omfiEffectDefNew(	file, 
									   kOmfFxSpeedControlGlobalName,
									   kOmfFxSpeedControlName,
									   kOmfFxSpeedControlDescription,
									   0, IS_TIMEWARP, &effectDef));
			  }
			else
			  {
				CHECK(omfErr);
			  }
			
			CHECK(omfiEffectNew(file, outputDatakind, length, effectDef,
								&tmpEffect));
			
			CHECK(omfiEffectAddNewSlot(file, tmpEffect, kOmfFxIDGlobalATrack,
									   inputSegment, &effectSlot));
			
			CHECK(omfiConstValueNew(file, rationalDatakind, length,
									(omfInt32)sizeof(omfRational_t),
									(void *)&speedRatio, &speedRatioCVAL));
			CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
									   kOmfFxIDSpeedControlRatio,
									   speedRatioCVAL, &effectSlot));

			CHECK(omfiConstValueNew(file, uint32Datakind, length,
									(omfInt32)sizeof(omfUInt32),
									(void *)&phaseOffset, &phaseOffsetCVAL));
			CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
									   kOmfFxIDSpeedControlPhaseOffset,
									   phaseOffsetCVAL, &effectSlot));
		  } /* OmfRev2x */
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (tmpEffect)
		  omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeVideoSpeedControlGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:VideoSpeedControl effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeVideoSpeedControlGetInfo(
    const omfHdl_t		file,         /* IN - File Handle */
    const omfEffObj_t 	effect,       /* IN - Effect object */
    omfLength_t         *length,      /* OUT - Length of effect */
	omfSegObj_t			*inputSegment, /* OUT - Input segment */
	omfRational_t		*speedRatio,   /* OUT - Speed Ratio */
	omfUInt32		 	*phaseOffset)  /* OUT - Phase Offset */
{
	omfBool			isAdd = FALSE;
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfSegObj_t		ratioCVAL = NULL;
	omfSegObj_t		phaseOffsetCVAL = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfNumSlots_t	numEffectSlots = 0;
	omfObject_t     tmpTrack = NULL;
	omfLength_t		effectLength;

	/* Initialize outputs */	
	*inputSegment = NULL;
	*phaseOffset = 0;
	MakeRational(0,1, speedRatio);
	omfsCvtInt32toLength(0, effectLength);
	
	omfAssertValidFHdl(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	
	XPROTECT(file)
	{
	  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		{
		  if (length)
			{
			  CHECK(omfiComponentGetInfo(file, effect, &effectDatakind, length));
			}
		  if (inputSegment)
			{
			  CHECK(omfsGetNthObjRefArray(file, effect, OMTRKGTracks, 
										  &tmpTrack, 1));
			  CHECK(omfsReadObjRef(file, tmpTrack, OMTRAKTrackComponent, 
								   inputSegment));
			}
		  if (phaseOffset)
			{
			  CHECK(omfsReadInt16(file, effect, OMWARPPhaseOffset, 
								  (omfInt16 *)phaseOffset));
			}
		  if (speedRatio)
			{
			  CHECK(omfsReadInt32(file, effect, OMSPEDNumerator, 
								 &((*speedRatio).numerator)));
			  CHECK(omfsReadInt32(file, effect, OMSPEDDenominator,
								 &((*speedRatio).denominator)));
			}
		}
	  else /* kOmfRev2x */
		{
		  if (! omfiEffectDefLookup(file, kOmfFxSpeedControlGlobalName,
									&expectedEffectDef, &omfErr))
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		  /*
		   *  The appropriate effect definition object MUST be in the file
		   *  AND MUST equal the definition object that is on the effect.  
		   *  If not, then the effect is invalid.
		   */
		  
		  CHECK(omfErr);
		  
		  CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
								  &effectLength, &effectDef));
								
		  if (effectDef != expectedEffectDef)
			RAISE(OM_ERR_INVALID_EFFECTDEF);
		  
		  /* get length */
		  if (length)
			*length = effectLength;
					
		  /* get input segment */
		  if (inputSegment)
			{
			  CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalATrack,
									&effectSlot, inputSegment));
			  if (effectSlot == NULL)
				  *inputSegment = NULL;
			}
		
		  /* get speedRatio */
		  if (speedRatio)
			{
			  CHECK(FindSlotByArgID(file, effect, kOmfFxIDSpeedControlRatio,
									&effectSlot, &ratioCVAL));
			  if (effectSlot)
				{
				  CHECK(GetConstRational(file, ratioCVAL, speedRatio));
				}
			  else
				{
				  RAISE(OM_ERR_PROP_NOT_PRESENT);
				}	
			}
		
		  /* get phase offset */
		  if (phaseOffset)
			{
			  CHECK(FindSlotByArgID(file, effect, 
									kOmfFxIDSpeedControlPhaseOffset,
									&effectSlot, &phaseOffsetCVAL));
			  if (effectSlot)
				{
				  CHECK(GetConstUint32(file, phaseOffsetCVAL, phaseOffset));
				}
			  else
				{
				  phaseOffset = 0;
				}  
			}
		} /* kOmfRev2x */
	  } /* XPROTECT */

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeVideoFrameMaskNew()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:VideoFrameMask.  If the effect definition object (EDEF)
 *      does not already exist, it will be created, and registered with the
 *      file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      The VideoFrameMask timewarp effect maps an input video segment to an
 *      output video segment according to a cyclical pattern.
 *
 *      Effect INputs:
 *          ArgID -1: Input video segment.  This control is required.
 *          ArgID 1: Mask bits (CVAL)
 *          ArgID 2: Phase offset, indicates where to start reading
 *                   the mask on the first cycle.  Range 0 to 31.
 *          ArgID 3: Add/Drop indicator. (CVAL)
 *
 *      The input video segment and resulting datakind must be 
 *      omfi:data:PictureWithMatte.
 *
 *      This is a single-video-input effect, and is not legal as a
 *      transition effect.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *      Phase offset is optional, and a 0 should be specified if not desired.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeVideoFrameMaskNew(
    const omfHdl_t				file,         /* IN - File Handle */
    const omfLength_t			length,       /* IN - Effect Length */
	const omfSegObj_t			inputSegment, /* IN - Input segment */
	const omfUInt32				mask,         /* IN - Mask */
	const omfFxFrameMaskDirection_t addOrDrop,    /* IN - Specify add or drop*/
	const omfUInt32		 		phaseOffset,  /* IN - Phase Offset */
    omfEffObj_t 				*newEffect)   /* OUT - New Effect object */
{
    omfBool		isAdd = FALSE;
	omfDDefObj_t	uint32Datakind = NULL;
	omfDDefObj_t	booleanDatakind = NULL;
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	inputDatakind = NULL;
	omfDDefObj_t	pictDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfSegObj_t		maskCVAL = NULL;
	omfSegObj_t		addOrDropCVAL = NULL;
	omfSegObj_t		phaseOffsetCVAL = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfObject_t     tmpEffect = NULL;
	omfObject_t     tmpTrack = NULL;
	omfEDefObj_t	effectDef = NULL;
	
	omfAssertValidFHdl(file);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((inputSegment != NULL), file, OM_ERR_NULLOBJECT);
    omfAssert((omfsIsInt64Positive(length)), file, OM_ERR_BAD_LENGTH);
	*newEffect = NULL;
	
    XPROTECT(file)
	  {
		/* Verify that inputSegment is convertible to datakind 
		 * PictureWithMatte which is the arg type for VideoSpeedControl
		 */
		if (file->semanticCheckEnable)
		  {
			CHECK(omfiComponentGetInfo(file, inputSegment, &inputDatakind, NULL));
			if (!omfiIsPictureWithMatteKind(file, inputDatakind, kConvertTo, 
											&omfErr))
			  {
				RAISE(OM_ERR_INVALID_EFFECTARG);
			  }
		  }

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(LookupCreateDatakind(file, PICTUREKIND, &pictDatakind));

			CHECK(omfsObjectNew(file, "MASK", &tmpEffect));
			CHECK(ComponentSetNewProps(file, tmpEffect, length, 
									   pictDatakind));
			CHECK(omfsWriteInt16(file, tmpEffect, OMWARPPhaseOffset,
								 (omfInt16)phaseOffset));
			CHECK(omfsWriteUInt32(file, tmpEffect, OMMASKMaskBits, mask));
			if (addOrDrop == kOmfFxFrameMaskDirAdd)
			  {
				CHECK(omfsWriteBoolean(file, tmpEffect, OMMASKIsDouble, 1));
			  }
			else if (addOrDrop == kOmfFxFrameMaskDirDrop)
			  {
				CHECK(omfsWriteBoolean(file, tmpEffect, OMMASKIsDouble, 0));
			  }
			CHECK(omfsObjectNew(file, "TRAK", &tmpTrack));
			CHECK(omfsWriteInt16(file, tmpTrack, OMTRAKLabelNumber, 1));
			CHECK(omfsWriteObjRef(file, tmpTrack, OMTRAKTrackComponent, 
								  inputSegment));
			CHECK(omfsAppendObjRefArray(file, tmpEffect, OMTRKGTracks, 
										tmpTrack));
		  }

		else /* kOmfRev2x */
		  {
			isAdd = (addOrDrop == kOmfFxFrameMaskDirAdd ? TRUE : FALSE);
	  	
			CHECK(LookupCreateDatakind(file, UINT32KIND, &uint32Datakind));
			CHECK(LookupCreateDatakind(file, BOOLEANKIND, &booleanDatakind));
			CHECK(LookupCreateDatakind(file, PICTWITHMATTEKIND, 
									   &outputDatakind));
							
			omfiEffectDefLookup(file, kOmfFxFrameMaskGlobalName, &effectDef, 
								&omfErr);

			if (effectDef == NULL && omfErr == OM_ERR_NONE)
			  {
				CHECK(omfiEffectDefNew(	file, 
									   kOmfFxFrameMaskGlobalName,
									   kOmfFxFrameMaskName,
									   kOmfFxFrameMaskDescription,
									   0, IS_TIMEWARP, &effectDef));
			  }
			else
			  {
				CHECK(omfErr);
			  }
		
			CHECK(omfiEffectNew(file, outputDatakind, length, effectDef, 
								&tmpEffect));
	  
			/* INPUT MEDIA */
			CHECK(omfiEffectAddNewSlot(	file, tmpEffect, kOmfFxIDGlobalATrack,
									   inputSegment, &effectSlot));

			/* MASK */
			CHECK(omfiConstValueNew(file, uint32Datakind, length, 
									(omfInt32)sizeof(omfUInt32),
									(void *)&mask, &maskCVAL));
	  							
			CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
									   kOmfFxIDFrameMaskMaskBits,
									   maskCVAL, &effectSlot));

			/* ADD/DROP */
			CHECK(omfiConstValueNew(file, booleanDatakind, length, 
									(omfInt32)sizeof(omfBool),
									(void *)&isAdd, &addOrDropCVAL));
	  							
			CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
									   kOmfFxIDFrameMaskAddOrDrop,
									   addOrDropCVAL, &effectSlot));

			/* Phase offset is optional.  A 0 is passed in for no phase
			 * offset.  A constant value is still created, even if the
			 * phase offset is 0.
			 */
			/* PHASE OFFSET */
			CHECK(omfiConstValueNew(file, uint32Datakind, length, 
									(omfInt32)sizeof(omfUInt32),
									(void *)&phaseOffset, &phaseOffsetCVAL));

			CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
									   kOmfFxIDFrameMaskPhaseOffset,
									   phaseOffsetCVAL, &effectSlot));
		  } /* omfRev2x */
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (tmpEffect)
		  omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeVideoFrameMaskGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:VideoFrameMask effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeVideoFrameMaskGetInfo(
    const omfHdl_t			file,          /* IN - File Handle */
    const omfEffObj_t 		effect,        /* IN - Effect object */
    omfLength_t             *length,      /* OUT - Length of effect */
	omfSegObj_t				*inputSegment, /* OUT - Input segment */
	omfUInt32				*mask,         /* OUT - Mask */
	omfFxFrameMaskDirection_t	*addOrDrop,    /* OUT - Add or drop */
	omfUInt32		 		*phaseOffset)  /* OUT - Phase Offset */
{
	omfBool			isAdd = FALSE;
	omfBool         drop = FALSE;
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfSegObj_t		maskCVAL = NULL;
	omfSegObj_t		addOrDropCVAL = NULL;
	omfSegObj_t		phaseOffsetCVAL = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfNumSlots_t	numEffectSlots = 0;
	omfLength_t		effectLength;
	omfObject_t     tmpTrack = NULL;

	/* Initialize outputs */	
	*inputSegment = NULL;
	*mask = 0;
	*phaseOffset = 0;
	*addOrDrop = kOmfFxFrameMaskDirNone;
	omfsCvtInt32toLength(0, effectLength);
	
	omfAssertValidFHdl(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	
	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			if (length)
			  {
				CHECK(omfiComponentGetInfo(file, effect, &effectDatakind, 
										length));
			  }

			if (inputSegment)
			  {
				CHECK(omfsGetNthObjRefArray(file, effect, OMTRKGTracks, 
											&tmpTrack, 1));
				CHECK(omfsReadObjRef(file, tmpTrack, OMTRAKTrackComponent,
									 inputSegment));
			  }

			if (phaseOffset)
			  {
				CHECK(omfsReadInt16(file, effect, OMWARPPhaseOffset, 
									(omfInt16 *)phaseOffset));
			  }

			if (mask)
			  {
				CHECK(omfsReadUInt32(file, effect, OMMASKMaskBits, mask));
			  }

			if (drop)
			  {
				CHECK(omfsReadBoolean(file, effect, OMMASKIsDouble, &drop));
				if (drop)
				  *addOrDrop = kOmfFxFrameMaskDirDrop;
				else
				  *addOrDrop = kOmfFxFrameMaskDirAdd;
			  }
		  }
		else /* kOmfRev2x */
		  {
			if (! omfiEffectDefLookup(file, kOmfFxFrameMaskGlobalName,
									  &expectedEffectDef, &omfErr))
			  RAISE(OM_ERR_INVALID_EFFECTDEF);

			/*
			 *  The appropriate effect definition object MUST be in the file
			 *  AND MUST equal the definition object that is on the effect.  
			 *  If not, then the effect is invalid.
			 */
		 
			CHECK(omfErr);
		
			CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
									&effectLength, &effectDef));
			
			if (effectDef != expectedEffectDef)
			  RAISE(OM_ERR_INVALID_EFFECTDEF);

			/* get length */
			if (length)
			  *length = effectLength;
					
			/* get input segment */
			if (inputSegment)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalATrack,
									  &effectSlot, inputSegment));
				if (effectSlot == NULL)
					*inputSegment = NULL;
			  }
		
			/* get mask */
			if (mask)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDFrameMaskMaskBits,
									  &effectSlot, &maskCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstUint32(file, maskCVAL, mask));
				  }
			  }
			
			/* get phase offset */
			if (phaseOffset)
			  {
				CHECK(FindSlotByArgID(file, effect, 
									  kOmfFxIDFrameMaskPhaseOffset,
									  &effectSlot, &phaseOffsetCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstUint32(file, phaseOffsetCVAL, phaseOffset));
				  }
			  }

			/* get direction */
			if (addOrDrop)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDFrameMaskAddOrDrop,
									  &effectSlot,  &addOrDropCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstBoolean(file, addOrDropCVAL, &isAdd));
					if (isAdd)
					  *addOrDrop = kOmfFxFrameMaskDirAdd;
					else
					  *addOrDrop = kOmfFxFrameMaskDirDrop;
				  }
			  }
		  } /* kOmfRev2x */
	  } /* XPROTECT */

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeVideoRepeatNew()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:VideoRepeat.  If the effect definition object (EDEF)
 *      does not already exist, it will be created, and registered with the
 *      file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      A VideoRepeat timewarp effect repeats a segment of video for a
 *      given amount of time.
 *
 *      Effect Inputs:
 *           ArgID -1: Input video segment.  This control is required.
 *           ArgID 1: Phase Offset. (CVAL)  This control is optional,
 *                    the default value is 0.  The range is 0 to the length
 *                    of the input segment.
 *
 *      The input segment and resulting datakind must be 
 *      omfi:data:PictureWithMatte.
 *
 *      This is a single-video-input effect, and is not legal as a transition 
 *      effect.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *      Pass a 0 for phaseOffset, to specify the default value.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeVideoRepeatNew(
    const omfHdl_t 		file,         /* IN - File Handle */
    const omfLength_t	length,       /* IN - Effect Length */
	const omfSegObj_t	inputSegment, /* IN - Input segment */
	const omfUInt32		phaseOffset,  /* IN - Phase Offset */
    omfEffObj_t		    *newEffect)   /* OUT - New Effect Object */
{
	omfDDefObj_t	uint32Datakind = NULL;
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	inputDatakind = NULL;
	omfDDefObj_t	pictDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfSegObj_t		phaseOffsetCVAL = NULL;
	omfObject_t     tmpEffect = NULL;
	omfObject_t     tmpTrack = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;


	omfAssertValidFHdl(file);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((inputSegment != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((omfsIsInt64Positive(length)), file, OM_ERR_BAD_LENGTH);
	*newEffect = NULL;
	
    XPROTECT(file)
	  {
		/* Verify that inputSegment is convertible to datakind 
		 * PictureWithMatte which is the arg type for VideoRepeat
		 */
		if (file->semanticCheckEnable)
		  {
			CHECK(omfiComponentGetInfo(file, inputSegment, &inputDatakind, NULL));
			if (!omfiIsPictureWithMatteKind(file, inputDatakind, kConvertTo, 
											&omfErr))
			  {
				RAISE(OM_ERR_INVALID_EFFECTARG);
			  }
		  }

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(LookupCreateDatakind(file, PICTUREKIND, &pictDatakind));

			CHECK(omfsObjectNew(file, "REPT", &tmpEffect));
			CHECK(ComponentSetNewProps(file, tmpEffect, length, 
									   pictDatakind));
			CHECK(omfsWriteInt16(file, tmpEffect, OMWARPPhaseOffset,
								 (omfInt16)phaseOffset));
	    
			CHECK(omfsObjectNew(file, "TRAK", &tmpTrack));
			CHECK(omfsWriteInt16(file, tmpTrack, OMTRAKLabelNumber, 1));
			CHECK(omfsWriteObjRef(file, tmpTrack, OMTRAKTrackComponent, 
								  inputSegment));
			CHECK(omfsAppendObjRefArray(file, tmpEffect, OMTRKGTracks, 
										tmpTrack));
		  }
		else /* kOmfRev2x */
		  {
			CHECK(LookupCreateDatakind(file, PICTWITHMATTEKIND, 
									   &outputDatakind));
			CHECK(LookupCreateDatakind(file, UINT32KIND, &uint32Datakind));

			omfiEffectDefLookup(file, kOmfFxRepeatGlobalName, &effectDef, 
								&omfErr);

			if (effectDef == NULL && omfErr == OM_ERR_NONE)
			  {
				CHECK(omfiEffectDefNew(	file, 
									   kOmfFxRepeatGlobalName,
									   kOmfFxRepeatName,
									   kOmfFxRepeatDescription,
									   0, IS_TIMEWARP, &effectDef));
			  }
			else
			  {
				CHECK(omfErr);
			  }
		
			CHECK(omfiEffectNew(file, outputDatakind, length, 
								effectDef, &tmpEffect));
			/* INPUT MEDIA */
			CHECK(omfiEffectAddNewSlot(	file, tmpEffect, kOmfFxIDGlobalATrack,
									   inputSegment, &effectSlot));

			/* PHASE OFFSET */
			CHECK(omfiConstValueNew(file, uint32Datakind, length, 
									(omfInt32)sizeof(omfUInt32),
									(void *)&phaseOffset, &phaseOffsetCVAL));

			CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
									   kOmfFxIDRepeatPhaseOffset,
									   phaseOffsetCVAL, &effectSlot));
		  } /* kOmf2x */
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (tmpEffect)
		  omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeVideoRepeatGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:VideoRepeat effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeVideoRepeatGetInfo(
    const omfHdl_t			file,          /* IN - File Handle */
    const omfEffObj_t 		effect,        /* IN - Effect Object */
    omfLength_t             *length,      /* OUT - Length of effect */
	omfSegObj_t				*inputSegment, /* OUT - Input segment */
	omfUInt32		 		*phaseOffset)  /* OUT - Phase Offset */
{
	omfBool			isAdd = FALSE;
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfSegObj_t		phaseOffsetCVAL = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfNumSlots_t	numEffectSlots = 0;
	omfObject_t     tmpTrack = NULL;
	omfLength_t		effectLength;

	/* Initialize outputs */	
	*inputSegment = NULL;
	*phaseOffset = 0;
	omfsCvtInt32toLength(0, effectLength);
	
	omfAssertValidFHdl(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	
	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			if (length)
			  {
				CHECK(omfiComponentGetInfo(file, effect, &effectDatakind, length));
			  }
			if (inputSegment)
			  {
				CHECK(omfsGetNthObjRefArray(file, effect, OMTRKGTracks, 
											&tmpTrack, 1));
				CHECK(omfsReadObjRef(file, tmpTrack, OMTRAKTrackComponent,
									 inputSegment));
			  }
			if (phaseOffset)
			  {
				CHECK(omfsReadInt16(file, effect, OMWARPPhaseOffset, 
									(omfInt16 *)phaseOffset));
			  }
		  }
		else /* kOmfRev2x */
		  {
			if (! omfiEffectDefLookup(file, kOmfFxRepeatGlobalName,
									  &expectedEffectDef, &omfErr))
			  RAISE(OM_ERR_INVALID_EFFECTDEF);

			/*
			 *  The appropriate effect definition object MUST be in the file
			 *  AND MUST equal the definition object that is on the effect.  
			 *  If not, then the effect is invalid.
			 */
		 
			CHECK(omfErr);
		
			CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
									&effectLength, &effectDef));
								
			if (effectDef != expectedEffectDef)
			  RAISE(OM_ERR_INVALID_EFFECTDEF);

			/* get length */
			if (length)
			  *length = effectLength;
					
			/* get input segment */
			if (inputSegment)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalATrack,
									  &effectSlot, inputSegment));
				if (effectSlot == NULL)
				  *inputSegment = NULL;
			  }
		
			/* get phase offset */
			if (phaseOffset)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDRepeatPhaseOffset,
									  &effectSlot, &phaseOffsetCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstUint32(file, phaseOffsetCVAL, phaseOffset));
				  }
			  }
		  } /* kOmfRev2x */
	  } /* XPROTECT */

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeVideoDissolveNew()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:VideoDissolve.  If the effect definition object (EDEF)
 *      does not already exist, it will be created, and registered with the
 *      file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      The VideoDissolve effect combines two streams using a simple
 *      linear equation.
 *     
 *      Effect Inputs:
 *          ArgID -1: Input video segment "A" (outgoing in transitions)
 *          ArgID -2: Input video segment "B" (incoming in transitions)
 *          ArgID -3: Level, equal to mix ratio of B/A.
 *
 *      The input segment datakinds must be omfi:data:PictureWithMatte
 *      or some datakind that is convertible.  The resulting datakind
 *      is omfi:data:PictureWithMatte.  The range for level is 0 to 1.
 *     
 *      This effect is intended primarily for video transitions, but can
 *      also be used outside of transitions.
 *
 *      This function only supports 1.x and 2.x files.  For 1.x, this
 *      function will ignore the inputSegmentA, inputSegmentB and level
 *      arguments and return a TRKG with the EffectID set to Blend:Dissolve.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeVideoDissolveNew(
    omfHdl_t 		file,         /* IN - File Handle */
    omfLength_t		length,       /* IN - Effect Length */
	omfSegObj_t		inputSegmentA,/* IN - Input segment A */
	omfSegObj_t		inputSegmentB,/* IN - Input segment B */
	omfSegObj_t		level,        /* IN - Level */
    omfEffObj_t		*newEffect)   /* OUT - New Effect Object */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	inputDatakindA = NULL;
	omfDDefObj_t	inputDatakindB = NULL;
	omfDDefObj_t	levelDatakind = NULL;
	omfDDefObj_t	pictDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfObject_t	    tmpEffect = NULL;
	omfObject_t     tmpTrack = NULL;
	omfRational_t   editRate;

	omfAssertValidFHdl(file);
    omfAssert((omfsIsInt64Positive(length)), file, OM_ERR_BAD_LENGTH);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
	*newEffect = NULL;
	
    XPROTECT(file)
	  {
		/* Verify that inputSegmentA and inputSegmentB are convertible to 
		 * datakind PictWithMatte which are the arg types of VideoDissolve
		 * effect.
		 */
		if (file->semanticCheckEnable)
		  {
			if (inputSegmentA)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegmentA, 
											&inputDatakindA, NULL));
				if (!omfiIsPictureWithMatteKind(file, inputDatakindA, 
												kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (inputSegmentB)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegmentB, 
											&inputDatakindB, NULL));
				if (!omfiIsPictureWithMatteKind(file, inputDatakindB, 
												kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (level)
			  {
				CHECK(omfiComponentGetInfo(file, level, &levelDatakind, NULL));
				if (!omfiIsRationalKind(file, levelDatakind, kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
		  } /* semanticCheckEnable */

		/* For 1.x, create a TRKG object with an EffectID of Blend:Dissolve. */
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(LookupCreateDatakind(file, PICTUREKIND, &pictDatakind));
			CHECK(omfsObjectNew(file, "TRKG", &tmpEffect));
			/* Set the editrate to NTSC by default */
			editRate.numerator = 2997;
			editRate.denominator = 100;
			CHECK(omfsWriteExactEditRate(file, tmpEffect, OMCPNTEditRate,
										 editRate));
			CHECK(omfsObjectNew(file, "TRAK", &tmpTrack));
			CHECK(omfsWriteInt16(file, tmpTrack, OMTRAKLabelNumber, 1));
			CHECK(omfsAppendObjRefArray(file, tmpEffect, OMTRKGTracks,
										tmpTrack));

			CHECK(ComponentSetNewProps(file, tmpEffect, length, 
									   pictDatakind));

			/* For 1.x, level on a TRAN Blend:Dissolve is assumed to 
			 * be 0->100.  So, ignore the level argument.
			 */

			/* For 1.x, the tracks in the TRAN are assumed to be 
			 * the incoming and outgoing clips.  So, ignore the 
			 * inputSegmentA and inputSegmentB arguments.
			 */

			CHECK(omfsWriteString(file, tmpEffect, OMCPNTEffectID,
								  "Blend:Dissolve"));
		  }
		else /* kOmfRev2x */
		  {
			CHECK(LookupCreateDatakind(file, PICTWITHMATTEKIND, 
									   &outputDatakind));

			omfiEffectDefLookup(file, kOmfFxSVDissolveGlobalName, &effectDef, 
								&omfErr);

			if (effectDef == NULL && omfErr == OM_ERR_NONE)
			  {
				CHECK(omfiEffectDefNew(	file, 
									   kOmfFxSVDissolveGlobalName,
									   kOmfFxVDissolveName,
									   kOmfFxSVDissolveDescription,
									   0, NOT_TIMEWARP, &effectDef));
			  }
			else
			  {
				CHECK(omfErr);
			  }
		
			CHECK(omfiEffectNew(file, outputDatakind, length, effectDef, 
								&tmpEffect));
		
			/* INPUT MEDIA A */
			if (inputSegmentA)
			  CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
										 kOmfFxIDGlobalATrack,
										 inputSegmentA, &effectSlot));
			
			/* INPUT MEDIA B */
			if (inputSegmentB)
			  CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
										 kOmfFxIDGlobalBTrack,
										 inputSegmentB, &effectSlot));

			/* Level */
			if (level)
			  CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
										 kOmfFxIDGlobalLevel,
										 level, &effectSlot));
		  } /* kOmfRev2x */
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (tmpEffect)
		  omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeVideoDissolveGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:VideoDissolve effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeVideoDissolveGetInfo(
    const omfHdl_t			file,          /* IN - File Handle */
    const omfEffObj_t 		effect,        /* IN - Effect Object */
    omfLength_t             *length,       /* OUT - Length of effect */
	omfSegObj_t				*inputSegmentA,/* OUT - Input Segment A */
	omfSegObj_t				*inputSegmentB,/* OUT - Input Segment B */
	omfSegObj_t				*level)        /* OUT - Level */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfLength_t		effectLength;

	/* Initialize outputs */	
	
	omfAssertValidFHdl(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfsCvtInt32toLength(0, effectLength);
	
	XPROTECT(file)
	{
	    /* For 1.x, return NULL for level, inputSegmentA and inputSegmentB */
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			if (length)
			  {
				CHECK(omfiComponentGetInfo(file, effect, &effectDatakind, length));
			  }
			if (level)
			  *level = NULL;
			if (inputSegmentA)
			  *inputSegmentA = NULL;
			if (inputSegmentB)
			  *inputSegmentB = NULL;
		  }
		else /* kOmfRev2x */
		  {
			if (! omfiEffectDefLookup(file, kOmfFxSVDissolveGlobalName,
									  &expectedEffectDef, &omfErr)
				&& ! omfiEffectDefLookup(file, kOmfFxSVMCDissolveGlobalName,
									  &expectedEffectDef, &omfErr))
			  RAISE(OM_ERR_INVALID_EFFECTDEF);

			/*
			 *  The appropriate effect definition object MUST be in the file
			 *  AND MUST equal the definition object that is on the effect.  
			 *  If not, then the effect is invalid.
			 */
			CHECK(omfErr);
			
			CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
									&effectLength, &effectDef));
								
			if (effectDef != expectedEffectDef)
			  RAISE(OM_ERR_INVALID_EFFECTDEF);
			
			/* get length */
			if (length)
			  *length = effectLength;
					
			/* get input segment */
			if (inputSegmentA)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalATrack,
										   &effectSlot, inputSegmentA));
				if (effectSlot == NULL)
					*inputSegmentA = NULL;
			  }
		
			if (inputSegmentB)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalBTrack,
										   &effectSlot, inputSegmentB));
				if (effectSlot == NULL)
					*inputSegmentB = NULL;
			  }

			if (level)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalLevel,
										   &effectSlot, level));
				if (effectSlot == NULL)
				  *level = NULL;
			  }
		  } /* kOmfRev2x */
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeMonoAudioDissolveNew()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:MonoAudioDissolve.  If the effect definition object (EDEF)
 *      does not already exist, it will be created, and registered with the
 *      file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      The MonoAudioDissolve effect combines 2 mono audio streams.  It
 *      behaves like a symmetric linear crossfade.
 *
 *      Effect Inputs:
 *          ArgID -1: Input audio segment "A" (outgoing for transitions)
 *          ArgID -2: Input audio segment "B" (incoming for transitions)
 *          ArgID -3: Level, equal to mix ratio of B/A. 
 *
 *      The input segment datakinds must be omfi:data:Sound.  The
 *      resulting datakind is omfi:data:Sound.  The range for level is
 *      0 to 1.
 *
 *      This effect is intended primarily for audio transitions, but it
 *      can be used ou;ide of transitions also.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeMonoAudioDissolveNew(
    omfHdl_t 		file,          /* IN - File Handle */
    omfLength_t		length,        /* IN - Effect Length */
	omfSegObj_t		inputSegmentA, /* IN - Input Audio Segment A */
	omfSegObj_t		inputSegmentB, /* IN - Input Audio Segment B */
	omfSegObj_t		level,         /* IN - Level */
    omfEffObj_t		*newEffect)    /* OUT - New Effect Object */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	inputDatakindA = NULL;
	omfDDefObj_t	inputDatakindB = NULL;
	omfDDefObj_t	levelDatakind = NULL;
	omfDDefObj_t	soundDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfObject_t	    tmpEffect = NULL;
	omfObject_t     tmpTrack = NULL;
	omfRational_t   editRate;

	omfAssertValidFHdl(file);
    omfAssert((omfsIsInt64Positive(length)), file, OM_ERR_BAD_LENGTH);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
	*newEffect = NULL;
	
    XPROTECT(file)
	  {
		/* Verify that inputSegmentA and inputSegmentB are convertible to 
		 * datakind Sound which are the arg types of MonoAudioDissolve effect.
		 */
		if (file->semanticCheckEnable)
		  {
			if (inputSegmentA)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegmentA, 
											&inputDatakindA, NULL));
				if (!omfiIsSoundKind(file, inputDatakindA, kConvertTo, 
									 &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (inputSegmentB)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegmentB, 
											&inputDatakindB, NULL));
				if (!omfiIsSoundKind(file, inputDatakindB, 
									 kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (level)
			  {
				CHECK(omfiComponentGetInfo(file, level, &levelDatakind, NULL));
				if (!omfiIsRationalKind(file, levelDatakind, kConvertTo, 
										&omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
		  } /* semanticCheckEnable */

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(LookupCreateDatakind(file, SOUNDKIND, &soundDatakind));

			CHECK(omfsObjectNew(file, "TRKG", &tmpEffect));
			/* Set the editrate to NTSC by default */
			editRate.numerator = 2997;
			editRate.denominator = 100;
			CHECK(omfsWriteExactEditRate(file, tmpEffect, OMCPNTEditRate,
										 editRate));
			CHECK(omfsObjectNew(file, "TRAK", &tmpTrack));
			CHECK(omfsWriteInt16(file, tmpTrack, OMTRAKLabelNumber, 1));
			CHECK(omfsAppendObjRefArray(file, tmpEffect, OMTRKGTracks,
										tmpTrack));
			CHECK(ComponentSetNewProps(file, tmpEffect, length,soundDatakind));

			/* For 1.x, level on a TRAN Blend:Dissolve is assumed to 
			 * be 0->100.  So, ignore the level argument.
			 */

			/* For 1.x, the tracks in the TRAN are assumed to be 
			 * the incoming and outgoing clips.  So, ignore the 
			 * inputSegmentA and inputSegmentB arguments.
			 */

			CHECK(omfsWriteString(file, tmpEffect, OMCPNTEffectID,
								  "Blend:Dissolve"));
		  }
		else /* kOmfRev2x */
		  {
			CHECK(LookupCreateDatakind(file, SOUNDKIND, &outputDatakind));

			omfiEffectDefLookup(file, kOmfFxSMADissolveGlobalName, &effectDef, 
								&omfErr);

			if (effectDef == NULL && omfErr == OM_ERR_NONE)
			  {
				CHECK(omfiEffectDefNew(	file, 
									   kOmfFxSMADissolveGlobalName,
									   kOmfFxSMADissolveName,
									   kOmfFxSMADissolveDescription,
									   0, NOT_TIMEWARP, &effectDef));
			  }
			else
			  {
				CHECK(omfErr);
			  }
		
			CHECK(omfiEffectNew(file, outputDatakind, length, 
								effectDef, &tmpEffect));
		
			/* INPUT MEDIA A */
			if (inputSegmentA)
			  CHECK(omfiEffectAddNewSlot(file, tmpEffect, kOmfFxIDGlobalATrack,
										 inputSegmentA, &effectSlot));

			/* INPUT MEDIA B */
			if (inputSegmentB)
			  CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
										 kOmfFxIDGlobalBTrack,
										 inputSegmentB, &effectSlot));

			/* Level */
			if (level)
			  CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
										 kOmfFxIDGlobalLevel,
										 level, &effectSlot));
		  }
	  } /* kOmfRev2x */

	XEXCEPT
	  {
		if (tmpEffect)
		  omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeMonoAudioDissolveGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:MonoAudioDissolve effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeMonoAudioDissolveGetInfo(
    const omfHdl_t			file,           /* IN - File Handle */
    const omfEffObj_t 		effect,         /* IN - Effect Object */
    omfLength_t             *length,        /* OUT - Length of effect */
	omfSegObj_t				*inputSegmentA, /* OUT - Input Audio Segment A */
	omfSegObj_t				*inputSegmentB, /* OUT - Input Audio Segment B */
	omfSegObj_t				*level)         /* OUT - Level */
{
	omfBool			isAdd = FALSE;
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfSegObj_t		phaseOffsetCVAL = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfObject_t		head = NULL;
	omfLength_t		effectLength;

	/* Initialize outputs */	
	
	omfAssertValidFHdl(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfsCvtInt32toLength(0, effectLength);
	
	
	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			if (length)
			  {
				CHECK(omfiComponentGetInfo(file, effect, &effectDatakind, length));
			  }
			if (level)
			  *level = NULL;
			if (inputSegmentA)
			  *inputSegmentA = NULL;
			if (inputSegmentB)
			  *inputSegmentB = NULL;
		  }
		else /* kOmfRev2x */
		  {
			if (! omfiEffectDefLookup(file, kOmfFxSMADissolveGlobalName,
									  &expectedEffectDef, &omfErr)
			 && ! omfiEffectDefLookup(file, kOmfFxSMAMCDissolveGlobalName,
									  &expectedEffectDef, &omfErr))
			  RAISE(OM_ERR_INVALID_EFFECTDEF);

			/*
			 *  The appropriate effect definition object MUST be in the file
			 *  AND MUST equal the definition object that is on the effect.  
			 *  If not, then the effect is invalid.
			 */
			CHECK(omfErr);
		
			CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
									&effectLength, &effectDef));
								
			if (effectDef != expectedEffectDef)
			  RAISE(OM_ERR_INVALID_EFFECTDEF);
			
			/* get length */
			if (length)
			  *length = effectLength;
					
			/* get input segment */
			if (inputSegmentA)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalATrack,
									  &effectSlot, inputSegmentA));
				if (effectSlot == NULL)
				  *inputSegmentA = NULL;
			  }
		
			if (inputSegmentB)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalBTrack,
									  &effectSlot, inputSegmentB));
				if (effectSlot == NULL)
				  *inputSegmentB = NULL;
			  }
		
			if (level)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalLevel,
									  &effectSlot, level));
				if (effectSlot == NULL)
				  *level = NULL;
			  }
		  } /* kOmfRev2x */
	  } /* XPROTECT */

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeStereoAudioDissolveNew()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:StereoAudioDissolve.  If the effect definition object
 *      (EDEF) does not already exist, it will be created, and registered with
 *      the file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      The StereoAudioDissolve effect combines 2 stereo audio streams.
 *
 *      Effect Inputs:
 *          ArgID -1: Input audio segment "A" (outgoing in transitions)
 *          ArgID -2: Input audio segment "B" (incoming in transitions)
 *          ArgID -3: Level, equal to mix ration of B/A
 *
 *      The input segment datakinds must be omfi:data:StereoSound.  The
 *      resulting datakind is omfi:data:StereoSound.  The range for level
 *      is 0 to 1.
 *
 *      This effect is intended primarily for audio transitions, but it
 *      can be used outside of transitions also.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeStereoAudioDissolveNew(
    omfHdl_t 		file,          /* IN - File Handle */
    omfLength_t		length,        /* IN - Effect Length */
	omfSegObj_t		inputSegmentA, /* IN - Input segment A */
	omfSegObj_t		inputSegmentB, /* IN - Input segment B */
	omfSegObj_t		level,         /* IN - Level */
    omfEffObj_t		*newEffect)    /* OUT - New Effect Object */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	inputDatakindA = NULL;
	omfDDefObj_t	inputDatakindB = NULL;
	omfDDefObj_t	levelDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfObject_t	    tmpEffect = NULL;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((omfsIsInt64Positive(length)), file, OM_ERR_BAD_LENGTH);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
	*newEffect = NULL;
	
    XPROTECT(file)
	  {
		/* Verify that inputSegmentA and inputSegmentB are convertible to 
		 * datakind StereoSound which are the arg types of StereoAudioDissolve
		 * effect.
		 */
		if (file->semanticCheckEnable)
		  {
			if (inputSegmentA)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegmentA, 
											&inputDatakindA, NULL));
				if (!omfiIsStereoSoundKind(file, inputDatakindA, kConvertTo, 
									 &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (inputSegmentB)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegmentB, 
											&inputDatakindB, NULL));
				if (!omfiIsStereoSoundKind(file, inputDatakindB, 
									 kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (level)
			  {
				CHECK(omfiComponentGetInfo(file, level, &levelDatakind, NULL));
				if (!omfiIsRationalKind(file, levelDatakind, kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
		  } /* semanticCheckEnable */

		CHECK(LookupCreateDatakind(file, STEREOKIND, &outputDatakind));

		omfiEffectDefLookup(file, kOmfFxSSADissolveGlobalName, &effectDef, 
							&omfErr);

		if (effectDef == NULL && omfErr == OM_ERR_NONE)
		{
			CHECK(omfiEffectDefNew(	file, 
									kOmfFxSSADissolveGlobalName,
									kOmfFxSSADissolveName,
									kOmfFxSSADissolveDescription,
									0, NOT_TIMEWARP, &effectDef));
		}
		else
		{
			CHECK(omfErr);
		}
		
	  	CHECK(omfiEffectNew(file, outputDatakind, length, 
	  						effectDef, &tmpEffect));
		
		/* INPUT MEDIA A */
		if (inputSegmentA)
	  		CHECK(omfiEffectAddNewSlot(	file, tmpEffect, kOmfFxIDGlobalATrack,
	  								inputSegmentA, &effectSlot));

		/* INPUT MEDIA B */
		if (inputSegmentB)
	  		CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
	  								kOmfFxIDGlobalBTrack,
	  								inputSegmentB, &effectSlot));

		/* Level */
		if (level)
	  		CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
	  								kOmfFxIDGlobalLevel,
	  								level, &effectSlot));
	  }

	XEXCEPT
	  {
		if (tmpEffect)
		  omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeStereoAudioDissolveGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:StereoAudioDissolve effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeStereoAudioDissolveGetInfo(
    const omfHdl_t			file,           /* IN - File Handle */
    const omfEffObj_t 		effect,         /* IN - Effect Object */
    omfLength_t             *length,        /* OUT - Length of effect */
	omfSegObj_t				*inputSegmentA, /* OUT - Input Segment A */
	omfSegObj_t				*inputSegmentB, /* OUT - Input Segment B */
	omfSegObj_t				*level)         /* OUT - Level */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfLength_t		effectLength;

	/* Initialize outputs */	

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfsCvtInt32toLength(0, effectLength);
	
	XPROTECT(file)
	{
		if (! omfiEffectDefLookup(file, kOmfFxSSADissolveGlobalName,
							 &expectedEffectDef, &omfErr)
		&& ! omfiEffectDefLookup(file, kOmfFxSSAMCDissolveGlobalName,
							 &expectedEffectDef, &omfErr))
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/*
		 *  The appropriate effect definition object MUST be in the file
		 *  AND MUST equal the definition object that is on the effect.  
		 *  If not, then the effect is invalid.
		 */
		 
		CHECK(omfErr);
		
		CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
								&effectLength, &effectDef));
								
		if (effectDef != expectedEffectDef)
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/* get length */
		if (length)
		  *length = effectLength;
					
		/* get input segment */
		if (inputSegmentA)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalATrack,
								  &effectSlot, inputSegmentA));
			if (effectSlot == NULL)
			  {
				*inputSegmentA = NULL;
			  }
		  }
		
		if (inputSegmentB)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalBTrack,
								  &effectSlot, inputSegmentB));
			if (effectSlot == NULL)
			  {
				*inputSegmentB = NULL;
			  }
		  }
		
		if (level)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalLevel,
								  &effectSlot, level));
			if (effectSlot == NULL)
			  {
				*level = NULL;
			  }
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeMonoAudioGainNew()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:MonoAudioGain.  If the effect definition object
 *      (EDEF) does not already exist, it will be created, and registered with
 *      the file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      The MonoAudioGain effect adjusts the volume of the input
 *      audio segment.  The input segment and result datakind is 
 *      omfi:data:Sound.  The multiplier argument specifies the amplitude
 *      multiplier, which can range from 0 to 2^32.  Unity is gain 1.
 *
 *      Effect Inputs:
 *          ArgID -1: Input audio segment
 *          ArgID 1: amplitude multiplier
 *
 *      This effect is not allowed in Transitions.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeMonoAudioGainNew(
    const omfHdl_t 			file,         /* IN - File Handle */
    const omfLength_t		length,       /* IN - Effect Length */
	const omfSegObj_t		inputSegment, /* IN - Input audio segment */
	const omfSegObj_t		multiplier,   /* IN - Amplitude Multiplier */
    omfEffObj_t		        *newEffect)   /* OUT - New Effect Object */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	inputDatakind = NULL;
	omfDDefObj_t	multDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfObject_t	    tmpEffect = NULL;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((omfsIsInt64Positive(length)), file, OM_ERR_BAD_LENGTH);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
  	*newEffect = NULL;

	
    XPROTECT(file)
	  {
		if (file->semanticCheckEnable)
		  {
			if (inputSegment)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegment, &inputDatakind,
											NULL));
				if (!omfiIsSoundKind(file, inputDatakind, kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (multiplier)
			  {
				CHECK(omfiComponentGetInfo(file, multiplier, &multDatakind,
											NULL));
				if (!omfiIsRationalKind(file, multDatakind, kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
		  }

		CHECK(LookupCreateDatakind(file, SOUNDKIND, &outputDatakind));

		omfiEffectDefLookup(file, kOmfFxMAGainGlobalName, &effectDef, 
							&omfErr);

		if (effectDef == NULL && omfErr == OM_ERR_NONE)
		{
			CHECK(omfiEffectDefNew(	file, 
									kOmfFxMAGainGlobalName,
									kOmfFxMAGainName,
									kOmfFxMAGainDescription,
									0, NOT_TIMEWARP, &effectDef));
		}
		else
		{
			CHECK(omfErr);
		}
		
	  	CHECK(omfiEffectNew(file, outputDatakind, length, 
	  						effectDef, &tmpEffect));
		
		/* INPUT MEDIA A */
		if (inputSegment)
	  		CHECK(omfiEffectAddNewSlot(	file, tmpEffect, kOmfFxIDGlobalATrack,
	  								inputSegment, &effectSlot));

		/* Gain */
		if (multiplier)
	  		CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
	  								kOmfFxIDMAGainMultiplier,
	  								multiplier, &effectSlot));
	  }

	XEXCEPT
	  {
		if (tmpEffect)
		  omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeMonoAudioGainGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:MonoAudioGain effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeMonoAudioGainGetInfo(
    const omfHdl_t			file,         /* IN - File Handle */
    const omfEffObj_t 		effect,       /* IN - Effect object */
    omfLength_t             *length,      /* OUT - Length of effect */
	omfSegObj_t				*inputSegment,/* OUT - Input audio segment */
	omfSegObj_t				*multiplier)  /* OUT - Amplitude Multiplier */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfLength_t		effectLength;

	/* Initialize outputs */	
	
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfsCvtInt32toLength(0, effectLength);
	
	
	XPROTECT(file)
	{
		if (! omfiEffectDefLookup(file, kOmfFxMAGainGlobalName,
							 &expectedEffectDef, &omfErr))
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/*
		 *  The appropriate effect definition object MUST be in the file
		 *  AND MUST equal the definition object that is on the effect.  
		 *  If not, then the effect is invalid.
		 */
		 
		CHECK(omfErr);
		
		CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
								&effectLength, &effectDef));
								
		if (effectDef != expectedEffectDef)
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/* get length */
		if (length)
		  *length = effectLength;
					
		/* get input segment */
		if (inputSegment)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalATrack,
								  &effectSlot, inputSegment));
			if (effectSlot == NULL)
			  {
				*inputSegment = NULL;
			  }
		  }
		
		if (multiplier)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxIDMAGainMultiplier,
								  &effectSlot, multiplier));
			if (effectSlot == NULL)
			  {
				*multiplier = NULL;
			  }
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeStereoAudioGainNew()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:StereoAudioGain.  If the effect definition object
 *      (EDEF) does not already exist, it will be created, and registered with
 *      the file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      The StereoAudioGain effect adjusts the volume of a stereo audio
 *      segment.  The adjustment is applied identically to both channels.
 *
 *      Effect Inputs:
 *          ArgID -1: Input audio segment
 *          ArgID 1: Amplitude multiplier
 *
 *      The datakind of the input segment must be omfi:data:StereoSound.
 *      The resulting datakind must be omfi:data:StereoSound.
 *
 *      This effect has only a single segment for input and is not
 *      allowed in transitions.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeStereoAudioGainNew(
    const omfHdl_t 			file,         /* IN - File Handle */
    const omfLength_t		length,       /* IN - Effect Length */
	const omfSegObj_t		inputSegment, /* IN - Input segment */
	const omfSegObj_t		multiplier,   /* IN - Multiplier */
    omfEffObj_t		        *newEffect)   /* OUT - New Effect Object */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	inputDatakind = NULL;
	omfDDefObj_t	multDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfObject_t	    tmpEffect = NULL;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((omfsIsInt64Positive(length)), file, OM_ERR_BAD_LENGTH);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
	*newEffect = NULL;
	
    XPROTECT(file)
	  {
		if (file->semanticCheckEnable)
		  {
			if (inputSegment)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegment, &inputDatakind,
											NULL));
				if (!omfiIsStereoSoundKind(file, inputDatakind, kConvertTo, 
										   &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (multiplier)
			  {
				CHECK(omfiComponentGetInfo(file, multiplier, &multDatakind,
											NULL));
				if (!omfiIsRationalKind(file, multDatakind, kConvertTo, 
										&omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
		  }

		CHECK(LookupCreateDatakind(file, STEREOKIND, &outputDatakind));

		omfiEffectDefLookup(file, kOmfFxSAGainGlobalName, &effectDef, 
							&omfErr);

		if (effectDef == NULL && omfErr == OM_ERR_NONE)
		{
			CHECK(omfiEffectDefNew(	file, 
									kOmfFxSAGainGlobalName,
									kOmfFxSAGainName,
									kOmfFxSAGainDescription,
									0, NOT_TIMEWARP, &effectDef));
		}
		else
		{
			CHECK(omfErr);
		}
		
	  	CHECK(omfiEffectNew(file, outputDatakind, length, 
	  						effectDef, &tmpEffect));
		
		/* INPUT MEDIA A */
		if (inputSegment)
	  		CHECK(omfiEffectAddNewSlot(	file, tmpEffect, kOmfFxIDGlobalATrack,
	  								inputSegment, &effectSlot));

		/* Gain */
		if (multiplier)
	  		CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
	  								kOmfFxIDSAGainMultiplier,
	  								multiplier, &effectSlot));
	  }

	XEXCEPT
	  {
		if (tmpEffect)
		  *newEffect = tmpEffect;
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeStereoAudioGainGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:StereoAudioGain effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeStereoAudioGainGetInfo(
    const omfHdl_t			file,         /* IN - File Handle */
    const omfEffObj_t 		effect,       /* IN - Effect Object */
    omfLength_t             *length,      /* OUT - Length of effect */
	omfSegObj_t				*inputSegment,/* OUT - Input segment */
	omfSegObj_t				*multiplier)  /* OUT - Multiplier */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfLength_t		effectLength;

	/* Initialize outputs */	
	
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfsCvtInt32toLength(0, effectLength);
	
	XPROTECT(file)
	{
		if (! omfiEffectDefLookup(file, kOmfFxSAGainGlobalName,
							 &expectedEffectDef, &omfErr))
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/*
		 *  The appropriate effect definition object MUST be in the file
		 *  AND MUST equal the definition object that is on the effect.  
		 *  If not, then the effect is invalid.
		 */
		 
		CHECK(omfErr);
		
		CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
								&effectLength, &effectDef));
								
		if (effectDef != expectedEffectDef)
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/* get length */
		if (length)
		  *length = effectLength;
					
		/* get input segment */
		if (inputSegment)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalATrack,
								  &effectSlot, inputSegment));
			if (effectSlot == NULL)
			  {
				*inputSegment = NULL;
			  }
		  }
		
		if (multiplier)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxIDMAGainMultiplier,
								  &effectSlot, multiplier));
			if (effectSlot == NULL)
			  {
				*multiplier = NULL;
			  }
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeMonoAudioPanNew()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:MonoAudioPan.  If the effect definition object
 *      (EDEF) does not already exist, it will be created, and registered with
 *      the file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      The AudioPan effect converts a mono audio segment into stereo audio.
 *      The amount of data sent to each channel is determined by the pan
 *      control.
 *
 *      Effect Inputs:
 *          ArgID -1: Input audio segment
 *          ArgID 1: Pan value
 *
 *      The input audio segment must be of datakind omfi:data:Sound.
 *      The result datakind is omfi:data:StereoSound.   The pan value has a
 *      range from 0.0 to 1.0.  This value represents the ratio of
 *      left/right apportionment of sound.  A value of 0 is full left; 
 *      a value of 1 is full right.  and a value of 1/2 is half left and
 *      half right.
 *
 *      This effect is not allowed in transitions.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeMonoAudioPanNew(
    const omfHdl_t 			file,         /* IN - File Handle */
    const omfLength_t		length,       /* IN  - Effect length */
	const omfSegObj_t		inputSegment, /* IN - Input segment */
	const omfSegObj_t		ratio,        /* IN - Ratio */
    	  omfEffObj_t		*newEffect)   /* OUT - New Effect Object */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	inputDatakind = NULL;
	omfDDefObj_t	ratioDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfObject_t   	tmpEffect = NULL;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((omfsIsInt64Positive(length)), file, OM_ERR_BAD_LENGTH);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
	*newEffect = NULL;
	
    XPROTECT(file)
	  {
		if (file->semanticCheckEnable)
		  {
			if (inputSegment)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegment, &inputDatakind,
											NULL));
				if (!omfiIsSoundKind(file, inputDatakind, kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (ratio)
			  {
				CHECK(omfiComponentGetInfo(file, ratio, &ratioDatakind,
											NULL));
				if (!omfiIsRationalKind(file, ratioDatakind, kConvertTo, 
										&omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
		  }

		CHECK(LookupCreateDatakind(file, SOUNDKIND, &outputDatakind));

		omfiEffectDefLookup(file, kOmfFxMAPanGlobalName, &effectDef, 
							&omfErr);

		if (effectDef == NULL && omfErr == OM_ERR_NONE)
		{
			CHECK(omfiEffectDefNew(	file, 
									kOmfFxMAPanGlobalName,
									kOmfFxMAPanName,
									kOmfFxMAPanDescription,
									0, NOT_TIMEWARP, &effectDef));
		}
		else
		{
			CHECK(omfErr);
		}
		
	  	CHECK(omfiEffectNew(file, outputDatakind, length, 
	  						effectDef, &tmpEffect));
		
		/* INPUT MEDIA A */
		if (inputSegment)
	  		CHECK(omfiEffectAddNewSlot(	file, tmpEffect, kOmfFxIDGlobalATrack,
	  								inputSegment, &effectSlot));

		/* Gain */
		if (ratio)
	  		CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
	  								kOmfFxIDMAPanRatio,
	  								ratio, &effectSlot));
	  }

	XEXCEPT
	  {
		if (tmpEffect)
		  omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeMonoAudioPanGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:MonoAudioPan effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeMonoAudioPanGetInfo(
    const omfHdl_t			file,         /* IN - File Handle */
    const omfEffObj_t 		effect,       /* IN - Effect object */
    omfLength_t             *length,      /* OUT - Length of effect */
	omfSegObj_t				*inputSegment,/* OUT - Input segment */
	omfSegObj_t				*ratio)       /* OUT - Ratio */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfLength_t		effectLength;

	/* Initialize outputs */	
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfsCvtInt32toLength(0, effectLength);
	
	XPROTECT(file)
	{
		if (! omfiEffectDefLookup(file, kOmfFxMAPanGlobalName,
							 &expectedEffectDef, &omfErr))
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/*
		 *  The appropriate effect definition object MUST be in the file
		 *  AND MUST equal the definition object that is on the effect.  
		 *  If not, then the effect is invalid.
		 */
		 
		CHECK(omfErr);
		
		CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
								&effectLength, &effectDef));
								
		if (effectDef != expectedEffectDef)
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/* get length */
		if (length)
		  *length = effectLength;
					
		/* get input segment */
		if (inputSegment)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalATrack,
								  &effectSlot, inputSegment));
			if (effectSlot == NULL)
			  {
				*inputSegment = NULL;
			  }
		  }

		if (ratio)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxIDMAPanRatio,
								  &effectSlot, ratio));
			if (effectSlot == NULL)
			  {
				*ratio = NULL;
			  }
		  }
				
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeVideoFadeToBlackNew()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:VideoFadeToBlack.  If the effect definition object
 *      (EDEF) does not already exist, it will be created, and registered with
 *      the file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      The VideoFadeToBlack effect combines a video stream with black
 *      using a simple linear equation.
 *
 *      Effect Inputs:
 *          ArgID -1: Input video Segment A
 *          ArgID 1: Level, equals mix ratio of black/A.
 *          ArgID 2: Reverse (fade from black).
 *
 *      The input segment and resulting datakind must be 
 *      omfi:data:PictureWithMatte.  If reverse is FALSE, then start with
 *      the input video and finish with black (fade to black).  If
 *      reverse is TRUE, start with black, end with the input video
 *      (fade from black).
 *
 *      This is a single-video-input effect, and is not legal as a 
 *      transition effect.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeVideoFadeToBlackNew(
    omfHdl_t 		file,         /* IN - File Handle */
    omfLength_t		length,       /* IN - Effect Length */
	omfSegObj_t		inputSegment, /* IN - Input segment */
	omfSegObj_t		level,        /* IN - Level */
	omfBool			reverse,      /* IN - Whether fade-to (FALSE) or 
								   *      fade-from (TRUE) */
    omfEffObj_t		*newEffect)   /* OUT - New Effect Object */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	booleanDatakind	= NULL;
	omfDDefObj_t	inputDatakind	= NULL;
	omfDDefObj_t	levelDatakind	= NULL;
	omfSegObj_t		reverseCVAL = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfObject_t	    tmpEffect = NULL;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((omfsIsInt64Positive(length)), file, OM_ERR_BAD_LENGTH);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
	*newEffect = NULL;
	
    XPROTECT(file)
	  {
		if (file->semanticCheckEnable)
		  {
			if (inputSegment)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegment, &inputDatakind,
											NULL));
				if (!omfiIsPictureWithMatteKind(file, inputDatakind,kConvertTo,
												&omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (level)
			  {
				CHECK(omfiComponentGetInfo(file, level, &levelDatakind,
											NULL));
				if (!omfiIsRationalKind(file, levelDatakind, kConvertTo, 
										&omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
		  }

		CHECK(LookupCreateDatakind(file, PICTWITHMATTEKIND, &outputDatakind));
		CHECK(LookupCreateDatakind(file, BOOLEANKIND, &booleanDatakind));

		omfiEffectDefLookup(file, kOmfFxVFadeToBlackGlobalName, &effectDef, 
							&omfErr);

		if (effectDef == NULL && omfErr == OM_ERR_NONE)
		{
			CHECK(omfiEffectDefNew(	file, 
									kOmfFxVFadeToBlackGlobalName,
									kOmfFxVFadeToBlackName,
									kOmfFxVFadeToBlackDescription,
									0, NOT_TIMEWARP, &effectDef));
		}
		else
		{
			CHECK(omfErr);
		}
		
	  	CHECK(omfiEffectNew(file, outputDatakind, length, 
							effectDef, &tmpEffect));
		
		/* INPUT MEDIA A */
		if (inputSegment)
	  		CHECK(omfiEffectAddNewSlot(	file, tmpEffect, kOmfFxIDGlobalATrack,
	  								inputSegment, &effectSlot));

		/* Gain */
		if (level)
	  		CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
	  								kOmfFxIDGlobalLevel,
	  								level, &effectSlot));
		/* Reverse */
	  	CHECK(omfiConstValueNew(file, booleanDatakind, length, 
	  							(omfInt32)sizeof(omfBool),
	  							(void *)&reverse, &reverseCVAL));
	  							
	  	CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
	  								kOmfFxVFadeToBlackReverse,
	  								reverseCVAL, &effectSlot));
	  }

	XEXCEPT
	  {
		if (tmpEffect)
		  omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeVideoFadeToBlackGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:VideoFadeToBlack effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeVideoFadeToBlackGetInfo(
    const omfHdl_t			file,         /* IN - File Handle */
    const omfEffObj_t 		effect,       /* IN - Effect Object */
    omfLength_t             *length,      /* OUT - Length of effect */
	omfSegObj_t				*inputSegment,/* OUT - Input segment */
	omfSegObj_t				*level,       /* OUT - Level */
	omfBool					*reverse,     /* OUT - Reverse */
									      /* OUT - Whether fade-to (FALSE) or 
								           *       fade-from (TRUE) */
	omfUInt32		 		*phaseOffset) /* OUT - Phase Offset */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfSegObj_t		reverseCVAL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfLength_t		effectLength;

	/* Initialize outputs */	
	
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfsCvtInt32toLength(0, effectLength);
	
	XPROTECT(file)
	{
		if (! omfiEffectDefLookup(file, kOmfFxVFadeToBlackGlobalName,
							 &expectedEffectDef, &omfErr))
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/*
		 *  The appropriate effect definition object MUST be in the file
		 *  AND MUST equal the definition object that is on the effect.  
		 *  If not, then the effect is invalid.
		 */
		 
		CHECK(omfErr);
		
		CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
								&effectLength, &effectDef));
								
		if (effectDef != expectedEffectDef)
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/* get length */
		if (length)
		  *length = effectLength;
					
		/* get input segment */
		if (inputSegment)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalATrack,
								  &effectSlot, inputSegment));
			if (effectSlot == NULL)
			  {
				*inputSegment = NULL;
			  }
		  }
		
		/* get level */
		if (level)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalLevel,
								  &effectSlot, level));
			if (effectSlot == NULL)
			  {
				*level = NULL;
			  }
		  }
		
		/* get reverse */
		if (reverse)
		  {
			CHECK(FindSlotByArgID(file, effect, kOmfFxVFadeToBlackReverse,
								  &effectSlot, &reverseCVAL));
			if (effectSlot)
			  {
				CHECK(GetConstBoolean(file, reverseCVAL, reverse));
			  }
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeMonoAudioMixdownNew()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:MonoAudioMixdown.  If the effect definition object
 *      (EDEF) does not already exist, it will be created, and registered with
 *      the file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      The MonoAudioMixdown effect mixes any number of mono audio 
 *      input segments into a single mono audio output segment. The audio
 *      segments are added together.  Except when the effect is used in
 *      a transition, there must be at least one effect slot.  The argID
 *      values specified in all effect slots must form a set of
 *      sequential integers starting at 1 (no duplicates are allowed).
 *      Since effect invocation order has an unordered set of effect
 *      slots, they can appear in any order.
 *
 *      Effect Inputs:
 *         ArgID 1: First input mono audio segment(required, unless transition)
 *         ArgID 2: Second input mono audio segment (optional)
 *         ArgID n: Nth input mono audio segment (optional)
 *
 *      The input segments must have the datakind of omfi:data:Sound or a
 *      datakind that can be converted to omfi:data:Sound.  The resulting
 *      datakind is omfi:data:Sound.
 *
 *      In a transition, no effect slots may be specified.  The effect mixes
 *      the overlapping sections of the adjacent segments.  
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeMonoAudioMixdownNew(
    omfHdl_t 		file,             /* IN - File Handle */
    omfLength_t		length,           /* IN - Effect Length */
	omfInt32		numInputSegments, /* IN - Number of input segments */
	omfSegObj_t		*inputSegments,   /* IN - List of input segments */
    omfEffObj_t		*newEffect)       /* OUT - New Effect Object */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t    inputDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfObject_t	    tmpEffect = NULL;
	omfInt32			i;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((omfsIsInt64Positive(length)), file, OM_ERR_BAD_LENGTH);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
	*newEffect = NULL;
	
    XPROTECT(file)
	  {
		CHECK(LookupCreateDatakind(file, SOUNDKIND, &outputDatakind));

		omfiEffectDefLookup(file, kOmfFxMAMixdownGlobalName, &effectDef, 
							&omfErr);

		if (effectDef == NULL && omfErr == OM_ERR_NONE)
		{
			CHECK(omfiEffectDefNew(	file, 
									kOmfFxMAMixdownGlobalName,
									kOmfFxMAMixdownName,
									kOmfFxMAMixdownDescription,
									0, NOT_TIMEWARP, &effectDef));
		}
		else
		{
			CHECK(omfErr);
		}
		
	  	CHECK(omfiEffectNew(file, outputDatakind, length, 
	  						effectDef, &tmpEffect));
		
		/* INPUT MEDIA */
		for (i = 1; i<= numInputSegments; i++)
		{
			if (inputSegments[i-1])
			  {
				if (file->semanticCheckEnable)
				  {
					CHECK(omfiComponentGetInfo(file, inputSegments[i-1], 
												&inputDatakind, NULL));
					if (!omfiIsSoundKind(file, inputDatakind, kConvertTo, 
										 &omfErr))
					  {
						RAISE(OM_ERR_INVALID_EFFECTARG);
					  }
				  }
	  			CHECK(omfiEffectAddNewSlot(	file, tmpEffect, i,
	  										inputSegments[i-1], &effectSlot));
			  }
	  	}

	  }

	XEXCEPT
	  {
		if (tmpEffect)
		  omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeMonoAudioMixdownGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:VideoSpeedControl effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeMonoAudioMixdownGetInfo(
    const omfHdl_t				file,         /* IN - File Handle */
    const omfEffObj_t 			effect,       /* IN - Effect Object */
    omfLength_t                 *length,      /* OUT - Length of effect */
	omfSegObj_t					*inputSegments, /* OUT - List of Input Segs */
	omfInt32					maxNumSegments, /* IN - Max number of Segs */
	omfInt32		 			*numSegments) /* OUT - Number of Segments */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfESlotObj_t	effectSlot = NULL;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfLength_t		effectLength;
	omfSegObj_t     tmpSeg = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;

	/* Initialize outputs */	
	
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfsCvtInt32toLength(0, effectLength);
	
	XPROTECT(file)
	{
		if (! omfiEffectDefLookup(file, kOmfFxMAMixdownGlobalName,
							 &expectedEffectDef, &omfErr))
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/*
		 *  The appropriate effect definition object MUST be in the file
		 *  AND MUST equal the definition object that is on the effect.  
		 *  If not, then the effect is invalid.
		 */
		 
		CHECK(omfErr);
		
		CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
								&effectLength, &effectDef));
								
		if (effectDef != expectedEffectDef)
			RAISE(OM_ERR_INVALID_EFFECTDEF);

		/* get length */
		if (length)
		  *length = effectLength;
					
		/* get input segments */
		/* set indexto negative one so that first trip sets to zero */
		/* use this method since you don't know the index is valid until */
		/* after the slot is checked */
		if (inputSegments && (maxNumSegments > 0))
		  {
			*numSegments = -1; 
			/* make it non-NULL to start loop*/
			effectSlot = (omfESlotObj_t)effect; 
			while (effectSlot != NULL)
			  {
				effectSlot = NULL;
				*numSegments++;
				
				if (*numSegments > maxNumSegments)
				  RAISE(OM_ERR_INTERN_TOO_SMALL);

				CHECK(FindSlotByArgID(file, effect, *numSegments+1,
								  &effectSlot, &tmpSeg));
				inputSegments[*numSegments] = tmpSeg;
			  }
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeSMPTEVideoWipeNew()
 *
 *      This function creates an effect object of effect definition type 
 *      omfi:effect:SMPTEVideoWipe.  If the effect definition object
 *      (EDEF) does not already exist, it will be created, and registered with
 *      the file.  Effect inputs are passed in as arguments, and turned into 
 *      effect slots containing the input values.  The newly created effect 
 *      object is returned.
 *
 *      A SMPTEVideoWipe effect combines 2 video streams using a
 *      SMPTE video wipe.
 *
 *      Effect Inputs:
 *          ArgID -1: Input video segment "A" (outgoing if transition)
 *          ArgID -2: Input video segment "B" (outgoing if transition)
 *          ArgID -3: Level, equal to "percentage done", range 0 to 1
 *          ArgID 1: SMPTE Wipe Number. No default, must be specified.
 *          ArgID 2: Reverse, default FALSE
 *          ArgID 3: Soft, default FALSE
 *          ArgID 4: Border, default FALSE
 *          ArgID 5: Position, default FALSE
 *          ArgID 6: Modulator, default FALSE
 *          ArgID 7: Shadow, default FALSE
 *          ArgID 8: Tumble, default FALSE
 *          ArgID 9: Spotlight, default FALSE
 *          ArgID 10: ReplicationH
 *          ArgID 11: ReplicationV
 *          ArgID 12: Checkerboard
 *
 *      The input segment datakinds must be omfi:data:PictureWithMatte
 *      or some datakind that is convertible.  The resulting datakind
 *      is omfi:data:PictureWithMatte.  The range for level is 0 to 1.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeSMPTEVideoWipeNew(
    omfHdl_t 		file,          /* IN - File Handle */
    omfLength_t		length,        /* IN - Effect Length */
	omfSegObj_t		inputSegmentA, /* IN - Input segment A */
	omfSegObj_t		inputSegmentB, /* IN - Input segment B */
	omfSegObj_t		level,         /* IN - Level */
	omfInt32		wipeNumber,    /* IN - Wipe Number */
	omfWipeArgs_t	*wipeControls, /* IN - Wipe Controls */
    omfEffObj_t		*newEffect)    /* OUT - New Effect Object */
{
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	inputDatakind = NULL;
	omfDDefObj_t	int32Datakind = NULL;
	omfDDefObj_t	booleanDatakind = NULL;
	omfDDefObj_t	pictDatakind = NULL;
	omfDDefObj_t	inputDatakindA = NULL;
	omfDDefObj_t	inputDatakindB = NULL;
	omfDDefObj_t	levelDatakind = NULL;
	omfSegObj_t		slotCVAL = NULL;	  	
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfObject_t	    tmpEffect = NULL;
	omfObject_t     tmpTrack = NULL;
	char            effectID1x[20], wipeNumString[4];
	omfRational_t   editRate;

	omfAssertValidFHdl(file);
	omfAssert((omfsIsInt64Positive(length)), file, OM_ERR_BAD_LENGTH);
	omfAssert((newEffect != NULL), file, OM_ERR_NULLOBJECT);
	*newEffect = NULL;
	
    XPROTECT(file)
	  {
		if (file->semanticCheckEnable)
		  {
			if (inputSegmentA)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegmentA, 
											&inputDatakindA, NULL));
				if (!omfiIsPictureWithMatteKind(file, inputDatakindA, 
												kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (inputSegmentB)
			  {
				CHECK(omfiComponentGetInfo(file, inputSegmentB, 
											&inputDatakindB, NULL));
				if (!omfiIsPictureWithMatteKind(file, inputDatakindB, 
												kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
			if (level)
			  {
				CHECK(omfiComponentGetInfo(file, level, &levelDatakind, NULL));
				if (!omfiIsRationalKind(file, levelDatakind, kConvertTo, &omfErr))
				  {
					RAISE(OM_ERR_INVALID_EFFECTARG);
				  }
			  }
		  } /* semanticCheckEnable */

		/* For 1.x, create a TRKG object with an EffectID of Wipe:SMPTE:# */
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(LookupCreateDatakind(file, PICTUREKIND, &pictDatakind));

			CHECK(omfsObjectNew(file, "TRKG", &tmpEffect));
			/* Set the editrate to NTSC by default */
			editRate.numerator = 2997;
			editRate.denominator = 100;
			CHECK(omfsWriteExactEditRate(file, tmpEffect, OMCPNTEditRate,
										 editRate));
			CHECK(omfsObjectNew(file, "TRAK", &tmpTrack));
			CHECK(omfsWriteInt16(file, tmpTrack, OMTRAKLabelNumber, 1));
			CHECK(omfsAppendObjRefArray(file, tmpEffect, OMTRKGTracks,
										tmpTrack));
			CHECK(ComponentSetNewProps(file, tmpEffect, length, 
									   pictDatakind));

			/* For 1.x, level on a TRAN Wipe:SMPTE:# is assumed to 
			 * be 0->100.  So, ignore the level argument.
			 */

			/* For 1.x, all wipe controls are assumed to be defaults.
			 * So, ignore the controls argument.
			 */

			/* For 1.x, the tracks in the TRAN are assumed to be 
			 * the incoming and outgoing clips.  So, ignore the 
			 * inputSegmentA and inputSegmentB arguments.
			 */
		    strcpy( (char*) effectID1x, "Wipe:SMPTE:");
		    itoaLocal(wipeNumber, 4, wipeNumString);
			/* NOTE: Report error if wipeNumString is NULL? */
		    strcat(effectID1x, wipeNumString);
		    CHECK(omfsWriteString(file, tmpEffect, OMCPNTEffectID,
					  effectID1x));
		  }

		/* For 1.x, create a TRKG object with an EffectID of  */
		else /* kOmfRev2x */
		  {
			CHECK(LookupCreateDatakind(file, INT32KIND, &int32Datakind));
			CHECK(LookupCreateDatakind(file, BOOLEANKIND, &booleanDatakind));
			CHECK(LookupCreateDatakind(file, PICTWITHMATTEKIND, &outputDatakind));

			omfiEffectDefLookup(file, kOmfFxVSMPTEWipeGlobalName, &effectDef, 
								&omfErr);

			if (effectDef == NULL && omfErr == OM_ERR_NONE)
			  {
				CHECK(omfiEffectDefNew(	file, 
									   kOmfFxVSMPTEWipeGlobalName,
									   kOmfFxVSMPTEWipeName,
									   kOmfFxVSMPTEWipeDescription,
									   0, NOT_TIMEWARP, &effectDef));
			  }
			else
			  {
				CHECK(omfErr);
			  }
		
			CHECK(omfiEffectNew(file, outputDatakind, length, 
								effectDef, &tmpEffect));
		
			/* INPUT MEDIA A */
			if (inputSegmentA)
			  CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
										 kOmfFxIDGlobalATrack,
										 inputSegmentA, &effectSlot));

			/* INPUT MEDIA B */
			if (inputSegmentB)
			  CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
										 kOmfFxIDGlobalBTrack,
										 inputSegmentB, &effectSlot));

			/* Level */
			if (level)
			  CHECK(omfiEffectAddNewSlot(	file, tmpEffect, 
										 kOmfFxIDGlobalLevel,
										 level, &effectSlot));
	  								
			/* Wipe Number */
			CHECK(omfiConstValueNew(file, int32Datakind, length, 
									(omfInt32)sizeof(omfInt32),
									(void *)&wipeNumber,
									&slotCVAL));
	  							
			CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
									   kOmfFxIDVSMPTEWipeWipeNumber,
									   slotCVAL, &effectSlot));
	  								
			if (wipeControls)
			  {
				
				/* REVERSE */
				CHECK(omfiConstValueNew(file, booleanDatakind, length, 
										(omfInt32)sizeof(omfBool),
										(void *)&(wipeControls->reverse),
										&slotCVAL));
	  							
				CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
										   kOmfFxIDVSMPTEWipeReverse,
										   slotCVAL, &effectSlot));
				
				/* SOFT */
				CHECK(omfiConstValueNew(file, booleanDatakind, length, 
										(omfInt32)sizeof(omfBool),
										(void *)&(wipeControls->soft),
										&slotCVAL));
	  							
				CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
										   kOmfFxIDVSMPTEWipeSoft,
										   slotCVAL, &effectSlot));
	  								
				/* BORDER */
				CHECK(omfiConstValueNew(file, booleanDatakind, length, 
										(omfInt32)sizeof(omfBool),
										(void *)&(wipeControls->border),
										&slotCVAL));
	  							
				CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
										   kOmfFxIDVSMPTEWipeBorder,
										   slotCVAL, &effectSlot));
	  								
				/* POSITION */
				CHECK(omfiConstValueNew(file, booleanDatakind, length, 
										(omfInt32)sizeof(omfBool),
										(void *)&(wipeControls->position),
										&slotCVAL));
				
				CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
										   kOmfFxIDVSMPTEWipePosition,
										   slotCVAL, &effectSlot));
	  								
				/* MODULATOR */
				CHECK(omfiConstValueNew(file, booleanDatakind, length, 
										(omfInt32)sizeof(omfBool),
										(void *)&(wipeControls->modulator),
										&slotCVAL));
	  							
				CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
										   kOmfFxIDVSMPTEWipeModulator,
										   slotCVAL, &effectSlot));
	  								
				/* SHADOW */
				CHECK(omfiConstValueNew(file, booleanDatakind, length, 
										(omfInt32)sizeof(omfBool),
										(void *)&(wipeControls->shadow),
										&slotCVAL));
	  							
				CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
										   kOmfFxIDVSMPTEWipeShadow,
										   slotCVAL, &effectSlot));
	  								
				/* TUMBLE */
				CHECK(omfiConstValueNew(file, booleanDatakind, length, 
										(omfInt32)sizeof(omfBool),
										(void *)&(wipeControls->tumble),
										&slotCVAL));
	  							
				CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
										   kOmfFxIDVSMPTEWipeTumble,
										   slotCVAL, &effectSlot));
	  								
				/* SPOTLIGHT */
				CHECK(omfiConstValueNew(file, booleanDatakind, length, 
										(omfInt32)sizeof(omfBool),
										(void *)&(wipeControls->spotlight),
										&slotCVAL));
	  							
				CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
										   kOmfFxIDVSMPTEWipeSpotLight,
										   slotCVAL, &effectSlot));
	  								
				/* REPLICATION H */
				CHECK(omfiConstValueNew(file, int32Datakind, length, 
										(omfInt32)sizeof(omfInt32),
										(void *)&(wipeControls->replicationH),
										&slotCVAL));
		  							
				CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
										   kOmfFxIDVSMPTEWipeReplicationH,
										   slotCVAL, &effectSlot));
				
				/* REPLICATION V */
				CHECK(omfiConstValueNew(file, int32Datakind, length, 
										(omfInt32)sizeof(omfInt32),
										(void *)&(wipeControls->replicationV),
										&slotCVAL));
		  							
				CHECK(omfiEffectAddNewSlot(	file, tmpEffect,
										   kOmfFxIDVSMPTEWipeReplicationV,
										   slotCVAL, &effectSlot));
			  }
		  } /* kOmfRev2x */
	  }

	XEXCEPT
	  {
		if (tmpEffect)
		  omfsObjectDelete(file, tmpEffect);
		return(XCODE());
	  }
	XEND;

	*newEffect = tmpEffect;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfeSMPTEVideoWipeGetInfo()
 *
 *      Given an effect object, this function returns the parameters on the
 *      effect slots of an omfi:effect:SMPTEVideoWipe effect.  It does
 *      not return time varying information.
 *
 * Argument Notes:
 *      This function will only return information through non-NULL
 *      arguments.  If an effect property/argument is not desired, specify a
 *      NULL for its argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfeSMPTEVideoWipeGetInfo(
    const omfHdl_t 		file,         /* IN - File Handle */
    const omfEffObj_t	effect,       /* IN - Effect object */
    omfLength_t         *length,      /* OUT - Length of effect */
	omfSegObj_t	        *inputSegmentA,       /* OUT - Input segment A */
	omfSegObj_t	        *inputSegmentB,       /* OUT - Input segment B */
	omfSegObj_t	        *level,               /* OUT - Level */
	omfInt32		*wipeNumber,      /* OUT - Wipe Number */
	omfWipeArgs_t	*wipeControls)    /* OUT - Wipe Controls */
{
	omfBool			isAdd = FALSE;
	omfDDefObj_t	outputDatakind = NULL;
	omfDDefObj_t	effectDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfSegObj_t		slotCVAL;
	omfESlotObj_t	effectSlot = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfEDefObj_t	effectDef = NULL;
	omfEDefObj_t	expectedEffectDef = NULL;
	omfLength_t		effectLength;
	char            *buf = NULL, *bufres = NULL;
	omfInt32        len;

	/* Initialize outputs */	
	omfAssertValidFHdl(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfsCvtInt32toLength(0, effectLength);
	
	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			if (length)
			  {
				CHECK(omfiComponentGetInfo(file, effect, &effectDatakind, 
										length));
			  }

			/* Pull the SMPTE Wipe Number off of the EffectID string */
			if (wipeNumber)
			  {
				len = omfsLengthString(file, effect, OMCPNTEffectID);
				buf = (char *)omfsMalloc(len+1);
				if(buf == NULL)
					RAISE(OM_ERR_NOMEMORY);
				memset(buf, 0, len+1);
				CHECK(omfsReadString(file, effect, OMCPNTEffectID, buf, len));
				bufres = strrchr(buf, ':');
				bufres++;
				*wipeNumber = atoi(bufres);
				omfsFree(buf);
				buf = NULL;
			  }

			/* These don't exist on 1.x, so set to defaults and NULL */
			if (level)
			  *level = NULL;
			if (inputSegmentA)
			  *inputSegmentA = NULL;
			if (inputSegmentB)
			  *inputSegmentB = NULL;
			if (wipeControls)
			  {
				wipeControls->reverse = FALSE;
				wipeControls->soft = FALSE;
				wipeControls->border = FALSE;
				wipeControls->position = FALSE;
				wipeControls->modulator = FALSE;
				wipeControls->shadow = FALSE;
				wipeControls->tumble = FALSE;
				wipeControls->spotlight = FALSE;
				wipeControls->replicationH = FALSE;
				wipeControls->replicationV = FALSE;
			  }
		  }
		else /* kOmfRev2x */
		  {
			if (! omfiEffectDefLookup(file, kOmfFxVSMPTEWipeGlobalName,
									  &expectedEffectDef, &omfErr))
			  RAISE(OM_ERR_INVALID_EFFECTDEF);
			
			/*
			 *  The appropriate effect definition object MUST be in the file
			 *  AND MUST equal the definition object that is on the effect.  
			 *  If not, then the effect is invalid.
			 */
		 
			CHECK(omfErr);
		
			CHECK(omfiEffectGetInfo(file, effect, &effectDatakind, 
									&effectLength, &effectDef));
			
			if (effectDef != expectedEffectDef)
			  RAISE(OM_ERR_INVALID_EFFECTDEF);

			/* get length */
			if (length)
			  *length = effectLength;
					
			/* get input segment */
			if (inputSegmentA)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalATrack,
									  &effectSlot, inputSegmentA));
				if (effectSlot == NULL)
				  *inputSegmentA = NULL;
			  }
		
			if (inputSegmentB)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalBTrack,
									  &effectSlot, inputSegmentB));
				if (effectSlot == NULL)
				  *inputSegmentB = NULL;
			  }
		
			if (level)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDGlobalLevel,
									  &effectSlot, level));
				if (effectSlot == NULL)
				  *level = NULL;
			  }

			/* get wipe number */
			if (wipeNumber)
			  {
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDVSMPTEWipeWipeNumber,
									  &effectSlot, &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstInt32(file, slotCVAL, wipeNumber));
				  }
				else
				  {
					RAISE(OM_ERR_PROP_NOT_PRESENT);
				  }	
			  }

			/* get wipe controls */
			if (wipeControls)
			  {
				/* get reverse */
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDVSMPTEWipeReverse,
									  &effectSlot,  &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstBoolean(file, slotCVAL, 
										  &(wipeControls->reverse)));
				  }
				else
				  {
					wipeControls->reverse = false;
				  }
				
				/* get soft */
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDVSMPTEWipeSoft,
									  &effectSlot,  &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstBoolean(file, slotCVAL, &(wipeControls->soft)));
				  }
				else
				  {
					wipeControls->soft = false;
				  }
				
				/* get border */
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDVSMPTEWipeBorder,
									  &effectSlot,  &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstBoolean(file, slotCVAL, 
										  &(wipeControls->border)));
				  }
				else
				  {
					wipeControls->border = false;
				  }
				
				/* get position */
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDVSMPTEWipePosition,
									  &effectSlot,  &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstBoolean(file, slotCVAL, 
										  &(wipeControls->position)));
				  }
				else
				  {
					wipeControls->position = false;
				  }
				
				/* get modulator */
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDVSMPTEWipeModulator,
									  &effectSlot,  &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstBoolean(file, slotCVAL, 
										  &(wipeControls->modulator)));
				  }
				else
				  {
					wipeControls->modulator = false;
				  }
		
				/* get shadow */
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDVSMPTEWipeShadow,
									  &effectSlot,  &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstBoolean(file, slotCVAL, 
										  &(wipeControls->shadow)));
				  }
				else
				  {
					wipeControls->shadow = false;
				  }
		
				/* get tumble */
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDVSMPTEWipeTumble,
									  &effectSlot,  &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstBoolean(file, slotCVAL, 
										  &(wipeControls->tumble)));
				  }
				else
				  {
					wipeControls->tumble = false;
				  }
		
				/* get spotlight */
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDVSMPTEWipeSpotLight,
									  &effectSlot,  &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstBoolean(file, slotCVAL, 
										  &(wipeControls->spotlight)));
				  }
				else
				  {
					wipeControls->spotlight = false;
				  }
				
				/* get replicationH */
				CHECK(FindSlotByArgID(file, effect, 
									  kOmfFxIDVSMPTEWipeReplicationH,
									  &effectSlot, &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstInt32(file, slotCVAL, 
										&(wipeControls->replicationH)));
				  }
				else
				  {
					wipeControls->replicationH = 1;
				  }

				/* get replicationV */
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDVSMPTEWipeReplicationV,
									  &effectSlot, &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstInt32(file, slotCVAL, 
										&(wipeControls->replicationV)));
				  }
				else
				  {
					wipeControls->replicationV = 1;
				  }

				/* get checkerboard */
				CHECK(FindSlotByArgID(file, effect, kOmfFxIDVSMPTEWipeCheckerboard,
									  &effectSlot, &slotCVAL));
				if (effectSlot)
				  {
					CHECK(GetConstBoolean(file, slotCVAL, &(wipeControls->checkerboard)));
				  }
				else
				  {
					wipeControls->checkerboard = false;
				  }
			  }
		  } /* kOmfRev2x */
	  }

	XEXCEPT
	  {
		if (buf)
		  omfsFree(buf);
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: LookupCreateDatakind()
 *
 *      This internal function looks up a datakind with the given name,
 *      and creates a datakind object if it does not already exist.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t LookupCreateDatakind(	
						const omfHdl_t file,        /* IN - File Handle */
						const omfUniqueNamePtr_t name, /* IN - Datakind name */
						omfDDefObj_t *defObject)    /* OUT - Datakind object */
{
	omfBool		 found = FALSE;
    omfDDefObj_t tmpDef = NULL;	
	omfErr_t	 omfErr = OM_ERR_NONE;

	XPROTECT(file)
	  {
		*defObject = NULL;
	
		found = omfiDatakindLookup(file, name, defObject, &omfErr);
	
		if (omfErr != OM_ERR_NONE)	/* oops, didn't work */
		  {
			RAISE(omfErr);
		  }
		else if (! found)			/* if not found, add the definition */
		  {
			CHECK(omfsObjectNew(file, "DDEF", &tmpDef));
			CHECK(omfsWriteUniqueName(file, tmpDef, OMDDEFDatakindID, name));
			CHECK(omfiDatakindRegister(file, name, tmpDef));
			*defObject = tmpDef;			
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE); 

}			

/*************************************************************************
 * Private Function: ExtractValueData()
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static void ExtractValueData(	
							 const omfHdl_t	file,      /* IN - File Handle */
							 void		*destination,  /* OUT - Dest data */
							 const void		*source,   /* IN - Source data */
							 const omfInt32		size)  /* IN - Data size */

{
	omfUInt16 tshort;
	omfUInt32 tlong;
	omfInt16 nativeByteOrder = 0;
	
#if PORT_BYTESEX_LITTLE_ENDIAN
	nativeByteOrder = INTEL_ORDER;
#else
	nativeByteOrder = MOTOROLA_ORDER;
#endif

	if(  file->byteOrder != nativeByteOrder )
	{
		switch (size)
		{
		case 1:
			*(omfUInt8 *)destination = *(omfUInt8 *)source;
			break;
		case 2:
			tshort = *((omfUInt16 *)source);
			*(omfUInt16 *) destination = (tshort >> 8) |
									   ((tshort & 0xff) << 8);
			break;
		case 4:
			tlong = *((omfUInt32 *)source);
			*(omfUInt32 *) destination = 
									((tlong & 0xff000000) >> 24) | 
									((tlong & 0x00ff0000) >>  8) |
									((tlong & 0x0000ff00) <<  8) |
									((tlong & 0x000000ff) << 24);
			break;
		default:
			memcpy(destination, source, size);
		}
	}
	else
	{
		memcpy(destination, source, size);
	}


}

/*************************************************************************
 * Private Function: GetConstUint32()
 *
 *      Given a CVAL object of datakind omfi:data:Uint32, this internal
 *      function returns the CVAL's value.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t GetConstUint32(	
							   const omfHdl_t file,    /* IN - File Handle */
							   const omfSegObj_t cval, /* IN - CVAL object */
							   const omfUInt32 *value) /* OUT - Data Value */
{
	omfDDefObj_t	uint32Datakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfInt64        bytesRead, valueSize64;
	omfInt32 			bytesRead32 = 0, valueSize;
	omfLength_t 	valueLength;
	omfUInt8 			buffer[sizeof(omfUInt32)];
	
	XPROTECT(file)
	{
	  /* NOTE: This should be converted to handle Int64 */
	    omfsCvtInt32toLength(0, valueLength);
		CHECK(LookupCreateDatakind(file, UINT32KIND, &uint32Datakind));
		
		if (omfiIsAConstValue(file, cval, &omfErr))
		{
		    valueSize = sizeof(omfUInt32);
			omfsCvtInt32toInt64(valueSize, &valueSize64);
			CHECK(omfiConstValueGetInfo(file, cval,
										&valueDatakind, &valueLength,
										valueSize64, &bytesRead,
										(void *)buffer));
			CHECK(omfsTruncInt64toInt32(bytesRead, &bytesRead32));	/* OK EFFECTPARM */
			if (bytesRead32 == sizeof(omfUInt32) &&
				valueDatakind == uint32Datakind)
			{
				ExtractValueData(	file, (void *)value, 
									(void *)buffer, 
									(omfInt32)sizeof(omfUInt32));
			}
			else
			{
				RAISE(OM_ERR_INVALID_CVAL);
			}
		}
		else if (omfErr == OM_ERR_NONE)
		{
			RAISE(OM_ERR_INVALID_CVAL);
		}
		else
		{
			CHECK(omfErr);
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;
	
	return(OM_ERR_NONE);
}


/*************************************************************************
 * Private Function: GetConstInt32
 *
 *      Given a CVAL object of datakind omfi:data:Int32, this internal
 *      function returns the CVAL's value.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t GetConstInt32(	
							  const omfHdl_t file,      /* IN - File Handle */
							  const omfSegObj_t	cval,   /* IN - CVAL object */
							  const omfInt32 *value)    /* OUT - Data Value */
{
	omfDDefObj_t	int32Datakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfErr_t		omfErr = OM_ERR_NONE;
	omfInt32 			bytesRead32 = 0, valueSize;
	omfInt64        bytesRead, valueSize64;
	omfLength_t 	valueLength;
	omfUInt8 			buffer[sizeof(omfUInt32)];
	
	XPROTECT(file)
	{
	  /* NOTE: This should be converted to handle Int64 */
	    omfsCvtInt32toLength(0, valueLength);
		CHECK(LookupCreateDatakind(file, INT32KIND, &int32Datakind));
		
		if (omfiIsAConstValue(file, cval, &omfErr))
		{
		    valueSize = sizeof(omfInt32);
			omfsCvtInt32toInt64(valueSize, &valueSize64);
			CHECK(omfiConstValueGetInfo(file, cval,
										&valueDatakind, &valueLength,
										valueSize64, &bytesRead,
										(void *)buffer));
			CHECK(omfsTruncInt64toInt32(bytesRead, &bytesRead32));	/* OK EFFECTPARM */
			if (bytesRead32 == sizeof(omfInt32) &&
				valueDatakind == int32Datakind)
			{
				ExtractValueData(	file, (void *)value, 
									(void *)buffer, 
									(omfInt32)sizeof(omfInt32));
			}
			else
			{
				RAISE(OM_ERR_INVALID_CVAL);
			}
		}
		else if (omfErr == OM_ERR_NONE)
		{
			RAISE(OM_ERR_INVALID_CVAL);
		}
		else
		{
			CHECK(omfErr);
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;
	
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: GetConstBoolean()
 *
 *      Given a CVAL object of datakind omfi:data:Boolean, this internal
 *      function returns the CVAL's value.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t GetConstBoolean(
								const omfHdl_t file,    /* IN - File Handle */
								const omfSegObj_t cval, /* IN - CVAL object */
								const omfBool *value)   /* OUT - Data Value */
{
	omfDDefObj_t	booleanDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfErr_t 		omfErr = OM_ERR_NONE;
	omfInt64        bytesRead, valueSize64;
	omfInt32 			bytesRead32 = 0, valueSize;
	omfLength_t 	valueLength;
	omfUInt8 			buffer[sizeof(omfBool)]; 
	
	XPROTECT(file)
	{
	    omfsCvtInt32toLength(0, valueLength);
		CHECK(LookupCreateDatakind(file, BOOLEANKIND, &booleanDatakind));
		
		if (omfiIsAConstValue(file, cval, &omfErr))
		{
		    valueSize = sizeof(omfBool);
			omfsCvtInt32toInt64(valueSize, &valueSize64);
			CHECK(omfiConstValueGetInfo(file, cval,
										&valueDatakind, &valueLength,
										valueSize64, &bytesRead,
										(void *)buffer));
			CHECK(omfsTruncInt64toInt32(bytesRead, &bytesRead32));	/* OK EFFECTPARM */
			if (bytesRead32 == sizeof(omfBool) &&
				valueDatakind == booleanDatakind)
			{
				ExtractValueData(	file, (void *)value, 
									(void *)buffer, 
									(omfInt32)sizeof(omfBool));
			}
			else
			{
				RAISE(OM_ERR_INVALID_CVAL);
			}
		}
		else if (omfErr == OM_ERR_NONE)
		{
			RAISE(OM_ERR_INVALID_CVAL);
		}
		else
		{
			CHECK(omfErr);
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;
	
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: GetConstRational()
 *
 *      Given a CVAL object of datakind omfi:data:Rational, this internal
 *      function returns the CVAL's value.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t GetConstRational(
						 const omfHdl_t file,        /* IN - File Handle */
						 const omfSegObj_t cval,     /* IN - CVAL Object */
						 const omfRational_t *value) /* OUT - Data Value */
{
	omfDDefObj_t	rationalDatakind = NULL;
	omfDDefObj_t	valueDatakind = NULL;
	omfErr_t 		omfErr = OM_ERR_NONE;
	omfInt64        bytesRead, valueSize64;
	omfInt32 			bytesRead32 = 0, valueSize;
	omfLength_t 	valueLength;
	omfUInt8 			buffer[sizeof(omfRational_t)];
	omfUInt8 			*tmpBufferPtr = buffer;
	
	XPROTECT(file)
	{
	    omfsCvtInt32toLength(0, valueLength);
		CHECK(LookupCreateDatakind(file, RATIONALKIND, &rationalDatakind));
		
		if (omfiIsAConstValue(file, cval, &omfErr))
		{
		    valueSize = sizeof(omfRational_t);
			omfsCvtInt32toInt64(valueSize, &valueSize64);
			CHECK(omfiConstValueGetInfo(file, cval,
										&valueDatakind, &valueLength,
										valueSize64, &bytesRead,
										(void *)buffer));
			CHECK(omfsTruncInt64toInt32(bytesRead, &bytesRead32));	/* OK EFFECTPARM */
			if (bytesRead32 == sizeof(omfRational_t) &&
				valueDatakind == rationalDatakind)
			{
				ExtractValueData(	file, (void *)&(value->numerator), 
									(void *)tmpBufferPtr, 
									(omfInt32)sizeof(omfInt32));
				tmpBufferPtr += sizeof(omfInt32);
				ExtractValueData(	file, (void *)&(value->denominator), 
									(void *)tmpBufferPtr, 
									(omfInt32)sizeof(omfInt32));
			}
			else
			{
				RAISE(OM_ERR_INVALID_CVAL);
			}
		}
		else if (omfErr == OM_ERR_NONE)
		{
			RAISE(OM_ERR_INVALID_CVAL);
		}
		else
		{
			CHECK(omfErr);
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;
	
	return(OM_ERR_NONE);
}

/* reverse: reverse string s in place */
static void reverse(char s[])
{
  int c, i, j;

  for (i = 0, j = strlen(s)-1; i < j; i++, j--)
    {
      c = s[i];
      s[i] = s[j];
      s[j] = c;
    }
}

/* itoaLocal: Convert n to characters in s */
static void itoaLocal(omfInt32 n, 
					  omfInt32 bufLen,
					  char buf[])
{
  int i;

  i = 0;
  do {               /* generate digits in reverse order */
      buf[i++] = n % 10 + '0';  /* get next digit */
  } while (((n /= 10) > 0) && (i < bufLen));  /* delete it */

  if (i < bufLen)
	{
	  buf[i] = '\0';
	  reverse(buf);
	}
  else  /* Buffer too small */
	buf[0] = '\0';
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
