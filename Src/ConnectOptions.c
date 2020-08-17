#include <PalmOS.h>
#include "ConnectOptions.h"
#include <DefaultHelperLibTypes.h>
#include <DefaultHelperLib.h>
#include "AddrPrefs.h"
#include "globals.h"
#include "Address.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "dia.h"

#define DIAL_INDEX	0
#define SMS_INDEX 	1
#define EMAIL_INDEX 2
#define WEB_INDEX 	3
#define IM_INDEX 	4
#define MAP_INDEX 	5


void ConnectOptionsInitForm();
void ConnectOptionsSave();

void ConnectOptionsSave()
{
	UInt32 serviceClassID[connectoptions_num] = 
	{
		kHelperServiceClassIDVoiceDial,
		kHelperServiceClassIDSMS,
		kHelperServiceClassIDEMail,
		'mapH',
		'webH',
		'imsH'
	};
	UInt16 prefsSize = sizeof(AddrDialOptionsPreferenceType);
	UInt16 i, sel;
	UInt16 refNum = 0;
	AddrDialOptionsPreferenceType prefs;
	globalVars* globals = getGlobalsPtr();
	Char* text = NULL;
	if(!CustomFldIsEmptySmp(ConnectOptionsNumberPrefixField))
	{
		text = CustomFldGetTextPtrSmp(ConnectOptionsNumberPrefixField);
	}
	MemSet(prefs.dialPrefix, 32, 0);
	if(text)
		StrCopy(prefs.dialPrefix, text);
	for(i = 0; i < connectoptions_num; i++)
	{
		sel = CustomLstGetSelection(globals->listID[i]);
		prefs.creatorID[i] = globals->ContactSettings[i].crIDs[sel];
	}	
	if(prefs.dialPrefix[0])
	{
		prefs.dialPrefixActive = CtlGetValue(
		CustomGetObjectPtrSmp (	ConnectOptionsNumberPrefix ));
	}
	else
	{
		prefs.dialPrefixActive = false;
	}
	PrefSetAppPreferences (CREATORID, addrPrefDialOptionsDataID, 1, &prefs,
						   prefsSize, true);
	
	if(SysLibFind(defaultHelperLibName, &refNum) == sysErrLibNotFound )
	{
		SysLibLoad(defaultHelperLibType, defaultHelperLibCreator, &refNum);
	} 
	if(refNum)
	{
		if(DefaultHelperLibOpen(refNum) == 0)
		{
			for(i = 0; i < connectoptions_num; i++)
			{
				CustomLstGetSelection(globals->listID[i]);
				if(prefs.creatorID[i] != googleMapCreatorID)
					DefaultHelperLibSetDefaultHelper(refNum, serviceClassID[i], prefs.creatorID[i]);
			}				
			DefaultHelperLibClose(refNum);
		}
	}
}

void ConnectOptionsInitForm()
{
   	AddrDialOptionsPreferenceType prefs;
	UInt16 prefsSize;
	Int16 prefsVersion;
	UInt16 count, optionIndex = 0;
	globalVars* globals = getGlobalsPtr();
	EventType event;
   	//fill lists
	UInt16 triggerID[connectoptions_num] = 
	{
		ConnectOptionsDialTrigger,
		ConnectOptionsSMSTrigger,
		ConnectOptionsEmailTrigger,
		ConnectOptionsMapTrigger,
		ConnectOptionsWebTrigger,
		ConnectOptionsIMTrigger
	};
	FillConnectOptions();
	//load saved options
	prefsSize = sizeof (AddrDialOptionsPreferenceType);
	prefsVersion =  PrefGetAppPreferences (CREATORID, addrPrefDialOptionsDataID, &prefs, &prefsSize, true);
	if(prefsVersion <= noPreferenceFound)
	{
		SetDefaultConnectOptions(&prefs);
	}
	CustomEditableFldSetTextPtrSmp(ConnectOptionsNumberPrefixField, prefs.dialPrefix);
	CtlSetValue(CustomGetObjectPtrSmp(ConnectOptionsNumberPrefix), prefs.dialPrefixActive);
	FrmDrawForm(FrmGetActiveForm());		
		
	for(optionIndex = 0; optionIndex < connectoptions_num; optionIndex ++)
 	{
		Boolean found = false;
		UInt16 foundIndex;
		LstSetListChoices(CustomGetObjectPtrSmp(globals->listID[optionIndex]), globals->ContactSettings[optionIndex].strings, globals->ContactSettings[optionIndex].num);
		LstSetHeight(CustomGetObjectPtrSmp(globals->listID[optionIndex]), globals->ContactSettings[optionIndex].num);
		for(count = 0; count < globals->ContactSettings[optionIndex].num; count ++)
		{
			if(globals->ContactSettings[optionIndex].crIDs[count] == prefs.creatorID[optionIndex])
			{
				found = true;
				foundIndex = count;
				break;
			}
		}
		if(!found)
			foundIndex = globals->ContactSettings[optionIndex].num - 1;
		LstSetSelection(CustomGetObjectPtrSmp(globals->listID[optionIndex]), foundIndex);
		event.eType = popSelectEvent;
		event.data.popSelect.controlID = triggerID[optionIndex];
		event.data.popSelect.controlP = CustomGetObjectPtrSmp(triggerID[optionIndex]);
		event.data.popSelect.listID = globals->listID[optionIndex];
		event.data.popSelect.listP = CustomGetObjectPtrSmp(globals->listID[optionIndex]);
		event.data.popSelect.selection = foundIndex;
		event.data.popSelect.priorSelection = foundIndex;
		
		EvtAddEventToQueue(&event);
  	}
}

static Boolean ConnectOptionsOnCtlSelectEvent(EventPtr event)
{
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
		case ConnectOptionsOKButton:
			ConnectOptionsSave();
			FrmUpdateForm (DialListDialog, updatePrefs);
			FrmReturnToForm(0);
			handled = true;
			break;
		case ConnectOptionsCancelButton:
			FrmReturnToForm(0);
			handled=true;
			break;
		default:
			break;
	}
	return handled;
}

static Boolean ConnectOptionsOnFldEnter()
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gDeviceFlags.bits.treo)
		HsGrfSetStateExt(false, true, true, false, false, false);
	return false;
}

static Boolean ConnectOptionsOnFrmOpenEvent()
{
	FormPtr frm = FrmGetFormPtr(ConnectOptionsDialog);
	dia_save_state(); 
	dia_enable(frm, false);
	FrmDrawForm (frm);
	ConnectOptionsInitForm();
	return true;
}

Boolean ConnectOptionsHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = ConnectOptionsOnCtlSelectEvent(event);
			break;			
		case frmOpenEvent:
			handled = ConnectOptionsOnFrmOpenEvent();
			break;	
		case fldEnterEvent:
			handled = ConnectOptionsOnFldEnter();
			break;			
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}	
	return (handled);
}
