/******************************************************************************
 *
 * Copyright (c) 1995-2002 PalmSource, Inc. All rights reserved.
 *
 * File: AddrNote.c
 *
 * Release: Palm OS 5 SDK (68K) R3.
 *
 * Description:
 *  This is the Address Book Note screen
 *
 *****************************************************************************/

#include "AddrNote.h"
#include "Address.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "AddressDB2.h"
#include "globals.h"
#include "dia.h"
#include "syslog.h"

#include <Category.h>
#include <UIResources.h>
#include <StringMgr.h>
#include <PhoneLookup.h>

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/

Boolean			NoteOnUpdate(EventType * event);
static void		PrvNoteViewInit ();
static void		PrvNoteViewDrawTitleAndForm (FormPtr frm);
static void		PrvNoteViewUpdateScrollBar (void);
static void		PrvNoteViewLoadRecord (void);
static void		PrvNoteViewSave (void);
static Boolean	PrvNoteViewDeleteNote (void);
static Boolean	PrvNoteViewDoCommand (UInt16 command);
static void		PrvNoteViewScroll (Int16 linesToScroll, Boolean updateScrollbar);
static void		PrvNoteViewPageScroll (WinDirectionType direction);
static Boolean 	AddrNoteMoveObjects(FormType* frmP, Coord dx, Coord dy);



static Boolean AddrNoteMoveObjects(FormType* frmP, Coord dx, Coord dy)
{
    RectangleType bounds;
    UInt16 scrollPos;
    Boolean resized = false;

#ifdef DEBUG
	LogWrite("xt_log", "note", "mover");
#endif
    
    if (dx != 0 || dy != 0)
    {
        
        // NOTE: be careful to avoid object migration!
		MoveFormObjectHide(frmP, NoteDoneButton, 0, dy);
        MoveFormObjectHide(frmP, NoteDeleteButton, 0, dy);
        
       	scrollPos=FldGetScrollPosition(GetObjectPtrSmp(NoteField));
       	FrmHideObjectSmp(NoteField);
        FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), NoteField), &bounds);
		bounds.extent.x+=dx;
		bounds.extent.y+=dy;
		FrmSetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), NoteField), &bounds);
		FldRecalculateField (GetObjectPtrSmp(NoteField), true);
		FldSetScrollPosition(GetObjectPtrSmp(NoteField), scrollPos);
        FrmShowObjectSmp(NoteField);
		
        FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), NoteScrollBar), &bounds);
		bounds.topLeft.x+=dx;
		bounds.extent.y+=dy;
		FrmSetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), NoteScrollBar), &bounds);
        MoveFormGSI(frmP, dx, dy);
        resized = true;
		
#ifdef DEBUG
		LogWrite("xt_log", "note", "mover: resized");
#endif

    }
   	return resized;
}

Boolean NoteOnUpdate(EventType * event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP;
	Boolean rv;
	FieldPtr fldP;
	
	frmP = FrmGetActiveForm();

	if (event->data.frmUpdate.updateCode & updateFontChanged)
	{
		fldP = CustomGetObjectPtrSmp(NoteField);
		FldSetFont(fldP, globals->NoteFont);
		PrvNoteViewUpdateScrollBar();
		PrvNoteViewDrawTitleAndForm (frmP);
		
	}
	else
	{
		// Handle the case that form is not active (frmRedrawUpdateCode)
		frmP = FrmGetFormPtr(NewNoteView);
		PrvNoteViewDrawTitleAndForm(frmP);
	}
	rv=true;
	return rv;
}

static Boolean NoteOnFrmOpenEvent()
{
	FormPtr frmP = FrmGetActiveForm();

#ifdef DEBUG
	LogWrite("xt_log", "note", "frmOpen");
#endif

	dia_save_state();
	dia_enable(frmP, true);
	// mover_func -- функция, перемещающая объекты на форме
	dia_resize(frmP, AddrNoteMoveObjects);
	
	PrvNoteViewInit ();
	PrvNoteViewDrawTitleAndForm (frmP);
	PrvNoteViewUpdateScrollBar ();
	FrmSetFocus (frmP, FrmGetObjectIndex (frmP, NoteField));
	return true;
}

static Boolean NoteOnFrmCloseEvent()
{
	globalVars* globals = getGlobalsPtr();
	AttnIndicatorEnable(true);		// Custom title doesn't support attention indicator.

	if ( globals->UnnamedRecordStringPtr != 0 )
	{
		MemPtrUnlock(globals->UnnamedRecordStringPtr);
		globals->UnnamedRecordStringPtr = NULL;
	}

	if ( globals->UnnamedRecordStringH != 0 )
	{
		DmReleaseResource(globals->UnnamedRecordStringH);
		globals->UnnamedRecordStringH = NULL;
	}
	
	if ( FldGetTextHandle (CustomGetObjectPtrSmp (NoteField)))
		PrvNoteViewSave ();
	return false;
}

static Boolean NoteOnKeyDownEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
	{
		PrvNoteViewSave ();
		FrmGotoForm (ListView);
		handled = true;
	}

	else if(IsFiveWayNavEvent(event))
	{
		if (NavKeyPressed(event, Select) && !globals->gNavigation)
		{
			PrvNoteViewSave ();
			// When we return to the ListView highlight this record.
			if (globals->PriorAddressFormID == ListView)
				globals->ListViewSelectThisRecord = globals->CurrentRecord;
			FrmGotoForm(globals->PriorAddressFormID);
			handled = true;
		}
	}  
	return handled;
}

static Boolean NoteOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
		case NoteDoneButton:
			SetDialer();
			PrvNoteViewSave ();

			// When we return to the ListView highlight this record.
			if (globals->PriorAddressFormID == ListView)
				globals->ListViewSelectThisRecord = globals->CurrentRecord;

			FrmGotoForm(globals->PriorAddressFormID);
			handled = true;
			break;

		case NoteDeleteButton:
			if (PrvNoteViewDeleteNote ())
			{
				SetDialer();
				FrmGotoForm (globals->PriorAddressFormID);
			}
			globals->ListViewSelectThisRecord = noRecord;
			handled = true;
			break;
		default:
			break;
	}
	return handled;
}

static Boolean NoteOnFrmGotoEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetActiveForm ();
	FieldPtr fldP;
	globals->CurrentRecord = event->data.frmGoto.recordNum;
	
	dia_save_state();
	dia_enable(frmP, true);
	dia_resize(frmP, AddrNoteMoveObjects);
	
	PrvNoteViewInit();
	fldP = CustomGetObjectPtrSmp(NoteField);
	FldSetScrollPosition(fldP, event->data.frmGoto.matchPos);
	FldSetSelection(fldP, event->data.frmGoto.matchPos,
					event->data.frmGoto.matchPos + event->data.frmGoto.matchLen);
	PrvNoteViewDrawTitleAndForm (frmP);
	PrvNoteViewUpdateScrollBar();
	FrmSetFocus(frmP, FrmGetObjectIndex(frmP, NoteField));
	return true;
}

Boolean NoteViewHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch (event->eType)
	{
		case frmOpenEvent:
			handled = NoteOnFrmOpenEvent();    		
			break;	
		case frmCloseEvent:
			handled = NoteOnFrmCloseEvent();    		
			break;	
		case keyDownEvent:
			handled = NoteOnKeyDownEvent(event);    		
			break;	
		case ctlSelectEvent:
			handled = NoteOnCtlSelectEvent(event);    		
			break;	
		case fldChangedEvent:
			PrvNoteViewUpdateScrollBar ();
			handled = true;
			break;	
		case menuEvent:
			return PrvNoteViewDoCommand (event->data.menu.itemID);	
		case frmGotoEvent:
			handled = NoteOnFrmGotoEvent(event);  			
			break;	
		case frmUpdateEvent:
			handled = NoteOnUpdate(event);
			break;	
		case sclRepeatEvent:
			PrvNoteViewScroll (event->data.sclRepeat.newValue - event->data.sclRepeat.value, false);
			break;	
		case winEnterEvent:
			if(event->data.winEnter.enterWindow == 0)
				break;
			dia_win_enter();
			break;
		default:
			handled = dia_handle_event(event, AddrNoteMoveObjects);
			break;
	}
	return (handled);
}


Boolean NoteViewCreate (void)
{
	globalVars* globals = getGlobalsPtr();
	univAddrDBRecordType record;
	univAddrDBRecordFlags bit;
	MemHandle recordH;
	Err err;

	univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);
	if (!record.fields[univNote])

#ifdef CONTACTS
	{
		record.fields[P1Contactsnote] = (char *)"";
		bit.bits2.note = 1;
		err = PrvP1ContactsDBChangeRecord(globals->adxtLibRef, globals->AddrDB, &globals->CurrentRecord, &record, bit, false);
#else
	{
		record.fields[note] = (char *)"";
		bit.allBits = (UInt32)1 << note;
		err = AddrDBChangeRecord(globals->AddrDB, &globals->CurrentRecord, &record, bit);
#endif
		if (err)
		{
			MemHandleUnlock(recordH);
			FrmAlert(DeviceFullAlert);
			return false;            // can't make an note field.
		}

	}
	else
	{
		MemHandleUnlock(recordH);
	}

	return true;                  // a note field exists.
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewDelete
 *
 * DESCRIPTION: Deletes the note field from the current record.
 ***********************************************************************/
void NoteViewDelete (void)
{
	globalVars* globals = getGlobalsPtr();
#ifndef CONTACTS
	AddrDBRecordType record;
	AddrDBRecordFlags changedField;
#else
	P1ContactsDBRecordType contact;
	P1ContactsDBRecordFlags contactbit;
#endif
	MemHandle recordH;

	Err err;


#ifdef CONTACTS
	PrvP1ContactsDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &contact, &recordH);
	contact.fields[P1Contactsnote] = NULL;
	contactbit.bits2.note = 1;
	err = PrvP1ContactsDBChangeRecord(globals->adxtLibRef, globals->AddrDB, &globals->CurrentRecord, &contact, contactbit, false);
	if (err)
	{
		MemHandleUnlock(recordH);
		FrmAlert(DeviceFullAlert);
		return;
	}
#else
	AddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);
	record.fields[note] = NULL;
	changedField.allBits = (UInt32)1 << note;
	err = AddrDBChangeRecord(globals->AddrDB, &globals->CurrentRecord, &record, changedField);
	if (err)
	{
		MemHandleUnlock(recordH);
		FrmAlert(DeviceFullAlert);
		return;
	}
	#endif
	// Mark the record dirty.
	ToolsDirtyRecord (globals->CurrentRecord);
}

#pragma mark -

void PrvNoteViewInit()
{
	globalVars* globals = getGlobalsPtr();
	FieldPtr       fld;
	FieldAttrType  attr;

	AttnIndicatorEnable(false);		// Custom title doesn't support attention indicator.
	PrvNoteViewLoadRecord ();

	// Have the field send events to maintain the scroll bar.
	fld = CustomGetObjectPtrSmp(NoteField);
	FldGetAttributes (fld, &attr);
	attr.hasScrollBar = true;
	FldSetAttributes (fld, &attr);
	
	RestoreDialer();
}

void PrvNoteViewDrawTitleAndForm (FormPtr frm)
{
	globalVars* globals = getGlobalsPtr();
	Coord x, y;
	Int16 fieldSeparatorWidth;
	Int16 shortenedFieldWidth;
	Char * name1;
	Char * name2;
	Int16 name1Length;
	Int16 name2Length;
	Int16 name1Width;
	Int16 name2Width;
	Int16 nameExtent;
	Coord formWidth;
	RectangleType r;
	FontID curFont;
	RectangleType eraseRect,drawRect;
	Boolean name1HasPriority;
	IndexedColorType curForeColor;
	IndexedColorType curBackColor;
	IndexedColorType curTextColor;
	univAddrDBRecordType record;
	MemHandle recordH;
	UInt8 * lockedWinP;
	Err error;

	// "Lock" the screen so that all drawing occurs offscreen to avoid
	// the anamolies associated with drawing the Form's title then drawing
	// the NoteView title.  We REALLY need to make a variant for doing
	// this in a more official way!
	//
	lockedWinP = WinScreenLock (winLockCopy);

	FrmDrawForm (frm);

	// Peform initial set up.
	//
	FrmGetFormBounds (frm, &r);
	formWidth = r.extent.x;
	x = 2;
	y = 1;
	nameExtent = formWidth - 4;

	// Save/Set window colors and font.  Do this after FrmDrawForm() is called
	// because FrmDrawForm() leaves the fore/back colors in a state that we
	// don't want here.
	//
	curForeColor = WinSetForeColor (UIColorGetTableEntryIndex(UIFormFrame));
	curBackColor = WinSetBackColor (UIColorGetTableEntryIndex(UIFormFill));
	curTextColor = WinSetTextColor (UIColorGetTableEntryIndex(UIFormFrame));
	curFont = FntSetFont (boldFont);

	RctSetRectangle (&eraseRect, 0, 0, formWidth, FntLineHeight()+4);
	RctSetRectangle (&drawRect, 0, 0, formWidth, FntLineHeight()+2);

	// Erase the Form's title area and draw the NoteView's.
	//
	WinEraseRectangle (&eraseRect, 0);
	WinDrawRectangle (&drawRect, 3);

	error = univAddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);
	name1HasPriority = ToolsDetermineRecordName(globals->adxtLibRef, &record, &shortenedFieldWidth, &fieldSeparatorWidth, globals->SortByCompany, &name1, &name1Length, &name1Width, &name2, &name2Length, &name2Width, &globals->UnnamedRecordStringPtr, &globals->UnnamedRecordStringH, nameExtent,
	true);

	ErrNonFatalDisplayIf(error, "Record not found");

	ToolsDrawRecordName(globals->adxtLibRef, name1, name1Length, name1Width, name2, name2Length, name2Width,
				   nameExtent, &x, y, shortenedFieldWidth, fieldSeparatorWidth, true,
				   name1HasPriority, true, true);

	// Now that we've drawn everything, blast it all back on the screen at once.
	//
	if (lockedWinP)
		WinScreenUnlock ();

	// Unlock the record that AddrGetRecord() implicitly locked.
	//
	MemHandleUnlock (recordH);

	// Restore window colors and font.
	//
	WinSetForeColor (curForeColor);
	WinSetBackColor (curBackColor);
	WinSetTextColor (curTextColor);
	FntSetFont (curFont);
}

void PrvNoteViewUpdateScrollBar (void)
{
	UInt16 scrollPos;
	UInt16 textHeight;
	UInt16 fieldHeight;
	Int16 maxValue;
	FieldPtr fld;
	ScrollBarPtr bar;

	fld = CustomGetObjectPtrSmp (NoteField);
	bar = CustomGetObjectPtrSmp (NoteScrollBar);

	FldGetScrollValues (fld, &scrollPos, &textHeight,  &fieldHeight);

	if (textHeight > fieldHeight)
	{
		// On occasion, such as after deleting a multi-line selection of text,
		// the display might be the last few lines of a field followed by some
		// blank lines.  To keep the current position in place and allow the user
		// to "gracefully" scroll out of the blank area, the number of blank lines
		// visible needs to be added to max value.  Otherwise the scroll position
		// may be greater than maxValue, get pinned to maxvalue in SclSetScrollBar
		// resulting in the scroll bar and the display being out of sync.
		maxValue = (textHeight - fieldHeight) + FldGetNumberOfBlankLines (fld);
	}
	else if (scrollPos)
		maxValue = scrollPos;
	else
		maxValue = 0;

	SclSetScrollBar (bar, scrollPos, 0, maxValue, fieldHeight-1);
}


/***********************************************************************
 *
 * FUNCTION:    PrvNoteViewLoadRecord
 *
 * DESCRIPTION: Load the record's note field into the field object
 * for editing in place.  The note field is too big (4K) to edit in
 * the heap.
 ***********************************************************************/
void PrvNoteViewLoadRecord (void)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 offset;
	FieldPtr fld;
	MemHandle recordH;
	Char * ptr;
	univAddrDBRecordType record;
	
	// Get a pointer to the memo field.
	fld = CustomGetObjectPtrSmp (NoteField);

	// Set the font used in the memo field.
	FldSetFont (fld, globals->NoteFont);

	univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);
	ptr = MemHandleLock (recordH);
	offset = record.fields[univNote] - ptr;
	FldSetText (fld, recordH, offset, StrLen(record.fields[univNote])+1);
	
	// CreateNote will have been called before the NoteView was switched
	// to.  It will have insured that a note field exists.

	// Find out where the note field is to edit it
	// Unlock recordH twice because AddrGetRecord() locks it, and we had to lock
	// it to deref it. 
	
	MemHandleUnlock(recordH);
	MemHandleUnlock(recordH);
}

void PrvNoteViewSave (void)
{
	globalVars* globals = getGlobalsPtr();
	FieldPtr fld;
	int textLength;


	fld = CustomGetObjectPtrSmp (NoteField);


	// If the field wasn't modified then don't do anything
	if (FldDirty (fld))
	{
		// Release any free space in the note field.
		FldCompactText (fld);

		ToolsDirtyRecord (globals->CurrentRecord);
	}


	textLength = FldGetTextLength(fld);

	// Clear the handle value in the field, otherwise the handle
	// will be freed when the form is disposed of,  this call also unlocks
	// the handle that contains the note string.
	FldSetTextHandle (fld, 0);


	// Empty fields are not allowed because they cause problems
	if (textLength == 0)
		NoteViewDelete();
}

Boolean PrvNoteViewDeleteNote (void)
{
	FieldPtr fld;

	// CodeWarrior in Debug creates a problem... FrmAlert() sets UnnamedRecordStringH to 0 -> memory leak
	if (FrmAlert(DeleteNoteAlert) != DeleteNoteYes)
	{
		return false;
	}

	// Unlock the handle that contains the text of the memo.
	fld = CustomGetObjectPtrSmp (NoteField);
	ErrFatalDisplayIf ((! fld), "Bad field");

	// Clear the handle value in the field, otherwise the handle
	// will be freed when the form is disposed of. this call also
	// unlocks the handle the contains the note string.
	FldCompactText (fld);
	FldSetTextHandle (fld, 0);


	NoteViewDelete();

	return (true);
}

Boolean PrvNoteViewDoCommand (UInt16 command)
{
	globalVars* globals = getGlobalsPtr();
	FieldPtr fld;
	Boolean handled = true;

	switch (command)
	{
	case newNoteFontCmd:
		globals->NoteFont = ToolsSelectFont(globals->NoteFont);
		dia_restore_state();
		break;

	case newNotePhoneLookupCmd:
		fld = CustomGetObjectPtrSmp(NoteField);
		dia_open();
		PhoneNumberLookup(fld);
		dia_restore_state();
		break;

	default:
		handled = false;
	}
	return (handled);
}

void PrvNoteViewScroll (Int16 linesToScroll, Boolean updateScrollbar)
{
	UInt16           blankLines;
	FieldPtr         fld;

	fld = CustomGetObjectPtrSmp (NoteField);
	blankLines = FldGetNumberOfBlankLines (fld);

	if (linesToScroll < 0)
		FldScrollField (fld, -linesToScroll, winUp);
	else if (linesToScroll > 0)
		FldScrollField (fld, linesToScroll, winDown);

	// If there were blank lines visible at the end of the field
	// then we need to update the scroll bar.
	if (blankLines && linesToScroll < 0 || updateScrollbar)
	{
		PrvNoteViewUpdateScrollBar();
	}
}

void PrvNoteViewPageScroll (WinDirectionType direction)
{
	Int16 linesToScroll;
	FieldPtr fld;

	fld = CustomGetObjectPtrSmp (NoteField);

	if (FldScrollable (fld, direction))
	{
		linesToScroll = FldGetVisibleLines (fld) - 1;

		if (direction == winUp)
			linesToScroll = -linesToScroll;

		PrvNoteViewScroll(linesToScroll, true);
	}
}
