#include <PalmOS.h>
#include "AddrTools.h"
#include "AddrTools2.h"
#include "Address.h"

#include "globals.h"
#include "dia.h"

void EMInitForm();
void EMFree();

void EMSendMail();

void EMSendMail()
{
	globalVars* globals = getGlobalsPtr();
	if(EMailContact(globals->gEMail)==false)
	{
		FrmAlert(SendMailFailedAlert);
	}
}

void EMFree()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 cnt;
	for(cnt=0;cnt<globals->EMailCount;cnt++)
	{
		MemPtrFree(globals->EMailListPtr[cnt]);
	}	
	MemPtrFree(globals->EMailListPtr);
}

void EMInitForm()
{
	globalVars* globals = getGlobalsPtr();
	ListType *lListPtr=(ListType*)GetObjectPtrSmp(EMailList); 
	globals->EMailCount=LoadEMailAddress(globals->CurrentRecord, NULL, true);
	if(globals->EMailCount==0)
		FrmReturnToForm(0);
	globals->EMailListPtr=MemPtrNew(globals->EMailCount*sizeof(Char**));
	LoadEMailAddress(globals->CurrentRecord, globals->EMailListPtr, false);
	if(globals->EMailCount == 1)
	{
		StrCopy(globals->gEMail,globals->EMailListPtr[0]);
		EMFree();
		EMSendMail();
		return;
	}
	LstSetListChoices(lListPtr, globals->EMailListPtr, globals->EMailCount);	 
	LstDrawList(lListPtr);
}

static Boolean EMOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
	case EMailOkButton:
		StrCopy(globals->gEMail, LstGetSelectionText(GetObjectPtrSmp(EMailList), LstGetSelection(GetObjectPtrSmp(EMailList))));
		EMFree();
		EMSendMail();
		handled = true;
		break;
	case EMailCancelButton:
		EMFree();
		FrmReturnToForm(0);
		handled=true;
		break;
	default:
		break;
	}
	return handled;
}

static Boolean EMOnFrmOpenEvent()
{
	FormPtr frm = FrmGetFormPtr(EMailDialog);
	dia_save_state(); 
	dia_enable(frm, false);
	
	FrmDrawForm (frm);
	EMInitForm();
	return true;
}

static Boolean  EMOnLstSelectEvent()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetActiveForm();
	FrmSetFocus(frmP, FrmGetObjectIndex(frmP, EMailOkButton));
	if(globals->gNavigation)
	{
		FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, EMailOkButton);	
	}
	return true;
}

Boolean EMHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch(event->eType)
	{
		case lstSelectEvent:
			handled = EMOnLstSelectEvent();
			break;
		case ctlSelectEvent:
			handled = EMOnCtlSelectEvent(event);
			break;
		case frmOpenEvent:
			handled = EMOnFrmOpenEvent();
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	return (handled);
}
