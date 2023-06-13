/*---------------------------------------------------------------------------*
 |                                                                           |
 |                          <<<    Values.c     >>>                          |
 |                                                                           |
 |                  Container Manager Common Value Routines                  |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  7/22/92                                  |
 |                                                                           |
 |                     Copyright Apple Computer, Inc. 1992                   |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file contains various routines needed for value operations, e.g., CMWriteValueData()
 CMDeleteValueData(), etc.  Some of these routines are used when applying updates at open
 time (insert and delete data, move a value header).
 */


#include <stddef.h>
#include <stdio.h>
#include "omCvt.h"

#ifndef __CMTYPES__
#include "CMTypes.h"
#endif
#ifndef __CM_API__
#include "CMAPI.h"
#endif
#ifndef __LISTMGR__
#include "ListMgr.h"
#endif
#ifndef __VALUEROUTINES__
#include "Values.h"
#endif
#ifndef __TOCENTRIES__
#include "TOCEnts.h"
#endif
#ifndef __TOCOBJECTS__
#include "TOCObjs.h"
#endif
#ifndef __CONTAINEROPS__
#include "Containr.h"
#endif
#ifndef __HANDLERS__
#include "Handlers.h"
#endif
#ifndef __FREESPACE__
#include "FreeSpce.h"
#endif
#ifndef __SESSIONDATA__
#include "Session.h"
#endif
#ifndef __ERRORRPT__
#include "ErrorRpt.h"
#endif
#ifndef __UTILITYROUTINES__
#include "Utility.h"
#endif

                                  CM_CFUNCTIONS

/* The following generates a segment directive for Mac only due to 32K Jump Table       */
/* Limitations.  If you don't know what I'm talking about don't worry about it.  The    */
/* default is not to generate the pragma.  Theoritically unknown pragmas in ANSI won't  */
/* choke compilers that don't recognize them.  Besides why would they be looked at if   */
/* it's being conditionally skipped over anyway?  I know, famous last words!            */

#if CM_MPW
#pragma segment CMValueOps
#endif


/*----------------------------------------------------------------------------------*
 | cmGetStartingValue - find value and value offset corresponding to value position |
 *----------------------------------------------------------------------------------*

 This returns the TOCValuePtr to the value entry which contains the stream starting
 offset, startingOffset.  Also returned in valueOffset is the offset within the returned
 value entry which is the startingOffset'th byte.

 Note, NULL is returned for the value pointer if we cannot find the offset. This could
 happen when the startingOffset is beyond the end of the value.

 This routine is necessary because of continued values (segments) which are represented by
 a list of TOCValue entries off of a TOCValueHdr.  Each entry represents a discontinous
 segment of the value data which is always viewed as a stream of contiguous bytes even
 though they are not.  This routine is one of those that allows its caller to view the
 stream as contiguous.
*/

TOCValuePtr cmGetStartingValue(TOCValueHdrPtr theValueHdr, CMCount startingOffset,
                               CMCount *valueOffset)
{
  TOCValuePtr   theValue = (TOCValuePtr)cmGetListHead(&theValueHdr->valueList);
  CMCount prevOffset, offset, middleOffset;
  unsigned int  offset32;

  if (theValue == NULL) return (NULL);                  /* saftey test                  */

  /* Global names are handled separately. There is only one value entry for these. The  */
  /* reason we split these off is because there is no length in the value. The scanning */
  /* logic below assumes there is always a length in the value, which there is except   */
  /* for this one special case.                                                         */

  if ((theValueHdr->valueFlags & ValueGlobal) != 0) {
    offset32 = GetGlobalNameLength(theValue->value.globalName.globalNameSymbol) + 1;
    omfsCvtUInt32toInt64(offset32, &offset);
    if (omfsInt64LessEqual(offset, startingOffset))
    	return (NULL);
    *valueOffset = startingOffset;
    return (theValue);
  }

  /* Many times the user is just appending on to the end of value. We can quickly test  */
  /* for this and eliminate that possibility...                                         */

  if (omfsInt64GreaterEqual(startingOffset, theValueHdr->size)) /* quick check for appending */
    return (NULL);

  /* So we don't have a global name and we're not appending.  Damn!  But most values    */
  /* may not be continued either.  So if there is only one value there's not much       */
  /* question of which value segment to use!                                            */

  if (cmCountListCells(&theValueHdr->valueList) == 1) {
    if (omfsInt64LessEqual(theValueHdr->size, startingOffset))
    	 return (NULL);
    *valueOffset = startingOffset;
    return (theValue);
  }

  /* Ok we got to scan the damn thing.  We potentially can limit the scan if we assume  */
  /* that all the continued value segments are approximately the same size.  Then we    */
  /* should scan the values low-to-high (left-to-right) by ascending offset if the      */
  /* startingOffset is less than half the total size. We should scan the values hit-to- */
  /* low (right-to-left) if the startingOffset is more than half way.  That's as fancy  */
  /* as we get.  A binary search would be nice.  But since we have a linked list the    */
  /* scanning wouldn't really be minimized that much.                                   */

  omfsDivideInt64byInt32(theValueHdr->size, 2, &middleOffset, NULL);
  if (omfsInt64GreaterEqual(startingOffset, middleOffset)) {        /* scan right-to-left...        */
    prevOffset = theValueHdr->size;                     /* current max offset is size   */
    theValue = (TOCValuePtr)cmGetListTail(&theValueHdr->valueList);
    while (theValue) {                                  /* scan to we find it...        */
      omfsSubInt64fromInt64(theValue->value.notImm.valueLen, &prevOffset);      /* offset to this segment     */
      if (omfsInt64LessEqual(prevOffset, startingOffset))
      	 break;            /* break if we found it       */
      theValue = (TOCValuePtr)cmGetPrevListCell(theValue);/* keep scanning...           */
    } /* while */
  } else {                                              /* scan left-to-right...        */
    omfsCvtUInt32toInt64(0, &offset);                    /* this is current max offset   */
    while (theValue) {                                  /* scan to we find it...        */
      prevOffset = offset;                                /* save current total offset  */
      omfsAddInt64toInt64(theValue->value.notImm.valueLen, &offset);          /* add size of this segment   */
      if (omfsInt64Greater(offset, startingOffset))
      	 break;                 /* break if we found it       */
      theValue = (TOCValuePtr)cmGetNextListCell(theValue);/* keep scanning...           */
    } /* while */
  }

  if (theValue == NULL) return (NULL);                  /* offset out of range          */

  *valueOffset = startingOffset;
  omfsSubInt64fromInt64(prevOffset, valueOffset);           /* relativize the offset        */
  return (theValue);                                    /* return value segment entry   */
}


/*------------------------------------------------------------------------------*
 | cmRead1ValueData - copy data for a single value (segment) to caller's buffer |
 *------------------------------------------------------------------------------*

 This routine copies the data for the specified value (NOT a value header -- continued
 values are not worried about here), starting at the specified offset, TO the caller's
 buffer.  A maximum of maxSize characters or the size of the data, which ever is smaller,
 is copied.  The function returns the amount of data copied to the buffer.

 It is possible here that what we are reading is from a container TOC that was created for
 updating.  Such a TOC are a mixture of entries from the base TOC and all of its updaters.
 Each updater has its own handler package to access the value data in its respective
 container.  To get at the proper handlers for the value we must know which container
 "owns" the value.  To that end each value segment entry (a TOCValue) has a container
 pointer of its own.  This points to the container that created that value segment and
 that's the container we use to get the proper handlers.  It is NOT necessarily to the
 container who "owns" the entire TOC.  It will be if we're not updating, but we never
 count on that fact.

 Note, this routine handles the special cases for immediate data.
*/

CMSize32 cmRead1ValueData(TOCValuePtr theValue, unsigned char *buffer,
                               CMCount offset, CMSize32 maxSize)
{
  ContainerPtr  container = theValue->container;  /* use container "owning" the value   */
  CMSize	len;
  CMCount	newOffset;
  unsigned int  amountRead, offset32, len32, value32;
  char          *p;
  omfInt64		zero;

  omfsCvtUInt32toInt64(0, &zero);
  if (maxSize == 0) return (0);

  len = cmGet1ValueSize(theValue);       /* get size of the value we're reading*/
  omfsSubInt64fromInt64(offset, &len);
  if (omfsInt64Equal(len, zero))
  	 return (0);                       /* if offset too far, don't read      */
  if(omfsTruncInt64toUInt32(len, &len32) != OM_ERR_NONE) /* truncate if necessary to maxSize   */
  	omfsCvtUInt32toInt64(maxSize, &len);
   if (len32 > maxSize)               /* truncate if necessary to maxSize   */
  	 len32 = maxSize;

  if (theValue->flags & kCMGlobalName) {          /* read global names specially        */
    p = GetGlobalName(theValue->value.globalName.globalNameSymbol);
    omfsTruncInt64toUInt32(offset, &offset32);	/* !!! Handle errors */
    memcpy(buffer, p + offset32, (size_t)(amountRead = len32));
  } else if (theValue->flags & kCMImmediate) {    /* read immediates specially          */
    omfsTruncInt64toUInt32(theValue->value.imm.value, &value32);
    p = (char *)&value32;
    omfsTruncInt64toUInt32(offset, &offset32);	/* !!! Handle errors */
    memcpy(buffer, p + offset32, (size_t)(amountRead = len32));
  } else {                                        /* non-immediate data                 */
    newOffset = theValue->value.notImm.value;
    omfsAddInt64toInt64(offset, &newOffset);
   	CMfseek(container, newOffset, kCMSeekSet);
    amountRead = CMfread(container, buffer, sizeof(unsigned char), len32);
  }

  return (amountRead);                            /* return amount we read              */
}


/*---------------------------------------------------------------------*
 | cmOverwrite1ValueData - overwrite data for a single value (segment) |
 *---------------------------------------------------------------------*

 This routine copies the data for the specified value (NOT a value header -- continued
 values are not worried about here), starting at the specified offset, FROM the caller's
 buffer. A maximum of size characters are overwritten to the value.  The function returns
 the amount of data copied from the buffer to the value.

 Note, overwriting of global names is NOT allowed and ASSUMED not passed to this routine.
 Immediates are, however, handled and can be overwritten up to their existing size.

 The reason we don't allow writing to global names is that they are kept in a binary tree
 symbol table as a function of the names. If you could change them you would foul up the
 binary tree search.  I don't feel like figuring out how to allow this case so screw it!

 One more thing.  The offset to the next free position in the container is NOT updated
 here. Nor should it be.  We are overwriting existing, i.e., already written data.
*/

CMSize32 cmOverwrite1ValueData(TOCValuePtr theValue, unsigned char *buffer,
                                    CMCount offset, CMSize32 size)
{
  ContainerPtr  container = theValue->container->updatingContainer;
  unsigned int  amountWritten, len, dataVal;
  CMSize		bigLen;

  if (size == 0) return (0);

  bigLen = cmGet1ValueSize(theValue);
  omfsSubInt64fromInt64(offset, &bigLen);
  omfsTruncInt64toUInt32(bigLen, &len);       /* get size of the value we're writing*/
  if (len == 0) return (0);                       /* if offset too far, don't write     */
  if (len > size) len = size;                     /* truncate if necessary to size      */

  if (theValue->flags & kCMImmediate) {           /* overwrite immediates specially     */
    amountWritten = len;
    dataVal = 0;
    memcpy(&dataVal, buffer, size);
    omfsCvtUInt32toInt64(dataVal, &theValue->value.imm.value);
  } else {                                        /* non-immediate data                 */
  	omfsAddInt64toInt64(theValue->value.notImm.value, &offset);
    CMfseek(container, offset, kCMSeekSet);
    amountWritten = CMfwrite(container, buffer, sizeof(unsigned char), len);
    if (amountWritten != len) {
      ERROR1(container,CM_err_BadWrite, CONTAINERNAME);
      return (0);
    }
  }

  return (amountWritten);                         /* return amount actually overwritten */
}


/*-----------------------------------------------------------------*
 | cmConvertImmediate - convert a immediate value to non-immediate |
 *-----------------------------------------------------------------*

 This routine is called whenever an immediate data value must be converted to a
 non-immediate.  It takes as parameters the pointer to the immediate value and returns
 true if there are no errors.  On return the value will have been converted to a
 non-immediate and the input segment CMValue changed to container the offset to the
 now written value.
*/

Boolean cmConvertImmediate(TOCValuePtr theValue)
{
  ContainerPtr   container;
  TOCValueHdrPtr theValueHdr = theValue->theValueHdr;
  CMCount  offset0, offset;
  unsigned int   valueSize, actualSize, oldVal32;
  int            fillBytes;
  omfInt32       pastBoundary;
  omfInt64		zero, largeValueSize, largeActualSize;

  omfsCvtUInt32toInt64(0, &zero);
  container = theValue->container->updatingContainer; /* get container for the value... */
  largeValueSize = CMGetValueSize((CMValue)theValueHdr);   /* ...get current size of value...*/
  omfsTruncInt64toUInt32(largeValueSize, &valueSize); 	/* !!! Handle error */

  /* Reuse free space if we're allowed and there is some.  Otherwise just write to the  */
  /* end of the container.                                                              */

  offset0 = cmGetFreeListEntry(container, largeValueSize, true, &largeActualSize);
  omfsTruncInt64toUInt32(largeActualSize, &actualSize); 	/* !!! Handle error */
  if (actualSize != 0)
    CMfseek(container, offset0, kCMSeekSet);        /* reuse free space                 */
  else {
    offset0 = CMgetContainerSize(container);        /* get current size of container    */
    CMfseek(container, zero, kCMSeekEnd);              /* write data to end of container   */

    /* align new values according to container constraints */

	omfsDivideInt64byInt32(offset0, container->valueAlignment, NULL, &pastBoundary);
	if ((container->valueAlignment != 1) && (pastBoundary != 0))
	   {

	   /* if pastBoundary is the remainder of the division, then the number of bytes to */
	   /* reach the next boundary (fillBytes) is the difference between the boundary    */
	   /* factor and pastBoundary */

	   unsigned char  zero[16];
      unsigned int  i,overBytes;
	   unsigned char* ZERO = NULL;

	   overBytes = pastBoundary;
	   fillBytes = container->valueAlignment - overBytes;
	   if ( fillBytes <= 16 )
	   		ZERO = zero;
	   else
	     	ZERO = (unsigned char *)CMmalloc( container, fillBytes );

		for (i = 0; i< (omfUInt32)fillBytes; i++)
			ZERO[i] = 0;

	   if (CMfwrite(container, (void *)ZERO, 1, fillBytes) != (unsigned int ) fillBytes) {
	        ERROR1(container,CM_err_BadWrite, CONTAINERNAME);
	        if (fillBytes > 16) CMfree(container, ZERO);
	        return(false);
	   }
	   if (fillBytes > 16) CMfree(container, ZERO);
	   omfsAddInt32toInt64(fillBytes, &offset0);
    }
  }

  omfsTruncInt64toUInt32(theValue->value.imm.value, &oldVal32);
  if (CMfwrite(container, &oldVal32, sizeof(unsigned char), valueSize) != valueSize) {
    ERROR1(container,CM_err_BadWrite, CONTAINERNAME);
    return (false);
  }

  /* Redefine the value to be non-immediate...                                          */

  (void)cmSetValueBytes(container, &theValue->value, Value_NotImm, offset0, largeValueSize);
  theValue->flags = 0;
  theValueHdr->valueFlags = ValueDefined;

  offset = offset0;
  omfsAddInt32toInt64(valueSize, &offset);          /* update logical OR physical EOF   */
  if (actualSize == 0)
    container->physicalEOF = offset;                /* update next free container byte  */
  SetLogicalEOF(offset);                            /* set logical EOF (may != physical)*/

  theValue->container = container;                  /* owner is now updating container  */

  return (true);
}


/*--------------------------------------------------------------------------------------*
 | cmInsertNewSegment - create and insert a new value segment to into an existing value |
 *--------------------------------------------------------------------------------------*

 This routine is called by CMInsertValueData() to create a new value segment insert to be
 inserted into a previously existing value.  It is also called when applying data insert
 updates at open time (by the internal routine applyValueUpdateInstructions() in
  Update.c ).  The new insert data is already written to the (updating) container, and the
 data is at container offset dataOffset.  It is size bytes int .  The insert is to be at
 the specified segOffset within the value segment insBeforeValue.

 The function returns a pointer to the newly created insert segment, or NULL if there is
 an error and the error reporter returns.

 Data inserts fall into three cases:

   (1). Inserting into an immediate.  If the size of the immediate plus the size of the
        insert still fits in a immediate the result will remain an immediate.  Otherwise,
        the immediate isc onverted to a non-immediate and we have cases (2) or (3).
        Immediates are handled differently for updating and like normal insertions
        processed separately.

   (2). Inserting into the START of an existing non-immediate value segment (segOffset
        offset 0).  The insert is made a segment and placed before the existing segment
        (insBeforeValue).

   (3). Inserting into the MIDDLE of a non-immediate value segment.  The existing segment
        must be split into two segments and the insert segment placed between them.

 CMInsertValueData() handles case (1) and calls this routine for cases (2) and (3) after
 it determines it has these cases and after it has written the new insert data to the
 container.

 Updating treats immediates separately and thus, like CMInsertValueData(), only cases (2)
 and (3) are possible here.  Indeed, it is because of updating that this routine exists
 at all!   If we didn't have updating, this code could live directly in
 CMInsertValueData().  In CMInsertValueData() the caller explicitly passes the data which
 CMInsertValueData() must write out to the container.  However, for updating, we already
 have the data written in the updating container.  Both cases thus merge here to create
 the segment for that data and insert it into the segment list in the appropriate place.
*/

TOCValuePtr cmInsertNewSegment(TOCValueHdrPtr theValueHdr, TOCValuePtr insBeforeValue,
                               CMCount segOffset, CMCount dataOffset,
                               CMSize size)
{
  ContainerPtr  container = theValueHdr->container->updatingContainer;
  TOCValuePtr   theLeftValue, theInsertValue;
  TOCValueBytes valueBytes;
  omfInt64		zero, one;
  CMCount		newOff;
  CMSize		newLen;

  omfsCvtUInt32toInt64(0, &zero);
  omfsCvtUInt32toInt64(1, &one);
  (void)cmSetValueBytes(container, &valueBytes, Value_NotImm, dataOffset, size);
  theInsertValue = cmCreateValueSegment(theValueHdr, &valueBytes, 0);
  if (theInsertValue == NULL) return (NULL);        /* if insert failed, abort now      */

  theInsertValue->logicalOffset = insBeforeValue->logicalOffset;
  omfsAddInt64toInt64(segOffset, &theInsertValue->logicalOffset);

  if (omfsInt64Greater(segOffset, zero)) {          /* split the segment to be inserted */
    theLeftValue = insBeforeValue;                  /* use original seg for left portion*/
    newOff = insBeforeValue->value.notImm.value;
    omfsAddInt64toInt64(segOffset, &newOff);
    newLen = insBeforeValue->value.notImm.valueLen;
    omfsSubInt64fromInt64(segOffset, &newLen);
    (void)cmSetValueBytes(container, &valueBytes, Value_NotImm,            /*right value*/
                          newOff,  					 /*fix offset */
                          newLen);					/* ...& len  */
    insBeforeValue = cmCreateValueSegment(theValueHdr,/* create right portion of insert */
                                          &valueBytes,/* with adjusted offset length    */
                                          theLeftValue->flags);
    if (insBeforeValue == NULL) return (NULL);      /* something went wrong?            */
    insBeforeValue->logicalOffset = theInsertValue->logicalOffset; /* adjust logical offset */
    insBeforeValue->container = theLeftValue->container;/* container is still the same  */
    theLeftValue->flags |= kCMContinued;            /* flag the left portion as cont'd  */
    theValueHdr->valueFlags |= ValueContinued;      /* echo flags in the header         */
    theLeftValue->value.notImm.valueLen = segOffset;/* set size of left portion         */

    /* Insert right portion after the left portion. The insBeforeValue pointer will now */
    /* be pointing at the segment we want to insert the new segment before.             */

    (void)cmInsertAfterListCell(&theValueHdr->valueList, insBeforeValue, theLeftValue);
  } /* splitting segment */

  /* Now cases (2) and (3) come back together.  We insert the new segment in front of   */
  /* the right segment pointed to by insBeforeValue.  That will make it a continued     */
  /* value.                                                                             */

  (void)cmInsertBeforeListCell(&theValueHdr->valueList, theInsertValue, insBeforeValue);
  theInsertValue->flags = (unsigned short)(insBeforeValue->flags | kCMContinued);
  theValueHdr->valueFlags |= ValueContinued;                  /* echo flags in the hdr  */
  omfsAddInt64toInt64(theInsertValue->value.notImm.valueLen, &theValueHdr->size); /* add to total value len */

  return (theInsertValue);                          /* give back newly inserted segment */
}


/*-----------------------------------------------------------------*
 | cmDeleteSegmentData - delete data represented by value segments |
 *-----------------------------------------------------------------*

 This routine is called by CMDeleteValueData() to delete value data  It is also called
 when applying data deletion updates at open time (by the internal routine
 applyValueUpdateInstructions() in  Update.c ).  All the data starting at offset
 startOffset, up to and including the end offset, endOffset, are to be deleted.  If the
 start and end offsets span entire value segments, those segments are removed.

 Note, this routine is for non-immediate data deletions only.  CMDeleteValueData() handles
 deletions on immediates.  For updating, immediates are handled differently and thus can't
 get in here.

 The start and end may map into the middle of segments.  This means that deleions of
 partial segments is possible.  Both the start and end offsets may map into a single
 segment.  In that case the segment must be split into twp segments.  The comments in the
 code explain these cases.
*/

void cmDeleteSegmentData(TOCValueHdrPtr theValueHdr, CMCount startOffset,
                                                     CMCount endOffset)
{
  TOCValuePtr    theStartValue, theEndValue, thePrevValue, theNextValue, theValue,
                 theRtValue, thePrevValue1;
  ContainerPtr   container = theValueHdr->container->updatingContainer;
  CMCount  offset1, offset2, segMaxOffset, amountDeleted, totalDeleted, holeOffset;
  CMCount		newOff;
  CMSize		newLen;
  TOCValueBytes  valueBytes;
  omfInt64		zero, one;

  omfsCvtUInt32toInt64(0, &zero);
  omfsCvtUInt32toInt64(1, &one);

  /* Map the start and end offsets into the offsets within their respective segments... */

  theStartValue  = cmGetStartingValue(theValueHdr, startOffset, &startOffset);
  theEndValue    = cmGetStartingValue(theValueHdr, endOffset,   &endOffset);

  /* We now have the start and end value segments and offsets within them.  Everything  */
  /* between is to be deleted.  We will loop through the segments starting with         */
  /* theStartValue and up to theEndValue.  The startOffset and endOffset will be used   */
  /* to delete the partial first and last segment portions.  Remember all this stuff    */
  /* could be for a single segment or multiple segments.  All the "interior" segments   */
  /* for a multi-segment delete are removed.  The partial segment cases take a little   */
  /* more work.                                                                         */

  thePrevValue = NULL;                              /* this is segment just processed   */
  theValue     = theStartValue;                     /* this is the segment to process   */
  totalDeleted = zero;                                 /* sum total amount deleted         */

  while (thePrevValue != theEndValue) {             /* loop through the segments...     */
    theNextValue = (TOCValuePtr)cmGetNextListCell(theValue);/* next now for seg delete  */
    thePrevValue = theValue;                        /* remember segment we're processing*/

    segMaxOffset = theValue->value.notImm.valueLen;
    omfsSubInt64fromInt64(one, &segMaxOffset);     /* get segments max offset  */
    offset1 = (theValue == theStartValue) ? startOffset : zero;/* get segment offsets...   */
    offset2 = (theValue == theEndValue  ) ? endOffset   : segMaxOffset;

    /* There are four cases depending on offset1 and offset2, i.e., the offsets within  */
    /* the current segment we're dealing with each time around this loop:               */

    /*  (1). The offsets specify the entire segment is to be deleted. [///////]         */
    /*  (2). offset1 == 0 ==> delete left portion of segment.         [////   ]         */
    /*  (3). offset2 == segMaxOffset == delete right portion.         [   ////]         */
    /*  (4)  if not (1), (2), (3) delete interior portion.            [  ///  ]         */

    /* The last case requires us to split the segment into two segments which means we  */
    /* must "manufacture" an additional segment.                                        */

    if (omfsInt64Equal(offset1, zero) && omfsInt64Equal(offset2, segMaxOffset)) {  /* delete entire segment...         */
      if (cmCountListCells(&theValueHdr->valueList) == 1) { /* if value going null...   */
        cmAddToFreeList(container, theValue, zero, zero); /* add freed space to free list     */
        omfsCvtUInt32toInt64(0, &theValue->value.notImm.value);        /* ...define remaining (only) seg   */
        omfsCvtUInt32toInt64(0, &theValue->value.notImm.valueLen);        /*    as a null value               */
        theValue->flags = kCMImmediate;             /*    a null immediate value        */
        theValueHdr->valueFlags |= ValueImmediate;  /*    echo flag in the value hdr    */
      } else {                                      /* value not null (yet), delete seg */
        if (cmGetNextListCell(theValue) == NULL) {  /* make sure of seg cont'd bit...   */
          thePrevValue1 = (TOCValuePtr)cmGetPrevListCell(theValue);
          if (thePrevValue1 != NULL)                /* ...no longer cont'd if last seg  */
            thePrevValue1->flags &= ~kCMContinued;  /*    is to be deleted              */
        }
        cmAddToFreeList(container, theValue, zero, zero); /* add freed space to free list     */
        cmDeleteListCell(&theValueHdr->valueList, theValue);
        CMfree(container, theValue);                           /* poof!                            */
      }
      amountDeleted = segMaxOffset;
      omfsAddInt32toInt64(1, &amountDeleted);       /* this is how much we're deleting  */
    } else if (omfsInt64Equal(offset1, zero)) {     /* delete left portion of 1st seg...*/
      amountDeleted = offset2;
      omfsAddInt32toInt64(1, &amountDeleted);       /* this is how much we're deleting  */
      cmAddToFreeList(container, NULL,              /* add freed space to free list     */
                      theValue->value.notImm.value, /* ...this is freed left part       */
                      amountDeleted);
      omfsAddInt64toInt64(amountDeleted, &theValue->value.notImm.value);/* bump up offset                   */
      theValue->value.notImm.valueLen  = segMaxOffset;
      omfsSubInt64fromInt64(offset2, &theValue->value.notImm.valueLen); /* adjust length       */
      omfsAddInt64toInt64(amountDeleted, &theValue->logicalOffset);     /* adjust starting logical offset   */
    } else if (omfsInt64Equal(offset2, segMaxOffset)) {           /* delete rt. portion of last seg...*/
      amountDeleted = segMaxOffset;
      omfsSubInt64fromInt64(offset1, &amountDeleted);
      omfsAddInt32toInt64(1, &amountDeleted);       /* this is how much we're deleting  */
      newOff = theValue->value.notImm.value;
      omfsAddInt64toInt64(offset1, &newOff); 			/* this is freed rt. part */
      cmAddToFreeList(container, NULL,              /* add freed space to free list     */
                      newOff,
                      amountDeleted);
      theValue->value.notImm.valueLen = offset1;
    } else {                                        /* delete interior portion of seg...*/
      newOff = theValue->value.notImm.value;
      omfsAddInt64toInt64(offset2, &newOff);
      omfsAddInt32toInt64(1, &newOff);
      newLen = segMaxOffset;
      omfsSubInt64fromInt64(offset2, &newLen);
      (void)cmSetValueBytes(container, &valueBytes, Value_NotImm,       /* rt. chunk    */
                            newOff, 									/* adjust offset*/
                            newLen);    		                /* ...and length*/
      theRtValue = cmCreateValueSegment(theValueHdr,/* create right portion             */
                                        &valueBytes, theValue->flags);
      if (theRtValue == NULL) return;               /* something went wrong?            */
      theRtValue->logicalOffset = theValue->logicalOffset;
      omfsAddInt64toInt64(offset2, &theRtValue->logicalOffset);
      omfsAddInt32toInt64(1, &theRtValue->logicalOffset); /* fix log. offset */
      theRtValue->container = theValue->container;  /* rt. seg is same container as left*/
      (void)cmInsertAfterListCell(&theValueHdr->valueList, theRtValue, theValue);
      theValue->flags |= kCMContinued;              /* flag the left portion as cont'd  */
      theValue->value.notImm.valueLen = offset1;    /* set size of left portion         */
      amountDeleted = offset2;
      omfsSubInt64fromInt64(offset1, &amountDeleted);
      omfsAddInt32toInt64(1, &amountDeleted);        /* this is how much we're deleting  */
      holeOffset = theValue->value.notImm.value;
      omfsAddInt64toInt64(offset1, &holeOffset);
      cmAddToFreeList(container, NULL,              /* add freed space to free list     */
                      holeOffset,					 /* this is "hole" created */
                      amountDeleted);
    }

    omfsAddInt64toInt64(amountDeleted, &totalDeleted);/* tally total amount we delete     */
    theValue = theNextValue;                        /* point at next segment            */
  } /* while */                                     /* loop...                          */

  omfsSubInt64fromInt64(totalDeleted, &theValueHdr->size); /* update total size in value hdr     */
  if (cmCountListCells(&theValueHdr->valueList) < 2)/* make sure of cont'd bit in hdr   */
    theValueHdr->valueFlags &= ~ValueContinued;
}


 /*---------------------------------------------------------------------*
  | cmMoveValueHdr - move a value (header) to a new position in the TOC |
  *---------------------------------------------------------------------*

 This routine is called by CMMoveValueData() to move a value (header).  It is also called
 when applying "inserted" (move) updates at open time (by the internal routine
 applyValueUpdateInstructions() in  Update.c ).   The value is physically deleted from its
 original object/property as if a CMDeleteValue() were done on it.  If the value deleted
 is the only one for the property, the property itself is deleted as in
 CMDeleteObjectProperty().

 The value is added to the "to"s object propery in a manner similar to a CMNewValue(). The
 order of the values for both the value's original object property and for the value's
 new object property may be changed.

 Note, that although the effect of a move is a combination CMDeleteValue()/CMNewValue(),
 THE INPUT REFNUM REMAINS VALID!  Its association is now with the new object property.

 The input refNum is returned as the function result.  NULL is returned if there is an
 error and the error reporter returns.
*/

TOCValueHdrPtr cmMoveValueHdr(TOCValueHdrPtr theFromValueHdr, CMObject object, CMProperty property)
{
  TOCObjectPtr   theFromObject, theToObject;
  TOCPropertyPtr theFromProperty;
  TOCValueHdrPtr theToValueHdr, prev, next;
  ContainerPtr   container = theFromValueHdr->container;

  /* We will need pointers to the the "from" object and its property...                 */

  theFromProperty = theFromValueHdr->theProperty;
  theFromObject   = theFromProperty->theObject;

  /* For the hell of it see if the "from" and "to" represent the SAME place.  There's   */
  /* no sense going on with this turkey if they are!                                    */

  if (theFromObject == (TOCObjectPtr)object)
    if (cmFindObject(container->toc, theFromProperty->propertyID) == (TOCObjectPtr)property)
      return (theFromValueHdr);                         /* incredible, ain't it?        */

  /* Now that we got all of that crap out of the way we can proceed. The easiest way to */
  /* do the move is to create a dummy value entry in the "to" object property chain and */
  /* then do list manipulations on it to move the "from" value header and its value     */
  /* chain.  We then must make sure to free the dummy we just created.  Doing it this   */
  /* way allows us to use the mechanisms already in place without providing special     */
  /* case code or routines.  For example, we don't have to worry about creating the     */
  /* property at the destination if it does not happen to previously exist.             */

  /* The first thing we do is remove the "from" value header from its property list.    */
  /* This does not free the space.  It only jumps it out of the links.                  */

  cmDeleteListCell(&theFromValueHdr->theProperty->valueHdrList, theFromValueHdr);
  prev = (TOCValueHdrPtr)cmGetPrevListCell(theFromValueHdr); /* save for error recovery */
  next = (TOCValueHdrPtr)cmGetNextListCell(theFromValueHdr);
  cmNullListLinks(theFromValueHdr);

  /* Now we can create the dummy "to" value header place holder.  For lack of anything  */
  /* better, we give it the same flags, etc. from the "from" value.  By giving it the   */
  /* same type, we automatically get the check for duplicate types in the "to" property.*/

  theToObject = cmDefineObject(container, ((TOCObjectPtr)object)->objectID,
                               ((TOCObjectPtr)property)->objectID,
                               theFromValueHdr->typeID,   /* error if dup               */
                               NULL, theFromValueHdr->generation, 0,
                               theFromObject->objectFlags,
                               &theToValueHdr);

  if (theToObject == NULL) {                              /* if something is wrong...   */
    if (prev != NULL)                                     /* put back original "from"   */
      cmInsertAfterListCell(&theFromValueHdr->theProperty->valueHdrList, theFromValueHdr, prev);
    else if (next != NULL)
      cmInsertBeforeListCell(&theFromValueHdr->theProperty->valueHdrList, theFromValueHdr, next);
    else
      cmAppendListCell(&theFromValueHdr->theProperty->valueHdrList, theFromValueHdr);
    return (NULL);
  }

  /* Now we want to replace the value header we just created with the "from" value      */
  /* header (which is now deleted off the "from" property list and only pointed to by   */
  /* theFromValueHdr).  That will carry along all the value segments on the value       */
  /* header's chain and all the proper field settings in the value header as well.      */

  /* We play some list manipulation games to move the "from" value header to the "to"   */
  /* list.  We know where the "to" dummy header was placed, theToValueHdr. So all we    */
  /* have to do is insert the "from" value header before the dummy and then delete the  */
  /* dummy.  We also now have to set the owning property field to point to the owning   */
  /* property on the new list.                                                          */

  cmInsertBeforeListCell(&theToValueHdr->theProperty->valueHdrList, theFromValueHdr, theToValueHdr);
  theFromValueHdr->theProperty = theToValueHdr->theProperty;
  cmDeleteListCell(&theToValueHdr->theProperty->valueHdrList, theToValueHdr);
  CMfree(container, theToValueHdr);                                    /* done with the dummy      */

  /* That was easy!  Now we look at the property for "from" value header and delete it  */
  /* if there are no more values.                                                       */

  if (cmIsEmptyList(&theFromProperty->valueHdrList))
    CMfree(container, cmDeleteListCell(&theFromObject->propertyList, theFromProperty));

  return (theFromValueHdr);                                 /* returned move refNum     */
}

                              CM_END_CFUNCTIONS
