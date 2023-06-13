/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                             <<< cmapi.h  >>>                              |
 |                                                                           |
 |                           Container Manager API                           |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 11/18/91                                  |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1991-1992                 |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This is the Container Manager API definitions.  All routines needed to communicate with
 the Container Manager are defined here.  This is the only explicit #include needed. All
 other required headers are included from here.
 
 Refer to Container Manager API documentation for details on these routines.


                        *------------------------------*
                        | NOTE TO DOS (80x86) USERS... |
                        *------------------------------*

 The Container Manager should be compiled with the "large" memory model so that all
 routines, pointers, etc. are "far".  All external API interfaces are appropriately
 qualified this way.  Since handlers are implementation or user dependent, these files
 must be compiled with the large memory model.  The only exception is local static
 routines which usually can be qualified with "near" (except, of course, for handlers).
 
 This file uses the function and pointer memory model attribute macros defined
 CM_API_Environment.h.  See that file for further details.
*/

#ifndef __CM_API__
#define __CM_API__

#include <stdarg.h>
#include <stdio.h>

#ifndef masterhd_h
#include "masterhd.h"
#endif

#ifndef __CM_API_ENV__
#include "CMAPIEnv.h"                         /* Environment-specific definitions       */
#endif
#ifndef __CM_API_TYPES__
#include "CMAPITyp.h"                         /* API type definitions                   */
#endif
#ifndef __CM_API_STDOBJIDS__
#include "CMAPIIDs.h"                         /* Standard object ID definitions, etc.   */
#endif
#ifndef __CM_API_ERRNO__
#include "CMAPIErr.h"                         /* Error code numbers and their meaning   */
#endif

#if sparc

#if PORT_NEEDS_MEMMOVE
#define memmove(a1, a2, l) bcopy(a1, a2, l)

extern char *sys_errlist[];
#define strerror(e) sys_errlist[e]
#endif

#endif

                                CM_CFUNCTIONS
                                
/*---------------------------*
 | Session (task) operations |
 *---------------------------*/

CM_EXPORT CMSession CM_FIXEDARGS CMStartSession(CMMetaHandler metaHandler, CMRefCon sessionRefCon);
CM_EXPORT void CM_FIXEDARGS CMEndSession(CMSession sessionData, CMBoolean closeOpenContainers);
CM_EXPORT void CM_FIXEDARGS CMAbortSession(CMSession sessionData);
CM_EXPORT CMRefCon CM_FIXEDARGS CMGetSessionRefCon(CMContainer container);
CM_EXPORT void CM_FIXEDARGS CMSetSessionRefCon(CMContainer container, CMRefCon refCon);


/*--------------------*
 | Handler Operations |
 *--------------------*/

CM_EXPORT CMHandlerAddr CM_FIXEDARGS CMSetMetaHandler(CMconst_CMSession sessionData,
                                                      CMconst_CMGlobalName typeName,
                                                      CMMetaHandler metaHandler);
CM_EXPORT CMHandlerAddr CM_FIXEDARGS CMGetMetaHandler(CMconst_CMSession sessionData,
                                                      CMconst_CMGlobalName typeName);
CM_EXPORT CMHandlerAddr CM_FIXEDARGS CMGetOperation(CMType targetType,
                                                    CMconst_CMGlobalName operationType);


/*----------------------*
 | Container Operations |
 *----------------------*/

CM_EXPORT CMContainer CM_FIXEDARGS CMOpenContainer(CMSession sessionData,
                                                   CMRefCon attributes,
                                                   CMconst_CMGlobalName typeName, 
                                                   CMContainerUseMode useFlags);
CM_EXPORT CMContainer CM_VARARGS CMOpenNewContainer(CMSession sessionData,
                                                    CMRefCon attributes,
                                                    CMconst_CMGlobalName typeName, 
                                                    CMContainerUseMode useFlags,
                                                    CMGeneration generation,
                                                    CMContainerFlags containerFlags, ...);
CM_EXPORT CMContainer CM_FIXEDARGS CMVOpenNewContainer(CMSession sessionData,
                                                       CMRefCon attributes,
                                                       CMconst_CMGlobalName typeName, 
                                                       CMContainerUseMode useFlags,
                                                       CMGeneration generation,
                                                       CMContainerFlags containerFlags,
                                                       va_list targetTypeInitParams);
CM_EXPORT void CM_FIXEDARGS CMCloseContainer(CMconst_CMContainer container);
CM_EXPORT void CM_FIXEDARGS CMAbortContainer(CMconst_CMContainer container);
CM_EXPORT void CM_FIXEDARGS CMGetContainerInfo(CMconst_CMContainer container,
                                               CMGeneration CM_PTR *generation, 
                                               CM_USHORT CM_PTR *bufSize,
                                               CMContainerFlags CM_PTR *containerFlags,
                                               CMGlobalName typeName,
                                               CMContainerModeFlags CM_PTR *openMode);
CM_EXPORT void CM_FIXEDARGS CMSetContainerVersion1(CMconst_CMContainer container);
CM_EXPORT CMSession CM_FIXEDARGS CMGetSession(CMContainer container);


/*------------------------------*
 | Type and Property Operations |
 *------------------------------*/

CM_EXPORT CMType CM_FIXEDARGS CMRegisterType(CMContainer targetContainer, CMconst_CMGlobalName name);
CM_EXPORT CMProperty CM_FIXEDARGS CMRegisterProperty(CMContainer targetContainer,
                                                     CMconst_CMGlobalName name);
CM_EXPORT CMCount32 CM_FIXEDARGS CMAddBaseType(CMType type, CMType baseType);
CM_EXPORT CMCount32 CM_FIXEDARGS CMRemoveBaseType(CMType type, CMType baseType);
CM_EXPORT CMBoolean CM_FIXEDARGS CMIsType(CMObject theObject);
CM_EXPORT CMBoolean CM_FIXEDARGS CMIsProperty(CMObject theObject);
CM_EXPORT CMType CM_FIXEDARGS CMGetNextType(CMContainer targetContainer, CMType currType);
CM_EXPORT CMProperty CM_FIXEDARGS CMGetNextProperty(CMContainer targetContainer,
                                                    CMProperty currProperty);


/*-------------------*
 | Object Operations |
 *-------------------*/

CM_EXPORT CMObject CM_FIXEDARGS CMNewObject(CMContainer targetContainer);
CM_EXPORT CMObject CM_FIXEDARGS CMGetNextObject(CMContainer targetContainer, CMObject currObject);
CM_EXPORT CMProperty CM_FIXEDARGS CMGetNextObjectProperty(CMObject theObject, CMProperty currProperty);
CM_EXPORT CMObject CM_FIXEDARGS CMGetNextObjectWithProperty(CMContainer targetContainer,
                                                            CMObject currObject, CMProperty property);
CM_EXPORT CMContainer CM_FIXEDARGS CMGetObjectContainer(CMObject theObject);
CM_EXPORT CMGlobalName CM_FIXEDARGS CMGetGlobalName(CMObject theObject);
CM_EXPORT CMRefCon CM_FIXEDARGS CMGetObjectRefCon(CMObject theObject);
CM_EXPORT void CM_FIXEDARGS CMSetObjectRefCon(CMObject theObject, CMRefCon refCon);
CM_EXPORT void CM_FIXEDARGS CMDeleteObject(CMObject theObject);
CM_EXPORT void CM_FIXEDARGS CMDeleteObjectProperty(CMObject theObject, CMProperty theProperty);
CM_EXPORT void CM_FIXEDARGS CMReleaseObject(CMObject theObject);
CM_EXPORT void CM_FIXEDARGS CMPurgeObjectVM(CMObject theObject);


/*------------------*
 | Value Operations |
 *------------------*/

CM_EXPORT CMCount32 CM_FIXEDARGS CMCountValues(CMObject object, CMProperty property, CMType type);
CM_EXPORT CMValue CM_FIXEDARGS CMUseValue(CMObject object, CMProperty property, CMType type);
CM_EXPORT CMValue CM_FIXEDARGS CMGetNextValue(CMObject object, CMProperty property, CMValue currValue);
CM_EXPORT CMValue CM_VARARGS CMNewValue(CMObject object, CMProperty property, CMType type, ...);
CM_EXPORT CMValue CM_FIXEDARGS CMVNewValue(CMObject object, CMProperty property, CMType type,
                                           va_list dataInitParams);
CM_EXPORT CMValue CM_FIXEDARGS CMGetBaseValue(CMValue value);
CM_EXPORT CMSize CM_FIXEDARGS CMGetValueSize(CMValue value);
CM_EXPORT CMSize32 CM_FIXEDARGS CMReadValueData(CMValue value, CMPtr buffer, CMCount offset, CMSize32 maxSize);
CM_EXPORT CMSize CM_FIXEDARGS CMGetValueDataOffset(CMValue value, CMCount offset, short *errVal);
CM_EXPORT void CM_FIXEDARGS CMWriteValueData(CMValue value, CMPtr buffer, CMCount offset, CMSize32 size);
CM_EXPORT void CM_FIXEDARGS CMDefineValueData(CMValue value, CMCount offset, CMSize size);
CM_EXPORT void CM_FIXEDARGS CMInsertValueData(CMValue value, CMPtr buffer, CMCount offset, CMSize32 size);
CM_EXPORT void CM_FIXEDARGS CMDeleteValueData(CMValue value, CMCount offset, CMSize size);
CM_EXPORT void CM_FIXEDARGS CMMoveValue(CMValue value, CMObject object, CMProperty property);
CM_EXPORT void CM_FIXEDARGS CMGetValueInfo(CMValue value, CMContainer CM_PTR *container,
                                           CMObject CM_PTR *object, CMProperty CM_PTR *property,
                                           CMType CM_PTR *type, CMGeneration CM_PTR *generation);
CM_EXPORT void CM_FIXEDARGS CMSetValueType(CMValue value, CMType type);
CM_EXPORT void CM_FIXEDARGS CMSetValueGeneration(CMValue value, CMGeneration generation);
CM_EXPORT CMContainer CM_FIXEDARGS CMGetValueContainer(CMValue value);
CM_EXPORT CMRefCon CM_FIXEDARGS CMGetValueRefCon(CMValue value);
CM_EXPORT void CM_FIXEDARGS CMSetValueRefCon(CMValue value, CMRefCon refCon);
CM_EXPORT void CM_FIXEDARGS CMDeleteValue(CMValue value);
CM_EXPORT void CM_FIXEDARGS CMReleaseValue(CMValue value);


/*----------------------*
 | Reference Operations |
 *----------------------*/

/* roger put this in for backward compatibility */
#define CMGetReferenceData(v, o, d) CMNewReference(v, (CMObject)o, d)

CM_EXPORT CMReference CM_PTR * CM_FIXEDARGS CMNewReference(CMValue value,
                                                           CMObject referencedObject,
                                                           CMReference CM_PTR theReferenceData);
CM_EXPORT CMReference CM_PTR * CM_FIXEDARGS CMSetReference(CMValue value,
                                                           CMObject referencedObject,
                                                           CMReference CM_PTR theReferenceData);
CM_EXPORT CMObject CM_FIXEDARGS CMGetReferencedObject(CMValue value, CMReference CM_PTR theReferenceData);
CM_EXPORT void CM_FIXEDARGS CMDeleteReference(CMValue value, CMReference CM_PTR theReferenceData);
CM_EXPORT CMCount CM_FIXEDARGS CMCountReferences(CMValue value);
CM_EXPORT CMReference CM_PTR * CM_FIXEDARGS CMGetNextReference(CMValue value,
                                                               CMReference CM_PTR currReferenceData);


/*--------------------------*
 | Value Handler Operations |
 *--------------------------*/

CM_EXPORT CMCount32 CM_VARARGS CMScanDataPacket(CMType type, CMMetaData metaData,
                                                CMDataPacket dataPacket, ...);
CM_EXPORT CMCount32 CM_FIXEDARGS CMVScanDataPacket(CMType type, CMMetaData metaData,
                                                   CMDataPacket dataPacket,
                                                   va_list dataInitParams);


/*--------------------------*
 | Error Handler Operations |
 *--------------------------*/

CM_EXPORT char CM_PTR * CM_VARARGS CMAddMsgInserts(char CM_PTR *msgString, CMSize32 maxLength, ...);
CM_EXPORT char CM_PTR * CM_FIXEDARGS CMVAddMsgInserts(char CM_PTR *msgString, CMSize32 maxLength,
                                                      va_list inserts);
CM_EXPORT CMErrorString CM_VARARGS omfCMGetErrorString(CMErrorString errorString, CMSize32 maxLength, 
                                                       CMErrorNbr errorNumber, ...);
CM_EXPORT CMErrorString CM_FIXEDARGS CMVGetErrorString(CMErrorString errorString, CMSize32 maxLength, 
                                                       CMErrorNbr errorNumber, va_list inserts);
CM_EXPORT char CM_PTR * CM_FIXEDARGS CMReturnContainerName(CMContainer container);


/*--------------------------------*
 | Special Environment Operations |
 *--------------------------------*/

CM_EXPORT void CM_PTR * CM_FIXEDARGS CMMalloc(CMContainer container, CMSize32 size, CMSession sessionData);
CM_EXPORT void CM_FIXEDARGS CMFree(CMContainer container, CMPtr ptr, CMSession sessionData);

CM_EXPORT void CM_VARARGS omfCMError(CMSession sessionData, CMErrorString message, ...);
CM_EXPORT void CM_FIXEDARGS CMVError(CMSession sessionData, CMErrorString message, va_list inserts);

                            CM_END_CFUNCTIONS

#endif
