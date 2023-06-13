/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                     <<<        XSubValu.c         >>>                     |
 |                                                                           |
 |               Example SubValue Dynamic Value Handler Package              |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 3/10/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This file illustrates how one might implement subvalues using the "dynamic value" 
 mechanisms provided by the Container Manager.   Dynamic values are special values which
 "know" how to manipulate on their own value data.  They do this through a set of value
 operation handlers which are expected to be semantically the same as the standard API
 value operations.  This is described in more detail later.
  
 The subvalues we want to support represent values that have as their data some subset of
 other already existing base value data.  This can be illustrated with the following
 diagram:
 
                                        x
        base value data: |--------------|------------|-----------|
        subvalue data:                  |------------|
                                         <---size--->
                              offset = x
                            
 In the above diagram, the |--------| represents a data value in ascending offsets in a
 container.  The subvalue is just a subrange of (size) characters of the base value
 starting at offset x from the start of the baseValue.
 
 By definition, data (reads, writes, etc.) operations are permitted on subvalues so int 
 as they do not exceed the original definition of the subvalue.   In other words, if the
 subvalue represents size bytes at an offset withing some base value, then nothing can be
 done that will make the size grow larger (by definition).
  
 Note, subvalues of subvalues (of...) are permitted!  The origin of the second subvalue
 is with respect to the (sub)value on which it is based.  
 
 There is one restriction on this subvalue implementation. If the base value of a subvalue
 is MOVED, the results will be unpredictible and may (or may not) result in an error
 report.  It won't blow.  But the implementation of subvalues is done in terms of the base
 value's object ID, property ID, and type ID.  This is data actually written to the
 container for the subvalue.  If the base moves, this data is NOT updated.  A future 
 reference will thus get whatever represents that object ID, property ID, and type ID. It
 may or may not exist.  If it doesn't exist, an error report will be produced.  But, it
 won't be the proper base if the base is moved and there is something else in the original
 base position.

 Basically, dynamic values are similar to C++ objects where the handlers are the object's
 methods.  Dynamic values are a generalized mechanism that provides for type inheritance.
 They are similar to C++ objects where the handlers are the object's methods and an
 object's type represents its class.  They are dynamic in the sense that they only exist
 from creation (discussed below) and last until until they are released (CMReleaseValue()).

 The subvalue example we will illustrate here illustrates the type inheritance where the
 type of the subvalue (which describes what we're kind of subvalue this is) has as its
 base type, a "subvalue" type.  The subvalue type will be used to create the dynamic value
 whose value operation handlers will effect the subvalue results.
 
 Follow the code in this file in the order presented.  The comments document the steps
 required to support subvalues (and more generally types for dynamic values).  The
 newSubValue() routine defines the interface that can be used to create a subvalue.  It
 would be called just like any other Container Manager API routine.
*/


/*------------------------------------------------------------------------------------*
 | DOS (80x86) USERS -- USE THE "LARGE" MEMORY MODEL WHEN COMPILED FOR 80X86 MACHINES |
 *------------------------------------------------------------------------------------*

 The Container Manager is built with this same option and assumes all handler and 
 metahandler interfaces are similarly built and can be accessed with "far" pointers.
*/


/*#include <types.h>*/
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifndef __CM_API__
#include "CMAPI.h" 
#endif
#ifndef __XSUBVALUEHANDLERS__
#include "XSubValu.h"               
#endif

                                  CM_CFUNCTIONS
                                  
/* See comments in      cmapienv.h      about CM_CFUNCTIONS/CM_END_CFUNCTIONS (used     */
/* only when compiling under C++).                                                      */


/* In order to make this code read from the top down, it is necessary in a few instances*/
/* to forward reference some metahandler functions.  Due to some "complaints" from      */
/* certain C compilers (who shall remain nameless), the forward declarations are placed */
/* here.  Basically, ignore this stuff.                                                 */

static CMHandlerAddr subValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType);


/* GENERAL NOTE...

   In order to make the code as general as possible, the session handlers provided by the
   API are used.  This includes memory allocation, deallocation, error reporter, and
   access to the container identification.  These interfaces allow us to use the same 
   memory and error handlers that the Container Manager uses as provided to
   CMStartSession() through the methandler passed there.  The container identification
   interface allows us to add that information to the error reports.  The wording of these
   messages follows the "flavor" of those done by the Container Manager itself.
*/


/*----------------------------------------------------------------------------*
 | newSubValue - create refNum for a value that's a subvalue of another value |
 *----------------------------------------------------------------------------*

 This is a special case of CMNewValue() in that a new value entry of the specified type is
 created for the specified object with the specified property.  A refNum for this value is
 returned.  However, unlike CMNewValue(), the refNum is associated with a "subvalue" of
 some OTHER already existing value data (size characters starting at the offset). Whenever
 the returned refNum is used for a value operation, that operation will take place on the
 subvalue of the base value.
 
 Basically all we do here is make sure the proper types are registered and based prior
 to doing a "normal" CMNewValue().  But it isolates the type registration to this one 
 place so we can illustrate how to create the desired dynamic value.
 
 The types are the key to generating dynamic values.  A dynamic value results when a value
 is created by to CMNewValue() or attempted to be used by CMUseValue(), and the following
 two conditions occur:
 
  1. The type or any of its base types (if any, created by CMAddBaseType()), are for
     global names that have associated metahandlers registered by CMRegisterType().
     
  2. The metahandlers support the operation type to return a "use value" handler, and
     in addition for CMNewValue(), "metadata" and "new value" handlers.

 "Metadata" handlers are used to define metadata that is a format description that directs
 CMNewValue() on how to interpret it's "..." parameters to produce a data packet of those
 parameters.  The data packet is sent to the "new value" handler.  In this routine we will
 do a CMNewValue() to create the dynamic value with the newSubValue() baseValue, offset,
 and size passed as the "..." parameters.
 
 "New value" handlers are used to define initialization data for the "use value" handlers
 based on the data packets.  The "use value" handlers are called to set up and return
 refCons.  Another metahandler address is also returned from the "use value" handler.  This
 is used to get the address of the value operation handlers corresponding to the standard
 API value routines.

 The goal is to create a dynamic value to return, which when used will produce the desired
 subvalue effect. Thus we need two types:
 
  1. A type that describes the meaning of this subvalue.  That is the type passed to
     this routine.
      
  2. A base type that represents a subvalue in general.  The user's type is given the
     subvalue type as its base type. 
     
 It is that general subvalue base type that most of this file supports. Namely the various
 metahandlers and handlers for subvalues. Indeed, the only thing we do here is set up the
 types, register their methandlers, and do the CMNewValue().  
*/

CMValue CM_FIXEDARGS newSubValue(CMObject object, CMProperty property, CMType type,
                                 CMValue baseValue, CMCount offset, CMSize size)
{
  CMContainer container;
  CMSession   sessionData;
  CMType      subValueType;
  CMValue     dynamicValue;

  /* Get the container refNum and session data pointer. They are needed for registering */
  /* metahandlers and types and for reporting errors.  Note, if the container is passed */
  /* as NULL, the session pointer will end up being NULL. In that case, we can't report */
  /* the error and just return NULL.                                                    */
  
  container   = CMGetObjectContainer(object);
  sessionData = CMGetSession(container);
  
  if (sessionData == NULL)              /* if no session...                             */
    return (NULL);                      /* ...there is nothing much else we can do here */
  
  /* Do a safety check on the passed refNums...                                         */
    
  if (object == NULL || property == NULL || type == NULL || baseValue == NULL) {
    CMError(sessionData, "Null object, property, type, or base value refNum passed to newSubValue()");
    return (NULL);
  }
  
  /* If you wanted to really be paranoid, you could check that the property, type, and  */
  /* baseValue all belonged to the same container.  But lets be "trusting" for now!     */
  
  /* Register the subvalue type (or reregister it just to make sure its registered) and */
  /* then define the metahandler for that type which will return the "metadata", "new   */
  /* value", and "use value" handlers (described later in this file). Note, the subvalue*/
  /* global name, as well as the metahandler are published in the subvalue header file  */
  /* to allow other combinations of base types that might want subvalues as one of their*/
  /* inherited types.  Further, to use the value we're creating in the future sessions, */
  /* the caller simply does a CMUseValue().   But that requires the type and its        */
  /* metahandler be registered just as we do here.  As a convenience we provide a macro */
  /* to do this in the header file.  For clearity, we do it explicitly here.            */
  
  subValueType = CMRegisterType(container, SubValueGlobalName);
  if (subValueType == NULL) return (NULL);
  CMSetMetaHandler(sessionData, SubValueGlobalName, subValueDynamicValueMetahandler);
  
  /* The subvalue type will be defined as a base type for the caller's type.  The       */
  /* caller's type is already defined.  It is assumed to be a type that describes what  */
  /* kind of subvalue is being used.  The type heirarchy we want looks something like   */
  /* this:                                                                              */
  
  /*                               Subvalue Type (bottom type)                          */
  /*                                          *                                         */
  /*                                          *                                         */
  /*                                          *                                         */
  /*                                Caller's Type (top type)                            */
                                     
  /* In other words, we want the subvalue type as a base type of the description type.  */
  /* This is easily accomplished as follows:                                            */

  CMAddBaseType(type, subValueType);
  
  /* Base types are essentially values for a special "base type" property added to the  */
  /* type object. Above we're defining subValueType as a single base type for the type. */
  
  /* That's about all there is to it!  Now all that's left is to do the CMNewValue()    */
  /* to create the dynamic value.                                                       */
  
  dynamicValue = CMNewValue(object, property, type, baseValue, offset, size);
  
  /* The dynamicValue is now created!  We can return it to the caller.  Note that the   */
  /* baseValue, offset, and size are passed to CMNewValue().  It has no idea what these */
  /* are!   But it does know how to deal with them.  Specifically, it passes them to    */
  /* the "metadata" and "new value" handlers during the creation of the dynamic value.  */
  /* CMNewValue() knows there are three additional parameters because of the metadata   */
  /* we return from the "metadata" handler.  You can see it works something like        */
  /* "printf()", although across different calls.  The metadata tells CMNewValue() to   */
  /* consume the additional parameters just like the "printf()" format string tells     */
  /* "printf()".                                                                        */
  
  /* Getting back to the dynamic value -- to reiterate, the rules for dynamic value     */
  /* creation are that when a CMNewValue() or CMUseValue() are done, if the passed type,*/
  /* or any of its base types have (through associated metahandlers) a "use value"      */
  /* handler, and additionally for CMNewValue(), a "metadata" and "new value" handler,  */
  /* a dynamic value is created.                                                        */
  
  return (dynamicValue);  
}

/* That's all there is to it! From this point on we turn our attention to the definition*/
/* of the metahandlers and handlers.                                                    */


/*======================================================================================*/
/*======================================================================================*/


/*-----------------------------------------------------------------------------------*
 | subValueDynamicValueMetahandler - matahandler for subvalue dynamic value creation |
 *-----------------------------------------------------------------------------------*
 
 The first methandler to be defined is the metahandler called from CMNewValue() or
 CMUseValue() for dynamic value creation.  It does this to see if a type, which had this
 metahandler registered for the type global name, has a "use value" handler, and
 additionally for CMNewValue(), "metadata" and "new value" handlers defined.  That is the
 criteria on whether a dynamic value is to be created.
 
 The "metadata", "new value", and "use value" handlers have the following prototypes:   */
 
 static CMMetaData metaData_Handler(CMType type);
 static CMBoolean newValue_Handler(CMValue dynamicBaseValue, CMType type,
                                   CMDataPacket dataPacket);
 static CMBoolean useValue_Handler(CMValue dynamicBaseValue, CMType type,
                                   CMMetaHandler *metahandler, CMRefCon *refCon);
                                                                                        /*
 The parameters are defined later in this file in the documentation for these routines.
*/

CMHandlerAddr CM_FIXEDARGS subValueDynamicValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType)
{
  CMType ignored = targetType;
  
  if (strcmp((char *)operationType, CMDefineMetaDataOpType)==0) /* "metadata"           */
    return ((CMHandlerAddr)metaData_Handler);
  else if (strcmp((char *)operationType, CMNewValueOpType) == 0)/* "new value"          */
    return ((CMHandlerAddr)newValue_Handler);
  else if (strcmp((char *)operationType, CMUseValueOpType) == 0)/* "use value"          */
    return ((CMHandlerAddr)useValue_Handler);
  else                                                          /* what's the question? */
    return (NULL);
}


/*--------------------------------------------------------------*
 | metaData_Handler - return the matadata for the subvalue type |
 *--------------------------------------------------------------*
 
 The "metadata" handler is called from CMNewValue() so that the proper number of its "..."
 parameters can be placed into a data packet to be passed to the "new value" handler. This
 handler takes the type for which we ant to return the corresponding metadata.
 
 In the case of subvalues, we want to pass the newSubValue() parameters that define which
 value we're taking the subvalue of and its limits, i.e., the baseValue, offset, and size.
 The metadata "%v %ld %ld" will serve the purpose.  It specifies a value refNum and two 
 longs.  
 
 Note, we can define any format we thing is appropriate for the "new value" handler so 
 int  as it uses CMScanDataPacket() to extract the parameters and uses this handler itself
 to get the metadata format string.  That's what we will do when we get to the "new value"
 handler.
*/
  
static CMMetaData metaData_Handler(CMType type)
{
  CMType unused = type;
  
  return ((CMMetaData)"%v %ld %ld");          /* for baseValue, offset, and size        */
}


/*----------------------------------------------------------------------*
 | newValue_Handler - create permanent data for the "use value" handler |
 *----------------------------------------------------------------------*
 
 The "new value" handler is called from CMNewValue() for dynamic value creation just prior
 to calling the "use value" handler.  It is not used by CMUseValue().

 "New value" handlers are used to define initialization data for the "use value" handlers.
 It is assumed that such data will be some function of the original parameters passed to
 CMNewValue().  These are packed into the dataPacket according to the metadata defined for
 the type.  If there are multiple base types, each type's metadata directs CMNewValue() on
 how many of its "..." parameters to consume in build a data packet.
 
 Base types are processed in a depth-first manner, so CMNewValue() "..." parameters are
 consumed left-to-right, for the deepest base type to the highest.  For example, given the
 following type inheritance heirarchy (read these as T1 inherits from T2 and T3, and so
 on):
 
                                      T4      T5
                                        *    *
                                         *  *
                                  T2      T3
                                    *    *
                                     *  *
                                      T1
              
 The depth-first search, starting at T1, yields the sequence: T2 T4 T5 T3 T1.  Then this
 is the order the CMNewValue() "..." parameters must be in.  It is also the order the
 dynamic value layers are generated.  T1 is the top layer, and T2 the bottom.  T1 would be
 the dynamic value returned from CMNewValue().
 
 In this example we only have a single base type, the subvalue type, which has metadata
 defined to pass the subvalue's base (i.e., the value we're taking the subvalue of, not
 the dynamicBaseValue of the dynamic value -- sorry about the terminology here -- too
 many bases).  Also the offset and size were packed in.
 
 From this data we want to write permanent information so that the "use value" handler,
 which is always called for CMNewValue() and CMUseValue(), can create an appropriate refCon
 to pass among the value operation handlers.  The data layout chosen is as follows:     */
 
 struct SubValueData {              /* Subvalue data layout:                            */
   CMReference baseObjectRef;       /*    base value's object ref.  [these 3 values are]*/
   CMReference basePropertyRef;     /*    base value's property ref.[used to access the]*/ 
   CMReference baseTypeRef;         /*    base value's type ref.    [base value.       ]*/
   CMCount     offset;              /*    the subvalue's starting offset in the base    */
   CMSize      size;                /*    the size of the subvalue                      */
 };
 typedef struct SubValueData SubValueData;
 
 #define SubValueDataSize (sizeof(SubValueData))                          
                                                                                         /*
 Note, that the base value (remember the one we're taking the subvalue of) is defined in 
 terms of its object references.  The reason for this is that this data, like any other
 value data, is written to the container and remains after the container is closed.  Now,
 if the container is opened for reading, if a CMUseValue() is done on the value, the
 "use value" handler will read the data just as in the CMNewValue() case.  This means
 subvalues may be permanently used in the container.  Remember that newSubValue() is done
 only to initially create the subvalue.  Future uses don't call it.  So nothing should be
 written that can't be used by only calling "use value" handler without its "new value"
 handler.
 
 There is one problem with this scheme.  Since the data is in terms of object references,
 we could end up referencing the wrong stuff if the referenced value is moved or deleted.
 This is a restriction on subvalues which we cannot get around!  Sorry!  All we can do is
 mention this in the documentation.
 
 On final point, if any errors are reported, we abort the execution of the program in this 
 example.  As documented, error reporters are not supposed to return.  But in case they
 do, the Container Manager wants to know about it!  The CMBoolean function result is used
 for that purpose.  Nonzero should be returned for success, and if this should happen to
 return for a failure, 0 should be returned as the function result.
*/

static CMBoolean newValue_Handler(CMValue dynamicBaseValue, CMType type, CMDataPacket dataPacket)
{
  CMContainer  container   = CMGetObjectContainer(type);
  CMSession    sessionData = CMGetSession(container);
  CMObject     baseObject;
  CMProperty   baseProperty;
  CMType       baseType;
  CMValue      baseValue;
  CMCount      offset, i;
  CMSize       size;
  SubValueData subValueData;
  
  /* Scan the dataPacket to put its data back into variables.  The API provides         */
  /* CMScanDataPacket() to do that.  We reverse what CMNewValue() did by using our own  */
  /* "metadata" handler to get the metadata to direct the extraction of dataPacket      */
  /* fields back to variables (sort of like a "scanf()").                               */            
  
  i = CMScanDataPacket(type, metaData_Handler(type), dataPacket, &baseValue, &offset, &size);
  
  if (i != 3) {
    CMError(sessionData, "Unable to get all of CMNewValue()'s \"...\" parameters in container \"^0\"", CMReturnContainerName(container));
    return (0);
  }
  
  /* Fill in and write the data buffer to define subvalue info that the "use value"     */
  /* handler will need to build its refCon.  The info is as described above.  Note, we  */
  /* get the value info using CMGetValueInfo() on the baseValue.  CMGetValueInfo()      */
  /* always returns the info on "real" values.  It is possible we could be taking the   */
  /* subvalue of a subvalue! In that case baseValue will itself be a dynamic value. Its */
  /* handler for CMGetValueInfo() will return the info for its "real" value (which you  */
  /* can see in this file later).  As you will see later, when the "use value" handler  */
  /* reads this stuff it will do a CMUseValue() on it. The effect of that is to recreate*/
  /* a dynamic value for the original dynamic value we're taking the subvalue of.  This */
  /* is consistent with taking the subvalues of normal values.  But what is happening   */
  /* for this is two sets of handlers (obviously only one set of code) are operating    */
  /* independently.                                                                     */
  
  CMGetValueInfo(baseValue, NULL, &baseObject, &baseProperty, &baseType, NULL);
  
  /* Create the object reference data...                                                */
    
  subValueData.offset = offset;             /* remember the subvalue location           */
  subValueData.size   = size;

  CMNewReference(dynamicBaseValue, baseObject,   subValueData.baseObjectRef);
  CMNewReference(dynamicBaseValue, baseProperty, subValueData.basePropertyRef);
  CMNewReference(dynamicBaseValue, baseType,     subValueData.baseTypeRef);
  
  CMWriteValueData(dynamicBaseValue, (CMPtr)&subValueData, 0, SubValueDataSize);
  
  return (1);                               /* that's all there is to do here!          */
}


/*----------------------------------------------------------------------------------*
 | useValue_Handler - create handler refCon and define value operations metahandler |
 *----------------------------------------------------------------------------------*
 
 The "use value" handler is called for both CMNewValue() and CMUseValue().  For
 CMNewValue(), it is called after the "new value" handler.  Either way, the "use value"
 handler is expected to build a refCon to pass among its value operation handlers, and
 to return a pointer to the metahandler that will yield the addresses of those handlers.
 
 Generally, the refCon is built from the data written to a dynamic values base value by
 the "new value" handler.  That data is permanent, and thus a CMUseValue() on the value
 of the dynamic value type will cause the creation of the dynamic value even when the
 "new value" handler is not called (as is the case for CMUseValue()).

 As with the "new value" handler, the type is passed as a convenience.  It may or may not
 be needed.  For the "new value" handler it is passed to CMScanDataPacket() to convert
 the data packet back into variables.  Here, it may not be needed.  It is possible for a
 type object to contain OTHER data for other properties.  Types, after all, are ordinary
 objects.  There is nothing prohibiting the creation of additional properties and their
 values.  This fact could be used to pass additional (static?) information to the "new
 value" or "use value" handlers which would read the data values.

 In this example, the "new value" handler data written to the base value is as defined by
 the SubValueData struct defined in the "new value" handler.  It contains the subvalue
 offset and size, and the access info (i.e, object ID property ID, and type ID) to the
 subvalue's base value (i.e., the value on which we want to subvalue -- you have to keep
 "base values" straingt here).
 
 The refCon we build from this data has the following layout:                           */
 
 struct SubValueRefCon {                    /* Subvalue handler's refCon:               */
   CMSession sessionData;                   /*    ptr to the current container session  */
   CMValue   baseValue;                     /*    refNum of the base value              */
   CMCount   offset;                        /*    subvalue's starting offset            */
   CMSize    size;                          /*    subvalue's size                       */
 };
 typedef struct SubValueRefCon SubValueRefCon, *SubValueRefConPtr;                      /*

 The current session data pointer, which we need to allocate the refCon is kept just as a
 convenience in case we need it again.  At the very least it will be needed to free the
 refCon.

 The refCon is accessable to all the handler routines via a CMGetValueRefCon() call.
 
 Note, as with the "new value" handler, if any errors are reported, we abort the execution
 of the program in this example. As documented, error reporters are not supposed to return.
 But in case they do, the Container Manager wants to know about it!  The CMBoolean function
 result is used for that purpose.  Nonzero should be returned for success, and if this
 should happen to return for a failure, 0 should be returned as the function result.
*/

 static CMBoolean useValue_Handler(CMValue dynamicBaseValue, CMType type,
                                   CMMetaHandler *metahandler, CMRefCon *refCon)
{
  CMContainer       container   = CMGetObjectContainer(type);
  CMSession         sessionData = CMGetSession(container);
  CMObject          baseObject;
  CMProperty        baseProperty;
  CMType            baseType;
  CMValue           baseValue;
  SubValueRefConPtr myRefCon;
  SubValueData      subValueData;
  
  /* Do a normal read of the dynamicBaseValue's data to get the access info that was    */
  /* created by the "new value" handler.  Remember, if the value we're taking the       */
  /* subvalue of was moved after this data was written, then we could be looking at the */
  /* wrong stuff!  We can't enforce this.  It is just a documented restriction we place */
  /* on subvalues (i.e., don't move their bases).                                       */
  
  if (CMReadValueData(dynamicBaseValue, (CMPtr)&subValueData, 0, SubValueDataSize) != SubValueDataSize) {
    CMError(sessionData, "Incorrect byte length read while reading subvalue data in container \"^0\"", CMReturnContainerName(container));
    return (0);
  }
  
  /* Get the refNum to the subvalue's base value (the one we're taking the subvalue of).*/
  /* The subvalue data contains the base value's object and property ID's and its value */
  /* index.  The ID's are used to get the object descriptor refNums for the base value  */
  /* object and property.  The refNums allow us to use CMUseValue() to get the refNum   */
  /* for the value which we want to stick into the refCon we will allocate shortly. This*/
  /* refNum, by the way, thus gets us to the same base  value we originally passed to   */
  /* newSubValue() (assuming it wasn't moved).                                          */
  
  baseObject = CMGetReferencedObject(dynamicBaseValue, subValueData.baseObjectRef);
  if (baseObject == NULL) return (0);
  baseProperty = CMGetReferencedObject(dynamicBaseValue, subValueData.basePropertyRef);
  if (baseProperty == NULL) return (0);
  baseType = CMGetReferencedObject(dynamicBaseValue, subValueData.baseTypeRef);
  if (baseType == NULL) return (0);
  
  baseValue = CMUseValue(baseObject, baseProperty, baseType);
  if (baseValue == NULL) return (0);
  
  /* Allocate the refCon that we will pass among the handlers.  Since we are doing a    */
  /* dynamic allocation here we will use the "malloc" handler defined for the container.*/
  
  myRefCon = (SubValueRefConPtr)CMMalloc(sizeof(SubValueRefCon), sessionData);
  if (myRefCon == NULL) {
    CMError(sessionData, "Cannot allocate space for subvalue handler refCon in container \"^0\"", CMReturnContainerName(container));
    return (0);
  }

  myRefCon->sessionData = sessionData;                /* save the current session ptr   */
  
  /* Fill in the refCon with all the info we collected...                               */
  
  myRefCon->baseValue = baseValue;
  myRefCon->offset    = subValueData.offset;
  myRefCon->size      = subValueData.size;

  /* All that's left to do here is to return the refCon and the pointer to the value    */
  /* operations metahandler to the Container Manager who called us...                   */
  
  *metahandler = (CMMetaHandler)subValueMetahandler;  /* return metahandler             */
  *refCon      = (CMRefCon)myRefCon;                  /* ...and refCon                  */
    
  return (1);                                         /* we're now ready to go          */
}


/*---------------------------------------------------------------------------*
 | subValueMetahandler - metahandler for a dynamic value operations handlers |
 *---------------------------------------------------------------------------*
 
 This is the metahandler that the "use value" handler returns to the Container Manager.
 It defines the addresses for all the value operations handlers supported for subvalues.
 
 The dynamic value operations handlers have the following prototypes:                   */
 
 static CMSize getValueSize_Handler(CMValue value);
 static CMSize readValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize maxSize);
 static void writeValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize size);
 static void insertValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize size);
 static void deleteValueData_Handler(CMValue value, CMCount offset, CMSize size);
 static void getValueInfo_Handler(CMValue value, CMContainer *container, CMObject *object,
                                  CMProperty *property, CMType *type,
                                  CMGeneration *generation);
 static void setValueType_Handler(CMValue value, CMType type);
 static void setValueGeneration_Handler(CMValue value, CMGeneration generation);
 static void releaseValue_Handler(CMValue value);
                                                                                        /*
 Note that these handlers have exactly the same calling conventions as the standard API
 routines.  These handlers must also have the same semantics.  What happens is that every
 time a dynamic value is passed to the Container Manager, it is validated and then checked
 to see if it is indeed a dynamic value.  If it is, the corresponding handler is called to
 do the work.
 
 The actual handler called for the dynamic value may or may NOT be associated with the
 type associated with the dynamic value.  Just like C++, dynamic values may be "subclassed"
 via their (base) types.  If a handler for a particular operation is not defined for a
 value, its "base value" is used to get the "inherited" handler.  This continues up a
 dynamic value's chain of base values, up to the original "real" value that spawned the
 base values from the CMNewValue() or CMUseValue().
 
 In this example we had two types, the subvalue base type and its base "real" value type,
 i.e, the one passed to newSubValue().  The dynamic value returned from there was for
 the subvalue.  So if we don't supply a value handler for a particular operation, the
 operation will end up be done to the base "real" value.
 
 Taking advantage of this means you don't have to supply a handler unless it is absolutly
 necessary.  This minimizes the coding, not to mention code size!  We actually do take 
 advantage of the method ('er, excuse me, value operation) inheritance here.  The subvalue
 operations for the "getValueInfo", "setValueType", and "setValueGeneration" are not
 defined (i.e., we return NULL for their address) as handlers.
 
 For "getValueInfo", "setValueType", and "setValueGeneration" it's obvious you want to use\
 the base "real" value.  Note however, if the type is changed, you are "pulling" the rug
 out" from under the dynamic value creation.  Once released, the value will not have a
 subvalue base type, unless, of course, the new type had one.
 
 Note that we actually declare all the value operations handlers as "static" since they
 only need to be visible in this file.  The Container Manager gets at them through the
 addresses we return from here.
 
 The handlers do their work by calls back to the Container Manager.  But the values passed
 back cannot be the dynamic value passed in.  This could lead to a recursive and endless
 loop.  This IS detected by the Container Manager as an error condition.
 
 In our subvalue case, the handlers will use the refCon to do their respective operations
 on the base value (the one we're taking the subvalue of).  Remember the refNum to the
 base data value was placed in the refCon by the "use value" handler.

 Since there is nothing preventing a user from screwing around with the base value (the
 value we're taking the subvalue of) between subvalue references, the handlers will all
 have to check to see if the base value's data is valid for the offset and size of the
 subvalue.  This is done in a manner consistent with the handler operation involved.
 However, other validations on the value are NOT necessary.  
 
 By the time a dynamic value operation handler has been called, all validation checks have
 been done on the dynamic value by the Container Manager.  So they need not be done every
 time here.  Validation checks include such things as the Container Manager being
 initialized, non-NULL value pointers, etc.  In short, all the validations the value
 routines do for normal values before discovering it should call a dynamic value handler.
*/

static CMHandlerAddr subValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType)
{
  static char *operationTypes[] = {CMGetValueSizeOpType,    /* 0 */ /* Operation Types  */
                                   CMReadValueDataOpType,   /* 1 */
                                   CMWriteValueDataOpType,  /* 2 */
                                   CMInsertValueDataOpType, /* 3 */
                                   CMDeleteValueDataOpType, /* 4 */
                                   CMGetValueInfoOpType,    /* 5 */
                                   CMSetValueTypeOpType,    /* 6 */
                                   CMSetValueGenOpType,     /* 7 */
                                   CMReleaseValueOpType,    /* 8 */
                                   NULL};
  char   **t;
  CMType ignored = targetType;
  
  /* Look up the operation type in the operationTypes table above...                    */
  
  t = operationTypes - 1;
  while (*++t) if (strcmp((char *)operationType, *t) == 0) break;

  /* Now that we got it (hopefully), return the appropriate routine address...          */
  
  switch (t - operationTypes) {
    case  0:  return ((CMHandlerAddr)getValueSize_Handler);   /* CMGetValueSizeOpType   */
    case  1:  return ((CMHandlerAddr)readValueData_Handler);  /* CMReadValueDataOpType  */
    case  2:  return ((CMHandlerAddr)writeValueData_Handler); /* CMWriteValueDataOpType */
    case  3:  return ((CMHandlerAddr)insertValueData_Handler);/* CMInsertValueDataOpType*/
    case  4:  return ((CMHandlerAddr)deleteValueData_Handler);/* CMDeleteValueDataOpType*/
    case  5:  return (NULL);/* use inherited handler or API *//* CMGetValueInfoOpType   */
    case  6:  return (NULL);/* use inherited handler or API *//* CMSetValueTypeOpType   */
    case  7:  return (NULL);/* use inherited handler or API *//* CMSetValueGenOpType    */
    case  8:  return ((CMHandlerAddr)releaseValue_Handler);   /* CMReleaseValueOpType   */
    
    default:  return (NULL);
  }
}


/*======================================================================================*/
/*======================================================================================*/

/* What follows now are the individual dynamic value operations handlers whose addresses*/
/* we returned from the above metahandler.  As stated above, these routines MUST have   */
/* the same semantics (within reason) as the corresponding API routine that calls them. */
/* If a routine doesn't make sense for the dynamic value it should report an error.     */

/* All validation has already been done by the time a handler routine is called. Except */
/* for validation, the API routine that calls the handler does NOTHING else.  Note,     */
/* however, that since there is a one-to-one mapping between the handlers and their     */
/* corresponding API routines, many handlers end up doing a call back to the Container  */
/* Manager to do their tasks passing the parameters unaltered (e.g., buffer pointers,   */
/* sizes, etc.).  The Container Manager always trys to validate its parameters as best  */
/* it can.  For example, a NULL buffer pointer is processed appropriately.  What all    */
/* this means is that the handlers do not need to do these valiations either on the     */
/* parameters.  It can be left to the Container Manager if the parameters are passed in */
/* a call back.                                                                         */

/* What each of these handlers must do first, usually, is get the refCon we built in    */
/* the "use value" handler.  The refCon contains the refNum to the base data value (the */
/* one we're taking the subvalue of) to be operated upon by the handlers. The subvalues */
/* definition of offset and size with respect to the base value are there too.          */

/* Remember that there is nothing prohibiting modifications on the base between subvalue*/
/* operations.  Thus the size of the base value's data could be changing. This has to be*/
/* appropriately taken into account in the handlers.                                    */


/*---------------------------------------------------------------*
 | getValueSize_Handler - CMGetValueSize() dynamic value handler |
 *---------------------------------------------------------------*
 
 The returned size can be less than the original size passed to newSubValue() if the
 base value has been edited smaller.  In the limit, if enough data has beend deleted off 
 the end of the base value, then 0 can be returned.
*/

static CMSize getValueSize_Handler(CMValue value)
{
  SubValueRefConPtr refCon = (SubValueRefConPtr)CMGetValueRefCon(value);
  CMValue           baseValue = refCon->baseValue;
  CMSize            actualSubValueSize, baseValueSize;
  
  baseValueSize = CMGetValueSize(baseValue);/* current base value size*/
  
  /* If the starting offset of the subvalue is beyond the end of the base value return  */
  /* 0.  Otherwise return from the offset to the end of the base value size or the      */
  /* original subvalue size, whiever is smaller.                                        */
  
  if (baseValueSize <= (CMSize)refCon->offset)
    actualSubValueSize = 0;                                   /* base value too small   */
  else {                                                      
    actualSubValueSize = baseValueSize - (CMSize)refCon->offset;
    if (actualSubValueSize > refCon->size)                    /* use smaller of the two */
      actualSubValueSize = refCon->size;
  }
  
  return (actualSubValueSize);
}


/*-----------------------------------------------------------------*
 | readValueData_Handler - CMReadValueData() dynamic value handler |
 *-----------------------------------------------------------------*

 The data, starting at the offset, for the subvalue is read into the buffer.  The size of
 the data read is returned.  Up to maxSize characters will be read (can be 0).
 
 The data is read starting at the offset, up to the end of the data, or maxSize characters,
 whichever comes first.  Offsets are relative to 0, where 0 is the first byte of the
 SUBVALUE.  If the starting offset is greater than or equal to the current subvalue size,
 no data is read and 0 returned.
*/

static CMSize readValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize maxSize)
{
  SubValueRefConPtr refCon = (SubValueRefConPtr)CMGetValueRefCon(value);
  CMSize            actualSubValueSize = getValueSize_Handler(value);
  
  /* The offset is with respect to the subvalue.  If it is beyond the end of the        */
  /* subvalue we don't read anything.                                                   */
  
  if (offset >= actualSubValueSize) return (0);
  
  /* Read the data representing the subvalue from its base value.  Up to maxSize or     */
  /* actualSubValueSize - offset bytes are read, whichever is smaller. The actual amount*/
  /* read is returned.  Note, that the offset passed is with respect to the subvalue.   */
  /* We therefore must relocate this value into the offset of the base value.           */
  
  return (CMReadValueData(refCon->baseValue, buffer, refCon->offset + offset,
                          (maxSize <= actualSubValueSize - offset) ? maxSize 
                                                                   : (actualSubValueSize - offset)));
}


/*-------------------------------------------------------------------*
 | writeValueData_Handler - CMWriteValueData() dynamic value handler |
 *-------------------------------------------------------------------*
 
 The buffer is written to the container and defined as the data for the subvalue. Note,
 this OVERWRITES data in the base value.  This is equivelent to doing a CMWriteValueData()
 directly to the base value data with the offset appropriately relocated.  The offset
 here is with respect to the subvalue.
 
 If the subvalue's original size is defined as a value large enough so that if size bytes
 are written at the offset, the base value data will be EXTENDED.  It is an error to
 attempt to write more than the original size definition of the subvalue.  It is also an
 error if the base value's data length is less than the subvalue's offset.  In other words,
 the base value data can possibly extended, but "holes" cannot be created between the end
 of the base data and the subvalue data.
*/

static void writeValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize size)
{
  SubValueRefConPtr refCon = (SubValueRefConPtr)CMGetValueRefCon(value);

  /* First do the error check to see if caller is trying to write outside the           */
  /* boundaries of the original subvalue definition.                                    */
  
  if (offset > refCon->size || size > refCon->size - offset) {
    CMError(refCon->sessionData, "Attempt to write too much data into a subvalue in container \"^0\"", CMReturnContainerName(CMGetValueContainer(value)));
    return;
  }
  
  /* Now all we have to do is simply write the data to the base value.  As with reading */
  /* we have to remember to relocate the passed offset into the offset within the base  */
  /* value.  Note, the check for attempt to create "holes" is done by the API.          */
  
  CMWriteValueData(refCon->baseValue, buffer, refCon->offset + offset, size);
}


/*---------------------------------------------------------------------*
 | insertValueData_Handler - CMInsertValueData() dynamic value handler |
 *---------------------------------------------------------------------*
 
 The buffer is inserted into the subvalue's data at the specified offset within the
 subvalue.  size bytes are inserted.  Note, this inserts data in the base value.  This is
 equivelent to doing a CMInsertValueData() directly to the base value data with the offset
 appropriately relocated.  The offset here is with respect to the subvalue.
 
 It is an error to make a subvalue larger than its original size definition as passed to
 CMNewValue().  Thus doing a CMInsertValueData() on the subvalue will always result in an
 error unless some of the subvalue data has been deleted to make the current subvalue size
 less than the original size.  Further, this can only happen if the data deleted was off
 the end of the original base value data.
 
 Given that the insertion does not change the size definition of the subvalue, there can
 still be an error if the offset within the subvalue is such that a hole would be created
 off the end of the base value data.  This is the same restriction as for writing, since
 insertion is a form of writing.
*/

static void insertValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize size)
{
  SubValueRefConPtr refCon = (SubValueRefConPtr)CMGetValueRefCon(value);

  /* Check to see if the insertion would cause the subvalues definition to change...    */
  
  if (getValueSize_Handler(value) + size > refCon->size) {
    CMError(refCon->sessionData, "Attempt to insert too much data into a subvalue in continer \"^0\"", CMReturnContainerName(CMGetValueContainer(value)));
    return;
  }
  
  /* Insert the data.  As with the write case, the "hole" check will be done by the API.*/
  /* Also, just like everwhere else, we must reloacte the subvalue offset to that of the*/
  /* base data value.                                                                   */
  
  CMInsertValueData(refCon->baseValue, buffer, refCon->offset + offset, size);
}


/*---------------------------------------------------------------------*
 | deleteValueData_Handler - CMDeleteValueData() dynamic value handler |
 *---------------------------------------------------------------------*
 
 Deletes size bytes from the subvalue data starting at the offset within the subvalue.
 Note, this deletes data in the base value.  This is equivelent to doing a
 CMDeleteValueData() directly on the base value with the offset appropriate relocated.
 The offset here is with respect to the subvalue.
 
 The only way the current size of the subvalue can be made smaller is if the subvalue maps
 over the end of the base data and it is then deleted. Only if this happens can inserts be
 done (see insertValueData_Handler() above).
 
 In the limit, if the subvalue entirely maps over the base value data, then it is possible
 to null out the data for the base value.
  
 If the subvalue offset is greater than the subvalue data size, nothing is deleted.  The
 amount to delete may be greater than the current subvalue data size.  In that case, all
 the data starting from the offset will be deleted.
 
 If ALL the subvalue data is deleted, the value is defined as null, i.e. a data length of
 0.  Generally, the offset defioned for the subvalue will not be 0.  Thus if the deletions
 are to the end of the base value data as described above, it is possible that the subvalue
 size be 0 even though the base value's size is not.
*/

static void deleteValueData_Handler(CMValue value, CMCount offset, CMSize size)
{
  SubValueRefConPtr refCon = (SubValueRefConPtr)CMGetValueRefCon(value);
  CMSize            actualSubValueSize = getValueSize_Handler(value);
  
  /* There is nothing to delete if the offset within the subvalue is beyond the end of  */
  /* the subvalue's definition.                                                         */
  
  if (offset >= actualSubValueSize) return;
  
  /* Truncate the max size to within the subvalue...                                    */
  
  if (size > actualSubValueSize) size = actualSubValueSize;
  
  /* Do the delete. The "hole" check is done by the API.  As usual, the offset must be  */
  /* relocated.                                                                         */
  
  CMDeleteValueData(refCon->baseValue, refCon->offset + offset, size);
}


/*---------------------------------------------------------------*
 | getValueInfo_Handler - CMGetValueInfo() dynamic value handler |
 *---------------------------------------------------------------*

 The specified information for the refNum associated with a subvalue is returned. A
 parameter may be NULL indicating that info is not needed.
 
 We always want the info for the dynamic value's base value (NOT the one we're taking the
 subvalue of).  This means we would simply do a CMGetBaseValue() and pass the resulting
 value to CMGetValueInfo(). If that's all we need to do we can let the Container Manager
 do it!  Its automatic handler inheritance will do it.
 
 Based on this fact, we show the function code (all one line of it).  The
 subValueMetahandler() returned NULL for this handler indicating it isn't used for this
 dynamic value.
*/

static void getValueInfo_Handler(CMValue value, CMContainer *container, CMObject *object,
                                 CMProperty *property, CMType *type,
                                 CMGeneration *generation)
{ 
  CMGetValueInfo(CMGetBaseValue(value), container, object, property, type, generation);
}


/*---------------------------------------------------------------*
 | setValueType_Handler - CMSetValueType() dynamic value handler |
 *---------------------------------------------------------------*
 
 The type ID from the type is set for the specified value.  
 
 See comments in getValueInfo_Handler() above for the reason this code is not used.
*/

static void setValueType_Handler(CMValue value, CMType type)
{
  CMSetValueType(CMGetBaseValue(value), type);
}


/*---------------------------------------------------------------------------*
 | setValueGeneration_Handler - CMSetValueGeneration() dynamic value handler |
 *---------------------------------------------------------------------------*
 
 The generation for the specified subvalue is set.  The generation number must be greater
 than or equal to 1.
 
 See comments in getValueInfo_Handler() above for the reason this code is not used.
*/

static void setValueGeneration_Handler(CMValue value, CMGeneration generation)
{
  CMSetValueGeneration(CMGetBaseValue(value), generation);
}


/*---------------------------------------------------------------*
 | releaseValue_Handler - CMReleaseValue() dynamic value handler |
 *---------------------------------------------------------------*
 
 The subvalue association between the refNum and the subvalue is destroyed.  There should
 be no further operations on the subvalue once this routine is called.  This handler is
 only called if this is the LAST use of the dynamic value.
 
 A count is kept by the Container Manager of every CMUseValue() and CMNewValue().  Calling 
 CMReleaseValue() reduces this count by one.  When the last release is done on the dynamic
 value, this handler will be called.
 
 Note, in the context of subvalues, the only thing to do here is release the base value
 (i.e., the one we're taking the subvalue of -- we did a CMUseValue() in it in the "use
 value" handler) and free the refCon memory.  This keeps the use counts correct.  Keeping
 usage counts correct is important for dynamic values, because if the base value happens
 itself to be a dynamic value (e.g., subvalue of a subvalue!), then we would get an error
 at container close time if the use count of a dynamic value was not 0, i.e., was not
 completely released.
 
 The reason the Container Manager is so insistent on forcing a release for every use of a
 dynamic value is mainly because of this particular handler.  For subvalues it is not
 obvious why (except perhaps the desire to free the refCon memory).  But if you were, for
 example, indirecting through a pathname to a file, you would also be using a dynamic
 value.  And for that case you would want to know if you didn't completely release the
 dynamic value, because in that case you most probably didn't close the file you
 indirected to.  The close would be done at release time.
 
 Warning!  It is an error for the release handler to release its passed base value. It can
 free any other values, just not its base.  The reason is that the Container Manager is
 responsible for calling the release handlers for all base types.
*/

static void releaseValue_Handler(CMValue value)
{
  SubValueRefConPtr refCon = (SubValueRefConPtr)CMGetValueRefCon(value);
  CMSession         sessionData = refCon->sessionData;
  
  CMReleaseValue(refCon->baseValue);            /* release the value we were subvaluing */
  CMFree(refCon, sessionData);                  /* free our refCon                      */
}
                             
                              CM_END_CFUNCTIONS
