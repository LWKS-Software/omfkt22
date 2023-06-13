/*---------------------------------------------------------------------------*
 |                                                                           |
 |                            <<<  TOCEnts.h   >>>                           |
 |                                                                           |
 |                   Container Manager TOC Entry Interfaces                  |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 12/02/91                                  |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1991-1992                 |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This file contains the interfaces to the manipulation routines to handle TOC objects
 below the object level.  The objects and the means to get at them are maintained in
 TOCObjects.c.  The split is done this way to keep the accessing of objects separate from
 the stuff represented by those objects.  See  TOCObjs.c   for complete details.
*/

#ifndef __OLDTOCENTRIES__
#define __OLDTOCENTRIES__


#include <setjmp.h>
#include <stdio.h>

/*------------------------------------------------------------------------------------*
 | W A R N I N G - - -> this header as well as  TOCObjs.h   and  TOCEnts.h   all have |
 |                      mutual references.  The include order is NOT arbitrary.  It's |
 |                      the only one that works!  Even with it we have to do some     |
 |                      forward struct pointer references of the form "struct foo *p" |
 *------------------------------------------------------------------------------------*/

#ifndef __CMTYPES__
#include "CMTypes.h"
#endif
#ifndef __CM_API_TYPES__
#include "CMAPITyp.h"    
#endif
#ifndef __TOCOBJECTS__
#include "TOCObjs.h"   
#endif
#ifndef __CONTAINEROPS__
#include "Containr.h"  
#endif
#ifndef __DYNAMICVALUES__
#include "DynValus.h"     
#endif
#ifndef __GLOBALNAMES__
#include "GlbNames.h"   
#endif
#ifndef __BUFFEREDIO__
#include "BufferIO.h"  
#endif
#ifndef __UPDATING__
#include "Update.h"  
#endif
#ifndef __LISTMGR__
#include "ListMgr.h"
#endif

struct TOCProperty;
struct TOCValueHdr;
struct DynValueHdrExt;
                        /*-------------------------------*
                         | Defined Layout of a TOC Entry |
                         *-------------------------------*
 
As defined by the API documentation, a container TOC entry has the following layout:
 
      0           4           8           12          16          20    22    24
     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     | Object ID |Property ID|  Type ID  |Valu Offset| Value Len | Gen |Flags|
     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
           4           4           4           4           4        2     2

These are defined as "hard-coded" constants since the format is, by definition, platform
independent.  The actual formatting and I/O for a TOC entry is done by a handler because
of this.                                                                                */

#define TOCentryObjectID     0        /* 4 bytes: the object's ID                       */
#define TOCentryPropertyID   4        /* 4        its property ID                       */
#define TOCentryTypeID       8        /* 4        its type ID                           */
#define TOCentryValueOffset 12        /* 4        offset to its value                   */
#define TOCentryValueLength 16        /* 4        size of the value                     */
#define TOCentryGeneration  20        /* 2        generation number                     */
#define TOCentryFlags       22        /* 2        flags                                 */
           
#define TOCentrySize        24        /* defined size of a TOC entry in its container   */

/* The following macros allow portable (I hope) access to a TOC's entry fields.  There  */
/* are two sets of macros; one set for input, and one for output.  Each set consists of */
/* two definitions; one when using buffered I/O and the other when calling a handler    */
/* directly.  The code is configurable (CMConfig.h) to do either.  But in some cases    */
/* we ignore the configuration, so all definitions must be defined here.  The goal here */
/* is to define in THIS ONE PLACE all the dependencies on the actual TOC format.  If it */
/* changes, then theoritically all we need to do is change this stuff here.  Cross your */
/* fingers!                                                                             */

/* The first pair of macros are for extracting the TOC fields.  In the buffered case the*/
/* fields are extracted directly out of the buffer.  In the nonbuffered case, the buffer*/
/* is supplied to the macro to extract the fields.                                      */

/*#define BufferedExtractTOC(ioBuffer, objectID, propertyID, typeID, value, valueLen, generation, flags) \
          {objectID   = (unsigned int )GET4(ioBuffer);  \
           propertyID = (unsigned int )GET4(ioBuffer);  \
           typeID     = (unsigned int )GET4(ioBuffer);  \
           value      = (unsigned int )GET4(ioBuffer);  \
           valueLen   = (unsigned int )GET4(ioBuffer);  \
           generation = (unsigned short)GET2(ioBuffer); \
           flags      = (unsigned short)GET2(ioBuffer);}
*/

/* get flags first, since value is needed to figure out offset */

/*#define ExtractTOC(tocBuffer, objectID, propertyID, typeID, value, valueLen, generation, flags) \
          {\
           CMextractData(container, (char *)tocBuffer + TOCentryFlags,       2, &flags);\
           CMextractData(container, (char *)tocBuffer + TOCentryObjectID,    4, &objectID);     \
           CMextractData(container, (char *)tocBuffer + TOCentryPropertyID,  4, &propertyID);   \
           CMextractData(container, (char *)tocBuffer + TOCentryTypeID,      4, &typeID);       \
           CMextractData(container, (char *)tocBuffer + TOCentryValueLength, 4, &valueLen);     \
	   if ((flags & kCMImmediate) && valueLen <= 4) memcpy(&value, (char *)tocBuffer + TOCentryValueOffset, 4); else  \
                CMextractData(container, (char *)tocBuffer + TOCentryValueOffset, 4, &value);   \
           CMextractData(container, (char *)tocBuffer + TOCentryGeneration,  2, &generation);   \
          }
*/

/* The second pair of macros is for formatting the TOC fields.  In the buffered case the*/
/* fields are placed directly into the buffer.  In the nonbuffered case, the supplied   */
/* buffer is correctly formatted (no -- "I think I'll do it incorrectly", he said with a*/
/* smerk).                                                                              */

#define BufferedFormatTOC(ioBuffer, objectID, propertyID, typeID, value, valueLen, generation, flags) \
          {PUT4(objectID,   ioBuffer);  \
           PUT4(propertyID, ioBuffer);  \
           PUT4(typeID,     ioBuffer);  \
           PUT4(value,      ioBuffer);  \
           PUT4(valueLen,   ioBuffer);  \
           PUT2(generation, ioBuffer);  \
           PUT2(flags,      ioBuffer);}
           
#define OldFormatTOC(tocBuffer, objectID, propertyID, typeID, value, valueLen, generation, flags)  \
          {CMformatData(container, (char *)tocBuffer + TOCentryObjectID,    4, &objectID);      \
           CMformatData(container, (char *)tocBuffer + TOCentryPropertyID,  4, &propertyID);    \
           CMformatData(container, (char *)tocBuffer + TOCentryTypeID,      4, &typeID);        \
           if ((flags & kCMImmediate) && (valueLen <= 4) )\
               memcpy((char *)tocBuffer + TOCentryValueOffset, &value, 4);\
           else \
               CMformatData(container, (char *)tocBuffer + TOCentryValueOffset, 4, &value);     \
	   CMformatData(container, (char *)tocBuffer + TOCentryValueLength, 4, &valueLen);      \
           CMformatData(container, (char *)tocBuffer + TOCentryGeneration,  2, &generation);    \
           CMformatData(container, (char *)tocBuffer + TOCentryFlags,       2, &flags);}


#ifdef __cplusplus
extern "C" {
#endif



CM_EXPORT Boolean cmOldWriteTOC(ContainerPtr container, void *toc, CMObjectID startingID,
                                CMObjectID endingID, BackPatchesPtr thePatches,
                                CMCount *tocStart, CMCount *tocSize);
  /*
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


CM_EXPORT Boolean cmOldDoBackPatches(ContainerPtr container, BackPatchesPtr thePatches,
                                     CMCount tocStart, CMSize tocSize,
                                     CMCount newValuesTOCStart, CMSize newValuesTOCSize);
  /*
  When a TOC is written (e.g., cmWriteTOC()), there are some entries that are dependent on
  the final size of the TOC.  Such entries must be back-pached with the proper info.  Only
  the callers generating the TOC in the container know when it is time to do the patching.
  They call this routine to do it when that time comes.
  
  True is returned if the back-patching is successful and false otherwise.
  
  By the time this routine is called, the original TOC entries in question and their
  offsets within the container have been recorded in a BackPatches struct whose pointer is
  passed in thePatches.  Currently only 3 such entries need back-patching:
  
  tocSizeEntry         -  The TOC size entry.  This is a property of TOC #1 which
                          represents the size and offset of the TOC itself.  The caller
                          passes this info in the tocSize and tocStart parameters
                          respectively.
                      
  tocContEntry         -  The TOC container entry. This is a property of TOC #1 which
                          represents the entire container, from offset 0 to the end of the
                          label.  No additional parameters are passed for this.  But to
                          compute this, it is required that cmDoBackPathes() be the LAST
                          thing called prior to writing the container label.
   
  tocNewValuesTOCEntry - The non-private TOC offset/size entry.  An updating TOC contains
                         a TOC #1 property that defines the offset and size of all TOC
                         entries that are to be applied to this container's target.  The
                         caller passes this info in the newValuesTOCSize and
                         newValuesTOCStart parameters respectively.  For non-updating
                         containers, these are ignored.
  */
  
  

#ifdef __cplusplus
}
#endif

#endif
