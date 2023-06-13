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

#ifndef _OMF_PRIVATE_FUNCTIONS_
#define _OMF_PRIVATE_FUNCTIONS_ 1

#include "omErr.h"
#include "omCntPvt.h"
#include "omUtils.h"
#include "omCodec.h"
#include "omCodCmn.h"
#include "omTable.h" 

#if PORT_LANG_CPLUSPLUS
extern          "C"
{
#endif

#define DEFAULT_NUM_DATAOBJ			200

/************************************************************
 *
 * General types and definitions
 *
 * xxxx_COOKIE defines are valus used to validate structures
 * at least statisticaly.
 *
 *************************************************************/

typedef enum
{
	kOmCreate, kOmModify, kOmOpenRead
}				openType_t;
	
struct omfiFile;

#define FILE_COOKIE		0x46494C45	/* 'FILE' */
#define SESSION_COOKIE	0x53455353	/* 'SESS' */
#define DEFTABLE_COOKIE	0x44454654	/* 'DEFT' */
#define ITER_COOKIE		0x49544552	/* 'ITER' */
#define SIMPLETRAK_COOKIE 0x5452414B	/* 'TRAK' */
#define ProgressCallback(file,curVal,endVal) \
                       (*file->progressProc)(file,curVal,endVal)
#define streq(a,b) (strncmp(a, b, (size_t)4) == 0)

#define PROP_HDR_BENTO	"OMFI:"
#define TYPE_HDR_BENTO	"omfi:"
#define MAX_TYPE_VARIENTS	2

/************************************************************
 *
 * Structures kept in defTables
 *
 *************************************************************/
typedef struct
{
	char           			propName[64];
	omfClassID_t  			classTag;
	omfProperty_t			propID;
	omfType_t       		type[MAX_TYPE_VARIENTS];
	omfInt16			numTypes;
	omfDataValidityTest_t	testProc;
	omfClassIDPtr_t			dataClass;
	omfValidRev_t   		revs;
	omfBool					isOptional;
	char           			*testName; 
}               OMPropDef;

typedef struct
{
	omfValidRev_t  		revs;
	char            			typeName[32];
	omfSwabCheck_t		swabNeeded;
}               OMTypeDef;

typedef struct
{
	omfClassType_t  type;
	omfClassID_t  	aClass;
	omfValidRev_t   revs;
	omfClassID_t  	itsSuperClass;
	omfObject_t		clsd;		/* During close, contains the clsd object */
}               OMClassDef;

typedef struct
{
	CMType  		bentoID;
	omfSwabCheck_t	swab;
}               OMTypeCache;

/************************************************************
 *
 * The session opaque handle
 *
 * This is a growable, sparse array construct used in the toolkit.
 *
 *************************************************************/

typedef         omfErr_t(*omfOpenFileCB) (omfHdl_t file);
typedef         omfErr_t(*omfCloseSessCB) (omfSessionHdl_t sess);

struct omfiSession
{
	omfInt32           cookie;
	omfHdl_t        topFile;
	CMSession       BentoSession;

	/* */
	omTable_t    	*types1X;
	omTable_t    	*properties1X;
	omTable_t    	*types2X;
	omTable_t    	*properties2X;
	omTable_t	*typeNames;
	omTable_t	*propertyNames;
	
	/* */
	omTable_t   	*classDefs1X;
	omTable_t   	*classDefs2X;
	omfInt32           prefix;
		
	/* */
	omTable_t    	*codecMDES;
	omTable_t    	*codecID;
		
	/* */
	omfInt32		BentoErrorNumber;
	omfBool			BentoErrorRaised;
	char				BentoErrorString[256];

	/* */
	omfOpenFileCB	openCB;	/* Used by the media layer to do per-file open */
	omfCloseSessCB	closeSessCB; /* Used by media layer to dispose codec data*/
		
	omfUInt32		nextDynamicProp;
	omfUInt32		nextDynamicType;
	omfBool			mediaLayerInitComplete;
	omfProductIdentification_t	*ident;

	struct omfiBentoIOFuncs ioFuncs;

};

/************************************************************
 *
 * The iterator function opaque handle
 *
 *************************************************************/
   typedef enum
  {
     kIterNull, kIterObj, kIterProp, kIterMobIndex, 
	 kIterMob, kIterEffect, kIterSelector, kIterScope, 
	 kIterSequence, kIterSequTran, kIterSequEffect, 
	 kIterMobSource, kIterEffectRender, 
	 kIterVaryValue, kIterLocator, kIterDupMob,
	 kIterMobComment,
	 kIterDone
} omfIterType_t;

struct omfiIterate
{
		omfInt32           cookie;
		omfIterType_t      iterType;
		omfInt32           currentIndex;
		struct omfiFile *file;
		omfInt32           maxIter;		
		omfInt32           maxMobs;    
		omfObject_t     head;
		omfSearchCrit_t searchCrit;
		omfMediaCriteria_t searchMediaCrit; /* if search Tag == kMediaCrit;
											  only works for renderings */
        omfObject_t   mdes;        /* Used by omfmMobGetNextLocator() */
        CMObject      currentObj;  /* Used by GetNextObject(), uses Bento */
		CMProperty    currentProp; /* Used by GetNextProp(), uses Bento */
		CMValue		currentValue;
		omfMobObj_t   masterMob;   /* MasterMob for Media Group; used
									  while iterating over renderings */
		omfTrackID_t  masterTrack; /* Track in MasterMob for Media Group;
									  used while iterating over renderings */
		omfPosition_t position;    /* Used to hold position while iterating
									  through a sequence */
		omfBool       prevTransition; /* If the previous cpnt was a TRAN */
		omfLength_t   prevLength;     /* Used by sequence to remember len 
									   * of previous component
									   */
		omfLength_t   prevOverlap;    /* Used by a sequence to remember the
                                       * overlap of the previous transition
                                       * into the current segment
									   */
		struct omfiIterate *nextIterator; /* In case an iterator needs
											 to use an iterator; dispose
											 will free everything on this
											 chain */
		omTableIterate_t *tableIter; /* Used by duplication MobID iterator */
};

/************************************************************
 *
 * The simple track opaque handle
 *
 *************************************************************/
struct omfiSimpleTrack
  {
		omfInt32            cookie;
		struct omfiFile  *file;
		omfMobObj_t      mob;
        omfTrackID_t     trackID;
        omfMSlotObj_t    track;
		omfSegObj_t      sequence;
		omfDDefObj_t     datakind;
		omfPosition_t    currentPos;
};

/************************************************************
 *
 * The raw stream data.
 *
 *************************************************************/

typedef struct omfRawStream omfRawStream_t;
	
/************************************************************
 *
 * The per-file opaque handle (omdHdl_t) and related definitions.
 *
 *************************************************************/

typedef         omfErr_t(*omfCloseMediaPtr) (omfMediaHdl_t media);



struct omfiFile
{
		omfInt32       	cookie;
		omfFileFormat_t fmt;
		omfRawStream_t	*rawFile;		/* If non-omfi file */
		void           	*rawFileDesc;	/* If non-omfi file */
		omfCodecID_t	rawCodecID;
		struct omfiSession *session;
		omfHdl_t        prevFile;
		omfFileRev_t     setrev;
		omfMediaHdl_t   topMedia;
		omfBool         semanticCheckEnable;	/* Used to stop
							 * recursion in checks */
		CMContainer     container;
		omfObject_t     head;
		omfInt16           byteOrder;
		omfCloseMediaPtr closeMediaProc;
		openType_t		openType;
		
		/* */
		omfInt32			BentoErrorNumber;
		omfBool			BentoErrorRaised;
		char			BentoErrorString[256];

		/* */
		omTable_t	    *types;	     /* Either 1.x OR 2.x, but not both */
		omTable_t	    *properties; /* Either 1.x OR 2.x, but not both */
		/* Reverse lookup tables to get from Bento -> Avid numbers */
		omTable_t	    *typeReverse;	     /* Either 1.x OR 2.x, but not both */
		omTable_t	    *propReverse; /* Either 1.x OR 2.x, but not both */
		omTable_t       *datakinds;
		omTable_t       *effectDefs;

		/* */
		CMProperty	byteOrderProp;
		CMType		byteOrderType;
		CMType		objTagProp1x;
		CMType		objTagType1x;
		CMType		objClassProp2x;
		CMType		objClassType2x;

		/* */
		omfDDefObj_t	nilKind;
		omfDDefObj_t	pictureKind;
		omfDDefObj_t	soundKind;

		/* */
		omTable_t       *mobs;
		omTable_t       *dataObjs;

		omfLocatorFailureCB locatorFailureCallback;
		omfBool			customStreamFuncsExist;
		struct omfCodecStreamFuncs streamFuncs;
		omfProgressProc_t	progressProc;
		omfCompressEnable_t	compatSoftCodec;

#ifdef OMFI_ERROR_TRACE
		char			*stackTrace;
		omfInt32			stackTraceSize;
#endif
};

omfErr_t clearBentoErrors(omfHdl_t file);	/* IN -- For this omf file */
omfErr_t omfsInit(void);

/************************************************************
 *
 * The search handle used by the iterative findSource type functions.
 * AND a few supporting typedefs
 *
 *************************************************************/

typedef struct omfiScopeStack *omfScopeStack_t;

struct omfiMobFind
{
   omfInt32      cookie;	     /* kIterMobFind */
   struct omfiFile *file;		
   omfObject_t   head;
   omfObject_t   mob;
   omfObject_t   track;
   omfInt32      numSegs;
   omfBool       isSequence;
   omfObject_t   segment;
   omfObject_t   sequence;
   omfIterHdl_t  sequIter;

   omfInt32      currentIndex;
   omfObject_t   currentObj;
   omfPosition_t currentObjPos;
   omfLength_t   currentObjLen;
   omfPosition_t currentPos;

   omfSearchCrit_t *searchCrit;
   omfMediaCriteria_t *mediaCrit;

   omfObject_t   prevObject;
   omfBool       prevTransition; /* If the previous cpnt was a TRAN */
   omfLength_t   prevOverlap;    /* Used by a sequence to remember the
                                       * overlap of the previous transition
                                       * into the current segment */

   omfObject_t   nextObject;
   omfLength_t   nextLength;
   omfPosition_t nextPosition;

   omfScopeStack_t scopeStack;
  omfPosition_t	effeNextPos;
  omfInt32		prevNestDepth;
};

typedef struct
{
  omfObject_t scope;
  omfInt32 numSlots;
  omfPosition_t scopeOffset;
  omfInt32 currSlotIndex;
} omfScopeInfo_t;

struct omfiScopeStack
{
  omfInt32 stackSize;
  omfInt32 maxSize;
  omfScopeInfo_t **scopeList;
};

/************************************************************
 *
 * Private data on the media handle (don't expose to the codecs)
 *
 *************************************************************/

typedef struct codecTable
{
	omfCodecDispPtr_t		initRtn;
	omfCodecOptDispPtr_t	dispTable[NUM_FUNCTION_CODES];
	void					*persist;
	omfCodecRev_t		rev;
	omfCodecType_t		type;
	omfString			dataKindNameList;	/* Comma-delimited list of kind names */
	omfFileRev_t			minFileRev;		/* revs lower than this will be private */
	omfFileRev_t			maxFileRev;		/* (revs higher than this will be private */
	/* Put the smaller quantities (bool) at the end of the struct */
	omfBool				maxFileRevIsValid;	 /* Is there a maximum rev? */
} codecTable_t;

struct omfiMediaPrivate
{
	codecTable_t   	codecInfo;
	omfPixelFormat_t	pixType;
	struct omfiMedia 	*nextMedia;
	omfLength_t        	repeatCount;
};

OMF_EXPORT omfErr_t omfmFindCodecForMedia(
             			omfHdl_t		file,
			            omfObject_t		mdes,
			            codecTable_t	*result);

/************************************************************
 *
 * CODEC API Functions
 *
 *************************************************************/

OMF_EXPORT omfErr_t codecInit(
			omfSessionHdl_t		session,
			codecTable_t		*codecPtr,
			void				**persistentData);
OMF_EXPORT void *codecGetPersistentData(omfSessionHdl_t sess, 
							 omfCodecID_t id, 
							 omfErr_t *errCode);
OMF_EXPORT omfErr_t codecGetMetaInfo(omfSessionHdl_t session, 
						  codecTable_t *codecPtr, 
							char				*variant,
						  char *nameBuf, 
						  omfInt32 nameBufLen, 
						  omfCodecMetaInfo_t *info);
OMF_EXPORT omfErr_t codecGetSelectInfo(omfHdl_t file, 
							codecTable_t *codecPtr, 
							omfObject_t mdes, 
							omfCodecSelectInfo_t *info);

OMF_EXPORT omfErr_t        codecGetNumChannels(omfHdl_t file, 
									omfObject_t fileMob, 
									omfDDefObj_t mediaKind, 
									omfInt16 * numCh);
OMF_EXPORT omfErr_t		codecInitMDESProps(omfHdl_t main, 
								   omfObject_t mdes, 
								   omfCodecID_t codecID,
								   char *variant);
OMF_EXPORT omfErr_t        codecCreate(omfMediaHdl_t media);
OMF_EXPORT omfErr_t        codecOpen(omfMediaHdl_t media);
OMF_EXPORT omfErr_t        codecGetAudioInfo(omfMediaHdl_t media, omfAudioMemOp_t *ops);
OMF_EXPORT omfErr_t        codecPutAudioInfo(omfMediaHdl_t media, omfAudioMemOp_t *ops);
OMF_EXPORT omfErr_t        codecGetVideoInfo(omfMediaHdl_t media, omfVideoMemOp_t *ops);
OMF_EXPORT omfErr_t        codecPutVideoInfo(omfMediaHdl_t media, omfVideoMemOp_t *ops);
OMF_EXPORT omfErr_t        codecWriteHeader(omfMediaHdl_t fmt);
OMF_EXPORT omfErr_t        codecReadHeader(omfMediaHdl_t fmt);
OMF_EXPORT omfErr_t        codecSeek(omfMediaHdl_t fmt, omfPosition_t sampleFrame);
OMF_EXPORT omfErr_t        codecWriteBlocks(omfMediaHdl_t media, 
								 omfDeinterleave_t inter,
								 omfInt16 xferBlockCount, 
								 omfmMultiXfer_t * xferBlock);
OMF_EXPORT omfErr_t        codecReadBlocks(omfMediaHdl_t media, 
								omfDeinterleave_t inter,
								omfInt16 xferBlockCount, 
								omfmMultiXfer_t * xferBlock);
OMF_EXPORT omfErr_t        codecWriteLines(omfMediaHdl_t media, 
								omfInt32 nLines, 
								void *buffer, 
								omfInt32 * bytesWritten);
OMF_EXPORT omfErr_t        codecReadLines(omfMediaHdl_t media, 
							   omfInt32 nLines, 
							   omfInt32 bufLen, 
							   void *buffer, 
							   omfInt32 * bytesRead);
OMF_EXPORT omfErr_t        codecSetFrameNum(omfMediaHdl_t media, omfPosition_t frameNum);
OMF_EXPORT omfErr_t        codecGetFrameOffset(omfMediaHdl_t media, 
									omfInt64 frameNum,
									omfInt64 *frameOffset);
OMF_EXPORT omfErr_t        codecClose(omfMediaHdl_t media);
OMF_EXPORT omfErr_t        codecGetLastError(omfMediaHdl_t Handle);
OMF_EXPORT omfErr_t        codecGetInfo(omfMediaHdl_t media, 
							 omfInfoType_t infoType, 
							 omfDDefObj_t mediaKind, 
							 omfInt32 infoBufSize, 
							 void *info);
OMF_EXPORT omfErr_t        codecPutInfo(omfMediaHdl_t media, 
							 omfInfoType_t infoType, 
							 omfDDefObj_t mediaKind, 
							 omfInt32 infoBufSize, 
							 void *info);
OMF_EXPORT omfInt32        codecGetLastSampleCount(omfMediaHdl_t Handle);
OMF_EXPORT omfErr_t        codecImportRaw(omfHdl_t mainFile, 
							   omfObject_t fileMob, 
							   omfHdl_t rawFile);
OMF_EXPORT omfErr_t codecGetNumCodecs(
			omfSessionHdl_t 	sess,
			omfString		dataKindString,
			omfFileRev_t		rev,
			omfInt32		*numCodecPtr);
OMF_EXPORT omfErr_t codecGetIndexedCodecID(
			omfSessionHdl_t 	sess,
			omfString		dataKindString,
			omfFileRev_t		rev,
			omfInt32		index,
			char			**codecIDPtr);

OMF_EXPORT omfErr_t codecGetNumCodecVarieties(
			omfSessionHdl_t 	sess,
			omfCodecID_t		codecID,
			omfFileRev_t    	setrev,
			char				*mediaKindString,
			omfInt32		*varietyCountPtr);
			
OMF_EXPORT omfErr_t codecGetIndexedCodecVariety(
			omfSessionHdl_t	sess,
			omfCodecID_t		codecID,
			omfFileRev_t    	setrev,
			char				*mediaKindString,
			omfInt32		index,
			char				**varietyPtr);
OMF_EXPORT omfErr_t codecSemanticCheck(
			omfHdl_t			main,
			omfObject_t		fileMob,
			omfCheckVerbose_t verbose,
			omfCheckWarnings_t warn, 
			omfInt32		msgBufSize,
			char				*msgBuf);	/* OUT - Which property (if any) failed */
OMF_EXPORT omfErr_t codecAddFrameIndexEntry(
			omfMediaHdl_t	media,			/* IN -- */
			omfInt64		frameOffset);	/* IN -- */
OMF_EXPORT omfErr_t codecPassThrough(
			codecTable_t		*subCodec,	/* IN -- */
			omfCodecFunctions_t	func,
			omfCodecParms_t 	*parmblk,
			omfMediaHdl_t 		media,
			omfHdl_t			main,
			void				*clientPData);

/************************************************************
 *
 * Mob Table Management Functions
 *
 *************************************************************/

typedef struct
{
	omfObject_t		mob;
	omTable_t		*map1x;
	omfRational_t	createEditRate;
} mobTableEntry_t;

OMF_EXPORT omfErr_t AddMobTableEntry(omfHdl_t file, 
						  omfObject_t mob, 
						  omfUID_t mobID, 
						  omTableDuplicate_t dup);

/************************************************************
 *
 * Functions for handling UIDs.
 *
 *************************************************************/
#define NULLMobID(mob) mob.prefix = 0, mob.major = 0, mob.minor = 0;

/* two UIDs are equal if all of the fields are equal */
#define equalUIDs(a, b) \
    ((a.prefix == b.prefix) && (a.major == b.major) && (a.minor == b.minor))

/* initUID stores values into a Unique ID structure */
#define initUID(id, pref, maj, min) \
          { (id).prefix = pref; (id).major = maj; (id).minor = min; }

#define copyUIDs(a, b) \
       {a.prefix = b.prefix; a.major = b.major; a.minor = b.minor;}

/* Definitions for Timecode calculations */
#define FPSEC		((omfInt32)(30))
#define FPMIN		((omfInt32)(60*FPSEC))
#define FPHR		((omfInt32)(60*FPMIN))
#define PALFPSEC	((omfInt32)(25))
#define PALFPMIN	((omfInt32)(60*PALFPSEC))
#define PALFPHR		((omfInt32)(60*PALFPMIN))

#define DFPMIN	((omfInt32)(FPMIN-2))				/* minutes */
#define DFP10M	((omfInt32)(10*(FPMIN-2)+2))		/* df 10 minutes */
#define DFPHR	((omfInt32)(6*(10*(FPMIN-2)+2)))	/* hours */

/************************************************************
 *
 * Initialization and utility functions.
 *
 *************************************************************/
OMF_EXPORT omfErr_t        initSemanticChecks(omfSessionHdl_t session);
OMF_EXPORT omfBool objRefIndexInBounds(omfHdl_t file, 
							omfObject_t obj, 
							omfProperty_t prop, 
							omfInt32 i);
OMF_EXPORT omfBool mobIndexInBounds(omfHdl_t file, 
						 omfObject_t obj, 
						 omfProperty_t prop, 
						 omfInt32 i);
OMF_EXPORT omfBool isVirtualObj(omfClassIDPtr_t objID);


OMF_EXPORT CMProperty      CvtPropertyToBento(omfHdl_t file,	/* IN -- For this file */
								   omfProperty_t prop);	/* IN -- look up this
														 * property. */
OMF_EXPORT CMType          CvtTypeToBento(omfHdl_t file,	/* IN -- For this omf file */
							   omfType_t type,	/* IN -- look up this
												 * omfType_t code. */
						omfSwabCheck_t *swab);	/* OUT -- Tell if this needs to be swabbed */

OMF_EXPORT omfProperty_t	CvtPropertyFromBento(omfHdl_t file, 
									 CMProperty srchProp, 
									 omfBool *found);
OMF_EXPORT omfType_t		CvtTypeFromBento(omfHdl_t file, 
								 CMType srchProp, 
								 omfBool *found);

OMF_EXPORT omfBool ompvtIsForeignByteOrder(
			omfHdl_t 	file,		/* IN -- For this omf file */
			omfObject_t obj);		/* IN -- is this object foreign byte order? */

OMF_EXPORT void			ConvertTimeToCanonical(omfUInt32 *time);
OMF_EXPORT void			ConvertTimeFromCanonical(omfUInt32 *time);

OMF_EXPORT omfErr_t GetReferencedObject(
			omfHdl_t			file,		/* IN - For this omf file */
			omfObject_t		obj,		/* IN - and this containing object */
			omfProperty_t	prop,		/* IN - in this property */
			omfType_t		dataType,/* IN - of this type */
			CMReference		refData,	/* IN - Given this object reference */
			omfObject_t 	*result);/* OUT - return the object pointer */

OMF_EXPORT omfErr_t GetReferenceData(
			omfHdl_t			file,		/* IN - For this omf file */
			omfObject_t		obj,		/* IN - and this containing object */
			omfProperty_t	prop,		/* IN - in this property */
			omfType_t		dataType,/* IN - of this type */
			omfObject_t		ref,		/* IN - Given this object */
			CMReference		*result);/* IN - Return a persistant reference */

OMF_EXPORT omfErr_t FindMobInIndex(
	omfHdl_t file,
    omfUID_t mobID,
    omfProperty_t indexProp,
	omfInt32 *foundIndex,
    omfObject_t *mob);

OMF_EXPORT omfErr_t SourceClipGetRef(
    omfHdl_t file,
	omfSegObj_t sourceClip,
	omfSourceRef_t *sourceRef);

OMF_EXPORT omfErr_t MobSlotGetTrackDesc(
    omfHdl_t file,
	omfMobObj_t mob, /* Need for 1.x trackID compatibility */
	omfMSlotObj_t slot,
   	omfInt32 nameSize,
	omfString name,
	omfPosition_t *origin,
	omfTrackID_t *trackID);

OMF_EXPORT omfErr_t MobSetNewProps(
	omfHdl_t file,
	omfMobObj_t mob,
	omfBool isMasterMob,
	omfString name,
	omfBool isPrimary);

OMF_EXPORT omfErr_t ComponentSetNewProps(
        omfHdl_t file,
        omfCpntObj_t component,
        omfLength_t length,
        omfDDefObj_t datakind);

OMF_EXPORT omfErr_t MobFindCpntByPosition(
    omfHdl_t    file,
	omfObject_t mob,
	omfTrackID_t trackID,
    omfObject_t rootObj,
	omfPosition_t startPos,
	omfPosition_t position,
    omfMediaCriteria_t *mediaCrit,
	omfPosition_t     *diffPos,
    omfObject_t *foundObj,
	omfLength_t *foundLen);

OMF_EXPORT omfErr_t SequencePeekNextCpnt(omfIterHdl_t sequIter,
						  omfObject_t sequence,
						  omfObject_t *component,
						  omfPosition_t *offset);


OMF_EXPORT omfErr_t FindTrackByTrackID(omfHdl_t file, 
							omfMobObj_t mob, 
							omfTrackID_t trackID, 
							omfObject_t *destTrack);

OMF_EXPORT omfErr_t omfiComponentGetLength(
	omfHdl_t file,          /* IN - File Handle */
	omfCpntObj_t component, /* IN - Component Object */
	omfLength_t *length);    /* OUT - Component Length */

/************************************************************
 *
 * 1.x <=> 2.x track mapping functions
 *
 *************************************************************/

OMF_EXPORT omfErr_t AddTrackMapping(omfHdl_t file, 
						 omfUID_t mobID, 
						 omfTrackID_t trackID, 
						 omfDDefObj_t mediaKind);

OMF_EXPORT omfErr_t AddTrackMappingExact(omfHdl_t file, 
							  omfUID_t mobID, 
							  omfTrackID_t trackID, 
							  omfDDefObj_t mediaKind,
							  omfInt16	trackNum1X);

OMF_EXPORT omfErr_t CvtTrackIDtoNum(omfHdl_t file, 
						 omfUID_t mobID, 
						 omfTrackID_t trackID, 
						 omfInt16 *trackNum1X);

OMF_EXPORT omfErr_t CvtTrackIDtoTrackType1X(omfHdl_t file, 
								 omfUID_t mobID, 
								 omfTrackID_t trackID, 
								 omfTrackType_t *trackType1X);

OMF_EXPORT omfErr_t CvtTrackNumtoID(omfHdl_t file, 
						 omfUID_t mobID, 
						 omfInt16 trackNum1X, 
						 omfInt16 trackType1X,
						 omfTrackID_t *trackID);

OMF_EXPORT omfInt32 MediaKindToTrackType(omfHdl_t file, 
						   omfDDefObj_t mediaKind);

OMF_EXPORT omfDDefObj_t TrackTypeToMediaKind(omfHdl_t file, 
								  omfInt32 trackType);

OMF_EXPORT omfErr_t TimecodeToOffset(
	omfInt16 frameRate,
	omfInt16 hours,
	omfInt16 minutes,
	omfInt16 seconds,
	omfInt16 frames,
	omfDropType_t drop,
	omfFrameOffset_t	*result);

OMF_EXPORT omfErr_t OffsetToTimecode(
	omfFrameOffset_t offset,
	omfInt16 frameRate,
	omfDropType_t drop,
	omfInt16 *hours,
	omfInt16 *minutes,
	omfInt16 *seconds,
	omfInt16 *frames);

OMF_EXPORT omfErr_t LocateMediaFile(
			omfHdl_t			file,		/* IN */
			omfObject_t		fileMob,	/* IN */
			omfHdl_t			*dataFile,
			omfBool			*isOMFI);

OMF_EXPORT omfErr_t InitFileHandle(
			omfSessionHdl_t	session, 
			omfFileFormat_t 	fmt, 
			omfHdl_t				*result);

OMF_EXPORT omfBool IsValidTranEffect(
    omfHdl_t    file, 
    omfEffObj_t effect, 
    omfErr_t    *omfError);

OMF_EXPORT omfErr_t FindSlotByArgID(
								const omfHdl_t 		 file,
								const omfSegObj_t 	 effect, 
								const omfArgIDType_t argID,
								omfESlotObj_t  *effectSlot,
								omfSegObj_t 	 *slotValue);

OMF_EXPORT omfBool IsDatakindOf(
    omfHdl_t file,
    omfDDefObj_t dataDef,
	omfUniqueNamePtr_t datakindName,
	omfErr_t *omfError);

OMF_EXPORT omfBool DoesDatakindConvertTo(
    omfHdl_t file,
    omfDDefObj_t dataDef,
	omfUniqueNamePtr_t datakindName,
	omfErr_t *omfError);

OMF_EXPORT omfBool DoesDatakindConvertFrom(
    omfHdl_t file,                   /* IN - File Handle */
    omfDDefObj_t dataDef,            /* IN - Datakind definition object */
	omfUniqueNamePtr_t datakindName, /* IN - DatakindName to compare against */
	omfErr_t *omfError);             /* OUT - Error code */

OMF_EXPORT omfBool IsDatakindMediaStream(
    omfHdl_t file,        /* IN - File Handle */
	omfDDefObj_t dataDef, /* IN - Datakind object */
	omfErr_t *omfError);   /* OUT - Error Code */


/************************************************************
 *
 * Rational Conversion Functions
 *
 *************************************************************/

OMF_EXPORT omfRational_t RationalFromFloat(
			double	f);	/* IN - Convert this number into a rational */

OMF_EXPORT double FloatFromRational(
			omfRational_t	e);		/* IN - Convert this into a double */

#define MakeRational(num, den, resPtr) \
        {(resPtr)->numerator = num; (resPtr)->denominator = den;}

/* Private functions for the compatability layer 
 */
OMF_EXPORT void UpdateBentoErrGlobals(omfHdl_t file);

OMF_EXPORT omfErr_t CopyProperty(
							omfHdl_t file,
							omfObject_t obj, 
							CMProperty cprop, 
							CMValue cpropValue, 
							CMType cvalueType);

OMF_EXPORT omfBool isObjFunc(omfHdl_t file,
                         omfObject_t obj,
                         void *data);
OMF_EXPORT omfErr_t set1xEditrate(omfHdl_t file,
							omfObject_t obj,
							omfInt32 level,
                            void *data);


OMF_EXPORT omfErr_t ompvtMediaOpenFromFileMob(
			omfHdl_t					file,		/* IN -- For this file */
			omfObject_t				fileMob,		/* IN -- In this master mob */
			omfTrackID_t				trackID,
			omfMediaOpenMode_t	openMode,	/* IN -- ReadOnly or Append */
			omfDDefObj_t			mediaKind,	/* IN -- and this media type */
			omfInt16					subTrackNum,	/* IN -- open media at this track */
			omfCompressEnable_t	compEnable,	/* IN -- optionally decompressing */
			omfMediaHdl_t			*mediaPtr);	/* OUT -- and return a meida handle */

OMF_EXPORT omfErr_t ompvtMediaMultiOpenFromFileMob(
			omfHdl_t					file,			/* IN -- For this file */
			omfObject_t				fileMob,	/* IN -- In this master mob */
			omfTrackID_t				trackID,
			omfMediaOpenMode_t	openMode,	/* IN -- ReadOnly or Append */
			omfCompressEnable_t	compEnable,	/* IN -- optionally decompressing */
			omfMediaHdl_t			*mediaPtr);	/* OUT -- and return the media handle */
			
OMF_EXPORT omfErr_t ompvtGetNumChannelsFromFileMob(
			omfHdl_t			file,			/* IN -- For this file */
			omfObject_t		fileMob,	/* IN -- In this master mob */
			omfDDefObj_t  	mediaKind,	/* IN -- for this media type */
			omfInt16			*numCh);		/* OUT -- How many channels? */
			
OMF_EXPORT omfErr_t omfPvtGetPulldownMask(omfHdl_t file,
			   omfPulldownKind_t	pulldown,
			   omfUInt32 			*outMask,
			   omfInt32				*maskLen,
			   omfBool	 			*isOneToOne);

OMF_EXPORT void *omOptMalloc(
			omfHdl_t	file,			/* IN - For this file */
			size_t	size);			/* Allocate this many bytes */

OMF_EXPORT void omOptFree(
			omfHdl_t	file,		/* IN - For this file */
			void 		*ptr);		/* Free up this buffer */

#if PORT_LANG_CPLUSPLUS
}
#endif
#endif				/* _OMF_PRIVATE_FUNCTIONS_ */

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
