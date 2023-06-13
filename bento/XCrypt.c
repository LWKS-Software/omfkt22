/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                       <<<        XCrypt.c        >>>                      |
 |                                                                           |
 |            Example Encryption/Decryption Value Handler Package            |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  5/11/92                                  |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file illustrates how one might implement "special value data manipulations" using
 the "dynamic value" mechanisms provided by the Container Manager.  Dynamic values are
 special values which "know" how to manipulate on their own value data.  They do this
 through a set of value operation handlers which are expected to be semantically the same
 as the standard API value operations.  This is described in more detail later.

 The "special value data manipulations" dynamic value we want to support here is for
 simple encryption and decryption of value data.  All value operations on the dynamic
 value will then be encrypted or decrypted appropriately.  The encryption/decryption
 algorithm we use is a simple "exclusive-or" of a key with the data.  The key is cyclicly
 used over the data.

 The key is passed as a null-terminated string.  There is no limit on its length.  A null
 string has the effect of no encryption or decryption.

 There is one restriction on this encryption/decryption value implementation. Data inserts
 and deletes are NOT supported.  This is because the "exclusive-or" algorithm depends on
 the data offset in order to know how to use the key.  Inserts and deletes affect all
 data that follows.  Hence there would be no way to know what part of the key was
 originally used.

 Basically, dynamic values are similar to C++ objects where the handlers are the object's
 methods.  Dynamic values are a generalized mechanism that provides for type inheritance.
 They are similar to C++ objects where the handlers are the object's methods and an
 object's type represents its class.  They are dynamic in the sense that they only exist
 from creation (discussed below) and last until until they are released (CMReleaseValue()).

 The encryption/decryption example we will illustrate here illustrates the type inheritance
 where the type of the value has as its base type, a "crypt" type.  As discussed in the
 companion header file for this code, the intent is to define a type which can be mixed in
 with other types.  In its most basic use, the "crypt" type would be a base type on a
 descriptor type (i.e., a type which describes what we're kind of encryption this is, or
 whatever).  In that case all I/O to a value's data would then be encrypted or decrypted
 appropriately.

 By defining the "crypt" operations in terms of its base type, we can use it more generally
 with other types.  The example illustrated in the header is for a "file" type that does
 I/O indirectly to a file.  So by using both the "crypt" type and the "file" type, I/O
 could then be encrypted/decrypted to a separate file.

 Follow the code in this file in the order presented.  The comments document the steps
 required to support the "crypt" type.  The newCryptValue() routine defines the interface
 that can be used to create a crypt dynamic value.  It would be called just like any other
 Container Manager API routine.

 Note, however, as just stated, you won't need newCryptValue() except perhaps for the
 basic case.  For mix-ins, you would follow the procedures described in the header file.
*/


/*------------------------------------------------------------------------------------*
 | DOS (80x86) USERS -- USE THE "LARGE" MEMORY MODEL WHEN COMPILED FOR 80X86 MACHINES |
 *------------------------------------------------------------------------------------*

 The Container Manager is built with this same option and assumes all handler and
 metahandler interfaces are similarly built and can be accessed with "far" pointers.
*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifndef __CM_API__
#include "CMAPI.h"
#endif
#ifndef __XCRYPTHANDLERS__
#include "XCrypt.h"
#endif

                                  CM_CFUNCTIONS

/* See comments in      cmapienv.h      about CM_CFUNCTIONS/CM_END_CFUNCTIONS (used     */
/* only when compiling under C++).                                                      */


/* In order to make this code read from the top down, it is necessary in a few instances*/
/* to forward reference some metahandler functions.  Due to some "complaints" from      */
/* certain C compilers (who shall remain nameless), the forward declarations are placed */
/* here.  Basically, ignore this stuff.                                                 */

static CMHandlerAddr cryptValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType);


/* GENERAL NOTE...

   In order to make the code as general as possible, the session handlers provided by the
   API are used.  This includes memory allocation, deallocation, error reporter, and
   access to the container identification.  These interfaces allow us to use the same
   memory and error handlers that the Container Manager uses as provided to
   CMStartSession() through the methandler passed there.  The container identification
   interface allows us to add that information to the error reports.  The wording of these
   messages follows the "flavor" of those done by the Container Manager itself.
*/


/*---------------------------------------------------------------------------------*
 | newCryptValue - create refNum for a value that encrypts/decrypts its value data |
 *---------------------------------------------------------------------------------*

 This is a special case of CMNewValue() in that a new value entry of the specified type is
 created for the specified object with the specified property.  A refNum for this value is
 returned.  However, unlike CMNewValue(), the refNum is associated with a value that
 encrypts or decrypts its data.  Whenever the returned refNum is used for a value
 operation, that operation will encrypt or decrypt of its value appropriately.  The
 encryption/decryption algorithm used is a simple "exclusive-or" of a key with the data.
 The key is passed as a null-terminated string which is cyclicly used over the data.
 There is no limit on its length.  A null string has the effect of no encryption or
 decryption.

 The types are the key to generating dynamic values.  A dynamic value results when a value
 is created by to CMNewValue() or attempted to be used by CMUseValue(), and the following
 two conditions occur:

  1. The type or any of its base types (if any, created by CMAddBaseType()), are for
     global names that have associated metahandlers registered by CMRegisterType().

  2. The metahandlers support the operation type to return a "use value" handler, and
     in addition for CMNewValue(), "metadata" and "new value" handlers.

 "Metadata" handlers are used to define metadata that is a format description that directs
 CMNewValue() on how to interpret it's "..." parameters to produce a data packet of those
 parameters.  The data packet is sent to the "new value" handler. In this routine we will
 do a CMNewValue() to create the dynamic value with the newCryptValue() cryptKey as the
 "..." parameters.

 "New value" handlers are used to define initialization data for the "use value" handlers
 based on the data packets.  The "use value" handlers are called to set up and return
 refCons.  Another metahandler address is also returned from the "use value" handler.  This
 is used to get the address of the value operation handlers corresponding to the standard
 API value routines.

 The goal is to create a dynamic value to return, which when used will produce the desired
 crypt. Thus we need two types:

  1. A type that describes the meaning of this crypt value.  That is the type passed to
     this routine.

  2. A base type that represents a "crypt" type in general.  The user's type is given the
     "crypt" type as its base type.

 It is that general "crypt" base type that most of this file supports.  Namely the various
 metahandlers and handlers for encrypt and decryption.  Indeed, the only  thing we do here
 is set up the types, register their methandlers, and do the CMNewValue().

 Note, you would only use newCryptValue() to explicitly create a value with encrypted data.
 This routine is NOT used for layering of base types as discussed in the header.
*/

CMValue CM_FIXEDARGS newCryptValue(CMObject object, CMProperty property, CMType type,
                                   char CM_PTR *cryptKey)
{
  CMContainer container;
  CMSession   sessionData;
  CMType      cryptType;
  CMValue     dynamicValue;

  /* Get the container refNum and session data pointer. They are needed for registering */
  /* metahandlers and types and for reporting errors.  Note, if the container is passed */
  /* as NULL, the session pointer will end up being NULL. In that case, we can't report */
  /* the error and just return NULL.                                                    */

  container   = CMGetObjectContainer(object);
  sessionData = CMGetSession(container);

  if (sessionData == NULL)              /* if no session...                             */
    return (NULL);                      /* ...there is nothing much else we can do here */

  /* Do a safety check on the refNums...                                                */

  if (object == NULL || property == NULL || type == NULL || cryptKey == NULL) {
    CMError(sessionData, "Null object, property, type refNum, or key refNum passed to newCryptValue()");
    return (NULL);
  }

  /* If you wanted to really be paranoid, you could check that the property and type    */
  /* both belonged to the same container.  But lets be "trusting" for now!              */

  /* Register the crypt type (or reregister it just to make sure its registered) and    */
  /* then define the metahandler for that type which will return the "metadata", "new   */
  /* value", and "use value" handlers (described later in this file).  Note, the crypt  */
  /* global name, as well as the metahandler are published in the crypt header file to  */
  /* allow other combinations of base types that might want crypt as one of their       */
  /* inherited types.  Further, to use the value we're creating in the future sessions, */
  /* the caller simply does a CMUseValue().  But that requires the type and its         */
  /* metahandler be registered just as we do here.  As a convenience we provide a macro */
  /* to do this in the header file.  For clearity, we do it explicitly here.            */

  CMSetMetaHandler(CMGetSession(container), CryptValueGlobalName, cryptDynamicValueMetahandler);
  cryptType = CMRegisterType(container, CryptValueGlobalName);
  if (cryptType == NULL) return (NULL);

  /* The crypt type will be defined as a base type for the caller's type.  The caller's */
  /* type is already defined.  It is assumed to be a type that describes what kind of   */
  /* encryption this is.  The type heirarchy we want looks something like this:         */

  /*                                Crypt Type (bottom type)                              */
  /*                                          *                                         */
  /*                                          *                                         */
  /*                                          *                                         */
  /*                                Caller's Type (top type)                            */

  /* In other words, we want the crypt type as a base type of the description type.     */
  /* This is easily accomplished as follows:                                            */

  CMAddBaseType(type, cryptType);

  /* Base types are essentially values for a special "base type" property added to the  */
  /* type object.   Above we're defining cryptType as a single base type for the type.  */

  /* That's about all there is to it!  Now all that's left is to do the CMNewValue()    */
  /* to create the dynamic value.                                                       */

  dynamicValue = CMNewValue(object, property, type, cryptKey);

  /* The dynamicValue is now created!  We can return it to the caller.  Note that the   */
  /* cryptKey is passed to CMNewValue().  It has no idea what it is!   But it does know */
  /* how to deal with it.  Specifically, it passes it to the "metadata" and "new value" */
  /* handlers during the creation of the dynamic value. CMNewValue() knows there is one */
  /* additional parameter because of the metadata we return from the "metadata" handler.*/
  /* You can see it works something like "printf()", although across two different      */
  /* places.  The metadata tells CMNewValue() to consume the additional parameter just  */
  /* like the "printf()" format string tells "printf()".                                */

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


/*-----------------------------------------------------------------------------*
 | cryptDynamicValueMetahandler - matahandler for crypt dynamic value creation |
 *-----------------------------------------------------------------------------*

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

CMHandlerAddr CM_FIXEDARGS cryptDynamicValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType)
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


/*-----------------------------------------------------------*
 | metaData_Handler - return the matadata for the crypt type |
 *-----------------------------------------------------------*

 The "metadata" handler is called from CMNewValue() so that the proper number of its "..."
 parameters can be placed into a data packet to be passed to the "new value" handler. This
 handler takes the type for which we ant to return the corresponding metadata.

 In the case of crypt values, we want to pass the newCryptValue() parameter that defines
 the encryption key, i.e., the cryptKey.  The metadata "%p" will serve the purpose.  It
 specifies a pointer to the key string.

 Note, we can define any format we thing is appropriate for the "new value" handler so
 int  as it uses CMScanDataPacket() to extract the parameters and uses this handler itself
 to get the metadata format string.  That's what we will do when we get to the "new value"
 handler.
*/

static CMMetaData metaData_Handler(CMType type)
{
  CMType unused = type;

  return ((CMMetaData)"%p");                  /* for the cryptKey pointer               */
}


/*-------------------------------------------------------------------------*
 | newValue_Handler - create permanent data to for the "use value" handler |
 *-------------------------------------------------------------------------*

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

 In this example we only have a single base type, the "crypt" type, which has metadata
 defined to pass a pointer to the encryption key string.

 From this data we want to write permanent information so that the "use value" handler,
 which is always called for CMNewValue() and CMUseValue(), can create an appropriate refCon
 to pass among the value operation handlers.  The data layout chosen is as follows:     */

 struct CryptData {                 /* Encrypt/decrypt data layout:                     */
   unsigned int  keySize;           /*    size of the encryption/decryption key         */
   unsigned char theKey[1];         /*    encryption/decryption key starts here         */
 };
 typedef struct CryptData CryptData, *CryptDataPtr;                                     /*

 In the above data, we only have to remember the encryption key and its size (the reason
 for the size will be explained shortly).  This is the data we will write to the dynamic
 value's base value.  It is thus permanently remembered.  Now, if the container is opened
 for reading, if a CMUseValue() is done on the value, the "use value" handler will read
 the data just as in the CMNewValue() case.  Remember that newCryptValue() is done only to
 initially create the crypt value.  Future uses don't call it.  So nothing should be
 written that can't be used by only calling "use value" handler without its "new value"
 handler.

 In this particular example, whe have a "security leak"!  We are writing the key to the
 base value.  If that base value is a real container value, then we are writing the key
 into the container.  As an example, we're not going to get to picky here.  We're not
 trying to stop the CIA!  So we will leave it at that.

 For what it's worth, if the base value is NOT a real value, but a dynamic value that
 is at a lower level in the base type inheritance chain, then we're writing the key to
 the dynamic value.  It may choose to dispose of it any way it sees fit.

 Take, for example, the "file" base type that writes to a file instead of the container
 (done in ExampleIndFileHandlers.[hc] and discussed in the header for this file).  Then
 we will be writing the key to the file and not the container.  The file base type
 highlights why we save the key size as part of the data.  The key is variable length.
 But you would think that the "use value" handler could do a CMGetValueSize() on the
 dynamic base value.  In this file case you would end up getting the size of the file!
 The key is in there at the beginning of the file.  But we would not know its length.
 Hence the safest (only) thing to do is explicitly write the size.  This will mean that
 the "use value" handler will have to do two reads; first for the size and then for the
 key.  But that's life!

 We don't know our context here.  Nor should we!  We only know we're writing special data
 to the base value.  Since we also do all our value handler operations to the base value
 we must take into account the data we have written.  In particular, the user of the
 value we return from here doesn't know we wrote this data.  So a user will always think
 of the data s/he reads or writes starts at offset 0.  In reality it will be offset by the
 size of the data we write.  Our handlers must take this into account.

 This will generally be true of all generic type dynamic values that do all their
 operations on only their base values.  The subvalue example did its operations on an
 existing value.  The file example does its operations to an external file.  Here we have
 a "true" atomic base type that uses only its base value.  Its the main distinction of
 this example.

 On final point, if any errors are reported, we abort the execution of the program in this
 example.  As documented, error reporters are not supposed to return.  But in case they
 do, the Container Manager wants to know about it!  The CMBoolean function result is used
 for that purpose.  Nonzero should be returned for success, and if this should happen to
 return for a failure, 0 should be returned as the function result.
*/

static CMBoolean newValue_Handler(CMValue dynamicBaseValue, CMType type, CMDataPacket dataPacket)
{
  CMContainer   container   = CMGetObjectContainer(type);
  CMSession     sessionData = CMGetSession(container);
  unsigned char *cryptKey;
  CMCount       i;
  unsigned int  keySize;

  /* Scan the dataPacket to put its data back into variables.  The API provides         */
  /* CMScanDataPacket() to do that.  We reverse what CMNewValue() did by using our own  */
  /* "metadata" handler to get the metadata to direct the extraction of dataPacket      */
  /* fields back to variables (sort of like a "scanf()").                               */

  i = CMScanDataPacket(type, metaData_Handler(type), dataPacket, &cryptKey);

  if (i != 1) {
    CMError(sessionData, "Unable to get all of CMNewValue()'s \"...\" cryptKey parameter in container \"^0\"", CMReturnContainerName(CMGetObjectContainer(type)));
    return (0);
  }

  /* Write the encryption key that the "use value" handler will to build its refCon. As */
  /* discissed above, we write the size of the key and then the key itself.  We're going*/
  /* to cheat a little here and write the two values separately just like the "use      */
  /* value" handler will read them.  We never really use the CryptData struct above.    */

  keySize = strlen((char *)cryptKey);

  CMWriteValueData(dynamicBaseValue, (CMPtr)&keySize, 0, sizeof(unsigned int ));
  CMWriteValueData(dynamicBaseValue, (CMPtr)cryptKey, sizeof(unsigned int ), (CMSize)keySize);

  return (1);                               /* that's all there is to do here!          */
}


/*--------------------------------------------------------------------------*
 | useValue_Handler - create a dynamic value and its associated metahandler |
 *--------------------------------------------------------------------------*

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
 the CryptValueDataPtr typedef defined in the "new value" handler.  It is just the
 original encryption key and its size.

 The refCon we build from this data has the following layout:                           */

 struct CryptRefCon {                       /* Crypt value handler's refCon:            */
   CMSession     sessionData;               /*    ptr to the current container session  */
   unsigned int  dataOffset;                /*    amount to offset user's offsets       */
   unsigned int  keySize;                   /*    size of the encryption/decryption key */
   unsigned char theKey[1];                 /*    encryption/decryption key starts here */
 };
 typedef struct CryptRefCon CryptRefCon, *CryptRefConPtr;                               /*

 The current session data pointer, which we need to allocate the refCon is kept just as a
 convenience in case we need it again.  At the very least it will be needed to free the
 refCon.

 As discussed in the "use value" handler all operations are to the base value.  Hence, the
 data that the "new value" handler wrote is in there.  So we must offset the user's offsets
 to the value operations by the amount of the data we wrote and s/he has no knowledge of.
 Hence the dataOffset field in the refCon.

 The refCon is accessable to all the handler routines via a CMGetValueRefCon() call.  The
 keySize is the size of the encryption key.  The "new value" handler removed the delimiting
 null when it wrote the data.  We don't need it.

 Note, as with the "new value" handler, if any errors are reported, we abort the execution
 of the program in this example. As documented, error reporters are not supposed to return.
 But in case they do, the Container Manager wants to know about it!  The CMBoolean function
 result is used for that purpose.  Nonzero should be returned for success, and if this
 should happen to return for a failure, 0 should be returned as the function result.
*/

static CMBoolean useValue_Handler(CMValue dynamicBaseValue, CMType type,
                                  CMMetaHandler *metahandler, CMRefCon *refCon)
{
  CMContainer    container   = CMGetObjectContainer(type);
  CMSession      sessionData = CMGetSession(container);
  unsigned int   keySize;
  CryptRefConPtr myRefCon;

  /* Since the key can be any size, we must get its size before we can allocate our     */
  /* refNum. The size is the first int  in the dynamicBaseValue's data.                 */

  if (CMReadValueData(dynamicBaseValue, (CMPtr)&keySize, 0, sizeof(unsigned int )) != sizeof(unsigned int )) {
    CMError(sessionData, "Incorrect byte length read while reading encryption key size in container \"^0\"", CMReturnContainerName(container));
    return (0);
  }

  /* Allocate the refCon that we will pass among the handlers.  Since we are doing a    */
  /* dynamic allocation here we will use the "malloc" handler defined for the container.*/

  myRefCon = (CryptRefConPtr)CMMalloc(sizeof(CryptRefCon) + keySize, sessionData);
  if (myRefCon == NULL) {
    CMError(sessionData, "Cannot allocate space for file encryption handler refCon in container \"^0\"", CMReturnContainerName(container));
    return (0);
  }

  myRefCon->sessionData = sessionData;                /* save current session pointer   */

  /* Read the key directly into our allocated refNum and save the key size...           */

  myRefCon->keySize = keySize;                        /* save key size                  */

  if (CMReadValueData(dynamicBaseValue, (CMPtr)myRefCon->theKey, sizeof(unsigned int ), (CMSize)keySize) != keySize) {
    CMError(sessionData, "Incorrect byte length read while reading encryption key in container \"^0\"", CMReturnContainerName(container));
    CMFree(myRefCon, sessionData);
    return (0);
  }

  /* Set the dataOffset field of the refCon with the amount of data we read in both the */
  /* above reads...                                                                     */

  myRefCon->dataOffset = sizeof(unsigned int ) + keySize;

  /* All that's left to do here is to return the refCon and the pointer to the value    */
  /* operations metahandler to the Container Manager who called us...                   */

  *metahandler = (CMMetaHandler)cryptValueMetahandler;/* return metahandler             */
  *refCon      = (CMRefCon)myRefCon;                  /* ...and refCon                  */

  return (1);                                         /* we're now ready to go          */
}


/*---------------------------------------------------------------------*
 | cryptValueMetahandler - metahandler for a dynamic value handler set |
 *---------------------------------------------------------------------*

 This is the metahandler that the "use value" handler returns to the Container Manager.
 It defines the addresses for all the value operations handlers supported for files.

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

 In this example we had two types, the "crypt" base type and its base value type, i.e, the
 one passed to newCryptValue().  The dynamic value returned from there was for the crypt
 type.  So if we don't supply a value handler for a particular operation, the operation
 will end up be done to the base value.

 Taking advantage of this means you don't have to supply a handler unless it is absolutely
 necessary.  This minimizes the coding, not to mention code size!  We actually do take
 advantage of the method ('er, excuse me, value operation) inheritance here.  All value
 operations which do not involve encryption, decryption, or data offsets (remember we
 wrote our own data to the base value) are not defined (i.e., we return NULL for their
 address) as handlers.

 The "setValueType"/"setValueGeneration" handlers are such handlers.  It should be pointed
 out that if the type is changed, you are "pulling" the rug out" from under the dynamic
 value creation.  Once released, the value will not have a crypt base type, unless, of
 course, the new type had one.

 Note that we actually declare all the value operations handlers as "static" since they
 only need to be visible in this file.  The Container Manager gets at them through the
 addresses we return from here.

 The handlers do their work by calls back to the Container Manager.  But the values passed
 back cannot be the dynamic value passed in.  This could lead to a recursive and endless
 loop.  This IS detected by the Container Manager as an error condition.

 In our crypt case, the handlers will use the refCon to do their respective operations on
 the base value which we get from the passed value by doing a CMGetBaseValue() on it.

 By the time a dynamic value operation handler has been called, all validation checks have
 been done on the dynamic value by the Container Manager.  So they need not be done every
 time here.  Validation checks include such things as the Container Manager being
 initialized, non-NULL value pointers, etc.  In short, all the validations the value
 routines do for normal values before discovering it should call a dynamic value handler.
*/

static CMHandlerAddr cryptValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType)
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
    case  0:  return ((CMHandlerAddr)getValueSize_Handler);   /* CMGetValueInfoOpType   */
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

/* Since all that's left are the value operations that do encrypting and decrypting, we */
/* need a private routine to do the encryption/decryption work...                       */

/*----------------------*
 | Encrypt/Decrypt Data |
 *----------------------*

 This routine takes data, consisting of dataSize bytes, and encrypts or decrypts it using
 the key, keySize bytes.  The algorithm does a simple "exclusive-or" of the key with the
 data.  So encryption and decryption are the same.  Which way it happens depends on whether
 the call is for reading or writing.

 Note, the key is cyclicly used over the data.  As such which key character used depends
 on the offset of each data character.  Therefore the starting offset for the data is
 passed.

 This implementation only makes sense if no data is inserted or deleted once encrypted.
 Hence, inserts and deletes are not supported in this example implementation.
*/

static void CM_NEAR crypt(unsigned char *data, unsigned int  dataSize,
                          unsigned char *key, unsigned int  keySize,
                          unsigned int  offset)
{
  unsigned char *keyStart = key;

  if (keySize == 0) return;                 /* no key means no encryption               */

  key += (offset % keySize);                /* starting key char is a function of offset*/

  while (dataSize--) {                      /* process each character of the data...    */
    *data++ ^= *key++;                      /* ...encrypt/decrypt it                    */
    if (++offset % keySize == 0)            /* ...if we exhausted the key...            */
      key = keyStart;                       /* ...cycle it                              */
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
/* the "use value" handler.  The refCon contains the encryption key and key size.       */
/* Remember, we have to adjust all passed offsets by the keysize sinze we wrote the key */
/* to the base value (in the "new value" handler) but the caller doesn't know that.     */


/*---------------------------------------------------------------*
 | getValueSize_Handler - CMGetValueSize() dynamic value handler |
 *---------------------------------------------------------------*

 The character size of the file is returned.

 We must adjust the size by the size of our own data.  The user is not aware of that stuff.
 We do similar offset arithmetic in the handlers that involve offsets.
*/

static CMSize getValueSize_Handler(CMValue value)
{
  CryptRefConPtr refCon = (CryptRefConPtr)CMGetValueRefCon(value);

  return (CMGetValueSize(CMGetBaseValue(value)) - refCon->dataOffset);
}


/*-----------------------------------------------------------------*
 | readValueData_Handler - CMReadValueData() dynamic value handler |
 *-----------------------------------------------------------------*

 The encrypted data, starting at the offset, for the value is read and decrypted into the
 buffer.  The size of the data read is returned.  Up to maxSize characters will be read
 (can be 0).

 The data is read starting at the offset, up to the end of the data, or maxSize characters,
 whichever comes first.  Offsets are relative to 0, where 0 is the first byte of the
 file.  If the starting offset is greater than or equal to the current data size, no data
 is read and 0 returned.
*/

static CMSize readValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize maxSize)
{
  CryptRefConPtr refCon = (CryptRefConPtr)CMGetValueRefCon(value);
  CMSize         amountRead;

  /* Read the data and decrypt it...                                                    */

  amountRead = CMReadValueData(CMGetBaseValue(value), buffer, refCon->dataOffset + offset, maxSize);

  crypt((unsigned char *)buffer, (unsigned int )amountRead,
        refCon->theKey, refCon->keySize, (unsigned int )offset);

  return (amountRead);
}


/*-------------------------------------------------------------------*
 | writeValueData_Handler - CMWriteValueData() dynamic value handler |
 *-------------------------------------------------------------------*

 The buffer is encrypted and written to the container and defined as the data for the
 value.  If the value already has data associated with it, the buffer overwrites the "old"
 data starting at the offset character position.  size bytes are written.

 If the current size of the value data is T, then the offset may be any value from 0 to
 T.  That is, existing data may be overwritten or the value extended with new data.  The
 value of T can be gotton using CMGetValueSize().  Note, no "holes" can be created.  An
 offset cannot be greater than T.
*/

static void writeValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize size)
{
  CryptRefConPtr refCon    = (CryptRefConPtr)CMGetValueRefCon(value);
  CMContainer    container = CMGetValueContainer(value);
  unsigned char  *cryptBuffer;

  /* We must encrypt the passed buffer before sending it to the base value for writing. */
  /* It is possible that the buffer passed to us here is a simple string constant. ANSI */
  /* does NOT specify where string constants are placed.  They could be in the code.    */
  /* Since code segments may be execute only and not permit modification, we must copy  */
  /* the string to a local buffer and encrypt that.  [For what it's worth, the RS/6000  */
  /* showed this protected behavior.]                                                   */

  if (size == 0) return;                          /* nothing to do if no data to write  */

  cryptBuffer = (unsigned char *)CMMalloc(size, refCon->sessionData);
  if (cryptBuffer == NULL) {
    CMError(refCon->sessionData, "Unable to allocate output encryption buffer in container \"^0\"",
            CMReturnContainerName(container));
    return;
  }

  memcpy((char *)cryptBuffer, (char *)buffer, (size_t)size);

  /* Encrypt the data in the local buffer and write it.  After that we can free the     */
  /* buffer.                                                                            */

  crypt(cryptBuffer, (unsigned int )size, refCon->theKey, refCon->keySize, (unsigned int )offset);

  CMWriteValueData(CMGetBaseValue(value), cryptBuffer, refCon->dataOffset + offset, size);

  CMFree((CMPtr)cryptBuffer, refCon->sessionData);
}


/*---------------------------------------------------------------------*
 | insertValueData_Handler - CMInsertValueData() dynamic value handler |
 *---------------------------------------------------------------------*

 The buffer is encrypted and inserted into the value's data at the specified offset.  size
 bytes are inserted.

 If the current size of the value data is T, then the offset may be any value from 0 to
 T.  That is, the insertion may be anywhere within the data value or the value extended
 with new data.  The value of T can be gotton using CMGetValueSize().  Note, no "holes" can
 be created.  An offset cannot be greater than T.  Also, note, that an insertion at
 offset T is identical to a CMWriteValueData() to the same place.

 Note, this routine is not supported for crypt values.  See crypt() comments for further
 details.
*/

static void insertValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize size)
{
  CryptRefConPtr refCon = (CryptRefConPtr)CMGetValueRefCon(value);
  CMCount        unused1 = offset;
  CMSize         unused2 = size;
  CMPtr          unused3 = buffer;

  CMError(refCon->sessionData, "Insertions into encrypted data for container \"^0\" are not supported",
          CMReturnContainerName(CMGetValueContainer(value)));
}


/*---------------------------------------------------------------------*
 | deleteValueData_Handler - CMDeleteValueData() dynamic value handler |
 *---------------------------------------------------------------------*

 Deletes size bytes from the value data starting at the offset.  If the offset is greater
 than the value data size, nothing is deleted.  The amount to delete may be greater than
 the current data size.  In that case, all the data starting from the offset will be
 deleted.  If ALL the data is deleted, the value is defined as null, i.e. a data length of
 0.

 Note, this routine is not supported for crypt values.  See crypt() comments for further
 details.
*/

static void deleteValueData_Handler(CMValue value, CMCount offset, CMSize size)
{
  CryptRefConPtr refCon = (CryptRefConPtr)CMGetValueRefCon(value);
  CMCount        unused1 = offset;
  CMSize         unused2 = size;

  CMError(refCon->sessionData, "Deletions of encrypted data for container \"^0\" are not supported",
          CMReturnContainerName(CMGetValueContainer(value)));
}



/*---------------------------------------------------------------*
 | getValueInfo_Handler - CMGetValueInfo() dynamic value handler |
 *---------------------------------------------------------------*

 The specified information for the refNum associated with an value is returned.  A
 parameter may be NULL indicating that info is not needed.

 Since no encryption or decryption is involved, we actually use the inherited routine.
 cryptValueMetahandler() returned NULL for this handler indicating it isn't used for this
 dynamic value.  We do show what it would do if it were really called.
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

 Since no encryption or decryption is involved, we actually use the inherited routine.
 cryptValueMetahandler() returned NULL for this handler indicating it isn't used for this
 dynamic value.  We do show what it would do if it were really called.
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

 Since no encryption or decryption is involved, we actually use the inherited routine.
 cryptValueMetahandler() returned NULL for this handler indicating it isn't used for this
 dynamic value.  We do show what it would do if it were really called.
*/

static void setValueGeneration_Handler(CMValue value, CMGeneration generation)
{
  CMSetValueGeneration(CMGetBaseValue(value), generation);
}


/*---------------------------------------------------------------*
 | releaseValue_Handler - CMReleaseValue() dynamic value handler |
 *---------------------------------------------------------------*

 The association between the refNum and the indirect file dynamic value is destroyed.
 There should be no further operations on the value once this routine is called.  This
 handler is only called if this is the LAST use of the dynamic value.

 A count is kept by the Container Manager of every CMUseValue() and CMNewValue().  Calling
 CMReleaseValue() reduces this count by one.  When the last release is done on the dynamic
 value, this handler will be called.

 Note, in the context of these crypt values, the only thing to do here free the refCon
 memory.

 Warning!  It is an error for the release handler to release its passed base value. It can
 free any other values, just not its base.  The reason is that the Container Manager is
 responsible for calling the release handlers for all base types.
*/

static void releaseValue_Handler(CMValue value)
{
  CryptRefConPtr refCon = (CryptRefConPtr)CMGetValueRefCon(value);
  CMSession      sessionData = refCon->sessionData;

  CMFree(refCon, sessionData);                  /* free our refCon                      */
}

                              CM_END_CFUNCTIONS
