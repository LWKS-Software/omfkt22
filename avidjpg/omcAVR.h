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
#ifndef _OMF_AJPG_CODEC_
#define _OMF_AJPG_CODEC_ 1
#if PORT_LANG_CPLUSPLUS
extern          "C"
{
#endif

#include "omCodec.h"

#define	CODEC_AJPG_VIDEO	"AJPG"  /* Avid JPEG-compressed image data */


/******************************************************************************

                   PRIVATE DATA STRUCTURE DEFINITION
                   For write operations, controls compression level.
                   Use a variable of type avidAVRdata_t for the 
                     avrData argument in the avrVideoMediaCreate
                     call.  e.g.,
                     
  avidAVRdata_t avrData;
  omfErr_t error;
  avrData.product = avidComposerMac;
  avrData.avr = "AVR26";
  error = avrVideoMediaCreate(	filePtr, masterMob, 1, fileMob, 
			    				videoRate, avrData, kNTSCSignal, &mediaPtr);

                                   
                   WARNING:  This is the only specification necessary to control
                     the image compression.  Do NOT call omfmSetJPEGTables!
                     
                   Call this function after creating the media object, and                        
                     before writing image data.
                    
                    
       			   The function will return OM_ERR_NOT_IMPLEMENTED if the avr
       				string is not recognized in this version of the codec.
       				
       				Rules for avr string:
       				
       					Use uppercase "AVR" as the prefix, followed by
       					the numeric code, followed by an optional lowercase
       					suffix, e.g, AVR6e, or AVR6s, or AVR75

*******************************************************************************/

typedef enum
	{
		avidComposerMac, avidBroadcastMac, avidRealImpact, avidMSPIndigo
	}				avidProductCode_t;
	
typedef struct
	{
		avidProductCode_t	product;
		char				*avr;
	}				avidAVRdata_t;
	
/******************************************************************************

                   PRIMARY CODEC ENTRY POINT

*******************************************************************************/

OMF_EXPORT omfErr_t   omfCodecAJPG(omfCodecFunctions_t func, omfCodecParms_t * parmblk);

/******************************************************************************

                   AUXILIARY CODEC ENTRY POINTS

*******************************************************************************/
/* 4/9/96 This interface (avrVideoMediaCreate) has changed.  The "signalType"
   parameter has been added to allow for accurate representation of film image
   data.  Film Composer, while visibly at 24fps, still requires that the video
   frames conform to either PAL or NTSC.  Since that information is not derivable
   from the edit rate, an extra parameter is used to transmit the information.
   For non-FilmComposer projects, the signalType value must match the editRate,
   e.g., PAL must be 25.0 fps, and NTSC must be 29.97 fps.  Film media (24fps)
   can be created with a signalType value of either kPALSignal or kNTSCSignal.
 */


OMF_EXPORT omfErr_t avrVideoMediaCreate(
			omfHdl_t		filePtr,			/* IN -- In this file */
			omfObject_t		masterMob,	/* IN -- on this master mob */
			omfTrackID_t    trackID,
			omfObject_t		fileMob,		/* IN -- & this file mob, create video */
			omfRational_t	editrate,
			avidAVRdata_t	avrData,
			omfVideoSignalType_t signalType, /* IN -- PAL, NTSC, or Null (compute from rate) */
			omfMediaHdl_t	*resultPtr);	/* OUT -- Return the object here. */


omfErr_t avrGetFormatData(  omfInt32 index,     /* IN - index */
	   int is_pal,      /* IN - true if we want pal height */
       char **name,     /* OUT - name of AVR */
       int *width,      /* OUT - width of image (field) */
       int *height,     /* OUT - height of image (field) */
       int *n_fields,      /* OUT - no of fields */
       int *black_line_count); /* OUT - no. of black lines at top of field */

#if  PORT_LANG_CPLUSPLUS
}
#endif
#endif				/* _OMF_AJPG_CODEC_ */

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
