#pragma once

#include "AddressTransfer.h"
#include <PdiLib.h>


/************************************************************
 * Function Prototypes
 *************************************************************/

void	TransferSendRecord (DmOpenRef dbP, Int16 recordNum, const Char * const prefix, UInt16 noDataAlertID);
void	TransferSendCategory (DmOpenRef dbP, UInt16 categoryNum, const Char * const prefix, UInt16 noDataAlertID);
void 	TransferExportVCard(DmOpenRef dbP, Int16 index, univAddrDBRecordType* recordP, UInt16 pdiRefNum, PdiWriterType* writer, Boolean writeUniqueIDs);
