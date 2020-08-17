#include "AddrDialList.h"
#include "Address.h"

// Convenient access
#define gAddrP			(&(globals->gDialListData->addr))
#define gAddrH			(globals->gDialListData->addrH)
#define gAppInfoP		(globals->gDialListData->appInfoP)
#define gDisplayName	(globals->gDialListData->displayName)
#define gPhones			(globals->gDialListData->phones)
#define gPhonesCount	(globals->gDialListData->phonesCount)
#define gPhoneX			(globals->gDialListData->phoneX)
#define gSelectedIndex	(globals->gDialListData->selectedIndex)

typedef enum
{
	dialtype_phone = 0,
	dialtype_email,
	dialtype_im,
	dialtype_sms,
	dialtype_map,
	dialtype_web
} dialType;


/***********************************************************************
 *
 *	Internal types
 *
 ***********************************************************************/


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

// Internal tools
static Boolean	PrvDialListCanBePhoneNumber( Char* text, Int16 textLen );
static void		PrvDialListPhoneNumberFilter( Char* outString, Int16* outLen, const Char* inString, Int16 inLen );
static Char*	PrvDialListAllocStringFrom( const Char* s1, const Char* s2, const Char* s3, Boolean checkLineFeed );
static void		PrvDialListSetFieldHandle( FieldType* fldP, MemHandle handle );

// Internal Dial List function
static void		PrvDialListBuildDescription( void );
#ifdef FORM_GADGET_TYPE_IN_CALLBACK_DEFINED // Added for Compatibility with SDK 4.0 update 1
static Boolean	PrvDialListHandleDescriptionEvent( struct FormGadgetTypeInCallback *gadgetP, UInt16 cmd, void *paramP );
#else
static Boolean	PrvDialListHandleDescriptionEvent( struct FormGadgetType *gadgetP, UInt16 cmd, void *paramP );
#endif

static void		PrvDialListInit( FormType* frmP );
static void		PrvDialListScroll( WinDirectionType direction );
static void		PrvDialListDrawPhoneItem( Int16 index, RectangleType *boundsP, Char **itemsText );
static void		PrvDialListUpdateAfterSelection( FormType* frmP );
static void		PrvDialListUpdatePhoneNumber( FieldType* fldP );
static void		PrvDialListFreeMemory( void );
static void		PrvDialListLeaveDialog( void );
static Boolean	PrvDialListDialSelected( FormType* frmP );
static Boolean	PrvDialListMessageSelected( FormType* frmP );
static Boolean 	DialListOnUpdate(UInt16 updateCode);
static Boolean 	DialListInitPhoneList(UInt16 recordIndex, UInt16 phoneIndex, UInt16 lineIndex);

static UInt16	PrvGetFieldLabel(UInt16 field, univAddrDBRecordType* rec)
{
	UInt16 label;
#ifdef CONTACTS
	switch(field)
	{
		case P1Contactsphone1:
			label = (AddressPhoneLabels)(rec->options.phones.phone1);
			break;
		case P1Contactsphone2:
			label = (AddressPhoneLabels)(rec->options.phones.phone2);
			break;
		case P1Contactsphone3:
			label = (AddressPhoneLabels)(rec->options.phones.phone3);
			break;
		case P1Contactsphone4:
			label = (AddressPhoneLabels)(rec->options.phones.phone4);
			break;
		case P1Contactsphone5:
			label = (AddressPhoneLabels)(rec->options.phones.phone5);
			break;
		case P1Contactsphone6:
			label = (AddressPhoneLabels)(rec->options.phones.phone6);
			break;
		case P1Contactsphone7:
			label = (AddressPhoneLabels)(rec->options.phones.phone7);
			break;			
	}
#else
	switch (field)
	{
	case phone1:
		label = (AddressPhoneLabels)(rec->options.phones.phone1);
		break;
	case phone2:
		label = (AddressPhoneLabels)(rec->options.phones.phone2);
		break;
	case phone3:
		label = (AddressPhoneLabels)(rec->options.phones.phone3);
		break;
	case phone4:
		label = (AddressPhoneLabels)(rec->options.phones.phone4);
		break;
	case phone5:
		label = (AddressPhoneLabels)(rec->options.phones.phone5);
		break;
	}
#endif
	return label;
}

Boolean AddressDialable(UInt16 recordIndex, UInt16 phoneIndex)
{
	globalVars* globals = getGlobalsPtr();
	Err			err;
	Int16		fieldIndex;
	DialListPhoneType	phones[kMaxPhonesCount];
	UInt16 phonesCount;
	
	// Get the current record
	univAddrDBRecordType rec;
	MemHandle mH;
	err = univAddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, recordIndex, &rec, &mH);
	if (err)
		goto exit;	
	// Check default phone index
	// If type is not a supported one, continue
	if (phoneIndex == kDialListShowInListPhoneIndex)
		phoneIndex = rec.options.phones.displayPhoneForList;

	// Build the phone array
	// Splitting each phone field per line
	phonesCount = 0;
	
	for (fieldIndex = univFirstPhoneField; fieldIndex <= univLastPhoneField; fieldIndex++)
	{
		Char* text = rec.fields[fieldIndex];
		Char* next = text;
		Int16 length;
		Int16 phoneLineIndex = 0;
		Boolean email = false;
		UInt16 label;
		
		if (text && ToolsIsPhoneIndexSupported(&rec, fieldIndex - univFirstPhoneField))
		{
			do
			{
				phones[phonesCount].number = text;

				// Check if a another line is available
				next = StrChr(text, chrLineFeed);

				// If a line feed is found
				if (next)
					length = next - text;
				else
					length = StrLen(text);

				// Check that the phone is a phone number (ie at least one phone character)
				
				
				label = PrvGetFieldLabel(fieldIndex, &rec);					
				
				if (PrvDialListCanBePhoneNumber(text, length) || (label == univEmailLabel && StrLen(text) >0 && StrChr(text, '@') != NULL))
				{
					phonesCount++;
					if (phonesCount == kMaxPhonesCount)
						break;
				}
				text = next + 1;
				phoneLineIndex++;
			}
			while (next);
		}
		if (phonesCount == kMaxPhonesCount)
			break;
	}
	
	// Exit if no phone are available for this record
	if (!(phonesCount))
		goto exit;

	if (mH)
		MemHandleUnlock(mH);
	
	
	if(phonesCount>0)
		return true;
	else
		return false;
exit:
	if (mH)
		MemHandleUnlock(mH);
	
	globals->gDialListData = 0;
	return false;

	return true;
}

Boolean DialListInitPhoneList(UInt16 recordIndex, UInt16 phoneIndex, UInt16 lineIndex)
{
	globalVars* globals = getGlobalsPtr();
	Err			err;
	Int16		fieldIndex;
	AddressPhoneLabels label;
	UInt16 		prefsSize;
	Int16 		prefsVersion;
	Int16 		phoneLabelIndex;
	Int16 		prefixLen = 0;
	Int16 		i, length;
	Boolean 	added;
	UInt32 		recordID;
	Char* text;
									
	globals->gDialListData = MemPtrNew(sizeof(DialListDataType));
	if (!globals->gDialListData)
		return false;
	MemSet(globals->gDialListData, sizeof(DialListDataType), 0);
	
	prefsSize = sizeof (AddrDialOptionsPreferenceType);
	prefsVersion =  PrefGetAppPreferences (CREATORID, addrPrefDialOptionsDataID, &(globals->dialOptions), &prefsSize, true);
	if (prefsVersion <= noPreferenceFound)
	{
		FillConnectOptions();//load default options
		SetDefaultConnectOptions(&(globals->dialOptions));	
	}
		
	// Get the current record
	err = univAddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, recordIndex, gAddrP, &gAddrH);
	DmRecordInfo (globals->AddrDB,recordIndex, NULL, &recordID, NULL);
	
	if (err)
		return false;
	
	// Check default phone index
	// If type is not a supported one, continue
	if (phoneIndex == kDialListShowInListPhoneIndex)
		phoneIndex = gAddrP->options.phones.displayPhoneForList;

	gAppInfoP = univAddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);

	// Build the phone array
	// Splitting each phone field per line
	gPhonesCount = 0;
#ifdef CONTACTS
	for (fieldIndex = P1ContactsfirstPhoneField; fieldIndex <= P1ContactslastPhoneField; fieldIndex++)
	{
		Char* next;
		Int16 phoneLineIndex = 0;
		added = false;		
		text = gAddrP->fields[fieldIndex];		
		
		if (text)
		{
			label = PrvGetFieldLabel(fieldIndex, gAddrP);		
			do
			{
				// Check if a another line is available
				next = StrChr(text, chrLineFeed);

				// If a line feed is found
				if (next)
					length = next - text;
				else
					length = StrLen(text);

				gPhones[gPhonesCount].allocated = false;
				// Check that the phone is a phone number (ie at least one phone character)
				phoneLabelIndex = P1ContactsGetPhoneLabel(gAddrP, fieldIndex); // 0 = Work... 7 = Mobile
					// gAppInfoP->fieldLabels stored name of fields in a strange way
				if (phoneLabelIndex < 7) // Work, Home, Fax, Other, Email are stored as phone1... phone5
					phoneLabelIndex += P1Contactsphone1;
				else	// Main, Pager and Mobile are stored after Note.
					phoneLabelIndex += P1Contactsphone1 + 29;
				if(label == P1ContactsemailLabel)
				{
					added = true;
					gPhones[gPhonesCount].number = MemPtrNew(length + 1);
					StrCopy(gPhones[gPhonesCount].number, text);
					gPhones[gPhonesCount].allocated = true;		
					gPhones[gPhonesCount].type = dialtype_email;	
					gPhones[gPhonesCount].numberLen = length;
				}
				else if(PrvDialListCanBePhoneNumber(text, length))
				{
					added = true;
					prefixLen = 0;
					if(globals->dialOptions.dialPrefixActive && text[0] != '+')
					{
						prefixLen = StrLen(globals->dialOptions.dialPrefix);
					}
					if(prefixLen)
					{
						gPhones[gPhonesCount].number = MemPtrNew(length + prefixLen + 2);
						StrCopy(gPhones[gPhonesCount].number, globals->dialOptions.dialPrefix);
						StrCat(gPhones[gPhonesCount].number, " ");
						StrCat(gPhones[gPhonesCount].number, text);
						gPhones[gPhonesCount].allocated = true;
					}
					else
					{
						gPhones[gPhonesCount].number = text;
						gPhones[gPhonesCount].allocated = false;
					}
					if(prefixLen > 0)
						gPhones[gPhonesCount].numberLen = length + prefixLen + 1;
					else
						gPhones[gPhonesCount].numberLen = length;
					gPhones[gPhonesCount].type = dialtype_phone;
				}
				if(added)
				{
					gPhones[gPhonesCount].label = gAppInfoP->fieldLabels[phoneLabelIndex];

					// Is is the selected one
					if ((phoneIndex == fieldIndex - P1ContactsfirstPhoneField) && (phoneLineIndex == lineIndex))
						gSelectedIndex = gPhonesCount;					
										
					gPhonesCount++;
					
					if (gPhonesCount == kMaxPhonesCount)
						break;
				}
				text = next + 1;
				phoneLineIndex++;
			}
			while (next);
		}
		if (gPhonesCount == kMaxPhonesCount)
			break;		
	}
	//now check for address, IM and web site fields
	if(globals->dialOptions.creatorID[connectoptions_web])
	{
		text = gAddrP->fields[P1Contactswebpage];	
		if (text)
		{
			gPhones[gPhonesCount].number = text;
			gPhones[gPhonesCount].allocated = false;
			gPhones[gPhonesCount].numberLen = StrLen(text);
			gPhones[gPhonesCount].label = gAppInfoP->fieldLabels[P1Contactswebpage];
			gPhones[gPhonesCount].type = dialtype_web;
			gPhonesCount++;					
		}	
	}
	if(globals->dialOptions.creatorID[connectoptions_im])
	{
		for (fieldIndex = firstChatField; fieldIndex <= lastChatField; fieldIndex++)
		{
			text = gAddrP->fields[fieldIndex];	
			if (text)
			{
				UInt16 phoneLabelNum = GetChatLabel(gAddrP, fieldIndex);
				gPhones[gPhonesCount].number = text;
				gPhones[gPhonesCount].allocated = false;
				gPhones[gPhonesCount].numberLen = StrLen(text);
				gPhones[gPhonesCount].label = gAppInfoP->fieldLabels[phoneLabelNum + 41];
				gPhones[gPhonesCount].type = dialtype_im;
				gPhonesCount++;					
			}
		}
	}
	for(i = 0; i < 3; i++)
	{
		text =  gAddrP->fields[P1Contactsaddress + (P1Contactsaddress2-P1Contactsaddress)*i];
		if (text)
		{
			UInt16 phoneLabelNum = GetAddressLabel(gAddrP, P1Contactsaddress + (P1Contactsaddress2-P1Contactsaddress)*i);
			switch(phoneLabelNum)
			{
				case 0:
					gPhones[gPhonesCount].label = gAppInfoP->fieldLabels[4];
					break;
				case 1:
					gPhones[gPhonesCount].label = gAppInfoP->fieldLabels[5];
					break;
				case 2:
					gPhones[gPhonesCount].label = gAppInfoP->fieldLabels[7];
					break;							
			}					
			gPhones[gPhonesCount].number2 = gAddrP->fields[P1Contactscity + (P1Contactsaddress2-P1Contactsaddress)*i];
			gPhones[gPhonesCount].number3 = gAddrP->fields[P1Contactsstate + (P1Contactsaddress2-P1Contactsaddress)*i];
			gPhones[gPhonesCount].number4 = gAddrP->fields[P1ContactszipCode + (P1Contactsaddress2-P1Contactsaddress)*i];
			gPhones[gPhonesCount].number5 = gAddrP->fields[P1Contactscountry + (P1Contactsaddress2-P1Contactsaddress)*i];
			gPhones[gPhonesCount].number5 = gAddrP->fields[P1Contactscountry + (P1Contactsaddress2-P1Contactsaddress)*i];
			gPhones[gPhonesCount].recID = recordID;
			
			gPhones[gPhonesCount].allocated = false;
			gPhones[gPhonesCount].number = text;
			gPhones[gPhonesCount].numberLen = StrLen(text);
			gPhones[gPhonesCount].type = dialtype_map;
			gPhonesCount++;					
		}
	}
	
	
		
#else
	for (fieldIndex = firstPhoneField; fieldIndex <= lastPhoneField; fieldIndex++)
	{
		Char* text = gAddrP->fields[fieldIndex];
		Char* next = text;
		Int16 phoneLineIndex = 0;
		
		label = PrvGetFieldLabel(fieldIndex, gAddrP);
		
		if (text && (label == emailLabel || ToolsIsPhoneIndexSupported(gAddrP, fieldIndex - firstPhoneField)))
		{
			do
			{
				gPhones[gPhonesCount].number = text;

				// Check if a another line is available
				next = StrChr(text, chrLineFeed);

				// If a line feed is found
				if (next)
					length = next - text;
				else
					length = StrLen(text);
				
				gPhones[gPhonesCount].allocated = false;
				// Check that the phone is a phone number (ie at least one phone character)
				if ((PrvDialListCanBePhoneNumber(text, length) && label != emailLabel) || (label == emailLabel/* && ToolsVersaMailPresent()*/))
				{
					prefixLen = 0;
					if(globals->dialOptions.dialPrefixActive)
					{
						prefixLen = StrLen(globals->dialOptions.dialPrefix);
					}
					if(prefixLen)
					{
						gPhones[gPhonesCount].number = MemPtrNew(length + prefixLen + 2);
						StrCopy(gPhones[gPhonesCount].number, globals->dialOptions.dialPrefix);
						StrCat(gPhones[gPhonesCount].number, " ");
						StrCat(gPhones[gPhonesCount].number, text);
						gPhones[gPhonesCount].allocated = true;
					}
					else
					{
						gPhones[gPhonesCount].number = text;
						gPhones[gPhonesCount].allocated = false;
					}
					
					if(prefixLen > 0)
						gPhones[gPhonesCount].numberLen = length + prefixLen + 1;
					else
						gPhones[gPhonesCount].numberLen = length;
					
					phoneLabelIndex = GetPhoneLabel(gAddrP, fieldIndex); // 0 = Work... 7 = Mobile
					// gAppInfoP->fieldLabels stored name of fields in a strange way
					if (phoneLabelIndex < 5) // Work, Home, Fax, Other, Email are stored as phone1... phone5
						phoneLabelIndex += phone1;
					else	// Main, Pager and Mobile are stored after Note.
						phoneLabelIndex += note - 4;
					gPhones[gPhonesCount].label = gAppInfoP->fieldLabels[phoneLabelIndex];

					// Is is the selected one
					if ((phoneIndex == fieldIndex - firstPhoneField) && (phoneLineIndex == lineIndex))
						gSelectedIndex = gPhonesCount;

					if(label == emailLabel)
						gPhones[gPhonesCount].type = dialtype_email;
					
					gPhonesCount++;
					
					phoneLabelIndex = GetPhoneLabel(gAddrP, fieldIndex); // 0 = Work... 7 = Mobile
					
					if (gPhonesCount == kMaxPhonesCount)
						break;
				}
				text = next + 1;
				phoneLineIndex++;
			}
			while (next);
		}
		if (gPhonesCount == kMaxPhonesCount)
			break;
	}
#endif
	return true;	
}

/***********************************************************************
 *
 * FUNCTION:
 *	DialListShowDialog
 *
 * DESCRIPTION:
 *	This routine show the dialog of the given record
 *	if the phoneIndex is not a phone number, first phone number is selected
 *
 * PARAMETERS:
 *	recordIndex		IN		index of the record
 *	phoneIndex		IN		index of the phone, kDialListShowInListPhoneIndex
 *							to use default show in list
 *	lineIndex		IN		index of the line
 *
 * RETURNED:
 *	false if the form must not be displayed
 *
 * REVISION HISTORY:
 *	Name		Date		Description
 *	----		----		-----------
 *	aro			6/12/00		Initial Revision
 *
 ***********************************************************************/
Boolean DialListShowDialog( UInt16 recordIndex, UInt16 phoneIndex, UInt16 lineIndex )
{
	globalVars* globals = getGlobalsPtr();
	globals->dialListRecordIndex = recordIndex;
	globals->dialListPhoneIndex = phoneIndex;
	globals->dialListLineIndex = lineIndex;
	if(!DialListInitPhoneList(recordIndex, phoneIndex, lineIndex))
		goto exit;
	// Exit if no phone are available for this record
	if (!(gPhonesCount))
		goto exit;

	// Ok so no show the dialog...
	FrmPopupForm(DialListDialog);

	return true;

exit:
	PrvDialListFreeMemory();
	return false;
}

static Boolean DialListOnLstSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	gSelectedIndex = event->data.lstSelect.selection;
	PrvDialListUpdateAfterSelection(FrmGetActiveForm());
	if(globals->gDialListKeyDown)
	{
		PrvDialListDialSelected(FrmGetActiveForm());
		globals->gDialListKeyDown = false;			
	}
	return true;
}

static Boolean DialListOnCtlSelectEvent(EventType* event)
{
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
	case DialListDialButton:
		if (PrvDialListDialSelected(FrmGetActiveForm()))
			PrvDialListLeaveDialog();
		handled = true;
		break;
	case DialListMessageButton:
		if (PrvDialListMessageSelected(FrmGetActiveForm()))
			PrvDialListLeaveDialog();
		handled = true;
		break;
	case DialListCancelButton:
		PrvDialListLeaveDialog();
		handled = true;
		break;
	}
	return handled;
}

static Boolean DialListOnFrmOpenEvent()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetActiveForm();
	dia_save_state(); 
	dia_enable(frmP, false);
	PrvDialListInit(frmP);
	FrmDrawForm(frmP);
	LstSetSelection(CustomGetObjectPtrSmp(DialListList), gSelectedIndex);
	if(globals->gNavigation)
	{
		EventType event;
		MemSet(&event, sizeof(EventType), 0);
		event.eType = keyUpEvent;
		event.data.keyDown.chr = vchrHardRockerCenter;
		event.data.keyDown.keyCode = vchrHardRockerCenter;
		event.data.keyDown.modifiers = 8;
		EvtAddEventToQueue(&event);
	}
	return true;
}

static Boolean DialListOnFrmCloseEvent()
{
	PrvDialListSetFieldHandle(CustomGetObjectPtrSmp(DialListNumberField), 0);
	PrvDialListFreeMemory();
	return false;
}

static Boolean DialListOnKeyDownEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if(TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
	{
		if(globals->gDeviceFlags.bits.treoWithPhoneKeyOnly && event->data.keyDown.chr == vchrHard1)
		{
			if(!globals->gDialListKeyDown)
			{
				EventType event2;
				MemSet(&event2, sizeof(EventType), 0);
				event2.eType = keyUpEvent;
				event2.data.keyDown.chr = vchrHardRockerCenter;
				event2.data.keyDown.keyCode = vchrHardRockerCenter;
				event2.data.keyDown.modifiers = 8;
				globals->gDialListKeyDown = true;
				EvtAddEventToQueue(&event2);
				//EvtAddEventToQueue(event);
				return false;
			}
			//else
			//{
			//	PrvDialListDialSelected(FrmGetActiveForm());
			//}
		}
		else if(globals->gDeviceFlags.bits.treoWithSendKeys && event->data.keyDown.chr == vchrHard11)
		{
			if(!globals->gDialListKeyDown)
			{
				EventType event2;
				MemSet(&event2, sizeof(EventType), 0);
				event2.eType = keyUpEvent;
				event2.data.keyDown.chr = vchrHardRockerCenter;
				event2.data.keyDown.keyCode = vchrHardRockerCenter;
				event2.data.keyDown.modifiers = 8;
				globals->gDialListKeyDown = true;
				EvtAddEventToQueue(&event2);
				//EvtAddEventToQueue(event);
			}
			//else
			//{
			//	PrvDialListDialSelected(FrmGetActiveForm());
			//}
		}
		return true;
	}
		
	if(!globals->gNavigation)
	{
		switch(event->data.keyDown.chr)
		{
			case vchrRockerUp:
				PrvDialListScroll(winUp);
				return true;
				break;
			case vchrRockerDown:
				PrvDialListScroll(winDown);
				return true;
				break;
			case vchrRockerLeft:
				PrvDialListLeaveDialog();
				return true;
				break;
			case vchrRockerRight:
				if (PrvDialListDialSelected(FrmGetActiveForm()))
					PrvDialListLeaveDialog();
				return true;
				break;
			case vchrRockerCenter:
				if (PrvDialListDialSelected(FrmGetActiveForm()))
					PrvDialListLeaveDialog();
				return true;
				break;
		}
	}
	
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event) && !globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Left))
		{
			PrvDialListLeaveDialog();
			return true;
		}
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			if (PrvDialListDialSelected(FrmGetActiveForm()))
				PrvDialListLeaveDialog();
			return true;
		}
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Right))
		{
			if (PrvDialListDialSelected(FrmGetActiveForm()))
				PrvDialListLeaveDialog();
			return true;
		}
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Up) || (event->data.keyDown.chr==vchrPageUp))
		{
			PrvDialListScroll(winUp);
			return true;
		}
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Down) || (event->data.keyDown.chr==vchrPageDown))
		{
			PrvDialListScroll(winDown);
			return true;
		}   	 	
	}  
	return false;
}

Boolean DialListOnUpdate(UInt16 updateCode)
{
	if(updateCode & updatePrefs)
	{
		globalVars* globals = getGlobalsPtr();
		PrvDialListSetFieldHandle(CustomGetObjectPtrSmp(DialListNumberField), 0);
		PrvDialListFreeMemory();
		if(!DialListInitPhoneList(globals->dialListRecordIndex, globals->dialListPhoneIndex, globals->dialListLineIndex))
			return false;
		PrvDialListInit(FrmGetActiveForm());
		FrmDrawForm(FrmGetActiveForm());
	}
	return false;
}

Boolean DialListHandleEvent( EventType * event )
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;

	switch (event->eType)
	{
	case lstSelectEvent:
		handled = DialListOnLstSelectEvent(event);
		break;
	case ctlSelectEvent:
		handled = DialListOnCtlSelectEvent(event);
		break;
	case frmOpenEvent:
		handled = DialListOnFrmOpenEvent();
		break;
	case frmUpdateEvent:
		handled = DialListOnUpdate(event->data.frmUpdate.updateCode);
		break;
	case frmObjectFocusTakeEvent:
		handled = false;
		break;
	case frmCloseEvent:
		handled = DialListOnFrmCloseEvent();
		break;	
	case keyDownEvent:
		handled = DialListOnKeyDownEvent(event);
		break;
	default:
		handled = dia_handle_event(event, NULL);
		break;
	}
	return (handled);
}

#pragma mark -

Boolean PrvDialListCanBePhoneNumber( Char* text, Int16 textLen )
{
	globalVars* globals = getGlobalsPtr();
	UInt16 offset = 0;
	Boolean numbersFound = false;
	while (offset < textLen)
	{
		WChar curChar;
		offset += TxtGetNextChar(text, offset, &curChar);
		if ( StrChr(globals->gPhoneChars, curChar) != NULL )
			numbersFound = true;
		else if(!numbersFound && TxtCharIsAlpha(curChar))
			return false;
	}

	return numbersFound;
}


/***********************************************************************
 *
 * FUNCTION:
 *	PrvDialListPhoneNumberFilter
 *
 * DESCRIPTION:
 *	This routine filter a phone number
 *
 * PARAMETERS:
 *	outString	OUT	filterd phone number
 *	outLen		IN	max text len for outString
 *			 	OUT	phone number len
 *	inString	IN	text to filter
 *	inLen		IN	text len
 *
 * RETURNED:
 *	nothing
 *
 * REVISION HISTORY:
 *	Name		Date		Description
 *	----		----		-----------
 *	aro			06/12/00	Initial Revision
 *	fpa			11/11/00	Fixed a coherency problem with SMS: now, +++123+++456 -> + 123 456
 *
 ***********************************************************************/
void PrvDialListPhoneNumberFilter( Char* outString, Int16* outLen, const Char* inString, Int16 inLen )
{
	globalVars* globals = getGlobalsPtr();
	UInt16 inOffset;
	UInt16 outOffset;
	inOffset = 0;
	outOffset = 0;
	
	while ( (inOffset < inLen) && (outOffset < *outLen) )
	{
		WChar curChar;

		inOffset += TxtGetNextChar(inString, inOffset, &curChar);
		if (StrChr(globals->gPhoneChars, curChar))
		{
			outOffset += TxtSetNextChar(outString, outOffset, curChar);
		}
		else
		{
			if(TxtCharIsAlpha(curChar))
			{
				break;
			}
		}
	}

	TxtSetNextChar(outString, outOffset, chrNull);

	*outLen = outOffset;
}

/***********************************************************************
 *
 * FUNCTION:
 *		PrvDialListAllocStringFrom
 *
 * DESCRIPTION:
 *		This routine build a string from 3 string
 *		It cut after the first lineFeed according to checkLineFeed
 *
 * PARAMETERS:
 *  	s1, s2, s3		IN 	the 3 string that can be null...
 *      checkLineFeed	IN  check lineFeed?
 *
 * RETURNED:
 *		pointer to the string allocated
 *
 * REVISION HISTORY:
 *		Name		Date		Description
 *		----		----		-----------
 *		aro			6/12/00		Initial Revision
 *
 ***********************************************************************/
Char* PrvDialListAllocStringFrom( const Char* s1, const Char* s2, const Char* s3, Boolean checkLineFeed )
{

#define SafeStrLen(string) ( (Int16) ( (string)? StrLen(string) : 0 ) )
#define CopyString(base, string, length) \
	{ if (length) { MemMove(base, string, length + 1); base += length; } } while (false)

	Int16 size = 1;
	Int16 length1;
	Int16 length2;
	Int16 length3;
	Char* string;
	Char* tmpStr;

	// Concatenate all non null & non empty string
	// Cut it at first lineFeed

	length1 = SafeStrLen(s1);
	length2 = SafeStrLen(s2);
	length3 = SafeStrLen(s3);

	size = length1 + length2 + length3 + 1;
	if (size == 1)
		return 0;

	string = MemPtrNew(size);
	if (!string)
		return 0;

	tmpStr = string;
	CopyString(tmpStr, s1, length1);
	CopyString(tmpStr, s2, length2);
	CopyString(tmpStr, s3, length3);

	if (checkLineFeed)
	{
		tmpStr = StrChr(string, chrLineFeed);
		if (tmpStr)
		{
			length1 = tmpStr - string;
			string[length1] = chrNull;
			// Shrink so it can't fail
			MemPtrResize(string, length1 + 1);
		}
	}

	return string;
}

void	PrvDialListSetFieldHandle( FieldType* fldP, MemHandle handle )
{
	MemHandle oldH;
	oldH = FldGetTextHandle(fldP);
	FldSetTextHandle(fldP, handle);
	if (oldH)
	{
		MemHandleFree(oldH);
	}
}

void PrvDialListBuildDescription( void )
{
	globalVars* globals = getGlobalsPtr();
	// Initialize description, sorted by default choice
	// - firstName name
	// - name
	// - firstName
	// - Company
	// Then cut at the first line feed
	if (gAddrP->fields[univFirstName])
	{
		if (gAddrP->fields[name])
		{
			gDisplayName = PrvDialListAllocStringFrom(gAddrP->fields[univFirstName], " ", gAddrP->fields[univName], true);
		}
		else
		{
			// first name only
			gDisplayName = PrvDialListAllocStringFrom(gAddrP->fields[univFirstName], 0, 0, true);
		}
	}
	else
	{
		if (gAddrP->fields[univName])
		{
			// name only
			gDisplayName = PrvDialListAllocStringFrom(gAddrP->fields[univName], 0, 0, true);
		}
		else if (gAddrP->fields[univCompany])
		{
			// company only
			gDisplayName = PrvDialListAllocStringFrom(gAddrP->fields[univCompany], 0, 0, true);
		}
	}
	
	if (!gDisplayName)
	{
		MemHandle unnamedH;
		// - unnamed - (need allocation)
		unnamedH = DmGetResource(strRsc, UnnamedRecordStr);
		gDisplayName = PrvDialListAllocStringFrom(MemHandleLock(unnamedH), 0, 0, true);
		MemHandleUnlock(unnamedH);
	}
}

Boolean PrvDialListHandleDescriptionEvent( struct FormGadgetTypeInCallback *gadgetP, UInt16 cmd, void *paramP )
{
#pragma unused(paramP)

	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	FormType *gadgetFormP;
	UInt16 gadgetIndex;
	RectangleType gadgetBounds;

	switch (cmd)
	{
	case formGadgetDeleteCmd:
		// Free the display Name
		break;
	case formGadgetDrawCmd:
		{
			FontID fontId;

			// Get the bounds of the gadget from the form
			gadgetFormP = FrmGetActiveForm();
			if (gadgetFormP == NULL)
				goto Done;
			gadgetIndex = FrmGetObjectIndexFromPtr(gadgetFormP, gadgetP);
			if ( gadgetIndex == frmInvalidObjectId )
				goto Done;
			FrmGetObjectBounds(gadgetFormP, gadgetIndex, &gadgetBounds);
			// The displayName is left-aligned and truncated to fit in the gadget
			fontId = FntSetFont(largeBoldFont);
			WinDrawTruncChars(gDisplayName, StrLen(gDisplayName), gadgetBounds.topLeft.x, gadgetBounds.topLeft.y,
							  gadgetBounds.extent.x);
			FntSetFont(fontId);
			handled = true;
			break;
		}
	}
Done:
	return handled;
}

void PrvDialListInit( FormType* frmP )
{
	globalVars* globals = getGlobalsPtr();
	ListType*	lstP;
	UInt16		fldIndex;
	FieldType*	fldP;
	const Char*	description = "";
	Coord		dummy;
	Int16		middle;
	Int16		visibleItems;
	Int16		topIndex;
	
	// Build the description for drawing in the form
	PrvDialListBuildDescription();
	FrmSetGadgetHandler(frmP, FrmGetObjectIndex(frmP, DialListDescriptionGadget), PrvDialListHandleDescriptionEvent);

	// Get the gadget phone rectangle position, since phone in the list will be aligned to that
	FrmGetObjectPosition(frmP, FrmGetObjectIndex(frmP, DialListPhoneRectangleGadget),
						 &gPhoneX, &dummy);

	// Initialize the address list.
	lstP = CustomGetObjectPtrSmp(DialListList);
	LstSetListChoices(lstP, 0, gPhonesCount);
	LstSetHeight(lstP, 5);
	LstSetDrawFunction(lstP, PrvDialListDrawPhoneItem);

	// Set the top item to avoid flickering
	// Try to have the selected one in the middle
	visibleItems = LstGetVisibleItems(lstP);
	middle = ((visibleItems - 1) / 2);
	if ((gPhonesCount <= visibleItems) || (gSelectedIndex <= middle))
	{
		// top aligned
		topIndex = 0;
	}
	else if (gSelectedIndex >= (gPhonesCount - (visibleItems - middle)))
	{
		// bottom aligned
		topIndex = gPhonesCount - visibleItems;
	}
	else
	{
		// centered
		topIndex = gSelectedIndex - middle;
	}
	LstSetTopItem(lstP, topIndex);

	// initiate phone number field
	
	fldIndex = FrmGetObjectIndex(frmP, DialListNumberField);
	fldP = FrmGetObjectPtr(frmP, fldIndex);
	FldSetMaxChars(fldP, kMaxCharsPhoneNumber);
	PrvDialListUpdatePhoneNumber(fldP);
	FrmSetFocus(frmP, fldIndex);
	globals->gDialListKeyDown = false;
}

void PrvDialListScroll( WinDirectionType direction )
{
	globalVars* globals = getGlobalsPtr();
	FormType* frmP = FrmGetActiveForm();
	ListPtr lstP = GetObjectPtrSmp(DialListList);
	Int16 count = LstGetVisibleItems(lstP);
	Int16 selection = LstGetSelection(lstP);
	Int16 num = LstGetNumberOfItems(lstP);
	if(selection == 0 && direction == winUp)
		return;
	if(selection == num-1 && direction == winDown)
		return;
	if(direction == winUp)
		selection--;
	else if(direction == winDown)
		selection++;
	gSelectedIndex = selection;
	LstSetSelection(lstP, selection);
	PrvDialListUpdateAfterSelection(frmP);
			
}


void PrvDialListDrawPhoneItem( Int16 index, RectangleType *boundsP, Char **itemsText )
{
#pragma unused(itemsText)
	globalVars* globals = getGlobalsPtr();
	Char* number;
	Char* label;
	Int16 numberLen;
	Int16 labelLen;
	Int16 dummyLen;
	Coord labelMaxWidth;
	Coord dummyWidth;
	Boolean	fit;

	// retrieve the name and the label
	
	number = gPhones[index].number;
	numberLen = gPhones[index].numberLen;
	label = gPhones[index].label;
	labelLen = StrLen(label);
	dummyLen = labelLen;
	
	// Draw the label on the left (truncated if needed) + ":")
	labelMaxWidth = gPhoneX - boundsP->topLeft.x - kSpaceBetweenLabelAndNumber;
	dummyWidth = labelMaxWidth;
	FntCharsInWidth(label, &labelMaxWidth, &dummyLen, &fit);
	WinDrawTruncChars(label, labelLen, boundsP->topLeft.x, boundsP->topLeft.y, dummyWidth);
	WinDrawChars(":", 1, boundsP->topLeft.x + labelMaxWidth + 1, boundsP->topLeft.y);

	WinDrawTruncChars(number, numberLen, gPhoneX, boundsP->topLeft.y,
					  boundsP->extent.x + boundsP->topLeft.x - gPhoneX);
}


/***********************************************************************
 *
 * FUNCTION:
 *	PrvDialListUpdateAfterSelection
 *
 * DESCRIPTION:
 *	This routine update the number
 *	according to the new or current selection
 *	Set focus to the field
 *
 * PARAMETERS:
 *	frmP	IN	form
 *
 * RETURNED:
 *	nothing
 *
 * REVISION HISTORY:
 *	Name		Date		Description
 *	----		----		-----------
 *	aro			6/12/00		Initial Revision
 *
 ***********************************************************************/
void PrvDialListUpdateAfterSelection( FormType* frmP )
{
	FieldType* 	fldP;
	UInt16		fieldIndex;

	// Set the number in the field
	// Number is parse according to characters allowed
	fieldIndex = FrmGetObjectIndex(frmP, DialListNumberField);
	fldP = FrmGetObjectPtr(frmP, fieldIndex);
	PrvDialListUpdatePhoneNumber(fldP);
	FldDrawField(fldP);
	FrmSetFocus(frmP, fieldIndex);
}


/***********************************************************************
 *
 * FUNCTION:
 *	PrvDialListUpdatePhoneNumber
 *
 * DESCRIPTION:
 *	This routine update the number
 *	in the field according to current selection
 *	No drawn is made
 *
 * PARAMETERS:
 *	fldP	IN	field
 *
 * RETURNED:
 *	nothing
 *
 * REVISION HISTORY:
 *	Name		Date		Description
 *	----		----		-----------
 *	aro			6/12/00		Initial Revision
 *
 ***********************************************************************/
void PrvDialListUpdatePhoneNumber( FieldType* fldP )
{
	globalVars* globals = getGlobalsPtr();
	MemHandle	numberH;
	Char*		numberP;
	Int16		len;
	Int16 index = gSelectedIndex;
	RectangleType messageBounds, cancelBounds;
	UInt16 indexMessage = FrmGetObjectIndex(FrmGetActiveForm(), DialListMessageButton);
	UInt16 indexCancel = FrmGetObjectIndex(FrmGetActiveForm(), DialListCancelButton);

	FldSetMaxChars(fldP, 255);
	len = (Int16)FldGetMaxChars(fldP);

	len = min(len, gPhones[index].numberLen);
	
	numberH = MemHandleNew(len + 1);
	if (!numberH)
		return;

	numberP = MemHandleLock(numberH);

	FrmGetObjectBounds(FrmGetActiveForm(), indexMessage, &messageBounds);
	FrmGetObjectBounds(FrmGetActiveForm(), indexCancel, &cancelBounds);
	
	FrmHideObject(FrmGetActiveForm(), indexMessage);
	FrmHideObject(FrmGetActiveForm(), indexCancel);
		
	if(gPhones[index].type == dialtype_email || gPhones[index].type == dialtype_web || gPhones[index].type == dialtype_im || gPhones[index].type == dialtype_map)
	{
		if(cancelBounds.topLeft.x > messageBounds.topLeft.x)
		{
			FrmSetObjectBounds(FrmGetActiveForm(), indexMessage, &cancelBounds);
			FrmSetObjectBounds(FrmGetActiveForm(), indexCancel, &messageBounds);
		}
		
		FrmShowObject(FrmGetActiveForm(), indexCancel);

		StrCopy(numberP, gPhones[index].number);
		MemHandleUnlock(numberH);
	}
	else
	{
		if(cancelBounds.topLeft.x < messageBounds.topLeft.x)
		{
			FrmSetObjectBounds(FrmGetActiveForm(), indexMessage, &cancelBounds);
			FrmSetObjectBounds(FrmGetActiveForm(), indexCancel, &messageBounds);
		}
		
		FrmShowObject(FrmGetActiveForm(), indexMessage);
		FrmShowObject(FrmGetActiveForm(), indexCancel);

		PrvDialListPhoneNumberFilter(numberP, &len, gPhones[index].number, gPhones[index].numberLen);
		numberP[len] = chrNull;
		MemHandleUnlock(numberH);
		MemHandleResize(numberH, len + 1);
	}
	PrvDialListSetFieldHandle(fldP, numberH);
}

void PrvDialListFreeMemory( void )
{
	globalVars* globals = getGlobalsPtr();
	UInt16 i;
	if (gAppInfoP)
		MemPtrUnlock(gAppInfoP);
	
	if (gAddrH)
		MemHandleUnlock(gAddrH);
	
	if (gDisplayName)
		MemPtrFree(gDisplayName);

	if(gPhones && gPhonesCount)
	{
		for(i = 0; i < gPhonesCount; i++)
		{
			if(gPhones[i].number && gPhones[i].allocated)
			{
				MemPtrFree(gPhones[i].number);
				gPhones[i].number = NULL;
			}
		}	
	}
	if (globals->gDialListData)
		MemPtrFree(globals->gDialListData);

	globals->gDialListData = 0;
}

void PrvDialListLeaveDialog( void )
{
	PrvDialListSetFieldHandle(CustomGetObjectPtrSmp(DialListNumberField), 0);
	PrvDialListFreeMemory();

	FrmReturnToForm(0);
}


// check bluetooth status, ask user for enable it
void static PrvDialListBluetoothCheck()
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gAutoBluetooth)
	{
		ToolsEnableBluetooth(globals->adxtLibRef, true);
	}	
	return;
}

Boolean PrvDialListDialSelected( FormType* frmP )
{	
	globalVars* globals = getGlobalsPtr();
	SysNotifyParamType param;
	HelperNotifyEventType details;
	HelperNotifyExecuteType execute;

	Int16 selection = CustomLstGetSelection(DialListList);
	
	switch(gPhones[selection].type)
	{
		case dialtype_email:
			return ConnectContact(FldGetTextPtr(CustomGetObjectPtrSmp(DialListNumberField)), connectoptions_email);
			break;
		case dialtype_web:
			return ConnectContact(FldGetTextPtr(CustomGetObjectPtrSmp(DialListNumberField)), connectoptions_web);
			break;
		case dialtype_im:
			return ConnectContact(FldGetTextPtr(CustomGetObjectPtrSmp(DialListNumberField)), connectoptions_im);
			break;
		case dialtype_map:
			return ConnectMap(gPhones[selection].number, gPhones[selection].number2, gPhones[selection].number3, gPhones[selection].number4, gPhones[selection].number5, gPhones[selection].recID);
			break;
	}
	
	// check, if BT is On
	if(!globals->gDeviceFlags.bits.treo)
		PrvDialListBluetoothCheck();
		
	/*if(globals->gKyocera)
	{
		Err err=0;
		UInt16 coreRefNum;
		err = SysLibFind(PDQCoreLibName,&coreRefNum);
		if (!err)
		{
			err = PDQTelMakeCall(coreRefNum, FldGetTextPtr(CustomGetObjectPtrSmp(DialListNumberField)));
			return (err == errNone);
		}
		else
		{
			return false;
		}
	}*/
	
	param.notifyType = sysNotifyHelperEvent;
	param.broadcaster = sysFileCAddress;
	param.notifyDetailsP = &details;
	param.handled = false;

	details.version = kHelperNotifyCurrentVersion;
	details.actionCode = kHelperNotifyActionCodeExecute;
	details.data.executeP = &execute;

	execute.serviceClassID = kHelperServiceClassIDVoiceDial;
	execute.helperAppID = 0;
	execute.dataP = FldGetTextPtr(CustomGetObjectPtrSmp(DialListNumberField));
	execute.displayedName = gDisplayName;
	execute.detailsP = 0;
	execute.err = errNone;

	SysNotifyBroadcast(&param);
	
	dia_restore_state();

	// Check error code
	if (!param.handled)
		// Not handled so exit the list - Unexepcted error
		return true;
	else
	{
		return (execute.err == errNone);
	}
	return true;
}

Boolean PrvDialListMessageSelected( FormType* frmP )
{
	globalVars* globals = getGlobalsPtr();
	HelperNotifyEventType param;
	HelperNotifyExecuteType execute;
	SysNotifyParamType notifyParam;
	Err err = errNone;
	MemSet(&param, sizeof(HelperNotifyEventType), 0);
	param.version = kHelperNotifyCurrentVersion;
	param.actionCode = kHelperNotifyActionCodeExecute;
	execute.helperAppID = 0;
	execute.serviceClassID = kHelperServiceClassIDSMS;
	execute.dataP = FldGetTextPtr(CustomGetObjectPtrSmp(DialListNumberField));
	execute.displayedName = gDisplayName;
	execute.detailsP = NULL;
	param.data.executeP = &execute;
	MemSet (&notifyParam, sizeof(SysNotifyParamType), 0);
	notifyParam.broadcaster = CREATORID;
	notifyParam.notifyType = sysNotifyHelperEvent;
	notifyParam.notifyDetailsP = &param;
	err = SysNotifyBroadcast(&notifyParam);
	return true;
}