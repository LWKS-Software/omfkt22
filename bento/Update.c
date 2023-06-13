/*---------------------------------------------------------------------------*
 |                                                                           |
 |                             <<<  Update.c  >>>                            |
 |                                                                           |
 |              Container Manager Container Updating Routines                |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  7/10/92                                  |
 |                                                                           |
 |                     Copyright Apple Computer, Inc. 1992                   |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file includes almost all the "incremental update" processing routines when one
 updating container updates its target container.  The routines in here are responsible
 for maintaining the "touched chain" and "touched lists". These are additional data
 structures layered on to the main data structures (see TOCEntries.[hc]) as described in
 the following (abbreviated) diagram:


                                *---------*
                                |   CCB   |    Touched Chain
                                |         |--------->---------+
                                *---------*                   |
                                                              |
                                                              |
                   +------------<-------------+               |
                   |                          |               |
          *----*<--+                          +------*----*<--+
          | O1 |          *-----*                    | O2 |          *-----*
          |    |--------->| P11 |- - ->              |    |--------->| P21 |- - ->
          *----*          *-----*                    *----*          *-----*
            |                |                         |                |
       Touched List          |                    Touched List          |
            |            *-------*                     |            *-------*
       *---------*<--+   |VH(T11)|                *---------*<----->|VH(T21)|
       |O1 P1 T12|   |   *-------*                |O2 P2 T21|       *-------*
       | [flags] |   |       |                    | [flags] |           |
       *---------*   |       |                    *---------*           |
            |        |   *-------*                     |            *-------*
            |        +-->|VH(T12)|                     |            |VH(T22)|
            |            *-------*                     |            *-------*
       *---------*           |                    *---------*<--+       |
       |O1 P1 T13|           |                    |O1 P1 T12|   |       |
       | removed |       '''''''''                |inserted |   |   *-------*
       *---------*       'VH(T13)'                *---------*   +-->|VH(T13)|
                         '''''''''                                  *-------*
                             .                                          .
                             ............................................


 This diagram shows parts of two objects, O1 and O2, with at least one property each, P11
 and P12 respectively. Each property has two value (headers) of their own of the indicated
 types.  The value segments and all links details for this stuff are not shown.  Value
 header VH(T13) is shown moved from O1/P11 to O2/P21.

                                The "Touched Chain"

 This is a singly linked chain of all objects (through the TOCObjects) that are "touched"
 during the updating session. The head of this chain is contained in the Container Control
 Block (CCB).  The order of updating recording does not matter, so the most convenient way
 to link this list is backward.  Thus in the above diagram object O1 was touched before O2.

                                The "Touched List"

 This is a doubly linked list whose header is in the object (TOCObject).  A doubly linked
 list is used to make it easy to remove or move these entries.  List entries are generated
 for each value updated.  There is only one list entry per value no matter how many times
 the corresponding value is updated.  The value header for an updated value points back to
 its touched list entry.  This pointer is initially NULL and doubles as a "first touched"
 switch.

 Each touched list entry contains the following:

 (a). The original object, property, and type IDs which we refer to as the "OPT address".
      The OPTs will work their way into the updating instructions generated at close time
      for use in addressing the original values at open time.  They uniquely address the
      original value prior to applying any updates to properly apply those updates.

 (b). A pointer to the value header that was touched.  Thus the touched list entry and its
      value header point to each other.

 (c). A "back pointer" to an initial touched list entry for moved values.  The use of this
      pointer is discussed later.  For unmoved touched values, this pointer is NULL.

 (d). Flags to indicate special conditions of the touched entry.  These conditions are:

      -> "Removed"            The value has been moved to another object. The touched list
                              entry for the original "from" object's value is created or
                              flagged as "removed".

      -> "Inserted"           The value has been moved from another or the same object.  A
                              touched list entry for the destination object is created and
                              marked as "inserted". If it is the same object and previously
                              touched, the touched list entry is changed to "inserted".

      -> "Deleted Value"      The value has been deleted.

      -> "Deleted Property"   The property specified by address "OP" has been deleted.

 Except for the "deleted property" flag, all flags deal with moved values.

 The only things not in this file that deal with updating are the open-time and close-time
 processing dealing with connecting and detaching the updating container from its target.
 Most everything else is here.

 Enjoy...
*/

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>

#ifndef __CMTYPES__
#include "CMTypes.h"
#endif
#ifndef __CM_API_TYPES__
#include "CMAPITyp.h"
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
#ifndef __UPDATING__
#include "Update.h"
#endif
#ifndef __BUFFEREDIO__
#include "BufferIO.h"
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

#if USE_UPDATE_MODE
                                  CM_CFUNCTIONS

/* The following generates a segment directive for Mac only due to 32K Jump Table       */
/* Limitations.  If you don't know what I'm talking about don't worry about it.  The    */
/* default is not to generate the pragma.  Theoritically unknown pragmas in ANSI won't  */
/* choke compilers that don't recognize them.  Besides why would they be looked at if   */
/* it's being conditionally skipped over anyway?  I know, famous last words!            */

#if CM_MPW
#pragma segment Updating
#endif


/*---------------------------------------------*
 | File Organization (yes, this file has one!) |
 *---------------------------------------------*

 To aid those reading this file, and who aren't using MPW and its marker facility (if you
 don't know what markers are keep reading) the following list of function names summarizes
 the grouping organization in thei file.  This may make it easier to see the breakdown of
 this file to understand it.

 Miscellaneoush utilities
 ------------------------
   cmAddObjToTouchedChain       - add an object on to the container's touched chain
   deleteObjFromTouchedChain    - delete an object from the container's touched chain
   createTouchedListEntry       - create a touched list entry for a value
   cmDeleteTouchedList          - delete entire touched list for an object

 Simple Touches (touch list creation)
 ------------------------------------
   simpleValueTouch             - generate touched list entry for a "simple" touch
   cmTouchEditedValue           - generate a touched list entry for an edited value
   cmTouchImmediateValue        - generate a touched list entry for a modified immediate
   cmTouchBaseType              - generate a touched list entry for a modified base type
   cmTouchSetInfoedValue        - generate a touched list entry for a "set-infoed" value

 Complex Touches (touch list creation)
 -------------------------------------
   cmTouchMovedValue            - generate touched lists for a value being moved
   cmTouchDeletedValue          - generate touched list entry for value being deleted
   cmImplicitDeleteValueTouch   - delete touched list entry for an implicitly deleted value
   cmTouchDeletedProperty       - generate a touched list entry for a deleted property
   cmTouchDeletedObject         - generate a touched list entry for a deleted object

 Update Instruction Generation (output)
 --------------------------------------
   generateDataEdits            - generate insert/delete (data editing) instructions
   genValueUpdateInstructions   - generate value instructions
   generateValueUpdates         - main control generate value instructions
   generateDeleteUpdates        - generate object and property delete instructions
   cmGenerateUpdates            - main control for update instruction generation

 Update Instruction Interpretation (input)
 -----------------------------------------
   applyValueUpdateInstructions - value updating instruction interpreter
   applyValueUpdates            - main control interpreter to apply value updating instructions
   applyDeleteUpdates           - interpreter to apply object/property deletion instructions
   cmApplyUpdates               - apply all updates to bring a target "up to date"
*/


/*------------------------*
 | Updating Control Codes |
 *------------------------*/

/* The following define the control code values generated in the updating instructions  */
/* written to the container.  See code that generates the actual data for the exact     */
/* instruction format layout.                                                           */

#define EndUpdates      0xFFU               /* end of updating instruction sequence     */

                                            /* ***The following are main control codes***/
#define NewProperty        1U               /* new property (type follows)              */
#define NewType            2U               /* new type, same prop. (value ctl follows) */
#define DeleteObject       3U               /* delete entire object                     */
#define DeleteProperty1    4U               /* delete property within object            */
#define DeleteProperty2    5U               /* delete property for current object       */

                                            /* ***The following are value update codes***/
#define RemovedValue       6U               /* remove value from object                 */
#define InsertedValue      7U               /* insert a value from somplace into object */
#define SetInfoedValue     8U               /* change type and generation               */
#define DeleteData         9U               /* delete data                              */
#define InsertData1       10U               /* insert data (one segment)                */
#define InsertDataN       11U               /* insert data (N segments)                 */
#define ReplaceImmediate  12U               /* replace with new immediate data          */
#define ReplaceBaseType   13U               /* replace with new base type data          */
#define DeleteValue       14U               /* delete value                             */

#define ErrorCtl          15U               /* used only for error recover during input */


/*----------------------------------------------------------------------------*
 | cmAddObjToTouchedChain - add an object on to the container's touched chain |
 *----------------------------------------------------------------------------*

 The TOCObject pointed to by theObject is "touched", i.e., it is put on the updating
 container's singly linked chain of touched objects, the touchedChain.

 The container is passed because the touched list has to be attached to a "know fixed"
 container.  The fixed container is usually the updatingContainer.  But this needs to be
 "controlled" during open time because the updatingContainer is also used define the
 "owner" of NEW objects read in during TOC loading.  The updatingContainer then is the
 container doing the loading, NOT the original container the user opened.

 cmReadTOC(), who loads the TOC, calls this routine (which is why it is "exposed" outside
 this file) to add objects to the touched chain that have the special "updates" property.
 Such objects have associated updating instructions (generated by generateValueUpdates()
 at close time). These must be chained together until the TOC is fully read in.  This is
 because "inserted" (i.e., move) updates could refer to objects not yet  read in.  The
 touched chain is a convenient place!  So we use it and this routine to build the chain.
 This chain is only temporary.  It is "fed" to cmApplyUpdates() who walks the chain to
 apply all the updates. The chain is then immediately cleared ready to receive new updates
 in the "normal" way.

 Note, the object is placed on the touched chain only once.  This routine is, however,
 called whenever we think we need to touch the object.  We worry about the multiple
 touches here.
*/

void cmAddObjToTouchedChain(ContainerPtr container, TOCObjectPtr theObject)
{
  if ((theObject->objectFlags & TouchedObject) == 0) {      /* if this is 1st touch...  */
    theObject->objectFlags |= TouchedObject;                /* ...mark object as touched*/
    theObject->nextTouchedObject = container->touchedChain; /* ...link object into chain*/
    container->touchedChain      = theObject;
  }
}

/*---------------------------------------------------------------------------------*
 | deleteObjFromTouchedChain - delete an object from the container's touched chain |
 *---------------------------------------------------------------------------------*

 The TOCObject pointed to by theObject is "untouched", i.e., it is removed from the
 updating container's touched chain.  The pointer to the previous object on this chain is
 also passed.  If it is NULL, then the previous value is assumed to be the touchedChain
 list header in the updating container.

 The container is passed as an explicit parameter for the same reason it is in
 cmAddObjToTouchedChain().  See its comments for details.

 Note, the object is removed only once.  A untouched object has its touched chain link
 set to NULL.  This routine resets the link to NULL and makes sure we don't try to unlink
 it twice.

 Just as a reminder in case it's not clear how to use this thing, in particular, how to
 manage thePrevObject...

 The touched chain is a singly linked list whose header, touchedChain, is in the container
 control block.  If an object is to be removed from from the chain, thePrevObject should
 KEEP its current value.  This could be NULL as int  as the entries starting with the
 first continue to be deleted.  As soon as one is retained, then thePrevObject should be
 set to point at that one.

 In summary, don't change thePrevObject if this routine is called, and set thePrevObject
 to each retained object at the time it's encountered.  Enough said...
*/

static void CM_NEAR deleteObjFromTouchedChain(ContainerPtr container,
                                              TOCObjectPtr theObject,
                                              TOCObjectPtr thePrevObject)
{
  if (theObject->objectFlags & TouchedObject) {              /*if object was touched... */
    if (thePrevObject == NULL)                               /* ...unlink it from chain */
      container->touchedChain = theObject->nextTouchedObject;
    else
      thePrevObject->nextTouchedObject = theObject->nextTouchedObject;

    theObject->nextTouchedObject = NULL;                     /* ...(un)mark as touched  */
    theObject->objectFlags      &= ~TouchedObject;
  }
}


/*-------------------------------------------------------------------*
 | createTouchedListEntry - create a touched list entry for a value  |
 *-------------------------------------------------------------------*

 A touched list entry for the value's object is created for the first touch of the
 specified value (header).  The value header is set to point at the touched list entry and
 vice versa.  The original object, property, and type ID (the "OPT address") are saved in
 the touched list entry as well.  The touchFlags are initialized to 0 and must be set by
 the caller.

 The function returns the pointer to the value's corresponding touched list entry.  NULL
 is returned, and an error reported if there is an allocation error for a new touched
 list entry.

 Note, if there were no previous touch list entries for the object, the object is put on
 the updating container's touched chain.  If the value was previously touched, the pointer
 to the already existing touched list entry is returned.

 Also note, this routine is only for creating touched list entries for values (headers).
 The case for a deleted property touch is handled separately in cmTouchDeletedProperty().
*/

static TouchedListEntryPtr CM_NEAR createTouchedListEntry(TOCValueHdrPtr theValueHdr)
{
  ContainerPtr        container;
  TOCObjectPtr        theObject;
  TouchedListEntryPtr touch;

  if (theValueHdr->touch != NULL)               /* if not first touch...                */
    return (theValueHdr->touch);                /* ...return corresponding touched entry*/

  container = theValueHdr->container->updatingContainer; /* allocate new touched entry  */

  if ((touch = (TouchedListEntryPtr)CMmalloc(container, sizeof(TouchedListEntry))) == NULL) {
    ERROR1(container,CM_err_NoTouchedEntry, CONTAINERNAME);
    return (NULL);
  }

  theValueHdr->touch = touch;                   /* point hdr at its touched entry       */
  theObject = theValueHdr->theProperty->theObject;

  cmNullListLinks(touch);                       /* init touched list entry fields...    */
  touch->theValueHdr  = theValueHdr;
  touch->objectID     = theObject->objectID;
  touch->propertyID   = theValueHdr->theProperty->propertyID;
  touch->typeID       = theValueHdr->typeID;
  touch->touchFlags   = 0;
  touch->removedEntry = NULL;

  cmAddObjToTouchedChain(container, theObject); /* make sure obj. is on touched chain   */

  cmAppendListCell(&theObject->touchedList, touch); /* add touch to the object          */

  return (touch);                               /* return ptr to new touch list entry   */
}


/*----------------------------------------------------------------*
 | cmDeleteTouchedList - delete entire touched list for an object |
 *----------------------------------------------------------------*

 This routine is only used when all TOC data structures are being freed.  For each object
 passed, that object's entire touched list (if any) is freed.  Since all TOC entities are
 being freed, there is no need to remove the object from the touched chain since the
 objects on the chain will be freed.

 Note, that the object's passed are just objects picked up during the freeing of all the
 objects.  Therefore the touched chain is not being followed in the context used here.
*/

void cmDeleteTouchedList(TOCObjectPtr theObject)
{
  TouchedListEntryPtr nextTouch, touch;
  ContainerPtr        container;

  if (theObject->objectFlags & TouchedObject) {     /*if object was touched...          */
    container = theObject->container;               /* needed for CMfree()              */
    theObject->objectFlags &= ~TouchedObject;       /* ...unmark just to be safe!       */
    touch = (TouchedListEntryPtr)cmGetListHead(&theObject->touchedList);

    while (touch != NULL) {                         /* ...loop through entire touch list*/
      nextTouch = (TouchedListEntryPtr)cmGetNextListCell(touch);
      container = theObject->container;
      CMfree(container, cmDeleteListCell(&theObject->touchedList, touch)); /* ...free each entry   */
      touch = nextTouch;
    } /* while */
  } /* TouchedObject */
}


/*-------------------------------------------------------------------------------------*
 | simpleValueTouch - generate touched list entry for a value touched in specified way |
 *-------------------------------------------------------------------------------------*

 This routine defines a touched list entry for a value. A touched list entry for the object
 "owning" the value is defined.  It is the common internal routine to cmTouchEditedValue(),
 cmTouchImmediateValue(), cmTouchBaseType(), and cmTouchSetInfoedValue().  These are all
 "simple" touches in the sense that the touched list entry is or'ed with a flag appropriate
 to the caller.  The flag is passed as the parameter.  The touched list entry for the
 object "owning" a value is defined if necessary.

 The function returns true if everything went ok and false otherwise.  This can only fail
 because of an allocation error creating a touched list entry.
*/

static Boolean CM_NEAR simpleValueTouch(TOCValueHdrPtr theValueHdr, unsigned short flag)
{
  ContainerPtr        container = theValueHdr->container->updatingContainer;
  TouchedListEntryPtr touch;

  /* Don't create touched list entries if we're not updating or we're processing a new  */
  /* value header...                                                                    */

  if (!TouchIt(container, theValueHdr->container))
    return (true);

  /* If the value has never been touched, create a "modified" touched list entry for it */

  touch = theValueHdr->touch;                         /* not NULL if previously touched */

  if (touch == NULL) {                                /* if this is the first touch...  */
    touch = createTouchedListEntry(theValueHdr);      /* ...create "modified" entry     */
    if (touch == NULL) return (false);
  }

  touch->touchFlags |= flag;                          /* OR in the appropriate flag     */

  return (true);                                      /* return success                 */
}


/*------------------------------------------------------------------------*
 | cmTouchEditedValue - generate a touched list entry for an edited value |
 *------------------------------------------------------------------------*

 This routine defines a touched list entry for a value when it is edited.  A touched list
 entry for the object "owning" the value is defined.  The parameter specifies the value
 header being edited. The function returns true if everything went ok and false otherwise.
 This can only fail because of an allocation error creating a touched list entry.
*/

Boolean cmTouchEditedValue(TOCValueHdrPtr theValueHdr)
{
  return (simpleValueTouch(theValueHdr, TouchedEdited));  /* touch as "edited"          */
}


/*--------------------------------------------------------------------------------------*
 | cmTouchImmediateValue - generate a touched list entry for a modified immediate value |
 *--------------------------------------------------------------------------------------*

 This routine defines a touched list entry for an immediate value when it is edited.  A
 touched list entry for the object "owning" the value is defined.  The parameter specifies
 the value header of the immediate value being edited.  The function returns true if
 everything went ok and false otherwise. This can only fail because of an allocation error
 creating a touched list entry.
*/

Boolean cmTouchImmediateValue(TOCValueHdrPtr theValueHdr)
{
  return (simpleValueTouch(theValueHdr, TouchedImmediate)); /* touch as "immediate"     */
}


/*--------------------------------------------------------------------------*
 | cmTouchBaseType - generate a touched list entry for a modified base type |
 *--------------------------------------------------------------------------*

 This routine defines a touched list entry for an base type when it is changed.  A touched
 list entry for the object "owning" the base type (value) is defined.  The parameter
 specifies the value header of the base type value being changed.  The function returns
 true if everything went ok and false otherwise.  This can only fail because of an
 allocation error creating a touched list entry.
*/

Boolean cmTouchBaseType(TOCValueHdrPtr theValueHdr)
{
  return (simpleValueTouch(theValueHdr, TouchedBaseType));  /* touch as "base type"     */
}


/*--------------------------------------------------------------------------------*
 | cmTouchSetInfoedValue - generate a touched list entry for a "set-infoed" value |
 *--------------------------------------------------------------------------------*

 This routine defines a touched list entry for an value when its type or generation number
 is changed (i.e., it's "set-infoed").  A touched list entry for the object "owning" the
 value is defined. The parameter specifies the value header of the value being set-infoed.
 The function returns true if everything went ok and false otherwise.  This can only fail
 because of an allocation error creating a touched list entry.

 Warning: The call to this routine MUST be done before any changes to the type because the
          touch list info is in terms of the original object, property, and type.
*/

Boolean cmTouchSetInfoedValue(TOCValueHdrPtr theValueHdr)
{
  return (simpleValueTouch(theValueHdr, TouchedSetinfoed)); /* touch as "set-infoed"    */
}


/*----------------------------------------------------------------------------*
 | cmTouchMovedValue - generate touched lists for a value that is being moved |
 *----------------------------------------------------------------------------*

 This routine defines touched list entries for a value move when a CMMoveValue() is done.
 Touched lists entries for the "from" and "to" (source and destination) objects involved
 in the moved value are defined.  The parameters specify the value header to be moved
 (theFromValueHdr), the source ("from") TOCObject (theFromObject), the destination ("to")
 TOCObject (theToObject), and destination property ID (toPropertyID).  The function
 returns true if everything went ok and false otherwise.  This can only fail because of an
 allocation error creating a touched list entry.

 Warning: The call to this routine MUST be done before the value is moved because the
          touch list info is in terms of the original object, property, and type.  The
          move will obviously change the object and/or property.

 Note, that a move only moves the value header passed in here.  The segments "go along for
 the ride".  By moving the value header, the user's refNum to it remains valid.

 The source and destination object's touched lists are modified "appropriately".  The
 algorithm is sufficiently complex as to warrent outlining here.  Indeed, it is
 sufficiently complex, that the only way to describe it is as a "finite state machine".
 The following is a "state-transition" table describing the FSM.  Read states on the left,
 conditions across the top.  The intersections are in the form "X/s".  This says, "execute
 action X and go to state number s".  The special action "NOP" means just go to the
 indicated state.

                                         CMMoveValue(from current location to ...)
             "State"
         Current Location         |Orig O,Orig P|Orig O,Diff P|Same O,Diff P|   Diff. O   |
 =================================|=============|=============|=============|=============|
 Initial state                  0 |    NOP/0    |     A/1     |     A/1     |     B/2     |
 Orig. Obj., different Property 1 |     C/0     |    NOP/1    |    NOP/1    |     D/2     |
 Different 0bject               2 |     E/0     |     F/1     |    NOP/2    |     G/2     |
 ==========================================================================================

 Note, in this table, "O" and "P" stand for object and property respectively.  Also,
 "Orig O" and "Same O" are identical while in states 0 or 1.  Here are the actions:

  A: if (no "from" touched list entry) create it
     flag "from" touched list entry as "inserted"

  B: if (no "from" touched list entry) create it
     flag "from" touched list entry as "removed"
     create "to" touched list entry as "inserted" (set value hdr to point to this)
     set back ptr in "to" "inserted" entry to point to "from" "removed" entry

  C: remove "inserted" flag from "from" touched list entry ("from" == "to" object)
     if (touched value not edited or set-infoed)
       delete "from" touched list entry

  D: move "from" touched list ("inserted") entry to "to" object
     create "from" touched list entry as "removed"
     set back ptr in "to" "inserted" entry to point to "from" "removed" entry

  E: move "from" touched list ("inserted") entry to "to" (original) object
     delete "removed" entry in "to" touched list
     remove "inserted" flag from "from" touched list entry
     if (touched value not edited or set-infoed)
       delete "from" touched list entry

  F: move "from" touched list ("inserted") entry to "to" (original) object
     delete "to" "removed" touched list entry

  G: move "from" touched ("inserted") list entry to "to" object

 State 0 occurs when an object has never moved (or was moved back to it's original
 position).  This is the initial state.  There could still be a touched list entry for
 the value if it was edited or set-infoed.

 State 1 is used when an value is moved to or within the same original object.  This
 can be from another object (moved there from the original object by an earlier move)
 within the original object, but from a different property (again from an earlier move).

 State 2 is entered when a value is moved to a different object.

 The general "gist" of this whole thing is to generate a "removed" entry at the original
 source and an "inserted" at the destination.  The "removed" entry is suppressed when
 the "inserted" is for the same original object.  Both "removed" and "inserted" are
 suppressed when a value is moved back to its original object and property.

 Note, an actual state number is not maintained in the touched list entries.  Instead we
 infer the state by current conditions.
*/

Boolean cmTouchMovedValue(TOCValueHdrPtr theFromValueHdr, TOCObjectPtr theFromObject,
                          TOCObjectPtr theToObject, CMObjectID toPropertyID)
{
  ContainerPtr        container = theFromValueHdr->container->updatingContainer;
  TouchedListEntryPtr fromTouch, toTouch;
  Boolean             moveWithinSameObject;

  /* Don't create touched list entries if we're not updating or we're processing a new  */
  /* value header...                                                                    */

  if (!TouchIt(container, theFromValueHdr->container))
    return (true);

  /* We can tell if we're in the initial state (0) by whether the value was ever        */
  /* previously touched.  If it wasn't we know it's the first touch.  If it was, then   */
  /* if the touched list entry is not flagged as "inserted" we still have the initial   */
  /* move state.  It is touched because it was edited or set-infoed.                    */

  /* Actions "A" and "B" occur here; a "from" touched list entry is created and flagged */
  /* as "inserted" or "removed".  If "removed", a "to" "inserted" touch list entry is   */
  /* created.                                                                           */

  fromTouch = theFromValueHdr->touch;                     /* not NULL if already touched*/
  moveWithinSameObject = (Boolean)(theFromObject==theToObject); /*moving within same obj*/

  if (fromTouch == NULL || (fromTouch->touchFlags & TouchedInserted) == 0) {
    if (fromTouch == NULL) {                          /* create "from" list entry       */
      fromTouch = createTouchedListEntry(theFromValueHdr);
      if (fromTouch == NULL) return (false);
    }

    if (moveWithinSameObject)                         /* "Orig O,Diff P"...             */
      fromTouch->touchFlags |= TouchedInserted;       /* ...action "A"                  */
    else {                                            /* "Diff. O"...                   */
      if ((toTouch = (TouchedListEntryPtr)CMmalloc(container, sizeof(TouchedListEntry))) == NULL) {
        ERROR1(container,CM_err_NoTouchedEntry, CONTAINERNAME);
        return (false);
      }
      *toTouch = *fromTouch;                          /* create "to" list entry         */
      cmAppendListCell(&theToObject->touchedList, toTouch);

      cmAddObjToTouchedChain(container, theToObject); /* put "to" obj. on touched chain */

      toTouch->touchFlags    |= TouchedInserted;      /* flag "to" entry as "inserted"  */
      fromTouch->touchFlags   = TouchedRemoved;       /* flag "from" as "removed"       */
      toTouch->removedEntry   = fromTouch;            /* set back ptr to "from" entry   */
      theFromValueHdr->touch  = toTouch;              /* point value hdr at "inserted"  */
      fromTouch->removedEntry = NULL;                 /* make sure of "from" back ptr   */
    }

    return (true);                                    /* end of actions "A"/"B"; state 0*/
  } /* 1st move */

  /* At this point only states 1 and 2 are possible, i.e., the object has been moved.   */
  /* Unlike the actions listed in the comments some common parts can be factored out.   */
  /* The first thing to factor is the moving of the "inserted" entry to the "to" object */
  /* for actions "D", "E", "F", and "G". It is unnecessary to move the "inserted" entry */
  /* if we're moving within the same object.  The "inserted" entry will still point to  */
  /* the moved value header whether moved or not moved.                                 */

  if (!moveWithinSameObject) {                        /* factor "D", "E", "F", "G"...   */
    cmDeleteListCell(&theFromObject->touchedList, fromTouch); /* move "inserted" to "to"*/
    cmAppendListCell(&theToObject->touchedList, fromTouch);
    cmAddObjToTouchedChain(container, theToObject);   /* put "to" obj. on touched chain */
  }

  /* At this point the "inserted" entry is considered moved to the "to" object.  We now */
  /* have to see if we're in state 2 or 3.  It will be 3 if the "to" object ID is       */
  /* different from the original object ID of the value which was put in the value's    */
  /* touched list entry when it was first created.                                      */

  /* For moving to a different object, we must check that the "from" is in the original */
  /* object.  If it is, there is no "removed" touched list entry.  We therefore must    */
  /* create it at set the back pointer of the "to" "inserted" entry to point to it.     */

  toTouch = fromTouch;                                /* "to" now has "inserted" entry  */

  if (toTouch->objectID != theToObject->objectID) {   /* "Diff. O" ==> "D" or "G"       */
    if (toTouch->removedEntry == NULL) {              /* coming from state 0 ("B")==>"D"*/
      if ((fromTouch = (TouchedListEntryPtr)CMmalloc(container, sizeof(TouchedListEntry))) == NULL) {
        ERROR1(container,CM_err_NoTouchedEntry, CONTAINERNAME);
        return (false);
      }
      *fromTouch = *toTouch;                          /* create "from" list entry       */
      cmAppendListCell(&theFromObject->touchedList, fromTouch);

      fromTouch->touchFlags   = TouchedRemoved;       /* flag "from" as "removed"       */
      toTouch->removedEntry   = fromTouch;            /* set back ptr to "from" entry   */
      fromTouch->removedEntry = NULL;                 /* no back ptr in "removed" entry */
    }

    return (true);                                    /* end of actions "D"/"G"         */
  }

  /* States 2 and 3 are still in effect here.  But now we also know that we're moving   */
  /* the value back to the original object or we're already there.  Thus actions "C",   */
  /* "E", and "F" are possible.  So are the NOP actions.  For action "F" there is a     */
  /* back pointer to the "removed" entry.  There is no "removed" entry when there is an */
  /* "inserted" in the original object.  Thus it is deleted first...                    */

  if (toTouch->removedEntry != NULL) {                /* action "F" ==> delete "removed"*/
    CMfree(container, cmDeleteListCell(&theToObject->touchedList, toTouch->removedEntry));
    toTouch->removedEntry = NULL;                     /* no back ptr any more           */
  }

  /* If we're moving back to the original object and property, then put things back to  */
  /* the initial state (action "E").  The "inserted" entry is all that remains in the   */
  /* "to" object.  Thus we play around with it.  If we're not moving back to the        */
  /* original property, then we've completed actions "C" and "F" and there is nothing   */
  /* else to do.  Note that the "initial state" may not be quite as it was when the     */
  /* value was moved that first time.  It could have been edited along the way.  It     */
  /* could also have been edited before the initial move.  Either way, if that is the   */
  /* case, we must keep a touched list entry.                                           */

  if (toTouch->propertyID == toPropertyID) {          /* moving back to orig. property  */
    toTouch->touchFlags &= ~TouchedInserted;          /* remove "inserted" flag         */
    toTouch->removedEntry = NULL;                     /* make sure of back ptr          */

    if ((toTouch->touchFlags & TouchedModified) == 0) {             /* keep entry if    */
      CMfree(container, cmDeleteListCell(&theToObject->touchedList, toTouch)); /* other things done*/
      theFromValueHdr->touch = NULL;                                /* to touched value */
    }
  }

  return (true);                                      /* return success                 */
}


/*---------------------------------------------------------------------------------------*
 | cmTouchDeletedValue - generate a touched list entry for a value that is being deleted |
 *---------------------------------------------------------------------------------------*

 This routine defines a touched list entry for a value when an explicit CMDeleteValue() is
 done.  A touched list entry for the object "owning" a value that is (to be) deleted is
 defined.  The parameters specify the value header to be deleted (theValueHdr) and the
 value's TOCObject (theObject).  The function returns true if everything went ok and false
 otherwise.  This can only fail because of an allocation error creating a touched list
 entry.

 Values are deleted individually and explicitly via CMDeleteValue(), or implicitly by
 implicit deletion of the value's property.  An implicit property deletion comes about
 when all the values of a property are explicitly deleted or moved out, or if the object
 itself is deleted.  Implicit value deletions are handled cmImplicitDeleteValueTouch()
 specially as descriped there.

 For explicit deletions, touched list entry for a deleted value is always created or set
 to "deleted value" in  the ORIGINAL object.  If the value has been moved to a different
 object, then the "inserted" entry for the value (there must be one) has a back pointer to
 the original touched list entry entry which must be flagged "removed".  That "removed"
 entry is therefore changed to "deleted value" and the "insert" entry is removed.
*/

Boolean cmTouchDeletedValue(TOCValueHdrPtr theValueHdr, TOCObjectPtr theObject)
{
  ContainerPtr        container = theValueHdr->container->updatingContainer;
  TouchedListEntryPtr touch;

  /* Don't create touched list entries if we're not updating or we're processing a new  */
  /* value header...                                                                    */

  if (!TouchIt(container, theValueHdr->container))
    return (true);

  /* If the value has never been touched, create a "deleted value" touched list entry   */
  /* for it...                                                                          */

  touch = theValueHdr->touch;                         /* not NULL if previously touched */

  if (touch == NULL) {                                /* if this is the first touch...  */
    touch = createTouchedListEntry(theValueHdr);      /* ...create "deleted value" entry*/
    if (touch == NULL) return (false);
  }

  /* At this point the value has a corresponding touched list entry. If it is the first */
  /* touch, or touched but not moved, or moved, but it's in the original object (i.e.,  */
  /* there's no "removed" touched list entry), then change the existing touched list    */
  /* entry to "deleted value".  Then we're done.                                        */

  if ((touch->touchFlags & TouchedInserted) == 0 || touch->removedEntry == NULL) {
    touch->touchFlags = TouchedDeletedValue;          /* changed to "deleted value"     */
    return (true);                                    /* exit successfully              */
  }

  /* If the value has been moved to a different object, the back pointer in the         */
  /* corresponding touched list ("inserted") entry points to a "removed" entry in the   */
  /* original object.  Change the "removed" entry to "deleted value" and delete the     */
  /* "inserted" entry where ever the value happens to be at this moment.                */

  touch->removedEntry->touchFlags = TouchedDeletedValue;   /* "removed"=>"deleted value"*/

  CMfree(container, cmDeleteListCell(&theObject->touchedList, touch));/* delete "inserted" entry   */

  return (true);                                      /* return success                 */
}


/*----------------------------------------------------------------------------------------*
 | cmImplicitDeleteValueTouch - delete touched list entry for an implicitly deleted value |
 *----------------------------------------------------------------------------------------*

 This routine is called whenever a value is IMPLICITLY deleted because an object or a
 property is deleted.  In that case, all the values for the object, or one of the object's
 property's are deleted.  We also must delete the the corresponding touched list entry
 if there is one for those values since there isn't going to be any value there.  However,
 if a value was moved in from another object, then the "removed" entry in the original
 value is flagged as "deleted value".

 Implicit deletion of values because their property is going away is essentially a subset
 of the explicit deletion case handled by cmTouchDeletedValue().  The difference between
 deleting an individual value, and deleting a value when its property is going to be
 deleted, is that a "deleted value" entry is created in the individual delete case, while
 the entry is always deleted in the property value delete case.  What both have in common,
 however, is that "removed" entries in other objects for values that were moved in to the
 property being deleted are set to "deleted value".

 The current callers to this routine are (both in  TOCEnts.c  ):

 cmDeleteProperty()   Called when a CMDeleteObjectProperty() is done.  A single property
                      is deleted.  Hence all the values for that property are implicitly
                      deleted.

 deleteProperties()   Internal routine called when an object is deleted.  There, all the
                      properties of the object are to be deleted. This is action routine
                      for cmDelete1Object().  cmDelete1Object() is called whenever all the
                      properties are deleted by deleting values or deleting properties.
                      In those cases however, values would have been singly deleted and
                      cmTouchDeletedValue() would be handling those.  Only when the
                      deletion is a byproduct does this routine get called.
*/

void cmImplicitDeleteValueTouch(TOCValueHdrPtr theValueHdr, TOCObjectPtr theObject)
{
  ContainerPtr        container;
  TouchedListEntryPtr touch = theValueHdr->touch;

  if (touch != NULL) {                                /* if touched value...            */
    if (touch->touchFlags & TouchedInserted)          /* ...if "inserted"...            */
      if (touch->removedEntry)                        /* ...and has "removed" entry     */
        touch->removedEntry->touchFlags = TouchedDeletedValue;/* make a "deleted value" */

    container = theObject->container;                        /* needed for CMfree()     */
    CMfree(container, cmDeleteListCell(&theObject->touchedList, touch));/* delete touched entry    */
    theValueHdr->touch = NULL;                               /* fix hdr accordingly     */
  }
}


/*-------------------------------------------------------------------------------*
 | cmTouchDeletedProperty - generate a touched list entry for a deleted property |
 *-------------------------------------------------------------------------------*

 This routine defines a touched list entry for a property when the property is explicitly
 deleted by CMDeleteObjectProperty().  A touched list entry for the object "owning" the
 property that has been deleted is defined. The parameters specify the property of the
 object to be deleted (theProperty) and the property's TOCObject (theObject). The function
 returns true if everything went ok and false otherwise.  This can only fail because of an
 allocation error creating a touched list entry.

 Note, this routine is slightly different from touches on values in that no value is
 involved explicitly in the property delete.  They are, however, implicitly involved to
 the degree that all values are deleted from the property (and the value headers put on
 the deleted values list).  The process of value deletion deletes all the touched list
 entries for those values in the property's object.  Any values moved into this object
 from other objects also have a "removed" touched list entry in the original object.
 Deleting such values for the property has exactly the same effect as CMDeleteValue(),
 i.e., the "removed" entry is changed to a "deleted value" entry.  See
 cmTouchDeletedValue() for further details.

 Note, that values previously moved OUT of the object containing the property being deleted
 have "removed" entries of their own in this object.  Thus, there still could be "dangling"
 "removed" entries on this object's touched list; dangling, because no value header of the
 property being deleted points to it -- the value header has been moved out!  These must
 remain in the normal course of move handling.  However, see what happens to these things
 when the entire object is deleted!  Read cmTouchDeletedObject() for details.

 Because a deleted property must create a touched list entry, we must do it here, because
 the standard touched list creation routine (createTouchedListEntry()) deals only with
 values (headers).
*/

Boolean cmTouchDeletedProperty(TOCPropertyPtr theProperty, TOCObjectPtr theObject)
{
  ContainerPtr        container = theObject->container->updatingContainer;
  TouchedListEntryPtr touch;

  /* Don't create touched list entries if we're not updating or we're processing a new  */
  /* object...                                                                          */

  if (!TouchIt(container, theObject->container))
    return (true);

  /* Create the "deleted property" touched list entry...                                */

  if ((touch = (TouchedListEntryPtr)CMmalloc(container, sizeof(TouchedListEntry))) == NULL) {
    ERROR1(container,CM_err_NoTouchedEntry, CONTAINERNAME);
    return (false);
  }

  cmNullListLinks(touch);                             /* init entry fields...           */
  touch->theValueHdr  = NULL;
  touch->objectID     = theObject->objectID;
  touch->propertyID   = theProperty->propertyID;
  touch->typeID       = 0;
  touch->touchFlags   = TouchedDeletedProperty;       /* "deleted property" entry       */
  touch->removedEntry = NULL;

  cmAddObjToTouchedChain(container, theObject);       /* put obj. is on touched chain   */

  cmAppendListCell(&theObject->touchedList, touch);   /* add touch to the object        */

  return (true);                                      /* return success                 */
}


/*---------------------------------------------------------------------------*
 | cmTouchDeletedObject - generate a touched list entry for a deleted object |
 *---------------------------------------------------------------------------*

 This routine is called whenever an object is deleted. The object is placed on the touched
 chain if it is not already there.  The object's touched chain, if any, is deleted EXCEPT
 for "removed" entries for values previously moved out of this object.

 The "removed" entries are kept because there are references to these entries from
 "inserted" entries from the other objects (touched list) to where the values were moved.
 If a moved value is itself deleted after the object it came from is deleted, then the back
 pointer in its "inserted" entry must be kept valid so that the "removed" entry it points
 to can be changed to a "deleted value".  Although this "deleted value" is on a touched
 list of a deleted object, it won't cause harm, since the final walks of the touched chain
 know to expect this.

 Deleted objects are moved to the deleted objects list and flagged as deleted.  This is
 done to protect against the user reusing the refNum after the delete.  Thus an object on
 the touched chain, but flagged as deleted, is enough to indicated to close-time
 processing what is going on.  The remaining touched list "goes along for the ride".

 This routine MUST be called AFTER all objects and properties for the object have been
 deleted.  By that time all touched list entries for values will have been processed
 (using cmImplicitDeleteValueTouch()).   The reason it must be after is that if it were
 before, we could have touched value headers pointing to touched list entries we free up
 here.  Not too cool!  cmImplicitDeleteValueTouch() would have a hard time dealing with
 that.
*/

void cmTouchDeletedObject(TOCObjectPtr theObject)
{
  ContainerPtr        container = theObject->container->updatingContainer;
  TouchedListEntryPtr touch, nextTouch;

  /* Don't create touched list entries if we're not updating or we're processing a new  */
  /* object...                                                                          */

  if (!TouchIt(container, theObject->container))
    return;

  /* Look at the remaining touched list entries for the deleted object (if any).  By    */
  /* point there should only be "removed" entries for values previously moved out of    */
  /* this object.  These must remain so that if a moved value is itself deleted, its    */
  /* "inserted" entry back pointer has a valid "removed" entry to point at!  Everything */
  /* else that might be left on the touched list is freed.  There should be anything,   */
  /* but what the hell.  Continuing in this "paranoid" mode, if we see any "inserted"   */
  /* entries, we use thier back pointers to change their corresponding "removed" entries*/
  /* to "deleted value".                                                                */

  touch = (TouchedListEntryPtr)cmGetListHead(&theObject->touchedList);

  while (touch) {                                               /* process touched list */
    nextTouch = (TouchedListEntryPtr)cmGetNextListCell(touch);  /* get next touch entry */

    if (touch->touchFlags & TouchedInserted)                    /* ...if "inserted"...  */
      if (touch->removedEntry)                                  /* ...+ "removed" entry */
        touch->removedEntry->touchFlags = TouchedDeletedValue;  /* make "deleted value" */

    if ((touch->touchFlags & TouchedRemoved) == 0)              /* if not "removed"...  */
      CMfree(container, cmDeleteListCell(&theObject->touchedList, touch)); /* delete touched entry */

    touch = nextTouch;                                          /* process next entry   */
  } /* while */

  /* Make sure object is on the touched chain...                                        */

  cmAddObjToTouchedChain(container, theObject);
}


/*---------------------------------------------------------------------------------*
 | generateDataEdits - generate insert/delete (data editing) updating instructions |
 *---------------------------------------------------------------------------------*

 This routine is called only by genValueUpdateInstructions() to generate data edits, i.e.
 data insert and delete sequences, for a value.  The value header and the owning object's
 special "updates" property value refNum are passed.

 The value refNum is where the updating instructions are written.  The value header is
 used to analyze the value segments to see what has to be done.  The fact that this
 routine is called at all means that this value has been touched because it was written
 to or had some of its data deleted.  We also know that the value is not immediate since
 they are handled separately using a distinct touch (flag).  Further, by this point, the
 value header ptr is not NULL and has at least one segment.  All of these checks were
 done.

 In order to understand the algorithm used here to determine data inserts and deletes, the
 "logical offset" and "logical size" for a value must be described and what is expected in
 the segment arrangements.  Assume the following example value header and sequence of
 value segments:

 *-------------*   X                                                     Y
 |  Value Hdr  |   *-------------*   *-------------*   *-------------*   *-------------*
 |             |-->|  Value Seg. |-->|  Value Seg. |-->|  Value Seg. |-->|  Value Seg. |
 |- - - - - - -|   |- - - - - - -|   |- - - - - - -|   |- - - - - - -|   |- - - - - - -|
 |             |   | "log. off." |   |    (new)    |   |    (new)    |   | "log. off." |
 | "log. size" |   *-------------*   *-------------*   *-------------*   *-------------*
 *-------------*   |<---- l ---->|

 The "logical size" is the original total size of the data as it was read in at open time.
 It remains constant in the value header.  The "logical offset" is the original start of
 each value segment when the TOC was loaded at open time.  This is the relative offset of
 the start of each value segment, viewing each segment as the user would, i.e., as
 contiguous data.  This offset has nothing to do with the actual data offset in the
 container.

 As each segment is created, it is "tagged" to its "owning" container.  This tag takes the
 form of a pointer to the container (control block) that owns it.  "New" segments are
 therfore created after open time and will always belong to the updating container.  "Old"
 segments are any segments not belonging to the updating container.

 Therfore, a data insert can always be determined simply by seeing if the segment is a
 "new" segment.  In the above example, two "new" segments appear between two "old"
 segments.

 Of course, "new" segments" never have a valid logical offset given to them.  The logical
 offsets are only set at open time in the "old" segments.  They supply the information
 needed to determine a delete and also where new segments are inserted.

 When data is deleted from an old segment, its logical offset is "corrected" as necessary.
 For example, if a segment started at offset X, and the n left-most bytes are deleted from
 that segment, then its corrected logical offset is X+n.  Corrections are done for all
 the delete possibilites on old segments as appropriate.

 Thus, knowing that ORIGINALLY the logical offsets in each old segment were contiguous
 (i.e., the logical offset for a segment plus that segments length exactly equals the
 logical offset of the next old segment), a deletion is detected when the starting offset
 is different than what's expected for it.  The "expected" logical offset of the next old
 segment is always the offset of current segment plus the current segment's length.  The
 original contiguous set of segments always has the "expected" offsets.

 Deletions can only produce a corrected logical offset that is greater than what is
 expected.  The difference of the corrected logical offset and what is expected is the
 number of bytes deleted.  The starting point for the deleted is just the expected logical
 offset.

 Note, for insertions, the insertion point is also the expected logical offset.  If the
 next segment after an old segment is a new segment, then it must be at the expected
 offset.  This is true even if an old segment had to be split to do the insertion.  The
 logical offsets in the split old segments are appropriately corrected to be "contiguous"
 (from a logical offset point of view).

 All through the above discussion the point was made that a deletion is determined using
 the expected logical offset and the logical offset of the "next old" segment.  The
 emphasis is on the "next old", because it doesn't matter how many new segments may be
 inserted between the two old segments.  This is illustrated above.  Thus both a delete
 and insert can be detected and produced during analysis.  Indeed, overwrites are
 produced when updating as deletes followed by inserts.

 So, in the above example, the first old segment starts at offset X (in reality, the first
 segment will always be 0, but generalize this to an arbitrary sequence of segments that
 may not be first).  It's length is l.  The expected logical offset for the next old
 segment is X+l.  The next old segment starts at logical offset Y.  Thus if Y > (X+l) then
 (X+l)-Y bytes have been deleted.  Also, two new segments are inserted at X+l.

 The algorithm below always assumes we start with an old segment and looks for the next
 old segment, keeping track of new segments in between.  The expected offset is maintained.
 The deletes and/or inserts are generated whenever we don't get what's expected or we
 encountered new segments.  That's basically all there is to it!

 One final point.  Nothing was mentioned above about how the "logical size" is used.
 Essentially, it just modifies this algorithm by always pretending there is a final old
 segment whose logical offset is the logical size.  This is needed when checking for
 deletions of the last n bytes of the data.

 Finally, the insert/delete updating instructions generated have the following formats:

                      +---+---+---+---+---+---+---+---+---+
     DeleteData       |   |starting offset|    amount     |
                      +---+---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+---+---+---+---+---+---+
     InsertData1      |   | insert point  |  data offset  |  data length  |
       (N = 1)        +---+---+---+---+---+---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     InsertDataN      |   | insert point  |       N       | data offset1  | data length1  |
       (N > 1)        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
                                                                        - - -
                                                          +---+---+---+---+---+---+---+---+
                                                          | data offsetN  | data lengthN  |
                                                          +---+---+---+---+---+---+---+---+

 There are two data insert formats; one when only one segment is to be inserted, and one
 when N > 1.
*/

static void CM_NEAR generateDataEdits(TOCValueHdrPtr theValueHdr, void *ioBuffer)
{
  ContainerPtr  container = theValueHdr->container->updatingContainer;
  TOCValuePtr   theValue, theNewValue;
  CMCount		 expected, nextOldLogicalOffset;
  unsigned int 	nbrOfInsertedValues;
  int           adjustment, deleteSize, insertSize;

  /* To make this code a little easire to follow, the following macros are defined...   */

  #define NewValue(v)   ((v)->container == container) /* check for an "new" value seg.  */
  #define OldValue(v)   ((v)->container != container) /* check for an "old" value seg.  */

  #define LogicalOffset(v)  ((v)->logicalOffset)      /* logical offset of an old seg.  */
  #define LogicalSize(vh)   ((vh)->logicalSize)       /* logical size                   */

  /* Process all the segments.  The variable "expected" is maintained as the expected   */
  /* offset to the next segment.                                                        */

  expected = 0;                                       /* 0 is initial expected offset   */
  adjustment = 0;                                     /* no "adjustment" yet            */

  for (theValue = (TOCValuePtr)cmGetListHead(&theValueHdr->valueList);;) {

    /* Skip contiguous old segments, i.e., old segments that have the expected offsets. */

    while (theValue != NULL   &&                      /* skip while there are segments  */
           OldValue(theValue) &&                      /* ...and its "old"               */
           LogicalOffset(theValue) == expected) {     /* ...and is contiguous...        */
      expected = LogicalOffset(theValue) + theValue->value.notImm.valueLen;
      theValue = (TOCValuePtr)cmGetNextListCell(theValue);
    }

    /* A new segment or an old segment with a logical offset that was not expected has  */
    /* been seen (or there are no more segments).  Remember this segment assuming it's  */
    /* new segment and skip over the new segments if it indeed is a neew segment.  A    */
    /* count of the number of new segments is done to indicate later that we do have an */
    /* insertion and how many segments.  This will also iindicate which insertion       */
    /* control code to generate (InsertData1 or InsertDataN).                           */

    theNewValue = theValue;                           /* remember where the segment is  */
    nbrOfInsertedValues = 0;                          /* assume it's not a new segment  */
    insertSize = 0;                                   /* accumulate size of this insert */

    while (theValue != NULL && NewValue(theValue)) {  /* count nbr of new inserted segs */
      insertSize += theValue->value.notImm.valueLen;  /* ...and size of this insertion  */
      ++nbrOfInsertedValues;
      theValue = (TOCValuePtr)cmGetNextListCell(theValue);
    }

    /* The current segment is always an old segment here (assuming "old" if there is no */
    /* more segments).  This old segment is either the one immediately following a set  */
    /* of one or more newly inserted segments, or it's just then next old segment that  */
    /* doesn't have the expected logical offset.  Either way, we process deletions here.*/

    if (theValue == NULL || OldValue(theValue)) {     /* have next old segment...       */
      nextOldLogicalOffset = (theValue != NULL) ? LogicalOffset(theValue) :
                                                  LogicalSize(theValueHdr);
      if (expected < nextOldLogicalOffset) {          /* generate deletion if required  */
        deleteSize = nextOldLogicalOffset - expected;
        PUT1(DeleteData         , ioBuffer);          /* <DeleteData>                   */
        PUT4(expected+adjustment, ioBuffer);          /*             offset             */
        PUT4(deleteSize         , ioBuffer);          /*                   amount       */
      } else
        deleteSize = 0;
    } /* deletions */

    /* If new segments were encountered, generate the insertion instructions...         */

    if (nbrOfInsertedValues > 0) {                    /* if N inserted segments (N > 0) */
      if (nbrOfInsertedValues == 1) {                 /* N = 1...                       */
        PUT1(InsertData1        , ioBuffer);          /* <InsertData1>                  */
        PUT4(expected+adjustment, ioBuffer);          /*              insertPt          */
      } else {                                        /* N > 1...                       */
        PUT1(InsertDataN        , ioBuffer);          /* <InsertDataN>                  */
        PUT4(expected+adjustment, ioBuffer);          /*              insertPt          */
        PUT4(nbrOfInsertedValues, ioBuffer);          /*                       N        */
      }

      while (nbrOfInsertedValues--) {                 /* generate N offset/length pairs */
        PUT4(theNewValue->value.notImm.value   , ioBuffer);
        PUT4(theNewValue->value.notImm.valueLen, ioBuffer);
        theNewValue = (TOCValuePtr)cmGetNextListCell(theNewValue);
      }
    } /* insertions */

    /* Exit if there are no more segments.  If there are, reset the expected logical    */
    /* offset to the (corrected) value in the current segment.  Processing continues    */
    /* using this segment.  Thus the end of one interation around this loop overlaps    */
    /* next, with the "end" old segment of the previous interation being the "start"    */
    /* segment of the next iteration.                                                   */

    if (theValue == NULL) break;                      /* done if no more segments       */
    expected = LogicalOffset(theValue);               /* reset expected logical offset  */
    adjustment += (insertSize - deleteSize);          /* fix "adjustment" to expected   */
  } /* for */
}


/*-------------------------------------------------------------------*
 | genValueUpdateInstructions - generate value updating instructions |
 *-------------------------------------------------------------------*

 This routine is called only by generateValueUpdates() to generate the specific value
 updating instruction sequences needed to bring a value "up-to-date".  The touched list
 entry for the value and the owning object's special "updates" property value refNum are
 passed.

 The value refNum is where the updating instructions are written.  The touch list entry
 pointer indicates what has been done to the value and its original "OPT" address.  The
 entry also points to the touched value header (except for "removed" values).  The value
 header's flags, in turn, can add additional information about what's been done to the
 value.  Specifically, whether the value's been edited, set-infoed, immediate, and so on.

 By the time generateValueUpdates() calls this routine all the updating instructions
 sequences needed to address the value in question have been output.  A "<new prop> P T"
 sequence when the property changes, or a "<new type> T" sequence when a new value is
 to be processed but for the "current" same property.  The object is never needed since
 these sequences are attached to the object in its special "updates" property.

 After generateValueUpdates() generates the "<new prop> P T" or "<new type> T", it calls
 this routine to generate "<v> [params...]", where <v> is a control code (byte) and
 [params...] is one or more parameters as a function of that control code.  The following
 diagrams show the exact layout of the sequences generated for each value control code:

                      +---+
     RemovedValue     |   |
                      +---+

                      +---+---+---+---+---+---+---+---+---+---+---+---+---+
     InsertedValue    |   |  original O   |  original P   |  original T   |
                      +---+---+---+---+---+---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+---+---+
     SetInfoedValue   |   |     new T     |  geneneration |
                      +---+---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+---+---+
     DeleteData       |   |starting offset|    amount     |
                      +---+---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+---+---+---+---+---+---+
     InsertData1      |   | insert point  |  data offset  |  data length  |
                      +---+---+---+---+---+---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     InsertDataN      |   | insert point  |       N       | data offset1  | data length1  |
                      +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
                                                                        - - -
                                                          +---+---+---+---+---+---+---+---+
                                                          | data offsetN  | data lengthN  |
                                                          +---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+
     ReplaceImmediate |   | l | new imm value |
      (length=l<=4)   +---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+       +---+---+---+---+
     ReplaceBaseType  |   |n bases| base type #1  | - - - | base type #n  |
                      +---+---+---+---+---+---+---+       +---+---+---+---+

                      +---+
     DeleteValue      |   |
                      +---+

 The way to view a complete value update sequence, i.e., "<new prop> P T <v> [params...]"
 or "<new type> T <v> [params...]" is that the "<v> [params...]" act upon their receiver.
 It's sort of like a postfix notation.

 Note also that more that one sequence may be generated for a specific value.  Thus the
 [params...] definition is recursive and itself can have "<v> [params...]" sequences.

 The following code tests each of the value touch flags to generate each flag's associated
 sequence as described above.  The tests follow one after another.  Some return if
 satisfied, other fall through to the next test. THE TESTING ORDER IS NOT ARBITRARY (except
 for, perhaps, the ones that don't fall through).  Here are the tests in the indicated
 order and the reason(s) they are in that order.

 1. "removed"             Value removed from this object.  Nothing else can be done to it.
                          Done first to get it out of the way.

 2. "deleted value"       Value deleted from this object. Nothing else can be done to it.
                          Also done second to get it out of the way too.  It could have
                          been done first.

 3. "inserted"            Data moved from someplace else. This must be done before anything
                          further is done to the value. It has to be moved to this object
                          from its original address since this set of update instructions
                          is attached to the destination object and property (where the
                          value was moved to).  The destination property for the object
                          need NOT exist prior to the move at open-time.  The move brings
                          it into existance.  Thus "inserted" has to be first so that there
                          is a value to operate on by the other updates.

 All the following tests deal with a value (header) that remains after updating.  These
 tests fall through to one another.  Most are mutually exclusive, but the order is more or
 less logically chosen.

 4. "base type changed"   Replace base type array of IDs.  Base types are basically
                          immediate data.  Immediates are processed early to get them out
                          of the way too.  Base types, however, cannot be set-infoed.  So
                          they are processed as the first value operation.

 5. "set-infoed"          Set-infoing can be done in addition to all further sequences so
                          it should be done now.

 6. "immediate"           Replace immediate with new value.  This is the last of the
                          immediate value possibilities.  A "few words" should be said
                          about what happens to immediate data if it needed to be converted
                          to non-immediate during editing...

                          If a value has made it this far and is still an immediate, then
                          a "replace immediate" is the most efficient way to deal with it.
                          If, along the way, it was converted to a non-immediate, then
                          the immediate data is written to the updating container as "new"
                          data. From our point of view here it looks as though the value
                          data was completely overwritten (i.e., deleted and new data
                          inserted).  We can't tell the difference, and the delete/insert
                          sequences, as done with non-immidate overwrites is generated.
                          There is no special case.

 7. "edited"              Data insert and delete are the last (and most complicated)
                          operations that can be performed on the (non-immediate) data, so
                          they are placed last (so I can avoid figuring out how to code
                          them as int  as possible -- how's that for a reason?).
*/

static void CM_NEAR genValueUpdateInstructions(TouchedListEntryPtr touch, void *ioBuffer)
{
  TOCValueHdrPtr theValueHdr;
  TOCValuePtr    theValue;
  unsigned short touchFlags = touch->touchFlags;

  /* 1. "removed": <RemovedValue>                                                       */

  if (touchFlags & TouchedRemoved) {                    /* 1. "removed"                 */
    PUT1(RemovedValue, ioBuffer);                       /* <RemovedValue>               */
    return;                                             /* exit                         */
  }

  /* 2. "deleted value": <DeleteValue>                                                  */

  if (touchFlags & TouchedDeletedValue) {               /* 2. "deleted value"           */
    PUT1(DeleteValue, ioBuffer);                        /* <DeleteValue>                */
    return;                                             /* exit                         */
  }

  /* 3. "inserted": <InsertedValue> O P T                                               */

  if (touchFlags & TouchedInserted) {                   /* 3. "inserted"                */
    PUT1(InsertedValue    , ioBuffer);                  /* <InsertedValue>              */
    PUT4(touch->objectID  , ioBuffer);                  /*                O             */
    PUT4(touch->propertyID, ioBuffer);                  /*                  P           */
    PUT4(touch->typeID    , ioBuffer);                  /*                    T         */
  }

  /* The following are all the value data or info manipulations...                      */

  theValueHdr = touch->theValueHdr;                     /* point at the touched valueHdr*/
  theValue = (TOCValuePtr)cmGetListHead(&theValueHdr->valueList); /* and 1st value      */

  /* 4: "base type changed": <ReplaceBaseType> N baseType1 . . . baseTypeN              */

  if (touchFlags & TouchedBaseType) {                   /* 4: "base type changed"       */
    PUT1(ReplaceBaseType, ioBuffer);                    /* <ReplaceBaseType>            */

    PUT2(cmCountListCells(&theValueHdr->valueList), ioBuffer);/*           N            */

    while (theValue != NULL) {                          /*                 bt1 . . . btN*/
      PUT4(theValue->value.imm.ulongValue, ioBuffer);
      theValue = (TOCValuePtr)cmGetNextListCell(theValue);
    }
  }

  /* 5: "set-infoed": <SetInfoedValue> T g                                              */

  if (touchFlags & TouchedSetinfoed) {                  /* 5: "set-infoed"              */
    PUT1(SetInfoedValue         , ioBuffer);            /* <SetInfoedValue>             */
    PUT4(theValueHdr->typeID    , ioBuffer);            /*                 T            */
    PUT4(theValueHdr->generation, ioBuffer);            /*                  g           */
  }

  /* 6: "immediate": <ReplaceImmediate> length value                                    */

  if (touchFlags & TouchedImmediate)                    /* 6: "immediate"               */
    if (theValue->flags & kCMImmediate) {               /* ...only if not converted!    */
      PUT1(ReplaceImmediate               , ioBuffer);  /* <ReplaceImmediate>           */
      PUT1(theValue->value.notImm.valueLen, ioBuffer);  /*                   length     */
      PUT4(theValue->value.imm.ulongValue , ioBuffer);  /*                         value*/
    }

  /* 7: "edited": <DeleteData> offset amount                                            */
  /*              <InsertDataX> [N] insertPt offset length [...]                        */

  if (touchFlags & TouchedEdited)                       /* 7: "edited"                  */
    generateDataEdits(theValueHdr, ioBuffer);           /* generate separately...       */
}


/*--------------------------------------------------------------------------*
 | generateValueUpdates - main control generate value updating instructions |
 *--------------------------------------------------------------------------*

 This routine is called only by cmGenerateUpdates() to control the generation of value
 updating instructions.  The entire touched list chain is walked and all non-deleted
 objects on the chain have their respective touched lists processed.  Property deletes are
 skipped on the touched lists.  Thus all that are processed here are touched values and
 their update instructions generated.

 The update instructions for all the touched values of any one object are generated as data
 for a special "updating" property of that object.  That property and value are defined
 here.  The TOC has not yet been written, so this is permissible.  It thus goes into the
 TOC like any other value and will be written to the container when the time comes.  At
 open time, the TOC reader can look for this special property to get at the object's
 update list.

 All touched value list entries are freed after processing.  Thus all that will be left
 on the touched chain are touched deleted objects and non-deleted objects with touched
 chains consisting solely of "deleted property" entries.

 False is returned for failure, and true for success.

 The instructions are generated in the following general sequences:

                                    --                                    --
                                    |  ( <new prop> P T <v> [params...] )  |
    <new prop> P T <v> [params...]  |  <                                >  | ... <end>
                                    |  ( <new type> T <v> [params...]   )  |
                                    --                                    --

 where <new prop> = NewProperty control byte
       P, T       = property and type IDs (4 bytes)
       <v>        = value update control byte (see genValueUpdateInstructions())
       params...  = parameters as a function of <v> (see genValueUpdateInstructions())
       <new type> = NewType control byte
       <end>      = EndUpdates control byte to mark the end of the updating sequence

 Brackets mean optional, "..." means repeating sequences of the preceding, and braces
 mean alternatives (sorry, it's the best I can do with this font).

 What the sequence does is generate "<v> [params...]" for every value update.  However,
 only the P is generated as new properties are encountered for the current object.  The
 object isn't needed since this set of instructions is just the instructions dealing with
 one object.  Remember, the instructions are value data for a value belonging to the
 special "updating" property of the object.

 Note, the "<v> [params...]" is a recursive definition, in that the "params..." itself may
 be a "<v> [params...]" sequence.

 All this encoding is done to minimize generated data space for this stuff but still make
 it easy to interpret at open time.  Thus NewProperty implies a PT immediately follow.
 NewType implies only a T follows.  This can happen when sequences of touches for an
 object all refer to the same property.  This is not the absolute minimum generation,
 however, since touch list entries are NOT sorted by property.
*/

static Boolean CM_NEAR generateValueUpdates(ContainerPtr container, void *ioBuffer)
{
  TOCObjectPtr        thePrevObject, theObject, nextTouchedObject;
  TouchedListEntryPtr nextTouch, touch;
  TOCValueHdrPtr      updatesValueHdr;
  CMObjectID          currPropertyID, nextPropertyID;
  Boolean             noTouches;
  ContainerPtr        updatingContainer = container->updatingContainer;

  /* Walk the entire touched chain looking for non-deleted touched objects...           */

  thePrevObject = NULL;                                 /* initially no previous object */
  theObject = updatingContainer->touchedChain;          /* point to 1st touched object  */

  while (theObject != NULL) {                           /* walk touched chain...        */
    nextTouchedObject = theObject->nextTouchedObject;   /* get ptr to next object early */

    if ((theObject->objectFlags & DeletedObject) == 0) {/* non-deleted objects only...  */

      /* Here we have a non-deleted object.  Generally, being that it's on the touched  */
      /* chain, it should have a touched list.  But being paranoid, we never assume     */
      /* that.  We do have to make sure that the touched list does not consist solely   */
      /* of "deleted property" entries.  At the very least we have to skip to the first */
      /* value touch on the list.  Further, moves out of an object could leave that     */
      /* object on the touched chain, but with no touched list entries if that was the  */
      /* only thing done to the object.  Those kind of objects we just want to delete.  */

      touch = (TouchedListEntryPtr)cmGetListHead(&theObject->touchedList);
      noTouches = (Boolean)(touch == NULL);             /* remember that list is empty  */

      while (touch != NULL) {                           /* look for first value touch...*/
        if ((touch->touchFlags & TouchedDeletedProperty) == 0)
          break;                                        /* ...got 'em!                  */
        touch = (TouchedListEntryPtr)cmGetNextListCell(touch);
      }

      /* If we are "looking" at the first value touch on its touched list, we know we   */
      /* have some updating instructions to generate for its object.  Thus we must      */
      /* create the special "updating" property for this object (basically done as a    */
      /* CMNewValue()).  Then we can start the  generation sequence.                    */

      if (touch != NULL) {                              /* if at least 1 value touched  */
        theObject = cmDefineObject(container,           /* ...create "updates" property */
                                   theObject->objectID,
                                   CM_StdObjID_ValueUpdates,
                                   CM_StdObjID_UpdatesData,
                                   NULL, container->generation, 0,
                                   ObjectObject, &updatesValueHdr);
        if (theObject == NULL) return (false);          /* failure (shit!)              */

        cmNewBufferedOutputData(ioBuffer, updatesValueHdr); /* init for buffered output */

        /* The property's value header is used as the "refNum" for all output operations*/
        /* done by the output routine (writeUpdate()).  The updating is buffered and    */
        /* written as value data for the new value.                                     */

        /* Generate the appropriate value update sequences for each touched value,      */
        /* skipping any further touched deleted properties we find on the list. We only */
        /* skipped initial ones above. That does not mean there aren't any further ones */
        /* on this list beyond the first touched value!                                 */

        /* For entries we do process, a check must be made each time to see if the      */
        /* property has changed. If it has (and the first time is always "change") then */
        /* a "<new prop> PT" sequence is generated. Otherwise a "<new type> T" sequence */
        /* is generated, since we always have a new type for each separate touched list */
        /* entry.                                                                       */

        /* Note, there is a non-obvious subtility in the way the "P" is chosen in the   */
        /* "<new prop> PT" sequence.  This sequence, incombination with the object they */
        /* are data for (the "update" property value data for the "owning" object), are */
        /* used to address the receiver of the actions by the updating instructions.    */
        /* That MUST address where the value is NOW.  If a value wasn't moved, the      */
        /* property in the touch list entry agree with where the value is now.  But if  */
        /* it IS moved, there is no guarantee it is (and usually won't be) in the same  */
        /* property!  So the property ID in the touched list entry is NOT used here to  */
        /* address the value.  Rather, the value header itself (it "knows" where it is) */
        /* is used to get the property ID of the property is is presently on.           */

        /* The SPECIFIC value instruction sequences are generated separately by         */
        /* genValueUpdateInstructions().  That routine assumes the "<new prop> PT" or   */
        /* "<new type> T" prefix is generated here.  But, as a safety check we do NOT   */
        /* want to generate the prefix if genValueUpdateInstructions() CAN'T generate a */
        /* "<v> [params...]" sequence.  It can't, if for some reason, the touched value */
        /* header has no values!  For "removed" and "deleted value", it doesn't matter. */
        /* But for every other kind of touch it does.  So, just to be  safe, we do this */
        /* check here and not generate the prefix or call genValueUpdateInstructions()  */
        /* if the test fails.                                                           */

        currPropertyID = 0;                             /* force first property change  */

        do {                                            /* walk touched list...         */
          nextTouch = (TouchedListEntryPtr)cmGetNextListCell(touch); /* need next now   */

          if ((touch->touchFlags & TouchedDeletedProperty) == 0)  /* not "deleted prop" */
            if ((touch->touchFlags & (TouchedRemoved | DeleteValue)) != 0 ||
                (touch->theValueHdr != NULL && cmCountListCells(&touch->theValueHdr->valueList) > 0)) {
              nextPropertyID = touch->theValueHdr->theProperty->propertyID;
              if (nextPropertyID != currPropertyID) {   /* new property...              */
                currPropertyID = nextPropertyID;
                PUT1(NewProperty   , ioBuffer);         /* <new prop>                   */
                PUT4(currPropertyID, ioBuffer);         /*            P                 */
                PUT4(touch->typeID , ioBuffer);         /*              T               */
              } else {
                PUT1(NewType       , ioBuffer);         /* <new type>                   */
                PUT4(touch->typeID , ioBuffer);         /*            T                 */
              }

              /* The specific value updating sequence, "<v> [params...]", is generated  */
              /* separately.  We got enough on our hands here as it is!  Remember, the  */
              /* "<v> [params...]" sequence is recursive so that the "params..." may    */
              /* itself container "<v> [params...]" sequences for additional updates    */
              /* for the same value.                                                    */

              genValueUpdateInstructions(touch, ioBuffer);

              /* Now that the touched value is fully processed, we can delete its       */
              /* touched entry from the touched chain.                                  */

              CMfree(container, cmDeleteListCell(&theObject->touchedList, touch));  /* poof!       */
            } /* value update */

          touch = nextTouch;                            /* next touched chain entry     */
        } while (touch != NULL);                        /* look at all of 'em           */

        /* At this point all the updating instructions for all touched values for the   */
        /* current object have been generated and written as value data for the special */
        /* updates" property of that object.  All that remains to do is write an "end-  */
        /* of-sequence" marker byte and to flush the buffer.                            */

        PUT1(EndUpdates, ioBuffer);                     /* add "end-of-update-sequence" */
        cmFlushOutputBuffer(ioBuffer);                  /* end-of-updates               */

        /* The object may be removed from the touched chain if its touched chain is     */
        /* empty.  It won't be empty if there are any "deleted property" entries which  */
        /* we skipped.                                                                  */

        if (cmIsEmptyList(&theObject->touchedList))     /* remove if no touched list    */
          deleteObjFromTouchedChain(updatingContainer, theObject, thePrevObject);
        else                                            /* if "deleted property" entries*/
          thePrevObject = theObject;                    /* ...leave it on touched list  */
      } else if (noTouches)                             /* if empty touch list...       */
        deleteObjFromTouchedChain(updatingContainer, theObject,thePrevObject);
      else                                              /* if all property deletes...   */
        thePrevObject = theObject;                      /* ...leave obj on touched chain*/
    } else                                              /* if deleted object...         */
      thePrevObject = theObject;                        /* ...leave it on touched chain */

    theObject = nextTouchedObject;                      /* process next touched object  */
  } /* while (theObject)... */

  return (true);                                        /* success                      */
}


/*-----------------------------------------------------------------------------------*
 | generateDeleteUpdates - generate object and property delete updating instructions |
 *-----------------------------------------------------------------------------------*

 This routine is called only by cmGenerateUpdates() to control the generation of object
 and property deletion updating instructions.  The entire touched list chain is walked and
 all deleted objects on the chain or non-deleted objects with a touched list are processed.
 This is the second walk of the touched chain.  The first walk was done with
 generateValueUpdates() above.  All that should remain on the touched chain are deleted
 objects (with possibly a touched chains of "removed" and "deleted value" entries) and
 non-deleted objects with "deleted property" entries.

 False is returned for failure, and true for success.

 The reason a deleted object can still have a touched chain of its own is that when an
 object is deleted, we explicitly leave "removed" entries on the chain but delete every-
 thing else.  By doing this, values moved out of the object prior to its deletion, have
 "inserted" entries with back pointers pointing to their corresponding "removed" entry.
 If these values are then deleted, they better have a valid "removed" entry to point to
 so that it can be changed to a "deleted value" entry.  That's how "deleted value" entries
 can also be on the chain.  None of this causes problems, because if we see a deleted
 object on the touched chain, we generate the "delete object" updating instruction and
 ignore the touched list except to free up its memory.

 Note, we do not remove the remaining touched objects from the touched chain.  It's
 unnecessary since we will never look at the touched chain again after this. The container
 is about to be closed.

 The deletion update instructions are generated as a single sequence for a value defined
 as a special "delete list" property of TOC #1 defined here. This is different from value
 updates where the update instructions for a particular object are "attached" to that
 object with a special property for it.  The TOC has not yet been written, so this it is
 permissible to still create TOC objects.  It thus goes into the TOC like any other value
 and will be written to the container when the time comes.  At open time, the TOC reader
 can look for this special property to get at the object's update list.

 The deletions are processed and kept separate because they must be used at open time AFTER
 all the value updates are applied.  The reason for this is mainly because of "moves".  A
 value could be moved from its original position and then its original object deleted.  If
 we didn't delay the deletes, there potentially may not be any source to move the value
 from!

 The deletion instructions are generated in the following general sequence:

    ( <del obj> O                         )
    <                                     > ... <end>
    ( <del prop1> O P [<del prop2> P ...] )

 where <del obj>   = DeleteObject control byte
       O, P        = object and property IDs (4 bytes)
       <del prop1> = DeleteProperty1 control byte (property for another object)
       <del prop2> = DeleteProperty2 control byte (property for same object)
       <end>      = EndUpdates control byte to mark the end of the updating sequence

 Brackets mean optional, "..." means repeating sequences of the preceding, and braces
 mean alternatives (sorry, it's the best I can do with this font).

 What this sequence does is generate "delete object" or "delete property" for a new or
 the same object.
*/

static Boolean CM_NEAR generateDeleteUpdates(ContainerPtr container, void *ioBuffer)
{
  TOCObjectPtr        theTOCobject, theObject;
  TouchedListEntryPtr nextTouch, touch;
  TOCValueHdrPtr      deletesValueHdr;
  void                *toc;
  Boolean             deletedObject, first;

  /* Walk the entire touched chain looking for everthing remaining there.  The "refNum" */
  /* for the special TOC #1 property is kept in deletesValueHdr and it is NULL until we */
  /* actually find something to generate.                                               */

  deletesValueHdr = NULL;                               /* NULL ==> no TOC #1 value yet */
  theObject = container->updatingContainer->touchedChain;/* point to 1st touched object */

  while (theObject != NULL) {                           /* walk touched chain...        */

    /* We need to know if we're going to generate any deletion instructions for the     */
    /* current object. If we have a deleted object, it's on the touched chain, so that's*/
    /* sufficient to know we need to generate a delete object instruction. On the other */
    /* hand, if we have a non-deleted object, just to be safe, we scan its touched list */
    /* to see if there are any "deleted property" entries.  If there are, then we know  */
    /* for that case we got something to generate.                                      */

    /* Note, for deleted objects, we always make sure all the remaining touched list    */
    /* entries are freed.  We no longer need them.                                      */

    deletedObject = (Boolean)((theObject->objectFlags & DeletedObject) != 0);
    touch = (TouchedListEntryPtr)cmGetListHead(&theObject->touchedList);

    if (deletedObject)                                  /* if deleted object...         */
      while (touch) {                                   /* ...free the touched list     */
        nextTouch = (TouchedListEntryPtr)cmGetNextListCell(touch);
        CMfree(container, cmDeleteListCell(&theObject->touchedList, touch));
        touch = nextTouch;
      } /* while */
    else                                                /* if not deleted object...     */
      while (touch != NULL) {                           /* look for 1st deleted property*/
        if ((touch->touchFlags & TouchedDeletedProperty) != 0)
          break;                                        /* ...got 'em!                  */
        touch = (TouchedListEntryPtr)cmGetNextListCell(touch);
      }

    /* At this point we know whether we got somthing to generate.  It's either for a    */
    /* deleted object, or a deleted property.  If there's nothing to generate, just go  */
    /* process the next object on the touched chain.                                    */

    if (touch != NULL || deletedObject) {               /* if somthing to generate...   */

      /* If this is the first deletion generation, create the TOC #1 "deletes" property */
      /* IN THE PRIVATE TOC.  That's the TOC #1 object we're interested in.             */

      if (deletesValueHdr == NULL) {                    /* if this is the first delete  */
        toc = container->toc;                           /* create TOC #1 property/value */
        container->toc = container->privateTOC;         /* IN THE PRIVATE CONTAINER TOC */

        theTOCobject = cmDefineObject(container,        /* ...create "updates" property */
                                      CM_StdObjID_TOC, CM_StdObjID_TOC_DeleteList,
                                      CM_StdObjID_UpdatesData, NULL,
                                      container->generation, 0,
                                      (ObjectObject | ProtectedObject),
                                      &deletesValueHdr);

        container->toc = toc;                           /* restore current TOC          */
        if (theTOCobject == NULL) return (false);       /* if failure, give up          */
        container->deletesValueHdr = deletesValueHdr;   /* save "refNum" for the world  */

        cmNewBufferedOutputData(ioBuffer, deletesValueHdr); /* init for buffered output */
      } /* 1st delete */

      /* We're now ready to generate update instructions for the current touched object.*/
      /* For a deleted object we have the sequence "<del obj> O" and that's all need be */
      /* done for this object.  For deleted property we have "<del prop1> OP" the first */
      /* time for this object, and "<del prop2> P" for additional touched deleted       */
      /* properties on the touched list.                                                */

      if (deletedObject) {                              /* if deleted object...         */
        PUT1(DeleteObject       , ioBuffer);            /* <del obj>                    */
        PUT4(theObject->objectID, ioBuffer);            /*            O                 */
      } else if (touch) {                               /* if non-deleted object...     */
        PUT1(DeleteProperty1    , ioBuffer);            /* <del prop1>                  */
        PUT4(theObject->objectID, ioBuffer);            /*            O                 */

        first = true;                                   /* this will gen P the 1st time */

        do {                                            /* walk the touched list...     */
          nextTouch = (TouchedListEntryPtr)cmGetNextListCell(touch);

          if ((touch->touchFlags & TouchedDeletedProperty) != 0) { /* deleted property? */
            if (!first)                                 /* new property...              */
              PUT1(DeleteProperty2, ioBuffer);          /* <del prop2>                  */

            PUT4(touch->propertyID, ioBuffer);          /*             P                */
            first = false;
          }

          /* Free everything remaining on the touched list just like we did for deleted */
          /* objects...                                                                 */

          CMfree(container, cmDeleteListCell(&theObject->touchedList, touch));

          touch = nextTouch;                            /* process all list entries     */
        } while (touch != NULL);
      }
    } /* object or property deletions */

    theObject = theObject->nextTouchedObject;
  } /* while (theObject)... */

  /* All deletion instructions (if any) have been generated.  All that remains to do is */
  /* write an "end- of-sequence" marker byte and to flush the buffer.                   */

  if (deletesValueHdr) {                                /* if deletions generate...     */
    PUT1(EndUpdates, ioBuffer);                         /* add "end-of-update-sequence" */
    cmFlushOutputBuffer(ioBuffer);                      /* ...end-of-updates            */
  }

  return (true);                                        /* success                      */
}


/*--------------------------------------------------------------------*
 | cmGenerateUpdates - main control for update instruction generation |
 *--------------------------------------------------------------------*

 This is called at close time (CMCloseContainer()) to generate all the updating
 instructions for all objects and their values touched during an updating session. True is
 returned if successful and false otherwise (and the error reporter returns).

 The touched objects are on the touched chain pointed to from the container control block.
 Two passes are made over the touched chain:

    Pass 1: All touched values are processed.  The touched list for each non-deleted
            object that is still on the touched chain (see pass 2) are processed for
            touched values.  Touched properties are skipped.

            A special "updates" property is generated for each object involved which
            contains a value whose data is all the value updates for that object.  Each
            processed touched value's touched list entry is deleted.

            The end of pass 1 leaves only deleted property touched list entries in
            non-deleted objects on the touched chain and touched, but deleted, objects.
            Deleted objects too can still have a touched chain of "removed" and "deleted
            objects" since such entries are retained due to the way the move and deletion
            touch algorithms work.

    Pass 2: All touched deleted objects and deleted properties.  This is the remaining
            stuff on the touched chain and lists.  The data for this is attached to a
            special TOC #1 property.  The remaining touched list entries are deleted.
            Thus, at the end of pass 2, all touched list space is freed up.
*/

Boolean cmGenerateUpdates(ContainerPtr container)
{
  Boolean success;
  void    *ioBuffer = NULL;
  jmp_buf genUpdatesEnv;

  container = container->updatingContainer;     /* make sure we're using updater        */

  if (setjmp(genUpdatesEnv)) {                  /* set setjmp/longjmp environment vector*/
    cmReleaseIOBuffer(ioBuffer);                /* ...if longjmp taken back here...     */
    ERROR1(container,CM_err_BadUpdateWrite,CONTAINERNAME);/* ...free buffer and report failure    */
    return (false);                             /* ...false ==> failure                 */
  }

  /* Allocate the output buffer to more efficiently generate the update data...         */

  ioBuffer = cmUseIOBuffer(container, UpdateBufSize, (jmp_buf *)&genUpdatesEnv);
  if (ioBuffer == NULL) return (false);

  /* Process both passes...                                                             */

  success = (Boolean)(generateValueUpdates(container, ioBuffer) && /* Pass 1: values    */
                      generateDeleteUpdates(container, ioBuffer)); /* Pass 2: deletions */

  /* Either of the above routines returns "false" if they fail for any reason.  Either  */
  /* way we give back the buffer space and then return success or failure.              */

  cmReleaseIOBuffer(ioBuffer);                  /* say good bye to the output buffer    */

  return (success);                             /* true ==> continue close processing   */
}


/*-----------------------------------------------------------------------*
 | applyValueUpdateInstructions - value updating instruction interpreter |
 *-----------------------------------------------------------------------*

 This routine is called only by applyValueUpdates() to apply value updating instructions.
 applyValueUpdates() calls this routine at the point in the sequence where it sees
 "<v> [params...]", where <v> is the next control byte to be read in.  The property and
 type information have already been processed and is passed to this routine along with the
 target object refNum.  These uniquely address the value to be operated upon by the
 "<v> [params...]" instructions.  Also passed is the object's refNum special "updates"
 property value refNum from which the updating  "<v> [params...]" instructions are being
 read.

 Note, that there may be multiple sets of updating instructions for different values of
 the SAME property.  The type IDs change, while the property ID stays the same.  For
 efficiency, we search for the object's property only when property IDs change.
 applyValueUpdates() supplys a place to save the pointer, *theProperty, and it doubles as
 a switch indicating whether the property ID should be processed.  NULL indicates we need
 the property pointer.

 The "proper" amount of updating info is consumed (read in).  The "[params...]" is
 recursive in that it may itself contain "<v> [params...]" for additional updates to the
 same value.  Thus we read "ahead" here until an "unrecognizable" control byte is
 encountered.  That control byte is returned as the function result.  If any errors are
 detected, a special "control", ErrorCtl, is returned.

 This routine is the symmetric counterpart to its generator, genValueUpdateInstructions().
 It interprets the updating instructions in the following formats:

                      +---+
     RemovedValue     |   |
                      +---+

                      +---+---+---+---+---+---+---+---+---+---+---+---+---+
     InsertedValue    |   |  original O   |  original P   |  original T   |
                      +---+---+---+---+---+---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+---+---+
     SetInfoedValue   |   |     new T     |   generation  |
                      +---+---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+---+---+
     DeleteData       |   |starting offset|    amount     |
                      +---+---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+---+---+---+---+---+---+
     InsertData1      |   | insert point  |  data offset  |  data length  |
                      +---+---+---+---+---+---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     InsertDataN      |   | insert point  |       N       | data offset1  | data length1  |
                      +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
                                                                        - - -
                                                          +---+---+---+---+---+---+---+---+
                                                          | data offsetN  | data lengthN  |
                                                          +---+---+---+---+---+---+---+---+

                      +---+---+---+---+---+---+
     ReplaceImmediate |   | l | new imm value |
      (length=l<=4)   +---+---+---+---+---+---+

                      +---+---+---+---+---+---+---+       +---+---+---+---+
     ReplaceBaseType  |   |n bases| base type #1  | - - - | base type #n  |
                      +---+---+---+---+---+---+---+       +---+---+---+---+

                      +---+
     DeleteValue      |   |
                      +---+

 See genValueUpdateInstructions() for more info on these sequences.
*/

static CM_UCHAR CM_NEAR applyValueUpdateInstructions(ContainerPtr container,
                                                     TOCObjectPtr theObject,
                                                     TOCPropertyPtr *theProperty,
                                                     unsigned int  propertyID,
                                                     unsigned int  typeID,
                                                     void *ioBuffer)
{
  CM_UCHAR       ctlByte;
  unsigned int   fromObjectID, fromPropertyID, fromTypeID, newTypeID, newImmediate,
                 baseTypeID, offset, size, endOffset, n, insertPoint, segOffset;
  unsigned short nBases;
  TOCObjectPtr   theFromObject, newTypeObject, theToProperty;
  TOCPropertyPtr theFromProperty;
  TOCValueHdrPtr theFromValueHdr, theValueHdr;
  TOCValuePtr    theValue, theNextValue, theInsertValue, insBeforeValue;
  CMGeneration   newGeneration;
  TOCValueBytes  valueBytes;
  Boolean        gotData, reLogicalize;
  char           errStr[15], errStr1[15], errStr2[15];
  #ifdef __MWERKS__
  char    		 *tmpstr;
  #endif

  /* When applyValueUpdates() calls this routine, it only has extracted the property    */
  /* and type IDs for the target value.  It did nothing else.  The reason is because of */
  /* moves, i.e., "inserted" and "removed" updating instruction entries.                */

  /* For "inserted" entries, the target object, i.e., the receiver (i.e., destination)  */
  /* property for the move, does NOT have to exist prior to applying the "inserted"     */
  /* instruction for that object!  For "removed" the last value in a source objects's   */
  /* property could have been moved out, thus deleting the property in that object.     */
  /* Either way, there is the potential that the property being acted upon does not     */
  /* exist.                                                                             */

  /* In all other cases, there is no value movement, so the property does have to exist.*/
  /* But applyValueUpdates() doesn't know whether the <v> in the "<v> [params...]"      */
  /* sequence is for a move or not.  As a matter of fact, at this point, neither do we! */
  /* So we can't set up the target value refNum until we know for sure what we have as  */
  /* the first control byte.                                                            */

  /* If it is "inserted", it is guaranteed to be first the first in the sequence by     */
  /* genValueUpdateInstructions().  This is done so that possible additional editing    */
  /* operations have a value in the target object/property to work on.  The move will   */
  /* cause the value refNum to be set up.                                               */

  /* If the control byte is "removed", it will also be the first of a sequence because  */
  /* "removed" entries are never combined with any other.   We never try to set or use  */
  /* the value refNum for this since, in this implementation, "removed" entries are     */
  /* completely ignored.                                                                */

  /* As soon as we know e don't have "inserted" or "removed" we can set the target value*/
  /* refNum.  Since we do not yet know the first value instruction, we set a switch,    */
  /* appropriately enough, the value (hdr) refNum, to indicate we need the value refNum.*/
  /* In the processing loop we set the value refNum at the "proper" time. That will be  */
  /* the first time for anything except moves or after the move is done.                */

  theValueHdr = NULL;                                   /* don't have target value yet  */

  /* Insert and delete updates change the data. As described in value update generation */
  /* routine, generateDataEdits(), the algorithm for determining whether NEW inserts and*/
  /* deletes occurred at close time requires that the segment's logical offsets reflect */
  /* a contiguous sequence, from 0 to the logical size.  As "old" (i.e., previous)      */
  /* updates are applied to the target container (which is what we're doing here), the  */
  /* logical offsets and size, as set when the TOC was loaded, are invalidated. Thus at */
  /* the end of updating this value, if any inserts or deletes occured, we must         */
  /* "re-logicalize" (do you like that verb?) the updates segments.  To that end, the   */
  /* switch, reLogicalize, is maintained and tested to see if this must be done.        */

  reLogicalize = false;                                 /* assume no data edits         */

  /* Process "<v> [params...]" sequences.  The "[params...]" is  recursive in that it   */
  /* may itself contain "<v> [params...]" for additional updates to the same value.     */
  /* Thus we loop here until we hit a "<v>" that is not valid in this context.          */

  for (;;) {                                            /* process each sequence...     */
    ctlByte = GET1(ioBuffer);                           /* read in next "<v>" ctl byte  */

    /* Now that we have a control byte, we can ask if this is the first one for the     */
    /* value, and if we have an "inserted" (move) entry.  If it's not a move, we can    */
    /* set up the value refNum.  Otherwise, we wait!  The property is only searched for */
    /* only when the property ID changes.  For updates to multiple values for the same  */
    /* property, it will not change.  applyValueUpdates() indicates we have a new value */
    /* by setting *theProperty to NULL. We set *theProperty here. As int  as it reamins */
    /* non-NULL there is no need to process the property ID.                            */

    if (theValueHdr == NULL      &&                     /* if no value refNum yet...    */
        ctlByte != InsertedValue &&                     /* ...and not "inserted" entry..*/
        ctlByte != RemovedValue) {                      /* ...and not "removed" entry...*/
      if (*theProperty == NULL) {                       /* ...get property if  changed  */
        *theProperty = cmGetObjectProperty(theObject, propertyID);
        if (*theProperty == NULL) {
          ERROR4(container,CM_err_NoUpdateProperty, CONTAINERNAME,
                 cmltostr(propertyID, 1, false, errStr),
                 cmltostr(theObject->objectID, 1, false, errStr1),
                 "updated");
          return (ErrorCtl);
        }
      }

      theValueHdr = cmGetPropertyType(*theProperty,typeID); /* get a new type...        */
      if (theValueHdr == NULL) {
#ifdef __MWERKS__
       /* OMFI - workaround for Metrowerks Code Warrior compiler bug */
        tmpstr = cmltostr(propertyID, 1, false, errStr2);
        ERROR4(container,CM_err_NoUpdateType, CONTAINERNAME,
               cmltostr(typeID, 1, false, errStr),
               cmltostr(theObject->objectID, 1, false, errStr1),
               tmpstr);
#else
        ERROR4(container,CM_err_NoUpdateType, CONTAINERNAME,
               cmltostr(typeID, 1, false, errStr),
               cmltostr(theObject->objectID, 1, false, errStr1),
               cmltostr(propertyID, 1, false, errStr2));
        return (ErrorCtl);
#endif
      }
    }

    /* The value refNum (theValueHdr) is now set if we don't have a "inserted" entry.   */
    /* The "inserted" entry will set it after it does the move.  The move will be first */
    /* if it's there.  Either way we have a value refNum which is the receiver of the   */
    /* actions determined by the updating instructions we're about to process...        */

    switch (ctlByte) {                                  /* act upon the control byte... */

                                 /*--------------*
                                  | RemovedValue |
                                  *--------------*

      A "removed" instruction is provided for random access implementation that can
      explicitly access the TOC.  Its counterpart is the "inserted value" instruction.
      Thus a random access implementation knows to remove the value from the object it's
      in and to insert it in the one with the "inserted value" instruction.

      In this implementation we load the entire TOC into memory.  Therefore, a "inserted
      value" maps into a "move" and the "removed" instruction maps into a NOP so that the
      move has a source.

      Since a "removed" instruction can be the only instruction for the value if it is
      indeed generated, we can exit and return the next byte, should should be a control
      byte.                                                                             */

      case RemovedValue:                                /* "removed" value...           */
        return (GET1(ioBuffer));                        /* ...exit with next byte       */

                                  /*-------------*
                                   | DeleteValue |
                                   *-------------*

      A "deleted value" instruction requires that the target value be deleted.  As with
     "removed" there can be no other instructions for the value, so again we can exit
      and return the next byte as the control byte.                                     */

      case DeleteValue:                                 /* "deleted value"...           */
        cmMarkValueDeleted(container, theValueHdr, true);
        return (GET1(ioBuffer));                        /* ...exit with next byte       */

                                 /*---------------*
                                  | InsertedValue |
                                  *---------------*

      An "inserted value" in this implementation (see "removed" above) means MOVE the
      value to the current location from the source defined by the instruction's
      operands.  We do the the move just like CMMoveValue() does it, by calling the common
      routine cmMoveValueHdr().   Before doing that we have to do some work to convert
      the operands, in the form of object IDs to object refNums.  We never trust anything
      as we do this!                                                                    */

      case InsertedValue:                               /* "inserted" value...          */
        fromObjectID   = (unsigned int )GET4(ioBuffer); /* ...get source IDs            */
        fromPropertyID = (unsigned int )GET4(ioBuffer);
        fromTypeID     = (unsigned int )GET4(ioBuffer);

        /* Convert the "from" object ID to its object refNum...                         */

        theFromObject = cmFindObject(container->toc, fromObjectID);
        if (theFromObject == NULL) {
          ERROR3(container,CM_err_NoUpdateObject, CONTAINERNAME,
                 cmltostr(fromObjectID, 1, false, errStr), "moved");
          return (ErrorCtl);
        }

        /* Find the property with the object...                                         */

        theFromProperty = cmGetObjectProperty(theFromObject, fromPropertyID);
        if (theFromProperty == NULL) {
          ERROR4(container,CM_err_NoUpdateProperty, CONTAINERNAME,
                 cmltostr(fromPropertyID, 1, false, errStr),
                 cmltostr(fromObjectID, 1, false, errStr1),
                 "moved");
          return (ErrorCtl);
        }

        /* Find the value with the desired type ID in the "from" object's property.     */
        /* This yields the "from" (source) value refNum to move.                        */

        theFromValueHdr = cmGetPropertyType(theFromProperty, fromTypeID);
        if (theFromValueHdr == NULL) {
#ifdef __MWERKS__
        /* OMFI - workaround for Metrowerks Code Warrior compiler bug */
            tmpstr = cmltostr(fromObjectID, 1, false, errStr2);
            ERROR4(container,CM_err_NoUpdateType, CONTAINERNAME,
                cmltostr(fromTypeID, 1, false, errStr),
                cmltostr(fromPropertyID, 1, false, errStr1),
                tmpstr);
#else
          ERROR4(container,CM_err_NoUpdateType, CONTAINERNAME,
                 cmltostr(fromTypeID, 1, false, errStr),
                 cmltostr(fromPropertyID, 1, false, errStr1),
                 cmltostr(fromObjectID, 1, false, errStr2));
#endif
          return (ErrorCtl);
        }

        /* Finally, we need the refNum of the target property...                        */

        theToProperty = cmFindObject(container->toc, propertyID);
        if (theToProperty == NULL) {
          ERROR4(container,CM_err_UndefUpdateObject, CONTAINERNAME, "property",
                 cmltostr(theValueHdr->theProperty->propertyID, 1, false, errStr),
                 "move");
          return (ErrorCtl);
        }

        /* Move the "from" value to the "to" (current target) object and property. This */
        /* potentially brings the property for the object into existence if this is the */
        /* first reference to the property for that object. The property object itslef, */
        /* of course, must already exist.  That's the check we did just above.          */

        theValueHdr = cmMoveValueHdr(theFromValueHdr, (CMObject)theObject, (CMProperty)theToProperty);
        if (theValueHdr == NULL) return (ErrorCtl);     /* must have value refNum now   */
        *theProperty = theValueHdr->theProperty;        /* set the property for value   */

        continue;                                       /* look for more updates        */

                                /*----------------*
                                 | SetInfoedValue |
                                 *----------------*

      Change the type and generation for the target value.  The instruction parameters
      contain the new values.                                                           */

      case SetInfoedValue:                              /* "set-infoed" value...        */
        newTypeID     = (unsigned int )GET4(ioBuffer);  /* ...get new values            */
        #if TOC1_SUPPORT
        if (container->majorVersion == 1)
          newGeneration = (unsigned int )GET2(ioBuffer);
        else
          newGeneration = (unsigned int )GET4(ioBuffer);
        #else
        newGeneration = (unsigned int )GET4(ioBuffer);
        #endif

        /* Validate that the type object exists...                                      */

        newTypeObject = cmFindObject(container->toc, newTypeID);
        if (newTypeObject == NULL) {
          ERROR4(container,CM_err_UndefUpdateObject, CONTAINERNAME, "type",
                 cmltostr(newTypeID, 1, false, errStr),
                 "set-info");
          return (ErrorCtl);
        }

        /* Must have a legal generation number too...                                   */

        if (newGeneration < 1) {
          ERROR2(container,CM_err_UpdateBadGenNbr, CONTAINERNAME,
                 cmltostr(newGeneration, 1, false, errStr));
          return (ErrorCtl);
        }

        theValueHdr->generation = newGeneration;        /* set new generation number    */
        theValueHdr->typeID = (CMObjectID)newTypeID;    /* ...and type ID               */

        continue;                                       /* look for more updates        */

                                  /*------------*
                                   | DeleteData |
                                   *------------*

      Delete data from a value.  This will cause "re-logicalizing" at the end.  The
      instruction parameters contain the offset from where to start deleting and the size
      size of the deletion.  The deletion is done just as CMDeleteValueData() does it, by
      calling the common routine cmDeleteSegmentData().                                 */

      case DeleteData:                                  /* "delete data"...             */
        offset = (unsigned int )GET4(ioBuffer);         /* ...get deletion parameters.. */
        size   = (unsigned int )GET4(ioBuffer);

        endOffset   = offset + size - 1;                /* ...get ending offset         */
        if (endOffset >= theValueHdr->size)
          endOffset = theValueHdr->size - 1;

        cmDeleteSegmentData(theValueHdr, offset, endOffset); /* delete the data         */

        reLogicalize = true;                            /* this requires re-logicalizing*/
        continue;                                       /* look for more updates        */

                             /*-------------------------*
                              | InsertData1/InsertDataN |
                              *-------------------------*

      Both data insertion cases are handled here.  The case of a single insertion is the
      same as for multi-segment insertions except the count is always 1.  The insertion
      is done by inserting the first segment just as CMInsertValueData() does it.  The
      common routine cmInsertNewSegment() is used to insert that first segment.  Here,
      we're given the offset and length to a segment.  CMInsertValueData(), on the other
      hand, must write the data passed to it by the user.  Either way the data is written
      by the time cmInsertNewSegment() is called.  It handles worrying about splitting the
      segment being inserted into if necessary.

      Once the first segment is inserted, all the other segments for a multi-segment
      insert are simply linked in following the new segment.

      Having said all this, there is one special case.  That is the case of "inserting"
      to the end of the current value (i.e., an append operation).  In that case all we
      need to do is append the new segment(s) to the target value.

      Since NEW segments for a value are being generated here, these segments must be
      tagged, like all others, with the container that "owns" them.  That is the passed
      container.  Unfortunatley, standard segment creation uses the value header to get
      at updatingContainer.  In the normal case, updatingContainer points to the container
      to the top-most updater to receive new updates.  But at the time we're applying
      updates, we don't want that container because we're not necessarily at the top yet.
      So we have to retag the segment with the current container.

      Like delete, inserts cause "re-logicalizing" at the end.                          */

      case InsertData1:                                 /* insert 1 new segment...      */
      case InsertDataN:                                 /* insert N new segments...     */

        /* Get the insert point and number of inserts, n.  For InsertData1, n is 1.     */

        insertPoint = (unsigned int )GET4(ioBuffer);
        n           = (ctlByte == InsertData1) ? 1UL : (unsigned int )GET4(ioBuffer);
        if (n == 0) {
          ERROR1(container,CM_err_BadInsertData, CONTAINERNAME);
          return (ErrorCtl);
        }

        /* If we're apppending the data instead of actually inserting it, then just     */
        /* prepare to append new segments to the end of the existing ones.  If it is a  */
        /* true insertion, then we must process the first segment specially. It must be */
        /* inserted just like CMInsertValueData() would do it.                          */

        offset = (unsigned int )GET4(ioBuffer);         /* get 1st seg params...        */
        size   = (unsigned int )GET4(ioBuffer);

        if (insertPoint == theValueHdr->size) {         /* if appending...              */
          gotData        = true;                        /* indicate we have 1st params  */
          theInsertValue = (TOCValuePtr)cmGetListTail(&theValueHdr->valueList);
        } else {                                        /* if inserting...              */

          /* Just as in CMInsertValueData(), find out which existing segment we're going*/
          /* to insert into and the offset within that segment...                       */

          insBeforeValue  = cmGetStartingValue(theValueHdr, insertPoint, &segOffset);
          if (insBeforeValue == NULL) {
            ERROR2(container,CM_err_BadInsertOffset, CONTAINERNAME, cmltostr(insertPoint, 1, false, errStr));
            return (ErrorCtl);
          }

          theInsertValue = cmInsertNewSegment(theValueHdr, insBeforeValue, segOffset, offset, size);
          if (theInsertValue == NULL) return (ErrorCtl);

          theInsertValue->container = container;        /* set proper owner of this seg */

          --n;                                          /* count 1st segment processed  */
          gotData = false;                              /* we need to read if more segs */
        }

        /* The remaining segments (if any) are just inserted after the one inserted     */
        /* above.  If we're just appeneding, all the segments are handled here.         */

        while (n--) {                                   /* insert/append remaining segs */
          if (!gotData) {                               /* get i'th segment params...   */
            offset = (unsigned int )GET4(ioBuffer);
            size   = (unsigned int )GET4(ioBuffer);
          }

          /* Create a new value segment using the updating offset/size values...        */

          (void)cmSetValueBytes(container, &valueBytes, Value_NotImm, offset, size);
          theValue = cmCreateValueSegment(theValueHdr, &valueBytes, 0);
          if (theValue == NULL) return (ErrorCtl);

          theValue->container = container;              /* set proper owner of this seg */

          /* Insert/append the new segment and set the associated flags and value header*/
          /* size...                                                                    */

          cmInsertAfterListCell(&theValueHdr->valueList, theValue, theInsertValue);

          theInsertValue->flags = (unsigned short)(theInsertValue->flags | kCMContinued);
          theValueHdr->valueFlags |= ValueContinued;
          theValueHdr->size += size;

          gotData        = false;                       /* need info for each new seg   */
          theInsertValue = theValue;                    /* set to insert next segment   */
        } /* while */

        reLogicalize = true;                            /* this requires re-logicalizing*/
        continue;                                       /* look for more updates        */

                                /*------------------*
                                 | ReplaceImmediate |
                                 *------------------*

      Any immediate that was edited in a previous session and was recored as an update at
      close time is treated specially.  Rather than recording individual updates, the
      entire immediate is output as a "replace immediate" update instruction.  That
      replacement is to be done now (at open time).  As a safety check we make sure that
      the value we're about to replace is indeed an immediate.  It should be!

      Note, re-logicalizing is currently not necessary since there can be only one
      segment for continued values...                                                   */

      case ReplaceImmediate:                            /* "replace immediate"...       */
        theValue = (TOCValuePtr)cmGetListHead(&theValueHdr->valueList);
        if ((theValue->flags & kCMImmediate) == 0) {
          ERROR1(container,CM_err_CantRepImmediate, CONTAINERNAME);
          return (ErrorCtl);
        }

        size         = (unsigned int )GET1(ioBuffer);   /* get new size                 */
        newImmediate = (unsigned int )GET4(ioBuffer);   /* and immediate                */

        omfsCvtUInt32toInt64(size, &theValue->value.notImm.valueLen);  /* replace the immediate...     */
        theValue->value.imm.ulongValue  = newImmediate;
        omfsCvtUInt32toInt64(size, &theValueHdr->size);
        continue;                                       /* look for more updates        */

                                 /*-----------------*
                                  | ReplaceBaseType |
                                  *-----------------*

      A replaced base type is similar to a replaced immediate.  Base type values are
      segments of immediate data wich each immediate being a base type ID.  The entire
      base type is replaced with the values from the updating instruction's parameters.
      A ssafety check here is that the referenced value is indeed a base type.          */

      case ReplaceBaseType:                             /* replace base type...         */
        if (theValueHdr->typeID != CM_StdObjID_BaseTypeList) {
          ERROR1(container,CM_err_CantRepBaseType, CONTAINERNAME);
          return (ErrorCtl);
        }

        /* There are N base type parameters.  Knowing that we know how to set the value */
        /* header info...                                                               */

        nBases = (unsigned short)GET2(ioBuffer);        /* get nbr of bases             */

        if (nBases > 1)                                 /* set echoed "continued" bit   */
          theValueHdr->valueFlags |= ValueContinued;    /* set if N > 1                 */
        else
          theValueHdr->valueFlags &= ~ValueContinued;   /* reset if N = 1               */
        theValueHdr->size = nBases * sizeof(CMObjectID);/* set new total size           */

        /* replace existing segments with the new base type values as int  as there are */
        /* enough segments.  After that we must create new segments.                    */

        theValue = (TOCValuePtr)cmGetListHead(&theValueHdr->valueList);

        while (nBases--) {                              /* create N segments...         */
          baseTypeID = (unsigned int )GET4(ioBuffer);   /* get next base ID             */

          if (theValue == NULL) {                       /* create seg if no more to use */
            cmSetImmValueBytes(container, &valueBytes, Value_Imm_Long, baseTypeID, sizeof(CMObjectID));
            theValue = cmAppendValue(theValueHdr, &valueBytes, kCMImmediate);
            theValue->container = container;
            if (nBases > 0) theValue->flags |= kCMContinued;
            theValue = NULL;
          } else {                                      /* replace existing base type   */
            theValue->value.imm.ulongValue = baseTypeID;
            if (nBases > 0)
              theValue->flags |= kCMContinued;
            else
              theValue->flags &= ~kCMContinued;
            theValue = (TOCValuePtr)cmGetNextListCell(theValue);
          }
        }

        /* If the replacement used less segments than what was originally there, delete */
        /* the excess segments...                                                       */

        while (theValue != NULL) {                      /* delete excess segments...    */
          theNextValue = (TOCValuePtr)cmGetNextListCell(theValue);
          CMfree(container, cmDeleteListCell(&theValueHdr->valueList, theValue));
          theValue = theNextValue;
        }

        continue;                                       /* look for more updates        */

                              /*----------------------*
                               | Unrecognized Control |
                               *----------------------*

      An unrecognized control byte has been encountered.  If any editing was done to the
      data, the value must be "re-logicalized".  The unrecognized control byte is
      returned to applyValueUpdates() to process.                                       */

      default:                                          /* unrecognized control byte... */
        if (reLogicalize) {                             /* if we must "re-logicalize"...*/
          size = 0;                                     /* ...do it...                  */
          theValue = (TOCValuePtr)cmGetListHead(&theValueHdr->valueList);

          while (theValue) {                            /* set new logical offsets      */
            theValue->logicalOffset = size;
            size += theValue->value.notImm.valueLen;
            theValue = (TOCValuePtr)cmGetNextListCell(theValue);
          }

          theValueHdr->logicalSize = theValueHdr->size = size; /* set new logical size  */
        }

        return (ctlByte);                               /* return unrecognized ctl byte */
    } /* switch */
  } /* for */
}


/*-----------------------------------------------------------------------------------*
 | applyValueUpdates - main control interpreter to apply value updating instructions |
 *-----------------------------------------------------------------------------------*

 This routine is called only by cmApplyUpdates() to control the interpretation of value
 updating instructions.  All objects that contain an updating property were put on the
 touched chain as the TOC was read.  Here we walk that chain and process that property.
 For each chained object, the updating property has a value whose data is all the updating
 instructions for its object.

 False is returned for failure, and true for success.  Read errors will longjmp back to
 cmApplyUpdates().

 This routine is the symmetric counterpart to its generator, generateValueUpdates().  It
 generates the updating instructions in the following sequence:

                                    --                                    --
                                    |  ( <new prop> P T <v> [params...] )  |
    <new prop> P T <v> [params...]  |  <                                >  | ... <end>
                                    |  ( <new type> T <v> [params...]   )  |
                                    --                                    --

 See generateValueUpdates() for the notation and meaning.
*/

static Boolean CM_NEAR applyValueUpdates(ContainerPtr container, void *ioBuffer)
{
  TOCObjectPtr      theObject, nextTouchedObject;
  TOCPropertyPtr    theProperty;
  TOCValueHdrPtr    updatesValueHdr;
  unsigned int      propertyID, typeID;
  register CM_UCHAR ctlByte;
  char              errStr[15];

  /* Walk the entire touched chain built of objects because each such object contains a */
  /* special "updates" property detected when the TOC was read in.                      */

  theObject = container->touchedChain;                /* point to 1st touched object    */

  while (theObject != NULL) {                         /* walk touched chain...          */
    nextTouchedObject = theObject->nextTouchedObject; /* get ptr to next object early   */

    /* To make it easier and more efficient for us to get at the special property and   */
    /* its value header (which is what we're really after), when the TOC reader put the */
    /* object on the touched chain, it set the object's refCon to point at the updating */
    /* instructions value header.  That's what we use to read the instruction value     */
    /* data.                                                                            */

    updatesValueHdr = (TOCValueHdrPtr)theObject->objectRefCon; /* "updates" value hdr   */

    if (updatesValueHdr != NULL) {                    /* the test is just for safety!   */
      cmNewBufferedInputData(ioBuffer, updatesValueHdr, updatesValueHdr->size);
      theProperty = NULL;                             /* NULL means haven't processed ID*/

      /* At this point we have one set of updating instructions for one object.  Here   */
      /* we expect property and type changes.  The object, of course, we know.  The     */
      /* SPECIFIC value updating interpretation is done separately, just like it was    */
      /* generated, by genValueUpdateInstructions().                                    */

      ctlByte = GET1(ioBuffer);                       /* read ahead in this "scan"      */

      while (ctlByte != EndUpdates)                   /* stop at <end>                  */
        switch (ctlByte) {                            /* process <new prop>/<new  type> */

          /* The "<new prop> P T" merges with the "<new type> T" case to get the type   */
          /* ID ("T").  Both these cases can be seen first in the sequence.             */

          case NewProperty:                           /* <new prop>                     */
            propertyID = (unsigned int )GET4(ioBuffer);/*           P                   */
            theProperty = NULL;

                              /* FALL THROUGH to NewType case */

          /* Handle the "<new type> T" case for multiple types for the same property,   */
          /* and complete the "<new prop> P T" case falling into here from above.  This */
          /* yields the property/type pair to uniquely address a target value for the   */
          /* current object.                                                            */

          case NewType:                               /* <new type>                     */
            typeID = (unsigned int )GET4(ioBuffer);   /*           T                    */

            /* A "<v> [params...]" sequence should always follow "<new prop> P T" and   */
            /* "<new type> T".  These sequences are processed separately by the routine */
            /* applyValueUpdateInstructions().  The "proper" amount of updating info is */
            /* consumed (read in). The "[params...]" are recursive in that it may       */
            /* itself contain "<v> [params...]" for additional updates to the same      */
            /* value. Thus applyValueUpdateInstructions() will be reading "ahead" until */
            /* it finds an "unrecognizable" control byte it returns that here.  So we   */
            /* already have the next control byte to process on return. This should be  */
            /* either <new prop>, <new type>, or <end>. If any errors are detected, a   */
            /* special "control", ErrorCtl, is returned so we know to quit.             */

            ctlByte = applyValueUpdateInstructions(container, theObject, &theProperty,
                                                   propertyID, typeID, ioBuffer);
            if (ctlByte == ErrorCtl) return (false);

            break;                                    /* process next control           */

          default:                                    /* huh?                           */
            ERROR3(container,CM_err_BadUpdateControl, CONTAINERNAME,
                   cmltostr(ctlByte, -2, true, errStr), "updating");
            return (false);
        } /* switch, while */

        /* All the updating instructions for the current object have now been processed.*/
        /* We now have to free the property so that a new one can be generated at close */
        /* time if this object happens to receive new updates.  Besides, why horde all  */
        /* that wonderful memory?                                                       */

        cmDeleteProperty(container, updatesValueHdr->theProperty);
    } /* updatesValueHdr */

    /* At this point we're done updating this object (unless it gets deleted by phase   */
    /* 2 -- applyDeleteUpdates()).  The object is removed from the touched chain and its*/
    /* refCon reset to NULL.  By the time we process the entire touched chain, it will  */
    /* end up being empty ready for recording NEW updates, if any.                      */

    deleteObjFromTouchedChain(container, theObject, NULL);

    theObject->objectRefCon = (CMRefCon)NULL;         /* put refCon back to orig state  */
    theObject = nextTouchedObject;                    /* process next touched object    */
  } /* while (theObject)... */

  return (true);                                      /* we actually made it!           */
}


/*-------------------------------------------------------------------------------------*
 | applyDeleteUpdates - interpreter to apply object and property deletion instructions |
 *-------------------------------------------------------------------------------------*

 This routine is called only by cmApplyUpdates() to control the interpretation of object
 and property deletion updating instructions.  The deletesValueHdr is the refNum for the
 TOC #1 deletes property, if one is present.  If there are no deletions, it will be NULL.

 False is returned for failure, and true for success.  Read errors will longjmp back to
 cmApplyUpdates().

 This routine is the symmetric counterpart to its generator, generateDeleteUpdates().  It
 generates the updating instructions in the following sequence:

    ( <del obj> O                         )
    <                                     > ... <end>
    ( <del prop1> O P [<del prop2> P ...] )

 See generateDeleteUpdates() for the notation and meaning.  It is also described there
 why the deletions are kept separate and must be processed last.
*/

static Boolean CM_NEAR applyDeleteUpdates(ContainerPtr container, void *ioBuffer)
{
  TOCObjectPtr      theObject;
  TOCPropertyPtr    theProperty;
  TOCValueHdrPtr    deletesValueHdr = container->deletesValueHdr;
  register CM_UCHAR ctlByte;
  CMObjectID        objectID, propertyID;
  void              *toc;
  char              errStr[15], errStr1[15];

  if (deletesValueHdr == NULL)                        /* exit if nothing to delete      */
    return (true);

  toc = container->toc;                               /* make things a little faster!   */
  theObject = NULL;                                   /* NULL to safety check this      */

  /* Process the updating instruction data of the TOC #1 "deletes" property...          */

  cmNewBufferedInputData(ioBuffer, deletesValueHdr, deletesValueHdr->size);

  while ((Boolean)((ctlByte = GET1(ioBuffer)) != EndUpdates))
    switch (ctlByte) {

      /* Both "<del obj> O" and "<del prop1> O P" expect an object ID ("O") first. Thus */
      /* both cases are processed together for the O.  Both these cases can be seen     */
      /* first in the sequence.                                                         */

      case DeleteProperty1:                           /* <del prop1>                    */
      case DeleteObject:                              /* <del obj>                      */
        objectID  = (CMObjectID)GET4(ioBuffer);       /*          O                     */
        theObject = cmFindObject(toc, objectID);

        /* If only "<del obj> O" then just delete the object and go get next control    */
        /* byte.  Otherwise we still need the "P" for the "<del prop1> O P" case.       */

        if (ctlByte == DeleteObject) {                /* <del obj> O ==> just delete obj*/
          if (theObject == NULL) {
            ERROR3(container,CM_err_NoUpdateObject, CONTAINERNAME,
                   cmltostr(objectID, 1, false, errStr), "deleted");
            return (false);
          }
          cmMarkObjectDeleted(container, theObject);
          break;                                      /* process next control byte      */
        }

                        /* FALL THROUGH to DeleteProperty2 case */

      /* Get the property ID ("P") for the "<del prop1> O P" case or completely process */
      /* the "<del prop2> P" case for additional properties for the same object. In     */
      /* both these cases the property for the current object is deleted.  Note that a  */
      /* "<del prop2> P" case should never be first in the deletion sequence, i.e., we  */
      /* better have an object.                                                         */

      case DeleteProperty2:                           /* <del prop2>                    */
        if (theObject == NULL) {
          ERROR3(container,CM_err_NoUpdateObject, CONTAINERNAME, "?", "deleted");
          return (false);
        }

        propertyID = (CMObjectID)GET4(ioBuffer);      /*            P                   */
        theProperty = cmGetObjectProperty(theObject, propertyID);
        if (theProperty == NULL) {
          ERROR4(container,CM_err_NoUpdateProperty, CONTAINERNAME,
                 cmltostr(propertyID, 1, false, errStr),
                 cmltostr(objectID, 1, false, errStr1),
                 "deleted");
          return (false);
        }

        cmDeleteProperty(container, theProperty);     /* delete property from its object*/
        break;                                        /* process next control byte      */

      default:                                        /* huh?                           */
        ERROR3(container,CM_err_BadUpdateControl, CONTAINERNAME,
               cmltostr(ctlByte, -2, true, errStr), "deleting");
        return (false);
    } /* switch, while */

  /* All the deletions have been done.  The TOC #1 property memory is freed  since it   */
  /* is not needed any longer.                                                          */

  cmDeleteProperty(container, deletesValueHdr->theProperty);
  container->deletesValueHdr = NULL;

  return (true);                                      /* incredible? isn't it?          */
}


/*-------------------------------------------------------------------*
 | cmApplyUpdates - apply all updates to bring a target "up to date" |
 *-------------------------------------------------------------------*

 This is called at open time (openTargetContainer()) to apply (execute) all the updating
 instructions for all objects and their values "touched" while loading in the non-private
 portion of an updating container's TOC.  True is returned if successful and false
 otherwise (and the error reporter returns).

 The touched objects are on the touched chain pointed to from the passed container control
 block.  These were objects containing a special "updates" property.  Such a property has
 a value whose value data is all the updating instructions dealing with that object.  The
 reason they were chained was so that we could get at the objects with that property and
 do it at a time when we know we have all the objects read in.  During the read there could
 be references in the instructions for as-yet unread objects.

 To make it easier (and more efficient) to get at the "updates" property value for the
 objects on the touched chain, such objects have had their refCon set with the refNum
 (pointer) of the updating instruction value header.  That was done at the same time the
 object was placed on the touched chain.  We'll clear it to NULL as we process those
 objects.

 This routine is more-or-less the symmetric counterpart to cmGenerateUpdates() above,
 which generated the instructions we're about to read here.  That generation is done in
 two passes to generate two groups of updates; first the value updates, and then the
 object/property deletions.  The value updates are the instructions attached to each
 object and that is what caused the touched chain to be built that we are going to process
 here.  The deletions are attached to a special TOC #1 "deletes" property.  We process
 those AFTER the value updates are processed (more about this below).

 Thus, rather than two passes over the touched chain that we do in generating the updates,
 we have two phases here:

    Phase 1: All updating instructions are processed for each object on the touched chain.
             This will update all the values appropriately.  Since the updating
             instructions are in terms of the original "OPT addresses" of the values we
             know how to find them.  Moves can be made.

             Each object processed is removed from the touched chain.  Thus at the end of
             phase 1, the touched chain is empty and ready to receive REAL update touches.

    Phase 2: The "deletes" TOC #1 property is used to get the deletion updates list.  All
             object and property deletes are done at this time.  This must be done last
             in order for phase 1 to work correctly. The problem is with moves interacting
             with deletes.  If a value is moved from A to B, and then A (its original
             location) deleted, then the updates to that value would not have A to refer
             to. We always need the ORIGINAL "OPT" addresses of the values.

 A note about the container and updatingContainer pointers...

 At the time the non-private TOC was loaded, and when we're called here, the container
 pointer is obviously the container that just received new values and whose updates are
 to be applied to a target container.  The "target" takes the form of the TOC which
 everyone is using at this point.  In order to have tagged new values with this container
 and in order to be able to get at the touched chain, the updatingContainer pointer is
 temporarily set to be the SAME as the container pointer, i.e., it points to its own
 container.  This is fully described in openTargetContainer().  It's sufficient to know
 that in the context of applying these updates, the container and updatingContainer
 pointers are equal in here.
*/

Boolean cmApplyUpdates(ContainerPtr container)
{
  Boolean        success;
  unsigned short savedUseFlags;
  void           *ioBuffer = NULL;
  jmp_buf        applyUpdatesEnv;

  /* Make sure there's something to do!  We must have either or both a touched chain    */
  /* and deletion list.  The deletion list "refNum" is saved in deletesValueHdr.        */

  if (container->touchedChain == NULL && container->deletesValueHdr == NULL)
    return (true);

  /* "Adjust" the world to allow us to apply updates to a target container that opened  */
  /* for reading.  "Adjusting" here means that we pretend the container was opened for  */
  /* writing.  Further, we do not track free space created by deletion updates, and we  */
  /* free the refNums from those deletions rather than putting them keeping track of    */
  /* them as we normally do (to guard against user reuse, which we obviously don't have */
  /* here).                                                                             */

  savedUseFlags = container->useFlags;          /* save container useFlags              */
  container->useFlags |= kCMWriting;            /* set opened for writing to update     */
  container->trackFreeSpace = false;            /* stop free space tracking             */
  cmChangeRefNumHandling(container->toc, false);/* allow freeing of obj/value refNums   */

  /* Rather than check for read failures on every read, a setjmp/longjmp environment is */
  /* set up.  The buffered read routine, readUpdate(), will longjmp back here if it     */
  /* detects and reports an error and the error reporter returns.                       */

  if (setjmp(applyUpdatesEnv)) {                /* set setjmp/longjmp environment vector*/
    cmReleaseIOBuffer(ioBuffer);                /* ...read err if longjmp taken...      */
    container->useFlags = savedUseFlags;        /* ...restore the original useFlags     */
    container->trackFreeSpace = true;           /* ...track free space again            */
    cmChangeRefNumHandling(container->toc,true);/* ...restore refNum tracking           */
    ERROR1(container,CM_err_BadUpdateRead, CONTAINERNAME);/* ...report error                      */
    return (false);                             /* ...I bet we have a pissed off user!  */
  }

  /* Allocate the input buffer to more efficiently load the update data...              */

  ioBuffer = cmUseIOBuffer(container, UpdateBufSize, (jmp_buf *)&applyUpdatesEnv);

  /* Process both phases...                                                             */

  if (ioBuffer != NULL)
    success = (Boolean)(applyValueUpdates(container, ioBuffer) && /* Phase 1: values    */
                        applyDeleteUpdates(container, ioBuffer)); /* Phase 2: deletions */
  else
    success = false;

  /* Either of the above routines returns "false" if they fail for any reason (excluding*/
  /* read failures, which take the setjmp/longjmp exit above).  Either way we give back */
  /* the buffer space, undo the "damage" we did, and then return success or failure.    */

  cmReleaseIOBuffer(ioBuffer);                  /* say good bye to the input buffer     */
  container->useFlags = savedUseFlags;          /* restore the original useFlags        */
  container->trackFreeSpace = true;             /* reenable free space tracking         */
  cmChangeRefNumHandling(container->toc, true); /* restore refNum tracking              */

  return (success);                             /* true ==> continue close processing   */
}

                              CM_END_CFUNCTIONS
#endif
