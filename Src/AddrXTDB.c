#ifdef LIBDEBUG //moved to shared library

#include "AddrXTDB.h"

Err AddrXTDBGetRecord(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 index, AddrXTDBRecordPtr recordP,
				  MemHandle *recordH)
{
	*recordH = DmQueryRecord(dbP, index);
	if(*recordH == NULL)
		return dmErrIndexOutOfRange;
	*recordP = *((AddrXTDBRecordPtr) MemHandleLock(*recordH));
	if (recordP == NULL)
		return dmErrIndexOutOfRange;

	return 0;
}

Err AddrXTDBNewRecord(UInt16 adxtRefNum, DmOpenRef dbP, AddrXTDBRecordPtr r)
{
	MemHandle           	 recordH;
	Err                  	 err;
	void*  					 recordP;
	UInt16                   index = dmMaxRecordIndex ;

	recordH = DmNewHandle(dbP, (Int32) sizeof(AddrXTDBRecordType));
	if (recordH == NULL)
		return dmErrMemError;

	// 3) Copy the data from the unpacked record to the packed one.
	
	recordP = MemHandleLock(recordH);
	
	
	DmWrite(recordP, 0, r, sizeof(AddrXTDBRecordType));
	
	MemPtrUnlock(recordP);

	// 4) attach in place
	err = DmAttachRecord(dbP, &index, recordH, 0);
	if (err)
		MemHandleFree(recordH);
	
	return err;
}

Err AddrXTDBChangeRecord(UInt16 adxtRefNum, DmOpenRef dbP, AddrXTDBRecordPtr r, UInt16 index)
{
	MemHandle           	 recordH, recordOldH;
	Err                  	 err;
	void*  					 recordP;
	
	recordOldH=DmQueryRecord(dbP, index);
	
	recordH = DmNewHandle(dbP, (Int32) sizeof(AddrXTDBRecordType));
	if (recordH == NULL)
		return dmErrMemError;

	// 3) Copy the data from the unpacked record to the packed one.
	
	recordP = MemHandleLock(recordH);
	
	DmWrite(recordP, 0, r, sizeof(AddrXTDBRecordType));
	
	MemPtrUnlock(recordP);

	// 4) attach in place
	err = DmAttachRecord(dbP, &index, recordH, &recordOldH);
	if (err)
		MemHandleFree(recordH);
	
	MemHandleFree(recordOldH);
	
	return err;
}

#endif