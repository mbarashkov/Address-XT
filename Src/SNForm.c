#include <PalmOS.h>
#include <HsNavCommon.h>
#include <HsExt.h>
#include "AddrPrefs.h"
#include "Address.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "SNForm.h"
#include "globals.h"
#include "dia.h"

void SNOnOK();
void SNOnCancel();

void SNOnOK()
{
	globalVars* globals = getGlobalsPtr();
	Char lSN[255];
	if(FldEmptySmp(SNField))
		return;
	MemSet(lSN, 255, 0);
	
	StrCopy(lSN, (Char*)(FldGetTextPtr(FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), SNField)))));
	PrefSetAppPreferences(CREATORID, addrPrefRegID, 1, &lSN, StrLen(lSN), true);
	SetDialer();
	FrmReturnToForm(0);
}

void SNOnCancel()
{
	globalVars* globals = getGlobalsPtr();
	SetDialer();
	FrmReturnToForm(0);
}

static Boolean SNOnCtlSelectEvent(EventType* event)
{
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
	case SNOK:
		SNOnOK();
		handled=true;
		break;
	case SNCancel:
		SNOnCancel();
		handled=true;
		break;	
	default:
		break;
	}
	return handled;
}

static Boolean SNOnKeyDownEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			SNOnOK();
			return true;
		}		   	 	
	}  
	return handled;
}

static Boolean SNOnFrmOpenEvent()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm = FrmGetActiveForm ();
	dia_enable(frm, false);
	FrmDrawForm (frm);
	FrmSetFocus(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), SNField));
	if(globals->gDeviceFlags.bits.treo)
	{
		HsGrfSetStateExt(false, true, true, false, false, false);
	}		
	RestoreDialer();
	return true;
}

Boolean SNHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = SNOnCtlSelectEvent(event);
			break;			
		case keyDownEvent:
			handled = SNOnKeyDownEvent(event);
			break;
		case frmOpenEvent:
			handled = SNOnFrmOpenEvent();
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	return (handled);
}
