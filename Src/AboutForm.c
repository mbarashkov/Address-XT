#include <PalmOS.h>
#include "AddrTools.h"
#include "AddrTools2.h"
#include "AboutForm.h"
#include "Address.h"
#include "globals.h"
#include "dia.h"
#include "syslog.h"
#include "Reg.h"


void AboutInitForm()
{
	globalVars* globals = getGlobalsPtr();
	Char lStr[31], userField[63], lStr2[255];
	Char userName[dlkUserNameBufSize];
	Char trstr[255];
	Char *lP;
	MemHandle lH;
	jpgClose();
	lH=DmGetResource('tver', 1000);
	jpgInit();
	lP=(Char*)MemHandleLock(lH);
	StrCopy(lStr, "Version ");
	StrCat(lStr, lP);
	DmReleaseResource(lH);
	MemHandleUnlock(lH);
	FldSetTextPtrSmp(AboutVersionField, lStr);
	
	//Get HotSync User name
	DlkGetSyncInfo(0, 0, 0, userName, 0, 0);
	StrCopy(userField, "HotSync name: ");
	StrCat(userField, userName);
	FldSetTextPtrSmp(AboutHotSyncNameField, userField);
	
	if(!CheckRegistration(globals->adxtLib2Ref))
	{
		CustomShowObjectSmp(AboutReg2);
		CustomShowObjectSmp(AboutReg3);
		CustomShowObjectSmp(AboutReg4);
		CustomShowObjectSmp(AboutReg5);
		CustomShowObjectSmp(AboutTrialDay);
		if(!globals->gTrialExpired)
		{
			StrCopy(trstr, "Day ");
			StrIToA(lStr2, globals->gTrialDays);
			StrCat(trstr, lStr2);
			StrCopy(lStr2, " of 30 day free trial");
			StrCat(trstr, lStr2);
		}
		else
		{
			StrCopy(trstr, "30-day trial period has ended!");
		}
		FldSetTextPtrSmp(AboutTrialDay, trstr);
	}
	else
	{
		CustomHideObjectSmp(AboutReg2);
		CustomHideObjectSmp(AboutReg3);
		CustomHideObjectSmp(AboutReg4);
		CustomHideObjectSmp(AboutReg5);
		CustomHideObjectSmp(AboutTrialDay);
	}	
	if(!globals->gDeviceFlags.bits.treoWithSendKeys)
		PrefSetPreference(prefHard1CharAppCreator, globals->gDialerCreatorID);			
}

static Boolean AboutOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
	case AboutOkButton:
		SetDialer();
		if(globals->gTrialExpired)
		{
			FrmAlert(alertExpiredTrial);
		}
		else
		{
			ToolsLeaveForm();
			dia_restore_state();
		}
		handled = true;
		break;
	case AboutSNButton:
		globals->gSNCalled = true;
		FrmPopupForm(SerialNumberDialog);
		handled=true;
		break;
	default:
		break;
	}
	return handled;
}

static Boolean AboutOnWinEnterEvent()
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gSNCalled == true)
	{
		globals->gSNCalled = false;
		AboutInitForm();
	}
	return false;
}

static Boolean AboutOnKeyDownEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			ToolsLeaveForm();
			dia_restore_state();
			return true;
		}		   	 	
	}  
	return false;
}

static Boolean AboutOnFrmOpenEvent()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm;
	globals->gSNCalled = false;
	frm = FrmGetFormPtr(AboutForm);
	dia_save_state(); 
	dia_enable(frm, false);
	FrmDrawForm (frm);
	//PRINTDEBUG(0);
	AboutInitForm();
	//PRINTDEBUG(999);
	return true;
}

Boolean AboutHandleEvent (EventType * event)
{
	Boolean handled = false;
	
	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = AboutOnCtlSelectEvent(event);
			break;			
		case winEnterEvent:
			handled = AboutOnWinEnterEvent();			
			break;
		case keyDownEvent:
			handled = AboutOnKeyDownEvent(event);			
			break;
		case frmOpenEvent:
			handled = AboutOnFrmOpenEvent();			
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	return (handled);
}