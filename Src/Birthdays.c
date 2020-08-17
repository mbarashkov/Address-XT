#include <PalmOS.h>
#include "AddrTools.h"
#include "AddrTools2.h"
#include "Address.h"
#include "Birthdays.h"
#include "globals.h"
#include "AddrXTDB.h"
#include "dia.h"
#include "syslog.h"

#define TBLB_MAINCOL 0

void 	BirthdaysOnMonthSelector();
void 	BInitForm();
void 	InitBScrollBar();
Int16 	NumBirthdays(Int16 month, Boolean ret, UInt16 offset, UInt32* idRet, UInt32* secondsRet);
static 	void BDrawRecord(MemPtr tTable, Int16 tRow, Int16 tColumn, RectangleType *tBounds);
void 	BirthdaysOnSelect(EventType *event);
static 	Boolean OnScroll(EventPtr event);
Int16 	AddrXTCompare (void *rec1, void *rec2,
		Int16 other, SortRecordInfoPtr rec1SortInfo,
		SortRecordInfoPtr rec2SortInfo,
		MemHandle appInfoH);

static Boolean OnScroll(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	globals->gBScrollPosition=event->data.sclRepeat.newValue;
	TblDrawTable(globals->gBTable);
	return false;
}


void BirthdaysOnSelect(EventType *event)
{
	globalVars* globals = getGlobalsPtr();
	int row = event->data.tblSelect.row + globals->gBScrollPosition;
	UInt32 id, seconds;
	UInt16 index;
	if(row >= globals->gBRows)
		return;
	NumBirthdays(globals->gBListMonth, true, row, &id, &seconds);
	DmFindRecordByID(globals->AddrDB, id, &index);
	globals->CurrentCategory = dmAllCategories;
	globals->CurrentRecord = index;
	FrmCloseAllForms();
	FrmGotoForm(RecordView);		
}
		
Int16 NumBirthdays(Int16 month, Boolean ret, UInt16 offset, UInt32* idRet, UInt32* secondsRet)
{
	//count birthdays in this month
	globalVars* globals = getGlobalsPtr();
	UInt16 addrXTDBSize;
	DateTimeType date;
	UInt16 rv=0;
	UInt16 attr, index, counter;
	DmOpenRef addrXTDB;
	CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &addrXTDB);
	if(addrXTDB == 0)
		return 0;
	addrXTDBSize = DmNumRecords(addrXTDB);
	if(addrXTDBSize == 0)
	{
		DmCloseDatabase(addrXTDB);
		return 0;
	}
	for(counter=0;counter<addrXTDBSize;counter++)
	{
		AddrXTDBRecordType rec;
		MemHandle mH;
		UInt32 id, seconds;
		
		DmRecordInfo(addrXTDB, counter, &attr, NULL, NULL);
		if(attr & dmRecAttrDelete )
			continue;
		
		if(AddrXTDBGetRecord(globals->adxtLibRef, addrXTDB, counter, &rec, &mH) != 0)
			continue;
		
		id = rec.id;
		seconds = rec.bday;
		
		MemHandleUnlock(mH);
		
		TimSecondsToDateTime(seconds, &date);
		if(date.month == month)
		{
			if(DmFindRecordByID(globals->AddrDB, id, &index)!=0)
				continue;
			else
			{
				DmRecordInfo (globals->AddrDB, index, &attr, NULL, NULL);
				if(!((attr & dmRecAttrSecret)  && globals->PrivateRecordVisualStatus != showPrivateRecords))
				{
					if(rv == offset && ret)
					{
						*idRet = id;
						*secondsRet = seconds;
						DmCloseDatabase(addrXTDB);
						return counter;
					}
					rv++;								
				}
			}
		}
	}
	DmCloseDatabase(addrXTDB);
	if(ret == false)
		return rv;	
	else
		return -1;
}

void BirthdaysOnMonthSelector()
{
	globalVars* globals = getGlobalsPtr();
	Int16 day=1;
	
	SelectDay(selectDayByMonth, &(globals->gBListMonth), &day, &(globals->gBListYear), "Select month"); 
	
	DateToAscii(globals->gBListMonth, day, globals->gBListYear, dfMYMed, (Char*)&(globals->gMonth));
	CustomSetCtlLabelPtrSmp(BirthdaysMonthSelector, globals->gMonth);
	
	globals->gBRows = NumBirthdays(globals->gBListMonth, false, 0, 0, 0);
	
	TblDrawTable(globals->gBTable);
	InitBScrollBar();
}

void InitBScrollBar()
{
	globalVars* globals = getGlobalsPtr();
	Int16 cnt;
	for(cnt=0;cnt<globals->gBTableRows;cnt++)
	{
		TblSetRowSelectable(globals->gBTable, cnt, true);
	}
	if(globals->gBTableRows>globals->gBRows)
	{
		CustomHideObjectSmp(BirthdaysScrollbar);
		for(cnt=globals->gBRows;cnt<globals->gBTableRows;cnt++)
		{
			TblSetRowSelectable(globals->gBTable, cnt, false);
		}
		return;
	}
	else
		CustomShowObjectSmp(BirthdaysScrollbar);
	SclSetScrollBar((ScrollBarPtr)CustomGetObjectPtrSmp(BirthdaysScrollbar), globals->gBScrollPosition, 0, globals->gBRows-globals->gBTableRows, globals->gBRows);
}

static void BDrawRecord(MemPtr tTable, Int16 tRow, Int16 tColumn, RectangleType *tBounds)
{
#pragma unused(tTable)
	globalVars* globals = getGlobalsPtr();
	UInt16 index, lAbsPos=tRow+globals->gBScrollPosition;
	UInt32 id, seconds;
	MemHandle            recordH;
	univAddrDBRecordType     record;
	DateTimeType birthdate;
	Char str[255];
	UInt16 d, m, year, y; Int16 x;
	DateFormatType dateFormat;
	Int16 shortenedFieldWidth;
	Int16 fieldSeparatorWidth;
	Char * name1;
	Char * name2;
	Int16 name1Length;
	Int16 name2Length;
	Int16 name1Width;
	Int16 name2Width;
	Boolean addrListHighRes;
	Boolean name1HasPriority;
	
	if(lAbsPos >= globals->gBRows)
		return;
		
	NumBirthdays(globals->gBListMonth, true, lAbsPos, &id, &seconds);
	
	switch(tColumn)
	{
		case TBLB_MAINCOL:
			FntSetFont(stdFont);
			TimSecondsToDateTime(seconds, &birthdate);
			dateFormat=PrefGetPreference(prefDateFormat);
			d=birthdate.day;
			m=birthdate.month;
			year=birthdate.year;
			DateToAscii(m, d, year, dateFormat, str);	
			y = tBounds->topLeft.y;
			x = tBounds->topLeft.x+tBounds->extent.x-40;
			WinDrawTruncChars(str, StrLen(str), x, y, 40);
	
			addrListHighRes=globals->AddrListHighRes;
			globals->AddrListHighRes=false;
			
			x = tBounds->topLeft.x;
			
			DmFindRecordByID(globals->AddrDB, id, &index);
			
			univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, index, &record, &recordH);
			FntSetFont (stdFont);
			name1HasPriority = ToolsDetermineRecordName (globals->adxtLibRef, &record, &shortenedFieldWidth, 
			&fieldSeparatorWidth, globals->SortByCompany,
			 &name1, &name1Length, &name1Width, &name2, 
			 &name2Length, &name2Width, NULL, NULL, tBounds->extent.x-45, true);
	
	
			ToolsDrawRecordName(globals->adxtLibRef, name1, name1Length, name1Width, name2, name2Length, name2Width,
				   tBounds->extent.x-45, &x, y, shortenedFieldWidth, fieldSeparatorWidth, false,
				   name1HasPriority, false, true);
			MemHandleUnlock (recordH);
			
			globals->AddrListHighRes=addrListHighRes;
 			break;
	}			
	
}

Int16 AddrXTCompare (void *rec1, void *rec2, Int16 other, SortRecordInfoPtr rec1SortInfo, 
						SortRecordInfoPtr rec2SortInfo, MemHandle appInfoH)
{
#pragma unused(rec1, rec2, other, appInfoH)
	globalVars* globals = getGlobalsPtr();
	UInt32 id1, id2, seconds1, seconds2;
	UInt16 index1, index2;
	DateTimeType date1, date2;
	AddrXTDBRecordType xtRec1, xtRec2;
	MemHandle mH1, mH2;
	
	id1 =  rec1SortInfo->uniqueID[0];
	id1 = (id1 << 8) | rec1SortInfo->uniqueID[1];
	id1 = (id1 << 8) | rec1SortInfo->uniqueID[2];
				
	id2 =  rec2SortInfo->uniqueID[0];
	id2 = (id2 << 8) | rec2SortInfo->uniqueID[1];
	id2 = (id2 << 8) | rec2SortInfo->uniqueID[2];

	if(DmFindRecordByID(globals->gAddrXTDB, id1, &index1)!=0)
		return 0;
	if(DmFindRecordByID(globals->gAddrXTDB, id2, &index2)!=0)
		return 0;
	
	if(AddrXTDBGetRecord(globals->adxtLibRef, globals->gAddrXTDB, index1, &xtRec1, &mH1) != 0)
		return 0;
	if(AddrXTDBGetRecord(globals->adxtLibRef, globals->gAddrXTDB, index2, &xtRec2, &mH2) != 0)
	{
		MemHandleUnlock(mH1);
		return 0;
	}
	
	seconds1 = xtRec1.bday;
	
	seconds2 = xtRec2.bday;
	
	MemHandleUnlock(mH1);
	MemHandleUnlock(mH2);

	TimSecondsToDateTime(seconds1, &date1);
	TimSecondsToDateTime(seconds2, &date2);

	if (date1.day > date2.day)
		return 1;
	else
		return -1;				
}

void BInitForm()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 lRow;
	UInt16 addrXTDBSize;
	
	WinDrawLine(2, 58, 149, 58);	
	ImportBirthdays();
	DateToAscii(globals->gBListMonth, 1, globals->gBListYear, dfMYMed, (Char*)&(globals->gMonth));
	CustomSetCtlLabelPtrSmp(BirthdaysMonthSelector, globals->gMonth);
	
	CustomShowObjectSmp(BirthdaysMonthSelector);

	//init table
	globals->gBScrollPosition = 0;
	globals->gBTable = (TablePtr)CustomGetObjectPtrSmp(BirthdaysTable); 
	globals->gBTableRows=TblGetNumberOfRows(globals->gBTable);
	for(lRow=0;lRow<globals->gBTableRows;lRow++)
	{
		TblSetItemStyle(globals->gBTable, lRow, TBLB_MAINCOL, customTableItem);
		TblSetRowSelectable(globals->gBTable, lRow, true);
	}
	TblSetColumnUsable(globals->gBTable, TBLB_MAINCOL, true);
	TblSetCustomDrawProcedure(globals->gBTable, TBLB_MAINCOL, BDrawRecord);
	globals->gBRows = NumBirthdays(globals->gBListMonth, false, 0, 0, 0);
	
	
	CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &(globals->gAddrXTDB));
	if(globals->gAddrXTDB == 0)
		return;
	addrXTDBSize = DmNumRecords(globals->gAddrXTDB);
	if(addrXTDBSize == 0)
	{
		DmCloseDatabase(globals->gAddrXTDB);
		return;
	}
	//sort AddrXTDB
	DmInsertionSort(globals->gAddrXTDB, AddrXTCompare, 0);
	DmCloseDatabase(globals->gAddrXTDB);
	
	CustomShowObjectSmp(BirthdaysTable);
		
	InitBScrollBar();
}

static Boolean BirthdaysOnCtlSelectEvent(EventType* event)
{
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
		case BirthdaysMonthSelector:
			BirthdaysOnMonthSelector();
			handled = true;
			break;
		case BirthdaysOkButton:
			FrmReturnToForm(ListView);
			handled = true;
			break;
		default:
			handled = true;
			break;
	}
	return handled;
}

static Boolean BirthdaysOnKeyDownEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
	{
		if(globals->gDeviceFlags.bits.treoWithPhoneKeyOnly && event->data.keyDown.chr == vchrHard1)
		{
			PrefSetPreference(prefHard1CharAppCreator, globals->gDialerCreatorID);
			StartPhone();
		}
		return true;
	}
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			FrmReturnToForm(ListView);
			return true;
		}		   	 	
	}  
	return false;
}

static Boolean BirthdaysOnFrmOpenEvent()
{
	FormPtr frm = FrmGetFormPtr(BirthdaysDialog);
	//BDIAInit(frm);
	dia_save_state(); 
	dia_enable(frm, false);
	FrmDrawForm (frm);
	BInitForm();
#ifdef DEBUG
	LogWrite("xt_log", "b", "frmOpen");
#endif
	return true;
}

Boolean BHandleEvent (EventType * event)
{
	Boolean handled = false;
	
	switch(event->eType)
	{
		case tblSelectEvent:
			BirthdaysOnSelect(event);
			handled = true;
			break;
		case frmUpdateEvent:
			FrmDrawForm (FrmGetFormPtr(BirthdaysDialog));
			handled=true;
			break;
		case ctlSelectEvent:
			handled = BirthdaysOnCtlSelectEvent(event);
			break;
		case keyDownEvent:
			handled = BirthdaysOnKeyDownEvent(event);
			break;
		case frmOpenEvent:
			handled = BirthdaysOnFrmOpenEvent();
			break;
		case sclRepeatEvent:
			handled = OnScroll(event);
			break;
		case winEnterEvent:
			// works well for T3 device & tips window
			if(event->data.winEnter.exitWindow == 0)
				dia_restore_state();
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	return (handled);
}