#ifndef CONTDB2_H
#define CONTDB2_H
#include "ContactsDB.h"

extern Err PrvP1ContactsDBGetRecordBDOnly(DmOpenRef dbP, UInt16 index, P1ContactsDBRecordPtr recordP,
				  MemHandle *recordH);
extern Err PrvP1ContactsDBClearBDay(DmOpenRef dbP, UInt16 *index, P1ContactsDBRecordFlags changedFields);
Err PrvP1ContactsDBChangeRecordCustom(DmOpenRef dbP, UInt16 *index, P1ContactsDBRecordPtr r, P1ContactsDBRecordFlags changedFields);
void 			ImportBirthdays();

#endif
