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
 * Name: omSearch.c
 *
 * Function: The purpose of this file is to isolate the two functions which map
 *		between codec ID's or MDES tags and the index into the array used as a
 *		cache in general usage.  By isolating these functions, a nonstandard
 *		search path may be implemented without changing the sources to the main
 *		toolkit.
 *
 *		The normal search path for codecs is to progress backwards from
 *		the most recent codec added back to the standard codecs.
 *
 * Audience: Clients who need a specialized search order for codecs.
 *
 * General error codes returned:
 * 		OM_ERR_BAD_FHDL -- media referenced a bad omfHdl_t.
 */

#include "masterhd.h"
#include <string.h>

#include "omPublic.h"
#include "omPvt.h" 

/* A 75-column ruler
         111111111122222222223333333333444444444455555555556666666666777777
123456789012345678901234567890123456789012345678901234567890123456789012345
*/

/************************
 * omfmFindCodecForMedia
 *
 * 		Given a media descriptor tag (the tag field on the media descriptor)
 *		return the codecID string and index value.
 *
 *		The codec ID may be different from the MDES tag, as multiple codecs
 *		may implement the same media type.  For instance, a hardware
 *		decompress version of the video codec.
 *
 * Argument Notes:
 *		The pointer to the returnID will probably be to static storage, so
 *		it's best to make a copy if you want to modify the result.
 *		The codecIndex is stored in the media opaque handle, and is used to
 *		optimize access to codec functions.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 *		OM_ERR_CODEC_INVALID - Can't find a codec which handles this MDES
 */
omfErr_t omfmFindCodecForMedia(
			omfHdl_t		file,
			omfObject_t		mdes,
			codecTable_t	*result)
{
	omfBool				more;
	omfClassID_t		mdesTag;
	omTableIterate_t	iter;
	codecTable_t		*codecPtr, *hwCodecPtr = NULL, *swCodecPtr = NULL;
	omfCodecSelectInfo_t info;
	omfProperty_t		idProp;
	
	XPROTECT(file)
	{
		if ((file->setrev == kOmfRev1x) || (file->setrev == kOmfRevIMA))
			idProp = OMObjID;
		else
			idProp = OMOOBJObjClass;

		CHECK(omfsReadClassID(file, mdes, idProp, mdesTag));

		CHECK(omfsTableFirstEntryMatching(file->session->codecMDES, &iter, mdesTag, &more));
		while(more && ((hwCodecPtr == NULL) || (swCodecPtr == NULL)))
		{
			codecPtr = (codecTable_t *)iter.valuePtr;
			CHECK(codecGetSelectInfo(file, codecPtr, mdes, &info));
			if(info.willHandleMDES)
			{
				if(info.hwAssisted)
					hwCodecPtr = codecPtr;
				else
					swCodecPtr = codecPtr;
			}
			
			CHECK(omfsTableNextEntry(&iter, &more));
		}
	
		if(hwCodecPtr != NULL)
			*result = *hwCodecPtr;
		else if(swCodecPtr != NULL)
			*result = *swCodecPtr;
		else
		  {
			RAISE(OM_ERR_CODEC_INVALID);
		  }
	}
	XEXCEPT
		result->rev = 0;
	XEND
							
	return (OM_ERR_NONE);
}


/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
