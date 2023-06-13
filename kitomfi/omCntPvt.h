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

#ifndef _OMF_BENTO_PVT_
#define _OMF_BENTO_PVT_ 1

#include <string.h>

#if PORT_LANG_CPLUSPLUS
extern          "C"
{
#endif

/*
 * The next four includes MUST be included in the same order
 */

#include "XSession.h"
#include "TOCEnts.h"
#include "Containr.h"
#include "XHandlrs.h" /* Interface to handlers is same for different streams */

#if OMFI_MACSF_STREAM || OMFI_MACFSSPEC_STREAM
	typedef omfErr_t  (*omfsTypedOpenFileFunc_t)(omfSessionHdl_t, omfInt16, 
												 omfInt32, char *, omfHdl_t *);
	typedef omfErr_t  (*omfsTypedOpenRawFileFunc_t)(omfSessionHdl_t, omfInt16, 
													omfInt32, char *, omfHdl_t *);
#else
	typedef omfErr_t  (*omfsTypedOpenFileFunc_t)(omfSessionHdl_t, char *, omfHdl_t *);
	typedef omfErr_t  (*omfsTypedOpenRawFileFunc_t)(omfSessionHdl_t, char *, omfHdl_t *);
#endif

struct omfiBentoIOFuncs
{
	omfsTypedOpenFileFunc_t typedOpenFileFunc;
	omfsTypedOpenRawFileFunc_t typedOpenRawFileFunc;
	CMRefConFunc   createRefConFunc;
	CMMetaHandler  containerMetahandlerFunc;
};

#if PORT_LANG_CPLUSPLUS
}
#endif
#endif				/* _OMF_BENTO_PVT_ */

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
