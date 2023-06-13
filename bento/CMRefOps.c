/*---------------------------------------------------------------------------*
 |                                                                           |
 |                         <<<    CMRefOps.c    >>>                          |
 |                                                                           |
 |               Container Manager Object Reference Operations               |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 10/15/92                                  |
 |                                                                           |
 |                     Copyright Apple Computer, Inc. 1992                   |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 An object's value may contain data that refers to other objects.  This file contains
 all the API routines that allow a user to embed object ID references in value data.  The
 routines defined here allow that data to be manipulated without the API user explicitly
 knowing the form such object references take.

 But, just for the record, we better describe how the references are maintained here!

 When a reference is to be saved in some value, the user calls CMNewReference() or
 CMSetReference().  A reference key is to be associated with some object.  The user
 retrieves objects via their keys.  We record the key/object (ID) associations.  They
 are recorded in a private recording object which is "tied" to the user's value from a
 field (refDataObject) in the value header.

 The recording object has one special property and one specially typed value for that
 property.  The value data for that value is the list of key/object ID associations.  They
 are formatted as 8-byte entries; 4 bytes for the key (which comes first), and 4 bytes for
 the corresponding object ID.

 This data is a linear list on the assumption there won't be "too many" references in each
 value.  We could be wrong about this.

 The list is maintained as data because it is subject to updating like any other data!
 The price we pay for this, however, is to do handler I/O to read and write this data.
 To gain some efficiency, buffered I/O, using the routines in  BufferIO.c , is used.
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
#ifndef __TOCENTRIES__
#include "TOCEnts.h"
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
#ifndef __HANDLERS__
#include "Handlers.h"
#endif
#ifndef __BUFFEREDIO__
#include "BufferIO.h"
#endif
#ifndef __LISTMGR__
#include "ListMgr.h"
#endif
#ifndef __REFERENCES__
#include "Refs.h"
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
#pragma segment CMReferenceOps
#endif


/* The references are always searched for based on the reference key in the data.  One  */
/* common routine, getReference(), is used to do this searhing.  It returns the         */
/* following status codes to indicate the results of its search.                        */

enum RefSearchStatus {                        /* getReference() search status:          */
  RefReadError,                               /*    some read error occurred            */
  RefFound,                                   /*    returned offset == position & ID    */
  RefNotFound                                 /*    offset == list size, ID undefined   */
};
typedef enum RefSearchStatus RefSearchStatus;


#if CMSHADOW_LIST
/*----------------------------------------------------------------------------------------*
 | appendShadowListEntry - create/append in a single shadow list entry to its shadow list |
 *----------------------------------------------------------------------------------------*

 This internal routine creates and then appends a single shadow list entry to the specified
 shadow list.  The list header pointer, key, and its associated object ID are passed.  The
 container is used for allocating the entry and error reporting.

 The function returns a pointer to the created entry.  NULL is returned if an error is
 reported for an allocation failure and the error reporter returns.
*/

static RefDataShadowEntryPtr CM_NEAR CM_PASCAL appendShadowListEntry(ContainerPtr container,
                                                                     ListHdrPtr refShadowList,
                                                                     unsigned int  key,
                                                                     CMObjectID objectID)
{
  RefDataShadowEntryPtr refShadowEntry;

  refShadowEntry = (RefDataShadowEntryPtr)CMmalloc(container, sizeof(RefDataShadowEntry));
  if (refShadowEntry == NULL) {
    ERROR1(container, CM_err_NoRefShadowList, CONTAINERNAME);
    return (NULL);
  }

  cmNullListLinks(refShadowEntry);
  refShadowEntry->key      = key;
  refShadowEntry->objectID = objectID;

  return ((RefDataShadowEntryPtr)cmAppendListCell(refShadowList, refShadowEntry));
}


/*----------------------------------------------------------------------------*
 | deleteShadowListEntry - delete a single recording object shadow list entry |
 *----------------------------------------------------------------------------*

 This internal routine is used to shadow CMDeleteReference(), i.e., to delete the
 corresponding shadow list entry.  The recording object's value header and the pointer to
 the entry to be deleted are passed.

 The pointer is the value returned by getReference() when CMDeleteReference() called it to
 find the reference entry to be deleted.

 If the last list entry is deleted, the list header itself is deleted and the pointer in
 the value header set to NULL to indicate there is no shadow list.
*/

static void CM_NEAR CM_PASCAL deleteShadowListEntry(TOCValueHdrPtr refDataValueHdr,
                                                    RefDataShadowEntryPtr refShadowEntry)
{
  ContainerPtr container = refDataValueHdr->container->targetContainer;
  ListHdrPtr   refShadowList = RefShadowList(refDataValueHdr);

  if (refShadowList != NULL && refShadowEntry != NULL) {
    CMfree(container, cmDeleteListCell(refShadowList, refShadowEntry));  /* delete the entry       */
    if (cmIsEmptyList(refShadowList)) {                       /* if no more entries...  */
      CMfree(container, refShadowList);                                  /* ...delete the list hdr */
      RefShadowList(refDataValueHdr) = NULL;                  /* ...indicate no list    */
    }
  }
}


/*-----------------------------------------------------------------------------*
 | cmDeleteRefDataShadowList - delete an entire recording object's shadow list |
 *-----------------------------------------------------------------------------*

 This routine is called to delete the entire shadow list pointed to from the specified
 recording object's value header.  It is used for clearing the list during error recovery
 and value (header) deletions.

 Note, generally the caller should have done a HasRefShadowList(refDataValueHdr) prior to
 calling this routine to make sure that the value header is indeed a recording object
 value header.
*/

void cmDeleteRefDataShadowList(TOCValueHdrPtr refDataValueHdr)
{
  ContainerPtr          container = refDataValueHdr->container->targetContainer;
  ListHdrPtr            refShadowList = RefShadowList(refDataValueHdr);
  RefDataShadowEntryPtr refShadowEntry, nextEntry;

  if (refShadowList != NULL) {
    refShadowEntry = (RefDataShadowEntryPtr)cmGetListHead(refShadowList);

    while (refShadowEntry) {                      /* delete all the list entries...     */
      nextEntry = (RefDataShadowEntryPtr)cmGetNextListCell(refShadowEntry);
      CMfree(container,refShadowEntry);
      refShadowEntry = nextEntry;
    }

    CMfree(container, refShadowList);                        /* delete the list header             */
    RefShadowList(refDataValueHdr) = NULL;        /* indicate there's no shadow list    */
  }
}
#endif


/*---------------------------------------------------------------------------*
 | getRecordingValueHdr - get recording object's reference list value header |
 *---------------------------------------------------------------------------*

 This is an internal routine used to return the value (header) pointer (refNum) for the
 recording object's object/reference association list.  NULL is returned if the recording
 object doesn't exist or an error is reported and the error reporter returns.

 The only possible errors here are that the property or the property's value header do not
 exist in the recording object.  This shoould never happen.  But we check it anyway.
*/

static TOCValueHdrPtr CM_NEAR CM_PASCAL getRecordingValueHdr(TOCValueHdrPtr theValueHdr)
{
  ContainerPtr   container;
  TOCPropertyPtr refDataProperty;
  TOCValueHdrPtr refDataValueHdr;

  if (!HasRefDataObject(theValueHdr))                     /* if no recording object...  */
    return (NULL);                                        /* ...not found               */

  /* Validate that we have the "proper" property and type in the recording object...    */
  /* Call me paranoid (ok, you're paranoid!)                                            */

  refDataProperty = cmGetObjectProperty(RefDataObject(theValueHdr), CM_StdObjID_ObjReferences);
  if (refDataProperty) {
    refDataValueHdr = (TOCValueHdrPtr)cmGetListHead(&refDataProperty->valueHdrList);
    if (refDataValueHdr && refDataValueHdr->typeID != CM_StdObjID_ObjRefData)
      refDataValueHdr = NULL;
  }
  if (refDataProperty == NULL || refDataValueHdr == NULL){/* gee, what went wrong?      */
    container = theValueHdr->container->targetContainer;  /* this is an internal error! */
    ERROR1(container, CM_err_Internal6, CONTAINERNAME);
    return (NULL);
  }

  return (refDataValueHdr);                               /* return recording value hdr */
}


/*----------------------------------------------------------------------*
 | getReference - find a reference recorded in a value's reference list |
 *----------------------------------------------------------------------*

 This is an internal routine used to search a recording object's reference list for
 theReferenceData "key". The reference list is value data for the specified refDataValueHdr
 (the value header for the recording object's reference list).  The result of the search
 is returned as the function result along with an offset and object ID.  The meaning of the
 returned key, objectID, and refShadowEntry are a function of the returned search status as
 follows:

   RefFound:     The key was found.
                 key            = internal (hardware) representation of the CMReference key.
                 objectID       = object ID associated with the found key.
                 refShadowEntry = ptr to found shadow list entry (NULL if no shadow list).
                 offset         = value data offset to the key.

   RefNotFound:  The key was not found.
                 key            = internal (hardware) representation of the CMReference key.
                 objectID       = undefined.
                 refShadowEntry = NULL.
                 offset         = current size of the value data.  This allows the caller
                                  to append the new reference if necessary.

   RefReadError: A read error has occurred and the error reporter returned.  Caller must
                 not "go on".

 Note, the returned key is the hardware (unsigned int ) representation of the input
 CMReference theReferenceData key.  This is converted here to make it easier to work with.
 So for convenience it is given back to the caller.  However, the caller may have already
 did this for the same reason.  So, by convention, if the key is PASSED IN as 0, the
 conversion is done here.  If it is nozero, it is assumed the caller did it, and the key
 is used as passed, i.e., it is assumed that theReferenceData was converted to key.

                                The Shadow List
                                ===============

 Under configuation option (CMSHADOW_LIST) a shadow list is created here.  The "shadow
 list" is a copy of the actual value data (thus it "shadows" the data - tricky name 'eh?).
 All changes to the data are shadowed in the list.  But the list is in internal (hardware)
 format for the keys and object IDs and in memory to make these searches we do here more
 efficient than reading it each time.  Of course, we have to read it the first time.
*/

static RefSearchStatus CM_NEAR CM_PASCAL getReference(TOCValueHdrPtr refDataValueHdr,
                                                      CMReference theReferenceData,
                                                      CMCount *offset,
                                                      unsigned int  *key,
                                                      CMObjectID *objectID,
                                                      RefDataShadowEntryPtr *refShadowEntry)
{
  ContainerPtr          container = refDataValueHdr->container->targetContainer;
  unsigned int          x, currKey, size, xferSize;
  CMObjectID            currObjectID;
  RefSearchStatus       searchStatus = RefNotFound;
  void                  *ioBuffer = NULL;
  jmp_buf               getRefEnv;
  #if CMSHADOW_LIST
  ListHdrPtr            refShadowList;
  RefDataShadowEntryPtr r;
  #endif

  (void)omfsTruncInt64toUInt32(refDataValueHdr->size, &size);

  /* The user's key is a CMReference which are awkward to work with internally.  So all */
  /* manipulations of the key are done in internal (i.e., hardware) format.  To avoid   */
  /* unnecessarily doing this conversion a second time if the caller already did it,    */
  /* the key can be passed as non-zero.  Zero indicates we are to convert it here.      */

  if (*key == 0)                                          /* if caller didn't supply key*/
    CMextractData(container, theReferenceData, 4, key);   /* ...convert key to internal */

  /* If the shadow list already exists, search it now since we know it's in sync        */
  /* (hopefully) with the recording object's actual reference list value data.          */

  #if CMSHADOW_LIST
  if (RefShadowList(refDataValueHdr) != NULL) {           /* if shadow lists exists...  */
    r = (RefDataShadowEntryPtr)cmGetListHead(RefShadowList(refDataValueHdr));
    omfsCvtUInt32toInt64(0, offset);                       /* maintain value data offset */

    while (r) {                                           /* search the list...         */
      if (r->key == *key) {                               /* ...if the key is found...  */
        *objectID = r->objectID;                          /* ...return its object ID    */
        *refShadowEntry = r;                              /* ...return shadow entry ptr */
        return(RefFound);                                 /* ...return key found status */
      }
      r = (RefDataShadowEntryPtr)cmGetNextListCell(r);    /* ...get next shadow entry   */
      omfsAddInt32toInt64(sizeof(ReferenceData), offset); /* ...account for value data  */
    }

    *refShadowEntry = NULL;                               /* the key was not found      */

    return (RefNotFound);
  }

  /* If the shadow list doesn't exist for the recording value create it now.  First the */
  /* list header which is pointed to from the value's header.  The pointer is the same  */
  /* field as the recording object pointer. It is a union to give it a more appropriate */
  /* name. We always know the difference because the recording object's value header is */
  /* uniquely typed.                                                                    */

  refShadowList = RefShadowList(refDataValueHdr) = (ListHdrPtr)CMmalloc(container, sizeof(ListHdr));
  if (refShadowList == NULL) {
    ERROR1(container,CM_err_NoRefShadowList, CONTAINERNAME);
    return (RefReadError);
  }
  cmInitList(refShadowList);
  #endif

  /* There's nothing to do if there are no references.  If we were called from          */
  /* CMSetReference(), it will create the first entry.                                  */

  if (size == 0) {                                        /* if no data to read...      */
    omfsCvtUInt32toInt64(0, offset);                                          /* ...return current size     */
    *refShadowEntry = NULL;                               /* ...no shadow entry         */
    return (RefNotFound);                                 /* ...return key not found    */
  }

  /* Read in the reference data to build the shadow list this first time if the config  */
  /* allows it. All further entry modifications are done by the callers to shadow their */
  /* respective operations to the recording value data in the list.  While we're at it, */
  /* we do the required search for the desired key.  If we're not building the shadow   */
  /* list, we can break out of the loop as soon as we find the key.  Otherwise, the     */
  /* entire list must be read in.                                                       */

  /* Note, in a (crude) attempt to make this read and search more efficient (after all  */
  /* we have to read read value data here), we use buffered value I/O (what the hell).  */
  /* See  BufferIO.c  for further details on how buffered value data I/O works.         */

  if (setjmp(getRefEnv)) {                                /* ...set for buffering errors*/
    cmReleaseIOBuffer(ioBuffer);                          /* ...if longjmp taken to here*/
    #if CMSHADOW_LIST
    cmDeleteRefDataShadowList(refDataValueHdr);
    #endif
    ERROR1(container,CM_err_BadRefRead, CONTAINERNAME);             /* ...we had an read error    */
    return (RefReadError);                                /* ...screw it                */
  }

  ioBuffer = cmUseIOBuffer(container, RefsBufSize, (jmp_buf *)&getRefEnv); /*define bufr*/
  if (ioBuffer == NULL) {
    #if CMSHADOW_LIST
    cmDeleteRefDataShadowList(refDataValueHdr);
    #endif
    return (RefReadError);
  }

  (void)omfsTruncInt64toUInt32(refDataValueHdr->size, &xferSize);
  cmNewBufferedInputData(ioBuffer, refDataValueHdr, xferSize);

  for (x = 0; x < size; x += 8) {                         /* find the reference (key)...*/
    currKey      = (unsigned int )GET4(ioBuffer);         /* ...read next key           */
    currObjectID = (CMObjectID)GET4(ioBuffer);            /* ...its assoc. ID follows   */

    #if CMSHADOW_LIST
    r = appendShadowListEntry(container, refShadowList, currKey, currObjectID);
    if (r == NULL) {
      cmDeleteRefDataShadowList(refDataValueHdr);
      cmReleaseIOBuffer(ioBuffer);
      return (RefReadError);
    }
    #endif

    if (searchStatus == RefNotFound)                      /* ...if key not found yet... */
      if (currKey == *key) {                              /* ...keep looking for it...  */
        searchStatus = RefFound;                          /* ...we found it!            */
        omfsCvtUInt32toInt64(x, offset);                   /* ...return value data offset*/
        *objectID = currObjectID;                         /* ...and the assoc. object ID*/
        #if CMSHADOW_LIST
        *refShadowEntry = r;                              /* ...set ptr if shadowing    */
        #else
        *refShadowEntry = NULL;                           /* ...NULL if not shadowing   */
        break;                                            /* ...and we can stop reading */
        #endif
      }
  } /* for */                                             /* ...keep reading            */

  cmReleaseIOBuffer(ioBuffer);                            /* done searching and reading */

  if (searchStatus == RefNotFound) {                      /* if the key wasn't found... */
    omfsCvtUInt32toInt64(size, offset);                    /* ...ret end of list position*/
    *refShadowEntry = NULL;                               /* ...no shadow entry         */
  }

  return (searchStatus);                                  /* return search status       */
}


/*----------------------------------------------------------------*
 | CMNewReference - define a "reference" to an object for a value |
 *----------------------------------------------------------------*

 Creates a "reference" to the referencedObject and places it in theReferenceData.  The
 (input) pointer to theReferenceData is returned.  It is ASSUMED that this data will be
 written as (part of) the value data for the specified value.  The size of this data is
 determined by the size of the CMReference type, i.e., sizeof(CMReference).

 A side effect of this routine is the creation of a (private to the Container Manager)
 object (for the first reference) that records unique object/reference associations for the
 passed value.  What is returned is a "key" that is recorded as the "reference" to allow
 retrevial (e.g., using CMGetReferencedObject()) of the object (refNum) corresponding to a
 reference (key).

 Calling CMNewReference() for a already existing reference (key) simply sets
 theReferenceData and returns its pointer.

 Note, that CMNewReference() always defines theReferenceData key. However, CMSetReference()
 can be called instead to define, or redefine, an ARBITRARY key (still of type CMReference)
 to associate with an object.  See CMSetReference() for further details.
*/

CMReference CM_PTR * CM_FIXEDARGS CMNewReference(CMValue value,
                                                 CMObject referencedObject,
                                                 CMReference CM_PTR theReferenceData)
{
  ContainerPtr container;

  ExitIfBadValue(value, NULL);                          /* validate value               */
  ExitIfBadObject(referencedObject, NULL);              /* validate referencedObject    */

  container = ((TOCValueHdrPtr)value)->container->targetContainer;

  if (container != ((TOCObjectPtr)referencedObject)->container->targetContainer) {
    ERROR2(container, CM_err_CantReference, CONTAINERNAMEx(container),
                                 CONTAINERNAMEx(((TOCObjectPtr)referencedObject)->container));
    return (NULL);
  }

  /* A CMObjectID for some hardware implementations may NOT be exactly 4 bytes (but it  */
  /* better be at least 4 bytes).  A user's "reference" data is always 4 bytes because  */
  /* we define such data here as an object ID.  We must take care to convert the        */
  /* internal, i.e., hardware, representation to that of a container representation. By */
  /* share "luck" (yea right, want to buy a bridge?) we just happen to have a handler   */
  /* that does this conversion.                                                         */

  CMformatData(container, theReferenceData, 4, &((TOCObjectPtr)referencedObject)->objectID);

  /* Since we have defined the CMReference key here, we can call CMSetReference() just  */
  /* like the user to (re)define the object reference with this key.                    */

  return (CMSetReference(value, referencedObject, theReferenceData));
}


/*--------------------------------------------------------------------*
 | CMSetReference - (re)define a "reference" to an object for a value |
 *--------------------------------------------------------------------*

 This is similar to CMNewReference() except that here the caller defines the CMReference
 key to associate with an object.  The specified key must not be a nonzero value.  The
 (input) pointer to theReferenceData key is returned.

 In all cases the specified CMReference key is associated with the specified
 referencedObject.  The associations are recorded in the private recording object for the
 passed value.  New references are recorded, and previously existing references (i.e.,
 theReferenceData key matches one of the previously recorded keys) are CHANGED to assoicate
 it with the (new) referencedObject.

 The only difference between CMNewReference() and CMSetReference() is that with
 CMNewReference(), the Container Manager defines the CMReference key, while with
 CMSetReference() the caller defines the key.  The net result is the same; the keys are
 recorded in the value's recording object to define the association to the specified
 referenced object.

 Note, that multiple references to the SAME object can be recorded by passing different
 keys (theReferenceData).

 Once these associations are recorded, they may be counted, deleted, and accessed using
 CMCountReferences(), CMDeleteReference(), and CMGetNextReference() respectively.
*/

CMReference CM_PTR * CM_FIXEDARGS CMSetReference(CMValue value,
                                                 CMObject referencedObject,
                                                 CMReference CM_PTR theReferenceData)
{
  ContainerPtr          container;
  TOCObjectPtr          refDataObject, refedObject;
  TOCValueHdrPtr        theValueHdr, refDataValueHdr;
  unsigned int          key;
  CMCount				offset;
  CMObjectID            objectID;
  RefDataShadowEntryPtr refShadowEntry;
  ReferenceData         refData;
  omfInt64				refLen;

  omfsCvtUInt32toInt64(8, &refLen);
  ExitIfBadValue(value, NULL);                          /* validate value               */
  ExitIfBadObject(referencedObject, NULL);              /* validate referencedObject    */

  theValueHdr = (TOCValueHdrPtr)value;
  container   = theValueHdr->container->targetContainer;
  refedObject = (TOCObjectPtr)referencedObject;

  if (container != refedObject->container->targetContainer) {
    ERROR2(container,CM_err_CantReference, CONTAINERNAMEx(container),
                                 CONTAINERNAMEx(refedObject->container));
    return (NULL);
  }

  CMextractData(container, theReferenceData, 4, &key);  /* don't allow a key of 0       */
  if (key == 0) {
    ERROR1(container,CM_err_ZeroRefKey, CONTAINERNAME);
    return (NULL);
  }

  /* The list of references is recorded in a private object associated with the user's  */
  /* value (header).  The reference data object is "tied" to the user's value through a */
  /* pointer in the user's value header.  Of course, the first such reference must      */
  /* create the recording object and tie it to the user's value header.                 */

  /* Since an object is created, the user would see it if if s/he walked the TOC with   */
  /* CMGetNextObject().  But this is to be a private object.  So to stop the user from  */
  /* seeing it, it is unlinked from the object master chain which is used by the        */
  /* CMGetNextObject().                                                                 */

  /* Note that a value move moves a value header and hence the recording object will    */
  /* "go along for the ride".  Property and value deletes, however, must make sure to   */
  /* delete this object.                                                                */

  if (!HasRefDataObject(theValueHdr)) {                 /* if 1st ref for this value... */
    refDataObject = cmDefineObject(container,           /* ...create recording object   */
                                   container->nextUserObjectID,
                                   CM_StdObjID_ObjReferences,
                                   CM_StdObjID_ObjRefData,
                                   NULL,
                                   container->generation, 0,
                                   (ObjectObject | ProtectedObject),
                                   &refDataValueHdr);
    if (refDataObject == NULL) return (NULL);           /* exit if failure              */
    cmUnlinkObject(container->toc, refDataObject);      /* unlink from master chain     */
    refDataObject->useCount = 1;                        /* initial use of this object   */
    IncrementObjectID(container->nextUserObjectID);     /* set to use next available ID */
    refDataValueHdr->valueFlags |= ValueProtected;      /* protect from user fiddling   */
    RefDataObject(theValueHdr) = refDataObject;         /* link value to the object     */
  } else {                                              /* use existing obj if >1st time*/
    refDataValueHdr = getRecordingValueHdr(theValueHdr);/* get recording obj's value hdr*/
    if (refDataValueHdr == NULL) return (NULL);         /* something went wrong?        */
  }

  /* Search for theReferenceData key in the recording object...                         */

  switch (getReference(refDataValueHdr, theReferenceData, &offset, &key, &objectID, &refShadowEntry)) {
    case RefReadError: /* If there was some kind of read error, and the error reporter  */
    default:           /* returned, just exit...                                        */

                       return (NULL);

    case RefFound:     /* If theReferenceData key was found and the associated ID does  */
                       /* NOT need to be changed, just return the reference key. On the */
                       /* the other hand, if the key does need changing, then we FALL   */
                       /* THROUGH to the "not found" case to rerecord the list entry as */
                       /* if it's a new entry.  The offset is set to the proper place.  */

                       if (refedObject->objectID == objectID)         /* if same ID...  */
                         return ((CMReference *)theReferenceData);    /* ...return ref. */

    case RefNotFound:  /* If theReferenceData key was not found (or we must change its  */
                       /* associated ID if it was found) then we (re)write the value    */
                       /* data entry with theReferenceData key and its (new) ID. The ID */
                       /* follows the key in the data (4 bytes each).                   */

                       refDataValueHdr->valueFlags &= ~ValueProtected;/* allow write    */
                       memcpy(refData.key, theReferenceData, 4);      /* format data    */
                       CMformatData(container, refData.objectID, 4, &refedObject->objectID);
                       CMWriteValueData((CMValue)refDataValueHdr, (CMPtr)&refData, offset, 8);
                       refDataValueHdr->valueFlags |= ValueProtected; /* reprotect value*/

                       /* Create a new shadow list entry or change data in found entry. */
                       /* All this info was generated by getReference() in the switch   */
                       /* statement above.                                              */

                       #if CMSHADOW_LIST
                       if (refShadowEntry == NULL) {                  /* new entry      */
                         refShadowEntry = appendShadowListEntry(container, RefShadowList(refDataValueHdr), key, refedObject->objectID);
                         if (refShadowEntry == NULL) {
                           cmDeleteRefDataShadowList(refDataValueHdr);
                           return (NULL);
                         }
                       } else                                         /* change entry   */
                         refShadowEntry->objectID = refedObject->objectID;
                       #endif

                       return ((CMReference *)theReferenceData);      /* return ref.    */
  } /* switch */
}


/*-----------------------------------------------------------------------*
 | CMGetReferencedObject - retrieve a object (refNum) from a "reference" |
 *-----------------------------------------------------------------------*

 Converts an object "reference", created by CMNewReference() or CMSetReference(), back to
 an object refNum, from theReferenceData key is (assumed) supplied from the specified
 value.  If theReferenceData key is not found, NULL is returned.  However, it is an error
 if the key is found but the referenced object is NOT found.
*/

CMObject CM_FIXEDARGS CMGetReferencedObject(CMValue value, CMReference CM_PTR theReferenceData)
{
  ContainerPtr          container;
  TOCObjectPtr          refedObject;
  TOCValueHdrPtr        theValueHdr, refDataValueHdr;
  CMCount				offset;
  unsigned int          key = 0;
  CMObjectID            objectID;
  RefDataShadowEntryPtr refShadowEntry;
  char                  idStr[15];
  int 					idLong;

  ExitIfBadValue(value, NULL);                            /* validate value             */

  theValueHdr = (TOCValueHdrPtr)value;
  container   = theValueHdr->container->targetContainer;

  /* If there is NO recording object, there are no references.  This is treated as a    */
  /* "not found" condition.                                                             */

  refDataValueHdr = getRecordingValueHdr(theValueHdr);    /* get recording value hdr    */
  if ((refDataValueHdr == NULL) || (container->majorVersion == 1))
#if TOC1_SUPPORT
  {
    /* humor me, in 1.0d4, the object ID was stored directly */
    memcpy(&idLong, theReferenceData, sizeof(idLong));
	CMextractData(container, &idLong, sizeof(idLong), &objectID);

    refedObject = cmFindObject(container->toc, objectID);/* lookup ID*/
    if (refedObject == NULL)                             /* ...oops! */
    {
       ERROR2(container,CM_err_UndefReference, cmltostr(objectID, 1, false, idStr), CONTAINERNAME);
       return(NULL);
    }
    ++refedObject->useCount;                             /* count use*/
    return ((CMObject)refedObject);
	}
#else
   return (NULL);             /* "not found" if no object   */
#endif

  /* Search for theReferenceData key in the recording object...                         */

  switch (getReference(refDataValueHdr, theReferenceData, &offset, &key, &objectID, &refShadowEntry)) {
    case RefReadError: /* If there was some kind of read error, and the error reporter  */
                       /* returned, just exit...                                        */

                       return (NULL);

    case RefFound:     /* If theReferenceData key was found, use the associated ID to   */
                       /* find the object in the TOC. It must be found or it's an error.*/
                       /* If it is, return the object's refNum and treat it as a "use"  */
                       /* (i.e., increment the object's use count).                     */

                       refedObject = cmFindObject(container->toc, objectID);/* lookup ID*/
                       if (refedObject == NULL)                             /* ...oops! */
                         ERROR2(container,CM_err_UndefReference, cmltostr(objectID, 1, false, idStr), CONTAINERNAME);
                       ++refedObject->useCount;                             /* count use*/
                       return ((CMObject)refedObject);

    case RefNotFound:  /* If theReferenceData key was not found just exit...            */
             default:
                       return (NULL);
  } /* switch */
}


/*------------------------------------------------------------*
 | CMDeleteReference - delete an object/reference association |
 *------------------------------------------------------------*

 Deletes a  single object "reference", created by CMNewReference() or CMSetReference()
 assocated with the theReferenceData key (assumed) supplied from the specified value.

 The value's recording object's object/reference association list is searched for the
 specified theReferenceData key.  If it is found, the association is removed.  If it is
 not found this routine does nothing.  Thus it is NOT considered an error if the
 theReferenceData key is not found, or if found, that the associated object exist.

 Note, if all the references for the value's recording object are deleted, the recording
 object itself is deleted.
*/

void CM_FIXEDARGS CMDeleteReference(CMValue value, CMReference CM_PTR theReferenceData)
{
  ContainerPtr          container;
  TOCValueHdrPtr        theValueHdr, refDataValueHdr;
  unsigned int          key = 0;
  CMCount				offset;
  CMObjectID            objectID;
  RefDataShadowEntryPtr refShadowEntry;
  omfInt64				refLen, zero;

  omfsCvtUInt32toInt64(8, &refLen);
  omfsCvtUInt32toInt64(0, &zero);
  ExitIfBadValue(value, CM_NOVALUE);                      /* validate value             */

  theValueHdr = (TOCValueHdrPtr)value;
  container   = theValueHdr->container->targetContainer;

  /* If there is NO recording object, there are no references.  This is treated as a    */
  /* "not found" condition, so there's nothing to delete.                               */

  refDataValueHdr = getRecordingValueHdr(theValueHdr);    /* get recording value hdr    */
  if (refDataValueHdr == NULL) return;                    /* ...nothing to delete       */

  /* Search for theReferenceData key in the recording object...                         */

  switch (getReference(refDataValueHdr, theReferenceData, &offset, &key, &objectID, &refShadowEntry)) {
    case RefReadError: /* If there was some kind of read error, and the error reporter  */
                       /* returned, just exit...                                        */

                       return;

    case RefFound:     /* If theReferenceData key was found, use its offset to delete   */
                       /* the reference (data).  If there are no more references in the */
                       /* recording object, the object itself is deleted.               */

                       CMDeleteValueData((CMValue)refDataValueHdr, offset, refLen);

                       if (omfsInt64Equal(refDataValueHdr->size, zero)) {   /* if no  list...   */
                         RefDataObject(theValueHdr)->objectFlags &= ProtectedObject;
                         CMDeleteObject((CMObject)RefDataObject(theValueHdr)); /* delete*/
                         RefDataObject(theValueHdr) = NULL;         /* ...break link    */
                       }

                       #if CMSHADOW_LIST
                       deleteShadowListEntry(refDataValueHdr, refShadowEntry);
                       #endif

                       return;

    case RefNotFound:  /* If theReferenceData key was not found just exit...            */
             default:
                       return;
  } /* switch */
}


/*----------------------------------------------------------------------------------*
 | CMCountReferences - return number of references recorded for the specified value |
 *----------------------------------------------------------------------------------*

 Returns the total number of references recorded by CMNewReference() or CMSetReference()
 for the specified value.
*/

CMCount CM_FIXEDARGS CMCountReferences(CMValue value)
{
  ContainerPtr   container;
  TOCValueHdrPtr theValueHdr, refDataValueHdr;
  omfInt64		zero, size64;
  CM_ULONG		size;

  omfsCvtUInt32toInt64(0, &zero);
  ExitIfBadValue(value, zero);                               /* validate value             */

  theValueHdr = (TOCValueHdrPtr)value;
  container   = theValueHdr->container->targetContainer;

  /* If there is NO recording object, there are no references...                        */

  refDataValueHdr = getRecordingValueHdr(theValueHdr);    /* get recording value hdr    */
  if (refDataValueHdr == NULL) return (zero);                /* ...no references           */

  (void)omfsTruncInt64toUInt32(refDataValueHdr->size, &size);
  size = size / 8;
  omfsCvtUInt32toInt64(size, &size64);
  return (size64);                     /* 8 bytes per reference      */
}


/*------------------------------------------------------------------------*
 | CMGetNextReferenceData - get the next reference referred to by a value |
 *------------------------------------------------------------------------*

 This routine returns the next reference (key) following the currReferenceData key for the
 specified value.

 If currReferenceData is 0, then the first object reference key is returned.  If
 currReferenceData is not 0, the next reference key in sequence is returned.  In both cases
 the currReferenceData is CHANGED to the next reference and the pointer to it returned as
 the function result.  NULL is returned and currReferenceData is undefined if there are no
 more references following currReferenceData.

 Note, the object refNum corresponding to each returned reference key mey be retrieved by
 calling CMGetReferencedObject().
*/

CMReference CM_PTR * CM_FIXEDARGS CMGetNextReference(CMValue value,
                                                     CMReference CM_PTR currReferenceData)
{
  ContainerPtr   container;
  TOCValueHdrPtr theValueHdr, refDataValueHdr;
  unsigned int   currKey,size;
  omfInt64		valOffset;

  ExitIfBadValue(value, NULL);                            /* validate value             */

  theValueHdr = (TOCValueHdrPtr)value;
  container   = theValueHdr->container->targetContainer;

  /* If there is NO recording object, there are no references...                        */

  refDataValueHdr = getRecordingValueHdr(theValueHdr);    /* get recording value hdr    */
  if (refDataValueHdr == NULL) return (NULL);             /* ...no references           */

  /* If the passed key is 0, treat this as the first reference. The "next" reference    */
  /* position is maintained in the recording value's refCon (it's available -- no one   */
  /* else is using it in the recording value header).                                   */

  CMextractData(container, currReferenceData, 4,&currKey);/* convert key to internal    */

  /* From this point on we do the "get next" operations differently depending on        */
  /* whether a shadow list exists for the recording object's value data list.  It need  */
  /* not exist if getReference() was never called or we're configured not to have a     */
  /* shadow list.  If we do have the list, we search it, rather than reading the data   */
  /* explicitly which is what we have to do if the list does not exist.                 */

  #if CMSHADOW_LIST
  if (RefShadowList(refDataValueHdr) != NULL) {           /* if shadow lists exists...  */

    /* When using the shadow list, the recording value's refCon is a pointer to the     */
    /* next shadow list entry.  When the key is 0 we know to init the pointer to the    */
    /* first shadow list entry.                                                         */

    if (currKey == 0)                                     /* if first time...           */
      refDataValueHdr->valueRefCon = (CMRefCon)cmGetListHead(RefShadowList(refDataValueHdr));

    /* If there is no "next" list entry, there are no more references...                */

    if (refDataValueHdr->valueRefCon == NULL)
      return (NULL);

    /* The "next" pointer from the previous call is the "current" pointer for this call.*/
    /* Use it to convert the shadow list key of the entry pointed to to a CMReference.  */
    /* This is what we will give back to the user.                                      */

    CMformatData(container, currReferenceData, 4,
                 &((RefDataShadowEntryPtr)(refDataValueHdr->valueRefCon))->key);

    /* Update the pointer to the "next" shadow list entry for the next time around...   */

    refDataValueHdr->valueRefCon = (CMRefCon)(cmGetNextListCell(refDataValueHdr->valueRefCon));

    return ((CMReference *)currReferenceData);            /* give back ptr to reference */
  }  /* using shadow list */
  #endif

  /* If there is no shadow list, we have to do things the "hard way".  We have to       */
  /* explicitly read the recording object's value data list entries in succession. Here */
  /* the refCon is still a "pointer" to the next shadow list entry.  But unlike the     */
  /* shadow list case, the refCon takes the form of the next value data offset to be    */
  /* used to read in the next entry.  As usual, a input key of 0 signals that this is   */
  /* the first call and thus to init the pointer to offset 0.                           */

  if (currKey == 0)                                       /* if first time...           */
    refDataValueHdr->valueRefCon = (CMRefCon)0;           /* ...init value data offset  */

  /* If the current offset is at the end of the data, there are no more references...   */

  (void)omfsTruncInt64toUInt32(refDataValueHdr->size, &size);
  if ((unsigned int )refDataValueHdr->valueRefCon >= size)
    return (NULL);

  /* Read the next reference at the current offset and bump the offset for the next     */
  /* time.  Note, we only have to read the key, but we have to bump the offset by the   */
  /* entry size.  Also, since the key is already in CMReference format, we can return   */
  /* it to the caller as is.                                                            */

  omfsCvtUInt32toInt64((int )refDataValueHdr->valueRefCon, &valOffset);
  /* !!! valOffset is the offset into a list of references, so it should */
  if (CMReadValueData((CMValue)refDataValueHdr, (CMPtr)currReferenceData,
                      valOffset, sizeof(CMReference)) != sizeof(CMReference)) {
    ERROR1(container,CM_err_BadRefRead, CONTAINERNAME);
    return (NULL);
  }

  refDataValueHdr->valueRefCon = (CMRefCon)((unsigned int )refDataValueHdr->valueRefCon
                                              + sizeof(ReferenceData));

  return ((CMReference *)currReferenceData);              /* give back ptr to reference */
}

                              CM_END_CFUNCTIONS
