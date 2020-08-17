#include "AddrCustom.h"
#include "AddressDB.h"
#include "AddressDB2.h"
#include "AddressRsc.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "Address.h"
#include "globals.h"
#include "dia.h"
#include "../AddrTools.h"

#include <Form.h>
#include <StringMgr.h>

static void CustomEditSave (FormPtr frm);
static void CustomEditInit (FormPtr frm);

void CustomEditSave (FormPtr frm)
{
	globalVars* globals = getGlobalsPtr();
	UInt16      index;
	FieldPtr   fld;
	UInt16      objNumber;
	Char * textP;
#ifndef CONTACTS
	AddrAppInfoPtr appInfoPtr;
#else
	P1ContactsAppInfoPtr contactsInfoPtr;
#endif
	Boolean sendUpdate = false;


	// Get the object number of the first field.
	objNumber = FrmGetObjectIndex (frm, CustomEditFirstField);


	// For each dirty field update the corresponding label.
#ifdef CONTACTS
	for (index = P1ContactsfirstRenameableLabel; index <= P1ContactslastRenameableLabel; index++)
#else
	for (index = firstRenameableLabel; index <= lastRenameableLabel; index++)
#endif
	{
		fld = FrmGetObjectPtr (frm, objNumber++);
		if (FldDirty(fld))
		{
			sendUpdate = true;
			textP = FldGetTextPtr(fld);
			if (textP)
				AddrDBSetFieldLabel(globals->AddrDB, index, textP);
		}
	}
	if (sendUpdate)
	{
		// Update the column width since a label changed.
#ifdef CONTACTS
		contactsInfoPtr = (P1ContactsAppInfoPtr) P1ContactsDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
		globals->EditLabelColumnWidth = ToolsGetLabelColumnWidth (globals->adxtLibRef, contactsInfoPtr, stdFont);
		globals->RecordLabelColumnWidth = ToolsGetLabelColumnWidth (globals->adxtLibRef, contactsInfoPtr, globals->AddrRecordFont);
		MemPtrUnlock(contactsInfoPtr);
#else
		appInfoPtr = (AddrAppInfoPtr) AddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
		globals->EditLabelColumnWidth = ToolsGetLabelColumnWidth (globals->adxtLibRef, appInfoPtr, stdFont);
		globals->RecordLabelColumnWidth = ToolsGetLabelColumnWidth (globals->adxtLibRef, appInfoPtr, globals->AddrRecordFont);
		MemPtrUnlock(appInfoPtr);
#endif
		FrmUpdateForm (0, updateCustomFieldLabelChanged);
	}

}

void CustomEditInit (FormPtr frm)
{
	globalVars* globals = getGlobalsPtr();
	UInt16      index;
	UInt16      length;
	FieldPtr   fld;
	UInt16      objNumber;
	MemHandle textH;
	Char * textP;
	univAppInfoPtr appInfoPtr;
	addressLabel *fieldLabels;


	// Get the object number of the first field.
	objNumber = CustomEditFirstField;
	appInfoPtr = univAdrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	fieldLabels = appInfoPtr->fieldLabels;
	for (index = univFirstRenameableLabel; index <= univLastRenameableLabel; index++)
	{
		CustomShowObjectSmp(CustomEditFirstField + (index - univFirstRenameableLabel) );
		fld = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, objNumber++));
		length = StrLen(fieldLabels[index]);
		if (length > 0)
		{
			length += 1;         // include space for a null terminator
			textH = MemHandleNew(length);
			if (textH)
			{
				textP = MemHandleLock(textH);
				MemMove(textP, fieldLabels[index], length);
				FldSetTextHandle (fld, textH);
				MemHandleUnlock(textH);
			}
		}
	}

	MemPtrUnlock(appInfoPtr);
	
	// For each label, allocate some global heap space and copy the
	// the string to the global heap.  Then set a field to use the
	// copied string for editing.  If the field is unused no space is
	// allocated.  The field will allocate space if text is typed in.
	
	RestoreDialer();		
}


static Boolean CustomOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm;
	switch (event->data.ctlSelect.controlID)
	{
		case CustomEditOkButton:
		{
			EventType evt;			
			SetDialer();
			frm = FrmGetActiveForm();
			CustomEditSave(frm);
			ToolsLeaveForm();

			evt.eType = kFrmCustomUpdateEvent;
			EvtAddEventToQueue(&evt);	// We send this event because View screen needs to recalculate its display when Custom fields are renamed
			return true;
		}
		case CustomEditCancelButton:
			SetDialer();
			ToolsLeaveForm();
			return true;
		default:
			break;
	}
	return false;
}

static Boolean CustomOnKeyDownEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm;
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			EventType evt;
	
			frm = FrmGetActiveForm();
			CustomEditSave(frm);
			ToolsLeaveForm();

			evt.eType = kFrmCustomUpdateEvent;
			EvtAddEventToQueue(&evt);	// We send this event because View screen needs to recalculate its display when Custom fields are renamed
			return true;
		}		   	 	
	}  
	return false;
}

static Boolean CustomOnFrmOpenEvent()
{
	FormPtr frm = FrmGetActiveForm ();
	dia_save_state(); 
	dia_enable(frm, false);
	CustomEditInit (frm);
	FrmDrawForm (frm);
	return true;
}

Boolean CustomEditHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = CustomOnCtlSelectEvent(event);
			break;
		case keyDownEvent:
			handled = CustomOnKeyDownEvent(event);
			break;
		case frmOpenEvent:
			handled = CustomOnFrmOpenEvent();
			break;
		case winDisplayChangedEvent:
			dia_display_changed();
			FrmUpdateForm(CustomEditDialog, frmRedrawUpdateCode);
			handled = true;
			break;
	}
	return (handled);
}
