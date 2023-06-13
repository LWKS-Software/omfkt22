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
 * Name: omCvt.c
 *
 * Function:
 *		Contains conversion routines, including 64-bit unsigned 
 *              math package, used by the media and composition layers,
 *              and the timecode conversion routines.
 *
 *              The 64-bit math packages are wrapped with a 
 *              #if PORT_USE_NATIVE64 in case someone wants to replace
 *              them with their own platform-dependent versions.  Define
 *				PORT_USE_NATIVE64 as 1 for your platform in portkey.h, and
 *				define PORTKEY_INT64_TYPE to the type name (ie: int  int ).
 *
 *				The code uses the #define 'PORT_USE_NATIVE64'
 *				which is keyed off of PORTKEY_INT64_NATIVE, with the addiition
 *				that the Makefile may define 'OMF_DONT_USE_NATIVE64' in order to disable
 *				64-bit native math on a machine which would otherwise support it
 *				(see the bottom of portvars.h).
 *
 * Audience:
 *		Generally internal to the toolkit, although clients may
 *		use these if needed.
 *
 * Function Summary:
 *      omfsIsInt64Positive(omfInt64)
 *      omfsCvtInt32toLength(in, out)
 *      omfsCvtInt32toPosition(in, out)
 *      omfsCvtInt32toInt64(omfUInt32, omfInt64 *)
 *      omfsAddInt32toInt64(omfUInt32, omfInt64 *)
 *      omfsAddInt64toInt64(omfInt64, omfInt64 *)
 *      omfsSubInt64fromInt64(omfInt64, omfInt64 *)
 *      omfsMultInt32byInt64(omfInt32, omfInt64, omfInt64 *)
 *      omfsTruncInt64toInt32(omfInt64, omfInt32 *)		 OK
 *      omfsTruncInt64toUInt32(omfInt64, omfUInt32 *)	 OK
 *      omfsInt64Less(omfInt64, omfInt64)
 *      omfsInt64Greater(omfInt64, omfInt64)
 *      omfsInt64LessEqual(omfInt64, omfInt64)
 *      omfsInt64GreaterEqual(omfInt64, omfInt64)
 *      omfsInt64Equal(omfInt64, omfInt64)
 *      omfsInt64NotEqual(omfInt64, omfInt64)
 *
 *      omfsTimecodeToString()
 *      omfsStringToTimecode()
 *
 * General error codes returned:
 *		<see below>
 */

#include "masterhd.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h> 

#include "omPublic.h"
#include "omPvt.h" 

/* Moved math.h down here to make NEXT's compiler happy */
#include <math.h>

static omfInt32 nondropTbl[] = { 
  1L, 10L, FPSEC, FPSEC*10, FPMIN, FPMIN*10, FPHR, FPHR*10 };
static omfInt32 dropTbl[] = { 
  1L, 10L, FPSEC, FPSEC*10, DFPMIN, DFP10M, DFPHR, DFPHR*10 };
static omfInt32 PALnondropTbl[] = { 
  1L, 10L, PALFPSEC, PALFPSEC*10, PALFPMIN, PALFPMIN*10, PALFPHR, PALFPHR*10 };
static omfInt32 PALdropTbl[]    = { 
  1L, 10L, PALFPSEC, PALFPSEC*10, PALFPMIN, PALFPMIN*10, PALFPHR, PALFPHR*10 };
/* End of Timecode definitions */

omfErr_t omfsMakeInt64(omfInt32 high, omfInt32 low, omfInt64 *out)
{
	if(out != NULL)
	{
#if PORT_USE_NATIVE64
	  *out = ((PORTKEY_INT64_TYPE)high << (PORTKEY_INT64_TYPE)32L) | (PORTKEY_INT64_TYPE)low;
#else
		out->words[0] = (omfUInt16)((high >> 16L) & 0xFFFF);
		out->words[1] = (omfUInt16)(high & 0xFFFF);
		out->words[2] = (omfUInt16)((low >> 16L) & 0xFFFF);
		out->words[3] = (omfUInt16)(low & 0xFFFF);
#endif
		return(OM_ERR_NONE);
	}
	else
		return(OM_ERR_NULL_PARAM);
}

omfErr_t omfsDecomposeInt64(omfInt64 in, omfInt32 *high, omfInt32 *low)
{
#if PORT_USE_NATIVE64
	if(high != NULL)
		*high = (omfInt32)((in >> (PORTKEY_INT64_TYPE)32L) & (PORTKEY_INT64_TYPE)0xFFFFFFFF);
	if(low != NULL)
		*low = (omfInt32)(in & 0xFFFFFFFF);
#else
	if(high != NULL)
		*high = ((omfUInt32)in.words[0] << 16L) | in.words[1];
	if(low != NULL)
		*low = ((omfUInt32)in.words[2] << 16L) | in.words[3];
#endif

	return(OM_ERR_NONE);
}

/************************
 * Function: omfsCvtInt32toInt64
 *
 * 		Initialized a 64-bit integer with a value from an omfUInt32.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).  The error code in the opaque handle
 *		is not updated.
 *
 * Possible Errors:
 * 		OM_ERR_NULL_PARAM -- A NULL result pointer.
 */
#if !PORT_USE_NATIVE64
omfErr_t omfsCvtInt32toInt64(
			omfInt32		in,		/* IN -- Convert this omfInt32 */
			omfInt64		*out)	/* OUT -- into this omfInt64 */
{
	if(out != NULL)
	{
		if(in < 0)
		{
			out->words[0] = 0xFFFF;
			out->words[1] = 0xFFFF;
		}
		else
		{
			out->words[0] = 0;
			out->words[1] = 0;
		}
		out->words[2] = (omfUInt16)((in >> 16L) & 0xFFFF);
		out->words[3] = (omfUInt16)(in & 0xFFFF);
		return(OM_ERR_NONE);
	}
	else
		return(OM_ERR_NULL_PARAM);
}

omfErr_t omfsCvtUInt32toInt64(
			omfUInt32		in,		/* IN -- Convert this omfInt32 */
			omfInt64		*out)	/* OUT -- into this omfInt64 */
{
	if(out != NULL)
	{
		out->words[0] = 0;
		out->words[1] = 0;
		out->words[2] = (omfUInt16)((in >> 16L) & 0xFFFF);
		out->words[3] = (omfUInt16)(in & 0xFFFF);
		return(OM_ERR_NONE);
	}
	else
		return(OM_ERR_NULL_PARAM);
}
#endif

/************************
 * Function: omfsAddInt32toInt64
 *
 * 		Adds a omfUInt32 value to a omfInt64 value.
 *
 * Argument Notes:
 *		That the second parameter is overwritten with the result.
 *
 * ReturnValue:
 *		Error code (see below).  The error code in the opaque handle
 *		is not updated.
 *
 * Possible Errors:
 *		OM_ERR_OVERFLOW64 - Arithmetic overflow.
 * 		OM_ERR_NULL_PARAM -- A NULL result pointer.
 */
omfErr_t omfsAddInt32toInt64(
			omfUInt32	in,			/* IN - Add this omfUInt32 to */
			omfInt64	*out)		/* IN/OUT - this existing omfInt64 */
{
#if !PORT_USE_NATIVE64
	omfInt64          src;
#endif

	if(out == NULL)
		return(OM_ERR_NULL_PARAM);
		
#if PORT_USE_NATIVE64
	*out += in;
	return(OM_ERR_NONE);
#else
	omfsCvtInt32toInt64(in, &src);
	return (omfsAddInt64toInt64(src, out));
#endif
}

/************************
 * Function: omfsAddInt64toInt64
 *
 * 		Adds a omfInt64 value to a omfInt64 value.
 *
 * Argument Notes:
 *		That the second parameter is overwritten with the result.
 *
 * ReturnValue:
 *		Error code (see below).  The error code in the opaque handle
 *		is not updated.
 *
 * Possible Errors:
 *		OM_ERR_OVERFLOW64 - Arithmetic overflow.
 * 		OM_ERR_NULL_PARAM -- A NULL result pointer.
 */
omfErr_t omfsAddInt64toInt64(
			omfInt64	in,		/* IN - Add this omfInt64 to */
			omfInt64	*out)		/* IN/OUT - this existing omfInt64 */
{
#if !PORT_USE_NATIVE64
	omfUInt32          temp, carry;
	omfInt32			n, inSign, in2Sign, outSign;
#endif

	if(out == NULL)
		return(OM_ERR_NULL_PARAM);

#if PORT_USE_NATIVE64
	*out += in;
	return(OM_ERR_NONE);
#else
	inSign = in.words[0] & 0x8000;
	in2Sign = out->words[0] & 0x8000;
	for (n = 3, carry = 0; n >= 0; n--)
	{
		temp = (omfUInt32)in.words[n] + (omfUInt32)out->words[n] + carry;
		out->words[n] = (omfUInt16)(temp & 0xFFFF);
		carry = (temp >> 16L) & 0xFFFF;
	}
	outSign = out->words[0] & 0x8000;

	if((inSign == in2Sign) && (outSign != in2Sign))
		return (OM_ERR_OVERFLOW64);
	else
		return (OM_ERR_NONE);
#endif
}

/************************
 * Function: omfsSubInt64fromInt64
 *
 * 		Subtracts a omfInt64 value (in) from an existing omfInt64 value
 *		(out), returning the result in "out".
 *
 * Argument Notes:
 *		The second parameter is overwritten with the result.
 *
 * ReturnValue:
 *		Error code (see below).  The error code in the opaque handle
 *		is not updated.
 *
 * Possible Errors:
 *		OM_ERR_INTERNAL_NEG64 - Result would be negative.
 * 		OM_ERR_NULL_PARAM -- A NULL result pointer.
 */
omfErr_t omfsSubInt64fromInt64(
			omfInt64	in,		/* IN - Subtract this omfInt64 from */
			omfInt64 *out)		/* IN/OUT - this existing omfInt64 */
{
#if !PORT_USE_NATIVE64
	omfInt32		temp;
	omfInt32		borrow;
	omfInt16		j;
	omfInt32		inSign, in2Sign, outSign;
#endif

	if(out == NULL)
		return(OM_ERR_NULL_PARAM);

#if PORT_USE_NATIVE64
	*out -= in;
	return(OM_ERR_NONE);
#else
	inSign = in.words[0] & 0x8000;
	in2Sign = out->words[0] & 0x8000;
	for (j = 3, borrow = 0; j >= 0; j--)
	{
		temp = ((omfInt32)out->words[j] - (omfInt32)in.words[j]) - borrow;
		if (temp < 0)
			borrow = 1;
		else
			borrow = 0;
		out->words[j] = temp & 0xFFFF;
	}

	outSign = out->words[0] & 0x8000;
	if((inSign != in2Sign) && (outSign != in2Sign))
		return (OM_ERR_OVERFLOW64);
	else
		return (OM_ERR_NONE);
#endif
}

omfErr_t omfsSubInt32fromInt64(
			omfInt32	in,		/* IN - Subtract this omfInt64 from */
			omfInt64 *out)		/* IN/OUT - this existing omfInt64 */
{
#if PORT_USE_NATIVE64
	*out -= in;
	return(OM_ERR_NONE);
#else
	omfInt64		largeIn;

	omfsCvtInt32toInt64(in, &largeIn);
	return(omfsSubInt64fromInt64(largeIn, out));
#endif
}


static void NegateInt64(omfInt64 *out)
{
#if PORT_USE_NATIVE64
	*out = -1 * (*out);
#else
	out->words[0] = ~out->words[0];
	out->words[1] = ~out->words[1];
	out->words[2] = ~out->words[2];
	out->words[3] = ~out->words[3];
	omfsAddInt32toInt64(1, out);
#endif
}

/************************
 * Function: omfsMultInt32byInt64
 *
 * 		Multiplies "in" * "in2", and puts the result in the variable
 *		pointed to by "out".
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).  The error code in the opaque handle
 *		is not updated.
 *
 * Possible Errors:
 *		OM_ERR_OVERFLOW64 - Arithmetic overflow.
 * 		OM_ERR_NULL_PARAM -- A NULL result pointer.
 */
omfErr_t omfsMultInt32byInt64(
			omfInt32	in,	/* IN - Multiply this omfInt32 */
			omfInt64		in2,	/* IN - by this omfInt64 */
			omfInt64		*out)	/* OUT - and put the result here. */
{
#if !PORT_USE_NATIVE64
	omfInt32           i, j, temp, carry, n;
	omfInt64          in1;
	omfUInt16			result[8];		/* Must be twice as large as either input */
	omfInt32			inSign, in2Sign;
#endif
	
	if(out == NULL)
		return(OM_ERR_NULL_PARAM);

#if PORT_USE_NATIVE64
	*out = ((int )in) * in2;
	return(OM_ERR_NONE);
#else
	omfsCvtInt32toInt64(in, &in1);
	inSign = in1.words[0] & 0x8000;
	in2Sign = in2.words[0] & 0x8000;
	if(inSign != 0)
		NegateInt64(&in1);
	if(in2Sign != 0)
		NegateInt64(&in2);

	for(n = 0; n < sizeof(result)/sizeof(omfUInt16); n++)
		result[n] = 0;
		
	for (j = 3; j >= 0; j--)
	{
		if (in2.words[j] == 0)
			result[j] = 0;
		else
		{
			for (i = 3, carry = 0; i >= 0; i--)
			{
				/* i+j+1 is because we want i+j from a 1-based array
				 * +1 each for i & j to convert to 1-based array, 
				 * -1 to convert back to zero-based array
				 */
				 temp = (omfInt32)in1.words[i] * (omfInt32)in2.words[j] +
						result[i+j+1] + carry;
				result[i+j+1] = temp & 0xFFFF;
				carry = (temp >> 16L) & 0xFFFF;
			}
			result[j] = (omfUInt16)carry;
		}
	}

	for(n = 4; n < sizeof(result)/sizeof(omfUInt16); n++)
		out->words[n-4] = result[n];
	if(inSign != in2Sign)
	{
		NegateInt64(out);
	}
	
	if(result[0] == 0 && result[1] == 0 && result[2] == 0 && result[3] == 0)
		return(OM_ERR_NONE);
	else
		return(OM_ERR_OVERFLOW64);
#endif
}

#if !PORT_USE_NATIVE64
static omfErr_t shiftRight(omfInt64 *in)
{
	omfInt32	carry0, carry1, carry2;
	
	carry0 = in->words[0] & 0x01;
	carry1 = in->words[1] & 0x01;
	carry2 = in->words[2] & 0x01;
	in->words[0] >>= 1;
	in->words[1] = (omfUInt16)((in->words[1] >> 1) | (carry0 << 15L));
	in->words[2] = (omfUInt16)((in->words[2] >> 1) | (carry1 << 15L));
	in->words[3] = (omfUInt16)((in->words[3] >> 1) | (carry2 << 15L));
	return(OM_ERR_NONE);
}
#endif

omfErr_t omfsDivideInt64byInt32(
		       omfInt64		in, 		/* IN - Divide this omfInt64 */
		       omfUInt32	in2,		/* IN - by this omfInt32 */
		       omfInt64		*result,	/* OUT - and put the result here. */
		       omfInt32		*remainder)	/* OUT - and put the remainder here. */
{
#if !PORT_USE_NATIVE64
	omfInt64	num64, den64, localResult, tmp, tmp2;
	omfInt32	j, num32, den, test;
	omfInt32	inSign;
#endif
	
	XPROTECT(NULL)
	{
		XASSERT(in2 != 0, OM_ERR_ZERO_DIVIDE);
#if PORT_USE_NATIVE64
		if(result != NULL)
			*result = in / ((int )in2);
		if(remainder != NULL)
			*remainder = (omfInt32)(in % ((int )in2));
#else
		if(result == NULL)
			result = &localResult;

		omfsDecomposeInt64(in, &test, &num32);
		if(test == 0)	/* fits in 32-bit number */
		{
			omfsCvtInt32toInt64((omfUInt32)num32 / in2, result);
			if(remainder != NULL)
			  *remainder = (omfUInt32)num32 % in2;
		}
		else
		{
			inSign = in.words[0] & 0x8000;
			if(inSign != 0)
				NegateInt64(&in);

			omfsCvtInt32toInt64(0, result);
		
			for (j = 0, den = in2; (den & 0x80000000) == 0; j++)
				den <<= 1;
			CHECK(omfsMakeInt64(den, 0, &den64));
			j += 32;
			num64 = in;
			
			do
			{
				XASSERT (j >= 0, OM_ERR_INTERNAL_DIVIDE);
				omfsAddInt64toInt64(*result, result);
				if (omfsInt64GreaterEqual(num64, den64))
				{
					CHECK(omfsAddInt32toInt64(1, result));
					CHECK(omfsSubInt64fromInt64(den64, &num64));
					XASSERT(omfsInt64Less(num64, den64), OM_ERR_INTERNAL_DIVIDE);
				}
				CHECK(shiftRight(&den64));
			}
			while (j--);
	
			if(remainder != NULL)
			{
				CHECK(omfsMultInt32byInt64(in2, *result, &tmp));
				tmp2 = in;
				CHECK(omfsSubInt64fromInt64(tmp, &tmp2));
				CHECK(omfsTruncInt64toInt32(tmp2, remainder));	/* OK (remainder must be < divisor) */
			}
			if(inSign != 0)
			{
				NegateInt64(result);
			}
		}
#endif
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function: omfsTruncInt64toInt32/omfsTruncInt64toUInt32	OK
 *
 * 	Truncate a 64-bit integer to a 32-bit value, and tell the
 *		caller whether siginifcant digits were truncated
 *		(TRUE == no significant truncation).
 *
 *		Some formats have 32-bit limits on offsets and lengths, and
 *		so would use this to truncate 64-bit offsets and lengths.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).  The error code in the opaque handle
 *		is not updated.
 *
 * Possible Errors:
 *		OM_ERR_OFFSET_SIZE -- Truncation occurred.
 * 		OM_ERR_NULL_PARAM -- A NULL result pointer.
 */
omfErr_t omfsTruncInt64toInt32(				/* OK */
			omfInt64		in,		/* IN - Truncate this value */
			omfInt32		*out)		/* OUT - to 32-bits and put the result here */
{
	if(out == NULL)
		return(OM_ERR_NULL_PARAM);

#if PORT_USE_NATIVE64
	*out = (omfInt32)(in & 0xFFFFFFFF);
	if(*out != in)
	{
		if((omfInt32)(-in & 0xFFFFFFFF) != *out)
			return (OM_ERR_OFFSET_SIZE);
	}
	return(OM_ERR_NONE);
#else
	if ((in.words[0] == 0) && (in.words[1] == 0) && ((omfInt16)in.words[2] >= 0))
	{
		*out = ((omfInt32) (in.words[2] & 0x7FFF) << 16L) | (in.words[3]);
		return (OM_ERR_NONE);
	}
	else if ((in.words[0] == 0xFFFF) && (in.words[1] == 0xFFFF) && ((omfInt16)in.words[2] <= 0))
	{
		*out = ((omfInt32) in.words[2] << 16L) | (in.words[3]);
		return (OM_ERR_NONE);
	}
	else
		return (OM_ERR_OFFSET_SIZE);
#endif
}

omfErr_t omfsTruncInt64toUInt32(				/* OK */
			omfInt64		in,		/* IN - Truncate this value */
			omfUInt32	*out)		/* OUT - to 32-bits and put the result here */
{
	if(out == NULL)
		return(OM_ERR_NULL_PARAM);

#if PORT_USE_NATIVE64
	*out = (omfUInt32)(in & 0xFFFFFFFF);
	if(*out != in)
		return (OM_ERR_OFFSET_SIZE);
	return(OM_ERR_NONE);
#else
	if ((in.words[0] == 0) && (in.words[1] == 0))
	{
		*out = ((omfUInt32) in.words[2] << 16L) | (in.words[3]);
		return (OM_ERR_NONE);
	} else
		return (OM_ERR_OFFSET_SIZE);
#endif
}

omfErr_t omfsInt64AndInt64(
			omfInt64	in,		/* IN - Subtract this omfInt64 from */
			omfInt64 *out)		/* IN/OUT - this existing omfInt64 */
{
#if !PORT_USE_NATIVE64
	omfInt16		j;
#endif

	if(out == NULL)
		return(OM_ERR_NULL_PARAM);

#if PORT_USE_NATIVE64
	*out &= in;
	return(OM_ERR_NONE);
#else
	for (j = 3; j >= 0; j--)
	{
		out->words[j] &= in.words[j];
	}
	return (OM_ERR_NONE);
#endif
}

/************************
 * Function: omfsInt64ToString
 *
 * 		64-bit to string conversions.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		OM_ERR_NULL_PARAM - Buffer pointer is NULL.
 *		OM_ERR_NOT_IMPLEMENTED - Currently only handles bases 10 & 16.
 */
omfErr_t omfsInt64ToString(
			omfInt64	in,			/* IN - Print this number */
			omfInt16	base,		/* IN - in this numerical base */
			omfInt16	bufSize,	/* IN - into a buffer of this length */
			char 		*buf)		/* IN/OUT - here is the buffer. */
{
	omfInt64		workval, zero;
	char			tmpBuf[64];
	omfInt32		numDigits, remainder, src, dest;
	omfBool			negative = FALSE;
	
	if(buf == NULL)
		return(OM_ERR_NULL_PARAM);

	XPROTECT(NULL)
	{
		omfsCvtInt32toInt64(0, &zero);
		numDigits = 0;
		workval = in;
		if(omfsInt64Equal(workval, zero))
		{
			buf[0] = '0';
			buf[1] = '\0';
		}
		else
		{
			if(omfsInt64Less(workval, zero))
			{
				NegateInt64(&workval);
				negative = TRUE;
			}
			while(omfsInt64Greater(workval, zero))
			{
				CHECK(omfsDivideInt64byInt32(workval, base, &workval, &remainder));
				tmpBuf[numDigits++] = remainder + '0';
			}
			if(negative)
				tmpBuf[numDigits++] = '-';
			if(numDigits < bufSize)		/* Allow one byte for '\0' */
			{
				for(src = numDigits-1, dest = 0; src >= 0; src--, dest++)
					buf[dest] = tmpBuf[src];
				buf[numDigits] = '\0';
			}
			else
			{
				strncpy(buf, "*err*", bufSize-1);
				buf[bufSize-1] = '\0';
			}
		}
	}
	XEXCEPT
	XEND
	
	return(OM_ERR_NONE);
}

/************************
 * Function:omfsInt64Less
 * Function: omfsInt64Greater
 * Function: omfsInt64LessEqual
 * Function: omfsInt64GreaterEqual
 * Function: omfsInt64Equal
 * Function: omfsInt64NotEqual
 *
 * 		64-bit comparison tests.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Boolean TRUE if the comparison is true (ie: omfsInt64Less(val2, val3)).
 *
 * Possible Errors:
 *		<none>
 */
#if !PORT_USE_NATIVE64
omfInt16 compare64(		/* Returns -1 if a<b, 0 if a == b, 1 if a > b) */
			omfInt64	a,
			omfInt64 b)
{
	omfInt16	n, signA, signB;
	omfInt32	extendA, extendB;
	
	signA = ((omfInt16)a.words[0] < 0 ? -1 : 1);
	signB = ((omfInt16)b.words[0] < 0 ? -1 : 1);
	if(signA < signB)
		return(-1);
	if(signB > signA)
		return(1);
	for (n = 0; n <= 3; n++)
	{
		extendA = a.words[n];
		extendB = b.words[n];
		if(extendA < extendB)
			return(-1);
		else if(extendA > extendB)
			return(1);
	}
	return(0);
}

/*****/
omfBool omfsInt64Equal(
			omfInt64 a,		/* IN - Is a == b */
			omfInt64 b)
{
	omfInt16	n;

	for(n = 0; n <= 3; n++)
	{
		if(a.words[n] != b.words[n])
			return FALSE;
	}
	return(TRUE);
}

/*****/
omfBool omfsInt64NotEqual(
			omfInt64 a,		/* IN - Is a != b */
			omfInt64 b)
{
	return(!omfsInt64Equal(a, b));
}
#endif

#ifdef OMFI_SELF_TEST
/************************
 * Function: testUint64
 *
 * 		WhatItDoes
 *
 * Argument Notes:
 *		StuffNeededBeyondNotesInDefinition.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
#include <stdlib.h>
#if PORT_USE_NATIVE64
	static omfInt64 test4 = 	0x4444444444444444;
	static omfInt64 test3a =	0x4000000000000000;
	static omfInt64 test3b =	0x0CCCCCCCCCCCCCCD;
	static omfInt64 test3 =		0x3333333333333333;
	static omfInt64 test2 =		0x2222222222222222;
	static omfInt64 test1 =		0x1111111111111111;
	static omfInt64 testF =		0x7FFFFFFFFFFFFFFF;
	static omfInt64 test0 =		0x0000000000000000;
	static omfInt64 testNeg1 =	0xFFFFFFFFFFFFFFFF;
	static omfInt64 testNeg2 =	0xFFFFFFFFFFFFFFFE;
	static omfInt64 test1p1 = 	0x1111111111111112;
	static omfInt64 test1m1 =	0x1111111111111110;
	static omfInt64 testRoll =	0x0000000080000000;
	static omfInt64 testRoll2 =	0x0000000100000000;
	static omfInt64 test1Neg =	0xEEEEEEEEEEEEEEEF;
	static omfInt64 test2Neg =	0xDDDDDDDDDDDDDDDE;
	static omfInt64 testJust31 = 0x000000000000001F;
#else
	static omfInt64 test4 = { 0x4444, 0x4444, 0x4444, 0x4444 };
	static omfInt64 test3a = { 0x4000, 0x0000, 0x0000, 0x0000 };
	static omfInt64 test3b = { 0x0CCC, 0xCCCC, 0xCCCC, 0xCCCD };
	static omfInt64 test3 = { 0x3333, 0x3333, 0x3333, 0x3333 };
	static omfInt64 test2 = { 0x2222, 0x2222, 0x2222, 0x2222 };
	static omfInt64 test1 = { 0x1111, 0x1111, 0x1111, 0x1111 };
	static omfInt64 testF = { 0x7FFF, 0xFFFF, 0xFFFF, 0xFFFF };
	static omfInt64 test0 = { 0x0000, 0x0000, 0x0000, 0x0000 };
	static omfInt64 testNeg1 = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
	static omfInt64 testNeg2 = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFE };
	static omfInt64 test1p1 = { 0x1111, 0x1111, 0x1111, 0x1112 };
	static omfInt64 test1m1 = { 0x1111, 0x1111, 0x1111, 0x1110 };
	static omfInt64 testRoll = { 0x0000, 0x0000, 0x8000, 0x0000 };
	static omfInt64 testRoll2 = { 0x0000, 0x0001, 0x0000, 0x0000 };
	static omfInt64 test1Neg = { 0xEEEE, 0xEEEE, 0xEEEE, 0xEEEF };
	static omfInt64 test2Neg = { 0xDDDD, 0xDDDD, 0xDDDD, 0xDDDE };
	static omfInt64 testJust31 = { 0x0000, 0x0000, 0x0000, 0x001F };
#endif
		
#define testErr(msg) { printf(msg); exit(1); }
	
void testUint64(void)
{
	omfInt64	tmp, result, testPrint;
	omfInt32	remainder;
	char		buf[16];
	
	/********************************************/
	/***** Check some simple add conditions *****/
	/* Both #'s positive */
	tmp = test1;
	if(omfsAddInt64toInt64(test2, &tmp) != OM_ERR_NONE)
		testErr("failed add (error).\n");
	if(omfsInt64NotEqual(tmp, test3))
		testErr("failed add value.\n");

	/* Positive + negative */
	tmp = test1;
	if(omfsAddInt64toInt64(testNeg1, &tmp) != OM_ERR_NONE)
		testErr("failed add positive + negative (error).\n");
	if(omfsInt64NotEqual(tmp, test1m1))
		testErr("failed add positive + negative value.\n");

	/* Negative + negative */
	tmp = testNeg1;
	if(omfsAddInt64toInt64(testNeg1, &tmp) != OM_ERR_NONE)
		testErr("failed add negative + negative (error).\n");
	if(omfsInt64NotEqual(tmp, testNeg2))
		testErr("failed add negative + negative value.\n");

	/* Check for expected overflow */
/* !!!	tmp = test1;
	if(omfsAddInt64toInt64(testF, &tmp) != OM_ERR_OVERFLOW64)
		testErr("missing expected overflow.\n");
*/	
	/*************************************************/
	/***** Check some simple subtract conditions *****/
	/* (pos - pos = pos) */
	tmp = test4;
	if(omfsSubInt64fromInt64(test3, &tmp) != OM_ERR_NONE)
		testErr("failed subtract (pos - pos = pos) (error).\n");
	if(omfsInt64NotEqual(tmp, test1))
		testErr("failed subtract (pos - pos = pos) value.\n");

	/* (pos - pos = neg) */
	tmp = test1;
	if(omfsSubInt64fromInt64(test1p1, &tmp) != OM_ERR_NONE)
		testErr("failed subtract (pos - pos = neg) (error).\n");
	if(omfsInt64NotEqual(tmp, testNeg1))
		testErr("failed subtract (pos - pos = neg) value.\n");

	/* (pos - neg = pos) */
	tmp = test1;
	if(omfsSubInt64fromInt64(testNeg1, &tmp) != OM_ERR_NONE)
		testErr("failed subtract (pos - neg = pos) (error).\n");
	if(omfsInt64NotEqual(tmp, test1p1))
		testErr("failed subtract (pos - neg = pos) value.\n");

	/* (pos - pos = zero) */
	tmp = test1;
	if(omfsSubInt64fromInt64(test1, &tmp) != OM_ERR_NONE)
		testErr("failed subtract (error).\n");
	if(omfsInt64NotEqual(tmp, test0))
		testErr("failed subtract value.\n");

	/* (neg - neg = zero) */
	tmp = testNeg1;
	if(omfsSubInt64fromInt64(testNeg1, &tmp) != OM_ERR_NONE)
		testErr("failed subtract (neg - neg = zero) (error).\n");
	if(omfsInt64NotEqual(tmp, test0))
		testErr("failed subtract (neg - neg = zero) value.\n");
	
	/* Check for borrow without error */
	tmp = test3a;
	if(omfsSubInt64fromInt64(test3, &tmp) != OM_ERR_NONE)
		testErr("failed borrow without error (error).\n");
	if(omfsInt64NotEqual(tmp, test3b))
		testErr("failed borrow without error (value).\n");
			
	/********************************************/
	/* Check some simple multiply conditions */
	/* (pos * pos = pos) */
	if(omfsMultInt32byInt64(2, test2, &tmp) != OM_ERR_NONE)
		testErr("failed simple multiply (error).\n");
	if(omfsInt64NotEqual(tmp, test4))
		testErr("failed simple multiply (value).\n");

	/* (pos * neg = neg) */
	if(omfsMultInt32byInt64(2, testNeg1, &tmp) != OM_ERR_NONE)
		testErr("failed (pos * neg = neg) multiply (error).\n");
	if(omfsInt64NotEqual(tmp, testNeg2))
		testErr("failed (pos * neg = neg) multiply (value).\n");

	/* (neg * pos = neg) */
	if(omfsMultInt32byInt64(-1, test2, &tmp) != OM_ERR_NONE)
		testErr("failed (neg * pos = neg) multiply (error).\n");
	if(omfsInt64NotEqual(tmp, test2Neg))
		testErr("failed (neg * pos = neg) multiply (value).\n");
	
	/* (neg * neg = pos) */
	if(omfsMultInt32byInt64(-2, test2Neg, &tmp) != OM_ERR_NONE)
		testErr("failed (neg * neg = pos) multiply (error).\n");
	if(omfsInt64NotEqual(tmp, test4))
		testErr("failed (neg * neg = pos) multiply (value).\n");
	
	/* Test 31-bit rollover */
	if(omfsMultInt32byInt64(2, testRoll, &tmp) != OM_ERR_NONE)
		testErr("failed multiply 31 bit rollover (error).\n");
	if(omfsInt64NotEqual(tmp, testRoll2))
		testErr("failed multiply 31 bit rollover (value).\n");

	/********************************************/
	/* Check some simple divide conditions */
	/* (pos / pos = pos) */
	if(omfsDivideInt64byInt32(test4, 2, &result, &remainder))
		testErr("failed (pos / pos = pos) divide (error).\n");
	if(omfsInt64NotEqual(result, test2) || (remainder != 0))
		testErr("failed (pos / pos = pos) divide (value).\n");

	/* (neg / pos = neg) */
	if(omfsDivideInt64byInt32(test2Neg, 2, &result, &remainder))
		testErr("failed (neg / pos = neg) divide (error).\n");
	if(omfsInt64NotEqual(result, test1Neg) || (remainder != 0))
		testErr("failed (neg / pos = neg) divide (value).\n");

	/* Positive remainder */
	if(omfsDivideInt64byInt32(test1, 32, &result, &remainder))
		testErr("failed (neg / neg = pos) divide (error).\n");
	if(remainder != 17)
		testErr("failed (neg / neg = pos) divide (value).\n");
		
	omfsCvtInt32toInt64(0x12345678, &testPrint);			
	if(omfsInt64ToString(testPrint, 16, sizeof(buf), buf))
		testErr("failed base 16 print (error).\n");
	printf("Printing 0x12345678 in hexadecimal = %s\n", buf);
	if(strcmp(buf, "12345678") != 0)
		testErr("failed base 16 print (value).\n");
	if(omfsInt64ToString(testPrint, 10, sizeof(buf), buf))
		testErr("failed base 16 print).\n");
	printf("Printing 0x12345678 in decimal (should be 305419896) = %s\n", buf);
	if(strcmp(buf, "305419896") != 0)
		testErr("failed base 16 print).\n");
}

#endif


/*************************************************************************
 * Function: omfsTimecodeToString()/omfsStringToTimecode()
 *
 *      omfsTimecodeToString converts a timecode value to a string
 *      representation of the timecode.  omfsStringToTimecode converts 
 *      a string representation of timecode to a timecode value.
 *      Drop and non-drop are distinguished with the delimiters in
 *      the string: drop uses a ';' and non-drop uses a ':'.
 *      These functions do not take a file handle as an argument.
 *
 *      These function support both 1.x and 2.x files.
 *
 * Argument Notes:
 *      omfsTimecodeToString() assumes that an already allocated character
 *      array is passed in as tcString.  The length of the array is
 *      in strLen.  The function will return an error (OM_ERR_INTERN_TOO_SMALL)
 *      if strLen is too small to hold the string representation of the
 *      timecode.
 *
 *      omfsStringToTimecode() also takes a frameRate argument.  It will
 *      calculate the given timecode taking the rate into account.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *************************************************************************/
omfErr_t omfsTimecodeToString(
	omfTimecode_t timeCode,   /* IN - Timecode Value */
	omfInt32 strLen,          /* IN - Length of string to hold timecode */
	omfString tcString)       /* IN/OUT - Pre-allocated buffer to hold
							   *          timecode string
							   */
{
  register int i, ten;
  omfInt16 hours, minutes, seconds, frames;
  omfErr_t omfError = OM_ERR_NONE;

  XPROTECT(NULL)
	{
	  if ((tcString == NULL) || (strLen < 12))
		{
		  RAISE(OM_ERR_INTERN_TOO_SMALL);
		}

	  if (timeCode.drop == kTcDrop)
		strcpy(tcString, "00;00;00;00");
	  else 
		strcpy(tcString, "00:00:00:00");

	  CHECK(OffsetToTimecode(timeCode.startFrame, timeCode.fps, timeCode.drop, &hours, 
							  &minutes, &seconds, &frames));

	  ten = 10;
	  i = hours;
	  if (i > 0)
		{
		  tcString[0] += i/ten;
		  tcString[1] += i%ten;
		}
	  i = minutes;
	  if (i > 0)
		{
		  tcString[3] += i/ten;
		  tcString[4] += i%ten;
		}
	  i = seconds;
	  if (i > 0)
		{
		  tcString[6] += i/ten;
		  tcString[7] += i%ten;
		}
	  i = frames;
	  if (i > 0)
		{
		  tcString[9] += i/ten;
		  tcString[10] += i%ten;
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
 * Private Function: roundFrameRate()
 *************************************************************************/
static omfInt32 roundFrameRate(omfRational_t frameRate)
{
  omfInt32 intRate;
  double tmpFloatRate, floatRate;

  tmpFloatRate = FloatFromRational(frameRate);
  floatRate = (float)CEIL(tmpFloatRate);  /* To make 29.97 into 30 */

  if (floatRate == 30.0)
	intRate = 30;
  else if (floatRate == 29.97) /* For timecode, 29.97 should be 30 */
	intRate = 30;
  else if (floatRate == 25.0)
	intRate = 25;
  else if (floatRate == 24.0)
	intRate = 24;
  else
	intRate = 0;

  return(intRate);
}

/*************************************************************************
 * Function: omfsStringToTimecode()
 *************************************************************************/
omfErr_t omfsStringToTimecode(
	omfString timecodeString, /* IN - Timecode String */
	omfRational_t frameRate,  /* IN - Frame Rate */
	omfTimecode_t *timecode)  /* OUT - Timecode Value */
{
  register char *c;
  omfInt32 *multiplier, total = 0;
  omfBool drop;
  omfInt32 len, k = 0;
  omfInt32 intFRate;
  char tcString[36];

  XPROTECT(NULL)
	{
	  len = strlen(timecodeString);

	  if (len == 0 || len > 12)
		{
		  RAISE(OM_ERR_INVALID_TIMECODE);
		}

	  strncpy(tcString, timecodeString, len);
	  tcString[len] = '\0';
	  intFRate = roundFrameRate(frameRate);
	  if (intFRate == 0)
		{
		  RAISE(OM_ERR_INVALID_TIMECODE);
		}

	  /* Prescan for drop/nondrop */
	  drop = kTcNonDrop;
	  for (c = &tcString[len-1]; c >= tcString; c--)
		if (*c == ';')
		  {
			drop = kTcDrop;
			break;
		  }

	  multiplier = (drop ? dropTbl : nondropTbl);
	  if (intFRate == 25)
		{
		  multiplier = PALnondropTbl;
		  drop = kTcNonDrop;
		}

	  for (c = &tcString[len-1]; c >= tcString; c--)
		{
		  if (isdigit(*c))
			{
			  /* If start of a minute which is not a multiple of 10, and frames
			   * are 0 or 1, then the user typed a bad, move forward
			   */
			  if ((k == 4) && (*c != '0') && (total >= 0) && (total <= 1) 
				  && drop)
				{
				  if (total == 0)
					total += 2;
				  else if (total == 1)
					total++;
				}
			  total += atoi(c) * multiplier[k];
			  *c = '\0';
			  k++;
			}
		  else if (*c != ' ' && *c != ':' && *c != '.' && *c != ';' && *c != '+')
			{
			  total = 0;		/* error condition */
			  RAISE(OM_ERR_INVALID_TIMECODE);
			}
		}
	  
	  if (!drop)
		(*timecode).drop = kTcNonDrop;
	  else 
		(*timecode).drop = kTcDrop;
	  
	  (*timecode).fps = (omfUInt16)intFRate;
	  (*timecode).startFrame = total;
	} /* XPROTECT */

  XEXCEPT
	{
	  return(XCODE());
	}
  XEND;

  return(OM_ERR_NONE);
}



/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
