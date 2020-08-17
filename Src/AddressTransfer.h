#pragma once

#ifndef LIBDEBUG
#include "..\SharedLibrary\AddressXTLib\Src\AddressXTLib.h"
#endif

#include <PdiLib.h>

/***********************************************************************
 *
 *   Defines
 *
 ***********************************************************************/

#define identifierLengthMax			40
#define addrFilenameExtension		"vcf"
#define addrFilenameExtensionLength	3
#define addrMIMEType				"text/x-vCard"

// Aba: internal version of vCalendar. Must be updated
// the export side of the vCalendar code evoluate

#define kVObjectVersion						"4.0"


/************************************************************
 * Function Prototypes
 *************************************************************/

#ifdef LIBDEBUG
void	TransferRegisterData (UInt16 adxtRefNum);
Err		TransferReceiveData(UInt16 adxtRefNum, DmOpenRef dbP, ExgSocketPtr obxSocketP);
Boolean TransferImportVCard(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 pdiRefNum, PdiReaderType* reader, Boolean obeyUniqueIDs, Boolean beginAlreadyRead);
void	TransferPreview(UInt16 adxtRefNum, ExgPreviewInfoType *infoP);
void	PrvTransferCleanFileName(UInt16 adxtRefNum, Char* ioFileName);
Err		PrvTransferPdiLibLoad(UInt16 adxtRefNum, UInt16* refNum, Boolean *loadedP);
void	PrvTransferPdiLibUnload(UInt16 adxtRefNum, UInt16 refNum, Boolean loaded);
void 	PrvTransferSetGoToParams (UInt16 adxtRefNum, DmOpenRef dbP, ExgSocketPtr exgSocketP, UInt32 uniqueID);
#else
extern void		TransferRegisterData (UInt16 adxtRefNum)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapTransferRegisterData);
extern Err		TransferReceiveData(UInt16 adxtRefNum, DmOpenRef dbP, ExgSocketPtr obxSocketP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapTransferReceiveData);
extern Boolean	TransferImportVCard(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 pdiRefNum, PdiReaderType* reader, Boolean obeyUniqueIDs, Boolean beginAlreadyRead)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapTransferImportVCard);
extern void		TransferPreview(UInt16 adxtRefNum, ExgPreviewInfoType *infoP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapTransferPreview);
extern void		PrvTransferCleanFileName(UInt16 adxtRefNum, Char* ioFileName)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvTransferCleanFileName);
extern Err		PrvTransferPdiLibLoad(UInt16 adxtRefNum, UInt16* refNum, Boolean *loadedP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvTransferPdiLibLoad);
extern void		PrvTransferPdiLibUnload(UInt16 adxtRefNum, UInt16 refNum, Boolean loaded)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvTransferPdiLibUnload);
extern void		PrvTransferSetGoToParams (UInt16 adxtRefNum, DmOpenRef dbP, ExgSocketPtr exgSocketP, UInt32 uniqueID)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvTransferSetGoToParams);
#endif