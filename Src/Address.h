#pragma once

#include <IMCUtils.h>
#include <ExgMgr.h>
#include <PrivateRecords.h>
#include <UIColor.h>
#include <PdiLib.h>
#include <Form.h>
#include <PalmOS.h>
//#include <palmOnePhotoCommon.h>
#include <PalmOne_68K.h>
#include <HsNav.h>

#include "PalmContactsDB/ContactsDB.h"
#include "AddressDB.h"
#include "dia.h"
#include "Plugins/DateDB.h"
#include "AddrPrefs.h"
#include "TonesLibTypes.h"
#include "TonesLib.h"


#define kSpaceBetweenLabelAndNumber 6
#define kLeftAndRightSpace			2
#define	kMaxPhonesCount				25
#define kMaxCharsPhoneNumber		25

#define LOOKUP_END_EVENT firstUserEvent+100
typedef enum
{
	connectoptions_dial = 0,
	connectoptions_sms,
	connectoptions_email,
	connectoptions_map,
	connectoptions_web,
	connectoptions_im,
	connectoptions_num
} ConnectOptionsType;

typedef struct DialListPhoneTag
{
	Char*		label;
	Char*		number;
	Char*		number2;
	Char*		number3;
	Char*		number4;
	Char*		number5;
	UInt32		recID;
	
	Int16		numberLen;
	Int16		type;
	Int16		allocated;//whether memory was allocated
} DialListPhoneType;

typedef enum startupTypeEnum {
startupNormal,
startupLookup
} startupTypeEnum;

typedef struct
{
	AddrLookupParamsPtr params;
	AddressFields lookupFieldMap[addrLookupFieldCount];
	Boolean beepOnFail;
	UInt8 reserved;
} LookupVariablesType;

typedef LookupVariablesType *LookupVariablesPtr;

typedef struct DialListDataTag
{
	// Record
	MemHandle 			addrH;

	// Temp only accurate when drawing
#ifdef CONTACTS
	P1ContactsDBRecordType addr;
	P1ContactsAppInfoType*	appInfoP;
#else	
	AddrDBRecordType	addr;
	AddrAppInfoType*	appInfoP;
#endif	
	// Record description - allocated
	Char*				displayName;

	// Phone position - got from field position so that localization
	// can enable various position
	// Label will be aligned right + delta to that
	Coord				phoneX;
	Coord				phoneY;
	// X min is got from te Description label
	Coord				displayXMin;
	Coord				displayNameY;

	// list info
	Int16				topIndex;
	Int16				selectedIndex;

	// Array of data
	DialListPhoneType	phones[kMaxPhonesCount];
	UInt16				phonesCount;
} DialListDataType;

typedef struct 
{
	Char* address;
	Char* city;
	Char* state;
	Char* zip;
} AddressStruct;

typedef struct 
{
	Char**		strings;
	UInt32*		crIDs;
	UInt16		num;
} DialOptionStruct;

typedef struct
ListEntriesType
{
    Char **Pointer;
    UInt16 Size;
}
ListEntriesType;

typedef struct {UInt32 key;} RegKey; 

	typedef struct
	{
 	   UInt16 refNum;
 	   Boolean resizeEnabled;

 	   // The resized flag is a bit redundant, since resizeAmount == 0 could
	    // be used as a substitute.  Doing so, however, would make cases where
	    // the silkscreen is resized and restored--such that the net change is
	    // 0--a bit more difficult to handle.
	    Boolean resized;
	    PointType resizeAmount;
	    FormType* curResizableFormP;
	} SilkStateType;
	
typedef struct
{
	UInt16			fieldNum;
	UInt16			length;
	UInt16			offset;
	UInt16			x;
	UInt16			empty;
} RecordViewLineType;	

typedef enum
{
	type_palm = 0,
	type_oldTreo,
	type_treo650,
	type_treo700,
	type_treo755,
	type_treo680,
	type_centro,
	type_T5orTx
} DeviceType;

typedef union
{
	
	struct
	{
		unsigned reserved				:16;
		unsigned reserved2				:13;
		unsigned treoWithSendKeys		:1;
		unsigned treoWithPhoneKeyOnly	:1;
		unsigned treo					:1;
	} bits;
} PhoneTypeFlags;

typedef struct globalVarsStruct
{
	startupTypeEnum				startupType;
	UInt32						gLinkID;
	UInt16						gLinkType;
	//Boolean					gTreo, gTreo700;
	Boolean						gFiveWay, gJogDial;
	DeviceType					gDeviceType;
	PhoneTypeFlags				gDeviceFlags;
	ToneItemPtr					gRingerStruct;
	UInt16						gRingerCount;
	// G18 Xplore device
	Boolean						gNavigation;
	Boolean						gTrialExpired;
	UInt16						gTrialDays;
	Int16						gBListMonth, gBListDay, gBListYear;
	UInt32 						gTomTomNumber;
	UInt8						gDelimiter, gAutoBluetooth;
	UInt32						gShowNamesOnly;
	UInt32						gTouchMode;
	
	UInt32						gCalendarCrID, gMemosCrID, gTasksCrID;

	Boolean						gTomTom;
	UInt16						gTomTomVolRef;
	UInt16 						gScreen, refNum, gEnabledRecent;
	Int16 						gMaxRecent;
	Boolean						gShowAll;
	UInt32						gDepth;
	UInt32 						gDialerCreatorID;
	UInt16						gDuplicateCaller;
	UInt16						gRecentMax;
	Boolean						gAddressView;
	IndexedColorType			gForeground, gBackground, gFieldText, gObjSelText, gFldSelText, gFill, gAlert, gDialogFill, gObjectFill; 
	Boolean						gJogLeftRight;
	Boolean						gScrollMode;
	UInt32						gOldFlag;
	Boolean						gMenuVisible;
	DmOpenRef					AddrDB, RecentDB, linksDB;
	UInt16						gColorFormID;
	privateRecordViewEnum		PrivateRecordVisualStatus;
	Char						CategoryName [dmCategoryLength];
	UInt16        				TopVisibleRecord;
	UInt16						TopVisibleFieldIndex;
	UInt16						EditFieldPosition;
	UInt16          			CurrentRecord, SelectedRecord;
	UInt16           			ListViewSelectThisRecord;		// This must
	UInt16						adxtLibRef;
	UInt32 						clientContext;
	Char						ringerLabel[50];
	Char						annivLabel[50];
	// be set whenever we leave a
	// dialog because a frmSaveEvent
	// happens whenever the focus is
	// lost in the EditView and then
	// a find and goto can happen
	// causing a wrong selection to
	// be used.
	UInt16						SortByCompany;
	UInt16						PriorAddressFormID;
	UInt32						PrevCreatorID;
	Boolean						DIA;//, gKyocera;

	UInt16						/*JogDialUpDown,*/ FiveWayUpDown, StdUpDown;

	// These are used for controlling the display of the duplicated address records.
	UInt16						NumCharsToHilite;

	UInt16						EditRowIDWhichHadFocus;
	UInt16						EditLabelColumnWidth;
	UInt16						RecordLabelColumnWidth;
	Char *						UnnamedRecordStringPtr;
	MemHandle					UnnamedRecordStringH;
	Boolean						RecordNeededAfterEditView;
	Boolean						gTrackRecent;

	// The following global variable are saved to a state file.
	UInt16						CurrentCategory;
	Boolean						EnableTapDialing;//, gTapwave;	// tap dialing is not enabled by default
	UInt16						gAdvancedFind;
	Boolean						ShowAllCategories;
	Boolean						SaveBackup;
	Boolean 					SaveCatBackup;
	Boolean						RememberLastCategory;
	FontID						NoteFont;
	FontID						AddrListFont;
	FontID						AddrRecordFont;
	FontID						AddrEditFont;
	UInt32						BusinessCardRecordID;
	Boolean						AddrListHighRes;

	// For business card beaming
	UInt32						TickAppButtonPushed;

	// Valid after PrvAppStart
	#ifdef CONTACTS
	Char						PhoneLabelLetters[P1ContactsnumPhoneLabels];
	#else
	Char						PhoneLabelLetters[numPhoneLabels];
	#endif

	Boolean						DialerPresentChecked;
	Boolean						DialerPresent;

	Boolean 					gRememberLastContact;
	UInt16 						gLastRecord;
	Boolean 					gAddrView;
	Boolean 					gOneHanded;

	Char 						gTasksStr[5];
	Char 						gCalendarStr[5];
	Char 						gMemosStr[5];
	
	Char 						triggerStr[255];
	UInt16 						gLinkTypeSel;

	Boolean 					gToDoPlugin;
	Boolean 					gMemoPlugin;

	Char 						LinkList[3][9];

	Boolean 					MemoPlugin;
	Boolean 					ToDoPlugin;

	Char 						**LinkTypes;
	
	Boolean 					gSNCalled;
	Int16 						gD, gM, gY;
	Char 						gDateTxt[255];
	
	UInt16				CurrentFieldIndex;

	// global to remember the row of the last tapped tel label
	UInt16				CurrentTableRow;
	Boolean 					gEditTableActive;
	// The following structure maps row in the edit table to fields in the
	// address record.  This controls the order in which fields are edited.
	// Valid after PrvEditInit.
	MemHandle			FieldMapH;

	// Valid after PrvEditInit
	Char * EditPhoneListChoices[numPhoneLabels];
	Char * EditIMChoices[P1ContactsnumChatLabels];
	Char * EditAddrChoices[P1ContactsnumAddressLabels];

	Boolean skipWinEnter/* = false*/;
	
	// For business card beaming
	UInt16				AppButtonPushedModifiers/* = 0*/;
	Boolean				BusinessCardSentForThisButtonPress/* = false*/;
	UInt16				AppButtonPushed/* = nullChr*/;

	AddressStruct **AddressListPtr;
	Char **AddressListPtrStr;
	UInt16 AddressCount;
	UInt16 gAddressSel;
	Char gTomTomNumberStr[255];

	Char gSortMode[255];
	Char gDelimiterStr[31];
	Char gROStr[31];
	Boolean gRefresh;

	// what's selected now
	UInt16 selection;
	
	UInt16 RowEnter, ColEnter, RowExit, ColExit;
	Char gName2[dmCategoryLength+10];

	Char **RecentList;
	Boolean	gMenuActive;
	Boolean gListTableActive;
	Boolean gReturnFocusToTable/* = false*/;


	// These are used for accelerated scrolling
	UInt16 				LastSeconds/* = 0*/;
	UInt16 				ScrollUnits/* = 0*/;

	UInt16				ScrollPosition/*=0*/;

	// title bitmap caching
	MemHandle 			bitmapH/* = 0*/;
	BitmapType* 			bitmap/* = 0*/;

	Char Birthday[31];

	univAddrDBRecordType		recordViewRecord;
	MemHandle					recordViewRecordH/* = 0*/;
	RecordViewLineType			*RecordViewLines;
	UInt16				RecordViewLastLine;   // Line after last one containing data
	UInt16				TopRecordViewLine;
	UInt16				RecordViewFirstPlainLine;
	UInt16 				ImageOffset;
	UInt16 				ImageHeight;

	Char gMonth[255];
	UInt16 gBScrollPosition, gBRows, gBMaxRow, gBTableRows;
	TablePtr gBTable;

	DmOpenRef gAddrXTDB;

	Char gStr1[31], gStr2[31], gStr3[31];

	IndexedColorType gSavedColorText, gColorText;	
	IndexedColorType gSavedColorBack, gColorBack, gSavedColorSel, gColorSel, gSavedInactiveSel, gInactiveSel, gSavedEachOther, gEachOther;	
	Boolean gEachOtherSelected;

	UInt16 gTheme, gSavedTheme;

	Char gLabelStr[63];

	UInt16 /*ScrollPosition, */MaxScrollPosition;
	UInt16 Rows, MaxRow, TableRows;
	TablePtr Table;
	UInt16 				ContactsSelScrollUnits/* = 0*/;

	Char *gAddress, *gCompany, *gCity, *gZipCode, *gCountry, *gState, *gNewCompany, *gDomain;
	Char *gNewEMail;

	Boolean ListSizeSet;

	//Boolean skipWinEnter/* = true*/;

	ListEntriesType gCompanyList;
	ListEntriesType gRingerList;
	
	ToneItemType*	gRingerListStruct;
	UInt16 ContactSelCurrentRecord;
	UInt16 ContactSelSelectedRecord/* = noRecord*/;
	UInt16 ContactsSelTopVisibleRecord/* = 0*/;

	UInt16 ContactsSelCurrentCategory/* = dmAllCategories*/;
	struct dia_support dia_object;
	//=== globals ====
	// saved state
	//UInt16 pin_state/*= 0*/;
	UInt16 trigger_state/* = 0*/;

	//=== globals ===
	// silk lib
	UInt16 silk_lib;
	UInt32 vskVersion/* = 0*/; 
	// status
	UInt16 area_state/* = 0*/, pin_state/* = 0*/;
	//UInt16 silk_lib;

	Char **EMailListPtr;
	UInt16 EMailCount;
	Char gEMail[255];

	TablePtr gLTable;
	UInt16 gLScrollPosition, gLTableRows, gLRows;
	
/***********************************************************************
 *
 *	Global variables
 *
 ***********************************************************************/
MemHandle			ApptsOnlyH;
UInt16				NumApptsOnly;
MemHandle			ApptsH;
UInt16				NumAppts;

DmOpenRef			ApptDB;									// datebook database
DateType				Date/* = { 91, 7, 31 }*/;				// date currently displayed
TimeFormatType		TimeFormat;
DateFormatType		LongDateFormat;
DateFormatType		ShortDateFormat;
UInt16				TopVisibleAppt;
privateRecordViewEnum	DatePrivateRecordVisualStatus;
Boolean				NoteUpScrollerVisible;				// true if note can be scroll winUp
Boolean				NoteDownScrollerVisible;			// true if note can be scroll winUp
UInt16				PendingUpdate/* = 0*/;					// code of pending day view update
RGBColorType 		colorLine/* = {0x00, 0x77, 0x77, 0x77}*/;	// like 0x88 but draws as black in 1bpp mode

Boolean				InPhoneLookup/* = false*/;				// true if we've called PhoneNumberLookup()

// The following global variables are used to keep track of the edit
// state of the application.
UInt16				DayEditPosition/* = 0*/;					// position of the insertion point in the desc field
UInt16				DayEditSelectionLength;				// length of the current selection.
UInt16				DateCurrentRecord/* = noRecordSelected*/;// record being edited
Boolean				ItemSelected/* = false*/;				// true if a day view item is selected
Boolean				RecordDirty/* = false*/;					// true if a record has been modified


// The following global variable are only valid while editng the detail
// of an appointment.
void *				DetailsP;
DateType			RepeatEndDate;
RepeatType			RepeatingEventType;

// The following global variables are only valid while editing the repeat information
// of an appointment
void *				RepeatDetailsP;

// The following global variable are saved to a state file.
UInt16				DayStartHour/* = defaultDayStartHour*/;	// start of the day 8:00am
UInt16				DayEndHour/* = defaultDayEndHour*/;		// end of the day 6:00pm
UInt16				StartDayOfWeek/* = sunday*/;
UInt16				RepeatStartOfWeek/* = sunday*/;		//	status of Repeat Dialog
AlarmInfoType		AlarmPreset/* = { defaultAlarmPresetAdvance, defaultAlarmPresetUnit }*/;
Boolean				ShowTimeBars/* = defaultShowTimeBars*/;			// show time bars in the day view
Boolean				CompressDayView/* = defaultCompressDayView*/;	// remove empty time slot to prevent scrolling
Boolean				ShowTimedAppts/* = defaultShowTimedAppts*/;	// show timed appointments in month view
Boolean				ShowUntimedAppts/* = defaultShowUntimedAppts*/;	// show untimed appointments in month view
Boolean				ShowDailyRepeatingAppts/* = defaultShowDailyRepeatingAppts*/;	// show daily repeating appointments in month view

// The following global variable is used to control the behavior Datebook
// Hard Button when pressed from the week or month views.  If no pen or key event 
// when occurred since enter the Week View then pressing the Datebook button
// will nagivate to the Month View, otherwise we go the the Day View of Today.
// Likewise, pressing the Datebook Hard Button will navigate from the Month View
// to either the Agenda View or the Day View, depending upon whether or not there
// were any user actions.
Boolean				EventInCurrentView;


UInt16				TimeBarColumns;						// Number of columns of time bars.


// The following global variable is used to control the displaying of the
// current time in the title of a view.
Boolean				TimeDisplayed/* = false*/;				// True if time in been displayed
UInt32				TimeDisplayTick;						// Tick count when we stop showing time

// The following globals are for the repeat rates of the alarms.
																		// number of times to repeat alarm sound 
UInt16				AlarmSoundRepeatCount/* = defaultAlarmSoundRepeatCount*/;
																	
																		// interval between repeat sounds, in seconds
UInt16				AlarmSoundRepeatInterval/* = defaultAlarmSoundRepeatInterval*/;

																		// Alarm sound MIDI file unique ID record identifier
UInt32				AlarmSoundUniqueRecID/* = defaultAlarmSoundUniqueRecID*/;

FontID				ApptDescFont;								// font for drawing event description.
																			
UInt16				AlarmSnooze/* = defaultAlarmSnooze*/;		// snooze delay, in seconds

UInt16				DismissedAlarmCount/* = 0*/;
UInt32				DismissedAlarms [apptMaxDisplayableAlarms]/* = {}*/;


DmOpenRef		MemoDB;
char				MemoCategoryName [dmCategoryLength];
UInt16			MemosInCategory;
privateRecordViewEnum		MemoPrivateRecordVisualStatus;
MenuBarPtr		CurrentMenu;

// The following global variable are saved to a state file.
UInt16			MemoTopVisibleRecord/* = 0*/;
UInt16			MemoCurrentRecord/* = noRecordSelected*/;
UInt16			MemoCurrentView/* = MemoListView*/;
UInt16			MemoCurrentCategory/* = dmAllCategories*/;
Boolean			MemoShowAllCategories/* = true*/;
UInt16			MemoEditScrollPosition/* = 0*/;
Boolean			MemoSaveBackup/* = true*/;

Boolean			MemoInPhoneLookup/* = false*/;


DmOpenRef			ToDoDB;										// ToDo database
char					ToDoCategoryName [dmCategoryLength];		// name of the current category
 privateRecordViewEnum		ToDoPrivateRecordVisualStatus;
 MenuBarPtr			ToDoCurrentMenu;								// pointer to the current menu bar
 UInt16				ToDoTopVisibleRecord/* = 0*/;					// top visible record in list view
 UInt16				ToDoPendingUpdate/* = 0*/;						// code of pending list view update
 DateFormatType	DateFormat;
 DateType			Today;										// Date when the device was powered on.

// The following global variables are used to keep track of the edit
// state of the application.
 UInt16				ToDoCurrentRecord/* = noRecordSelected*/;	// record being edited
 Boolean				ToDoItemSelected/* = false*/;					// true if a list view item is selected
 Boolean				ToDoRecordDirty/* = false*/;						// true if a record has been modified
 UInt16				ListEditPosition/* = 0*/;					// position of the insertion point in the desc field
 UInt16				ListEditSelectionLength;				// length of the current selection.

// The following global variables are saved to a state file.
 FontID				ToDoNoteFont/* = stdFont*/;						// font used in note view
 UInt16				ToDoCurrentCategory/* = dmAllCategories*/;	// currently displayed category
 Boolean				ToDoShowAllCategories/* = true*/;				// true if all categories are being displayed
 Boolean 			ShowCompletedItems/* = true*/;				// true if completed items are being displayed
 Boolean 			ShowOnlyDueItems/* = false*/;				// true if only due items are displayed
 Boolean				ShowDueDates/* = false*/;					// true if due dates are displayed in the list view
 Boolean				ShowPriorities/* = true*/;					// true if priorities are displayed in the list view
 Boolean				ShowCategories/* = false*/;					// true if categories are displayed in the list view
 Boolean				ToDoSaveBackup/* = true*/;						// true if save backup to PC is the default
 Boolean				ChangeDueDate/* = false*/;					// true if due date is changed to completion date when completed
 FontID				ListFont/* = stdFont*/;						// font used to draw to do item

 Boolean				ToDoInPhoneLookup/* = false*/;					// true if we've called PhoneNumberLookup()


UInt16 editLastFieldIndex;

#ifdef CONTACTS
	P1ContactsFields 	FieldMap[P1ContactsNumStringFields+1];
#else
	AddressFields* 		FieldMap;
#endif

Char * 	DetailsPhoneListChoices[P1ContactsnumPhoneFields];
UInt16       category;
Boolean      categoryEdited;

Boolean gDialListKeyDown;

DialListDataType* gDialListData;
Char gPhoneChars[15];/* = "0123456789,+#*";*/

LookupVariablesType lookupVars;	

UInt16 						SelectedLookupRecord;
UInt16						adxtLib2Ref;
Char 						lookupString[addrLookupStringLength];

DialOptionStruct			ContactSettings[connectoptions_num];

UInt16						listID[connectoptions_num]; 

AddrDialOptionsPreferenceType	dialOptions;

UInt16 dialListRecordIndex, dialListPhoneIndex, dialListLineIndex;

Boolean	suppressListFocusEvent;

UInt16 imageLibRef;
UInt16 tonesLibRef;

} globalVars;
	

#include <Find.h>
#include <NotifyMgr.h>
#include <ErrorMgr.h>
#include <TimeMgr.h>
#include <KeyMgr.h>
#include <Menu.h>
#include <UIResources.h>
#include <SysEvtMgr.h>
#include <Preferences.h>
//#include <pdQCore.h>
#include <HsExt.h>

/***********************************************************************
 *
 *   Defines
 *
 ***********************************************************************/
// Time to depress the app's button to send a business card
#define AppButtonPushTimeout					(sysTicksPerSecond)


typedef enum {
	appLaunchCmdAlarmEventGoto = sysAppLaunchCmdCustomBase
} AddressBookCustomLaunchCodes;

typedef void DIAResizeFuncType  
		(FormPtr frmP, Coord dxP, Coord dyP);

typedef DIAResizeFuncType *DIAResizeFuncPtr;

//#define IsZodiacLeft(eventP) (eventP->data.keyDown.chr == 28 && eventP->data.keyDown.keyCode == 0 && eventP->data.keyDown.modifiers == 4)


//#define IsZodiacRight(eventP) (eventP->data.keyDown.chr == 29 && eventP->data.keyDown.keyCode == 0 && eventP->data.keyDown.modifiers == 4)

//#define IsZodiacUp(eventP) (eventP->data.keyDown.chr == 30 && eventP->data.keyDown.keyCode == 0 && eventP->data.keyDown.modifiers == 4)

//#define IsZodiacDown(eventP) (eventP->data.keyDown.chr == 31 && eventP->data.keyDown.keyCode == 0 && eventP->data.keyDown.modifiers == 4)

//#define IsZodiacSelect(eventP) (eventP->data.keyDown.chr == 310 && eventP->data.keyDown.keyCode == 0 && eventP->data.keyDown.modifiers == 2060)

void AllocGlobals();
void DeleteGlobals();
Boolean PrvAppHandleEvent (EventType * event);
Boolean LoadAddressXTLibraries(UInt16* libRef, UInt16* libRef2);
Err PrvAppStart();
void PrvAppStop();
void PrvAppEventLoop ();
Boolean PrvAppHandleKeyDown (EventType * event);
Boolean PrvAppLookupEventLoop ();
void SetDialer();
void RestoreDialer();


//Custom macros
#define CustomLstGetSelection(listID) (LstGetSelection((ListType*)GetObjectPtrSmp(listID)))
#define CustomLstSetSelection(listID, itemNum) (LstGetSelection((ListType*)GetObjectPtrSmp(listID)), itemNum)