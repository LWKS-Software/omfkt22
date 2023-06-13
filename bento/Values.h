/*---------------------------------------------------------------------------*
 |                                                                           |
 |                          <<<    values.h     >>>                          |
 |                                                                           |
 |             Container Manager Common Value Routines Interfaces            |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  7/22/92                                  |
 |                                                                           |
 |                     Copyright Apple Computer, Inc. 1992                   |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This file contains the interfaces to the various routines needed for value operations,
 e.g., CMWriteValueData() CMDeleteValueData(), etc.  Some of these routines are used when
 applying updates at open time (insert and delete data, move a value header).
*/

#ifndef __VALUEROUTINES__
#define __VALUEROUTINES__


#ifndef __CMTYPES__
#include "CMTypes.h"
#endif
#ifndef __CM_API_TYPES__
#include "CMAPITyp.h"    
#endif

struct TOCValueHdr;
struct TOCValue;
struct TOCObject;


                                  CM_CFUNCTIONS


struct TOCValue *cmGetStartingValue(struct TOCValueHdr *theValueHdr,
                                    CMCount startingOffset, 
                                    CMCount *valueOffset);
  /*
  This returns the TOCValuePtr to the value entry which contains the stream starting 
  offset, startingOffset.  Also returned in valueOffset is the offset within the returned
  value entry which is the startingOffset'th byte.
  
  Note, NULL is returned for the value pointer if we cannot find the offset. This could 
  happen when the startingOffset is beyond the end of the value.
  
  This routine is necessary because of continued values which are represented by a list of
  TOCValue entries off of a TOCValueHdr.  Each entry represents a discontinous segment of
  the value data which is always viewed as a stream of contiguous bytes even though they 
  are not.  This routine is one of those that allows its caller to view the stream as
  contiguous.
  */
  

CM_EXPORT CMSize32 cmRead1ValueData(struct TOCValue *theValue, unsigned char *buffer,
                                    CMCount offset,CMSize32 maxSize);
  /*
  This routine copies the data for the specified value (NOT a value header -- continued
  values are not worried about here), starting at the specified offset, TO the caller's
  buffer.  A maximum of maxSize characters or the size of the data, which ever is smaller,
  is copied.  The function returns the amount of data copied to the buffer.
  
  Note, this routine handles the special cases for immediate data.
  */


CM_EXPORT CMSize32 cmOverwrite1ValueData(struct TOCValue *theValue, unsigned char *buffer,
                                         CMCount offset, CMSize32 size);
  /*
  This routine copies the data for the specified value (NOT a value header -- continued
  values are not worried about here), starting at the specified offset, FROM the caller's
  buffer. A maximum of size characters are overwritten to the value.  The function returns
  the amount of data copied from the buffer to the value.
  
  Note, overwriting of global names is NOT allowed and assumed not passed to this routine.
  Immediates are, however, handled and can be overwritten up to their existing size.
  */
  
  
CM_EXPORT Boolean cmConvertImmediate(struct TOCValue *theValue);
  /*
  This routine is called whenever an immediate data value must be converted to a
  non-immediate.  It takes as parameters the pointer to the immediate value and returns
  true if there are no errors.  On return the value will have been converted to a
  non-immediate and the input segment CMValue changed to container the offset to the 
  now written value.
  */
  
  
CM_EXPORT struct TOCValue *cmInsertNewSegment(struct TOCValueHdr *theValueHdr,
                                              struct TOCValue *insBeforeValue,
                                              CMCount segOffset, CMCount dataOffset,
                                              CMSize size);
  /*
  This routine is called by CMInsertValueData() to create a new value segment insert to be
  inserted into a previously existing value.  It is also called when applying data insert
  updates at open time (by the internal routine applyValueUpdateInstructions() in 
   Update.c ).  The new insert data is already written to the (updating) container, and the
  data is at container offset dataOffset.  It is size bytes int .  The insert is to be at
  the specified segOffset within the value segment insBeforeValue. 
  
  The function returns a pointer to the newly created insert segment, or NULL if there is
  an error and the error reporter returns.

  Note, this routine is for non-immediate data insertions only.  CMInsertValueData() 
  handles insertions on immediates.  For updating, immediates are handled differently and
  thus can't get in here.
  */
  
  
CM_EXPORT void cmDeleteSegmentData(struct TOCValueHdr *theValueHdr, CMCount startOffset,
                                   CMCount endOffset);
  /*
  This routine is called by CMDeleteValueData() to delete value data  It is also called
  when applying data deletion updates at open time (by the internal routine
  applyValueUpdateInstructions() in  Update.c ).  All the data starting at offset 
  startOffset, up to and including the end offset, endOffset, are to be deleted.  If the
  start and end offsets span entire value segments, those segments are removed.
 
  Note, this routine is for non-immediate data deletions only.  CMDeleteValueData() handles
  deletions on immediates.  For updating, immediates are handled differently and thus can't
  get in here.
  */
  
  
CM_EXPORT struct TOCValueHdr *cmMoveValueHdr(struct TOCValueHdr *theFromValueHdr, CMObject object,
                                             CMProperty property);
  /*
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
  

                              CM_END_CFUNCTIONS
#endif
