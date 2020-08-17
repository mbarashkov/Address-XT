#include "ContactSelList.h"
#include "../Address.h"
#include "../AddressDB.h"
#include "../PalmContactsDB/ContactsDB.h"
#include "../AddrTools.h"
#include "../AddrTools2.h"
#include "../AddrNote.h"
#include "../AddrPrefs.h"
#include "../AddrDefines.h"
#include "../AddressRsc.h"
#include "../AddrPrefs.h"
#include "ColorOptions.h"

#include "globals.h"
#include <TextMgr.h>
#include <ErrorMgr.h>
#include <SoundMgr.h>
#include <TimeMgr.h>
#include <AboutBox.h>
#include <Category.h>
#include <Menu.h>
#include <UIResources.h>
#include "../dia.h"

// Address list table columns
#define nameAndNumColumn					0
#define noteColumn							1

// Scroll rate values
#define scrollDelay							2
#define scrollAcceleration					2
#define scrollSpeedLimit					5


// These are used for accelerated scrolling

void			ContactsSetCategoryLabel(UInt16 category, Boolean changeCat);
Boolean			ContactSelListOnUpdate(EventType* event);
Boolean			ContactSelListOnOpen();

static void		PrvContactSelListInit( FormType* frmP );
static void		PrvContactSelListSelectRecord( FormType* frmP, UInt16 recordNum, Boolean forceSelection );
static void		PrvContactSelListScroll( WinDirectionType direction, UInt16 units, Boolean byLine );
static void		PrvContactSelListResetScrollRate(void);
static void		PrvContactSelListAdjustScrollRate(void);
static void		PrvContactSelListDrawRecord (void * table, Int16 row, Int16 column, RectanglePtr bounds);
static void		PrvContactSelListUpdateDisplay( UInt16 updateCode );
static UInt16	PrvContactSelListNumberOfRows (TablePtr table);
static void		PrvContactSelListUpdateScrollButtons( FormType* frmP );
static void		PrvContactSelListLoadTable( FormType* frmP );
static UInt16	PrvContactSelListSelectCategory (void);
static void		PrvContactSelListNextCategory (void);
static Boolean	PrvContactSelListHandleRecordSelection( EventType* event );
static void		PrvContactSelListReplaceTwoColors (const RectangleType *rP, UInt16 cornerDiam, IndexedColorType oldForeground, IndexedColorType oldBackground, IndexedColorType newForeground, IndexedColorType newBackground);
void ContactsCstSelUpDown(Int16 direction);
static Boolean ContactsSelToolsSeekRecord (UInt16 * indexP, Int16 offset, Int16 direction);
Boolean PrvContactsSelListLookupString (EventType * event);

void ContactsCstSelUpDown(Int16 direction)
{
	globalVars* globals = getGlobalsPtr();
	TableType* tblP;
	UInt16 rowsInPage;
	UInt16 newContactsSelTopVisibleRecord;
	UInt16 prevContactsSelTopVisibleRecord = globals->ContactsSelTopVisibleRecord;
	UInt16 indexP=0;
	UInt16 offset=0;
	if(PrvAddrDBSeekVisibleRecordInCategory(globals->adxtLibRef, globals->AddrDB, &indexP, offset, dmSeekForward, globals->ContactsSelCurrentCategory, (globals->PrivateRecordVisualStatus != showPrivateRecords))!=errNone)
		return;
	tblP = CustomGetObjectPtrSmp(ContactSelListTable);
	// Safe. There must be at least one row in the table.
	rowsInPage = PrvContactSelListNumberOfRows(tblP) - 1;
	if(globals->ContactSelCurrentRecord==noRecord)
	{
		globals->ContactSelCurrentRecord=indexP;
		newContactsSelTopVisibleRecord = globals->ContactSelCurrentRecord;
			
		PrvContactSelListSelectRecord(FrmGetActiveForm(), newContactsSelTopVisibleRecord, true);
		return;
	}
	newContactsSelTopVisibleRecord = globals->ContactSelCurrentRecord;
	ContactsSelToolsSeekRecord (&newContactsSelTopVisibleRecord, 1, direction);
	PrvContactSelListSelectRecord(FrmGetActiveForm(), newContactsSelTopVisibleRecord, true);		
}


Boolean ContactSelListOnUpdate(EventType* event)
{
#pragma unused(event)
	FormPtr frmP = FrmGetFormPtr(ContactSelListView);
	PrvContactSelListUpdateScrollButtons(frmP);
	return true;
}

Boolean ContactSelListOnOpen()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetActiveForm ();
	WinHandle winH;
	RectangleType winBounds;
	globals->ContactsSelCurrentCategory = globals->CurrentCategory;	
	globals->gAddressView=false;
	
	dia_save_state(); 
	dia_enable(frmP, false);
	
	CustomHideObjectSmp(ContactSelListTable);
	//if(globals->gScreen == CLIE_320x320)
	//	WinEraseWindow();
	FrmDrawForm (frmP);//disable table drawing here!
	winH=FrmGetWindowHandle(frmP);
	WinGetBounds(winH, &winBounds);
	PrvContactSelListInit (frmP);
	
	// Make sure the record to be selected is one of the table's rows or
	// else it reloads the table with the record at the top.  Nothing is
	// drawn by this because the table isn't visible.
	
	
	CustomShowObjectSmp(ContactSelListTable);
	
	

	// Set the focus in the lookup field so that the user can easily
	// bring up the keyboard.
	
	globals->PriorAddressFormID = FrmGetFormId (frmP);

	// Check the dialing abilities
	// Only the first call is long and further called are fast
	// So it's better to do it the first time the form is drawn
	
	globals->ContactSelCurrentRecord = noRecord;
	//Check registration
	
	return true;
}

static void ContactSelListOnOK()
{
	globalVars* globals = getGlobalsPtr();
	if(globals->ContactSelCurrentRecord==noRecord)
		globals->gLinkID = 0;
	else
	{
		if(DmRecordInfo (globals->AddrDB, globals->ContactSelCurrentRecord, NULL, &globals->gLinkID, NULL)!=errNone)
			globals->gLinkID = 0;
	}
	FrmUpdateForm(NewLinkDialog, 0);
	FrmUpdateForm (EditView, updatePrefs);
	FrmReturnToForm(0);
}

static void ContactSelListOnCancel()
{
	globalVars* globals = getGlobalsPtr();
	globals->gLinkID = 0;
	FrmUpdateForm(NewLinkDialog, 0);
	FrmUpdateForm (EditView, updatePrefs);
	FrmReturnToForm(0);
}

Boolean PrvContactsSelListLookupString (EventType * event)
{
	globalVars* globals = getGlobalsPtr();
	FormType* frmP;
	UInt16 fldIndex;
	FieldPtr fldP;
	Char * fldTextP;
	TablePtr tableP;
	UInt16 foundRecord;
	Boolean completeMatch;
	Boolean advancedFind = false;
	
	if(globals->gAdvancedFind == ADVANCEDFIND_ON)
		advancedFind = true;
	
	frmP = FrmGetActiveForm();
	fldIndex = FrmGetObjectIndex(frmP, ContactSelListLookupField);
	
	FrmSetFocus(frmP, fldIndex);
	fldP = FrmGetObjectPtr (frmP, fldIndex);

	if (FldHandleEvent (fldP, event) || event->eType == fldChangedEvent)
	{
		fldTextP = FldGetTextPtr(fldP);
		tableP = CustomGetObjectPtrSmp(ContactSelListTable);

		if(advancedFind)
		{
			if (!AddrDBLookupString(globals->adxtLibRef, globals->AddrDB, fldTextP, globals->SortByCompany,
								  globals->ContactsSelCurrentCategory, &foundRecord, &completeMatch,
								  (globals->PrivateRecordVisualStatus == maskPrivateRecords)))
			{
				if (!AddrDBLookupStringEx(globals->AddrDB, fldTextP, globals->SortByCompany,
								  globals->ContactsSelCurrentCategory, &foundRecord, &completeMatch,
								  (globals->PrivateRecordVisualStatus == maskPrivateRecords), 10, noRecord, 0))
			
				{
					// If the user deleted the lookup text remove the
					// highlight.
					globals->ContactSelCurrentRecord = noRecord;
					globals->ContactSelSelectedRecord=noRecord;
					TblDrawTable(tableP);
				}
				else
				{
					PrvContactSelListSelectRecord(frmP, foundRecord, true);
				}
			}
			else
			{
				UInt16 rectemp = foundRecord;
				if(!completeMatch)
				{
					if (AddrDBLookupStringEx(globals->AddrDB, fldTextP, globals->SortByCompany,
								  globals->ContactsSelCurrentCategory, &foundRecord, &completeMatch,
								  (globals->PrivateRecordVisualStatus == maskPrivateRecords), 10, noRecord, 0))
			
					{
						PrvContactSelListSelectRecord(frmP, foundRecord, true);
					}
					else
					{
						foundRecord = rectemp;
						PrvContactSelListSelectRecord(frmP, foundRecord, true);
					}
				}
				else
				{
					PrvContactSelListSelectRecord(frmP, foundRecord, true);
				}			
			}
		}
		else
		{
			if (!AddrDBLookupString(globals->adxtLibRef, globals->AddrDB, fldTextP, globals->SortByCompany,
								  globals->ContactsSelCurrentCategory, &foundRecord, &completeMatch,
								  (globals->PrivateRecordVisualStatus == maskPrivateRecords)))
				{
					// If the user deleted the lookup text remove the
					// highlight.
					globals->ContactSelCurrentRecord = noRecord;
					globals->ContactSelSelectedRecord=noRecord;
					TblDrawTable(tableP);
				}
				else
				{
					PrvContactSelListSelectRecord(frmP, foundRecord, true);
				}		
		}
		return true;
	}
	return false;
}




Boolean ContactSelListHandleEvent (EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	
	switch (event->eType)
	{
	case frmOpenEvent:
		handled =ContactSelListOnOpen();
		break;

	case frmCloseEvent:
		if (globals->UnnamedRecordStringPtr)
		{
			MemPtrUnlock(globals->UnnamedRecordStringPtr);
			globals->UnnamedRecordStringPtr = NULL;
		}

		if (globals->UnnamedRecordStringH)
		{
			DmReleaseResource(globals->UnnamedRecordStringH);
			globals->UnnamedRecordStringH = NULL;
		}
		break;

	case tblEnterEvent:
		PrvContactSelListHandleRecordSelection (event);
		handled=true;
		break;
	
	case ctlSelectEvent:
		switch (event->data.ctlSelect.controlID)
		{
			case ContactSelListCategoryTrigger:
				PrvContactSelListSelectCategory ();
				handled = true;
				break;
	
			case ContactSelListOKButton:
				ContactSelListOnOK();
				handled = true;
				break;
			case ContactSelListCancelButton:
				ContactSelListOnCancel();
				handled = true;
				break;
		}
		break;

	case ctlEnterEvent:
		switch (event->data.ctlEnter.controlID)
		{
			case ContactSelListUpButton:
			case ContactSelListDownButton:
				// Reset scroll rate
				PrvContactSelListResetScrollRate();
				// Clear lookup string
				break;
		}
		break;
	
	case ctlRepeatEvent:
		// Adjust the scroll rate if necessary
		PrvContactSelListAdjustScrollRate();

		switch (event->data.ctlRepeat.controlID)
		{
			case ContactSelListUpButton:
				PrvContactSelListScroll (winUp, globals->ContactsSelScrollUnits, false);
				// leave unhandled so the buttons can repeat
				break;
	
			case ContactSelListDownButton:
				PrvContactSelListScroll (winDown, globals->ContactsSelScrollUnits, false);
				// leave unhandled so the buttons can repeat
				break;
			default:
				break;
		}
		break;

	case fldChangedEvent:

		handled = PrvContactsSelListLookupString(event);
		return true;
		break;
	case keyDownEvent:
		// Address Book key pressed for the first time?
		if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
		{
			if (! (event->data.keyDown.modifiers & poweredOnKeyMask))
			{
				PrvContactSelListNextCategory ();
				handled = true;
				break;
			}
		}
		else if (event->data.keyDown.chr==vchrRockerUp)
		{
				ContactsCstSelUpDown(dmSeekBackward);
				handled=true;
				break;
		}
		else if (event->data.keyDown.chr==vchrRockerDown) 
		{
			ContactsCstSelUpDown(dmSeekForward);
			handled=true;
			break;
		}		
		
 		else if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event))
		{
			if (ToolsNavKeyPressed(globals->adxtLibRef, event, Up) || (event->data.keyDown.chr==vchrPageUp))
			{
				ContactsCstSelUpDown(dmSeekBackward);
				handled=true;
				break;
			}
			if (ToolsNavKeyPressed(globals->adxtLibRef, event, Down) || (event->data.keyDown.chr==vchrPageDown)) 
			{
				ContactsCstSelUpDown(dmSeekForward);
				handled=true;
				break;
			}		
		}  
		
		else if (EvtKeydownIsVirtual(event))
		{
			switch (event->data.keyDown.chr)
			{
				case vchrPageUp:
					ContactsCstSelUpDown(dmSeekBackward);
					handled = true;
					break;
				case vchrPageDown:
					ContactsCstSelUpDown(dmSeekForward);
					handled = true;
					break;
			}
		}
		else
		{
			Boolean handled = PrvContactsSelListLookupString(event);
			return true;
		}
		break;
	case frmUpdateEvent:
		handled = ContactSelListOnUpdate(event);
		break;	
		
	case winDisplayChangedEvent:
		dia_display_changed();
		FrmUpdateForm(FrmGetActiveFormID(), frmRedrawUpdateCode);
		handled = true;
		break;
		
	default:
		break;
	}

	return (handled);
}

void PrvContactSelListInit( FormType* frmP )
{
	globalVars* globals = getGlobalsPtr();
	UInt16 row;
	UInt16 lineHeight;
	UInt16 width, height;
	UInt16 rowsInTable;
	TableType* tblP;
	RectangleType r1;
	if (globals->ShowAllCategories)
		globals->ContactsSelCurrentCategory = dmAllCategories;
	
	FrmDrawForm(frmP);
	width=GetWindowWidth();
	height=GetWindowHeight();
	tblP = CustomGetObjectPtrSmp(ContactSelListTable);
	TblEraseTable(tblP);
	if(globals->AddrListHighRes && (/*globals->gScreen==CLIE_320x320 ||*/ globals->gScreen==PALM_320x320))
	{
		if(height!=160)
		{
			RctSetRectangle(&r1, 0, 18, 160, 189);
		}
		else
		{
			RctSetRectangle(&r1, 0, 18, width, 126);
		}	
	}
	else
	{
		if(height>220)
		{
			RctSetRectangle(&r1, 0, 18, 160, 187);
		}
		else
		{
			RctSetRectangle(&r1, 0, 18, width, 121);
		}	
	}
	TblSetBounds(tblP, &r1);
		
	TblSetColumnWidth(tblP, nameAndNumColumn, width-8);
	TblSetColumnWidth(tblP, noteColumn, 7);
	
	rowsInTable = TblGetNumberOfRows(tblP);
	for (row = PrvContactSelListNumberOfRows(tblP); row < rowsInTable; row++)
	{
		TblSetRowSelectable(tblP, row, false);
	}
	
	
	FntSetFont (globals->AddrListFont);
	lineHeight=FntLineHeight()>>1;
			
	for (row = 0; row < PrvContactSelListNumberOfRows(tblP); row++)
	{
		TblSetItemStyle(tblP, row, nameAndNumColumn, customTableItem);
		TblSetItemStyle(tblP, row, noteColumn, customTableItem);
		TblSetRowUsable(tblP, row, false);
		TblSetRowSelectable(tblP, row, true);
		if(globals->AddrListHighRes && (/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320))
		{
			TblSetRowHeight(tblP, row, (FntLineHeight()+1)>>1);
		}		
		else
		{
			TblSetRowHeight(tblP, row, FntLineHeight());
		}
	}
	     
    TblSetColumnUsable(tblP, nameAndNumColumn, true);
	TblSetColumnUsable(tblP, noteColumn, true);

	TblSetColumnMasked(tblP, nameAndNumColumn, true);
	TblSetColumnMasked(tblP, noteColumn, true);

// Set the callback routine that will draw the records.
	TblSetCustomDrawProcedure (tblP, nameAndNumColumn, PrvContactSelListDrawRecord);
	TblSetCustomDrawProcedure (tblP, noteColumn, PrvContactSelListDrawRecord);


	// Load records into the address list.
	PrvContactSelListLoadTable(frmP);

	ContactsSetCategoryLabel(globals->ContactsSelCurrentCategory, false);
}


/***********************************************************************
 *
 * FUNCTION:    PrvListResetScrollRate
 *
 * DESCRIPTION: This routine resets the scroll rate
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			frigino	8/14/97	Initial Revision
 *
 ***********************************************************************/
void PrvContactSelListResetScrollRate(void)
{
	globalVars* globals = getGlobalsPtr();
	// Reset last seconds
	globals->LastSeconds = TimGetSeconds();
	// Reset scroll units
	globals->ContactsSelScrollUnits = 1;
}


/***********************************************************************
 *
 * FUNCTION:    PrvListAdjustScrollRate
 *
 * DESCRIPTION: This routine adjusts the scroll rate based on the current
 *              scroll rate, given a certain delay, and plays a sound
 *              to notify the user of the change
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			frigino		8/14/97		Initial Revision
 *			vmk			12/2/97		Fix crash from uninitialized sndCmd and
 *									derive sound amplitude from system amplitude
 *
 ***********************************************************************/
void PrvContactSelListAdjustScrollRate(void)
{
	globalVars* globals = getGlobalsPtr();
	// Accelerate the scroll rate every 3 seconds if not already at max scroll speed
	UInt16 newSeconds = TimGetSeconds();
	if ((globals->ContactsSelScrollUnits < scrollSpeedLimit) && ((newSeconds - globals->LastSeconds) > scrollDelay))
	{
		// Save new seconds
		globals->LastSeconds = newSeconds;

		// increase scroll units
		globals->ContactsSelScrollUnits += scrollAcceleration;
	}

}

void PrvContactSelListDrawRecord (void * table, Int16 row, Int16 column, RectanglePtr bounds)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 recordNum;
	Err error;
	univAddrDBRecordType record;
	MemHandle recordH;
	char noteChar;
	FontID currFont;
	RectangleType rect2, rect3, rect4;
	Int16 phoneX;
	IndexedColorType oldColor;
	UInt32 addrId;
	Boolean hasNote;
	
	// Get the record number that corresponds to the table item to draw.
	// The record number is stored in the "intValue" field of the item.
	//
	recordNum = TblGetRowID (table, row);
	RctCopyRectangle(bounds, &rect2);
	TblGetBounds(table, &rect3);
	rect2.extent.y=rect3.extent.y;
	RctCopyRectangle(bounds, &rect4);
	rect4.extent.x+=1;
	
	oldColor=UIColorGetTableEntryIndex(UIFormFill);
	if(globals->gEachOtherSelected && column==0)
	{
		if(row%2)
		{
			WinSetBackColor(globals->gColorBack);
		}
		else
		{
			WinSetBackColor(globals->gEachOther);
		}
	}
	WinEraseRectangle(&rect4, 0);
	
	DmRecordInfo(globals->AddrDB, recordNum, 0, &addrId, 0);
	
	error = univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, recordNum, &record, &recordH);
	if (error)
	{
		ErrNonFatalDisplay ("Record not found");
		return;
	}
	
	switch (column)
	{
		case nameAndNumColumn:
			if(!globals->AddrListHighRes)
			{
				currFont = FntSetFont (globals->AddrListFont);
			}
			//else  if (globals->gScreen==CLIE_320x320)
			//{
			//	currFont=HRFntSetFont(globals->refNum, globals->AddrListFont);
			//}
			else if(globals->gScreen==PALM_320x320)
			{
				currFont=FntSetFont(globals->AddrListFont);
			}
			phoneX = ToolsDrawRecordNameAndPhoneNumber (globals->adxtLibRef, &record, bounds, globals->PhoneLabelLetters, globals->SortByCompany, &globals->UnnamedRecordStringPtr, &globals->UnnamedRecordStringH, !globals->AddrListHighRes);					
			if(!globals->AddrListHighRes)
			{
				FntSetFont (currFont);
			}
			/*else if (globals->gScreen==CLIE_320x320)
			{
				HRFntSetFont (globals->refNum, currFont);
			}*/
			else if (globals->gScreen==PALM_320x320)
			{
				FntSetFont (currFont);
			}	
			TblSetRowData(table, row, phoneX);			// Store in table for later tap testing
			break;
		case noteColumn:
			// Draw a note symbol if the field has a note.
			hasNote = (record.fields[univNote] != NULL);
			
			if (hasNote)
			{
				MemHandle resH;
				BitmapType *bitmap;
				
				if(!globals->AddrListHighRes)
				{
					if(globals->gDepth<8)
					{
						currFont = FntSetFont (symbolFont);
					}
					else
					{
					if(globals->gScreen==PALM_320x320)
						resH = DmGetResource (bitmapRsc, ListNoteBmpPalmHRStd);
					//else if (globals->gScreen==CLIE_320x320)
					//	resH = DmGetResource (bitmapRsc, ListNoteBmpStd);
					else if(globals->gScreen==PALM_160x160)
						resH = DmGetResource (bitmapRsc, ListNoteBmpSmall);
					bitmap = MemHandleLock (resH);
					}	
				}
				/*else if (globals->gScreen==CLIE_320x320)
				{
					if(globals->gDepth<8)
					{
						currFont=HRFntSetFont(globals->refNum, symbolFont);
					}
					else
					{
						resH = DmGetResource (bitmapRsc, ListNoteBmpSmall);
						bitmap = MemHandleLock (resH);
					}
				}*/
				else if(globals->gScreen==PALM_320x320)
				{
					if(globals->gDepth<8)
					{
						currFont=FntSetFont(symbolFont);
					}
					else
					{
						resH = DmGetResource (bitmapRsc, ListNoteBmpPalmHRSmall);
						bitmap = MemHandleLock (resH);
					}
				}
				noteChar = symbolNote;
				if(!globals->AddrListHighRes)
				{
					if(globals->gDepth<8)
					{
						WinDrawChars (&noteChar, 1, bounds->topLeft.x, bounds->topLeft.y);
						FntSetFont (currFont);
					}
					else
					{	
					//if(globals->gScreen==CLIE_320x320)
					//	HRWinDrawBitmap(globals->refNum, bitmap,  bounds->topLeft.x<<1, (bounds->topLeft.y+1)<<1);
					//else
						WinDrawBitmap(bitmap,  bounds->topLeft.x, bounds->topLeft.y+2);
					}
				}
				/*else if (globals->gScreen==CLIE_320x320)
				{
					if(globals->gDepth<8)
					{
						HRWinDrawChars (globals->refNum, &noteChar, 1, bounds->topLeft.x<<1, bounds->topLeft.y<<1);
						HRFntSetFont (globals->refNum, currFont);
					}
					else
					{	
						HRWinDrawBitmap(globals->refNum, bitmap,  bounds->topLeft.x<<1, (bounds->topLeft.y+1)<<1);
					}
				}*/
				else if (globals->gScreen==PALM_320x320)
				{
					if(globals->gDepth<8)
					{
						ToolsWinDrawCharsHD (globals->adxtLibRef, &noteChar, 1, bounds->topLeft.x, bounds->topLeft.y);
						FntSetFont (currFont);
					}
					else
					{
						WinDrawBitmap(bitmap,  bounds->topLeft.x, bounds->topLeft.y+1);
					}
				}	
				if(globals->gDepth>=8)
				{
					MemHandleUnlock(resH);
					DmReleaseResource(resH);
				}
			}
		break;
	}
	
	if(globals->gEachOtherSelected)
	{
		WinSetBackColor(oldColor);
	}	
	MemHandleUnlock(recordH);
}

UInt16 PrvContactSelListNumberOfRows (TablePtr table)
{
	UInt16				rows;
	UInt16				rowsInTable;
	UInt16				tableHeight;
	FontID			currFont;
	RectangleType	r;
	globalVars* globals = getGlobalsPtr();
	

	rowsInTable = TblGetNumberOfRows (table);

	//PRINTDEBUG(rowsInTable);
	TblGetBounds (table, &r);
	tableHeight = r.extent.y;

	currFont = FntSetFont (globals->AddrListFont);
	if(!globals->AddrListHighRes)
	{
		rows = tableHeight / FntLineHeight ();
	}
	else if (/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320)
	{
		rows = tableHeight / ((FntLineHeight ()+1)>>1);
	}
	FntSetFont (currFont);
	if (rows <= rowsInTable)
		return (rows);
	else
		return (rowsInTable);
}

void PrvContactSelListUpdateScrollButtons( FormType* frmP )
{
	globalVars* globals = getGlobalsPtr();
	UInt16 row;
	UInt16 upIndex;
	UInt16 downIndex;
	UInt16 recordNum;
	Boolean scrollableUp;
	Boolean scrollableDown;
	TableType* tblP;

	// Update the button that scroll the list.
	//
	// If the first record displayed is not the fist record in the category,
	// enable the up scroller.
	recordNum = globals->ContactsSelTopVisibleRecord;
	scrollableUp = ContactsSelToolsSeekRecord (&recordNum, 1, dmSeekBackward);


	// Find the record in the last row of the table
	tblP = CustomGetObjectPtrSmp(ContactSelListTable);
	row = TblGetLastUsableRow(tblP);
	if (row != tblUnusableRow)
		recordNum = TblGetRowID(tblP, row);


	// If the last record displayed is not the last record in the category,
	// enable the down scroller.
	scrollableDown = ContactsSelToolsSeekRecord (&recordNum, 1, dmSeekForward);

	// Update the scroll button.
	upIndex = FrmGetObjectIndex(frmP, ContactSelListUpButton);
	downIndex = FrmGetObjectIndex(frmP, ContactSelListDownButton);
	FrmUpdateScrollers(frmP, upIndex, downIndex, scrollableUp, scrollableDown);
}

static Boolean ContactsSelToolsSeekRecord (UInt16 * indexP, Int16 offset, Int16 direction)
{
	globalVars* globals = getGlobalsPtr();
	DmSeekRecordInCategory (globals->AddrDB, indexP, offset, direction, globals->ContactsSelCurrentCategory);
	if (DmGetLastErr()) return (false);

	return (true);
}

/***********************************************************************
 *
 * FUNCTION:    PrvListLoadTable
 *
 * DESCRIPTION: This routine loads address book database records into
 *              the list view form.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *      Name   	Date      	Description
 *      ----   	----      	-----------
 *      art  	6/5/95      Initial Revision
 *		aro		9/25/00		Adding frmP as a parameter for frmUpdateEvent
 *
 ***********************************************************************/
void PrvContactSelListLoadTable( FormType* frmP )
{
	globalVars* globals = getGlobalsPtr();
	UInt16      row;
	UInt16      numRows;
	UInt16		lineHeight;
	UInt16		recordNum;
	UInt16		visibleRows;
	FontID		currFont;
	TableType* 	tblP;
	UInt16 		attr;
	Boolean		masked;


	// For each row in the table, store the record number as the row id.
	tblP = CustomGetObjectPtrSmp(ContactSelListTable);
	// Make sure we haven't scrolled too far down the list of records
	// leaving blank lines in the table.

	// Try going forward to the last record that should be visible
	visibleRows = PrvContactSelListNumberOfRows(tblP);
	TblUnhighlightSelection(tblP);
	recordNum = globals->ContactsSelTopVisibleRecord;
	//PRINTDEBUG(visibleRows);
	if (!ContactsSelToolsSeekRecord (&recordNum, visibleRows - 1, dmSeekForward))
	{
		// We have at least one line without a record.  Fix it.
		// Try going backwards one page from the last record
		globals->ContactsSelTopVisibleRecord = dmMaxRecordIndex;
		if (!ContactsSelToolsSeekRecord (&globals->ContactsSelTopVisibleRecord, visibleRows - 1, dmSeekBackward))
		{
			// Not enough records to fill one page.  Start with the first record
			globals->ContactsSelTopVisibleRecord = 0;
			ContactsSelToolsSeekRecord (&globals->ContactsSelTopVisibleRecord, 0, dmSeekForward);
		}
	}
	
	currFont = FntSetFont (globals->AddrListFont);
	lineHeight = FntLineHeight ();
	FntSetFont (currFont);

	numRows = TblGetNumberOfRows(tblP);
	recordNum = globals->ContactsSelTopVisibleRecord;

	for (row = 0; row < visibleRows; row++)
	{
		//PRINTDEBUG(row);
		if ( ! ContactsSelToolsSeekRecord (&recordNum, 0, dmSeekForward))
			break;
		
		// Make the row usable.
		TblSetRowUsable (tblP, row, true);

		DmRecordInfo (globals->AddrDB, recordNum, &attr, NULL, NULL);
		masked = (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == maskPrivateRecords));
		TblSetRowMasked(tblP,row,masked);

		// Mark the row invalid so that it will draw when we call the
		// draw routine.
		TblMarkRowInvalid (tblP, row);

		// Store the record number as the row id.
		TblSetRowID (tblP, row, recordNum);

		TblSetItemFont (tblP, row, nameAndNumColumn, globals->AddrListFont);
		recordNum++;
	}

	// Hide the item that don't have any data.
	while (row < numRows)
	{
		TblSetRowUsable (tblP, row, false);
		row++;
	}

	PrvContactSelListUpdateScrollButtons(frmP);
}


void ContactsSetCategoryLabel(UInt16 category, Boolean changeCat)
{
	globalVars* globals = getGlobalsPtr();
	FormType* frmP;
	TableType* tblP;
	
	UInt16 numRec;
	Char str[15];
	Char str2[15];
	Char name[dmCategoryLength];
	frmP = FrmGetActiveForm();
			
	tblP = CustomGetObjectPtrSmp(ContactSelListTable);
	
	if(changeCat)
	{
		globals->ContactsSelCurrentCategory = category;
		globals->ContactsSelTopVisibleRecord = 0;
		globals->ContactSelCurrentRecord = noRecord;
	
		
		globals->ContactsSelTopVisibleRecord=0;
		DmSeekRecordInCategory(globals->AddrDB, &globals->ContactsSelTopVisibleRecord, 0, dmSeekForward, category);
	}
		
	// Display the new category.
	numRec=DmNumRecordsInCategory(globals->AddrDB, category);
	CategoryGetName (globals->AddrDB, category, name);
		
	StrCopy(str2, "(");
	StrIToA(str, numRec);
	StrCat(str2, str);
	StrCopy(str, ")");
	StrCat(str2, str);
		
	{
		CategoryTruncateName (name, ResLoadConstant(maxCategoryWidthID)-10-FntLineWidth(str2, StrLen(str2)));
	}
	
	StrCopy(globals->gName2, name);
	StrCat(globals->gName2, str2); 
	
	CustomSetCtlLabelPtrSmp(ContactSelListCategoryTrigger, (Char*)&globals->gName2);

}

/***********************************************************************
 *
 * FUNCTION:    PrvListSelectCategory
 *
 * DESCRIPTION: This routine handles selection, creation and deletion of
 *              categories form the Details Dialog.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    The index of the new category.
 *
 *              The following global variables are modified:
 *                     ContactsSelCurrentCategory
 *                     ShowAllCategories
 *                     CategoryName
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   06/05/95   Initial Revision
 *			  gap	  08/13/99   Update to use new constant categoryDefaultEditCategoryString.
 *
 ***********************************************************************/
UInt16 PrvContactSelListSelectCategory (void)
{
	globalVars* globals = getGlobalsPtr();
	FormType* frmP;
	TableType* tblP;
	UInt16 category;
	Boolean categoryEdited;

	// Process the category popup list.
	category = globals->ContactsSelCurrentCategory;

	frmP = FrmGetActiveForm();
	categoryEdited = CategorySelect (globals->AddrDB, frmP, NULL,
									 ListCategoryContactSelList, true, &category, globals->CategoryName, 1, categoryHideEditCategory );
	
	
	if (category == dmAllCategories)
		globals->ShowAllCategories = true;
	else
		globals->ShowAllCategories = false;

	if ( categoryEdited || (category != globals->ContactsSelCurrentCategory))
	{
		globals->ScrollPosition=0;
		ContactsSetCategoryLabel(category, true);
		PrvContactSelListLoadTable(frmP);
				
		tblP = CustomGetObjectPtrSmp(ContactSelListTable);
		globals->ContactsSelTopVisibleRecord=0;
		DmSeekRecordInCategory(globals->AddrDB, &globals->ContactsSelTopVisibleRecord, 0, dmSeekForward, category);
		TblEraseTable(tblP);
		TblDrawTable(tblP);
		
		// By changing the category the current record is lost.
		globals->ContactSelCurrentRecord = noRecord;
		
	}

	return (category);
}


/***********************************************************************
 *
 * FUNCTION:    PrvListNextCategory
 *
 * DESCRIPTION: This routine display the next category,  if the last
 *              catagory is being displayed
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 *              The following global variables are modified:
 *                     ContactsSelCurrentCategory
 *                     ShowAllCategories
 *                     CategoryName
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   9/15/95   Initial Revision
 *         rsf   9/20/95   Copied from To Do
 *
 ***********************************************************************/
void PrvContactSelListNextCategory (void)
{
	UInt16 category;
	TableType* tblP;
	FormType* frmP;
	globalVars* globals = getGlobalsPtr();
	
	category = CategoryGetNext (globals->AddrDB, globals->ContactsSelCurrentCategory);

	if (category != globals->ContactsSelCurrentCategory)
	{
		if (category == dmAllCategories)
			globals->ShowAllCategories = true;
		else
			globals->ShowAllCategories = false;

		ContactsSetCategoryLabel(category, true);
		
		frmP = FrmGetActiveForm();
		globals->ScrollPosition=0;
		PrvContactSelListLoadTable(frmP);
		tblP = CustomGetObjectPtrSmp(ContactSelListTable);
		globals->ContactsSelTopVisibleRecord=0;
		DmSeekRecordInCategory(globals->AddrDB, &globals->ContactsSelTopVisibleRecord, 0, dmSeekForward, category);
		
		TblEraseTable(tblP);
		TblDrawTable(tblP);

		// By changing the category the current record is lost.
		globals->ContactSelCurrentRecord = noRecord;
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvListScroll
 *
 * DESCRIPTION: This routine scrolls the list of names and phone numbers
 *              in the direction specified.
 *
 * PARAMETERS:  direction	- up or dowm
 *              units		- unit amount to scroll
 *              byLine		- if true, list scrolls in line units
 *									- if false, list scrolls in page units
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			art		6/5/95	Initial Revision
 *       frigino	8/14/97	Modified to scroll by line or page in units
 *       gap   	10/12/99	Close command bar before processing scroll
 *       gap   	10/15/99	Clean up selection handling after scroll
 *       gap   	10/25/99	Optimized scrolling to only redraw if item position changed
 *
 ***********************************************************************/
void PrvContactSelListScroll (WinDirectionType direction, UInt16 units, Boolean byLine)
{
	globalVars* globals = getGlobalsPtr();
	TableType* tblP;
	FormType* frmP;
	UInt16 rowsInPage;
	UInt16 newContactsSelTopVisibleRecord;
	UInt16 prevContactsSelTopVisibleRecord = globals->ContactsSelTopVisibleRecord;
	
	// Before processing the scroll, be sure that the command bar has been closed.
	MenuEraseStatus(0);

	frmP = FrmGetActiveForm();
	tblP = CustomGetObjectPtrSmp(ContactSelListTable);
	// Safe. There must be at least one row in the table.
	rowsInPage = PrvContactSelListNumberOfRows(tblP) - 1;
	newContactsSelTopVisibleRecord = globals->ContactsSelTopVisibleRecord;

	// Scroll the table down.
	if (direction == winDown)
	{
		// Scroll down by line units
		if (byLine)
		{
			globals->ScrollPosition+=units;
			// Scroll down by the requested number of lines
			
			if (!ContactsSelToolsSeekRecord (&newContactsSelTopVisibleRecord, units, dmSeekForward))
			{
				// Tried to scroll past bottom. Goto last record
				newContactsSelTopVisibleRecord = dmMaxRecordIndex;
				ContactsSelToolsSeekRecord (&newContactsSelTopVisibleRecord, 1, dmSeekBackward);
				globals->ScrollPosition--;
			}
		}
		// Scroll in page units
		else
		{
			globals->ScrollPosition+=units*rowsInPage;
			
			// Try scrolling down by the requested number of pages
			if (!ContactsSelToolsSeekRecord (&newContactsSelTopVisibleRecord, units * rowsInPage, dmSeekForward))
			{
				globals->ScrollPosition+=units * rowsInPage;
				// Hit bottom. Try going backwards one page from the last record
				newContactsSelTopVisibleRecord = dmMaxRecordIndex;
				if (!ContactsSelToolsSeekRecord (&newContactsSelTopVisibleRecord, rowsInPage, dmSeekBackward))
				{
					// Not enough records to fill one page. Goto the first record
					newContactsSelTopVisibleRecord = 0;
					ContactsSelToolsSeekRecord (&newContactsSelTopVisibleRecord, 0, dmSeekForward);
					globals->ScrollPosition=0;
				}
			}
		}
	}
	// Scroll the table up
	else
	{
		// Scroll up by line units
		if (byLine)
		{
			globals->ScrollPosition-=units;
			// Scroll up by the requested number of lines
			if (!ContactsSelToolsSeekRecord (&newContactsSelTopVisibleRecord, units, dmSeekBackward))
			{
				// Tried to scroll past top. Goto first record
				newContactsSelTopVisibleRecord = 0;
				ContactsSelToolsSeekRecord (&newContactsSelTopVisibleRecord, 0, dmSeekForward);
				globals->ScrollPosition++;
			}
		}
		// Scroll in page units
		else
		{
			globals->ScrollPosition-=units * rowsInPage;
			// Try scrolling up by the requested number of pages
			if (!ContactsSelToolsSeekRecord (&newContactsSelTopVisibleRecord, units * rowsInPage, dmSeekBackward))
			{
				// Hit top. Goto the first record
				newContactsSelTopVisibleRecord = 0;
				ContactsSelToolsSeekRecord (&newContactsSelTopVisibleRecord, 0, dmSeekForward);
				globals->ScrollPosition=0;
			}
		}
	}


	// Avoid redraw if no change
	if (globals->ContactsSelTopVisibleRecord != newContactsSelTopVisibleRecord)
	{
		globals->ContactsSelTopVisibleRecord = newContactsSelTopVisibleRecord;
		globals->ContactSelCurrentRecord = noRecord;  	// scrolling always deselects current selection
		PrvContactSelListLoadTable(frmP);

		// Need to compare the previous top record to the current after PrvListLoadTable
		// as it will adjust ContactsSelTopVisibleRecord if drawing from newContactsSelTopVisibleRecord will
		// not fill the whole screen with items.
		if (globals->ContactsSelTopVisibleRecord != prevContactsSelTopVisibleRecord)
			TblRedrawTable(tblP);
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvListSelectRecord
 *
 * DESCRIPTION: Selects (highlights) a record on the table, scrolling
 *              the record if neccessary.  Also sets the ContactSelCurrentRecord.
 *
 * PARAMETERS:  frmP			IN	form
 *				recordNum 		IN 	record to select
 *				forceSelection	IN	force selection
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         	Name   	Date      	Description
 *        	----   	----      	-----------
 *        	roger  	6/30/95   	Initial Revision
 *			aro		9/25/00		Add frmP as a parameter for frmUpdateEvent
 *								Add a boolean to force selection
 *			MB		
 *
 ***********************************************************************/
void PrvContactSelListSelectRecord( FormType* frmP, UInt16 recordNum, Boolean forceSelection )
{
	globalVars* globals = getGlobalsPtr();
	Int16 row, column;
	TableType* tblP;
	UInt16 attr;
	RectangleType bounds;
	IndexedColorType backColor;
	
	if (recordNum == noRecord)
		return;
	ErrFatalDisplayIf (recordNum >= DmNumRecords(globals->AddrDB), "Record outside AddrDB");

	tblP = CustomGetObjectPtrSmp(ContactSelListTable);
	

	// Don't change anything if the same record is selected
	if ((TblGetSelection(tblP, &row, &column)) &&
		(recordNum == TblGetRowID (tblP, row)) &&
		(!forceSelection))
	{
		return;
	}
	
	
	if(globals->AddrListHighRes && recordNum==globals->ContactSelSelectedRecord)
		return;
	//remove selection
	TblFindRowID(tblP, globals->ContactSelCurrentRecord, &row);
	if((row>= 0) && (row<=TblGetNumberOfRows(tblP)))
	{
		TblMarkRowInvalid(tblP, row);
		TblRedrawTable(tblP);
	}		
	// See if the record is displayed by one of the rows in the table
	// A while is used because if TblFindRowID fails we need to
	// call it again to find the row in the reloaded table.
	while (!TblFindRowID(tblP, recordNum, &row))
	{

		// Scroll the view down placing the item
		// on the top row
		globals->ContactsSelTopVisibleRecord = recordNum;

		// Make sure that ContactsSelTopVisibleRecord is visible in ContactsSelCurrentCategory
		if (globals->ContactsSelCurrentCategory != dmAllCategories)
		{
			// Get the category and the secret attribute of the current record.
			DmRecordInfo (globals->AddrDB, globals->ContactsSelTopVisibleRecord, &attr, NULL, NULL);
			if ((attr & dmRecAttrCategoryMask) != globals->ContactsSelCurrentCategory)
			{
				ErrNonFatalDisplay("Record not in ContactsSelCurrentCategory");
				globals->ContactsSelCurrentCategory = (attr & dmRecAttrCategoryMask);
			}
		}

		PrvContactSelListLoadTable(frmP);
		TblRedrawTable(tblP);
	}


	// Select the item
	TblUnhighlightSelection(tblP);
	if(globals->gEachOtherSelected && !(row%2) && !TblRowMasked(tblP, row))
	{
		backColor=globals->gEachOther;
	}
	else
	{
		backColor=UIColorGetTableEntryIndex(UIFieldBackground);
	}
	WinPushDrawState();
	WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetTableEntryIndex(UIObjectForeground));
	TblGetItemBounds (tblP, row, column, &bounds);
	bounds.extent.y=TblGetRowHeight(tblP, 0);
	bounds.extent.x++;
	PrvContactSelListReplaceTwoColors(&bounds, 0,
					UIColorGetTableEntryIndex(UIObjectForeground), backColor, UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill));
	WinPopDrawState();

	globals->ContactSelCurrentRecord = recordNum;
}


/***********************************************************************
 *
 * FUNCTION:    PrvListUpdateDisplay
 *
 * DESCRIPTION: This routine update the display of the list view
 *
 * PARAMETERS:  updateCode - a code that indicated what changes have been
 *                           made to the to do list.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *				Name	Date		Description
 *				----	----		-----------
 *				art		6/5/95		Initial Revision
 *				ppo		10/13/99	Fixed bug #22753 (selection not redrawn)
 *				jmp		10/19/99	Changed previous fix to actually to set everything
 *									back up.  The previous change caused bug #23053, and
 *									didn't work in several cases anyway!  Also, optimized
 *									this routine space-wise.
 *				aro		9/26/00		Don't use GetActiveForm for frmRedrawUpdateCode
 *									Fix bug in debug ROM: selection was not restored after Dial or About
 *				fpa		10/26/00	Fixed bug #44352 (Selected line display problem when tapping
 *									menu | Dial, then cancel into Dial Number screen)
 *
 ***********************************************************************/
void PrvContactSelListUpdateDisplay( UInt16 updateCode )
{
	globalVars* globals = getGlobalsPtr();
	TableType* tblP;
	FormType* frmP;

	// Do not use active form here since the update event is broadcasted
	frmP = FrmGetFormPtr(ContactSelListView);
	tblP = CustomGetObjectPtrSmp(ContactSelListTable);
			
	if (updateCode == frmRedrawUpdateCode)
	{
		globals->ContactSelSelectedRecord=noRecord;
		TblUnhighlightSelection(tblP);	// Fixed bug #44352. If we don't do that, using a Debug rom, selection is too width when tapping Menu | Dial, then cancel into Dial Number screen (note is selected)
		PrvContactSelListInit( frmP );
			
		FrmDrawForm(frmP);
		
		return;
	}

	if(updateCode & updatePrefs)
	{
		PrvContactSelListInit( frmP );
		FrmDrawForm(frmP);
		TblRedrawTable(tblP);
	}
	
	if (updateCode & updateRedrawAll ||	updateCode&updateColorsChanged)
	{
		PrvContactSelListLoadTable(frmP);
		if(updateCode&updateColorsChanged)
		{
			if (/*((globals->gScreen==CLIE_320x320) && !globals->DIA) || */updateCode&updateColorsChanged)
				FrmEraseForm(frmP);
		}
		FrmDrawForm(frmP);
		
	
		TblRedrawTable(tblP);
	}
}

/***********************************************************************
 *
 * FUNCTION:    PrvListHandleRecordSelection
 *
 * DESCRIPTION: This routine handles table selection in the list view,
 *					 either selecting the name to go to RecordView, or selecting
 *					 the phone number to dial.
 *
 *
 * PARAMETERS:	 event	- pointer to the table enter event
 *
 * RETURNED:	 whether the event was handled
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	05/08/00	Initial Revision
 *			aro		06/22/00	Add dialing checking and feature
 *			aro		9/25/00		Disable selection when entering the table
 *			MB		03/02/04	Fix for row height !=11
 *
 ***********************************************************************/
Boolean PrvContactSelListHandleRecordSelection( EventType* event )
{
	TablePtr table;
	Int16 row, rowPrev, column, phoneX;
	Boolean isSelected, isPenDown;
	RectangleType bounds;
	Coord x, y;
	globalVars* globals = getGlobalsPtr();	
	IndexedColorType backColor;
			
	table = event->data.tblSelect.pTable;
	row = event->data.tblSelect.row;
	column = event->data.tblSelect.column;

	// If the column being tapped on isn't the name and number column,
	// let the table handle selection to view the note.
	
	// Extract the x coordinate of the start of the phone number for this row.
	// This was computed and stored in the row data when the row was drawn.
	phoneX = TblGetRowData(table, row);
	if(globals->gEachOtherSelected && !(row%2) && !TblRowMasked(table, row) && column!=noteColumn)
	{
		backColor=globals->gEachOther;
	}
	else
	{
		backColor=UIColorGetTableEntryIndex(UIFieldBackground);
	}
			
	TblFindRowID(table, globals->ContactSelCurrentRecord, &rowPrev);
	if((rowPrev>= 0) && (rowPrev<=TblGetNumberOfRows(table)))
	{
		TblMarkRowInvalid(table, rowPrev);
		TblRedrawTable(table);
	}	
	// Disable the current selection
	globals->ContactSelCurrentRecord = noRecord;
	if(column!=noteColumn)
	{		
		TblFindRowID(table, globals->ContactSelCurrentRecord, &rowPrev);
		if((rowPrev>= 0) && (rowPrev<=TblGetNumberOfRows(table)))
		{
			TblMarkRowInvalid(table, rowPrev);
			TblRedrawTable(table);
		}	
		// Disable the current selection
		globals->ContactSelCurrentRecord = noRecord;

		isSelected=false;
		WinPushDrawState();
		if(globals->gEachOtherSelected && !(row%2) && !TblRowMasked(table, row) && column!=noteColumn)
		{
			WinSetBackColor(backColor);
		}
		else
		{
			WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
		}
		WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
		WinSetTextColor(UIColorGetTableEntryIndex(UIObjectForeground));
		TblGetItemBounds (table, row, column, &bounds);
		bounds.extent.y=TblGetRowHeight(table, 0);
		bounds.extent.x++;
		// Draw the phone number selected.
		
		
		PrvContactSelListReplaceTwoColors(&bounds, 0,
							UIColorGetTableEntryIndex(UIObjectForeground), backColor, UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill));
			
		isSelected = true;
		do 
		{
			PenGetPoint (&x, &y, &isPenDown);
			if (RctPtInRectangle (x, y, &bounds))
			{
				if (! isSelected)
				{
					isSelected = true;
					PrvContactSelListReplaceTwoColors(&bounds, 0,
									UIColorGetTableEntryIndex(UIObjectForeground), backColor, UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill));
				}
			}
		}
		while (isPenDown);
		if (isSelected)
		//PrvContactSelListReplaceTwoColors(&bounds, 0,
		//						UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill), UIColorGetTableEntryIndex(UIObjectForeground), backColor);
	
		WinPopDrawState();
				
		if (RctPtInRectangle (x, y, &bounds))
		{
					
			// An item in the list of names and phone numbers was selected, go to
			// the record view.
			globals->ContactSelCurrentRecord = TblGetRowID (event->data.tblSelect.pTable,
								 event->data.tblSelect.row);
			// Set the global variable that determines which field is the top visible
			// field in the edit view.  Also done when New is pressed.
			globals->TopVisibleFieldIndex = 0;
			globals->EditRowIDWhichHadFocus = editFirstFieldIndex;
			globals->EditFieldPosition = 0;
		}		
	}
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    PrvListReplaceTwoColors
 *
 * DESCRIPTION: This routine does a selection or deselection effect by
 *					 replacing foreground and background colors with a new pair
 *					 of colors. In order to reverse the process, you must pass
 *					 the colors in the opposite order, so that the current
 *					 and new colors are known to this routine. This routine
 *					 correctly handling the cases when two or more of these
 *					 four colors are the same, but it requires that the
 *					 affected area of the screen contains neither of the
 *					 given NEW colors, unless these colors are the same as
 *					 one of the old colors.
 *
 * PARAMETERS:	 rP  - pointer to a rectangle to 'invert'
 *					 cornerDiam	- corner diameter
 *					 oldForeground	- UI color currently used for foreground
 *					 oldBackground	- UI color currently used for background
 *					 newForeground	- UI color that you want for foreground
 *					 newBackground	- UI color that you want for background
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	05/19/00	Initial Revision
 *
 ***********************************************************************/
void PrvContactSelListReplaceTwoColors (const RectangleType *rP, UInt16 cornerDiam, IndexedColorType oldForeground, IndexedColorType oldBackground, IndexedColorType newForeground, IndexedColorType newBackground)
{
	WinPushDrawState();
	WinSetDrawMode(winSwap);
	WinSetPatternType (blackPattern);

	if (newBackground == oldForeground)
	{
		if (newForeground == oldBackground)
		{
			// Handle the case when foreground and background colors change places,
			// such as on black and white systems, with a single swap.
			WinSetBackColor(oldBackground);
			WinSetForeColor(oldForeground);
			WinPaintRectangle(rP, cornerDiam);
		}
		else
		{
			// Handle the case when the old foreground and the new background
			// are the same, using two swaps.
			WinSetBackColor(oldForeground);
			WinSetForeColor(oldBackground);
			WinPaintRectangle(rP, cornerDiam);
			WinSetBackColor(oldBackground);
			WinSetForeColor(newForeground);
			WinPaintRectangle(rP, cornerDiam);
		}
	}
	else if (oldBackground==newForeground)
	{
		// Handle the case when the old background and the new foreground
		// are the same, using two swaps.
		WinSetBackColor(newForeground);
		WinSetForeColor(oldForeground);
		WinPaintRectangle(rP, cornerDiam);
		WinSetBackColor(newBackground);
		WinSetForeColor(oldForeground);
		WinPaintRectangle(rP, cornerDiam);
	}
	else
	{
		// Handle the case when no two colors are the same, as is typically the case
		// on color systems, using two swaps.
		WinSetBackColor(oldBackground);
		WinSetForeColor(newBackground);
		WinPaintRectangle(rP, cornerDiam);
		WinSetBackColor(oldForeground);
		WinSetForeColor(newForeground);
		WinPaintRectangle(rP, cornerDiam);
	}

	WinPopDrawState();
}
