/*---------------------------------------------------------------------------*
 |                                                                           |
 |                            <<< Handlers.c >>>                             |
 |                                                                           |
 |                    Container Manager Handlers Routines                    |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 11/18/91                                  |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1991-1992                 |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 The routines in this file maintain handler/type name associations.  Handlers are defined
 in terms of their type.  They may be retrieved by using that type.
*/


#include <stddef.h>
#include <string.h>
#include <stdio.h>

#ifndef __CMTYPES__
#include "CMTypes.h"
#endif
#ifndef __CM_API__
#include "CMAPI.h" 
#endif
#ifndef __SYMMGR__
#include "SymTbMgr.h"      
#endif
#ifndef __SESSIONDATA__
#include "Session.h"          
#endif
#ifndef __HANDLERS__
#include "Handlers.h"
#endif
#ifndef __UTILITYROUTINES__
#include "Utility.h"        
#endif

                                  CM_CFUNCTIONS

/* The following generates a segment directive for Mac only due to 32K Jump Table       */
/* Limitations.  If you don't know what I'm talking about don't worry about it.  The    */
/* default is not to generate the pragma.  Theoritically unknown pragmas in ANSI won't  */
/* choke compilers that don't recognize them.  Besides why would they be looked at if   */
/* it's being conditionally skipped over anyway?  I know, famous last words!            */

#if CM_MPW
#pragma segment Handlers
#endif


/*------------------------------------------------------------*
 | compare - type name comparison routine for cmEnterSymbol() |
 *------------------------------------------------------------*
 
 This routine is "sent" to cmEnterSymbol() to do the proper comparisons for handlers.  The
 handler tree is based on their type names.  cmEnterSymbol() is a generic binary tree
 routine that requires the comparsion to be supplied by its caller to decide on what
 basis the tree is build.  Hence this routine!
 
 Note, this "static" is intentionally left to default memory model under DOS since it is
 passed as a function pointer to cmEnterSymbol().
*/

static int compare(const void *h1, const void *h2)
{
  return (strcmp((char *)((MetaHandlerPtr)h1)->typeName,
                 (char *)((MetaHandlerPtr)h2)->typeName));
}

static int compare2(const void *h1, const void *h2)
{
  return (strcmp((char *)((MetaHandlerPtr)h1)->typeName,
                 (char *)h2));
}


/*--------------------------------------------------*
 | cmDefineMetaHandler - define a new "metahandler" |
 *--------------------------------------------------*
 
 Define a new metahandler with the specifed type (a C string). The function returns a
 pointer to the new handler or, if dup is true, a pointer to a previously defined entry.
 
 If NULL is returned then there was an allocation failure and the new type could not be
 created.
 
 Note, the global data session pointer created by CMStartSession() is passed since
 methandlers are global to all containers and thus the root of the metahandler symbol
 table is kept as part of the session data.
*/

MetaHandlerPtr cmDefineMetaHandler(CMMetaHandler metaHandler, 
                                   const unsigned char *typeName,
                                   Boolean *dup,
                                   SessionGlobalDataPtr sessionData)
{
  MetaHandlerPtr h, newMetaHandler;
  
  if ((newMetaHandler = (MetaHandlerPtr)SessionMalloc(NULL, sizeof(MetaHandler) + strlen((char *)typeName))) != NULL) {
    strcpy((char *)newMetaHandler->typeName, (char *)typeName); /* fill in new entry    */
    newMetaHandler->metaHandler = metaHandler;
    
    h = (MetaHandlerPtr)cmEnterSymbol(newMetaHandler, (void **)&SessionMetaHandlerTable, dup, compare);
    
    if (*dup) SessionFree(NULL, newMetaHandler);                      /* oops!                */
  } else {
    h = NULL;
    *dup = false;
  }
  
  return (h);                                                   /* return entry ptr     */
}


/*---------------------------------------------------------------*
 | cmLookupMetaHandler - find a previously defined "metahandler" |
 *---------------------------------------------------------------*
 
 Find a metahandler associated with the specified type.  If found, the HandlerPtr to the
 found entry is returned (which includes the handler proc and type).  If not found NULL
 is returned.
 
 Note, we allocate a temporary handler table entry here and then free it.   If the
 allocation fails, SessionSuccess, a session status switch, is returned false.  Otherwise
 SessionSuccess is true.
  
 The global data session pointer created by CMStartSession() is passed since methandlers
 are global to all containers and thus the root of the metahandler symbol table is kept
 as part of the session data.
*/

MetaHandlerPtr cmLookupMetaHandler(const unsigned char *typeName,
                                   SessionGlobalDataPtr sessionData)
{
  MetaHandlerPtr h;
  
  /* Because of the variable length nature of type names, we malloc() a temporary meta- */
  /* handler entry to be looked up.  This, of course, could caluse a allocation failure!*/
  
#if OLD_STYLE_SYMLOOKUP
  {
    MetaHandlerPtr tmpMetaHandler;
    
	if ((tmpMetaHandler = (MetaHandlerPtr)SessionMalloc(NULL, sizeof(MetaHandler) + strlen((char *)typeName))) != NULL) {
		strcpy((char *)tmpMetaHandler->typeName, (char *)typeName); /* fill in tmp entry    */
		h = (MetaHandlerPtr)cmLookupSymbol(tmpMetaHandler, SessionMetaHandlerTable, compare);
		SessionFree(NULL, tmpMetaHandler);                                /* remove tmp entry     */
		sessionData->success = true;                                /* indicate malloc is ok*/
	} else {
		h = NULL;                                                   /* bummer!              */
		sessionData->success = false;                               /* indicate bad malloc  */
	}
  }
#else
    h = (MetaHandlerPtr)cmLookupSymbol(typeName, SessionMetaHandlerTable, compare2);
    sessionData->success = true;                                /* indicate malloc is ok*/
#endif
  
  return (h);
}


/*-------------------------------------------------------------*
 | cmForEachMetaHandler - do some action on each "metahandler" |
 *-------------------------------------------------------------*
 
 Do (call) the specified action for each defined metahandler in the current session. The
 pointer to each metahandler entry is passed to the action routine along with a "refCon"
 which the caller can use as a communication facility to convey additional info to the
 action routine.
*/

void cmForEachMetaHandler(CMRefCon refCon, 
                          void (*action)(MetaHandlerPtr aHandler, CMRefCon refCon),
                          SessionGlobalDataPtr sessionData) 
{
  cmForEachSymbol(SessionMetaHandlerTable, refCon, (SymbolAction)action);
}


/*-------------------------------------------------*
 | cmFreeAllMetaHandlers - free all "metahandlers" |
 *-------------------------------------------------*
 
 This routine is called to remove the definitions of ALL previously defined metahandlers
 for the current session.
*/

void cmFreeAllMetaHandlers(SessionGlobalDataPtr sessionData)
{
  cmFreeAllSymbols(NULL, (void **)&SessionMetaHandlerTable, sessionData); /* just glue code   */
}
                              
                              CM_END_CFUNCTIONS
