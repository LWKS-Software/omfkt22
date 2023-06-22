/***************************************************************************
 * InfoSupp.c: Contains functions shared between omfInfo and omfDump.      *
 ***************************************************************************/

#include "masterhd.h"
#if PORT_SYS_MAC
#include <Types.h>
#include <OSUtils.h>
#endif
#include <stdio.h>
#include <string.h>
#include "omPublic.h"
#include "InfoSupp.h"

/**************
 * PROTOTYPES *
 **************/

static void PrintProductVersion(omfProductVersion_t *version);
static void PrintToolkitVersion(omfProductVersion_t *version);

/********************/
/* GLOBAL VARIABLES */
/********************/

/*******************
 * VERSION HISTORY *
 *******************/

/****************
 * MAIN PROGRAM *
 ****************/

void omfPrintAuditTrail(omfHdl_t fileHdl)
{
    omfInt32			n, numIdent;
    omfObject_t			identObj, head;
	char				text[256];
	omfProductVersion_t	productVersion;
	omfInt32			productID;
	omfTimeStamp_t		timestamp;
	omfInt16			byteOrder;

    XPROTECT(fileHdl)
    {
	    /* Print out any IDNT objects found */
		omfsGetHeadObject(fileHdl, &head);
		numIdent = omfsLengthObjRefArray(fileHdl, head, OMHEADIdentList);
		for(n = 1; n <= numIdent; n++)
		{
			if(n == 1)
				printf("Created By:\n");
			else
				printf("Modified By (%ld):\n", n-1);
				
			CHECK(omfsGetNthObjRefArray(fileHdl, head, OMHEADIdentList,
										&identObj, n));
			/***/
			if(omfsReadString(fileHdl, identObj, OMIDNTCompanyName, text, sizeof(text)) != OM_ERR_NONE)
				strcpy( (char*) text, "<Not Specified>");
			printf("    Company Name:      %s\n", text);
			/***/
			if(omfsReadString(fileHdl, identObj, OMIDNTProductName, text, sizeof(text)) != OM_ERR_NONE)
				strcpy( (char*) text, "<Not Specified>");
			printf("    Product Name:      %s\n", text);
			/***/
			if(omfsReadProductVersionType(fileHdl, identObj, OMIDNTProductVersion,
											&productVersion) == OM_ERR_NONE)
					PrintProductVersion(&productVersion);
			else
				printf("    Product Version:   <not specified>\n");
			/***/
			if(omfsReadString(fileHdl, identObj, OMIDNTProductVersionString, text,
								sizeof(text)) == OM_ERR_NONE)
			{
				printf("                       (%s)\n", text);
			}
			/***/
			if(omfsReadString(fileHdl, identObj, OMIDNTPlatform, text, sizeof(text)) != OM_ERR_NONE)
				strcpy( (char*) text, "<Not Specified>");
			printf("    Platform:          %s\n", text);
			/***/
			if(omfsReadInt16(fileHdl, identObj, OMIDNTByteOrder, &byteOrder) == OM_ERR_NONE)
			{
				if(byteOrder == MOTOROLA_ORDER)
					strcpy( (char*) text, "Big-Endian");
				else if(byteOrder == INTEL_ORDER)
					strcpy( (char*) text, "Little-Endian");
				else 
					strcpy( (char*) text, "<illegal>");
			}
			else
				strcpy( (char*) text, "<Not Specified>");
			printf("    Byte Order:        %s\n", text);
			
			
			/***/
			CHECK(omfsReadInt32(fileHdl, identObj, OMIDNTProductID, &productID));
			printf("	Product ID:        %ld\n", productID);
			/***/
			if(omfsReadTimeStamp(fileHdl, identObj, OMIDNTDate, &timestamp) == OM_ERR_NONE)
			{
	#if PORT_SYS_MAC
				{
					DateTimeRec	time;
				
					SecondsToDate(timestamp.TimeVal, &time);
					printf("	Modified on:       %d/%d/%d at %d:%02d\n",
							time.year, time.month,
							time.day, time.hour, time.minute);
				}
	#else
			printf("	ModifyTimestamp: %ld\n", timestamp.TimeVal);
	#endif
			}
			else
				printf("	ModifyTimestamp:   <unknown>\n");

			/***/
			if(omfsReadProductVersionType(fileHdl, identObj, OMIDNTToolkitVersion,
											&productVersion) == OM_ERR_NONE)
				PrintToolkitVersion(&productVersion);
			if(n != numIdent)
				printf("    ----------------\n");
		}
		printf("\n");
	}
    XEXCEPT
    XEND_VOID;
}

static void PrintProductVersion(omfProductVersion_t *version)
{
	if(version->type == kVersionUnknown)
	{
		return;
	}
	
	printf("    Product Version:   (%d, %d, %d)    ",
			version->major, version->minor, version->tertiary);
	switch(version->type)
	{
		case kVersionReleased:
			printf("Released\n");
			break;

		case kVersionDebug:
			printf("Pre-Release Debug Version %d\n",
					version->patchLevel);
			break;

		case kVersionPatched:
			printf("Patched Version (to patch level %d)\n",
					version->patchLevel);
			break;

		case kVersionBeta:
			printf("Pre-Release Version %d\n",
					version->patchLevel);
			break;

		case kVersionPrivateBuild:
			printf("Private Build %d\n",
					version->patchLevel);
			break;

		default:
			break;
	}
}

static void PrintToolkitVersion(omfProductVersion_t *version)
{
	if(version->type == kVersionUnknown)
	{
		return;
	}
	
	if(version->tertiary == 0)
	{
		printf("    Toolkit Version:   %d.%d",
			version->major, version->minor);
	}
	else
	{
		printf("    Toolkit Version:   %d.%d.%d",
			version->major, version->minor, version->tertiary);
	}
	switch(version->type)
	{
		case kVersionReleased:
			printf(" (Released)\n");
			break;

		case kVersionDebug:
			printf("d%d\n", version->patchLevel);
			break;

		case kVersionPatched:
			printf("Patched Version (to patch level %d)\n",
					version->patchLevel);
			break;

		case kVersionBeta:
			printf("b%d\n", version->patchLevel);
			break;

		case kVersionPrivateBuild:
			printf("Private Build %d\n",
					version->patchLevel);
			break;

		default:
			break;
	}
}
