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

typedef enum
  {
  Motorola,
  Intel
  } ByteOrder_t;

/* Define Enumerated type for all chunks */
typedef enum
  {
    CHUNK_NULL,
    NONOMFIOBJECT,
    CHUNK_DELETED,
    HEAD,
    MACFILELOCATOR,
    UNIXFILELOCATOR,
    DOSFILELOCATOR,
    COMPONENT,
    TRANSITION,
    CLIP,
    TRACKGROUP,
    SELECTOR,
    SEQUENCE,
    SOURCECLIP,
    TIMECODECLIP,
    EDGECODECLIP,
    TRACKREF,
    MEDIAOBJECT,
    MEDIADESC,
    FILEMEDIADESC,
    ATTRLIST,
    ATTRIBUTE,
    ATTRCLIP,
    AIFCDATA,
    AIFCMEDIADESC,
    WAVEDATA,
    WAVEMEDIADESC,
    TIFFDATA,
    TIFFMEDIADESC,
    FILLCLIP,
    REPEATCLIP,
    MOTIONCONTROL,
    CAPTUREMASK,
    TIMEWARP,
    TRACKOBJECT,
    CLASSDICT,
    HEADOBJECT,
    LRCMDATA,
    LRCDMEDIADESC,
    OMFI_CHUNK_UNKNOWN
} OMFI_chunks;


/* Define Table of chunks */
#define CHUNK_TABLE_DEFINING

typedef struct
  {
  char *code;
  OMFI_chunks number;
  } OMFIChunkNameStruct_t;

OMFIChunkNameStruct_t ChunkNameTable[] =
  {
    {"\?\?\?\?", CHUNK_NULL},
    {"    ", NONOMFIOBJECT},
    {"    ", CHUNK_DELETED},
    {"HEAD", HEAD},
    {"MACL", MACFILELOCATOR},
    {"UNXL", UNIXFILELOCATOR},
    {"DOSL", DOSFILELOCATOR},
    {"CPNT", COMPONENT},
    {"TRAN", TRANSITION},
    {"CLIP", CLIP},
    {"TRKG", TRACKGROUP},
    {"SLCT", SELECTOR},
    {"SEQU", SEQUENCE},
    {"SCLP", SOURCECLIP},
    {"TCCP", TIMECODECLIP},
    {"ECCP", EDGECODECLIP},
    {"TRKR", TRACKREF},
    {"MOBJ", MEDIAOBJECT},
    {"MDES", MEDIADESC},
    {"MDFL", FILEMEDIADESC},
    {"ATTR", ATTRLIST},
    {"ATTB", ATTRIBUTE},
    {"ATCP", ATTRCLIP},
    {"AIFC", AIFCDATA},
    {"AIFD", AIFCMEDIADESC},
    {"WAVE", WAVEDATA},
    {"WAVD", WAVEMEDIADESC},
    {"TIFF", TIFFDATA},
    {"TIFD", TIFFMEDIADESC},
    {"FILL", FILLCLIP},
    {"REPT", REPEATCLIP},
    {"SPED", MOTIONCONTROL},
    {"MASK", CAPTUREMASK},
    {"WARP", TIMEWARP},
    {"TRAK", TRACKOBJECT},
    {"CLSD", CLASSDICT},
    {"LRCM", LRCMDATA},
    {"LRCD", LRCDMEDIADESC},
    {"HEAD", HEADOBJECT},
    {"Unknown chunk", OMFI_CHUNK_UNKNOWN}
  };

/* Macros to manipulate property bit masks */
#define REQ (unsigned int )1
#define OPT (unsigned int )0

/* makePropTag: c=required|optional; b=property; a=class */
#define makePropTag(a,b,c) ((c<<16) + (a<<8) + b)

#define isRequired(a) (a>>16)
#define getClassTag(a) ((a>>8) & 0x000000FF)
#define getPropTag(a) (a & 0x000000FF)

/* PROPERTY MASKS */
/* Each of these masks defines the set of properties (either required or
   optional, depending on the mask) defined for a particular object class */
#define reqALLmask ((REQ<<16) | (ALLClass <<8) | OMFIObjID)
#define reqCPNTmask ((REQ<<16) | (CPNTClass <<8) | OMFICPNTTrackKind | OMFICPNTEditRate)
#define reqHEADmask ((REQ<<16) | (HEADClass <<8) | OMFIByteOrder | OMFILastModified | OMFISourceMobs | OMFICompositionMobs | OMFIMediaData)
#define reqCLSDmask ((REQ<<16) | (CLSDClass <<8) | OMFICLSDClassID | OMFICLSDParentClass)
#define reqATTRmask ((REQ<<16) | (ATTRClass <<8) | OMFIATTRAttrRefs)
#define reqATTBmask ((REQ<<16) | (ATTBClass <<8) | OMFIATTBKind | OMFIATTBName)
#define reqTRKGmask ((REQ<<16) | (TRKGClass <<8) | OMFITRKGGroupLength | OMFITRKGTracks)
#define reqMOBJmask ((REQ<<16) | (MOBJClass <<8) | OMFIMOBJMobID | OMFIMOBJLastModified | OMFIMOBJStartPosition | OMFIMOBJPhysicalMedia)
#define reqTRAKmask ((REQ<<16) | (TRAKClass <<8))
#define reqCLIPmask ((REQ<<16) | (CLIPClass <<8) | OMFICLIPLength)
/* SCLP is tricky since other SourceTrack and SourcePosition must exist if
 * Source ID does */
#define reqSCLPmask ((REQ<<16) | (SCLPClass <<8))
#define reqTRKRmask ((REQ<<16) | (TRKRClass <<8) | OMFITRKRRelativeScope | OMFITRKRRelativeTrack)
#define reqFILLmask ((REQ<<16) | (FILLClass <<8))
#define reqATCPmask ((REQ<<16) | (ATCPClass <<8) | OMFIATCPAttributes)
#define reqTCCPmask ((REQ<<16) | (TCCPClass <<8) | OMFITCCPFlags | OMFITCCPFPS | OMFITCCPStartTC)
#define reqECCPmask ((REQ<<16) | (ECCPClass <<8) | OMFIECCPFilmKind | OMFIECCPCodeFormat | OMFIECCPStartEC)
#define reqSEQUmask ((REQ<<16) | (SEQUClass <<8) | OMFISEQUSequence)
#define reqSLCTmask ((REQ<<16) | (SLCTClass <<8) | OMFISLCTSelectedTrack)
#define reqTRANmask ((REQ<<16) | (TRANClass <<8) | OMFITRANCutPoint)
#define reqMASKmask ((REQ<<16) | (MASKClass <<8) | OMFIMASKMaskBits  | OMFIMASKIsDouble)
#define reqSPEDmask ((REQ<<16) | (SPEDClass <<8) | OMFISPEDNumerator | OMFISPEDDenominator)
#define reqREPTmask ((REQ<<16) | (REPTClass <<8))
#define reqMDESmask ((REQ<<16) | (MDESClass <<8) | OMFIMDESMobKind)
#define reqTXTLmask ((REQ<<16) | (TXTLClass <<8))
#define reqDOSLmask ((REQ<<16) | (DOSLClass <<8))
#define reqMACLmask ((REQ<<16) | (MACLClass <<8))
#define reqUNXLmask ((REQ<<16) | (UNXLClass <<8))
#define reqMDFLmask ((REQ<<16) | (MDFLClass <<8) | OMFIMDFLIsOMFI | OMFIMDFLSampleRate | OMFIMDFLLength)
#define reqAIFDmask ((REQ<<16) | (AIFDClass <<8) | OMFIAIFDSummary)
#define reqWAVDmask ((REQ<<16) | (WAVDClass <<8) | OMFIWAVDSummary)
#define reqTIFDmask ((REQ<<16) | (TIFDClass <<8) | OMFITIFDIsUniform | OMFITIFDIsContiguous | OMFITIFDSummary)
#define reqLRCDmask ((REQ<<16) | (LRCDClass <<8) | OMFILRCDMobID | OMFILRCDFrameLength | OMFILRCDWordLength | OMFILRCDCompressedLength | OMFILRCDIndexPresent)
#define reqTIFFmask ((REQ<<16) | (TIFFClass <<8) | OMFITIFFMobID | OMFITIFFData)
#define reqLRCMmask ((REQ<<16) | (LRCMClass <<8) | OMFILRCMMobID | OMFILRCMFrameLength | OMLRCMWordLength | OMLRCMSampleRate | OMLRCMLength | OMLRCMCompressedLength | OMLRCMDataByteOrder |OMFITIFFData)
#define reqAIFCmask ((REQ<<16) | (AIFCClass <<8) | OMFIAIFCMobID | OMFIAIFCData)
#define reqWAVEmask ((REQ<<16) | (WAVEClass <<8) | OMFIWAVEMobID | OMFIWAVEData)
#define reqWARPmask ((REQ<<16) | (WARPClass <<8) | OMFIWARPEditRate | OMFIWARPPhaseOffset)

#define optALLmask ((ALLClass <<8) | OMFIWAVEData | OMFICopyright | OMFIDate | OMFIByteOrder | OMFICharacterSet)
#define optHEADmask ((HEADClass <<8) | OMFIVersion | OMFIClassDictionary | OMFIExternalFiles | OMFIAttributes)
#define optCLSDmask ((CLSDClass <<8) | OMFICLSDVersion)
#define optCPNTmask ((CPNTClass <<8) | OMFICPNTVersion | OMFICPNTName | OMFICPNTEffectID | OMFICPNTPrecomputed | OMFICPNTAttributes)
#define optATTRmask ((ATTRClass <<8) | OMFIATTRVersion)
#define optATTBmask ((ATTBClass <<8) | OMFIATTBVersion | OMFIATTBIntAttribute | OMFIATTBStringAttribute | OMFIATTBObjAttribute)
#define optTRKGmask ((TRKGClass <<8) | OMFITRKGVersion)
#define optMOBJmask ((MOBJClass <<8) | OMFIMOBJVersion | OMFIMOBJUsageCode)
#define optTRAKmask ((TRAKClass <<8) | OMFITRAKVersion | OMFITRAKLabelNumber | OMFITRAKAttributes | OMFITRAKFillerProxy | OMFITRAKTrackComponent)
#define optCLIPmask ((CLIPClass <<8) | OMFICLIPVersion)
#define optSCLPmask ((SCLPClass <<8) | OMFISCLPVersion | OMFISCLPSourceID | OMFISCLPSourceTrack | OMFISCLPSourcePosition)
#define optTRKRmask ((TRKRClass <<8) | OMFITRKRVersion)
#define optFILLmask ((FILLClass <<8) | OMFIFILLVersion)
#define optATCPmask ((ATCPClass <<8) | OMFIATCPVersion)
#define optTCCPmask ((TCCPClass <<8) | OMFITCCPVersion)
#define optECCPmask ((ECCPClass <<8) | OMFIECCPVersion)
#define optSEQUmask ((SEQUClass <<8) | OMFISEQUVersion)
#define optSLCTmask ((SLCTClass <<8) | OMFISLCTVersion | OMFISLCTIsGanged)
#define optTRANmask ((TRANClass <<8) | OMFITRANVersion)
#define optMASKmask ((MASKClass <<8) | OMFIMASKVersion)
#define optSPEDmask ((SPEDClass <<8) | OMFISPEDVersion)
#define optREPTmask ((REPTClass <<8) | OMFIREPTVersion)
#define optMDESmask ((MDESClass <<8) | OMFIMDESVersion | OMFIMDESLocator)
#define optTXTLmask ((TXTLClass <<8) | OMFITXTLVersion | OMFITXTLName)
#define optDOSLmask ((DOSLClass <<8) | OMFIDOSLVersion | OMFIDOSLPathName)
#define optMACLmask ((MACLClass <<8) | OMFIMACLVersion | OMFIMACLVRef | OMFIMACLDirID | OMFIMACLFileName)
#define optUNXLmask ((UNXLClass <<8) | OMFIUNXLVersion | OMFIUNXLPathName)
#define optMDFLmask ((MDFLClass <<8) | OMFIMDFLVersion)
#define optAIFDmask ((AIFDClass <<8) | OMFIAIFDVersion)
#define optWAVDmask ((WAVDClass <<8) | OMFIWAVDVersion)
#define optTIFDmask ((TIFDClass <<8) | OMFITIFDVersion | OMFITIFDTrailingLines | OMFITIFDLeadingLines | OMFITIFDJPEGTableID)
#define optTIFFmask ((TIFFClass <<8) | OMFITIFFVersion)
#define optAIFCmask ((AIFCClass <<8) | OMFIAIFCVersion)
#define optWAVEmask ((WAVEClass <<8) | OMFIWAVEVersion)
#define optWARPmask ((WARPClass <<8) | OMFIWARPVersion)
#define optLRCDmask ((LRCDClass <<8) | OMFILRCDVersion)
#define optLRCMmask ((LRCMClass <<8) | OMFILRCMVersion | OMFILRCMIndex)


/* ************* */
/* PROPERTY BITS */
/* ************* */
/**** Object Types ****/
/* Superclasses */
#define CPNTClass (unsigned int )1
#define CLIPClass (unsigned int )2
#define TRKGClass (unsigned int )3
#define WARPClass (unsigned int )4
#define MDESClass (unsigned int )5
#define MDFLClass (unsigned int )6
#define ALLClass (unsigned int )7  /* Properties on all objects */

/* Leaf Classes */
#define ATCPClass (unsigned int )8
#define FILLClass (unsigned int )9
#define ECCPClass (unsigned int )10
#define TCCPClass (unsigned int )11
#define SCLPClass (unsigned int )12
#define TRKRClass (unsigned int )13
#define SEQUClass (unsigned int )14
#define MOBJClass (unsigned int )15
#define TRANClass (unsigned int )16
#define SLCTClass (unsigned int )17
#define SPEDClass (unsigned int )18
#define MASKClass (unsigned int )19
#define REPTClass (unsigned int )20
#define HEADClass (unsigned int )21
#define CLSDClass (unsigned int )22
#define TRAKClass (unsigned int )23
#define ATTRClass (unsigned int )24
#define ATTBClass (unsigned int )25
#define AIFDClass (unsigned int )26
#define WAVDClass (unsigned int )27
#define TIFDClass (unsigned int )28
#define DOSLClass (unsigned int )29
#define MACLClass (unsigned int )30
#define UNXLClass (unsigned int )31
#define TXTLClass (unsigned int )32
#define WAVEClass (unsigned int )33
#define AIFCClass (unsigned int )34
#define TIFFClass (unsigned int )35
#define LRCMClass (unsigned int )36
#define LRCDClass (unsigned int )37

/**** REQUIRED PROPERTIES ****/
#define OMFIObjID 1

/* HEAD Object */
#define OMFIHEADByteOrder (unsigned int )1
#define OMFILastModified (unsigned int )2
#define OMFIObjectSpine (unsigned int )4

/* CLSD Object */
#define OMFICLSDClassID (unsigned int )1
#define OMFICLSDParentClass (unsigned int )2

/* CPNT Object */
#define OMFICPNTTrackKind (unsigned int )1
#define OMFICPNTEditRate (unsigned int )2

/* ATTR Object */
#define OMFIATTRAttrRefs (unsigned int )1

/* ATTB Object */
#define OMFIATTBKind (unsigned int )1
#define OMFIATTBName (unsigned int )2

/* TRKG Object */
#define OMFITRKGGroupLength (unsigned int )1
#define OMFITRKGTracks (unsigned int )2

/* MOBJ Object */
#define OMFIMOBJMobID (unsigned int )1
#define OMFIMOBJLastModified (unsigned int )2
#define OMFIMOBJStartPosition (unsigned int )4
#define OMFIMOBJPhysicalMedia (unsigned int )8

/* TRAK Object */

/* CLIP Object */
#define OMFICLIPLength (unsigned int )1

/* SCLP Object */

/* TRKR Object */
#define OMFITRKRRelativeScope (unsigned int )1
#define OMFITRKRRelativeTrack (unsigned int )2

/* FILL Object */

/* ATCP Object */
#define OMFIATCPAttributes (unsigned int )1

/* TCCP Object */
#define OMFITCCPFlags (unsigned int )1
#define OMFITCCPFPS (unsigned int )2
#define OMFITCCPStartTC (unsigned int )4

/* ECCP Object */
#define OMFIECCPFilmKind (unsigned int )1
#define OMFIECCPCodeFormat (unsigned int )2
#define OMFIECCPStartEC (unsigned int )4

/* SEQU Object */
#define OMFISEQUSequence (unsigned int )1

/* SLCT Object */
#define OMFISLCTSelectedTrack (unsigned int )1

/* TRAN Object */
#define OMFITRANCutPoint (unsigned int )1

/* WARP Object */
#define OMFIWARPEditRate (unsigned int )1
#define OMFIWARPPhaseOffset (unsigned int )2

/* MASK Object */
#define OMFIMASKMaskBits (unsigned int )1
#define OMFIMASKIsDouble (unsigned int )2

/* SPED Object */
#define OMFISPEDNumerator (unsigned int )1
#define OMFISPEDDenominator (unsigned int )2

/* REPT Object */

/* MDES Object */
#define OMFIMDESMobKind (unsigned int )1

/* TXTL Object */
/* DOSL Object */
/* MACL Object */
/* UNXL Object */

/* MDFL Object */
#define OMFIMDFLIsOMFI (unsigned int )1
#define OMFIMDFLSampleRate (unsigned int )2
#define OMFIMDFLLength (unsigned int )4

/* AIFD Object */
#define OMFIAIFDSummary (unsigned int )1

/* WAVD Object */
#define OMFIWAVDSummary (unsigned int )1

/* TIFD Object */
#define OMFITIFDIsUniform (unsigned int )1
#define OMFITIFDIsContiguous (unsigned int )2
#define OMFITIFDSummary (unsigned int )4

/* TIFF Object */
#define OMFITIFFMobID (unsigned int )1
#define OMFITIFFData (unsigned int )2

/* AIFC Object */
#define OMFIAIFCMobID (unsigned int )1
#define OMFIAIFCData (unsigned int )2

/* WAVE Object */
#define OMFIWAVEMobID (unsigned int )1
#define OMFIWAVEData (unsigned int )2

/* LRCM Object */
#define OMFILRCMMobID (unsigned int )1
#define OMFILRCMFrameLength (unsigned int )2
#define OMFILRCMWordLength (unsigned int )4
#define OMFILRCMSampleRate (unsigned int )8
#define OMFILRCMLength (unsigned int )16
#define OMFILRCMCompressedLength (unsigned int )32
#define OMFILRCMDataByteOrder (unsigned int )64
#define OMFILRCMData (unsigned int )128

/* LRCD Object */
#define OMFILRCDMobID (unsigned int )1
#define OMFILRCDFrameLength (unsigned int )2
#define OMFILRCDWordLength (unsigned int )4
#define OMFILRCDCompressedLength (unsigned int )8
#define OMFILRCDIndexPresent (unsigned int )16


/**** OPTIONAL PROPERTIES ****/
#define OMFIAuthor (unsigned int )1
#define OMFICopyright (unsigned int )2
#define OMFIDate (unsigned int )4
#define OMFIByteOrder (unsigned int )8
#define OMFICharacterSet (unsigned int )16

/* HEAD Object */
#define OMFIVersion (unsigned int )1
#define OMFIClassDictionary (unsigned int )2
#define OMFISourceMobs (unsigned int )4
#define OMFICompositionMobs (unsigned int )8
#define OMFIMediaData (unsigned int )16
#define OMFIExternalFiles (unsigned int )32
#define OMFIAttributes (unsigned int )64

/* CLSD Object */
#define OMFICLSDVersion (unsigned int )1

/* CPNT Object */
#define OMFICPNTVersion (unsigned int )1
#define OMFICPNTName (unsigned int )2
#define OMFICPNTEffectID (unsigned int )4
#define OMFICPNTPrecomputed (unsigned int )8
#define OMFICPNTAttributes (unsigned int )16

/* ATTR Object */
#define OMFIATTRVersion (unsigned int )1

/* ATTB Object */
#define OMFIATTBVersion (unsigned int )1
#define OMFIATTBIntAttribute (unsigned int )2
#define OMFIATTBStringAttribute (unsigned int )4
#define OMFIATTBObjAttribute (unsigned int )8

/* TRKG Object */
#define OMFITRKGVersion (unsigned int )1

/* MOBJ Object */
#define OMFIMOBJVersion (unsigned int )1
#define OMFIMOBJUsageCode (unsigned int )2

/* TRAK Object */
#define OMFITRAKVersion (unsigned int )1
#define OMFITRAKLabelNumber (unsigned int )2
#define OMFITRAKAttributes (unsigned int )4
#define OMFITRAKFillerProxy (unsigned int )8
#define OMFITRAKTrackComponent (unsigned int )16

/* CLIP Object */
#define OMFICLIPVersion (unsigned int )1

/* SCLP Object */
#define OMFISCLPVersion (unsigned int )1
#define OMFISCLPSourceID (unsigned int )2
#define OMFISCLPSourceTrack (unsigned int )4
#define OMFISCLPSourcePosition (unsigned int )8

/* TRKR Object */
#define OMFITRKRVersion (unsigned int )1

/* FILL Object */
#define OMFIFILLVersion (unsigned int )1

/* ATCP Object */
#define OMFIATCPVersion (unsigned int )1

/* TCCP Object */
#define OMFITCCPVersion (unsigned int )1

/* ECCP Object */
#define OMFIECCPVersion (unsigned int )1

/* SEQU Object */
#define OMFISEQUVersion (unsigned int )1

/* SLCT Object */
#define OMFISLCTVersion (unsigned int )1
#define OMFISLCTIsGanged (unsigned int )2

/* TRAN Object */
#define OMFITRANVersion (unsigned int )1

/* WARP Object */
#define OMFIWARPVersion (unsigned int )1

/* MASK Object */
#define OMFIMASKVersion (unsigned int )1

/* SPED Object */
#define OMFISPEDVersion (unsigned int )1

/* REPT Object */
#define OMFIREPTVersion (unsigned int )1

/* MDES Object */
#define OMFIMDESVersion (unsigned int )1
#define OMFIMDESLocator (unsigned int )1

/* TXTL Object */
#define OMFITXTLVersion (unsigned int )1
#define OMFITXTLName (unsigned int )2

/* DOSL Object */
#define OMFIDOSLVersion (unsigned int )1
#define OMFIDOSLPathName (unsigned int )2

/* MACL Object */
#define OMFIMACLVersion (unsigned int )1
#define OMFIMACLVRef (unsigned int )2
#define OMFIMACLDirID (unsigned int )4
#define OMFIMACLFileName (unsigned int )8

/* UNXL Object */
#define OMFIUNXLVersion (unsigned int )1
#define OMFIUNXLPathName (unsigned int )2

/* MDFL Object */
#define OMFIMDFLVersion (unsigned int )1

/* AIFD Object */
#define OMFIAIFDVersion (unsigned int )1

/* WAVD Object */
#define OMFIWAVDVersion (unsigned int )1

/* TIFD Object */
#define OMFITIFDVersion (unsigned int )1
#define OMFITIFDTrailingLines (unsigned int )2
#define OMFITIFDLeadingLines (unsigned int )4
#define OMFITIFDJPEGTableID (unsigned int )8

/* TIFF Object */
#define OMFITIFFVersion (unsigned int )1

/* AIFC Object */
#define OMFIAIFCVersion (unsigned int )1

/* WAVE Object */
#define OMFIWAVEVersion (unsigned int )1

/* LRCM Object */
#define OMFILRCMVersion (unsigned int )1
#define OMFILRCMIndex (unsigned int )2

/* LRCD Object */
#define OMFILRCDVersion (unsigned int )1

/**********************************************************/
typedef struct
  {
    unsigned int  code;
    char *propName;
  } PropertyNameStruct_t;

/*
 * PropertyNameTable defines the unique "code" for each property string; a
 * code is defined as follows:
 *        Property ID = lowest 8 bits (0-7)
 *	  Class ID = bits 8-15
 * Required Bit = bit 16 (either 1 for required, or 0 for optional)
 */
#define PROPNAME_TABSIZE 139
PropertyNameStruct_t PropertyNameTable[] =
{
  {0, "OMFI:NoProperty"},
  {((REQ << 16) | (ALLClass << 8) | OMObjID), "OMFI:ObjID"},
  {((ALLClass << 8) | OMFIAuthor), "OMFI:Author"},
  {((REQ << 16) | (HEADClass << 8) | OMFIHEADByteOrder), "OMFI:ByteOrder"},
  {((ALLClass << 8) | OMFIByteOrder), "OMFI:ByteOrder"},
  {((ALLClass << 8) | OMFICharacterSet), "OMFI:CharacterSet"},
  {((ALLClass << 8) | OMFICopyright), "OMFI:Copyright"},
  {((ALLClass << 8) | OMFIDate), "OMFI:Date"},
  {((AIFCClass << 8) | OMFIAIFCVersion), "OMFI:AIFC:Version"},
  {((REQ << 16) | (AIFCClass << 8) | OMFIAIFCMobID), "OMFI:AIFC:MobID"},
  {((REQ << 16) | (AIFCClass << 8) | OMFIAIFCData), "OMFI:AIFC:Data"},
  {((AIFDClass << 8) | OMFIAIFDVersion), "OMFI:AIFD:Version"},
  {((REQ << 16) | (AIFDClass << 8) | OMFIAIFDSummary), "OMFI:AIFD:Summary"},
  {((ATCPClass << 8) | OMFIATCPVersion), "OMFI:ATCP:Version"},
  {((REQ << 16) | (ATCPClass << 8) | OMFIATCPAttributes), "OMFI:ATCP:Attributes"},
  {((ATTBClass << 8) | OMFIATTBVersion), "OMFI:ATTB:Version"},
  {((REQ << 16) | (ATTBClass << 8) | OMFIATTBKind), "OMFI:ATTB:Kind"},
  {((REQ << 16) | (ATTBClass << 8) | OMFIATTBName), "OMFI:ATTB:Name"},
  {((ATTBClass << 8) | OMFIATTBIntAttribute), "OMFI:ATTB:IntAttribute"},
  {((ATTBClass << 8) | OMFIATTBStringAttribute), "OMFI:ATTB:StringAttribute"},
  {((ATTBClass << 8) | OMFIATTBObjAttribute), "OMFI:ATTB:ObjAttribute"},
  {((ATTRClass << 8) | OMFIATTRVersion), "OMFI:ATTR:Version"},
  {((REQ << 16) | (ATTRClass << 8) | OMFIATTRAttrRefs), "OMFI:ATTR:AttrRefs"},
  {((CLIPClass << 8) | OMFICLIPVersion), "OMFI:CLIP:Version"},
  {((REQ << 16) | (CLIPClass << 8) | OMFICLIPLength), "OMFI:CLIP:Length"},
  {((CLSDClass << 8) | OMFICLSDVersion), "OMFI:CLSD:Version"},
  {((REQ << 16) | (CLSDClass << 8) | OMFICLSDClassID), "OMFI:CLSD:ClassID"},
  {((REQ << 16) | (CLSDClass << 8) | OMFICLSDParentClass), "OMFI:CLSD:ParentClass"},
  {((CPNTClass << 8) | OMFICPNTVersion), "OMFI:CPNT:Version"},
  {((REQ << 16) | (CPNTClass << 8) | OMFICPNTTrackKind), "OMFI:CPNT:TrackKind"},
  {((REQ << 16) | (CPNTClass << 8) | OMFICPNTEditRate), "OMFI:CPNT:EditRate"},
  {((CPNTClass << 8) | OMFICPNTName), "OMFI:CPNT:Name"},
  {((CPNTClass << 8) | OMFICPNTEffectID), "OMFI:CPNT:EffectID"},
  {((CPNTClass << 8) | OMFICPNTPrecomputed), "OMFI:CPNT:Precomputed"},
  {((CPNTClass << 8) | OMFICPNTAttributes), "OMFI:CPNT:Attributes"},
  {((DOSLClass << 8) | OMFIDOSLVersion), "OMFI:DOSL:Version"},
  {((DOSLClass << 8) | OMFIDOSLPathName), "OMFI:DOSL:PathName"},
  {((ECCPClass << 8) | OMFIECCPVersion), "OMFI:ECCP:Version"},
  {((REQ << 16) | (ECCPClass << 8) | OMFIECCPFilmKind), "OMFI:ECCP:FilmKind"},
  {((REQ << 16) | (ECCPClass << 8) | OMFIECCPCodeFormat), "OMFI:ECCP:CodeFormat"},
  {((REQ << 16) | (ECCPClass << 8) | OMFIECCPStartEC), "OMFI:ECCP:StartEC"},
  {((FILLClass << 8) | OMFIFILLVersion), "OMFI:FILL:Version"},
  {((HEADClass << 8) | OMFIVersion), "OMFI:Version"},
  {((REQ << 16) | (HEADClass << 8) | OMFILastModified), "OMFI:LastModified"},
  {((HEADClass << 8) | OMFIClassDictionary), "OMFI:ClassDictionary"},
  {((HEADClass << 8) | OMFISourceMobs), "OMFI:SourceMobs"},
  {((HEADClass << 8) | OMFICompositionMobs), "OMFI:CompositionMobs"},
  {((HEADClass << 8) | OMFIMediaData), "OMFI:MediaData"},
  {((HEADClass << 8) | OMFIExternalFiles), "OMFI:ExternalFiles"},
  {((HEADClass << 8) | OMFIAttributes), "OMFI:Attributes"},
  {((REQ << 16) | (HEADClass << 8) | OMFIObjectSpine), "OMFI:ObjectSpine"},
  {((MACLClass << 8) | OMFIMACLVersion), "OMFI:MACL:Version"},
  {((MACLClass << 8) | OMFIMACLVRef), "OMFI:MACL:VRef"},
  {((MACLClass << 8) | OMFIMACLDirID), "OMFI:MACL:DirID"},
  {((MACLClass << 8) | OMFIMACLFileName), "OMFI:MACL:FileName"},
  {((MASKClass << 8) | OMFIMASKVersion), "OMFI:MASK:Version"},
  {((REQ << 16) | (MASKClass << 8) | OMFIMASKMaskBits), "OMFI:MASK:MaskBits"},
  {((REQ << 16) | (MASKClass << 8) | OMFIMASKIsDouble), "OMFI:MASK:IsDouble"},
  {((MDESClass << 8) | OMFIMDESVersion), "OMFI:MDES:Version"},
  {((REQ << 16) | (MDESClass << 8) | OMFIMDESMobKind), "OMFI:MDES:MobKind"},
  {((MDESClass << 8) | OMFIMDESLocator), "OMFI:MDES:Locator"},
  {((MDFLClass << 8) | OMFIMDFLVersion), "OMFI:MDFL:Version"},
  {((REQ << 16) | (MDFLClass << 8) | OMFIMDFLIsOMFI), "OMFI:MDFL:IsOMFI"},
  {((REQ << 16) | (MDFLClass << 8) | OMFIMDFLSampleRate), "OMFI:MDFL:SampleRate"},
  {((REQ << 16) | (MDFLClass << 8) | OMFIMDFLLength), "OMFI:MDFL:Length"},
  {((MOBJClass << 8) | OMFIMOBJVersion), "OMFI:MOBJ:Version"},
  {((REQ << 16) | (MOBJClass << 8) | OMFIMOBJMobID), "OMFI:MOBJ:MobID"},
  {((REQ << 16) | (MOBJClass << 8) | OMFIMOBJLastModified), "OMFI:MOBJ:LastModified"},
  {((MOBJClass << 8) | OMFIMOBJUsageCode), "OMFI:MOBJ:UsageCode"},
  {((REQ << 16) | (MOBJClass << 8) | OMFIMOBJStartPosition), "OMFI:MOBJ:StartPosition"},
  {((REQ << 16) | (MOBJClass << 8) | OMFIMOBJPhysicalMedia), "OMFI:MOBJ:PhysicalMedia"},
  {((REPTClass << 8) | OMFIREPTVersion), "OMFI:REPT:Version"},
  {((SCLPClass << 8) | OMFISCLPVersion), "OMFI:SCLP:Version"},
  {((SCLPClass << 8) | OMFISCLPSourceID), "OMFI:SCLP:SourceID"},
  {((SCLPClass << 8) | OMFISCLPSourceTrack), "OMFI:SCLP:SourceTrack"},
  {((SCLPClass << 8) | OMFISCLPSourcePosition), "OMFI:SCLP:SourcePosition"},
  {((SEQUClass << 8) | OMFISEQUVersion), "OMFI:SEQU:Version"},
  {((REQ << 16) | (SEQUClass << 8) | OMFISEQUSequence), "OMFI:SEQU:Sequence"},
  {((SLCTClass << 8) | OMFISLCTVersion), "OMFI:SLCT:Version"},
  {((SLCTClass << 8) | OMFISLCTIsGanged), "OMFI:SLCT:IsGanged"},
  {((REQ << 16) | (SLCTClass << 8) | OMFISLCTSelectedTrack), "OMFI:SLCT:SelectedTrack"},
  {((SPEDClass << 8) | OMFISPEDVersion), "OMFI:SPED:Version"},
  {((REQ << 16) | (SPEDClass << 8) | OMFISPEDNumerator), "OMFI:SPED:Numerator"},
  {((REQ << 16) | (SPEDClass << 8) | OMFISPEDDenominator), "OMFI:SPED:Denominator"},
  {((TCCPClass << 8) | OMFITCCPVersion), "OMFI:TCCP:Version"},
  {((REQ << 16) | (TCCPClass << 8) | OMFITCCPFlags), "OMFI:TCCP:Flags"},
  {((REQ << 16) | (TCCPClass << 8) | OMFITCCPFPS), "OMFI:TCCP:FPS"},
  {((REQ << 16) | (TCCPClass << 8) | OMFITCCPStartTC), "OMFI:TCCP:StartTC"},
  {((TIFDClass << 8) | OMFITIFDVersion), "OMFI:TIFD:Version"},
  {((REQ << 16) | (TIFDClass << 8) | OMFITIFDIsUniform), "OMFI:TIFD:IsUniform"},
  {((REQ << 16) | (TIFDClass << 8) | OMFITIFDIsContiguous), "OMFI:TIFD:IsContiguous"},
  {((TIFDClass << 8) | OMFITIFDTrailingLines), "OMFI:TIFD:TrailingLines"},
  {((TIFDClass << 8) | OMFITIFDLeadingLines), "OMFI:TIFD:LeadingLines"},
  {((TIFDClass << 8) | OMFITIFDJPEGTableID), "OMFI:TIFD:JPEGTableID"},
  {((REQ << 16) | (TIFDClass << 8) | OMFITIFDSummary), "OMFI:TIFD:Summary"},
  {((TIFFClass << 8) | OMFITIFFVersion), "OMFI:TIFF:Version"},
  {((REQ << 16) | (TIFFClass << 8) | OMFITIFFMobID), "OMFI:TIFF:MobID"},
  {((REQ << 16) | (TIFFClass << 8) | OMFITIFFData), "OMFI:TIFF:Data"},
  {((LRCMClass << 8) | OMFILRCMVersion), "OMFI:LRCM:Version"},
  {((LRCMClass << 8) | OMFILRCMIndex), "OMFI:LRCM:Index"},
  {((REQ << 16) | (LRCMClass << 8) | OMFILRCMMobID), "OMFI:LRCM:MobID"},
  {((REQ << 16) | (LRCMClass << 8) | OMFILRCMFrameLength), "OMFI:LRCM:FrameLength"},
  {((REQ << 16) | (LRCMClass << 8) | OMFILRCMWordLength), "OMFI:LRCM:WordLength"},
  {((REQ << 16) | (LRCMClass << 8) | OMFILRCMSampleRate), "OMFI:LRCM:SampleRate"},
  {((REQ << 16) | (LRCMClass << 8) | OMFILRCMLength), "OMFI:LRCM:Length"},
  {((REQ << 16) | (LRCMClass << 8) | OMFILRCMCompressedLength), "OMFI:LRCM:CompressedLength"},
  {((REQ << 16) | (LRCMClass << 8) | OMFILRCMDataByteOrder), "OMFI:LRCM:DataByteOrder"},
  {((REQ << 16) | (LRCMClass << 8) | OMFILRCMData), "OMFI:LRCM:Data"},
  {((LRCDClass << 8) | OMFILRCDVersion), "OMFI:LRCD:Version"},
  {((REQ << 16) | (LRCDClass << 8) | OMFILRCDMobID), "OMFI:LRCD:MobID"},
  {((REQ << 16) | (LRCDClass << 8) | OMFILRCDFrameLength), "OMFI:LRCD:FrameLength"},
  {((REQ << 16) | (LRCDClass << 8) | OMFILRCDWordLength), "OMFI:LRCD:WordLength"},
  {((REQ << 16) | (LRCDClass << 8) | OMFILRCDCompressedLength), "OMFI:LRCD:CompressedLength"},
  {((REQ << 16) | (LRCDClass << 8) | OMFILRCDIndexPresent), "OMFI:LRCD:IndexPresent"},
  {((TRAKClass << 8) | OMFITRAKVersion), "OMFI:TRAK:Version"},
  {((TRAKClass << 8) | OMFITRAKLabelNumber), "OMFI:TRAK:LabelNumber"},
  {((TRAKClass << 8) | OMFITRAKAttributes), "OMFI:TRAK:Attributes"},
  {((TRAKClass << 8) | OMFITRAKFillerProxy), "OMFI:TRAK:FillerProxy"},
  {((TRAKClass << 8) | OMFITRAKTrackComponent), "OMFI:TRAK:TrackComponent"},
  {((TRANClass << 8) | OMFITRANVersion), "OMFI:TRAN:Version"},
  {((REQ << 16) | (TRANClass << 8) | OMFITRANCutPoint), "OMFI:TRAN:CutPoint"},
  {((TRKGClass << 8) | OMFITRKGVersion), "OMFI:TRKG:Version"},
  {((REQ << 16) | (TRKGClass << 8) | OMFITRKGGroupLength), "OMFI:TRKG:GroupLength"},
  {((REQ << 16) | (TRKGClass << 8) | OMFITRKGTracks), "OMFI:TRKG:Tracks"},
  {((TRKRClass << 8) | OMFITRKRVersion), "OMFI:TRKR:Version"},
  {((REQ << 16) | (TRKRClass << 8) | OMFITRKRRelativeScope), "OMFI:TRKR:RelativeScope"},
  {((REQ << 16) | (TRKRClass << 8) | OMFITRKRRelativeTrack), "OMFI:TRKR:RelativeTrack"},
  {((TXTLClass << 8) | OMFITXTLVersion), "OMFI:TXTL:Version"},
  {((TXTLClass << 8) | OMFITXTLName), "OMFI:TXTL:Name"},
  {((UNXLClass << 8) | OMFIUNXLVersion), "OMFI:UNXL:Version"},
  {((UNXLClass << 8) | OMFIUNXLPathName), "OMFI:UNXL:PathName"},
  {((WARPClass << 8) | OMFIWARPVersion), "OMFI:WARP:Version"},
  {((REQ << 16) | (WARPClass << 8) | OMFIWARPEditRate), "OMFI:WARP:EditRate"},
  {((REQ << 16) | (WARPClass << 8) | OMFIWARPPhaseOffset), "OMFI:WARP:PhaseOffset"},
  {((WAVDClass << 8) | OMFIWAVDVersion), "OMFI:WAVD:Version"},
  {((REQ << 16) | (WAVDClass << 8) | OMFIWAVDSummary), "OMFI:WAVD:Summary"},
  {((WAVEClass << 8) | OMFIWAVEVersion), "OMFI:WAVE:Version"},
  {((REQ << 16) | (WAVEClass << 8) | OMFIWAVEMobID), "OMFI:WAVE:MobID"},
  {((REQ << 16) | (WAVEClass << 8) | OMFIWAVEData), "OMFI:WAVE:Data"}
};


/* OMF Semantic Checker Errors */
#define SC_ERR_NONE 0

#define SC_ERR_MAXCODE 79  /* must be one greater than last #define */

/* localErrorStrings[0] = "OMFI: Success"; */

/* *** WARNING: Data Object [Label: %ld] missing from Media Data Index */
/* *** ERROR: Data Object [Label: %ld] missing from ObjectSpine */
/* *** WARNING: Mob [Label: %ld] missing from Composition Mob Index */
/* *** WARNING: Mob [Label: %ld] missing from Source Mob Index */
/* *** ERROR: Mob [Label: %ld] missing from ObjectSpine */
