/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                      <<<        xsession.h        >>>                     |
 |                                                                           |
 |           Example Container Manager Special Handlers Interfaces           |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  2/5/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file represents the handler header file for the example handler file 
 ExampleSessionHandlers.c.  That file contains complete documentation and an example on how
 to write a set of required session handlers and a metahandler for the Container Manager
 API.
 
 The only visible routine is the metahandler you need to pass to CMStartSession().  See
        XSession.c        for complete documentation.
*/


#ifndef __EXAMPLESESSIONHANDLERS__
#define __EXAMPLESESSIONHANDLERS__

#include "CMAPI.h" 

                                  CM_CFUNCTION
  
CMHandlerAddr CM_FIXEDARGS sessionRoutinesMetahandler(CMType targetType, CMconst_CMGlobalName operationType);
  /*
  Metahandler proc for determining the addresses of the session handler operation routines.
  Pass the address of this routine to a CMStartSession() call.
  */

                                CM_END_CFUNCTIONS

#endif
