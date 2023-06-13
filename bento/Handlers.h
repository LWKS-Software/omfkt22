/*---------------------------------------------------------------------------*
 |                                                                           |
 |                            <<< handlers.h >>>                             |
 |                                                                           |
 |                   Container Manager Handlers Interfaces                   |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 11/18/91                                  |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1991-1992                 |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 Containers (files, memory, or whatever) are always access through handlers, to provide
 platform independence.  Handlers are responsible for doing all the I/O operations,
 including opening and closing, to containers.  They are also responsible for reading and
 writing the container label.
 
 The routines defined by this header are the API interfaces to allow the Container Manager
 to determine what handlers it should use for a particular container.  The Container
 Manager handler routines defined here maintain container type name/metahandler
 associations.
*/

#ifndef __HANDLERS__
#define __HANDLERS__

#include <stdio.h>

#ifndef __CMTYPES__
#include "CMTypes.h"
#endif
#ifndef __CM_API_TYPES__
#include "CMAPITyp.h"    
#endif
#ifndef __SYMMGR__
#include "SymTbMgr.h"      
#endif
#ifndef __SESSIONDATA__
#include "Session.h"          
#endif

struct SessionGlobalData;

                                  CM_CFUNCTIONS

/* The metahandler symbol table contains the associations between type names and meta-  */
/* handler routine addresses.                                                           */
 
struct MetaHandler {                    /* Layout for a metahandler symbol table entry: */
  SymbolLinks   theTreeLinks;           /*    standard right/left links (must be 1st)   */
  CMMetaHandler metaHandler;            /*    ptr to the metahandler                    */
  unsigned char typeName[1];            /*    start of type name for this handler       */
};
typedef struct MetaHandler MetaHandler, *MetaHandlerPtr;


/* The following defines the vector of handler routines which is built as part of every */
/* container control block. The vector is filled in when a container is opened by       */
/* calling a metahandler proc which returns the handler addresses.                      */

struct HandlerOps {                     /* Basic I/O operations and their prototypes    */
  CMRefCon (*cmfopen)(CMRefCon attributes, CMOpenMode mode);  
  void (*cmfclose)(CMRefCon refCon);
  int (*cmfflush)(CMRefCon refCon);
  int (*cmfseek)(CMRefCon refCon, CMCount posOff, CMSeekMode mode);
  CMSize (*cmftell)(CMRefCon refCon);
  CMSize32 (*cmfread)(CMRefCon refCon, CMPtr buffer,  CMSize32 elementSize, CMCount32 theCount);
  CMSize32 (*cmfwrite)(CMRefCon refCon, CMPtr buffer, CMSize32 elementSize, CMCount32 theCount);
  CMEofStatus (*cmfeof)(CMRefCon refCon);
  CMBoolean (*cmftrunc)(CMRefCon refCon, CMSize containerSize);
  CMSize (*cmgetContainerSize)(CMRefCon refCon);
  void (*cmreadLabel)(CMRefCon refCon, CMMagicBytes magicByteSequence, CMContainerFlags *flags,
                      CM_USHORT *bufSize, CM_USHORT *majorVersion, CM_USHORT *minorVersion,
                      CMCount *tocOffset, CMSize *tocSize);
  void (*cmwriteLabel)(CMRefCon refCon, CMMagicBytes magicByteSequence, CMContainerFlags flags,
                       CM_USHORT bufSize, CM_USHORT majorVersion, CM_USHORT minorVersion,
                       CMCount tocOffset, CMSize tocSize);
  CMValue (*cmreturnParentValue)(CMRefCon refCon);
  char *(*cmreturnContainerName)(CMRefCon refCon);
  CMType (*cmreturnTargetType)(CMRefCon refCon, CMContainer container);
  void (*cmextractData)(CMRefCon refCon, CMDataBuffer updateBuffer, CMSize32 size,
                        CMPrivateData data);
  void (*cmformatData)(CMRefCon refCon, CMDataBuffer updateBuffer, CMSize32 size,
                       CMPrivateData data);
};
typedef struct HandlerOps HandlerOps;


/* To make it easier and more readable for the Container Manager to call the handlers,  */
/* the following macros are defined.  The "refCon" is located in the container control  */
/* block.  Note the container is always an explicit parameter to these macros since in  */
/* some places we may be dealing with more than one container due to updating.          */

#define CMfopen(container, refCon, m)       (*((ContainerPtr)container)->handler.cmfopen)((void *)refCon, (CMOpenMode)(m))
#define CMfclose(container)                 (*((ContainerPtr)container)->handler.cmfclose)(((ContainerPtr)container)->refCon)
#define CMfflush(container)                 (*((ContainerPtr)container)->handler.cmfflush)(((ContainerPtr)container)->refCon)
#define CMfseek(container, p, m)            (*((ContainerPtr)container)->handler.cmfseek)(((ContainerPtr)container)->refCon, (p), (CMSeekMode)(m))
#define CMftell(container)                  (*((ContainerPtr)container)->handler.cmftell)(((ContainerPtr)container)->refCon)
#define CMfread(container, b, e, c)         (*((ContainerPtr)container)->handler.cmfread)(((ContainerPtr)container)->refCon, (CMPtr)(b), (e), (c))
#define CMfwrite(container, b, e, c)        (*((ContainerPtr)container)->handler.cmfwrite)(((ContainerPtr)container)->refCon, (CMPtr)(b), (e), (c))
#define CMfeof(container)                   (*((ContainerPtr)container)->handler.cmfeof)(((ContainerPtr)container)->refCon)
#define CMftrunc(container, s)              (*((ContainerPtr)container)->handler.cmftrunc)(((ContainerPtr)container)->refCon, (s))
#define CMgetContainerSize(container)       (*((ContainerPtr)container)->handler.cmgetContainerSize)(((ContainerPtr)container)->refCon)
#define CMreadLabel(container, m, f, n, v1, v2, t, l)  (*((ContainerPtr)container)->handler.cmreadLabel)(((ContainerPtr)container)->refCon, (CMMagicBytes)(m), (CMContainerFlags*)(f), (CM_USHORT*)(n), (CM_USHORT*)(v1), (CM_USHORT*)(v2), (t), (l))
#define CMwriteLabel(container, m, f, n, v1, v2, t, l) (*((ContainerPtr)container)->handler.cmwriteLabel)(((ContainerPtr)container)->refCon, (CMMagicBytes)(m), (CMContainerFlags)(f), (CM_USHORT)(n), (CM_USHORT)(v1), (CM_USHORT)(v2), (t), (l))
#define CMreturnParentValue(container)      (TOCValueHdrPtr)(*((ContainerPtr)container)->handler.cmreturnParentValue)(((ContainerPtr)container)->refCon)
#define CMreturnContainerName(container)    (*((ContainerPtr)container)->handler.cmreturnContainerName)(((ContainerPtr)container)->refCon)
#define CMreturnTargeType(container)        (*((ContainerPtr)container)->handler.cmreturnTargetType)(((ContainerPtr)container)->refCon, (CMContainer)container)
#define CMextractData(container, b, n, d)   (*((ContainerPtr)container)->handler.cmextractData)(((ContainerPtr)container)->refCon, (CMDataBuffer)(b), (CMSize32)(n), (CMPrivateData)(d))
#define CMformatData(container, b, n, d)    (*((ContainerPtr)container)->handler.cmformatData)(((ContainerPtr)container)->refCon, (CMDataBuffer)(b), (CMSize32)(n), (CMPrivateData)(d))

/* Sorry about the length of these lines.  The casts make them that way.  That is done  */
/* because some of the Container Manager routines that use these pass values coming in  */
/* from external (i.e., "CM...") calls.  The external types are generic and have to be  */
/* converted to the "real thing" (if you know what I mean).                             */

/* NOTE: Currently CMfeof() is NOT used by the container manager...                     */


/* Just like the container specific handlers, we have macros for calling the memory     */
/* management handlers.  These all use the session data pointer via the current         */
/* container whose pointer is assumed to be the variable name "container".              */

#define CMmalloc(ctr, size)            (*(SESSION->cmMalloc))(((CMContainer)ctr), (CMSize32)size)
#define CMfree(ctr, ptr)               (*(SESSION->cmFree))(((CMContainer)ctr), (CMPtr)ptr)
#define CMfreeCtr(ctr)                 (*(SESSION->cmFreeCtr))(((CMContainer)ctr))

CM_EXPORT MetaHandlerPtr cmDefineMetaHandler(CMMetaHandler metaHandler, 
                                             const unsigned char *typeName,
                                             Boolean *dup,
                                             struct SessionGlobalData *sessionData);
  /*
  Define a new metahandler with the specifed type (a C string). The function returns a
  pointer to the new handler or, if dup is true, a pointer to a previously defined entry.
  
  If NULL is returned then there was an allocation failure and the new type could not be
  created.
  
  Note, the global data session pointer created by CMStartSession() is passed since
  methandlers are global to all containers and thus the root of the metahandler symbol
  table is kept as part of the session data.
  */


CM_EXPORT MetaHandlerPtr cmLookupMetaHandler(const unsigned char *typeName,
                                             struct SessionGlobalData *sessionData);
  /*
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
  

CM_EXPORT void cmForEachMetaHandler(CMRefCon refCon, 
                                    void (*action)(MetaHandlerPtr aHandler, CMRefCon refCon),
                                    struct SessionGlobalData *sessionData); 
  /*
  Do (call) the specified action for each defined metahandler in the current session. The
  pointer to each metahandler entry is passed to the action routine along with a "refCon"
  which the caller can use as a communication facility to convey additional info to the
  action routine.
  */


CM_EXPORT void cmFreeAllMetaHandlers(struct SessionGlobalData *sessionData);
  /*
  This routine is called to remove the definitions of ALL previously defined metahandlers
  for the current session.
  */
  

                              CM_END_CFUNCTIONS
#endif
