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

/*
 * Name: omMobGet.c
 *
 * Overall Function: The main API file for traversing mobs.
 *
 * Audience: Clients writing or reading OMFI compositions.
 *
 * Public Functions:
 *     omfiIsAXXX() - IsA calls for classes
 *     omfiIsXXXKind() - IsA calls for datakinds
 *     omfiIsAXXXMob() - IsA calls for mobs
 *     omfiXXXGetInfo() - Get property values for individual objects
 *     omfiSourceClipResolveRef()
 *     omfiGetNumXXX() - Get number of components of a non-atomic object
 *     omfiIteratorAlloc(), omfiIteratorDispose(), omfiIteratorClear()
 *     omfiGetNextXXX() - iterators
 *     omfiMobMatchAndExecute()
 *     omfiConvertEditRate()
 *	   omfiMobPurge() - Purge an objects data from memory but allow reload (Virtual TOC)
 *
 * General error codes returned:
 *      OM_ERR_NONE
 *             Success
 * 		OM_ERR_BAD_FHDL
 *				Bad file handle passed in
 * 		OM_ERR_BENTO_PROBLEM
 *				Bento returned an error, check BentoErrorNumber.
 *      OM_ERR_NULLOBJECT
 *              Null object passed in as an argument
 *      OM_ERR_INVALID_DATAKIND
 *              An invalid datakind was passed in
 *      OM_ERR_NULL_PARAM
 *              A required parameter was NULL
 *      OM_ERR_NOT_IN_20
 *              This function is not supported in 2.0 (GetSequLength)
 *      OM_ERR_INVALID_OBJ
 *              An object of the wrong class was passed in
 *      OM_ERR_INTERN_TOO_SMALL
 *              Buffer size is too small (GetDataValue)
 *      OM_ERR_MOB_NOT_FOUND
 *              A source clip references a mob that is not in the file
 *      OM_ERR_REQUIRED_POSITIVE
 *              The value (e.g., length) must be positive)
 *      OM_ERR_TRAVERSAL_NOT_POSS
 *              An attempt to traverse a mob failed
 *      OM_ERR_TRACK_NOT_FOUND
 *              A source clip references a track that does not exist in a mob
 *      OM_ERR_NO_MORE_MOBS
 *              The end of the mob chain was reached
 *      OM_ERR_INVALID_MOBTYPE
 *              This operation does not apply to this mob type
 *      OM_ERR_NOMEMORY
 *              We ran out of memory (omfsMalloc failed)
 *      OM_ERR_ITER_WRONG_TYPE
 *              An iterator of the wrong type was passed in
 *      OM_ERR_INVALID_SEARCH_CRIT
 *              This search criteria does not apply to this iterator
 *      OM_ERR_INTERNAL_ITERATOR
 *              An internal iterator error occurred
 */

#include "masterhd.h"
#include <stdlib.h> /* for abs() */

#include "omPublic.h"
#include "omPvt.h"
#include "omMedia.h" 
#include "omMobMgt.h"
#include "omEffect.h" /* For MASK processing */

/* Moved math.h down here to make NEXT's compiler happy */
#include <math.h>

/* Render types for NextRender function */
#define WORKING_RENDER 1
#define FINAL_RENDER 2

/*******************************/
/* Static Function Definitions */
/*******************************/
static omfErr_t GetNextArrayElem(
    omfIterHdl_t iterHdl,
    omfSearchCrit_t *searchCrit,
	omfObject_t parent,

    omfProperty_t prop,
    omfIterType_t iterType,
	omfObject_t *nextObj);

static omfBool HasAMediaDesc(
	omfHdl_t file,
    omfMobObj_t mob,
	omfClassIDPtr_t mdescClass,
    omfErr_t *omfError);

static omfErr_t GetDataValue(
    omfHdl_t file,
	omfObject_t value,
	omfProperty_t prop,
	omfDDefObj_t datakind, 
    omfInt32 valueSize,
	omfInt32 *bytesRead,
    void *data);

static omfErr_t CountMobInIndex(
	omfHdl_t file,
    omfMobKind_t mobKind,
    omfProperty_t indexProp,
	omfInt32 totalMobs,
    omfInt32 *numMobs);

static omfErr_t GetNextRender(
    omfIterHdl_t iterHdl,
	omfCpntObj_t effect,
	omfInt16 renderType,
	omfSearchCrit_t *searchCrit,
	omfMobObj_t *fileMob);

static omfErr_t GetDatakindFromObj(
    omfHdl_t file,
	omfObject_t obj,
    omfDDefObj_t *def);

static omfErr_t Get1xSequLength(
    omfHdl_t file,
    omfObject_t sequence,
    omfLength_t *length);

omfErr_t GetRemFramesDrop(omfUInt32 maskBits, 
							char ones,
							omfUInt32 phase, 
							omfUInt32 masksize,
							omfInt32 *result);

omfErr_t GetRemFramesDouble(omfUInt32 maskBits, 
							char ones,
							omfUInt32 phase,
							omfUInt32 masksize,
							omfInt32 *result);

omfErr_t MaskGetBits(omfUInt32 maskBits,
					 char *maskones);

static omfErr_t objectPurgeTree(
    omfHdl_t file,    /* IN - File Handle */
	omfObject_t obj);  /* IN - Object to purge */

/*******************************/
/* Functions                   */
/*******************************/
/*************************************************************************
 * Function: omfiIsAXXX()
 *
 * 		This set of boolean functions answer the question "does this
 *      object belong to class XXX".  The function will return TRUE,
 *      if the object belongs to class XXX or any of its subclasses.
 *      For example, omfiIsAComponent() will return TRUE for an object
 *      of class SCLP (source clip).
 *
 *      These routines are supported for 1.x and 2.x files.  FALSE will
 *      be returned if IsA[2.xClass] is asked on a 1.x object.
 *
 * Argument Notes:
 *      If an error occurs, it is returned as the last argument, and the
 *      return value will be FALSE.
 *
 * ReturnValue:
 *		Boolean (as described above)
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfBool omfiIsAComponent(
    omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
  return(omfsIsTypeOf(file, obj, "CPNT", omfError));
}

omfBool omfiIsASegment(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
	return(omfsIsTypeOf(file, obj, "SEGM", omfError));
}

omfBool omfiIsASourceClip(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
	return (omfsIsTypeOf(file, obj, "SCLP", omfError));
}

omfBool omfiIsAnEdgecodeClip(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
	return (omfsIsTypeOf(file, obj, "ECCP", omfError));
}

omfBool omfiIsATimecodeClip(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
  return(omfsIsTypeOf(file, obj, "TCCP", omfError));
}

omfBool omfiIsAVaryValue(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
  return(omfsIsTypeOf(file, obj, "VVAL", omfError));
}

omfBool omfiIsAConstValue(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
  return(omfsIsTypeOf(file, obj, "CVAL", omfError));
}

omfBool omfiIsAFiller(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
  return(omfsIsTypeOf(file, obj, "FILL", omfError));
}

omfBool omfiIsAnEffect(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
	return(omfsIsTypeOf(file, obj, "EFFE", omfError));
}

omfBool omfiIsANestedScope(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
	return(omfsIsTypeOf(file, obj, "NEST", omfError));
}

omfBool omfiIsAScopeRef(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
	return(omfsIsTypeOf(file, obj, "SREF", omfError));
}

omfBool omfiIsASelector(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
	return(omfsIsTypeOf(file, obj, "SLCT", omfError));
}

omfBool omfiIsASequence(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
	return(omfsIsTypeOf(file, obj, "SEQU", omfError));
}

omfBool omfiIsATransition(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
	return(omfsIsTypeOf(file, obj, "TRAN", omfError));
}

omfBool omfiIsADatakind(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
	return(omfsIsTypeOf(file, obj, "DDEF", omfError));
}

omfBool omfiIsAnEffectDef(
	omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
	omfErr_t *omfError)  /* OUT - Error code */
{
	return(omfsIsTypeOf(file, obj, "EDEF", omfError));
}

omfBool omfiIsAMediaGroup(
    omfHdl_t file,       /* IN - File Handle */
    omfObject_t obj,     /* IN - Object to operate on */
    omfErr_t *omfError)  /* OUT - Error code */
{
	return(omfsIsTypeOf(file, obj, "MGRP", omfError));
}

/*************************************************************************
 * Function: omfiDatakindGetName()
 *
 * 		Given a datakind object, this function will return a UniqueName
 *      string describing its datakind (e.g., omfi:data:Picture).
 *
 * Argument Notes:
 *      The UniqueName string buffer must be a preallocated buffer of
 *      size nameSize.  Passing in a variable of type omfUniqueName_t
 *      and a size of OMUNIQUENAME_SIZE is recommended.  If the name is
 *      longer than the buffer provided, it will truncate the string to
 *      fit into the buffer.
 * 
 *      This routine supports both 1.x and 2.x files.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiDatakindGetName(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
	omfInt32 nameSize,   /* IN - Size of name buffer */
	omfUniqueNamePtr_t datakindName) /* IN/OUT - preallocated buffer to
									  * return name string
									  */
{
    omfInt32 type;

	omfAssertValidFHdl(file);
	omfAssert((dataDef != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((datakindName != NULL), file, OM_ERR_NULL_PARAM);

	XPROTECT(file)
	  {
		if (datakindName)
		  {
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				/* Convert from our fake 1.x datakinds to a uniquename */
				type = MediaKindToTrackType(file, dataDef);
				switch(type)
				  {
				  case TT_PICTURE:
					strncpy(datakindName, PICTUREKIND, nameSize);
					break;
				  case TT_SOUND:
					strncpy(datakindName, SOUNDKIND, nameSize);
					break;
				  case TT_NULL:
					strncpy(datakindName, NODATAKIND, nameSize);
					break;
				  case TT_TIMECODE:
					strncpy(datakindName, TIMECODEKIND, nameSize);
					break;
				  case TT_EDGECODE:
					strncpy(datakindName, EDGECODEKIND, nameSize);
					break;
				  default:
					RAISE(OM_ERR_INVALID_DATAKIND);
					break;
				  }
			  }
			else /* kOmfRev2x */
			  {
				CHECK(omfsReadUniqueName(file, dataDef, OMDDEFDatakindID, 
										 datakindName, nameSize));
			  }
		  }
		else
		  {
			RAISE(OM_ERR_NULL_PARAM);
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE); 
}

/*************************************************************************
 * Private Function: IsDatakindOf()
 *
 * 		Boolean function that returns TRUE if the datakind of the
 *      dataDef IN parameter matches the datakindName string.
 *
 *      This routine supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *      Like other boolean functions, the error code is returned through
 *      the last parameter.
 *
 * ReturnValue:
 *		Boolean (As described above).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfBool IsDatakindOf(
    omfHdl_t file,                   /* IN - File Handle */
    omfDDefObj_t dataDef,            /* IN - Datakind definition object */
	omfUniqueNamePtr_t datakindName, /* IN - DatakindName to compare against */
	omfErr_t *omfError)              /* OUT - Error code */
{
   omfUniqueName_t name;

    if (omfError)
	  *omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE);
	omfAssertBool((dataDef != NULL), file, OM_ERR_NULLOBJECT, 
				  omfError, FALSE);
	omfAssertBool((datakindName != NULL), file, OM_ERR_NULL_PARAM,
				  omfError, FALSE);

	XPROTECT(file)
	  {
		CHECK(omfiDatakindGetName(file, dataDef, OMUNIQUENAME_SIZE, name));
		if (!strcmp(name, datakindName))
		  return(TRUE);
		return(FALSE); /* No match */
	  }

	XEXCEPT
	  {
		if (omfError)
		  *omfError = XCODE();
	  }
	XEND_SPECIAL(FALSE);

	return(TRUE);
}

/*************************************************************************
 * Private Function: DoesDatakindConvertTo()
 *
 * 		Boolean function that returns TRUE if the datakind of the
 *      dataDef IN parameter can convert to the datakind specified
 *      with the the datakindName string.
 *
 *      NOTE: For now, this function will simply check the convertibility
 *      of the datakinds that are listed in the OMF spec.  It won't work
 *      for private datakinds.  This should be updated in a future
 *      version of the Toolkit.
 *
 *      This routine supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *      Like other boolean functions, the error code is returned through
 *      the last parameter.
 *
 * ReturnValue:
 *		Boolean (As described above).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfBool DoesDatakindConvertTo(
    omfHdl_t file,                   /* IN - File Handle */
    omfDDefObj_t dataDef,            /* IN - Datakind definition object */
	omfUniqueNamePtr_t datakindName, /* IN - DatakindName to compare against */
	omfErr_t *omfError)              /* OUT - Error code */
{
   omfUniqueName_t name;
   omfBool colorMatch = FALSE;

    if (omfError)
	  *omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE);
	omfAssertBool((dataDef != NULL), file, OM_ERR_NULLOBJECT, 
				  omfError, FALSE);
	omfAssertBool((datakindName != NULL), file, OM_ERR_NULL_PARAM,
				  omfError, FALSE);

	XPROTECT(file)
	  {
		/* NOTE: This will only validate required datakinds.  This function
		 * should be updated in the future to support private datakinds.
		 */

		/* NOTE: This could probably be optimized not to use strings.
		 * For now, this will work.
		 */

		/* Return TRUE for an exact match */
		CHECK(omfiDatakindGetName(file, dataDef, OMUNIQUENAME_SIZE, name));
		if (!strcmp(name, datakindName))
		  return(TRUE);

		/* PictureWithMatte, Matte, Color ==> Picture */
		/* Picture, Matte, Color ==> PictureWithMatte */
		/* PictureWithMatte, Picture, Color ==> Matte */
		if (!strcmp(name, COLORKIND))
		  colorMatch = TRUE;
		if (!strcmp(datakindName, PICTUREKIND) &&
			(!strcmp(name, PICTWITHMATTEKIND) ||
			 !strcmp(name, MATTEKIND) ||
			 colorMatch))
		  return(TRUE);

		if (!strcmp(datakindName, PICTWITHMATTEKIND) &&
			(!strcmp(name, PICTUREKIND) ||
			 !strcmp(name, MATTEKIND) ||
			 colorMatch))
		  return(TRUE);

		if (!strcmp(datakindName, MATTEKIND) &&
			(!strcmp(name, PICTWITHMATTEKIND) ||
			 !strcmp(name, PICTUREKIND) ||
			 colorMatch))
		  return(TRUE);

		/* Sound and StereoSound are not convertible to each other */
		if (!strcmp(datakindName, SOUNDKIND) &&
			!strcmp(name, STEREOKIND))
		  return(TRUE);

		if (!strcmp(datakindName, STEREOKIND) &&
			!strcmp(name, SOUNDKIND))
		  return(TRUE);

		/* Only Rational is convertible to Color */
		if (!strcmp(datakindName, COLORKIND) &&
			!strcmp(name, RATIONALKIND))
		  return(TRUE);

		/* Color, Int8, Int16, Int32, UInt8, UInt16, UInt32 ==> Rational */
		if (!strcmp(datakindName, RATIONALKIND) &&
			(!strcmp(name, INT8KIND) ||
			 !strcmp(name, INT16KIND) ||
			 !strcmp(name, INT32KIND) ||
			 !strcmp(name, UINT8KIND) ||
			 !strcmp(name, UINT16KIND) ||
			 !strcmp(name, UINT32KIND)))
		  return(TRUE);

		/* Rational ==> Int32 */
		if (!strcmp(datakindName, INT32KIND) &&
			!strcmp(name, RATIONALKIND))
		  return(TRUE);

		return(FALSE); /* No match */
	  }

	XEXCEPT
	  {
		if (omfError)
		  *omfError = XCODE();
	  }
	XEND_SPECIAL(FALSE);

	return(TRUE);
}

/*************************************************************************
 * Private Function: DoesDatakindConvertFrom()
 *
 * 		Boolean function that returns TRUE if the datakind of the
 *      dataDef IN parameter can convert from the datakind specified
 *      with the the datakindName string.
 *
 *      NOTE: For now, this function will simply check the convertibility
 *      of the datakinds that are listed in the OMF spec.  It won't work
 *      for private datakinds.  This should be updated in a future
 *      version of the Toolkit.
 *
 *      This routine supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *      Like other boolean functions, the error code is returned through
 *      the last parameter.
 *
 * ReturnValue:
 *		Boolean (As described above).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfBool DoesDatakindConvertFrom(
    omfHdl_t file,                   /* IN - File Handle */
    omfDDefObj_t dataDef,            /* IN - Datakind definition object */
	omfUniqueNamePtr_t datakindName, /* IN - DatakindName to compare against */
	omfErr_t *omfError)              /* OUT - Error code */
{
   omfUniqueName_t name;

    if (omfError)
	  *omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE);
	omfAssertBool((dataDef != NULL), file, OM_ERR_NULLOBJECT, 
				  omfError, FALSE);
	omfAssertBool((datakindName != NULL), file, OM_ERR_NULL_PARAM,
				  omfError, FALSE);

	XPROTECT(file)
	  {
		/* NOTE: This will only validate required datakinds.  This function
		 * should be updated in the future to support private datakinds.
		 */

		/* NOTE: This could probably be optimized not to use strings.
		 * For now, this will work.
		 */

		/* Return TRUE for an exact match */
		CHECK(omfiDatakindGetName(file, dataDef, OMUNIQUENAME_SIZE, name));
		if (!strcmp(name, datakindName))
		  return(TRUE);

		/* Picture ==> PictureWithMatte, Matte */
		/* PictureWithMatte ==> Picture, Matte */
		/* Matte ==> Picture, PictureWithMatte */
		/* Color ==> Picture, Matte, PictureWithMatte, Rational */
		if (!strcmp(datakindName, PICTUREKIND) &&
			(!strcmp(name, PICTWITHMATTEKIND) ||
			 !strcmp(name, MATTEKIND)))
		  return(TRUE);

		if (!strcmp(datakindName, PICTWITHMATTEKIND) &&
			(!strcmp(name, PICTUREKIND) ||
			 !strcmp(name, MATTEKIND)))
		  return(TRUE);

		if (!strcmp(datakindName, MATTEKIND) &&
			(!strcmp(name, PICTWITHMATTEKIND) ||
			 !strcmp(name, PICTUREKIND)))
		  return(TRUE);

		/* Sound and StereoSound are not convertible to each other */
		if (!strcmp(datakindName, SOUNDKIND) &&
			!strcmp(name, STEREOKIND))
		  return(TRUE);

		if (!strcmp(datakindName, STEREOKIND) &&
			!strcmp(name, SOUNDKIND))
		  return(TRUE);

		/* Color ==> PictureWithmatte, Picture, Matte, Rational */
		if (!strcmp(datakindName, COLORKIND) &&
			(!strcmp(name, PICTWITHMATTEKIND) ||
			 !strcmp(name, PICTUREKIND) ||
			 !strcmp(name, MATTEKIND) ||
			 !strcmp(name, RATIONALKIND)))
		  return(TRUE);

		/* Rational ==> Color, Int32 */
		if (!strcmp(datakindName, RATIONALKIND) &&
			(!strcmp(name, COLORKIND) ||
			 !strcmp(name, INT32KIND)))
		  return(TRUE);

		/* Int32, Int16, Int8, UInt32, UInt16, UInt8 ==> Rational */
		if ((!strcmp(datakindName, INT32KIND) ||
			 !strcmp(datakindName, INT16KIND) ||
			 !strcmp(datakindName, INT8KIND) ||
			 !strcmp(datakindName, UINT32KIND) ||
			 !strcmp(datakindName, UINT16KIND) ||
			 !strcmp(datakindName, UINT8KIND)) &&
			(!strcmp(name, RATIONALKIND)))
		  return(TRUE);

		return(FALSE); /* No match */
	  }

	XEXCEPT
	  {
		if (omfError)
		  *omfError = XCODE();
	  }
	XEND_SPECIAL(FALSE);

	return(TRUE);
}

/*************************************************************************
 * Private Function: IsDatakindMediaStream()
 *
 *      This function returns whether or not the input datakind object
 *      represents a "media stream" datakind.  The media stream datakinds
 *      are "omfi:data:Edgecode", "omfi:data:Timecode", "omfi:data:Picture",
 *      "omfi:data:PictureWithMatte", "omfi:data:Matte", "omfi:data:Sound",
 *      "omfi:data:StereoSound".
 *
 *      NOTE: For now, this function will simply check against the
 *      "media stream" datakinds that are listed in the OMF spec. 
 *      It won't support private datakinds.  This should be updated in a future
 *      version of the Toolkit.
 *
 *      This routine supports only supports 2.x files.
 *
 * Argument Notes:
 *      Like other boolean functions, the error code is returned through
 *      the last parameter.
 *
 * ReturnValue:
 *		Boolean (As described above).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfBool IsDatakindMediaStream(
							  omfHdl_t file,        /* IN - File Handle */
							  omfDDefObj_t dataDef, /* IN - Datakind object */
							  omfErr_t *omfError)   /* OUT - Error Code */
{
  omfUniqueName_t name;
  omfErr_t tmpError = OM_ERR_NONE;

  if (omfError)
	  *omfError = OM_ERR_NONE;
  omfAssertValidFHdlBool(file, omfError, FALSE);
  omfAssertBool((dataDef != NULL), file, OM_ERR_NULLOBJECT, 
				  omfError, FALSE);

  XPROTECT(file)
	{
	  /* NOTE: This will only validate required datakinds.  This function
	   * should be updated in the future to support private datakinds.
	   */

	  /* Getting the name once and strcpy() is faster than calling each
	   * "isDatakind" function which will retrieve the name each time.
	   */
	  CHECK(omfiDatakindGetName(file, dataDef, OMUNIQUENAME_SIZE, name));

	  if (!strcmp(name, PICTUREKIND) ||
		  !strcmp(name, PICTWITHMATTEKIND) ||
		  !strcmp(name, MATTEKIND) ||
		  !strcmp(name, SOUNDKIND) ||
		  !strcmp(name, STEREOKIND) ||
		  !strcmp(name, TIMECODEKIND) ||
		  !strcmp(name, EDGECODEKIND))
		return(TRUE);

	  return(FALSE);
	}
  
  XEXCEPT
	{
	  if (omfError)
		*omfError = XCODE();
	}
  XEND_SPECIAL(FALSE);

  return(TRUE);
}

/*************************************************************************
 * Function: omfiIsAXXXKind()
 *
 * 		This set of boolean functions answer the question "is this 
 *      data definition object of datakind XXX".  The function will return
 *      TRUE if the UniqueName property matches the string "omfi:data:XXX".
 *      It will return FALSE otherwise.  (NOTE: These functions do not check
 *      for convertible datakinds.)
 *
 *      These routines are supported for 1.x and 2.x files.  1.x files do
 *      have the concept of a "datakind".  However, a mapping from tracktype
 *      is provided to support backward compatibility.
 *
 *      The matchKind argument specifies whether the datakind object 
 *      exactly matches the requested datakind, or whether it is 
 *      convertible to the requested datakind.
 *
 * Argument Notes:
 *      If an error occurs, it is returned as the last argument, and the return
 *      value will be FALSE.
 *
 * ReturnValue:
 *		Boolean (as described above)
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/

omfBool omfiIsCharKind(
	omfHdl_t file,        /* IN - File Handle */
	omfDDefObj_t dataDef, /* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)   /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, CHARKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, CHARKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, CHARKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsUInt8Kind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{  
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, UINT8KIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, UINT8KIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, UINT8KIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsUInt16Kind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{  
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, UINT16KIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, UINT16KIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, UINT16KIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsUInt32Kind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{  
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, UINT32KIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, UINT32KIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, UINT32KIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsInt8Kind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, INT8KIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, INT8KIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, INT8KIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsInt16Kind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, INT16KIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, INT16KIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, INT16KIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsInt32Kind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, INT32KIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, INT32KIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, INT32KIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsRationalKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, RATIONALKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, RATIONALKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, RATIONALKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsBooleanKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, BOOLEANKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, BOOLEANKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, BOOLEANKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsStringKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */

{	
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, STRINGKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, STRINGKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, STRINGKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsColorSpaceKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, COLORSPACEKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, COLORSPACEKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, COLORSPACEKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsColorKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, COLORKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, COLORKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, COLORKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsDistanceKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, DISTANCEKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, DISTANCEKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, DISTANCEKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsPointKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, POINTKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, POINTKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, POINTKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsDirectionCodeKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, DIRECTIONKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, DIRECTIONKIND, 
								 omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, DIRECTIONKIND, 
								   omfError));
  else
	return(FALSE);
}

omfBool omfiIsPolynomialKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, POLYKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, POLYKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, POLYKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsPictureKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, PICTUREKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, PICTUREKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, PICTUREKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsMatteKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, MATTEKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, MATTEKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, MATTEKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsPictureWithMatteKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, PICTWITHMATTEKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, PICTWITHMATTEKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, PICTWITHMATTEKIND,omfError));
  else
	return(FALSE);
}

omfBool omfiIsSoundKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, SOUNDKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, SOUNDKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, SOUNDKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsStereoSoundKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, STEREOKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, STEREOKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, STEREOKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsTimecodeKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, TIMECODEKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, TIMECODEKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, TIMECODEKIND, omfError));
  else
	return(FALSE);
}

omfBool omfiIsEdgecodeKind(
	omfHdl_t file,       /* IN - File Handle */
	omfDDefObj_t dataDef,/* IN - Datakind definition object */
    omfDatakindMatch_t matchKind, /* IN - kExactMatch, kConvertTo, kConvertFrom */
	omfErr_t *omfError)  /* OUT - Error code */
{
  if (matchKind == kExactMatch)
	return(IsDatakindOf(file, dataDef, EDGECODEKIND, omfError));
  else if (matchKind == kConvertTo)
	return(DoesDatakindConvertTo(file, dataDef, EDGECODEKIND, omfError));
  else if (matchKind == kConvertFrom)
	return(DoesDatakindConvertFrom(file, dataDef, EDGECODEKIND, omfError));
  else
	return(FALSE);
}

/*************************************************************************
 * Private Function: Get1xSequLength()
 *
 * 		This function is called to calculate the length of a 1.x sequence
 *      object.  It recursively calculates the length for its subcomponents.
 *      This function is needed for backward compatibility since sequences
 *      did not have an explicit length property in 1.x.
 *
 *      This function is only supported on 1.x files and objects.
 *
 * Argument Notes:
 *      The input sequence object must be a 1.x object.  The length
 *      is passed back through the last argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t Get1xSequLength(
    omfHdl_t file,        /* IN - File Handle */
    omfObject_t sequence, /* IN - Sequence object */
    omfLength_t *length)  /* OUT - Sequence Length */
{
    omfInt32 loop, numCpnts;
	omfLength_t tmpLen = omfsCvtInt32toLength(0, tmpLen);
	omfLength_t totalLen = omfsCvtInt32toLength(0, totalLen);
	omfLength_t zeroLen = omfsCvtInt32toLength(0, zeroLen);
	omfObject_t cpnt = NULL;
	omfErr_t tmpError = OM_ERR_NONE;  

	omfsCvtInt32toInt64(0, length);
	omfAssertValidFHdl(file);
	omfAssert((sequence != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((length != NULL), file, OM_ERR_NULL_PARAM);

	XPROTECT(file)
	  {
		if (file->setrev == kOmfRev2x)
		  {
			RAISE(OM_ERR_NOT_IN_20);
		  }
		numCpnts = omfsLengthObjRefArray(file, sequence, OMSEQUSequence);
		for (loop = 1; loop <= numCpnts; loop++)
		  {
			CHECK(omfsGetNthObjRefArray(file, sequence, OMSEQUSequence, 
										&cpnt, loop));
			CHECK(omfiComponentGetLength(file, cpnt, &tmpLen));
			if (!omfiIsATransition(file, cpnt, &tmpError))
			  omfsAddInt64toInt64(tmpLen, &totalLen);
			else /* It is a Transition */
			  omfsSubInt64fromInt64(tmpLen, &totalLen);  

			/* Release Bento reference, so the useCount is decremented */
			CMReleaseObject((CMObject)cpnt);
		  }
	  }

	XEXCEPT
	  {
		*length = zeroLen;
		return(XCODE());
	  }
	XEND;

	*length = totalLen;
	return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: omfiComponentGetLength()
 *
 * 		This function returns the length of a component.  It supports
 *      both 1.x and 2.x component objects.  Length is not stored as sn
 *      implicit property on 1.x objects, so this routine recursively
 *      calculates the length.
 *
 * Argument Notes:
 *      The input component object can be either a 1.x or a 2.x object.
 *      The length is returned as the last argument.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiComponentGetLength(
	omfHdl_t file,          /* IN - File Handle */
	omfCpntObj_t component, /* IN - Component Object */
	omfLength_t *length)    /* OUT - Component Length */
{
    omfInt32 tmp1xLength = 0;
    omfLength_t tmpLength;
    omfErr_t omfError = OM_ERR_NONE;

	omfsCvtInt32toInt64(0, length);
	omfAssertValidFHdl(file);
	omfAssert((component != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((length != NULL), file, OM_ERR_NULL_PARAM);
	
	XPROTECT(file)
	  {
		if (length)
		  {
			/* If 1.5, get length of TRKG, CLIP or SEQU */
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				if (omfsIsTypeOf(file, component, "TRKG", &omfError))
				  {
					CHECK(omfsReadInt32(file, component, OMTRKGGroupLength, 
									   &tmp1xLength));
					omfsCvtInt32toInt64(tmp1xLength, &tmpLength);
				  }
				else if (omfsIsTypeOf(file, component, "CLIP", &omfError))
				  {
					CHECK(omfsReadLength(file, component, OMCLIPLength, 
									   &tmpLength));
				  }
				else if (omfsIsTypeOf(file, component, "SEQU", &omfError))
				  {
					CHECK(Get1xSequLength(file, component, &tmpLength));
				  }
				else
				  {
					RAISE(OM_ERR_INVALID_OBJ);
				  }
			  }
			else /* kOmfRev2x */
			  {
					CHECK(omfsReadLength(file, component, OMCPNTLength,
										 &tmpLength));
			  }
		  }
		else
		  {
			RAISE(OM_ERR_NULL_PARAM);
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	*length = tmpLength;
	return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: omfiMobSlotIsTrack()
 *
 * 		This is a boolean function that determines whether or not a mob slot
 *      is a track (externally visible).  For 1.x, it will always return
 *      TRUE since a mob slot is a TRAK object.  For 2.x, it will return
 *      TRUE if the mob slot has a track descriptor (TRKD) object attached
 *      to it.
 *
 * Argument Notes:
 *      As with all boolean functions, the error status is returned as
 *      the last argument.
 *
 * ReturnValue:
 *		Boolean (as described above).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfBool omfiMobSlotIsTrack(
	omfHdl_t file,       /* IN - File Handle */
	omfMSlotObj_t slot,  /* IN - Mob Slot object */
	omfErr_t *omfError)  /* OUT - Error code */
{
    omfObject_t trackDesc = NULL;

	*omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE);
	omfAssertBool((slot != NULL), file, OM_ERR_NULLOBJECT,
				  omfError, FALSE);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			/* All TRAK's are tracks in 1.x */
			if (omfsIsTypeOf(file, slot, "TRAK", omfError))
			  return(TRUE);
			else
			  {
				RAISE(OM_ERR_INVALID_OBJ);
			  }
		  }
		else /* kOmfRev2x */
		  {
			if (omfsIsTypeOf(file, slot, "MSLT", omfError))
			  {
				/* If there is a trackDesc object on the slot, it is a track */
				CHECK(omfsReadObjRef(file, slot, OMMSLTTrackDesc, &trackDesc));
				if (trackDesc)
				  return(TRUE); /* trackDesc found */
			  }
			else
			  {
				RAISE(OM_ERR_INVALID_OBJ);
			  }
		  }
	  }
	XEXCEPT
	  {
		if (XCODE() == OM_ERR_PROP_NOT_PRESENT) /* trackDesc !found */
		  {
			*omfError = OM_ERR_NONE;
			return(FALSE);
		  }
		*omfError = XCODE();
	  }
	XEND_SPECIAL(FALSE); /* Error */

	return(TRUE);
}

/*************************************************************************
 * Private Function: MobSlotGetTrackDesc()
 *
 * 		This function returns the requested information (non-NULL
 *      parameters) identifying a track.  For 1.x, the information is
 *      taken from the TRAK object.  For 2.x, it is taken from the
 *      TRKD object associated with the input mob slot object.
 *
 * Argument Notes:
 *      NULL should be passed for any argument that is not requested.
 *      The name buffer must be a preallocated buffer of size nameSize.
 *      If the name is longer than the buffer provided, it will truncate
 *      the string to fit into the buffer.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t MobSlotGetTrackDesc(
    omfHdl_t file,         /* IN - File Handle */
	omfMobObj_t mob,       /* IN - Mob Object (for 1.x trackID compat) */
	omfMSlotObj_t slot,    /* IN - Mob Slot object */
   	omfInt32 nameSize,     /* IN - Size of name buffer */
	omfString name,        /* IN/OUT - preallocated buffer to return name */
	omfPosition_t *origin, /* OUT - Origin property value */ 
	omfTrackID_t *trackID) /* OUT - trackID property value */
{
    omfObject_t trackDesc = NULL, tmp1xTrackCpnt = NULL;
	omfInt16 tmp1xTrackID = 0;
	omfInt16 tmp1xTrackType = 0;
	omfInt32 tmp1xStartPosition = 0;
	omfUID_t mobID;
	omfBool found = FALSE;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssert((slot != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* Initialize values if not found (mob slot doesn't have descriptor */
		if (trackID)
		  *trackID = 0;
		if (origin)
		  omfsCvtInt32toPosition(0, (*origin));
		  
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			/* There is no name property on a TRAK object */
			if (name)
			  strncpy(name, "\0", 1);
			if (origin)
			  {
				/* Return the 1.x mob's StartPosition */
				omfError = omfsReadInt32(file, mob, OMMOBJStartPosition, 
										 &tmp1xStartPosition);
				if (omfError == OM_ERR_PROP_NOT_PRESENT)
				  omfsCvtInt32toInt64(0, origin);
				else if (omfError == OM_ERR_NONE)
				  omfsCvtInt32toInt64(tmp1xStartPosition, origin);
				else
				  {
					RAISE(omfError);
				  }
			  }
			if (trackID)
			  {
				/* Read the trackLabel and map it back to a 2.x trackID */
				omfError = omfsReadInt16(file, slot, OMTRAKLabelNumber, 
										 &tmp1xTrackID);
				/* The 1.x track may not have a label */
				if (omfError != OM_ERR_PROP_NOT_PRESENT)
				  {
					/* Get the track's component for the tracktype */
					CHECK(omfsReadObjRef(file, slot, OMTRAKTrackComponent,
										 &tmp1xTrackCpnt));
					CHECK(omfsReadTrackType(file, tmp1xTrackCpnt, 
											OMCPNTTrackKind, &tmp1xTrackType));
					/* Get the MobID from the track's mob */
					CHECK(omfiMobGetMobID(file, mob, &mobID));
					CHECK(CvtTrackNumtoID(file, mobID, tmp1xTrackID, 
										  tmp1xTrackType, trackID));
				  }
				else
					*trackID = 0;
			  }
		  }
		else /* kOmfRev2x */
		  {
			omfError = omfsReadObjRef(file, slot, OMMSLTTrackDesc, &trackDesc);
			if (omfError == OM_ERR_NONE)
			  found = TRUE;

			if (name && found)
			  {
				omfError = omfsReadString(file, trackDesc, OMTRKDTrackName, 
										  (char *)name, nameSize);
				if (omfError == OM_ERR_PROP_NOT_PRESENT)
				  strncpy(name, "\0", 1);
				else if (omfError != OM_ERR_NONE)
				  {
					RAISE(omfError);
				  }
			  }
			if (origin && found)
			  {
				CHECK(omfsReadPosition(file, trackDesc, OMTRKDOrigin, origin));
			  }
			if (trackID && found)
			  {	
				CHECK(omfsReadUInt32(file, trackDesc, OMTRKDTrackID, trackID));
			  }
		  } /* OmfRev2x */
	  } /* XPROTECT */

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiTrackSetPhysicalNum()
 *
 *      Adds physical track num property to the specified track.
 *
 *      This function works only for 2.x files.
 *
 * Argument Notes:
 *		PhysicalTrackNum should be >= 1.
 *		A return value of zero indicates that no physical num has been set.
 *		For audio:
 *			1 = left
 *			2 = right
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiTrackGetPhysicalNum(
	omfHdl_t		file,      			/* IN - File Handle */
	omfMSlotObj_t	track,				/* IN - An existing track */
	omfUInt32		*physicalTrackNum) 	/* IN - A physical track number */
{
    omfObject_t 	trackDesc = NULL;
	omfErr_t		status;
	
	omfAssertValidFHdl(file);
	omfAssert((track != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((file->setrev != kOmfRev1x) && (file->setrev != kOmfRevIMA), file, OM_ERR_NOT_IN_15);
	XPROTECT(file)
	{
		XASSERT(omfiMobSlotIsTrack(file, track, &status), OM_ERR_NOT_A_TRACK);
		
		CHECK(omfsReadObjRef(file, track, OMMSLTTrackDesc, &trackDesc));
		if(omfsReadUInt32(file, trackDesc, OMTRKDPhysTrack, physicalTrackNum) != OM_ERR_NONE)
			*physicalTrackNum = 0;
	} /* XPROTECT */
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiComponentGetInfo()
 *
 * 		This function returns the datakind and length properties
 *      common to all component properties.  For 1.x component objects,
 *      the function will convert the trackkind to a "dummy" datakind
 *      object that can be passed into other 2.x APIs that take a
 *      datakind object as an argument.  See ComponentGetLength() for
 *      a description of how the length is acquired for 1.x objects.
 *
 * Argument Notes:
 *      A NULL should be specified if an argument value is not requested.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiComponentGetInfo(
	omfHdl_t file,          /* IN - File Handle */
    omfCpntObj_t component, /* IN - Component object */
    omfDDefObj_t *datakind, /* OUT - Datakind definition object */
    omfLength_t *length)    /* OUT - Length of component */
{
	omfTrackType_t trackType;

	omfAssertValidFHdl(file);
	omfAssert((component != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		if (datakind)
		  {
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				CHECK(omfsReadTrackType(file, component, OMCPNTTrackKind, 
										&trackType));
				*datakind = TrackTypeToMediaKind(file, trackType);
			  }
			else /* kOmfRev2x */
			  {
				CHECK(omfsReadObjRef(file, component, OMCPNTDatakind, 
									 datakind));
			  }
		  }
		if (length)
		  {
			/* This call should handle both the 1.x and 2.x cases */
			CHECK(omfiComponentGetLength(file, component, length));
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

 	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiDataValueGetSize()
 *
 * 		This function returns the size of the input data value.
 *
 *      This function does not support 1.x files or objects.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiDataValueGetSize(
    omfHdl_t file,       /* IN - File Handle */
	omfObject_t value,   /* IN - Data value object */
    omfLength_t *valueSize)  /* OUT - Value size */
{
	CMValue cvalue;
	CMProperty cprop;
	CMType ctype;
	omfProperty_t prop, idProp;
	omfClassID_t classID;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((value != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((valueSize != NULL), file, OM_ERR_NULL_PARAM);
	if (valueSize)
	  omfsCvtInt32toInt64(0, valueSize);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		/* Get ClassID to determine which property to use */
		CHECK(omfsReadClassID(file, value, idProp, classID));
		if (streq(classID, "CVAL"))
		  prop = OMCVALValue;
		else if (streq(classID, "CTLP"))
		  prop = OMCTLPValue;
		else
		  {
			RAISE(OM_ERR_INVALID_OBJ);
		  }

		/* Use Bento to get the size of the data value */
		cprop = CvtPropertyToBento(file, prop);
		ctype = CvtTypeToBento(file, OMDataValue, NULL);
		cvalue = CMUseValue((CMObject)value, cprop, ctype);
		*valueSize = CMGetValueSize(cvalue);
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: GetDataValue()
 *
 * 		<Function Description>
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t GetDataValue(
    omfHdl_t file,       /* IN - File Handle */
	omfObject_t value,   /* IN - Data value object */
	omfProperty_t prop,  /* IN - Data Value property */
	omfDDefObj_t datakind, /* IN - Datakind object */
    omfInt32 valueSize,  /* IN - Size of value data */
	omfInt32 *bytesRead, /* OUT - Bytes Read */
    void *data)          /* OUT - Value data */
{
	CMValue cvalue;
	CMProperty cprop;
	CMType ctype;
	omfUInt32	bytesRet;
	omfPosition_t	zeroPos;
	
	omfsCvtInt32toPosition(0, zeroPos);	
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((value != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		cprop = CvtPropertyToBento(file, prop);
		ctype = CvtTypeToBento(file, OMDataValue, NULL);
		cvalue = CMUseValue((CMObject)value, cprop, ctype);

		CHECK(omfsReadDataValue(file, value, prop, datakind,
								data, zeroPos, valueSize, &bytesRet));
		*bytesRead = bytesRet;

	  } /* XPROTECT */
	
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiXXXGetInfo()
 *
 * 		This set of functions returns the required property values for the
 *      object identified by XXX.  If a value is not requested, a NULL
 *      should be passed into the parameter corresponding to that property.
 *      For example --
 *            omfiMobSlotGetInfo(file, slotObj, NULL, &segment);
 *      would not return the editRate property from a mob slot.
 *      Additional functions are provided to return optional properties.
 *
 *      These functions support both 1.x and 2.x objects with the following
 *      mappings and exceptions.
 *      Mappings:
 *           omfiMobSlotGetInfo() - returns information from 1.x TRAK object
 *      Not Supported for 1.x:
 *           omfiNestedScopeGetInfo()
 *           omfiConstValueGetInfo()
 *           omfiVaryValueGetInfo()
 *           omfiControlPtGetInfo()
 *           omfiEffectGetInfo()
 *           omfiEffectDefGetInfo()
 *           omfiEffectSlotGetInfo()
 *           
 * Argument Notes:
 *      See individual functions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/

/*************************************************************************
 * Function: omfiMobSlotGetInfo()
 *************************************************************************/
omfErr_t omfiMobSlotGetInfo(
	omfHdl_t file,           /* IN - File Handle */
	omfMSlotObj_t slot,      /* IN - Mob Slot Object */
   	omfRational_t *editRate, /* OUT - Edit Rate property value */
   	omfSegObj_t *segment)    /* OUT - Segment property value (an objref) */
{
    omfObject_t tmpSegment = NULL;

	if (segment)
	  *segment = NULL;
	omfAssertValidFHdl(file);
	omfAssert((slot != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			if (editRate)
			  {
				CHECK(omfsReadObjRef(file, slot, OMTRAKTrackComponent, 
									 &tmpSegment));
				CHECK(omfsReadExactEditRate(file, tmpSegment, OMCPNTEditRate,
											editRate));
			  }
			if (segment)
			  {
				CHECK(omfsReadObjRef(file, slot, OMTRAKTrackComponent, 
									 segment));
			  }
		  }

		else /* kOmfRev2x */
		  {
			if (editRate)
			  {
				CHECK(omfsReadRational(file, slot, OMMSLTEditRate, editRate));
			  }
			if (segment)
			  {
				CHECK(omfsReadObjRef(file, slot, OMMSLTSegment, segment));
			  }
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiTrackGetInfo()
 *************************************************************************/
omfErr_t omfiTrackGetInfo(
	omfHdl_t file,           /* IN - File Handle */
	omfMobObj_t mob,         /* IN - Mob object (for 1.x trackID compat) */
	omfMSlotObj_t slot,      /* IN - Mob Slot object */
   	omfRational_t *editRate, /* OUT - Editrate property value */
   	omfInt32 nameSize,       /* IN - Size of name buffer */
	omfString name,          /* IN/OUT - preallocated buffer for name */
	omfPosition_t *origin,   /* OUT - Origin property value */
	omfTrackID_t *trackID,   /* OUT - TrackID property value */
   	omfSegObj_t *segment)    /* OUT - Segment property value (objref) */
{
	omfPosition_t tmpOrigin;
	omfTrackID_t tmpTrackID;
	omfRational_t tmpEditRate;
	omfObject_t tmpSegment = NULL;

	omfAssertValidFHdl(file);
	omfAssert((slot != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* These API calls should handle both the 1.x and 2.x cases */
		CHECK(omfiMobSlotGetInfo(file, slot, &tmpEditRate, &tmpSegment));

		if (editRate)
		  {
			(*editRate).numerator = tmpEditRate.numerator;
			(*editRate).denominator = tmpEditRate.denominator;
		  }
		if (segment)
		  {
			*segment = tmpSegment;
		  }
		if (name)
		  {
			CHECK(MobSlotGetTrackDesc(file, mob, slot, nameSize,
										  name, &tmpOrigin, &tmpTrackID));
		  }
		else
		  {
			CHECK(MobSlotGetTrackDesc(file, mob, slot, 0, NULL, &tmpOrigin,
										  &tmpTrackID));
		  }
		if (origin)
		  *origin = tmpOrigin;
		if (trackID)
		  *trackID = tmpTrackID;
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiFillerGetInfo()
 *************************************************************************/
omfErr_t omfiFillerGetInfo(
	omfHdl_t file,          /* IN - File Handle */
    omfCpntObj_t component, /* IN - Filler object */
    omfDDefObj_t *datakind, /* OUT - Datakind definition object */
    omfLength_t *length)    /* OUT - Length of filler */
{
	omfAssertValidFHdl(file);
	omfAssert((component != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, component, datakind, length));
	  }
	XEXCEPT
	  {
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSequenceGetInfo()
 *************************************************************************/
omfErr_t omfiSequenceGetInfo(
	omfHdl_t file,          /* IN - File Handle */
    omfCpntObj_t component, /* IN - Sequence object */
    omfDDefObj_t *datakind, /* OUT - Datakind definition object */
    omfLength_t *length)    /* OUT - Length of sequence */
{
	omfAssertValidFHdl(file);
	omfAssert((component != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, component, datakind, length));
	  }
	XEXCEPT
	  {
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSourceClipGetInfo()
 *************************************************************************/
omfErr_t omfiSourceClipGetInfo(
	omfHdl_t file,             /* IN - File Handle */
    omfCpntObj_t component,    /* IN - Source Clip object */
    omfDDefObj_t *datakind,    /* OUT - Datakind definition object */
    omfLength_t *length,       /* OUT - Length of source clip */
	omfSourceRef_t *sourceRef) /* OUT - Source reference (sourceID, 
								*       sourceTrackID, startTime)
								*/
{
	omfAssertValidFHdl(file);
	omfAssert((component != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, component, datakind, length));

		CHECK(SourceClipGetRef(file, component, sourceRef));
	  }
	XEXCEPT
	  {
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiNestedScopeGetInfo()
 *************************************************************************/
omfErr_t omfiNestedScopeGetInfo(
	omfHdl_t file,          /* IN - File Handle */
    omfCpntObj_t component, /* IN - Nested Scope object */
    omfDDefObj_t *datakind, /* OUT - Datakind definition object */
    omfLength_t *length)    /* OUT - Length of nested scope */
{
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((component != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, component, datakind, length));
	  }
	XEXCEPT
	  {
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiConstValueGetInfo()
 * 
 * Argument Notes:
 *      The data value is returned in a preallocated buffer of size
 *      valueSize.  The actual number of bytes read is returned in
 *      bytesRead.  If the buffer is not big enough to return the entire
 *      value, an error is returned.  omfiDataValueGetSize() can be
 *      called to calculate the size of the buffer to allocate.
 *************************************************************************/
omfErr_t omfiConstValueGetInfo(
    omfHdl_t file,          /* IN - File Handle */
	omfObject_t constValue, /* IN - Constant value object */
	omfDDefObj_t *datakind, /* OUT - Datakind definition object */
	omfLength_t *length,    /* OUT - Length of constant value */
	omfInt64 valueSize,     /* IN - Size of preallocated buffer */
	omfInt64 *bytesRead,    /* OUT - Number of actual bytes read */
	void *value)            /* IN/OUT - Preallocated buffer to hold value */
{
    omfLength_t tmpLength;
	omfDDefObj_t tmpDatakind = NULL;
	omfInt32 valueSize32, bytesRead32;

	*datakind = NULL;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((constValue != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, constValue, &tmpDatakind, &tmpLength));
		if (length)
		  *length = tmpLength;
		if (datakind)
		  *datakind = tmpDatakind;

		if (value)
		  {
			CHECK(omfsTruncInt64toInt32(valueSize, &valueSize32)); /* OK EFFECTPARM */
			CHECK(GetDataValue(file, constValue, OMCVALValue, tmpDatakind,
							   valueSize32,
							   &bytesRead32, value));		  
			omfsCvtInt32toInt64(bytesRead32, bytesRead);
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiVaryValueGetInfo()
 *************************************************************************/
omfErr_t omfiVaryValueGetInfo(
    omfHdl_t file,          /* IN - File Handle */
	omfSegObj_t varyValue,  /* IN - Varying Value Object */
	omfDDefObj_t *datakind, /* OUT - Datakind definition object */
	omfLength_t *length,    /* OUT - Length of varying value object */
	omfInterpKind_t *interpolation) /* OUT - Interpolation method */
{
    omfLength_t tmpLength;
    omfDDefObj_t tmpDatakind = NULL;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((varyValue != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, varyValue, &tmpDatakind, &tmpLength));
		if (length)
		  *length = tmpLength;
		if (datakind)
		  *datakind = tmpDatakind;

		if (interpolation)
		  {
			CHECK(omfsReadInterpKind(file, varyValue, OMVVALInterpolation,
									 interpolation));
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiControlPtGetInfo()
 *
 * Argument Notes:
 *      The data value is returned in a preallocated buffer of size
 *      valueSize.  The actual number of bytes read is returned in
 *      bytesRead.  If the buffer is not big enough to return the entire
 *      value, an error is returned.  omfiDataValueGetSize() can be
 *      called to calculate the size of the buffer to allocate.
 *************************************************************************/
omfErr_t omfiControlPtGetInfo(
    omfHdl_t file,            /* IN - File Handle */
	omfCntlPtObj_t controlPt, /* IN - Control Point object */
	omfRational_t *time,      /* OUT - Time of control point */
	omfEditHint_t *editHint,  /* OUT - Edit hint */
	omfDDefObj_t *datakind,   /* OUT - Datakind definition object */
	omfInt32 valueSize,       /* IN - Size of preallocated buffer */
    omfInt32 *bytesRead,      /* OUT - Number of actual bytes read */
	void *value)              /* IN/OUT - Preallocated buffer to hold value */
{
	omfDDefObj_t tmpDatakind = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((controlPt != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		if (time)
		  {
			CHECK(omfsReadRational(file, controlPt, OMCTLPTime, time));
		  }

		if (editHint)
		  {
			omfError = omfsReadEditHint(file, controlPt, OMCTLPEditHint, 
										editHint);
			if (omfError == OM_ERR_PROP_NOT_PRESENT)
			  *editHint = kNoEditHint;
			else 
			  {
				CHECK(omfError);
			  }
		  }

		CHECK(omfsReadObjRef(file, controlPt, OMCTLPDatakind, &tmpDatakind));
		if (datakind)
		  {
			*datakind = tmpDatakind;
		  }
		if (value)
		  {
			CHECK(GetDataValue(file, controlPt, OMCTLPValue, tmpDatakind,
							   valueSize, bytesRead, value));
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobGetInfo()
 *************************************************************************/
omfErr_t omfiMobGetInfo(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob object */
	omfUID_t *mobID,     /* OUT - Mob ID */
	omfInt32 strSize,    /* IN - Size of name buffer */
	omfString name,      /* IN/OUT - Preallocated buffer to hold name */
	omfTimeStamp_t *lastModified, /* OUT - Last Modified date */
	omfTimeStamp_t *creationTime) /* OUT - Creation Time */
{
    omfProperty_t prop;
    omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* This will work for both 1.x and 2.x */
		if (mobID)
		  {
			CHECK(omfiMobGetMobID(file, mob, mobID));
		  }

		if (name)
		  {
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  prop = OMCPNTName;
			else /* kOmfRev2x */
			  prop = OMMOBJName;

			omfError = omfsReadString(file, mob, prop, (char *)name,
									  strSize);
			if (omfError == OM_ERR_PROP_NOT_PRESENT)
			  strncpy(name, "\0", 1);
			else if (omfError != OM_ERR_NONE)
			  {
				RAISE(omfError);
			  }
		  }

		/* This will work for either 1.x or 2.x */
		if (lastModified)
		  {
			CHECK(omfsReadTimeStamp(file, mob, OMMOBJLastModified, 
									lastModified));
		  }
		if (creationTime)
		  {
			/* CreationTime doesn't exist on 1.x mobs */
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				(*creationTime).TimeVal = 0;
				(*creationTime).IsGMT = FALSE;
			  }
			else /* kOmfRev2x */
			  {
				CHECK(omfsReadTimeStamp(file, mob, OMMOBJCreationTime, 
										creationTime));
			  }
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE); 
}

/*************************************************************************
 * Function: omfiTimecodeGetInfo()
 *************************************************************************/
omfErr_t omfiTimecodeGetInfo(
    omfHdl_t file,            /* IN - File Handle */
    omfSegObj_t timecodeClip, /* IN - Timecode Clip object */
    omfDDefObj_t *datakind,   /* OUT - Datakind definition object */
	omfLength_t *length,      /* OUT - Length of timecode clip */
	omfTimecode_t *timecode)  /* OUT - Timecode (startFrame, drop, fps) */
{
    omfDDefObj_t tmpDatakind = NULL;
	omfLength_t tmpLength;
	omfInt16 tmp1xFps = 0;
	omfInt32 tmp1xFlags = 0, tmp1xStartTC = 0;
	omfPosition_t position;
	omfBool		dropBool, checkState;
	
	omfAssertValidFHdl(file);
	omfAssert((timecodeClip != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, timecodeClip, &tmpDatakind, 
									&tmpLength));
		if (length)
		  *length = tmpLength;
		if (datakind)
		  *datakind = tmpDatakind;

		/* Many type and name changes from 1.x -> 2.x */
		if (timecode)
		  {
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				CHECK(omfsReadInt32(file, timecodeClip, OMTCCPStartTC,
								   &tmp1xStartTC));
				(*timecode).startFrame = tmp1xStartTC;
				CHECK(omfsReadInt16(file, timecodeClip, OMTCCPFPS,
									&tmp1xFps));
				(*timecode).fps = (omfUInt16)tmp1xFps;
				if(omfsReadInt32(file, timecodeClip, OMTCCPFlags,
								   &tmp1xFlags) == OM_ERR_NONE)
					(*timecode).drop = (omfDropType_t)tmp1xFlags;
				else
				{
#if OMFI_ENABLE_SEMCHECK
					checkState = omfsIsSemanticCheckOn(file);
					omfsSemanticCheckOff(file);
#endif
					if(omfsReadUInt32(file, timecodeClip, OMTCCPFlags,
								   (omfUInt32 *)&tmp1xFlags) == OM_ERR_NONE)
						(*timecode).drop = (omfDropType_t)tmp1xFlags;
					else
						(*timecode).drop = (omfDropType_t)0;	/* missing from a composer file */
#if OMFI_ENABLE_SEMCHECK
					if(checkState)
						omfsSemanticCheckOn(file);
#endif
				}
			  }
			else /* kOmfRev2x */
			  {
				CHECK(omfsReadPosition(file, timecodeClip, OMTCCPStart,
								   &position));
				CHECK(omfsTruncInt64toUInt32(position, &((*timecode).startFrame)));		/* OK FRAMEOFFSET */
				CHECK(omfsReadUInt16(file, timecodeClip, OMTCCPFPS,
									 &((*timecode).fps)));
				CHECK(omfsReadBoolean(file, timecodeClip, OMTCCPDrop,
									   &dropBool));
				(*timecode).drop = (dropBool ? kTcDrop : kTcNonDrop);
			  }
		  }
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEdgecodeGetInfo()
 *************************************************************************/
omfErr_t omfiEdgecodeGetInfo(
    omfHdl_t file,            /* IN - File Handle */
	omfSegObj_t edgecodeClip, /* IN - Edgecode Clip object */
    omfDDefObj_t *datakind,   /* OUT - Datakind definition object */
	omfLength_t *length,      /* OUT - Length of edgecode clip */
	omfEdgecode_t *edgecode)  /* OUT - Edgecode (startFrame, filmKind, 
							   * codeFormat)
							   */
{
    omfDDefObj_t tmpDatakind = NULL;
    omfLength_t tmpLength;
    omfPosition_t tmpPos, zeroPos;
	omfInt32 tmp1xStartEC = 0;
	omfUInt32		n, bytesRead;
	omfErr_t	omfError;
	
	omfAssertValidFHdl(file);
	omfAssert((edgecodeClip != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		omfsCvtInt32toInt64(0, &zeroPos);
		CHECK(omfiComponentGetInfo(file, edgecodeClip, &tmpDatakind, 
									&tmpLength));
		if (length)
		  *length = tmpLength;
		if (datakind)
		  *datakind = tmpDatakind;
		if (edgecode)
		  {
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				CHECK(omfsReadInt32(file, edgecodeClip, OMECCPStartEC,
								   &tmp1xStartEC));
				(*edgecode).startFrame = tmp1xStartEC;
				omfError = omfsReadVarLenBytes(file, edgecodeClip, OMECCPHeader,
									   zeroPos, 8, edgecode->header, &bytesRead);
				
			  }
			else /* kOmfRev2x */
			  {
				CHECK(omfsReadPosition(file, edgecodeClip, OMECCPStart,
								   &tmpPos));
				CHECK(omfsTruncInt64toUInt32(tmpPos, &((*edgecode).startFrame)));		/* OK FRAMEOFFSET */
				omfError = omfsReadDataValue(file, edgecodeClip, OMECCPHeader,
									   *datakind, edgecode->header, zeroPos, 8, &bytesRead);
			  }
			if(omfError != OM_ERR_NONE)
			{
				for(n = 0; n < sizeof(edgecode->header); n++)
					edgecode->header[n] = '\0';
			}
			CHECK(omfsReadFilmType(file, edgecodeClip, OMECCPFilmKind,
								   &((*edgecode).filmKind)));
			CHECK(omfsReadEdgeType(file, edgecodeClip, OMECCPCodeFormat,
								   &((*edgecode).codeFormat)));
		  }
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectGetInfo()
 *************************************************************************/
omfErr_t omfiEffectGetInfo(
    omfHdl_t file,          /* IN - File Handle */
	omfEffObj_t effect,     /* IN - Effect object */
	omfDDefObj_t *datakind, /* OUT - Datakind definition object */
	omfLength_t *length,    /* OUT - Length of Effect object */
	omfEDefObj_t *effectDef)/* OUT - Effect definition object (objref) */
{
    omfDDefObj_t tmpDatakind = NULL;
	omfLength_t tmpLength;

	omfAssertValidFHdl(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	if (effectDef)
	  *effectDef = NULL;

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, effect, &tmpDatakind, &tmpLength));
		if (length)
		  *length = tmpLength;
		if (datakind)
		  *datakind = tmpDatakind;
		  
		if (effectDef)
		  {
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				*effectDef = NULL;
			  }
			else /* kOmfRev2x */
			  {
				CHECK(omfsReadObjRef(file, effect, OMEFFEEffectKind, 
									 effectDef));
			  }
		  }
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectDefGetInfo()
 *************************************************************************/
omfErr_t omfiEffectDefGetInfo(
    omfHdl_t file,          /* IN - File Handle */
	omfEDefObj_t effectDef, /* IN - Effect definition object */
	omfInt32 idSize,        /* IN - Size of EffectID buffer */
	omfUniqueNamePtr_t effectID, /* IN/OUT - Preallocated buffer for effectID*/
    omfInt32 nameSize,      /* IN - Size of name buffer */
    omfString nameBuf,      /* IN/OUT - Preallocated buffer for name */
    omfInt32 descSize,      /* IN - Size of description buffer */
    omfString descBuf,      /* IN/OUT - Preallocated buffer for description */
    omfArgIDType_t *bypass, /* OUT - Bypass */
    omfBool *isTimeWarp)    /* OUT - Whether or not a timewarp object */
{
    omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssert((effectDef != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		if (effectID)
		  {
			CHECK(omfsReadUniqueName(file, effectDef, OMEDEFEffectID, 
									 effectID, idSize));
		  }
		if (nameBuf)
		  {
			omfError = omfsReadString(file, effectDef, OMEDEFEffectName, 
								 (char *)nameBuf, nameSize);
			if (omfError == OM_ERR_PROP_NOT_PRESENT)
			  strcpy(nameBuf, "\0");
			else if (omfError != OM_ERR_NONE)
			  {
				RAISE(omfError);
			  }
		  }
		if (descBuf)
		  {
			omfError = omfsReadString(file, effectDef, OMEDEFEffectDescription,
								 (char *)descBuf, descSize);
			if (omfError == OM_ERR_PROP_NOT_PRESENT)
			  strcpy(descBuf, "\0");
			else if (omfError != OM_ERR_NONE)
			  {
				RAISE(omfError);
			  }
		  }
		if (bypass)
		  {
			omfError = omfsReadArgIDType(file, effectDef, OMEDEFBypass, 
										 bypass);
			if (omfError == OM_ERR_PROP_NOT_PRESENT)
			  *bypass = 0;
			else if (omfError != OM_ERR_NONE)
			  {
				RAISE(omfError);
			  }
		  }
		if (isTimeWarp)
		  {
			omfError = omfsReadBoolean(file, effectDef, OMEDEFIsTimeWarp,
								  isTimeWarp);
			if (omfError == OM_ERR_PROP_NOT_PRESENT)
			  *isTimeWarp = FALSE;
			else if (omfError != OM_ERR_NONE)
			  {
				RAISE(omfError);
			  }
		  }
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectSlotGetInfo()
 *************************************************************************/
omfErr_t omfiEffectSlotGetInfo(
    omfHdl_t file,          /* IN - File Handle */
	omfESlotObj_t slot,     /* IN - Effect Slot object */
    omfArgIDType_t *argID,  /* OUT - ArgID */
	omfSegObj_t *argValue)  /* OUT - Arg Value object */
{
    omfErr_t omfError = OM_ERR_NONE;

	if (argValue)
	  *argValue = NULL;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((slot != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		if (argID)
		  {
			CHECK(omfsReadArgIDType(file, slot, OMESLTArgID, argID));
		  }
		if (argValue)
		  {
			CHECK(omfsReadObjRef(file, slot, OMESLTArgValue, argValue));
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {		
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSelectorGetInfo()
 *************************************************************************/
omfErr_t omfiSelectorGetInfo(
    omfHdl_t file,          /* IN - File Handle */
	omfSegObj_t selector,   /* IN - Selector object */
	omfDDefObj_t *datakind, /* OUT - Datakind definition object */
	omfLength_t *length,    /* OUT - Length of selector */
	omfSegObj_t *selected)  /* OUT - Selected slot (objref) */
{
    omfDDefObj_t tmpDatakind = NULL;
	omfLength_t tmpLength;
	omfInt16 trackLabel = 0, tmpLabel = 0;
	omfInt32 loop, numSlots;
	omfObject_t slot = NULL;
	omfBool found = FALSE;

	*selected = NULL;
	omfAssertValidFHdl(file);
	omfAssert((selector != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, selector, &tmpDatakind, &tmpLength));
		if (length)
		  *length = tmpLength;
		if (datakind)
		  *datakind = tmpDatakind;

		if (selected)
		  {
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				CHECK(omfsReadInt16(file, selector, OMSLCTSelectedTrack, 
									&trackLabel));

				/* Iterate through the selected tracks looking for trackLabel*/
				numSlots = omfsLengthObjRefArray(file, selector, OMTRKGTracks);
				for (loop = 1; loop <= numSlots; loop++)
				  {
					CHECK(omfsGetNthObjRefArray(file, selector, OMTRKGTracks, 
												&slot, loop));
					CHECK(omfsReadInt16(file, slot, OMTRAKLabelNumber, 
										&tmpLabel));
					if (tmpLabel == trackLabel)
					  {
						CHECK(omfsReadObjRef(file, slot, OMTRAKTrackComponent,
											 selected));
						found = TRUE;
						break;
					  }
					/* Release Bento reference, so the useCount is decremented */
					CMReleaseObject((CMObject)slot);
				  }
			  }
			else /* xOmfRev2x */
			  {
				CHECK(omfsReadObjRef(file, selector, OMSLCTSelected, selected));
			  }
		  }
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiTransitionGetInfo()
 *
 *       The effObj will be NULL for 1.x files.  
 *************************************************************************/
omfErr_t omfiTransitionGetInfo(
    omfHdl_t file,          /* IN - File Handle */
	omfSegObj_t transition, /* IN - Transition object */
	omfDDefObj_t *datakind, /* OUT - Datakind definition object */
	omfLength_t *length,    /* OUT - Length of transition */
   	omfPosition_t *cutPoint,     /* OUT - Cut Point */
    omfEffObj_t *effObj)    /* OUT - Effect used by transition (objref) */
{
    omfDDefObj_t tmpDatakind = NULL;
	omfLength_t tmpLength;
	omfInt32 tmpCutPoint = 0;
	omfInt32 len = 0;
	
	omfAssertValidFHdl(file);
	omfAssert((transition != NULL), file, OM_ERR_NULLOBJECT);
	if (effObj)
	  *effObj = NULL;

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, transition, &tmpDatakind, 
									&tmpLength));
		if (length)
		  *length = tmpLength;
		if (datakind)
		  *datakind = tmpDatakind;

		if (cutPoint)
		  {
			CHECK(omfsReadPosition(file, transition, OMTRANCutPoint, 
									cutPoint));
		  }
		if (effObj)
		  {
			/* For 1.x, return NULL for the effect object. */
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				*effObj = NULL;
			  }
			else /* xOmfRef2x */
			  {
				CHECK(omfsReadObjRef(file, transition, OMTRANEffect, effObj));
			  }
		  }

	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: omfiScopeRefGetInfo()
 *************************************************************************/
omfErr_t omfiScopeRefGetInfo(
	omfHdl_t file,          /* IN - File Handle */
	omfSegObj_t scopeRef,   /* IN - Scope reference object */
	omfDDefObj_t *datakind, /* OUT - Datakind definition object */
	omfLength_t *length,    /* OUT - Length of scope reference */
	omfUInt32      *relScope, /* OUT - Relative scope */
    omfUInt32      *relSlot)  /* OUT - Relative slot */
{
    omfDDefObj_t tmpDatakind = NULL;
	omfLength_t tmpLength;

	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((scopeRef != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, scopeRef, &tmpDatakind, &tmpLength));
		if (length)
		  *length = tmpLength;

		if (datakind)
		  *datakind = tmpDatakind;
		if (relScope)
		  {
			CHECK(omfsReadUInt32(file, scopeRef, OMSREFRelativeScope, 
								 relScope));
		  }
		if (relSlot)
		  {
			CHECK(omfsReadUInt32(file, scopeRef, OMSREFRelativeSlot, 
								 relSlot));
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMediaGroupGetInfo()
 *************************************************************************/
omfErr_t omfiMediaGroupGetInfo(
	omfHdl_t file,          /* IN - File Handle */
    omfSegObj_t segment,    /* IN - Media Group object */
    omfDDefObj_t *datakind, /* OUT - Datakind definition object */
    omfLength_t *length)    /* OUT - Length of media group */
{
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((segment != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiComponentGetInfo(file, segment, datakind, length));
	  }
	XEXCEPT
	  {
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobGetMobID()
 *
 * 		This function returns the unique Mob ID for the input mob.
 *
 *      This function supports both 1.x and 2.x files and objects.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobGetMobID(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob object */
	omfUID_t *mobID)     /* OUT - Mob ID */
{
    omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* This will support both 1.x and 2.x */
		if (mobID)
		  {
				CHECK(omfsReadUID(file, mob, OMMOBJMobID, mobID));
		  }
		else
		  {
			RAISE(OM_ERR_NULL_PARAM);
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSourceClipGetFade()
 *
 * 		This function returns the optional fade information from a 
 *      source clip.  This function only applies to audio source clips.
 *
 * Argument Notes:
 *      A NULL should be passed for an argument value that is not requested.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiSourceClipGetFade(
    omfHdl_t		file,             	/* IN - File Handle */
	omfSegObj_t		sourceClip,    		/* IN - Source clip object */
	omfInt32		*fadeInLen,       	/* OUT - Fade In Length */
	omfFadeType_t	*fadeInType, 		/* OUT - Fade In Type */
	omfBool			*fadeInPresent, 	/* OUT - Fade In Type */
	omfInt32 		*fadeOutLen,      	/* OUT - Fade Out Length */
	omfFadeType_t	*fadeOutType, 		/* OUT - Fade Out Type */
	omfBool			*fadeOutPresent)	/* OUT - Fade In Type */
{
    omfErr_t omfError = OM_ERR_NONE;
	omfPosition_t	inLen64, outLen64;
	omfBool			foundInLen, foundInType, foundOutLen, foundOutType;
#if OMFI_ENABLE_SEMCHECK
	omfBool		enabled;
#endif
	
	omfAssertValidFHdl(file);
	omfAssert((sourceClip != NULL), file, OM_ERR_NULLOBJECT);
	
	XPROTECT(file)
	  {
		/* These functions should for for both 1.x and 2.x */
		foundInLen = FALSE;
		foundInType = FALSE;
		foundOutLen = FALSE;
		foundOutType = FALSE;
		if (fadeInLen)
		{
			if(file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
				omfError = omfsReadInt32(file, sourceClip, OMSCLPFadeInLength, 
								 fadeInLen);
			else
			{
				omfError = omfsReadLength(file, sourceClip, OMSCLPFadeInLength, 
								 &inLen64);
#if OMFI_ENABLE_SEMCHECK
				enabled = omfsIsSemanticCheckOn(file);
				omfsSemanticCheckOff(file);
#endif
				if(omfError != OM_ERR_NONE)
					omfError = omfsReadPosition(file, sourceClip, OMSCLPFadeInLength, 
								 &inLen64);
#if OMFI_ENABLE_SEMCHECK
				if(enabled)
					omfsSemanticCheckOn(file);
#endif
				if (omfError == OM_ERR_NONE)
					omfsTruncInt64toInt32(inLen64, fadeInLen);	/* OK API */
			}
			if (omfError == OM_ERR_PROP_NOT_PRESENT)
			{
				*fadeInLen = 0;
			}
			else if (omfError != OM_ERR_NONE)
			{
				RAISE(omfError);
			}
			else
			{
				foundInLen = TRUE;
			}
		}
		if (fadeInType)
		  {
			if(file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
				omfError = omfsReadInt32(file, sourceClip, OMSCLPFadeInType, 
								   (omfInt32 *)fadeInType);
			else
				omfError = omfsReadFadeType(file, sourceClip, OMSCLPFadeInType, 
								   fadeInType);
			if (omfError == OM_ERR_PROP_NOT_PRESENT)
			{
				*fadeInType = kFadeNone;
			}
			else if (omfError != OM_ERR_NONE)
			  {
				RAISE(omfError);
			  }
			else
			{
				foundInType = TRUE;
			}
		  }
		if (fadeOutLen)
		  {
			if(file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
				omfError = omfsReadInt32(file, sourceClip, OMSCLPFadeOutLength, 
								 fadeOutLen);
			else
			{
				omfError = omfsReadLength(file, sourceClip, OMSCLPFadeOutLength, 
								 &outLen64);
#if OMFI_ENABLE_SEMCHECK
				enabled = omfsIsSemanticCheckOn(file);
				omfsSemanticCheckOff(file);
#endif
				if(omfError != OM_ERR_NONE)
					omfError = omfsReadPosition(file, sourceClip, OMSCLPFadeOutLength, 
								 &outLen64);
#if OMFI_ENABLE_SEMCHECK
				if(enabled)
					omfsSemanticCheckOn(file);
#endif
				if (omfError == OM_ERR_NONE)
					omfsTruncInt64toInt32(outLen64, fadeOutLen);	/* OK API */
			}
			if (omfError == OM_ERR_PROP_NOT_PRESENT)
			  *fadeOutLen = 0;
			else if (omfError != OM_ERR_NONE)
			  {
				RAISE(omfError);
			  }
			else
			{
				foundOutLen = TRUE;
			}
		  }
		if (fadeOutType)
		  {
			if(file->setrev == kOmfRev1x || file->setrev == kOmfRevIMA)
				omfError = omfsReadInt32(file, sourceClip, OMSCLPFadeOutType, 
								   (omfInt32 *)fadeOutType);
			else
				omfError = omfsReadFadeType(file, sourceClip, OMSCLPFadeOutType, 
								   fadeOutType);
			if (omfError == OM_ERR_PROP_NOT_PRESENT)
			  *fadeOutType = kFadeNone;
			else if (omfError != OM_ERR_NONE)
			  {
				RAISE(omfError);
			  }
			else
			{
				foundOutType = TRUE;
			}
		  }
		  if(fadeInPresent)
		  	*fadeInPresent = (foundInLen && foundInType);
		  if(fadeOutPresent)
		  	*fadeOutPresent = (foundOutLen && foundOutType);
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectGetFinalRender()
 *
 * 		This function returns the source clip that represents the
 *      optional final rendering on an effect object.  If this
 *      property does not exist, the error OM_ERR_PROP_NOT_PRESENT will be 
 *      returned.
 *  
 *      This function does not support 1.x files or objects.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiEffectGetFinalRender(
    omfHdl_t file,          /* IN - File Handle */
	omfEffObj_t effect,     /* IN - Effect object */
	omfSegObj_t *sourceClip)/* OUT - Final Rendering Source Clip */
{
	*sourceClip = NULL;
	omfAssertValidFHdl(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((sourceClip != NULL), file, OM_ERR_NULL_PARAM);

	XPROTECT(file)
	  {
		if (file->setrev == kOmfRev2x)
		  {
			CHECK(omfsReadObjRef(file, effect, OMEFFEFinalRendering, 
								 sourceClip));
		  }
		else
		  RAISE(OM_ERR_RENDER_NOT_FOUND);
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectGetWorkingRender()
 *
 * 		This function returns the source clip that represents the
 *      optional working rendering on an effect object.  If this
 *      property does not exist, the error OM_ERR_PROP_NOT_PRESENT will be 
 *      returned.
 *  
 *      This function does not support 1.x files or objects.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiEffectGetWorkingRender(
    omfHdl_t file,           /* IN - File Handle */
	omfEffObj_t effect,      /* IN - Effect object */
	omfSegObj_t *sourceClip) /* OUT - Working Rendering Source Clip */
{
	*sourceClip = NULL;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((sourceClip != NULL), file, OM_ERR_NULL_PARAM);

	XPROTECT(file)
	  {
		CHECK(omfsReadObjRef(file, effect, OMEFFEWorkingRendering,sourceClip));
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectIsATimeWarp()
 *
 * 		This boolean function returns whether or not an effect is
 *      a timewarp effect.
 *  
 *      This function does not support 1.x files or objects.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfBool omfiEffectIsATimeWarp(
    omfHdl_t file,       /* IN - File Handle */
	omfEffObj_t effect,  /* IN - Effect object */
	omfErr_t *omfError)  /* OUT - Error code */
{
    omfEDefObj_t effectDef = NULL;
	omfBool isTimeWarp = FALSE;

	*omfError = OM_ERR_NONE;	
	omfAssertValidFHdlBool(file, omfError, FALSE);
	omfAssertBool((effect != NULL), file, OM_ERR_NULLOBJECT,
				  omfError, FALSE);
	omfAssertNot1xBool(file, omfError, FALSE);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			RAISE(OM_ERR_NOT_IN_15);
		  }

		if (omfsIsTypeOf(file, effect, "EFFE", omfError))
		  {
			CHECK(omfsReadObjRef(file, effect, OMEFFEEffectKind, &effectDef));
			CHECK(omfsReadBoolean(file, effectDef, OMEDEFIsTimeWarp,
								  &isTimeWarp));
			if (isTimeWarp)
			  return(TRUE);
		  }
		else
		  {
			RAISE(OM_ERR_INVALID_OBJ);
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		*omfError = XCODE();
	  }
	XEND_SPECIAL(FALSE);

	*omfError = OM_ERR_NONE;
	return(FALSE); 
}

/*************************************************************************
 * Function: omfiEffectGetBypassOverride()
 *
 * 		This function returns the optional bypass override property
 *      value from the input effect object.  If the property does not
 *      exist, the error OM_ERR_PROP_NOT_PRESENT will be returned.
 *  
 *      This function does not support 1.x files or objects.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiEffectGetBypassOverride(
    omfHdl_t file,       /* IN - File Handle */
	omfEffObj_t effect,  /* IN - Effect object */
	omfArgIDType_t *bypassOverride) /* OUT - Bypass override property value */
{
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((bypassOverride != NULL), file, OM_ERR_NULL_PARAM);

	XPROTECT(file)
	  {
		CHECK(omfsReadArgIDType(file, effect, OMEFFEBypassOverride, 
								bypassOverride));
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: omfiIsAXXXMob()
 *
 * 		This set of boolean functions answer the question "is the input
 *      mob of type XXX", where XXX is either File, Tape, Film, Master,
 *      composition or primary.
 *
 *      These functions support both 1.x and 2.x files.
 *
 * Argument Notes:
 *      If an error occurs, it is returned as the last argument, and the
 *      return value will be FALSE.
 *
 * Return Value:
 *		Boolean (as described above)
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/

/*************************************************************************
 * Function: omfiIsAPrimaryMob()
 *************************************************************************/
omfBool omfiIsAPrimaryMob(
    omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob object */
	omfErr_t *omfError)  /* OUT - Error code */
{
    omfInt32 loop = 1, numMobs;
	omfObject_t tmpMob = NULL;
	omfUID_t mobID, tmpMobID;
	omfObject_t head = NULL;

	*omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE);
	omfAssertBool((mob != NULL), file, OM_ERR_NULLOBJECT, omfError, FALSE);

	XPROTECT(file)
	  {
		/* There are no primary mobs in 1.x */
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			return(FALSE);
		  }

		/* kOmfRev2x */
		CHECK(omfsGetHeadObject(file, &head));
		CHECK(omfiMobGetMobID(file, mob, &mobID));
		numMobs = omfsLengthObjRefArray(file, head, OMHEADPrimaryMobs);

		for (loop=1; loop <= numMobs; loop++)
		  {
			CHECK(omfsGetNthObjRefArray(file, head, OMHEADPrimaryMobs, 
										&tmpMob, loop));
			CHECK(omfiMobGetMobID(file, tmpMob, &tmpMobID));

			/* Release Bento reference, so the useCount is decremented */
			CMReleaseObject((CMObject)tmpMob);

			if (equalUIDs(tmpMobID, mobID))
			  return(TRUE);
		  }
	  }

	XEXCEPT
	  {
		*omfError = XCODE();
	  }
	XEND_SPECIAL(FALSE);

	*omfError = OM_ERR_NONE;
	return(FALSE);  /* Not found */
}

/*************************************************************************
 * Function: omfiIsAMob()
 *************************************************************************/
omfBool omfiIsAMob(
	omfHdl_t file,       /* IN - File Handle */
    omfMobObj_t mob,     /* IN - Mob object */
	omfErr_t *omfError)  /* OUT - Error code */
{
  return(omfsIsTypeOf(file, mob, "MOBJ", omfError));
}

/*************************************************************************
 * Function: omfiIsASourceMob()
 *************************************************************************/
omfBool omfiIsASourceMob(
	omfHdl_t file,       /* IN - File Handle */
    omfMobObj_t mob,     /* IN - Mob object */
	omfErr_t *omfError)  /* OUT - Error code */
{
  omfObject_t mdesc = NULL;
  omfErr_t tmpError = OM_ERR_NONE;

  *omfError = OM_ERR_NONE;
  omfAssertValidFHdlBool(file, omfError, FALSE);
  omfAssertBool((mob != NULL), file, OM_ERR_NULLOBJECT, omfError, FALSE);

  XPROTECT(file)
	{
	  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		{
		  if(!omfsIsPropPresent(file, mob, OMMOBJPhysicalMedia, OMObjRef))
		  	return(FALSE);
		  tmpError = omfsReadObjRef(file, mob, OMMOBJPhysicalMedia, &mdesc);
		  if ((tmpError != OM_ERR_NONE) && 
			  (tmpError != OM_ERR_PROP_NOT_PRESENT))
			RAISE(tmpError);

		  /* Release Bento reference, so the useCount is decremented */
		  CMReleaseObject((CMObject)mdesc);

		  if (mdesc)
			return(TRUE);
		  else
			return(FALSE);
		}
	  else /* if kOmfRev2x */
		return(omfsIsTypeOf(file, mob, "SMOB", omfError));
	}
  
  XEXCEPT
	{
	  if (XCODE() == OM_ERR_PROP_NOT_PRESENT)
		*omfError = OM_ERR_NONE;
	  else
		*omfError = XCODE();
	}
  XEND_SPECIAL(FALSE);
  return(FALSE);
}


/*************************************************************************
 * Function: omfiIsACompositionMob()
 *************************************************************************/
omfBool omfiIsACompositionMob(
	omfHdl_t file,       /* IN - File Handle */
    omfMobObj_t mob,     /* IN - Mob object */
	omfErr_t *omfError)  /* OUT - Error code */
{
  omfObject_t mdesc = NULL;
  omfUsageCode_t usageCode = UC_NULL;
  omfErr_t tmpError = OM_ERR_NONE; 

  *omfError = OM_ERR_NONE;
  omfAssertValidFHdlBool(file, omfError, FALSE);
  omfAssertBool((mob != NULL), file, OM_ERR_NULLOBJECT, omfError, FALSE);

  XPROTECT(file)
	{
	  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		{
		  /* If a Media Descriptor exists, it isn't a composition mob */
		  if(omfsIsPropPresent(file, mob, OMMOBJPhysicalMedia, OMObjRef))
					return(FALSE);

		  /* For a composition mob, it must have a usageCode property
		   * of UC_NULL.
		   */
		  if(omfsIsPropPresent(file, mob, OMMOBJUsageCode, OMUsageCodeType))
			{
			  CHECK(omfsReadUsageCodeType(file, mob, OMMOBJUsageCode, 
										  &usageCode));
			  if ((usageCode == UC_NULL) || (usageCode == UC_SUBCLIP) ||
				  (usageCode == UC_EFFECT) || (usageCode == UC_GROUP) ||
				  (usageCode == UC_GROUPOOFTER) || (usageCode == UC_MOTION))
				return(TRUE);
			  else
				return(FALSE);
			}

			/* Missing usage code is legal, I guess it defaults to 0 (composition)
			 */
		  return(TRUE);
		}
	  else /* if kOmfRev2x */
		return(omfsIsTypeOf(file, mob, "CMOB", omfError));
	}

  XEXCEPT
	{
	  *omfError = XCODE();
	}
  XEND_SPECIAL(FALSE);
  return(FALSE);
}

/*************************************************************************
 * Function: omfiIsAMasterMob()
 *************************************************************************/
omfBool omfiIsAMasterMob(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob object */
	omfErr_t *omfError)  /* OUT - Error code */
{
  omfUsageCode_t usageCode;

  *omfError = OM_ERR_NONE;
  omfAssertValidFHdlBool(file, omfError, FALSE);
  omfAssertBool((mob != NULL), file, OM_ERR_NULLOBJECT, omfError, FALSE);

  XPROTECT(file)
	{
	  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		{
		  CHECK(omfsReadUsageCodeType(file, mob, OMMOBJUsageCode, &usageCode));
		  if ((usageCode == UC_MASTERMOB) || (usageCode == UC_PRECOMPUTE))
			return(TRUE);
		  else
			return(FALSE);
		}
	  else /* if kOmfRev2x */
		return(omfsIsTypeOf(file, mob, "MMOB", omfError));
	} /* XPROTECT */

  XEXCEPT
	{
	  if (XCODE() == OM_ERR_PROP_NOT_PRESENT)
		*omfError = OM_ERR_NONE;
	  else
		*omfError = XCODE();
	}
  XEND_SPECIAL(FALSE);
  return(FALSE);
}

/*************************************************************************
 * Function: omfiIsAFileMob()
 *************************************************************************/
omfBool omfiIsAFileMob(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob object */
	omfErr_t *omfError)  /* OUT - Error code */
{
    omfObject_t mDesc = NULL;
	omfPhysicalMobType_t mobKind;
	omfErr_t tmpError = OM_ERR_NONE;

	*omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE);
	omfAssertBool((mob != NULL), file, OM_ERR_NULLOBJECT, omfError, FALSE);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			if (!omfsIsTypeOf(file, mob, "MOBJ", omfError))
			  return(FALSE);

			tmpError = omfsReadObjRef(file, mob, OMMOBJPhysicalMedia, &mDesc);
			if ((tmpError != OM_ERR_NONE) && 
				(tmpError != OM_ERR_PROP_NOT_PRESENT))
			  RAISE(tmpError);

			if (mDesc)
			  {
				CHECK(omfsReadPhysicalMobType(file, mDesc, OMMDESMobKind,
											  &mobKind));
				/* Release Bento reference, so the useCount is decremented */
				CMReleaseObject((CMObject)mDesc);

				if (mobKind == PT_FILE_MOB)
				  return(TRUE);
				else
				  return(FALSE);
			  }
		  }
		else /* kOmfRev2x */
		  {
			if (!omfsIsTypeOf(file, mob, "SMOB", omfError))
			  return(FALSE);
			if (HasAMediaDesc(file, mob, "MDFL", &tmpError))
			  return(TRUE);
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		*omfError = XCODE();
	  }
	XEND_SPECIAL(FALSE);

	*omfError = OM_ERR_NONE;
	return(FALSE); /* No match */
}

/*************************************************************************
 * Function: omfiIsATapeMob()
 *************************************************************************/
omfBool omfiIsATapeMob(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob object */
	omfErr_t *omfError)  /* OUT - Error code */
{
    omfObject_t mDesc = NULL;
	omfPhysicalMobType_t mobKind;
	omfErr_t tmpError = OM_ERR_NONE;

	*omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE);
	omfAssertBool((mob != NULL), file, OM_ERR_NULLOBJECT, omfError, FALSE);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			if (!omfsIsTypeOf(file, mob, "MOBJ", omfError))
			  return(FALSE);
			tmpError = omfsReadObjRef(file, mob, OMMOBJPhysicalMedia, &mDesc);
			if ((tmpError != OM_ERR_NONE) && 
				(tmpError != OM_ERR_PROP_NOT_PRESENT))
			  RAISE(tmpError);
			if (mDesc)
			  {
				CHECK(omfsReadPhysicalMobType(file, mDesc, OMMDESMobKind,
											  &mobKind));
				/* Release Bento reference, so the useCount is decremented */
				CMReleaseObject((CMObject)mDesc);

				/* If video tape or nagra (audio tape) mob */
				if ((mobKind == PT_TAPE_MOB) || (mobKind == PT_NAGRA_MOB))
				  return(TRUE);
				else
				  return(FALSE);
			  }
		  }
		else /* kOmfRev2x */
		  {
			if (!omfsIsTypeOf(file, mob, "SMOB", omfError))
			  return(FALSE);
			if (HasAMediaDesc(file, mob, "MDTP", &tmpError) || HasAMediaDesc(file, mob, "GMFL", &tmpError))
			  return(TRUE);
		  }

	  } /* XPROTECT */
	
	XEXCEPT
	  {
		*omfError = XCODE();
	  }
	XEND_SPECIAL(FALSE);

	*omfError = OM_ERR_NONE;
	return(FALSE); /* No match */
}

/*************************************************************************
 * Function: omfiIsAFilmMob()
 *************************************************************************/
omfBool omfiIsAFilmMob(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob object */
	omfErr_t *omfError)  /* OUT - Error code */
{
    omfObject_t mDesc = NULL;
	omfPhysicalMobType_t mobKind;
	omfErr_t tmpError = OM_ERR_NONE;

	*omfError = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, omfError, FALSE);
	omfAssertBool((mob != NULL), file, OM_ERR_NULLOBJECT, omfError, FALSE);

	XPROTECT(file)
	  {
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			if (!omfsIsTypeOf(file, mob, "MOBJ", omfError))
			  return(FALSE);
			tmpError = omfsReadObjRef(file, mob, OMMOBJPhysicalMedia, &mDesc);
			if ((tmpError != OM_ERR_NONE) && 
				(tmpError != OM_ERR_PROP_NOT_PRESENT))
			  RAISE(tmpError);
			if (mDesc)
			  {
				CHECK(omfsReadPhysicalMobType(file, mDesc, OMMDESMobKind,
											  &mobKind));
				/* Release Bento reference, so the useCount is decremented */
				CMReleaseObject((CMObject)mDesc);

				if (mobKind == PT_FILM_MOB)
				  return(TRUE);
				else
				  return(FALSE);
			  }
		  }
		else /* kOmfRev2x */
		  {
			if (!omfsIsTypeOf(file, mob, "SMOB", omfError))
			  return(FALSE);
			if (HasAMediaDesc(file, mob, "MDFM", &tmpError))
			  return(TRUE);
		  }
	  } /* XPROTECT */
	
	XEXCEPT
	  {
		*omfError = XCODE();
	  }
	XEND_SPECIAL(FALSE);

	*omfError = OM_ERR_NONE;
	return(FALSE); /* No match */
}

/*************************************************************************
 * Private Function: HasAMediaDesc()
 *
 *      This boolean function is used by the omfiIsAXXXMob() functions to 
 *      determine whether or not a mob is a source mob of a certain
 *      mobkind.  It pulls the media descriptor off of the mob (if it
 *      exists, and compares the classID of the media descriptor to
 *      the input mdescClass.
 *    
 *      This function supports both 1.x and 2.x files and objects.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Boolean (as described above).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfBool HasAMediaDesc(
	omfHdl_t file,              /* IN - File Handle */
    omfMobObj_t mob,            /* IN - Mob object */
	omfClassIDPtr_t mdescClass, /* IN - Media Descriptor class string */
    omfErr_t *omfError)         /* OUT - Error code */
{
  omfObject_t mdesc = NULL;
  omfBool found = FALSE;

  *omfError = OM_ERR_NONE;
  omfAssertValidFHdlBool(file, omfError, FALSE);
  omfAssertBool((mob != NULL), file, OM_ERR_NULLOBJECT, omfError, FALSE);

  XPROTECT(file)
	{
	  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		{
		  CHECK(omfsReadObjRef(file, mob, OMMOBJPhysicalMedia, &mdesc));
		}
	  else /* if kOmfRev2x */
		{
		  CHECK(omfsReadObjRef(file, mob, OMSMOBMediaDescription, &mdesc));
		}

	  if (omfsIsTypeOf(file, mdesc, mdescClass, omfError))
		{
		  if (*omfError == OM_ERR_NONE)
			found = TRUE;
		  else
			{
			  RAISE(*omfError);
			}
		}

	  /* Release Bento reference, so the useCount is decremented */
	  CMReleaseObject((CMObject)mdesc);

	}

  XEXCEPT
	{
	  *omfError = XCODE();
	}
  XEND_SPECIAL(FALSE);

  return(found);  
}


/*************************************************************************
 * Private Function: SourceClipGetRef()
 *
 * 		This function returns the 3 properties on a source Clip that
 *      make of the "source reference" (sourceID, sourceTrackID, and
 *      startTime).
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t SourceClipGetRef(
    omfHdl_t file,             /* IN - File Handle */
	omfSegObj_t sourceClip,    /* IN - Source clip object */
	omfSourceRef_t *sourceRef) /* OUT - Source reference */
{
    omfInt16 tmpTrackID;
	omfInt32 tmpSourcePosition;
    omfErr_t omfError = OM_ERR_NONE;

	omfAssertValidFHdl(file);
	omfAssert((sourceClip != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		if (sourceRef)
		  {
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				omfError = omfsReadUID(file, sourceClip, OMSCLPSourceID,
								  &((*sourceRef).sourceID));
			    if (omfError == OM_ERR_PROP_NOT_PRESENT)
					initUID((*sourceRef).sourceID, 0, 0, 0);

				omfError = omfsReadInt16(file, sourceClip, OMSCLPSourceTrack,
										 &tmpTrackID);
				if (omfError == OM_ERR_PROP_NOT_PRESENT)
				  (*sourceRef).sourceTrackID = 0;
				else if (omfError == OM_ERR_NONE)
				  {
					(*sourceRef).sourceTrackID = (omfTrackID_t)tmpTrackID;
				  }

				omfError = omfsReadInt32(file, sourceClip, OMSCLPSourcePosition,
										&tmpSourcePosition);
				if (omfError == OM_ERR_PROP_NOT_PRESENT)
				  omfsCvtInt32toInt64(0, &(*sourceRef).startTime);
				else if (omfError == OM_ERR_NONE)
				  omfsCvtInt32toInt64(tmpSourcePosition, &(*sourceRef).startTime);
			  }
			else /* kOmfRev2x */
			  {
				CHECK(omfsReadUID(file, sourceClip, OMSCLPSourceID, 
								  &((*sourceRef).sourceID)));
				CHECK(omfsReadUInt32(file, sourceClip, OMSCLPSourceTrackID,
									&((*sourceRef).sourceTrackID)));
				CHECK(omfsReadPosition(file, sourceClip, OMSCLPStartTime,
								   &((*sourceRef).startTime)));
			  }
		  } /* if sourceRef */
		else
		  {
			RAISE(OM_ERR_NULL_PARAM);
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: FindMobInIndex()
 *
 * 		This function searches through the HEAD index specified by
 *      indexProp for the mob with mobID.  If it is found, the index
 *      position is returned through foundIndex, and a pointer to the
 *      mob is returned through the mob parameter.  If the mob is not
 *      found, the OM_ERR_MOB_NOT_FOUND error is returned.
 *
 *      This function works for both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t FindMobInIndex(
	omfHdl_t file,            /* IN - File Handle */
    omfUID_t mobID,           /* IN - Mob ID to search for */
    omfProperty_t indexProp,  /* IN - Property name of index to search */
	omfInt32 *foundIndex,     /* OUT - Index position where mob was found */
    omfObject_t *mob)         /* OUT - Found mob */
{
  omfInt32 loop, tmpNumMobs = 0, totalMobs;
  omfUID_t tmpMobID;
  omfObject_t tmpMob = NULL;
  omfBool foundMob = FALSE;
  omfObject_t head = NULL;
  omfObjIndexElement_t elem;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(file)
	{
	  *foundIndex = 0;
	  CHECK(omfsGetHeadObject(file, &head));

	  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		totalMobs = omfsLengthObjIndex(file, head, indexProp);
	  else
		totalMobs = omfsLengthObjRefArray(file, head, indexProp);

	  for (loop=1; loop <= totalMobs; loop++)
		{
		  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			{
			  CHECK(omfsGetNthObjIndex(file, head, indexProp, &elem, loop));
			  tmpMob = elem.Mob;
			  copyUIDs(tmpMobID, elem.ID);
			}
		  else /* kOmfRev2x */
			{
			  CHECK(omfsGetNthObjRefArray(file, head, indexProp, &tmpMob, 
										  loop));
			  CHECK(omfiMobGetMobID(file, tmpMob, &tmpMobID));
			}

		  if (equalUIDs(mobID, tmpMobID))
			{
			  foundMob = TRUE;
			  *foundIndex = loop;
			  break;
			}
		  else /* Release Bento reference, so the useCount is decremented */
			CMReleaseObject((CMObject)tmpMob);
		} /* for loop */

	  if (foundMob)
		*mob = tmpMob;
	  else
		{
		  RAISE(OM_ERR_MOB_NOT_FOUND);
		}
	} /* XPROTECT */

  XEXCEPT
	{
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSourceClipResolveRef()
 *
 * 		Given a source clip object, this function returns a pointer to
 *      the mob that it references.  If this mob does not exist, the
 *      error OM_ERR_MOB_NOT_FOUND is returned.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiSourceClipResolveRef(
    omfHdl_t file,           /* IN - File Handle */
	omfSegObj_t sourceClip,  /* IN - Source Clip object */
	omfMobObj_t *mob)        /* OUT - Referenced mob */
{
    omfSourceRef_t sourceRef;
	omfObject_t head = NULL;
	omfMobObj_t tmpMob = NULL;
	mobTableEntry_t	*entry;
	omfInt32 index = 0;
	omfErr_t omfError = OM_ERR_NONE;

	*mob = NULL;
	omfAssertValidFHdl(file);

	XPROTECT(file)
	  {
		if (!mob)
		  {
			RAISE(OM_ERR_NULL_PARAM);
		  }

		/* NOTE: This function should really be doing more checking,
		 * i.e., does track exist in mob, does position (adjusted by
		 * editrate) exist in track, etc.
		 */

		CHECK(SourceClipGetRef(file, sourceClip, &sourceRef));

		CHECK(omfsGetHeadObject(file, &head));

		/* Get the mob out of the mob hash table */
		entry = (mobTableEntry_t *)omfsTableUIDLookupPtr(file->mobs, 
												sourceRef.sourceID);
		if (entry != NULL)
		  tmpMob = entry->mob; 

		if (tmpMob)
		  *mob = tmpMob;
		else
		  {
			RAISE(OM_ERR_MOB_NOT_FOUND);
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

omfErr_t FindTrackByTrackID(omfHdl_t file, 
							omfMobObj_t mob, 
							omfTrackID_t trackID, 
							omfObject_t *destTrack)
{
  omfIterHdl_t mobIter = NULL;
  omfInt32 numTracks, loop;
  omfObject_t tmpTrack = NULL;
  omfTrackID_t tmpTrackID;
  omfBool foundTrack = FALSE;

  XPROTECT(file)
	{
	  *destTrack = NULL;

	  CHECK(omfiIteratorAlloc(file, &mobIter));
	  CHECK(omfiMobGetNumTracks(file, mob, &numTracks));
	  for (loop = 1; loop <= numTracks; loop++)
		{
		  CHECK(omfiMobGetNextTrack(mobIter, mob, NULL, &tmpTrack));
		  CHECK(omfiTrackGetInfo(file, mob, tmpTrack, NULL, 0, NULL, 
								 NULL, &tmpTrackID, NULL));
		  if (tmpTrackID == trackID)
			{
			  foundTrack = TRUE;
			  break;
			}
		}
	  CHECK(omfiIteratorDispose(file, mobIter));
	mobIter = NULL;
	  if (!foundTrack)
		{
		  RAISE(OM_ERR_TRACK_NOT_FOUND);
		}
	  *destTrack = tmpTrack;
	}
  XEXCEPT
	{
		if (mobIter)
		  omfiIteratorDispose(file, mobIter);
	}
  XEND;

  return(OM_ERR_NONE);
}
	
/*************************************************************************
 * Private Function: CountMobInIndex
 *
 * 		This function counts the number of mobKind mobs in the HEAD
 *      index identified by indexProp.  totalMobs gives the total number
 *      of mobs in the index.
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t CountMobInIndex(
	omfHdl_t file,           /* IN - File Handle */
    omfMobKind_t mobKind,    /* IN - Mob Kind to search for */
    omfProperty_t indexProp, /* IN - Property name of HEAD index */
	omfInt32 totalMobs,      /* IN - Total number of mobs in index */
    omfInt32 *numMobs)       /* OUT - Count of mobKind mobs in index */
{
  omfInt32 loop, tmpNumMobs = 0;
  omfObject_t tmpMob = NULL;
  omfBool foundMob = FALSE;
  omfObject_t head = NULL;
  omfObjIndexElement_t elem;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(file)
	{
	  CHECK(omfsGetHeadObject(file, &head));

	  for (loop=1; loop <= totalMobs; loop++)
		{
		  if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			{
			  CHECK(omfsGetNthObjIndex(file, head, indexProp, &elem,
										  loop));
			  tmpMob = elem.Mob;
			}
		  else /* kOmfRev2x */
			{
			  CHECK(omfsGetNthObjRefArray(file, head, indexProp, &tmpMob, 
										  loop));
			}
		  switch (mobKind) 
			{
			case kCompMob:
			  if (omfiIsACompositionMob(file, tmpMob, &omfError))
				foundMob = TRUE;
			  break;

			case kMasterMob:
			  if (omfiIsAMasterMob(file, tmpMob, &omfError))
				foundMob = TRUE;
			  break;
					
			case  kFileMob:
			  if (omfiIsAFileMob(file, tmpMob, &omfError))
				foundMob = TRUE;
			  break;
					
			case kTapeMob:
			  if (omfiIsATapeMob(file, tmpMob, &omfError))
				foundMob = TRUE;
			  break;
					
			case kFilmMob:
			  if (omfiIsAFilmMob(file, tmpMob, &omfError))
				foundMob = TRUE;
			  break;

			default:
			  {
				RAISE(OM_ERR_INVALID_MOBTYPE);
			  }
			} /* switch */
			
		  if (omfError == OM_ERR_NONE)
			{
			  if (foundMob)
				{
				  tmpNumMobs++;
				  foundMob = FALSE;
				}
			}
		  else /* If error, break out of loop */
			{
			  RAISE(omfError);
			}

		  /* Release Bento reference, so the useCount is decremented */
		  CMReleaseObject((CMObject)tmpMob);
		} /* for loop */
	} /* XPROTECT */

  XEXCEPT
	{
	  return(XCODE());
	}
  XEND;

  *numMobs = tmpNumMobs;
  return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiGetNumXXX()
 *
 * 		This function returns the number of XXX in the containing
 *      object.  GetNum functions are provided for mobs in a file,
 *      tracks and slots in a mob, slots in a scope, alternate slots in a 
 *      selector, components in a sequence, slots in an effect,
 *      renderings in an effect, and control points in a varying value.
 *      The resulting number is returned as the last argument.
 *
 *      It supports both 1.x and 2.x files with the following exceptions:
 *      Not Supported for 1.x:
 *           omfiNestedScopeGetNumSlots()
 *           omfiEffectGetNumSlots()
 *           omfiEffectGetNumWorkingRenders()
 *           omfiEffectGetNumFinalRenders()
 *           omfiVaryValueGetNumPoints()
 *
 * Argument Notes:
 *      See individual functions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/

/*************************************************************************
 * Function: omfiGetNumMobs()
 *************************************************************************/
omfErr_t omfiGetNumMobs(
    omfHdl_t file,           /* IN - File Handle */
	omfMobKind_t mobKind,    /* IN - Kind of mob to count */
	omfNumSlots_t *numMobs)  /* OUT - Total number of mobs of kind mobKind */
{
    omfInt32 totalMobs, tmpNumMobs = 0;
	omfInt32 numSourceMobs, numCompMobs;
	omfObject_t head = NULL;

	*numMobs = 0;
	omfAssertValidFHdl(file);

	XPROTECT(file)
	  {
		CHECK(omfsGetHeadObject(file, &head));

		/* If asking for primary mobs, return length of primary mob index */
		if (mobKind == kPrimaryMob)
		  {
			if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
			  {
				RAISE(OM_ERR_NOT_IN_15);
			  }
			*numMobs = omfsLengthObjRefArray(file, head, OMHEADPrimaryMobs);
			return(OM_ERR_NONE);
		  }

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			numSourceMobs = omfsLengthObjIndex(file, head, OMSourceMobs);
			numCompMobs = omfsLengthObjIndex(file, head, OMCompositionMobs);
			totalMobs = numSourceMobs + numCompMobs;
			if (mobKind == kAllMob)
			  {
				*numMobs = totalMobs;
				return(OM_ERR_NONE);
			  }

			/* Else, iterate over source and comp mob indexes and count
			 * desired mobs.
			 */
			if ((mobKind == kMasterMob) || (mobKind == kCompMob))
			  {
				CHECK(CountMobInIndex(file, mobKind, OMCompositionMobs,
									  numCompMobs, &tmpNumMobs));
			  } /* If Master mob or Composition mob */
			else if ((mobKind == kFileMob) || (mobKind == kTapeMob)
					 || (mobKind == kFilmMob))
			  {
				CHECK(CountMobInIndex(file, mobKind, OMSourceMobs,
									  numSourceMobs, &tmpNumMobs));
			  } /* if source mob */
		  } /* if kOmfRev1x or kOmfRevIMA */

		else /* if kOmfRev2x */
		  {
			totalMobs = omfsLengthObjRefArray(file, head, OMHEADMobs);
			/* If asking for all mobs in the file,return length of mob index */
			if (mobKind == kAllMob)
			  {
				*numMobs = totalMobs;
				return(OM_ERR_NONE);
			  }

			/* Else, iterate over mob index and count desired mobs */
			CHECK(CountMobInIndex(file, mobKind, OMHEADMobs, totalMobs,
								  &tmpNumMobs));
		  } /* if kOmfRev2x */
	  } /* XPROTECT */

	XEXCEPT
	  {
		*numMobs = 0;
		return(XCODE());
	  }
	XEND;

	*numMobs = tmpNumMobs;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobGetNumTracks()
 *************************************************************************/
omfErr_t omfiMobGetNumTracks(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob object */
	omfNumTracks_t *numTracks) /* OUT - Number of tracks in the mob */
{
    omfInt32 tmpNumTracks = 0, numSlots, loop;
    omfObject_t slot = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	*numTracks = 0;
	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* For 1.x, simply return # of elements in mob's Tracks property */
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			*numTracks = omfsLengthObjRefArray(file, mob, OMTRKGTracks);
			return(OM_ERR_NONE);
		  }

		/* kOmfRev2x */
		CHECK(omfiMobGetNumSlots(file, mob, &numSlots));
	
		for (loop = 1; loop <= numSlots; loop++)
		  {
			CHECK(omfsGetNthObjRefArray(file, mob, OMMOBJSlots, &slot, loop));
			if (omfiMobSlotIsTrack(file, slot, &omfError))
			  {
				if (omfError == OM_ERR_NONE)
				  tmpNumTracks++;
				else
				  {
					RAISE(omfError);
				  }
			  }

			/* Release Bento reference, so the useCount is decremented */
			CMReleaseObject((CMObject)slot);
	       } /* for */
	  } /* XPROTECT */

	XEXCEPT
	  {
		*numTracks = 0;
		return(XCODE());
	  }
	XEND;

	*numTracks = tmpNumTracks;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobGetNumSlots()
 *************************************************************************/
omfErr_t omfiMobGetNumSlots(
	omfHdl_t file,       /* IN - File Handle */
	omfMobObj_t mob,     /* IN - Mob object */
	omfNumSlots_t *numSlots) /* OUT - Number of slots in the mob */
{
    omfNumSlots_t tmpNumSlots;
	omfProperty_t prop;

	*numSlots = 0;
	omfAssertValidFHdl(file);
	omfAssert((mob != NULL), file, OM_ERR_NULLOBJECT);

	if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
	  prop = OMTRKGTracks;
	else /* xOmfRef2x */
	  prop = OMMOBJSlots;

	tmpNumSlots = omfsLengthObjRefArray(file, mob, prop);

	*numSlots = tmpNumSlots;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobGetDefaultFade()
 * NOTE: If there is no default fade, this function returns with no error,
 *		but the VALID field of the structure is false.  This allows you to
 *		pass this struct to omfiSourceClipGetFade() in all cases.
 *************************************************************************/
omfErr_t omfiMobGetDefaultFade(
	omfHdl_t			file,       /* IN - File Handle */
	omfMobObj_t			mob,   		/* IN - Mob object */
	omfDefaultFade_t	*result)	/* OUT - a default fade struct */
{
	omfErr_t		status;
	
	omfAssert((mob != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((result != NULL), file, OM_ERR_NULL_PARAM);
	omfAssert((file->setrev != kOmfRev1x) && (file->setrev != kOmfRevIMA), file, OM_ERR_NOT_IN_15);
	omfAssert(omfiIsACompositionMob(file, mob, &status), file, OM_ERR_NOT_COMPOSITION);
	
	XPROTECT(file)
	{
		result->valid = FALSE;		/* Assume not valid, prove that it is */
		status = omfsReadLength(file, mob, OMCMOBDefFadeLength, &result->fadeLength);
		if(status == OM_ERR_NONE)
		{
			status = omfsReadFadeType(file, mob, OMCMOBDefFadeType, &result->fadeType);
		}
		if(status == OM_ERR_NONE)
		{
			status = omfsReadRational(file, mob, OMCMOBDefFadeEditUnit, &result->fadeEditUnit);
		}
		if(status == OM_ERR_NONE)
		{
			result->valid = TRUE;
		}
		else
		{
			if(status != OM_ERR_PROP_NOT_PRESENT)	/* Not an error if no default present */
				RAISE(status);
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;
	
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiNestedScopeGetNumSlots()
 *************************************************************************/
omfErr_t omfiNestedScopeGetNumSlots(
    omfHdl_t file,       /* IN - File Handle */
	omfSegObj_t scope,   /* IN - Scope object */
	omfNumSlots_t *numSlots) /* OUT - Number of slots in the scope */
{
    omfNumSlots_t tmpNumSlots;

	*numSlots = 0;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((scope != NULL), file, OM_ERR_NULLOBJECT);

	tmpNumSlots = omfsLengthObjRefArray(file, scope, OMNESTSlots);

	*numSlots = tmpNumSlots;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSelectorGetNumAltSlots()
 *************************************************************************/
omfErr_t omfiSelectorGetNumAltSlots(
    omfHdl_t file,         /* IN - File Handle */
	omfSegObj_t selector,  /* IN - Selector object */
	omfNumSlots_t *numSlots) /* OUT - Number of slots in the selector */
{
    omfNumSlots_t tmpNumSlots;

	*numSlots = 0;
	omfAssertValidFHdl(file);
	omfAssert((selector != NULL), file, OM_ERR_NULLOBJECT);

	/* For 1.x, subtract the selected slot from the total slots */
	if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
	  {
		tmpNumSlots = omfsLengthObjRefArray(file, selector, OMTRKGTracks);
		tmpNumSlots--;
	  }
	else
	  tmpNumSlots = omfsLengthObjRefArray(file, selector, 
										  OMSLCTAlternates);
	*numSlots = tmpNumSlots;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSequenceGetNumCpnts()
 *************************************************************************/
omfErr_t omfiSequenceGetNumCpnts(
    omfHdl_t file,        /* IN - File Handle */
	omfSegObj_t sequence, /* IN - Sequence object */
	omfInt32 *numCpnts)   /* OUT - Number of components in sequence */
{
    omfInt32 tmpNumCpnts;
	omfProperty_t prop;

	*numCpnts = 0;
	omfAssertValidFHdl(file);
	omfAssert((sequence != NULL), file, OM_ERR_NULLOBJECT);

	if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
	  prop = OMSEQUSequence;
	else /* kOmfRev2x */
	  prop = OMSEQUComponents;

	tmpNumCpnts = omfsLengthObjRefArray(file, sequence, prop);

	*numCpnts = tmpNumCpnts;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectNumSlots()
 *************************************************************************/
omfErr_t omfiEffectGetNumSlots(
    omfHdl_t file,       /* IN - File Handle */
	omfSegObj_t effect,  /* IN - Effect object */
	omfNumSlots_t *numSlots) /* OUT - Number of slots in the effect */
{
    omfNumSlots_t tmpNumSlots;

	*numSlots = 0;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);

	tmpNumSlots = omfsLengthObjRefArray(file, effect, OMEFFEEffectSlots);

	*numSlots = tmpNumSlots;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectGetNumWorkingRenders()
 *************************************************************************/
omfErr_t omfiEffectGetNumWorkingRenders(
    omfHdl_t file,       /* IN - File Handle */
	omfSegObj_t effect,  /* IN - Effect object */
	omfInt32 *numRenderings) /* OUT - Number of working renderings */
{
    omfInt32 tmpNumRenderings = 0;
	omfMobObj_t mob = NULL;
	omfSegObj_t sourceClip = NULL;
	omfUID_t nullID = {0,0,0};
	omfSourceRef_t sourceRef;

	*numRenderings = 0;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* Get the Source clip for the rendering */
		CHECK(omfiEffectGetWorkingRender(file, effect, &sourceClip));
		CHECK(SourceClipGetRef(file, sourceClip, &sourceRef));

		/* Find the mob that the render source clip refers to */
		if (!equalUIDs(nullID, sourceRef.sourceID))
		  {
			CHECK(omfiSourceClipResolveRef(file, sourceClip, &mob));
			
			CHECK(omfmGetNumRepresentations(file, mob, sourceRef.sourceTrackID,
											&tmpNumRenderings));
		  }
		/* If there is no sourceRef, there are no renderings so return 0 */
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	*numRenderings = tmpNumRenderings;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectGetNumFinalRenders()
 *************************************************************************/
omfErr_t omfiEffectGetNumFinalRenders(
    omfHdl_t file,       /* IN - File Handle */
	omfSegObj_t effect,  /* IN - Effect object */
	omfInt32 *numRenderings) /* OUT - Number of final renderings */
{
    omfInt32 tmpNumRenderings = 0;
	omfMobObj_t mob = NULL;
	omfSegObj_t sourceClip = NULL;
	omfUID_t nullID = {0,0,0};
	omfSourceRef_t sourceRef;

	*numRenderings = 0;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((effect != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		/* Get the Source clip for the rendering */
		CHECK(omfiEffectGetFinalRender(file, effect, &sourceClip));
		CHECK(SourceClipGetRef(file, sourceClip, &sourceRef));

		/* Find the mob that the render source clip refers to */
		if (!equalUIDs(nullID, sourceRef.sourceID))
		  {
			CHECK(omfiSourceClipResolveRef(file, sourceClip, &mob));

			CHECK(omfmGetNumRepresentations(file, mob, sourceRef.sourceTrackID,
											&tmpNumRenderings));
		  }
		/* If there is no sourceRef, there are no renderings so return 0 */
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	*numRenderings = tmpNumRenderings;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiVaryValueGetNumPoints()
 *************************************************************************/
omfErr_t omfiVaryValueGetNumPoints(
    omfHdl_t file,       /* IN - File Handle */
	omfSegObj_t value,   /* IN - Varying value object */
	omfInt32 *numPoints) /* OUT - Number of control points */
{
    omfInt32 tmpNumPoints;

	*numPoints = 0;
	omfAssertValidFHdl(file);
	omfAssertNot1x(file);
	omfAssert((value != NULL), file, OM_ERR_NULLOBJECT);

	tmpNumPoints = omfsLengthObjRefArray(file, value, OMVVALPointList);

	*numPoints = tmpNumPoints;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiIteratorAlloc()
 *
 * 		This function allocates an iterator handle to be subsequently
 *      used by the iterator functions.  The "type" of the iterator
 *      isn't initialized until the first iterator function is called.
 *      The iterator should be disposed with omfiIteratorDispose().
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiIteratorAlloc(
	omfHdl_t file,           /* IN - File Handle */
	omfIterHdl_t *iterHdl)   /* OUT - Newly allocated iterator handle */
{
    omfIterHdl_t tmpHdl = NULL;

	*iterHdl = NULL;
	omfAssertValidFHdl(file);
	
	XPROTECT(file)
	  {
		tmpHdl = (omfIterHdl_t)omOptMalloc(file, sizeof(struct omfiIterate));
		if (tmpHdl == NULL)
		  {
			RAISE(OM_ERR_NOMEMORY);
		  }
		tmpHdl->tableIter = NULL;
		CHECK(omfiIteratorClear(file, tmpHdl));
	  }
	XEXCEPT
	  {
		/* Assume don't have to free if malloc failed */
		if ((XCODE() != OM_ERR_NOMEMORY) && tmpHdl)
		  omOptFree(file, tmpHdl);
		return(XCODE());
	  }
	XEND;

	*iterHdl = tmpHdl;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiIteratorDispose()
 *
 * 		This function disposes an iterator handle that was allocated
 *      with omfiIteratorAlloc().
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiIteratorDispose(
	omfHdl_t file,        /* IN - File Handle */
	omfIterHdl_t iterHdl) /* IN/OUT - Iterator handle */
{
    omfIterHdl_t nextIter = NULL, saveIter;

	omfAssertValidFHdl(file);

	/* Dispose the chain of iterators attached to iterHdl */
	if (iterHdl)
	  nextIter = iterHdl->nextIterator;
	if (nextIter)
	  saveIter = nextIter->nextIterator;

	while (nextIter != NULL)
	  {
		omOptFree(file, nextIter);
		nextIter = saveIter;
		if (nextIter)
		  saveIter = nextIter->nextIterator;
	  }
	
	/* Free iterHdl itself */
	if (iterHdl)
	  {
		if (iterHdl->tableIter)
		  {
			omOptFree(file, iterHdl->tableIter);
			iterHdl->tableIter = NULL;
		  }
		omOptFree(file, iterHdl);
	  }
	
	iterHdl = NULL;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiIteratorClear()
 *
 * 		This function clears or resets an iterator that was allocated
 *      with omfiIteratorAlloc().  It reinitializes all fields in the
 *      iterator so that it can be reused.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiIteratorClear(
	omfHdl_t file,         /* IN - File Handle */
	omfIterHdl_t iterHdl)  /* IN/OUT - Iterator Handle */
{
	omfAssertValidFHdl(file);

	iterHdl->cookie = ITER_COOKIE;
	iterHdl->iterType = kIterNull;
	iterHdl->currentIndex = 0;
	iterHdl->file = file;
	iterHdl->maxIter = 0;
	iterHdl->maxMobs = 0;
	iterHdl->head = NULL;
	iterHdl->mdes = NULL;
	iterHdl->currentObj = NULL;
	iterHdl->currentProp = NULL;
	iterHdl->nextIterator = NULL;
	iterHdl->masterMob = NULL;
	iterHdl->masterTrack = 0;
	iterHdl->prevTransition = FALSE;
	omfsCvtInt32toInt64(0, &iterHdl->prevLength);
	omfsCvtInt32toInt64(0, &iterHdl->prevOverlap);
	omfsCvtInt32toInt64(0, &iterHdl->position);
	if (iterHdl->tableIter)
	  {
		omOptFree(file, iterHdl->tableIter);
		iterHdl->tableIter = NULL;
	  }

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiGetNextXXX()
 *
 *      These functions are iterators for various objects in an OMF file.  
 *      This comment block describes functionality that is common between
 *      all iterator functions.  See the individual functions for specific
 *      functionality or constraints.
 *
 *      The iterator handle is initialized to be a certain type of iterator 
 *      the first time a GetNext call is made.  Subsequent calls will assume 
 *      that the same type of iterator is being passed in.  Search criteria 
 *      can be used to specify various constraints and filters while
 *      searching.  Currently, search criteria is limited to a single
 *      criterium (i.e., you can't && or || multiple criteria).
 *      The iterator functions make an assumption that the file is not
 *      being modified while being iterated over.  If the file is being 
 *      modified, the behavior of the iterator functions in undefined.
 *
 *      When no more mobs exist, this function returns the error code
 *      OM_ERR_NO_MORE_OBJECTS.
 *
 *      The iterator handle should be allocated with omfiIteratorAlloc()
 *      before calling this function.  It should be freed with 
 *      omfiIteratorDispose() when finished.
 * 
 *      This function assumes that the search criteria has not changed
 *      between calls to the iterator.  The search criteria is initialized
 *      the first time that the iterator is called.
 *
 *      Most iterators will support both 1.x and 2.x files.  Limitations
 *      are described for each specific type of iterator below.
 *
 * Argument Notes:
 *      If no search criteria is desired, NULL should be passed into
 *      the searchCrit argument.  The default behavior is to search for
 *      all mobs.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
/*************************************************************************
 * Function: omfiGetNextMob()
 *
 *      This function is an iterator for mobs in a file.  Search criteria 
 *      can be used to specify the following: a mob kind to search for, 
 *      a mob ID to search for, or a mob name to search for. 
 *
 *      This iterator will support both 1.x and 2.x files.
 *
 * Argument Notes:
 *      If no search criteria is desired, NULL should be passed into
 *      the searchCrit argument.  The default behavior is to search for
 *      all mobs.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiGetNextMob(
						omfIterHdl_t iterHdl,        /* IN - Iterator handle */
						omfSearchCrit_t *searchCrit, /* IN - Search Criteria */
						omfMobObj_t *mob)            /* OUT - Found Mob */
{
    omfObject_t tmpMob = NULL;
	omfInt32 rememberCurrIndex;
	omfBool initialized = FALSE, found = FALSE, firstTime = FALSE;
	omfBool foundEntry;
	omfString name;
	omfUID_t mobID;
	mobTableEntry_t *mobEntry;
	omfProperty_t prop;
	omfErr_t omfError = OM_ERR_NONE;

	*mob = NULL;
	omfAssertIterHdl(iterHdl);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterMobIndex) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);

	XPROTECT(iterHdl->file)
	  {
		/* Save current state in case of error */
		rememberCurrIndex = iterHdl->currentIndex;

		/* INITIALIZE ITERATOR if first time through */
		if (iterHdl->currentIndex == 0)
		  {
			if (iterHdl->tableIter == NULL)
			  iterHdl->tableIter = 
				(omTableIterate_t *)omOptMalloc(iterHdl->file, 
												sizeof(omTableIterate_t));
			firstTime = TRUE;
			CHECK(omfsGetHeadObject(iterHdl->file, &iterHdl->head));
			CHECK(omfiGetNumMobs(iterHdl->file, kAllMob, &(iterHdl->maxMobs)));
			iterHdl->currentIndex = 1;
			iterHdl->iterType = kIterMobIndex;

			/* Verify correct search criteria for mob iterator */
			if (searchCrit)
			  {
				if ((*searchCrit).searchTag == kByMobID)
				  {
					iterHdl->searchCrit.tags.mobID.prefix = 
					  (*searchCrit).tags.mobID.prefix;
					iterHdl->searchCrit.tags.mobID.major = 
					  (*searchCrit).tags.mobID.major;
					iterHdl->searchCrit.tags.mobID.minor = 
					  (*searchCrit).tags.mobID.minor; 
				  }
				else if ((*searchCrit).searchTag == kByMobKind)
				  iterHdl->searchCrit.tags.mobKind = 
					(omfMobKind_t)searchCrit->tags.mobKind;
				else if ((*searchCrit).searchTag == kByName)
				  {
					iterHdl->searchCrit.tags.name = (omfString)
					  omOptMalloc(iterHdl->file, strlen((omfString)searchCrit->tags.name));
					strcpy(iterHdl->searchCrit.tags.name, 
						   (omfString)searchCrit->tags.name);
				  }
				else if ((*searchCrit).searchTag != kNoSearch)
				  {
					RAISE(OM_ERR_INVALID_SEARCH_CRIT);
				  }
				iterHdl->searchCrit.searchTag = searchCrit->searchTag;
			  }
			else
			  iterHdl->searchCrit.searchTag = kNoSearch;
		  }

		initialized = TRUE;

		/* GET NEXT MOB using search criteria */
		if (firstTime)
		{
		  CHECK(omfsTableFirstEntry(iterHdl->file->mobs, 
											iterHdl->tableIter, &foundEntry));
		}
		else
		{
		  CHECK(omfsTableNextEntry(iterHdl->tableIter, &foundEntry));
		}
		
		while (!found && foundEntry)
		  {
			mobEntry = (mobTableEntry_t *)iterHdl->tableIter->valuePtr;
			tmpMob = (omfObject_t)mobEntry->mob;
			switch(iterHdl->searchCrit.searchTag)
			  {
			  case kNoSearch:
				found = TRUE;
				break;
				
			  case kByName:
				if ((iterHdl->file->setrev == kOmfRev1x) || 
					iterHdl->file->setrev == kOmfRevIMA)
				  prop = OMCPNTName;
				else
				  prop = OMMOBJName;
				name = (omfString)omOptMalloc(iterHdl->file, OMMOBNAME_SIZE);
				omfError = omfsReadString(iterHdl->file, tmpMob, prop,
									 (char *)name, OMMOBNAME_SIZE);
				if (name)
				  {
					if (!strcmp(name, iterHdl->searchCrit.tags.name))
					  found = TRUE;
				  }
				if (name)
				  omOptFree(iterHdl->file, name);
				break;

			  case kByMobID:
				CHECK(omfsReadUID(iterHdl->file, tmpMob, OMMOBJMobID, &mobID));
				if (equalUIDs(mobID, iterHdl->searchCrit.tags.mobID))
					found = TRUE;
				break;

			  case kByMobKind:
				if ((iterHdl->searchCrit.tags.mobKind == kCompMob) && 
					omfiIsACompositionMob(iterHdl->file, tmpMob, &omfError))
				  found = TRUE;
				else if ((iterHdl->searchCrit.tags.mobKind == kMasterMob) && 
					omfiIsAMasterMob(iterHdl->file, tmpMob, &omfError))
				  found = TRUE;
				else if ((iterHdl->searchCrit.tags.mobKind == kFileMob) && 
				    omfiIsAFileMob(iterHdl->file, tmpMob, &omfError))
				  found = TRUE;
				else if ((iterHdl->searchCrit.tags.mobKind == kTapeMob) && 
					omfiIsATapeMob(iterHdl->file, tmpMob, &omfError))
				  found = TRUE;
				else if ((iterHdl->searchCrit.tags.mobKind == kFilmMob) && 
					omfiIsAFilmMob(iterHdl->file, tmpMob, &omfError))
				  found = TRUE;
				else if ((iterHdl->searchCrit.tags.mobKind == kPrimaryMob) &&
						 omfiIsAPrimaryMob(iterHdl->file, tmpMob, &omfError))
				  found = TRUE;
				else if (iterHdl->searchCrit.tags.mobKind == kAllMob)
				  found = TRUE;
				break;

			  default: /* Shouldn't reach this point */
				RAISE(OM_ERR_INVALID_SEARCH_CRIT);
				break;
			  } /* switch */

			if (omfError != OM_ERR_NONE)
			  {
				RAISE(omfError);
			  }
			else if (!found)
			  {
				CHECK(omfsTableNextEntry(iterHdl->tableIter, &foundEntry));
			  }
	      } /* while */

		if (found)
		  *mob = tmpMob;
		else if (!foundEntry)
		  {
			RAISE(OM_ERR_NO_MORE_OBJECTS);
		  }
		else
		  {
			RAISE(OM_ERR_INTERNAL_ITERATOR);
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (!initialized)
		  omfiIteratorClear(iterHdl->file, iterHdl);
		else
		  iterHdl->currentIndex = rememberCurrIndex;
		*mob = NULL;

		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
  }

/*************************************************************************
 * Private Function: GetDatakindFromObj()
 *
 * 		This private function returns the datakind object from the
 *      input obj.  If the is 1.x, it will retrieve the track kind and
 *      translate it into a "dummy" datakind object.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t GetDatakindFromObj(
    omfHdl_t file,       /* IN - File Handle */
	omfObject_t obj,     /* IN - Object to operate on */
    omfDDefObj_t *def)   /* OUT - Datakind definition object */
{
    omfObject_t tmpSeg = NULL, tmpCpnt = NULL;
	omfTrackType_t trackType;
    omfErr_t omfError = OM_ERR_NONE;

    XPROTECT(file)
	  {
		*def = NULL;

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			if (omfiIsAComponent(file, obj, &omfError))
			  {
				CHECK(omfsReadTrackType(file, obj, OMCPNTTrackKind, 
										&trackType));
				*def = TrackTypeToMediaKind(file, trackType);
			  }
			else if (omfsIsTypeOf(file, obj, "TRAK", &omfError))
			  {
				CHECK(omfsReadObjRef(file, obj, OMTRAKTrackComponent,
									 &tmpCpnt));
				if (tmpCpnt)
				  {
					CHECK(omfsReadTrackType(file, tmpCpnt, 
											OMCPNTTrackKind, &trackType));
					*def = TrackTypeToMediaKind(file, trackType);
				  }
			  }
		  } 
		else /* kOmfRev2x */
		  {
			if (omfiIsASegment(file, obj, &omfError))
			  {
				CHECK(omfsReadObjRef(file, obj, OMCPNTDatakind, def));
			  }
			else if (omfsIsTypeOf(file, obj, "MSLT", &omfError))
			  {
				CHECK(omfiMobSlotGetInfo(file, obj, NULL, &tmpSeg));
				if (tmpSeg)
				  {
					CHECK(omfsReadObjRef(file, tmpSeg, OMCPNTDatakind, def));
				  }
			  }
			else if (omfsIsTypeOf(file, obj, "ESLT", &omfError))
			  {
				CHECK(omfiEffectSlotGetInfo(file, obj, NULL, &tmpSeg));
				if (tmpSeg)
				  {
					CHECK(omfsReadObjRef(file, tmpSeg, OMCPNTDatakind, def));
				  }
			  }
		  } /* kOmfRev2x */
	  }
	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Private Function: GetNextArrayElem()
 *
 * 		This private function encapsulates the internal searching
 *      for most of the intra-mob iterators.  It takes the containing
 *      object, the name of the parent's array property and the
 *      type of iterator as arguments, and returns the next object found.
 *      Refer to each iterator function for the specifics about it.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t GetNextArrayElem(
    omfIterHdl_t iterHdl,        /* IN - Iterator Handle */
    omfSearchCrit_t *searchCrit, /* IN - Search Criteria */
	omfObject_t parent,          /* IN - Containing object */
    omfProperty_t prop,          /* IN - Parent Property containing children */
    omfIterType_t iterType,      /* IN - Type of iterator */
	omfObject_t *nextObj)        /* OUT - Next object found */
{
    omfObject_t tmpNext = NULL, tmpTrack = NULL;
	omfBool initialized = FALSE, found = FALSE;
	omfInt32 rememberCurrIndex;
	omfInt16 tmpLabel, selectedLabel;
	omfDDefObj_t def = NULL;
	omfClassID_t tmpClassID;
	omfUniqueName_t name;
	omfErr_t omfError = OM_ERR_NONE;
	omfProperty_t	idProp;

	*nextObj = NULL;
	omfAssertIterHdl(iterHdl);

	XPROTECT(iterHdl->file)
	  {
		if ((iterHdl->file->setrev == kOmfRev1x) || (iterHdl->file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		rememberCurrIndex = iterHdl->currentIndex;

		/* INITIALIZE ITERATOR if first time through */
		if (iterHdl->currentIndex == 0)
		  {
			iterHdl->maxIter = omfsLengthObjRefArray(iterHdl->file, parent, 
													 prop);
			iterHdl->currentIndex = 1;
			iterHdl->iterType = iterType;

			/* Verify correct search criteria */
			if (searchCrit)
			  {
				if ((*searchCrit).searchTag == kByDatakind)
				  {
					strcpy(iterHdl->searchCrit.tags.datakind, 
						   (*searchCrit).tags.datakind);
				  }
				else if (((*searchCrit).searchTag == kByClass) && 
						 (iterHdl->iterType == kIterSequence))
				  {
					strncpy(iterHdl->searchCrit.tags.objClass, 
							(*searchCrit).tags.objClass, sizeof(omfClassID_t));
				  }
				else if ((*searchCrit).searchTag != kNoSearch)
				  {
					RAISE(OM_ERR_INVALID_SEARCH_CRIT);
				  }
				iterHdl->searchCrit.searchTag = searchCrit->searchTag;
			  }
			else
			  iterHdl->searchCrit.searchTag = kNoSearch;
		  } /* end of initialization */
		initialized = TRUE;

		/* GET NEXT ARRAY ELEMENT */
		while ((iterHdl->currentIndex <= iterHdl->maxIter) && !found)
		  {
			CHECK(omfsGetNthObjRefArray(iterHdl->file, parent, prop,
					     &tmpNext, iterHdl->currentIndex));

			if ((iterHdl->file->setrev == kOmfRev1x) || 
				iterHdl->file->setrev == kOmfRevIMA)
			  {
				/* For 1.x selectors, we need to get the track component
				 * out of the track.
				 * If this is GetNextAlternate on a selector, skip past
				 * the selected track (which is not separated from the 
				 * list of tracks in 1.x, as it is in 2.x)
				 */
				if (omfiIsASelector(iterHdl->file, parent, &omfError))
				  {
					tmpTrack = tmpNext;
					/* For 1.x, selector track nums are sequential */
					CHECK(omfsReadInt16(iterHdl->file, tmpTrack, 
										OMTRAKLabelNumber, &tmpLabel));
					CHECK(omfsReadInt16(iterHdl->file, parent, 
										OMSLCTSelectedTrack, &selectedLabel));
					/* If this is the selectedTrack, continue and get next */
					if (tmpLabel == selectedLabel)
					  {
						iterHdl->currentIndex++;
						continue;
					  }
					/* Else, get track component and continue to see if passes
					 * search criteria.
					 */
					CHECK(omfsReadObjRef(iterHdl->file, tmpTrack, 
										 OMTRAKTrackComponent, &tmpNext));
				  }
			  } /* if kOmfRev1x or kOmfRevIMA */

			switch (iterHdl->searchCrit.searchTag)
			  {
			  case kNoSearch:
				found = TRUE;
				break;

			  case kByDatakind:
				CHECK(GetDatakindFromObj(iterHdl->file, tmpNext, &def));
				CHECK(omfiDatakindGetName(iterHdl->file, def, 
										  OMUNIQUENAME_SIZE, name));

				if (!strcmp(name, iterHdl->searchCrit.tags.datakind))
				  found = TRUE;
				break;

			  case kByClass:
				CHECK(omfsReadClassID(iterHdl->file, tmpNext, idProp, tmpClassID));
				if (streq(tmpClassID, iterHdl->searchCrit.tags.objClass))
				  found = TRUE;
				break;
					  

			  default: /* Shouldn't reach this point */
				RAISE(OM_ERR_INVALID_SEARCH_CRIT);
				break;
			  } /* switch */

			iterHdl->currentIndex++;
		  } /* while */

		if (found)
		  *nextObj = tmpNext;
		else if (iterHdl->currentIndex > iterHdl->maxMobs)
		  {
			RAISE(OM_ERR_NO_MORE_OBJECTS);
		  }
		else
		  {
			RAISE(OM_ERR_INTERNAL_ITERATOR);
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (!initialized)
		  omfiIteratorClear(iterHdl->file, iterHdl);
		else
		  iterHdl->currentIndex = rememberCurrIndex;

		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobGetNextSlot()/omfiMobGetNextTrack()
 *
 *      This function is an iterator for slots/tracks in a mob. 
 *      Search criteria can be used to specify the following: the 
 *      datakind of the object being searched for.
 *
 *      This iterator will support both 1.x and 2.x files.
 *      With 2.x, GetNextSlot will return the next mob slot that matches
 *      the search criteria.  GetNextTrack will return the next "track"
 *      that matches the search criteria, where a "track" is defined to
 *      to be a mob slot that has a track descriptor.  For 1.x, both
 *      iterator functions will return the same set of objects, that is,
 *      all of the TRAK objects that match the search criteria.
 *
 * Argument Notes:
 *      If no search criteria is desired, NULL should be passed into
 *      the searchCrit argument.  The default behavior is to search for
 *      all mobs.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobGetNextSlot(
	omfIterHdl_t iterHdl,         /* IN - Iterator Handle */
	omfMobObj_t mob,              /* IN - Mob to search */
	omfSearchCrit_t *searchCrit,  /* IN - Search Criteria */
	omfMSlotObj_t *slot)          /* OUT - Found Track/Slot */
{
	omfSegObj_t tmpSlot = NULL;
	omfProperty_t prop;
	omfErr_t omfError = OM_ERR_NONE;

	*slot = NULL;
	omfAssertIterHdl(iterHdl);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterMob) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);
	omfAssert((mob != NULL), iterHdl->file, OM_ERR_NULLOBJECT);
	 
	XPROTECT(iterHdl->file)
	  {
		if ((iterHdl->file->setrev == kOmfRev1x) || 
			iterHdl->file->setrev == kOmfRevIMA)
		  prop = OMTRKGTracks;
		else /* kOmfRev2x */
		  prop = OMMOBJSlots;

		CHECK(GetNextArrayElem(iterHdl, searchCrit, mob, prop, kIterMob, 
							   &tmpSlot));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	*slot = tmpSlot;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobGetNextTrack()
 *************************************************************************/
omfErr_t omfiMobGetNextTrack(
	omfIterHdl_t iterHdl,         /* IN - Iterator Handle */
	omfMobObj_t mob,              /* IN - Mob to search */
	omfSearchCrit_t *searchCrit,  /* IN - Search Criteria */
	omfMSlotObj_t *track)         /* OUT - Found Track/Slot */
{
    omfMSlotObj_t mSlot = NULL;
    omfBool found = FALSE;
	omfErr_t omfError = OM_ERR_NONE;

	*track = NULL;
	omfAssertIterHdl(iterHdl);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterMob) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);
	omfAssert((mob != NULL), iterHdl->file, OM_ERR_NULLOBJECT);

	XPROTECT(iterHdl->file)
	  {
		while (!found)
		  {
			CHECK(omfiMobGetNextSlot(iterHdl, mob, searchCrit, &mSlot));
			if (omfiMobSlotIsTrack(iterHdl->file, mSlot, &omfError))
			  {
				if (omfError == OM_ERR_NONE)
				  found = TRUE;
				else
				  {
					RAISE(omfError);
				  }
			  }
		  }
		if (found)
		  *track = mSlot;
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectGetNextSlot()
 *
 *      This function is an iterator for slots in an effect.  
 *      Search criteria can be used to specify the following: the 
 *      datakind of the object being searched for.
 *
 *      This iterator will only support 2.x files.
 *
 * Argument Notes:
 *      If no search criteria is desired, NULL should be passed into
 *      the searchCrit argument.  The default behavior is to search for
 *      all mobs.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiEffectGetNextSlot(
    omfIterHdl_t iterHdl,        /* IN - Iterator Handle */
	omfCpntObj_t effect,         /* IN - Effect to search */
	omfSearchCrit_t *searchCrit, /* IN - Search Criteria */
	omfESlotObj_t *effSlot)      /* OUT - Found effect slot */
{
	omfSegObj_t tmpSlot = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	*effSlot = NULL;
	omfAssertIterHdl(iterHdl);
	omfAssertNot1x(iterHdl->file);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterEffect) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);
	omfAssert((effect != NULL), iterHdl->file, OM_ERR_NULLOBJECT);


	XPROTECT(iterHdl->file)
	  {
		if (!omfiIsAnEffect(iterHdl->file, effect, &omfError))
		  {
			RAISE(OM_ERR_INVALID_OBJ);
		  }
		CHECK(GetNextArrayElem(iterHdl, searchCrit, effect, 
				    OMEFFEEffectSlots, kIterEffect, &tmpSlot));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	*effSlot = tmpSlot;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSelectorGetNextSlot()
 *
 *      This function is an iterator for alternate slots in a selector.  
 *      Search criteria can be used to specify the following: the 
 *      datakind of the object being searched for.
 *
 *      This iterator will support both 1.x and 2.x files.
 *
 * Argument Notes:
 *      If no search criteria is desired, NULL should be passed into
 *      the searchCrit argument.  The default behavior is to search for
 *      all mobs.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiSelectorGetNextAltSlot(
    omfIterHdl_t iterHdl,         /* IN - Iterator Handle */
	omfCpntObj_t selector,        /* IN - Selector object to search */
	omfSearchCrit_t *searchCrit,  /* IN - Search Criteria */
	omfSegObj_t *selSlot)         /* OUT - Found Selector slot */
{
    omfSegObj_t tmpSlot = NULL;
	omfProperty_t prop;
	omfErr_t omfError = OM_ERR_NONE;

	*selSlot = NULL;
	omfAssertIterHdl(iterHdl);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterSelector) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);
	omfAssert((selector != NULL), iterHdl->file, OM_ERR_NULLOBJECT);

	XPROTECT(iterHdl->file)
	  {
		if (!omfiIsASelector(iterHdl->file, selector, &omfError))
		  {
			RAISE(OM_ERR_INVALID_OBJ);
		  }
		if ((iterHdl->file->setrev == kOmfRev1x) || 
			iterHdl->file->setrev == kOmfRevIMA)
		  prop = OMTRKGTracks;
		else /* kOmfRev2x */
		  prop = OMSLCTAlternates;

		CHECK(GetNextArrayElem(iterHdl, searchCrit, selector, prop,
							   kIterSelector, &tmpSlot));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	*selSlot = tmpSlot;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiNestedScopeGetNextSlot()
 *
 *      This function is an iterator for slots in an scope.  
 *      Search criteria can be used to specify the following: the 
 *      datakind of the object being searched for.
 *
 *      This iterator will only support 2.x files.
 *
 * Argument Notes:
 *      If no search criteria is desired, NULL should be passed into
 *      the searchCrit argument.  The default behavior is to search for
 *      all mobs.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiNestedScopeGetNextSlot(
    omfIterHdl_t iterHdl,        /* IN - Iterator Handle */
	omfCpntObj_t scope,          /* IN - Scope object to search */
	omfSearchCrit_t *searchCrit, /* IN - Search Criteria */
	omfSegObj_t *scopeSlot)      /* OUT - Found Scope Slot */
{
    omfSegObj_t tmpSlot = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	*scopeSlot = NULL;
	omfAssertIterHdl(iterHdl);
	omfAssertNot1x(iterHdl->file);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterScope) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);
	omfAssert((scope != NULL), iterHdl->file, OM_ERR_NULLOBJECT);

	XPROTECT(iterHdl->file)
	  {
		if (!omfiIsANestedScope(iterHdl->file, scope, &omfError))
		  {
			RAISE(OM_ERR_INVALID_OBJ);
		  }
		CHECK(GetNextArrayElem(iterHdl, searchCrit, scope, 
				    OMNESTSlots, kIterScope, &tmpSlot));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	*scopeSlot = tmpSlot;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiSequenceGetNextCpnt()
 *
 *      This function is an iterator for components in a sequence.  
 *      Search criteria can be used to specify the following: the 
 *      datakind of the object being searched for or the classID of 
 *      the object.
 *
 *      This object will also return the position that the component is
 *      found in the sequence.
 *
 *      This iterator will support both 1.x and 2.x files.
 *
 * Argument Notes:
 *      If no search criteria is desired, NULL should be passed into
 *      the searchCrit argument.  The default behavior is to search for
 *      all mobs.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiSequenceGetNextCpnt(
 	omfIterHdl_t iterHdl,         /* IN - Iterator Handle */
	omfCpntObj_t sequence,        /* IN - Sequence to Search */
	omfSearchCrit_t *searchCrit,  /* IN - Search Criteria */
	omfPosition_t *offset,        /* OUT - Offset into sequence for cpnt */
	omfCpntObj_t *component)      /* OUT - Found component */
{
	omfCpntObj_t tmpCpnt = NULL;
	omfLength_t len;
	omfProperty_t prop;
	omfLength_t tranLen, zeroLen;
	omfErr_t omfError = OM_ERR_NONE;

	if (offset)
	  omfsCvtInt32toInt64(0, offset);
	omfAssertIterHdl(iterHdl);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterSequence) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);
	omfAssert((sequence != NULL), iterHdl->file, OM_ERR_NULLOBJECT);
	*component = NULL;
	omfsCvtInt32toLength(0, zeroLen);

	XPROTECT(iterHdl->file)
	  {
		if (!omfiIsASequence(iterHdl->file, sequence, &omfError))
		  {
			RAISE(OM_ERR_INVALID_OBJ);
		  }

		if ((iterHdl->file->setrev == kOmfRev1x) || 
			iterHdl->file->setrev == kOmfRevIMA)
		  prop = OMSEQUSequence;
		else
		  prop = OMSEQUComponents;
		
		CHECK(GetNextArrayElem(iterHdl, searchCrit, sequence, prop,
				    kIterSequence, &tmpCpnt));
		CHECK(omfiComponentGetLength(iterHdl->file, tmpCpnt, &len));

		/* For non-Transitions... */
		if (!omfiIsATransition(iterHdl->file, tmpCpnt, &omfError))
		  {
			/* If the previous component was a transition... */
			if (iterHdl->prevTransition)
			  {
				/* Calculate the new size of the segment, taking out the
				 * transition overlap.  
				 */
				omfsSubInt64fromInt64(iterHdl->prevOverlap, &len);
				iterHdl->prevLength = len;

				/* If the transition completely covers the segment, 
				 * pass back position at end of transition for the component's 
				 * position (and length 0)
				 */
				omfsAddInt64toInt64(iterHdl->prevOverlap,
									&iterHdl->position);

				iterHdl->prevTransition = FALSE;    /* Reset */
				omfsCvtInt32toInt64(0, &iterHdl->prevOverlap); /* Reset */
			  }
			else /* Normal case */
			  {
				omfsAddInt64toInt64(iterHdl->prevLength, &iterHdl->position);
				iterHdl->prevLength = len;
			  }
		  }
		else /* Transition */
		  {
			iterHdl->prevTransition = TRUE;
			CHECK(omfiTransitionGetInfo(iterHdl->file, tmpCpnt, 
										NULL, &tranLen, NULL, NULL));
			/* Position = PrevLen - tranLength */
			omfsAddInt64toInt64(iterHdl->prevLength, &iterHdl->position);
			omfsSubInt64fromInt64(tranLen, &iterHdl->position);

			/* Overlap into next CPNT = TRAN len */
			iterHdl->prevOverlap = len;
			iterHdl->prevLength = len;
		  }
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;

	if (offset)
	  *offset = iterHdl->position;
	*component = tmpCpnt;
	return(OM_ERR_NONE);
}


/*************************************************************************
 * Private Function: GetNextRender()
 *
 * 		This function is the guts of the omfiEffectGetNextWorkingRender and
 *      omfiEffectGetNextFinalRender functionality.  
 *
 *      This function will follow the source clip and return a pointer
 *      to the file mob that corresponds to the "next" rendering.  With
 *      2.x, multiple renderings are created by adding a media group to
 *      the master mob that the rendering source clip points to.  The
 *      search criteria can be used to select a slot in the media group.
 *
 *      This iterator will only support 2.x files.
 *
 * Argument Notes:
 *      If no search criteria is desired, NULL should be passed into
 *      the searchCrit argument.  The default behavior is to search for
 *      all mobs.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t GetNextRender(
    omfIterHdl_t iterHdl,         /* IN - Iterator Handle */
	omfCpntObj_t effect,          /* IN - Effect object to search */
	omfInt16 renderType,          /* IN - Working or Final rendering */
	omfSearchCrit_t *searchCrit,  /* IN - Search Criteria */
	omfMobObj_t *fileMob)         /* OUT - File Mob corresponding to the
								   *       rendered source clip.
								   */
{
    omfBool initialized = FALSE, found = FALSE;
	omfInt32 rememberCurrIndex;
	omfSegObj_t sourceClip = NULL;
	omfSourceRef_t sourceRef;
	omfObject_t tmpNext = NULL, tmpClip = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	*fileMob = NULL;
	omfAssertIterHdl(iterHdl);
	omfAssertNot1x(iterHdl->file);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterEffectRender) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);
	omfAssert((effect != NULL), iterHdl->file, OM_ERR_NULLOBJECT);

	XPROTECT(iterHdl->file)
	  {
		rememberCurrIndex = iterHdl->currentIndex;

		if (!omfiIsAnEffect(iterHdl->file, effect, &omfError))
		  {
			RAISE(OM_ERR_INVALID_OBJ);
		  }

		/* INITIALIZE ITERATOR if first time through */
		if (iterHdl->currentIndex == 0)
		  {
			if (renderType == WORKING_RENDER)
			  {
				CHECK(omfiEffectGetNumWorkingRenders(iterHdl->file, effect, 
													(&iterHdl->maxIter)));
			  }
			else if (renderType == FINAL_RENDER)
			  {
				CHECK(omfiEffectGetNumFinalRenders(iterHdl->file, effect, 
												  (&iterHdl->maxIter)));
			  }
			iterHdl->currentIndex = 1;
			iterHdl->iterType = kIterEffectRender;
			/* Get the master mob and track that contains the renderings */
			if (iterHdl->maxIter > 0)
			  {
				if (renderType == WORKING_RENDER)
				  {
					CHECK(omfiEffectGetWorkingRender(iterHdl->file, effect,
													 &sourceClip));
				  }
				else if (renderType == FINAL_RENDER)
				  {
					CHECK(omfiEffectGetFinalRender(iterHdl->file, effect, 
												   &sourceClip));
				  }
				CHECK(SourceClipGetRef(iterHdl->file, sourceClip, 
										   &sourceRef));
				CHECK(omfiSourceClipResolveRef(iterHdl->file, sourceClip, 
											   &(iterHdl->masterMob)));
			  }
			iterHdl->masterTrack = sourceRef.sourceTrackID;
			   
			/* Verify correct search criteria */
			if (searchCrit)
			  {
				if ((*searchCrit).searchTag == kByMediaCrit)
				  {
					iterHdl->searchMediaCrit.type = searchCrit->tags.mediaCrit;
				  }
				else if ((*searchCrit).searchTag != kNoSearch)
				  {
					RAISE(OM_ERR_INVALID_SEARCH_CRIT);
				  }
				iterHdl->searchCrit.searchTag = searchCrit->searchTag;
			  }
			else 
				iterHdl->searchCrit.searchTag = kNoSearch;
		  } /* end of initialization */
		initialized = TRUE;

		/* GET NEXT ELEMENT */
		while ((iterHdl->currentIndex <= iterHdl->maxIter) && !found)
		  {
			/* There will only be one thing that matches mediaCrit */
			if (iterHdl->searchCrit.searchTag == kByMediaCrit)
			  {
				CHECK(omfmGetCriteriaSourceClip(iterHdl->file, 
							 iterHdl->masterMob, iterHdl->masterTrack, 
							 &(iterHdl->searchMediaCrit), &tmpClip));
				CHECK(omfiSourceClipResolveRef(iterHdl->file, tmpClip, 
											   &tmpNext));
				/* No more matches, so set iterator to finish next time */
				iterHdl->currentIndex = iterHdl->maxMobs+1;
			  }
			else
			  {
				CHECK(omfmGetRepresentationSourceClip(iterHdl->file, 
							     iterHdl->masterMob,
								 iterHdl->masterTrack, iterHdl->currentIndex, 
								 &tmpClip));
				CHECK(omfiSourceClipResolveRef(iterHdl->file, tmpClip, 
											   &tmpNext));
			  }

			switch (iterHdl->searchCrit.searchTag)
			  {
			  case kByMediaCrit: /* Already accounted for this case above */
				found = TRUE;
				break;
			  case kNoSearch:
				found = TRUE;
				break;
			  default:
				RAISE(OM_ERR_INVALID_SEARCH_CRIT);
				break;
			  } /* switch */
			iterHdl->currentIndex++;
		  } /* while */

		if (found)
		  *fileMob = tmpNext;
		else if (iterHdl->currentIndex > iterHdl->maxMobs)
		  {
			RAISE(OM_ERR_NO_MORE_OBJECTS);
		  }
		else
		  {
			RAISE(OM_ERR_INTERNAL_ITERATOR);
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (!initialized)
		  omfiIteratorClear(iterHdl->file, iterHdl);
		else
		  iterHdl->currentIndex = rememberCurrIndex;

		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectGetNextWorkingRender()
 *************************************************************************/
omfErr_t omfiEffectGetNextWorkingRender(
    omfIterHdl_t iterHdl,
	omfCpntObj_t effect,
	omfSearchCrit_t *searchCrit,
	omfMobObj_t *fileMob)
{
  XPROTECT(iterHdl->file)
	{
	  CHECK(GetNextRender(iterHdl, effect, WORKING_RENDER, searchCrit, 
						  fileMob));
	}
  XEXCEPT
	{
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectGetNextFinalRender()
 *************************************************************************/
omfErr_t omfiEffectGetNextFinalRender(
    omfIterHdl_t iterHdl,
	omfCpntObj_t effect,
	omfSearchCrit_t *searchCrit,
	omfMobObj_t *fileMob)
{
  XPROTECT(iterHdl->file)
	{
	  CHECK(GetNextRender(iterHdl, effect, FINAL_RENDER, searchCrit, 
						  fileMob));
	}
  XEXCEPT
	{
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiVaryValueGetNextPoint()
 *
 *      This function is an iterator for control points in a varying
 *      value object.  
 *
 *      Search criteria can be used to specify the following: the 
 *      datakind of the object being searched for.
 *
 *      This iterator will only support 2.x files.
 *
 * Argument Notes:
 *      If no search criteria is desired, NULL should be passed into
 *      the searchCrit argument.  The default behavior is to search for
 *      all mobs.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiVaryValueGetNextPoint(
    omfIterHdl_t iterHdl,         /* IN - Iterator Handle */
	omfCpntObj_t varyValue,       /* IN - Varying value to search */
	omfSearchCrit_t *searchCrit,  /* IN - Search Criteria */
	omfCntlPtObj_t *control)      /* OUT - Found control point */
{
    omfSegObj_t tmpPoint = NULL;
	omfErr_t omfError = OM_ERR_NONE;

	*control = NULL;
	omfAssertIterHdl(iterHdl);
	omfAssertNot1x(iterHdl->file);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterVaryValue) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);
	omfAssert((varyValue != NULL), iterHdl->file, OM_ERR_NULLOBJECT);

	XPROTECT(iterHdl->file)
	  {
		if (!omfiIsAVaryValue(iterHdl->file, varyValue, &omfError))
		  {
			RAISE(OM_ERR_INVALID_OBJ);
		  }
		CHECK(GetNextArrayElem(iterHdl, searchCrit, varyValue,
				    OMVVALPointList, kIterVaryValue, &tmpPoint));
	  }

	XEXCEPT
	  {
		return(XCODE());
	  }
	XEND;
	
	*control = tmpPoint;
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobMatchAndExecute()
 *
 *      This function recurses depth first through the composition rooted 
 *      at the object given in rootObj, and executes the callbackFunc callback
 *      on all objects in the tree that match the search criteria.  The
 *      match is also user defined, by calling the matchFunc callback.
 *      Data can be passed in and out of the callback with the matchData
 *      and callbackData arguments.
 *
 *      This function will maintain the the depth level relative to the
 *      initial level passed in and pass it into the execute callback.  
 *      It will return the number of matches.
 *
 *      This functionality is most useful for cases where an entire tree
 *      is to be searched, and an operation is to be performed.  It provides
 *      the traversal algorithm.  Examples would be OMF file dumpers,
 *      functionality that make sure that all source clips point to mobs that
 *      are in the file, etc.
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobMatchAndExecute(
	omfHdl_t     file,          /* IN - File Handle */
	omfObject_t  rootObj,       /* IN - Root object */
	omfInt32 	 level,         /* IN - Depth Level in the Hierarchy */
	MatchFunc    *matchFunc,    /* IN - Match Callback Function */
	void         *matchData,    /* IN - Data for Match Callback */
	CallbackFunc *callbackFunc, /* IN - Execute Callback Function */
    void         *callbackData, /* IN - Data for Execute Callback */
	omfInt32     *matches)      /* OUT - Number of matches found */
{
    omfIterHdl_t propIter = NULL;
	omfProperty_t prop;
	omfType_t type;
	CMProperty cprop;
	CMType ctype;
	CMValue cvalue;
    omfInt32 tmpMatches = 0, subMatches;
	CMGlobalName propTypeName, propName;

	*matches = 0;
	omfAssertValidFHdl(file);
	omfAssert((matchFunc != NULL), file, OM_ERR_NULL_MATCHFUNC);
	omfAssert((callbackFunc != NULL), file, OM_ERR_NULL_CALLBACKFUNC);

	XPROTECT(file)
	  {

		/* First see if current object matches, and execute callback if match*/
		if (matchFunc)
		  {
			/* If there is a match and the callback function exists */
			if (matchFunc(file, rootObj, matchData) && callbackFunc)
			  {
				CHECK(callbackFunc(file, rootObj, level, callbackData));
				tmpMatches++;
			  }
		  }

		CHECK(omfiIteratorAlloc(file, &propIter));

		/* Iterate over properties. If ObjRef or ObjRefArray, recurse */
		CHECK(omfiGetNextProperty(propIter, rootObj, &prop, &type));

		while (prop)
		  {
			cprop = CvtPropertyToBento(file, prop);
			propName = CMGetGlobalName((CMObject)cprop);
			ctype = CvtTypeToBento(file, type, NULL);
			propTypeName = CMGetGlobalName((CMObject)ctype);

			if (CMCountValues((CMObject)rootObj, cprop, ctype))
			  {
				cvalue = CMGetNextValue((CMObject)rootObj, cprop, NULL);
				propTypeName = CMGetGlobalName((CMObject)ctype);

				/* If the property is an ObjRefArray, recurse for each item */
				if (!strcmp(propTypeName, "omfi:ObjRefArray"))
				  {
					omfInt32 arrayLen, loop;
					omfObject_t objRef;

					arrayLen = omfsLengthObjRefArray(file, rootObj, 
													 (omfProperty_t)prop);
					for (loop=1; loop<=arrayLen; loop++)
					  {
						CHECK(omfsGetNthObjRefArray(file, rootObj, 
													   prop, &objRef, loop));

						if (objRef)
						  {
							CHECK(omfiMobMatchAndExecute(file,
									objRef, 1+level, matchFunc, matchData, 
                                    callbackFunc, callbackData, &subMatches));
							tmpMatches += subMatches;
						  }
					  }
				  } /* If ObjRefArray */

				/* If the property is an ObjRef, recurse */
				else if (!strcmp(propTypeName, "omfi:ObjRef"))
				  {
					omfObject_t objRef;
					
					CHECK(omfsReadObjRef(file, rootObj, prop, &objRef));
					if (objRef)
					  {
						CHECK(omfiMobMatchAndExecute(file,
							objRef, 1+level, matchFunc, matchData, 
                            callbackFunc, callbackData, &subMatches));
						tmpMatches += subMatches;
					  }
				  } /* If ObjRef */

			  } /* if Count Values */
			CHECK(omfiGetNextProperty(propIter, rootObj, &prop, &type));
		  } /* while */

	  } /* XPROTECT */

	XEXCEPT
	  {
		if (XCODE() == OM_ERR_NO_MORE_OBJECTS)
		  {
			if (propIter)
			  omfiIteratorDispose(file, propIter);
			*matches = tmpMatches;
			return(OM_ERR_NONE);
		  }
		if (propIter)
		  omfiIteratorDispose(file, propIter);
		return(XCODE());
	  }
	XEND;

	*matches = tmpMatches;
	if (propIter)
	  omfiIteratorDispose(file, propIter);
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiMobGetNextComment()
 *
 *      Iterates through the specified Mob to retrieve the next user-defined
 *		comment.
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobGetNumComments(
	omfHdl_t     	file,
	omfMobObj_t		mob,
	omfInt32		*numComments)
{
	omfObject_t			attb;
	omfInt32			n, cnt;
	omfErr_t			omfError;
	char				nameBuf[32];
	omfAttributeKind_t	kind;
	
	omfAssert((numComments != NULL), file, OM_ERR_NULLOBJECT);
	*numComments = 0;

	XPROTECT(file)
	  {
		if (!omfiIsAMob(file, mob, &omfError))
		  {
			RAISE(OM_ERR_INVALID_OBJ);
		  }

		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
	  	{
			cnt = omfsNumAttributes(file, mob, OMCPNTAttributes);
			for(n = 1; n <= cnt; n++)
			{
				CHECK(omfsGetNthAttribute(file, mob, OMCPNTAttributes, &attb, n));
	 			CHECK(omfsReadAttrKind(file, attb, OMATTBKind, &kind));
				CHECK(omfsReadString(file, attb, OMATTBName, nameBuf, sizeof(nameBuf)));
				if((kind == kOMFObjectAttribute) &&
				   (strcmp(nameBuf, "_USER") == 0))
				{
	  				*numComments = omfsNumAttributes(file, attb, OMATTBObjAttribute);
	  				break;
	  			}
			}
	  	}
	  	else
	  	{
	  		*numComments = omfsNumAttributes(file, mob, OMMOBJUserAttributes);
	  	}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;

	return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: omfiMobGetNextComment()
 *
 *      Iterates through the specified Mob to retrieve the next user-defined
 *		comment.
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobGetNextComment(
	omfIterHdl_t	iterHdl,
	omfMobObj_t		mob,
	omfInt32		categoryBufferSize,
	char			*category,
	omfInt32		commentBufferSize,
	char			*comment)
{
	omfObject_t			attb;
	omfInt32			n, cnt;
	char				nameBuf[32];
	omfAttributeKind_t	kind;
	omfErr_t			omfError;
	
	omfAssertIterHdl(iterHdl);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterMobComment) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);
	omfAssert((category != NULL), iterHdl->file, OM_ERR_NULLOBJECT);
	omfAssert((comment != NULL), iterHdl->file, OM_ERR_NULLOBJECT);
	category[0] = '\0';
	comment[0] = '\0';

	XPROTECT(iterHdl->file)
	{
		if (!omfiIsAMob(iterHdl->file, mob, &omfError))
		{
			RAISE(OM_ERR_INVALID_OBJ);
		}

		if(iterHdl->iterType == kIterNull)
		{
			if ((iterHdl->file->setrev == kOmfRev1x) || 
				iterHdl->file->setrev == kOmfRevIMA)
		  	{
				cnt = omfsNumAttributes(iterHdl->file, mob, OMCPNTAttributes);
				for(n = 1; n <= cnt; n++)
				{
					CHECK(omfsGetNthAttribute(iterHdl->file, mob, OMCPNTAttributes, &attb, n));
		 			CHECK(omfsReadAttrKind(iterHdl->file, attb, OMATTBKind, &kind));
					CHECK(omfsReadString(iterHdl->file, attb, OMATTBName, nameBuf,
										sizeof(nameBuf)));
					if((kind == kOMFObjectAttribute) &&
					   (strcmp(nameBuf, "_USER") == 0))
					{
		  				iterHdl->head = attb;
		  				iterHdl->iterType = kIterMobComment;
						iterHdl->currentIndex = 1;
		  				iterHdl->maxIter = omfsNumAttributes(iterHdl->file, attb, OMATTBObjAttribute);
		  				break;
		  			}
				}
		  	}
		  	else
		  	{
		  		iterHdl->head = mob;
			  	iterHdl->iterType = kIterMobComment;
  				iterHdl->currentIndex = 1;
		  		iterHdl->maxIter = omfsNumAttributes(iterHdl->file, mob, OMMOBJUserAttributes);
		  	}
		}
		
		if(iterHdl->currentIndex > iterHdl->maxIter)
			RAISE(OM_ERR_NO_MORE_OBJECTS);
			
		if(iterHdl->iterType == kIterMobComment)
		{
			if ((iterHdl->file->setrev == kOmfRev1x) || (iterHdl->file->setrev == kOmfRevIMA))
			{
				CHECK(omfsGetNthAttribute(iterHdl->file, iterHdl->head, OMATTBObjAttribute,
											&attb, iterHdl->currentIndex));
			}
			else
			{
				CHECK(omfsGetNthAttribute(iterHdl->file, iterHdl->head, OMMOBJUserAttributes,
											&attb, iterHdl->currentIndex));
			}
			iterHdl->currentIndex++;
			CHECK(omfsReadAttrKind(iterHdl->file, attb, OMATTBKind, &kind));
			if(kind == kOMFStringAttribute)
			{
				CHECK(omfsReadString(iterHdl->file, attb, OMATTBName,
									category, categoryBufferSize));
				CHECK(omfsReadString(iterHdl->file, attb, OMATTBStringAttribute, 
									 comment, commentBufferSize));
			}
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND;

	return(OM_ERR_NONE);
}


/*************************************************************************
 * Function: omfiConvertEditRate()
 *
 *      Given a source edit rate and position, a destination editrate,
 *      and a rounding method (kRoundFloor or kRoundCeiling), this
 *      function calculates a destination position.
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiConvertEditRate(
	omfRational_t srcRate,        /* IN - Source Edit Rate */
	omfPosition_t srcPosition,    /* IN - Source Position */
	omfRational_t destRate,       /* IN - Destination Edit Rate */
	omfRounding_t howRound,	      /* IN - Rounding method (floor or ceiling) */
	omfPosition_t *destPosition)  /* OUT - Destination Position */
{
	omfInt64		intPos, destPos;
	omfInt32		remainder;
		
	XPROTECT(NULL)
	{
		omfsCvtInt32toInt64(0, destPosition);
		if ((howRound != kRoundCeiling) && (howRound != kRoundFloor) && (howRound != kRoundNormally))
		{
			RAISE(OM_ERR_INVALID_ROUNDING);
		}

		if(FloatFromRational(srcRate) != FloatFromRational(destRate))
		{
			CHECK(omfsMultInt32byInt64((srcRate.denominator * destRate.numerator), srcPosition, &intPos));
			CHECK(omfsDivideInt64byInt32(intPos, (srcRate.numerator * destRate.denominator), &destPos, &remainder));
		}
		else
		{
	  		/* JeffB: (1 / 29.97) * 29.97 often doesn't == 1
	  		 * (it seems to be .99999... on the 68K FPU)
	  		 * The debugger says it's 1.0, but if(foo >= 1.0) fails.
	  		 */
	  		destPos = srcPosition;
	  		remainder = 0;
		}

		/* Usually used for lower edit rate to higher edit rate conversion - 
		 * to return the first sample of the higher edit rate that contains a
		 * sample of the lower edit rate. (i.e., video -> audio)
		 */
		if (howRound == kRoundFloor)
		{
			*destPosition = destPos;
		}
		/* Usually used for higher edit rate to lower edit rate conversion -
		 * to return the sample of the lower edit rate that fully contains
		 * the sample of the higher edit rate. (i.e., audio -> video)
		 */
		else if (howRound == kRoundCeiling)
		{
			*destPosition = destPos;
			if(remainder != 0)
				omfsAddInt32toInt64(1, destPosition);
		}
      // Added by NPW..
		else if (howRound == kRoundNormally)
		{
			*destPosition = destPos;
			// round up if > 0.5
			if(remainder * 2 > srcRate.numerator * destRate.denominator)
				omfsAddInt32toInt64(1, destPosition);
		}
	} /* XPROTECT */

	XEXCEPT
	{
		return(XCODE());
	}
	XEND;

	return(OM_ERR_NONE);
}

omfErr_t omfiOffsetToMobTimecode(
    omfHdl_t		file,         
	omfObject_t		mob,
	omfSegObj_t		tcSeg,
	omfPosition_t	offset,  
	omfTimecode_t	*result)	
{
  omfObject_t slot = NULL, subSegment = NULL, pdwn = NULL;
  omfSegObj_t tmpSlot = NULL, pdwnInput = NULL;
  omfIterHdl_t iterHdl = NULL;
  omfInt32 numSlots, loop, numSegs, sequLoop;
  omfTimecode_t timecode;
  omfBool tcFound = FALSE, reverse = FALSE;
  omfUInt32	frameOffset;
  omfDDefObj_t datakind = NULL;
  omfPosition_t newStart;
  omfInt32 start32;
  omfErr_t omfError = OM_ERR_NONE;
  omfPosition_t		begPos, endPos, sequPos;
  omfLength_t		segLen;
  omfIterHdl_t		sequIter = NULL;
  omfLength_t		zeroLen = omfsCvtInt32toLength(0, zeroLen);
  
	memset(result, 0, sizeof(omfTimecode_t));
	memset(&timecode, 0, sizeof(omfTimecode_t));
	result->startFrame = 0;

	omfAssertValidFHdl(file);

	XPROTECT(file)
	{
		if(tcSeg == NULL)
		{
			omfObject_t seg = NULL;
			
			/* Find timecode track in mob */
			CHECK(omfiIteratorAlloc(file, &iterHdl));
			CHECK(omfiMobGetNumSlots(file, mob, &numSlots));
			for (loop = 1; loop <= numSlots; loop++)
			{
				CHECK(omfiMobGetNextSlot(iterHdl, mob, NULL, &slot));
				CHECK(omfiMobSlotGetInfo(file, slot, NULL, &seg));

				/* Verify that it's a timecode track by looking at the
				 * datakind of the track segment. 
				 */
				CHECK(omfiComponentGetInfo(file, seg, &datakind, NULL));

				if (omfiIsTimecodeKind(file, datakind, kExactMatch, &omfError))
				{
					tcSeg = seg;
					break;
				}
				else
					CMReleaseObject((CMObject)seg);	
			}
		}
		if (tcSeg == NULL)
		{
			RAISE(OM_ERR_TIMECODE_NOT_FOUND);
		}
			
		if (omfiIsATimecodeClip(file, tcSeg, &omfError))
		{
			if (!tcFound)
			{
				CHECK(omfiTimecodeGetInfo(file, tcSeg, NULL, NULL, 
											&timecode));
				tcFound = TRUE;
			}
		} /* If timecode clip */
		  /* This is a special case for Avid Film Composer.  This
		   * is the timecode that we want - there will probably be
		   * another timecode track without a MASK object.
		   */
		else if (omfsIsTypeOf(file, tcSeg, "MASK", &omfError))
		{
			pdwn = tcSeg;
			CHECK(omfsGetNthObjRefArray(file, pdwn, OMTRKGTracks,
										  &tmpSlot, 1));
			CHECK(omfiMobSlotGetInfo(file, tmpSlot, NULL, &pdwnInput));
			CHECK(omfiTimecodeGetInfo(file, pdwnInput, NULL, NULL, 
										&timecode));
			tcFound = TRUE;
		}
		else if (omfsIsTypeOf(file, tcSeg, "PDWN", &omfError))
		{
			pdwn = tcSeg;
			CHECK(omfsReadObjRef(file, pdwn, OMPDWNInputSegment, &pdwnInput));
			CHECK(omfiTimecodeGetInfo(file, pdwnInput, NULL, NULL, 
										&timecode));
			tcFound = TRUE;
		}
		  else if (omfiIsASequence(file, tcSeg, &omfError))
			{
	  			CHECK(omfiIteratorAlloc(file, &sequIter));
	  			CHECK(omfiSequenceGetNumCpnts(file, tcSeg, &numSegs));
				for (sequLoop=0; sequLoop < numSegs; sequLoop++)
				{
					CHECK(omfiSequenceGetNextCpnt(sequIter, tcSeg,
													NULL, &sequPos, &subSegment));
					if (omfsIsTypeOf(file, subSegment, "MASK", &omfError))
					{
						pdwn = subSegment;
						CHECK(omfsGetNthObjRefArray(file, pdwn, OMTRKGTracks,
													  &tmpSlot, 1));
						CHECK(omfiMobSlotGetInfo(file, tmpSlot, NULL, &subSegment));
					}
					else if (omfsIsTypeOf(file, subSegment, "PDWN", &omfError))
					{
						pdwn = subSegment;
						CHECK(omfsReadObjRef(file, pdwn, OMPDWNInputSegment, &subSegment));
					}
					CHECK(omfiComponentGetLength(file, subSegment, &segLen));
					/* Skip zero-length clips, sometimes found in MC files */
					if (omfsInt64Equal(segLen, zeroLen))
						continue;
					begPos = sequPos;
					endPos = sequPos;
					CHECK(omfsAddInt64toInt64(segLen, &endPos));
					if (omfsInt64Less(offset, endPos) &&
						omfsInt64LessEqual(begPos, offset))
					{
			 			tcFound = TRUE;
			 			CHECK(omfiTimecodeGetInfo(file, subSegment, NULL, NULL, &timecode));
						omfsSubInt64fromInt64(sequPos, &offset);
						break;
					}
				} /* for */
				CHECK(omfiIteratorDispose(file, sequIter));
				sequIter = NULL;
				
				/* Release Bento reference, so the useCount is decremented */
				if (subSegment)
				  {
					 CMReleaseObject((CMObject)subSegment);	
					 subSegment = NULL;
				  }
			  } /* If Sequence */


		  /* Release Bento reference, so the useCount is decremented */
		  if (tcSeg)
			{
			  CMReleaseObject((CMObject)tcSeg);	
			  tcSeg = NULL;
			}
		  if (slot)
			{
			  CMReleaseObject((CMObject)slot);	
			  slot = NULL;
			}

	  /* Assume found at this point, so finish generating result */
	  /* If this is a Film Composer file that has a mask or pulldown object
	   * in the timecode track, pass the position through the mask/pulldown
	   * before adding it to the start timecode.
	   */
	  if (pdwn)
		{
		  reverse = FALSE;
		  CHECK(omfiPulldownMapOffset(file, pdwn,
								  offset, reverse, &newStart, NULL));
		  CHECK(omfsTruncInt64toInt32(newStart, &start32));
		  timecode.startFrame += start32;
		}
	  else
	  {
	  	CHECK(omfsTruncInt64toUInt32(offset, &frameOffset));
		timecode.startFrame += frameOffset;
	  }

	  *result = timecode;
	  if (iterHdl)
		{
		  CHECK(omfiIteratorDispose(file, iterHdl));
		  iterHdl = NULL;
		}
	} /* XPROTECT */

  XEXCEPT
	{
	  if (iterHdl)
		omfiIteratorDispose(file, iterHdl);
	  if (sequIter)
		omfiIteratorDispose(file, sequIter);
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);

}

omfErr_t omfPvtGetPulldownMask(omfHdl_t file,
			   omfPulldownKind_t	pulldown,
			   omfUInt32 			*outMask,
			   omfInt32				*maskLen,
			   omfBool	 			*isOneToOne)
{
	switch(pulldown)
	{
		/* !! How do we handle "average of fields" mode (it's still even or odd dominant */
		/* NTSC pullown pattern is:
		 *		AA BB BC CD DD
		 *			Looking at the odd field, we see:
		 *				A B B C D
		 */
		case kOMFTwoThreePD:
			*outMask = 0xD8000000;	/* 3623878656 decimal */
			*maskLen = 5;
			*isOneToOne = FALSE;
			break;
		/* PAL pullown pattern is:
		 *		AA BB CC DD EE FF GG HH II JJ KK LL MM MN NO OP PQ QR RS ST TU UV VW WX XY YY
		 *			Looking at the odd field, we see:
		 *				A B C D E F G H I J K L M M N O P Q R S T U V W X Y
		 */
		case kOMFPALPD:
			*outMask = 0xFFF7FF80;
			*maskLen = 25;
			*isOneToOne = FALSE;
			break;
			
		case kOMFOneToOneNTSC:
		case kOMFOneToOnePAL:
			*isOneToOne = TRUE;
			
		default:
			return(OM_ERR_PULLDOWN_KIND);
	}
	
	return(OM_ERR_NONE);
}

/* Functions to support calculating dropping/doubling frames with masks */
omfErr_t omfiPulldownMapOffset(omfHdl_t file,
			   omfObject_t pulldown,
			   omfPosition_t offset,
			   omfBool reverse,
			   omfLength_t *numFrames,
			   omfInt32 *srcPhase)
{
  omfESlotObj_t	effectSlot = NULL;
  omfSegObj_t	pulldownCVAL = NULL;
  omfUInt32     maskBits, shiftBits;
  omfBool       drop;
  omfUInt32     phaseOffset = 0;
  omfInt32	        sign, revolutions, ones = 0L, tmpOnes = 0L;
  char          	masksize, maskones, remainder;
  omfPosition_t 	zeroPos;
  omfInt32     		offset32, full = 0L, maskLen;
	omfPulldownKind_t	pulldownKind;
	omfPhaseFrame_t		phaseFrame;
	omfPulldownDir_t	direction;
	omfBool				oneToOne;
	
  XPROTECT(file)
    {
      /* Get properties off of the pulldown */
      if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		{
		  CHECK(omfsReadInt16(file, pulldown, OMWARPPhaseOffset, 
							  &phaseFrame));
		  phaseOffset = (omfUInt32)phaseFrame;
		  CHECK(omfsReadUInt32(file, pulldown, OMMASKMaskBits, &maskBits));
		  CHECK(omfsReadBoolean(file, pulldown, OMMASKIsDouble, &drop));
		  oneToOne = FALSE;
		  shiftBits = maskBits;
		  maskLen = 0;
		  while(shiftBits != 0)
			{
			  shiftBits = shiftBits << 1L;
			  maskLen++;
			}
		}
      else /* kOmfRev2x */
		{
			CHECK(omfsReadPhaseFrameType(file, pulldown, OMPDWNPhaseFrame, &phaseFrame));
			phaseOffset = (omfUInt32)phaseFrame;
			
			CHECK(omfsReadPulldownDirectionType(file, pulldown, OMPDWNDirection, &direction));
			drop = (direction == kOMFTapeToFilmSpeed);
			CHECK(omfsReadPulldownKindType(file, pulldown, OMPDWNPulldownKind, &pulldownKind));
			CHECK(omfPvtGetPulldownMask(file, pulldownKind, &maskBits, &maskLen, &oneToOne));
		}

	  if(oneToOne)
	  {
	  	if(numFrames)
	  		*numFrames = offset;
	  }
	  else
	  {		
		  if (reverse)
			drop = !drop;

		  omfsCvtInt32toPosition(0, zeroPos);
		  if (omfsInt64Less(offset, zeroPos))
			sign = -1;
		  else 
			sign = 1;
		  omfsTruncInt64toInt32(offset, &offset32);	/* OK FRAMEOFFSET */

		  MaskGetBits(maskBits, &maskones);
		  masksize = (char)maskLen;
		  /* If dropping frames */
		  if (drop)
			{
			  if (maskBits)
				{
				  revolutions = abs(offset32) / masksize;
				  remainder = offset32 % masksize;
				  if(srcPhase != NULL)
				  	*srcPhase = remainder;
				  
				  ones = revolutions * maskones;
				  GetRemFramesDrop(maskBits, remainder, phaseOffset, masksize, &tmpOnes);
				  ones += tmpOnes;
				  ones *= sign;

				  if (numFrames)
					omfsCvtInt32toInt64(ones, numFrames);
				}
			} 

		  else /* Doubling frames */
			{
			  if (maskBits)
				{
				  revolutions = abs(offset32) / maskones;
				  remainder = offset32 % maskones;
				  if(srcPhase != NULL)
				  	*srcPhase = remainder;

				  full = revolutions * masksize;
				  GetRemFramesDouble(maskBits, remainder, phaseOffset, masksize, &tmpOnes);
				  full += tmpOnes;
				  full *= sign;

				  if (numFrames)
					omfsCvtInt32toInt64(full, numFrames);
				}
			}
		}
    }

  XEXCEPT
    {
    }
  XEND;

  return(OM_ERR_NONE);
}

omfErr_t GetRemFramesDrop(omfUInt32 maskBits, 
							char ones, 
							omfUInt32 phase,
							omfUInt32 masksize,
							omfInt32 *result)
{
  int  remMask, maskBitsLeft;
  omfInt32 ret;
  unsigned char phasePtr;

  remMask = maskBits;
  
  /* Shift mask to start at phase we want */
  for (phasePtr=0; (phasePtr<phase) && remMask; remMask<<=1,phasePtr++)
	;

  for (ret=0, maskBitsLeft = masksize; ones--; remMask<<=1, maskBitsLeft--)
	{
	  if (maskBitsLeft == 0)
	  {
	  	remMask = maskBits;
	  	maskBitsLeft = masksize;
	  }
	  if (remMask<0L)  /* Is high bit a 1 */
		ret++;
	}

  if (result)
	*result = ret;

  return(OM_ERR_NONE);
 }

omfErr_t GetRemFramesDouble(omfUInt32 maskBits, 
							char ones,
							omfUInt32 phase,
							omfUInt32 masksize,
							omfInt32 *result)
{
  int  remMask, maskBitsLeft;
  omfInt32 ret;
  omfUInt32 phasePtr;

  remMask = maskBits;

  /* Shift mask to start at phase we want */
  for (phasePtr=0; (phasePtr<phase) && remMask; remMask<<=1,phasePtr++)
	;
  for (ret=0,phasePtr=phase, maskBitsLeft = masksize; ones; remMask<<=1,
  										phasePtr++, maskBitsLeft--)
	{
	  if (maskBitsLeft == 0)
		{
		  remMask = maskBits;
		  phasePtr = 0;
	  	maskBitsLeft = masksize;
		}
	  ret++;
	  if (remMask<0L)
		ones--;
	}

  if (result)
	*result = ret;
  return(OM_ERR_NONE);
}

omfErr_t MaskGetBits(omfUInt32 maskBits, char *maskones)
{
  omfInt32 tmpMask;
  char		ones;

  /* Iterate over the mask counting all bits and 1-bits */
  for (tmpMask=maskBits,ones=0; tmpMask; tmpMask<<=1)
	{
	  /* If high bit is 1 */
	  if (tmpMask<0) 
		ones++;
	}

  if (maskones)
	*maskones = ones;
  return(OM_ERR_NONE);
 }

/*************************************************************************
 * Private Function: FindSlotByArgID()
 *
 *      Given an argID, this internal function returns the corresponding effect
 *      slot, effect slot value, and effect slot edit rate.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t FindSlotByArgID(
		const omfHdl_t 		 file,     /* IN - File Handle */
		const omfSegObj_t 	 effect,   /* IN - Effect Object */
		const omfArgIDType_t argID,    /* IN - Arg ID */
  		omfESlotObj_t  *effectSlot,    /* OUT - Effect Slot object */
		omfSegObj_t 	 *slotValue)   /* OUT - Effect Slot value */
{
	omfIterHdl_t iterHdl = NULL;
	omfIterHdl_t tmpIterHdl = NULL;
	omfESlotObj_t slot = NULL;
	omfArgIDType_t slotArgID = 0;

	/* NOTE: It would be nice to search for an effect slot by the argID
	 * using the search criteria.  This is not currently implemented.
	 */
	
	*effectSlot = NULL;

	XPROTECT(file)
	{
		CHECK(omfiIteratorAlloc(file, &iterHdl));
		
		CHECK(omfiEffectGetNextSlot(iterHdl, effect, NULL, &slot));
	   
		/* look through all the slots for one that matches the given ID */

		while (slot != NULL)
		{
			CHECK(omfiEffectSlotGetInfo(file, slot, &slotArgID, 
										slotValue));
		
			if (slotArgID == argID)
			{
				*effectSlot = slot;
				tmpIterHdl = iterHdl;
				iterHdl = NULL;
				CHECK(omfiIteratorDispose(file, tmpIterHdl));
				return(OM_ERR_NONE);
			}
			
			CHECK(omfiEffectGetNextSlot(iterHdl, effect, 
										NULL, &slot));			
		}
		
		tmpIterHdl = iterHdl;
		iterHdl = NULL;  /* prevent redundancy in exception handling */
		CHECK(omfiIteratorDispose(file, tmpIterHdl));
		tmpIterHdl = NULL;
	}
	XEXCEPT
	{
		if (iterHdl)
			(void)omfiIteratorDispose(file, iterHdl);  /* don't CHECK here */
		if (XCODE() == OM_ERR_NO_MORE_OBJECTS)
		  {
			*effectSlot = NULL;
			*slotValue = NULL;
			return(OM_ERR_NONE);
		  }
		else
		  return(XCODE());
	}
	XEND;
	
	return(OM_ERR_NONE);
}

#ifdef VIRTUAL_BENTO_OBJECTS
/*************************************************************************
 * Function: omfiMobPurge()
 *
 *      This function purges the mob with the given mobID from the
 *      file specified by the file handle.  It will purge all objects
 *      contained in the mob.
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiMobPurge(
	omfHdl_t 	file,   /* IN - File Handle */
	omfObject_t mob) 	/* IN - Mob ID of mob to purge */
{
	omfAssertValidFHdl(file);

	return(objectPurgeTree(file, mob));
}

/*************************************************************************
 * Function: omfiObjectPurgeTree()		INTERNAL
 *
 *      This function recursively purges all objects in the subtree rooted
 *      at the object identified by the 'obj' parameter.  It is used by
 *      the omfiMobPurge() function.
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
static omfErr_t objectPurgeTree(
    omfHdl_t file,    /* IN - File Handle */
	omfObject_t obj)  /* IN - Object to purge */
{
    omfIterHdl_t propIter = NULL;
	omfProperty_t prop;
	omfType_t type;
	CMProperty cprop;
	CMType ctype;
	CMValue cvalue;
	omfInt32 arrayLen, loop;
	omfObject_t objRef;
	omfErr_t omfError = OM_ERR_NONE;

	clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);

	XPROTECT(file)
	  {
		CHECK(omfiIteratorAlloc(file, &propIter));

		CHECK(omfiGetNextProperty(propIter, obj, &prop, &type));
		while (prop)
		  {
			cprop = CvtPropertyToBento(file, prop);
			ctype = CvtTypeToBento(file, type, NULL);
			if (file->BentoErrorRaised)
			  {
				RAISE(OM_ERR_BENTO_PROBLEM);
			  }
			if (CMCountValues((CMObject)obj, cprop, ctype))
			  {
				cvalue = CMGetNextValue((CMObject)obj, cprop, NULL);
				if (type == OMObjRefArray)
				  {
					arrayLen = omfsLengthObjRefArray(file, obj,
												 (omfProperty_t)prop);
					for (loop=1; loop<=arrayLen; loop++)
					  {
						CHECK(omfsGetNthObjRefArray(file, obj, prop, &objRef, 
												loop));
						  /* If not Datakind or Effectdef, purge object */
						  if (objRef)
							{
							  if (!omfiIsADatakind(file, objRef, &omfError) && 
								  !omfiIsAnEffectDef(file, objRef, &omfError))
								{
								  if (omfError == OM_ERR_NONE)
									{
									  CHECK(objectPurgeTree(file, objRef));
									}
								  else
									{
									  RAISE(omfError);
									}
								}
							}
					  } /* for */
				  } /* If ObjRefArray */
				else if (type == OMObjRef)
				  {
					CHECK(omfsReadObjRef(file, obj, prop, &objRef));
					  if (objRef)
						{
						  /* If not Datakind or Effectdef, purge object */
						  if (!omfiIsADatakind(file, objRef, &omfError) && 
							  !omfiIsAnEffectDef(file, objRef, &omfError))
							{
							  if (omfError == OM_ERR_NONE)
								{
								  CHECK(objectPurgeTree(file, objRef));
								}
							  else
								{
								  RAISE(omfError);
								}
							}
						}
				  } /* If ObjRef */
			  } /* if Count Values */
			omfError = omfiGetNextProperty(propIter, obj, &prop, &type);
			if(omfError == OM_ERR_NO_MORE_OBJECTS)
				break;
			else
				RAISE(omfError);
		  } /* while */
		
		/* After all subtrees are purged, purge obj */
		CHECK(omfsObjectPurge(file, obj));

	  } /* XPROTECT */

	XEXCEPT
	  {
		/* After all subtrees are purge, purge obj */
		if (propIter)
		  omfiIteratorDispose(file, propIter);
	  }
    XEND;

	if (propIter)
	  omfiIteratorDispose(file, propIter);
	return(OM_ERR_NONE);
}
#endif

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
