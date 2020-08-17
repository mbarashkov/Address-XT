#include "CompanyData.h"
#include "Address.h"
#include "AddressDB2.h"
#include "globals.h"
#include "AddrPrefs.h"
#include "AddrTools2.h"
#include "dia.h"
#include "ContactsDB2.h"

static Boolean CompanyDataMoveObjects(FormType* frmP, Coord dx, Coord dy);
Boolean PrvCompanyDoCommand (UInt16 command);
static Boolean CompanyOnFrmUpdateEvent();

Boolean PrvCompanyDoCommand (UInt16 command)
{
	switch (command)
	{
		case ListEditGraffitiLegacyHelpCmd:
		case ListEditGraffitiHelpCmd:
			SysGraffitiReferenceDialog(referenceDefault);
			return true;
		case CompanyDataHelpTips:
			FrmHelp(CompanyDataHelp);
			return true;
		case CompanyDataPreferences:
			FrmPopupForm(CompanyDataPrefDialog);
			return true;
		default:
			break;
	}
	return false;
}

static Boolean CompanyDataMoveObjects(FormType* frmP, Coord dx, Coord dy)
{
 	globalVars* globals = getGlobalsPtr();
	RectangleType bounds;
 	UInt16 controlID[10];
	UInt16 counter;
	Boolean resized = false;
	
 	controlID[0]=3009;
	controlID[1]=3013;
	controlID[2]=3015;
	controlID[3]=3017;
	controlID[4]=3019;
	controlID[5]=3011;
	controlID[6]=3023;
	controlID[7]=3005;
	controlID[8]=3006;
	controlID[9]=3024;
	
	if (dx != 0 || dy != 0)
    {
  		MoveFormObjectHide(frmP, CompanyDataCancelButton, 0, dy);
        MoveFormObjectHide(frmP, CompanyDataOkButton, 0, dy);
  		
		if(GetWindowHeight()>=220)
		{
           	LstSetHeight(CustomGetObjectPtrSmp(CompanyList), 17>globals->gCompanyList.Size?globals->gCompanyList.Size:17);
        	globals->ListSizeSet=true;
        }
        if(GetWindowHeight()<=160)
        {
      		LstSetHeight(CustomGetObjectPtrSmp(CompanyList), 12>globals->gCompanyList.Size?globals->gCompanyList.Size:12);
        	globals->ListSizeSet=true;
        
        }
        if(GetWindowWidth()>=220)
        {
        	
        	for(counter=0;counter<8;counter++)
        	{
        		FrmHideObjectSmp(controlID[counter]);
        		FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[counter]), &bounds);
				bounds.extent.x+=dx;
				FrmSetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[counter]), &bounds);
				FrmShowObjectSmp(controlID[counter]);
				FldRecalculateField (GetObjectPtrSmp(controlID[counter]), true);
				
        	}
        		FrmHideObjectSmp(controlID[8]);
        		FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[8]), &bounds);
				bounds.topLeft.x+=dx;
				FrmSetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[8]), &bounds);
				FrmShowObjectSmp(controlID[8]);
				
				FrmHideObjectSmp(controlID[9]);
        		FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[9]), &bounds);
				bounds.extent.x+=dx;
				FrmSetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[9]), &bounds);
		 	
        	
        }
        else if(dx<0 && (GetWindowWidth()<=160))
        {
        	for(counter=0;counter<8;counter++)
        	{
        		FrmHideObjectSmp(controlID[counter]);
        		FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[counter]), &bounds);
				bounds.extent.x+=dx;
				FrmSetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[counter]), &bounds);
				FrmShowObjectSmp(controlID[counter]);
				FldRecalculateField (GetObjectPtrSmp(controlID[counter]), true);
				
				
        	}
        		FrmHideObjectSmp(controlID[8]);
        		FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[8]), &bounds);
				bounds.topLeft.x+=dx;
				FrmSetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[8]), &bounds);
				FrmShowObjectSmp(controlID[8]);
				
				FrmHideObjectSmp(controlID[9]);
        		FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[9]), &bounds);
				bounds.extent.x+=dx;
				FrmSetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), controlID[9]), &bounds);
		  	
        	
        }
		MoveFormGSI(frmP, dx, dy);
       	resized = true;   
    }
 	return resized;
}

void CompanyScroll(Int16 value)
{
	globalVars* globals = getGlobalsPtr();
	switch(value)
	{
		case -1:
			if(globals->ScrollPosition==0)
				return;
			globals->ScrollPosition--;
			break;
		case 1:
			if(globals->ScrollPosition==globals->MaxScrollPosition)
				return;
			globals->ScrollPosition++;
			break;
		default:
			break;	
	}
}


void CompanySetScrollers(Boolean up, Boolean down)
{
	UInt16 upIndex;
	UInt16 downIndex;
	upIndex = FrmGetObjectIndex (FrmGetActiveForm(), CompanyScrollUp);
	downIndex = FrmGetObjectIndex (FrmGetActiveForm(), CompanyScrollDown);
	
	FrmUpdateScrollers(FrmGetActiveForm(), upIndex, downIndex, up, down);
}

Boolean CompanyListEntryFound(const Char* tEntry)
{
	globalVars* globals = getGlobalsPtr();
	Int16 lIndex;
	if(globals->gCompanyList.Size==0) //list is still empty
		return false;
	for(lIndex=0;lIndex<=globals->gCompanyList.Size-1;lIndex++)
		if(!StrCompare(globals->gCompanyList.Pointer[lIndex], tEntry))
			return true;
	return false;	
}

void CompanyListAddEntry(const Char* tEntry)
{
	globalVars* globals = getGlobalsPtr();
	if(tEntry[0]==0)
		return;
	if(StrLen(tEntry)==0)
		return;
	if(!CompanyListEntryFound(tEntry))
		{
		//Add new entry
		if(globals->gCompanyList.Size==MAX_COMPANIES)
			return;
		globals->gCompanyList.Pointer[globals->gCompanyList.Size]=MemPtrNew(StrLen(tEntry)+1);
		StrCopy(globals->gCompanyList.Pointer[globals->gCompanyList.Size], tEntry);
		globals->gCompanyList.Size++;
		}
	return;
}

Int16 CompanyCompare(void *ptr1, void *ptr2, Int32 other)
{
#pragma unused(other)
	return StrCompare(*(Char**)ptr1, *(Char**)ptr2);
}

void CompanyFillList()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 lRecordIndex=0;
	MemHandle recordH;
	univAddrDBRecordType record;
	MemHandle lCurrentRecordMemHandle;
	UInt16 lSize;
	UInt16 attr;
	ListType *lCompanyListPtr=(ListType*)GetObjectPtrSmp(CompanyList); 
	//First, init list
	//now, fill the list with values
	globals->gCompanyList.Size=0;
	while(true)
	{
		lCurrentRecordMemHandle=DmQueryNextInCategory(globals->AddrDB, &lRecordIndex, dmAllCategories);
	 	if (!lCurrentRecordMemHandle)
      	{
			//We've reached end of database
        	break;
      	}
      	DmRecordInfo (globals->AddrDB, lRecordIndex, &attr, NULL, NULL);
		if((attr & dmRecAttrSecret)  && globals->PrivateRecordVisualStatus != showPrivateRecords)
		{
			lRecordIndex++;
			continue;			
		}
		univAddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, lRecordIndex, &record, &recordH);
		if(record.fields[univCompany] != NULL)
		{
			CompanyListAddEntry(record.fields[univCompany]);
		}		
		//Unlock which is supposed to be done by AddrDBChangeRecord
		MemHandleUnlock(recordH);
		lRecordIndex++;
	}
	//sort gCompanyList
	SysQSort(globals->gCompanyList.Pointer, globals->gCompanyList.Size, sizeof(Char**), &CompanyCompare, 0);
	//fill the list UI control with values
	lSize=globals->gCompanyList.Size;
	if(lSize>12) lSize=12;
	if(!globals->ListSizeSet)
		LstSetHeight(lCompanyListPtr, lSize);
	LstSetListChoices(lCompanyListPtr, globals->gCompanyList.Pointer, globals->gCompanyList.Size);
}


/***********************************************************************
 *
 * FUNCTION:    CompanyInitForm
 *
 * DESCRIPTION: Init Company Data form
 *
 * PARAMETERS:  
 *
 * RETURNED:    
 * 
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         MB     18/02/04  Initial Revision
 *
 ***********************************************************************/
void CompanyInitForm()
{
	globalVars* globals = getGlobalsPtr();
	globals->ScrollPosition=0;
	globals->MaxScrollPosition=1;
	FrmSetFocus(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), CompanyCompanyNameField));	
}

Boolean CompanyChangeEMailDomain(const char* lSrc, const char* lDomain, char* lDst)
{
	int lPos;
	if(lSrc==0)
		return false;
	if(StrLen(lSrc)==0)
		return false; //Empty e-mail
	if(StrNumOfChrAdv(lSrc, '@', false)!=1) //Incorrect e-mail - number of '@' other than 1
		return false;
	lPos=StrChr(lSrc, '@')-lSrc; //lPos is '@' position
	StrNCopy(lDst, lSrc, lPos+1);
	lDst[lPos+1]=0;
	//Now we have e-mail address truncated to username on domain	
	StrCat(lDst, lDomain);
	//Now we added new domain
	return true;
}

void CompanyFillFields()
{
	globalVars* globals = getGlobalsPtr();
	MemHandle lH, recordH;
	univAddrDBRecordType record;
	AddrDBRecordFlags recFlags;
	P1ContactsDBRecordFlags contactFlags;
	Char str[15];
	UInt16 fieldIndex;
	Char **records;
	Boolean EmptyOnly;
	UInt16 attr;
	UInt16 recIndex=0, recSavedIndex, modRecords=0, cnt;
	
	Int16 prefsVersion;
	UInt16 prefsSize;
	AddrCDataPreferenceType prefs;
	
	EmptyOnly = false;
	prefsSize = 0;
	PrefGetAppPreferences (CREATORID, addrPrefCDataID, NULL, &prefsSize, true);
	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefCDataID, &prefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	}
	if (prefsVersion != noPreferenceFound)
	{
		EmptyOnly=prefs.EmptyOnly;
	}	
	
	
		
	recFlags.allBits=1;
	contactFlags.allBits = -1;
	contactFlags.allBits2 = -1;
	
	if(FldEmptySmp(OldCompanyField))
	{
		FrmCustomAlert(FieldnotenteredAlert, "Company name", NULL, NULL);
		return;
	}
	if((FldEmptySmp(CompanyCompanyNameField) && FldEmptySmp(CompanyAddressField) && FldEmptySmp(CompanyCityField) && FldEmptySmp(CompanyZipCodeField) && FldEmptySmp(CompanyStateField) && FldEmptySmp(CompanyCountryField) && FldEmptySmp(CompanyEmaildomainField)))
	{
		FrmAlert(NothingtodoAlert);
		return;	
	}
	
	
	records=MemPtrNew((univLastPhoneField-univFirstPhoneField+1)*sizeof(Char**));
	for (fieldIndex = univFirstPhoneField; fieldIndex <= univLastPhoneField; fieldIndex++)
	{
		*(records+fieldIndex-univFirstPhoneField)=MemPtrNew(255*sizeof(Char));
	}
	
	if(FrmAlert(ConfirmAlert)==1)
	{
		return;
	}
	StrFldCopy(globals->gCompany, OldCompanyField);
	StrFldCopy(globals->gNewCompany, CompanyCompanyNameField);
	StrFldCopy(globals->gAddress, CompanyAddressField);
	StrFldCopy(globals->gCity, CompanyCityField);
	StrFldCopy(globals->gZipCode, CompanyZipCodeField);
	StrFldCopy(globals->gState, CompanyStateField);
	StrFldCopy(globals->gCountry, CompanyCountryField);
	StrFldCopy(globals->gDomain, CompanyEmaildomainField);
	while(true)
	{
		lH=DmQueryNextInCategory(globals->AddrDB, &recIndex, dmAllCategories);
		if(!lH)
		{
			break;
		}
		DmRecordInfo (globals->AddrDB, recIndex, &attr, NULL, NULL);
		if((attr & dmRecAttrSecret)  && globals->PrivateRecordVisualStatus != showPrivateRecords)
		{
			recIndex++;
			continue;			
		}
		univAddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, recIndex, &record, &recordH);
		if(record.fields[univCompany])
		{
			if(!StrCompare(record.fields[univCompany], globals->gCompany))
			{
				modRecords++;
				if(!FldEmptySmp(CompanyCompanyNameField) && StrCompare(globals->gCompany, globals->gNewCompany))
				{
					record.fields[univCompany]=globals->gNewCompany;
				}
				if(!FldEmptySmp(CompanyAddressField))
				{
					if(!EmptyOnly)
					{
							record.fields[univAddress]=globals->gAddress;
					}
					else if(record.fields[univAddress] == 0)
					{
							record.fields[univAddress]=globals->gAddress;
					}
				}
				if(!FldEmptySmp(CompanyCityField))
				{
					if(!EmptyOnly)
					{
							record.fields[univCity]=globals->gCity;
					}
					else if(record.fields[univCity] == 0)
					{
							record.fields[univCity]=globals->gCity;
					}
				}
				if(!FldEmptySmp(CompanyZipCodeField))
				{
					if(!EmptyOnly)
					{
							record.fields[univZipCode]=globals->gZipCode;
					}
					else if(record.fields[univZipCode] == 0)
					{
							record.fields[univZipCode]=globals->gZipCode;
					}
				}
				if(!FldEmptySmp(CompanyStateField))
				{
					if(!EmptyOnly)
					{
						record.fields[univState]=globals->gState;
					}
					else if(record.fields[univState] == 0)
					{
						record.fields[univState]=globals->gState;
					}
				}
				if(!FldEmptySmp(CompanyCountryField))
				{
					if(!EmptyOnly)
					{
						record.fields[univCountry]=globals->gCountry;
					}
					else if(record.fields[univCountry] == 0)
					{
						record.fields[univCountry]=globals->gCountry;
					}
				}
				if(!FldEmptySmp(CompanyEmaildomainField))
				{
					for (fieldIndex = univFirstPhoneField; fieldIndex <= univLastPhoneField; fieldIndex++)
					{
						if (univGetPhoneLabel(&record, fieldIndex)==INDEX_EMAIL)
						{
							if(CompanyChangeEMailDomain(record.fields[fieldIndex], globals->gDomain, globals->gNewEMail))
							{
								
								StrCopy(records[fieldIndex-univFirstPhoneField], globals->gNewEMail);
								record.fields[fieldIndex]=records[fieldIndex-univFirstPhoneField];
							}
						}
					}				
				}
				recSavedIndex=recIndex;
				MemHandleUnlock(recordH);
#ifdef CONTACTS
				PrvP1ContactsDBChangeRecordCustom(globals->AddrDB,&recIndex, &record, contactFlags);
#else
				AddrDBChangeRecordCustom(globals->AddrDB,&recIndex, &record, recFlags);
#endif				
				recIndex=recSavedIndex;
				if(!FldEmptySmp(CompanyCompanyNameField))
				{
					if(StrCompare(globals->gNewCompany, globals->gCompany))
					{
						recIndex=0;
						continue;
					}
				}
			}
			else
			{
				MemHandleUnlock(recordH);
			}	
				
		}
		else
		{
		
			MemHandleUnlock(recordH);
		}
		recIndex++;
	}
	if(!FldEmptySmp(CompanyCompanyNameField))
	{
		for(cnt=0; cnt<globals->gCompanyList.Size; cnt++)
		{
			MemPtrFree(globals->gCompanyList.Pointer[cnt]);
		}
		globals->gCompanyList.Size=0;
		CompanyFillList();			
	}
	for (fieldIndex = firstPhoneField; fieldIndex <= lastPhoneField; fieldIndex++)
	{
		MemPtrFree(*(records+fieldIndex-firstPhoneField));
	}
	MemPtrFree(records);
	StrIToA(str, modRecords);
	FrmCustomAlert(CompleteAlert, str, 0, 0);
	FrmDrawForm(FrmGetActiveForm());
}

void CompanyOnTrigger()
{
	globalVars* globals = getGlobalsPtr();
	Int16 lIndex;
	ListType *lCompanyListPtr=GetObjectPtrSmp(CompanyList);
	lIndex=LstPopupList(lCompanyListPtr);
	if(lIndex==-1)
		return;
	StrCopy(globals->gCompany, LstGetSelectionText(lCompanyListPtr, lIndex));
	FldSetTextPtrSmp(OldCompanyField, globals->gCompany);
	return;
}

static Boolean CompanyOnFrmUpdateEvent()
{
 	FormPtr frm = FrmGetActiveForm ();
	FrmDrawForm (frm);
	return true;
}

static Boolean CompanyOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
	case CompanyDataOkButton:
		SetDialer();
		CompanyFillFields();			
		handled = true;
		break;
	case CompanyDataCancelButton:
		SetDialer();
		FrmGotoForm (ListView);
		handled = true;
		break;
	case SelectorTrigger:
		CompanyOnTrigger();
		handled=true;
		break;
	default:
		break;
	}
	return handled;
}

static Boolean CompanyOnFldEnterEvent(EventType* event)
{
	Boolean handled = false;;
	switch (event->data.ctlSelect.controlID)
	{
	case OldCompanyField:
		CompanyOnTrigger();
		handled=true;
		break;
	default:
		break;
	}	
	return handled;
}

static Boolean CompanyOnCtlRepeatEvent(EventType* event)
{
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
	case CompanyScrollUp:
		CompanyScroll(-1);
		handled = false;
		break;
	case CompanyScrollDown:
		CompanyScroll(1);
		handled = false;
		break;
	default:
		break;
	}
	return handled;
}

static Boolean CompanyOnKeyDownEvent(EventType* event)
{
	Boolean handled = false;
	globalVars* globals = getGlobalsPtr();
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			CompanyFillFields();			
			handled=true;
		}		   	 	
	}  
	return handled;
}

static Boolean CompanyOnMenuEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gDeviceFlags.bits.treo)
	{
		switch(event->data.menu.itemID)
		{
			case 10200:
				event->data.menu.itemID = 10000;
				EvtAddEventToQueue(event); 
				break;
			case 10201:
				event->data.menu.itemID = 10001;
				EvtAddEventToQueue(event); 
				break;
			case 10202:
				event->data.menu.itemID = 10002;
				EvtAddEventToQueue(event); 
				break;
			case 10203:
				event->data.menu.itemID = 10003;
				EvtAddEventToQueue(event); 
				break;
			case 10204:
				event->data.menu.itemID = 10004;
				EvtAddEventToQueue(event); 
				break;
			case 10206:
				event->data.menu.itemID = 10006;
				EvtAddEventToQueue(event); 
				break;
			case 10207:
				event->data.menu.itemID = 10007;
				EvtAddEventToQueue(event); 
				break;	
		}
	}
	return PrvCompanyDoCommand (event->data.menu.itemID);
}

static Boolean CompanyOnFrmOpenEvent()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm;
	if(globals->gDeviceFlags.bits.treo)
	{
		FrmSetMenu(FrmGetActiveForm(), CompanyDataMenuBarTreo);
	}
	SetDialer();
	globals->ListSizeSet=false;
	frm = FrmGetActiveForm ();
	dia_save_state();
	dia_enable(frm, true);
	dia_resize(frm, CompanyDataMoveObjects);
	globals->skipWinEnter = dia_skip_first();
	
	globals->ListSizeSet = false;	
	CompanyFillList();
	FrmDrawForm (frm);
	CompanyInitForm();
	return true;
}

static Boolean CompanyOnMenuOpenEvent()
{
	globalVars* globals = getGlobalsPtr();
	UInt32 g2DynamicID;
	
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

Boolean CompanyHandleEvent (EventType * event)
{
	Boolean handled = false;
	
	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = CompanyOnCtlSelectEvent(event);
			break;
		case fldEnterEvent:
			handled = CompanyOnFldEnterEvent(event);
			break;
		case ctlRepeatEvent:
			handled = CompanyOnCtlRepeatEvent(event);
			break;
		case keyDownEvent:
			handled = CompanyOnKeyDownEvent(event);
			break;
		case menuEvent:
			handled = CompanyOnMenuEvent(event);
			break;
		case frmOpenEvent:
			handled = CompanyOnFrmOpenEvent();
			break;
		case menuOpenEvent:
			handled = CompanyOnMenuOpenEvent();
			break;
		case frmUpdateEvent:
			handled = CompanyOnFrmUpdateEvent();
			break;
		case winEnterEvent:
			dia_win_enter();
			break;
		default:
			handled = dia_handle_event(event, CompanyDataMoveObjects);
			break;
	}
	return (handled);
}
