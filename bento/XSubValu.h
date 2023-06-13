/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                     <<<        xsubvalu.h         >>>                     |
 |                                                                           |
 |         Example SubValue Dynamic Value Handler Package Interfaces         |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 3/10/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This file contains the interfaces for subvalue example of dynamic value use.  See 
        XSubValu.c         for complete documentation on the implementation.  What is
 provided here is all that you would need to use subvalues.  See newSubValue() for
 further details.
*/


#ifndef __XSUBVALUEHANDLERS__
#define __XSUBVALUEHANDLERS__

#include "CMAPI.h" 

                                    CM_CFUNCTIONS

CMValue CM_FIXEDARGS newSubValue(CMObject object, CMProperty property, CMType type,
                                 CMValue baseValue, CMCount offset, CMSize size);
  /*
  This is a special case of CMNewValue() in that a new value entry of the specified type is
  created for the specified object with the specified property.  A refNum for this value is
  returned.  However, unlike CMNewValue(), the refNum is associated with a "subvalue" of
  some OTHER already existing value data (size characters starting at the offset). Whenever
  the returned refNum is used for a value operation, that operation will take place on the
  subvalue of the base value.
   
  A subvalue defines a portion of data already written for the baseValue.  This can be
  illustrated with the following diagram:
  
                                         x
         baseValue data: |--------------|------------|-----------|
         subvalue data:                 |------------|
                                         <---size--->
                             offset = x
                             
  In the above diagram, the |--------| represents a data value in ascending offsets in a
  container. The subvalue is just a subrange of (size) characters of the baseValue starting
  at offset x from the start of the baseValue.
 
  By definition, data (reads, writes, etc.) operations are permitted on subvalues so int 
  as they do not exceed the original definition of the subvalue.   In other words, if the
  subvalue represents size bytes at an offset withing some base value, then nothing can be
  done that will make the size grow larger (by definition).
  
  Note, subvalues of subvalues (of...) are permitted!  The origin of the second subvalue
  is with respect to the (sub)value on which it is based.  
  
  There is one restriction on this subvalue implementation. If the base value of a subvalue
  is MOVED, the results will be unpredictible and may (or may not) result in an error
  report. It won't blow.  But the implementation of subvalues is done in terms of the base
  value's object, property, type object references.  This is data actually written to the
  container for the subvalue.  If the base moves, this data is NOT updated.  A future 
  reference will thus get whatever represents that object, property, and type object
  reference.  It may or may not exist.  If it doesn't exist, an error report will be
  produced.  But, it won't be the proper base if the base is moved and there is something
  else in the original base position.
  */
  

#define SubValueGlobalName  (CMGlobalName)"User:SubValue" /* subvalue global name(type) */

#define RegisterSubValueType(container)   \
    (CMSetMetaHandler(CMGetSession(container), SubValueGlobalName, subValueDynamicValueMetahandler), \
     CMRegisterType(container, SubValueGlobalName))
  /*
  If subvalues are created in a container and then the executing session terminated, the
  subvalues can be used in a future session just like any other values by calling
  CMUseValue().  The only requirement is that you must register the subvalue base type
  prior to doing the CMUseValue().  To make this simpler, the above macro is defined. Just
  call RegisterSubValueType() to do the registration.
  
  Note RegisterSubValueType() is defined as an expression which can be assigned to a 
  CMType if you so desire.  This would be done to mix the subvalues type into some other
  combination of base types.
  
  A new value with a subvalue type requires a three values passed to CMNewValue(), i.e.,
  baseValue, offset, and size (in that order).  Those parameters are described in
  newSubValue() above.  This fact must be known when mixing subvalue types with other
  types to set the proper CMNewValue() "..." parameters.
  */
  
  
CMHandlerAddr CM_FIXEDARGS subValueDynamicValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType);
  /*
  This metahandler is called by the Container Manager for dynamic value creation to allow
  the Container Manager to get the addresses of the "metadata", "new value", and "use
  value" handlers.  These handlers are used as part of the process of creating dynamic
  values. See code for further documentation.
  
  Note, the Container Manager uses the address of this metahandler when it sees a type
  which has this metahandler registered for it.  Registering the metahandler and the type
  can be done with the RegisterSubValueType macro defined above.
  */
  
                                  CM_END_CFUNCTIONS

#endif
