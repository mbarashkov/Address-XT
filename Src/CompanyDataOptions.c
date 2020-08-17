#include <PalmOS.h>
#include "CompanyDataOptions.h"
#include "AddrPrefs.h"
#include "globals.h"
#include "Address.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "dia.h"

void CDataInitForm();
void CDataSave();

void CDataSave()
{
	UInt16 prefsSize;
	Int16 prefsVersion;
	AddrCDataPreferenceType prefs;

	// Write the preferences / saved-state information.
	prefsSize=0;
	PrefGetAppPreferences (CREATORID, addrPrefCDataID, NULL, &prefsSize, true);
	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefCDataID, &prefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	
	}
	if (prefsVersion == noPreferenceFound)
	{
		prefs.reserved8=false;
		prefs.reserved9=0;
		prefs.reserved10=0;
		
		prefsSize=sizeof(AddrCDataPreferenceType);
	}
	// Write the state information.
	prefs.EmptyOnly = 	CtlGetValue(FrmGetObjectPtr (FrmGetFormPtr(CompanyDataPrefDialog), FrmGetObjectIndex(FrmGetFormPtr(CompanyDataPrefDialog), CompanyDataEmptyCheck)));

	PrefSetAppPreferences (CREATORID, addrPrefCDataID, 1, &prefs,
						   prefsSize, true);
}

void CDataInitForm()
{
	Int16 prefsVersion;
	UInt16 prefsSize;
	AddrCDataPreferenceType prefs;
	Boolean emptyOnly=false;

	prefsSize = 0;
	PrefGetAppPreferences (CREATORID, addrPrefCDataID, NULL, &prefsSize, true);
	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefCDataID, &prefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	}
	if (prefsVersion != noPreferenceFound)
	{
		emptyOnly=prefs.EmptyOnly;
	}	
	CtlSetValue(FrmGetObjectPtr (FrmGetFormPtr(CompanyDataPrefDialog), FrmGetObjectIndex(FrmGetFormPtr(CompanyDataPrefDialog), CompanyDataEmptyCheck)), emptyOnly);
}

static Boolean CDataOnCtlSelectEvent(EventPtr event)
{
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
		case CompanyDataPrefOK:
			CDataSave();
			FrmReturnToForm(0);
			handled = true;
			break;
		case CompanyDataPrefCancel:
			FrmReturnToForm(0);
			handled=true;
			break;
		default:
			break;
	}
	return handled;
}

static Boolean CDataOnFrmOpenEvent()
{
	FormPtr frm = FrmGetFormPtr(CompanyDataPrefDialog);
	dia_save_state(); 
	dia_enable(frm, false);
	FrmDrawForm (frm);
	CDataInitForm();
	return true;
}

static Boolean CDataOnKeyDownEvent(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
	{
		if(globals->gDeviceFlags.bits.treoWithPhoneKeyOnly && event->data.keyDown.chr == vchrHard1)
		{
			PrefSetPreference(prefHard1CharAppCreator, globals->gDialerCreatorID);
			StartPhone();
		}
		return true;
	}
	if(IsFiveWayNavEvent(event) && !globals->gNavigation)
	{
		if (NavKeyPressed(event, Select))
		{
			CDataSave();
			FrmReturnToForm(0);
			return true;
		}		   	 	
	}  
	return handled;
}

Boolean CDataHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = CDataOnCtlSelectEvent(event);
			break;			
		case frmOpenEvent:
			handled = CDataOnFrmOpenEvent();
			break;				
		case keyDownEvent:
			handled = CDataOnKeyDownEvent(event);
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}	
	return (handled);
}
