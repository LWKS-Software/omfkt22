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
 * ERCvt - This unittest tests that the editrate conversion functions
 *         work properly on different editrates.  It does not open or
 *         create an OMF file.
 ********************************************************************/

#include "masterhd.h"
#include <stdio.h>
#include <string.h>

#include "omDefs.h" 
#include "omTypes.h"
#include "omUtils.h"
#include "omFile.h"
#include "omMobBld.h"
#include "omMobGet.h"
#include "omCvt.h"
#include "UnitTest.h"

omfErr_t printConversion(
		omfRational_t srcEditRate, 
		omfRational_t destEditRate,
                omfPosition_t srcPosition,
		omfRounding_t round);

/* Please excuse the lack of error handling code! */
#ifdef MAKE_TEST_HARNESS
int ERCvt()
{
#else
int main(int argc, char *argv[])
{
#endif
  omfRational_t srcEditRate, destEditRate;
  omfHdl_t fileHdl = NULL;
  omfPosition_t	srcPos23;
  omfPosition_t	srcPos2345;
  
  XPROTECT(NULL)
    {
      omfsCvtInt32toPosition(23, srcPos23);
      omfsCvtInt32toPosition(2345, srcPos2345);

      srcEditRate.numerator = 2997;
      srcEditRate.denominator = 100;
      destEditRate.numerator = 44100;
      destEditRate.denominator = 1;
      CHECK(printConversion(srcEditRate, destEditRate, srcPos23, 
			    kRoundCeiling));

      srcEditRate.numerator = 2997;
      srcEditRate.denominator = 100;
      destEditRate.numerator = 44100;
      destEditRate.denominator = 1;
      CHECK(printConversion(srcEditRate, destEditRate, srcPos23, kRoundFloor));

      srcEditRate.numerator = 25;
      srcEditRate.denominator = 1;
      destEditRate.numerator = 48000;
      destEditRate.denominator = 1;
      CHECK(printConversion(srcEditRate, destEditRate, srcPos2345, 
			    kRoundCeiling));

      srcEditRate.numerator = 25;
      srcEditRate.denominator = 1;
      destEditRate.numerator = 48000;
      destEditRate.denominator = 1;
      CHECK(printConversion(srcEditRate, destEditRate, srcPos2345, 
			    kRoundFloor));

      srcEditRate.numerator = 44100;
      srcEditRate.denominator = 1;
      destEditRate.numerator = 30;
      destEditRate.denominator = 1;
      CHECK(printConversion(srcEditRate, destEditRate, srcPos23, 
			    kRoundCeiling));

      srcEditRate.numerator = 44100;
      srcEditRate.denominator = 1;
      destEditRate.numerator = 30;
      destEditRate.denominator = 1;
      CHECK(printConversion(srcEditRate, destEditRate, srcPos23, kRoundFloor));

      srcEditRate.numerator = 48000;
      srcEditRate.denominator = 1;
      destEditRate.numerator = 25;
      destEditRate.denominator = 1;
      CHECK(printConversion(srcEditRate, destEditRate, srcPos2345, 
			    kRoundCeiling));

      srcEditRate.numerator = 48000;
      srcEditRate.denominator = 1;
      destEditRate.numerator = 25;
      destEditRate.denominator = 1;
      CHECK(printConversion(srcEditRate, destEditRate, srcPos2345, 
			    kRoundFloor));

      printf("ERCvt completed successfully.\n");
    }

  XEXCEPT
    {
      printf("***ERROR: %d: %s\n", XCODE(), omfsGetErrorString(XCODE()));
      return(-1);
    }
  XEND;
  return(0);
}

omfErr_t printConversion(
			 omfRational_t srcEditRate, 
			 omfRational_t destEditRate,
			 omfPosition_t srcPosition,
			 omfRounding_t round)
{
  omfPosition_t destPosition;
  omfErr_t omfError;
  char		srcPosBuf[32], destPosBuf[32];

  omfsInt64ToString(srcPosition, 10, sizeof(srcPosBuf), srcPosBuf);  
  omfError = omfiConvertEditRate(srcEditRate, srcPosition,
				 destEditRate, round, &destPosition);
  omfsInt64ToString(destPosition, 10, sizeof(destPosBuf), destPosBuf);  
  printf("Source Edit Rate: %d.%d, Source Position: %s, Round: %d\n", 
        srcEditRate.numerator, srcEditRate.denominator, srcPosBuf, round);
  printf("Dest Edit Rate: %d.%d, Dest Position: %d, round %d\n", 
	 destEditRate.numerator, destEditRate.denominator, destPosBuf, 
	 round);

  return(omfError);
}


