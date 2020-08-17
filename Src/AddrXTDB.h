#pragma once

#ifndef LIBDEBUG
#include "..\SharedLibrary\AddressXTLib\Src\AddressXTLib.h"
#endif

#include <PalmOS.h>
#include "globals.h"
#include "Address.h"
#include "AddrTools.h"

typedef struct {
	UInt32	id;
	UInt32	bday;
	UInt16 	remind;
	UInt16	passed;
} AddrXTDBRecordType;

typedef AddrXTDBRecordType* AddrXTDBRecordPtr;


#ifdef LIBDEBUG
Err 			AddrXTDBGetRecord(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 index, AddrXTDBRecordPtr recordP, MemHandle *recordH);
Err 			AddrXTDBNewRecord(UInt16 adxtRefNum, DmOpenRef dbP, AddrXTDBRecordPtr r);
Err 			AddrXTDBChangeRecord(UInt16 adxtRefNum, DmOpenRef dbP, AddrXTDBRecordPtr r, UInt16 index);
#else
extern Err		AddrXTDBGetRecord(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 index, AddrXTDBRecordPtr recordP, MemHandle *recordH)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrXTDBGetRecord);
extern Err		AddrXTDBNewRecord(UInt16 adxtRefNum, DmOpenRef dbP, AddrXTDBRecordPtr r)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrXTDBNewRecord);
extern Err		AddrXTDBChangeRecord(UInt16 adxtRefNum, DmOpenRef dbP, AddrXTDBRecordPtr r, UInt16 index)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrXTDBChangeRecord);
#endif