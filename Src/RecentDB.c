#include "RecentDB.h"
#include "AddrTools.h"
#include "Address.h"

Int16 RecentCompare(void *rec1, void *rec2, Int16 other, SortRecordInfoPtr rec1SortInfo,
SortRecordInfoPtr rec2SortInfo,
MemHandle appInfoH);


Int16 RecentCompare(void *rec1, void *rec2, Int16 other, SortRecordInfoPtr rec1SortInfo,
SortRecordInfoPtr rec2SortInfo,
MemHandle appInfoH) 
{
#pragma unused(rec1, rec2, appInfoH)
	globalVars* globals = getGlobalsPtr();
	Int16 rv=0;
	UInt16 index1, index2;
	UInt32 time1, time2, id1=0, id2=0;
	RecentDBRecordType recentRecord1, recentRecord2;		
	MemHandle mH1, mH2;	
	id1=rec1SortInfo->uniqueID[0];
	id1<<=8;
	id1|=rec1SortInfo->uniqueID[1];
	id1<<=8;
	id1|=rec1SortInfo->uniqueID[2];
	
	id2=rec2SortInfo->uniqueID[0];
	id2<<=8;
	id2|=rec2SortInfo->uniqueID[1];
	id2<<=8;
	id2|=rec2SortInfo->uniqueID[2];
	
	if(DmFindRecordByID(globals->RecentDB, id1, &index1)!=0)
		return 0;
	if(DmFindRecordByID(globals->RecentDB, id2, &index2)!=0)
		return 0;
	
	if(RecentDBGetRecord(globals->adxtLibRef, globals->RecentDB, index1, &recentRecord1, &mH1) != 0)
		return 0;
	if(RecentDBGetRecord(globals->adxtLibRef, globals->RecentDB, index2, &recentRecord2, &mH2) != 0)
	{
		MemHandleUnlock(mH1);
		return 0;
	}
	
	time1 = recentRecord1.time;
	time2 = recentRecord2.time;
	MemHandleUnlock(mH1);
	MemHandleUnlock(mH2);
	
	if(time1!=time2)
	{
		if(time1>time2)
			rv=-1;
		else
			rv=1;
	}	
	
	return rv*other;
}

#ifdef LIBDEBUG //moved to shared library
UInt32 SeekRecentInCategory(UInt16 refNum, UInt16 category, UInt16* index)
{
	globalVars* globals = getGlobalsPtr();
	RecentDBRecordType rec;		
	MemHandle mH;	
	UInt16 numRecent=0;
	LocalID recentDbId;
	UInt16 counter=0;
	UInt16 cnt2=0;
	Boolean masked=(globals->PrivateRecordVisualStatus != showPrivateRecords);
	if(*index>AddrDBCountRecentInCategory(globals->adxtLibRef, category))
	{
		return 0;
	}
	recentDbId=DmFindDatabase(0, RECENTDB_DBNAME);
	if(recentDbId==0)
	{
		return 0;
	}
	if(globals->RecentDB==NULL)
		return 0;
	numRecent = DmNumRecords(globals->RecentDB);
	if(numRecent<=*index)
		return 0;
	for(counter=0;counter<numRecent;counter++)
	{
		UInt32 id;
		UInt16 addrIndex, attr;
		
		if(RecentDBGetRecord(globals->adxtLibRef, globals->RecentDB, counter, &rec, &mH) != 0)
			continue;
		
		id = rec.id;
		
		MemHandleUnlock(mH);
		
		DmFindRecordByID(globals->AddrDB, id, &addrIndex);
		DmRecordInfo(globals->AddrDB, addrIndex, &attr, NULL, NULL);
		if(((attr & dmRecAttrCategoryMask) == category) || (category==dmAllCategories))
		{
			Boolean masked = (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == maskPrivateRecords));
			if(masked)
				continue;
			if(cnt2==*index)
			{
				*index=addrIndex;
				return id;
			}
			cnt2++;
		} 	
	}
	return 0;
}

UInt16 AddrDBCountRecentInCategory(UInt16 refNum, UInt16 category)
{
	globalVars* globals = getGlobalsPtr();
	UInt32 index;
	UInt16 numRecent=0;
	LocalID recentDbId;
	RecentDBRecordType rec;		
	MemHandle mH;	
	UInt16 rv=0;
	Boolean masked=(globals->PrivateRecordVisualStatus != showPrivateRecords);
	recentDbId=DmFindDatabase(0, RECENTDB_DBNAME);
	if(recentDbId==0)
	{
		return rv;
	}
	if(globals->RecentDB==NULL)
		return rv;
	numRecent = DmNumRecords(globals->RecentDB);
	if(numRecent==0)
		return rv;
	if(category==dmAllCategories)
		return numRecent;
	
	for(index=0;index<numRecent;index++)
	{
		UInt32 id;
		UInt16 addrIndex, attr;
		
		if(RecentDBGetRecord(globals->adxtLibRef, globals->RecentDB, index, &rec, &mH) != 0)
			continue;
		
		id = rec.id;
		
		MemHandleUnlock(mH);
		DmFindRecordByID(globals->AddrDB, id, &addrIndex);
		DmRecordInfo(globals->AddrDB, addrIndex, &attr, NULL, NULL);
		if((attr & dmRecAttrCategoryMask) == category)
		{
			Boolean masked = (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == maskPrivateRecords));
			if(!masked)
				rv++;
		} 	
	}
	return rv;	
}

/************************************************************
 *
 *  FUNCTION: CleanRecentDB
 *
 *  DESCRIPTION: Clean up RecentDB (delete links to records that dont exist) 
 *
 *  PARAMETERS:  Max number of record in RecentDB
 *
 *  RETURNS: nothing
 *
 * HISTORY:
 *		04/10/04	MB Initial revision
 *
 *************************************************************/
void CleanRecentDB(UInt16 refNum)
{
	globalVars* globals = getGlobalsPtr();
	RecentDBRecordType rec;		
	MemHandle mH;	
	UInt32 id;
	UInt16 index, attr; Int16 counter;
	UInt16 numRecent;
	LocalID recentDbId=DmFindDatabase(0, RECENTDB_DBNAME);
	if(recentDbId==0)
	{
		return;
	}
	if(globals->RecentDB==NULL)
		return;
	numRecent = DmNumRecords(globals->RecentDB);
	if(numRecent==0)
	{
		return;
	}
	else
	{
		//delete records that dont exist
		for(counter=0;counter<numRecent;counter++)
		{
			if(RecentDBGetRecord(globals->adxtLibRef, globals->RecentDB, counter, &rec, &mH) != 0)
				continue;
		
			id = rec.id;
			
			MemHandleUnlock(mH);
			
			if(DmFindRecordByID(globals->AddrDB, id, &index)!=0)
			{
				DmRemoveRecord(globals->RecentDB,counter);
				numRecent = DmNumRecords(globals->RecentDB);
				counter=-1;
			} 	
			else
			{
				DmRecordInfo(globals->AddrDB, index, &attr, NULL, NULL);
				if(attr & dmRecAttrDelete)
				{
					DmRemoveRecord(globals->RecentDB,counter);
					numRecent = DmNumRecords(globals->RecentDB);
					counter=-1;
				}
			}
		}
	} 	
}

/************************************************************
 *
 *  FUNCTION: TrimRecentDB
 *
 *  DESCRIPTION: Trims RecentDB 
 *
 *  PARAMETERS:  Max number of record in RecentDB
 *
 *  RETURNS: nothing
 *
 * HISTORY:
 *		04/10/04	MB Initial revision
 *
 *************************************************************/
void TrimRecentDB(UInt16 refNum, UInt16 maxRecords)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 numRecent=0;UInt16 index;
	LocalID recentDbId=DmFindDatabase(0, RECENTDB_DBNAME);
	if(recentDbId==0)
	{
		return;
	}
	if(globals->RecentDB==NULL)
		return;
	numRecent = DmNumRecords(globals->RecentDB);
	if(numRecent<maxRecords+1)
	{
		index=dmMaxRecordIndex;
	}
	else
	{
		//sort records in descending order and delete last record
		DmQuickSort(globals->RecentDB, RecentCompare, sort_descending);
		while(!(numRecent<maxRecords+1))
		{
			DmRemoveRecord(globals->RecentDB, numRecent-1);
			numRecent = DmNumRecords(globals->RecentDB);
		}
		index=maxRecords;
	} 	
}

Err RecentDBGetRecord(UInt16 refNum, DmOpenRef dbP, UInt16 index, RecentDBRecordPtr recordP,
				  MemHandle *recordH)
{
	*recordH = DmQueryRecord(dbP, index);
	if(*recordH == NULL)
		return dmErrIndexOutOfRange;
	*recordP = *((RecentDBRecordPtr) MemHandleLock(*recordH));
	if (recordP == NULL)
		return dmErrIndexOutOfRange;

	return 0;
}

/************************************************************
 *
 *  FUNCTION: AddrDBAddToRecent
 *
 *  DESCRIPTION: Adds the record with given index to the
 *				 RecentDB
 *
 *  PARAMETERS:  Record index
 *
 *  RETURNS: nothing
 *
 * HISTORY:
 *		03/10/04	MB Initial revision
 *
 *************************************************************/
void AddrDBAddToRecent(UInt16 refNum, DmOpenRef addrDB, UInt16 record)
{
	globalVars* globals = getGlobalsPtr();
	RecentDBRecordType rec;		
	MemHandle mH;	
	UInt32 id;
	UInt16 numRecent=0;
	UInt16 counter;
	Boolean found=false;
	LocalID recentDbId=DmFindDatabase(0, RECENTDB_DBNAME);
	if(globals->gMaxRecent == 0)
		return;
	if(DmRecordInfo(addrDB, record, NULL, &id, NULL)!=errNone)
		return;
	if(recentDbId==0)
	{
		return;
	}
	if(globals->RecentDB==NULL)
		return;
	numRecent = DmNumRecords(globals->RecentDB);
	for(counter=0;counter<numRecent;counter++)
	{
		
		if(RecentDBGetRecord(globals->adxtLibRef, globals->RecentDB, counter, &rec, &mH) != 0)
			continue;
		
		if(id == rec.id)
		{
			found=true;
			break;
		}
		else
		{
			MemHandleUnlock(mH);
		}
	}
	
	if(!found)
		TrimRecentDB(globals->adxtLibRef, globals->gMaxRecent-1);
	
	rec.id = id;
	rec.time=TimGetSeconds();
	if(!found)
		RecentDBNewRecord(globals->adxtLibRef, globals->RecentDB, &rec);
	else
	{
		MemHandleUnlock(mH);
		RecentDBChangeRecord(globals->adxtLibRef, globals->RecentDB, &rec, counter);
	}
	DmQuickSort(globals->RecentDB, RecentCompare, sort_descending);
			
}

Err RecentDBNewRecord(UInt16 refNum, DmOpenRef dbP, RecentDBRecordPtr r)
{
	MemHandle           	 recordH;
	Err                  	 err;
	void*  					 recordP;
	UInt16                   index = dmMaxRecordIndex ;

	recordH = DmNewHandle(dbP, (Int32) sizeof(RecentDBRecordType));
	if (recordH == NULL)
		return dmErrMemError;

	// 3) Copy the data from the unpacked record to the packed one.
	
	recordP = MemHandleLock(recordH);
	
	
	DmWrite(recordP, 0, r, sizeof(RecentDBRecordType));
	
	MemPtrUnlock(recordP);

	// 4) attach in place
	err = DmAttachRecord(dbP, &index, recordH, 0);
	if (err)
		MemHandleFree(recordH);
	
	return err;
}

Err RecentDBChangeRecord(UInt16 refNum, DmOpenRef dbP, RecentDBRecordPtr r, UInt16 index)
{
	MemHandle           	 recordH, recordOldH;
	Err                  	 err;
	void*  					 recordP;
	
	recordOldH=DmQueryRecord(dbP, index);
	
	recordH = DmNewHandle(dbP, (Int32) sizeof(RecentDBRecordType));
	if (recordH == NULL)
		return dmErrMemError;

	// 3) Copy the data from the unpacked record to the packed one.
	
	recordP = MemHandleLock(recordH);
	
	
	DmWrite(recordP, 0, r, sizeof(RecentDBRecordType));
	
	MemPtrUnlock(recordP);

	// 4) attach in place
	err = DmAttachRecord(dbP, &index, recordH, &recordOldH);
	if (err)
		MemHandleFree(recordH);
	
	
	//MemHandleUnlock(recordH);
	MemHandleFree(recordOldH);
	
	return err;
}
#endif
