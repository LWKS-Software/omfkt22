/*---------------------------------------------------------------------------*
 |                                                                           |
 |                            <<<  cmvers.h   >>>                            |
 |                                                                           |
 |            Container Manager Version and Copyright Definitions            |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 4/15/92                                   |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1992-1993                 |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This file sole goal in life is to define the Container Manager version and copyright
 strings.
 
 Note, the copyright string is used in   CMSesOps.c   by a routine called from
 CMStartSession().  The name of that routine is the copyright string with underscores in
 place of spaces.  This comes from a macro also defined here.
 
 The version string defines the current version of the Container Manager so we can tell
 what versions go with what versions of the specification and for bug reporting.  The
 version string is placed in the global session data.
*/

#ifndef __CMVERSION__
#define __CMVERSION__

#ifndef CMVersion
#define CMVersion "<<< 1.0d5 >>>"
#endif
  /*
  This version number should agree with the current spec so we know who goes with what!
  Currently this must be manually changed.  Although it could be mechanized and put in
  the build script, that would only satisfy the Mac MPW build.  It wouldn't help builds
  on "alien" development systems.
  
  Note, that the version is under control of a #ifndef.  This means on systems that support
  it, you can control the version number from the compiler command line (e.g., -d).  Thus
  you can control it from your build scripts (assuming you can figure out how to get the
  damn quotes in).
  */
  

#define Copyright "Copyright Apple Computer, Inc. 1991-1993. " \
                  "All rights reserved. "                      \
                  "By Ira L. Ruben."
                  
#define Copyright_Apple_Computer Copyright_Apple_Computer_Inc_1991_1993_All_rights_reserved
  /*
  This is the copyright notice needed to make Apple lawers happy.  The first macro is used
  in a call to a routine named by the second macro.  That will have the effect of embedding
  the string into the code (as data and a routine name) and keeping it from being dead code
  stripped (at least on linkers smart enough to do such things).  You sould always keep 
  both strings in parallel.
  */

#endif
