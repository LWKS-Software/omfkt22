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

#include "masterhd.h"
#include "omTypes.h"
#include "omCodec.h"

/* Moved math.h down here to make NEXT's compiler happy */
#include <math.h>

/*
 * C O N V E R T   T O   I E E E   E X T E N D E D
 * 
 * C O N V E R T   F R O M   I E E E   E X T E N D E D
 */

/*
 * Copyright (C) 1988-1991 Apple Computer, Inc. All rights reserved.
 * 
 * Warranty Information Even though Apple has reviewed this software, Apple
 * makes no warranty or representation, either express or implied, with
 * respect to this software, its quality, accuracy, merchantability, or
 * fitness for a particular purpose.  As a result, this software is provided
 * "as is," and you, its user, are assuming the entire risk as to its quality
 * and accuracy.
 * 
 * This code may be used and freely distributed as int  as it includes this
 * copyright notice and the above warranty information.
 * 
 * Machine-independent I/O routines for IEEE floating-point numbers.
 * 
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which happens to be
 * infinity on IEEE machines.  Unfortunately, it is impossible to preserve
 * NaN's in a machine-independent way. Infinities are, however, preserved on
 * IEEE machines.
 * 
 * These routines have been tested on the following machines: Apple Macintosh,
 * MPW 3.1 C compiler Apple Macintosh, THINK C compiler Silicon Graphics
 * IRIS, MIPS compiler Cray X/MP and Y/MP Digital Equipment VAX
 * 
 * 
 * Implemented by Malcolm Slaney and Ken Turkowski.
 * 
 * Malcolm Slaney contributions during 1988-1990 include big- and little- endian
 * file I/O, conversion to and from Motorola's extended 80-bit floating-point
 * format, and conversions to and from IEEE single- precision floating-point
 * format.
 * 
 * In 1991, Ken Turkowski implemented the conversions to and from IEEE
 * double-precision format, added more precision to the extended conversions,
 * and accommodated conversions involving +/- infinity, NaN's, and
 * denormalized numbers.
 */

/* #ifndef  PORT_MEM_DOS16     */

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif				/* HUGE_VAL */

# define FloatToUnsigned(f)  \
    ((omfUInt32)((((omfInt32)(f - 2147483648.0)) + 2147483647L) + 1))

# define UnsignedToFloat(u)    \
  (((double)((omfInt32)(u - 2147483647L - 1))) + 2147483648.0)

/* #endif   PORT_MEM_DOS16 */
/****************************************************************
 * Extended precision IEEE floating-point conversion routine.
 ****************************************************************/

/************************
 * name
 *
 * 		WhatIt(Internal)Does
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
void            ConvertToIeeeExtended(double num, char *bytes)
{

#if 1
	int             sign;
	int             expon;
	double          fMant, fsMant;
	omfUInt32          hiMant, loMant;

	if (num < 0)
	{
		sign = 0x8000;
		num *= -1;
	} else
	{
		sign = 0;
	}

	if (num == 0)
	{
		expon = 0;
		hiMant = 0;
		loMant = 0;
	} else
	{
		fMant = frexp(num, &expon);
		if ((expon > 16384) || !(fMant < 1))
		{		/* Infinity or NaN */
			expon = sign | 0x7FFF;
			hiMant = 0;
			loMant = 0;	/* infinity */
		} else
		{		/* Finite */
			expon += 16382;
			if (expon < 0)
			{	/* denormalized */
				fMant = ldexp(fMant, expon);
				expon = 0;
			}
			expon |= sign;
			fMant = ldexp(fMant, 32);
			fsMant = floor(fMant);
			hiMant = (omfUInt32)FloatToUnsigned(fsMant);
			fMant = ldexp(fMant - fsMant, 32);
			fsMant = floor(fMant);
			loMant = (omfUInt32)FloatToUnsigned(fsMant);
		}
	}

	bytes[0] = expon >> 8;
	bytes[1] = expon;
	bytes[2] = (char)(hiMant >> 24L);
	bytes[3] = (char) (hiMant >> 16L);
	bytes[4] = (char) (hiMant >> 8L);
	bytes[5] = (char) hiMant;
	bytes[6] = (char) (loMant >> 24L);
	bytes[7] = (char) (loMant >> 16L);
	bytes[8] = (char)( loMant >> 8L);
	bytes[9] = (char) loMant;
#else				/* Little-endian: int  double in PC is IEEE
				 * 80 bit */

	int  double     x;
	unsigned char  *p = (unsigned char *) &x;
	int             i;

	x = (int  double) num;
	// flop the bytes
	for (i = 0; i < 10; i++)
		bytes[i] = *(p + 9 - i);

#endif				/* ifndef  PORT_MEM_DOS16 */
}

/************************
 * name
 *
 * 		WhatIt(Internal)Does
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
double          ConvertFromIeeeExtended(char *bytes)
{
	double          f;
	omfInt32           expon;
	omfUInt32          hiMant, loMant;

	expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
	hiMant = ((omfUInt32) (bytes[2] & 0xFF) << 24)
		| ((omfUInt32) (bytes[3] & 0xFF) << 16)
		| ((omfUInt32) (bytes[4] & 0xFF) << 8)
		| ((omfUInt32) (bytes[5] & 0xFF));
	loMant = ((omfUInt32) (bytes[6] & 0xFF) << 24)
		| ((omfUInt32) (bytes[7] & 0xFF) << 16)
		| ((omfUInt32) (bytes[8] & 0xFF) << 8)
		| ((omfUInt32) (bytes[9] & 0xFF));

	if (expon == 0 && hiMant == 0 && loMant == 0)
	{
		f = 0;
	} else
	{
		if (expon == 0x7FFF)
		{		/* Infinity or NaN */
			f = HUGE_VAL;
		} else
		{
			expon -= 16383;
			f = ldexp(UnsignedToFloat(hiMant), (int) (expon -= 31));
			f += ldexp(UnsignedToFloat(loMant), (int) (expon -= 32));
		}
	}

	if (bytes[0] & 0x80)
		return -f;
	else
		return f;
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
