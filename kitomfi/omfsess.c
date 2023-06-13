/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                      <<<        XSession.c        >>>                     |
 |                                                                           |
 |                 Example Container Manager Session Handlers                |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  2/5/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

This file contains a fully documented example of the set of session
handlers and its metahandler as it would be used by the Container
Manager CMStartSession() routine.

When you call CMStartSession() you pass a pointer to a metahandler
that is used to get the addresses of the error reporting, memory
allocator and memory deallocator routines.  Possibly others might be
defined in the future.

This file contains an example of the session metahandler and the
associated handler routines.

Note, this is a working example.  It is used in conjunction with the
other handler examples in the files XHandlrs.c and
ExampleEmbeddedHandlers.c.  */


/*--------------------------------------------------------------------------*
 | DOS (80x86) USERS -- USE THE "LARGE" MEMORY MODEL WHEN COMPILED FOR 80X86
 | MACHINES |
 *--------------------------------------------------------------------------*

The Container Manager is built with this same option and assumes all
handler and metahandler interfaces are similarly built and can be
accessed with "far" pointers.
*/

/* JeffB: Start new optimizations
Memory allocator contains an array of size maxOptimized (100?) which contains pointers
to the heads of freelists for memory blocks of that size (the array index)

Free Operation
	if(blocksize <= maxOptimized)
		A free operation places a pointer to the current freelist (freeLists[blocksize])
		in the first 4 bytes of the memory block.  This implies that blocks of < 4 bytes
		must not be allowed, and accesses to this pointer must be done as bytes to avoid
		address errors.  freeLists[blockSize] is then adjusted to point to the memory block.
	else (blockSize > maxOptimized)
		free the block.

MallocOperation
	if(blockSize <= maxOptimized)
		if freeLists[blockSize] != NULL
			ptr = freeLists[blockSize]
			freeLists[blockSize] = *ptr (do operation as bytes)
		else
			for each optimized pool (pools contain heterogeneous blocksizes)
				if(the pool contains a large enough block)
					ptr = pool->nextFree;
					pool->nextFree += blockSize;
					break;
	else (blocksize > maxOptimized)
		malloc the block.
*/

#include "masterhd.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "masterhd.h"

#if PORT_SYS_MAC && !LW_MAC
#include "memory.h"
#endif

#include "CMAPI.h"
#include "XSession.h"
#include "GlbNames.h"
#include "Containr.h"

#include "omTypes.h"
#include "omPvt.h"
/* callback error signalling */


char            OriginalBentoErrorString[256];

#define DEBUG_OMFI_AME_MEMMGR		0
#if LW_MAC
#define OPTIMIZED_MEMORY_MECHANISM	0
#else
#define OPTIMIZED_MEMORY_MECHANISM	1
#endif

static void		*localMalloc(size_t size);
static void		localFree(void *mem);

#if OPTIMIZED_MEMORY_MECHANISM
/*--------------------------------------------------------------------------*
 |   Memory Manager - memory optimization                       |
 *--------------------------------------------------------------------------*
*/

static void    *newPoolMemory(CMContainer container, CMSize32 size);

#define POOL_DATA_SIZE	100000
#define MAX_OPTIMIZED	100				/* Maximum object size kept in fixed size allocator */

omfInt32	totalMallocs = 0, optMallocs = 0;

typedef struct memPool
{
	void           	*nextPool;
	int             currentIndex;
   unsigned int    poolMemAvailable;
   unsigned int   dummy; /* data needs to be on 8-byte boundary */
	unsigned char   data[POOL_DATA_SIZE];
} *memPoolPtr_t;

typedef struct
{
	char			*freeList[MAX_OPTIMIZED];	/* char because we can't guarantee byte alignment */
	memPoolPtr_t	pools;
} memInfo_t, *memInfoPtr_t;

typedef struct allocHdr
{
	omfInt32		size;
	char			*next;		/* char because we can't guarantee byte alignment */
} allocHdr_t;

#define MEM_ADDR_MOD  8

static void    *newPoolMemory(CMContainer container, CMSize32 size)
{
	void           	*poolAddr;
	memPoolPtr_t   	poolPtr;
	memInfoPtr_t	info;
  omfInt32 extra;

	if(size > MAX_OPTIMIZED)
		return (NULL);					/* Only fixed size allocs are in pools */

	if (container == NULL)
		return (NULL);

	/* Make sure that each size is rounded up to MEM_ADDR_MOD
	 * so that each allocation will start at a valid boundry.
   */
	extra = size % MEM_ADDR_MOD;
	if(extra != 0)
		size += MEM_ADDR_MOD - extra;

	info = (memInfoPtr_t)((ContainerPtr) container)->memAllocInfo;
	if ((info->pools == NULL) || (info->pools->poolMemAvailable < size))
    {
		poolPtr = (memPoolPtr_t)localMalloc(sizeof(struct memPool));
	    if(poolPtr == NULL)
	    	return(NULL);

	    poolAddr = poolPtr->data;
      	poolPtr->currentIndex = size;
      	poolPtr->poolMemAvailable = POOL_DATA_SIZE - size;
		poolPtr->nextPool = info->pools;
		info->pools = poolPtr;
	}
	else
    {
		poolPtr = info->pools;
		poolAddr = poolPtr->data + poolPtr->currentIndex;
		poolPtr->currentIndex += size;
		poolPtr->poolMemAvailable -= size;
    }

	return (poolAddr);
}

static void     freeContainer_Handler(CMContainer container)
{
	memPoolPtr_t	poolPtr, nextPoolPtr;
	memInfoPtr_t	info;

#if 0
	printf("Total mallocs %ld, optimized mallocs %ld\n", totalMallocs, optMallocs);
#if PORT_SYS_MAC && !LW_MAC
	printf("Free Memory (before container free) %ld\n", FreeMem());
#endif
#endif

	if (!container)
		return;
	info = (memInfoPtr_t)((ContainerPtr) container)->memAllocInfo;
	if (info == NULL)
		return;

	poolPtr = info->pools;

	while (poolPtr != NULL)
	{
		nextPoolPtr = (memPoolPtr_t)poolPtr->nextPool;	/* advance to next
							   								buffer before
							   								disposal	*/

		localFree(poolPtr);
		poolPtr = nextPoolPtr;
	}

	localFree(info);
	((ContainerPtr) container)->memAllocInfo = NULL;
}
#else
static void     freeContainer_Handler(CMContainer container)
{
}
#endif


/*---------------------------------------------------------------------------*
 | sessionRoutinesMetahandler - metahandler to return the session handler
 | addresses |
 *---------------------------------------------------------------------------*

As mentioned above, you pass the address of a metahandler that will
return the address of each session routine required by the Container
Manager. The address of the metahandler is passed to
CMStartSession().  It, in turn calls the specified routine to get
the address for each session routine.  The operationType parameter
defines which routine is requested.

For each call a unique operation type string is passed to indicate
which routine address is to be returned.  There are 3 session handlers
needed by the API.  They should have the following prototypes:
*/

CM_CFUNCTIONS

static void     error_Handler(struct SessionGlobalData *sess,
			      struct Container *cnt,
			      CMErrorNbr errorNumber,...);

static void    *alloc_Handler(CMContainer container, CMSize32 size);
static void     free_Handler(CMContainer container, CMPtr ptr);
//static void     freeContainer_Handler(CMContainer container);

CM_END_CFUNCTIONS
/*
 * See comments in      CMAPIEnv.h      about CM_CFUNCTIONS/CM_END_CFUNCTIONS
 * (used only when compiling under C++).
 *
 * The documentation for these routines is specified below as each routine is
 * defined so we will not repeat it here.  Note, that we actually declare all
 * these routines as "static" since they only need to be visible in this
 * file.  The API gets at them through the addresses we return from here.
 *
 * In the code below, you will see a list of macro calls for the operationType
 * strings. These are defined in     CMAPIIDs.h     which is included by
 * CM_API.h.
 *
 * The API generalizes the use of metahandlers to allow specific metahandlers
 * for specific type objects within a container.  This is the targetType
 * argument.  All metahandlers have the same prototype, but when used by
 * CMStartSession(), the targetType will always be NULL.
 */

CMHandlerAddr CM_FIXEDARGS sessionRoutinesMetahandler(CMType targetType,
				      CMconst_CMGlobalName operationType)
{
  static char    *operationTypes[] = {CMErrorOpType,	/* 0 *//* Operation
							 * Types  */
					CMAllocOpType,	/* 1 */
					CMFreeOpType,	/* 2 */
					CMFreeCtrOpType,/* 3 */
					NULL};
  char          **t;
  CMType          ignored = targetType;

  /* Look up the operation type in the operationTypes table above...       */

  t = operationTypes;
  while (*t)
	{
    if (strcmp((char *) operationType, *t) == 0)
      break;
	t++;
	}

  /*
   * Now that we got it (hopefully), return the appropriate routine
   * address...
   */

  switch (t - operationTypes)
    {
    case 0:
      return ((CMHandlerAddr) error_Handler);	/* CMErrorOpType        */
    case 1:
      return ((CMHandlerAddr) alloc_Handler);	/* CMAllocOpType        */
    case 2:
      return ((CMHandlerAddr) free_Handler);	/* CMFreeOpType         */
    case 3:
      return ((CMHandlerAddr) freeContainer_Handler);	/* CMFreeCtrOpType      */
    default:
      return (NULL);	/* huh?                 */
    }
}


/*-----------------------------------------*
 | error_Handler - error reporting handler |
 *-----------------------------------------*

The error reporting handler is a required session routine whose
address is asked for by CMStartSession().  All Container Manager
errors are reported through here.

The Container Manager API makes available some of the same routines
used internally.  Specifically, the ability to take an string that can
contain inserts and "edit in" those inserts (CM[V]AddMsgInserts()).

CMAPIErr.h defines the meaning for each error number in the comments
along side the number's definition.  A typical definition looks like
the following:
*/

#define CM_err_WhatEver 1	/* Example error msg with inset ^0 and ^1         */
/*
 * The ^0, ^1, etc. represent places for insertion strings that are also
 * passed to the error handler in the "..." portion of the parameter list.  A
 * ^0 corresponds to the first insert, ^1 the second, and so on.  The inserts
 * can appear in any order and more than once.
 *
 * This is an example of an error handler that simply prints the the meaning of
 * the error number with the inserts placed in the indicated spots according
 * to the meanings defined in CM_API_Errno.h.
 *
 * The Container Manager API provides routines to allow an error handler to
 * convert the error number and its associated insert to the messages shown
 * in the comments. In this example error hander we use one of those
 * routines, CMVGetErrorString() to do all the necessary work.  There is no
 * explicit need to define the message strings and place the inserts.  The
 * API provides all the routines necessary to do this work for you!
 */

static void error_Handler(struct SessionGlobalData *bentoSess,
			  struct Container *cnt,
			  CMErrorNbr errorNumber,...)
{
	va_list         inserts;
	char            errorString[256], *str = NULL;
	omfSessionHdl_t	sess;
	omfHdl_t		tst;
	omfBool			found;

	if(bentoSess != NULL)
	{
		sess = (omfSessionHdl_t)bentoSess->refCon;
		sess->BentoErrorNumber = errorNumber;
		sess->BentoErrorRaised = 1;
		str = sess->BentoErrorString;
	}
	else if(cnt != NULL)
	{
		sess = (omfSessionHdl_t)cnt->sessionData->refCon;

		tst = sess->topFile;
		found = FALSE;
		while (tst != NULL)
		{
			/* Look for the obj reference here */
			if (cnt == (ContainerPtr)tst->container)
			{
				tst->BentoErrorNumber = errorNumber;
				tst->BentoErrorRaised = 1;
				found = TRUE;
				str = tst->BentoErrorString;
				break;
			}

			tst = tst->prevFile;
		}

		if((!found) && (sess != NULL))
		{
			sess->BentoErrorNumber = errorNumber;
			sess->BentoErrorRaised = 1;
			str = sess->BentoErrorString;
		}
	}
	if(str != NULL)
	{
		va_start(inserts, errorNumber);
		sprintf(str, "%s", CMVGetErrorString(errorString, 256,
						       errorNumber, inserts));
		va_end(inserts);
	}
	return;

}


/*---------------------------------*
 | alloc_Handler - allocate memory |
 *---------------------------------*

The Container Manager API requires some form of memory management that
can allocate memory and return pointers to it.  By generalizing this
as a handler you are free to choose a memory management mechanism
appropriate to your environment.

For the purposes of this example, we map the handler directly onto the
C runtime malloc().  If you are running in a standard C runtime
environment, this may prove sufficient.
*/

static void    *alloc_Handler(CMContainer container, CMSize32 size)
{
#if OPTIMIZED_MEMORY_MECHANISM
	void       		*result = NULL;
	memInfoPtr_t	info;
	allocHdr_t		hdr;
	omfInt32		n;

	size += sizeof(allocHdr_t);
	totalMallocs++;
	if ((container != NULL) && (size < MAX_OPTIMIZED))
	{
		info = (memInfoPtr_t)((ContainerPtr) container)->memAllocInfo;
		if(info == NULL)
		{
			info = (memInfoPtr_t)localMalloc(sizeof(memInfo_t));
			if(info == NULL)
				return(NULL);

			for(n = 0; n < MAX_OPTIMIZED; n++)
				info->freeList[n] = NULL;
			info->pools = NULL;
			((ContainerPtr) container)->memAllocInfo = info;
		}

		optMallocs++;
		if(info->freeList[size] != NULL)
		{
			/* Unlink from the free list */
			memcpy(&hdr, info->freeList[size], sizeof(allocHdr_t));
			result = info->freeList[size];
			info->freeList[size] = hdr.next;
		}
		else
		{
			result = newPoolMemory(container, size);
			if(result == NULL)
				return(NULL);
			hdr.size = size;
			hdr.next = NULL;
			memcpy(result, &hdr, sizeof(allocHdr_t));
		}
		return ((char *)result + sizeof(allocHdr_t));
	} else
	{
		result = localMalloc(size);
		if(result == NULL)
			return(NULL);

		hdr.size = size;
		hdr.next = NULL;
		memcpy(result, &hdr, sizeof(allocHdr_t));
		return((char *)result + sizeof(allocHdr_t));
	}

#else
	return (localMalloc(size));

#endif

}

/*----------------------------------*
 | free_Handler - deallocate memory |
 *----------------------------------*

The Container Manager API calls this handler when it wants to free up
memory it no longer needs.  The memory attempting to be freed will,
of course, be memory previously allocated by the memory allocator
handler.

There is nothing requiring you to free the memory.  If you want to
chew it all up, be my guest!  But for this example, we'll be good guys
and use the standard C free() since we allocated with malloc() above.
*/

static void     free_Handler(CMContainer container, CMPtr ptr)
{

#if OPTIMIZED_MEMORY_MECHANISM
	allocHdr_t		hdr;
	memInfoPtr_t	info;
	memPoolPtr_t tstPool;
	char			*hdrPtr;
	omfBool found;
	unsigned char *tstPtr = (unsigned char *)ptr;

	hdrPtr = ((char *)ptr) - sizeof(allocHdr_t);
	if (container != NULL)
	{
		info = (memInfoPtr_t)((ContainerPtr) container)->memAllocInfo;

		/* Store the old freelist pointer in the freed memory block (this can't be
		 * < sizeof(void *), alloc guarantees this.
		 * This code links the nw block onto the head of the free list
		 * for the particular block size
		 */
		memcpy(&hdr, hdrPtr, sizeof(allocHdr_t));
		if((info != NULL) && hdr.size < MAX_OPTIMIZED)
 		{
 			for(tstPool = info->pools; tstPool != NULL; tstPool = (memPoolPtr_t)tstPool->nextPool)
				{
					if((tstPtr >= tstPool->data) && (tstPtr < tstPool->data+POOL_DATA_SIZE))
						{
							hdr.next = info->freeList[hdr.size];
							memcpy(hdrPtr, &hdr, sizeof(allocHdr_t));
							info->freeList[hdr.size] = hdrPtr;
							break;
						}
				}
 		}
		else
		{
 			found = FALSE;
			for(tstPool = info->pools; (tstPool != NULL) && !found; tstPool = (memPoolPtr_t)tstPool->nextPool)
				{
					found = ((tstPtr >= tstPool->data) && (tstPtr < tstPool->data+POOL_DATA_SIZE));
				}
	      if(!found)
					localFree(hdrPtr);
	    }
	}
	else
	{
      localFree(hdrPtr);
    }
#else
  localFree(ptr);
#endif
}

static void		*localMalloc(size_t size)
{
#if PORT_SYS_MAC && !LW_MAC
/* ... don't bothere with FreeMem() here.
//     NewPtr() reports 0 if it can't allocate the requested memory.
//	if (FreeMem() < size)
//		return (NULL);
*/
	return(NewPtr(size));
#else
	return(malloc(size));
#endif
}

static void		localFree(void *mem)
{
#if PORT_SYS_MAC && !LW_MAC
	DisposePtr((char *)mem);
#else
	free(mem);
#endif
}
/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:2 ***
;;; End: ***
*/
