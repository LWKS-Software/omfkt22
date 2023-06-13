/*---------------------------------------------------------------------------*
 |                                                                           |
 |                            <<< cmconfig.h >>>                             |
 |                                                                           |
 |                 Container Manager Configuration Controls                  |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  8/4/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This header is used to defined various "configuration" attributes that control optional
 or alternative versions of various aspects of the Container Manager code.  Some constants
 are also defined here which may be a function of the environment in which the Container
 Manager is run.  Such constants are not in      cmapienv.h      because these also may
 control the way code is generated and this header is not a published header as is the
 case for CM_API_Environment.h.
*/


#ifndef __CMCONFIG__
#define __CMCONFIG__


/*---------------------------------------------------------------*
 | Crap for Lotus (support for "original format 1 TOC layout)... |
 *---------------------------------------------------------------*
 
 The following macros are used to define call "old" code for support of format 1 TOC
 manipulations.  They are sufficiently "grotesque" to make them stand out.  That is because
 the support of the "old" TOC format is only there TEMPORARILY for Lotus and initial
 format size comparisons with the "new" and more compact TOC format.  To make it easier to
 find and remove the bracketed code, we make these macros stand out.
*/

#ifndef TOC1_SUPPORT
#define TOC1_SUPPORT 1          /* Must be explicitly requested as compiler option!     */
#endif

#if TOC1_SUPPORT                /* function call TOC format 1 alternative with result   */

/* (OMF) Doug Cooper - 04-02-96 - removed ## from macro definitions and 
 * replaced with single whitespace.  Function calls do not require that their
 * arguments be concatenated back-to-back,  and the NeXTStep 'cc' is choking
 * on this use of the ## directive.
 */
#define USE_TOC_FORMAT_1_ALTERNATIVE(format1Proc, params) \
  if (((ContainerPtr)container)->majorVersion == 1)       \
    return format1Proc params
#else
#define USE_TOC_FORMAT_1_ALTERNATIVE(format1Proc, params) 
#endif

#if TOC1_SUPPORT                /* function call TOC format 1 alternative with no result*/
#define USE_TOC_FORMAT_1_ALTERNATIVE1(format1Proc, params)  \
  if (((ContainerPtr)container)->majorVersion == 1) {       \
    format1Proc params;                                    \
    return;                                                 \
  }
#else
#define USE_TOC_FORMAT_1_ALTERNATIVE1(format1Proc, params)  
#endif


/*--------------------*
 | Buffering Controls |
 *--------------------*
 
 Some routines buffer their I/O.  The following define the buffer sizes used.  Where 
 indicated, some sizes may be defined as 0 to disable the explicit buffering.  This would
 usually be done if the the I/O is through handlers which do their own buffering so there
 would be n need to do additional buffering by the handler callers.
 
 Unless indicated, these buffers are only allocated during the time they are used. These 
 uses are generally at mutually exclusive times and thus the total space defined is not
 the total space used.
 
 The buffers being defined here are used as the buffer size parameter to  BufferIO.c 's
 cmUseIOBuffer() which allocate the buffers.  The buffering routines will take any
 (reasonable) size.  It does not have to be a multiple of something.  Although that 
 probably would not hurt!
*/
 

/* The reading and writing the TOC is buffered.  The following defines the buffer size  */
/* used...                                                                              */

#if TOC1_SUPPORT
#ifndef TOCInputBufSize
#define TOCInputBufSize (0)         /* This defines the size of the TOC input buffer. It*/
#endif                              /* should be a "moderate" size (maybe about 50*24)  */
                                    /* chosen as a comprimize to wanting to read large  */
                                    /* chunks of TOC and not wanting to read too far    */
                                    /* into a "split" TOC which has a public and private*/
                                    /* section when the TOC is used for updating a      */
                                    /* target container.                                */
                                        
                                    /* THIS CAN BE 0.  No explict TOC buffering will be */
                                    /* done. It is then assumed that either no buffering*/
                                    /* is wanted (why?), or that the handlers do the    */
                                    /* buffering.                                       */
                                    
                                    /* If the handlers are doing standard stream I/O,   */
                                    /* then presumably a setvbuf() can be done to allow */
                                    /* the stream I/O to do the buffering so none would */
                                    /* be needed to be defined here.                    */
#endif

#ifndef UpdateBufSize
#define UpdateBufSize     512       /* Max size of the updates buffer, i.e., the buffer */
#endif                              /* used to write updating instructions into as value*/
                                    /* data.  This can NEVER be 0.  Update instructions */
                                    /* are always buffered because the handlers are not */
                                    /* explicitly used as is the case for TOC I/O.      */
                                    
                                    /* A "moderate" size should be chosen for this since*/
                                    /* the buffer is reused for each object that has    */
                                    /* updates associated with it.  It come down to     */
                                    /* guessing how much updating will be done to the   */
                                    /* values belonging to an object.                   */

#define DefaultTOCBufSize 1         /* Default TOC output buffer size in units of 1024  */
                                    /* bytes. A value of "n" means n*1024 bytes is used */
                                    /* as the TOC output buffer size.  The TOC is       */
                                    /* formatted in "blocks" of n*1024 and will never   */
                                    /* span (cross) a block boundary.  Padding is added */
                                    /* if necessary.  The n here is placed in the       */
                                    /* container label and is limited to a maximum value*/
                                    /* of 65535 (0xFFFF).                               */
                                    
                                    /* For reading, whatever "n" is found in the label  */
                                    /* is used to determine the input TOC buffer size.  */
                                    /* Thus, there can be no default for the input      */
                                    /* buffer size.                                     */

#define RefsBufSize       (64*8)    /* Size of the input buffer used to search for      */
                                    /* a value's references. See "Reference Shadow List */
                                    /* Controls" for a brief description of references. */
                                    /* The value data is formatted as 8-byte CMReference*/
                                    /* key/object ID pairs (4 bytes each).  Almost any  */
                                    /* reasonable size will do here depending on your   */
                                    /* estimate on how heavily references are used.     */


/*--------------------*
 | Debugging Controls |
 *--------------------*
 
 The following two switches control whether the indicated debugging code should be 
 generated. The first controls the generation of CMDebugging() while the second controls
 the generation of CMDumpTOCStructures() and CMDumpContainerTOC().  These two routines have
 the following actions:
 
 CMDebugging()          - to set some internal debugging "options"
 CMDumpTOCStructures()  - to dump in-memory TOC as a tree-like format
 CMDumpContainerTOC()   - to read in container TOC and display it in a table format
*/

#ifndef CMDEBUGGING
#define CMDEBUGGING 0     /* 1 ==> define CMDebugging()                                 */
#endif

#ifndef CMDUMPTOC
#define CMDUMPTOC 0       /* 1 ==> define CMDumpTOCStructures() and CMDumpContainerTOC()*/
#endif


/*------------------------------------*
 | Validation and Protection Controls |
 *------------------------------------*

 The following two switches control the validation checking code in the Container Manager.
 Validations include checking that CMStartSession() was called and for NULL refNums. There
 is a dynamic switch controlled by CMDebugging() (the switch is in the session global data)
 and a static (compile time) switch to suppress the validation checks entirely.
 
 Remember, suppressing the validation checks removes what little protection the Container
 Manager has against sadistic (or just plain stupid or careless) users.  If this stuff
 blows, don't bug me!
*/

#ifndef CMVALIDATE
#define CMVALIDATE 1            /* 1 ==> generate validation code; 0 ==> don't generate */
#endif

#ifndef CMDEFAULT_VALIDATE
#define CMDEFAULT_VALIDATE 1    /* default (initial) dynamic validation switch setting  */
#endif


/*--------------------------------*
 | Reference Shadow List Controls |
 *--------------------------------*

 Object references are "pointers" (i.e., object IDs) to other objects from a value 
 (CMValue) that contains data that refers to those objects.  The data is in the form of a
 CMReference "key" and associated object ID.  The routines in    CMRefOps.c    manipulate
 those key/object (ID) associations.  The associations are maintained in a list as value
 data for a uniquely typed value in a private object "tied" to the value through a pointer
 in the value header.

 To increase efficiency, the data is read into memory the first time it's accessed.  From
 that point on, all updates to the actual data are "shadowed" in the in-memory list.  The
 updating can't be avoided (so updating, e.g., touches, etc. work correctly).  But at least
 the searches can be made to not be I/O bound!  This shadow list support is OPTIONAL and
 controlled by the macro variable defined below.  If the variable is defined as 0, no
 shadow list support is provided.  All search operations for a reference will take place
 directly on the value data.  This has the benefit of not requiring additional memory 
 space.  It also could potentially be more efficient depending on the supporting I/O
 handlers.  These are the reasons the shadow list support is optional.
*/

#ifndef CMSHADOW_LIST
#define CMSHADOW_LIST 1         /* 1 ==> support reference shadow list                  */
#endif

#endif
