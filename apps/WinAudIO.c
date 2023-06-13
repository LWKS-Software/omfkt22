/*
 * Name: WinAudIO.c
 *
 * Function: Windows audio support for  play, in order to remove excess
 *			baggage from the examples.
 *
 *			Currently restricted to playing mono.
 */

#include "portvars.h"
#include <windows.h>
#include <stdio.h>
#include <String.h>
#include <Windef.h>
#include <MMSYSTEM.H>
#include <MMREG.H>
#include <malloc.h>
#include <math.h>

#include "omPublic.h"

#include "WinAudIO.h"

struct winAudioInfo
{
    WAVEFORMATEX   *pFormat ;
	HWAVEOUT       hWaveOut ;
};

omfErr_t MyInitAudio(omfRational_t rate,
					omfInt32 sampleSize,
					omfInt32 numCh,
					audioInfoPtr_t *handleRtn)
{
	audioInfoPtr_t  hdl         ;
	omfInt32		intRate     ;
	UINT            num_devs	;
	UINT        	target_dev  ;
	MMRESULT        res         ;
	int             success = 0 ;
/*    HANDLE          hFormat	    ;*/
	LPWAVEOUTCAPS   caps_stuff  ;
	
	hdl = (struct winAudioInfo *)omfsMalloc(sizeof(struct winAudioInfo)) ;

	caps_stuff = omfsMalloc(sizeof(WAVEOUTCAPS));
	hdl->pFormat = omfsMalloc(sizeof(WAVEFORMATEX));

	hdl->pFormat->wFormatTag = WAVE_FORMAT_PCM ;
	hdl->pFormat->nChannels = (unsigned short)numCh ;
	hdl->pFormat->wBitsPerSample = (unsigned short)sampleSize ;
	hdl->pFormat->nBlockAlign =  hdl->pFormat->nChannels * hdl->pFormat->wBitsPerSample  / 8 ;

/*	if(rate.denominator == 1)
		hdl->pFormat->nSamplesPerSec = (rate.numerator & 0xFFFF) << 16L ;
	else 
*/
	
		intRate = rate.numerator / rate.denominator;
		if(intRate == 22254)
			hdl->pFormat->nSamplesPerSec = 22050;
		else if(intRate == 11127)
			hdl->pFormat->nSamplesPerSec = 11025;
		else
			hdl->pFormat->nSamplesPerSec = intRate;
	

	hdl->pFormat->nAvgBytesPerSec =hdl->pFormat->nSamplesPerSec * hdl->pFormat->nBlockAlign;
	hdl->pFormat->cbSize = 0 ;

	num_devs = waveOutGetNumDevs()	 ; 

	target_dev = 0 ;
	while (target_dev < num_devs)
	{

		res =  waveOutGetDevCaps(target_dev,caps_stuff, sizeof(WAVEOUTCAPS));
		if (res != MMSYSERR_NOERROR	)
			printf("error on waveoutgetdevcaps\n") ;
		
		printf("%s\n",caps_stuff->szPname ) ;

		if(	(caps_stuff->dwFormats & WAVE_FORMAT_1M08) &&
			(caps_stuff->dwFormats & WAVE_FORMAT_1M16) &&
			(caps_stuff->dwFormats & WAVE_FORMAT_2M08) &&
			(caps_stuff->dwFormats & WAVE_FORMAT_2M16) &&
			(caps_stuff->dwFormats & WAVE_FORMAT_4M08) &&
			(caps_stuff->dwFormats & WAVE_FORMAT_4M16) )
		{
			/*
			printf("in the break section\n") ;
			printf("target_dev = %d\n",target_dev) ;
			*/

			break ;
		}
		/*
		printf("does the target_device++\n") ;
		printf("target_dev = %d\n",target_dev) ;
		*/
		target_dev++ ;
	}

	res =  waveOutOpen((LPHWAVEOUT)&hdl->hWaveOut, target_dev,hdl->pFormat, 0,0,WAVE_MAPPED); 
 	if (res != MMSYSERR_NOERROR	)
	{
/*		LocalUnlock(hFormat); */
/*		LocalFree(hFormat);*/
		printf("res = %d\n",res) ;
		printf("error on waveoutopen\n") ;
		return(OM_ERR_TEST_FAILED); 
	}
	
	*handleRtn = hdl ;

	return(OM_ERR_NONE);
}

omfInt32 MyGetInterfaceSampleWidth(void)
{
		return(16);
}

omfErr_t MyRecordData(	
			omfInt32		*bytesPerSample,
			char			*buffer,
			omfUInt32		buflen,
			omfUInt32		*numSamples,
			omfRational_t	*audioRate,
			omfInt16		*sampleSize,
			omfInt16		*numChannels)
{

	return(OM_ERR_NONE);
}

omfErr_t MyPlaySamples(
					   audioInfoPtr_t handle,
					   char           *buffer,
					   omfUInt32      numSamples)
{
    HGLOBAL     hWaveHdr	   ; 
    UINT        wResult		   ; 
	int			bytesPerSample ;
	LPWAVEHDR   lpWaveHdr      ;


	/* 
	Allocate and lock memory for the header. This memory must 
	also be globally allocated with GMEM_MOVEABLE and 
	GMEM_SHARE flags. 
	*/
	
	bytesPerSample = (handle->pFormat->wBitsPerSample + 7)/8;

	hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, 
		(DWORD) sizeof(WAVEHDR));
 
	lpWaveHdr = (LPWAVEHDR) GlobalLock(hWaveHdr); 
	if (lpWaveHdr == NULL) 
	{ 
		printf("Failed to lock memory for header."); 
		return(OM_ERR_TEST_FAILED); 
	} 
 		 
	/* After allocation, set up and prepare header. */ 
 
	lpWaveHdr->lpData = buffer ; 
	lpWaveHdr->dwBufferLength = numSamples * bytesPerSample; 
	lpWaveHdr->dwFlags = 0L; 
	lpWaveHdr->dwLoops = 0L;


	/* 
	Now the data block can be sent to the output device. The 
	waveOutWrite function returns immediately and waveform 
	data is sent to the output device in the background. 
	*/ 

	waveOutPrepareHeader(handle->hWaveOut,lpWaveHdr,sizeof(WAVEHDR)); 

	wResult = waveOutWrite(handle->hWaveOut,lpWaveHdr,sizeof(WAVEHDR)); 
	if (wResult != 0) 
	{ 
		waveOutUnprepareHeader(handle->hWaveOut,lpWaveHdr, 
									  sizeof(WAVEHDR)); 
		printf("Failed to write block to device\n"); 
		return(OM_ERR_TEST_FAILED);
	}

	while((lpWaveHdr->dwFlags & WHDR_DONE) == 0)
	{
			/* !!! Put a timeout here */
	}
	return(OM_ERR_NONE);
}

omfErr_t MyFinishAudio(audioInfoPtr_t handle)
{
	MMRESULT res ;

	res = waveOutClose(handle->hWaveOut);
	if (res != MMSYSERR_NOERROR	)
		printf("error on waveoutclose\n") ;

	return(OM_ERR_NONE);
}
