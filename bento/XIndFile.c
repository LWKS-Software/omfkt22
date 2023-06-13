/* (use tabs = 2 to view this file correctly) */
/*---------------------------------------------------------------------------*
 |                                                                           |
 |                      <<<        XIndFile.c        >>>                     |
 |                                                                           |
 |            Example Indirect File Dynamic Value Handler Package            |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                  4/4/92                                   |
 |                                                                           |
 |                    Copyright Apple Computer, Inc. 1992                    |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This file illustrates how one might implement indirect values using the "dynamic value" 
 mechanisms provided by the Container Manager.  Dynamic values are special values which
 "know" how to manipulate on their own value data.  They do this through a set of value
 operation handlers which are expected to be semantically the same as the standard API
 value operations.  This is described in more detail later.
 
 The indirect value we want to support here is for data "pointed to" by a file pathname.
 All value operations on the indirect value will then be to the file.
  
 Note, multiple indirections are permitted!  That is the data pointed to by an indirect
 value might be for another indirect value.
 
 There is one restriction on this indirect value implementation. That is that data inserts
 and deletes are NOT supported.  File indirections are supported using standard C stream
 I/O for portability of this example.  With standard C there is no way to cut a file size
 down.  Hence deletes cannot be done.  Although inserts could be supported, the cost in
 data I/O is potentially expensive.  Thus inserts are not supported either.
 
 Basically, dynamic values are similar to C++ objects where the handlers are the object's
 methods.  Dynamic values are a generalized mechanism that provides for type inheritance.
 They are similar to C++ objects where the handlers are the object's methods and an
 object's type represents its class.  They are dynamic in the sense that they only exist
 from creation (discussed below) and last until until they are released (CMReleaseValue()).

 The indirect file example we will illustrate here illustrates the type inheritance where
 the type of the indirect value (which describes what we're kind of file its using) has as
 its base type, a "file" type.  The file type will be used to create the dynamic value
 whose value operation handlers will effect the file operations.

 Follow the code in this file in the order presented.  The comments document the steps
 required to support indirection through a file pathname.  The newIndirectFileValue()
 routine defines the interface that can be used to create an indirect file value.  It
 would be called just like any other Container Manager API routine.
*/


/*------------------------------------------------------------------------------------*
 | DOS (80x86) USERS -- USE THE "LARGE" MEMORY MODEL WHEN COMPILED FOR 80X86 MACHINES |
 *------------------------------------------------------------------------------------*

 The Container Manager is built with this same option and assumes all handler and 
 metahandler interfaces are similarly built and can be accessed with "far" pointers.
*/


/*#include <types.h>*/
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#ifndef __CM_API__
#include "CMAPI.h" 
#endif
#ifndef __XINDFILEHANDLERS__
#include "XIndFile.h"              
#endif

                                  CM_CFUNCTIONS
                                  
/* See comments in      cmapienv.h      about CM_CFUNCTIONS/CM_END_CFUNCTIONS (used     */
/* only when compiling under C++).                                                      */


/* In order to make this code read from the top down, it is necessary in a few instances*/
/* to forward reference some metahandler functions.  Due to some "complaints" from      */
/* certain C compilers (who shall remain nameless), the forward declarations are placed */
/* here.  Basically, ignore this stuff.                                                 */

static CMHandlerAddr fileValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType);


/* GENERAL NOTE...

   In order to make the code as general as possible, the session handlers provided by the
   API are used.  This includes memory allocation, deallocation, error reporter, and
   access to the container identification.  These interfaces allow us to use the same 
   memory and error handlers that the Container Manager uses as provided to
   CMStartSession() through the methandler passed there.  The container identification
   interface allows us to add that information to the error reports.  The wording of these
   messages follows the "flavor" of those done by the Container Manager itself.
*/


/*--------------------------------------------------------------------------------*
 | newIndirectFileValue - create refNum for a value that indirects to a data file |
 *--------------------------------------------------------------------------------*

 This is a special case of CMNewValue() in that a new value entry of the specified type is
 created for the specified object with the specified property.  A refNum for this value is
 returned. However, unlike CMNewValue(), the refNum is associated with an value whose data
 (file) is indirectly pointed to by a pathname.  Whenever the returned refNum is used for
 a value operation, that operation will take place on the file pointed to by the pathname.
 
 The specified file is opened according to the open mode.  This may have any one of the
 the following values:
   
         "r", "rb"      open the file for reading
         "r+", "rb+"    open the file for updating
         "w+", "wb+"    truncate or create file and open for updating (reading/writing)
         
 The "b" is always implied if it is not explicitly specified.  If a write mode ("w") is
 specified with a "+", the "+" is assumed.
 
 Note, if the file is opened for writing ("wb+"), then all future uses of the value, i.e.,
 via CMUseValue() will imply update mode ("rb+").  Further, dynamic value operations do
 both reads and writes of their data.  Since a file type may be a base type of some other
 type, update mode is always assumed ("wb+").
 
 If a file is opened for reading, then a CMUseValue() will open preserve the open status
 and open for reading only.  Similarly, update mode is preserved.  The write is the only
 exception.  This is because there is no way (in this example at least) to tell
 CMUseValue() a change in open mode.  Sorry!  It bothers me too!
 
 The way we implement this function is to create a new value according to the passed 
 object, property, and type.  But it is NOT this value we want to return.  What we want to
 do is create a dynamic value based on this value.
 
 Basically all we do here is make sure the proper types are registered and based prior
 to doing a "normal" CMNewValue().  But it isolates the type registration to this one 
 place so we can illustrate how to create the desired dynamic value.
 
 The types are the key to generating dynamic values.  A dynamic value results when a value
 is created by to CMNewValue() or attempted to be used by CMUseValue(), and the following
 two conditions occur:
 
  1. The type or any of its base types (if any, created by CMAddBaseType()), are for
     global names that have associated metahandlers registered by CMRegisterType().
     
  2. The metahandlers support the operation type to return a "use value" handler, and
     in addition for CMNewValue(), "metadata" and "new value" handlers.
 
 "Metadata" handlers are used to define metadata that is a format description that directs
 CMNewValue() on how to interpret it's "..." parameters to produce a data packet of those
 parameters.  The data packet is sent to the "new value" handler.  In this routine we will
 do a CMNewValue() to create the dynamic value with the newIndirectFileValue() pathname and
 mode passed as the "..." parameters.

 "New value" handlers are used to define initialization data for the "use value" handlers
 based on the data packets.  The "use value" handlers are called to set up and return
 refCons.  Another metahandler address is also returned from the "use value" handler.  This
 is used to get the address of the value operation handlers corresponding to the standard
 API value routines.

 The goal is to create a dynamic value to return, which when used will produce the desired
 indirect-to-a-file effect. Thus we need two types:
 
  1. A type that describes the meaning of this indirect value.  That is the type passed to
     this routine.
      
  2. A base type that represents a "file" in general.  The user's type is given the "file"
     type as its base type. 
     
 It is that general "file" base type that most of this file supports.  Namely the various
 metahandlers and handlers for files (stream files in this example).  Indeed, the only
 thing we do here is set up the types, register their methandlers, and do the CMNewValue().
*/

CMValue CM_FIXEDARGS newIndirectFileValue(CMObject object, CMProperty property,
                                          CMType type,
                                          char CM_PTR *pathname, char CM_PTR *mode)
{
  CMContainer container;
  CMSession   sessionData;
  CMType      fileType;
  CMValue     dynamicValue;

  /* Get the container refNum and session data pointer. They are needed for registering */
  /* metahandlers and types and for reporting errors.  Note, if the container is passed */
  /* as NULL, the session pointer will end up being NULL. In that case, we can't report */
  /* the error and just return NULL.                                                    */
  
  container   = CMGetObjectContainer(object);
  sessionData = CMGetSession(container);
  
  if (sessionData == NULL)              /* if no session...                             */
    return (NULL);                      /* ...there is nothing much else we can do here */
  
  /* Do a safety check on the refNums...                                                */
  
  if (object == NULL || property == NULL || type == NULL || pathname == NULL || mode == NULL) {
    CMError(sessionData, "Null object, property, type refNum or null pathname or mode passed to newIndirectFileValue()");
    return (NULL);
  }
  
  /* If you wanted to really be paranoid, you could check that the property and type    */
  /* both belonged to the same container.  But lets be "trusting" for now!              */
    
  /* Register the "file" type (or reregister it just to make sure its registered) and   */
  /* then define the metahandler for that type which will return the "metadata", "new   */
  /* value", and "use value" handlers (described later in this file).  Note, the "file" */
  /* global name, as well as the metahandler are published in the indirect file header  */
  /* file to allow other combinations of base types that might want files as one of     */
  /* their inherited types.  Further, to use the value we're creating in the future     */
  /* sessions, the caller simply does a CMUseValue().   But that requires the type and  */
  /* its  metahandler be registered just as we do here.  As a convenience we provide a  */
  /* macro to do this in the header file.  For clearity, we do it explicitly here.      */
  
  CMSetMetaHandler(sessionData, FileValueGlobalName, fileDynamicValueMetahandler);
  fileType = CMRegisterType(container, FileValueGlobalName);
  if (fileType == NULL) return (NULL);
  
  /* The file type will be defined as a base type for the caller's type.  The caller's  */
  /* type is already defined.  It is assumed to be a type that describes what kind of   */
  /* file indirection this is.  The type heirarchy we want looks something like this:   */
  
  /*                                File Type (bottom type)                             */
  /*                                          *                                         */
  /*                                          *                                         */
  /*                                          *                                         */
  /*                                Caller's Type (top type)                            */
                                     
  /* In other words, we want the file type as a base type of the description type. This */
  /* is easily accomplished as follows:                                                 */
  
  CMAddBaseType(type, fileType);
  
  /* Base types are essentially values for a special "base type" property added to the  */
  /* type object.   Above we're defining fileType as a single base type for the type.   */
  
  /* That's about all there is to it!  Now all that's left is to do the CMNewValue()    */
  /* to create the dynamic value.                                                       */
  
  dynamicValue = CMNewValue(object, property, type, pathname, mode);
  
  /* The dynamicValue is now created!  We can return it to the caller.  Note that the   */
  /* pathname and mode are passed to CMNewValue().  It has no idea what these are!  But */
  /* it does know how to deal with them. Specifically, it passes them to the "metadata" */
  /* "new value" handlers during the creation of the dynamic value.  CMNewValue() knows */
  /* there are two additional parameters because of the metadata we defined for the base*/
  /* type. You can see it works something like "printf()", although across two different*/
  /* places.  The metadata tells CMNewValue() to consume the additional parameters just */
  /* like the "printf()" format string tells "printf()".                                */
  
  /* Getting back to the dynamic value -- to reiterate, the rules for dynamic value     */
  /* creation are that when a CMNewValue() or CMUseValue() are done, if the passed type,*/
  /* or any of its base types have (through associated metahandlers) a "use value"      */
  /* handler, and additionally for CMNewValue(), a "metadata" and "new value" handler,  */
  /* a dynamic value is created.                                                        */
  
  return (dynamicValue);  
}

/* That's all there is to it! From this point on we turn our attention to the definition*/
/* of the metahandlers and handlers.                                                    */


/*======================================================================================*/
/*======================================================================================*/


/*---------------------------------------------------------------------------*
 | fileDynamicValueMetahandler - matahandler for file dynamic value creation |
 *---------------------------------------------------------------------------*
 
 The first methandler to be defined is the metahandler called from CMNewValue() or
 CMUseValue() for dynamic value creation.  It does this to see if a type, which had this
 metahandler registered for the type global name, has a "use value" handler, and
 additionally for CMNewValue(), "metadata" and "new value" handlers defined.  That is the
 criteria on whether a dynamic value is to be created.
 
 The "metadata", "new value", and "use value" handlers have the following prototypes:   */
                                
 static CMMetaData metaData_Handler(CMType type);
 static CMBoolean newValue_Handler(CMValue dynamicBaseValue, CMType type,
                                   CMDataPacket dataPacket);
 static CMBoolean useValue_Handler(CMValue dynamicBaseValue, CMType type,
                                   CMMetaHandler *metahandler, CMRefCon *refCon);
                                                                                        /*
 The parameters are defined later in this file in the documentation for these routines.
*/

CMHandlerAddr CM_FIXEDARGS fileDynamicValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType)
{
  CMType ignored = targetType;
  
  if (strcmp((char *)operationType, CMDefineMetaDataOpType)==0) /* "metadata"           */
    return ((CMHandlerAddr)metaData_Handler);
  else if (strcmp((char *)operationType, CMNewValueOpType) == 0)/* "new value"          */
    return ((CMHandlerAddr)newValue_Handler);
  else if (strcmp((char *)operationType, CMUseValueOpType) == 0)/* "use value"          */
    return ((CMHandlerAddr)useValue_Handler);
  else                                                          /* what's the question? */
    return (NULL);
}


/*-------------------------------------------------------------------*
 | metaData_Handler - return the matadata for the indirect file type |
 *-------------------------------------------------------------------*
 
 The "metadata" handler is called from CMNewValue() so that the proper number of its "..."
 parameters can be placed into a data packet to be passed to the "new value" handler. This
 handler takes the type for which we ant to return the corresponding metadata.
 
 In the case of indirect file values, we want to pass the newIndirectFileValue() parameters
 that defines the pathname and open mode.  The metadata "%p %p" will serve the purpose.  It
 specifies a pointer to the pathname and mode strings.
 
 Note, we can define any format we thing is appropriate for the "new value" handler so 
 int  as it uses CMScanDataPacket() to extract the parameters and uses this handler itself
 to get the metadata format string.  That's what we will do when we get to the "new value"
 handler.
*/  

static CMMetaData metaData_Handler(CMType type)
{
  CMType unused = type;
  
  return ((CMMetaData)"%p(=path) %p(=mode)"); 
}


/*----------------------------------------------------------------------*
 | newValue_Handler - create permanent data for the "use value" handler |
 *----------------------------------------------------------------------*
 
 The "new value" handler is called from CMNewValue() for dynamic value creation just prior
 to calling the "use value" handler.  It is not used by CMUseValue().

 "New value" handlers are used to define initialization data for the "use value" handlers.
 It is assumed that such data will be some function of the original parameters passed to
 CMNewValue().  These are packed into the dataPacket according to the metadata defined for
 the type.  If there are multiple base types, each type's metadata directs CMNewValue() on
 how many of its "..." parameters to consume in build a data packet.
 
 Base types are processed in a depth-first manner, so CMNewValue() "..." parameters are
 consumed left-to-right, for the deepest base type to the highest.  For example, given the
 following type inheritance heirarchy (read these as T1 inherits from T2 and T3, and so
 on):
 
                                      T4      T5
                                        *    *
                                         *  *
                                  T2      T3
                                    *    *
                                     *  *
                                      T1
              
 The depth-first search, starting at T1, yields the sequence: T2 T4 T5 T3 T1.  Then this
 is the order the CMNewValue() "..." parameters must be in.  It is also the order the
 dynamic value layers are generated.  T1 is the top layer, and T2 the bottom.  T1 would be
 the dynamic value returned from CMNewValue().
 
 In this example we only have a single base type, the "file" type, which has metadata
 defined to pass the file pathname and open mode string pointers.
 
 From this data we want to write permanent information so that the "use value" handler,
 which is always called for CMNewValue() and CMUseValue(), can create an appropriate refCon
 to pass among the value operation handlers.  The data layout chosen is as follows:     */
 
 struct FileData {                  /* File data layout:                                */
   char  mode[4];                   /*    open mode                                     */
   short pathnameLength;            /*    size of the pathname (including null)         */
 };                                 /*    the pathname follows the size                 */
 typedef struct FileData FileData;
 
 #define FileDataSize (sizeof(FileData))                          
                                                                                        /*
 In the above data, we only have to remember the open mode, the pathname length and the
 pathname string.  This is the data we will write to the dynamic value's base value.  It is
 thus permanently remembered.  Now, if the container is opened for reading, if a
 CMUseValue() is done on the value, the "use value" handler will read the data just as in
 the CMNewValue() case.  Remember that newIndirectFileValue() is done only to initially
 create the crypt value.  Future uses don't call it.  So nothing should be written that
 can't be used by only calling "use value" handler without its "new value" handler.
 
 Note, that the data includes the length of the pathname.  This is necessary so that the
 "use value" handler knows how much to read for the pathname.  This will therefore require
 multiple reads by the "use value" handler.
 
 On final point, if any errors are reported, we abort the execution of the program in this 
 example.  As documented, error reporters are not supposed to return.  But in case they
 do, the Container Manager wants to know about it!  The CMBoolean function result is used
 for that purpose.  Nonzero should be returned for success, and if this should happen to
 return for a failure, 0 should be returned as the function result.
*/

static CMBoolean newValue_Handler(CMValue dynamicBaseValue, CMType type, CMDataPacket dataPacket)
{
  CMContainer container   = CMGetObjectContainer(type);
  CMSession   sessionData = CMGetSession(container);
  CMCount     i;
  char        *pathname, *mode0, *mode, *m, c;
  FileData    fileData;
  
  /* Scan the dataPacket to get the pathname and mode back into variables.  The API     */
  /* provides CMScanDataPacket() to do that.  We reverse what CMNewValue() did by using */
  /* our own "metadata" handler to get the metadata to direct the extraction of         */
  /* dataPacket fields back to variables (sort of like a "scanf()").                    */
  
  i = CMScanDataPacket(type, metaData_Handler(type), dataPacket, &pathname, &mode);
  
  if (i != 2) {
    CMError(sessionData, "Unable to get all of CMNewValue()'s \"...\" pathname parameter in container \"^0\"", CMReturnContainerName(container));
    return (0);
  }
  
  /* Set the pathname length in the data buffer (include the null)...                   */
  
  fileData.pathnameLength = (short)(strlen(pathname) + 1);  
    
  /* Validate the open mode and put it in the data buffer. We only accept "w[b][+]" and */
  /* "r[b][+]".  We always insert the "b" if it isn't there.  We also insert "+" on "w" */
  /* if it isn't there.                                                                 */
  
  mode0 = mode;                             /* save original mode for possible errors   */
  m = fileData.mode;                        /* build mode directly in data buffer       */
  c = *mode++;
  if (c == 'w' || c == 'r') {               /* "r"        "w"                           */
    *m++ = c;
    c = *mode++;
    if (c == 'b') {                         /* "rb"       "wb"                          */
      *m++ = 'b';
      c = *mode++;
      if (c == '+') {                       /* "rb+"      "wb+"                         */
        *m++ = '+';       
        c = *mode++;
      } else if (*fileData.mode == 'w')     /*            "wb+"                         */
        *m++ = '+';                         
    } else if (c == '+') {                  /* "r"        "w+"                          */
      *m++ = 'b';
      *m++ = '+';                           /* "rb+"      "wb+"                         */
      c = *mode++;
    } else {                                /* "rb"       "wb"                          */
      *m++ = 'b';
      if (*fileData.mode == 'w')            /*            "wb+"                         */
        *m++ = '+';
    }
  } else                                    /* error                                    */
    c = '?';
  
  if (c != '\0') {                          /* must be looking at null delimiter in mode*/
    CMError(sessionData, "Invalid open mode (\"^0\") in container \"^1\"", mode0, CMReturnContainerName(container));
    return (0);
  }
  
  *m = '\0';                                /* null at end of the mode string           */
  
  /* Write the data buffer that "use value" will read. We are going to "cheat" a little */
  /* here and write pathname separately just like the "use value" handler will read it. */
  
  CMWriteValueData(dynamicBaseValue, (CMPtr)&fileData, 0, FileDataSize);
  CMWriteValueData(dynamicBaseValue, (CMPtr)pathname, FileDataSize, (CMSize)fileData.pathnameLength);
  
  return (1);                               /* that's all there is to do here!          */
}


/*--------------------------------------------------------------------------*
 | useValue_Handler - create a dynamic value and its associated metahandler |
 *--------------------------------------------------------------------------*
 
 The "use value" handler is called for both CMNewValue() and CMUseValue().  For
 CMNewValue(), it is called after the "new value" handler.  Either way, the "use value"
 handler is expected to build a refCon to pass among its value operation handlers, and
 to return a pointer to the metahandler that will yield the addresses of those handlers.
 
 Generally, the refCon is built from the data written to a dynamic values base value by
 the "new value" handler.  That data is permanent, and thus a CMUseValue() on the value
 of the dynamic value type will cause the creation of the dynamic value even when the
 "new value" handler is not called (as is the case for CMUseValue()).

 As with the "new value" handler, the type is passed as a convenience.  It may or may not
 be needed.  For the "new value" handler it is passed to CMScanDataPacket() to convert
 the data packet back into variables.  Here, it may not be needed.  It is possible for a
 type object to contain OTHER data for other properties.  Types, after all, are ordinary
 objects.  There is nothing prohibiting the creation of additional properties and their
 values.  This fact could be used to pass additional (static?) information to the "new
 value" or "use value" handlers which would read the data values.

 In this example, the "new value" handler data written to the base value is as defined by
 the FileData struct defined in the "new value" handler.  It contains the open mode and
 pathname.
 
 The refCon we build from this data has the following layout:                           */
 
 struct FileRefCon {                        /* File handler's refCon:                   */
   CMSession sessionData;                   /*    ptr to the current container session  */
   FILE      *f;                            /*    file variable to pointed to file      */
   char      pathname[1];                   /*    the START of the pathname             */
 };
 typedef struct FileRefCon FileRefCon, *FileRefConPtr;                                  /*

 The current session data pointer, which we need to allocate the refCon is kept just as a
 convenience in case we need it again.  At the very least it will be needed to free the
 refCon.

 The refCon is accessable to all the handler routines via a CMGetValueRefCon() call.
 
 Note, as with the "new value" handler, if any errors are reported, we abort the execution
 of the program in this example. As documented, error reporters are not supposed to return.
 But in case they do, the Container Manager wants to know about it!  The CMBoolean function
 result is used for that purpose.  Nonzero should be returned for success, and if this
 should happen to return for a failure, 0 should be returned as the function result.
*/

static CMBoolean useValue_Handler(CMValue dynamicBaseValue, CMType type,
                                  CMMetaHandler *metahandler, CMRefCon *refCon)
{
  CMContainer   container   = CMGetObjectContainer(type);
  CMSession     sessionData = CMGetSession(container);
  FileRefConPtr myRefCon;
  FileData      fileData;

  /* Read in the the open mode and pathname length written by the "new value" handler   */
  /* according to the FileData format...                                                */
  
  if (CMReadValueData(dynamicBaseValue, (CMPtr)&fileData, 0, FileDataSize) != FileDataSize) {
    CMError(sessionData, "Incorrect byte length read while reading indirect (pathname) value data in container \"^0\"", CMReturnContainerName(container));
    return (0);
  }
  
  /* Allocate the refCon that we will pass among the handlers.  Since we are doing a    */
  /* dynamic allocation here we will use the "malloc" handler defined for the container.*/
  
  myRefCon = (FileRefConPtr)CMMalloc(sizeof(FileRefCon) + fileData.pathnameLength, sessionData);
  if (myRefCon == NULL) {
    CMError(sessionData, "Cannot allocate space for file handler refCon in container \"^0\"", CMReturnContainerName(container));
    return (0);
  }

  myRefCon->sessionData = sessionData;                /* save the current session ptr   */
  
  /* Read the pathname directly into our allocated refNum...                            */
  
  if (CMReadValueData(dynamicBaseValue, (CMPtr)myRefCon->pathname, FileDataSize, (CMSize)fileData.pathnameLength) != (CMSize)fileData.pathnameLength) {
    CMError(sessionData, "Incorrect byte length read while reading pathname in container \"^0\"", CMReturnContainerName(container));
    CMFree(refCon, sessionData);
    return (0);
  }
  
  /* Open the file.  We use the open mode provided in the fileData.  If its is "wb" or  */
  /* "wb+" then it can only be from newIndirectFileValue(). This is because after we do */
  /* the open, we CHANGE the open mode in the data to "rb+" for future updating.  This  */
  /* is not a satisfying solution.  But there is no clean way to tell a CMUseValue(),   */
  /* which is all future uses of the value that will get to us here, there's no clean   */
  /* way to pass a different open mode.  We could clutter up this example and provide a */
  /* "useIndirectFileValue()" that did the "CMUseValue() for the caller and changed the */
  /* data to the appriate mode (a parameter to the useIndirectFileValue) BEFORE doing a */
  /* CMUseValue().  That would fake out this routine. But even that would only work for */
  /* a container opened for update mode because we're rewriting the data we're using    */
  /* here.  Still another solution would be to pass the mode is a static global!  But   */
  /* we have been avoiding them like the plague everywhere. The bottom line -- screw it!*/
  
  myRefCon->f = fopen(myRefCon->pathname, fileData.mode); /* open the file...           */
  if (myRefCon->f == NULL) {                              /* oops!                      */
    CMError(sessionData, "Cannot open \"^0\" using mode \"^1\" for container \"^2\"\n    ^3",
            myRefCon->pathname, fileData.mode, CMReturnContainerName(container), strerror(errno));
    CMFree(refCon, sessionData);
    return (0);
  }
  
  if (*fileData.mode == 'w') {                        /* if file was opened for writing */
    strcpy(fileData.mode, "rb+");                     /* future opens will be updating  */
    CMWriteValueData(dynamicBaseValue, (CMPtr)&fileData, 0, FileDataSize); 
  }                                                   /* it's forever changed!          */

  /* All that's left to do here is to return the refCon and the pointer to the value    */
  /* operations metahandler to the Container Manager who called us...                   */
  
  *metahandler = (CMMetaHandler)fileValueMetahandler; /* return metahandler             */
  *refCon      = (CMRefCon)myRefCon;                  /* ...and refCon                  */
    
  return (1);                                         /* we're now ready to go          */
}


/*----------------------------------------------------------------------------*
 | fileValueMetahandler - metahandler for a dynamic value operations handlers |
 *----------------------------------------------------------------------------*
 
 This is the metahandler that the "use value" handler returns to the Container Manager.
 It defines the addresses for all the value operations handlers supported for files.
 
 The dynamic value operations handlers have the following prototypes:                   */
 
 static CMSize getValueSize_Handler(CMValue value);
 static CMSize readValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize maxSize);
 static void writeValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize size);
 static void insertValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize size);
 static void deleteValueData_Handler(CMValue value, CMCount offset, CMSize size);
 static void getValueInfo_Handler(CMValue value, CMContainer *container, CMObject *object,
                                  CMProperty *property, CMType *type, 
                                  CMGeneration *generation);
 static void setValueType_Handler(CMValue value, CMType type);
 static void setValueGeneration_Handler(CMValue value, CMGeneration generation);
 static void releaseValue_Handler(CMValue value);
                                                                                        /*
 Note that these handlers have exactly the same calling conventions as the standard API
 routines.  These handlers must also have the same semantics.  What happens is that every
 time a dynamic value is passed to the Container Manager, it is validated and then checked
 to see if it is indeed a dynamic value.  If it is, the corresponding handler is called to
 do the work.

 The actual handler called for the dynamic value may or may NOT be associated with the
 type associated with the dynamic value.  Just like C++, dynamic values may be "subclassed"
 via their (base) types.  If a handler for a particular operation is not defined for a
 value, its "base value" is used to get the "inherited" handler.  This continues up a
 dynamic value's chain of base values, up to the original "real" value that spawned the
 base values from the CMNewValue() or CMUseValue().
 
 In this example we had two types, the "file" base type and its base "real" value type,
 i.e, the one passed to newIndirectFileValue().  The dynamic value returned from there was
 for the file.  So if we don't supply a value handler for a particular operation, the
 operation will end up be done to the base "real" value.
 
 Taking advantage of this means you don't have to supply a handler unless it is absolutely
 necessary.  This minimizes the coding, not to mention code size!  We actually do take 
 advantage of the method ('er, excuse me, value operation) inheritance here.  The file
 operations for the "getValueInfo", "setValueType", and "setValueGeneration" are not
 defined (i.e., we return NULL for their address) as handlers.
 
 For "getValueInfo", "setValueType", and "setValueGeneration" it's obvious you want to use
 the base "real" value.  Note however, if the type is changed, you are "pulling" the rug
 out" from under the dynamic value creation.  Once released, the value will not have a file
 base type, unless, of course, the new type had one.
 
 Note that we actually declare all the value operations handlers as "static" since they
 only need to be visible in this file.  The Container Manager gets at them through the
 addresses we return from here.
 
 The handlers do their work by calls back to the Container Manager.  But the values passed
 back cannot be the dynamic value passed in.  This could lead to a recursive and endless
 loop.  This IS detected by the Container Manager as an error condition.
 
 In our indirect file case, the handlers will use the refCon to do their respective
 operations on the file.  Remember stream file variable was placed in the refCon by the
 "use value" handler when it opened the file.
 
 By the time a dynamic value operation handler has been called, all validation checks have
 been done on the dynamic value by the Container Manager.  So they need not be done every
 time here.  Validation checks include such things as the Container Manager being
 initialized, non-NULL value pointers, etc.  In short, all the validations the value
 routines do for normal values before discovering it should call a dynamic value handler.
 
 Note, as documented in the comments at the start of this file, we do not support data
 deletes and inserts.  However, we do define handlers for these.  We could not provide
 handlers and just return NULL for those operation types.  But by providing handlers we
 can report a more appropriate error message than the more generic one used by the API
 when it descovers it needs a handler and none was provided.
 
 One other thing to keep in mind.  Since these handlers operate directly on a file, a file
 type can only be used as a bottom-most layer of a set of layered dynamic values.  For
 example, an encryption type can have a file type as its base value.  But the file type
 cannot have any meaningful additional base types of its own.  So you couldn't, for
 example, have a file type that has an encryption base.
 
 In theory, given a different definition of "file" types, i.e., what their handlers do, 
 you could have a file type with an encryption base type.  Only the fact that "file" 
 type handlers don't do their operations in terms of their base value is prohibiting this.
*/

static CMHandlerAddr fileValueMetahandler(CMType targetType, CMconst_CMGlobalName operationType)
{
  static char *operationTypes[] = {CMGetValueSizeOpType,    /* 0 */ /* Operation Types  */
                                   CMReadValueDataOpType,   /* 1 */
                                   CMWriteValueDataOpType,  /* 2 */
                                   CMInsertValueDataOpType, /* 3 */
                                   CMDeleteValueDataOpType, /* 4 */
                                   CMGetValueInfoOpType,    /* 5 */
                                   CMSetValueTypeOpType,    /* 6 */
                                   CMSetValueGenOpType,     /* 7 */
                                   CMReleaseValueOpType,    /* 8 */
                                   NULL};
  char   **t;
  CMType ignored = targetType;
  
  /* Look up the operation type in the operationTypes table above...                    */
  
  t = operationTypes - 1;
  while (*++t) if (strcmp((char *)operationType, *t) == 0) break;

  /* Now that we got it (hopefully), return the appropriate routine address...          */
  
  switch (t - operationTypes) {
    case  0:  return ((CMHandlerAddr)getValueSize_Handler);   /* CMGetValueSizeOpType   */
    case  1:  return ((CMHandlerAddr)readValueData_Handler);  /* CMReadValueDataOpType  */
    case  2:  return ((CMHandlerAddr)writeValueData_Handler); /* CMWriteValueDataOpType */
    case  3:  return ((CMHandlerAddr)insertValueData_Handler);/* CMInsertValueDataOpType*/
    case  4:  return ((CMHandlerAddr)deleteValueData_Handler);/* CMDeleteValueDataOpType*/
    case  5:  return (NULL);/* use inherited handler or API *//* CMGetValueInfoOpType   */
    case  6:  return (NULL);/* use inherited handler or API *//* CMSetValueTypeOpType   */
    case  7:  return (NULL);/* use inherited handler or API *//* CMSetValueGenOpType    */
    case  8:  return ((CMHandlerAddr)releaseValue_Handler);   /* CMReleaseValueOpType   */
    
    default:  return (NULL);
  }
}


/*======================================================================================*/
/*======================================================================================*/

/* What follows now are the individual dynamic value operations handlers whose addresses*/
/* we returned from the above metahandler.  As stated above, these routines MUST have   */
/* the same semantics (within reason) as the corresponding API routine that calls them. */
/* If a routine doesn't make sense for the dynamic value it should report an error.     */

/* All validation has already been done by the time a handler routine is called. Except */
/* for validation, the API routine that calls the handler does NOTHING else.  Note,     */
/* however, that since there is a one-to-one mapping between the handlers and their     */
/* corresponding API routines, many handlers end up doing a call back to the API to do  */
/* their tasks passing the parameters unaltered (e.g., buffer pointers, sizes, etc.).   */
/* The API always trys to validate its parameters as best it can.  For example, a NULL  */
/* buffer pointer is processed appropriately.  What all this means is that the handlers */
/* do not need to do these valiations either on the parameters.  It can be left to the  */
/* API if the parameters are passed in a call back.                                     */

/* What each of these handlers must do first, usually, is get the refCon we built in    */
/* the "use value" handler.  The refCon contains the file variable for the file to be   */
/* operated upon by the handlers.                                                       */


/*---------------------------------------------------------------*
 | getValueSize_Handler - CMGetValueSize() dynamic value handler |
 *---------------------------------------------------------------*
 
 The character size of the file is returned.  We do this be seeking to the end of the file
 and getting that position.  Other systems might have more efficient (?) ways of doing
 this.
*/

static CMSize getValueSize_Handler(CMValue value)
{
  FileRefConPtr refCon = (FileRefConPtr)CMGetValueRefCon(value);
  int           err;
  
  fseek(refCon->f, 0L, SEEK_END);                       /* seek to the end of file      */
  
  err = ferror(refCon->f);                              /* check for error...           */
  if (err != 0) {
    CMError(refCon->sessionData, "Error trying to get file size of file \"^0\" for container \"^1\"\n    ^2",
            refCon->pathname, CMReturnContainerName(CMGetValueContainer(value)), strerror(err));
    return (0);
  }
  
  return ((CMSize)ftell(refCon->f));                    /* size is where we are         */
}


/*----------------------------------------------------------------*
 | readValueData_Handler - CMReadValueData() dynamic value handlr |
 *----------------------------------------------------------------*

 The data, starting at the offset, for the file is read into the buffer.  The size of the
 data read is returned.  Up to maxSize characters will be read (can be 0).
 
 The data is read starting at the offset, up to the end of the data, or maxSize characters,
 whichever comes first.  Offsets are relative to 0, where 0 is the first byte of the
 file.  If the starting offset is greater than or equal to the current file size, no data
 is read and 0 returned.
*/

static CMSize readValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize maxSize)
{
  FileRefConPtr refCon   = (FileRefConPtr)CMGetValueRefCon(value);
  CMSize        fileSize = getValueSize_Handler(value);
  CMSize        amountRead;
  int           err;
  
  /* The offset is with respect to the file.  If it is beyond the end of the file we    */
  /* we don't read anything.                                                            */
  
  if (offset >= fileSize) return (0);
  
  /* Read the data from the file.  Up to maxSize or fileSize - offset bytes are read,   */
  /* whichever is smaller.  The actual amount read is returned.                         */
  
  fseek(refCon->f, (int )offset, SEEK_SET);
  err = ferror(refCon->f);                              /* check for error...           */
  if (err != 0) {
    CMError(refCon->sessionData, "Error trying to set write position of file \"^0\" for container \"^1\"\n    ^2",
            refCon->pathname, CMReturnContainerName(CMGetValueContainer(value)), strerror(err));
    return (0);
  }
  
  amountRead = (CMSize)fread((char *)buffer, 1, 
                             (size_t)((maxSize <= fileSize - offset) ? maxSize : (fileSize - offset)),
                             refCon->f);
          
  return (amountRead);
}


/*-------------------------------------------------------------------*
 | writeValueData_Handler - CMWriteValueData() dynamic value handler |
 *-------------------------------------------------------------------*
 
 The buffer is written to the file and defined as the data for the indirect value. Note,
 this OVERWRITES data in the file, possibly extending the file.  It is, however, an error
 to try to create a "hole" in the file by writing beyond the end.
*/

static void writeValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize size)
{
  FileRefConPtr refCon   = (FileRefConPtr)CMGetValueRefCon(value);
  CMSize        fileSize = getValueSize_Handler(value);
  int           err;

  /* First do the error check to see if caller is trying to write beyond the current    */
  /* end of file.                                                                       */
  
  if (offset > fileSize) {
    CMError(refCon->sessionData, "Attempt to write beyond eof in file \"^0\" for container \"^1\"",
            refCon->pathname, CMReturnContainerName(CMGetValueContainer(value)));
    return;
  }
  
  /* Now all we have to do is simply write the data to the file...                      */
  
  if (size > 0) {                                       /* must have some data to write */
    fseek(refCon->f, (int )offset, SEEK_SET);
    err = ferror(refCon->f);                            /* check for error...           */
    if (err != 0) {
      CMError(refCon->sessionData, "Error trying to set write position of file \"^0\" for container \"^1\"\n    ^2",
              refCon->pathname, CMReturnContainerName(CMGetValueContainer(value)), strerror(err));
      return;
    }
    
    fwrite((char *)buffer, 1, (size_t)size, refCon->f);
  }
}


/*---------------------------------------------------------------------*
 | insertValueData_Handler - CMInsertValueData() dynamic value handler |
 *---------------------------------------------------------------------*
 
 If supported, this routine inserts size bytes from the buffer into the file at the
 specified offset.  However, in this example, for indirection to a file of data, we simply
 report an error that this functionality is not supported.  To do so is potentially
 expensive in data I/O.
 
 Remember, this is only an example.  We (I) don't want to distract from the intent of this
 file of how to support indirection with dynamic values.
*/

static void insertValueData_Handler(CMValue value, CMPtr buffer, CMCount offset, CMSize size)
{
  FileRefConPtr refCon = (FileRefConPtr)CMGetValueRefCon(value);
  CMCount       unused1 = offset;
  CMSize        unused2 = size;
  CMPtr         unused3 = buffer;

  CMError(refCon->sessionData, "Insertions into a file \"^0\" for container \"^1\" are not supported",
          refCon->pathname, CMReturnContainerName(CMGetValueContainer(value)));
}


/*---------------------------------------------------------------------*
 | deleteValueData_Handler - CMDeleteValueData() dynamic value handler |
 *---------------------------------------------------------------------*
 
 If supported, deletes size bytes from the file starting at the offset. However, in this
 example, for indirection to a file of data, we simply report an error that this
 functionality is not supported.  Standard stream C I/O is used to implement these handlers
 for portability.  There is no (legal and portable) way to delete data from a stream file.
*/

static void deleteValueData_Handler(CMValue value, CMCount offset, CMSize size)
{
  FileRefConPtr refCon = (FileRefConPtr)CMGetValueRefCon(value);
  CMCount       unused1 = offset;
  CMSize        unused2 = size;

  CMError(refCon->sessionData, "Deletions of data in a file \"^0\" for container \"^1\" are not supported",
          refCon->pathname, CMReturnContainerName(CMGetValueContainer(value)));
}


/*---------------------------------------------------------------*
 | getValueInfo_Handler - CMGetValueInfo() dynamic value handler |
 *---------------------------------------------------------------*

 The specified information for the refNum associated with a indirect file value is
 returned.  A parameter may be NULL indicating that info is not needed.
 
 We always want the info for the dynamic value's base value.  This means we would simply
 do a CMGetBaseValue() and pass the resulting value to CMGetValueInfo().  If that's all we
 need to do we can let the Container Manager do it!  Its automatic handler inheritance
 will do it.
 
 Based on this fact, we show the function code (all one line of it).  The
 fileValueMetahandler() returned NULL for this handler indicating it isn't used for this
 dynamic value.
*/

static void getValueInfo_Handler(CMValue value, CMContainer *container, CMObject *object,
                                 CMProperty *property, CMType *type,
                                 CMGeneration *generation)
{ 
  CMGetValueInfo(CMGetBaseValue(value), container, object, property, type, generation);
}


/*---------------------------------------------------------------*
 | setValueType_Handler - CMSetValueType() dynamic value handler |
 *---------------------------------------------------------------*
 
 The type ID from the type is set for the specified value.  
 
 See comments in getValueInfo_Handler() above for the reason this code is not used.
*/

static void setValueType_Handler(CMValue value, CMType type)
{
  CMSetValueType(CMGetBaseValue(value), type);
}


/*---------------------------------------------------------------------------*
 | setValueGeneration_Handler - CMSetValueGeneration() dynamic value handler |
 *---------------------------------------------------------------------------*
 
 The generation for the specified subvalue is set.  The generation number must be greater
 than or equal to 1.
 
 See comments in getValueInfo_Handler() above for the reason this code is not used.
*/

static void setValueGeneration_Handler(CMValue value, CMGeneration generation)
{
  CMSetValueGeneration(CMGetBaseValue(value), generation);
}


/*---------------------------------------------------------------*
 | releaseValue_Handler - CMReleaseValue() dynamic value handler |
 *---------------------------------------------------------------*
 
 The association between the refNum and the indirect file dynamic value is destroyed.  
 There should be no further operations on the value once this routine is called.  This
 handler is only called if this is the LAST use of the dynamic value.
 
 A count is kept by the Container Manager of every CMUseValue() and CMNewValue().  Calling 
 CMReleaseValue() reduces this count by one.  When the last release is done on the dynamic
 value, this handler will be called.
 
 Note, in the context of these file values, the only thing to do here is close the file,
 and free the refCon memory.
 
 Warning!  It is an error for the release handler to release its passed base value. It can
 free any other values, just not its base.  The reason is that the Container Manager is
 responsible for calling the release handlers for all base types.
*/

static void releaseValue_Handler(CMValue value)
{
  FileRefConPtr refCon = (FileRefConPtr)CMGetValueRefCon(value);
  CMSession     sessionData = refCon->sessionData;
  
  fclose(refCon->f);                            /* close the file                       */
  CMFree(refCon, sessionData);                  /* free our refCon                      */
}
                              
                              CM_END_CFUNCTIONS
