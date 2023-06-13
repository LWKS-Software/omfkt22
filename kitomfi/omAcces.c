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
 * Name: omAcces.c
 *
 * Function: The main API file for object/property layer operations.
 *
 * Audience: Clients Requiring a more specialized interface than the
 *			composition convenience layer provides.
 *
 * All functions can return the following error codes if the following
 * argument values are NULL:
 *		OM_ERR_NULL_FHDL -- omfHdl_t was NULL.
 *		OM_ERR_NULLOBJECT -- omfObject_t was NULL.
 *		OM_ERR_BADDATAADDRESS -- Data address was NULL.
 *		OM_ERR_NULL_SESSION -- No session was open.
 *		OM_ERR_SWAB -- Unable to byte swap the given data type.
 * 
 * Any function may also return the following error codes:
 *		OM_ERR_BENTO_PROBLEM -- Bento returned an error, check BentoErrorNumber.
 * 
 * Accessor functions can also return the error codes below:
 *		OM_ERR_BAD_PROP -- omfProperty_t code was out of range.
 *		OM_ERR_OBJECT_SEMANTIC -- Failed a semantic check on an input obj
 *		OM_ERR_DATA_IN_SEMANTIC -- Failed a semantic check on an input data
 *		OM_ERR_DATA_OUT_SEMANTIC -- Failed a semantic check on an output data
 */

#include "masterhd.h"
#include <string.h>

#include "omPublic.h"
#include "omCntPvt.h"
#include "omPvt.h"



/*
 * Basic Accessor Functions
 */

#if OMFI_ENABLE_SEMCHECK
#define CHECK_DATA_IN(file, dataPtr) \
    { omfErr_t err;\
		err = omfsCheckDataValidity((omfHdl_t)file, (omfProperty_t)prop, \
						 (const void*)dataPtr, (omfAccessorType_t)kOmSetFunction); \
      if (err != OM_ERR_NONE) { RAISE(OM_ERR_DATA_IN_SEMANTIC) } }
#define CHECK_DATA_OUT(file, dataPtr) \
    { omfErr_t err; \
		err = omfsCheckDataValidity((omfHdl_t)file, (omfProperty_t)prop, \
						 (const void*)dataPtr, (omfAccessorType_t)kOmSetFunction); \
      if (err != OM_ERR_NONE) { RAISE(OM_ERR_DATA_OUT_SEMANTIC) } }
#else
#define CHECK_DATA_IN(file, dataPtr)
#define CHECK_DATA_OUT(file, dataPtr)
#endif

/* A 75-column ruler
         111111111122222222223333333333444444444455555555556666666666777777
123456789012345678901234567890123456789012345678901234567890123456789012345
*/
static omfBool OKToWriteProperties(omfHdl_t file)
{
	omfMediaHdl_t	media = file->topMedia;
	
	while(media != NULL)
	{
		if(media->openType == kOmfiCreated)
		{
			if((media->stream != NULL) && (media->stream->cookie == STREAM_COOKIE))
				return(FALSE);
		}
		media = media->pvt->nextMedia;
	}
	
	return(TRUE);
}

/***********************************************************************
 * omfBool (8-bit) processing
 *
 ***********************************************************************/

/************************
 * Function: omfsReadBoolean/omfsWriteBoolean
 *
 * 		Read/Write a boolean value from the given property of the given
 *		object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions, accessor, and Bento errors.
 *		See top of the file.
 *		OM_ERR_BAD_PROP -- omfProperty_t code was out of range.
 */
omfErr_t omfsReadBoolean(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfBool 			*data)/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMBoolean, sizeof(omfBool),
								data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteBoolean
 */
omfErr_t omfsWriteBoolean(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfBool			data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMBoolean,
								sizeof(omfBool), &data));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfsReadInt8/omfsWriteInt8
 *
 * 	Read/Write a character value from the given property of the
 *		given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadInt8(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
		   char	        	*data)/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMInt8,
								sizeof(char), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	XEND
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteInt8
 */
omfErr_t omfsWriteInt8(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			char				data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMInt8,
					sizeof(char), &data));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfsReadInt16/omfsWriteInt16
 *
 * 	Read/Write a short(omfInt16) value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadInt16(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t 	obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfInt16			*data)/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMInt16, sizeof(omfInt16), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteInt16
 */
omfErr_t omfsWriteInt16(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfInt16			data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMInt16, sizeof(omfInt16),
										&data));
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfsLengthString/omfsReadString/omfsWriteString
 *
 * 	Returns the length of the string property on a Bento object.
 * 	Read/Write a string value from the given property of the given
 *		object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		For omfsLengthString:
 *			The length of the string property, or 0 on error.
 *		Others:
 *			Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfInt32 omfsLengthString(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop)		/* IN - Return the length */
{
	omfLength_t          len;
	omfInt32					result;
	
	XPROTECT(file)
	{
		CHECK(OMLengthProp(file, obj, prop, OMString, -1, &len));
		CHECK(omfsTruncInt64toInt32(len, &result));	/* OK */
	}
	XEXCEPT
	{
		return(0);
	}
	XEND

	return (result);
}

/************************
 * Function: omfsReadString
 */
omfErr_t omfsReadString(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */
			char				*data,	/* IN - into a buffer */
			omfInt32			maxsize)	/* IN - of this size */
{
	omfInt32				siz, maxLen;
	omfPosition_t		zero;
	
	omfsCvtInt32toPosition(0, zero);
	
	XPROTECT(file)
	{
		maxLen = maxsize - 1;
		siz = omfsLengthString(file, obj, prop);
		if (maxLen > siz)
			maxLen = siz;
	
		CHECK(OMReadProp(file, obj, prop, zero, kNeverSwab, OMString,
								maxLen, data));
	
		/* if the size is the maximum allowed in the read call, and if the
		 * last character is not NULL, then terminate the string.  Note
		 * that the index for the array is zero-based, so the values are
		 * 1 less than the ordinal value.
		 */
		siz = omfsLengthString(file, obj, prop);
		if ((siz == (maxLen)) && (data[maxLen - 1] != 0))
		{
			data[maxLen] = 0;
			siz++;
		}
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteString
 */
omfErr_t omfsWriteString(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			const char		*data)/* OUT - and return the result here */
{
	omfPosition_t		zero;
	
	omfsCvtInt32toPosition(0, zero);

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);

		// this is a fix for MC bug that expecting all Strings/UniqueNames to
		// be non-immediate
		if (strlen(data) < 4)
		{
			char tmp[5];

			memset(tmp, 0, 5);
			strcpy(tmp, data);
			CHECK (OMWriteProp(file, obj, prop, zero, OMString, 5,
									tmp));
			
		}
		else
			CHECK (OMWriteProp(file, obj, prop, zero, OMString, strlen(data) + 1,
								data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadUInt8/omfsWriteUInt8
 *
 * 	Read/Write a unsigned char value from the given property of the
 *		given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadUInt8(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfUInt8			*data)/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMUInt8,
									sizeof(unsigned char), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteUInt8
 */
omfErr_t omfsWriteUInt8(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfUInt8			data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMUInt8,
										sizeof(unsigned char), &data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfsReadUInt32/omfsWriteUInt32
 *
 * 	Read/Write a omfUInt32 value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadUInt32(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfUInt32		*data)/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMUInt32,
									sizeof(omfUInt32), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteUInt32
 */
omfErr_t omfsWriteUInt32(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfUInt32		data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMUInt32,
										sizeof(omfUInt32), &data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadUInt16/omfsWriteUInt16
 *
 * 	Read/Write a omfUInt16 value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadUInt16(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfUInt16		*data)/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMUInt16,
									sizeof(omfUInt16), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteUInt16
 */
omfErr_t omfsWriteUInt16(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfUInt16		data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMUInt16,
										sizeof(omfUInt16), &data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadVarLenBytes/omfsWriteVarLenBytes/omfsLenVarLenBytes
 *
 * 	Read/Write a variable number of bytes from the given property
 *		of the given object.
 *
 * Argument Notes:
 *		The property must be of type "varLenBytes".
 *
 * ReturnValue:
 *		For omfsLenVarLenBytes:
 *			The length of the varLenBytes property, or 0 on error.
 *		Others:
 *			Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadVarLenBytes(
			omfHdl_t			file,			/* IN - From this file */
			omfObject_t		obj,			/* IN - and this object */
			omfProperty_t	prop,			/* IN - read this property */
			omfPosition_t	offset,		/* IN - from this Bento offset */
			omfInt32			maxsize,		/* IN - for this many bytes */
			void				*data,		/* IN/OUT - into this buffer */
			omfUInt32		*bytesRead)	/* OUT - and return the count here */
{
	omfErr_t    lenStat;
	omfLength_t	bytes;
	omfErr_t		status;

	XPROTECT(file)
	{
		status = OMReadProp(file, obj, prop, offset, kNeverSwab,
											OMVarLenBytes, maxsize, data);
		if (status == OM_ERR_NONE)
			*bytesRead = maxsize;
		else
		{
			lenStat = OMLengthProp(file, obj, prop, OMVarLenBytes, 0, &bytes);
			if (lenStat == OM_ERR_NONE)
			{
				if(omfsInt64Greater(offset, bytes))
					*bytesRead = 0;
				else
				{
					CHECK(omfsSubInt64fromInt64(offset, &bytes));
					if(omfsTruncInt64toUInt32(bytes, bytesRead) != OM_ERR_NONE)	/* OK MAXREAD */
						*bytesRead = 0;
				}
			}
			else
				*bytesRead = 0;
		}
	
		if (*bytesRead != (omfUInt32)maxsize)
			RAISE(OM_ERR_END_OF_DATA);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND


	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteVarLenBytes
 */
omfErr_t omfsWriteVarLenBytes(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - write this property */
			omfPosition_t	offset,	/* IN - at this bento offset */
			omfInt32			count,	/* IN - for this many bytes */
			const void		*data)	/* IN - with this data */
{
	XPROTECT(file)
	{
		CHECK (OMWriteProp(file, obj, prop, offset, OMVarLenBytes,
								count, data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsLengthVarLenBytes
 */
omfLength_t omfsLengthVarLenBytes(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop)	/* IN - get the length of this property */
{
	omfLength_t	result, zero = omfsCvtInt32toLength(0, zero);
	
	XPROTECT(file)
	{
		CHECK(OMLengthProp(file, obj, prop, OMVarLenBytes, 0, &result));
	}
	XEXCEPT
	{
		return(zero);
	}
	XEND_SPECIAL(zero)
	
	return (result);
}

/************************
 * Function: omfsReadInt32/omfsWriteInt32
 *
 * 	Read/Write a omfInt32 value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadInt32(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfInt32			*data)	/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMInt32, sizeof(omfInt32), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteInt32
 */
omfErr_t omfsWriteInt32(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfInt32			data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMInt32,
										sizeof(omfInt32), &data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}


/*
 * OMFI-Specific Accessor Functions
 */

/************************
 *  Function: omfsReadCharSetType/omfsWriteCharSetType
 *
 * 	Read/Write a omfCharSetType value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadCharSetType(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN - read this property */
			omfCharSetType_t	*data)/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMCharSetType,
									sizeof(omfInt16), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteCharSetType
 */
omfErr_t omfsWriteCharSetType(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN - Write this property */
			omfCharSetType_t	data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMCharSetType,
										sizeof(omfInt16), &data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsReadAttrKind/omfsWriteAttrKind
 *
 * 	Read/Write a omfAttrKind value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadAttrKind(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN - read this property */
			omfAttributeKind_t	*data)      /* OUT - and return the result here */
{
 	omfLength_t 	len;
 	omfInt32		len32;
	omfInt16 localData;

	XPROTECT(file)
	{
 		CHECK(OMLengthProp(file, obj, prop, OMAttrKind, 0, &len));
		CHECK(omfsTruncInt64toInt32(len, &len32));	/* OK */
		CHECK(OMReadBaseProp(file, obj, prop, OMAttrKind,
									sizeof(omfInt16), &localData));
		CHECK_DATA_OUT(file, &localData);
		*data = (omfAttributeKind_t)localData;
		/* HACK -- Many old Composer files write NULL kind for all attributes */
		if(*data == kOMFNullAttribute)
		{
			if(omfsIsPropPresent(file, obj, OMATTBIntAttribute, OMInt32))
				*data = kOMFIntegerAttribute;
			else if(omfsIsPropPresent(file, obj, OMATTBStringAttribute, OMString))
				*data = kOMFStringAttribute;
			else if(omfsIsPropPresent(file, obj, OMATTBObjAttribute, OMObjRef))
				*data = kOMFObjectAttribute;
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteAttrKind
 */
omfErr_t omfsWriteAttrKind(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN - Write this property */
			omfAttributeKind_t	data)	      /* IN - with this value */
{
	omfInt16 localData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		localData = (omfInt16)data;
		CHECK_DATA_IN(file, &localData);
		CHECK (OMWriteBaseProp(file, obj, prop, OMAttrKind,
										sizeof(omfInt16), &localData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsReadEdgeType/omfsWriteEdgeType
 *
 * 	Read/Write a omfEdgeType_t value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadEdgeType(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */ 
			omfEdgeType_t	*data)	/* OUT - and return the result here */
{
  	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMEdgeType, sizeof(omfInt16), 
						&tmpData));
		
		*data = (omfEdgeType_t)tmpData;
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteEdgeType
 */
omfErr_t omfsWriteEdgeType(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t 	obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - Write this property */ 
			omfEdgeType_t	data)		/* IN - with this value */
{
  omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;

		CHECK (OMWriteBaseProp(file, obj, prop, OMEdgeType, sizeof(omfInt16), 
			  &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsReadTrackType/omfsWriteTrackType
 *
 * 	Read/Write a trackType value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadTrackType(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */
			omfInt16			*data)	/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMTrackType,
									sizeof(omfInt16), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteTrackType
 */
omfErr_t omfsWriteTrackType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfInt16			data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMTrackType,
										sizeof(omfInt16), &data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsReadRational/omfsWriteRational
 *
 * 	Read/Write a rational value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadRational(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */
			omfRational_t	*data)	/* OUT - and return the result here */
{
	omfPosition_t	field1, field2;
	
	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(sizeof(omfInt32), field2);
	
	XPROTECT(file)
	{
		CHECK(OMReadProp(file, obj, prop, field1, kSwabIfNeeded, OMRational,
					  sizeof(omfInt32), &data->numerator));
		CHECK(OMReadProp(file, obj, prop, field2, kSwabIfNeeded,
						OMRational, sizeof(omfInt32), &data->denominator));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteRational
 */
omfErr_t omfsWriteRational(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfRational_t	data)	/* IN - with this value */
{
	omfPosition_t	field1, field2;
	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(sizeof(omfInt32), field2);

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK(OMWriteProp(file, obj, prop, field1, OMRational,
					   sizeof(omfInt32), &data.numerator));
		CHECK(OMWriteProp(file, obj, prop, field2, OMRational,
					   sizeof(omfInt32), &data.denominator));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsReadExactEditRate/omfsWriteExaceEditRate
 *
 * 	Read/Write a rational value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadExactEditRate(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */
			omfRational_t	*data)	/* OUT - and return the result here */
{
	omfPosition_t	field1, field2;
	
	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(sizeof(omfInt32), field2);

	XPROTECT(file)
	{
		CHECK(OMReadProp(file, obj, prop, field1, kSwabIfNeeded, OMExactEditRate,
					  sizeof(omfInt32), &data->numerator));
		CHECK(OMReadProp(file, obj, prop, field2, kSwabIfNeeded,
						OMExactEditRate, sizeof(omfInt32), &data->denominator));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteExactEditRate
 */
omfErr_t omfsWriteExactEditRate(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN - write this property */
			omfRational_t		data)	/* IN - with this value */
{
	omfPosition_t	field1, field2;
	
	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(sizeof(omfInt32), field2);

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK(OMWriteProp(file, obj, prop, field1, OMExactEditRate,
					   sizeof(omfInt32), &data.numerator));
		CHECK(OMWriteProp(file, obj, prop, field2, OMExactEditRate,
					   sizeof(omfInt32), &data.denominator));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsRead/WriteFilmType
 *
 * 	Read/Write a omfFilmType_t value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadFilmType(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */ 
			omfFilmType_t	*data)	/* OUT - and return the result here */
{
  	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMFilmType, sizeof(omfInt16), 
									&tmpData));
	
		*data = (omfFilmType_t)tmpData;
		CHECK_DATA_OUT(file, data);  
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteFilmType
 */
omfErr_t omfsWriteFilmType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */ 
			omfFilmType_t	data)	/* IN - with this value */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMFilmType,
									sizeof(omfInt16), &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}


/************************
 *  Function: omfsRead/WriteDirectionCode
 *
 * 	Read/Write a omfDirectionCode_t value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadDirectionCode(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */ 
			omfDirectionCode_t	*data)	/* OUT - and return the result here */
{
  	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMDirectionCode, sizeof(omfInt16), 
									&tmpData));
	
		*data = (omfDirectionCode_t)tmpData;
		CHECK_DATA_OUT(file, data);  
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteDirectionCode
 */
omfErr_t omfsWriteDirectionCode(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */ 
			omfDirectionCode_t	data)	/* IN - with this value */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMDirectionCode,
									sizeof(omfInt16), &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}


/************************
 *  Function: omfsRead/WriteColorSpace
 *
 * 	Read/Write a omfColorSpace_t value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadColorSpace(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */ 
			omfColorSpace_t	*data)	/* OUT - and return the result here */
{
  	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMColorSpace, sizeof(omfInt16), 
									&tmpData));
	
		*data = (omfColorSpace_t)tmpData;
		CHECK_DATA_OUT(file, data);  
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteColorSpace
 */
omfErr_t omfsWriteColorSpace(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */ 
			omfColorSpace_t	data)	/* IN - with this value */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMColorSpace,
									sizeof(omfInt16), &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}


/************************
 * Function: omfsReadTapeCaseType/omfsWriteTapeCaseType
 *
 * 	Read/Write a omfTapeCaseType_t value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadTapeCaseType(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */ 
			omfTapeCaseType_t	*data)/* OUT - and return the result here */
{
  	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMTapeCaseType, sizeof(omfInt16), 
									&tmpData));
	
		*data = (omfTapeCaseType_t)tmpData;
		CHECK_DATA_OUT(file, data);  
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteTapeCaseType
 */
omfErr_t omfsWriteTapeCaseType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */ 
			omfTapeCaseType_t	data)	/* IN - with this value */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMTapeCaseType,
									sizeof(omfInt16), &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsReadTapeFormatType/omfsWriteTapeFormatType
 *
 * 	Read/Write a omfTapeFormatType_t value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadTapeFormatType(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */ 
			omfTapeFormatType_t	*data)/* OUT - and return the result here */
{
  	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMTapeFormatType, sizeof(omfInt16),
									&tmpData));
	
		*data = (omfTapeFormatType_t)tmpData;
		CHECK_DATA_OUT(file, data);  
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteTapeFormatType
 */
omfErr_t omfsWriteTapeFormatType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */ 
			omfTapeFormatType_t	data)	/* IN - with this value */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMTapeFormatType,
									sizeof(omfInt16), &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}


/************************
 *  Function: omfsReadVideoSignalType/omfsWriteVideoSignalType
 *
 * 	Read/Write a omfVideoSignalType_t value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadVideoSignalType(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */ 
			omfVideoSignalType_t	*data)	/* OUT - and return the result here */
{
  	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMVideoSignalType, 
									sizeof(omfInt16), &tmpData));
	
		*data = (omfVideoSignalType_t)tmpData;
		CHECK_DATA_OUT(file, data);  
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteVideoSignalType
 */
omfErr_t omfsWriteVideoSignalType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */ 
			omfVideoSignalType_t	data)	/* IN - with this value */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMVideoSignalType,
									sizeof(omfInt16), &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsReadJPEGTableIDType/omfsReadJPEGTableIDType
 *
 * 	Read/Write a omfJPEGTableID_t value from the given property
 *		of the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadJPEGTableIDType(
			omfHdl_t				file,		/* IN - From this file */
			omfObject_t			obj,		/* IN - and this object */
			omfProperty_t		prop,		/* IN - read this property */
			omfJPEGTableID_t	*data)	/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMJPEGTableIDType,
									sizeof(omfInt32), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteJPEGTableIDType
 */
omfErr_t omfsWriteJPEGTableIDType(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN - write this property */ 
			omfJPEGTableID_t	data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMJPEGTableIDType,
										sizeof(omfInt32), &data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadPhysicalMobType / omfsWritePhysicalMobType
 *
 * 	Read/Write a omfPhysicalMobType_t value from the given property
 *		of the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadPhysicalMobType(
			omfHdl_t					file,	/* IN - From this file */
			omfObject_t				obj,	/* IN - and this object */
			omfProperty_t			prop,	/* IN - read this property */
			omfPhysicalMobType_t	*data)/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMPhysicalMobType,
									sizeof(omfInt16), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWritePhysicalMobType
 */
omfErr_t omfsWritePhysicalMobType(
			omfHdl_t					file,	/* IN - From this file */
			omfObject_t				obj,	/* IN - and this object */
			omfProperty_t			prop,	/* IN - write this property */ 
			omfPhysicalMobType_t	data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMPhysicalMobType,
										sizeof(omfInt16), &data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsGetNthObjIndex
 * Function:omfsPutNthObjIndex
 * Function: omfsAppendObjIndex
 * Function: omfsLengthObjIndex
 *
 * 	Routines to work with rev1.x ObjectIndexes.  This data
 *		type is obsolete in 2.0, and these routines should only be
 *		used when reading/writing rev1.x files.
 *
 * Argument Notes:
 *		i - Index is 1-based.
 *
 * ReturnValue:
 *		For omfsLengthObjIndex:
 *			The length of the objIndex property, or 0 on error.
 *		Others:
 *			Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsGetNthObjIndex(
			omfHdl_t					file,		/* IN - From this file */
			omfObject_t				obj,		/* IN - and this object */
			omfProperty_t			prop,		/* IN - and this property */
			omfObjIndexElement_t	*data,	/* OUT - read index element into here */
			omfInt32					i)			/* IN - from this index (1-based) */
{
	omfPosition_t	offset;
	omfInt32			bytesPerObjref;
	CMReference    refData;
	omfBool		swab;
	
	XPROTECT(file)
	{
		swab = ompvtIsForeignByteOrder(file, obj);

#if OMFI_ENABLE_SEMCHECK
		XASSERT(mobIndexInBounds(file, obj, prop, i), OM_ERR_BADINDEX)
#endif
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			bytesPerObjref = OMBYTES_PER_OBJREF_1X;
		else
			bytesPerObjref = sizeof(CMReference);
		
		CHECK(OMGetNthPropHdr(file, obj, prop, i, OMMobIndex,
				    OMBYTES_PER_UID + bytesPerObjref, &offset));
		CHECK(OMReadProp(file, obj, prop, offset, kNeverSwab, OMMobIndex,
					  sizeof(omfInt32), &data->ID.prefix));
		CHECK(omfsAddInt32toInt64(sizeof(omfInt32), &offset));
		CHECK(OMReadProp(file, obj, prop, offset,
								kNeverSwab, OMMobIndex, 
				       		sizeof(omfInt32), &data->ID.major));
		CHECK(omfsAddInt32toInt64(sizeof(omfInt32), &offset));
		CHECK(OMReadProp(file, obj, prop, offset,
			kNeverSwab, OMMobIndex, sizeof(omfInt32), &data->ID.minor));
		if (swab)
		{
			omfsFixLong(&data->ID.prefix);
			omfsFixLong((omfInt32 *)&data->ID.major);
			omfsFixLong((omfInt32 *)&data->ID.minor);
		}
		
		CHECK(omfsAddInt32toInt64(OMBYTES_PER_UID-(2*sizeof(omfInt32)), &offset));
		CHECK(OMReadProp(file, obj, prop, offset, kNeverSwab,
								OMMobIndex, sizeof(CMReference), refData));
		CHECK(GetReferencedObject(file, obj, prop, OMMobIndex, refData,
											&data->Mob));
	
#if OMFI_ENABLE_SEMCHECK 
		CHECK(omfsCheckObjRefValidity(file, prop, data->Mob, kOmGetFunction));
#endif
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsPutNthObjIndex
 */
omfErr_t omfsPutNthObjIndex(
			omfHdl_t							file,	/* IN - From this file */
			omfObject_t						obj,	/* IN - and this object */
			omfProperty_t					prop,	/* IN - and this property */
			const omfObjIndexElement_t	*data,/* IN - write this data */
			omfInt32							i)		/* IN - at this array index (1-based)*/
{
	omfPosition_t	offset;
	omfInt32			bytesPerObjref;
	omfInt32       zero = 0;
	CMReference    refData;
	char				buf[OMBYTES_PER_OBJREF_1X];
	omfUID_t			localUID;
	omfBool			swab;

	XPROTECT(file)
	{
		swab = ompvtIsForeignByteOrder(file, obj);
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
#if OMFI_ENABLE_SEMCHECK
		XASSERT(i >= 0, OM_ERR_REQUIRED_POSITIVE)
		CHECK(omfsCheckObjRefValidity(file, prop,
												data->Mob, kOmSetFunction));
#endif

		localUID = data->ID;
		if(swab)
		{
			omfsFixLong((omfInt32 *)&localUID.prefix);
			omfsFixLong((omfInt32 *)&localUID.major);
			omfsFixLong((omfInt32 *)&localUID.minor);
		}

		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			bytesPerObjref = OMBYTES_PER_OBJREF_1X;
		else
			bytesPerObjref = sizeof(CMReference);
		CHECK(OMPutNthPropHdr(file, obj, prop, i, OMMobIndex,
									OMBYTES_PER_UID + bytesPerObjref, &offset));
	
		CHECK(OMWriteProp(file, obj, prop, offset, OMMobIndex,
								sizeof(omfInt32), &localUID.prefix));
		CHECK(omfsAddInt32toInt64(sizeof(omfInt32), &offset));
		CHECK(OMWriteProp(file, obj, prop, offset,
								OMMobIndex, sizeof(omfInt32), &localUID.major));
		CHECK(omfsAddInt32toInt64(sizeof(omfInt32), &offset));
		CHECK(OMWriteProp(file, obj, prop, offset,
								OMMobIndex, sizeof(omfInt32), &localUID.minor));
	
		CHECK(omfsAddInt32toInt64(OMBYTES_PER_UID-(2*sizeof(omfInt32)), &offset));
	
		CHECK(GetReferenceData(file, obj, prop, OMMobIndex,
										data->Mob, &refData));
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			memset(buf, 0, OMBYTES_PER_OBJREF_1X);
			memcpy(buf, (char *)refData, sizeof(CMReference));
			CHECK(OMWriteProp(file, obj, prop, offset, OMMobIndex,
									OMBYTES_PER_OBJREF_1X, buf));
		}
		else
			CHECK(OMWriteProp(file, obj, prop, offset, OMMobIndex,
									sizeof(CMReference), refData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND


	return (OM_ERR_NONE);
}

/************************
 * Function: omfsAppendObjIndex
 */
omfErr_t omfsAppendObjIndex(
			omfHdl_t							file,	/* IN - From this file */
			omfObject_t						obj,	/* IN - and this object */
			omfProperty_t					prop,	/* IN - and this property */
			const omfObjIndexElement_t	*data)/* IN - Append this data */
{
	omfInt32		length = 0;
	omfErr_t		status;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		status = omfsGetArrayLength(file, obj, prop, OMMobIndex,
									OMBYTES_PER_UID + OMBYTES_PER_OBJREF_1X, &length);
		if ((status == OM_ERR_NONE) || (status == OM_ERR_PROP_NOT_PRESENT))
			status = omfsPutNthObjIndex(file, obj, prop, data, length + 1);
		CHECK(status);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsLengthObjIndex
 */
omfInt32 omfsLengthObjIndex(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object, return the */
			omfProperty_t	prop)	/* IN - number of elements in this array */
{
	omfInt32           length = 0;

	XPROTECT(file)
	{
		CHECK(omfsGetArrayLength(file, obj, prop, OMMobIndex,
								OMBYTES_PER_UID + OMBYTES_PER_OBJREF_1X, &length));
	}
	XEXCEPT
	{
		return(0);
	}
	XEND

	return (length);
}

/************************
 * Function: omfsReadObjectTag
 *
 * 	Read/Write a object tag from the given property of the given object.
 *
 * Argument Notes:
 *		data - Refers to a pre-existing array.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadObjectTag(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t 	prop,	/* IN = read this property */
			omfTagPtr_t		data)	/* IN = into this array space */
{
	omfPosition_t	field1;
	
	omfsCvtInt32toPosition(0, field1);

	XPROTECT(file)
	{
		CHECK(OMReadProp(file, obj, prop, field1, kNeverSwab, OMObjectTag,
								OMBYTES_PER_OBJECTTAG, data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteObjectTag
 */
omfErr_t omfsWriteObjectTag(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN = write this property */
			const omfTagPtr_t	data)	/* IN - with this value */
{
   omfBool         saveCheckStatus = file->semanticCheckEnable;
	omfErr_t	status;
	omfPosition_t	field1;
	
	omfsCvtInt32toPosition(0, field1);

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		/* There is no object tag to validate yet */
		file->semanticCheckEnable = FALSE;	
		status = OMWriteProp(file, obj, prop, field1, OMObjectTag,
									OMBYTES_PER_OBJECTTAG, data);
		/* Stop recursion in checks */
		file->semanticCheckEnable = saveCheckStatus;
		CHECK(status);
	}
	XEXCEPT
	{
	   file->semanticCheckEnable = saveCheckStatus;
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadObjRef
 * Function:	omfsWriteObjRef
 *
 * 	Read/Write an object reference from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadObjRef(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfObject_t		*data)/* OUT - and return the result here */
{
	CMReference		refData;
	omfPosition_t	field1;
	
	omfsCvtInt32toPosition(0, field1);

	XPROTECT(file)
	{
		CHECK(OMReadProp(file, obj, prop, field1, kNeverSwab, OMObjRef, sizeof(CMReference), 
							  refData));
		CHECK(GetReferencedObject(file, obj, prop, OMObjRef, refData, data));

#if OMFI_ENABLE_SEMCHECK
		CHECK(omfsCheckObjRefValidity(file, prop, *data, kOmGetFunction));
#endif
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteObjRef
 */
omfErr_t omfsWriteObjRef(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN - write this property */
			const omfObject_t	data)	/* IN - with this value */
{
	CMReference     refData;
	omfPosition_t	field1;
	char			buf[OMBYTES_PER_OBJREF_1X];
			
	omfsCvtInt32toPosition(0, field1);
	 
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
#if OMFI_ENABLE_SEMCHECK
		CHECK(omfsCheckObjRefValidity(file, prop, data, kOmSetFunction));
#endif

		CHECK(GetReferenceData(file, obj, prop, OMObjRef, data, &refData));
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			memset(buf, 0, OMBYTES_PER_OBJREF_1X);
			memcpy(buf, (char *)refData, sizeof(CMReference));
			CHECK(OMWriteProp(file, obj, prop, field1, OMObjRef,
									OMBYTES_PER_OBJREF_1X, buf));
		}
		CHECK(OMWriteProp(file, obj, prop, field1, OMObjRef,
								sizeof(CMReference), refData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

 	return (OM_ERR_NONE);
}

/************************
 * Function: omfsGetNthObjRefArray
 * Function:	omfsPutNthObjRefArray
 * Function: omfsAppendObjRefArray
 * Function:	omfsInsertObjRefArray
 * Function: omfsLengthObjRefArray
 *
 * 	Routines to handle objRefArrays, which are used to hold lists of related
 *		objects or mobs.
 *
 * Argument Notes:
 *		i - Index is 1-based.
 *
 * ReturnValue:
 *		For omfsLengthObjRefArray:
 *			The length of the objRefArray property, or 0 on error.
 *		Others:
 *			Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsGetNthObjRefArray(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - and this property */
			omfObject_t		*data,	/* OUT - Get the object reference */
			omfInt32			i)			/* IN - at this index (1-based) */
{
	omfPosition_t	offset;
	omfInt32			bytesPerObjref;
	CMReference     refData;
	
	XPROTECT(file)
	{
#if OMFI_ENABLE_SEMCHECK
		XASSERT(objRefIndexInBounds(file, obj, prop, i), OM_ERR_BADINDEX);
#endif
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			bytesPerObjref = OMBYTES_PER_OBJREF_1X;
		else
			bytesPerObjref = sizeof(CMReference);
		CHECK(OMGetNthPropHdr(file, obj, prop, i, OMObjRefArray,
										bytesPerObjref, &offset));
		CHECK(OMReadProp(file, obj, prop, offset, kNeverSwab,
								OMObjRefArray, sizeof(CMReference), refData));
		CHECK(GetReferencedObject(file, obj, prop, OMObjRefArray,
										refData, data));
	
#if OMFI_ENABLE_SEMCHECK 
		CHECK(omfsCheckObjRefValidity(file, prop, *data, kOmGetFunction));
#endif
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsPutNthObjRefArray
 */
omfErr_t omfsPutNthObjRefArray(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN - and this property */
			const omfObject_t	data,	/* IN - Write the object reference */
			omfInt32					i)	/* IN - at this index (1-based) */
{
	omfPosition_t	offset;
	omfInt32			bytesPerObjref;
	omfInt32			zero = 0;
	CMReference		refData;
	char				buf[OMBYTES_PER_OBJREF_1X];

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
#if OMFI_ENABLE_SEMCHECK
		if (i < 0)
		  {
			 RAISE(OM_ERR_REQUIRED_POSITIVE);
		  }
		else
		  {
			 CHECK(omfsCheckObjRefValidity(file, prop, data, kOmSetFunction));
		  }
#endif
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			bytesPerObjref = OMBYTES_PER_OBJREF_1X;
		else
			bytesPerObjref = sizeof(CMReference);
		CHECK(OMPutNthPropHdr(file, obj, prop, i, OMObjRefArray,
									bytesPerObjref, &offset));
		CHECK(GetReferenceData(file, obj, prop, OMObjRefArray,
									data, &refData));
									
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			memset(buf, 0, OMBYTES_PER_OBJREF_1X);
			memcpy(buf, (char *)refData, sizeof(CMReference));
			CHECK(OMWriteProp(file, obj, prop, offset, OMObjRefArray,
									OMBYTES_PER_OBJREF_1X, buf));
		}
		else
		{
			CHECK(OMWriteProp(file, obj, prop, offset, OMObjRefArray,
									sizeof(CMReference), refData));
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsAppendObjRefArray
 */
omfErr_t omfsAppendObjRefArray(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN - and this property */
			const omfObject_t	data)	/* IN - append this value */
{
	omfInt32          	length = 0;
	omfInt16			bytesPerObjref;
	omfErr_t			status;
	
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			bytesPerObjref = OMBYTES_PER_OBJREF_1X;
		else
			bytesPerObjref = sizeof(CMReference);
		status = omfsGetArrayLength(file, obj, prop, OMObjRefArray, bytesPerObjref, &length);
		if ((status == OM_ERR_NONE) || (status == OM_ERR_PROP_NOT_PRESENT))
			status = omfsPutNthObjRefArray(file, obj, prop, data, length + 1);
		CHECK(status);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * omfsInsertObjRefArray
 */
omfErr_t omfsInsertObjRefArray(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - inset in this property */
			omfObject_t		data,	/* IN - this data */
			omfInt32			pos)	/* IN - at this position */
{
	omfInt32 loop, length = 0;
	omfObject_t tmpData;
	omfBool		exists;
	
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);
		
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		exists = omfsIsPropPresent(file, obj, prop, OMObjRefArray);
		if(exists)
		{
			length = omfsLengthObjRefArray(file, obj, prop);
			/* Else, move items past pos ahead one array element,
			 * and insert pos
			 */
	   	if(pos > length)
		 		RAISE(OM_ERR_BADINDEX);
			for (loop=length; loop==pos; loop--)
				{
				CHECK(omfsGetNthObjRefArray(file, obj, prop, &tmpData, loop));
				CHECK(omfsPutNthObjRefArray(file, obj, prop, tmpData, loop+1));
		      }
			CHECK(omfsPutNthObjRefArray(file, obj, prop, data, pos));
		}
	   else if(pos == 1)
		{
			CHECK(omfsAppendObjRefArray(file, obj, prop, data));
		}
		else
			RAISE(OM_ERR_BADINDEX);
	
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	 return(OM_ERR_NONE);
}

/************************
 * Function: omfsLengthObjRefArray
 */
omfInt32 omfsLengthObjRefArray(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object, return the  */
			omfProperty_t	prop)	/* IN - # of of elements in this array */
{
	omfInt32           length = 0;
	omfInt16			bytesPerObjref;

	XPROTECT(file)
	{
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			bytesPerObjref = OMBYTES_PER_OBJREF_1X;
		else
			bytesPerObjref = sizeof(CMReference);
		CHECK(omfsGetArrayLength(file, obj, prop, OMObjRefArray, bytesPerObjref, &length));
	}
	XEXCEPT
	{
		return(0);
	}
	XEND

	return (length);
}

/************************
 * Function: omfsReadTimeStamp
 * Function:	omfsWriteTimeStamp
 *
 * 	Read/Write a omfTimeStamp_t value from the given property
 *		of the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadTimeStamp(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */
			omfTimeStamp_t	*data)	/* OUT - and return the result here */
{
	omfPosition_t	field1, field2;
	
	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(sizeof(omfInt32), field2);

	XPROTECT(file)
	{
		CHECK(OMReadProp(file, obj, prop, field1, kSwabIfNeeded, OMTimeStamp,
								sizeof(omfInt32), &data->TimeVal));
		ConvertTimeFromCanonical(&data->TimeVal);
		CHECK(OMReadProp(file, obj, prop, field2, kSwabIfNeeded,
								OMTimeStamp, 1, &data->IsGMT));
	
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteTimeStamp
 */
omfErr_t omfsWriteTimeStamp(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfTimeStamp_t	data)	/* IN - with this value */
{
	omfTimeStamp_t ts;
	omfPosition_t	field1, field2;
	
	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(sizeof(omfInt32), field2);

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		ts = data;
		CHECK_DATA_IN(file, &ts);
		ConvertTimeToCanonical(&ts.TimeVal);
		CHECK(OMWriteProp(file, obj, prop, field1, OMTimeStamp,
								sizeof(omfInt32), &ts.TimeVal));
		CHECK(OMWriteProp(file, obj, prop, field2, OMTimeStamp,
								1, &ts.IsGMT));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadUID
 * Function: omfsWriteUID
 *
 * 	Read/Write a UID value from the given property of the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadUID(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfUID_t			*data)/* OUT - and return the result here */
{
	omfPosition_t	field1, field2, field3;
	
	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(sizeof(omfInt32), field2);
	omfsCvtInt32toPosition(2 * sizeof(omfInt32), field3);

	XPROTECT(file)
	{
		CHECK(OMReadProp(file, obj, prop, field1, kSwabIfNeeded, OMUID,
								sizeof(omfInt32), &data->prefix));
		CHECK(OMReadProp(file, obj, prop, field2, kSwabIfNeeded,
								OMUID, sizeof(omfInt32), &data->major));
		CHECK(OMReadProp(file, obj, prop, field3,
							kSwabIfNeeded, OMUID, sizeof(omfInt32), &data->minor));
	
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteUID
 */
omfErr_t omfsWriteUID(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfUID_t			data)	/* IN - with this value */
{
	omfPosition_t	field1, field2, field3;

	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(sizeof(omfInt32), field2);
	omfsCvtInt32toPosition(2 * sizeof(omfInt32), field3);

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK(OMWriteProp(file, obj, prop, field1, OMUID,
								sizeof(omfInt32), &data.prefix));
		CHECK(OMWriteProp(file, obj, prop, field2,
								OMUID, sizeof(omfInt32), &data.major));
		CHECK(OMWriteProp(file, obj, prop, field3,
								OMUID, sizeof(omfInt32), &data.minor));

	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadUsageCodeType/omfsWriteUsageCodeType
 *
 * 	Read/Write a usage code value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadUsageCodeType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfUsageCode_t	*data)/* OUT - and return the result here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMUsageCodeType,
									sizeof(omfInt32), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteUsageCodeType
 */
omfErr_t omfsWriteUsageCodeType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfUsageCode_t	data)	/* IN - with this value */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMUsageCodeType,
										sizeof(omfInt32), &data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadVersionType/omfsWriteVersionType
 *
 * 	Read/Write a version value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadVersionType(
			omfHdl_t				file,		/* IN - From this file */
			omfObject_t			obj,		/* IN - and this object */
			omfProperty_t		prop,		/* IN - read this property */
			omfVersionType_t	*data)	/* OUT - and return the result here */
{
	omfPosition_t	field1, field2;
	
	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(1, field2);

	XPROTECT(file)
	{
		CHECK(OMReadProp(file, obj, prop, field1, kNeverSwab,
								OMVersionType, 1, &data->major));
		CHECK(OMReadProp(file, obj, prop, field2, kNeverSwab,
								OMVersionType, 1, &data->minor));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteVersionType
 */
omfErr_t  omfsWriteVersionType(
			omfHdl_t				file,		/* IN - From this file */
			omfObject_t			obj,		/* IN - and this object */
			omfProperty_t		prop,		/* IN - write this property */
			omfVersionType_t	data)		/* IN - with this value */
{
	omfPosition_t	field1, field2;
	
	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(1, field2);

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK(OMWriteProp(file, obj, prop, field1, OMVersionType,
								1, &data.major));
		CHECK(OMWriteProp(file, obj, prop, field2, OMVersionType,
								1, &data.minor));

	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsObjectDelete
 *
 * 	Deletes an object from the given file.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsObjectDelete(
			omfHdl_t		file,		/* IN - From this file */
			omfObject_t obj)		/* IN - delete this object */
{
	clearBentoErrors(file);
	XPROTECT(file)
	{
		if (obj)
		{
			CMDeleteObject((CMObject) obj);
			XASSERT(!file->BentoErrorRaised, OM_ERR_BENTO_PROBLEM);
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND


	return (OM_ERR_NONE);
}

/************************
 * Function: omfsRemoveNthObjIndex		(1.x ONLY PROPERTY)
 *
 * 	Remove an entry from an ObjIndex, closing up the rest
 *		of the objects around it.
 *
 * Argument Notes:
 *		index - valid values are 1 <= index <= number of objects in
 *			the index.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsRemoveNthObjIndex(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - and this property */
			omfInt32			index)	/* IN - Remove this element index (1-based) */
{
	omfInt32	bytesPerObjref;
	
	XPROTECT(file)
	{
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA)) 
			bytesPerObjref = OMBYTES_PER_OBJREF_1X;
		else 
			bytesPerObjref = sizeof(CMReference);
		CHECK (OMRemoveNthArrayProp(file, obj, prop, OMMobIndex,
									OMBYTES_PER_UID + bytesPerObjref, index));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsRemoveNthObjRefArray
 *
 * 	Removes one element from an ObjRefArray, closing up the array
 *		around it.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsRemoveNthObjRefArray(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - and this property */
			omfInt32			i)			/* IN - Remove this element index (1-based) */
{
	omfInt32	bytesPerObjref;
	
	XPROTECT(file)
	{
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			bytesPerObjref = OMBYTES_PER_OBJREF_1X;
		else
			bytesPerObjref = sizeof(CMReference);
		CHECK (OMRemoveNthArrayProp(file, obj, prop, OMObjRefArray,
												bytesPerObjref, i));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadPosition/omfsWritePosition
 *
 *	Read/Write a position value from the given property of the given obj.
 *      The current implementation of position is an omfUInt32. 
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions, accessor, and Bento errors.  See top of the file.
 */
omfErr_t omfsReadPosition(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */
			omfPosition_t	*data)   /* OUT -- The data returned */
{
	omfUInt32	data32;
	
	XPROTECT(file)
	{
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			CHECK(omfsReadInt32(file, obj,prop, (omfInt32 *)&data32));
			omfsCvtInt32toInt64(data32, data);
		}
		else
		{
			if(omfsIsPropPresent(file, obj, prop, OMPosition64))
			{
				CHECK(OMReadBaseProp(file, obj, prop, OMPosition64, 
								sizeof(omfPosition_t), data));
				CHECK_DATA_OUT(file, data);
			}
			else
			{
				CHECK(OMReadBaseProp(file, obj, prop, OMPosition32, 
								sizeof(data32), &data32));
				CHECK_DATA_OUT(file, &data32);
				omfsCvtInt32toInt64(data32, data);
			}
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

  return (OM_ERR_NONE);
}

/************************
 * Function: omfsWritePosition
 */
omfErr_t omfsWritePosition(
			omfHdl_t			file,	/* IN - Into this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfPosition_t	data)	/* IN -- with this data */
{
	omfInt32	data32;
	
  	XPROTECT(file)
  	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			CHECK(omfsTruncInt64toInt32(data, &data32));	/* OK 1.x */
			CHECK(omfsWriteInt32(file, obj, prop, data32));
		}
		else
		{
		 	if(omfsTruncInt64toInt32(data, &data32) != OM_ERR_NONE)	/* OK HANDLED */
			{
			  	CHECK_DATA_IN(file, &data);
				CHECK (OMWriteBaseProp(file, obj, prop, OMPosition64,
							sizeof(omfPosition_t), &data));
			}
			else
			{
			  	CHECK_DATA_IN(file, &data32);
				CHECK (OMWriteBaseProp(file, obj, prop, OMPosition32,
							sizeof(data32), &data32));
			}
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}


/************************
 * Function: omfsReadLength/omfsWriteLength
 *
 *		Read/write a length value from the given property of
 *		the given object.
 *
 *    The current implementation of length is an omfInt32. 
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions, accessor, and Bento errors.
 *		See top of the file.
 */
omfErr_t omfsReadLength(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfLength_t		*data)/* OUT -- and return the data here */
{
 	omfUInt32	data32;
 	
	XPROTECT(file)
	{
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			CHECK(omfsReadInt32(file, obj,prop, (omfInt32 *)&data32));
			omfsCvtInt32toInt64(data32, data);
		}
		else
		{
			if(omfsIsPropPresent(file, obj, prop, OMLength64))
			{
				CHECK(OMReadBaseProp(file, obj, prop, OMLength64, 
						sizeof(omfLength_t), data));
				CHECK_DATA_OUT(file, data);
			}
			else
			{
				CHECK(OMReadBaseProp(file, obj, prop, OMLength32, 
						sizeof(data32), &data32));
				CHECK_DATA_OUT(file, &data32);
				omfsCvtInt32toInt64(data32, data);
			}
		}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

  return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWriteLength
 */
omfErr_t omfsWriteLength(
			omfHdl_t			file,	/* IN - Into this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfLength_t		data)	/* IN -- with this data */
{
  	omfUInt32	data32;
	CMProperty  cprop;
  	
  	XPROTECT(file)
  	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			CHECK(omfsTruncInt64toUInt32(data, &data32));	/* OK 1.x */
			CHECK(omfsWriteInt32(file, obj, prop, data32));
		}
		else
		{
		 	if(omfsTruncInt64toUInt32(data, &data32) != OM_ERR_NONE)		/* OK HANDLED */
		 	{
			  	CHECK_DATA_IN(file, &data);
				if(omfsIsPropPresent(file, obj, prop, OMLength32))
				{
					cprop = CvtPropertyToBento(file, prop);
					XASSERT((cprop != NULL), OM_ERR_BAD_PROP);
					CMDeleteObjectProperty((CMObject)obj, cprop);
				}
			 	CHECK (OMWriteBaseProp(file, obj, prop, OMLength64,
			 									sizeof(omfLength_t), &data));
		 	}
		 	else
		 	{
			  	CHECK_DATA_IN(file, &data32);
				if(omfsIsPropPresent(file, obj, prop, OMLength64))
				{
					cprop = CvtPropertyToBento(file, prop);
					XASSERT((cprop != NULL), OM_ERR_BAD_PROP);
					CMDeleteObjectProperty((CMObject)obj, cprop);
				}
			 	CHECK (OMWriteBaseProp(file, obj, prop, OMLength32,
			 									sizeof(data32), &data32));
		 	}
	 	}
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}


/************************
 * Function: omfsReadClassID/omfsWriteClassID
 *
 *		Read/write a ClassID value from the given property of
 *		the given object.
 *
 *     A class ID is a 4 character array containing a class identifier.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions, accessor, and Bento errors.
 *		See top of the file.
 */
omfErr_t omfsReadClassID(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t 		obj,	/* IN - and this object */
			omfProperty_t		prop,
			omfClassIDPtr_t	data)	/* OUT - read the classID */
{
	omfPosition_t	field1;
	
	omfsCvtInt32toPosition(0, field1);

	XPROTECT(file)
	{
		/* For 1.0, the class ID is stored as an "objectTag" type */
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			CHECK(OMReadProp(file, obj, prop,field1, kNeverSwab, OMObjectTag, 
										OMBYTES_PER_OBJECTTAG, data));
		}
		
		/* For 2.0, the class ID is stored as a "classID" type */
		else if (file->setrev == kOmfRev2x)
		{
			CHECK(OMReadProp(file, obj, prop, field1, kNeverSwab, 
										OMClassID, OMBYTES_PER_CLASSID, data));
		}
		
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

  return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteClassID
 */
omfErr_t omfsWriteClassID(
			omfHdl_t						file,	/* IN - Into this file */
			omfObject_t					obj, 	/* IN - and this object */
			omfProperty_t				prop,
			const omfClassIDPtr_t		data)	/* IN - write the classID */
{
	omfBool         saveCheckStatus = file->semanticCheckEnable;
	omfPosition_t	field1;
	
	omfsCvtInt32toPosition(0, field1);

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
	
		file->semanticCheckEnable = FALSE;	/* stop recursion in checks */	
	
		/* If 1.x, write an ObjID property of type ObjectTag */
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
		{
			CHECK(OMWriteProp(file, obj, prop, field1, OMObjectTag,
		  								OMBYTES_PER_OBJECTTAG, data));
		}
		/* If 2.x, write a ObjClass property of type ClassID */
		else if (file->setrev == kOmfRev2x)
		{
			CHECK(OMWriteProp(file, obj, prop, field1, OMClassID,
										OMBYTES_PER_CLASSID, data));
		}
		else
			RAISE(OM_ERR_FILEREV_NOT_SUPP);
			
		/* stop recursion in checks */
		file->semanticCheckEnable = saveCheckStatus;
/*		file->semanticCheckEnable = TRUE;	 */
	}
	XEXCEPT
	{
	  /* stop recursion in checks */
/*		file->semanticCheckEnable = TRUE;	 */
	  file->semanticCheckEnable = saveCheckStatus;
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}


/************************
 * Function: omfsReadUniqueName/omfsUniqueName
 *
 *	Read/write a UniqueName value from the given property of the given obj.
 *      Uniquenames are used to name effects and dataKinds.  Uniquenames
 *      are limited to 64 characters.  If a name longer is specified, it
 *      will be truncated.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions, accessor, and Bento errors.
 *		See top of the file.
 */
omfErr_t omfsReadUniqueName(
			omfHdl_t					file,		/* IN - From this file */
			omfObject_t				obj,		/* IN - and this object */
			omfProperty_t			prop,		/* IN - read this property */
			omfUniqueNamePtr_t 	data,		/* IN - and return the data into */
			omfInt32					bufsize)	/* IN - a buffer of this size */
{
	omfLength_t		siz64;
	omfPosition_t	field1;
	omfInt32			siz, maxLen;

	omfsCvtInt32toPosition(0, field1);
	 
	XPROTECT(file)
	{
		maxLen = bufsize - 1;
		CHECK(OMLengthProp(file, obj, prop, OMUniqueName, -1, &siz64));
		CHECK(omfsTruncInt64toInt32(siz64, &siz));		/* OK UNIQUE */
		if (maxLen > siz)
			maxLen = siz;
	  
		CHECK(OMReadProp(file, obj, prop, field1, kNeverSwab, OMUniqueName, 
				    maxLen, data));
	
		/* if the size is the maximum allowed in the read call, and if the
		 * last character is not NULL, then terminate the string.  Note
		 * that the index for the array is zero-based, so the values are
		 * 1 less than the ordinal value.
		 */
		CHECK(OMLengthProp(file, obj, prop, OMUniqueName, -1, &siz64));
		CHECK(omfsTruncInt64toInt32(siz64, &siz));		/* OK UNIQUE */
		if ((siz == (maxLen)) && (data[maxLen - 1] != 0))
		{
			data[maxLen] = 0;
			siz++;
	    }
	
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

  return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteUniqueName
 */
omfErr_t omfsWriteUniqueName(
			omfHdl_t							file,	/* IN - Into this file */
			omfObject_t						obj,	/* IN - and this object */
			omfProperty_t					prop,	/* IN - write this property */
			const omfUniqueNamePtr_t	data)	/* IN - with this data */
{
  	omfPosition_t	field1;
  	
  	omfsCvtInt32toPosition(0, field1);
  	
  	XPROTECT(file)
  	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
	  	CHECK_DATA_IN(file, &data);

		// this is a fix for MC bug that expecting all Strings/UniqueNames to
		// be non-immediate
		if (strlen(data) < 4)
		{
			char tmp[5];

			memset(tmp, 0, 5);
			strcpy(tmp, data);
	  		CHECK (OMWriteProp(file, obj, prop, field1, OMUniqueName, 
					5, tmp));
			
		}
		else
	  		CHECK (OMWriteProp(file, obj, prop, field1, OMUniqueName, 
			      strlen(data) + 1, data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadFadeType/omfsFadeType
 *
 *	Read/write a fadetype value from the given property of the given obj.
 *      Audio source clips may contain fade in and fade out properties 
 *      of type fadetype.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions, accessor, and Bento errors.
 *		See top of the file.
 */
omfErr_t omfsReadFadeType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop, /* IN - read this property */
			omfFadeType_t	*data)/* OUT -- and return the data here */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMFadeType, sizeof(omfInt16), 
					&tmpData));
	
		*data = (omfFadeType_t)tmpData;  
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteFadeType
 */
omfErr_t omfsWriteFadeType(
			omfHdl_t			file,	/* IN - Into this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfFadeType_t	data)	/* IN -- with this data */
{
  omfInt16 tmpData;

 	XPROTECT(file)
 	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
 		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMFadeType, sizeof(omfInt16), 
				  &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}


/************************
 * Function: omfsReadInterpKind/omfsWriteInterpKind
 *
 *	Read/write an interpKind value from the given property of the 
 *      given object.  Interpolations exist on varying values.
 *
 * Argument Notes:
 *	See argument comments and assertions.
 *
 * ReturnValue:
 *	Error code (see below).
 *
 * Possible Errors:
 *	Standard assertions, accessor, and Bento errors.  See top of the file.
 */
omfErr_t omfsReadInterpKind(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj, 	/* IN - and this object */
			omfProperty_t		prop,	/* IN - read this property */
			omfInterpKind_t	*data)/* OUT -- and return the data here */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMInterpKind, sizeof(omfInt16), 
					&tmpData));
	
		*data = (omfInterpKind_t)tmpData;
	  
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteInterpKind
 */
omfErr_t omfsWriteInterpKind(
			omfHdl_t				file,	/* IN - Into this file */
			omfObject_t			obj, 	/* IN - and this object */
			omfProperty_t		prop,	/* IN - write this property */
			omfInterpKind_t	data)	/* IN -- with this data */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMInterpKind, sizeof(omfInt16), 
				  &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadArgIDType/omfsWriteArgIDType
 *
 *	Read/write an ArgIDType value from the given property of the given 
 *      object.  ArgIDs are used to identify arguments to effects.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions, accessor, and Bento errors.
 *		See top of the file.
 */
omfErr_t omfsReadArgIDType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfArgIDType_t	*data)/* OUT - and return the data here */
{
	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMArgIDType,
					sizeof(omfArgIDType_t), data));
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

  return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteArgIDType
 */
omfErr_t omfsWriteArgIDType(
			omfHdl_t			file,	/* IN - Into this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfArgIDType_t	data)	/* IN - with this data */
{
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK (OMWriteBaseProp(file, obj, prop, OMArgIDType,
										sizeof(omfArgIDType_t), &data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsReadEditHint/omfsWriteEditHint
 *
 *		Read/write an editHint value from the given property of the 
 *    given object.  EditHints represent hints about how an editing
 *    application should constrain control points when editing a 
 *    Varying Value.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions, accessor, and Bento errors.
 *		See top of the file.
 */
omfErr_t omfsReadEditHint(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj, 	/* IN - and this object */
			omfProperty_t	prop,	/* IN - read this property */
			omfEditHint_t	*data)/* OUT - and return the data here */
{
  omfInt16 tmpData;

	XPROTECT(file)
	{
	  	CHECK(OMReadBaseProp(file, obj, prop, OMEditHintType, 
					sizeof(omfInt16), &tmpData));
	
	  	*data = (omfEditHint_t)tmpData;  
	  	CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

  return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteEditHint
 */
omfErr_t omfsWriteEditHint(
			omfHdl_t			file,	/* IN - Into this file */
			omfObject_t		obj, 	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */
			omfEditHint_t	data)	/* IN - with this data */
{
  omfInt16 tmpData;

  	XPROTECT(file)
  	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
	  	CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMEditHintType,sizeof(omfInt16), 
				  &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}


/************************
 * Function: omfsReadDataValue/omfsWriteDataValue/omfsLengthDataValue
 *
 *		Read/write a dataValue value from the given property of the 
 *    given object.  A dataValue is an arbitrary sized array of
 *    bytes containing a data value to be used in an effect, or
 *    output from a composition.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		For omfsLengthDataValue:
 *			The length of the dataValue property, or 0 on error.
 *		Others:
 *			Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions, accessor, and Bento errors.
 *		See top of the file.
 */
omfErr_t omfsReadDataValue(
			omfHdl_t			file,			/* IN - From this file */
			omfObject_t		obj, 			/* IN - and this object */
			omfProperty_t	prop, 		/* IN - read this property */
			omfDDefObj_t   datakind,   /* IN - datakind object */
			omfDataValue_t	data,			/* IN -- into a buffer. */
			omfPosition_t	offset,		/* IN - Read from this offset */
			omfUInt32		maxsize,		/* IN - into a buffer of this size */
			omfUInt32		*bytesRead)	/* OUT - and return the bytes read */
{
	omfErr_t			lenStat, omfError = OM_ERR_NONE;
	omfUniqueName_t name;
	omfRational_t rat;
	omfLength_t		bytes;
	omfBool			swab;
	
	XPROTECT(file)
	{
	  /* If numeric types, swap the bytes if necessary */
	  CHECK(omfsReadUniqueName(file, datakind, OMDDEFDatakindID,
										name, OMUNIQUENAME_SIZE));
	swab = ompvtIsForeignByteOrder(file, obj);
	  if (!strcmp(name, INT16KIND) ||
			!strcmp(name, UINT16KIND))
		 {
			omfError = OMReadProp(file, obj, prop, offset, kNeverSwab, 
										 OMDataValue, maxsize, data);
			if(swab)
				omfsFixShort((omfInt16 *)data);
		 }
	  else if (!strcmp(name, INT32KIND) ||
			!strcmp(name, UINT32KIND))
		 {
			omfError = OMReadProp(file, obj, prop, offset, kNeverSwab, 
										 OMDataValue, maxsize, data);
			if(swab)
				omfsFixLong((omfInt32 *)data);
		 }
	  else if (!strcmp(name, RATIONALKIND))
		 {
			if (maxsize < (sizeof(omfInt32)*2))
			  {
				 RAISE(OM_ERR_INTERN_TOO_SMALL);
			  }
			omfError = OMReadProp(file, obj, prop, offset, kNeverSwab, 
										 OMDataValue, sizeof(omfInt32), &rat.numerator);
			omfsAddInt32toInt64(sizeof(omfInt32), &offset);
			omfError = OMReadProp(file, obj, prop, offset, kNeverSwab, 
										 OMDataValue, sizeof(omfInt32),&rat.denominator);
			if(swab)
			{
				omfsFixLong(&rat.numerator);
				omfsFixLong(&rat.denominator);
			}
			memcpy(data, &rat, sizeof(omfInt32)*2);
		 }
	  else
		 omfError = OMReadProp(file, obj, prop, offset, kNeverSwab, 
									  OMDataValue, maxsize, data);

	  if (omfError == OM_ERR_NONE)
		 *bytesRead = maxsize;
	  else
	    {
			lenStat = OMLengthProp(file, obj, prop, OMDataValue, 0, &bytes);
			if (lenStat == OM_ERR_NONE)
			{
			  omfsSubInt64fromInt64(offset, &bytes);
			  CHECK(omfsTruncInt64toUInt32(bytes, bytesRead));		/* OK MAXREAD */
			}
			else
			  *bytesRead = 0;
	    }
	
		if (*bytesRead != maxsize)
		  {
			 RAISE(OM_ERR_END_OF_DATA);
		  }
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

  
  	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteDataValue
 */
omfErr_t omfsWriteDataValue(
			omfHdl_t					file,		/* IN - Into this file */
			omfObject_t				obj, 		/* IN - and this object */
			omfProperty_t			prop, 	/* IN - write this property */
			omfDDefObj_t 		  datakind,  		 /* IN - datakind object */
			omfDataValue_t 		data,		/* IN - with this data */
			omfPosition_t			offset,	/* IN - at this byte offset */
			omfUInt32				count)	/* IN - for this many bytes */
{
	omfInt32			local32;
 	omfInt16			local16;
	omfRational_t	localRat;
	omfUniqueName_t name;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
	  	/* If numeric types, swap the bytes if necessary */
	  	CHECK(omfsReadUniqueName(file, datakind, OMDDEFDatakindID,
										name, OMUNIQUENAME_SIZE));
	  	if (!strcmp(name, INT16KIND) ||
			!strcmp(name, UINT16KIND))
		 {
			if(file->byteOrder != OMNativeByteOrder)
			{
		 				local16 = *((omfInt16 *)data);
				omfsFixShort(&local16);
				data = &local16;
			}
		 }
	  	else if (!strcmp(name, INT32KIND) ||
			!strcmp(name, UINT32KIND))
		 {
			if(file->byteOrder != OMNativeByteOrder)
			{
				local32 = *((omfInt32 *)data);
				omfsFixLong(&local32);
				data = &local32;
			}
		 }
	 	 else if (!strcmp(name, RATIONALKIND))
		 {
			if(file->byteOrder != OMNativeByteOrder)
			{
		 		localRat= *((omfRational_t *)data);
		 		omfsFixLong(&localRat.numerator);
		 		omfsFixLong(&localRat.denominator);
		 		data = &localRat;
		 	}
		 }
		CHECK (OMWriteProp(file, obj, prop, offset,
									OMDataValue, count, data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsLengthDataValue
 */
omfLength_t omfsLengthDataValue(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object, return the */
			omfProperty_t	prop) /* IN - length of this property. */
{
	omfLength_t result, zero = omfsCvtInt32toLength(0, zero);
	
	XPROTECT(file)
	{
		CHECK(OMLengthProp(file, obj, prop, OMDataValue, 0, &result));
	}
	XEXCEPT
	XEND_SPECIAL(zero)

	return (result);
}

/************************
 * Function: omfsRemovePropData - removes data from a datavalue object (such as trimming an IDAT)
 */
omfErr_t omfsRemovePropData(
			omfHdl_t					file,		/* IN - From this file */
			omfObject_t				obj, 		/* IN - and this object */
			omfProperty_t			prop, 	/* IN - and this property */
			omfPosition_t			offset,	/* IN - at this byte offset */
			omfLength_t				length)	/* IN - for this many bytes */
{
	omfLength_t		maxLength;

	XPROTECT(file)
	{
		maxLength	= omfsLengthDataValue( file, obj, prop );
		if( omfsInt64Greater( offset, maxLength ) ){
			RAISE( OM_ERR_END_OF_DATA )
			}
		
		omfsSubInt64fromInt64( offset, &maxLength );
		if( omfsInt64Less( maxLength, length ) ){
			length	= maxLength;
			}

		CHECK (OMRemovePropData(file, obj, prop, OMDataValue, offset, length ) );
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}
/************************
 * Function: omfsReadLayoutType/omfsWriteDropType
 *
 *		Read/write a LayoutType value from the given property of the 
 *    given object.  LayoutType identifies whether video includes
 *		FULL_FRAME, SEPARATE_FIELDS, SINGLE_FIELD, or MIXED_FIELDS.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions, accessor, and Bento errors.
 *		See top of the file.
 */
omfErr_t omfsReadLayoutType(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj, 	/* IN - and this object */
			omfProperty_t		prop,	/* IN - read this property */
			omfFrameLayout_t	*data)/* OUT - and return the data here */
{
  omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMLayoutType, 
					sizeof(omfInt16), &tmpData));
	
		switch (tmpData)
		{
			case OMFI_LAYOUT_FULL:
				*data = kFullFrame;
				break;
		
			case OMFI_LAYOUT_SINGLE:
				*data = kOneField;
				break;
		
			case OMFI_LAYOUT_SEPARATE:
				*data = kSeparateFields;
				break;
		
			case OMFI_LAYOUT_MIXED:
				*data = kMixedFields;
				break;
			
			default:
				RAISE(OM_ERR_BADLAYOUT);
		
		} /* end switch */

		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

  return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteLayoutType
 */
omfErr_t omfsWriteLayoutType(
			omfHdl_t				file,	/* IN - Into this file */
			omfObject_t			obj, 	/* IN - and this object */
			omfProperty_t		prop,	/* IN - write this property */
			omfFrameLayout_t	data)	/* IN - with this data */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		switch (data)
		{
			case kFullFrame:
				tmpData = OMFI_LAYOUT_FULL;
				break;
		
			case kOneField:
				tmpData = OMFI_LAYOUT_SINGLE;
				break;
		
			case kSeparateFields:
				tmpData = OMFI_LAYOUT_SEPARATE;
				break;
		
			case kMixedFields:
				tmpData = OMFI_LAYOUT_MIXED;
				break;
		
			default:
				RAISE(OM_ERR_BADLAYOUT);
		
		} /* end switch */
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMLayoutType,sizeof(omfInt16), 
				  &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsGetNthAttribute
 * Function: omfsAppendAttribute
 * Function: omfsNumAttributes
 *
 * 	Routines to handle objRefArrays, which are used to hold lists of related
 *		objects or mobs.
 *
 * Argument Notes:
 *		i - Index is 1-based.
 *
 * ReturnValue:
 *		For omfsLengthObjRefArray:
 *			The length of the objRefArray property, or 0 on error.
 *		Others:
 *			Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsGetNthAttribute(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - and this property */
			omfObject_t		*data,	/* OUT - Get the object reference */
			omfInt32			i)			/* IN - at this index (1-based) */
{
	omfObject_t	attrList;
	
	XPROTECT(file)
	{
		CHECK(omfsReadObjRef(file, obj, prop, &attrList));
		CHECK(omfsGetNthObjRefArray(file, attrList, OMATTRAttrRefs, data, i));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsAppendAttribute
 */
omfErr_t omfsAppendAttribute(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t			obj,	/* IN - and this object */
			omfProperty_t		prop,	/* IN - and this property */
			const omfObject_t	data)	/* IN - append this value */
{
	omfObject_t	attrList;
	
	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		if(omfsIsPropPresent(file, obj, prop, OMObjRef))
		{
			CHECK(omfsReadObjRef(file, obj, prop, &attrList));
		}
		else
		{
			CHECK(omfsObjectNew(file, "ATTR", &attrList));
			CHECK(omfsWriteObjRef(file, obj, prop, attrList));
		}
		CHECK(omfsAppendObjRefArray(file, attrList, OMATTRAttrRefs, data));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsNumAttributes
 */
omfInt32 omfsNumAttributes(
			omfHdl_t		file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object, return the  */
			omfProperty_t	prop)	/* IN - # of of elements in this array */
{
	omfInt32	length = 0;
	omfInt16	bytesPerObjref;
	omfObject_t	attrList;
			
	XPROTECT(file)
	{
		if(omfsIsPropPresent(file, obj, prop, OMObjRef))
		{
			CHECK(omfsReadObjRef(file, obj, prop, &attrList));
			if((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
				bytesPerObjref = OMBYTES_PER_OBJREF_1X;
			else
				bytesPerObjref = sizeof(CMReference);
			CHECK(omfsGetArrayLength(file, attrList, OMATTRAttrRefs,
									OMObjRefArray, bytesPerObjref, &length));
		}
	}
	XEXCEPT
	{
		return(0);
	}
	XEND

	return (length);
}

/************************
 * Function: omfsReadProductVersionType
 * Function: omfsWriteProductVersionType
 *
 * 	Read/Write a omfClientAppVersion_t value from the given property of the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadProductVersionType(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t				obj,	/* IN - and this object */
			omfProperty_t			prop,	/* IN - read this property */
			omfProductVersion_t	*data)/* OUT - and return the result here */
{
	omfPosition_t	field1, field2, field3, field4, field5;
	
	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(sizeof(omfUInt16), field2);
	omfsCvtInt32toPosition(2 * sizeof(omfUInt16), field3);
	omfsCvtInt32toPosition(3 * sizeof(omfUInt16), field4);
	omfsCvtInt32toPosition(4 * sizeof(omfUInt16), field5);

	XPROTECT(file)
	{
		CHECK(OMReadProp(file, obj, prop, field1, kSwabIfNeeded, OMProductVersion,
								sizeof(omfUInt16), &data->major));
		CHECK(OMReadProp(file, obj, prop, field2, kSwabIfNeeded, OMProductVersion,
								sizeof(omfUInt16), &data->minor));
		CHECK(OMReadProp(file, obj, prop, field3, kSwabIfNeeded, OMProductVersion,
								sizeof(omfUInt16), &data->tertiary));
		CHECK(OMReadProp(file, obj, prop, field4, kSwabIfNeeded, OMProductVersion,
								sizeof(omfUInt16), &data->type));
		CHECK(OMReadProp(file, obj, prop, field5, kSwabIfNeeded, OMProductVersion,
								sizeof(omfUInt16), &data->patchLevel));
	
		CHECK_DATA_OUT(file, data);
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsWriteProductVersionType
 */
omfErr_t omfsWriteProductVersionType(
			omfHdl_t				file,	/* IN - From this file */
			omfObject_t				obj,	/* IN - and this object */
			omfProperty_t			prop,	/* IN - write this property */
			omfProductVersion_t		data)	/* IN - with this value */
{
	omfPosition_t	field1, field2, field3, field4, field5;
	
	omfsCvtInt32toPosition(0, field1);
	omfsCvtInt32toPosition(sizeof(omfUInt16), field2);
	omfsCvtInt32toPosition(2 * sizeof(omfUInt16), field3);
	omfsCvtInt32toPosition(3 * sizeof(omfUInt16), field4);
	omfsCvtInt32toPosition(4 * sizeof(omfUInt16), field5);

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		CHECK(OMWriteProp(file, obj, prop, field1, OMProductVersion,
								sizeof(omfUInt16), &data.major));
		CHECK(OMWriteProp(file, obj, prop, field2, OMProductVersion,
								sizeof(omfUInt16), &data.minor));
		CHECK(OMWriteProp(file, obj, prop, field3, OMProductVersion,
								sizeof(omfUInt16), &data.tertiary));
		CHECK(OMWriteProp(file, obj, prop, field4, OMProductVersion,
								sizeof(omfUInt16), &data.type));
		CHECK(OMWriteProp(file, obj, prop, field5, OMProductVersion,
								sizeof(omfUInt16), &data.patchLevel));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsReadPulldownKindType/omfsWritePulldownKindType
 *
 * 	Read/Write a omfPulldownKind_t value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadPulldownKindType(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */ 
			omfPulldownKind_t	*data)/* OUT - and return the result here */
{
  	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMPulldownKindType, sizeof(omfInt16),
									&tmpData));
	
		*data = (omfPulldownKind_t)tmpData;
		CHECK_DATA_OUT(file, data);  
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWritePulldownKindType
 */
omfErr_t omfsWritePulldownKindType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */ 
			omfPulldownKind_t	data)	/* IN - with this value */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMPulldownKindType,
									sizeof(omfInt16), &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsReadPhaseFrameType/omfsWritePhaseFrameType
 *
 * 	Read/Write a omfPhaseFrame_t value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadPhaseFrameType(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */ 
			omfPhaseFrame_t	*data)/* OUT - and return the result here */
{
  	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMPhaseFrameType, sizeof(omfInt16),
									&tmpData));
	
		*data = (omfPhaseFrame_t)tmpData;
		CHECK_DATA_OUT(file, data);  
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWritePhaseFrameType
 */
omfErr_t omfsWritePhaseFrameType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */ 
			omfPhaseFrame_t	data)	/* IN - with this value */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMPhaseFrameType,
									sizeof(omfInt16), &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsReadPulldownDirectionType/omfsWritePulldownDirectionType
 *
 * 	Read/Write a omfPulldownKind_t value from the given property of
 *		the given object.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsReadPulldownDirectionType(
			omfHdl_t			file,		/* IN - From this file */
			omfObject_t		obj,		/* IN - and this object */
			omfProperty_t	prop,		/* IN - read this property */ 
			omfPulldownDir_t	*data)/* OUT - and return the result here */
{
  	omfInt16 tmpData;

	XPROTECT(file)
	{
		CHECK(OMReadBaseProp(file, obj, prop, OMPulldownDirectionType, sizeof(omfInt16),
									&tmpData));
	
		*data = (omfPulldownDir_t)tmpData;
		CHECK_DATA_OUT(file, data);  
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 *  Function: omfsWritePulldownDirectionType
 */
omfErr_t omfsWritePulldownDirectionType(
			omfHdl_t			file,	/* IN - From this file */
			omfObject_t		obj,	/* IN - and this object */
			omfProperty_t	prop,	/* IN - write this property */ 
			omfPulldownDir_t	data)	/* IN - with this value */
{
	omfInt16 tmpData;

	XPROTECT(file)
	{
		XASSERT(OKToWriteProperties(file), OM_ERR_DATA_NONCONTIG);
		CHECK_DATA_IN(file, &data);
		tmpData = (omfInt16)data;
	
		CHECK (OMWriteBaseProp(file, obj, prop, OMPulldownDirectionType,
									sizeof(omfInt16), &tmpData));
	}
	XEXCEPT
	{
		return(XCODE());
	}
	XEND

	return (OM_ERR_NONE);
}


/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/

