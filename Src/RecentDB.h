#pragma once

#include <PalmOS.h>
#include "globals.h"
#include "AddrTools.h"

typedef struct {
	UInt32	id;
	UInt32	time;
} RecentDBRecordType;

typedef RecentDBRecordType* RecentDBRecordPtr;


#ifdef LIBDEBUG
void			AddrDBAddToRecent(UInt16 refNum, DmOpenRef addrDB, UInt16 record);
UInt16			AddrDBCountRecentInCategory(UInt16 refNum, UInt16 category);
void 			TrimRecentDB(UInt16 refNum, UInt16 maxRecords);
UInt32			SeekRecentInCategory(UInt16 refNum, UInt16 category, UInt16* index);
void			CleanRecentDB(UInt16 refNum);
Err 			RecentDBGetRecord(UInt16 refNum, DmOpenRef dbP, UInt16 index, RecentDBRecordPtr recordP, MemHandle *recordH);
Err 			RecentDBNewRecord(UInt16 refNum, DmOpenRef dbP, RecentDBRecordPtr r);
Err 			RecentDBChangeRecord(UInt16 refNum, DmOpenRef dbP, RecentDBRecordPtr r, UInt16 index);
#else
extern void		AddrDBAddToRecent(UInt16 refNum, DmOpenRef addrDB, UInt16 record)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBAddToRecent);
extern UInt16	AddrDBCountRecentInCategory(UInt16 refNum, UInt16 category)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBCountRecentInCategory);
extern void		TrimRecentDB(UInt16 refNum, UInt16 maxRecords)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapTrimRecentDB);
extern UInt32	SeekRecentInCategory(UInt16 refNum, UInt16 category, UInt16* index)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapSeekRecentInCategory);
extern void		CleanRecentDB(UInt16 refNum)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapCleanRecentDB);
extern Err		RecentDBGetRecord(UInt16 refNum, DmOpenRef dbP, UInt16 index, RecentDBRecordPtr recordP, MemHandle *recordH)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapRecentDBGetRecord);
extern Err		RecentDBNewRecord(UInt16 refNum, DmOpenRef dbP, RecentDBRecordPtr r)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapRecentDBNewRecord);
extern Err		RecentDBChangeRecord(UInt16 refNum, DmOpenRef dbP, RecentDBRecordPtr r, UInt16 index)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapRecentDBChangeRecord);
#endif
