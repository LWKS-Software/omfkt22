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
#ifndef _OMF_AVR_PVT_
#define _OMF_AVR_PVT_ 1
#if PORT_LANG_CPLUSPLUS
extern          "C"
{
#endif

#define MAX_QTABLE_SIZE 193

#define MAX_NUM_AVRS 43

typedef struct
{
	char *avrName;
	omfInt32 TableID;
	omfInt32 QFactor;
	omfInt32 XSize;
	omfInt32 YSizeNTSC;
	omfInt32 YSizePAL;
	omfInt32 LeadingLinesNTSC;
	omfInt32 LeadingLinesPAL;
	omfInt32 NumberOfFields;
	omfInt32 TargetSize;
	omfUInt8 QT[MAX_QTABLE_SIZE];
	omfUInt16 QT16[MAX_QTABLE_SIZE];
} avrInfoStruct_t;


#if PORT_LANG_CPLUSPLUS
}
#endif
#endif				/* _OMF_AVR_PVT_ */

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
