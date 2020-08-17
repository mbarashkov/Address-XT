#include "AddrView2.h"

Boolean ViewOnMenuCmdBarOpenEvent(EventType* event)
{
	MenuCmdBarAddButton(menuCmdBarOnLeft, BarDeleteBitmap, menuCmdBarResultMenuItem, RecordRecordDeleteRecordCmd, 0);
	MenuCmdBarAddButton(menuCmdBarOnLeft, BarBeamBitmap, menuCmdBarResultMenuItem, RecordRecordBeamRecordCmd, 0);
	// tell the field package to not add cut/copy/paste buttons automatically
	event->data.menuCmdBarOpen.preventFieldButtons = true;
	// don't set handled to true; this event must fall through to the system.
	return false;
}

Boolean ViewOnMenuOpenEvent()
{
	UInt32 romVersion;
	UInt32 numLibs;
	Err err;
	globalVars* globals = getGlobalsPtr();
	if(!AddressDialable(globals->CurrentRecord, kDialListShowInListPhoneIndex) || !ToolsIsDialerPresent())
		MenuHideItem(RecordRecordDialCmd);
	else
		MenuShowItem(RecordRecordDialCmd);
	
	//if(LoadAddress(globals->CurrentRecord, NULL, true) == 0)
	//	MenuHideItem(RecordRecordMapCmd);
	//else if(MAPOPOLIS || globals->gTomTom)
	//	MenuShowItem(RecordRecordMapCmd);
				
	//if(!(MAPOPOLIS || globals->gTomTom))
		MenuHideItem(RecordRecordMapCmd);
	
	err = FtrGet(sysFtrCreator,	sysFtrNumROMVersion, &romVersion);

	if(romVersion>=0x04003000)
	{
		if (ExgGetRegisteredApplications(NULL, &numLibs, NULL, NULL, exgRegSchemeID, exgSendScheme) || !numLibs)
			MenuHideItem(RecordRecordSendRecordCmd);
		else
			MenuShowItem(RecordRecordSendRecordCmd);
	}
	else
			MenuHideItem(RecordRecordSendRecordCmd);
	return false;
}

Boolean PrvViewDoCommand (UInt16 command)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 newRecord;
	UInt16 numCharsToHilite;
	FormType *frmP;
	FontID oldFont;
	DmOpenRef addrXTDB;
	UInt16 answer;


	switch (command)
	{
	case RecordRecordDeleteRecordCmd:
		if (DetailsDeleteRecord ())
		{
			globals->recordViewRecordH = 0;      // freed by the last routine
			FrmGotoForm (ListView);
		}
		return true;
	
	
	case RecordRecordMapCmd:
		{
			FrmPopupForm(AddressDialog);
		}
		return true;

	case RecordRecordDuplicateAddressCmd:
		globals->gDuplicateCaller = RecordView;
		
		newRecord = ToolsDuplicateCurrentRecord (&numCharsToHilite, false);
		
		// If we have a new record take the user to be able to edit it
		// automatically.
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
	case RecordRecordDialCmd:
		MenuEraseStatus (0);
		DialListShowDialog(globals->CurrentRecord, kDialListShowInListPhoneIndex, 0);
		return true;

	case RecordRecordAttachNoteCmd:
		if (NoteViewCreate())
			FrmGotoForm (NewNoteView);
		// CreateNote may or may not have freed the record.  Compare
		// the record's handle to recordViewRecordH.  If they differ
		// the record is new and recordViewRecordH shouldn't be freed
		// by the frmClose.
		if (globals->recordViewRecordH != DmQueryRecord(globals->AddrDB, globals->CurrentRecord))
			globals->recordViewRecordH = 0;
		return true;

	case RecordRecordDeleteNoteCmd:
		if (globals->recordViewRecord.fields[note] != NULL ) // &&
			// FrmAlert(DeleteNoteAlert) == DeleteNoteYes)
		{
			FormType* frmP;

			answer = FrmAlert(DeleteNoteAlert);
			dia_restore_state();

		    if(answer == DeleteNoteYes)
		    {
		
			// Free recordViewRecordH because recordViewRecordH() calls AddrDBGetRecord()
			if (globals->recordViewRecordH)
			{
				MemHandleUnlock(globals->recordViewRecordH);
				globals->recordViewRecordH = 0;
			}

			NoteViewDelete ();
			// Deleting the note caused the record to be unlocked
			// Get it again for the record view's usage
			univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &(globals->recordViewRecord), &(globals->recordViewRecordH));
			
			// Initialize RecordViewLines, RecordViewLastLine... so that the Note won't be drawn
		  	frmP = FrmGetActiveForm();
		  	MemHandleFree(MemPtrRecoverHandle(globals->RecordViewLines));
		  	globals->RecordViewLines = 0;
	  		PrvViewInit(frmP);

			PrvViewUpdate(frmP);
		    }
		}
		return true;

	case RecordRecordSelectBusinessCardCmd:
		MenuEraseStatus (0);
		//if (FrmAlert(SelectBusinessCardAlert) == SelectBusinessCardYes)
		answer = FrmAlert(SelectBusinessCardAlert);
		dia_restore_state();
		if(answer == SelectBusinessCardYes)
		{
			DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &(globals->BusinessCardRecordID), NULL);
			PrvViewDrawBusinessCardIndicator (FrmGetActiveForm());
		}
		return true;

	case RecordRecordBeamBusinessCardCmd:
		MenuEraseStatus (0);
		ToolsAddrBeamBusinessCard(globals->AddrDB);
		dia_restore_state();
		return true;

	case RecordRecordBeamRecordCmd:
		MenuEraseStatus (0);
		TransferSendRecord(globals->AddrDB, globals->CurrentRecord, exgBeamPrefix, NoDataToBeamAlert);
		dia_restore_state();
		return true;

	case RecordRecordSendRecordCmd:
		MenuEraseStatus (0);
		TransferSendRecord(globals->AddrDB, globals->CurrentRecord, exgSendPrefix, NoDataToSendAlert);
		dia_restore_state();
		return true;

	case RecordOptionsFontCmd:
		MenuEraseStatus (0);
		oldFont = globals->AddrRecordFont;
		RestoreDialer();
		globals->AddrRecordFont = ToolsSelectFont (globals->AddrRecordFont);
		dia_restore_state();
		SetDialer();
		if (oldFont != globals->AddrRecordFont)
		{
			PrvViewClose();
			frmP = FrmGetFormPtr(RecordView);
			PrvViewInit(frmP);
		}
		return true;
	case RecordOptionsColors:
		globals->gColorFormID=RecordView;
		FrmPopupForm(ColorOptionsDialog);
		return true;
	case RecordOptionsGeneral:
		FrmPopupForm (PreferencesDialog);
		return true;
	case RecordOptionsConnect:
		FrmPopupForm(ConnectOptionsDialog);
		return true;
	case RecordOptionsEditCustomFldsCmd:
		MenuEraseStatus (0);
		FrmPopupForm (CustomEditDialog);
		return true;

	case RecordOptionsAboutCmd:
		FrmPopupForm(AboutForm);
		return true;
	case RecordHelpTips:
		RestoreDialer();
		FrmHelp(AddressViewHelp);
		dia_restore_state();
		SetDialer();
		return true;
	
	}

	return false;
}

void LinksDrawList(Int16 itemNum, RectangleType *tBounds, Char **itemsText)
{
#pragma unused(itemsText)
	globalVars* globals = getGlobalsPtr();
	UInt16 lAbsPos=itemNum;
	Int16 x, y;
	UInt32 id;
	UInt16 index;
	Char * memoP;
	MemHandle mH;
	Err err;
	LinkDBRecordType link_rec;
	ToDoDBRecordPtr rec;
	ApptDBRecordType apptRec;
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
	UInt16 attr;
	MemHandle resH;
	BitmapType *bitmap;
	UInt16 linkCount;
	DmOpenRef DateDB;
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
	linkCount = GetLinkCount(globals->adxtLibRef, globals->linksDB, id);
	
	if(lAbsPos >= linkCount+1)
		return;
	y = tBounds->topLeft.y;
	x = tBounds->topLeft.x;
	if(lAbsPos == linkCount)
	{
		WinDrawTruncChars("Edit links...", 13, tBounds->topLeft.x, tBounds->topLeft.y, tBounds->extent.x);
	}
	
	
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
					//else if (globals->gScreen==CLIE_320x320)
					//	resH = DmGetResource (bitmapRsc, BmpMemoSonyHr);
					else if(globals->gScreen==PALM_160x160)
						resH = DmGetResource (bitmapRsc, BmpMemoStd);
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
				
					
					mH = DmQueryRecord(globals->MemoDB, index);
					if(mH)
					{
						if(MemHandleSize(mH)>0)
						{
							memoP = MemHandleLock(mH);
							DrawMemoTitle(memoP, x + 20, y, tBounds->extent.x-20);
							MemHandleUnlock(mH);
						}					
					}
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
					if(mH)
					{
						if(MemHandleSize(mH)>0)
						{
							rec = (ToDoDBRecordPtr) MemHandleLock (mH);
							WinDrawTruncChars(&(rec->description), StrLen(&(rec->description)), x+20, y, tBounds->extent.x-20);
							MemHandleUnlock(mH);
						}
					}
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
					//else if (globals->gScreen==CLIE_320x320)
					//	resH = DmGetResource (bitmapRsc, BmpAddrSonyHr);
					else if(globals->gScreen==PALM_160x160)
						resH = DmGetResource (bitmapRsc, BmpAddrStd);
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
				
					x+=20;
					univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, index, &record, &recordH);
					if(recordH)
					{
						if(MemHandleSize(recordH)>0)
						{
							FntSetFont (stdFont);
							name1HasPriority = ToolsDetermineRecordName (globals->adxtLibRef, &record, &shortenedFieldWidth, 
							&fieldSeparatorWidth, globals->SortByCompany,
							 &name1, &name1Length, &name1Width, &name2, 
							 &name2Length, &name2Width, NULL, NULL, tBounds->extent.x-20, true);
							ToolsDrawRecordName(globals->adxtLibRef, name1, name1Length, name1Width, name2, name2Length, name2Width,
								   tBounds->extent.x-20, &x, y, shortenedFieldWidth, fieldSeparatorWidth, false,
								   name1HasPriority, false, true);
			
						}
						MemHandleUnlock (recordH);
					}
				}
				break;
			default:
				break;		
		}
	}	
}

void DisplayLinks()
{
	globalVars* globals = getGlobalsPtr();
	ListPtr linksList = GetObjectPtrSmp(RecordLinksList);
	UInt32 id;
	UInt16 linkCount;
	Int16 sel;
	DmRecordInfo(globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
	linkCount = GetLinkCount(globals->adxtLibRef, globals->linksDB, id);
	if(linkCount == 0)
		return;
	LstSetListChoices(linksList, NULL, linkCount+1);
	if(linkCount+1 < 6)
		LstSetHeight(linksList, linkCount+1);
	else
		LstSetHeight(linksList, 6);
	LstSetDrawFunction(linksList, LinksDrawList);
	sel = LstPopupList(linksList);
	if(sel == linkCount)
	{
		FrmPopupForm(LinksDialog);
	}	
	else if(sel != -1)
	{
		if(GoToLink(globals->adxtLibRef, globals->linksDB, id, sel+1) == ERROR_GOTOLINK_PRIVATE)
			FrmAlert(alertGoToPrivateLink);
	}
}