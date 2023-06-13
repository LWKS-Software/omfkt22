/*---------------------------------------------------------------------------*
 |                                                                           |
 |                              <<< TOCIO.c >>>                              |
 |                                                                           |
 |                     Container Manager TOC I/O Routines                    |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 10/01/92                                  |
 |                                                                           |
 |                     Copyright Apple Computer, Inc. 1992                   |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file contains all the routines responsible for reading and writing the container
 TOC.  The routines are divided into two main groups as follows:

 (1). "Raw" low-level TOC I/O: all the routines to read and write TOC basic value segment
      entries.  These routines format the TOC with the syntatic layout described in the
      header (.h) file.

      cmStartTOCIO            - initiate TOC I/O
      cmEndTOCIO              - end TOC I/O
      cmFtellTOC              - get current container TOC I/O "seek" position
      cmFlushTOCOutputBuffer  - write current TOC output buffer
      cmWrite1TOCSegment      - write one value segment
        writeTOCEntry         - write one TOC control code and its data
      cmRead1TOCSegment       - read one value segment
        readTOCEntry          - read one TOC control code and its data

 (2). High-level TOC I/O control that determines how to generate or process TOC entries.
      This calles the group (1) routines.

      cmBuildGlobalNameTable  - add a global name to the global name table
      cmReadTOC               - read (laod) in a container TOC
      cmWriteTOC              - write a TOC to the container
        writeTOCValue         - add value data info to segment and write the segment
        writeTOCValueHdr      - add value header info to segment
        writeTOCProperty      - add property info to segment
        writeTOCObject        - add object info to segment
      cmDoBackPatches         - back patch certain TOC entries
*/

#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#include "omCvt.h"

#ifndef __CMTYPES__
#include "CMTypes.h"
#endif
#ifndef __CM_API__
#include "CMAPI.h"
#endif
#ifndef __LISTMGR__
#include "ListMgr.h"
#endif
#ifndef __TOCENTRIES__
#include "TOCEnts.h"
#endif
#ifndef __TOCOBJECTS__
#include "TOCObjs.h"
#endif
#ifndef __TOCIO__
#include "TOCIO.h"
#endif
#ifndef __OLDTOCENTRIES__
#include "oldtoc.h"
#endif
#ifndef __GLOBALNAMES__
#include "GlbNames.h"
#endif
#ifndef __CONTAINEROPS__
#include "Containr.h"
#endif
#ifndef __HANDLERS__
#include "Handlers.h"
#endif
#ifndef __UPDATING__
#include "Update.h"
#endif
#ifndef __DYNAMICVALUES__
#include "DynValus.h"
#endif
#ifndef __BUFFEREDIO__
#include "BufferIO.h"
#endif
#ifndef __FREESPACE__
#include "FreeSpce.h"
#endif
#ifndef __REFERENCES__
#include "Refs.h"
#endif
#ifndef __SESSIONDATA__
#include "Session.h"
#endif
#ifndef __ERRORRPT__
#include "ErrorRpt.h"
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
#pragma segment TOCEntries
#endif

static Boolean ObjectDefCleanup(ContainerPtr container, TOCObjectPtr theObject,
                                                  TOCentry tocEntry, unsigned int  *undefObjectsPtr,
                                                  unsigned int  refsDataObjectID, TOCValueHdrPtr theValueHdr);

                              /*-----------------------*
                               | TOC I/O Control Block |
                               *-----------------------*/

/* This is the TOC I/O control block which is passed around to the various TOC I/O      */
/* routines.  It contains the buffers, associated pointers, etc.  For callers outside   */
/* this file, they con only "see" this block through an anonymous "void *" pointer.     */

struct TOCIOControl {                     /* Layout of a TOC I/O control block:         */
  ContainerPtr    container;              /*    container control block ptr             */
  unsigned int    tocBufSize;             /*    max size of the TOC I/O buffer          */
  union {                                 /*    setjmp environment for buffered I/O     */
    jmp_buf       jmpBuf;                 /*          *** PORTABILITY PROBLEM ***       */
    char          firstByte;              /*       Don't assume the layout of a jmp_buf */
  } tocIOEnv;                             /*       firstByte used to memcpy to  jmp_buf */
  Boolean         reading;                /*    true ==> reading TOC; false ==> writing */
  TOCCodeArray    entrySize;              /*    entry size corresponding to each code   */
  unsigned int    totalRead;              /*    total number of bytes read so far       */
  CMCount					ioPos;                  /*    current container I/O position          */
  CMCount				  currBufrPos;            /*    current buffer container position       */
  unsigned int    currBufSize;            /*    buffer size of current read buffer      */
  CMSize			   tocSize;                /*    total (current write) size of the TOC   */
  unsigned int    nextByte;               /*    0-rel. next byte to use in tocBuffer    */
  TOCentry        prevSegment;            /*    previous TOC segment entry              */
  #if CMDUMPTOC
  CMCount			   currTOCoffset;          /*    offset to most recent TOC entry         */
  #endif
  unsigned char   tocBuffer[1];           /*    the START of the TOC I/O buffer         */
};
typedef struct TOCIOControl TOCIOControl, *TOCIOControlPtr;

/* CAUTION: I/O error "recovery" here is done by using a longjmp on a setjmp environment*/
/*          passed to startTOCIO() below. In order to save that environment, a memcpy() */
/*          is done to copy the startTOCIO() parameter to the above struct.  No         */
/*          assumptions are made about the layout of a setjmp's jmp_buf!  For example,  */
/*          it is never assumed to be an array.  Therefore, in order to PORTABLY pass   */
/*          the jmp_buf address to a memcpy(), a union is done to the first byte of the */
/*          jmp_buf to address it.  To some "picky" compilers (in particular, the       */
/*          Zortech MPW C/C++ compiler), addressing the first byte of a jmp_buf which IS*/
/*          an array as "&foo" is treated as an error (it expects "&foo[0]" -- I always */
/*          thought this was benign).  To avoid such problems, the above "kludge" is    */
/*          used.                                                                       */


                           /*--------------------------------*
                            | TOC Writing Communication Area |
                            *--------------------------------*/

/* The following is used in this file by cmWriteTOC(), etc. to common info needed to be */
/* maintained while walking the TOC as it is being written.  The pointer to the callers */
/* back patch records is also in here so that those records may eventually be passed to */
/* cmDoBackPatches().                                                                   */

struct WriteTOCCommArea {       /* cmWriteTOC() TOC walking communication area layout:  */
  TOCentry       tocEntry;      /*    the TOC entry buffer to be created and written    */
  TOCObjectPtr   refDataObject; /*    NULL or ptr to references recording object        */
  NewBackPatchesPtr thePatches;    /*    ptr to the TOC entries saved for back-patching    */
};
typedef struct WriteTOCCommArea WriteTOCCommArea, *WriteTOCCommAreaPtr;


/*-------------------------------------------------------*
 | cmStartTOCIO - prepare for reading or writing the TOC |
 *-------------------------------------------------------*

 This must be called prior to any reading or writing of the TOC.  The basic standard
 protocol to be followed is as follows:

  if (setjmp(env)) {
    <caller's recovery and exit>
  }

  CMseek(to proper TOC position in the container);

  t = cmStartTOCIO(container, container->tocBufSize, (jmp_buf *)&env, tocOffset, tocSize);

             when reading                  or          when writing
             ------------                              ------------

      while (cmRead1TOCSegment(t, ...)) {   |   some kind loop through the TOC
        cmRead1TOCSegment(t, ...);          |   {
        <process TOC entry>                 |      <set up TOC entry>
      }                                     |      cmWrite1TOCSegment(t, ...);
                                            |   }
                                            |   cmFlushTOCOutputBuffer(t, true);
  cmEndTOCIO(t);

 The TOC to be read or written is assumed to start at the specified tocOffset. It is the
 caller's responsibility to seek to this position.  For reading a TOC, tocSize defines the
 EXACT amount of TOC to be read.  For writing, tocSize must be passed as 0. The final size
 will be returned when endTOCIO() is called to terminate TOC creation (alternatively,
 cmFlushTOCOutputBuffer() will return the final size).

 cmStartTOCIO() allocates an I/O buffer and its associated control information. The buffer
 is used to format or extract TOC entries.  The TOC I/O control block pointer allocated is
 returned as the function result as an anonymous "void *" pointer.  This is passed to all
 the other low level TOC externalized buffering routines (cmWrite1TOCSegment(),
 cmread1TOCSegment(), cmFlushTOCOutputBuffer(), cmFTellTOC(), and endTOCIO()).  An error
 is reported for allocation errors.  If the error reporter returns, NULL is returned as
 the function result.

 The maximum size of the buffer to be allocated is passed.  This will generally be
 determined from the container label, but that is not assumed here.  The size is, however,
 always ensured to be rounded up to a multiple of 4.

 A set setjmp/longjmp environment variable is also passed for read and write error
 reporting and recovery.  The setjmp/longjmp is used rather than have to check for errors
 on each I/O call of the buffered I/O routines.  If the longjmp is taken, the caller can
 assume that the appropriate error has been reported and the buffer and its control block
 freed.

 Note, in the case of updating, the private TOC and the non-private (updating) TOC are
 viewed as two INDEPENDENT TOCs.  There is no need to worry that the non-private (i.e.,
 second TOC) does not start on a buffer boundary with respect to the private (first) TOC.
 Each is read or written independently and viewed as distinct from one another.  They just
 happen to be adjacent to one another in the container.  The tocBufSize and tocOffset
 must be appropriately passed for each TOC.

 Also note, that the reason some of the low level buffering routines have been made
 external (cmWrite1TOCSegment(), cmread1TOCSegment(), cmFlushTOCOutputBuffer(),
 cmFTellTOC(), and endTOCIO()) is to mainly allow for the debugging trace routines to read
 the TOC.  cmWrite1TOCSegment() and cmFlushTOCOutputBuffer() are external only for
 symmetry, but it should have no external callers (i.e., they're all in this file).
 Making these routines externally visable is also why the I/O control block pointer is
 returned as an anonymous "void *".  We may have to make the routines visable outside of
 here, but no one outside has any business knowing what's in the control block!
*/

void *cmStartTOCIO(ContainerPtr container, unsigned int  tocBufSize, jmp_buf *ioEnv,
                   CMCount tocOffset, CMSize tocSize)
{
  TOCIOControlPtr t;

  /* Allocate the control block with the buffer as the last field of the block...       */

  tocBufSize = 4 * ((tocBufSize + 3) / 4);                /* ensure a multiple of 4     */

  if ((t = (TOCIOControlPtr)CMmalloc(container, sizeof(TOCIOControl) + tocBufSize)) == NULL) {
    ERROR1(container,CM_err_NoTOCBuffer, CONTAINERNAME);
    return (NULL);
  }

  t->container     = container;                           /* remember the container     */
  t->tocBufSize    = tocBufSize;                          /* remember buffer size       */
  memcpy(&t->tocIOEnv.firstByte, ioEnv, sizeof(jmp_buf)); /* remember setjmp env.       */

  InitCodeSizes(t->entrySize);                            /* init TOC entry size array  */

  t->nextByte    = 0;                                     /* next read/write byte ptr   */
  t->ioPos       = tocOffset;                             /* starting container offset  */
  t->currBufrPos = tocOffset;                             /* 1st buffer always the same */
  t->tocSize     = tocSize;                               /* total amount (0 for write) */
  t->currBufSize = 0;                                     /* 1st read buffer is empty   */
  t->totalRead   = 0;                                     /* because nothing read yet   */

  t->prevSegment.objectID   = 0x00000000UL;               /* init to no previous object */
  t->prevSegment.propertyID = 0x00000000UL;               /* init to no previous prop.  */
  t->prevSegment.typeID     = 0x00000000UL;               /* init to no previous type   */
  t->prevSegment.generation = 0;                          /* init to no previous gen.   */

  return ((void *)t);                                     /* pass back ctl block ptr    */
}


/*---------------------------------------------------*
 | cmEndTOCIO - terminate reading or writing the TOC |
 *---------------------------------------------------*

 After all I/O to a TOC is completed, this routine must be called to free the TOC I/O
 buffer and control block (whose pointer is passed as the t parameter).  If the TOC was
 created (written), then it is assumed the last buffer was already flushed by calling
 flushTOCOutputBuffer() with its finalFlush parameter set to true.

 The function returns the size of the TOC.  For TOC creation (writing), this will be the
 size of the TOC now written to the container.  For reading, the returned value is
 identical to the size passed to cmStartTOCIO() (i.e., the returned value is not looked at
 for reading).

 Note, this routine must NOT be called if an error was reported and the longjmp taken
 through the setjmp environment buffer passed to cmStartTOCIO()>  Part of standard error
 recovery for TOC I/O errors is effectively do a cmEndTOCIO() and then report the error.
 If the error reporter returns, the longjmp is taken.
*/

CMSize cmEndTOCIO(void *tocIOCtl)
{
  TOCIOControlPtr t = (TOCIOControlPtr)tocIOCtl;  /* convert the anonymous I/O block ptr*/
  ContainerPtr    container = t->container;       /* needed for CMfree()                */
  CMSize   tocSize = t->tocSize;

  CMfree(container, t);                                      /* free buffer and control block      */

  return (tocSize);                               /* return total size of the TOC       */
}


/*--------------------------------------------------------*
 | cmFtellTOC - return current TOC container I/O position |
 *--------------------------------------------------------*

 This returns the current container I/O position.  The position is updated as the TOC I/O
 buffer fills and is written or reloaded when reading.

 This routine is needed when only when there is the possibility that other code might be
 doing "seeks" to the same container behind the TOC I/O routine's back!  Code doing such
 seeks must be able to reseek the container position according to the value returned here.
*/

CMCount cmFtellTOC(void *tocIOCtl)
{
  return (((TOCIOControlPtr)tocIOCtl)->ioPos);
}


/*------------------------------------------------------*
 | cmFlushTOCOutputBuffer - flush the TOC output buffer |
 *------------------------------------------------------*

 This must be called to make sure that the TOC output buffer (allocated by cmStartTOCIO()
 and associated with tocIOCtl) is flushed, i.e., fully written.  This buffer will be the
 remaining partial buffer when finalFlush is true, and a full buffer otherwise.  The
 buffer is padded if necessary up to the full buffer size (defined by cmStartTOCIO()) or
 to the nearest multiple of 4 when finalFlush is true.  finalFlush would, of course, only
 be true prior to calling cmEndTOCIO() to write the LAST buffer out as a "short" buffer.

 Padding consists of a EndOfBufr byte followed by 0 or more NOP bytes.  Padding is not
 necessary, of course, if the buffer is exactly full.

 The function returns the current total size of the TOC including the flushed buffer. This
 generally has no meaning until finalFlush is true, at which time the returned amount will
 be the total size of the TOC written to the container.

 Note, internally, this routine is also called from writeTOCEntry() when the buffer fills,
 i.e., the next TOC entry cannot fully fit into the buffer.  Spanning across buffer
 boundaries is not allowed for TOC entries.
*/

CMSize cmFlushTOCOutputBuffer(void *tocIOCtl, Boolean finalFlush)
{
  TOCIOControlPtr t = (TOCIOControlPtr)tocIOCtl;
  ContainerPtr    container = t->container;
  unsigned int    bufSize, padAmount;
  unsigned char   *p, entryCode;
  jmp_buf         jmpBuf;

  /* Determine the buffer size to use: a full buffer if finalFlush is false, and the    */
  /* next multiple of 4 if this is the last buffer.                                     */

  bufSize = finalFlush ? 4 * ((t->nextByte + 3) / 4) : t->tocBufSize;

  /* Pad the remaining bytes in the buffer (if any) with a EndOfBufr and 0 or more NOPs.*/

  padAmount = bufSize - t->nextByte;                  /* number of pad bytes needed     */

  if (padAmount > 0) {                                /* if padding needed...           */
    p = t->tocBuffer + t->nextByte;                   /* ...point at where to start pad */
    entryCode = EndOfBufr;                            /* ...put in the EndOfBufr code   */
    CMformatData(container, p, 1, &entryCode);
    if (padAmount > 1) {                              /* ...if more pads still needed...*/
      entryCode = NOP;                                /* ...pad out with the NOPs       */
      for (--padAmount; padAmount > 0; --padAmount)
        CMformatData(container, ++p, 1, &entryCode);
    }
  } /* padding */

  /* Write the buffer to the container...                                               */

  if (CMfwrite(container, t->tocBuffer, sizeof(unsigned char), bufSize) != bufSize) {
    memcpy(&jmpBuf, &t->tocIOEnv.firstByte, sizeof(jmp_buf));
    CMfree(container, t);
    ERROR1(container,CM_err_BadTOCWrite, CONTAINERNAME);
    longjmp(jmpBuf, 1);
  }

  omfsAddInt32toInt64(bufSize, &t->ioPos);            /* update the container position  */
  omfsAddInt32toInt64(bufSize, &t->tocSize);          /* total the TOC size             */

  t->currBufrPos = t->ioPos;                          /* remember offset to this buffer */
  t->nextByte    = 0;                                 /* reset buffer byte ptr          */

  t->prevSegment.generation = 0;                      /* force gen for next OPT | PT | T*/

  return (t->tocSize);                                /* return current total TOC size  */
}


/*------------------------------------------*
 | writeTOCEntry - write a single TOC entry |
 *------------------------------------------*

 This is the low-level interface to write one entry to the TOC to the buffer allocated by
 cmStartTOCIO() and associated with the t control block parameter.  The entry's code
 (entryCode) and a pointer to the entry data (tocEntryData) are passed.  The codes
 determine the size of the entry (plus one for the code) and hence how much data the
 pointer is pointing to.  The data is always multiples of 4.  Thus the pointer is to an
 array of (unsigned) longs assumed aligned on a boundary appropriate to a int .

 The function returns the container offset to the entry data, i.e., where it will be when
 the buffer is written to the container.  This is done because some entries must be back
 patched later.  The value returned from here must therefore be saved for such entries.
 Note, that this is an offset to the data itself, NOT the code placed in front of that
 data, since it is the data that must be back patched and it is the offset of interest.
*/

static CMCount CM_NEAR writeTOCEntry(TOCIOControlPtr t, unsigned char entryCode,
                                                              unsigned int  *tocEntryData)
{
  ContainerPtr    container = t->container;
  CMCount					dataPos;
  unsigned int    nextFree, writePos, entrySize;
  unsigned char   *p;

  /* Get the buffer position where the next entry is to be placed and its size          */
  /* according to the code.                                                             */

  writePos  = t->nextByte;                      /* where to put next entry in  buffer   */
  entrySize = t->entrySize[entryCode];          /* entry data size                      */

  /* The entry data plus its code must fit completely in the buffer.  If it doesn't,    */
  /* flush the buffer and put the entry in a "new" (empty) buffer.                      */

  nextFree = writePos + 1 + entrySize;          /* next free byte after entry           */

  if (nextFree > t->tocBufSize) {               /* if entry doesn't fit entirely...     */
    (void)cmFlushTOCOutputBuffer(t, false);     /* ...flush the buffer                  */
    nextFree = 1 + entrySize;                   /* ...reset next free position          */
    writePos = 0;                               /* ...and the write buffer position     */
  }

  t->nextByte = nextFree;                       /* set next free position in buffer     */

  /* Copy the data to the buffer...                                                     */

  p = t->tocBuffer + writePos;                  /* point to where to put it             */

  CMformatData(container, p++, 1, &entryCode);  /* put in the code                      */

  /* Don't format immediate data!!  Leave in user order...                              */
  /* caller...                                                                          */
	switch (entryCode) {
    case Immediate0:                                /* immediate (length = 0)           */
    case Immediate1:                                /* immediate (length = 1)           */
    case Immediate2:                                /* immediate (length = 2)           */
    case Immediate3:                                /* immediate (length = 3)           */
    case Immediate4:                                /* immediate (length = 4)           */
    case ContdImmediate4:                           /* continued immediate (length = 4) */
			memcpy(p, tocEntryData++, (size_t)4);         /* immediates always use 4 bytes    */
    	p += 4;
      break;

  	default:
  		while (entrySize > 0) {                       /* then put in the data itself...       */
		    CMformatData(container,p, 4,tocEntryData++);/* ...in chunks of 4 bytes              */
		    p         += 4;
		    entrySize -= 4;
		  }
 	} /* end switch */


  /* Return container offset to the data in the buffer (NOT to the code in front)...    */
  dataPos = t->currBufrPos;
  omfsAddInt32toInt64(writePos, &dataPos);
  omfsAddInt32toInt64(1, &dataPos);

  return (dataPos);       /* container offset to the data         */
}


/*----------------------------------------*
 | readTOCEntry - read a single TOC entry |
 *----------------------------------------*

 This is the low-level interface to read one entry from the TOC from the buffer allocated
 by startTOCIO() and associated with the t control block parameter.  The entry's code is
 returned as the function result and the entry data copied to the int  array pointed to by
 tocEntryData.  The codes determine the size of the entry (plus one for the code) and hence
 how much data the pointer will be pointing to.  The data is always multiples of 4.  Thus
 the pointer to the array of (unsigned) longs is assumed aligned on a boundary appropriate
 to a int .  It is assumed there are enough longs to hold all the data (see code
 definitions).

 An "eof" condition is detected when the entire TOC has been read.  The entire TOC is
 considered read when the amount of bytes specified to cmStartTOCIO() has been processed.
 The "eof" is signalled by returning the special entry code, TOCEOF.  This code is NOT part
 of the TOC and only returned from here when ALL TOC entries have been read.
*/

static unsigned char CM_NEAR readTOCEntry(TOCIOControlPtr t, unsigned int  *tocEntryData,
                                                               CMCount *entryStartPtr, unsigned int  *entrySizePtr)
{
  ContainerPtr    container = t->container;
  CMSize32        amountRead;
  CMSize32				bufSize;
  unsigned char   entryCode, *p;
  CMSize				   remaining, totalRead;
  unsigned int       entrySize, remain32, ecSize;
  jmp_buf         jmpBuf;
  char            errStr[15];
  omfInt64				zero;

  omfsCvtUInt32toInt64(0, &zero);

  /* Process the next byte, which is assumed to always be a entry code byte. NOPs are   */
  /* ignored, so we loop back to process the next byte.  EndOfBufr cause the buffer to  */
  /* be reloaded.                                                                       */

  ecSize = 0;
	if(entryStartPtr)
	{
		*entryStartPtr = t->currBufrPos;					         /* tell interested parties the TOC      */
  	omfsAddInt32toInt64(t->nextByte, entryStartPtr);  /*   offset we're about to read from    */
	}
  for (;;) {                                    /* loop on NOPs and EndOfBufr...        */

    /* Reload the input buffer if all the bytes in it have been processed.  currBufSize */
    /* defines the size of the current input buffer.  Return TOCEOF as the entry code   */
    /* if all the bytes of the TOC have been read.                                      */

    if (t->nextByte >= t->currBufSize) {        /* if all the bytes were processed...   */
      t->currBufrPos = t->ioPos;                /* ...remember where new buffer begins  */

      remaining = t->tocSize;
      omfsCvtUInt32toInt64(t->totalRead, &totalRead);
      omfsSubInt64fromInt64(totalRead, &remaining);    /* ...reload the buffer...              */

      if (omfsInt64Equal(remaining, zero)) {                     /* if all TOC bytes have been read...   */
        #if CMDUMPTOC
        t->currTOCoffset = t->currBufrPos;      /* ...tell debugging world where we are */
        #endif
	*entrySizePtr = 0;
        return (TOCEOF);                        /* ...return "eof" condition entry code */
      }

      if(omfsTruncInt64toUInt32(remaining, &remain32) == OM_ERR_NONE)
      	bufSize    = (remain32 > t->tocBufSize) ? t->tocBufSize : remain32;
      else
      	bufSize    = t->tocBufSize;
      amountRead = CMfread(container, t->tocBuffer, sizeof(unsigned char), bufSize);

      if (amountRead != bufSize || amountRead == 0) {
        memcpy(&jmpBuf, &t->tocIOEnv.firstByte, sizeof(jmp_buf));
        CMfree(container, t);
        ERROR1(container,CM_err_BadTOCRead, CONTAINERNAME);
        longjmp(jmpBuf, 1);
      }

      t->currBufSize = bufSize;                 /* ...set new buffer size               */

      t->totalRead += bufSize;  							  /* ...total the amount read             */
      omfsAddInt32toInt64(bufSize, &t->ioPos);  /* ...remember container position       */

      t->nextByte    = 0;                       /* ...start processing from 1st byte    */
    } /* reloading buffer */

    /* Get the entry code first so we know how to process the entry data (if any) that  */
    /* follows...                                                                       */

    #if CMDUMPTOC
    t->currTOCoffset = t->currBufrPos +         /* tell interested parties the TOC      */
                       t->nextByte;             /*   offset we're about to read from    */
    #endif

    p = t->tocBuffer + t->nextByte;             /* point at the code byte in the buffer */

    CMextractData(container, p++,1, &entryCode);/* get code byte                        */

    if (entryCode == EndOfBufr) {               /* if EndOfBufr...                      */
#ifdef VIRTUAL_BENTO_OBJECTS
  		++t->nextByte;                              /* set pointer to next byte after code  */
	    /* ecSize should be the total of the entryCode + any EOF/Filler bytes needed to
	     * reach the entry (prepended) or else the object size will be computed incorrectly
	     */
		ecSize++;
		entryCode = NOP; /* Prime the pump */
		while((entryCode == NOP) && (t->nextByte < t->currBufSize))
    		{
		  CMextractData(container, p++,1, &entryCode);/* get code byte                        */
		  if(entryCode == NOP)
		    {
		      t->nextByte++;             								/* ...pretend we read all bytes         */
		      ecSize++;
		    }
		}
#else
      t->nextByte = t->currBufSize;             /* ...pretend we read all bytes         */
#endif
			continue;
    }

	  ++t->nextByte;                              /* set pointer to next byte after code  */
	  ecSize++;

    if (entryCode != NOP) break;                /* if NOPs, loop to process next byte   */
  } /* for */

  /* At this point the entry code has been extracted and it is not one of the special   */
  /* codes (NOP and EndOfBufr).  Verify that the code we do have is valid.  We assume   */
  /* nothing is safe!                                                                   */

  if (entryCode < FirstCode || entryCode > LastCode) {    /* error if not valid range   */
    memcpy(&jmpBuf, &t->tocIOEnv.firstByte, sizeof(jmp_buf));
    CMfree(container, t);
    ERROR2(container,CM_err_BadTOCCode, cmltostr(entryCode, -2, true, errStr), CONTAINERNAME);
    longjmp(jmpBuf, 2);
  }

  /* Now we have a valid entry code.  Get the corresponding data size and make sure that*/
  /* all the data can be extracted from the buffer according to that code.  This is     */
  /* another validation check.                                                          */

  entrySize = t->entrySize[entryCode];            /* get entry data size                */
	*entrySizePtr = entrySize+ecSize;								/* Return size including entryCode */

  t->nextByte += entrySize;                       /* point where next entry will begin  */

  if (t->nextByte > t->currBufSize+1) {           /* error if all bytes not in buffer   */
    memcpy(&jmpBuf, &t->tocIOEnv.firstByte, sizeof(jmp_buf));
    CMfree(container, t);
    ERROR1(container,CM_err_TOCoutOfSync, CONTAINERNAME);
    longjmp(jmpBuf, 3);
  }

  /* Extract the data from the buffer and it pass back in the array passed by the       */
  /* caller...                                                                          */
  /* Don't use formatting if immediate!!                                                */
	switch (entryCode) {
    case Immediate0:                                /* immediate (length = 0)           */
    case Immediate1:                                /* immediate (length = 1)           */
    case Immediate2:                                /* immediate (length = 2)           */
    case Immediate3:                                /* immediate (length = 3)           */
    case Immediate4:                                /* immediate (length = 4)           */
    case ContdImmediate4:                           /* continued immediate (length = 4) */
			memcpy(tocEntryData++, p, (size_t)4);
      break;


  	default:
  		while (entrySize > 0) {                         /* copy groups of 4-byte chunks...    */
   			CMextractData(container, p, 4, tocEntryData++);
    		p         += 4;
    		entrySize -= 4;
 		 }
 	} /* end switch */

  return (entryCode);                             /* entry code as function result      */
}


/*-------------------------------------------------------*
 | cmWrite1TOCSegment - write a single TOC value segment |
 *-------------------------------------------------------*

 All TOC value segment writing is done with this routine.  This routine interfaces the
 low-level TOC entry buffering routines (whose I/O control block is associated with
 tocIOCtl and allocated by cmStartTOCIO()) with the upper-level writers.  The upper level
 (i.e., the caller) views all value segments in terms of the fields shown as this
 routine's parameters.  Here that information is selectively passed to the lower-level
 buffering routines to generate a TOC in the container with the following syntactic layout:

                                 --                        --
                                 |  ( PT [g] [R] v... )     |
              { OPT [g] [R] v... |  <                 > ... | } ...
                                 |  ( T  [g] [R] v... )     |
                                 --                        --

 where OPT = object entry (NewObject code plus 4-byte object, property, and type ID)
       PT  = property entry (NewProperty code plus 4-byte property and type ID)
       T   = type entry (newType plus 4-byte type ID)
       g   = optional generation number if different from previous generation number
       R   = object ID of value's references recording object (in refDataObject's ID)
       v   = value entry (code plus offset/length or 4-byte immediate) -- code determines
             4 or 8 byte offsets and actual size of immediate

 In other words, nothing is repeated if it doesn't change. This routine thus remembers and
 controls what goes out and when.  Note that, at a minimum, a data value (v) is always
 generated.

 The "R" represents the object ID from the refDataObject parameter.  refDataObject is a
 pointer to a value's private reference recording object ID.  This is the value's private
 object used to hold the list of CMReference "key"/object ID associations as value data.
 If refDataObject is NULL no "R" is generated.  If not NULL we generate the ID after the
 OPT, PT, or T.

 The function returns the container offset to the value entry entry data (v).  This can
 be remembered by the higher-level caller for possible back patching later.

 Note, no consistency checking is done here. It is assumed all error checks have been done
 by the caller, and that by the time this routine is called, it is EXPECTED that a TOC
 entry will be created.  I/O errors can, of course, occur.  They use the setjmp/longjmp
 mechanism defined by cmStartTOCIO().
*/

CMCount cmWrite1TOCSegment(void *tocIOCtl,
                                 unsigned int   objectID,
                                 unsigned int   propertyID,
                                 unsigned int   typeID,
                                 CMCount			  value,
                                 CMSize			  valueLen,
                                 unsigned int   generation,
                                 unsigned short flags,
                                 TOCObjectPtr   refDataObject)
{
  TOCIOControlPtr t = (TOCIOControlPtr)tocIOCtl;
  unsigned char   entryCode;
  unsigned int    tocEntryData[MaxEntrySize], junk32;
  Boolean         newTypeSet;

  /* Generate OPT, PT, or T depending on how much has changed.  The first write will,   */
  /* of course, generate an OPT since the "previous" object ID is initicalized to 0.    */
  /* If any of these generate, then a possible "generation" can follow.  This is        */
  /* described after we get through here.                                               */

  /* Note, the "previous" values for the object, propery, and type IDs are saved in the */
  /* I/O control block.  If we change them here, the new value becomes the "previous"   */
  /* value.  That is how the change is detected.                                        */

  if (t->prevSegment.objectID != objectID) {              /* new object...              */
    t->prevSegment.objectID   = tocEntryData[0] = objectID;
    t->prevSegment.propertyID = tocEntryData[1] = propertyID;
    t->prevSegment.typeID     = tocEntryData[2] = typeID;
    (void)writeTOCEntry(t, NewObject, tocEntryData);      /* ...NewObject O P T         */
    newTypeSet = true;
  } else if (t->prevSegment.propertyID != propertyID) {   /* same object,new property...*/
    t->prevSegment.propertyID = tocEntryData[0] = propertyID;
    t->prevSegment.typeID     = tocEntryData[1] = typeID;
    (void)writeTOCEntry(t, NewProperty, tocEntryData);    /* ...NewProperty P T         */
    newTypeSet = true;
  } else if (t->prevSegment.typeID != typeID) {           /* same obj/prop, new type... */
    t->prevSegment.typeID     = typeID;
    (void)writeTOCEntry(t, NewType, &t->prevSegment.typeID);/*...NewType T              */
    newTypeSet = true;
  } else
    newTypeSet = false;                                   /* no type was set            */

  /* If OPT, PT, or T was generated, then a generation entry and/or references recording*/
  /* object ID can follow it.  The generation is suppressed whenever the value we would */
  /* generate is equal to the previous value.  Note that the "previous" value is        */
  /* invalidated (made 0) whenever the buffer is flushed. This will cause the gnenration*/
  /* to repeat following the first OPT, PT, or T in the next block. This is a concession*/
  /* to direct access implementation of TOC blocks (not our implementation) where it    */
  /* will need to pick up the generation again as it accesses the TOC blocks            */
  /* individually.                                                                      */

  /* The references recording object ID can similarly only occur after a OPT, PT, or T. */
  /* All these conditions indicate a new value (data).  Thus we can unconditionally put */
  /* out the ID at this point if one is provided.  It will NEVER be the same as a       */
  /* previous one since each value that has references has its own private recording    */
  /* object to hold the CMReference key/object ID associations.                         */

  if (newTypeSet) {                                       /* if OPT, PT, or T, and...   */
    if (generation != t->prevSegment.generation) {        /* ...not same as last time...*/
      t->prevSegment.generation = generation;             /* ...remember for next time  */
      (void)writeTOCEntry(t, ExplicitGen, &generation);   /* ...g                       */
    }

    if (refDataObject)                                    /* ...R (if one is specified) */
      (void)writeTOCEntry(t, ReferenceListID, &refDataObject->objectID);
  }

  /* The value offset/length or immediate (v) is always generated.  The flags tell us   */
  /* how to interpret the value and which code to put out.                              */

  omfsTruncInt64toUInt32(value, &tocEntryData[0]);        /* set up value data          */
  omfsTruncInt64toUInt32(valueLen, &tocEntryData[1]);     /*... and its length          */

  if (flags & kCMImmediate) {                             /* if immediate data...       */
    if (flags & kCMContinued)                             /* ...cont'd always means 4   */
      entryCode = ContdImmediate4;
    else                                                  /* ...not cont'd ==> 0 to 4   */
      switch (tocEntryData[1]) {
        case 0:  entryCode = Immediate0; break;
        case 1:  entryCode = Immediate1; break;
        case 2:  entryCode = Immediate2; break;
        case 3:  entryCode = Immediate3; break;
        default: entryCode = Immediate4; break;
      }
	  omfsTruncInt64toUInt32(value, &tocEntryData[0]);        /* set up value data          */
 	  omfsTruncInt64toUInt32(valueLen, &tocEntryData[1]);     /*... and its length          */
  }
  else if(omfsTruncInt64toUInt32(valueLen, &junk32) != OM_ERR_NONE)	 /* non-immediate 8-byte offset...  */
  {
      entryCode = (unsigned char)((flags & kCMContinued) ? ContdOffset8Len8 : Offset8Len8);
	  omfsDecomposeInt64(value, (int  *)&tocEntryData[0], (int  *)&tocEntryData[1]);    /* set up value data          */
	  omfsDecomposeInt64(valueLen, (int  *)&tocEntryData[2], (int  *)&tocEntryData[3]);    /* set up value data          */
  }
  else if(omfsTruncInt64toUInt32(value, &junk32) != OM_ERR_NONE)	 /* non-immediate 8-byte offset...  */
  {
      entryCode = (unsigned char)((flags & kCMContinued) ? ContdOffset8Len4 : Offset8Len4);
	  omfsDecomposeInt64(value, (int  *)&tocEntryData[0], (int  *)&tocEntryData[1]);    /* set up value data          */
 	  omfsTruncInt64toUInt32(valueLen, &tocEntryData[2]);     /*... and its length          */
  }
  else                                                  /* non-immediate...           */
  {
   	  entryCode = (unsigned char)((flags & kCMContinued) ? ContdOffset4Len4 : Offset4Len4);
	  omfsTruncInt64toUInt32(value, &tocEntryData[0]);        /* set up value data          */
 	  omfsTruncInt64toUInt32(valueLen, &tocEntryData[1]);     /*... and its length          */
  }

  return (writeTOCEntry(t, entryCode, tocEntryData));     /* return offset to the value */
}


/*-----------------------------------------------------*
 | cmRead1TOCSegment - read a single TOC value segment |
 *-----------------------------------------------------*

 All TOC value segment reading is done with this routine.  This routine interfaces the
 low-level TOC entry buffering routines (whose I/O control block is associated with
 tocIOCtl and allocated by cmStartTOCIO()) with the upper-level writers.  The upper level
 (caller) views all value segments in terms of the fields shown as this routine's
 parameters.

 The container TOC has the syntactic layout described by cmWrite1TOCSegment().  Since this
 syntax describes a TOC layout where nothing is repeated if it doesn't change, this
 routine remembers the unrepeated information are repeats it back to the caller.

 The function always returns true until an "eof" is detected, i.e., until all the bytes,
 whose size was specified to cmStartTOCIO(), are read.  False is returned for the "eof".

 Note, refsDataObjectID represents a value's private reference recording object ID.  This
 is the value's private object used to hold the list of CMReference "key"/object ID
 associations as value data.  0x00000000UL is returned in refsDataObjectID if there is no
 recording object and, of course, will be the object ID if it is.
*/

/* JeffB: Many changes here to support virtual TOC, all are supplying extra data
          for higher level routines */
Boolean cmRead1TOCSegment(void *tocIOCtl,
                          unsigned int   *objectID,
                          unsigned int   *propertyID,
                          unsigned int   *typeID,
                          TOCValueBytesPtr value,
                          CMSize			  *valueLen,
                          unsigned int   *generation,
                          unsigned short *flags,
                          unsigned int   *refsDataObjectID,
                          CMCount				*segmentOffsetPtr,
                          unsigned int     *segmentLenPtr)
{
  TOCIOControlPtr t = (TOCIOControlPtr)tocIOCtl;
  ContainerPtr    container = t->container;
  unsigned char   entryCode;
  unsigned int    tocEntryData[MaxEntrySize];
  Boolean         newTypeRead;
  jmp_buf         jmpBuf;
  char            errStr[15];
  unsigned int       segmentLen = 0, entryLen;

  /* Is is always assumed that TOC reading is "aligned" on a entry code byte, i.e., it  */
  /* is always the next byte to read and at least one entire entry is consumed.  More   */
  /* entries may be consumed.  Pick up the first code and do some initial checks on it. */

  entryCode = readTOCEntry(t, tocEntryData, segmentOffsetPtr, &entryLen);        /* get the initial code             */
	segmentLen += entryLen;

  #if CMDUMPTOC
  SESSION->currTOCoffset = t->currTOCoffset;        /* announce offset for debug display*/
  #endif

  if (entryCode == TOCEOF)
  	return (false);          /* if "eof", tell caller now        */

  if (entryCode != NewObject && t->prevSegment.objectID == 0x00000000UL) { /* 1st time? */
    memcpy(&jmpBuf, &t->tocIOEnv.firstByte, sizeof(jmp_buf));
    CMfree(container, t);
    ERROR2(container,CM_err_TOCParseErr1, cmltostr(entryCode, -2, true, errStr), CONTAINERNAME);
    longjmp(jmpBuf, 1);
  }

  /* Process the initial code for OPT, PT, and T.  Suck up the appropriate amount of    */
  /* ID values if we have one of these and read another entry code which must be either */
  /* a generation number (g) of value data (v).  If we don't have OPT, PT, or T, only   */
  /* value data may follow and we use the "previous" O P T IDs we aready have.  Note,   */
  /* they must exist in that case, since we did an initial validation check that ensured*/
  /* the first TOC entry was OPT and thus set all the "previous" values.                */

  switch (entryCode) {
    case NewObject:                                 /* O P T                            */
      *objectID   = t->prevSegment.objectID   = tocEntryData[0];
      *propertyID = t->prevSegment.propertyID = tocEntryData[1];
      *typeID     = t->prevSegment.typeID     = tocEntryData[2];
      entryCode = readTOCEntry(t, tocEntryData, NULL, &entryLen);
			segmentLen += entryLen;
      newTypeRead = true;
      break;

    case NewProperty:                               /* P T                              */
      *objectID   = t->prevSegment.objectID;
      *propertyID = t->prevSegment.propertyID = tocEntryData[0];
      *typeID     = t->prevSegment.typeID     = tocEntryData[1];
      entryCode = readTOCEntry(t, tocEntryData, NULL, &entryLen);
 			segmentLen += entryLen;
    	newTypeRead = true;
      break;

    case NewType:                                   /* T                                */
      *objectID   = t->prevSegment.objectID;
      *propertyID = t->prevSegment.propertyID;
      *typeID     = t->prevSegment.typeID = tocEntryData[0];
      entryCode = readTOCEntry(t, tocEntryData, NULL, &entryLen);
			segmentLen += entryLen;
      newTypeRead = true;
      break;

    default:                                        /* use "previous" O P T             */
      *objectID   = t->prevSegment.objectID;
      *propertyID = t->prevSegment.propertyID;
      *typeID     = t->prevSegment.typeID;
      newTypeRead = false;                          /* no type was read                 */
      break;
  } /* OPT | PT | T */

  /* If we encountered an OPT, PT, or T, then a generation number can follow.  If it    */
  /* does we use it and continue to use it until we see another explicit generation     */
  /* number.  All continued segments will get the same generation number.  It is        */
  /* guaranteed that we will see an generation number after the first OPT, PT, or T in  */
  /* EACH new block we read.                                                            */

  #if CMDUMPTOC
  SESSION->gotExplicitGen = false;                  /* tell debuggers no change in gen  */
  #endif

  *refsDataObjectID = 0x00000000UL;                 /* assume no refs recording object  */

  if (newTypeRead) {                                /* if OPT, PT, or T...              */
    if (entryCode == ExplicitGen) {                 /* ...if generation follows...      */
      t->prevSegment.generation = tocEntryData[0];  /* ...save it                       */
      #if CMDUMPTOC
      SESSION->gotExplicitGen = true;               /* announce explicit gen seen       */
      #endif
      entryCode = readTOCEntry(t, tocEntryData, NULL, &entryLen);    /* ...a v or R better follow it     */
			segmentLen += entryLen;
    }

    if (entryCode == ReferenceListID) {             /* ...if recording object ID follows*/
      *refsDataObjectID = tocEntryData[0];          /* ...return it                     */
      entryCode = readTOCEntry(t, tocEntryData, NULL, &entryLen);    /* ...a value (v) better follow it  */
			segmentLen += entryLen;
    }
  }

  *generation = t->prevSegment.generation;          /* return appropriate generation nbr*/

  /* At this point a data value is expected.  The entry code tells us exactly what we   */
  /* got, i.e., how much data there is and how to set the flags.                        */

  switch (entryCode) {
    case Offset4Len4:                               /* 4-byte offset + 4-byte length    */
      omfsCvtUInt32toInt64(tocEntryData[0], &value->notImm.value);
      omfsCvtUInt32toInt64(tocEntryData[1], valueLen);
      *flags    = 0x0000U;
      break;

    case ContdOffset4Len4:                          /* cont'd 4-byte offset + 4-byte len*/
      omfsCvtUInt32toInt64(tocEntryData[0], &value->notImm.value);
      omfsCvtUInt32toInt64(tocEntryData[1], valueLen);
      *flags    = kCMContinued;
      break;

    case Offset8Len4:                               /* 8-byte offset + 4-byte length    */
	  omfsMakeInt64(tocEntryData[0], tocEntryData[1], &value->notImm.value);
      omfsCvtUInt32toInt64(tocEntryData[2], valueLen);
      *flags    = 0x0000U;
      break;

    case ContdOffset8Len4:                          /* cont'd 8-byte offset + 4-byte len*/
	  omfsMakeInt64(tocEntryData[0], tocEntryData[1], &value->notImm.value);
      omfsCvtUInt32toInt64(tocEntryData[2], valueLen);
      *flags    = kCMContinued;
      break;

    case Offset8Len8:                               /* 8-byte offset + 4-byte length    */
	  omfsMakeInt64(tocEntryData[0], tocEntryData[1], &value->notImm.value);
	  omfsMakeInt64(tocEntryData[2], tocEntryData[3], valueLen);
      *flags    = 0x0000U;
      break;

    case ContdOffset8Len8:                          /* cont'd 8-byte offset + 4-byte len*/
	  omfsMakeInt64(tocEntryData[0], tocEntryData[1], &value->notImm.value);
	  omfsMakeInt64(tocEntryData[2], tocEntryData[3], valueLen);
      *flags    = kCMContinued;
      break;

    case Immediate0:                                /* immediate (length = 0)           */
      omfsCvtUInt32toInt64(0, &value->imm.value);
      omfsCvtUInt32toInt64(0, valueLen);
      *flags    = kCMImmediate;
      break;

    case Immediate1:                                /* immediate (length = 1)           */
      omfsCvtUInt32toInt64(tocEntryData[0], &value->imm.value);
      omfsCvtUInt32toInt64(1, valueLen);
      *flags    = kCMImmediate;
      break;

    case Immediate2:                                /* immediate (length = 2)           */
      omfsCvtUInt32toInt64(tocEntryData[0], &value->imm.value);
      omfsCvtUInt32toInt64(2, valueLen);
      *flags    = kCMImmediate;
      break;

    case Immediate3:                                /* immediate (length = 3)           */
      omfsCvtUInt32toInt64(tocEntryData[0], &value->imm.value);
      omfsCvtUInt32toInt64(3, valueLen);
      *flags    = kCMImmediate;
      break;

    case Immediate4:                                /* immediate (length = 4)           */
      omfsCvtUInt32toInt64(tocEntryData[0], &value->imm.value);
      omfsCvtUInt32toInt64(4, valueLen);
      *flags    = kCMImmediate;
      break;

    case ContdImmediate4:                           /* continued immediate (length = 4) */
      omfsCvtUInt32toInt64(tocEntryData[0], &value->imm.value);
      omfsCvtUInt32toInt64(4, valueLen);
      *flags    = (kCMImmediate | kCMContinued);
      break;

    default:                                        /* "I've fallen and can't get up!"  */
      memcpy(&jmpBuf, &t->tocIOEnv.firstByte, sizeof(jmp_buf));
      CMfree(container, t);
      ERROR2(container,CM_err_TOCParseErr2, cmltostr(entryCode, -2, true, errStr), CONTAINERNAME);
      longjmp(jmpBuf, 3);
  } /* switch */

	*segmentLenPtr = segmentLen;
  return (true);                                    /* tell caller to keep reading      */
}


#if TOC1_SUPPORT
/*----------------------------------------------------------------------------------------*
 | buildGlobalNameTable - create global symbol table entries for TOC 1 global name values |
 *----------------------------------------------------------------------------------------*

 This routine creates a global name symbol table entry for each value that has a property
 ID that indicates the value is for a global name.

 When a format 1 TOC is read in from a container we must build up our memory structures
 exactly as they were when the container was previously written.  That means that, in
 addition to the TOC data structures, we have to build the global name symbol table
 containing the global name strings for those values. Thus is routine is ONLY called during
 read-in (and only from cmAppendValue()).  At that time the switch tocFullyReadIn is false,
 which is a status switch indicating the TOC read is not yet complete.

 A word of caution!  Remember that this routine is only called during read in of a TOC. But
 here we must read in a global name.  That will CHANGE the current "seek" position of the
 next read.  cmReadTOC(), which is the routine that does the TOC reading, always keeps its
 current TOC offset (the one just read) in tocInputOffset in the container.  We use that to
 re-seek so that cmReadTOC() is none the wiser.

 Note, there IS an alternate scheme that will equally work to avoid the additional seeks.
 That is to build the global name table AFTER the TOC is read in at the same time we walk
 the entire TOC to add objects to their "master chains" of objects, properties, and types.
 Indeed, once upon a time, that's exactly what we did do!  But the current scheme is
 probably (depending on your I/O efficiency -- your milage may vary) more efficient.  Why?
 Because here the global names are built only when we know we have a global name.  So only
 those objects are involved.  Second, the final TOC walk for the "master chains" is now
 only for objects.  This is a LOT more efficient than walking the entire TOC down to the
 values.
*/

static Boolean CM_NEAR buildGlobalNameTable(TOCValuePtr theValue)
{
  GlobalNamePtr  g, newGlobalName;
  Boolean        dup;
  unsigned int  valueLen;
  CMCount				offset, newPos;
  TOCValueHdrPtr theValueHdr = theValue->theValueHdr;
  ContainerPtr   container   = theValueHdr->container;

  /* Extract the global name container offset and its length from the TOC value...      */

  offset = theValue->value.notImm.value;
  omfsTruncInt64toUInt32(theValue->value.notImm.valueLen, &valueLen);

  /* Allocate a new global name symbol table entry.  Failure is disastrous!             */

  if ((newGlobalName = (GlobalNamePtr)CMmalloc(container, sizeof(GlobalName) + valueLen - 1)) == NULL) {
    ERROR1(container,CM_err_NoGNameLoad, CONTAINERNAME);
    return (false);
  }

  /* Seek to the name string and read it in.  Failures are still disastrous!            */

  CMfseek(container, offset, kCMSeekSet);

  if (CMfread(container, newGlobalName->globalName, sizeof(unsigned char), valueLen) != valueLen) {
    CMfree(container, newGlobalName);
    ERROR1(container,CM_err_BadGNameRead, CONTAINERNAME);
    return (false);
  }

  /* Since this routine is only called during TOC read in, we use tocInputOffset to seek*/
  /* back to the next TOC input position.  The TOC reader assumes, rightly so, that the */
  /* TOC is contiguous, so it doesn't do seeks.  It doesn't know we did one here to get */
  /* the global name.  So we put things back the way they should be.                    */

  #if (TOCInputBufSize > 0)                             /* buffering TOC input...       */
    CMfseek(container, cmBufferedIOftell(container->ioBuffer), kCMSeekSet);
  #else                                                 /* not buffering TOC input...   */
    newPos = container->tocInputOffset;
    omfsAddInt32toInt64(TOCentrySize, &newPos);
    CMfseek(container, newPos, kCMSeekSet);
  #endif

  if (*(newGlobalName->globalName + valueLen - 1) != '\0') { /* Now check the read...   */
    CMfree(container, newGlobalName);
    if((valueLen == 10) || (valueLen == 13))
    {
  	  /* JeffB 6/12/96:
  	   * We have seen a lot of problems where systems would translate ASCII <cr> info
  	   * <lf> and vice-versa.  Mac's use <cr> and Un*x uses <lf> as a line terminator.
  	   * This causes valueLen's on the global name 'omfi:Boolean'
  	   * to translate to a length of 10, which truncates the name & raises the error.
  	   */
  	  ERROR1(container,CM_err_CRSwap, CONTAINERNAME);
    }
    else
    {
  	  ERROR1(container,CM_err_NotGName, CONTAINERNAME);
    }
    return (false);
  }

  /* Enter the symbol and check for dups.  We must make sure we're not looking at a     */
  /* for a deleted value.  Of course, considering the time at which this routine is     */
  /* called, there shouldn't be any deleted entries!  But, what the hell!               */

  g = cmEnterGlobalName(container->globalNameTable, newGlobalName, &dup);

  if (dup) {                                            /* if we have a real dup...     */
    CMfree(container, newGlobalName);                              /* ...still don't need new entry*/
    ERROR2(container,CM_err_DupGlobalName, g->globalName, CONTAINERNAME); /* yell                 */
    return (false);                                     /* but continue if error returns*/
  }

  /* We're now happy with what we got! Set the value to point to the entry but retain   */
  /* the offset for the eventual write out to the container.  Set the flags accordingly.*/
  /* Echo flags in the value hdr.  Set the back pointer from the global name entry to   */
  /* the value.                                                                         */

  theValue->value.globalName.globalNameSymbol = g;
  theValue->value.globalName.offset           = offset;
  theValue->flags                             = kCMGlobalName;

  newGlobalName->theValue = theValue;                   /* set back pointer             */

  /* Echo the flags in the value header for this global name value.  Also set the size. */

  (void)cmSetTOCValueHdrFlags(theValueHdr, kCMGlobalName);
  omfsCvtUInt32toInt64(valueLen, &theValueHdr->size);     /* this includes null at end    */

  return (true);                                        /* global name entry now built  */
}


/*------------------------------------------------------------------------*
 | readTOCfmt1 - read in a TOC format 1 from a already existing container |
 *------------------------------------------------------------------------*

 This routine is called to read in a TOC format 1 from a container which has been opened
 for input by a call to CMOpenContainer().  The container, offset to TOC, and the size of
 the TOC are passed.  The function returns true if successfully loaded and false otherwise
 (this is a safety, since error reports should never return from the error handler).

 Reading a TOC can be stopped in two possible ways:

    1. By reading the entire TOC, i.e., tocSize bytes starting at tocOffset.

    2. By reading only a "private" TOC of an updating container.  This is stuff defined
       only for the container. That is read during "normal" opening of the container. What
       follows are TOC entries for objects to be merged with a target container's TOC.
       That will be read on a second call to read it AFTER the target is opened.  The
       updates are then automatically merged with the target's TOC which is inherited.
*/

static Boolean CM_NEAR readTOCfmt1(ContainerPtr container, CMCount tocOffset, CMSize tocSize)
{
  TOCObjectPtr   theObject;
  TOCValueHdrPtr theValueHdr;
  TOCentry       tocEntry;
  unsigned short objectFlags, shortGen;
  Boolean        updating = false, gotUpdatingStart = false;
  unsigned int   nbrOfUndefObjects, value32, valueLen32;
  CMCount				updatingStart;
#ifdef VIRTUAL_BENTO_OBJECTS
  CMCount				entryStart;
#endif
  char           countStr[12];
  omfInt64				entrySize64, zero;

  #if (TOCInputBufSize > 0)                   /* differnt stuff needed if buffering...  */
  void           *ioBuffer = NULL;
  jmp_buf        readTOCEnv;
  #else                                       /* ...as opposed to not buffering!        */
  unsigned char  tocBuffer[TOCentrySize];
  #endif

  #if (TOCInputBufSize > 0)                   /* if buffering...                        */

  /* If we are using buffered reads to load the TOC, then the input buffering routines  */
  /* report their errors and exit via a longjmp on the setjmp/longjmp environment       */
  /* we are about to define.  This avoids checking for errors explicitly on each read   */
  /* that we do in here.                                                                */

  if (setjmp(readTOCEnv)) {                   /* set setjmp/longjmp environment vector  */
    cmReleaseIOBuffer(ioBuffer);              /* ...if longjmp taken back to here...    */
    ERROR1(container,CM_err_BadTOCRead, CONTAINERNAME); /* ...free buffer and report failure      */
    return (false);                           /* ...false ==> failure                   */
  }

  #endif

  omfsCvtUInt32toInt64(0, &zero);
  /* Position to the start of the TOC...                                                */

  CMfseek(container, tocOffset, kCMSeekSet);

  /* Allocate the input buffer and prepare for buffered reading of the TOC...           */

  #if (TOCInputBufSize > 0)                   /* if buffering...                        */
  ioBuffer = cmUseIOBuffer(container, TOCInputBufSize, (jmp_buf *)&readTOCEnv);
  if (ioBuffer == NULL) return (false);
  cmNewBufferedInputData(ioBuffer, NULL, tocSize);
  #endif

  /* At all time while we're reading a TOC, the container variable, tocInputOffset,     */
  /* contains the current TOC offset of the entry being processed.  This is used in     */
  /* determining whether to "split" the TOC read for updating between the public and    */
  /* private sections.  It's also good to know anyway, so we always know where we are   */
  /* the TOC read.                                                                      */

  container->tocInputOffset = tocOffset;

  /* Also keep the ioBuffer we're using for the case where we encounter a global name.  */
  /* These names must be read to put them in the global name table.  That will change   */
  /* the current input position.  The ioBuffer must be know so that it can be reseeked  */
  /* back to the current buffered input position.                                       */

  #if (TOCInputBufSize > 0)                   /* if buffering...                        */
  container->ioBuffer = ioBuffer;             /* ..."broadcast" the buffer we're using  */
  #else                                       /* if no buffering...                     */
  container->ioBuffer = NULL;                 /* ..."broadcast" that too                */
  #endif

  /* As object ID's are encountered during the reading we maintain a counter,           */
  /* nbrOfUndefObjects, representing the number of undefined objects.  Objects can be   */
  /* forward or backward referenced and we can have multiple references.  Thus we have  */
  /* to be carfull how we handle this counter.  A special flag in the object is used to */
  /* ensure we don't count undefined objects more than once and to not count defined    */
  /* objects.  The counter is set initially, of course, to 0...                         */

  nbrOfUndefObjects = 0;                  /* hopefully it will be 0 at the end too!     */

  /* Read in the TOC, one object definition at a time.  For each object we will get a   */
  /* object ID (what a suprise!), a property ID, and a type ID.  All three IDs are      */
  /* used to create objects.  The object ID may or may not be a property or type.  We   */
  /* don't know that here.  The property and type IDs are known to be such because of   */
  /* where they are extracted for the TOC object.  But all we know is that these are    */
  /* for property and type objects.  We don't know their full object definitions until  */
  /* we actually encounter them in the TOC load.  All this forward (and backward) ID    */
  /* referencing is handled by the object definition routine, cmDefineObject(). All we  */
  /* do here is define the IDs as objects for each ID number seen.  The property and    */
  /* type IDs are always considered as undefined objects here.  cmDefineObject() will   */
  /* handle the forward refrences behind our backs.  So here it goes...                 */

  while (omfsInt64Greater(tocSize, zero)) {                   /* for each entry in the container's TOC...   */
#ifdef VIRTUAL_BENTO_OBJECTS
		entryStart = container->tocInputOffset;
#endif
#if (TOCInputBufSize > 0)             /* if buffering, extract fields here...       */
    BufferedExtractTOC(ioBuffer, tocEntry.objectID,
                                 tocEntry.propertyID,
                                 tocEntry.typeID,
                                 value32,
                                 valueLen32,
                                 shortGen,
                                 tocEntry.flags,
                                 isSmall);
    tocEntry.generation = (unsigned int )shortGen;
#else                                 /* if not buffering, extract directly...      */
    if (CMfread(container, tocBuffer, sizeof(unsigned char), TOCentrySize) != TOCentrySize) {
      ERROR1(container,CM_err_BadTOCRead, CONTAINERNAME);
      return (false);
    }
    ExtractTOC(tocBuffer, tocEntry.objectID,
                          tocEntry.propertyID,
                          tocEntry.typeID,
                          value32,
                          valueLen32,
                          shortGen,
                          tocEntry.flags);
    tocEntry.generation = (unsigned int )shortGen;
#endif

#ifdef PORT_FILESYS_40BIT
	(void)omfsMakeInt64((tocEntry.propertyID >> 24L) & 0xFF, value32, &tocEntry.value.notImm.value);
	(void)omfsMakeInt64((tocEntry.typeID >> 24L) & 0xFF, valueLen32, &tocEntry.value.notImm.valueLen);
#else
   	omfsCvtUInt32toInt64(value32, &tocEntry.value.notImm.value);
    omfsCvtUInt32toInt64(valueLen32, &tocEntry.value.notImm.valueLen);
#endif
	/* JeffB: We used to do this only for large files (>4gig), but a bug existed which
	 * caused negative immediate values to trigger the code which put data into
	 * the upper byte of the property and type ID's.  In order to be safe, we will
	 * strip that byte in  all cases.  This restricts the number of property names
	 * to 16 million, so it should be taken out at some time.    !!!
	 */
	tocEntry.propertyID &= 0x00FFFFFF;
	tocEntry.typeID &= 0x00FFFFFF;

    /* Look for an updating container's "pointing" value that points to this container's*/
    /* target.  If it's there we know we're reading a "private" TOC of the updating     */
    /* container because the pointing value is a property of TOC #1.  Under these       */
    /* conditions we want only want to read the private TOC.  What follows are updating */
    /* TOC entries (if any) that are to be merged with the target. That will be done by */
    /* a second read.  The updating TOC entries are defined by another TOC #1 property. */
    /* That gives us the starting offset and size.  Using this offset we know where the */
    /* provate TOC ends.  Note, there is no TOC #1 in the updates, so we will read to   */
    /* the end of the TOC on the second read.                                           */

    if (tocEntry.objectID == CM_StdObjID_TOC)             /* if looking at TOC #1...    */
      if (tocEntry.propertyID == CM_StdObjID_TOC_Target)  /* if "pointing" value...     */
        updating = true;                                  /* ...we have an updating TOC */
      else if (tocEntry.propertyID == CM_StdObjID_TOC_NewValuesTOC) {/*if updating TOC..*/
        updatingStart = tocEntry.value.imm.value;   		 /* ...save updating TOC start */
        gotUpdatingStart = true;                          /* ...and that we got it      */
      }

    if (updating && gotUpdatingStart)                     /* if updating TOC...         */
      if (omfsInt64GreaterEqual(container->tocInputOffset, updatingStart))     /* ...don't read updates now  */
          break;

    /* Define the object.  If the property ID is for a the standard type or property    */
    /* global name, the flags are set accordingly.  Everything else is treated as a     */
    /* plain object. It may turn out to be a property or type, but it is not in terms   */
    /* of a global name.  Also see comments above.                                      */

    if (tocEntry.propertyID == CM_StdObjID_GlobalTypeName)
      objectFlags = TypeObject;
    else if (tocEntry.propertyID == CM_StdObjID_GlobalPropName)
      objectFlags = PropertyObject;
    else
      objectFlags = ObjectObject;

#ifdef VIRTUAL_BENTO_OBJECTS
		/* JeffB: Added code to support virtual objects
		 */
	  if(!container->objectsMayBeVirtual || (objectFlags != ObjectObject))
	  {
#endif
	  	theObject = cmDefineObject(container, tocEntry.objectID, tocEntry.propertyID,
	                               tocEntry.typeID, &tocEntry.value, tocEntry.generation,
	                               tocEntry.flags, objectFlags, &theValueHdr);
#ifdef VIRTUAL_BENTO_OBJECTS
	  }
	  else
	  {
		  theObject = cmIntDefineObject(container, tocEntry.objectID, tocEntry.propertyID,
	                               tocEntry.typeID, &tocEntry.value, tocEntry.generation,
	                               tocEntry.flags, objectFlags,
	                               	LoadAsStub, entryStart, TOCentrySize,
																	&theValueHdr);
	  }
#endif
	  if (theObject == NULL) return (false);

    /* If this object was previously counted as undefined, reduce the count...          */

    if ((theObject->objectFlags & UndefObjectCounted) != 0) {
      theObject->objectFlags &= ~UndefObjectCounted;
      --nbrOfUndefObjects;
    }

    /* Link the object to its appropriate "master" chains.  The TOC is always sorted by */
    /* ascending object ID's.  Thus we know that we are linking these things in order.  */
    /* Note that type and property global names are ALWAYS marked as such by their      */
    /* property ID (we used that fact to set to flags above).  Thus these will go to    */
    /* their appropriate master chains.  See cmLinkObject() for details on the master   */
    /* chain links.                                                                     */

    cmLinkObject(container->toc, theObject, NULL);

    /* If the TOC value entry just defined is for the special "updates" property, then  */
    /* the object owning this property is a set of updates to be applied to a target    */
    /* TOC.  In this context we're already using that TOC.  The updates have to be      */
    /* applied to the "old" objects.  Pure new values just merge in.  But all other     */
    /* updates (e.g., moving, editing, etc.) are done by interpreting updating          */
    /* "instructions".  Those instructions are value data for the special "updates"     */
    /* property.  However, we can't process them now.  We have to wait until the entire */
    /* TOC is read in because the instructions could forward reference yet-to-be-read   */
    /* objects.  So, if the property is there, we chain those objects together on the   */
    /* touched chain.  This chain is normally used for recording objects touched for    */
    /* new updates. But that's later. Here we can overload it and use the same chaining */
    /* mechanisms.  Also, to make it easier for the chain walker, cmApplyUpdates(), to  */
    /* get at the property value, the object's refCon is set with the value "refNum".   */

    if (tocEntry.propertyID == CM_StdObjID_ValueUpdates)  /* if "updates" property...   */
#if USE_UPDATE_MODE
      if ((theObject->objectFlags & TouchedObject) == 0) {/* ...add to touched chain... */
        cmAddObjToTouchedChain(container, theObject);
        theObject->objectRefCon = (CMRefCon)theValueHdr;  /* ...save refNum to value    */
      }
#endif

    /* As mentioned above we always know when we have type and property objects by their*/
    /* property ID's.  Anything not so marked must be a "garden-variety" object.  Such  */
    /* an object may be forward referencing to yet-to-be-defined type and property      */
    /* objects.  We know which is which, by the referencing type and property fields in */
    /* the TOC entry. So what we do is "declare" (create) these objects as undefined but*/
    /* marked as to what they are (type or property).  This is an added check so that   */
    /* when we actually encounter these objects we can ask if they actually are a type  */
    /* or property.                                                                     */

    /* Define a (undefined) property user object...                                     */

    if (tocEntry.propertyID >= MinUserObjectID) {
      theObject = cmDefineObject(container, tocEntry.propertyID, 0, 0, NULL, 0, 0,
                                 UndefinedObject | PropertyObject, NULL);
      if (theObject == NULL) return (false);

      /* If this object actually is undefined, count it as such (but only once)...      */

      if (theObject->objectFlags & UndefinedObject)
        if ((theObject->objectFlags & UndefObjectCounted) == 0) {
          theObject->objectFlags |= UndefObjectCounted;
          ++nbrOfUndefObjects;
        }
    }

    /* Define a (undefined) type user object...                                         */

    if (tocEntry.typeID >= MinUserObjectID) {
      theObject = cmDefineObject(container, tocEntry.typeID, 0, 0, NULL, 0, 0,
                                 UndefinedObject | TypeObject, NULL);
      if (theObject == NULL) return (false);

      /* If this object actually is undefined, count it as such (but only once)...      */

      if (theObject->objectFlags & UndefinedObject)
        if ((theObject->objectFlags & UndefObjectCounted) == 0) {
          theObject->objectFlags |= UndefObjectCounted;
          ++nbrOfUndefObjects;
        }
    }


		omfsCvtUInt32toInt64(TOCentrySize, &entrySize64);
    omfsSubInt64fromInt64(entrySize64, &tocSize); /* count down the size                  */
    omfsAddInt64toInt64(entrySize64, &container->tocInputOffset);  /* ...and bump offset    */
  } /* while */

  #if (TOCInputBufSize > 0)
  cmReleaseIOBuffer(ioBuffer);                  /* done with input buffer               */
  #endif

  /* We're done reading.  Report an error if there are any undefined objects...         */

  if (nbrOfUndefObjects > 0) {
    ERROR2(container,CM_err_UndefObjects, cmltostr(nbrOfUndefObjects, 1, false, countStr), CONTAINERNAME);
    return (false);
  }

  return (true);
}
#endif

#ifdef VIRTUAL_BENTO_OBJECTS
/*------------------------------------------------------------------------------------*
 | cmLoadUpStubObject - Load up the properties on an object stub										  |
 *------------------------------------------------------------------------------------*

 JeffB: Added to support virtual objects.  Returns theObject if passed, NULL on failure.
 */

TOCObjectPtr cmLoadUpStubObject(TOCObjectPtr theObject)
{
	CMCount					saveOffset;
	TOCentry    		tocEntry;
	ContainerPtr		container = theObject->container;
  unsigned short	shortGen;
  unsigned int       value32, valueLen32;
  TOCValueHdrPtr	theValueHdr;
  unsigned int       refsDataObjectID, entrySize, junk;
  CMSize					bytesLeft, zero, entryStart;
  jmp_buf     		readEnv;
  void            *localTocIOCtl;          /*    current TOC I/O control block pointer   */
/*!!! ;--- handle malloc errors here	*/

#if (TOCInputBufSize > 0)
#error Buffering not supported on Bento d4 files
#else
  unsigned char  tocBuffer[TOCentrySize];
#endif

	if(theObject->objectFlags  & ObjectIsNotLoaded)
	{
 		/* !!! Set the
 		 */
 		container->tocFullyReadIn = false;

	  /* Define a TOC I/O control block and buffer. The control block pointer is saved in   */
	  /* the container mainly for cmBuildGlobalNameTable() to use when it defines global    */
	  /* names and needs to reseek to the current TOC input position. Note, the size of the */
	  /* TOC buffer required for reading this TOC is as defined in the container label.  It */
	  /* was squirreled away at open time.                                                  */

		saveOffset = CMftell(container);
		CMfseek(container, theObject->fileOffset, kCMSeekSet);

		if (((ContainerPtr)container)->majorVersion != 1)
		{
		  if (setjmp(readEnv))                    /* set setjmp/longjmp environment vector      */
		    return(NULL);       			      /* ...just quit if there's a TOC input error  */
		  localTocIOCtl = cmStartTOCIO(container, container->tocBufSize, (jmp_buf *)&readEnv,
		                                     theObject->fileOffset, theObject->allEntrySize);
		  if (localTocIOCtl == NULL)
		    return(NULL);                       /* something went wrong (too bad)!            */
		}

		bytesLeft = theObject->allEntrySize;
		omfsCvtInt32toInt64(0, &zero);
		while(omfsInt64Greater(bytesLeft, zero))
		{
			if (((ContainerPtr)container)->majorVersion == 1)
			{
				if (CMfread(container, tocBuffer, sizeof(unsigned char), TOCentrySize) != TOCentrySize) {
					ERROR1(container,CM_err_BadTOCRead, CONTAINERNAME);
					return(NULL);
				}
	    	ExtractTOC(tocBuffer, tocEntry.objectID,
	                          tocEntry.propertyID,
	                          tocEntry.typeID,
	                          value32,
	                          valueLen32,
	                          shortGen,
	                          tocEntry.flags);
				tocEntry.generation = (unsigned int )shortGen;
#ifdef PORT_FILESYS_40BIT
				(void)omfsMakeInt64((tocEntry.propertyID >> 24L) & 0xFF, value32, &tocEntry.value.notImm.value);
				(void)omfsMakeInt64((tocEntry.typeID >> 24L) & 0xFF, valueLen32, &tocEntry.value.notImm.valueLen);
				tocEntry.propertyID &= 0x00FFFFFF;
				tocEntry.typeID &= 0x00FFFFFF;
#else
   			omfsCvtUInt32toInt64(value32, &tocEntry.value.notImm.value);
   			omfsCvtUInt32toInt64(valueLen32, &tocEntry.value.notImm.valueLen);
#endif
				entrySize = TOCentrySize;
			}
	 		else
	 		{
				cmRead1TOCSegment(localTocIOCtl,
                          &tocEntry.objectID,
	                        &tocEntry.propertyID,
	                        &tocEntry.typeID,
                          &tocEntry.value,
                          &tocEntry.value.notImm.valueLen,
	                        &tocEntry.generation,
	                        &tocEntry.flags,
                          &refsDataObjectID,	/* Not used at present */
                          &entryStart,
                          &entrySize);
	 		}
			omfsSubInt32fromInt64(entrySize, &bytesLeft);

		  theObject = cmIntDefineObject(container, tocEntry.objectID, tocEntry.propertyID,
		                               tocEntry.typeID, &tocEntry.value, tocEntry.generation,
		                               tocEntry.flags, 0,
		                               LoadAllFields, container->tocInputOffset, 0,
		                               &theValueHdr);
			if(theObject == NULL)
				return(NULL);

			if (((ContainerPtr)container)->majorVersion != 1)
			{
				if(!ObjectDefCleanup(container, theObject, tocEntry, &junk,
												refsDataObjectID, theValueHdr))
					return(NULL);
			}
 		}

		CMfseek(theObject->container, saveOffset, kCMSeekSet);
		if (((ContainerPtr)container)->majorVersion != 1)
		{
			/* We're done reading.  Free the TOC I/O control block and its buffer and report an   */
	  		/* error if there are any undefined objects...                                        */
	  		/* Allocating & Deallocating this buffering may be a major performance bottleneck */
	  		(void)cmEndTOCIO(localTocIOCtl);
			/*  	container->tocIOCtl = NULL; */
		}

		theObject->objectFlags &= ~ObjectIsNotLoaded;
 		container->tocFullyReadIn = true;
	}

	return(theObject);
}
#endif

/*** !!!! ***/
static Boolean ObjectDefCleanup(ContainerPtr container, TOCObjectPtr theObject,
                                                  TOCentry tocEntry, unsigned int  *undefObjectsPtr,
                                                  unsigned int  refsDataObjectID, TOCValueHdrPtr theValueHdr)
{
		/* if recording value...*/
		if (tocEntry.typeID == CM_StdObjID_ObjRefData)
		{
	    theValueHdr->valueFlags |= ValueProtected;          /* ...protect it              */
	    theValueHdr->theProperty->theObject->objectFlags |= ProtectedObject;
	  }
	 else if ((theValueHdr->valueFlags & ValueProtected) == 0)
	 {
		  /* If the TOC value entry just defined is for the special "updates" property, then  */
		  /* the object owning this property is a set of updates to be applied to a target    */
		  /* TOC.  In this context we're already using that TOC.  The updates have to be      */
		  /* applied to the "old" objects.  Pure new values just merge in.  But all other     */
		  /* updates (e.g., moving, editing, etc.) are done by interpreting updating          */
		  /* "instructions".  Those instructions are value data for the special "updates"     */
		  /* property.  However, we can't process them now.  We have to wait until the entire */
		  /* TOC is read in because the instructions could forward reference yet-to-be-read   */
		  /* objects.  So, if the property is there, we chain those objects together on the   */
		  /* touched chain.  This chain is normally used for recording objects touched for    */
		  /* new updates. But that's later. Here we can overload it and use the same chaining */
		  /* mechanisms.  Also, to make it easier for the chain walker, cmApplyUpdates(), to  */
		  /* get at the property value, the object's refCon is set with the value "refNum".   */
#if USE_UPDATE_MODE
		  if (tocEntry.propertyID == CM_StdObjID_ValueUpdates)  /* if "updates" property...   */
		    if ((theObject->objectFlags & TouchedObject) == 0) {/* ...add to touched chain... */
		      cmAddObjToTouchedChain(container, theObject);
		      theObject->objectRefCon = (CMRefCon)theValueHdr;  /* ...save refNum to value    */
		    }
#endif

		  /* As mentioned above we always know when we have type and property objects by their*/
		  /* property ID's.  Anything not so marked must be a "garden-variety" object.  Such  */
		  /* an object may be forward referencing to yet-to-be-defined type and property      */
		  /* objects.  We know which is which, by the referencing type and property fields in */
		  /* the TOC entry. So what we do is "declare" (create) these objects as undefined but*/
		  /* marked as to what they are (type or property).  This is an added check so that   */
		  /* when we actually encounter these objects we can ask if they actually are a type  */
		  /* or property.                                                                     */

		  /* Define a (undefined) property user object...                                     */

		  if (tocEntry.propertyID >= MinUserObjectID) {
		    theObject = cmDefineObject(container, tocEntry.propertyID, 0, 0, NULL, 0, 0,
		                               UndefinedObject | PropertyObject, NULL);
		    if (theObject == NULL) return (false);

		    /* If this object actually is undefined, count it as such (but only once)...      */

		    if (theObject->objectFlags & UndefinedObject)
		      if ((theObject->objectFlags & UndefObjectCounted) == 0) {
		        theObject->objectFlags |= UndefObjectCounted;
		        ++(*undefObjectsPtr);
		      }
		  } /* fwd ref to property */

		  /* Define a (undefined) type user object...                                         */

		  if (tocEntry.typeID >= MinUserObjectID) {
		    theObject = cmDefineObject(container, tocEntry.typeID, 0, 0, NULL, 0, 0,
		                               UndefinedObject | TypeObject, NULL);
		    if (theObject == NULL) return (false);

		    /* If this object actually is undefined, count it as such (but only once)...      */

		    if (theObject->objectFlags & UndefinedObject)
		      if ((theObject->objectFlags & UndefObjectCounted) == 0) {
		        theObject->objectFlags |= UndefObjectCounted;
		        ++(*undefObjectsPtr);
		      }
		  } /* fwd ref to type */
	  }

   /* Just as we handle possible forward referenced properties and types, we have the  */
  /* possibility of forward references to a value's recording object for references.  */
  /* Object references are "pointers" (i.e., object IDs) to other objects from a value*/
  /* that contains data that refers to those objects.  The data is in the form of a   */
  /* CMReference "key" and associated object ID.  The associations are maintained in  */
  /* a list as value data for a uniquely typed value in a private object "tied" to the*/
  /* value through a pointer in the value header.  It is this private recording object*/
  /* we define here and tie it to the value (header).                                 */

  /* We know we have a recording object for a value when the cmRead1TOCSegment() we   */
  /* did above returns a non-zero refsDataObjectID.  The container TOC places the     */
  /* recording object ID just in front of the value (see cmWrite1TOCSegment() for a   */
  /* description of the TOC format).  If there is none, 0 is retured.                 */

  /* If the recording object is still undefined here, then it will eventually get     */
  /* defined just like forward referenced types and properties. If it already defined,*/
  /* so be it.  Either way we get the object pointer so we can hook it to the value.  */

  if (refsDataObjectID != 0x00000000UL) {
    theObject = cmDefineObject(container, refsDataObjectID, 0, 0, NULL, 0, 0,
                               UndefinedObject | ObjectObject, NULL);
    if (theObject == NULL) return (false);

    /* If this object actually is undefined, count it as such (but only once)...      */

    if (theObject->objectFlags & UndefinedObject)
      if ((theObject->objectFlags & UndefObjectCounted) == 0) {
        theObject->objectFlags |= UndefObjectCounted;
        ++(*undefObjectsPtr);
      }

    /* Tie the value header to the its recording object...                            */

    RefDataObject(theValueHdr) = theObject;           /* simple, isn't it?            */
	}
	return(true);
}
/*** !!!! ***/

/*------------------------------------------------------------------------------------*
 | cmBuildGlobalNameTable - create global symbol table entries for global name values |
 *------------------------------------------------------------------------------------*

 This routine creates a global name symbol table entry for each value that has a property
 ID that indicates the value is for a global name.

 When a TOC is read in from a container we must build up our memory structures exactly as
 they were when the container was previously written.  That means that, in addition to the
 TOC data structures, we have to build the global name symbol table containing the global
 name strings for those values. Thus is routine is ONLY called during read-in (and only
 from cmAppendValue()).  At that time the switch tocFullyReadIn is false, which is a status
 switch indicating the TOC read is not yet complete.

 A word of caution!  Remember that this routine is only called during read in of a TOC. But
 here we must read in a global name.  That will CHANGE the current "seek" position of the
 next read.  cmReadTOC(), which is the routine that does the TOC reading, always keeps its
 current TOC offset (the one just read) in tocInputOffset in the container.  We use that to
 re-seek so that cmReadTOC() is none the wiser.

 Note, there IS an alternate scheme that will equally work to avoid the additional seeks.
 That is to build the global name table AFTER the TOC is read in at the same time we walk
 the entire TOC to add objects to their "master chains" of objects, properties, and types.
 Indeed, once upon a time, that's exactly what we did do!  But the current scheme is
 probably (depending on your I/O efficiency -- your milage may vary) more efficient.  Why?
 Because here the global names are built only when we know we have a global name.  So only
 those objects are involved.  Second, the final TOC walk for the "master chains" is now
 only for objects.  This is a LOT more efficient than walking the entire TOC down to the
 values.
*/

Boolean cmBuildGlobalNameTable(TOCValuePtr theValue)
{
  GlobalNamePtr  g, newGlobalName;
  Boolean        dup;
  unsigned int   valueLen;
  CMCount				offset;
  TOCValueHdrPtr theValueHdr = theValue->theValueHdr;
  ContainerPtr   container   = theValueHdr->container;

  USE_TOC_FORMAT_1_ALTERNATIVE(buildGlobalNameTable,(theValue));

  /* Extract the global name container offset and its length from the TOC value...      */

  offset   = theValue->value.notImm.value;
  omfsTruncInt64toUInt32(theValue->value.notImm.valueLen, &valueLen);	/* !!! */

  /* Allocate a new global name symbol table entry.  Failure is disastrous!             */

  if ((newGlobalName = (GlobalNamePtr)CMmalloc(container, sizeof(GlobalName) + valueLen - 1)) == NULL) {
    ERROR1(container,CM_err_NoGNameLoad, CONTAINERNAME);
    return (false);
  }

  /* Seek to the name string and read it in.  Failures are still disastrous!            */

  CMfseek(container, offset, kCMSeekSet);

  if (CMfread(container, newGlobalName->globalName, sizeof(unsigned char), valueLen) != valueLen) {
    CMfree(container, newGlobalName);
    ERROR1(container,CM_err_BadGNameRead, CONTAINERNAME);
    return (false);
  }

  /* Since this routine is only called during TOC read in, we must seek BACK to the     */
  /* next TOC input position.  The TOC input routines assume, rightly so, that the TOC  */
  /* is contiguous, so it doesn't do seeks.  It doesn't know we did one here to get     */
  /* the global name.  So we put things back the way they should be.                    */

  CMfseek(container, cmFtellTOC(container->tocIOCtl), kCMSeekSet);

  if (*(newGlobalName->globalName + valueLen - 1) != '\0') { /* Now check the read...   */
    CMfree(container, newGlobalName);
    if((valueLen == 10) || (valueLen == 13))
    {
  	  /* JeffB 6/12/96:
  	   * We have seen a lot of problems where systems would translate ASCII <cr> info
  	   * <lf> and vice-versa.  Mac's use <cr> and Un*x uses <lf> as a line terminator.
  	   * This causes valueLen's on the global name 'omfi:Boolean'
  	   * to translate to a length of 10, which truncates the name & raises the error.
  	   */
  	  ERROR1(container,CM_err_CRSwap, CONTAINERNAME);
    }
    else
    {
  	  ERROR1(container,CM_err_NotGName, CONTAINERNAME);
    }
    return (false);
  }

  /* Enter the symbol and check for dups.  We must make sure we're not looking at a     */
  /* for a deleted value.  Of course, considering the time at which this routine is     */
  /* called, there shouldn't be any deleted entries!  But, what the hell!               */

  g = cmEnterGlobalName(container->globalNameTable, newGlobalName, &dup);

  if (dup) {                                            /* if we have a real dup...     */
    CMfree(container, newGlobalName);                              /* ...still don't need new entry*/
    ERROR2(container,CM_err_DupGlobalName, g->globalName, CONTAINERNAME); /* yell                 */
    return (false);                                     /* but continue if error returns*/
  }

  /* We're now happy with what we got! Set the value to point to the entry but retain   */
  /* the offset for the eventual write out to the container.  Set the flags accordingly.*/
  /* Echo flags in the value hdr.  Set the back pointer from the global name entry to   */
  /* the value.                                                                         */

  theValue->value.globalName.globalNameSymbol = g;
  theValue->value.globalName.offset           = offset;
  theValue->flags                             = kCMGlobalName;

  newGlobalName->theValue = theValue;                   /* set back pointer             */

  /* Echo the flags in the value header for this global name value.  Also set the size. */

  (void)cmSetTOCValueHdrFlags(theValueHdr, kCMGlobalName);
  omfsCvtUInt32toInt64(valueLen, &theValueHdr->size);    /* this includes null at end    */

  return (true);                                        /* global name entry now built  */
}


/*-------------------------------------------------------------*
 | cmReadTOC - read in a TOC from a already existing container |
 *-------------------------------------------------------------*

 This routine is called to read in a TOC from a container which has been opened for input
 by a call to CMOpenContainer().  The container, offset to TOC, and the size of the TOC
 are passed.  The function returns true if successfully loaded and false otherwise (this
 is a safety, since error reports should never return from the error handler).

 Note, the entire TOC whose size is defined by tocSize will be read.  It is an error if
 not all the bytes are read.  This means that it is expected that tocSize be the EXACT
 amount to read from tocOffset.  This is emphasized because, for updating, two TOCs must
 be read; the private TOC, and the non-private (updating) TOC.  The private offset and
 size are defined in the container label (by definition).  For non-updating containers,
 this will reflect the entire (only) TOC.  The non-private (updating) portion is defined
 by a TOC #1 property.
*/

Boolean cmReadTOC(ContainerPtr container, CMCount tocOffset, CMSize tocSize)
{
  TOCObjectPtr   theObject;
  TOCValueHdrPtr theValueHdr;
  TOCentry       tocEntry;
  unsigned short objectFlags;
  unsigned int   nbrOfUndefObjects, refsDataObjectID;
  char           countStr[12];
  jmp_buf        readEnv;
  CMCount				entryStart;
  unsigned int  entrySize;

  USE_TOC_FORMAT_1_ALTERNATIVE(readTOCfmt1,(container, tocOffset, tocSize));

  /* Set up current environment for jmpbuf to be restored by the TOC I/O routines if    */
  /* they detect an error.  The error will have already been reported and the TOC I/O   */
  /* control block with its buffer freed if the jump is taken.  Thus all we have to do  */
  /* here is return the bad news to the caller.                                         */

  if (setjmp(readEnv))                    /* set setjmp/longjmp environment vector      */
    return (false);                       /* ...just quit if there's a TOC input error  */

  /* Position to the start of the TOC...                                                */

  CMfseek(container, tocOffset, kCMSeekSet);

  /* Define a TOC I/O control block and buffer. The control block pointer is saved in   */
  /* the container mainly for cmBuildGlobalNameTable() to use when it defines global    */
  /* names and needs to reseek to the current TOC input position. Note, the size of the */
  /* TOC buffer required for reading this TOC is as defined in the container label.  It */
  /* was squirreled away at open time.                                                  */

  container->tocIOCtl = cmStartTOCIO(container, container->tocBufSize, (jmp_buf *)&readEnv,
                                     tocOffset, tocSize);
  if (container->tocIOCtl == NULL)
    return (false);                       /* something went wrong (too bad)!            */

  /* As object ID's are encountered during the reading we maintain a counter,           */
  /* nbrOfUndefObjects, representing the number of undefined objects.  Objects can be   */
  /* forward or backward referenced and we can have multiple references.  Thus we have  */
  /* to be carfull how we handle this counter.  A special flag in the object is used to */
  /* ensure we don't count undefined objects more than once and to not count defined    */
  /* objects.  The counter is set initially, of course, to 0...                         */

  nbrOfUndefObjects = 0;                  /* hopefully it will be 0 at the end too!     */

  /* Read in the TOC, one object definition at a time.  For each object we will get a   */
  /* object ID (what a suprise!), a property ID, and a type ID.  All three IDs are      */
  /* used to create objects.  The object ID may or may not be a property or type.  We   */
  /* don't know that here.  The property and type IDs are known to be such because of   */
  /* where they are extracted for the TOC object.  But all we know is that these are    */
  /* for property and type objects.  We don't know their full object definitions until  */
  /* we actually encounter them in the TOC load.  All this forward (and backward) ID    */
  /* referencing is handled by the object definition routine, cmDefineObject(). All we  */
  /* do here is define the IDs as objects for each ID number seen.  The property and    */
  /* type IDs are always considered as undefined objects here.  cmDefineObject() will   */
  /* handle the forward refrences behind our backs.  So here it goes...                 */

  /* Oh, by the way, when calling cmRead1TOCSegment() to read TOC entries, it will only */
  /* return false to terminate the read when ALL the TOC entries have been read (whose  */
  /* total size was passed to cmStartTOCIO() above). Hence we do not have to check that */
  /* we read it all.                                                                    */

		while (cmRead1TOCSegment(container->tocIOCtl,           /* read each entry in TOC...  */
                           &tocEntry.objectID, &tocEntry.propertyID, &tocEntry.typeID,
                           &tocEntry.value, &tocEntry.value.notImm.valueLen,
                           &tocEntry.generation, &tocEntry.flags, &refsDataObjectID,
                           &entryStart,
                           &entrySize)) {

    /* Define the object.  If the property ID is for a the standard type or property    */
    /* global name, the flags are set accordingly.  Everything else is treated as a     */
    /* plain object. It may turn out to be a property or type, but it is not in terms   */
    /* of a global name.  Also see comments above.                                      */

    if (tocEntry.propertyID == CM_StdObjID_GlobalTypeName)
      objectFlags = TypeObject;
    else if (tocEntry.propertyID == CM_StdObjID_GlobalPropName)
      objectFlags = PropertyObject;
    else
      objectFlags = ObjectObject;

#ifdef VIRTUAL_BENTO_OBJECTS
		/* JeffB: Added code to support virtual objects
		 */
	  if(!container->objectsMayBeVirtual || (objectFlags != ObjectObject))
	  {
#endif
	  	theObject = cmDefineObject(container, tocEntry.objectID, tocEntry.propertyID,
	                               tocEntry.typeID, &tocEntry.value, tocEntry.generation,
	                               tocEntry.flags, objectFlags, &theValueHdr);
#ifdef VIRTUAL_BENTO_OBJECTS
	  }
	  else
	  {
		  theObject = cmIntDefineObject(container, tocEntry.objectID, tocEntry.propertyID,
	                               tocEntry.typeID, &tocEntry.value, tocEntry.generation,
	                               tocEntry.flags, objectFlags,
	                               	LoadAsStub, entryStart,entrySize,
																	&theValueHdr);
	  }
#endif
    if (theObject == NULL) return (false);

    /* If this object was previously counted as undefined, reduce the count...          */

    if ((theObject->objectFlags & UndefObjectCounted) != 0) {
      theObject->objectFlags &= ~UndefObjectCounted;
      --nbrOfUndefObjects;
    }
	  if (tocEntry.typeID != CM_StdObjID_ObjRefData)    /* if not a recording value...*/
	    cmLinkObject(container->toc, theObject, NULL);      /* ...link it to master chains*/

     /* Link the object to its appropriate "master" chains.  The TOC is always sorted by */
     /* ascending object ID's.  Thus we know that we are linking these things in order.  */
     /* Note that type and property global names are ALWAYS marked as such by their      */
     /* property ID (we used that fact to set to flags above).  Thus these will go to    */
     /* their appropriate master chains.  See cmLinkObject() for details on the master   */
     /* chain links.  By the way, the object is linked only once on their respective     */
     /* chains.                                                                          */

	   /* If the object we just defined is a value's private recording object for a value's*/
	   /* references, then we want to protect it (object and its value header) from user   */
	   /* fiddling just to be safe.  We also don't want it linked on to the master chains. */
	   /* That makes it truly private.  The primary "hook" to this object is from a user   */
	   /* value header that contains data with the reference keys.  These keys are used to */
	   /* get their associated object (ID)  and hence the referenced object itself.  It is */
	   /* these associations that are recorded as a list in the recording object's value   */
	   /* data.  The pointer from the user's value to the private object we just defined.  */
	   /* The pointer will be or already is set when the user's value is picked up while   */
	   /* reading.  This is discussed below where the refsDataObjectID returned from the   */
	   /* cmRead1TOCSegment() is processed.                                                */


#ifdef VIRTUAL_BENTO_OBJECTS
	  if(!container->objectsMayBeVirtual || (objectFlags != ObjectObject))
#endif
		{
			ObjectDefCleanup(container, theObject, tocEntry, &nbrOfUndefObjects,
											refsDataObjectID, theValueHdr);
    } /* fwd ref recording object */
  } /* while */

  /* We're done reading.  Free the TOC I/O control block and its buffer and report an   */
  /* error if there are any undefined objects...                                        */
  (void)cmEndTOCIO(container->tocIOCtl);
  container->tocIOCtl = NULL;

  if (nbrOfUndefObjects > 0) {
    ERROR2(container,CM_err_UndefObjects, cmltostr(nbrOfUndefObjects, 1, false, countStr), CONTAINERNAME);
    return (false);
  }

  return (true);
}


/*--------------------------------------------------------------------------*
 | writeTOCValue - write a TOC container entry for a value to the container |
 *--------------------------------------------------------------------------*

 This is used as the "action" parameter to cmWalkThroughEntireTOC() to handle values while
 writing TOC entries to the container.  The refCon here is a pointer to a communication
 area (type WriteTOCCommArea) which contains the TOC container entry we have been building
 up with writeTOCObject(), writeTOCProperty(), and writeTOCValueHdr().  They were called
 as cmWalkThroughEntireTOC() worked its way through the TOC links.

 writeTOCValue() is the deepest call in the walk. It is here we add the value, flags, and
 generation info to the TOC entry and write it to the container.

 In addition to the TOC entry we are building, the communication area also contains the
 current total TOC size and offset.  These are updated as we write.

 Note, we assume the proper positioning here as initiated by cmWriteTOC() which called
 cmWalkThroughEntireTOC() to start this thing going.  We update the offset assuming it is
 in sync with the writing.

 Note, this "static" is intentionally left to default memory model under DOS since it is
 passed as a function pointer to cmWalkThroughEntireTOC().
*/

static TOCWalkReturns writeTOCValue(ContainerPtr container, TOCValuePtr theValue, CMRefCon refCon)
{
  WriteTOCCommAreaPtr t = (WriteTOCCommAreaPtr)refCon;
  unsigned int        propertyID;
  CMCount						offset;

  /* Don't write values for standard properties except for the TOC object itself.  To   */
  /* give us some latitude here, we allow writing of standard properties if their       */
  /* property ID's are less than or equal to CM_StdObjID_Writable.  There may be some   */
  /* private properties we might want written.  Using an upper limit means we don't     */
  /* have to change the following code every time.  Note that the initial test here is  */
  /* on the object ID.  If we have a user object we ALWAYS write all its properties,    */
  /* predefined or not.                                                                 */

  if (t->tocEntry.objectID >= CM_StdObjID_TOC && t->tocEntry.objectID < MinUserObjectID) {
    propertyID = theValue->theValueHdr->theProperty->propertyID;
    if (propertyID > CM_StdObjID_Writable && propertyID < MinUserObjectID)
      return (WalkNextTOCValueHdr);
  }

  /* Don't write dynamic values...                                                      */

  if (t->tocEntry.propertyID == CM_StdObjID_DynamicValues) return (WalkNextTOCValueHdr);

  /* If we have a global name symbol table entry pointer for the value, then we assume  */
  /* that by this time the global names have been written to the TOC and the offset     */
  /* placed in the value data.  We have to use the global name pointer to get the length*/
  /* of the string.                                                                     */

  if (theValue->flags & kCMGlobalName) {
    t->tocEntry.value.notImm.value    = theValue->value.globalName.offset;
    omfsCvtUInt32toInt64(GetGlobalNameLength(theValue->value.globalName.globalNameSymbol) + 1, &t->tocEntry.value.notImm.valueLen);
    t->tocEntry.flags                 = 0;              /* std value in the container   */
  } else {                                              /* normal values handled here...*/
    t->tocEntry.value = theValue->value;
    t->tocEntry.flags = theValue->flags;                /* set flags as is              */
  }

  /* Write the TOC entry to the container.  The offset returned from this write is to   */
  /* where the value  is placed in the container.  We use this if the entry requires    */
  /* back patching.                                                                     */

  offset = cmWrite1TOCSegment(container->tocIOCtl,
                              t->tocEntry.objectID,
                              t->tocEntry.propertyID,
                              t->tocEntry.typeID,
                              t->tocEntry.value.notImm.value,
                              t->tocEntry.value.notImm.valueLen,
                              t->tocEntry.generation,
                              t->tocEntry.flags,
                              t->refDataObject);

  /* Some TOC values are NOT known at this time.  We must remember where they are and   */
  /* back patch them later after we do know.  The following values currently require    */
  /* back patching (all of these belong to TOC #1):                                     */

  /*    1. The TOC size.                                                                */
  /*    2. The total size of the container including the TOC.                           */
  /*    3. The non-private portion of an updating TOC.                                  */

  if (t->thePatches != NULL)                            /* if patches are required...   */
    if (theValue == container->tocObjValue) {           /* ...remember appropriate info */
      t->thePatches->tocSizeEntry = offset;             /*    1. TOC size               */
      t->thePatches->whatWeGot |= Patch_TOCsize;
    } else if (theValue == container->tocContainerValue) {
      t->thePatches->tocContEntry = offset;             /*    2. entire container       */
      t->thePatches->whatWeGot |= Patch_TOCContSize;
    } else if (theValue == container->tocNewValuesValue) {
      t->thePatches->tocNewValuesTOCEntry = offset;     /*    3. non-private TOC        */
      t->thePatches->whatWeGot |= Patch_TOCNewValuesTOC;
    }

  return (WalkNextTOCValue);                            /* continue walking value segs  */
}


/*------------------------------------------------------------------*
 | writeTOCValueHdr - build up a TOC container entry with a type ID |
 *------------------------------------------------------------------*

 This is used as the "action" parameter to cmWalkThroughEntireTOC() to handle value header
 structs while writing TOC entries to the container.  The refCon here is a pointer to a
 communication area (type WriteTOCCommArea) which contains the TOC container entry we are
 building up in preparation for writing when a value is reached.  All we need do here is
 set the type ID in the entry.  cmWalkThroughEntireTOC() will next call writeTOCValue()
 which is where we will do the actual writing of the TOC entry we have been building up
 with this routine, writeTOCProperty(), and writeTOCObject() below.

 Note, this "static" is intentionally left to default memory model under DOS since it is
 passed as a function pointer to cmWalkThroughEntireTOC().
*/

static TOCWalkReturns writeTOCValueHdr(ContainerPtr container, TOCValueHdrPtr theValueHdr, CMRefCon refCon)
{
  WriteTOCCommAreaPtr t = (WriteTOCCommAreaPtr)refCon;
  TOCValueBytes       value;
  char                idStr[15];
  ContainerPtr        unused = container;
  unsigned int             minimumSeed;

  /* Only write TOC entries for the current container.  Values can be marked as         */
  /* belonging to other (i.e, updating target) containers.  We don't want those.        */

  if (theValueHdr->container != container)                /* if not current container...*/
    return (WalkNextTOCValueHdr);                         /* ...suppress writing value  */

  /* If we are looking at the special property for a dynamic value, then we have an     */
  /* outstanding dynamic value that has not been released with a CMReleaseValue().  It  */
  /* is required that dynamic values be released COMPLETELY.  That causes the special   */
  /* property containing the dynamic values for the object to be removed.  We would     */
  /* then not see them here.  If we do, it's an error...                                */

  /* Well, not always an error!  We "cheat" a little here.  As it turns out we may be   */
  /* use a dynamic value as a pointing value to a target we're updating.  Its one we    */
  /* created that the user knows nothing about.  It WILL (trust me) be released, just   */
  /* not at the time we're writing this TOC.  So we want to ignore that dynamic value.  */
  /* We can detect we got it because we create its type before we have the target       */
  /* container (we need it to get the target, so it's a "chicken-and-egg" problem). Such*/
  /* types will have type IDs less than the min seed value.  That's why we keep a min   */
  /* seed.  So we can tell what objects "belong" to the updating container and what     */
  /* objects are updates.                                                               */

  if (t->tocEntry.propertyID == CM_StdObjID_DynamicValues) {/* if a dynamic value...    */
		omfsTruncInt64toUInt32(container->tocIDMinSeedValue->value.imm.value, &minimumSeed);
    if (theValueHdr->typeID >= minimumSeed)                     /* ...that's >= MinSeed...  */
      ERROR1(container,CM_err_NotReleased, CONTAINERNAME);            /* ...why wasn't it released*/
    return (WalkNextTOCValueHdr);                           /* ...suppress writing value*/
  }

  t->tocEntry.typeID     = theValueHdr->typeID;           /* set type ID                */
  t->tocEntry.generation = theValueHdr->generation;       /* set generation             */

  t->refDataObject       = HasRefDataObject(theValueHdr) ? RefDataObject(theValueHdr)
                                                         : NULL;

  /* Make sure that this valueHdr does indeed have a value. When a CMNewValue() is done */
  /* an object is created but the value header contains no value list.  At that point   */
  /* if any of these are still lying around we will see them here. It represents        */
  /* an object that was never given a value.  That's a no, no...                        */

  if (cmIsEmptyList(&theValueHdr->valueList)) {           /* error if no value list...  */
    ERROR3(container,CM_err_NoValue, cmltostr(theValueHdr->theProperty->theObject->objectID, 1, false, idStr),
                           cmGetGlobalPropertyName(container, t->tocEntry.propertyID),
                           CONTAINERNAME);
    (void)cmSetImmValueBytes(container, &value, Value_NotImm, 0xFFFFFFFFU, 0); /* -1 offset*/
    if (cmAppendValue(theValueHdr, &value, 0) == NULL)
      AbortWalkThroughEntireTOC(CM_err_NoValue);          /* abort if this fails (damit)*/
  }
  #if 0
    else if (theValueHdr->dynValueData.dynValue != NULL)  /* safety for dynamic values  */
      theValueHdr->dynValueData.dynValue = NULL;
  #endif

  return (WalkNextTOCValue);                              /* walk the value segments    */
}


/*----------------------------------------------------------------------*
 | writeTOCProperty - build up a TOC container entry with a property ID |
 *----------------------------------------------------------------------*

 This is used as the "action" parameter to cmWalkThroughEntireTOC() to handle property
 structs while writing TOC entries to the container.  The refCon here is a pointer to a
 communication area (type WriteTOCCommArea) which contains the TOC container entry we are
 building up in preparation for writing when a value is reached.  All we need do here is
 set the property ID in the entry.  cmWalkThroughEntireTOC() will next call
 writeTOCValueHdr() to do a similar stunt for the type ID.

 Note, this "static" is intentionally left to default memory model under DOS since it is
 passed as a function pointer to cmWalkThroughEntireTOC().
*/

static TOCWalkReturns writeTOCProperty(ContainerPtr container, TOCPropertyPtr theProperty, CMRefCon refCon)
{
  WriteTOCCommAreaPtr t = (WriteTOCCommAreaPtr)refCon;
  ContainerPtr        unused = container;

  t->tocEntry.propertyID = theProperty->propertyID;

  return (WalkNextTOCValueHdr);                           /* walk value headers         */
}


/*------------------------------------------------------------------*
 | writeTOCObject - build up a TOC container entry with a object ID |
 *------------------------------------------------------------------*

 This is used as the "action" parameter to cmWalkThroughEntireTOC() to handle object
 structs while writing TOC entries to the container.  The refCon here is a pointer to a
 communication area (type WriteTOCCommArea) which contains the TOC container entry we are
 building up in preparation for writing when a value is reached.  All we need do here is
 set the value ID in the entry.  cmWalkThroughEntireTOC() will next call writeTOCProperty()
 to do a similar stunt for the property ID.

 Note, this "static" is intentionally left to default memory model under DOS since it is
 passed as a function pointer to cmWalkThroughEntireTOC().
*/

static TOCWalkReturns writeTOCObject(ContainerPtr container, TOCObjectPtr theObject, CMRefCon refCon)
{
  WriteTOCCommAreaPtr t = (WriteTOCCommAreaPtr)refCon;
  ContainerPtr        unused = container;

  t->tocEntry.objectID = theObject->objectID;

  return (WalkNextTOCProperty);                           /* walk properties            */
}


/*-----------------------------------------*
 | cmWriteTOC - write a TOC to a container |
 *-----------------------------------------*

 This routine is the inverse to cmReadTOC() to write the in-memory TOC to a container. The
 TOC is written to the end of the container.  Only objects with ID's greater than or equal
 to the startingID and less than or equal to the endingID are written.  Further, only
 objects whose value headers are equal to the specified container are written.

 With the restriction on the container, we will only write to the TOC newly created values
 for this open session.  If we're not updating, only one container so this is not an
 issue.  But for updating, only looking at the stuff in the updating container will yield
 all new objects and all new properties for "old" objects.  We can split these two groups
 using the ID limits.

 Note, non-TOC updates are generated by a separate walk not done here.  See Updating.c.

 For updating, a container may have its own TOC and the target TOC.  Thus the TOC pointer
 is an explicit parameter.  The container offset is returned in tocStart and the size of
 the TOC in tocSize.  The function returns true to indicate success and false otherwise
 (as a safety).

 There are some TOC entries that must be back-patched. This is because they are a function
 of the final TOC size in the container.  We don't know that until the entire TOC is
 written.  At that time we can rewrite the entries with the proper values filled in.

 Because of updating, there may be multiple calls to cmWriteTOC() to write various subsets
 of the TOC (i.e., the updates).  Thus we may not know the final TOC size when we return
 from here.  The back-patching cannot be done.  Only the caller knows when all TOC writes
 are complete and the back-patching can be done.

 To allow this to happen, thePatches is passed as a pointer to a BackPatches struct where
 we save the original unpatched TOC info and offset for the entries of interest.  As these
 entries are encountered (and they must be sometime) during TOC writing, we save them and
 their offsets in the struct. When the caller is ready to rewrite them cmDoBackPathes() is
 called to do it.

 If thePatches is passed as NULL, nothing (obviously) is saved for back-patching.
*/

Boolean cmWriteTOC(ContainerPtr container, void *toc, CMObjectID startingID,
                   CMObjectID endingID, BackPatchesPtr patches,
                   CMCount *tocStart, CMSize *tocSize)
{
  WriteTOCCommArea tocCommArea;
  int              x;
  CMCount    offset, logicalEOF0, logicalEOF, slopSize, truncVal;
  unsigned char    padding[5], *p;
  jmp_buf          writeEnv;
  NewBackPatchesPtr thePatches = &(patches->newBP);
  omfInt64				zero;
  CMSize					physSize, labelSiz;

   omfsCvtUInt32toInt64(0, &zero);
  /* OMFI version 1.0 compatibility!!  If set for writing version 1.0-style TOC, call   */
  /* old write																			*/
  if (container->writeVersion1TOC)
  	return(cmOldWriteTOC(container, toc, startingID, endingID, patches, tocStart, tocSize));

  /* Set up current environment for jmpbuf to be restored by the TOC I/O routines if    */
  /* they detect an error.  The error will have already been reported and the TOC I/O   */
  /* control block with its buffer freed if the jump is taken.  Thus all we have to do  */
  /* here is return the bad news to the caller.                                         */

  if (setjmp(writeEnv))                   /* set setjmp/longjmp environment vector      */
    return (false);                       /* ...just quit if there's a TOC output error */

  /* Set the "seed" user object ID value in the special TOC entry.  This is picked up   */
  /* when the container is opened for reading to allow us to continue on using unique   */
  /* object ID numbers.  The pointer to the value for the special TOC entry was set when*/
  /* the container was opened.                                                          */

  omfsCvtUInt32toInt64(container->nextUserObjectID, &container->tocIDSeedValue->value.imm.value);

  /* We are now ready to write the TOC. Except for space reuse we always append the TOC */
  /* to the end of the container (i.e., at the physical EOF).  But because of space     */
  /* reuse we may have to write the TOC twice!  Let me try to explain...                */

  /* The reason is that with space reuse we start the TOC at the logical EOF rather     */
  /* than the physical EOF if it is less than the physical EOF.  CMOpenContainer() sets */
  /* the logical EOF to the start of the "old" TOC for space reusing updates and the    */
  /* "old" TOC is redefined as reusable data free space.  That, along with possibly     */
  /* already exiting free space, may be enough to cover all the updates.  Then we can   */
  /* start writing the new TOC at the logical EOF to minimize growth of the container.  */
  /* If it isn't, we append the TOC to the end of the container as usual.               */

  /* Note, that the container can NEVER get smaller in this "pure" portable             */
  /* implementation of the Container Manager.  The reason is that we assume the I/O     */
  /* handlers are using standard ANSI stream I/O.  It provides NO facility for cutting  */
  /* a stream back!  This is important and the reason there are two potential writes of */
  /* the TOC.                                                                           */

  /* If we indeed do start writing the TOC at the logical EOF, it is possible that the  */
  /* new TOC is small enough that we don't write enough entries to reach the physical   */
  /* EOF.  We then have a bunch of space ("slop") from the end of the new TOC to the    */
  /* physical EOF to account for.  The container looks something like this at this      */
  /* point (the label is not yet written at this point, but the physical EOF would      */
  /* follow the old label):                                                             */

  /*              *------------------*-----------*----------*- - - - - - -*             */
  /*   container: |<----- data ----->|<-- TOC -->|<- slop ->|    label    |physical eof */
  /*              *------------------*-----------*----------* - - - - - - -*            */

  /* We cannot leave the slop where it is because we always write the container label   */
  /* immediately following the TOC.  Thus we must "move" the TOC so that we have the    */
  /* following configuration:                                                           */

  /*              *------------------*----------*-----------*- - - - - - -*             */
  /*   container: |<----- data ----->|<- slop ->|<-- TOC -->|    label    |physical eof */
  /*              *------------------*----------*-----------*- - - - - - -*             */

  /* Unfortunately, there is no easy way to determine the TOC size until we write it, so*/
  /* there is no way to know the slop ahead of time.  We could do all the same logic as */
  /* we do to write it and suppress the writing.  But who knows?  We could get lucky    */
  /* and have no slop.  So we write the TOC to see if the slop occurs.  This is TOC     */
  /* write number 1.                                                                    */

  /* Now if there indeed is slop, we do the "move" by doing the TOC write the second    */
  /* time (hence the two possible writes).  The slop is first moved to the free list.   */
  /* This means the second TOC write will be slightly different (and one TOC entry      */
  /* larger).  This implies that the copy must be done as a second TOC write rather     */
  /* than by some other means (e.g., like copying the TOC as raw bytes down).           */

  /* The following will help define the variables we use for this so you can understand */
  /* the math:                                                                          */

  /*                            LogicalEOF0                 x        physicalEOF        */
  /*                                 |                      |             |             */
  /*              *------------------*-----------*----------*- - - - - - -*             */
  /*   container: |<----- data ----->|<-- TOC -->|<- slop ->|    label    |             */
  /*              *------------------*-----------*----------*- - - - - - -*             */
  /*                                             |                                      */
  /*                                         LogicalEOF     |<- LBLsize ->|             */

  /*  size of slop = (physicalEOF - LBLsize) - LogicalEOF  (takes label into account)   */

  /* For the second TOC write to happen, the logical EOF after the first TOC write must */
  /* be less then "x" (= physicalEOF - LBLsize).  The slop is added to the free list    */
  /* with a starting offset of LogicalEOF0.  LogicalEOF is then moved back to           */
  /* LogicalEOF0 + slop size.  The second TOC write will thus end at "x" (or one TOC    */
  /* entry further if the slop was placed uniquely on the free list).                   */

  /* Now, having described all this crap, there is a possibility you can forget it!     */
  /* Remember, that the reason for all of this is due to the limitations of standard    */
  /* ANSI C I/O.  Specific implementations may have library (or whatever) support for   */
  /* cutting a stream back. If we could cut it back, we would cut it back to the logical*/
  /* EOF and write the TOC to the new physical EOF which now equals the logical EOF.    */

  /* So, on the possibility that such a support exists, a special handler, "trunc", may */
  /* be defined to "truncate" the stream.  We test for its presence here. If it exists, */
  /* we call it and "trust" it to do the truncation. Then we only have to do the single */
  /* TOC write.                                                                         */

  /* Finally, one last point to keep in mind about all this.  Even if the trunc handler */
  /* is not supplied, the probability of the two writes is, I think, fairly small!  The */
  /* reason comes from the fact of noting how you can get a smaller TOC in the first    */
  /* place.  The only way it can happen is to free values.  That reduces the number of  */
  /* TOC entries.  But they are replaced by free list entries.  So, unless free space   */
  /* coalesces (which it could do), there will be no net loss of TOC entries and hence  */
  /* the TOC will not get smaller.  So what's the probability of separate values data   */
  /* space coalescing?  That is basically the probability of the TOC growing smaller.   */

  /* Whew!                                                                              */

  logicalEOF0 = container->logicalEOF;              /* save original logical EOF        */

  for (;/*two possible tries*/;) {

    /* Seek to the start of where we want to put the TOC...                             */

    offset = CMgetContainerSize(container);         /* this is the next free byte       */
    if (omfsInt64GreaterEqual(container->logicalEOF, offset))    /* if we wrote up to this point...  */
      CMfseek(container, zero, kCMSeekEnd);            /* ...append TOC to end of container*/
    else if (container->handler.cmftrunc != NULL && /* if truncation handler provided...*/
             CMftrunc(container, container->logicalEOF)) {/* ...and it trucated containr*/
      offset = CMgetContainerSize(container);       /* ...get new truncated size        */
      container->logicalEOF = offset;               /* ...logical EOF == physical now   */
      CMfseek(container, zero, kCMSeekEnd);            /* ...position to EOF               */
      cmDeleteFreeSpace(container, offset);         /* ...kill free list entries >= EOF */
    } else {                                        /* if we didn't use all (old) space */
      offset = container->logicalEOF;               /* ...overwrite from logical EOF    */
      CMfseek(container, offset, kCMSeekSet);       /* ...position to EOF               */
      cmDeleteFreeSpace(container, offset);         /* ...kill free list entries >= EOF */
    }

    /* Pad the container to align it on a multiple of 4 boundary. We fill the pad bytes */
    /* with 0xFF so they are easy to spot during debugging.                             */

    omfsCvtUInt32toInt64(0, tocSize);                /* init size                        */
		*tocStart = offset;
		omfsAddInt32toInt64(3, tocStart);
		omfsMakeInt64(~0x0, ~0x03, &truncVal);
		omfsInt64AndInt64(truncVal, tocStart);          /* round start up to multiple of 4  */

    if (omfsInt64Less(offset, *tocStart)) {         /* if padding is necessary...       */
      p = padding;                                  /* ...create the padding of FF's    */
      do
      {
        *p++ = 0xFF;
        omfsAddInt32toInt64(1, &offset);
      } while (omfsInt64Less(offset, *tocStart));
      x = p - padding;
      if ((int)CMfwrite(container, padding, sizeof(char), x) != x) { /*write the padding*/
        ERROR1(container,CM_err_BadTOCWrite, CONTAINERNAME);
        return (false);
      }
    }

    container->tocOffset = offset;                  /* announce where TOC starts        */

    /* Define a TOC I/O control block and buffer. The control block pointer is saved in */
    /* the container mainly for the TOC walk routines to use when it comes time to write*/
    /* a TOC entry.                                                                     */

    container->tocIOCtl = cmStartTOCIO(container, container->tocBufSize, (jmp_buf *)&writeEnv,
                                       offset, zero);
    if (container->tocIOCtl == NULL)
      return (false);                               /* something went wrong (too bad)!  */

    /* Walk the entire TOC building up and writing each TOC container entry...          */

    tocCommArea.thePatches = thePatches;

    x = cmWalkThroughEntireTOC(container, toc, startingID, endingID, &tocCommArea,
                               writeTOCObject, writeTOCProperty, writeTOCValueHdr,
                               writeTOCValue);
    if (x != 0) return (false);                             /* something went wrong!    */

    /* Flush the last TOC buffer and return TOC size to caller...                                                     */

    *tocSize = container->tocSize = cmFlushTOCOutputBuffer(container->tocIOCtl, true);

    /* We're done writing.  Free the TOC I/O control block and its buffer.              */

    (void)cmEndTOCIO(container->tocIOCtl);
    container->tocIOCtl = NULL;

    container->physicalEOF = CMgetContainerSize(container); /* update physical EOF      */
    logicalEOF = *tocStart;
    omfsAddInt64toInt64(*tocSize, &logicalEOF);            /* ...and the logical EOF   */
    SetLogicalEOF(logicalEOF);

    /* If we write up to the physical EOF, there is no slop, so we're done...           */

		omfsCvtUInt32toInt64(LBLsize, &labelSiz);
		physSize = container->physicalEOF;
    omfsSubInt64fromInt64(labelSiz, &physSize);
    if (omfsInt64GreaterEqual(container->logicalEOF, physSize)) break;

    /* Damn, we got some slop!  Put it on the free list with the original logical EOF as*/
    /* its position.  The set the logical EOF past it and loop around for the second    */
    /* TOC write.  That is guaranteed to reach the physical EOF (it better!) so we will */
    /* break out of the loop with the statement above.                                  */

    slopSize = physSize;
    omfsSubInt64fromInt64(container->logicalEOF, &slopSize);
    cmAddToFreeList(container, NULL, logicalEOF0, slopSize);/* put slop on free list    */
    container->logicalEOF = logicalEOF0;
    omfsAddInt64toInt64(slopSize, &container->logicalEOF);   /* back up logicalEOF       */
  } /* for (;two possible tries;) */                        /* try, try, again...       */

  return (true);                                            /* TOC successfully written!*/
}


/*-------------------------------------*
 | cmDoBackPathes - back-patch the TOC |
 *-------------------------------------*

 When a TOC is written (e.g., cmWriteTOC()), there are some entries that are dependent on
 the final size of the TOC.  Such entries must be back-pached with the proper info.  Only
 the callers generating the TOC in the container know when it is time to do the patching.
 They call this routine to do it when that time comes.

 True is returned if the back-patching is successful and false otherwise.

 By the time this routine is called, the original TOC entries in question and their
 offsets within the container have been recorded in a BackPatches struct whose pointer is
 passed in thePatches.  Currently only 3 such entries need back-patching:

 tocSizeEntry          -  The TOC size entry.  This is a property of TOC #1 which
                          represents the size and offset of the TOC itself.  The caller
                          passes this info in the tocSize and tocStart parameters
                          respectively.

 tocContEntry          -  The TOC container entry. This is a property of TOC #1 which
                          represents the entire container, from offset 0 to the end of the
                          label.  No additional parameters are passed for this.  But to
                          compute this, it is required that cmDoBackPathes() be the LAST
                          thing called prior to writing the container label.

 tocNewValuesTOCEntry -  The non-private TOC offset/size entry.  An updating TOC contains
                         a TOC #1 property that defines the offset and size of all TOC
                         entries that are to be applied to this container's target.  The
                         caller passes this info in the newValuesTOCSize and
                         newValuesTOCStart parameters respectively.  For non-updating
                         containers, these are ignored.
*/

Boolean cmDoBackPatches(ContainerPtr container, BackPatchesPtr patches,
                        CMCount tocStart, CMSize tocSize,
                        CMCount newValuesTOCStart, CMSize newValuesTOCSize)
{
  CMSize 		size;
  CMCount		offset;
  CMCount32	offset32;
  CMSize32	size32;
  unsigned char valueBuffer[8];
  NewBackPatchesPtr thePatches = &(patches->newBP);

  /* OMFI version 1.0 compatibility!!  If set for writing version 1.0-style TOC, call   */
  /* old write																			*/
  if (container->writeVersion1TOC)
  	return(cmOldDoBackPatches(container, patches, tocStart, tocSize, newValuesTOCStart,
  							  newValuesTOCSize));

  /* Back-patch the TOC size entry, i.e., the entry that defines the offset and size of */
  /* the TOC itself...                                                                  */

  if ((thePatches->whatWeGot & Patch_TOCsize) == 0) {       /* must have gotten offset! */
    ERROR1(container,CM_err_Internal2, CONTAINERNAME);
    return (false);
  }

  omfsTruncInt64toUInt32(tocStart, &offset32);
  omfsTruncInt64toUInt32(tocSize, &size32);
  CMformatData(container, &valueBuffer[0], 4, &offset32);
  CMformatData(container, &valueBuffer[4], 4, &size32);
  CMfseek(container, thePatches->tocSizeEntry, kCMSeekSet);
  if (CMfwrite(container, valueBuffer, sizeof(unsigned char), 8) != 8) {
    ERROR1(container,CM_err_BadTOCWrite, CONTAINERNAME);
    return (false);
  }

  /* Back-patch the TOC updates TOC entry, i.e., the entry that defines the offset and  */
  /* size of that portion of an updating container's TOC to be applied as updates to its*/
  /* target.  Note, this entry only exists in updating containers and we only get into  */
  /* this function it it's a new updating container.                                    */

  if (UPDATING(container)) {                                  /* if expecting patch...  */
    if ((thePatches->whatWeGot & Patch_TOCNewValuesTOC) == 0){/*must have gotten offset!*/
      ERROR1(container,CM_err_Internal5, CONTAINERNAME);
      return (false);
    }

  	omfsTruncInt64toUInt32(newValuesTOCStart, &offset32);
  	omfsTruncInt64toUInt32(newValuesTOCSize, &size32);
    CMformatData(container, &valueBuffer[0], 4, &offset32);
    CMformatData(container, &valueBuffer[4], 4, &size32);
    CMfseek(container, thePatches->tocNewValuesTOCEntry, kCMSeekSet);
    if (CMfwrite(container, valueBuffer, sizeof(unsigned char), 8) != 8) {
      ERROR1(container,CM_err_BadTOCWrite, CONTAINERNAME);
      return (false);
    }
  }

  /*               ********** The following must be done last! **********               */

  /* Back-patch the TOC container entry, i.e., the entry that defines the size of the   */
  /* entire container, including its label.  This entry must be done last and this      */
  /* routine must be called just prior to writing the container label.  The label is    */
  /* assumed to always immediatly follow the TOC so, even though it is not yet written, */
  /* we can take it into account in setting the size field.                             */

  if ((thePatches->whatWeGot & Patch_TOCContSize) == 0) {   /* must have gotten offset  */
    ERROR1(container,CM_err_Internal3, CONTAINERNAME);
    return (false);
  }

  omfsCvtUInt32toInt64(0, &offset);
  size   = ((container->embeddedValue) ? CMGetValueSize((CMValue)container->embeddedValue)
                                                 : container->physicalEOF);
  omfsAddInt32toInt64(LBLsize, &size);

  omfsTruncInt64toUInt32(offset, &offset32);
  omfsTruncInt64toUInt32(size, &size32);
  CMformatData(container, &valueBuffer[0], 4, &offset32);
  CMformatData(container, &valueBuffer[4], 4, &size32);
  CMfseek(container, thePatches->tocContEntry, kCMSeekSet);
  if (CMfwrite(container, valueBuffer, sizeof(unsigned char), 8) != 8) {
    ERROR1(container,CM_err_BadTOCWrite, CONTAINERNAME);
    return (false);
  }

  return (true);                                            /* success!                 */
}

                              CM_END_CFUNCTIONS
