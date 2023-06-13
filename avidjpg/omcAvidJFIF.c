#include <string.h>
#include "omPublic.h"
#include "omcAvidJFIF.h" 
#include "omCodId.h"
#include "omcJPEG.h" 
#include "omPvt.h"

omfErr_t        omfCodecAvidJFIF(omfCodecFunctions_t func, omfCodecParms_t * info)
{
	if(func == kCodecInit)
	{
		omfsNewClass(info->spc.init.session, kClsRegistered, kOmfTstRevEither,
			"JPED", "CDCI");	/* This may fail if already registered */
		return(omfCodecJPEG(func, info));
	}
	else if(func == kCodecGetMetaInfo)
	{
		strncpy((char *)info->spc.metaInfo.name, "Avid JFIF Codec (Compressed ONLY)", info->spc.metaInfo.nameLen);
		info->spc.metaInfo.info.mdesClassID = "JPED";
		info->spc.metaInfo.info.dataClassID = "JPEG";
		info->spc.metaInfo.info.codecID = CODEC_AVID_JFIF_VIDEO;
		info->spc.metaInfo.info.rev = CODEC_REV_3;
		info->spc.metaInfo.info.dataKindNameList = PICTUREKIND;
		info->spc.metaInfo.info.minFileRev = kOmfRev2x;
		info->spc.metaInfo.info.maxFileRevIsValid = FALSE;		/* There is no maximum rev */
		return (OM_ERR_NONE);
	}
	else
		return(omfCodecJPEG(func, info));
}

