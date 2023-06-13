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

/********************************************************************
 * Convert - This unittest opens an existing OMF file (of either type
 *          1.x or 2.x) and attempts to convert the file to the other
 *          revision.  It does this by mapping each construct to the
 *          equivalent construct in the other revision.  If it cannot
 *          map the construct, it either gives up or converts it to
 *          a FILL object.  It does not handle private data.  1.x 
 *          attributes are not converted to 2.x structures, they are
 *          simply ignored.  2.x effects are only converted if a 1.x
 *          equivalent structure exists.
 ********************************************************************/

#include "masterhd.h"
#define TRUE 1
#define FALSE 0

#define OMFI_ENABLE_SEMCHECK 1 /* for types defined in omPvt.h and omUtils.h */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omPublic.h"
#include "omMedia.h"
#include "omEffect.h"
#include "omCntPvt.h"
#include "omPvt.h" 

#ifdef MAKE_TEST_HARNESS
#include "UnitTest.h"
#endif
#if AVID_CODEC_SUPPORT
#include "omcAvJPED.h" 
#include "omcAvidJFIF.h" 
#endif

/***********************/
/* FUNCTION PROTOTYPES */
/***********************/
static omfErr_t processComponent(omfHdl_t infile, 
								 omfCpntObj_t incpnt,
								 omfHdl_t outfile,
								 omfCpntObj_t *outcpnt,
								 omfBool *unknownTRAN);

static omfErr_t processEffect(omfHdl_t infile, 
							  omfSegObj_t effect,
							  omfHdl_t outfile,
							  omfSegObj_t *outeffect);

static omfErr_t traverseMob(omfHdl_t infile, 
							omfMobObj_t inmob, 
							omfHdl_t outfile,
							omfMobObj_t outmob);

omfErr_t CopyMediaDescriptor(omfHdl_t infile,
							 omfObject_t inmob,
							 omfObject_t mdes,
							 omfHdl_t outfile,
							 omfObject_t *destMdes);

omfErr_t CopyMedia(omfHdl_t infile,
				   omfObject_t inMedia,
				   omfHdl_t outfile, 
				   omfObject_t *outMedia);


/* from omMobMgt.c */
omfErr_t omfiCopyPropertyExternal(
					  omfHdl_t srcFile,       /* IN - Src File Handle */
					  omfHdl_t destFile,      /* IN - Dest File Handle */
					  omfObject_t srcObj,     /* IN - Object to copy from */	
					  omfObject_t destObj,    /* IN - Object to copy to */
					  omfProperty_t srcProp,  /* IN - Src Property Name */
					  omfType_t srcType,      /* IN - Src Property Type Name */
					  omfProperty_t destProp, /* IN - Dest Property Name */
					  omfType_t destType);    /* IN - Src Property Type Name */


/***********/
/* DEFINES */
/***********/
omfFileRev_t inRev;
#define NOT_TIMEWARP 0
#define TIMEWARP 1
#define NUMMOBKIND 5

/********* MAIN PROGRAM *********/
#ifdef MAKE_TEST_HARNESS
int Convert(char *srcName, char *destname)
{
  int	argc;
  char	*argv[3];
#else
  int main(int argc, char *argv[])
{
#endif
  omfSessionHdl_t	session;
  omfHdl_t infile = NULL, outfile = NULL;
  omfMobObj_t mob = NULL, newMob = NULL;
  omfInt32 loop, mobloop, numMobs, nameSize = 128;
  omfIterHdl_t mobIter = NULL;
  omfUID_t ID;
  omfString name = NULL;
  omfTimeStamp_t createTime, lastModified;
  omfRational_t samplerate;
  omfCodecID_t codec;
  omfInt16 byteOrder;
  omfProperty_t inprop, outprop;
  omfObject_t inhead = NULL, outhead = NULL, mdes = NULL, outmdes = NULL;
  omfObject_t inDataObj = NULL, outDataObj = NULL;
  omfObjIndexElement_t inDataIndex, outDataIndex;
  omfInt32 numMedData;
  omfInt32 matches = 0;
  omfMobKind_t mobKindList[5] = {kFilmMob, kTapeMob, kFileMob, kMasterMob,
								   kCompMob};
  omfSearchCrit_t searchCrit;
  omfErr_t omfError = OM_ERR_NONE;
	omfProductIdentification_t ProductInfo;
	
	ProductInfo.companyName = "OMF Developers Desk";
	ProductInfo.productName = "Convert UnitTest";
	ProductInfo.productVersion = omfiToolkitVersion;
	ProductInfo.productVersionString = NULL;
	ProductInfo.productID = -1;
	ProductInfo.platform = NULL;
	
  XPROTECT(NULL)
	{

#ifdef MAKE_TEST_HARNESS
	  argc = 3;
	  argv[1] = srcName;
	  argv[2] = destname;
#else
#if PORT_MAC_HAS_CCOMMAND
	  argc = ccommand(&argv); 
#endif
#endif
	  if (argc < 3)
		{
		  printf("*** ERROR - Convert <infile> <outfile>\n");
		  return(-1);
		}

	  /* Null out time structures, so no uninitialized memory 
	   * (Turned up by purify, because of struct alignment differences
	   * on different platforms.)
	   */
	  memset(&lastModified, 0, sizeof(lastModified));
	  memset(&createTime, 0, sizeof(createTime));
	  lastModified.TimeVal = 0;
	  lastModified.IsGMT = '\0';
	  createTime.TimeVal = 0;
	  createTime.IsGMT = '\0';

	  /* Open the given file, and create a file of opposite OMF revision */
	  CHECK(omfsBeginSession(&ProductInfo, &session));
	  CHECK(omfmInit(session));
#if AVID_CODEC_SUPPORT
	  CHECK(omfmRegisterCodec(session, omfCodecAvJPED, kOMFRegisterLinked));
	  CHECK(omfmRegisterCodec(session, omfCodecAvidJFIF, kOMFRegisterLinked));
#endif
	  CHECK(omfsOpenFile((fileHandleType)argv[1], session, &infile));

	  CHECK(omfsFileGetRev(infile, &inRev));
	  if ((inRev == kOmfRev1x) || inRev == kOmfRevIMA)
		{
		  printf("Converting a 1.x file to a 2.x file...\n");
		  CHECK(omfsCreateFile(argv[2], session, kOmfRev2x, &outfile));
		}
	  else /* infile revision is 2.x, so create a 1.x file */
		{
		  printf("Converting a 2.x file to a 1.x file...\n");
		  CHECK(omfsCreateFile(argv[2], session, kOmfRev1x, &outfile));
		}

	  /* Convert relevant HEAD properties */
	  CHECK(omfsGetHeadObject(infile, &inhead));
	  CHECK(omfsGetHeadObject(outfile, &outhead));

	  /* Coping Byteorder property on head object */
	  if ((inRev == kOmfRev1x) || inRev == kOmfRevIMA)
		{
		  inprop = OMByteOrder;
		  outprop = OMHEADByteOrder;
		}
	  else
		{
		  inprop = OMHEADByteOrder;
		  outprop = OMByteOrder;
		}
	  CHECK(omfsReadInt16(infile, inhead, inprop, &byteOrder));
	  CHECK(omfsWriteInt16(outfile, outhead, outprop, byteOrder));

	  /* Copying the Class Dictionary is done for free.  Class 
	   * dictionary objects are registered with the session when
	   * a file is open.  When a file is created, the in memory
	   * class dictionary that was created is written to the file
	   * (whether 1.x or 2.x).  
	   *
	   * So, when infile is opened, all of its private classes will
	   * be added to the session handle.  When outfile is created and
	   * closed, this class dictionary will be written to it.
	   */
	  
	  /* Iterate over all mobs in the infile, attempting to 
	   * access them bottom up, and copy the converted
	   * equivalent to the new file.
	   */
	  CHECK(omfiIteratorAlloc(infile, &mobIter));
	  for (mobloop = 0; mobloop < NUMMOBKIND; mobloop++)
		{
		  CHECK(omfiGetNumMobs(infile, mobKindList[mobloop], &numMobs)); 
		  for (loop = 0; loop < numMobs; loop++)
			{
			  searchCrit.searchTag = kByMobKind;
			  searchCrit.tags.mobKind = mobKindList[mobloop];
			  CHECK(omfiGetNextMob(mobIter, &searchCrit, &mob));

			  name = (omfString)omfsMalloc((size_t) nameSize);
			  CHECK(omfiMobGetInfo(infile, mob, &ID, nameSize, 
								   name, &lastModified, &createTime));

			  newMob = NULL;
			  if (omfiIsACompositionMob(infile, mob, &omfError))
				{
				  CHECK(omfiCompMobNew(outfile, name, FALSE, &newMob));
				}
			  else if (omfiIsAMasterMob(infile, mob, &omfError))
				{
				  CHECK(omfmMasterMobNew(outfile, name, FALSE, &newMob));
				}
			  /* For file mobs, get the codec, and copy the media descriptor */
			  else if (omfiIsAFileMob(infile, mob, &omfError))
				{
				  CHECK(omfmFindCodec(infile, mob, &codec));
				  if(inRev == kOmfRev2x)
					{
					  CHECK(omfsReadObjRef(infile, mob, 
										   OMSMOBMediaDescription, &mdes));
					  CHECK(omfsReadRational(infile, mdes, 
											 OMMDFLSampleRate, &samplerate));
					}
				  else
					{
					  CHECK(omfsReadObjRef(infile, mob, 
										   OMMOBJPhysicalMedia, &mdes));
					  CHECK(omfsReadExactEditRate(infile, mdes, 
											  OMMDFLSampleRate, &samplerate));
					}
				  CHECK(omfmFileMobNew(outfile, name, samplerate, codec, 
									   &newMob));
				  if (inRev == kOmfRev2x)
					{
					  CHECK(omfsReadObjRef(outfile, newMob, 
										   OMMOBJPhysicalMedia, &outmdes));
					}
				  else
					{
					  CHECK(omfsReadObjRef(outfile, newMob, 
										   OMSMOBMediaDescription, &outmdes));
					}
				  
				  CHECK(CopyMediaDescriptor(infile, mob, mdes, outfile, 
											&outmdes));
				}
			  else if (omfiIsATapeMob(infile, mob, &omfError))
				{
				  CHECK(omfmTapeMobNew(outfile, name, &newMob));
				}
			  else if (omfiIsAFilmMob(infile, mob, &omfError))
				{
				  CHECK(omfmFilmMobNew(outfile, name, &newMob));
				}
			  if(newMob != NULL)
				{
				  CHECK(omfiMobGetMobID(infile, mob, &ID));
				  CHECK(omfiMobSetModTime(outfile, newMob, lastModified));
				  CHECK(omfiMobSetIdentity(outfile, newMob, ID));
				}
			  
			  omfsFree(name);
			  name = NULL;
			  
			  CHECK(traverseMob(infile, mob, outfile, newMob));
			} /* loop */

		  omfiIteratorClear(infile, mobIter); 
		} /* mobloop */

	  if (mobIter)
		{
		  omfiIteratorDispose(infile, mobIter);
		  mobIter = NULL;
		}

	  /*** Copy media data objects ***/

	  /* Converting from 1.0 to 2.0 file */
	  if ((inRev == kOmfRev1x) || inRev == kOmfRevIMA)
		{
		  inprop = OMMediaData;
		  outprop = OMHEADMediaData;   /* Convert to 2.0 file */
		  numMedData = omfsLengthObjIndex(infile, inhead, inprop);
		  for (loop = 1; loop <= numMedData; loop++)
			{
			  CHECK(omfsGetNthObjIndex(infile, inhead, inprop, 
									   &inDataIndex, loop));
			  CHECK(CopyMedia(infile, inDataIndex.Mob, outfile,
							  &outDataObj));
			  CHECK(omfsAppendObjRefArray(outfile, outhead, outprop,
										  outDataObj));
			}
		}
	  else /* Converting from 2.0 to 1.0 */
		{
		  inprop = OMHEADMediaData;
		  outprop = OMMediaData;       /* Convert to 1.0 file */
		  numMedData = omfsLengthObjRefArray(infile, inhead, inprop);
		  for (loop = 1; loop <= numMedData; loop++)
			{
			  CHECK(omfsGetNthObjRefArray(infile, inhead, inprop, 
										  &inDataObj, loop));
			  CHECK(CopyMedia(infile, inDataObj, outfile,
							  &outDataIndex.Mob));
			  CHECK(omfsReadUID(infile, inDataObj, OMMDATMobID,
								&outDataIndex.ID));
			  CHECK(omfsAppendObjIndex(outfile, outhead, outprop,
									   &outDataIndex));
			  CHECK(omfsAppendObjRefArray(outfile, outhead,
										  OMObjectSpine, 
										  outDataIndex.Mob));
			}
		}

	  /* Clean up and close */
	  CHECK(omfsCloseFile(infile));
	  CHECK(omfsCloseFile(outfile));

	  CHECK(omfsEndSession(session));
	  printf("Convert completed successfully.\n");
	} /* XPROTECT */

  XEXCEPT
	{	  
	  if (mobIter)
		omfiIteratorDispose(infile, mobIter);
	  if (name)
		omfsFree(name);
	  printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
	  return(-1);
	}
  XEND;
  return(0);
}


/*
 * traverseMob()
 *
 * Convert the given mob to its equivalent in the opposite format.
 */
static omfErr_t traverseMob(omfHdl_t infile,
							omfMobObj_t inmob,
							omfHdl_t outfile,
							omfMobObj_t outmob)
{
  omfInt32 numSlots, nameSize;
  omfInt32 loop;
  char trackName[32];
  omfMSlotObj_t slot, newSlot;
  omfSegObj_t trackSeg, outSeg;
  omfIterHdl_t slotIter = NULL;
  omfPosition_t origin;
  omfTrackID_t trackID;
  omfRational_t editRate;
  omfBool unknownTRAN = FALSE;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(infile)
	{
	  CHECK(omfiIteratorAlloc(infile, &slotIter));
	  CHECK(omfiMobGetNumSlots(infile, inmob, &numSlots));

	  for (loop = 0; loop < numSlots; loop++)
		{
		  CHECK(omfiMobGetNextSlot(slotIter, inmob, NULL, &slot));
		  if (omfiMobSlotIsTrack(infile, slot, &omfError))
			{
			  nameSize = 32;
			  CHECK(omfiTrackGetInfo(infile, inmob, slot, &editRate,
									 nameSize, (omfString)trackName, &origin,
									 &trackID, &trackSeg));
			  CHECK(processComponent(infile, trackSeg, outfile, &outSeg,
									 &unknownTRAN));
			  CHECK(omfiMobAppendNewTrack(outfile, outmob, editRate,
										  outSeg, origin, trackID, 
										  trackName, &newSlot));
			}
		  else /* Mob Slot, not a track */
			{
			  CHECK(omfiMobSlotGetInfo(infile, slot, &editRate, &trackSeg));
			  CHECK(processComponent(infile, trackSeg, outfile, &outSeg,
									 &unknownTRAN));
			  CHECK(omfiMobAppendNewSlot(outfile, outmob, editRate, outSeg,
										 &newSlot));
			}
		}
		
	  CHECK(omfiIteratorDispose(infile, slotIter));
	  slotIter = NULL;
	} /* XPROTECT */
  
  XEXCEPT
	{
	  if (slotIter)
		omfiIteratorDispose(infile, slotIter);
	  return(XCODE());
	} 
  XEND;
  return(OM_ERR_NONE);
}

/* 
 * processComponent()
 *
 * This function should not run into control points, effect slots,
 * constant values, varying values, or effect definition objects.
 * They should only occur in effects, which will be handled by the
 * process effect function.
 */
static omfErr_t processComponent(omfHdl_t infile, 
								 omfCpntObj_t incpnt,
								 omfHdl_t outfile,
								 omfCpntObj_t *outcpnt,
								 omfBool *unknownTRAN)
{
    omfInt32 loop, numSegs, numSlots, cutPoint32;
    omfInt32 wipeNumber, idSize = 64;
    omfLength_t length, effLength;
    omfPosition_t sequPos, cutPoint;
    omfCpntObj_t sequCpnt = NULL, tmpCpnt = NULL;
    omfSegObj_t slotSeg = NULL, selected = NULL;
	omfSegObj_t inputSegment = NULL, outputSegment = NULL;
    omfEffObj_t effect = NULL, tmpEffect = NULL;
    omfEDefObj_t effectDef = NULL;
    omfIterHdl_t segIter = NULL, scopeIter = NULL, selIter = NULL;
    omfTimecode_t timecode;
    omfEdgecode_t edgecode;
    omfSourceRef_t sourceRef;
    omfDDefObj_t datakind = NULL, outdatakind = NULL;
    omfMSlotObj_t tmpTrack = NULL;
    omfInt32 fadeInLen, fadeOutLen;
    omfFadeType_t fadeInType, fadeOutType;
    omfClassID_t classID;
    omfUniqueName_t effectID, datakindname, effectID1x;
    omfInt32 len = 0;
    omfRational_t speedRatio;
    omfInt16 phaseOffset1x = 0, relSlot1x = 0, relScope1x = 0;
    omfUInt32 phaseOffset = 0;
    omfUInt32 mask = 0, relScope = 0, relSlot = 0;
    omfFxFrameMaskDirection_t addOrDrop = kOmfFxFrameMaskDirNone;
	omfBool tmpUnknownTRAN = FALSE;
	omfInt64 zero64;
    omfFileRev_t fileRev;
    omfProperty_t idProp;
	omfTrackID_t foundTrackID;
	omfInt16 tmp1xTrackType = 0;
	omfUID_t nullUID = {0,0,0};
	omfInt16 trackNum1x;
	omfBool	fadeInPresent, fadeOutPresent;
    omfErr_t omfError = OM_ERR_NONE;

    XPROTECT(infile)
      {
		CHECK(omfsFileGetRev(infile,  &fileRev));
		if ((fileRev == kOmfRev1x) ||(fileRev == kOmfRevIMA))
			idProp = OMObjID;
		else /* kOmfRev2x */
			idProp = OMOOBJObjClass;

		*unknownTRAN = FALSE;
		CHECK(omfsReadClassID(infile, incpnt, idProp, classID));

		/*** SEQUENCE ***/
		if (omfiIsASequence(infile, incpnt, &omfError))
		  {
			CHECK(omfiIteratorAlloc(infile, &segIter));

			CHECK(omfiSequenceGetInfo(infile, incpnt, &datakind, &length));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			CHECK(omfiSequenceNew(outfile, outdatakind, outcpnt));

			CHECK(omfiSequenceGetNumCpnts(infile, incpnt, &numSegs));
			for (loop = 0; loop < numSegs; loop++)
			  {
				CHECK(omfiSequenceGetNextCpnt(segIter, incpnt, NULL, &sequPos, 
											  &sequCpnt));
				CHECK(processComponent(infile, sequCpnt, outfile, &tmpCpnt,
									   &tmpUnknownTRAN));
				if (!tmpUnknownTRAN)
				  {
					CHECK(omfiSequenceAppendCpnt(outfile, *outcpnt, tmpCpnt));
				  }
			  }
			omfiIteratorDispose(infile, segIter);
			segIter = NULL;
		  } /* sequence */

		/*** SOURCE CLIP ***/
		else if (omfiIsASourceClip(infile, incpnt, &omfError))
		  {
			CHECK(omfiSourceClipGetInfo(infile, incpnt, &datakind, &length, 
										&sourceRef));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);

			/* Convert the track IDs between 1x and 2x */
			if (!equalUIDs((sourceRef.sourceID), nullUID))
			  {
				/* if converting 1x -> 2x */
				if ((inRev == kOmfRev1x) || inRev == kOmfRevIMA)
				  {
					CHECK(omfsReadTrackType(infile, incpnt, OMCPNTTrackKind, 
											&tmp1xTrackType)); 
					CHECK(CvtTrackNumtoID(infile, 
										  sourceRef.sourceID, 
										  (omfInt16)sourceRef.sourceTrackID,
										  tmp1xTrackType, &foundTrackID));
					sourceRef.sourceTrackID = foundTrackID;
				  }
				else /* else if converting 2x -> 1x */
				  {
					CHECK(CvtTrackIDtoNum(outfile, sourceRef.sourceID, 
									  sourceRef.sourceTrackID, &trackNum1x));
					sourceRef.sourceTrackID = trackNum1x;
				  }
			  }
			
			CHECK(omfiSourceClipNew(outfile, outdatakind, length, sourceRef,
									outcpnt));

			/* If Sound Source Clip, check for fade info */
			if (omfiIsSoundKind(infile, datakind, kConvertTo, &omfError))
			  {
				omfError = omfiSourceClipGetFade(infile, incpnt,
												 &fadeInLen, &fadeInType,  &fadeInPresent,
												 &fadeOutLen, &fadeOutType, &fadeOutPresent);
				if (omfError == OM_ERR_NONE)
				  {
					/* Fix this, only write what exists */
					CHECK(omfiSourceClipSetFade(outfile, *outcpnt, fadeInLen,
										fadeInType, fadeOutLen, fadeOutType));
				  }
			  } /* If Soundkind */
		  } /* source clip */

		/*** TIMECODE CLIP ***/
		else if (omfiIsATimecodeClip(infile, incpnt, &omfError))
		  {
			CHECK(omfiTimecodeGetInfo(infile, incpnt, &datakind, &length,
									  &timecode));
			CHECK(omfiTimecodeNew(outfile, length, timecode, outcpnt));
		  }

		/*** EDGECODE CLIP ***/
		else if (omfiIsAnEdgecodeClip(infile, incpnt, &omfError))
		  {
			CHECK(omfiEdgecodeGetInfo(infile, incpnt,
									  &datakind, &length, &edgecode));
			CHECK(omfiEdgecodeNew(outfile, length, edgecode, outcpnt));
		  }

		/*** FILLER ***/
		else if (omfiIsAFiller(infile, incpnt, &omfError))
		  {
			CHECK(omfiFillerGetInfo(infile, incpnt, &datakind, &length));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			CHECK(omfiFillerNew(outfile, outdatakind, length, outcpnt));
		  }

		/*** 2.x EFFECT ***/
		else if (omfiIsAnEffect(infile, incpnt, &omfError))
		  {
			CHECK(processEffect(infile, incpnt, outfile, outcpnt));
		  }
 
		/*** 1.x EFFECTS: MASK, PVOL, REPT, SPED, TKFX, TNFX, WARP ***/
		else if (!strncmp(classID, "MASK", (size_t)4))
		  {
			CHECK(omfeVideoFrameMaskGetInfo(infile, incpnt, &length,
											&inputSegment, &mask, &addOrDrop,
											&phaseOffset));

			CHECK(processComponent(infile, inputSegment, outfile, 
								   &outputSegment, &tmpUnknownTRAN));

			CHECK(omfeVideoFrameMaskNew(outfile, length, outputSegment,
										mask, addOrDrop, 
										(omfUInt32)phaseOffset, outcpnt));
		  } /* MASK */

		else if (!strncmp(classID, "REPT", (size_t)4))
		  {
			CHECK(omfeVideoRepeatGetInfo(infile, incpnt, &length, 
										 &inputSegment, &phaseOffset));
			CHECK(processComponent(infile, inputSegment, outfile, 
								   &outputSegment, &tmpUnknownTRAN));
			CHECK(omfeVideoRepeatNew(outfile, length, outputSegment,
									 phaseOffset, outcpnt));
		  } /* REPT */

		else if (!strncmp(classID, "SPED", (size_t)4))
		  {
			CHECK(omfeVideoSpeedControlGetInfo(infile, incpnt, &length,
											   &inputSegment, &speedRatio,
											   &phaseOffset));
			CHECK(processComponent(infile, inputSegment, outfile, 
								   &outputSegment, &tmpUnknownTRAN));
			CHECK(omfeVideoSpeedControlNew(outfile, length, outputSegment,
										   speedRatio, phaseOffset, outcpnt));
		  } /* SPED */

		/* NOTE: What does a PVOL translate to? */
		else if (!strncmp(classID, "PVOL", (size_t)4))
		  {
			CHECK(omfiComponentGetInfo(infile, incpnt, &datakind, &length));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			CHECK(omfiFillerNew(outfile, outdatakind, length, outcpnt));
			printf("WARNING: Converting 1.x PVOL to FILL for 2.x...\n");
		  } /* PVOL */

		/* Avid Media Composer 1.0 private effect type, subclass of TRKG */
		else if (!strncmp(classID, "TKFX", (size_t)4))
		  {
			CHECK(omfiComponentGetInfo(infile, incpnt, &datakind, &length));
			CHECK(omfsReadString(infile, incpnt, OMCPNTEffectID, effectID1x, 
								 OMUNIQUENAME_SIZE));
			strcpy(effectID, "omfi:effect:");
			strcat(effectID, effectID1x);
			CHECK(omfiEffectDefNew(outfile, effectID, NULL, NULL, NULL,
								   NOT_TIMEWARP, &effectDef));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			CHECK(omfiEffectNew(outfile, outdatakind, length, effectDef,
								outcpnt));

			/* NOTE: add slots */
		  } /* TKFX */

		/* Avid Media Composer 1.0 private transition type, subclass of TRAN */
		else if (!strncmp(classID, "TNFX", (size_t)4))
		  {
			CHECK(omfiComponentGetInfo(infile, incpnt, &datakind, &length));
			CHECK(omfsReadInt32(infile, incpnt, OMTRANCutPoint, &cutPoint32));
			CHECK(omfsCvtInt32toInt64(cutPoint32, &cutPoint));
			CHECK(omfsReadString(infile, incpnt, OMCPNTEffectID, effectID1x, 
								 OMUNIQUENAME_SIZE));
			strcpy(effectID, "omfi:effect:");
			strcat(effectID, effectID1x);
			CHECK(omfiEffectDefNew(outfile, effectID, NULL, NULL, NULL,
								   NOT_TIMEWARP, &effectDef));
			
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			CHECK(omfiEffectNew(outfile, outdatakind, length, effectDef, 
								&tmpEffect));
			CHECK(omfiTransitionNew(outfile, datakind, length, 
									cutPoint,
									tmpEffect, outcpnt));
			/* NOTE: add slots */
		  } /* TKFX */

		/*** TRANSITION  ***/
		else if (omfiIsATransition(infile, incpnt, &omfError))
		  {
			CHECK(omfiTransitionGetInfo(infile, incpnt, &datakind,
										&length, &cutPoint, 
										&effect));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			
			/* NOTE: Could use isA functions for effect types */
			/* If 1.x TRAN -> 2.x TRAN, create an effect object.
			 * This will only work for recognized transitions (SMPTE Wipe,
			 * and BlendDissolve.
			 */
			if ((inRev == kOmfRev1x) || inRev == kOmfRevIMA)
			  {
				omfError = omfsReadString(infile, incpnt, OMCPNTEffectID, 
										  effectID1x, OMUNIQUENAME_SIZE);
				if (omfError == OM_ERR_NONE)
				  {
					if (!strcmp(effectID1x, "Blend:Dissolve") && 
						(omfiIsSoundKind(infile, datakind, kExactMatch, 
										 &omfError)))
					  {
						CHECK(omfeMonoAudioDissolveNew(outfile, length, 
													   NULL, NULL, NULL, 
													   &tmpEffect));
					  }
					else if (!strcmp(effectID1x, "Blend:Dissolve") && 
							 (omfiIsPictureWithMatteKind(infile, datakind, 
													 kConvertTo, &omfError)))
					  {
						CHECK(omfeVideoDissolveNew(outfile, length, NULL, NULL,
												   NULL, &tmpEffect));
					  }
					else if (!strncmp(effectID1x, "Wipe:SMPTE:", 11))
					  {
						CHECK(omfeSMPTEVideoWipeGetInfo(infile, incpnt, NULL,
														NULL, NULL, NULL,
														&wipeNumber, NULL));
						CHECK(omfeSMPTEVideoWipeNew(outfile, length, NULL, 
													NULL, NULL, wipeNumber, 
													NULL, &tmpEffect));
					  }
					else /* Not SMPTEWipe or simple dissolve */
					  tmpUnknownTRAN = TRUE;
					
					if (!tmpUnknownTRAN)
					  effect = tmpEffect;
				  }
				else if (omfError == OM_ERR_PROP_NOT_PRESENT) /* No effectID */
				  {
					tmpUnknownTRAN = TRUE;
				  }
				else
				  {
					RAISE(omfError);
				  }
				if (tmpUnknownTRAN)
				  {
					printf("WARNING: Converting unknown 1.x TRAN to FILL...\n");
					CHECK(omfiFillerNew(outfile, outdatakind, length, outcpnt));
#if 0
					printf("WARNING: Converting unknown 1.x TRAN into cut...\n");
#endif
					*unknownTRAN = tmpUnknownTRAN;
				  }
				else
				  {
					CHECK(omfiTransitionNew(outfile, outdatakind, length, 
											cutPoint, tmpEffect, 
											outcpnt));
				  }
			  } /* kOmfRev1x */

			/* If 2.x TRAN -> 1.x TRAN, add the Effect ID private property
			 * if simple dissolve or wipe.
			 */
			if (inRev == kOmfRev2x)
			  {
				CHECK(omfiEffectGetInfo(infile, effect, NULL,
										&effLength, &effectDef));
				CHECK(omfiEffectDefGetInfo(infile, effectDef, idSize, effectID,
										   0, NULL, 0, NULL, NULL, NULL));
				if (!strcmp(effectID, "omfi:effect:MonoAudioDissolve"))
				  {
					CHECK(omfeMonoAudioDissolveNew(outfile, effLength, 
												   NULL, NULL,
												   NULL, &tmpEffect));
					CHECK(omfiTransitionNew(outfile, outdatakind, length, 
											cutPoint,
											NULL, outcpnt));
				  }
				if (!strcmp(effectID, "omfi:effect:VideoDissolve"))
				  {
					CHECK(omfeVideoDissolveNew(outfile, effLength, 
											   NULL, NULL,NULL, &tmpEffect));
					CHECK(omfiTransitionNew(outfile, outdatakind, length, 
											cutPoint,
											tmpEffect, outcpnt));
				  }
				else if (!strcmp(effectID, "omfi:effect:SMPTEVideoWipe"))
				  {
					CHECK(omfeSMPTEVideoWipeGetInfo(infile, effect, &length, 
													NULL, NULL, 
													NULL, &wipeNumber, NULL));
					CHECK(omfeSMPTEVideoWipeNew(outfile, effLength, NULL,
												NULL, NULL, wipeNumber, NULL,
												&tmpEffect));
					CHECK(omfiTransitionNew(outfile, outdatakind, length, 
											cutPoint,
											tmpEffect, outcpnt));
				  }
				/* For 1.x Delete the effect object, since not used */
				CHECK(omfsObjectDelete(outfile, tmpEffect));
			  }
		  } /* Transition */

		/*** CONST VALUE ***/
		/* If we found a constant value, this must be a 2.x file.  Write
		 * filler with the same length and datakind into the 1.x file.
		 */
		else if (omfiIsAConstValue(infile, incpnt, &omfError))
		  {
			omfsCvtInt32toInt64(0, &zero64);
			CHECK(omfiConstValueGetInfo(infile, incpnt, &datakind, &length, 
										zero64, NULL, NULL));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			printf("WARNING: Converting CVAL to FILL for 1.x...\n");
			CHECK(omfiFillerNew(outfile, outdatakind, length, outcpnt));
		  } /* Const Value */

		/*** VARYING VALUE ***/
		/* If we found a varying value, this must be a 2.x file.  Write
		 * filler with the same length and datakind into the 1.x file.
		 */
		else if (omfiIsAVaryValue(infile, incpnt, &omfError))
		  {
			CHECK(omfiVaryValueGetInfo(infile, incpnt, &datakind, &length,
									   NULL));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			printf("WARNING: Converting VVAL to FILL for 1.x...\n");
			CHECK(omfiFillerNew(outfile, outdatakind, length, outcpnt));
		  } /* varying value */

		/*** NESTED SCOPE ***/
		/* If we found a nested scope, this must be a 2.x file.  Write
		 * filler with the same length and datakind into the 1.x file.
		 */
		else if (omfiIsANestedScope(infile, incpnt, &omfError))
		  {
			CHECK(omfiNestedScopeGetInfo(infile, incpnt, &datakind, &length));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			printf("WARNING: Converting NEST to FILL for 1.x...\n");
			CHECK(omfiFillerNew(outfile, outdatakind, length, outcpnt));
			
#if 0
			CHECK(omfiIteratorAlloc(infile, &scopeIter));
			CHECK(omfiScopeGetNumSlots(infile, incpnt, &numSlots));
			for (loop = 0; loop < numSlots; loop++)
			  {
				CHECK(omfiNestedScopeGetNextSlot(scopeIter, incpnt, NULL, 
												 &slotSeg));
				CHECK(processComponent(infile, slotSeg, outfile, outcpnt,
									   &tmpUnknownTRAN));
			  }
			omfiIteratorDispose(infile, scopeIter);
			scopeIter = NULL;
#endif
		  } /* nested scope */

		/*** SELECTOR ***/
		else if (omfiIsASelector(infile, incpnt, &omfError))
		  {
			CHECK(omfiSelectorGetInfo(infile, incpnt, &datakind, &length, 
									  &selected));
			
			CHECK(processComponent(infile, selected, outfile, &tmpCpnt,
								   &tmpUnknownTRAN));
			
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			CHECK(omfiSelectorNew(outfile, outdatakind, length, outcpnt));
			CHECK(omfiSelectorSetSelected(outfile, *outcpnt, tmpCpnt));
			
			CHECK(omfiIteratorAlloc(infile, &selIter));
			CHECK(omfiSelectorGetNumAltSlots(infile, incpnt, &numSlots));
			for (loop = 0; loop < numSlots; loop++)
			  {
				CHECK(omfiSelectorGetNextAltSlot(selIter, incpnt, NULL, 
												 &slotSeg));
				CHECK(processComponent(infile, slotSeg, outfile, &tmpCpnt,
									   &tmpUnknownTRAN));
				CHECK(omfiSelectorAddAlt(outfile, *outcpnt, tmpCpnt));
			  }	    
			omfiIteratorDispose(infile, selIter);
			selIter = NULL;
		  } /* selector */

		/*** MEDIA GROUP ***/
		/* If we found a media group, this must be a 2.x file.  Write
		 * filler with the same length and datakind into the 1.x file.
		 */
		else if (omfiIsAMediaGroup(infile, incpnt, &omfError))
		  {
			CHECK(omfiMediaGroupGetInfo(infile, incpnt, &datakind, &length));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			printf("WARNING: Converting MGRP to FILL for 1.x...\n");
			CHECK(omfiFillerNew(outfile, outdatakind, length, outcpnt));
		  } /* media group */

		/*** SCOPE REFERENCE ***/
		else if (omfiIsAScopeRef(infile, incpnt, &omfError))
		  {
			CHECK(omfiScopeRefGetInfo(infile, incpnt, &datakind, &length,
									  &relScope, &relSlot));
			
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			
			CHECK(omfsObjectNew(outfile, "TRKR", outcpnt));
			CHECK(ComponentSetNewProps(outfile, *outcpnt, length,outdatakind));
			CHECK(omfsWriteInt16(outfile, *outcpnt, OMTRKRRelativeScope,
								 (omfInt16)relScope));
			relSlot1x = (short) (relSlot * -1);
			CHECK(omfsWriteInt16(outfile, *outcpnt, OMTRKRRelativeTrack,
								 relSlot1x));
		  }

		/*** TRACK REFERENCE (1.x) ***/
		else if (!strncmp(classID, "TRKR", (size_t)4))
		  {
			CHECK(omfiComponentGetInfo(infile, incpnt, &datakind, &length));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			CHECK(omfsReadInt16(infile, incpnt, OMTRKRRelativeScope,
								&relScope1x));
			CHECK(omfsReadInt16(infile, incpnt, OMTRKRRelativeTrack,
								&relSlot1x));

			relSlot = relSlot1x * -1;
			CHECK(omfiScopeRefNew(outfile, outdatakind, length,
								  (omfUInt32)relScope1x, 
								  relSlot, outcpnt));
		  }

		/*** NOTE: EDITRATE CONVERTOR  ***/
		else if (!strncmp(classID, "ERAT", (size_t)4))
		  {
		  }

		/*** ELSE... ***/
		/* Replace anything else with filler.  This is the place holder
		 * for 1.x classes that went away.
		 * This should cover: ATCP, etc.
		 */
		else 
		  {
			CHECK(omfiComponentGetInfo(infile, incpnt, &datakind, &length));
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			printf("WARNING: Converting %.4s to FILL for 1.x...\n", classID);
			CHECK(omfiFillerNew(outfile, outdatakind, length, outcpnt));
		  } /* else */

      } /* XPROTECT */

    XEXCEPT
      {
		if (segIter)
		  omfiIteratorDispose(infile, segIter);
		if (selIter)
		  omfiIteratorDispose(infile, selIter);
		return(XCODE());
      }
    XEND;

    return(OM_ERR_NONE);
}

static omfErr_t processEffect(omfHdl_t infile, 
							  omfSegObj_t effect,
							  omfHdl_t outfile,
							  omfSegObj_t *outeffect)
{
    omfInt32 idSize = 64, nameSize = 64, descSize = 64;
    omfLength_t length;
    omfEDefObj_t effectDef;
    omfDDefObj_t datakind = NULL, outdatakind = NULL;
    char nameBuf[64];
    omfUniqueName_t effectID, datakindname;
    omfBool isTimeWarp = FALSE;
    omfIterHdl_t effectIter = NULL, renderIter = NULL;
    omfESlotObj_t slot = NULL;
    omfSegObj_t argValue = NULL, renderClip = NULL, precompute = NULL;
    omfObject_t fileMob = NULL;
    omfUInt32 phaseOffset = 0;
    omfSegObj_t inputSegment = NULL, outputSegment = NULL;
    omfUInt32 mask = 0;
    omfFxFrameMaskDirection_t addOrDrop = kOmfFxFrameMaskDirNone;
    omfRational_t speedRatio;
    omfMSlotObj_t tmpTrack = NULL;
    omfBool foundEffect = FALSE;
	omfBool unknownTRAN = FALSE;
    omfErr_t omfError = OM_ERR_NONE;

    XPROTECT(infile)
      {
		CHECK(omfiEffectGetInfo(infile, effect, 
								&datakind, &length, &effectDef));

		/* Get Effect Definition Information */
		CHECK(omfiEffectDefGetInfo(infile, effectDef, idSize, effectID,
								   nameSize, nameBuf,
								   0, NULL,
								   NULL, NULL));

		if (!strcmp(effectID, "omfi:effect:VideoSpeedControl"))
		  {
			CHECK(omfeVideoSpeedControlGetInfo(infile, effect, 
											   &length, &inputSegment,
											   &speedRatio, &phaseOffset));
			
			CHECK(processComponent(infile, inputSegment, outfile, 
								   &outputSegment, &unknownTRAN));

			CHECK(omfeVideoSpeedControlNew(outfile, length, outputSegment,
									   speedRatio, phaseOffset, outeffect));
			foundEffect = TRUE;
		  }

		else if (!strcmp(effectID, "omfi:effect:VideoFrameMask"))
		  {
			CHECK(omfeVideoFrameMaskGetInfo(infile, effect, 
											&length, &inputSegment,
											&mask, &addOrDrop, &phaseOffset));

			CHECK(processComponent(infile, inputSegment, outfile, 
								   &outputSegment, &unknownTRAN));

			CHECK(omfeVideoFrameMaskNew(outfile, length, outputSegment,
										mask, addOrDrop, 
										(omfUInt32)phaseOffset, outeffect));
			foundEffect = TRUE;
		  }

		else if (!strcmp(effectID, "omfi:effect:VideoRepeat"))
		  {
			CHECK(omfeVideoRepeatGetInfo(infile, effect, 
										 &length, &inputSegment,
										 &phaseOffset));

			CHECK(processComponent(infile, inputSegment, outfile, 
								   &outputSegment, &unknownTRAN));

			CHECK(omfeVideoRepeatNew(outfile, length, outputSegment,
									 phaseOffset, outeffect));
			foundEffect = TRUE;
		  }

		/* VideoDissolve, MonoAudioDissolve, StereoAudioDissolve, 
		 * StereoAudioGain, MonoAudioPan, VideoFadeToBlack, MonoAudioMixdown, 
		 * SMPTEVideoWipe, MonoAudioGain
		 */
		else 
		  {
			printf("WARNING: Converting unrecognized EFFE to FILL for 1.x...\n");
			CHECK(omfiDatakindGetName(infile, datakind, OMUNIQUENAME_SIZE,
									  datakindname));
			omfiDatakindLookup(outfile, datakindname, &outdatakind,
							   &omfError);
			CHECK(omfiFillerNew(outfile, outdatakind, length, outeffect));
#if 0
			/* Iterate through Effect Slots and get information */
			CHECK(omfiIteratorAlloc(infile, &effectIter));
			CHECK(omfiEffectGetNumSlots(infile, effect, &numSlots));
			for (loop = 0; loop < numSlots; loop++)
			  {
				CHECK(omfiEffectGetNextSlot(effectIter, effect, NULL, &slot));
				CHECK(omfiEffectSlotGetInfo(infile, slot, &argID, &argValue));
				CHECK(processComponent(infile, argValue, outfile, outeffect,
									   &tmpUnknownTRAN));
			  }
			omfiIteratorDispose(infile, effectIter);
			effectIter = NULL;
#endif
		  }

		if (foundEffect)
		  {
			/* Convert final rendering to CPNT:Precomputed effect property */
			omfError = omfiEffectGetFinalRender(infile, effect, &renderClip);
			if (renderClip)
			  {
				CHECK(processComponent(infile, renderClip, outfile, 
									   &precompute, &unknownTRAN));
				CHECK(omfsWriteObjRef(outfile, *outeffect, OMCPNTPrecomputed, 
									  precompute));
			  }

			/* Bypass didn't exist in 1.x, so ignore it */
		  } /* foundEffect */

      } /* XPROTECT */

    XEXCEPT
      {
		return(XCODE());
      }
    XEND;

    return(OM_ERR_NONE);
}

omfErr_t CopyMediaDescriptor(omfHdl_t infile,
							 omfObject_t inmob,
							 omfObject_t mdes,
							 omfHdl_t outfile,
							 omfObject_t *outmdes)
{
  omfIterHdl_t    propIter = NULL;
  omfProperty_t   prop, newProp;
  omfType_t       type, newType;
  omfClassID_t classID;
  omfPhysicalMobType_t mobType;
  omfErr_t omfError = OM_ERR_NONE;
  omfFileRev_t fileRev;
  omfProperty_t idProp;

  XPROTECT(infile)
    {
	  CHECK(omfsFileGetRev(infile,  &fileRev));
	  if ((fileRev == kOmfRev1x) ||(fileRev == kOmfRevIMA))
		  idProp = OMObjID;
	  else /* kOmfRev2x */
		idProp = OMOOBJObjClass;

      CHECK(omfsReadClassID(infile, mdes, idProp,classID));

      omfiIteratorAlloc(infile, &propIter);
      omfiGetNextProperty(propIter, mdes, &prop, &type);
      while ((omfError == OM_ERR_NONE) && prop) 
		{
		  newType = type;
		  newProp = prop;

		  if ((type == OMObjRef) || (type == OMObjRefArray))
			{
			  /* Don't copy locators yet */
			}
		  else if ((type == OMObjectTag) || (type == OMClassID))
			{
			  /* skip */
			}
		  else if(type == OMPhysicalMobType)
			{
			  /* skip */
			}
		  else
			{
			  if (type == OMVarLenBytes)  /* Converting from 1.0 to 2.0 */
				{
				  newType = OMDataValue;
				  newProp = prop;
				}
			  else if (type == OMDataValue) /* From 2.0 to 1.0 */
				{
					if ((strncmp(classID, "TIFD", (size_t)4) == 0) ||
						(strncmp(classID, "AIFD", (size_t)4) == 0) ||
						(strncmp(classID, "WAVD", (size_t)4) == 0) )
						newType = OMVarLenBytes;
				}
			  else if (prop == OMMDFLLength)
				{
				  if (type == OMLength32)
					  {
						newType = OMInt32;
						newProp = prop;
					  }
				  else if (type == OMInt32)
					  {
						newType = OMLength32;
						newProp = prop;
					  }
				}
			  else if (type == OMExactEditRate)
				  {
					newType = OMRational;
					newProp = prop;
				  }
			  else if ((type == OMRational) && (prop == OMMDFLSampleRate))
				  {
					newType = OMExactEditRate;
					newProp = prop;
				  }
			  CHECK(omfiCopyPropertyExternal(infile, outfile, mdes,
						*outmdes, prop, type, newProp, newType));
			}
		  omfError = omfiGetNextProperty(propIter, mdes, &prop, &type);
		} /* Property loop */
	  
      omfiIteratorDispose(infile, propIter);
      propIter = NULL;

      if (inRev == kOmfRev2x)
		{
		  if (omfiIsATapeMob(infile, inmob, &omfError) && 
			  strncmp(classID, "MDTP", 4))
			mobType = PT_TAPE_MOB;
		  else if (omfiIsATapeMob(infile, inmob, &omfError)) 
			mobType = PT_NAGRA_MOB;
		  else if (omfiIsAFilmMob(infile, inmob, &omfError))
			mobType = PT_FILM_MOB;
		  else if (omfiIsAFileMob(infile, inmob, &omfError))
			mobType = PT_FILE_MOB;
		  else 
			printf("INVALID MOB TYPE\n");
		  omfsWritePhysicalMobType(outfile, *outmdes, OMMDESMobKind, mobType);
		}
    } /* XPROTECT */
  
  XEXCEPT
    {
      if (XCODE() == OM_ERR_NO_MORE_OBJECTS)
		return(OM_ERR_NONE);
      else 
		{
		  if (propIter)
			omfiIteratorDispose(infile, propIter);
		}
    }
  XEND;

  return(OM_ERR_NONE);
}


omfErr_t CopyMedia(omfHdl_t infile,
				   omfObject_t inMedia,
				   omfHdl_t outfile, 
				   omfObject_t *outMedia)
{
  omfIterHdl_t    propIter = NULL;
  omfProperty_t   prop, newProp;
  omfType_t       type, newType;
  omfClassID_t classID;
  omfErr_t omfError = OM_ERR_NONE;
  omfFileRev_t fileRev;
  omfProperty_t idProp;

  XPROTECT(infile)
    {
	  CHECK(omfsFileGetRev(infile,  &fileRev));
	  if ((fileRev == kOmfRev1x) ||(fileRev == kOmfRevIMA))
		  idProp = OMObjID;
	  else /* kOmfRev2x */
		  idProp = OMOOBJObjClass;

	  CHECK(omfsReadClassID(infile, inMedia, idProp, classID));
      CHECK(omfsObjectNew(outfile, classID, outMedia));
	  
      omfiIteratorAlloc(infile, &propIter);
      omfiGetNextProperty(propIter, inMedia, &prop, &type);
      while ((omfError == OM_ERR_NONE) && prop) 
		{
		  if ((type == OMObjectTag) || (type == OMClassID))
			{
			  /* skip */
			}
		  else
			{
			  newType = type;
			  newProp = prop;
			  if (type == OMVarLenBytes)  /* Converting from 1.0 to 2.0 */
				{
				  newType = OMDataValue;
				  newProp = prop;
				}
			  else if (type == OMDataValue) /* From 2.0 to 1.0 */
				{
					newType = OMVarLenBytes;
					newProp = prop;
				}
			  else if (type == OMUID)
				{
				/* If converting from 1.0 to 2.0 */  
				if ((inRev == kOmfRev1x) || inRev == kOmfRevIMA)
					{
						newProp = OMMDATMobID;
						newType = type;
					}
				else /* Converting from 2.0 to 1.0 - depends on media type */
					{
					if (!strncmp(classID, "TIFF", (size_t)4))
						newProp = OMTIFFMobID;
					else if (!strncmp(classID, "AIFC", (size_t)4))
						newProp = OMAIFCMobID;
					else if (!strncmp(classID, "WAVE", (size_t)4))
						newProp = OMWAVEMobID;
					newType = type;
					}
				}
			CHECK(omfiCopyPropertyExternal(infile, outfile, inMedia,
						*outMedia, prop, type, newProp, newType));
			}
		  omfError = omfiGetNextProperty(propIter, inMedia, &prop, &type);
		} /* Property loop */

      omfiIteratorDispose(infile, propIter);
      propIter = NULL;
    } /* XPROTECT */

  XEXCEPT
    {
      if (XCODE() == OM_ERR_NO_MORE_OBJECTS)
		return(OM_ERR_NONE);
      else 
		{
		  if (propIter)
			omfiIteratorDispose(infile, propIter);
		}
    }
  XEND;

  return(OM_ERR_NONE);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
