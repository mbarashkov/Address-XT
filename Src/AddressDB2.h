#pragma once

Boolean	AddrDBRecordContainsData (void* recordP);
void 	AddrDBSetFieldLabel(DmOpenRef dbP, UInt16 fieldNum, Char * fieldLabel);
Err 	AddrDBChangeRecord(DmOpenRef dbP, UInt16 *index, AddrDBRecordPtr r, AddrDBRecordFlags changedFields);
Err 	AddrDBChangeRecordCustom(DmOpenRef dbP, UInt16 *index, AddrDBRecordPtr r, AddrDBRecordFlags changedFields);