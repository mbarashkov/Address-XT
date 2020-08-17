/*
 * AddressXTLibImpl.c
 *
 * implementation for shared library
 *
 * This wizard-generated code is based on code adapted from the
 * SampleLib project distributed as part of the Palm OS SDK 4.0.
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */

/* Our library public definitions (library API) */
#define BUILDING_ADDRESSXTLIB2
#include "AddressXTLib.h"
#include "AddressXTLibPrivate.h"

/*********************************************************************
 *
 * LIBRARY GLOBALS:
 *
 * IMPORTANT:
 * ==========
 * Libraries are *not* allowed to have global or static variables.
 * Instead, they allocate a memory chunk to hold their persistent data, 
 * and save a handle to it in the library's system library table entry.
 * Example functions below demostrate how the library "globals" chunk 
 * is set up, saved, and accessed.
 *
 * We use a movable memory chunk for our library globals to minimize
 * dynamic heap fragmentation.  Our library globals are locked only
 * when needed and unlocked as soon as possible.  This behavior is
 * critical for healthy system performance.
 *
 *********************************************************************/


/*********************************************************************
 * Internally used routines
 *********************************************************************/

static AddressXTLibGlobalsType * PrvMakeGlobals(UInt16 refNum);
static void PrvFreeGlobals(UInt16 refNum);
static AddressXTLibGlobalsType * PrvLockGlobals(UInt16 refNum);
static Boolean PrvIsLibOpen(UInt16 refNum);
static Err PrvCreateClientContext(AddressXTLibGlobalsType * gP, UInt32 * clientContextP);
static Err PrvDestroyClientContext(AddressXTLibGlobalsType * gP, UInt32 clientContext);
static AddressXTLibClientContextType * PrvLockContext(UInt32 context);

/*********************************************************************
 * Internally used macros
 *********************************************************************/

/* Unlock globals */
#define PrvUnlockGlobals(gP)        MemPtrUnlock(gP)

/* Unlock the client context */
#define PrvUnlockContext(contextP)	MemPtrUnlock(contextP)
 
/*********************************************************************
 * Library API Routines
 *********************************************************************/
 
/*
 * FUNCTION: AddressXTLibOpen
 *
 * DESCRIPTION:
 *
 * Opens the library, creates and initializes the globals.
 * This function must be called before any other library functions.
 *
 * If AddressXTLibOpen fails, do not call any other library API functions.
 * If AddressXTLibOpen succeeds, call AddressXTLibClose when you are done using
 * the library to enable it to release critical system resources.
 *
 * LIBRARY DEVELOPER NOTES:
 *
 * The library's "open" and "close" functions should *not* take an excessive
 * amount of time to complete.  If the processing time for either of these
 * is lengthy, consider creating additional library API function(s) to handle
 * the time-consuming chores.
 *
 *
 * PARAMETERS:
 *
 * refNum
 *		Library reference number returned by SysLibLoad() or SysLibFind().
 *
 * clientContextP
 *		pointer to variable for returning client context.  The client context is
 *		used to maintain client-specific data for multiple client support.  The 
 *		value returned here will be used as a parameter for other library 
 *		functions which require a client context.  
 *
 * CALLED BY: anyone who wants to use this library
 *
 * RETURNS:
 *		errNone
 *		memErrNotEnoughSpace
 *
 * SIDE EFFECTS:
 *		*clientContextP will be set to client context on success, or zero on error.
 */

Err AddressXTLib2Open(UInt16 refNum, UInt32 *clientContextP)
{
	AddressXTLibGlobalsType *gP;
	Err err = errNone;
	Int16 originalOpenCount = 0;

	/* Error-check our parameters */
	ErrFatalDisplayIf(
		clientContextP == NULL, 
		"null context variable pointer");

	/* Initialize return variable */
	*clientContextP = 0;

	/* Get library globals */
	gP = PrvLockGlobals(refNum);

	/* Check if already open */
	if (!gP)
	{
		/* Allocate and initialize our library globals. */
		gP = PrvMakeGlobals(refNum);
		if ( !gP )
			err = memErrNotEnoughSpace;
	}

	/* If we have globals, create a client context, increment open
	 * count, and unlock our globals */
	if ( gP )
	{
		originalOpenCount = gP->openCount;

		err = PrvCreateClientContext(gP, clientContextP);
		if ( !err )
			gP->openCount++;

		PrvUnlockGlobals(gP);

		/* If there was an error creating a client context and there  */
		/* are no other clients, free our globals */
		if ( err && (originalOpenCount == 0) )
			PrvFreeGlobals(refNum);
	}

	return( err );
}

/*
 * FUNCTION: AddressXTLibClose
 *
 * DESCRIPTION:	
 *
 * Closes the library, frees client context and globals.
 *
 * ***IMPORTANT***
 * May be called only if AddressXTLibOpen succeeded.
 *
 * If other applications still have the library open, decrements
 * the reference count and returns AddressXTLibErrStillOpen.
 *
 * LIBRARY DEVELOPER NOTES:
 *
 * The library's "open" and "close" functions should *not* take an excessive
 * amount of time to complete.  If the processing time for either of these
 * is lengthy, consider creating additional library API function(s) to handle
 * the time-consuming chores.
 *
 * PARAMETERS:
 *
 * refNum
 *		Library reference number returned by SysLibLoad() or SysLibFind().
 *
 * clientContext
 *		client context
 *
 * CALLED BY: Whoever wants to close the library
 *
 * RETURNS:
 *		errNone
 *		AddressXTLibErrStillOpen -- library is still open by others (no error)
 */

Err AddressXTLib2Close(UInt16 refNum, UInt32 clientContext)
{
	AddressXTLibGlobalsType * gP;
	Int16 openCount;
	Int16 contextCount;
	Err err = errNone;

	gP = PrvLockGlobals(refNum);

	/* If not open, return */
	if (!gP)
	{
		/* MUST return zero here to get around a bug in system v1.x that
		 * would cause SysLibRemove to fail. */
		return errNone;
	}

	/* Destroy the client context (we ignore the return code in this implementation) */
	PrvDestroyClientContext(gP, clientContext);

	/* Decrement our library open count */
	gP->openCount--;

	/* Error check for open count underflow */
	ErrFatalDisplayIf(gP->openCount < 0, "AddressXTLib open count underflow");

	/* Save the new open count and the context count */
	openCount = gP->openCount;
	contextCount = gP->contextCount;

	PrvUnlockGlobals(gP);

	/* If open count reached zero, free our library globals */
	if ( openCount <= 0 )
	{
		/* Error check to make sure that all client contexts were destroyed */
		ErrFatalDisplayIf(contextCount != 0, "not all client contexts were destroyed");

		/* Free our library globals */
		PrvFreeGlobals(refNum);
	}
	else
	{
		/* return this error code to inform the caller
		 * that others are still using this library */
		err = AddressXTLib2ErrStillOpen;
	}

	return err;
}

/*
 * FUNCTION: AddressXTLibSleep
 *
 * DESCRIPTION:
 *
 * Handles system sleep notification.
 *
 * ***IMPORTANT***
 * This notification function is called from a system interrupt.
 * It is only allowed to use system services which are interrupt-
 * safe.  Presently, this is limited to EvtEnqueueKey, SysDisableInts,
 * SysRestoreStatus.  Because it is called from an interrupt,
 * it must *not* take a long time to complete to preserve system
 * integrity.  The intention is to allow system-level libraries
 * to disable hardware components to conserve power while the system
 * is asleep.
 * 
 * PARAMETERS:
 *
 * refNum
 *		Library reference number returned by SysLibLoad() or SysLibFind().
 *
 * CALLED BY: System
 *
 * RETURNS:	errNone
 */

Err AddressXTLib2Sleep(UInt16 refNum)
{
	#pragma unused(refNum)
	return errNone;
}

/*
 * FUNCTION: AddressXTLibWake
 *
 * DESCRIPTION:
 *
 * Handles system wake notification.
 *
 * ***IMPORTANT***
 * This notification function is called from a system interrupt.
 * It is only allowed to use system services which are interrupt-
 * safe.  Presently, this is limited to EvtEnqueueKey, SysDisableInts,
 * SysRestoreStatus.  Because it is called from an interrupt,
 * it must *not* take a long time to complete to preserve system
 * integrity.  The intention is to allow system-level libraries
 * to enable hardware components which were disabled when the system
 * went to sleep.
 * 
 * PARAMETERS:
 *
 * refNum
 *		Library reference number returned by SysLibLoad() or SysLibFind().
 *
 * CALLED BY: System
 *
 * RETURNS:	errNone
 */

Err AddressXTLib2Wake(UInt16 refNum)
{
	#pragma unused(refNum)
	return errNone;
}

/*********************************************************************
 * Private Functions
 *********************************************************************/

/*
 * FUNCTION: PrvMakeGlobals
 *
 * DESCRIPTION: Create our library globals.
 *
 * PARAMETERS:
 *
 * refNum
 *		Library reference number returned by SysLibLoad() or SysLibFind().
 *
 * CALLED BY: internal
 *
 * RETURNS:
 *
 *		pointer to our *locked* library globals
 *		NULL if our globals	could not be created.
 */

static AddressXTLibGlobalsType * PrvMakeGlobals(UInt16 refNum)
{
	AddressXTLibGlobalsType * gP = NULL;
	MemHandle gH;
	SysLibTblEntryType * libEntryP;

	/* Get library globals */
	libEntryP = SysLibTblEntry(refNum);
	ErrFatalDisplayIf(libEntryP == NULL, "invalid AddressXTLib refNum");

	/* Error check to make sure the globals don't already exist */
	ErrFatalDisplayIf(libEntryP->globalsP, "AddressXTLib globals already exist");

	/* Allocate and initialize our library globals. */
	gH = MemHandleNew(sizeof(AddressXTLibGlobalsType));
	if ( !gH )
		return( NULL );

	/* Save the handle of our library globals in the system library table  */
	/* entry so we can later retrieve it using SysLibTblEntry(). */
	libEntryP->globalsP = (void*)gH;

	/* Lock our globals (should not fail) */
	gP = PrvLockGlobals(refNum);
	ErrFatalDisplayIf(gP == NULL, "failed to lock AddressXTLib globals");

	/* Set the owner of our globals memory chunk to "system" (zero), so it won't get
	 * freed automatically by Memory Manager when the first application to call
	 * AddressXTLibOpen exits.  This is important if the library is going to stay open
	 * between apps. */
	MemPtrSetOwner(gP, 0);

	/* Initialize our library globals */
	MemSet(gP, sizeof(AddressXTLibGlobalsType), 0);

	/* for convenience and debugging, save ref in globals structure */
	gP->thisLibRefNum = refNum;

	/* initial open count */
	gP->openCount = 0;

	/* return a pointer to our *locked* globals */
	return( gP );
}

/*
 * FUNCTION: PrvFreeGlobals
 *
 * DESCRIPTION: Free our library globals.
 *
 * PARAMETERS:
 *
 * refNum
 *		Library reference number returned by SysLibLoad() or SysLibFind().
 *
 * CALLED BY: internal
 *
 * RETURNS: nothing
 */

static void PrvFreeGlobals(UInt16 refNum)
{
	MemHandle gH;
	SysLibTblEntryType * libEntryP;

	/* Get our library globals handle */
	libEntryP = SysLibTblEntry(refNum);
	ErrFatalDisplayIf(libEntryP == NULL, "invalid AddressXTLib refNum");

	gH = (MemHandle)(libEntryP->globalsP);

	/* Free our library globals */
	if ( gH )
	{
		/* clear our globals reference */
		libEntryP->globalsP = NULL;

		/* free our globals */
		MemHandleFree(gH);
	}
}

/*
 * FUNCTION: PrvLockGlobals
 *
 * DESCRIPTION:	Lock our library globals
 *
 * PARAMETERS:
 *
 * refNum
 *		Library reference number returned by SysLibLoad() or SysLibFind().
 *
 * CALLED BY: internal
 *
 * RETURNS:
 *		pointer to our library globals
 *		NULL if our globals	have not been created yet.
 */

static AddressXTLibGlobalsType * PrvLockGlobals(UInt16 refNum)
{
	AddressXTLibGlobalsType * gP = NULL;
	MemHandle gH;
	SysLibTblEntryType * libEntryP;

	libEntryP = SysLibTblEntry(refNum);
	if ( libEntryP )
		gH = (MemHandle)(libEntryP->globalsP);
	if ( gH )
		gP = (AddressXTLibGlobalsType *)MemHandleLock(gH);

	return( gP );
}

/*
 * FUNCTION: PrvIsLibOpen
 *
 * DESCRIPTION:	Check if the library has been opened.
 *
 * PARAMETERS:
 *
 * refNum
 *		Library reference number returned by SysLibLoad() or SysLibFind().
 *
 * CALLED BY: internal
 *
 * RETURNS: non-zero if the library has been opened
 */

static Boolean PrvIsLibOpen(UInt16 refNum)
{
	AddressXTLibGlobalsType * gP;
	Boolean	isOpen = false;

	gP = PrvLockGlobals(refNum);

	if ( gP )
	{
		isOpen = true;
		PrvUnlockGlobals(gP);
	}

	return( isOpen );
}

/*
 * FUNCTION: PrvCreateClientContext
 *
 * DESCRIPTION:	
 *
 * Create a client context for storing client-specific data.
 * The client context allows the library to support multiple clients.
 *
 * PARAMETERS:
 *
 * gP
 *		pointer to our locked globals
 *
 * clientContextP
 *		pointer to variable for returning client context
 *
 * CALLED BY: Internal
 *
 * RETURNS:
 *		errNone
 *		memErrNotEnoughSpace
 *		AddressXTLibErrNotOpen -- the library is not open
 *
 * SIDE EFFECTS:
 *
 *		*clientContextP will be set to client context on success, or zero on error.
 */

static Err PrvCreateClientContext(AddressXTLibGlobalsType * gP, UInt32 * clientContextP)
{
	Err err = errNone;
	MemHandle contextH;
	AddressXTLibClientContextType * contextP;

	/* Error-check our parameters */
	ErrFatalDisplayIf(gP == NULL, "null globals pointer");
	ErrFatalDisplayIf(clientContextP == NULL, "null context variable pointer");

	/* Initialize return variable */
	*clientContextP = 0;

	/* Allocate a new client context structure */
	contextH = MemHandleNew(sizeof(AddressXTLibClientContextType));
	if ( !contextH )
	{
		err = memErrNotEnoughSpace;
	}
	else
	{
		/* save context chunk handle in return variable */
		*clientContextP = (UInt32)contextH;

		/* Initialize the context chunk */
		contextP = (AddressXTLibClientContextType *)MemHandleLock(contextH);

		/* save address of open routine as signature to validate context */
		contextP->libSignature = (void *)&AddressXTLib2Open;

		/* TODO: Insert code to initialize context members */

		PrvUnlockContext(contextP);

		/* increment context count (for debugging) */
		gP->contextCount++;
		ErrFatalDisplayIf(gP->contextCount == 0, "context count overflow");
	}

	return( err );
}

/*
 * FUNCTION: PrvDestroyClientContext
 *
 * DESCRIPTION:	Destroy a client context which was created by PrvCreateClientContext.
 *
 * PARAMETERS:
 *
 * gP
 *		pointer to our locked globals
 *
 * clientContextP
 *		client context
 *
 * CALLED BY: Internal
 *
 * RETURNS:
 *		errNone
 *		AddressXTLibErrNotOpen -- the library is not open
 */

static Err PrvDestroyClientContext(AddressXTLibGlobalsType * gP, UInt32 clientContext)
{
	AddressXTLibClientContextType *	contextP;

	/* Error-check our parameters */
	ErrFatalDisplayIf(gP == NULL, "null globals pointer");

	/* Validate the client context by locking it */
	contextP = PrvLockContext(clientContext);

	if ( contextP )
	{
		/* freeing a locked chunk is permitted by the system */
		MemPtrFree(contextP);

		/* decrement context count (for debugging) */
		gP->contextCount--;
		ErrFatalDisplayIf(gP->contextCount < 0, "context count underflow");
	}

	return errNone;
}

/*
 * FUNCTION: PrvLockContext
 *
 * DESCRIPTION:	Validate and lock a client context.
 *
 * PARAMETERS: 
 *
 * context
 *		a client context to lock
 *
 * CALLED BY: internal
 *
 * RETURNS: pointer to the locked client context.
 */
 
static AddressXTLibClientContextType * PrvLockContext(UInt32 context)
{
	AddressXTLibClientContextType *	contextP = NULL;

	/* Error-check our parameters */
	ErrFatalDisplayIf(context == 0, "null client context");

	/* Lock the client context */
	contextP = (AddressXTLibClientContextType *)MemHandleLock((MemHandle)context);
	ErrFatalDisplayIf(contextP == NULL, "failed to lock client context");

	/* Validate the client context */
	ErrFatalDisplayIf(contextP->libSignature != (void *)&AddressXTLib2Open,
		"invalid client context");

	return( contextP );
}