/*
 * AddressXTLib.h
 *
 * public header for shared library
 *
 * This wizard-generated code is based on code adapted from the
 * SampleLib project distributed as part of the Palm OS SDK 4.0,
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */

#pragma once

/* Palm OS common definitions */
#include <SystemMgr.h>


/* If we're actually compiling the library code, then we need to
 * eliminate the trap glue that would otherwise be generated from
 * this header file in order to prevent compiler errors in CW Pro 2. */
#ifdef BUILDING_ADDRESSXTLIB
	#define ADDRESSXTLIB_LIB_TRAP(trapNum)
#else
	#define ADDRESSXTLIB_LIB_TRAP(trapNum) SYS_TRAP(trapNum)
#endif

/*********************************************************************
 * Type and creator of Sample Library database
 *********************************************************************/

#define		AddressXTLibCreatorID	'adXT'
#define		AddressXTLibTypeID		'lib1'

/*********************************************************************
 * Internal library name which can be passed to SysLibFind()
 *********************************************************************/

#define		AddressXTLibName		"AddressXTLib"

/*********************************************************************
 * AddressXTLib result codes
 * (appErrorClass is reserved for 3rd party apps/libraries.
 * It is defined in SystemMgr.h)
 *********************************************************************/

/* invalid parameter */
#define AddressXTLibErrParam		(appErrorClass | 1)		

/* library is not open */
#define AddressXTLibErrNotOpen		(appErrorClass | 2)		

/* returned from AddressXTLibClose() if the library is still open */
#define AddressXTLibErrStillOpen	(appErrorClass | 3)		

/*********************************************************************
 * API Prototypes
 *********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Standard library open, close, sleep and wake functions */

extern Err AddressXTLibOpen(UInt16 refNum, UInt32 * clientContextP)
	ADDRESSXTLIB_LIB_TRAP(sysLibTrapOpen);
				
extern Err AddressXTLibClose(UInt16 refNum, UInt32 clientContext)
	ADDRESSXTLIB_LIB_TRAP(sysLibTrapClose);

extern Err AddressXTLibSleep(UInt16 refNum)
	ADDRESSXTLIB_LIB_TRAP(sysLibTrapSleep);

extern Err AddressXTLibWake(UInt16 refNum)
	ADDRESSXTLIB_LIB_TRAP(sysLibTrapWake);

/* Custom library API functions */

#ifdef __cplusplus
}
#endif

/*
 * FUNCTION: AddressXTLib_OpenLibrary
 *
 * DESCRIPTION:
 *
 * User-level call to open the library.  This inline function
 * handles the messy task of finding or loading the library
 * and calling its open function, including handling cleanup
 * if the library could not be opened.
 * 
 * PARAMETERS:
 *
 * refNumP
 *		Pointer to UInt16 variable that will hold the new
 *      library reference number for use in later calls
 *
 * clientContextP
 *		pointer to variable for returning client context.  The client context is
 *		used to maintain client-specific data for multiple client support.  The 
 *		value returned here will be used as a parameter for other library 
 *		functions which require a client context.  
 *
 * CALLED BY: System
 *
 * RETURNS:
 *		errNone
 *		memErrNotEnoughSpace
 *      sysErrLibNotFound
 *      sysErrNoFreeRAM
 *      sysErrNoFreeLibSlots
 *
 * SIDE EFFECTS:
 *		*clientContextP will be set to client context on success, or zero on
 *      error.
 */
 
__inline Err AddressXTLib_OpenLibrary(UInt16 *refNumP, UInt32 * clientContextP)
{
	Err error;
	Boolean loaded = false;
	
	/* first try to find the library */
	error = SysLibFind(AddressXTLibName, refNumP);
	
	/* If not found, load the library instead */
	if (error == sysErrLibNotFound)
	{
		error = SysLibLoad(AddressXTLibTypeID, AddressXTLibCreatorID, refNumP);
		loaded = true;
	}
	
	if (error == errNone)
	{
		error = AddressXTLibOpen(*refNumP, clientContextP);
		if (error != errNone)
		{
			if (loaded)
			{
				SysLibRemove(*refNumP);
			}

			*refNumP = sysInvalidRefNum;
		}
	}
	
	return error;
}

/*
 * FUNCTION: AddressXTLib_CloseLibrary
 *
 * DESCRIPTION:	
 *
 * User-level call to closes the shared library.  This handles removal
 * of the library from system if there are no users remaining.
 *
 * PARAMETERS:
 *
 * refNum
 *		Library reference number obtained from AddressXTLib_OpenLibrary().
 *
 * clientContext
 *		client context (as returned by the open call)
 *
 * CALLED BY: Whoever wants to close the library
 *
 * RETURNS:
 *		errNone
 *		sysErrParamErr
 */

__inline Err AddressXTLib_CloseLibrary(UInt16 refNum, UInt32 clientContext)
{
	Err error;
	
	if (refNum == sysInvalidRefNum)
	{
		return sysErrParamErr;
	}

	error = AddressXTLibClose(refNum, clientContext);

	if (error == errNone)
	{
		/* no users left, so unload library */
		SysLibRemove(refNum);
	} 
	else if (error == AddressXTLibErrStillOpen)
	{
		/* don't unload library, but mask "still open" from caller  */
		error = errNone;
	}
	
	return error;
}

typedef enum {
	//AddressDB.c
	adxtLibTrapUpdateAlarms	= sysLibTrapCustom,
	adxtLibTrapReserved,	
	adxtLibTrapAddrDBAppInfoGetPtr,	
	adxtLibTrapAddrDBChangeSortOrder,
	adxtLibTrapAddrDBLookupSeekRecord,
	adxtLibTrapAddrDBLookupString,
	adxtLibTrapAddrDBLookupLookupString,
	adxtLibTrapAddrDBGetDatabase,
	adxtLibTrapPrvAddrDBFindSortPosition,
	adxtLibTrapPrvAddrDBFindKey,
	adxtLibTrapAddrDBAppInfoInit,
	adxtLibTrapAddrDBNewRecord,
	adxtLibTrapAddrDBGetRecord,
	adxtLibTrapPrvAddrDBUnpackedSize,
	adxtLibTrapPrvAddrDBPack,
	adxtLibTrapPrvAddrDBUnpack,
	adxtLibTrapAddrDBChangeCountry,
	adxtLibTrapPrvAddrDBStrCmpMatches,
	adxtLibTrapPrvAddrDBSeekVisibleRecordInCategory,
	adxtLibTrapPrvAddrDBLocalizeAppInfo,
	//addrtools.c
	adxtLibTrapTxtFindStringEx,	
	adxtLibTrapGetSortByCompany,	
	adxtLibTrapToolsSeekRecordEx,	
	adxtLibTrapdeleteRecord,
	adxtLibTrapCstOpenOrCreateDB,
	adxtLibTrapToolsWinDrawCharsHD,
	adxtLibTrapToolsSetDBAttrBits,
	adxtLibTrapToolsDetermineRecordName,
	adxtLibTrapToolsDrawRecordName,
	adxtLibTrapToolsDrawRecordNameAndPhoneNumber,
	adxtLibTrapToolsGetLabelColumnWidth,
	adxtLibTrapToolsSeekRecord,
	adxtLibTrapToolsCustomAcceptBeamDialog,
	adxtLibTrapToolsInitPhoneLabelLetters,
	adxtLibTrapToolsGetStringResource,
	adxtLibTrapToolsGetLineIndexAtOffset,
	adxtLibTrapToolsUnivWinDrawCharsHD,
	adxtLibTrapToolsEnableBluetooth,
	adxtLibTrapToolsFntSetFont,
	adxtLibTrapToolsGetSortInfo,
	adxtLibTrapFrmGlueNavObjectTakeFocus,
	adxtLibTrapFrmGlueNavDrawFocusRing,
	adxtLibTrapFrmGlueNavRemoveFocusRing,
	adxtLibTrapFrmGlueNavGetFocusRingInfo,
	adxtLibTrapToolsIsFiveWayNavPalmEvent,
	adxtLibTrapToolsIsFiveWayNavEvent,
	adxtLibTrapToolsNavSelectHSPressed,
	adxtLibTrapToolsNavSelectPalmPressed,
	adxtLibTrapToolsNavSelectPressed,
	adxtLibTrapToolsNavDirectionHSPressed,
	adxtLibTrapToolsNavDirectionPalmPressed,
	adxtLibTrapToolsNavDirectionPressed,
	adxtLibTrapToolsNavKeyPressed,
	adxtLibTrapPrvToolsPhoneIsANumber,
	adxtLibTrapGetDelimiterStr,
	adxtLibTrapGetDelimiterLen,
//addrtransfer.c
	adxtLibTrapTransferRegisterData,
	adxtLibTrapTransferReceiveData,
	adxtLibTrapTransferImportVCard,
	adxtLibTrapTransferPreview,
	adxtLibTrapPrvTransferCleanFileName,
	adxtLibTrapPrvTransferPdiLibLoad,
	adxtLibTrapPrvTransferPdiLibUnload,
	adxtLibTrapPrvTransferSetGoToParams,
//ContactsDB.c
	adxtLibTrapPrvP1ContactsDBChangeRecord,
	adxtLibTrapPrvP1ContactsDBUnpackedSize,
	adxtLibTrapPrvP1ContactsDBUnpack,
	adxtLibTrapPrvP1ContactsDBPack,
	adxtLibTrapP1ContactsDBAppInfoGetPtr,
	adxtLibTrapP1ContactsDBNewRecord,
	adxtLibTrapPrvP1ContactsDBGetRecord,
//AddrXTDB.c
	adxtLibTrapAddrXTDBGetRecord,
	adxtLibTrapAddrXTDBNewRecord,
	adxtLibTrapAddrXTDBChangeRecord,	
//AddrTools.c
	adxtLibTrapLoadColors,
//RecentDB
	adxtLibTrapAddrDBAddToRecent,
	adxtLibTrapAddrDBCountRecentInCategory,
	adxtLibTrapTrimRecentDB,
	adxtLibTrapSeekRecentInCategory,
	adxtLibTrapCleanRecentDB,
	adxtLibTrapRecentDBGetRecord,
	adxtLibTrapRecentDBNewRecord,
	adxtLibTrapRecentDBChangeRecord,
//Links
	adxtLibTrapGetLinkCount,
	adxtLibTrapFindLink,
	adxtLibTrapUpdateLink,
	adxtLibTrapAddLink,
	adxtLibTrapDeleteLink,
	adxtLibTrapDeleteOrphaned,
	adxtLibTrapDeleteAllLinks,
	adxtLibTrapGetLinkByIndex,
	adxtLibTrapGoToLink,
	adxtLibTrapLinkDBChangeRecord,
	adxtLibTrapGetLinksLocalID,
	adxtLibTrapGetAllLinkCount,
//AddrTools
	adxtLibTrapPrvDetermineRecordNameHelper,
	
	sampleLibTrapLast

	} adxtLibTrapNumberEnum;