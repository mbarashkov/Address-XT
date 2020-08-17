/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ToDoDB.c
 *
 * Description:
 *		To Do Manager routines
 *
 * History:
 *   	1/20/95  rsf - Created
 *		10/4/99	jmp - Added ToDoGetDatabase() routine.
 *		10/4/99	rbb - Moved SetDBBackupBit from ToDo.c (used by
 *							ToDoGetDatabase) and renamed as ToDoSetDBBackupBit
 *
 *****************************************************************************/

// Set this to get to private database defines
#define __TODOMGR_PRIVATE__

#include <PalmOS.h>

#include "ToDo.h"
#include "ToDoDB.h"

// Export error checking routines
void ECToDoDBValidate(DmOpenRef dbP);


/************************************************************
 * Private routines used only in this module
 *************************************************************/


/************************************************************
 *
 *  FUNCTION: ECToDoDBValidate
 *
 *  DESCRIPTION: This routine validates the integrity of the to do
 *		datebase.
 *
 *  PARAMETERS: database pointer
 *
 *  RETURNS: nothing
 *
 *  CREATED: 6/22/95 
 *
 *  BY: Art Lamb
 *
 *************************************************************/

#define maxDescLen	256
#define maxNoteLen	4000


/************************************************************
 *
 *  FUNCTION: DateTypeCmp
 *
 *  DESCRIPTION: Compare two dates
 *
 *  PARAMETERS: 
 *
 *  RETURNS: 
 *
 *  CREATED: 1/20/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static Int16 DateTypeCmp(DateType d1, DateType d2)
{
	Int16 result;
	
	result = d1.year - d2.year;
	if (result != 0)
		{
		if (DateToInt(d1) == 0xffff)
			return 1;
		if (DateToInt(d2) == 0xffff)
			return -1;
		return result;
		}
	
	result = d1.month - d2.month;
	if (result != 0)
		return result;
	
	result = d1.day - d2.day;

	return result;

}

void ToDoSetDBBackupBit(DmOpenRef dbP)
{
	DmOpenRef localDBP;
	LocalID dbID;
	UInt16 cardNo;
	UInt16 attributes;

	// Open database if necessary. If it doesn't exist, simply exit (don't create it).
	if (dbP == NULL)
		{
		localDBP = DmOpenDatabaseByTypeCreator (toDoDBType, sysFileCToDo, dmModeReadWrite);
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
 *  FUNCTION: CategoryCompare
 *
 *  DESCRIPTION: Compare two categories
 *
 *  PARAMETERS:  attr1, attr2 - record attributes, which contain 
 *						  the category. 
 *					  appInfoH - MemHandle of the applications category info
 *
 *  RETURNS: 0 if they match, non-zero if not
 *				 + if s1 > s2
 *				 - if s1 < s2
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/5/96	Initial Revision
 *
 *************************************************************/
static Int16 CategoryCompare (UInt8 attr1, UInt8 attr2, MemHandle appInfoH)
{
	Int16 result;
	UInt8 category1;
	UInt8 category2;
	ToDoAppInfoPtr appInfoP;
	
	
	category1 = attr1 & dmRecAttrCategoryMask;
	category2 = attr2 & dmRecAttrCategoryMask;

	result = category1 - category2;
	if (result != 0)
		{
		if (category1 == dmUnfiledCategory)
			return (1);
		else if (category2 == dmUnfiledCategory)
			return (-1);
		
		appInfoP = MemHandleLock (appInfoH);
		result = StrCompare (appInfoP->categoryLabels[category1],
									appInfoP->categoryLabels[category2]);

		MemPtrUnlock (appInfoP);
		return result;
		}
	
	return result;
}


/************************************************************
 *
 *  FUNCTION: ToDoAppInfoInit
 *
 *  DESCRIPTION: Create an app info chunk if missing.  Set
 *		the strings to a default.
 *
 *  PARAMETERS: database pointer
 *
 *  RETURNS: 0 if successful, errorcode if not
 *
 *  CREATED: 1/20/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
Err	ToDoAppInfoInit(DmOpenRef dbP)
{
	UInt16 cardNo;
	UInt16 wordValue;
	MemHandle h;
	LocalID dbID;
	LocalID appInfoID;
	ToDoAppInfoPtr	nilP = 0;
	ToDoAppInfoPtr appInfoP;
	
	if (DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL))
		return dmErrInvalidParam;

	if (DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, 
		 NULL, &appInfoID, NULL, NULL, NULL))
		return dmErrInvalidParam;
	
	if (appInfoID == NULL)
		{
		h = DmNewHandle(dbP, sizeof (ToDoAppInfoType));
		if (! h) return dmErrMemError;

		appInfoID = MemHandleToLocalID (h);
		DmSetDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, 
			NULL, &appInfoID, NULL, NULL, NULL);
		}
	
	appInfoP = MemLocalIDToLockedPtr(appInfoID, cardNo);

	// Clear the app info block.
	DmSet (appInfoP, 0, sizeof(ToDoAppInfoType), 0);

	// Initialize the categories.
	CategoryInitialize ((AppInfoPtr) appInfoP, LocalizedAppInfoStr);

	// I don't know what this field is used for.
	wordValue = 0xFFFF;
	DmWrite (appInfoP, (UInt32)&nilP->dirtyAppInfo, &wordValue,
		sizeof(appInfoP->dirtyAppInfo));

	// Initialize the sort order.
	DmSet (appInfoP, (UInt32)&nilP->sortOrder, sizeof(appInfoP->sortOrder), 
		soPriorityDueDate);

	MemPtrUnlock (appInfoP);

	return 0;
}


/************************************************************
 *
 *  FUNCTION: ToDoGetAppInfo
 *
 *  DESCRIPTION: Get the app info chunk 
 *
 *  PARAMETERS: database pointer
 *
 *  RETURNS: MemHandle to the to do application info block (ToDoAppInfoType)
 *
 *  CREATED: 5/12/95 
 *
 *  BY: Art Lamb
 *
 *************************************************************/
MemHandle ToDoGetAppInfo (DmOpenRef dbP)
{
	Err error;
	UInt16 cardNo;
	LocalID dbID;
	LocalID appInfoID;
	
	error = DmOpenDatabaseInfo (dbP, &dbID, NULL, NULL, &cardNo, NULL);
	ErrFatalDisplayIf (error,  "Get getting to do app info block");

	error = DmDatabaseInfo (cardNo, dbID, NULL, NULL, NULL, NULL, NULL, 
			NULL, NULL, &appInfoID, NULL, NULL, NULL);
	ErrFatalDisplayIf (error,  "Get getting to do app info block");

	return ((MemHandle) MemLocalIDToGlobal (appInfoID, cardNo));
}

/************************************************************
 *
 *  FUNCTION: ToDoSize
 *
 *  DESCRIPTION: Return the size of a ToDoDBRecordType
 *
 *  PARAMETERS: database record
 *
 *  RETURNS: the size in bytes
 *
 *  CREATED: 1/10/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static UInt16 ToDoSize(ToDoDBRecordPtr r)
{
	UInt16 size;
	char *c;
	int cLen;
	
	c = &r->description;
	cLen = StrLen(c) + 1;
	size = sizeof (DateType) + 1 + cLen;
	c += cLen;
	
	size += StrLen(c) + 1;

	return size;
}

/***********************************************************************
 *
 * FUNCTION:     ToDoGetDatabase
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
 *			jmp		10/04/99	Initial Revision
 *
 ***********************************************************************/
Err ToDoGetDatabase (DmOpenRef *dbPP, UInt16 mode)
{
	Err error = 0;
	DmOpenRef dbP;
	UInt16 cardNo;
	LocalID dbID;

	*dbPP = NULL;
  
  // Find the application's data file.  If it doesn't exist create it.
	dbP = DmOpenDatabaseByTypeCreator (toDoDBType, sysFileCToDo, mode);
	if(dbP == 0)
		dbP = DmOpenDatabaseByTypeCreator ('DATA', 'PTod', mode);
	
	
	if (!dbP)
		{
		error = DmCreateDatabase (0, toDoDBName, sysFileCToDo, toDoDBType, false);
		if (error)
			return error;
		
		dbP = DmOpenDatabaseByTypeCreator(toDoDBType, sysFileCToDo, mode);
		if (!dbP)
			return (1);

		// Set the backup bit.  This is to aid syncs with non Palm software.
		ToDoSetDBBackupBit(dbP);
		
		error = ToDoAppInfoInit (dbP);
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

