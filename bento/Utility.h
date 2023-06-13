/*---------------------------------------------------------------------------*
 |                                                                           |
 |                         <<<     utility.h     >>>                         |
 |                                                                           |
 |        Container Manager Miscellaneous Utility Routines Interfaces        |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 12/02/91                                  |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1991-1992                 |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*

 This file containes a collection of generally usefull utility routines that don't
 logically fit anywhere else.
*/

#ifndef __UTILITYROUTINES__
#define __UTILITYROUTINES__


#ifndef __CMTYPES__
#include "CMTypes.h"
#endif
#ifndef __HANDLERS__
#include "Handlers.h"
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

                                  CM_CFUNCTIONS


char *cmltostr(int  n, short width, Boolean hexConversion, char *s);
  /*
  Converts the (int ) integer n to a string. IF width>0 and width > strlen(string), then
  the number will be right-justified in a string of width characters.  If width<0 and 
  abs(width) > length(string), then the number is again right-justified but padded with
  leading zeros instead of blanks.  In this case a negative number will have a minus as
  its first character.  If width is too small the entire number is returned overflowing
  the width.
  
  If hex conversion is desired, set the hexConversion parameter to true.
  
  The function returns the specified string parameter as its value. That string is assumed
  large enough to hold the number.
  
  Note, this routine is provided as a substitute for sprintf().  It is more compact,
  faster, and doesn't "drag" in a shit-load of I/O library routines with it.
  */
  
CM_EXPORT char *cml64tostr(omfInt64 n, short width, Boolean hexConversion, char *s);
  
                              CM_END_CFUNCTIONS
#endif
