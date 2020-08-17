/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MemoDB.c
 *
 * Description:
 *		Memo Manager routines
 *
 * History:
 *   	1/3/95	rsf - Created
 *   	9/13/95	mlb - added MemoNewRecord for use by mbs shell command
 *		10/02/99 jmp - Add MemoGetDatabase() routine.
 *
 *****************************************************************************/

#include <PalmOS.h>

#include "MemoDB.h"
#include "MemoMain.h"


/************************************************************
 *
 *  FUNCTION: MemoLocalizeAppInfo
 *
 *  DESCRIPTION: Look for localize app info strings and copy
 *  them into the app info block.
 *
 *  PARAMETERS: application info ptr
 *
 *  RETURNS: nothing
 *
 *  CREATED: 12/13/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static void MemoLocalizeAppInfo (MemoAppInfoPtr appInfoP)
{
	Int16 		i;
	Char **		stringsP;
	MemHandle 	stringsH;
	MemHandle 	localizedAppInfoH;
	Char *		localizedAppInfoP;
	MemoAppInfoPtr	nilP = 0;


	localizedAppInfoH = DmGetResource(appInfoStringsRsc, LocalizedAppInfoStr);
	if (localizedAppInfoH)
		{
		localizedAppInfoP = MemHandleLock(localizedAppInfoH);
		stringsH = SysFormPointerArrayToStrings(localizedAppInfoP, 
			dmRecNumCategories);
		stringsP = MemHandleLock(stringsH);
		
		// Copy each category
		for (i = 0; i < dmRecNumCategories; i++)
			{
			if (stringsP[i][0] != '\0')
				DmStrCopy(appInfoP, (UInt32) nilP->categoryLabels[i], stringsP[i]);
			}
		
		MemPtrFree(stringsP);
		MemPtrUnlock(localizedAppInfoP);
		}
}

static void SetDBBackupBit(DmOpenRef dbP)
{
	DmOpenRef localDBP;
	LocalID dbID;
	UInt16 cardNo;
	UInt16 attributes;

	// Open database if necessary. If it doesn't exist, simply exit (don't create it).
	if (dbP == NULL)
		{
		localDBP = DmOpenDatabaseByTypeCreator (memoDBType, sysFileCMemo, dmModeReadWrite);
		if (localDBP == NULL)  return;
		}
	else
		{
		localDBP = dbP;
		}
	
	// now set the backup bit on localDBP
	DmOpenDatabaseInfo(localDBP, &dbID, NULL, NULL, &cardNo, NULL);
	DmDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	attributes |= dmHdrAttrBackup;
	DmSetDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	
	// close database if necessary
   if (dbP == NULL) 
   	{
   	DmCloseDatabase(localDBP);
      }
}



/************************************************************
 *
 *  FUNCTION: MemoAppInfoInit
 *
 *  DESCRIPTION: Create an app info chunk if missing.  Set
 *		the strings to a default.
 *
 *  PARAMETERS: database pointer
 *
 *  RETURNS: 0 if successful, errorcode if not
 *
 *  CREATED: 1/3/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
Err	MemoAppInfoInit(DmOpenRef dbP)
{
	UInt16 				cardNo;
	MemHandle			h;
	LocalID 				dbID;
	LocalID 				appInfoID;
	MemoAppInfoPtr 	nilP = 0;
	MemoAppInfoPtr		appInfoP;
	
	if (DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL))
		return dmErrInvalidParam;
	if (DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, 
			NULL, NULL, &appInfoID, NULL, NULL, NULL))
		return dmErrInvalidParam;
	
	if (appInfoID == NULL)
		{
		h = DmNewHandle (dbP, sizeof (MemoAppInfoType));
		if (! h) return dmErrMemError;
		
		appInfoID = MemHandleToLocalID (h);
		DmSetDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL);
		}
	
	appInfoP = MemLocalIDToLockedPtr(appInfoID, cardNo);

	// Clear the app info block.
	DmSet (appInfoP, 0, sizeof(MemoAppInfoType), 0);

	// Initialize the categories.
	CategoryInitialize ((AppInfoPtr) appInfoP, LocalizedAppInfoStr);

	// Initialize the sort order.
	DmSet (appInfoP, (UInt32)&nilP->sortOrder, sizeof(appInfoP->sortOrder), 
		soAlphabetic);

	MemPtrUnlock(appInfoP);

	// The conduit ignores dmHdrAttrAppInfoDirty
	return 0;
}


/************************************************************
 *
 *  FUNCTION:    MemoGetAppInfo
 *
 *  DESCRIPTION: Get the app info chunk 
 *
 *  PARAMETERS:  database pointer
 *
 *  RETURNS:     MemHandle to the to do application info block 
 *               (MemoAppInfoType)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/18/96	Initial Revision
 *
 *************************************************************/
static MemHandle MemoGetAppInfo (DmOpenRef dbP)
{
	Err error;
	UInt16 cardNo;
	LocalID dbID;
	LocalID appInfoID;
	
	error = DmOpenDatabaseInfo (dbP, &dbID, NULL, NULL, &cardNo, NULL);
	ErrFatalDisplayIf (error,  "Get getting app info block");
	
	error = DmDatabaseInfo (cardNo, dbID, NULL, NULL, NULL, NULL, NULL, 
			NULL, NULL, &appInfoID, NULL, NULL, NULL);
	ErrFatalDisplayIf (error,  "Get getting app info block");

	return ((MemHandle) MemLocalIDToGlobal (appInfoID, cardNo));
}



/***********************************************************************
 *
 * FUNCTION:     MemoGetDatabase
 *
 * DESCRIPTION:  Get the application's database.  Open the database if it
 * exists, create it if neccessary.
 *
 * PARAMETERS:   *dbPP - pointer to a database ref (DmOpenRef) to be set
 *					  mode - how to open the database (dmModeReadWrite)
 *
 * RETURNED:     Err - zero if no error, else the error
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			jmp		10/02/99	Initial Revision
 *
 ***********************************************************************/
Err MemoGetDatabase (DmOpenRef *dbPP, UInt16 mode)
{
	Err error = 0;
	DmOpenRef dbP;
	UInt16 cardNo;
	LocalID dbID;

	*dbPP = NULL;
  
  // Find the application's data file.  If it doesn't exist create it.
	dbP = DmOpenDatabaseByTypeCreator (memoDBType, sysFileCMemo, mode);
	if(dbP == 0)
		dbP = DmOpenDatabaseByTypeCreator ('DATA', 'PMem', mode);
	if (!dbP)
		{
		error = DmCreateDatabase (0, memoDBName, sysFileCMemo, memoDBType, false);
		if (error)
			return error;
		
		dbP = DmOpenDatabaseByTypeCreator(memoDBType, sysFileCMemo, mode);
		if (!dbP)
			return (1);

		// Set the backup bit.  This is to aid syncs with non Palm software.
		SetDBBackupBit(dbP);
		
		error = MemoAppInfoInit (dbP);
      if (error) 
      	{
			DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL);
      	DmCloseDatabase(dbP);
      	DmDeleteDatabase(cardNo, dbID);
         return error;
         }
		}
	
	*dbPP = dbP;
	return 0;
}

