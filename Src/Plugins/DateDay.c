/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: DateDay.c
 *
 * Description:
 *	  This is the Datebook application's main module.  This module
 *   starts the application, dispatches events, and stops
 *   the application.
 *
 * History:
 *		June 12, 1995	Created by Art Lamb
 *			Name		Date		Description
 *			----		----		-----------
 *			???		????		Initial Revision
 *			frigino	970909	Added alarmSoundUniqueRecID to DatebookPreferenceType
 *									to remember the alarm sound to play. Moved
 *									DatebookPreferenceType out of this file.
 *			grant		3/5/99	Removed dependece on MemDeref and MemoryPrv.h.
 *									DetailsH was a handle that was always left locked;
 *									replaced by a pointer DetailsP.
 *			rbb		4/9/99	Removed time bar and end-time for zero-duration appts
 *			rbb		4/22/99	Added snooze
 *			rbb		6/10/99	Removed obsoleted code that worked around
 *									single-segment linker limitation
 *			grant		6/28/99	New global - RepeatDetailsP.  When editing an event's details,
 *									there is one details info block that is pointed to by either
 *									DetailsP or RepeatDetailsP but not both.  When the "Details"
 *									form is active, then DetailsP is valid and RepeatDetailsP
 *									should be NULL.  And vice versa for the "Repeat" form.
 *			gap		8/27/99	Replaced call of ExceptionAlert with call to RangeDialog
 *									in DetailsApply().
 *			gap		9/8/99	Added current, current & future, or all occurrences support
 *									for addition/removal of notes.
 *
 *****************************************************************************/

#include <PalmOS.h>
#include <Graffiti.h>
#include <SelTime.h>
#include <SysEvtMgr.h>
#include "../Address.h"
#include "../AddressRsc.h"
#include "../AddrTools.h"


#include "Datebook.h"

extern ECApptDBValidate (DmOpenRef dbP);
//#define min(a,b) (a<b? a:b)
//#define max(a,b) (a>b? a:b)


/***********************************************************************
 *
 *	Global variables, declarded in DateGlobals.c.  Because of a bug in
 *  the Metrowerks compiler, we must compile the globals separately with
 *	 PC-relative strings turned off.
 *
 ***********************************************************************/
/*extern	MemHandle				ApptsH;
extern	UInt16					NumAppts;
extern	MemHandle				ApptsOnlyH;
extern	UInt16					NumApptsOnly;

extern	UInt16					TopVisibleAppt;
extern privateRecordViewEnum	DatePrivateRecordVisualStatus;
extern	UInt16					PendingUpdate;						// code of pending day view update


// The following global variables are used to keep track of the edit
// state of the application.
extern 	UInt16					DateCurrentRecord;						// record being edited
extern 	Boolean					ItemSelected;						// true if a day view item is selected
extern 	UInt16					DayEditPosition;					// position of the insertion point in the desc field
extern	UInt16					DayEditSelectionLength;			// length of the current selection.
extern	Boolean					RecordDirty;						// true if a record has been modified


// The following global variables are only valid while editng the detail
// of an appointment.
extern	void*						DetailsP;
extern	DateType					RepeatEndDate;
extern	RepeatType				RepeatingEventType;
extern	UInt16					RepeatStartOfWeek;				// status of Repeat Dialog.

// This global variable is only valid while editing the repeat info of an appointment.
extern	void *				RepeatDetailsP;

extern	UInt16				TimeBarColumns;					// Number of columns of time bars.
*/
// The following structure is used by the details dialog to hold
// changes made to an appointment record.
typedef struct {
	Boolean					secret;
	UInt8						reserved;
	ApptDateTimeType		when;
	AlarmInfoType 			alarm;
	RepeatInfoType 		repeat;
} DetailsType;

typedef DetailsType * DetailsPtr;


typedef enum {
	dateRangeNone,
	dateRangeCurrent,
	dateRangeCurrentAndFuture,
	dateRangeAll
} DateRangeType;


/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/

static void DayViewLoadTable (void);

static void DayViewLayoutDay (Boolean retieve);

static void DayViewDrawTimeBars (void);

static void DayViewInitRow (TablePtr table, UInt16 row, UInt16 apptIndex, 
	Int16 rowHeight, UInt32 uniqueID, UInt16 iconsWidth, FontID fontID);

static void DayViewUpdateDisplay (UInt16 updateCode);


static Boolean RepeatDescRectHandler(FormGadgetType *gadgetP, UInt16 cmd, void *paramP);

static void* GetObjectPtr (UInt16 objectID)
{
	FormPtr frm;
	
	frm = FrmGetActiveForm ();
	return (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, objectID)));

}

/***********************************************************************
 *
 * FUNCTION:    SetDateToNextOccurrence
 *
 * DESCRIPTION: This routine set the "current date" global variable to
 *              the date that the specified record occurs on.  If the
 *              record is a repeating appointmnet, we set the date to 
 *              the next occurrence of the appointment.  If we are beyond
 *              the end date of the repeating appointment, we set the 
 *              date to the last occurrence of the event.
 *
 * PARAMETERS:  recordNum - index of appointment record 
 *
 * RETURNED:    true if successful, false if not.  It's posible that 
 *              a repeating event may have no displayable occurrences.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/24/95	Initial Revision
 *
 ***********************************************************************/
static Boolean SetDateToNextOccurrence (UInt16 recordNum)
{
	globalVars* globals = getGlobalsPtr();
	Boolean dateSet = true;
	MemHandle recordH;
	DateType today;
	DateTimeType dateTime;
	ApptDBRecordType apptRec;

	ApptGetRecord (globals->ApptDB, recordNum, &apptRec, &recordH);

	if (! apptRec.repeat)
		globals->Date = apptRec.when->date;

	// If the appointment is a repeating event,  go to the date of the 
	// next occurrence of the appointment.
	else
		{
		// Get today's date.
		TimSecondsToDateTime (TimGetSeconds (), &dateTime);
		today.year = dateTime.year - firstYear;
		today.month = dateTime.month;
		today.day = dateTime.day;

		globals->Date = today;
		if ( ! ApptNextRepeat (&apptRec, &globals->Date))
			{
			// If we are beyond the end date of the repeating event, display
			// the last occurrence of the event.
			globals->Date = apptRec.repeat->repeatEndDate;
			while ( ! ApptNextRepeat (&apptRec, &globals->Date))
				{
				// It posible that there are no occurences that are displayable
				// (ex: an expections is created for each occurrences),  if so
				// just go to today.
				if (DateToInt (globals->Date) == DateToInt (apptRec.when->date))
					{
					ErrDisplay ("No displayable occurrences of event");
					globals->Date = today;
					dateSet = false;
					break;
					}
				DateAdjust (&globals->Date, -1);
				}
			}
		}
	MemHandleUnlock (recordH);
	
	return (dateSet);
}


/***********************************************************************
 *
 * FUNCTION:    GetTime
 *
 * DESCRIPTION: This routine selects the start and end time of an event.
 *
 * PARAMETERS:  startP  - passed:   current start time
 *                        returned: selected start time
 *              endtP   - passed:   current end time
 *                        returned: selected end time
 *              titleID - resource id of the title to display in the 
 *                        time picker dialog.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	10/24/95	Initial Revision
 *
 ***********************************************************************/
static Boolean GetTime (TimePtr startP, TimePtr endP, UInt16 titleStrID)
{
	globalVars* globals = getGlobalsPtr();
	Char* title;
	TimeType start, end;
	Boolean selected;
	Boolean untimed;
	DateTimeType dateTime;
	Int16	firstDisplayHour;
	
	
	// Get today's date, are we displaying today?
	TimSecondsToDateTime (TimGetSeconds (), &dateTime);
	if (globals->Date.year == dateTime.year - firstYear &&
				globals->Date.month == dateTime.month &&
				globals->Date.day == dateTime.day)
		{
		firstDisplayHour = dateTime.hour;
		}
	else //not today
		{
		firstDisplayHour = globals->DayStartHour;
		}

	// If the event is untimed, pass the default start time
	// and duration.
	if (TimeToInt (*startP) == apptNoTime)
		{
		untimed = true;
		start.hours = min (firstDisplayHour + 1, hoursPerDay-1);
		start.minutes = 0;
		end.hours = min (firstDisplayHour + 2, hoursPerDay);
		end.minutes = 0;
		}
	else
		{
		untimed = false;
		start = *startP;
		end = *endP;
		}
		
	
	title = MemHandleLock(DmGetResource (strRsc, titleStrID));

	selected = SelectTime (&start, &end, untimed, title, globals->DayStartHour, globals->DayEndHour, firstDisplayHour);

	MemPtrUnlock (title);
	
	if (selected)
		{
		*startP = start;
		*endP = end;
		}
	
	return (selected);
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
 *			art	2/21/95	Initial Revision
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
 * FUNCTION:    ShowObject
 *
 * DESCRIPTION: This routine set an object usable and draws the object if
 *              the form it is in is visible.
 *
 * PARAMETERS:  frm      - pointer to a form
 *              objectID - id of the object to set usable
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *
 ***********************************************************************/
static void ShowObject (FormPtr frm, UInt16 objectID)
{
	FrmShowObject (frm, FrmGetObjectIndex (frm, objectID));
}


/***********************************************************************
 *
 * FUNCTION:    HideObject
 *
 * DESCRIPTION: This routine set an object not-usable and erases it
 *              if the form it is in is visible.
 *
 * PARAMETERS:  frm      - pointer to a form
 *              objectID - id of the object to set not usable
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *
 ***********************************************************************/
static void HideObject (FormPtr frm, UInt16 objectID)
{
	FrmHideObject (frm, FrmGetObjectIndex (frm, objectID));
}



/***********************************************************************
 *
 * FUNCTION:    DatebookDayOfWeek
 *
 * DESCRIPTION: This routine returns the day-of-the-week, adjusted by the 
 *              preference setting that specifies the first day-of-
 *              the-week.  If the date passed is a Tuesday and the 
 *              start day of week is Monday, this routine will return
 *              a value of one.
 *
 * PARAMETERS:	 month - month (1-12)
 *              day   - day (1-31)
 *              year  - year (1904-2031)
 *
 * RETURNED:	 day of the week (0-first, 1-second)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/27/95	Initial Revision
 *
 ***********************************************************************/
static UInt16 DatebookDayOfWeek (UInt16 month, UInt16 day, UInt16 year)
{
	globalVars* globals = getGlobalsPtr();
	return ((DayOfWeek (month, day, year) - globals->StartDayOfWeek + daysInWeek)
	 % daysInWeek);
}


/***********************************************************************
 *
 * FUNCTION:    SubstituteStr
 *
 * DESCRIPTION: This routine substitutes the occurrence a token, within
 *              a string, with another string.
 *
 * PARAMETERS:  str    - string containing token string
 *              token  - the string to be replaced
 *              sub    - the string to substitute for the token
 *              subLen - length of the substitute string.
 *
 * RETURNED:    pointer to the string
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/6/95	Initial Revision
 *
 ***********************************************************************/
static Char* SubstituteStr (Char* str, Char* token, Char* sub, UInt16 subLen)
{
	int charsToMove;
	UInt16 tokenLen;
	UInt16 strLen;
	UInt16 blockSize;
	Char* ptr;
	MemHandle strH;

	// Find the start of the token string, if it doesn't exist, exit.
	ptr = StrStr(str, token);
	if (ptr == NULL) return (str);
	
	tokenLen = StrLen (token);
	charsToMove = subLen - tokenLen;
	
	
	// Resize the string if necessary.
	strH = MemPtrRecoverHandle (str);
	strLen = StrLen (str);
	blockSize = MemHandleSize (strH);
	if (strLen + charsToMove + 1 >= blockSize)
		{
		MemHandleUnlock (strH);
		MemHandleResize (strH, strLen + charsToMove + 1);
		str = MemHandleLock (strH);
		ptr = StrStr (str, token);
		ErrNonFatalDisplayIf(ptr == NULL, "Msg missing token");
		}
	
	// Make room for the substitute string.
	if (charsToMove)
		MemMove (ptr + subLen, ptr + tokenLen, StrLen (ptr + tokenLen)+1);
		
	// Replace the token with the substitute string.
	MemMove (ptr, sub, subLen);
	
	return (str);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewFindAppointment
 *
 * DESCRIPTION: Given the database index of a record, this routine 
 *              finds the record in the appointment list and returns
 *              the index of the appointment
 *
 * PARAMETERS:  recordNum - db index of record to fine 
 *              recordNum - 
 *
 * RETURNED:   appointment list index
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/27/95	Initial Revision
 *
 ***********************************************************************/
static UInt16 DayViewFindAppointment (UInt16 recordNum)
{
	UInt16 i;
	ApptInfoPtr appts;
	globalVars* globals = getGlobalsPtr();
	
	appts = MemHandleLock (globals->ApptsH);

	for (i = 0; i < globals->NumAppts; i++)
		{
		if (appts[i].recordNum == recordNum)
			{
			MemPtrUnlock (appts);
			return (i);
			}
		}

	MemPtrUnlock (appts);

	// If we're beyond the maximun number of appointment that can be 
	// shown on a day, then the record passed may not be in the list.
	if (globals->NumApptsOnly >= apptMaxPerDay)
		{
		return (0);
		}

	
	ErrDisplay ("Record not on day");

	return (0);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewSetTopAppointment
 *
 * DESCRIPTION: This routine determines the first appointment that should
 *              be visible on the current day.  For all dates other than
 *              today the fisrt time slot of the appointment list
 *              is the first visible appointment.  For today the time
 *              slot that stats before to the current time should be the top 
 *              visible time slot.
 *
 * PARAMETERS:  nothing.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/29/95	Initial Revision
 *
 ***********************************************************************/
static void DayViewSetTopAppointment (void)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 i;
	TimeType time;
	DateTimeType dateTime;
	ApptInfoPtr appts;
	
	globals->TopVisibleAppt = 0;

	TimSecondsToDateTime (TimGetSeconds (), &dateTime);

	// If the current date is not today, then the first appointment 
	// is the first one visible.
	if ( (dateTime.year - firstYear != globals->Date.year) ||
		  (dateTime.month != globals->Date.month) ||
		  (dateTime.day != globals->Date.day))
		{
		return;
		}

	// If the current date is today, then the top visible appointment is
	// the appointment with the greatest end time that is before the 
	// current time.
	time.hours = dateTime.hour;
	time.minutes = dateTime.minute;
	
	appts = MemHandleLock (globals->ApptsH);
	for (i = 0; i < globals->NumAppts; i++)
		{
		if (TimeToInt (appts[i].endTime) < TimeToInt (time))
			globals->TopVisibleAppt = i;
		}

	MemPtrUnlock (appts);
}

void DrawTime (
	TimeType							inTime,
	TimeFormatType					inFormat,
	FontID							inFont,
	JustificationType				inJustification,
	RectangleType*					inBoundsP )
{
	FontID							curFont;
	char								timeStr [timeStringLength];
	TimeFormatType					format;
	UInt16							len;
	UInt16							width;
	Int16								x;

	// No-time appointment?
	if (TimeToInt(inTime) == apptNoTime)
		{
		// Show a centered diamond symbol, overriding the font params
		inFont = symbolFont;
		inJustification  = centerAlign;

		timeStr[0] = symbolNoTime;
		timeStr[1] = '\0';
		}
	else
		{
		if (inFormat == tfColonAMPM)
			format = tfColon;
		 else if (inFormat == tfDotAMPM)
		 	format = tfDot;
		 else
			format = inFormat;

		TimeToAscii (inTime.hours, inTime.minutes, format, timeStr);

		}	
	
	// Use the string width and alignment to compute its starting point
	len = StrLen (timeStr);
	width = FntCharsWidth (timeStr, len);
	x = inBoundsP->topLeft.x;
	switch (inJustification)
		{
		case rightAlign:
			x += inBoundsP->extent.x - width;
			break;
		
		case centerAlign:
			x += (inBoundsP->extent.x - width) / 2;
			break;
		}
	
	x = max (inBoundsP->topLeft.x, x);
	
	// Draw the time
	curFont = FntSetFont (inFont);
	WinDrawChars (timeStr, len, x, inBoundsP->topLeft.y);	
	FntSetFont (curFont);
}



/***********************************************************************
 *
 * FUNCTION:    DayViewDrawTime
 *
 * DESCRIPTION: This routine draws the start time of an appointment.  This
 *              routine is called by the table object as a callback 
 *              routine.
 *
 * PARAMETERS:  table  - pointer to the memo Day table (TablePtr)
 *              row    - row of the table to draw
 *              column - column of the table to draw
 *              bounds - region to draw in 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/29/95	Initial Revision
 *			kcr	11/14/95	Display 'no time' char for untimed events
 *			rbb	6/4/99	Moved bulk of code to DrawTime, for sharing w/Agenda
 *
 ***********************************************************************/
static void DayViewDrawTime (void * table, Int16 row, Int16 column, 
	RectanglePtr bounds)
{
#pragma unused(column)
	globalVars* globals = getGlobalsPtr();
	UInt16 apptIndex;
	TimeType startTime;
	ApptInfoPtr appts;
	

	// Get the appointment index that corresponds to the table item.
	// The index of the appointment in the appointment list, is stored
	// as the row id.
	apptIndex =TblGetRowID (table, row);

	// Get the start time of the appointment.
	appts = MemHandleLock (globals->ApptsH);
	startTime = appts[apptIndex].startTime;
	MemHandleUnlock (globals->ApptsH);
	
	DrawTime (startTime, globals->TimeFormat, apptTimeFont, rightAlign, bounds);
}

static void SetDBBackupBit(DmOpenRef dbP)
{
	DmOpenRef localDBP;
	LocalID dbID;
	UInt16 cardNo;
	UInt16 attributes;

	// Open database if necessary. If it doesn't exist, simply exit (don't create it).
	if (dbP == NULL)
		{
		localDBP = DmOpenDatabaseByTypeCreator (datebookDBType, sysFileCDatebook, dmModeReadWrite);
		if (localDBP == NULL)  return;
		}
	else
		{
		localDBP = dbP;
		}
	
	// now set the backup bit on localDBP
	DmOpenDatabaseInfo(localDBP, &dbID, NULL, NULL, &cardNo, NULL);
	DmDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	attributes |= dmHdrAttrBackup;
	DmSetDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	
	// close database if necessary
   if (dbP == NULL) 
   	{
   	DmCloseDatabase(localDBP);
      }
}

Err DateGetDatabase (DmOpenRef *dbPP, UInt16 mode)
{
	Err error = 0;
	DmOpenRef dbP;
	UInt16 cardNo;
	LocalID dbID;
	
	
	*dbPP = 0;
	dbP = DmOpenDatabaseByTypeCreator(datebookDBType, sysFileCDatebook, mode);
	if(dbP == 0)
		dbP = DmOpenDatabaseByTypeCreator('DATA', 'PDat', mode);
	
	if (! dbP)
		{
		error = DmCreateDatabase (0, datebookDBName, sysFileCDatebook,
								datebookDBType, false);
		if (error) return error;
		
		dbP = DmOpenDatabaseByTypeCreator(datebookDBType, sysFileCDatebook, mode);
		if (! dbP) return ~0;

		// Set the backup bit.  This is to aid syncs with non Palm software.
		SetDBBackupBit(dbP);
		
		error = ApptAppInfoInit (dbP);
      if (error) 
      	{
			DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL);
      	DmCloseDatabase(dbP);
      	DmDeleteDatabase(cardNo, dbID);
         return error;
         }
		}
	
	*dbPP = dbP;
	return 0;
}

/***********************************************************************
 *
 * FUNCTION:		DatebookLoadPrefs
 *
 * DESCRIPTION:	Loads app's preferences and fixes them up if they didn't exist
 *						or were of the wrong version.
 *
 * PARAMETERS:		prefsP		-- ptr to preferences structure to fill in
 *
 * RETURNED:		the version of preferences from which values were read
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			vmk		12/9/97	Initial version
 *			vmk		12/11/97	Fix up note font
 *			rbb		4/23/99	Added alarmSnooze
 *
 ***********************************************************************/
Int16 DatebookLoadPrefs (DatebookPreferenceType* prefsP)
{
	UInt16	prefsSize;
	Int16	prefsVersion = noPreferenceFound;
	Boolean haveDefaultFont = false;
	UInt32 defaultFont;
	
	ErrNonFatalDisplayIf(!prefsP, "null prefP arg");
		
	
	// Read the preferences / saved-state information.  Fix-up if no prefs or older/newer version
	prefsSize = sizeof (DatebookPreferenceType);
	prefsVersion = PrefGetAppPreferences (sysFileCDatebook, datebookPrefID, prefsP, &prefsSize, true);
	
	// If the preferences version is from a future release (as can happen when going back
	// and syncing to an older version of the device), treat it the same as "not found" because
	// it could be significantly different
	if ( prefsVersion > datebookPrefsVersionNum )
		prefsVersion = noPreferenceFound;
		
	if ( prefsVersion == noPreferenceFound )
		{
		// Version 1 and 2 preferences
		prefsP->dayStartHour = defaultDayStartHour;
		prefsP->dayEndHour = defaultDayEndHour;
		prefsP->alarmPreset.advance = defaultAlarmPresetAdvance;
		prefsP->alarmPreset.advanceUnit = defaultAlarmPresetUnit;
		prefsP->saveBackup = defaultSaveBackup;
		prefsP->showTimeBars = defaultShowTimeBars;
		prefsP->compressDayView = defaultCompressDayView;
		prefsP->showTimedAppts = defaultShowTimedAppts;
		prefsP->showUntimedAppts = defaultShowUntimedAppts;
		prefsP->showDailyRepeatingAppts = defaultShowDailyRepeatingAppts;
		
		// We need to set up the note font with a default value for the system.
		FtrGet(sysFtrCreator, sysFtrDefaultFont, &defaultFont);
		haveDefaultFont = true;
		
		prefsP->v20NoteFont = (FontID)defaultFont;
		}
		
	if ((prefsVersion == noPreferenceFound) || (prefsVersion < datebookPrefsVersionNum))
		{
		// Version 3 preferences
		prefsP->alarmSoundRepeatCount = defaultAlarmSoundRepeatCount;
		prefsP->alarmSoundRepeatInterval = defaultAlarmSoundRepeatInterval;
		prefsP->alarmSoundUniqueRecID = defaultAlarmSoundUniqueRecID;
		prefsP->noteFont = prefsP->v20NoteFont;	// 2.0 compatibility (BGT)
		
		// Fix up the note font if we copied from older preferences.
		if ((prefsVersion != noPreferenceFound) && (prefsP->noteFont == largeFont))
			prefsP->noteFont = largeBoldFont;

		if (!haveDefaultFont)
			FtrGet(sysFtrCreator, sysFtrDefaultFont, &defaultFont);
		
		prefsP->apptDescFont = (FontID)defaultFont;
		}
	
	if ((prefsVersion == noPreferenceFound) || (prefsVersion < datebookPrefsVersionNum))
		{
		// Version 4 preferences
		prefsP->alarmSnooze = defaultAlarmSnooze;
		}

	return prefsVersion;
}



static UInt16 StartApplication (void)
{
	globalVars* globals = getGlobalsPtr();
	Err err = 0;
	UInt16 mode;
	DateTimeType dateTime;
	DatebookPreferenceType prefs;
	Int16 prefsVersion;
	
	
	// Determime if secret record should be shown.
	globals->DatePrivateRecordVisualStatus = (privateRecordViewEnum)PrefGetPreference(prefShowPrivateRecords);
	mode = (globals->DatePrivateRecordVisualStatus == hidePrivateRecords) ?
					dmModeReadOnly : (dmModeReadOnly | dmModeShowSecret);
	

	// Get the time formats from the system preferences.
	globals->TimeFormat = (TimeFormatType)PrefGetPreference(prefTimeFormat);

	// Get the date formats from the system preferences.
	globals->LongDateFormat = (DateFormatType)PrefGetPreference(prefLongDateFormat);
	globals->ShortDateFormat = (DateFormatType)PrefGetPreference(prefDateFormat);

	// Get the starting day of the week from the system preferences.
	globals->StartDayOfWeek = PrefGetPreference(prefWeekStartDay);
	

	// Get today's date.
	TimSecondsToDateTime (TimGetSeconds (), &dateTime);
	globals->Date.year = dateTime.year - firstYear;
	globals->Date.month = dateTime.month;
	globals->Date.day = dateTime.day;


	// Find the application's data file.  If it don't exist create it.
	err = DateGetDatabase (&globals->ApptDB, mode);
	if (err)	return err;
	
	
	// Read the preferences / saved-state information (will fix up incompatible versions).
	prefsVersion = DatebookLoadPrefs (&prefs);
	globals->DayStartHour = prefs.dayStartHour;
	globals->DayEndHour = prefs.dayEndHour;
	globals->AlarmPreset = prefs.alarmPreset;
	globals->NoteFont = prefs.noteFont;
	globals->SaveBackup = prefs.saveBackup;
	globals->ShowTimeBars = prefs.showTimeBars;
	globals->CompressDayView = prefs.compressDayView;
	globals->ShowTimedAppts = prefs.showTimedAppts;
	globals->ShowUntimedAppts = prefs.showUntimedAppts;
	globals->ShowDailyRepeatingAppts = prefs.showDailyRepeatingAppts;

	// The first time this app starts register to handle vCard data.
	if (prefsVersion != datebookPrefsVersionNum)
		ExgRegisterData(sysFileCDatebook, exgRegExtensionID, "vcs");
   

	globals->TopVisibleAppt = 0;
	globals->DateCurrentRecord = noRecordSelected;	
	
	return 0;
}

static void StopApplication (void)
{
	globalVars* globals = getGlobalsPtr();
	if(globals->ApptsH)
		MemHandleFree(globals->ApptsH);
	if(globals->ApptsOnlyH)
		MemHandleFree(globals->ApptsOnlyH);
	globals->ApptsH = NULL;
	globals->ApptsOnlyH = NULL;
	DmCloseDatabase (globals->ApptDB);
}



/***********************************************************************
 *
 * FUNCTION:    DayViewDrawIcons
 *
 * DESCRIPTION: This routine draws the note, alarm and repeat icons.
 *              It is called by the table object as a callback 
 *              routine. 
 *
 * PARAMETERS:  table  - pointer to the memo Day table (TablePtr)
 *              row    - row of the table to draw
 *              column - column of the table to draw
 *              bounds - region to draw in 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/16/96	Initial Revision
 *
 ***********************************************************************/
static void DayViewDrawIcons (void * table, Int16 row, Int16 column, 
	RectanglePtr bounds)
{
	globalVars* globals = getGlobalsPtr();
	Char					chr;
	UInt16					apptIndex;
	UInt16 					recordNum;
	Int16					x, y;
	FontID				curFont;
	MemHandle 			recordH;
	ApptInfoPtr 		appts;
	ApptDBRecordType	apptRec;

	// Get the appointment index that corresponds to the table item.
	// The index of the appointment in the appointment list, is stored
	// as the row id.
	apptIndex =TblGetRowID (table, row);

	// Get the start time of the appointment.
	appts = MemHandleLock (globals->ApptsH);
	recordNum = appts[apptIndex].recordNum;
	MemHandleUnlock (globals->ApptsH);

	ApptGetRecord (globals->ApptDB, recordNum, &apptRec, &recordH);

	x = bounds->topLeft.x + bounds->extent.x - TblGetItemInt (table, row, column);
	y = bounds->topLeft.y;
	curFont = FntSetFont (symbolFont);

	// Draw note icon
	if (apptRec.note)
		{
		chr = symbolNote;
		WinDrawChars (&chr, 1, x, y);
		x += FntCharWidth (chr) + 1;
		}

	// Draw alarm icon
	if (apptRec.alarm)
		{
		chr = symbolAlarm;
		WinDrawChars (&chr, 1, x, y);
		x += FntCharWidth (chr) + 1;
		}

	// Draw repeat icon
	if (apptRec.repeat)
		{
		chr = symbolRepeat;
		WinDrawChars (&chr, 1, x, y);
		x += FntCharWidth (chr) + 1;
		}

	FntSetFont (curFont);

	MemHandleUnlock (recordH);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewCheckForConflicts
 *
 * DESCRIPTION: This routine check the apointment list for conflicts
 *              (overlapping appointmens).
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    the number of columns of time bars necessary to display
 *              the conflicts
 *              
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/3/96	Initial Revision
 *
 ***********************************************************************/
static UInt16 DayViewCheckForConflicts (void)
{
	globalVars* globals = getGlobalsPtr();
	UInt16				i;
	UInt16				numColumns;
	UInt16				apptIndex;
	UInt16				width;
	TablePtr			table;
	TimeType			endTime [maxTimeBarColumns];
	ApptInfoPtr		appts;
	RectangleType	tableR;
	
	MemSet (endTime, sizeof (endTime), 0);

	numColumns = 1;

	appts = MemHandleLock (globals->ApptsH);
	for (apptIndex = 0; apptIndex < globals->NumAppts; apptIndex++)
		{
		if (appts[apptIndex].recordNum == emptySlot)
			continue;

		else if (TimeToInt (appts[apptIndex].startTime) == apptNoTime)
			continue;
		
		for (i = 0; i < maxTimeBarColumns; i++)
			{
			if (TimeToInt (appts[apptIndex].startTime) >= TimeToInt (endTime[i]))
				{
				endTime[i] = appts[apptIndex].endTime;
				if (i+1 > numColumns)
					numColumns = i+1;
				break;
				}
			}
		}
	MemPtrUnlock (appts);
	

	// Reserve spase for the time bars.  We will show time bars if the user
	// has requested them or if there are overlapping appointments.
	if (numColumns == 1 && (! globals->ShowTimeBars))
		numColumns = 0;

	if (globals->TimeBarColumns != numColumns)
		{
		globals->TimeBarColumns = numColumns;
		table = GetObjectPtr (DayTable);
		
		// Set the width of the time bar table column.
		TblSetColumnWidth (table, timeBarColumn, numColumns * timeBarWidth);
		
		// Adjust the width of the description column.
		TblGetBounds (table, &tableR);
		width = tableR.extent.x - 
				  (numColumns * timeBarWidth) -
 				  TblGetColumnWidth (table, timeColumn) -
 				  TblGetColumnSpacing (table, timeColumn);
 		TblSetColumnWidth (table, descColumn, width);
 		
 		// Invalid the whole table since the positions of the time and 
 		// description columns has changed.
 		TblMarkTableInvalid (table);
		}

	return (numColumns);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewDrawTimeBars
 *
 * DESCRIPTION: This routine draw the time bars the indicate the durations
 *              of apointments.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *              
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/3/96	Initial Revision
 *			rbb	4/9/99	Removed time bar for zero-duration appts
 *			jmp	9/29/99	Use FrmGetFormPtr() & FrmGetObjectIndex() instead of
 *								GetObjectPtr() because GetObjectPtr() calls FrmGetActiveForm(),
 *								and FrmGetActiveForm() may return a form that isn't the one we
 *								want when other forms are up when we are called.
 *								Fixes bug #22481.
 ***********************************************************************/
static void DayViewDrawTimeBars (void)
{
	globalVars* globals = getGlobalsPtr();
	Int16				i, j;
	Int16				apptIndex;
	Int16				numColumns;
	Int16				row;
	Int16				lineHeight;
	Int16				x, y1, y2;
	FontID			curFont;
	TablePtr			table;
	Boolean			drawTop;
	Boolean			drawBottom;
	Int16				endPoint [maxTimeBarColumns];
	TimeType			endTime [maxTimeBarColumns];
	ApptInfoPtr		appts;
	RectangleType	r;
	RectangleType	tableR;
	RectangleType	eraseR;
	FormPtr			frm;

	if (! globals->TimeBarColumns) return;

	MemSet (endTime, sizeof (endTime), 0);
	numColumns = 1;

	frm = FrmGetFormPtr (DayView);
	if (! FrmVisible (frm)) return;
	
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, DayTable));
	TblGetBounds (table, &tableR);

	for (i = 0; i < maxTimeBarColumns; i++)
		endPoint[i] = tableR.topLeft.y;

 	WinPushDrawState();
	curFont = FntSetFont (apptTimeFont);
	lineHeight = FntLineHeight ();
	FntSetFont (curFont);

	appts = MemHandleLock (globals->ApptsH);
	for (apptIndex = 0; apptIndex < globals->NumAppts; apptIndex++)
		{
		if (appts[apptIndex].recordNum == emptySlot)
			continue;
		else if (TimeToInt (appts[apptIndex].startTime) == apptNoTime)
			continue;
		
		for (i = 0; i < maxTimeBarColumns; i++)
			{
			if (i == 0)
				WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
			else
				WinSetForeColor(UIColorGetTableEntryIndex(UIWarning));
			
			if (TimeToInt (appts[apptIndex].startTime) >= TimeToInt (endTime[i]))
				{
				endTime[i] = appts[apptIndex].endTime;
				
				// Find the row that hold the appointment, it may not be
				// visible.
				if (TblFindRowID (table, apptIndex, &row))
					{
					TblGetItemBounds (table, row, descColumn, &r);
					y1 = r.topLeft.y + (lineHeight >> 1);
					drawTop = true;
					}

				// Is the appointment off the top of the display.
				else if (apptIndex < globals->TopVisibleAppt)
					{
					y1 = tableR.topLeft.y;
					drawTop = false;
					}

				// If the appointment is below the bottom of the display we
				// don't draw anything.	
				else 
					break;

				// If the start time matches the end time we don't draw anything
				if ( TimeToInt (appts[apptIndex].startTime) ==
						TimeToInt (appts[apptIndex].endTime) )
					break;				

				// Find the row that contains the end time of the appointment.
				for (j = apptIndex + 1; j < globals->NumAppts; j++)
					{
					// There may be more the one time slot with the time
					// we're searching for, get the last one.
					if (TimeToInt (appts[apptIndex].endTime) <=
						 TimeToInt (appts[j].startTime))
						break;
					}

				// Is the end-time visible.
				if (TblFindRowID (table, j, &row))
					{
					TblGetItemBounds (table, row, descColumn, &r);
					y2 = r.topLeft.y + (lineHeight >> 1);
					drawBottom = true;				
					}

				// Is the end of the appointment off the top of the display, if so
				// don't draw anything.
				else if (j < globals->TopVisibleAppt)
					break;

				else
					{
					y2 = tableR.topLeft.y + tableR.extent.y - 1;
					drawBottom = false;				
					}

				x = tableR.topLeft.x + (i * timeBarWidth);


				// Erase the region between the top of the time bar we're
				// about to draw and the bottom of the previous time bar.
				if (y1 > endPoint[i])
					{
					eraseR.topLeft.x = x;
					eraseR.topLeft.y = endPoint[i];
					eraseR.extent.x = timeBarWidth;
					eraseR.extent.y = y1 - endPoint[i];
					WinEraseRectangle (&eraseR, 0);
					}
				endPoint[i] = y2 + 1;
				

				// Draw the time bar.
				WinEraseLine (x+1, y1+1, x+1, y2-2);

				if (drawTop) y1++;
				if (drawBottom) y2--;

				WinDrawLine (x, y1, x, y2);
				if (drawTop)
					WinDrawLine (x, y1, x + timeBarWidth - 1, y1);
				if (drawBottom)
					WinDrawLine (x, y2, x + timeBarWidth - 1, y2);
				
				if (i+1 > numColumns)
					numColumns = i+1;

				break;
				}
			}
		}
		
	// Erase the regions between the botton of the last time bar in 
	// each column and the bottom of the table.
	for (i = 0; i < numColumns; i++)
		{
		if (tableR.topLeft.y + tableR.extent.y > endPoint[i])
			{
			eraseR.topLeft.x = tableR.topLeft.x + (i * timeBarWidth);
			eraseR.topLeft.y = endPoint[i];
			eraseR.extent.x = timeBarWidth;
			eraseR.extent.y = tableR.topLeft.y + tableR.extent.y - endPoint[i];
			WinEraseRectangle (&eraseR, 0);
			}
		}
	
	MemPtrUnlock (appts);
 	WinPopDrawState();
	}


/***********************************************************************
 *
 * FUNCTION:    DayViewInsertAppointment
 *
 * DESCRIPTION: This routine inserts the record index of a new record 
 *              into the structure that keeps track of the appointment
 *              on the current day.
 *
 * PARAMETERS:  apptIndex - 
 *              recordNum - appointment record index
 *              row       - row in the table object of the new record
 *
 * RETURNED:   nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/28/95	Initial Revision
 *
 ***********************************************************************/
static void DayViewInsertAppointment (UInt16 apptIndex, UInt16 recordNum, UInt16 row)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 i;
	UInt32 uniqueID;
	TablePtr table;
	ApptInfoPtr appts;
	
	appts = MemHandleLock (globals->ApptsH);

	// Adjust all the record index that are greater than the new record.
	for (i = 0; i < globals->NumAppts; i++)
		{
		if ((appts[i].recordNum != emptySlot) && 
			 (appts[i].recordNum >= recordNum))
			appts[i].recordNum++;
		}

	appts[apptIndex].recordNum = recordNum;
//	appts[apptIndex].endTime.hours = appts[apptIndex].startTime.hours + 1;

	MemHandleUnlock (globals->ApptsH);

	// Store the unique id of the record in the row.
	table = GetObjectPtr (DayTable);
	DmRecordInfo (globals->ApptDB, recordNum, NULL, &uniqueID, NULL);
	TblSetRowData (table, row, uniqueID);
}


static Err DayViewGetDescription (void * table, Int16 row, Int16 column,
	Boolean editable, MemHandle * textH, Int16 * textOffset, 
	Int16 * textAllocSize, FieldPtr fld)
{
#pragma unused (column, editable)

	globalVars* globals = getGlobalsPtr();
	Err error = 0;
	UInt16 apptIndex;
	UInt16 recordNum;
	Char* recordP;
	MemHandle recordH;
	Boolean redraw = false;
	FieldAttrType attr;
	ApptInfoPtr appts;
	ApptDBRecordType apptRec;
	
//	UInt16 height;
//	UInt16 iconsWidth;
//	UInt16 tableHeight;
//	UInt16 columnWidth;
//	UInt32 uniqueID;
//	FontID fontID;
//	RectangleType r;
	
	*textH = 0;

	// Get the appoitment that corresponds to the table item.
	// The index of the appointment in the appointment list, is stored
	// as the row id.
	apptIndex = TblGetRowID (table, row);

	// Get the record index of the next appointment,  empty time slots 
	// have a minus one in the recordNum field.
	appts = MemHandleLock (globals->ApptsH);
	recordNum = appts[apptIndex].recordNum;
	MemHandleUnlock (globals->ApptsH);
	
	
	if(recordNum == emptySlot)
		return error;
	/*if (recordNum == emptySlot)
		{
		// If we're drawing the description, return a null MemHandle.
		if  (! editable) 
			return (0);

		// If we have reached the maximum number of displayable event, then
		// exit returning an error.
		if (NumApptsOnly >= apptMaxPerDay)
			return (-1);


		// If we're editing the description, create a new record.
		MemSet (&apptRec, sizeof (apptRec), 0);

		appts = MemHandleLock (ApptsH);
		when.startTime = appts[apptIndex].startTime;

		// If the start time is before 11:00 pm, the end time is one hour 
		// after the start time.
		if (when.startTime.hours < maxHours)
			{
			when.endTime.hours = when.startTime.hours + 1;
			when.endTime.minutes = when.startTime.minutes;
			}
			
		// If the start time is 11:00 pm or later, the end time is 11:55 pm. 
		else
			{
			when.endTime.hours = maxHours;
			when.endTime.minutes = maxMinutes;
			}

			// Don't let the new appointment overlap the next appointment.
		if (((apptIndex+1) < NumAppts) &&
			 (TimeToInt(when.endTime) > TimeToInt(appts[apptIndex+1].startTime)))
			{
			when.endTime = appts[apptIndex+1].startTime;
			appts[apptIndex].endTime = when.endTime;
			}

			// If the end time of the new event is not currently displayed, then
		// don't redraw the time bars.  We need to redraw the day, but we cannot
		// do it here.  We'll redraw the row on the tblSelect event.
		if (((apptIndex+1) == NumAppts) ||
			 (((apptIndex+1) < NumAppts) &&
				(TimeToInt(when.endTime) < TimeToInt(appts[apptIndex+1].startTime))))
			{
			redraw = true;
			TblMarkRowInvalid (table, row);
			}

		// If the description font is a different than the empty appointment font 
		// then we need to redraw the day, but we cannot
		// do it here.  We'll redraw the row on the tblSelect event.
		if (ApptDescFont != apptEmptyDescFont)
			{
			redraw = true;
			TblMarkRowInvalid (table, row);
			}

		MemHandleUnlock (ApptsH);

		when.date = Date;
		apptRec.when = &when;

		// Make sure the record has a description field so that we have
		// something to edit.
		apptRec.description = "";
		
		if (AlarmPreset.advance != apptNoAlarm)
			apptRec.alarm = &AlarmPreset;

		error = ApptNewRecord (ApptDB, &apptRec, &recordNum);

		if (error)
			{
			FrmAlert (DeviceFullAlert);
			return (error);
			}

		DayViewInsertAppointment (apptIndex, recordNum, row);

		// If the alarm preset preference is set we needed to reinitialize the
		// row so that the alarm icon will draw.  We don't redraw the row
		// here, we'll do that on the tblSelect event.
		if (AlarmPreset.advance != apptNoAlarm)
			{
			UInt32 ref;
			UInt32 trigger;
			UInt32 newAlarm;
			
			// If the new event's alarm will sound between now and the currently
			// registered alarm, the new one must be registered
			trigger = AlarmGetTrigger (&ref);
			newAlarm = ApptGetAlarmTime (&apptRec, TimGetSeconds ());

			if (newAlarm && ((newAlarm < trigger) || (trigger == 0)))
				{
				RescheduleAlarms (ApptDB);
				}

//			columnWidth = TblGetColumnWidth (table, descColumn);
//			TblGetBounds (table, &r);
//			tableHeight = r.extent.y;
//			height = DayViewGetDescriptionHeight (apptIndex, columnWidth, tableHeight, &iconsWidth, &fontID);
//			DmRecordInfo (ApptDB, recordNum, NULL, &uniqueID, NULL);
//			appts = MemHandleLock (ApptsH);
//			DayViewInitRow (table, row, apptIndex, height, uniqueID, iconsWidth, fontID);
//			MemPtrUnlock (appts);

			TblMarkRowInvalid (table, row);
			}

		if (! redraw)
			DayViewDrawTimeBars ();
		}

*/
	// Get the offset and length of the description field 
	ApptGetRecord (globals->ApptDB, recordNum, &apptRec, &recordH);
	
	
	/*if (apptRec.description == NULL)
		{
		ApptDBRecordFlags changedFields;
		
		
		// Add the note to the record.
		MemSet (&changedFields, sizeof (changedFields), 0);
		changedFields.description = true;
		apptRec.description = "";
		error = ApptChangeRecord (ApptDB, &recordNum, &apptRec, changedFields);
		if (error) return error;
		
		ApptGetRecord (ApptDB, recordNum, &apptRec, &recordH);
		}*/
	
	if(recordH)
	{
		recordP = MemHandleLock(recordH);
		*textOffset = apptRec.description - recordP;
		*textAllocSize = StrLen (apptRec.description) + 1;  // one for null terminator
		*textH = recordH;
		MemHandleUnlock (recordH);
		MemHandleUnlock (recordH);  // MemHandle was also locked in ApptGetRecord
	}
	// Set the field to support auto-shift.
	if (fld)
		{
		FldGetAttributes (fld, &attr);
		attr.autoShift = true;
		FldSetAttributes (fld, &attr);
		}

	return (0);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewGetDescriptionHeight
 *
 * DESCRIPTION: This routine returns the height, in pixels, of the 
 *              description field of an appointment record.
 *
 * PARAMETERS:  apptIndex   - index in appointment list
 *              width       - width of the description column
 * 				 maxHeight   - the maximum height of the field
 *              iconsWidthP - space ot reserve for note, alarm and 
 *                            repeat icons
 *              fontIdP     - font id to draw the text with,
 *
 * RETURNED:    height in pixels
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/29/95	Initial Revision
 *
 ***********************************************************************/
static Int16 DayViewGetDescriptionHeight (Int16 apptIndex, Int16 width, Int16 maxHeight,
	Int16 * iconsWidthP, FontID * fontIdP)
{
	globalVars* globals = getGlobalsPtr();
	Int16 height;
	Int16 iconsWidth;
	UInt16 recordNum;
	Int16 lineHeight;
	FontID curFont;
	MemHandle recordH;
	ApptInfoPtr	appts;
	ApptDBRecordType apptRec;
	UInt16 			attr;
	

	// Get the record index.
	appts = MemHandleLock (globals->ApptsH);
	recordNum = appts[apptIndex].recordNum;
	MemPtrUnlock (appts);

	iconsWidth = 0;

	// Empty time slot?
	if (recordNum == emptySlot)
		{
		curFont = FntSetFont (apptTimeFont);
		height = FntLineHeight ();
		FntSetFont (curFont);
		*fontIdP = apptTimeFont;
		}
	else
		{	  
		// Get the appointment record.
		ApptGetRecord (globals->ApptDB, recordNum, &apptRec, &recordH);


		DmRecordInfo (globals->ApptDB, recordNum, &attr, NULL, NULL);
	   if (((attr & dmRecAttrSecret) && globals->DatePrivateRecordVisualStatus == maskPrivateRecords))
	   	{
	   	//masked
			curFont = FntSetFont (apptTimeFont);
			height = FntLineHeight ();
			*fontIdP = apptTimeFont;
			}
		else
			{
			//unmasked
			// Compute the width needed to draw the note, alarm and repeat icons.
			curFont = FntSetFont (symbolFont);

			if (apptRec.note)
				iconsWidth += FntCharWidth (symbolNote);

			if (apptRec.alarm)
				{
				if (iconsWidth) iconsWidth++;
				iconsWidth += FntCharWidth (symbolAlarm);
				}

			if (apptRec.repeat)
				{
				if (iconsWidth) iconsWidth++;
				iconsWidth += FntCharWidth (symbolRepeat);
				}


			// Compute the height of the appointment description.
			FntSetFont (globals->ApptDescFont);
			
			height = FldCalcFieldHeight (apptRec.description, width - iconsWidth);
			lineHeight = FntLineHeight ();
			height = min (height, (maxHeight / lineHeight));
			height *= lineHeight;
			
			*fontIdP = globals->ApptDescFont;
			}

		FntSetFont (curFont);

		MemHandleUnlock (recordH);
		}


	*iconsWidthP = iconsWidth;
	return (height);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewUpdateScrollers
 *
 * DESCRIPTION: This routine draws or erases the Day View scroll arrow
 *              buttons.
 *
 * PARAMETERS:  frm             -  pointer to the to do Day form
 *              bottomAppt      -  record index of the last visible record
 *              lastItemClipped - true if the list is partially off the 
 *                                display
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/28/95	Initial Revision
 *
 ***********************************************************************/
static void DayViewUpdateScrollers (FormPtr frm, UInt16 bottomAppt,
	Boolean lastItemClipped)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 upIndex;
	UInt16 downIndex;
	Boolean scrollableUp;
	Boolean scrollableDown;
	
	// If the first appointment displayed is not the fist appointment
	// of the day, enable the up scroller.
	scrollableUp = (globals->TopVisibleAppt > 0);

	// If the last appointment displayed is not the last appointment
	// of the day or if it partially clipped, enable the down scroller.
	scrollableDown = ( lastItemClipped || (bottomAppt+1 < globals->NumAppts) );

	// Update the scroll button.
	upIndex = FrmGetObjectIndex (frm, DayUpButton);
	downIndex = FrmGetObjectIndex (frm, DayDownButton);
	FrmUpdateScrollers (frm, upIndex, downIndex, scrollableUp, scrollableDown);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewInitRow
 *
 * DESCRIPTION: This routine initializes a row in the Day View table.
 *
 * PARAMETERS:  table      - pointer to the table of appointments
 *              row        - row number (first row is zero)
 *              apptIndex  - index in appointment list
 *              rowHeight  - height of the row in pixels
 *              uniqueID   - unique ID of appointment record
 *              iconsWidth - spase to reserve for note, alarm and repeat
 *                           icons
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/29/95	Initial Revision
 *
 ***********************************************************************/
static void DayViewInitRow (TablePtr table, UInt16 row, UInt16 apptIndex, 
	Int16 rowHeight, UInt32 uniqueID, UInt16 iconsWidth, FontID fontID)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 time;
	ApptInfoPtr	appts;	

	// Make the row usable.
	TblSetRowUsable (table, row, true);
	TblSetRowSelectable (table, row, true);

	// Set the height of the row to the height of the description.
	TblSetRowHeight (table, row, rowHeight);
	
	// Store the record number as the row id.
	TblSetRowID (table, row, apptIndex);
	
	// Store the start time of the appointment in the table.
	appts = MemHandleLock(globals->ApptsH);
	time = TimeToInt(appts[apptIndex].startTime);
	MemHandleUnlock(globals->ApptsH);
	TblSetItemInt (table, row, timeColumn, time);
	
	// Store the unique id of the record in the row.
	TblSetRowData (table, row, uniqueID);

	// Set the table item type for the description,  it will differ depending
	// on the presents of a note.
	if (! iconsWidth)
		{
		TblSetItemStyle (table, row, descColumn, textTableItem);		
		TblSetItemInt (table, row, descColumn, 0);
		}
	else
		{
		TblSetItemStyle (table, row, descColumn, narrowTextTableItem);
		TblSetItemInt (table, row, descColumn, iconsWidth);
		}


	// Set the font used to draw the text of the row.
	TblSetItemFont (table, row, descColumn, fontID);

	// Mark the row invalid so that it will draw when we call the 
	// draw routine.
	TblMarkRowInvalid (table, row);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewLoadTable
 *
 * DESCRIPTION: This routine reloads appointment database records into
 *              the Day view.  This routine is called when:
 *              	o A new item is inserted
 *              	o An item is deleted
 *              	o The time of an items is changed
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/29/95	Initial Revision
 *			jmp	9/29/99	Use FrmGetFormPtr() & FrmGetObjectIndex() instead of
 *								GetObjectPtr() because GetObjectPtr() calls FrmGetActiveForm(),
 *								and FrmGetActiveForm() may return a form that isn't the one we
 *								want when other forms are up when we are called.
 *								Fixes bug #22418.
 ***********************************************************************/
static void DayViewLoadTable (void)
{
	globalVars* globals = getGlobalsPtr();
	Int16 apptIndex;
	Int16 row;
	UInt16 numRows;
	UInt16 recordNum;
	UInt16 lastAppt;
	Int16 iconsWidth;
	Int16 lineHeight;
	Int16 dataHeight;
	Int16 tableHeight;
	Int16 columnWidth;
	UInt16 pos, oldPos;
	UInt16 height, oldHeight;
	UInt32	uniqueID;
	FontID fontID;
	FontID currFont;
	FormPtr frm;
	TablePtr table;
	Boolean init;
	Boolean rowUsable;
	Boolean rowsInserted = false;
	Boolean lastItemClipped;
	ApptInfoPtr	appts;
	RectangleType r;
	UInt16 attr;
	Boolean masked;
	

	appts = MemHandleLock (globals->ApptsH);

	frm = FrmGetFormPtr (DayView);
	
	// Get the height of the table and the width of the description
	// column.
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, DayTable));
	TblGetBounds (table, &r);
	tableHeight = r.extent.y;
	columnWidth = TblGetColumnWidth (table, descColumn);


	// If we currently have a selected record, make sure that it is not
	// above the first visible record.
	if (globals->DateCurrentRecord != noRecordSelected)
		{
		apptIndex = DayViewFindAppointment (globals->DateCurrentRecord);
		if (apptIndex < globals->TopVisibleAppt)
			globals->TopVisibleAppt = apptIndex;
		}

	apptIndex = globals->TopVisibleAppt;
	lastAppt = apptIndex;

	// Load records into the table.
	row = 0;
	dataHeight = 0;
	oldPos = pos = 0;

	while (apptIndex < globals->NumAppts)
		{		
		// Compute the height of the appointment's description.
		height = DayViewGetDescriptionHeight (apptIndex, columnWidth, tableHeight, &iconsWidth, &fontID);

		// Is there enought room for at least one line of the the decription.
		currFont = FntSetFont (fontID);
		lineHeight = FntLineHeight ();
		FntSetFont (currFont);
		if (tableHeight >= dataHeight + lineHeight)
			{
			// Get the height of the current row.
			rowUsable = TblRowUsable (table, row);
			if (rowUsable)
				oldHeight = TblGetRowHeight (table, row);
			else
				oldHeight = 0;


			// Determine if the row needs to be initialized.  We will initialize 
			// the row if: the row is not usable (not displayed),  the unique
			// id of the record does not match the unique id stored in the 
			// row, or if the start time of the appointment does not match the
			// start time stored in the table.
			init = (! rowUsable);
			uniqueID = 0;
			masked = false;
			recordNum = appts[apptIndex].recordNum;
			if (recordNum != emptySlot)	// empty time slot?
				{
				DmRecordInfo (globals->ApptDB, recordNum, NULL, &uniqueID, NULL);
				init |= TblGetRowData (table, row) != uniqueID;
				
				//Mask if appropriate
				DmRecordInfo (globals->ApptDB, recordNum, &attr, NULL, NULL);
		   	masked = (((attr & dmRecAttrSecret) && globals->DatePrivateRecordVisualStatus == maskPrivateRecords));	
				}
			else if (! init)
				init |= (TblGetRowData (table, row) != 0);	
			
			if (masked != TblRowMasked(table,row))
				TblMarkRowInvalid (table, row);
				
			TblSetRowMasked(table,row,masked);
		

			if (! init)
				init = TimeToInt (appts[apptIndex].startTime) != 
					    TblGetItemInt (table, row, timeColumn);
					    
			if (! init)
				init = TblGetItemInt (table, row, descColumn) != iconsWidth;

			if (! init)
				init = TblGetItemFont (table, row, descColumn) != fontID;

			// If the record is not already being displayed in the current 
			// row load the record into the table.
			if (init)
				{
				DayViewInitRow (table, row, apptIndex, height, uniqueID, iconsWidth, fontID);
				}

			// If the height or the position of the item has changed draw the item.
			else 
				{
				TblSetRowID (table, row, apptIndex);
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

			lastAppt = apptIndex;
			apptIndex++;
			row++;
			}
		
		dataHeight += height;

		// Is the table full?
		if (dataHeight >= tableHeight)		
			{
			// If we have a currently selected record, make sure that it is
			// not below the last visible record.  If the currently selected 
			// record is the last visible record, make sure the whole description 
			// is visible.
			if (globals->DateCurrentRecord == noRecordSelected) break;

			apptIndex = DayViewFindAppointment (globals->DateCurrentRecord);
			if (apptIndex < lastAppt)
				 break;

			// Last visible?
			else if (apptIndex == lastAppt)
				{
				if ((apptIndex == globals->TopVisibleAppt) || (dataHeight == tableHeight))
					break;
					
				// Remove the top item from the table and reload the table again.
				globals->TopVisibleAppt++;
				apptIndex = globals->TopVisibleAppt;
				}
			// Below last visible.
			else
				globals->TopVisibleAppt = apptIndex;
				
			row = 0;
			dataHeight = 0;
			oldPos = pos = 0;
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
		apptIndex = globals->TopVisibleAppt;
		if (apptIndex == 0) break;
		apptIndex--;

		height = DayViewGetDescriptionHeight (apptIndex, columnWidth, tableHeight, &iconsWidth, &fontID);
			
		// If adding the item to the table will overflow the height of
		// the table, don't add the item.
		if (dataHeight + height > tableHeight)
			break;
		
		// Insert a row before the first row.
		TblInsertRow (table, 0);

		recordNum = appts[apptIndex].recordNum;
		masked = false;
		if (recordNum != emptySlot)	// empty time slot?
			{
			DmRecordInfo (globals->ApptDB, recordNum, NULL, &uniqueID, NULL);
			//mask if appropriate
			DmRecordInfo (globals->ApptDB, recordNum, &attr, NULL, NULL);
	   		masked = (((attr & dmRecAttrSecret) && globals->DatePrivateRecordVisualStatus == maskPrivateRecords));
			}
		else
			uniqueID = 0;		
				
		TblSetRowMasked(table,0,masked);


		DayViewInitRow (table, 0, apptIndex, height, uniqueID, iconsWidth, fontID);
		
		globals->TopVisibleAppt = apptIndex;
		
		rowsInserted = true;

		dataHeight += height;
		}
		
	// If rows were inserted to full out the page, invalidate the whole
	// table, it all needs to be redrawn.
	if (rowsInserted)
		TblMarkTableInvalid (table);

	// If the height of the data in the table is greater than the height
	// of the table, then the bottom of the last row is clip and the 
	// table is scrollable.
	lastItemClipped = (dataHeight > tableHeight);

	// Update the scroll arrows.
	DayViewUpdateScrollers (frm, lastAppt, lastItemClipped);

	MemPtrUnlock (appts);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewLayoutDay
 *
 * DESCRIPTION: This routine builds a list of: untimed appointment, 
 *              timed appointment, and empty time slots, for the 
 *              current day (the date store in the global variable Date).
 *
 * PARAMETERS:  retrieve - true if the list if appointment should be 
 *
 * RETURNED:    nothing
 *
 * NOTE:			the global variables ApptsH and NumAppts are set by
 *             this routine.
 *              
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/1/96	Initial Revision
 *			rbb	4/9/99	Don't use extra line when end time matches start
 *			rbb	5/18/99	Events ending on the hour caused display to skip an hour
 *			jmp	10/7/99	Replace GetObjectPtr() with FrmGetFormPtr() and 
 *								FrmGetObjectIndex(); fixes bug #22548.
 *
 ***********************************************************************/
static void DayViewLayoutDay (Boolean retieve)
{
	globalVars* globals = getGlobalsPtr();
	Int16				i, j;
	Int16				index;
	Int16				numRows;
	Int16				height;
	Int16				lineHeight;
	Int16				iconsWidth;
	Int16				tableHeight;
	Int16				columnWidth;
	FontID			fontID;
	FontID			currFont;
	TablePtr			table;
	TimeType			next;
	TimeType			endTime;
	Boolean			replace;
	Boolean			addEndTime;
	ApptInfoPtr		appts;
	ApptInfoPtr		apptsOnly;
	RectangleType 	r;
	FormPtr			frm;
	

	// Get a list of: untimed appointment and timed appointment for the
	// current day.
	
	if (retieve)
		{
			if (globals->ApptsOnlyH)
			{
				MemHandleFree (globals->ApptsOnlyH);
			}
			ApptGetAppointments (globals->ApptDB, globals->Date, 1, &globals->ApptsOnlyH, &globals->NumApptsOnly);
		}


	// Free the existing list.
	if (globals->ApptsH)
		MemHandleFree (globals->ApptsH);

	// If there are no appointsment on the day, fill in the appointment list
	// with empty time slots.
	if (! globals->ApptsOnlyH)
		{
		globals->NumAppts = globals->DayEndHour - globals->DayStartHour + 1;
		globals->ApptsH = MemHandleNew (globals->NumAppts * sizeof (ApptInfoType));
		appts = MemHandleLock (globals->ApptsH);

		for (i = 0; i < globals->NumAppts; i++)
			{
			appts[i].startTime.hours = globals->DayStartHour + i;				
			appts[i].startTime.minutes = 0;				
			appts[i].endTime.hours = globals->DayStartHour + i + 1;				
			appts[i].endTime.minutes = 0;				
			appts[i].recordNum = emptySlot;	
			}
			
		DayViewCheckForConflicts ();
		MemHandleUnlock (globals->ApptsH);	
		return;
		}
	
	// Merge empty time slots into the appointment list.
	//
	// Allocate space for the maximun number of empty time slots that
	// we may need to add. 
	globals->ApptsH = MemHandleNew ((globals->NumApptsOnly+(hoursPerDay*2)) * sizeof (ApptInfoType));
	appts = MemHandleLock (globals->ApptsH);
	globals->NumAppts = 0;
	index = 0;

	// Add the untimed events, the timed events, and a blank time slot for 
	// the end-time of each timed event.
	apptsOnly = MemHandleLock (globals->ApptsOnlyH);
	for (i = 0; i < globals->NumApptsOnly; i++)
		{
		// Find the correct position at which to insert the current event.
		replace = false;
		for (j = index; j < globals->NumAppts; j++)
			{
			if (appts[j].recordNum == emptySlot)
				{
				// If an empty time slot with the same start-time already exist, then
				// replace it.
				if (TimeToInt (appts[j].startTime) == TimeToInt (apptsOnly[i].startTime))
					{
					replace = true;
					break;
					}

				// If we find an empty time slot that has an start-time before
				// the start-time of the current event and an end-time after the 
				// the start-time of the current event, adjust the end-time of the 
				// empty time such that it is equal to the start of the current event.
			 	if (TimeToInt (appts[j].startTime) < TimeToInt (apptsOnly[i].startTime) &&
			 		 TimeToInt (appts[j].endTime)   > TimeToInt (apptsOnly[i].startTime))
			 		appts[j].endTime = apptsOnly[i].startTime;
			 	}
			
			if (TimeToInt (appts[j].startTime) > TimeToInt (apptsOnly[i].startTime))
				{
				// Make room for the empty time slot we're about to add.
				MemMove (&appts[j+1], &appts[j], (globals->NumAppts-j) * sizeof (ApptInfoType));
				break;
				}
			}

		// Add the event to the list.
		appts[j] = apptsOnly[i];
		index = j + 1;
		if (! replace)
			globals->NumAppts++;
			

		// If the event is a timed event add an empty time slot to display the end-time.
		// If the event has no duration, skip it to avoid displaying the same time twice.
		if ( (TimeToInt (apptsOnly[i].startTime) != apptNoTime)
				&& (TimeToInt (apptsOnly[i].startTime) != TimeToInt (apptsOnly[i].endTime)) )
			{
			// Find the correct position at which to insert the end-time time slot.
			addEndTime = true;
			for (j = index; j < globals->NumAppts; j++)
				{
				// If an event already exist that has a start-time equal to the 
				// end-time of the current event then we don't need to add an 
				//  end-time time slot.
				if (TimeToInt (appts[j].startTime) == TimeToInt (apptsOnly[i].endTime))
					{
					addEndTime = false;
					break;
					}
					
				// We're found the position to insert the empty time slot when we find 
				// an appointment with a start-time greater than the end-time of the 
				// current event.
				if (TimeToInt (appts[j].startTime) > TimeToInt (apptsOnly[i].endTime))
					{
					// Make room for the empty time slot we're about to add.
					MemMove (&appts[j+1], &appts[j], (globals->NumAppts-j) * sizeof (ApptInfoType));
					break;
					}
				}
			
			if (addEndTime)
				{
				// The end time of the empty time slot is the earlier of:
				//		o the start time plus one hour
				//		o 11:55 pm
				//		o the start time of the next event.
				if (apptsOnly[i].endTime.hours < 23)
					{
					endTime.hours = apptsOnly[i].endTime.hours + 1;
					endTime.minutes = 0;
//					endTime.minutes = apptsOnly[i].endTime.minutes;
					}
				else
					{
					endTime.hours = 23;		// max end time is 11:55 pm
					endTime.minutes = 55;
					}
	
				if (j < globals->NumAppts && TimeToInt(endTime) > TimeToInt(appts[j+1].startTime))
					endTime = appts[j+1].startTime;
		
	
				appts[j].recordNum = emptySlot;
				appts[j].startTime = apptsOnly[i].endTime;
				appts[j].endTime = endTime;
				globals->NumAppts++;
				}
			}
		}


	// Reserve spase for the time bars.  We will show time bars if the user
	// has requested them or if there are overlapping appointments.
	DayViewCheckForConflicts ();
	

	// Determine if that is space to add empty time slot, these time slot are in
	// addition to the empty time slot that represent end times of events.
	frm = FrmGetFormPtr (DayView);
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, DayTable));
	numRows = TblGetNumberOfRows (table);
	if (( ! globals->CompressDayView) || (numRows > globals->NumAppts))
		{
		TblGetBounds (table, &r);
		tableHeight = r.extent.y;
		height = 0;
		columnWidth = TblGetColumnWidth (table, descColumn);

		currFont = FntSetFont (apptEmptyDescFont);
		lineHeight = FntLineHeight ();
		FntSetFont (currFont);

		if (globals->CompressDayView)
			{
			for (j = 0; j < globals->NumAppts; j++)
				{
				height += DayViewGetDescriptionHeight (j, columnWidth, tableHeight, &iconsWidth, &fontID);
				if (height >= tableHeight)
					break;
				}
			}
		
		// Add empty time slots to the list of appointment until the table is full.
		next.hours = globals->DayStartHour;
		next.minutes = 0;
		i = 0;

		while (( ! globals->CompressDayView) || (height + lineHeight <= tableHeight))
			{
			if ((i < globals->NumAppts) &&
				 (TimeToInt (next) >= TimeToInt (appts[i].startTime)))
				{
				if (TimeToInt (next) <= TimeToInt (appts[i].endTime) &&
					(appts[i].endTime.hours >= globals->DayStartHour))
					{
					next = appts[i].endTime;
					if (next.minutes || (TimeToInt (appts[i].startTime)
													== TimeToInt (appts[i].endTime)))
						{
						next.hours++;
						next.minutes = 0;
						}
					}
				i++;
				}

			// Insert an empty time slot if we're not passed the end of the 
			// day.
			else if ((next.hours < globals->DayEndHour) || 
						((next.hours == globals->DayEndHour) && next.minutes == 0))
				{
				MemMove (&appts[i+1], &appts[i], (globals->NumAppts-i) * sizeof (ApptInfoType));
				globals->NumAppts++;
	
				appts[i].startTime = next;
				appts[i].recordNum = emptySlot;	
	
				// The end time is the beginning of the next hour or the 
				// start time of the next appointment, which ever is earliest.
				next.hours++;
				next.minutes = 0;
				if ( (i+1 < globals->NumAppts) &&
					  (TimeToInt (next) > TimeToInt (appts[i+1].startTime)))
					next = appts[i+1].startTime;
	
				appts[i].endTime = next;
				
				height += DayViewGetDescriptionHeight (i, columnWidth, tableHeight, &iconsWidth, &fontID);
				i++;
				}

			else if (i < globals->NumAppts)
				{
				next.hours++;
				next.minutes = 0;
				}
			
			else
				break;
			}
		}
				
	MemHandleUnlock (globals->ApptsOnlyH);
			
	// Release any unused space in the appointment list;
	MemHandleUnlock (globals->ApptsH);
	MemHandleResize (globals->ApptsH, (globals->NumAppts * sizeof (ApptInfoType)));
}

/***********************************************************************
 *
 * FUNCTION:    DayViewSelectTime
 *
 * DESCRIPTION: This routine is called when a time item in the day view
 *              table is selected.  The time picker is displayed, if 
 *              the start or end time of the appointment is changed, 
 *              the day's appointments are resorted and the appointment
 *              table is redrawn.
 *
 * PARAMETERS:  table - table of appointments
 *              row   - row selected
 *              
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/12/95		Initial Revision
 *
 ***********************************************************************/
static void DayViewSelectTime (TablePtr table, UInt16 row)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 apptIndex;
	UInt16 recordNum;
	Boolean sameTime;
	TimeType startTime;
	TimeType endTime;
	ApptInfoPtr appts;

	// Get the record index, start time, and end time.
	apptIndex = TblGetRowID (table, row);
	appts = MemHandleLock (globals->ApptsH);
	startTime = appts[apptIndex].startTime;
	endTime = appts[apptIndex].endTime;
	recordNum = appts[apptIndex].recordNum;
	MemHandleUnlock (globals->ApptsH);
	
	// Display the time picker, exit if the time picker if not confirmed.
	sameTime = (! GetTime (&startTime, &endTime, setTimeTitleStrID));
	if (sameTime)
		{
		return;
		}
		

	//if (MoveEvent (&recordNum, startTime, endTime, Date, true, &moved))
	//	return;
	
}


/***********************************************************************
 *
 * FUNCTION:    DayViewItemSelected
 *
 * DESCRIPTION: This routine is called when an item in Day View table
 *              is selected.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/9/95	Initial Revision
 *			jmp	9/17/99	Use NewNoteView instead of NoteView.
 *
 ***********************************************************************/
static void DayViewItemSelected (TablePtr table, UInt16 row)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 apptIndex;
	ApptInfoPtr appts;
	
	// Get the record index of the selected appointment.
	apptIndex = TblGetRowID (table, row);
	appts = MemHandleLock (globals->ApptsH);
	globals->DateCurrentRecord = appts[apptIndex].recordNum;
	MemPtrUnlock (appts);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewSetTitle
 *
 * DESCRIPTION: Set the Day View form's title, based on the Day and
 *		LongDateFormat global variables.
 *
 * PARAMETERS:  frmP		Pointer to Day View form.
 *
 * RETURNED:    nothing
 *
 * HISTORY:
 *		09/12/99	kwk	Created by Ken Krugler.
 *		10/22/99	kwk	Directly use short date format as index into the
 *							formatting strings.
 *
 ***********************************************************************/
static void DayViewSetTitle(FormType* frmP)
{
	globalVars* globals = getGlobalsPtr();
	Char title[longDateStrLength];
	UInt16 dateFormatIndex;
	MemHandle templateH;
	Char* templateP;

	// We can't use the long date format to guess at the short date
	// format, since there's not a one-to-one mapping set up in the
	// Formats panel. We'll directly use the short date format.
	
	
	// We need to derive the appropriate date template string based on
	// the LongDateFormat global, which is loaded from sys prefs (and
	// thus is set by the user in the Formats panel).
	if (globals->ShortDateFormat > dfYMDWithDashes)
		{
		ErrNonFatalDisplay("Unknown short date format");
		dateFormatIndex = 0;
		}
	else
		{
		dateFormatIndex = (UInt16)globals->ShortDateFormat;
		}
	
	templateH = DmGetResource(strRsc, DayViewFirstTitleTemplateStrID + dateFormatIndex);
	templateP = (Char*)MemHandleLock(templateH);
	DateTemplateToAscii(templateP, globals->Date.month, globals->Date.day, (globals->Date.year+firstYear), title, sizeof(title) - 1);
	MemHandleUnlock(templateH);

	FrmCopyTitle (frmP, title);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewDrawTitle
 *
 * DESCRIPTION: This routine draws the day view title and highlights
 *              the current day's the day-of-week push button. 
 *
 * PARAMETERS:  void
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/1/95	Initial Revision
 *
 ***********************************************************************/
static void DayViewDrawTitle (void)
{
	globalVars* globals = getGlobalsPtr();
	UInt16	dayOfWeek;
	FormPtr	frm;

	frm = FrmGetActiveForm ();
	DayViewSetTitle(frm);
	
	// Update the day-of-week push button to highlight the correct day of
	// the week.
	dayOfWeek = DatebookDayOfWeek (globals->Date.month, globals->Date.day, globals->Date.year+firstYear);
	FrmSetControlGroupSelection (frm, DayOfWeekGroup, DayDOW1Button + dayOfWeek);

	globals->TimeDisplayed = false;
}


/***********************************************************************
 *
 * FUNCTION:    DayViewShowTime
 *
 * DESCRIPTION: This routine display the current time in the title of the
 *              day view.
 *
 * PARAMETERS:  frm - pointer to the day view form.
 *
 * RETURNED:    nothing
 *
 * NOTE:        The global variables TimeDisplayed and TimeDisplayTick are
 *					 set by this routine.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/14/96	Initial Revision
 *			grant 2/2/99	update TimeDisplayTick (now matches WeekViewShowTime
 *								and MonthViewShowTime)
 *
 ***********************************************************************/
static void DayViewShowTime (void)
{
	globalVars* globals = getGlobalsPtr();
	Char				title[timeStringLength];
	DateTimeType 	dateTime;

	TimSecondsToDateTime (TimGetSeconds (), &dateTime);
	TimeToAscii (dateTime.hour, dateTime.minute, globals->TimeFormat, title);
	FrmCopyTitle (FrmGetActiveForm (), title);
	
	globals->TimeDisplayed = true;
	globals->TimeDisplayTick = TimGetTicks () + timeDisplayTicks;
}


/***********************************************************************
 *
 * FUNCTION:    DayViewHideTime
 *
 * DESCRIPTION: If the title of the Day View is displaying the current 
 *              time, this routine will change the title to the standard
 *					 title (the current date).
 *
 * PARAMETERS:  nothing
 *
 * PARAMETERS:  hide - true to always hide, false hide only if
 *                     to time has been display for the require
 *                      length of time.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/14/96	Initial Revision
 *			grant	2/2/99	Use TimeToWait(), don't use EvtSetNullEventTick()
 *
 ***********************************************************************/
static void DayViewHideTime (Boolean hide)
{
	globalVars* globals = getGlobalsPtr();
	if (globals->TimeDisplayed)
		{
		if (hide || TimeToWait() == 0)
			{
			// If the Day View is the draw window then redraw the title.
			if (WinGetDrawWindow () == FrmGetWindowHandle (FrmGetFormPtr (DayView)))
				DayViewDrawTitle ();
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    DayViewDrawDate
 *
 * DESCRIPTION: This routine display the date passed.
 *
 * PARAMETERS:  date - date to display
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/29/95	Initial Revision
 *
 ***********************************************************************/
static void DayViewDrawDate (DateType date)
{
	globalVars* globals = getGlobalsPtr();
	TablePtr table;

	// Adjust the current date.
	globals->Date = date;

	DayViewDrawTitle ();
		
	// Get all the appointments and empty time slots on the new day.
	DayViewLayoutDay (true);

	// Determine the first appointment to display.
	DayViewSetTopAppointment ();

 	// Load the appointments and empty time slots into the table object 
	// that will display them.
	DayViewLoadTable ();
	
	// Draw the new day's events.
	table = GetObjectPtr (DayTable);
	TblRedrawTable (table);
	DayViewDrawTimeBars ();
}


/***********************************************************************
 *
 * FUNCTION:    DayViewDayOfWeekSelected
 *
 * DESCRIPTION: This routine is called when of the day-of-week push buttons
 *              is pressed.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/27/95	Initial Revision
 *
 ***********************************************************************/
static void DayViewDayOfWeekSelected (EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Int16			adjust;
	UInt16		dayOfWeek;
	UInt16		newDayOfWeek;

	// Adjust the current date.
	dayOfWeek = DatebookDayOfWeek (globals->Date.month, globals->Date.day, globals->Date.year+firstYear);
	newDayOfWeek = event->data.ctlSelect.controlID - DayDOW1Button;
	if (dayOfWeek == newDayOfWeek) return;
	
	adjust = newDayOfWeek - dayOfWeek;
	DateAdjust (&globals->Date, adjust);

	DayViewDrawDate (globals->Date);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewGoToDate
 *
 * DESCRIPTION: This routine displays the date picker so that the 
 *              user can select a date to navigate to.  If the date
 *              picker is confirmed, the date selected is displayed.
 *
 *              This routine is called when a "go to" button is pressed.
 *              
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/29/95	Initial Revision
 *
 ***********************************************************************/
static void DayViewGoToDate (void)
{
	globalVars* globals = getGlobalsPtr();
	Char* title;
	MemHandle titleH;
	Int16 month, day, year;

	// Get the title for the date picker dialog box.
	titleH = DmGetResource (strRsc, goToDateTitleStrID);
	title = MemHandleLock (titleH);

	day = globals->Date.day;
	month = globals->Date.month;
	year = globals->Date.year + firstYear;

	// Display the date picker.
	if (SelectDay (selectDayByDay, &month, &day, &year, title))
		{
		globals->Date.day = day;
		globals->Date.month = month;
		globals->Date.year = year - firstYear;

		DayViewDrawDate (globals->Date);
		}
		
	MemHandleUnlock (titleH);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewGotoAppointment
 *
 * DESCRIPTION: This routine sets gloabal variables such the Day View
 *              will display the text found by the text search
 *              command.
 *
 * PARAMETERS:  event - frmGotoEvent 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/24/95	Initial Revision
 *
 ***********************************************************************/
static void DayViewGotoAppointment (EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	globals->TopVisibleAppt = 0;
	globals->ItemSelected = true;
	globals->DateCurrentRecord = event->data.frmGoto.recordNum;
	globals->DayEditPosition = event->data.frmGoto.matchPos;
	globals->DayEditSelectionLength = event->data.frmGoto.matchLen;
	
	SetDateToNextOccurrence (globals->DateCurrentRecord);
}


/***********************************************************************
 *
 * FUNCTION:    DayViewScroll
 *
 * DESCRIPTION: This routine scrolls the day of of appointments
 *              in the direction specified.
 *
 * PARAMETERS:  direction - up or dowm
 *              wrap      - if true the day is wrap to the first appointment
 *                          if we're at the end of the day.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *
 ***********************************************************************/
static void DayViewScroll (WinDirectionType direction, Boolean wrap)
{
	globalVars* globals = getGlobalsPtr();
	Int16				row;
	Int16				height;
	UInt16			apptIndex;
	Int16				iconsWidth;
	Int16 			columnWidth;
	Int16 			tableHeight;
	FontID			fontID;
	TablePtr			table;
	RectangleType	r;

	table = GetObjectPtr (DayTable);
	TblReleaseFocus (table);

	// Get the height of the table and the width of the description
	// column.
	TblGetBounds (table, &r);
	tableHeight = r.extent.y;
	height = 0;
	columnWidth = TblGetColumnWidth (table, descColumn);

	apptIndex = globals->TopVisibleAppt;

	// Scroll the table down.
	if (direction == winDown)
		{
		// If we're at the bottom of the day, and page wrapping is allowed,
		// go to the top of the page.
		if (wrap && ( ! CtlEnabled (GetObjectPtr (DayDownButton))))
			globals->TopVisibleAppt = 0;
		
		else if (globals->TopVisibleAppt+1 >= globals->NumAppts) 
			return;

		else
			{
			row = TblGetLastUsableRow (table);
			apptIndex = TblGetRowID (table, row);				
	
			// If there is only one appointment visible, this is the case 
			// when a appointment occupies the whole screeen, and its not
			// the last appointment, move to the next appointment.
			if (row == 0)
				apptIndex++;
			}
		}


	// Scroll the table up.
	else
		{
		if (globals->TopVisibleAppt == 0) return;
		
		// Scan the records starting with the first visible record to 
		// determine how many record we need to scroll.  Since the 
		// heights of the records vary,  we sum the heights of the 
		// records until we get a screen full.
		height = TblGetRowHeight (table, 0);
		if (height >= tableHeight)
			height = 0;							

		while ( (height < tableHeight) && (apptIndex > 0) )
			{
			height += DayViewGetDescriptionHeight (apptIndex-1, columnWidth, tableHeight, &iconsWidth, &fontID);
			if ((height <= tableHeight) || (apptIndex == globals->TopVisibleAppt))
				apptIndex--;
			}
		}

	TblMarkTableInvalid (table);
				
	globals->TopVisibleAppt = apptIndex;
	DayViewLoadTable ();	

	TblUnhighlightSelection (table);
	TblRedrawTable (table);
	DayViewDrawTimeBars ();
}


/***********************************************************************
 *
 * FUNCTION:    DayViewUpdateDisplay
 *
 * DESCRIPTION: This routine updates the display of the day View.
 *
 * PARAMETERS:  updateCode - a code that indicated what changes have been
 *                           made to the Day View.
 *                		
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/12/95	Initial Revision
*			jmp	11/01/99	Fixed problem on frmRedrawUpdateCode events when
 *								we're still in the edit state but we weren't redrawing
 *								the edit indicator.  Fixes Datebook part of bug #23235.
 *
 ***********************************************************************/
static void DayViewUpdateDisplay (UInt16 updateCode)
{
	Int16 row, column;
	TablePtr table;
	
	table = GetObjectPtr (DayTable);

	// Was the UI unable to save an image of the day view when is 
	// obscured part of the day view with another dialog?  If not,
	// we'll handle it here.
	if (updateCode & frmRedrawUpdateCode)
		{
		FrmDrawForm (FrmGetActiveForm ());
		
		// If we're editing, then find out which row is being edited,
		// mark it invalid, and redraw the table.
		if (TblEditing(table))
			{
			TblGetSelection (table, &row, &column);
			TblMarkRowInvalid(table, row);
			TblRedrawTable (table);
			}
			
		DayViewDrawTimeBars ();
		return;
		}
		
	// Was the display options modified (Preferences dialog) or was the 
	// font changed.
	
	
	DayViewLoadTable ();
	TblRedrawTable (table);
	DayViewDrawTimeBars ();
}


/***********************************************************************
 *
 * FUNCTION:    DayViewInit
 *
 * DESCRIPTION: This routine initializes the "Day View" of the 
 *              Datebook application.
 *
 * PARAMETERS:  frm - pointer to the day view form.
 *
 * RETURNED:    nothing
 *
 *	HISTORY:
 *		06/12/95	art	Created by Art Lamb.
 *		08/04/99	kwk	Copy day-of-week pushbutton ptrs vs. just first byte
 *							of each label.
 *
 ***********************************************************************/
static void DayViewInit (FormPtr frm)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 month;
	UInt16 day;
	UInt16 year;
	UInt16 dayOfWeek;
	UInt16 id;
	UInt16 row;
	UInt16 rowsInTable;
	TablePtr table;

	// Get the day we're displaying.
	day = globals->Date.day;
	month = globals->Date.month;
	year = globals->Date.year+firstYear;

	DayViewSetTitle(frm);
	
	// If the start-day-of-week is monday rearrange the labels on the 
	// days-of-week push buttons.
	if (globals->StartDayOfWeek == monday)
		{
		const Char* sundayLabel = CtlGetLabel (GetObjectPtr (DayDOW1Button));
		for (id = DayDOW1Button; id < DayDOW7Button; id++)
			{
			CtlSetLabel (GetObjectPtr (id), CtlGetLabel (GetObjectPtr (id + 1)));
			}
		CtlSetLabel (GetObjectPtr (DayDOW7Button), sundayLabel);
		}

	
	// Highlight the correct day-of-week push button.
	dayOfWeek = DatebookDayOfWeek (month, day, year);
	FrmSetControlGroupSelection (frm, DayOfWeekGroup, DayDOW1Button + dayOfWeek);

	// Highlight the Day View push button.
	
	// Initialize the table used to display the day's appointments.
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, DayTable));

	rowsInTable = TblGetNumberOfRows (table);
	for (row = 0; row < rowsInTable; row++)
		{		
		TblSetItemStyle (table, row, timeBarColumn, customTableItem);
		TblSetItemStyle (table, row, timeColumn, customTableItem);
		TblSetItemStyle (table, row, descColumn, textTableItem);
		TblSetRowUsable (table, row, false);
		}

	
	TblSetColumnUsable (table, timeBarColumn, true);
	TblSetColumnUsable (table, timeColumn, true);
	TblSetColumnUsable (table, descColumn, true);
	
	TblSetColumnMasked (table, descColumn, true);

	TblSetColumnSpacing (table, timeBarColumn, spaceAfterTimeBarColumn);
	TblSetColumnSpacing (table, timeColumn, spaceAfterTimeColumn);
	
	TblSetColumnEditIndicator (table, timeBarColumn, false);

	// Set the callback routines that will load and save the 
	// description field.
	TblSetLoadDataProcedure (table, descColumn, DayViewGetDescription);
	
	// Set the callback routine that draws the time field.
	TblSetCustomDrawProcedure (table, timeColumn, DayViewDrawTime);

	// Set the callback routine that draws the note, alarm, and repeat icons.
	TblSetCustomDrawProcedure (table, descColumn, DayViewDrawIcons);

	// By default the list view assume no time bar are displayed.
	globals->TimeBarColumns = 0;

	// Get all the appointments and empty time slots on the current day.
	
	DayViewLayoutDay (true);

	
	// If we do not have an appointment selected, then position the table
	// so the the first appointment of the day is visible, unless we're on
	// today, then make sure the next appointment is visible.
	if (! globals->ItemSelected)
		DayViewSetTopAppointment ();

	// Load the appointments and empty time slots into the table object 
	// that will display them.
	if (globals->PendingUpdate && globals->ItemSelected)
		DayViewUpdateDisplay (globals->PendingUpdate);
	else
		DayViewLoadTable ();


	// Initial miscellaneous global variables.
	globals->TimeDisplayed = false;
	globals->PendingUpdate = 0;
}


static void DayOnOK()
{
	globalVars* globals = getGlobalsPtr();
	if (globals->DateCurrentRecord != noRecordSelected)
	{
		if(DmRecordInfo (globals->ApptDB, globals->DateCurrentRecord, NULL, &globals->gLinkID, NULL)!=errNone)
			globals->gLinkID = 0;
	}
	else
	{
		globals->gLinkID = 0;
	}
	StopApplication();
	FrmUpdateForm(NewLinkDialog, 0);
	FrmUpdateForm (EditView, updatePrefs);
	FrmReturnToForm(0);
}

static void DayOnCancel()
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
 * FUNCTION:    DayViewHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the Day View
 *              of the Datebook application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 *	HISTORY:
 *		06/12/95	art	Created by Art Lamb.
 *		11/22/98	kwk	Handle command keys in separate code block so that
 *							TxtCharIsPrint doesn't get called w/virtual chars.
 *		06/22/99	CS		Standardized keyDownEvent handling
 *							(TxtCharIsHardKey, commandKeyMask, etc.)
 *		08/04/99	kwk	Tweak auto-entry generation w/keydown event so that
 *							it works when a FEP is active.
 *		09/23/99 jmp	On frmCloseEvent, don't call FrmSetFocus() with
 *							a NULL frm to prevent debug build from going ErrFatalDisplay
 *							crazy.
 *		11/04/99	jmp	Restored DayView edit state after beaming; fixes bug #23315.
 *
 ***********************************************************************/
Boolean DayViewHandleEvent (EventType* event)
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
				DateAdjust (&globals->Date, -1);
				DayViewDrawDate (globals->Date);
				handled = true;
				}
	
			// Scroll down key pressed?
			else if (event->data.keyDown.chr == vchrPageDown)
				{
				DateAdjust (&globals->Date, 1);
				DayViewDrawDate (globals->Date);
				handled = true;
				}
	
			else
				{
				handled = false;
				}
			}

		}
	

	// If the pen is not in any of the object of the view, take the 
	// view out of edit mode.
	// Check for buttons that take us out of edit mode.
	
	// Handle the button that was selected.
	else if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			
			case DayOKButton:
				DayOnOK();
				handled = true;
				break;
			case DayCancelButton:
				DayOnCancel();
				handled = true;
				break;
			
			case DayGoToButton:
				DayViewGoToDate ();
				handled = true;
				break;

			case DayDOW1Button:
			case DayDOW2Button:
			case DayDOW3Button:
			case DayDOW4Button:
			case DayDOW5Button:
			case DayDOW6Button:
			case DayDOW7Button:
				DayViewDayOfWeekSelected (event);
				handled = true;
				break;
			}
		}


	// If the pen when down in the details button but did not go up in it,
	// restore the edit state.
	// Handle the scrolling controls.
	else if (event->eType == ctlRepeatEvent)
		{
		switch (event->data.ctlRepeat.controlID)
			{
			case DayPrevWeekButton:
				DateAdjust (&globals->Date, -daysInWeek);
				DayViewDrawDate (globals->Date);
				break;

			case DayNextWeekButton:
				DateAdjust (&globals->Date, daysInWeek);
				DayViewDrawDate (globals->Date);
				break;

			case DayUpButton:
				DayViewScroll (winUp, false);
				break;
				
			case DayDownButton:
				DayViewScroll (winDown, false);
				break;
			}
		}


	// Check if we've changed row in the day view table, if so 
	// clear the edit state.
	
	// An item in the table has been selected.
	/*else if (event->eType == tblSelectEvent)
	{
		handled = true;
	}*/
	else if (event->eType == tblEnterEvent)
		{
		table = GetObjectPtr (DayTable);
		DayViewItemSelected(table, event->data.tblEnter.row);
		TblSelectItem(table, event->data.tblEnter.row, timeColumn);
		handled = true;
		//DayViewSelectTime (table, event->data.tblEnter.row);
		
		/*if(event->data.tblEnter.column != timeColumn)
		{
			TblSelectItem(table, event->data.tblEnter.row, timeColumn);
			handled = true;
		}
		else
		{
			handled = false;
		}*/
		//DayViewItemSelected (event);
		//TblSelectItem(table, event->data.tblEnter.row, timeColumn);
		
		}
		

	// Expand or compress the height of the appointments description.
	
	// Add the buttons that we want available on the command bar, based on the current context
	
	else if (event->eType == frmOpenEvent)
	{
		frm = FrmGetActiveForm ();
		StartApplication ();
		DayViewInit (frm);
		FrmDrawForm (frm);
		DayViewDrawTimeBars ();
		handled = true;
	}


	else if (event->eType == frmUpdateEvent)
		{
		DayViewUpdateDisplay (event->data.frmUpdate.updateCode);
		handled = true;
		}

	else if (event->eType == frmCloseEvent)
		{
		// If necessary, release the focus before freeing ApptsH.  Releasing the focus causes some
		// data to be saved, and that action depends on ApptsH.  If the data isn't
		// saved now, trying to save it later will access a NULL ApptsH.
		//MemHandleFree (ApptsH);
		//ApptsH = 0;
		
		// Also free ApptsOnlyH if necessary (was allocated in last call to DayViewLayoutDay)
		StopApplication();
		if (globals->ApptsOnlyH)
			{
			MemHandleFree(globals->ApptsOnlyH);
			globals->ApptsOnlyH = 0;
			}
		}

	return (handled);
}

