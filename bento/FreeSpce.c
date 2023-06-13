/*---------------------------------------------------------------------------*
 |                                                                           |
 |                            <<< FreeSpce.c  >>>                            |
 |                                                                           |
 |          Container Manager Free Space Value Management Routines           |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 4/23/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file contains routines for reusing deleted data space.  The routines record the
 deleted space and reuse it for value data writes.

 The free space is maintained as a list of value segment entries for a "free space"
 property belonging to the TOC ID 1 entry.  It is written to the container like other ID 1
 properties to keep track of all the deleted space.

 If a container is opened for reusing free space, we use the free list to reuse the space.
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
#ifndef __TOCENTRIES__
#include "TOCEnts.h"
#endif
#ifndef __TOCOBJECTS__
#include "TOCObjs.h"
#endif
#ifndef __TOCIO__
#include "TOCIO.h"
#endif
#ifndef __GLOBALNAMES__
#include "GlbNames.h"
#endif
#ifndef __CONTAINEROPS__
#include "Containr.h"
#endif
#ifndef __HANDLERS__
#include "Handlers.h"
#endif
#ifndef __SESSIONDATA__
#include "Session.h"
#endif
#ifndef __FREESPACE__
#include "FreeSpce.h"
#endif

                                  CM_CFUNCTIONS

/* The following generates a segment directive for Mac only due to 32K Jump Table       */
/* Limitations.  If you don't know what I'm talking about don't worry about it.  The    */
/* default is not to generate the pragma.  Theoritically unknown pragmas in ANSI won't  */
/* choke compilers that don't recognize them.  Besides why would they be looked at if   */
/* it's being conditionally skipped over anyway?  I know, famous last words!            */

#if CM_MPW
#pragma segment TOCEntries
#endif


/*------------------------------------------------------------*
 | cmAddToFreeList - add freed space to the "free space" list |
 *------------------------------------------------------------*

 This routine is called whenever space for value data is to be freed.  The space is
 recorded in a free list. The free list entries are maintained as value segments belonging
 to the "free space" property of TOC object ID 1.

 The amount of space to be freed is specified in one of two possible ways:

    1. By passing a pointer to a value segment in theValueToFree.  The offset and size
       are extracted from the segment info.  If theValueToFree is for an immediate value,
       we simply exit since there is no container space to actually free.  Global names,
       as usual, are handled specially.  In particular, for no global names that have not
       yet been written to the container yet (we keep 'em in memory unto we write the TOC
       at container close time), then we ingore them just like immediates.  For "old"
       global names we do account for the space.

    2. If theValueToFree is passed as NULL, then an explicit size and offset may be
       passed.  Note that a non-null theValueToFree has precedence over explicit offset
       and size.

  As part of the freeing process, we scan the free list to see if the new space can be
  combined with an already existing entry.  It's a sort of on-the-fly garbage collector.

  For new containers, there is no "free space" property initially for ID 1.  It is created
  here the first time we need to free space.  We remember the value header for the "free
  space" property so that we may efficiently get at the list on all future calls.
*/

void cmAddToFreeList(ContainerPtr container, TOCValuePtr theValueToFree,
                     CMCount offset, CMSize size)
{
#ifdef REUSE_ENABLED
  TOCObjectPtr   theTOCobject;
  TOCValueHdrPtr freeSpaceValueHdr = container->freeSpaceValueHdr;
  TOCValuePtr    theValue;
  unsigned int   valueStart, valueEndPlus1;
  TOCValueBytes  valueBytes;
  void           *toc;
  omfInt64		zero;

  omfsCvtUInt32toInt64(0, &zero);
  /* We only track free space when a container is opened for writing or updating.       */
  /* However, even when writing there is an override switch to suppress the space       */
  /* tracking.  This is done during applying updates during open time.  We don't want   */
  /* to track what's going on then!                                                     */

  if (!container->trackFreeSpace ||               /* if no tracking of free space or... */
      (container->useFlags & kCMWriting) == 0)    /* ...not opened for writing/updating */
    return;                                       /* ...exit                            */

  /* Get the offset and size from the value if it's specified. This is the amount being */
  /* freed.  If theValueToFree is NULL, then the offset and size must be explicitly     */
  /* specified, otherwise these parameters are ignored.  As a safety, if the value is   */
  /* passed, but it is for an immediate value, there is no space being given up, so we  */
  /* just exit.                                                                         */

  if (theValueToFree != NULL) {                   /* if value explicitly specified...   */
    if (theValueToFree->flags & kCMGlobalName) {  /* ...handle global names specially   */
      offset = theValueToFree->value.globalName.offset;
      if (omfsInt64Equal(offset, zero)) return;
      omfsCvtUInt32toInt64((GetGlobalNameLength(theValueToFree->value.globalName.globalNameSymbol)
      						 + 1), &size);
    } else if (theValueToFree->flags & kCMImmediate)
      return;                                     /* ...ignore immediates...            */
    else {                                        /* ...have "normal" value             */
      offset = theValueToFree->value.notImm.value;
      size   = theValueToFree->value.notImm.valueLen;
    }
  }

  /* Always record the total amount of free space for the TOC ID 1 "total free space"   */
  /* property...                                                                        */

  container->spaceDeletedValue->value.imm.ulongValue += size;

  /* See if the space we're freeing can be combined with some already existing free     */
  /* space.  If the new space is already freed (now how could that happen?) we ignore   */
  /* the new space.  If the chunk of space we are about to free is less than the size   */
  /* of a TOC entry, we ignore it (losing track of that space), since it would cost more*/
  /* to remember it.  However, if that space can be combined with already existing      */
  /* space, we do the combining.                                                        */

  if (freeSpaceValueHdr != NULL) {                /* if free space list exists...       */
    theValue = (TOCValuePtr)cmGetListHead(&freeSpaceValueHdr->valueList);

    while (theValue) {                            /* scan the free space list...        */
      valueStart    = theValue->value.imm.ulongValue;
      valueEndPlus1 = valueStart + theValue->value.notImm.valueLen;
      if (offset >= valueStart && offset <= valueEndPlus1) { /* have overlap or concat    */
        if (offset + size > valueEndPlus1) {      /* combine new amount with this entry */
          size = (offset + size) - valueEndPlus1; /* reduce actual size to add in       */
          theValue->value.notImm.valueLen += size;/* this is what we combine in         */
          freeSpaceValueHdr->size += size;        /* echo total size in header          */
          container->spaceDeletedValue->value.imm.ulongValue += size; /* and container    */
        }
        return;                                   /* exit since we updated "old" entry  */
      }
      theValue = (TOCValuePtr)cmGetNextListCell(theValue);
    }
  } /* checking for combining */

  /* Since the cost of recording the free space in the container will be a TOC entry, we*/
  /* only record free space chucks whose size is LARGER than a TOC entry.  Since we     */
  /* couldn't combine it with already existing space, we simply exit and forget it.     */

  #if TOC1_SUPPORT
  if (container->majorVersion == 1) {
    if (size <= TOCentrySize) return;
  } else
    if (size <= MinTOCSize) return;
  #else
  if (size <= MinTOCSize) return;
  #endif

  /* If this is the first freed space in the container, create the "free space" property*/
  /* for the TOC object 1.  Otherwise, just use it. We remember the pointer to the value*/
  /* header for this property in the container.  Note, the TOC we want to deal with here*/
  /* is the container's own (private) TOC.  This will be different from the current TOC */
  /* when we're updating containers.                                                    */

  if (freeSpaceValueHdr == NULL) {                /* if 1st free space for container... */
    toc = container->toc;                         /* ...remember current TOC            */
    container->toc = container->privateTOC;       /* use container's own (private) TOC  */

    theTOCobject = cmDefineObject(container,      /* ...create "free space" property    */
                                  CM_StdObjID_TOC, CM_StdObjID_TOC_Free,
                                  CM_StdObjID_TOC_Type, NULL,
                                  container->generation, 0,
                                  (ObjectObject | ProtectedObject),
                                  &freeSpaceValueHdr);

    container->toc = toc;                         /* restore current container          */
    if (theTOCobject == NULL) return;
    freeSpaceValueHdr->valueFlags |= ValueProtected;/* don't allow writing to this value*/
    container->freeSpaceValueHdr = freeSpaceValueHdr;
  }

  /* At this point we want to create a new free list entry for the newly freed offset   */
  /* size.  The value list are standard value entries for the "free space" property of  */
  /* TOC object ID 1.                                                                   */

  theValue = (TOCValuePtr)cmGetListTail(&freeSpaceValueHdr->valueList);
  if (theValue) {
    theValue->flags |= kCMContinued;                /* flag the current value as cont'd */
    freeSpaceValueHdr->valueFlags |= ValueContinued;/* also set a more convenient flag  */
  }

  (void)cmSetValueBytes(container, &valueBytes, Value_NotImm, offset, size);
  cmAppendValue(freeSpaceValueHdr, &valueBytes, 0);

  container->spaceDeletedValue->value.imm.ulongValue += size;
#endif
}


/*------------------------------------------------*
 | deleteFreeListEntry - delete a free list entry |
 *------------------------------------------------*

 This routine removes the specified free list entry and frees up its space.  If this is
 the last entry of the free list, the free list property itself (along with its value
 header, of course) are removed.

 Note, this routine is a low-level routine called from the other routines in this file.
 As such there are no error checks.  The caller is assumed "happy" with what its doing by
 the time it calls this routine.
*/

static void CM_NEAR deleteFreeListEntry(TOCValuePtr theValue)
{
  ContainerPtr   container = theValue->theValueHdr->container;
  TOCObjectPtr   theTOCObject;
  TOCPropertyPtr theFreeListProperty;
  TOCValueHdrPtr freeSpaceValueHdr = container->freeSpaceValueHdr;

  /* Delete the value entry from its list and free its space...                         */

  CMfree(container, cmDeleteListCell(&freeSpaceValueHdr->valueList, theValue));

  /* If there are no more free list entries, delete the value header and the "free      */
  /* space" property from TOC ID 1...                                                   */

  if (cmIsEmptyList(&freeSpaceValueHdr->valueList)) {       /* if no more free list...  */
    theFreeListProperty = freeSpaceValueHdr->theProperty;   /* ...get owning property   */
    theTOCObject        = theFreeListProperty->theObject;   /* ...get owning object (1) */
    CMfree(container, freeSpaceValueHdr);                              /* ...clobber the value hdr */
    CMfree(container, cmDeleteListCell(&theTOCObject->propertyList, theFreeListProperty));
    container->freeSpaceValueHdr = NULL;                    /* ...no more free list     */
  }
}


/*--------------------------------------------------------------*
 | cmGetFreeListEntry - get a free space entry from "free list" |
 *--------------------------------------------------------------*

 This routine returns one free list entry (if one exists).  The offset from that entry is
 returned as the function result.  The size is returned in actualSize.  If there is no
 free space, or we can't find one big enough (see mustAllFit below), 0 is returned as the
 function result and for the actualSize.  0's are also returned if the container was not
 opened to reuse free space.

 The desiredSize is passed as what the caller would like as the single free list entry
 amount, i.e., a "best fit".  It is also used when we find a bigger free list entry and
 only have to reduce part of it.

 If mustAllFit is true, then we MUST find a single free list segment big enough or 0 will
 be returned.  This is used for data that is not allowed to be continued with multiple
 value segments (e.g., global names).

 The free list is built by cmAddToFreeList() as value segments belonging to a value header
 of the "free space" property for TOC object ID 1.   If all the free list entries are
 exhausted, the value header is freed along with the property.  If space is ever given up
 after this through cmAddToFreeList(), it will recreate the "free space" property, its
 value header, and the free list value entry segments.
*/

CMSize cmGetFreeListEntry(ContainerPtr container,
                                 CMSize desiredSize, Boolean mustAllFit,
                                 CMSize *actualSize)
{
  TOCValueHdrPtr freeSpaceValueHdr = container->freeSpaceValueHdr;
  omfInt64		zero, one;

  omfsCvtUInt32toInt64(0, &zero);
  omfsCvtUInt32toInt64(1, &one);

  /* Return null amount if there is no free list or not updating...                     */

#ifdef REUSE_ENABLED
  if (freeSpaceValueHdr == NULL || (container->useFlags & kCMReuseFreeSpace) == 0)
#endif
	{
	*actualSize = zero;
    return (zero);
    }

#ifdef REUSE_ENABLED
  /* Use the next available free list entry (segment) no matter what it is unless we    */
  /* mustAllFit.  For mustAllFit we must scann the free list for a single entry large   */
  /* enough to hold the entire data (desiredSize bytes).                                */

  /* Note as just said, when mustAllFit is false, we just use the first free list       */
  /* segment on the list.  A better algorithm might be to use the desired amount and    */
  /* find a "best fit" by scanning the free list just as in the mustAllFit case.  So    */
  /* which is better in time and/or space?  A scan each time or just reusing each entry */
  /* unconditionally.  The latter guarantees reuse of all available space.  But the     */
  /* former potentially uses less value segments.  For now I take the easy way.         */

  /* Note we still need it to split larger free space entries when we don't need all    */
  /* the space from an entry.                                                           */

  theValue = (TOCValuePtr)cmGetListHead(&freeSpaceValueHdr->valueList);

  if (mustAllFit) {                                         /* if must find single seg  */
    while (theValue) {                                      /* ...scan free list        */
      if (theValue->value.notImm.valueLen >= desiredSize)   /* ...for one big enough    */
        break;
      theValue = (TOCValuePtr)cmGetNextListCell(theValue);
    }
    if (theValue == NULL) return (*actualSize = 0);         /* if none found, too bad   */
  }

  offset      = theValue->value.notImm.value;               /* set to return this offset*/
  *actualSize = theValue->value.notImm.valueLen;            /* and this size (maybe!)   */

  /* If the free list entry is big enough to cover all that is desired, use only the    */
  /* required amount and retain the free list entry.  It must, however, be reduced by   */
  /* the amount used.                                                                   */

  /* UNRESOLVED QUESTION! Since we never allow free list entries to be created when the */
  /* space is less than a TOC entry size, should we keep the split space here when it   */
  /* becomes less than a TOC entry size?  What does it cost?                            */

  if (desiredSize < *actualSize) {                          /* free entry is too big    */
    theValue->value.notImm.value    += desiredSize;         /* ...cut it down           */
    theValue->value.notImm.valueLen -= desiredSize;
    container->spaceDeletedValue->value.imm.ulongValue -= desiredSize; /* cut total     */
    *actualSize = desiredSize;                              /* return what was desired  */
    return (offset);
  }

  /* Now that we used an entire an free list entry, delete it from the free list and    */
  /* free its memory.  If there are no more free list entries, delete the value header  */
  /* and the "free space" property from TOC ID 1...                                     */

  deleteFreeListEntry(theValue);

  container->spaceDeletedValue->value.imm.ulongValue -= *actualSize; /* cut total       */

  return (offset);                                          /* return new space to use  */

#endif
}


/*--------------------------------------------------------------------------*
 | cmWriteData - write data to container with possible resuse of free space |
 *--------------------------------------------------------------------------*

 This routine is the Container Manager's low-level value data writer.  It takes a value
 header, and size bytes in the buffer and writes it to the container via the output
 handler.  The amount written is returned.  If it is not equal to the passed size, the
 caller should report an error.

 The reason this routine is used rather than using the CMfwrite() handler call directly is
 that this routine checks to see if the container has been opened to reuse free space.  If
 it hasn't, we degenerate into a simple CMfwrite() call.  If it has, then we use the free
 list, built by cmAddToFreeList(), to write out the data segments to reuse the free space.

 In all cases the data written is recorded as value segments in the value list belonging
 to the specified value header. The new segments are appended on to the END of the segment
 list.  It is up to the caller to guarantee this is where the segments are to go.  The
 continued flags are appropriately set.

 Note, as just mentioned, use this routine ONLY for appending data segments to a value.
 Also, use this routine ONLY if continued segments are permitted for the data.  This means
 you cannot call this routine to write global names.  They are always written as single
 segments.

 Even with these restrictions, for the main case, i.e., CMWriteValueData(), this is not a
 problem.  CMWriteValueData() is the primary caller.  It knows what it's doing with the
 data (lets hope so).  So it knows when to call us here.

 The net effect is to reuse free space whenever we can if the container was opened for
 updating, and to do straight CMfwrite() otherwise.
*/

CMSize32 cmWriteData(TOCValueHdrPtr theValueHdr, unsigned char *buffer, CMSize32 size)
{
  ContainerPtr  container = theValueHdr->container->updatingContainer;
  TOCValuePtr   prevValue;
#ifdef REUSE_ENABLED
  unsigned int  chunkSize;
#endif
  unsigned int  amountWritten = 0;
  CMCount	offset;
  TOCValueBytes valueBytes;
  omfInt64		zero, longSize;

  omfsCvtUInt32toInt64(0, &zero);
  omfsCvtUInt32toInt64(size, &longSize);

  /* Write out all the data (size bytes of it).  To save a little code space we check   */
  /* for updating in the loop.  If we are not updating we will attempt to do a standard */
  /* CMfwrite() of all the data as a single value segment.  That is also what happens   */
  /* when we run out of free space.  Then we try to write out all the remaining data    */
  /* as a single segment.  The code is the same, so that's why the update check is in   */
  /* the loop.  If we are updating, and we do have free list entries, we use them to    */
  /* see where to write and how much.  This will reuse the free list space.             */

  prevValue = (TOCValuePtr)cmGetListTail(&theValueHdr->valueList); /* last value seg    */

#ifdef REUSE_ENABLED
  while (size > 0) {                                  /* loop till no more data         */
    if ((container->useFlags & kCMReuseFreeSpace)==0) /* if not reusing free space...   */
      chunkSize = 0;                                  /* ...this will cause std write   */
    else                                              /* if updating,get some free space*/
      offset = cmGetFreeListEntry(container, size, false, &chunkSize);

    if (chunkSize == 0) {                             /* if no free space to reuse...   */
#endif
      offset = CMgetContainerSize(container);
      CMfseek(container, zero, kCMSeekEnd);              /* position to current eof        */
      if (CMfwrite(container, buffer, sizeof(unsigned char), size) == size) { /* write  */
        amountWritten += size;                        /* count total amount written     */
        container->physicalEOF = offset;
        omfsAddInt64toInt64(longSize, &container->physicalEOF);       /* update next free container byte*/
        SetLogicalEOF(container->physicalEOF);        /* logical EOF == physical EOF    */
        (void)cmSetValueBytes(container, &valueBytes, Value_NotImm, offset, longSize);
        cmAppendValue(theValueHdr, &valueBytes, 0);   /* append new value segment       */
        if (prevValue != NULL) {                      /* if this is not 1st segment...  */
          prevValue->flags |= kCMContinued;           /* ...flag last seg as cont'd     */
          theValueHdr->valueFlags |= ValueContinued;  /* ...also echo flag in the hdr   */
        }
      }
      return (amountWritten);                         /* return amount we wrote         */
 #ifdef REUSE_ENABLED
   } /* chunkSize == 0 */

    /* If we get here then we are going to reuse a free space segment...                */

    CMfseek(container, offset, kCMSeekSet);           /* overwrite the free space chunk */
    if (CMfwrite(container, buffer, sizeof(unsigned char), chunkSize) != chunkSize)
      return (amountWritten);

    if (prevValue != NULL) {                          /* if this is not 1st segment...  */
      prevValue->flags |= kCMContinued;               /* ...flag last seg as cont'd     */
      theValueHdr->valueFlags |= ValueContinued;      /* ...also echo flag in the hdr   */
    }

    (void)cmSetValueBytes(container, &valueBytes, Value_NotImm, offset, chunkSize);
    prevValue = cmAppendValue(theValueHdr, &valueBytes, 0); /* append new value segment */
    if (prevValue == NULL) return (amountWritten);

    offset += chunkSize;                              /* set logical EOF                */
    SetLogicalEOF(offset);

    buffer        += chunkSize;                       /* point to next chunk to write   */
    amountWritten += chunkSize;                       /* count total amount written     */
    size          -= chunkSize;                       /* reduce amount yet to be written*/
  } /* while */                                       /* loop through all the data      */

  return (amountWritten);                             /* return amount we wrote         */
#endif
}


/*----------------------------------------------------------------*
 | cmDeleteFreeSpace - delete free space beyond a specified point |
 *----------------------------------------------------------------*

 This routine is called to remove all free list entries that have their space beyond the
 specified offset, beyondHere.  All entries with starting offsets greater than or equal
 to beyondHere are removed.  If one spans beyondHere it will be reduced appropriately.

 These entries are removed so so that we may overwrite from beyondHere with a new TOC.
 Since we are reusing that space for the TOC we obviously can't keep free list entries for
 it.
*/

void cmDeleteFreeSpace(ContainerPtr container, CMCount beyondHere)
{
#ifdef REUSE_ENABLED
  TOCValuePtr   theValue, nextValue;
  CMCount offset;
  CMSize 	newLen;

  if (container->freeSpaceValueHdr == NULL) return;       /* exit if no free list       */

  theValue = (TOCValuePtr)cmGetListHead(&container->freeSpaceValueHdr->valueList);

  while (theValue) {                                      /* ...scan free list          */
    nextValue = (TOCValuePtr)cmGetNextListCell(theValue); /* get next now for deletes   */
    offset    = theValue->value.notImm.value;             /* offset of a free chunk     */

    if (offset >= beyondHere)                             /* if chunk entirely beyond   */
      deleteFreeListEntry(theValue);                      /* ...delete it               */
    else if (offset + theValue->value.notImm.valueLen > beyondHere) { /* if partial...  */
      newLen = beyondHere;
      omfsSubInt64fromInt64(offset, &newLen);             /* ...cut size down           */
      #if TOC1_SUPPORT
      if (container->majorVersion == 1) {                 /* and if format 1 TOC...     */
        if (newLen <= TOCentrySize)                       /* ...and too small to keep   */
          deleteFreeListEntry(theValue);                  /* ...delete it too           */
        else                                              /* ...if still acceptable     */
          theValue->value.notImm.valueLen = newLen;       /* ...set its new smaller size*/
      } else {                                            /* if >format 1 TOC           */
        if (newLen <= MinTOCSize)                         /* ...and still too small     */
          deleteFreeListEntry(theValue);                  /* ...delete it too           */
        else                                              /* ...if still acceptable     */
          theValue->value.notImm.valueLen = newLen;       /* ...set its new smaller size*/
      }
      #else
      if (newLen <= MinTOCSize)                           /* ...but if too small to keep*/
        deleteFreeListEntry(theValue);                    /* ...delete it too           */
      else                                                /* ...if still acceptable     */
        theValue->value.notImm.valueLen = newLen;         /* ...set its new smaller size*/
      #endif
    }

    theValue = nextValue;                                 /* loop till no more entries  */
  } /* while */
#else
	return;
#endif
}

                              CM_END_CFUNCTIONS
