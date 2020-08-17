#pragma once

#include <Form.h>
#include <Event.h>
#include "AddressDB.h"


/************************************************************
 * Function Prototypes
 *************************************************************/

void 	PrefsLoad(AddrAppInfoPtr appInfoPtr);
void	PrefsExtLoad();
void	PrefsSave(void);
void	PrefsExtSave(void);


Boolean	PrefsHandleEvent (EventType * event);

/***********************************************************************
 *
 *	Defines
 *
 ***********************************************************************/

#define FIVEWAYPAGE 0
#define FIVEWAYRECORD 1
#define FIVEWAYCONTACTS 2
//#define JOGDIALPAGE 0
//#define JOGDIALRECORD 1
#define STDPAGE 0
#define STDRECORD 1


#define addrPrefID							0x00
#define addrPrefExtID						0x00
#define addrPrefRegID						0x01
#define addrPrefExt2ID						0x02
#define addrPrefBtnID						0x03
#define addrPrefVersionNum					0x04
#define addrPrefColorID						0x05
#define addrPrefExt3ID						0x06
#define addrPrefRcntID						0x07
#define addrInstalledRegID					0x09
#define addrPrefCDataID						0x10
#define addrPrefSearchID					0x0A
#define addrPrefImportID					0x0B
#define addrPrefLinkID						0x0C
#define addrPrefExt4ID						0x0D
#define addrPrefImportBirthdayID			0x0E
#define addrPrefDialOptionsDataID			0x0F

typedef struct {
	UInt16			currentCategory;
	FontID			v20NoteFont;				// For 2.0 compatibility (BGT)
	Boolean			showAllCategories;
	Boolean			saveBackup;
	Boolean			rememberLastCategory;

	// Version 3 preferences
	FontID			addrListFont;
	FontID			addrRecordFont;
	FontID			addrEditFont;
	UInt8 			reserved1;
	UInt32			businessCardRecordID;
	FontID			noteFont;
	UInt8 			reserved2;

	// Version 4 preferences
	Boolean			enableTapDialing;
} AddrPreferenceType;

typedef struct {
	Boolean			addrListHighRes;
	Boolean			FullTextSearch;
} AddrExtPreferenceType;

typedef struct {
	Boolean			idsImported;
	Boolean			reserved1;
	Boolean			reserved2;
	Boolean			reserved3;
	Boolean			reserved4;
	Boolean			reserved5;
	Boolean			reserved6;
	Boolean			reserved7;
	Boolean			reserved8;
	Boolean			reserved9;
	Boolean			reserved10;
	Boolean			reserved11;
} AddrImportPreferenceType;

typedef struct {
	UInt16			OneHandedSearch;
	UInt16			AdvancedFind;
	UInt16			reserved2;
	UInt16			reserved3;
	UInt16			reserved4;
	UInt16			reserved5;
	UInt16			reserved6;
	UInt16			reserved7;
	UInt16			reserved8;
} AddrSearchPreferenceType;

typedef struct {
	UInt16			sortByCompany;
	UInt16			savedStdSort;
} AddrExt2PreferenceType;

typedef struct {
	UInt16			rememberLastRecord;//Boolean, 1 if remember
	UInt16			currentRecord;
	UInt16			addrView;//Boolean, 1 if exited while record was edited
} AddrExt3PreferenceType;

typedef struct {
	UInt16			JogDialUpDown;
	UInt16			FiveWayUpDown;
	UInt16			StdUpDown;
} AddrBtnPreferenceType;

typedef struct {
	UInt16			Enabled;
	UInt16			Number;
	UInt16			ShowAll;
	UInt16			reserved2;
	UInt16			reserved3;
	UInt16			reserved4;
} AddrRecentPreferenceType;


typedef struct {
	IndexedColorType	backColor;
	IndexedColorType	textColor;
	Boolean				defaults;
	IndexedColorType	selColor;
	IndexedColorType	eachOtherColor;
	Boolean				eachOther;
	IndexedColorType	recentColor;
	IndexedColorType	inactiveSelColor;
	IndexedColorType	reserv6;
	IndexedColorType	reserv7;
} AddrColorPreferenceType;

typedef struct {
Boolean EmptyOnly;
Boolean reserved8;
UInt16 reserved9;
UInt16 reserved10;
} AddrCDataPreferenceType;

typedef struct {
UInt32 creatorID[6];
Char dialPrefix[32];
Boolean	dialPrefixActive;
UInt16 reserved;
} AddrDialOptionsPreferenceType;


typedef struct {
UInt32 calendarCrID;
UInt32 memosCrID;
UInt32 tasksCrID;
UInt32 reserved11;
UInt32 reserved12;
UInt32 reserved13;
} AddrLinkPreferenceType;

typedef struct {
UInt8	delimiter;
UInt8	autoBluetooth;
UInt32	tomTomNumber;
UInt32	namesOnly;
UInt32	touchMode;
UInt32	reserved18;
UInt32	reserved19;
UInt32	reserved20;
} AddrExt4PreferenceType;
