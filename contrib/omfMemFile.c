/*---------------------------------------------------------------------------*
 |                                                                           |
 |                         <<<    OMFAnsiC.c     >>>                         |
 |                                                                           |
 |                    OMFI Container Manager Basic Handlers                  |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 12/26/91                                  |
 |                                                                           |
 |                             Roger P. Sacilotto                            |
 |                                 6/29/93                                   |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1991-1992                 |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file may be used to replace the omfansic.c or omfmacio.c handlers.  It operates
 by reading the entire OMF file into memory using ANSI calls, and then uses memcpy
 to handle all future reads and writes.

 This file has been submitted by an OMF developer, and it not part of the standard
 toolkit distribution, but is believed to work.  You may modify this file to create your
 own handler, but please includall previous copyrights in the header.
*/


/*------------------------------------------------------------------------------------*
 | DOS (80x86) USERS -- USE THE "LARGE" MEMORY MODEL WHEN COMPILED FOR 80X86 MACHINES |
 *------------------------------------------------------------------------------------*

 The Container Manager is built with this same option and assumes all handler and
 metahandler interfaces are similarly built and can be accessed with "far" pointers.
*/


#include "masterhd.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include "omCvt.h"

#ifndef __CMTYPES__
#include "CMTypes.h"
#endif
#ifndef __CM_API_TYPES__
#include "CMAPITyp.h"
#endif
#ifndef __CM_API__
#include "CMAPI.h"
#endif
#ifndef __LISTMGR__
#include "ListMgr.h"
#endif
#ifndef __TOCENTRIES__
#include "TOCEnts.h"
#endif
#ifndef __TOCOBJECTS__
#include "TOCObjs.h"
#endif
#ifndef __CONTAINEROPS__
#include "Containr.h"
#endif
#ifndef __HANDLERS__
#include "Handlers.h"
#endif
#ifndef __SESSIONDATA__
#include "Session.h"
#endif
#ifndef __ERRORRPT__
#include "ErrorRpt.h"
#endif
#ifndef __UTILITYROUTINES__
#include "Utility.h"
#endif
#ifndef __BUFFEREDIO__
#include "BufferIO.h"
#endif
/*
 * #include "Handlers.h" #include "CMAPI.h"
 */
#include "XHandlrs.h"
#include "omErr.h"
#include "omTypes.h"
#include "omPvt.h"
#include "omfMemFile.h"

/* Seek method constants */

#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0


static CMSize32   uncachedRead(CMRefCon refCon, CMPtr buffer, CMSize32 elementSize, CMCount32 theCount);

static int  local_ftell(omfMemDescriptorPtr_t f);
static int  local_feof(omfMemDescriptorPtr_t);
static int local_fflush(omfMemDescriptorPtr_t);
static int local_fclose(omfMemDescriptorPtr_t);
static Boolean local_fopen(omfMemDescriptorPtr_t, char *);
static size_t local_fread(void *, size_t, size_t, omfMemDescriptorPtr_t);
static int local_fseek(omfMemDescriptorPtr_t, int , int);
static size_t local_fwrite(const void *, size_t, size_t, omfMemDescriptorPtr_t);


/*
 * Passed to every handler except for "open" is a "reference constant" or
 * just "refCon" for short. This is a general user defined value that allows
 * the user to associate a particular container with its handlers.
 *
 * The "open" is unique in that it returns the refCon to the Container Manager
 * so that it can pass it to all the other handlers. In order for the "open"
 * itself to know what it is to do, the "moral equivalent" of a refCon is
 * passed.  In the API this is referred to as "attributes".  The attributes
 * are passed to CMOpen[New]Container() who in turn pass it through to the
 * "open".  The open then uses the attributes to associate it with a refCon
 * to be used from that point on.
 *
 * The Container Manager only transmits attributes and refCons.  The Container
 * Manager itself does not know or care what they mean.
 *
 * In this example, we define the refCon that is a pointer to a allocated struct
 * with the following definition:
 */

struct MyRefCon
{											/* Our "refCon" layout:					*/
   CMSession          	sessionData;		/*    ptr to current container session	*/
	omfMemDescriptor_t	*f;
   char           		haveSize;			/*    1 ==> have file size				*/
   char              	seekValid;			/*    1 ==> file pointer hasn't moved	*/
   CMSeekMode          	lastSeekMode;		/*    most recent seek mode				*/
   int                	lastSeekOffset;	/*    most recent seek offset			*/
   int               	fileSize;			/*    file size if haveSize == 1		*/
   GetUpdatingTargetType getTargetType;		/*    "get target type" user function	*/
   omfHdl_t				file;
   omfBool				swapMeta;
   CMSize				tocOffset;			/* Position of TOC in the file */
   omfBool				inTOC;				/* Is current seek pos in the TOC */
   char            		pathname[128];		/* start of name of container file		*/
};
typedef struct MyRefCon MyRefCon, *MyRefConPtr;

/* The current session data pointer is saved.  It is needed to allocate and
 * free the refCon and to report errors through the session handlers.
 *
 * The example handlers are for containers that can be
 * manipulated with standard C stream I/O calls.  Therefore we need a stream
 * file variable (f). The path name to the container file itself will be placed
 * in the pathname field.  When the struct is allocated, the size of the
 * pathname will be taken into account.  That's why it is placed at the end
 * of the struct.
 *
 * In this implementation we get the file size by seeking to the end of
 * file and getting the stream pointer position.  This may be inefficient
 * in some systems.  So to address this, the switch haveSize is used.
 * When set, fileSize will be the file size.  It is reset by any write
 * (of course, if we trusted ourselves, we could just maintain the
 * size all the time, incrementing it for writes).
 *
 * Along the same lines, the seekValid, lastSeekMode, and lastSeekOffset
 * fields are used to suppress needless seeks.  These fields are set by
 * the seek handler.  seekValid indicates a seek has been done and
 * lastSeekMode is the seek mode used.  Any change of the stream file
 * pointer resets the seekValid switch.  If the file pointer doesn't
 * change, seekValid is still set.  Further seeks are suppressed when the
 * switch is still set AND the seek mode and value are the same.
 *
 * The traceFile and tracing fields are used for tracing the behavior of
 * these example handlers. They are manipulated by another special routine
 * setHandlersTrace() described later.
 *
 * The getTargetType field is a copy of the function pointer passed as a
 * parameter to the routine that allocates this refCon,
 * createRefConForMyHandlers().  It defines a pointer to a user defined
 * routine to create a type that, when used by CMNewValue() or
 * CMUseValue() will spawn a dynamic value for CMOpenNewContainer() to
 * use when updating another container (i.e., the useFlags were
 * kCMUpdateTarget).  The type must be associated with a dynamic value
 * handler package that will access the target container to be updated.
 * See the returnTargetType_Handler() for further details.  It
 * is the only one that uses this field.
 *
 * In this particular example, we define the attributes and the refCon
 * as one in the same. They need not be however.  The main reason
 * we do this is to allow tracing of the open itself. The
 * refcon/attributes defined here can be passed to a special trace
 * enable/disable routine (setHandlersTrace() described later) before we
 * call CMOpen[New]Container() to allow tracing of it.  If
 * it were not for the tracing, the attributes would probably be just a
 * container file name and the refCon would be set up by the open handler.
 *
 */




/* Now for the interesting stuff...                                                     */


/*--------------------------------------------------------------------------*
 | createRefConForMyHandlers - create a handler "refCon" (and "attributes") |
 *--------------------------------------------------------------------------*

 You call this special routine BEFORE calling CMOpen[New]Container().  The thing returned
 from here is the refCon/attributes that will be passed to the open handler in this file.
 In this example, the attributes and the refCon are the same.  So open will return it to
 the Container Manager.  To the Container Manager, the thing returned from the open is a
 "refCon".  That is then passed to all other handler calls.

 Note, callers outside this file as well as the Container Manager have no knowledge of what
 the refCon means. Nor should they!

 This routine allocates the refCon as a MyRefCon struct using the session memory allocator
 accessed through the sessionData pointer.  Each MyRefCon struct is variable length as a
 function of the passed pathname.  The pathname is the name of the container file to be
 manipulated by this handler.  It is placed in the allocated struct so that when it comes
 time for the Container Manager to open the container, the name will be known.

 The getTargetType defines a pointer to a user defined routine to create a type that, when
 used by CMNewValue() or CMUseValue() will spawn a dynamic value for CMOpenNewContainer()
 to use when updating another container (i.e., the useFlags were kCMUpdateTarget). The type
 must be associated with a dynamic value handler package that will access the target
 container to be updated.

 The prototype for the getTargetType function should be defined as follows:

       CMType CM_FAR getTargetType(CMContainer theUpdatingContainer);

 The container in which the type is to be created is passed in theUpdatingContainer.

 See documentation for returnTargetType_Handler() for further details.

 Since this is an example, any errors result in termination of the program with an error
 message.
*/

CMRefCon CM_FIXEDARGS createRefConForMyHandlers(CMSession sessionData,
			                       const char CM_FAR * pathname,
			                GetUpdatingTargetType getTargetType,
			                omfHdl_t	file)
{
	MyRefConPtr     p = (MyRefConPtr) CMMalloc(NULL, sizeof(MyRefCon), sessionData);
	omfMemDescriptorPtr_t pMem = (omfMemDescriptorPtr_t)pathname;

	if (p == NULL)
	{			/* allocation failed!                   */
		omfCMError(sessionData, "Allocation of \"refCon\" failed for container \"^0\"!", pathname);
		return (NULL);
	}
	strcpy( (char*) p->pathname, "omf Memory File");	/* save pathname for the open handler   */
	p->f = pMem;

	p->haveSize = 0;	/* no file size known yet               */
	p->seekValid = 0;	/* no seek done yet either              */
	p->getTargetType = getTargetType;	/* use only by
						 * returnTargetType_Handler */
	p->sessionData = sessionData;	/* save current session data pointer    */
	p->file = file;
    if(p->file->setrev == kOmfRev1x)				/* Save the revision for writing */
      p->swapMeta = TRUE;
    else
      p->swapMeta = FALSE;

	return ((CMRefCon) p);	/* return refCon as anonymous ptr       */
}


/*--------------------------------------------------------------------*
 | containerMetahandler - metahandler to return the handler addresses |
 *--------------------------------------------------------------------*

 A REQUIRED routine that MUST be called BEFORE calling CMOpen[New]Container() (but after
 CMStartSession()) is CMSetMetaHandler().  You pass to it an address of a metahandler
 procedure to be associated with a particular container type name (string).

 When CMOpen[New]Container() is called, you pass the same type name as one of the
 arguments.  CMOpen[New]Container(), in turn, uses that type name to find the metahandler
 address for the type as defined by the earlier CMSetMetaHandler() call.

 CMOpen[New]Container() now uses the resulting metahandler address and repeatedly calls
 it to get the addresses of all the handler routines it uses.  That is what this routine
 is -- a metahandler that is was set by a CMSetMetaHandler() call.

 For each call a unique operation type string is passed to indicate which routine address
 is to be returned.  The following handlers are needed by the Container Manager.  They
 should have the indicated prototypes:                                                  */

CM_CFUNCTIONS

static CMRefCon open_Handler(CMRefCon attributes, CMOpenMode mode);
static void     close_Handler(CMRefCon refCon);
static CMSize32   flush_Handler(CMRefCon refCon);
static CMSize32   seek_Handler(CMRefCon refCon, CMCount posOff, CMSeekMode mode);
static CMSize   tell_Handler(CMRefCon refCon);
static CMSize32   read_Handler(CMRefCon refCon, CMPtr buffer, CMSize32 elementSize, CMCount32 theCount);
static CMSize32   write_Handler(CMRefCon refCon, CMPtr buffer, CMSize32 elementSize, CMCount32 theCount);
static CMEofStatus eof_Handler(CMRefCon refCon);
static CMBoolean trunc_Handler(CMRefCon refCon, CMSize containerSize);
static CMSize   containerSize_Handler(CMRefCon refCon);
static void     readLabel_Handler(CMRefCon refCon, CMMagicBytes magicByteSequence,
	                      CMContainerFlags * flags, CM_USHORT * blksize,
	                 CM_USHORT * majorVersion, CM_USHORT * minorVersion,
		                      CMSize * tocOffset, CMSize * tocSize);
static void     writeLabel_Handler(CMRefCon refCon, CMMagicBytes magicByteSequence,
		                  CMContainerFlags flags, CM_USHORT blksize,
	                     CM_USHORT majorVersion, CM_USHORT minorVersion,
			                  CMSize tocOffset, CMSize tocSize);
static CMValue  returnParentValue_Handler(CMRefCon refCon);
static CM_UCHAR *returnContainerName_Handler(CMRefCon refCon);
static CMType   returnTargetType_Handler(CMRefCon refCon, CMContainer container);
static void     extractData_Handler(CMRefCon refCon, CMDataBuffer buffer,
			                   CMSize32 size, CMPrivateData data);
static void     formatData_Handler(CMRefCon refCon, CMDataBuffer buffer,
			                   CMSize32 size, CMPrivateData data);

CM_END_CFUNCTIONS
/*
 * See comments in      CMAPIEnv.h      about CM_CFUNCTIONS/CM_END_CFUNCTIONS
 * (used only when compiling under C++).
 *
 * The documentation for these routines is specified below as each routine is
 * defined so we will not repeat it here.  Note, that we actually declare all
 * these routines as "static" since they only need to be visible in this
 * file.  The Container Manager gets at them through the addresses we return
 * from here.
 *
 * NULL may be returned from here.  That indicates to the Container Manager that
 * the routine is not provided.  It is an error to not provide a routine
 * required by the API as a function of whether you are opening the container
 * for reading (i.e., CMOpenContainer()) or writing  (i.e.,
 * CMOpenNewContainer() -- this includes converting).  Here is a chart as to
 * which routines are required for reading, writing, and updating:
 *
 * Mode     Reading  |  Writing  | Updating
 *          ---------+-----------+---------
 * open        X     |     X     |    X
 * close       X     |     X     |    X
 * flush             |           |
 * seek        X     |     X     |    X
 * tell        X     |     X     |    X
 * read        X     |           |    X
 * write             |     X     |    X
 * eof               |           |
 * trunc             |           |
 * size        X     |     X     |    X
 * readLabel   X     |           |    X
 * writeLabel        |     X     |    X
 * parent      1     |     1     |
 * name              |           |
 * targetType        |           |    X
 * extract     X     |           |    X
 * format            |     X     |    X
 *          ---------+-----------+---------
 *
 * Notes: 1. The parent value handler is required ONLY for embedded container
 * handlers.
 *
 * The X's indicate required for the mode.  Blanks mean optional or are not used
 * for the mode.  Note, that updating generally is an or'ing of the reading
 * and writing cases.
 *
 * Note, these (and the special handlers) are the ONLY routines required by the
 * Container Manager itself.  Others may be supplied for special purposes as
 * described in the API documentation.  The additional routines we define in
 * this file, namely createRefConForMyHandlers() and setHandlersTrace()
 * (described later) are supplied just as a mechanism to isolate this example
 * handler and its refCon to this file.  They are not known to the Container
 * Manager itself.
 *
 * So at last we come to the definition of our example metahandler proc. It's a
 * lot shorter that the documentation it took to get to this point!  You will
 * see a list of macro calls for the operationType strings.  These are
 * defined in     CMAPIIDs.h     which is also included by CM_API.h.  Since
 * our example is an illustration for both reading and writing all routines
 * are defined in this file.
 *
 * The API generalizes the use of metahandlers to allow specific metahandlers
 * for specific type objects within a container.  This is the targetType
 * argument.  For simple direct handlers, this is not needed and passed as
 * NULL.  It is ignored here.
 */

CMHandlerAddr CM_FIXEDARGS containerMetahandler(CMType targetType, CMconst_CMGlobalName operationType)
{
	if (strcmp((char *) operationType, (char *) CMOpenOpType) == 0)
		return ((CMHandlerAddr) open_Handler);
	else if (strcmp((char *) operationType, (char *) CMCloseOpType) == 0)
		return ((CMHandlerAddr) close_Handler);
	else if (strcmp((char *) operationType, (char *) CMFlushOpType) == 0)
		return ((CMHandlerAddr) flush_Handler);
	else if (strcmp((char *) operationType, (char *) CMSeekOpType) == 0)
		return ((CMHandlerAddr) seek_Handler);
	else if (strcmp((char *) operationType, (char *) CMTellOpType) == 0)
		return ((CMHandlerAddr) tell_Handler);
	else if (strcmp((char *) operationType, (char *) CMReadOpType) == 0)
		return ((CMHandlerAddr) read_Handler);
	else if (strcmp((char *) operationType, (char *) CMWriteOpType) == 0)
		return ((CMHandlerAddr) write_Handler);
	else if (strcmp((char *) operationType, (char *) CMEofOpType) == 0)
		return ((CMHandlerAddr) eof_Handler);
	else if (strcmp((char *) operationType, (char *) CMTruncOpType) == 0)
		return ((CMHandlerAddr) NULL);
	else if (strcmp((char *) operationType, (char *) CMSizeOpType) == 0)
		return ((CMHandlerAddr) containerSize_Handler);
	else if (strcmp((char *) operationType, (char *) CMReadLblOpType) == 0)
		return ((CMHandlerAddr) readLabel_Handler);
	else if (strcmp((char *) operationType, (char *) CMWriteLblOpType) == 0)
		return ((CMHandlerAddr) writeLabel_Handler);
	else if (strcmp((char *) operationType, (char *) CMParentOpType) == 0)
		return ((CMHandlerAddr) NULL);
	else if (strcmp((char *) operationType, (char *) CMContainerOpName) == 0)
		return ((CMHandlerAddr) returnContainerName_Handler);
	else if (strcmp((char *) operationType, (char *) CMTargetTypeOpType) == 0)
		return ((CMHandlerAddr) returnTargetType_Handler);
	else if (strcmp((char *) operationType, (char *) CMExtractDataOpType) == 0)
		return ((CMHandlerAddr) extractData_Handler);
	else if (strcmp((char *) operationType, (char *) CMFormatDataOpType) == 0)
		return ((CMHandlerAddr) formatData_Handler);
	else
		return (NULL);

  /* Note that NULL is being returned for CMTruncOpType. We do however define a dummy   */
  /* handler for this to explain its purpose.  See the documentation for that routine   */
  /* for details.                                                                       */

  /* Note also that NULL is also being returned for CMParentOpType since the handlers   */
  /* in this file are for stream container files and not embedded containers.  For      */
  /* details on embedded container handlers see ExampleEmbeddedHandlers.c.              */
}




/*--------------------------------------------------------------------------------------*
  From this point one we have the handler routines themselves.  Order doesn't matter from
  here on.  Read them at your leisure (yea, right!)

  Note: To keep the example simple, there is little or no error checking.  More error
        checking should be provided where applicable.
 *--------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------*
 | open_Handler - open a container for input, output, or updating |
 *----------------------------------------------------------------*

 This is called only by CMOpen[New]Container() to open the container associated with the
 specified attributes.  The open modes passed are "wb+" (truncate and updating) for
 creating a new container, "rb" for reading an existing container, and "rb+" (updating)
 for converting a "bunch" of data in a file to container format.

 The open routine is unique among the handlers in that the potential interpretion of the
 first argument is different.  The attributes are passed to CMOpen[New]Container() and
 them passed to here unaltered and not looked at by the Container Manager.  It is intended
 to tie the open handler to a specific container.  Thus the attributes serves as a
 communication channel strictly to the open.  In its simplest form for a container file it
 probably would be a file pathname.

 What the Container Manager expects as a return value is a "refCon" to be passed unaltered
 to all of the other handler routines.  This too is a communication channel that ties the
 handlers to their open.  Again, in its simplest form the pathname attributes would
 probably be converted to a FILE* variable for stream operations.  It would be returned to
 the Container Manager which would pass it to all following handler calls.

 In our example, we defined the attributes and the refCon to be the same thing.  That is a
 pointer to a struct that contains the pathname, the FILE* variable, and some trace info.
 This is allocated by a previous call to createRefConForMyHandlers() which set the
 pathname for use here to set the FILE* variable.  Thus the input attributes is simply
 returned to the Container Manager. Thus all the other handlers will know that the refCon
 is a pointer to our struct.

 The modes of container (file) opening passed here from the Container Manager need some
 further discussion.  There are three open modes to go with the three open cases supported
 by the Container Manager:

 converting   For this mode the open mode is "rb+".  The intent is to open a container for
              updating, i.e., reading and writing, but preserve the current contents of the
              file.

 writing      The mode here is "wb+".  The intent is to create the container if it doesn't
              already exist, set its file size to 0 (truncate it), and to allow BOTH
              reading and writing (update).  The Container Manager allows reading of stuff
              previously written.

 reading      The read open mode is "rb" (no plus). An existing container is to be open for
              input (reading) only.  It cannot be written to for updating.

 updating     The read open mode is "rb+".  An existing container is to be opened for
              updating.  This is also used for reusing free space.

 Note, it is required in this sample handler that files be opened as binary, rather than
 text files, hence the "b" suffix on the modes.  Considering what we put in a container
 this makes sence at first glance. But there is an additional reason.  See the note in
 tell_Handler() for further details on this.

 This is a REQUIRED handler routine.
*/

static CMRefCon open_Handler(CMRefCon attributes, CMOpenMode mode)
{
	MyRefConPtr     p = (MyRefConPtr) attributes;
	char			*ansiMode = (char *)mode;
	Boolean         result;

	result = local_fopen(p->f, (char *) mode);	/* set file variable in
							 * refCon  */
	if (! result)
	  {			/* oops, open didn't work!      */
		omfCMError(p->sessionData, "Cannot open \"^0\"!\n    ^1",
				   p->pathname, strerror(errno));
		return (NULL);
	  }

	p->haveSize = 0;	/* size is not known            */
	p->seekValid = 0;	/* no seek done yet either      */

	return (attributes);
}


/*------------------------------------------*
 | close_Handler - close the container file |
 *------------------------------------------*

 This is called when CMCloseContainer() is called.  The container file is closed and the
 space allocated by createRefConForMyHandlers() for our refCon is freed. Once the Container
 Manager calls this routine, it will, of course, make no further calls the the handler.

 This is a REQUIRED handler routine.
*/

static void     close_Handler(CMRefCon refCon)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;

	local_fclose(p->f);		/* close it...                  */

	CMFree(NULL, p, p->sessionData);	/* bye, bye, refcon...          */
}


/*---------------------------------------*
 | flush_Handler - flush the I/O buffers |
 *---------------------------------------*

 This routine is called by the Container Manager when it wants to make sure the I/O
 (actually just output) buffer(s) are written to the container.  In most systems it is
 probably unnecessary.

 0 is returned for success and non-zero for failure.

 This is an OPTIONAL routine which will be called by the Container Manager when it thinks
 it needs it.  If it is not defined, it is ignored on the assumtion the handler writer
 knows it is unnecessary.
 */

static CMSize32   flush_Handler(CMRefCon refCon)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;
	int             result;

	result = local_fflush(p->f);	/* yup, you guessed it!         */
	p->haveSize = 0;	/* size is not known            */
	p->seekValid = 0;	/* assume stream pointer changed */

	return ((CMSize32) result);
}


/*-------------------------------------------------------------------*
 | seek_Handler - position the I/O to the specified container offset |
 *-------------------------------------------------------------------*

 This allows the Container Manager random access to the container. The stream is positioned
 according to the mode and posOff.  0 is returned for success and non-zero for failure.

 The mode determines how to use posOff to position the stream.  It is defined as one of
 the following seek modes (defined in   CMAPITyp.h   which is included by CMAPI.h ):

  kCMSeekSet          Set the stream to the container offset specified by posOff.  The
                      posOff is the character offset from the beginning of the container.

  kCMSeekCurrent      Set the stream posOff characters from the current stream position.
                      posOff can be postive or negative here.

  kCMSeekEnd          Set the stream posOff characters from the current end of the
                      container.  posOff here is only zero or negative. We never assume or
                      allow undefined contents to be generated in a container.  Indeed,
                      some file systems may not even allw this!

 As implemented here, we have "optimized" the seek operation to NOT actually do a seek if
 it is not necessary.  Generally the Container Manager always seeks to set the proper
 position.  Thus, there are may occasions where a physical seek may be unnecessary

 Three fields in the refCon are used to support this; seekValid and lastSeekMode.  These
 fields are set here.  seekValid indicates the seek has been done, lastSeekMode is the
 seek mode used, and lastSeekOffset is the seek value.  Any change of the stream file
 pointer resets the seekValid switch.  If the file pointer doesn't change, seekValid is
 still set if we come through here again.  Further seeks are suppressed when the switch is
 still set AND the seek mode and value are the same.

 This is a REQUIRED handler routine.
*/

static CMSize32   seek_Handler(CMRefCon refCon, CMCount posOff, CMSeekMode mode)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;
	int             seekMode, result;
	omfInt32		pos32, upper;

	omfsDecomposeInt64(posOff, &upper, &pos32);

	if (mode == kCMSeekSet)	/* map out seek mode into standard ANSI */
		seekMode = SEEK_SET;
	else if (mode == kCMSeekEnd)
		seekMode = SEEK_END;
	else
		seekMode = SEEK_CUR;

	result = local_fseek(p->f, (int ) pos32, seekMode);	/* seek...                              */

	p->seekValid = 1;	/* indicate seek has been done          */
	p->lastSeekMode = mode;	/* remember the mode that we just used  */
	p->lastSeekOffset = (int ) pos32;	/* and also the specified
						 * offset        */

	return (result);
}


/*-----------------------------------------------*
 | tell_Handler - return current stream position |
 *-----------------------------------------------*

 This routine returns the current stream position as characters from the start of the
 container.

                        --->  N O T E  and  C A U T I O N!  <---

 We uses a standard local_ftell() to implement this routine in this example.  The ANSI standard
 says the local_ftell() ONLY returns a character position for binary files.  There is no
 guarantee what the form of an local_ftell()  is for text files.  Thus it is required that if
 this form of implementation is used, that the file must be opened as a binary and not a
 text file.

 This is a REQUIRED handler routine.
*/

static CMSize   tell_Handler(CMRefCon refCon)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;
	int             posOff = local_ftell(p->f);	/* get position                 */
	CMCount largeOffset = *((CMCount *)&posOff);


	return (largeOffset);
}



/*------------------------------------------------------------------*
 | read_Handler - read information from the container into a buffer |
 *------------------------------------------------------------------*

 This handler reads bytes from the current stream position into the specified buffer.  Up
 to theCount elements of the specified size are read.  The actual number of items read,
 which may be less than theCount (e.g., if the end-of-file is read) or even 0 is returned
 as the function result.

 This is a REQUIRED ROUTINE FOR READING only (CMOpenContainer()).
*/

static CMSize32   read_Handler(CMRefCon refCon, CMPtr buffer, CMSize32 elementSize, CMCount32 theCount)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;
	CMSize32          amountRead, bytesToRead;


	bytesToRead = elementSize * theCount;
		amountRead = uncachedRead(refCon, buffer, 1, bytesToRead);

	p->seekValid = 0;	/* stream pointer has now changed */

	return (amountRead);
}

static CMSize32   uncachedRead(CMRefCon refCon, CMPtr buffer, CMSize32 elementSize, CMCount32 theCount)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;
	CMSize32          amountRead;

	amountRead = local_fread((char *) buffer, (size_t) elementSize, (size_t) theCount, p->f);

	return (amountRead);
}



/*------------------------------------------------------------------*
 | write_Handler - write information from a buffer to the container |
 *------------------------------------------------------------------*

 This handler writes bytes from the specified buffer to the container at the current stream
 position.  Up to theCount elements of the specified size are written.  The actual number
 of items written is returned.  It should be the same as theCount unless errors occur.  The
 Container Manager checks this so it need not be checked here.

 This is a REQUIRED ROUTINE FOR WRITING only (CMOpenNewContainer()).
*/

static CMSize32   write_Handler(CMRefCon refCon, CMPtr buffer, CMSize32 elementSize,
			                      CMCount32 theCount)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;
	CMSize32          amountWritten;

	if (theCount > 0)
	{			/* do write only if there's data  */
		amountWritten = local_fwrite((char *) buffer, (size_t) elementSize, (size_t) theCount, p->f);

		p->haveSize = 0;/* size is not known now          */
		p->seekValid = 0;	/* stream pointer now changed     */
	} else
		amountWritten = 0;

	return (amountWritten);
}


/*------------------------------------------*
 | eof_Handler - determine eof input status |
 *------------------------------------------*

 This handler checks to see if an end-of-file was detected while reading the container (by
 read_Handler()).  If it has a non-zero value is returned.  Otherwise, 0 is returned.

 This is an OPTIONAL routine.  It is not used by the Container Manager itself, but might be
 used by handlers for special types.
*/

static CMEofStatus eof_Handler(CMRefCon refCon)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;
	int             status = (int ) local_feof(p->f);

	return ((CMEofStatus) (status != 0));
}


/*---------------------------------------------------------*
 | trunc_Handler - truncate container to the specifed size |
 *---------------------------------------------------------*

 This handler is expected to truncate the container to the specified size.  1 is returned
 if it is truncated and 0 otherwise.

 This is an OPTIONAL routine used only for reusing free space where it is possible for a
 new updated TOC is smaller than the original.  The excess space must be removed or
 declared as free space.  This routine, if it is provided, should do the removal.  If not
 supplied, or if it returns false, then potentially it will take TWO writes of the TOC to
 move the excess space on to the container's free list and to its proper place before the
 TOC.

 In this example handler set, we returned NULL in the metahandler to indicate that this
 routine does not exist.  The reason is that we are using standard ANSI stream I/O.  It
 provides NO way to truncate a stream!

 If your system provides a way for do the truncation, you should do it.  It removes the
 possible two-TOC-write suitation just described.

 There is, however, one thing to keep in mind before you lose any sleep over this (yea,
 right).  Even if the trunc handler is not supplied, the probability of the two TOC writes
 is fairly small!  The reason comes from the fact of noting how you can get a smaller TOC
 in the first place.  The only way it can happen is to delete values.  That reduces the
 number of TOC entries.  But they are replaced by free list entries which is the way the
 Container Manager keeps track of the deleted value data.  So, unless free space coalesces
 (which it could do), there will be no net loss of TOC entries and hence the TOC will not
 get smaller.  So what's the probability of separate values data space coalescing?  That
 is basically the probability of the TOC growing smaller.
*/

static CMBoolean trunc_Handler(CMRefCon refCon, CMSize containerSize)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;

	p->seekValid = 0;	/* assume stream pointer changed */

	return (0);
}


/*------------------------------------------------------------------*
 | containerSize_Handler - return the current size of the container |
 *------------------------------------------------------------------*

 This handler is used to get the size of the container.  It returns the 1-relative size of
 the container (file).

 For this stream container example, getting the size is implemented as a seek to the end
 of file and then local_ftell the position.  This requires that the stream be a C as binary
 file.  Only in those kind of files are file offset positions in units of bytes starting at
 0.  The ANSI standard says that for text files the thing returned from an local_ftell is
 implementation defined and is only to be used by an local_fseek.

 Note, the reason that getting the size is a handler is that different I/O systems may have
 more efficient ways of getting the container size.  Doing local_fseeks and local_ftells is probably
 not the best implementation of doing this!
*/

static CMSize   containerSize_Handler(CMRefCon refCon)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;
	omfInt64		largeSize;


	/*	if (p->haveSize == 0) !!! */
	{			/* if size is not known...      */
		local_fseek(p->f, 0L, SEEK_END);	/* ...seek to the end of file   */
		p->fileSize = local_ftell(p->f);	/* ...size is where we are      */
		p->haveSize = 1;/* ...announce we know it       */
		p->seekValid = 1;	/* ...indicate seek was done    */
		p->lastSeekMode = kCMSeekEnd;	/* ...remember the seek mode    */
		p->lastSeekOffset = 0L;	/* ...also the seek value       */
	}
	omfsCvtInt32toInt64(p->fileSize, &largeSize);
	return (largeSize);
}


/*---------------------------------------------------------------*
 | readLabel_Handler - read in and extract the container's label |
 *---------------------------------------------------------------*

 This handler finds the label in the container and returns all the information it contains.
 In this example, the container is a stream file and we expect the label to be at the end
 of the container (see also writeLabel_Handler()).

 As defined by the API documentation, a container label has the following layout:

      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
      |      Magic Bytes      |flags| enc |major|minor|TOC offset |  TOC size |
      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                  8              2     2     2     2        4           4

*/
#define CM_LABEL_SIZE 24
/*
 * For C compilers that generate code for machines where char is 1 byte, short 2, and int 
 * 4, we may use the following struct to define the label format:
 */

 struct ContainerLabelFmt {               /* Layout of a container label:               */
	unsigned char  magicBytes[8];          /* 8 bytes: the magic byte identifier         */
	unsigned short flags;                  /* 2        the label flags                   */
	unsigned short encoding;               /* 2        data encoding flags               */
	unsigned short majorVersion;           /* 2        major format version number       */
	unsigned short minorVersion;           /* 2        minor format version number       */
   unsigned int   tocOffset;              /* 4        offset to start of TOC            */
   unsigned int   tocSize;                /* 4        total byte size of the TOC        */
 };
 typedef struct ContainerLabelFmt ContainerLabelFmt;


 /*

                                    W A R N I N G !!

 If your C compiler and/or hardware cannot use the above struct, then this handler will
 have to be recoded accordingly.  Indeed, it is for this very reason this IS a handler. It
 shifts the portability problem outside the Container Manager.

 This handler looks for the label and decodes it according to the above struct.  So this
 is only for implementations with the above C size definitions.  Our own handler routines
 defined for containers are used to do the positioning and read.

 No editoralizing is done on the extracted information.  It is simply passed back to the
 Container Manager where the validation is done.

 This is a REQUIRED ROUTINE FOR READING only (CMOpenContainer()).
*/

static void readLabel_Handler(CMRefCon refCon, CMMagicBytes magicByteSequence,
                              CMContainerFlags *flags, CM_USHORT *blksize,
                              CM_USHORT *majorVersion, CM_USHORT *minorVersion,
                              CMSize *tocOffset, CMSize *tocSize)
{
	MyRefConPtr     p = (MyRefConPtr)refCon;
   unsigned int    labelSize, tocSize32, tocOffset32;
	CM_UCHAR        labelBuffer[CM_LABEL_SIZE];
	CMDataBuffer    data;
	short			versionSwp, flagsSwp;
	CMSize			labelOffset;

	/* Seek to the end of the label at the end of the container and read it...            */

	omfsCvtInt32toInt64(-(int )CM_LABEL_SIZE, &labelOffset);
	data = labelBuffer;
	seek_Handler(refCon, labelOffset, kCMSeekEnd);
	labelSize = (unsigned int )read_Handler(refCon, (CMPtr)data,
                                          (CMSize32)sizeof(unsigned char),
                                          (CMCount32)CM_LABEL_SIZE);

	p->seekValid = 0;                               /* stream pointer has now changed     */
	if (labelSize != CM_LABEL_SIZE)
	{   /* must have read it all!             */
		omfCMError(p->sessionData, "Incorrect byte length read while reading label from container \"^0\"!", p->pathname);
		return;
	}

	/* Return all the label info...                                                       */
	/*  Also, don't read in a struct, because padding errors between compilers may
		cause data errors.  Instead, extract the information out of a buffer.            */

/* (recap from above...)
 *
 * As defined by the API documentation, a container label has the following layout:
 *
 *      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *      |      Magic Bytes      |flags| bks |major|minor|TOC offset |  TOC size |
 *      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *                  8              2     2     2     2        4           4
#ifdef PORT_FILESYS_40BIT
 *
 *      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *      |    Ext  Magic Bytes   |       TOC offset      |       TOC size        |
 *      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *                  8                       8                       8
#endif
 */

	versionSwp = (labelBuffer[12] << 8) + labelBuffer[13];	/* May be byte-swapped */
	flagsSwp = (labelBuffer[8] << 8) + labelBuffer[9];	/* May be byte-swapped */

	if(versionSwp == 0x0001 || versionSwp == 0x0100)
		p->swapMeta = (PORT_BYTESEX_BIG_ENDIAN ? TRUE : FALSE);
	else
	{
		if(flagsSwp & 0x0101)
			p->swapMeta = (PORT_BYTESEX_LITTLE_ENDIAN ? FALSE : TRUE);
		else
			p->swapMeta = (PORT_BYTESEX_BIG_ENDIAN ? FALSE : TRUE);
	}

	memcpy(magicByteSequence, data, 8); data+= 8;
	extractData_Handler(refCon, data, 2, (CMPrivateData)flags); data +=2;
	extractData_Handler(refCon, data, 2, (CMPrivateData)blksize); data +=2;
	extractData_Handler(refCon, data, 2, (CMPrivateData)majorVersion); data +=2;
	extractData_Handler(refCon, data, 2, (CMPrivateData)minorVersion); data +=2;
	{
		extractData_Handler(refCon, data, 4, (CMPrivateData)&tocOffset32); data +=4;
  		extractData_Handler(refCon, data, 4, (CMPrivateData)&tocSize32); data +=4;
  		omfsCvtUInt32toInt64(tocSize32, tocSize);
 		omfsCvtUInt32toInt64(tocOffset32, tocOffset);
	  }
	if((*blksize == 0) || (omfUInt32)(*blksize*1024L) > tocSize32)
  		*blksize = (CM_USHORT)((tocSize32 / 1024L)+1L);  /* limit size */

	p->tocOffset = *tocOffset;
}


/*-----------------------------------------------------------*
 | writeLabel_Handler - format and write a container's label |
 *-----------------------------------------------------------*

 This handler is called to write a label to the container.  It is called by the Container
 Manager as the LAST thing prior to closing the container which was opened for writing (or
 converting).

 In this example, the container is a stream file.  We want to always put the label at the
 end of the container.  Since everything  else has already been written, there isn't
 really any other place we could put it!

 The label format is defined in readLabel_Handler() above. Here we take all the parameters,
 format them according to the label layout, and write the data to the end of the container.
 Our own handler routines defined for embedded containers are used to do the positioning
 and write.

 Note, see discussion in readLabel_Handler() for portability issues. The problems described
 there equally apply here.

 In this example implementation (with 2-byte words and 4-byte longs), the ContainerLabelFmt
 exactly maps over the desired label format.  Hence no special work needs to be done here.

 This is a REQUIRED ROUTINE FOR WRITING only (CMOpenNewContainer()).
*/

static void writeLabel_Handler(CMRefCon refCon, CMMagicBytes magicByteSequence,
                               CMContainerFlags flags, CM_USHORT blksize,
                               CM_USHORT majorVersion, CM_USHORT minorVersion,
			                   CMSize tocOffset, CMSize tocSize)
{
	MyRefConPtr       p = (MyRefConPtr)refCon;
   unsigned int      labelSize, tocOffset32, tocSize32;
	CM_UCHAR          labelBuffer[CM_LABEL_SIZE]; /* add some extra for safety */
	CMDataBuffer      data;
	CMSize			zero;

	omfsCvtInt32toInt64(0, &zero);
	if(p->file->setrev == kOmfRev1x)		/* For 1.x compatability, write little-endian meta-data */
		p->swapMeta = (PORT_BYTESEX_LITTLE_ENDIAN ? FALSE : TRUE);
	else if (PORT_BYTESEX_LITTLE_ENDIAN)
		flags |= kCMLittleEndian;


  /* Fill in the label buffer with the info...                                          */
  /*   Also, don't write out a struct, because padding errors between compilers may
       cause data errors.  Instead, format the information into a buffer.               */

/* (recap from above...)
 *
 * As defined by the API documentation, a container label has the following layout:
 *
 *      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *      |      Magic Bytes      |flags| bks |major|minor|TOC offset |  TOC size |
 *      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *                  8              2     2     2     2        4           4
 */
	omfsTruncInt64toUInt32(tocOffset, &tocOffset32);
	omfsTruncInt64toUInt32(tocSize, &tocSize32);
	data = labelBuffer;
	memcpy(data, magicByteSequence, 8); data += 8;
	formatData_Handler(refCon, data, 2, (CMPrivateData)&flags); data +=2;
	formatData_Handler(refCon, data, 2, (CMPrivateData)&blksize); data +=2;
	formatData_Handler(refCon, data, 2, (CMPrivateData)&majorVersion); data +=2;
	formatData_Handler(refCon, data, 2, (CMPrivateData)&minorVersion); data +=2;
	{
		formatData_Handler(refCon, data, 4, (CMPrivateData)&tocOffset32); data +=4;
		formatData_Handler(refCon, data, 4, (CMPrivateData)&tocSize32); data +=4;
	}

  /* Write the label to the end of the container value...                               */

	seek_Handler(refCon, zero, kCMSeekEnd);
	labelSize = (unsigned int )write_Handler(refCon, (CMPtr)labelBuffer,
                                           (CMSize32)sizeof(unsigned char),
                                           (CMCount32)CM_LABEL_SIZE);

	p->haveSize  = 0;                             /* size is not known now                */
	p->seekValid = 0;                             /* stream pointer has now changed       */

	if (labelSize != sizeof(ContainerLabelFmt))   /* must have written it all!            */
		omfCMError(p->sessionData, "Incorrect byte length written while writing label to container \"^0\"!", p->pathname);
}


/*----------------------------------------------------------------------------------*
 | returnParentValue_Handler - return value refNum for parent of embedded container |
 *----------------------------------------------------------------------------------*

 This handler routine is used ONLY for embedded containers.  It is called at open time by
 CMOpen[New]Container() so that the Container Manager may determine for itself that it is
 opening an embedded container for a value and what that value is. It is the parent CMValue
 for this handler that is retured by this function.

 This is a REQUIRED handler routine for embedded containers only.  Therefore,
 containerMetahandler() returned NULL for this handler (alternatively, NULL can be returned
 from here to indicate the same thing).  We show it only as a stub (and to resolve the
 forward reference we had to it -- some compilers complain about not having it!).
*/

static CMValue  returnParentValue_Handler(CMRefCon refCon)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;

	return (NULL);
}


/*---------------------------------------------------------------------------*
 | returnContainerName_Handler - return string that identifies the container |
 *---------------------------------------------------------------------------*

 When the Container Manager reports errors (through the error handler -- see
 ExampleSessionHandlers.[ch]) it passes appropriate string inserts that may be formatted
 into the error message.  One of those inserts is usually an string that identifies for
 which container the error applies.  The handler defined here is used by the Container
 Manager to get that identification.

 For this stream container example, the most appropriate identification we can use is the
 pathname for the container file.  No tracing is provided in this handler.  If it's called
 you will see its results in the error inserts (or message).  Besides, you're dead meat
 anyway!

 This is a OPTIONAL routine for reading and writing.  If not provided, the type name passed
 to CMOpen[New]Container(), i.e., the metahandler type, is used for the identification.
*/

static CM_UCHAR *returnContainerName_Handler(CMRefCon refCon)
{
	return ((CM_UCHAR *) ((MyRefConPtr) refCon)->pathname);
}


/*------------------------------------------------------------------------------*
 | returnTargetType_Handler - return target type for updating another container |
 *------------------------------------------------------------------------------*

 If CMOpenNewContainer() is called with useFlags set to kCMUpdateTarget, then the intent is
 to open a new container which will record updating operations of updates applied to
 ANOTHER distinct (target) container.  The target container is accessed indirectly through
 a dynamic value whose type is gotten from a this handler.  This handler must have be
 supplied and it must return a type which will spawn a dynamic value when kCMUpdateTarget
 is passed to CMOpenNewContainer().

 The process of creating a dynamic value (by the Container Manager using the returned type)
 will generate a "real" value in the parent container (the new container to record the
 updates).  That value can be used by future CMOpenContainer()'s to "get at" the target
 again.  To be able to find it, the created value becomes a special property of TOC #1.
 CMOpenContainer() will look for that property.

 This handler is running in the context of the parent container, which is passed as a
 parameter along with the usual refCon.  The handler registers the type in this container.

 See the description of dynamic values and their base types for further details on how
 these types spawn dynamic values.  There are also example handler packages illustrating
 end explaining dynamic values for writing indirectly to a file specified by a pathname
 (ExampleIndFileHandlers.[hc]), and subvalues (ExampleSubValueHandlers.[hc]).
 There is also a more complex example of layered dynamic values for encrypted file I/O
 (       XCrypt.c       ).  See those files for more details on dynamic values.

 To add some generality to this particular example of this handler, we allow the caller
 of CMOpenNewContainer(), who specified that this handler package be used in the first
 place, to actually define the type.  That way we have no particular dependence on a
 specific dynamic handler package here.  We could, for example, only restrict updating
 of other containers which are accessed via a pathname.  We could then define the refCon
 SETUP (the private createRefConForMyHandlers() we defined earlier) to take that pathname.
 Then we could #include        XIndFile.h        and use that example for registering a
 file indirect dynamic value type.

 To generalize this handler, we define its type creation in terms of a user function
 which we call from here.  We simply pass along the container for the user to register the
 type.  The function is pointed to by a function address the user passed to our
 createRefConForMyHandlers() and we saved in our refCon.  If the user passed NULL, we
 return NULL.  That will cause the Container Manager to report an appropriate error.

 This is a OPTIONAL routine for reading and writing.  It is REQUIRED for updating when
 CMOpenNewContainer() is called with useFlags set to kCMUpdateTarget.
*/

static CMType   returnTargetType_Handler(CMRefCon refCon, CMContainer container)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;

	if (p->getTargetType == NULL)
		return (NULL);	/* return NULL if no user proc  */

	return ((*p->getTargetType) (container));	/* return whatever user
							 * created */
}


/*----------------------------------------------------------------------------*
 | extractData_Handler - extract private Container Manager data from a buffer |
 *----------------------------------------------------------------------------*

 This handler is used to extract "internal" Container Manager data previously written to
 the container (e.g., updating data).  1, 2, or 4 bytes (size 8-bit byte) "chunks" of data
 are expected to be copied from a buffer to the data.  Pointers to the data and the buffer
 are passed in.  The buffer can always be assumed large enough to supply all the requested
 data.  The pointer to the data can be assumed to point to a CM_UCHAR if size is 1,
 CM_USHORT if size is 2, and CM_ULONG is size if 4.

 The 1, 2, or 4 bytes are, of course, formatted within the CM_UCHAR, CM_USHORT, or CM_ULONG
 as a function of the architecture.  These may be a different size than what is expected
 to be written to the container.  Indeed, it is the potential difference between the
 architecture from the data layout in the  container that this handler must be provided.

 The information is stored in the container in a layout PRIVATE to the Container Manager.
 For example, it is used to extract the fields of the TOC.  It does repeated calls to this
 handler to extract the information it needs from a buffer that it loads using the
 read_Handler.

 In this example CM_UCHAR, CM_USHORT and CM_ULONG map directly into the container format
 1, 2, and 4 byte entities.  Hence extracting the data is straight-forward.

 This is a OPTIONAL routine for writing.  It is REQUIRED for reading, or if an updating
 container is opened (i.e., a container used previously for updating).  It is REQUIRED for
 updating when CMOpenNewContainer() is called with useFlags set to kCMUpdateTarget or
 kCMUpdateByAppend.
*/

static void     extractData_Handler(CMRefCon refCon, CMDataBuffer buffer,
			                    CMSize32 size, CMPrivateData data)
{
	MyRefConPtr p = (MyRefConPtr)refCon;
	CM_USHORT   tshort;
	CM_ULONG    tlong;
	CM_UCHAR *dest;

	/* Jeff: No longer use INTEL order for private data in IMA & 2.x */
	if(p->swapMeta)
	{
		switch (size)
		{
		case 1:
			*data = *buffer;/* copy the data top the buffer   */
			break;
		case 2:
			dest = (CM_UCHAR *)&tshort;
			dest[0] = buffer[0];
			dest[1] = buffer[1];
		/*	tshort = tshort = *((unsigned short *)buffer); */
			*(unsigned short *) data = (tshort >> 8) |
									   ((tshort & 0xff) << 8);
			break;
		case 4:
			dest = (CM_UCHAR *)&tlong;
			dest[0] = buffer[0];
			dest[1] = buffer[1];
			dest[2] = buffer[2];
			dest[3] = buffer[3];
         /* tlong = *((unsigned int  *)buffer); */
         *(unsigned int  *) data = ((tlong & 0xff000000) >> 24) |
									  ((tlong & 0x00ff0000) >>  8) |
									  ((tlong & 0x0000ff00) <<  8) |
									  ((tlong & 0x000000ff) << 24);
			break;
		case 8:
			dest = (CM_UCHAR *)data;
			dest[0] = buffer[7];
			dest[1] = buffer[6];
			dest[2] = buffer[5];
			dest[3] = buffer[4];
			dest[4] = buffer[3];
			dest[5] = buffer[2];
			dest[6] = buffer[1];
			dest[7] = buffer[0];
			break;
		}
	}
	else
	{
		memcpy(data, buffer, size);
	}


}


/*--------------------------------------------------------------------------*
 | formatData_Handler - format private Container Manager data into a buffer |
 *--------------------------------------------------------------------------*

 This handler is used to format "internal" Container Manager data to be written to
 the container (e.g., updating data).  1, 2, or 4 bytes (size 8-bit byte) "chunks" of data
 are expected to be copied from the data to a buffer.  Pointers to the data and the buffer
 are passed in.  The buffer can always be assumed large enough to hold the data.  The
 pointer to the data can be assumed to point to a CM_UCHAR if size is 1, CM_USHORT if size
 is 2, and CM_ULONG is size if 4.

 The 1, 2, or 4 bytes are, of course, stored in the CM_UCHAR, CM_USHORT, or CM_ULONG as a
 function of the architecture.  These may be a different size than what is expected to be
 written to the container.  Indeed, it is the potential difference between the architecture
 from the data layout in the container that this handler must be provided.

 The information is stored in the container in a layout PRIVATE to the Container Manager.
 For example, it is used to format the fields of the TOC.  It does repeated calls to this
 handler to format the information it needs into a buffer that is eventually written using
 the write_Handler.

 In this example CM_UCHAR, CM_USHORT and CM_ULONG directly map into the container format
 1, 2, and 4 byte entities.  Hence the formatting is straight-forward.

 This is a OPTIONAL routine for reading and writing.  It is REQUIRED for writing, or
 updating when CMOpenNewContainer() is called with useFlags set to kCMUpdateTarget or
 kCMUpdateByAppend.
*/

static void     formatData_Handler(CMRefCon refCon, CMDataBuffer buffer,
			                    CMSize32 size, CMPrivateData data)
{
	MyRefConPtr     p = (MyRefConPtr) refCon;
	CM_UCHAR        *src;

	/* Jeff: No longer use INTEL order for private data in IMA & 2.x */
	if(p->swapMeta)
	{
		switch (size)
		{
		case 1:
			*buffer = *data;                 /* copy the data top the buffer   */
			break;
		case 2:
			*buffer++ = (char)(*(unsigned short *)data & 0xff);
			*buffer++ = (char)((*(unsigned short *)data >> 8) & 0xff);
			break;
		case 4:
         *buffer++ = (char)(*(unsigned int  *)data & 0xff);
         *buffer++ = (char)((*(unsigned int  *)data >> 8) & 0xff);
         *buffer++ = (char)((*(unsigned int  *)data >> 16) & 0xff);
         *buffer++ = (char)((*(unsigned int  *)data >> 24) & 0xff);
			break;
		  case 8:
			src = (CM_UCHAR *)data;
			buffer[0] = src[7];
			buffer[1] = src[6];
			buffer[2] = src[5];
			buffer[3] = src[4];
			buffer[4] = src[3];
			buffer[5] = src[2];
			buffer[6] = src[1];
			buffer[7] = src[0];
			break;
		}
	}
	else
	{
		memcpy(buffer, data, size);
	}
}

static int  local_ftell(omfMemDescriptorPtr_t f)
{
	assert(f != NULL);
	assert(f->magic == OMFMEM_GOOD_MAGIC);
	return(f->currentPos);

}
static int  local_feof(omfMemDescriptorPtr_t f)
{
	assert(f != NULL);
	assert(f->magic == OMFMEM_GOOD_MAGIC);
	if (f->currentPos < f->endOfData)
		return(0);
	else
		return(16);	/* stdio.h copied? */
}

static int local_fflush(omfMemDescriptorPtr_t f)
{
	return(0); /* no flush needed for in-memory file */

}


static Boolean local_fopen(omfMemDescriptorPtr_t f, char *mode)
{
	omfMemStatus_t openStat;

	assert(f != NULL);

	if (strcmp(mode, "rb") == 0)
		openStat = kOmfMemReadOnly;
	else if (strcmp(mode, "rb+") == 0)
		openStat = kOmfMemAppend;
	else if (strcmp(mode, "wb+") == 0)
		openStat = kOmfMemCreate;
	else
		return(FALSE); /* bad mode */

	f->openStatus = openStat;
	f->magic = OMFMEM_GOOD_MAGIC;

	switch (openStat) {
	case kOmfMemReadOnly:
	case kOmfMemAppend:
		if (f->memPtr == NULL || f->memSize == 0 || f->endOfData == 0)
			return(FALSE);

		f->currentPos = 0;
		break;

	case kOmfMemCreate:
		if (f->initialBlock <= 0)
			return(FALSE);

		f->memPtr = (char *)omfsMalloc(f->initialBlock);
		if (f->memPtr == NULL)
			return(FALSE);

		f->memSize = f->initialBlock;
		f->currentPos = 0;
		f->endOfData = 0;
		break;

	default:/* should never get here */
		return(FALSE);
		break;
	}


}

static size_t local_fread(void *buffer, size_t itemSize, size_t numItems, omfMemDescriptorPtr_t f)
{
	size_t numBytesToRead;
	size_t itemsRead;

	assert(buffer != NULL);
	assert(f != NULL);
	assert(f->magic == OMFMEM_GOOD_MAGIC);

	if (numItems == 0)
		return(0);

	if (f->currentPos >= f->endOfData) /* past EOF */
		return(0);

	/* first, check size of buffer vs size of read */
	numBytesToRead = itemSize * numItems;
	itemsRead = numItems;

	/* previous EOF check ensures that currentPos is less than endOfData, guaranteeing
	   positive difference (no problem casting to unsigned). */
	if (numBytesToRead > (size_t)(f->endOfData - f->currentPos)) /* read will go past EOF */
	{
		itemsRead = (f->endOfData - f->currentPos) / itemSize;
		numBytesToRead = itemsRead * itemSize;
	}

	memcpy(buffer, (void *)&(f->memPtr[f->currentPos]), numBytesToRead);
	f->currentPos += numBytesToRead;

	/* If we ran into EOF, set pos at end so local_feof() works. This is necessary if there
	   is a fraction of itemSize left at the end of the file. */
	if (itemsRead < numItems)
		f->currentPos = f->endOfData;

	return(itemsRead);

}

static int local_fseek(omfMemDescriptorPtr_t f, int  offset, int mode)
{
	int filePos = 0;

	assert(f != NULL);
	assert(f->magic == OMFMEM_GOOD_MAGIC);

	switch (mode) {
	case SEEK_SET:
		if (offset < 0 || offset >= f->endOfData)
			return(-1); /* offset out of range */
		else
			f->currentPos = offset;

		break;

	case SEEK_END:
 		if (offset > 0 || offset + f->endOfData < 0)
			return(-1); /* offset out of range */
		else
			f->currentPos = f->endOfData + offset; /* count backwards from end */

		break;

	case SEEK_CUR:
		if ((f->currentPos + offset) >= f->endOfData)
			return(-1); /* offset out of range */
		else
			f->currentPos += offset;

		break;

	default:
		return(-2); /* unknown origin mode */
		break;
	}

	return(0);

}

static int local_fclose(omfMemDescriptorPtr_t f)
{
	assert(f != NULL);
	assert(f->magic == OMFMEM_GOOD_MAGIC);

	f->magic = OMFMEM_BAD_MAGIC; /* prevent further operations */

	return(0);

}

static size_t local_fwrite(const void *buffer, size_t itemSize, size_t numItems, omfMemDescriptorPtr_t f)
{
	int  numBytesToWrite;
	int  itemsWritten;

	assert(buffer != NULL);
	assert(f != NULL);
	assert(f->magic == OMFMEM_GOOD_MAGIC);

	if (numItems == 0)
		return(0);

	/* first, check size of buffer vs size of read */
	numBytesToWrite = itemSize * numItems;
	itemsWritten = numItems;

	if (numBytesToWrite > (f->memSize - f->currentPos)) /* past end of memory */
	{
		char *tmpPtr = f->memPtr;
		int  sizeToIncrease;

		if (f->blockSize <= 0) /* check against error */
			f->blockSize = 128*1024; /* arbitrary 128KB */

		sizeToIncrease = f->blockSize;

		/* make sure that the block increment is enough.  It should be, but don't assume */
		while ( ((f->memSize + sizeToIncrease) - f->currentPos) < numBytesToWrite)
			sizeToIncrease += f->blockSize;

		f->memPtr = (char *)omfsMalloc(f->memSize + sizeToIncrease);
		if (f->memPtr == NULL)
		{
			f->memPtr = tmpPtr;
			return(0);
		}
		f->memSize += sizeToIncrease;
		memcpy((void *)f->memPtr, (void *)tmpPtr, f->endOfData); /* only copy data, not extra stuff */
		omfsFree(tmpPtr);
	}

	memcpy((void *)&(f->memPtr[f->currentPos]), (void *)buffer,  numBytesToWrite);
	f->currentPos += numBytesToWrite;
	if (f->currentPos > f->endOfData)
		f->endOfData = f->currentPos;

	return(itemsWritten);

}


omfErr_t        omfsTypedOpenFile(omfSessionHdl_t session,
				                  char *path,
				                  omfHdl_t * result)
{
	return (OM_ERR_NONE);
}

omfErr_t        omfsTypedOpenRawFile(omfSessionHdl_t session,
				                  char *path,
				                  omfHdl_t * result)
{
	return (OM_ERR_NONE);
}


/* INDENT OFF */
/*
;;; Local Variables: ***
;;; tab-width:4 ***
;;; End: ***
*/
