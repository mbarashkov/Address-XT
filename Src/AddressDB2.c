#include "AddressDB.h"
#include "AddressDB2.h"
#include "ContactsDB.h"
#include "Address.h"
#include "AddrTools.h"
#include "AddrDefines.h"
#include "globals.h"
#include "AddrXTDB.h"

#include <UIResources.h>
#include <SysUtils.h>
#include <ErrorMgr.h>
#include <StringMgr.h>
#include <TextMgr.h>
#include <PalmUtils.h>

#include <FeatureMgr.h>
#include "AddressSortLib.h"

/***********************************************************************
 *
 * FUNCTION:    AddrDBRecordContainsData
 *
 * DESCRIPTION: Checks the record returns true if it contains any data.
 *
 * PARAMETERS:  recordP  - a pointer to an address record
 *
 * RETURNED:    true if one of the fields has data
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         rsf   12/3/97   Initial Revision
 *
 ***********************************************************************/
Boolean AddrDBRecordContainsData (void* recordP)
{
	UInt16 i;
	
	// Look for a field which isn't empty
#ifdef CONTACTS
	for (i = P1ContactsfirstAddressField; i < P1ContactsaddressFieldsCount; i++)
	{
		if (((P1ContactsDBRecordPtr)recordP)->fields[i] != NULL)
			return true;
	}
#else
	for (i = firstAddressField; i < addressFieldsCount; i++)
	{
		if (((AddrDBRecordPtr)recordP)->fields[i] != NULL)
			return true;
	}
#endif
	return false;
}

/************************************************************
 *
 *  FUNCTION: AddrDBSetFieldLabel
 *
 *  DESCRIPTION: Set a field's label and mark it dirty.
 *
 *  PARAMETERS: dbP - open database pointer
 *                fieldNum - field label to change
 *                fieldLabel - new field label to use
 *
 *  RETURNS: 0 if successful, errorcode if not
 *
 *  CREATED: 6/28/95
 *
 *  BY: Roger Flores
 *
 *************************************************************/
void AddrDBSetFieldLabel(DmOpenRef dbP, UInt16 fieldNum, Char * fieldLabel)
{
	globalVars* globals = getGlobalsPtr();
#ifndef CONTACTS
	AddrAppInfoPtr    appInfoP;
#else
	P1ContactsAppInfoPtr    appInfoCP;
	P1ContactsAppInfoType   copyC;
#endif
	AddrAppInfoType   copy;

#ifdef CONTACTS
		ErrFatalDisplayIf(fieldNum >= P1ContactslastLabel,
						  "fieldNum out of range");

		// Get a copy of the app info
		appInfoCP = P1ContactsDBAppInfoGetPtr(globals->adxtLibRef, dbP);
		ErrFatalDisplayIf(appInfoCP == NULL,
						  "Bad database (invalid or no app info block)");
		MemMove(&copyC, appInfoCP, sizeof(copyC));

		// Make the changes
		StrCopy(copyC.fieldLabels[fieldNum], fieldLabel); //lint !e661
		SetBitMacro(copyC.dirtyFieldLabels.allBits, fieldNum);

		// Write changes to record
		//DmWrite(appInfoCP, 0, &copy, sizeof(copy));
		DmWrite(appInfoCP, 0, &copyC, sizeof(copy)+100);
		// Unlock app info
		MemPtrUnlock(appInfoCP);
#else
		ErrFatalDisplayIf(fieldNum >= lastLabel,
						  "fieldNum out of range");

		// Get a copy of the app info
		appInfoP = AddrDBAppInfoGetPtr(globals->adxtLibRef, dbP);
		ErrFatalDisplayIf(appInfoP == NULL,
						  "Bad database (invalid or no app info block)");
		MemMove(&copy, appInfoP, sizeof(copy));

		// Make the changes
		StrCopy(copy.fieldLabels[fieldNum], fieldLabel); //lint !e661
		SetBitMacro(copy.dirtyFieldLabels.allBits, fieldNum);

		// Write changes to record
		DmWrite(appInfoP, 0, &copy, sizeof(copy));

		// Unlock app info
		MemPtrUnlock(appInfoP);
#endif
}

#ifndef CONTACTS
/************************************************************
 *
 *  FUNCTION: AddrDBChangeRecord
 *
 *  DESCRIPTION: Change a record in the Address Database
 *
 *  PARAMETERS: dbP - open database pointer
 *            database index
 *            address record
 *            changed fields
 *
 *  RETURNS: ##0 if successful, errorcode if not
 *
 *  CREATED: 1/14/95
 *
 *  BY: Roger Flores
 *
 *   COMMENTS:   Records are not stored with extra padding - they
 *   are always resized to their exact storage space.  This avoids
 *   a database compression issue.  The code works as follows:
 *
 *   1)   get the size of the new record
 *   2)   make the new record
 *   3)   pack the packed record plus the changes into the new record
 *   4)   if the sort position is changes move to the new position
 *   5)   attach in position
 *
 * The MemHandle to the record passed doesn't need to be unlocked
 * since that chunk is freed by this routine.  It should be discarded.
 *
 *************************************************************/
Err AddrDBChangeRecord(DmOpenRef dbP, UInt16 *index, AddrDBRecordPtr r, AddrDBRecordFlags changedFields)
{
	globalVars* globals = getGlobalsPtr();
	AddrDBRecordType    src;
	MemHandle             srcH;
	Err                result;
	MemHandle             recordH=0;
	MemHandle             oldH;
	Int16                i;
	UInt32             changes = changedFields.allBits;
	AddrAppInfoPtr    appInfoPtr;
	Boolean            dontMove;
	UInt16                attributes;      // to contain the deleted flag
	PrvAddrPackedDBRecord*   cmpP;
	PrvAddrPackedDBRecord*   recordP;
	
	UInt16 sortByCompany = GetSortByCompany(globals->adxtLibRef);
	
	// We do not assume that r is completely valid so we get a valid
	// AddrDBRecordPtr...
	if ((result = AddrDBGetRecord(globals->adxtLibRef, dbP, *index, &src, &srcH)) != 0)
		return result;

	// and we apply the changes to it.
	src.options = r->options;         // copy the phone info
	for (i = firstAddressField; i < addressFieldsCount; i++)
	{
		// If the flag is set point to the string else NULL
		if (GetBitMacro(changes, i) != 0)
		{
			src.fields[i] = r->fields[i];
			RemoveBitMacro(changes, i);
		}
		if (changes == 0)
			break;      // no more changes
	}


	// 1) and 2) (make a new chunk with the correct size)
	recordH = DmNewHandle(dbP, PrvAddrDBUnpackedSize(globals->adxtLibRef, &src));
	if (recordH == NULL)
	{
		MemHandleUnlock(srcH);      // undo lock from AddrGetRecord above
		return dmErrMemError;
	}
	recordP = MemHandleLock(recordH);


	// 3) Copy the data from the unpacked record to the packed one.
	PrvAddrDBPack(globals->adxtLibRef, &src, recordP);

	// The original record is copied and no longer needed.
	MemHandleUnlock(srcH);


	// 4) if the sort position changes...
	// Check if any of the key fields have changed
	if ((changedFields.allBits & sortKeyFieldBits) == 0)
		goto attachRecord;


	// Make sure *index-1 < *index < *index+1, if so it's in sorted
	// order.  Leave it there.
	appInfoPtr = (AddrAppInfoPtr) AddrDBAppInfoGetPtr(globals->adxtLibRef, dbP);
	//sortByCompany = appInfoPtr->misc.sortByCompany;
	MemPtrUnlock(appInfoPtr);

	if (*index > 0)
	{
		// This record wasn't deleted and deleted records are at the end of the
		// database so the prior record may not be deleted!
		cmpP = MemHandleLock(DmQueryRecord(dbP, *index-1));
/* all this function - if undef CONTACTS
#ifdef CONTACTS
		dontMove = (PrvAddrDBComparePackedRecords (cmpP,  recordP, sortByCompany + CONTACTS_SORTOFFSET,
											  NULL, NULL, 0) == -1);
#else
*/
		dontMove = (PrvAddrDBComparePackedRecords (cmpP,  recordP, sortByCompany,
										  NULL, NULL, 0) == -1);
// #endif
		MemPtrUnlock(cmpP);
	}
	else
		dontMove = true;


	if (*index+1 < DmNumRecords (dbP))
	{
		DmRecordInfo(dbP, *index+1, &attributes, NULL, NULL);
		if (attributes & dmRecAttrDelete)
			;      // don't move it after the deleted record!
		else {
			cmpP = MemHandleLock(DmQueryRecord(dbP, *index+1));
/* all this function - if undef CONTACTS
#ifdef CONTACTS
			dontMove = dontMove && (PrvAddrDBComparePackedRecords (recordP, cmpP, sortByCompany + CONTACTS_SORTOFFSET, NULL, NULL, 0) == -1);
#else
*/
			dontMove = dontMove && (PrvAddrDBComparePackedRecords (recordP, cmpP, sortByCompany, NULL, NULL, 0) == -1);
// #endif
			MemPtrUnlock(cmpP);
		}
	}


	if (dontMove)
		goto attachRecord;



	// The record isn't in the right position.  Move it.
	i = PrvAddrDBFindSortPosition(globals->adxtLibRef, dbP, recordP);
	DmMoveRecord(dbP, *index, i);
	if (i > *index) i--;
	*index = i;                  // return new position


	// Attach the new record to the old index,  the preserves the
	// category and record id.
attachRecord:

	result = DmAttachRecord(dbP, index, recordH, &oldH);
	MemPtrUnlock(recordP);
	if (result) return result;

	MemHandleFree(oldH);
	return 0;
}


Err AddrDBChangeRecordCustom(DmOpenRef dbP, UInt16 *index, AddrDBRecordPtr r, AddrDBRecordFlags changedFields)
{
	globalVars* globals = getGlobalsPtr();
	AddrDBRecordType    src;
	MemHandle             srcH;
	Err                result;
	MemHandle             recordH=0;
	MemHandle             oldH;
	Int16                i;
	UInt32             changes = changedFields.allBits;
	UInt16                attributes;
	
	PrvAddrPackedDBRecord*   recordP;
	UInt16 attr;

	if ((result = AddrDBGetRecord(globals->adxtLibRef, dbP, *index, &src, &srcH)) != 0)
		return result;

	src.options = r->options;
	for (i = firstAddressField; i < addressFieldsCount; i++)
	{
			src.fields[i] = r->fields[i];
	}


	recordH = DmNewHandle(dbP, PrvAddrDBUnpackedSize(globals->adxtLibRef, &src));
	if (recordH == NULL)
	{
		MemHandleUnlock(srcH);
		return dmErrMemError;
	}
	recordP = MemHandleLock(recordH);

	PrvAddrDBPack(globals->adxtLibRef, &src, recordP);

	MemHandleUnlock(srcH);

	if (*index+1 < DmNumRecords (dbP))
	{
		DmRecordInfo(dbP, *index+1, &attributes, NULL, NULL);

	}


	result = DmAttachRecord(dbP, index, recordH, &oldH);
	MemPtrUnlock(recordP);
	if (result) return result;
	
	MemHandleFree(oldH);
	
	DmRecordInfo(dbP, *index, &attr, NULL, NULL);
	attr |= dmRecAttrDirty;
	DmSetRecordInfo(dbP, *index, &attr, NULL);
	
	return 0;
}
#endif  // CONTACTS

