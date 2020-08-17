#pragma once
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


/* Palm OS common definitions */
#include <SystemMgr.h>


/* If we're actually compiling the library code, then we need to
 * eliminate the trap glue that would otherwise be generated from
 * this header file in order to prevent compiler errors in CW Pro 2. */
#ifdef BUILDING_ADDRESSXTLIB2
	#define ADDRESSXTLIB2_LIB_TRAP(trapNum)
#else
	#define ADDRESSXTLIB2_LIB_TRAP(trapNum) SYS_TRAP(trapNum)
#endif

/*********************************************************************
 * Type and creator of Sample Library database
 *********************************************************************/

#define		AddressXTLib2CreatorID	'adXT'
#define		AddressXTLib2TypeID		'lib2'

/*********************************************************************
 * Internal library name which can be passed to SysLibFind()
 *********************************************************************/

#define		AddressXTLib2Name		"AddressXTLib2"

/*********************************************************************
 * AddressXTLib result codes
 * (appErrorClass is reserved for 3rd party apps/libraries.
 * It is defined in SystemMgr.h)
 *********************************************************************/

/* invalid parameter */
#define AddressXTLib2ErrParam		(appErrorClass | 1)		

/* library is not open */
#define AddressXTLib2ErrNotOpen		(appErrorClass | 2)		

/* returned from AddressXTLibClose() if the library is still open */
#define AddressXTLib2ErrStillOpen	(appErrorClass | 3)		

/*********************************************************************
 * API Prototypes
 *********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Standard library open, close, sleep and wake functions */

extern Err AddressXTLib2Open(UInt16 refNum, UInt32 * clientContextP)
	ADDRESSXTLIB2_LIB_TRAP(sysLibTrapOpen);
				
extern Err AddressXTLib2Close(UInt16 refNum, UInt32 clientContext)
	ADDRESSXTLIB2_LIB_TRAP(sysLibTrapClose);

extern Err AddressXTLib2Sleep(UInt16 refNum)
	ADDRESSXTLIB2_LIB_TRAP(sysLibTrapSleep);

extern Err AddressXTLib2Wake(UInt16 refNum)
	ADDRESSXTLIB2_LIB_TRAP(sysLibTrapWake);

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
 
__inline Err AddressXTLib2_OpenLibrary(UInt16 *refNumP, UInt32 * clientContextP)
{
	Err error;
	Boolean loaded = false;
	
	/* first try to find the library */
	error = SysLibFind(AddressXTLib2Name, refNumP);
	
	/* If not found, load the library instead */
	if (error == sysErrLibNotFound)
	{
		error = SysLibLoad(AddressXTLib2TypeID, AddressXTLib2CreatorID, refNumP);
		loaded = true;
	}
	
	if (error == errNone)
	{
		error = AddressXTLib2Open(*refNumP, clientContextP);
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

__inline Err AddressXTLib2_CloseLibrary(UInt16 refNum, UInt32 clientContext)
{
	Err error;
	
	if (refNum == sysInvalidRefNum)
	{
		return sysErrParamErr;
	}

	error = AddressXTLib2Close(refNum, clientContext);

	if (error == errNone)
	{
		/* no users left, so unload library */
		SysLibRemove(refNum);
	} 
	else if (error == AddressXTLib2ErrStillOpen)
	{
		/* don't unload library, but mask "still open" from caller  */
		error = errNone;
	}
	
	return error;
}

typedef enum {
	//Reg.c
	adxtLib2TrapCheckRegistration	= sysLibTrapCustom//,
	//sampleLibTrapLast

	} adxtLib2TrapNumberEnum;
