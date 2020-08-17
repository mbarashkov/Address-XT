#pragma once

#include <IMCUtils.h>
#include <ExgMgr.h>

#include "ToDoDB.h"

/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/
#define toDoVersionNum					3
#define toDoPrefsVersionNum			3
#define todoPrefID						0x00
#define toDoDBName						"ToDoDB"
#define toDoDBType						'DATA'


/************************************************************
 * Function Prototypes
 *************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


// From ToDoTransfer.c
typedef Err ImportVEventF(DmOpenRef dbP, void * inputStream, GetCharF inputFunc, 
	Boolean obeyUniqueIDs, Boolean beginAlreadyRead);
 

extern void ToDoSendRecord (DmOpenRef dbP, Int16 recordNum);
extern void ToDoSendCategory (DmOpenRef dbP, UInt16 categoryNum);
extern Err ToDoReceiveData(DmOpenRef dbP, ExgSocketPtr exgSocketP);
extern Boolean ToDoImportVToDo(DmOpenRef dbP, void * inputStream, GetCharF inputFunc, 
	Boolean obeyUniqueIDs, Boolean beginAlreadyRead, UInt32 * uniqueIDP);
extern Boolean ToDoImportVCal(DmOpenRef dbP, void * inputStream, GetCharF inputFunc, 
	Boolean obeyUniqueIDs, Boolean beginAlreadyRead, ImportVEventF vEventFunc);
extern void ToDoExportVCal(DmOpenRef dbP, Int16 index, ToDoDBRecordPtr recordP, 
	void * outputStream, PutStringF outputFunc, Boolean writeUniqueIDs);
	
extern void SetDBBackupBit(DmOpenRef dbP);
extern Boolean ToDoListViewHandleEvent (EventPtr event);

Err ToDoListGetLabel(UInt32 id, MemHandle *textHP);

#ifdef __cplusplus 
}
#endif

