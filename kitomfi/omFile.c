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
 * Name: 	omFile.c
 *
 * Function: The API file for session and file operations.
 *
 * Audience: All clients.
 *
 * General error codes returned:
 * 	OM_ERR_BENTO_PROBLEM
 *			Bento returned an error, check BentoErrorNumber.
 *		OM_ERR_NOMEMORY
 *			Ran out of memory during initialization.
 */

/* A 75-column ruler
         111111111122222222223333333333444444444455555555556666666666777777
123456789012345678901234567890123456789012345678901234567890123456789012345
*/
#include "masterhd.h"
#include <string.h>

#if PORT_SYS_MAC && !LW_MAC
#include <CarbonCore/Files.h>
#include <CarbonCore/Resources.h>
#endif

#include "omPublic.h"
#include "omErr.h"
#include "omPvt.h"

#if OMFI_MACSF_STREAM || OMFI_MACFSSPEC_STREAM
#include "OMFMacIO.h"
#else
#include "OMFAnsiC.h"
#endif



#define DEFAULT_NUM_PUBLIC_PROPS		300
#define DEFAULT_NUM_PRIVATE_PROPS	100
#define DEFAULT_NUM_PUBLIC_TYPES		100
#define DEFAULT_NUM_PRIVATE_TYPES	100
#define DEFAULT_NUM_CLASS_DEFS		100
#define DEFAULT_NUM_DATAKIND_DEFS	100
#define DEFAULT_NUM_EFFECT_DEFS		100
#define DEFAULT_NUM_MOBS				1000

#define DEFAULT_STACK_TRACE_SIZE		2048L

/* Private function definitions */
omfErr_t InitFileHandle(omfSessionHdl_t session,
								omfFileFormat_t fmt, omfHdl_t *result);
static omfErr_t BuildPropIDCache(omfHdl_t file);
static omfErr_t BuildTypeIDCache(omfHdl_t file);
static omfInt32 GetNumDefs(omfHdl_t file, omfClassIDPtr_t tag);
static omfErr_t BuildDatakindCache(omfHdl_t file, omfInt32 defTableSize,
			omTable_t **dest);
static omfErr_t BuildEffectDefCache(omfHdl_t file, omfInt32 defTableSize,
			omTable_t **dest);
static omfErr_t BuildMediaCache(omfHdl_t file);
static omfErr_t omfsInternOpenFile(fileHandleType stream,
			omfSessionHdl_t session, CMContainerUseMode useMode,
			openType_t type, omfHdl_t *result);
static void MobDisposeMap1X(void *valuePtr);
static omfErr_t UpdateFileCLSD(omfHdl_t file);
static omfErr_t UpdateLocalCLSD(omfHdl_t file);
static omfErr_t UpdateLocalTypesProps(omfHdl_t file);
static omfErr_t AddIDENTObject(omfHdl_t file, omfProductIdentification_t *identPtr);

/************************
 * Function: omfsBeginSession
 *
 * 		Opens a an OMFI session, initializing read-only data used by
 *			the current application.  Should be paired (at the end of
 *			your application) with omfsEndSession().
 *
 * Argument Notes:
 *		NOTE: Replaced PREFIX with a pointer to a structure which contains the following:
 *			char		*companyName;
 *			char		*ProductName;
 *			char		*productVersionString;
 *			omfInt32	productID				 * Should be the prefix code assigned by the OMFI
 *												 * developer desk to your company.
 *			omfProductVersion_t	productVersion;
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsBeginSession(
			omfProductIdentification_t	*ident, 		/* IN - Open a session */
			omfSessionHdl_t				*sessionPtr)	/* OUT - and return its handle */
{
	omfSessionHdl_t sess;

	*sessionPtr = NULL;

	XPROTECT(NULL)
	{
		CHECK(omfsInit());
	
		sess = (omfSessionHdl_t) omOptMalloc(NULL, sizeof(struct omfiSession));
		if (sess == NULL)
			RAISE(OM_ERR_NOMEMORY);
	
		sess->BentoErrorRaised = 0;

		sess->ioFuncs.containerMetahandlerFunc = containerMetahandler;
		sess->ioFuncs.createRefConFunc = createRefConForMyHandlers;
		sess->ioFuncs.typedOpenFileFunc = omfsTypedOpenFile;
		sess->ioFuncs.typedOpenRawFileFunc = omfsTypedOpenRawFile;
	
		/*
		 * Initialize all fields in the same order as defined in the struct
		 */
		sess->topFile = NULL;
		if(ident != NULL)
			sess->prefix = ident->productID;
		else
			sess->prefix = -2;
		sess->types1X = NULL;
		sess->typeNames = NULL;
		sess->propertyNames = NULL;
		sess->properties1X = NULL;
		sess->types2X = NULL;
		sess->properties2X = NULL;
		sess->classDefs1X = NULL;
		sess->classDefs2X = NULL;
		sess->codecMDES = NULL;
		sess->codecID = NULL;
		sess->mediaLayerInitComplete = FALSE;
		if(ident != NULL)
		{
			sess->ident = (omfProductIdentification_t*) omOptMalloc(NULL, sizeof(omfProductIdentification_t));
			if (sess->ident == NULL)
				RAISE(OM_ERR_NOMEMORY);
			memcpy(sess->ident, ident, sizeof(omfProductIdentification_t));
		}
		else
			sess->ident = NULL;
		sess->BentoSession = CMStartSession(sessionRoutinesMetahandler, sess);
		if (sess->BentoErrorRaised)
		{
			RAISE(OM_ERR_BADSESSIONOPEN);
		}
		CMSetMetaHandler(sess->BentoSession, "OMFI", containerMetahandler);
		if (sess->BentoErrorRaised)
		{
			RAISE(OM_ERR_BADSESSIONOPEN);
		}
	
		CHECK(omfsNewTypeTable(NULL, DEFAULT_NUM_PUBLIC_TYPES,
											&sess->types1X));
		CHECK(omfsNewPropertyTable(NULL, DEFAULT_NUM_PUBLIC_PROPS,
											&sess->properties1X));
		CHECK(omfsNewTypeTable(NULL, DEFAULT_NUM_PUBLIC_TYPES,
											&sess->types2X));
		CHECK(omfsNewPropertyTable(NULL, DEFAULT_NUM_PUBLIC_PROPS,
											&sess->properties2X));
		CHECK(omfsNewClassIDTable(NULL, DEFAULT_NUM_CLASS_DEFS,
											&sess->classDefs1X));
		CHECK(omfsNewClassIDTable(NULL, DEFAULT_NUM_CLASS_DEFS,
											&sess->classDefs2X));
		CHECK(omfsNewStringTable(NULL, TRUE, NULL, DEFAULT_NUM_PUBLIC_TYPES,
										 &sess->typeNames));
		CHECK(omfsNewStringTable(NULL, TRUE, NULL, DEFAULT_NUM_PUBLIC_PROPS,
										 &sess->propertyNames));
	
		sess->cookie = SESSION_COOKIE;
		sess->nextDynamicType = OMLASTTYPE+1;
		sess->nextDynamicProp = OMLASTPROP+1;
		sess->codecID = NULL;
		sess->codecMDES = NULL;
		sess->openCB = NULL;
		sess->closeSessCB = NULL;
		
		/********************* Class definitions ***************************/
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassCPNT, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassCLIP, 
						   OMClassCPNT));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassATCP, 
						   OMClassCLIP));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassFILL, 
						   OMClassCLIP));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassECCP, 
						   OMClassCLIP));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassTCCP, 
						   OMClassCLIP));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassSCLP, 
						   OMClassCLIP));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassTRKR, 
						   OMClassCLIP));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassSEQU, 
						   OMClassCPNT));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassTRKG, 
						   OMClassCPNT));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassMOBJ, 
						   OMClassTRKG));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassTRAN, 
						   OMClassTRKG));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassSLCT, 
						   OMClassTRKG));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassWARP, 
						   OMClassTRKG));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassSPED, 
						   OMClassWARP));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassMASK, 
						   OMClassWARP));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassREPT, 
						   OMClassWARP));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassHEAD, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassCLSD, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassTRAK, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassATTR, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassATTB, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassMDES, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassMDFL, 
						   OMClassMDES));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassMDTP, 
						   OMClassMDES));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassMDFM, 
						   OMClassMDES));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassMDNG, 
						   OMClassMDES));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassDOSL, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassMACL, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassUNXL, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassTXTL, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassOOBJ, 
						   OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassHEAD, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassCLSD, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassDDEF, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassMOBJ, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassMSLT, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassCPNT, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassSEGM, 
						   OMClassCPNT));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassTRAN, 
						   OMClassCPNT));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassSEQU, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassFILL, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassSCLP, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassTCCP, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassECCP, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassEFFE, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassESLT, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassEDEF, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassCVAL, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassVVAL, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassCTLP, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassNEST, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassSREF, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassSLCT, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassCMOB, 
						   OMClassMOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassERAT, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassLOCR, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassMMOB, 
						   OMClassMOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassSMOB, 
						   OMClassMOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassMDFM, 
						   OMClassMDES));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassMGRP, 
						   OMClassSEGM));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassMDTP, 
						   OMClassMDES));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassTRKD, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassDOSL, 
						   OMClassLOCR));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassUNXL, 
						   OMClassLOCR));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassMACL, 
						   OMClassLOCR));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassTXTL, 
						   OMClassLOCR));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassWINL, 
						   OMClassLOCR));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassMDES, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassMDFL, 
						   OMClassMDES));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassMDAT, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassATTB, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassATTR, 
						   OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsPrivate, kOmfTstRev1x, OMClassMDAT, 
						   OMClassNone));
			
		/* New Video Format is Private in 1.0, and Public in 2.0 */
		CHECK(omfsNewClass(sess, kClsPrivate, kOmfTstRev1x, OMClassDIDD, 
						   OMClassMDFL));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassDIDD, 
						   OMClassMDFL));

		/* add the codec-related classes */
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassTIFF,
			OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassTIFF,
			OMClassMDAT));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRevEither, OMClassTIFD,
			OMClassMDFL));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassWAVE,
			OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassWAVE,
			OMClassMDAT));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRevEither, OMClassWAVD,
			OMClassMDFL));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassIDAT,
			OMClassMDAT));
		CHECK(omfsNewClass(sess, kClsPrivate, kOmfTstRev1x, OMClassIDAT,
			OMClassMDAT));
		CHECK(omfsNewClass(sess, kClsPrivate, kOmfTstRev1x, OMClassRGBA,
			OMClassDIDD));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassRGBA,
			OMClassDIDD));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev1x, OMClassAIFC,
			OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassAIFC,
			OMClassMDAT));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRevEither, OMClassAIFD,
			OMClassMDFL));
		CHECK(omfsNewClass(sess, kClsPrivate, kOmfTstRev1x, OMClassCDCI,
			OMClassDIDD));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassCDCI,
			OMClassDIDD));
		CHECK(omfsNewClass(sess, kClsPrivate, kOmfTstRev1x, OMClassJPEG,
			OMClassIDAT));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassJPEG,
			OMClassIDAT));
		CHECK(omfsNewClass(sess, kClsPrivate, kOmfTstRev1x, OMClassIDNT,
			OMClassNone));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassIDNT,
			OMClassOOBJ));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassNETL,
			OMClassLOCR));
		CHECK(omfsNewClass(sess, kClsRequired, kOmfTstRev2x, OMClassPDWN,
			OMClassSEGM));

		/********************* Type definitions ***************************/
		CHECK(omfsRegisterType(sess, OMNoType, kOmfTstRevEither,
					   "NoType", kNeverSwab));
		CHECK(omfsRegisterType(sess, OMAttrKind, kOmfTstRevEither,
					   "AttrKind", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMBoolean, kOmfTstRevEither,
					   "Boolean", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMInt8, kOmfTstRev1x,
					   "Char", kSwabIfNeeded));
		/* !!! Need to add an Int8 type & use where appropriate 
		 *	CHECK(omfsRegisterType(sess, OMInt8, kOmfTstRev2x,
		 *			   "Char"));	
		 */
		CHECK(omfsRegisterType(sess, OMCharSetType, kOmfTstRev1x,
					   "CharSetType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMEdgeType, kOmfTstRevEither,
					   "EdgeType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMExactEditRate, kOmfTstRev1x,
					   "ExactEditRate", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMFilmType, kOmfTstRevEither,
					   "FilmType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMJPEGTableIDType, kOmfTstRevEither,
					   "JPEGTableIDType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMMobIndex, kOmfTstRev1x,
					   "MobIndex", kNeverSwab));
		CHECK(omfsRegisterType(sess, OMObjRefArray, kOmfTstRevEither,
					   "ObjRefArray", kNeverSwab));
		CHECK(omfsRegisterType(sess, OMObjRef, kOmfTstRevEither,
					   "ObjRef", kNeverSwab));
		CHECK(omfsRegisterType(sess, OMObjectTag, kOmfTstRev1x,
					   "ObjectTag", kNeverSwab));
		CHECK(omfsRegisterType(sess, OMPhysicalMobType, kOmfTstRev1x,
					   "PhysicalMobType", kSwabIfNeeded));

		CHECK(omfsRegisterType(sess, OMInt16, kOmfTstRev1x,
					   "Short", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMInt32, kOmfTstRev1x,
					   "Long", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMInt64, kOmfTstRev1x,
					   "Int64", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMInt16, kOmfTstRev2x,
					   "Int16", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMInt32, kOmfTstRev2x,
					   "Int32", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMInt64, kOmfTstRev2x,
					   "Int64", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMPosition32, kOmfTstRev2x,
					   "Position32", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMLength32, kOmfTstRev2x,
					   "Length32", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMPosition64, kOmfTstRev2x,
					   "Position64", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMLength64, kOmfTstRev2x,
					   "Length64", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMUInt32, kOmfTstRev1x,
					   "Ulong", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMUInt16, kOmfTstRev1x,
					   "Ushort", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMUInt32, kOmfTstRev2x,
					   "UInt32", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMUInt16, kOmfTstRev2x,
					   "UInt16", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMUInt8, kOmfTstRev1x,
					   "Uchar", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMUInt8, kOmfTstRev2x,
					   "UInt8", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMInt8, kOmfTstRev2x,
					   "Int8", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMString, kOmfTstRevEither,
					   "String", kNeverSwab));
		CHECK(omfsRegisterType(sess, OMTimeStamp, kOmfTstRevEither,
					   "TimeStamp", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMTrackType, kOmfTstRev1x,
					   "TrackType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMUID, kOmfTstRevEither,
					   "UID", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMUsageCodeType, kOmfTstRev1x,
					   "UsageCodeType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMVarLenBytes, kOmfTstRev1x,
					   "VarLenBytes", kNeverSwab));
		CHECK(omfsRegisterType(sess, OMVersionType, kOmfTstRevEither,
					   "VersionType", kNeverSwab));
	
		/* New types for 2.0 */
		CHECK(omfsRegisterType(sess, OMClassID, kOmfTstRev2x,
					   "ClassID", kNeverSwab));
		CHECK(omfsRegisterType(sess, OMUniqueName, kOmfTstRev2x,
					   "UniqueName", kNeverSwab));
		CHECK(omfsRegisterType(sess, OMFadeType, kOmfTstRev2x,
	 				   "FadeType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMInterpKind, kOmfTstRev2x,
					   "InterpKind", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMArgIDType, kOmfTstRev2x,
					   "ArgIDType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMEditHintType, kOmfTstRev2x,
					   "EditHintType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMDataValue, kOmfTstRevEither,
					   "DataValue", kNeverSwab));		/* revEither so that new image formats may be private data */
		CHECK(omfsRegisterType(sess, OMTapeCaseType, kOmfTstRev2x,
					   "TapeCaseType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMVideoSignalType, kOmfTstRev2x,
					   "VideoSignalType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMTapeFormatType, kOmfTstRev2x,
					   "TapeFormatType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMPulldownKindType, kOmfTstRev2x,
					   "PulldownKindType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMPhaseFrameType, kOmfTstRev2x,
					   "PhaseFrameType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMPulldownDirectionType, kOmfTstRev2x,
					   "PulldownDirectionType", kSwabIfNeeded));
		/****/
		CHECK(omfsRegisterType(sess, OMRational, kOmfTstRevEither,
					   "Rational", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMLayoutType, kOmfTstRevEither,
					   "LayoutType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMPosition32Array, kOmfTstRevEither,
					   "Position32Array", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMPosition64Array, kOmfTstRevEither,
					   "Position64Array", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMDirectionCode, kOmfTstRev2x,
					   "DirectionCode", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMColorSpace, kOmfTstRev2x,
					   "ColorSpace", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMInt32Array, kOmfTstRevEither,
					   "Int32Array", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMColorSitingType, kOmfTstRevEither,
									  "ColorSitingType", kSwabIfNeeded));
		CHECK(omfsRegisterType(sess, OMCompCodeArray, kOmfTstRevEither,
									  "CompCodeArray", kNeverSwab));
		CHECK(omfsRegisterType(sess, OMCompSizeArray, kOmfTstRevEither,
									  "CompSizeArray", kNeverSwab));
		CHECK(omfsRegisterType(sess, OMProductVersion, kOmfTstRevEither,
									  "ProductVersion", kSwabIfNeeded));
	
		/********************* Property definitions ***********************/
		CHECK(omfsRegisterProp(sess, OMNoProperty, kOmfTstRev1x,
					   "NoProperty", NULL, OMNoType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMObjID, kOmfTstRev1x,
					   "ObjID", NULL, OMObjectTag, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMAuthor, kOmfTstRev1x,
					   "Author", NULL, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMByteOrder, kOmfTstRev1x,
					   "ByteOrder", NULL, OMInt16, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMCharacterSet, kOmfTstRev1x,
					   "CharacterSet", NULL, OMCharSetType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCopyright, kOmfTstRev1x,
					   "Copyright", NULL, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMDate, kOmfTstRev1x,
					   "Date", NULL, OMString, kPropOptional));
		/* ATCP */
		CHECK(omfsRegisterProp(sess, OMATCPVersion, kOmfTstRev1x,
					   "Version", OMClassATCP, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMATCPAttributes, kOmfTstRev1x,
					   "Attributes", OMClassATCP, OMObjRef, kPropRequired));
		/* ATTB */
		CHECK(omfsRegisterProp(sess, OMATTBVersion, kOmfTstRev1x,
					   "Version", OMClassATTB, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMATTBKind, kOmfTstRevEither,
					   "Kind", OMClassATTB, OMAttrKind, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMATTBName, kOmfTstRevEither,
					   "Name", OMClassATTB, OMString, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMATTBIntAttribute, kOmfTstRevEither,
					   "IntAttribute", OMClassATTB, OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMATTBStringAttribute, kOmfTstRevEither,
					 "StringAttribute", OMClassATTB, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMATTBObjAttribute, kOmfTstRevEither,
					   "ObjAttribute", OMClassATTB, OMObjRef, kPropOptional));
		/* ATTR */
		CHECK(omfsRegisterProp(sess, OMATTRVersion, kOmfTstRev1x,
					   "Version", OMClassATTR, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMATTRAttrRefs, kOmfTstRevEither,
					   "AttrRefs", OMClassATTR, OMObjRefArray, kPropRequired));
		/* CLIP (1.x) */
		CHECK(omfsRegisterProp(sess, OMCLIPVersion, kOmfTstRev1x,
					   "Version", OMClassCLIP, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCLIPLength, kOmfTstRev1x,
					   "Length", OMClassCLIP, OMInt32, kPropRequired));
		/* CLSD */
		CHECK(omfsRegisterProp(sess, OMCLSDVersion, kOmfTstRev1x,
					   "Version", OMClassCLSD, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCLSDClassID, kOmfTstRev1x,
					   "ClassID", OMClassCLSD, OMObjectTag, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMCLSDParentClass, kOmfTstRevEither,
					   "ParentClass", OMClassCLSD, OMObjRef, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCLSDClass, kOmfTstRev2x,
					   "Class", OMClassCLSD, OMClassID, kPropRequired));
		/* CMOB (2.x) */
		CHECK(omfsRegisterProp(sess, OMCMOBDefFadeLength, kOmfTstRev2x,
					   "DefFadeLength", OMClassCMOB, OMLength32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCMOBDefFadeLength, kOmfTstRev2x,
					   "DefFadeLength", OMClassCMOB, OMLength64, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCMOBDefFadeType, kOmfTstRev2x,
					   "DefFadeType", OMClassCMOB, OMFadeType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCMOBDefFadeEditUnit, kOmfTstRev2x,
					   "DefFadeEditUnit", OMClassCMOB, OMRational, kPropOptional));
		/* CPNT (1.x) */
		CHECK(omfsRegisterProp(sess, OMCPNTVersion, kOmfTstRev1x,
					   "Version", OMClassCPNT, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCPNTTrackKind, kOmfTstRev1x,
					   "TrackKind", OMClassCPNT, OMTrackType, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMCPNTEditRate, kOmfTstRev1x,
					 "EditRate", OMClassCPNT, OMExactEditRate, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMCPNTName, kOmfTstRev1x,
					   "Name", OMClassCPNT, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCPNTEffectID, kOmfTstRev1x,
					   "EffectID", OMClassCPNT, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCPNTPrecomputed, kOmfTstRev1x,
					   "Precomputed", OMClassCPNT, OMObjRef, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCPNTAttributes, kOmfTstRev1x,
					   "Attributes", OMClassCPNT, OMObjRef, kPropOptional));
		/* CPNT (2.x) */
		CHECK(omfsRegisterProp(sess, OMCPNTDatakind, kOmfTstRev2x,
					   "DataKind", OMClassCPNT, OMObjRef, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMCPNTLength, kOmfTstRev2x,
					   "Length", OMClassCPNT, OMLength32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMCPNTLength, kOmfTstRev2x,
					   "Length", OMClassCPNT, OMLength64, kPropRequired));
		/* CTLP (2.x) */
		CHECK(omfsRegisterProp(sess, OMCTLPTime, kOmfTstRev2x,
					   "Time", OMClassCTLP, OMRational, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMCTLPDatakind, kOmfTstRev2x,
					   "DataKind", OMClassCTLP, OMObjRef, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMCTLPValue, kOmfTstRev2x,
					   "Value", OMClassCTLP, OMDataValue, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMCTLPEditHint, kOmfTstRev2x,
					   "EditHint", OMClassCTLP, OMEditHintType, kPropOptional));
		/* CVAL */
		CHECK(omfsRegisterProp(sess, OMCVALValue, kOmfTstRev2x,
					   "Value", OMClassCVAL, OMDataValue, kPropRequired));
		/* DDEF (2.x) */
		CHECK(omfsRegisterProp(sess, OMDDEFDatakindID, kOmfTstRev2x,
					   "DataKindID", OMClassDDEF, OMUniqueName, kPropRequired));
		/* DIDD (2.x) */
	    CHECK(omfsRegisterProp(sess, OMDIDDCompression, kOmfTstRevEither,
	                   "Compression", OMClassDIDD, OMString, kPropOptional));
	    CHECK(omfsRegisterProp(sess, OMDIDDStoredHeight, kOmfTstRevEither,
	                   "StoredHeight", OMClassDIDD, OMInt32, kPropRequired));
	    CHECK(omfsRegisterProp(sess, OMDIDDStoredWidth, kOmfTstRevEither,
	                   "StoredWidth", OMClassDIDD, OMInt32, kPropRequired));
	    CHECK(omfsRegisterProp(sess, OMDIDDSampledHeight, kOmfTstRevEither,
	                   "SampledHeight", OMClassDIDD, OMInt32, kPropOptional));
	    CHECK(omfsRegisterProp(sess, OMDIDDSampledWidth, kOmfTstRevEither,
	                   "SampledWidth", OMClassDIDD, OMInt32, kPropOptional));
	    CHECK(omfsRegisterProp(sess, OMDIDDSampledXOffset, kOmfTstRevEither,
	                   "SampledXOffset", OMClassDIDD, OMInt32, kPropOptional));
	    CHECK(omfsRegisterProp(sess, OMDIDDSampledYOffset, kOmfTstRevEither,
	                   "SampledYOffset", OMClassDIDD, OMInt32, kPropOptional));
	    CHECK(omfsRegisterProp(sess, OMDIDDDisplayHeight, kOmfTstRevEither,
	                   "DisplayHeight", OMClassDIDD, OMInt32, kPropOptional));
	    CHECK(omfsRegisterProp(sess, OMDIDDDisplayWidth, kOmfTstRevEither,
	                   "DisplayWidth", OMClassDIDD, OMInt32, kPropOptional));
	    CHECK(omfsRegisterProp(sess, OMDIDDDisplayXOffset, kOmfTstRevEither,
	                   "DisplayXOffset", OMClassDIDD, OMInt32, kPropOptional));
	    CHECK(omfsRegisterProp(sess, OMDIDDDisplayYOffset, kOmfTstRevEither,
	                   "DisplayYOffset", OMClassDIDD, OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMDIDDFieldAlignment, kOmfTstRevEither,
					   "ImageAlignmentFactor", OMClassDIDD, OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMDIDDGamma, kOmfTstRevEither,
					   "Gamma", OMClassDIDD, OMRational, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMDIDDClientFillStart, kOmfTstRevEither,
					   "ClientFillStart", OMClassDIDD, OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMDIDDClientFillEnd, kOmfTstRevEither,
					   "ClientFillEnd", OMClassDIDD, OMInt32, kPropOptional));

	   /* revEither so that new image formats may be private data */
	    CHECK(omfsRegisterProp(sess, OMDIDDFrameLayout, kOmfTstRevEither,
	                   "FrameLayout",  OMClassDIDD, OMLayoutType, kPropRequired));
	    CHECK(omfsRegisterProp(sess, OMDIDDAlphaTransparency,
	    						kOmfTstRevEither,
	                   "AlphaTransparency", OMClassDIDD, OMInt32, kPropOptional));
	    CHECK(omfsRegisterProp(sess, OMDIDDVideoLineMap, kOmfTstRevEither,
	                   "VideoLineMap", OMClassDIDD, OMInt32Array, kPropRequired));
	    CHECK(omfsRegisterProp(sess, OMDIDDImageAspectRatio, kOmfTstRevEither,
	                   "ImageAspectRatio", OMClassDIDD, OMRational, kPropRequired));
		/* DOSL */
		CHECK(omfsRegisterProp(sess, OMDOSLVersion, kOmfTstRev1x,
					   "Version", OMClassDOSL, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMDOSLPathName, kOmfTstRevEither,
					   "PathName", OMClassDOSL, OMString, kPropOptional));
		/* ECCP */
		CHECK(omfsRegisterProp(sess, OMECCPVersion, kOmfTstRev1x,
					   "Version", OMClassECCP, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMECCPFilmKind, kOmfTstRevEither,
					   "FilmKind", OMClassECCP, OMFilmType, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMECCPCodeFormat, kOmfTstRevEither,
					   "CodeFormat", OMClassECCP, OMEdgeType, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMECCPStartEC, kOmfTstRev1x,
					   "StartEC", OMClassECCP, OMInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMECCPHeader, kOmfTstRev1x,
					   "Header", OMClassECCP, OMVarLenBytes, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMECCPHeader, kOmfTstRev2x,
					   "Header", OMClassECCP, OMDataValue, kPropOptional));
		/* ECCP (2.x) */
		CHECK(omfsRegisterProp(sess, OMECCPStart, kOmfTstRev2x,
					   "Start", OMClassECCP, OMPosition32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMECCPStart, kOmfTstRev2x,
					   "Start", OMClassECCP, OMPosition64, kPropRequired));
		/* EDEF (2.x) */
		CHECK(omfsRegisterProp(sess, OMEDEFEffectID, kOmfTstRev2x,
					   "EffectID", OMClassEDEF, OMUniqueName, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMEDEFEffectName, kOmfTstRev2x,
					   "EffectName", OMClassEDEF, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMEDEFEffectDescription, kOmfTstRev2x,
					   "EffectDescription", OMClassEDEF, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMEDEFBypass, kOmfTstRev2x,
					   "Bypass", OMClassEDEF, OMArgIDType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMEDEFIsTimeWarp, kOmfTstRev2x,
					   "IsTimeWarp", OMClassEDEF, OMBoolean, kPropOptional));
		/* EFFE (2.x) */
		CHECK(omfsRegisterProp(sess, OMEFFEEffectKind, kOmfTstRev2x,
					   "EffectKind", OMClassEFFE, OMObjRef, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMEFFEEffectSlots, kOmfTstRev2x,
					   "EffectSlots", OMClassEFFE, OMObjRefArray, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMEFFEBypassOverride, kOmfTstRev2x,
					   "BypassOverride", OMClassEFFE, OMArgIDType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMEFFEFinalRendering, kOmfTstRev2x,
					   "FinalRendering", OMClassEFFE, OMObjRef, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMEFFEWorkingRendering, kOmfTstRev2x,
					   "WorkingRendering", OMClassEFFE, OMObjRef, kPropOptional));
		/* ERAT (2.x) */
		CHECK(omfsRegisterProp(sess, OMERATInputSegment, kOmfTstRev2x,
					   "InputSegment", OMClassERAT, OMObjRef, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMERATInputEditRate, kOmfTstRev2x,
					   "InputEditRate", OMClassERAT, OMRational, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMERATInputOffset, kOmfTstRev2x,
					   "InputOffset", OMClassERAT, OMPosition32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMERATInputOffset, kOmfTstRev2x,
					   "InputOffset", OMClassERAT, OMPosition64, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMERATResultOffset, kOmfTstRev2x,
					   "ResultOffset", OMClassERAT, OMPosition32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMERATResultOffset, kOmfTstRev2x,
					   "ResultOffset", OMClassERAT, OMPosition64, kPropRequired));
		/* ESLT (2.x) */
		CHECK(omfsRegisterProp(sess, OMESLTArgID, kOmfTstRev2x,
					   "ArgID", OMClassESLT, OMArgIDType, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMESLTArgValue, kOmfTstRev2x,
					   "ArgValue", OMClassESLT, OMObjRef, kPropRequired));
		/* FILL (1.x) */
		CHECK(omfsRegisterProp(sess, OMFILLVersion, kOmfTstRev1x,
					   "Version", OMClassFILL, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMVersion, kOmfTstRev1x,
					   "Version", NULL, OMVersionType, kPropOptional));
		/* HEAD */
		CHECK(omfsRegisterProp(sess, OMVersion, kOmfTstRev2x,
					   "Version", OMClassHEAD, OMVersionType, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMLastModified, kOmfTstRev1x,
					   "LastModified", NULL, OMTimeStamp, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMClassDictionary, kOmfTstRev1x,
					   "ClassDictionary", NULL, OMObjRefArray, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSourceMobs, kOmfTstRev1x,
					   "SourceMobs", NULL, OMMobIndex, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMCompositionMobs, kOmfTstRev1x,
					   "CompositionMobs", NULL, OMMobIndex, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMediaData, kOmfTstRev1x,
					   "MediaData", NULL, OMMobIndex, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMExternalFiles, kOmfTstRev1x,
					   "ExternalFiles", NULL, OMObjRefArray, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMAttributes, kOmfTstRev1x,
					   "Attributes", NULL, OMObjRef, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMObjectSpine, kOmfTstRev1x,
					   "ObjectSpine", NULL, OMObjRefArray, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMToolkitVersion, kOmfTstRev1x,
					   "ToolkitVersion", NULL, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMToolkitVersion, kOmfTstRev2x,
					   "ToolkitVersion", OMClassHEAD, OMInt32Array, kPropOptional));
		/* HEAD (2.x) */
		CHECK(omfsRegisterProp(sess, OMHEADByteOrder, kOmfTstRev2x,
					   "ByteOrder", OMClassHEAD, OMInt16, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMHEADLastModified, kOmfTstRev2x,
					   "LastModified", OMClassHEAD, OMTimeStamp, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMHEADClassDictionary, kOmfTstRev2x,
					   "ClassDictionary", OMClassHEAD, OMObjRefArray, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMHEADPrimaryMobs, kOmfTstRev2x,
					   "PrimaryMobs", OMClassHEAD, OMObjRefArray, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMHEADDefinitionObjects, kOmfTstRev2x,
					   "DefinitionObjects", OMClassHEAD, OMObjRefArray, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMHEADMobs, kOmfTstRev2x,
					   "Mobs", OMClassHEAD, OMObjRefArray, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMHEADMediaData, kOmfTstRev2x,
					   "MediaData", OMClassHEAD, OMObjRefArray, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMHEADIdentList, kOmfTstRevEither,
					   "IdentificationList", OMClassHEAD, OMObjRefArray, kPropOptional));
		/* IDAT (2.x) */
	    CHECK(omfsRegisterProp(sess, OMIDATImageData, kOmfTstRevEither,
	                   "ImageData", OMClassIDAT, OMDataValue, kPropRequired));
		/* IDNT */
		/* The IDNT class should be added to all files, but it's a private
		 * class in 1.x files.
		 */
		CHECK(omfsRegisterProp(sess, OMIDNTCompanyName, kOmfTstRevEither,
					   "CompanyName", OMClassIDNT, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMIDNTProductName, kOmfTstRevEither,
					   "ProductName", OMClassIDNT, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMIDNTProductVersion, kOmfTstRevEither,
					   "ProductVersion", OMClassIDNT, OMProductVersion, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMIDNTProductVersionString, kOmfTstRevEither,
					   "ProductVersionString", OMClassIDNT, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMIDNTProductID, kOmfTstRevEither,
					   "ProductID", OMClassIDNT, OMInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMIDNTDate, kOmfTstRevEither,
					   "Date", OMClassIDNT, OMTimeStamp, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMIDNTToolkitVersion, kOmfTstRevEither,
					   "ToolkitVersion", OMClassIDNT, OMProductVersion, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMIDNTPlatform, kOmfTstRevEither,
					   "Platform", OMClassIDNT, OMString, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMIDNTByteOrder, kOmfTstRevEither,
					   "IDByteOrder", OMClassIDNT, OMInt16, kPropRequired));
		/* MACL */
		CHECK(omfsRegisterProp(sess, OMMACLVersion, kOmfTstRev1x,
					   "Version", OMClassMACL, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMACLVRef, kOmfTstRev1x,
					   "VRef", OMClassMACL, OMInt16, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMACLDirID, kOmfTstRevEither,
					   "DirID", OMClassMACL, OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMACLFileName, kOmfTstRevEither,
					   "FileName", OMClassMACL, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMACLVName, kOmfTstRev2x,
					   "VName", OMClassMACL, OMString, kPropOptional));
		/* MASK (1.x) */
		CHECK(omfsRegisterProp(sess, OMMASKVersion, kOmfTstRev1x,
					   "Version", OMClassMASK, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMASKMaskBits, kOmfTstRev1x,
					   "MaskBits", OMClassMASK, OMUInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMASKIsDouble, kOmfTstRev1x,
					   "IsDouble", OMClassMASK, OMBoolean, kPropRequired));
		/* MDAT */
		CHECK(omfsRegisterProp(sess, OMMDATMobID, kOmfTstRevEither,
					   "MobID", OMClassMDAT, OMUID, kPropRequired));
		/* MDES */
		CHECK(omfsRegisterProp(sess, OMMDESVersion, kOmfTstRev1x,
					   "Version", OMClassMDES, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDESMobKind, kOmfTstRev1x,
					"MobKind", OMClassMDES, OMPhysicalMobType, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMDESLocator, kOmfTstRevEither,
					   "Locator", OMClassMDES, OMObjRefArray, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDESCodecID, kOmfTstRevEither,
					   "KitCodecID", OMClassMDES, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDESCodecDescription, kOmfTstRevEither,
					   "KitCodecDesc", OMClassMDES, OMString, kPropOptional));
		/* MDFL */
		CHECK(omfsRegisterProp(sess, OMMDFLVersion, kOmfTstRev1x,
					   "Version", OMClassMDFL, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDFLIsOMFI, kOmfTstRevEither,
					   "IsOMFI", OMClassMDFL, OMBoolean, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMDFLSampleRate, kOmfTstRev1x,
				       "SampleRate", OMClassMDFL, OMExactEditRate, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMDFLLength, kOmfTstRev1x,
					   "Length", OMClassMDFL, OMInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMDFLLength, kOmfTstRev2x,
					   "Length", OMClassMDFL, OMLength32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMDFLLength, kOmfTstRev2x,
					   "Length", OMClassMDFL, OMLength64, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMDFLSampleRate, kOmfTstRev2x,
				       "SampleRate", OMClassMDFL, OMRational, kPropRequired));
		/* MDFM (2.x) */
		CHECK(omfsRegisterProp(sess, OMMDFMFilmFormat, kOmfTstRev2x,
					   "FilmFormat", OMClassMDFM, OMFilmType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDFMFrameRate, kOmfTstRev2x,
					   "FrameRate", OMClassMDFM, OMUInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDFMPerforationsPerFrame, kOmfTstRev2x,
					   "PerforationsPerFrame", OMClassMDFM, OMUInt8, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDFMFilmAspectRatio, kOmfTstRev2x,
					   "FilmAspectRatio", OMClassMDFM, OMRational, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDFMManufacturer, kOmfTstRev2x,
					   "Manufacturer", OMClassMDFM, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDFMModel, kOmfTstRev2x,
					   "Model", OMClassMDFM, OMString, kPropOptional));
		/* MDTP (2.x) */
		CHECK(omfsRegisterProp(sess, OMMDTPFormFactor, kOmfTstRev2x,
					   "FormFactor", OMClassMDTP, OMTapeCaseType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDTPVideoSignal, kOmfTstRev2x,
					   "VideoSignal", OMClassMDTP, OMVideoSignalType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDTPTapeFormat, kOmfTstRev2x,
					   "TapeFormat", OMClassMDTP, OMTapeFormatType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDTPLength, kOmfTstRev2x,
					   "Length", OMClassMDTP, OMUInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDTPManufacturer, kOmfTstRev2x,
					   "Manufacturer", OMClassMDTP, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMDTPModel, kOmfTstRev2x,
					   "Model", OMClassMDTP, OMString, kPropOptional));
		/* MGRP (2.x) */
		CHECK(omfsRegisterProp(sess, OMMGRPChoices, kOmfTstRev2x,
					   "Choices", OMClassMGRP, OMObjRefArray, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMGRPStillFrame, kOmfTstRev2x,
					   "StillFrame", OMClassMGRP, OMObjRef, kPropOptional));
		/* MOBJ */
		CHECK(omfsRegisterProp(sess, OMMOBJVersion, kOmfTstRev1x,
					   "Version", OMClassMOBJ, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMOBJMobID, kOmfTstRevEither,
					   "MobID", OMClassMOBJ, OMUID, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMOBJLastModified, kOmfTstRevEither,
					 "LastModified", OMClassMOBJ, OMTimeStamp, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMOBJUsageCode, kOmfTstRev1x,
					"UsageCode", OMClassMOBJ, OMUsageCodeType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMOBJStartPosition, kOmfTstRev1x,
					   "StartPosition", OMClassMOBJ, OMInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMOBJPhysicalMedia, kOmfTstRev1x,
					   "PhysicalMedia", OMClassMOBJ, OMObjRef, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMOBJSlots, kOmfTstRev2x,
					   "Slots", OMClassMOBJ, OMObjRefArray, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMOBJName, kOmfTstRev2x,
					   "Name", OMClassMOBJ, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMMOBJCreationTime, kOmfTstRev2x,
					   "CreationTime", OMClassMOBJ, OMTimeStamp, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMOBJUserAttributes, kOmfTstRev2x,
					   "UserAttributes", OMClassMOBJ, OMObjRef, kPropOptional));

		/* MSLT (2.x) */
		CHECK(omfsRegisterProp(sess, OMMSLTSegment, kOmfTstRev2x,
					   "Segment", OMClassMSLT, OMObjRef, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMSLTEditRate, kOmfTstRev2x,
					   "EditRate", OMClassMSLT, OMRational, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMMSLTTrackDesc, kOmfTstRev2x,
					   "TrackDesc", OMClassMSLT, OMObjRef, kPropOptional));
		/* NEST (2.x) */
		CHECK(omfsRegisterProp(sess, OMNESTSlots, kOmfTstRev2x,
					   "Slots", OMClassNEST, OMObjRefArray, kPropRequired));
		/* NETL (2.x) */
		CHECK(omfsRegisterProp(sess, OMNETLURLString, kOmfTstRev2x,
					   "URLString", OMClassNETL, OMString, kPropRequired));
		/* OOBJ (2.x) */
		CHECK(omfsRegisterProp(sess, OMOOBJObjClass, kOmfTstRev2x,
					   "ObjClass", OMClassOOBJ, OMClassID, kPropRequired));		
		/* PDWN (2.x) */
		CHECK(omfsRegisterProp(sess, OMPDWNInputSegment, kOmfTstRev2x,
					   "InputSegment", OMClassPDWN, OMObjRef, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMPDWNPulldownKind, kOmfTstRev2x,
					   "PulldownKind", OMClassPDWN, OMPulldownKindType, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMPDWNPhaseFrame, kOmfTstRev2x,
					   "PhaseFrame", OMClassPDWN, OMPhaseFrameType, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMPDWNDirection, kOmfTstRev2x,
					   "PulldownDirection", OMClassPDWN, OMPulldownDirectionType, kPropRequired));
		/* REPT (1.x) */
		CHECK(omfsRegisterProp(sess, OMREPTVersion, kOmfTstRev1x,
					   "Version", OMClassREPT, OMVersionType, kPropOptional));
		/* SCLP */
		CHECK(omfsRegisterProp(sess, OMSCLPVersion, kOmfTstRev1x,
					   "Version", OMClassSCLP, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPSourceID, kOmfTstRevEither,
					   "SourceID", OMClassSCLP, OMUID, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPSourceTrack, kOmfTstRev1x,
					   "SourceTrack", OMClassSCLP, OMInt16, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMSCLPSourcePosition, kOmfTstRev1x,
					   "SourcePosition", OMClassSCLP, OMInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMSCLPFadeInLength, kOmfTstRev1x,
					   "FadeInLength", OMClassSCLP, OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPFadeInLength, kOmfTstRev2x,
					   "FadeInLength", OMClassSCLP, OMLength32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPFadeInLength, kOmfTstRev2x,
					   "FadeInLength", OMClassSCLP, OMLength64, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPFadeInType, kOmfTstRev1x,
					   "FadeInType", OMClassSCLP, OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPFadeInType, kOmfTstRev2x,
					   "FadeInType", OMClassSCLP, OMFadeType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPFadeOutLength, kOmfTstRev1x,
					   "FadeOutLength", OMClassSCLP, OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPFadeOutLength, kOmfTstRev2x,
					   "FadeOutLength", OMClassSCLP, OMLength32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPFadeOutLength, kOmfTstRev2x,
					   "FadeOutLength", OMClassSCLP, OMLength64, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPFadeOutType, kOmfTstRev1x,
					   "FadeOutType", OMClassSCLP, OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPFadeOutType, kOmfTstRev2x,
					   "FadeOutType", OMClassSCLP, OMFadeType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSCLPSourceTrackID, kOmfTstRev2x,
					   "SourceTrackID", OMClassSCLP, OMUInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMSCLPStartTime, kOmfTstRev2x,
					   "StartTime", OMClassSCLP, OMPosition32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMSCLPStartTime, kOmfTstRev2x,
					   "StartTime", OMClassSCLP, OMPosition64, kPropRequired));
		/* SEQU (1.x) */
		CHECK(omfsRegisterProp(sess, OMSEQUVersion, kOmfTstRev1x,
					   "Version", OMClassSEQU, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSEQUSequence, kOmfTstRev1x,
					   "Sequence", OMClassSEQU, OMObjRefArray, kPropRequired));
		/* SEQU (2.x) */
		CHECK(omfsRegisterProp(sess, OMSEQUComponents, kOmfTstRev2x,
					   "Components", OMClassSEQU, OMObjRefArray, kPropRequired));
		/* SLCT (1.x) */
		CHECK(omfsRegisterProp(sess, OMSLCTVersion, kOmfTstRev1x,
					   "Version", OMClassSLCT, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSLCTIsGanged, kOmfTstRev1x,
					   "IsGanged", OMClassSLCT, OMBoolean, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSLCTSelectedTrack, kOmfTstRev1x,
					   "SelectedTrack", OMClassSLCT, OMInt16, kPropRequired));
		/* SLCT (2.x) */
		CHECK(omfsRegisterProp(sess, OMSLCTSelected, kOmfTstRev2x,
					   "Selected", OMClassSLCT, OMObjRef, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMSLCTAlternates, kOmfTstRev2x,
					   "Alternates", OMClassSLCT, OMObjRefArray, kPropOptional));
		/* SMOB (2.x) */
		CHECK(omfsRegisterProp(sess, OMSMOBMediaDescription, kOmfTstRev2x,
					   "MediaDescription", OMClassSMOB, OMObjRef, kPropRequired));
		/* SPED (1.x) */
		CHECK(omfsRegisterProp(sess, OMSPEDVersion, kOmfTstRev1x,
					   "Version", OMClassSPED, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMSPEDNumerator, kOmfTstRev1x,
					   "Numerator", OMClassSPED, OMInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMSPEDDenominator, kOmfTstRev1x,
					   "Denominator", OMClassSPED, OMInt32, kPropRequired));
		/* SREF (2.x) */
		CHECK(omfsRegisterProp(sess, OMSREFRelativeScope, kOmfTstRev2x,
					   "RelativeScope", OMClassSREF, OMUInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMSREFRelativeSlot, kOmfTstRev2x,
					   "RelativeSlot", OMClassSREF, OMUInt32, kPropRequired));
		/* TCCP */
		CHECK(omfsRegisterProp(sess, OMTCCPVersion, kOmfTstRev1x,
					   "Version", OMClassTCCP, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTCCPFlags, kOmfTstRev1x,
					   "Flags", OMClassTCCP, OMInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTCCPFPS, kOmfTstRev1x,
					   "FPS", OMClassTCCP, OMInt16, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTCCPStartTC, kOmfTstRev1x,
					   "StartTC", OMClassTCCP, OMInt32, kPropRequired));
		/* TCCP (2.x) */
		CHECK(omfsRegisterProp(sess, OMTCCPDrop, kOmfTstRev2x,
					   "Drop", OMClassTCCP, OMBoolean, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTCCPFPS, kOmfTstRev2x,
					   "FPS", OMClassTCCP, OMUInt16, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTCCPStart, kOmfTstRev2x,
					   "Start", OMClassTCCP, OMPosition32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTCCPStart, kOmfTstRev2x,
					   "Start", OMClassTCCP, OMPosition64, kPropRequired));
		/* TRAK (1.x) */
		CHECK(omfsRegisterProp(sess, OMTRAKVersion, kOmfTstRev1x,
					   "Version", OMClassTRAK, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTRAKLabelNumber, kOmfTstRev1x,
					   "LabelNumber", OMClassTRAK, OMInt16, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTRAKAttributes, kOmfTstRev1x,
					   "Attributes", OMClassTRAK, OMObjRef, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTRAKFillerProxy, kOmfTstRev1x,
					   "FillerProxy", OMClassTRAK, OMObjRef, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTRAKTrackComponent, kOmfTstRev1x,
					   "TrackComponent", OMClassTRAK, OMObjRef, kPropRequired));
		/* TRAN (1.x) */
		CHECK(omfsRegisterProp(sess, OMTRANVersion, kOmfTstRev1x,
					   "Version", OMClassTRAN, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTRANCutPoint, kOmfTstRev1x,
					   "CutPoint", OMClassTRAN, OMInt32, kPropRequired));
		/* TRAN (2.x) */
		CHECK(omfsRegisterProp(sess, OMTRANCutPoint, kOmfTstRev2x,
					   "CutPoint", OMClassTRAN, OMPosition32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTRANCutPoint, kOmfTstRev2x,
					   "CutPoint", OMClassTRAN, OMPosition64, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTRANEffect, kOmfTstRev2x,
					   "Effect", OMClassTRAN, OMObjRef, kPropRequired));
		/* TRKD (2.x) */
		CHECK(omfsRegisterProp(sess, OMTRKDOrigin, kOmfTstRev2x,
					   "Origin", OMClassTRKD, OMPosition32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTRKDOrigin, kOmfTstRev2x,
					   "Origin", OMClassTRKD, OMPosition64, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTRKDTrackID, kOmfTstRev2x,
					   "TrackID", OMClassTRKD, OMUInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTRKDTrackName, kOmfTstRev2x,
					   "TrackName", OMClassTRKD, OMString, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTRKDPhysTrack, kOmfTstRev2x,
					   "PhysicalTrack", OMClassTRKD, OMUInt32, kPropOptional));
		/* TRKG (1.x) */
		CHECK(omfsRegisterProp(sess, OMTRKGVersion, kOmfTstRev1x,
					   "Version", OMClassTRKG, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTRKGGroupLength, kOmfTstRev1x,
					   "GroupLength", OMClassTRKG, OMInt32, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTRKGTracks, kOmfTstRev1x,
					   "Tracks", OMClassTRKG, OMObjRefArray, kPropRequired));
		/* TRKR (1.x) */
		CHECK(omfsRegisterProp(sess, OMTRKRVersion, kOmfTstRev1x,
					   "Version", OMClassTRKR, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTRKRRelativeScope, kOmfTstRev1x,
					   "RelativeScope", OMClassTRKR, OMInt16, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTRKRRelativeTrack, kOmfTstRev1x,
					   "RelativeTrack", OMClassTRKR, OMInt16, kPropRequired));
		/* TXTL */
		CHECK(omfsRegisterProp(sess, OMTXTLVersion, kOmfTstRev1x,
					   "Version", OMClassTXTL, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTXTLName, kOmfTstRevEither,
					   "Name", OMClassTXTL, OMString, kPropOptional));
		/* UNXL */
		CHECK(omfsRegisterProp(sess, OMUNXLVersion, kOmfTstRev1x,
					   "Version", OMClassUNXL, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMUNXLPathName, kOmfTstRevEither,
					   "PathName", OMClassUNXL, OMString, kPropOptional));
		/* VVAL (2.x) */
		CHECK(omfsRegisterProp(sess, OMVVALInterpolation, kOmfTstRev2x,
					   "Interpolation", OMClassVVAL, OMInterpKind, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMVVALPointList, kOmfTstRev2x,
					   "PointList", OMClassVVAL, OMObjRefArray, kPropRequired));
		/* WARP (1.x) */
		CHECK(omfsRegisterProp(sess, OMWARPVersion, kOmfTstRev1x,
					   "Version", OMClassWARP, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMWARPEditRate, kOmfTstRev1x,
					 "EditRate", OMClassWARP, OMExactEditRate, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMWARPPhaseOffset, kOmfTstRev1x,
					   "PhaseOffset", OMClassWARP, OMInt16, kPropRequired));
		/* WINL (2.x) */
		CHECK(omfsRegisterProp(sess, OMWINLPathName, kOmfTstRev2x,
					   "PathName", OMClassWINL, OMString, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMWINLShortcut, kOmfTstRev2x,
					   "Shortcut", OMClassWINL, OMDataValue, kPropOptional));
		/* Add new strings for 2.0 */


					   
	

		/*** Register 1.x Media Properties (TIFF, AIFC, and WAVE) ***/
		CHECK(omfsRegisterProp(sess, OMTIFDVersion, kOmfTstRev1x,
							  "Version", OMClassTIFD, OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTIFDIsUniform, kOmfTstRevEither, 
								  "IsUniform", OMClassTIFD, OMBoolean, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTIFDIsContiguous, kOmfTstRevEither, 
									  "IsContiguous", OMClassTIFD, 
									  OMBoolean, kPropRequired ));
		CHECK(omfsRegisterProp(sess, OMTIFDTrailingLines, kOmfTstRevEither, 
									  "TrailingLines", OMClassTIFD, 
									  OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTIFDLeadingLines, kOmfTstRevEither, 
									  "LeadingLines", OMClassTIFD, 
									  OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTIFDJPEGTableID, kOmfTstRevEither, 
									  "JPEGTableID", OMClassTIFD, 
									  OMJPEGTableIDType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTIFDSummary, kOmfTstRev1x, 
									  "Summary", OMClassTIFD, 
									  OMVarLenBytes, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTIFDSummary, kOmfTstRev2x, 
									  "Summary", OMClassTIFD, 
									  OMDataValue, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTIFFVersion, kOmfTstRev1x, 
									  "Version", OMClassTIFF, 
									  OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMTIFFMobID, kOmfTstRev1x, 
									  "MobID", OMClassTIFF, 
									  OMUID, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTIFFData, kOmfTstRev1x, "Data", 
									  OMClassTIFF, OMVarLenBytes, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMTIFFData, kOmfTstRev2x, 
									  "ImageData", OMClassTIFF, 
									  OMDataValue, kPropRequired));

		CHECK(omfsRegisterProp(sess, OMAIFCVersion, kOmfTstRev1x, 
									  "Version", OMClassAIFC, 
									  OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMAIFCMobID, kOmfTstRev1x, "MobID", 
									  OMClassAIFC, 
									  OMUID, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMAIFDVersion, kOmfTstRev1x, "Version", 
									  OMClassAIFD, 
									  OMInt32, kPropOptional));			
		CHECK(omfsRegisterProp(sess, OMAIFCData, kOmfTstRev1x, "Data", 
									  OMClassAIFC, 
									  OMVarLenBytes, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMAIFDSummary, kOmfTstRev1x, 
									  "Summary", OMClassAIFD, 
									  OMVarLenBytes, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMAIFCData, kOmfTstRev2x, "AudioData", 
									  OMClassAIFC, 
									  OMDataValue, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMAIFDSummary, kOmfTstRev2x, "Summary", 
									  OMClassAIFD, 
									  OMDataValue, kPropRequired));

		CHECK(omfsRegisterProp(sess, OMWAVEVersion, kOmfTstRev1x, 
									  "Version", OMClassWAVE, 
									  OMVersionType, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMWAVEMobID, kOmfTstRev1x, "MobID", 
									  OMClassWAVE, 
									  OMUID, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMWAVEData, kOmfTstRev1x, "Data", 
									  OMClassWAVE, 
									  OMVarLenBytes, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMWAVEData, kOmfTstRev2x, "AudioData", 
									  OMClassWAVE, 
									  OMDataValue, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMWAVDVersion, kOmfTstRev1x, "Version", 
									  OMClassWAVD, 
									  OMInt32, kPropOptional));
		CHECK(omfsRegisterProp(sess, OMWAVDSummary, kOmfTstRev1x, "Summary", 
									  OMClassWAVD, 
									  OMVarLenBytes, kPropRequired));
		CHECK(omfsRegisterProp(sess, OMWAVDSummary, kOmfTstRev2x, "Summary", 
									  OMClassWAVD, 
									  OMDataValue, kPropRequired));

		/*
		 * Now that the session is set up, go initialize the other modules.
		 */
#if OMFI_ENABLE_SEMCHECK
		CHECK(initSemanticChecks(sess));
#endif
	}
	XEXCEPT
	{
		*sessionPtr = NULL;
		RERAISE(OM_ERR_BADSESSIONOPEN);
	}
	XEND
	
	*sessionPtr = sess;
	return (OM_ERR_NONE);
}


/************************
 * Function: omfsSetSessionIOHandlers
 *
 * 	Sets the Bento IO Handlers for an OMFI session.
 *
 * Argument Notes:
 *		Session - The session handle returned from omfsBeginSession.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BAD_SESSION - The session ptr was not valid.
 */

omfErr_t omfsSetSessionIOHandlers( omfSessionHdl_t session,
							       struct omfiBentoIOFuncs ioFuncs)
{
	if ((session == NULL) || (session->cookie != SESSION_COOKIE))
		return (OM_ERR_BAD_SESSION);

	if (ioFuncs.containerMetahandlerFunc)
	{
		CMSetMetaHandler(session->BentoSession, "OMFI", ioFuncs.containerMetahandlerFunc);
		if (session->BentoErrorRaised)
		{
			return OM_ERR_BADSESSIONOPEN;
		}
	}
	session->ioFuncs = ioFuncs;

	return OM_ERR_NONE;
}



/************************
 * Function: omfsEndSession
 *
 * 	Closes an OMFI session, and releases all memory used by the
 *		session.
 *
 * Argument Notes:
 *		Session - The session handle returned from omfsBeginSession.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BAD_SESSION - The session ptr was not valid.
 */
omfErr_t omfsEndSession(
			omfSessionHdl_t session)
{
	omfInt32           save_BENumber;
	omfHdl_t			tstFile;
	omfHdl_t			tmpFile;
	omfErr_t			status, totalStatus;
	
	if ((session == NULL) || (session->cookie != SESSION_COOKIE))
		return (OM_ERR_BAD_SESSION);
		
	XPROTECT(NULL)
	{
		save_BENumber = session->BentoErrorNumber;
		session->BentoErrorNumber = 0;
		session->BentoErrorRaised = 0;
		
		/* The media layer may install a callback to do end-of-session
		 * cleanup here.
		 */
		if(session->closeSessCB != NULL)
			(*session->closeSessCB)(session);
		
		/* Dispose all tables used, and clear pointers to that this
		 * won't look like a valid session
		 */
		if(session->types1X != NULL)
			omfsTableDisposeAll(session->types1X);
		if(session->properties1X != NULL)
			omfsTableDisposeAll(session->properties1X);
		if(session->types2X != NULL)
			omfsTableDisposeAll(session->types2X);
		if(session->properties2X != NULL)
			omfsTableDisposeAll(session->properties2X);
		if(session->classDefs1X != NULL)
			omfsTableDisposeAll(session->classDefs1X);
		if(session->classDefs2X != NULL)
			omfsTableDisposeAll(session->classDefs2X);
		if(session->codecMDES != NULL)
			omfsTableDisposeAll(session->codecMDES);
		if(session->codecID != NULL)
			omfsTableDisposeAll(session->codecID);
		if(session->typeNames != NULL)
		  omfsTableDisposeAll(session->typeNames);
		if(session->propertyNames != NULL)
		  omfsTableDisposeAll(session->propertyNames);
	
		session->types1X = NULL;
		session->properties1X = NULL;
		session->types2X = NULL;
		session->properties2X = NULL;
		session->classDefs1X = NULL;
		session->classDefs2X = NULL;
		session->codecMDES = NULL;
		session->codecID = NULL;
		session->cookie = 0;

		if (session->BentoErrorRaised)
			CMAbortSession(session->BentoSession);
		else
		{
			totalStatus = OM_ERR_NONE;
			tstFile = session->topFile;
			while (tstFile != NULL)
			{
				tmpFile = tstFile->prevFile; /* advances the while loop */
				status = omfsCloseFile(tstFile);
				if(status != OM_ERR_NONE)
					totalStatus = status;
				tstFile = tmpFile;
			}
			
			/* TRUE= close open containers */
			CMEndSession(session->BentoSession, TRUE);	
			CHECK(totalStatus);
		}
		if (session->BentoErrorRaised)
			RAISE(OM_ERR_BENTO_PROBLEM);
	
		if(session->ident)
			omOptFree(NULL, session->ident);
		omOptFree(NULL, session);
	}
	XEXCEPT
	{
		if(session->ident)
			omOptFree(NULL, session->ident);
		omOptFree(NULL, session);
	}
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsFileGetRev
 *
 * 	Returns the revision of the given file.   Supported revisions
 *		are:
 *			kOmfRev1x -- Rev 1.0-1.5 files
 *			kOmfRevIMA -- Rev 1.5 files with Bento 1.0d5
 *			kOmfRev2x -- Rev 2.0- files
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
omfErr_t omfsFileGetRev(
			omfHdl_t			file, 
			omfFileRev_t	*fileRev)
{
  omfAssertValidFHdl(file);

  *fileRev = file->setrev;

  return(OM_ERR_NONE);
}

/************************
 * Function: omfsCreateFile
 *
 * 	Create an OMFI file referencing the given session, of the
 *		given revision.
 *
 * Argument Notes:
 *		Stream -- Must be of a type which matches the Bento file handler.
 *		ie:	a SFReply record pointer for omfmacsf.c
 *				or a path string for omfansic.c
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsCreateFile(
			fileHandleType		stream, 
			omfSessionHdl_t	session, 
			omfFileRev_t		rev, 
			omfHdl_t				*result)
{
	omfHdl_t        file;
	CMRefCon        myRefCon;
	CMContainer     container;
	omfObject_t     head;
	omfVersionType_t fileVersion;
	omfDDefObj_t    datakind;
	omfTimeStamp_t  create_timestamp;
	omfErr_t		status;
	omfProperty_t	idProp;

	XPROTECT(NULL)
	{
		if (stream == NULL) 
		  RAISE(OM_ERR_NULL_PARAM);

		if (session == NULL)
		  RAISE(OM_ERR_BAD_SESSION);	

		if(session->cookie != SESSION_COOKIE)
		  RAISE(OM_ERR_BAD_SESSION);	

		CHECK(InitFileHandle(session, kOmfiMedia, &file));	
		*result = file;
		clearBentoErrors(file);
		file->setrev = rev;
		file->openType = kOmCreate;
		
		myRefCon = (*session->ioFuncs.createRefConFunc)(session->BentoSession, 
											 (char *) stream, NULL, file);
		omfCheckBentoRaiseError(file, OM_ERR_BADSESSIONMETA);
	
		container = CMOpenNewContainer(session->BentoSession, myRefCon,
												"OMFI",
												(CMContainerUseMode) kCMWriting,
												1, 0, 0);
		omfCheckBentoRaiseError(file, OM_ERR_BADOPEN);
		if(container == NULL)
			RAISE(OM_ERR_BADOPEN);
	
		file->container = container;
		head = (omfObject_t)cmFindObject(((ContainerPtr) container)->toc, 1);
		omfCheckBentoRaiseError(file, OM_ERR_BADHEAD);
	
		file->head = head;
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;
		if (omfsWriteClassID(file, head, idProp, "HEAD") != OM_ERR_NONE)
			RAISE(OM_ERR_NOBYTEORDER);
	
		if ((rev == kOmfRev1x) || rev == kOmfRevIMA)
		  {
			fileVersion.major = 1;
			fileVersion.minor = 0;
			if(rev == kOmfRev1x)
				CMSetContainerVersion1(file->container);
		  }
		else if (rev == kOmfRev2x)
		  {
			fileVersion.major = 2;
			fileVersion.minor = 0;
		  }
		 
		if (omfsWriteVersionType(file, head, OMVersion, fileVersion))
		  {
			RAISE(OM_ERR_BADHEAD);
		  }
	
		/* 'MM' or 'II' see OMVars.h, OMDefs.h */
		if (omfsPutByteOrder(file, head, file->byteOrder) != OM_ERR_NONE)
		  {
			RAISE(OM_ERR_NOBYTEORDER);
		  }

		/* Write HEAD:LastModified */
		omfsGetDateTime(&create_timestamp);
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		  {
			 CHECK(omfsWriteTimeStamp(file, file->head, OMLastModified,
											  create_timestamp));
		  }
		else /* kOmf2x */
		  {
			 CHECK(omfsWriteTimeStamp(file, file->head, OMHEADLastModified,
											  create_timestamp));
		  }

		/**** File object is now correct enough to pass to OMFI functions 
		 **** to read some data. *****/
		CHECK(BuildPropIDCache(file));
		CHECK(BuildTypeIDCache(file));
	
		/* Since new file: create datakind objects for all required datakinds
	    * defined in the specification and add to HEAD:Definitions,
		 * then initialize file cache with these objects.  These should only
		 * be created if it is a kOmfRev2x file.
	    */
		if (rev == kOmfRev2x)
		{
		  /* Create Datakind cache; the omfiDatakindNew() calls will add
			* entries to it.
			*/
		  CHECK(omfsNewDefTable(file, DEFAULT_NUM_DATAKIND_DEFS, &(file->datakinds)));

		  CHECK(omfiDatakindNew(file, NODATAKIND, &datakind));
		  CHECK(omfiDatakindNew(file, CHARKIND, &datakind));
		  CHECK(omfiDatakindNew(file, UINT8KIND, &datakind));
		  CHECK(omfiDatakindNew(file, UINT16KIND, &datakind));
		  CHECK(omfiDatakindNew(file, UINT32KIND, &datakind));
		  CHECK(omfiDatakindNew(file, INT8KIND, &datakind));
		  CHECK(omfiDatakindNew(file, INT16KIND, &datakind));
		  CHECK(omfiDatakindNew(file, INT32KIND, &datakind));
		  CHECK(omfiDatakindNew(file, RATIONALKIND, &datakind));
		  CHECK(omfiDatakindNew(file, BOOLEANKIND, &datakind));
		  CHECK(omfiDatakindNew(file, STRINGKIND, &datakind));
		  CHECK(omfiDatakindNew(file, COLORSPACEKIND, &datakind));
		  CHECK(omfiDatakindNew(file, COLORKIND, &datakind));
		  CHECK(omfiDatakindNew(file, DISTANCEKIND, &datakind));
		  CHECK(omfiDatakindNew(file, POINTKIND, &datakind));
		  CHECK(omfiDatakindNew(file, DIRECTIONKIND, &datakind));
		  CHECK(omfiDatakindNew(file, POLYKIND, &datakind));
		  CHECK(omfiDatakindNew(file, MATTEKIND, &datakind));
		  CHECK(omfiDatakindNew(file, PICTUREKIND, &datakind));
		  CHECK(omfiDatakindNew(file, SOUNDKIND, &datakind));
		  CHECK(omfiDatakindNew(file, PICTWITHMATTEKIND, &datakind));
		  CHECK(omfiDatakindNew(file, STEREOKIND, &datakind));
		  CHECK(omfiDatakindNew(file, TIMECODEKIND, &datakind));
		  CHECK(omfiDatakindNew(file, EDGECODEKIND, &datakind));

		  CHECK(BuildEffectDefCache(file, 
							DEFAULT_NUM_EFFECT_DEFS, &(file->effectDefs)));
		}
	
		/* We now use datakinds in the whole API, not just 2.x
		 */
		omfiDatakindLookup(file, NODATAKIND, &file->nilKind, &status);
		CHECK(status);
		omfiDatakindLookup(file, PICTUREKIND, &file->pictureKind, &status);
		CHECK(status);
		omfiDatakindLookup(file, SOUNDKIND, &file->soundKind, &status);
		CHECK(status);

		CHECK(omfsNewUIDTable(file, DEFAULT_NUM_MOBS, &(file->mobs)));
		CHECK(omfsSetTableDispose(file->mobs, &MobDisposeMap1X));
		CHECK(omfsNewUIDTable(file, DEFAULT_NUM_DATAOBJ, &(file->dataObjs)));
	
		/* this is a private property only to be used by the toolkit */
		if (rev == kOmfRev1x) /* fake out toolkit version for old OMCntr.c */
		{
			omfVersionType_t tmpVersion = {1,0};
			CHECK(omfsWriteVersionType(file, head, OMToolkitVersion,
												tmpVersion));
		}
		else if(rev == kOmfRevIMA)
		{
			omfVersionType_t tmpVersion = {1,0};

			/* JeffB: Loss of precision here is OK for 1.x era files
			 * (should be gone by rev 256 of toolkit) 
			 */
			tmpVersion.major = (char)omfiToolkitVersion.major;
			tmpVersion.minor = (char)omfiToolkitVersion.minor;
			CHECK(omfsWriteVersionType(file, head, OMToolkitVersion,
												tmpVersion));
		}
		else
		{
			omfPosition_t	field1, field2, field3;
			omfInt32		major, minor, tertiary;
			
			omfsCvtInt32toPosition(0, field1);
			omfsCvtInt32toPosition(sizeof(omfInt32), field2);
			omfsCvtInt32toPosition(2 * sizeof(omfInt32), field3);
			major = omfiToolkitVersion.major;
			minor = omfiToolkitVersion.minor;
			tertiary = omfiToolkitVersion.tertiary;
			CHECK(OMWriteProp(file, head, OMToolkitVersion, field1, OMInt32Array,
								sizeof(omfInt32), &major));
			CHECK(OMWriteProp(file, head, OMToolkitVersion, field2,
								OMInt32Array, sizeof(omfInt32), &minor));
			CHECK(OMWriteProp(file, head, OMToolkitVersion, field3,
								OMInt32Array, sizeof(omfInt32), &tertiary));
		}

		AddIDENTObject(file, file->session->ident);
	}

	XEXCEPT
	  {
		 /* Reset to previous state before returning with error */
		 if (session && file)
		{
			/* Added by MT: you need to save the pointer before deleting
			 * the  object
			 */
			struct omfiFile *pSavedAddr = file->prevFile;

			if (session->topFile)
				omOptFree(NULL, session->topFile);
			     
			/* Set to previous file or NULL */
			session->topFile = pSavedAddr;
		}
	  }
	XEND;
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsOpenFile
 *
 * 	Opens an OMFI file read-only.
 *
 * Argument Notes:
 *		Stream -- Must be of a type which matches the Bento file handler.
 *		ie:	a SFReply record pointer for omfmacsf.c
 *				or a path string for omfansic.c
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BAD_SESSION - Session handle is invalid.
 *		OM_ERR_BADHEAD - Head object is invalid or missing.
 *		OM_ERR_NOTOMFIFILE - The file is not an OMFI file.
 *		OM_ERR_BADOPEN - The file is an OMFI file, but fails to open.
 */
omfErr_t omfsOpenFile(
			fileHandleType		stream, 
			omfSessionHdl_t	session, 
			omfHdl_t				*result)
{
	return (omfsInternOpenFile(stream, session, (CMContainerUseMode) 0, 
							   kOmOpenRead, result));
}


/************************
 * Function: omfsModifyFile
 *
 * 	Opens an OMFI file for update (read-write).
 *
 * Argument Notes:
 *		Stream -- Must be of a type which matches the Bento file handler.
 *		ie:	a SFReply record pointer for omfmacsf.c
 *				or a path string for omfansic.c
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BAD_SESSION - Session handle is invalid.
 *		OM_ERR_BADHEAD - Head object is invalid or missing.
 *		OM_ERR_NOTOMFIFILE - The file is not an OMFI file.
 *		OM_ERR_BADOPEN - The file is an OMFI file, but fails to open.
 */
omfErr_t omfsModifyFile(
			fileHandleType		stream, 
			omfSessionHdl_t	session, 
			omfHdl_t				*result)
{
	XPROTECT(NULL)
	{
		CHECK(omfsInternOpenFile(stream, session,
				 (CMContainerUseMode) kCMReuseFreeSpace, kOmModify, result));
		if((*result)->setrev == kOmfRev1x)
		{
			omfVersionType_t tmpVersion;
			
			/* JeffB: Loss of precision here is OK for 1.x era files
			 * (should be gone by rev 256 of toolkit) 
			 */
			tmpVersion.major = (char)omfiToolkitVersion.major;
			tmpVersion.minor = (char)omfiToolkitVersion.minor;
			CMSetContainerVersion1((*result)->container);

		/* this is a private property only to be used by the toolkit */
			CHECK(omfsWriteVersionType(*result, (*result)->head, 
								OMToolkitVersion, tmpVersion));
		}
		else if((*result)->setrev == kOmfRevIMA)
		{
			omfVersionType_t tmpVersion;

			/* JeffB: Loss of precision here is OK for 1.x era files
			 * (should be gone by rev 256 of toolkit) 
			 */
			tmpVersion.major = (char)omfiToolkitVersion.major;
			tmpVersion.minor = (char)omfiToolkitVersion.minor;
			CHECK(omfsWriteVersionType(*result, (*result)->head, OMToolkitVersion,
												tmpVersion));
		}
		else
		{
			omfPosition_t	field1, field2, field3;
			omfInt32		major, minor, tertiary;

			omfsCvtInt32toPosition(0, field1);
			omfsCvtInt32toPosition(sizeof(omfInt32), field2);
			omfsCvtInt32toPosition(2 * sizeof(omfInt32), field3);

			major = omfiToolkitVersion.major;
			minor = omfiToolkitVersion.minor;
			tertiary = omfiToolkitVersion.tertiary;
			CHECK(OMWriteProp(*result, (*result)->head, OMToolkitVersion, field1, OMInt32Array,
								sizeof(omfInt32), &major));
			CHECK(OMWriteProp(*result, (*result)->head, OMToolkitVersion, field2,
								OMInt32Array, sizeof(omfInt32), &minor));
			CHECK(OMWriteProp(*result, (*result)->head, OMToolkitVersion, field3,
								OMInt32Array, sizeof(omfInt32), &tertiary));
		}
		
		/* NOTE: If modifying an existing file WITHOUT an IDNT object, add a
		 * dummy IDNT object to indicate that this program was not the creator.
		 */
		if(omfsLengthObjRefArray(*result, (*result)->head, OMHEADIdentList) == 0)
		{
			AddIDENTObject(*result, NULL);
		}
		/* Now, always add the information from THIS application */
		AddIDENTObject(*result, session->ident);
	}
	XEXCEPT
	  {
	  }
	XEND;
  
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsOpenRawFile
 *
 * 	Opens a non-OMFI file read-only, returning an opaque file handle.
 *		This routine is usually called by the media layer when following
 *		a locator to a non-OMFI file.
 *
 * Argument Notes:
 *		fh -- The same file structure given to omfsCreateFile.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *
 * See Also:
 */
omfErr_t omfsOpenRawFile(
			fileHandleType		fh, 
			omfInt16			fhSize,
			omfSessionHdl_t		session, 
			omfHdl_t			*result)
{
	omfHdl_t				file;
	omfCodecStream_t	tstStream;
	
	*result = NULL;
	XPROTECT(NULL)
	{
		if(session->cookie != SESSION_COOKIE)
		  RAISE(OM_ERR_BAD_SESSION);	

		CHECK(InitFileHandle(session, kForeignMedia, &file));
		file->openType = kOmOpenRead;
		file->rawFileDesc = omOptMalloc(file, fhSize);
		file->nilKind = (omfDDefObj_t)0;
		file->pictureKind = (omfDDefObj_t)1;
		file->soundKind = (omfDDefObj_t)2;
		if (file->rawFileDesc == NULL)
			RAISE(OM_ERR_BADOPEN);
		memcpy((char *)file->rawFileDesc, fh, fhSize);
		
		/* omcOpenStream is where the file is actually opened, so do a trial openStream
		 * to see if the file is there.
		 */
#if OMFI_ENABLE_STREAM_CACHE
		tstStream.cachePtr = NULL;
#endif
		tstStream.procData = NULL;
		CHECK(omcOpenStream(file, file, &tstStream, 0, OMNoProperty, OMNoType));
		CHECK(omcCloseStream(&tstStream));
		*result = file;
	}
	XEXCEPT
	{
		/* Reset to previous state before returning with error */
		if (session && file)
		{
			/* Added by NW (!) you need to save the pointer before deleting
			 * the  object
			 */
			struct omfiFile *pSavedAddr = file->prevFile;

			if (session->topFile)
				omOptFree(NULL, session->topFile);

			/* Set to previous file or NULL */
			session->topFile = pSavedAddr;
		}
	}
	XEND;
	
	return(OM_ERR_NONE);
}
/************************
 * Function: omfsAppendRawFile
 *
 * 	Create an OMFI file referencing the given session, of the
 *		given revision.
 *
 * Argument Notes:
 *		Stream -- Must be of a type which matches the Bento file handler.
 *		ie:	a SFReply record pointer for omfmacsf.c
 *				or a path string for omfansic.c
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsAppendRawFile(
			fileHandleType		stream, 
			omfSessionHdl_t	session,
			omfFileRev_t		rev, 
			omfHdl_t				*result)
{
	omfHdl_t        file;
	CMRefCon        myRefCon;
	CMContainer     container;
	omfObject_t     head;
	omfVersionType_t fileVersion;
	omfDDefObj_t    datakind;
	omfTimeStamp_t  create_timestamp;
	omfErr_t		status;
	omfProperty_t	idProp;

	XPROTECT(NULL)
	{
		if (stream == NULL) 
		  RAISE(OM_ERR_NULL_PARAM);

		if (session == NULL)
		  RAISE(OM_ERR_BAD_SESSION);	

		if(session->cookie != SESSION_COOKIE)
		  RAISE(OM_ERR_BAD_SESSION);	

		CHECK(InitFileHandle(session, kOmfiMedia, &file));	
		*result = file;
		clearBentoErrors(file);
		file->setrev = rev;
		file->openType = kOmCreate;
		
		myRefCon = (*session->ioFuncs.createRefConFunc)(session->BentoSession, 
											 (char *) stream, NULL, file);
		omfCheckBentoRaiseError(file, OM_ERR_BADSESSIONMETA);
	
		container = CMOpenNewContainer(session->BentoSession, myRefCon,
												"OMFI",
												(CMContainerUseMode) kCMAppendRaw,/* DIFF */
												1, 0, 0);
		omfCheckBentoRaiseError(file, OM_ERR_BADOPEN);
		if(container == NULL)
			RAISE(OM_ERR_BADOPEN);
	
		file->container = container;
		head = (omfObject_t)cmFindObject(((ContainerPtr) container)->toc, 1);
		omfCheckBentoRaiseError(file, OM_ERR_BADHEAD);
	
		file->head = head;
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;
		if (omfsWriteClassID(file, head, idProp, "HEAD") != OM_ERR_NONE)
			RAISE(OM_ERR_NOBYTEORDER);
	
		if ((rev == kOmfRev1x) || rev == kOmfRevIMA)
		  {
			fileVersion.major = 1;
			fileVersion.minor = 0;
			if(rev == kOmfRev1x)
				CMSetContainerVersion1(file->container);
		  }
		else if (rev == kOmfRev2x)
		  {
			fileVersion.major = 2;
			fileVersion.minor = 0;
		  }
		 
		if (omfsWriteVersionType(file, head, OMVersion, fileVersion))
		  {
			RAISE(OM_ERR_BADHEAD);
		  }
	
		/* 'MM' or 'II' see OMVars.h, OMDefs.h */
		if (omfsPutByteOrder(file, head, file->byteOrder) != OM_ERR_NONE)
		  {
			RAISE(OM_ERR_NOBYTEORDER);
		  }

		/* Write HEAD:LastModified */
		omfsGetDateTime(&create_timestamp);
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		  {
			 CHECK(omfsWriteTimeStamp(file, file->head, OMLastModified,
											  create_timestamp));
		  }
		else /* kOmf2x */
		  {
			 CHECK(omfsWriteTimeStamp(file, file->head, OMHEADLastModified,
											  create_timestamp));
		  }

		/**** File object is now correct enough to pass to OMFI functions 
		 **** to read some data. *****/
		CHECK(BuildPropIDCache(file));
		CHECK(BuildTypeIDCache(file));
	
		/* Since new file: create datakind objects for all required datakinds
	    * defined in the specification and add to HEAD:Definitions,
		 * then initialize file cache with these objects.  These should only
		 * be created if it is a kOmfRev2x file.
	    */
		if (rev == kOmfRev2x)
		{
		  /* Create Datakind cache; the omfiDatakindNew() calls will add
			* entries to it.
			*/
		  CHECK(omfsNewDefTable(file, DEFAULT_NUM_DATAKIND_DEFS, &(file->datakinds)));

		  CHECK(omfiDatakindNew(file, NODATAKIND, &datakind));
		  CHECK(omfiDatakindNew(file, CHARKIND, &datakind));
		  CHECK(omfiDatakindNew(file, UINT8KIND, &datakind));
		  CHECK(omfiDatakindNew(file, UINT16KIND, &datakind));
		  CHECK(omfiDatakindNew(file, UINT32KIND, &datakind));
		  CHECK(omfiDatakindNew(file, INT8KIND, &datakind));
		  CHECK(omfiDatakindNew(file, INT16KIND, &datakind));
		  CHECK(omfiDatakindNew(file, INT32KIND, &datakind));
		  CHECK(omfiDatakindNew(file, RATIONALKIND, &datakind));
		  CHECK(omfiDatakindNew(file, BOOLEANKIND, &datakind));
		  CHECK(omfiDatakindNew(file, STRINGKIND, &datakind));
		  CHECK(omfiDatakindNew(file, COLORSPACEKIND, &datakind));
		  CHECK(omfiDatakindNew(file, COLORKIND, &datakind));
		  CHECK(omfiDatakindNew(file, DISTANCEKIND, &datakind));
		  CHECK(omfiDatakindNew(file, POINTKIND, &datakind));
		  CHECK(omfiDatakindNew(file, DIRECTIONKIND, &datakind));
		  CHECK(omfiDatakindNew(file, POLYKIND, &datakind));
		  CHECK(omfiDatakindNew(file, MATTEKIND, &datakind));
		  CHECK(omfiDatakindNew(file, PICTUREKIND, &datakind));
		  CHECK(omfiDatakindNew(file, SOUNDKIND, &datakind));
		  CHECK(omfiDatakindNew(file, PICTWITHMATTEKIND, &datakind));
		  CHECK(omfiDatakindNew(file, STEREOKIND, &datakind));
		  CHECK(omfiDatakindNew(file, TIMECODEKIND, &datakind));
		  CHECK(omfiDatakindNew(file, EDGECODEKIND, &datakind));

		  CHECK(BuildEffectDefCache(file, 
							DEFAULT_NUM_EFFECT_DEFS, &(file->effectDefs)));
		}
	
		/* We now use datakinds in the whole API, not just 2.x
		 */
		omfiDatakindLookup(file, NODATAKIND, &file->nilKind, &status);
		CHECK(status);
		omfiDatakindLookup(file, PICTUREKIND, &file->pictureKind, &status);
		CHECK(status);
		omfiDatakindLookup(file, SOUNDKIND, &file->soundKind, &status);
		CHECK(status);

		CHECK(omfsNewUIDTable(file, DEFAULT_NUM_MOBS, &(file->mobs)));
		CHECK(omfsSetTableDispose(file->mobs, &MobDisposeMap1X));
		CHECK(omfsNewUIDTable(file, DEFAULT_NUM_DATAOBJ, &(file->dataObjs)));
	
		/* this is a private property only to be used by the toolkit */
		if (rev == kOmfRev1x) /* fake out toolkit version for old OMCntr.c */
		{
			omfVersionType_t tmpVersion = {1,0};
			CHECK(omfsWriteVersionType(file, head, OMToolkitVersion,
												tmpVersion));
		}
		else if(rev == kOmfRevIMA)
		{
			omfVersionType_t tmpVersion = {1,0};

			/* JeffB: Loss of precision here is OK for 1.x era files
			 * (should be gone by rev 256 of toolkit) 
			 */
			tmpVersion.major = (char)omfiToolkitVersion.major;
			tmpVersion.minor = (char)omfiToolkitVersion.minor;
			CHECK(omfsWriteVersionType(file, head, OMToolkitVersion,
												tmpVersion));
		}
		else
		{
			omfPosition_t	field1, field2, field3;
			omfInt32		major, minor, tertiary;
			
			omfsCvtInt32toPosition(0, field1);
			omfsCvtInt32toPosition(sizeof(omfInt32), field2);
			omfsCvtInt32toPosition(2 * sizeof(omfInt32), field3);
			major = omfiToolkitVersion.major;
			minor = omfiToolkitVersion.minor;
			tertiary = omfiToolkitVersion.tertiary;
			CHECK(OMWriteProp(file, head, OMToolkitVersion, field1, OMInt32Array,
								sizeof(omfInt32), &major));
			CHECK(OMWriteProp(file, head, OMToolkitVersion, field2,
								OMInt32Array, sizeof(omfInt32), &minor));
			CHECK(OMWriteProp(file, head, OMToolkitVersion, field3,
								OMInt32Array, sizeof(omfInt32), &tertiary));
		}

		AddIDENTObject(file, file->session->ident);
	}

	XEXCEPT
	  {
		 /* Reset to previous state before returning with error */
		 if (session && file)
		{
			/* Added by MT: you need to save the pointer before deleting
			 * the  object
			 */
			struct omfiFile *pSavedAddr = file->prevFile;

			if (session->topFile)
				omOptFree(NULL, session->topFile);
			     
			/* Set to previous file or NULL */
			session->topFile = pSavedAddr;
		}
	  }
	XEND;
	
	return (OM_ERR_NONE);
}



/************************
 * Function: omfsCreateRawFile
 *
 * 	Creates a non-OMFI file, returning an opaque file handle.
 *		This routine is provided for rare cases where you may need
 *		to transfer data files in a non-OMFI format.
 *
 * Argument Notes:
 *		fh -- The same file structure given to omfsCreateFile.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *
 * See Also:
 */
omfErr_t omfsCreateRawFile(
			fileHandleType	fh, 
			omfInt16			fhSize,
			omfSessionHdl_t	session, 
			omfCodecID_t		codecID,
			omfHdl_t			*result)
{	
	omfHdl_t	file;
	
	*result = NULL;
	XPROTECT(NULL)
	{
		if(session->cookie != SESSION_COOKIE)
		  RAISE(OM_ERR_BAD_SESSION);	

		CHECK(InitFileHandle(session, kForeignMedia, &file));
		file->openType = kOmCreate;
		file->rawFileDesc = omOptMalloc(file, fhSize);
		file->rawCodecID = codecID;
		file->nilKind = (omfDDefObj_t)0;
		file->pictureKind = (omfDDefObj_t)1;
		file->soundKind = (omfDDefObj_t)2;
		if (file->rawFileDesc == NULL)
			RAISE(OM_ERR_BADOPEN);
		memcpy((char *)file->rawFileDesc, fh, fhSize);
		*result = file;
	}
	XEXCEPT
	  {
		/* Reset to previous state before returning with error */
		if (session && file)
		{
			/* Added by NW (!) you need to save the pointer before deleting
			 * the  object
			 */
			struct omfiFile *pSavedAddr = file->prevFile;

			if (session->topFile)
				omOptFree(NULL, session->topFile);

			/* Set to previous file or NULL */
			session->topFile = pSavedAddr;
		}
	}
	XEND;

	return (OM_ERR_NONE);
}

omfErr_t omfsPatch1xMobs(omfHdl_t file)
{
  omfSearchCrit_t searchCrit;
  omfMSlotObj_t   track = NULL;
  omfSegObj_t     segment = NULL;
  omfMobObj_t     compMob;
  omfIterHdl_t    mobIter = NULL, trackIter = NULL;
  omfInt32        groupLength, segLength;
  omfInt32        loop, trackLoop, matches;
  omfInt32        numMobs, numTracks;
  omfLength_t     length;
  omfRational_t   editRate;
  omfErr_t        tmpError = OM_ERR_NONE;
  
  XPROTECT(file)
	 {
	  /* If 1.x file, patch Composition Mob GroupLength properties */
	  if (file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
		 {
			/* Iterator over mobs */
			CHECK(omfiIteratorAlloc(file, &mobIter));
			CHECK(omfiGetNumMobs(file, kAllMob, &numMobs));
			for (loop = 0; loop < numMobs; loop++)
			  {
				 searchCrit.searchTag = kByMobKind;
				 searchCrit.tags.mobKind = kAllMob;

				 /* 
				  * If this fails, then there is probably a SCLP in a mob
				  * somewhere in the file that points to a mob that doesn't 
				  * exist in the file.  Continue for now, and remember the
				  * error.
				  */
				 if (omfiGetNextMob(mobIter, &searchCrit, &compMob) != OM_ERR_NONE)
					{
					  tmpError = OM_ERR_MOB_NOT_FOUND;
					  break;
					}

				 /* Iterate over tracks in a mob */
				 CHECK(omfiIteratorAlloc(file, &trackIter));
				 CHECK(omfsReadInt32(file, compMob, OMTRKGGroupLength, 
										  &groupLength));
				 CHECK(omfiMobGetNumTracks(file, compMob, &numTracks));
				 for (trackLoop = 0; trackLoop < numTracks; trackLoop++)
					{
					  CHECK(omfiMobGetNextTrack(trackIter, compMob, NULL, &track));
					  CHECK(omfiTrackGetInfo(file, compMob, track,
													 NULL, 0, NULL, NULL, NULL,
													 &segment));
					  /* 
						* 1) Patch each component to make sure that it has an 
						*    editrate property.
						*/
					  CHECK(omfsReadExactEditRate(file, segment, 
															OMCPNTEditRate, &editRate)); 
					  CHECK(omfiMobMatchAndExecute(file, segment,
															 0, /* level */
															 isObjFunc, NULL,
															 set1xEditrate, &editRate,
															 &matches));

					  /* 
						* 2) Patch the Mob's TRKG:GroupLength, if necessary.
						*/
					  CHECK(omfiComponentGetLength(file, segment, &length));
					  /* Truncation not a problem since 1.x file */
					  CHECK(omfsTruncInt64toInt32(length, &segLength));	/* OK 1.x */
					  /* If length of segment is greater, patch mob group length */
					  if (segLength > groupLength)
						 {
							CHECK(omfsWriteInt32(file, compMob, OMTRKGGroupLength, 
													  segLength));
						 }
					}
				 CHECK(omfiIteratorDispose(file, trackIter));
			  }
			CHECK(omfiIteratorDispose(file, mobIter));
			mobIter = NULL;
		 }
	}
  XEXCEPT
	 {
	 }
  XEND;

  if (tmpError == OM_ERR_MOB_NOT_FOUND)
	 return(OM_ERR_MOB_NOT_FOUND);
  else
	 return(OM_ERR_NONE);
}

/************************
 * Function: omfsCloseFile
 *
 * 	Close the given file, closing any media still open on the
 *		file, and disposing of memory allocated for the file.
 * 	The file may be an OMFI or a non-OMFI opaque handle.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_MEDIA_CANNOT_CLOSE - If there is media to be closed, and the
 *			internal callback has not been installed.
 *		OM_ERR_BADCLOSE - Error closing a non-OMFI file.
 */
omfErr_t omfsCloseFile(omfHdl_t file)
{
  omfSessionHdl_t	session;
  omfHdl_t        	tstFile;
  omfMediaHdl_t		tstMedia;
  omfMediaHdl_t		tmpMedia;
  omfErr_t			status, totalStatus;
	
  omfAssertValidFHdl(file);
  clearBentoErrors(file);
  session = file->session;
  omfAssert((file->topMedia == NULL) || (file->closeMediaProc != NULL), 
				  file, OM_ERR_MEDIA_CANNOT_CLOSE);
  
  XPROTECT(file)
	{
	  if (file->closeMediaProc != NULL)
		{
		  totalStatus = OM_ERR_NONE;

		  tstMedia = file->topMedia;
		  while(tstMedia !=  NULL)
			{
                /* The following advances the while exit condition */
				if ((tstMedia != NULL) && (tstMedia->pvt != NULL))
					{
					tmpMedia = tstMedia->pvt->nextMedia;
					}
				else
					{
					tmpMedia = NULL;
					}

				status = ((*file->closeMediaProc) (tstMedia));
				if(status != OM_ERR_NONE)
					totalStatus = status;

				tstMedia = tmpMedia;
			}
		 CHECK(totalStatus);
		}

	  if (file == session->topFile)
		session->topFile = file->prevFile;
	  else
		{
		  tstFile = session->topFile;
		  while (tstFile && tstFile->prevFile != NULL)
			{
			  if (tstFile->prevFile == file)
				 {
					tstFile->prevFile = file->prevFile;
					break;
				 }
			  else
					tstFile = tstFile->prevFile;
			}
		}

		
	  if (file->fmt == kOmfiMedia)
		{
		  switch(file->openType)
			{
			case kOmCreate:
			case kOmModify:
			  CHECK(UpdateFileCLSD(file));
			  break;
			
			default:
			  break;
			}
		
		  if (file->properties)
			omfsTableDisposeAll(file->properties);
		  if (file->types)
			omfsTableDisposeAll(file->types);
		  if (file->propReverse)
			omfsTableDisposeAll(file->propReverse);
		  if (file->typeReverse)
			omfsTableDisposeAll(file->typeReverse);
		  if (file->mobs)
			{
			  omfsTableDisposeAll(file->mobs);
			}
		  if (file->dataObjs)
			omfsTableDispose(file->dataObjs);
		  if (file->datakinds)
			omfsTableDispose(file->datakinds);
		  if (file->effectDefs)
			omfsTableDispose(file->effectDefs);
		}
		else
		{
			if(file->rawFileDesc != NULL)
				omOptFree(NULL, file->rawFileDesc);
		}

#ifdef OMFI_ERROR_TRACE
	  if(file->stackTrace != NULL)
	  {
			omOptFree(NULL, file->stackTrace);
		}
	  file->stackTraceSize = 0;
	  file->stackTrace = NULL;
#endif
	  if (file->fmt == kOmfiMedia)
		{
 		  if (file->BentoErrorNumber)
			CMAbortContainer(file->container);
		  else
			CMCloseContainer(file->container);
		  if (file->BentoErrorRaised)
			{
				if(file->BentoErrorNumber == CM_err_BadWrite)
				{
					RAISE(OM_ERR_CONTAINERWRITE);
				}
				else
				{
					RAISE(OM_ERR_BADCLOSE);
				}
			}
		}
	  file->cookie = 0;
	}
  XEXCEPT
  {
  }
	XEND

	  omOptFree(NULL, file);

  return (OM_ERR_NONE);
}

/************************
 * Function: ReadTrackMapping	(INTERNAL)
 *
 * 	Reads the tract structure from a 1.x generation track group, and
 *		and saves the mapping table in the mob index, so that tracks may
 *		be written out using the same track labels.
 *
 *		Since the toolkit uses 2.x track IDs (unique to the track group)
 *		in the API, a mapping is required in order to preserve 1.x track
 *		label (unique only for a particular media type).
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
static omfErr_t ReadTrackMapping(
			omfHdl_t		file, 
			omfObject_t	mob)
{
	omfObject_t		trak, cpnt;
	omfTrackType_t	type;
	omfInt16			i, numTracks1X, trackNum1X = 0;
	omfDDefObj_t	mediaKind;
	omfUID_t		uid;
	omfErr_t        omfError = OM_ERR_NONE;
	
	if(file->setrev == kOmfRev2x)
		return(OM_ERR_NONE);
		
	XPROTECT(file)
	{
		CHECK(omfsReadUID(file, mob, OMMOBJMobID, &uid));
		numTracks1X = (omfInt16)omfsLengthObjRefArray(file, mob, OMTRKGTracks);
		for(i = 1; i <= numTracks1X; i++)
		{
			CHECK(omfsGetNthObjRefArray(file, mob, OMTRKGTracks, &trak, i));
			omfError = omfsReadInt16(file, trak, OMTRAKLabelNumber, 
									 &trackNum1X);
		    /* It's possible that a 1.x track may not have a label */
			if (omfError != OM_ERR_PROP_NOT_PRESENT)
			{
				CHECK(omfsReadObjRef(file, trak, OMTRAKTrackComponent, &cpnt));
				CHECK(omfsReadTrackType(file, cpnt, OMCPNTTrackKind, &type));
				mediaKind = TrackTypeToMediaKind(file, type);
				CHECK(AddTrackMappingExact(file, uid, (omfTrackID_t)i, 
										   mediaKind, trackNum1X));

				/* Release Bento reference, so the useCount is decremented */
				CMReleaseObject((CMObject)cpnt);
			}
			/* Release Bento reference, so the useCount is decremented */
			CMReleaseObject((CMObject)trak);
		}
	}	
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);	
}

/************************
 * Function: omfsInternOpenFile	(INTERNAL)
 *
 * 	Open a file, given a container use mode and type.   This
 *		is the function which takes care of most of the operations
 *		involved in opening the file.   The wrapper routines 
 *		omfsOpenFile() and omfsModifyFile() should be called instead
 *		by client applications.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BAD_SESSION - Session handle is invalid.
 *		OM_ERR_BADHEAD - Head object is invalid or missing.
 *		OM_ERR_NOTOMFIFILE - The file is not an OMFI file.
 *		OM_ERR_BADOPEN - The file is an OMFI file, but fails to open.
 */
static omfErr_t omfsInternOpenFile(fileHandleType stream, 
								   omfSessionHdl_t session,
								   CMContainerUseMode useMode, 
								   openType_t type, 
								   omfHdl_t * result)
{
	omfHdl_t        	file;
	CMRefCon        	myRefCon = NULL;
	omfUInt16       	byte_order;
	omfClassID_t  		omfiID;
	omfVersionType_t 	fileVersion, headVersion;
	omfProductVersion_t	tkFileVers;
	CMContainer     	container = NULL;
	ContainerPtr		bentoCntPtr;
	omfObject_t     	head;
	omfErr_t      		status, finalStatus = OM_ERR_NONE;
	omfInt32				mobTableSize, n, siz, errnum;
	omfObject_t			obj;
	omfUID_t				uid;
	omfObjIndexElement_t elem;
	omfProperty_t		prop;
	
	if (session == NULL)
	  return(OM_ERR_BAD_SESSION);	

	if(session->cookie != SESSION_COOKIE)
		  return(OM_ERR_BAD_SESSION);	

	status = InitFileHandle(session, kOmfiMedia, &file);
	if (status != OM_ERR_NONE)
		return (status);

	XPROTECT(file)
	{
	   if (result)
		  *result = NULL;
		else
		  RAISE(OM_ERR_NULL_PARAM);

		clearBentoErrors(file);
		file->openType = type;

		if (stream == NULL) 
		  RAISE(OM_ERR_NULL_PARAM);
		
		myRefCon = (*session->ioFuncs.createRefConFunc)(session->BentoSession, 
											 (char *) stream, NULL, file);
		omfCheckBentoRaiseError(file, OM_ERR_BADSESSIONMETA);
	
		/*
		 * Open the container
		 */
		file->BentoErrorNumber = 0;
		session->BentoErrorNumber = 0;
		container = CMOpenContainer(session->BentoSession, myRefCon, "OMFI", 
									useMode);
		file->container = container;
		if (container == NULL)
		{
			if(file->BentoErrorRaised)
				errnum = file->BentoErrorNumber;
			else if(session->BentoErrorRaised)
				errnum = session->BentoErrorNumber;
			if (errnum== CM_err_BadMagicBytes)
			{
				session->BentoSession = NULL;
				RAISE(OM_ERR_NOTOMFIFILE);
			}
			else if(session->BentoErrorRaised && (session->BentoErrorNumber == CM_err_GenericMessage))
			{
				RAISE(OM_ERR_FILE_NOT_FOUND);
			}
			else
			{
				RAISE(OM_ERR_BADOPEN);
			}
		}
		
		/* This is a little bit of a catch-22, since we're setting
		 * the rev "before" we can verify that the HEAD object is a head
		 * object.   But, we need head object to get the rev! */
		head = (omfObject_t) cmFindObject(((ContainerPtr) container)->toc, 1);
		omfCheckBentoRaiseError(file, OM_ERR_BADHEAD);
		if (head == NULL)
		  RAISE(OM_ERR_BADHEAD);
	
		/* Try looking for the 2.x version first */
		file->setrev = kOmfRev2x;
		if (omfsIsPropPresent(file, head, OMVersion, OMVersionType))
		  {
			omfsReadVersionType(file, head, OMVersion, &fileVersion);
			if ((fileVersion.major == 2) && (fileVersion.minor == 0))
			  file->setrev = kOmfRev2x;
			else if (fileVersion.major == 1)
			  file->setrev = kOmfRev1x;
			else if ((fileVersion.major == 2) && (fileVersion.minor > 0)
					 || (fileVersion.major > 2))
			  RAISE(OM_ERR_FILEREV_NOT_SUPP);
		  }
		else /* 2.x Version property does not exist, try reading 1.x version property */
		{
			file->setrev = kOmfRev1x;
			if (omfsIsPropPresent(file, head, OMVersion, OMVersionType))
			  {
				omfsReadVersionType(file, head, OMVersion, &fileVersion);
				if ((fileVersion.major == 2) && (fileVersion.minor == 0))
				  file->setrev = kOmfRev2x;
				else if (fileVersion.major == 1)
				  file->setrev = kOmfRev1x;
				else if ((fileVersion.major == 2) && (fileVersion.minor > 0)
						 || (fileVersion.major > 2))
				  RAISE(OM_ERR_FILEREV_NOT_SUPP);
			  }
			else /* Version property does not exist, assume < 2.0 */
			  file->setrev = kOmfRev1x;
		}
		  
		if(file->setrev == kOmfRev1x)
		{
			bentoCntPtr = (ContainerPtr)container;
		  	if(bentoCntPtr->majorVersion >= 2)
		  		file->setrev = kOmfRevIMA;
		}
		
		/*
		 * Head object is the one with bento ID == 1 and is of the correct
		 * type.
		 */
		file->head = head;
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			prop = OMObjID;
		else
			prop = OMOOBJObjClass;
		if (omfsReadClassID(file, head, prop, omfiID) != OM_ERR_NONE)
		  RAISE(OM_ERR_NOTOMFIFILE);
		if (!streq(omfiID, "HEAD"))
		  RAISE(OM_ERR_NOTOMFIFILE);
		
		/*
		 * Getting the byte order of the HEAD object sets the default for the
		 * file
		 */
		if (omfsGetByteOrder(file, head, (omfInt16 *)&byte_order) != OM_ERR_NONE)
			RAISE(OM_ERR_NOBYTEORDER);
		file->byteOrder = byte_order;
		
		CHECK(BuildPropIDCache(file));
		CHECK(BuildTypeIDCache(file));
	
		if(file->setrev == kOmfRev2x)
		  {
			CHECK(BuildDatakindCache(file, 
				    DEFAULT_NUM_DATAKIND_DEFS, &(file->datakinds)));
			CHECK(BuildEffectDefCache(file, 
						DEFAULT_NUM_EFFECT_DEFS, &(file->effectDefs)));
		  }
	
		/* We now use datakinds in the whole API, not just 2.x
		 */
		omfiDatakindLookup(file, NODATAKIND, &file->nilKind, &status);
		CHECK(status);
		omfiDatakindLookup(file, PICTUREKIND, &file->pictureKind, &status);
		CHECK(status);
		omfiDatakindLookup(file, SOUNDKIND, &file->soundKind, &status);
		CHECK(status);

		/**** File object is now correct enough to pass to OMFI functions to
		 * read some data. *****/
	
		/*
		 * Verify that the Toolkit Version of the opened file isn't 3.0 or
		 * greater
		 */
		;
		if (omfsIsPropPresent(file, head, OMToolkitVersion, OMVersionType))
		{
			omfsReadVersionType(file, head, OMToolkitVersion, &headVersion);
			tkFileVers.major = headVersion.major;
			tkFileVers.minor = headVersion.minor;
			tkFileVers.tertiary = 0;
			tkFileVers.type = kVersionUnknown;
			tkFileVers.patchLevel = 0;
			if (headVersion.major >= 3)
			  RAISE(OM_ERR_FILEREV_NOT_SUPP);
			if ((file->setrev == kOmfRev2x) &&
				 (headVersion.major == 2) &&
				 (headVersion.minor == 0))
			  RAISE(OM_ERR_FILE_REV_200);
		}
		if (omfsIsPropPresent(file, head, OMToolkitVersion, OMInt32Array))
		{
			omfPosition_t	field1, field2, field3;
			omfInt32		major, minor, tertiary;

			omfsCvtInt32toPosition(0, field1);
			omfsCvtInt32toPosition(sizeof(omfInt32), field2);
			omfsCvtInt32toPosition(2 * sizeof(omfInt32), field3);

			CHECK(OMReadProp(file, head, OMToolkitVersion, field1, kSwabIfNeeded,
								  OMInt32Array, sizeof(omfInt32), &major));
			CHECK(OMReadProp(file, head, OMToolkitVersion, field2, kSwabIfNeeded,
								OMInt32Array, sizeof(omfInt32), &minor));
			CHECK(OMReadProp(file, head, OMToolkitVersion, field3, kSwabIfNeeded, 
							OMInt32Array, sizeof(omfInt32), &tertiary));
			tkFileVers.major = (omfUInt16)major;
			tkFileVers.minor = (omfUInt16)minor;
			tkFileVers.tertiary = (omfUInt16)tertiary;
			tkFileVers.type = kVersionUnknown;
			tkFileVers.patchLevel = 0;
			
			if (tkFileVers.major >= 3)
			  RAISE(OM_ERR_FILEREV_NOT_SUPP);
			if ((tkFileVers.major == 2) &&
				 (tkFileVers.minor == 0) &&
				 (tkFileVers.tertiary == 0))
			  RAISE(OM_ERR_FILE_REV_200);
		}

		/* Set up the class dictionary cache */
		CHECK(UpdateLocalCLSD(file));

		/*
		 * Make a table of all of the source & composition mobs
		 */
		if(file->setrev == kOmfRev2x)
			mobTableSize = omfsLengthObjRefArray(file, head, OMHEADMobs);
		else
		{
			mobTableSize = omfsLengthObjIndex(file, head, OMCompositionMobs);
			mobTableSize += omfsLengthObjIndex(file, head, OMSourceMobs);
		}
		mobTableSize *= 2; /* Allow for some growth */
		if(mobTableSize < DEFAULT_NUM_MOBS)
			mobTableSize = DEFAULT_NUM_MOBS;
		CHECK(omfsNewUIDTable(file, mobTableSize, &(file->mobs)));
		CHECK(omfsSetTableDispose(file->mobs, &MobDisposeMap1X));

		if(file->setrev == kOmfRev2x)
		{
			siz = omfsLengthObjRefArray(file, head, OMHEADMobs);
			for(n = 1; n <= siz; n++)
			{
				CHECK(omfsGetNthObjRefArray(file, head, OMHEADMobs, &obj, n));
				CHECK(omfsReadUID(file, obj, OMMOBJMobID, &uid));
				CHECK(AddMobTableEntry(file, obj, uid, kOmTableDupAddDup));
			}
		}
		else
		{
			siz = omfsLengthObjIndex(file, head, OMCompositionMobs);
			for(n = 1; n <= siz; n++)
			{
				CHECK(omfsGetNthObjIndex(file, head, OMCompositionMobs, 
										 &elem, n));
				CHECK(AddMobTableEntry(file, elem.Mob, elem.ID, 
									   kOmTableDupAddDup));
				if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
				{
					CHECK(ReadTrackMapping(file, elem.Mob));
				}
			}
			siz = omfsLengthObjIndex(file, head, OMSourceMobs);
			for(n = 1; n <= siz; n++)
			{
				CHECK(omfsGetNthObjIndex(file, head, OMSourceMobs, &elem, n) );
				CHECK(AddMobTableEntry(file, elem.Mob, elem.ID, 
									   kOmTableDupAddDup));
				if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
				{
					CHECK(ReadTrackMapping(file, elem.Mob));
				}
			}
		}

		/* Register any other properties and types in the file in the cache */
		CHECK(UpdateLocalTypesProps(file));

		if(file->session->openCB != NULL)
		{
			CHECK((*file->session->openCB)(file));
		}

		CHECK(BuildMediaCache(file));
		*result = file;
	}
	XEXCEPT
	{
	  finalStatus = XCODE();
	  NO_PROPAGATE(); /* Rely on finalStatus variable, as file will be NULL */

		/* Reset to previous state before returning with error */
		if (session && file)
		{
		  if (file->properties)
			 omfsTableDisposeAll(file->properties);
		  if (file->types)
			 omfsTableDisposeAll(file->types);
		  if (file->propReverse)
			 omfsTableDisposeAll(file->propReverse);
		  if (file->typeReverse)
			 omfsTableDisposeAll(file->typeReverse);
		  if (file->mobs)
			 {
				omfsTableDisposeAll(file->mobs);
			 }
		  if (file->dataObjs)
			 omfsTableDispose(file->dataObjs);
		  if (file->datakinds)
			 omfsTableDispose(file->datakinds);
		  if (file->effectDefs)
			 omfsTableDispose(file->effectDefs);

#ifdef OMFI_ERROR_TRACE
		  if(file->stackTrace != NULL)
			 {
				omOptFree(NULL, file->stackTrace);
			 }
		  file->stackTraceSize = 0;
		  file->stackTrace = NULL;
#endif
		  if(file->container != NULL)
			 {
				CMAbortContainer(file->container);
			 }
		  /* Post 2.2 fix: Do not free myRefCon, it is already freed 
		   * else if(myRefCon != NULL)
			* CMFree(NULL, myRefCon, session->BentoSession);
         */

		  /* Set to previous file or NULL */
		  session->topFile = file->prevFile;
		  file->cookie = 0;
		  omOptFree(NULL, file);
		}
  		file = NULL;
	}
	XEND;

	return (finalStatus);
}


/************************
 * Function: BuildMediaCache (INTERNAL)
 *
 * 		This function is a callback from the openFile and createFile
 *		group of functions.  This callback exists in order to allow the
 *		media layer to be independant, and yet have information of its
 *		own in the opaque file handle.
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
static omfErr_t BuildMediaCache(omfHdl_t file)
{
	omfInt32						siz, n, dataObjTableSize;
	omfObject_t				obj;
	omfUID_t					uid;
	omfObjIndexElement_t	elem;
	
	XPROTECT(file)
	{
		if(file->setrev == kOmfRev2x)
		{
		   siz = omfsLengthObjRefArray(file, file->head, OMHEADMediaData);
			dataObjTableSize = (siz < DEFAULT_NUM_DATAOBJ ? 
									  DEFAULT_NUM_DATAOBJ : siz);
			CHECK(omfsNewUIDTable(file, dataObjTableSize, &(file->dataObjs)));
			for(n = 1; n <= siz; n++)
			  {
				 CHECK(omfsGetNthObjRefArray(file, file->head, OMHEADMediaData, 
													  &obj, n));
				 CHECK(omfsReadUID(file, obj, OMMDATMobID, &uid));
				 CHECK(omfsTableAddUID(file->dataObjs, uid,obj,kOmTableDupAddDup));
			  }
		 }
		else
		{
			siz = omfsLengthObjIndex(file, file->head, OMMediaData);
			dataObjTableSize = (siz < DEFAULT_NUM_DATAOBJ ? 
									  DEFAULT_NUM_DATAOBJ : siz);
			CHECK(omfsNewUIDTable(file, dataObjTableSize, &(file->dataObjs)));
			for(n = 1; n <= siz; n++)
					{
				CHECK(omfsGetNthObjIndex(file, file->head, OMMediaData, &elem, n));
				CHECK(omfsTableAddUID(file->dataObjs, elem.ID, elem.Mob, 
									  kOmTableDupAddDup) );
			}
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}


/************************
 * Function: MobDisposeMap1X	(INTERNAL)
 *
 * 	Disposes of the mapping table for a particular mob.  Handles
 *		the case where a 2.x mob has no mapping table.  This function
 *		is installed as a table dispose proc.
 *
 * Argument Notes:
 *		valuePtr - a mobTableEntry, passed in from the omTable code.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
static void MobDisposeMap1X(void *valuePtr)
{
  mobTableEntry_t *entry = (mobTableEntry_t *)valuePtr;

  if (entry->map1x)
	omfsTableDisposeAll(entry->map1x);
}


/************************
 * Function: InitFileHandle		(INTERNAL)
 *
 * 	Allocates an opaque file handle, and initializes its fields of 
 *		to reasonable values.
 *
 *		The open and create routines will initialize some of these fields
 *		to the real values, these are just for fields not initialized
 *		in some cases.
 *
 * Argument Notes:
 *		session -- The session containing this handle.
 *		fmt -- kOmfRev1x, kOmfRevIMA, or kOmfRev2x.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BAD_SESSION -- Bad session handle
 *		OM_ERR_NOMEMORY -- Ran out of memory allocating opaque handle.
 */
omfErr_t InitFileHandle(
			omfSessionHdl_t	session, 
			omfFileFormat_t 	fmt, 
			omfHdl_t				*result)
{
	omfHdl_t        file;

	if ((session == NULL) || (session->cookie != SESSION_COOKIE))
		return (OM_ERR_BAD_SESSION);
	XPROTECT(NULL)
	{
		session->BentoErrorNumber = 0;
		session->BentoErrorRaised = FALSE;
	
		file = (omfHdl_t) omOptMalloc(NULL, sizeof(struct omfiFile));
		if (file == NULL)
		{
			RAISE(OM_ERR_NOMEMORY);
		}
		*result = file;
	
		file->prevFile = session->topFile;
		session->topFile = file;
		file->session = session;
		file->fmt = fmt;
		file->closeMediaProc = NULL;
		file->container = NULL;
		if (fmt == kOmfiMedia)
		{
			file->head = NULL;
			file->byteOrder = OMNativeByteOrder;
		}
		file->topMedia = NULL;
		
		/* SemanticCheckEnable is used to stop recursion in checks */
		file->semanticCheckEnable = TRUE;	
		file->cookie = FILE_COOKIE;
		file->rawFile = NULL;
		file->rawFileDesc = NULL;
	
		/* Setrev should be overwritten by create or open */
		file->setrev = kOmfRev1x;		
		file->properties = NULL;
		file->types = NULL;
		file->propReverse = NULL;
		file->typeReverse = NULL;
		file->mobs = NULL;
		file->dataObjs = NULL;
		file->datakinds = NULL;
		file->effectDefs = NULL;
		file->byteOrderProp = 0;
		file->byteOrderType = 0;
				
		file->locatorFailureCallback = NULL;
		file->progressProc = NULL;
		file->customStreamFuncsExist = FALSE;
		file->compatSoftCodec = kToolkitCompressionEnable;
		/*
		 * allocate the property and type cache tables here, and call
		 * lookupProp or lookupType for every non-zero entry in the session
		 * table.  Clear the cache entries w/memset at the start so that we
		 * can tell bogus entries
		 */
#ifdef OMFI_ERROR_TRACE
		file->stackTrace = (char *)omOptMalloc(NULL, DEFAULT_STACK_TRACE_SIZE);
		if(file->stackTrace == NULL)
			RAISE(OM_ERR_NOMEMORY);
		file->stackTrace[0] = '\0';
		file->stackTraceSize = DEFAULT_STACK_TRACE_SIZE;
#endif
	}
	XEXCEPT
	{
	}
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: BuildPropIDCache	(INTERNAL)
 *
 * 	Builds a cache of property IDs to the Bento properties.  The
 *		session stores the propertyID to name mapping, along with some
 *		other read-only data.   The Bento ID however, is only valid for
 *		a particular file, and must be cached with each individual
 *		opaque file handle.
 *
 *		In order to use the cache lookup function, a property must be added
 *		to the session before any files are open.
 *
 *		The cache starts out with DEFAULT_NUM_PUBLIC_PROPS entries, but
 *		will grow to fit.
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
static omfInt32 BentoPropMap(void *temp)
{
  CMProperty *key = (CMProperty *)temp;

  return((omfInt32)*key);	/* Truncation does not matter, it's just a hash key */
}

static omfBool	BentoPropCompare(void *temp1, void *temp2)
{
  CMProperty *key1 = (CMProperty *)temp1;
  CMProperty *key2 = (CMProperty *)temp2;

  return( *key1 == *key2 );
}

static omfErr_t BuildPropIDCache(omfHdl_t file)
{
	omTable_t			*table, *destTable;
	omfBool				more;
	CMProperty			bentoID;
	omfProperty_t		prop;
	omTableIterate_t	iter;
	
	XPROTECT(file)
	{
		table = (file->setrev == kOmfRev2x ? file->session->properties2X
														: file->session->properties1X);
		CHECK(omfsNewPropertyTable(file, DEFAULT_NUM_PUBLIC_PROPS,
											&destTable));
		CHECK(omfsNewTable(file, sizeof(bentoID), BentoPropMap, BentoPropCompare,
								DEFAULT_NUM_PUBLIC_PROPS, &file->propReverse));
		CHECK(omfsTableFirstEntry(table, &iter, &more)); 
		while(more)
		{
			prop = *((omfProperty_t *)iter.key);
			bentoID = CvtPropertyToBento(file, prop);
			CHECK(omfsTableAddProperty(destTable, prop, &bentoID,
									sizeof(bentoID), kOmTableDupError));
			CHECK(omfsTableAddValueBlock(file->propReverse, &bentoID, sizeof(bentoID), &prop,
									sizeof(prop), kOmTableDupError));
		
			CHECK(omfsTableNextEntry(&iter, &more));
		}
		
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			file->byteOrderProp = CvtPropertyToBento(file, OMByteOrder);
		file->objTagProp1x = CvtPropertyToBento(file, OMObjID);
		file->objClassProp2x = CvtPropertyToBento(file, OMOOBJObjClass);
		
		/* NOW link the table into the file handle so that it may be used
		 * for lookups
		 */
		file->properties = destTable;
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: BuildTypeIDCache	(INTERNAL)
 *
 * 	Builds a cache of type IDs to the Bento types.  The
 *		session stores the typeID to name mapping, along with some
 *		other read-only data.   The Bento ID however, is only valid for
 *		a particular file, and must be cached with each individual
 *		opaque file handle.
 *
 *		In order to use the cache lookup function, a type must be added
 *		to the session before any files are open.
 *
 *		The cache starts out with DEFAULT_NUM_PUBLIC_TYPES entries, but
 *		will grow to fit.
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
static omfInt32 BentoTypeMap(void *temp)
{
  CMType *key = (CMType *)temp;

  return((omfInt32)*key);	/* Truncation does not matter, it's just a hash key */
}

static omfBool	BentoTypeCompare(void *temp1, void *temp2)
{
  CMType *key1 = (CMType *)temp1;
  CMType *key2 = (CMType *)temp2;

  return( *key1 == *key2 );
}


static omfErr_t BuildTypeIDCache(omfHdl_t file)
{
	omTable_t			*table, *destTable;
	omfBool				more;
	OMTypeCache			typeCache;
	omfType_t			type;
	omTableIterate_t	iter;
	
	XPROTECT(file)
	{
		table = (file->setrev == kOmfRev2x ? 
				 file->session->types2X : file->session->types1X);
		CHECK(omfsNewTypeTable(file, DEFAULT_NUM_PUBLIC_TYPES, &destTable));
		CHECK(omfsNewTable(file, sizeof(typeCache.bentoID), BentoTypeMap, BentoTypeCompare,
								DEFAULT_NUM_PUBLIC_TYPES, &file->typeReverse));
		CHECK(omfsTableFirstEntry(table, &iter, &more)); 
		while(more)
		{
			type = *((omfType_t *)iter.key);
			typeCache.bentoID = CvtTypeToBento(file, type, &typeCache.swab);
			CHECK(omfsTableAddType(destTable, type, &typeCache,
									sizeof(typeCache)));
			CHECK(omfsTableAddValueBlock(file->typeReverse,
									&typeCache.bentoID, sizeof(typeCache.bentoID),
									&type, sizeof(type), kOmTableDupError));
		
			CHECK(omfsTableNextEntry(&iter, &more));
		}
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			file->byteOrderType = CvtTypeToBento(file, OMInt16, NULL);
		file->objTagType1x = CvtTypeToBento(file, OMObjectTag, NULL);
		file->objClassType2x = CvtTypeToBento(file, OMClassID, NULL);
		
		/* NOW link the table into the file handle so that it may be used
		 * for lookups
		 */
		file->types = destTable;
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}


/************************
 * Function: GetNumDefs	(INTERNAL)
 *
 * 	This appears to be a debug function to test that a file is
 *		valid.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADHEAD -- File does not have a head object.
 */
static omfInt32 GetNumDefs(omfHdl_t file, 
						omfClassIDPtr_t tag)
{
	omfObject_t		head;
	omfObject_t 		tmpDef;
	char				omfiID[5];
	omfInt32			loop, numDef, size = 0;
	omfProperty_t	prop;
	
	XPROTECT(file)
	{
		if (omfsGetHeadObject(file, &head) != OM_ERR_NONE)
			RAISE(OM_ERR_BADHEAD);
	
		/* Search HEAD:Definitions index for DDEF or EDEF objects */
		numDef = omfsLengthObjRefArray(file, head, OMHEADDefinitionObjects);
		for (loop = 1; loop <= numDef; loop++)
		{
			CHECK(omfsGetNthObjRefArray(file, head, OMHEADDefinitionObjects, 
											   &tmpDef, loop));
			if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
				prop = OMObjID;
			else
				prop = OMOOBJObjClass;
				
			CHECK(omfsReadClassID(file,tmpDef,prop, (omfClassIDPtr_t) omfiID));
			/* HEAD:Definitions has DDEFs and EDEFs in it, look for tag */
			if (omfiID)
			{
				if (streq(omfiID, tag))
					size++;
			}
		}
	}
	XEXCEPT
	XEND_SPECIAL(0);

	return(size);
}

/************************
 * Function: UpdateFileCLSD	(INTERNAL)
 *
 * 	Updates the class dictionary object in the given file to contain
 *		the registered and private classes in the current session.
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
static omfErr_t UpdateFileCLSD(
			omfHdl_t file)
{
	omfBool				more, found, superFound;
	omTable_t			*table;
	omTableIterate_t	iter;
	OMClassDef			*entry, superEntry;
	omfErr_t				status = OM_ERR_NONE;
	omfObject_t			classDef, superDef;
	omfClassID_t		classID;
	omfInt32				n, dictSize;
	omfProperty_t		dictProp, classProp;
	
	if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
	{
		table = file->session->classDefs1X;
		dictProp = OMClassDictionary;
		classProp = OMCLSDClassID;
	}
	else
	{
		table = file->session->classDefs2X;
		dictProp = OMHEADClassDictionary;
		classProp = OMCLSDClass;
	}
		
	dictSize = omfsLengthObjRefArray(file, file->head, 
								dictProp);

	XPROTECT(file)
	{
		/* Create a CLSD object for every non-required class which is
		 * not already in the file.  The CLSD objects will not have a
		 * superclass objref at this point, as the superclass object
		 * may not have been created yet.  Store these clsd objects in
		 * the class dictionary.
		 */
		CHECK(omfsTableFirstEntry(table, &iter, &more));
		while(more)
		{
			entry = (OMClassDef *)iter.valuePtr;
			entry->clsd = NULL;
			if ((entry != NULL) && (entry->type != kClsRequired))
			{
				for(n = 1, found = FALSE; n <= dictSize && !found; n++)
				{
					CHECK(omfsGetNthObjRefArray(file, file->head, 
											dictProp, &classDef, n));
					CHECK(omfsReadClassID(file, classDef,  classProp, classID));
					
					if(streq(classID, entry->aClass))
						found = TRUE;
				}
				
				if(!found)
				{
					/* Write the class/superclass CLSD object to the file */
					CHECK(omfsObjectNew(file, OMClassCLSD, &classDef));
					CHECK(omfsWriteClassID(file, classDef,  classProp,
												  entry->aClass));
										
					entry->clsd = classDef;								
				}
				else
				{
					if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
						entry->clsd = classDef;
					else
						entry->clsd = NULL;
				}
			}
	
			CHECK(omfsTableNextEntry(&iter, &more));
		}

		/* Make a second pass over the class dictionary.  For every
		 * non-required class, fill in the superclass object reference.
		 *
		 * If the superclass is required, then create a stub CLSD with
		 * a name and no superclass reference.
		 *
		 * Otherwise, look up the superclass in the class dictionary in
		 * order to find its CLSD object, and write that.
		 */
		CHECK(omfsTableFirstEntry(table, &iter, &more));
		while(more)
		{
			entry = (OMClassDef *)iter.valuePtr;
			if ((entry != NULL) &&
				 (entry->type != kClsRequired) &&
				 (entry->clsd != NULL) &&
				 (entry->itsSuperClass != NULL) &&
				 (entry->itsSuperClass[0] != '\0'))
			{
				/* Find the entry for the superclass */
				CHECK(omfsTableClassIDLookup(table, entry->itsSuperClass,
														sizeof(superEntry),
														&superEntry, &superFound));
				XASSERT(superFound, OM_ERR_CANNOT_SAVE_CLSD);
				
												
				/* If the superclass is a required class, then create
				 * a "stub" class definition for the superclass object
				 * in order to anchor it.  If the superclass's clsd is NULL,
				 * then it is already in the CLSD???
				 */
				if(superEntry.type == kClsRequired)
				{
					CHECK(omfsObjectNew(file, OMClassCLSD, &classDef));
					CHECK(omfsWriteClassID(file, classDef,  classProp,
												  superEntry.aClass));
					CHECK(omfsAppendObjRefArray(file, file->head, 
													dictProp, classDef));
					superDef = classDef;
				}
				else   /* Otherwise, fill in the parent class reference */
					superDef = superEntry.clsd;

				XASSERT(superDef != NULL, OM_ERR_CANNOT_SAVE_CLSD);

				CHECK(omfsWriteObjRef(file, entry->clsd, OMCLSDParentClass, 
									  superDef));
				CHECK(omfsAppendObjRefArray(file, file->head, 
												dictProp, entry->clsd));
			 }

			/* If the class doesn't have a super class, still add it */
			else if ((entry != NULL) &&
				 (entry->type != kClsRequired) &&
				 (entry->clsd != NULL))
			  {
				 CHECK(omfsAppendObjRefArray(file, file->head, 
													  dictProp, entry->clsd));
			  }
	
			CHECK(omfsTableNextEntry(&iter, &more));
		}
	}
	XEXCEPT
	{
		RERAISE(OM_ERR_CANNOT_SAVE_CLSD);
	}
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: UpdateLocalCLSD	(INTERNAL)
 *
 * 	Updates the in-memory class dictionary object from the class
 *		dictionary object in the given file.
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


static omfErr_t UpdateLocalTypesProps(
			omfHdl_t file)
{
	CMContainer	cnt;
	char			*name, *className, *leafName;
	CMProperty	bentoProp;
	CMType		bentoType;
	omfBool		found;
	omfType_t	typeCode;
	omfProperty_t propCode;
	omfInt16		typeHdrLen = strlen(TYPE_HDR_BENTO);
	omfInt16		propHdrLen = strlen(PROP_HDR_BENTO);
	omfClassID_t refClass;
	omfValidRev_t tstRev;
	
	XPROTECT(file)
	{
		cnt = file->container;
		bentoType = CMGetNextType(file->container, 0);
		while(bentoType != 0)
		{
			name = CMGetGlobalName(bentoType);
			if((name != NULL) && strncmp(name, TYPE_HDR_BENTO, typeHdrLen) == 0)
			{
				bentoType = CMRegisterType(file->container, name);
				CvtTypeFromBento(file, bentoType, &found);		
				if(!found)
				{
				  /* LF-W: 12/10/96 - changed RevEither to be file specific
					*                  so duplicate types are registered.
					*/
				   if (file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
					  tstRev = kOmfTstRev1x;
					else /* kOmfRev2x */
					  tstRev = kOmfTstRev2x;
					CHECK(omfsRegisterDynamicType(file->session, tstRev,
															name+typeHdrLen,
															kNeverSwab,
															&typeCode));
				}
			}

			bentoType = CMGetNextType(file->container, bentoType);
		}
		
		bentoProp = CMGetNextProperty(file->container, 0);
		while(bentoProp != 0)
		{
			name = CMGetGlobalName(bentoProp);
			if((name != NULL) && strncmp(name, PROP_HDR_BENTO, propHdrLen) == 0)
			{
				bentoProp = CMRegisterProperty(file->container, name);
				CvtPropertyFromBento(file, bentoProp, &found);
				if(!found)
				{
					className = name+propHdrLen;
					leafName = strchr(className, ':');
					if(leafName != NULL)
					{
						leafName++;	/* Skip over ':' */
						strncpy(refClass, className, sizeof(refClass));
						CHECK(omfsRegisterDynamicProp(file->session, kOmfTstRevEither,
																leafName,
																refClass, OMNoType, kPropOptional,
																&propCode));
					}
				}
			}

			bentoProp = CMGetNextProperty(file->container, bentoProp);
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: BuildDatakindCache	(INTERNAL)
 *
 * 	Builds a cache of unique names to DDEF objects.  The DDEF
 *		object is only valid for a particular file, and must be
 *		cached with each individual opaque file handle.
 *
 *		In order to use the cache lookup function, a datakind definition
 *		object must be created before any files are open.
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
static omfErr_t BuildDatakindCache(
			omfHdl_t		file,
			omfInt32		defTableSize,
			omTable_t	**dest)
{
	omfObject_t head;
	omfInt32 defArrayLength, loop;
	omfDDefObj_t tmpDef;
	omfClassID_t omfiID;
	omfUniqueName_t name;
	omfProperty_t	prop;
	
	omfAssertValidFHdl(file);
	if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		prop = OMObjID;
	else
		prop = OMOOBJObjClass;

	/* Allocate Definition Hash Table */
	XPROTECT(file)
	{
		CHECK(omfsNewDefTable(file, defTableSize, dest));

		/* Get number of DDEFs or EDEFs from HEAD:Definitions */
		CHECK(omfsGetHeadObject(file, &head));
		defArrayLength = omfsLengthObjRefArray(file, head, 
											   OMHEADDefinitionObjects);
		/* Search HEAD:Definitions index for DDEF or EDEF object */
		for (loop = 1; loop<= defArrayLength; loop++)
		{
			CHECK(omfsGetNthObjRefArray(file, head, OMHEADDefinitionObjects, 
											   &tmpDef, loop));
			CHECK(omfsReadClassID(file, tmpDef,prop, (omfClassIDPtr_t)omfiID));
			/* HEAD:Definitions has DDEFs and EDEFs in it, look for DDEF */
			if (omfiID && streq(omfiID, "DDEF"))
			{
				CHECK(omfsReadUniqueName(file, tmpDef,
									OMDDEFDatakindID, name, OMUNIQUENAME_SIZE));
				CHECK(omfsTableAddDef(*dest, name, tmpDef));
			}
		} /* for loop */
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: BuildEffectDefCache	(INTERNAL)
 *
 * 	Builds a cache of unique names to effect definition objects.
 *		The EDEF object is only valid for a particular file, and must be
 *		cached with each individual opaque file handle.
 *
 *		In order to use the cache lookup function, an effect definition
 *		object must be created before any files are open.
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
static omfErr_t BuildEffectDefCache(omfHdl_t file,
							  omfInt32 defTableSize,
							  omTable_t **dest)
{
	omfObject_t head;
	omfInt32 defArrayLength, loop;
	omfDDefObj_t tmpDef;
	omfClassID_t omfiID;
	omfUniqueName_t name;
	omfProperty_t	idProp;
	
	omfAssertValidFHdl(file);

	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		/* Allocate Definition Hash Table */
		CHECK(omfsNewDefTable(file, defTableSize, dest));
	
		/* Get number of DDEFs or EDEFs from HEAD:Definitions */
		CHECK(omfsGetHeadObject(file, &head));
		defArrayLength = omfsLengthObjRefArray(file, head, 
											   OMHEADDefinitionObjects);
		/* Search HEAD:Definitions index for DDEF or EDEF object */
		for (loop = 1; loop<= defArrayLength; loop++)
		{
			CHECK(omfsGetNthObjRefArray(file, head, OMHEADDefinitionObjects, 
											   &tmpDef, loop));
				CHECK(omfsReadClassID(file, tmpDef,idProp,(omfClassIDPtr_t)omfiID));
				/* HEAD:Definitions has DDEFs and EDEFs in it, look for DDEF */
				if (omfiID && streq(omfiID, "EDEF"))
				{
					CHECK(omfsReadUniqueName(file, tmpDef,
										OMEDEFEffectID, name, OMUNIQUENAME_SIZE));
					CHECK(omfsTableAddDef(*dest, name, tmpDef));
				}
		} /* for loop */
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

static omfErr_t UpdateLocalCLSD(
			omfHdl_t file)
{
#if OMFI_ENABLE_SEMCHECK
   omfBool wasEnabled;
#endif
   omfBool				found;
	omTable_t			*table;
	OMClassDef			superEntry;
	omfObject_t			classDef, superClassDef;
	omfClassID_t		classID, superclassID;
	omfInt32				n, dictSize;
	omfProperty_t		dictProp, classProp;
	omfValidRev_t		tstRev;
	
	if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
	{
		table = file->session->classDefs1X;
		dictProp = OMClassDictionary;
		classProp = OMCLSDClassID;
		tstRev = kOmfTstRev1x;
	}
	else
	{
		table = file->session->classDefs2X;
		dictProp = OMHEADClassDictionary;
		classProp = OMCLSDClass;
		tstRev = kOmfTstRev2x;
	}
		
	dictSize = omfsLengthObjRefArray(file, file->head, 
								dictProp);

/* Some older applications made CLSD objects without a class ID.
 * These objects would fail if semantic checking was left enabled.
 */
#if OMFI_ENABLE_SEMCHECK
	wasEnabled = file->semanticCheckEnable;
	  if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		(void)omfsSemanticCheckOff(file);
#endif

	XPROTECT(file)
	{
		for(n = 1; n <= dictSize; n++)
		{
			CHECK(omfsGetNthObjRefArray(file, file->head, 
									dictProp, &classDef, n));
			CHECK(omfsReadClassID(file, classDef, classProp,
								 classID));
			if(omfsIsPropPresent(file, classDef, OMCLSDParentClass, OMObjRef))
			{
				CHECK(omfsReadObjRef(file, classDef, OMCLSDParentClass, 
									  &superClassDef));
				CHECK(omfsReadClassID(file, superClassDef, classProp,
									 superclassID));
			 }
			else
			  strcpy( (char*) superclassID, OMClassNone);

			CHECK(omfsTableClassIDLookup(table, classID,
												  sizeof(superEntry),
												  &superEntry, &found));
			if(!found)
			  {
				 CHECK(omfsNewClass(file->session, kClsPrivate,
										  tstRev, classID, superclassID));
			  }
		 } /* for */
	 }
	XEXCEPT
	{
		RERAISE(OM_ERR_CANNOT_LOAD_CLSD);
	}
	XEND
	
#if OMFI_ENABLE_SEMCHECK
	if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		if(wasEnabled)
		  (void)omfsSemanticCheckOn(file);
#endif
	
	return (OM_ERR_NONE);
}

static omfErr_t AddIDENTObject(omfHdl_t file, omfProductIdentification_t *identPtr)
{
	omfObject_t					identObj;
	omfTimeStamp_t				timestamp;
	omfProductIdentification_t	fiction;
	Boolean						dummyIDNT = FALSE;
	omfProductVersion_t			dummyVersion;
	omfInt16					byteOrder;
	
	omfAssertValidFHdl(file);
	XPROTECT(file)
	{		
		if(identPtr == NULL)
		{
			fiction.companyName = "Unknown";
			fiction.productName = "Unknown";
			fiction.productVersionString = NULL;
			fiction.productID = -1;
			fiction.platform = NULL;
			fiction.productVersion.major = 0;
			fiction.productVersion.minor = 0;
			fiction.productVersion.tertiary = 0;
			fiction.productVersion.patchLevel = 0;
			fiction.productVersion.type = kVersionUnknown;
			identPtr = &fiction;
			dummyIDNT = TRUE;
		}
		
		XASSERT(identPtr != NULL, OM_ERR_NEED_PRODUCT_IDENT);
		CHECK(omfsObjectNew(file, OMClassIDNT, &identObj));
		if(identPtr->companyName != NULL)
		{
			CHECK(omfsWriteString(file, identObj, OMIDNTCompanyName, identPtr->companyName));
		}
		if(identPtr->productName != NULL)
		{
			CHECK(omfsWriteString(file, identObj, OMIDNTProductName, identPtr->productName));
		}
		CHECK(omfsWriteProductVersionType(file, identObj, OMIDNTProductVersion,
										identPtr->productVersion));
		if(identPtr->productVersionString != NULL)
		{
			CHECK(omfsWriteString(file, identObj, OMIDNTProductVersionString,
										identPtr->productVersionString));
		}
		CHECK(omfsWriteInt32(file, identObj, OMIDNTProductID, identPtr->productID));
		if(dummyIDNT)
		{
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				CHECK(omfsReadInt16(file, file->head, OMByteOrder, &byteOrder));
				CHECK(omfsReadTimeStamp(file, file->head, OMLastModified,
											  &timestamp));
			  }
			else /* kOmfRev2x */
			  {
				CHECK(omfsReadInt16(file, file->head, OMHEADByteOrder, &byteOrder));
				CHECK(omfsReadTimeStamp(file, file->head, OMHEADLastModified,
												  &timestamp));
			  }

			CHECK(omfsWriteTimeStamp(file, identObj, OMIDNTDate, timestamp));
			dummyVersion.major = 0;
			dummyVersion.minor = 0;
			dummyVersion.tertiary = 0;
			dummyVersion.patchLevel = 0;
			dummyVersion.type = kVersionUnknown;
			CHECK(omfsWriteProductVersionType(file, identObj, OMIDNTToolkitVersion,
											dummyVersion));
		}
		else
		{
			byteOrder = file->byteOrder;
			omfsGetDateTime(&timestamp);
			CHECK(omfsWriteTimeStamp(file, identObj, OMIDNTDate, timestamp));
			CHECK(omfsWriteProductVersionType(file, identObj, OMIDNTToolkitVersion,
											omfiToolkitVersion));
		}
		
		CHECK(omfsWriteInt16(file, identObj, OMIDNTByteOrder, byteOrder));
		CHECK(omfsWriteString(file, identObj, OMIDNTPlatform, 
				(identPtr->platform != NULL ? identPtr->platform : "Unspecified")));
				
		CHECK(omfsAppendObjRefArray(file, file->head, OMHEADIdentList, identObj));
	}
	XEXCEPT
	{
	}
	XEND
	
	return(OM_ERR_NONE);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
