/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                     <<<        THandlrs.c         >>>                     |
 |                                                                           |
 |        Container Manager Basic Target Container Handlers Interfaces       |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 5/28/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file contains an the interfaces for of basic container I/O handler's metahandler
 used by the Container Manager when doing updating operations on a target container.  The
 "I/O" defined by this methandler is in terms of Container Manager API calls on a dynamic
 OR real value.
 
 See        THandlrs.c         for complete details.
 
 Note, this header and its corresponding code are NOT to be viewed as example handlers.
 They are not internal to the Container Manager code for portability reasons.  That is 
 because these handlers, like all other handlers may have to be modified as a function of
 a particular hardware installation (for example, TOC and label I/O handlers for machines
 with different byte sizes).
*/

#ifndef __TARGETCONTAINERHANDLERS__
#define __TARGETCONTAINERHANDLERS__

#include "CMAPI.h" 

                                    CM_CFUNCTION
  
CMHandlerAddr CM_FIXEDARGS targetContainerMetahandler(CMType targetType, CMconst_CMGlobalName operationType);
  /*
  Metahandler proc for determining the addresses of the handler operation routines.  The
  API user is expected to register a predefined type, CMTargetHandlersTypeName (defined 
  in     cmapiids.h    ), with this methandler using CMSetMetaHandler().  The Container
  Manager uses this type to open the target container.  That, in turn, will use this
  metahandler due to the methandler/type association.  The methandler will thus return the
  addresses of the handler routines defined in TargetContainerHandlers.c. 
  */

                                  CM_END_CFUNCTION

#endif
