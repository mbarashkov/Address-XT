
#include <UIResources.h>
#include <SysUtils.h>
#include <ErrorMgr.h>
#include <StringMgr.h>
#include <TextMgr.h>
#include <PalmUtils.h>

#include "AddressDB.h"
#include "Address.h"
#include "AddrTools.h"
#include "AddressRsc.h"

#ifdef LIBDEBUG //moved to shared library


#include <FeatureMgr.h>
#include "AddressSortLib.h"

#include "AddressDB2.h"
#include "ContactsDB.h"
#include "AddrDefines.h"
#include "globals.h"
#include "AddrXTDB.h"

/***********************************************************************
 *
 *   Defines
 *
 ***********************************************************************/


// The following structure doesn't really exist.  The first field
// varies depending on the data present.  However, it is convient
// (and less error prone) to use when accessing the other information.


/************************************************************
 *
 *  FUNCTION: AddrDBAppInfoGetPtr
 *
 *  DESCRIPTION: Return a locked pointer to the AddrAppInfo or NULL
 *
 *  PARAMETERS: dbP - open database pointer
 *
 *  RETURNS: locked ptr to the AddrAppInfo or NULL
 *************************************************************/
AddrAppInfoPtr   AddrDBAppInfoGetPtr(UInt16 refNum, DmOpenRef dbP)
{
	UInt16     cardNo;
	LocalID    dbID;
	LocalID    appInfoID;

	if (DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL))
		return NULL;
	if (DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL))
		return NULL;

	if (appInfoID == 0)
		return NULL;
	else
		return MemLocalIDToLockedPtr(appInfoID, cardNo);
}

void UpdateAlarms(UInt16 refNum, DmOpenRef addrXTDB)
{
	/*globalVars* globals = getGlobalsPtr();
	UInt32 counter, minID = 0;
	UInt32 curSeconds, nowSeconds, minSeconds, minRemSeconds, remSeconds;
	DateTimeType date, curDate;
	AddrXTDBRecordType rec;
	UInt16 addrXTDBSize;
	//init birthday
	UInt16 attr;
	Boolean found=false;
	MemHandle mH;
	UInt32 id, minimumID, seconds, remind, passed;
	privateRecordViewEnum privateRecordVisualStatus;
	DmOpenRef dbP;
	//UInt16 minCount;
	LocalID lID = DmFindDatabase(0, APP_DBNAME);
	UInt32 alarmSeconds;
	
	if(lID == 0)
		return;

	privateRecordVisualStatus = (privateRecordViewEnum)PrefGetPreference(prefShowPrivateRecords);
	
	addrXTDBSize = DmNumRecords(addrXTDB);
	nowSeconds=curSeconds=TimGetSeconds();
	TimSecondsToDateTime(curSeconds, &curDate);
	curDate.hour=0;
	curDate.minute=0;
	curDate.second=0;
	curSeconds=TimDateTimeToSeconds(&curDate);			
	AlmSetAlarm(0, (LocalID)DmFindDatabase(0, APP_DBNAME), 0, 0, true);
	minSeconds=-1;
	counter = 0;

	for(counter=0;counter<addrXTDBSize;counter++)
	{
		if(DmRecordInfo(addrXTDB, counter, &attr, NULL, NULL) != errNone)
			continue;
		if(attr & dmRecAttrDelete )
			continue;		
		if(AddrXTDBGetRecord(globals->adxtLibRef, addrXTDB, counter, &rec, &mH) != 0)
			continue;
		
		if(MemHandleSize(mH) != sizeof(AddrXTDBRecordType))
		{
			MemHandleUnlock(mH);
			continue;
		}
		
		id = rec.id;
		seconds = rec.bday;
		remind = rec.remind;
		passed = rec.passed;
		
		if(passed != 0)
		{
			//PASSED field is set to TRUE
			TimSecondsToDateTime(seconds, &date);
			if(date.month<curDate.month)
				date.year=curDate.year+1;
			else
				date.year=curDate.year;
			seconds=TimDateTimeToSeconds(&date);
			seconds-=remind*86400;
			if(seconds>curSeconds)
			{
				rec.passed = 0;
				AddrXTDBChangeRecord(globals->adxtLibRef, addrXTDB, &rec, counter);
				counter = 0;
				continue;
			}				
		}
		MemHandleUnlock(mH);
	}

	dbP = DmOpenDatabaseByTypeCreator (addrDBType, sysFileCAddress, dmModeReadOnly);
	
	if(dbP == 0)
		return;
	
	for(counter=0;counter<addrXTDBSize;counter++)
	{
		AddrXTDBRecordType rec;
		MemHandle mH;
		
		//reset ADDRESS_PASSED FIELDS
		if(DmRecordInfo(addrXTDB, counter, &attr, NULL, NULL) != errNone)
			continue;
		if(attr & dmRecAttrDelete )
			continue;		
		if(AddrXTDBGetRecord(globals->adxtLibRef, addrXTDB, counter, &rec, &mH) != 0)
			continue;
		
		if(MemHandleSize(mH) != sizeof(AddrXTDBRecordType))
		{
			MemHandleUnlock(mH);
			continue;
		}
		
		id = rec.id;
		seconds = rec.bday;
		passed = rec.passed;
		remind = rec.remind;
		
		MemHandleUnlock(mH);

		if(remind != REMIND_NONE)
		{
			UInt32 diff;
			
			remSeconds=remind*86400;
			TimSecondsToDateTime(seconds, &date);
			if(date.month<curDate.month)
				date.year=curDate.year+1;
			else
				date.year=curDate.year;
			
			seconds=TimDateTimeToSeconds(&date);
			
			diff = seconds - curSeconds;
			
			if((seconds>=curSeconds) && (seconds-remSeconds<=minSeconds))
			{
				UInt16 index = 0;
				UInt16 attr;
	
				if(passed==1)
					continue;
				minSeconds=seconds;
				minRemSeconds=remSeconds;
				//check if the address exists in AddrDB
				if(id & 0xFF000000)
				{
					//invalid unique ID
					found = false;
					continue;
				}
				if(DmFindRecordByID(dbP, id, &index)!=0)
				{
					found=false;
				}
				else
				{
					DmRecordInfo(dbP, index, &attr, NULL, NULL);
					if(attr & dmRecAttrDelete )
					{
						found=false;
					}
					else
					{
						if(!((attr & dmRecAttrSecret)  && globals->PrivateRecordVisualStatus != showPrivateRecords))
						{
							found=true;
							minID = id;
							minimumID = id;
							
									
						}
					}
				}
			}
		}		
	}	

	DmCloseDatabase(dbP);

	
	if(minSeconds!=-1 && found)
	{
					
		if(minSeconds-minRemSeconds<nowSeconds)
			alarmSeconds=nowSeconds+1;
		else
			alarmSeconds=minSeconds-minRemSeconds;
		
		AlmSetAlarm(0, lID, minID, alarmSeconds, false);
	}*/
}

/************************************************************
 *
 *  FUNCTION: AddrDBChangeCountry
 *
 *  DESCRIPTION: Set the field labels to those appropriate
 *  to the current country (based on system preferences).
 *
 *  PARAMETERS: application info ptr
 *
 *  RETURNS: nothing
 *************************************************************/
void AddrDBChangeCountry(UInt16 refNum, void* appInfoP)
{
#ifndef CONTACTS
	CountryType countryCurrent;
	AddrAppInfoPtr   nilP = NULL;
	P1ContactsAppInfoPtr   nilCP = NULL;
	MemHandle textH;
	UInt16 strListID;
	Char fieldName[maxFieldName + sizeOf7BitChar(chrNull)];

	// Localize the field labels to the current country
	countryCurrent = (CountryType) PrefGetPreference(prefCountry);
	strListID = (UInt16)countryCurrent + FieldNamesStrList;
	textH = DmGetResource(strListRscType, strListID);
	if (textH != NULL)
	{
		AddrDBRecordFlags dirtyFieldLabels;
		//P1ContactsDBRecordFlags dirtyContactsFieldLabels;

		DmStrCopy(appInfoP, (Int32) nilP->fieldLabels[city],
				  SysStringByIndex(strListID, fieldNameStrListCity, fieldName, maxFieldName));
		DmStrCopy(appInfoP, (Int32) nilP->fieldLabels[state],
				  SysStringByIndex(strListID, fieldNameStrListState, fieldName, maxFieldName));
		DmStrCopy(appInfoP, (Int32) nilP->fieldLabels[zipCode],
				  SysStringByIndex(strListID, fieldNameStrListZip, fieldName, maxFieldName));

		dirtyFieldLabels.allBits = (((AddrAppInfoPtr)appInfoP)->dirtyFieldLabels.allBits) |
			BitAtPosition(city) | BitAtPosition(state) | BitAtPosition(zipCode);

		DmWrite(appInfoP, (Int32) &nilP->dirtyFieldLabels, &dirtyFieldLabels, sizeof dirtyFieldLabels);	
	}

	// Record the country.
	DmWrite(appInfoP, (Int32) &nilP->country, &countryCurrent, sizeof(countryCurrent));
#else
#pragma unused(appInfoP)
#endif

}
/************************************************************
 *
 *  FUNCTION: AddrDBAppInfoInit
 *
 *  DESCRIPTION: Create an app info chunk if missing.  Set
 *      the strings to a default.
 *
 *  PARAMETERS: dbP - open database pointer
 *
 *  RETURNS: 0 if successful, errorcode if not
 *************************************************************/
Err   AddrDBAppInfoInit(UInt16 refNum, DmOpenRef dbP)
{
#ifndef CONTACTS
	globalVars* globals = getGlobalsPtr();
	UInt16         cardNo;
	LocalID        dbID;
	LocalID        appInfoID;
	MemHandle         h;
	AddrAppInfoPtr appInfoP;
	AddrAppInfoPtr defaultAddrApplicationInfoP;
	UInt8 i;    

	return 0;

	appInfoP = AddrDBAppInfoGetPtr(globals->adxtLibRef, dbP);

	// If there isn't an AddrApplicationInfo make space for one
	if (appInfoP == NULL)
	{
		if (DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL))
			return dmErrInvalidParam;
		if (DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL))
			return dmErrInvalidParam;

		h = DmNewHandle(dbP, sizeof(AddrAppInfoType));
		if (!h) return dmErrMemError;

		appInfoID = MemHandleToLocalID( h);
		if (DmSetDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL))
		{
			MemHandleFree(h);
			return 1;
		}

		appInfoP = MemHandleLock(h);
	}


	// Allocate & Clear the app info
	defaultAddrApplicationInfoP = MemPtrNew(sizeof(AddrAppInfoType));
	if (defaultAddrApplicationInfoP == NULL)
	{
		ErrDisplay("Unable to init AddressDB");
		return 1;
	}

	MemSet(defaultAddrApplicationInfoP, sizeof(AddrAppInfoType), 0);

	// Init the categories
	for (i = 0; i < dmRecNumCategories; i++)
	{
		defaultAddrApplicationInfoP->categoryUniqIDs[i] = i;
	}
	defaultAddrApplicationInfoP->lastUniqID = dmRecNumCategories - 1;

	// Set to sort by name
	defaultAddrApplicationInfoP->misc.sortByCompany =  PreferencesLastName;


	// copy in the defaults and free the default app info
	DmWrite(appInfoP, 0, defaultAddrApplicationInfoP,  sizeof(AddrAppInfoType));
	MemPtrFree(defaultAddrApplicationInfoP);


	// Try to use localized app info block strings.
	PrvAddrDBLocalizeAppInfo(globals->adxtLibRef, appInfoP);

	// Localize the field labels to the current country
	AddrDBChangeCountry(globals->adxtLibRef, appInfoP);


	// Unlock
	MemPtrUnlock(appInfoP);
#else
#pragma unused(dbP)
#endif
	return 0;
}

/************************************************************
 *
 *  FUNCTION: AddrDBNewRecord
 *
 *  DESCRIPTION: Create a new packed record in sorted position
 *
 *  PARAMETERS: database pointer - open db pointer
 *            address record   - pointer to a record to copy into the DB
 *            record index      - to be set to the new record's index
 *
 *  RETURNS: ##0 if successful, errorcode if not
 *             index set if a new record is created.
 *************************************************************/
Err AddrDBNewRecord(UInt16 refNum, DmOpenRef dbP, AddrDBRecordPtr r, UInt16 *index)
{
#ifndef CONTACTS
	globalVars* globals = getGlobalsPtr();
	MemHandle               recordH;
	Err                   err;
	PrvAddrPackedDBRecord*   recordP;
	UInt16                   newIndex;


	// 1) and 2) (make a new chunk with the correct size)
	
	recordH = DmNewHandle(dbP, (Int32) PrvAddrDBUnpackedSize(globals->adxtLibRef, r));
	if (recordH == NULL)
		return dmErrMemError;


	// 3) Copy the data from the unpacked record to the packed one.
	
	recordP = MemHandleLock(recordH);
	
	PrvAddrDBPack(globals->adxtLibRef, r, recordP);

	// Get the index
	newIndex = PrvAddrDBFindSortPosition(globals->adxtLibRef, dbP, recordP);
	MemPtrUnlock(recordP);

	// 4) attach in place
	err = DmAttachRecord(dbP, &newIndex, recordH, 0);
	if (err)
		MemHandleFree(recordH);
	else
		*index = newIndex;
	
	return err;
#else
#pragma unused(dbP, r, index)
	return 0;
#endif
}




/************************************************************
 *
 *  FUNCTION: AddrDBGetRecord
 *
 *  DESCRIPTION: Get a record from the Address Database
 *
 *  PARAMETERS: database pointer - open db pointer
 *            database index - index of record to lock
 *            address record pointer - pointer address structure
 *            address record - MemHandle to unlock when done
 *
 *  RETURNS: ##0 if successful, errorcode if not
 *    The record's MemHandle is locked so that the pointer to
 *  strings within the record remain pointing to valid chunk
 *  versus the record randomly moving.  Unlock the MemHandle when
 *  AddrDBRecord is destroyed.
 *************************************************************/
Err AddrDBGetRecord(UInt16 refNum, DmOpenRef dbP, UInt16 index, AddrDBRecordPtr recordP,
				  MemHandle *recordH)
{
#ifndef CONTACTS
	globalVars* globals = getGlobalsPtr();
	PrvAddrPackedDBRecord *src;

	*recordH = DmQueryRecord(dbP, index);
	if(*recordH == NULL)
		return dmErrIndexOutOfRange;
	src = (PrvAddrPackedDBRecord *) MemHandleLock(*recordH);
	if (src == NULL)
		return dmErrIndexOutOfRange;

	PrvAddrDBUnpack(globals->adxtLibRef, src, recordP);
#else
#pragma unused(dbP, index, recordP, recordH)
#endif
	return 0;
}

Err AddrDBChangeSortOrder(UInt16 refNum, DmOpenRef dbP, UInt16 sortByCompany)
{
	UInt16 numRec;
	numRec = DmNumRecordsInCategory(dbP, dmAllCategories);
	if(numRec == 0)
		return 0;
	DmInsertionSort(dbP, (DmComparF *) PrvAddrDBComparePackedRecords, (Int16) sortByCompany);
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:     AddrDBGetDatabase
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
 *			jmp		10/01/99	Initial Revision
 *
 ***********************************************************************/
Err AddrDBGetDatabase (UInt16 refNum, DmOpenRef *dbPP, UInt16 mode)
{
	globalVars* globals = getGlobalsPtr();
	Err error = 0;
	DmOpenRef dbP;
#ifndef CONTACTS
	UInt16 cardNo;
	LocalID dbID;
#endif

	*dbPP = NULL;

	// Find the application's data file.  If it doesn't exist create it.
	dbP = DmOpenDatabaseByTypeCreator (addrDBType, sysFileCAddress, mode);
	if (!dbP)
	{
		error = DmCreateDatabase (0, addrDBName, sysFileCAddress, addrDBType, false);
		if (error)
			return error;

		dbP = DmOpenDatabaseByTypeCreator(addrDBType, sysFileCAddress, mode);
		if (!dbP)
			return (1);

		// Set the backup bit.  This is to aid syncs with non Palm software.
		ToolsSetDBAttrBits(globals->adxtLibRef, dbP, dmHdrAttrBackup);

#ifndef CONTACTS
		error = AddrDBAppInfoInit (globals->adxtLibRef, dbP);
		if (error)
		{
			DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL);
			DmCloseDatabase(dbP);
			DmDeleteDatabase(cardNo, dbID);
			return error;
		}
#endif
	}

	*dbPP = dbP;
	return 0;
}

#pragma mark -

/************************************************************
 *
 *  FUNCTION: PrvAddrDBLocalizeAppInfo
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
 *  MODIFICATIONS:
 *      10/22/96   roger      Set flags when field modified
 *************************************************************/
void PrvAddrDBLocalizeAppInfo(UInt16 refNum, AddrAppInfoPtr appInfoP)
{
#ifndef CONTACTS
	MemHandle       localizedAppInfoH;
	Char *          localizedAppInfoP;
	AddrAppInfoPtr   nilP = 0;
	MemHandle       stringsH;
	Char *         *stringsP;
	int             i;
	UInt16            localRenamedCategories;
	UInt32            localDirtyFieldLabels;


	localizedAppInfoH = DmGetResource(appInfoStringsRsc, LocalizedAppInfoStr);
	if (!localizedAppInfoH)
		return;
	localizedAppInfoP = MemHandleLock(localizedAppInfoH);
	stringsH = SysFormPointerArrayToStrings(localizedAppInfoP,
											dmRecNumCategories + addrNumFields + numPhoneLabelsStoredSecond);
	stringsP = MemHandleLock(stringsH);


	// Copy each category
	localRenamedCategories = appInfoP->renamedCategories;
	for (i = 0; i < dmRecNumCategories; i++)
	{
		if (stringsP[i][0] != '\0')
		{
			DmStrCopy(appInfoP, (Int32) nilP->categoryLabels[i], stringsP[i]);
			SetBitMacro(localRenamedCategories, i);
		}
	}
	DmWrite(appInfoP, (Int32) &nilP->renamedCategories, &localRenamedCategories,
			sizeof(localRenamedCategories));


	// Copy each field label
	localDirtyFieldLabels = appInfoP->dirtyFieldLabels.allBits;
	for (i = 0; i < (addrNumFields + numPhoneLabelsStoredSecond); i++)
	{
		if (stringsP[i + dmRecNumCategories][0] != '\0')
		{
			DmStrCopy(appInfoP, (Int32) nilP->fieldLabels[i],
					  stringsP[i + dmRecNumCategories]);
			SetBitMacro(localDirtyFieldLabels, i);
		}
	}
	DmWrite(appInfoP, (Int32) &nilP->dirtyFieldLabels.allBits, &localDirtyFieldLabels,
			sizeof(localDirtyFieldLabels));


	MemPtrFree(stringsP);
	MemPtrUnlock(localizedAppInfoP);
	DmReleaseResource(localizedAppInfoH);
#endif
}

/************************************************************
 *
 *  FUNCTION: PrvAddrDBUnpackedSize
 *
 *  DESCRIPTION: Return the size of an AddrDBRecordType
 *
 *  PARAMETERS: address record
 *
 *  RETURNS: the size in bytes
 *
 *  CREATED: 1/10/95
 *
 *  BY: Roger Flores
 *
 *************************************************************/
Int16 PrvAddrDBUnpackedSize(UInt16 refNum, AddrDBRecordPtr r)
{
#ifndef CONTACTS
	Int16 size;
	Int16   index;

	size = sizeof (PrvAddrPackedDBRecord) - sizeof (char);   // correct
	for (index = firstAddressField; index < addressFieldsCount; index++)
	{
		if (r->fields[index] != NULL)
			size += StrLen(r->fields[index]) + 1;
	}
	return size;
#else
#pragma unused(r)
	return 0;
#endif
}


/************************************************************
 *
 *  FUNCTION: PrvAddrDBPack
 *
 *  DESCRIPTION: Pack an AddrDBRecordType.  Doesn't pack empty strings.
 *
 *  PARAMETERS: address record to pack
 *                address record to pack into
 *
 *  RETURNS: the PrvAddrPackedDBRecord is packed
 *
 *  CREATED: 1/10/95
 *
 *  BY: Roger Flores
 *
 *************************************************************/
void PrvAddrDBPack(UInt16 refNum, AddrDBRecordPtr s, void * recordP)
{
#ifndef CONTACTS
	Int32                offset;
	AddrDBRecordFlags    flags;
	Int16                index;
	univAddrPackedDBRecord*  d=0;
	Int16                len;
	void *               srcP;
	UInt8                companyFieldOffset;

	flags.allBits = 0;

	DmWrite(recordP, (Int32)&d->options, &s->options, sizeof(s->options));
	offset = (Int32)&d->firstField;

	for (index = firstAddressField; index < addressFieldsCount; index++) {
		if (s->fields[index] != NULL)
		{
			ErrFatalDisplayIf(s->fields[index][0] == '\0' && index != note,
							  "Empty field being added");
			srcP = s->fields[index];
			len = StrLen(srcP) + 1;
			DmWrite(recordP, offset, srcP, len);
			offset += len;
			SetBitMacro(flags.allBits, index);
		}
	}

	// Set the flags indicating which fields are used
	DmWrite(recordP, (Int32)&d->flags.allBits, &flags.allBits, sizeof(flags.allBits));

	// Set the companyFieldOffset or clear it
	if (s->fields[company] == NULL)
		companyFieldOffset = 0;
	else {
		index = 1;
		if (s->fields[name] != NULL)
			index += StrLen(s->fields[name]) + 1;
		if (s->fields[firstName] != NULL)
			index += StrLen(s->fields[firstName]) + 1;
		companyFieldOffset = (UInt8) index;
	}
	DmWrite(recordP, (Int32)(&d->companyFieldOffset), &companyFieldOffset, sizeof(companyFieldOffset));
#else
#pragma unused(s, recordP)
#endif
}

/************************************************************
 *
 *  FUNCTION: PrvAddrDBUnpack
 *
 *  DESCRIPTION: Fills in the AddrDBRecord structure
 *
 *  PARAMETERS: address record to unpack
 *                the address record to unpack into
 *
 *  RETURNS: the record unpacked
 *
 *  CREATED: 1/14/95
 *
 *  BY: Roger Flores
 *
 *************************************************************/
void PrvAddrDBUnpack(UInt16 refNum, PrvAddrPackedDBRecord *src, AddrDBRecordPtr dest)
{
#ifndef CONTACTS
	Int16   index;
	UInt32 flags;
	char *p;


	dest->options = src->options;
	flags = src->flags.allBits;
	p = &src->firstField;


	for (index = firstAddressField; index < addressFieldsCount; index++)
	{
		// If the flag is set point to the string else NULL
		if (GetBitMacro(flags, index) != 0)
		{
			dest->fields[index] = p;
			p += StrLen(p) + 1;
		}
		else
			dest->fields[index] = NULL;
	}
#else
#pragma unused(src, dest)
#endif
}

/************************************************************
 *
 *  FUNCTION: PrvAddrDBFindSortPosition
 *
 *  DESCRIPTION: Return where a record is or should be
 *      Useful to find or find where to insert a record.
 *
 *  PARAMETERS: address record
 *
 *  RETURNS: the size in bytes
 *
 *  CREATED: 1/11/95
 *
 *  BY: Roger Flores
 *
 *************************************************************/
UInt16 PrvAddrDBFindSortPosition(UInt16 refNum, DmOpenRef dbP, void *newRecord)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 sortByCompany = GetSortByCompany(globals->adxtLibRef);
	
	UInt16 res;
	res = DmFindSortPosition(dbP, (void *) newRecord, NULL, (DmComparF *)
							  PrvAddrDBComparePackedRecords, (Int16) sortByCompany);
	return res;
}

/************************************************************
 *
 *  FUNCTION: PrvAddrDBFindKey
 *
 *  DESCRIPTION: Return the next valid key
 *
 *  PARAMETERS: database packed record
 *            <-> key to use (ptr to string or NULL for uniq ID)
 *            <-> which key (incremented for use again, starts at 1)
 *            -> sortByCompany
 *
 *  RETURNS:
 *
 *  CREATED: 1/16/95
 *
 *  BY: Roger Flores
 *
 *   COMMENTS:   Returns the key which is asked for if possible and
 *   advances whichKey.  If the key is not available the key advances
 *   to the next one.  The order of keys is:
 *
 * if sortByCompany:
 *      companyKey, nameKey, firstNameKey, uniq ID
 *
 * if !sortByCompany:
 *      nameKey, firstNameKey, companyKey (if no name or first name), uniq ID
 *
 *
 *************************************************************/
void PrvAddrDBFindKey(UInt16 refNum, void *r, char **key, UInt16 *whichKey, UInt16 sortByCompany)
{
#ifndef CONTACTS
	AddrDBRecordFlags fieldFlags;
#else
	P1ContactsDBRecordFlags contactsFieldFlags;
#endif
	Int16   index;
	UInt32 flags;
	Boolean nameFlag, companyFlag, firstNameFlag, titleFlag;
	char *p=NULL;
#ifdef CONTACTS
	PrvP1ContactsPackedDBRecord* pRec = (PrvP1ContactsPackedDBRecord*)r;
	p = &(pRec->firstField);
	flags = contactsFieldFlags.allBits = pRec->flags.allBits;
	nameFlag = contactsFieldFlags.bits.name;
	companyFlag = contactsFieldFlags.bits.company;
	firstNameFlag = contactsFieldFlags.bits.firstName;
	titleFlag = contactsFieldFlags.bits.title;
#else
	PrvAddrPackedDBRecord* pRec = (PrvAddrPackedDBRecord*)r;
	p = &(pRec->firstField);
	flags = fieldFlags.allBits = pRec->flags.allBits;
	nameFlag = fieldFlags.bits.name;
	companyFlag = fieldFlags.bits.company;
	firstNameFlag = fieldFlags.bits.firstName;
	titleFlag = fieldFlags.bits.title;
#endif
	
	ErrFatalDisplayIf(*whichKey == 0 || *whichKey == 5, "Bad addr key");

	if (sortByCompany==PreferencesCompanyName)
	{
		if (*whichKey == 1 && companyFlag)
		{
			*whichKey = 2;
			goto returnCompanyKey;
		}

		if (*whichKey <= 2 && nameFlag)
		{
			*whichKey = 3;
			goto returnNameKey;
		}

		if (*whichKey <= 3 && firstNameFlag)
		{
			*whichKey = 4;
			goto returnFirstNameKey;
		}
	}
	else if(sortByCompany==PreferencesLastName)
	{
		
		if (*whichKey == 1 && nameFlag)
		{
			*whichKey = 2;
			goto returnNameKey;
		}

		if (*whichKey <= 2 && firstNameFlag)
		{
			*whichKey = 3;
			goto returnFirstNameKey;
		}

		// For now don't consider company name when sorting by person name
		// unless there isn't a name or firstName
		if (*whichKey <= 3 && companyFlag &&
			!(nameFlag || firstNameFlag))
		{
			*whichKey = 4;
			goto returnCompanyKey;
		}
	}
	else if(sortByCompany==PreferencesFirstLast)
	{
		
		if (*whichKey == 1 && firstNameFlag)
		{
			*whichKey = 2;
			goto returnFirstNameKey;
		}

		if (*whichKey <= 2 && nameFlag)
		{
			*whichKey = 3;
			goto returnNameKey;
		}

		// For now don't consider company name when sorting by person name
		// unless there isn't a name or firstName
		if (*whichKey <= 3 && companyFlag &&
			!(nameFlag|| firstNameFlag))
		{
			*whichKey = 4;
			goto returnCompanyKey;
		}
	}
	else if(sortByCompany==PreferencesCompanyFirst)
	{
		if (*whichKey == 1 && companyFlag)
		{
			*whichKey = 2;
			goto returnCompanyKey;
		}

		if (*whichKey <= 2 && firstNameFlag)
		{
			*whichKey = 3;
			goto returnFirstNameKey;
		}

		if (*whichKey <= 3 && nameFlag)
		{
			*whichKey = 4;
			goto returnNameKey;
		}
	}
	else if(sortByCompany==PreferencesCompanyTitle)
	{				

#ifdef CONTACTS
		for (index = P1ContactsfirstAddressField; index < P1Contactstitle; index++)
		{
			// If the flag is set point to the string else NULL
			Boolean test;
			switch(index)
			{
				case P1ContactsfirstAddressField:
					test = nameFlag;
					break;
				case P1ContactsfirstName:
					test = firstNameFlag;
					break;
				case P1Contactscompany:
					test = companyFlag;
					break;
				case P1Contactstitle:
					test = titleFlag;
					break;
				default:
					test = false;
					break;
			}
			if (test)
			{
				p += StrLen(p) + 1;
			}
		}
#else
		for (index = firstAddressField; index < titleField; index++)
		{
			// If the flag is set point to the string else NULL
			if (GetBitMacro(flags, index) != 0)
			{
				p += StrLen(p) + 1;
			}
		}
#endif
			
		if (*whichKey == 1 && companyFlag)
		{
			*whichKey = 2;
			goto returnCompanyKey;
		}

		if (*whichKey <= 2 && titleFlag)
		{
			*whichKey = 3;
			goto returnTitleKey;
		}

		if (*whichKey <= 3 && nameFlag)
		{
			*whichKey = 4;
			goto returnNameKey;
		}
		if (*whichKey <= 3 && firstNameFlag)
		{
			*whichKey = 4;
			goto returnFirstNameKey;
		}
	}
	else if(sortByCompany==PreferencesLastTitle)
	{
#ifdef CONTACTS
		for (index = P1ContactsfirstAddressField; index < P1Contactstitle; index++)
		{
			// If the flag is set point to the string else NULL
			Boolean test;
			switch(index)
			{
				case P1ContactsfirstAddressField:
					test = nameFlag;
					break;
				case P1ContactsfirstName:
					test = firstNameFlag;
					break;
				case P1Contactscompany:
					test = companyFlag;
					break;
				case P1Contactstitle:
					test = titleFlag;
					break;
				default:
					test = false;
					break;
			}
			if (test)
			{
				p += StrLen(p) + 1;
			}
		}
#else
		for (index = firstAddressField; index < titleField; index++)
		{
			// If the flag is set point to the string else NULL
			if (GetBitMacro(flags, index) != 0)
			{
				p += StrLen(p) + 1;
			}
		}
#endif
		
		if (*whichKey == 1 && nameFlag)
		{
			*whichKey = 2;
			goto returnNameKey;
		}

		if (*whichKey <= 2 && titleFlag)
		{
			*whichKey = 3;
			goto returnTitleKey;
		}

		if (*whichKey <= 3 && companyFlag)
		{
			*whichKey = 4;
			goto returnCompanyKey;
		}
		if (*whichKey <= 3 && firstNameFlag)
		{
			*whichKey = 4;
			goto returnFirstNameKey;
		}		
	}
	// All possible fields have been tried so return NULL so that
	// the uniq ID is compared.
	*whichKey = 5;
	*key = NULL;
	return;


returnCompanyKey:
	*key = (char *) (&(pRec->companyFieldOffset)) + pRec->companyFieldOffset;
	return;


returnNameKey:
	*key = (char *) &(pRec->firstField);
	return;
	
returnTitleKey:
	*key = p;
	return;


returnFirstNameKey:
	*key = (char *) &(pRec->firstField);
	if (pRec->flags.bits.name)
	{
		*key += StrLen(*key) + 1;
	}
	return;
}

Boolean AddrDBLookupSeekRecord (UInt16 refNum, DmOpenRef dbP, UInt16 * indexP, Int16 * phoneP, Int16 offset, Int16 direction, AddressLookupFields field1, AddressLookupFields field2, AddressFields lookupFieldMap[])
{
	/*UInt16 index;
	UInt16 oldIndex;
	UInt16 count;
	UInt16 numRecords;
	MemHandle recordH;
	Boolean match;
	Int16 phone;
	Boolean searchPhones;
	PrvAddrPackedDBRecord *packedRecordP;


	globalVars* globals = getGlobalsPtr();
	ErrFatalDisplayIf ( (direction != dmSeekForward) && (direction != dmSeekBackward),
						"Bad Param");

	ErrFatalDisplayIf ( (offset < 0), "Bad param");


	index = *indexP;
	phone = *phoneP;

#ifndef CONTACTS
	searchPhones = IsPhoneLookupField(field1) || IsPhoneLookupField(field2);
#else
	searchPhones = P1ContactsIsPhoneLookupField(field1) || P1ContactsIsPhoneLookupField(field2);
#endif

	numRecords = DmNumRecords(dbP);

	if (index >= numRecords)
	{
		if (direction == dmSeekForward)
			return false;
		else
			index = numRecords - 1;
	}


	// Moving forward?
	if (direction == dmSeekForward )
		count = numRecords - index;
	else
		count = index + 1;

	// Loop through the records
	while (count--) {

		// Make sure the current record isn't hidden.  If so skip it and find the
		// next non hidden record.  Decrease the record count to search by the number
		// of records skipped.
		oldIndex = index;
		if (DmSeekRecordInCategory (dbP, &index, 0, direction, dmAllCategories))
		{
			// There are no more records.
			break;
		}
		if (index != oldIndex)
		{
			if (direction == dmSeekForward)
				count -= index - oldIndex;
			else
				count -= oldIndex - index;
		}

		recordH = DmQueryRecord(dbP, index);

		// If we have found a deleted record stop the search.
		if (!recordH)
			break;

		packedRecordP = MemHandleLock(recordH);
		if (!packedRecordP)
			goto Exit;

		//match = PrvAddrDBRecordContainsField(globals->adxtLibRef, packedRecordP, field1, &phone, direction, lookupFieldMap) &&
		//	PrvAddrDBRecordContainsField(globals->adxtLibRef, packedRecordP, field2, &phone, direction, lookupFieldMap);

		MemHandleUnlock(recordH);

		if (match)
		{
			*indexP = index;
			*phoneP = phone;
			if (offset == 0) return true;
			offset--;
		}

		// Look for another phone in this record if one was found or
		// else look at the next record.
		if (searchPhones && match)
		{
			phone += direction;
			// We their are no more phones to search so advance to next record
			if (phone == -1 || numPhoneFields <= phone)
			{
				if (direction == dmSeekForward)
					phone = 0;
				else
					phone = numPhoneFields - 1;

				index += direction;
			}
			else
			{
				// Since we are going to search this record again bump the count up
				// by one.  This loop is supposed to loop once per record to search.
				count++;
			}
		}
		else
			index += direction;

	}

	return false;

Exit:
	ErrDisplay("Err seeking rec");
*/
	return false;
}


Boolean AddrDBLookupLookupString(UInt16 refNum, DmOpenRef dbP, Char * key, UInt16 sortByCompany, AddressLookupFields field1, AddressLookupFields field2, UInt16 * recordP, Int16 * phoneP, AddressFields lookupFieldMap[], Boolean *completeMatch, Boolean *uniqueMatch)
{
	/*Int16                   numOfRecords;
	MemHandle                rH;
	univAddrPackedDBRecord*   r;
	//   UInt16                  kmin, i;                     // all positions in the database.
	UInt16                  probe, probe2         ;      // all positions in the database.
	Int16                  phoneProbe, phoneProbe2;
	//   Int16                   result;                     // result of comparing two records
	UInt16                   whichKey;
	char*                  recordKey;
	UInt16                   matches1, matches2;
#ifdef CONTACTS
	P1ContactsFields         searchField;
	P1ContactsDBRecordFlags searchFieldFlag;
#else
	AddressFields         searchField;
	AddrDBRecordFlags      searchFieldFlag;
#endif
	globalVars* globals = getGlobalsPtr();
	univAddrDBRecordType record;
	
	*uniqueMatch = false;
	*completeMatch = false;

	// If there isn't a key to search with stop the with the first record.
	if (key == NULL || *key == '\0')
	{
		*completeMatch = true;
		return false;
	}

	numOfRecords = DmNumRecords(dbP);
	if (numOfRecords == 0)
		return false;

	// Performing a lookup on the sort field allows the use a binary search which
	// takes advantage of the ordered field.
	/*if (field1 == addrLookupSortField)
	{
		// Perform the standard lookup on the sort fields looking at all categories.
		if (!AddrDBLookupString(globals->adxtLibRef, dbP, key, sortByCompany, dmAllCategories,
							  recordP, completeMatch, false))
			return false;   // nothing matched


		// At this point probe is the position where the string could be
		// inserted.  It is in between two entries.  Neither the record
		// before or after may have ANY letters in common, especially after
		// those records in other catergories are skipped.  Go with the
		// record that has the most letters in common.


		// Make sure the record returned is of the same category.
		// If not return the first prior record of the same category.
		probe2 = probe = *recordP;
		phoneProbe2 = phoneProbe = 0;
		if (AddrDBLookupSeekRecord (globals->adxtLibRef, dbP, &probe, &phoneProbe, 0, dmSeekForward,
								  field1, field2, lookupFieldMap))
		{
			// Now count the number of matching characters in probe
			rH = DmQueryRecord(dbP, probe);      // No deleted record possible
			r = (univAddrPackedDBRecord *) MemHandleLock(rH);
			ErrFatalDisplayIf(r == 0, "AddrLookup bsearch: data somehow missing");
			whichKey = 1;
			PrvAddrDBFindKey(globals->adxtLibRef, r, &recordKey, &whichKey, sortByCompany);
			if (recordKey == NULL)
				matches1 = 0;
			else
				matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);

			MemHandleUnlock(rH);
		}
		else
		{
			// No record in this category was found or probe is past all
			// records in this category.  Either way there aren't any matching
			// letters.
			matches1 = 0;
		}


		*uniqueMatch = true;


		// Sometimes the record before has more matching letters. Check it.
		// Passing DmSeekRecordInCategory an offset of 1 doesn't work
		// when probe is at the end of the database and there isn't at least
		// one record to skip.
		probe2 = probe - 1;
		if (probe == 0 ||
			!AddrDBLookupSeekRecord (globals->adxtLibRef, dbP, &probe2, &phoneProbe2, 0, dmSeekBackward,
								   field1, field2, lookupFieldMap))
		{
			// There isn't an earlier record.  Try to find a following record.
			probe2 = probe + 1;
			phoneProbe2 = phoneProbe;
			if (!AddrDBLookupSeekRecord (globals->adxtLibRef, dbP, &probe2, &phoneProbe2, 0, dmSeekForward,
									   field1, field2, lookupFieldMap))
			{
				// There isn't a following record.  Try to use the probe.
				if (matches1 > 0)
				{
					// Go with probe because they have at least some letters in common.
					*recordP = probe;   //
					*phoneP = phoneProbe;
					*completeMatch = (matches1 == StrLen(key));
					return true;
				}
				else
				{
					// probe has no letters in common and nothing earlier in this category
					// was found so this is a failed lookup.
					*completeMatch = false;
					return false;
				}
			}
		}


		// Now count the number of matching characters in probe2
		rH = DmQueryRecord(dbP, probe2);      // No deleted record possible
		r = (univAddrPackedDBRecord *) MemHandleLock(rH);
		ErrFatalDisplayIf(r == 0, "AddrLookup bsearch: data somehow missing");
		whichKey = 1;
		PrvAddrDBFindKey(globals->adxtLibRef, r, &recordKey, &whichKey, sortByCompany);
		if (recordKey == NULL)
			matches2 = 0;
		else
			matches2 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
		MemHandleUnlock(rH);


		// Now, return the probe which has the most letters in common.
		if (matches1 > matches2)
		{
			*completeMatch = (matches1 == StrLen(key));
			*recordP = probe;
			*phoneP = phoneProbe;

			// If the next item has the same number of
			// matching letters then the match is not unique.
			probe2 = probe;
			phoneProbe2 = phoneProbe;
			if (AddrDBLookupSeekRecord (globals->adxtLibRef, dbP, &probe2, &phoneProbe2, 1, dmSeekForward,
									  field1, field2, lookupFieldMap))
			{
				rH = DmQueryRecord(dbP, probe2);
				r = (univAddrPackedDBRecord *) MemHandleLock(rH);
				ErrFatalDisplayIf(r == 0, "AddrLookup bsearch: data somehow missing");

				// Compare the string to the first sort key only
				whichKey = 1;
				PrvAddrDBFindKey(globals->adxtLibRef, r, &recordKey, &whichKey, sortByCompany);

				if (recordKey == NULL)
					matches2 = 0;
				else
					matches2 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);

				MemHandleUnlock(rH);

				if (matches1 <= matches2)
				{
					*uniqueMatch = false;
				}
			}
		}
		else
			if (matches1 == 0 && matches2 == 0)
			{
				*completeMatch = false;
				*uniqueMatch = false;
				return false;            // no item with same first letter found
			}
			else
			{
				// The first item matches as much or more as the second item
				*recordP = probe2;
				*phoneP = phoneProbe2;

				// If the prior item in the category has the same number of
				// matching letters use it instead.  Repeat to find the
				// earliest such match.
				while (AddrDBLookupSeekRecord (globals->adxtLibRef, dbP, &probe2, &phoneProbe2, 1, dmSeekBackward,
											 field1, field2, lookupFieldMap))
				{
					rH = DmQueryRecord(dbP, probe2);
					r = (univAddrPackedDBRecord *) MemHandleLock(rH);
					ErrFatalDisplayIf(r == 0, "AddrLookup bsearch: data somehow missing");

					// Compare the string to the first sort key only
					whichKey = 1;
					PrvAddrDBFindKey(globals->adxtLibRef, r, &recordKey, &whichKey, sortByCompany);
					if (recordKey == NULL)
						matches1 = 0;
					else
						matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);

					MemHandleUnlock(rH);

					if (matches1 == matches2)
					{
						*recordP = probe2;
						*phoneP = phoneProbe2;
					}
					else
						break;
				}

				*completeMatch = (matches2 == StrLen(key));
				*uniqueMatch = false;
			}

		return true;

	}
	else
	{
		// Peform a lookup based on unordered data.  This gets real slow with lots of data
		// Because to check for uniqueness we must search every record.  This means on average
		// this lookup is twice as slow as it would be it it could stop with the first match.
		

		*completeMatch = false;

		matches1 = 0;         // treat this as the most matches

		// cache these values
		searchField = lookupFieldMap[field1];
		searchFieldFlag.allBits = BitAtPosition(field1);

		// Start with the first record and look at each record until there are no more.
		// Look for the record with the most number of matching records.  Even if we found
		// a record containing all the record we are searching for we must still look
		// for one more complete match to confirm or deny uniqueness of the match.
		probe2 = 0;
		phoneProbe2 = 0;
		while (AddrDBLookupSeekRecord (globals->adxtLibRef, dbP, &probe2, &phoneProbe2, 1, dmSeekForward,
									 field1, field2, lookupFieldMap))
		{
			rH = DmQueryRecord(dbP, probe2);
			r = (univAddrPackedDBRecord *) MemHandleLock(rH);
			ErrFatalDisplayIf(r == 0, "AddrLookup bsearch: data somehow missing");

			// Compare the string to the search field
			if (r->flags.allBits & searchFieldFlag.allBits)
			{
				univAddrDBUnpack(globals->adxtLibRef, r, &record);
				recordKey = record.fields[searchField];

				if (recordKey == NULL)
					matches2 = 0;
				else
					matches2 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
			}
			else
			{
				matches2 = 0;
			}

			MemHandleUnlock(rH);

			if (matches2 > matches1)
			{
				matches1 = matches2;      // the most matches so far

				*recordP = probe2;      // return the best record
				*phoneP = phoneProbe2;

				*completeMatch = (matches2 == StrLen(key));
			}
			// Did we find another record which is a complete match?
			else if (matches2 > 0 &&
					 matches1 == matches2 &&
					 *completeMatch)
			{
				*uniqueMatch = false;
				return true;
			}
			else
			{
				// The record is a matching failure.  Since AddrLookupSeekRecord is going
				// to return this record again for every phone field we cheat by specifying
				// the last phone field to skip all other entries.
				//            phoneProbe2 = numPhoneFields - 1;
			}
		}


		// Was at least one record found with at least one matching character?
		if (matches1 > 0)
		{
			// At this point every record was searched and no other match was found.
			*uniqueMatch = true;

			return true;
		}
	//}
*/
	return false;
}

/*Boolean PrvAddrDBRecordContainsField(UInt16 refNum, univPrvAddrPackedDBRecord *packedRecordP, AddressLookupFields field, Int16 * phoneP, Int16 direction, univAddressFields lookupFieldMap[])
{
	int index;
	int stopIndex;
	int phoneType;


	switch (field)
	{
	//case addrLookupSortField:
	//	return packedRecordP->flags.allBits & sortKeyFieldBits;

	case addrLookupListPhone:
#ifndef CONTACTS
		return GetBitMacro(packedRecordP->flags.allBits, firstPhoneField + packedRecordP->options.phones.displayPhoneForList);
#else
		return P1ContactsGetBitMacro(packedRecordP->flags, P1ContactsfirstPhoneField +
						   packedRecordP->options.phones.displayPhoneForList);
#endif
	case addrLookupNoField:
		return true;

	default:
#ifndef CONTACTS
		if (!IsPhoneLookupField(field))
			return GetBitMacro(packedRecordP->flags.allBits, lookupFieldMap[field]) != 0;
#else
		if (!P1ContactsIsPhoneLookupField(field))
			return P1ContactsGetBitMacro(packedRecordP->flags, lookupFieldMap[field]) != 0;
#endif

		phoneType = field - addrLookupWork;
#ifndef CONTACTS
		index = firstPhoneField + *phoneP;
		if (direction == dmSeekForward)
			stopIndex = lastPhoneField + direction;
		else
			stopIndex = firstPhoneField + direction;

		while (index != stopIndex)
		{
			// If the phone field is the type requested and it's not empty
			// return it.
			if (GetPhoneLabel(packedRecordP, index) == phoneType &&
				GetBitMacro(packedRecordP->flags.allBits, index))
			{
				*phoneP = index - firstPhoneField;
				return true;
			}
			index += direction;
		}
#else
		index = P1ContactsfirstPhoneField + *phoneP;
		if (direction == dmSeekForward)
			stopIndex = P1ContactslastPhoneField + direction;
		else
			stopIndex = P1ContactsfirstPhoneField + direction;

		while (index != stopIndex)
		{
			// If the phone field is the type requested and it's not empty
			// return it.
			if (P1ContactsGetPhoneLabel(packedRecordP, index) == phoneType &&
				P1ContactsGetBitMacro(packedRecordP->flags.allBits, index))
			{
				*phoneP = index - P1ContactsfirstPhoneField;
				return true;
			}
			index += direction;
		}
#endif

		// The phone type wasn't used.
		if (direction == dmSeekForward)
			*phoneP = 0; 						     // Reset for the next record
		else
			*phoneP = numPhoneFields - 1;      // Reset for the next record

		return false;
	}
}*/

UInt16 PrvAddrDBStrCmpMatches(UInt16 refNum, const Char* s1, const Char* s2)
{
	UInt16 matches;

	ErrFatalDisplayIf ( s1 == NULL, "Error NULL string parameter");
	ErrFatalDisplayIf ( s2 == NULL, "Error NULL string parameter");

	// In Palm OS 4.0, TxtCaselessCompare now handles null-terminated
	// strings, so the actual string lengths don't need to be passed in.
	// Instead we'll pass kMaxUInt16 (doesn't exist, use 0xFFFF).
	// On pre-4.0 ROMs, calls to StrLen(s1) & StrLen(s2) would still
	// be necessary.
	TxtCaselessCompare(s1, 0xFFFF, &matches, s2, 0xFFFF, NULL);
	return (matches);
}

Boolean AddrDBLookupString(UInt16 refNum, DmOpenRef dbP, Char * key, UInt16 sortByCompany, UInt16 category, UInt16 * recordP, Boolean *completeMatch, Boolean masked)
{
	Int16                   numOfRecords;
	MemHandle                rH;
	univPrvAddrPackedDBRecord*   r;
	UInt16                  kmin, probe, probe2, i;      // all positions in the database.
	Int16                   result;                     // result of comparing two records
	UInt16                   whichKey;
	char*                  recordKey;
	UInt16                   matches1, matches2;
	globalVars* globals = getGlobalsPtr();
	
	// If there isn't a key to search with stop the with the first record.
	if (key == NULL || *key == '\0')
	{
		*completeMatch = true;
		return false;
	}

	numOfRecords = DmNumRecords(dbP);
	if (numOfRecords == 0)
		return false;

	result = 0;
	kmin = probe = 0;
	rH = 0;


	while (numOfRecords > 0)
	{
		i = numOfRecords / 2;
		probe = kmin + i;


		// Compare the two records.  Treat deleted records as greater.
		// If the records are equal look at the following position.
		if (rH)
			MemHandleUnlock(rH);
		rH = DmQueryRecord(dbP, probe);
		if (rH == 0)
		{
			result = -1;      // Delete record is greater
		}
		else
		{
			r = (univPrvAddrPackedDBRecord *) MemHandleLock(rH);
			ErrFatalDisplayIf(r == 0, "Addr bsearch: data somehow missing");


			// Compare the string to the first sort key only
			whichKey = 1;
			PrvAddrDBFindKey(globals->adxtLibRef, r, &recordKey, &whichKey, sortByCompany);
			if (recordKey == NULL)
				result = 1;
			else
				result = StrCaselessCompare(key, recordKey);


			// If equal stop here!  We don't want the position after.
			if (result == 0)
				goto findRecordInCategory;
		}


		ErrFatalDisplayIf(result == 0, "Impossible bsearch state");

		// More likely than < 0 because of deleted records
		if (result < 0)
			numOfRecords = i;
		else
		{
			kmin = probe + 1;
			numOfRecords = numOfRecords - i - 1;
		}
	}

	if (result >= 0)
		probe++;

findRecordInCategory:
	if (rH)
		MemHandleUnlock(rH);

	// At this point probe is the position where the string could be
	// inserted.  It is in between two entries.  Neither the record
	// before or after may have ANY letters in common, especially after
	// those records in other catergories are skipped.  Go with the
	// record that has the most letters in common.


	// Make sure the record returned is of the same category.
	// If not return the first prior record of the same category.
	probe2 = probe;
	if (!PrvAddrDBSeekVisibleRecordInCategory (globals->adxtLibRef, dbP, &probe, 0, dmSeekForward, category, masked))
	{
		// Now count the number of matching characters in probe
		rH = DmQueryRecord(dbP, probe);      // No deleted record possible
		r = (univPrvAddrPackedDBRecord *) MemHandleLock(rH);
		ErrFatalDisplayIf(r == 0, "Addr bsearch: data somehow missing");
		whichKey = 1;
		PrvAddrDBFindKey(globals->adxtLibRef, r, &recordKey, &whichKey, sortByCompany);
		
		if (recordKey == NULL)
			matches1 = 0;
		else
			matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);

		MemHandleUnlock(rH);
	}
	else
	{
		// No record in this category was found or probe is past all
		// records in this category.  Either way there aren't any matching
		// letters.
		matches1 = 0;
	}



	// Sometimes the record before has more matching letters. Check it.
	// Passing DmSeekRecordInCategory an offset of 1 doesn't work
	// when probe is at the end of the database and there isn't at least
	// one record to skip.
	probe2 = probe - 1;
	if (probe == 0 ||
		PrvAddrDBSeekVisibleRecordInCategory (globals->adxtLibRef, dbP, &probe2, 0, dmSeekBackward, category, masked))
	{
		if (matches1 > 0)
		{
			// Go with probe because they have at least some letters in common.
			*recordP = probe;   //
			*completeMatch = (matches1 == StrLen(key));
			return true;
		}
		else
		{
			// probe has no letters in common and nothing earlier in this category
			// was found so this is a failed lookup.
			*completeMatch = false;
			return false;
		}
	}


	// Now count the number of matching characters in probe2
	rH = DmQueryRecord(dbP, probe2);      // No deleted record possible
	r = (univPrvAddrPackedDBRecord *) MemHandleLock(rH);
	ErrFatalDisplayIf(r == 0, "Addr bsearch: data somehow missing");
	whichKey = 1;
	PrvAddrDBFindKey(globals->adxtLibRef, r, &recordKey, &whichKey, sortByCompany);
	
	if (recordKey == NULL)
		matches2 = 0;
	else
		matches2 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
	MemHandleUnlock(rH);


	// Now, return the probe which has the most letters in common.
	if (matches1 > matches2)
	{
		*completeMatch = (matches1 == StrLen(key));
		*recordP = probe;
	}
	else
		if (matches1 == 0 && matches2 == 0)
		{
			*completeMatch = false;
			return false;            // no item with same first letter found
		}
		else
		{
			// The first item matches as much or more as the second item
			*recordP = probe2;

			// If the prior item in the category has the same number of
			// matching letters use it instead.  Repeat to find the
			// earliest such match.
			while (!PrvAddrDBSeekVisibleRecordInCategory (globals->adxtLibRef, dbP, &probe2, 1, dmSeekBackward, category, masked))
			{
				rH = DmQueryRecord(dbP, probe2);
				r = (univPrvAddrPackedDBRecord *) MemHandleLock(rH);
				ErrFatalDisplayIf(r == 0, "Addr bsearch: data somehow missing");
				whichKey = 1;
				PrvAddrDBFindKey(globals->adxtLibRef, r, &recordKey, &whichKey, sortByCompany);
				
				if (recordKey == NULL)
					matches1 = 0;
				else
					matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);

				MemHandleUnlock(rH);

				if (matches1 == matches2)
					*recordP = probe2;
				else
					break;
			}

			*completeMatch = (matches2 == StrLen(key));
		}
	return true;
}

/************************************************************
 *
 *  FUNCTION: PrvAddrDBSeekVisibleRecordInCategory
 *
 *  DESCRIPTION: Like DmSeekRecordInCategory, but if masked is true
 *						also explicitly skips past private records
 *
 *  PARAMETERS: masked - indicates that database is opened in show secret mode
 *							but should be hide secret.
 *
 *  RETURNS: as DmSeekRecordInCategory
 *
 *  CREATED: 6/15/99
 *
 *  BY: Jameson Quinn
 *
 *************************************************************/
Boolean PrvAddrDBSeekVisibleRecordInCategory (UInt16 refNum, DmOpenRef dbR, UInt16 * indexP, UInt16 offset, Int16 direction, UInt16 category, Boolean masked)
{
	UInt16		attr;
	Boolean result;

	result = DmSeekRecordInCategory(dbR,indexP,offset,direction,category);

	if (result != errNone)
	{
		goto Exit;
	}

	DmRecordInfo (dbR, *indexP, &attr, NULL, NULL);

	while (masked && (attr & dmRecAttrSecret))
	{
		result = DmSeekRecordInCategory(dbR,indexP,1,direction,category);

		if (result != errNone)
		{
			goto Exit;
		}

		DmRecordInfo (dbR, *indexP, &attr, NULL, NULL);
	}

Exit:
	return result;
}

#endif

/************************************************************
 *
 *	FUNCTION:	PrvAddrDBComparePackedRecords
 *
 *	DESCRIPTION: Compare two packed records  key by key until
 *		there is a difference.  Return -1 if r1 is less or 1 if r2
 *		is less.  A zero may be returned if two records seem
 *		identical. NULL fields are considered less than others.
 *
 *	PARAMETERS:	address record 1
 *            address record 2
 *
 *	RETURNS: -1 if record one is less
 *           1 if record two is less
 *
 *	HISTORY:
 *		01/14/95	rsf	Created by Roger Flores.
 *		11/30/00	kwk	Only call StrCompare, not StrCaselessCompare
 *							first and then StrCompare. Also use TxtCompare
 *							instead of StrCompare, to skip a trap call.
 *
 *************************************************************/
Int16 PrvAddrDBComparePackedRecords(void *r1,
												void *r2,
												Int16 sortByCompany,
												SortRecordInfoPtr UNUSED_PARAM(info1),
												SortRecordInfoPtr UNUSED_PARAM(info2),
												MemHandle UNUSED_PARAM(appInfoH))
{
	globalVars* globals = getGlobalsPtr();
	UInt16 whichKey1, whichKey2;
	char *key1, *key2;
	Int16 result;

	whichKey1 = 1;
	whichKey2 = 1;

	do {
		PrvAddrDBFindKey(globals->adxtLibRef, r1, &key1, &whichKey1, sortByCompany);
		PrvAddrDBFindKey(globals->adxtLibRef, r2, &key2, &whichKey2, sortByCompany);

		// A key with NULL loses the StrCompare.
		if (key1 == NULL)
		{
			// If both are NULL then return them as equal
			if (key2 == NULL)
			{
				result = 0;
				return result;
			}
			else
				result = -1;
		}
		else
			if (key2 == NULL)
				result = 1;
			else
			{
				// With Palm OS 4.0, StrCompare will try a caseless
				// comparison first, then a case-sensitive, so we
				// only need to call StrCompare. Also, we can call
				// TxtCompare to avoid one extra trap dispatch.
				
				// result = StrCaselessCompare(key1, key2);
				// if (result == 0)
				//		result = StrCompare(key1, key2);
				
				result = TxtCompare(	key1,		// const Char *s1,
											0xFFFF,	// UInt16 s1Len,
											NULL,		// UInt16 *s1MatchLen,
											key2,		// const Char *s2,
											0xFFFF,	// UInt16 s2Len,
											NULL);	// UInt16 *s2MatchLen
			}

	} while (!result);


	return result;
}


Boolean AddrDBLookupStringEx(DmOpenRef dbP, Char * key, UInt16 sortByCompany, UInt16 category, UInt16 * recordP, Boolean *completeMatch, Boolean masked, UInt16 iWhichKey, UInt16 startRecord, WinDirectionType searchDir)
{
	globalVars* globals = getGlobalsPtr();
	Int16                   numOfRecords;
	MemHandle                rH;
	void*   r;
	UInt16                  probe;      // all positions in the database.
	Int16                   result;                     // result of comparing two records
	UInt16                   whichKey;
	char*                  recordKey;
	UInt16                   matches1, matchesCurr;
	UInt16 matchesMax, recMax = 0, matchesSaved;

	// If there isn't a key to search with stop the with the first record.
	if (key == NULL || *key == '\0')
	{
		*completeMatch = true;
		return false;
	}

	numOfRecords = DmNumRecords(dbP);
	if (numOfRecords == 0)
		return false;

	result = 0;
	
	probe = 0;
	
	matches1 = 0;
	matchesMax = 0;
	matchesCurr = 0;
	matchesSaved = 0;
	
	//test current record
	if(globals->CurrentRecord != noRecord)
	{
		rH = DmQueryRecord(dbP, globals->CurrentRecord);      // No deleted record possible
		r = MemHandleLock(rH);
		whichKey = iWhichKey;
		PrvAddrDBFindKeyEx(r, &recordKey, 2, sortByCompany);
		
		if (recordKey == NULL)
		{
			matchesCurr = 0;
		}
		else
		{
			if(startRecord == noRecord)
			{
				matchesCurr = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
				if(matchesCurr > 32768)
					matchesCurr = 0;
			}
			else
			{
				matchesSaved = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
				if(matchesSaved > 32768)
					matchesSaved = 0;
			
			}
		}
		PrvAddrDBFindKeyEx(r, &recordKey, 1, sortByCompany);
		if (recordKey != NULL)
		{
			if(startRecord == noRecord)
			{
				matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
				if(matches1 > matchesCurr && matches1 < 32768)
					matchesCurr = matches1;
			}
			else
			{
				matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
				if(matches1 >= matchesSaved && matches1 < 32768)
					matchesSaved = matches1;
			}
		}
		
		MemHandleUnlock(rH);		
	}
	
	if(PrvAddrDBSeekVisibleRecordInCategory (globals->adxtLibRef, dbP, &probe, 0, dmSeekForward, category, masked) == errNone)
	{
		// Now count the number of matching characters in probe
		rH = DmQueryRecord(dbP, probe);      // No deleted record possible
		r = MemHandleLock(rH);
		ErrFatalDisplayIf(r == 0, "Addr bsearch: data somehow missing");
		whichKey = iWhichKey;
		PrvAddrDBFindKeyEx(r, &recordKey, 2, sortByCompany);
		
		if (recordKey == NULL)
		{
			matches1 = 0;
			*completeMatch = 0;
		}
		else
		{
			if(startRecord == noRecord)
			{
				matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
				if(matches1 > matchesMax && matches1 < 32768)
				{
					matchesMax = matches1;
					recMax = probe;
				}
			}
			else
			{
				if((searchDir == winUp && startRecord > probe && recMax <= probe) || (searchDir == winDown && startRecord < probe && recMax >= probe))
				{
					matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
					if(matches1 >= matchesSaved && matches1 < 32768)
					{
						matchesMax = matches1;
						recMax = probe;
					}
				}
			}
		}
		PrvAddrDBFindKeyEx(r, &recordKey, 1, sortByCompany);
		if (recordKey == NULL)
		{
			matches1 = 0;
			*completeMatch = 0;
		}
		else
		{
			if(startRecord == noRecord)
			{
				matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
				if(matches1 > matchesMax && matches1)
				{
					matchesMax = matches1;
					recMax = probe;
				}
			}
			else
			{
				if((searchDir == winUp && startRecord > probe && recMax <= probe) || (searchDir == winDown && startRecord < probe && recMax >= probe))
				{
					matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
					if(matches1 >= matchesSaved && matches1 < 32768)
					{
						matchesMax = matches1;
						recMax = probe;
					}
				}
			}
		}
		
		MemHandleUnlock(rH);		
	}
	
	if(searchDir == winDown && recMax == 0 && startRecord != noRecord)
		recMax = 65535;
	while(PrvAddrDBSeekVisibleRecordInCategory (globals->adxtLibRef, dbP, &probe, 1, dmSeekForward, category, masked) == errNone)
	{
		// Now count the number of matching characters in probe
		rH = DmQueryRecord(dbP, probe);      // No deleted record possible
		r = MemHandleLock(rH);
		ErrFatalDisplayIf(r == 0, "Addr bsearch: data somehow missing");
		whichKey = iWhichKey;
		PrvAddrDBFindKeyEx(r, &recordKey, 2, sortByCompany);
		
		if (recordKey == NULL)
		{
			matches1 = 0;
			*completeMatch = 0;
		}
		else
		{
			if(startRecord == noRecord)
			{
				matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
				if(matches1 > matchesMax && matches1 < 32768)
				{
					matchesMax = matches1;
					recMax = probe;
				}
			}
			else
			{
				if((searchDir == winUp && startRecord > probe && recMax <= probe) || (searchDir == winDown && startRecord < probe && recMax >= probe))
				{
					matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
					if(matches1 >= matchesSaved && matches1 < 32768)
					{
						matchesMax = matches1;
						recMax = probe;
					}
				}
			}
		}
		PrvAddrDBFindKeyEx(r, &recordKey, 1, sortByCompany);
		if (recordKey == NULL)
		{
			matches1 = 0;
			*completeMatch = 0;
		}
		else
		{
			if(startRecord == noRecord)
			{
				matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
				if(matches1 > matchesMax && matches1 < 32768)
				{
					matchesMax = matches1;
					recMax = probe;
				}
			}
			else
			{
				if((searchDir == winUp && startRecord > probe && recMax <= probe) || (searchDir == winDown && startRecord < probe && recMax >= probe))
				{
					matches1 = PrvAddrDBStrCmpMatches(globals->adxtLibRef, key, recordKey);
					if(matches1 >= matchesSaved && matches1 < 32768)
					{
						matchesMax = matches1;
						recMax = probe;
					}
				}
			}
		}
		MemHandleUnlock(rH);		
	}
	
	if(matchesMax == 0)
	{
		if(matchesSaved > 0)
		{
			*recordP = globals->CurrentRecord;
			*completeMatch = (matchesSaved == StrLen(key));
			return true;		
		}
		else
			return false;
	}
	else
	{
		if(matchesSaved > 0 && matchesMax < matchesSaved)
		{
			*recordP = globals->CurrentRecord;
			*completeMatch = (matchesCurr == StrLen(key));
			return true;		
		}
		else if(matchesSaved > 0 && matchesMax >= matchesSaved && recMax != 65535)
		{
			*recordP = recMax;
			*completeMatch = (matchesMax == StrLen(key));
			return true;
		}
		else if(matchesMax > matchesCurr)
		{
			*recordP = recMax;
			*completeMatch = (matchesMax == StrLen(key));
			return true;
		}
		else if(matchesSaved > 0)
		{
			*recordP = globals->CurrentRecord;
			*completeMatch = (matchesCurr == StrLen(key));
			return true;
		}
		else if(startRecord == noRecord && matchesMax == matchesCurr)
		{
			*recordP = globals->CurrentRecord;
			*completeMatch = (matchesMax == StrLen(key));
			return true;
		}
	}	
	return false;
}




void PrvAddrDBFindKeyEx(void *r, char **key, UInt16 fieldIndex, UInt16 sortByCompany)
{
#ifndef CONTACTS
	AddrDBRecordFlags fieldFlags;
#else
	P1ContactsDBRecordFlags fieldContactFlags;
#endif

	Int16   index;
	UInt32 flags;
	char *p=NULL;
	UInt16 i = 0;

#ifdef CONTACTS
	flags = ((PrvP1ContactsPackedDBRecord*)r)->flags.allBits;
	p = &(((PrvP1ContactsPackedDBRecord*)r)->firstField);


	fieldContactFlags.allBits = ((PrvP1ContactsPackedDBRecord*)r)->flags.allBits;


	if (sortByCompany==PreferencesCompanyName)
	{
		if (fieldContactFlags.bits.company)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnCompanyKey;
		}

		if (fieldContactFlags.bits.name)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnNameKey;
		}

		if (fieldContactFlags.bits.firstName && i < 2)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnFirstNameKey;
		}
	}
	else if(sortByCompany==PreferencesLastName)
	{
		if (fieldContactFlags.bits.name)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnNameKey;
		}

		if (fieldContactFlags.bits.firstName)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnFirstNameKey;
		}

		// For now don't consider company name when sorting by person name
		// unless there isn't a name or firstName
		if (i < 2 && fieldContactFlags.bits.company &&
			!(fieldContactFlags.bits.name || fieldContactFlags.bits.firstName))
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnCompanyKey;
		}

	}
	else if(sortByCompany==PreferencesFirstLast)
	{
		if (fieldContactFlags.bits.firstName)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnFirstNameKey;
		}

		if (fieldContactFlags.bits.name)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnNameKey;
		}

		// For now don't consider company name when sorting by person name
		// unless there isn't a name or firstName
		if (i < 2 && fieldContactFlags.bits.company &&
			!(fieldContactFlags.bits.name || fieldContactFlags.bits.firstName))
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnCompanyKey;
		}

	}
	else if(sortByCompany==PreferencesCompanyFirst)
	{
		if (fieldContactFlags.bits.company)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnCompanyKey;
		}

		if (fieldContactFlags.bits.firstName)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnFirstNameKey;
		}

		if (fieldContactFlags.bits.name && i < 2)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnNameKey;
		}
	}
	else if(sortByCompany==PreferencesCompanyTitle)
	{				

		for (index = P1ContactsfirstAddressField; index < P1Contactstitle; index++)
		{
			// If the flag is set point to the string else NULL
			if (GetBitMacro(flags, index) != 0)
			{
				p += StrLen(p) + 1;
			}
		}
		
		if (fieldContactFlags.bits.company)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnCompanyKey;
		}

		if (fieldContactFlags.bits.title)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnTitleKey;
		}

		if (fieldContactFlags.bits.name && i < 2)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnNameKey;
		}
		if (fieldContactFlags.bits.firstName && i < 1)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnFirstNameKey;
		}
	}
	else if(sortByCompany==PreferencesLastTitle)
	{
		for (index = P1ContactsfirstAddressField; index < P1Contactstitle; index++)
		{
			// If the flag is set point to the string else NULL
			if (GetBitMacro(flags, index) != 0)
			{
				p += StrLen(p) + 1;
			}
		}
		if (fieldContactFlags.bits.name)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnNameKey;
		}

		if (fieldContactFlags.bits.title)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnTitleKey;
		}

		if (fieldContactFlags.bits.company && i < 2)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnCompanyKey;
		}
		
		if (fieldContactFlags.bits.firstName && i < 2)
		{
			i++;
			if(i == fieldIndex)
				goto P1contactsreturnFirstNameKey;
		}
		
	}
	// All possible fields have been tried so return NULL so that
	// the uniq ID is compared.
	*key = NULL;
	return;



P1contactsreturnCompanyKey:
	*key = (char *) &(((PrvP1ContactsPackedDBRecord*)r)->companyFieldOffset) + ((PrvP1ContactsPackedDBRecord*)r)->companyFieldOffset;
	return;


P1contactsreturnNameKey:
	*key = &(((PrvP1ContactsPackedDBRecord*)r)->firstField);
	return;
	
P1contactsreturnTitleKey:
	*key = p;
	return;


P1contactsreturnFirstNameKey:
	*key = &(((PrvP1ContactsPackedDBRecord*)r)->firstField);
	if (((PrvP1ContactsPackedDBRecord*)r)->flags.bits.name)
	{
		*key += StrLen(*key) + 1;
	}
#else
	flags = ((PrvAddrPackedDBRecord*)r)->flags.allBits;
	p = &(((PrvAddrPackedDBRecord*)r)->firstField);


	fieldFlags.allBits = ((PrvAddrPackedDBRecord*)r)->flags.allBits;


	if (sortByCompany==PreferencesCompanyName)
	{
		if (fieldFlags.bits.company)
		{
			i++;
			if(i == fieldIndex)
				goto returnCompanyKey;
		}

		if (fieldFlags.bits.name)
		{
			i++;
			if(i == fieldIndex)
				goto returnNameKey;
		}

		if (fieldFlags.bits.firstName && i < 2)
		{
			i++;
			if(i == fieldIndex)
				goto returnFirstNameKey;
		}
	}
	else if(sortByCompany==PreferencesLastName)
	{
		if (fieldFlags.bits.name)
		{
			i++;
			if(i == fieldIndex)
				goto returnNameKey;
		}

		if (fieldFlags.bits.firstName)
		{
			i++;
			if(i == fieldIndex)
				goto returnFirstNameKey;
		}

		// For now don't consider company name when sorting by person name
		// unless there isn't a name or firstName
		if (i < 2 && fieldFlags.bits.company &&
			!(fieldFlags.bits.name || fieldFlags.bits.firstName))
		{
			i++;
			if(i == fieldIndex)
				goto returnCompanyKey;
		}

	}
	else if(sortByCompany==PreferencesFirstLast)
	{
		if (fieldFlags.bits.firstName)
		{
			i++;
			if(i == fieldIndex)
				goto returnFirstNameKey;
		}

		if (fieldFlags.bits.name)
		{
			i++;
			if(i == fieldIndex)
				goto returnNameKey;
		}

		// For now don't consider company name when sorting by person name
		// unless there isn't a name or firstName
		if (i < 2 && fieldFlags.bits.company &&
			!(fieldFlags.bits.name || fieldFlags.bits.firstName))
		{
			i++;
			if(i == fieldIndex)
				goto returnCompanyKey;
		}

	}
	else if(sortByCompany==PreferencesCompanyFirst)
	{
		if (fieldFlags.bits.company)
		{
			i++;
			if(i == fieldIndex)
				goto returnCompanyKey;
		}

		if (fieldFlags.bits.firstName)
		{
			i++;
			if(i == fieldIndex)
				goto returnFirstNameKey;
		}

		if (fieldFlags.bits.name && i < 2)
		{
			i++;
			if(i == fieldIndex)
				goto returnNameKey;
		}
	}
	else if(sortByCompany==PreferencesCompanyTitle)
	{				

		for (index = firstAddressField; index < titleField; index++)
		{
			// If the flag is set point to the string else NULL
			if (GetBitMacro(flags, index) != 0)
			{
				p += StrLen(p) + 1;
			}
		}
		
		if (fieldFlags.bits.company)
		{
			i++;
			if(i == fieldIndex)
				goto returnCompanyKey;
		}

		if (fieldFlags.bits.title)
		{
			i++;
			if(i == fieldIndex)
				goto returnTitleKey;
		}

		if (fieldFlags.bits.name && i < 2)
		{
			i++;
			if(i == fieldIndex)
				goto returnNameKey;
		}
		if (fieldFlags.bits.firstName && i < 1)
		{
			i++;
			if(i == fieldIndex)
				goto returnFirstNameKey;
		}
	}
	else if(sortByCompany==PreferencesLastTitle)
	{
		for (index = firstAddressField; index < titleField; index++)
		{
			// If the flag is set point to the string else NULL
			if (GetBitMacro(flags, index) != 0)
			{
				p += StrLen(p) + 1;
			}
		}
		if (fieldFlags.bits.name)
		{
			i++;
			if(i == fieldIndex)
				goto returnNameKey;
		}

		if (fieldFlags.bits.title)
		{
			i++;
			if(i == fieldIndex)
				goto returnTitleKey;
		}

		if (fieldFlags.bits.company && i < 2)
		{
			i++;
			if(i == fieldIndex)
				goto returnCompanyKey;
		}
		
		if (fieldFlags.bits.firstName && i < 2)
		{
			i++;
			if(i == fieldIndex)
				goto returnFirstNameKey;
		}
		
	}
	// All possible fields have been tried so return NULL so that
	// the uniq ID is compared.
	*key = NULL;
	return;



returnCompanyKey:
	*key = (char *) &((PrvAddrPackedDBRecord*)r)->companyFieldOffset + ((PrvAddrPackedDBRecord*)r)->companyFieldOffset;
	return;


returnNameKey:
	*key = &(((PrvAddrPackedDBRecord*)r)->firstField);
	return;
	
returnTitleKey:
	*key = p;
	return;


returnFirstNameKey:
	*key = &(((PrvAddrPackedDBRecord*)r)->firstField);
	if (((PrvAddrPackedDBRecord*)r)->flags.bits.name)
	{
		*key += StrLen(*key) + 1;
	}
#endif
	
	return;
}

