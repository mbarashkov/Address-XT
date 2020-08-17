#include <PalmOS.h>
#include "AddrTools.h"
#include "AddrTools2.h"
#include "Address.h"
#include "NewLink.h"
#include "Plugins/DateBookRsc.h"

#include "globals.h"
#include "dia.h"

#define TODOPLUGIN_DBNAME "XTToDo Plugin"
#define MEMOPLUGIN_DBNAME "XTMemo Plugin"

#define TODOLINK_NAME "Task"
#define MEMOLINK_NAME "Memo"
#define CONTACTSLINK_NAME "Contact"
#define DATELINK_NAME "Appointment"


static void InitForm();

static void InitForm()
{
	globalVars* globals = getGlobalsPtr();
	ListType *lListPtr=GetObjectPtrSmp(NewLinkList);
	globals->gLinkTypeSel = globals->gLinkType;
	LstSetSelection(lListPtr, globals->gLinkTypeSel);
}

static void NewLinkOnOK()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 sel = CustomLstGetSelection(NewLinkList);
	if(sel == noListSelection)
		return;
	globals->gLinkType = sel;
	globals->gLinkID = 0;
	if(sel == LINKTYPE_MEMO)
	{
		FrmPopupForm(MemoListView);
	}
	else if(sel == LINKTYPE_TODO)
	{
		FrmPopupForm(ToDoListView);
	}
	else if(sel == LINKTYPE_CONTACT)
	{
		FrmPopupForm(ContactSelListView);
	}
	else if(sel == LINKTYPE_DATE)
	{
		FrmPopupForm(DayView);
	}
	if(globals->gLinkID!=0)
	{
		FrmUpdateForm (ListView, updatePrefs);
		FrmUpdateForm (RecordView, updatePrefs);
		FrmUpdateForm (EditView, updatePrefs);
			
		FrmReturnToForm(0);
	}
}

static Boolean NLOnCtlSelectEvent(EventType* event)
{
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
	case NewLinkOkButton:
		NewLinkOnOK();
		handled = true;
		break;
	case NewLinkCancelButton:
		FrmUpdateForm (ListView, updatePrefs);
		FrmUpdateForm (RecordView, updatePrefs);
		FrmUpdateForm (EditView, updatePrefs);
		FrmReturnToForm(0);
		handled=true;
		break;
	default:
		break;
	}
	return handled;
}

static Boolean NLOnKeyDownEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			NewLinkOnOK();
			return true;
		}		   	 	
	}  
	return handled;
}

static Boolean NLOnFrmOpenEvent()
{
	FormPtr frm = FrmGetFormPtr(NewLinkDialog);
	dia_save_state(); 
	dia_enable(frm, false);
	FrmDrawForm (frm);
	InitForm();
	return true;
}

static Boolean NLOnWinEnterEvent()
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gLinkID!=0)
	{
		FrmReturnToForm(0);
		return true;
	}	
	else
	{
		return false;
	}
}

static Boolean  NLOnLstSelectEvent()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetActiveForm();//was commented from here
	FrmSetFocus(frmP, FrmGetObjectIndex(frmP, NewLinkOkButton));
	if(globals->gNavigation)
	{
		FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, NewLinkOkButton);	
	}//to here
	return true;
}

Boolean NLHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch(event->eType)
	{
		case lstSelectEvent:
			handled = NLOnLstSelectEvent();
			break;
		case ctlSelectEvent:
			handled = NLOnCtlSelectEvent(event);
			break;
		case keyDownEvent:
			handled = NLOnKeyDownEvent(event);
			break;
		case frmOpenEvent:
			handled = NLOnFrmOpenEvent(event);
			break;
		case winEnterEvent:
			handled = NLOnWinEnterEvent(event);
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	return (handled);
}
