/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                     <<<        xembeded.h         >>>                     |
 |                                                                           |
 |      Example Container Manager Embedded Container Handlers Interfaces     |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  1/8/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file represents the handler header file for the example embedded container handler
 file ExampleEmbeddedHandler.c.  That file contains complete documentation and an example
 on how to write a set of handlers and a metahandler for the Container Manager API that
 supports embedded containers.
 
 The example is a complete working set of handlers for embedded containers.  Such handlers
 do all "I/O" in terms of API calls for a value.  Thus this example is fully general to
 support all forms of embedded containers!
 
 To make this a working example, and to hide its details from the "outside world", this
 header is provided.  As documented in        XEmbeded.c         there are a few
 additional routines that have nothing to do with the API handlers per-se.  Rather they
 allow for manipulation of some of the hidden information.
 
 See        XEmbeded.c         for complete documentation on these routines.
*/

#ifndef __EXAMPLEEMBEDDEDHANDLERS__
#define __EXAMPLEEMBEDDEDHANDLERS__

#include "CMAPI.h" 

                                CM_CFUNCTIONS

/*------------------------------*
 | Routines REQUIRED by the API |
 *------------------------------*/
  
CMHandlerAddr CM_FIXEDARGS embeddedContainerMetahandler(CMType targetType, CMconst_CMGlobalName operationType);
  /*
  Metahandler proc for determining the addresses of the handler operation routines.  Pass
  the address of this routine to a CMSetMetaHandler() call.
  */


/*------------------------------------------------------------------*
 | Auxiliary routines to make the example available for general use |
 *------------------------------------------------------------------*/

CMRefCon CM_FIXEDARGS createRefConForMyEmbHandlers(const CMValue parentValue);
  /*
  Create a reference constant (a "refCon") for container handler use.  Passed as the
  "attributes" to CMOpen[New]Container() and used as the "refCon" for all handler calls.
  */
  
  
CMRefCon CM_FIXEDARGS setEmbeddedHandlersTrace(CMRefCon refCon, const char tracingState,
                                               FILE CM_PTR *tracingFile);
  /*
  Enable (tracingState != 0) or disable (tracingState == 0) handler tracing to the
  tracingFile.  This routine returns the refCon as its result.
  */

                              CM_END_CFUNCTIONS

#endif
