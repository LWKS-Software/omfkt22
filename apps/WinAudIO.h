/*
 * Name: WinAudIO.h
 *
 * Function: Windows audio support for play, in order to remove excess
 *			baggage from the examples.
 *
 *			Currently restricted to playing mono.
 */

#include "omPublic.h"

typedef struct winAudioInfo *audioInfoPtr_t;

omfInt32 MyGetInterfaceSampleWidth(void);

omfErr_t MyInitAudio(omfRational_t rate,
		     omfInt32 sampleSize,
		     omfInt32 numCh,
		     audioInfoPtr_t *handle);

omfErr_t  MyRecordData(	omfInt32 		*bytesPerSample,
						char 			*buffer,
						omfUInt32 		buflen,
						omfUInt32 		*numSamples,
						omfRational_t	*audioRate,
						omfInt16 		*sampleSize,
						omfInt16 		*numChannels);

omfErr_t MyPlaySamples(	audioInfoPtr_t	handle,
			char 		*buffer,
			omfUInt32 	numSamples);
						
omfErr_t MyFinishAudio(audioInfoPtr_t	handle);

