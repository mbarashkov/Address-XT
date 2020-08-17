#include <PalmOS.h>
#include "AddressListOptions.h"
#include "AddrPrefs.h"
#include "globals.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "Address.h"
#include "dia.h"


void ALOInitForm();
void ALOSave();
void RecentOnEnabledSelector();
void SetEnabled(UInt16 enabled);
Boolean resort(Boolean force);

	
static Boolean resort_form_handler(EventType *event)
{
	return dia_handle_event(event, NULL);
}

Boolean resort(Boolean force)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 sortMode, sortByCompany;
	sortMode = CustomLstGetSelection(AddressListOptionsSortList);
	
	switch(sortMode)
	{
		case 0:
			sortByCompany = PreferencesLastName;
			break;
		case 1:
			sortByCompany = PreferencesFirstLast;
			break;
		case 2:
			sortByCompany = PreferencesCompanyName;
			break;
		case 3:
			sortByCompany = PreferencesCompanyFirst;
			break;
		case 4:
			sortByCompany = PreferencesCompanyTitle;
			break;
		case 5:
			sortByCompany = PreferencesLastTitle;
			break;	
		default:
			sortByCompany = PreferencesLastName;
	}
	
	if (sortByCompany != globals->SortByCompany || force)
	{
		// Put up the sorting message dialog so the user knows what's going on
		// while the sort locks up the device.
		FormPtr curFormP = FrmGetActiveForm ();
		FormPtr formP = FrmInitForm (SortingMessageDialog);
		FrmSetActiveForm (formP);
		dia_enable(formP, false);
		FrmSetEventHandler(formP, resort_form_handler);
		FrmDrawForm (formP);

		// Peform the sort.
		globals->SortByCompany = sortByCompany;
		AddrDBChangeSortOrder(globals->adxtLibRef, globals->AddrDB, globals->SortByCompany);//temporarily by MB
		// clear the current selection 
		globals->CurrentRecord = noRecord;
		globals->TopVisibleRecord=0;
		DmSeekRecordInCategory(globals->AddrDB, &(globals->TopVisibleRecord), 0, dmSeekForward, globals->CurrentCategory);
		
		// post an event to update the form
		//FrmUpdateForm (ListView, updatePrefs);

		// Remove the sorting message.
		FrmEraseForm (formP);
		FrmDeleteForm (formP);
		FrmSetActiveForm (curFormP);
		return true;
	}
	return false;
	
}


void ALOSave()
{
	globalVars* globals = getGlobalsPtr();
	//save sort mode
	UInt16 prefsSize;
	Int16 prefsVersion;
	UInt32 showNamesOnly, touchMode;
	AddrSearchPreferenceType searchPrefs;
	
	if(globals->gRefresh)
	{
		FrmUpdateForm (ListView, updatePrefs);
	}
	
	if(!globals->gRefresh)
	{
		globals->gRefresh = resort(false);
		if(globals->gRefresh)
			FrmUpdateForm (ListView, updatePrefs);
	}
	
	
	showNamesOnly = CtlGetValue(FrmGetObjectPtr (FrmGetActiveForm(),
													FrmGetObjectIndex (FrmGetActiveForm(), AddressListOptionsShowNameOnly)));
	touchMode = CtlGetValue(FrmGetObjectPtr (FrmGetActiveForm(),
													FrmGetObjectIndex (FrmGetActiveForm(), AddressListOptionsTouchMode)));
	if(showNamesOnly != globals->gShowNamesOnly || touchMode != globals->gTouchMode)
	{
		if(!globals->gRefresh)
		{
			FrmUpdateForm(ListView, updatePrefs);	
			globals->gRefresh = true;
		}
	}
		
	globals->gShowNamesOnly = showNamesOnly;
	globals->gTouchMode = touchMode;
	
	//save tap dialing options
	globals->EnableTapDialing = CtlGetValue(FrmGetObjectPtr (FrmGetActiveForm(),
													FrmGetObjectIndex (FrmGetActiveForm(), AddressListOptionsTapDialingCheckbox)));
	//save one-handed search option
	
	prefsSize=0;
	PrefGetAppPreferences (CREATORID, addrPrefExtID, NULL, &prefsSize, true);
	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefSearchID, &searchPrefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	}
	if (prefsVersion == noPreferenceFound)
	{
		prefsSize=sizeof(AddrSearchPreferenceType);
	}
	// Write the state information.
	globals->gOneHanded = searchPrefs.OneHandedSearch = CtlGetValue(FrmGetObjectPtr (FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsOneHanded)));
	//if(CtlGetValue(FrmGetObjectPtr (FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsAdvancedFind))) == 1)
	//	globals->gAdvancedFind = searchPrefs.AdvancedFind = ADVANCEDFIND_ON;
	//else 
		globals->gAdvancedFind = searchPrefs.AdvancedFind = ADVANCEDFIND_OFF;
	searchPrefs.reserved2 = 0;
	searchPrefs.reserved3 = 0;
	searchPrefs.reserved4 = 0;
	searchPrefs.reserved5 = 0;
	searchPrefs.reserved6 = 0;
	searchPrefs.reserved7 = 0;
	searchPrefs.reserved8 = 0;
	
	PrefSetAppPreferences (CREATORID, addrPrefSearchID, 1, &searchPrefs,
						   prefsSize, true);
		
	//save recent contacts options
	{
		UInt16 prefsSize, selection;
		Int16 prefsVersion;
		UInt32 numRecent=0;
		UInt16 enabled=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsRecentEnableCheckbox));
		UInt16 showAll=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsRecentShowAll));
		AddrRecentPreferenceType prefs;
		Boolean refreshRecent = false;
		// Write the preferences / saved-state information.
		prefsSize=0;
		PrefGetAppPreferences (CREATORID, addrPrefRcntID, NULL, &prefsSize, true);
		if(prefsSize>0)
		{
			prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefRcntID, &prefs, &prefsSize, true);
		}
		else
		{
			prefsVersion=noPreferenceFound;
		
		}
		if (prefsVersion == noPreferenceFound)
		{
			prefs.Number=0;
		}
		// Write the state information.
		if(prefs.Enabled != enabled)
			refreshRecent = true;
		prefs.Enabled=enabled;
		prefs.ShowAll=showAll;
		if(enabled)
		{
			selection=CustomLstGetSelection(AddressListOptionsRecentNumberList);
			switch(selection)
			{
				case 0:
					prefs.Number=5;
					break;
				case 1:
					prefs.Number=10;
					break;
				case 2:
					prefs.Number=20;
					break;
				case 3:
					prefs.Number=50;
					break;
				case 4:
					prefs.Number=100;
					break;		
			}	
		}
		prefsSize=sizeof(AddrRecentPreferenceType);
		globals->gMaxRecent=prefs.Number;
		globals->gEnabledRecent=prefs.Enabled;
		globals->gShowAll=prefs.ShowAll;
		PrefSetAppPreferences (CREATORID, addrPrefRcntID, 2, &prefs,
							   prefsSize, true);
		TrimRecentDB(globals->adxtLibRef, globals->gMaxRecent);
		if(!globals->gRefresh && refreshRecent)
		{
			FrmUpdateForm (ListView, updateRecent);
			globals->gRefresh = true;
		}
	}	
	//save delimiter information
	{
		UInt16 delimiterIndex = CustomLstGetSelection(AddressListOptionsDelimiterList);
		UInt8 delimiter = (UInt8)delimiterIndex;
		
		if(delimiter != globals->gDelimiter)
		{
			if(!globals->gRefresh)
			{
				FrmUpdateForm(ListView, updatePrefs);	
				globals->gRefresh = true;
			}
		}
		
		globals->gDelimiter = delimiter;
	}
}

void ALOInitForm()
{
	globalVars* globals = getGlobalsPtr();
	ListType *sortList=(ListType*)GetObjectPtrSmp(AddressListOptionsSortList);
	ListType *delimiterList=(ListType*)GetObjectPtrSmp(AddressListOptionsDelimiterList);
	Int16 prefsVersion;
	UInt16 prefsSize;
	Boolean showAll=false;
	AddrRecentPreferenceType prefs;
	AddrSearchPreferenceType searchPrefs;
	Boolean enabled=false;
	UInt16 number=0;
	ListType *recentListPtr=(ListType*)GetObjectPtrSmp(AddressListOptionsRecentNumberList);
	Boolean oneHanded = true;
	Boolean advancedFind = false;
	
	UInt16 sortModeIndex, delimiterIndex;
	
	globals->gRefresh = false;
	
	delimiterIndex = globals->gDelimiter;
	
	if(delimiterIndex >1)
	{
		delimiterIndex = 0;
	}
	LstSetSelection(delimiterList, delimiterIndex);
	
	StrCopy(globals->gDelimiterStr, LstGetSelectionText(delimiterList, delimiterIndex));
	
	CustomSetCtlLabelPtrSmp(AddressListOptionsDelimiterTrigger, globals->gDelimiterStr);

	
	switch(globals->SortByCompany)
	{
		case PreferencesLastName:
			sortModeIndex = 0;
			break;
		case PreferencesFirstLast:
			sortModeIndex = 1;
			break;
		case PreferencesCompanyName:
			sortModeIndex = 2;
			break;
		case PreferencesCompanyFirst:
			sortModeIndex = 3;
			break;
		case PreferencesCompanyTitle:
			sortModeIndex = 4;
			break;
		case PreferencesLastTitle:
			sortModeIndex = 5;
			break;	
		default:
			sortModeIndex = 0;
	}
	
	LstSetSelection(sortList, sortModeIndex);
	
	StrCopy(globals->gSortMode, LstGetSelectionText(sortList, sortModeIndex));
	
	CustomSetCtlLabelPtrSmp(AddressListOptionsSortTrigger, globals->gSortMode);
	
	
	
	prefsSize = 0;
	PrefGetAppPreferences (CREATORID, addrPrefRcntID, NULL, &prefsSize, true);
	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefRcntID, &prefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	}
	if (prefsVersion != noPreferenceFound)
	{
		enabled=prefs.Enabled;
		number=prefs.Number;
		if(prefsVersion==1)
		{
			showAll=false;
		}
		else
		{
			showAll=prefs.ShowAll;
		}
	}	
	
	
	if(number!=5 && number!=10 && number!=20 && number!=50 && number!=100)
		number=5; 
	StrIToA(globals->gROStr, number);
	
	switch(number)
	{
		case 5:
			LstSetSelection(recentListPtr, 0);
			break;
		case 10:
			LstSetSelection(recentListPtr, 1);
			break;
		case 20:
			LstSetSelection(recentListPtr, 2);
			break;
		case 50:
			LstSetSelection(recentListPtr, 3);
			break;
		case 100:
			LstSetSelection(recentListPtr, 4);
			break;	
	}
	CustomSetCtlLabelPtrSmp(AddressListOptionsRecentNumberTrigger, globals->gROStr);
	CtlSetValue(FrmGetObjectPtr (FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsRecentShowAll)), showAll);
	
	SetEnabled(enabled);
	
	//set tap-dialing checkbox
	if (!ToolsIsDialerPresent())
	{
		CustomHideObjectSmp(AddressListOptionsTapDialingCheckbox);		
	}
	else
	{
		CustomShowObjectSmp(AddressListOptionsTapDialingCheckbox);		
		CtlSetValue(FrmGetObjectPtr (FrmGetActiveForm(),FrmGetObjectIndex(FrmGetActiveForm(),  AddressListOptionsTapDialingCheckbox)), globals->EnableTapDialing);
	}

	CtlSetValue(FrmGetObjectPtr (FrmGetActiveForm(),FrmGetObjectIndex(FrmGetActiveForm(),  AddressListOptionsShowNameOnly)), globals->gShowNamesOnly);
	CtlSetValue(FrmGetObjectPtr (FrmGetActiveForm(),FrmGetObjectIndex(FrmGetActiveForm(),  AddressListOptionsTouchMode)), globals->gTouchMode);
	
	//set one-handed search checkbox
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
		oneHanded=searchPrefs.OneHandedSearch;
		if(searchPrefs.AdvancedFind == ADVANCEDFIND_OFF)
			advancedFind = false;
		else if(searchPrefs.AdvancedFind == ADVANCEDFIND_ON)
			advancedFind = true;
		else if(searchPrefs.AdvancedFind == 0)
		{
				advancedFind = false;
		}
	}	
	CtlSetValue(FrmGetObjectPtr (FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsOneHanded)), oneHanded);
	//CtlSetValue(FrmGetObjectPtr (FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsAdvancedFind)),advancedFind);

	//if(!globals->gFiveWay && !globals->gJogLeftRight)
	{
		//CustomHideObjectSmp(AddressListOptionsOneHanded);
	}
	//else
	//{
	//	CustomShowObjectSmp(AddressListOptionsOneHanded);
	//}
	
	RestoreDialer();
}

void SetEnabled(UInt16 enabled)
{
	CtlSetValue(FrmGetObjectPtr (FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsRecentEnableCheckbox)), enabled);
	if(enabled)
	{
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsRecentNumberLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsRecentNumberTrigger));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsRecentShowAll));
	}
	else
	{
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsRecentNumberLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsRecentNumberTrigger));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsRecentShowAll));
	}
}

void RecentOnEnabledSelector()
{
	UInt16 enabled=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), AddressListOptionsRecentEnableCheckbox));
	SetEnabled(enabled);
}


static Boolean ALOOnCtlSelectEvent(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
		case AddressListOptionsRecentEnableCheckbox:
			RecentOnEnabledSelector();
			handled=true;
			break;
		case AddressListOptionsResortNow:
			globals->gRefresh = resort(true);
			handled = true;
			break;
		case AddressListOptionsOkButton:
			SetDialer();
			ALOSave();
			FrmReturnToForm(0);
			handled = true;
			break;
		case AddressListOptionsCancelButton:
			SetDialer();
			FrmReturnToForm(0);
			handled=true;
			break;
		default:
			break;
	}
	return handled;
}

static Boolean ALOOnFrmOpenEvent()
{
	FormPtr frm = FrmGetActiveForm();
	dia_save_state(); 
	dia_enable(frm, false);
	FrmDrawForm (frm);
	ALOInitForm();
	return true;
}

static Boolean ALOOnKeyDownEvent(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			ALOSave();
			FrmReturnToForm(0);
			return true;
		}		   	 	
	}  
	return handled;
}

Boolean ALOHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = ALOOnCtlSelectEvent(event);
			break;
		case frmOpenEvent:
			handled = ALOOnFrmOpenEvent();
			break;
		case keyDownEvent:
			handled = ALOOnKeyDownEvent(event);
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	
	return (handled);
}
