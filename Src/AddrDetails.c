#include "AddrDetails.h"
#include "AddrEdit.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "AddrNote.h"
#include "AddressRsc.h"
#include "AddressDB.h"
#include "AddressDB2.h"
#include "globals.h"
#include <Category.h>
#include <UIResources.h>
#include <HsNavCommon.h>
#include <HsExt.h>
#include "AddrXTDB.h"
#include "dia.h"
#include "syslog.h"
#include "ContactsDB2.h"

#define	fieldMapRscType		'fmap'

//Globals
/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/

static Boolean	PrvDetailsSelectCategory (UInt16 * category);
static UInt16	PrvDetailsApply (UInt16 category, Boolean categoryEdited);
static UInt16	PrvBirthdayApply ();
static void		PrvDetailsInit (UInt16 * categoryP);
static void 	PrvBirthdayInit (UInt16 * categoryP);
void 			BirthdayOnDateSelector();
void			BirthdayOnDateClear();
void 			BirthdayOnRemindCheck();

void BirthdayOnRemindCheck()
{
	UInt16 remindCheck=CtlGetValue(CustomGetObjectPtrSmp(BirthdayRemindCheck));
	if(remindCheck)
	{
		CustomShowObjectSmp(BirthdayRemindDays);
		CustomShowObjectSmp(1217);
		CustomShowObjectSmp(1219);
		FrmSetFocus(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), BirthdayRemindDays));
	}
	else
	{
		CustomHideObjectSmp(BirthdayRemindDays);
		CustomHideObjectSmp(1217);
		CustomHideObjectSmp(1219);
	}
}

void BirthdayOnDateClear()
{
	globalVars* globals = getGlobalsPtr();
	globals->gM=0;
	globals->gD=0;
	globals->gY=0;
	StrCopy(globals->gDateTxt, "None");
	CustomSetCtlLabelPtrSmp(BirthdayDateSelector, globals->gDateTxt);	
	CtlSetValue(CustomGetObjectPtrSmp(BirthdayRemindCheck), false); 
	CustomHideObjectSmp(BirthdayRemindDays);
	CustomHideObjectSmp(1217);
	CustomHideObjectSmp(1219);
}	

void BirthdayOnDateSelector()
{
	globalVars* globals = getGlobalsPtr();
	Int16 lYear, lDay, lMonth;
	Char lStr[255];
	if(globals->gM+globals->gD+globals->gY!=0)
	{
		lYear=globals->gY;
		lDay=globals->gD;
		lMonth=globals->gM;
	}
	else
	{
		DateType lDate;
		DateSecondsToDate(TimGetSeconds(), &lDate);
		lYear=lDate.year+1904;
		lDay=lDate.day;
		lMonth=lDate.month;
	}
	StrCopy(lStr, "Birthday");
	if(SelectDay(selectDayByDay, &lMonth, &lDay, &lYear, lStr))
	{
		//Set new selector label -- new date was selected
		DateFormatType dateFormat;
		globals->gM=lMonth;
		globals->gD=lDay;
		globals->gY=lYear;
		dateFormat=PrefGetPreference(prefLongDateFormat);
		DateToAscii(lMonth, lDay, lYear, dateFormat, globals->gDateTxt);
		CustomSetCtlLabelPtrSmp(BirthdayDateSelector, globals->gDateTxt);
	}
	return;
}

static Boolean DetailsOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean				handled;
	UInt16 				updateCode;
	
	switch (event->data.ctlSelect.controlID)		
	{
	case DetailsOkButton:
		updateCode = PrvDetailsApply (globals->category, globals->categoryEdited);
		SetDialer();
		ToolsLeaveForm ();
		FrmUpdateForm (EditView, updateCode);
		handled = true;
		break;
	case DetailsCancelButton:
		SetDialer();
		if (globals->categoryEdited)
			FrmUpdateForm (EditView, updateCategoryChanged);
		ToolsLeaveForm ();
		handled = true;
		break;
	case DetailsDeleteButton:
		SetDialer();
		if ( DetailsDeleteRecord ())
		{
			FrmCloseAllForms ();
			FrmGotoForm (ListView);
		}
		handled = true;
		break;

	case DetailsNoteButton:
		PrvDetailsApply (globals->category, globals->categoryEdited);
		FrmReturnToForm (EditView);
		if (NoteViewCreate())
		{
			FrmGotoForm (NewNoteView);
			globals->RecordNeededAfterEditView = true;
		}
		handled = true;
		break;

	case DetailsCategoryTrigger:
		globals->categoryEdited = PrvDetailsSelectCategory (&globals->category) || globals->categoryEdited;
		handled = true;
		break;
	}
	return handled;
}
		
static Boolean BirthdayOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean				handled;
	UInt16 				updateCode;
	
	switch (event->data.ctlSelect.controlID)		
	{
	case BirthdayDateSelector:
		BirthdayOnDateSelector();
		handled=true;
		break;
	case BirthdayDateClear:
		BirthdayOnDateClear();
		handled=true;
		break;
	case BirthdayRemindCheck:
		BirthdayOnRemindCheck();
		handled=true;
		break;
	case BirthdayOkButton:
		updateCode = PrvBirthdayApply ();
		SetDialer();
		FrmReturnToForm(EditView);
		FrmUpdateForm (EditView, updateCode);
		handled = true;
		break;
	case BirthdayCancelButton:
		SetDialer();
		FrmReturnToForm(EditView);
		handled = true;
		break;
	}
	return handled;
}

static Boolean DetailsOnKeyDownEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	UInt16 updateCode;
	if(IsFiveWayNavEvent(event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			updateCode = PrvDetailsApply (globals->category, globals->categoryEdited);
			ToolsLeaveForm ();
			if (updateCode)
				FrmUpdateForm (EditView, updateCode);
			handled = true;
		}
	}
	return handled;
}

static Boolean BirthdayOnKeyDownEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	UInt16 updateCode;
	if(IsFiveWayNavEvent(event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			/*updateCode = PrvDetailsApply (globals->category, globals->categoryEdited);
			ToolsLeaveForm ();
			if (updateCode)
				FrmUpdateForm (EditView, updateCode);
			handled = true;*/
		}
	}
	return handled;
}

static Boolean DetailsOnFrmOpenEvent()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm;
	frm = FrmGetFormPtr(DetailsDialog);
	dia_save_state(); 
	dia_enable(frm, false);
	PrvDetailsInit (&globals->category);
	FrmDrawForm (frm);
	globals->categoryEdited = false;
	RestoreDialer();
	return true;
}

static Boolean BirthdayOnFrmOpenEvent()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm;
	frm = FrmGetFormPtr(BirthdayDialog);
	dia_save_state(); 
	dia_enable(frm, false);
	PrvBirthdayInit (&globals->category);
	FrmDrawForm (frm);
	globals->categoryEdited = false;
	RestoreDialer();
	return true;
}

static Boolean DetailsOnFrmUpdateEvent()
{
	FormPtr frm;
	frm = FrmGetFormPtr(DetailsDialog);
	FrmDrawForm (frm);
	return true;
}

static Boolean BirthdayOnFrmUpdateEvent()
{
	FormPtr frm;
	frm = FrmGetFormPtr(BirthdayDialog);
	FrmDrawForm (frm);
	return true;
}

Boolean DetailsHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = DetailsOnCtlSelectEvent(event);
			break;	
		case keyDownEvent:
			handled = DetailsOnKeyDownEvent(event);
			break;	
		case frmOpenEvent:
			handled = DetailsOnFrmOpenEvent();
			break;	
		case frmUpdateEvent:
			handled = DetailsOnFrmUpdateEvent();
			break;
		default:
			handled = dia_handle_event(event, NULL);
	}
	return (handled);
}

Boolean BirthdayHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = BirthdayOnCtlSelectEvent(event);
			break;	
		case keyDownEvent:
			handled = BirthdayOnKeyDownEvent(event);
			break;	
		case frmOpenEvent:
			handled = BirthdayOnFrmOpenEvent();
			break;	
		case frmUpdateEvent:
			handled = BirthdayOnFrmUpdateEvent();
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	return (handled);
}

Boolean DetailsDeleteRecord (void)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 ctlIndex;
	UInt16 buttonHit;
	FormPtr alert;
	Boolean archive;
	
	// Display an alert to comfirm the operation.
	RestoreDialer();
	alert = FrmInitForm (DeleteAddrDialog);

	// Set the "save backup" checkbox to its previous setting.
	ctlIndex = FrmGetObjectIndex (alert, DeleteAddrSaveBackup);
	FrmSetControlValue (alert, ctlIndex, globals->SaveBackup);

	buttonHit = FrmDoDialog (alert);
	dia_restore_state();
	
	archive = FrmGetControlValue (alert, ctlIndex);

	FrmDeleteForm (alert);
	if (buttonHit == DeleteAddrCancel)
		return (false);
		
	SetDialer();
	
	// Remember the "save backup" checkbox setting.
	globals->SaveBackup = archive;

	ToolsDeleteRecord(archive);

	return (true);
}

#pragma mark -


Boolean PrvDetailsSelectCategory (UInt16 * category)
{
	globalVars* globals = getGlobalsPtr();
	Boolean categoryEdited;

	categoryEdited = CategorySelect (globals->AddrDB, FrmGetActiveForm (),
									 DetailsCategoryTrigger, DetailsCategoryList,
									 false, category, globals->CategoryName, 1, categoryDefaultEditCategoryString);

	return (categoryEdited);
}


UInt16 PrvBirthdayApply ()
{
	globalVars* globals = getGlobalsPtr();
	UInt16                updateCode = 0;
	UInt16 index;
	DateType bday;
	MemHandle mH;
	AddressDBBirthdayFlags flag;
	P1ContactsDBRecordType contactsRec;
	P1ContactsDBRecordFlags changed;
	PrvP1ContactsDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &contactsRec, &mH);  
	if(globals->gD + globals->gM + globals->gY != 0)
	{
		UInt16 remindCheck=CtlGetValue(CustomGetObjectPtrSmp(BirthdayRemindCheck));
		UInt16 remind = REMIND_NONE;
		bday.year = globals->gY - 1904;
		bday.month = globals->gM;
		bday.day = globals->gD;
		if(contactsRec.birthdayInfo.birthdayDate.day != bday.day ||
		contactsRec.birthdayInfo.birthdayDate.day != bday.year ||
		contactsRec.birthdayInfo.birthdayDate.month != bday.month)
		{
			updateCode = updateBirthday;
		}
		contactsRec.birthdayInfo.birthdayDate = bday;
		P1ContactsSetBitMacro(changed, P1ContactsbirthdayMask);
		P1ContactsSetBitMacro(changed, P1ContactsbirthdayDate);
		P1ContactsSetBitMacro(changed, P1ContactsbirthdayPreset);
		if(remindCheck)
		{
			if(!CustomFldIsEmptySmp(BirthdayRemindDays))
			{
				Char str[255];
				StrCopy(str, CustomFldGetTextPtrSmp(BirthdayRemindDays));
				remind = StrAToI(str);
			}
			else
				remind = 0;
		}
		if(remind != REMIND_NONE)
		{
			contactsRec.birthdayInfo.birthdayPreset = remind;
			flag.alarm = 1;
		}
		else
		{
			flag.alarm = 0;
		}
		flag.reserved = 0;
		contactsRec.birthdayInfo.birthdayMask = flag;
		index = globals->CurrentRecord;
		
		PrvP1ContactsDBChangeRecord(globals->adxtLibRef, globals->AddrDB, &index, &contactsRec, changed, 0);
	}
	else
	{
		MemHandleUnlock(mH);
		index = globals->CurrentRecord;
		if(contactsRec.birthdayInfo.birthdayDate.day != 0 ||
			contactsRec.birthdayInfo.birthdayDate.day != 0)
		{
			updateCode = updateBirthday;
		}
		PrvP1ContactsDBClearBDay(globals->AddrDB, &index, changed);
	}
	return updateCode;	
}

UInt16 PrvDetailsApply (UInt16 category, Boolean categoryEdited)
{
	globalVars* globals = getGlobalsPtr();
	UInt16               attr;
	UInt16                updateCode = 0;
	Boolean           secret;
	Boolean            dirty = false;
#ifndef CONTACTS
	AddrDBRecordType currentRecord;
	MemHandle recordH;
#else
	P1ContactsDBRecordType contactsRec;
	MemHandle mH;
	UInt16 index;
#endif
	UInt32 contactsDBSize=0;
	MemHandle handle=0, newHandle=0;
	Int16 resizeNeeded=0;
	
	P1ContactsDBRecordFlags changed;
			
#ifndef CONTACTS
	AddrDBRecordFlags changedFields;
#endif
	UInt16               newPhoneFieldToDisplay;
	Err               err;
	
	changed.allBits = 0;
	changed.allBits2 = 0;
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, &attr, NULL, NULL);
	
	// Get the phone number to show at the list view.
#ifndef CONTACTS
	AddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &currentRecord, &recordH);
	newPhoneFieldToDisplay = CustomLstGetSelection(DetailsPhoneList);
	if (currentRecord.options.phones.displayPhoneForList != newPhoneFieldToDisplay)
	{
		currentRecord.options.phones.displayPhoneForList = newPhoneFieldToDisplay;
		changedFields.allBits = 0;
		err = AddrDBChangeRecord(globals->AddrDB, &globals->CurrentRecord, &currentRecord,
							   changedFields);
		if (err)
		{
			MemHandleUnlock(recordH);
			FrmAlert(DeviceFullAlert);
		}

		updateCode |= updateListViewPhoneChanged;
	}
	else
		MemHandleUnlock(recordH);
#endif

#ifdef CONTACTS
	PrvP1ContactsDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &contactsRec, &mH);  
	newPhoneFieldToDisplay = CustomLstGetSelection(DetailsPhoneList);
	if (contactsRec.options.phones.displayPhoneForList != newPhoneFieldToDisplay)
	{
		contactsRec.options.phones.displayPhoneForList = newPhoneFieldToDisplay;
		index = globals->CurrentRecord;
		err = 	PrvP1ContactsDBChangeRecord(globals->adxtLibRef, globals->AddrDB, &index, &contactsRec, changed, 0);

		if (err)
		{
			MemHandleUnlock(mH);
			FrmAlert(DeviceFullAlert);
		}

		updateCode |= updateListViewPhoneChanged;
	}
	else
		MemHandleUnlock(mH);
#endif
	
	// Get the current setting of the secret checkbox and compare it the
	// the setting of the record.  Update the record if the values
	// are different.  If the record is being set 'secret' for the
	//   first time, and the system 'hide secret records' setting is
	//   off, display an informational alert to the user.
	
	secret = CtlGetValue (CustomGetObjectPtrSmp (DetailsSecretCheckbox));
	if (((attr & dmRecAttrSecret) == dmRecAttrSecret) != secret)
	{
		if (globals->PrivateRecordVisualStatus > showPrivateRecords)
		{
			updateCode |= updateItemHide;
		}
		else if (secret)
		{
			FrmAlert (privateRecordInfoAlert);
		}
		dirty = true;

		if (secret)
			attr |= dmRecAttrSecret;
		else
			attr &= ~dmRecAttrSecret;
	}


	// Compare the current category to the category setting of the dialog.
	// Update the record if the category are different.
	if ((attr & dmRecAttrCategoryMask) != category)
	{
		attr &= ~dmRecAttrCategoryMask;
		attr |= category;
		dirty = true;

		globals->CurrentCategory = category;
		updateCode |= updateCategoryChanged;
	}

	// If current category was moved, deleted renamed, or merged with
	// another category, then the list view needs to be redrawn.
	if (categoryEdited)
	{
		globals->CurrentCategory = category;
		updateCode |= updateCategoryChanged;
	}


	// Save the new category and/or secret status, and mark the record dirty.
	if (dirty)
	{
		attr |= dmRecAttrDirty;
		DmSetRecordInfo (globals->AddrDB, globals->CurrentRecord, &attr, NULL);
	}
	
	return (updateCode);
}


void PrvDetailsInit (UInt16 * categoryP)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 attr;
	UInt16 category;
	ControlPtr ctl;
	ListPtr popupPhoneList;
	univAddrDBRecordType currentRecord;
	MemHandle recordH;
#ifndef CONTACTS
	AddrAppInfoPtr appInfoPtr;
#endif
	UInt16 phoneLabel;
	UInt16 i;UInt32 id;
#ifdef CONTACTS
	P1ContactsAppInfoPtr p1InfoPtr;
#endif
	//init birthday
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
	
	popupPhoneList = CustomGetObjectPtrSmp (DetailsPhoneList);
	
	// Make a list of the phone labels used by this record
#ifndef CONTACTS
	appInfoPtr = (AddrAppInfoPtr) AddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	AddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &currentRecord, &recordH);
	for (i = 0; i < numPhoneFields; i++)
	{
		phoneLabel = GetPhoneLabel(&currentRecord, firstPhoneField + i);
		globals->DetailsPhoneListChoices[i] = appInfoPtr->fieldLabels[phoneLabel +
															 ((phoneLabel < numPhoneLabelsStoredFirst) ?
															  firstPhoneField : (addressFieldsCount - numPhoneLabelsStoredFirst))];
	}
#else
	p1InfoPtr = P1ContactsDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	PrvP1ContactsDBGetRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &currentRecord, &recordH);  
	for (i = 0; i < P1ContactsnumPhoneFields; i++)
	{
		phoneLabel = P1ContactsGetPhoneLabel(&currentRecord, P1ContactsfirstPhoneField + i);
		globals->DetailsPhoneListChoices[i] = p1InfoPtr->fieldLabels[phoneLabel + 
							 ((phoneLabel < P1ContactsnumPhoneLabelsStoredFirst) ?
															  P1ContactsfirstPhoneField : (P1ContactsaddressFieldsCount - P1ContactsnumPhoneLabelsStoredFirst-3))];
	}
#endif
	LstSetListChoices(popupPhoneList, globals->DetailsPhoneListChoices, univNumPhoneFields);
	LstSetSelection (popupPhoneList, currentRecord.options.phones.displayPhoneForList);
	LstSetHeight (popupPhoneList, univNumPhoneFields);
	CtlSetLabel(CustomGetObjectPtrSmp (DetailsPhoneTrigger),
			LstGetSelectionText (popupPhoneList, currentRecord.options.phones.displayPhoneForList));
		
	// If the record is mark secret, turn on the secret checkbox.
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, &attr, NULL, NULL);
	ctl = CustomGetObjectPtrSmp (DetailsSecretCheckbox);
	CtlSetValue (ctl, attr & dmRecAttrSecret);
	
	// Set the label of the category trigger.
	category = attr & dmRecAttrCategoryMask;
	CategoryGetName (globals->AddrDB, category, globals->CategoryName);
	ctl = CustomGetObjectPtrSmp (DetailsCategoryTrigger);
	CategorySetTriggerLabel (ctl, globals->CategoryName);
	
	// Return the current category and due date.
	*categoryP = category;

	MemHandleUnlock(recordH);
#ifdef CONTACTS
	MemPtrUnlock(p1InfoPtr);
#else
	MemPtrUnlock(appInfoPtr);
#endif
}

void PrvBirthdayInit (UInt16 * categoryP)
{
	MemHandle mH;
	P1ContactsDBRecordType contact;
	globalVars* globals = getGlobalsPtr();
	PrvP1ContactsDBGetRecordBDOnly (globals->AddrDB, globals->CurrentRecord, &contact, &mH);
		
	if(mH == 0)
		return;
	
	//remindDays=ALARM_NONE;//impossible value;
	
	if(contact.birthdayInfo.birthdayDate.day != 0 || contact.birthdayInfo.birthdayDate.month != 0)
	{
		UInt16 remindDays;
		DateFormatType dateFormat;
		dateFormat=PrefGetPreference(prefLongDateFormat);
		DateToAscii(contact.birthdayInfo.birthdayDate.month, contact.birthdayInfo.birthdayDate.day, contact.birthdayInfo.birthdayDate.year+1904, dateFormat, globals->gDateTxt);
		CustomSetCtlLabelPtrSmp(BirthdayDateSelector, globals->gDateTxt);
		globals->gY = contact.birthdayInfo.birthdayDate.year + 1904;
		globals->gD = contact.birthdayInfo.birthdayDate.day;
		globals->gM = contact.birthdayInfo.birthdayDate.month;
		if(contact.birthdayInfo.birthdayMask.alarm != 0)
		{
			Char lStr[255];
			remindDays = contact.birthdayInfo.birthdayPreset;
			StrIToA(lStr, remindDays);
			CtlSetValue(CustomGetObjectPtrSmp(BirthdayRemindCheck), true);
			CustomShowObjectSmp(BirthdayRemindDays);
			CustomShowObjectSmp(1217);
			CustomShowObjectSmp(1219);
			CustomEditableFldSetTextPtrSmp(BirthdayRemindDays, lStr);	
		}	
	}
	else
	{
		globals->gM=0;
		globals->gD=0;
		globals->gY=0;
	}	
	MemHandleUnlock(mH);
	/*globalVars* globals = getGlobalsPtr();
	UInt16 attr;
	UInt16 category;
	ControlPtr ctl;
	ListPtr popupPhoneList;
	univAddrDBRecordType currentRecord;
	MemHandle recordH;
#ifndef CONTACTS
	AddrAppInfoPtr appInfoPtr;
#endif
	UInt16 phoneLabel;
	UInt16 i, counter;UInt32 id;
	UInt16 addrXTDBSize;
	DmOpenRef addrXTDB;
#ifdef CONTACTS
	P1ContactsAppInfoPtr p1InfoPtr;
#endif
	//init birthday
	Char lStr[31], lStr2[31];
	UInt16 d, m, y;
	DateTimeType date;
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
	CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &addrXTDB);
	addrXTDBSize = DmNumRecords(addrXTDB);		
	StrIToA(lStr, id);
	globals->gD=0;
	globals->gM=0;
	globals->gY=0;
	
	popupPhoneList = CustomGetObjectPtrSmp (DetailsPhoneList);
	for(counter=0;counter<addrXTDBSize;counter++)
	{
		AddrXTDBRecordType rec;
		MemHandle mH;
		UInt32 recId, seconds, remind;
		
		if(AddrXTDBGetRecord(globals->adxtLibRef, addrXTDB, counter, &rec, &mH) != 0)
			continue;
		
		recId = rec.id;
		remind = rec.remind;
		seconds = rec.bday;
		
		MemHandleUnlock(mH);
		
		if(recId == id)
		{
			DateFormatType dateFormat;
			TimSecondsToDateTime(seconds, &date);
			d=date.day;
			m=date.month;
			y=date.year;
			globals->gD=d;
			globals->gM=m;
			globals->gY=y;
			dateFormat=PrefGetPreference(prefDateFormat);
			DateToAscii(m, d, y, dateFormat, globals->gDateTxt);
			CustomSetCtlLabelPtrSmp(DetailsDateSelector, globals->gDateTxt);
			StrIToA(lStr2, remind);
			if(remind != REMIND_NONE)
			{
				CtlSetValue(CustomGetObjectPtrSmp(DetailsRemindCheck), true);
				CustomShowObjectSmp(DetailsRemindDays);
				CustomShowObjectSmp(1217);
				CustomShowObjectSmp(1219);
				CustomEditableFldSetTextPtrSmp(DetailsRemindDays, lStr2);
			}			
			break;
		}			
	}	
	DmCloseDatabase(addrXTDB);
	
	if(globals->gDeviceFlags.bits.treo)
		HsGrfSetStateExt(false, true, true, false, false, false);
	
	MemHandleUnlock(recordH);
#ifdef CONTACTS
	MemPtrUnlock(p1InfoPtr);
#else
	MemPtrUnlock(appInfoPtr);
#endif*/
}