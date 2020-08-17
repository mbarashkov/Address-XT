/******************************************************************************
 *
 * Copyright (c) 1995-2002 PalmSource, Inc. All rights reserved.
 *
 * File: AddrPrefs.c
 *
 * Release: Palm OS 5 SDK (68K) R3.
 *
 * Description:
 *  This is the Address Book application's pref screen
 *
 *****************************************************************************/

#include "AddrPrefs.h"
#include "Address.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "AddressTransfer.h"
#include "AddrDialList.h"
#include "AddressRsc.h"
#include "globals.h"
#include "dia.h"

#include <ErrorMgr.h>
#include <Preferences.h>
#include <BtLib.h>



/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/

static Boolean PrvPrefsSave();
static void PrvPrefsInit (FormPtr frm);

void SetCrIDField(UInt16 field, UInt32 type);
UInt32 GetCrIDField(UInt16 field);

/***********************************************************************
 *
 * FUNCTION:    AddressLoadExtPrefs
 *
 * DESCRIPTION: Load the application extended preferences.
 *
 * PARAMETERS:  appInfoPtr	-- Pointer to the app info structure
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         MB	  26/01/04  Initial revision
 *
 ***********************************************************************/
void PrefsExtLoad()
{
	globalVars* globals = getGlobalsPtr();
	Int16 prefsVersion;
	UInt16 prefsSize;
	AddrExtPreferenceType prefs;
	AddrBtnPreferenceType prefs2;
	AddrRecentPreferenceType prefsRcnt;
	AddrExt3PreferenceType prefs3;
	AddrExt4PreferenceType prefs4;
	AddrSearchPreferenceType searchPrefs;
	AddrLinkPreferenceType linkPrefs;
	
	globals->AddrListHighRes=false;
	prefsSize = sizeof (AddrExtPreferenceType);
	prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefExtID, &prefs, &prefsSize, true);
	if (prefsVersion > noPreferenceFound)
	{
		globals->AddrListHighRes=prefs.addrListHighRes;
	}
	
	//load button prefs
	//globals->JogDialUpDown=JOGDIALPAGE;
	globals->FiveWayUpDown=FIVEWAYCONTACTS;
	globals->StdUpDown=STDPAGE;
	
	prefsSize = sizeof (AddrBtnPreferenceType);
	prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefBtnID, &prefs2, &prefsSize, true);
	if (prefsVersion > noPreferenceFound)
	{
		//globals->JogDialUpDown=prefs2.JogDialUpDown;
		globals->FiveWayUpDown=prefs2.FiveWayUpDown;
		globals->StdUpDown=prefs2.StdUpDown;
		
	}
	
	if(globals->FiveWayUpDown!=FIVEWAYPAGE && globals->FiveWayUpDown!= FIVEWAYRECORD)
		globals->FiveWayUpDown=FIVEWAYRECORD;
	//if(globals->JogDialUpDown!=JOGDIALPAGE && globals->JogDialUpDown!= JOGDIALRECORD)
	//	globals->JogDialUpDown=JOGDIALPAGE;
	if(globals->StdUpDown!=STDPAGE && globals->StdUpDown!= STDRECORD)
		globals->StdUpDown=STDPAGE;
	
	//if(globals->gDeviceFlags.bits.treo)
	//	globals->FiveWayUpDown = FIVEWAYRECORD;
			
		
	globals->gRememberLastContact=false;
	globals->gLastRecord=noRecord;
	globals->gAddrView=false;
	prefsSize = sizeof (AddrExt3PreferenceType);
	prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefExt3ID, &prefs3, &prefsSize, true);
	if (prefsVersion > noPreferenceFound)
	{
		globals->gRememberLastContact=prefs3.rememberLastRecord;
		globals->gLastRecord=prefs3.currentRecord;
		globals->gAddrView=prefs3.addrView;
	}
	
	
	prefsSize = 0;
	PrefGetAppPreferences (CREATORID, addrPrefSearchID, NULL, &prefsSize, true);
	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefSearchID, &searchPrefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	}
	if (prefsVersion != noPreferenceFound)
	{
		globals->gOneHanded=searchPrefs.OneHandedSearch;
		globals->gAdvancedFind = searchPrefs.AdvancedFind;
	}	
	else
	{
		globals->gOneHanded = false;
		globals->gAdvancedFind = false;
	}

	globals->gDelimiter = DELIMITER_DEFAULT;
	prefsSize = sizeof (AddrExt4PreferenceType);
	prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefExt4ID, &prefs4, &prefsSize, true);
	if (prefsVersion > noPreferenceFound)
	{
		globals->gDelimiter=prefs4.delimiter;
		globals->gAutoBluetooth = prefs4.autoBluetooth;
		globals->gTomTomNumber = prefs4.tomTomNumber;
		if(globals->gDelimiter > DELIMITER_COUNT - 1)
			globals->gDelimiter = DELIMITER_DEFAULT;
		globals->gShowNamesOnly = prefs4.namesOnly;	
		globals->gTouchMode = prefs4.touchMode;	
	}
	
	prefsSize = 0;
	PrefGetAppPreferences (CREATORID, addrPrefRcntID, NULL, &prefsSize, true);
	globals->gMaxRecent=0;
	globals->gEnabledRecent=0;
	globals->gShowAll=false;
	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefRcntID, &prefsRcnt, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	}
	if (prefsVersion != noPreferenceFound)
	{
		globals->gMaxRecent=prefsRcnt.Number;
		globals->gEnabledRecent=prefsRcnt.Enabled;
		if(prefsVersion==1)
		{
			globals->gShowAll=false;
		}
		else
		{
			globals->gShowAll=prefsRcnt.ShowAll;
		}
	}	
	
	prefsSize = 0;
	PrefGetAppPreferences (CREATORID, addrPrefLinkID, NULL, &prefsSize, true);
	globals->gCalendarCrID=0;
	globals->gMemosCrID=0;
	globals->gTasksCrID=0;
	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefLinkID, &linkPrefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
#ifdef CONTACTS
		globals->gMemosCrID = NEW_MEMOS;
		globals->gCalendarCrID = NEW_CALENDAR;
		globals->gTasksCrID = NEW_TASKS;
#else
		globals->gMemosCrID = OLD_MEMOS;
		globals->gCalendarCrID = OLD_CALENDAR;
		globals->gTasksCrID = OLD_TASKS;
#endif			
	}
	if (prefsVersion != noPreferenceFound)
	{
		globals->gCalendarCrID=linkPrefs.calendarCrID;
		globals->gMemosCrID=linkPrefs.memosCrID;
		globals->gTasksCrID=linkPrefs.tasksCrID;
	}			
}

/***********************************************************************
 *
 * FUNCTION:    AddressLoadPrefs
 *
 * DESCRIPTION: Load the application preferences and fix them up if
 *				there's a version mismatch.
 *
 * PARAMETERS:  appInfoPtr	-- Pointer to the app info structure
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         BGT   1/8/98     Initial revision
 *
 ***********************************************************************/

void PrefsLoad(AddrAppInfoPtr appInfoPtr)
{
	globalVars* globals = getGlobalsPtr();
	Int16 prefsVersion;
	UInt16 prefsSize;
	AddrPreferenceType prefs;
	
	// Read the preferences / saved-state information.  There is only one
	// version of the Address Book preferences so don't worry about multiple
	// versions.  Users appreciate the transferal of their preferences from prior
	// versions.
	prefsSize = sizeof (AddrPreferenceType);
	prefsVersion = PrefGetAppPreferences (sysFileCAddress, addrPrefID, &prefs, &prefsSize, true);
	
	if (prefsVersion > addrPrefVersionNum) {
		prefsVersion = noPreferenceFound;
	}
	if (prefsVersion > noPreferenceFound)
	{
		if (prefsVersion < addrPrefVersionNum) {
			prefs.noteFont = prefs.v20NoteFont;
		}
		globals->SaveBackup = prefs.saveBackup;
		globals->RememberLastCategory = prefs.rememberLastCategory;
		if (prefs.noteFont == largeFont)
			globals->NoteFont = largeBoldFont;
		else
			globals->NoteFont = prefs.noteFont;

		// If the preferences are set to use the last category and if the
		// category hasn't been deleted then use the last category.
	
		if (globals->RememberLastCategory &&
			prefs.currentCategory != dmAllCategories &&
			appInfoPtr->categoryLabels[prefs.currentCategory][0] != '\0')
		{
			globals->CurrentCategory = prefs.currentCategory;
			DmSeekRecordInCategory(globals->AddrDB, &(globals->TopVisibleRecord), 0, dmSeekForward, globals->CurrentCategory);
		
			globals->ShowAllCategories = prefs.showAllCategories;
		}
		// Support transferal of preferences from the previous version of the software.
		if (prefsVersion >= 3)
		{
			// Values not set here are left at their default values
			globals->AddrListFont = prefs.addrListFont;
			globals->AddrRecordFont = prefs.addrRecordFont;
			globals->AddrEditFont = prefs.addrEditFont;
			globals->BusinessCardRecordID = prefs.businessCardRecordID;
		}

		// Support transferal of preferences from the previous version of the software.
		if (prefsVersion >= addrPrefVersionNum)
		{
			globals->EnableTapDialing = prefs.enableTapDialing;
		}
	}

	// The first time this app starts register to handle vCard data.
	if (prefsVersion != addrPrefVersionNum)
		TransferRegisterData(globals->adxtLibRef);

	MemPtrUnlock(appInfoPtr);
}

/***********************************************************************
 *
 * FUNCTION:    AddressSavePrefs
 *
 * DESCRIPTION: Save the Address preferences with fixups so that
 *				previous versions won't go crazy.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         BGT   1/8/98     Initial Revision
 *         SCL   2/12/99    Clear reserved fields before writing saved prefs
 *			  gap  12/06/00	 EnableTapDialing is now false by default, only
 *									 checked if turned on by user
 *
 ***********************************************************************/

void PrefsSave(void)
{
	globalVars* globals = getGlobalsPtr();
	AddrPreferenceType prefs;
	// Write the preferences / saved-state information.
	prefs.currentCategory = globals->CurrentCategory;
	ErrNonFatalDisplayIf(globals->NoteFont > largeBoldFont, "Note font invalid.");
	prefs.noteFont = globals->NoteFont;
	if (prefs.noteFont > largeFont) {
		prefs.v20NoteFont = stdFont;
	}
	else {
		prefs.v20NoteFont = prefs.noteFont;
	}
	prefs.addrListFont = globals->AddrListFont;
	prefs.addrRecordFont = globals->AddrRecordFont;
	prefs.addrEditFont = globals->AddrEditFont;
	prefs.showAllCategories = globals->ShowAllCategories;
	prefs.saveBackup = globals->SaveBackup;
	prefs.rememberLastCategory = globals->RememberLastCategory;
	prefs.businessCardRecordID = globals->BusinessCardRecordID;
	prefs.enableTapDialing = globals->EnableTapDialing;

	// Clear reserved fields so prefs don't look "different" just from stack garbage!
	prefs.reserved1 = 0;
	prefs.reserved2 = 0;

	// Write the state information.
	PrefSetAppPreferences (sysFileCAddress, addrPrefID, addrPrefVersionNum, &prefs,
						   sizeof (AddrPreferenceType), true);
}

/***********************************************************************
 *
 * FUNCTION:    AddressSaveExtPrefs
 *
 * DESCRIPTION: Save the Address XT preferences 
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         MB     26/01/04  Initial Revision
 *        
 *
 ***********************************************************************/

void PrefsExtSave(void)
{
	globalVars* globals = getGlobalsPtr();
	Int16 prefsVersion;
	AddrExtPreferenceType prefs;
	AddrExt2PreferenceType prefs2;
	AddrExt3PreferenceType prefs3;
	AddrExt4PreferenceType prefs4;
	UInt16 prefsSize=0;
	FormPtr frm=FrmGetActiveForm();
	
	PrefGetAppPreferences (CREATORID, addrPrefExtID, NULL, &prefsSize, true);
	// Write the preferences / saved-state information.
	if(prefsSize>0)
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefExtID, &prefs, &prefsSize, true);
	else
	{
		prefs.FullTextSearch=false;
		prefsSize=sizeof(AddrExtPreferenceType);
	}
	if (prefsVersion == noPreferenceFound)
	{
		prefs.FullTextSearch=false;
		prefsSize=sizeof(AddrExtPreferenceType);
	}
	// Write the state information.
	prefs.addrListHighRes = globals->AddrListHighRes;
	
	PrefSetAppPreferences (CREATORID, addrPrefExtID, addrPrefVersionNum, &prefs,
						   prefsSize, true);
						   
	prefsSize=0;
						   
	PrefGetAppPreferences (CREATORID, addrPrefExt2ID, NULL, &prefsSize, true);
	if(prefsSize>0)
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefExt2ID, &prefs2, &prefsSize, true);
	else
	{
		prefs2.savedStdSort=1000;//impossible value
		prefsSize=sizeof(AddrExt2PreferenceType);
	}
	if (prefsVersion == noPreferenceFound)
	{
		prefs2.savedStdSort=1000;//impossible value
		prefsSize=sizeof(AddrExt2PreferenceType);
	}
	prefs2.sortByCompany=globals->SortByCompany;
	PrefSetAppPreferences (CREATORID, addrPrefExt2ID, addrPrefVersionNum, &prefs2,
						   prefsSize, true);
						 
	
	prefsSize=0;
						   
	PrefGetAppPreferences (CREATORID, addrPrefExt3ID, NULL, &prefsSize, true);
	if(prefsSize>0)
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefExt3ID, &prefs3, &prefsSize, true);
	else
	{
		prefsSize=sizeof(AddrExt3PreferenceType);
	}
	if (prefsVersion == noPreferenceFound)
	{
		prefsSize=sizeof(AddrExt3PreferenceType);
	}
	prefs3.rememberLastRecord=globals->gRememberLastContact;
	prefs3.addrView=globals->gAddressView;
	prefs3.currentRecord=globals->CurrentRecord;
	PrefSetAppPreferences (CREATORID, addrPrefExt3ID, addrPrefVersionNum, &prefs3, prefsSize, true);

	
	prefsSize=0;
						   
	PrefGetAppPreferences (CREATORID, addrPrefExt4ID, NULL, &prefsSize, true);
	if(prefsSize>0)
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefExt4ID, &prefs4, &prefsSize, true);
	else
	{
		prefsSize=sizeof(AddrExt4PreferenceType);
	}
	if (prefsVersion == noPreferenceFound)
	{
		prefsSize=sizeof(AddrExt4PreferenceType);
	}
	prefs4.delimiter=globals->gDelimiter;
	prefs4.autoBluetooth=globals->gAutoBluetooth;
	prefs4.tomTomNumber=globals->gTomTomNumber;
	prefs4.namesOnly=globals->gShowNamesOnly;
	prefs4.touchMode=globals->gTouchMode;
	prefs4.reserved18=0;
	prefs4.reserved19=0;
	prefs4.reserved20=0;
	PrefSetAppPreferences (CREATORID, addrPrefExt4ID, addrPrefVersionNum, &prefs4, prefsSize, true);
}

void SetCrIDField(UInt16 field, UInt32 type)
{
	globalVars* globals = getGlobalsPtr();
	Char crID[5];
	FieldPtr fieldP = (FieldPtr) CustomGetObjectPtrSmp(field);;
	crID[0] = (Char) (type >> 24);
	crID[1] = (Char) ((type << 8) >> 24);
	crID[2] = (Char) ((type << 16) >> 24);
	crID[3] = (Char) ((type << 24) >> 24);
	crID[4] = 0;
	switch(field)
	{
		case PreferencesCalendarCrID:
			StrCopy(globals->gCalendarStr, crID);
			CustomEditableFldSetTextPtrSmp(field, globals->gCalendarStr);
			break;
		case PreferencesMemosCrID:
			StrCopy(globals->gMemosStr, crID);
			CustomEditableFldSetTextPtrSmp(field, globals->gMemosStr);
			break;
		case PreferencesTasksCrID:
			StrCopy(globals->gTasksStr, crID);
			CustomEditableFldSetTextPtrSmp(field, globals->gTasksStr);
			break;
	}
	
}

UInt32 GetCrIDField(UInt16 field)
{
	UInt32 crID;
	Char* text;
	if(CustomFldIsEmptySmp(field))
		return 0;
	text = CustomFldGetTextPtrSmp(field);
	if(text[0] == 0)
		return 0;
	if(StrLen(text) != 4)
		return 0;
	crID = text[0];
	crID <<= 8;
	crID |= text[1];
	crID <<= 8;
	crID |= text[2];
	crID <<= 8;
	crID |= text[3];
	
	return crID;
}

static Boolean PrefsOnCtlSelectEvent(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	FormPtr frm;
	switch (event->data.ctlSelect.controlID)
	{
		case PreferencesDefaultCalendar:
#ifdef CONTACTS
			SetCrIDField(PreferencesCalendarCrID, NEW_CALENDAR);
#else
			SetCrIDField(PreferencesCalendarCrID, OLD_CALENDAR);	
#endif
			FrmDrawForm(FrmGetActiveForm());		
			handled = true;
			break;
		case PreferencesDefaultTasks:
#ifdef CONTACTS
			SetCrIDField(PreferencesTasksCrID, NEW_TASKS);
#else
			SetCrIDField(PreferencesTasksCrID, OLD_TASKS);			
#endif
			FrmDrawForm(FrmGetActiveForm());		
			handled = true;
			break;
		case PreferencesDefaultMemos:
#ifdef CONTACTS
			SetCrIDField(PreferencesMemosCrID, NEW_MEMOS);
#else
			SetCrIDField(PreferencesMemosCrID, OLD_MEMOS);			
#endif
			FrmDrawForm(FrmGetActiveForm());		
			handled = true;
			break;
		
		case PreferencesOkButton:
			SetDialer();
			frm = FrmGetActiveForm();
			if(PrvPrefsSave())
				ToolsLeaveForm();
			handled = true;
			break;

		case PreferencesCancelButton:
			SetDialer();
			ToolsLeaveForm();
			handled = true;
			break;
		default:
			break;
	}
	return handled;
}

static Boolean PrefsOnKeyDownEvent(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	FormPtr frm;
	if(IsFiveWayNavEvent(event) && !globals->gNavigation)
	{
		if (NavKeyPressed(event, Select))
		{
			frm = FrmGetActiveForm();
			if(PrvPrefsSave())
				ToolsLeaveForm();
			return true;
		}		   	 	
	}  
	return handled;
}
			
static Boolean PrefsOnMenuOpenEvent()
{
	globalVars* globals = getGlobalsPtr();
	UInt32 g2DynamicID;
	if(!globals->gDeviceFlags.bits.treo)
	{
		if (!FtrGet('grft', 1110, &g2DynamicID) ||
			!FtrGet('grf2', 1110, &g2DynamicID))
		{
			MenuHideItem(ListEditGraffitiLegacyHelpCmd);
		}
		else
		{
			MenuHideItem(ListEditGraffitiHelpCmd);
		}
	}	
	return false;
}

static Boolean PrefsOnMenuEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gDeviceFlags.bits.treo)
	{
		switch(event->data.menu.itemID)
		{
			case 10300:
				event->data.menu.itemID = 10000;
				EvtAddEventToQueue(event); 
				break;
			case 10301:
				event->data.menu.itemID = 10001;
				EvtAddEventToQueue(event); 
				break;
			case 10302:
				event->data.menu.itemID = 10002;
				EvtAddEventToQueue(event); 
				break;
			case 10303:
				event->data.menu.itemID = 10003;
				EvtAddEventToQueue(event); 
				break;
			case 10304:
				event->data.menu.itemID = 10004;
				EvtAddEventToQueue(event); 
				break;
			case 10306:
				event->data.menu.itemID = 10006;
				EvtAddEventToQueue(event); 
				break;
			case 10307:
				event->data.menu.itemID = 10007;
				EvtAddEventToQueue(event); 
				break;	
		}
	}
	return false;
}

static Boolean PrefsOnFrmOpenEvent()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm = FrmGetActiveForm ();
	dia_save_state(); 
	dia_enable(frm, false);
	PrvPrefsInit (frm);
	FrmDrawForm (frm);
	if(globals->gDeviceFlags.bits.treo)
	{
		FrmSetMenu(FrmGetActiveForm(), GeneralOptionsMenuBarTreo);
	}
	return true;
}

Boolean PrefsHandleEvent (EventType * event)
{
	Boolean handled = false;
	
	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = PrefsOnCtlSelectEvent(event);
			break;
		case keyDownEvent:
			handled = PrefsOnKeyDownEvent(event);
			break;
		case menuOpenEvent:
			handled = PrefsOnMenuOpenEvent();
			break;
		case menuEvent:
			handled = PrefsOnMenuEvent(event);
			break;
		case frmOpenEvent:
			handled = PrefsOnFrmOpenEvent();
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	return (handled);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:    PrvPrefsInit
 *
 * DESCRIPTION: Initialize the dialog's ui.  Sets the database sort by
 * buttons.
 *
 * PARAMETERS:  frm
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date     	 Description
 *         ----   ----       -----------
 *         roger  08/02/95   Initial Revision
 *         FPa    11/23/00   Added PreferencesEnableTapDialingHeightGadget
 *							 handling
 *
 ***********************************************************************/
void PrvPrefsInit (FormPtr frm)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 rememberCategoryIndex;
	
	UInt16 rememberRecordIndex;
	UInt32 btVersion;
	UInt16 btLibRefNum; 
	Err error = errNone;
	Boolean is_loaded = false;

	error = FtrGet(btLibFeatureCreator, btLibFeatureVersion, &btVersion);
	if(error == errNone && !globals->gDeviceFlags.bits.treo)
	{
		if(SysLibFind(btLibName, &btLibRefNum))
		{ 
			error = SysLibLoad(sysFileTLibrary, sysFileCBtLib, &btLibRefNum);
		}
		if(error == errNone)
		{
			CustomShowObjectSmp(PreferencesAutoBluetooth);
			CtlSetValue(CustomGetObjectPtrSmp(PreferencesAutoBluetooth), (globals->gAutoBluetooth != 0));
		}
	}		
	
	rememberCategoryIndex = FrmGetObjectIndex (frm, PreferencesRememberCategoryCheckbox);
	rememberRecordIndex = FrmGetObjectIndex (frm, PreferencesRememberRecordCheckbox);
	CtlSetValue(FrmGetObjectPtr (frm, rememberCategoryIndex), globals->RememberLastCategory);
	
	CtlSetValue(FrmGetObjectPtr (frm, rememberRecordIndex), globals->gRememberLastContact);
	
	//init Full-Text Global Find checkbox
	
	{
		Int16 prefsVersion;
		UInt16 prefsSize;
		AddrExtPreferenceType prefs;
		Boolean fullText=false, oneHanded = true;

		prefsSize = 0;
		PrefGetAppPreferences (CREATORID, addrPrefExtID, NULL, &prefsSize, true);
		if(prefsSize>0)
		{
			prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefExtID, &prefs, &prefsSize, true);
		}
		else
		{
			prefsVersion=noPreferenceFound;
		}
		if (prefsVersion != noPreferenceFound)
		{
			fullText=prefs.FullTextSearch;
		}	
		CtlSetValue(FrmGetObjectPtr (FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), PreferencesFullText)), fullText);
	}
	
	if(globals->gCalendarCrID == 0)
	{
#ifdef CONTACTS
		SetCrIDField(PreferencesCalendarCrID, NEW_CALENDAR);
#else
		SetCrIDField(PreferencesCalendarCrID, OLD_CALENDAR);
#endif
	}
	else
	{
		SetCrIDField(PreferencesCalendarCrID, globals->gCalendarCrID);
	}
	if(globals->gMemosCrID == 0)
	{
#ifdef CONTACTS
		SetCrIDField(PreferencesMemosCrID, NEW_MEMOS);
#else
		SetCrIDField(PreferencesMemosCrID, OLD_MEMOS);
#endif
	}
	else
	{
		SetCrIDField(PreferencesMemosCrID, globals->gMemosCrID);
	}
	if(globals->gTasksCrID == 0)
	{
#ifdef CONTACTS
		SetCrIDField(PreferencesTasksCrID, NEW_TASKS);
#else
		SetCrIDField(PreferencesTasksCrID, OLD_TASKS);
#endif
	}
	else
	{
		SetCrIDField(PreferencesTasksCrID, globals->gTasksCrID);
	}	
		
	RestoreDialer();
}


/***********************************************************************
 *
 * FUNCTION:    PrvPrefsSave
 *
 * DESCRIPTION: Write the renamed field labels
 *
 * PARAMETERS:  frm
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *				Name		Date		Description
 *				----		----		-----------
 *				roger		8/2/95	Initial Revision
 *				jmp		11/02/99	Don't reset globals->CurrentRecord to zero if it's been set to
 *										noRecord.  Fixes bug #23571.
 *				gap		10/27/00	actually, when the records are resorted, the current
 *										selection should always be cleared.
 *				MB		26/01/04	Update address list form if highres setting changes
 *
 ***********************************************************************/
Boolean PrvPrefsSave ()
{
	globalVars* globals = getGlobalsPtr();
	UInt32 calendarID, tasksID, memosID;
	
	calendarID = GetCrIDField(PreferencesCalendarCrID);
	if(calendarID == 0)
	{
		FrmCustomAlert(PreferencesCrIDAlert, "Calendar", 0, 0);
		return false;
	}
	tasksID = GetCrIDField(PreferencesTasksCrID);
	if(tasksID == 0)
	{
		FrmCustomAlert(PreferencesCrIDAlert, "Tasks", 0, 0);
		return false;
	}
	memosID = GetCrIDField(PreferencesMemosCrID);
	if(memosID == 0)
	{
		FrmCustomAlert(PreferencesCrIDAlert, "Memos", 0, 0);
		return false;
	}
	
	
	{
		UInt16 prefsSize;
		AddrLinkPreferenceType linkPrefs;
		
		// Write the state information.
		globals->gCalendarCrID=linkPrefs.calendarCrID = calendarID;
		globals->gMemosCrID=linkPrefs.memosCrID = memosID;
		globals->gTasksCrID=linkPrefs.tasksCrID = tasksID;
		
		prefsSize=sizeof(AddrLinkPreferenceType);
		
		PrefSetAppPreferences (CREATORID, addrPrefLinkID, 1, &linkPrefs,
							   prefsSize, true);
		
	}
	
	globals->RememberLastCategory = CtlGetValue(
	FrmGetObjectPtr (
		FrmGetActiveForm(),
		FrmGetObjectIndex
			(FrmGetActiveForm(),
			PreferencesRememberCategoryCheckbox)));

	globals->gRememberLastContact = CtlGetValue(
	FrmGetObjectPtr (
		FrmGetActiveForm(),
		FrmGetObjectIndex
			(FrmGetActiveForm(),
			PreferencesRememberRecordCheckbox)));

	globals->gAutoBluetooth = CtlGetValue(
	CustomGetObjectPtrSmp(PreferencesAutoBluetooth));
	
	//save search option
	{
		UInt16 prefsSize;
		Int16 prefsVersion;
		AddrExtPreferenceType prefs;
		
		// Write the preferences / saved-state information.
		prefsSize=0;
		PrefGetAppPreferences (CREATORID, addrPrefExtID, NULL, &prefsSize, true);
		if(prefsSize>0)
		{
			prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefExtID, &prefs, &prefsSize, true);
		}
		else
		{
			prefsVersion=noPreferenceFound;
		
		}
		if (prefsVersion == noPreferenceFound)
		{
			prefs.addrListHighRes=false;
			prefsSize=sizeof(AddrExtPreferenceType);
		}
		// Write the state information.
		prefs.FullTextSearch = 	CtlGetValue(FrmGetObjectPtr (FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), PreferencesFullText)));

		PrefSetAppPreferences (CREATORID, addrPrefExtID, 1, &prefs,
							   prefsSize, true);

		}
	return true;
}
