/*---------------------------------------------------------------------------*
 |                                                                           |
 |                             <<< cmtypes.h >>>                             |
 |                                                                           |
 |                    Container Manager Private "Types.h"                    |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 11/18/91                                  |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1991-1992                 |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file is the "moral equivalent" to Apple's Types.h, stripped down to the basic
 definitions needed by the Container Manager.  All internal Container Manager code files
 include this header.  Because of that, those files do not include the other environmental
 and configuration headers because we do that here for them.  Thus this file is to be
 include for all basic definitions.
*/

#ifndef __CMTYPES__
#define __CMTYPES__

#ifndef __CM_API_ENV__
#include "CMAPIEnv.h"          
#endif
#ifndef __CMCONFIG__
#include "CMConfig.h"
#endif

#ifndef __DEBUG__
#define __DEBUG__ 0
#endif


/*---------------------*
 | Mac "Types.h" Stuff |
 *---------------------*
 
 These are some basic types we use internally in the Container Manager:
*/
#ifdef PORTKEY_OS_MACSYS
#if !LW_MAC
#include <CarbonCore/MacTypes.h>
#endif
#endif

/* ...switch from "__TYPES__" to "__MACTYPES__"; updating for univeral headers --wrj 12/3/98 */
 /* allows me to test with Macintosh MPW   */                           
#if !defined(__TYPES__) && !defined(__MACTYPES__) /* JeffB Added old define too for pre CWPro compilers */

#if !PORTKEY_NATIVE_BOOL

#if TARGET_OS_WIN32

#if !PORT_LANG_CPLUSPLUS
enum {
	false						= 0,
	true						= 1
};
#endif

#else
#if _MSC_VER < 1700
#ifndef true
	#define true			1
#endif
#ifndef false
	#define false			0
#endif
#endif
#endif

#endif  /*  !TYPE_BOOL */

typedef unsigned char 					Boolean;

#endif


/*-----------------------*
 | Debugging Definitions |
 *-----------------------*
 
 For Mac debugging, the low level Mac debugger, "MacsBug" is used.  It's definitions are
 defined under a __DEBUG__ compile-time switch. 
*/

#if __DEBUG__
#if !defined(__TYPES__) && !defined(__MACTYPES__)

                              CM_CFUNCTIONS
                              
pascal void Debugger(void) = 0xA9FF; 
pascal void DebugStr(const unsigned char * aStr) = 0xABFF; 

                            CM_END_CFUNCTIONS

#endif /* __TYPES__ or __MACTYPES__ */
#endif /* __DEBUG__ */

#endif /* __CMTYPES__ */
