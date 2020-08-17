/******************************************************************************
 *
 * Copyright (c) 1995-2002 PalmSource, Inc. All rights reserved.
 *
 * File: AddrEdit.c
 *
 * Release: Palm OS 5 SDK (68K) R3.
 *
 * Description:
 *   This is the Address Book application's edit form module.
 *
 *****************************************************************************/

#include <TextMgr.h>
#include <ErrorMgr.h>
#include <StringMgr.h>
#include <AboutBox.h>
#include <Category.h>
#include <Graffiti.h>
#include <Menu.h>
#include <UIResources.h>

#include <PalmUtils.h>
#include <FeatureMgr.h>

#include "AddrEdit.h"

#include <HsNavCommon.h>
#include <HsExt.h>
#include "AddrDialList.h"
#include "Address.h"
#include "AddressAutoFill.h"
#include "AddressRsc.h"
#include "AddressDB2.h"
#include "ContactsDB2.h"
#include "AddrDefines.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "AddrDetails.h"
#include "AddrNote.h"
#include "AddressTransfer.h"
#include "AddressTransfer2.h"
#include "globals.h"
//#include "Mapopolis.h"
#include "dia.h"
#include "syslog.h"
#include "AddrPrefs.h"

/***********************************************************************
 *
 *   Defines
 *
 ***********************************************************************/

#define addrEditLabelFont	stdFont
#define addrEditBlankFont	stdFont

#define noFieldIndex		0xff

// Resource type used to specify order of fields in Edit view.
#define	fieldMapRscType		'fmap'

// Address edit table's rows and columns
#define editLabelColumn		0
#define editDataColumn		1

#define spaceBeforeDesc		2

#define isPhoneField(f)		(f >= firstPhoneField && f <= lastPhoneField)

#define editInvalidRow		((UInt16) 0xFFFF)

P1ContactsFields FieldMapIndexes[40] = 
{
	P1Contactsname,
	P1ContactsfirstName,
	P1ContactspictureInfo,
	//P1Contactsringer,
	P1Contactscompany,
	P1Contactstitle,
	P1Contactsphone1,
	P1Contactsphone2,
	P1Contactsphone3,
	P1Contactsphone4,
	P1Contactsphone5,
	P1Contactsphone6,
	P1Contactsphone7,
	P1Contactschat1,
	P1Contactschat2,
	P1Contactswebpage,
	P1Contactsaddress,
	P1Contactscity,
	P1Contactsstate,
	P1ContactszipCode,
	P1Contactscountry,
	P1Contactsaddress2,
	P1Contactscity2,
	P1Contactsstate2,
	P1ContactszipCode2,
	P1Contactscountry2,
	P1Contactsaddress3,
	P1Contactscity3,
	P1Contactsstate3,
	P1ContactszipCode3,
	P1Contactscountry3,
	P1ContactsbirthdayDate,
	//P1ContactsanniversaryDate,
	P1Contactscustom1,
	P1Contactscustom2,
	P1Contactscustom3,
	P1Contactscustom4,
	P1Contactscustom5,
	P1Contactscustom6,
	P1Contactscustom7,
	P1Contactscustom8,
	P1Contactscustom9
};

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/
Boolean			EditOnOpen();
Boolean			EditOnUpdate(EventType * event);

static void		PrvEditInit( FormPtr frmP, Boolean leaveDataLocked );
static void 	PrvEditInitTableRow( FormPtr frmP, TablePtr table, UInt16 row, UInt16 fieldIndex, FontID fontID, void* record, void* appInfoPtr );
static Boolean	PrvEditHandleSelectField (Int16 row, Int16 column);
static void		PrvEditRestoreEditState( FormType* frmP );
static Boolean	PrvEditSetGraffitiMode (FieldPtr fld, UInt16 currentField);
static Err		PrvEditGetRecordField (void * table, Int16 row, Int16 /*column*/, Boolean editing, MemHandle * textH, Int16 * textOffset, Int16 * textAllocSize, FieldPtr fld);
static Boolean	PrvEditSaveRecordField (void * table, Int16 row, Int16 /*column*/);
static UInt16	PrvEditSaveRecord ();
static void		PrvEditSelectCategory (void);
static void		PrvEditUpdateScrollers (FormPtr frmP, UInt16 bottomFieldIndex, Boolean lastItemClipped);
static UInt16 	PrvEditGetFieldHeight (TablePtr table, UInt16 fieldIndex, Int16 columnWidth, Int16 maxHeight, univAddrDBRecordPtr record, FontID * fontIdP);
static void		PrvEditDrawBusinessCardIndicator (FormPtr formP);
static void		PrvEditResizeDescription (EventType * event);
static void		PrvEditScroll (WinDirectionType direction);
static Boolean	PrvEditNextField (WinDirectionType direction);
static void		PrvEditUpdateCustomFieldLabels( FormType* frmP);
static void		PrvEditUpdateDisplay( UInt16 updateCode );
static Boolean	PrvEditDoCommand (UInt16 command);
static Boolean	PrvEditAutoFill (EventPtr event);
static Boolean	PrvEditDialCurrent( void );
static void		PrvEditLoadTable( FormType* frmP );
static Boolean 	AddrEditMoveObjects(FormType* frmP, Coord dx, Coord dy);
static Boolean 	EditOnKeyDown(EventPtr event);
static void 	EditDrawRecord(MemPtr tTable, Int16 tRow, Int16 tColumn, RectangleType *tBounds);

static Boolean 	AddrEditPopupTrigger(TablePtr table);
static Boolean 	NewContactFields();
static void 	FillRingerList();
static void 	RingerListAddEntry(const Char* tEntry);

static void	FillRingerList()
{
	globalVars* globals = getGlobalsPtr();
	ListType *lRingerListPtr=(ListType*)GetObjectPtrSmp(EditRingtoneList); 
	
	UInt16 lSize=globals->gRingerCount;
	if(lSize>14) lSize=14;
	LstSetHeight(lRingerListPtr, lSize);
	LstSetListChoices(lRingerListPtr, globals->gRingerList.Pointer, globals->gRingerList.Size);
}

static Boolean EditOnKeyDown(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	UInt32 id;
	FormPtr frmP = FrmGetActiveForm();
	TablePtr tableP;
	FieldPtr fldP;
	
	tableP = CustomGetObjectPtrSmp(EditTable);
	fldP = TblGetCurrentField(tableP);			
	
	if(TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
	{
		if(globals->gDeviceFlags.bits.treoWithPhoneKeyOnly && event->data.keyDown.chr == vchrHard1)
		{
			if(PrvEditDialCurrent() == false)
				StartPhone();
		}
		else if(globals->gDeviceFlags.bits.treoWithSendKeys && event->data.keyDown.chr == vchrHard11)
		{
			PrvEditDialCurrent();	
		}
		return true;
	}
	else if(event->data.keyDown.modifiers == 0 && event->data.keyDown.chr == chrHorizontalTabulation)
	{
		PrvEditNextField (winDown);
		return true;						
	}
	else if(event->data.keyDown.modifiers == commandKeyMask && event->data.keyDown.chr == vchrPrevField)
	{
		PrvEditNextField (winUp);
		return true;						
	}
	else if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
	{
		TblReleaseFocus(CustomGetObjectPtrSmp(EditTable));
		globals->TopVisibleRecord = 0;      // Same as when app switched to
		globals->CurrentFieldIndex = noFieldIndex;
		FrmGotoForm (ListView);
		return (true);
	}
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event))
	{
		UInt16 scrollPos, textHeight, fieldHeight;
		if(globals->gNavigation)
		{
			if(fldP)
				FldGetScrollValues(fldP, &scrollPos, &textHeight, &fieldHeight);
			if(!globals->gEditTableActive)
			{
				return false;
			}
			if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
			{
				FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, EditDoneButton);
			}
			else if(ToolsNavKeyPressed(globals->adxtLibRef, event, Down))
			{
				if(event->data.keyDown.modifiers & optionKeyMask)
				{
					PrvEditScroll(winDown);
					return true;
				}
				if(fldP && fieldHeight > 1)
				{
					if(FldHandleEvent(fldP, event))
						return true;
				}
				if(!PrvEditNextField (winDown))
				{
					FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, EditDoneButton);	
				}
				return true;
			}		
			else if(ToolsNavKeyPressed(globals->adxtLibRef, event, Up))
			{
				if(event->data.keyDown.modifiers & optionKeyMask)
				{
					PrvEditScroll(winUp);
					return true;
				}
				if(fldP && fieldHeight > 1)
				{
					if(FldHandleEvent(fldP, event))
						return true;
				}
				if(!PrvEditNextField (winUp))
				{
					FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, EditCategoryTrigger);	
				}
				return true;	
			}		
			else if(ToolsNavKeyPressed(globals->adxtLibRef, event, Left))
			{
				if(fldP)
				{
					if(FldHandleEvent(fldP, event))
						return true;					
					else if(AddrEditPopupTrigger(tableP))
						return true;
					else if(PrvEditNextField (winLeft))
						return true;
					else
					{
						FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, EditCategoryTrigger);	
						return true;
					}
				}
				return true;
			}		
			else if(ToolsNavKeyPressed(globals->adxtLibRef, event, Right))
			{
				if(fldP)
				{
					if(FldHandleEvent(fldP, event))
						return true;
					else
					{
						FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, EditDoneButton);	
						return true;
					}
				}
				return true;
			}		
		}
		
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select) && !globals->gNavigation)
		{
			TblReleaseFocus(CustomGetObjectPtrSmp(EditTable));
			globals->TopVisibleRecord = 0;// Same as when app switched to
			globals->CurrentFieldIndex = noFieldIndex;
			FrmGotoForm (ListView);
			return true;
		}
  	}
	if (EvtKeydownIsVirtual(event) && !globals->gNavigation)
	{
		FtrGet(sysFtrCreator,sysFtrNumOEMDeviceID, &id);
		switch (event->data.keyDown.chr)
		{
			
			case vchrPageUp:
			case vchrRockerUp:
				if(id!='H101' && id!= 'H201' && id!='H102' && id!= 'H202')
					PrvEditScroll (winUp);
				else
					PrvEditNextField (winUp);				
				return true;
				break;

			case vchrPageDown:
			case vchrRockerDown:
				if(id!='H101' && id!= 'H201' && id!='H102' && id!= 'H202')
					PrvEditScroll (winDown);
				else
					PrvEditNextField (winDown);
				return true;
				break;

			case vchrNextField:
				PrvEditNextField (winDown);
				return true;
				break;

			case vchrPrevField:
				PrvEditNextField (winUp);
				return true;
				break;

			case vchrSendData:
				// Make sure the field being edited is saved
				TblReleaseFocus(tableP);

				MenuEraseStatus (0);
				TransferSendRecord(globals->AddrDB, globals->CurrentRecord, exgBeamPrefix, NoDataToBeamAlert);
				return true;
				break;

			default:
				break;
		}
	}
	else
	{
		return PrvEditAutoFill(event);
	}

	return false;
	
}

static Boolean AddrEditPopupTrigger(TablePtr table)
{
	globalVars* globals = getGlobalsPtr();
	Int16 row, column, new_value;
	ListPtr list;
	RectangleType cell;
	UInt16 list_i, fieldIndex;
	
	// get current row
	TblGetSelection(table, &row, &column);
	column = 0;
	
	fieldIndex = TblGetRowID(table, row);
#ifdef CONTACTS
	if(P1ContactsisChatField(globals->FieldMap[fieldIndex]))
		list_i = EditIMList;
	else if(P1ContactsisAddressField(globals->FieldMap[fieldIndex]))
		list_i = EditAddressList;
	else if(P1ContactsisPhoneField(globals->FieldMap[fieldIndex]))
		list_i = EditPhoneList;
	else
		return false;
#else
	if( isPhoneField(globals->FieldMap[fieldIndex]) )
		list_i = EditPhoneList;
	else
		return false;
#endif
	
	list = CustomGetObjectPtrSmp(list_i);
	if(list == NULL) return false;
	
	TblGetItemBounds(table, row, column, &cell);
	LstSetPosition(list, cell.topLeft.x, cell.topLeft.y);
	LstSetSelection(list, TblGetItemInt(table, row, column) );
	new_value = LstPopupList(list);
	
	if(new_value == -1) return true;
	
	TblSetItemInt(table, row, column, new_value);
	PrvEditHandleSelectField (row, column);
	return true;
}

static Boolean AddrEditMoveObjects(FormType* frmP, Coord dx, Coord dy)
{
	globalVars* globals = getGlobalsPtr();
	Boolean resized = false;
    RectangleType r;    
    if (dx != 0 || dy != 0)
    {
#ifdef DEBUG
		LogWrite("xt_log", "edit", "mover");
#endif
		if(globals->gNavigation)
		{
			WinGetBounds(WinGetDisplayWindow(), &r);
			WinEraseRectangle (&r, 0);
		}
        // NOTE: be careful to avoid object migration!
		MoveFormObjectHide(frmP, EditDoneButton, 0, dy);
        MoveFormObjectHide(frmP, EditDetailsButton, 0, dy);
        MoveFormObjectHide(frmP, EditNoteButton, 0, dy);
       	MoveFormObjectHide(frmP, EditUpButton, dx, dy);
       	MoveFormObjectHide(frmP, EditDownButton, dx, dy);
       	MoveFormObject(frmP, EditCategoryTrigger, dx, 0);
       	MoveFormObject(frmP, EditCategoryList, dx, 0);
       	
        // move the Graffiti Shift indicator
        // (FrmGetObjectIndex does not work on GSI objects)
        MoveFormGSI(frmP, dx, dy);
        resized = true;
    }
    FrmUpdateForm(EditView, 0);
    return resized;
}

Boolean NewContactFields()
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gDeviceType == type_treo700 || globals->gDeviceType == type_treo680
	|| globals->gDeviceType == type_treo755 || globals->gDeviceType == type_centro)
	{
		return false;
	}
	else
	{
		return false;
	}
}

Boolean EditOnOpen()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 tableIndex;
	FieldPtr fldP;
	FormPtr frmP;
	Int16 row;
	TablePtr tableP;
	
#ifdef DEBUG
	LogWrite("xt_log", "edit", "EditOnOpen");
#endif
	
	CustomHideObjectSmp(EditDoneButton);
	CustomHideObjectSmp(EditDetailsButton);
	CustomHideObjectSmp(EditNoteButton);
	
	globals->editLastFieldIndex = univNote-univName-1 + 1;//plus birthday
	if(NewContactFields())
	{
		globals->editLastFieldIndex += 2;//plus ringer, anniversary
	}
	
	globals->gEditTableActive = true;	
	globals->gMenuVisible=false;
	globals->gAddressView=true;
	frmP = FrmGetActiveForm ();
	
	dia_save_state();
	dia_enable(frmP, true);
	dia_resize(frmP, AddrEditMoveObjects);
	globals->skipWinEnter = dia_skip_first();
	
	// FIXME: is redundant in general, 
	// but it needs for Garmin, Zodiac -- got error in GetWindowHeight() from PrvEditInit()
	//FrmDrawForm (frmP);
#ifdef DEBUG
	LogWrite("xt_log", "edit", "EditOnOpen: FrmDrawForm");
#endif

	globals->TopVisibleFieldIndex = globals->EditRowIDWhichHadFocus;
	globals->CurrentFieldIndex = globals->EditRowIDWhichHadFocus;
	PrvEditInit (frmP, true);
	FrmDrawForm(FrmGetActiveForm ());
	tableIndex = FrmGetObjectIndex(frmP, EditTable);
	tableP = FrmGetObjectPtr (frmP, tableIndex);
	// Make sure the field which will get the focus is visible
	
	
	while (!TblFindRowID (tableP, globals->EditRowIDWhichHadFocus, &row))
	{
		globals->TopVisibleFieldIndex = globals->EditRowIDWhichHadFocus;
		globals->CurrentFieldIndex = globals->EditRowIDWhichHadFocus;
		PrvEditLoadTable(frmP);
	}
	
	
	FrmDrawForm (frmP);
#ifdef DEBUG
	LogWrite("xt_log", "edit", "EditOnOpen: FrmDrawForm 2");
#endif
	CustomShowObjectSmp(EditDoneButton);
	CustomShowObjectSmp(EditDetailsButton);
	CustomShowObjectSmp(EditNoteButton);
	PrvEditDrawBusinessCardIndicator (frmP);

	// Now set the focus.
	FrmSetFocus(frmP, tableIndex);
	TblGrabFocus (tableP, row, editDataColumn);
	fldP = TblGetCurrentField(tableP);
	FldGrabFocus (fldP);
	

	// If NumCharsToHilite is not 0, then we know that we are displaying
	// a duplicated message for the first time and we must hilite the last
	// NumCharsToHilite of the field (first name) to indicate the modification
	// to that duplicated field.
	if (globals->NumCharsToHilite > 0)
	{
		globals->EditFieldPosition = FldGetTextLength (fldP);

		// Now hilite the chars added.
		FldSetSelection (fldP, globals->EditFieldPosition - globals->NumCharsToHilite, globals->EditFieldPosition);
		globals->NumCharsToHilite = 0;
		FldSetInsPtPosition (fldP, globals->EditFieldPosition);
	}
	else if(globals->gNavigation)
	{
		globals->EditFieldPosition = FldGetTextLength (fldP);
		FldSetInsPtPosition (fldP, globals->EditFieldPosition);
	}
	else
	{
		FldSetInsPtPosition (fldP, globals->EditFieldPosition);
	}
	
	globals->PriorAddressFormID = FrmGetFormId (frmP);
	
	// Simulate a tap in last name field in order to fix bug #27480
	PrvEditHandleSelectField(globals->EditRowIDWhichHadFocus - globals->TopVisibleFieldIndex, 1);
	
	if(globals->gDeviceFlags.bits.treo)
	{
		FrmSetMenu(FrmGetActiveForm(), EditViewMenuBarTreo);
	}
	SetDialer();

	return true;
}

Boolean EditOnUpdate(EventType * event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetFormPtr (EditView);
	TablePtr tableP = CustomGetObjectPtr(frmP, EditTable);
	Boolean hasData;
	UInt16 tableIndex;
	MemHandle currentRecordH;
	P1ContactsDBRecordType currentContact;
	UInt16 focus;
#ifndef CONTACTS
	AddrAppInfoPtr appInfoPtr;
#else
	P1ContactsAppInfoPtr contactsInfoPtr;
#endif
	IndexedColorType formFillColor,
                         oldBackColor;
    
   
#ifdef DEBUG
	LogWrite("xt_log", "edit", "EditOnUpdate");
#endif

	if(event->data.frmUpdate.formID != EditView)
		return true;
	if(event->data.frmUpdate.updateCode == frmRedrawUpdateCode)
	{
		FrmDrawForm(frmP);
		return true;
	}
	else if(event->data.frmUpdate.updateCode == updateBirthday)
	{
		P1ContactsDBRecordType record;
		MemHandle recordH;
		PrvP1ContactsDBGetRecordBDOnly (globals->AddrDB, globals->CurrentRecord, &record, &recordH);	
		// Get the height of the table and the width of the description
		// column.
		if(record.birthdayInfo.birthdayDate.day != 0 || record.birthdayInfo.birthdayDate.month != 0)
		{
			DateFormatType dateFormat;
			dateFormat=PrefGetPreference(prefLongDateFormat);
			DateToAscii(record.birthdayInfo.birthdayDate.month, record.birthdayInfo.birthdayDate.day, record.birthdayInfo.birthdayDate.year+1904, dateFormat, globals->gDateTxt);
		}
		else
		{
			StrCopy(globals->gDateTxt, " -Tap to add- ");
		}	
		CustomSetCtlLabelPtrSmp(EditBirthdaySelector, globals->gDateTxt);	
		MemHandleUnlock (recordH);
		return true;
	}
	 
	if(globals->gNavigation)
   	{
   		if(FrmGlueNavGetFocusRingInfo (globals->adxtLibRef, frmP, &focus, NULL, NULL, NULL) != errNone)
   			focus = noFocus;
   	}
   	
   	TblReleaseFocus(tableP);
		
	PrvP1ContactsDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &currentContact, &currentRecordH);
	hasData = AddrDBRecordContainsData(&currentContact);

	// Unlock before the DeleteRecord.   We can only rely on
	// NULL pointers from here on out.
	MemHandleUnlock(currentRecordH);
	
   	if (!hasData)
	{
		PrvEditInit(frmP, false);
		FrmDrawForm(frmP);
#ifdef DEBUG
LogWrite("xt_log", "edit", "EditOnUpdate: FrmDrawForm");
#endif
		PrvEditHandleSelectField(globals->EditRowIDWhichHadFocus - globals->TopVisibleFieldIndex, 1);
		return true;		
	}
	// Check if the record is empty and should be deleted.  This cannot
	// be done earlier because if the record is deleted there is nothing
	// to display in the table.

	// We need to unlock the block containing the phone labels.
#ifdef CONTACTS
	contactsInfoPtr = (P1ContactsAppInfoPtr) P1ContactsDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	MemPtrUnlock(contactsInfoPtr);	// Call to AddrAppInfoGetPtr did a lock
	MemPtrUnlock(contactsInfoPtr);   // Unlock lock in PrvEditInit		
#else
	appInfoPtr = (AddrAppInfoPtr) AddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	MemPtrUnlock(appInfoPtr);	// Call to AddrAppInfoGetPtr did a lock
	MemPtrUnlock(appInfoPtr);   // Unlock lock in PrvEditInit
	MemHandleUnlock(globals->FieldMapH);
	DmReleaseResource(globals->FieldMapH);
	globals->FieldMap = 0;
#endif

	// We need to unlock the FieldMap resource, which was also locked
	// in PrvEditInit.
	tableIndex = FrmGetObjectIndex(frmP, EditTable);
	tableP = FrmGetObjectPtr (frmP, tableIndex);
	PrvEditInit (frmP, true);
	FrmDrawForm (frmP);
#ifdef DEBUG
LogWrite("xt_log", "edit", "EditOnUpdate: FrmDrawForm");
#endif

	PrvEditDrawBusinessCardIndicator (frmP);
	PrvEditHandleSelectField(globals->EditRowIDWhichHadFocus - globals->TopVisibleFieldIndex, 1);

	PrvEditUpdateDisplay(event->data.frmUpdate.updateCode);
	
	if(globals->gNavigation && focus != noFocus)
   	{
   	   	FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, focus);
   	}
   	return true;						
}

static Boolean EditOnWinEnterEvent(EventType* event)
{
	if(event->data.winEnter.enterWindow != 0)
		dia_win_enter();
	return false;
}

static Boolean EditOnFrmCloseEvent()
{
	globalVars* globals = getGlobalsPtr();
#ifndef CONTACTS
	AddrAppInfoPtr appInfoPtr;
#else
	P1ContactsAppInfoPtr contactsInfoPtr;
#endif

	PrvEditSaveRecord ();	
	// We need to unlock the block containing the phone labels.
#ifdef CONTACTS
	contactsInfoPtr = (P1ContactsAppInfoPtr) P1ContactsDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	MemPtrUnlock(contactsInfoPtr);	// Call to AddrAppInfoGetPtr did a lock
	MemPtrUnlock(contactsInfoPtr);   // Unlock lock in PrvEditInit		
#else
	appInfoPtr = (AddrAppInfoPtr) AddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	MemPtrUnlock(appInfoPtr);	// Call to AddrAppInfoGetPtr did a lock
	MemPtrUnlock(appInfoPtr);   // Unlock lock in PrvEditInit
	MemHandleUnlock(globals->FieldMapH);
	DmReleaseResource(globals->FieldMapH);
	globals->FieldMap = 0;
#endif

	// We need to unlock the FieldMap resource, which was also locked
	// in PrvEditInit.
	
	return false;
}

static Boolean EditOnTblEnterEvent(EventType* event)
{
	// if a phone label is tapped: store current 
	globalVars* globals = getGlobalsPtr();
	if (univIsPhoneField(globals->FieldMap[TblGetRowID(event->data.tblEnter.pTable, event->data.tblEnter.row)]))
	{
		globals->CurrentTableRow = event->data.tblEnter.row;
	}
	return false;
}

static Boolean EditOnTblSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
#ifdef CONTACTS
	if (globals->CurrentFieldIndex != TblGetRowID (event->data.tblSelect.pTable, event->data.tblSelect.row) ||
		(event->data.tblSelect.column == editLabelColumn  
		
		&& (P1ContactsisPhoneField(globals->FieldMap[TblGetRowID(event->data.tblSelect.pTable, event->data.tblSelect.row)])

		|| P1ContactsisChatField(globals->FieldMap[TblGetRowID(event->data.tblSelect.pTable, event->data.tblSelect.row)])		
		
		|| P1ContactsisAddressField(globals->FieldMap[TblGetRowID(event->data.tblSelect.pTable, event->data.tblSelect.row)]) )
		
		))
#else
	if (globals->CurrentFieldIndex != TblGetRowID (event->data.tblSelect.pTable, event->data.tblSelect.row) ||
		(event->data.tblSelect.column == editLabelColumn && isPhoneField(globals->FieldMap[
																		  TblGetRowID(event->data.tblSelect.pTable, event->data.tblSelect.row)])))
#endif
	{
		PrvEditHandleSelectField (event->data.tblSelect.row, event->data.tblSelect.column);
	}
	globals->CurrentTableRow = editInvalidRow;
#ifdef CONTACTS
	if(globals->gDeviceFlags.bits.treo)
	{
		Boolean isNumericField;
		P1ContactsDBRecordType currentContact;
		MemHandle currentRecordH; 
		P1ContactsFields contactField = globals->FieldMap[TblGetRowID(event->data.tblSelect.pTable, event->data.tblSelect.row)];
		
		PrvP1ContactsDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &currentContact, &currentRecordH);
		
		if(currentRecordH)
		{
			if(P1ContactsisPhoneField(contactField))
			{
				isNumericField = true;
				switch(contactField)
				{
					case P1Contactsphone1:
						if(currentContact.options.phones.phone1 == P1ContactsemailLabel)
							isNumericField = false;
						break;
					case P1Contactsphone2:
						if(currentContact.options.phones.phone2 == P1ContactsemailLabel)
							isNumericField = false;
						break;
					case P1Contactsphone3:
						if(currentContact.options.phones.phone3 == P1ContactsemailLabel)
							isNumericField = false;
						break;
					case P1Contactsphone4:
						if(currentContact.options.phones.phone4 == P1ContactsemailLabel)
							isNumericField = false;
						break;
					case P1Contactsphone5:
						if(currentContact.options.phones.phone5 == P1ContactsemailLabel)
							isNumericField = false;
						break;
					case P1Contactsphone6:
						if(currentContact.options.phones.phone6 == P1ContactsemailLabel)
							isNumericField = false;
						break;
					case P1Contactsphone7:
						if(currentContact.options.phones.phone7 == P1ContactsemailLabel)
							isNumericField = false;
						break;
				}		
			}
			else
				isNumericField = (contactField == P1ContactszipCode) || (contactField == P1ContactszipCode2) || (contactField == P1ContactszipCode3);

			MemHandleUnlock(currentRecordH);
			
			if(isNumericField)
				HsGrfSetStateExt(false, true, true, false, false, false);
		}
	}
#endif
		
	return false;
}

static Boolean EditOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	UInt16 index;
	switch (event->data.ctlSelect.controlID)
	{
		case EditCategoryTrigger:
			PrvEditSelectCategory ();
			PrvEditRestoreEditState(FrmGetActiveForm());			
			handled = false;
			break;
		case EditBirthdaySelector:
			FrmPopupForm (BirthdayDialog);
			handled = true;
			break;
		case EditRingerTrigger:
			handled = true;
			break;

		case EditDoneButton:
			FrmSetFocus(FrmGetActiveForm(), noFocus);
			globals->SelectedRecord=noRecord;
			
			FrmGotoForm (ListView);
			handled = true;
			break;

		case EditDetailsButton:
			FrmPopupForm (DetailsDialog);
			handled = true;
			break;

		case EditNoteButton:
			if (NoteViewCreate())
			{
				globals->RecordNeededAfterEditView = true;
				FrmGotoForm (NewNoteView);
			}
			handled = true;
			break;
		default:
			break;
	}
	return handled;
}

static Boolean EditOnCtlRepeatEvent(EventType* event)
{
	switch (event->data.ctlRepeat.controlID)
	{
		case EditUpButton:
			PrvEditScroll (winUp);
			// leave unhandled so the buttons can repeat
			break;

		case EditDownButton:
			PrvEditScroll (winDown);
			// leave unhandled so the buttons can repeat
			break;
		default:
			break;
	}
	return false;
}

static Boolean EditOnMenuEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gDeviceFlags.bits.treo)
	{
		switch(event->data.menu.itemID)
		{
			case 10100:
			case 10101:
			case 10102:
			case 10103:
			case 10104:
			case 10106:
			case 10107:
				event->data.menu.itemID -= 100;
				EvtAddEventToQueue(event); 
				break;
		}
	}
	return PrvEditDoCommand (event->data.menu.itemID);
}

static Boolean EditOnMenuCmdBarOpenEvent(EventType* event)
{
	FieldType* fldP;
	UInt16 startPos, endPos;

	fldP = TblGetCurrentField(CustomGetObjectPtrSmp(EditTable));
	if (fldP)
		FldGetSelection(fldP, &startPos, &endPos);

	if ((fldP) && (startPos == endPos))  // there's no highlighted text
	{
		// Call directly the Field event handler so that edit buttons are added if applicable
		FldHandleEvent(fldP, event);
		
		MenuCmdBarAddButton(menuCmdBarOnRight, BarDeleteBitmap, menuCmdBarResultMenuItem, EditRecordDeleteRecordCmd, 0);
		MenuCmdBarAddButton(menuCmdBarOnLeft, BarBeamBitmap, menuCmdBarResultMenuItem, EditRecordBeamRecordCmd, 0);

		// Prevent the field package to add edit buttons again
		event->data.menuCmdBarOpen.preventFieldButtons = true;
	}
	else if (fldP == NULL)	// there is no active text field (none have cursor visible)
	{
		MenuCmdBarAddButton(menuCmdBarOnLeft, BarDeleteBitmap, menuCmdBarResultMenuItem, EditRecordDeleteRecordCmd, 0);
		MenuCmdBarAddButton(menuCmdBarOnLeft, BarBeamBitmap, menuCmdBarResultMenuItem, EditRecordBeamRecordCmd, 0);

		// Prevent the field package to add edit buttons again
		event->data.menuCmdBarOpen.preventFieldButtons = true;
	}
	else
	{
	// When there is a selection range of text (ie startPos != endPos)
	// fall through to the field code to add the appropriate cut, copy, 
	// paste, and undo selections to the command bar.
		event->data.menuCmdBarOpen.preventFieldButtons = false;
	}
	
	// don't set handled to true; this event must fall through to the system.
	return false;
}
			
static Boolean EditOnMenuOpenEvent()
{
	UInt32 numLibs, g2DynamicID;
	globalVars* globals = getGlobalsPtr();
	Err err;
	UInt32 romVersion;
	
	//if(LoadAddress(globals->CurrentRecord, NULL, true)==0)
	//	MenuHideItem(EditRecordMapCmd);
	//else if(MAPOPOLIS || globals->gTomTom)
	//	MenuShowItem(EditRecordMapCmd);
	
	//if(!(MAPOPOLIS || globals->gTomTom))
		MenuHideItem(RecordRecordMapCmd);


	if(!AddressDialable(globals->CurrentRecord, kDialListShowInListPhoneIndex) || !ToolsIsDialerPresent())
		MenuHideItem(EditRecordDialCmd);
	else
		MenuShowItem(EditRecordDialCmd);
	
	err = FtrGet(sysFtrCreator,	sysFtrNumROMVersion, &romVersion);

	if(romVersion>=0x04003000)
	{
		if (ExgGetRegisteredApplications(NULL, &numLibs, NULL, NULL, exgRegSchemeID, exgSendScheme) || !numLibs)
			MenuHideItem(EditRecordSendRecordCmd);
		else
			MenuShowItem(EditRecordSendRecordCmd);
	}
	else
		MenuHideItem(EditRecordSendRecordCmd);
		
	
	//Depending on PalmOS version, HIDE the appropriate graffiti menu item
	if(!globals->gDeviceFlags.bits.treo)
	{
	
		if (!FtrGet('grft', 1110, &g2DynamicID) ||
			!FtrGet('grf2', 1110, &g2DynamicID))
		{
			MenuHideItem(ListEditGraffitiLegacyHelpCmd);
		}
		else
		{
			MenuHideItem(ListEditGraffitiHelpCmd);
		}
	}
	return false;			
}

static Boolean EditOnFrmObjectFocusLostEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gNavigation)
	{
		if(event->data.frmObjectFocusLost.objectID == EditTable)
		{
			UInt16		focus = FrmGetFocus(FrmGetActiveForm());
			globals->gEditTableActive = false;
			if(focus != noFocus)
			{
				if(FrmGetObjectId(FrmGetActiveForm(), focus) == EditBirthdaySelector)
				{
					globals->gEditTableActive = true;
				}
			}
		}
	}
	return false;
}

static Boolean EditOnFrmObjectFocusTakeEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetFormPtr (EditView);
	TablePtr tableP = CustomGetObjectPtrSmp(EditTable);
	if(globals->gNavigation)
	{
		Int16 currRow;
		tableP = CustomGetObjectPtrSmp(EditTable);		
		switch(event->data.frmObjectFocusTake.objectID)
		{
			case EditTable:
				globals->gEditTableActive = true;
				if (TblFindRowID (tableP, globals->CurrentFieldIndex, &currRow))
				{
					PrvEditHandleSelectField (currRow, 1);	
				}
				else
				{
					PrvEditHandleSelectField (0, 1);	
				}
				break;
		}
	}
	return false;
}

static Boolean EditOnFrmSaveEvent()
{
	// Save the field being edited.  Do not delete the record if it's
	// empty because a frmSaveEvent can be sent without the form being
	// closed.  A canceled find does this.
	TblReleaseFocus(CustomGetObjectPtrSmp(EditTable));
	return false;
}

Boolean EditHandleEvent (EventType * event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	
	switch (event->eType)
	{
		case winEnterEvent:
    		handled = EditOnWinEnterEvent(event);
    		break;
    	case frmOpenEvent:
			handled = EditOnOpen();
			break;		
		case frmCloseEvent:
			handled = EditOnFrmCloseEvent();
			break;	
		case tblEnterEvent:
			handled = EditOnTblEnterEvent(event);
			break;			
		case winExitEvent:
			globals->gMenuVisible=true;
			break;			
		case tblSelectEvent:
			handled = EditOnTblSelectEvent(event);
			break;			
		case ctlSelectEvent:
			handled = EditOnCtlSelectEvent(event);
			break;	
		case ctlRepeatEvent:
			handled = EditOnCtlRepeatEvent(event);
			break;	
		case menuEvent:
			handled = EditOnMenuEvent(event);
			break;	
		case menuCmdBarOpenEvent:
			handled = EditOnMenuCmdBarOpenEvent(event);
			break;	
		case menuOpenEvent:
			handled = EditOnMenuOpenEvent();
			break;	
		case fldHeightChangedEvent:
			PrvEditResizeDescription (event);
			handled = true;
			break;		
		case frmObjectFocusLostEvent:
			handled = EditOnFrmObjectFocusLostEvent(event);
			break;
		case frmObjectFocusTakeEvent:
			handled = EditOnFrmObjectFocusTakeEvent(event);			
			break;
		case keyDownEvent:
			handled = EditOnKeyDown(event);
			break;		
		case frmUpdateEvent:
			handled = EditOnUpdate(event);
			break;	
		case frmSaveEvent:
			handled = EditOnFrmSaveEvent();			
			break;
		default:
			handled = dia_handle_event(event, AddrEditMoveObjects);
			break;
	}


	return (handled);
}

void EditNewRecord ()
{
	globalVars* globals = getGlobalsPtr();
#ifndef CONTACTS
	AddrDBRecordType newRecord;
#else
	P1ContactsDBRecordType newContact;
#endif
	AddressFields i;
	UInt16 attr;
	Err err;


	// Set up the new record
#ifdef CONTACTS
	AddressDBBirthdayFlags flag;
	
	MemSet(&newContact, sizeof(newContact), 0);
	
	flag.alarm = 0;
	flag.reserved = 0;
	newContact.birthdayInfo.birthdayMask = flag;
	newContact.options.phones.displayPhoneForList = 0;
	newContact.options.phones.phone1 = P1ContactsworkLabel;
	newContact.options.phones.phone2 = P1ContactshomeLabel;
	newContact.options.phones.phone3 = P1ContactsmobileLabel;
	newContact.options.phones.phone4 = P1ContactsemailLabel;
	newContact.options.phones.phone5 = P1ContactsmainLabel;
	newContact.options.phones.phone6 = P1ContactspagerLabel;
	newContact.options.phones.phone7 = P1ContactsotherLabel;
	
	newContact.options.chatIds.reserved = 0;
	newContact.options.chatIds.chat1 = aimChatLabel;
	newContact.options.chatIds.chat2 = msnChatLabel;
	
	newContact.options.addresses.address1 = P1ContactsworkAddressLabel;
	newContact.options.addresses.address2 = P1ContactshomeAddressLabel;
	newContact.options.addresses.address3 = P1ContactsotherAddressLabel;
	
	newContact.numBlobs = 0;
	newContact.pictureInfo.pictureSize = 0;
	newContact.pictureInfo.pictureData = 0;
	
	newContact.birthdayInfo.birthdayDate.month = 0;
	newContact.birthdayInfo.birthdayDate.day = 0;
	newContact.birthdayInfo.birthdayDate.year = 0;
	
	for (i = P1ContactsfirstAddressField; i < P1Contactsnote + 1; i++)
	{
		newContact.fields[i] = NULL;
	}
	err = P1ContactsDBNewRecord(globals->adxtLibRef, globals->AddrDB, &newContact, &globals->CurrentRecord);
#else
	UInt32 romVersion;
	FtrGet(sysFtrCreator,	sysFtrNumROMVersion, &romVersion);
	//check PalmOS version
	if(romVersion<0x05000000)
	{
		newRecord.options.phones.displayPhoneForList = 0;
		newRecord.options.phones.phone1 = workLabel;
		newRecord.options.phones.phone2 = homeLabel;
		newRecord.options.phones.phone3 = faxLabel;
		newRecord.options.phones.phone4 = otherLabel;
		newRecord.options.phones.phone5 = emailLabel;
	}
	else
	{
		newRecord.options.phones.displayPhoneForList = 0;
		newRecord.options.phones.phone1 = workLabel;
		newRecord.options.phones.phone2 = homeLabel;
		newRecord.options.phones.phone3 = mobileLabel;
		newRecord.options.phones.phone4 = emailLabel;
		newRecord.options.phones.phone5 = otherLabel;
	}
	for (i = firstAddressField; i < addressFieldsCount; i++)
	{
		newRecord.fields[i] = NULL;
	}
	err = AddrDBNewRecord(globals->adxtLibRef, globals->AddrDB, &newRecord, &globals->CurrentRecord);
#endif
	
	if (err)
	{
		FrmAlert(DeviceFullAlert);
		return;
	}

	// Set it's category to the category being viewed.
	// If the category is All then set the category to unfiled.
	
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, &attr, NULL, NULL);
	
	attr &= ~dmRecAttrCategoryMask;
	attr |= ((globals->CurrentCategory == dmAllCategories) ? dmUnfiledCategory :
			 globals->CurrentCategory) | dmRecAttrDirty;
	DmSetRecordInfo (globals->AddrDB, globals->CurrentRecord, &attr, NULL);
	
		
	
	// Set the global variable that determines which field is the top visible
	// field in the edit view.  Also done when New is pressed.
	
	globals->TopVisibleFieldIndex = 0;
	globals->CurrentFieldIndex = editFirstFieldIndex;
	globals->EditRowIDWhichHadFocus = editFirstFieldIndex;
	globals->EditFieldPosition = 0;
	
	FrmGotoForm (EditView);
}

static void EditDrawRecord(MemPtr tTable, Int16 tRow, Int16 tColumn, RectangleType *tBounds)
{
	globalVars* globals = getGlobalsPtr();
	RectangleType bounds;
	P1ContactsDBRecordType record;
	MemHandle recordH;
	TablePtr tableP = CustomGetObjectPtrSmp(EditTable);
	UInt16 currentField = globals->FieldMap[TblGetRowID(tableP, tRow)];
	UInt16 id = TblGetRowID(tableP, tRow);
	currentField = id;
	currentField = globals->FieldMap[TblGetRowID(tableP, tRow)];
	if(currentField == P1ContactsbirthdayDate)
	{
		MemMove(&bounds, tBounds, sizeof(RectangleType));
		bounds.topLeft.y += 4;
		bounds.extent.y = 12;
		bounds.extent.x = 62;
	
		FrmSetObjectBounds(FrmGetActiveForm(), GetObjectIndexSmp(EditBirthdaySelector), &bounds);
				
		FrmShowObjectSmp(EditBirthdaySelector);
	}
	else if(currentField == P1ContactsanniversaryDate)
	{
		MemMove(&bounds, tBounds, sizeof(RectangleType));
		bounds.topLeft.y += 4;
		bounds.extent.y = 12;
		bounds.extent.x = 62;
	
		FrmSetObjectBounds(FrmGetActiveForm(), GetObjectIndexSmp(EditAnnivSelector), &bounds);
				
		FrmShowObjectSmp(EditAnnivSelector);
	}
	else if(currentField == P1ContactspictureInfo)
	{
	}
	else if(currentField == P1Contactsringer)
	{
		MemMove(&bounds, tBounds, sizeof(RectangleType));
		bounds.topLeft.y += 4;
		bounds.extent.y = 10;
		bounds.extent.x = 62;
		
		FrmSetObjectBounds(FrmGetActiveForm(), GetObjectIndexSmp(EditRingerTrigger), &bounds);
		bounds.extent.y = 500;
		FrmSetObjectBounds(FrmGetActiveForm(), GetObjectIndexSmp(EditRingtoneList), &bounds);
		FillRingerList();		
		FrmShowObjectSmp(EditRingerTrigger);
	}
}

/***********************************************************************
 *
 * FUNCTION:    PrvEditInit
 *
 * DESCRIPTION: This routine initializes the "Edit View" of the
 *              Address application.
 *
 * PARAMETERS:	frmP					Pointer to the Edit form structure
 *					leaveDataLocked	T=>keep app info, form map data locked.
 *
 * RETURNED:	nothing
 *
 *	HISTORY:
 *		06/05/99	art	Created by Art Lamb.
 *		07/29/99	kwk	Set up locked FieldMap pointer.
 *		09/21/00	aro	GetObjectPtr => GetFrmObjectPtr
 *
 ***********************************************************************/
void PrvEditInit( FormPtr frmP, Boolean leaveDataLocked )
{
	globalVars* globals = getGlobalsPtr();
	UInt16 attr;
	UInt16 row, i;
	UInt16 rowsInTable;
	UInt16 category;
	UInt16 dataColumnWidth;
	TablePtr table;
	univAppInfoPtr appInfoPtr;
	ListPtr popupPhoneList;
#ifdef CONTACTS
	ListPtr popupIMList, popupAddrList;
#endif
	FontID   currFont;
	RectangleType bounds, r1;
	Int16 width, height;
	
#ifdef DEBUG
	LogWrite("xt_log", "edit", "PrvEditInit");
#endif

	// Set the label of the category trigger.
	if (globals->CurrentCategory == dmAllCategories)
	{
		DmRecordInfo (globals->AddrDB, globals->CurrentRecord, &attr, NULL, NULL);
		category = attr & dmRecAttrCategoryMask;
	}
	else
		category = globals->CurrentCategory;
	CategoryGetName (globals->AddrDB, category, globals->CategoryName);
	CategorySetTriggerLabel(CustomGetObjectPtrSmp(EditCategoryTrigger), globals->CategoryName);
	
	appInfoPtr = univAddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	
	globals->FieldMapH = DmGetResource(fieldMapRscType, FieldMapID);	
#ifdef CONTACTS
	row = 0;
	for(i = 0; i < 40; i++)
	{
		if(!NewContactFields() && (FieldMapIndexes[i] == P1Contactsringer || FieldMapIndexes[i] == P1ContactsanniversaryDate))
			continue;
		if(FieldMapIndexes[i] == P1ContactspictureInfo)
			continue;
		globals->FieldMap[row++] = FieldMapIndexes[i];
	}
#else
	globals->FieldMap = (AddressFields*)MemHandleLock(globals->FieldMapH);
#endif
	// If get rid of it, remove extra FrmDrawForm from EditOnOpen()
	width=GetWindowWidth();
	height=GetWindowHeight();
//#ifdef DEBUG
//	LogWriteNum("xt_log", "edit", width, "width");
//	LogWriteNum("xt_log", "edit", height, "height");
//#endif
	
	globals->CurrentTableRow = editInvalidRow;

	// Set the choices to the phone list
#ifdef CONTACTS
	StrCopy(globals->ringerLabel, "Ringtone");
	StrCopy(globals->annivLabel, "Anniversary");
	globals->EditPhoneListChoices[0] = appInfoPtr->fieldLabels[P1ContactsfirstPhoneField];
	globals->EditPhoneListChoices[1] = appInfoPtr->fieldLabels[P1ContactsfirstPhoneField + 1];
	globals->EditPhoneListChoices[2] = appInfoPtr->fieldLabels[P1ContactsfirstPhoneField + 2];
	globals->EditPhoneListChoices[3] = appInfoPtr->fieldLabels[P1ContactsfirstPhoneField + 3];
	globals->EditPhoneListChoices[4] = appInfoPtr->fieldLabels[P1ContactsfirstPhoneField + 4];
	globals->EditPhoneListChoices[5] = appInfoPtr->fieldLabels[P1ContactsfirstPhoneField + 5];
	globals->EditPhoneListChoices[6] = appInfoPtr->fieldLabels[P1ContactsfirstPhoneField + 6];
	globals->EditPhoneListChoices[7] = appInfoPtr->fieldLabels[P1ContactsfirstPhoneField + 36];
	
	popupPhoneList = CustomGetObjectPtrSmp(EditPhoneList);
	LstSetListChoices(popupPhoneList, globals->EditPhoneListChoices, numPhoneLabels);
	LstSetHeight (popupPhoneList, numPhoneLabels);
	
	popupIMList = CustomGetObjectPtrSmp(EditIMList);
	globals->EditIMChoices[0] = appInfoPtr->fieldLabels[42];
	globals->EditIMChoices[1] = appInfoPtr->fieldLabels[43];
	globals->EditIMChoices[2] = appInfoPtr->fieldLabels[44];
	globals->EditIMChoices[3] = appInfoPtr->fieldLabels[45];
	globals->EditIMChoices[4] = appInfoPtr->fieldLabels[41];
	LstSetListChoices(popupIMList, globals->EditIMChoices, numChatLabels);
	LstSetHeight (popupIMList, numChatLabels);
		
	
	popupAddrList = CustomGetObjectPtrSmp(EditAddressList);
	globals->EditAddrChoices[0] = appInfoPtr->fieldLabels[23];
	globals->EditAddrChoices[1] = appInfoPtr->fieldLabels[28];
	globals->EditAddrChoices[2] = appInfoPtr->fieldLabels[33];
	LstSetListChoices(popupAddrList,globals-> EditAddrChoices, numAddressLabels);
	LstSetHeight (popupAddrList, numAddressLabels);
#else
	globals->EditPhoneListChoices[0] = appInfoPtr->fieldLabels[firstPhoneField];
	globals->EditPhoneListChoices[1] = appInfoPtr->fieldLabels[firstPhoneField + 1];
	globals->EditPhoneListChoices[2] = appInfoPtr->fieldLabels[firstPhoneField + 2];
	globals->EditPhoneListChoices[3] = appInfoPtr->fieldLabels[firstPhoneField + 3];
	globals->EditPhoneListChoices[4] = appInfoPtr->fieldLabels[firstPhoneField + 4];
	globals->EditPhoneListChoices[5] = appInfoPtr->fieldLabels[addressFieldsCount];
	globals->EditPhoneListChoices[6] = appInfoPtr->fieldLabels[addressFieldsCount + 1];
	globals->EditPhoneListChoices[7] = appInfoPtr->fieldLabels[addressFieldsCount + 2];
	popupPhoneList = CustomGetObjectPtrSmp(EditPhoneList);
	LstSetListChoices(popupPhoneList, globals->EditPhoneListChoices, numPhoneLabels);
	LstSetHeight (popupPhoneList, numPhoneLabels);
#endif

	// Initialize the address list table.
	table = CustomGetObjectPtrSmp(EditTable);
	TblSetCustomDrawProcedure(table, editDataColumn, EditDrawRecord);
	if(width>220)
	{
		RctSetRectangle(&r1, 0, 18, width, 121);
		TblSetBounds(table, &r1);
		TblSetColumnWidth(table, editLabelColumn, 55);
		TblSetColumnWidth(table, editDataColumn, width-57);
	}
	else if(height>220)
	{
		// HiRes+, usable 160x224
		RctSetRectangle(&r1, 0, 18, 160, 187);
		TblSetBounds(table, &r1);
		TblSetColumnWidth(table, editLabelColumn, 55);
		TblSetColumnWidth(table, editDataColumn, 103);
	
	}
	else if(height == 200)
	{
		// G18, usable 160x200
		RctSetRectangle(&r1, 0, 18, width, 165);
		TblSetBounds(table, &r1);
		TblSetColumnWidth(table, editLabelColumn, 55);
		TblSetColumnWidth(table, editDataColumn, 103);		
	}
	else // if(width==160 && height==160)
	{
		RctSetRectangle(&r1, 0, 18, width, 121);
		TblSetBounds(table, &r1);
		TblSetColumnWidth(table, editLabelColumn, 55);
		TblSetColumnWidth(table, editDataColumn, 103);
	}

	
	rowsInTable = TblGetNumberOfRows (table);
#ifdef DEBUG
	LogWriteNum("xt_log", "edit", rowsInTable, "rows");
#endif

	/*currFont = FntSetFont (globals->AddrEditFont);
	for (row = 0; row < rowsInTable; row++)
	{
		TblSetRowHeight(table, row, 20);//FntLineHeight());
	}
	FntSetFont (currFont);*/
	
	rowsInTable = TblGetNumberOfRows (table);

	for (row = 0; row < rowsInTable; row++)
	{
		// This sets the data column
		//TblSetItemStyle (table, row, editDataColumn, textTableItem);
		TblSetRowUsable (table, row, false);
	}
	
	
	TblSetColumnUsable (table, editLabelColumn, true);
	TblSetColumnUsable (table, editDataColumn, true);

	TblSetColumnSpacing (table, editLabelColumn, spaceBeforeDesc);


	// Set the callback routines that will load and save the
	// description field.
	TblSetLoadDataProcedure (table, editDataColumn, PrvEditGetRecordField);
	TblSetSaveDataProcedure (table, editDataColumn, PrvEditSaveRecordField);


	// Set the column widths so that the label column contents fit exactly.
	// Those labels change as the country changes.
	if (globals->EditLabelColumnWidth == 0)
	{
		globals->EditLabelColumnWidth = ToolsGetLabelColumnWidth (globals->adxtLibRef, appInfoPtr, stdFont);		
	}
	// Compute the width of the data column, account for the table column gutter.
	TblGetBounds (table, &bounds);
	dataColumnWidth = bounds.extent.x - spaceBeforeDesc - globals->EditLabelColumnWidth;

	if(TblGetColumnWidth(table, editLabelColumn)< globals->EditLabelColumnWidth)
	{
		TblSetColumnWidth(table, editLabelColumn, globals->EditLabelColumnWidth);
		TblSetColumnWidth(table, editDataColumn, dataColumnWidth);
	}

	PrvEditLoadTable(frmP);


	// if the caller is using us to reset the form, then we don't want
	// to repeatedly lock down the app info block.
	if (!leaveDataLocked)
	{
		MemPtrUnlock(appInfoPtr);
#ifndef CONTACTS
		MemHandleUnlock(globals->FieldMapH);
		DmReleaseResource(globals->FieldMapH);
#endif
	}

	// In general, the record isn't needed after this form is closed.
	// It is if the user is going to the Note View.  In that case
	// we must keep the record.
	globals->RecordNeededAfterEditView = false;

#ifdef DEBUG
	LogWrite("xt_log", "edit", "PrvEditInit exit");
#endif
}

void PrvEditInitTableRow( FormPtr frmP, TablePtr table, UInt16 row, UInt16 fieldIndex, FontID fontID, void* record, void* appInfoPtr )
{
	globalVars* globals = getGlobalsPtr();
	if(row >= TblGetNumberOfRows(table))
		return;
	
	// Make the row usable.
	TblSetRowUsable (table, row, true);

	// Set the height of the row to the height of the desc
	
	// Store the record number as the row id.
	TblSetRowID (table, row, fieldIndex);

	// Mark the row invalid so that it will draw when we call the
	// draw routine.
	TblMarkRowInvalid (table, row);

	// The label is either a text label or a popup menu (of phones)
#ifdef CONTACTS
	if(globals->FieldMap[fieldIndex] == P1ContactsbirthdayDate)
	{
		TblSetItemStyle (table, row, editDataColumn, tallCustomTableItem);
		TblSetRowSelectable(table, row, false);		
		{
			P1ContactsDBRecordType record;
			MemHandle recordH;
			PrvP1ContactsDBGetRecordBDOnly (globals->AddrDB, globals->CurrentRecord, &record, &recordH);	
			// Get the height of the table and the width of the description
			// column.
			if(record.birthdayInfo.birthdayDate.day != 0 || record.birthdayInfo.birthdayDate.month != 0)
			{
				DateFormatType dateFormat;
				dateFormat=PrefGetPreference(prefLongDateFormat);
				DateToAscii(record.birthdayInfo.birthdayDate.month, record.birthdayInfo.birthdayDate.day, record.birthdayInfo.birthdayDate.year+1904, dateFormat, globals->gDateTxt);
			}
			else
			{
				StrCopy(globals->gDateTxt, " -Tap to add- ");
			}	
			CustomSetCtlLabelPtrSmp(EditBirthdaySelector, globals->gDateTxt);	
			MemHandleUnlock (recordH);
		}
	}
	else if(globals->FieldMap[fieldIndex] == P1ContactspictureInfo)
	{
		TblSetItemStyle (table, row, editDataColumn, tallCustomTableItem);
		TblSetRowSelectable(table, row, false);		
		
	}
	else
	{
		TblSetItemFont (table, row, editDataColumn, fontID);
		TblSetItemStyle (table, row, editDataColumn, textTableItem);
		TblSetRowSelectable(table, row, true);
	}
	if(P1ContactsisChatField(globals->FieldMap[fieldIndex]))
	{
		UInt16 chatLabel = GetChatLabel((P1ContactsDBRecordPtr)record, globals->FieldMap[fieldIndex]) - 1;
		if(chatLabel == -1)
			chatLabel = 4;
		
		TblSetItemStyle (table, row, editLabelColumn, popupTriggerTableItem);
		TblSetItemInt (table, row, editLabelColumn, chatLabel);
		TblSetItemPtr (table, row, editLabelColumn, CustomGetObjectPtr(frmP, EditIMList));		
	}
	else if(P1ContactsisAddressField(globals->FieldMap[fieldIndex]))
	{
		TblSetItemStyle (table, row, editLabelColumn, popupTriggerTableItem);
		TblSetItemInt (table, row, editLabelColumn, GetAddressLabel((P1ContactsDBRecordPtr)record, globals->FieldMap[fieldIndex]));
		TblSetItemPtr (table, row, editLabelColumn, CustomGetObjectPtr(frmP, EditAddressList));				
	}
	else if(P1ContactsisPhoneField(globals->FieldMap[fieldIndex]))
	{
		TblSetItemStyle (table, row, editLabelColumn, popupTriggerTableItem);
		TblSetItemInt (table, row, editLabelColumn, P1ContactsGetPhoneLabel((P1ContactsDBRecordPtr)record, globals->FieldMap[fieldIndex]));
		TblSetItemPtr (table, row, editLabelColumn, CustomGetObjectPtr(frmP, EditPhoneList));				
	}
	else if(globals->FieldMap[fieldIndex] == P1ContactspictureInfo)
	{
		TblSetItemStyle (table, row, editLabelColumn, labelTableItem);
		TblSetItemPtr (table, row, editLabelColumn,
					   ((P1ContactsAppInfoPtr)appInfoPtr)->fieldLabels[46]);
	}
	else if(globals->FieldMap[fieldIndex] == P1Contactsringer)
	{
		TblSetItemStyle (table, row, editLabelColumn, labelTableItem);
		TblSetItemStyle (table, row, editDataColumn, tallCustomTableItem);
		TblSetRowSelectable(table, row, false);		
		TblSetItemPtr (table, row, editLabelColumn,
					   globals->ringerLabel);
	}
	else
	{
		TblSetItemStyle (table, row, editLabelColumn, labelTableItem);
		TblSetItemPtr (table, row, editLabelColumn,
					   ((P1ContactsAppInfoPtr)appInfoPtr)->fieldLabels[globals->FieldMap[fieldIndex]]);
	}
#else
	if (! isPhoneField(globals->FieldMap[fieldIndex]))
	{
		TblSetItemStyle (table, row, editLabelColumn, labelTableItem);
		TblSetItemPtr (table, row, editLabelColumn,
					   ((AddrAppInfoPtr)appInfoPtr)->fieldLabels[globals->FieldMap[fieldIndex]]);
	}
	else
	{
		// The label is a popup list
		TblSetItemStyle (table, row, editLabelColumn, popupTriggerTableItem);
		TblSetItemInt (table, row, editLabelColumn, GetPhoneLabel((AddrDBRecordPtr)record, globals->FieldMap[fieldIndex]));
		TblSetItemPtr (table, row, editLabelColumn, CustomGetObjectPtr(frmP, EditPhoneList));
	}
#endif
}


/***********************************************************************
 *
 * FUNCTION:    PrvEditHandleSelectField
 *
 * DESCRIPTION: Handle the user tapping an edit view field label.
 *   Either the a phone label is changed or the user wants to edit
 * a field by tapping on it's label.
 *
 * PARAMETERS:  row    - row of the item to select (zero based)
 *              column - column of the item to select (zero based)
 *
 * RETURNED:    true if the event was handled and nothing else should
 *              be done
 *
 ***********************************************************************/
Boolean PrvEditHandleSelectField (Int16 row, Int16 column)
{
	globalVars* globals = getGlobalsPtr();
	Err					err;
	Int16				currRow;
	UInt16				fieldNum;
	UInt16				fieldIndex;
	MemHandle			currentRecordH;
	UInt16				i;
	UInt16				currentField;
	FontID				currFont;
	Boolean				redraw = false;
	FormType*			frmP;
	TablePtr			tableP;
	FieldPtr			fldP;
	univAddrDBRecordType	currentRecord;
	univAddrDBRecordFlags	changedFields;

	UInt16				startPosition;
	UInt16				stopPosition;
	
	frmP = FrmGetActiveForm();
	tableP = CustomGetObjectPtrSmp(EditTable);
	fldP = NULL;
	
	currentField = globals->FieldMap[TblGetRowID(tableP, row)];
	
	if(currentField == P1ContactsbirthdayDate)
	{
		FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, EditBirthdaySelector);
	}
	if (column == editLabelColumn)
	{
#ifdef CONTACTS
		if (P1ContactsisPhoneField(currentField) || P1ContactsisChatField(currentField) || P1ContactsisAddressField(currentField))
		{
			i = TblGetItemInt(tableP, row, editLabelColumn);
			PrvP1ContactsDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &currentRecord, &currentRecordH);

			switch (currentField)
			{
				case P1ContactsfirstPhoneField:
					currentRecord.options.phones.phone1 = i;
					break;

				case P1ContactsfirstPhoneField + 1:
					currentRecord.options.phones.phone2 = i;
					break;

				case P1ContactsfirstPhoneField + 2:
					currentRecord.options.phones.phone3 = i;
					break;

				case P1ContactsfirstPhoneField + 3:
					currentRecord.options.phones.phone4 = i;
					break;

				case P1ContactsfirstPhoneField + 4:
					currentRecord.options.phones.phone5 = i;
					break;

				case P1ContactsfirstPhoneField + 5:
					currentRecord.options.phones.phone6 = i;
					break;

				case P1ContactsfirstPhoneField + 6:
					currentRecord.options.phones.phone7 = i;
					break;

				case P1Contactsaddress:
					currentRecord.options.addresses.address1 = i;
					break;

				case P1Contactsaddress2:
					currentRecord.options.addresses.address2 = i;
					break;

				case P1Contactsaddress3:
					currentRecord.options.addresses.address3 = i;
					break;

				case P1Contactschat1:
					currentRecord.options.chatIds.reserved = 0;
					if(i < 4)
						currentRecord.options.chatIds.chat1 = i+1;
					else
						currentRecord.options.chatIds.chat1 = 0;
					break;

				case P1Contactschat2:
					currentRecord.options.chatIds.reserved = 0;
					if(i < 4)
						currentRecord.options.chatIds.chat2 = i+1;
					else
						currentRecord.options.chatIds.chat2 = 0;
					break;
			}

			changedFields.allBits = 0;
			err = PrvP1ContactsDBChangeRecord(globals->adxtLibRef, globals->AddrDB, &globals->CurrentRecord, &currentRecord,
								   changedFields, false);
#else
		if (isPhoneField(currentField))
		{
			i = TblGetItemInt(tableP, row, editLabelColumn);
			AddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &currentRecord, &currentRecordH);

			switch (currentField)
			{
				case firstPhoneField:
					currentRecord.options.phones.phone1 = i;
					break;
	
				case firstPhoneField + 1:
					currentRecord.options.phones.phone2 = i;
					break;
	
				case firstPhoneField + 2:
					currentRecord.options.phones.phone3 = i;
					break;
	
				case firstPhoneField + 3:
					currentRecord.options.phones.phone4 = i;
					break;
	
				case firstPhoneField + 4:
					currentRecord.options.phones.phone5 = i;
					break;
			}

			changedFields.allBits = 0;
			err = AddrDBChangeRecord(globals->AddrDB, &globals->CurrentRecord, &currentRecord,
								   changedFields);
#endif

			if ( err != errNone )
			{
				MemHandleUnlock(currentRecordH);
				FrmAlert(DeviceFullAlert);
				// Redraw the table without the change.  The phone label
				// is unchanged in the record but the screen and the table row
				// are changed.  Reinit the table to fix it.  Mark the row
				// invalid and redraw it.
				PrvEditInit(frmP, false);
				TblMarkRowInvalid(tableP, row);
				TblRedrawTable(tableP);

				return true;
			}
		}

		// The user selected the label of a field.  So, set the table to edit the field to
		// the right of the label.  Also, mark the row invalid and say that we want to redraw
		// it so all the colors and such come out right.
		TblReleaseFocus(tableP);
		TblUnhighlightSelection(tableP);
		TblMarkRowInvalid (tableP, row);
		redraw = true;
	}

	// Make sure the the heights the the field we are exiting and the
	// that we are entering are correct.  They may be incorrect if the
	// font used to display blank line is a different height then the
	// font used to display field text.
	fieldIndex = TblGetRowID (tableP, row);
	fieldNum = globals->FieldMap[fieldIndex];
	//if (fieldIndex != P1ContactsbirthdayDate)
	//{
		globals->CurrentFieldIndex = fieldIndex;
	//}	
	/*if (fieldIndex != P1ContactsbirthdayDate && (fieldIndex != globals->CurrentFieldIndex || TblGetCurrentField(tableP) == NULL))
	{
		
		currFont = FntGetFont ();

		univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &currentRecord, &currentRecordH);
		if (globals->CurrentFieldIndex != noFieldIndex &&
			!currentRecord.fields[globals->FieldMap[globals->CurrentFieldIndex]])
		{
			if (TblFindRowID (tableP, globals->CurrentFieldIndex, &currRow))
			{
				// Is the height of the field correct?
				FntSetFont (addrEditBlankFont);
				if (FntLineHeight () != TblGetRowHeight (tableP, currRow))
				{
					TblMarkRowInvalid (tableP, currRow);
					redraw = true;
				}
			}
		}

		globals->CurrentFieldIndex = fieldIndex;

		
		fieldNum = globals->FieldMap[fieldIndex];
		if (!currentRecord.fields[fieldNum])
		{
			// Is the height of the field correct?
			FntSetFont (globals->AddrEditFont);
			if (FntLineHeight () != TblGetRowHeight (tableP, row))
			{
				TblMarkRowInvalid (tableP, row);
				redraw = true;
			}
		}
		// Do before the table focus is released and the record is saved.
		MemHandleUnlock (currentRecordH);
		
		if (redraw)
		{
			fldP = TblGetCurrentField(tableP);
			if ( fldP != NULL )
				FldGetSelection(fldP, &startPosition, &stopPosition);	// Save the selection of the field

			TblReleaseFocus (tableP);
			PrvEditLoadTable(frmP);
			TblFindRowID (tableP, fieldIndex, &row);

			TblRedrawTable (tableP);
		}

		FntSetFont (currFont);
	}*/

	// Set the focus on the field if necessary
	if ( fieldNum != P1ContactsbirthdayDate && (TblGetCurrentField(tableP) == NULL && row >=0 && row < TblGetNumberOfRows(tableP)) )
	{
		FieldPtr fldTempP;
		
		FrmSetFocus(frmP, FrmGetObjectIndex(frmP, EditTable));
		TblGrabFocus (tableP, row, editDataColumn);
		fldTempP = TblGetCurrentField(tableP);
		FldGrabFocus(fldTempP);
		FldMakeFullyVisible (fldTempP);
		if(globals->gDeviceFlags.bits.treo)
		{
			PrvEditSetGraffitiMode (fldTempP, TblGetRowID(tableP, row));
		}
	}

	// Restore the selection of the field or restore the insertion point
	if ( fieldIndex != P1ContactsbirthdayDate && redraw && (fldP != NULL) )
	{
		if ( startPosition != stopPosition )
			FldSetSelection(fldP, startPosition, stopPosition);
		else
			FldSetInsPtPosition(fldP, globals->EditFieldPosition);
	}
		
	return false;
}

void PrvEditRestoreEditState( FormType* frmP )
{
	globalVars* globals = getGlobalsPtr();
	Int16			row;
	TablePtr		table;
	FieldPtr		fld;

	if (globals->CurrentFieldIndex == noFieldIndex) return;

	// Find the row that the current field is in.
	table = CustomGetObjectPtrSmp(EditTable);
	if ( ! TblFindRowID (table, globals->CurrentFieldIndex, &row) )
		return;

	FrmSetFocus(frmP, FrmGetObjectIndex(frmP, EditTable));
	TblReleaseFocus(table);
	TblGrabFocus (table, row, editDataColumn);
	
	if(globals->gNavigation)
	{
		FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, EditTable);
	}
	
	// Restore the insertion point position.
	fld = TblGetCurrentField (table);
	FldSetInsPtPosition (fld, globals->EditFieldPosition);
	FldGrabFocus (fld);
}

Boolean PrvEditSetGraffitiMode (FieldPtr fld, UInt16 currentField)
{
	globalVars* globals = getGlobalsPtr();
	MemHandle currentRecordH;
	Boolean autoShift;
	Boolean isNumericField = false;
	FieldAttrType attr;
	univAddrDBRecordType currentRecord;
	univAddressFields addressField;

	autoShift = true;
	
	addressField = globals->FieldMap[currentField];	
	univAddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &currentRecord, &currentRecordH);
#ifdef CONTACTS
	if(addressField == P1Contactswebpage)
	{
		autoShift = false;
	}	
	else if(P1ContactsisPhoneField(addressField))
	{
		isNumericField = true;
		switch(addressField)
		{
			case P1Contactsphone1:
				autoShift = false;
				if(currentRecord.options.phones.phone1 == P1ContactsemailLabel)
					isNumericField = false;
				break;
			case P1Contactsphone2:
				autoShift = false;
				if(currentRecord.options.phones.phone2 == P1ContactsemailLabel)
					isNumericField = false;
				break;
			case P1Contactsphone3:
				autoShift = false;
				if(currentRecord.options.phones.phone3 == P1ContactsemailLabel)
					isNumericField = false;
				break;
			case P1Contactsphone4:
				autoShift = false;
				if(currentRecord.options.phones.phone4 == P1ContactsemailLabel)
					isNumericField = false;
				break;
			case P1Contactsphone5:
				autoShift = false;
				if(currentRecord.options.phones.phone5 == P1ContactsemailLabel)
					isNumericField = false;
				break;
			case P1Contactsphone6:
				autoShift = false;
				if(currentRecord.options.phones.phone6 == P1ContactsemailLabel)
					isNumericField = false;
				break;
			case P1Contactsphone7:
				autoShift = false;
				if(currentRecord.options.phones.phone7 == P1ContactsemailLabel)
					isNumericField = false;
				break;
		}		
	}
	else
		isNumericField = (addressField == P1ContactszipCode) || (addressField == P1ContactszipCode2) || (addressField == P1ContactszipCode3);
#else
	if(isPhoneField(addressField))
	{
		isNumericField = true;
		switch(addressField)
		{
			case phone1:
				autoShift = false;
				if(currentRecord.options.phones.phone1 == emailLabel)
					isNumericField = false;
				break;
			case phone2:
				autoShift = false;
				if(currentRecord.options.phones.phone2 == emailLabel)
					isNumericField = false;
				break;
			case phone3:
				autoShift = false;
				if(currentRecord.options.phones.phone3 == emailLabel)
					isNumericField = false;
				break;
			case phone4:
				autoShift = false;
				if(currentRecord.options.phones.phone4 == emailLabel)
					isNumericField = false;
				break;
			case phone5:
				autoShift = false;
				if(currentRecord.options.phones.phone5 == emailLabel)
					isNumericField = false;
				break;
		}					
	}
	else
		isNumericField = isPhoneField(addressField) || (addressField == zipCode);
#endif
	
	if (isNumericField)
	{
		// Set the field to support auto-shift.
		GrfSetState(false, true, false);
		autoShift = false;
		if(globals->gDeviceFlags.bits.treo)
		{
			HsGrfSetStateExt(false, true, true, false, false, false);
		}
	}
	else
	{
		if(globals->gDeviceFlags.bits.treo)
		{
			HsGrfSetStateExt(false, false, false, autoShift, false, autoShift);
		}
	}
	
	if (fld)
	{
		FldGetAttributes (fld, &attr);
		attr.autoShift = autoShift;
		FldSetAttributes (fld, &attr);
	}

	
	MemHandleUnlock(currentRecordH);
	
	if(globals->gDeviceFlags.bits.treo && isNumericField)
		return true;
	else
		return false;
}


/***********************************************************************
 *
 * FUNCTION:    PrvEditGetRecordField
 *
 * DESCRIPTION: This routine returns a pointer to a field of the
 *              address record.  This routine is called by the table
 *              object as a callback routine when it wants to display or
 *              edit a field.
 *
 * PARAMETERS:  table  - pointer to the memo list table (TablePtr)
 *              row    - row of the table to draw
 *              column - column of the table to draw
 *
 * RETURNED:    nothing
 ***********************************************************************/
Err PrvEditGetRecordField (void * table, Int16 row, Int16 UNUSED_PARAM(column), Boolean editing, MemHandle * textH, Int16 * textOffset, Int16 * textAllocSize, FieldPtr fld)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 fieldNum = 0;
	UInt16  fieldIndex;
	Char * recordP;
	Char * fieldP;
	MemHandle recordH, fieldH;
	UInt16 fieldSize;
	univAddrDBRecordType record;
	
#ifdef DEBUG
	if(row == 0)
		LogWrite("xt_log", "edit", "EditGetRec #0");
#endif
	
	// Get the field number that corresponds to the table item.
	// The field number is stored as the row id.
	//
	fieldIndex = TblGetRowID (table, row);
	
	fieldNum = globals->FieldMap[fieldIndex];
	
	if(fieldNum == P1ContactsbirthdayDate)
		fieldNum--;//return 0;
	
	univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);

	if (editing)
	{
		PrvEditSetGraffitiMode(fld, TblGetRowID (table, row));
		
		if (record.fields[fieldNum])
		{
			fieldSize = StrLen(record.fields[fieldNum]) + 1;
			fieldH = MemHandleNew(fieldSize);	// Handle freeing done into PrvEditSaveRecordField() by calling FldFreeMemory() function
			fieldP = MemHandleLock(fieldH);
			MemMove(fieldP, record.fields[fieldNum], fieldSize);
			*textAllocSize = fieldSize;
			MemHandleUnlock(fieldH);
		}
		else
		{
			fieldH = 0;
			*textAllocSize = 0;
		}
		
		MemHandleUnlock (recordH);
		*textOffset = 0;         // only one string
		*textH = fieldH;
		return (0);

	}
	else
	{
		// Calculate the offset from the start of the record.
		recordP = MemHandleLock (recordH);   // record now locked twice

		if (record.fields[fieldNum])
		{
			*textOffset = record.fields[fieldNum] - recordP;
			*textAllocSize = StrLen (record.fields[fieldNum]) + 1;  // one for null terminator
		}
		else
		{
			do
			{
				fieldNum++;
			} while (fieldNum < P1ContactsbirthdayDate &&
					 record.fields[fieldNum] == NULL);

			if (fieldNum < P1ContactsbirthdayDate)
				*textOffset = record.fields[fieldNum] - recordP;
			else
				// Place the new field at the end of the text.
				*textOffset = MemHandleSize(recordH);
			*textAllocSize = 0;  // one for null terminator
		}

		MemHandleUnlock (recordH);   // unlock the second lock
	}
	MemHandleUnlock (recordH);      // unlock the AddrGetRecord lock

	*textH = recordH;
	return (0);
}


/***********************************************************************
 *
 * FUNCTION:    PrvEditSaveRecordField
 *
 * DESCRIPTION: This routine saves a field of an address to the
 *              database.  This routine is called by the table
 *              object, as a callback routine, when it wants to save
 *              an item.
 *
 * PARAMETERS:  table  - pointer to the memo list table (TablePtr)
 *              row    - row of the table to draw
 *              column - column of the table to draw
 *
 * RETURNED:    true if the table needs to be redrawn
 ***********************************************************************/
Boolean PrvEditSaveRecordField (void * table, Int16 row, Int16 UNUSED_PARAM(column))
{
	globalVars* globals = getGlobalsPtr();
	UInt16 fieldNum;
	UInt16 fieldIndex;
	FieldPtr fld;
	univAddrDBRecordType record;
	MemHandle recordH;
	MemHandle textH;
	Char * textP;
	univAddrDBRecordFlags bit;

	Err err;
	Boolean redraw = false;
	UInt16 numOfRows;
	Int16 newSize;
	

	fld = TblGetCurrentField (table);
	textH = FldGetTextHandle(fld);

	// Get the field number that corresponds to the table item to save.
	fieldIndex = TblGetRowID (table, row);
	fieldNum = globals->FieldMap[fieldIndex];
	
	// Save the field last edited.
	globals->EditRowIDWhichHadFocus = fieldIndex;

	// Save the cursor position of the field last edited.
	// Check if the top of the text is scroll off the top of the
	// field, if it is then redraw the field.
	if (FldGetScrollPosition (fld))
	{
		FldSetScrollPosition (fld, 0);
		globals->EditFieldPosition = 0;
	}
	else
		globals->EditFieldPosition = FldGetInsPtPosition (fld);

	// Make sure there any selection is removed since we will free
	// the text memory before the callee can remove the selection.
	FldSetSelection (fld, 0, 0);


	if (FldDirty (fld))
	{
		// Since the field is dirty, mark the record as dirty
		ToolsDirtyRecord (globals->CurrentRecord);

		// Get a pointer to the text of the field.
		if (textH == 0)
			textP = NULL;
		else
		{
			textP = MemHandleLock(textH);
			if (textP[0] == '\0')
				textP = NULL;
		}

		// If we have text, and saving an auto-fill field, save the data to the proper database
		if (textP) {
			UInt32	dbType;

			// Select the proper database for the field we are editing,
			// or skip if not an autofill enabled field
#ifdef CONTACTS
			switch (fieldNum) {
				case P1Contactstitle:		dbType = P1ContactstitleDBType; break;
				case P1Contactscompany:		dbType = P1ContactscompanyDBType; break;
				case P1Contactscity:		dbType = P1ContactscityDBType; break;
				case P1Contactsstate:		dbType = P1ContactsstateDBType; break;
				case P1Contactscountry:		dbType = P1ContactscountryDBType; break;
				case P1Contactscity2:		dbType = P1ContactscityDBType; break;
				case P1Contactsstate2:		dbType = P1ContactsstateDBType; break;
				case P1Contactscountry2:	dbType = P1ContactscountryDBType; break;
				case P1Contactscity3:		dbType = P1ContactscityDBType; break;
				case P1Contactsstate3:		dbType = P1ContactsstateDBType; break;
				case P1Contactscountry3:	dbType = P1ContactscountryDBType; break;
				default:					dbType = 0;
			}		
			if (dbType) AutoFillLookupSave(dbType, contactsFileCAddress, textP);
			
#else
			switch (fieldNum) {
				case title:		dbType = titleDBType; break;
				case company:	dbType = companyDBType; break;
				case city:		dbType = cityDBType; break;
				case state:		dbType = stateDBType; break;
				case country:	dbType = countryDBType; break;
				default:			dbType = 0;
			}			
			if (dbType) AutoFillLookupSave(dbType, sysFileCAddress, textP);
#endif
		}

		univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);
		record.fields[fieldNum] = textP;

		if (univIsPhoneField(fieldNum) &&
			record.fields[univFirstPhoneField + record.options.phones.displayPhoneForList] == NULL)
		{
			UInt16 i;
			for (i = univFirstPhoneField; i <= univLastPhoneField; i++)
			{
				if (record.fields[i] != NULL)
				{
					record.options.phones.displayPhoneForList = i - univFirstPhoneField;
					break;
				}
			}
		}


#ifdef CONTACTS
		bit.allBits = -1;
		bit.allBits2 = -1;
		err = PrvP1ContactsDBChangeRecord(globals->adxtLibRef, globals->AddrDB, &globals->CurrentRecord, &record, bit, false);		
#else

		bit.allBits = (UInt32)1 << fieldNum;
		err = AddrDBChangeRecord(globals->AddrDB, &globals->CurrentRecord, &record, bit);
#endif
			
		// The new field has been copied into the new record.  Unlock it.
		if (textP)
			MemPtrUnlock(textP);
		
		// The change was not made (probably storage out of memory)
		if (err)
		{
			// Because the storage is full the text in the text field differs
			// from the text in the record.  PrvEditGetFieldHeight uses
			// the text in the field (because it's being edited).
			// Make the text in the field the same as the text in the record.
			// Resizing should always be possible.
			MemHandleUnlock(recordH);      // Get original text
			univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);
			if (record.fields[fieldNum] == NULL)
				newSize = 1;
			else
				newSize = StrLen(record.fields[fieldNum]) + 1;

			// Have the field stop using the chunk to unlock it.  Otherwise the resize can't
			// move the chunk if more space is needed and no adjacent free space exists.
			FldSetTextHandle (fld, 0);
			if (!MemHandleResize(textH, newSize))
			{
				textP = MemHandleLock(textH);
				if (newSize > 1)
					StrCopy(textP, record.fields[fieldNum]);
				else
					textP[0] = '\0';
				MemPtrUnlock(textP);
			}
			else
			{
				ErrNonFatalDisplay("Resize failed.");
			}

			// Update the text field to use whatever text we have.
			FldSetTextHandle (fld, textH);

			MemHandleUnlock(recordH);
			FrmAlert(DeviceFullAlert);
			// The field may no longer be the same height.  This row and those
			// below may need to be recalced. Mark this row and those
			// below it not usable and reload the table.
			numOfRows = TblGetNumberOfRows(table);
			while (row < numOfRows)
			{
				TblSetRowUsable(table, row, false);
				row++;
			}
			PrvEditLoadTable(FrmGetActiveForm());
			redraw = true;                  // redraw the table showing change lost
		}

	}

	// Free the memory used for the field's text because the table suppresses it.
	FldFreeMemory (fld);
	
	if(MenuGetActiveMenu() != NULL)
	{
		if(!AddressDialable(globals->CurrentRecord, kDialListShowInListPhoneIndex) || !ToolsIsDialerPresent())
			MenuHideItem(EditRecordDialCmd);
		else
			MenuShowItem(EditRecordDialCmd);
	}

	return redraw;
}


/***********************************************************************
 *
 * FUNCTION:    PrvEditSaveRecord
 *
 * DESCRIPTION: Checks the record and saves it if it's OK
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    The view that should be switched to.
 ***********************************************************************/
UInt16 PrvEditSaveRecord ()
{
	globalVars* globals = getGlobalsPtr();
	MemHandle currentRecordH;
	univAddrDBRecordType currentRecord;
	TablePtr tableP;
	Boolean hasData;
	Boolean currChanged = true;
	UInt16 savedCurr;

	// Make sure the field being edited is saved
	tableP = CustomGetObjectPtrSmp(EditTable);

	TblReleaseFocus(tableP);


	// If this record is needed then leave.  This is a good time because
	// the data is saved and this is before the record could be deleted.
	if (globals->RecordNeededAfterEditView)
	{
		globals->ListViewSelectThisRecord = noRecord;
		return ListView;
	}


	// The record may have already been delete by the Delete menu command
	// or the details dialog.  If there isn't a globals->CurrentRecord assume the
	// record has been deleted.
	if (globals->CurrentRecord == noRecord)
	{
		globals->ListViewSelectThisRecord = noRecord;
		return ListView;
	}

	// If there is no data then then delete the record.
	// If there is data but no name data then demand some.

	univAddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &currentRecord, &currentRecordH);
	hasData = AddrDBRecordContainsData(&currentRecord);
	
	// Unlock before the DeleteRecord.   We can only rely on
	// NULL pointers from here on out.
	MemHandleUnlock(currentRecordH);


	// If none are the fields contained anything then
	// delete the field.
	if (!hasData)
	{
		ToolsDeleteRecord(false);   // uniq ID wasted?  Yes. We don't care.
		globals->CurrentRecord = noRecord;
		PrefsExtSave();
		return ListView;
	}
	else
	{
		if(globals->gEnabledRecent)
		{
			AddrDBAddToRecent(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord);
			PrefsExtSave();
		}
	}


	// The record's category may have been changed.  The globals->CurrentCategory
	// isn't supposed to change in this case.  Make sure the CurrentRecord
	// is still visible in this category or pick another one near it.
	savedCurr = globals->CurrentRecord;
	if (!ToolsSeekRecord(globals->adxtLibRef, &globals->CurrentRecord, 0, dmSeekBackward))
	{
		if (!ToolsSeekRecord(globals->adxtLibRef, &globals->CurrentRecord, 0, dmSeekForward))
		{
			globals->CurrentRecord = noRecord;
			currChanged = false;
		}
	}
	if(currChanged)
		globals->CurrentRecord = savedCurr;
	

	globals->ListViewSelectThisRecord = globals->CurrentRecord;
	//CurrentRecord=noRecord;
	
	return ListView;
}

void PrvEditSelectCategory (void)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 attr;
	FormType* frmP;
	UInt16 category;
	Boolean categoryEdited;

	// Process the category popup list.
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, &attr, NULL, NULL);
	category = attr & dmRecAttrCategoryMask;

	frmP = FrmGetActiveForm();
	categoryEdited = CategorySelect(globals->AddrDB, frmP, EditCategoryTrigger,
									 EditCategoryList, false, &category, globals->CategoryName, 1, categoryDefaultEditCategoryString);

	if (categoryEdited || (category != (attr & dmRecAttrCategoryMask)))
	{
		// Change the category of the record.
		DmRecordInfo (globals->AddrDB, globals->CurrentRecord, &attr, NULL, NULL);
		attr &= ~dmRecAttrCategoryMask;
		attr |= category | dmRecAttrDirty;
		DmSetRecordInfo(globals->AddrDB, globals->CurrentRecord, &attr, NULL);

		globals->CurrentCategory = category;
		globals->TopVisibleRecord = 0;	
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvEditUpdateScrollers
 *
 * DESCRIPTION: This routine draws or erases the edit view scroll arrow
 *              buttons.
 *
 * PARAMETERS:  frmP             -  pointer to the address edit form
 *              bottomField     -  field index of the last visible row
 *              lastItemClipped - true if the last visible row is clip at
 *                                 the bottom
 ***********************************************************************/
void PrvEditUpdateScrollers (FormPtr frmP, UInt16 bottomFieldIndex,
							 Boolean lastItemClipped)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 upIndex;
	UInt16 downIndex;
	Boolean scrollableUp;
	Boolean scrollableDown;

	UInt16 lastEdit = globals->editLastFieldIndex;

#ifdef DEBUG
	LogWrite("xt_log", "edit", "PrvEditUpdateScrollers");
#endif

	
	// If the first field displayed is not the fist field in the record,
	// enable the up scroller.
	scrollableUp = globals->TopVisibleFieldIndex > 0;

	// If the last field displayed is not the last field in the record,
	// enable the down scroller.
	scrollableDown = (lastItemClipped || (bottomFieldIndex < lastEdit));
	
	
	// Update the scroll button.
	upIndex = FrmGetObjectIndex (frmP, EditUpButton);
	downIndex = FrmGetObjectIndex (frmP, EditDownButton);
	FrmUpdateScrollers (frmP, upIndex, downIndex, scrollableUp, scrollableDown);
}

UInt16 PrvEditGetFieldHeight (TablePtr table, UInt16 fieldIndex, Int16 columnWidth, Int16 maxHeight, univAddrDBRecordPtr record, FontID * fontIdP)
{
	globalVars* globals = getGlobalsPtr();
	Int16 row;
	Int16 column;
	UInt16 index;
	Int16 height;
	UInt16 lineHeight;
	FontID currFont;
	Char * str;
	FieldPtr fld;

	if(globals->FieldMap[fieldIndex] == P1ContactsbirthdayDate)
	{
		*fontIdP = globals->AddrEditFont;
		currFont = FntSetFont (*fontIdP);
		height = FntLineHeight ();
		FntSetFont (currFont);
		if(height > 20)
			return height;
		else
			return 20;
	}
	else if(globals->FieldMap[fieldIndex] == P1ContactspictureInfo)
	{
		return 32;
	}
	if (TblEditing (table))
	{
		TblGetSelection (table, &row, &column);
		if (fieldIndex == TblGetRowID (table, row))
		{
			fld = TblGetCurrentField (table);
			str = FldGetTextPtr (fld);
		}
		else
		{
			index = globals->FieldMap[fieldIndex];
			str = ((univAddrDBRecordPtr)record)->fields[index];
		}
	}
	else
	{
		index = globals->FieldMap[fieldIndex];
		str = ((univAddrDBRecordPtr)record)->fields[index];
	}


	*fontIdP = globals->AddrEditFont;
	currFont = FntSetFont (*fontIdP);
	// If the field has text empty, or the field is the current field, or
	// the font used to display blank lines is the same as the font used
	// to display text then used the view's current font setting.
	/*if ( (str && *str) ||
		 (globals->CurrentFieldIndex == fieldIndex) ||
		 (globals->AddrEditFont == addrEditBlankFont))
	{
		*fontIdP = globals->AddrEditFont;
		currFont = FntSetFont (*fontIdP);
	}

	// If the height of the font used to display blank lines is the same
	// height as the font used to display text then used the view's
	// current font setting.
	else
	{
		currFont = FntSetFont (addrEditBlankFont);
		lineHeight = FntLineHeight ();

		FntSetFont (globals->AddrEditFont);
		if (lineHeight == FntLineHeight ())
			*fontIdP = globals->AddrEditFont;
		else
		{
			*fontIdP = addrEditBlankFont;
			FntSetFont (addrEditBlankFont);
		}
	}*/

	height = FldCalcFieldHeight (str, columnWidth);
	lineHeight = FntLineHeight ();
	height = min (height, (maxHeight / lineHeight));
	height *= lineHeight;

	FntSetFont (currFont);


	return (height);
}

void PrvEditDrawBusinessCardIndicator (FormPtr formP)
{
	globalVars* globals = getGlobalsPtr();
	UInt32 uniqueID;

	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &uniqueID, NULL);
	if (globals->BusinessCardRecordID == uniqueID)
		FrmShowObject(formP, FrmGetObjectIndex (formP, EditViewBusinessCardBmp));
	else
		FrmHideObject(formP, FrmGetObjectIndex (formP, EditViewBusinessCardBmp));

}


/***********************************************************************
 *
 * FUNCTION:    PrvEditResizeDescription
 *
 * DESCRIPTION: This routine is called when the height of address
 *              field is changed as a result of user input.
 *              If the new height of the field is shorter, more items
 *              may need to be added to the bottom of the list.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    nothing
 ***********************************************************************/
void PrvEditResizeDescription (EventType * event)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 pos;
	Int16 row;
	Int16 column;
	Int16 lastRow;
	UInt16 fieldIndex;
	UInt16 lastFieldIndex;
	UInt16 topFieldIndex;
	FieldPtr fld;
	TablePtr table;
	Boolean restoreFocus = false;
	Boolean lastItemClipped;
	RectangleType itemR;
	RectangleType tableR;
	RectangleType fieldR;
	FormType* frmP;


	frmP = FrmGetActiveForm();
	// Get the current height of the field;
	fld = event->data.fldHeightChanged.pField;
	FldGetBounds (fld, &fieldR);

	// Have the table object resize the field and move the items below
	// the field up or down.
	table = CustomGetObjectPtrSmp(EditTable);
	TblHandleEvent (table, event);

	// If the field's height has expanded , we're done.
	if (event->data.fldHeightChanged.newHeight >= fieldR.extent.y)
	{
		topFieldIndex = TblGetRowID (table, 0);
		if (topFieldIndex != globals->TopVisibleFieldIndex)
			globals->TopVisibleFieldIndex = topFieldIndex;
		else
		{
			// Since the table has expanded we may be able to scroll
			// when before we might not have.
			lastRow = TblGetLastUsableRow (table);
			TblGetBounds (table, &tableR);
			TblGetItemBounds (table, lastRow, editDataColumn, &itemR);
			lastItemClipped = (itemR.topLeft.y + itemR.extent.y >
							   tableR.topLeft.y + tableR.extent.y);
			lastFieldIndex = TblGetRowID (table, lastRow);

			PrvEditUpdateScrollers(frmP, lastFieldIndex,
								   lastItemClipped);

			return;
		}
	}

	// If the field's height has contracted and the field edit field
	// is not visible then the table may be scrolled.  Release the
	// focus,  which will force the saving of the field we are editing.
	else if (TblGetRowID (table, 0) != editFirstFieldIndex)
	{
		TblGetSelection (table, &row, &column);
		fieldIndex = TblGetRowID (table, row);

		fld = TblGetCurrentField (table);
		pos = FldGetInsPtPosition (fld);
		TblReleaseFocus (table);

		restoreFocus = true;
	}

	// Add items to the table to fill in the space made available by the
	// shorting the field.
	PrvEditLoadTable(frmP);
	TblRedrawTable (table);

	// Restore the insertion point position.
	if (restoreFocus)
	{
		TblFindRowID (table, fieldIndex, &row);
		TblGrabFocus (table, row, column);
		FldSetInsPtPosition (fld, pos);
		FldGrabFocus (fld);
	}
}

void PrvEditScroll (WinDirectionType direction)
{
	globalVars* globals = getGlobalsPtr();
	UInt16				row;
	UInt16				height;
	UInt16				fieldIndex;
	UInt16				columnWidth;
	UInt16				tableHeight;
	TablePtr				table;
	FontID				curFont;
	RectangleType		r;
	univAddrDBRecordType	record;
	MemHandle			recordH;
	FormType*			frmP;

	// Before processing the scroll, be sure that the command bar has been closed.
	MenuEraseStatus (0);
	frmP = FrmGetActiveForm();

	curFont = FntSetFont (stdFont);

	table = CustomGetObjectPtrSmp(EditTable);
	TblReleaseFocus (table);

	// Get the height of the table and the width of the description
	// column.
	TblGetBounds (table, &r);
	tableHeight = r.extent.y;
	height = 0;
	columnWidth = TblGetColumnWidth (table, editDataColumn);

	// Scroll the table down.
	CustomHideObjectSmp(EditBirthdaySelector);
	if (direction == winDown)
	{
		// Get the index of the last visible field, this will become
		// the index of the top visible field, unless it occupies the
		// whole screeen, in which case the next field will be the
		// top filed.

		row = TblGetLastUsableRow (table);
		fieldIndex = TblGetRowID (table, row);

		// If the last visible field is also the first visible field
		// then it occupies the whole screeen.
		if (row == 0)
			fieldIndex = min (globals->editLastFieldIndex, fieldIndex+1);
	}

	// Scroll the table up.
	else
	{
		// Scan the fields before the first visible field to determine
		// how many fields we need to scroll.  Since the heights of the
		// fields vary,  we sum the height of the records until we get
		// a screen full.

		fieldIndex = TblGetRowID (table, 0);
		ErrFatalDisplayIf(fieldIndex > globals->editLastFieldIndex, "Invalid field Index");
		if (fieldIndex == 0)
			goto exit;

		// Get the current record
		univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);
		height = TblGetRowHeight (table, 0);
		if (height >= tableHeight)
			height = 0;

		while (height < tableHeight && fieldIndex > 0)
		{
			if(globals->FieldMap[fieldIndex-1] != P1ContactsbirthdayDate)
			{
				height += FldCalcFieldHeight (record.fields[globals->FieldMap[fieldIndex-1]],
											  columnWidth) * FntLineHeight ();
			}
			else
			{
				if(FntLineHeight() > 20)
					height += FntLineHeight ();
				else
					height += 20;
			}
			if ((height <= tableHeight) || (fieldIndex == TblGetRowID (table, 0)))
				fieldIndex--;
		}
		MemHandleUnlock(recordH);
	}

	TblMarkTableInvalid (table);
	globals->CurrentFieldIndex = noFieldIndex;
	globals->TopVisibleFieldIndex = fieldIndex;
	globals->EditRowIDWhichHadFocus = editFirstFieldIndex;
	globals->EditFieldPosition = 0;

	TblUnhighlightSelection (table);
	PrvEditLoadTable(frmP);

	TblRedrawTable (table);

	exit:
		FntSetFont (curFont);

}

Boolean PrvEditNextField (WinDirectionType direction)
{
	globalVars* globals = getGlobalsPtr();
	TablePtr tableP;
	Int16 row;
	Int16 column;
	UInt16 nextFieldNumIndex;
	FormType* frmP;
	Boolean rv = true;
	UInt16		focus = FrmGetFocus(FrmGetActiveForm());
	Boolean bDay = false;
	frmP = FrmGetActiveForm();
	tableP = CustomGetObjectPtrSmp(EditTable);
	
	if(focus != noFocus)
	{
		if(FrmGetObjectId(FrmGetActiveForm(), focus) == EditBirthdaySelector)
		{
			bDay = true;	
		}
	}
	
	//if (!TblEditing(tableP))
	//	return false;

	// Find out which field is being edited.
	if(!bDay)
	{
		TblGetSelection (tableP, &row, &column);
		nextFieldNumIndex = TblGetRowID (tableP, row);
	}
	else
	{
		nextFieldNumIndex = 29;
	}
	if (direction == winDown)
	{
		if (nextFieldNumIndex >= globals->editLastFieldIndex)
			nextFieldNumIndex = 0;
		else
			nextFieldNumIndex++;
	}
	else
	{
		if (nextFieldNumIndex == 0)
		{
			if(direction == winUp)
				return false;
			else if(direction == winLeft)
				nextFieldNumIndex = globals->editLastFieldIndex;			
		}
		else
			nextFieldNumIndex--;
	}
	TblReleaseFocus (tableP);

	globals->CurrentFieldIndex = nextFieldNumIndex;

	// If the new field isn't visible move the edit view and then
	// find the row where the next field is.
	while (!TblFindRowID(tableP, nextFieldNumIndex, &row))
	{
		// Scroll the view down placing the item
		// on the top row
		globals->TopVisibleFieldIndex = nextFieldNumIndex;
		PrvEditLoadTable(frmP);
		TblRedrawTable(tableP);
	}
	PrvEditHandleSelectField(row, editDataColumn);
	return rv;
}

void PrvEditUpdateCustomFieldLabels( FormType* frmP)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 row;
	UInt16 rowsInTable;
	TablePtr table;
#ifdef CONTACTS
	P1ContactsAppInfoPtr appInfoPtr;
#else
	AddrAppInfoPtr appInfoPtr;
#endif
	UInt16 fieldIndex;
	Int16 fieldNum;
	univAddrDBRecordType record;
	MemHandle recordH;
	Boolean redraw = false;
	
#ifdef CONTACTS
	appInfoPtr = (P1ContactsAppInfoPtr)P1ContactsDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
#else
	appInfoPtr = (AddrAppInfoPtr)AddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
#endif
	table = CustomGetObjectPtrSmp(EditTable);

	if (TblGetColumnWidth(table, editLabelColumn) != globals->EditLabelColumnWidth)
	{
		PrvEditInit (frmP, false);
		redraw = true;
	}
	else
	{
#ifdef CONTACTS
		// Get the current record
		PrvP1ContactsDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);

		rowsInTable = TblGetNumberOfRows(table);

		// Reload any renameable fields
		for (row = 0; row < rowsInTable; row++)
		{
			if (TblRowUsable (table, row))
			{
				fieldIndex = TblGetRowID (table, row);
				fieldNum = globals->FieldMap[fieldIndex];
				if(fieldNum == -1)
					continue;
				if (fieldNum >= P1ContactsfirstRenameableLabel &&
					fieldNum <= P1ContactslastRenameableLabel)
				{
					// FIXME - appInfoPtr is not initialized there!
					PrvEditInitTableRow(frmP, table, row, fieldIndex,
										TblGetItemFont (table, row, editDataColumn),
										&record, appInfoPtr);
					redraw = true;

					// Mark the row invalid so that it will draw when we call the
					// draw routine.
					TblMarkRowInvalid (table, row);
				}
			}
		}
#else
		// Get the current record
		AddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);

		rowsInTable = TblGetNumberOfRows(table);

		// Reload any renameable fields
		for (row = 0; row < rowsInTable; row++)
		{
			if (TblRowUsable (table, row))
			{
				fieldIndex = TblGetRowID (table, row);
				fieldNum = globals->FieldMap[fieldIndex];
				if(fieldNum == -1)
					continue;
				if (fieldNum >= firstRenameableLabel &&
					fieldNum <= lastRenameableLabel)
				{
					PrvEditInitTableRow(frmP, table, row, fieldIndex,
										TblGetItemFont (table, row, editDataColumn),
										&record, appInfoPtr);
					redraw = true;

					// Mark the row invalid so that it will draw when we call the
					// draw routine.
					TblMarkRowInvalid (table, row);
				}
			}
		}
#endif
		MemHandleUnlock(recordH);
	}


	if (redraw)
		TblRedrawTable(table);

	MemPtrUnlock(appInfoPtr);
}

void PrvEditUpdateDisplay( UInt16 updateCode )
{
	globalVars* globals = getGlobalsPtr();
	TableType* table;
	FormType* frmP;

#ifdef DEBUG
	LogWrite("xt_log", "edit", "PrvEditUpdateDisplay");
#endif


	// Get form by Id since it might now be the active form
	frmP = FrmGetFormPtr(EditView);

	if (updateCode & updateCustomFieldLabelChanged)
	{
		PrvEditUpdateCustomFieldLabels(frmP);
	}
	
	if (updateCode & updateCategoryChanged)
	{
		// Set the label of the category trigger.
		CategoryGetName (globals->AddrDB, globals->CurrentCategory, globals->CategoryName);
		CategorySetTriggerLabel(CustomGetObjectPtrSmp(EditCategoryTrigger), globals->CategoryName);
	}

	// Perform common tasks as necessary, and in the proper order.
	if ((updateCode & updateFontChanged) || (updateCode & updateGrabFocus) || (updateCode & frmRedrawUpdateCode))
	{
		if (updateCode & frmRedrawUpdateCode)
		{
			FrmDrawForm(frmP);
#ifdef DEBUG
			LogWrite("xt_log", "edit", "PrvEditUpdateDisplay: FrmDrawForm");
#endif
		}

		if ((updateCode & updateFontChanged) || (updateCode & frmRedrawUpdateCode))
			table = CustomGetObjectPtrSmp(EditTable);

		if (updateCode & updateFontChanged)
			TblReleaseFocus (table);

		if (updateCode & frmRedrawUpdateCode)
		{
			// If we're editing, then find out which row is being edited,
			// mark it invalid, and redraw the table (below).
			if (TblEditing(table))
			{
				Int16 row;
				Int16 column;

				TblGetSelection (table, &row, &column);
				TblMarkRowInvalid(table, row);
				TblRedrawTable (table);
			}
		}

		if ((updateCode & updateFontChanged))
		{
			PrvEditLoadTable(frmP);
			TblRedrawTable (table);
		}

		if ((updateCode & updateFontChanged) || (updateCode & updateGrabFocus))
			PrvEditRestoreEditState(frmP);
	}
}

Boolean PrvEditDoCommand (UInt16 command)
{
	globalVars* globals = getGlobalsPtr();
	univAddrDBRecordType record;
	MemHandle recordH;
	Boolean hasNote;
	UInt16 newRecord;
	UInt16 numCharsToHilite;
	FormType* frmP;
	DmOpenRef addrXTDB;
	Boolean hasData;
	UInt16 answer;

	frmP = FrmGetActiveForm();

	switch (command)
	{
		case EditRecordDeleteRecordCmd:
			MenuEraseStatus (0);
			FrmSetFocus(frmP, noFocus);   // save the field
			if (DetailsDeleteRecord ())
				FrmGotoForm (ListView);
			else
				PrvEditRestoreEditState(frmP);
			return true;
	
		case ListEditGraffitiLegacyHelpCmd:
		case ListEditGraffitiHelpCmd:
			SysGraffitiReferenceDialog(referenceDefault);
			dia_restore_state();
			return true;
	
		case EditRecordDuplicateAddressCmd:
			// Make sure the field being edited is saved
			FrmSetFocus(frmP, noFocus);
	
			univAddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);
			hasData = AddrDBRecordContainsData(&record);

			MemHandleUnlock(recordH);
			newRecord = ToolsDuplicateCurrentRecord (&numCharsToHilite, !hasData);
			if (newRecord != noRecord)
			{
				CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &addrXTDB);
				UpdateAlarms(globals->adxtLibRef, addrXTDB);	
				DmCloseDatabase(addrXTDB);
				
				globals->NumCharsToHilite = numCharsToHilite;
				globals->CurrentRecord = newRecord;
				FrmGotoForm (EditView);
			}
			return true;
	
		case EditRecordMapCmd:
			FrmPopupForm(AddressDialog);
			return true;
		case EditRecordDialCmd:
			MenuEraseStatus (0);
			PrvEditDialCurrent();
			return true;
	
		case EditRecordAttachNoteCmd:
			MenuEraseStatus (0);
			TblReleaseFocus(CustomGetObjectPtrSmp(EditTable));   // save the field
			if (NoteViewCreate())
			{
				globals->RecordNeededAfterEditView = true;
				FrmGotoForm (NewNoteView);
			}
			return true;
	
		case EditRecordDeleteNoteCmd:
			MenuEraseStatus (0);
			univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);
			hasNote = (record.fields[univNote] != NULL);
			MemHandleUnlock(recordH);
			if (hasNote )
			{
				answer = FrmAlert(DeleteNoteAlert);
				dia_restore_state();
				if(answer == DeleteNoteYes)
					NoteViewDelete ();
			}
			return true;
	
		case EditRecordSelectBusinessCardCmd:
			MenuEraseStatus (0);
			if (FrmAlert(SelectBusinessCardAlert) == SelectBusinessCardYes)
			{
				DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &globals->BusinessCardRecordID, NULL);
				PrvEditDrawBusinessCardIndicator (frmP);
			}
			dia_restore_state();
			return true;
	
		case EditRecordBeamBusinessCardCmd:
			// Make sure the field being edited is saved
			FrmSetFocus(frmP, noFocus);
	
			MenuEraseStatus (0);
			ToolsAddrBeamBusinessCard(globals->AddrDB);
			dia_restore_state();
			return true;
	
		case EditRecordLinksCmd:
			FrmPopupForm(LinksDialog);
			return true;
		case EditRecordBeamRecordCmd:
			// Make sure the field being edited is saved
			FrmSetFocus(frmP, noFocus);
	
			MenuEraseStatus (0);
			TransferSendRecord(globals->AddrDB, globals->CurrentRecord, exgBeamPrefix, NoDataToBeamAlert);
			dia_restore_state();
			return true;
	
		case EditRecordSendRecordCmd:
			// Make sure the field being edited is saved
			FrmSetFocus(frmP, noFocus);
	
			MenuEraseStatus (0);
			TransferSendRecord(globals->AddrDB, globals->CurrentRecord, exgSendPrefix, NoDataToSendAlert);
			dia_restore_state();
			return true;
	
		case EditOptionsFontCmd:
			MenuEraseStatus (0);
			RestoreDialer();
			globals->AddrEditFont = ToolsSelectFont (globals->AddrEditFont);
			SetDialer();
			return true;
		case EditOptionsGeneral:
			FrmPopupForm (PreferencesDialog);
			return true;
		case EditOptionsConnect:
			FrmPopupForm(ConnectOptionsDialog);
			return true;
		case EditOptionsColors:
			globals->gColorFormID=EditView;
			FrmPopupForm(ColorOptionsDialog);
			return true;
	
		case EditOptionsEditCustomFldsCmd:
			MenuEraseStatus (0);
			FrmSetFocus(frmP, noFocus);   // save the field
			FrmPopupForm (CustomEditDialog);
			return true;
	
		case EditOptionsAboutCmd:
			FrmPopupForm(AboutForm);
			return true;
		case EditHelpTips:
			RestoreDialer();
			FrmHelp(AddressEditHelp);
			dia_restore_state();
			SetDialer();
			return true;
	
		default:
			break;
	}

	return false;
}

Boolean PrvEditAutoFill (EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	UInt16		index;
	UInt16		focus;
	Char			*key;
	UInt32		dbType;
	FormPtr		frmP;
	FieldPtr		fld;
	DmOpenRef	dbP;
	TablePtr		table;
	UInt16 		fieldIndex;
	UInt16		fieldNum;
	UInt16		pos;

		
	if	(	TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr)
			||	(EvtKeydownIsVirtual(event))
			|| (!TxtCharIsPrint(event->data.keyDown.chr)))
		return false;

	frmP = FrmGetActiveForm();
	focus = FrmGetFocus(frmP);
	if (focus == noFocus)
		return false;

	// Find the table
	table = CustomGetObjectPtrSmp(EditTable);

	// check to see if there really is a field before continuing.
	// in case table has stopped editing but form still thinks table is the
	// focus.
	if (TblGetCurrentField(table) == NULL)
		return false;

	// Get the field number that corresponds to the table item to save.
	{
		Int16 row;
		Int16 column;

		TblGetSelection(table, &row, &column);
		fieldIndex = TblGetRowID(table, row);
	}
	
	// Select the proper database for the field we are editing,
	// or return right away if not an autofill enabled field
#ifdef CONTACTS
	fieldNum = globals->FieldMap[fieldIndex];
	switch (fieldNum) {
	case P1Contactstitle:		dbType = P1ContactstitleDBType; break;
	case P1Contactscompany:		dbType = P1ContactscompanyDBType; break;
	case P1Contactscity:		dbType = P1ContactscityDBType; break;
	case P1Contactsstate:		dbType = P1ContactsstateDBType; break;
	case P1Contactscountry:		dbType = P1ContactscountryDBType; break;
	default:	return false;
	}
#else
	fieldNum = globals->FieldMap[fieldIndex];
	switch (fieldNum) {
	case title:		dbType = titleDBType; break;
	case company:	dbType = companyDBType; break;
	case city:		dbType = cityDBType; break;
	case state:		dbType = stateDBType; break;
	case country:	dbType = countryDBType; break;
	default:	return false;
	}
#endif
	// Let the OS insert the character into the field.
	FrmHandleEvent(frmP, event);

	// The current value of the field with the focus.
	fld = TblGetCurrentField(table);
	key = FldGetTextPtr(fld);
	pos = FldGetInsPtPosition(fld);
	
	// Only auto-fill if the insertion point is at the end.
	if (pos != FldGetTextLength(fld))
		return true;

	// Open the database
	dbP = DmOpenDatabaseByTypeCreator(dbType, sysFileCAddress, dmModeReadOnly);
	if (!dbP) return true;

	// Check for a match.
	if (AutoFillLookupStringInDatabase(dbP, key, &index)) {
		MemHandle				h;
		LookupRecordPtr	r;
		UInt16				len;
		Char 					*ptr;

		h = DmQueryRecord(dbP, index);
		r = MemHandleLock(h);

		// Auto-fill.
		ptr = &r->text + StrLen (key);
		len = StrLen(ptr);

		FldInsert(fld, ptr, StrLen(ptr));

		// Highlight the inserted text.
		FldSetSelection(fld, pos, pos + len);

		MemHandleUnlock(h);
	}

	// Close the database
	DmCloseDatabase(dbP);

	return true;
}

Boolean PrvEditDialCurrent( void )
{
	globalVars* globals = getGlobalsPtr();
	TableType* tblP;
	MemHandle	addrH;
	univAddrDBRecordType record;
	Err			err;
	UInt16	phoneIndex = kDialListShowInListPhoneIndex;
	UInt16	lineIndex = 0;
	UInt16	fieldPosition = 0;

	// if focus on a phone, dial current number
	// else dial show in list

	tblP = CustomGetObjectPtrSmp(EditTable);

	if (TblEditing(tblP))
	{
		Int16		row;
		Int16 		column;
		UInt16 		fieldIndex;
		FieldType* 	fldP;

		// Find out which field is being edited.
		TblGetSelection(tblP, &row, &column);
		fldP = TblGetCurrentField(tblP);
		if (fldP)
			fieldPosition = FldGetInsPtPosition(fldP);

		// phoneIndex = TblGetRowID(tblP, row) - firstPhoneField;
		TblReleaseFocus(tblP);

		fieldIndex = globals->FieldMap[TblGetRowID(tblP, row)];
		if (isPhoneField(fieldIndex))
			phoneIndex = fieldIndex - univFirstPhoneField;
	}

	err = univAddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &addrH);
	if ( err != errNone )
		return false;

	if (phoneIndex != kDialListShowInListPhoneIndex)
	{
		if (ToolsIsPhoneIndexSupported(&record, phoneIndex))
			lineIndex = ToolsGetLineIndexAtOffset(globals->adxtLibRef, record.fields[phoneIndex + univFirstPhoneField], fieldPosition);
		else
			// This mean like something like mail was edited
			phoneIndex = kDialListShowInListPhoneIndex;
	}	
	MemHandleUnlock(addrH);
	return DialListShowDialog(globals->CurrentRecord, phoneIndex, lineIndex);
}

void PrvEditLoadTable( FormType* frmP )
{
	globalVars* globals = getGlobalsPtr();
	UInt16 row, savedRow;
	Int16 testRow;
	UInt16 numRows;
	UInt16 lineHeight;
	UInt16 fieldIndex, savedFieldIndex;
	UInt16 lastFieldIndex;
	UInt16 dataHeight;
	UInt16 tableHeight;
	UInt16 columnWidth;
	UInt16 pos, oldPos;
	UInt16 height, oldHeight;
	FontID fontID;
	FontID currFont;
	TablePtr table;
	Boolean rowUsable;
	Boolean rowsInserted = false;
	Boolean lastItemClipped;
	RectangleType r;
	Boolean bDayLast = false;
	MemHandle recordH;
#ifdef CONTACTS
	P1ContactsAppInfoPtr appInfoPtr;
#else
	AddrAppInfoPtr appInfoPtr;
#endif
	univAddrDBRecordType record;

#ifdef DEBUG
	LogWrite("xt_log", "edit", "PrvEditLoadTable");
#endif

#ifdef CONTACTS
	appInfoPtr = (P1ContactsAppInfoPtr) P1ContactsDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	// Get the current record
#else
	appInfoPtr = (AddrAppInfoPtr) AddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	// Get the current record
#endif
	univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);	
	// Get the height of the table and the width of the description
	// column.
	table = CustomGetObjectPtrSmp(EditTable);
	if(TblFindRowID (table, P1ContactsbirthdayDate, &testRow))
	{
		if(record.birthdayInfo.birthdayDate.day != 0 || record.birthdayInfo.birthdayDate.month != 0)
		{
			DateFormatType dateFormat;
			dateFormat=PrefGetPreference(prefLongDateFormat);
			DateToAscii(record.birthdayInfo.birthdayDate.month, record.birthdayInfo.birthdayDate.day, record.birthdayInfo.birthdayDate.year+1904, dateFormat, globals->gDateTxt);
			CustomSetCtlLabelPtrSmp(EditBirthdaySelector, globals->gDateTxt);	
		}
	}
	else
	{
		//CustomHideObject(frmP, EditBirthdaySelector);
	}
	
	TblGetBounds (table, &r);
	tableHeight = r.extent.y;
	columnWidth = TblGetColumnWidth (table, editDataColumn);

	// If we currently have a selected record, make sure that it is not
	// above the first visible record.
	if (globals->CurrentFieldIndex != noFieldIndex)
	{
		if (globals->CurrentFieldIndex < globals->TopVisibleFieldIndex)
			globals->TopVisibleFieldIndex = globals->CurrentFieldIndex;
	}

	row = 0;
	dataHeight = 0;
	oldPos = pos = 0;
	fieldIndex = globals->TopVisibleFieldIndex;
	lastFieldIndex = fieldIndex;

	// Load records into the table.
	currFont = FntSetFont (globals->AddrEditFont);
	lineHeight = FntLineHeight ();
	FntSetFont (currFont);
	while (fieldIndex <= globals->editLastFieldIndex)
	{
		// Compute the height of the field's text string.
		height = PrvEditGetFieldHeight (table, fieldIndex, columnWidth, tableHeight, &record, &fontID);
		
		// Is there enought room for at least one line of the the decription.
		if ((fieldIndex  != 29 && tableHeight >= dataHeight + lineHeight) || (fieldIndex == 29 && tableHeight >= dataHeight + height))
		{
			rowUsable = TblRowUsable (table, row);

			// Get the height of the current row.
			if (rowUsable)
				oldHeight = TblGetRowHeight (table, row);
			else
				oldHeight = 0;

			// If the field is not already being displayed in the current
			// row, load the field into the table.
			if(fieldIndex == 29)
			{
				if(tableHeight - dataHeight - height < lineHeight)
				{
					bDayLast = true;
				}
			}
			if ((! rowUsable) ||
				(TblGetRowID (table, row) != fieldIndex) ||
				(TblGetItemFont (table, row, editDataColumn) != fontID))
			{
				PrvEditInitTableRow(frmP, table, row, fieldIndex, fontID,
									&record, appInfoPtr);	
			}

			// If the height or the position of the item has changed draw the item.
			if (height != oldHeight)
			{
				TblSetRowHeight (table, row, height);
				TblMarkRowInvalid (table, row);
			}
			else if (pos != oldPos)
			{
				TblMarkRowInvalid (table, row);
			}

			pos += height;
			oldPos += oldHeight;
			lastFieldIndex = fieldIndex;
			fieldIndex++;
			row++;
		}

		dataHeight += height;
		savedFieldIndex = fieldIndex;
		savedRow = row;
		

		// Is the table full?
		if (dataHeight >= tableHeight)
		{
			// If we have a currently selected field, make sure that it is
			// not below the last visible field.  If the currently selected
			// field is the last visible record, make sure the whole field
			// is visible.
			if (globals->CurrentFieldIndex == noFieldIndex)
				break;

			// Above last visible?
			else if  (globals->CurrentFieldIndex < fieldIndex)
				break;

			// Last visible?
			else if (fieldIndex == lastFieldIndex)
			{
				if ((fieldIndex == globals->TopVisibleFieldIndex) || (dataHeight == tableHeight))
					break;
			}

			// Remove the top item from the table and reload the table again.
			globals->TopVisibleFieldIndex++;
			fieldIndex = globals->TopVisibleFieldIndex;

			row = 0;
			dataHeight = 0;
			oldPos = pos = 0;
		}
	}


	// Hide the item that don't have any data.
	numRows = TblGetNumberOfRows (table);
	while (row < numRows)
	{
		TblSetRowUsable (table, row, false);
		row++;
	}

	//bday field shouldn't be the last one
	//if(savedFieldIndex == 30)
	//{
	//	TblRemoveRow(table, row);
	//}
	
	// If the table is not full and the first visible field is
	// not the first field	in the record, displays enough fields
	// to fill out the table by adding fields to the top of the table.
	if (bDayLast || dataHeight + lineHeight <= tableHeight)
	{
		//fieldIndex = globals->TopVisibleFieldIndex;
		//if (fieldIndex == 0) break;
		//fieldIndex--;
		if(globals->TopVisibleFieldIndex > 0)
		{
			if(bDayLast)
				globals->TopVisibleFieldIndex--;
			else
			{
				UInt16 numLines;
				numLines = (tableHeight - dataHeight) / 20;
				if(numLines == 0)
					globals->TopVisibleFieldIndex--;
				else
					globals->TopVisibleFieldIndex -= numLines;
			}
			
			PrvEditLoadTable( frmP );
		}
		/*// Compute the height of the field.
		height = PrvEditGetFieldHeight (table, fieldIndex,
										columnWidth, tableHeight, &record, &fontID);

		// If adding the item to the table will overflow the height of
		// the table, don't add the item.
		if (dataHeight + height > tableHeight)
			break;

		// Insert a row before the first row.
		TblInsertRow (table, 0);

		PrvEditInitTableRow(frmP, table, 0, fieldIndex, fontID, &record, appInfoPtr);
		TblSetRowHeight (table, 0, height);
				
		globals->TopVisibleFieldIndex = fieldIndex;

		rowsInserted = true;

		dataHeight += height;*/
	}

	//if (rowsInserted)
	//	TblMarkTableInvalid (table);

	// If the height of the data in the table is greater than the height
	// of the table, then the bottom of the last row is clip and the
	// table is scrollable.
	lastItemClipped = (dataHeight > tableHeight);

	// Update the scroll arrows.
	PrvEditUpdateScrollers(frmP, lastFieldIndex, lastItemClipped);


	MemHandleUnlock(recordH);
	MemPtrUnlock(appInfoPtr);
}