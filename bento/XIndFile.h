/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                      <<<        xindfile.h        >>>                     |
 |                                                                           |
 |       Example Indirect File Dynamic Value Handler Package Interfaces      |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  4/4/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This file contains the interfaces for indirect file value example of dynamic value use.
 See        XIndFile.c        for complete documentation on the implementation.  What is
 provided here is all that you would need to use indirect file values.  See
 newIndirectFileValue() for further details.
*/


#ifndef __XINDFILEHANDLERS__
#define __XINDFILEHANDLERS__

#include "CMAPI.h" 

                                  CM_CFUNCTIONS

CMValue CM_FIXEDARGS newIndirectFileValue(CMObject object, CMProperty property,
                                          CMType type,
                                          char CM_PTR *pathname, char CM_PTR *mode);
  /*
  This is a special case of CMNewValue() in that a new value entry of the specified type is
  created for the specified object with the specified property.  A refNum for this value is
  returned. However, unlike CMNewValue(), the refNum is associated with an value whose data
  (file) is indirectly pointed to by a pathname.  Whenever the returned refNum is used for
  a value operation, that operation will take place on the file pointed to by the pathname.
  
  The specified file is opened according to the open mode.  This may have any one of the
  the following values:
    
          "r", "rb"     open the file for reading
          "r+", "rb+"   open the file for updating
          "w+", "wb+"   truncate or create file and open for updating (reading/writing)
          
  The "b" is always implied if it is not explicitly specified.  If a write mode ("w") is
  specified with a "+", the "+" is assumed.
  
  Note, if the file is opened for writing ("wb+"), then all future uses of the value, i.e.,
  via CMUseValue() will imply update mode ("rb+").  Further, dynamic value operations do
  both reads and writes of their data.  Since a file type may be a base type of some other
  type, update mode is always assumed ("wb+").
  
  If a file is opened for reading, then a CMUseValue() will open preserve the open status
  and open for reading only.  Similarly, update mode is preserved.  The write is the only
  exception.  This is because there is no way (in this example at least) to tell
  CMUseValue() a change in open mode.  Sorry!  It bothers me too!
   
  The indirect value we want to support here is for data "pointed to" by a file pathname.
  All value operations on the indirect value will then be to the file.
   
  Note, multiple indirections are permitted!  That is the data pointed to by an indirect
  value might be for another indirect value.
  
  There is one restriction on this indirect value implementation. That is that data inserts
  and deletes are NOT supported.  File indirections are supported using standard C stream
  I/O for portability of this example.  With standard C there is no way to cut a file size
  down.  Hence deletes cannot be done.  Although inserts could be supported, the cost in
  data I/O is potentially expensive.  Thus inserts are not supported either.
  */
  

#define FileValueGlobalName  (CMGlobalName)"User:FileValue" /* file global name (type)  */

#define RegisterFileValueType(container) \
    (CMSetMetaHandler(CMGetSession(container), FileValueGlobalName, fileDynamicValueMetahandler), \
     CMRegisterType(container, FileValueGlobalName))
  /*
  If file values are created in a container and then the executing session terminated, the
  file values can be used in a future session just like normal values by calling
  CMUseValue().  The only requirement is that you must register the crypt base type prior
  to doing the CMUseValue().  To make this simpler, the above macro is defined. Just call
  RegisterFileValueType() to do the registration.
  
  Note RegisterFileValueType() is defined as an expression which can be assigned to a 
  CMType if you so desire.  This would be done to mix the file type into some other
  combination of base types.
  
  A new value with a file type requires two values passed to CMNewValue(), i.e., the 
  pathname and the open mode.  The parameters are described in newIndirectFileValue()
  above.  This fact must be known when mixing file types with other types to set the proper
  CMNewValue() "..." parameters.
  */
  
  
CMHandlerAddr CM_FIXEDARGS fileDynamicValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType);
  /*
  This metahandler is called by the Container Manager for dynamic value creation to allow
  the Container Manager to get the addresses of the "metadata", "new value", and "use
  value" handlers.  These handlers are used as part of the process of creating dynamic
  values.  See code for further documentation.
  
  Note, the Container Manager uses the address of this metahandler when it sees a type
  which has this metahandler registered for it.  Registering the metahandler and the type
  can be done with the RegisterFileValueType macro defined above.
  */

                                CM_END_CFUNCTIONS

#endif
