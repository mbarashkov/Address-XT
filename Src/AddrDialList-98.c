/******************************************************************************
 *
 * Copyright (c) 1995-2002 PalmSource, Inc. All rights reserved.
 *
 * File: AddrDialList.c
 *
 * Release: Palm OS 5 SDK (68K) R3.
 *
 * Description:
 *  This is the Address Book application's dial list form module.
 *
 *****************************************************************************/

#include <Chars.h>
#include <ErrorMgr.h>
#include <NotifyMgr.h>
#include <StringMgr.h>
#include <SysUtils.h>
#include <UIResources.h>
#include <DataMgr.h>
#include <LocaleMgr.h>
#include <Form.h>
#include <Helper.h>
#include <HelperServiceClass.h>
#include <TextMgr.h>
#include <PalmUtils.h>
#include <BtLib.h>

#include "AddrDialList.h"
#include "Address.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "AddressRsc.h"
#include "AddressDB.h"
#include "AddrDefines.h"
#include "PalmContactsDB/ContactsDB.h"
#include "globals.h"
#include "dia.h"
#include "syslog.h"

#define kSpaceBetweenLabelAndNumber 6
#define kLeftAndRightSpace			2
#define	kMaxPhonesCount				25
#define kMaxCharsPhoneNumber		25

// Convenient access
#define gAddrP			(&(gDialListData->addr))
#define gAddrContactP	(&(gDialListData->contact))
#define gAddrH			(gDialListData->addrH)
#define gAppInfoP		(gDialListData->appInfoP)
#define gContactsInfoP	(gDialListData->contactsInfoP)
#define gDisplayName	(gDialListData->displayName)
#define gPhones			(gDialListData->phones)
#define gPhonesCount	(gDialListData->phonesCount)
#define gPhoneX			(gDialListData->phoneX)
#define gSelectedIndex	(gDialListData->selectedIndex)

Boolean gDialListKeyDown;
/***********************************************************************
 *
 *	Internal types
 *
 ***********************************************************************/

#define DIALTYPE_PHONE 0
#define DIALTYPE_EMAIL 1

typedef struct DialListPhoneTag
{
	Char*		label;
	Char*		number;
	Int16		numberLen;
	Int16		type;
} DialListPhoneType;

typedef struct DialListDataTag
{
	// Record
	MemHandle 			addrH;

	// Temp only accurate when drawing
	AddrDBRecordType	addr;
	AddrAppInfoType*	appInfoP;
	
	P1ContactsDBRecordType contact;
	P1ContactsAppInfoType*	contactsInfoP;
	
	// Record description - allocated
	Char*				displayName;

	// Phone position - got from field position so that localization
	// can enable various position
	// Label will be aligned right + delta to that
	Coord				phoneX;
	Coord				phoneY;
	// X min is got from te Description label
	Coord				displayXMin;
	Coord				displayNameY;

	// list info
	Int16				topIndex;
	Int16				selectedIndex;

	// Array of data
	DialListPhoneType	phones[kMaxPhonesCount];
	UInt16				phonesCount;
} DialListDataType;


/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/

static DialListDataType* gDialListData;
static const Char gPhoneChars[] = "0123456789,+#*";


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

Boolean 		PrvDialListMessageSelected( FormType* frmP );
static void		PrvDialListInit( FormType* frmP );
static void		PrvDialListScroll( WinDirectionType direction );
static void		PrvDialListDrawPhoneItem( Int16 index, RectangleType *boundsP, Char **itemsText );
static void		PrvDialListUpdateAfterSelection( FormType* frmP );
static void		PrvDialListUpdatePhoneNumber( FieldType* fldP );
static void		PrvDialListFreeMemory( void );
static void		PrvDialListLeaveDialog( void );
static Boolean	PrvDialListDialSelected( FormType* frmP );

static UInt16	PrvGetFieldLabel(UInt16 field, void* rec)
{
	UInt16 label;
	if(Contacts)
	{
		switch(field)
		{
			case P1Contactsphone1:
				label = (AddressPhoneLabels)(((P1ContactsDBRecordPtr)rec)->options.phones.phone1);
				break;
			case P1Contactsphone2:
				label = (AddressPhoneLabels)(((P1ContactsDBRecordPtr)rec)->options.phones.phone2);
				break;
			case P1Contactsphone3:
				label = (AddressPhoneLabels)(((P1ContactsDBRecordPtr)rec)->options.phones.phone3);
				break;
			case P1Contactsphone4:
				label = (AddressPhoneLabels)(((P1ContactsDBRecordPtr)rec)->options.phones.phone4);
				break;
			case P1Contactsphone5:
				label = (AddressPhoneLabels)(((P1ContactsDBRecordPtr)rec)->options.phones.phone5);
				break;
			case P1Contactsphone6:
				label = (AddressPhoneLabels)(((P1ContactsDBRecordPtr)rec)->options.phones.phone6);
				break;
			case P1Contactsphone7:
				label = (AddressPhoneLabels)(((P1ContactsDBRecordPtr)rec)->options.phones.phone7);
				break;			
		}
	}
	else
	{
		switch (field)
		{
		case phone1:
			label = (AddressPhoneLabels)(((AddrDBRecordPtr)rec)->options.phones.phone1);
			break;
		case phone2:
			label = (AddressPhoneLabels)(((AddrDBRecordPtr)rec)->options.phones.phone2);
			break;
		case phone3:
			label = (AddressPhoneLabels)(((AddrDBRecordPtr)rec)->options.phones.phone3);
			break;
		case phone4:
			label = (AddressPhoneLabels)(((AddrDBRecordPtr)rec)->options.phones.phone4);
			break;
		case phone5:
			label = (AddressPhoneLabels)(((AddrDBRecordPtr)rec)->options.phones.phone5);
			break;
		}
	}
	return label;
}

Boolean AddressDialable(UInt16 recordIndex, UInt16 phoneIndex)
{
	Err			err;
	Int16		fieldIndex;
	DialListPhoneType	phones[kMaxPhonesCount];
	UInt16 phonesCount;
	
	// Get the current record
	AddrDBRecordType rec;
	P1ContactsDBRecordType contactsRec;
	MemHandle mH;
	if(!Contacts)
	{
		err = AddrDBGetRecord(AddrDB, recordIndex, &rec, &mH);
		if (err)
			goto exit;
	}
	else
	{
		err = PrvP1ContactsDBGetRecord(AddrDB, recordIndex, &contactsRec, &mH);
		if (err)
			goto exit;	
	}
	// Check default phone index
	// If type is not a supported one, continue
	if (phoneIndex == kDialListShowInListPhoneIndex)
		phoneIndex = rec.options.phones.displayPhoneForList;

	// Build the phone array
	// Splitting each phone field per line
	phonesCount = 0;
	
	if(!Contacts)
	{
	
		for (fieldIndex = firstPhoneField; fieldIndex <= lastPhoneField; fieldIndex++)
		{
			Char* text = rec.fields[fieldIndex];
			Char* next = text;
			Int16 length;
			Int16 phoneLineIndex = 0;
			Boolean email = false;
			UInt16 label;
			
			if (text && ToolsIsPhoneIndexSupported(&rec, fieldIndex - firstPhoneField))
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
					
					if (PrvDialListCanBePhoneNumber(text, length) || (label == emailLabel && StrLen(text) >0 && StrChr(text, '@') != NULL))
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
	}	
	else
	{
		for (fieldIndex = P1ContactsfirstPhoneField; fieldIndex<P1ContactslastPhoneField; fieldIndex++)
		{
			Char* text;
			Char* next;
			Int16 length;
			Int16 phoneLineIndex = 0;
			UInt16 label;
			
			text = contactsRec.fields[fieldIndex];
			next = text;
			if(text && ToolsIsPhoneIndexSupported(&contactsRec, fieldIndex - P1ContactsfirstPhoneField))
			{
				label = PrvGetFieldLabel(fieldIndex, &contactsRec);					
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
					if (PrvDialListCanBePhoneNumber(text, length) || (label == P1ContactsemailLabel && StrLen(text) >0 && StrChr(text, '@') != NULL))
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
	
	gDialListData = 0;
	return false;

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
	Err			err;
	Int16		fieldIndex;
	AddressPhoneLabels label;
		
	gDialListData = MemPtrNew(sizeof(DialListDataType));
	if (!gDialListData)
		return false;
	MemSet(gDialListData, sizeof(DialListDataType), 0);

	// Get the current record
	if(!Contacts)
	{
		err = AddrDBGetRecord(AddrDB, recordIndex, gAddrP, &gAddrH);
	}
	else
	{
		err = PrvP1ContactsDBGetRecord(AddrDB, recordIndex, gAddrContactP, &gAddrH);
	}
	if (err)
		goto exit;
	
	// Check default phone index
	// If type is not a supported one, continue
	if (phoneIndex == kDialListShowInListPhoneIndex)
		phoneIndex = gAddrP->options.phones.displayPhoneForList;

	if(!Contacts)
		gAppInfoP = (AddrAppInfoType*)AddrDBAppInfoGetPtr(AddrDB);
	else
		gContactsInfoP = (P1ContactsAppInfoType*)P1ContactsDBAppInfoGetPtr(AddrDB);
	// Build the phone array
	// Splitting each phone field per line
	gPhonesCount = 0;
	if(!Contacts)
	{
		for (fieldIndex = firstPhoneField; fieldIndex <= lastPhoneField; fieldIndex++)
		{
			Char* text = gAddrP->fields[fieldIndex];
			Char* next = text;
			Int16 length;
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

					// Check that the phone is a phone number (ie at least one phone character)
					if ((PrvDialListCanBePhoneNumber(text, length) && label != emailLabel) || (label == emailLabel && ToolsVersaMailPresent()))
					{
						Int16 phoneLabelIndex;

						gPhones[gPhonesCount].number = text;
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
							gPhones[gPhonesCount].type = DIALTYPE_EMAIL;
						
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
	}	
	else
	{
		for (fieldIndex = P1ContactsfirstPhoneField; fieldIndex <= P1ContactslastPhoneField; fieldIndex++)
		{
			Char* text;
			Char* next;
			Int16 length;
			Int16 phoneLineIndex = 0;
			
			text = gAddrContactP->fields[fieldIndex];
			
			label = PrvGetFieldLabel(fieldIndex, gAddrContactP);
			
			
			if (text)
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

					// Check that the phone is a phone number (ie at least one phone character)
					if ((PrvDialListCanBePhoneNumber(text, length) && label != P1ContactsemailLabel) || (label == P1ContactsemailLabel && ToolsVersaMailPresent()))
					{
						Int16 phoneLabelIndex;

						gPhones[gPhonesCount].number = text;
						gPhones[gPhonesCount].numberLen = length;
						if(label == P1ContactsemailLabel)
							gPhones[gPhonesCount].type = DIALTYPE_EMAIL;
						else
							gPhones[gPhonesCount].type = DIALTYPE_PHONE;
						
						phoneLabelIndex = P1ContactsGetPhoneLabel(gAddrContactP, fieldIndex); // 0 = Work... 7 = Mobile
						// gAppInfoP->fieldLabels stored name of fields in a strange way
						if (phoneLabelIndex < 7) // Work, Home, Fax, Other, Email are stored as phone1... phone5
							phoneLabelIndex += P1Contactsphone1;
						else	// Main, Pager and Mobile are stored after Note.
							phoneLabelIndex += P1Contactsphone1 + 29;
						gPhones[gPhonesCount].label = gContactsInfoP->fieldLabels[phoneLabelIndex];

						// Is is the selected one
						if ((phoneIndex == fieldIndex - P1ContactsfirstPhoneField) && (phoneLineIndex == lineIndex))
							gSelectedIndex = gPhonesCount;

						gPhonesCount++;
						
						phoneLabelIndex = GetPhoneLabel(gAddrContactP, fieldIndex); // 0 = Work... 7 = Mobile
						
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
	}
	
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
	gSelectedIndex = event->data.lstSelect.selection;
	PrvDialListUpdateAfterSelection(FrmGetActiveForm());
	return true;
}

static Boolean DialListOnCtlSelectEvent(EventType* event)
{
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
	case DialListSMSButton:
		if (PrvDialListMessageSelected(FrmGetActiveForm()))
			PrvDialListLeaveDialog();
		handled = true;
		break;
	case DialListDialButton:
		if (PrvDialListDialSelected(FrmGetActiveForm()))
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
	FormPtr frmP = FrmGetActiveForm();
	dia_save_state(); 
	dia_enable(frmP, false);
	PrvDialListInit(frmP);
	FrmDrawForm(frmP);
	LstSetSelection(ToolsGetFrmObjectPtr(frmP, DialListList), gSelectedIndex);
	if(gNavigation)
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
	PrvDialListSetFieldHandle(ToolsGetObjectPtr(DialListNumberField), 0);
	PrvDialListFreeMemory();
	return false;
}

static Boolean DialListOnKeyDownEvent(EventType* event)
{
	if(TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
	{
		if(gTreo && event->data.keyDown.chr == vchrHard1)
		{
			if(!gDialListKeyDown)
			{
				EventType event2;
				MemSet(&event2, sizeof(EventType), 0);
				event2.eType = keyUpEvent;
				event2.data.keyDown.chr = vchrHardRockerCenter;
				event2.data.keyDown.keyCode = vchrHardRockerCenter;
				event2.data.keyDown.modifiers = 8;
				gDialListKeyDown = true;
				EvtAddEventToQueue(&event2);
				EvtAddEventToQueue(event);
			}
			else
			{
				PrvDialListDialSelected(FrmGetActiveForm());
			}
		}
		return true;
	}
		
	if(!gNavigation)
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
	if(IsFiveWayNavEvent(event) && !gNavigation)
	{
		if (NavKeyPressed(event, Left))
		{
			PrvDialListLeaveDialog();
			return true;
		}
		if (NavKeyPressed(event, Select))
		{
			if (PrvDialListDialSelected(FrmGetActiveForm()))
				PrvDialListLeaveDialog();
			return true;
		}
		if (NavKeyPressed(event, Right))
		{
			if (PrvDialListDialSelected(FrmGetActiveForm()))
				PrvDialListLeaveDialog();
			return true;
		}
		if (NavKeyPressed(event, Up) || (event->data.keyDown.chr==vchrPageUp))
		{
			PrvDialListScroll(winUp);
			return true;
		}
		if (NavKeyPressed(event, Down) || (event->data.keyDown.chr==vchrPageDown))
		{
			PrvDialListScroll(winDown);
			return true;
		}   	 	
	}  
	else if (EvtKeydownIsVirtual(event))
	{
		switch(event->data.keyDown.chr)
		{
			case vchrJogUp:
				PrvDialListScroll (winUp);
				return true;
				break;		
			case vchrPageUp:
				PrvDialListScroll (winUp);
				return true;
				break;		
			case vchrJogDown:
				PrvDialListScroll (winDown);
				return true;
				break;		
			case vchrPageDown:
				PrvDialListScroll (winDown);
				return true;
				break;		
			case vchrJogLeft:
				PrvDialListLeaveDialog();
				return true;
				break;		
		}
	}
	return false;
}

Boolean DialListHandleEvent( EventType * event )
{
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
	case frmObjectFocusTakeEvent:
		handled = false;
		break;
	case frmCloseEvent:
		handled = DialListOnFrmCloseEvent();
		break;	
	case keyDownEvent:
		handled = DialListOnKeyDownEvent(event);
		break;
	/*
	case winDisplayChangedEvent:
		dia_display_changed();
		FrmUpdateForm(DialListDialog, frmRedrawUpdateCode);
		handled = true;
		break;	
	*/
	default:
		handled = dia_handle_event(event, NULL);
		break;
	}
	return (handled);
}

#pragma mark -

/***********************************************************************
 *
 * FUNCTION:
 *	PrvDialListCanBePhoneNumber
 *
 * DESCRIPTION:
 *	This routine check if a text could be a phone number
 *	ie if it contains phone chars
 *
 * PARAMETERS:
 *	text	IN	text string to parse
 *	textLen	IN	text len
 *
 * RETURNED:
 *	true if acceptable
 *
 * REVISION HISTORY:
 *	Name		Date		Description
 *	----		----		-----------
 *	aro			6/12/00		Initial Revision
 *	kwk			07/26/00	Modified to use Text Mgr, avoid sign extension
 *							problem calling StrChr with signed Char value.
 *
 ***********************************************************************/
Boolean PrvDialListCanBePhoneNumber( Char* text, Int16 textLen )
{
	UInt16 offset = 0;

	while (offset < textLen)
	{
		WChar curChar;
		offset += TxtGetNextChar(text, offset, &curChar);
		if ( StrChr(gPhoneChars, curChar) != NULL )
			return true;
	}

	return false;
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
	UInt16 inOffset;
	UInt16 outOffset;
	Boolean fLastWasSpace;

	inOffset = 0;
	outOffset = 0;
	fLastWasSpace = false;

	while ( (inOffset < inLen) && (outOffset < *outLen) )
	{
		WChar curChar;

		inOffset += TxtGetNextChar(inString, inOffset, &curChar);
		if (StrChr(gPhoneChars, curChar))
		{
			// Only + at the beginning
			if ( (curChar == chrPlusSign) && (outOffset > 0) )
			{
				outOffset += TxtSetNextChar(outString, outOffset, chrSpace);
				fLastWasSpace = true;
			}
			else
			{
				outOffset += TxtSetNextChar(outString, outOffset, curChar);
				fLastWasSpace = false;
			}
		}
		else if ( !fLastWasSpace && (outOffset > 0) )	// No space at the beginning
		{
			outOffset += TxtSetNextChar(outString, outOffset, chrSpace);
			fLastWasSpace = true;
		}
	}

	// No space at the end
	if (fLastWasSpace)
		outOffset--;

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


/***********************************************************************
 *
 * FUNCTION:
 *	Safe set field handle, previous one is freed if needed
 *
 * DESCRIPTION:
 *	Safe set field handle, previous one is freed if needed
 *
 * PARAMETERS:
 *	fldP	IN	field
 *	handle	IN	handle
 *
 * RETURNED:
 *	nothing
 *
 * REVISION HISTORY:
 *	Name		Date		Description
 *	----		----		-----------
 *	aro			8/3/00		Initial Revision
 *
 ***********************************************************************/
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


/***********************************************************************
 *
 * FUNCTION:
 *	PrvDialListBuildDescription
 *
 * DESCRIPTION:
 *	This routine build the description
 *
 * PARAMETERS:
 *	none
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
void PrvDialListBuildDescription( void )
{
	// Initialize description, sorted by default choice
	// - firstName name
	// - name
	// - firstName
	// - Company
	// Then cut at the first line feed
	if(!Contacts)
	{
		if (gAddrP->fields[firstName])
		{
			if (gAddrP->fields[name])
			{
				gDisplayName = PrvDialListAllocStringFrom(gAddrP->fields[firstName], " ", gAddrP->fields[name], true);
			}
			else
			{
				// first name only
				gDisplayName = PrvDialListAllocStringFrom(gAddrP->fields[firstName], 0, 0, true);
			}
		}
		else
		{
			if (gAddrP->fields[name])
			{
				// name only
				gDisplayName = PrvDialListAllocStringFrom(gAddrP->fields[name], 0, 0, true);
			}
			else if (gAddrP->fields[company])
			{
				// company only
				gDisplayName = PrvDialListAllocStringFrom(gAddrP->fields[company], 0, 0, true);
			}
		}
	}
	else
	{
		if (gAddrContactP->fields[P1ContactsfirstName])
		{
			if (gAddrContactP->fields[P1Contactsname])
			{
				gDisplayName = PrvDialListAllocStringFrom(gAddrContactP->fields[P1ContactsfirstName], " ", gAddrContactP->fields[P1Contactsname], true);
			}
			else
			{
				// first name only
				gDisplayName = PrvDialListAllocStringFrom(gAddrContactP->fields[P1ContactsfirstName], 0, 0, true);
			}
		}
		else
		{
			if (gAddrContactP->fields[P1Contactsname])
			{
				// name only
				gDisplayName = PrvDialListAllocStringFrom(gAddrContactP->fields[P1Contactsname], 0, 0, true);
			}
			else if (gAddrContactP->fields[P1Contactscompany])
			{
				// company only
				gDisplayName = PrvDialListAllocStringFrom(gAddrContactP->fields[P1Contactscompany], 0, 0, true);
			}
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


/***********************************************************************
 *
 * FUNCTION:
 *	PrvDialListHandleDescriptionEvent
 *
 * DESCRIPTION:
 *	This routine handle gadget event for descrption (mainly drawing)
 *
 * PARAMETERS
 *	gadgetP	IN  gadget pointer
 *	cmd		IN	command
 *	paramP	IN	param (unused)
 *
 * RETURNED:
 *	nothing
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	aro		08/02/00	Initial Revision
 *
 ***********************************************************************/
Boolean PrvDialListHandleDescriptionEvent( struct FormGadgetTypeInCallback *gadgetP, UInt16 cmd, void *paramP )
{
#pragma unused(paramP)

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


/***********************************************************************
 *
 * FUNCTION:    PrvDialListInit
 *
 * DESCRIPTION: This routine initializes the "Dial List" dialog of the
 *              Address application.
 *
 * PARAMETERS:  frmP  - a pointer to the Form
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			aro			6/12/00		Initial Revision
 *
 ***********************************************************************/
void PrvDialListInit( FormType* frmP )
{
	ListType*	lstP;
	UInt16		fldIndex;
	FieldType*	fldP;
	const Char*	description = "";
	Coord		dummy;
	Int16		middle;
	Int16		visibleItems;
	Int16		topIndex;
	Boolean		palmSMS = false;

	// Build the description for drawing in the form
	PrvDialListBuildDescription();
	FrmSetGadgetHandler(frmP, FrmGetObjectIndex(frmP, DialListDescriptionGadget), PrvDialListHandleDescriptionEvent);

	// Get the gadget phone rectangle position, since phone in the list will be aligned to that
	FrmGetObjectPosition(frmP, FrmGetObjectIndex(frmP, DialListPhoneRectangleGadget),
						 &gPhoneX, &dummy);

	// Initialize the address list.
	lstP = ToolsGetFrmObjectPtr(frmP, DialListList);
	LstSetListChoices(lstP, 0, gPhonesCount);
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
	gDialListKeyDown = false;
	
	if(!gTreo)
	{
		UInt16 cardNo;
		DmSearchStateType searchState;
		LocalID localID;
		if(DmGetNextDatabaseByTypeCreator(true, &searchState, 'appl', 'smsm', false, &cardNo, &localID) == errNone)
			palmSMS = true;		
	}
	
	if(gTreo || palmSMS)
	{
		RectangleType rect1, rect2, rect3;
		fldIndex = FrmGetObjectIndex(frmP, DialListCancelButton);
		FrmHideObject(frmP, fldIndex);
		fldIndex = FrmGetObjectIndex(frmP, DialListCancelButton);
		FrmGetObjectBounds(frmP, fldIndex, &rect1);
		fldIndex = FrmGetObjectIndex(frmP, DialListSMSButton);
		FrmGetObjectBounds(frmP, fldIndex, &rect2);
		rect3 = rect1;
		rect1.topLeft.x = rect2.topLeft.x + rect2.extent.x - rect1.extent.x;
		rect1.topLeft.y = rect2.topLeft.y;
		rect2.topLeft.x = rect3.topLeft.x;
		rect2.topLeft.y = rect3.topLeft.y;
		fldIndex = FrmGetObjectIndex(frmP, DialListCancelButton);
		FrmSetObjectBounds(frmP, fldIndex, &rect1);
		fldIndex = FrmGetObjectIndex(frmP, DialListSMSButton);
		FrmSetObjectBounds(frmP, fldIndex, &rect2);
		fldIndex = FrmGetObjectIndex(frmP, DialListCancelButton);
		FrmShowObject(frmP, fldIndex);
		fldIndex = FrmGetObjectIndex(frmP, DialListSMSButton);
		FrmShowObject(frmP, fldIndex);
	}
}


/***********************************************************************
 *
 * FUNCTION:
 *	PrvDialListScroll
 *
 * DESCRIPTION:
 *	This routine scroll the list up or down (page per page)
 *
 * PARAMETERS
 *	direction	IN	direction to scroll (winUp or winDown)
 *
 * RETURNED:
 *	nothing
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	aro		06/19/00	Initial Revision
 *
 ***********************************************************************/
void PrvDialListScroll( WinDirectionType direction )
{
	FormType* frmP = FrmGetActiveForm();
	ListType* lstP = ToolsGetFrmObjectPtr(FrmGetActiveForm(), DialListList);
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


/***********************************************************************
 *
 * FUNCTION:
 *	PrvDialListDrawPhoneItem
 *
 * DESCRIPTION:
 *	This routine draws a phone item line (label & number)
 *	It is called as a callback routine by the list object.
 *
 * PARAMETERS:
 *	itenNum		IN	index of the item to draw
 *	boundsP		IN	boundsP of rectangle to draw in
 *	itemsText	IN	data if any
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *	Name		Date		Description
 *	----		----		-----------
 *	aro			6/12/00		Initial Revision
 *
 ***********************************************************************/
void PrvDialListDrawPhoneItem( Int16 index, RectangleType *boundsP, Char **itemsText )
{
#pragma unused(itemsText)

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
	MemHandle	numberH;
	Char*		numberP;
	Int16		len;
	Int16 index = gSelectedIndex;
	
	len = (Int16)FldGetMaxChars(fldP);

	len = min(len, gPhones[index].numberLen);

	numberH = MemHandleNew(len + 1);
	if (!numberH)
		return;

	numberP = MemHandleLock(numberH);
	if(gPhones[index].type == DIALTYPE_EMAIL)
		StrCopy(numberP, gPhones[index].number);
	else
		PrvDialListPhoneNumberFilter(numberP, &len, gPhones[index].number, gPhones[index].numberLen);
	numberP[len] = chrNull;
	MemHandleUnlock(numberH);
	MemHandleResize(numberH, len + 1);

	PrvDialListSetFieldHandle(fldP, numberH);
}


/***********************************************************************
 *
 * FUNCTION:
 *	PrvDialListFreeMemory
 *
 * DESCRIPTION:
 *	This routine frees memory allocated by the dialog
 *
 * PARAMETERS:
 *	none
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
void PrvDialListFreeMemory( void )
{
	if(Contacts)
	{
		if (gContactsInfoP)
			MemPtrUnlock(gContactsInfoP);
	
	}
	else
	{
		if (gAppInfoP)
			MemPtrUnlock(gAppInfoP);
	}
	if (gAddrH)
		MemHandleUnlock(gAddrH);
	
	if (gDisplayName)
		MemPtrFree(gDisplayName);

	if (gDialListData)
		MemPtrFree(gDialListData);

	gDialListData = 0;
}


/***********************************************************************
 *
 * FUNCTION:
 *	PrvDialListLeaveDialog
 *
 * DESCRIPTION:
 *	This routine leave the dialog
 *
 * PARAMETERS:
 *	none
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
void PrvDialListLeaveDialog( void )
{
	PrvDialListSetFieldHandle(ToolsGetObjectPtr(DialListNumberField), 0);
	PrvDialListFreeMemory();

	FrmReturnToForm(0);
	//FrmUpdateForm(0, updateSelectCurrentRecord);
}


// check bluetooth status, ask user for enable it
void static PrvDialListBluetoothCheck()
{
	//UInt32 btVersion;
	//UInt16 btLibRefNum; 
	//Err error = errNone;
	//Boolean is_loaded = false;

	if(gAutoBluetooth)
	{
		/*
		error = FtrGet(btLibFeatureCreator, btLibFeatureVersion, &btVersion);
		if(error != errNone) return;

		if(SysLibFind(btLibName, &btLibRefNum)) { 
			error = SysLibLoad(sysFileTLibrary, sysFileCBtLib, &btLibRefNum);
			is_loaded = true;
		}
		
		if(error != errNone)
			return;

		error = BtLibOpen(btLibRefNum, false);
		if(error == errNone)
		{
			return;
		}
		
		BtLibClose(btLibRefNum);
		if(is_loaded)
			SysLibRemove(btLibRefNum);
		*/
		
		ToolsEnableBluetooth(true);
	}	
	return;
}

/***********************************************************************
 *
 * FUNCTION:
 *	PrvDialListDialSelected
 *
 * DESCRIPTION:
 *	This routine dial selected number
 *
 * PARAMETERS:
 *	none
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
Boolean PrvDialListDialSelected( FormType* frmP )
{
	
	SysNotifyParamType param;
	HelperNotifyEventType details;
	HelperNotifyExecuteType execute;

	ListType* lstP = ToolsGetFrmObjectPtr(FrmGetActiveForm(), DialListList);
	Int16 selection = LstGetSelection(lstP);
	
	if(gPhones[selection].type == DIALTYPE_EMAIL)
	{
		return EMailContact(FldGetTextPtr(ToolsGetFrmObjectPtr(frmP, DialListNumberField)));
	}
	
	// check, if BT is On
	if(!gTreo)
		PrvDialListBluetoothCheck();
		
	param.notifyType = sysNotifyHelperEvent;
	param.broadcaster = sysFileCAddress;
	param.notifyDetailsP = &details;
	param.handled = false;

	details.version = kHelperNotifyCurrentVersion;
	details.actionCode = kHelperNotifyActionCodeExecute;
	details.data.executeP = &execute;

	execute.serviceClassID = kHelperServiceClassIDVoiceDial;
	execute.helperAppID = 0;
	execute.dataP = FldGetTextPtr(ToolsGetFrmObjectPtr(frmP, DialListNumberField));
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
	/*ListType* lstP = ToolsGetFrmObjectPtr(FrmGetActiveForm(), DialListList);
	Int16 selection = LstGetSelection(lstP);
	UInt16 cardNo;
	LocalID dbID;
	Err err;
	DmSearchStateType searchState;
	PhoneAppLaunchCmdDialPtr paramsP = NULL;
	UInt16 size = sizeof(PhoneAppLaunchCmdDialType);
	Char* numberP =FldGetTextPtr(ToolsGetFrmObjectPtr(frmP, DialListNumberField));
	// Setup a parameter block so the Phone application pre-fills
	// a phone number in the Dial Pad number field
	if (numberP)
	{
		size += StrLen(numberP) + 1;
	}
	paramsP = MemPtrNew (size);
	MemSet (paramsP, size, 0);
	paramsP->version = 1;
	paramsP->failLaunchCreator = CREATORID;
	if (numberP)
	{
		paramsP->number = MemPtrNew (StrLen(numberP) + 1);
		StrCopy(paramsP->number, numberP);
		MemPtrSetOwner (paramsP->number, 0);
	}
	MemPtrSetOwner (paramsP, 0);
	DmGetNextDatabaseByTypeCreator ( true,
		&searchState,
		sysFileTApplication,
		hsFileCPhone,
		true,
		&cardNo,
		&dbID);
	err = SysUIAppSwitch( cardNo,
		dbID,
		phoneAppLaunchCmdViewKeypad,
		paramsP);*/

	return true;
}

Boolean PrvDialListMessageSelected( FormType* frmP )
{
	HelperNotifyEventType param;
	HelperNotifyExecuteType execute;
	SysNotifyParamType notifyParam;
	Err err = errNone;

	ListType* lstP = ToolsGetFrmObjectPtr(FrmGetActiveForm(), DialListList);
	Int16 selection = LstGetSelection(lstP);

	if(gPhones[selection].type == DIALTYPE_EMAIL)
	{
		return EMailContact(FldGetTextPtr(ToolsGetFrmObjectPtr(frmP, DialListNumberField)));
	}	

	MemSet(&param, sizeof(HelperNotifyEventType), 0);
	param.version = kHelperNotifyCurrentVersion;
	param.actionCode = kHelperNotifyActionCodeExecute;
	execute.helperAppID = 0; // Setting the helperAppID to 0 means
	// "use default helper"
	execute.serviceClassID = kHelperServiceClassIDSMS;
	execute.dataP = FldGetTextPtr(ToolsGetFrmObjectPtr(frmP, DialListNumberField));
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
