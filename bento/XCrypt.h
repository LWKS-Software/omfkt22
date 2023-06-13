/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                       <<<        xcrypt.h        >>>                      |
 |                                                                           |
 |       Example Encryption/Decryption Value Handler Package Interfaces      |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  5/11/92                                  |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This file contains the interfaces for Encryption/Decryption value example of dynamic
 value use.  See        XCrypt.c        for complete documentation on the implementation.
 What is provided here is all that you would need to use values that encrypt or decrypt
 data to and from a base value.  See newCryptValue() for further details.
 
 Note, this encryption/decryption example offers no new information on how to create
 dynamic values.  The examples for subvalues and indirect file pointers do that.  The
 main purpose of this example is to provide a dynamic value type that operates on its
 base value.  The interesting part is that we can then use this type AND the file type to
 illustrate multiple base types!  Specifically, a value that does encrypted file I/O.
 
 Assuming you are familiar with dynamic values by the time you are reading this (if not
 see the subvalue and file value examples), then in order to make a crypt/file type we
 want to produce the following type inheritance heirarchy:
 
                           File Type       Crypt type
                                    *     *
                                     *   *
                                      * *
                                Descriptor Type

 In other words, a "descriptor type" that describes the meaning, and two base types; a
 encryption/decryption type and a file type.  A depth-first walk of the heirarchy produces
 File Type at the bottom, then Crypt type, and finally, at the top, the Descriptor Type.
 
 The following inheritance will equally work:
 
                               File Type        (bottom type)
                                   *
                                   *
                                   *
                               Crypt Type       (middle type)
                                   *
                                   *
                                   *
                               Descriptor Type  (top type)
                               
 With this, the file type is a base type of the crypt type.
 
 So dynamic value types let you mix in ther types in different ways.  Using a crypt by 
 itself is uninteresting, unless of course you want to have all your data values
 encrypted!  But the overall algorithm presented in the code is very much like the subvalue
 and file value code.
 
 Using this header, however, you can build the crypt/file type.  Assuming you have
 registered a descriptor type, descriptorType, then you can add the base values to it as
 follows:
      
      CMAddBaseType(descriptorType, RegisterFileValueType(container));
      CMAddBaseType(descriptorType, RegisterCryptValueType(container));
 
 The best way to think of the ordering is that the base types for any one type are
 processed in order.  Processing includes first checking if each of those types has base
 types, and so on.  This effects the depth-first search.
 
 The above two CMAddBaseType() calls add the file type first and crypt type second.  This
 produces the first type inheritance heirarchy pictured above.
 
 To produce the second pictured inheritance heirarchy:
 
    cryptType = RegisterCryptValueType(container)
    CMAddBaseType(cryptType, RegisterFileValueType(container));
    CMAddBaseType(descriptorType, cryptType);
    
 Here the file type is made a base value of the crypt type and the crypt type, now with
 its file base type, made a base type of the descriptor type.
 
 This second method is not as general as the first.  That's because using a crypt type
 will always imply using the file as a base type.
 
 Leaving the types more-or-less "atomic" allows you to have mix-in classes (oops, sorry,
 types).  So the first method is desired.
 
 Finally, in all of this discussion, we have been using definitions provided in other
 headers.  Specifically the        xindfile.h        and ExampleSubValuehandlers.h.  The
 RegisterCryptValueType is a macro defined in this header.  You would, therefore, have to
 include those headers if you wanted to do the above operations.
*/


#ifndef __XCRYPTHANDLERS__
#define __XCRYPTHANDLERS__

#include "CMAPI.h" 

                                  CM_CFUNCTIONS 

CMValue CM_FIXEDARGS newCryptValue(CMObject object, CMProperty property, CMType type,
                                   char CM_PTR *cryptKey);
  /*
  This is a special case of CMNewValue() in that a new value entry of the specified type is
  created for the specified object with the specified property.  A refNum for this value is
  returned.  However, unlike CMNewValue(), the refNum is associated with a value that
  encrypts or decrypts its data.  Whenever the returned refNum is used for a value
  operation, that operation will encrypt or decrypt of its value appropriately.  The
  encryption/decryption algorithm used is a simple "exclusive-or" of a key with the data.
  The key is passed as a null-terminated string which is cyclicly used over the data. 
  There is no limit on its length.  A null string has the effect of no encryption or
  decryption.

  There is one restriction on this encryption/decryption value implementation. Data inserts
  and deletes are NOT supported.  This is because the "exclusive-or" algorithm depends on
  the data offset in order to know how to use the key.  Inserts and deletes affect all
  data that follows.  Hence there would be no way to know what part of the key was
  originally used.
  
  Note, you would only use newCryptValue() to explicitly create a value with encrypted
  data.  This routine is NOT used for layering of base types as discussed above.
  */
  

#define CryptValueGlobalName  (CMGlobalName)"User:CryptValue" 

#define RegisterCryptValueType(container) \
    (CMSetMetaHandler(CMGetSession(container), CryptValueGlobalName, cryptDynamicValueMetahandler), \
     CMRegisterType(container, CryptValueGlobalName))
  /*
  If crypt values are created in a container and then the executing session terminated, the indirect
  crypt values can be used in a future session opening just like normal values by calling
  CMUseValue().  The only requirement is that you must register the crypt base type prior
  to doing the CMUseValue().  To make this simpler, the above macro is defined. Just call
  RegisterCryptValueType() to do the registration.
  
  Note RegisterCryptValueType() is defined as an expression which can be assigned to a 
  CMType if you so desire.  This would be done to mix the crypt type into some other
  combination of base types.

  A new value with a crypt type requires a single value passed to CMNewValue(), i.e.,
  cryptKey.  That parameter is described in newCryptValue() above.   This fact must be
  known when mixing crypt types with other types to set the proper CMNewValue() "..."
  parameters.
  */
  
  
CMHandlerAddr CM_FIXEDARGS cryptDynamicValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType);
  /*
  This metahandler is called by the Container Manager for dynamic value creation to allow
  the Container Manager to get the addresses of the "metadata", "new value", and "use
  value" handlers.  These handlers are used as part of the process of creating dynamic
  values. See code for further documentation.
  
  Note, the Container Manager uses the address of this metahandler when it sees a type
  which has this metahandler registered for it.  Registering the metahandler and the type
  can be done with the RegisterCryptValueType macro defined above.
  */
  
                              CM_END_CFUNCTIONS

#endif
