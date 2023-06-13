/*---------------------------------------------------------------------------*
 |                                                                           |
 |                            <<<  TOCEnts.c   >>>                           |
 |                                                                           |
 |                    Container Manager TOC Entry Routines                   |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 12/02/91                                  |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1991-1992                 |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file contains the manipulation routines to handle TOC objects below the object
 level.  The objects and the means to get at them are maintained in TOCObjects.c.  The
 split is done this way to keep the accessing of objects separate from the stuff
 represented by those objects.

 The following diagram shows the pointer relationships among the various data structures
 descending from an object.  The data structurs leading to the object itself are handled
 independently by the  TOCObjs.c   routines.  They are of no concern here as the structures
 described here are of no concern to  TOCObjs.c  !

 Arrows of the form "<---->" represent double links of forward/backward link lists. Arrows
 of the form "--->" are pointers in the indicated direction.  A "+" in an arrow just means
 it's turning or intersecting since there is no other means to notate it given the
 character set used!

 A name above a box corresponds to the actual typedef struct name.  A "*name*" just
 signifies what the structure is and destinguishes it from other similar structures in the
 diagram.  Names in brackets are fields in the structures.  The fields representing the
 list links and headers are not shown.


                        +<-----+<--------------------+<--------------------+
                        |      |                     |                     |
      TOCObject         |      | TOCProperty         |                     |
 *------------------*<--+     *--------------*      *--------------*      *--------------*
 |     *Object*     |<------->|  *Property1* |<---->|  *Property2* |<---->|  *Property3* |
 |   [objectID]     | +>+---->| [propertyID] |      | [propertyID] |      | [propertyID] |
 |   [container]    | | |     *--------------*      *--------------*<-+   *--------------*<-+
 |   [nextObject]   | | |                                             |                     |
 |[nextTypeProperty]| | |                                             |                     |
 |  [objectFlags]   | | |                                             +->...                +->...
 *------------------* | |
                      | |       TOCValueHdr          TOCValue
                      | |    *--------------*      *----------*
                      | +--->|  *ValueHdr1* |<---->| *Value1* |
 The nextObject       +<-----|   [typeID]   |      | [flags]  |
 links ALL objects.   |      | [container]  |      | [value]  |
                      |      | [valueFlags] |      *----------*
                      |      | [generation] |           |
                      | +--->|    [size]    |           |
 The nextTypeProperty | |    *--------------*<----------+
 is used for two      | |
 separate chains;     | |
 one that links all   | |
 property objects,    | |    *--------------*      *-----------*      *-----------*
 and one that links   | +--->|  *ValueHdr2* |<---->| *Value21* |<---->| *Value22* |
 all type objects.    +<-----|   [typeID]   |      |  [flags]  |      |  [flags]  |
                      |      | [container]  |      |  [value]  |      |  [value]  |
                      |      | [valueFlags] |      *-----------*      *-----------*
                      |      | [generation] |            |                  |
                      | +--->|    [size]    |            |                  |
                      | |    *--------------*<-----------+<-----------------+
                      | |                                     (continued)
                      | |
                      | |
                      | |
                      | |    *--------------*      *-----------*    Note, all value
                      | +--->|  *ValueHdr3* |<---->|  *Value3* |    entries have a ptr
                      +<-----|   [typeID]   |      |  [flags]  |    to their "owning"
                             | [container]  |      |  [value]  |    ValueHdr to be able
                             | [valueFlags] |      *-----------*    to get at the TypeID
                             | [generation] |            |          and container.
                             |    [size]    |            |
                             *--------------*<-----------+


 The above diagram shows a single object with three properties. Properties are chained off
 the object. Only the layout for the first property is shown.  Propert1 has three values.
 Values are chained off a value header and the value headers are chained off the properties.
 Value2 represents a continued value. They are chained together, and each value entry
 (continued or not) has a back pointer to its value header.  Similarly, the value headers
 have back pointers to their property and the properties back to their object.  This
 allows us to get at the owning container and list headers.

 Not shown in this example diagram is the handling of global names. They are contained in
 a binary tree symbol table.  Each entry in that symbol table contains the symbol itself
 and a back pointer to its corresponding value entry for which the global name represents
 the value.  The value, in turn, points to its global name symbol table entry.  This
 global name layout allows us to keep the names in memory for mapping of global names
 to objects or vice versa.  By keeping the global names on a binary tree we can
 efficiently walk the tree to write the value out to the container. This is more efficient
 than the alternative, which is walking the entire TOC structure looking only for the
 global name value pointers to just write them out.

 Note, the API permits deletion of objecs and values.  This is implemented by two separate
 deletion lists.  There is a list of all deleted TOCObject's and another list of all
 deleted TOCValueHdr's.  Pointers to these are returned to the API user as "refNums". Thus
 these are the only entities that we must never free in an open container.  All the other
 stuff, including TOCValue's off the TOCValueHdr's, can be freed when a delete is done.
 The reason we keep this stuff around at all is to protect ourselves against the silly
 user passing a "refNum" to a deleted object back to us!  The "refNum"s remain valid
 pointers.  When a TOCObject or TOCValueHdr is put on its list, it is flagged as deleted.
 Wenever a API caller passes in a "refNum" we check it to make sure it's still valid and
 yell if it isn't.  It's a lot of machinery for this.  But do you have a better idea?


            ---> Help for those looking at this file for the firtst time <---

 The place to start looking is at the four routines cmAppendValue(), defineValue(),
 defineProperty(), and cmDefineObject().  They are the main routines that implement the
 above structures. Understand them and the rest should fall out.  That's how this file was
 developed and all other stuff "spun out" from there.
*/


#include <stddef.h>
#include <string.h>
#include <setjmp.h>
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
#ifndef __TOCIO__
#include "TOCIO.h"
#endif
#ifndef __OLDTOCENTRIES__
#include "oldtoc.h"
#endif
#ifndef __TOCOBJECTS__
#include "TOCObjs.h"
#endif
#ifndef __GLOBALNAMES__
#include "GlbNames.h"
#endif
#ifndef __CONTAINEROPS__
#include "Containr.h"
#endif
#ifndef __VALUEROUTINES__
#include "Values.h"
#endif
#ifndef __HANDLERS__
#include "Handlers.h"
#endif
#ifndef __UPDATING__
#include "Update.h"
#endif
#ifndef __DYNAMICVALUES__
#include "DynValus.h"
#endif
#ifndef __BUFFEREDIO__
#include "BufferIO.h"
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


/* The following generates a segment directive for Mac only due to 32K Jump Table       */
/* Limitations.  If you don't know what I'm talking about don't worry about it.  The    */
/* default is not to generate the pragma.  Theoritically unknown pragmas in ANSI won't  */
/* choke compilers that don't recognize them.  Besides why would they be looked at if   */
/* it's being conditionally skipped over anyway?  I know, famous last words!            */

#if __MAC__
#pragma segment TOCEntries
#endif


#ifndef DEBUG_TOCENTRIES
#define DEBUG_TOCENTRIES 0
#endif

#define dbgFile stdout          /* trace display output file                            */

/* The following is used in this file by cmWalkThroughEntireTOC(), etc. to hold the     */
/* function pointers, caller's refCon, and setjmp/longjmp escape vector.  See that      */
/* routine for further details.                                                         */

/* The following has a similar goal in life as above, but is used by cmWriteTOC()...    */

struct WriteTOCCommArea {       /* cmWriteTOC() TOC walking communication area layout:  */
  void           *toc;          /*    TOC being walked                                  */
  CMCount	  tocOffset;     /*    next free byte offset in toc to write to          */
  CMSize	  tocSize;       /*    total size of the toc                             */
  TOCentry       tocEntry;      /*    the TOC entry buffer to be created and written    */
  void           *ioBuffer;     /*    the I/O buffer when buffered I/O is used          */
  OldBackPatchesPtr thePatches;    /*    ptr to the TOC entries saved for back-patching    */
};
typedef struct WriteTOCCommArea WriteTOCCommArea, *WriteTOCCommAreaPtr;

/* Note, the tocSizeEntry fields in the above struct are used to overwrite those TOC    */
/* entries.  The tocSizeEntry is the entry that contains the TOC size, which we don't   */
/* know until we're done writing the TOC. Thus we must "go back" and rewrite this entry */
/* after we do know.  We overwrite the entire entry.  That lets us use the handler      */
/* provided for TOC entry writes. We thus keep all the fields we do know for efficiency.*/

/* The tocContEntry is similar to tocSizeEntry, but that represents the entire container*/
/* size, up to and including the label.  Again we must delay until we know the size.    */



/*--------------------------------------------------------------------------*
 | writeTOCValue - write a TOC container entry for a value to the container |
 *--------------------------------------------------------------------------*

 This is used as the "action" parameter to cmWalkThroughEntireTOC() to handle values while
 writing TOC entries to the container.  The refCon here is a pointer to a communication
 area (type WriteTOCCommArea) which contains the TOC container entry we have been building
 up with writeTOCObject(), writeTOCProperty(), and writeTOCValueHdr().  They were called
 as cmWalkThroughEntireTOC() worked its way through the TOC links.

 writeTOCValue() is the deepest call in the walk. It is here we add the value, flags, and
 generation info to the TOC entry and write it to the container.

 In addition to the TOC entry we are building, the communication area also contains the
 current total TOC size and offset.  These are updated as we write.

 Note, we assume the proper positioning here as initiated by cmWriteTOC() which called
 cmWalkThroughEntireTOC() to start this thing going.  We update the offset assuming it is
 in sync with the writing.

 Note, this "static" is intentionally left to default memory model under DOS since it is
 passed as a function pointer to cmWalkThroughEntireTOC().
*/
CM_CFUNCTION
static TOCWalkReturns writeTOCValue(ContainerPtr container, TOCValuePtr theValue, CMRefCon refCon)
{
  WriteTOCCommAreaPtr t = (WriteTOCCommAreaPtr)refCon;
#ifdef PORT_FILESYS_40BIT
  omfInt32			upper;
#endif
  unsigned int 		wrPropID, wrTypeID;
  unsigned int        propertyID, value32, valueLen32;
  #if (TOCOutputBufSize == 0)
  unsigned char       tocBuffer[TOCentrySize];
  #endif

  /* Don't write values for standard properties except for the TOC object itself.  To   */
  /* give us some latitude here, we allow writing of standard properties if their       */
  /* property ID's are less than or equal to CM_StdObjID_Writable.  There may be some   */
  /* private properties we might want written.  Using an upper limit means we don't     */
  /* have to change the following code every time.  Note that the initial test here is  */
  /* on the object ID.  If we have a user object we ALWAYS write all its properties,    */
  /* predefined or not.                                                                 */

  if (t->tocEntry.objectID >= CM_StdObjID_TOC && t->tocEntry.objectID < MinUserObjectID) {
    propertyID = theValue->theValueHdr->theProperty->propertyID;
    if (propertyID > CM_StdObjID_Writable && propertyID < MinUserObjectID)
      return (WalkNextTOCValueHdr);
  }

  /* Don't write dynamic values...                                                      */

  if (t->tocEntry.propertyID == CM_StdObjID_DynamicValues) return (WalkNextTOCValueHdr);

  /* Don't write refData lists (1.0d4 ONLY!!!! RPS!!!) */

  if (t->tocEntry.propertyID == CM_StdObjID_ObjReferences) return (WalkNextTOCValueHdr);

  /* If we have a global name symbol table entry pointer for the value, then we assume  */
  /* that by this time the global names have been written to the TOC and the offset     */
  /* placed in the value data.  We have to use the global name pointer to get the length*/
  /* of the string.                                                                     */

  if (theValue->flags & kCMGlobalName) {
    t->tocEntry.value.notImm.value    = theValue->value.globalName.offset;
    omfsCvtUInt32toInt64(GetGlobalNameLength(theValue->value.globalName.globalNameSymbol) + 1, &t->tocEntry.value.notImm.valueLen);
    t->tocEntry.flags                 = 0;              /* std value in the container   */
  } else {                                              /* normal values handled here...*/
    t->tocEntry.value = theValue->value;
    t->tocEntry.flags = theValue->flags;                /* set flags as is              */
  }

  /* If the TOC entry we are about to write is for the TOC object property value, we    */
  /* must remember the offset to the TOC entry as written to the container.  At this    */
  /* point we still don't know the total TOC size (although we do know its offset).  So */
  /* we must "back patch" this entry in the container after the entire TOC is written.  */
  /* Remembering the offset and entry fields allows us to do the back patch.            */

  /* In a similar way, we have to back patch the container size value. This is a value  */
  /* that represents the entire container from offset 0 to the end of label.            */

  if (t->thePatches != NULL)                            /* if patches are required...   */
    if (theValue == container->tocObjValue) {           /* ...remember appropriate info */
      t->thePatches->tocSizeEntry.offset   = t->tocOffset;
      t->thePatches->tocSizeEntry.tocEntry = t->tocEntry;
      t->thePatches->whatWeGot |= Patch_TOCsize;
    } else if (theValue == container->tocContainerValue) {
      t->thePatches->tocContEntry.offset   = t->tocOffset;
      t->thePatches->tocContEntry.tocEntry = t->tocEntry;
      t->thePatches->whatWeGot |= Patch_TOCContSize;
    } else if (theValue == container->tocNewValuesValue) {
      t->thePatches->tocNewValuesTOCEntry.offset   = t->tocOffset;
      t->thePatches->tocNewValuesTOCEntry.tocEntry = t->tocEntry;
      t->thePatches->whatWeGot |= Patch_TOCNewValuesTOC;
    }

  /* Finially, write the completed TOC entry to its container...                        */
	omfsTruncInt64toUInt32(t->tocEntry.value.notImm.value, &value32);
	omfsTruncInt64toUInt32(t->tocEntry.value.notImm.valueLen, &valueLen32);
	wrPropID = t->tocEntry.propertyID;
	wrTypeID = t->tocEntry.typeID;
#ifdef PORT_FILESYS_40BIT
        if((theValue->flags & kCMImmediate) == 0)
	   {
	     omfsDecomposeInt64(t->tocEntry.value.notImm.value, &upper,
			   (omfInt32 *)&value32);
	     wrPropID |= (upper << 24L) & 0xFF000000;
	     omfsDecomposeInt64(t->tocEntry.value.notImm.valueLen, &upper,
			   (omfInt32 *)&valueLen32);
	     wrTypeID |= (upper << 24L) & 0xFF000000;
	   }
/*	else
	  {
	    omfsDecomposeInt64(t->tocEntry.value.notImm.value, &upper,
			   (omfInt32 *)&value32);
	    omfsDecomposeInt64(t->tocEntry.value.notImm.valueLen, &upper,
			   (omfInt32 *)&valueLen32);
	  }
*/
#endif

  #if !DEBUG_TOCENTRIES
  #if (TOCOutputBufSize > 0)                          /* if buffering, format here...   */
  BufferedFormatTOC(t->ioBuffer, t->tocEntry.objectID,
                                 wrPropID,
                                 wrTypeID,
                                 value32,
                                 valueLen32,
                                 t->tocEntry.generation,
                                 t->tocEntry.flags);
  #else                                               /* non-buffered output...         */
  OldFormatTOC(tocBuffer, t->tocEntry.objectID,
                       wrPropID,
                       wrTypeID,
                       value32,
                       valueLen32,
                       t->tocEntry.generation,
                       t->tocEntry.flags);
  if (CMfwrite(container, tocBuffer, sizeof(unsigned char), TOCentrySize) != TOCentrySize) {
    ERROR1(container, CM_err_BadTOCWrite, CONTAINERNAME);
    AbortWalkThroughEntireTOC(CM_err_BadTOCWrite);
  }
  #endif
  #else
  #pragma unused (container)
  fprintf(dbgFile, "%.8lX:%.4X | %.8lX | %.8lX | %.8lX | %.8lX | %.8lX | %.4X | %.4X |\n",
                   t->tocOffset, t->tocSize,
                   t->tocEntry.objectID, t->tocEntry.propertyID, t->tocEntry.typeID,
                   t->tocEntry.value.notImm.value, t->tocEntry.value.notImm.valueLen,
                   t->tocEntry.generation, t->tocEntry.flags);
  #endif

  /* Update the offset and size info in the communication area...                       */

  omfsAddInt32toInt64(TOCentrySize, &t->tocOffset);   /* maintained only for debugging  */
  omfsAddInt32toInt64(TOCentrySize, &t->tocSize);     /* this will be returned to caller*/

  return (WalkNextTOCValue);                          /* continue walking value segs    */
}
CM_END_CFUNCTION


/*------------------------------------------------------------------*
 | writeTOCValueHdr - build up a TOC container entry with a type ID |
 *------------------------------------------------------------------*

 This is used as the "action" parameter to cmWalkThroughEntireTOC() to handle value header
 structs while writing TOC entries to the container.  The refCon here is a pointer to a
 communication area (type WriteTOCCommArea) which contains the TOC container entry we are
 building up in preparation for writing when a value is reached.  All we need do here is
 set the type ID in the entry.  cmWalkThroughEntireTOC() will next call writeTOCValue()
 which is where we will do the actual writing of the TOC entry we have been building up
 with this routine, writeTOCProperty(), and writeTOCObject() below.

 Note, this "static" is intentionally left to default memory model under DOS since it is
 passed as a function pointer to cmWalkThroughEntireTOC().
*/
CM_CFUNCTION
static TOCWalkReturns writeTOCValueHdr(ContainerPtr container, TOCValueHdrPtr theValueHdr, CMRefCon refCon)
{
  WriteTOCCommAreaPtr t = (WriteTOCCommAreaPtr)refCon;
  TOCValueBytes       value;
  char                idStr[15];
  ContainerPtr        unused = container;
  unsigned int       minimumSeed;

  /* Only write TOC entries for the current container.  Values can be marked as         */
  /* belonging to other (i.e, updating target) containers.  We don't want those.        */

  if (theValueHdr->container != container)                /* if not current container...*/
    return (WalkNextTOCValueHdr);                         /* ...suppress writing value  */

  /* If we are looking at the special property for a dynamic value, then we have an     */
  /* outstanding dynamic value that has not been released with a CMReleaseValue().  It  */
  /* is required that dynamic values be released COMPLETELY.  That causes the special   */
  /* property containing the dynamic values for the object to be removed.  We would     */
  /* then not see them here.  If we do, it's an error...                                */

  /* Well, not always an error!  We "cheat" a little here.  As it turns out we may be   */
  /* use a dynamic value as a pointing value to a target we're updating.  Its one we    */
  /* created that the user knows nothing about.  It WILL (trust me) be released, just   */
  /* not at the time we're writing this TOC.  So we want to ignore that dynamic value.  */
  /* We can detect we got it because we create its type before we have the target       */
  /* container (we need it to get the target, so it's a "chicken-and-egg" problem). Such*/
  /* types will have type IDs less than the min seed value.  That's why we keep a min   */
  /* seed.  So we can tell what objects "belong" to the updating container and what     */
  /* objects are updates.                                                               */

  if (t->tocEntry.propertyID == CM_StdObjID_DynamicValues) {/* if a dynamic value...    */
    omfsTruncInt64toUInt32(container->tocIDMinSeedValue->value.imm.value, &minimumSeed);
    if (theValueHdr->typeID >= minimumSeed)                     /* ...that's >= MinSeed...  */
      ERROR1(container, CM_err_NotReleased, CONTAINERNAME);            /* ...why wasn't it released*/
    return (WalkNextTOCValueHdr);                           /* ...suppress writing value*/
  }

  t->tocEntry.typeID     = theValueHdr->typeID;           /* set type ID                */
  t->tocEntry.generation = theValueHdr->generation;       /* set generation             */

  /* Make sure that this valueHdr does indeed have a value. When a CMNewValue() is done */
  /* an object is created but the value header contains no value list.  At that point   */
  /* if any of these are still lying around we will see them here. It represents        */
  /* an object that was never given a value.  That's a no, no...                        */

  if (cmIsEmptyList(&theValueHdr->valueList)) {           /* error if no value list...  */
    ERROR3(container, CM_err_NoValue, cmltostr(theValueHdr->theProperty->theObject->objectID, 1, false, idStr),
                           cmGetGlobalPropertyName(container, t->tocEntry.propertyID),
                           CONTAINERNAME);
    (void)cmSetImmValueBytes(container, &value, Value_NotImm, 0xFFFFFFFFU, 0); /* -1 offset*/
    if (cmAppendValue(theValueHdr, &value, 0) == NULL)
      AbortWalkThroughEntireTOC(CM_err_NoValue);          /* abort if this fails (damit)*/
  } else if (theValueHdr->dynValueData.dynValue != NULL)  /* safety for dynamic values  */
    theValueHdr->dynValueData.dynValue = NULL;

  return (WalkNextTOCValue);                              /* walk the value segments    */
}
CM_END_CFUNCTION


/*----------------------------------------------------------------------*
 | writeTOCProperty - build up a TOC container entry with a property ID |
 *----------------------------------------------------------------------*

 This is used as the "action" parameter to cmWalkThroughEntireTOC() to handle property
 structs while writing TOC entries to the container.  The refCon here is a pointer to a
 communication area (type WriteTOCCommArea) which contains the TOC container entry we are
 building up in preparation for writing when a value is reached.  All we need do here is
 set the property ID in the entry.  cmWalkThroughEntireTOC() will next call
 writeTOCValueHdr() to do a similar stunt for the type ID.

 Note, this "static" is intentionally left to default memory model under DOS since it is
 passed as a function pointer to cmWalkThroughEntireTOC().
*/
CM_CFUNCTION
static TOCWalkReturns writeTOCProperty(ContainerPtr container, TOCPropertyPtr theProperty, CMRefCon refCon)
{
  WriteTOCCommAreaPtr t = (WriteTOCCommAreaPtr)refCon;
  ContainerPtr        unused = container;

  t->tocEntry.propertyID = theProperty->propertyID;

  return (WalkNextTOCValueHdr);                           /* walk value headers         */
}
CM_END_CFUNCTION


/*------------------------------------------------------------------*
 | writeTOCObject - build up a TOC container entry with a object ID |
 *------------------------------------------------------------------*

 This is used as the "action" parameter to cmWalkThroughEntireTOC() to handle object
 structs while writing TOC entries to the container.  The refCon here is a pointer to a
 communication area (type WriteTOCCommArea) which contains the TOC container entry we are
 building up in preparation for writing when a value is reached.  All we need do here is
 set the value ID in the entry.  cmWalkThroughEntireTOC() will next call writeTOCProperty()
 to do a similar stunt for the property ID.

 Note, this "static" is intentionally left to default memory model under DOS since it is
 passed as a function pointer to cmWalkThroughEntireTOC().
*/
CM_CFUNCTION
static TOCWalkReturns writeTOCObject(ContainerPtr container, TOCObjectPtr theObject, CMRefCon refCon)
{
  WriteTOCCommAreaPtr t = (WriteTOCCommAreaPtr)refCon;
  ContainerPtr        unused = container;

  t->tocEntry.objectID = theObject->objectID;

  return (WalkNextTOCProperty);                           /* walk properties            */
}
CM_END_CFUNCTION


/*-----------------------------------------*
 | cmWriteTOC - write a TOC to a container |
 *-----------------------------------------*

 This routine is the inverse to cmReadTOC() to write the in-memory TOC to a container. The
 TOC is written to the end of the container.  Only objects with ID's greater than or equal
 to the startingID and less than or equal to the endingID are written.  Further, only
 objects whose value headers are equal to the specified container are written.

 With the restriction on the container, we will only write to the TOC newly created values
 for this open session.  If we're not updating, only one container so this is not an
 issue.  But for updating, only looking at the stuff in the updating container will yield
 all new objects and all new properties for "old" objects.  We can split these two groups
 using the ID limits.

 Note, non-TOC updates are generated by a separate walk not done here.  See Updating.c.

 For updating, a container may have its own TOC and the target TOC.  Thus the TOC pointer
 is an explicit parameter.  The container offset is returned in tocStart and the size of
 the TOC in tocSize.  The function returns true to indicate success and false otherwise
 (as a safety).

 There are some TOC entries that must be back-patched. This is because they are a function
 of the final TOC size in the container.  We don't know that until the entire TOC is
 written.  At that time we can rewrite the entries with the proper values filled in.

 Because of updating, there may be multiple calls to cmWriteTOC() to write various subsets
 of the TOC (i.e., the updates).  Thus we may not know the final TOC size when we return
 from here.  The back-patching cannot be done.  Only the caller knows when all TOC writes
 are complete and the back-patching can be done.

 To allow this to happen, thePatches is passed as a pointer to a BackPatches struct where
 we save the original unpatched TOC info and offset for the entries of interest.  As these
 entries are encountered (and they must be sometime) during TOC writing, we save them and
 their offsets in the struct. When the caller is ready to rewrite them cmDoBackPathes() is
 called to do it.

 If thePatches is passed as NULL, nothing (obviously) is saved for back-patching.
*/

Boolean cmOldWriteTOC(ContainerPtr container, void *toc, CMObjectID startingID,
                   CMObjectID endingID, BackPatchesPtr patches,
                   CMCount *tocStart, CMSize *tocSize)
{
  WriteTOCCommArea tocCommArea;
  int              x;
  CMCount	    offset, logicalEOF0, logicalEOF, slopSize, truncVal;
  CMSize		physSize, labelSiz;
  OldBackPatchesPtr thePatches = &(patches->oldBP);
  omfInt64			zero;


  #if (TOCOutputBufSize > 0)
  jmp_buf          writeTOCEnv;
  #else
  unsigned char    padding[5], *p;
  #endif

 omfsCvtUInt32toInt64(0, &zero);
  #if (TOCOutputBufSize > 0)                        /* if buffering...                  */

  /* If we are using buffered writes to output the TOC, then the output buffering       */
  /* routines report their errors and exit via a longjmp on the setjmp/longjmp          */
  /* environment we are about to define.  This avoids checking for errors explicitly on */
  /* each write that we do in here.                                                     */

  tocCommArea.ioBuffer = NULL;                      /* buffer not allocated yet         */

  if (setjmp(writeTOCEnv)) {                        /* set setjmp/longjmp env.  vector  */
    cmReleaseIOBuffer(tocCommArea.ioBuffer);        /* ...if longjmp taken back to here */
    ERROR1(container, CM_err_BadTOCWrite, CONTAINERNAME);      /* ...free buffer and report failure*/
    return (false);                                 /* ...false ==> failure             */
  }

  #endif

  /* RPS! Set major and minor versions so that label will be written correctly */
  container->majorVersion = 1;
  container->minorVersion = 0;

  /* Set the "seed" user object ID value in the special TOC entry.  This is picked up   */
  /* when the container is opened for reading to allow us to continue on using unique   */
  /* object ID numbers.  The pointer to the value for the special TOC entry was set when*/
  /* the container was opened.                                                          */

  omfsCvtUInt32toInt64(container->nextUserObjectID, &container->tocIDSeedValue->value.imm.value);

  #if DEBUG_TOCENTRIES
  fprintf(dbgFile, "\n"
                   "              | objectID |propertyID|  typeID  |  value   |  length  | gen  |flags |\n"
                   "              +----------+----------+----------+----------+----------+------+------+\n");
  #endif

  /* We are now ready to write the TOC. Except for space reuse we always append the TOC */
  /* to the end of the container (i.e., at the physical EOF).  But because of space     */
  /* reuse we may have to write the TOC twice!  Let me try to explain...                */

  /* The reason is that with space reuse we start the TOC at the logical EOF rather     */
  /* than the physical EOF if it is less than the physical EOF.  CMOpenContainer() sets */
  /* the logical EOF to the start of the "old" TOC for space reusing updates and the    */
  /* "old" TOC is redefined as reusable data free space.  That, along with possibly     */
  /* already exiting free space, may be enough to cover all the updates.  Then we can   */
  /* start writing the new TOC at the logical EOF to minimize growth of the container.  */
  /* If it isn't, we append the TOC to the end of the container as usual.               */

  /* Note, that the container can NEVER get smaller in this "pure" portable             */
  /* implementation of the Container Manager.  The reason is that we assume the I/O     */
  /* handlers are using standard ANSI stream I/O.  It provides NO facility for cutting  */
  /* a stream back!  This is important and the reason there are two potential writes of */
  /* the TOC.                                                                           */

  /* If we indeed do start writing the TOC at the logical EOF, it is possible that the  */
  /* new TOC is small enough that we don't write enough entries to reach the physical   */
  /* EOF.  We then have a bunch of space ("slop") from the end of the new TOC to the    */
  /* physical EOF to account for.  The container looks something like this at this      */
  /* point (the label is not yet written at this point, but the physical EOF would      */
  /* follow the old label):                                                             */

  /*              *------------------*-----------*----------*- - - - - - -*             */
  /*   container: |<----- data ----->|<-- TOC -->|<- slop ->|    label    |physical eof */
  /*              *------------------*-----------*----------* - - - - - - -*            */

  /* We cannot leave the slop where it is because we always write the container label   */
  /* immediately following the TOC.  Thus we must "move" the TOC so that we have the    */
  /* following configuration:                                                           */

  /*              *------------------*----------*-----------*- - - - - - -*             */
  /*   container: |<----- data ----->|<- slop ->|<-- TOC -->|    label    |physical eof */
  /*              *------------------*----------*-----------*- - - - - - -*             */

  /* Unfortunately, there is no easy way to determine the TOC size until we write it, so*/
  /* there is no way to know the slop ahead of time.  We could do all the same logic as */
  /* we do to write it and suppress the writing.  But who knows?  We could get lucky    */
  /* and have no slop.  So we write the TOC to see if the slop occurs.  This is TOC     */
  /* write number 1.                                                                    */

  /* Now if there indeed is slop, we do the "move" by doing the TOC write the second    */
  /* time (hence the two possible writes).  The slop is first moved to the free list.   */
  /* This means the second TOC write will be slightly different (and one TOC entry      */
  /* larger).  This implies that the copy must be done as a second TOC write rather     */
  /* than by some other means (e.g., like copying the TOC as raw bytes down).           */

  /* The following will help define the variables we use for this so you can understand */
  /* the math:                                                                          */

  /*                            LogicalEOF0                 x        physicalEOF        */
  /*                                 |                      |             |             */
  /*              *------------------*-----------*----------*- - - - - - -*             */
  /*   container: |<----- data ----->|<-- TOC -->|<- slop ->|    label    |             */
  /*              *------------------*-----------*----------*- - - - - - -*             */
  /*                                             |                                      */
  /*                                         LogicalEOF     |<- LBLsize ->|             */

  /*  size of slop = (physicalEOF - LBLsize) - LogicalEOF  (takes label into account)   */

  /* For the second TOC write to happen, the logical EOF after the first TOC write must */
  /* be less then "x" (= physicalEOF - LBLsize).  The slop is added to the free list    */
  /* with a starting offset of LogicalEOF0.  LogicalEOF is then moved back to           */
  /* LogicalEOF0 + slop size.  The second TOC write will thus end at "x" (or one TOC    */
  /* entry further if the slop was placed uniquely on the free list).                   */

  /* Now, having described all this crap, there is a possibility you can forget it!     */
  /* Remember, that the reason for all of this is due to the limitations of standard    */
  /* ANSI C I/O.  Specific implementations may have library (or whatever) support for   */
  /* cutting a stream back. If we could cut it back, we would cut it back to the logical*/
  /* EOF and write the TOC to the new physical EOF which now equals the logical EOF.    */

  /* So, on the possibility that such a support exists, a special handler, "trunc", may */
  /* be defined to "truncate" the stream.  We test for its presence here. If it exists, */
  /* we call it and "trust" it to do the truncation. Then we only have to do the single */
  /* TOC write.                                                                         */

  /* Finally, one last point to keep in mind about all this.  Even if the trunc handler */
  /* is not supplied, the probability of the two writes is, I think, fairly small!  The */
  /* reason comes from the fact of noting how you can get a smaller TOC in the first    */
  /* place.  The only way it can happen is to free values.  That reduces the number of  */
  /* TOC entries.  But they are replaced by free list entries.  So, unless free space   */
  /* coalesces (which it could do), there will be no net loss of TOC entries and hence  */
  /* the TOC will not get smaller.  So what's the probability of separate values data   */
  /* space coalescing?  That is basically the probability of the TOC growing smaller.   */

  /* Whew!                                                                              */

  logicalEOF0 = container->logicalEOF;              /* save original logical EOF        */

  for (;/*two possible tries*/;) {

    /* Seek to the start of where we want to put the TOC...                             */

    offset = CMgetContainerSize(container);         /* this is the next free byte       */
    if (omfsInt64GreaterEqual(container->logicalEOF, offset))    /* if we wrote up to this point...  */
      CMfseek(container, zero, kCMSeekEnd);            /* ...append TOC to end of container*/
    else if (container->handler.cmftrunc != NULL && /* if truncation handler provided...*/
             CMftrunc(container, container->logicalEOF)) {/* ...and it trucated containr*/
      offset = CMgetContainerSize(container);       /* ...get new truncated size        */
      container->logicalEOF = offset;               /* ...logical EOF == physical now   */
      CMfseek(container, zero, kCMSeekEnd);            /* ...position to EOF               */
    } else {                                        /* if we didn't use all (old) space */
      offset = container->logicalEOF;               /* ...overwrite from logical EOF    */
      CMfseek(container, offset, kCMSeekSet);
      cmDeleteFreeSpace(container, offset);         /* kill free list entries beyond EOF*/
    }

    /* Allocate the output buffer and prepare for buffered writing of the TOC...        */

    #if (TOCOutputBufSize > 0)                      /* if buffering...                  */
    if (tocCommArea.ioBuffer == NULL) {             /* allocate buffer 1st time around  */
      tocCommArea.ioBuffer = cmUseIOBuffer(container, TOCOutputBufSize, (jmp_buf *)&writeTOCEnv);
      if (tocCommArea.ioBuffer == NULL) return (false);
    }

    cmNewBufferedOutputData(tocCommArea.ioBuffer, NULL);
    #endif

    /* Pad the container to align it on a multiple of 4 boundary. We fill the pad bytes */
    /* with 0xFF so they are easy to spot during debugging.                             */

    omfsCvtUInt32toInt64(0, tocSize);                /* init size                        */
	*tocStart = offset;
	omfsAddInt32toInt64(3, tocStart);
	omfsMakeInt64(~0x0, ~0x03, &truncVal);
	omfsInt64AndInt64(truncVal, tocStart);          /* round start up to multiple of 4  */

    if (omfsInt64Less(offset, *tocStart)) {                       /* if padding is necessary...       */
      #if (TOCOutputBufSize > 0)                    /* if buffering...                  */
      x = *tocStart - offset;                       /* ...create the padding of FF's    */
      if (x == 3) {
        PUT1(0xFFU, tocCommArea.ioBuffer);
        PUT2(0xFFFFU, tocCommArea.ioBuffer);
      } else if (x == 2)
        PUT2(0xFFFFU, tocCommArea.ioBuffer);
      else
        PUT1(0xFFU, tocCommArea.ioBuffer);
      offset = *tocStart;
      #else
      p = padding;                                  /* ...create the padding of FF's    */
      do
      {
        *p++ = 0xFF;
        omfsAddInt32toInt64(1, &offset);
      } while (omfsInt64Less(offset, *tocStart));
      x = p - padding;
      if ((int)CMfwrite(container, padding, sizeof(char), x) != x) { /*write the padding*/
        ERROR1(container, CM_err_BadTOCWrite, CONTAINERNAME);
        return (false);
      }
      #endif
    }

    /* Set up our communication area for the walk with the initial offset and size...   */

    tocCommArea.toc        = toc;
    tocCommArea.tocOffset  = container->tocOffset = offset;
    omfsCvtUInt32toInt64(0, &tocCommArea.tocSize);
    tocCommArea.thePatches = thePatches;

    /* Walk the entire TOC building up and writing each TOC container entry...          */

    x = cmWalkThroughEntireTOC(container, toc, startingID, endingID, &tocCommArea,
                               writeTOCObject, writeTOCProperty, writeTOCValueHdr,
                               writeTOCValue);
    if (x != 0) return (false);                             /* something went wrong!    */

    #if (TOCOutputBufSize > 0)                              /* if buffering...          */
    cmFlushOutputBuffer(tocCommArea.ioBuffer);              /* ...flush out last buffer */
    #endif

    #if DEBUG_TOCENTRIES
    fprintf(dbgFile, "%.8lX:%.4X +----------+----------+----------+----------+----------+------+------+\n\n",
                     tocCommArea.tocOffset, tocCommArea.tocSize);
    #endif

    /* Return TOC size to caller...                                                     */

    *tocSize = container->tocSize = tocCommArea.tocSize;

    container->physicalEOF = CMgetContainerSize(container); /* update physical EOF      */
    logicalEOF = *tocStart;
    omfsAddInt64toInt64(*tocSize, &logicalEOF);                      /* ...and the logical EOF   */
    SetLogicalEOF(logicalEOF);

    /* If we write up to the physical EOF, there is no slop, so we're done...           */

	omfsCvtUInt32toInt64(LBLsize, &labelSiz);
	physSize = container->physicalEOF;
    omfsSubInt64fromInt64(labelSiz, &physSize);
    if (omfsInt64GreaterEqual(container->logicalEOF, physSize)) break;

    /* Damn, we got some slop!  Put it on the free list with the original logical EOF as*/
    /* its position.  The set the logical EOF past it and loop around for the second    */
    /* TOC write.  That is guaranteed to reach the physical EOF (it better!) so we will */
    /* break out of the loop with the statement above.                                  */

    slopSize = physSize;
    omfsSubInt64fromInt64(container->logicalEOF, &slopSize);
    cmAddToFreeList(container, NULL, logicalEOF0, slopSize);/* put slop on free list    */
    container->logicalEOF = logicalEOF0;
    omfsAddInt64toInt64(slopSize, &container->logicalEOF);   /* back up logicalEOF       */
  } /* for (;two possible tries;) */                        /* try, try, again...       */

  #if (TOCOutputBufSize > 0)                                /* if buffering...          */
  cmReleaseIOBuffer(tocCommArea.ioBuffer);                  /* ...free the I/O buffer   */
  #endif

  return (true);                                            /* TOC successfully written!*/
}


/*----------------------------------------*
 | cmOldDoBackPathes - back-patch the TOC |
 *----------------------------------------*

 When a TOC is written (e.g., cmWriteTOC()), there are some entries that are dependent on
 the final size of the TOC.  Such entries must be back-pached with the proper info.  Only
 the callers generating the TOC in the container know when it is time to do the patching.
 They call this routine to do it when that time comes.

 True is returned if the back-patching is successful and false otherwise.

 By the time this routine is called, the original TOC entries in question and their
 offsets within the container have been recorded in a BackPatches struct whose pointer is
 passed in thePatches.  Currently only 3 such entries need back-patching:

 tocSizeEntry          -  The TOC size entry.  This is a property of TOC #1 which
                          represents the size and offset of the TOC itself.  The caller
                          passes this info in the tocSize and tocStart parameters
                          respectively.

 tocContEntry          -  The TOC container entry. This is a property of TOC #1 which
                          represents the entire container, from offset 0 to the end of the
                          label.  No additional parameters are passed for this.  But to
                          compute this, it is required that cmDoBackPathes() be the LAST
                          thing called prior to writing the container label.

 tocNewValuesTOCEntry -  The non-private TOC offset/size entry.  An updating TOC contains
                         a TOC #1 property that defines the offset and size of all TOC
                         entries that are to be applied to this container's target.  The
                         caller passes this info in the newValuesTOCSize and
                         newValuesTOCStart parameters respectively.  For non-updating
                         containers, these are ignored.
*/

Boolean cmOldDoBackPatches(ContainerPtr container, BackPatchesPtr patches,
                        CMCount tocStart, CMSize tocSize,
                        CMCount newValuesTOCStart, CMSize newValuesTOCSize)
{
  CMSize	 size;
  unsigned char tocBuffer[TOCentrySize];
  OldBackPatchesPtr thePatches = &(patches->oldBP);
  omfInt64		zero;
  unsigned int  value32, valueLen32, wrPropID, wrTypeID;
#ifdef PORT_FILESYS_40BIT
  int  upper;
#endif

  omfsCvtUInt32toInt64(0, &zero);
  /* Back-patch the TOC size entry, i.e., the entry that defines the offset and size of */
  /* the TOC itself...                                                                  */

  if ((thePatches->whatWeGot & Patch_TOCsize) == 0) {       /* must have gotten offset! */
    ERROR1(container, CM_err_Internal2, CONTAINERNAME);
    return (false);
  }

  cmSetValueBytes(container, &thePatches->tocSizeEntry.tocEntry.value, Value_NotImm,
                  tocStart, tocSize);
  omfsTruncInt64toUInt32(thePatches->tocSizeEntry.tocEntry.value.notImm.value, &value32);
  omfsTruncInt64toUInt32(thePatches->tocSizeEntry.tocEntry.value.notImm.valueLen, &valueLen32);
  wrPropID = thePatches->tocSizeEntry.tocEntry.propertyID;
  wrTypeID = thePatches->tocSizeEntry.tocEntry.typeID;
#ifdef PORT_FILESYS_40BIT
  omfsDecomposeInt64(thePatches->tocSizeEntry.tocEntry.value.notImm.value, &upper, (omfInt32 *)&value32);
  wrPropID |= (upper << 24L) & 0xFF000000;
  omfsDecomposeInt64(thePatches->tocSizeEntry.tocEntry.value.notImm.valueLen, &upper, (omfInt32 *)&valueLen32);
  wrTypeID |= (upper << 24L) & 0xFF000000;
#endif
  OldFormatTOC(tocBuffer, thePatches->tocSizeEntry.tocEntry.objectID,
                       wrPropID,
                       wrTypeID,
                       value32,
                       valueLen32,
                       thePatches->tocSizeEntry.tocEntry.generation,
                       thePatches->tocSizeEntry.tocEntry.flags);
  CMfseek(container, thePatches->tocSizeEntry.offset, kCMSeekSet);
  if (CMfwrite(container, tocBuffer, sizeof(unsigned char), TOCentrySize) != TOCentrySize) {
    ERROR1(container, CM_err_BadTOCWrite, CONTAINERNAME);
    return (false);
  }

  /* Back-patch the TOC updates TOC entry, i.e., the entry that defines the offset and  */
  /* size of that portion of an updating container's TOC to be applied as updates to its*/
  /* target.  Note, this entry only exists in updating containers and we only get into  */
  /* this function it it's a new updating container.                                    */

  if (UPDATING(container)) {                                  /* if expecting patch...  */
    if ((thePatches->whatWeGot & Patch_TOCNewValuesTOC) == 0){/*must have gotten offset!*/
      ERROR1(container, CM_err_Internal5, CONTAINERNAME);
      return (false);
    }

    cmSetValueBytes(container, &thePatches->tocNewValuesTOCEntry.tocEntry.value, Value_NotImm,
                    newValuesTOCStart, newValuesTOCSize);
   omfsTruncInt64toUInt32(thePatches->tocNewValuesTOCEntry.tocEntry.value.notImm.value, &value32);
   omfsTruncInt64toUInt32(thePatches->tocNewValuesTOCEntry.tocEntry.value.notImm.valueLen, &valueLen32);
    wrPropID = thePatches->tocNewValuesTOCEntry.tocEntry.propertyID;
    wrTypeID = thePatches->tocNewValuesTOCEntry.tocEntry.typeID;
#ifdef PORT_FILESYS_40BIT
    omfsDecomposeInt64(thePatches->tocNewValuesTOCEntry.tocEntry.value.notImm.value, &upper, (omfInt32 *)&value32);
    wrPropID |= (upper << 24L) & 0xFF000000;
    omfsDecomposeInt64(thePatches->tocNewValuesTOCEntry.tocEntry.value.notImm.valueLen, &upper, (omfInt32 *)&valueLen32);
    wrTypeID |= (upper << 24L) & 0xFF000000;
#endif
    OldFormatTOC(tocBuffer, thePatches->tocNewValuesTOCEntry.tocEntry.objectID,
                         wrPropID,
                         wrTypeID,
                         value32,
                         valueLen32,
                         thePatches->tocNewValuesTOCEntry.tocEntry.generation,
                         thePatches->tocNewValuesTOCEntry.tocEntry.flags);
    CMfseek(container, thePatches->tocNewValuesTOCEntry.offset, kCMSeekSet);
    if (CMfwrite(container, tocBuffer, sizeof(unsigned char), TOCentrySize) != TOCentrySize) {
      ERROR1(container, CM_err_BadTOCWrite, CONTAINERNAME);
      return (false);
    }
  }

  /*               ********** The following must be done last! **********               */

  /* Back-patch the TOC container entry, i.e., the entry that defines the size of the   */
  /* entire container, including its label.  This entry must be done last and this      */
  /* routine must be called just prior to writing the container label.  The label is    */
  /* assumed to always immediatly follow the TOC so, even though it is not yet written, */
  /* we can take it into account in setting the size field.                             */

  if ((thePatches->whatWeGot & Patch_TOCContSize) == 0) {   /* must have gotten offset  */
    ERROR1(container, CM_err_Internal3, CONTAINERNAME);
    return (false);
  }

  size = (container->embeddedValue) ? CMGetValueSize((CMValue)container->embeddedValue)
                                    : container->physicalEOF;
  omfsAddInt32toInt64(LBLsize, &size);
  cmSetValueBytes(container, &thePatches->tocContEntry.tocEntry.value, Value_NotImm,
                  zero, size);
  omfsTruncInt64toUInt32(thePatches->tocContEntry.tocEntry.value.notImm.value, &value32);
  omfsTruncInt64toUInt32(thePatches->tocContEntry.tocEntry.value.notImm.valueLen, &valueLen32);
  wrPropID = thePatches->tocContEntry.tocEntry.propertyID;
  wrTypeID = thePatches->tocContEntry.tocEntry.typeID;
 #ifdef PORT_FILESYS_40BIT
  omfsDecomposeInt64(thePatches->tocContEntry.tocEntry.value.notImm.value, &upper, (omfInt32 *)&value32);
  wrPropID |= (upper << 24L) & 0xFF000000;
  omfsDecomposeInt64(thePatches->tocContEntry.tocEntry.value.notImm.valueLen, &upper, (omfInt32 *)&valueLen32);
  wrTypeID |= (upper << 24L) & 0xFF000000;
 #endif
  OldFormatTOC(tocBuffer, thePatches->tocContEntry.tocEntry.objectID,
                       wrPropID,
                       wrTypeID,
                       value32,
                       valueLen32,
                       thePatches->tocContEntry.tocEntry.generation,
                       thePatches->tocContEntry.tocEntry.flags);
  CMfseek(container, thePatches->tocContEntry.offset, kCMSeekSet);
  if (CMfwrite(container, tocBuffer, sizeof(unsigned char), TOCentrySize) != TOCentrySize) {
    ERROR1(container, CM_err_BadTOCWrite, CONTAINERNAME);
    return (false);
  }

  return (true);                                            /* success!                 */
}



