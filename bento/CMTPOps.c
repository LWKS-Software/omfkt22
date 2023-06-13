/*---------------------------------------------------------------------------*
 |                                                                           |
 |                        <<<      CMTPOps.c      >>>                        |
 |                                                                           |
 |              Container Manager Type and Property Operations               |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 12/15/91                                  |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1991-1992                 |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 All types and properties must be registered with CMRegisterType() and CMRegisterProperty()
 to get their associated refNums.  The refNums, in turn, are used as "type" and "property"
 parameters to other API calls.  When reading containers CMRegisterType() and
 CMRegisterProperty() are also used to get refNums for existing types and properties in
 the container.  Obviously CMRegisterType() and CMRegisterProperty() are two key routines
 and are defined in this file.

 The other routines here allow for testing refNums to see if they are types or properties
 and for accessing all the types and properties in the container.
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
#ifndef __DYNAMICVALUES__
#include "DynValus.h"
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
#ifndef __FREESPACE__
#include "FreeSpce.h"
#endif
#ifndef __UPDATING__
#include "Update.h"
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
#pragma segment CMTypePropertyOps
#endif


/*----------------------------------------------*
 | CMRegisterType - register a global type name |
 *----------------------------------------------*

 The specified type name is registered as a type in the specified container.  A refNum
 for it is returned.  If the type name already exists, the refNum for it is returned.

 Note, types may be used to create dynamic values when passed to CMNewValue().  Such types
 must have a metahandler registered for them and that metahandler must have handler
 routines for "use value", "new value", and "metadata" operation types.  See
   DynValus.c    for a full explaination of dynamic values (more than you probably ever
 want to know).
*/

CMType CM_FIXEDARGS CMRegisterType(CMContainer targetContainer, CMconst_CMGlobalName name)
{
  TOCObjectPtr  t;
  GlobalNamePtr g;
  ContainerPtr  container = (ContainerPtr)targetContainer;

  NOPifNotInitialized(NULL);                        /* NOP if not initialized!          */

  /* See if the type global name already exists...                                      */

  g = cmLookupGlobalName(container->globalNameTable, (unsigned char *)name);

  if (!SessionSuccess) {
    ERROR2(container,CM_err_GloblaNameError, name, CONTAINERNAME);
    return (NULL);
  }

  /* If the name previously existed, cmLookupGlobalName() will return a pointer to the  */
  /* global name entry.  Even deleted global names have entries!  But we can tell that  */
  /* because the value back pointer in the entry is NULLed out. If we do have a already */
  /* existing entry, just return the owning object pointer after one last validation    */
  /* that it is indeed a type object (so I'm paranoid).                                 */

  if (g && g->theValue) {                           /* previously existing name         */
    t = g->theValue->theValueHdr->theProperty->theObject;
    if ((t->objectFlags & TypeObject) == 0) {
      ERROR4(container,CM_err_MultTypeProp, "type", name, CONTAINERNAME,
             (t->objectFlags & PropertyObject) ? "property" : "object");
      t = NULL;
    } else
      ++t->useCount;                                /* bump this object's use count     */
  } else {                                          /* a new name                       */
    t = cmDefineGlobalNameObject(container, container->nextUserObjectID, CM_StdObjID_GlobalTypeName,
                                 CM_StdObjID_7BitASCII, (unsigned char *)name,
                                 container->generation, 0, TypeObject);
    if (t) {
      t->useCount = 1;                              /* initial use of this object       */
      IncrementObjectID(container->nextUserObjectID);
    }
  }

  return ((CMType)t);
}


/*-----------------------------------------------*
 | CMRegisterProperty - register a property name |
 *-----------------------------------------------*

 The specified property name is registered as a property in the specified container.  A
 refNum or it is returned.  If the property name already exists, the refNum for it is
 returned.

 Note, the implementation here is basically the same as CMRegisterType() except for the
 metaData.  Properties have none.
*/

CMProperty CM_FIXEDARGS CMRegisterProperty(CMContainer targetContainer,
                                           CMconst_CMGlobalName name)
{
  TOCObjectPtr  p;
  GlobalNamePtr g;
  ContainerPtr  container = (ContainerPtr)targetContainer;

  NOPifNotInitialized(NULL);                        /* NOP if not initialized!          */

  g = cmLookupGlobalName(container->globalNameTable, (unsigned char *)name);

  if (!SessionSuccess) {
    ERROR2(container,CM_err_GloblaNameError, name, CONTAINERNAME);
    return (NULL);
  }

  if (g && g->theValue) {
    p = g->theValue->theValueHdr->theProperty->theObject;
    if ((p->objectFlags & PropertyObject) == 0) {
      ERROR4(container,CM_err_MultTypeProp, "type", name, CONTAINERNAME,
             (p->objectFlags & TypeObject) ? "type" : "object");
      p = NULL;
    } else
      ++p->useCount;                                /* bump this object's use count     */
  } else {
    p = cmDefineGlobalNameObject(container, container->nextUserObjectID, CM_StdObjID_GlobalPropName,
                                 CM_StdObjID_7BitASCII, (unsigned char *)name,
                                 container->generation, 0, PropertyObject);
    if (p) {
      p->useCount = 1;                              /* initial use of this object       */
      IncrementObjectID(container->nextUserObjectID);
    }
  }

  return ((CMProperty)p);
}


/*-------------------------------------------*
 | CMAddBaseType - add a base type to a type |
 *-------------------------------------------*

 A base type (baseType) is added to the specified type.  For each call to CMAddBaseType()
 for the type a new base type is recorded.  They are recorded in the order of the calls.
 The total number of base types recorded for the type is retuned.   0 is returned if there
 is an error and the error reporter returns.

 It is an error to attempt to add the SAME base type more than once to the type. Base types
 may be removed from a type by using CMRemoveBaseType().

 The purpose of this routine is to define base types for a given type so that layered
 dynamic values can be created.  Base types simulate the form of type inheritance.

 See   DynValus.c    for a full description of dynamic values and how base types are used.
*/

CMCount32 CM_FIXEDARGS CMAddBaseType(CMType type, CMType baseType)
{
  ContainerPtr   container;
  TOCObjectPtr   theObject;
  TOCPropertyPtr theProperty;
  TOCValueHdrPtr theValueHdr;
  TOCValuePtr    theValue;
  TOCValueBytes  valueBytes;
  CMObjectID     baseTypeID;
  char           *typeName, *baseTypeName;
  unsigned int  aValue;

  ExitIfBadType(type, 0);
  ExitIfBadType(baseType, 0);

  container = ((TOCObjectPtr)type)->container;

  if (container->targetContainer != ((TOCObjectPtr)type)->container->targetContainer) {
    ERROR2(container,CM_err_2Containers, CONTAINERNAMEx(container),
                               CONTAINERNAMEx(((TOCObjectPtr)baseType)->container));
    return (0);
  }

  container = container->updatingContainer;

  if ((container->useFlags & kCMWriting) == 0) {      /* make sure opened for writing   */
    ERROR1(container,CM_err_WriteIllegal1, CONTAINERNAME);
    return (0);
  }

  /* Scan the "base type" property value data to see if the base type ID is already     */
  /* there.  It's an error if it's already there.                                       */

  baseTypeID  = ((TOCObjectPtr)baseType)->objectID;
  theProperty = cmGetObjectProperty((TOCObjectPtr)type, CM_StdObjID_BaseTypes);

  if (theProperty != NULL) {                          /* if base type property exists   */
    theValueHdr = (TOCValueHdrPtr)cmGetListHead(&theProperty->valueHdrList);

    if (theValueHdr != NULL) {                        /* double check for value hdr     */
      theValue  = (TOCValuePtr)cmGetListHead(&theValueHdr->valueList); /* 1st base ID   */

      while (theValue != NULL) {                      /* look at the base type values   */
        omfsTruncInt64toUInt32(theValue->value.imm.value, &aValue);
        if ((CMObjectID)aValue == baseTypeID) { /* look for ID  */
          typeName     = cmIsGlobalNameObject((TOCObjectPtr)type, CM_StdObjID_GlobalTypeName);
          baseTypeName = cmIsGlobalNameObject((TOCObjectPtr)baseType, CM_StdObjID_GlobalTypeName);
          ERROR3(container,CM_err_DupBaseType, baseTypeName, typeName, CONTAINERNAME);
          return (0);
        }

        theValue = (TOCValuePtr)cmGetNextListCell(theValue); /* look at next base type  */
      } /* while */

    } /* theValueHdr */
  } /* theProperty */

  /* We have a new base type for the type.  If this is the first base type, create the  */
  /* special "base type" property with a single immediate base type ID value. Additional*/
  /* base type ID's form a continued immediate value producing the list of base types.  */
  /* Note, this is rare instance where we allow a continued immediate value!            */

  cmSetImmValueBytes(container, &valueBytes, Value_Imm_Long, (unsigned int )baseTypeID, sizeof(CMObjectID));

  if (theProperty == NULL || theValueHdr == NULL) {   /* if 1st base type for type...   */
    theObject = cmDefineObject(container,             /* ...create base type property   */
                               ((TOCObjectPtr)type)->objectID,
                               CM_StdObjID_BaseTypes, CM_StdObjID_BaseTypeList,
                               &valueBytes, container->generation, kCMImmediate,
                               TypeObject, &theValueHdr);
    if (theObject == NULL) return (0);
    theValueHdr->useCount = 1;                        /* ...always count use of value   */
  } else {                                            /* if additional base types...    */
    theValue  = (TOCValuePtr)cmGetListTail(&theValueHdr->valueList);
    theValue->flags |= kCMContinued;                  /* ...flag current value as cont'd*/
    cmAppendValue(theValueHdr, &valueBytes, kCMImmediate); /* ...add in new base type   */
    theValueHdr->valueFlags |= ValueContinued;        /* ...create cont'd imm ID list   */
   /* !!! cmTouchBaseType(theValueHdr);                     /* touch for updating if necessary*/
  }

  return (cmCountListCells(&theValueHdr->valueList)); /* ret nbr of base types */
}


/*---------------------------------------------------*
 | CMRemoveBaseType - remove a base type from a type |
 *---------------------------------------------------*

 A base type (baseType) previously added to the specifed type by CMAddBaseType() is
 removed.  If NULL is specified as the baseType, ALL base types are removed.  The number of
 base types remaining for the type is returned.

 Note, no error is reported if the specified base type is not present or the type has no
 base types.
*/

CMCount32 CM_FIXEDARGS CMRemoveBaseType(CMType type, CMType baseType)
{
  ContainerPtr   container;
  TOCPropertyPtr theProperty;
  TOCValueHdrPtr theValueHdr;
  TOCValuePtr    theValue, thePrevValue;
  CMObjectID     baseTypeID;
  CMCount32        n;
  omfInt64		zero, objIDSize;
  unsigned int    ulongValue;

  omfsCvtUInt32toInt64(0, &zero);
  ExitIfBadType(type, 0);
  ExitIfBadType(baseType, 0);

  container = ((TOCObjectPtr)type)->container;

  if (container->targetContainer != ((TOCObjectPtr)type)->container->targetContainer) {
    ERROR2(container,CM_err_2Containers, CONTAINERNAMEx(container),
                               CONTAINERNAMEx(((TOCObjectPtr)baseType)->container));
    return (0);
  }

  container = container->updatingContainer;

  if ((container->useFlags & kCMWriting) == 0) {      /* make sure opened for writing   */
    ERROR1(container,CM_err_WriteIllegal1, CONTAINERNAME);
    return (0);
  }

  /* Scan the "base type" property value data to see if the base type ID is already     */
  /* there so that we know we can delete it.  We count the number of base types so that */
  /* we may return the new count as the function result. Note if the base type does not */
  /* exist, we return the current count and do nothing else.  The user didn't want the  */
  /* the type and it's not there, so it's not an error to not find it.  In a similar    */
  /* vain, we remove the property if the caller asked that all base types be removed.   */

  theProperty = cmGetObjectProperty((TOCObjectPtr)type, CM_StdObjID_BaseTypes);
  if (theProperty == NULL) return (0);

  theValueHdr = (TOCValueHdrPtr)cmGetListHead(&theProperty->valueHdrList);
  if (theValueHdr == NULL) return (0);

  if (baseType == NULL) {                             /* delete entire property ?       */
    CMDeleteObjectProperty((CMObject)type, container->baseTypesProperty);
    return (0);
  }

  baseTypeID = ((TOCObjectPtr)baseType)->objectID;

  theValue = (TOCValuePtr)cmGetListHead(&theValueHdr->valueList); /* 1st base ID        */

  while (theValue != NULL) {                               /* look at base type values..*/
    omfsTruncInt64toUInt32(theValue->value.imm.value, &ulongValue);
    if ((CMObjectID)ulongValue != baseTypeID) { /*if ID is not found*/
      theValue = (TOCValuePtr)cmGetNextListCell(theValue); /* ...look at next base type */
      continue;                                            /* ...keep looking           */
    }

    if (cmCountListCells(&theValueHdr->valueList) > 1) {   /* if more than 1 basetype...*/
      if (cmGetNextListCell(theValue) == NULL) {           /* make sure of seg contd bit*/
        thePrevValue = (TOCValuePtr)cmGetPrevListCell(theValue);
        if (thePrevValue != NULL)                          /* ...no longer cont'd if    */
          thePrevValue->flags &= ~kCMContinued;            /*    last sef is deleted    */
      }
    }

    cmAddToFreeList(container, theValue, zero, zero);       /* add freed space to free list   */
    cmDeleteListCell(&theValueHdr->valueList, theValue);
    CMfree(container, theValue);                                 /* poof!                          */

  	omfsCvtUInt32toInt64(sizeof(CMObjectID), &objIDSize);
    omfsSubInt64fromInt64(objIDSize, &theValueHdr->size);  /* update total size in value hdr */
    if (cmCountListCells(&theValueHdr->valueList) < 2)/* make sure of cont'd bit in hdr */
      theValueHdr->valueFlags &= ~ValueContinued;

    break;                                            /* we're done -- it's gone        */
  } /* while */

  /* If there are no base types left, remove the property.  Also touch for updating if  */
  /* needed.                                                                            */

  n = cmCountListCells(&theValueHdr->valueList);

  if (n > 0)
  {
    /* !!! cmTouchBaseType(theValueHdr);  */                   /* touch for updating if necessary*/
  }
else
    CMDeleteObjectProperty((CMObject)type, container->baseTypesProperty);

  return (n);                                         /* nbr of base types left         */
}


/*----------------------------------------*
 | CMIsType - test if an object is a type |
 *----------------------------------------*

 Returns non-zero if the object is a type and 0 otherwise.
*/

CMBoolean CM_FIXEDARGS CMIsType(CMObject theObject)
{
  ExitIfBadObject(theObject, 0);                    /* validate theObject               */

  return ((CMBoolean)((((TOCObjectPtr)theObject)->objectFlags & TypeObject) != 0));
}


/*------------------------------------------------*
 | CMIsProperty - test if an object is a property |
 *------------------------------------------------*

 Returns non-zero if the object is a property and 0 otherwise.
*/

CMBoolean CM_FIXEDARGS CMIsProperty(CMObject theObject)
{
  ExitIfBadObject(theObject, 0);                    /* validate theObject               */

  return ((CMBoolean)((((TOCObjectPtr)theObject)->objectFlags & PropertyObject) != 0));
}


/*------------------------------------------------------------------------*
 | CMGetNextType - get next type in the container by increasing object ID |
 *------------------------------------------------------------------------*

 This routine returns the refNum for the next type object in the container following
 currType.  If currType is NULL, the refNum for the first type object is returned.  If
 currType is not NULL, the type object with the next highest ID is returned.  NULL is
 returned if there are no more type objects following currType.

 currType is generally a refNum previously returned from this routine.  Successive calls
 to this routine will thus yield all the type objects in the container.
*/

CMType CM_FIXEDARGS CMGetNextType(CMContainer targetContainer, CMType currType)
{
  TOCObjectPtr theObject;
  ContainerPtr container = (ContainerPtr)targetContainer;

  NOPifNotInitialized(NULL);                        /* NOP if not initialized!          */

  if (currType == NULL)                             /* NULL ==> use head of list        */
    return ((CMType)cmGetMasterListHead(container->toc, TypeObject));

  ExitIfBadType(currType, NULL);                    /* validate currType                */

  #if 0
  if (((TOCObjectPtr)currType)->container != container ||   /* safety check container   */
      (((TOCObjectPtr)currType)->objectFlags & TypeObject) == 0) {
    ERROR3(CM_err_BadType, "CMType", "CMGetNextType", CONTAINERNAME);
    return (NULL);
  }
  #endif

  /* In our implementation the type objects are already chained in sorted order. So all */
  /* we have to do is follow the type chain.                                            */

  theObject = ((TOCObjectPtr)currType)->nextTypeProperty;
  if (theObject) ++theObject->useCount;             /* bump use count for next object   */

  return ((CMType)theObject);
}


/*--------------------------------------------------------------------------------*
 | CMGetNextProperty - get next property in the container by increasing object ID |
 *--------------------------------------------------------------------------------*

 This routine returns the refNum for the next property object in the container following
 currProperty.  If currProperty is NULL, the refNum for the first property object is
 returned.  If currProperty is not NULL, the property object with the next highest ID is
 returned.  NULL is returned if there are no more property objects following currProperty.

 currProperty is generally a refNum previously returned from this routine.  Successive
 calls to this routine will thus yield all the property objects in the container.
*/

CMProperty CM_FIXEDARGS CMGetNextProperty(CMContainer targetContainer,
                                          CMProperty currProperty)
{
  TOCObjectPtr theObject;
  ContainerPtr container = (ContainerPtr)targetContainer;

  NOPifNotInitialized(NULL);                        /* NOP if not initialized!          */

  if (currProperty == NULL)                         /* NULL ==> use head of list        */
    return ((CMProperty)cmGetMasterListHead(container->toc, PropertyObject));

  ExitIfBadProperty(currProperty, NULL);            /* validate currProperty            */

  #if 0
  if (((TOCObjectPtr)currProperty)->container != container || /* safety check container */
      (((TOCObjectPtr)currProperty)->objectFlags & PropertyObject) == 0) {
    ERROR3(CM_err_BadType, "CMProperty", "CMGetNextProperty", CONTAINERNAME);
    return (NULL);
  }
  #endif

  /* In our implementation the property objects are already chained in sorted order.    */
  /* So all we have to do is follow the property chain.                                 */

  theObject = ((TOCObjectPtr)currProperty)->nextTypeProperty;
  if (theObject) ++theObject->useCount;             /* bump use count for next object   */

  return ((CMProperty)theObject);
}

                              CM_END_CFUNCTIONS
