/***********************************************************************
 *
 *              Copyright (c) 1996 Avid Technology, Inc.
 *
 * Permission to use, copy and modify this software and to distribute
 * and sublicense application software incorporating this software for
 * any purpose is hereby granted, provided that (i) the above
 * copyright notice and this permission notice appear in all copies of
 * the software and related documentation, and (ii) the name Avid
 * Technology, Inc. may not be used in any advertising or publicity
 * relating to the software without the specific, prior written
 * permission of Avid Technology, Inc.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL AVID TECHNOLOGY, INC. BE LIABLE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT, CONSEQUENTIAL OR OTHER DAMAGES OF
 * ANY KIND, OR ANY DAMAGES WHATSOEVER ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE, INCLUDING,
 * WITHOUT  LIMITATION, DAMAGES RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, AND WHETHER OR NOT ADVISED OF THE POSSIBILITY OF
 * DAMAGE, REGARDLESS OF THE THEORY OF LIABILITY.
 *
 ************************************************************************/

#ifndef _OMF_NEWMEMFILE_
#define _OMF_NEWMEMFILE_ 1

#include "omErr.h"

#define OMFSTREAM_GOOD_MAGIC 0xdebeaded
#define OMFSTREAM_BAD_MAGIC  0xdeadbeef

typedef enum { kOmfStreamStatNULL, kOmfStreamReadOnly, kOmfStreamAppend, kOmfStreamCreate } omfStreamStatus_t;
typedef enum { kOmfStreamTypeNULL, kOmfStreamFile, kOmfStreamMemory } omfStreamType_t;

typedef struct {

   unsigned int  magic;        /* for memory integrity assertion */
	omfStreamType_t streamType; /* is this a file stream or memory stream? */
	char *streamPtr;			/* pointer to memory buffer */
	int  streamSize;			/* size of memory buffer */
	FILE *fPtr;					/* for kOmfStreamFile, the file pointer */
	int  endOfData;				/* number of characters in file data */
	int  currentPos;			/* offset of next place to read/write */
	int  blockSize;				/* incremental size to grow buffer */
	int  initialBlock;			/* initial buffer size */
	omfStreamStatus_t openStatus;

} omfStreamDescriptor_t, *omfStreamDescriptorPtr_t;

/*

  When using omfStreamDescriptor_t for stream type Memory, consider the following guidelines:

  READONLY
  APPEND
		allocate memory buffer to hold entire OMFI file, assign address to memPtr.
		memSize = number of bytes in buffer
		endOfData = number of bytes in buffer
		currentPos = 0;
		blockSize = 0;
		initialBlock = 0;

  CREATE
		memPtr = NULL;
		memSize = 0;
		endOfData = 0;
		currentPos = 0;
		blockSize = 128*1024;	  // for example...
		initialBlock = 256*1024;
	Note: large blockSize can waste memory, small blockSize can cause too much copying.

  */

#endif /* _OMF_NEWMEMFILE_ */

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
