#include "Address.h"
#include "ContactsDB2.h"
#include "AddressDB.h"
#include "../AddrTools.h"
#include "ContactsDefines.h"

#include <PalmOS.h>
#include <DateTime.h>

#include <UIResources.h>
#include <SysUtils.h>
#include <ErrorMgr.h>
#include <StringMgr.h>
#include <TextMgr.h>
#include <PalmUtils.h>

#include <FeatureMgr.h>
#include <AddressSortLib.h>

Boolean PrvP1ContactsDBUnpackBDOnly(PrvP1ContactsPackedDBRecord *src, P1ContactsDBRecordPtr dest);

Err PrvP1ContactsDBGetRecordBDOnly(DmOpenRef dbP, UInt16 index, P1ContactsDBRecordPtr recordP,
				  MemHandle *recordH)
{
	PrvP1ContactsPackedDBRecord *src;

	*recordH = DmQueryRecord(dbP, index);
	
	if (*recordH == NULL)
		return dmErrIndexOutOfRange;
		
	src = (PrvP1ContactsPackedDBRecord *) MemHandleLock(*recordH);
	if (src == NULL)
		return dmErrIndexOutOfRange;

	PrvP1ContactsDBUnpackBDOnly(src, recordP);

	return 0;
}

Err PrvP1ContactsDBClearBDay(DmOpenRef dbP, UInt16 *index, P1ContactsDBRecordFlags changedFields)
{
	globalVars* globals = getGlobalsPtr();
	P1ContactsDBRecordType		src;
	MemHandle          		srcH;
	Err            			result;
	MemHandle          		recordH=0;
	MemHandle          		oldH;
	P1ContactsDBRecordFlags	changes;
	
	PrvP1ContactsPackedDBRecord* 	recordP;
	UInt16				    Counter = 0;
	
	MemHandle packedH;
	
	MemMove(&changes, &changedFields, sizeof(changedFields));

	// We do not assume that r is completely valid so we get a valid
	// AddrDBRecordPtr...
	if ((result = PrvP1ContactsDBGetRecord(globals->adxtLibRef, dbP, *index, &src, &srcH)) != 0)
		return result;
	
	
	{
		//clear birthday
		MemHandle cacheRecordH;
		UInt16 i;
		DmOpenRef BDCache;
		UInt16 bdIndex;
		Err err;
		UInt32 size;
		UInt32 curID;
		//now update the ContactsDBIndex-PAdd
		DmRecordInfo(dbP, *index, 0, &curID, 0);
		BDCache = DmOpenDatabase(0, DmFindDatabase(0, "ContactsBDIndex-PAdd"), dmModeReadWrite); 
		if(BDCache)
		{
			UInt32 days = 0;
			UInt16 monthIndex;
			if(src.birthdayInfo.birthdayDate.month || src.birthdayInfo.birthdayDate.day)
			{
				for(monthIndex = 1; monthIndex < src.birthdayInfo.birthdayDate.month; monthIndex++)
				{
					days += DaysInMonth(monthIndex, src.birthdayInfo.birthdayDate.year);
				}
				days += src.birthdayInfo.birthdayDate.day;
				bdIndex = days - 1;
				//first, get a record for current birthday
				cacheRecordH = DmQueryRecord(BDCache, bdIndex);
				if(cacheRecordH)
				{
					UInt32 iNew;
					UInt32* p = (UInt32*) MemHandleLock(cacheRecordH);
					Boolean found = false;
					MemHandle changedH;
					void* newCacheRecordP;
					size = MemHandleSize(cacheRecordH);
					
					if(size/sizeof(UInt32) > 1)
					{
						for(i = 1; i < size/sizeof(UInt32); i++)
						{
							if(p[i] == curID)
							{
								found = true;
								break;
							}
						}
					}
					
					if(found)
					{
						changedH = DmNewHandle(BDCache, size - sizeof(UInt32));
						
						newCacheRecordP = (UInt32*)MemHandleLock(changedH);
						
						iNew = 0;
						for(i = 0; i < size/sizeof(UInt32); i++)
						{
							if(i == 0)
							{
								UInt32 num = p[i] - 1;
								DmWrite(newCacheRecordP, iNew*sizeof(UInt32), &num, sizeof(UInt32));
								//newCacheRecordP[iNew] = p[i] - 1;
								iNew ++;
							}
							else if(p[i] != curID)
							{
								DmWrite(newCacheRecordP, iNew*sizeof(UInt32), &p[i], sizeof(UInt32));
								//newCacheRecordP[iNew] = p[i];
								iNew++;
							}
						}
						
						MemPtrUnlock(newCacheRecordP);

						err = DmAttachRecord(BDCache, &bdIndex, changedH, &cacheRecordH);
						if (err)
							MemHandleFree(changedH);
					}					
					MemHandleUnlock(cacheRecordH);
				}
			}
			DmCloseDatabase(BDCache); 
		}
	}
	
	// Get handle to packed record, we need to figure out if there is blob data
	packedH = DmQueryRecord(dbP, *index);
	
	// And we apply the changes to it.
	
	src.birthdayInfo.birthdayDate.month = 0;
	src.birthdayInfo.birthdayDate.day = 0;
	
	// 1) and 2) (make a new chunk with the correct size)
	recordH = DmNewHandle(dbP, PrvP1ContactsDBUnpackedSize(globals->adxtLibRef, &src));
	if (recordH == NULL)
	{
		MemHandleUnlock(srcH);  // undo lock from AddrGetRecord above
		return dmErrMemError;
	}
	recordP = (PrvP1ContactsPackedDBRecord*) MemHandleLock(recordH);
	
	// 3) Copy the data from the unpacked record to the packed one.
	PrvP1ContactsDBPack(globals->adxtLibRef, &src, recordP);

	// The original record is copied and no longer needed.
	MemHandleUnlock(srcH);

	
	result = DmAttachRecord(dbP, index, recordH, &oldH);
	MemPtrUnlock(recordP);
	if (result) return result;

	MemHandleFree(oldH);
	return 0;
}

Err PrvP1ContactsDBChangeRecordCustom(DmOpenRef dbP, UInt16 *index, P1ContactsDBRecordPtr r, P1ContactsDBRecordFlags changedFields)
{
	globalVars* globals = getGlobalsPtr();
	P1ContactsDBRecordType    src;
	MemHandle             srcH;
	Err                result;
	MemHandle             recordH=0;
	MemHandle             oldH;
	Int16                i;
	UInt32             changes = changedFields.allBits;
	UInt16                attributes;
	
	PrvP1ContactsPackedDBRecord*   recordP;
	UInt16 attr;

	if ((result = PrvP1ContactsDBGetRecord(globals->adxtLibRef, dbP, *index, &src, &srcH)) != 0)
		return result;

	src.options = r->options;
	for (i = P1ContactsfirstAddressField; i < P1ContactsaddressFieldsCount; i++)
	{
			src.fields[i] = r->fields[i];
	}


	recordH = DmNewHandle(dbP, PrvP1ContactsDBUnpackedSize(globals->adxtLibRef, &src));
	if (recordH == NULL)
	{
		MemHandleUnlock(srcH);
		return dmErrMemError;
	}
	recordP = MemHandleLock(recordH);

	PrvP1ContactsDBPack(globals->adxtLibRef, &src, recordP);

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

Boolean PrvP1ContactsDBUnpackBDOnly(PrvP1ContactsPackedDBRecord *src, P1ContactsDBRecordPtr dest)
{
	Int16   				index;
	char 					*p;
 	P1ContactsDBRecordFlags flags;
	p = &src->firstField;

	MemMove(&flags, &(src->flags), sizeof(P1ContactsDBRecordFlags));
	
	MemSet(&(dest->birthdayInfo), sizeof(BirthdayInfo), 0 );
	
	if(!P1ContactsGetBitMacro(flags, P1ContactsbirthdayDate))
		return false;
	
	if(!P1ContactsGetBitMacro(flags, P1ContactsbirthdayMask))
		return false;
	
	for (index = P1ContactsfirstAddressField; index < P1ContactsaddrNumStringFields; index++)
	{
		// If the flag is set, point to the string else NULL
		if (P1ContactsGetBitMacro(flags, index) != 0)
		{
			p += StrLen(p) + 1;
		}
	}
	
	// Unpack birthday info
	if(P1ContactsGetBitMacro(flags, P1ContactsbirthdayDate))
	{
		MemMove(&(dest->birthdayInfo.birthdayDate), p, sizeof(DateType));
		p += sizeof(DateType);
	}
	
	if(P1ContactsGetBitMacro(flags, P1ContactsbirthdayMask))
	{
		MemMove(&(dest->birthdayInfo.birthdayMask), p, sizeof(AddressDBBirthdayFlags));
		p += sizeof(AddressDBBirthdayFlags);
	}
	
	if(P1ContactsGetBitMacro(flags, P1ContactsbirthdayPreset))
	{
		dest->birthdayInfo.birthdayPreset = *((UInt8*)p);		
	}
	return true;
}

