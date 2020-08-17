/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MemoMain.c
 *
 * Description:
 *	  This is the Memo application's main module.  This module
 *   starts the application, dispatches events, and stops
 *   the application. 
 *
 * History:
 *		Feb 21, 1995	Created by Art Lamb
 *
 *****************************************************************************/

// general Palm OS includes
#include <PalmOS.h>

// private Memo includes
#include "MemoDB.h"
#include "../AddressRsc.h"
#include "../Address.h"
#include "MemoMain.h"
#include "../dia.h"
#include "../AddrTools.h"


//#define min(a,b) (a<b? a:b)

/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/
#define memoVersionNum					3
#define memoPrefsVersionNum			3
#define memoPrefID						0x00


#define newMemoSize  					64

#define noRecordSelected				0xffff
#define noRecordSelectedID				-1


//The listViewIndexStringSize is the size of the character array
//that holds the string representation of the index that is displayed to
//the left of the memo title in the list view.  The string can have a 
//range of 1 - 99,999 with the current value.
#define listViewIndexStringSize		7		

/***********************************************************************
 *
 *	Internal Structures
 *
 ***********************************************************************/

typedef struct {
	UInt16			topVisibleRecord;
	UInt16			currentRecord;
	UInt16			currentView;
	UInt16			currentCategory;
	FontID			v20editFont;
	UInt8				reserved1;
	UInt16			editScrollPosition;
	Boolean			showAllCategories;
	UInt8				reserved2;
	UInt32			currentRecordID;
	Boolean			saveBackup;
	
	// Version 2 preferences
	FontID			v20listFont;
	
	// Version 3 preferences
	FontID			editFont;
	FontID			listFont;
} MemoPreferenceType;

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
static Boolean EditViewDeleteRecord (void);
static void MemoLoadPrefs(UInt32*	currentRecordID);
static void MemoSavePrefs(UInt16 scrollPosition);
static Boolean MemoListViewUpdateDisplay (UInt16 updateCode);

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
 *			jmp	10/2/99	Call new MemoGetDataBase() to create database
 *								if it doesn't already exist.
 *
 ***********************************************************************/
static void StopMemo (void)
{
	globalVars* globals = getGlobalsPtr();
	DmCloseDatabase (globals->MemoDB);	
}

static UInt16 StartApplication (void)
{
	globalVars* globals = getGlobalsPtr();
	Err err = 0;
	UInt16 attr;
	UInt16 mode;
	UInt32 uniqueID;
	UInt32 currentRecordID = 0;
	Boolean recordFound = false;
	
	globals->gLinkID = 0;
	
	// Determime if secert record should be shown.
	globals->PrivateRecordVisualStatus = (privateRecordViewEnum)PrefGetPreference (prefShowPrivateRecords);
	if (globals->PrivateRecordVisualStatus == hidePrivateRecords)
		{
		mode = dmModeReadWrite;
		}
	else
		{
		mode = dmModeReadWrite | dmModeShowSecret;
		}

	// Find the application's data file.  If it doesn't exist, create it.
	err = MemoGetDatabase (&globals->MemoDB, mode);
	if (err)
		return err;

	// Read the preferences.
	MemoLoadPrefs(&currentRecordID);

	// The file may have been synchronized since the last time we used it, 
	// check that the current record and the currrent category still
	// exist.  Also, if secret records are being hidden, check if the 
	// the current record is marked secret.
	CategoryGetName (globals->MemoDB, globals->CurrentCategory, globals->CategoryName);
	if (*(globals->CategoryName) == 0)
		{
		globals->CurrentCategory = dmAllCategories;
		globals->ShowAllCategories = true;
		}

	if ( DmQueryRecord (globals->MemoDB, globals->MemoCurrentRecord) != 0)
		{
		DmRecordInfo (globals->MemoDB, globals->MemoCurrentRecord, &attr, &uniqueID, NULL);
		recordFound = (uniqueID == currentRecordID) &&
						  ((globals->PrivateRecordVisualStatus == showPrivateRecords) || 
						   (!(attr & dmRecAttrSecret)));
		}

	if (! recordFound)
		{
		globals->TopVisibleRecord = 0;
		globals->MemoCurrentRecord = noRecordSelected;
		globals->MemoCurrentView = MemoListView;
		globals->MemoEditScrollPosition = 0;
		}
	
	if (globals->ShowAllCategories)
		globals->MemosInCategory = DmNumRecordsInCategory (globals->MemoDB, dmAllCategories);
	else
		globals->MemosInCategory = DmNumRecordsInCategory (globals->MemoDB, globals->CurrentCategory);
	
	return (err);
}

/***********************************************************************
 *
 * FUNCTION:    MemoLoadPrefs
 *
 * DESCRIPTION: Load the preferences and do any fixups needed for backwards
 *					 and forwards compatibility
 *
 * PARAMETERS:  currentRecordID <- returned record id from preferences.
 *
 * RETURNED:    nothing
 *
 *	HISTORY:
 *		01/13/98	BGT	Initial Revision.
 *		08/04/99	kwk	Cleaned up setting EditFont/ListFont from prefs.
 *
 ***********************************************************************/
void MemoLoadPrefs(UInt32*	currentRecordID)
{
	globalVars* globals = getGlobalsPtr();
	MemoPreferenceType prefs;
	UInt16 prefsSize;
	Int16 prefsVersion;
	Boolean needFontInfo = false;
	
	// Read the preferences / saved-state information.
	prefsSize = sizeof (MemoPreferenceType);
	prefsVersion = PrefGetAppPreferences (sysFileCMemo, memoPrefID, &prefs, &prefsSize, true);
	if (prefsVersion > memoPrefsVersionNum) {
		prefsVersion = noPreferenceFound;
	}
	
	if (prefsVersion > noPreferenceFound)
		{
		// Try to carry forward the version 2 preferences for the font
		if (prefsVersion < 2)
			{
			// No font data in original prefs
			needFontInfo = true;
			}
		else if (prefsVersion == 2)
			{
			prefs.editFont = prefs.v20editFont;
			prefs.listFont = prefs.v20listFont;
			
			// Use the 'better' large font if we've got it, since version 2
			// prefs would have been created on an older version of the OS
			// which didn't have the largeBoldFont available.
			if (prefs.editFont == largeFont)
				prefs.editFont = largeBoldFont;
			
			if (prefs.listFont == largeFont)
				prefs.listFont = largeBoldFont;
			}
		
		globals->TopVisibleRecord = prefs.topVisibleRecord;
		globals->MemoCurrentRecord = prefs.currentRecord;
		globals->MemoCurrentView = prefs.currentView;
		globals->CurrentCategory = prefs.currentCategory;
		globals->MemoEditScrollPosition = prefs.editScrollPosition;
		globals->ShowAllCategories = prefs.showAllCategories;
		globals->SaveBackup = prefs.saveBackup;
		*currentRecordID = prefs.currentRecordID;
		}
	else
		{
		needFontInfo = true;
		}
	
	
	// The first time this app starts register to handle vCard data.
	if (prefsVersion != memoPrefsVersionNum)
		ExgRegisterData(sysFileCMemo, exgRegExtensionID, "txt");
   
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
 *			art	2/21/95	Initial Revision
 *
 ***********************************************************************/
static void * GetObjectPtr (UInt16 objectID)
{
	FormPtr frm;
	
	frm = FrmGetActiveForm ();
	return (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, objectID)));

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
	
	frm = FrmGetActiveForm ();
	focus = FrmGetFocus (frm);
	if (focus == noFocus)
		return (NULL);
		
	return (FrmGetObjectPtr (frm, focus));
}


/***********************************************************************
 *
 * FUNCTION:    SeekRecord
 *
 * DESCRIPTION: Given the index of a 'to do' record, this routine scans 
 *              forwards or backwards for displayable 'to do' records.           
 *
 * PARAMETERS:  indexP  - pointer to the index of a record to start from;
 *                        the index of the record sought is returned in
 *                        this parameter.
 *
 *              offset  - number of records to skip:   
 *                        	0 - mean seek from the current record to the
 *                             next display record, if the current record is
 *                             a displayable record, its index is retuned.
 *                         1 - mean seek foreward, skipping one displayable 
 *                             record
 *                        -1 - menas seek backwards, skipping one 
 *                             displayable record
 *                             
 *
 * RETURNED:    false is return if a displayable record was not found.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/5/95	Initial Revision
 *
 ***********************************************************************/
static Boolean SeekRecord (UInt16 * indexP, Int16 offset, Int16 direction)
{
	globalVars* globals = getGlobalsPtr();
	DmSeekRecordInCategory (globals->MemoDB, indexP, offset, direction, globals->CurrentCategory);
	if (DmGetLastErr()) return (false);
	
	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    ChangeCategory
 *
 * DESCRIPTION: This routine updates the global varibles that keep track
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
	if (globals->ShowAllCategories)
		globals->MemosInCategory = DmNumRecordsInCategory (globals->MemoDB, dmAllCategories);
	else
		globals->MemosInCategory = DmNumRecordsInCategory (globals->MemoDB, category);
		
	globals->CurrentCategory = category;
	globals->TopVisibleRecord = 0;
}


/***********************************************************************
 *
 * FUNCTION:    DrawMemoTitle
 *
 * DESCRIPTION: This routine draws the title of the specified memo. 
 *
 * PARAMETERS:	 memo  - pointer to a memo
 *              x     - draw position
 *              y     - draw position
 *              width - maximum width to draw.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	04/18/95	Initial Revision
 *			roger	07/27/95	Combined both cases
 *			kwk	05/15/99	Use Int'l code for truncation of title.
 *
 ***********************************************************************/
void DrawMemoTitle (Char * memo, Int16 x, Int16 y, Int16 width)
{
	Char * ptr = StrChr (memo, linefeedChr);
	UInt16 titleLen = (ptr == NULL ? StrLen (memo) : (UInt16) (ptr - memo));
	if (FntWidthToOffset (memo, titleLen, width, NULL, NULL) == titleLen)
		{
		WinDrawChars (memo, titleLen, x, y);
		}
	else
		{
		Int16 titleWidth;
		titleLen = FntWidthToOffset (memo, titleLen, width - FntCharWidth (chrEllipsis), NULL, &titleWidth);
		WinDrawChars (memo, titleLen, x, y);
		WinDrawChar (chrEllipsis, x + titleWidth, y);
		}
}


/***********************************************************************
 *
 * FUNCTION:    DrawInversionEffect
 *
 * DESCRIPTION: This routine does an inversion effect by swapping colors
 *					 this is NOT undoable by calling it a second time, rather
 *					 it just applies a selected look on top of already
 *					 rendered data.  (It's kind of a hack.)
 *
 * PARAMETERS:	 rP  - pointer to a rectangle to 'invert'
 *					 cornerDiam	- corner diameter
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	10/28/99	Initial revision
 *
 ***********************************************************************/
static void DrawInversionEffect (const RectangleType *rP, UInt16 cornerDiam) 
{
	WinPushDrawState();
	WinSetDrawMode(winSwap);
	WinSetPatternType (blackPattern);
	WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetTableEntryIndex(UIObjectSelectedFill));
	WinPaintRectangle(rP, cornerDiam);
	
	if (UIColorGetTableEntryIndex(UIObjectSelectedFill) != UIColorGetTableEntryIndex(UIObjectForeground)) {
		WinSetBackColor(UIColorGetTableEntryIndex(UIObjectForeground));
		WinSetForeColor(UIColorGetTableEntryIndex(UIObjectSelectedForeground));
		WinPaintRectangle(rP, cornerDiam);
	}
	WinPopDrawState();
}

/***********************************************************************
 *
 * FUNCTION:    MemoListViewNumberOfRows
 *
 * DESCRIPTION: This routine return the maximun number of visible rows,
 *              with the current list view font setting.
 *
 * PARAMETERS:  table - List View table
 *
 * RETURNED:    maximun number of displayable rows
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/28/97	Initial Revision
 *
 ***********************************************************************/
static UInt16 MemoListViewNumberOfRows (TablePtr table)
{
	UInt16				rows;
	UInt16				rowsInTable;
	UInt16				tableHeight;
	FontID			currFont;
	RectangleType	r;


	rowsInTable = TblGetNumberOfRows (table);

	TblGetBounds (table, &r);
	tableHeight = r.extent.y;

	currFont = FntSetFont (stdFont);
	rows = tableHeight / FntLineHeight ();
	FntSetFont (currFont);

	if (rows <= rowsInTable)
		return (rows);
	else
		return (rowsInTable);
}


/***********************************************************************
 *
 * FUNCTION:    MemoListViewDrawRecord
 *
 * DESCRIPTION: This routine draws the title memo record in the list 
 *              view.  This routine is called by the table routine, 
 *              TblDrawTable, each time a line of the table needs to
 *              be drawn.
 *
 * PARAMETERS:  table  - pointer to the memo list table (TablePtr)
 *              row    - row of the table to draw
 *              column - column of the table to draw 
 *              bounds - bound to the draw region
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *			ADH	7/21/99	Increased the size of the posStr character array
 *								to allow for display of five digit numbers.
 *								Previously only four digit numbers could be 
 *								displayed.
 *
 ***********************************************************************/
static void MemoListViewDrawRecord (void * table, Int16 row, Int16 /* column */, 
	RectanglePtr bounds)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 len;
	UInt16 category;
	UInt16 recordNum;
	MemHandle memoH;
	Int16 x, y;
	Char * memoP;
	UInt16 pos;
	char posStr [6];
	UInt16 attr;
	RectangleType maskRectangle;
	
	// Get the record number that corresponds to the table item to draw.
	// The record number is stored in the "intValue" field of the item.
	recordNum = TblGetRowID (table, row);

	DmRecordInfo (globals->MemoDB, recordNum, &attr, NULL, NULL);
   // If the record is private and we are to hide private records, then get out of here.
   // This should be taken care of by the calling function, but we will go ahead and
   // take care of it here also.
   if ((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == hidePrivateRecords)
		{
		return;
		}
		
	x = bounds->topLeft.x + 1;
	y = bounds->topLeft.y;

	FntSetFont (stdFont);
	
	// Format the memo's postion, within its category, an draw it.
	if (globals->ShowAllCategories)
		category = dmAllCategories;
	else 
		category = globals->CurrentCategory;
	pos = DmPositionInCategory (globals->MemoDB, recordNum, category);
	StrIToA (posStr, pos+1);
	len = StrLen(posStr);
	ErrNonFatalDisplayIf(len > sizeof(posStr) - 2, "Too many records");
	posStr[len++] = '.';
	posStr[len] = 0;
	
	if (len < 3) x += FntCharWidth ('1');
	WinDrawChars (posStr, len, x, y);
	x += FntCharsWidth (posStr, len) + 4;

	// If we are here then we either we either mask the memo out or display the
	// memo title.
   	if (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == maskPrivateRecords))
		{
		MemMove (&maskRectangle, bounds, sizeof (RectangleType));
		maskRectangle.topLeft.x = x;
		maskRectangle.extent.x = bounds->extent.x - x;
		
		MemoListViewDisplayMask (&maskRectangle);
		}
	else
		{
		// Display the memo's title, the title is the first line of the memo.
		memoH = DmQueryRecord(globals->MemoDB, recordNum);
		memoP = MemHandleLock(memoH);
		DrawMemoTitle (memoP, x, y, bounds->extent.x - x);
		MemHandleUnlock(memoH);
		}
}

/***********************************************************************
 *
 * FUNCTION:    MemoListViewDisplayMask
 *
 * DESCRIPTION: Draws the masked display for the record.
 *
 * PARAMETERS:  bounds (Input):  The bounds of the table item to display.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         css    06/21/99   Initial Revision
 *
 ***********************************************************************/
void MemoListViewDisplayMask (RectanglePtr bounds)
{
	RectangleType tempRect;
	CustomPatternType origPattern;
	MemHandle	bitmapH;
	BitmapType * bitmapP;
	
	CustomPatternType pattern = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
	
	MemMove (&tempRect, bounds, sizeof (RectangleType));
	// Make sure it fits nicely into the display.
	tempRect.topLeft.y++;
	tempRect.extent.y --;
	tempRect.extent.x -= SecLockWidth + 3;
	
	WinGetPattern(&origPattern);
	WinSetPattern (&pattern);
	WinFillRectangle (&tempRect, 0);
	WinSetPattern(&origPattern);
	
	//draw lock icon
	bitmapH = DmGetResource (bitmapRsc, SecLockBitmap);
	if (bitmapH)
	{
		bitmapP = MemHandleLock (bitmapH);
		WinDrawBitmap (bitmapP, tempRect.topLeft.x + tempRect.extent.x + 1, 
										tempRect.topLeft.y + ((tempRect.extent.y - SecLockHeight) / 2));
		MemPtrUnlock (bitmapP);
	}
}

/***********************************************************************
 *
 * FUNCTION:    MemoListViewUpdateScrollers
 *
 * DESCRIPTION: This routine draws or erases the list view scroll arrow
 *              buttons.
 *
 * PARAMETERS:  frm          -  pointer to the to do list form
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/1/95	Initial Revision
 *
 ***********************************************************************/
static void MemoListViewUpdateScrollers (FormPtr /* frm */)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 pos;
	Int16 rows;
	UInt16 maxValue;

	rows = MemoListViewNumberOfRows (GetObjectPtr(MemoListTable));
	if (globals->MemosInCategory > rows)
		{
		pos = DmPositionInCategory (globals->MemoDB, globals->TopVisibleRecord, globals->CurrentCategory);
		maxValue = globals->MemosInCategory - rows;
		}
	else
		{
		pos = 0;
		maxValue = 0;
		}

	SclSetScrollBar (GetObjectPtr (MemoListScrollBar), pos, 0, maxValue, rows);
}


/***********************************************************************
 *
 * FUNCTION:    MemoListViewLoadTable
 *
 * DESCRIPTION: This routine loads memo database records into
 *              the list view form.
 *
 * PARAMETERS:  recordNum index of the first record to display.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/16/95	Initial Revision
 *			grant	1/29/99	Set the heights of unused rows
 *
 ***********************************************************************/
static void MemoListViewLoadTable (FormPtr frm)
{
	globalVars* globals = getGlobalsPtr();
	UInt16			row;
	UInt16			recordNum;
	UInt16			lineHeight;
	UInt16			dataHeight;
	UInt16			tableHeight;
	UInt16			numRows;
	UInt32			uniqueID;
	FontID			currFont;
	TablePtr 		table;
	MemHandle			recordH;
	RectangleType	r;

	
	table = GetObjectPtr (MemoListTable);

	TblGetBounds (table, &r);
	tableHeight = r.extent.y;

	currFont = FntSetFont (stdFont);
	lineHeight = FntLineHeight ();
	FntSetFont (currFont);

	dataHeight = 0;
	
	recordNum = globals->TopVisibleRecord;

	// For each row in the table, store the record number in the table item
	// that will dispaly the record.  
	numRows = TblGetNumberOfRows (table);
	for (row = 0; row < numRows; row++)
		{
		// Get the next record in the currunt category.
		recordH = DmQueryNextInCategory (globals->MemoDB, &recordNum, globals->CurrentCategory);
		
		// If the record was found, store the record number in the table item,
		// otherwise set the table row unusable.
		if (recordH && (tableHeight >= dataHeight + lineHeight))
			{
			TblSetRowID (table, row, recordNum);
			TblSetItemStyle (table, row, 0, customTableItem);
			TblSetItemFont (table, row, 0, stdFont);
			
			TblSetRowHeight (table, row, lineHeight);

			DmRecordInfo (globals->MemoDB, recordNum, NULL, &uniqueID, NULL);
			if ((TblGetRowData (table, row) != uniqueID) ||
				 ( ! TblRowUsable (table, row)))
				{
				TblSetRowUsable (table, row, true);
			TblSetRowSelectable(table, row, true);

				// Store the unique id of the record in the row.
				TblSetRowData (table, row, uniqueID);

				// Mark the row invalid so that it will draw when we call the 
				// draw routine.
				TblMarkRowInvalid (table, row);
				}

			if (row+1 < numRows) recordNum++;
			
			dataHeight += lineHeight;
			}
		else
			{
			// Set the row height - when scrolling winDown, the heights of the last rows of
			// the table are used to determine how far to scroll.  As rows are deleted
			// from the top of the table, formerly unused rows scroll into view, and the
			// height is used before the next call to MemoListViewLoadTable (which would set
			// the height correctly).
			TblSetRowHeight (table, row, lineHeight);
			
			TblSetRowUsable (table, row, false);
			TblSetRowSelectable(table, row, false);
			}
		}
		

	// Update the scroll arrows.
	MemoListViewUpdateScrollers (frm);
}


/***********************************************************************
 *
 * FUNCTION:    MemoListViewLoadRecords
 *
 * DESCRIPTION: This routine loads memo database records into
 *              the list view form.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/16/95		Initial Revision
 *
 ***********************************************************************/
static void MemoListViewLoadRecords (FormPtr frm)
{
	globalVars* globals = getGlobalsPtr();
	TablePtr 	table;
	UInt16			recordNum;
	UInt16			rowsInTable;
	
	if (globals->ShowAllCategories)
		globals->CurrentCategory = dmAllCategories;

	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MemoListTable));
	rowsInTable = MemoListViewNumberOfRows (table);

	// Is the current record before the first visible record?
	if (globals->MemoCurrentRecord != noRecordSelected)
		{
		if (globals->TopVisibleRecord > globals->MemoCurrentRecord)
			globals->TopVisibleRecord = globals->MemoCurrentRecord;
		
		// Is the current record after the last visible record?
		else
			{
			recordNum = globals->TopVisibleRecord;
			DmSeekRecordInCategory (globals->MemoDB, &recordNum, rowsInTable-1,
				dmSeekForward, globals->CurrentCategory);
			if (recordNum < globals->MemoCurrentRecord)
				globals->TopVisibleRecord = globals->MemoCurrentRecord;
			}
		}

	
	// Make sure we show a full display of records.
	if (globals->MemosInCategory)
		{
		recordNum = dmMaxRecordIndex;
		DmSeekRecordInCategory (globals->MemoDB, &recordNum, (rowsInTable-1),
			dmSeekBackward, globals->CurrentCategory);
		globals->TopVisibleRecord = min (globals->TopVisibleRecord, recordNum);
		}
	else
		globals->TopVisibleRecord = 0;

	MemoListViewLoadTable (frm);

	// Set the callback routine that will draw the records.
	TblSetCustomDrawProcedure (table, 0, MemoListViewDrawRecord);

	TblSetColumnUsable (table, 0, true);	
}


/***********************************************************************
 *
 * FUNCTION:    MemoListViewNextCategory
 *
 * DESCRIPTION: This routine display the next category,  if the last
 *              catagory isn't being displayed  
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
 *			art	9/15/95	Initial Revision
 *
 ***********************************************************************/
static void MemoListViewNextCategory (void)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 category;
	FormPtr frm;
	TablePtr table;
	ControlPtr ctl;

	category = CategoryGetNext (globals->MemoDB, globals->CurrentCategory);

	if (category != globals->CurrentCategory)
		{
		if (category == dmAllCategories)
			globals->ShowAllCategories = true;
		else
			globals->ShowAllCategories = false;

		ChangeCategory (category);

		// Set the label of the category trigger.
		ctl = GetObjectPtr (MemoListCategoryTrigger);
		CategoryGetName (globals->MemoDB, globals->CurrentCategory, globals->CategoryName);
		CategorySetTriggerLabel (ctl, globals->CategoryName);


		// Display the new category.
		globals->TopVisibleRecord = 0;
		frm = FrmGetActiveForm ();
		MemoListViewLoadTable (frm);
		table = GetObjectPtr (MemoListTable);
		TblEraseTable (table);
		TblDrawTable (table);
		}
}


/***********************************************************************
 *
 * FUNCTION:    MemoListViewPageScroll
 *
 * DESCRIPTION: This routine scrolls the list of of memo titles
 *              in the direction specified.
 *
 * PARAMETERS:  direction - winUp or dowm
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *			roger 7/27/95	Copied fixed code from Address Book
 *
 ***********************************************************************/
static void MemoListViewPageScroll (WinDirectionType direction)
{
	globalVars* globals = getGlobalsPtr();
	TablePtr table;
	Int16 rowsInTable;
	UInt16 newTopVisibleRecord;

	table = GetObjectPtr (MemoListTable);
	rowsInTable = MemoListViewNumberOfRows (table);

	newTopVisibleRecord = globals->TopVisibleRecord;
	globals->MemoCurrentRecord = noRecordSelected;

	// Scroll the table winDown a page (less one row).
	if (direction == winDown)
		{
		// Try going forward one page
		if (!SeekRecord (&newTopVisibleRecord, rowsInTable - 1, dmSeekForward))
			{
			// Try going backwards one page from the last record
			newTopVisibleRecord = dmMaxRecordIndex;
			if (!SeekRecord (&newTopVisibleRecord, rowsInTable - 1, dmSeekBackward))
				{
				// Not enough records to fill one page.  Start with the first record
				newTopVisibleRecord = 0;
				SeekRecord (&newTopVisibleRecord, 0, dmSeekForward);
				}
			}
		}
		
	// Scroll up a page (less one row).
	else
		{
		if (!SeekRecord (&newTopVisibleRecord, rowsInTable - 1, dmSeekBackward))
			{
			// Not enough records to fill one page.  Start with the first record
			newTopVisibleRecord = 0;
			SeekRecord (&newTopVisibleRecord, 0, dmSeekForward);
			}
		}
	


	// Avoid redraw if no change
	if (globals->TopVisibleRecord != newTopVisibleRecord)
		{
		globals->TopVisibleRecord = newTopVisibleRecord;
		MemoListViewLoadRecords (FrmGetActiveForm ());
		TblRedrawTable(table);
		}
}



/***********************************************************************
 *
 * FUNCTION:    MemoListViewScroll
 *
 * DESCRIPTION: This routine scrolls the list of of memo titles
 *              in the direction specified.
 *
 * PARAMETERS:  direction - winUp or dowm
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *			roger 7/27/95	Copied fixed code from Address Book
 *
 ***********************************************************************/
static void MemoListViewScroll (Int16 linesToScroll)
{
	globalVars* globals = getGlobalsPtr();
	Int16 				i;
	UInt16				rows;
	UInt16				lastRow;
	UInt16 				scrollAmount;
	UInt16 				newTopVisibleRecord;
	TablePtr 		table;
	RectangleType	scrollR;
	RectangleType	vacated;
	WinDirectionType	direction;


	table = GetObjectPtr (MemoListTable);
	globals->MemoCurrentRecord = noRecordSelected;


	// Find the new top visible record
	newTopVisibleRecord = globals->TopVisibleRecord;

	// Scroll down.
	if (linesToScroll > 0)
		SeekRecord (&newTopVisibleRecord, linesToScroll, dmSeekForward);

	// Scroll up.
	else if (linesToScroll < 0)
		SeekRecord (&newTopVisibleRecord, -linesToScroll, dmSeekBackward);

	ErrFatalDisplayIf (globals->TopVisibleRecord == newTopVisibleRecord, 
		"Invalid scroll value");

	globals->TopVisibleRecord = newTopVisibleRecord;


	// Move the bits that will remain visible.
	rows = MemoListViewNumberOfRows (table);
	if (((linesToScroll > 0) && (linesToScroll < rows)) ||
		 ((linesToScroll < 0) && (-linesToScroll < rows)))
		{
		scrollAmount = 0;
	
		if (linesToScroll > 0)
			{
			lastRow = TblGetLastUsableRow (table) - 1;
			for (i = 0; i < linesToScroll; i++)
				{
				scrollAmount += TblGetRowHeight (table, lastRow);
				TblRemoveRow (table, 0);
				}
			direction = winUp;
			}
		else
			{
			for (i = 0; i < -linesToScroll; i++)
				{
				scrollAmount += TblGetRowHeight (table, 0);
				TblInsertRow (table, 0);
				}
			direction = winDown;
			}

		TblGetBounds (table, &scrollR);
		WinScrollRectangle (&scrollR, direction, scrollAmount, &vacated);
		WinEraseRectangle (&vacated, 0);
		}
	

	MemoListViewLoadTable (FrmGetActiveForm ());
	TblRedrawTable(table);
}



/***********************************************************************
 *
 * FUNCTION:    MemoListViewInit
 *
 * DESCRIPTION: This routine initializes the "List View" of the 
 *              Memo application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95		Initial Revision
 *
 ***********************************************************************/
static void MemoListViewInit (FormPtr frm)
{
	globalVars* globals = getGlobalsPtr();
	ControlPtr ctl;
	
	MemoListViewLoadRecords (frm);

	// Set the label of the category trigger.
	ctl = GetObjectPtr (MemoListCategoryTrigger);
	CategoryGetName (globals->MemoDB, globals->CurrentCategory, globals->CategoryName);
	CategorySetTriggerLabel (ctl, globals->CategoryName);

}


/***********************************************************************
 *
 * FUNCTION:    MemoListViewInvertMoveIndicator
 *
 * DESCRIPTION:   If draw is true, then save the area behind the rectangle,
 *					then draw the indicator there. If draw is false, then restore 
 *					the screen bits.
 *              
 *
 * PARAMETERS:	 itemR - bounds of the move indicator
 *					 savedBits - if draw is true, then restore this window of bits at 
 *					 	itemR.
 *					 draw - draw or erase the move indicator. 
 *
 * RETURNED:	 WinHandle - handle to a saved window of screen bits, if the move
 *				indicator is visible. Otherwise, the value is 0.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/29/96	Initial Revision
 *
 ***********************************************************************/
static WinHandle MemoListViewInvertMoveIndicator (RectanglePtr itemR, WinHandle savedBits, 
	Boolean draw)
{
	UInt16 i;
	UInt16 err;
	WinHandle winH = 0;
	RectangleType indictatorR;
	CustomPatternType pattern;
	CustomPatternType savedPattern;


	indictatorR.topLeft.x = itemR->topLeft.x;
	indictatorR.topLeft.y = itemR->topLeft.y + itemR->extent.y - 2;
	indictatorR.extent.x = itemR->extent.x;
	indictatorR.extent.y = 2;
				
	if (draw)
		{
		WinGetPattern (&savedPattern);
	
		for (i = 0; i < sizeof (CustomPatternType) / sizeof (*pattern); i++)
			pattern[i]= 0xAA55;
	
		WinSetPattern (&pattern);
	
		winH = WinSaveBits (&indictatorR, &err);
	
		WinFillRectangle (&indictatorR, 0);
	
		WinSetPattern (&savedPattern);
		}
		
	else
		{
		WinRestoreBits (savedBits, indictatorR.topLeft.x, indictatorR.topLeft.y);
		}
	
	return (winH);
}


/***********************************************************************
 *
 * FUNCTION:    MemoListViewSelectTableItem
 *
 * DESCRIPTION: This routine either selects or unselects the specified
 *					 table item.				
 *
 * PARAMETERS:	 selected - specifies whether an item should be selected or
 *									unselected
 *					 table	 - pointer to a table object
 *              row      - row of the item (zero based)
 *              column   - column of the item (zero based)
 *              rP       - pointer to a structure that will hold the bound
 *                         of the item
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jmp	10/29/99	Initial Revision
 *			jmp	11/12/99	While a table item is "on the move," having it be selected
 *								can cause the Table code grief.  So, instead of using
 *								the TblSelectItem()/TblUnhighlightSelect() calls, we now
 *								manually select/unselect the table's row.  Before color,
 *								only WinInvertRectangle() was called, so this is now in line
 *								again with the way things used to work.  Sigh.
 *
 ***********************************************************************/

static void MemoListViewSelectTableItem (Boolean selected, TablePtr table, Int16 row, Int16 column, RectangleType *r)
{
	// Get the item's rectangle.
	//
	TblGetItemBounds (table, row, column, r);
	
	// Set up the drawing state the way we want it.
	//
	WinPushDrawState();
	WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetTableEntryIndex(UIObjectForeground));

	// Erase and (re)draw the item.
	//
	WinEraseRectangle(r, 0);
	MemoListViewDrawRecord(table, row, column, r);
	
	// If selected, make it look that way.
	//
	if (selected)
		DrawInversionEffect(r, 0);
		
	// Restore the previous drawing state.
	//
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION:    MemoListViewUpdateDisplay
 *
 * DESCRIPTION: This routine update the display of the list view
 *
 * PARAMETERS:  updateCode - a code that indicated what changes have been
 *                           made to the view.
 *                		
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/19/95	Initial Revision
 *
 ***********************************************************************/
static Boolean MemoListViewUpdateDisplay (UInt16 updateCode)
{
	globalVars* globals = getGlobalsPtr();
	TablePtr table;

	if (updateCode & (updateDisplayOptsChanged | updateFontChanged))
		{
		if (updateCode & updateDisplayOptsChanged)
			globals->TopVisibleRecord = 0;

		MemoListViewLoadRecords (FrmGetActiveForm());
		table = GetObjectPtr (MemoListTable);
		TblEraseTable (table);
		TblDrawTable (table);		

		return (true);
		}
		
	return (false);
}

/***********************************************************************
 *
 * FUNCTION:    MemoListViewSelectCategory
 *
 * DESCRIPTION: This routine handles selection, creation and deletion of
 *              categories form the Details Dialog.  
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
 *			art	03/10/95	Initial Revision
 *			gap	08/13/99	Update to use new constant categoryDefaultEditCategoryString.
 *
 ***********************************************************************/
static UInt16 MemoListViewSelectCategory (void)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm;
	TablePtr table;
	UInt16 category;
	Boolean categoryEdited;

	
	// Process the category popup list.  
	category = globals->CurrentCategory;

	frm = FrmGetActiveForm();
	categoryEdited = CategorySelect (globals->MemoDB, frm, MemoListCategoryTrigger,
					  MemoListCategoryList, true, &category, globals->CategoryName, 1, categoryHideEditCategory );
	
	if (category == dmAllCategories)
		globals->ShowAllCategories = true;
	else
		globals->ShowAllCategories = false;
		
	if (categoryEdited || (category != globals->CurrentCategory))
		{
		ChangeCategory (category);

		// Display the new category.
		MemoListViewLoadRecords (frm);
		table = GetObjectPtr (MemoListTable);
		TblEraseTable (table);
		TblDrawTable (table);
		}

	return (category);
}

static void ListOnOK()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 recordNum;
	Int16 row, column;
	DmOpenRef dbRef;
	TablePtr table = GetObjectPtr (MemoListTable);
	LocalID db;
	if(TblGetSelection(table, &row, &column) == false)
	{
		globals->gLinkID = 0;
	}
	else
	{
		recordNum = TblGetRowID (table, row);
		db = DmFindDatabase(0, "MemosDB-PMem");
		if(db == 0)
		{
			if(DmRecordInfo (globals->MemoDB, recordNum, NULL, &globals->gLinkID, NULL)!=errNone)
				globals->gLinkID = 0;
		}
		else
		{
			dbRef = DmOpenDatabase(0, db, dmModeReadOnly);
			if(DmRecordInfo (dbRef, recordNum, NULL, &globals->gLinkID, NULL)!=errNone)
				globals->gLinkID = 0;			
		}
	}
	StopMemo();
	FrmUpdateForm(NewLinkDialog, 0);
	FrmUpdateForm (EditView, updatePrefs);
	FrmReturnToForm(0);
}

static void ListOnCancel()
{
	globalVars* globals = getGlobalsPtr();
	globals->gLinkID = 0;
	StopMemo();
	FrmUpdateForm(NewLinkDialog, 0);
	FrmUpdateForm (EditView, updatePrefs);
	FrmReturnToForm(0);
}

/***********************************************************************
 *
 * FUNCTION:    MemoListViewHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "List View"
 *              of the Memo application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * HISTORY:
 *		02/21/95	art	Created by Art Lamb.
 *		11/22/98	kwk	Handle command keys in separate code block so that
 *							TxtCharIsPrint doesn't get called w/virtual chars.
 *		09/25/99	kwk	Use TxtGlueUpperChar to capitalize initial char for
 *							memo that's autocreated by writing a printable char.
 *
 ***********************************************************************/
Boolean MemoListViewHandleEvent (EventPtr event)
{
	FormPtr frm;
	Boolean handled = false;
	
	if (event->eType == keyDownEvent)
		{
		if (EvtKeydownIsVirtual(event))
			{
			// Scroll up key presed?
			if (event->data.keyDown.chr == vchrPageUp)
				{
				MemoListViewPageScroll (winUp);
				handled = true;
				}
	
			// Scroll down key presed?
			else if (event->data.keyDown.chr == vchrPageDown)
				{
				MemoListViewPageScroll (winDown);
				handled = true;
				}
			}		
		}

		
	else if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case MemoListCategoryTrigger:
				MemoListViewSelectCategory ();
				handled = true;
				break;
			case MemoListOK:
				ListOnOK ();
				handled = true;
				break;
			case MemoListCancel:
				ListOnCancel ();
				handled = true;
				break;
			}
		}
		
	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		dia_save_state(); 
		dia_enable(frm, false);
		StartApplication();
		MemoListViewInit (frm);
		FrmDrawForm (frm);
		handled = true;
		}

	else if (event->eType == frmCloseEvent)
		{
		StopMemo();
		}

	else if (event->eType == frmUpdateEvent)
		{
		handled =  MemoListViewUpdateDisplay (event->data.frmUpdate.updateCode);
		}

	else if (event->eType == sclRepeatEvent)
		{
		MemoListViewScroll (event->data.sclRepeat.newValue - 
			event->data.sclRepeat.value);
		}
	else if(event->eType == winDisplayChangedEvent)
	{
		dia_display_changed();
		FrmUpdateForm(FrmGetActiveFormID(), frmRedrawUpdateCode);
		handled = true;
	}

	return (handled);
}






