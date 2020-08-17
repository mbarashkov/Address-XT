#include "AddrView.h"
#include "AddrView2.h"

/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/

Boolean			ViewOnUpdate(EventType * event);
static void		PrvViewOpen(void);
static void		PrvViewNewLine (UInt16 *width);
static void		PrvViewAddSpaceForText (const Char * const string, UInt16 *width);
static void		PrvViewPositionTextAt (UInt16 *width, const UInt16 position);
static void		PrvViewAddField (const UInt16 fieldNum, UInt16 *width, const UInt16 maxWidth, const UInt16 indentation);
static void		PrvViewAddEmptyField (const UInt16 fieldNum, UInt16 *width, const UInt16 maxWidth, const UInt16 indentation);
static void		PrvViewErase( FormType* frmP );
static UInt16	PrvViewCalcNextLine(UInt16 i, UInt16 oneLine, Boolean recursive);
static void		PrvViewDrawSelectedText (UInt16 currentField, UInt16 selectPos, UInt16 selectLen, UInt16 textY);
static void		PrvViewDraw ( FormType* frmP, UInt16 selectFieldNum, UInt16 selectPos, UInt16 selectLen, Boolean drawOnlySelectField );
static UInt16	PrvViewScrollOnePage (Int16 newTopRecordViewLine, WinDirectionType direction);
static void		PrvViewScroll (WinDirectionType direction);
static void		PrvViewMakeVisible (UInt16 selectFieldNum, UInt16 selectPos, UInt16 selectLen);
static Boolean	PrvViewPhoneNumberAt (Int16 x, Int16 y, UInt16 *fieldNumP, UInt16 *offsetP, UInt16 *lengthP);
static Boolean 	PrvViewHandleTapOnPhoneNumber (UInt16 fieldNum, UInt16 offset, UInt16 length);
static Boolean 	PrvViewHandlePen (EventType * event);

static Boolean 	AddrViewMoveObjects(FormType* frmP, Coord dx, Coord dy);
static void 	PrvViewDrawLinksButton();

static Boolean AddrViewMoveObjects(FormType* frmP, Coord dx, Coord dy)
{
    globalVars* globals = getGlobalsPtr();
	RectangleType r;
    Boolean resized = false;
    
    if (dx != 0 || dy != 0)
    {
#ifdef DEBUG
		LogWrite("xt_log", "view", "AddrViewMoveObjects");
#endif
		if(globals->gNavigation)
		{
		    WinGetBounds(WinGetDisplayWindow(), &r);
			WinEraseRectangle (&r, 0);
		}

       	// NOTE: be careful to avoid object migration!
		MoveFormObjectHide(frmP, RecordDoneButton, 0, dy);
        MoveFormObjectHide(frmP, RecordEditButton, 0, dy);
        MoveFormObjectHide(frmP, RecordNewButton, 0, dy);
        
        MoveFormObject(frmP, RecordUpButton, dx, dy);
        MoveFormObject(frmP, RecordDownButton, dx, dy);
        
       	MoveFormObjectHide(frmP, RecordCategoryLabel, dx, 0);
        
       	FrmGetObjectBounds(frmP, FrmGetObjectIndex(frmP, RecordViewDisplay), &r);
       	r.extent.y+=dy;
       	r.extent.x+=dx;
       	FrmSetObjectBounds(frmP, FrmGetObjectIndex(frmP, RecordViewDisplay), &r);
       	
       	resized = true;
    }
    
    return resized;
}

Boolean ViewOnUpdate(EventType * event)
{
#pragma unused(event)
	FormPtr frmP = FrmGetActiveForm ();

#ifdef DEBUG
	LogWrite("xt_log", "view", "ViewOnUpdate");
#endif

	PrvViewClose();
	PrvViewErase(frmP);
   	PrvViewOpen();
	return true;
}

static Boolean ViewOnFrmOpenEvent()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetActiveForm ();
	dia_save_state();
	dia_enable(frmP, true);
	dia_resize(frmP, AddrViewMoveObjects);
	globals->gAddressView=true;
	PrvViewOpen();
	return true;
}


static Boolean ViewOnCtlSelectEvent(EventType* event)
{
	Boolean handled = false;
	globalVars* globals = getGlobalsPtr();
	switch (event->data.ctlSelect.controlID)
	{
		case RecordDialButton:
			DialListShowDialog(globals->CurrentRecord, kDialListShowInListPhoneIndex, 0);
			handled=true;
			break;
		case RecordLinksButton:
			DisplayLinks();
			handled = true;
			break;		
		case RecordDoneButton:
			// When we return to the ListView highlight this record.
			globals->ListViewSelectThisRecord = globals->CurrentRecord;
			FrmGotoForm (ListView);
			handled = true;
			break;
		case RecordEditButton:
			FrmGotoForm (EditView);
			handled = true;
			break;
		case RecordNewButton:
			EditNewRecord();
			handled = true;
			break;
		default:
			break;
	}
	return handled;
}

static Boolean ViewOnCtlRepeatEvent(EventType* event)
{
	switch (event->data.ctlRepeat.controlID)
	{
		case RecordUpButton:
			PrvViewScroll(winUp);
			// leave unhandled so the buttons can repeat
			break;
		case RecordDownButton:
			PrvViewScroll(winDown);
			// leave unhandled so the buttons can repeat
			break;
		default:
			break;
	}
	return false;
}

static Boolean ViewOnKeyDownEvent(EventType* event)
{
	Boolean handled = false;
	globalVars* globals = getGlobalsPtr();

#ifdef DEBUG
	LogWrite("xt_log", "view", "ViewOnKeyDownEvent");
#endif

	if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
	{
		if(globals->gDeviceFlags.bits.treoWithPhoneKeyOnly && event->data.keyDown.chr == vchrHard1)
		{
			if(AddressDialable(globals->CurrentRecord, kDialListShowInListPhoneIndex) && ToolsIsDialerPresent())
			{
				DialListShowDialog(globals->CurrentRecord, kDialListShowInListPhoneIndex, 0);
			}
			else
			{
				StartPhone();
			}			
			return true;
		}	
		else if(globals->gDeviceFlags.bits.treoWithSendKeys && event->data.keyDown.chr == vchrHard11)
		{
			if(AddressDialable(globals->CurrentRecord, kDialListShowInListPhoneIndex) && ToolsIsDialerPresent())
			{
				DialListShowDialog(globals->CurrentRecord, kDialListShowInListPhoneIndex, 0);
			}
			return true;
		}
		globals->ListViewSelectThisRecord = globals->CurrentRecord;
		FrmGotoForm (ListView);
		return true;
	}
	
	else if (event->data.keyDown.chr==vchrRockerCenter && !globals->gNavigation)
	{
		FrmGotoForm (EditView);
		return true;
	}
	else if (event->data.keyDown.chr==vchrRockerUp && !globals->gNavigation)
	{
		PrvViewScroll (winUp);
		return true;
	}
	else if (event->data.keyDown.chr==vchrRockerRight && !globals->gNavigation)
	{
		DialListShowDialog(globals->CurrentRecord, kDialListShowInListPhoneIndex, 0);
		return true;
	}
	else if (event->data.keyDown.chr==vchrRockerDown && !globals->gNavigation)
	{
		PrvViewScroll (winDown);
		return true;
	}		
	
	else if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event) && !globals->gNavigation)
	{
			
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Left))
		{
			globals->ListViewSelectThisRecord = globals->CurrentRecord;
			FrmGotoForm (ListView);
			return true;
		}
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			FrmGotoForm (EditView);
			return true;
		}		
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Right))
		{
			DialListShowDialog(globals->CurrentRecord, kDialListShowInListPhoneIndex, 0);
			return true;
		}
		
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Up) || (event->data.keyDown.chr==vchrPageUp))
		{
			PrvViewScroll (winUp);
			return true;
		}
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Down) || (event->data.keyDown.chr==vchrPageDown))
		{
			PrvViewScroll (winDown);
			return true;
		}		   	 	
	}  
	else if (EvtKeydownIsVirtual(event))
	{
		switch (event->data.keyDown.chr)
		{
			case vchrPageUp:
			case vchrRockerUp:
				PrvViewScroll (winUp);
				handled = true;
				break;

			case vchrPageDown:
			case vchrRockerDown:
				PrvViewScroll (winDown);
				handled = true;
				break;
				
			case vchrSendData:
				TransferSendRecord(globals->AddrDB, globals->CurrentRecord, exgBeamPrefix, NoDataToBeamAlert);
				handled = true;
				break;
			default:
				break;
		}
	}
	return handled;
}


static Boolean ViewOnFrmGotoEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetActiveForm ();
	// enable dia - if we are called from GoTo-launch code
	dia_save_state();
	dia_enable(frmP, true);
	dia_resize(frmP, AddrViewMoveObjects);
	
	globals->CurrentRecord = event->data.frmGoto.recordNum;
	PrvViewInit(frmP);
	PrvViewMakeVisible(event->data.frmGoto.matchFieldNum, event->data.frmGoto.matchPos, event->data.frmGoto.matchLen);
	FrmDrawForm(frmP);
	PrvViewDraw(frmP, event->data.frmGoto.matchFieldNum, event->data.frmGoto.matchPos, event->data.frmGoto.matchLen, false);
	globals->PriorAddressFormID = FrmGetFormId(frmP);
	return true;
}
			
static Boolean ViewOnKFrmCustomUpdateEvent()
{
	// Event sent by Custom view because when custom fields are renamed, it can be necessary to recalculate view screen display: if the width of the custom field is enlarged (and if its content can only be displayed using 2 lines), its content can be displayed on the next line
	PrvViewClose();
	PrvViewOpen();
	return true;
}

Boolean ViewHandleEvent(EventType * event)
{
	Boolean handled = false;
	switch (event->eType)
	{
		case frmOpenEvent:
			handled = ViewOnFrmOpenEvent();
			break;	
		case frmCloseEvent:
			WinEraseWindow();
			PrvViewClose();
			break;	
		case ctlSelectEvent:
			handled = ViewOnCtlSelectEvent(event);
			break;	
		case penDownEvent:
			handled = PrvViewHandlePen(event);
			break;	
		case ctlRepeatEvent:
			handled = ViewOnCtlRepeatEvent(event);
			break;		
		case keyDownEvent:
			handled = ViewOnKeyDownEvent(event);
			break;		
		case menuEvent:
			return PrvViewDoCommand (event->data.menu.itemID);	
		case menuCmdBarOpenEvent:
			handled = ViewOnMenuCmdBarOpenEvent(event);
			break;	
		case menuOpenEvent:
			handled = ViewOnMenuOpenEvent();
			break;	
		case frmUpdateEvent:
			handled = ViewOnUpdate(event);
			break;	
		case kFrmCustomUpdateEvent:
			handled = ViewOnKFrmCustomUpdateEvent();
			break;			
		case frmGotoEvent:
			handled = ViewOnFrmGotoEvent(event);
			break;
		case winEnterEvent:
			// menu
			if(event->data.winEnter.enterWindow == 0)
				break;
			dia_win_enter();
			break;
		default:
			handled = dia_handle_event(event, AddrViewMoveObjects);
			break;
	}

	return (handled);
}

#pragma mark -

/***********************************************************************
 *
 * FUNCTION:    PrvViewOpen
 *
 * DESCRIPTION: This routine is called when frmOpenEvent is received
 *
 * PARAMETERS:  None
 *
 * RETURNED:    None
 *
 * REVISION HISTORY:
 *        	Name   	Date      	Description
 *        	----   	----      	-----------
 *			FPa		11/28/00	Initial Revision
 *
 ***********************************************************************/
void PrvViewOpen(void)
{
	globalVars* globals = getGlobalsPtr();
	FormType* frmP;
	
#ifdef DEBUG
	LogWrite("xt_log", "view", "PrvViewOpen");
#endif
	
	frmP = FrmGetActiveForm ();
	PrvViewInit(frmP);
	FrmDrawForm(frmP);
#ifdef DEBUG
	LogWrite("xt_log", "view", "PrvViewOpen: FrmDrawForm");
#endif
	// 
	PrvViewDraw(frmP, 0, 0, 0, false);
	
	PrvViewDrawBusinessCardIndicator(frmP);
	globals->PriorAddressFormID = FrmGetFormId(frmP);
	//if(AddressDialable(globals->CurrentRecord, kDialListShowInListPhoneIndex) && ToolsIsDialerPresent())
		FrmShowObject(frmP, FrmGetObjectIndex (frmP, RecordDialButton));
	//else
	//	FrmHideObject(frmP, FrmGetObjectIndex (frmP, RecordDialButton));
	
	PrvViewDrawLinksButton();
	
	//add phone to recent numbers list
	if(globals->gEnabledRecent)
		AddrDBAddToRecent(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord);
}


/***********************************************************************
 *
 * FUNCTION:    PrvViewClose
 *
 * DESCRIPTION: This routine is called when frmCloseEvent is received
 *
 * PARAMETERS:  None
 *
 * RETURNED:    None
 *
 * REVISION HISTORY:
 *        	Name   	Date      	Description
 *        	----   	----      	-----------
 *			FPa		11/28/00	Initial Revision
 *			FPa		12/05/00	Added MemHandleUnlock
 *
 ***********************************************************************/
void PrvViewClose(void)
{
	MemHandle handle;
	globalVars* globals = getGlobalsPtr();

#ifdef DEBUG
	LogWrite("xt_log", "view", "PrvViewClose");
#endif

	if (globals->recordViewRecordH)
	{
		MemHandleUnlock(globals->recordViewRecordH);
		globals->recordViewRecordH = 0;
	}
	
	if (globals->RecordViewLines)
	{
		handle = MemPtrRecoverHandle(globals->RecordViewLines);
		MemHandleUnlock(handle);
		MemHandleFree(handle);
		
		globals->RecordViewLines = 0;
	}
}

static void PrvViewAddEmptyField (const UInt16 fieldNum, UInt16 *width, const UInt16 maxWidth, const UInt16 indentation)
{
	globalVars* globals = getGlobalsPtr();
	if (globals->RecordViewLastLine >= recordViewLinesMax)
		return;

	if (*width >= maxWidth)
		*width = indentation;

	globals->RecordViewLines[globals->RecordViewLastLine].fieldNum = fieldNum;
	globals->RecordViewLines[globals->RecordViewLastLine].offset = 0;
	globals->RecordViewLines[globals->RecordViewLastLine].x = *width;
	globals->RecordViewLines[globals->RecordViewLastLine].length = 0;
	globals->RecordViewLines[globals->RecordViewLastLine].empty = true;
	globals->RecordViewLastLine++;
}

static void PrvViewAddBlankLine(UInt16 *width)
{
	globalVars* globals = getGlobalsPtr();
	if (globals->RecordViewLastLine > 0 &&
		globals->RecordViewLines[globals->RecordViewLastLine - 1].fieldNum != recordViewBlankLine)
	{
		PrvViewNewLine(width);
	}

}

/***********************************************************************
 *
 * FUNCTION:    PrvViewInit
 *
 * DESCRIPTION: This routine initializes the "Record View" of the
 *              Address application.  Most importantly it lays out the
 *                record and decides how the record is drawn.
 *
 * PARAMETERS:  frm - pointer to the view form.
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *        	Name   	Date      	Description
 *        	----   	----      	-----------
 *        	roger  	6/21/95   	Initial Revision
 *			aro		09/25/00	Add fieldMapH to release the resource properly
 *			FPa		11/27/00	Do not add company field if this field is blank
 *
 ***********************************************************************/
void PrvViewInit( FormType* frm )
{
	globalVars* globals = getGlobalsPtr();
	UInt16 attr;
	UInt16 index;
	UInt16 category;
	MemHandle RecordViewLinesH;
	UInt16 width = 0;
	RectangleType r;
	UInt16 maxWidth;
	FontID curFont;
	UInt16 i;
	UInt16 fieldIndex;
	UInt16 phoneLabelNum;
#ifndef CONTACTS
	Int16 counter;
	UInt16 addrXTDBSize;
#endif
	UInt32 id;
	DmOpenRef addrXTDB;
	Boolean hasImage = false;
	univAppInfoPtr appInfoPtr;
	
	UInt32 pref = PrefGetPreference(prefLocale);
	LmLocaleType* pUserLocale = (LmLocaleType*)&pref;
	Boolean eurAddrFormat = true;
		
#ifdef DEBUG
	LogWrite("xt_log", "view", "PrvViewInit");
#endif

	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
	CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &addrXTDB);
	globals->ImageOffset = 0;
	
	appInfoPtr = univAddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);

	if(!ToolsIsDialerPresent())
		CustomHideObjectSmp(RecordDialButton);
	
	// Set the category label.
	if (globals->CurrentCategory == dmAllCategories)
	{
		DmRecordInfo (globals->AddrDB, globals->CurrentRecord, &attr, NULL, NULL);
		category = attr & dmRecAttrCategoryMask;
	}
	else
		category = globals->CurrentCategory;

	CategoryGetName (globals->AddrDB, category, globals->CategoryName);
	index = FrmGetObjectIndex (frm, RecordCategoryLabel);
	FrmSetCategoryLabel (frm, index, globals->CategoryName);

	// Allocate the record view lines
	RecordViewLinesH = MemHandleNew(sizeof(RecordViewLineType) * recordViewLinesMax);
	ErrFatalDisplayIf (!RecordViewLinesH, "Out of memory");

	globals->RecordViewLines = MemHandleLock(RecordViewLinesH);
	globals->RecordViewLastLine = 0;
	globals->TopRecordViewLine = 0;

	FrmGetFormBounds(frm, &r);
	maxWidth = r.extent.x;

	
	if (globals->RecordLabelColumnWidth == 0)
	{
		globals->RecordLabelColumnWidth = ToolsGetLabelColumnWidth (globals->adxtLibRef, appInfoPtr, globals->AddrRecordFont);
	}

	// Get the record to display.  recordViewRecordH may have data if
	// we are redisplaying the record (custom fields changed).
	if (globals->recordViewRecordH)
		MemHandleUnlock(globals->recordViewRecordH);
	
	univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &(globals->recordViewRecord), &(globals->recordViewRecordH));
	
	
	
#ifdef CONTACTS
	if(pnoJpegRefNum != sysInvalidRefNum )
	{
		if(globals->recordViewRecord.pictureInfo.pictureSize > 0)
		{
			Coord width, height;
			
			if(GetJpegInfo(globals->recordViewRecord.pictureInfo.pictureData, globals->recordViewRecord.pictureInfo.pictureSize, &width, &height))
			//if(ImageLibGetDimensionsJPEG(globals->imageLibRef, globals->recordViewRecord.pictureInfo.pictureData, globals->recordViewRecord.pictureInfo.pictureSize, &width, &height, false))
				hasImage = false;
			else
			{
			
				hasImage = true;
			}
			
			width /= 2;
			height /= 2;
			
			globals->ImageOffset = width + 2;
			globals->ImageHeight = height;
		}
	}
#endif
	
	if(!hasImage)
	{
		globals->ImageOffset = 0;
		globals->ImageHeight = 0;
	}
	// Here we construct the recordViewLines info by laying out
	// the record
	curFont = FntSetFont (largeBoldFont);
	
	//determine address format
	pref = PrefGetPreference(prefLocale);
	pUserLocale = (LmLocaleType*)&pref;
	switch(pUserLocale->country)
	{
		case cAustralia:		
			eurAddrFormat = false;
			break;
		case cCanada:			
			eurAddrFormat = false;
			break;
		case cIndia:					
			eurAddrFormat = false;
			break;
		case cIndonesia:				
			eurAddrFormat = false;
			break;
		case cIreland:				
			eurAddrFormat = false;
			break;
		case cJapan:					
			eurAddrFormat = false;
			break;
		case cUnitedKingdom:			
			eurAddrFormat = false;
			break;
		case cUnitedStates:					
			eurAddrFormat = false;
			break;
	}
			
	
#ifdef CONTACTS
	if(hasImage && 
	globals->recordViewRecord.fields[P1Contactscompany] == NULL &&
	globals->recordViewRecord.fields[P1Contactsname] == NULL &&
	globals->recordViewRecord.fields[P1ContactsfirstName] == NULL &&
	globals->recordViewRecord.fields[P1Contactstitle] == NULL)
	{
		PrvViewAddEmptyField(P1Contactscompany, &width, maxWidth - globals->ImageOffset, 0);
		PrvViewNewLine(&width);
	}


	if (globals->recordViewRecord.fields[P1Contactsname] == NULL &&
		globals->recordViewRecord.fields[P1ContactsfirstName] == NULL &&
		globals->recordViewRecord.fields[P1Contactscompany] != NULL)
	{
		PrvViewAddField(P1Contactscompany, &width, maxWidth - globals->ImageOffset, 0);
		PrvViewNewLine(&width);
	}
	else
	{
		if (globals->recordViewRecord.fields[P1ContactsfirstName] != NULL)
		{
			PrvViewAddField(P1ContactsfirstName, &width, maxWidth - globals->ImageOffset, 0);
			// Separate the last name from the first name as long
			// as they are together on the same line.
			if (width > 0)
				PrvViewAddSpaceForText (" ", &width);
		}
		if (globals->recordViewRecord.fields[P1Contactsname])
		{
			PrvViewAddField(P1Contactsname, &width, maxWidth - globals->ImageOffset, 0);					
		}
		PrvViewNewLine(&width);
	}
	globals->RecordViewFirstPlainLine = globals->RecordViewLastLine;
	FntSetFont (globals->AddrRecordFont);

	if (globals->recordViewRecord.fields[P1Contactstitle])
	{
		PrvViewAddField(P1Contactstitle, &width, maxWidth - globals->ImageOffset, 0);
		PrvViewNewLine(&width);
	}
	if (globals->recordViewRecord.fields[P1Contactscompany] != NULL &&
		(globals->recordViewRecord.fields[P1Contactsname] != NULL ||
		 globals->recordViewRecord.fields[P1ContactsfirstName] != NULL))
	{
		PrvViewAddField(P1Contactscompany, &width, maxWidth - globals->ImageOffset, 0);
		PrvViewNewLine(&width);
	}
			 
	PrvViewAddBlankLine(&width);

	// Layout the phone numbers.  Start each number on its own line.
	// Put the label first, followed by ": " and then the number
	for (fieldIndex = P1ContactsfirstPhoneField; fieldIndex <= P1ContactslastPhoneField; fieldIndex++)
	{
		if (globals->recordViewRecord.fields[fieldIndex])
		{
			phoneLabelNum = P1ContactsGetPhoneLabel(&(globals->recordViewRecord), fieldIndex);
			PrvViewAddSpaceForText (appInfoPtr->fieldLabels[phoneLabelNum + ((phoneLabelNum < 7) ? P1ContactsfirstPhoneField : (P1ContactsfirstPhoneField + 29))], &width);
			PrvViewAddSpaceForText (": ", &width);
			PrvViewPositionTextAt(&width, globals->RecordLabelColumnWidth);
			PrvViewAddField(fieldIndex, &width, maxWidth, globals->RecordLabelColumnWidth);
			PrvViewNewLine(&width);
		}
	}

	PrvViewAddBlankLine(&width);

	for (fieldIndex = firstChatField; fieldIndex <= lastChatField; fieldIndex++)
	{
		if (globals->recordViewRecord.fields[fieldIndex])
		{
			phoneLabelNum = GetChatLabel(&(globals->recordViewRecord), fieldIndex);
			PrvViewAddSpaceForText (appInfoPtr->fieldLabels[phoneLabelNum + 41], &width);
			PrvViewAddSpaceForText (": ", &width);
			PrvViewPositionTextAt(&width, globals->RecordLabelColumnWidth);
			PrvViewAddField(fieldIndex, &width, maxWidth, globals->RecordLabelColumnWidth);
			PrvViewNewLine(&width);
		}
	}
	
	if (globals->recordViewRecord.fields[P1Contactswebpage])
	{
		PrvViewAddSpaceForText (appInfoPtr->fieldLabels[P1Contactswebpage], &width);
		PrvViewAddSpaceForText (": ", &width);
		PrvViewPositionTextAt(&width, globals->RecordLabelColumnWidth);
		PrvViewAddField(fieldIndex, &width, maxWidth, globals->RecordLabelColumnWidth);
		PrvViewNewLine(&width);
	}

	PrvViewAddBlankLine(&width);
	
	// Now do the address information
	
	for(i = 0; i < 3; i++)
	{
		Boolean added;
		added = false;
		if (globals->recordViewRecord.fields[P1Contactsaddress + (P1Contactsaddress2-P1Contactsaddress)*i])
		{
			PrvViewAddField(P1ContactsVirtualAddr1Label + i, &width, maxWidth, 0);
			added = true;
			
			PrvViewAddField(P1Contactsaddress + (P1Contactsaddress2-P1Contactsaddress)*i, &width, maxWidth, 0);
			PrvViewNewLine(&width);
		}

		// We need to format the city, state, and zip code differently depending
		// on which country it is. For now, assume that if the city comes first,
		// we use the standard US formatting of [city, ][state   ][zip]<cr>,
		// otherwise we'll use the "int'l" format of [zip ][city]<cr>[state]<cr>.

		// Decide if we're formatting it US-style, or int'l-style
		if (!eurAddrFormat)
		{
			if (globals->recordViewRecord.fields[P1Contactscity + (P1Contactsaddress2-P1Contactsaddress)*i])
			{
				if(!added)
				{
					PrvViewAddField(P1ContactsVirtualAddr1Label + i, &width, maxWidth, 0);
					added = true;
				}
				PrvViewAddField(P1Contactscity + (P1Contactsaddress2-P1Contactsaddress)*i, &width, maxWidth, 0);
			}
			if (globals->recordViewRecord.fields[P1Contactsstate + (P1Contactsaddress2-P1Contactsaddress)*i])
			{
				if(!added)
				{
					PrvViewAddField(P1ContactsVirtualAddr1Label + i, &width, maxWidth, 0);
					added = true;
				}
				if (width > 0)
					PrvViewAddSpaceForText (", ", &width);
				PrvViewAddField(P1Contactsstate + (P1Contactsaddress2-P1Contactsaddress)*i, &width, maxWidth, 0);
			}
			if (globals->recordViewRecord.fields[P1ContactszipCode + (P1Contactsaddress2-P1Contactsaddress)*i])
			{
				if(!added)
				{
					PrvViewAddField(P1ContactsVirtualAddr1Label + i, &width, maxWidth, 0);
					added = true;
				}
				if (width > 0)
					PrvViewAddSpaceForText ("   ", &width);
				PrvViewAddField(P1ContactszipCode + (P1Contactsaddress2-P1Contactsaddress)*i, &width, maxWidth, 0);
			}
			if (globals->recordViewRecord.fields[P1Contactscity + (P1Contactsaddress2-P1Contactsaddress)*i] ||
				globals->recordViewRecord.fields[P1Contactsstate + (P1Contactsaddress2-P1Contactsaddress)*i] ||
				globals->recordViewRecord.fields[P1ContactszipCode + (P1Contactsaddress2-P1Contactsaddress)*i])
			{
				PrvViewNewLine(&width);
			}
		}
		else
		{
			if (globals->recordViewRecord.fields[P1ContactszipCode + (P1Contactsaddress2-P1Contactsaddress)*i])
			{
				if(!added)
				{
					PrvViewAddField(P1ContactsVirtualAddr1Label + i, &width, maxWidth, 0);
					added = true;
				}
				PrvViewAddField(P1ContactszipCode + (P1Contactsaddress2-P1Contactsaddress)*i, &width, maxWidth, 0);
			}
			if (globals->recordViewRecord.fields[P1Contactscity + (P1Contactsaddress2-P1Contactsaddress)*i])
			{
				if(!added)
				{
					PrvViewAddField(P1ContactsVirtualAddr1Label + i, &width, maxWidth, 0);
					added = true;
				}
				if (width > 0)
					PrvViewAddSpaceForText (" ", &width);
				PrvViewAddField(P1Contactscity + (P1Contactsaddress2-P1Contactsaddress)*i, &width, maxWidth, 0);
			}
			if (globals->recordViewRecord.fields[P1ContactszipCode + (P1Contactsaddress2-P1Contactsaddress)*i] ||
				globals->recordViewRecord.fields[P1Contactscity + (P1Contactsaddress2-P1Contactsaddress)*i])
			{
				PrvViewNewLine(&width);
			}
			if (globals->recordViewRecord.fields[P1Contactsstate + (P1Contactsaddress2-P1Contactsaddress)*i])
			{
				if(!added)
				{
					PrvViewAddField(P1ContactsVirtualAddr1Label + i, &width, maxWidth, 0);
					added = true;
				}
				PrvViewAddField(P1Contactsstate + (P1Contactsaddress2-P1Contactsaddress)*i, &width, maxWidth, 0);
				PrvViewNewLine(&width);
			}
		}

		if (globals->recordViewRecord.fields[P1Contactscountry + (P1Contactsaddress2-P1Contactsaddress)*i])
		{
			if(!added)
			{
				PrvViewAddField(P1ContactsVirtualAddr1Label + i, &width, maxWidth, 0);
				added = true;
			}
			PrvViewAddField(P1Contactscountry + (P1Contactsaddress2-P1Contactsaddress)*i, &width, maxWidth, 0);
			PrvViewNewLine(&width);
		}


		PrvViewAddBlankLine(&width);
	
	}
	
	// Do the custom fields
	for (i = P1Contactscustom1; i < P1Contactscustom9 + 1; i++)
	{
		if (globals->recordViewRecord.fields[i])
		{
			PrvViewAddSpaceForText (appInfoPtr->fieldLabels[i], &width);
			PrvViewAddSpaceForText (": ", &width);
			PrvViewPositionTextAt(&width, globals->RecordLabelColumnWidth);
			PrvViewAddField(i, &width, maxWidth, globals->RecordLabelColumnWidth);
			PrvViewNewLine(&width);      // leave a blank line
		}
	}

	if(globals->recordViewRecord.birthdayInfo.birthdayDate.day != 0 && globals->recordViewRecord.birthdayInfo.birthdayDate.month != 0)
	{		
		PrvViewNewLine(&width);
		PrvViewAddField(P1ContactsbirthdayDate, &width, maxWidth, globals->RecordLabelColumnWidth);
		width = 0;
	}
	
	// Show the note field.
	if (globals->recordViewRecord.fields[P1Contactsnote])
	{
		PrvViewNewLine(&width);
		PrvViewAddField(P1Contactsnote, &width, maxWidth, 0);
	}			
#else
	if (globals->recordViewRecord.fields[name] == NULL &&
		globals->recordViewRecord.fields[firstName] == NULL &&
		globals->recordViewRecord.fields[company] != NULL)
	{
		PrvViewAddField(company, &width, maxWidth, 0);
		PrvViewNewLine(&width);
	}
	else
	{
		if (globals->recordViewRecord.fields[firstName] != NULL)
		{
			PrvViewAddField(firstName, &width, maxWidth, 0);
			
			// Separate the last name from the first name as long
			// as they are together on the same line.
			if (width > 0)
				PrvViewAddSpaceForText (" ", &width);
		}
		PrvViewAddField(name, &width, maxWidth, 0);
		PrvViewNewLine(&width);
	}
	globals->RecordViewFirstPlainLine = globals->RecordViewLastLine;
	FntSetFont (globals->AddrRecordFont);

	if (globals->recordViewRecord.fields[title])
	{
		PrvViewAddField(title, &width, maxWidth, 0);
		PrvViewNewLine(&width);
	}
	if (globals->recordViewRecord.fields[company] != NULL &&
		(globals->recordViewRecord.fields[name] != NULL ||
		 globals->recordViewRecord.fields[firstName] != NULL))
	{
		PrvViewAddField(company, &width, maxWidth, 0);
		PrvViewNewLine(&width);
	}
		 
	PrvViewAddBlankLine(&width);

	// Layout the phone numbers.  Start each number on its own line.
	// Put the label first, followed by ": " and then the number
	for (fieldIndex = firstPhoneField; fieldIndex <= lastPhoneField; fieldIndex++)
	{
		if (globals->recordViewRecord.fields[fieldIndex])
		{
			phoneLabelNum = GetPhoneLabel(&globals->recordViewRecord, fieldIndex);
			PrvViewAddSpaceForText (appInfoPtr->fieldLabels[phoneLabelNum + ((phoneLabelNum < numPhoneLabelsStoredFirst) ? firstPhoneField : (addressFieldsCount - numPhoneLabelsStoredFirst))], &width);
			PrvViewAddSpaceForText (": ", &width);
			PrvViewPositionTextAt(&width, globals->RecordLabelColumnWidth);
			PrvViewAddField(fieldIndex, &width, maxWidth, globals->RecordLabelColumnWidth);
			PrvViewNewLine(&width);
		}
	}

	PrvViewAddBlankLine(&width);

	// Now do the address information
	if (globals->recordViewRecord.fields[address])
	{
		PrvViewAddField(address, &width, maxWidth, 0);//RecordLabelColumnWidth);
		PrvViewNewLine(&width);
	}

	// We need to format the city, state, and zip code differently depending
	// on which country it is. For now, assume that if the city comes first,
	// we use the standard US formatting of [city, ][state   ][zip]<cr>,
	// otherwise we'll use the "int'l" format of [zip ][city]<cr>[state]<cr>.
	
	// Decide if we're formatting it US-style, or int'l-style
	if (!eurAddrFormat)
	{
		if (globals->recordViewRecord.fields[city])
		{
			PrvViewAddField(city, &width, maxWidth, 0);
		}
		if (globals->recordViewRecord.fields[state])
		{
			if (width > 0)
				PrvViewAddSpaceForText (", ", &width);
			PrvViewAddField(state, &width, maxWidth, 0);
		}
		if (globals->recordViewRecord.fields[zipCode])
		{
			if (width > 0)
				PrvViewAddSpaceForText ("   ", &width);
			PrvViewAddField(zipCode, &width, maxWidth, 0);
		}
		if (globals->recordViewRecord.fields[city] ||
			globals->recordViewRecord.fields[state] ||
			globals->recordViewRecord.fields[zipCode])
		{
			PrvViewNewLine(&width);
		}
	}
	else
	{
		if (globals->recordViewRecord.fields[zipCode])
		{
			PrvViewAddField(zipCode, &width, maxWidth, 0);
		}
		if (globals->recordViewRecord.fields[city])
		{
			if (width > 0)
				PrvViewAddSpaceForText (" ", &width);
			PrvViewAddField(city, &width, maxWidth, 0);
		}
		if (globals->recordViewRecord.fields[zipCode] ||
			globals->recordViewRecord.fields[city])
		{
			PrvViewNewLine(&width);
		}
		if (globals->recordViewRecord.fields[state])
		{
			PrvViewAddField(state, &width, maxWidth, 0);
			PrvViewNewLine(&width);
		}
	}

	if (globals->recordViewRecord.fields[country])
	{
		PrvViewAddField(country, &width, maxWidth, 0);
		PrvViewNewLine(&width);
	}

	PrvViewAddBlankLine(&width);

	// Do the custom fields
	for (i = custom1; i < addressFieldsCount - 1; i++)
	{
		if (globals->recordViewRecord.fields[i])
		{
			PrvViewAddSpaceForText (appInfoPtr->fieldLabels[i], &width);
			PrvViewAddSpaceForText (": ", &width);
			PrvViewPositionTextAt(&width, globals->RecordLabelColumnWidth);
			PrvViewAddField(i, &width, maxWidth, globals->RecordLabelColumnWidth);
			PrvViewNewLine(&width);      // leave a blank line
		}
	}

	//show the bitrhday. FIrst, look for it.
	addrXTDBSize = DmNumRecords(addrXTDB);
	for(counter=0;counter<addrXTDBSize;counter++)
	{
		AddrXTDBRecordType rec;
		MemHandle mH;
		UInt32 recId, recSeconds;
		
		if(AddrXTDBGetRecord(globals->adxtLibRef, addrXTDB, counter, &rec, &mH) != 0)
			continue;
			
		recId = rec.id;
		recSeconds = rec.bday;
		
		MemHandleUnlock(mH);
		
		if(id == recId)
		{
			UInt16 d, m, y;
			DateTimeType date;
			DateFormatType dateFormat;
			PrvViewNewLine(&width);
			width = 0;
			PrvViewAddField(birthday, &width, maxWidth, globals->RecordLabelColumnWidth);
			PrvViewNewLine(&width);
			TimSecondsToDateTime(recSeconds, &date);
			d=date.day;
			m=date.month;
			y=date.year;
			dateFormat=PrefGetPreference(prefLongDateFormat);
			DateToAscii(m, d, y, dateFormat, globals->Birthday);
			break;
		}		
	}	
	
	// Show the note field.
	if (globals->recordViewRecord.fields[note])
	{
		PrvViewNewLine(&width);
		PrvViewAddField(note, &width, maxWidth, 0);
	}			
#endif	
	// Now remove trailing blank lines
	while (globals->RecordViewLastLine > 0 &&
		   globals->RecordViewLines[globals->RecordViewLastLine - 1].fieldNum == recordViewBlankLine)
	{
		globals->RecordViewLastLine--;
	}

	MemPtrUnlock(appInfoPtr);
	FntSetFont (curFont);
	DmCloseDatabase(addrXTDB);
	MemHandleUnlock(globals->recordViewRecordH);
	globals->recordViewRecordH = NULL;
}


/***********************************************************************
 *
 * FUNCTION:    PrvViewNewLine
 *
 * DESCRIPTION: Adds the next field at the start of a new line
 *
 * PARAMETERS:  width - width already occupied on the line
 *
 * RETURNED:    width is set
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/21/95   Initial Revision
 *
 ***********************************************************************/
void PrvViewNewLine (UInt16 *width)
{
	globalVars* globals = getGlobalsPtr();
	if (globals->RecordViewLastLine >= recordViewLinesMax)
		return;

	if (*width == 0)
	{
		if(globals->RecordViewLastLine > 0)
		{
			if(globals->RecordViewLines[globals->RecordViewLastLine-1].fieldNum == recordViewBlankLine)
				return; 
		}
		globals->RecordViewLines[globals->RecordViewLastLine].fieldNum = recordViewBlankLine;
		globals->RecordViewLines[globals->RecordViewLastLine].x = 0;
		globals->RecordViewLines[globals->RecordViewLastLine].offset = 0;
		globals->RecordViewLastLine++;
	}
	else
		*width = 0;
}


/***********************************************************************
 *
 * FUNCTION:    PrvViewAddSpaceForText
 *
 * DESCRIPTION: Adds space for text to the RecordViewLines info.
 *
 * PARAMETERS:  string - Char * to text to leave space for
 *                width - width already occupied on the line
 *
 * RETURNED:    width is increased by the width of the text
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/21/95   Initial Revision
 *
 ***********************************************************************/
void PrvViewAddSpaceForText (const Char * const string, UInt16 *width)
{
	*width += FntCharsWidth(string, StrLen(string));
}


/***********************************************************************
 *
 * FUNCTION:    PrvViewPositionTextAt
 *
 * DESCRIPTION: Position the following text at the given position.
 *
 * PARAMETERS:  position - position to indent to
 *                width - width already occupied on the line
 *
 * RETURNED:    width is increased if the position is greater
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   8/2/95   Initial Revision
 *
 ***********************************************************************/
void PrvViewPositionTextAt (UInt16 *width, const UInt16 position)
{
	if (*width < position)
		*width = position;
}


/***********************************************************************
 *
 * FUNCTION:    PrvViewAddField
 *
 * DESCRIPTION: Adds a field to the RecordViewLines info.
 *
 * PARAMETERS:  fieldNum - field to add
 *                width - width already occupied on the line
 *                maxWidth - can't add words past this width
 *                indentation - the amounnt of indentation wrapped lines of
 *                              text should begin with (except the last)
 *
 * RETURNED:    width is set to the width of the last line added
 *
 * HISTORY:
 *		06/21/95	rsf	Created by Roger Flores
 *		10/25/99	kwk	Fix sign extension w/calling TxtCharIsSpace
 *
 ***********************************************************************/
void PrvViewAddField (const UInt16 fieldNum, UInt16 *width, const UInt16 maxWidth, const UInt16 indentation)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 length;
	UInt16 offset = 0;
	UInt16 newOffset;


#ifdef CONTACTS
		if
		(
			(
				fieldNum != P1ContactsVirtualAddr1Label && fieldNum != P1ContactsVirtualAddr2Label && fieldNum != P1ContactsVirtualAddr3Label
			)
#else
		if
		(
			(
				fieldNum != birthday
			)
#endif
			&&			
			(	
				globals->recordViewRecord.fields[fieldNum] == NULL ||
				globals->RecordViewLastLine >= recordViewLinesMax
			)
		)
			return;

	if (*width >= maxWidth)
		*width = indentation;

	do
	{
		if (globals->RecordViewLastLine >= recordViewLinesMax)
			break;
		
#ifdef CONTACTS
		if(fieldNum == P1ContactsbirthdayDate || fieldNum == P1ContactsVirtualAddr1Label || fieldNum == P1ContactsVirtualAddr2Label || fieldNum == P1ContactsVirtualAddr3Label)
#else
		if(fieldNum == birthday)
#endif
		{
			*width = indentation;
		}
		else
		{
			length = FldWordWrap(&globals->recordViewRecord.fields[fieldNum][offset], maxWidth - *width);
			if (globals->recordViewRecord.fields[fieldNum][offset + length] != '\0'
				&& !TxtCharIsSpace((UInt8)globals->recordViewRecord.fields[fieldNum][offset + length - 1])
				&& (*width > indentation))
			{
				length = 0;            // don't word wrap - try next line
			}
		}			
		
		newOffset = offset + length;
		
#ifdef CONTACTS
		if(fieldNum != P1ContactsbirthdayDate && fieldNum != P1ContactsVirtualAddr1Label && fieldNum != P1ContactsVirtualAddr2Label && fieldNum != P1ContactsVirtualAddr3Label)
#else
		if(fieldNum != birthday)
#endif
		{
			if (newOffset > 0 && globals->recordViewRecord.fields[fieldNum][newOffset - 1] == linefeedChr)
				length--;
		}
		globals->RecordViewLines[globals->RecordViewLastLine].fieldNum = fieldNum;
		globals->RecordViewLines[globals->RecordViewLastLine].offset = offset;
		globals->RecordViewLines[globals->RecordViewLastLine].x = *width;
		globals->RecordViewLines[globals->RecordViewLastLine].length = length;
		globals->RecordViewLines[globals->RecordViewLastLine].empty = false;
		globals->RecordViewLastLine++;
		offset = newOffset;
#ifdef CONTACTS
		if(fieldNum != P1ContactsbirthdayDate && fieldNum != P1ContactsVirtualAddr1Label && fieldNum != P1ContactsVirtualAddr2Label && fieldNum != P1ContactsVirtualAddr3Label)
#else
		if(fieldNum != birthday)
#endif
		{
			if (globals->recordViewRecord.fields[fieldNum][offset] != '\0')
				*width = indentation;
			else
				break;
		}
		else
			break;
	} while(true);
	
#ifdef CONTACTS
	if(fieldNum != P1ContactsbirthdayDate && fieldNum != P1ContactsVirtualAddr1Label && fieldNum != P1ContactsVirtualAddr2Label && fieldNum != P1ContactsVirtualAddr3Label)
#else
	if(fieldNum != birthday)
#endif
	{
		if (globals->recordViewRecord.fields[fieldNum][offset - 1] == linefeedChr)
			*width = 0;
		else
			*width += FntCharsWidth(&globals->recordViewRecord.fields[fieldNum][globals->RecordViewLines[globals->RecordViewLastLine - 1].offset], globals->RecordViewLines[globals->RecordViewLastLine - 1].length);
	}
}

/***********************************************************************
 *
 * FUNCTION:    PrvViewErase
 *
 * DESCRIPTION: Erases the record view
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/30/95   Initial Revision
 *
 ***********************************************************************/
void PrvViewErase( FormType* frmP )
{
	RectangleType r;

#ifdef DEBUG
	LogWrite("xt_log", "view", "PrvViewErase");
#endif
	
	FrmGetObjectBounds(frmP, FrmGetObjectIndex(frmP, RecordViewDisplay), &r);
	WinEraseRectangle (&r, 0);
}

/***********************************************************************
 *
 * FUNCTION:    PrvViewCalcNextLine
 *
 * DESCRIPTION: This routine returns the amount of extra vertical space
 *					 required due to the item at this index in the lines array.
 *					 If there are multiple items in the line array which go on
 *					 the same y coordinate, the first is the one which
 *					 contributes the vertical space requirement. Text which
 *					 begins to the left of text on the previous line starts a
 *					 new line. Blank lines use only a half line to save space.
 *
 * PARAMETERS:  i - the line to base how far to advance
 *                oneLine - the amount which advance one line down.
 *
 * RETURNED:    the amount to advance.  Typically oneLine or oneLine / 2.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   9/28/95   Initial Revision
 *			  peter	10/06/00	  Replaced description above.
 *
 ***********************************************************************/
UInt16 PrvViewCalcNextLine(UInt16 i, UInt16 oneLine, Boolean recursive)
{
	UInt16 res;
	Boolean hasImage = false;
	Int16 lineToExtend = -1, heightToExtend = 0, currentHeight = 0;
#ifdef CONTACTS
	UInt16 j;
#endif
	globalVars* globals = getGlobalsPtr();
	
	if(!recursive)
	{
#ifdef CONTACTS
		if(pnoJpegRefNum != sysInvalidRefNum )
		{
			if(globals->recordViewRecord.pictureInfo.pictureSize > 0)
			{
				hasImage = true;
			}
		}
		
		lineToExtend = -1;
		heightToExtend = 0;
		if(globals->TopRecordViewLine == 0 && hasImage)//image should be visible
		{
			for (j = globals->RecordViewLastLine-1; j >= globals->TopRecordViewLine; j--)
			{
				if(j == -1)
					break;
				switch (globals->RecordViewLines[j].fieldNum)
					{
						case P1Contactsname:
						case P1ContactsfirstName:
						case P1Contactscompany:
						case P1Contactstitle:
							currentHeight = PrvViewCalcNextLine(j, FntLineHeight(), true);
							heightToExtend += currentHeight;
							if(lineToExtend == -1)
							{
								if(currentHeight > 0)
									lineToExtend = j;
							}
							break;
						default:
							break;
					}
						
			}	
		}	
		if(heightToExtend < globals->ImageHeight - 1)
			heightToExtend = globals->ImageHeight - 1 - heightToExtend;
		else
			heightToExtend = 0;			
#endif
	}
		
	// Advance down if the text starts before the text of the current line.
	if (globals->RecordViewLines[i].x == 0 ||
	   (i > 0 &&
	   (globals->RecordViewLines[i].x <= globals->RecordViewLines[i - 1].x || globals->RecordViewLines[i - 1].fieldNum == recordViewBlankLine)))
	{
		// A non blank line moves down a full line.
		if (globals->RecordViewLines[i].fieldNum != recordViewBlankLine)
		{
			res = oneLine;
		}
		else
		{
			// A recordViewBlankLine is half-height.
			res = oneLine / 2;
		}
		
		if(i == lineToExtend && hasImage)
		{
			res += heightToExtend;
		}
		return res;
	}
	return 0;      // Stay on the same line.
}


/***********************************************************************
 *
 * FUNCTION:    PrvViewDrawSelectedText
 *
 * DESCRIPTION: Inverts text which is considered selected.
 *
 * PARAMETERS:  currentField - field containing the selected text
 *              selectPos 	  - offset into field for start of selected text
 *              selectLen 	  - length of selected text.  This field
 *                  			    should be zero if selected text isn't desired.
 *              textY 		  - where on the screen the text was drawn
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  11/27/95  Cut from PrvViewDraw
 *         jmp    04-21-00  Fixed bug #23860:  This routine was calling
 *                          WInvertRectangle(), which produced somewhat unpredicatable
 *                          results in a color environment.  Changed the routine
 *                          to call WinDrawInvertChars() instead, which swaps
 *                          the foreground and background colors appropriately.
 *			  peter	05/05/00	 Remove code that tried but failed to extend selection rectangle 1 pixel.
 *									 This code could be fixed, but we'd also need to erase the extra pixel.
 *			  peter	05/17/00	 Change to display text with selected object colors.
 *
 ***********************************************************************/
void PrvViewDrawSelectedText (UInt16 currentField, UInt16 selectPos, UInt16 selectLen, UInt16 textY)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 selectXLeft = 0;
	UInt16 selectXRight = 0;
	
	// If the start of the selected region is on this line, calc an x.
	if ( (globals->RecordViewLines[currentField].offset <= selectPos) && (selectPos < globals->RecordViewLines[currentField].offset + globals->RecordViewLines[currentField].length) )
	{
		selectXLeft = FntCharsWidth(&globals->recordViewRecord.fields[globals->RecordViewLines[currentField].fieldNum][globals->RecordViewLines[currentField].offset], selectPos - globals->RecordViewLines[currentField].offset);
	}
	// If the end of the selected region is on this line, calc an x.
	if ( (globals->RecordViewLines[currentField].offset <= selectPos + selectLen) &&	(selectPos + selectLen <= globals->RecordViewLines[currentField].offset + globals->RecordViewLines[currentField].length))
	{
		selectXRight = FntCharsWidth(&globals->recordViewRecord.fields[globals->RecordViewLines[currentField].fieldNum][globals->RecordViewLines[currentField].offset], selectPos + selectLen - globals->RecordViewLines[currentField].offset);
	}

	// If either the left or right have been set then some
	// text needs to be selected.
	if (selectXLeft | selectXRight)
	{
		// Switch to selected object colors.
		WinPushDrawState();
		WinSetBackColor(UIColorGetTableEntryIndex(UIObjectSelectedFill));
		WinSetForeColor(UIColorGetTableEntryIndex(UIObjectSelectedForeground));
		WinSetTextColor(UIColorGetTableEntryIndex(UIObjectSelectedForeground));

		// Draw the text with the selection colors.
		WinDrawChars(&globals->recordViewRecord.fields[globals->RecordViewLines[currentField].fieldNum][selectPos], selectLen, selectXLeft += globals->RecordViewLines[currentField].x, textY);
		// Restore non-selected object colors.
		WinPopDrawState();
	}
}

static void DrawCurrentLine(UInt16 i, UInt16 y)
{
	globalVars* globals = getGlobalsPtr();
	WinDrawChars(&globals->recordViewRecord.fields[globals->RecordViewLines[i].fieldNum][globals->RecordViewLines[i].offset], globals->RecordViewLines[i].length, globals->RecordViewLines[i].x, y);						
}
/***********************************************************************
 *
 * FUNCTION:    PrvViewDraw
 *
 * DESCRIPTION: This routine initializes the "Record View"
 *
 * PARAMETERS:  selectFieldNum - field to show selected text
 *                selectPos - offset into field for start of selected text
 *                selectLen - length of selected text.  This field
 *                  should be zero if selected text isn't desired.
 *						drawOnlySelectField	- whether one or all fields are drawn
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * HISTORY:
 *		06/21/95	rsf		Created by Roger Flores
 *		02/06/98	tlw		Change test below "If we are past bottom stop drawing" from
 *							 if >= bottomOfRecordViewDisplay - FntLineHeight() to
 *							 if > bottomOfRecordViewDisplay - FntLineHeight()
 *							 to allow last line to be drawn.
 *		07/29/99	kwk		When drawing zip code, load prefix (# of spaces) from resource.
 *		05/17/00	peter	Explicitly set colors to draw in.
 *		09/25/00	aro		Adding frmP as an argument for the frmUpdateEvent
 *		10/06/00	peter & danny	When first line is blank, leave the 1/2 line gap used for blank lines.
 *		11/28/00	FPa		Fixed bug #45991
 *		11/29/00	FPa		Fixed bug #46272
 *
 ***********************************************************************/
void PrvViewDraw ( FormType* frmP, UInt16 selectFieldNum, UInt16 selectPos, UInt16 selectLen, Boolean drawOnlySelectField )
{
	globalVars* globals = getGlobalsPtr();
	univAppInfoPtr appInfoPtr;
	UInt16 y;
	UInt16 previousNonZeroHeight;
	UInt16 currentHeight;
	Int16 i;
	FontID curFont;
	UInt16 phoneLabelNum;
	Char * fieldLabelString;
	UInt16 fieldLabelLength;
	UInt16 upIndex;
	UInt16 downIndex;
	Boolean scrollableUp;
	Boolean scrollableDown;
	RectangleType r;
#ifndef CONTACTS
	Char bdaylabel[31];
#endif
	UInt8 adt = 0;
   	Int16 fieldLabelWidth;
	Int16 fieldLabelCompleteWidth;
	int bottomOfRecordViewDisplay;
	Boolean hasImage = false;
	UInt16 width;
#ifdef CONTACTS
	Char birthdayStr[31];
#endif

#ifdef DEBUG
	LogWrite("xt_log", "view", "PrvViewDraw");
#endif

	appInfoPtr = univAddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	
	FrmGetObjectBounds(frmP, FrmGetObjectIndex(frmP, RecordViewDisplay), &r);
	bottomOfRecordViewDisplay = r.topLeft.y +  r.extent.y;

		
	// Set the background color in control-style colors since the text is selectable.
	
	WinPushDrawState();
	WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetTableEntryIndex(UIObjectForeground));

	if (globals->TopRecordViewLine < globals->RecordViewFirstPlainLine)
		curFont = FntSetFont (largeBoldFont);
	else
		curFont = FntSetFont (globals->AddrRecordFont);

	y = r.topLeft.y;
	previousNonZeroHeight = 0;

#ifdef CONTACTS
	if(pnoJpegRefNum != sysInvalidRefNum )
	{
		if(globals->recordViewRecord.pictureInfo.pictureSize > 0)
		{
			hasImage = true;
		}
	}
#endif
	
	for (i = globals->TopRecordViewLine; i < globals->RecordViewLastLine; i++)
	{
		// Switch fonts if necessary before computing the extra vertical
		// space needed for this element of the array.
		if (i == globals->RecordViewFirstPlainLine)
			FntSetFont (globals->AddrRecordFont);
		currentHeight = PrvViewCalcNextLine(i, FntLineHeight(), false);
		
		// Since the above function returns zero for all but the first
		// item when several should be drawn at the same y coordinate,
		// we need to delay adding the result until we get to the next
		// non-zero result.
		if (currentHeight != 0)
		{
			y += previousNonZeroHeight;
			previousNonZeroHeight = currentHeight;
		}

		// If we are past the bottom stop drawing
		if (y > bottomOfRecordViewDisplay - FntLineHeight())
			break;

		ErrNonFatalDisplayIf(y < r.topLeft.y, "Drawing record out of gadget");

	
		if (!drawOnlySelectField || globals->RecordViewLines[i].fieldNum == selectFieldNum)
		{
			if (globals->RecordViewLines[i].offset == 0)
			{
#ifdef CONTACTS
				switch (globals->RecordViewLines[i].fieldNum)
				{
					case P1Contactsname:
					case P1ContactsfirstName:
					case P1Contactscompany:
					case P1Contactstitle:
						if(i == globals->TopRecordViewLine && hasImage && i == 0)
						{
							if(globals->recordViewRecord.pictureInfo.pictureSize > 0)
							{
								UInt16 maxWidth;
								FrmGetFormBounds(frmP, &r);
								maxWidth = r.extent.x;
								//ImageLibDrawJPEG(globals->imageLibRef, globals->recordViewRecord.pictureInfo.pictureData, globals->recordViewRecord.pictureInfo.pictureSize, maxWidth + 2 -globals->ImageOffset, 17, false, 64, 64); 
								LoadJpegFromPtr(globals->recordViewRecord.pictureInfo.pictureData, globals->recordViewRecord.pictureInfo.pictureSize, false, 0, 0, maxWidth + 2 -globals->ImageOffset, 17);
							}
						}
					if(!globals->RecordViewLines[i].empty)
						DrawCurrentLine(i, y);
					break;
					case recordViewBlankLine:
						width=GetWindowWidth();
						WinPushDrawState();
						WinSetPatternType(grayPattern);
						WinPaintLine(0, y+3, width, y+3);
						WinPopDrawState();	
						break;
					case P1Contactsphone1:
					case P1Contactsphone2:
					case P1Contactsphone3:
					case P1Contactsphone4:
					case P1Contactsphone5:
					case P1Contactsphone6:
					case P1Contactsphone7:
						phoneLabelNum = P1ContactsGetPhoneLabel(&globals->recordViewRecord, globals->RecordViewLines[i].fieldNum);
						fieldLabelString = appInfoPtr->fieldLabels[phoneLabelNum +
																   ((phoneLabelNum < 7) ? P1ContactsfirstPhoneField : (P1ContactsfirstPhoneField + 29))];
						fieldLabelLength = StrLen(fieldLabelString);
						WinDrawChars(fieldLabelString, fieldLabelLength, 0, y);
						WinDrawChars(": ", 2, FntCharsWidth(fieldLabelString, fieldLabelLength), y);
	
						DrawCurrentLine(i, y);
						break;
					case P1Contactschat1:
					case P1Contactschat2:
						phoneLabelNum = GetChatLabel(&globals->recordViewRecord, globals->RecordViewLines[i].fieldNum);
						fieldLabelString = appInfoPtr->fieldLabels[phoneLabelNum + 41];
						fieldLabelLength = StrLen(fieldLabelString);
						WinDrawChars(fieldLabelString, fieldLabelLength, 0, y);
						WinDrawChars(": ", 2, FntCharsWidth(fieldLabelString, fieldLabelLength), y);
	
						DrawCurrentLine(i, y);
						break;
					case P1Contactswebpage:
						fieldLabelString = appInfoPtr->fieldLabels[P1Contactswebpage];
						fieldLabelLength = StrLen(fieldLabelString);
						WinDrawChars(fieldLabelString, fieldLabelLength, 0, y);
						WinDrawChars(": ", 2, FntCharsWidth(fieldLabelString, fieldLabelLength), y);
	
						DrawCurrentLine(i, y);
						break;
					case P1Contactscustom1:
					case P1Contactscustom2:
					case P1Contactscustom3:
					case P1Contactscustom4:
					case P1Contactscustom5:
					case P1Contactscustom6:
					case P1Contactscustom7:
					case P1Contactscustom8:
					case P1Contactscustom9:
					{
						
						fieldLabelString = appInfoPtr->fieldLabels[globals->RecordViewLines[i].fieldNum];
						fieldLabelLength = StrLen(fieldLabelString);
						fieldLabelWidth = FntCharsWidth(fieldLabelString, fieldLabelLength);
						fieldLabelCompleteWidth = fieldLabelWidth + FntCharsWidth(": ", 2);
								
						if (globals->RecordViewLines[i].length == 0 ||	// If the custom label is displayed on a line and its content on the next line (because the custom label has been renamed using a long name and because it content is multi-line)
						    fieldLabelCompleteWidth <= globals->RecordViewLines[i].x)	// If the label name is not too width, then we display the label name and its content on the same line
						{
							WinDrawChars(fieldLabelString, fieldLabelLength, 0, y);
							WinDrawChars(": ", 2, fieldLabelWidth, y);
						}
						DrawCurrentLine(i, y);
						break;
					}
	
					case P1Contactsstate:
					case P1Contactsstate2:
					case P1Contactsstate3:
						if (globals->RecordViewLines[i].x > 0)
							WinDrawChars(", ", 2, globals->RecordViewLines[i].x - FntCharsWidth(", ", 2), y);
						DrawCurrentLine(i, y);
						break;
	
					case P1ContactszipCode:
					case P1ContactszipCode2:
					case P1ContactszipCode3:
						if (globals->RecordViewLines[i].x > 0)
						{
							const Char* textP;
							MemHandle zipCodePrefixH;
							
							zipCodePrefixH = DmGetResource(strRsc, ZipCodePrefixStr);
							textP = (const Char*)MemHandleLock(zipCodePrefixH);
							WinDrawChars(textP, StrLen(textP), globals->RecordViewLines[i].x - FntCharsWidth(textP, StrLen(textP)), y);
							MemPtrUnlock((MemPtr)textP);
							DmReleaseResource(zipCodePrefixH);
						}
	
						DrawCurrentLine(i, y);
						break;
					case P1ContactsbirthdayDate:
							fieldLabelString = appInfoPtr->fieldLabels[P1ContactsbirthdayDate];
							fieldLabelLength = StrLen(fieldLabelString);
							WinDrawChars(fieldLabelString, fieldLabelLength, 0, y);
							WinDrawChars(": ", 2, FntCharsWidth(fieldLabelString, fieldLabelLength), y);
							{
								DateFormatType dateFormat;
								DateType bDay = globals->recordViewRecord.birthdayInfo.birthdayDate;
								dateFormat=PrefGetPreference(prefLongDateFormat);
								DateToAscii(bDay.month, bDay.day, bDay.year + 1904, dateFormat, birthdayStr);
								WinDrawChars(birthdayStr, StrLen(birthdayStr), globals->RecordViewLines[i].x, y);
							}
							break;
					case P1ContactsVirtualAddr1Label:
					case P1ContactsVirtualAddr2Label:
					case P1ContactsVirtualAddr3Label:
						switch (globals->RecordViewLines[i].fieldNum)
						{
							case P1ContactsVirtualAddr1Label:
								phoneLabelNum = GetAddressLabel(&globals->recordViewRecord, P1Contactsaddress);
								break;
							case P1ContactsVirtualAddr2Label:
								phoneLabelNum = GetAddressLabel(&globals->recordViewRecord, P1Contactsaddress2);
								break;
							case P1ContactsVirtualAddr3Label:
								phoneLabelNum = GetAddressLabel(&globals->recordViewRecord, P1Contactsaddress3);
								break;		
						}
						switch(phoneLabelNum)
						{
							case 0:
								fieldLabelString = appInfoPtr->fieldLabels[4];
								break;
							case 1:
								fieldLabelString = appInfoPtr->fieldLabels[5];
								break;
							case 2:
								fieldLabelString = appInfoPtr->fieldLabels[7];
								break;							
						}
						fieldLabelLength = StrLen(fieldLabelString);
						WinDrawChars(fieldLabelString, fieldLabelLength, 0, y);
						WinDrawChars(": ", 2, FntCharsWidth(fieldLabelString, fieldLabelLength), y);
						break;	
					default:
						DrawCurrentLine(i, y);
						break;
					}
#else
					switch (globals->RecordViewLines[i].fieldNum)
					{
						case name:
						case firstName:
						case company:
						case title:
							if(!globals->RecordViewLines[i].empty)
								DrawCurrentLine(i, y);
							break;
						case recordViewBlankLine:
							width=GetWindowWidth();
							WinPushDrawState();
							WinSetPatternType(grayPattern);
							WinPaintLine(0, y+3, width, y+3);
							WinPopDrawState();	
							break;
						case phone1:
						case phone2:
						case phone3:
						case phone4:
						case phone5:
							phoneLabelNum = GetPhoneLabel(&globals->recordViewRecord, globals->RecordViewLines[i].fieldNum);
							fieldLabelString = appInfoPtr->fieldLabels[phoneLabelNum +
																	   ((phoneLabelNum < numPhoneLabelsStoredFirst) ? firstPhoneField : (addressFieldsCount - numPhoneLabelsStoredFirst))];
							fieldLabelLength = StrLen(fieldLabelString);
							WinDrawChars(fieldLabelString, fieldLabelLength, 0, y);
							WinDrawChars(": ", 2, FntCharsWidth(fieldLabelString, fieldLabelLength), y);
		
							DrawCurrentLine(i, y);
						break;
						case custom1:
						case custom2:
						case custom3:
						case custom4:
						{
							
							fieldLabelString = appInfoPtr->fieldLabels[globals->RecordViewLines[i].fieldNum];
							fieldLabelLength = StrLen(fieldLabelString);
							fieldLabelWidth = FntCharsWidth(fieldLabelString, fieldLabelLength);
							fieldLabelCompleteWidth = fieldLabelWidth + FntCharsWidth(": ", 2);
									
							if (globals->RecordViewLines[i].length == 0 ||	// If the custom label is displayed on a line and its content on the next line (because the custom label has been renamed using a long name and because it content is multi-line)
							    fieldLabelCompleteWidth <= globals->RecordViewLines[i].x)	// If the label name is not too width, then we display the label name and its content on the same line
							{
								WinDrawChars(fieldLabelString, fieldLabelLength, 0, y);
								WinDrawChars(": ", 2, fieldLabelWidth, y);
							}
							DrawCurrentLine(i, y);
							break;
						}
		
						case state:
							if (globals->RecordViewLines[i].x > 0)
								WinDrawChars(", ", 2, globals->RecordViewLines[i].x - FntCharsWidth(", ", 2), y);
							DrawCurrentLine(i, y);
							break;
		
						case zipCode:
							if (globals->RecordViewLines[i].x > 0)
							{
								const Char* textP;
								MemHandle zipCodePrefixH;
								
								zipCodePrefixH = DmGetResource(strRsc, ZipCodePrefixStr);
								textP = (const Char*)MemHandleLock(zipCodePrefixH);
								WinDrawChars(textP, StrLen(textP), globals->RecordViewLines[i].x - FntCharsWidth(textP, StrLen(textP)), y);
								MemPtrUnlock((MemPtr)textP);
								DmReleaseResource(zipCodePrefixH);
							}			
							DrawCurrentLine(i, y);
							break;
						case birthday:
							StrCopy(bdaylabel, "Birthday");
							fieldLabelLength = StrLen(bdaylabel);
							WinDrawChars(bdaylabel, fieldLabelLength, 0, y);
							WinDrawChars(": ", 2, FntCharsWidth(bdaylabel, fieldLabelLength), y);
		
							WinDrawChars(globals->Birthday, StrLen(globals->Birthday), globals->RecordViewLines[i].x, y);
							
							break;
						default:
							DrawCurrentLine(i, y);
							break;
						}
#endif
			}
			else
			{
				// Draw the remainder of the fields' lines without any
				// other special handling.
				if (globals->RecordViewLines[i].fieldNum != recordViewBlankLine && globals->RecordViewLines[i].fieldNum != P1ContactsbirthdayDate)
				{
					DrawCurrentLine(i, y);
				}
			}


			// Highlight text if it is within the selection bounds.  This is
			// used to select found text and phone numbers when the user taps on them.
			if ( (globals->RecordViewLines[i].fieldNum == selectFieldNum) && (selectLen > 0) )
			{
				if ( selectPos < globals->RecordViewLines[i].offset + globals->RecordViewLines[i].length )	// If there's a selection to draw on this line (if the beginning of the selection is on this line or one of the previous lines)
				{
					UInt16 pos;
					UInt16 len;
					UInt16 posOffsetFromBeginningOfLine;

					if ( selectPos >= globals->RecordViewLines[i].offset )	// If the beginning of the selection is within the current line
					{
						pos = selectPos;
						posOffsetFromBeginningOfLine = selectPos - globals->RecordViewLines[i].offset;
					}
					else	// The beginning of the selection is within one of the previous lines
					{
						pos = globals->RecordViewLines[i].offset;
						posOffsetFromBeginningOfLine = 0;
					}
					
					if ( selectPos + selectLen < globals->RecordViewLines[i].offset + globals->RecordViewLines[i].length )	// If the end of the selection is within the current line or within one of the previous lines
					{
						if ( selectPos + selectLen >=  globals->RecordViewLines[i].offset + posOffsetFromBeginningOfLine )	// <=> selectPos + selectLen - RecordViewLines[i].offset - posOffsetFromBeginningOfLine (Not to have len < 0)
							len = selectPos + selectLen - globals->RecordViewLines[i].offset - posOffsetFromBeginningOfLine;
						else
							len = 0;
					}
					else	// The end of the selection is within one of the next lines
					{
						len = globals->RecordViewLines[i].length - posOffsetFromBeginningOfLine;
					}
					
					PrvViewDrawSelectedText(i, pos, len, y);
				}
			}
		}
	}

	FntSetFont (curFont);
	WinPopDrawState ();
	MemPtrUnlock(appInfoPtr);
	
	if (!drawOnlySelectField)
	{
		// Now show/hide the scroll arrows
		scrollableUp = globals->TopRecordViewLine != 0;
		scrollableDown = i < globals->RecordViewLastLine;
		
		// Update the scroll button.
		upIndex = FrmGetObjectIndex(frmP, RecordUpButton);
		downIndex = FrmGetObjectIndex(frmP, RecordDownButton);
		FrmUpdateScrollers(frmP, upIndex, downIndex, scrollableUp, scrollableDown);
	}
}

/***********************************************************************
 *
 * FUNCTION:    PrvViewDrawBusinessCardIndicator
 *
 * DESCRIPTION: Draw the business card indicator if the current record is
 * the business card.
 *
 * PARAMETERS:  formP - the form containing the business card indicator
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  10/22/97  Initial Revision
 *
 ***********************************************************************/
void PrvViewDrawBusinessCardIndicator (FormPtr formP)
{
	globalVars* globals = getGlobalsPtr();
	UInt32 uniqueID;

	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &uniqueID, NULL);
	if (globals->BusinessCardRecordID == uniqueID)
		FrmShowObject(formP, FrmGetObjectIndex (formP, RecordViewBusinessCardBmp));
	else
		FrmHideObject(formP, FrmGetObjectIndex (formP, RecordViewBusinessCardBmp));
}

/***********************************************************************
 *
 * FUNCTION:    PrvViewUpdate
 *
 * DESCRIPTION: Update the record view and redraw it.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *		Name   Date      	Description
 *		----   ----      	-----------
 *		roger   10/18/95  	Initial Revision
 *		aro		09/26/00	Add frmP as an argument
 *		FPa		11/15/00	Fixed bug #44838
 *
 ***********************************************************************/
void PrvViewUpdate( FormType* frmP )
{
#ifdef DEBUG
	LogWrite("xt_log", "view", "PrvViewUpdate");
#endif
	PrvViewErase(frmP);
	PrvViewDraw(frmP, 0, 0, 0, false);
	PrvViewDrawBusinessCardIndicator(frmP);
}

/***********************************************************************
 *
 * FUNCTION:    PrvViewScrollOnePage
 *
 * DESCRIPTION: Scrolls the record view by one page less one line unless
 * we scroll from RecordViewLastLine (used by scroll code).
 *
 * PARAMETERS:  newTopRecordViewLine - top line of the display
 *              direction - up or dowm
 *
 * RETURNED:    new newTopRecordViewLine one page away
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *			roger		6/22/95	Initial Revision
 *			roger		8/2/95	Reworked to handle half height blank lines.
 *			roger		10/30/95	Reworked to obey FntLineHeight
 *			roger		10/31/95	Broke out of PrvViewScroll
 *
 ***********************************************************************/
UInt16 PrvViewScrollOnePage (Int16 newTopRecordViewLine, WinDirectionType direction)
{
	globalVars* globals = getGlobalsPtr();
	Int16 offset;
	FontID curFont;
	FormPtr frm;
	Int16 largeFontLineHeight;
	Int16 stdFontLineHeight;
	Int16 currentLineHeight;
	RectangleType r;
	Int16 recordViewDisplayHeight;


	// setup stuff
	curFont = FntSetFont (largeBoldFont);
	largeFontLineHeight = FntLineHeight();
	FntSetFont (globals->AddrRecordFont);
	stdFontLineHeight = FntLineHeight();
	FntSetFont (curFont);

	frm = FrmGetActiveForm();
	FrmGetObjectBounds(frm, FrmGetObjectIndex(frm, RecordViewDisplay), &r);
	recordViewDisplayHeight = r.extent.y;
	if (newTopRecordViewLine != globals->RecordViewLastLine)
		recordViewDisplayHeight -= stdFontLineHeight;   // less one one line


	if (direction == winUp)
		offset = -1;
	else
		offset = 1;


	while (recordViewDisplayHeight >= 0 &&
		   (newTopRecordViewLine > 0 || direction == winDown) &&
		   (newTopRecordViewLine < (globals->RecordViewLastLine - 1) || direction == winUp))
	{
		newTopRecordViewLine += offset;
		if (newTopRecordViewLine < globals->RecordViewFirstPlainLine)
			currentLineHeight = largeFontLineHeight;
		else
			currentLineHeight = stdFontLineHeight;

		recordViewDisplayHeight -= PrvViewCalcNextLine(newTopRecordViewLine,
													   currentLineHeight, false);
	};
	
	
	// Did we go too far?
	if (recordViewDisplayHeight < 0)
	{
		// The last line was too much so remove it
		newTopRecordViewLine -= offset;

		// Also remove any lines which don't have a height
		while (PrvViewCalcNextLine(newTopRecordViewLine, 2, false) == 0)
		{
			newTopRecordViewLine -= offset;   // skip it
		}
	}

	return newTopRecordViewLine;
}


/***********************************************************************
 *
 * FUNCTION:    PrvViewScroll
 *
 * DESCRIPTION: Scrolls the record view
 *
 * PARAMETERS:  direction - up or dowm
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	06/22/95	Initial Revision
 *			roger	08/02/95	Reworked to handle half height blank lines.
 *			roger	10/30/95	Reworked to obey FntLineHeight
 *			gap		10/12/99	Close command bar before processing scroll
 *			FPa		11/15/00	Fixed bug #44838
 *
 ***********************************************************************/
void PrvViewScroll (WinDirectionType direction)
{
	globalVars* globals = getGlobalsPtr();
	Int16 lastRecordViewLine;
	UInt16 newTopRecordViewLine;
	UInt16 category;
	UInt16 recordNum;
	Int16 seekDirection;
	UInt16	attr;
	FormType* frmP;
	// Before processing the scroll, be sure that the command bar has been closed.
	MenuEraseStatus (0);
	newTopRecordViewLine = globals->TopRecordViewLine;
	
	if (direction == winUp)
	{
		newTopRecordViewLine = PrvViewScrollOnePage (newTopRecordViewLine, direction);
	}
	else
	{
		// Simple two part algorithm.
		// 1) Scroll down one page
		// 2) Scroll up one page from the bottom
		// Use the higher of the two positions
		// Find the line one page down

		newTopRecordViewLine = PrvViewScrollOnePage (newTopRecordViewLine, direction);
	
		// Find the line at the top of the last page
		// (code copied to PrvViewMakeVisible).
		lastRecordViewLine = PrvViewScrollOnePage (globals->RecordViewLastLine, winUp);
	
		// We shouldn't be past the top line of the last page
		if (newTopRecordViewLine > lastRecordViewLine)
			newTopRecordViewLine = lastRecordViewLine;
	}

	// Get the active form
	frmP = FrmGetActiveForm();
	
	if (newTopRecordViewLine != globals->TopRecordViewLine && 
	! 
	((direction == winUp && newTopRecordViewLine > globals->TopRecordViewLine) || (direction == winDown && newTopRecordViewLine < globals->TopRecordViewLine))
	)
	{
		globals->TopRecordViewLine = newTopRecordViewLine;
		
		PrvViewErase(frmP);
		PrvViewDraw(frmP, 0, 0, 0, false);
	}

	// If we couldn't scroll then scroll to the next record.
	else
	{
		// Move to the next or previous record.
		if (direction == winUp)
		{
			seekDirection = dmSeekBackward;
		}
		else
		{
			seekDirection = dmSeekForward;
		}

		if (globals->ShowAllCategories)
			category = dmAllCategories;
		else
			category = globals->CurrentCategory;

		recordNum = globals->CurrentRecord;

		//skip masked records.
		while (!DmSeekRecordInCategory (globals->AddrDB, &recordNum, 1, seekDirection, category) &&
			   !DmRecordInfo (globals->AddrDB, recordNum, &attr, NULL, NULL) &&
			   ((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == maskPrivateRecords))
		{
		}
		if (recordNum == globals->CurrentRecord) return;
		
		// Don't show first/last record if it's private and we're masking.
		if (!DmRecordInfo (globals->AddrDB, recordNum, &attr, NULL, NULL) &&
			   ((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == maskPrivateRecords))
			return;

		SndPlaySystemSound (sndInfo);

		globals->CurrentRecord = recordNum;
		
	  	MemHandleFree(MemPtrRecoverHandle(globals->RecordViewLines));
	  	globals->RecordViewLines = 0;
	  	
		PrvViewErase(frmP);
		PrvViewOpen();
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvViewMakeVisible
 *
 * DESCRIPTION: Make a selection range visible
 *
 * PARAMETERS:  selectFieldNum - field to show selected text
 *                selectPos - offset into field for start of selected text
 *                selectLen - length of selected text
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   8/3/95   Initial Revision
 *
 ***********************************************************************/
void PrvViewMakeVisible (UInt16 selectFieldNum, UInt16 selectPos, UInt16 selectLen)
{
	UInt16 newTopRecordViewLine;
	UInt16 i;
	globalVars* globals = getGlobalsPtr();


	newTopRecordViewLine = globals->RecordViewLastLine;
	for (i = 0; i < globals->RecordViewLastLine; i++)
	{
		// Does the selected range end here?
		if (globals->RecordViewLines[i].fieldNum == selectFieldNum &&
			globals->RecordViewLines[i].offset <= selectPos + selectLen &&
			selectPos + selectLen <= globals->RecordViewLines[i].offset +
			globals->RecordViewLines[i].length)
		{
			newTopRecordViewLine = i;
		}
	}


	// If the selected range doesn't seem to exist then
	// we shouldn't scroll the view.
	if (newTopRecordViewLine == globals->RecordViewLastLine)
		return;


	// Display as much before the selected text as possible
	newTopRecordViewLine = PrvViewScrollOnePage (newTopRecordViewLine, winUp);

	if (newTopRecordViewLine != globals->TopRecordViewLine)
		globals->TopRecordViewLine = newTopRecordViewLine;
}


/***********************************************************************
 *
 * FUNCTION:    PrvViewPhoneNumberAt
 *
 * DESCRIPTION: Given a point on the screen in the RecordViewDisplay,
 *					 determine whether that point is in a phone number, and if
 *					 so, which one. Phone numbers are defined as linefeed
 *					 separated.
 *
 * PARAMETERS:	x				- x coordinate of point to look at
 *					y				- y coordinate of point to look at
 *					fieldNumP	- result: which field the phone number is in
 *					offsetP		- result: where phone number starts in field
 *					lengthP		- result: how long phone number is
 *
 * RETURNED:	whether there is a phone number at the given point
 *
 * REVISION HISTORY:
 *		Name	Date		Description
 *		----	----		-----------
 *		peter	05/05/00	Initial Revision
 *		peter	05/26/00	Fix bug: Restore font.
 *		aro		06/27/00	Fix bug for non phone field
 *
 ***********************************************************************/
Boolean PrvViewPhoneNumberAt (Int16 x, Int16 y, UInt16 *fieldNumP, UInt16 *offsetP, UInt16 *lengthP)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr			frm;
	FontID			curFont;
	RectangleType	r;
	Int16				lineY, bottomOfRecordViewDisplay, width, height;
	UInt16			previousNonZeroHeight, currentHeight;
	UInt16			i, j;

	frm = FrmGetActiveForm();
	FrmGetObjectBounds(frm, FrmGetObjectIndex(frm, RecordViewDisplay), &r);

	if (globals->TopRecordViewLine < globals->RecordViewFirstPlainLine)
		curFont = FntSetFont (largeBoldFont);
	else
		curFont = FntSetFont (globals->AddrRecordFont);

	lineY = r.topLeft.y;
	previousNonZeroHeight = 0;
	bottomOfRecordViewDisplay = r.topLeft.y +  r.extent.y;

	for (i = globals->TopRecordViewLine; i < globals->RecordViewLastLine; i++)
	{
		// Switch fonts if necessary before computing the extra vertical
		// space needed for this element of the array.
		if (i == globals->RecordViewFirstPlainLine)
			FntSetFont (globals->AddrRecordFont);
		currentHeight = PrvViewCalcNextLine(i, FntLineHeight(), false);

		// Since the above function returns zero for all but the first
		// item when several should be drawn at the same y coordinate,
		// we need to delay adding the result until we get to the next
		// non-zero result.
		if (currentHeight != 0)
		{
			lineY += previousNonZeroHeight;
			previousNonZeroHeight = currentHeight;
		}

		// If we are past the bottom stop drawing
		if (lineY > bottomOfRecordViewDisplay - FntLineHeight())
			break;

		ErrNonFatalDisplayIf(lineY < r.topLeft.y, "Searching for record out of gadget");

		// The remainder of the fields' lines were drawn without any other special
		// handling. These may include continuations of phone numbers that don't
		// fit on one line as well as entire phone numbers which were included in
		// a field, separated by the return stroke.

		// Check if this is a dialable phone
		if ((globals->RecordViewLines[i].fieldNum >= univFirstPhoneField)
			&& (globals->RecordViewLines[i].fieldNum <= univLastPhoneField)
			&& (ToolsIsPhoneIndexSupported(&globals->recordViewRecord, globals->RecordViewLines[i].fieldNum - firstPhoneField)))
		{
			// Dial the number tapped on.
			width = FntCharsWidth
				(&globals->recordViewRecord.fields[globals->RecordViewLines[i].fieldNum][globals->RecordViewLines[i].offset],
				 globals->RecordViewLines[i].length);
			height = FntCharHeight();
			RctSetRectangle(&r, globals->RecordViewLines[i].x, lineY, width, height);
			if (RctPtInRectangle (x, y, &r))
			{
				*fieldNumP = globals->RecordViewLines[i].fieldNum;

				// Look to see if this phone number started on a previous line.
				for (; i != 0; i--)
				{
					if (globals->RecordViewLines[i - 1].fieldNum != *fieldNumP)
						break;
					if (globals->recordViewRecord.fields[*fieldNumP][globals->RecordViewLines[i].offset - 1] == linefeedChr)
						break;
				}
				*offsetP = globals->RecordViewLines[i].offset;
				*lengthP = globals->RecordViewLines[i].length;

				// Look to see if this phone number continues on subsequent lines.
				for (j = i + 1; j < globals->RecordViewLastLine; j++)
				{
					if (globals->RecordViewLines[j].fieldNum != *fieldNumP)
						break;
					if (globals->recordViewRecord.fields[*fieldNumP][globals->RecordViewLines[j].offset - 1] == linefeedChr)
						break;
					*lengthP += globals->RecordViewLines[j].length;
				}

				FntSetFont (curFont);
				return true;
			}
		}
	}

	FntSetFont (curFont);
	return false;		// Given point isn't in a phone number.
}


/***********************************************************************
 *
 * FUNCTION:    PrvViewHandleTapOnPhoneNumber
 *
 * DESCRIPTION: Handle a tap on a phone number in the RecordViewDisplay.
 *					 Highlight the phone number while the pen is over it, and
 *					 dial if the pen is released over it.
 *
 * PARAMETERS:  	fieldNum	- which field
 *					offset		- start of phone number in field
 *					length		- length of phone number in field
 *
 * RETURNED:    Whether Dial Number screen has been displayed
 *
 * REVISION HISTORY:
 *		Name	Date		Description
 *		----	----		-----------
 *		peter	05/05/00	Initial Revision
 *		aro		06/27/00	Add dialing
 *		fpa		10/19/00	Returns a boolean
 *
 ***********************************************************************/
Boolean PrvViewHandleTapOnPhoneNumber (UInt16 fieldNum, UInt16 offset, UInt16 length)
{
	UInt16			testFieldNum, testOffset, testLength;
	Int16				testX, testY;
	Boolean			isPenDown, wasSelected, isSelected;
	FormType*		frmP;
	globalVars* globals = getGlobalsPtr();

	frmP = FrmGetActiveForm();
	wasSelected = true;
	PrvViewDraw(frmP, fieldNum, offset, length, true);

	do
	{
		PenGetPoint (&testX, &testY, &isPenDown);
		isSelected = PrvViewPhoneNumberAt(testX, testY, &testFieldNum, &testOffset, &testLength) &&
			testFieldNum == fieldNum && testOffset == offset;
		if (isSelected != wasSelected)
		{
			PrvViewDraw(frmP,fieldNum, offset, isSelected ? length : 0, true);
			wasSelected = isSelected;
		}
	} while (isPenDown);

	if (isSelected)
	{
		UInt16 lineIndex;
		PrvViewDraw(frmP, fieldNum, offset, 0, true);

		lineIndex = ToolsGetLineIndexAtOffset(globals->adxtLibRef, globals->recordViewRecord.fields[fieldNum], offset);
		return DialListShowDialog(globals->CurrentRecord, fieldNum - univFirstPhoneField, lineIndex);
	}
	else
		return false;
}


/***********************************************************************
 *
 * FUNCTION:    PrvViewHandlePen
 *
 * DESCRIPTION: Handle pen movement in the RecordViewDisplay. If the user
 *					 taps in the RecordViewDisplay take them to the Edit View
 *					 unless they tap on a phone number. In that case, arrange
 *					 to dial the selected number.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if handled.
 *
 * REVISION HISTORY:
 *		Name	Date		Description
 *		----	----		-----------
 *		roger	11/27/95	Cut from RecordViewHandleEvent
 *		peter	05/03/00	Add support for tapping on phone numbers to dial
 *      aro     06/27/00    Check for dialing abilities
 *
 ***********************************************************************/
Boolean PrvViewHandlePen (EventType * event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr			frm;
	RectangleType	r;
	Int16				x, y;
	Boolean			isPenDown;
	UInt16			fieldNum, offset, length;

	frm = FrmGetActiveForm();
	FrmGetObjectBounds(frm, FrmGetObjectIndex(frm, RecordViewDisplay), &r);
	if (! RctPtInRectangle (event->screenX, event->screenY, &r))
		return false;

	// Check if the user tapped on a phone number.
	if (globals->EnableTapDialing)
	{
		if (PrvViewPhoneNumberAt (event->screenX, event->screenY, &fieldNum, &offset, &length))
		{
			// The user tapped on this phone number. Wait for the pen up, highlighting the
			// phone number when the pen is over the number.
			if ( PrvViewHandleTapOnPhoneNumber (fieldNum, offset, length) )
				return true;
		}
	}

	// The user tapped in the record view display, but not on a phone number,
	// so wait for the pen to be released and if it's released inside the
	// record view display, edit the record.
	do
	{
		PenGetPoint (&x, &y, &isPenDown);
	} while (isPenDown);
	if (RctPtInRectangle (x, y, &r))
		FrmGotoForm (EditView);

	return true;
}

// show/hide links button
static void PrvViewDrawLinksButton()
{
	globalVars* globals = getGlobalsPtr();
	UInt32 id;
	UInt16 linkCount;
	FormPtr frm = FrmGetActiveForm();
		
	DmRecordInfo(globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
	linkCount = GetLinkCount(globals->adxtLibRef, globals->linksDB, id);
		
	if(linkCount>0)
		CustomShowObject(frm, RecordLinksButton);
	else
		CustomHideObject(frm, RecordLinksButton);
}