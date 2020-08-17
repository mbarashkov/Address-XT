/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ToDo.c
 *
 * Description:
 *	  This is the ToDo application's main module.  
 *
 * History:
 *		April 24, 1995	Created by Art Lamb
 *			Name	Date		Description
 *			----	----		-----------
 *			rbb	4/14/99	Optional category column is now shown only
 *								for "All".
 *								Added GetObjectValue, SetObjectValue,
 *								ToDoListViewDrawTable and ToDoListViewRedrawTable
 *								to reduce patches of duplicate code.
 *			rbb	10/4/99	Moved SetDBBackupBit to ToDoDB.c
 *
 *****************************************************************************/

#include <PalmOS.h>
#include "ToDo.h"
#include "ToDoDB.h"
#include "../AddressRsc.h"
#include "../Address.h"
#include "../AddrTools.h"


// Error checking routines
void ECToDoDBValidate(DmOpenRef dbP);

// Exported routines
extern Char* GetToDoNotePtr (ToDoDBRecordPtr recordP);

//#define min(a,b) (a<b? a:b)

/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/
#define toDoVersionNum					3
#define toDoPrefsVersionNum			3
#define todoPrefID						0x00
#define toDoDBName						"ToDoDB"
#define toDoDBType						'DATA'

// Fonts used by application
#define noteTitleFont					boldFont

// Columns in the ToDo table of the list view.
#define completedColumn					0
#define priorityColumn					1
#define descColumn						2
#define dueDateColumn					3
#define categoryColumn					4

#define spaceNoPriority					1
#define spaceBeforeDesc					2
#define spaceBeforeCategory			2

#define newToDoSize  					16
#define defaultPriority					1

#define maxNoteTitleLen					40

// Due Date popup list chooses
#define dueTodayItem						0
#define dueTomorrowItem					1
#define dueOneWeekLaterItem			2
#define noDueDateItem					3
#define selectDateItem					4

// Sort Order popup list choices
#define priorityDueDateItem			0
#define dueDatePriorityItem			1
#define categoryPriorityItem			2
#define categoryDueDateItem			3

// Update codes, used to determine how the ToDo list should 
// be redrawn.
#define updateItemDelete				0x01
#define updateItemMove					0x02
#define updateItemHide					0x04
#define updateGoTo						0x20

// Field numbers, used to indicate where search string was found.
#define descSeacrchFieldNum			0
#define noteSeacrchFieldNum			1

#define noRecordSelected				0xffff

// Number of system ticks (1/60 seconds) to display crossed out item
// before they're erased.
#define crossOutDelay					40


/***********************************************************************
 *
 *	Internal Structutes
 *
 ***********************************************************************/

// This is the structure of the data stored in the state file.
typedef struct {
	UInt16			currentCategory;
	FontID			v20NoteFont;		// For 2.0 compatibility (BGT)
	Boolean			showAllCategories;
	Boolean 			showCompletedItems;
	Boolean 			showOnlyDueItems;
	Boolean			showDueDates;
	Boolean			showPriorities;
	Boolean			showCategories;
	Boolean			saveBackup;
	Boolean			changeDueDate;
	
	// Version 3 preferences
	FontID			listFont;
	FontID			noteFont;		// For 3.0 and later units.	(BGT)
	
	UInt8				reserved;
} ToDoPreferenceType;

typedef struct {
	DmOpenRef		db;
	Char *			categoryName;
	UInt16			categoryIndex;
} AcceptBeamType;


/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/
static void ToDoListViewInit (FormPtr frm);
static void ToDoListViewLoadTable (Boolean fillTable);
static void ToDoListViewDrawTable (UInt16 updateCode);
static void ToDoListViewRedrawTable (Boolean fillTable);
static Boolean ToDoListViewUpdateDisplay (UInt16 updateCode);


static void ToDoLoadPrefs(void);		// (BGT)

/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  This routine opens the application's database, loads the 
 *               saved-state information and initializes global variables.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *			grant	4/6/99	Move code to set backup bit into SetDBBackupBit
 *			jmp	10/4/99	Call ToDoGetDatabase() in place of similar in-line code.
 *
 ***********************************************************************/
static void StopApplication (void)
{
	globalVars* globals = getGlobalsPtr();
	DmCloseDatabase (globals->ToDoDB);
}

static UInt16 StartApplication (void)
{
	globalVars* globals = getGlobalsPtr();
	Err err = errNone;
	UInt16 mode;

	// Determime if secret record should be shown.
	globals->PrivateRecordVisualStatus = (privateRecordViewEnum)PrefGetPreference(prefShowPrivateRecords);
	mode = (globals->PrivateRecordVisualStatus == hidePrivateRecords) ?
					dmModeReadWrite : (dmModeReadWrite | dmModeShowSecret);

	// Get the date format from the system preferences.
	globals->DateFormat = (DateFormatType)PrefGetPreference(prefDateFormat);

	// Find the application's data file.  If it doesn't exist create it.
	err = ToDoGetDatabase(&globals->ToDoDB, mode);
	if (err)
		return err;

	// Read the preferences.
	ToDoLoadPrefs();
	globals->TopVisibleRecord = 0;
	globals->ToDoCurrentRecord = noRecordSelected;

	// Get today's date.  Will will use it to determine if passed due items
	// need to be redrawn when the device is powered on the next time.
	DateSecondsToDate (TimGetSeconds (), &globals->Today);

	return err;
}


/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  formId - id of the form to display
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95		Initial Revision
 *
 ***********************************************************************/
static void* GetObjectPtr (UInt16 objectID)
{
	FormPtr frm;
	
	frm = FrmGetActiveForm ();
	return (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, objectID)));

}


/***********************************************************************
 *
 * FUNCTION:    SetObjectValue
 *
 * DESCRIPTION: Assign a value to the object with the given ID
 *
 * PARAMETERS:  objectID  - id of the object to change
 *              value     - new value of the object
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rbb	4/14/99	Initial Revision
 *
 ***********************************************************************/
static void SetObjectValue (UInt16 objectID, Int16 value)
{
	ControlPtr ctl;

	ctl = GetObjectPtr (objectID);
	CtlSetValue (ctl, value);
}


/***********************************************************************
 *
 * FUNCTION:    GetObjectValue
 *
 * DESCRIPTION: Return the value of the object with the given ID
 *
 * PARAMETERS:  objectID  - id of the object to change
 *
 * RETURNED:    value of the object
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rbb	4/14/99	Initial Revision
 *
 ***********************************************************************/
static Int16 GetObjectValue (UInt16 objectID)
{
	ControlPtr ctl;

	ctl = GetObjectPtr (objectID);
	return CtlGetValue (ctl);
}


/***********************************************************************
 *
 * FUNCTION:    GetFocusObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to the field object, in 
 *              the current form, that has the focus.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    pointer to a field object or NULL of there is no fucus
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95		Initial Revision
 *
 ***********************************************************************/
static FieldPtr GetFocusObjectPtr (void)
{
	FormPtr frm;
	UInt16 focus;
	FormObjectKind objType;
	
	frm = FrmGetActiveForm ();
	focus = FrmGetFocus (frm);
	if (focus == noFocus)
		return (NULL);
		
	objType = FrmGetObjectType (frm, focus);
	
	if (objType == frmFieldObj)
		return (FrmGetObjectPtr (frm, focus));
	
	else if (objType == frmTableObj)
		return (TblGetCurrentField (FrmGetObjectPtr (frm, focus)));
	
	return (NULL);
}


/***********************************************************************
 *
 * FUNCTION:    ChangeCategory
 *
 * DESCRIPTION: This routine updates the global variables that keep track
 *              of category information.  
 *
 * PARAMETERS:  category  - new category (index)
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/10/95	Initial Revision
 *
 ***********************************************************************/
static void ChangeCategory (UInt16 category)
{
	globalVars* globals = getGlobalsPtr();
	globals->CurrentCategory = category;
	globals->TopVisibleRecord = 0;
}

/***********************************************************************
 *
 * FUNCTION:    SeekRecord
 *
 * DESCRIPTION: Given the index of a ToDo record, this routine scans 
 *              forwards or backwards for displayable ToDo records.           
 *
 * PARAMETERS:  indexP  - pointer to the index of a record to start from;
 *                        the index of the record sought is returned in
 *                        this parameter.
 *
 *              offset  - number of records to skip:   
 *                        	0 - seek from the current record to the
 *                             next display record, if the current record is
 *                             a display record, its index is retuned.
 *                         1 - seek foreward, skipping one displayable 
 *                             record
 *                        -1 - seek backwards, skipping one displayable 
 *                             record
 *                             
 *
 * RETURNED:    false is return if a displayable record was not found.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/11/95	Initial Revision
 *
 ***********************************************************************/
static Boolean SeekRecord (UInt16* indexP, Int16 offset, Int16 direction)
{
	globalVars* globals = getGlobalsPtr();
	Int32 dateL;
	Int32 todayL;
	MemHandle recordH;
	DateTimeType today;
	ToDoDBRecordPtr toDoRec;

	ErrFatalDisplayIf ( (offset < -1 || offset > 1) , "Invalid offset");

	while (true)
		{
		DmSeekRecordInCategory (globals->ToDoDB, indexP, offset, direction, globals->CurrentCategory);
		if (DmGetLastErr()) return (false);
	
		if ( globals->ShowCompletedItems && (! globals->ShowOnlyDueItems))
			return (true);
		
		recordH = DmQueryRecord (globals->ToDoDB, *indexP);
		toDoRec = (ToDoDBRecordPtr) MemHandleLock (recordH);
		
		if ( (globals->ShowCompletedItems) || (! (toDoRec->priority & completeFlag)))
			{
			if (! globals->ShowOnlyDueItems) break;
			
			if (DateToInt (toDoRec->dueDate) == toDoNoDueDate) break;

			// Check if the item is due.
			TimSecondsToDateTime (TimGetSeconds(), &today);
			todayL = ( ((Int32) today.year) << 16) + 
						( ((Int32) today.month) << 8) + 
						  ((Int32) today.day);

			dateL = ( ((Int32) toDoRec->dueDate.year + firstYear) << 16) + 
					  ( ((Int32) toDoRec->dueDate.month) << 8) + 
						 ((Int32) toDoRec->dueDate.day);

			if (dateL <= todayL)	break;
			}
		
		if (offset == 0) offset = 1;
		
		MemHandleUnlock (recordH);	
		}

	MemHandleUnlock (recordH);	
	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    GetToDoNotePtr
 *
 * DESCRIPTION: This routine returns a pointer to the note field in a to 
 *              do record.
 *
 * PARAMETERS:  recordP - pointer to a ToDo record
 *
 * RETURNED:    pointer to a null-terminated note
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/15/95	Initial Revision
 *
 ***********************************************************************/
Char* GetToDoNotePtr (ToDoDBRecordPtr recordP)
{
	return (&recordP->description + StrLen (&recordP->description) + 1);
}

Err ToDoListGetLabel(UInt32 id, MemHandle *textHP)
{
	globalVars* globals = getGlobalsPtr();
	MemHandle recordH;
	UInt16 recordNum;
	/*err = ToDoGetDatabase(&ToDoDB, dmModeReadOnly);
	if (err)
		return err;*/
	
	DmFindRecordByID(globals->ToDoDB, id, &recordNum);
	
	// Get the record number that corresponds to the table item.
	// The record number is stored as the row id.
	recordH = DmQueryRecord( globals->ToDoDB, recordNum);
	ErrFatalDisplayIf ((! recordH), "Record not found");
	
	
	*textHP = recordH;
	
//	MemHandleUnlock (recordH);

	return (0);
}


/***********************************************************************
 *
 * FUNCTION:    ToDoListViewGetDescription
 *
 * DESCRIPTION: This routine returns a pointer to the description field
 *              of a ToDo record.  This routine is called by the table 
 *              object as a callback routine when it wants to display or
 *              edit a ToDo description.
 *
 * PARAMETERS:  table  - pointer to the ToDo list table (TablePtr)
 *              row    - row of the table
 *              column - column of the table
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95		Initial Revision
 *
 ***********************************************************************/
static Err ToDoListViewGetDescription (void * table, Int16 row, UInt16 /* column */,
	Boolean /* editable */, MemHandle *textHP, UInt16 * textOffset, UInt16 * textAllocSize,
	FieldPtr fld)
{
#pragma unused(fld)
	globalVars* globals = getGlobalsPtr();
	UInt16 recordNum;
	MemHandle recordH;
	ToDoDBRecordPtr toDoRec;
	
	// Get the record number that corresponds to the table item.
	// The record number is stored as the row id.
	recordNum = TblGetRowID (table, row);
	recordH = DmQueryRecord( globals->ToDoDB, recordNum);
	ErrFatalDisplayIf ((! recordH), "Record not found");
	
	toDoRec = (ToDoDBRecordPtr) MemHandleLock (recordH);

	*textOffset = &toDoRec->description - ((Char *) toDoRec);
	*textAllocSize = StrLen (&toDoRec->description) + 1;  // one for null terminator
	*textHP = recordH;
	
	MemHandleUnlock (recordH);

	return (0);
}

/***********************************************************************
 *
 * FUNCTION:    ToDoListViewGetDescriptionHeight
 *
 * DESCRIPTION: This routine returns the height, in pixels, of a ToDo 
 *              description.
 *
 * PARAMETERS:  recordNum - record index
 *              width     - width of description
 *
 * RETURNED:    height in pixels
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *			jmp	10/27/99	Restore current font in masked-records case; fixes
 *								bug #23276.
 *
 ***********************************************************************/
static UInt16 ToDoListViewGetDescriptionHeight (UInt16 recordNum, UInt16 width, UInt16 maxHeight)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 height;
	UInt16 lineHeight;
	Char* note;
	FontID curFont;
	MemHandle recordH;
	ToDoDBRecordPtr toDoRec;
	Boolean masked;
	UInt16 attr;
	
	//mask if appropriate
	DmRecordInfo (globals->ToDoDB, recordNum, &attr, NULL, NULL);
   masked = (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == maskPrivateRecords));
	
	curFont = FntSetFont (globals->ListFont);
	lineHeight = FntLineHeight ();
	
	if (masked)
		{
		FntSetFont (curFont);
		return lineHeight;
		}

	// Get a pointer to the ToDo record.
	recordH = DmQueryRecord( globals->ToDoDB, recordNum);
	ErrFatalDisplayIf ((! recordH), "Record not found");
	
	toDoRec = (ToDoDBRecordPtr) MemHandleLock (recordH);

	// If the record has a note, leave space for the note indicator.
	note = GetToDoNotePtr (toDoRec);
	if (*note)
		width -= tableNoteIndicatorWidth;

	// Compute the height of the ToDo item's description.

	height = FldCalcFieldHeight (&toDoRec->description, width);
	height = min (height, (maxHeight / lineHeight));
	height *= lineHeight;

	FntSetFont (curFont);

	MemHandleUnlock (recordH);

	return (height);
}


/***********************************************************************
 *
 * FUNCTION:    ToDoListViewPriorityFontID
 *
 * DESCRIPTION: This routine is called to determine the correct font to
 *						use for drawing the list view priority number - we
 *						want to bold the list view font.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    Font id for list view priority number.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kwk	06/23/99	New today.
 *
 ***********************************************************************/
static FontID ToDoListViewPriorityFontID (void)
{
	globalVars* globals = getGlobalsPtr();
	if (globals->ListFont == stdFont)
		return (boldFont);
	else if (globals->ListFont == largeFont)
		return (largeBoldFont);
	else
		return (globals->ListFont);
}

/***********************************************************************
 *
 * FUNCTION:    ToDoListViewDrawDueDate
 *
 * DESCRIPTION: This routine draws a ToDo items due date.
 *
 * PARAMETERS:	 table  - pointer to a table object
 *              row    - row the item is in
 *              column - column the item is in
 *              bounds - region to draw in
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/14/95	Initial Revision
 *
 ***********************************************************************/
static void ToDoListViewDrawDueDate (void * table, Int16 row, Int16 /* column */, 
	RectanglePtr bounds)
{
	globalVars* globals = getGlobalsPtr();
	char dueChr;
	char dateBuffer [dateStringLength];
	Char* dateStr;
	UInt16 dateStrLen;
	UInt16 dueChrWidth;
	Int16 drawX, drawY;
	FontID curFont;
	FontID fontID;
	DateType date;
	DateTimeType today;
	Int32 todayL, dateL;
	

	// Get the due date to the item being drawn.
	*((Int16 *) (&date)) = TblGetItemInt (table, row, dueDateColumn);


	// If there is no date draw a dash to indicate such.
	if (DateToInt (date) == -1)
		{
		curFont = FntSetFont (stdFont);
		drawX = bounds->topLeft.x + ((bounds->extent.x - 5) >> 1);
		drawY = bounds->topLeft.y + ((FntLineHeight () + 1) / 2);
		WinDrawLine (drawX, drawY, drawX+5, drawY);		
		FntSetFont (curFont);
		return;
		}
	
	// Get the width of the character that indicates the item is due.  Don't
	// count the whitespace in the character.
	fontID = ToDoListViewPriorityFontID();
	curFont = FntSetFont (fontID);
	dueChr = '!';
	dueChrWidth = FntCharWidth (dueChr) - 1;

	FntSetFont (globals->ListFont);
	
	DateToAscii (date.month, date.day, date.year + firstYear, 
					globals->DateFormat, dateBuffer);

	// Remove the year from the date string.
	dateStr = dateBuffer;
	if ((globals->DateFormat == dfYMDWithSlashes) ||
		 (globals->DateFormat == dfYMDWithDots) ||
		 (globals->DateFormat == dfYMDWithDashes))
		dateStr += 3;
	else
		{
		dateStr[StrLen(dateStr) - 3] = 0;
		}


	// Draw the due date, right aligned.
	dateStrLen = StrLen (dateStr);
	drawX = bounds->topLeft.x + bounds->extent.x - dueChrWidth -
		FntCharsWidth (dateStr, dateStrLen);
	drawY = bounds->topLeft.y ;
	WinDrawChars (dateStr, dateStrLen, drawX, drawY);
	
	
	// If the date is on or before today draw an exclamation mark.
	TimSecondsToDateTime (TimGetSeconds(), &today);

	todayL = ( ((Int32) today.year) << 16) + 
				( ((Int32) today.month) << 8) + 
				  ((Int32) today.day);

	dateL = ( ((Int32) date.year + firstYear) << 16) + 
			  ( ((Int32) date.month) << 8) + 
				 ((Int32) date.day);
	
	if (dateL < todayL)
	{
		drawX = bounds->topLeft.x + bounds->extent.x - dueChrWidth;
		FntSetFont (fontID);
		WinDrawChars (&dueChr, 1, drawX, drawY);
	}

	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:    ToDoListViewDrawCategory
 *
 * DESCRIPTION: This routine draws a ToDo item's category name.
 *
 * PARAMETERS:	 table  - pointer to a table object
 *              row    - row the item is in
 *              column - column the item is in
 *              bounds - region to draw in
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/21/96	Initial Revision
 *
 ***********************************************************************/
static void ToDoListViewDrawCategory (void * table, Int16 row, Int16 /* column */, 
	RectanglePtr bounds)
{
	globalVars* globals = getGlobalsPtr();
	Int16 width;
	Int16 length;
	UInt16 attr;
	UInt16 category;
	UInt16 recordNum;
	Boolean fits;
	Char categoryName [dmCategoryLength];
	FontID curFont;

	curFont = FntSetFont (globals->ListFont);

	// Get the category of the item in the specified row.
	recordNum = TblGetRowID (table, row);
	DmRecordInfo (globals->ToDoDB, recordNum, &attr, NULL, NULL);
	category = attr & dmRecAttrCategoryMask;

	// Get the name of the category and trunctae it to fix the the 
	// column passed.
	CategoryGetName (globals->ToDoDB, category, categoryName);
	width = bounds->extent.x;
	length = StrLen(categoryName);
	FntCharsInWidth (categoryName, &width, &length, &fits);
	
	// Draw the category name.
	WinDrawChars (categoryName, length, bounds->topLeft.x, 
		bounds->topLeft.y);

	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:    ToDoListViewUpdateScrollers
 *
 * DESCRIPTION: This routine draws or erases the list view scroll arrow
 *              buttons.
 *
 * PARAMETERS:  frm             - pointer to the ToDo list form
 *              bottomRecord    - record index of the last visible record
 *              lastItemClipped - true if the last item display is not fully
 *                                visible
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/1/95	Initial Revision
 *
 ***********************************************************************/
static void ToDoListViewUpdateScrollers (FormPtr frm, UInt16 bottomRecord,
	Boolean lastItemClipped)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 upIndex;
	UInt16 downIndex;
	UInt16 recordNum;
	Boolean scrollableUp;
	Boolean scrollableDown;
		
	// If the first record displayed is not the first record in the category,
	// enable the winUp scroller.
	recordNum = globals->TopVisibleRecord;
	scrollableUp = SeekRecord (&recordNum, 1, dmSeekBackward);


	// If the last record displayed is not the last record in the category,
	// or the list item is clipped, enable the winDown scroller.
	recordNum = bottomRecord;
	scrollableDown = SeekRecord (&recordNum, 1, dmSeekForward) || lastItemClipped; 


	// Update the scroll button.
	upIndex = FrmGetObjectIndex (frm, ToDoListUpButton);
	downIndex = FrmGetObjectIndex (frm, ToDoListDownButton);
	FrmUpdateScrollers (frm, upIndex, downIndex, scrollableUp, scrollableDown);
}


/***********************************************************************
 *
 * FUNCTION:    ListInitTableRow
 *
 * DESCRIPTION: This routine initialize a row in the ToDo list.
 *
 * PARAMETERS:  table      - pointer to the table of ToDo items
 *              row        - row number (first row is zero)
 *              recordNum  - the index of the record display in the row
 *              rowHeight  - height of the row in pixels
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/1/95		Initial Revision
 *
 ***********************************************************************/
static void ListInitTableRow (TablePtr table, Int16 row, UInt16 recordNum, 
	Int16 rowHeight)
{
	globalVars* globals = getGlobalsPtr();
	Char* note;
	UInt32	uniqueID;
	MemHandle recordH;
	ToDoDBRecordPtr toDoRec;

	// Get a pointer to the ToDo record.
	recordH = DmQueryRecord( globals->ToDoDB, recordNum);
	toDoRec = (ToDoDBRecordPtr) MemHandleLock (recordH);

	// Make the row usable.
	TblSetRowUsable (table, row, true);
	
	// Set the height of the row to the height of the description.
	TblSetRowHeight (table, row, rowHeight);
	
	// Store the record number as the row id.
	TblSetRowID (table, row, recordNum);
	
	// Store the unique id of the record in the table.
	DmRecordInfo (globals->ToDoDB, recordNum, NULL, &uniqueID, NULL);
	TblSetRowData (table, row, uniqueID);

	// Set the checkbox that indicates the completion status.
	TblSetItemInt (table, row, completedColumn, 
		(toDoRec->priority & completeFlag) == completeFlag);

	// Store the priority in the table.
	TblSetItemInt (table, row, priorityColumn, 
		toDoRec->priority & priorityOnly);
	
	// Store the due date in the table.
	TblSetItemInt (table, row, dueDateColumn, (*(Int16 *) &toDoRec->dueDate));

	// Set the table item type for the description, it will differ depending
	// on the presents of a note.
	note = GetToDoNotePtr (toDoRec);
	if (*note)
		TblSetItemStyle (table, row, descColumn, textWithNoteTableItem);
	else
		TblSetItemStyle (table, row, descColumn, textTableItem);		
	

	// Mark the row invalid so that it will drawn when we call the 
	// draw routine.
	TblMarkRowInvalid (table, row);
	
	MemHandleUnlock (recordH);
}


/***********************************************************************
 *
 * FUNCTION:    ToDoListViewLoadTable
 *
 * DESCRIPTION: This routine reloads ToDo database records into
 *              the list view.  This routine is called when:
 *              	o A new item is inserted
 *              	o An item is deleted
 *              	o The priority or due date of an items is changed
 *              	o An item is marked complete
 *              	o Hidden items are shown
 *              	o Completed items are hidden
 *
 * PARAMETERS:  fillTable - if true the top visible item will be scroll winDown
 *                          such that a full table is displayed
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/1/95	Initial Revision
  *		jmp	9/29/99	Use FrmGetFormPtr() & FrmGetObjectIndex() instead of
 *								GetObjectPtr() because GetObjectPtr() calls FrmGetActiveForm(),
 *								and FrmGetActiveForm() may return a form that isn't the one we
 *								want when other forms are up when we are called.
 *								Fixes bug #22418.
 *
 ***********************************************************************/
static void ToDoListViewLoadTable (Boolean fillTable)
{
	globalVars* globals = getGlobalsPtr();
	UInt16			row;
	UInt16			numRows;
	UInt16			recordNum;
	UInt16			lastRecordNum;
	UInt16			dataHeight;
	UInt16			lineHeight;
	UInt16			tableHeight;
	UInt16			columnWidth;
	UInt16			pos, oldPos;
	UInt16			height, oldHeight;
	UInt32			uniqueID;
	FontID			curFont;
	Boolean			rowUsable;
	Boolean			rowsInserted = false;
	Boolean			lastItemClipped;
	FormPtr			frm;
	TablePtr			table;
	RectangleType	r;
	UInt16 			attr;
	Boolean 			masked;
	
	frm = FrmGetFormPtr (ToDoListView);
	
	// Make sure the global variable that holds the index of the 
	// first visible record has a valid value.
	if (! SeekRecord (&globals->TopVisibleRecord, 0, dmSeekForward))
		if (! SeekRecord (&globals->TopVisibleRecord, 0, dmSeekBackward))
			globals->TopVisibleRecord = 0;

	// If we have a currently selected record, make sure that it is not
	// above the first visible record.
	if (globals->ToDoCurrentRecord != noRecordSelected)
		if (globals->ToDoCurrentRecord < globals->TopVisibleRecord)
			globals->ToDoCurrentRecord = globals->TopVisibleRecord;
		

	// Get the height of the table and the width of the description
	// column.
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ToDoListTable));
	TblGetBounds (table, &r);
	tableHeight = r.extent.y;
	columnWidth = TblGetColumnWidth (table, descColumn);

	// Get the height one one line.
	curFont = FntSetFont (globals->ListFont);
	lineHeight = FntLineHeight ();
	FntSetFont (curFont);

	row = 0;
	dataHeight = 0;
	oldPos = pos = 0;
	recordNum = globals->TopVisibleRecord;
	lastRecordNum = recordNum;
	
	// Load records into the table.
	while (true)
		{		
		// Get the next record in the currunt category.
		if ( ! SeekRecord (&recordNum, 0, dmSeekForward))
			break;

		// Compute the height of the ToDo item's description.
		height = ToDoListViewGetDescriptionHeight (recordNum, columnWidth, tableHeight);

		// Is there enought room for at least one line of the the decription.
		if (tableHeight >= dataHeight + lineHeight)
			{
			// Get the height of the current row.
			rowUsable = TblRowUsable (table, row);
			if (rowUsable)
				oldHeight = TblGetRowHeight (table, row);
			else
				oldHeight = 0;

			DmRecordInfo (globals->ToDoDB, recordNum, &attr, NULL, NULL);
	   		masked = (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == maskPrivateRecords));
			TblSetRowMasked(table,row,masked);
			
			// Determine if the row needs to be initialized.  We will initialize 
			// the row if: the row is not usable (not displayed),  the unique
			// id of the record does not match the unique id stored in the 
			// row.
			DmRecordInfo (globals->ToDoDB, recordNum, NULL, &uniqueID, NULL);
			if ((TblGetRowData (table, row) != uniqueID) ||
				 (! TblRowUsable (table, row)) ||
				 (TblRowInvalid (table, row)))
				{
				ListInitTableRow (table, row, recordNum, height);
				}

			// If the height or the position of the item has changed draw the item.
			else 
				{
				TblSetRowID (table, row, recordNum);
				if (height != oldHeight)
					{
					TblSetRowHeight (table, row, height);
					TblMarkRowInvalid (table, row);
					}
				else if (pos != oldPos)
					{
					TblMarkRowInvalid (table, row);
					}
				}
				
			pos += height;
			oldPos += oldHeight;

			lastRecordNum = recordNum;
			row++;
			recordNum++;
			}

		dataHeight += height;

		// Is the table full?
		if (dataHeight >= tableHeight)		
			{
			// If we have a currently selected record, make sure that it is
			// not below  the last visible record.
			if ((globals->ToDoCurrentRecord == noRecordSelected) ||
				 (globals->ToDoCurrentRecord <= lastRecordNum)) break;

			globals->TopVisibleRecord = recordNum;
			row = 0;
			dataHeight = 0;
			}
		}

	// Hide the items that don't have any data.
	numRows = TblGetNumberOfRows (table);
	while (row < numRows)
		{		
		TblSetRowUsable (table, row, false);
		row++;
		}
		
	// If the table is not full and the first visible record is 
	// not the first record	in the database, displays enough records
	// to fill out the table.
	while (dataHeight < tableHeight)
		{
		if (! fillTable) 
			break;
			
		recordNum = globals->TopVisibleRecord;
		if ( ! SeekRecord (&recordNum, 1, dmSeekBackward))
			break;

		// Compute the height of the ToDo item's description.
		height = ToDoListViewGetDescriptionHeight (recordNum, columnWidth, tableHeight);
			
		// If adding the item to the table will overflow the height of
		// the table, don't add the item.
		if (dataHeight + height > tableHeight)
			break;
		
		// Insert a row before the first row.
		TblInsertRow (table, 0);

		ListInitTableRow (table, 0, recordNum, height);
		//mask if appropriate
		DmRecordInfo (globals->ToDoDB, recordNum, &attr, NULL, NULL);
   		masked = (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == maskPrivateRecords));
		TblSetRowMasked(table,0,masked);
		
		globals->TopVisibleRecord = recordNum;
		
		rowsInserted = true;

		dataHeight += height;
		}
		
	// If rows were inserted to full out the page, invalidate the whole
	// table, it all needs to be redrawn.
	if (rowsInserted)
		TblMarkTableInvalid (table);

	// If the height of the data in the table is greater than the height
	// of the table, then the bottom of the last row is clipped and the 
	// table is scrollable.
	lastItemClipped = (dataHeight > tableHeight);

	// Update the scroll arrows.
	ToDoListViewUpdateScrollers (frm, lastRecordNum, lastItemClipped);
}


/***********************************************************************
 *
 * FUNCTION:    ToDoListViewDrawTable
 *
 * DESCRIPTION: Updates the entire list view, such as when changing categories 
 *
 * PARAMETERS:  updateCode - indicates how (or whether) to rebuild the table
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rbb	4/14/99	Initial Revision
 *
 ***********************************************************************/
static void ToDoListViewDrawTable (UInt16 updateCode)
{
	TablePtr table = GetObjectPtr (ToDoListTable);
	
	TblEraseTable (table);
	
	switch (updateCode)
		{
		case updateDisplayOptsChanged:
		case updateFontChanged:
			ToDoListViewInit (FrmGetActiveForm ());
			break;
			
		case updateCategoryChanged:
		case updateGoTo:
			ToDoListViewLoadTable (true);
			break;
		}
		
	TblDrawTable (table);
}


/***********************************************************************
 *
 * FUNCTION:    ToDoListViewRedrawTable
 *
 * DESCRIPTION: Redraw the rows of the table that are marked invalid 
 *
 * PARAMETERS:  fillTable - if true the top visible item will be scroll down
 *                          such that a full table is displayed
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rbb	4/14/99	Initial Revision
 *			jmp	9/29/99	Use FrmGetFormPtr() & FrmGetObjectIndex() instead of
 *								GetObjectPtr() because GetObjectPtr() calls FrmGetActiveForm(),
 *								and FrmGetActiveForm() may return a form that isn't the one we
 *								want when other forms are up when we are called.
 *								Fixes bug #22418.
 *
 ***********************************************************************/
static void ToDoListViewRedrawTable (Boolean fillTable)
{
	TablePtr table;
	FormPtr frm;
	
	frm = FrmGetFormPtr (ToDoListView);
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ToDoListTable));
	
	ToDoListViewLoadTable (fillTable);
	TblRedrawTable (table);
}




/***********************************************************************
 *
 * FUNCTION:    ToDoListViewSelectCategory
 *
 * DESCRIPTION: This routine handles selection, creation and deletion of
 *              categories in the List View. 
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    The index of the new category.
 *
 *              The following global variables are modified:
 *							globals->CurrentCategory
 *							ShowAllCategories
 *							CategoryName
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	03/10/95 Initial Revision
 *			rbb	04/14/99 Uses new ToDoListViewDrawTable
 *			gap	08/13/99	Update to use new constant categoryDefaultEditCategoryString.
 *
 ***********************************************************************/
static UInt16 ToDoListViewSelectCategory (void)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm;
	UInt16 category;
	Boolean categoryEdited;
	UInt8 updateCode = updateCategoryChanged;
	
	// Process the category popup list.  
	category = globals->CurrentCategory;

	frm = FrmGetActiveForm();
	categoryEdited = CategorySelect (globals->ToDoDB, frm, ToDoListCategoryTrigger,
					    ToDoListCategoryList, true, &category, globals->CategoryName, 1, categoryHideEditCategory );
	
	// If the option for category column is set and we switched to/from "All",
	// the table will need to be rebuilt with/without the column
	if ( globals->ShowCategories && (globals->CurrentCategory != category) &&
			( (category == dmAllCategories) || (globals->CurrentCategory == dmAllCategories) ))
		{
		updateCode = updateDisplayOptsChanged;
		}
		
	if (category == dmAllCategories)
		globals->ShowAllCategories = true;
	else
		globals->ShowAllCategories = false;
		
	if ( (categoryEdited) || (globals->CurrentCategory != category) || globals->ShowCategories)
		{
		ChangeCategory (category);
		// Display the new category.
		ToDoListViewDrawTable (updateCode);
		}

	return (category);
}


/***********************************************************************
 *
 * FUNCTION:    ToDoListViewNextCategory
 *
 * DESCRIPTION: This routine displays the next category, if the last
 *              catagory is being displayed we wrap to the first category.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 *              The following global variables are modified:
 *							globals->CurrentCategory
 *							ShowAllCategories
 *							CategoryName
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		9/15/95		Initial Revision
 *			rbb		4/14/99		Uses new ToDoListViewDrawTable
 *
 ***********************************************************************/
static void ToDoListViewNextCategory (void)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 index;
	UInt16 category;
	FormPtr frm;
	ControlPtr ctl;
	UInt16 updateCode = updateCategoryChanged;

	category = globals->CurrentCategory;

	// Find the next category that has displayable items.
	do
		{
		globals->CurrentCategory = CategoryGetNext (globals->ToDoDB, globals->CurrentCategory);
		
		index = 0;
		if (SeekRecord (&index, 0, dmSeekForward))
			break;
		}
	while (globals->CurrentCategory != dmAllCategories);


	if (category == globals->CurrentCategory) return;

	// If the option for category column is set and we switched to/from "All",
	// the table will need to be rebuilt with/without the column
	if ( globals->ShowCategories &&
			( (category == dmAllCategories) || (globals->CurrentCategory == dmAllCategories) ))
		{
		updateCode = updateDisplayOptsChanged;
		}
		
	if (globals->CurrentCategory == dmAllCategories)
		globals->ShowAllCategories = true;
	else
		globals->ShowAllCategories = false;

	ChangeCategory (globals->CurrentCategory);

	// Set the label of the category trigger.
	frm = FrmGetActiveForm ();
	ctl = GetObjectPtr (ToDoListCategoryTrigger);
	CategoryGetName (globals->ToDoDB, globals->CurrentCategory, globals->CategoryName);
	CategorySetTriggerLabel (ctl, globals->CategoryName);


	// Display the new category.
	ToDoListViewDrawTable (updateCode);
}

/***********************************************************************
 *
 * FUNCTION:    ToDoListViewScroll
 *
 * DESCRIPTION: This routine scrolls the list of ToDo items
 *              in the direction specified.
 *
 * PARAMETERS:  direction - winUp or dowm
 *              oneLine   - if true the list is scrolled by a single line,
 *                          if false the list is scrolled by a full screen.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	02/21/95	Initial Revision
 *			rbb	04/14/99	Uses new ToDoListViewDrawTable
 *			gap	10/25/99	Optimized scrolling to only redraw if item position changed.
 *
 ***********************************************************************/
static void ToDoListViewScroll (WinDirectionType direction)
{
	globalVars* globals = getGlobalsPtr();
	UInt16			row;
	UInt16			index;
	UInt16			height;
	UInt16 			recordNum;
	UInt16 			columnWidth;
	UInt16 			tableHeight;
	TablePtr 		table;
	RectangleType	r;
	UInt16			prevTopVisibleRecord = globals->TopVisibleRecord;

	
	table = GetObjectPtr (ToDoListTable);
	TblReleaseFocus (table);

	globals->ToDoCurrentRecord = noRecordSelected;

	// Get the height of the table and the width of the description
	// column.
	TblGetBounds (table, &r);
	tableHeight = r.extent.y;
	height = 0;
	columnWidth = TblGetColumnWidth (table, descColumn);

	// Scroll the table down.
	if (direction == winDown)
		{
		// Get the record index of the last visible record.  A row 
		// number of minus one indicates that there are no visible rows.
		row = TblGetLastUsableRow (table);
		if (row == tblUnusableRow) return;
		
		recordNum = TblGetRowID (table, row);				

		// If there is only one record visible, this is the case 
		// when a record occupies the whole screeen, move to the 
		// next record.
		if (row == 0)
			SeekRecord (&recordNum, 1, dmSeekForward);
		}

	// Scroll the table up.
	else
		{
		// Scan the records before the first visible record to determine 
		// how many record we need to scroll.  Since the heights of the 
		// records vary,  we sum the heights of the records until we get
		// a screen full.
		recordNum = TblGetRowID (table, 0);
		height = TblGetRowHeight (table, 0);
		if (height >= tableHeight)
			height = 0;

		while (height < tableHeight)
			{
			index = recordNum;
			if ( ! SeekRecord (&index, 1, dmSeekBackward) ) break;
			height += ToDoListViewGetDescriptionHeight (index, columnWidth, tableHeight);
			if ((height <= tableHeight) || (recordNum == TblGetRowID (table, 0)))
				recordNum = index;
			}
		}

	globals->TopVisibleRecord = recordNum;
	ToDoListViewLoadTable (true);
	
	// Need to compare the previous top record to the current after ToDoListViewLoadTable 
	// as it will adjust TopVisibleRecord if drawing from recordNum will not fill the 
	// whole screen with items.
	if (globals->TopVisibleRecord != prevTopVisibleRecord)
		TblRedrawTable (table);
}

/***********************************************************************
 *
 * FUNCTION:    ToDoListViewGetColumnWidth
 *
 * DESCRIPTION: This routine returns the width of the specified 
 *              column.
 *
 * PARAMETERS:	 column - column of the list table
 *
 * RETURNED:	 width of the column in pixels
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/10/97	Initial Revision
 *
 ***********************************************************************/
static UInt16 ToDoListViewGetColumnWidth (Int16 column)
{
	Char		chr;
	Char		dateBuffer [dateStringLength];
	UInt16	width;
	FontID	curFont;
	Char*		dateStr;
	globalVars* globals = getGlobalsPtr();

	if (column == priorityColumn)
		{
		curFont = FntSetFont (ToDoListViewPriorityFontID());
		}
	else
		{
		curFont = FntSetFont (globals->ListFont);
		}

	if (column == priorityColumn)
		{
		chr = '1';
		width = (FntCharWidth (chr) - 1) + 6;
		}

	else if (column == dueDateColumn)
		{
		DateToAscii (12, 31, 1997,	globals->DateFormat, dateBuffer);

		// Remove the year from the date string.
		dateStr = dateBuffer;
		if ((globals->DateFormat == dfYMDWithSlashes) ||
			 (globals->DateFormat == dfYMDWithDots) ||
			 (globals->DateFormat == dfYMDWithDashes))
			dateStr += 3;
		else
			dateStr[StrLen(dateStr) - 3] = 0;

		width = FntCharsWidth (dateStr, StrLen (dateStr));

		// Get the width of the character that indicates the item is due.
		// Don't count the whitespace in the character. Handle auto-bolding
		// of list font for the priority number.
		FntSetFont (ToDoListViewPriorityFontID());
		chr = '!';
		width += FntCharWidth (chr) - 1;
		}

	// Size the category column such that is can display about five 
	// characters.
	else if (column == categoryColumn)
		{
		chr = '1';
		width = (FntCharWidth (chr) * 5) - 1;
		}

	FntSetFont (curFont);
	
	return (width);
}

/***********************************************************************
 *
 * FUNCTION:    ToDoListViewInit
 *
 * DESCRIPTION: This routine initializes the "List View" of the 
 *              ToDo application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *			rbb	4/14/99	Only show category column when viewing "All"
 *			jmp	11/27/99	Fix long-standing bug where priority column's spacing
 *								would change depending on whether we were being initialized
 *								for the first time or we were being re-inited (as during
 *								an update event or such).
 *
 ***********************************************************************/
static void ToDoListViewInit (FormPtr frm)
{
	Int16 row;
	Int16 rowsInTable;
	UInt16 width;
	FontID fontID;
	TablePtr table;
	ControlPtr ctl;
	RectangleType r;
	globalVars* globals = getGlobalsPtr();
	Boolean showCategories = globals->ShowCategories && (globals->CurrentCategory == dmAllCategories);

	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ToDoListTable));
	
	rowsInTable = TblGetNumberOfRows (table);
	for (row = 0; row < rowsInTable; row++)
		{		
		TblSetItemStyle (table, row, completedColumn, checkboxTableItem);
		TblSetItemStyle (table, row, priorityColumn, numericTableItem);
		TblSetItemStyle (table, row, descColumn, textTableItem);
		TblSetItemStyle (table, row, dueDateColumn, customTableItem);
		TblSetItemStyle (table, row, categoryColumn, customTableItem);

		// Set the font used to draw the text of the row. We automatically
		// bold priority number.
		fontID = ToDoListViewPriorityFontID();

		TblSetItemFont (table, row, priorityColumn, fontID);
		TblSetItemFont (table, row, descColumn, globals->ListFont);
		TblSetItemFont (table, row, dueDateColumn, globals->ListFont);
		TblSetItemFont (table, row, categoryColumn, globals->ListFont);

		TblSetRowUsable (table, row, false);
		}

	TblSetColumnUsable (table, completedColumn, true);
	TblSetColumnUsable (table, priorityColumn, globals->ShowPriorities);
	TblSetColumnUsable (table, descColumn, true);
	TblSetColumnUsable (table, dueDateColumn, globals->ShowDueDates);
	TblSetColumnUsable (table, categoryColumn, showCategories);
	
	//Set up to mask all columns
	TblSetColumnMasked (table, completedColumn, true);
	TblSetColumnMasked (table, priorityColumn, true);
	TblSetColumnMasked (table, descColumn, true);
	TblSetColumnMasked (table, dueDateColumn, true);
	TblSetColumnMasked (table, categoryColumn, true);
	
	
	// Set the spacing after the complete column.
	if (globals->ShowPriorities)
		{
		TblSetColumnSpacing (table, completedColumn, 0);
		TblSetColumnSpacing (table, priorityColumn, spaceBeforeDesc);
		}
	else
		{
		TblSetColumnSpacing (table, completedColumn, spaceBeforeDesc);
		TblSetColumnSpacing (table, priorityColumn, spaceNoPriority);
		}	

	if (globals->ShowDueDates && showCategories)
		{
		TblSetColumnSpacing (table, dueDateColumn, spaceBeforeCategory);
		}


	// Set the width of the priorities column.
	if (globals->ShowPriorities)
		{
		width = ToDoListViewGetColumnWidth (priorityColumn);
		TblSetColumnWidth (table, priorityColumn, width);
		}

	// Set the width of the due date column.
	if (globals->ShowDueDates)
		{
		width = ToDoListViewGetColumnWidth (dueDateColumn);
		TblSetColumnWidth (table, dueDateColumn, width);
		}

	// Set the width of the category column.
	if (showCategories)
		{
		width = ToDoListViewGetColumnWidth (categoryColumn);
		TblSetColumnWidth (table, categoryColumn, width);
		}

	// Set the width of the description column.
	TblGetBounds (table, &r);
	width = r.extent.x;
	width -= TblGetColumnWidth (table, completedColumn) + 
				TblGetColumnSpacing (table, completedColumn);
	width -= TblGetColumnSpacing (table, descColumn);
	if (globals->ShowPriorities)
		width -= TblGetColumnWidth (table, priorityColumn) + 
				   TblGetColumnSpacing (table, priorityColumn);
	if (globals->ShowDueDates)
		width -= TblGetColumnWidth (table, dueDateColumn) + 
				   TblGetColumnSpacing (table, dueDateColumn);
	if (showCategories)
		width -= TblGetColumnWidth (table, categoryColumn) + 
				   TblGetColumnSpacing (table, categoryColumn);


	TblSetColumnWidth (table, descColumn, width);	


	// Set the callback routines that will load and save the 
	// description field.
	TblSetLoadDataProcedure (table, descColumn, (TableLoadDataFuncPtr)ToDoListViewGetDescription);
	// Set the callback routine that draws the due date field.
	TblSetCustomDrawProcedure (table, dueDateColumn, ToDoListViewDrawDueDate);

	// Set the callback routine that draws the category field.
	TblSetCustomDrawProcedure (table, categoryColumn, ToDoListViewDrawCategory);

	ToDoListViewLoadTable (true);

	// Set the label of the category trigger.
	ctl = GetObjectPtr (ToDoListCategoryTrigger);
	CategoryGetName (globals->ToDoDB, globals->CurrentCategory, globals->CategoryName);
	CategorySetTriggerLabel (ctl, globals->CategoryName);
}


/***********************************************************************
 *
 * FUNCTION:    ToDoListViewUpdateDisplay
 *
 * DESCRIPTION: This routine updates the display of the List View
 *
 * PARAMETERS:  updateCode - a code that indicated what changes have been
 *                           made to the ToDo list.
 *                		
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/2/95	Initial Revision
 *			rbb	4/14/99	Uses new ToDoListViewDrawTable and ToDoListViewRedrawTable
 *			jmp	11/01/99	Fixed problem on frmRedrawUpdateCode events when
 *								we're still in the edit state but we weren't redrawing
 *								the edit indicator.  Fixes ToDo part of bug #23235.
 *			jmp	11/15/99	Make sure to clear the edit state on updateItemDelete and
 *								updateItemHide!
 *			jmp	11/27/99	Make sure we call ToDoListViewInit() on frmRedrawUpdateCodes,
 *								otherwise column spacing may not be correct.
 *			jmp	12/09/99	Oops, calling ToDoListViewInit() on frmRedrawUpdateCodes actually
 *								causes us mucho grief.  Also, don't call ToDoListViewRedrawTable()
 *								when TblRedrawTable() will do.  Fixes bug #23914.
 *
 ***********************************************************************/
static Boolean ToDoListViewUpdateDisplay (UInt16 updateCode)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 attr;
	Int16 row;
	Int16 column;
	UInt16 numRows;
	UInt16 recordNum;
	UInt32 uniqueID;
	TablePtr table;
	ControlPtr ctl;
	
	table = GetObjectPtr (ToDoListTable);

	// Was the UI unable to save an image of the ToDoListView when it 
	// obscured part of the view with another dialog?  If not,
	// we'll handle the event here.
	if (updateCode & frmRedrawUpdateCode)
		{
		FrmDrawForm(FrmGetActiveForm());
		
		// If we're editing, then find out which row is being edited,
		// mark it invalid, and redraw the table.
		if (TblEditing(table))
			{
			TblGetSelection(table, &row, &column);
			TblMarkRowInvalid(table, row);
			TblRedrawTable(table);
			}
		return (true);
		}

	// Were the display options modified (ToDoOption dialog) or was the 
	// font changed?
	if (updateCode & (updateDisplayOptsChanged | updateFontChanged))
		{
		ToDoListViewDrawTable (updateCode);
		return (true);
		}

	// Was the category of an item changed?
	else if (updateCode & updateCategoryChanged)
		{
		if (globals->ShowAllCategories)
			globals->CurrentCategory = dmAllCategories;
		else
			{
			DmRecordInfo (globals->ToDoDB, globals->ToDoCurrentRecord, &attr, NULL, NULL);
			globals->CurrentCategory = attr & dmRecAttrCategoryMask;
			}
		// Set the label of the category trigger.
		ctl = GetObjectPtr (ToDoListCategoryTrigger);
		CategoryGetName (globals->ToDoDB, globals->CurrentCategory, globals->CategoryName);
		CategorySetTriggerLabel (ctl, globals->CategoryName);

		globals->TopVisibleRecord = globals->ToDoCurrentRecord;
		}

	// Was an item deleted or marked secret? If so, invalidate all the rows 
	// following the deleted/secret record.  Also, make sure the edit
	// state is now clear.
	if ( (updateCode & updateItemDelete) || (updateCode & updateItemHide))
		{
		TblGetSelection (table, &row, &column);
		numRows = TblGetNumberOfRows (table);
		for ( ; row < numRows; row++)
			TblSetRowUsable (table, row, false);
			
		}

	// Was the item moved?
	// Items are moved when their priority or due date is changed.  
	else if (updateCode & updateItemMove)
		{

		// Always redraw the current record
		DmRecordInfo (globals->ToDoDB, globals->ToDoCurrentRecord, NULL, &uniqueID, NULL);
		if (TblFindRowData (table, uniqueID, &row))
			TblSetRowUsable (table, row, false);

		// We don't want to scroll the current record into view.
		recordNum = globals->ToDoCurrentRecord;
		globals->ToDoCurrentRecord = noRecordSelected;
		
		ToDoListViewRedrawTable (true);

		// If the item is still visible we will restore the edit state.
		globals->ToDoCurrentRecord = recordNum;
		
		return (true);
		}

	ToDoListViewRedrawTable (true);

	return (true);
}

static void ListOnOK()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 recordNum;
	Int16 row, column;
	TablePtr table = GetObjectPtr (ToDoListTable);
	if(TblGetSelection(table, &row, &column) == false)
	{
		globals->gLinkID = 0;
	}
	else
	{
		recordNum = TblGetRowID (table, row);
		if(DmRecordInfo (globals->ToDoDB, recordNum, NULL, &globals->gLinkID, NULL)!=errNone)
			globals->gLinkID = 0;
	}
	StopApplication();
	FrmUpdateForm(NewLinkDialog, 0);
	FrmUpdateForm (EditView, updatePrefs);
	FrmReturnToForm(0);
}

static void ListOnCancel()
{
	globalVars* globals = getGlobalsPtr();
	globals->gLinkID = 0;
	StopApplication();
	FrmUpdateForm(NewLinkDialog, 0);
	FrmUpdateForm (EditView, updatePrefs);
	FrmReturnToForm(0);
}

/***********************************************************************
 *
 * FUNCTION:    ToDoListViewHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "List View"
 *              of the ToDo application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	2/21/95		Initial Revision
 *			kcr	10/11/95		set initial shift for new todos created via
 *									'New' button, and for re-edit of empty desc field.
 *			kwk	11/21/98		Handle command keys in separate code block, so
 *									TxtCharIsPrint doesn't get called w/virtual chars.
 *			rbb	4/14/99		Uses new ToDoListViewDrawTable
 *			jmp	11/01/99		Don't call ToDoListViewRestoreEditState() at frmUpdateForm
 *									time when the updateCode is frmRedrawUpdateCode because
 *									the edit state is either still valid or it has been
 *									cleared, and we've handled either item elsewhere.
 *			jmp	11/16/99		Release the table focus around the send item event.
 *									This fixes bug #24067 and makes this code consistent with
 *									what the Datebook does.
 *
 ***********************************************************************/
Boolean ToDoListViewHandleEvent (EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm;
	TablePtr table;
	Boolean handled = false;

	if (event->eType == keyDownEvent)
	{
	if (EvtKeydownIsVirtual(event))
		{
		// Scroll up key pressed?
		if (event->data.keyDown.chr == vchrPageUp)
			{
			ToDoListViewScroll (winUp);
			handled = true;
			}

		// Scroll down key pressed?
		else if (event->data.keyDown.chr == vchrPageDown)
			{
			ToDoListViewScroll (winDown);
			handled = true;
			}

			handled = true;
		}
	}

		// If the character is printable, then either auto-create a new entry,
		// or fix up Graffiti auto-shifting.
		
		
	else if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case ToDoListUpButton:
				ToDoListViewScroll (winUp);
				handled = true;
				break;
				
			case ToDoListDownButton:
				ToDoListViewScroll (winDown);
				handled = true;
				break;
				
			case ToDoListCategoryTrigger:
				ToDoListViewSelectCategory ();
				handled = true;
				break;
			case ToDoListOK:
				ListOnOK();
				handled = true;
				break;
			case ToDoListCancel:
				ListOnCancel();
				handled = true;
				break;
			
			}
		}


	else if (event->eType == ctlRepeatEvent)
		{
		switch (event->data.ctlRepeat.controlID)
			{
			case ToDoListUpButton:
				ToDoListViewScroll (winUp);
				break;
				
			case ToDoListDownButton:
				ToDoListViewScroll (winDown);
				break;
			}
		}

	else if (event->eType == tblEnterEvent)
		{
		table = GetObjectPtr (ToDoListTable);
	
		TblSelectItem(table, event->data.tblEnter.row, 1);
		handled = true;
		}
	
	
	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		StartApplication();
		ToDoListViewInit (frm);
		FrmDrawForm (frm);
		if (globals->PendingUpdate)
			{
			ToDoListViewUpdateDisplay (globals->PendingUpdate);
			globals->PendingUpdate = 0;
			}
		handled = true;
		}

	else if (event->eType == frmCloseEvent)
		{
		StopApplication();
		}

	/*else if (event->eType == frmUpdateEvent)
		{
		handled = ToDoListViewUpdateDisplay (event->data.frmUpdate.updateCode);
		if (handled && (event->data.frmUpdate.updateCode != frmRedrawUpdateCode))
			ToDoListViewRestoreEditState ();
		}*/

	return (handled);
}




/***********************************************************************
 *
 * FUNCTION:    ToDoLoadPrefs
 *
 * DESCRIPTION: Read the preferences and handle previous and future
 *					 versions of the prefs.
 *
 * PARAMETERS:  Nothing.
 *
 * RETURNED:    Nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			BGT	01/08/98	Initial Revision
 *			kwk	06/23/99	Use glue code for default Note/ListFont if prefs
 *								don't provide that information.
 *
 ***********************************************************************/
 void ToDoLoadPrefs(void)
 {
	ToDoPreferenceType prefs;
	UInt16 prefsSize;
	Int16 prefsVersion;
	Boolean contacts;
	globalVars* globals = getGlobalsPtr();	
	contacts = (DmFindDatabase(0, "ContactsDB-PAdd")!=0);
	// Read the preferences / saved-state information. If we get an older version of
	// the prefs, sync our new note font field with the original pref field.
	prefsSize = sizeof (ToDoPreferenceType);
	
	if(contacts)
	{
		prefsVersion = PrefGetAppPreferences ('PTod', todoPrefID, &prefs, &prefsSize, true);
	}
	else
	{
		prefsVersion = PrefGetAppPreferences (sysFileCToDo, todoPrefID, &prefs, &prefsSize, true);
	}
	
	if (prefsVersion > toDoPrefsVersionNum) {
		prefsVersion = noPreferenceFound;
	}
	if (prefsVersion > noPreferenceFound)
		{
		if (prefsVersion < toDoPrefsVersionNum) {
			prefs.noteFont = prefs.v20NoteFont;
		}
		globals->CurrentCategory = prefs.currentCategory;
		
		// Since the FontSelect call only lets the user pick the std, stdBold, and largeBold
		// fonts, remap the largeFont to largeBoldFont. DOLATER kwk - this code makes assumptions
		// about how the FontSelect call works, which might not be true in the future, especially
		// for other character encodings.
		if (prefs.noteFont == largeFont)
			globals->NoteFont = largeBoldFont;
		else
			globals->NoteFont = prefs.noteFont;
		
		globals->ShowAllCategories = false;//prefs.showAllCategories;
		if(!contacts)
			globals->ShowCompletedItems = prefs.showCompletedItems;
		else
			globals->ShowCompletedItems = prefs.showAllCategories;
		
		globals->ShowOnlyDueItems = false;//prefs.showOnlyDueItems;
		globals->ShowDueDates = true;//prefs.showDueDates;
		globals->ShowPriorities = true;//prefs.showPriorities;
		globals->ShowCategories = false;
		//ChangeDueDate = prefs.changeDueDate;
		//SaveBackup = prefs.saveBackup;
		
		
		// Support transferal of preferences from the second version of the preferences.
		if (prefsVersion == toDoPrefsVersionNum)
			{
			globals->ListFont = prefs.listFont;
			}
		}
	else
		{
		globals->ListFont = stdFont;
		globals->NoteFont = stdFont;
		}
}



