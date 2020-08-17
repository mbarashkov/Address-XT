/*
 * AddressXTLibDispatch.c
 *
 * dispatch table for AddressXTLib shared library
 *
 * This wizard-generated code is based on code adapted from the
 * SampleLib project distributed as part of the Palm OS SDK 4.0.
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */
 
/* Our library public definitions (library API) */
#define BUILDING_ADDRESSXTLIB
#include "AddressXTLib.h"
#include "..\\..\\..\\Src\AddressDB.h"
#include "..\\..\\..\\Src\AddrXTDB.h"
#include "..\\..\\..\\Src\Links.h"
#include "..\\..\\..\\Src\RecentDB.h"
#include "..\\..\\..\\Src\AddressTransfer.h"

/* Local prototypes */
Err AddressXTLibInstall(UInt16 refNum, SysLibTblEntryType *entryP);
static MemPtr AddressXTLibDispatchTable(void);


Err AddressXTLibInstall(UInt16 refNum, SysLibTblEntryType *entryP)
{
	#pragma unused(refNum)

	/* Install pointer to our dispatch table */
	entryP->dispatchTblP = (MemPtr *) AddressXTLibDispatchTable();
	
	/* Initialize globals pointer to zero (we will set up our
	 * library globals in the library "open" call). */
	entryP->globalsP = 0;

	return 0;
}

/* Palm OS uses short jumps */
#define prvJmpSize		4

/* Now, define a macro for offset list entries */
#define libDispatchEntry(index)		(kOffset+((index)*prvJmpSize))

/* Finally, define the size of the dispatch table's offset list --
 * it is equal to the size of each entry (which is presently 2
 * bytes) times the number of entries in the offset list (including
 * the @Name entry). */
#define numberOfAPIs        (96)
#define entrySize           (2)
#define	kOffset             (entrySize * (5 + numberOfAPIs))

static MemPtr asm AddressXTLibDispatchTable(void)
{
	LEA		@Table, A0			/* table ptr */
	RTS							/* exit with it */

@Table:
	/* Offset to library name */
	DC.W		@Name
	
	/*
	 * Library function dispatch entries
	 *
	 * ***IMPORTANT***
	 * The index parameters passed to the macro libDispatchEntry
	 * must be numbered consecutively, beginning with zero.
	 *
	 * The hard-wired values need to be used for offsets because
	 * CodeWarrior's inline assembler doesn't support label subtraction.
	 */
	
    /* Standard API entry points */
	DC.W		libDispatchEntry(0)			    /* AddressXTLibOpen */
	DC.W		libDispatchEntry(1)		    	/* AddressXTLibClose */
	DC.W		libDispatchEntry(2)	    		/* AddressXTLibSleep */
	DC.W		libDispatchEntry(3) 			/* AddressXTLibWake */

    /* Custom API entry points */
//AddressDB.c
	DC.W		libDispatchEntry(4) 			/* UpdateAlarms */
	DC.W		libDispatchEntry(5) 			/* reserved */
	DC.W		libDispatchEntry(6) 			/* AddrDBAppInfoGetPtr */
	DC.W		libDispatchEntry(7) 			/* AddrDBChangeSortOrder */
	DC.W		libDispatchEntry(8) 			/* AddrDBLookupSeekRecord */
	DC.W		libDispatchEntry(9) 			/* AddrDBLookupString */
	DC.W		libDispatchEntry(10) 			/* AddrDBLookupLookupString */
	DC.W		libDispatchEntry(11) 			/* AddrDBGetDatabase */
	DC.W		libDispatchEntry(12) 			/* PrvAddrDBFindSortPosition */
	DC.W		libDispatchEntry(13) 			/* PrvAddrDBFindKey */
	DC.W		libDispatchEntry(14) 			/* AddrDBAppInfoInit */
	DC.W		libDispatchEntry(15) 			/* AddrDBNewRecord */
	DC.W		libDispatchEntry(16) 			/* AddrDBGetRecord */
	DC.W		libDispatchEntry(17) 			/* PrvAddrDBUnpackedSize */
	DC.W		libDispatchEntry(18) 			/* PrvAddrDBPack */
	DC.W		libDispatchEntry(19) 			/* PrvAddrDBUnpack */
	DC.W		libDispatchEntry(20) 			/* AddrDBChangeCountry */
	DC.W		libDispatchEntry(21) 			/* PrvAddrDBStrCmpMatches */
	DC.W		libDispatchEntry(22) 			/* PrvAddrDBSeekVisibleRecordInCategory */
	DC.W		libDispatchEntry(23) 			/* PrvAddrDBLocalizeAppInfo */
//addrtools.c
	DC.W		libDispatchEntry(24) 			/* TxtFindStringEx */
	DC.W		libDispatchEntry(25) 			/* GetSortByCompany */
	DC.W		libDispatchEntry(26) 			/* ToolsSeekRecordEx */
	DC.W		libDispatchEntry(27) 			/* deleteRecord */
	DC.W		libDispatchEntry(28) 			/* CstOpenOrCreateDB */
	DC.W		libDispatchEntry(29) 			/* ToolsWinDrawCharsHD */
	DC.W		libDispatchEntry(30) 			/* ToolsSetDBAttrBits */
	DC.W		libDispatchEntry(31) 			/* ToolsDetermineRecordName */
	DC.W		libDispatchEntry(32) 			/* ToolsDrawRecordName */
	DC.W		libDispatchEntry(33) 			/* ToolsDrawRecordNameAndPhoneNumber */
	DC.W		libDispatchEntry(34) 			/* ToolsGetLabelColumnWidth */
	DC.W		libDispatchEntry(35) 			/* ToolsSeekRecord */
	DC.W		libDispatchEntry(36) 			/* ToolsCustomAcceptBeamDialog */
	DC.W		libDispatchEntry(37) 			/* ToolsInitPhoneLabelLetters */
	DC.W		libDispatchEntry(38) 			/* ToolsGetStringResource */
	DC.W		libDispatchEntry(39) 			/* ToolsGetLineIndexAtOffset */
	DC.W		libDispatchEntry(40) 			/* ToolsUnivWinDrawCharsHD */
	DC.W		libDispatchEntry(41) 			/* ToolsEnableBluetooth */
	DC.W		libDispatchEntry(42) 			/* ToolsFntSetFont */
	DC.W		libDispatchEntry(43) 			/* ToolsGetSortInfo */
	DC.W		libDispatchEntry(44) 			/* FrmGlueNavObjectTakeFocus */
	DC.W		libDispatchEntry(45) 			/* FrmGlueNavDrawFocusRing */
	DC.W		libDispatchEntry(46) 			/* FrmGlueNavRemoveFocusRing */
	DC.W		libDispatchEntry(47) 			/* FrmGlueNavGetFocusRingInfo */
	DC.W		libDispatchEntry(48) 			/* ToolsIsFiveWayNavPalmEvent */
	DC.W		libDispatchEntry(49) 			/* ToolsIsFiveWayNavEvent */
	DC.W		libDispatchEntry(50) 			/* ToolsNavSelectHSPressed */
	DC.W		libDispatchEntry(51) 			/* ToolsNavSelectPalmPressed */
	DC.W		libDispatchEntry(52) 			/* ToolsNavSelectPressed */
	DC.W		libDispatchEntry(53) 			/* ToolsNavDirectionHSPressed */
	DC.W		libDispatchEntry(54) 			/* ToolsNavDirectionPalmPressed */
	DC.W		libDispatchEntry(55) 			/* ToolsNavDirectionPressed */
	DC.W		libDispatchEntry(56) 			/* ToolsNavKeyPressed */
	DC.W		libDispatchEntry(57) 			/* PrvToolsPhoneIsANumber */
	DC.W		libDispatchEntry(58) 			/* GetDelimiterStr */
	DC.W		libDispatchEntry(59) 			/* GetDelimiterLen */
//addrtransfer.c
	DC.W		libDispatchEntry(60) 			/* TransferRegisterData */
	DC.W		libDispatchEntry(61) 			/* TransferReceiveData */
	DC.W		libDispatchEntry(62) 			/* TransferImportVCard */
	DC.W		libDispatchEntry(63) 			/* TransferPreview */
	DC.W		libDispatchEntry(64) 			/* PrvTransferCleanFileName */
	DC.W		libDispatchEntry(65) 			/* PrvTransferPdiLibLoad */
	DC.W		libDispatchEntry(66) 			/* PrvTransferPdiLibUnload */
	DC.W		libDispatchEntry(67) 			/* PrvTransferSetGoToParams */
//ContactsDB.c
	DC.W		libDispatchEntry(68) 			/* PrvP1ContactsDBChangeRecord */
	DC.W		libDispatchEntry(69) 			/* PrvP1ContactsDBUnpackedSize */
	DC.W		libDispatchEntry(70) 			/* PrvP1ContactsDBUnpack */
	DC.W		libDispatchEntry(71) 			/* PrvP1ContactsDBPack */
	DC.W		libDispatchEntry(72) 			/* P1ContactsDBAppInfoGetPtr */
	DC.W		libDispatchEntry(73) 			/* P1ContactsDBNewRecord */
	DC.W		libDispatchEntry(74) 			/* PrvP1ContactsDBGetRecord */
//AddrXTDB.c
	DC.W		libDispatchEntry(75) 			/* AddrXTDBGetRecord */
	DC.W		libDispatchEntry(76) 			/* AddrXTDBNewRecord */
	DC.W		libDispatchEntry(77) 			/* AddrXTDBChangeRecord */
//AddrTools.c
	DC.W		libDispatchEntry(78) 			/* LoadColors */
//RecentDB.c
	DC.W		libDispatchEntry(79) 			/* AddrDBAddToRecent */
	DC.W		libDispatchEntry(80) 			/* AddrDBCountRecentInCategory */
	DC.W		libDispatchEntry(81) 			/* TrimRecentDB */
	DC.W		libDispatchEntry(82) 			/* SeekRecentInCategory */
	DC.W		libDispatchEntry(83) 			/* CleanRecentDB */
	DC.W		libDispatchEntry(84) 			/* RecentDBGetRecord */
	DC.W		libDispatchEntry(85) 			/* RecentDBNewRecord */
	DC.W		libDispatchEntry(86) 			/* RecentDBChangeRecord */
//Links.c
	DC.W		libDispatchEntry(87) 			/* GetLinkCount */
	DC.W		libDispatchEntry(88) 			/* FindLink */
	DC.W		libDispatchEntry(89) 			/* UpdateLink */
	DC.W		libDispatchEntry(90) 			/* AddLink */
	DC.W		libDispatchEntry(91) 			/* DeleteLink */
	DC.W		libDispatchEntry(92) 			/* DeleteOrphaned */
	DC.W		libDispatchEntry(93) 			/* DeleteAllLinks */
	DC.W		libDispatchEntry(94) 			/* GetLinkByIndex */
	DC.W		libDispatchEntry(95) 			/* GoToLink */
	DC.W		libDispatchEntry(96) 			/* LinkDBChangeRecord */
	DC.W		libDispatchEntry(97) 			/* GetLinksLocalID */
	DC.W		libDispatchEntry(98) 			/* GetAllLinkCount */
//Tools.c
	DC.W		libDispatchEntry(99) 			/* PrvDetermineRecordNameHelper */
	
    /* Standard API entry points */
	JMP         AddressXTLibOpen
	JMP         AddressXTLibClose
	JMP         AddressXTLibSleep
	JMP         AddressXTLibWake

    /* Custom API entry points */
//AddressDB.c
	JMP 		UpdateAlarms	
	JMP 		UpdateAlarms /*this is reserved*/	
	JMP 		AddrDBAppInfoGetPtr	
	JMP			AddrDBChangeSortOrder
	JMP			AddrDBLookupSeekRecord
	JMP			AddrDBLookupString
	JMP			AddrDBLookupLookupString
	JMP			AddrDBGetDatabase
	JMP			PrvAddrDBFindSortPosition
	JMP			PrvAddrDBFindKey
	JMP			AddrDBAppInfoInit
	JMP			AddrDBNewRecord
	JMP			AddrDBGetRecord
	JMP			PrvAddrDBUnpackedSize
	JMP			PrvAddrDBPack
	JMP			PrvAddrDBUnpack
	JMP			AddrDBChangeCountry
	JMP			PrvAddrDBStrCmpMatches
	JMP			PrvAddrDBSeekVisibleRecordInCategory
	JMP			PrvAddrDBLocalizeAppInfo
//addrtools.c
	JMP 		TxtFindStringEx	
	JMP 		GetSortByCompany	
	JMP 		ToolsSeekRecordEx	
	JMP			deleteRecord
	JMP			CstOpenOrCreateDB
	JMP			ToolsWinDrawCharsHD
	JMP			ToolsSetDBAttrBits
	JMP			ToolsDetermineRecordName
	JMP			ToolsDrawRecordName
	JMP			ToolsDrawRecordNameAndPhoneNumber
	JMP			ToolsGetLabelColumnWidth
	JMP			ToolsSeekRecord
	JMP			ToolsCustomAcceptBeamDialog
	JMP			ToolsInitPhoneLabelLetters
	JMP			ToolsGetStringResource
	JMP			ToolsGetLineIndexAtOffset
	JMP			ToolsUnivWinDrawCharsHD
	JMP			ToolsEnableBluetooth
	JMP			ToolsFntSetFont
	JMP			ToolsGetSortInfo
	JMP			FrmGlueNavObjectTakeFocus
	JMP			FrmGlueNavDrawFocusRing
	JMP			FrmGlueNavRemoveFocusRing
	JMP			FrmGlueNavGetFocusRingInfo
	JMP			ToolsIsFiveWayNavPalmEvent
	JMP			ToolsIsFiveWayNavEvent
	JMP			ToolsNavSelectHSPressed
	JMP			ToolsNavSelectPalmPressed
	JMP			ToolsNavSelectPressed
	JMP			ToolsNavDirectionHSPressed
	JMP			ToolsNavDirectionPalmPressed
	JMP			ToolsNavDirectionPressed
	JMP			ToolsNavKeyPressed
	JMP			PrvToolsPhoneIsANumber
	JMP			GetDelimiterStr
	JMP			GetDelimiterLen
//addrtransfer.c
	JMP			TransferRegisterData
	JMP			TransferReceiveData
	JMP			TransferImportVCard
	JMP			TransferPreview
	JMP			PrvTransferCleanFileName
	JMP			PrvTransferPdiLibLoad
	JMP			PrvTransferPdiLibUnload
	JMP			PrvTransferSetGoToParams
//ContactsDB.c
	JMP			PrvP1ContactsDBChangeRecord
	JMP			PrvP1ContactsDBUnpackedSize
	JMP			PrvP1ContactsDBUnpack
	JMP			PrvP1ContactsDBPack
	JMP			P1ContactsDBAppInfoGetPtr
	JMP			P1ContactsDBNewRecord
	JMP			PrvP1ContactsDBGetRecord
//AddrXTDB.c
	JMP			AddrXTDBGetRecord
	JMP			AddrXTDBNewRecord
	JMP			AddrXTDBChangeRecord
//AddrTools.c
	JMP			LoadColors
//RecentDB.c
	JMP			AddrDBAddToRecent
	JMP			AddrDBCountRecentInCategory
	JMP			TrimRecentDB
	JMP			SeekRecentInCategory
	JMP			CleanRecentDB
	JMP			RecentDBGetRecord
	JMP			RecentDBNewRecord
	JMP			RecentDBChangeRecord
//Links.c
	JMP			GetLinkCount
	JMP			FindLink
	JMP			UpdateLink
	JMP			AddLink
	JMP			DeleteLink
	JMP			DeleteOrphaned
	JMP			DeleteAllLinks
	JMP			GetLinkByIndex
	JMP			GoToLink
	JMP			LinkDBChangeRecord
	JMP			GetLinksLocalID
	JMP			GetAllLinkCount
//AddrTools.c
	JMP			PrvDetermineRecordNameHelper
@Name:
	DC.B		"AddressXTLib"
}
