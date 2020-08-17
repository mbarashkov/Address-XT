#include <PalmOS.h>
#include "AddrTools.h"
#include "AddrTools2.h"
#include "Address.h"
#include "LinksDialog.h"
#include "Links.h"
#include "Plugins/ToDo.h"
#include "Plugins/ToDoDB.h"
#include "Plugins/DateDay.h"
#include "Plugins/MemoMain.h"
#include "Plugins/MemoDB.h"
#include "Plugins/DateDB.h"
#include "Plugins/ContactSelList.h"
#include "dia.h"

#define TBLL_MAINCOL 0

static void InitForm();
static void InitScrollBar();
static void DrawRecord(MemPtr tTable, Int16 tRow, Int16 tColumn, RectangleType *tBounds);
static void OnSelect(EventType *event);
static Boolean OnScroll(EventPtr event);



static void InitLScrollBar()
{
	globalVars* globals = getGlobalsPtr();
	Int16 cnt;
	for(cnt=0;cnt<globals->gLTableRows;cnt++)
	{
		TblSetRowSelectable(globals->gLTable, cnt, true);
	}
	if(globals->gLTableRows>globals->gLRows)
	{
		CustomHideObjectSmp(LinksScrollbar);
		for(cnt=globals->gLRows;cnt<globals->gLTableRows;cnt++)
		{
			TblSetRowSelectable(globals->gLTable, cnt, false);
		}
		return;
	}
	else
		CustomShowObjectSmp(LinksScrollbar);
	SclSetScrollBar((ScrollBarPtr)CustomGetObjectPtrSmp(LinksScrollbar), globals->gLScrollPosition, 0, globals->gLRows-globals->gLTableRows, globals->gLRows);
}

static Boolean OnScroll(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	globals->gLScrollPosition=event->data.sclRepeat.newValue;
	TblDrawTable(globals->gLTable);
	return false;
}


void OnSelect(EventType *event)
{
	globalVars* globals = getGlobalsPtr();
	int row = event->data.tblSelect.row + globals->ScrollPosition;
	if(row >= globals->gLRows)
		return;
}
		

void InitScrollBar()
{
	globalVars* globals = getGlobalsPtr();
	Int16 cnt;
	for(cnt=0;cnt<globals->TableRows;cnt++)
	{
		TblSetRowSelectable(globals->Table, cnt, true);
	}
	if(globals->TableRows>globals->Rows)
	{
		CustomHideObjectSmp(LinksScrollbar);
		for(cnt=globals->Rows;cnt<globals->TableRows;cnt++)
		{
			TblSetRowSelectable(globals->Table, cnt, false);
		}
		return;
	}
	else
		CustomShowObjectSmp(LinksScrollbar);
	SclSetScrollBar((ScrollBarPtr)CustomGetObjectPtrSmp(LinksScrollbar), globals->ScrollPosition, 0, globals->Rows-globals->TableRows, globals->Rows);
}

static void LDrawRecord(MemPtr tTable, Int16 tRow, Int16 tColumn, RectangleType *tBounds)
{
#pragma unused(tTable, tColumn)
	globalVars* globals = getGlobalsPtr();
	UInt16 lAbsPos=tRow+globals->gLScrollPosition;
	Int16 x, y;
	Boolean addrListHighRes;
	UInt32 id;
	UInt16 index;
	Char * memoP;
	MemHandle mH;
	Err err;
	LinkDBRecordType link_rec;
	ToDoDBRecordPtr rec;
	Int16 shortenedFieldWidth;
	Int16 fieldSeparatorWidth;
	Char * name1;
	Char * name2;
	Int16 name1Length;
	Int16 name2Length;
	Int16 name1Width;
	Int16 name2Width;
	Boolean name1HasPriority;
	MemHandle            recordH;
	univAddrDBRecordType     record;
	ApptDBRecordType 	apptRec;
	UInt16 attr;
	DmOpenRef DateDB;
	MemHandle resH;
	BitmapType *bitmap;
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
	
	if(lAbsPos >= globals->gLRows)
		return;
	y = tBounds->topLeft.y;
	x = tBounds->topLeft.x;
	FntSetFont(stdFont);
	if(GetLinkByIndex(globals->adxtLibRef, globals->linksDB, id, lAbsPos+1, &link_rec) == 0)
	{
		switch(link_rec.type)
		{
			case link_datebook:
				err = DateGetDatabase (&DateDB, dmModeReadOnly);
				if (err)
					return;
				
				if(DmFindRecordByID(DateDB, link_rec.linkID, &index)!=0)
					return;
				
				
				
				DmRecordInfo(DateDB, index, &attr, NULL, NULL);
				if (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus != showPrivateRecords))
				{
					MemoListViewDisplayMask (tBounds);		
				}
				else
				{
					if(globals->gScreen==PALM_320x320)
						resH = DmGetResource (bitmapRsc, BmpDateHr);
					else if(globals->gScreen==PALM_160x160)
						resH = DmGetResource (bitmapRsc, BmpDateStd);
					bitmap = MemHandleLock (resH);	
					if(bitmap != NULL)
					{
						WinDrawBitmap(bitmap,  tBounds->topLeft.x, tBounds->topLeft.y+1);
					}	
					MemHandleUnlock(resH);
					DmReleaseResource(resH);
				
					ApptGetRecord (DateDB, index, &apptRec, &mH);
					if(apptRec.description)
					{
						if(StrLen(apptRec.description)>0)
							WinDrawTruncChars(apptRec.description, StrLen(apptRec.description), x+20, y, tBounds->extent.x-20);
					}
					MemHandleUnlock(mH);
					DmCloseDatabase(DateDB);
				}
				break;
			case link_memo:
				err = MemoGetDatabase (&globals->MemoDB, dmModeReadOnly);
				if (err)
					return;
				
				if(DmFindRecordByID(globals->MemoDB, link_rec.linkID, &index)!=0)
					return;
				
				
				
				DmRecordInfo(globals->MemoDB, index, &attr, NULL, NULL);
				if (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus != showPrivateRecords))
				{
					MemoListViewDisplayMask (tBounds);		
				}
				else
				{
					if(globals->gScreen==PALM_320x320)
						resH = DmGetResource (bitmapRsc, BmpMemoHr);
					else if(globals->gScreen==PALM_160x160)
						resH = DmGetResource (bitmapRsc, BmpMemoStd);
					bitmap = MemHandleLock (resH);	
					if(bitmap != NULL)
					{
						WinDrawBitmap(bitmap,  tBounds->topLeft.x, tBounds->topLeft.y+1);
					}	
					MemHandleUnlock(resH);
					DmReleaseResource(resH);
				
					
					mH = DmQueryRecord(globals->MemoDB, index);
					memoP = MemHandleLock(mH);
					DrawMemoTitle(memoP, x + 20, y, tBounds->extent.x-20);
					MemHandleUnlock(mH);
					DmCloseDatabase(globals->MemoDB);
				}
				break;
			case link_todo:
				err = ToDoGetDatabase (&globals->ToDoDB, dmModeReadOnly);
				if (err)
					return;
				if(DmFindRecordByID(globals->ToDoDB, link_rec.linkID, &index)!=0)
					return;
				
				DmRecordInfo(globals->ToDoDB, index, &attr, NULL, NULL);
				if (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus != showPrivateRecords))
				{
					MemoListViewDisplayMask (tBounds);		
				}
				
				else
				{
					if(globals->gScreen==PALM_320x320)
						resH = DmGetResource (bitmapRsc, BmpTodoHr);
					//else if (globals->gScreen==CLIE_320x320)
					//	resH = DmGetResource (bitmapRsc, BmpTodoSonyHr);
					else if(globals->gScreen==PALM_160x160)
						resH = DmGetResource (bitmapRsc, BmpTodoStd);
					bitmap = MemHandleLock (resH);	
					if(bitmap != NULL)
					{
						//if(globals->gScreen==CLIE_320x320)
						//	HRWinDrawBitmap(globals->refNum, bitmap,  tBounds->topLeft.x<<1, (tBounds->topLeft.y+1)<<1);
						//else
							WinDrawBitmap(bitmap,  tBounds->topLeft.x, tBounds->topLeft.y+1);
					}	
					MemHandleUnlock(resH);
					DmReleaseResource(resH);
				
					ToDoListGetLabel(link_rec.linkID, &mH);
					rec = (ToDoDBRecordPtr) MemHandleLock (mH);
								
					WinDrawTruncChars(&(rec->description), StrLen(&(rec->description)), x+20, y, tBounds->extent.x-20);
					MemHandleUnlock(mH);
				}
				DmCloseDatabase(globals->ToDoDB);
				break;
			case link_address:
				FntSetFont(stdFont);
				x = tBounds->topLeft.x;
			
				if(DmFindRecordByID(globals->AddrDB,  link_rec.linkID, &index) != 0)
					return;
				
				
				DmRecordInfo(globals->AddrDB, index, &attr, NULL, NULL);
				if (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus != showPrivateRecords))
				{
					MemoListViewDisplayMask (tBounds);		
				}
				
				else
				{
					if(globals->gScreen==PALM_320x320)
						resH = DmGetResource (bitmapRsc, BmpAddrHr);
					else if(globals->gScreen==PALM_160x160)
						resH = DmGetResource (bitmapRsc, BmpAddrStd);
					bitmap = MemHandleLock (resH);	
					if(bitmap != NULL)
					{
						WinDrawBitmap(bitmap,  tBounds->topLeft.x, tBounds->topLeft.y+1);
					}	
					MemHandleUnlock(resH);
					DmReleaseResource(resH);
				
					x+=20;
					univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, index, &record, &recordH);
					FntSetFont (stdFont);
					addrListHighRes = globals->AddrListHighRes;
					globals->AddrListHighRes = false;
					name1HasPriority = ToolsDetermineRecordName (globals->adxtLibRef, &record, &shortenedFieldWidth, 
					&fieldSeparatorWidth, globals->SortByCompany,
					 &name1, &name1Length, &name1Width, &name2, 
					 &name2Length, &name2Width, NULL, NULL, tBounds->extent.x-20, false);
					ToolsDrawRecordName(globals->adxtLibRef, name1, name1Length, name1Width, name2, name2Length, name2Width,
						   tBounds->extent.x-20, &x, y, shortenedFieldWidth, fieldSeparatorWidth, false,
						   name1HasPriority, false, true);
					MemHandleUnlock (recordH);
					globals->AddrListHighRes = addrListHighRes;					
				}
				break;
			default:
				break;		
		}
	}	
}

static void RefreshTable(Boolean redraw)
{
	globalVars* globals = getGlobalsPtr();
	UInt32 id;
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
	globals->gLRows = GetLinkCount(globals->adxtLibRef, globals->linksDB, id);
	InitLScrollBar();
	if(redraw)
		TblDrawTable(globals->gLTable);
}

void InitForm()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 lRow;
	WinDrawLine(2, 29, 149, 29);
	
	//init table
	globals->gLScrollPosition = 0;
	globals->gLTable = (TablePtr)CustomGetObjectPtrSmp(LinksTable); 
	globals->gLTableRows=TblGetNumberOfRows(globals->gLTable);
	for(lRow=0;lRow<globals->gLTableRows;lRow++)
	{
		TblSetItemStyle(globals->gLTable, lRow, TBLL_MAINCOL, customTableItem);
		TblSetRowSelectable(globals->gLTable, lRow, true);
	}
	TblSetColumnUsable(globals->gLTable, TBLL_MAINCOL, true);
	TblSetCustomDrawProcedure(globals->gLTable, TBLL_MAINCOL, LDrawRecord);
	
	
	RefreshTable(false);
	CustomShowObjectSmp(LinksTable);
	SetDialer();			
}

static LinkType getLinkType(int linkType)
{
	switch(linkType)
	{
		case LINKTYPE_CONTACT:
			return link_address;
		case LINKTYPE_MEMO:
			return link_memo;
		case LINKTYPE_DATE:
			return link_datebook;
		case LINKTYPE_TODO:
			return link_todo;
		default:
			return link_none;	
	}
	return link_none;
}

static void AddCurrentLink()
{
	globalVars* globals = getGlobalsPtr();
	LinkType type = getLinkType(globals->gLinkType);
	UInt32 id;
	UInt16 err;
	if(type == link_none)
		return;
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
	err = AddLink(globals->adxtLibRef, globals->linksDB, id, globals->gLinkID, type);
	if(err == LINK_ALREADYEXISTS)
		FrmAlert(alertLinkExists);
	else if(err == LINK_ITSELF)
		FrmAlert(alertLinkItself);
	
	globals->gLinkID = 0;
	
	RefreshTable(true);
}

static void DeleteSelLink()
{
	globalVars* globals = getGlobalsPtr();
	Int16 row, column;
	UInt16 lAbsPos;
	UInt32 id;
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
	
	
	if(globals->gLRows == 0)
		return;
	if(TblGetSelection(globals->gLTable, &row, &column) == false)
		return;
	lAbsPos =row + globals->gLScrollPosition + 1;
	if(FrmAlert(alertDeleteLink) == 0)
	{
		LinkDBRecordType link_rec;
		if(GetLinkByIndex(globals->adxtLibRef, globals->linksDB, id, lAbsPos, &link_rec) == 0)
		{
			DeleteLink(globals->adxtLibRef, globals->linksDB, id, link_rec.linkID, link_rec.type);
		}
	}
	RefreshTable(true);
}

static void GoToSelLink()
{
	globalVars* globals = getGlobalsPtr();
	Int16 row, column;
	UInt16 lAbsPos;
	UInt32 id;
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
	
	
	if(globals->gLRows == 0)
		return;
	if(TblGetSelection(globals->gLTable, &row, &column) == false)
		return;
	lAbsPos =row + globals->gLScrollPosition + 1;
	if(GoToLink(globals->adxtLibRef, globals->linksDB, id, lAbsPos) == ERROR_GOTOLINK_PRIVATE)
		FrmAlert(alertGoToPrivateLink);
}

static Boolean LOnKeyDownEvent(EventType* event)
{
	Boolean handled = false;
	globalVars* globals = getGlobalsPtr();
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			FrmUpdateForm (ListView, updatePrefs);
			FrmUpdateForm (RecordView, updatePrefs);
			FrmReturnToForm(0);
			return true;
		}		   	 	
	}  
	return handled;
}

static Boolean LOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
		case LinksDone:
			SetDialer();
			FrmUpdateForm (ListView, updatePrefs);
			FrmUpdateForm (RecordView, updatePrefs);
			FrmReturnToForm(0);
			handled = true;
			break;
		case LinksNew:
			FrmPopupForm(NewLinkDialog);			
			handled = true;
			break;
		case LinksGoTo:
			GoToSelLink();
			handled = true;
			break;
		case LinksDelete:
			DeleteSelLink();
			handled = true;
			break;
		default:
			break;
	}
	return handled;
}

static Boolean LOnFrmOpenEvent()
{
	FormPtr frm = FrmGetFormPtr(LinksDialog);
	dia_save_state(); 
	dia_enable(frm, false);
	FrmDrawForm (frm);
	InitForm();
	return true;
}

static Boolean LOnWinEnterEvent()
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gLinkID!=0)
	{
		AddCurrentLink();
		return true;
	}
	else
	{
		return false;
	}
}

Boolean LHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch(event->eType)
	{
		case tblSelectEvent:
			OnSelect(event);
			handled = true;
			break;
		case frmUpdateEvent:
			FrmDrawForm (FrmGetFormPtr(LinksDialog));
			handled=true;
			break;
		case keyDownEvent:
			handled = LOnKeyDownEvent(event);
			break;
		case ctlSelectEvent:
			handled = LOnCtlSelectEvent(event);
			break;
		case frmOpenEvent:
			handled = LOnFrmOpenEvent();
			break;
		case winEnterEvent:	
			handled = LOnWinEnterEvent();
			break;
		case sclRepeatEvent:
			handled = OnScroll(event);
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	return (handled);
}
