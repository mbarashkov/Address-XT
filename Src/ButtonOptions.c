#include <PalmOS.h>
#include "ButtonOptions.h"
#include "AddrPrefs.h"
#include "Address.h"
#include "globals.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "dia.h"
#include "favorites.h"

void BOInitForm();
void BOSave();

void BOSave()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 prefsSize;
	AddrBtnPreferenceType prefs;
	
	Boolean mapped;

	if(globals->gDeviceFlags.bits.treo)
	{
		DmOpenRef favs = favorites_open(true);
		if(favs)
		{
			mapped = self_check(favs, 0, CREATORID);	
			DmCloseDatabase(favs);
		}
		else
		{
			mapped = false;
		}
	}
	else
	{
		mapped=(globals->PrevCreatorID==CREATORID);
	}
	if(mapped!=CtlGetValue(FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex (FrmGetActiveForm(), PreferencesMap))))
	{
		//map according to new settings
		if(globals->gDeviceFlags.bits.treo)
		{
			DmOpenRef favs = favorites_open(false);
			mapped = CtlGetValue(FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex (FrmGetActiveForm(), PreferencesMap)));
			if(favs)
			{
				if(mapped == true)
				{
					favorites_write(favs, 0, "Address XT", CREATORID,
									0);
				}
				else
				{
					Boolean create_new;
					Int16 pos = find_record(favs, 0, &create_new);
					if(!create_new && pos != -1)
					{
						Err err = DmRemoveRecord(favs, pos);
						err += 1;
					}
				}
				DmCloseDatabase(favs);
			}
		}
		else
		{
			if(mapped==true)
			{
				//set to standard address book
				globals->PrevCreatorID='addr';
			}
			else
			{
				//set to Address XT
				globals->PrevCreatorID=CREATORID;
			}
		}
	}
	
	// Write the state information.
	//globals->JogDialUpDown=prefs.JogDialUpDown = CustomLstGetSelection(ButtonJogDialList);
	globals->FiveWayUpDown=prefs.FiveWayUpDown = CustomLstGetSelection(ButtonFiveWayList);
	//globals->StdUpDown=prefs.StdUpDown = CustomLstGetSelection(ButtonStdList);
	prefsSize=sizeof(AddrBtnPreferenceType);
	
	PrefSetAppPreferences (CREATORID, addrPrefBtnID, 1, &prefs,
						   prefsSize, true);
}

void BOInitForm()
{
	globalVars* globals = getGlobalsPtr();
	Int16 prefsVersion;
	UInt16 prefsSize;
	UInt32 version;
	AddrBtnPreferenceType prefs;
	UInt16 fiveWay/*, jogDial, stdDial*/;
	ListType *lFilterList1Ptr=(ListType*)GetObjectPtrSmp(ButtonFiveWayList);
	/*ListType *lFilterList2Ptr=(ListType*)GetObjectPtrSmp(ButtonJogDialList);
	ListType *lFilterList3Ptr=(ListType*)GetObjectPtrSmp(ButtonStdList);
	*/
	if(FtrGet (navFtrCreator, navFtrVersion, &version)==errNone)
		fiveWay=FIVEWAYCONTACTS;
	else
		fiveWay=FIVEWAYPAGE;
	/*jogDial=JOGDIALPAGE;
	stdDial=STDPAGE;
	*/prefsSize = 0;
	PrefGetAppPreferences (CREATORID, addrPrefBtnID, NULL, &prefsSize, true);
	if(prefsSize>0)
	{
	prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefBtnID, &prefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	}
	if (prefsVersion != noPreferenceFound)
	{
		fiveWay=prefs.FiveWayUpDown;
		/*jogDial=prefs.JogDialUpDown;
		stdDial=prefs.StdUpDown;*/
	}	
	if(fiveWay!=FIVEWAYPAGE && fiveWay!= FIVEWAYRECORD && fiveWay!=FIVEWAYCONTACTS)
	{
		if(FtrGet (navFtrCreator, navFtrVersion, &version)==errNone)
			fiveWay=FIVEWAYCONTACTS;
		else
			fiveWay=FIVEWAYPAGE;
	}
	/*if(jogDial!=JOGDIALPAGE && jogDial!= JOGDIALRECORD)
		jogDial=JOGDIALPAGE;
	if(stdDial!=STDPAGE && stdDial!= STDRECORD)
		stdDial=STDPAGE;
	*/
	if(fiveWay == FIVEWAYCONTACTS)
		fiveWay = FIVEWAYRECORD;
	StrCopy(globals->gStr1, LstGetSelectionText(lFilterList1Ptr, fiveWay));
	/*StrCopy(globals->gStr2, LstGetSelectionText(lFilterList1Ptr, jogDial));
	StrCopy(globals->gStr3, LstGetSelectionText(lFilterList3Ptr, stdDial));
	*/
	LstSetSelection(lFilterList1Ptr, fiveWay);
	/*LstSetSelection(lFilterList2Ptr, jogDial);
	LstSetSelection(lFilterList3Ptr, stdDial);
	*/
	CustomSetCtlLabelPtrSmp(ButtonFiveWayTrigger, globals->gStr1);
	/*CustomSetCtlLabelPtrSmp(ButtonJogDialTrigger, globals->gStr2);
	CustomSetCtlLabelPtrSmp(ButtonStdTrigger, globals->gStr3);
	*/
	/*if(globals->gFiveWay)
	{
		CustomHideObjectSmp(ButtonStdTrigger);
		CustomHideObjectSmp(ButtonStdLabel);		
		CustomShowObjectSmp(ButtonFiveWayTrigger);
		CustomShowObjectSmp(ButtonFiveWayLabel);
	}
	else
	{
		CustomShowObjectSmp(ButtonStdTrigger);
		CustomShowObjectSmp(ButtonStdLabel);		
		CustomHideObjectSmp(ButtonFiveWayTrigger);
		CustomHideObjectSmp(ButtonFiveWayLabel);
	}*/
	
	/*if(!globals->gJogDial)
	{
		CustomHideObjectSmp(ButtonJogDialLabel);
		CustomHideObjectSmp(ButtonJogDialTrigger);
	}
	else
	{
		CustomShowObjectSmp(ButtonJogDialLabel);
		CustomShowObjectSmp(ButtonJogDialTrigger);
		CustomShowObjectSmp(ButtonStdTrigger);
		CustomShowObjectSmp(ButtonStdLabel);		
	}
	*/
	if(globals->gDeviceFlags.bits.treo)
	{
		DmOpenRef favs = favorites_open(true);
		if(favs)
		{
			CtlSetValue(FrmGetObjectPtr (FrmGetActiveForm(), FrmGetObjectIndex (FrmGetActiveForm(), PreferencesMap)), self_check(favs, 0, CREATORID));	
			CtlSetLabel(FrmGetObjectPtr (FrmGetActiveForm(), FrmGetObjectIndex (FrmGetActiveForm(), PreferencesMap)), "Map to Favorites");	
			DmCloseDatabase(favs);
		}
	}
	else
	{			
		CtlSetValue(FrmGetObjectPtr (FrmGetActiveForm(), FrmGetObjectIndex (FrmGetActiveForm(), PreferencesMap)), globals->PrevCreatorID==CREATORID);	
	}
	
	RestoreDialer();	
}

static Boolean ButtonOptionsOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
	case ButtonOptionsOkButton:
		SetDialer();
		BOSave();
		FrmReturnToForm(0);
		handled = true;
		break;
	case ButtonOptionsCancelButton:
		SetDialer();
		FrmReturnToForm(0);
		handled=true;
		break;
	default:
		break;
	}
	return handled;
}

static Boolean ButtonOptionsOnFrmOpenEvent()
{
	FormPtr frm = FrmGetFormPtr(ButtonOptionsDialog);
	dia_save_state(); 
	dia_enable(frm, false);
	FrmDrawForm (frm);
	BOInitForm();
	return true;
}

static Boolean ButtonOptionsOnKeyDownEvent(EventType* event)
{
	Boolean handled = false;
	globalVars* globals = getGlobalsPtr();
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			BOSave();
			FrmReturnToForm(0);
			handled=true;
		}		   	 	
	}  
	return handled;
}

Boolean BOHandleEvent (EventType * event)
{
	Boolean handled = false;
	
	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = ButtonOptionsOnCtlSelectEvent(event);
			break;
		case frmOpenEvent:
			handled = ButtonOptionsOnFrmOpenEvent();
			break;			
		case keyDownEvent:
			handled = ButtonOptionsOnKeyDownEvent(event);
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	
	return (handled);
}
