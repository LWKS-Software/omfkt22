/*---------------------------------------------------------------------------*
 |                                                                           |
 |                            <<< freespce.h  >>>                            |
 |                                                                           |
 |         Container Manager Free Space Value Management Interfaces          |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 4/23/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This file contains interfaces for reusing deleted data space.  The routines record the
 deleted space and reuse it for value data writes.
 
 The free space is maintained as a list of value segment entries for a "free space"
 property belonging to the TOC ID 1 entry.  It is written to the container like other ID 1
 properties to keep track of all the deleted space.
 
 If a container is opened for reusing free space, we use the free list to reuse the space.
*/


#ifndef __FREESPACE__
#define __FREESPACE__


#ifndef __CMTYPES__
#include "CMTypes.h"
#endif
#ifndef __CM_API_TYPES__
#include "CMAPITyp.h"    
#endif
#ifndef __TOCENTRIES__
#include "TOCEnts.h"   
#endif
#ifndef __CONTAINEROPS__
#include "Containr.h"  
#endif

                                  CM_CFUNCTIONS


CM_EXPORT void cmAddToFreeList(ContainerPtr container, TOCValuePtr theValueToFree,
                               CMCount offset, CMSize size);
  /*
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


CM_EXPORT CMCount cmGetFreeListEntry(ContainerPtr container, 
                                     CMSize desiredSize, Boolean mustAllFit,
                                     CMSize *actualSize);
  /*
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
  
  
CM_EXPORT CMSize32 cmWriteData(TOCValueHdrPtr theValueHdr, unsigned char *buffer, CMSize32 size);
  /*
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


CM_EXPORT void cmDeleteFreeSpace(ContainerPtr container, CMCount beyondHere);
  /*
  This routine is called to remove all free list entries that have their space beyond the
  specified offset, beyondHere.  All entries with starting offsets greater than or equal
  to beyondHere are removed.  If one spans beyondHere it will be reduced appropriately.
  
  These entries are removed so so that we may overwrite from beyondHere with a new TOC.
  Since we are reusing that space for the TOC we obviously can't keep free list entries for
  it.
  */
  

                              CM_END_CFUNCTIONS
#endif




