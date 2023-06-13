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

#ifndef _OMF_TABLE_API_
#define _OMF_TABLE_API_ 1

#include "omErr.h"
#include "omTypes.h"

#if PORT_LANG_CPLUSPLUS
extern          "C"
{
#endif

#define USE_DEFAULT		0

typedef enum
{
	kOmTableDupError,
	kOmTableDupReplace,
	kOmTableDupAddDup
} omTableDuplicate_t;

typedef enum
{
	kTableSrchAny,
	kTableSrchMatch,
	kTableSrchUnique
} omTableSearch_t;

typedef omfInt32	(*omTblMapProc)( void *key);
typedef omfBool	(*omTblCompareProc)( void *key1, void *key2);
typedef void	(*omTblDisposeProc)(void *valuePtr);

typedef struct omTable omTable_t;
typedef struct omfTableLink tableLink_t;

typedef struct
{
	omfInt32				cookie;		/* Private */
	omTable_t			*table;		/* Private */
	omfInt32				curHash;	/* Private */
	tableLink_t			*nextEntry;	/* Private */
	omTableSearch_t		srch;		/* Private */
	void				*srchKey;	/* Private */
	/* */
	void				*key;		/* OUT */
	omfInt32				keylen;		/* OUT */
	void				*valuePtr;	/* OUT */
	omfInt32				valueLen;	/* OUT */
} omTableIterate_t;

/************************************************************************
 *
 * Root Table Functions (used to implement specializations below)
 *
 * If you need a hash table, create a specialized type, either
 * below or in your code.  A compare callback and a hash callback will
 * need to be provided when the table is created if you use omfsNewTable
 * directly
 *
 ************************************************************************/

omfErr_t omfsNewTable(
			omfHdl_t file,
			omfInt16 initKeySize,
			omTblMapProc myMap,
			omTblCompareProc myCompare,
			omfInt32 numBuckets,
			omTable_t **resultPtr);
			
omfErr_t omfsSetTableDispose(
			omTable_t *table,
			omTblDisposeProc proc);
			
omfErr_t omfsTableAddValuePtr(
			omTable_t *table,
			void *key,
			omfInt16 keyLen,
			void *value,
			omTableDuplicate_t dup);
			
omfErr_t omfsTableAddValueBlock(
			omTable_t *table,
			void *key,
			short keyLen,
			void *value,
			omfInt32 valueLen,
			omTableDuplicate_t dup);
			
omfErr_t omfsTableRemove(
			omTable_t *table,
			void *key);
			
omfBool omfsTableIncludesKey(
			omTable_t *table,
			void *key);

void *omfsTableLookupPtr(
			omTable_t *table,
			void *key);
			
omfErr_t omfsTableLookupBlock(
			omTable_t *table,
			void *key,
			omfInt32 valueLen,
			void *valuePtr,
			omfBool *found);
			
OMF_EXPORT omfErr_t omfsTableFirstEntry(
			omTable_t *table,
			omTableIterate_t *iter,
			omfBool *found);
			
omfErr_t omfsTableFirstEntryMatching(
			omTable_t *table,
			omTableIterate_t *iter,
			void *key,
			omfBool *found);
			
omfErr_t omfsTableFirstEntryUnique(
			omTable_t *table,
			omTableIterate_t *iter,
			omfBool *found);
			
OMF_EXPORT omfErr_t omfsTableNextEntry(
			omTableIterate_t *iter,
			omfBool *found);

omfInt32 omfmTableNumEntriesMatching(
			omTable_t *table,
			void *key);
			
omfErr_t omfsTableSearchDataValue(
			omTable_t *table,
			omfInt32 valueLen,
			void *value,
			omfInt32 keyLen,
			void *key,
			omfBool *found);
			
omfErr_t omfsTableDispose(
			omTable_t *table);
			
omfErr_t omfsTableDisposeAll(
			omTable_t *table);
			
omfErr_t omfsTableDisposeItems(
			omTable_t *table);


/************************************************************************
 *
 * String Table Functions
 *
 ************************************************************************/

omfErr_t omfsNewStringTable(
			omfHdl_t file,
			omfBool caseSensistive,
			omTblCompareProc myCompare,
			omfInt32 numBuckets,
			omTable_t **resultPtr);
			
omfErr_t omfsTableAddString(
			omTable_t *table,
			char *key,
			void *value,
			omTableDuplicate_t dup);
			
omfErr_t omfsTableAddStringBlock(
			omTable_t *table,
			char *key,
			void *value,
			omfInt32 valueLen,
			omTableDuplicate_t dup);

/************************************************************************
 *
 * UID Table Functions
 *
 ************************************************************************/

omfErr_t omfsNewUIDTable(
			omfHdl_t file,
			omfInt32 numBuckets,
			omTable_t **result);
			
omfErr_t omfsTableAddUID(
			omTable_t *table,
			omfUID_t key,
			void *value,
			omTableDuplicate_t dup);
			
omfErr_t omfsTableRemoveUID(
			omTable_t *table,
			omfUID_t key);
			
omfBool omfsTableIncludesUID(
			omTable_t *table,
			omfUID_t key);
			
void *omfsTableUIDLookupPtr(
			omTable_t *table,
			omfUID_t key);

/************************************************************************
 *
 * ClassID Table Functions
 *
 ************************************************************************/

omfErr_t omfsNewClassIDTable(
			omfHdl_t file,
			omfInt32 numBuckets,
			omTable_t **result);

omfErr_t omfsTableAddClassID(
			omTable_t *table,
			omfClassIDPtr_t key,
			void *value,
			omfInt32 valueLen);
			
omfErr_t omfsTableClassIDLookup(
			omTable_t *table,
			omfClassIDPtr_t key,
			omfInt32 valueLen,
			void *valuePtr,
			omfBool *found);

/************************************************************************
 *
 * TrackID Table Functions
 *
 ************************************************************************/

omfErr_t omfsNewTrackIDTable(
			omfHdl_t file,
			omfInt32 numBuckets,
			omTable_t **result);
			
omfErr_t omfsTableAddTrackID(
			omTable_t *table,
			omfTrackID_t key,
			void *value,
			omfInt32 valueLen);
			
omfErr_t omfsTableTrackIDLookup(
			omTable_t *table,
			omfTrackID_t key,
			void *value,
			omfInt32 valueLen,
			omfBool *found);

/************************************************************************
 *
 * omfProperty_t Table Functions
 *
 ************************************************************************/

omfErr_t omfsNewPropertyTable(
			omfHdl_t file,
			omfInt32 numBuckets,
			omTable_t **result);
			
omfErr_t omfsTableAddProperty(
			omTable_t *table,
			omfProperty_t key,
			void *value, 
			omfInt32 valueLen,
			omTableDuplicate_t dup);
			
omfErr_t omfsTablePropertyLookup(
			omTable_t *table,
			omfProperty_t key,
			omfInt32 valueLen,
			void *valuePtr,
			omfBool *found);

/************************************************************************
 *
 * omfType_t Table Functions
 *
 ************************************************************************/

omfErr_t omfsNewTypeTable(
			omfHdl_t file,
			omfInt32 numBuckets,
			omTable_t **result);
			
omfErr_t omfsTableAddType(
			omTable_t *table,
			omfType_t key,
			void *value,
			omfInt32 valueLen);
			
omfErr_t omfsTableTypeLookup(
			omTable_t *table,
			omfType_t key,
			omfInt32 valueLen,
			void *valuePtr,
			omfBool *found);

/************************************************************************
 *
 * Definition (Datakind and EffectDef) Table Functions
 *
 ************************************************************************/

omfErr_t omfsNewDefTable(
			omfHdl_t file,
			omfInt32 numBuckets,
			omTable_t **result);
			
omfErr_t omfsTableAddDef(
			omTable_t *table,
			omfUniqueName_t key,
			void *value);
			
void *omfsTableDefLookup(
			omTable_t *table,
			omfUniqueName_t key);

#ifdef OMFI_SELF_TEST
void testOmTable(void);
#endif

#if PORT_LANG_CPLUSPLUS
}
#endif
#endif				/* _OMF_TABLE_API_ */

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
