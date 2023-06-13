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

#include "omErr.h"
#include "omPvt.h"

static char    *localErrorStrings[300];
/* A 75-column ruler
         111111111122222222223333333333444444444455555555556666666666777777
123456789012345678901234567890123456789012345678901234567890123456789012345
*/

/************************
 * Function: omfsGetErrorString
 *
 * 		Given an error code, returns an error string.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Always returns an error string of some sort, although
 *		it may just be "unknown error code".
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
char *omfsGetErrorString(
		omfErr_t code)
{
	if (code < OM_ERR_MAXCODE && code >= 0 && localErrorStrings[0] != '\0')
		return (localErrorStrings[code]);
	else
		return ("OMFI_ERR: Unknown error code");
}

/************************
 * Function: omfsGetExpandedErrorString
 *
 * 		Given an error code, returns an error string.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Always returns an error string of some sort, although
 *		it may just be "unknown error code".
 *
 * Possible Errors:
 *		Standard errors (see top of file).
 */
char *omfsGetExpandedErrorString(omfHdl_t file,
											omfErr_t code,
											omfInt16 buflen,
											char *buffer)
{
	char	*basic = omfsGetErrorString(code);
	
	strncpy(buffer, basic, buflen);
	if((strlen(basic) < (omfUInt16)buflen) && (file != NULL))
	{
		if(code == OM_ERR_BENTO_PROBLEM ||
		   code == OM_ERR_BADOPEN ||
		   code == OM_ERR_NOTOMFIFILE ||
		   code == OM_ERR_FILE_NOT_FOUND)
		{
			if(file->BentoErrorRaised != 0)
			{
				strncat(buffer, "\n[", buflen - strlen(buffer));
				strncat(buffer, file->BentoErrorString, buflen - strlen(buffer));
				strncat(buffer, "]", buflen - strlen(buffer));
			}
			else if(file->session->BentoErrorRaised != 0)
			{
				strncat(buffer, "\n[", buflen - strlen(buffer));
				strncat(buffer, file->session->BentoErrorString, buflen - strlen(buffer));
				strncat(buffer, "]", buflen - strlen(buffer));
			}
		}
	}

	return(buffer);
}

/************************
 * Function: omfRegErr			(CALLED BY MACROS)
 *
 * 	Internal routine to log an error.  Should not used anymore.
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
void omfRegErr(
			omfHdl_t		file,
			omfErr_t		msgcode)
{
}

#ifdef OMFI_ERROR_TRACE
/************************
 * Function: omfRegErrDebug		(CALLED BY MACROS)
 *
 * 	Debug version of omfRefErr.  This routine is used ONLY if OMFI_ERROR_TRACE
 *		is enabled, and prints information about the error raised to a stack trace
 *		buffer, which may be printed.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		<none>.
 *
 * Possible Errors:
 *		<none>.
 */
void omfRegErrDebug(
			omfHdl_t		file,
			omfErr_t		ec,
			char			*filename,
			omfInt16		line)
{
	if (file != NULL && file->cookie == FILE_COOKIE)
	{
		if(file->stackTrace != NULL)
			sprintf(file->stackTrace, "Error Raised <%1d>%s @ %d in '%s'\n", ec, omfsGetErrorString(ec), line, filename);
	}
}

/************************
 * Function: omfReregErrDebug	(CALLED BY MACROS)
 *
 * 	Called when an error is reraised (as opposed to being
 *		initially raised).  This is to allow errors being propagated
 *		up the stack looking different in the stack trace from
 *		Errors initially raised.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		Error code (see below).
 *
 * Possible Errors:
 *		<none>.
 */
void omfReregErrDebug(
			omfHdl_t file,
			omfErr_t msgcode,
			char *filename,
			omfInt16 line)
{
	char	buf[256];
	omfInt32	left;
		
	if (file != NULL && file->cookie == FILE_COOKIE)
	{
		sprintf(buf, "   Error <%1d>%s propagated  @ %d in '%s'\n", msgcode,
					omfsGetErrorString(msgcode), line, filename);
		left = file->stackTraceSize - (strlen(file->stackTrace) + 1);
		strncat(file->stackTrace, buf, left);
	}
}


/************************
 * Function: omfPrintStackTrace
 *
 * 	Put this routine in the top-level exception block of the test
 *		application to print out the trace buffer when an error occurs.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		<none>.
 *
 * Possible Errors:
 *		<none>.
 */
void omfPrintStackTrace(
			omfHdl_t file)
{
   if (file != NULL)
	  {
		 printf("***Stack trace from last error****\n");
		 printf("%s\n", file->stackTrace);
	  }
}
#endif

/************************
 * Function: omfsErrorInit	(INTERNAL)
 *
 * 	Called on session open to load up the error message table.
 *		A routine is used asopposed to static data, because Mac code
 *		resources can't statically initialize pointers.
 *
 * Argument Notes:
 *		<none>.
 *
 * ReturnValue:
 *		<none>.
 *
 * Possible Errors:
 *		<none>.
 */
void omfsErrorInit(void)
{
	localErrorStrings[OM_ERR_NONE] =
		"OMFI: Success";

/*** 1.0 COMPATIBILITY ONLY Error Codes ***/
	localErrorStrings[OM_ERR_BADOBJ] =
		"OMFI_ERR: Null OMFI/Bento object";

	localErrorStrings[OM_ERR_INULLMOBIDPTR] =
		"OMFI_ERR(Import): NULL Mob ID pointer in GetNthMediaDesc/SetMediaDesc";

	localErrorStrings[OM_ERR_INULLMTPTR] =
		"OMFI_ERR(Import): NULL omfMediaType_t pointer in GetNthMediaDesc";

	localErrorStrings[OM_ERR_INULLDESCPTR] =
		"OMFI_ERR(Import): NULL descriptor pointer in GetNthMediaDesc";

	localErrorStrings[OM_ERR_IBADMEDIAINDEX] =
		"OMFI_ERR(Import): Input OMFI file does not have enough media objects";

	localErrorStrings[OM_ERR_LRC_DISABLED] =
		"OMFI_ERR: LRC codec disabled";

	localErrorStrings[OM_ERR_LRC_BADSAMPLESIZE] =
		"OMFI_ERR: Only 16 bit samples supported for LRC";

	localErrorStrings[OM_ERR_LRC_MONOONLY] =
	   "OMFI_ERR: Only one channel audio supported for LRC";

	localErrorStrings[OM_ERR_BADSESSION] =
		"OMFI_INTERNAL_ERR: Bad Bento Session operation";

	localErrorStrings[OM_ERR_INTERNAL_BADOBJ] =
		"OMFI_INTERNAL_ERR: Error creating OMFI/Bento object";

	localErrorStrings[OM_ERR_BADSAMPLEOFFSET] =
		"OMFI_ERR: Sample Index offset out of range";


/*** SESSION/FILE Error Codes ***/
	/* Session Error Codes */
	localErrorStrings[OM_ERR_BAD_SESSION] =
		"OMFI_ERR: Bad Session Handle";

	localErrorStrings[OM_ERR_BADSESSIONOPEN] =
		"OMFI_ERR: Container Session open failed";

	localErrorStrings[OM_ERR_BADSESSIONMETA] =
		"OMFI_ERR: Container Session meta-handler operation failed";

	localErrorStrings[OM_ERR_BADSESSIONCLOSE] =
		"OMFI_ERR: Container Session close failed";

	localErrorStrings[OM_ERR_BADCONTAINER] =
		"OMFI_ERR: Cannot find Bento container";

	/* File Error Codes */
	localErrorStrings[OM_ERR_FILEREV_NOT_SUPP] =
		"OMFI_ERR: OMFI File Revision > 2.0 not supported";

	localErrorStrings[OM_ERR_FILEREV_DIFF] = 
		"OMFI_ERR: Files must have same file revisions";

	localErrorStrings[OM_ERR_BADOPEN] =
		"OMFI_ERR: Cannot open file";

	localErrorStrings[OM_ERR_BADCLOSE] =
		"OMFI_ERR: Cannot close file";

	localErrorStrings[OM_ERR_BAD_FHDL] =
		"OMFI_ERR: Bad File Handle";

	localErrorStrings[OM_ERR_BADHEAD] =
		"OMFI_ERR: Invalid OMFI HEAD object";

	localErrorStrings[OM_ERR_NOBYTEORDER] =
		"OMFI_ERR: No File Byte Order Specified";

	localErrorStrings[OM_ERR_INVALID_BYTEORDER] =
		"OMFI_ERR: Invalid byte order";

	localErrorStrings[OM_ERR_NOTOMFIFILE] =
		"OMFI_ERR: Invalid OMFI file";

	localErrorStrings[OM_ERR_WRONG_FILETYPE] =
		"OMFI_ERR: Invalid file type (Raw vs. OMFI)";

	localErrorStrings[OM_ERR_WRONG_OPENMODE] =
		"OMFI_ERR: File opened in wrong mode (readonly vs. readwrite)";

	localErrorStrings[OM_ERR_CONTAINERWRITE] = 
		"OMFI_ERR: Error writing to container media (Possibly media is full).";
		
	localErrorStrings[OM_ERR_FILE_NOT_FOUND] = 
		"OMFI_ERR: File not found.";
		
	/* Class Dictionary Error Codes */
	localErrorStrings[OM_ERR_CANNOT_SAVE_CLSD] = 
		"OMFI_ERR: Error updating the class dictionary in the OMFI file";

	localErrorStrings[OM_ERR_CANNOT_LOAD_CLSD] = 
		"OMFI_ERR: Error loading the class dictionary from the file";
		
	localErrorStrings[OM_ERR_FILE_REV_200] = 
		"OMFI_ERR: Please use toolkit revision 2.0.1 or later to write 2.x compliant files";

	localErrorStrings[OM_ERR_NEED_PRODUCT_IDENT] = 
		"OMFI_ERR: You must supply a product identification struct when creating or updating files";


/*** MEDIA Error Codes ***/
	/* General Media Error Codes */
	localErrorStrings[OM_ERR_DESCSAMPRATE] =
		"OMFI_ERR: Error getting sample rate from file descriptor";

	localErrorStrings[OM_ERR_SOURCEMOBLIST] =
		"OMFI_ERR: Error processing source mob list";

	localErrorStrings[OM_ERR_DESCLENGTH] =
		"OMFI_ERR: Error getting length from file descriptor";

	localErrorStrings[OM_ERR_INTERNAL_MDO] =
		"OMFI_ERR: Internal Media Data system not initialized";

	localErrorStrings[OM_ERR_3COMPONENT] =
		"OMFI_ERR: Only 3-component video allowed in OMF files";

	localErrorStrings[OM_ERR_INTERNAL_CORRUPTVINFO] =
		"OMFI_INTERNAL_ERR: Corrupt Video Info Structure";

	localErrorStrings[OM_ERR_BADSAMPLEOFFSET] =
		"OMFI_ERR: Sample Index offset out of range";

	localErrorStrings[OM_ERR_ONESAMPLEREAD] =
		"OMFI_ERR: Only one sample allowed for reading compressed data";

	localErrorStrings[OM_ERR_ONESAMPLEWRITE] =
		"OMFI_ERR: Only one sample allowed for writing compressed data";

	localErrorStrings[OM_ERR_DECOMPRESS] =
		"OMFI_ERR: Software Decompression Failed";

	localErrorStrings[OM_ERR_NODATA] =
		"OMFI_ERR: No data read";

	localErrorStrings[OM_ERR_SMALLBUF] =
		"OMFI_ERR: Sample too large to fit in given buffer";

	localErrorStrings[OM_ERR_INTERN_TOO_SMALL] =
		"OMFI_ERR: Buffer is not large enough to hold data";

	localErrorStrings[OM_ERR_INTERNAL_UNKNOWN_LOC] =
		"OMFI_ERR: Unknown locator type";

	localErrorStrings[OM_ERR_TRANSLATE] =
		"OMFI_ERR: Can't translate to expected memory format";

	localErrorStrings[OM_ERR_EOF] =
		"OMFI_ERR: End of file";

	localErrorStrings[OM_ERR_BADCOMPR] =
		"OMFI_ERR: Unrecognized compression type in OMF file";

	localErrorStrings[OM_ERR_BADPIXFORM] =
		"OMFI_ERR: Unrecognized pixel format in OMF file";

	localErrorStrings[OM_ERR_BADLAYOUT] =
		"OMFI_ERR: Unrecognized frame layout in OMF file";

	localErrorStrings[OM_ERR_COMPRLINESWR] =
		"OMFI_ERR: omfWriteLines not allowed with compression";

	localErrorStrings[OM_ERR_COMPRLINESRD] =
		"OMFI_ERR: omfReadLines not allowed with compression";

	localErrorStrings[OM_ERR_BADMEDIATYPE] =
		"OMFI_ERR: Unrecognized Media Type";

	localErrorStrings[OM_ERR_BADDATAADDRESS] =
		"OMFI_ERR: Null data address for transfer operation";

	localErrorStrings[OM_ERR_BAD_MDHDL] =
		"OMFI_ERR: Bad Media Handle";

	localErrorStrings[OM_ERR_MEDIA_NOT_FOUND] =
		"OMFI_ERR: Cannot locate media data";

	localErrorStrings[OM_ERR_ILLEGAL_MEMFMT] =
		"OMFI_ERR: Illegal memory format code";

	localErrorStrings[OM_ERR_ILLEGAL_FILEFMT] =
		"OMFI_ERR: Illegal file format code";

	localErrorStrings[OM_ERR_SWABBUFSIZE] =
		"OMFI_ERR: Invalid swab buffer size";

	localErrorStrings[OM_ERR_MISSING_SWAP_PROC] =
		"OMFI_ERR: Missing stream swap proc";

	localErrorStrings[OM_ERR_NULL_STREAMPROC] = 
		"OMFI_ERR: A Stream Callback is NULL";

	localErrorStrings[OM_ERR_NULLBUF] =
		"OMFI_ERR: Null transfer buffer";

	localErrorStrings[OM_ERR_SWAP_SETUP] =
		"OMFI_ERR: Need to set up file & memory format before calling this function";

	localErrorStrings[OM_ERR_INVALID_FILE_MOB] =
		"OMFI_ERR: Invalid file mob reference";
		
	localErrorStrings[OM_ERR_SINGLE_CHANNEL_OP] =
		"OMFI_ERR: Operation not valid on multi-channel media streams";

	localErrorStrings[OM_ERR_INVALID_CACHE_SIZE] = 
		"OMFI_ERR: Stream cache size must be positive or zero";

	localErrorStrings[OM_ERR_NOT_FILEMOB] = 
		"OMFI_ERR: Operation requires a file mob";

	localErrorStrings[OM_ERR_TRANSLATE_SAMPLE_SIZE] = 
		"OMFI_ERR: Codec can't translate to/from that sample size";
		
	localErrorStrings[OM_ERR_TRANSLATE_NON_INTEGRAL_RATE] =
		"OMFI_ERR: Codec can't translate to/from rates which are not multiples";

	localErrorStrings[OM_ERR_MISSING_MEDIA_REP] =
		"OMFI_ERR: Media representation not present in the mob";

	localErrorStrings[OM_ERR_NOT_LONGWORD] = 
		"OMFI_ERR: Buffer must be longword aligned for this translation";
		
	localErrorStrings[OM_ERR_XFER_DUPCH] = 
		"OMFI_ERR: Cannot specify the same channel twice in multi-xfers";

	localErrorStrings[OM_ERR_MEDIA_NOT_INIT] = 
		"OMFI_ERR: omfmInit() must be called before making this call";
		
	localErrorStrings[OM_ERR_BLOCKING_SIZE] = 
		"OMFI_ERR: Blocking size must be >= 0 bytes";
		
	localErrorStrings[OM_ERR_WRONG_MEDIATYPE] = 
		"OMFI_ERR: Incorrect media type for this operation";

	localErrorStrings[OM_ERR_MULTI_WRITELEN] = 
		"OMFI_ERR: Writelengths in a writeMultiple must be uniform";

	localErrorStrings[OM_ERR_STREAM_REOPEN] = 
		"OMFI_ERROR: Stream reopened without being closed";

	localErrorStrings[OM_ERR_TOO_MANY_FMT_OPS] = 
		"OMFI_ERROR: Too many format specifiers for this codec";
		
	localErrorStrings[OM_ERR_MEDIASTREAM_NOTALLOWED] =
		"OMFI_ERR: An object with a media stream datakind is not allowed";

	localErrorStrings[OM_ERR_STILLFRAME_BADLENGTH] =
		"OMFI_ERR: Length of a still frame in a media group must be 1";

	localErrorStrings[OM_ERR_DATA_NONCONTIG] = 
		"OMFI_ERR: Calling this function now will produce non-contiguous media data";
	   
	localErrorStrings[OM_ERR_OPLIST_OVERFLOW] =
	   "OMFI_ERR: Operation list overflow";
	   
	localErrorStrings[OM_ERR_STREAM_CLOSED] = 
		"OMFI_ERR: Stream must be open for this operation";
		
	localErrorStrings[OM_ERR_USE_MULTI_CREATE] = 
		"OMFI_ERR: Use multiCreate to create this many interleaved channels";

	localErrorStrings[OM_ERR_MEDIA_OPENMODE] =
		"OMFI_ERR: Media object opened in wrong mode (readonly vs. readwrite)";

	localErrorStrings[OM_ERR_MEDIA_CANNOT_CLOSE] =
		"OMFI_ERR: No proc to close media";

	localErrorStrings[OM_ERR_XFER_NOT_BYTES] =
		"OMFI_ERR: Sample transfers must be an integral number of bytes";
		
	localErrorStrings[OM_ERR_ZERO_SAMPLESIZE] =
		"OMFI_ERR: Sample size of zero not allowed";
		
	localErrorStrings[OM_ERR_ZERO_PIXELSIZE] =
		"OMFI_ERR: Pixel size of zero not allowed";
		
	/* Codec Error Codes */
	localErrorStrings[OM_ERR_CODEC_INVALID] =
		"CODEC invalid or not loaded";

	localErrorStrings[OM_ERR_INVALID_OP_CODEC] =
		"OMFI_ERR: Operation not valid on this codec";

	localErrorStrings[OM_ERR_BAD_CODEC_REV] =
		"OMFI_ERR: Out of date codec";

	localErrorStrings[OM_ERR_CODEC_CHANNELS] =
		"OMFI_ERR: Channel out of range for codec";
		
	localErrorStrings[OM_ERR_BAD_VARIETY] = 
		"OMFI_ERR: Badly formed codec variety string";

	localErrorStrings[OM_ERR_CODEC_NAME_SIZE] = 
		"OMFI_ERR: Codec name string too large";

	/* Image Error Codes */
	localErrorStrings[OM_ERR_TIFFVERSION] =
		"OMFI_ERR: Error reading tiff version";

	localErrorStrings[OM_ERR_BADTIFFCOUNT] =
		"OMFI_ERR: Video: TIFF count less than 1";

	localErrorStrings[OM_ERR_24BITVIDEO] =
		"OMFI_ERR: 24 Bit (8-8-8) video only";

	localErrorStrings[OM_ERR_JPEGBASELINE] =
		"OMFI_ERR: Only baseline JPEG allowed in OMF files";

	localErrorStrings[OM_ERR_BADJPEGINFO] =
		"OMFI_ERR: JPEG TIFF fields not allowed without COMPRESSION == JPEG";

	localErrorStrings[OM_ERR_BADQTABLE] =
		"OMFI_ERR: Bad JPEG Quantization Table";

	localErrorStrings[OM_ERR_BADACTABLE] =
		"OMFI_ERR: Bad JPEG AC Huffman Table";

	localErrorStrings[OM_ERR_BADDCTABLE] =
		"OMFI_ERR: Bad JPEG DC Huffman Table";

	localErrorStrings[OM_ERR_NOFRAMEINDEX] =
		"OMFI_ERR: No JPEG Video Frame Index";

	localErrorStrings[OM_ERR_BADFRAMEOFFSET] =
		"OMFI_ERR: Frame Index offset out of range";

	localErrorStrings[OM_ERR_JPEGPCM] =
		"OMFI_ERR: OMJPEG.c: put_color_map called";

	localErrorStrings[OM_ERR_JPEGDISABLED] =
		"OMFI_ERR: JPEG codec disabled";

	localErrorStrings[OM_ERR_JPEGPROBLEM] =
		"OMFI_ERR: Unspecified JPEG codec problem";

	localErrorStrings[OM_ERR_BADEXPORTPIXFORM] =
		"OMFI_ERR: Unrecognized Pixel Format";

	localErrorStrings[OM_ERR_BADEXPORTLAYOUT] =
		"OMFI_ERR: Unrecognized Frame Layout";

	localErrorStrings[OM_ERR_BADRWLINES] =
		"OMFI_ERR: Read/Write Lines only enabled for common video format";

	/* Audio Error Codes */
	localErrorStrings[OM_ERR_BADAIFCDATA] =
		"OMFI_ERR: Error reading AIFC data";

	localErrorStrings[OM_ERR_BADWAVEDATA] =
		"OMFI_ERR: Error reading WAVE data";

	localErrorStrings[OM_ERR_NOAUDIOCONV] =
		"OMFI_ERR: Audio Read Samples: Can't convert to requested sample size";


/*** OBJECT Error Codes ***/
	localErrorStrings[OM_ERR_NULLOBJECT] =
		"OMFI_ERR: Null Object not allowed";

	localErrorStrings[OM_ERR_BADINDEX] =
		"OMFI_ERR: Array Index Out of Range";

	localErrorStrings[OM_ERR_INVALID_LINKAGE] =
		"OMFI_ERR: Invalid object attached to property";

	localErrorStrings[OM_ERR_BAD_PROP] =
		"OMFI_ERR: Property code out of range";

	localErrorStrings[OM_ERR_BAD_TYPE] =
		"OMFI_ERR: Type code out of range";

	localErrorStrings[OM_ERR_SWAB] =
		"OMFI_ERR: Cannot swab that data size";

	localErrorStrings[OM_ERR_END_OF_DATA] =
		"OMFI_ERR: Read past end of data";

	localErrorStrings[OM_ERR_PROP_NOT_PRESENT] =
		"OMFI_ERR: Property missing from the file";

	localErrorStrings[OM_ERR_INVALID_DATAKIND] =
		"OMFI_ERR: Datakind invalid or nonexistant";

	localErrorStrings[OM_ERR_DATAKIND_EXIST] =
		"OMFI_ERR: A Datakind Definition with this ID already exists";
		
	localErrorStrings[OM_ERR_TOO_MANY_TYPES] =
		"OMFI_ERR: Too many types for a single property";

/*** MOB Error Codes ***/
	/* General Segment Error Codes */
	localErrorStrings[OM_ERR_NOT_SOURCE_CLIP] =
		"OMFI_ERR: This property must be a Source Clip";

	localErrorStrings[OM_ERR_FILL_FOUND] =
		"OMFI_ERR: An unexpected fill property was found";

	localErrorStrings[OM_ERR_BAD_LENGTH] = 
		"OMFI_ERR: Segment has an illegal length";

	localErrorStrings[OM_ERR_BADRATE] = 
		"OMFI_ERR: Illegal value for edit rate";

	localErrorStrings[OM_ERR_INVALID_ROUNDING] = 
		"OMFI_ERR: Editrate rounding must be either Floor or Ceiling";

	localErrorStrings[OM_ERR_PULLDOWN_DIRECTION] = 
		"OMFI_ERR: Illegal pulldown direction";
	
	localErrorStrings[OM_ERR_PULLDOWN_FUNC] = 
		"OMFI_ERR: use omfmMobAddPulldownRef() instead of omfmMobAddPhysSourceRef() for pulldown";
	
	localErrorStrings[OM_ERR_NOT_COMPOSITION] =
		"OMFI_ERR: This property must be a Composition";

	/* Timecode Error Codes */
	localErrorStrings[OM_ERR_TIMECODE_NOT_FOUND] =
		"OMFI_ERR: Timecode was not found in the mob chain";

	localErrorStrings[OM_ERR_NO_TIMECODE] = 
		"OMFI_ERR: Cannot find timecode on given track";

	localErrorStrings[OM_ERR_INVALID_TIMECODE] = 
		"OMFI_ERR: Timecode value is invalid";

	/* Track Error Codes */
	localErrorStrings[OM_ERR_TRACK_NOT_FOUND] =
		"OMFI_ERR: Track not found";

	localErrorStrings[OM_ERR_BAD_SLOTLENGTH] =
		"OMFI_ERR: Bad Slot Length";

	localErrorStrings[OM_ERR_MISSING_TRACKID] =
		"OMFI_ERR: TrackID not present in the mob";

	localErrorStrings[OM_ERR_TRACK_EXISTS] =
		"OMFI_ERR: A Track with this trackID already exists";

	localErrorStrings[OM_ERR_NOT_A_TRACK] =
		"OMFI_ERR: This function requires a track, not a mob slot.";

	/* MOBJ Error Codes */
	localErrorStrings[OM_ERR_MOB_NOT_FOUND] =
		"OMFI_ERR: Mob does not exist in this file";

	localErrorStrings[OM_ERR_NO_MORE_MOBS] =
		"OMFI_ERR: The end of the mob chain has been reached";

	localErrorStrings[OM_ERR_DUPLICATE_MOBID] =
		"OMFI_ERR: MobID already exists in the file";

	localErrorStrings[OM_ERR_MISSING_MOBID] =
		"OMFI_ERR: MobID not present in the file";

	/* Effect Error Codes */
	localErrorStrings[OM_ERR_EFFECTDEF_EXIST] =
		"OMFI_ERR: An Effect Definition with this Effect ID already exists";

	localErrorStrings[OM_ERR_INVALID_EFFECTDEF] = 
		"OMFI_ERR: Effect Definition invalid or nonexistant";

	localErrorStrings[OM_ERR_INVALID_EFFECT] = 
		"OMFI_ERR: Effect is invalid or non-existent";

	localErrorStrings[OM_ERR_INVALID_EFFECTARG] =
		"OMFI_ERR: The given effect argument is not valid for this effect";

	localErrorStrings[OM_ERR_INVALID_CVAL] = 
		"OMFI_ERR: Constant value is invalid or nonexistent";

	localErrorStrings[OM_ERR_RENDER_NOT_FOUND] = 
		"OMFI_ERR: Effect Rendering does not exist";

	/* Iterator Error Codes */
	localErrorStrings[OM_ERR_BAD_ITHDL] =
		"OMFI_ERR: Bad Iterator handle";

	localErrorStrings[OM_ERR_NO_MORE_OBJECTS] =
		"OMFI_ERR: No More Objects";

	localErrorStrings[OM_ERR_ITER_WRONG_TYPE] =
		"OMFI_ERR: Wrong iterator type for this function";

	localErrorStrings[OM_ERR_INVALID_SEARCH_CRIT] =
		"OMFI_ERR: Invalid search criteria for this kind of iterator";

	localErrorStrings[OM_ERR_INTERNAL_ITERATOR] =
		"OMFI_INTERNAL_ERR: Internal error with iterator";

	localErrorStrings[OM_ERR_BAD_SRCH_ITER] =
		"OMFI_ERR: This iterator handle must be allocated by omfiMobOpenSearch()";

	/* Traversal Error Codes */
	localErrorStrings[OM_ERR_NULL_MATCHFUNC] = 
		"OMFI_ERR: Match function to traversal routine is null";

	localErrorStrings[OM_ERR_NULL_CALLBACKFUNC] = 
		"OMFI_ERR: Callback function to traversal routine is null";

	localErrorStrings[OM_ERR_TRAVERSAL_NOT_POSS] =
		"OMFI_ERR: Mob traversal failed";

	localErrorStrings[OM_ERR_PARSE_EFFECT_AMBIGUOUS] =
		"OMFI_ERR: Need more information to parse further through an effect";

	/* Transition Error Codes */
	localErrorStrings[OM_ERR_INVALID_TRAN_EFFECT] =
		"OMFI_ERR: Effect is not a transition effect";

	localErrorStrings[OM_ERR_ADJACENT_TRAN] =
		"OMFI_ERR: Adjacent transitions in a sequence are illegal";

	localErrorStrings[OM_ERR_LEADING_TRAN] =
		"OMFI_ERR: Beginning a sequence with a transition is illegal";

	localErrorStrings[OM_ERR_INSUFF_TRAN_MATERIAL] =
		"OMFI_ERR: Not enough material as input to transition";


/*** SIMPLE COMPOSITION Error Codes ***/
	localErrorStrings[OM_ERR_BAD_STRACKHDL] =
		"OMFI_ERR: Bad Simple Track handle";

	localErrorStrings[OM_ERR_STRACK_APPEND_ILLEGAL] =
		"OMFI_ERR: This track does not contain a sequence, appending is illegal";


/*** GENERIC Error Codes ***/
	localErrorStrings[OM_ERR_NOMEMORY] =
		"OMFI_ERR: Memory allocation failed, no more heap memory";

	localErrorStrings[OM_ERR_OFFSET_SIZE] =
		"OMFI_ERR: 64-bit truncation error";

	localErrorStrings[OM_ERR_INTERNAL_NEG64] =
		"OMFI_ERR: Negative 64-bit number";

	localErrorStrings[OM_ERR_OVERFLOW64] = 
		"OMFI_ERR: Overflow on a 64-bit operation";

	localErrorStrings[OM_ERR_NOT_IN_15] =
		"OMFI_ERR: Function not available in 1.x compatability mode";

	localErrorStrings[OM_ERR_NOT_IN_20] =
		"OMFI_ERR: Function not available in 2.x native mode";

	localErrorStrings[OM_ERR_NOT_IMPLEMENTED] =
		"OMFI_ERR: Not Implemented";

	localErrorStrings[OM_ERR_NULL_PARAM] =
		"OMFI_ERR: NULL Actual parameter to function call";

	localErrorStrings[OM_ERR_ZERO_DIVIDE] = 
		"OMFI_ERR: Divide by zero";
		
/*** SEMANTIC CHECKING Error Codes ***/
	localErrorStrings[OM_ERR_REQUIRED_POSITIVE] =
		"OMFI_ERR: Value should be positive or zero";

	localErrorStrings[OM_ERR_INVALID_TRACKKIND] =
		"OMFI_ERR: Invalid Track Kind";

	localErrorStrings[OM_ERR_INVALID_EDGETYPE] =
		"OMFI_ERR: Invalid Edge Code Format Kind";

	localErrorStrings[OM_ERR_INVALID_FILMTYPE] =
		"OMFI_ERR: Invalid Film Type";

	localErrorStrings[OM_ERR_INVALID_TAPECASETYPE] =
		"OMFI_ERR: Invalid Tape Case Type";

	localErrorStrings[OM_ERR_INVALID_VIDEOSIGNALTYPE] =
		"OMFI_ERR: Invalid Video Signal Type";

	localErrorStrings[OM_ERR_INVALID_TAPEFORMATTYPE] =
		"OMFI_ERR: Invalid Tape Format Type";

	localErrorStrings[OM_ERR_INVALID_EDITHINT] =
		"OMFI_ERR: Invalid Effect Edit Hint";

	localErrorStrings[OM_ERR_INVALID_INTERPKIND] =
		"OMFI_ERR: Invalid Effect Interpolation Kind";

	localErrorStrings[OM_ERR_INVALID_MOBTYPE] =
		"OMFI_ERR: Invalid Mob Type";

	localErrorStrings[OM_ERR_INVALID_TRACK_REF] =
		"OMFI_ERR: Positive Relative Track Reference not allowed";

	localErrorStrings[OM_ERR_INVALID_OBJ] =
		"OMFI_ERR: Invalid object for this operation";

	localErrorStrings[OM_ERR_BAD_VIRTUAL_CREATE] =
		"OMFI_ERR: Creation of virtual objects not allowed";

	localErrorStrings[OM_ERR_INVALID_CLASS_ID] =
		"OMFI_ERR: Invalid Object Class ID";

	localErrorStrings[OM_ERR_OBJECT_SEMANTIC] =
		"OMFI_ERR: Failed a semantic check on an input obj";

	localErrorStrings[OM_ERR_DATA_IN_SEMANTIC] =
		"OMFI_ERR: Failed a semantic check on an input data";

	localErrorStrings[OM_ERR_DATA_OUT_SEMANTIC] =
		"OMFI_ERR: Failed a semantic check on an output data";

	localErrorStrings[OM_ERR_TYPE_SEMANTIC] = 
		"OMFI_ERR: Property and type do not match";

	localErrorStrings[OM_ERR_INVALID_ATTRIBUTEKIND] =
		"OMFI_ERR: Invalid Attribute Kind";
		
	localErrorStrings[OM_ERR_DATA_MDES_DISAGREE] =
		"OMFI_ERR: Media descriptor summary data does not agree with media";

	localErrorStrings[OM_ERR_CODEC_SEMANTIC_WARN] =
		"OMFI_ERR: A semantic check warning occured when checking media";
		
	localErrorStrings[OM_ERR_INVALID_BOOLTYPE] =
		"OMFI_ERR: Invalid Boolean Value";

/*** INTERNAL Error Codes ***/
	localErrorStrings[OM_ERR_TABLE_DUP_KEY] =
		"OMFI_INTERNAL_ERR: Duplicate Key detected in internal table";
	
	localErrorStrings[OM_ERR_TABLE_MISSING_COMPARE] =
		"OMFI_INTERNAL_ERR: Missing compare function on table";

	localErrorStrings[OM_ERR_TABLE_BAD_HDL] =
		"OMFI_INTERNAL_ERR: Bad table handle";
		
	localErrorStrings[OM_ERR_TABLE_BAD_ITER] =
		"OMFI_INTERNAL_ERR: Bad table iterator handle";
		
	localErrorStrings[OM_ERR_BENTO_PROBLEM] =
		"OMFI_ERR: Bento container error";

	localErrorStrings[OM_ERR_BENTO_HANDLER] = 
		"OMFI_INTERNAL_ERR: Can't locate Bento handler";
		
	localErrorStrings[OM_ERR_PROPID_MATCH] =
		"OMFI_INTERNAL_ERR: Property ID code doesn't match between revs";

	localErrorStrings[OM_ERR_INTERNAL_DIVIDE] = 
		"OMFI_ERR: Internal division error";
		
/*** Testing Error Codes ***/
	localErrorStrings[OM_ERR_TEST_FAILED] =
		"OMFI_TESTING_ERR: Test Failed";
}

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:3 ***
;;; End: ***
*/
