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
 * Function: The main API file for media layer operations.
 *
 * Audience: Clients writing or reading OMFI media data.
 *
 * Public Functions:
 *		omfsMalloc()
 *			Allocate this many bytes
 *		omfsFree()
 *			Free up this buffer
 *		omfsIsPropPresent()
 *			Test if a given property is present in a file.
 *		omfsIsTypeOf()
 *			Test if an object is a member of the given class, or
 *			one of its subclasses.
 *		omfsClassFindSuperClass()
 *			Given a pointer to a classID, returns the superclass ID
 *			and a boolean indicating if a superclass was found.
 *		omfsGetByteOrder()
 *			Return the byte ordering of a particular object in the file. 
 *		omfsPutByteOrder()
 *			Sets the byte ordering of a particular object in the file.
 *		omfsFileGetDefaultByteOrder()
 *			Returns the default byte ordering from the given file.
 *		omfsFileSetDefaultByteOrder()
 *			Sets the default byte ordering from the given file.
 *		omfsGetHeadObject()
 *			Given an opaque file handle, return the head object for
 *			that file.
 *		omfsObjectNew()
 *			Create a new object of a given classID in the given file.
 *		omfsSetProgressCallback()
 *			Sets a callback which will be called at intervals
 *			during int  operations, in order to allow your application
 *			to pin a watch cursor, move a thermometer, etc...
 *    omfiGetNextProperty()
 *    omfiGetPropertyName()
 *    omfiGetPropertyTypename()
 *    omfiGetNextObject()
 *
 * Public functions used to extend the toolkit API:
 *	(Used to write wrapper functions for new types and properties)
 *
 *		omfsNewClass()
 *			Add a new class to the class definition table.
 *		omfsRegisterType()
 *			Add a new type to the type definition table.  This function takes an explicit
 *			omType_t, and should be called for required & registered types
 *			which are not registered by a media codec.
 *		omfsRegisterProp()
 *			Register a omfProperty code, supplying a name, class, type, and an enum
 *			value indicating which revisions the field is valid for.  This function takes
 *			an explicit omfProperty_t, and should be called for required & registered
 *			properties which are not registered by a media codec.
 *		omfsRegisterDynamicType()
 *			Register a type with a dynamic type code.  This function should be called when
 *			registering a private type, or when a codec requires a type.  The type code used
 *			to refer to the type is returned from this function, and must be saved in a variable
 *			in the client application, or in the persistant data block of a codec.
 *		omfsRegisterDynamicProp()
 *			Register a type with a dynamic property code.  This function should be called when
 *			registering a private property, or when a codec requires a property.  The type code used
 *			to refer to the property is returned from this function, and must be saved in a variable
 *			in the client application, or in the persistant data block of a codec.
 *		OMReadProp()
 *			Internal function to read the given property from the file, and apply
 *			semantic error checking to the input parameters and the result.
 *		OMWriteProp()
 *			Internal function to write the given property to the file, and apply
 *			semantic error checking to the input parameters.
 *		OMIsPropertyContiguous()
 *			Tell if a property is contigous in the file.   This function
 *			is usually applied to media data, when the application wishes
 *			to read the data directly, without the toolkit.
 *		omfsGetArrayLength()
 *			Get the number of elements in an arrayed OMFI object.
 *		OMPutNthPropHdr()
 *			Set an indexed elements value in an arrayed OMFI object.
 *		OMGetNthPropHdr()
 *			Get an indexed elements value from an arrayed OMFI object.
 *		OMReadBaseProp()
 *			Version of OMReadProp which byte-swaps when needed and always reads
 *			from an offset of 0.
 *		OMWriteBaseProp()
 *			Version of OMWriteProp which always reads from an offset of 0.
 *		OMLengthProp()
 *			Internal function to find the length of the given property.
 *		GetReferencedObject()
 *		GetReferenceData()
 *		OMRemoveNthArrayProp()
 *		omfsFileGetValueAlignment()
 *		omfsFileSetValueAlignment()
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if PORT_SYS_MAC
#include <memory.h>		/* For omfsMalloc() and omfsFree() */
#if !LW_MAC
#include <CarbonCore/OSUtils.h>
#endif
#else
#if PORT_INC_NEEDS_SYSTIME
#include <sys/time.h>
#endif
#endif
#if PORT_INC_NEEDS_TIMEH
#include <time.h>
#endif

#include "omPublic.h"
#include "omPvt.h" 
#include "omCntPvt.h"
#include "omTypes.h"

/* Moved math.h down here to make NEXT's compiler happy */
#include <math.h>


#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

static const omfInt32 zeroes[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static omfBool  InitCalled = FALSE;

#if PORT_LANG_CPLUSPLUS
extern          "C"
{
#endif
const omfUInt16    OMNativeByteOrder = NATIVE_ORDER;

const omfProductVersion_t omfiToolkitVersion = {2, 2, 0, 0, kVersionReleased};
#if PORT_LANG_CPLUSPLUS
}
#endif


static omfInt32 powi(
			omfInt32	base,
			omfInt32	exponent);

	/************************************************************
	 *
	 * Public Functions (Part of the toolkit API)
	 *
	 *************************************************************/


/************************
 * Function: omfsMalloc
 *
 * 		Allocates a block of memory of a given size.  Having this
 *		function separate from ANSI malloc works better on the
 *		Mac, and allows for memory leak tracking.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Returns a pointer to a block of memory on the heap, or
 *		NULL if there is no block of that size available.
 *
 * Possible Errors:
 *		NULL -- No memory available.
 */
void *omfsMalloc(
			size_t size)	/* Allocate this many bytes */
{
	return(omOptMalloc(NULL, size));
/*
#if PORT_SYS_MAC
	return (NewPtr(size));
#else
#if XPDEBUG
	if (size == 140)
	  {
		printf("Size == 140\n");
	  }
#endif
	return ((void *) malloc((size_t) size));
#endif
*/
}

/************************
 * Function: omfsFree
 *
 * 	Frees a given block of memory allocated by omfsMalloc. Having this
 *		function separate from ANSI free() works better on the Mac, and
 *		allows for memory leak tracking.
 *
 * Argument Notes:
 *		Make sure that the pointer given was really allocated
 *		by omfsMalloc, just as for ANSI malloc.
 *
 * ReturnValue:
 *		<none>
 *
 * Possible Errors:
 *		<none known>
 */
void omfsFree(
			void *ptr)	/* Free up this buffer */
{
	omOptFree(NULL, ptr);
/*
#if PORT_SYS_MAC
	DisposPtr((Ptr) ptr);
#else
	free((void *) ptr);
#endif
*/
}

/************************
 * Function: omfsIsPropPresent
 *
 * 	Test if a given property is present in a file without
 *		generating an error.
 *
 * Argument Notes:
 *		Note that this function takes an omfType_t.  You can find the
 *		type for a given property in the omFile.c.  Look the property
 *		init calls in BeginSession().
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfBool omfsIsPropPresent(
			omfHdl_t			file,		/* IN -- For this omf file */
			omfObject_t		obj,		/* IN -- in this object */
			omfProperty_t	prop,		/* IN -- read this property */
			omfType_t		dataType)/* IN -- check the type */
{
	CMProperty      cprop;
	CMType          ctype;

	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);

	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	if((cprop != NULL) && (ctype != NULL))
	{
		return (CMCountValues((CMObject) obj, cprop, ctype) != 0);
	}
	else
	{
		return FALSE;
	}
}



/************************
 * Function: omfsGetByteOrder
 *
 * 	Return the byte ordering of a particular object in the file.  Returns
 *		the field from the object if present, or the default for the file
 *		if not present.
 *
 * Argument Notes:
 *		obj - As of rev 2.0 of the spec, this call is only allowed for media
 *				data objects.  All other objects should be written in native order.
 *		data - Will be either 0x4d4d (Big-endian order) or 0x4949 (Little-endian
 *				order.  This can be compared against the constant OMNativeByteOrder
 *				to see if byte-swapping will be required.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions and Bento errors.  See top of the file.
 *		Bad property and type are asserted, but off of internal constant values.
 *		OM_ERR_NOBYTEORDER -- No byte order value is present in the file.
 */
omfErr_t omfsGetByteOrder(
			omfHdl_t 	file,		/* IN -- For this omf file */
			omfObject_t obj,		/* IN -- in this object */
			omfInt16		*data)	/* OUT -- return the byte order. */
{
	CMValue         val;
	CMSize32          siz;
	CMProperty      cprop;
	CMType          ctype;
	omfInt16           defValue;
	omfInt64			zero;
	
	omfsCvtInt32toInt64(0, &zero);
	clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);

	if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
	  cprop = CvtPropertyToBento(file, OMByteOrder);
	else
	  cprop = CvtPropertyToBento(file, OMHEADByteOrder);

	ctype = CvtTypeToBento(file, OMInt16, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);
	omfAssert((data != NULL), file, OM_ERR_BADDATAADDRESS);

	/* Special processing here because of possibility of default. */
	/* Do not use WP routines. */

	XPROTECT(file)
	{
		if (CMCountValues((CMObject) obj, cprop, ctype))
		{
			val = CMUseValue((CMObject) obj, cprop, ctype);
	
			siz = CMReadValueData(val, (CMPtr) data, zero, sizeof(omfInt16));
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
	
			if (omfsGetBentoID(file, obj) == 1)	/* HEAD object */
			{
				CHECK(omfsFileSetDefaultByteOrder(file, *data));
			}
			else
				RAISE(OM_ERR_NOBYTEORDER);
		} else
		{
			CHECK(omfsFileGetDefaultByteOrder(file, &defValue));
			if (defValue != 0)
			{
				*data = defValue;
			}
		else
			RAISE(OM_ERR_NOBYTEORDER);
		}
	
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: ompvtIsForeignByteOrder
 *
 * 	Return TRUE if the given object has a non-native byte order.  This routine
 *		is optimized in the common case (objects byte order == file byte order).
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Return TRUE if the given object has a non-native byte order, FALSE otherwise.
 *
 * Possible Errors:
 */
omfBool ompvtIsForeignByteOrder(
			omfHdl_t 	file,		/* IN -- For this omf file */
			omfObject_t obj)		/* IN -- is this object foreign byte order? */
{
	omfInt16		objByteOrder;
	omfBool		result;
	
	if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
	{
		omfAssert((file->byteOrderProp != NULL), file, OM_ERR_BAD_PROP);
		omfAssert((file->byteOrderType != NULL), file, OM_ERR_BAD_TYPE);
		if (CMCountValues((CMObject) obj, file->byteOrderProp, file->byteOrderType))
		{
			if(omfsGetByteOrder(file, obj, &objByteOrder) == OM_ERR_NONE)
				result = (objByteOrder != OMNativeByteOrder);
			else
				result = (file->byteOrder != OMNativeByteOrder);
		}
		else
			result = (file->byteOrder != OMNativeByteOrder);
	}
	else
		result = (file->byteOrder != OMNativeByteOrder);
		
	return(result);
}

/************************
 * Function: omfsPutByteOrder
 *
 * 	Sets the byte ordering of a particular object in the file.  Checks
 *		to make sure that the byteorder value is valid for the object.
 *
 * Argument Notes:
 *		obj - As of rev 2.0 of the spec, this call is only allowed for media
 *				data objects.  All other objects should be written in native order.
 *		data - Will be either 0x4d4d (Big-endian order) or 0x4949 (Little-endian
 *				order.  This can be set from the constant OMNativeByteOrder
 *				if the data is being written in native order.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions and Bento errors.  See top of the file.
 *		Bad property and type are asserted, but off of internal constant values.
 *
 *		OM_ERR_INVALID_BYTEORDER -- Byteorder is not a valid vaue, or an audio
 *		data object is assigned an invalid value.
 */
omfErr_t omfsPutByteOrder(
			omfHdl_t 	file,	/* IN -- For this omf file */
			omfObject_t obj,	/* IN -- in this object */
		   omfInt16		data)	/* OUT -- set the byte order to "data". */
{

#if OMFI_ENABLE_SEMCHECK
	omfClassID_t  objID;
	omfProperty_t	idProp;
#endif

	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	XPROTECT(file)
	{
#if OMFI_ENABLE_SEMCHECK
		if ((data != MOTOROLA_ORDER) && (data != INTEL_ORDER))
			RAISE(OM_ERR_INVALID_BYTEORDER);
	
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		CHECK(omfsReadClassID(file, obj, idProp, (omfClassIDPtr_t) &objID));
		if ((streq(objID, "AIFC") && data != MOTOROLA_ORDER) ||
		    (streq(objID, "WAVE") && data != INTEL_ORDER))
			RAISE(OM_ERR_INVALID_BYTEORDER);
#endif

		if (omfsGetBentoID(file, obj) == 1)	/* HEAD object */
			omfsFileSetDefaultByteOrder(file, data);

		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		  {
			CHECK(omfsWriteInt16(file, obj, OMByteOrder, data));
		  }
		else /* kOmfRev2x */
		  {
			CHECK(omfsWriteInt16(file, obj, OMHEADByteOrder, data));
		  }
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfsFileGetDefaultByteOrder
 *
 * 	Returns the default byte ordering from the given file.
 *
 * Argument Notes:
 *		order - Will be either 0x4d4d (Big-endian order) or 0x4949 (Little-endian
 *				order.  This can be compared against the constant OMNativeByteOrder
 *				to see if byte-swapping will be required.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions.  See top of the file.
 *		OM_ERR_BADCONTAINER -- The file did not have a valid container.
 */
omfErr_t omfsFileGetDefaultByteOrder(
			omfHdl_t	file,		/* IN -- For this omf file */
			omfInt16	*order)	/* OUT -- return the default byte order. */
{
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((file->container != NULL), file, OM_ERR_BADCONTAINER);
	*order = ((ContainerPtr) file->container)->defaultByteOrder;

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsFileSetDefaultByteOrder
 *
 * 	Sets the default byte ordering from the given file.
 *
 * Argument Notes:
 *		order - Will be either 0x4d4d (Big-endian order) or 0x4949 (Little-endian
 *				order.  This can be set from the constant OMNativeByteOrder
 *				if the data is being written in native order.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions.  See top of the file.
 *		OM_ERR_BADCONTAINER -- The file did not have a valid container.
 */
omfErr_t omfsFileSetDefaultByteOrder(
			omfHdl_t file,		/* IN -- For this omf file */
			omfInt16 order)	/* OUT -- set the default byte order. */
{
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((file->container != NULL), file, OM_ERR_BADCONTAINER);
	((ContainerPtr) file->container)->defaultByteOrder = order;

	return (OM_ERR_NONE);
}

/************************
 * Function: RationalFromFloat (INTERNAL)
 *
 * 	Translate a floating point number into the nearest rational
 *		number.  Used when rates must be stored as floats.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		A rational number
 *
 * Possible Errors:
 */
omfRational_t RationalFromFloat(
			double	f)		/* IN - Convert this number into a rational */
{
	/* chintzy conversion of old code  */
	omfRational_t   rate;
	double          rem;
	omfInt32           i;

	/* special check for NTSC video */
	if (fabs(f - 29.97) < .01)
	{
		rate.numerator = 2997;
		rate.denominator = 100;
		return (rate);
	}
	/* need to normalize to make compare work */
	rem = f - floor(f);
	for (i = 0; (i < 4) && (rem > .001); i++)
	{
		f = f * 10;
		rem = f - floor(f);
	}
	rate.numerator = (omfInt32) f;
	rate.denominator = powi(10L, i);
	return (rate);
}

/************************
 * Function: FloatFromRational (INTERNAL)
 *
 * 	Convert a rational number into a floating point double
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		The result, or zero if the denominator was zero.
 *
 * Possible Errors:
 *		Returns zero if the denominator was zero.
 */
double FloatFromRational(
			omfRational_t	e)		/* IN - Convert this into a double */
{
	double          num = (double) e.numerator;
	double          den = (double) e.denominator;

	if(den == 0)
		return(0);
	else
		return (num / den);
}

/************************
 * Function: omfsGetHeadObject
 *
 * 	Given an opaque file handle, return the head object for
 *		that file.  This function is useful to retrieve properties
 *		which exist only on the head object.
 *
 * Argument Notes:
 *		Returns a pointer to the head object through head.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsGetHeadObject(
			omfHdl_t		file,		/* IN -- For this omf file */
			omfObject_t	*head)	/* OUT -- return the head object */
{
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	*head = file->head;
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsObjectNew
 *
 * 		Create a new object of a given classID in the given file.
 *
 * Argument Notes:
 *		The classID must either be a required class, or have been
 *		registered with omfsNewClass().
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_INVALID_CLASS_ID -- The classID was not valid
 *		OM_ERR_BAD_VIRTUAL_CREATE -- The given classID is an
 *			abstract superclass, which is not supposed to have
 *			instantiations.
 */
omfErr_t omfsObjectNew(
			omfHdl_t				file,		/* IN - In this file */
			omfClassIDPtr_t	classID,	/* IN - create an object of this class */
			omfObject_t			*result)	/* OUT - and return the result here. */
{
	omfObject_t     	newobj;
	omfClassID_t  		idstr;
	omfInt16          byteOrder;
	omfProperty_t		idProp;
	
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);

	XPROTECT(file)
	{
#if OMFI_ENABLE_SEMCHECK
		if (isVirtualObj(classID))
			RAISE(OM_ERR_BAD_VIRTUAL_CREATE);
#endif
	
		strncpy(idstr, classID, (size_t) 4);
		newobj = (omfObject_t) CMNewObject(file->container);
	
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;
		CHECK(omfsWriteClassID(file, newobj, idProp, idstr));
	
		/*
		 * If the byteorder is different than the byteorder when created, add
		 * a byte order property to this object.
		 */
		if (file->byteOrder != OMNativeByteOrder)
		{
			if (streq(idstr, "AIFC"))
			{
				byteOrder = MOTOROLA_ORDER;
				CHECK(omfsPutByteOrder(file, newobj, byteOrder));
			}
			else if (streq(idstr, "WAVE"))
			{
				byteOrder = INTEL_ORDER;
				CHECK(omfsPutByteOrder(file, newobj, byteOrder));
			}
		}
	}
	XEXCEPT
	XEND
	
	*result = newobj;
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsSetProgressCallback
 *
 * 	Sets a callback which will be called at intervals
 *		during int  operations, in order to allow your application
 *		to pin a watch cursor, move a thermometer, etc...
 *
 * Argument Notes:
 *
 *		The progress callback supplied by you should have the following
 *		prototype:
 *
 *			void ProgressProc(omfHdl_t file, omfInt32 curVal, omfInt32 endVal);
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsSetProgressCallback(
			omfHdl_t				file,		/* IN - For this file */
			omfProgressProc_t aProc)	/* IN - Set this progress callback */
{
	omfAssertValidFHdl(file);
	file->progressProc = aProc;
	return(OM_ERR_NONE);
}

	/************************************************************
	 *
	 * Public functions used to extend the toolkit API
	 *
	 *************************************************************/

/************************
 * Function: omfsNewClass
 *
 * 	Add a new class to the class definition table.  Classes which are marked
 *		clsRegistered or clsPrivate may be written out to the files class
 *		dictionary.
 *
 * Argument Notes:
 *		type - Whether the class is kClsRequired, clsRegistered, or clsPrivate.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		OM_ERR_NOMEMORY -- not enough free memory to hold the entry.
 */
omfErr_t omfsNewClass(
			omfSessionHdl_t	session,			/* IN - For this session */
			omfClassType_t		type,				/* IN - add a class of this type */
			omfValidRev_t		revs,				/* IN - valid for these revisions */
			omfClassIDPtr_t	aClass,			/* IN - with this classID */
			omfClassIDPtr_t	itsSuperclass)	/* IN - derived from this superclass */
{
	OMClassDef     def;

	omfAssert((aClass != NULL), NULL, OM_ERR_NULL_PARAM);
	omfAssert((itsSuperclass != NULL), NULL, OM_ERR_NULL_PARAM);

	session->BentoErrorNumber = 0;
	session->BentoErrorRaised = FALSE;

	XPROTECT(NULL)
	{
		def.type = type;
		def.revs = revs;
		strncpy(def.aClass, aClass, 4);
		strncpy(def.itsSuperClass, itsSuperclass, 4);
		if((revs == kOmfTstRev1x) || (revs == kOmfTstRevEither))
		{
			CHECK(omfsTableAddClassID(session->classDefs1X, aClass, &def, sizeof(def)));
		}
		if((revs == kOmfTstRev2x) || (revs == kOmfTstRevEither))
		{
			CHECK(omfsTableAddClassID(session->classDefs2X, aClass, &def, sizeof(def)));
		}
	}
	XEXCEPT
	{
	}
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsRegisterType
 *
 * 	Add a new type to the type definition table.  This function takes an explicit
 *		omType_t, and should be called for required & registered types
 *		which are not registered by a media codec.
 *
 * Argument Notes:
 *		typeName - The leaf name of the type, will be preceeded by a header string
 *					(see below) in the Bento file.
 *		revs - Not the same as the revision which is sent to create.  This
 *			enum has three values:
 *					kOmfTstRev1x - valid for 1.x files only
 *					kOmfTstRev2x - valid for 2.x files only
 *					kOmfTstRevEither - valid for either rev
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		OM_ERR_NOMEMORY -- not enough free memory to hold the entry.
 */
omfErr_t omfsRegisterType(
			omfSessionHdl_t	session,		/* IN - For this session */
			omfType_t			aType,		/* IN - Register this typecode */
			omfValidRev_t		revs,			/* IN - calid for these revisions */
			char					*typeName,	/* IN - With this name */
			omfSwabCheck_t	swabNeeded)	/* IN - Does this type depend on "endian-ness" */
{
	OMTypeDef      type;
	char           *header = TYPE_HDR_BENTO;
	omfHdl_t			file;
	CMType			bentoID;
	
	session->BentoErrorNumber = 0;
	session->BentoErrorRaised = FALSE;

	XPROTECT(NULL)
	{
		type.revs = revs;
		strcpy( (char*) type.typeName, header);
		strcat(type.typeName, typeName);
		type.swabNeeded = swabNeeded;
		if(revs == kOmfTstRev1x || revs == kOmfTstRevEither)
		{
			CHECK(omfsTableAddType(session->types1X, aType, &type, sizeof(type)));
		}

		if(revs == kOmfTstRev2x || revs == kOmfTstRevEither)
		{
			CHECK(omfsTableAddType(session->types2X, aType, &type, sizeof(type)));
		}
		CHECK(omfsTableAddStringBlock(session->typeNames, type.typeName, &aType,
												sizeof(aType), kOmTableDupReplace));

		/* Add the new property to the caches of all open files
		 */
		for(file = session->topFile; file != NULL; file = file->prevFile)
		{
			bentoID = CvtTypeToBento(file, aType, NULL);
			CHECK(omfsTableAddType(file->types, aType, &bentoID,
									sizeof(bentoID)));
			CHECK(omfsTableAddValueBlock(file->typeReverse, &bentoID,
									sizeof(bentoID), &aType,
									sizeof(aType), kOmTableDupReplace)); /* !!!Needed by Parallax */
		}
	}
	XEXCEPT
	{
	}
	XEND

	return (OM_ERR_NONE);
}


static void ComputePropertyName(omfClassIDPtr_t classTag, char *propName, char *destBuf)
{
	omfInt32			tagLen, nameLen;
	char           *header = PROP_HDR_BENTO;
	
	if (classTag != NULL)
	  tagLen = 5;			/* name + ':' */
	else
		tagLen = 0;

	nameLen = strlen(header) + tagLen + 1 + strlen(propName) + 1;

	strcpy( (char*) destBuf, header);
	if (classTag != NULL)
	{
		strncat(destBuf, classTag, sizeof(omfClassID_t));
		strcat(destBuf, ":");
	}
	else
		classTag = OMClassAny;
	strcat(destBuf, propName);
}

/************************
 * Function: omfsRegisterProp
 *
 * 	Register a omfProperty code, supplying a name, class, type, and an enum
 *		value indicating which revisions the field is valid for.  This function takes
 *		an explicit omfProperty_t, and should be called for required & registered
 *		properties which are not registered by a media codec.
 *
 * Argument Notes:
 *		propName - The leaf name of the property, will be preceeded by a header string
 *					(see below) in the Bento file.
 *		revs - Not the same as the revision which is sent to create.  This
 *			enum has three values:
 *					kOmfTstRev1x - valid for 1.x files only
 *					kOmfTstRev2x - valid for 2.x files only
 *					kOmfTstRevEither - valid for either rev
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsRegisterProp(
			omfSessionHdl_t	session,		/* IN - For this session */
			omfProperty_t		aProp,		/* IN - Register this property code */
			omfValidRev_t		revs,			/* IN - valid for these revisions */
			char					*propName,	/* IN - with this name */
			omfClassIDPtr_t	classTag,	/* IN - attached to this class */
			omfType_t			type,			/* IN - with this type */
			omfPropertyOpt_t	opt)			/* IN - may be optional property */
{
	OMPropDef      prop;
	CMProperty		bentoID;
	omfHdl_t			file;
	omfBool			found;
	
	session->BentoErrorNumber = 0;
	session->BentoErrorRaised = FALSE;

	XPROTECT(NULL)
	{
			ComputePropertyName(classTag, propName, prop.propName);
			if (classTag == NULL)
				classTag = OMClassAny;
			strncpy(prop.classTag, classTag, 4);
			prop.revs = revs;
			prop.propID = aProp;
			prop.type[0] = type;
			prop.numTypes = 1;
			prop.testProc = NULL;
			prop.testName = NULL;
			prop.dataClass = NULL;
			prop.isOptional = (opt == kPropOptional);
			if(revs == kOmfTstRev1x || revs == kOmfTstRevEither)
			{
				CHECK(omfsTablePropertyLookup(session->properties1X, aProp, sizeof(prop), &prop,  &found));
				if(found)
				{
					if(prop.numTypes >= MAX_TYPE_VARIENTS)
						RAISE(OM_ERR_TOO_MANY_TYPES);
					prop.type[prop.numTypes++] = type;
				}

				CHECK(omfsTableAddProperty(session->properties1X, aProp, &prop,
										 sizeof(prop), kOmTableDupReplace));
			}
			if(revs == kOmfTstRev2x || revs == kOmfTstRevEither)
			{
				CHECK(omfsTablePropertyLookup(session->properties2X, aProp, sizeof(prop), &prop,  &found));
				if(found)
				{
					if(prop.numTypes >= MAX_TYPE_VARIENTS)
						RAISE(OM_ERR_TOO_MANY_TYPES);
					prop.type[prop.numTypes++] = type;
				}

				CHECK(omfsTableAddProperty(session->properties2X, aProp, &prop,
										 sizeof(prop), kOmTableDupReplace));
			}

		CHECK(omfsTableAddStringBlock(session->propertyNames, prop.propName, &aProp,
											sizeof(aProp), kOmTableDupReplace ));

		/* Add the new property to the caches of all open files
		 */
		for(file = session->topFile; file != NULL; file = file->prevFile)
		{
			bentoID = CvtPropertyToBento(file, aProp);
			CHECK(omfsTableAddProperty(file->properties, aProp, &bentoID,
									sizeof(bentoID), kOmTableDupError));
			CHECK(omfsTableAddValueBlock(file->propReverse, &bentoID, sizeof(bentoID), &aProp,
									sizeof(aProp), kOmTableDupError));
		}
	}
	XEXCEPT
	{
	}
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsRegisterDynamicType
 *
 * 	Register a type with a dynamic type code.  This function should be called when
 *		registering a private type, or when a codec requires a type.  The type code used
 *		to refer to the type is returned from this function, and must be saved in a variable
 *		in the client application, or in the persistant data block of a codec.
 *
 * Argument Notes:
 *		typeName - The leaf name of the type, will be preceeded by a header string
 *					(see below) in the Bento file.
 *		revs - Not the same as the revision which is sent to create.  This
 *			enum has three values:
 *					kOmfTstRev1x - valid for 1.x files only
 *					kOmfTstRev2x - valid for 2.x files only
 *					kOmfTstRevEither - valid for either rev
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsRegisterDynamicType(
			omfSessionHdl_t	session,		/* IN - For this session, register a */
			omfValidRev_t		revs,			/* IN - type for these revisions */
			char					*typeName,	/* IN - with this name */
			omfSwabCheck_t	swabNeeded,	/* IN - Does this type depend on "endian-ness" */
			omfType_t			*resultPtr)	/* OUT - Return the type code */
{
	*resultPtr = (omfType_t)session->nextDynamicType++;
	return(omfsRegisterType(session, *resultPtr, revs, typeName, swabNeeded));
}

/************************
 * Function: omfsRegisterDynamicProp
 *
 * 	Register a type with a dynamic property code.  This function should be called when
 *		registering a private property, or when a codec requires a property.  The type code used
 *		to refer to the property is returned from this function, and must be saved in a variable
 *		in the client application, or in the persistant data block of a codec.
 *
 * Argument Notes:
 *		propName - The leaf name of the property, will be preceeded by a header string
 *					(see below) in the Bento file.
 *		revs - Not the same as the revision which is sent to create.  This
 *			enum has three values:
 *					kOmfTstRev1x - valid for 1.x files only
 *					kOmfTstRev2x - valid for 2.x files only
 *					kOmfTstRevEither - valid for either rev
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsRegisterDynamicProp(
			omfSessionHdl_t	session,		/* IN - For this session, register a */
			omfValidRev_t		revs,			/* IN - property for these revisions */
			char					*propName,	/* IN - with this name */
			omfClassIDPtr_t	classTag,	/* IN - attached to this class */
			omfType_t			type,			/* IN - with this type */
			omfPropertyOpt_t	opt,			/* IN - may be optional property */
			omfProperty_t		*resultPtr)	/* OUT - return the property code */
{
	char						propNameBuf[128];
	omfBool					foundName, found1x, found2x;
	OMPropDef				propDef1x, propDef2x;
	omfProperty_t			propID;
	
	XPROTECT(NULL)
	{
		ComputePropertyName(classTag, propName, propNameBuf);
		CHECK(omfsTableLookupBlock(session->propertyNames, propNameBuf,
												sizeof(propID), &propID, &foundName));
		if(foundName)
		{
			CHECK(omfsTablePropertyLookup(session->properties1X, propID, sizeof(propDef1x),
													&propDef1x, &found1x));
			CHECK(omfsTablePropertyLookup(session->properties2X, propID, sizeof(propDef2x),
													&propDef2x, &found2x));
			XASSERT(found1x || found2x, OM_ERR_PROPID_MATCH);
			if(found1x && (propID >= OMLASTPROP) )
			{
				XASSERT(strcmp(propDef1x.propName, propNameBuf) == 0, OM_ERR_PROPID_MATCH);
			}
			if(found2x && propID > OMLASTPROP)
			{
				XASSERT(strcmp(propDef2x.propName, propNameBuf) == 0, OM_ERR_PROPID_MATCH);
			}
			*resultPtr = propID;
			if(!found1x && (revs == kOmfTstRev1x || revs == kOmfTstRevEither))
			{
				/* Name is not found in the 1x tree, and we are asked to add it, find the
				 * property ID, and add to the 1x tree.
				 */
				CHECK(omfsRegisterProp(session, *resultPtr, kOmfTstRev1x, propName, classTag, type, opt));
			}
			else if(!found2x && (revs == kOmfTstRev2x || revs == kOmfTstRevEither))
			{
				/* Name is not found in the 2x tree, and we are asked to add it, find the
				 * property ID, and add to the 2x tree.
				 */
				CHECK(omfsRegisterProp(session, *resultPtr, kOmfTstRev2x, propName, classTag, type, opt));
			}
		}
		else
		{
			/* Name is not found, make a new ID and add to requested revisions */
			*resultPtr = (omfProperty_t)session->nextDynamicProp++;
			CHECK(omfsRegisterProp(session, *resultPtr, revs, propName, classTag, type, opt));
		}
				
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiDatakindRegister()
 *
 *      This function can be used to register a new datakind with a
 *      file.  The name of the datakind and the already created datakind
 *      object must be supplied as input. If the datakind already exists, 
 *      no action will be taken and no error will be returned.  The function
 *      will add the datakind object to the HEAD Definitions index and
 *      it will add it to the file handle's datakind cache.
 *
 *      This function will only support 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiDatakindRegister(
	omfHdl_t file,            /* IN - File Handle */
    omfUniqueNamePtr_t name,  /* IN - Datakind Unique Name */
	omfDDefObj_t defObject)   /* IN - Datakind Definition Object */
{
    omfDDefObj_t tmpDef = NULL;
	omfObject_t head;
	char *header = "omfi:data:";
	omfUniqueName_t datakindName;
	omfInt32 numDefs;
	omfBool appended = FALSE;

	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssertNot1x(file);

	XPROTECT(file)
	  {
		/* Append "omfi:data" if name doesn't have it */
		if (strncmp(name, "omfi:data:", 10))
		  {
			strcpy( (char*) datakindName, header);
			strcat(datakindName, name);
		  }
		else
		  strcpy( (char*) datakindName, name);

		tmpDef = (omfDDefObj_t)omfsTableDefLookup(file->datakinds, 
												  datakindName);
		if (!tmpDef)
		  {
			/* Append the datakind object to the HEAD index */
			CHECK(omfsGetHeadObject(file, &head));
			CHECK(omfsAppendObjRefArray(file, head, OMHEADDefinitionObjects, 
										defObject));
			appended = TRUE;
			/* Add to the cache */
			CHECK(omfsTableAddDef(file->datakinds, datakindName, defObject));
		  }
		else
		  {
			RAISE(OM_ERR_DATAKIND_EXIST);
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (appended) /* Remove from OMHEADDefinitionObjects index */
		  {
			numDefs = omfsLengthObjRefArray(file, head, 
											OMHEADDefinitionObjects);
			if (numDefs > 0)
			  omfsRemoveNthObjRefArray(file, head, OMHEADDefinitionObjects, 
									   numDefs);
		  }
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiEffectDefRegister()
 *
 *      This function can be used to register a new effect definition with
 *      a file.  The name of the effect definition and the already created
 *      effect definition object must be supplied as input.  If the effect
 *      definition already exists, no action will be taken and no error will
 *      be returned.  The function will add the effect definition object to
 *      the HEAD Definitions index and it will add it to the file handle's
 *      effect definitions cache.
 *
 *      This function only supports 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiEffectDefRegister(
	omfHdl_t file,           /* IN - File Handle */
    omfUniqueNamePtr_t name, /* IN - Effect Definition Unique Name */
	omfDDefObj_t defObject)  /* IN - Effect Definition object */
{
    omfDDefObj_t tmpDef = NULL;
	omfObject_t head;
	char *header = "omfi:effect:";
	omfUniqueName_t effectName;
	omfBool appended = FALSE;
	omfInt32 numDefs;

	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssertNot1x(file);

	XPROTECT(file)
	  {
		/* Append "omfi:effect" if name doesn't have it */
		if (strncmp(name, "omfi:effect:", 12))
		  {
			strcpy( (char*) effectName, header);
			strcat(effectName, name);
		  }
		else
		  strcpy( (char*) effectName, name);

		tmpDef = (omfEDefObj_t)omfsTableDefLookup(file->effectDefs, 
												  effectName);
		if (!tmpDef)
		  {
			/* Append effectdef object to the HEAD index */
			CHECK(omfsGetHeadObject(file, &head));
			CHECK(omfsAppendObjRefArray(file, head, OMHEADDefinitionObjects, 
										defObject));
			appended = TRUE;
			/* Add to the cache */
			CHECK(omfsTableAddDef(file->effectDefs, effectName, defObject));
		  } 
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (appended) /* Remove from OMHEADDefinitionObjects index */
		  {
			numDefs = omfsLengthObjRefArray(file, head, 
											OMHEADDefinitionObjects);
			if (numDefs > 0)
			  omfsRemoveNthObjRefArray(file, head, OMHEADDefinitionObjects, 
									   numDefs);
		  }
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiGetNextProperty()
 *
 *      This function is an iterator over properties on a given object.
 *      Each time that it is called, it will return the next property
 *      (public or private) that it finds in Bento order.  An iterator
 *      handle must be created by calling omfiIteratorAlloc() before
 *      calling this function.  This iterator does not take search
 *      criteria.  It returns the OMF property and type IDs for the
 *      "next" property.  This information can be used to retrieve the
 *      value of the property by calling the lower level Object/Property
 *      accessor functions.  When all of the properties have been iterated
 *      over, the function returns the error status OM_ERR_NO_MORE_OBJECTS.
 *
 *      When the iteration is complete, the iterator can be cleared to
 *      be used on another object by calling omfiIteratorClear(), or it
 *      can be destroyed by calling omfiIteratorDispose().
 *
 *      This function will support both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiGetNextProperty(
	omfIterHdl_t iterHdl,    /* IN - Iterator Handle */
	omfObject_t obj,         /* IN - Object to iterator over */
	omfProperty_t *property, /* OUT - Property ID of "next" property */
	omfType_t *type)         /* OUT - Type ID of "next" type */
{
    CMProperty nextProperty, currProperty;
	CMType typeObj;
	CMValue valueObj;
	omfBool initialized = FALSE, found = FALSE, typeFound, propFound;

	*property = OMNoProperty;
	*type = OMNoType;
	omfAssertIterHdl(iterHdl);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterProp) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);
	omfAssert((obj != NULL) && (iterHdl->currentObj == NULL ||
									  iterHdl->currentObj == obj), 
					iterHdl->file, OM_ERR_INVALID_OBJ);

	XPROTECT(iterHdl->file)
	  {
		/* Initialize iterator if first time through */
		if (iterHdl->iterType == kIterNull)
		  {
			iterHdl->iterType = kIterProp;
			iterHdl->currentObj = (CMObject)obj;
			iterHdl->currentProp = NULL;
			iterHdl->currentValue = NULL;
		  }

		initialized = TRUE;

		if((iterHdl->currentProp != NULL) && (iterHdl->currentValue != NULL))
		{
			nextProperty = iterHdl->currentProp;
			valueObj = CMGetNextValue((CMObject)obj, 
									  (CMProperty)nextProperty, iterHdl->currentValue);
			/* Bento SHOULD catch this (infinite loop), but double-check */
			if((valueObj != NULL) && (valueObj != iterHdl->currentValue))
			{
				*property = CvtPropertyFromBento(iterHdl->file, nextProperty, &propFound);
				CMGetValueInfo(valueObj, NULL, NULL, NULL, &typeObj, NULL);
				*type = CvtTypeFromBento(iterHdl->file, typeObj, &typeFound);
				found = TRUE;
			}
		}
		
		if(!found)
		{
			/* If it is the first time, iterHdl->currentProp will be NULL */
			currProperty = iterHdl->currentProp;
			nextProperty = CMGetNextObjectProperty((CMObject)obj, 
												   currProperty);
		}
		while (nextProperty && !found)
		  {
			*property = CvtPropertyFromBento(iterHdl->file, nextProperty, &propFound);
			/* Skip past non-OMF properties */
			if (!propFound)
			  {
				currProperty = nextProperty;
				nextProperty = CMGetNextObjectProperty((CMObject)obj, 
													   currProperty);
			  }
			else
			  {
				valueObj = CMGetNextValue((CMObject)obj, 
										  (CMProperty)nextProperty, NULL);
				CMGetValueInfo(valueObj, NULL, NULL, NULL, &typeObj, NULL);
				*type = CvtTypeFromBento(iterHdl->file, typeObj, &typeFound);
				found = TRUE;
			  }
		  } /* while */

		if (found)
		{
			iterHdl->currentProp = nextProperty;
			iterHdl->currentValue = valueObj;
		}
		else /* No more properties */
		  {
			RAISE(OM_ERR_NO_MORE_OBJECTS);
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (!initialized)
		  omfiIteratorClear(iterHdl->file, iterHdl);
		else
		  iterHdl->currentProp = NULL;

		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiGetPropertyName()
 *
 *      Given an OMF property ID, this function returns the property
 *      name string associated with that ID.
 *
 *      This function supports both 1.x and 2.x files.
 *    
 * Argument Notes:
 *      An array of characters large enough to hold a uniquename
 *      (OMUNIQUENAME_SIZE) must be passed in through propName (use
 *      the omfUniqueName_t type).
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiGetPropertyName(
	omfHdl_t file,               /* IN - File Handle */
	omfProperty_t property,      /* IN - Property ID */
	omfInt32 nameSize,           /* IN - Size of name buffer */
	omfUniqueNamePtr_t propName) /* IN/OUT - Property Unique Name */
{
  CMProperty propObj;
  CMGlobalName currName;

  omfAssertValidFHdl(file);
  omfAssertIsOMFI(file);

  propObj = CvtPropertyToBento(file, property);
  currName = (char *)CMGetGlobalName(propObj);
	  
  if (currName)
	 strncpy(propName, currName, nameSize);

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiGetPropertyTypename()
 *
 *      Given an OMF type ID, this function returns the type name string
 *      associated with that ID.
 *
 *      This function supports both 1.x and 2.x files.
 *
 * Argument Notes:
 *      An array of characters large enough to hold a uniquename
 *      (OMUNIQUENAME_SIZE) must be passed in through propName (use
 *      the omfUniqueName_t type).
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiGetPropertyTypename(
	omfHdl_t file,               /* IN - File Handle */
	omfType_t type,              /* IN - Type ID */
	omfInt32 nameSize,           /* IN - Size of name buffer */
	omfUniqueNamePtr_t typeName) /* IN/OUT - Type Unique Name */
{
  CMType typeObj;
  CMGlobalName currName;

  omfAssertValidFHdl(file);
  omfAssertIsOMFI(file);

  typeObj = CvtTypeToBento(file, type, NULL);
  currName = (char *)CMGetGlobalName(typeObj);

  if (currName)
	 strncpy(typeName, currName, nameSize);

  return(OM_ERR_NONE);
}

/*************************************************************************
 * Function: omfiGetNextObject()
 *
 *      This function is an iterator over all of the objects in the
 *      file.  The order of the objects is not dependent of the mobs that
 *      they belong to, but instead based on the order that they were
 *      created in the file and registered with Bento.  The iterator
 *      will return all objects (public and private) that were registered
 *      with the Toolkit.  The property iterator (omfiGetNextProperty())
 *      can be used to retrieve the properties off of the objects.
 *
 *      An iterator handle must be created by calling omfiIteratorAlloc()
 *      before calling this function.  This iterator does not take search
 *      criteria.  When all of the objects have been iterated over, the
 *      function returns the error status OM_ERR_NO_MORE_OBJECTS.
 *
 *      When the iteration is complete, the iterator should be destroyed
 *      by calling omfiIteratorDispose().
 *
 *      This function will support both 1.x and 2.x files.
 *
 * Argument Notes:
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfiGetNextObject(
	omfIterHdl_t iterHdl,    /* IN - Iterator Handle */
	omfObject_t *obj)        /* OUT - Next object */
{
    omfObject_t head, nextObject, currObject;
	CMContainer OMFIContainer;
	omfBool found = FALSE, initialized = FALSE;

	omfAssertIsOMFI(iterHdl->file);
	*obj = NULL;
	omfAssertIterHdl(iterHdl);
	omfAssertIterMore(iterHdl);
	omfAssert((iterHdl->iterType == kIterObj) ||
					(iterHdl->iterType == kIterNull), 
					iterHdl->file, OM_ERR_ITER_WRONG_TYPE);

 	XPROTECT(iterHdl->file) 
	  {
		/* Get head object and container */
		CHECK(omfsGetHeadObject(iterHdl->file, &head));
		OMFIContainer = CMGetObjectContainer((CMObject)head);

		/* Initialize iterator if first time through */
		if (iterHdl->iterType == kIterNull)
		  {
			/* Skip past the first object, the Bento Container Object */
			iterHdl->currentObj = CMGetNextObject(OMFIContainer, NULL);
			iterHdl->iterType = kIterObj;
		  }
		initialized = TRUE;

		currObject = (omfObject_t)iterHdl->currentObj;
		nextObject = CMGetNextObject(OMFIContainer, (CMObject)currObject);
		while (nextObject && !found)
		  {
			/* 
			 * If the object is a Bento type or property description object,
			 * then skip past it.
			 */
			if (!CMIsType((CMObject)nextObject) && 
				!CMIsProperty((CMObject)nextObject))
			  {
				found = TRUE;
			  }
			else
			  {
				currObject = nextObject;
				nextObject = CMGetNextObject(OMFIContainer, 
											 (CMObject)currObject);
			  }
		  }

		if (found)
		  {
			iterHdl->currentObj = (CMObject)nextObject;
			*obj = (omfObject_t)iterHdl->currentObj;
		  }
		else /* No more objects */
		  {
			RAISE(OM_ERR_NO_MORE_OBJECTS);
		  }
	  } /* XPROTECT */

	XEXCEPT
	  {
		if (!initialized)
		  omfiIteratorClear(iterHdl->file, iterHdl);
		else
		  iterHdl->currentObj = NULL;
		return(XCODE());
	  }
	XEND;

	return(OM_ERR_NONE);
}


/************************
 * Function: OMReadProp
 *
 * 		Internal function to read the given property from the file, and apply
 *		semantic error checking to the input parameters and the result.  This function
 *		should only be called by wrapper functions for the correct type.
 *		The following semantic checks are performed:
 *			1)	Validate that object contains the given property.  This is possible even
 *				for private types and properties, as all types, and properties
 *				use common registration functions and all classes are registered in
 *				a common class dictionary.
 *			2)	Validate the range of the data returned.  The client application may
 *				ignore this error, as it has a different error code.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions and Bento errors.  See top of the file.
 *		OM_ERR_OBJECT_SEMANTIC -- Failed a semantic check on an input obj
 *		OM_ERR_DATA_OUT_SEMANTIC -- Failed a semantic check on an output data
 *		OM_ERR_BAD_PROP -- omfProperty_t code was out of range.
 *		OM_ERR_BAD_TYPE -- omfType_t code was out of range.
 *		OM_ERR_SWAB -- Unable to byte swap the given data type.
 */
omfErr_t OMReadProp(
			omfHdl_t			file,			/* IN - For this omf file */
			omfObject_t		obj,			/* IN - in this object */
			omfProperty_t	prop,			/* IN - read this property */
			omfPosition_t	offset,		/* IN - at this Bento Offset. */
			omfSwabCheck_t		swabType,	/* IN - Swab the data (optional) */
			omfType_t		dataType,	/* IN - check the type */
			omfInt32			dataSize,	/* IN --and size */
			void				*data)		/* IN/OUT - and put the result here. */
{
	CMValue         val;
	CMProperty      cprop;
	CMType          ctype;
	omfBool         swab;
	omfPosition_t	endOffset;
	omfLength_t		objectSize;
#if OMFI_ENABLE_SEMCHECK
	omfObjectTag_t	objDebugID;
#endif
	omfSwabCheck_t		existingSwabType;
	
	clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((data != NULL), file, OM_ERR_BADDATAADDRESS);

	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, &existingSwabType);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);


	XPROTECT(file)
	{			
#if OMFI_ENABLE_SEMCHECK
		/*
		 * Check for preconditions here
		 */
		if (omfsCheckObjectType(file, obj, prop, dataType))
		{
			(void)omfsReadObjectTag(file, obj, OMObjID, objDebugID);
			RAISE(OM_ERR_OBJECT_SEMANTIC);
		}
#endif

		if (CMCountValues((CMObject) obj, cprop, ctype))
		{
			XASSERT(existingSwabType == swabType, OM_ERR_BAD_TYPE);
			if ((dataSize > 1) && (swabType == kSwabIfNeeded))
			{
				swab = ompvtIsForeignByteOrder(file, obj);
			}
			else
				swab = FALSE;
	
			val = CMUseValue((CMObject) obj, cprop, ctype);
	
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
	
			/* if(offset+dataSize > (omfInt32)CMGetValueSize(val))		 */
	
			objectSize = CMGetValueSize(val);
			if (omfsInt64Greater(offset, objectSize))	/* If completely off of the end, read nothing */
				RAISE(OM_ERR_END_OF_DATA);
	
			(void) CMReadValueData(val, (CMPtr) data, offset, dataSize);
			if (swab)
			{
				if (dataSize == sizeof(omfInt16))
					omfsFixShort((omfInt16 *) data);
				else if (dataSize == sizeof(omfInt32))
					omfsFixLong((omfInt32 *) data);
				else if (dataSize == sizeof(omfInt64))
					omfsFixLong64((omfInt64 *) data);
				else
					RAISE(OM_ERR_SWAB);
			}

			endOffset = offset;
			omfsAddInt32toInt64(dataSize, &endOffset);
			if (omfsInt64Greater(endOffset, objectSize))		/* If partly off of the end, read & return error */
				RAISE(OM_ERR_END_OF_DATA);
		} else
			RAISE(OM_ERR_PROP_NOT_PRESENT);
	
		if (file->BentoErrorRaised)
			RAISE(OM_ERR_BENTO_PROBLEM);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: OMGetPropertyFileOffset
 *
 *		Given a bento property, return the absolute file offset of the
 *		start of the property.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions and Bento errors.  See top of the file.
 *		OM_ERR_BAD_PROP -- omfProperty_t code was out of range.
 *		OM_ERR_BAD_TYPE -- omfType_t code was out of range.
 */
omfErr_t OMGetPropertyFileOffset(
			omfHdl_t			file,			/* IN - For this omf file */
			omfObject_t		obj,			/* IN - in this object */
			omfProperty_t	prop,			/* IN - find the file offset of this property */
			omfPosition_t		offset,			/* IN - at this Bento Offset. */
			omfType_t		dataType,		/* IN - check the type */
			omfLength_t		*result)		/* OUT - and put the result here. */
{
	CMValue         	val;
	CMProperty     cprop;
	CMType          	ctype;
	omfInt16		errVal;
	clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((result != NULL), file, OM_ERR_NULL_PARAM);

	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);


	XPROTECT(file)
	{		
#if OMFI_ENABLE_SEMCHECK
		/*
		 * Check for preconditions here
		 */
		if (omfsCheckObjectType(file, obj, prop, dataType))
			RAISE(OM_ERR_OBJECT_SEMANTIC);
#endif

		if (CMCountValues((CMObject) obj, cprop, ctype))
		{
			val = CMUseValue((CMObject) obj, cprop, ctype);
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
	
			*result = CMGetValueDataOffset(val, offset, &errVal);
		} else
			RAISE(OM_ERR_PROP_NOT_PRESENT);
	
		if (file->BentoErrorRaised)
			RAISE(OM_ERR_BENTO_PROBLEM);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: OMWriteProp		(INTERNAL)
 *
 * 		Internal function to write the given property to the file, and apply
 *		semantic error checking to the input parameters.  This function
 *		should only be called by wrapper functions for the correct type.
 *		The following semantic checks are performed:
 *			1)	Validate that object contains the given property.  This is possible even
 *				for private types and properties, as all types, and properties
 *				use common registration functions and all classes are registered in
 *				a common class dictionary.
 *			2)	Validate the range of the data passed in.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions and Bento errors.  See top of the file.
 *		OM_ERR_BAD_PROP -- omfProperty_t code was out of range.
 *		OM_ERR_OBJECT_SEMANTIC -- Failed a semantic check on an input obj
 *		OM_ERR_DATA_IN_SEMANTIC -- Failed a semantic check on an input data
 *		OM_ERR_BAD_TYPE -- omfType_t code was out of range.
 *		OM_ERR_SWAB -- Unable to byte swap the given data type.
 */
omfErr_t OMWriteProp(
			omfHdl_t			file,			/* IN -- For this omf file */
			omfObject_t		obj,			/* IN -- in this object */
			omfProperty_t	prop,			/* IN -- read this property */
			omfPosition_t	offset,		/* IN -- at this Bento Offset. */
			omfType_t		dataType,	/* IN -- check the type */
			omfInt32			dataSize,	/* IN -- and size. */
			const void		*data)		/* IN -- This buffer points to the data. */
{
	CMValue         val;
	CMProperty      cprop;
	CMType          ctype;
	omfInt16			tmp16;
	omfInt32			tmp32;
	omfInt64			tmp64;
	omfSwabCheck_t		swabType;
	
	clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((data != NULL), file, OM_ERR_BADDATAADDRESS);

	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, &swabType);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);

	XPROTECT(file)
	{
#if OMFI_ENABLE_SEMCHECK
		/*
		 * Check for object validity here (not data validity)
		 */
		if (omfsCheckObjectType(file, obj, prop, dataType))
			RAISE(OM_ERR_OBJECT_SEMANTIC);
#endif

		if (CMCountValues((CMObject) obj, cprop, ctype))
			val = CMUseValue((CMObject) obj, cprop, ctype);
		else
			val = CMNewValue((CMObject) obj, cprop, ctype);
	
		if (file->BentoErrorRaised)
			RAISE(OM_ERR_BENTO_PROBLEM);
	
		if(file->byteOrder != OMNativeByteOrder)
		{
			if(swabType == kSwabIfNeeded)
			{
				if (dataSize == sizeof(omfInt16))
				{
					tmp16 = *((omfInt16 *)data);
					omfsFixShort(&tmp16);
					data = &tmp16;
				}
				else if (dataSize == sizeof(omfInt32))
				{
					tmp32 = *((omfInt32 *)data);
					omfsFixLong(&tmp32);
					data = &tmp32;
				}
				else if (dataSize == sizeof(omfInt64))
				{
					tmp64 = *((omfInt64 *)data);
					omfsFixLong64(&tmp64);
					data = &tmp64;
				}
				else if(dataSize != 1)
					RAISE(OM_ERR_SWAB);
			}
		}
		(void) CMWriteValueData(val, (CMPtr) data, offset, dataSize);
	
		if (file->BentoErrorRaised)
		{
			if(file->BentoErrorNumber == CM_err_BadWrite)
			{
				RAISE(OM_ERR_CONTAINERWRITE);
			}
			else
			{
				RAISE(OM_ERR_BENTO_PROBLEM);
			}
		}
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: OMGetNumPropertySegments
 *
 * 	Tell how many segments a property is broken up into within the file.
 *		This function is usually applied to media data, when the
 *		application wishes to read the data directly, without the toolkit.
 *
 *		Note that the toolkit can not tell if the file is contigous on
 *		the disk.
 *
 * Argument Notes:
 *		Both property code and data type need to be specified.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 * 	OM_ERR_OBJECT_SEMANTIC - The propertry given does not match the
 *			object given
 */
omfErr_t OMGetNumPropertySegments(
			omfHdl_t		file,			/* IN -- For this omf file */
			omfObject_t		obj,			/* IN -- in this object */
			omfProperty_t	prop,			/* IN -- is property */
			omfType_t		dataType,		/* IN -- of this type */
			omfInt32		*numSegments)	/* OUT -- how many segments are there? */
{
	CMValue       	val;
	CMProperty    	cprop;
	CMType         	ctype;
	TOCValueHdrPtr	theValueHdr;
	
	clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((numSegments != NULL), file, OM_ERR_NULL_PARAM);
	*numSegments = 0;

	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);
	
	XPROTECT(file)
	{
#if OMFI_ENABLE_SEMCHECK
		/*
		 * Check for preconditions here
		 */
		if (omfsCheckObjectType(file, obj, prop, dataType))
			RAISE(OM_ERR_OBJECT_SEMANTIC);
#endif

		if (CMCountValues((CMObject) obj, cprop, ctype))
		{
			val = CMUseValue((CMObject) obj, cprop, ctype);
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
			theValueHdr = (TOCValueHdrPtr)val;
			*numSegments = cmCountListCells(&theValueHdr->valueList);
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
		}
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: OMGetPropertySegmentInfo
 *
 * 	Tell how many segments a property is broken up into within the file.
 *		This function is usually applied to media data, when the
 *		application wishes to read the data directly, without the toolkit.
 *
 *		Note that the toolkit can not tell if the file is contigous on
 *		the disk.
 *
 * Argument Notes:
 *		Both property code and data type need to be specified.
 *		The index is 1-based.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 * 	OM_ERR_OBJECT_SEMANTIC - The propertry given does not match the
 *			object given
 */
omfErr_t OMGetPropertySegmentInfo(
			omfHdl_t		file,		/* IN -- For this omf file */
			omfObject_t		obj,		/* IN -- in this object */
			omfProperty_t	prop,		/* IN -- is property */
			omfType_t		dataType,	/* IN -- of this type */
			omfInt32		index,		/* IN -- for this segment */
			omfPosition_t	*startPos,	/* OUT -- where tdoe sthe segment begin, */
			omfLength_t		*length)	/* OUT -- and how int  is it? */
{
	CMValue       	val;
	CMProperty    	cprop;
	CMType         	ctype;
	TOCValueHdrPtr	theValueHdr;
	TOCValuePtr		valuePtr;
	omfPosition_t	zeroPos;
	omfInt32		numSeg;
		
	clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);
	omfAssert((startPos != NULL), file, OM_ERR_NULL_PARAM);
	omfAssert((length != NULL), file, OM_ERR_NULL_PARAM);
	omfsCvtInt32toInt64(0, startPos);
	omfsCvtInt32toInt64(0, length);
	omfsCvtInt32toInt64(0, &zeroPos);

	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);
	
	XPROTECT(file)
	{
#if OMFI_ENABLE_SEMCHECK
		/*
		 * Check for preconditions here
		 */
		if (omfsCheckObjectType(file, obj, prop, dataType))
			RAISE(OM_ERR_OBJECT_SEMANTIC);
#endif

		if (CMCountValues((CMObject) obj, cprop, ctype))
		{
			val = CMUseValue((CMObject) obj, cprop, ctype);
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
			theValueHdr = (TOCValueHdrPtr)val;
			numSeg = cmCountListCells(&theValueHdr->valueList);
			if((index < 1) || (index > numSeg))
				RAISE(OM_ERR_BADINDEX);
			valuePtr = (TOCValuePtr)cmGetNthListCell(&theValueHdr->valueList, index);
			*startPos = valuePtr->value.notImm.value;
			*length = valuePtr->value.notImm.valueLen;

			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
		}
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: OMIsPropertyContiguous
 *
 * 	Tell if a property is contigous in the file.   This function
 *		is usually applied to media data, when the application wishes
 *		to read the data directly, without the toolkit.
 *
 *		Note that the toolkit can not tell if the file is contigous on
 *		the disk.
 *
 * Argument Notes:
 *		Both property code and data type need to be specified.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 * 	OM_ERR_OBJECT_SEMANTIC - The propertry given does not match the
 *			object given
 */
omfErr_t OMIsPropertyContiguous(
			omfHdl_t			file,				/* IN -- For this omf file */
			omfObject_t		obj,				/* IN -- in this object */
			omfProperty_t	prop,				/* IN -- is property */
			omfType_t		dataType,		/* IN -- of this type */
			omfBool			*isContig)		/* OUT -- contiguous? */
{
	omfInt32	numSegments;
	XPROTECT(file)
	{
		CHECK(OMGetNumPropertySegments(file, obj, prop, dataType, &numSegments));
		*isContig = (numSegments == 1 || numSegments == 0);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsGetArrayLength
 *
 * 	Get the number of elements in an arrayed OMFI object.
 *		Wrapper functions exist for the standard OMFI arrayed types
 *		(ObjRefArray), and should be used instead of this function.
 *		This function is semi-public in order to allow the use of
 *		registered and private properties which are arrayed.
 *
 * Argument Notes:
 *		Both property code and data type need to be specified.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsGetArrayLength(
			omfHdl_t			file,			/* IN -- For this omf file */
			omfObject_t		obj,			/* IN -- in this object */
			omfProperty_t	prop,			/* IN -- and this property */
			omfType_t		dataType,	/* IN -- of this data type */
			omfInt32			dataSize,	/* IN -- with entries of this size */
			omfInt32			*length)		/* OUT - return the number of elements */
{
	omfBool         Swab;
	CMProperty      cprop;
	CMType          ctype;
	CMValue         val;
	CMSize          siz;
	CMSize32        siz32;
	omfInt64		zero;

	omfsCvtInt32toInt64(0, &zero);
	(void) clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);
	*length = 0;

	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);

	XPROTECT(file)
	{
		if (CMCountValues((CMObject) obj, cprop, ctype))
		{
			Swab = ompvtIsForeignByteOrder(file, obj);
			val = CMUseValue((CMObject) obj, cprop, ctype);
	
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
	
			siz = CMGetValueSize(val);
			CHECK(omfsTruncInt64toUInt32(siz, &siz32)); /* OK FRAMEOFFSET */
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
			if(siz32 < sizeof(omfInt16))
				*length = 0;
			else
				*length = (siz32 - sizeof(omfInt16)) / dataSize;
		}
		else
			*length = 0;
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: OMPutNthPropHdr
 *
 * 	Set an indexed elements value in an arrayed OMFI object.
 *		Wrapper functions exist for the standard OMFI arrayed types
 *		(ObjRefArray), and should be used instead of this function.
 *		This function is semi-public in order to allow the use of
 *		registered and private properties which are arrayed.
 *
 * Argument Notes:
 *		dataSize -- This field allows the data storage format on disk
 *		Both property code and data type need to be specified.
 *		to be padded beyond the in-memory size.
 *		index - Must be in the range of 1 <= index <= (arraySize+1).
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADINDEX - Index is out of range.
 */
omfErr_t OMPutNthPropHdr(
			omfHdl_t			file,			/* IN -- For this omf file */
			omfObject_t		obj,			/* IN -- in this object */
			omfProperty_t	prop,			/* IN -- and this property */
			omfInt32			index,		/* IN -- Set this array element */
			omfType_t		dataType,	/* IN - array element type */
			omfInt32			dataSize,	/* IN - array element size in bytes */
			omfPosition_t	*outOffset)	/* OUT - Here is where to write the data */
{
	omfInt32       NElements;
	omfInt16			elem16;
	CMCount        Offset;
	CMValue        val;
	CMProperty     cprop;
	CMType         ctype;
	omfInt64			zero;
	omfBool			swab;
	
	(void) clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);

	omfsCvtInt32toInt64(0, &zero);
	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);

	XPROTECT(file)
	{
		omfsCvtInt32toInt64(0, outOffset);
		if (CMCountValues((CMObject) obj, cprop, ctype))
		{
			val = CMUseValue((CMObject) obj, cprop, ctype);
	
			CHECK(omfsGetArrayLength(file, obj, prop, dataType, dataSize, &NElements));
	
			if (index > NElements + 1)	/* can't put more than 1 past
							 * the end */
				RAISE(OM_ERR_BADINDEX);
	
			omfsCvtInt32toInt64(2 + ((index - 1) * dataSize), &Offset);
	
			NElements = (omfInt16) max(NElements, index);
			if((NElements & 0x7FFF000) != 0)
				elem16 = (omfInt16)0xFFFF;
			else
				elem16 = (omfInt16)NElements;
			swab = ompvtIsForeignByteOrder(file, obj);
			if(swab)
				omfsFixShort(&elem16);
			CMWriteValueData(val, (CMPtr) & elem16, zero, sizeof(omfInt16));
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
		} else if (index == 1)
		{
			val = CMNewValue((CMObject) obj, cprop, ctype);
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
				
			/* hack to discourage immediate data!  Write out index PLUS first element (zeroes)
			   to save the space in the file, then fill with real data.  If the index is simply
			   written out first, then it will go into an immediate which will then be 
			   converted into a non-immediate.  This technique avoids the immediate in the first
			   place */
			CMWriteValueData(val, (CMPtr) zeroes, zero, sizeof(omfInt16) + dataSize);	
	
			elem16 = 1;
			if(file->byteOrder != OMNativeByteOrder)
				omfsFixShort(&elem16);
			CMWriteValueData(val, (CMPtr) & elem16, zero, sizeof(omfInt16));
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
	
			omfsCvtInt32toInt64(2, &Offset);
		}
		*outOffset = Offset;
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: OMGetNthPropHdr
 *
 * 	Get an indexed elements value from an arrayed OMFI object.
 *		Wrapper functions exist for the standard OMFI arrayed types
 *		(ObjRefArray), and should be used instead of this function.
 *		This function is semi-public in order to allow the use of
 *		registered and private properties which are arrayed.
 *
 * Argument Notes:
 *		dataType - The type of the property (ie: OMObjIndex), as
 *						opposed to the type of the data within.
 *		dataSize - The size of an individual element in the array.
 *		index - Must be in the range of 1 <= index <= arraySize.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_BADINDEX - Index is out of range.
 */
omfErr_t OMGetNthPropHdr(
			omfHdl_t			file,			/* IN -- For this omf file */
			omfObject_t		obj,			/* IN -- in this object */
			omfProperty_t	prop,			/* IN -- and this property */
			omfInt32			index,		/* IN -- Set this array element header */
			omfType_t		dataType,	/* IN - array element type */
			omfInt32			dataSize,	/* IN - array element size in bytes */
			omfPosition_t	*outOffset)	/* OUT - Here is where to read the data */
{
	omfInt32        NElements;
	CMCount         Offset;
	CMValue         val;
	CMProperty      cprop;
	CMType          ctype;

	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);

	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);

	XPROTECT(file)
	{
		omfsCvtInt32toInt64(0, outOffset);
		if (CMCountValues((CMObject) obj, cprop, ctype))
		{
			val = CMUseValue((CMObject) obj, cprop, ctype);
		
			CHECK(omfsGetArrayLength(file, obj, prop, dataType, dataSize, &NElements));
		
			if (index > NElements)
				RAISE(OM_ERR_BADINDEX);
		
			omfsCvtInt32toInt64(2 + ((index - 1) * dataSize), &Offset);
		}
		else
			RAISE(OM_ERR_PROP_NOT_PRESENT);
		
		*outOffset = Offset;
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}


/************************
 * Function: OMReadBaseProp
 *
 * 	Version of OMReadProp which byte-swaps when needed and always reads
 *		from an offset of 0.  OMReadProp must be used whenever byte swapping
 *		must be suppressed, or if a property is a compound type.
 *
 * Argument Notes:
 *		dataType - The type of the property (ie: OMObjIndex), as
 *						opposed to the type of the data within.
 *		dataSize - The size of an individual element in the array.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions and Bento errors.  See top of the file.
 *		OM_ERR_OBJECT_SEMANTIC -- Failed a semantic check on an input obj
 *		OM_ERR_DATA_OUT_SEMANTIC -- Failed a semantic check on an output data
 *		OM_ERR_BAD_PROP -- omfProperty_t code was out of range.
 *		OM_ERR_BAD_TYPE -- omfType_t code was out of range.
 *		OM_ERR_SWAB -- Unable to byte swap the given data type.
 */
omfErr_t OMReadBaseProp(
			omfHdl_t			file,			/* IN -- For this omf file */
			omfObject_t		obj,			/* IN -- in this object */
			omfProperty_t	prop,			/* IN -- read this property */
			omfType_t		dataType,	/* IN -- Check the type */
			omfInt32			dataSize,	/* IN -- and size */
			void				*data)		/* IN/OUT -- and put the result here. */
{
	omfPosition_t	zeroPos = omfsCvtInt32toPosition(0, zeroPos);
	
	return (OMReadProp(file, obj, prop, zeroPos, kSwabIfNeeded, dataType, dataSize, data));
}

/************************
 * Function: OMWriteBaseProp
 *
 * 	Version of OMWriteProp which always reads from an offset of 0.
 *		OMWrite Prop must be used whenever a property is a compound type.
 *
 * Argument Notes:
 *		dataType - The type of the property (ie: OMObjIndex), as
 *						opposed to the type of the data within.
 *		dataSize - The size of an individual element in the array.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions and Bento errors.  See top of the file.
 *		OM_ERR_OBJECT_SEMANTIC -- Failed a semantic check on an input obj
 *		OM_ERR_DATA_IN_SEMANTIC -- Failed a semantic check on an input data
 *		OM_ERR_BAD_PROP -- omfProperty_t code was out of range.
 *		OM_ERR_BAD_TYPE -- omfType_t code was out of range.
 *		OM_ERR_SWAB -- Unable to byte swap the given data type.
 */
omfErr_t OMWriteBaseProp(
			omfHdl_t			file,			/* IN -- For this omf file */
			omfObject_t		obj,			/* IN -- in this object */
			omfProperty_t	prop,			/* IN -- write this property */
			omfType_t		dataType,	/* IN -- Check the type */
			omfInt32			dataSize,	/* IN -- and size */
			const void		*data)		/* IN -- This buffer points to the data. */
{
	omfPosition_t	zeroPos = omfsCvtInt32toPosition(0, zeroPos);

	return (OMWriteProp(file, obj, prop, zeroPos, dataType, dataSize, data));
}

/************************
 * Function: OMLengthProp
 *
 * 	Internal function to find the length of the given property.  This function
 *		should only be called by wrapper functions for the correct type.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions and Bento errors.  See top of the file.
 *		OM_ERR_BAD_PROP -- omfProperty_t code was out of range.
 *		OM_ERR_BAD_TYPE -- omfType_t code was out of range.
 */
omfErr_t OMLengthProp(
			omfHdl_t			file,			/* IN -- For this omf file */
			omfObject_t		obj,			/* IN -- in this object */
			omfProperty_t	prop,			/* IN -- find the length of this properties value */
			omfType_t		dataType,	/* IN -- Check the type */
			omfInt32			defaultVal,	/* IN -- and pass back this if property is missing */
			omfLength_t		*length)		/* OUT -- Return the length through here. */
{
	CMValue         val;
	CMProperty      cprop;
	CMType          ctype;

	clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((length != NULL), file, OM_ERR_BADDATAADDRESS);
	omfsCvtInt32toInt64(defaultVal, length);

	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);
	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);

	XPROTECT(file)
	{
#if OMFI_ENABLE_SEMCHECK
		/*
		 * Check for preconditions here
		 */
		if (omfsCheckObjectType(file, obj, prop, dataType))
		{
			RAISE(OM_ERR_OBJECT_SEMANTIC);
		}
#endif

		if (CMCountValues((CMObject) obj, cprop, ctype))
		{
			val = CMUseValue((CMObject) obj, cprop, ctype);
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
	
			*length = CMGetValueSize(val);
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
		} else
			RAISE(OM_ERR_PROP_NOT_PRESENT);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: OMRemovePropData
 *
 * 	Internal function to remove data from a given property.  This function
 *		should only be called by wrapper functions for the correct type.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions and Bento errors.  See top of the file.
 *		OM_ERR_BAD_PROP -- omfProperty_t code was out of range.
 *		OM_ERR_BAD_TYPE -- omfType_t code was out of range.
 *		OM_ERR_END_OF_DATA -- <offset> or <offset>+<count> is out of range.
 */
omfErr_t OMRemovePropData(
			omfHdl_t			file,			/* IN -- For this omf file */
			omfObject_t		obj,			/* IN -- in this object */
			omfProperty_t	prop,			/* IN -- find the length of this properties value */
			omfType_t		dataType,	/* IN -- Check the type */
			omfPosition_t	offset,	/* IN - at this byte offset */
			omfLength_t		length)	/* IN - for this many bytes */
{
	CMValue         val;
	CMProperty      cprop;
	CMType          ctype;
	omfLength_t		endOffset;
	omfLength_t		maxLength;
	
	clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);

	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);
	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);

	XPROTECT(file)
	{
#if OMFI_ENABLE_SEMCHECK
		/*
		 * Check for preconditions here
		 */
		if (omfsCheckObjectType(file, obj, prop, dataType))
		{
			RAISE(OM_ERR_OBJECT_SEMANTIC);
		}
#endif

		if (CMCountValues((CMObject) obj, cprop, ctype))
		{
			val = CMUseValue((CMObject) obj, cprop, ctype);
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
			
			maxLength = CMGetValueSize(val);
			endOffset	= offset;
			omfsAddInt64toInt64( length, &endOffset );
			if( omfsInt64Less( maxLength, offset ) || 
				omfsInt64Less( maxLength, endOffset )
				)
				RAISE( OM_ERR_END_OF_DATA );
				
			CMDeleteValueData( val, offset, length );
	
		} else
			RAISE(OM_ERR_PROP_NOT_PRESENT);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}
/************************
 * Function: GetReferencedObject
 *
 * 	Given an object reference and the information about the object
 *		which contains the reference (NOT the object being referenced),
 *		return an OMF object.
 *
 *		In the case  of an arrayed object reference, the refData may not
 *		be easily derivable from the object & property, hance this routine
 *		takes the refData as a separate argument.
 *
 * Argument Notes:
 *		dataType - The type of the property (ie: OMObjIndex), as
 *						opposed to the type of the data within.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t GetReferencedObject(
			omfHdl_t			file,		/* IN - For this omf file */
			omfObject_t		obj,		/* IN - and this containing object */
			omfProperty_t	prop,		/* IN - in this property */
			omfType_t		dataType,/* IN - of this type */
			CMReference		refData,	/* IN - Given this object reference */
			omfObject_t 	*result)	/* OUT - return the object pointer */
{
	CMValue         val;
	CMProperty      cprop;
	CMType          ctype;
	CMReference     aRef;

	(void) clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);

	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);

	XPROTECT(file)
	{
		val = CMUseValue((CMObject) obj, cprop, ctype);
		if (file->BentoErrorRaised)
			RAISE(OM_ERR_BENTO_PROBLEM);
		memcpy(aRef, refData, sizeof(CMReference));
	
		*result = CMGetReferencedObject(val, aRef);
		if (file->BentoErrorRaised)
			RAISE(OM_ERR_BENTO_PROBLEM);
	}
	XEXCEPT
	XEND

	return (OM_ERR_NONE);
}

/************************
 * Function: GetReferencedObject
 *
 * 	Given an object pointer and the information about the object
 *		which contains the reference (NOT the object being referenced),
 *		return a persistant object reference, suitable for writing into a Bento
 *		peroperty.
 *
 *		In the case  of an arrayed object reference, the location of the
 *		refData may not be easily derivable from the object & property,
 *		hence this routine returns the refData as a separate argument.
 *
 * Argument Notes:
 *		dataType - The type of the property (ie: OMObjIndex), as
 *						opposed to the type of the data within.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t GetReferenceData(
			omfHdl_t			file,		/* IN - For this omf file */
			omfObject_t		obj,		/* IN - and this containing object */
			omfProperty_t	prop,		/* IN - in this property */
			omfType_t		dataType,/* IN - of this type */
			omfObject_t		ref,		/* IN - Given this object */
			CMReference		*result)	/* IN - Return a persistant reference */
{
	CMValue         val;
	CMProperty      cprop;
	CMType          ctype;
	CMReference     aRef;

	(void) clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);

	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);

	XPROTECT(file)
	{
		if (CMCountValues((CMObject) obj, cprop, ctype))
			val = CMUseValue((CMObject) obj, cprop, ctype);
		else
			val = CMNewValue((CMObject) obj, cprop, ctype);
		if (file->BentoErrorRaised)
			RAISE(OM_ERR_BENTO_PROBLEM);
		CMGetReferenceData(val, ref, aRef);
		memcpy(*result, aRef, sizeof(CMReference));
		if (file->BentoErrorRaised)
			RAISE(OM_ERR_BENTO_PROBLEM);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: OMRemoveNthArrayProp
 *
 * 	Remove the indexed element from an arrayed reference.
 *
 * Argument Notes:
 *		dataType - The type of the property (ie: OMObjIndex), as
 *						opposed to the type of the data within.
 *		dataSize - The size of an individual element in the array.
 *		index - Must be in the range of 1 <= index <= arraySize.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t OMRemoveNthArrayProp(
			omfHdl_t			file,			/* IN -- For this omf file */
			omfObject_t		obj,			/* IN -- in this object */
			omfProperty_t	prop,			/* IN -- delete this array index */
			omfType_t		dataType,	/* IN -- Check the type */
			omfInt32			dataSize,	/* IN -- and size */
			const omfInt32	index)		/* IN - The element index */
{
	omfInt16           NElements;
	CMCount         Offset;
	omfBool         Swab;
	CMValue         val;
	CMProperty      cprop;
	CMType          ctype;
	omfInt64			zero;
	omfLength_t		deleteSize;

	omfsCvtInt32toInt64(0, &zero);
	(void) clearBentoErrors(file);
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);

	cprop = CvtPropertyToBento(file, prop);
	ctype = CvtTypeToBento(file, dataType, NULL);
	omfAssert((cprop != NULL), file, OM_ERR_BAD_PROP);
	omfAssert((ctype != NULL), file, OM_ERR_BAD_TYPE);

	XPROTECT(file)
	{
		if (CMCountValues((CMObject) obj, cprop, ctype))
		{
			Swab = ompvtIsForeignByteOrder(file, obj);
			val = CMUseValue((CMObject) obj, cprop, ctype);
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
	
			(void) CMReadValueData(val, &NElements, zero, sizeof(omfInt16));
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
			if (Swab)
				omfsFixShort(&NElements);
	
			if (index > NElements)
				RAISE(OM_ERR_BADINDEX);
	
			NElements--;
			if (Swab)
				omfsFixShort(&NElements);
			CMWriteValueData(val, (CMPtr) & NElements, zero, sizeof(omfInt16));
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
	
			omfsCvtInt32toInt64(2 + ((index - 1) * dataSize), &Offset);
			omfsCvtInt32toInt64(dataSize, &deleteSize);
			CMDeleteValueData(val, Offset, deleteSize);
			if (file->BentoErrorRaised)
				RAISE(OM_ERR_BENTO_PROBLEM);
	
			return (OM_ERR_NONE);
		} else
			RAISE(OM_ERR_PROP_NOT_PRESENT);
	
		if (file->BentoErrorRaised)
			RAISE(OM_ERR_BENTO_PROBLEM);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsFileGetValueAlignment
 *
 * 	Gets the current alignment for the given file.  Used to save and
 *		restore the standard value alignment around an object being writen
 *		at a non-standard aligment.
 *
 * Argument Notes:
 *		valueAlignment - values will be written
 *								to offsets which are multiples of "alignment".
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsFileGetValueAlignment(
			omfHdl_t		file,					/* IN - For this file */
			omfUInt32	*valueAlignment)	/* OUT - return the alignment factor */
{
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);

	*valueAlignment = ((ContainerPtr) file->container)->valueAlignment;
	return (OM_ERR_NONE);
}

/************************
 * Function: omfsFileSetValueAlignment
 *
 * 	Gets the current alignment for the given file.  Used to write an
 *		object at a non-standard aligment.
 *
 * Argument Notes:
 *		valueAlignment - values will be written
 *								to offsets which are multiples of "alignment".
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsFileSetValueAlignment(
			omfHdl_t	file,							/* IN - For this file */
			omfUInt32		valueAlignment)	/* IN - set the alignment factor */
{
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);

	((ContainerPtr) file->container)->valueAlignment = valueAlignment;
	return (OM_ERR_NONE);
}


/************************
 * Function: omfsFixShort		(INTERNAL)
 *
 * 	Byte swap a short value to convert between big-endian and
 *		little-endian formats.
 *
 * Argument Notes:
 *		See argument comments.
 *
 * ReturnValue:
 *		Modifies the value in place
 *
 * Possible Errors:
 *		none
 */
void omfsFixShort(
			omfInt16 * wp)	/* IN/OUT -- Byte swap this value */
{
	register unsigned char *cp = (unsigned char *) wp;
	int             t;

	t = cp[1];
	cp[1] = cp[0];
	cp[0] = t;
}

/************************
 * Function: omfsFixLong		(INTERNAL)
 *
 * 	Byte swap a int  value to convert between big-endian and
 *		little-endian formats.
 *
 * Argument Notes:
 *		See argument comments.
 *
 * ReturnValue:
 *		Modifies the value in place
 *
 * Possible Errors:
 *		none
 */
void omfsFixLong(
			omfInt32 *lp)	/* IN/OUT -- Byte swap this value */
{
	register unsigned char *cp = (unsigned char *) lp;
	int             t;

	t = cp[3];
	cp[3] = cp[0];
	cp[0] = t;
	t = cp[2];
	cp[2] = cp[1];
	cp[1] = t;
}

/************************
 * Function: omfsFixLong64		(INTERNAL)
 *
 * 	Byte swap a int  value to convert between big-endian and
 *		little-endian formats.
 *
 * Argument Notes:
 *		See argument comments.
 *
 * ReturnValue:
 *		Modifies the value in place
 *
 * Possible Errors:
 *		none
 */
void omfsFixLong64(
			omfInt64 *lp)	/* IN/OUT -- Byte swap this value */
{
	register unsigned char *cp = (unsigned char *) lp;
	int             t;

	t = cp[7];
	cp[7] = cp[0];
	cp[0] = t;
	t = cp[6];
	cp[6] = cp[1];
	cp[1] = t;
	t = cp[5];
	cp[5] = cp[2];
	cp[2] = t;
	t = cp[4];
	cp[4] = cp[3];
	cp[3] = t;
}

	/************************************************************
	 *
	 * Toolkit private functions
	 *
	 *************************************************************/

/************************
 * Function: omfsOptMalloc
 *
 * 		Allocates a block of memory of a given size.  This is an optimized
 *			version, working ONLY with omfsOptFree().
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Returns a pointer to a block of memory on the heap, or
 *		NULL if there is no block of that size available.
 *
 * Possible Errors:
 *		NULL -- No memory available.
 */
void *omOptMalloc(
			omfHdl_t	file,			/* IN - For this file */
			size_t	size)			/* Allocate this many bytes */
{
	SessionGlobalDataPtr	bentoGlobals;
	
	if(file != NULL)
	{
		bentoGlobals = (SessionGlobalDataPtr)file->session->BentoSession;
		if((file->fmt == kOmfiMedia) && (file->container != NULL))
			return((*(bentoGlobals->cmMalloc))((file->container), (CMSize32)(size)));
	}
#if PORT_SYS_MAC && !LW_MAC
/* ... don't bothere with FreeMem() here.  NewPtr() reports 0 if it can't allocate the requested memory.
//	if (FreeMem() < size)
//		return (NULL);
*/
	return(NewPtr(size));
#else
	return(malloc(size));
#endif
}

/************************
 * Function: omfsOptFree
 *
 * 	Frees a given block of memory allocated by omfsOptMalloc.  This is an
 *		optimized version, working ONLY with omfsOptMalloc().
 *
 * Argument Notes:
 *		Make sure that the pointer given was really allocated
 *		by omfsOptMalloc, just as for ANSI malloc.
 *
 * ReturnValue:
 *		<none>
 *
 * Possible Errors:
 *		<none known>
 */
void omOptFree(
			omfHdl_t	file,		/* IN - For this file */
			void 		*ptr)		/* Free up this buffer */
{
	SessionGlobalDataPtr	bentoGlobals;
	
	if(file != NULL)
	{
		bentoGlobals = (SessionGlobalDataPtr)file->session->BentoSession;
		if((file->fmt == kOmfiMedia) && (file->container != NULL))
		{
			(*(bentoGlobals->cmFree))((file->container), (CMPtr)(ptr));
			return;
		}
	}

#if PORT_SYS_MAC && !LW_MAC
	DisposePtr((char *)ptr);
#else
	free(ptr);
#endif
}

/************************
 * Function: omfsGetDateTime			(INTERNAL)
 *
 * 	Returns the number of seconds since the standard root date
 *		for the current machine.  The date returned here will be converted
 *		to the canonical date format in the date write routine.
 *
 * Argument Notes:
 *		Time - is NATIVE format.  That is relative to 1/1/1904 for the
 *			Mac and 1/1/70? for everyone else.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
void omfsGetDateTime(omfTimeStamp_t *ptime)
{
#if PORT_SYS_MAC
	time(&(ptime->TimeVal));
	ptime->IsGMT = FALSE;
#elif  PORT_INC_NEEDS_TIMEH
	time((time_t*)&(ptime->TimeVal));
	ptime->IsGMT = FALSE;
#else
	{
		struct timeval  tv;
		struct timezone tz;

		gettimeofday(&tv, &tz);
		ptime->TimeVal = tv.tv_sec;
		ptime->IsGMT = FALSE;
	}
#endif
}

/************************
 * Function: omfsGetBentoID	
 *
 * 	Internal function to return a Bento objectID from an omfObject_t.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		A Bento ID, useful only to pass to Bento.
 *
 * Possible Errors:
 *		Standard assertions.  See top of the file.
 */
omfInt32 omfsGetBentoID(
			omfHdl_t file,		/* IN -- For this omf file */
			omfObject_t obj)	/* IN -- return objectID for this object */
{
	omfAssertValidFHdl(file);
	omfAssertIsOMFI(file);
	omfAssert((obj != NULL), file, OM_ERR_NULLOBJECT);
	return (((TOCObjectPtr) obj)->objectID);
}

/************************
 * Function: CvtPropertyToBento		(INTERNAL)
 *
 * 	Internal function to convert an omfProperty_t value into a CMProperty,
 *		which is a type internal to Bento.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Returns a CMProperty value on success, or NULL if an error was encountered.
 *
 * Possible Errors:
 *		Returns NULL if file handle was NULL, no session was open, or the omfProperty_t code
 *		was out of range.
 */
CMProperty CvtPropertyToBento(
			omfHdl_t file,			/* IN -- For this omf file */
			omfProperty_t prop)	/* IN -- look up this property. */
{
	CMProperty  	cacheResult, result = NULL;
	omfSessionHdl_t session;
	OMPropDef    	def;
	omfBool			found;
	
	clearBentoErrors(file);
	if ((file == NULL) || (file->cookie != FILE_COOKIE))
		return (NULL);
	if (file->fmt != kOmfiMedia)
		return (NULL);

	XPROTECT(file)
	{
		if(file->properties != NULL)
		{
			/* These two properties account for a large percentage of total lookups */
			if(prop == OMObjID)
				return(file->objTagProp1x);
			if(prop == OMOOBJObjClass)
				return(file->objClassProp2x);
				
			CHECK(omfsTablePropertyLookup(file->properties, prop, sizeof(cacheResult), &cacheResult, &found));
			if(found)
				return (cacheResult);
		}
	
		session = file->session;
		if ((session == NULL) || (session->cookie != SESSION_COOKIE))
			return (NULL);
	
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		{
			CHECK(omfsTablePropertyLookup(session->properties1X, prop, sizeof(def), &def, &found));
		}
		else
		{
			CHECK(omfsTablePropertyLookup(session->properties2X, prop, sizeof(def), &def, &found));
			if(!found)
			{
				CHECK(omfsTablePropertyLookup(session->properties1X, prop, sizeof(def), &def, &found));
			}
		}
		
		if(found)
		{
			result = CMRegisterProperty(file->container, def.propName);
			omfCheckBentoRaiseError(file, OM_ERR_BENTO_PROBLEM);
		}
	}
	XEXCEPT
	XEND_SPECIAL(NULL)

	return (result);
}

/************************
 * Function: CvtTypeToBento		(INTERNAL)
 *
 * 	Internal function to convert an omfType_t value into a CMType,
 *		which is a type internal to Bento.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Returns a CMType value on success, or NULL if an error was encountered.
 *
 * Possible Errors:
 *		Returns NULL if file handle was NULL, no session was open, or the omfType_t code
 *		was out of range.
 */
CMType CvtTypeToBento(
			omfHdl_t		file,		/* IN -- For this omf file */
			omfType_t	type,		/* IN -- look up this omfType_t code. */
			omfSwabCheck_t *swab)	/* OUT -- Tell if this needs to be swabbed */
{
	OMTypeCache		cacheResult;
	CMType      	result = NULL;
	omfSessionHdl_t session;
	OMTypeDef  		def;
	omfBool			found;
	omfSwabCheck_t local;

	clearBentoErrors(file);
	if ((file == NULL) || (file->cookie != FILE_COOKIE))
		return (NULL);
	if (file->fmt != kOmfiMedia)
		return (NULL);
	if(swab == NULL)
	  swab = &local;

	XPROTECT(file)
	{
		if(file->types != NULL)
		{
			/* These two types account for a large percentage of total lookups */
			if(type == OMObjectTag)
			{
				*swab = kNeverSwab;
				return(file->objTagType1x);
			}
			if(type == OMClassID)
			{
				*swab = kNeverSwab;
				return(file->objClassType2x);
			}
			
			(void)omfsTableTypeLookup(file->types, type, sizeof(cacheResult), &cacheResult, &found);
			if(found)
			{
				if(swab != NULL)
					*swab = cacheResult.swab;
				return (cacheResult.bentoID);
			}
		}
		
		session = file->session;
		if ((session == NULL) || (session->cookie != SESSION_COOKIE))
			return (NULL);
	
		if ((file->setrev == kOmfRev1x) || file->setrev == kOmfRevIMA)
		{
			(void)omfsTableTypeLookup(session->types1X, type, sizeof(def), &def, &found);
		}
		else
		{
			(void)omfsTableTypeLookup(session->types2X, type, sizeof(def), &def, &found);
		}
		
		if(found)
		{
			if(swab != NULL)
				*swab = def.swabNeeded;
			result = CMRegisterType(file->container, def.typeName);
			omfCheckBentoRaiseError(file, OM_ERR_BENTO_PROBLEM);
		}
	}
	XEXCEPT
	XEND_SPECIAL(NULL)
	
	return (result);
}

/************************
 * Function: CvtPropertyFromBento			(INTERNAL)
 *
 * 	Given a Bento property, return the omProperty_t value.
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
omfProperty_t CvtPropertyFromBento(
			omfHdl_t		file,			/* IN _ In this file, return */
			CMProperty	srchProp,	/* IN - property code for this Bento Prop */
			omfBool		*found)
{
	omfProperty_t	result;	
	
	if(file->propReverse != NULL)
	{
		(void)omfsTableLookupBlock(file->propReverse, &srchProp, sizeof(result), &result, found);
	}
	else
	{
		(void)omfsTableSearchDataValue(file->properties, sizeof(srchProp), &srchProp, sizeof(result), &result, found);
	}
	
	return(*found ? result : OMNoProperty);
}

/************************
 * Function: CvtTypeFromBento			(INTERNAL)
 *
 * 	Given a Bento type, return the omType_t value.
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
omfType_t CvtTypeFromBento(
			omfHdl_t		file,			/* IN _ In this file, return */
			CMType		srchProp,	/* IN - the type code for this Bento Prop */
			omfBool		*found)
{
	omfType_t		result;
	
	if(file->typeReverse != NULL)
	{
		(void)omfsTableLookupBlock(file->typeReverse, &srchProp, sizeof(result), &result, found);
	}
	else
	{
		(void)omfsTableSearchDataValue(file->types, sizeof(srchProp), &srchProp, sizeof(result), &result, found);
	}
	
	return (*found ? result : OMNoType);
}

/************************
 * Function: powi		(INTERNAL)
 *
 * 	Return base ^ exponent.
 *
 * Argument Notes:
 * 	<none>
 *
 * ReturnValue:
 *		The result.
 *
 * Possible Errors:
 *		<none>.
 */
static omfInt32 powi(
			omfInt32	base,
			omfInt32	exponent)
{
	omfInt32           result = 1;
	omfInt32           i = exponent;

	if (exponent == 0)
		return (1);
	else if (exponent > 0)
	{
		while (i--)
			result *= base;
		return (result);
	}
	else
	{
		/* negative exponent not good for integer
		 * exponentiation
		 */
		return (0);
	}
}

/************************
 * Function: ConvertTimeToCanonical	(INTERNAL)
 *
 *		Convert system time to canonical time by subtracting seconds (1/1/70 - 1/1/04)
 *		in the Mac case.
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
void ConvertTimeToCanonical(
			omfUInt32	*time)
{
#if PORT_SYS_MAC && !LW_MAC
    *time &=  0xffffffff;
    *time -= 2082844800;
    *time += 5*3600;
#endif
}

/************************
 * Function: ConvertTimeFromCanonical	(INTERNAL)
 *
 *		Convert canonical time to system time by adding seconds (1/1/70 - 1/1/04)
 *		in the Mac case.
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
void ConvertTimeFromCanonical(
			omfUInt32	*time)
{
#if PORT_SYS_MAC && !LW_MAC
    *time &=  0xffffffff;
    *time += 2082844800;
    *time -= 5*3600;
#endif
}

/************************
 * Function: omfsInit	(INTERNAL)
 *
 * 		Called from BeginSession() to initialize static information
 *
 *		This function initializes read-only variables, and therefore contains a
 *		reference to the only write/read global.  Do not call this function or
 *		BeginSession() from a thread.
 *
 * Argument Notes:
 *		<none>
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions.  See top of the file.
 */
omfErr_t omfsInit(
			void)
{
	if (!InitCalled)
	{
		omfsErrorInit();
	}
	InitCalled = TRUE;

	return (OM_ERR_NONE);
}

/************************
 * Function: clearBentoErrors	(INTERNAL)
 *
 * 		Internal function to reset error information at the start of a toolkit
 *		call to avoid passing back stale information.  Toolkit functions should
 *		immediately return on error without calling other functions to avoid clearing
 *		state.
 *
 * Argument Notes:
 *		See argument comments and assertions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard assertions.  See top of the file.
 */
omfErr_t clearBentoErrors(
			omfHdl_t file)	/* IN -- For this omf file */
{
	if (file != NULL)
	{
		if(file->cookie != FILE_COOKIE)
			return(OM_ERR_BAD_FHDL);
		
		file->BentoErrorNumber = 0;
		file->BentoErrorRaised = FALSE;
		if(file->session != NULL)
		{
			file->session->BentoErrorNumber = 0;
			file->session->BentoErrorRaised = FALSE;
		}
	}

	return (OM_ERR_NONE);
}

/************************
 * Function: omfsIsTypeOf	
 *
 * 	Test if an object is a member of the given class, or a member
 *		of one of its subclasses.
 *
 * Argument Notes:
 *		This routine returns error status through a parameter.  The
 *		variable errRtn may be NULL, but error status will be lost.
 *
 * ReturnValue:
 *		This routine will return FALSE if the object does not belong
 *		to the given class, OR if an error occurred.
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfBool omfsIsTypeOf(
			omfHdl_t				file,		/* IN -- For this omf file */
			omfObject_t			anObject,/* IN -- see if this object */
			omfClassIDPtr_t	aClass,	/* IN -- belongs to this class */
			omfErr_t				*errRtn)		/* OUT -- returning any error here. */
{
	omfClassID_t  thisObjID, superclassID;
	char			*thisIDPtr;
	omfBool         found;
	omfErr_t        status;
	omfProperty_t		idProp;
	
	if (errRtn == NULL)
		errRtn = &status;
	*errRtn = OM_ERR_NONE;
	omfAssertValidFHdlBool(file, errRtn, FALSE);

	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		CHECK(omfsReadClassID(file, anObject, idProp, thisObjID));
		thisIDPtr = thisObjID;
	
		while (strncmp(aClass, thisIDPtr, 4) != 0)
		{
			CHECK(omfsClassFindSuperClass(file, thisIDPtr, superclassID, &found));
			if (!found)
				return (FALSE);
			thisIDPtr = superclassID;
		}
	}
	XEXCEPT
	{
		*errRtn = XCODE();
	}
	XEND_SPECIAL(FALSE)
	
	return (TRUE);
}

/************************
 * Function: omfsClassFindSuperClass
 *
 * 	Given a pointer to a classID, returns the superclass ID
 *		and a boolean indicating if a superclass was found.
 *
 * Argument Notes:
 *		A variable of type omfClassID_t must be provided in order to
 *		hold the returned superclass name.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
omfErr_t omfsClassFindSuperClass(
			omfHdl_t				file,				/* IN -- For this omf file */
			omfClassIDPtr_t	aClass,			/* IN -- lookup this class */
			omfClassIDPtr_t	aSuperclass,	/* IN/OUT -- Copy the superclass ID here */
			omfBool				*foundPtr)		/* OUT -- and tell if something was found */
{
	OMClassDef		def;
	omTable_t		*table;
	
	*foundPtr = FALSE;

	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			table = file->session->classDefs1X;
		else
			table = file->session->classDefs2X;

		CHECK(omfsTableClassIDLookup(table, aClass, sizeof(def), &def, foundPtr));
		if(*foundPtr)
			strncpy(aSuperclass, def.itsSuperClass, 4);
		else 
			strncpy(aSuperclass, "\0", 1);
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

#ifdef VIRTUAL_BENTO_OBJECTS
/*------------------------------------------------------------------------------*
 | omfsPurgeObject - Flush the property & value data if it has not been touched |
 *------------------------------------------------------------------------------*
 
 If an object has not been touched, flush the property & value data.  If the object
 is accessed again, then this information will be reloaded from the file.  This
 function should be called when a program is REASONABLY sure that it will not need
 to access an object again.
*/

omfErr_t omfsObjectPurge(
			omfHdl_t				file,				/* IN -- For this omf file */
			omfObject_t			anObject)		/* IN -- see if this object */
{
	CMPurgeObjectVM((CMObject)anObject);
	return(OM_ERR_NONE);
}
#endif

/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
