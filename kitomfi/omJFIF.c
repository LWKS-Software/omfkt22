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
 * example.c
 *
 * This file is not actually part of the JPEG software.  Rather, it provides
 * a skeleton that may be useful for constructing applications that use the
 * JPEG software as subroutines.  This code will NOT do anything useful as is.
 *
 * This file illustrates how to use the JPEG code as a subroutine library
 * to read or write JPEG image files.  We assume here that you are not
 * merely interested in converting the image to yet another image file format
 * (if you are, you should be adding another I/O module to cjpeg/djpeg, not
 * constructing a new application).  Instead, we show how to pass the
 * decompressed image data into or out of routines that you provide.  For
 * example, a viewer program might use the JPEG decompressor together with
 * routines that write the decompressed image directly to a display.
 *
 * We present these routines in the same coding style used in the JPEG code
 * (ANSI function definitions, etc); but you are of course free to code your
 * routines in a different style if you prefer.
 */

/*
 * Include file for declaring JPEG data structures.
 * This file also includes some system headers like <stdio.h>;
 * if you prefer, you can include "jconfig.h" and "jpegdata.h" instead.
 */

#include "masterhd.h"
#include "omPublic.h"
#include "omMedia.h" 
#include "omJPEG.h" 
#include "omPvt.h" 

#if defined(THINK_C) || defined(__MWERKS__)
#define INCLUDES_ARE_ANSI 1
#endif

/*
 * Only define JPEG software codec functions if specifically enabled. This
 * will allow audio-only applications to be a little smaller
 */

#ifdef OMFI_JPEG_CODEC
#include "jinclude.h"

/*
 * <setjmp.h> is used for the optional error recovery mechanism shown in
 * the second part of the example.
 */

#include <setjmp.h>

/* These static variables are needed by the error routines. */
static jmp_buf setjmp_buffer;	/* for return to caller */
static external_methods_ptr emethods; /* needed for access to message_parm */

static char            OMJPEGErrorString[512];
static int             OMJPEGErrorRaised = 0;
static omfErr_t        localJPEGStatus =	OM_ERR_NONE;

#define HI4(num)  ((num) >> 4)
#define LOW4(num) ((num) & 0x0f)

#if PORT_MEM_DOS16
#define PIXEL unsigned char huge
#else
#define PIXEL unsigned char
#endif

	/************************************************************
	 *
	 * Code from jwrjfif.c 
	 *
	 *************************************************************/

/* Write a single byte */
static void emit_byte(compress_info_ptr cinfo, char data)
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
	omfMediaHdl_t  		media = parms->media;
	omfErr_t			functionJPEGStatus =	OM_ERR_NONE;

	functionJPEGStatus = omcWriteStream(media->stream, 1, &data);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
	}

}

typedef enum {			/* JPEG marker codes */
  M_SOF0  = 0xc0,
  M_SOF1  = 0xc1,
  M_SOF2  = 0xc2,
  M_SOF3  = 0xc3,
  
  M_SOF5  = 0xc5,
  M_SOF6  = 0xc6,
  M_SOF7  = 0xc7,
  
  M_JPG   = 0xc8,
  M_SOF9  = 0xc9,
  M_SOF10 = 0xca,
  M_SOF11 = 0xcb,
  
  M_SOF13 = 0xcd,
  M_SOF14 = 0xce,
  M_SOF15 = 0xcf,
  
  M_DHT   = 0xc4,
  
  M_DAC   = 0xcc,
  
  M_RST0  = 0xd0,
  M_RST1  = 0xd1,
  M_RST2  = 0xd2,
  M_RST3  = 0xd3,
  M_RST4  = 0xd4,
  M_RST5  = 0xd5,
  M_RST6  = 0xd6,
  M_RST7  = 0xd7,
  
  M_SOI   = 0xd8,
  M_EOI   = 0xd9,
  M_SOS   = 0xda,
  M_DQT   = 0xdb,
  M_DNL   = 0xdc,
  M_DRI   = 0xdd,
  M_DHP   = 0xde,
  M_EXP   = 0xdf,
  
  M_APP0  = 0xe0,
  M_APP15 = 0xef,
  
  M_JPG0  = 0xf0,
  M_JPG13 = 0xfd,
  M_COM   = 0xfe,
  
  M_TEM   = 0x01,
  
  M_ERROR = 0x100
} JPEG_MARKER;


LOCAL void
emit_marker (compress_info_ptr cinfo, JPEG_MARKER mark)
/* Emit a marker code */
{
  emit_byte(cinfo, (char)0xFF);
  emit_byte(cinfo, (char)mark);
}


LOCAL void
emit_2bytes (compress_info_ptr cinfo, int value)
/* Emit a 2-byte integer; these are always MSB first in JPEG files */
{
  emit_byte(cinfo, (char)((value >> 8) & 0xFF));
  emit_byte(cinfo, (char)(value & 0xFF));
}


LOCAL int
emit_dqt (compress_info_ptr cinfo, int numComponents)
/* Emit a DQT marker */
/* Returns the precision used (0 = 8bits, 1 = 16bits) for baseline checking */
{
  QUANT_TBL_PTR data;
  int prec = 0, totalPrec = 0;
  int i, chk, out;
  
  emit_marker(cinfo, M_DQT);
  
  emit_2bytes(cinfo, prec ? (DCTSIZE2*2 * numComponents) + numComponents + 2 : (DCTSIZE2 *numComponents) + numComponents + 2);
  
  for (i = 0; i < numComponents; i++) {

 		prec = 0;
 		data = cinfo->quant_tbl_ptrs[i];
  
		  for (chk = 0; chk < DCTSIZE2; chk++) {
		    if (data[chk] > 255)
		      prec = 1;
		  }


  		emit_byte(cinfo, (char)(i + (prec<<4)));
  
	  for (out = 0; out < DCTSIZE2; out++) {
	    if (prec)
	      emit_byte(cinfo, (char)(data[out] >> 8));
	    emit_byte(cinfo, (char)(data[out] & 0xFF));
	  totalPrec += prec;
	  }
	} 
  return totalPrec;
}


LOCAL void
emit_dht (compress_info_ptr cinfo, int numComponents)
/* Emit a DHT marker */
{
	omfBool	dcSent[4], acSent[4];
	

  HUFF_TBL * htbl;
  int length, i, cpnt, is_ac, index, sent;
  
    emit_marker(cinfo, M_DHT);
    
  	for (i = 0; i < 4; i++)
    {
    	dcSent[i] = FALSE;
    	acSent[i] = FALSE;
    }
    length = 0;
  for(cpnt = 0; cpnt < numComponents; cpnt++)
    for(is_ac = 0; is_ac <= 1; is_ac++)
    {
		if (is_ac)
		{
			index = cinfo->cur_comp_info[cpnt]->ac_tbl_no;
			sent = (index < 4 ? acSent[index] : TRUE);
		}
		else
		{
			index = cinfo->cur_comp_info[cpnt]->dc_tbl_no;
			sent = (index < 4 ? dcSent[index] : TRUE);
		}
		if(!sent)
		{
			if (is_ac) {
		    htbl = cinfo->ac_huff_tbl_ptrs[index];
			acSent[index] = TRUE;
		  } else {
		    htbl = cinfo->dc_huff_tbl_ptrs[index];
			dcSent[index] = TRUE;
		  }
	      for (i = 1; i <= 16; i++)
		      	length += htbl->bits[i];
		   length += 16 + 1;		
	     }
	  }
    emit_2bytes(cinfo, length + 2 );

  	for (i = 0; i < 4; i++)
    {
    	dcSent[i] = FALSE;
    	acSent[i] = FALSE;
    }
  for(cpnt = 0; cpnt < numComponents; cpnt++)
    for(is_ac = 0; is_ac <= 1; is_ac++)
     {
		if (is_ac)
		{
			index = cinfo->cur_comp_info[cpnt]->ac_tbl_no;
			sent = (index < 4 ? acSent[index] : TRUE);
		}
		else
		{
			index = cinfo->cur_comp_info[cpnt]->dc_tbl_no;
			sent = (index < 4 ? dcSent[index] : TRUE);
		}
		if(!sent)
		{
		if (is_ac) {
	   	 	htbl = cinfo->ac_huff_tbl_ptrs[index];
			acSent[index] = TRUE;
	 	 } else {
	    	htbl = cinfo->dc_huff_tbl_ptrs[index];
			dcSent[index] = TRUE;
	 	 }
		if (is_ac)
			 emit_byte(cinfo, (char)(index + 0x10));	/* output index has AC bit set */
		else
			 emit_byte(cinfo, (char)index);

  		if (htbl == NULL)
    		ERREXIT1(cinfo->emethods, "Huffman table 0x%02x was not defined", index);
  
		    for (i = 1, length = 0; i <= 16; i++)
		     {
		      emit_byte(cinfo, htbl->bits[i]);
		      	length += htbl->bits[i];
		    }
		    
		    for (i = 0; i < length; i++)
		      emit_byte(cinfo, htbl->huffval[i]);
    
   			 htbl->sent_table = TRUE;
   		}
  	}
/*!!!  } */
}

LOCAL void
emit_dri (compress_info_ptr cinfo)
/* Emit a DRI marker */
{
  emit_marker(cinfo, M_DRI);
  
  emit_2bytes(cinfo, 4);	/* fixed length */

  emit_2bytes(cinfo, (int) cinfo->restart_interval);
}


LOCAL void
emit_sof (compress_info_ptr cinfo, JPEG_MARKER code)
/* Emit a SOF marker */
{
  int i;
  
  emit_marker(cinfo, code);
  
  emit_2bytes(cinfo, 3 * cinfo->num_components + 2 + 5 + 1); /* length */

  if (cinfo->image_height > 65535L || cinfo->image_width > 65535L)
    ERREXIT(cinfo->emethods, "Maximum image dimension for JFIF is 65535 pixels");

  emit_byte(cinfo, (char)cinfo->data_precision);
  emit_2bytes(cinfo, (int) cinfo->image_height);
  emit_2bytes(cinfo, (int) cinfo->image_width);

  emit_byte(cinfo, (char)cinfo->num_components);

  for (i = 0; i < cinfo->num_components; i++) {
    emit_byte(cinfo, (char)cinfo->comp_info[i].component_id);
    emit_byte(cinfo, (char)((cinfo->comp_info[i].h_samp_factor << 4)
		     + cinfo->comp_info[i].v_samp_factor));
    emit_byte(cinfo, (char)cinfo->comp_info[i].quant_tbl_no);
  }
}


LOCAL void
emit_sos (compress_info_ptr cinfo)
/* Emit a SOS marker */
{
  int i;
  
  emit_marker(cinfo, M_SOS);
  
  emit_2bytes(cinfo, 2 * cinfo->comps_in_scan + 2 + 1 + 3); /* length */
  
  emit_byte(cinfo, (char)cinfo->comps_in_scan);
  
  for (i = 0; i < cinfo->comps_in_scan; i++) {
    emit_byte(cinfo, (char)cinfo->cur_comp_info[i]->component_id);
    emit_byte(cinfo, (char)((cinfo->cur_comp_info[i]->dc_tbl_no << 4)
		     + cinfo->cur_comp_info[i]->ac_tbl_no));
  }

  emit_byte(cinfo, 0);		/* Spectral selection start */
  emit_byte(cinfo, DCTSIZE2-1);	/* Spectral selection end */
  emit_byte(cinfo, 0);		/* Successive approximation */
}


LOCAL void
emit_jfif_app0 (compress_info_ptr cinfo)
/* Emit a JFIF-compliant APP0 marker */
{
  JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
  omfMediaHdl_t  		media = parms->media;

  /*
   * Length of APP0 block	(2 bytes)
   * Block ID			(4 bytes - ASCII "JFIF")
   * Zero byte			(1 byte to terminate the ID string)
   * Version Major, Minor	(2 bytes - 0x01, 0x01)
   * Units			(1 byte - 0x00 = none, 0x01 = inch, 0x02 = cm)
   * Xdpu			(2 bytes - dots per unit horizontal)
   * Ydpu			(2 bytes - dots per unit vertical)
   * Thumbnail X size		(1 byte)
   * Thumbnail Y size		(1 byte)
   */
  
  emit_marker(cinfo, M_APP0);
  
  emit_2bytes(cinfo, 2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1); /* length */

  emit_byte(cinfo, 0x41);	/* !!! Changed to AVI1 */ /* Identifier: ASCII "JFIF" */
  emit_byte(cinfo, 0x56);
  emit_byte(cinfo, 0x49);
  emit_byte(cinfo, 0x31);
  emit_byte(cinfo, 0);
  emit_byte(cinfo, 0);		/* Major version */
  omcGetStreamPosition(media->stream, &parms->frameSizePatch);
  emit_2bytes(cinfo, (int) 0);	/* Padded field size (4 bytes) */
  emit_2bytes(cinfo, (int) 0);
  emit_2bytes(cinfo, (int) 0);	/* Actual Compressed field size (4 bytes) */
  emit_2bytes(cinfo, (int) 0);
}

LOCAL void
emit_jfif_com (compress_info_ptr cinfo)
/* Emit a Avid Comment (COM) marker */
{
  int i;
  JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;

  emit_marker(cinfo,   M_COM);
  
  emit_2bytes(cinfo, 61); /* length */

  emit_byte(cinfo, 0x41);	/* Identifier: ASCII "AVID" */
  emit_byte(cinfo, 0x56);
  emit_byte(cinfo, 0x49);
  emit_byte(cinfo, 0x44);
  emit_byte(cinfo, 0x11);	/* Revision 1.1 */
  emit_2bytes(cinfo, 0);		/* First Line Captured */
  emit_2bytes(cinfo, 0);		/* Size of previous field (MSB) */
  emit_2bytes(cinfo, 0);		/* Size of previous field (LSB) */
  emit_byte(cinfo, (char)parms->JPEGTableID); /* JPEGTableID/vcID in 1 byte */

  for (i =0; i<47; i++)
	  emit_byte(cinfo, 0);		/* reserved for Avid */
  
}

LOCAL void
emit_jfif_dri (compress_info_ptr cinfo)
/* Emit a JFIF-Compliant Restart Interval marker */
{
  emit_marker(cinfo, M_DRI);
  
  emit_2bytes(cinfo, 4); /* length */
  emit_2bytes(cinfo, 0); /* length */
}

/*
 * Write the file header.
 */


METHODDEF void
write_file_header (compress_info_ptr cinfo)
{
  char qt_in_use[NUM_QUANT_TBLS];
  int i, prec, numIndices;
  boolean is_baseline;
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
  omfMediaHdl_t  		media = parms->media;
	omcGetStreamPosition(media->stream, &parms->frameStart);
  
  emit_marker(cinfo, M_SOI);	/* first the SOI */

  if (cinfo->write_JFIF_header)	/* next an optional JFIF APP0 */
  {
    emit_jfif_app0(cinfo);
	emit_jfif_com (cinfo);
	emit_jfif_dri (cinfo);
 }
  
  /* Emit DQT for each quantization table. */
  /* Note that doing it here means we can't adjust the QTs on-the-fly. */
  /* If we did want to do that, we'd have a problem with checking precision */
  /* for the is_baseline determination. */

  for (i = 0; i < NUM_QUANT_TBLS; i++)
    qt_in_use[i] = 0;

  for (i = 0; i < cinfo->num_components; i++)
    qt_in_use[cinfo->comp_info[i].quant_tbl_no] = 1;

  for (i = 0, numIndices=0; i < NUM_QUANT_TBLS; i++)
  	if(qt_in_use[i])
  		numIndices++;

  prec = emit_dqt(cinfo, numIndices);
  /* now prec is nonzero iff there are any 16-bit quant tables. */

  /* Check for a non-baseline specification. */
  /* Note we assume that Huffman table numbers won't be changed later. */
  is_baseline = TRUE;
  if (cinfo->arith_code || (cinfo->data_precision != 8))
    is_baseline = FALSE;
  for (i = 0; i < cinfo->num_components; i++) {
    if (cinfo->comp_info[i].dc_tbl_no > 1 || cinfo->comp_info[i].ac_tbl_no > 1)
      is_baseline = FALSE;
  }
  if (prec && is_baseline) {
    is_baseline = FALSE;
    /* If it's baseline except for quantizer size, warn the user */
    TRACEMS(cinfo->emethods, 0,
	    "Caution: quantization tables are too coarse for baseline JPEG");
  }


	if(!parms->isAvidJFIF)
 	{
 		/* Emit the proper SOF marker */
  		if (cinfo->arith_code)
   			emit_sof(cinfo, M_SOF9);	/* SOF code for arithmetic coding */
  		else if (is_baseline)
  			emit_sof(cinfo, M_SOF0);	/* SOF code for baseline implementation */
 		else
			emit_sof(cinfo, M_SOF1);	/* SOF code for non-baseline Huffman file */
	}
}


/*
 * Write the start of a scan (everything through the SOS marker).
 */

METHODDEF void
write_scan_header (compress_info_ptr cinfo)
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;

  if(parms->isAvidJFIF)
  {
	    emit_dht(cinfo, cinfo->comps_in_scan);
  	 	emit_sof(cinfo, M_SOF0);	/* SOF code for baseline implementation */
  	 	/* SOF must be directly before SOS for the hardware */
  }
  else
  {
	  if (cinfo->arith_code) {
	  } else {
	    /* Emit Huffman tables. !!! Not any more Note that emit_dht takes care of
	     * suppressing duplicate tables.
	     */
	      emit_dht(cinfo, cinfo->comps_in_scan);
	  }

	  /* Emit DRI if required --- note that DRI value could change for each scan.
	   * If it doesn't, a tiny amount of space is wasted in multiple-scan files.
	   * We assume DRI will never be nonzero for one scan and zero for a later one.
	   */
	  if (cinfo->restart_interval)
	    emit_dri(cinfo);
  }

  emit_sos(cinfo);
}


/*
 * Write some bytes of compressed data within a scan.
 */

METHODDEF void
write_jpeg_data (compress_info_ptr cinfo, char *dataptr, int datacount)
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
	omfMediaHdl_t   	media = parms->media;
	omfErr_t			functionJPEGStatus =	OM_ERR_NONE;

	functionJPEGStatus = omcWriteStream(media->stream, datacount, dataptr);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
	}
}


/*
 * Finish up after a compressed scan (series of write_jpeg_data calls).
 */

METHODDEF void
write_scan_trailer (compress_info_ptr cinfo)
{
  /* no work needed in this format */
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
write_file_trailer (compress_info_ptr cinfo)
{
  JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
  omfMediaHdl_t  		media = parms->media;
  omfInt32	n, 	tst, pad;
  omfLength_t		length, endPos;

	omcGetStreamPosition(media->stream, &length);
	omfsSubInt64fromInt64(parms->frameStart, &length);
	omfsAddInt32toInt64(2, &length);	/* Account for the FFD9 */
	omfsTruncInt64toInt32(length, &tst);
	pad = 4-(tst%4);
	if(pad == 4)
		pad = 0;
  	pad += 4;
  	for(n = pad; n >= 1; n--)
		emit_byte(cinfo, (char)0xFF);

  emit_marker(cinfo, M_EOI);
	omcGetStreamPosition(media->stream, &endPos);
	length = endPos;
	omfsSubInt64fromInt64(parms->frameStart, &length);
	omfsTruncInt64toInt32(length, &tst);
	omcSeekStreamTo(media->stream, parms->frameSizePatch);
  emit_2bytes (cinfo, (omfInt16)((tst >> 16L) & 0xFFFF));	/* Write padded size */
  emit_2bytes (cinfo, (omfInt16)(tst & 0xFFFF));
  emit_2bytes (cinfo, (omfInt16)((tst >> 16L) & 0xFFFF));	/* Write actual size (same) */
  emit_2bytes (cinfo, (omfInt16)(tst & 0xFFFF));
	omcSeekStreamTo(media->stream, endPos);

  /* Make sure we wrote the output file OK */
}



	/************************************************************
	 *
	 * Code from example.c 
	 *
	 *************************************************************/
/*
 * Routines to parse JPEG markers & save away the useful info.
 */

METHODDEF int
                omjpeg_read_jpeg_data(decompress_info_ptr dinfo)
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)dinfo->input_file;
	omfMediaHdl_t   	media = parms->media;
	omfUInt32     		bytesRead, toRead;
	omfInt64			length, offset;
	omfErr_t			functionJPEGStatus =	OM_ERR_NONE;
	
	dinfo->next_input_byte = dinfo->input_buffer + MIN_UNGET;

	functionJPEGStatus = omcGetLength(media->stream, &length);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
	}

	functionJPEGStatus = omcGetStreamPosition(media->stream, &offset);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
	}

	functionJPEGStatus = omfsSubInt64fromInt64(offset, &length);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
	}

	functionJPEGStatus = omfsTruncInt64toUInt32(length, &toRead);	/* OK MAXREAD */
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
	}


	if(toRead > JPEG_BUF_SIZE)
		toRead = JPEG_BUF_SIZE;
	functionJPEGStatus = omcReadStream(media->stream, toRead, (dinfo->next_input_byte), &bytesRead);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
	}
	dinfo->bytes_in_buffer = bytesRead;

	if (dinfo->bytes_in_buffer <= 0)
	{
		WARNMS(dinfo->emethods, "Premature EOF in JPEG file");
		dinfo->next_input_byte[0] = (unsigned char) 0xFF;
		dinfo->next_input_byte[1] = (unsigned char) M_EOI;
		dinfo->bytes_in_buffer = 2;
	}
	return JGETC(dinfo);
}

LOCAL INT32
get_2bytes (decompress_info_ptr cinfo)
/* Get a 2-byte unsigned integer (e.g., a marker parameter length field) */
{
  INT32 a;
  
  a = JGETC(cinfo);
  return (a << 8) + JGETC(cinfo);
}


LOCAL void
skip_variable (decompress_info_ptr cinfo, int code)
/* Skip over an unknown or uninteresting variable-length marker */
{
  INT32 length;
  
  length = get_2bytes(cinfo);
  
  TRACEMS2(cinfo->emethods, 1,
	   "Skipping marker 0x%02x, length %u", code, (int) length);
  
  for (length -= 2; length > 0; length--)
    (void) JGETC(cinfo);
}


LOCAL void
get_dht (decompress_info_ptr cinfo)
/* Process a DHT marker */
{
  INT32 length;
  UINT8 bits[17];
  UINT8 huffval[256];
  int i, index, count;
  HUFF_TBL **htblptr;
  
  length = get_2bytes(cinfo)-2;
  
  while (length > 0) {
    index = JGETC(cinfo);

    TRACEMS1(cinfo->emethods, 1, "Define Huffman Table 0x%02x", index);
      
    bits[0] = 0;
    count = 0;
    for (i = 1; i <= 16; i++) {
      bits[i] = (UINT8) JGETC(cinfo);
      count += bits[i];
    }

    TRACEMS8(cinfo->emethods, 2, "        %3d %3d %3d %3d %3d %3d %3d %3d",
	     bits[1], bits[2], bits[3], bits[4],
	     bits[5], bits[6], bits[7], bits[8]);
    TRACEMS8(cinfo->emethods, 2, "        %3d %3d %3d %3d %3d %3d %3d %3d",
	     bits[9], bits[10], bits[11], bits[12],
	     bits[13], bits[14], bits[15], bits[16]);

    if (count > 256)
      ERREXIT(cinfo->emethods, "Bogus DHT counts");

    for (i = 0; i < count; i++)
      huffval[i] = (UINT8) JGETC(cinfo);

    length -= 1 + 16 + count;

    if (index & 0x10) {		/* AC table definition */
      index -= 0x10;
      htblptr = &cinfo->ac_huff_tbl_ptrs[index];
    } else {			/* DC table definition */
      htblptr = &cinfo->dc_huff_tbl_ptrs[index];
    }

    if (index < 0 || index >= NUM_HUFF_TBLS)
      ERREXIT1(cinfo->emethods, "Bogus DHT index %d", index);

    if (*htblptr == NULL)
      *htblptr = (HUFF_TBL *) (*cinfo->emethods->alloc_small) (SIZEOF(HUFF_TBL));
  
    MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
    MEMCOPY((*htblptr)->huffval, huffval, SIZEOF((*htblptr)->huffval));
    }
}

LOCAL void
get_dqt (decompress_info_ptr cinfo)
/* Process a DQT marker */
{
  INT32 length;
  int n, i, prec;
  UINT16 tmp;
  QUANT_TBL_PTR quant_ptr;
  
  length = get_2bytes(cinfo) - 2;
  
  while (length > 0) {
    n = JGETC(cinfo);
    prec = n >> 4;
    n &= 0x0F;

    TRACEMS2(cinfo->emethods, 1,
	     "Define Quantization Table %d  precision %d", n, prec);

    if (n >= NUM_QUANT_TBLS)
      ERREXIT1(cinfo->emethods, "Bogus table number %d", n);
      
    if (cinfo->quant_tbl_ptrs[n] == NULL)
      cinfo->quant_tbl_ptrs[n] = (QUANT_TBL_PTR)
	(*cinfo->emethods->alloc_small) (SIZEOF(QUANT_TBL));
    quant_ptr = cinfo->quant_tbl_ptrs[n];

    for (i = 0; i < DCTSIZE2; i++) {
      tmp = JGETC(cinfo);
      if (prec)
	tmp = (tmp<<8) + JGETC(cinfo);
      quant_ptr[i] = tmp;
    }

    for (i = 0; i < DCTSIZE2; i += 8) {
      TRACEMS8(cinfo->emethods, 2, "        %4u %4u %4u %4u %4u %4u %4u %4u",
	       quant_ptr[i  ], quant_ptr[i+1], quant_ptr[i+2], quant_ptr[i+3],
	       quant_ptr[i+4], quant_ptr[i+5], quant_ptr[i+6], quant_ptr[i+7]);
    }

    length -= DCTSIZE2+1;
    if (prec) length -= DCTSIZE2;
  }
}


LOCAL void
get_dri (decompress_info_ptr cinfo)
/* Process a DRI marker */
{
  if (get_2bytes(cinfo) != 4)
    ERREXIT(cinfo->emethods, "Bogus length in DRI");

  cinfo->restart_interval = (UINT16) get_2bytes(cinfo);

  TRACEMS1(cinfo->emethods, 1,
	   "Define Restart Interval %u", cinfo->restart_interval);
}


LOCAL void
get_app0 (decompress_info_ptr cinfo)
/* Process an APP0 marker 
 NOTE this process is for NORMAL JFIF header not the one the comes from AVID.
 Therefore on an avid file it will break once it checks the magic name (0x4A464946), 
 which on the avid one is really 'AVI1' (0x41564931) */
{
#define JFIF_LEN 14
  INT32 length;
  UINT8 b[JFIF_LEN];
  int buffp;

  length = get_2bytes(cinfo) - 2;

  /* See if a JFIF APP0 marker is present */

  if (length >= JFIF_LEN) {
    for (buffp = 0; buffp < JFIF_LEN; buffp++)
      b[buffp] = (UINT8) JGETC(cinfo);
    length -= JFIF_LEN;

    if (b[0]==0x4A && b[1]==0x46 && b[2]==0x49 && b[3]==0x46 && b[4]==0) {
      /* Found JFIF APP0 marker: check version */
      /* Major version must be 1 */
      if (b[5] != 1)
	ERREXIT2(cinfo->emethods, "Unsupported JFIF revision number %d.%02d",
		 b[5], b[6]);
      /* Minor version should be 0..2, but try to process anyway if newer */
      if (b[6] > 2)
	TRACEMS2(cinfo->emethods, 1, "Warning: unknown JFIF revision number %d.%02d",
		 b[5], b[6]);
      /* Save info */
      cinfo->density_unit = b[7];
      cinfo->X_density = (b[8] << 8) + b[9];
      cinfo->Y_density = (b[10] << 8) + b[11];
      /* Assume colorspace is YCbCr, unless UI has overridden me */
      if (cinfo->jpeg_color_space == CS_UNKNOWN)
	cinfo->jpeg_color_space = CS_YCbCr;
      TRACEMS3(cinfo->emethods, 1, "JFIF APP0 marker, density %dx%d  %d",
	       cinfo->X_density, cinfo->Y_density, cinfo->density_unit);
      if (b[12] | b[13])
	TRACEMS2(cinfo->emethods, 1, "    with %d x %d thumbnail image",
		 b[12], b[13]);
      if (length != ((INT32) b[12] * (INT32) b[13] * (INT32) 3))
	TRACEMS1(cinfo->emethods, 1,
		 "Warning: thumbnail image size does not match data length %u",
		 (int) length);
    } else {
      TRACEMS1(cinfo->emethods, 1, "Unknown APP0 marker (not JFIF), length %u",
	       (int) length + JFIF_LEN);
    }
  } else {
    TRACEMS1(cinfo->emethods, 1, "Short APP0 marker, length %u", (int) length);
  }

  while (length-- > 0)		/* skip any remaining data */
    (void) JGETC(cinfo);
}

LOCAL void
get_com (decompress_info_ptr cinfo)
/* Process an COM marker */
{
#define JFIF_COM_LEN_1_0 11
#define JFIF_COM_LEN_1_1 59
 	INT32 length;
	UINT8 b[JFIF_COM_LEN_1_1];
	int buffp;

	length = get_2bytes(cinfo) - 2;

	/* See if a JFIF COM marker is present */

	if (length >= JFIF_COM_LEN_1_0) 
	{
		/* First read in Version 1.0 */
		for (buffp = 0; buffp < JFIF_COM_LEN_1_0; buffp++)
			b[buffp] = (UINT8) JGETC(cinfo);
		length -= JFIF_COM_LEN_1_0;

		if (b[0]==0x41 && b[1]==0x56 && b[2]==0x49 && b[3]==0x44) 
		{
			/* Found JFIF COM marker: check version */
			/* Major version must be 1 */
			if (HI4(b[4]) != 1)
				ERREXIT2(cinfo->emethods, "Unsupported JFIF COM revision number %d.%02d",		
									HI4(b[4]), LOW4(b[4]));
			/* Minor version should be 0..1, but try to process anyway if newer */
			if (LOW4(b[4]) > 1)
				TRACEMS2(cinfo->emethods, 1, "Warning: unknown JFIF COM revision number %d.%02d",
						HI4(b[4]), LOW4(b[4]));
			if ( LOW4(b[4]) >=1 && length >= (JFIF_COM_LEN_1_1-JFIF_COM_LEN_1_0))
			{
				JPEG_MediaParms_t   *parms;
				/* for version 1.1 and above we can do some processing to get
					whatever information we can gather */
				for (buffp = JFIF_COM_LEN_1_0; buffp < JFIF_COM_LEN_1_1; buffp++)
					b[buffp] = (UINT8) JGETC(cinfo);
				length -= JFIF_COM_LEN_1_1 - JFIF_COM_LEN_1_0;
				
				/* Save info */
				parms = (JPEG_MediaParms_t *)cinfo->input_file;
				parms->JPEGTableID = b[11];
			}
			else 
			{
				TRACEMS3(cinfo->emethods, 1, "Short COM marker, version number %d.%02d, length %u",
						 HI4(b[4]), LOW4(b[4]), (int) length);
			}
		} 
		else 
		{
			TRACEMS1(cinfo->emethods, 1, "Unknown COM marker (not JFIF), length %u",
					 (int) length + JFIF_COM_LEN_1_0);
		}
	} 
	else 
	{
		TRACEMS1(cinfo->emethods, 1, "Short COM marker, length %u", (int) length);
	}

	while (length-- > 0)		/* skip any remaining data */
		(void) JGETC(cinfo);
}


LOCAL void
get_sof (decompress_info_ptr cinfo, int code)
/* Process a SOFn marker */
{
  INT32 length;
  short ci;
  int c;
  jpeg_component_info * compptr;
  
  length = get_2bytes(cinfo);
  
  cinfo->data_precision = JGETC(cinfo);
  cinfo->image_height   = get_2bytes(cinfo);
  cinfo->image_width    = get_2bytes(cinfo);
  cinfo->num_components = JGETC(cinfo);

  TRACEMS4(cinfo->emethods, 1,
	   "Start Of Frame 0x%02x: width=%u, height=%u, components=%d",
	   code, (int) cinfo->image_width, (int) cinfo->image_height,
	   cinfo->num_components);

  /* We don't support files in which the image height is initially specified */
  /* as 0 and is later redefined by DNL.  As int  as we have to check that,  */
  /* might as well have a general sanity check. */
  if (cinfo->image_height <= 0 || cinfo->image_width <= 0
      || cinfo->num_components <= 0)
    ERREXIT(cinfo->emethods, "Empty JPEG image (DNL not supported)");

#ifdef EIGHT_BIT_SAMPLES
  if (cinfo->data_precision != 8)
    ERREXIT(cinfo->emethods, "Unsupported JPEG data precision");
#endif
#ifdef TWELVE_BIT_SAMPLES
  if (cinfo->data_precision != 12) /* this needs more thought?? */
    ERREXIT(cinfo->emethods, "Unsupported JPEG data precision");
#endif
#ifdef SIXTEEN_BIT_SAMPLES
  if (cinfo->data_precision != 16) /* this needs more thought?? */
    ERREXIT(cinfo->emethods, "Unsupported JPEG data precision");
#endif

  if (length != (cinfo->num_components * 3 + 8))
    ERREXIT(cinfo->emethods, "Bogus SOF length");

  cinfo->comp_info = (jpeg_component_info *) (*cinfo->emethods->alloc_small)
			(cinfo->num_components * SIZEOF(jpeg_component_info));
  
  for (ci = 0; ci < cinfo->num_components; ci++) {
    compptr = &cinfo->comp_info[ci];
    compptr->component_index = ci;
    compptr->component_id = JGETC(cinfo);
    c = JGETC(cinfo);
    compptr->h_samp_factor = (c >> 4) & 15;
    compptr->v_samp_factor = (c     ) & 15;
    compptr->quant_tbl_no  = JGETC(cinfo);
      
    TRACEMS4(cinfo->emethods, 1, "    Component %d: %dhx%dv q=%d",
	     compptr->component_id, compptr->h_samp_factor,
	     compptr->v_samp_factor, compptr->quant_tbl_no);
  }
}


LOCAL void
get_sos (decompress_info_ptr cinfo)
/* Process a SOS marker */
{
  INT32 length;
  int i, ci, n, c, cc;
  jpeg_component_info * compptr;
  
  length = get_2bytes(cinfo);
  
  n = JGETC(cinfo);  /* Number of components */
  cinfo->comps_in_scan = n;
  length -= 3;
  
  if (length != (n * 2 + 3) || n < 1 || n > MAX_COMPS_IN_SCAN)
    ERREXIT(cinfo->emethods, "Bogus SOS length");

  TRACEMS1(cinfo->emethods, 1, "Start Of Scan: %d components", n);
  
  for (i = 0; i < n; i++) {
    cc = JGETC(cinfo);
    c = JGETC(cinfo);
    length -= 2;
    
    for (ci = 0; ci < cinfo->num_components; ci++)
      if (cc == cinfo->comp_info[ci].component_id)
	break;
    
    if (ci >= cinfo->num_components)
      ERREXIT(cinfo->emethods, "Invalid component number in SOS");
    
    compptr = &cinfo->comp_info[ci];
    cinfo->cur_comp_info[i] = compptr;
    compptr->dc_tbl_no = (c >> 4) & 15;
    compptr->ac_tbl_no = (c     ) & 15;
    
    TRACEMS3(cinfo->emethods, 1, "    c%d: [dc=%d ac=%d]", cc,
	     compptr->dc_tbl_no, compptr->ac_tbl_no);
  }
  
  while (length > 0) {
    (void) JGETC(cinfo);
    length--;
  }
}


LOCAL void
get_soi (decompress_info_ptr cinfo)
/* Process an SOI marker */
{
  int i;
  
  TRACEMS(cinfo->emethods, 1, "Start of Image");

  /* Reset all parameters that are defined to be reset by SOI */

  for (i = 0; i < NUM_ARITH_TBLS; i++) {
    cinfo->arith_dc_L[i] = 0;
    cinfo->arith_dc_U[i] = 1;
    cinfo->arith_ac_K[i] = 5;
  }
  cinfo->restart_interval = 0;

  cinfo->density_unit = 0;	/* set default JFIF APP0 values */
  cinfo->X_density = 1;
  cinfo->Y_density = 1;

  cinfo->CCIR601_sampling = FALSE; /* Assume non-CCIR sampling */
}


LOCAL int
next_marker (decompress_info_ptr cinfo)
/* Find the next JPEG marker */
/* Note that the output might not be a valid marker code, */
/* but it will never be 0 or FF */
{
  int c, nbytes;

  nbytes = 0;
  do {
    do {			/* skip any non-FF bytes */
      nbytes++;
      c = JGETC(cinfo);
    } while (c != 0xFF);
    do {			/* skip any duplicate FFs */
      /* we don't increment nbytes here since extra FFs are legal */
      c = JGETC(cinfo);
    } while (c == 0xFF);
  } while (c == 0);		/* repeat if it was a stuffed FF/00 */

  if (nbytes != 1)
    WARNMS2(cinfo->emethods,
	    "Corrupt JPEG data: %d extraneous bytes before marker 0x%02x",
	    nbytes-1, c);

  return c;
}


LOCAL JPEG_MARKER
process_tables (decompress_info_ptr cinfo)
/* Scan and process JPEG markers that can appear in any order */
/* Return when an SOI, EOI, SOFn, or SOS is found */
{
  int c;

  while (TRUE) {
    c = next_marker(cinfo);
      
    switch (c) {
    case M_SOF0:
    case M_SOF1:
    case M_SOF2:
    case M_SOF3:
    case M_SOF5:
    case M_SOF6:
    case M_SOF7:
    case M_JPG:
    case M_SOF9:
    case M_SOF10:
    case M_SOF11:
    case M_SOF13:
    case M_SOF14:
    case M_SOF15:
    case M_SOI:
    case M_EOI:
    case M_SOS:
      return ((JPEG_MARKER) c);
      
    case M_DHT:
      get_dht(cinfo);
      break;
         
    case M_DQT:
      get_dqt(cinfo);
      break;
      
    case M_DRI:
      get_dri(cinfo);
      break;
      
    case M_APP0:
      get_app0(cinfo);
      break;

    case M_COM:
      get_com(cinfo);
      break;

    case M_DAC:
    case M_RST0:		/* these are all parameterless */
    case M_RST1:
    case M_RST2:
    case M_RST3:
    case M_RST4:
    case M_RST5:
    case M_RST6:
    case M_RST7:
    case M_TEM:
      TRACEMS1(cinfo->emethods, 1, "Unexpected marker 0x%02x", c);
      break;

    default:	/* must be DNL, DHP, EXP, APPn, JPGn, or RESn */
      skip_variable(cinfo, c);
      break;
    }
  }
}



/*
 * Initialize and read the file header (everything through the SOF marker).
 */

METHODDEF void
read_file_header (decompress_info_ptr cinfo)
{
  int c;

  /* Demand an SOI marker at the start of the file --- otherwise it's
   * probably not a JPEG file at all.  If the user interface wants to support
   * nonstandard headers in front of the SOI, it must skip over them itself
   * before calling jpeg_decompress().
   */
  if (JGETC(cinfo) != 0xFF  ||  JGETC(cinfo) != M_SOI)
    ERREXIT(cinfo->emethods, "Not a JPEG file");

  get_soi(cinfo);		/* OK, process SOI */

  /* Process markers until SOF */
  c = process_tables(cinfo);

  switch (c) {
  case M_SOF0:
  case M_SOF1:
    get_sof(cinfo, c);
    cinfo->arith_code = FALSE;
    break;
      
  case M_SOF9:
    get_sof(cinfo, c);
    cinfo->arith_code = TRUE;
    break;

  default:
    ERREXIT1(cinfo->emethods, "Unsupported SOF marker type 0x%02x", c);
    break;
  }

  /* Figure out what colorspace we have */
  /* (too bad the JPEG committee didn't provide a real way to specify this) */

  switch (cinfo->num_components) {
  case 1:
    cinfo->jpeg_color_space = CS_GRAYSCALE;
    break;

  case 3:
    /* if we saw a JFIF marker, leave it set to YCbCr; */
    /* also leave it alone if UI has provided a value */
    if (cinfo->jpeg_color_space == CS_UNKNOWN) {
      short cid0 = cinfo->comp_info[0].component_id;
      short cid1 = cinfo->comp_info[1].component_id;
      short cid2 = cinfo->comp_info[2].component_id;

      if (cid0 == 1 && cid1 == 2 && cid2 == 3)
	cinfo->jpeg_color_space = CS_YCbCr; /* assume it's JFIF w/out marker */
      else if (cid0 == 1 && cid1 == 4 && cid2 == 5)
	cinfo->jpeg_color_space = CS_YIQ; /* prototype's YIQ matrix */
      else {
	TRACEMS3(cinfo->emethods, 1,
		 "Unrecognized component IDs %d %d %d, assuming YCbCr",
		 cid0, cid1, cid2);
	cinfo->jpeg_color_space = CS_YCbCr;
      }
    }
    break;

  case 4:
    cinfo->jpeg_color_space = CS_CMYK;
    break;

  default:
    cinfo->jpeg_color_space = CS_UNKNOWN;
    break;
  }
}


/*
 * Read the start of a scan (everything through the SOS marker).
 * Return TRUE if find SOS, FALSE if find EOI.
 */

METHODDEF boolean
read_scan_header (decompress_info_ptr cinfo)
{
  int c;
  
  /* Process markers until SOS or EOI */
  c = process_tables(cinfo);
  
  switch (c) {
  case M_SOS:
    get_sos(cinfo);
    return TRUE;
    
  case M_EOI:
    TRACEMS(cinfo->emethods, 1, "End Of Image");
    return FALSE;

  default:
    ERREXIT1(cinfo->emethods, "Unexpected marker 0x%02x", c);
    break;
  }
  return FALSE;			/* keeps lint happy */
}


/*
 * The entropy decoder calls this routine if it finds a marker other than
 * the restart marker it was expecting.  (This code is *not* used unless
 * a nonzero restart interval has been declared.)  The passed parameter is
 * the marker code actually found (might be anything, except 0 or FF).
 * The desired restart marker is that indicated by cinfo->next_restart_num.
 * This routine is supposed to apply whatever error recovery strategy seems
 * appropriate in order to position the input stream to the next data segment.
 * For some file formats (eg, TIFF) extra information such as tile boundary
 * pointers may be available to help in this decision.
 *
 * This implementation is substantially constrained by wanting to treat the
 * input as a data stream; this means we can't back up.  (For instance, we
 * generally can't fseek() if the input is a Unix pipe.)  Therefore, we have
 * only the following actions to work with:
 *   1. Do nothing, let the entropy decoder resume at next byte of file.
 *   2. Read forward until we find another marker, discarding intervening
 *      data.  (In theory we could look ahead within the current bufferload,
 *      without having to discard data if we don't find the desired marker.
 *      This idea is not implemented here, in part because it makes behavior
 *      dependent on buffer size and chance buffer-boundary positions.)
 *   3. Push back the passed marker (with JUNGETC).  This will cause the
 *      entropy decoder to process an empty data segment, inserting dummy
 *      zeroes, and then re-read the marker we pushed back.
 * #2 is appropriate if we think the desired marker lies ahead, while #3 is
 * appropriate if the found marker is a future restart marker (indicating
 * that we have missed the desired restart marker, probably because it got
 * corrupted).

 * We apply #2 or #3 if the found marker is a restart marker no more than
 * two counts behind or ahead of the expected one.  We also apply #2 if the
 * found marker is not a legal JPEG marker code (it's certainly bogus data).
 * If the found marker is a restart marker more than 2 counts away, we do #1
 * (too much risk that the marker is erroneous; with luck we will be able to
 * resync at some future point).
 * For any valid non-restart JPEG marker, we apply #3.  This keeps us from
 * overrunning the end of a scan.  An implementation limited to single-scan
 * files might find it better to apply #2 for markers other than EOI, since
 * any other marker would have to be bogus data in that case.
 */

METHODDEF void
resync_to_restart (decompress_info_ptr cinfo, int marker)
{
  int desired = cinfo->next_restart_num;
  int action = 1;

  /* Always put up a warning. */
  WARNMS2(cinfo->emethods,
	  "Corrupt JPEG data: found 0x%02x marker instead of RST%d",
	  marker, desired);
  /* Outer loop handles repeated decision after scanning forward. */
  for (;;) {
    if (marker < M_SOF0)
      action = 2;		/* invalid marker */
    else if (marker < M_RST0 || marker > M_RST7)
      action = 3;		/* valid non-restart marker */
    else {
      if (marker == (M_RST0 + ((desired+1) & 7)) ||
	  marker == (M_RST0 + ((desired+2) & 7)))
	action = 3;		/* one of the next two expected restarts */
      else if (marker == (M_RST0 + ((desired-1) & 7)) ||
	       marker == (M_RST0 + ((desired-2) & 7)))
	action = 2;		/* a prior restart, so advance */
      else
	action = 1;		/* desired restart or too far away */
    }
    TRACEMS2(cinfo->emethods, 4,
	     "At marker 0x%02x, recovery action %d", marker, action);
    switch (action) {
    case 1:
      /* Let entropy decoder resume processing. */
      return;
    case 2:
      /* Scan to the next marker, and repeat the decision loop. */
      marker = next_marker(cinfo);
      break;
    case 3:
      /* Put back this marker & return. */
      /* Entropy decoder will be forced to process an empty segment. */
      JUNGETC(marker, cinfo);
      JUNGETC(0xFF, cinfo);
      return;
    }
  }
}


/*
 * Finish up after a compressed scan (series of read_jpeg_data calls);
 * prepare for another read_scan_header call.
 */

METHODDEF void
read_scan_trailer (decompress_info_ptr cinfo)
{
  /* no work needed */
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
read_file_trailer (decompress_info_ptr cinfo)
{
  /* no work needed */
}

	/************************************************************
	 *
	 * Code from example.c 
	 *
	 *************************************************************/

/*
 * These routines replace the default trace/error routines included with the
 * JPEG code.  The example trace_message routine shown here is actually the
 * same as the standard one, but you could modify it if you don't want messages
 * sent to stderr.  The example error_exit routine is set up to return
 * control to read_JPEG_file() rather than calling exit().  You can use the
 * same routines for both compression and decompression error recovery.
 */


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
/* This routine is used for any and all trace, debug, or error printouts
 * from the JPEG code.  The parameter is a printf format string; up to 8
 * integer data values for the format string have been stored in the
 * message_parm[] field of the external_methods struct.
 */

METHODDEF void
trace_message (const char *msgtext)
{
	sprintf(OMJPEGErrorString, msgtext,
	  emethods->message_parm[0], emethods->message_parm[1],
	  emethods->message_parm[2], emethods->message_parm[3],
	  emethods->message_parm[4], emethods->message_parm[5],
	  emethods->message_parm[6], emethods->message_parm[7]);
#ifdef JPEG_TRACE
	fprintf(stderr, "%s\n", OMJPEGErrorString);	/* there is no \n in the */
#endif
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
/*
 * The error_exit() routine should not return to its caller.  The default
 * routine calls exit(), but here we assume that we want to return to
 * read_JPEG_file, which has set up a setjmp context for the purpose.
 * You should make sure that the free_all method is called, either within
 * error_exit or after the return to the outer-level routine.
 */

METHODDEF void
error_exit (const char *msgtext)
{
  trace_message(msgtext);	/* report the error message */

  (*emethods->free_all) ();	/* clean up memory allocation & temp files */
	/*
	 * Set OMFI error code to signal a JPEG failure, unless set
	 * previously
	 */
	if (localJPEGStatus == OM_ERR_NONE)
		localJPEGStatus = OM_ERR_JPEGPROBLEM;

	OMJPEGErrorRaised = 1;	/* indicate that the message string is an
				 * error message */

  longjmp(setjmp_buffer, 1);	/* return control to outer routine */
}


/******************** JPEG COMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to feed data into the JPEG compressor.
 * We present a minimal version that does not worry about refinements such
 * as error recovery (the JPEG code will just exit() if it gets an error).
 */


/*
 * To supply the image data for compression, you must define three routines
 * input_init, get_input_row, and input_term.  These routines will be called
 * from the JPEG compressor via function pointer values that you store in the
 * cinfo data structure; hence they need not be globally visible and the exact
 * names don't matter.  (In fact, the "METHODDEF" macro expands to "static" if
 * you use the unmodified JPEG include files.)
 *
 * The input file reading modules (jrdppm.c, jrdgif.c, jrdtarga.c, etc) may be
 * useful examples of what these routines should actually do, although each of
 * them is encrusted with a lot of specialized code for its own file format.
 */


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
METHODDEF void
input_init (compress_info_ptr cinfo)
/* Initialize for input; return image size and component data. */
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
	omfMediaHdl_t		media = parms->media;
	omfFrameLayout_t	layout;
	omfPixelFormat_t	memFmt;
	omfInt16      		bitsPerPixel, numFields;
	omfInt32 fieldHeight, fieldWidth;
 	omfVideoMemOp_t		oplist[2];
	omfErr_t			functionJPEGStatus =	OM_ERR_NONE;
	
  /* This routine must return five pieces of information about the incoming
   * image, and must do any setup needed for the get_input_row routine.
   * The image information is returned in fields of the cinfo struct.
   * (If you don't care about modularity, you could initialize these fields
   * in the main JPEG calling routine, and make this routine be a no-op.)
   * We show some example values here.
   */
	functionJPEGStatus = omfmGetVideoInfo(media, &layout, &fieldWidth, &fieldHeight, NULL, &bitsPerPixel, &memFmt);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
	}

	cinfo->image_width = fieldWidth;	/* width in pixels */
	numFields = ((layout == kSeparateFields) ? 2 : 1);
	cinfo->image_height = fieldHeight;	/* height in pixels */


  /* JPEG views an image as being a rectangular array of pixels, with each
   * pixel having the same number of "component" values (color channels).
   * You must specify how many components there are and the colorspace
   * interpretation of the components.  Most applications will use RGB data or
   * grayscale data.  If you want to use something else, you'll need to study
   * and perhaps modify jcdeflts.c, jccolor.c, and jdcolor.c.
   */
  cinfo->input_components = 3;		/* or 1 for grayscale */
  if(media->pvt->pixType == kOmfPixRGBA)
  	cinfo->in_color_space = CS_RGB;
  else
  	cinfo->in_color_space = CS_YCbCr;	/* or CS_GRAYSCALE for grayscale */
  
	oplist[0].opcode = kOmfCDCICompWidth;
	oplist[1].opcode = kOmfVFmtEnd;
	functionJPEGStatus = omfmGetVideoInfoArray(media, oplist);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
	}
	cinfo->data_precision = (short) oplist[0].operand.expInt32;	/* bits per pixel component value */
  /* In the current JPEG software, data_precision must be set equal to
   * BITS_IN_JSAMPLE, which is 8 unless you twiddle jconfig.h.  Future
   * versions might allow you to say either 8 or 12 if compiled with
   * 12-bit JSAMPLEs, or up to 16 in lossless mode.  In any case,
   * it is up to you to scale incoming pixel values to the range
   *   0 .. (1<<data_precision)-1.
   * If your image data format is fixed at a byte per component,
   * then saying "8" is probably the best int -term solution.
   */
	cinfo->write_JFIF_header = TRUE;
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
/*
 * This function is called repeatedly and must supply the next row of pixels
 * on each call.  The rows MUST be returned in top-to-bottom order if you want
 * your JPEG files to be compatible with everyone else's.  (If you cannot
 * readily read your data in that order, you'll need an intermediate array to
 * hold the image.  See jrdtarga.c or jrdrle.c for examples of handling
 * bottom-to-top source data using the JPEG code's portable mechanisms.)
 * The data is to be returned into a 2-D array of JSAMPLEs, indexed as
 *		JSAMPLE pixel_row[component][column]
 * where component runs from 0 to cinfo->input_components-1, and column runs
 * from 0 to cinfo->image_width-1 (column 0 is left edge of image).  Note that
 * this is actually an array of pointers to arrays rather than a true 2D array,
 * since C does not support variable-size multidimensional arrays.
 * JSAMPLE is typically typedef'd as "unsigned char".
 */


METHODDEF void
                get_input_row(compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* Read next row of pixels into pixel_row[][] */
{
	/*
	 * This example shows how you might read RGB data (3 components) from
	 * an input file in which the data is stored 3 bytes per pixel in
	 * left-to-right, top-to-bottom order.
	 */
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
	register JSAMPROW ptr0, ptr1, ptr2;
	register int    col;

	ptr0 = pixel_row[0];
	ptr1 = pixel_row[1];
	ptr2 = pixel_row[2];
	for (col = 0; col < cinfo->image_width; col++)
	{
		*ptr0++ = (JSAMPLE) ((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++];	/* red */
		*ptr1++ = (JSAMPLE) ((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++];	/* green */
		*ptr2++ = (JSAMPLE) ((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++];	/* blue */
	}
	
	ptr0 = pixel_row[0];		/* DEBUG: Keep these values around */
	ptr1 = pixel_row[1];
	ptr2 = pixel_row[2];
	
}


METHODDEF void
input_term (compress_info_ptr cinfo)
/* Finish up at the end of the input */
{
  /* This termination routine will very often have no work to do, */
  /* but you must provide it anyway. */
  /* Note that the JPEG code will only call it during successful exit; */
  /* if you want it called during error exit, you gotta do that yourself. */
}


/*
 * That's it for the routines that deal with reading the input image data.
 * Now we have overall control and parameter selection routines.
 */


/*
 * This routine must determine what output JPEG file format is to be written,
 * and make any other compression parameter changes that are desirable.
 * This routine gets control after the input file header has been read
 * (i.e., right after input_init has been called).  You could combine its
 * functions into input_init, or even into the main control routine, but
 * if you have several different input_init routines, it's a definite win
 * to keep this separate.  You MUST supply this routine even if it's a no-op.
 */

METHODDEF void
c_ui_method_selection (compress_info_ptr cinfo)
{
  /* If the input is gray scale, generate a monochrome JPEG file. */
  if (cinfo->in_color_space == CS_GRAYSCALE)
    j_monochrome_default(cinfo);
  /* For now, always select JFIF output format. */
  
  cinfo->methods->write_file_header = write_file_header;
  cinfo->methods->write_scan_header = write_scan_header;
  cinfo->methods->write_jpeg_data = write_jpeg_data;
  cinfo->methods->write_scan_trailer = write_scan_trailer;
  cinfo->methods->write_file_trailer = write_file_trailer;
}





/******************** JPEG DECOMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to read data from the JPEG decompressor.
 * It's a little more refined than the above in that we show how to do your
 * own error recovery.  If you don't care about that, you don't need these
 * next two routines.
 */





/*
 * To accept the image data from decompression, you must define four routines
 * output_init, put_color_map, put_pixel_rows, and output_term.
 *
 * You must understand the distinction between full color output mode
 * (N independent color components) and colormapped output mode (a single
 * output component representing an index into a color map).  You should use
 * colormapped mode to write to a colormapped display screen or output file.
 * Colormapped mode is also useful for reducing grayscale output to a small
 * number of gray levels: when using the 1-pass quantizer on grayscale data,
 * the colormap entries will be evenly spaced from 0 to MAX_JSAMPLE, so you
 * can regard the indexes are directly representing gray levels at reduced
 * precision.  In any other case, you should not depend on the colormap
 * entries having any particular order.
 * To get colormapped output, set cinfo->quantize_colors to TRUE and set
 * cinfo->desired_number_of_colors to the maximum number of entries in the
 * colormap.  This can be done either in your main routine or in
 * d_ui_method_selection.  For grayscale quantization, also set
 * cinfo->two_pass_quantize to FALSE to ensure the 1-pass quantizer is used
 * (presently this is the default, but it may not be so in the future).
 *
 * The output file writing modules (jwrppm.c, jwrgif.c, jwrtarga.c, etc) may be
 * useful examples of what these routines should actually do, although each of
 * them is encrusted with a lot of specialized code for its own file format.
 */


METHODDEF void
output_init (decompress_info_ptr cinfo)
/* This routine should do any setup required */
{
  /* This routine can initialize for output based on the data passed in cinfo.
   * Useful fields include:
   *	image_width, image_height	Pretty obvious, I hope.
   *	data_precision			bits per pixel value; typically 8.
   *	out_color_space			output colorspace previously requested
   *	color_out_comps			number of color components in same
   *	final_out_comps			number of components actually output
   * final_out_comps is 1 if quantize_colors is true, else it is equal to
   * color_out_comps.
   *
   * If you have requested color quantization, the colormap is NOT yet set.
   * You may wish to defer output initialization until put_color_map is called.
   */
}


/*
 * This routine is called if and only if you have set cinfo->quantize_colors
 * to TRUE.  It is given the selected colormap and can complete any required
 * initialization.  This call will occur after output_init and before any
 * calls to put_pixel_rows.  Note that the colormap pointer is also placed
 * in a cinfo field, whence it can be used by put_pixel_rows or output_term.
 * num_colors will be less than or equal to desired_number_of_colors.
 *
 * The colormap data is supplied as a 2-D array of JSAMPLEs, indexed as
 *		JSAMPLE colormap[component][indexvalue]
 * where component runs from 0 to cinfo->color_out_comps-1, and indexvalue
 * runs from 0 to num_colors-1.  Note that this is actually an array of
 * pointers to arrays rather than a true 2D array, since C does not support
 * variable-size multidimensional arrays.
 * JSAMPLE is typically typedef'd as "unsigned char".  If you want your code
 * to be as portable as the JPEG code proper, you should always access JSAMPLE
 * values with the GETJSAMPLE() macro, which will do the right thing if the
 * machine has only signed chars.
 */

METHODDEF void
put_color_map (decompress_info_ptr dinfo, int num_colors, JSAMPARRAY colormap)
/* Write the color map */
{
  /* You need not provide this routine if you always set cinfo->quantize_colors
   * FALSE; but a safer practice is to provide it and have it just print an
   * error message, like this:
   */
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)dinfo->input_file;
	omfMediaHdl_t   	media = parms->media;
	/*
	 * You need not provide this routine if you always set
	 * dinfo->quantize_colors FALSE; but a safer practice is to provide
	 * it and have it just print an error message, like this:
	 * fprintf(stderr, "put_color_map called: there's a bug here
	 * somewhere!\n");
	 */
	XPROTECT(media->mainFile)
	{
		RAISE(OM_ERR_JPEGPCM);
	}
	XEXCEPT
	XEND_VOID
	
	return;
}


/*
 * This function is called repeatedly, with a few more rows of pixels supplied
 * on each call.  With the current JPEG code, some multiple of 8 rows will be
 * passed on each call except the last, but it is extremely bad form to depend
 * on this.  You CAN assume num_rows > 0.
 * The data is supplied in top-to-bottom row order (the standard order within
 * a JPEG file).  If you cannot readily use the data in that order, you'll
 * need an intermediate array to hold the image.  See jwrrle.c for an example
 * of outputting data in bottom-to-top order.
 *
 * The data is supplied as a 3-D array of JSAMPLEs, indexed as
 *		JSAMPLE pixel_data[component][row][column]
 * where component runs from 0 to cinfo->final_out_comps-1, row runs from 0 to
 * num_rows-1, and column runs from 0 to cinfo->image_width-1 (column 0 is
 * left edge of image).  Note that this is actually an array of pointers to
 * pointers to arrays rather than a true 3D array, since C does not support
 * variable-size multidimensional arrays.
 * JSAMPLE is typically typedef'd as "unsigned char".  If you want your code
 * to be as portable as the JPEG code proper, you should always access JSAMPLE
 * values with the GETJSAMPLE() macro, which will do the right thing if the
 * machine has only signed chars.
 *
 * If quantize_colors is true, then there is only one component, and its values
 * are indexes into the previously supplied colormap.  Otherwise the values
 * are actual data in your selected output colorspace.
 */


METHODDEF void
put_pixel_rows (decompress_info_ptr dinfo, int num_rows, JSAMPIMAGE pixel_data)
/* Write some rows of output data */
{
	/*
	 * This example shows how you might write full-color RGB data (3
	 * components) to an output file in which the data is stored 3 bytes
	 * per pixel.
	 */
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)dinfo->input_file;
	register JSAMPROW ptr0, ptr1, ptr2;
	register int    col;
	register int    row;

	for (row = 0; row < num_rows; row++)
	{
		ptr0 = pixel_data[0][row];
		ptr1 = pixel_data[1][row];
		ptr2 = pixel_data[2][row];
		for (col = 0; col < dinfo->image_width; col++)
		{

			((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++] = GETJSAMPLE(*ptr0);	/* red */
			ptr0++;

			((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++] = GETJSAMPLE(*ptr1);	/* green */
			ptr1++;

			((PIXEL *) parms->pixelBuffer)[parms->pixelBufferIndex++] = GETJSAMPLE(*ptr2);	/* blue */
			ptr2++;

		}
	}
}


METHODDEF void
output_term (decompress_info_ptr cinfo)
/* Finish up at the end of the output */
{
  /* This termination routine may not need to do anything. */
  /* Note that the JPEG code will only call it during successful exit; */
  /* if you want it called during error exit, you gotta do that yourself. */
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
/*
 * Initialize and read the file header (everything through the SOF marker).
 */

static void     loadCompressionTable(omfMediaHdl_t media, jpeg_tables * table, omfInt32 n)
{
	omfJPEGTables_t aTable;
	omfInt16 m;
	omfErr_t			functionJPEGStatus =	OM_ERR_NONE;

	aTable.JPEGcomp = (omfJPEGcomponent_t) n;
	functionJPEGStatus = codecGetInfo(media, kJPEGTables, media->pictureKind, sizeof(omfJPEGTables_t), &aTable);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
	}
	table->Qlen = aTable.QTableSize;
	table->Q16len = aTable.QTableSize;
	table->DClen = aTable.DCTableSize;
	table->AClen = aTable.DCTableSize;
	memcpy(table->Q, aTable.QTables, aTable.QTableSize);
	for(m = 0; m < aTable.QTableSize; m++)
	  table->Q16[m] = aTable.QTables[m];
	memcpy(table->DC, aTable.DCTables, aTable.DCTableSize);
	memcpy(table->AC, aTable.ACTables, aTable.ACTableSize);
}

/*
 * That's it for the routines that deal with writing the output image.
 * Now we have overall control and parameter selection routines.
 */


/*
 * This routine gets control after the JPEG file header has been read;
 * at this point the image size and colorspace are known.
 * The routine must determine what output routines are to be used, and make
 * any decompression parameter changes that are desirable.  For example,
 * if it is found that the JPEG file is grayscale, you might want to do
 * things differently than if it is color.  You can also delay setting
 * quantize_colors and associated options until this point. 
 *
 * j_d_defaults initializes out_color_space to CS_RGB.  If you want grayscale
 * output you should set out_color_space to CS_GRAYSCALE.  Note that you can
 * force grayscale output from a color JPEG file (though not vice versa).
 */

METHODDEF void
d_ui_method_selection (decompress_info_ptr cinfo)
{
	JPEG_MediaParms_t   *parms = (JPEG_MediaParms_t *)cinfo->input_file;
	omfMediaHdl_t		media = parms->media;
  /* if grayscale input, force grayscale output; */
  /* else leave the output colorspace as set by main routine. */
  if (cinfo->jpeg_color_space == CS_GRAYSCALE)
    cinfo->out_color_space = CS_GRAYSCALE;
 else if(media->pvt->pixType == kOmfPixRGBA)
  	cinfo->out_color_space = CS_RGB;
  else
  	cinfo->out_color_space = CS_YCbCr;

  /* select output routines */
  cinfo->methods->output_init = output_init;
  cinfo->methods->put_color_map = put_color_map;
  cinfo->methods->put_pixel_rows = put_pixel_rows;
  cinfo->methods->output_term = output_term;
}


/*
 * OK, here is the main function that actually causes everything to happen.
 * We assume here that the JPEG filename is supplied by the caller of this
 * routine, and that all decompression parameters can be default values.
 * The routine returns 1 if successful, 0 if not.
 */


GLOBAL omfErr_t
omfmJFIFDecompressSample (JPEG_MediaParms_t *parms)
{
  /* These three structs contain JPEG parameters and working data.
   * They must survive for the duration of parameter setup and one
   * call to jpeg_decompress; typically, making them local data in the
   * calling routine is the best strategy.
   */
  struct Decompress_info_struct dinfo;
  struct Decompress_methods_struct dc_methods;
  struct External_methods_struct e_methods;
  	omfMediaHdl_t	media = parms->media;
  	omfUInt32		i, range, offset, upper;
  	

	omfAssertMediaHdl(media);
	XPROTECT(media->mainFile)
	{
  /* Select the input and output files.
   * In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * Note that cinfo.output_file is only used if your output handling routines
   * use it; otherwise, you can just make it NULL.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

		dinfo.input_file = (FILE *) parms;
		dinfo.output_file = (FILE *) parms;

  /* Initialize the system-dependent method pointers. */
  dinfo.methods = &dc_methods;	/* links to method structs */
  dinfo.emethods = &e_methods;
  /* Here we supply our own error handler; compare to use of standard error
   * handler in the previous write_JPEG_file example.
   */
  emethods = &e_methods;	/* save struct addr for possible access */
  e_methods.error_exit = error_exit; /* supply error-exit routine */
  e_methods.trace_message = trace_message; /* supply trace-message routine */
  e_methods.trace_level = 0;	/* default = no tracing */
  e_methods.num_warnings = 0;	/* no warnings emitted yet */
  e_methods.first_warning_level = 0; /* display first corrupt-data warning */
  e_methods.more_warning_level = 3; /* but suppress additional ones */

  /* prepare setjmp context for possible exit from error_exit */
  if (setjmp(setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * Memory allocation has already been cleaned up (see free_all call in
     * error_exit), but we need to close the input file before returning.
     * You might also need to close an output file, etc.
     */
			RAISE(OM_ERR_DECOMPRESS);
  }

  /* Here we use the standard memory manager provided with the JPEG code.
   * In some cases you might want to replace the memory manager, or at
   * least the system-dependent part of it, with your own code.
   */
  jselmemmgr(&e_methods);	/* select std memory allocation routines */
  /* If the decompressor requires full-image buffers (for two-pass color
   * quantization or a noninterleaved JPEG file), it will create temporary
   * files for anything that doesn't fit within the maximum-memory setting.
   * You can change the default maximum-memory setting by changing
   * e_methods.max_memory_to_use after jselmemmgr returns.
   * On some systems you may also need to set up a signal handler to
   * ensure that temporary files are deleted if the program is interrupted.
   * (This is most important if you are on MS-DOS and use the jmemdos.c
   * memory manager back end; it will try to grab extended memory for
   * temp files, and that space will NOT be freed automatically.)
   * See jcmain.c or jdmain.c for an example signal handler.
   */

  /* Here, set up the pointer to your own routine for post-header-reading
   * parameter selection.  You could also initialize the pointers to the
   * output data handling routines here, if they are not dependent on the
   * image type.
   */
  dc_methods.d_ui_method_selection = d_ui_method_selection;

  /* Set up default decompression parameters. */
  j_d_defaults(&dinfo, TRUE);
  /* TRUE indicates that an input buffer should be allocated.
   * In unusual cases you may want to allocate the input buffer yourself;
   * see jddeflts.c for commentary.
   */

  /* At this point you can modify the default parameters set by j_d_defaults
   * as needed; for example, you can request color quantization or force
   * grayscale output.  See jdmain.c for examples of what you might change.
   */

  /* Set up to read a JFIF or baseline-JPEG file. */
  /* This is the only JPEG file format currently supported. */
  dinfo.methods->read_file_header = read_file_header;
  dinfo.methods->read_scan_header = read_scan_header;
  /* For JFIF/raw-JPEG format, the user interface supplies read_jpeg_data. */
  dinfo.methods->read_jpeg_data = omjpeg_read_jpeg_data;
  dinfo.methods->resync_to_restart = resync_to_restart;
  dinfo.methods->read_scan_trailer = read_scan_trailer;
  dinfo.methods->read_file_trailer = read_file_trailer;
  
	/* Set up CCIR ChromaMap if whitelevel/blackLevel/chroma not full range
	 */
	if((parms->blackLevel != 0) || (parms->whiteLevel != 255) || (parms->colorRange < 254))
	{ 
		offset = parms->blackLevel;
		range = (parms->whiteLevel+1)-parms->blackLevel;
		dinfo.CCIRLumaOutMap = (JSAMPLE *) (*dinfo.emethods->alloc_small) (256 * SIZEOF(JCOEF));
		for (i = 0; i < 256; i++)
		{
			if(i <= parms->blackLevel) 
				dinfo.CCIRLumaOutMap[i] = 0;
			else if(i >= parms->whiteLevel) 
				dinfo.CCIRLumaOutMap[i] = 255;
			else 
				dinfo.CCIRLumaOutMap[i] = (JSAMPLE)(((i-offset) * 255) / 219.0 + .5);
		}
		
		offset = 128 - (parms->colorRange/2);
		range = parms->colorRange;
		upper = offset + range;
		dinfo.CCIRChromaOutMap = (JSAMPLE *) (*dinfo.emethods->alloc_small) (256 * SIZEOF(JCOEF));
		for (i = 0; i < 256; i++)
		{
			if(i <= offset) 
				dinfo.CCIRChromaOutMap[i] = 0;
			/* LF-W, change bound from 242 to 241 to fix overflow bug */
			else if(i >= upper) 
				dinfo.CCIRChromaOutMap[i] = 255;
			else 
				dinfo.CCIRChromaOutMap[i] = (JSAMPLE)(((i-offset) * 256) / range + .5);
		dinfo.CCIR = TRUE;
		}
	}

	/* Here we go! */
	jpeg_decompress(&dinfo);

	/* back out the remaining compressed bytes */
	omcSeekStreamRelative(media->stream, -1 * dinfo.bytes_in_buffer);

  /* That's it, son.  Nothin' else to do, except close files. */
  /* Here we assume only the input file need be closed. */
	}
	XEXCEPT
	XEND

  /* You might want to test e_methods.num_warnings to see if bad data was
   * detected.  In this example, we just blindly forge ahead.
   */
	return OM_ERR_NONE;	/* indicate success */

  /* Note: if you want to decompress more than one image, we recommend you
   * repeat this whole routine.  You MUST repeat the j_d_defaults()/alter
   * parameters/jpeg_decompress() sequence, as some data structures allocated
   * in j_d_defaults are freed upon exit from jpeg_decompress.
   */
}

/*
 * OK, here is the main function that actually causes everything to happen.
 * We assume here that the target filename is supplied by the caller of this
 * routine, and that all JPEG compression parameters can be default values.
 */

GLOBAL omfErr_t
omfmJFIFCompressSample (JPEG_MediaParms_t *parms, omfBool customTables, omfInt32 * sampleSize)
{
  /* These three structs contain JPEG parameters and working data.
   * They must survive for the duration of parameter setup and one
   * call to jpeg_compress; typically, making them local data in the
   * calling routine is the best strategy.
   */
  struct Compress_info_struct cinfo;
  struct Compress_methods_struct c_methods;
  struct External_methods_struct e_methods;
  omfInt16           bitsPerPixel, n;
  omfInt32 fieldHeight, fieldWidth;
  	omfMediaHdl_t	media = parms->media;
	omfFrameLayout_t layout;
	omfPixelFormat_t memFmt;
	jpeg_tables     compressionTables[3];
/*	omfJPEGInfo_t   info;
      ROGER--- cut this piece when I'm sure it is not needed    
 */
	omfInt32           ci, i, count;
	QUANT_TBL_PTR   quant_ptr;
	omfUInt8          *table_ptr;
	omfUInt8           bits[17];
	HUFF_TBL      **htblptr;
	omfUInt8           huffval[256];
	omfErr_t			functionJPEGStatus =	OM_ERR_NONE;

	omfAssertMediaHdl(media);
	functionJPEGStatus = omfmGetVideoInfo(media, &layout, &fieldWidth, &fieldHeight, NULL, &bitsPerPixel, &memFmt);
	if (functionJPEGStatus !=	OM_ERR_NONE)
	{
		if  (localJPEGStatus == OM_ERR_NONE)
			localJPEGStatus = functionJPEGStatus;
	}

/*	localJPEGStatus = codecGetInfo(media, kCompressionParms, media->pictureKind, sizeof(omfJPEGInfo_t), &info);
      ROGER--- cut this piece when I'm sure it is not needed    
 */
	if(customTables)
	{
		for (n = 0; n < 3; n++)
			loadCompressionTable(media, compressionTables + n, n);
	}

  /* Initialize the system-dependent method pointers. */
  cinfo.methods = &c_methods;	/* links to method structs */
  cinfo.emethods = &e_methods;
  /* Here we use the default JPEG error handler, which will just print
   * an error message on stderr and call exit().  See the second half of
   * this file for an example of more graceful error recovery.
   */
  jselerror(&e_methods);	/* select std error/trace message routines */
  /* Here we use the standard memory manager provided with the JPEG code.
   * In some cases you might want to replace the memory manager, or at
   * least the system-dependent part of it, with your own code.
   */
  jselmemmgr(&e_methods);	/* select std memory allocation routines */
  /* If the compressor requires full-image buffers (for entropy-coding
   * optimization or a noninterleaved JPEG file), it will create temporary
   * files for anything that doesn't fit within the maximum-memory setting.
   * (Note that temp files are NOT needed if you use the default parameters.)
   * You can change the default maximum-memory setting by changing
   * e_methods.max_memory_to_use after jselmemmgr returns.
   * On some systems you may also need to set up a signal handler to
   * ensure that temporary files are deleted if the program is interrupted.
   * (This is most important if you are on MS-DOS and use the jmemdos.c
   * memory manager back end; it will try to grab extended memory for
   * temp files, and that space will NOT be freed automatically.)
   * See jcmain.c or jdmain.c for an example signal handler.
   */

  /* Here, set up pointers to your own routines for input data handling
   * and post-init parameter selection.
   */
  c_methods.input_init = input_init;
  c_methods.get_input_row = get_input_row;
  c_methods.input_term = input_term;
  c_methods.c_ui_method_selection = c_ui_method_selection;

  /* Set up default JPEG parameters in the cinfo data structure. */
  j_c_defaults(&cinfo, 75, FALSE);
  /* Note: 75 is the recommended default quality level; you may instead pass
   * a user-specified quality level.  Be aware that values below 25 will cause
   * non-baseline JPEG files to be created (and a warning message to that
   * effect to be emitted on stderr).  This won't bother our decoder, but some
   * commercial JPEG implementations may choke on non-baseline JPEG files.
   * If you want to force baseline compatibility, pass TRUE instead of FALSE.
   * (If non-baseline files are fine, but you could do without that warning
   * message, set e_methods.trace_level to -1.)
   */

  /* At this point you can modify the default parameters set by j_c_defaults
   * as needed.  For a minimal implementation, you shouldn't need to change
   * anything.  See jcmain.c for some examples of what you might change.
   */
	if(customTables)
	{
		/*
		 * At this point you can modify the default parameters set by
		 * j_c_defaults as needed.  For a minimal implementation, you
		 * shouldn't need to change anything.  See jcmain.c for some examples
		 * of what you might change.
		 */
	
		cinfo.comp_info[0].h_samp_factor = 2;	/* OMFI specifies 2h1v */
		cinfo.comp_info[0].v_samp_factor = 1;
	
	
		if (cinfo.quant_tbl_ptrs[0] == NULL)
			cinfo.quant_tbl_ptrs[0] = (QUANT_TBL_PTR) (*cinfo.emethods->alloc_small) (SIZEOF(QUANT_TBL));
		quant_ptr = cinfo.quant_tbl_ptrs[0];
		table_ptr = customTables ? compressionTables[0].Q : STD_QT_PTR[0];
	
		for (i = 0; i < DCTSIZE2; i++)
			quant_ptr[i] = table_ptr[i];
	
		if (cinfo.quant_tbl_ptrs[1] == NULL)
			cinfo.quant_tbl_ptrs[1] = (QUANT_TBL_PTR) (*cinfo.emethods->alloc_small) (SIZEOF(QUANT_TBL));
		quant_ptr = cinfo.quant_tbl_ptrs[1];
		table_ptr = customTables ? compressionTables[1].Q : STD_QT_PTR[1];
	
		for (i = 0; i < DCTSIZE2; i++)
			quant_ptr[i] = table_ptr[i];
	
		/* Set up two Huffman tables, one for luminance, one for chrominance */
		for (ci = 0; ci < 2; ci++)
		{
			omfUInt8          *hv;
	
			/* DC table */
			bits[0] = 0;
			count = 0;
			hv = customTables ? compressionTables[ci].DC : STD_DCT_PTR[ci];
	
			for (i = 1; i <= 16; i++)
			{
				bits[i] = (UINT8) * hv++;
				count += bits[i];
			}
			if (count > 16)
				ERREXIT(cinfo.emethods, "Bogus DC count");
	
			for (i = 0; i < count; i++)
				huffval[i] = (UINT8) * hv++;
	
			htblptr = &cinfo.dc_huff_tbl_ptrs[ci];
	
			if (*htblptr == NULL)
				*htblptr = (HUFF_TBL *) (*cinfo.emethods->alloc_small) (SIZEOF(HUFF_TBL));
	
			MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
			MEMCOPY((*htblptr)->huffval, huffval, SIZEOF((*htblptr)->huffval));
	
			/* AC table */
			bits[0] = 0;
			count = 0;
			hv = customTables ? compressionTables[ci].AC : STD_ACT_PTR[ci];
	
			for (i = 1; i <= 16; i++)
			{
				bits[i] = (UINT8) * hv++;
				count += bits[i];
			}
	
			if (count > 256)
				ERREXIT(cinfo.emethods, "Bogus AC count");
	
			for (i = 0; i < count; i++)
				huffval[i] = (UINT8) * hv++;
	
			htblptr = &cinfo.ac_huff_tbl_ptrs[ci];
	
			if (*htblptr == NULL)
				*htblptr = (HUFF_TBL *) (*cinfo.emethods->alloc_small) (SIZEOF(HUFF_TBL));
	
			MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
			MEMCOPY((*htblptr)->huffval, huffval, SIZEOF((*htblptr)->huffval));
		}
	}
	/* Set up CCIR ChromaMap if whitelevel/blackLevel/chroma not full range
     */
    if((parms->blackLevel != 0) || (parms->whiteLevel != 255) || (parms->colorRange < 254))
    { 
		cinfo.CCIRLumaInMap = (JSAMPLE *) (*cinfo.emethods->alloc_small) (256 * SIZEOF(JCOEF));
		for (i = 0; i < 256; i++)
			cinfo.CCIRLumaInMap[i] = (omfUInt8)((i * ((parms->whiteLevel+1)-parms->blackLevel) / 256)
												+ parms->blackLevel);
		
		cinfo.CCIRChromaInMap = (JSAMPLE *) (*cinfo.emethods->alloc_small) (256 * SIZEOF(JCOEF));
		for (i = 0; i < 256; i++)
			cinfo.CCIRChromaInMap[i] = (omfUInt8)(((i * parms->colorRange) / 256)
												+ (128-(parms->colorRange/2)));
		cinfo.CCIR = TRUE;
	}
	
	/*
	 * Select the input and output files. Note that cinfo.input_file is
	 * only used if your input reading routines use it; otherwise, you
	 * can just make it NULL. VERY IMPORTANT: use "b" option to fopen()
	 * if you are on a machine that requires it in order to write binary
	 * files.
	 */

	cinfo.input_file = (FILE *) parms;
	cinfo.output_file = (FILE *) parms;

	/* Here we go! */
	if (setjmp(setjmp_buffer) == 0)
	{
		jpeg_compress(&cinfo);
	}

  /* That's it, son.  Nothin' else to do, except close files. */
  /* Here we assume only the output file need be closed. */
	/*
	 * Note: if you want to compress more than one image, we recommend
	 * you repeat this whole routine.  You MUST repeat the
	 * j_c_defaults()/alter parameters/jpeg_compress() sequence, as some
	 * data structures allocated in j_c_defaults are freed upon exit from
	 * jpeg_compress.
	 */
	if (localJPEGStatus == OM_ERR_NONE)
		*sampleSize = fieldWidth * fieldHeight * (bitsPerPixel / 8);
	else
		*sampleSize = 0;

	XPROTECT(media->mainFile)
	{
		if (localJPEGStatus != OM_ERR_NONE)
			RAISE(localJPEGStatus);
	}
	XEXCEPT
	XEND

	return(OM_ERR_NONE);
}
#else
/* OMFI_JPEG_CODEC */

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
omfErr_t        omfmJFIFCompressSample(JPEG_MediaParms_t *parms,  omfBool customTables, omfInt32 * sampleSize)
{
	return (OM_ERR_JPEGDISABLED);
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
omfErr_t        omfmJFIFDecompressSample(JPEG_MediaParms_t *parms)
{
	return (OM_ERR_JPEGDISABLED);
}

#endif

/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
