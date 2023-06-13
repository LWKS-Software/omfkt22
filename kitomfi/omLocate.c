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
#include <string.h>

#include "omPublic.h"
#include "omMedia.h" 
#include "omPvt.h" 
#include "omLocate.h" 

#if OMFI_MACSF_STREAM || OMFI_MACFSSPEC_STREAM
#include "OMFMacIO.h"
#else
#include "OMFAnsiC.h"
#endif

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
/*
 * In a separate file because it may need to be modified in the field for new
 * session handlers and/or new locator types
 */
omfErr_t        openLocator(omfHdl_t file, omfObject_t aLoc, omfFileFormat_t fmt, omfHdl_t * refFile)
{
	char            filename[256];
	omfClassID_t  locTag;
	omfInt16           vref = 0;
	omfInt32           DirID = 0;
	
	omfAssertValidFHdl(file);
	XPROTECT(file)
	{
		CHECK(omfmLocatorGetInfo(file, aLoc, locTag, sizeof(filename), filename));
		if (streq(locTag, "MACL"))
		{
			CHECK(omfmMacLocatorGetInfo(file, aLoc, &vref, &DirID));
		}
		if (fmt == kOmfiMedia)
		{


#if OMFI_MACSF_STREAM || OMFI_MACFSSPEC_STREAM
			CHECK((*file->session->ioFuncs.typedOpenFileFunc)(file->session, vref, DirID, filename, refFile));
#else
			CHECK((*file->session->ioFuncs.typedOpenFileFunc)(file->session, filename, refFile));
#endif
		} else
		{
#if OMFI_MACSF_STREAM || OMFI_MACFSSPEC_STREAM
			CHECK((*file->session->ioFuncs.typedOpenRawFileFunc)(file->session, vref, DirID, filename, refFile));
#else
			CHECK((*file->session->ioFuncs.typedOpenRawFileFunc)(file->session, filename, refFile));
#endif
		}
	}
	XEXCEPT
	XEND
	
	return (OM_ERR_NONE);
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
