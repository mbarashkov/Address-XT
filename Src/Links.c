#include "Links.h"
#include "AddressRsc.h"
#include "../AddrTools.h"

#ifdef LIBDEBUG //moved to shared library

LocalID GetLinksLocalID(UInt16 refNum)
{
	return DmFindDatabase(0, LINKSDB_DBNAME);
}

UInt16 GetAllLinkCount(UInt16 refNum)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 numRec;
	LocalID localID = GetLinksLocalID(globals->adxtLibRef);
	DmOpenRef linksDB;
	if(localID == 0)
		return 0;
	linksDB = DmOpenDatabase(0, localID, dmModeReadOnly);
	if(linksDB == 0)
		return 0;
	numRec = DmNumRecords(linksDB);
	DmCloseDatabase(linksDB);
	return numRec;
}

UInt16 FindLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type, UInt16 * foundIndex)
{
	//check that a same link does not exist
	globalVars* globals = getGlobalsPtr();
	UInt16 recIndex = 0;
	UInt32 maxRec;
	MemHandle mH;
	Boolean found = false;
	LinkDBRecordPtr recPtr;
	if((maxRec = GetAllLinkCount(globals->adxtLibRef)) == 0)
		return 1;
	for(recIndex = 0; recIndex < maxRec; recIndex ++)
	{
		mH = DmQueryRecord(db, recIndex);
		recPtr = (LinkDBRecordPtr)MemHandleLock(mH);
		if(recPtr->type != link_address)
		{
			if(recPtr->addrID == addrID && recPtr->linkID == linkID && recPtr->type == type)
			{
				MemHandleUnlock(mH);
				found = true;
				break;
			}
			else
			{
				MemHandleUnlock(mH);
			}
		}
		else
		{
			if(recPtr->addrID == addrID && recPtr->linkID == linkID && recPtr->type == type)
			{
				MemHandleUnlock(mH);
				found = true;
				break;
			}
			else if(recPtr->addrID == linkID && recPtr->linkID == addrID && recPtr->type == type)
			{
				MemHandleUnlock(mH);
				found = true;
				break;
			}
			else
			{
				MemHandleUnlock(mH);
			}
		
		}
	}	
	if(found)
	{
		*foundIndex = recIndex;
		return 0;
	}
	else
		return 1;
}

void UpdateLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type, UInt16 updIndex)
{
	globalVars* globals = getGlobalsPtr();
	LinkDBRecordType record;
	UInt16 found;
	UInt16 recIndex = 0;
	MemHandle mH, mOldH;
	MemPtr mP;
	if(FindLink(globals->adxtLibRef, db, addrID, linkID, type, &found) == 0)
	{
		//record already exists
		return;
	}
	if(updIndex < GetAllLinkCount(globals->adxtLibRef))
		//out of range
		return;
	//actually update a record
	record.addrID = addrID;
	record.linkID = linkID;
	record.type = type;
	
	recIndex = dmMaxRecordIndex;
	mH = DmNewHandle(db, sizeof(LinkDBRecordType));
	mP = MemHandleLock(mH);
	DmWrite(mP, 0, &record, sizeof(LinkDBRecordType));
	mOldH=DmQueryRecord(db, updIndex);
	DmAttachRecord(db, &recIndex, mH, &mOldH);
	MemHandleUnlock(mH);
	MemHandleFree(mOldH);
}

UInt16 AddLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type)
{
	globalVars* globals = getGlobalsPtr();
	LinkDBRecordType record;
	UInt16 found;
	UInt16 recIndex = 0;
	MemHandle mH;
	MemPtr mP;
	if(type == link_address && addrID == linkID)
	{
		return LINK_ITSELF;
	}
	if(FindLink(globals->adxtLibRef, db, addrID, linkID, type, &found) == 0)
	{
		//record already exists
		return LINK_ALREADYEXISTS;
	}
	//actually add a record
	record.addrID = addrID;
	record.linkID = linkID;
	record.type = type;
	
	recIndex = dmMaxRecordIndex;
	mH = DmNewHandle(db, sizeof(LinkDBRecordType));
	mP = MemHandleLock(mH);
	DmWrite(mP, 0, &record, sizeof(LinkDBRecordType));
	DmAttachRecord(db, &recIndex, mH, NULL);
	MemHandleUnlock(mH);
	return LINK_NOERROR;
}

void DeleteLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 found;
	if(FindLink(globals->adxtLibRef, db, addrID, linkID, type, &found) != 0)
	{
		//record does not exists
		return;
	}
	DmRemoveRecord(db, found);
}

void DeleteOrphaned(UInt16 refNum, DmOpenRef db)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 recIndex = 0, index, attr;
	UInt32 maxRec;
	UInt32 addrID, linkID, extID;
	LinkType type;
	DmOpenRef extDB;
	MemHandle mH;
	LinkDBRecordPtr recPtr;
	if((maxRec = GetAllLinkCount(globals->adxtLibRef)) == 0)
		return;
	for(recIndex = 0; recIndex < maxRec; recIndex ++)
	{
		extDB = NULL;
		mH = DmQueryRecord(db, recIndex);
		recPtr = (LinkDBRecordPtr)MemHandleLock(mH);
		addrID = recPtr->addrID;
		linkID = recPtr->linkID;
		type = recPtr->type;
		
		MemHandleUnlock(mH);
		
		if(DmFindRecordByID(globals->AddrDB, addrID, &index)!=0)
		{
			DmRemoveRecord(db, recIndex);
			recIndex = 0;
			maxRec = GetAllLinkCount(globals->adxtLibRef);
			if(maxRec == 0)
				break;
			else
				continue;
		}
		DmRecordInfo(globals->AddrDB, index, &attr, NULL, NULL);
		if(attr & dmRecAttrDelete)
		{
			DmRemoveRecord(db, recIndex);
			recIndex = 0;
			maxRec = GetAllLinkCount(globals->adxtLibRef);
			if(maxRec == 0)
				break;
			else
				continue;
		}
		else switch(type)
		{
			case link_datebook:
				extID = DmFindDatabase(0, "DatebookDB");
				if(extID != 0)
					extDB = DmOpenDatabase(0, extID, dmModeReadOnly);
				if(extDB == 0)
					extDB = DmOpenDatabaseByTypeCreator('DATA', 'PDat', dmModeReadOnly);
				break;
			case link_memo:
				extID = DmFindDatabase(0, "MemoDB");
				if(extID != 0)
					extDB = DmOpenDatabase(0, extID, dmModeReadOnly);
				if(extDB == 0)
					extDB = DmOpenDatabaseByTypeCreator ('DATA', 'PMem', dmModeReadOnly);
				break;
			case link_todo:
				extID = DmFindDatabase(0, "ToDoDB");
				if(extID != 0)
					extDB = DmOpenDatabase(0, extID, dmModeReadOnly);
				if(extDB == 0)
					extDB = DmOpenDatabaseByTypeCreator ('DATA', 'PTod', dmModeReadOnly);
				break;
			case link_address:
				extDB = globals->AddrDB;
				break;
			default:
				break;
		}	
		if(extDB == 0)
		{
			//bad record
			DmRemoveRecord(db, recIndex);
			recIndex = 0;
			maxRec = GetAllLinkCount(globals->adxtLibRef);
			if(maxRec == 0)
				break;
			else
				continue;
		}
		else
		{
			if(linkID & 0xFF000000)
			{
				if(extDB!=globals->AddrDB)
					DmCloseDatabase(extDB);
				DmRemoveRecord(db, recIndex);
				recIndex = 0;
				maxRec = GetAllLinkCount(globals->adxtLibRef);
				if(maxRec == 0)
					break;
				else
					continue;
			}
			if(DmFindRecordByID(extDB, linkID, &index)!=0)
			{
				if(extDB!=globals->AddrDB)
					DmCloseDatabase(extDB);
				DmRemoveRecord(db, recIndex);
				recIndex = 0;
				maxRec = GetAllLinkCount(globals->adxtLibRef);
				if(maxRec == 0)
					break;
				else
					continue;
			}
			DmRecordInfo(extDB, index, &attr, NULL, NULL);
			if(attr & dmRecAttrDelete)
			{
				if(extDB!=globals->AddrDB)
					DmCloseDatabase(extDB);
				DmRemoveRecord(db, recIndex);
				recIndex = 0;
				maxRec = GetAllLinkCount(globals->adxtLibRef);
				if(maxRec == 0)
					break;
				else
					continue;
			}	
			if(extDB!=globals->AddrDB)
				DmCloseDatabase(extDB);
		}
	}
}

void DeleteAllLinks(UInt16 refNum, DmOpenRef db, UInt32 addrID)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 recIndex = 0;
	UInt32 maxRec;
	MemHandle mH;
	LinkDBRecordPtr recPtr;
	if((maxRec = GetAllLinkCount(globals->adxtLibRef)) == 0)
		return;
	for(recIndex = 0; recIndex < maxRec; recIndex ++)
	{
		mH = DmQueryRecord(db, recIndex);
		if(mH != NULL)
		{
			recPtr = (LinkDBRecordPtr)MemHandleLock(mH);
			if(recPtr->addrID == addrID)
			{
				MemHandleUnlock(mH);
				DmRemoveRecord(db, recIndex);	
			}
			else
			{
				MemHandleUnlock(mH);
			}
		}
	}
}

UInt32 GetLinkCount(UInt16 refNum, DmOpenRef db, UInt32 addrID)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 recIndex = 0;
	UInt32 maxRec;
	UInt32 res = 0;
	MemHandle mH;
	LinkDBRecordPtr recPtr;
	if((maxRec = GetAllLinkCount(globals->adxtLibRef)) == 0)
		return 0;
	for(recIndex = 0; recIndex < maxRec; recIndex ++)
	{
		mH = DmQueryRecord(db, recIndex);
		recPtr = (LinkDBRecordPtr)MemHandleLock(mH);
		if(recPtr->addrID == addrID)
		{
			res ++;
		}
		else if(recPtr->linkID == addrID && recPtr->type == link_address)
		{
			res++;
		}
		MemHandleUnlock(mH);
	}
	return res;
}

UInt16 GoToLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 link_index)
{
	globalVars* globals = getGlobalsPtr();
	LinkDBRecordType link_rec;
	UInt16 attr;
	UInt16 cardNo;
	DmSearchStateType search; 
	Err err;
	LocalID localID;
	if(GetLinkByIndex(globals->adxtLibRef, db, addrID, link_index, &link_rec)!=0)
		return 1;
	else
	{
		LocalID appToLaunch = 0, db;
		DmOpenRef dbRef;
		GoToParamsPtr paramPtr;
		UInt16 index;
		if(link_rec.type == link_address)
		{
			UInt16 index;
			if(link_rec.addrID == addrID)
			{
				if(DmFindRecordByID(globals->AddrDB, link_rec.linkID, &index)!=0)
					return 1;
			}
			else
			{
				if(DmFindRecordByID(globals->AddrDB, link_rec.addrID, &index)!=0)
					return 1;
			}
			
			DmRecordInfo(globals->AddrDB, index, &attr, NULL, NULL);
			globals->CurrentCategory = attr & dmRecAttrCategoryMask;
		
			
			if (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus != showPrivateRecords))
			{
				return ERROR_GOTOLINK_PRIVATE;	
			}
			globals->CurrentRecord = index;
			FrmGotoForm(RecordView);
			return 0;
		}
		
		paramPtr = MemPtrNew(sizeof(GoToParamsType));
		
		MemPtrSetOwner(paramPtr, 0);
		MemSet(paramPtr, sizeof(GoToParamsType), 0);
        switch(link_rec.type)
		{
			case link_datebook:
				
				err = DmGetNextDatabaseByTypeCreator(true, &search, 'appl', globals->gCalendarCrID, false, &cardNo, &localID);
				if(err == errNone)
				{
					appToLaunch = localID;	
				}
				if(err != errNone || appToLaunch == NULL)
				{
					appToLaunch = DmFindDatabase(0, "Calendar-PDat");
					if(appToLaunch == NULL)
						appToLaunch = DmFindDatabase(0, "Date Book");
				}
				db = DmFindDatabase(0, "CalendarDB-PDat");
				if(db == 0)
					db = DmFindDatabase(0, "DatebookDB");
				break;
			case link_memo:
				err = DmGetNextDatabaseByTypeCreator(true, &search, 'appl', globals->gMemosCrID, false, &cardNo, &localID);
				if(err == errNone)
				{
					appToLaunch = localID;	
				}
				if(err != errNone || appToLaunch == NULL)
				{
					appToLaunch = DmFindDatabase(0, "Memos-PMem");
					if(appToLaunch == NULL)
						appToLaunch = DmFindDatabase(0, "Memo Pad");
				}
				db = DmFindDatabase(0, "MemosDB-PMem");
				if(db == 0)
					db = DmFindDatabase(0, "MemoDB");
				break;
			case link_todo:
				err = DmGetNextDatabaseByTypeCreator(true, &search, 'appl', globals->gTasksCrID, false, &cardNo, &localID);
				if(err == errNone)
				{
					appToLaunch = localID;	
				}
				if(err != errNone || appToLaunch == NULL)
				{
					appToLaunch = DmFindDatabase(0, "Tasks-PTod");
					if(appToLaunch == NULL)
						appToLaunch = DmFindDatabase(0, "To Do List");
				}
				db = DmFindDatabase(0, "TasksDB-PTod");
				if(db == 0)
					db = DmFindDatabase(0, "ToDoDB");
				break;
			default:
				appToLaunch = NULL;
				break;
		}
		if(appToLaunch == NULL || db == NULL)
			return 1;
		
		dbRef = DmOpenDatabase(0, db, dmModeReadOnly);
		DmFindRecordByID(dbRef, link_rec.linkID, &index);
		
		DmRecordInfo(dbRef, index, &attr, NULL, NULL);
		if (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus != showPrivateRecords))
		{
			return ERROR_GOTOLINK_PRIVATE;	
		}
		
		paramPtr->searchStrLen = 0;
		paramPtr->dbCardNo = 0;
		paramPtr->dbID = db;
		paramPtr->recordNum = index;
		paramPtr->matchPos = 0;
		paramPtr->matchFieldNum = 0;
		paramPtr->matchCustom = 0;
		DmCloseDatabase(dbRef);
		SysUIAppSwitch(0, appToLaunch, sysAppLaunchCmdGoTo, (MemPtr)paramPtr);
		return 0;
	}
}

UInt16 GetLinkByIndex(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 index, LinkDBRecordPtr rec)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 recIndex = 0;
	UInt32 maxRec;
	UInt32 res = 0;
	MemHandle mH;
	LinkDBRecordPtr recPtr;
	if((maxRec = GetAllLinkCount(globals->adxtLibRef)) == 0)
		return 1;
	for(recIndex = 0; recIndex < maxRec; recIndex ++)
	{
		mH = DmQueryRecord(db, recIndex);
		recPtr = (LinkDBRecordPtr)MemHandleLock(mH);
		if(recPtr->type != link_address)
		{
			if(recPtr->addrID == addrID)
			{
				res ++;
			}
			if(res == index)
			{
				rec->addrID = recPtr->addrID;
				rec->linkID = recPtr->linkID;
				rec->type = recPtr->type;
				MemHandleUnlock(mH);
				return 0;		
			}
		}
		else
		{
			if(recPtr->addrID == addrID || recPtr->linkID == addrID)
			{
				res ++;
			}
			if(res == index)
			{
				if(recPtr->addrID == addrID)
				{
					rec->addrID = recPtr->addrID;
					rec->linkID = recPtr->linkID;
				}
				else
				{
					rec->linkID = recPtr->addrID;
					rec->addrID = recPtr->linkID;
				}
				rec->type = recPtr->type;
				MemHandleUnlock(mH);
				return 0;		
			}
		}
		MemHandleUnlock(mH);
	}
	return 1;
}

Err LinkDBChangeRecord(UInt16 refNum, DmOpenRef dbP, LinkDBRecordPtr r, UInt16 index)
{
	MemHandle           	 recordH, recordOldH;
	Err                  	 err;
	void*  					 recordP;
	
	recordOldH=DmQueryRecord(dbP, index);
	
	recordH = DmNewHandle(dbP, (Int32) sizeof(LinkDBRecordType));
	if (recordH == NULL)
		return dmErrMemError;

	// 3) Copy the data from the unpacked record to the packed one.
	
	recordP = MemHandleLock(recordH);
	
	
	DmWrite(recordP, 0, r, sizeof(LinkDBRecordType));
	
	MemPtrUnlock(recordP);

	// 4) attach in place
	err = DmAttachRecord(dbP, &index, recordH, &recordOldH);
	if (err)
		MemHandleFree(recordH);	
	
	MemHandleFree(recordOldH);
	
	return err;
}

#endif