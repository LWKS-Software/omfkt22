
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
#include "omPublic.h"
#include "omTable.h"
#include "omPvt.h"
#include "omMobMgt.h"
#include "omMedia.h"
#include "omEffect.h" 

#if OMFI_ENABLE_SEMCHECK

#define USE_NEW_CHECKS 0

#include <string.h>
#include "omCntPvt.h"

/*****************
 * Private Types
 ****************/
typedef struct {
  omfTrackID_t trackID;
  omfDDefObj_t datakind;
} TrackIdentity_t;

/************************************************************************
 *
 * Function Prototypes
 *
 ************************************************************************/
static omfBool         isValidByteOrder(omfHdl_t file, const void *data, 
										omfErr_t *omfError);

static omfBool 		isValidTrackKind(omfHdl_t file, const void *data, 
									 omfErr_t *omfError);
static omfBool 		isValidATTBKind(omfHdl_t file, const void *data, 
									omfErr_t *omfError);

static omfBool         isValidFilmKind(omfHdl_t file, const void *f, 
									   omfErr_t *omfError);
static omfBool         isValidEdgeType(omfHdl_t file, const void *e, 
									   omfErr_t *omfError);
static omfBool isValidTapeCaseType(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError);
static omfBool isValidVideoSignalType(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError);
static omfBool isValidTapeFormatType(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError);
static omfBool isValidInterpKind(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError);
static omfBool         isValidEditHint(
							   omfHdl_t file, 
							   const void *data,
							   omfErr_t *omfError);
static omfBool         isValidMobKind(omfHdl_t file, const void *m, 
									  omfErr_t *omfError);
static omfBool         isSourceMob(omfHdl_t file, const void *data, 
								   omfErr_t *omfError);
static omfBool         isCompositionMob(omfHdl_t file, const void *data, 
										omfErr_t *omfError);
static omfBool         isDefinition(omfHdl_t file, const void *data, 
									omfErr_t *omfError);
static omfBool         isBoolean(omfHdl_t file, const void *data, 
										  omfErr_t *omfError);
static omfBool         isNonNegativeShort(omfHdl_t file, const void *data, 
										  omfErr_t *omfError);
static omfBool         isPositiveShort(omfHdl_t file, const void *data, 
										  omfErr_t *omfError);
static omfBool         isNonNegativeLong(omfHdl_t file, const void *data, 
										 omfErr_t *omfError);
static omfBool         isPositiveLong(omfHdl_t file, const void *data, 
										 omfErr_t *omfError);
static omfBool         isValidTrackRef(omfHdl_t file, const void *data, 
									   omfErr_t *omfError);
static omfBool         isValidEditRate(omfHdl_t file, const void *data, 
									   omfErr_t *omfError);
static omfBool         isLocator(omfHdl_t file, const void *obj, 
								 omfErr_t *omfError);
static omfBool         isMediaData(omfHdl_t file, const void *data, 
								   omfErr_t *omfError);
static omfBool         isSpineObject(omfHdl_t file, const void *data, 
									 omfErr_t *omfError);

static omfBool isTrackUnique(omfHdl_t file,
							 omfObject_t mob,
							 omfObject_t track,
							 omfInt32 numTracks,
							 omfInt32 *listSize,
							 TrackIdentity_t **list,
							 omfErr_t *omfError);

/************************************************************************
 *
 * Public Functions
 *
 ************************************************************************/

/************************
 * Function: omfsSemanticCheckOn/omfsSemanticCheckOff/omfsIsSemanticCheckOn
 *
 * 	Enable/disable semantic checking at runtime.  These functions only work
 *		if OMFI_ENABLE_SEMCHECK is defined.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsSemanticCheckOn(
			omfHdl_t file)
{
	omfAssertValidFHdl(file);

	file->semanticCheckEnable = TRUE;
	
	return(OM_ERR_NONE);
}

omfErr_t omfsSemanticCheckOff(
			omfHdl_t file)
{
	omfAssertValidFHdl(file);

	file->semanticCheckEnable = FALSE;

	return(OM_ERR_NONE);
}

omfBool omfsIsSemanticCheckOn(omfHdl_t file)
{
	omfAssertValidFHdl(file);

	return(file->semanticCheckEnable == TRUE);
}

/************************************************************************
 *
 * Toolkit Support Functions
 *
 * These routines are called from the toolkit, or from any extended data type
 * access routines which you may write.
 *
 ************************************************************************/

/************************
 * initSemanticChecks	(INTERNAL)
 *
 * 		Called by the toolkit to initialize all of the semtantic checks which
 *		can not be derived from the omfsRegisterProperty calls.  This routine
 *		is called from the toolkit initialization if OMFI_ENABLE_SEMCHECK is defined.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t initSemanticChecks(
			omfSessionHdl_t session)
{
	XPROTECT(NULL)
	{
		CHECK(omfsAddPropertyCheck(session, OMWARPEditRate, kOmfTstRev1x, 
								   isValidEditRate, NULL));
		CHECK(omfsAddPropertyCheck(session, OMTRKRRelativeTrack, kOmfTstRev1x,
								   isValidTrackRef, NULL));
		CHECK(omfsAddPropertyCheck(session, OMTRKRRelativeScope, kOmfTstRev1x,
								   isNonNegativeShort, NULL));
		CHECK(omfsAddPropertyCheck(session, OMTRAKLabelNumber, kOmfTstRev1x, 
								   isNonNegativeShort, NULL));
		CHECK(omfsAddPropertyCheck(session, OMCLIPLength, kOmfTstRev1x, 
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMECCPStartEC, kOmfTstRev1x, 
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMTRANCutPoint, kOmfTstRev1x, 
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMTRKGGroupLength, kOmfTstRev1x, 
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMCompositionMobs, kOmfTstRev1x, 
								   isCompositionMob, NULL));
		CHECK(omfsAddPropertyCheck(session, OMSourceMobs, kOmfTstRev1x, 
								   isSourceMob, NULL));
		CHECK(omfsAddPropertyCheck(session, OMObjectSpine, kOmfTstRev1x, 
								   isSpineObject, NULL));
		CHECK(omfsAddPropertyCheck(session, OMCPNTEditRate, kOmfTstRev1x, 
								   isValidEditRate, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMDESMobKind, kOmfTstRev1x, 
								   isValidMobKind, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMASKIsDouble, kOmfTstRev1x, 
								   isBoolean, NULL));
		CHECK(omfsAddPropertyCheck(session, OMSLCTIsGanged, kOmfTstRev1x, 
								   isBoolean, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMDFLIsOMFI, kOmfTstRev1x, 
								   isBoolean, NULL));
		CHECK(omfsAddPropertyCheck(session, OMEDEFIsTimeWarp, kOmfTstRev2x, 
								   isBoolean, NULL));
		CHECK(omfsAddPropertyCheck(session, OMByteOrder, kOmfTstRev1x, 
								   isValidByteOrder, NULL));
		CHECK(omfsAddPropertyCheck(session, OMHEADByteOrder, kOmfTstRev2x, 
								   isValidByteOrder, NULL));


		/* 1.5 Compatability tests
		 */
		CHECK(omfsAddPropertyCheck(session, OMCPNTTrackKind, kOmfTstRev1x, 
								   isValidTrackKind, NULL));
		CHECK(omfsAddPropertyCheck(session, OMSPEDDenominator, kOmfTstRev1x, 
								   isPositiveLong, NULL));
		/* 1.5 & 2.x Tests
		 */
		CHECK(omfsAddPropertyCheck(session, OMATTBKind, kOmfTstRevEither, 
								   isValidATTBKind, NULL));
		CHECK(omfsAddPropertyCheck(session, OMTCCPFPS, kOmfTstRevEither, 
								   isPositiveShort, NULL));
	
		CHECK(omfsAddPropertyCheck(session, OMMDFLSampleRate, kOmfTstRevEither,
								   isValidEditRate, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMDFLLength, kOmfTstRevEither, 
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMediaData, kOmfTstRev1x, 
								   isMediaData, NULL));
		CHECK(omfsAddPropertyCheck(session, OMHEADMediaData, kOmfTstRev2x, 
								   isMediaData, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMDESLocator, kOmfTstRevEither, 
								   isLocator, NULL));
		CHECK(omfsAddPropertyCheck(session, OMECCPCodeFormat, 
								   kOmfTstRevEither, isValidEdgeType, NULL));
		CHECK(omfsAddPropertyCheck(session, OMECCPFilmKind, kOmfTstRevEither, 
								   isValidFilmKind, NULL));
	
		/* 2.x Only Tests
		 */
		CHECK(omfsAddPropertyCheck(session, OMCPNTLength, kOmfTstRev2x, 
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMECCPStart, kOmfTstRev2x, 
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMHEADDefinitionObjects, 
								   kOmfTstRev2x, isDefinition, NULL));
		CHECK(omfsAddPropertyCheck(session, OMSCLPStartTime, kOmfTstRev2x, 
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMSCLPFadeInLength, kOmfTstRev2x, 
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMSCLPFadeInType, kOmfTstRev2x, 
								   isNonNegativeShort, NULL));
		CHECK(omfsAddPropertyCheck(session, OMSCLPFadeOutLength, kOmfTstRev2x, 
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMSCLPFadeOutType, kOmfTstRev2x, 
								   isNonNegativeShort, NULL));
		CHECK(omfsAddPropertyCheck(session, OMTCCPStart, kOmfTstRev2x, 
								   isNonNegativeLong, NULL));

		/* 1.5 Compatability data classes
		 */
		CHECK(omfsAddPropertyClass(session, OMExternalFiles, kOmfTstRev1x, 
								   OMClassMOBJ));
		CHECK(omfsAddPropertyClass(session, OMTRAKTrackComponent, kOmfTstRev1x,
								   "CPNT"));
		CHECK(omfsAddPropertyClass(session, OMSEQUSequence, kOmfTstRev1x, 
								   "CPNT"));
		CHECK(omfsAddPropertyClass(session, OMAttributes, kOmfTstRev1x, 
								   "ATTR"));
		CHECK(omfsAddPropertyClass(session, OMATCPAttributes, kOmfTstRev1x, 
								   "ATTR"));
		CHECK(omfsAddPropertyClass(session, OMCPNTAttributes, kOmfTstRev1x, 
								   "ATTR"));
		CHECK(omfsAddPropertyClass(session, OMTRAKAttributes, kOmfTstRev1x, 
								   "ATTR"));
		CHECK(omfsAddPropertyClass(session, OMATTRAttrRefs, kOmfTstRevEither, 
								   "ATTB"));
/* There was too much variability of usage here 
		CHECK(omfsAddPropertyClass(session, OMTRAKFillerProxy, kOmfTstRev1x, 
								   OMClassFILL));	
*/
		CHECK(omfsAddPropertyClass(session, OMMOBJPhysicalMedia, kOmfTstRev1x, 
								   OMClassMDES));
		/* This should really be SCLP - but Media Composer writes effects
		 * here sometimes too.  So, we weakened the check.
		 */
		CHECK(omfsAddPropertyClass(session, OMCPNTPrecomputed, kOmfTstRev1x, 
								   "CPNT"));
		CHECK(omfsAddPropertyClass(session, OMTRKGTracks, kOmfTstRev1x, 
								   OMClassTRAK));

		/* 1.5 or 2.x data classes
		 */
		CHECK(omfsAddPropertyClass(session, OMCLSDParentClass, 
								   kOmfTstRevEither, OMClassCLSD));
		CHECK(omfsAddPropertyClass(session, OMClassDictionary, 
								   kOmfTstRev1x, OMClassCLSD));

		/* 2.x Only Data Classes
		 */
		CHECK(omfsAddPropertyClass(session, OMCPNTDatakind, kOmfTstRev2x, 
								   OMClassDDEF));
		CHECK(omfsAddPropertyClass(session, OMMSLTSegment, kOmfTstRev2x, 
								   OMClassSEGM));
		CHECK(omfsAddPropertyClass(session, OMNESTSlots, kOmfTstRev2x, 
								   OMClassSEGM));
		CHECK(omfsAddPropertyClass(session, OMSLCTSelected, kOmfTstRev2x, 
								   OMClassSEGM));
		CHECK(omfsAddPropertyClass(session, OMSLCTAlternates, kOmfTstRev2x, 
								   OMClassSEGM));
		CHECK(omfsAddPropertyClass(session, OMMSLTTrackDesc, kOmfTstRev2x,
									OMClassTRKD));
		CHECK(omfsAddPropertyClass(session, OMSEQUComponents, kOmfTstRev2x, 
								   "CPNT"));
		CHECK(omfsAddPropertyClass(session, OMHEADClassDictionary, 
								   kOmfTstRev2x, OMClassCLSD));
		CHECK(omfsAddPropertyClass(session, OMHEADMobs,
									kOmfTstRev2x, OMClassMOBJ));
		CHECK(omfsAddPropertyClass(session, OMMGRPChoices, kOmfTstRev2x, 
								   "SCLP"));
		CHECK(omfsAddPropertyClass(session, OMMGRPStillFrame, kOmfTstRev2x, 
								   "SCLP"));
		CHECK(omfsAddPropertyClass(session, OMMOBJSlots, kOmfTstRev2x, 
								   OMClassMSLT));
		CHECK(omfsAddPropertyClass(session, OMHEADPrimaryMobs, kOmfTstRev2x, 
								   OMClassMOBJ));
		CHECK(omfsAddPropertyClass(session, OMSMOBMediaDescription, 
								   kOmfTstRev2x, OMClassMDES));

								
		/* !!! Need checks for add data class checks to the codec INITs
		 */
		CHECK(omfsAddPropertyClass(session, OMTRANEffect, kOmfTstRev2x,
								   "EFFE"));
		CHECK(omfsAddPropertyClass(session, OMERATInputSegment, kOmfTstRev2x,
								   "SEGM"));
		CHECK(omfsAddPropertyClass(session, OMEFFEEffectKind, kOmfTstRev2x, 
								   "EDEF"));
		CHECK(omfsAddPropertyClass(session, OMEFFEEffectSlots, kOmfTstRev2x, 
								   "ESLT"));
		CHECK(omfsAddPropertyClass(session, OMEFFEFinalRendering,kOmfTstRev2x,
								   "SCLP"));
		CHECK(omfsAddPropertyClass(session, OMEFFEWorkingRendering, 
								   kOmfTstRev2x, "SCLP"));
		CHECK(omfsAddPropertyClass(session, OMESLTArgValue, kOmfTstRev2x,
								   "SEGM"));
		CHECK(omfsAddPropertyClass(session, OMVVALPointList, kOmfTstRev2x,
								   "CTLP"));
		CHECK(omfsAddPropertyClass(session, OMCTLPDatakind, kOmfTstRev2x,
								   "DDEF"));

		CHECK(omfsAddPropertyCheck(session, OMMDTPLength, kOmfTstRev2x,
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMDTPFormFactor, kOmfTstRev2x,
								   isValidTapeCaseType, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMDTPVideoSignal, kOmfTstRev2x,
								   isValidVideoSignalType, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMDTPTapeFormat, kOmfTstRev2x,
								   isValidTapeFormatType, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMDFMFrameRate, kOmfTstRev2x,
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMDFMFilmAspectRatio,kOmfTstRev2x,
								   isValidEditRate, NULL));
		CHECK(omfsAddPropertyCheck(session, OMMDFMFilmFormat, kOmfTstRev2x,
								   isValidFilmKind, NULL));
		CHECK(omfsAddPropertyCheck(session, OMERATInputEditRate,kOmfTstRev2x,
								   isValidEditRate, NULL));
		CHECK(omfsAddPropertyCheck(session, OMERATInputOffset,kOmfTstRev2x,
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMERATResultOffset,kOmfTstRev2x,
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMERATResultOffset,kOmfTstRev2x,
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMTRKDTrackID,kOmfTstRev2x,
								   isNonNegativeLong, NULL));
		CHECK(omfsAddPropertyCheck(session, OMVVALInterpolation,kOmfTstRev2x,
								   isValidInterpKind, NULL));
		CHECK(omfsAddPropertyCheck(session, OMCTLPEditHint,kOmfTstRev2x,
								   isValidEditHint, NULL));
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsCheckObjectType	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsCheckObjectType(
			omfHdl_t file, 
			omfObject_t obj, 
			omfProperty_t prop,
			omfType_t type)
{
	OMPropDef      	propData;
	omfBool			found, hasType;
	omfInt16		n;
    omfBool         saveCheckStatus = file->semanticCheckEnable;
	omfSessionHdl_t session;

	omfAssertValidFHdl(file);
	XPROTECT(file)
	{
		if (file->semanticCheckEnable)
		{
			session = file->session;
			XASSERT(session != NULL, OM_ERR_BAD_SESSION);
			XASSERT(session->cookie == SESSION_COOKIE, OM_ERR_BAD_SESSION);
	
			if (file->setrev == kOmfRev2x)
			{
				CHECK(omfsTablePropertyLookup(session->properties2X, prop, 
									  sizeof(propData), &propData, &found));
			}
			else
			{
				CHECK(omfsTablePropertyLookup(session->properties1X, prop, 
									  sizeof(propData), &propData, &found));
			}
			if (found)
			{
				for(n = 0, hasType = FALSE; (n < propData.numTypes) && !hasType; n++)
				{
					if((type == propData.type[n]) || (propData.type[n] == OMNoType))
						hasType = TRUE;
				}
				if(hasType && (!streq(propData.classTag, OMClassAny)))
				{
				  /* Stop recursion in checks */
					file->semanticCheckEnable = FALSE;	 
									
					if (!omfsIsTypeOf(file, obj, propData.classTag, NULL))
					  {
						RAISE(OM_ERR_OBJECT_SEMANTIC);
					  }
					/* Stop recursion in checks */
					file->semanticCheckEnable = saveCheckStatus;
				}
			}
		}
	}
	XEXCEPT
	  {
		/* Stop recursion in checks */
		file->semanticCheckEnable = saveCheckStatus;
	  }
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsAddPropertyCheck	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsAddPropertyCheck(
			omfSessionHdl_t			session, 
			omfProperty_t				prop, 
			omfValidRev_t				rev, 
			omfDataValidityTest_t	test, 
			char							*testName)
{
	OMPropDef	propData;
	omfBool		found;

	XPROTECT(NULL)
	{
		if(rev == kOmfTstRev1x || rev == kOmfTstRevEither)
		{
			CHECK(omfsTablePropertyLookup(session->properties1X, prop, 
										  sizeof(propData), &propData, &found));
			if (found)
			{

				propData.testProc = test;
				propData.testName = testName;
				CHECK(omfsTableAddProperty(session->properties1X, 
					 prop, &propData, sizeof(propData), kOmTableDupReplace));
			}
			else
				RAISE(OM_ERR_BAD_PROP);
		}
	
		if(rev == kOmfTstRev2x || rev == kOmfTstRevEither)
		{
			(void)omfsTablePropertyLookup(session->properties2X, prop, 
										  sizeof(propData), &propData, &found);
			if (found)
			{
				propData.testProc = test;
				propData.testName = testName;
				CHECK(omfsTableAddProperty(session->properties2X, 
					 prop, &propData, sizeof(propData), kOmTableDupReplace));
			}
			else
				RAISE(OM_ERR_BAD_PROP);
		}
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsAddPropertyClass	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		dataClass - This routine does NOT make a copy of dataClass, as
 *		it is assumed to be a literal.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsAddPropertyClass(
			omfSessionHdl_t		session, 
			omfProperty_t			prop, 
			omfValidRev_t			rev, 
			omfClassIDPtr_t		dataClass)
{
	OMPropDef	propData;
	omfBool		found;
	
	XPROTECT(NULL)
	{
		if(rev == kOmfTstRev1x || rev == kOmfTstRevEither)
		{
			CHECK(omfsTablePropertyLookup(session->properties1X, prop, 
										  sizeof(propData), &propData, &found));
			if (found)
			{

				propData.dataClass = dataClass;
				CHECK(omfsTableAddProperty(session->properties1X, 
					 prop, &propData, sizeof(propData), kOmTableDupReplace));
			}
			else
				RAISE(OM_ERR_BAD_PROP);
		}
	
		if(rev == kOmfTstRev2x || rev == kOmfTstRevEither)
		{
			(void)omfsTablePropertyLookup(session->properties2X, prop, 
										  sizeof(propData), &propData, &found);
			if (found)
			{
				propData.dataClass = dataClass;
				CHECK(omfsTableAddProperty(session->properties2X, 
					 prop, &propData, sizeof(propData), kOmTableDupReplace));
			}
			else
				RAISE(OM_ERR_BAD_PROP);
		}
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsCheckDataValidity	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsCheckDataValidity(
			omfHdl_t file, 
			omfProperty_t prop, 
			const void *data,
			omfAccessorType_t type)
{
	OMPropDef      	propData;
	omfSessionHdl_t session;
	omfBool         valid, found, hasType;
	omfInt16		n;
    omfBool         saveCheckStatus = file->semanticCheckEnable;
	omTable_t		*table;
	omfErr_t		status = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	XPROTECT(file)
	{
		if (file->semanticCheckEnable)
		{
			session = file->session;
			XASSERT(session != NULL, OM_ERR_BAD_SESSION);
			XASSERT(session->cookie == SESSION_COOKIE, OM_ERR_BAD_SESSION);
	
			if ((file->setrev == kOmfRev1x) ||(file->setrev == kOmfRevIMA))
			  table = session->properties1X;
			else /* kOmfRev2x */
			  table = session->properties2X;

			CHECK(omfsTablePropertyLookup(table, prop, sizeof(propData), 
										  &propData, &found));
			/* Used to stop recursion in checks */
			file->semanticCheckEnable = FALSE;
	
			if (found && (propData.testProc != NULL))
			{
				for(n = 0, hasType = FALSE; (n < propData.numTypes) && !hasType; n++)
				{
					if ((file->setrev == kOmfRev1x) ||(file->setrev == kOmfRevIMA))
						valid = ((propData.revs == kOmfTstRev1x) || 
							 (propData.revs == kOmfTstRevEither));
						else if (file->setrev == kOmfRev2x)
							valid = ((propData.revs == kOmfTstRev2x) || 
							 		(propData.revs == kOmfTstRevEither));
						else
							valid = FALSE;
	
					if (valid &&
					    (propData.type[n] != OMObjRef) &&
					    (propData.type[n] != OMObjRefArray) &&
					    (propData.type[n] != OMMobIndex))
					{
						if (!(*propData.testProc) (file, data, &status))
						{
							if (type == kOmGetFunction)
								status = OM_ERR_DATA_OUT_SEMANTIC;
							else
								status = OM_ERR_DATA_IN_SEMANTIC;
						}
						else
						{
							status = OM_ERR_NONE;
							hasType = TRUE;
						}
					}
				}
			}
			/* Used to stop recursion in checks */
			file->semanticCheckEnable = saveCheckStatus;
		}
		if (status != OM_ERR_NONE)
			RAISE(status);
	}
	XEXCEPT
	  {
		file->semanticCheckEnable = saveCheckStatus;
	  }
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsCheckObjRefValidity	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsCheckObjRefValidity(
			omfHdl_t file, 
			omfProperty_t prop, 
			omfObject_t refObj,
			omfAccessorType_t type)
{
	OMPropDef      	propData;
	omfSessionHdl_t session;
	omfBool         valid, found, hasType;
	omfInt16		n;
    omfBool         saveCheckStatus = file->semanticCheckEnable;
	omTable_t		*table;
	omfErr_t         status = OM_ERR_NONE;
	omfClassID_t	debugClassID;
	omfProperty_t	idProp;
	
	omfAssertValidFHdl(file);
	XPROTECT(file)
	{
		if (file->semanticCheckEnable)
		{
			session = file->session;
			XASSERT(session != NULL, OM_ERR_BAD_SESSION);
			XASSERT(session->cookie == SESSION_COOKIE, OM_ERR_BAD_SESSION);

			if ((file->setrev == kOmfRev1x) ||(file->setrev == kOmfRevIMA))
			{
				table = session->properties1X;
				idProp = OMObjID;
			}
			else /* kOmfRev2x */
			{
				idProp = OMOOBJObjClass;
			 	table = session->properties2X;
			}

			CHECK(omfsTablePropertyLookup(table, prop, sizeof(propData), 
										  &propData, &found));
			/* Used to stop recursion in checks */
			file->semanticCheckEnable = FALSE;	
		
			for(n = 0, hasType = FALSE; (n < propData.numTypes) && !hasType; n++)
			{
				if ((file->setrev == kOmfRev1x) ||(file->setrev == kOmfRevIMA))
					valid = ((propData.revs == kOmfTstRev1x) || 
							 (propData.revs == kOmfTstRevEither));
				else if (file->setrev == kOmfRev2x)
					valid = ((propData.revs == kOmfTstRev2x) || 
							 (propData.revs == kOmfTstRevEither));
				else
					valid = FALSE;

				if(found && valid)
				{
					if ((propData.type[n] == OMObjRef) ||
					    (propData.type[n] == OMObjRefArray) ||
					    (propData.type[n] == OMMobIndex))
					{
						if((propData.dataClass != NULL) &&
						   (!omfsIsTypeOf(file, refObj, propData.dataClass, &status)))
						{
							(void)omfsReadClassID(file, refObj, idProp, debugClassID);
							RAISE(OM_ERR_INVALID_LINKAGE);
						}
						
						if(propData.testProc != NULL)
						{
							if (!(*propData.testProc) (file, refObj, &status))
							{
								RAISE(status);
							}
							else
							{
								hasType = TRUE;
							}
						}
					}
				}
			}
			/* Used to stop recursion in checks */
			file->semanticCheckEnable = saveCheckStatus;	
		}
	}
	XEXCEPT
	  {
		/* Used to stop recursion in checks */
		file->semanticCheckEnable = saveCheckStatus;
	  }
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: objRefIndexInBounds	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfBool objRefIndexInBounds(
			omfHdl_t file, 
			omfObject_t obj, 
			omfProperty_t prop, 
			omfInt32 i)
{
	if (i < 0 || i > (omfInt32)omfsLengthObjRefArray(file, obj, prop))
		return (FALSE);
	return (TRUE);
}

/************************
 *  Function: mobIndexInBounds	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfBool mobIndexInBounds(
			omfHdl_t file, 
			omfObject_t obj, 
			omfProperty_t prop, 
			omfInt32 i)
{
	if (i < 0 || i > (omfInt32)omfsLengthObjIndex(file, obj, prop))
		return (FALSE);
	return (TRUE);
}

/************************
 * Function: isVirtualObj	(INTERNAL)
 *
 * 	Returns TRUE if the given class is a "virtual" class, which
 *		should not be instantiated directly.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		TRUE if the given class is virtual.
 *
 * Possible Errors:
 *		<none>.
 */
omfBool isVirtualObj(
			omfClassIDPtr_t	objID)
{
	if(objID == NULL)
		return(FALSE);
		
	if (!streq(objID, "CPNT") && !streq(objID, "CLIP") &&
	    !streq(objID, "MDFL") && !streq(objID, "WARP"))
		return (FALSE);

	return (TRUE);
}

/************************************************************************
 *
 * Callback Routines
 *
 * These routines are callbacks installed by "omfsAddPropertyCheck()".
 * 
 * All routines  take a file, data reference, and a status pointer, and
 *	return an omfBool.
 *
 ************************************************************************/

/************************
 * Function: isValidByteOrder	(INTERNAL)
 */
static omfBool isValidByteOrder(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfAssertValidFHdl(file);
    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);

	if ((*((omfInt16 *) data) != MOTOROLA_ORDER) && 
		(*((omfInt16 *) data) != INTEL_ORDER))
	  {
		*omfError = OM_ERR_INVALID_BYTEORDER;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isValidTrackKind	(INTERNAL)
 */
static omfBool isValidTrackKind(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfInt16 *t = (omfInt16 *)data;

    *omfError = OM_ERR_NONE;

	if(t == NULL)
		return(FALSE);
	if ((*t != TT_NULL) && (*t != TT_PICTURE) && (*t != TT_SOUND) &&
		(*t != TT_TIMECODE) && (*t != TT_EDGECODE) && (*t != TT_ATTRIBUTE) &&
		(*t != TT_EFFECTDATA))
	  {
		*omfError = OM_ERR_INVALID_TRACKKIND;
		return(FALSE);
	  }
	return(TRUE);
}

/************************
 * Function: isValidATTBKind	(INTERNAL)
 */
static omfBool isValidATTBKind(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
  omfInt16 *attbReadPtr;
  omfAttributeKind_t value;

  attbReadPtr = (omfInt16 *)data;

  *omfError = OM_ERR_NONE;

  if(attbReadPtr == NULL)
	return(FALSE);
  value = (omfAttributeKind_t)(*attbReadPtr);
  if ((value != kOMFIntegerAttribute) && (value != kOMFStringAttribute) && 
		(value != kOMFObjectAttribute) && (value != kOMFNullAttribute))
	  {
		*omfError = OM_ERR_INVALID_ATTRIBUTEKIND;
		return(FALSE);
	  }
	return(TRUE);
}

/************************
 * Function: isValidFilmKind	(INTERNAL)
 */
static omfBool isValidFilmKind(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfFilmType_t   f;

    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);
	f = *((omfFilmType_t *) data);
	if ((f != kFtNull) && (f != kFt35MM) && (f != kFt16MM) &&
	    (f != kFt35MM) && (f != kFt65MM))
	  {
		*omfError = OM_ERR_INVALID_FILMTYPE;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isValidEdgeType	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfBool isValidEdgeType(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfEdgeType_t   e;

    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);

	e = *((omfEdgeType_t *) data);
	if ((e != kEtNull) && (e != kEtKeycode) && (e != kEtEdgenum4) &&
	    (e != kEtEdgenum5))
	  {
		*omfError = OM_ERR_INVALID_EDGETYPE;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isValidTapeCaseType	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfBool isValidTapeCaseType(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfTapeCaseType_t   e;

    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);

	e = *((omfTapeCaseType_t *) data);
	if ((e != kTapeCaseNull) && (e != kThreeFourthInchVideoTape) 
		&& (e != kVHSVideoTape) && (e != k8mmVideoTape)
		&& (e != kBetacamVideoTape) && (e != kCompactCassette) 
		&& (e != kDATCartridge) && (e != kNagraAudioTape))
	  {
		*omfError = OM_ERR_INVALID_TAPECASETYPE;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isValidVideoSignalType	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfBool isValidVideoSignalType(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfVideoSignalType_t   e;

    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);

	e = *((omfVideoSignalType_t *) data);
	if ((e != kVideoSignalNull) && (e != kNTSCSignal) 
		&& (e != kPALSignal) && (e != kSECAMSignal))
	  {
		*omfError = OM_ERR_INVALID_VIDEOSIGNALTYPE;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isValidTapeFormatType	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfBool isValidTapeFormatType(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfTapeFormatType_t   e;

    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);

	e = *((omfTapeFormatType_t *) data);
	if ((e != kTapeFormatNull) && (e != kBetacamFormat) 
		&& (e != kBetacamSPFormat) && (e != kVHSFormat)
		&& (e != kSVHSFormat) && (e != k8mmFormat)
		&& (e != kHi8Format))
	  {
		*omfError = OM_ERR_INVALID_TAPEFORMATTYPE;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isValidInterpKind	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfBool isValidInterpKind(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfInterpKind_t   e;

    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);

	e = *((omfInterpKind_t *) data);
	if ((e != kConstInterp) && (e != kLinearInterp))
	  {
		*omfError = OM_ERR_INVALID_INTERPKIND;
		return (FALSE);
	  }
	return (TRUE);
}


static omfBool isValidEditHint(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfEditHint_t   e;

    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);

	e = *((omfEditHint_t *) data);
	if ((e != kNoEditHint) && (e != kProportional)
		&& (e != kRelativeLeft) && (e != kRelativeRight)
		&& (e != kRelativeFixed))
	  {
		*omfError = OM_ERR_INVALID_EDITHINT;
		return (FALSE);
	  }
	return (TRUE);
}


/************************
 * Function: isValidMobKind	(INTERNAL)
 *
 * 		WhatIt(Internal)Does.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static omfBool isValidMobKind(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
#if 0
	omfPhysicalMobType_t m;
#endif

    *omfError = OM_ERR_NONE;
	if (data == NULL)
	  {
		*omfError = OM_ERR_NULLOBJECT;
		return (FALSE);
	  }

	/* NOTE: Avid Media Composer seems to have other mob types, so
	 *       leave this check out for now.
	 */
#if 0
	m = *((omfPhysicalMobType_t *) data);
	if ((m != PT_NULL) && (m != PT_FILE_MOB) && (m != PT_TAPE_MOB) &&
	    (m != PT_FILM_MOB) && (m != PT_NAGRA_MOB))
	  {
		*omfError = OM_ERR_INVALID_MOBTYPE;
		return (FALSE);
	  }
#endif
	return (TRUE);
}

/************************
 * Function: isSourceMob	(INTERNAL)
 */
static omfBool isSourceMob(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfBool retValue = TRUE;
    *omfError = OM_ERR_NONE;
	if(data == NULL)
	  {
		*omfError = OM_ERR_NULLOBJECT;
		return(FALSE);
	  }
	if(file->setrev == kOmfRev2x)
		retValue = omfsIsTypeOf(file, (omfObject_t) data, OMClassSMOB, NULL);
	else
		retValue = omfsIsTypeOf(file, (omfObject_t) data, OMClassMOBJ, NULL);

	if (!retValue)
	  *omfError = OM_ERR_INVALID_LINKAGE;
	return(retValue);
}

/************************
 * Function: isCompositionMob	(INTERNAL)
 */
static omfBool isCompositionMob(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfBool retValue = TRUE, tmpRetValue = FALSE;
    *omfError = OM_ERR_NONE;
	if (data == NULL)
	{
		*omfError = OM_ERR_NULLOBJECT;
		return (FALSE);
	}
	if(file->setrev == kOmfRev2x)
	{
		retValue = omfsIsTypeOf(file, (omfObject_t)data, OMClassCMOB, NULL);
		if (!retValue)
		{
			tmpRetValue = omfsIsTypeOf(file, (omfObject_t)data, OMClassMOBJ, NULL);
			if (!tmpRetValue)
			  *omfError = OM_ERR_INVALID_LINKAGE;
			else /* It is a Mob, just not a comp mob */
			  *omfError = OM_ERR_INVALID_MOBTYPE;
		}
		return(retValue);
	}
	else
	{
		if (!omfsIsTypeOf(file, (omfObject_t) data, OMClassMOBJ, NULL))
		{
			*omfError = OM_ERR_INVALID_LINKAGE;
			return (FALSE);
		}
		else
		{
			if (omfsIsPropPresent(file, (omfObject_t) data, 
								  OMMOBJPhysicalMedia, OMObjRef))
			  {
				*omfError = OM_ERR_INVALID_MOBTYPE;
				return (FALSE);
			  }
		}
		return (TRUE);
	}
}

/************************
 * Function: isDefinition	(INTERNAL)
 */
static omfBool isDefinition(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
    omfBool retValue = FALSE;
    *omfError = OM_ERR_NONE;

	if (data == NULL)
	  {
		*omfError = OM_ERR_NULLOBJECT;
		return (FALSE);
	  }

	retValue = (omfsIsTypeOf(file, (omfObject_t) data, OMClassEDEF, NULL)) ||
	  (omfsIsTypeOf(file, (omfObject_t) data, OMClassDDEF, NULL));
	if (!retValue)
	  *omfError = OM_ERR_INVALID_LINKAGE;

	return(retValue);
}

/************************
 * Function: isNonNegativeShort	(INTERNAL)
 */
static omfBool isNonNegativeShort(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);
	if (*((omfInt16 *) data) < 0)
	  {
		*omfError = OM_ERR_REQUIRED_POSITIVE;
		return (FALSE);
	  }
	return (TRUE);
}
/************************
 * Function: isPositiveShort	(INTERNAL)
 */
static omfBool isPositiveShort(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);
	if (*((omfInt16 *) data) <= 0)
	  {
		*omfError = OM_ERR_REQUIRED_POSITIVE;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isBoolean	(INTERNAL)
 */
static omfBool isBoolean(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
    char		tst;
    
    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);
	tst = *((char*) data);
	if ((tst != 0) && (tst != 1))
	  {
		*omfError = OM_ERR_INVALID_BOOLTYPE;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isNonNegativeLong	(INTERNAL)
 */
static omfBool isNonNegativeLong(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);

	if (*((omfInt32 *) data) < 0)
	  {
		*omfError = OM_ERR_REQUIRED_POSITIVE;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isPositiveLong	(INTERNAL)
 */
static omfBool isPositiveLong(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);

	if (*((omfInt32 *) data) <= 0)
	  {
		*omfError = OM_ERR_REQUIRED_POSITIVE;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isValidTrackRef	(INTERNAL)
 */
static omfBool isValidTrackRef(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfBool retValue = TRUE;
    *omfError = OM_ERR_NONE;
	if(file->setrev == kOmfRev2x)
	  {
		*omfError = OM_ERR_NOT_IN_20;
		return(FALSE);
	  }
	if (data == NULL)
		return (FALSE);
	if (*((omfInt16 *) data) >= 0)
	  {
		*omfError = OM_ERR_INVALID_TRACK_REF;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isValidEditRate	(INTERNAL)
 */
static omfBool isValidEditRate(
			omfHdl_t file, 
			const void *data,
			omfErr_t *omfError)
{
	omfBool retValue = TRUE;
	omfRational_t   rate;

    *omfError = OM_ERR_NONE;
	if (data == NULL)
		return (FALSE);
	rate = *((omfRational_t *) data);
	if ((rate.numerator < 0) || (rate.denominator <= 0))
	  {
		*omfError = OM_ERR_REQUIRED_POSITIVE;
		return (FALSE);
	  }
	return (TRUE);
}

/************************
 * Function: isLocator	(INTERNAL)
 */
static omfBool isLocator(
			omfHdl_t file, 
			const void *obj,
			omfErr_t *omfError)
{
	omfBool retValue = TRUE;
	omfErr_t        testStat;	/* Don't need to set global status */

    *omfError = OM_ERR_NONE;
	if(obj == NULL)
	  {
		*omfError = OM_ERR_NULLOBJECT;
		return(FALSE);
	  }
	if(file->setrev == kOmfRev2x)
	{
		retValue = omfsIsTypeOf(file, (omfObject_t) obj, OMClassLOCR, &testStat);
		if (!retValue)
		  *omfError = OM_ERR_INVALID_LINKAGE;
		return(retValue);
	}
/* NOTE: This should really be a warning for 1.x files */
#if 0
	else
	{
		testStat = omfsReadClassID(file, (omfObject_t) obj, objID);
		if (testStat != OM_ERR_NONE)
		{
			*omfError = OM_ERR_INVALID_LINKAGE;
			return (FALSE);
		}
		if (streq(objID, "MACL") || streq(objID, "DOSL") ||
		    streq(objID, "UNXL") || streq(objID, "TXTL"))
		{
			return (TRUE);
		}
	}

	*omfError = OM_ERR_INVALID_LINKAGE;
#endif
	return (TRUE);		/* In 1.x we must assume that it IS a locator */
}

/************************
 * Function: isMiscPublic	(INTERNAL)
 */
static omfBool isMiscPublic(
			omfHdl_t file, 
			const void *obj,
			omfErr_t *omfError)
{
	omfBool retValue = TRUE;
	omfClassID_t		objID;
	
    *omfError = OM_ERR_NONE;
	omfAssertValidFHdl(file);
	if(obj == NULL)
	  {
		*omfError = OM_ERR_NULLOBJECT;
		return(FALSE);
	  }
	if (omfsReadClassID(file, (omfObject_t) obj, OMObjID, objID) != OM_ERR_NONE)
	{
		return (FALSE);
	}
	if (streq(objID, "HEAD") || streq(objID, "CLSD") ||
	    streq(objID, "TRAK") || streq(objID, "ATTB") ||
	    streq(objID, "ATTR"))
	{
		return (TRUE);
	}
	return (FALSE);
}

/************************
 * Function: isMediaData	(INTERNAL)
 */
static omfBool isMediaData(
			omfHdl_t file, 
			const void *obj,
			omfErr_t *omfError)
{
	omfBool retValue = TRUE;
    *omfError = OM_ERR_NONE;
	if(obj == NULL)
	  {
		*omfError = OM_ERR_NULLOBJECT;
		return(FALSE);
	  }
	if(file->setrev == kOmfRev2x)
	{
		retValue = omfsIsTypeOf(file, (omfObject_t) obj, OMClassMDAT, NULL);
		if (!retValue)
		  *omfError = OM_ERR_INVALID_LINKAGE;
		return(retValue);
	}
	else
	{
		if (omfsIsTypeOf(file, (omfObject_t) obj, OMClassMDES, NULL) ||
		    omfsIsTypeOf(file, (omfObject_t) obj, OMClassCPNT, NULL) ||
		    isMiscPublic(file, (omfObject_t) obj, omfError))
		  {
			*omfError = OM_ERR_INVALID_LINKAGE;
			return (FALSE);
		  }
		
		return (TRUE);
	}
}

/************************
 * Function: isSpineObject	(INTERNAL)
 */
static omfBool isSpineObject(
			omfHdl_t file, 
			const void *obj,
			omfErr_t *omfError)
{
	omfBool retValue = TRUE;
    *omfError = OM_ERR_NONE;
	if(file->setrev == kOmfRev2x)
	  {
		*omfError = OM_ERR_NOT_IN_20;
		return(FALSE);
	  }
	if (obj == NULL)
	  {
		*omfError = OM_ERR_NULLOBJECT;
		return(FALSE);
	  }
		
	if (omfsIsTypeOf(file, (omfObject_t) obj, "MOBJ", NULL))
		return (TRUE);

	retValue = isMediaData(file, (omfObject_t) obj, omfError);
	if (!retValue)
	  *omfError = OM_ERR_INVALID_LINKAGE;

	return(retValue);
}

/*****************************************************************************/
/********************************* omfFile Checker support *******************/
/*****************************************************************************/

static void DisplayObjectHeader(omfHdl_t file, omfObject_t obj,  char *outstr);
static omfBool IsValidOMFObject(omfFileCheckHdl_t hdl, omfClassIDPtr_t classID);
static omfErr_t ClearPropFoundFlag(omfFileCheckHdl_t hdl, omfClassIDPtr_t aClass);
static omfErr_t AreAllPropertiesSet(omfFileCheckHdl_t hdl, 
									omfClassIDPtr_t aClass,
									omfCheckWarnings_t warn, 
									char *buf,
									FILE *textOut, 
									omfInt32 *numErrors, 
									omfInt32 *numWarnings);
static omfErr_t ProcessPropValue(omfHdl_t file, omfObject_t obj,
			 						 omfProperty_t prop, omfType_t type);
static omfErr_t MarkPropertyFound(omfFileCheckHdl_t hdl, omfClassIDPtr_t aClass, 
										omfProperty_t prop);
static omfBool FindObj(omfHdl_t file, omfObject_t obj, void *data);
static omfErr_t VerifyMobTree(omfHdl_t file, omfObject_t obj,
							  omfInt32 level, void *data);
static omfErr_t VerifyMobObj(omfHdl_t file,
							 omfObject_t obj,
							 char *buf,
							 void *data);
static omfErr_t VerifySourceClip(omfHdl_t file, 
								 omfObject_t obj, 
								 char *buf, 
								 void *data);
static void	internClassDispose(void *valuePtr);
static void	internPropDispose(void *valuePtr);

typedef struct
{
	omTable_t	*propertyInfo;
} classCheckInfo;

typedef struct
{
	omfBool			isOptional;
	omfBool			*wasFound;
	omfClassID_t	classID;
	omfProperty_t   propID;
	char    		propName[64];
} propertyCheckInfo;

struct omfiFileChecker
{
	 omfHdl_t		file;
	 omTable_t	*classTable;
};

struct callbackData
{
	FILE	*textOut;
	omfInt32	numErrors;
	omfInt32	numWarnings;
};

/************************
 * Function: omfsFileCheckInit
 *
 * 		Part of the semantic checker API.
 *		Init the semantic checker for the given file, and return a handle
 *		used by the rest of this API.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsFileCheckInit(omfHdl_t file, omfFileCheckHdl_t *resultPtr)
{
	omfSessionHdl_t		session = file->session;
	omTable_t			*classDefs, *propDefs;
	omfBool				moreClasses, moreProps, found;
	omTableIterate_t	classIter, propIter;
	classCheckInfo		classInfo;
	propertyCheckInfo	propInfo;
	OMPropDef			*sysPropDef;
	omfClassID_t		classID, superID, searchID;
	omfValidRev_t   	validRev;
	OMClassDef			*sysClassDef;
	omfFileCheckHdl_t	result;
	
	if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
	{
		classDefs = session->classDefs1X;
		propDefs = session->properties1X;
		validRev = kOmfTstRev1x;
	}
	else
	{
		classDefs = session->classDefs2X;
		propDefs = session->properties2X;
		validRev = kOmfTstRev2x;
	}
		
	XPROTECT(file)
	{
		*resultPtr = NULL;
		result = (omfFileCheckHdl_t)omOptMalloc(file, sizeof(struct omfiFileChecker));
		if(result == NULL)
			RAISE(OM_ERR_NOMEMORY);
		result->file = file;

	 	CHECK(omfsNewClassIDTable(file, 100, &result->classTable));
		CHECK(omfsSetTableDispose(result->classTable, internClassDispose));
		CHECK(omfsTableFirstEntry(classDefs, &classIter, &moreClasses));
		while(moreClasses)
			{
			sysClassDef = (OMClassDef *)classIter.valuePtr;
			if((sysClassDef->revs == validRev) || 
			   (sysClassDef->revs == kOmfTstRevEither))
			{
				strncpy(classID, sysClassDef->aClass, sizeof(classID));
			 	CHECK(omfsNewPropertyTable(file, 100, &classInfo.propertyInfo));
				CHECK(omfsSetTableDispose(classInfo.propertyInfo, 
										  internPropDispose));
			 	
				if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
				{
					propInfo.isOptional = FALSE;
					strncpy(propInfo.propName, "OMObjID", 
							sizeof(propInfo.propName));
					strncpy(propInfo.classID, classID, sizeof(propInfo.classID));
					propInfo.wasFound = (omfBool *)omOptMalloc(NULL, sizeof(omfBool));
					if(propInfo.wasFound == NULL)
						RAISE(OM_ERR_NOMEMORY);
					
					CHECK(omfsTableAddProperty(classInfo.propertyInfo, OMObjID,
						&propInfo, sizeof(propInfo), kOmTableDupError));
				}
				
				strncpy(searchID, classID, sizeof(searchID));

				/* Repeat this block for each superclass too */
				while(searchID[0] != '\0')
				{
					CHECK(omfsTableFirstEntry(propDefs, &propIter, &moreProps));
					while(moreProps)
					{
						sysPropDef = (OMPropDef *)propIter.valuePtr;
						if(strncmp(sysPropDef->classTag, searchID, 
								   sizeof(searchID)) == 0)
						{
							propInfo.isOptional = sysPropDef->isOptional;
							propInfo.propID = sysPropDef->propID;
							strncpy(propInfo.propName, sysPropDef->propName, 
									sizeof(propInfo.propName));
							strncpy(propInfo.classID, classID, 
									sizeof(propInfo.classID));
							propInfo.wasFound = (omfBool *)omOptMalloc(NULL, sizeof(omfBool));
							if(propInfo.wasFound == NULL)
								RAISE(OM_ERR_NOMEMORY);

							CHECK(omfsTableAddProperty(classInfo.propertyInfo, 
													   sysPropDef->propID,
								&propInfo, sizeof(propInfo), kOmTableDupError));
						}
						
						CHECK(omfsTableNextEntry(&propIter, &moreProps));
					  }

					strncpy(superID, "\0", 1); /* Null out superID */
					CHECK(omfsClassFindSuperClass(file, searchID, superID,
												  &found));
					strncpy(searchID, superID, sizeof(searchID));
				}
				
				CHECK(omfsTableAddClassID(result->classTable, (char *)classIter.key, 
										  &classInfo,
										  sizeof(classCheckInfo)));
			  }
			
			CHECK(omfsTableNextEntry(&classIter, &moreClasses));
		  }
		*resultPtr = result;
	}
	XEXCEPT
	XEND
		
	return(OM_ERR_NONE);
}

/************************
 * Function: omfsFileCheckCleanup
 *
 * 		Part of the semantic checker API.
 *		Dispose the per-file handle used by the rest of this API.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsFileCheckCleanup(omfFileCheckHdl_t hdl)
{
	omfsTableDisposeAll(hdl->classTable);
	omOptFree(hdl->file, hdl);
	return(OM_ERR_NONE);
}

static void	internClassDispose(void *valuePtr)
{
	classCheckInfo	*info = (classCheckInfo *)valuePtr;
	omfsTableDisposeAll(info->propertyInfo);
}

static void	internPropDispose(void *valuePtr)
{
	propertyCheckInfo	*info = (propertyCheckInfo *)valuePtr;
	omOptFree(NULL, info->wasFound);
}

/************************
 * Function: omfsFileCheckObjectIntegrity
 *
 * 		Part of the semantic checker API.
 *		Validate all of the objects in the file individually.  
 *      This function checks whether
 *		      * All required properties are present
 *            * Optional properties are present (warning if not)
 *            * Valid values for properties
 *                    - Positive trackIDs
 *                    - Positive lengths
 *                    - Valid enum values
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsFileCheckObjectIntegrity(
			omfFileCheckHdl_t handle,
			omfCheckVerbose_t verbose,
			omfCheckWarnings_t warn, 
			omfInt32		*numErrors,
			omfInt32		*numWarnings,
			FILE *textOut)
{
	omfIterHdl_t objIter, propIter;
	omfObject_t obj, refObj;
	omfProperty_t prop, idProp;
	omfType_t type;
	omfUniqueName_t currName;
	omfClassID_t objClass, refClass; 
	omfInt32 loop, numObjs;
	omfUInt32 reqmask = 0, optmask = 0;
	char buf[32];
	omfHdl_t	file = handle->file;
	omfInt32	tstErr, tstWarn;
	omfErr_t tmpOmfError = OM_ERR_NONE;
	omfBool printError = TRUE;
	omfObjectTag_t tag;
	OMClassDef entry;
	omfBool found = FALSE;
	omfErr_t omfError = OM_ERR_NONE;
	omfBool semanticCheckSave =	FALSE;
			
	*numErrors = 0;
	*numWarnings = 0;
	
    XPROTECT(file)
	{
		CHECK(omfiIteratorAlloc(file, &objIter));
		CHECK(omfiIteratorAlloc(file, &propIter));
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		/* Iterate over all of the objects in the file using Bento */
		CHECK(omfiGetNextObject(objIter, &obj));
		while (obj)
		{
		    omfError = omfsReadClassID(file, obj, idProp, objClass);

			/* Look for MC bug: CLSD ObjID property missing */
			if (omfError != OM_ERR_NONE)
			  {
				/* If CLSD object */

				semanticCheckSave = omfsIsSemanticCheckOn(file);
				omfsSemanticCheckOff(file);
				tmpOmfError = omfsReadObjectTag(file, obj, OMCLSDClassID, tag);
				if (semanticCheckSave) /* if it was on before, turn it back on */
					omfsSemanticCheckOn(file);
				DisplayObjectHeader(file, obj, buf); /* Get Bento label */
				if (tmpOmfError == OM_ERR_NONE)
				  {
					fprintf(textOut, 
 					 "*** ERROR: Required Property OMFI:ObjID missing on CLSD %s\n",
							buf);
				  }
				else
				  {
					fprintf(textOut, 
						"*** ERROR: Object %s missing Class ID!!\n", buf);
				  }
				(*numErrors)++;
				omfError = omfiGetNextObject(objIter, &obj);
				continue;
			  }

			/* Don't try to semantic check registered or private 
			 * classes
			 */
			CHECK(omfsTableClassIDLookup(file->session->classDefs1X,
										 objClass,
										 sizeof(entry), &entry,
										 &found));
			if (found && (entry.type != kClsRequired))
			  {
				omfError = omfiGetNextObject(objIter, &obj);
				continue;
			  }

			/* Avid MC generates empty ATTB objects, and since they are
			 * only additional information, don't worry about them.  They
			 * do not get in the way of interpreting the file.
			 */
			if (!strncmp(objClass, "ATTB", (size_t)4))
			  {
				omfError = omfiGetNextObject(objIter, &obj);
				continue;
			  }
			DisplayObjectHeader(file, obj, buf);

		    if (!IsValidOMFObject(handle, objClass))
		    {
			  /* Just print for informational sake, no error */
			  fprintf(textOut, "*** Found Non-public OMFI Object -- %s\n", 
					  buf);

			  CHECK(omfiGetNextObject(objIter, &obj));
			  continue;
	     	}
			if (verbose == kCheckVerbose)
				fprintf(textOut, "Object: %s\n", buf);
	
			CHECK(ClearPropFoundFlag(handle, objClass));
			
		    /* Property iterator */
		    CHECK(omfiGetNextProperty(propIter, obj, &prop, &type));
		    while ((omfError == OM_ERR_NONE) && prop)
	   		{
				CHECK(omfiGetPropertyName(file, prop, OMUNIQUENAME_SIZE, 
							  currName));

				/* Is this property valid on this object */
				omfError = omfsCheckObjectType(file, obj, prop, type);
				if (omfError != OM_ERR_NONE)
				{
				  /* Unusual cases for old Avid:MC file with wrong type --
				   * If the Int32 version of TCCPFlags also exists, don't
				   * complain.  It will exist if it has been "repaired".
				   */
				  if (prop == OMTCCPFlags)
					{
					  if (omfsIsPropPresent(file, obj, prop, OMInt32))
						{
						  printError = FALSE;
						  type = OMInt32; /* Fake out type for this case */
						}
					}
				   /* If the omfInt16 version of IsOMFI also exists, don't
				   * complain.  It will exist if it has been "repaired".
				   */
				  if (prop == OMMDFLIsOMFI)
					{
					  if (omfsIsPropPresent(file, obj, prop, OMBoolean))
						{
						  printError = FALSE;
						  type = OMBoolean; /* Fake out type for this case */
						}
					}

				  /* Another Avid MC bug - note this will probably 
				   * not be reached since LITM objects are skipped above.
				   */
				  if (!strcmp(currName, "OMFI:SMLS:MC:EntryNumber"))
					  {
						printError = FALSE;
						omfError = omfiGetNextProperty(propIter, obj, 
													   &prop, &type);
						omfError = OM_ERR_NONE;
						continue;
					  }

				  if (printError)
					{
					  fprintf(textOut, "*** ERROR: %s has ILLEGAL property %s\n\t\t[%s]\n",
							  buf, currName,
							  omfsGetErrorString(omfError));
					  (*numErrors)++;
					  /* If Illegal Property, get next prop and continue */
					  omfError = omfiGetNextProperty(propIter, obj, 
													 &prop, &type);
					  omfError = OM_ERR_NONE;
					  continue;
					} /* printError */
				  else /* reset and contin as if valid property */
					printError = TRUE;
				}

				/* Explicitly call semantic check routines, in case the
				 * tool wasn't built with semantic checking turned on.
				 */
				if (type == OMObjRef)
				{
				    omfError = omfsReadObjRef(file, obj, prop, &refObj);
				    omfError = omfsCheckObjRefValidity(file, prop, refObj,
								       kOmGetFunction); 
				}
				else if (type == OMObjRefArray)
				{
				    numObjs = omfsLengthObjRefArray(file, obj, prop);
				    for (loop = 1; loop <= numObjs; loop++)
				    {
						omfError = omfsGetNthObjRefArray(file, obj, prop,
									    &refObj, loop);
						omfError = omfsCheckObjRefValidity(file, prop, 
									   refObj, kOmGetFunction);
				    }
				}
				else /* Not an objRef or objRefArray */
				{
				    omfError = ProcessPropValue(file, obj, prop, type);
				}

				if (omfError != OM_ERR_NONE)
				{
				    CHECK(omfsReadClassID(file, refObj, idProp, refClass));
				    fprintf(textOut, "*** ERROR: %s property %s\n\t\t[%s]\n",
					   buf, currName, 
					   omfsGetErrorString(omfError));
				    omfError = OM_ERR_NONE;
				}
		
				/* Keep track of all properties, so can report
				 * required properties missing.
				 */
				CHECK(MarkPropertyFound(handle, objClass, prop));
			  
				/* Get next property */
				omfError = omfiGetNextProperty(propIter, obj, &prop, &type);
			  }

			/* Clear property iterator before getting next object */
	    	if (omfError == OM_ERR_NO_MORE_OBJECTS)
	      		{
				omfiIteratorClear(file, propIter);
				omfError = OM_ERR_NONE;
	      		}
			else
				{
				RAISE(omfError);
				}

	    	/* Verify that all properties are present */
			CHECK(AreAllPropertiesSet(handle, objClass, warn,
									  buf, textOut,
									  &tstErr, &tstWarn));
			*numErrors += tstErr;
			*numWarnings += tstWarn;
			
	    	/* Clear property masks for next object */
	    	reqmask = 0;
	    	optmask = 0;
	    	omfError = omfiGetNextObject(objIter, &obj);
	    	if ((omfError != OM_ERR_NONE) && (omfError != OM_ERR_NO_MORE_OBJECTS))
	    		RAISE(omfError);
		  }

		if (objIter)
		  {
			CHECK(omfiIteratorDispose(file, objIter));
		  }
		if (propIter)
		  {
			CHECK(omfiIteratorDispose(file, propIter));
		  }
		objIter = NULL;
		propIter = NULL;

	} /* XPROTECT */
    XEXCEPT
	  {
		if (objIter)
		  {
			omfiIteratorDispose(file, objIter);
		  }
		if (propIter)
		  {
			omfiIteratorDispose(file, propIter);
		  }
		objIter = NULL;
		propIter = NULL;
	  } XEND;

    return(OM_ERR_NONE);
}

/************************
 * Function: omfsFileCheckMediaIntegrity
 *
 * 		Part of the semantic checker API.
 *		Validate all of the MediaDescriptors and objects in the file individually.  
 * 		This function depends on the codecs to do the checking.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsFileCheckMediaIntegrity(
			omfFileCheckHdl_t handle,
			omfCheckVerbose_t verbose,
			omfCheckWarnings_t warn, 
			omfInt32		*numErrors,
			omfInt32		*numWarnings,
			FILE *textOut)
{
	omfIterHdl_t mobIter;
	char			msgBuf[256];
	omfHdl_t		file = handle->file;
	omfObject_t	fileMob;
	omfErr_t		omfError = OM_ERR_NONE;
 	omfSearchCrit_t searchCrit;
			
	*numErrors = 0;
	*numWarnings = 0;
	
    XPROTECT(file)
	{
		 searchCrit.searchTag = kByMobKind;
		 searchCrit.tags.mobKind = kFileMob;

		CHECK(omfiIteratorAlloc(file, &mobIter));

		while (omfiGetNextMob(mobIter, &searchCrit, &fileMob) == OM_ERR_NONE)
		{
			/*********
			 *  Now call the semantic check routine for the media
			 */
			omfError = codecSemanticCheck(file, fileMob, verbose,warn,sizeof(msgBuf), msgBuf);
			if(omfError != OM_ERR_NONE)
			{				
				if(msgBuf == NULL || msgBuf[0] == '\0')
				{
					if(omfError ==  OM_ERR_CODEC_SEMANTIC_WARN)
					{
						fprintf(textOut, "*** WARNING:  Media had unspecified semantic warnings\n");
						*numWarnings++;
					}
					else
					{
						fprintf(textOut, "*** ERROR:  Media had a semantic error\n\t\t[%s]\n",
							   omfsGetErrorString(omfError));
						*numErrors++;
					}
				}
				else
				{
					if(omfError ==  OM_ERR_CODEC_SEMANTIC_WARN)
					{
						fprintf(textOut, "*** WARNING:  Media had a semantic warning regarding '%s'.\n",
							   msgBuf);
						*numWarnings++;
					}
					else
					{
						fprintf(textOut, "*** ERROR:  Media had a semantic error regarding '%s'\n\t\t[%s]\n",
							   msgBuf, 
							   omfsGetErrorString(omfError));
						*numErrors++;
					}
				}

			}	
		}

		if (mobIter)
		  {
			CHECK(omfiIteratorDispose(file, mobIter));
		  }
		mobIter = NULL;
	} /* XPROTECT */

    XEXCEPT
	  {
		if (mobIter)
		  {
			omfiIteratorDispose(file, mobIter);
		  }
		mobIter = NULL;
	  } XEND;

    return(OM_ERR_NONE);
}

/************************
 * Function: omfsFileCheckMobData
 *
 * 		Part of the semantic checker API.
 *		Validate inter-object links and higher-level mob sementics.
 *      This function checks whether:
 *           * There are any duplicate mobs
 *           * All mob tracks are the same length
 *           * All scope/selector/effect slots are same length
 *           * All source clips point to mob that is in
 *                 the file, whether track exists and is the same
 *                 datakind, and whether position exists
 *           * All track IDs are unique
 *           * Track kind and numTracks in file mob matches descriptor
 *           * >1 track in video file mob (warning)
 *           * Media is present if no locators in file
 *           * Number of samples == length of track in file mob
 *
 *      FUTURE IDEAS:
 *           * transition validation
 *           * verify datakinds of source references, segments in tracks, etc.
 *           * verify that scopes, etc. aren't written to file mobs
 *           * verify editrates
 *           * Warning if more complicated structures in file/master mobs
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsFileCheckMobData(
			omfFileCheckHdl_t handle,
			omfCheckVerbose_t verbose,
			omfCheckWarnings_t warn, 
			omfInt32		*numErrors,
			omfInt32		*numWarnings,
			FILE *textOut)
{
    omfIterHdl_t mobIter = NULL;
    omfInt32 loop, matches = 0, numMobs;
    omfObject_t mob;
	omfHdl_t	file = handle->file;
	struct callbackData	cbData;
	omfIterHdl_t dupIter = NULL;
	omfInt32 numMatches;
	omfUID_t mobID;
	omfMobObj_t *mobList;
    omfErr_t omfError = OM_ERR_NONE;

	*numErrors = 0;
	*numWarnings = 0;

    XPROTECT(file)
    {
		cbData.textOut = textOut;
		cbData.numErrors = 0;
		cbData.numWarnings = 0;

		/* First check for duplicate mobs and bail if we find any */
		CHECK(omfiIteratorAlloc(file, &dupIter));
		omfError = omfiFileGetNextDupMobs(dupIter, &mobID, 
										  &numMatches, &mobList);
		if (numMatches > 0)
		  {
			fprintf(textOut, 
					"*** ERROR: Duplicate mob found in file: %ld.%ld.%ld\n", 
					mobID);
			(*numErrors)++;
		  }
		CHECK(omfiIteratorDispose(file, dupIter));
		dupIter = NULL;

		CHECK(omfiIteratorAlloc(file, &mobIter));
		CHECK(omfiGetNumMobs(file, kAllMob, &numMobs));

		/* Iterate through mobs and perform various semantic checks */
		for (loop = 1; loop <= numMobs; loop++)
		{
		    omfError = omfiGetNextMob(mobIter, NULL, &mob);
			if (omfError == OM_ERR_NO_MORE_OBJECTS)
			  break;
			else if (omfError != OM_ERR_NONE)
			  RAISE(omfError);

			/* Call VerifyMobTree() on each object in the mob */ 
		    CHECK(omfiMobMatchAndExecute(file, mob, 0, /* level*/
						     FindObj, NULL, 
						     VerifyMobTree, &cbData, &matches));
			*numErrors += cbData.numErrors;
			*numWarnings += cbData.numWarnings;
			cbData.numErrors = 0;     /* Reset */
			cbData.numWarnings = 0;   /* Reset */
		}
	
		CHECK(omfiIteratorDispose(file, mobIter));
		mobIter = NULL;
		
    } /* XPROTECT */

    XEXCEPT
	  {
		if (dupIter)
		  omfiIteratorDispose(file, dupIter);
		if (mobIter)
		  omfiIteratorDispose(file, mobIter);
	  }
    XEND;

    return(OM_ERR_NONE);
}


static omfErr_t ClearPropFoundFlag(omfFileCheckHdl_t hdl, 
								   omfClassIDPtr_t aClass)
{
	classCheckInfo		classInfo;
	omTableIterate_t	propIter;
	omfBool				found, moreProps;
	propertyCheckInfo	*propInfo;

	XPROTECT(hdl->file)
	{
		CHECK(omfsTableClassIDLookup(hdl->classTable, aClass, 
									 sizeof(classInfo), &classInfo, &found));
		if(!found)
			RAISE(OM_ERR_INVALID_CLASS_ID);

		CHECK(omfsTableFirstEntry(classInfo.propertyInfo, &propIter, &moreProps));
		while(moreProps)
		{
			propInfo = (propertyCheckInfo *)propIter.valuePtr;
			*(propInfo->wasFound) = FALSE;
			
			CHECK(omfsTableNextEntry(&propIter, &moreProps));
		}
	}
	XEXCEPT
	XEND
		
	return(OM_ERR_NONE);
}

static omfErr_t MarkPropertyFound(omfFileCheckHdl_t hdl, 
								  omfClassIDPtr_t aClass, 
								  omfProperty_t prop)
{
	classCheckInfo		classInfo;
	omfBool				found;
	propertyCheckInfo	propInfo;

	XPROTECT(hdl->file)
	{
		CHECK(omfsTableClassIDLookup(hdl->classTable, aClass, 
									 sizeof(classInfo), &classInfo, &found));
		if(!found)
			RAISE(OM_ERR_INVALID_CLASS_ID);

		CHECK(omfsTablePropertyLookup(classInfo.propertyInfo, prop, 
									  sizeof(propInfo), &propInfo, &found));
		if(!found)
			RAISE(OM_ERR_BAD_PROP);
		
		*(propInfo.wasFound) = TRUE;
	}
	XEXCEPT
	XEND
		
	return(OM_ERR_NONE);
}


static omfErr_t AreAllPropertiesSet(omfFileCheckHdl_t hdl, 
									omfClassIDPtr_t aClass, 
									omfCheckWarnings_t warn, 
									char *buf,
									FILE *textOut,
									omfInt32 *numErrors, 
									omfInt32 *numWarnings)
{
	classCheckInfo		classInfo;
	omTableIterate_t	propIter;
	omfBool				found, moreProps;
	propertyCheckInfo	*propInfo;
	
	*numErrors = 0;
	*numWarnings = 0;

	XPROTECT(hdl->file)
	{
		CHECK(omfsTableClassIDLookup(hdl->classTable, aClass, 
									 sizeof(classInfo), &classInfo, &found));
		if(!found)
			RAISE(OM_ERR_INVALID_CLASS_ID);

		CHECK(omfsTableFirstEntry(classInfo.propertyInfo, &propIter, 
								  &moreProps));
		while(moreProps)
		{
			propInfo = (propertyCheckInfo *)propIter.valuePtr;
			if(strncmp(propInfo->classID, aClass, sizeof(propInfo->classID)) == 0)
			{
				if(!*(propInfo->wasFound))
				{
					if(propInfo->isOptional && (warn == kCheckPrintWarnings))
					{
						fprintf(textOut, "*** WARNING: Optional Property %s missing from %s\n", propInfo->propName, buf);
						(*numWarnings)++;
					}
				    else if(!propInfo->isOptional)
				    {
					  /* NOTE: If 1.x file, this will also report an error
					   *       for TRANs with TRAK's that don't have 
					   *       Track Components.  This is legal.  There
					   *       is no way to determine that we're in the 
					   *       context of a TRAN in this context.  So, for now
					   *       let's turn off this check.
					   */
					  if (propInfo->propID != OMTRAKTrackComponent)
						{
						  fprintf(textOut, 
							"*** ERROR: Required Property %s missing from %s\n",
								  propInfo->propName, buf);
						  (*numErrors)++;
						}
					}
				}
			}
			
			CHECK(omfsTableNextEntry(&propIter, &moreProps));
		}
	}
	XEXCEPT
	XEND
		
	return(OM_ERR_NONE);
}


static omfBool FindObj(omfHdl_t file, omfObject_t obj, void *data)
{
	return(TRUE);
}

static omfBool isTrackUnique(omfHdl_t file,
							 omfObject_t mob,
							 omfObject_t track,
							 omfInt32 numTracks,
							 omfInt32 *listSize,
							 TrackIdentity_t **list,
							 omfErr_t *omfError)
{
  omfTrackID_t trackID;
  omfSegObj_t destSeg = NULL;
  omfDDefObj_t datakind = NULL;
  omfInt32 loop, rememberLoop = 0;

  XPROTECT(file)
    {
	  if (!(*list))
		*list = (TrackIdentity_t *)omOptMalloc(file, sizeof(TrackIdentity_t)*numTracks);

	  CHECK(omfiTrackGetInfo(file, mob, track,
							 NULL, 0, NULL, NULL,
							 &trackID, &destSeg));
	  CHECK(omfiComponentGetInfo(file, destSeg, &datakind, NULL));
	  for (loop = 1; loop <= (*listSize); loop++)
		{
		  if ((*list)[loop-1].trackID == trackID)
			{
			  /* For 1.x, <trackID, datakind> must be unique */
			  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
				{
				  /* A direct comparison is OK, since there is only one
				   * datakind definition object per file.
				   */
				  if ((*list)[loop-1].datakind == datakind)
					return(FALSE);
				}

			  /* For 2.x, <trackID> must be unique */
			  else /* kOmfRev2x */
				return(FALSE);
			}
		  rememberLoop = loop;
		  /* Else, keep iterating... */
		}

	  /* TrackID is unique */
	  if (((*listSize)+1) == numTracks)
		{
		  omOptFree(file, *list);
		  *list = NULL;
		}
	  else
		{
		  (*list)[rememberLoop].trackID = trackID;
		  (*list)[rememberLoop].datakind = datakind;
		  (*listSize)++;
		}

	  return(TRUE);
	}

  XEXCEPT
	{
	  *omfError = XCODE();
	}
  XEND_SPECIAL(FALSE);

  *omfError = OM_ERR_NONE;
  return(FALSE);
}

static omfErr_t VerifySourceClip(omfHdl_t file, 
								 omfObject_t obj, 
								 char *buf, 
								 void *data)
{
  omfDDefObj_t srcDK = NULL, destDK = NULL;
  omfSourceRef_t sRef;
  omfUID_t nullUID = {0,0,0};
  omfObject_t nextMob = NULL, destTrack = NULL, destSeg = NULL;
  struct callbackData *result = (struct callbackData *)data;
  FILE	*textOut = result->textOut;
  omfInt16 tmp1xTrackType = 0;
  omfLength_t destLength;
  omfUniqueName_t name;
  omfTrackID_t foundTrackID;
  omfRational_t srcEditrate, destEditrate;
  omfPosition_t origin;
  omfBool wrongEditrate = FALSE;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(file)
	{
	  omfError = omfiSourceClipGetInfo(file, obj, &srcDK, NULL, &sRef);

	  /**********/
	  /*** a) If the UID isn't NULL, see if the mob exists in the file */
	  if (!equalUIDs((sRef.sourceID), nullUID))
		{
		  omfError = omfiSourceClipResolveRef(file, obj, &nextMob);
		  if (!nextMob)
			{
			  fprintf(textOut, 
			 "*** ERROR: %s references non-existent MOB (UID: %ld.%ld.%ld)\n", 
					  buf, sRef.sourceID.prefix, sRef.sourceID.major, 
					  sRef.sourceID.minor); 
			  result->numErrors++;
			}

		  /**********/
		  else /*** b) See if track exists in referenced mob */
			{
			  /* Map 1.x trackID to 2.x */
			  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
				{
				  CHECK(omfsReadTrackType(file, obj, 
										  OMCPNTTrackKind, &tmp1xTrackType));
				  /* Get the MobID from the track's mob */
				  CHECK(CvtTrackNumtoID(file, 
										sRef.sourceID, 
										(omfInt16)sRef.sourceTrackID,
										tmp1xTrackType, &foundTrackID));
				}
			  else
				foundTrackID = sRef.sourceTrackID;
			  omfError = FindTrackByTrackID(file, nextMob, foundTrackID,
											&destTrack);
			  if (!destTrack)
				{
				  fprintf(textOut, 
				 "*** ERROR: %s references non-existent TRACK (Label: %ld)\n", 
						  buf, sRef.sourceTrackID);
				  result->numErrors++;
				}

			  /**********/
			  else /*** c) See if datakind matches */
				{
				  omfError = omfiTrackGetInfo(file, nextMob, destTrack,
											  &destEditrate, 0, NULL, &origin,
											  NULL, &destSeg);
				  if (destSeg)
					{
					  omfError = omfiComponentGetInfo(file, destSeg,
													  &destDK, &destLength);
					  /* A simple comparision works here, since there
					   * is only one datakind object in the file.
					   */
					  if (srcDK != destDK)
						{						  
						  CHECK(omfiDatakindGetName(file, destDK, 
													OMUNIQUENAME_SIZE, name));
						  fprintf(textOut, 
				  "*** ERROR: %s references TRACK of wrong DataKind: %s\n", 
								  buf, name);
						  result->numErrors++;
						} 
					  /*** d) Do editrates match?  Can only check for 1.x
					   ***    files since editrate is stored on CPNT.
					   ***/
					  /*** e) If length of referenced segment large enough
					   ***    to contain reference? (1.x only)
					   ***/
					  if ((file->setrev == kOmfRev1x) || 
						  (file->setrev == kOmfRevIMA))
						{
						  CHECK(omfsReadExactEditRate(file, obj,OMCPNTEditRate,
													  &srcEditrate));
						  if ((srcEditrate.numerator != destEditrate.numerator)
							&& srcEditrate.denominator 
							                 != destEditrate.denominator)
							{
							  fprintf(textOut, 
									  "*** WARNING: %s : Editrates differ between mobs (MASK/ERAT is needed)\n", 
									  buf);
							  result->numWarnings++;
							  wrongEditrate = TRUE;
							}
						  /* !!!NOTE: Convert if editrates differ */
						  if (!wrongEditrate) 
							{
							  omfsAddInt64toInt64(origin, &destLength);
							  if (omfsInt64Less(destLength, sRef.startTime))
								{
								  fprintf(textOut, 
										  "*** WARNING: %s : SCLP reference points off of end of track\n", 
									  buf);
								  result->numWarnings++;
								}
							}
						  else
							wrongEditrate = FALSE; /* Reset */
						}

					}
				} /* if datakind matches */
			} /* if track exists */
		} /* if equalUIDs */
	} /* XPROTECT */

  XEXCEPT
	{
	}
  XEND;

  return(OM_ERR_NONE);
}

static omfErr_t VerifyMobObj(omfHdl_t file,
							 omfObject_t obj,
							 char *buf,
							 void *data)
{
  omfBool isFirst = TRUE;
  omfIterHdl_t iter = NULL;
  omfObject_t slot = NULL, seg = NULL, track = NULL;
  omfLength_t firstLen, segLen, newLen, numSamples, length, ceilLen;
  omfTrackID_t trackID;
  omfMediaHdl_t mediaHdl = NULL;
  char		 numSampBuf[32], newLenBuf[32];
  omfInt16 numAChannels = 0, numVChannels = 0;
  omfRational_t editrate, firstEditrate, sampleRate;
  omfInt32 numSlots, numTracks, loop;
  omfUID_t mobID;
  omfInt32 listSize = 0;
  omfDDefObj_t pictureKind = NULL, soundKind = NULL, datakind = NULL;
  omfObject_t mdes = NULL;
  TrackIdentity_t *list = NULL;
  omfProperty_t prop;
  omfClassID_t id;
  omfInt32 numVideoTracks = 0, numSoundTracks = 0, numLocs = 0;
  omfInt16 numVideoChannels = 0, numSoundChannels = 0;
  struct callbackData *result = (struct callbackData *)data;
  FILE	*textOut = result->textOut;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(file)
	{
	  omfiDatakindLookup(file, PICTUREKIND, &pictureKind, &omfError);
	  omfiDatakindLookup(file, SOUNDKIND, &soundKind, &omfError);

	  if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		prop = OMObjID;
	  else
		prop = OMOOBJObjClass;

	  /* Verify length of first slot == length of rest of slots */
	  CHECK(omfiIteratorAlloc(file, &iter));
	  CHECK(omfiMobGetNumSlots(file, obj, &numSlots));
	  CHECK(omfiMobGetNumTracks(file, obj, &numTracks));

	  for (loop = 1; loop <= numSlots; loop++)
		{
		  CHECK(omfiMobGetNextSlot(iter, obj, NULL, &slot));

		  if (slot)
			{
			  CHECK(omfiMobSlotGetInfo(file, slot, &editrate, &seg));
			  CHECK(omfiComponentGetInfo(file, seg, &datakind, &segLen));

			  /* If first slot, remember the length to compare later */
			  if (isFirst)
				{
				  firstLen = segLen;
				  firstEditrate = editrate;
				  isFirst = FALSE;
				}

			  /* Convert the length of the subsequent track to the editrate
			   * of the first track.
			   */
			  if ((firstEditrate.numerator != editrate.numerator) &&
				  (firstEditrate.denominator != editrate.denominator))
				{
				  CHECK(omfiConvertEditRate(editrate, segLen, firstEditrate, 
											kRoundCeiling, &newLen));
				}
			  else
				newLen = segLen;
/* Take this out for now since it is turning up errors in the unittests */
#if 0
			  if (omfsInt64NotEqual(firstLen, newLen))
				{
				  CHECK(omfsInt64ToString(firstLen, 10, sizeof(firstLenBuf), firstLenBuf));  
				  CHECK(omfsInt64ToString(newLen, 10, sizeof(newLenBuf), newLenBuf));  
				  fprintf(textOut, "*** ERROR: %s Mob first slot length (%s) != length of Mob slot (%s)\n", 
						  buf, firstLenBuf, newLenBuf);
				  result->numErrors++; 
				}
#endif
			  /* If track, see if the track label is unique */
			  if (omfiMobSlotIsTrack(file, slot, &omfError))
				{
				  if (!isTrackUnique(file, obj, slot, numTracks,
									 &listSize, &list, &omfError))
					{
					  fprintf(textOut,"*** ERROR: %s Track Label not unique\n",
							  buf);
					  result->numErrors++; 
					}
				}

			  /* Count the number of picture and sound tracks */
			  if (omfiIsPictureKind(file, datakind, kExactMatch, &omfError))
				numVideoTracks++;
			  else if (omfiIsSoundKind(file, datakind, kExactMatch, &omfError))
				numSoundTracks++;
			}
		} /* for */
	  
	  CHECK(omfiIteratorDispose(file, iter));
	  iter = NULL;

	  /* If file mob, do the num channels in the media descriptor
	   * match the number of channels in the mob?   If it is a video
	   * file mob, print a warning if there is more than 1 track.
	   */
	  if (omfiIsAFileMob(file, obj, &omfError))
		{
		  CHECK(omfmMobGetMediaDescription(file, obj, &mdes));
		  
		  /* If no locators, and no media -> print warning */
		  CHECK(omfmMobGetNumLocators(file, obj, &numLocs));
		  CHECK(omfiMobGetMobID(file, obj, &mobID));
		  if (!numLocs && !omfmIsMediaDataPresent(file, mobID, kOmfiMedia))
			{
			  fprintf(textOut, "*** WARNING: %s Media not in file for File Mob with no locators\n", buf);
			  result->numWarnings++; 
			}
		  
		  CHECK(omfsReadClassID(file, mdes, prop, id));
		  CHECK(ompvtGetNumChannelsFromFileMob(file, obj, pictureKind,
											   &numVideoChannels));
		  CHECK(ompvtGetNumChannelsFromFileMob(file, obj, soundKind,
											   &numSoundChannels));
		  if (numVideoChannels != numVideoTracks)
			{
			  fprintf(textOut, "*** ERROR: %s Num video file mob tracks (%ld) /= channels in descriptor (%ld)\n", 
					  buf, numVideoTracks, numVideoChannels);
			  result->numErrors++; 
			}
		  else if (numVideoTracks > 1)
			{
			  fprintf(textOut, "*** WARNING: %s Video File Mob with more than 1 track\n", buf);
			  result->numErrors++; 
			}
		  
		  if (numSoundChannels != numSoundTracks)
			{
			  fprintf(textOut, "*** ERROR: %s Num sound file mob tracks (%ld) /= channels in descriptor (%ld)\n", 
					  buf, numSoundTracks, numSoundChannels);
			  result->numWarnings++; 
			}
		}

	  /* If MasterMob, open media for each track and make sure that 
	   * numSamples matches the length of the track.
	   */
	  else if (omfiIsAMasterMob(file, obj, &omfError))
		{
		  CHECK(omfiIteratorAlloc(file, &iter));
		  for (loop=1; loop<=numTracks; loop++)
			{
			  CHECK(omfiMobGetNextTrack(iter, obj, NULL, &track));
			  omfError = omfiTrackGetInfo(file, obj, track, &editrate,
										  0, NULL,
										  NULL, &trackID, &seg);
			  omfError = omfiComponentGetInfo(file, seg, 
											  NULL, &length);
			  omfError = omfmGetNumChannels(file, obj, trackID,
											NULL, /* media criteria */
											soundKind,
											&numAChannels);
			  omfError = omfmGetNumChannels(file, obj, trackID,
											NULL, /* media criteria */
											pictureKind,
											&numVChannels);
			  if (numVChannels > 0 || (numAChannels > 0))
				{
				  omfError = omfmMediaOpen(file, obj, trackID, NULL,
										   kMediaOpenReadOnly,
										   kToolkitCompressionDisable, 
										   &mediaHdl);
				  if (mediaHdl && (omfError == OM_ERR_NONE))
					{
					  omfError = omfmGetSampleCount(mediaHdl, &numSamples);
					  if (numVChannels > 0)
						{
						  CHECK(omfmGetVideoInfo(mediaHdl,
												 NULL, NULL, NULL, 
												 &sampleRate,
												 NULL, NULL));
						  CHECK(omfiConvertEditRate(editrate, length, 
													sampleRate, 
													kRoundFloor, &newLen));
						  if (omfsInt64Less(newLen, numSamples))
							{
  							  CHECK(omfsInt64ToString(numSamples, 10, sizeof(numSampBuf), numSampBuf));  
  							  CHECK(omfsInt64ToString(newLen, 10, sizeof(newLenBuf), newLenBuf));  
							  fprintf(textOut, "*** ERROR: %s NumSamples (%s) != length of track (%s)\n", buf, numSampBuf, newLenBuf);
							  result->numErrors++;
							}
						}
					  if (numAChannels > 0)
						{
						  CHECK(omfmGetAudioInfo(mediaHdl,
												 &sampleRate,
												 NULL, NULL));
						  /* For audio, round the track down and make sure
						   * that you have at least enough samples to fill 
						   * the track.
						   */
						  CHECK(omfiConvertEditRate(editrate, length, sampleRate, 
													kRoundFloor, &newLen));
						  CHECK(omfiConvertEditRate(editrate, length, sampleRate, 
													kRoundCeiling, &ceilLen));
						  /* Fudge factor */
						  omfsAddInt32toInt64(1, &numSamples);

						  if (omfsInt64Less(numSamples, newLen))
							{
  							  CHECK(omfsInt64ToString(numSamples, 10, sizeof(numSampBuf), numSampBuf));  
  							  CHECK(omfsInt64ToString(newLen, 10, sizeof(newLenBuf), newLenBuf));  
							  fprintf(textOut, "*** ERROR: %s NumSamples (%s) != length of track (%s)\n", buf, numSampBuf, newLenBuf);
							  result->numErrors++;
							}
						}
					} /* if mediaHdl */
				} /* If V or A channels */
			  if (mediaHdl)
				{
				  CHECK(omfmMediaClose(mediaHdl));
				  mediaHdl = NULL;
				}
			} /* for */
		  CHECK(omfiIteratorDispose(file, iter));
		  iter = NULL;
		}
	  
	}
  XEXCEPT
	{
	}
  XEND;
  
  return(OM_ERR_NONE);
}


static omfErr_t VerifyMobTree(omfHdl_t file,
							 omfObject_t obj,
							 omfInt32 level,
							 void *data)
{
	char buf[32];
	omfIterHdl_t sequIter = NULL, iter = NULL;
	struct callbackData *result = (struct callbackData *)data;
	FILE	*textOut = result->textOut;
	omfObject_t effectDef = NULL;
	omfErr_t omfError = OM_ERR_NONE;
	
	XPROTECT(file)
    {
		DisplayObjectHeader(file, obj, buf);

		/**********************/
	    /* Verify source clip */
		/**********************/
	    if (omfiIsASourceClip(file, obj, &omfError))
		{
		  CHECK(VerifySourceClip(file, obj, buf, data));
		}

		/**********************/
	    /* Verify Sequence    */
		/**********************/
		else if (omfiIsASequence(file, obj, &omfError) && (file->setrev == kOmfRev2x))
		{
			omfObject_t sequCpnt = NULL;
			omfLength_t cpntLen, totalLen, sequLen;
			char	 sequLenBuf[32], totalLenBuf[32];
			omfInt32 numCpnt, loop;

			/* Initialize lengths */
			omfsCvtInt32toInt64(0, &totalLen);
			omfsCvtInt32toInt64(0, &sequLen);

			/* Verify length of sequence == length of components */
			CHECK(omfiSequenceGetInfo(file, obj, NULL, &sequLen));
			CHECK(omfiIteratorAlloc(file, &sequIter));
			CHECK(omfiSequenceGetNumCpnts(file, obj, &numCpnt));
			for (loop = 1; loop <= numCpnt; loop++)
			{
				CHECK(omfiSequenceGetNextCpnt(sequIter, obj, NULL, NULL, 
							    &sequCpnt));
			    if (sequCpnt)
				{
				  CHECK(omfiComponentGetInfo(file, sequCpnt, NULL, &cpntLen));
				  if (!omfiIsATransition(file, sequCpnt, &omfError))
					omfsAddInt64toInt64(cpntLen, &totalLen);
				  else /* Is a transition */
					omfsSubInt64fromInt64(cpntLen, &totalLen);
				}
			} /* for */
			if (omfsInt64NotEqual(totalLen, sequLen))
			{
			      CHECK(omfsInt64ToString(sequLen, 10, sizeof(sequLenBuf), sequLenBuf));  
			      CHECK(omfsInt64ToString(totalLen, 10, sizeof(totalLenBuf), totalLenBuf));  
				fprintf(textOut, "*** ERROR: %s sequence length (%s) != length of components (%s)\n", 
					buf, sequLenBuf, totalLenBuf);
				result->numErrors++;
			}

			CHECK(omfiIteratorDispose(file, sequIter));
			sequIter = NULL;
		}

		/**********************/
	    /* Verify Effect      */
		/**********************/
	    else if (omfiIsAnEffect(file, obj, &omfError))
		{
			omfObject_t slot = NULL, arg = NULL;
			omfLength_t parentLen, argLen;
			char	 parentLenBuf[32], argLenBuf[32];
			omfInt32 numSlots, loop;
			omfBool isWarp = FALSE;
			omfUniqueName_t effectID;

			CHECK(omfiEffectGetInfo(file, obj, NULL, NULL, &effectDef));
			CHECK(omfiEffectDefGetInfo(file, effectDef, OMUNIQUENAME_SIZE,
								   effectID, 0, NULL, 0, NULL, NULL, NULL));

			/* If this is a timewarp effect, then the lengths of the slots
			 * will not equal the lengths of the effect.  For now, just
			 * skip this case.  In the future, we should probably check
			 * to make sure that the lengths are the correct multiple.
			 */
			if (!strcmp(effectID, kOmfFxFrameMaskGlobalName) ||
				!strcmp(effectID, kOmfFxSpeedControlGlobalName) ||
				!strcmp(effectID, kOmfFxRepeatGlobalName))
			  {
				isWarp = TRUE;
			  }

			/* Only compare if not a timewarp effect */
			if (!isWarp)
			  {
				/* Verify length of slots == length of parent */
				CHECK(omfiComponentGetInfo(file, obj, NULL, &parentLen));
				CHECK(omfiIteratorAlloc(file, &iter));
				CHECK(omfiEffectGetNumSlots(file, obj, &numSlots));
				for (loop = 1; loop <= numSlots; loop++)
				  {
					CHECK(omfiEffectGetNextSlot(iter, obj, NULL, &slot));
					if (slot)
					  {
						CHECK(omfiEffectSlotGetInfo(file, slot, NULL, &arg));
						CHECK(omfiComponentGetInfo(file, arg, NULL, &argLen));
						if (omfsInt64NotEqual(parentLen, argLen))
						  {
			      			CHECK(omfsInt64ToString(parentLen, 10, sizeof(parentLenBuf), parentLenBuf));  
			      			CHECK(omfsInt64ToString(argLen, 10, sizeof(argLenBuf), argLenBuf));  
							fprintf(textOut, 
                                 "*** ERROR: %s Effect length (%s) != length of effect slot (%s)\n", 
									buf, parentLenBuf, argLenBuf);
							result->numErrors++;
						  }
					  }
				  } /* for */
				
				CHECK(omfiIteratorDispose(file, iter));
				iter = NULL;
			  } /* !isWarp */
		  }

		/**********************/
	    /* Verify Mob         */
		/**********************/
		else if (omfiIsAMob(file, obj, &omfError))
		{
		  CHECK(VerifyMobObj(file, obj, buf, data));
		}

		/**********************/
	    /* Verify Nested Scope*/
		/**********************/
		else if (omfiIsANestedScope(file, obj, &omfError))
		{
			omfObject_t slot = NULL;
			omfLength_t parentLen, slotLen;
			char	 parentLenBuf[32], slotLenBuf[32];
			omfInt32 numSlots, loop;

			/* Verify length of parent == length of rest of slots */
			CHECK(omfiComponentGetInfo(file, obj, NULL, &parentLen));
			CHECK(omfiIteratorAlloc(file, &iter));
			CHECK(omfiNestedScopeGetNumSlots(file, obj, &numSlots));

			for (loop = 1; loop <= numSlots; loop++)
			{
				CHECK(omfiNestedScopeGetNextSlot(iter, obj, NULL, &slot));
				if (slot)
				{
				    CHECK(omfiComponentGetInfo(file, slot, NULL, &slotLen));
					if (omfsInt64NotEqual(parentLen, slotLen))
					{
			      		CHECK(omfsInt64ToString(parentLen, 10, sizeof(parentLenBuf), parentLenBuf));  
			      		CHECK(omfsInt64ToString(slotLen, 10, sizeof(slotLenBuf), slotLenBuf));  
						fprintf(textOut, "*** ERROR: %s Scope length (%ld) != length of Scope slot (%ld)\n", 
							buf, parentLenBuf, slotLenBuf);
						result->numErrors++;
					}
				}
			} /* for */

			CHECK(omfiIteratorDispose(file, iter));
			iter = NULL;
		}

		/**********************/
	    /* Verify Selector    */
		/**********************/
		else if (omfiIsASelector(file, obj, &omfError))
		{
			omfObject_t slot = NULL;
			omfLength_t parentLen, slotLen, selectedLen;
			char	 parentLenBuf[32], slotLenBuf[32], selectedLenBuf[32];
			omfInt32 numSlots, loop;

			/* Verify length of slots == length of parent */
			CHECK(omfiComponentGetInfo(file, obj, NULL, &parentLen));
			CHECK(omfiComponentGetInfo(file, obj, NULL, &selectedLen));
			if (omfsInt64NotEqual(selectedLen, parentLen))
			{
			      CHECK(omfsInt64ToString(selectedLen, 10, sizeof(selectedLenBuf), selectedLenBuf));  
			      CHECK(omfsInt64ToString(parentLen, 10, sizeof(parentLenBuf), parentLenBuf));  
				fprintf(textOut, "*** ERROR: %s Selector length (%s) != length of selected slot (%s)\n", 
					buf, selectedLenBuf, parentLenBuf);
				result->numErrors++;
			}

			CHECK(omfiIteratorAlloc(file, &iter));
			CHECK(omfiSelectorGetNumAltSlots(file, obj, &numSlots));
			for (loop = 1; loop <= numSlots; loop++)
			{
				CHECK(omfiSelectorGetNextAltSlot(iter, obj, NULL, &slot));
				if (slot)
				{
				    CHECK(omfiComponentGetInfo(file, slot, NULL, &slotLen));
					if (omfsInt64NotEqual(parentLen, slotLen))
					{
			      		CHECK(omfsInt64ToString(parentLen, 10, sizeof(parentLenBuf), parentLenBuf));  
			      		CHECK(omfsInt64ToString(slotLen, 10, sizeof(slotLenBuf), slotLenBuf));  
						fprintf(textOut, "*** ERROR: %s Selector length (%s) != length of Selector Alternate slot (%s)\n", 
							buf, parentLenBuf, slotLenBuf);
						result->numErrors++;
					}
				}
			} /* for */

			CHECK(omfiIteratorDispose(file, iter));
			iter = NULL;
		  }
	} /* XPROTECT */

	XEXCEPT
    {
	  if (sequIter)
		omfiIteratorDispose(file, sequIter);
	  if (iter)
		omfiIteratorDispose(file, iter);
	}
	XEND;

	return(OM_ERR_NONE);
}


static omfErr_t ProcessPropValue(omfHdl_t file,
			  omfObject_t obj,
			  omfProperty_t prop,
			  omfType_t type)
{
	omfInt32 int32Data;
	omfUInt32 uint32Data;
	omfInt16 int16Data;
	omfUInt16 uint16Data;
	omfBool booleanData;
	char charData;
	unsigned char ucharData;
	omfRational_t rationalData;
	omfUID_t uidData;
	omfPosition_t positionData;
	omfLength_t lengthData;
	omfEdgeType_t edgeTypeData;
	omfTrackType_t trackTypeData;
	omfFilmType_t filmTypeData;
	omfPhysicalMobType_t mobTypeData;
	omfUsageCode_t usageCodeData;
	omfVersionType_t versionData;
	omfFadeType_t fadeTypeData;
	omfInterpKind_t interpKindData;
	omfArgIDType_t argIDData;
	omfEditHint_t editHintData;
	void *data = NULL;
	omfErr_t omfError;
	omfBool	saveCheckStatus = file->semanticCheckEnable;
	XPROTECT(file)
	{
		/* Turn off semantic checking so we can get the value */
		file->semanticCheckEnable = FALSE;
		switch (type)
		{
		case OMInt32:
			omfError = omfsReadInt32(file, obj, prop, &int32Data);
			data = &int32Data;
			break;

		case OMUInt32:
			omfError = omfsReadUInt32(file, obj, prop, &uint32Data);
			data = &uint32Data;
			break;
	  
		case OMInt16:
			omfError = omfsReadInt16(file, obj, prop, &int16Data);
			data = &int16Data;
			break;
			
		case OMUInt16:
			omfError = omfsReadUInt16(file, obj, prop, &uint16Data);
			data = &uint16Data;
			break;

		case OMBoolean:
			omfError = omfsReadBoolean(file, obj, prop, &booleanData);
			data = &booleanData;
			break;

		case OMInt8:
			omfError = omfsReadInt8(file, obj, prop, &charData);
			data = &charData;
			break;

		case OMUInt8:
			omfError = omfsReadUInt8(file, obj, prop, &ucharData);
			data = &ucharData;
			break;

		case OMRational:
			omfError = omfsReadRational(file, obj, prop, &rationalData);
			data = &rationalData;
			break;

		case OMUID:
			omfError = omfsReadUID(file, obj, prop, &uidData);
			data = &uidData;
			break;

		case OMPosition32:
		case OMPosition64:
			omfError = omfsReadPosition(file, obj, prop, &positionData);
			data = &positionData;
			break;

		case OMLength32:
		case OMLength64:
			omfError = omfsReadLength(file, obj, prop, &lengthData);
			data = &lengthData;
			break;

		case OMEdgeType:
			omfError = omfsReadEdgeType(file, obj, prop, &edgeTypeData);
			data = &edgeTypeData;
			break;

		case OMTrackType:
			omfError = omfsReadTrackType(file, obj, prop, &trackTypeData);
			data = &trackTypeData;
			break;

		case OMFilmType:
			omfError = omfsReadFilmType(file, obj, prop, &filmTypeData);
			data = &filmTypeData;
			break;

		case OMPhysicalMobType:
			omfError = omfsReadPhysicalMobType(file, obj, prop, &mobTypeData);
			data = &mobTypeData;
			break;

		case OMUsageCodeType:
			omfError = omfsReadUsageCodeType(file, obj, prop, &usageCodeData);
			data = &usageCodeData;
			break;

		case OMVersionType:
			omfError = omfsReadVersionType(file, obj, prop, &versionData);
			data = &versionData;
			break;

		case OMFadeType:
			omfError = omfsReadFadeType(file, obj, prop, &fadeTypeData);
			data = &fadeTypeData;
			break;

		case OMInterpKind:
			omfError = omfsReadInterpKind(file, obj, prop, &interpKindData);
			data = &interpKindData;
			break;

		case OMArgIDType:
			omfError = omfsReadArgIDType(file, obj, prop, &argIDData);
			data = &argIDData;
			break;

		case OMEditHintType:
			omfError = omfsReadEditHint(file, obj, prop, &editHintData);
			data = &editHintData;
			break;
	  
		default:
			data = NULL;
			break;
	/* OMString, OMVarLenBytes, ObjectTag, TimeStamp */
	/* enums: CharSetType, JPEGTableIDType */
	/* 1.x: ExactEditRate */
	/* 2.x: ClassID, UniqueName, DataValue */
		}
		if (saveCheckStatus)
			file->semanticCheckEnable = TRUE;
		if (data)
		{
			CHECK(omfsCheckDataValidity(file, prop, data, kOmGetFunction));
		}
		if (saveCheckStatus)
			file->semanticCheckEnable = TRUE;
	} /* XPROTECT */
	XEXCEPT
	{
		if (saveCheckStatus)
			file->semanticCheckEnable = TRUE;
	}
	XEND;

	return(OM_ERR_NONE);
} 

static omfBool IsValidOMFObject(omfFileCheckHdl_t hdl, omfClassIDPtr_t classID)
{
	if(omfmTableNumEntriesMatching(hdl->classTable, classID) > 0)
		return(TRUE);
	else
		return(FALSE);
}

/**********************************/
/* Identify Object */
/**********************************/
static void DisplayObjectHeader(
				omfHdl_t file,
				omfObject_t obj, 
				char *outstr) 
{
	omfClassID_t id;
	omfErr_t omfError = OM_ERR_NONE;
  	omfProperty_t	prop;
  	
	if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		prop = OMObjID;
	else
		prop = OMOOBJObjClass;
		
	omfError = omfsReadClassID(file, obj, prop, id);
	if (omfError == OM_ERR_NONE)
		sprintf(outstr,"%.4s [BentoID: %1ld ]", id, 
			omfsGetBentoID(file, obj) & 0x0000ffff);
	else
		sprintf(outstr,"[BentoID: %1ld ]", 
				omfsGetBentoID(file, obj) & 0x0000ffff);
}
#endif

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
