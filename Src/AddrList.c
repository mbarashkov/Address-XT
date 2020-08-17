#include "AddrList.h"
#include "AddrList2.h"

/***********************************************************************
 *
 *   Defines
 *
 ***********************************************************************/

// Address list table columns
#define nameAndNumColumn					0
#define noteColumn							1

// Scroll rate values
#define scrollDelay							2
#define scrollAcceleration					2
#define scrollSpeedLimit					5

// T3 slider opened (we want hide DIA?)
#define vchrT3SliderOpen					0x505

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/

void			SetCategoryLabel(UInt16 category, Boolean changeCat);
Boolean			ListOnUpdate(EventType* event);
Boolean			ListOnOpen();
static Boolean 	PrvListDialCurrentRecord(UInt16 recordNum);

static void		PrvListSelectRecord( FormType* frmP, UInt16 recordNum, Boolean forceSelection, Boolean refresh);
static Boolean	PrvListScroll( WinDirectionType direction, UInt16 units, Boolean byLine );
static void		PrvListResetScrollRate(void);
static void		PrvListAdjustScrollRate(void);
static UInt16	PrvListLookupString (EventType * event);
static void		PrvListDrawRecord (void * table, Int16 row, Int16 column, RectanglePtr bounds);
static UInt16	PrvListNumberOfRows (TablePtr table);
static void		PrvListUpdateScrollButtons( FormType* frmP );
static UInt16	PrvListSelectCategory (void);
static void		PrvListNextCategory (void);
static Boolean	PrvListDoCommand (UInt16 command);
static Boolean	PrvListHandleRecordSelection( EventType* event );
static void		PrvListReplaceTwoColors (const RectangleType *rP, UInt16 cornerDiam, IndexedColorType oldForeground, IndexedColorType oldBackground, IndexedColorType newForeground, IndexedColorType newBackground);
void 			CstAppStop();
Boolean 		CstSelUpDown(Int16 direction, Boolean clearLookup);
void 			CstGoToCurrent();
Boolean 		AddrListMoveObjects(FormType* frmP, Coord dx, Coord dy);
void 			AddLookupLetter();
Boolean 		RefreshLookup (Boolean refresh);
Boolean 		RefreshLookupEx (Boolean refresh, WinDirectionType direction);
void 			AddRemoveLetter();
UInt16 			LookupLength();
Boolean 		LookupLetter(WinDirectionType direction, Boolean recursive);
int 			PrepareRecentList();
void 			CleanRecentList(UInt16 i);
void 			ShowSkin();
static void 	PrvListDrawRecentButton();
static Boolean 	PrvDrawBatteryCallback(struct FormGadgetTypeInCallback *gadgetP, UInt16 cmd, void *paramP);
Boolean 		AddrListMoveObjects(FormType* frmP, Coord dx, Coord dy);
MemHandle 		PrvLookupCreateResultString (LookupVariablesPtr vars, UInt16 recordNum, UInt16 phoneNum);
univAddressFields 	PrvLookupFindPhoneField (LookupVariablesPtr vars, univAddrDBRecordType* recordP, AddressLookupFields lookupField, UInt16 phoneNum);
Char * 			PrvLookupResizeResultString (MemHandle resultStringH, UInt16 newSize);
void 			ListLookupOnCancel();
void 			ListLookupOnOK();

//Lookup-related functions

/***********************************************************************
 *
 * FUNCTION:    PrvLookupFindPhoneField
 *
 * DESCRIPTION: Find a phone field from the record.  The first match
 * is used unless it's one of the fields displayed.  In that case the
 * field displayed is used.
 *
 * PARAMETERS:  vars - variables used by the lookup code.
 *                recordP - the record to find the field in
 *                lookupField - the phone field to lookup
 *                phoneNum - the phone field in the record that matches
 *                           (used when field 1 or field2 is a phone field).
 *
 * RETURNED:    The field to use or lookupNoField if none found
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         Roger   7/18/96   Initial Revision
 *
 ***********************************************************************/

univAddressFields PrvLookupFindPhoneField (LookupVariablesPtr vars, univAddrDBRecordType* recordP, AddressLookupFields lookupField, UInt16 phoneNum)
{
	int index;
	int phoneType;


	if (vars->params->field1 == lookupField || vars->params->field2 == lookupField)
		return (univAddressFields) phoneNum;
	else
	{
		phoneType = lookupField - addrLookupWork;

		// Scan through the phone fields looking for a phone of the right type
		// which also contains data.
		for (index = univFirstPhoneField; index <= univLastPhoneField; index++)
		{
			if (univGetPhoneLabel(recordP, index) == phoneType &&
				recordP->fields[index] != NULL)
			{
				return (univAddressFields) (univPhone1 + index - univFirstPhoneField);
			}
		}

	}

	return (univAddressFields) addrLookupNoField;
}


/***********************************************************************
 *
 * FUNCTION:    PrvLookupResizeResultString
 *
 * DESCRIPTION: Resize the lookup a result string
 *
 * PARAMETERS:  resultStringP - result string
 *              newSize       - new size
 *
 * RETURNED:    pointer to the resized result of zero if the resize failed.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         Art   12/11/96   Initial Revision
 *
 ***********************************************************************/
Char * PrvLookupResizeResultString (MemHandle resultStringH, UInt16 newSize)
{
	Err err;

	MemHandleUnlock (resultStringH);

	err = MemHandleResize (resultStringH, newSize);
	if (err)
		return (0);

	return (MemHandleLock (resultStringH));
}


/***********************************************************************
 *
 * FUNCTION:    PrvLookupCreateResultString
 *
 * DESCRIPTION: Create a result string which includes data from the record.
 *
 * PARAMETERS:  vars - variables used by the lookup code.
 *                recordNum - the record create a result string from
 *                phoneNum - the phone field in the record that matches
 *                           (used when field 1 or field2 is a phone field).
 *
 * RETURNED:    The MemHandle to the string created or NULL.
 *                vars->params->resultStringH is also set
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         Roger   7/9/96   Initial Revision
 *         Ludovic 4/27/00  Fix bug - formatStringP increment invalid for some fields
 *
 ***********************************************************************/
MemHandle PrvLookupCreateResultString (LookupVariablesPtr vars, UInt16 recordNum, UInt16 phoneNum)
{
	Char * formatStringP;
	MemHandle resultStringH;
	Char * resultStringP;
	Int16 resultSize = 0;
	Int16 nextChunkSize;
	Char * nextFieldP;
	Int16 field;
	univAddrDBRecordType record;
	MemHandle recordH;
	Char * fieldP;
	Err error;
	UInt16 separatorLength;
	UInt16 phoneLabel;
	globalVars* globals = getGlobalsPtr();	

	// Return the record's unique ID
	DmRecordInfo(globals->AddrDB, recordNum, NULL, &(globals->lookupVars.params->uniqueID), NULL);

	// Check if a format string was specified
	formatStringP = vars->params->formatStringP;
	if (formatStringP == NULL)
		return 0;

	// Allocate the string on the dynamic heap
	resultStringH = MemHandleNew(32);      // Allocate extra so there's room to grow
	if (!resultStringH)
		return 0;            // not enough memory?

	error = univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, recordNum, &record, &recordH);
	ErrFatalDisplayIf ((error), "Record not found");

	resultStringP = MemHandleLock(resultStringH);
	while (*formatStringP != '\0')
	{
		// Copy the next chunk (the string until a '^' or '\0'
		nextFieldP = StrChr(formatStringP, '^');
		if (nextFieldP)
			nextChunkSize = nextFieldP - formatStringP;
		else
			nextChunkSize = StrLen(formatStringP);

		if (nextChunkSize > 0)
		{
			resultStringP = PrvLookupResizeResultString (resultStringH,
													  resultSize + nextChunkSize);
			if (! resultStringP) goto exit;

			MemMove(resultStringP + resultSize, formatStringP, nextChunkSize);

			resultSize += nextChunkSize;
			formatStringP += nextChunkSize;
			nextChunkSize = 0;
		}

		// determine which field to copy next
		if (*formatStringP == '^')
		{
			formatStringP++;
			field = (univAddressFields) addrLookupNoField;
			// Decode which field to copy next.

			// Remember that the strings below can't be put into a table
			// because the lookup function runs without global variables
			// available with which we would store the table.
			if (StrNCompareAscii(formatStringP, "name", 4) == 0)
			{
				field = univName;
				formatStringP += 4;
			}
			else if (StrNCompareAscii(formatStringP, "first", 5) == 0)
			{
				field = univFirstName;
				formatStringP += 5;
			}
			else if (StrNCompareAscii(formatStringP, "company", 7) == 0)
			{
				field = univCompany;
				formatStringP += 7;
			}
			else if (StrNCompareAscii(formatStringP, "address", 7) == 0)
			{
				field = univAddress;
				formatStringP += 7;
			}
			else if (StrNCompareAscii(formatStringP, "city", 4) == 0)
			{
				field = univCity;
				formatStringP += 4;
			}
			else if (StrNCompareAscii(formatStringP, "state", 5) == 0)
			{
				field = univState;
				formatStringP += 5;
			}
			else if (StrNCompareAscii(formatStringP, "zipcode", 7) == 0)
			{
				field = univZipCode;
				formatStringP += 7;
			}
			else if (StrNCompareAscii(formatStringP, "country", 7) == 0)
			{
				field = univCountry;
				formatStringP += 7;
			}
			else if (StrNCompareAscii(formatStringP, "title", 5) == 0)
			{
				field = univTitle;
				formatStringP += 5;
			}
			else if (StrNCompareAscii(formatStringP, "custom1", 7) == 0)
			{
				field = univCustom1;
				formatStringP += 7;
			}
			else if (StrNCompareAscii(formatStringP, "custom2", 7) == 0)
			{
				field = univCustom2;
				formatStringP += 7;
			}
			else if (StrNCompareAscii(formatStringP, "custom3", 7) == 0)
			{
				field = univCustom3;
				formatStringP += 7;
			}
			else if (StrNCompareAscii(formatStringP, "custom4", 7) == 0)
			{
				field = univCustom4;
				formatStringP += 7;
			}
			else if (StrNCompareAscii(formatStringP, "work", 4) == 0)
			{
				field = PrvLookupFindPhoneField(vars, &record, addrLookupWork, phoneNum);
				formatStringP += 4;
			}
			else if (StrNCompareAscii(formatStringP, "home", 4) == 0)
			{
				field = PrvLookupFindPhoneField(vars, &record, addrLookupHome, phoneNum);
				formatStringP += 4;
			}
			else if (StrNCompareAscii(formatStringP, "fax", 3) == 0)
			{
				field = PrvLookupFindPhoneField(vars, &record, addrLookupFax, phoneNum);
				formatStringP += 3;
			}
			else if (StrNCompareAscii(formatStringP, "other", 5) == 0)
			{
				field = PrvLookupFindPhoneField(vars, &record, addrLookupOther, phoneNum);
				formatStringP += 5;
			}
			else if (StrNCompareAscii(formatStringP, "email", 5) == 0)
			{
				field = PrvLookupFindPhoneField(vars, &record, addrLookupEmail, phoneNum);
				formatStringP += 5;
			}
			else if (StrNCompareAscii(formatStringP, "main", 4) == 0)
			{
				field = PrvLookupFindPhoneField(vars, &record, addrLookupMain, phoneNum);
				formatStringP += 4;
			}
			else if (StrNCompareAscii(formatStringP, "pager", 5) == 0)
			{
				field = PrvLookupFindPhoneField(vars, &record, addrLookupPager, phoneNum);
				formatStringP += 5;
			}
			else if (StrNCompareAscii(formatStringP, "mobile", 6) == 0)
			{
				field = PrvLookupFindPhoneField(vars, &record, addrLookupMobile, phoneNum);
				formatStringP += 6;
			}
			else if (StrNCompareAscii(formatStringP, "listname", 8) == 0)
			{				
				Char *name1 = NULL, *name2 = NULL;
				formatStringP += 8;
				
				PrvDetermineRecordNameHelper(globals->adxtLibRef, globals->SortByCompany, &record, &name1, &name2);
				if(name1 && name1[0] != NULL)
				{
					nextChunkSize = StrLen(name1);

					if (nextChunkSize > 0)
					{
						resultStringP = PrvLookupResizeResultString (resultStringH,
																  resultSize + nextChunkSize);
						if (! resultStringP) goto exit;

						MemMove(resultStringP + resultSize, name1, nextChunkSize);

						resultSize += nextChunkSize;
						nextChunkSize = 0;
					}
				}	
				if(name2 && name2[0] != NULL)
				{
					nextChunkSize = StrLen(name2);

					if (nextChunkSize > 0)
					{
						Char* delimiter = GetDelimiterStr(globals->adxtLibRef, globals->gDelimiter);
						if(delimiter)
						{
							UInt16 delLen = StrLen(delimiter);
							resultStringP = PrvLookupResizeResultString (resultStringH,
																  resultSize + delLen);
							if (! resultStringP) goto exit;

							MemMove(resultStringP + resultSize, delimiter, delLen);

							resultSize += delLen;
						}
						
						resultStringP = PrvLookupResizeResultString (resultStringH,
																  resultSize + nextChunkSize);
						if (! resultStringP) goto exit;

						MemMove(resultStringP + resultSize, name2, nextChunkSize);

						resultSize += nextChunkSize;
						nextChunkSize = 0;
					}
				}	
				
				// We are done adding the data requested.  Continue to the next
				// chunk
				continue;
			}
			else if (StrNCompareAscii(formatStringP, "listphone", 9) == 0)
			{
				formatStringP += 9;
				separatorLength = 0;

				// Add the list phone number with a letter after it
				fieldP = record.fields[univFirstPhoneField +
									   record.options.phones.displayPhoneForList];
				/*if (fieldP)
				{
					nextChunkSize = StrLen(fieldP);
					if (nextChunkSize > 0)
					{
						Char 	phoneLabelBuf[6];
						Int16	x, len;
						
						phoneLabel = univGetPhoneLabel(&record, univFirstPhoneField +
												   record.options.phones.displayPhoneForList);
						//len = TxtSetNextChar (phoneLabelBuf, 0, vars->phoneLabelLetters[phoneLabel]);

						resultStringP = PrvLookupResizeResultString (resultStringH, resultSize + nextChunkSize + len + 1);
						if (! resultStringP) goto exit;

						MemMove(resultStringP + resultSize, fieldP, nextChunkSize);

						resultSize += nextChunkSize;

						resultStringP[resultSize] = ' ';
						resultSize++;						// This is the +1 in the above resize
						
						for (x = 0; x < len; x++)
						{
							resultStringP[resultSize++] = phoneLabelBuf[x];
						}
						//resultSize += 2;

						nextChunkSize = 0;
						separatorLength = 2;		// DOLATER : <vsm> why is this set to 2 ?
					}
				}*/
			}



			// Now copy in the correct field.  lookupNoField can result from
			// asking for a phone which isn't used.
			if (field != (univAddressFields) addrLookupNoField)
			{
				fieldP = record.fields[field];
				if (fieldP)
				{
					nextChunkSize = StrLen(fieldP);

					if (nextChunkSize > 0)
					{
						resultStringP = PrvLookupResizeResultString (resultStringH, resultSize + nextChunkSize);
						if (! resultStringP) goto exit;
						MemMove(resultStringP + resultSize, fieldP, nextChunkSize);

						resultSize += nextChunkSize;
						nextChunkSize = 0;
					}
				}
			}

		}

	}

	// Now null terminate the result string
	resultStringP = PrvLookupResizeResultString (resultStringH, resultSize + 1);
	if (! resultStringP) goto exit;

	resultStringP[resultSize] = '\0';

	vars->params->resultStringH = resultStringH;
	MemHandleUnlock(recordH);
	MemHandleUnlock(resultStringH);

	return resultStringH;


exit:
	// Error return
	MemHandleUnlock(recordH);
	MemHandleFree (resultStringH);
	vars->params->resultStringH = 0;
	return (0);
}

void Lookup(AddrLookupParamsType * params)
{
	globalVars* globals = getGlobalsPtr();
	Err err;
	DmOpenRef dbP;
	AddrAppInfoPtr appInfoPtr;
	UInt16      cardNo=0;
	LocalID   dbID;
	DmSearchStateType   searchState;
	FormPtr frm;
	FormPtr originalForm;
	Boolean uniqueMatch;
	Boolean completeMatch;
	UInt16 mode;
	FormPtr lastActiveForm;
	WinHandle offScr;
	RectangleType rectAll;
	Boolean res = true;
	//Char* formatStringP;
	
	// Check the parameters
	ErrFatalDisplayIf(params->field1 > addrLookupListPhone &&
					  params->field1 != addrLookupNoField, "Bad Lookup request - field1");
	ErrFatalDisplayIf(params->field2 > addrLookupListPhone &&
					  params->field2 != addrLookupNoField, "Bad Lookup request - field2");


	/*// Find the application's data file.
	err = DmGetNextDatabaseByTypeCreator (true, &searchState, addrDBType,
										  sysFileCAddress, true, &cardNo, &dbID);
	if (err)
	{
		params->resultStringH = 0;
		return;
	}

	// Obey the secret records setting.  Also, we only need to
	// read from the database.
	if (PrefGetPreference(prefHidePrivateRecordsV33))
		mode = dmModeReadOnly;
	else
		mode = dmModeReadOnly | dmModeShowSecret;

	// Open the address database.
	dbP = DmOpenDatabase(cardNo, dbID, mode);
	if (! dbP)
	{
		params->resultStringH = 0;
		params->uniqueID = 0;
		return;
	}

*/	// Initialize some of the lookup variables (those needed)
	globals->lookupVars.params = params;
	globals->lookupVars.beepOnFail = true;
	
	//formatStringP = globals->lookupVars.params->formatStringP;
	//FrmCustomAlert(alertDebug, formatStringP, 0, 0);

	// Find how the database is sorted.
	/*appInfoPtr = (AddrAppInfoPtr) AddrDBAppInfoGetPtr(dbP);
	vars.sortByCompany = appInfoPtr->misc.sortByCompany;
	ToolsInitPhoneLabelLetters(appInfoPtr, vars.phoneLabelLetters);
	MemPtrUnlock(appInfoPtr);
*/
	// Set the mappings from AddressLookupFields to AddressFields.  It is
	// necessary to initialize the mappings at runtime because static
	// global variables are not available to this launch code routine.
	globals->lookupVars.lookupFieldMap[addrLookupName] = univName;
	globals->lookupVars.lookupFieldMap[addrLookupFirstName] = univFirstName;
	globals->lookupVars.lookupFieldMap[addrLookupCompany] = univCompany;
	globals->lookupVars.lookupFieldMap[addrLookupAddress] = univAddress;
	globals->lookupVars.lookupFieldMap[addrLookupCity] = univCity;
	globals->lookupVars.lookupFieldMap[addrLookupState] = univState;
	globals->lookupVars.lookupFieldMap[addrLookupZipCode] = univZipCode;
	globals->lookupVars.lookupFieldMap[addrLookupCountry] = univCountry;
	globals->lookupVars.lookupFieldMap[addrLookupTitle] = univTitle;
	globals->lookupVars.lookupFieldMap[addrLookupCustom1] = univCustom1;
	globals->lookupVars.lookupFieldMap[addrLookupCustom2] = univCustom2;
	globals->lookupVars.lookupFieldMap[addrLookupCustom3] = univCustom3;
	globals->lookupVars.lookupFieldMap[addrLookupCustom4] = univCustom4;
	globals->lookupVars.lookupFieldMap[addrLookupNote] = univNote;
	/*// Check to see if the lookup string is sufficient for a unique match.
	// If so we skip presenting the user a lookup dialog and just use the match.
	AddrDBLookupLookupString(vars.dbP, params->lookupString, vars.sortByCompany,
						   vars.params->field1, vars.params->field2, &vars.currentRecord, &vars.currentPhone,
						   vars.lookupFieldMap, &completeMatch, &uniqueMatch);
	if (completeMatch && uniqueMatch)
	{
		PrvLookupCreateResultString(&vars, vars.currentRecord, vars.currentPhone + firstPhoneField);
		goto Exit;
	}

	// If the user isn't allowed to select a record then return without
	// a match.
	if (!params->userShouldInteract)
	{
		params->resultStringH = 0;
		params->uniqueID = 0;
		goto Exit;
	}


	// Initialize more of the lookup variables
	vars.currentRecord = noRecord;
	vars.currentPhone = 0;
	vars.topVisibleRecord = 0;
	vars.topVisibleRecordPhone = 0;
	vars.hideSecretRecords = PrefGetPreference(prefHidePrivateRecordsV33);


	// Custom title doesn't support attention indicator.
	// Disable indicator before switching forms.
	AttnIndicatorEnable(false);

	*/
	
	/*WinGetBounds(WinGetDisplayWindow(), &rectAll);
	offScr = WinSaveBits(&rectAll, &err);*/
	
	if(StrLen(params->lookupString))
	{
		StrCopy(globals->lookupString, params->lookupString);
	}
	else
	{
		MemSet(globals->lookupString, addrLookupStringLength, 0);
	}
	lastActiveForm = FrmGetActiveForm();
	frm = FrmInitForm (ListView);
	FrmSetActiveForm(FrmGetFormPtr(ListView));
	FrmDrawForm (frm);
	ListOnOpen();
	//FrmGotoForm(ListView);
	FrmSetEventHandler(frm, ListHandleEvent);
	res = PrvAppLookupEventLoop ();
	
	if((globals->SelectedLookupRecord != noRecord) && res)
	{
		PrvLookupCreateResultString(&(globals->lookupVars), globals->SelectedLookupRecord, 0);
		//DmRecordInfo(globals->AddrDB, globals->SelectedLookupRecord, NULL, &(globals->lookupVars.params->uniqueID), NULL);
		//params->resultStringH = 0;
		//PRINTDEBUG(0);
	}
	else
	{
		//PRINTDEBUG(1);
		params->resultStringH = 0;
		params->uniqueID = 0;
	}
	
	if(lastActiveForm)
	{
	//PRINTDEBUG(1);
		FrmSetFocus (frm, noFocus);
	//PRINTDEBUG(2);
		FrmEraseForm (frm);
	//PRINTDEBUG(3);
		FrmDeleteForm (frm);
	//PRINTDEBUG(4);
		FrmSetActiveForm(lastActiveForm);
	//PRINTDEBUG(5);
	}//WinRestoreBits(offScr, 0, 0);
	
	//PRINTDEBUG(6);
	/*// Remember the original form
	originalForm =  FrmGetActiveForm();

	// Initialize the dialog.
	frm = FrmInitForm (LookupView);
	vars.frm = frm;

	// Set the title
	if (params->title)
		FrmSetTitle(frm, params->title);

	// Set the paste button
	if (params->pasteButtonText)
		CtlSetLabel (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, LookupPasteButton)), params->pasteButtonText);

	FrmSetActiveForm (frm);

	PrvLookupViewInit (&vars);

	// Enter the lookup string
	if (params->lookupString && *params->lookupString != '\0')
	{
		FldInsert (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, LookupLookupField)), params->lookupString, StrLen(params->lookupString));
		PrvLookupViewLookupString(&vars, NULL);
	}

	FrmDrawForm (frm);

	FrmSetFocus (frm, FrmGetObjectIndex (frm, LookupLookupField));

	// Handle events until the user picks a record or cancels
	if (PrvLookupViewHandleEvent (&vars))
	{
		PrvLookupCreateResultString(&vars, vars.currentRecord, vars.currentPhone);
	}
	else
	{
		params->resultStringH = 0;
		params->uniqueID = 0;
	}

	AttnIndicatorEnable(true);		// Custom title doesn't support attention indicator.

	FrmSetFocus (frm, noFocus);
	FrmEraseForm (frm);
	FrmDeleteForm (frm);
	FrmSetActiveForm (originalForm);


Exit:
	DmCloseDatabase (dbP);*/
}

//end Lookup-related

void LoadSkin()
{
	globalVars* globals = getGlobalsPtr();
	DmResID resource;
	
	if(! globals->bitmap)
	{
#ifdef DEBUG
		LogWrite("xt_log", "list", "LoadSkin");
#endif

		if(globals->gScreen == PALM_160x160)
		{
			resource = SkinMainTitleBarStd;
		}
		else if(globals->gScreen == PALM_320x320)
		{
			resource = SkinMainTitleBarHR;
		}
		
		globals->bitmapH = DmGetResource(bitmapRsc, resource);
		
		if(globals->bitmapH == 0)
			return;
		
		globals->bitmap = MemHandleLock(globals->bitmapH);	
	}
}

void UnloadSkin()
{
	globalVars* globals = getGlobalsPtr();
	if(globals->bitmapH)
	{
#ifdef DEBUG
		LogWrite("xt_log", "list", "UnloadSkin");
#endif

		MemHandleUnlock(globals->bitmapH); 
		DmReleaseResource(globals->bitmapH);
		globals->bitmap = 0;
		globals->bitmapH = 0;
	}
}

void ShowSkin()
{
	globalVars* globals = getGlobalsPtr();
	WinHandle winH;
	RectangleType rect;
	IndexedColorType color, prevColor;
	
#ifdef DEBUG
	LogWrite("xt_log", "list", "ShowSkin");
#endif
	
	winH=WinGetDisplayWindow();
	if(!globals->gNavigation)
		WinSetDrawWindow(winH);
	
	if(! globals->bitmap)
	{
		LoadSkin();
		if(! globals->bitmap)
			return;
	}
				
	WinDrawBitmap(globals->bitmap, 0, 0);
					
	color = UIColorGetTableEntryIndex(UIFormFrame);
    prevColor = WinSetForeColor(color);
    WinGetBounds(winH, &rect);
	{
		WinDrawLine(0, 14, rect.extent.x, 14);
		WinDrawLine(0, 13, rect.extent.x, 13);
	}
	WinSetForeColor(prevColor);
}

// non-flickering drawing bitmap in form title
void PrvListDrawFormAndSkin(FormPtr frmP)
{
	UInt8 *locked;
	
	// artefacts on form under Sony OS4
	locked = WinScreenLock(winLockCopy);
	if(! locked)
	{
#ifdef DEBUG
		LogWrite("xt_log", "list", "WinScreenLock failed");
#endif
	}
	
	FrmDrawForm(frmP);

#ifdef DEBUG
	LogWrite("xt_log", "list", "FrmDrawForm");
#endif
	
	ShowSkin();
	
	if(locked)
		WinScreenUnlock();
}

Boolean LookupLetter(WinDirectionType direction, Boolean recursive)
{
	globalVars* globals = getGlobalsPtr();
	Char lStr[255], lStr1[2], lStr2[2];
	UInt16 posToChange=0;
	FormType* frmP;
	FieldType* fldP;
	UInt16 length,index =0;
	UInt16 attr;
	Err err;
	Int16 fieldSeparatorWidth, widthWithoutPhoneLabel=40;
	Int16 shortenedFieldWidth;
	Char * name1;
	Char * name2;
	Int16 name1Length;
	Int16 name2Length;
	Int16 name1Width;
	Int16 name2Width;
	static UInt16 oldCurrentRecord;
	const Int16 phoneColumnWidth = 82;
	
	MemHandle recordH;
	univAddrDBRecordType record;
	Boolean advancedFind = false;	
	if(globals->gAdvancedFind == ADVANCEDFIND_ON)
		advancedFind = true;
	advancedFind = false;
	frmP = FrmGetActiveForm();
	FrmSetFocus(frmP, FrmGetObjectIndex(frmP, ListLookupField));
	fldP = CustomGetObjectPtrSmp(ListLookupField);
				
	length = FldGetTextLength(fldP);
	MemSet(lStr, 255, 0);
	index=0;
	if(length==0)
		return false;
	if(!recursive)
		oldCurrentRecord = globals->CurrentRecord;
	
	if(globals->gOneHanded)
	{
		if (length > 0)
		{
			UInt16 minDiff = 10000, cnt = 0, minRec = noRecord, minNameNum;
			Int16 currDiff;
			StrCopy(lStr, CustomFldGetTextPtrSmp(ListLookupField));
			posToChange=StrLen(lStr);
			
			if(DmRecordInfo(globals->AddrDB, index, &attr, 0, 0)!=errNone)
				return false;
				
			
			while(true)
			{
				if(cnt == 0)
				{
					if(PrvAddrDBSeekVisibleRecordInCategory(globals->adxtLibRef, globals->AddrDB, &index, 0, dmSeekForward, globals->CurrentCategory, globals->PrivateRecordVisualStatus != showPrivateRecords)!=errNone)
					{
						if(PrvAddrDBSeekVisibleRecordInCategory(globals->adxtLibRef, globals->AddrDB, &index, 1, dmSeekForward, globals->CurrentCategory, globals->PrivateRecordVisualStatus != showPrivateRecords)!=errNone)
						{
								break;
						}				
					}
				}
				
				else
				{
					if(PrvAddrDBSeekVisibleRecordInCategory(globals->adxtLibRef, globals->AddrDB, &index, 1, dmSeekForward, globals->CurrentCategory, globals->PrivateRecordVisualStatus != showPrivateRecords)!=errNone)
					{
							break;
					}
				}
				cnt = 1;
				err = univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, index, &record, &recordH);
				if (err)
				{
					MemHandleUnlock(recordH);
					if(recursive)
						return false;
					else
						break;
				}
				ToolsDetermineRecordName(globals->adxtLibRef, &record, &shortenedFieldWidth, &fieldSeparatorWidth, globals->SortByCompany, &name1, &name1Length, &name1Width, &name2, &name2Length, &name2Width,  &globals->UnnamedRecordStringPtr, &globals->UnnamedRecordStringH, widthWithoutPhoneLabel, false);	
				MemHandleUnlock(recordH);
				if(name1 != NULL)
				{
					if(StrLen(name1) >= posToChange)
					{
						if(!StrNCaselessCompare(name1, lStr, posToChange-1))
						{
							MemSet(lStr1, 2, 0);
							MemSet(lStr2, 2, 0);
							lStr1[0]=name1[posToChange-1];
							lStr2[0]=lStr[posToChange-1];
							currDiff = StrNCaselessCompare(lStr1, lStr2, 1);
							if(!((currDiff < 0 && direction == winDown) || (currDiff >0 && direction == winUp) || currDiff == 0))
							{
								currDiff = Abs(StrNCaselessCompare(lStr1, lStr2, 1));
								if(minDiff > currDiff)
								{
									minDiff = currDiff;
									minRec = index;
									minNameNum = 1;
								}
							}
						}
					}
				}
				if(name2 != NULL && advancedFind)
				{
					if(StrLen(name2) >= posToChange)
					{
						if(!StrNCaselessCompare(name2, lStr, posToChange-1))
						{
							MemSet(lStr1, 2, 0);
							MemSet(lStr2, 2, 0);
							lStr1[0]=name2[posToChange-1];
							lStr2[0]=lStr[posToChange-1];
							currDiff = StrNCaselessCompare(lStr1, lStr2, 1);
							if((currDiff < 0 && direction == winDown) || (currDiff >0 && direction == winUp) || currDiff == 0)
								continue;
							currDiff = Abs(StrNCaselessCompare(lStr1, lStr2, 1));
							if(minDiff > currDiff)
							{
								minDiff = currDiff;
								minRec = index;
								minNameNum = 2;
							}
						}
					}			
				}
			}
			if(minRec != noRecord)
			{
				index = minRec;
				err = univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, index, &record, &recordH);
				if (err)
				{
					MemHandleUnlock(recordH);
					return false;
				}
				ToolsDetermineRecordName(globals->adxtLibRef, &record, &shortenedFieldWidth, &fieldSeparatorWidth, globals->SortByCompany, &name1, &name1Length, &name1Width, &name2, &name2Length, &name2Width,  &globals->UnnamedRecordStringPtr, &globals->UnnamedRecordStringH, 
					widthWithoutPhoneLabel, false);	
				MemHandleUnlock(recordH);
				MemSet(lStr1, 2, 0);
				MemSet(lStr2, 2, 0);
				if(minNameNum == 1 || !advancedFind)
					lStr1[0]=name1[posToChange-1];
				else
					lStr1[0]=name2[posToChange-1];
				lStr2[0]=lStr[posToChange-1];
				if(StrNCaselessCompare(lStr1, lStr2, 1))
				{
					if(minNameNum == 1)
						lStr[posToChange-1]=name1[posToChange-1];
					else
						lStr[posToChange-1]=name2[posToChange-1];
					
					CustomEditableFldSetTextPtrSmp(ListLookupField, lStr);
					if(!recursive)
					{
						FldDrawField(fldP);	
						globals->CurrentRecord = oldCurrentRecord;
						RefreshLookup(true);
					}
					else
						RefreshLookup(false);
					return true;
				}
			}		
		}
		//start it all over again
		index=globals->CurrentRecord;
		if(!recursive)
		{
			UInt16 opposite;
			UInt16 beforeLookup;
			if(direction == winUp)
				opposite = winDown;
			else
				opposite = winUp;
			
			while(LookupLetter(opposite, true)){};
			globals->CurrentRecord = oldCurrentRecord;
			beforeLookup = globals->CurrentRecord;
			RefreshLookup(true);
			if(globals->CurrentRecord == beforeLookup)
			{
				RefreshLookupEx(true, direction);
			}
		}		
	}
	else
	{
		RefreshLookupEx(true, direction);
	}	
	return false;			
}

UInt16 LookupLength()
{
	FieldType* fldP = CustomGetObjectPtrSmp(ListLookupField);
	return FldGetTextLength(fldP);
}

void AddRemoveLetter()
{
	globalVars* globals = getGlobalsPtr();
	Char lStr[255];
	UInt16 posToChange=0;
	FieldType* fldP;
	TablePtr tableP;
	UInt16 length,index =0;
	const Int16 phoneColumnWidth = 82;
	
	fldP = CustomGetObjectPtrSmp(ListLookupField);

	length = FldGetTextLength(fldP);
	MemSet(lStr, 255, 0);
	if(length==0)
	{
		tableP = CustomGetObjectPtrSmp(ListTable);
		if(lStr[0] == 0)
		{
			globals->CurrentRecord=noRecord;
			PrvListSelectRecord(FrmGetActiveForm(), globals->CurrentRecord, true, true);
			TblDrawTable(tableP);	
		}
		globals->gScrollMode = FIVEWAYPAGE;
		return;
	}
	else
	{
		StrCopy(lStr, CustomFldGetTextPtrSmp(ListLookupField));
		lStr[StrLen(lStr)-1]=0;
		CustomEditableFldSetTextPtrSmp(ListLookupField, lStr);
		FldDrawField(fldP);	
	}

	if(globals->gNavigation)
	{
		globals->gListTableActive = true;
	}
}

void AddLookupLetter()
{
	globalVars* globals = getGlobalsPtr();
	Char lStr[255];
	UInt16 posToChange=0;
	FieldType* fldP;
	UInt16 length,index =0, category;
	UInt16 attr;
	Err err;
	Int16 fieldSeparatorWidth;
	Int16 shortenedFieldWidth, widthWithoutPhoneLabel=40;
	Char * name1;
	Char * name2;
	UInt16 cnt = 0;
	Int16 name1Length;
	Int16 name2Length;
	Int16 name1Width;
	Int16 name2Width;
	const Int16 phoneColumnWidth = 82;
	MemHandle recordH;
	univAddrDBRecordType record;
	Boolean advancedFind = false;	
	if(globals->gAdvancedFind == ADVANCEDFIND_ON)
		advancedFind = true;
	
	fldP = CustomGetObjectPtrSmp(ListLookupField);

	length = FldGetTextLength(fldP);
	MemSet(lStr, 255, 0);
	if(length==7)
		return;
	if(length > 0 && globals->CurrentRecord == noRecord)
		return;
	globals->gReturnFocusToTable = true;
	
	if (length > 0)
	{
		index=globals->CurrentRecord;
		StrCopy(lStr, CustomFldGetTextPtrSmp(ListLookupField));
		posToChange=StrLen(lStr);
		
		if(DmRecordInfo(globals->AddrDB, index, &attr, 0, 0)!=errNone)
			return;
		category=attr & dmRecAttrCategoryMask;
		if(category != globals->CurrentCategory)
		{
			if(PrvAddrDBSeekVisibleRecordInCategory(globals->adxtLibRef, globals->AddrDB, &index, 0, dmSeekForward, globals->CurrentCategory, globals->PrivateRecordVisualStatus != showPrivateRecords)!=errNone)
			{
				if(globals->gNavigation)
				{
					globals->gListTableActive = true;
				}
				return;
				
			}
		}
		while(true)
		{
			err = univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, index, &record, &recordH);
			if (err)
			{
				MemHandleUnlock(recordH);
				break;
			}
			ToolsDetermineRecordName(globals->adxtLibRef, &record, &shortenedFieldWidth, &fieldSeparatorWidth, 
				globals->SortByCompany, &name1, &name1Length, &name1Width, &name2, &name2Length,
				&name2Width,  &globals->UnnamedRecordStringPtr, &globals->UnnamedRecordStringH, widthWithoutPhoneLabel, false);	
			MemHandleUnlock(recordH);
			
			if(!advancedFind)
			{
				if(StrLen(name1)<=StrLen(lStr))
				{
					UInt16 seek = 0;
					if(cnt == 0)
						seek = 1;					
					if((PrvAddrDBSeekVisibleRecordInCategory(globals->adxtLibRef, globals->AddrDB, &index, seek, dmSeekForward, globals->CurrentCategory, globals->PrivateRecordVisualStatus != showPrivateRecords)==errNone))
					{
						continue;
					}	
					else
						break;		
				}
				
				else if(!StrNCaselessCompare(lStr, name1, StrLen(lStr)))
				{
					lStr[StrLen(lStr)]=name1[StrLen(lStr)];
					CustomEditableFldSetTextPtrSmp(ListLookupField, lStr);
					FldDrawField(fldP);	
					RefreshLookup(true);
					if(globals->gNavigation)
					{
						globals->gListTableActive = true;
					}			
					return;
				}	
			}
			else
			{
				if(StrLen(name1)<=StrLen(lStr) && StrLen(name2) <= StrLen(lStr))
				{
					UInt16 seek = 0;
					if(cnt == 0)
						seek = 1;					
					if((PrvAddrDBSeekVisibleRecordInCategory(globals->adxtLibRef, globals->AddrDB, &index, seek, dmSeekForward, globals->CurrentCategory, globals->PrivateRecordVisualStatus != showPrivateRecords)==errNone))
					{
						continue;
					}	
					else
						break;		
				}
				
				else if(!StrNCaselessCompare(lStr, name1, StrLen(lStr)))
				{
					lStr[StrLen(lStr)]=name1[StrLen(lStr)];
					CustomEditableFldSetTextPtrSmp(ListLookupField, lStr);
					FldDrawField(fldP);	
					RefreshLookup(true);
					if(globals->gNavigation)
					{
						globals->gListTableActive = true;
					}			
					return;
				}	
				else if(!StrNCaselessCompare(lStr, name2, StrLen(lStr)))
				{
					lStr[StrLen(lStr)]=name2[StrLen(lStr)];
					CustomEditableFldSetTextPtrSmp(ListLookupField, lStr);
					FldDrawField(fldP);	
					RefreshLookup(true);
					if(globals->gNavigation)
					{
						globals->gListTableActive = true;
					}			
					return;
				}	
			
			}
			
			if(PrvAddrDBSeekVisibleRecordInCategory(globals->adxtLibRef, globals->AddrDB, &index, 1, dmSeekForward, globals->CurrentCategory, globals->PrivateRecordVisualStatus != showPrivateRecords)!=errNone)
				break;
			
		}
	}
	else
	{
		if(globals->CurrentRecord != noRecord)
			index=globals->CurrentRecord;
		else
		{
			//index should eq. index of top record on screen
			TablePtr tblP = CustomGetObjectPtrSmp(ListTable);
			index = TblGetRowID(tblP, 0);
			if(index == noRecord)
				index=0;
		}
		
		DmRecordInfo(globals->AddrDB, index, &attr, 0, 0);
		category=attr & dmRecAttrCategoryMask;
		if(category != globals->CurrentCategory)
		{
			PrvAddrDBSeekVisibleRecordInCategory(globals->adxtLibRef, globals->AddrDB, &index, 0, dmSeekForward, globals->CurrentCategory, globals->PrivateRecordVisualStatus != showPrivateRecords);
		}
		if(univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, index, &record, &recordH))
		{
			if(globals->gNavigation)
			{
				globals->gListTableActive = true;
			}
			return;
		}
		//determine first letter of 1 st field
		ToolsDetermineRecordName(globals->adxtLibRef, &record, &shortenedFieldWidth, &fieldSeparatorWidth, globals->SortByCompany, &name1, &name1Length, &name1Width, &name2, &name2Length, &name2Width,  &globals->UnnamedRecordStringPtr, &globals->UnnamedRecordStringH, 
		widthWithoutPhoneLabel, false);	
		if(StrLen(name1)>0)
		{
			Char lStr2[255];
			Char* textPtr;
			lStr[0]=name1[0];
			lStr[1] = 0;
			textPtr = CustomFldGetTextPtrSmp(ListLookupField);
			lStr2[0] = 0;
			if(textPtr != NULL)
			{
				if(StrLen(textPtr)>0)
				{
					StrCopy(lStr2, textPtr);
				}
			}
			StrCat(lStr2, lStr);
			
			
			CustomEditableFldSetTextPtrSmp(ListLookupField, lStr2);
			FldDrawField(fldP);	
			RefreshLookup(true);
		}	
		MemHandleUnlock(recordH);
	}
	if(globals->gNavigation)
	{
		globals->gListTableActive = true;
	}
}

Boolean AddrListMoveObjects(FormType* frmP, Coord dx, Coord dy)
{
 	globalVars* globals = getGlobalsPtr();
  	Boolean resized = false;
    
    globals->gMenuActive = false;
    
    if(dx != 0 || dy != 0)
    {
#ifdef DEBUG
		LogWrite("xt_log", "list", "AddrListMoveObjects");
#endif
		
		MoveFormObjectHide(frmP, ListNewButton, 0, dy);
        MoveFormObjectHide(frmP, ListLookupField, 0, dy);
       	MoveFormObjectHide(frmP, ListLookupLabel, 0, dy);
       	MoveFormObjectHide(frmP, ListUpButton, dx, dy);
       	MoveFormObjectHide(frmP, ListDownButton, dx, dy);
       	MoveFormObject(frmP, ListCategoryTrigger, dx, 0);
       	MoveFormObject(frmP, ListCategoryList, dx, 0);
       	MoveFormObject(frmP, ListBatteryGadget, 0, dy);
        MoveFormGSI(frmP, dx, dy);
        
    	resized = true;
    }
    return resized;
}

void CstGoToCurrent()
{
 	globalVars* globals = getGlobalsPtr();	
	TableType* tblP;
	Int16 row;
	if(globals->CurrentRecord==noRecord)
		return;
	if(globals->startupType == startupNormal)
	{
		tblP = CustomGetObjectPtrSmp(ListTable);
	
		TblFindRowID(tblP, globals->CurrentRecord, &row);
		if (TblRowMasked(tblP, row))
		{
			if (SecVerifyPW (showPrivateRecords) == true)
			{
				// We only want to unmask this one record, so restore the preference.
				PrefSetPreference (prefShowPrivateRecords, maskPrivateRecords);
			}
			else
				return;
		}

		globals->TopVisibleFieldIndex = 0;
		globals->EditRowIDWhichHadFocus = editFirstFieldIndex;
		globals->EditFieldPosition = 0;

		FrmGotoForm (RecordView);
	}
	else if(globals->startupType == startupLookup)
	{
		if(globals->SelectedLookupRecord != noRecord)
		{
			ListLookupOnOK();
		} 
	}			
}



void CstAppStop()
{
	EventType newEvent;
	
	newEvent.eType = keyDownEvent;
	newEvent.data.keyDown.chr = launchChr;
	newEvent.data.keyDown.modifiers = commandKeyMask;
	EvtAddEventToQueue (&newEvent);
}


Boolean CstSelUpDown(Int16 direction, Boolean clearLookup)
{
	globalVars* globals = getGlobalsPtr();
	TableType* tblP;
	Boolean res;
	UInt16 rowsInPage;
	UInt16 newTopVisibleRecord;
	UInt16 prevTopVisibleRecord = globals->TopVisibleRecord;
	UInt16 indexP=0;
	UInt16 offset=0;
	if(PrvAddrDBSeekVisibleRecordInCategory(globals->adxtLibRef, globals->AddrDB, &indexP, offset, dmSeekForward, globals->CurrentCategory, (globals->PrivateRecordVisualStatus != showPrivateRecords))!=errNone)
		return false;
	tblP = CustomGetObjectPtrSmp(ListTable);
	// Safe. There must be at least one row in the table.
	rowsInPage = PrvListNumberOfRows(tblP) - 1;
	if(globals->CurrentRecord==noRecord)
	{
		globals->CurrentRecord=indexP;
		newTopVisibleRecord = globals->CurrentRecord;
			
		PrvListSelectRecord(FrmGetActiveForm(), newTopVisibleRecord, true, true);
		return false;
	}
	newTopVisibleRecord = globals->CurrentRecord;
	res = ToolsSeekRecord (globals->adxtLibRef, &newTopVisibleRecord, 1, direction);
	PrvListSelectRecord(FrmGetActiveForm(), newTopVisibleRecord, true, true);
	if(clearLookup)
	{
		PrvListClearLookupString ();
	}		
	return res;	
}

int PrepareRecentList()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 i, cnt, cnt2=0;
	UInt16 index;
	univAddrDBRecordType record;
	MemHandle memHandle;
	Char *name1, *name2;
	Int16 shortenedFieldWidth;
	Int16 name1Length;
	Int16 name2Length;
	Int16 name1Width;
	Int16 name2Width;
	Int16 fieldSeparatorWidth;
	Int16 width=120-2;
	Char* name3;
	UInt16 l1=0, l2=0;
	Char delim[2];
	UInt16 category;
	StrCopy(delim, ", ");
	
	if(globals->gShowAll)
		category=dmAllCategories;
	else
		category=globals->CurrentCategory;
	
	CleanRecentDB(globals->adxtLibRef);
	i=AddrDBCountRecentInCategory(globals->adxtLibRef, category);
	if(i == 0)
		return i;
	
	for(cnt=0;cnt<i;cnt++)
	{
		Boolean recExists;
		//load contact name into RecentList[cnt]
		index=cnt;
		SeekRecentInCategory(globals->adxtLibRef, category, &index);
		
		recExists = (univAddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, index, &record, &memHandle)==0);
		if(recExists)
		{
			if(memHandle)
			{
				name1=0;
				name2=0;
				l1=0;
				l2=0;
				ToolsDetermineRecordName(globals->adxtLibRef, &record,  &shortenedFieldWidth, &fieldSeparatorWidth, globals->SortByCompany, &name1, &name1Length, &name1Width, &name2,
				 &name2Length, &name2Width, &globals->UnnamedRecordStringPtr, &globals->UnnamedRecordStringH, width, true); 			
				
				if(name1!=0)
					l1=StrLen(name1);
				if(name2!=0)
					l2=StrLen(name2);
				
				if(l1+l2>0)
				{
					name3=MemPtrNew(l1+l2+5);
					if(l1>0)
					{
						StrCopy(name3, name1);
						if(l2>0)
						{
							StrCat(name3, delim);
							StrCat(name3, name2);
						}
					}
					else if(l2>0)
					{
						StrCopy(name3, name2);

					}
					globals->RecentList[cnt2]=MemPtrNew(63*sizeof(Char));
					StrNCopy(globals->RecentList[cnt2++], name3, 63);
					MemPtrFree(name3);
				}      					
				
			}
			MemHandleUnlock(memHandle);
		}      	
	}
	if(cnt2!=0)
	{
		LstSetListChoices(GetObjectPtrSmp(ListRecentList), globals->RecentList, cnt2);
		LstSetHeight(GetObjectPtrSmp(ListRecentList), cnt2);
	}
	else if(i > 0)
	{
		if(globals->RecentList[0] != NULL)
			MemPtrFree(globals->RecentList[0]);
	}
	return i;
}	
			
void CleanRecentList(UInt16 i)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 cnt;
	if(i==0)
		return;
	for(cnt=0;cnt<i;cnt++)
	{
		if(globals->RecentList[cnt]!=NULL)
			MemPtrFree(globals->RecentList[cnt]);
	}
}				

Boolean ListOnUpdate(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetFormPtr(ListView);
	globals->gMenuActive = false;

#ifdef DEBUG
	LogWrite("xt_log", "list", "ListOnUpdate");
#endif
	
	if(event->data.frmUpdate.updateCode == frmRedrawUpdateCode)
	{
		FrmDrawForm (frmP);
		if(globals->CurrentRecord != noRecord)
			PrvListSelectRecord(FrmGetActiveForm(), globals->CurrentRecord, true, true);
		return true;
	}
	
	if(event->data.frmUpdate.updateCode == updateCustomFieldLabelChanged)
	{
		return true;
	}
	
	
	if(globals->gDeviceFlags.bits.treo)
	{
		PrvListDrawFormAndSkin(frmP);
	}
	
	if(event->data.frmUpdate.updateCode == updateRecent)
	{
		PrvListDrawRecentButton();
		if (globals->CurrentRecord != noRecord)
		{
			PrvListSelectRecord(frmP, globals->CurrentRecord, true, true);
		}
		return true;
	}
	else if(event->data.frmUpdate.updateCode == updatePrefs)
	{
	 	PrvListUpdateDisplay (event->data.frmUpdate.updateCode);		
		return true;
	}
	//else if(event->data.frmUpdate.updateCode == frmRedrawUpdateCode)// && !globals->gTapwave)
	//{
	//	PrvListUpdateDisplay (event->data.frmUpdate.updateCode);
	//	return true;
	//}
	
	//if(event->data.frmUpdate.updateCode == updateColorsChanged)
	//	WinEraseWindow();
	
	PrvListUpdateDisplay(event->data.frmUpdate.updateCode);
	
	if (globals->CurrentRecord != noRecord)
	{	
		PrvListSelectRecord(frmP, globals->CurrentRecord, true, true);
		globals->RowEnter=globals->CurrentRecord;
	}
	PrvListUpdateScrollButtons(frmP);
	
	return true;
}

Boolean ListOnOpen()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetActiveForm();
	WinHandle winH;
	RectangleType winBounds;
#ifdef DEBUG
	LogWrite("xt_log", "list", "ListOnOpen");
#endif
	if(globals->startupType != startupNormal)
	{
		CustomHideObjectSmp(ListSignalGadget);
		CustomHideObjectSmp(ListBatteryGadget);
		CustomHideObjectSmp(ListNewButton);	
		CustomShowObjectSmp(ListLookupAdd);
		CustomShowObjectSmp(ListLookupCancel);	
	}
	globals->gListTableActive = true;
	globals->gReturnFocusToTable = false;
	globals->gAddressView=false;
	CustomHideObjectSmp(ListTable);
	dia_save_state();
	dia_enable(frmP, true);
	dia_resize(frmP, AddrListMoveObjects);
	
	winH=FrmGetWindowHandle(frmP);
	WinGetBounds(winH, &winBounds);
	PrvListInit (frmP);
	
	if(!globals->gDeviceFlags.bits.treo && globals->startupType == startupNormal) // && show battery flag
	{
		FrmSetGadgetHandler(frmP, FrmGetObjectIndex(frmP, ListBatteryGadget), PrvDrawBatteryCallback);			
	}
	
	PrvListDrawFormAndSkin(frmP);
	
	if(globals->gScreen==PALM_320x320)
	{
		ControlType* c= (ControlType*) CustomGetObjectPtr(frmP, ListRecentButton);
		CtlSetGraphics(c, BmpRecentHr, BmpRecentHrPressed);
	}	
	// Make sure the record to be selected is one of the table's rows or
	// else it reloads the table with the record at the top.  Nothing is
	// drawn by this because the table isn't visible.
	globals->RowEnter=-1;
	globals->RowExit=-1;
	
	//show/hide recent button
	PrvListDrawRecentButton();
	
	//AttnIndicatorEnable(true);
	// Select the record.  This finds which row to select it and does it.
	if (/*globals->ListViewSelectThisRecord != noRecord && */globals->startupType != startupLookup)
	{
		if(globals->ListViewSelectThisRecord == noRecord)
			globals->ListViewSelectThisRecord = globals->TopVisibleRecord;
		PrvListSelectRecord(frmP, globals->ListViewSelectThisRecord, true, false);
		CustomShowObjectSmp(ListTable);
		PrvListSelectRecord(frmP, globals->ListViewSelectThisRecord, true, true);
		globals->RowEnter=globals->ListViewSelectThisRecord;
		globals->ListViewSelectThisRecord = noRecord;
	}
	else
	{
		CustomShowObjectSmp(ListTable);
	}

	//PRINTDEBUG(2);
	// Set the focus in the lookup field so that the user can easily
	// bring up the keyboard.
	if(StrLen(globals->lookupString) > 0)
	{
		CustomEditableFldSetTextPtrSmp(ListLookupField, globals->lookupString);		
		FldDrawField(CustomGetObjectPtrSmp(ListLookupField));	
		RefreshLookup(true);						
	}
	
	FrmSetFocus(frmP, FrmGetObjectIndex(frmP, ListLookupField));

	globals->PriorAddressFormID = FrmGetFormId (frmP);

	// Check the dialing abilities
	// Only the first call is long and further called are fast
	// So it's better to do it the first time the form is drawn

	if (!ToolsIsDialerPresent())
		globals->EnableTapDialing = false;
	
	if(globals->gDeviceFlags.bits.treo && globals->startupType == startupNormal)
	{
		HsStatusSetGadgetType(FrmGetActiveForm(),
		ListBatteryGadget,
		pmSysGadgetStatusGadgetBattery);
		
		HsStatusSetGadgetType(FrmGetActiveForm(),
		ListSignalGadget,
		pmSysGadgetStatusGadgetSignal);
	
		FrmSetMenu(FrmGetActiveForm(), ListViewTreoMenuBar);
	}
	
	//Check registration
	if(!CheckRegistration(globals->adxtLib2Ref))
	{
		if(globals->gTrialExpired)
			FrmPopupForm(AboutForm);
	}
	
	return true;
}

static Boolean Generic5WayRight()
{
	globalVars* globals = getGlobalsPtr();
	if(!globals->gDeviceFlags.bits.treo || globals->gOneHanded)
	{
		globalVars* globals = getGlobalsPtr();
		//add a letter to the lookup string
		if(globals->gOneHanded)
			AddLookupLetter();
		if(globals->FiveWayUpDown==FIVEWAYCONTACTS && globals->gScrollMode==FIVEWAYPAGE)
		{
			globals->gScrollMode=FIVEWAYRECORD;
		}
		return true;
	}
	else
	{
		return false;
	}
}

static Boolean Generic5WayUp(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetActiveForm();
	globals->gReturnFocusToTable = true;
	if(event->data.keyDown.modifiers & optionKeyMask)
	{
		// Reset scroll rate if not auto repeating
		if ((event->data.keyDown.modifiers & autoRepeatKeyMask) == 0)
		{
			PrvListResetScrollRate();
		}
		// Adjust the scroll rate if necessary
		PrvListAdjustScrollRate();
		PrvListClearLookupString ();
		PrvListScroll (winUp, globals->ScrollUnits, false);
		if(globals->gNavigation)
		{
			globals->gListTableActive = true;
			FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, ListTable);	
		}
		return true;
	}
	if(globals->gOneHanded && LookupLength()>0)
	{
		globals->gReturnFocusToTable = true;
		LookupLetter(winUp, false);
	}
	
	else if(globals->FiveWayUpDown==FIVEWAYRECORD || (globals->FiveWayUpDown==FIVEWAYCONTACTS && globals->gScrollMode==FIVEWAYRECORD))
	{
		if(!CstSelUpDown(dmSeekBackward, false) && globals->gNavigation)
		{
			globals->gListTableActive = false;
			globals->gReturnFocusToTable = false;
			return false;
		}
	}
	else
	{
		// Reset scroll rate if not auto repeating
		if ((event->data.keyDown.modifiers & autoRepeatKeyMask) == 0)
		{
			PrvListResetScrollRate();
		}
		// Adjust the scroll rate if necessary
		PrvListAdjustScrollRate();
		//PrvListClearLookupString ();
		if(!PrvListScroll (winUp, globals->ScrollUnits, false))
		{
			if(globals->gNavigation)
			{
				globals->gListTableActive = false;
				globals->gReturnFocusToTable = false;
				return false;
			}				
		}
	}
	return true;
}

static Boolean Generic5WayDown(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetActiveForm();
	globals->gReturnFocusToTable = true;
	if(event->data.keyDown.modifiers & optionKeyMask)
	{
		// Reset scroll rate if not auto repeating
		if ((event->data.keyDown.modifiers & autoRepeatKeyMask) == 0)
		{
			PrvListResetScrollRate();
		}
		// Adjust the scroll rate if necessary
		PrvListAdjustScrollRate();
		PrvListClearLookupString ();
		PrvListScroll (winDown, globals->ScrollUnits, false);
		if(globals->gNavigation)
		{
			globals->gListTableActive = true;
			FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, ListTable);	
		}
		return true;
	}
	if(globals->gOneHanded && LookupLength()>0)
	{
		globals->gReturnFocusToTable = true;
		LookupLetter(winDown, false);
	}
	else if(globals->FiveWayUpDown==FIVEWAYRECORD || (globals->FiveWayUpDown==FIVEWAYCONTACTS && globals->gScrollMode==FIVEWAYRECORD))
	{
		if(!CstSelUpDown(dmSeekForward, false) && globals->gNavigation)
		{
			globals->gListTableActive = false;
			globals->gReturnFocusToTable = false;
			return false;
		}
	
	}
	else
	{
		// Reset scroll rate if not auto repeating
		if ((event->data.keyDown.modifiers & autoRepeatKeyMask) == 0)
		{
			PrvListResetScrollRate();
		}
		// Adjust the scroll rate if necessary
		PrvListAdjustScrollRate();
		//PrvListClearLookupString ();
		//	globals->gListTableActive = false;
		//	globals->gReturnFocusToTable = false;
		//	return false;
		if(!PrvListScroll (winDown, globals->ScrollUnits, false))
		{
			if(globals->gNavigation)
			{
				globals->gListTableActive = false;
				globals->gReturnFocusToTable = false;
				return false;
			}				
		}
	}
	if(globals->gNavigation)
	{
		globals->gListTableActive = true;
		FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, ListTable);	
	}
	return true;
}

static void Generic5WayCenter()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 firstRec;
	if(globals->FiveWayUpDown==FIVEWAYCONTACTS)
	{
		UInt16 attr;
		Boolean hasRecords = true;
		
		firstRec = globals->CurrentRecord;
		if(firstRec == noRecord)
			firstRec = globals->TopVisibleRecord;
		DmRecordInfo (globals->AddrDB, firstRec, &attr, NULL, NULL);
		if(globals->CurrentCategory != dmAllCategories)
		{
			if ((attr & dmRecAttrCategoryMask) != globals->CurrentCategory)
				hasRecords = false;
		}
		if((globals->gScrollMode==FIVEWAYPAGE || globals->gScrollMode == FIVEWAYRECORD) && globals->CurrentRecord==noRecord && hasRecords)
		{
			globals->gScrollMode=FIVEWAYRECORD;
			globals->CurrentRecord=globals->TopVisibleRecord;
			PrvListSelectRecord(FrmGetActiveForm(), globals->CurrentRecord, true, true);
			return;
		}
	}
	if(globals->CurrentRecord!=noRecord)
	{
		CstGoToCurrent();
	}
}

static Boolean Generic5WayLeft(UInt16 length, RectangleType rect)
{
	globalVars* globals = getGlobalsPtr();
	if(!globals->gOneHanded && !globals->gDeviceFlags.bits.treo && globals->FiveWayUpDown==FIVEWAYCONTACTS)
	{
		if(globals->gScrollMode!=FIVEWAYRECORD)
		{
			if(length == 0 && globals->gNavigation)
			{
				globals->gListTableActive = false;
				globals->gReturnFocusToTable = false;
				FrmGlueNavDrawFocusRing(globals->adxtLibRef, FrmGetActiveForm(), ListTable, frmNavFocusRingNoExtraInfo, &rect, frmNavFocusRingStyleHorizontalBars, false);
	
				return true;
			}
		}
	}
	else if(globals->gDeviceFlags.bits.treo && !globals->gOneHanded || (length == 0 && globals->gNavigation))
	{
		globals->gListTableActive = false;
		globals->gReturnFocusToTable = false;
		return false;
	}
	globals->gReturnFocusToTable = true;
	AddRemoveLetter();
	return true;
}

static Boolean ListOnKeyDown(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP;
	Boolean tblFocus = false, fldFocus = false;
	UInt16 focusIndex = FrmGetFocus(FrmGetActiveForm());
	UInt16 tableIndex = FrmGetObjectIndex(FrmGetActiveForm(), ListTable);
	UInt16 length;
	UInt16 firstRec = noRecord;
	FieldType* fldP;
	TablePtr tblP;
	RectangleType rect;

#ifdef DEBUG
	LogWrite("xt_log", "list", "ListOnKeyDown");
#endif
	
	frmP = FrmGetActiveForm ();
	fldP = CustomGetObjectPtrSmp(ListLookupField);
	tblP = CustomGetObjectPtrSmp(ListTable);
	TblGetBounds(tblP, &rect);
			
	length = FldGetTextLength(fldP);
	
	if(length == 0 && event->data.keyDown.chr == 8)
		return true;
	
	if(focusIndex != noFocus)
	{
		if(FrmGetObjectId(FrmGetActiveForm(), focusIndex) == ListTable)
			tblFocus = true;
		else if(FrmGetObjectId(FrmGetActiveForm(), focusIndex) == ListLookupField)
			fldFocus = true;
	}
	else
	{
		FrmSetFocus(FrmGetActiveForm(), tableIndex);
		tblFocus = true;
	}
	
	if(fldFocus && (event->data.keyDown.chr==vchrRockerCenter || (IsFiveWayNavEvent(event) && ToolsNavKeyPressed(globals->adxtLibRef, event, Select))))
	{
		FrmSetFocus(FrmGetActiveForm(), tableIndex);
		tblFocus = true;
		fldFocus = false;
	}
	
	if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr) &&  globals->startupType == startupNormal)
	{
		if(globals->gDeviceFlags.bits.treoWithPhoneKeyOnly && event->data.keyDown.chr == vchrHard1)
		{
			if(!PrvListDialCurrentRecord(globals->CurrentRecord))
			{
				StartPhone();
			}			
			return true;
		}
		else if(globals->gDeviceFlags.bits.treoWithSendKeys && event->data.keyDown.chr == vchrHard11)
		{
			PrvListDialCurrentRecord(globals->CurrentRecord);
			return true;		
		}
		if (!(event->data.keyDown.modifiers & poweredOnKeyMask))
		{
			PrvListClearLookupString ();
			PrvListNextCategory ();
			return true;
		}
	}
	if (event->data.keyDown.chr == linefeedChr)
	{
		if(globals->CurrentRecord!=noRecord)
		{
			if(globals->startupType == startupNormal)
				CstGoToCurrent();
			else if(globals->startupType == startupLookup)
			{
				if(globals->SelectedLookupRecord != noRecord)
				{
					ListLookupOnOK();
					return true;
				} 
			}	
		}
		return true;
	}
	if(!globals->gNavigation)
	{
		if (event->data.keyDown.chr==vchrRockerCenter)
		{
			Generic5WayCenter();
			return true;
		}
	    if (event->data.keyDown.chr==vchrRockerRight)
		{
			Generic5WayRight();
			return true;
		}
		if (event->data.keyDown.chr==vchrRockerLeft)
		{
			Generic5WayLeft(length, rect);
			return true;
		}
		if (event->data.keyDown.chr==vchrRockerUp)
		{
			Generic5WayUp(event);
			return true;
		}
		if (event->data.keyDown.chr==vchrRockerDown) 
		{
			Generic5WayDown(event);
			return true;
		}	
	}
	if(IsFiveWayNavEvent(event) && globals->gNavigation && !globals->gListTableActive && tblFocus)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			Boolean hasRecords = true;
			UInt16 attr;
			Boolean lastTableActive;
			
			firstRec = globals->CurrentRecord;
			if(firstRec == noRecord)
				firstRec = globals->TopVisibleRecord;
			DmRecordInfo (globals->AddrDB, firstRec, &attr, NULL, NULL);
			if(globals->CurrentCategory != dmAllCategories)
			{
				if ((attr & dmRecAttrCategoryMask) != globals->CurrentCategory)
					hasRecords = false;
			}
			FrmGlueNavRemoveFocusRing(globals->adxtLibRef, FrmGetActiveForm());
			lastTableActive = globals->gListTableActive;
			globals->gListTableActive = true;		
			if(globals->CurrentRecord == noRecord && globals->gNavigation && hasRecords)
			{
				if(globals->FiveWayUpDown==FIVEWAYCONTACTS && globals->gScrollMode == FIVEWAYRECORD)
				{
					globals->CurrentRecord=globals->TopVisibleRecord;
					PrvListSelectRecord(FrmGetActiveForm(), globals->CurrentRecord, true, true);
					return true;
				}
				else if(globals->FiveWayUpDown==FIVEWAYCONTACTS && lastTableActive && (globals->gScrollMode == FIVEWAYPAGE && globals->CurrentRecord == noRecord))
				{
					globals->gScrollMode = FIVEWAYRECORD;
					globals->CurrentRecord=globals->TopVisibleRecord;
					PrvListSelectRecord(FrmGetActiveForm(), globals->CurrentRecord, true, true);
					return true;
				}
				else if(globals->FiveWayUpDown==FIVEWAYRECORD)
				{
					globals->CurrentRecord=globals->TopVisibleRecord;
					PrvListSelectRecord(FrmGetActiveForm(), globals->CurrentRecord, true, true);
					return true;
				}
			}
			return true;
		}
	}
	
	if(ToolsIsFiveWayNavEvent(globals->adxtLibRef, event))
	{
		if(globals->gNavigation && !globals->gListTableActive)
		{
			return false;
		}
		if ((!globals->gNavigation || tblFocus) && ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			Generic5WayCenter();
			return true;
		}
    	if ((!globals->gNavigation || tblFocus) && ToolsNavKeyPressed(globals->adxtLibRef, event, Left))
		{
			return Generic5WayLeft(length, rect);
		}
		if ((!globals->gNavigation || tblFocus) && ToolsNavKeyPressed(globals->adxtLibRef, event, Right))
		{
			return Generic5WayRight();			
		}
		if ((!globals->gNavigation || tblFocus) && (ToolsNavKeyPressed(globals->adxtLibRef, event, Up) || (event->data.keyDown.chr==vchrPageUp)))
		{
			return Generic5WayUp(event);
		}
		if ((!globals->gNavigation || tblFocus) && (ToolsNavKeyPressed(globals->adxtLibRef, event, Down) || (event->data.keyDown.chr==vchrPageDown))) 
		{
			return Generic5WayDown(event);
		}		
	} 
	
	if (EvtKeydownIsVirtual(event) && !globals->gNavigation)
	{
		switch (event->data.keyDown.chr)
		{	
			case vchrSendData:
				if (globals->CurrentRecord != noRecord)
				{
					MenuEraseStatus (0);
					TransferSendRecord(globals->AddrDB, globals->CurrentRecord, exgBeamPrefix, NoDataToBeamAlert);
				}
				else
					SndPlaySystemSound (sndError);
				return true;
			case vchrPageUp:
				if(globals->StdUpDown==STDRECORD)
				{
					CstSelUpDown(dmSeekBackward, true);
				}
				else
				 {
					// Reset scroll rate if not auto repeating
					if ((event->data.keyDown.modifiers & autoRepeatKeyMask) == 0)
					{
						PrvListResetScrollRate();
					}
					// Adjust the scroll rate if necessary
					PrvListAdjustScrollRate();
					PrvListScroll (winUp, globals->ScrollUnits, false);
					PrvListClearLookupString ();
				}
				return true;

			case vchrPageDown:
				if(globals->StdUpDown==STDRECORD)
				{
					CstSelUpDown(dmSeekForward, true);
				}
				else 
				{
					// Reset scroll rate if not auto repeating
					if ((event->data.keyDown.modifiers & autoRepeatKeyMask) == 0)
					{
						PrvListResetScrollRate();
					}
					// Adjust the scroll rate if necessary
					PrvListAdjustScrollRate();
					PrvListScroll (winDown, globals->ScrollUnits, false);
					PrvListClearLookupString ();
				}
				return true;
			
			case vchrT3SliderOpen:
				// force it
				dia_close();
				// form will be repainted after sysNotifyDisplayResizedEvent
				return true;
		}
	}
	else
	{
		Boolean handled;
		UInt16 foundRecord;
		if(globals->gNavigation)
		{
			if(event->data.keyDown.chr==vchrHardRockerCenter)
				return false;
			foundRecord = PrvListLookupString(event);
			if(foundRecord == noRecord)
			{
				globals->suppressListFocusEvent = true;
				if(tblFocus)
				{
					globals->gReturnFocusToTable = true;
					FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, ListTable);
				}
			}
			else
			{
				globals->suppressListFocusEvent = true;
				if(!fldFocus)
				{
					globals->gReturnFocusToTable = true;
					globals->gListTableActive = true;
					FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, ListTable);
				}
				if(globals->CurrentRecord != foundRecord)
					PrvListSelectRecord(frmP, foundRecord, true, true);
			}
			return true;
			
		}
		else
		{
			foundRecord = PrvListLookupString(event);
			//SysTaskDelay(100);
			if(foundRecord != noRecord)
				PrvListSelectRecord(frmP, foundRecord, true, true);
		}
		return true;
	}
	return false;
}

/*static Boolean ListOnPenUpEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if((globals->startupType != startupNormal) && (event->data.penUp.end.x<61)&&(event->data.penUp.end.y<17)&&!globals->gMenuActive)
	{
		EventType ev;
		ev.eType=keyDownEvent;
		ev.data.keyDown.chr=vchrMenu;
		ev.data.keyDown.modifiers=0;
		EvtAddEventToQueue(&ev);
		return true;
	} 
	else
		return false;
}
*/

static Boolean ListOnWinExitEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if(event->data.winEnter.exitWindow==FrmGetWindowHandle(FrmGetActiveForm()))
    	globals->gMenuActive=true;
	return true;
}

static Boolean ListOnFrmCloseEvent()
{
	globalVars* globals = getGlobalsPtr();
	if (globals->UnnamedRecordStringPtr)
	{
		MemPtrUnlock(globals->UnnamedRecordStringPtr);
		globals->UnnamedRecordStringPtr = NULL;
	}

	if (globals->UnnamedRecordStringH)
	{
		DmReleaseResource(globals->UnnamedRecordStringH);
		globals->UnnamedRecordStringH = NULL;
	}
	return false;
}

static Boolean ListOnTblSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if(globals->startupType != startupNormal)
	{
		return true;
	}
	if (TblRowMasked(event->data.tblSelect.pTable,
						 event->data.tblSelect.row))
	{
		if (SecVerifyPW (showPrivateRecords) == true)
		{
			// We only want to unmask this one record, so restore the preference.
			PrefSetPreference (prefShowPrivateRecords, maskPrivateRecords);

			event->data.tblSelect.column = nameAndNumColumn; //force non-note view
		}
		else
			return false;
	}
	else
	{
		return true;
	}	
	// An item in the list of names and phone numbers was selected, go to
	// the record view.
	globals->CurrentRecord = TblGetRowID (event->data.tblSelect.pTable,
								 event->data.tblSelect.row);

	// Set the global variable that determines which field is the top visible
	// field in the edit view.  Also done when New is pressed.
	globals->TopVisibleFieldIndex = 0;
	globals->EditRowIDWhichHadFocus = editFirstFieldIndex;
	globals->EditFieldPosition = 0;

	if (event->data.tblSelect.column == nameAndNumColumn)
	{
		FrmGotoForm (RecordView);
	}
	else if (NoteViewCreate())
	{
		FrmGotoForm (NewNoteView);
	}
	return true;
}	

static Boolean ListOnCtlEnterEvent(EventType* event)
{
	switch (event->data.ctlEnter.controlID)
	{
		case ListUpButton:
		case ListDownButton:
			// Reset scroll rate
			PrvListResetScrollRate();
			// Clear lookup string
			PrvListClearLookupString ();
			// leave unhandled so the buttons can repeat
			break;
	}	
	return false;
}

void ListLookupOnCancel()
{
	globalVars* globals = getGlobalsPtr();
	EventType newEvent;
	newEvent.eType = LOOKUP_END_EVENT;
	globals->SelectedLookupRecord = noRecord;
	EvtAddEventToQueue (&newEvent);				
}

void ListLookupOnOK()
{
	globalVars* globals = getGlobalsPtr();
	EventType newEvent;
	newEvent.eType = LOOKUP_END_EVENT;
	EvtAddEventToQueue (&newEvent);				
}

static Boolean ListOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
		case ListLookupCancel:
			ListLookupOnCancel();
			break;
		case ListLookupAdd:
			ListLookupOnOK();
			break;
		case ListCategoryTrigger:
			PrvListSelectCategory ();
			handled = true;
			break;
		case ListNewButton:
			globals->CurrentRecord = noRecord;
			EditNewRecord();
			handled = true;
			break;			
		case ListRecentButton:
			{
				UInt16 sel;
				UInt16 i=PrepareRecentList();
				if(i)
				{
					sel=LstPopupList(GetObjectPtrSmp(ListRecentList));
					CleanRecentList(i);
					if(sel!=-1)
					{
						if(globals->gShowAll && globals->CurrentCategory!= dmAllCategories)
						{
							UInt16 attr;
							SeekRecentInCategory(globals->adxtLibRef, dmAllCategories, &sel);
							DmRecordInfo(globals->AddrDB, sel, &attr, NULL, NULL);
							globals->CurrentCategory = attr & dmRecAttrCategoryMask;
							SetCategoryLabel(globals->CurrentCategory, true);
						}
							
						else
						{
							SeekRecentInCategory(globals->adxtLibRef, globals->CurrentCategory, &sel);
						}
						globals->CurrentRecord=sel;
						FrmGotoForm(RecordView);
					
					}
				}
			}
			handled=true;
			break;
	}
	return handled;
}

static Boolean ListOnCtlRepeatEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	// Adjust the scroll rate if necessary
	PrvListAdjustScrollRate();

	switch (event->data.ctlRepeat.controlID)
	{
		case ListUpButton:
			PrvListScroll (winUp, globals->ScrollUnits, false);
			// leave unhandled so the buttons can repeat
			break;

		case ListDownButton:
			PrvListScroll (winDown, globals->ScrollUnits, false);
			// leave unhandled so the buttons can repeat
			break;
		default:
			break;
	}
	return false;
}


static Boolean ListOnFrmObjectFocusLostEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if(event->data.frmObjectFocusLost.objectID == ListTable && globals->gNavigation && !globals->suppressListFocusEvent)
	{
		globals->gListTableActive = false;
		PrvListSelectRecord(FrmGetActiveForm(), globals->CurrentRecord, true, true);
	}
	return false;
}

static Boolean ListOnFrmObjectFocusTakeEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	TableType* tblP;
	RectangleType rect;
	if(globals->gNavigation)
	{
		switch(event->data.frmObjectFocusTake.objectID)
		{
			case ListTable:
				tblP = CustomGetObjectPtrSmp(ListTable);
				TblGetBounds(tblP, &rect);
				//if(globals->gReturnFocusToTable == true)
				{
					globals->gReturnFocusToTable = false;
					globals->gListTableActive = true;
				}
				if(globals->gNavigation && !globals->suppressListFocusEvent)
				{
					PrvListSelectRecord(FrmGetActiveForm(), globals->CurrentRecord, true, true);
				}
				globals->suppressListFocusEvent = false;
				//if(globals->gListTableActive == false)
				//	FrmGlueNavDrawFocusRing(globals->adxtLibRef, FrmGetActiveForm(), ListTable, frmNavFocusRingNoExtraInfo, &rect, frmNavFocusRingStyleHorizontalBars, false);
				FrmSetFocus(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ListTable));
				
				return true;
				break;
			default:
				return false;
				break;
		}
	}
	return false;
}

static Boolean ListOnFldChangedEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	PrvListLookupString(event);
	if(globals->gReturnFocusToTable && globals->gNavigation)
	{
		globals->gReturnFocusToTable = false;
		FrmGlueNavObjectTakeFocus(globals->adxtLibRef, FrmGetActiveForm(), ListTable);
	}
	return true;
}

static Boolean ListOnWinEnterEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if(event->data.winEnter.enterWindow==FrmGetWindowHandle(FrmGetActiveForm()))
    	globals->gMenuActive=false;
	
    if(event->data.winEnter.enterWindow != 0)
    	dia_win_enter();
    return false;
}

Boolean ListHandleEvent (EventType* event)
{
	Boolean handled = false;
						
	switch (event->eType)
	{
	case winExitEvent:
		handled = ListOnWinExitEvent(event);
		break;	
	case winEnterEvent:
    	handled = ListOnWinEnterEvent(event);
    	break;	
	case frmOpenEvent:
		handled = ListOnOpen();
		break;
	case frmCloseEvent:
		handled = ListOnFrmCloseEvent();
		break;
	case tblEnterEvent:
		PrvListHandleRecordSelection (event);
		handled=true;
		break;
	case tblSelectEvent:
		handled = ListOnTblSelectEvent(event);
		break;
	case ctlEnterEvent:
		handled = ListOnCtlEnterEvent(event);
		break;
	case ctlSelectEvent:
		handled = ListOnCtlSelectEvent(event);
		break;
	case ctlRepeatEvent:
		handled = ListOnCtlRepeatEvent(event);
		break;
	case frmObjectFocusLostEvent:
		handled = ListOnFrmObjectFocusLostEvent(event);		
		break;
	case frmObjectFocusTakeEvent:
		handled = ListOnFrmObjectFocusTakeEvent(event);		
		break;
	case keyDownEvent:
		handled = ListOnKeyDown(event);
		break;
	case fldChangedEvent:
		handled = ListOnFldChangedEvent(event);		
		break;
	case menuEvent:
		handled = ListOnMenuEvent(event);		
		break;
	case menuCmdBarOpenEvent:
		handled = ListOnMenuCmdBarOpenEvent(event);		
		break;
	case menuOpenEvent:
		handled = ListOnMenuOpenEvent();		
		break;
	case frmUpdateEvent:
		handled = ListOnUpdate(event);
		break;
	default:
		handled = dia_handle_event(event, AddrListMoveObjects);
		break;
	}

	return (handled);
}

void PrvListInit( FormType* frmP )
{
	globalVars* globals = getGlobalsPtr();
	UInt16 row;
	UInt16 width, height;
	UInt16 rowsInTable;
	TableType* tblP;
	RectangleType r1;

#ifdef DEBUG
	LogWrite("xt_log", "list", "PrvListInit");
#endif

	if (globals->ShowAllCategories)
		globals->CurrentCategory = dmAllCategories;
	width=GetWindowWidth();
	height=GetWindowHeight();
 	tblP = CustomGetObjectPtrSmp(ListTable);
	TblEraseTable(tblP);
	RctSetRectangle(&r1, 0, 18, width, height - 33);
	
	TblSetBounds(tblP, &r1);
		
	if(globals->gNavigation)
	{
		UInt16 focus;
		if(FrmGlueNavGetFocusRingInfo (globals->adxtLibRef, frmP, &focus, NULL, NULL, NULL) != errNone)
       		focus = noFocus;
	 	if(globals->gNavigation && focus == ListTable)
	    {
	  		FrmGlueNavRemoveFocusRing(globals->adxtLibRef, frmP);
	  		FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, focus);
	  	}
	}
	
	TblSetColumnWidth(tblP, nameAndNumColumn, width-8);
	TblSetColumnWidth(tblP, noteColumn, 7);
	
	rowsInTable = TblGetNumberOfRows(tblP);
	for (row = PrvListNumberOfRows(tblP); row < rowsInTable; row++)
	{
		TblSetRowSelectable(tblP, row, false);
	}
	
	
	FntSetFont (globals->AddrListFont);
			
	for (row = 0; row < PrvListNumberOfRows(tblP); row++)
	{
		Coord height;
		TblSetItemStyle(tblP, row, nameAndNumColumn, customTableItem);
		TblSetItemStyle(tblP, row, noteColumn, customTableItem);
		TblSetRowUsable(tblP, row, false);
		TblSetRowSelectable(tblP, row, true);
		if(globals->AddrListHighRes && (/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320))
		{
			height = (FntLineHeight() + 1) >> 1;
		}		
		else
		{
			height = FntLineHeight();
		}
		if(globals->gTouchMode)
			height *= 1.25;
		TblSetRowHeight(tblP, row, height);
	}
	     
    TblSetColumnUsable(tblP, nameAndNumColumn, true);
	TblSetColumnUsable(tblP, noteColumn, true);

	TblSetColumnMasked(tblP, nameAndNumColumn, true);
	TblSetColumnMasked(tblP, noteColumn, true);

	// Set the callback routine that will draw the records.
	TblSetCustomDrawProcedure (tblP, nameAndNumColumn, PrvListDrawRecord);
	TblSetCustomDrawProcedure (tblP, noteColumn, PrvListDrawRecord);


	// Load records into the address list.
	PrvListLoadTable(frmP, true);

	SetCategoryLabel(globals->CurrentCategory, false);	
}

void PrvListResetScrollRate(void)
{
	globalVars* globals = getGlobalsPtr();
	// Reset last seconds
	globals->LastSeconds = TimGetSeconds();
	// Reset scroll units
	globals->ScrollUnits = 1;
}


/***********************************************************************
 *
 * FUNCTION:    PrvListAdjustScrollRate
 *
 * DESCRIPTION: This routine adjusts the scroll rate based on the current
 *              scroll rate, given a certain delay, and plays a sound
 *              to notify the user of the change
 *
 ***********************************************************************/
void PrvListAdjustScrollRate(void)
{
	globalVars* globals = getGlobalsPtr();
	// Accelerate the scroll rate every 3 seconds if not already at max scroll speed
	UInt16 newSeconds = TimGetSeconds();
	if ((globals->ScrollUnits < scrollSpeedLimit) && ((newSeconds - globals->LastSeconds) > scrollDelay))
	{
		// Save new seconds
		globals->LastSeconds = newSeconds;

		// increase scroll units
		globals->ScrollUnits += scrollAcceleration;
	}

}

Boolean RefreshLookup (Boolean refresh)
{
	globalVars* globals = getGlobalsPtr();
	FormType* frmP;
	UInt16 fldIndex;
	FieldPtr fldP;
	Char * fldTextP;
	TablePtr tableP;
	UInt16 foundRecord;
	Boolean completeMatch;
	Boolean findResults;
	Boolean advancedFind = false;
	
	if(globals->gAdvancedFind == ADVANCEDFIND_ON)
		advancedFind = true;

	frmP = FrmGetActiveForm();
	fldIndex = FrmGetObjectIndex(frmP, ListLookupField);
	
	if(refresh && !globals->gNavigation)
	{
		FrmSetFocus(frmP, fldIndex);
	}
	
	fldP = FrmGetObjectPtr (frmP, fldIndex);


	fldTextP = FldGetTextPtr(fldP);
	tableP = CustomGetObjectPtrSmp(ListTable);
	
	if(advancedFind)
	{
		findResults = AddrDBLookupStringEx(globals->AddrDB, fldTextP, globals->SortByCompany,
						  globals->CurrentCategory, &foundRecord, &completeMatch,
						  (globals->PrivateRecordVisualStatus == maskPrivateRecords), 10, noRecord, 0);
	}
	else
	{
		findResults = AddrDBLookupString(globals->adxtLibRef, globals->AddrDB, fldTextP, globals->SortByCompany,
								  globals->CurrentCategory, &foundRecord, &completeMatch,
								  (globals->PrivateRecordVisualStatus == maskPrivateRecords));
	}
	
	if (findResults)
	{
		PrvListSelectRecord(frmP, foundRecord, false, refresh);
	}
	return true;
}

Boolean RefreshLookupEx (Boolean refresh, WinDirectionType direction)
{
	globalVars* globals = getGlobalsPtr();
	FormType* frmP;
	UInt16 fldIndex;
	FieldPtr fldP;
	Char * fldTextP;
	TablePtr tableP;
	UInt16 foundRecord;
	Boolean completeMatch;
	Boolean advancedFind = false;
	Boolean findResults;
	
	if(globals->gAdvancedFind == ADVANCEDFIND_ON)
		advancedFind = true;
	
	frmP = FrmGetActiveForm();
	fldIndex = FrmGetObjectIndex(frmP, ListLookupField);
	
	if(!globals->gNavigation)
		FrmSetFocus(frmP, fldIndex);
	
	fldP = FrmGetObjectPtr (frmP, fldIndex);


	fldTextP = FldGetTextPtr(fldP);
	tableP = CustomGetObjectPtrSmp(ListTable);
	
	if(advancedFind)
	{
		findResults = AddrDBLookupStringEx(globals->AddrDB, fldTextP, globals->SortByCompany,
						  globals->CurrentCategory, &foundRecord, &completeMatch,
						  (globals->PrivateRecordVisualStatus == maskPrivateRecords), 10, globals->CurrentRecord, direction);
	}
	else
	{
		findResults = AddrDBLookupString(globals->adxtLibRef, globals->AddrDB, fldTextP, globals->SortByCompany,
								  globals->CurrentCategory, &foundRecord, &completeMatch,
								  (globals->PrivateRecordVisualStatus == maskPrivateRecords));
	}
	if (!findResults)
	{
		globals->CurrentRecord = noRecord;
		globals->SelectedRecord = noRecord;
		if(refresh)
			TblDrawTable(tableP);
	}
	else
	{
		
		PrvListSelectRecord(frmP, foundRecord, false, refresh);
	}
	return true;
}

/***********************************************************************
 *
 * FUNCTION:    PrvListLookupString
 *
 * DESCRIPTION: Adds a character to ListLookupField, looks up the
 * string in the database and selects the item that matches.
 *
 * PARAMETERS:  event - EventType* containing character to add to ListLookupField
 *
 * RETURNED:    true if the field handled the event
 ***********************************************************************/
UInt16 PrvListLookupString (EventType * event)
{
	globalVars* globals = getGlobalsPtr();
	FormType* frmP;
	UInt16 fldIndex;
	FieldPtr fldP;
	Char * fldTextP;
	TablePtr tableP;
	UInt16 foundRecord;
	Boolean completeMatch;
	Boolean advancedFind = false;
	
	if(globals->gAdvancedFind == ADVANCEDFIND_ON)
		advancedFind = true;
	
	frmP = FrmGetActiveForm();
	fldIndex = FrmGetObjectIndex(frmP, ListLookupField);
	
	FrmSetFocus(frmP, fldIndex);
	fldP = FrmGetObjectPtr (frmP, fldIndex);

	if (FldHandleEvent (fldP, event) || event->eType == fldChangedEvent)
	{
		fldTextP = FldGetTextPtr(fldP);
		tableP = CustomGetObjectPtrSmp(ListTable);
		PrvListLoadTable(frmP, true);
		//TblRedrawTable(tableP);
		
		advancedFind = true;
		if(advancedFind)
		{
			if (!AddrDBLookupString(globals->adxtLibRef, globals->AddrDB, fldTextP, globals->SortByCompany,
								  globals->CurrentCategory, &foundRecord, &completeMatch,
								  (globals->PrivateRecordVisualStatus == maskPrivateRecords)))
			{
				if (!AddrDBLookupStringEx(globals->AddrDB, fldTextP, globals->SortByCompany,
								  globals->CurrentCategory, &foundRecord, &completeMatch,
								  (globals->PrivateRecordVisualStatus == maskPrivateRecords), 10, noRecord, 0))
			
				{
					// If the user deleted the lookup text remove the
					// highlight.
					//globals->CurrentRecord = noRecord;
					//globals->SelectedRecord=noRecord;
					//TblDrawTable(tableP);
					return noRecord;
				}
				else
				{
					return foundRecord;//PrvListSelectRecord(frmP, foundRecord, true, true);
				}
			}
			else
			{
				UInt16 rectemp = foundRecord;
				if(!completeMatch)
				{
					if (AddrDBLookupStringEx(globals->AddrDB, fldTextP, globals->SortByCompany,
								  globals->CurrentCategory, &foundRecord, &completeMatch,
								  (globals->PrivateRecordVisualStatus == maskPrivateRecords), 10, noRecord, 0))
			
					{
						return foundRecord;//PrvListSelectRecord(frmP, foundRecord, true, true);
					}
					else
					{
						foundRecord = rectemp;
						return foundRecord;//PrvListSelectRecord(frmP, foundRecord, true, true);
					}
				}
				else
				{
					return foundRecord;//PrvListSelectRecord(frmP, foundRecord, true, true);
				}			
			}
		}
		else
		{
			if (!AddrDBLookupString(globals->adxtLibRef, globals->AddrDB, fldTextP, globals->SortByCompany,
								  globals->CurrentCategory, &foundRecord, &completeMatch,
								  (globals->PrivateRecordVisualStatus == maskPrivateRecords)))
				{
					// If the user deleted the lookup text remove the
					// highlight.
					//globals->CurrentRecord = noRecord;
					//globals->SelectedRecord=noRecord;
					//TblDrawTable(tableP);
					return noRecord;
				}
				else
				{
					return foundRecord;//PrvListSelectRecord(frmP, foundRecord, true, true);
				}		
		}
		return true;
	}
	
	return noRecord;
}

void PrvListDrawRecord (void * table, Int16 row, Int16 column, RectanglePtr bounds)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 recordNum;
	Err error;
	univAddrDBRecordType record;
	MemHandle recordH;
	char noteChar;
	FontID currFont;
	RectangleType rect2, rect3, rect4;
	Int16 phoneX;
	IndexedColorType oldColor;
	UInt32 addrId;
	UInt16 linkCount;
	Boolean recent=false;

#ifdef DEBUG
	if(row == 0 && column == 0)
		LogWrite("xt_log", "list", "PrvListDrawRecord cell(0:0)");
#endif
	
	// Get the record number that corresponds to the table item to draw.
	// The record number is stored in the "intValue" field of the item.
	//
	recordNum = TblGetRowID (table, row);
	RctCopyRectangle(bounds, &rect2);
	TblGetBounds(table, &rect3);
	rect2.extent.y=rect3.extent.y;
	RctCopyRectangle(bounds, &rect4);
	rect4.extent.x+=1;
	
	oldColor=UIColorGetTableEntryIndex(UIFormFill);
	if(globals->gEachOtherSelected && column==0)
	{
		if(row%2)
		{
			WinSetBackColor(globals->gColorBack);
		}
		else
		{
			WinSetBackColor(globals->gEachOther);
		}
	}
	WinEraseRectangle(&rect4, 0);
	
	DmRecordInfo(globals->AddrDB, recordNum, 0, &addrId, 0);
	
	error = univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, recordNum, &record, &recordH);		
	
	if (error)
	{
		ErrNonFatalDisplay ("Record not found");
		return;
	}
	
	switch (column)
	{
		case nameAndNumColumn:
			linkCount = GetLinkCount(globals->adxtLibRef, globals->linksDB, addrId);
			
			ToolsFntSetFont(globals->adxtLibRef, globals->refNum, globals->AddrListHighRes, globals->gScreen, globals->AddrListFont);
			
			if(linkCount > 0)
			{
				MemHandle resH;
				BitmapType *bitmap;
				
				if(!globals->AddrListHighRes)
				{
					if(globals->gScreen==PALM_320x320)
						resH = DmGetResource (bitmapRsc, ListLinkBmpPalmHRStd);
					//else if (globals->gScreen==CLIE_320x320)
					//	resH = DmGetResource (bitmapRsc, ListLinkBmpStd);
					else if(globals->gScreen==PALM_160x160)
						resH = DmGetResource (bitmapRsc, ListLinkBmpSmall);
					bitmap = MemHandleLock (resH);
				}
				/*else if (globals->gScreen==CLIE_320x320)
				{
					resH = DmGetResource (bitmapRsc, ListLinkBmpSmall);
					bitmap = MemHandleLock (resH);
				}*/
				else if(globals->gScreen==PALM_320x320)
				{
					resH = DmGetResource (bitmapRsc, ListLinkBmpPalmHRSmall);
					bitmap = MemHandleLock (resH);
				}
				if(!globals->AddrListHighRes)
				{
					//if(globals->gScreen==CLIE_320x320)
					//	HRWinDrawBitmap(globals->refNum, bitmap,  bounds->topLeft.x<<1, (bounds->topLeft.y+1)<<1);
					//else
						WinDrawBitmap(bitmap,  bounds->topLeft.x, bounds->topLeft.y+2);
				}
				/*else if (globals->gScreen==CLIE_320x320)
				{
					HRWinDrawBitmap(globals->refNum, bitmap,  bounds->topLeft.x<<1, (bounds->topLeft.y+1)<<1);
				}*/
				else if (globals->gScreen==PALM_320x320)
				{
					WinDrawBitmap(bitmap,  bounds->topLeft.x, bounds->topLeft.y+1);
				}	
				MemHandleUnlock(resH);
				DmReleaseResource(resH);
				
				if(!globals->AddrListHighRes)
				{
					bounds->topLeft.x += LINKSICON_WIDTH;
					bounds->extent.x -= LINKSICON_WIDTH;
				}
				else
				{
					bounds->topLeft.x += LINKSICON_WIDTH/2;
					bounds->extent.x -= LINKSICON_WIDTH/2;
				
				}			
			}
			
			phoneX = ToolsDrawRecordNameAndPhoneNumber (globals->adxtLibRef, &record, bounds, globals->PhoneLabelLetters, globals->SortByCompany, &globals->UnnamedRecordStringPtr, &globals->UnnamedRecordStringH, !globals->AddrListHighRes);
			
			TblSetRowData(table, row, phoneX);			// Store in table for later tap testing
			break;
		case noteColumn:
			// Draw a note symbol if the field has a note.
			if (record.fields[univNote] != NULL)
			{
				MemHandle resH;
				BitmapType *bitmap;
				
				if(!globals->AddrListHighRes)
				{
					if(globals->gDepth<8)
					{
						currFont = FntSetFont (symbolFont);
					}
					else
					{
					if(globals->gScreen==PALM_320x320)
						resH = DmGetResource (bitmapRsc, ListNoteBmpPalmHRStd);
					//else if (globals->gScreen==CLIE_320x320)
					//	resH = DmGetResource (bitmapRsc, ListNoteBmpStd);
					else if(globals->gScreen==PALM_160x160)
						resH = DmGetResource (bitmapRsc, ListNoteBmpSmall);
					bitmap = MemHandleLock (resH);
					}	
				}
				/*else if (globals->gScreen==CLIE_320x320)
				{
					if(globals->gDepth<8)
					{
						currFont=HRFntSetFont(globals->refNum, symbolFont);
					}
					else
					{
						resH = DmGetResource (bitmapRsc, ListNoteBmpSmall);
						bitmap = MemHandleLock (resH);
					}
				}*/
				else if(globals->gScreen==PALM_320x320)
				{
					if(globals->gDepth<8)
					{
						currFont=FntSetFont(symbolFont);
					}
					else
					{
						resH = DmGetResource (bitmapRsc, ListNoteBmpPalmHRSmall);
						bitmap = MemHandleLock (resH);
					}
				}
				noteChar = symbolNote;
				if(!globals->AddrListHighRes)
				{
					if(globals->gDepth<8)
					{
						WinDrawChars (&noteChar, 1, bounds->topLeft.x, bounds->topLeft.y);
						FntSetFont (currFont);
					}
					else
					{	
						//if(globals->gScreen==CLIE_320x320)
						//	HRWinDrawBitmap(globals->refNum, bitmap,  bounds->topLeft.x<<1, (bounds->topLeft.y+1)<<1);
						//else
							WinDrawBitmap(bitmap,  bounds->topLeft.x, bounds->topLeft.y+2);
					}
				}
				/*else if (globals->gScreen==CLIE_320x320)
				{
					if(globals->gDepth<8)
					{
						HRWinDrawChars (globals->refNum, &noteChar, 1, bounds->topLeft.x<<1, bounds->topLeft.y<<1);
						HRFntSetFont (globals->refNum, currFont);
					}
					else
					{	
						HRWinDrawBitmap(globals->refNum, bitmap,  bounds->topLeft.x<<1, (bounds->topLeft.y+1)<<1);
					}
				}*/
				else if (globals->gScreen==PALM_320x320)
				{
					if(globals->gDepth<8)
					{
						ToolsWinDrawCharsHD (globals->adxtLibRef, &noteChar, 1, bounds->topLeft.x, bounds->topLeft.y);
						FntSetFont (currFont);
					}
					else
					{
						WinDrawBitmap(bitmap,  bounds->topLeft.x, bounds->topLeft.y+1);
					}
				}	
				if(globals->gDepth>=8)
				{
					MemHandleUnlock(resH);
					DmReleaseResource(resH);
				}
			}
		break;
	}
	MemHandleUnlock(recordH);
	if(globals->gEachOtherSelected)
	{
		WinSetBackColor(oldColor);
	}	
}


/***********************************************************************
 *
 * FUNCTION:    PrvListClearLookupString
 *
 * DESCRIPTION: Clears the ListLookupField.  Does not unhighlight the item.
 ***********************************************************************/
void PrvListClearLookupString ()
{
	FormType* frmP;
	FieldType* fldP;
	Int16 length;

	frmP = FrmGetActiveForm();
	FrmSetFocus(frmP, noFocus);
	fldP = CustomGetObjectPtrSmp(ListLookupField);

	length = FldGetTextLength(fldP);
	if (length > 0)
	{
		// Clear it this way instead of with FldDelete to avoid sending a
		// fldChangedEvent (which would undesirably unhighlight the item).
		FldFreeMemory (fldP);
		FldDrawField (fldP);
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvListNumberOfRows
 *
 * DESCRIPTION: This routine return the maximun number of visible rows,
 *              with the current list view font setting.
 *
 * PARAMETERS:  table - List View table
 *
 * RETURNED:    maximun number of displayable rows
 ***********************************************************************/
UInt16 PrvListNumberOfRows (TablePtr table)
{
	globalVars* globals = getGlobalsPtr();
	UInt16				rows;
	UInt16				rowsInTable;
	UInt16				tableHeight;
	FontID			currFont;
	Coord height;
	RectangleType	r;


	rowsInTable = TblGetNumberOfRows (table);

	TblGetBounds (table, &r);
	tableHeight = r.extent.y;

	if(globals->AddrListHighRes && (/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320))
	{
		height = (FntLineHeight() + 1) >> 1;
	}		
	else
	{
		height = FntLineHeight();
	}
	if(globals->gTouchMode)
		height *= 1.2;

	rows = tableHeight / height;
	FntSetFont (currFont);
	if (rows <= rowsInTable)
		return (rows);
	else
		return (rowsInTable);
}

void PrvListUpdateScrollButtons( FormType* frmP )
{
	globalVars* globals = getGlobalsPtr();
	UInt16 row;
	UInt16 upIndex;
	UInt16 downIndex;
	UInt16 recordNum;
	Boolean scrollableUp;
	Boolean scrollableDown;
	TableType* tblP;

	// Update the button that scroll the list.
	//
	// If the first record displayed is not the fist record in the category,
	// enable the up scroller.
	recordNum = globals->TopVisibleRecord;
	scrollableUp = ToolsSeekRecord (globals->adxtLibRef, &recordNum, 1, dmSeekBackward);


	// Find the record in the last row of the table
	tblP = CustomGetObjectPtrSmp(ListTable);
	row = TblGetLastUsableRow(tblP);
	if (row != tblUnusableRow)
		recordNum = TblGetRowID(tblP, row);


	// If the last record displayed is not the last record in the category,
	// enable the down scroller.
	scrollableDown = ToolsSeekRecord (globals->adxtLibRef, &recordNum, 1, dmSeekForward);

	// Update the scroll button.
	upIndex = FrmGetObjectIndex(frmP, ListUpButton);
	downIndex = FrmGetObjectIndex(frmP, ListDownButton);
	FrmUpdateScrollers(frmP, upIndex, downIndex, scrollableUp, scrollableDown);
}

void PrvListLoadTable( FormType* frmP, Boolean refresh )
{
	globalVars* globals = getGlobalsPtr();
	UInt16      row, i;
	UInt16      numRows;
	FieldPtr 	fldP;
	Char		*fldTextP;
	UInt16		fldIndex, length;
	UInt16		lineHeight;
	UInt16		recordNum;
	UInt16		visibleRows;
	FontID		currFont;
	TableType* 	tblP;
	UInt16 		attr;
	Boolean		masked;
	univAddrDBRecordType record;
	MemHandle recordH;
	
#ifdef DEBUG
	LogWrite("xt_log", "list", "PrvListLoadTable");
#endif

	// For each row in the table, store the record number as the row id.
	tblP = CustomGetObjectPtrSmp(ListTable);
	// Make sure we haven't scrolled too far down the list of records
	// leaving blank lines in the table.

	// Try going forward to the last record that should be visible
	visibleRows = PrvListNumberOfRows(tblP);
	
	if(refresh)
		TblUnhighlightSelection(tblP);
	recordNum = globals->TopVisibleRecord;
	if (!ToolsSeekRecord (globals->adxtLibRef, &recordNum, visibleRows - 1, dmSeekForward))
	{
		// We have at least one line without a record.  Fix it.
		// Try going backwards one page from the last record
		globals->TopVisibleRecord = dmMaxRecordIndex;
		if (!ToolsSeekRecord (globals->adxtLibRef, &globals->TopVisibleRecord, visibleRows - 1, dmSeekBackward))
		{
			// Not enough records to fill one page.  Start with the first record
			globals->TopVisibleRecord = 0;
			ToolsSeekRecord (globals->adxtLibRef, &globals->TopVisibleRecord, 0, dmSeekForward);
		}
	}
	
	currFont = FntSetFont (globals->AddrListFont);
	lineHeight = FntLineHeight ();
	FntSetFont (currFont);

	numRows = TblGetNumberOfRows(tblP);
	recordNum = globals->TopVisibleRecord;

	
	//Dynamic lookup
	/*fldIndex = FrmGetObjectIndex(frmP, ListLookupField);	
	fldP = FrmGetObjectPtr (frmP, fldIndex);
	fldTextP = FldGetTextPtr(fldP);
	length = FldGetTextLength(fldP);
	*/
	for (row = 0; row < visibleRows; row++)
	{
		if ( ! ToolsSeekRecord (globals->adxtLibRef, &recordNum, 0, dmSeekForward))
			break;		
		/*if(length)
		{
			Char		*name1 = NULL, *name2 = NULL;
			Boolean match = true;
			SortInfo info;		
			univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, recordNum, &record, &recordH);		
			PrvDetermineRecordNameHelper(globals->adxtLibRef, globals->SortByCompany, &record, &name1, &name2);
			
			if(name1)
			{
				if(StrNCaselessCompare(name1, fldTextP, length))
					match = false;		
			}	
			if(!match && globals->gAdvancedFind)
			{
				if(name2)
				{
					if(StrNCaselessCompare(name2, fldTextP, length))
						match = false;		
				}	
			}
			MemHandleUnlock(recordH);
			if(!match)
			{
				row--;
				recordNum++;
				continue;
			}
		}*/
			
		if(refresh)
		{
			// Make the row usable.
			TblSetRowUsable (tblP, row, true);

			DmRecordInfo (globals->AddrDB, recordNum, &attr, NULL, NULL);
			masked = (((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == maskPrivateRecords));
			TblSetRowMasked(tblP,row,masked);

			// Mark the row invalid so that it will draw when we call the
			// draw routine.
			TblMarkRowInvalid (tblP, row);

			TblSetItemFont (tblP, row, nameAndNumColumn, globals->AddrListFont);
		}
		
		// Store the record number as the row id.
		TblSetRowID (tblP, row, recordNum);

		recordNum++;
	}

	// Hide the item that don't have any data.
	if(refresh)
	{
		while (row < numRows)
		{
			TblSetRowUsable (tblP, row, false);
			row++;
		}
		PrvListUpdateScrollButtons(frmP);
	}
}

void SetCategoryLabel(UInt16 category, Boolean changeCat)
{
	globalVars* globals = getGlobalsPtr();
	FormType* frmP;
	TableType* tblP;
	FontID curFont;
	UInt16 numRec;
	Char str[15];
	Char str2[15];
	Char name[dmCategoryLength];
	frmP = FrmGetActiveForm();
			
	tblP = CustomGetObjectPtrSmp(ListTable);
	
	if(changeCat)
	{
		ToolsChangeCategory (category);
	
		globals->TopVisibleRecord=0;
		DmSeekRecordInCategory(globals->AddrDB, &globals->TopVisibleRecord, 0, dmSeekForward, category);
	}
		
	// Display the new category.
	numRec=DmNumRecordsInCategory(globals->AddrDB, category);
	CategoryGetName (globals->AddrDB, category, name);
		
	StrCopy(str2, "(");
	StrIToA(str, numRec);
	StrCat(str2, str);
	StrCopy(str, ")");
	StrCat(str2, str);
		
	curFont=FntSetFont(stdFont);
	if(!globals->gEnabledRecent)
		CategoryTruncateName (name, ResLoadConstant(maxCategoryWidthID)+10-FntLineWidth(str2, StrLen(str2)));
	else
		CategoryTruncateName (name, ResLoadConstant(maxCategoryWidthID)-FntLineWidth(str2, StrLen(str2)));
	FntSetFont(curFont);
	
	StrCopy(globals->gName2, name);
	StrCat(globals->gName2, str2); 
	
	CustomSetCtlLabelPtrSmp(ListCategoryTrigger, (Char*)&globals->gName2);

}

UInt16 PrvListSelectCategory (void)
{
	globalVars* globals = getGlobalsPtr();
	FormType* frmP;
	TableType* tblP;
	UInt16 category, hasRecords = true;
	Boolean categoryEdited;
					
	// Process the category popup list.
	category = globals->CurrentCategory;

	frmP = FrmGetActiveForm();
	categoryEdited = CategorySelect (globals->AddrDB, frmP, NULL,
									 ListCategoryList, true, &category, globals->CategoryName, 1, categoryDefaultEditCategoryString);
	
	
	if (category == dmAllCategories)
		globals->ShowAllCategories = true;
	else
		globals->ShowAllCategories = false;

	if ( categoryEdited || (category != globals->CurrentCategory))
	{
		globals->ScrollPosition=0;
		SetCategoryLabel(category, true);
		PrvListLoadTable(frmP, true);
				
		tblP = CustomGetObjectPtrSmp(ListTable);
		globals->TopVisibleRecord=0;
		DmSeekRecordInCategory(globals->AddrDB, &globals->TopVisibleRecord, 0, dmSeekForward, category);
		TblEraseTable(tblP);
		TblDrawTable(tblP);
		
		PrvListClearLookupString();
		// By changing the category the current record is lost.
		globals->CurrentRecord = noRecord;				
	}
	
	if(globals->gNavigation)
	{
		UInt16 firstRec;
		UInt16 attr;
		
		firstRec = globals->CurrentRecord;
		if(firstRec == noRecord)
			firstRec = globals->TopVisibleRecord;
		if(DmNumRecordsInCategory(globals->AddrDB, dmAllCategories) < firstRec + 1)
			return (category);
		DmRecordInfo (globals->AddrDB, firstRec, &attr, NULL, NULL);
		if(globals->CurrentCategory != dmAllCategories)
		{
			if ((attr & dmRecAttrCategoryMask) != globals->CurrentCategory)
				hasRecords = false;
		}
		globals->gListTableActive = true;
		globals->gReturnFocusToTable = true;
		FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, ListTable);	
		if(globals->CurrentRecord == noRecord && globals->gNavigation && hasRecords)
		{
			if(globals->FiveWayUpDown==FIVEWAYRECORD)
			{	
				globals->gScrollMode=FIVEWAYRECORD;	
			}
			globals->CurrentRecord=globals->TopVisibleRecord;
			PrvListSelectRecord(FrmGetActiveForm(), globals->CurrentRecord, true, true);
		}
	}
	return (category);
}

void PrvListNextCategory (void)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 category;
	TableType* tblP;
	FormType* frmP;

	category = CategoryGetNext (globals->AddrDB, globals->CurrentCategory);

	if (category != globals->CurrentCategory)
	{
		if (category == dmAllCategories)
			globals->ShowAllCategories = true;
		else
			globals->ShowAllCategories = false;

		SetCategoryLabel(category, true);
		
		frmP = FrmGetActiveForm();
		globals->ScrollPosition=0;
		PrvListLoadTable(frmP, true);
		tblP = CustomGetObjectPtrSmp(ListTable);
		globals->TopVisibleRecord=0;
		DmSeekRecordInCategory(globals->AddrDB, &globals->TopVisibleRecord, 0, dmSeekForward, category);
		
		TblEraseTable(tblP);
		TblDrawTable(tblP);

		// By changing the category the current record is lost.
		globals->CurrentRecord = noRecord;
		if(globals->gNavigation)
		{
			globals->gListTableActive = true;
			globals->gReturnFocusToTable = true;
			FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, ListTable);	
			if(globals->CurrentRecord == noRecord && globals->gNavigation)
			{
				if(globals->FiveWayUpDown==FIVEWAYRECORD)
				{
					globals->gScrollMode=FIVEWAYRECORD;
				}	
				globals->CurrentRecord=globals->TopVisibleRecord;
				PrvListSelectRecord(FrmGetActiveForm(), globals->CurrentRecord, true, true);
			}
		}	
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvListScroll
 *
 * DESCRIPTION: This routine scrolls the list of names and phone numbers
 *              in the direction specified.
 *
 * PARAMETERS:  direction	- up or dowm
 *              units		- unit amount to scroll
 *              byLine		- if true, list scrolls in line units
 *									- if false, list scrolls in page units
 *
 * RETURNED:    nothing
 ***********************************************************************/
Boolean PrvListScroll (WinDirectionType direction, UInt16 units, Boolean byLine)
{
	globalVars* globals = getGlobalsPtr();
	TableType* tblP;
	FormType* frmP;
	UInt16 rowsInPage;
	UInt16 newTopVisibleRecord;
	UInt16 prevTopVisibleRecord = globals->TopVisibleRecord;


	// Before processing the scroll, be sure that the command bar has been closed.
	MenuEraseStatus(0);

	frmP = FrmGetActiveForm();
	tblP = CustomGetObjectPtrSmp(ListTable);
	// Safe. There must be at least one row in the table.
	rowsInPage = PrvListNumberOfRows(tblP) - 1;
	newTopVisibleRecord = globals->TopVisibleRecord;

	// Scroll the table down.
	if (direction == winDown)
	{
		// Scroll down by line units
		if (byLine)
		{
			globals->ScrollPosition+=units;
			// Scroll down by the requested number of lines
			
			if (!ToolsSeekRecord (globals->adxtLibRef, &newTopVisibleRecord, units, dmSeekForward))
			{
				// Tried to scroll past bottom. Goto last record
				newTopVisibleRecord = dmMaxRecordIndex;
				ToolsSeekRecord (globals->adxtLibRef, &newTopVisibleRecord, 1, dmSeekBackward);
				globals->ScrollPosition--;
			}
		}
		// Scroll in page units
		else
		{
			globals->ScrollPosition+=units*rowsInPage;
			
			// Try scrolling down by the requested number of pages
			if (!ToolsSeekRecord (globals->adxtLibRef, &newTopVisibleRecord, units * rowsInPage, dmSeekForward))
			{
				globals->ScrollPosition+=units * rowsInPage;
				// Hit bottom. Try going backwards one page from the last record
				newTopVisibleRecord = dmMaxRecordIndex;
				if (!ToolsSeekRecord (globals->adxtLibRef, &newTopVisibleRecord, rowsInPage, dmSeekBackward))
				{
					// Not enough records to fill one page. Goto the first record
					newTopVisibleRecord = 0;
					ToolsSeekRecord (globals->adxtLibRef, &newTopVisibleRecord, 0, dmSeekForward);
					globals->ScrollPosition=0;
				}
			}
		}
	}
	// Scroll the table up
	else
	{
		if(!byLine)
			units *= rowsInPage;
		
		globals->ScrollPosition-=units;
		// Scroll up by the requested number of lines
		if (!ToolsSeekRecord (globals->adxtLibRef, &newTopVisibleRecord, units, dmSeekBackward))
		{
			// Tried to scroll past top. Goto first record
			newTopVisibleRecord = 0;
			ToolsSeekRecord (globals->adxtLibRef, &newTopVisibleRecord, 0, dmSeekForward);
			globals->ScrollPosition++;
		}
	}

	if(globals->gNavigation)
	{
		globals->gListTableActive = true;
	}
	// Avoid redraw if no change
	if (globals->TopVisibleRecord != newTopVisibleRecord)
	{
		globals->TopVisibleRecord = newTopVisibleRecord;
		//globals->RowEnter=globals->ListViewSelectThisRecord;
	
		
		PrvListLoadTable(frmP, true);
		globals->CurrentRecord = globals->TopVisibleRecord; 
		
		// Need to compare the previous top record to the current after PrvListLoadTable
		// as it will adjust TopVisibleRecord if drawing from newTopVisibleRecord will
		// not fill the whole screen with items.
		if (globals->TopVisibleRecord != prevTopVisibleRecord)
		{
			TblRedrawTable(tblP);
			PrvListSelectRecord(frmP, globals->CurrentRecord, true, true);
			
			return true;
		}
		else
		{
		}		
	}
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    PrvListSelectRecord
 *
 * DESCRIPTION: Selects (highlights) a record on the table, scrolling
 *              the record if neccessary.  Also sets the CurrentRecord.
 *
 * PARAMETERS:  frmP			IN	form
 *				recordNum 		IN 	record to select
 *				forceSelection	IN	force selection
 *
 * RETURNED:    nothing
 ***********************************************************************/
void PrvListSelectRecord( FormType* frmP, UInt16 recordNum, Boolean forceSelection, Boolean refresh )
{
	globalVars* globals = getGlobalsPtr();
	Int16 row, column;
	TableType* tblP;
	UInt16 attr;
	RectangleType bounds;
	IndexedColorType backColor;
	UInt16 cnt = 0;//to avoid lockup on PalmOS4
	
	if(recordNum >= DmNumRecords(globals->AddrDB))
	{
		return;
	}

	if(!refresh)
	{
		globals->CurrentRecord = recordNum;
		return;
	}
	
#ifdef DEBUG
	LogWrite("xt_log", "list", "PrvListSelectRecord");
#endif
	
	if (recordNum == noRecord)
		return;
	
	tblP = CustomGetObjectPtrSmp(ListTable);
	
	// Don't change anything if the same record is selected
	if ((TblGetSelection(tblP, &row, &column)) &&
		(recordNum == TblGetRowID (tblP, row)) &&
		(!forceSelection))
	{
		return;
	}
	if(globals->AddrListHighRes && recordNum==globals->SelectedRecord)
		return;
	//remove selection
	TblFindRowID(tblP, globals->CurrentRecord, &row);
	if((row>= 0) && (row<=TblGetNumberOfRows(tblP)) && refresh)
	{
		TblMarkRowInvalid(tblP, row);//
		TblRedrawTable(tblP);//
	}		
	// See if the record is displayed by one of the rows in the table
	// A while is used because if TblFindRowID fails we need to
	// call it again to find the row in the reloaded table.
	while (!TblFindRowID(tblP, recordNum, &row))
	{
		// Scroll the view down placing the item
		// on the top row
		cnt ++;
		if(cnt == 3)
			break;
		globals->TopVisibleRecord = recordNum;

		// Make sure that TopVisibleRecord is visible in globals->CurrentCategory
		if (globals->CurrentCategory != dmAllCategories)
		{
			// Get the category and the secret attribute of the current record.
			DmRecordInfo (globals->AddrDB, globals->TopVisibleRecord, &attr, NULL, NULL);
			if ((attr & dmRecAttrCategoryMask) != globals->CurrentCategory)
			{
				ErrNonFatalDisplay("Record not in globals->CurrentCategory");
				globals->CurrentCategory = (attr & dmRecAttrCategoryMask);
				SetCategoryLabel(globals->CurrentCategory, true);
			}
		}

		PrvListLoadTable(frmP,true);//
		if(refresh)
		{
			TblRedrawTable(tblP);//
		}
	}
	
	// Select the item
	if(refresh)//false
	{
		TblUnhighlightSelection(tblP);
		if(globals->gEachOtherSelected && !(row%2) && !TblRowMasked(tblP, row))
		{
			backColor=globals->gEachOther;
		}
		else
		{
			backColor=UIColorGetTableEntryIndex(UIFieldBackground);
		}
		WinPushDrawState();
		WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
		WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
		WinSetTextColor(UIColorGetTableEntryIndex(UIObjectForeground));
		TblGetItemBounds (tblP, row, column, &bounds);
		bounds.extent.y=TblGetRowHeight(tblP, 0);
		bounds.extent.x++;
		PrvListReplaceTwoColors(&bounds, 0,
						UIColorGetTableEntryIndex(UIObjectForeground), backColor, UIColorGetTableEntryIndex(UIObjectSelectedForeground), globals->gListTableActive?globals->gColorSel:globals->gInactiveSel);
		WinPopDrawState();
	}
	
	globals->CurrentRecord = recordNum;
	globals->SelectedLookupRecord = recordNum;
}


void PrvListUpdateDisplay( UInt16 updateCode )
{
	globalVars* globals = getGlobalsPtr();
	TableType* tblP;
	FormType* frmP;

#ifdef DEBUG
	LogWrite("xt_log", "list", "PrvListUpdateDisplay");
#endif

	// Do not use active form here since the update event is broadcasted
	frmP = FrmGetFormPtr(ListView);
	tblP = CustomGetObjectPtrSmp(ListTable);
			
	if (updateCode == frmRedrawUpdateCode)
	{
		globals->SelectedRecord=noRecord;
		TblUnhighlightSelection(tblP);	// Fixed bug #44352. If we don't do that, using a Debug rom, selection is too width when tapping Menu | Dial, then cancel into Dial Number screen (note is selected)
		PrvListInit( frmP );
		
		PrvListDrawFormAndSkin(frmP);
		PrvListDrawRecentButton();
		return;
	}

	if(updateCode & updatePrefs)
	{
		PrvListInit( frmP );
		PrvListDrawFormAndSkin(frmP);
		
		TblRedrawTable(tblP);
		if (globals->CurrentRecord != noRecord)
		{
			PrvListSelectRecord(frmP, globals->CurrentRecord, true, true);
		}
	}
	
	/*if (updateCode & updateRedrawAll ||	updateCode&updateColorsChanged)
	{
		PrvListLoadTable(frmP, true);
		if(updateCode&updateColorsChanged)
		{
			if (updateCode&updateColorsChanged)
				FrmEraseForm(frmP);
		}
		PrvListDrawFormAndSkin(frmP);
		PrvListDrawRecentButton();
	
		TblRedrawTable(tblP);	
	}*/
}

Boolean PrvListHandleRecordSelection( EventType* event )
{
	globalVars* globals = getGlobalsPtr();
	TablePtr table;
	Int16 row, rowPrev, column, phoneX;
	Boolean isSelected, isPenDown;
	UInt16 recordNum;
	Err error;
#ifdef CONTACTS
	P1ContactsDBRecordType record;
#else
	AddrDBRecordType record;
#endif
	MemHandle recordH;
	FontID currFont;
	Char * phone;
	RectangleType bounds, numberBounds;
	Coord x, y;
	UInt16 phoneIndex;
	Boolean eachOther;

	IndexedColorType backColor;
				
	table = event->data.tblSelect.pTable;
	row = event->data.tblSelect.row;
	column = event->data.tblSelect.column;

	// If the column being tapped on isn't the name and number column,
	// let the table handle selection to view the note.
	
	// Extract the x coordinate of the start of the phone number for this row.
	// This was computed and stored in the row data when the row was drawn.
	phoneX = TblGetRowData(table, row);
	
	eachOther = globals->gEachOtherSelected && !(row%2) && !TblRowMasked(table, row) && column!=noteColumn;
	if(eachOther)
	{
		backColor=globals->gEachOther;
	}
	else
	{
		backColor=UIColorGetTableEntryIndex(UIFieldBackground);
	}
			
	TblFindRowID(table, globals->CurrentRecord, &rowPrev);
	if((rowPrev>= 0) && (rowPrev<=TblGetNumberOfRows(table)))
	{
		TblMarkRowInvalid(table, rowPrev);
		TblRedrawTable(table);
	}
	// Disable the current selection
	globals->CurrentRecord = noRecord;
			
	if(((!globals->EnableTapDialing ||  globals->startupType == startupLookup) || event->screenX < phoneX || column==noteColumn))
	{
		/*TblFindRowID(table, globals->CurrentRecord, &rowPrev);
		if((rowPrev>= 0) && (rowPrev<=TblGetNumberOfRows(table)))
		{
			TblMarkRowInvalid(table, rowPrev);
			TblRedrawTable(table);
		}	*/
		// Disable the current selection
		globals->CurrentRecord = noRecord;

		isSelected=false;
		WinPushDrawState();
		if(eachOther)
		{
			WinSetBackColor(backColor);
		}
		else
		{
			WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
		}
		WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
		WinSetTextColor(UIColorGetTableEntryIndex(UIObjectForeground));
		TblGetItemBounds (table, row, column, &bounds);
		bounds.extent.y=TblGetRowHeight(table, 0);
		bounds.extent.x++;
		// Draw the phone number selected.
		
		
		PrvListReplaceTwoColors(&bounds, 0,
							UIColorGetTableEntryIndex(UIObjectForeground), backColor, UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill));
			
		isSelected = true;
		do 
		{
			PenGetPoint (&x, &y, &isPenDown);
			if (RctPtInRectangle (x, y, &bounds))
			{
				if (! isSelected)
				{
					isSelected = true;
					PrvListReplaceTwoColors(&bounds, 0,
									UIColorGetTableEntryIndex(UIObjectForeground), backColor, UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill));
				}
			}
			
			else if (isSelected)
			{
				isSelected = false;
				PrvListReplaceTwoColors(&bounds, 0,
									UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill), UIColorGetTableEntryIndex(UIObjectForeground), backColor);
			}
		}
		while (isPenDown);

		if (isSelected)
		{
			PrvListReplaceTwoColors(&bounds, 0,
									UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill), UIColorGetTableEntryIndex(UIObjectForeground), backColor);
		}
		
		WinPopDrawState();
				
		if (RctPtInRectangle (x, y, &bounds))
		{
			if (TblRowMasked(table, row))
			{
				if (SecVerifyPW (showPrivateRecords) == true)
				{
					// We only want to unmask this one record, so restore the preference.
					PrefSetPreference (prefShowPrivateRecords, maskPrivateRecords);
					column = nameAndNumColumn; //force non-note view
				}
				else
					return false;
			}
				
			// An item in the list of names and phone numbers was selected, go to
			// the record view.
			globals->CurrentRecord = TblGetRowID (event->data.tblSelect.pTable,
								 event->data.tblSelect.row);
			// Set the global variable that determines which field is the top visible
			// field in the edit view.  Also done when New is pressed.
			globals->TopVisibleFieldIndex = 0;
			globals->EditRowIDWhichHadFocus = editFirstFieldIndex;
			globals->EditFieldPosition = 0;

			if(globals->startupType == startupLookup)
			{
				if(column != nameAndNumColumn)
					return true;
				globals->SelectedLookupRecord = globals->CurrentRecord;
				
				//highlight selected record
				TblUnhighlightSelection(table);
				if(globals->gEachOtherSelected && !( event->data.tblSelect.row%2) && !TblRowMasked(table,  event->data.tblSelect.row))
				{
					backColor=globals->gEachOther;
				}
				else
				{
					backColor=UIColorGetTableEntryIndex(UIFieldBackground);
				}
				WinPushDrawState();
				WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
				WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
				WinSetTextColor(UIColorGetTableEntryIndex(UIObjectForeground));
				TblGetItemBounds (table,  event->data.tblSelect.row,  event->data.tblSelect.column, &bounds);
				bounds.extent.y=TblGetRowHeight(table, 0);
				bounds.extent.x++;
				PrvListReplaceTwoColors(&bounds, 0,
								UIColorGetTableEntryIndex(UIObjectForeground), backColor, UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill));
				WinPopDrawState();
				
				return true;
			}
				
			if (column == nameAndNumColumn)
			{	
				UInt16 linkCount;
				UInt32 id;
				UInt16 linksWidth = LINKSICON_WIDTH;
				if(globals->AddrListHighRes)
					linksWidth >>= 1;
				
				DmRecordInfo(globals->AddrDB, globals->CurrentRecord, NULL, &id, NULL);
				linkCount = GetLinkCount(globals->adxtLibRef, globals->linksDB, id);
				
				if(linkCount == 0 || x - bounds.topLeft.x > linksWidth - 1)
				{
					FrmGotoForm (RecordView);
					return true;
				}
			
				if (linkCount > 0)
				{
					//popup links list
					Int16 sel;
					ListPtr linksList = GetObjectPtrSmp(ListLinksList);
					LstSetListChoices(linksList, NULL, linkCount+1);
					if(linkCount < 6)
						LstSetHeight(linksList, linkCount+1);
					else
						LstSetHeight(linksList, 6);
					LstSetDrawFunction(linksList, LinksDrawList);
					FrmSetObjectPosition(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ListLinksList), 0, y);
					sel = LstPopupList(linksList);
					if(sel == linkCount)
					{
						FrmPopupForm(LinksDialog);
						return true;
					}
					else if(sel != -1)
					{
						if(GoToLink(globals->adxtLibRef, globals->linksDB, id, sel+1) == ERROR_GOTOLINK_PRIVATE)
							FrmAlert(alertGoToPrivateLink);
					}
				}
			}
			else if (NoteViewCreate())
					FrmGotoForm (NewNoteView);	
		}		
	}
	
	
	if(!globals->EnableTapDialing ||  globals->startupType == startupLookup)
		return false;
	if (column != nameAndNumColumn)
		return false;

	// If the record is masked, dialing is impossible, so just let
	//	the table handle selection.
	if (TblRowMasked(table, row))
	{
		isSelected=false;
		WinPushDrawState();
		if(eachOther)
		{
			WinSetBackColor(backColor);
		}
		else
		{
			WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
		}
		WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
		WinSetTextColor(UIColorGetTableEntryIndex(UIObjectForeground));
		TblGetItemBounds (table, row, column, &bounds);
		bounds.extent.y=TblGetRowHeight(table, 0);
		bounds.extent.x++;
		// Draw the phone number selected.
		
		
		PrvListReplaceTwoColors(&bounds, 0,
							UIColorGetTableEntryIndex(UIObjectForeground), backColor, UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill));
			
		isSelected = true;
		do 
		{
			PenGetPoint (&x, &y, &isPenDown);
			if (RctPtInRectangle (x, y, &bounds))
			{
				if (! isSelected)
				{
					isSelected = true;
					PrvListReplaceTwoColors(&bounds, 0,
									UIColorGetTableEntryIndex(UIObjectForeground), backColor, UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill));
				}
			}
			
			else if (isSelected)
			{
				isSelected = false;
				PrvListReplaceTwoColors(&bounds, 0,
									UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill), UIColorGetTableEntryIndex(UIObjectForeground), backColor);
			}
		}
		while (isPenDown);
		if (isSelected)
		{
			PrvListReplaceTwoColors(&bounds, 0,
									UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill), UIColorGetTableEntryIndex(UIObjectForeground), backColor);
		}
		
		WinPopDrawState();
				
		if (RctPtInRectangle (x, y, &bounds))
		{
			if (TblRowMasked(table, row))
			{
				if (SecVerifyPW (showPrivateRecords) == true)
				{
					// We only want to unmask this one record, so restore the preference.
					PrefSetPreference (prefShowPrivateRecords, maskPrivateRecords);
					column = nameAndNumColumn; //force non-note view
					globals->CurrentRecord = TblGetRowID (event->data.tblSelect.pTable,
								 event->data.tblSelect.row);
					FrmGotoForm (RecordView);
					return true;
				}
				else
					return false;
			}
		}
		return false;
	}
	// Extract the x coordinate of the start of the phone number for this row.
	// This was computed and stored in the row data when the row was drawn.
	phoneX = TblGetRowData(table, row);

	// If the user tapped on the name rather than the number, or on the space between them,
	// let the table handle selection so the user gets to view the record.
	if (event->screenX < phoneX)
		return false;

	// Get the record number that corresponds to the table item to draw.
	// The record number is stored in the "intValue" field of the item.
	recordNum = TblGetRowID (table, row);
	error = univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, recordNum, &record, &recordH);
	if (error)
	{
		ErrNonFatalDisplay ("Record not found");
		return false;
	}
	phoneIndex = record.options.phones.displayPhoneForList;
	phone = record.fields[univPhone1 + phoneIndex];

	// If there is no phone number, if the phone is not part of supported
	// dial phone number, let the table handle selection.
	if ((phone == chrNull) || (!ToolsIsPhoneIndexSupported(&record, phoneIndex)) )
		goto CleanUpAndLeaveUnhandled;
	

	// The user tapped on the phone number, so instead of letting the
	// table handle selection, we highlight just the phone number.

	// First, deselect the row if it is selected.
	globals->CurrentRecord = noRecord;
	TblUnhighlightSelection(table);

	// Set up the drawing state the way we want it.
	WinPushDrawState();
	WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetTableEntryIndex(UIObjectForeground));
	currFont = FntSetFont (globals->AddrListFont);

	TblGetItemBounds (table, row, column, &bounds);
	numberBounds = bounds;
	numberBounds.extent.x -= phoneX - numberBounds.topLeft.x;		// Maintain same right side.
	numberBounds.extent.y=TblGetRowHeight(table, 0);
	numberBounds.topLeft.x = phoneX;

	// Extend left side of selection rectangle one pixel since we have the room.
	// It'd be great if we could extend the right side as well, so that the 'W'
	// for Work numbers wouldn't touch the edge of the selection rectangle, but
	// then it'd hit up against the note icon.
	numberBounds.topLeft.x--;
	numberBounds.extent.x ++;
	numberBounds.extent.x ++;
	
	// Draw the phone number selected.
	PrvListReplaceTwoColors(&numberBounds, 0,
							UIColorGetTableEntryIndex(UIObjectForeground), backColor, UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill));

	isSelected = true;
	do {
		PenGetPoint (&x, &y, &isPenDown);
		if (RctPtInRectangle (x, y, &numberBounds))
		{
			if (! isSelected)
			{
				isSelected = true;
				PrvListReplaceTwoColors(&numberBounds, 0,
										UIColorGetTableEntryIndex(UIObjectForeground), backColor, UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill));
			}
		}
		else if (isSelected)
		{
			isSelected = false;
			PrvListReplaceTwoColors(&numberBounds, 0,
									UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill), UIColorGetTableEntryIndex(UIObjectForeground), backColor);
		}
	} while (isPenDown);

	if (isSelected)
		PrvListReplaceTwoColors(&numberBounds, 0,
								UIColorGetTableEntryIndex(UIObjectSelectedForeground), UIColorGetTableEntryIndex(UIObjectSelectedFill), UIColorGetTableEntryIndex(UIObjectForeground), backColor);

	// Restore the previous drawing state.
	FntSetFont (currFont);
	WinPopDrawState();

	// Release the record.
	MemHandleUnlock(recordH);

	if (isSelected)
	{
		// Make it the current item
		globals->CurrentRecord = recordNum;

		// Dial the number.
		return DialListShowDialog(recordNum, phoneIndex, 0);
	}

	return true;		// Don't let the table do any selection

CleanUpAndLeaveUnhandled:
	// Release the record.
	MemHandleUnlock(recordH);

	return false;
}

static Boolean PrvListDialCurrentRecord(UInt16 recordNum)
{
	globalVars* globals = getGlobalsPtr();
	Err error;
	Char * phone;
	UInt16 phoneIndex;
	univAddrDBRecordType record;
	MemHandle recordH;
	
	error = univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, recordNum, &record, &recordH);
	if (error)
	{
		ErrNonFatalDisplay ("Record not found");
		return false;
	}
	phoneIndex = record.options.phones.displayPhoneForList;
	phone = record.fields[univPhone1 + phoneIndex];

	// If there is no phone number, if the phone is not part of supported
	// dial phone number, let the table handle selection.
	if ((phone == chrNull) || (!ToolsIsPhoneIndexSupported(&record, phoneIndex)) )
	{
		UInt16 i;
		Boolean found = false;
		for (i = 0; i < univNumPhoneLabels; i++)
		{
			phone = record.fields[univPhone1 + i];
			if (!((phone == chrNull) || (!ToolsIsPhoneIndexSupported(&record, i)) ))
			{
				found = true;
				break;
			}
		}
		if(!found)
			goto CleanUpAndLeaveUnhandled;
	}	
	
	// Release the record.
	MemHandleUnlock(recordH);

	// Make it the current item
	globals->CurrentRecord = recordNum;

	// Dial the number.
	DialListShowDialog(recordNum, phoneIndex, 0);
	
	return true;		// Don't let the table do any selection

CleanUpAndLeaveUnhandled:
	// Release the record.
	MemHandleUnlock(recordH);

	return false;


}
/***********************************************************************
 *
 * FUNCTION:    PrvListReplaceTwoColors
 *
 * DESCRIPTION: This routine does a selection or deselection effect by
 *					 replacing foreground and background colors with a new pair
 *					 of colors. In order to reverse the process, you must pass
 *					 the colors in the opposite order, so that the current
 *					 and new colors are known to this routine. This routine
 *					 correctly handling the cases when two or more of these
 *					 four colors are the same, but it requires that the
 *					 affected area of the screen contains neither of the
 *					 given NEW colors, unless these colors are the same as
 *					 one of the old colors.
 *
 * PARAMETERS:	 rP  - pointer to a rectangle to 'invert'
 *					 cornerDiam	- corner diameter
 *					 oldForeground	- UI color currently used for foreground
 *					 oldBackground	- UI color currently used for background
 *					 newForeground	- UI color that you want for foreground
 *					 newBackground	- UI color that you want for background
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	05/19/00	Initial Revision
 *
 ***********************************************************************/
void PrvListReplaceTwoColors (const RectangleType *rP, UInt16 cornerDiam, IndexedColorType oldForeground, IndexedColorType oldBackground, IndexedColorType newForeground, IndexedColorType newBackground)
{
	WinPushDrawState();
	WinSetDrawMode(winSwap);
	WinSetPatternType (blackPattern);

	if (newBackground == oldForeground)
	{
		if (newForeground == oldBackground)
		{
			// Handle the case when foreground and background colors change places,
			// such as on black and white systems, with a single swap.
			WinSetBackColor(oldBackground);
			WinSetForeColor(oldForeground);
			WinPaintRectangle(rP, cornerDiam);
		}
		else
		{
			// Handle the case when the old foreground and the new background
			// are the same, using two swaps.
			WinSetBackColor(oldForeground);
			WinSetForeColor(oldBackground);
			WinPaintRectangle(rP, cornerDiam);
			WinSetBackColor(oldBackground);
			WinSetForeColor(newForeground);
			WinPaintRectangle(rP, cornerDiam);
		}
	}
	else if (oldBackground==newForeground)
	{
		// Handle the case when the old background and the new foreground
		// are the same, using two swaps.
		WinSetBackColor(newForeground);
		WinSetForeColor(oldForeground);
		WinPaintRectangle(rP, cornerDiam);
		WinSetBackColor(newBackground);
		WinSetForeColor(oldForeground);
		WinPaintRectangle(rP, cornerDiam);
	}
	else
	{
		// Handle the case when no two colors are the same, as is typically the case
		// on color systems, using two swaps.
		WinSetBackColor(oldBackground);
		WinSetForeColor(newBackground);
		WinPaintRectangle(rP, cornerDiam);
		WinSetBackColor(oldForeground);
		WinSetForeColor(newForeground);
		WinPaintRectangle(rP, cornerDiam);
	}

	WinPopDrawState();
}

// show/hide 'recent contacts button'
static void PrvListDrawRecentButton()
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gEnabledRecent && globals->startupType == startupNormal)
	{
		CustomShowObjectSmp(ListRecentButton);
	}
	else
	{
		CustomHideObjectSmp(ListRecentButton);
	}
}


// draw function for battery gadget (nonTreo)
static Boolean PrvDrawBatteryCallback(struct FormGadgetTypeInCallback *gadgetP, UInt16 cmd, void *paramP)
{
#pragma unused(paramP)

	Boolean handled = false;
	IndexedColorType old_color, color;
	RectangleType rect;
	RGBColorType rgb;
	UInt16 voltage, diff_y;
	UInt8 btr_percent;
	
	switch (cmd) {
		case formGadgetDrawCmd:
#ifdef DEBUG
			LogWrite("xt_log", "battery", "GadgetDraw");
#endif

			
			rect = gadgetP->rect;
			rect.topLeft.y += 2;
			rect.extent.y -= 2;
			rect.extent.x = 5;
			
			voltage = SysBatteryInfo(false, NULL, NULL, NULL, NULL, NULL, &btr_percent);
			
			diff_y = rect.extent.y * (100 - btr_percent) / 100;
			rect.topLeft.y += diff_y;
			rect.extent.y -= diff_y;
			WinDrawRectangle(&rect, 0);
			
			rgb.index = 0;
			rgb.r = 0;
			rgb.b = 0;
			rgb.g = 0;
			color = WinRGBToIndex(&rgb);
			old_color = WinSetForeColor(color);
			
			rect.topLeft.y = gadgetP->rect.topLeft.y;
			rect.extent.y = gadgetP->rect.extent.y;
			rect.topLeft.y += 2;
			rect.extent.y -= 2;
			WinDrawRectangleFrame(simpleFrame, &rect);
			rect.topLeft.y = gadgetP->rect.topLeft.y;
			rect.extent.y = 1;
			rect.topLeft.x =  gadgetP->rect.topLeft.x + 1;
			rect.extent.x =  gadgetP->rect.extent.x - 7;
			WinDrawRectangleFrame(simpleFrame, &rect);
			
			WinSetForeColor(old_color);
			handled = true;
			break;
	}

	return handled;
}

Boolean PrvListDoCommand (UInt16 command)
{
	UInt16 	newRecord;
	UInt16 	numCharsToHilite;
	Boolean	wasHiding;
	UInt16 	mode;
	DmOpenRef addrXTDB;
	globalVars* globals = getGlobalsPtr();
	globals->gMenuActive = false;
	switch (command)
	{
	case ListRecordPurgeCategory:
		PrvListPurgeCategory(globals->CurrentCategory);
		return true;	
	case ListRecordBeamBusinessCardCmd:
		MenuEraseStatus (0);
		ToolsAddrBeamBusinessCard(globals->AddrDB);
		dia_restore_state();
		return true;
	case ListRecordManageXTCmd:
		{
			DmSearchStateType srch; 
			UInt16 cardNo;
			UInt16 launchFlags=0;
			LocalID dbID;
			UInt32		*gotoInfoP;
			gotoInfoP = (UInt32*)MemPtrNew (sizeof(UInt32));
			MemPtrSetOwner(gotoInfoP, 0);			
			
			if(DmGetNextDatabaseByTypeCreator(true,&srch, 'appl', 'mnXT', true, &cardNo, &dbID)!=errNone)
				return true;
			SysUIAppSwitch(0, dbID, sysAppLaunchCmdNormalLaunch, gotoInfoP);
		}
		return true;
	case ListEditGraffitiLegacyHelpCmd:
	case ListEditGraffitiHelpCmd:
		SysGraffitiReferenceDialog(referenceDefault);
		dia_restore_state();
		return true;
	
	case ListRecordBeamCategoryCmd:
		MenuEraseStatus (0);
		TransferSendCategory(globals->AddrDB, globals->CurrentCategory, exgBeamPrefix, NoDataToBeamAlert);
		dia_restore_state();
		return true;

	case ListOptionsColor:
		globals->gColorFormID=ListView;
		FrmPopupForm(ColorOptionsDialog);
		return true;

	case ListRecordSendCategoryCmd:
		MenuEraseStatus (0);
		TransferSendCategory(globals->AddrDB, globals->CurrentCategory, exgSendPrefix, NoDataToSendAlert);
		dia_restore_state();
		return true;

	case ListRecordDuplicateAddressCmd:
		if (globals->CurrentRecord == noRecord)
		{
			return true;
		}
		globals->gDuplicateCaller = ListView;
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

	case ListRecordDialCmd:
		{
			UInt16 phoneIndex;
			Err error;
			univAddrDBRecordType record;
			MemHandle recordH;

			if (globals->CurrentRecord == noRecord)
			{
				SndPlaySystemSound (sndError);
				return true;
			}

			error = univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &record, &recordH);
			if (error)
			{
				ErrNonFatalDisplay ("Record not found");
				return false;
			}
			phoneIndex = record.options.phones.displayPhoneForList;

			// Release the record.
			MemHandleUnlock(recordH);
			
			DialListShowDialog(globals->CurrentRecord, phoneIndex, 0);
			return true;
		}

	case ListRecordCompanyData:
		FrmGotoForm(CompanyData);
		return true;
		break;
	case ListOptionsConnect:
		FrmPopupForm(ConnectOptionsDialog);
		return true;
		break;
	case ListOptionsSearch:
		FrmPopupForm(AddressListOptionsDialog);
		return true;
		break;
	case ListOptionsButton:
		FrmPopupForm(ButtonOptionsDialog);
		return true;
		break;
	case ListRecordBirthdays:
		FrmPopupForm(BirthdaysDialog);
		return true;
		break;	
	case ListRecordDeleteRecordCmd:
		if (globals->CurrentRecord != noRecord)
		{
			if (PrvListDeleteRecord ())
			{
				globals->CurrentRecord=noRecord;
				PrvListClearLookupString ();
				PrvListUpdateDisplay (updateRedrawAll | updateSelectCurrentRecord);
				PrvListLoadTable(FrmGetActiveForm(), true);
				TblRedrawTable(CustomGetObjectPtrSmp(ListTable));
				FrmGlueNavObjectTakeFocus(globals->adxtLibRef, FrmGetActiveForm(), ListTable);	
			}
		}
		else
			SndPlaySystemSound (sndError);
		return true;	
	case ListRecordDeleteRecordMenuCmd:
		if (globals->CurrentRecord != noRecord)
		{
			if (PrvListDeleteRecord ())
			{
				globals->CurrentRecord=noRecord;
				PrvListClearLookupString ();
				PrvListUpdateDisplay (updateRedrawAll | updateSelectCurrentRecord);
				globals->CurrentRecord = globals->TopVisibleRecord;
				PrvListLoadTable(FrmGetActiveForm(), true);
				TblRedrawTable(CustomGetObjectPtrSmp(ListTable));
				FrmGlueNavObjectTakeFocus(globals->adxtLibRef, FrmGetActiveForm(), ListTable);	
			}
		}
		else
			SndPlaySystemSound (sndError);
		return true;
	
	case ListRecordBeamRecordCmd:
		if (globals->CurrentRecord != noRecord)
		{
			MenuEraseStatus (0);
			TransferSendRecord(globals->AddrDB, globals->CurrentRecord, exgBeamPrefix, NoDataToBeamAlert);
			dia_restore_state();
		}
		else
			SndPlaySystemSound (sndError);
		return true;

	case ListOptionsFontCmd:
		{
			FontID newFont;	
			MenuEraseStatus(0);
			RestoreDialer();
			newFont = ToolsSelectFontWithHires(globals->AddrListFont, true);			
			SetDialer();
			// The update event for font changed is post so when the
			// item is highlighted in the updateEvent handler, the drawing was made with a bad font
			// So force unhighlight here
			if (newFont != globals->AddrListFont)
			{
				TblUnhighlightSelection(CustomGetObjectPtrSmp(ListTable));
				// now set the new font
				globals->AddrListFont = newFont;
				PrvListInit(FrmGetActiveForm());
				PrvListDrawFormAndSkin(FrmGetActiveForm());
			}
			return true;
		}

	case ListOptionsListByCmd:
		MenuEraseStatus (0);
		PrvListClearLookupString();
		FrmPopupForm (PreferencesDialog);
		return true;

	case ListOptionsEditCustomFldsCmd:
		MenuEraseStatus (0);
		FrmPopupForm (CustomEditDialog);
		return true;

	case ListOptionsSecurityCmd:
		
		wasHiding = (globals->PrivateRecordVisualStatus == hidePrivateRecords);

		RestoreDialer();
		globals->PrivateRecordVisualStatus = SecSelectViewStatus();
		dia_restore_state();
		SetDialer();
		if (wasHiding != (globals->PrivateRecordVisualStatus == hidePrivateRecords))
		{

			// Close the application's data file.
			DmCloseDatabase (globals->AddrDB);

			mode = (globals->PrivateRecordVisualStatus == hidePrivateRecords) ?
				dmModeReadWrite : (dmModeReadWrite | dmModeShowSecret);

#ifdef CONTACTS
			globals->AddrDB = DmOpenDatabase(0, DmFindDatabase(0, CONTACTS_DBNAME), mode);  
#else
			AddrDBGetDatabase(globals->adxtLibRef, &(globals->AddrDB), mode);
#endif
			ErrFatalDisplayIf(!globals->AddrDB,"Can't reopen DB");
		}

		//For safety, simply reset the currentRecord
		TblReleaseFocus (CustomGetObjectPtrSmp (ListTable));
		PrvListUpdateDisplay (updateRedrawAll | updateSelectCurrentRecord);
		//updateSelectglobals->CurrentRecord will cause currentRecord to be reset to noRecord if hidden or masked
		return true; // Hou: bug #44076 correction: used to be "break;" -> caused the event to not be handled

	case ListOptionsAboutCmd:
		FrmPopupForm(AboutForm);
		return true;
		
	case ListHelpTips:
		RestoreDialer();
		FrmHelp(AddressListHelp);
		dia_restore_state();
		SetDialer();
		return true;
	default:
		break;
	}
	return false;
}

Boolean ListOnMenuOpenEvent()
{
	globalVars* globals = getGlobalsPtr();
	//UInt32 encoding;
	UInt32 romVersion, g2DynamicID;
	Err err;
	UInt32 numLibs;
	/*if (FtrGet(sysFtrCreator, sysFtrNumEncoding, 
	 &encoding) != errNone)
	 encoding = charEncodingPalmLatin;
	if (encoding == charEncodingPalmSJIS) 
	{
		MenuHideItem(ListOptionsSearch);
	}*/
		
	if(globals->gDepth<4)
	{
		MenuHideItem(ListOptionsColor);
	}
	
	//check if Manage XT is installed
	if(!ManageXTPresent())
	{
		MenuHideItem(ListRecordManageXTCmd);
	}
	
	if(!ToolsIsDialerPresent())
	{
		MenuHideItem(ListRecordDialCmd);
	}
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
	
	err = FtrGet(sysFtrCreator,	sysFtrNumROMVersion, &romVersion);

	if(romVersion>=0x04003000)
	{
		if (ExgGetRegisteredApplications(NULL, &numLibs, NULL, NULL, exgRegSchemeID, exgSendScheme) || !numLibs)
			MenuHideItem(ListRecordSendCategoryCmd);
		else
			MenuShowItem(ListRecordSendCategoryCmd);
	}
	else
	{
		MenuHideItem(ListRecordSendCategoryCmd);
	}			
	return false;
}

Boolean ListOnMenuEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gDeviceFlags.bits.treo)
	{
		switch(event->data.menu.itemID)
		{
			case 1800:
				event->data.menu.itemID = 10000;
				EvtAddEventToQueue(event); 
				break;
			case 1801:
				event->data.menu.itemID = 10001;
				EvtAddEventToQueue(event); 
				break;
			case 1802:
				event->data.menu.itemID = 10002;
				EvtAddEventToQueue(event); 
				break;
			case 1803:
				event->data.menu.itemID = 10003;
				EvtAddEventToQueue(event); 
				break;
			case 1804:
				event->data.menu.itemID = 10004;
				EvtAddEventToQueue(event); 
				break;
			case 1806:
				event->data.menu.itemID = 10006;
				EvtAddEventToQueue(event); 
				break;
			case 1807:
				event->data.menu.itemID = 10007;
				EvtAddEventToQueue(event); 
				break;	
		}
	}
	return PrvListDoCommand (event->data.menu.itemID);
}

Boolean PrvListPurgeCategory(UInt16 categoryNum)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 index=0;
	UInt16 ctlIndex;
	UInt16 buttonHit;
	FormPtr alert;
	Boolean archive;
	TablePtr tblP;
	RestoreDialer();			
	// Display an alert to comfirm the operation.
	alert = FrmInitForm (PurgeCatDialog);

	// Set the "save backup" checkbox to its previous setting.
	ctlIndex = FrmGetObjectIndex (alert, PurgeCatSaveBackup);
	FrmSetControlValue (alert, ctlIndex, globals->SaveCatBackup);
	
	buttonHit = FrmDoDialog (alert);
	dia_restore_state();

	archive = FrmGetControlValue (alert, ctlIndex);

	FrmDeleteForm (alert);
	if (buttonHit == PurgeCatCancel)
		return (false);
	if(FrmAlert(PurgeAlert)==1)
	{
		return false;
	}
	SetDialer();
	// Remember the "save backup" checkbox setting.
	globals->SaveCatBackup = archive;
	
	tblP = CustomGetObjectPtrSmp(ListTable);
	
	TblUnhighlightSelection(tblP);

	while (true)
	{
		UInt16 attr;
		if (DmSeekRecordInCategory(globals->AddrDB, &index, 0, dmSeekForward, categoryNum) != 0)
			break;
		DmRecordInfo (globals->AddrDB, index, &attr, NULL, NULL);
		if(((attr & dmRecAttrSecret) && globals->PrivateRecordVisualStatus == maskPrivateRecords))
		{
			index++;
			continue;
		}
		
		deleteRecord(globals->adxtLibRef, globals->AddrDB, index, archive);
		DmMoveRecord (globals->AddrDB, index, DmNumRecords (globals->AddrDB));
	}
	PrvListLoadTable(FrmGetActiveForm(), true);
	PrvListUpdateDisplay (updateRedrawAll | updateSelectCurrentRecord);
	SetCategoryLabel(globals->CurrentCategory, true);
	CleanRecentDB(globals->adxtLibRef);
	DeleteOrphaned(globals->adxtLibRef, globals->linksDB);
	return (true);
}

/***********************************************************************
 *
 * FUNCTION:    PrvListDeleteRecord
 *
 * DESCRIPTION: This routine deletes an address record. This routine is
 *              called when the delete button in the command bar is
 *              pressed when address book is in list view.  The benefit
 *					 this routine proides over DetailsDeleteRecord is that the
 *					 selection is maintained right up to the point where the address
 *					 is being deleted.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the record was delete or archived.
 ***********************************************************************/
Boolean PrvListDeleteRecord (void)
{
	UInt16	ctlIndex;
	UInt16	buttonHit;
	FormType*	alert;
	Boolean	archive;
	TablePtr	table;
	globalVars* globals = getGlobalsPtr();

	// Display an alert to comfirm the operation.
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

	// Remember the "save backup" checkbox setting.
	globals->SaveBackup = archive;

	// Clear the highlight on the selection before deleting the item.
	table = CustomGetObjectPtrSmp (ListTable);
	TblUnhighlightSelection(table);

	ToolsDeleteRecord(archive);

	return (true);
}

Boolean ManageXTPresent()
{
	DmSearchStateType srch; 
	UInt16 cardNo;
	LocalID dbID;
	if(DmGetNextDatabaseByTypeCreator(true,&srch, 'appl', 'mnXT', true, &cardNo, &dbID)!=errNone)
		return false;
	return true;
}

Boolean ListOnMenuCmdBarOpenEvent(EventType* event)
{
	MemHandle cmdTextH;
	globalVars* globals = getGlobalsPtr();
	if (globals->CurrentRecord != noRecord)
	{
		// because this isn't a real menu command, get the text for the button from a resource
		cmdTextH = DmGetResource (strRsc, DeleteRecordStr);
		MenuCmdBarAddButton(menuCmdBarOnLeft, BarDeleteBitmap, menuCmdBarResultMenuItem, ListRecordDeleteRecordCmd, MemHandleLock(cmdTextH));
		MemHandleUnlock(cmdTextH);
		DmReleaseResource(cmdTextH);
	}

	MenuCmdBarAddButton(menuCmdBarOnLeft, BarSecureBitmap, menuCmdBarResultMenuItem, ListOptionsSecurityCmd, 0);

	if (globals->CurrentRecord != noRecord)
	{
		// because this isn't a real menu command, get the text for the button from a resource
		cmdTextH = DmGetResource (strRsc, BeamRecordStr);
		MenuCmdBarAddButton(menuCmdBarOnLeft, BarBeamBitmap, menuCmdBarResultMenuItem, ListRecordBeamRecordCmd, MemHandleLock(cmdTextH));
		MemHandleUnlock(cmdTextH);
		DmReleaseResource(cmdTextH);
	}

	// tell the field package to not add cut/copy/paste buttons automatically; we
	// don't want it for the lookup field since it'd cause confusion.
	event->data.menuCmdBarOpen.preventFieldButtons = true;

	// don't set handled to true; this event must fall through to the system.
	return false;
}
			
