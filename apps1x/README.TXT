Applications from the apps1x/ directory (1.5 API)
-------------------------------------------------

These are the original 1.5 applications, linked to the 2.0 Toolkit
using the back-compatibility API.

Program		Args	Description
----------------------------------------------------------------------------
DrawExample	infile	(SGI only) Display frames of video from 1.x
			media files on an SGI console, one frame at a
			time.   Note that this application displays 
			OMF files that have RGBA data in them.  (The
			OMF file generated by the MkABGR application 
			in the apps directory will not display properly.)

aifc2omf	infile  (aifc)
		outfile (omf)	
			(UNIX and Macintosh only) aifc2omf converts an
			AIFC (e.g. voice.aifc, voicemono.aifc) sound file 
			into an OMF Interchange file. 

bdump		infile	bdump prints the contents and structure of an
			OMF 1.x file in a readable format.  This app 
			does not work with 2.x files, omfDump and omfInfo
			found in the apps directory are more appropriate.

omfinfo		infile [infile2 ...]
			Prints description of media contained in an
			omf 1.x or 2.x file.
			
playOMFIaifc	infile	(SGI only) Plays aifc format audio from a 1.x
			media file on an SGI through the system
			speaker using the SGI AIFC library.

recordOMFIaifc	outfile	(SGI only) Records aifc format audio to a 1.x
			omf media file from an SGI microphone using
			the SGI AIFC library.

sgi2omf	[options] output_file_name sequence_base_name [start_frame stop_frame]
			(SGI only) Converts a sequence of SGI format
			rgb files to a 1.x omf media file.  Try:

	sgi2omf -avr 26 animation.avr26.omf .../Toolkit/apps1x/animation.%d 1 4
	omfinfo animation.avr26.omf


