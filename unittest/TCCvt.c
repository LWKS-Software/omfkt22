/***********************************************************************
 *
 *              Copyright (c) 1996 Avid Technology, Inc.
 *
 * Permission to use, copy and modify this software and to distribute
 * and sublicense application software incorporating this software for
 * any purpose is hereby granted, provided that (i) the above
 * copyright notice and this permission notice appear in all copies of
 * the software and related documentation, and (ii) the name Avid
 * Technology, Inc. may not be used in any advertising or publicity
 * relating to the software without the specific, prior written
 * permission of Avid Technology, Inc.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL AVID TECHNOLOGY, INC. BE LIABLE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT, CONSEQUENTIAL OR OTHER DAMAGES OF
 * ANY KIND, OR ANY DAMAGES WHATSOEVER ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE, INCLUDING, 
 * WITHOUT  LIMITATION, DAMAGES RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, AND WHETHER OR NOT ADVISED OF THE POSSIBILITY OF
 * DAMAGE, REGARDLESS OF THE THEORY OF LIABILITY.
 *
 ************************************************************************/
/********************************************************************
 * TCCvt - This unittest tests the timecode conversion functionality.
 *         There is no file opened or created for this unittest.
 ********************************************************************/

#include "masterhd.h"
#include <stdio.h>
#include <string.h>

#include "omPublic.h"

#include "UnitTest.h"

omfErr_t printTimecode(char *tcString, 
		   omfRational_t frameRate);

/* Please excuse the lack of error handling code! */
#ifdef MAKE_TEST_HARNESS
int TCCvt()
{
#else
int main(int argc, char *argv[])
{
#endif
  char tcString[12];
  omfRational_t frameRate;

  XPROTECT(NULL)
    {

      strncpy(tcString, "01:24:36:05", 12);

      /* Testing with 30 fps, nondrop */
      printf("*** Testing Timecode with 30 fps\n");
      frameRate.numerator = 30;
      frameRate.denominator = 1;
      CHECK(printTimecode(tcString, frameRate));

      /* Testing with 25 fps, nondrop */
      printf("\n*** Testing Timecode with 25 fps\n");
      frameRate.numerator = 25;
      frameRate.denominator = 1;
      CHECK(printTimecode(tcString, frameRate));

      strncpy(tcString, "01;24;36;05", 12);

      /* Testing with 30 fps, drop */
      printf("\n*** Testing Timecode with 30 fps\n");
      frameRate.numerator = 30;
      frameRate.denominator = 1;
      CHECK(printTimecode(tcString, frameRate));

      /* Testing with 29.97 fps, drop */
      printf("\n*** Testing Timecode with 29.97 fps\n");
      frameRate.numerator = 2997;
      frameRate.denominator = 100;
      CHECK(printTimecode(tcString, frameRate));

      printf("TCCvt completed successfully.\n");
    }

  XEXCEPT
    {
      printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
      return(-1);
    }
  XEND;
  return(0);
    
}

omfErr_t printTimecode(char *tcString, 
		   omfRational_t frameRate)
{
  omfTimecode_t timecode;

  XPROTECT(NULL)
    {
      memset(&timecode, 0, sizeof(timecode));
      CHECK(omfsStringToTimecode(tcString, frameRate, &timecode));
      printf("Timecode: %s is FPS %d, Drop %d, Start %d\n", tcString, 
	     timecode.fps, timecode.drop, timecode.startFrame);

      CHECK(omfsTimecodeToString(timecode, 12, tcString));
      printf("Timecode: %s is FPS %d, Drop %d, Start %d\n", tcString, 
	     timecode.fps, timecode.drop, timecode.startFrame);
    }

  XEXCEPT
    {
      printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
      return(XCODE());
    }
  XEND;

  return(OM_ERR_NONE);
}
