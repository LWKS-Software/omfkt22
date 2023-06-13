/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                         <<<    xhandlrs.h     >>>                         |
 |                                                                           |
 |            Example Container Manager Basic Handlers Interfaces            |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 12/26/91                                  |
 |                             Jeffrey L. Bedell                             |
 |                                 8/28/95                                   |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1991-1992                 |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file represents the handler header file for the example handler file 
 ExampleHandlers.c.  That file contains complete documentation and an example on how to
 write a set of handlers and a metahandler for the Container Manager API that supports 
 containers in a stream file.
 
 The example is a complete working set of handlers for containers that are stored in
 stream files.  To make this a working example, and to hide its details from the "outside
 world", this header is provided.  As documented in    XHandlrs.c     there are a few
 additional routines that have nothing to do with the API handlers per-se.  Rather they
 allow for manipulation of some of the hidden information.
 
 See    XHandlrs.c     for complete documentation on these routines.
*/

#ifndef __EXAMPLEHANDLERS__
#define __EXAMPLEHANDLERS__

#include "CMAPI.h" 
#include "omTypes.h"

                                  CM_CFUNCTIONS


/*------------------------------*
 | Routines REQUIRED by the API |
 *------------------------------*/
  
CMHandlerAddr CM_FIXEDARGS containerMetahandler(CMType targetType, CMconst_CMGlobalName operationType);
  /*
  Metahandler proc for determining the addresses of the handler operation routines.
  Pass the address of this routine to a CMSetMetaHandler() call.
  */


/*------------------------------------------------------------------*
 | Auxiliary routines to make the example available for general use |
 *------------------------------------------------------------------*/

CMRefCon CM_FIXEDARGS createRefConForMyHandlers(CMSession sessionData,
                                                const char CM_PTR *pathname,
                                                GetUpdatingTargetType getTargetType,
                                                omfHdl_t	file);
  /*
  Create a reference constant (a "refCon") for container handler use.  Passed as the
  "attributes" to CMOpen[New]Container() and used as the "refCon" for all handler calls.
  
  The sessionData is the pointer to the current global session deta defined by
  CMStartSession().
  
  The pathname defines the name of the stream file to be used as a container.  
  
  The getTargetType defines a pointer to a user defined routine (must be a far pointer for
  DOS 80x86 users) to create a type that, when used by CMNewValue() or CMUseValue() will
  spawn a dynamic value for CMOpenNewContainer() to use when updating another container
  (i.e., the useFlags were kCMUpdateTarget).  The type must be associated with a dynamic
  value handler package that will access the target container to be updated.
  
  The prototype for the getTargetType function should be defined as follows:
  
        CMType CM_FIXEDARGS getTargetType(CMContainer theUpdatingContainer);
    
  The container in which the type is to be created is passed in theUpdatingContainer.
  
  See documentation for returnTargetType_Handler() in    XHandlrs.c     for further
  details.
  
  Note, the getTargetType may be NULL if kCMUpdateTarget useFlags is never passed to 
  CMOpenNewContainer() using this handler package.
  */

  
CMRefCon CM_FIXEDARGS setHandlersTrace(CMRefCon refCon, const char tracingState,
                                                    FILE CM_PTR *tracingFile);
  /*
  Enable (tracingState != 0) or disable (tracingState == 0) handler tracing to the
  tracingFile.  This routine returns the refCon as its result.
  */

                                CM_END_CFUNCTIONS

#endif
