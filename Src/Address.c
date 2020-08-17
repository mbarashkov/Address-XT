#include "Address.h"

#include "AddrDialList.h"
#include "CompanyData.h"
#include "AddrList.h"
#include "AddrView.h"
#include "AddrEdit.h"
#include "AddrNote.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "AddrDetails.h"
#include "AddrCustom.h"
#include "AddressLookup.h"
#include "AddrDialList.h"
#include "AddressTransfer.h"
#include "AddrPrefs.h"
#include "AboutForm.h"
#include "AboutForm.h"
#include "AddressAutoFill.h"
#include "AddressRsc.h"
#include "Plugins/DatebookRsc.h"
#include "SNForm.h"
#include "globals.h"
#include "Links.h"
#include "AddressListOptions.h"
#include "ButtonOptions.h"
#include "ColorOptions.h"
#include "RecentOptions.h"
#include "Birthdays.h"
#include "AddressDialog.h"
#include "LinksDialog.h"
#include "NewLink.h"
#include "ConnectOptions.h"
#include "CompanyDataOptions.h"
#include "Plugins/MemoMain.h"
#include "Plugins/ToDo.h"
#include "Plugins/ContactSelList.h"
#include "Plugins/DateDay.h"
#include "RecentDB.h"
#include "AddrXTDB.h"
//#include "Mapopolis.h"
#include "syslog.h"
#include "favoritesdb/favorites.h"
#include "AddrDefines.h"
#include "pnoJpg/pnoJpeg.h"

#include "Plugins/Datebook.h"

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/

static UInt32	PrvAppPilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags);
static void		PrvAppSearch(FindParamsPtr findParams);
static void		PrvAppGoToItem (GoToParamsPtr goToParams, Boolean launchingApp);
static void		PrvAppGoToItemSmp (UInt16 index, Boolean launchingApp);
static void		PrvAppHandleSync(void);
static Boolean	PrvAppLaunchCmdDatabaseInit(DmOpenRef dbP);
//Boolean 		AttentionProc(AttnLaunchCodeArgsType * paramP, UInt16 launchFlags);
Boolean			SublaunchAllocGlobals();
void 			SublaunchDeallocGlobals();
void 			PrvAppStopEx(Boolean sublaunch);
Err				PrvAppStartEx(Boolean sublaunch);

Boolean LoadAddressXTLibraries(UInt16* libRef1, UInt16* libRef2)
{
#ifndef LIBDEBUG
	if(SysLibLoad('lib1', CREATORID, libRef1) != 0)
	{
		EventType newEvent;
		FrmAlert(alertLibNotFound);
		newEvent.eType = appStopEvent;
		EvtAddEventToQueue (&newEvent);	
		return false;
	}
	if(SysLibLoad('lib2', CREATORID, libRef2) != 0)
	{
		EventType newEvent;
		FrmAlert(alertLibNotFound);
		newEvent.eType = appStopEvent;
		EvtAddEventToQueue (&newEvent);	
		SysLibRemove(*libRef1);
		return false;
	}
#endif
	return true;
}

Boolean SublaunchAllocGlobals()
{
	globalVars* globals;
	globals = getGlobalsPtr();
 	if(globals)
 	{
 		return false;
 	}
	AllocGlobals();
	globals = getGlobalsPtr();
#ifndef LIBDEBUG
	LoadAddressXTLibraries(&globals->adxtLibRef, &globals->adxtLib2Ref);
#endif
	return true;
}

void SublaunchDeallocGlobals()
{
#ifndef LIBDEBUG
	globalVars* globals;
	globals = getGlobalsPtr();
	if(globals)
	{
		SysLibRemove(globals->adxtLibRef);
		SysLibRemove(globals->adxtLib2Ref);	
	}
#endif		
	DeleteGlobals();
}

/*Boolean AttentionProc(AttnLaunchCodeArgsType * paramP, UInt16 launchFlags)
{
	AttnCommandArgsType * argsP = paramP->commandArgsP;
	UInt32 ref=paramP->userData;
	UInt16 counter, x, y;
	Char lStr[255];
	UInt16 adxtLibRef = 0;
	univAppInfoPtr       appInfoPtr;
	univAddrDBRecordType record;
	Char                 PrvAppSearchPhoneLabelLetters[univNumPhoneLabels];
	Char *               unnamedRecordStringPtr = NULL;
	MemHandle			 unnamedRecordStringH = NULL;
	DmOpenRef            dbP = 0;
	LocalID              dbID;
	RectangleType        r;
	UInt16               cardNo=0;
	MemHandle            recordH;
	Boolean found;
	UInt16				sortByCompany;
	UInt32				*gotoInfoP;
#ifndef CONTACTS
	DateTimeType 		birthdate;
#endif
	DmOpenRef 			dbXTP=0;
	UInt16 				addrXTDBSize;
	UInt16 				d, m, year;
	LocalID 			dbXTID, appID;
	DateFormatType 		dateFormat;
	Char 				birthdayStr[31];
	UInt16 				attr;
#ifdef CONTACTS
	DateType 			bday;
#endif
	privateRecordViewEnum privateRecordVisualStatus;
	
	globalVars* globals = getGlobalsPtr();
	
	if(globals == NULL)
		return true;
	
	appID=DmFindDatabase(0, "Address XT");

	if(ref == 0)
	{
		//sent from reset
		AttnForgetIt(0, DmFindDatabase(0, APP_DBNAME), ref);
		CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &dbXTP);
		addrXTDBSize = DmNumRecords(dbXTP);
		if(addrXTDBSize>0)
		{
			UpdateAlarms(globals->adxtLibRef, dbXTP);
		}
		DmCloseDatabase(dbXTP);
		return true;
	}

	if((paramP->command!=kAttnCommandDrawList) && (paramP->command!=kAttnCommandGotIt) && (paramP->command!=kAttnCommandGoThere))
		return true;
	
#ifdef CONTACTS
	dbID=DmFindDatabase(0, CONTACTS_DBNAME);
#else
	dbID=DmFindDatabase(0, "AddressDB");
#endif

	if(dbID == 0)
	{
		return true;
	}
	dbP = DmOpenDatabase(0, dbID, dmModeReadOnly);  

	if(dbP == 0)
	{
		dbID=DmFindDatabase(0, "AddressDB");
		if (dbID==0)
		{
			return true;
		}		
		// Open the address database.
		dbP = DmOpenDatabase(cardNo, dbID, dmModeReadOnly);
		if (!dbP)
		{
			return true;
		}	
	}
	found=false;
	
	if(dbP == 0)
	{
		return true;
	}
	
	if(DmFindRecordByID(dbP, ref, &counter)!=0)
	{
		DmCloseDatabase(dbP);
		return true;
	}
	else
		found=true;
	
	DmRecordInfo(dbP, counter, &attr, NULL, NULL);
	if(attr & dmRecAttrDelete )
	{
		DmCloseDatabase(dbP);
		return true; 	
	}
	privateRecordVisualStatus = (privateRecordViewEnum)PrefGetPreference(prefShowPrivateRecords);
	if((attr & dmRecAttrSecret) && privateRecordVisualStatus != showPrivateRecords)
	{
		DmCloseDatabase(dbP);
		return true;
	} 					
				
	switch (paramP->command) 
		{
		case kAttnCommandDrawDetail:
			break;			

		case kAttnCommandDrawList:
			
			if(!found)
				break;
			found = false;
#ifdef CONTACTS
			if(PrvP1ContactsDBGetRecord(globals->adxtLibRef, dbP, counter, &record, &recordH) != 0)
				return true;			  
			bday = record.birthdayInfo.birthdayDate;
			d=bday.day;
			m=bday.month;
			year=bday.year + 1904;
			found = true;
#else
			dbXTID=DmFindDatabase(0, ADDRXTDB_DBNAME);
			CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &dbXTP);
			addrXTDBSize = DmNumRecords(dbXTP);
			StrCopy(lStr, "Birthday: ");
			if(addrXTDBSize>0)
			{
				for(counter=0;counter<addrXTDBSize;counter++)
				{
					AddrXTDBRecordType rec;
					MemHandle mH;
					UInt32 id, seconds;
					
					if(AddrXTDBGetRecord(globals->adxtLibRef, dbXTP, counter, &rec, &mH) != 0)
						continue;
					
					id = rec.id;
					seconds = rec.bday;
					
					MemHandleUnlock(mH);
		
					if(ref == id)
					{
						TimSecondsToDateTime(seconds, &birthdate);
						d=birthdate.day;
						m=birthdate.month;
						year=birthdate.year;
						found = true;
						break;		
					}
				}
			}
#endif
			if(found)
			{
				dateFormat=PrefGetPreference(prefLongDateFormat);
				DateToAscii(m, d, year, dateFormat, birthdayStr);
				StrCopy(lStr, "Birthday: ");
				StrCat(lStr, birthdayStr);
			}
			else
			{
#ifndef CONTACTS
				if(dbXTP!=0)
					DmCloseDatabase(dbXTP);
#endif
				return true;
			}			
			y = paramP->commandArgsP->drawList.bounds.topLeft.y;
			x = paramP->commandArgsP->drawList.bounds.topLeft.x;
			WinDrawChars(lStr, StrLen(lStr), x, y);
			
#ifndef CONTACTS
			if(univAddrDBGetRecord (globals->adxtLibRef, dbP, counter, &record, &recordH) != 0)
			{
				if(dbP != 0)
					DmCloseDatabase(dbP);
				if(dbXTP!=0)
					DmCloseDatabase(dbXTP);			
				return true;
			}
#endif
			appInfoPtr = univAddrDBAppInfoGetPtr(globals->adxtLibRef, dbP);
			if(appInfoPtr == 0)
			{
				DmCloseDatabase(dbP);
				return true;
			}
			ToolsInitPhoneLabelLetters(globals->adxtLibRef, appInfoPtr, PrvAppSearchPhoneLabelLetters);
			// Display the title of the description.
			FntSetFont (stdFont);
			RctCopyRectangle(&(paramP->commandArgsP->drawList.bounds), &r);
			r.extent.x-=5;
			if(r.extent.y>=22)
			{
				r.topLeft.y+=11;
				r.extent.y-=11;
			}
			sortByCompany=GetSortByCompany(globals->adxtLibRef);
		
			ToolsDrawRecordNameAndPhoneNumber (globals->adxtLibRef, &record, &r, PrvAppSearchPhoneLabelLetters, sortByCompany, &unnamedRecordStringPtr, &unnamedRecordStringH, true);
									
			MemPtrUnlock(appInfoPtr);	
			if(recordH)
				MemHandleUnlock(recordH);
			if ( unnamedRecordStringPtr != 0 )
				MemPtrUnlock(unnamedRecordStringPtr);
			if ( unnamedRecordStringH != 0 )
				DmReleaseResource(unnamedRecordStringH);
			WinDrawChars(lStr, StrLen(lStr), x, y);//strange but needed!
#ifndef CONTACTS
			if(dbXTP!=0)
				DmCloseDatabase(dbXTP);	
#endif		
			break;
		
		case kAttnCommandGotIt:
			
			dbXTID=DmFindDatabase(0, ADDRXTDB_DBNAME);
			CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &dbXTP);
			addrXTDBSize = DmNumRecords(dbXTP);
			if(addrXTDBSize>0)
			{
				for(counter=0;counter<addrXTDBSize;counter++)
				{
					AddrXTDBRecordType rec;
					MemHandle mH;
					UInt32 id, seconds, passed, remind;
					
					if(AddrXTDBGetRecord(globals->adxtLibRef, dbXTP, counter, &rec, &mH) != 0)
						continue;
					
					id = rec.id;
					seconds = rec.bday;
					passed = rec.passed;
					remind = rec.remind;					

					if(ref==id)
					{
						//replace record with identical but with PASSED=1
						rec.passed = 1;
						MemHandleUnlock(mH);
						AddrXTDBChangeRecord(globals->adxtLibRef, dbXTP, &rec, counter);
						break;
					}
					MemHandleUnlock(mH);
				}
			}
			// appID not initialized there before used
			AttnForgetIt(0, appID, ref);			
			
			UpdateAlarms(globals->adxtLibRef, dbXTP);
			if(dbXTP!=0)
				DmCloseDatabase(dbXTP);				
			break;			

		case kAttnCommandGoThere:
		{		
			Boolean launched;
			GoToParamsType gotoParams;
			if(!found)
				return true;
			gotoParams.recordNum=counter; 
			gotoParams.dbCardNo=0;
			gotoParams.dbID=dbID;
			gotoParams.matchPos=0;
			gotoParams.matchFieldNum=0;
			gotoParams.matchCustom=0;
			gotoParams.searchStrLen=0;
			
			launched = launchFlags & sysAppLaunchFlagNewGlobals;
			
			gotoInfoP = (UInt32*)MemPtrNew (sizeof(UInt32));
			ErrFatalDisplayIf ((!gotoInfoP), "Out of memory");
			MemPtrSetOwner(gotoInfoP, 0);
			*gotoInfoP = counter;
			dbID=DmFindDatabase(0, "Address XT");
			if(dbXTP!=0)
				DmCloseDatabase(dbXTP);
			SysUIAppSwitch(0, dbID, appLaunchCmdAlarmEventGoto, gotoInfoP);
		}
		break;	

		case kAttnCommandIterate:
			break;	
		}
	DmCloseDatabase (dbP);
	return true;
}
*/

UInt32   PilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	return PrvAppPilotMain(cmd, cmdPBP, launchFlags);
}

#pragma mark -

// Note: We need to create a branch island to PilotMain in order to successfully
//  link this application for the device.
UInt32 PrvAppPilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	globalVars* globals;
	Err error = errNone; //, err;
	UInt32 ref; 
	UInt16 data;
	Boolean globalsAllocated;
	switch (cmd)
	{
	case sysAppLaunchCmdNormalLaunch:
#ifdef DEBUG
		LogInit("xt_log");
		LogWrite("xt_log", "main", "app start");
#endif

		error = PrvAppStart ();
		if (error)
			return (error);
		
		globals = getGlobalsPtr();
	 	TransferRegisterData(globals->adxtLibRef);
		
		if(globals->gAddrView && globals->gRememberLastContact)
		{
			globals->gAddrView=false;
			globals->CurrentRecord=globals->gLastRecord;
			FrmGotoForm(RecordView);
		}
		else
		{
			FrmGotoForm (ListView);
		}
		PrvAppEventLoop ();
		PrvAppStop ();
		break;
	/*case MapopolisCallbackLaunch:
		// the app was launched back from Mapopolis
		if ( launchFlags & sysAppLaunchFlagNewGlobals )
		{
			error = PrvAppStart ();
			if (error)
				return (error);

			globals = getGlobalsPtr();
	 		TransferRegisterData(globals->adxtLibRef);
			
			if(globals->gAddrView && globals->gRememberLastContact)
			{
				globals->gAddrView=false;
				globals->CurrentRecord=globals->gLastRecord;
				FrmGotoForm(RecordView);
			}
			else
			{
				FrmGotoForm (ListView);
			}
			PrvAppEventLoop ();
			PrvAppStop ();
		}
		break;*/
	case sysAppLaunchCmdAttention:
	/*	globalsAllocated = SublaunchAllocGlobals();
		AttentionProc((AttnLaunchCodeArgsType*)cmdPBP, launchFlags);
		if(globalsAllocated)
			SublaunchDeallocGlobals();
		break;
	case sysAppLaunchCmdAlarmTriggered:
		ref=((SysAlarmTriggeredParamType*)cmdPBP)->ref;
		while(AttnForgetIt(0, (LocalID)DmFindDatabase(0, APP_DBNAME), ref)){;};
		AttnGetAttention(0, (LocalID)DmFindDatabase(0, APP_DBNAME), ref, NULL, kAttnLevelSubtle, kAttnFlagsLEDBit, 0, 20);
		break;*/	
	case sysAppLaunchCmdFind:
		globalsAllocated = SublaunchAllocGlobals();
		PrvAppSearch ((FindParamsPtr) cmdPBP);
		if(globalsAllocated)
			SublaunchDeallocGlobals();
		break;
	/*case appLaunchCmdAlarmEventGoto:
			{
				Boolean launched;

				launched = launchFlags & sysAppLaunchFlagNewGlobals;

				if (launched)
				{
					error = PrvAppStart ();
					if (error)
						return (error);
				}
				dia_init();
				PrvAppGoToItemSmp (*((UInt32*)cmdPBP), true);
				
				if (launched)
				{
					PrvAppEventLoop ();
					PrvAppStop ();
				}
			}
			break;


	case sysAppLaunchCmdTimeChange:
			//update alarms
			{
				DmOpenRef dbXTP=0;
				LocalID dbXTID;
				UInt32 addrXTDBSize;
				globalsAllocated = SublaunchAllocGlobals();
				globals = getGlobalsPtr();
	 			CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &dbXTP);
				if(dbXTP==0)
					break;
				dbXTID=DmFindDatabase(0, ADDRXTDB_DBNAME);
				if(dbXTID==0)
					break;
				addrXTDBSize = DmNumRecords(dbXTP);
				if(addrXTDBSize>0)
				{
					UpdateAlarms(globals->adxtLibRef, dbXTP);
				}
				DmCloseDatabase(dbXTP);
				if(globalsAllocated)
					SublaunchDeallocGlobals();
			}
			break;	
			// This action code could be sent to the app when it's already running.
	*/
	case sysAppLaunchCmdGoTo:
			{
				Boolean launched;

				launched = launchFlags & sysAppLaunchFlagNewGlobals;

				if (launched)
				{
					error = PrvAppStart ();
					if (error)
						return (error);
				}
				dia_init();
				PrvAppGoToItem ((GoToParamsPtr)cmdPBP, launched);
							
				if (launched)
				{
					PrvAppEventLoop ();
					PrvAppStop ();
				}
			}
		break;


	case sysAppLaunchCmdSyncNotify:
		PrvAppHandleSync();
		break;


		// Launch code sent to running app before sysAppLaunchCmdFind
		// or other action codes that will cause data PrvAppSearches or manipulation.
	case sysAppLaunchCmdSaveData:
		FrmSaveAllForms ();
		break;

		// This launch code is sent after the system is reset.  We use this time
		// to create our default database.  If there is no default database image,
		// then we create an empty database.
	/*case sysAppLaunchCmdSystemReset:
		
		//SysNotifyRegister(0, DmFindDatabase(0, APP_DBNAME), sysNotifySyncFinishEvent , NULL, sysNotifyNormalPriority, &data);

		//globalsAllocated = SublaunchAllocGlobals();
	
		// Register to receive vcf files on hard reset.
		globals = getGlobalsPtr();
	 	TransferRegisterData(globals->adxtLibRef);
		//update alarms
		{
			DmOpenRef dbXTP=0;
			UInt32 addrXTDBSize;
			CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &dbXTP);
			if(dbXTP==0)
				break;
			addrXTDBSize = DmNumRecords(dbXTP);
			if(addrXTDBSize>0)
			{
				UInt32 nowSeconds=TimGetSeconds();
				AlmSetAlarm(0, DmFindDatabase(0, APP_DBNAME), 0, nowSeconds+5, false);				
			}
			DmCloseDatabase(dbXTP);
		}
		if(globalsAllocated)
			SublaunchDeallocGlobals();
		break;
*/

		// Present the user with ui to perform a lookup and return a string
		// with information from the selected record.
	/*case sysAppLaunchCmdLookup:
		globalsAllocated = SublaunchAllocGlobals();
		globals = getGlobalsPtr();
 		globals->startupType = startupLookup;
		globals->SelectedLookupRecord = noRecord;
		if(globalsAllocated)
		{
			PrvAppStartEx(true);
		}
		Lookup((AddrLookupParamsPtr) cmdPBP);
		if(globalsAllocated)
			PrvAppStopEx(true);
		if(globalsAllocated)
			SublaunchDeallocGlobals();
		break;*/

	case sysAppLaunchCmdExgReceiveData:
		{
	      UInt32 currentUID;
	      DmOpenRef dbP;
		  globals = getGlobalsPtr();
	 	  
			// if our app is not active, we need to open the database
			// the subcall flag is used here since this call can be made without launching the app
			if (!(launchFlags & sysAppLaunchFlagSubCall))
			{
#ifdef CONTACTS
				dbP = DmOpenDatabase(0, DmFindDatabase(0, CONTACTS_DBNAME), dmModeReadWrite);  
#else
				error = AddrDBGetDatabase (globals->adxtLibRef, &dbP, dmModeReadWrite);				
#endif
			}
			else
			{
				globals = getGlobalsPtr();
	 			dbP = globals->AddrDB;
				
				// We don't save the current record because the keyboard dialog could have
				// stolen the table's field's handle. There's no need anyway.
				
				// TransferReceiveData() inserts the received record in sorted order. This may change the
				// index of the current record. So we remember its UID here, and refresh our copy of its
				// index afterwards.
				if (globals->CurrentRecord != noRecord)
					DmRecordInfo(dbP, globals->CurrentRecord, NULL, &currentUID, NULL);
			}

			if (dbP != NULL)
			{
				error = TransferReceiveData(globals->adxtLibRef, dbP, (ExgSocketPtr) cmdPBP);

				if (launchFlags & sysAppLaunchFlagSubCall)
				{
					if (globals->CurrentRecord != noRecord)
					{
						if (DmFindRecordByID(dbP, currentUID, &(globals->CurrentRecord)) != 0)
							globals->CurrentRecord = noRecord;	// Can't happen, but...
						
					}
				}
				else
					DmCloseDatabase(dbP);
			}
			else
				error = exgErrAppError;	
			// If we can't open our database, return the error since it wasn't passed to ExgDisconnect
		}
		break;

	case sysAppLaunchCmdExgPreview:
		globals = getGlobalsPtr();
	 	TransferPreview(globals->adxtLibRef, (ExgPreviewInfoType *)cmdPBP);
		break;

	
	case sysAppLaunchCmdNotify :
		{
			
			if (((SysNotifyParamType*)cmdPBP)->notifyType == sysNotifySyncFinishEvent)
			{
				PrvAppHandleSync();
			}
			if (((SysNotifyParamType*)cmdPBP)->notifyType == sysNotifyDisplayChangeEvent)
			{
				Boolean appIsActive;
				EventType resizedEvent;
				appIsActive = (Boolean) (launchFlags & sysAppLaunchFlagSubCall);
                if (appIsActive)
                {
                    MemSet(&resizedEvent, sizeof(EventType), 0); 
					resizedEvent.eType = winDisplayChangedEvent;
					EvtAddUniqueEventToQueue(&resizedEvent, 0, true); 
                }			
			}
			if (((SysNotifyParamType*) cmdPBP)->notifyType ==sysNotifyDisplayResizedEvent)
			{
				EventType resizedEvent; 
				MemSet(&resizedEvent, sizeof(EventType), 0); 
				resizedEvent.eType = winDisplayChangedEvent;
				EvtAddUniqueEventToQueue(&resizedEvent, 0, true);
			}
		}
		break;
	}

	return error;
}

void PrvAppSearch(FindParamsPtr findParams)
{
#ifdef CONTACTS
	P1ContactsDBRecordType    	record;
	P1ContactsAppInfoPtr      	appInfoPtr;
	Char                 PrvAppSearchPhoneLabelLetters[P1ContactsnumPhoneLabels];
#else
	AddrDBRecordType     		record;
	AddrAppInfoPtr       		appInfoPtr;
	Char                 PrvAppSearchPhoneLabelLetters[numPhoneLabels];
#endif
	Boolean              done;
	Boolean              match;
	Char *               header;
	Char *               unnamedRecordStringPtr = NULL;
	MemHandle			 unnamedRecordStringH = NULL;
	DmOpenRef            dbP;
	DmSearchStateType    PrvAppSearchState;
	Err                  err;
	MemHandle            headerStringH;
	LocalID              dbID;
	RectangleType        r;
	UInt16               cardNo=0;
	UInt16               recordNum;
	MemHandle            recordH;
	UInt16               i;
	UInt32               matchPos;
	UInt16					matchLen;
	UInt32 encoding;
	Int16 prefsVersion=noPreferenceFound;
	UInt16 prefsSize=0;
	AddrExtPreferenceType prefs;
	Boolean fullText=false;
	globalVars* globals = getGlobalsPtr();
	UInt16 sortByCompany = GetSortByCompany(globals->adxtLibRef);
	
	
	PrefGetAppPreferences (CREATORID, addrPrefExtID, NULL, &prefsSize, true);
	prefsSize = sizeof (AddrExtPreferenceType);
	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefExtID, &prefs, &prefsSize, true);
		if (prefsVersion != noPreferenceFound)
		{
			fullText=prefs.FullTextSearch;
		}	
	}
	
	// Find the application's data file.
#ifdef CONTACTS
	err = DmGetNextDatabaseByTypeCreator (true, &PrvAppSearchState, contactsDBType,
									  contactsFileCAddress, true, &cardNo, &dbID);
#else
	err = DmGetNextDatabaseByTypeCreator (true, &PrvAppSearchState, addrDBType,
									  sysFileCAddress, true, &cardNo, &dbID);
#endif
	if (err)
	{
		findParams->more = false;
		return;
	}
	
	// Open the address database.
	dbP = DmOpenDatabase(cardNo, dbID, findParams->dbAccesMode);
	if (!dbP)
	{
		findParams->more = false;
		return;
	}

	// Display the heading line.
	headerStringH = DmGetResource(strRsc, FindAddrHeaderStr);
	header = MemHandleLock(headerStringH);
	done = FindDrawHeader (findParams, header);
	MemHandleUnlock(headerStringH);
	DmReleaseResource(headerStringH);
	if (done)
		goto Exit;

	// PrvAppSearch the description and note fields for the "find" string.
	recordNum = findParams->recordNum;
	while (true)
	{
		UInt16 maxFields;
		// Because applications can take a long time to finish a find when
		// the result may be on the screen or for other reasons, users like
		// to be able to stop the find.  Stop the find if an event is pending.
		// This stops if the user does something with the device.  Because
		// this call slows down the PrvAppSearch we perform it every so many
		// records instead of every record.  The response time should still
		// be Int16 without introducing much extra work to the PrvAppSearch.

		// Note that in the implementation below, if the next 16th record is
		// secret the check doesn't happen.  Generally this shouldn't be a
		// problem since if most of the records are secret then the PrvAppSearch
		// won't take long anyways!
		if ((recordNum & 0x000f) == 0 &&         // every 16th record
			EvtSysEventAvail(true))
		{
			// Stop the PrvAppSearch process.
			findParams->more = true;
			break;
		}

		recordH = DmQueryNextInCategory (dbP, &recordNum, dmAllCategories);

		// Have we run out of records?
		if (! recordH)
		{
			findParams->more = false;
			break;
		}

		// PrvAppSearch all the fields of the address record.
		univAddrDBGetRecord (globals->adxtLibRef, dbP, recordNum, &record, &recordH);
		
		match = false;
		if (FtrGet(sysFtrCreator, sysFtrNumEncoding, 
 		 &encoding) != errNone)
 		 encoding = charEncodingPalmLatin;
 		 
#ifndef CONTACTS
 			maxFields = addrNumFields;
#else
 			maxFields = P1Contactsnote + 1;
#endif 
 		for (i = 0; i < maxFields; i++)
		{
			Boolean test;
			test = (record.fields[i] != 0);
			if (test)
			{
				if (encoding == charEncodingPalmSJIS || !fullText) 
				{
					match = TxtFindString(record.fields[i], findParams->strToFind, &matchPos, &matchLen);
					if (match)
					{
						break;
					}
				}
				else
				{
										
					Char *str1, *str2;
					if(record.fields[i]==0)
					{
						match=false;
						continue;
					}
					if(StrLen(record.fields[i])==0)
					{
						match=false;
						continue;
					}
					str1=MemPtrNew(sizeof(Char)*StrLen(record.fields[i])*2);
					str2=MemPtrNew(sizeof(Char)*StrLen(findParams->strAsTyped)*2);
					StrToLower(str1, record.fields[i]);
					StrToLower(str2, findParams->strAsTyped);
					if(!StrStr(str1, str2))
						match=false;
					else
					{
						matchPos=StrStr(str1, str2)-str1;
						matchLen=StrLen(str2);
						match=true;
					}
					MemPtrFree(str1);
					MemPtrFree(str2);
					if (match)
					{
						break;
					}
				}
			}
		}

		if (match)
		{
			// Add the match to the find paramter block, if there is no room to
			// display the match the following function will return true.
			done = FindSaveMatch (findParams, recordNum, matchPos, i, matchLen, cardNo, dbID);
			if (done)
			{
				MemHandleUnlock(recordH);
				break;
			}

			// Get the bounds of the region where we will draw the results.
			FindGetLineBounds (findParams, &r);

#ifdef CONTACTS
			appInfoPtr = (P1ContactsAppInfoPtr) AddrDBAppInfoGetPtr(globals->adxtLibRef, dbP);
			ToolsInitPhoneLabelLetters(globals->adxtLibRef, appInfoPtr, PrvAppSearchPhoneLabelLetters);	
#else
			appInfoPtr = (AddrAppInfoPtr) AddrDBAppInfoGetPtr(globals->adxtLibRef, dbP);
			ToolsInitPhoneLabelLetters(globals->adxtLibRef, appInfoPtr, PrvAppSearchPhoneLabelLetters);	
#endif			
			// Display the title of the description.
			FntSetFont (stdFont);

			ToolsDrawRecordNameAndPhoneNumber(globals->adxtLibRef, (void*)&record, &r, PrvAppSearchPhoneLabelLetters, sortByCompany, &unnamedRecordStringPtr, &unnamedRecordStringH, true);
			MemPtrUnlock(appInfoPtr);
			findParams->lineNumber++;
		}

		MemHandleUnlock(recordH);
		recordNum++;
	}

	if ( unnamedRecordStringPtr != 0 )
		MemPtrUnlock(unnamedRecordStringPtr);

	if ( unnamedRecordStringH != 0 )
		DmReleaseResource(unnamedRecordStringH);

Exit:
	DmCloseDatabase (dbP);
}




void PrvAppGoToItemSmp (UInt16 index, Boolean launchingApp)
{
	UInt16 formID;
	UInt16 recordNum;
	UInt16 attr;
	UInt32 uniqueID;
	EventType event;

	globalVars* globals = getGlobalsPtr();

	recordNum = index;
	if (!DmQueryRecord(globals->AddrDB, recordNum))
	{	
		if (!ToolsSeekRecord(globals->adxtLibRef, &recordNum, 0, dmSeekBackward))
			if (!ToolsSeekRecord(globals->adxtLibRef, &recordNum, 0, dmSeekForward))
			{
				FrmAlert(secGotoInvalidRecordAlert);
				FrmGotoForm(ListView);
				return;
			}
	}
	DmRecordInfo (globals->AddrDB, recordNum, &attr, &uniqueID, NULL);

	// Change the current category if necessary.
	if (globals->CurrentCategory != dmAllCategories)
	{
		globals->CurrentCategory = attr & dmRecAttrCategoryMask;
	}


	// If the application is already running, close all the open forms.  If
	// the current record is blank, then it will be deleted, so we'll
	// the record's unique id to find the record index again, after all
	// the forms are closed.
	if (! launchingApp)
	{
		FrmCloseAllForms ();
	}


	// Set global variables that keep track of the currently record.
	globals->CurrentRecord = recordNum;

	// Set PriorAddressFormID so the Note View returns to the List View
	globals->PriorAddressFormID = ListView;

	formID = RecordView;

	MemSet (&event, sizeof(EventType), 0);

	// Send an event to load the form we want to goto.
	event.eType = frmLoadEvent;
	event.data.frmLoad.formID = formID;
	EvtAddEventToQueue (&event);

	// Send an event to goto a form and select the matching text.
	event.eType = frmGotoEvent;
	event.data.frmGoto.formID = formID;
	event.data.frmGoto.recordNum = recordNum;
	event.data.frmGoto.matchPos = 0;
	event.data.frmGoto.matchLen = 0;
	event.data.frmGoto.matchFieldNum = 0;
	EvtAddEventToQueue (&event);

}

void PrvAppGoToItem (GoToParamsPtr goToParams, Boolean launchingApp)
{
	UInt16 formID;
	UInt16 recordNum;
	UInt16 attr;
	UInt32 uniqueID;
	EventType event;

	globalVars* globals = getGlobalsPtr();

	recordNum = goToParams->recordNum;
	if (!DmQueryRecord(globals->AddrDB, recordNum))
	{	
		if (!ToolsSeekRecord(globals->adxtLibRef, &recordNum, 0, dmSeekBackward))
			if (!ToolsSeekRecord(globals->adxtLibRef, &recordNum, 0, dmSeekForward))
			{
				FrmAlert(secGotoInvalidRecordAlert);
				FrmGotoForm(ListView);
				return;
			}
	}
	DmRecordInfo (globals->AddrDB, recordNum, &attr, &uniqueID, NULL);

	// Change the current category if necessary.
	if (globals->CurrentCategory != dmAllCategories)
	{
		globals->CurrentCategory = attr & dmRecAttrCategoryMask;
	}


	// If the application is already running, close all the open forms.  If
	// the current record is blank, then it will be deleted, so we'll
	// the record's unique id to find the record index again, after all
	// the forms are closed.
	if (! launchingApp)
	{
		FrmCloseAllForms ();
		DmFindRecordByID (globals->AddrDB, uniqueID, &recordNum);
	}


	// Set global variables that keep track of the currently record.
	globals->CurrentRecord = recordNum;

	// Set PriorAddressFormID so the Note View returns to the List View
	
	globals->PriorAddressFormID = ListView;

#ifdef CONTACTS
	if (goToParams->matchFieldNum == P1Contactsnote)
		formID = NewNoteView;
	else
		formID = RecordView;
#else
	if (goToParams->matchFieldNum == note)
		formID = NewNoteView;
	else
		formID = RecordView;
#endif
	MemSet (&event, sizeof(EventType), 0);

	// Send an event to load the form we want to goto.
	event.eType = frmLoadEvent;
	event.data.frmLoad.formID = formID;
	EvtAddEventToQueue (&event);
	
	CstHighResInit();
	//CstHRPlusInit();
	//CstDIAInit();
	dia_init();
	
	LoadColors(globals->adxtLibRef);
	if(globals->gScreen!=PALM_320x320)
	{
		UInt32 width, height;
		Boolean enableColor;
		
		WinScreenMode (winScreenModeGet, &width, &height, &(globals->gDepth), &enableColor);

	}
	
	// Send an event to goto a form and select the matching text.
	event.eType = frmGotoEvent;
	event.data.frmGoto.formID = formID;
	event.data.frmGoto.recordNum = recordNum;
	event.data.frmGoto.matchPos = goToParams->matchPos;
	event.data.frmGoto.matchLen = goToParams->matchCustom;
	event.data.frmGoto.matchFieldNum =  goToParams->matchFieldNum;
	EvtAddEventToQueue (&event);

}

void PrvAppHandleSync(void)
{
	globalVars* globals = getGlobalsPtr();
	DmOpenRef dbP;
#ifndef CONTACTS
	Err err;
#endif
	
	UInt16 sortByCompany = GetSortByCompany(globals->adxtLibRef);
	
	// Find the application's data file.
#ifdef CONTACTS
	dbP = DmOpenDatabase(0, DmFindDatabase(0, CONTACTS_DBNAME), dmModeReadWrite);  
	if(dbP == 0)
		return;
#else
	err = AddrDBGetDatabase (globals->adxtLibRef, &dbP, dmModeReadWrite);
	if (err)
		return;
#endif
	AddrDBChangeSortOrder(globals->adxtLibRef, dbP,sortByCompany);

	DmCloseDatabase (dbP);
}

Boolean PrvAppLaunchCmdDatabaseInit(DmOpenRef dbP)
{
	globalVars* globals = getGlobalsPtr();
#ifndef CONTACTS
	Err err;
#endif
	if (!dbP)
		return false;

	// Set the backup bit.  This is to aid syncs with non Palm software.
	ToolsSetDBAttrBits(globals->adxtLibRef, dbP, dmHdrAttrBackup);

	// Initialize the database's app info block
#ifndef CONTACTS
	err = AddrDBAppInfoInit (globals->adxtLibRef, dbP);
	if (err)
		return false;
#endif
	return true;
}

void AllocGlobals()
{
	MemPtr globalsPtr = NULL;
	globalVars* globals = NULL;
	UInt16 i;
	
	globalsPtr = MemPtrNew(sizeof(globalVars));
	if(globalsPtr == NULL)
		return;
	if(FtrSet(CREATORID, 1, (UInt32)globalsPtr) != 0)
		return;
	
	MemPtrSetOwner(globalsPtr, 0);	
	globals = (globalVars*)globalsPtr;
	globals->gDeviceType = type_palm;
	MemSet(&(globals->gDeviceFlags.bits), sizeof(globals->gDeviceFlags.bits), 0);
	globals->gFiveWay = false;
	globals->gJogDial = false;
	globals->TopVisibleRecord = 0;
	globals->TopVisibleFieldIndex = 0;
	globals->SelectedRecord = noRecord;
	globals->ListViewSelectThisRecord = noRecord;
	globals->CurrentRecord = noRecord;
	
	globals->gBListDay=1;
	globals->gLinkTypeSel = 0;
	globals->gLinkType = 0;
	globals->gAddressView=0;
	globals->gFiveWay = 0;

	globals->EditLabelColumnWidth = 0;
	globals->RecordLabelColumnWidth = 0;
	globals->UnnamedRecordStringPtr = 0;
	globals->UnnamedRecordStringH = 0;
	
	globals->CurrentCategory = dmAllCategories;
	globals->EnableTapDialing = false;	// tap dialing is not enabled by default
	globals->gAdvancedFind = 0;
	globals->ShowAllCategories = true;
	globals->SaveBackup = true;
	globals->SaveCatBackup = true;
	globals->RememberLastCategory = false;
	globals->NoteFont = stdFont;
	globals->AddrListFont = stdFont;
	globals->AddrRecordFont = largeBoldFont;
	globals->AddrEditFont = largeBoldFont;
	globals->BusinessCardRecordID = dmUnusedRecordID;
	
	for(i = 0; i < connectoptions_num; i++)
	{
		globals->ContactSettings[i].strings = NULL;
		globals->ContactSettings[i].num = 0;
		globals->ContactSettings[i].crIDs = NULL;
	}
	
	globals->AppButtonPushedModifiers = 0;
	globals->BusinessCardSentForThisButtonPress = false;
	globals->AppButtonPushed = nullChr;

	globals->gReturnFocusToTable = false;

	globals->LastSeconds = 0;
	globals->ScrollUnits = 0;

	globals->ScrollPosition=0;

	globals->bitmapH = 0;
	globals->bitmap = 0;
	
	globals->recordViewRecordH = 0;

	globals->ContactsSelScrollUnits = 0;
	
	globals->ContactSelSelectedRecord = noRecord;
	globals->ContactsSelTopVisibleRecord = 0;

	globals->ContactsSelCurrentCategory = dmAllCategories;

	globals->trigger_state = 0;

	globals->vskVersion = 0; 
	globals->area_state = 0;
	globals->pin_state = 0;
	globals->Date.year = 91;
	globals->Date.month = 7;
	globals->Date.day = 31;
	
	globals->PendingUpdate = 0;	
	globals->colorLine.index = 0x00;
	globals->colorLine.r = 0x77;
	globals->colorLine.g = 0x77;
	globals->colorLine.b = 0x77;
	
	globals->adxtLibRef = sysInvalidRefNum;
	globals->adxtLib2Ref = sysInvalidRefNum;
	
	globals->InPhoneLookup = false;

	globals->DayEditPosition = 0;	
	globals->DateCurrentRecord = noRecordSelected;
	globals->ItemSelected = false;
	globals->RecordDirty = false;

	globals->DayStartHour = defaultDayStartHour;	
	globals->DayEndHour = defaultDayEndHour;		
	globals->StartDayOfWeek = sunday;
	globals->RepeatStartOfWeek = sunday;		
	globals->AlarmPreset.advance = defaultAlarmPresetAdvance;
	globals->AlarmPreset.advanceUnit = defaultAlarmPresetUnit;
	globals->ShowTimeBars = defaultShowTimeBars;			
	globals->CompressDayView = defaultCompressDayView;	
	globals->ShowTimedAppts = defaultShowTimedAppts;	
	globals->ShowUntimedAppts = defaultShowUntimedAppts;	
	globals->ShowDailyRepeatingAppts = defaultShowDailyRepeatingAppts;	
	
	globals->TimeDisplayed = false;				
	globals->AlarmSoundRepeatCount = defaultAlarmSoundRepeatCount;
																																																			
	globals->AlarmSoundRepeatInterval = defaultAlarmSoundRepeatInterval;

	globals->AlarmSoundUniqueRecID = defaultAlarmSoundUniqueRecID;

	globals->AlarmSnooze = defaultAlarmSnooze;		
	
	globals->DismissedAlarmCount = 0;
	globals->MemoTopVisibleRecord = 0;
	globals->MemoCurrentRecord = noRecordSelected;
	globals->MemoCurrentView = MemoListView;
	globals->MemoCurrentCategory = dmAllCategories;
	globals->MemoShowAllCategories = true;
	globals->MemoEditScrollPosition = 0;
	globals->MemoSaveBackup = true;
	
	globals->imageLibRef = 0;
	globals->tonesLibRef = 0;
	globals->gShowNamesOnly = 0;
	globals->gTouchMode = 0;
	
	globals->MemoInPhoneLookup = false;
	globals->ToDoTopVisibleRecord = 0;					
	globals->ToDoPendingUpdate = 0;						
	globals->ToDoCurrentRecord = noRecordSelected;	
	globals->ToDoItemSelected = false;					
	globals->ToDoRecordDirty = false;						
	globals->ListEditPosition = 0;					
	globals->ToDoNoteFont = stdFont;						
	globals->ToDoCurrentCategory = dmAllCategories;	
	globals->ToDoShowAllCategories = true;				
	globals->ShowCompletedItems = true;				
	globals->ShowOnlyDueItems = false;				
	globals->ShowDueDates = false;					
	globals->ShowPriorities = true;					
	globals->ShowCategories = false;					
	globals->ToDoSaveBackup = true;						
	globals->ChangeDueDate = false;					
	globals->ListFont = stdFont;						
	
	globals->ToDoInPhoneLookup = false;					
	StrCopy(globals->gPhoneChars, "0123456789,+#*");
	
	globals->NumCharsToHilite = 0;
	
	globals->startupType = startupNormal;
	globals->lookupString[0] = 0;
	
	globals->ApptsH = 0;
	globals->ApptsOnlyH = 0;
	globals->EditRowIDWhichHadFocus = editFirstFieldIndex;
	globals->gLinkID = 0;
	globals->suppressListFocusEvent = false;
}

void DeleteGlobals()
{
	globalVars* globals = getGlobalsPtr();
	if(globals)
	{
		MemPtrFree(globals);
		FtrUnregister(CREATORID, 1);
	}
}

Boolean PrvAppHandleEvent (EventType * event)
{
	UInt16 formId;
	FormPtr frm;

	if (event->eType == frmLoadEvent)
	{

		// Load the form resource.
		formId = event->data.frmLoad.formID;
		frm = FrmInitForm (formId);
		FrmSetActiveForm (frm);
		
		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formId)
		{
		case DayView:
			FrmSetEventHandler(frm, DayViewHandleEvent);
			break;
		case ContactSelListView:
			FrmSetEventHandler(frm, ContactSelListHandleEvent);
			break;
		case MemoListView:
			FrmSetEventHandler(frm, MemoListViewHandleEvent);
			break;
		case ToDoListView:
			FrmSetEventHandler(frm, ToDoListViewHandleEvent);
			break;
		case CompanyDataPrefDialog:
			FrmSetEventHandler(frm, CDataHandleEvent);
			break;
		case NewLinkDialog:
			FrmSetEventHandler(frm, NLHandleEvent);
			break;
		case ConnectOptionsDialog:
			FrmSetEventHandler(frm, ConnectOptionsHandleEvent);
			break;
		case LinksDialog:
			FrmSetEventHandler(frm, LHandleEvent);
			break;
		case ColorOptionsDialog:
			FrmSetEventHandler(frm, COHandleEvent);
			break;
		case ListView:
			FrmSetEventHandler(frm, ListHandleEvent);
			break;
		case RecordView:
			FrmSetEventHandler(frm, ViewHandleEvent);
			break;
		case EditView:
			FrmSetEventHandler(frm, EditHandleEvent);
			break;
		//case AddressDialog:
		//	FrmSetEventHandler(frm, AddressDialogHandleEvent);
		//	break;
		case NewNoteView:
			FrmSetEventHandler(frm, NoteViewHandleEvent);
			break;
		case DetailsDialog:
			FrmSetEventHandler(frm, DetailsHandleEvent);
			break;
		case BirthdayDialog:
			FrmSetEventHandler(frm, BirthdayHandleEvent);
			break;
		case BirthdaysDialog:
			FrmSetEventHandler(frm, BHandleEvent);
			break;
		case CustomEditDialog:
			FrmSetEventHandler(frm, CustomEditHandleEvent);
			break;
		case ButtonOptionsDialog:
			FrmSetEventHandler(frm, BOHandleEvent);
			break;
		case PreferencesDialog:
			FrmSetEventHandler(frm, PrefsHandleEvent);
			break;
		case DialListDialog:
			FrmSetEventHandler(frm, DialListHandleEvent);
			break;
		case CompanyData:
			FrmSetEventHandler(frm, CompanyHandleEvent);
			break;
		case AboutForm:
			FrmSetEventHandler(frm, AboutHandleEvent);
			break;
		case SerialNumberDialog:
			FrmSetEventHandler(frm, SNHandleEvent);
			break;
		case AddressListOptionsDialog:
			FrmSetEventHandler(frm, ALOHandleEvent);
			break;
		default:
			ErrNonFatalDisplay("Invalid Form Load Event");
			break;
		}

		return (true);
	}
	return (false);
}

/*static Boolean IsKyocera()
{
	UInt32 romVersion;
	UInt16 coreRefNum;
	Err err=0;
	err = SysLibFind(PDQCoreLibName,&coreRefNum);
	if (!err)
	{
		PDQCoreLibGetVersion(coreRefNum, &romVersion);
		romVersion = sysGetROMVerMajor(romVersion);
		if(romVersion != KWC7135Version1)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		return false;
	}
	return false;
}*/

static Boolean IsFirstRun()
{
	UInt16 prefsSize=0;
	PrefGetAppPreferences (CREATORID, addrPrefExtID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefRegID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefExt2ID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefBtnID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefVersionNum, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefColorID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefExt3ID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefRcntID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrInstalledRegID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefCDataID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefSearchID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefImportID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefLinkID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	PrefGetAppPreferences (CREATORID, addrPrefExt4ID, NULL, &prefsSize, true);
	if(prefsSize > 0)
		return false;
	return true;
}

static void FindTomTom()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 volRefNum;
	Boolean found = false;
	Boolean cardsExist = false;
	UInt32 volIterator = vfsIteratorStart;
	UInt16 cardNo;
	LocalID localID;
	FileRef fileRef;
	UInt32 vfsMgrVersion;
	Err err;
	
	DmSearchStateType state;
	globals->gTomTom = false;
	
	
	 
	err = FtrGet(sysFileCVFSMgr,
	  vfsFtrIDVersion, &vfsMgrVersion);
	if(err)
	{	   
	    return;
	}
	
	while (volIterator != vfsIteratorStop)
	{
		Err err = VFSVolumeEnumerate(&volRefNum, &volIterator);
		if (err == errNone)
		{
			FileRef fileRef;
			cardsExist = true;			
			// Do something with the volRefNum
			if(VFSFileOpen(volRefNum, "/TomTom", vfsModeRead, &fileRef) == errNone)
			{
				UInt32 attributes;
				if(VFSFileGetAttributes(fileRef, &attributes) == errNone)
				{
					if(attributes & vfsFileAttrDirectory)
					{
						found = true;
						globals->gTomTomVolRef = volRefNum;
						break;
					}
				}	
				VFSFileClose(fileRef);			
			}
		}
		else
		{
			break;
		}
	}
	if(!cardsExist)
		return;
	if(!found)
	{
		return;
	}
	if(DmGetNextDatabaseByTypeCreator(true, &state, 'appl', 'MniC', false, &cardNo, &localID) != errNone)
	{
		if(VFSFileOpen(globals->gTomTomVolRef, "/Palm/start.prc", vfsModeRead, &fileRef) == errNone)
		{
			UInt32 type;
			UInt32 creator;
			if(VFSFileDBInfo(fileRef, 0, 0, 0, 0, 0, 0, 0, 0, 0, &type, &creator, 0) == errNone)
			{
				if(type == 'appl' && creator == 'MniC')
				{
					if(VFSImportDatabaseFromFile(globals->gTomTomVolRef, "/Palm/start.prc", &cardNo, &localID) == errNone)
					{
						FrmAlert(alertTomTomCopied);
					}
					else
					{
						VFSFileClose(fileRef);
						return;
					}
				}
			}
			VFSFileClose(fileRef);
		}
	}
	
	if(DmGetNextDatabaseByTypeCreator(true, &state, 'appl', 'MniC', false, &cardNo, &localID) != errNone)
		return;

	globals->gTomTom = true;
}

Err PrvAppStartEx(Boolean sublaunch)
{
	Err err = 0;
	UInt16 mode;
	UInt32 id;
	univAppInfoPtr appInfoPtr;
#ifdef CONTACTS
	AddrAppInfoPtr addrAppInfoPtr;
#endif
	UInt16 prefSize=sizeof(RegKey);
	UInt32 romVersion;
	UInt32 *flagP;
	LocalID recentDbId;
	UInt16 attr;
	UInt32 seconds = TimGetSeconds();
	UInt16 i;
	UInt16 data;
	UInt32 ret;
	DateTimeType date;
#ifdef CONTACTS
	DmOpenRef dbXTP;
#endif
	Boolean firstRun;
	globalVars* globals;
	
	TimSecondsToDateTime(seconds, &date);
	
	err = FtrGet(sysFtrCreator,	sysFtrNumROMVersion, &romVersion);
	//check PalmOS version
	if(romVersion<0x04000000)
	{
		EventType newEvent;
		FrmAlert(PalmOSVersionAlert);
		newEvent.eType = appStopEvent;
		EvtAddEventToQueue (&newEvent);	
		return 1;	
	}	
	
	if(!sublaunch)
	{
		AllocGlobals();
		globals = getGlobalsPtr();
		if(globals == NULL)
		{
			EventType newEvent;
			FrmAlert(alertLowMemory);
			newEvent.eType = appStopEvent;
			EvtAddEventToQueue (&newEvent);	
			return 1;	
		}
	}
#ifndef LIBDEBUG
	if(!sublaunch)
	{
		if(!LoadAddressXTLibraries(&globals->adxtLibRef, &globals->adxtLib2Ref))
		{
			return 1;
		}
	}
#endif
	
	/*if(SysLibLoad(PalmPhotoLibTypeID, PalmPhotoLibCreatorID, &globals->imageLibRef) != 0)
	{
		globals->imageLibRef = 0;
	}
	else
	{
		PalmPhotoLibOpen(globals->imageLibRef);
	}*/
	
//	if(SysLibLoad(tonesLibType, tonesLibCreator, &globals->tonesLibRef) != 0)
	{
		globals->tonesLibRef = 0;
	}
//	else
//	{
//		TonesLibOpen(globals->tonesLibRef);
		
//	}
		
#ifdef CONTACTS
	if((DmFindDatabase(0, CONTACTS_DBNAME)==0))
	{
		EventType newEvent;
		FrmAlert(alertIntendedForContacts);
		newEvent.eType = appStopEvent;
		EvtAddEventToQueue (&newEvent);	
		return 1;		
	}
#else
	if((DmFindDatabase(0, CONTACTS_DBNAME)!=0))
	{
		EventType newEvent;
		FrmAlert(alertIntendedForNonContacts);
		newEvent.eType = appStopEvent;
		EvtAddEventToQueue (&newEvent);	
		return 1;		
	}
#endif
	globals->gBListYear=date.year;
	globals->gBListMonth=date.month;	

	if(FtrGet (sysFtrCreator, sysFtrNumFiveWayNavVersion, &ret)==0)
	{
		if(ret == 1)
			globals->gFiveWay = true;
	}
	
	if(FtrGet (navFtrCreator, navFtrVersion, &ret)==0)
	{
		globals->gFiveWay = true;
	}

	if (FtrGet (hsFtrCreator, hsFtrIDNavigationSupported, &ret) == 0)
	{
		globals->gNavigation = true;
	}
	else
	{
		globals->gNavigation = false;
	}

	//globals->gKyocera = IsKyocera();
	
	FtrGet(sysFtrCreator,sysFtrNumOEMDeviceID, &id);
	
	switch(id)
	{
		case kPalmOneDeviceIDTungstenT5:
		case kPalmDeviceIDTX:
			globals->gDeviceType = type_T5orTx;
			break;
		case kPalmOneDeviceIDTreo600:
		case kPalmOneDeviceIDTreo600Sim:
			globals->gDeviceType = type_oldTreo;
			break;
		case kPalmOneDeviceIDTreo650:
		case kPalmOneDeviceIDTreo650Sim:
			globals->gDeviceType = type_treo650;
			break;
		case kPalmOneDeviceIDVentura:
			globals->gDeviceType = type_treo700;
			break;
		case kPalmOneDeviceIDLowrider:
			globals->gDeviceType = type_treo680;
			break;		
		case kPalmOneDeviceIDGandalf:
		case kPalmOneDeviceIDGnome:
			globals->gDeviceType = type_centro;
			break;
		case kPalmOneDeviceIDTorino:
			globals->gDeviceType = type_treo755;
			break;		
	}
	
	globals->gDeviceFlags.bits.treo = 1;
	switch(globals->gDeviceType)
	{
		case type_treo755:
		case type_centro:
		case type_treo680:
		case type_treo700:
			globals->gDeviceFlags.bits.treoWithSendKeys = 1;
			break;			
		case type_treo650:
		case type_oldTreo:
			globals->gDeviceFlags.bits.treoWithPhoneKeyOnly = 1;
			break;		
		default:
			globals->gDeviceFlags.bits.treo = 0;
			break;
	}		
	
	//		PrefSetPreference(prefHard1CharAppCreator, 'HsPh');
	if(globals->gDeviceFlags.bits.treo)
	{
		globals->gFiveWay = true;
		if(globals->gDeviceFlags.bits.treoWithPhoneKeyOnly)
		{
			globals->gDialerCreatorID = PrefGetPreference(prefHard1CharAppCreator);
			PrefSetPreference(prefHard1CharAppCreator, CREATORID);
		}
	}
	else 
	{
		if(globals->gDeviceType == type_T5orTx)
		{
			globals->PrevCreatorID=PrefGetPreference(prefHard3CharAppCreator);
			PrefSetPreference(prefHard3CharAppCreator, globals->PrevCreatorID);	
		}
		else
		{
			globals->PrevCreatorID=PrefGetPreference(prefHard2CharAppCreator);
			PrefSetPreference(prefHard2CharAppCreator, globals->PrevCreatorID);
		}
	}
	
	
	firstRun = IsFirstRun();
	
	if(firstRun)
	{
		AddrBtnPreferenceType btnPrefs;
		AddrSearchPreferenceType searchPrefs;
		AddrRecentPreferenceType recentPrefs;
		AddrExt3PreferenceType ext3Prefs;
		UInt16 prefsSize=0;
		Boolean map = false;
		UInt32 id;
		FtrGet(sysFtrCreator,sysFtrNumOEMDeviceID, &id);
						
		if(globals->gDeviceFlags.bits.treo)
		{
			if(FrmCustomAlert(WelcomeAlert, "Phone Favorites button", 0, 0) == 0)
			{
				DmOpenRef favs = favorites_open(false);
				if(favs)
				{
					favorites_write(favs, 0, "Address XT", CREATORID,
									0);
					DmCloseDatabase(favs);
				}
			}
		}
		else
		{
			if(FrmCustomAlert(WelcomeAlert, "hardware button", 0, 0) == 0)
			{
				if(id=='TnT5' || id == 'D050')
				{
					PrefSetPreference(prefHard3CharAppCreator, CREATORID);	
				}
				else 
				{
					PrefSetPreference(prefHard2CharAppCreator, CREATORID);
				}
			}
		}
		
		if(globals->gFiveWay)
		{
			if(globals->gDeviceFlags.bits.treo)
				btnPrefs.FiveWayUpDown = FIVEWAYRECORD;
			else
				btnPrefs.FiveWayUpDown = FIVEWAYPAGE;
			prefsSize = sizeof(AddrBtnPreferenceType);
			PrefSetAppPreferences (CREATORID, addrPrefBtnID, 1, &btnPrefs,
			   prefsSize, true);
			searchPrefs.OneHandedSearch = 1;
			searchPrefs.AdvancedFind = false;
			searchPrefs.reserved2 = 0;  
			searchPrefs.reserved3 = 0;  
			searchPrefs.reserved4 = 0;  
			searchPrefs.reserved5 = 0;  
			searchPrefs.reserved6 = 0;  
			searchPrefs.reserved7 = 0;  
			searchPrefs.reserved8 = 0;  
			prefsSize = sizeof(AddrSearchPreferenceType);
			PrefSetAppPreferences (CREATORID, addrPrefSearchID, 1, &searchPrefs,
			   prefsSize, true);
			recentPrefs.Enabled = 1;
			recentPrefs.Number = 5;
			recentPrefs.ShowAll = false;
			recentPrefs.reserved2 = 0;
			recentPrefs.reserved3 = 0;
			recentPrefs.reserved4 = 0;			
		}
		prefsSize = sizeof(AddrRecentPreferenceType);
		PrefSetAppPreferences (CREATORID, addrPrefRcntID, 2, &recentPrefs,
		   prefsSize, true);
		ext3Prefs.rememberLastRecord = 1;
		ext3Prefs.currentRecord = noRecord;
		ext3Prefs.addrView = 0;	
		prefsSize = sizeof(AddrExt3PreferenceType);
		PrefSetAppPreferences (CREATORID, addrPrefExt3ID, addrPrefVersionNum, &ext3Prefs,
		   prefsSize, true);
	}
	//LogWriteNum("xt_log", "Benchmark", TimGetTicks() , "Ticks6");
	globals->gJogLeftRight=false;
	globals->gScrollMode=FIVEWAYPAGE;	

	//FindTomTom();

	SysNotifyRegister(0, DmFindDatabase(0, APP_DBNAME), sysNotifySyncFinishEvent , NULL, sysNotifyNormalPriority, &data);
	
	//LogWriteNum("xt_log", "Benchmark", TimGetTicks() , "Ticks7");
	
	{
		UInt32 height=0, width=0, depth=2;
		Boolean color;
		WinScreenMode(winScreenModeGetSupportsColor, 0, 0, &depth, &color);
		WinScreenMode(winScreenModeGetSupportedDepths,0, 0, &depth, &color);
		if(depth>>15)
			globals->gDepth=16;
		else if(depth>>7)
			globals->gDepth=8;
		else if(depth>>3)
			globals->gDepth=4;
		else if(depth>>1)
			globals->gDepth=2;
		WinScreenMode(winScreenModeSet, NULL, NULL, &(globals->gDepth), NULL);
	}
	
	CstHighResInit();
	dia_init();
	jpgInit();
	
		
	//get system colors
	LoadColors(globals->adxtLibRef);
//	LogWriteNum("xt_log", "Benchmark", TimGetTicks() , "Ticks8");
	
	if(globals->gScreen!=PALM_320x320)
	{
		UInt32 width, height;
		Boolean enableColor;
		
		WinScreenMode (winScreenModeGet, &width, &height, &(globals->gDepth), &enableColor);

	}		
	globals->RecentList=MemPtrNew(RECENT_MAX*sizeof(Char**));
	
	//init popup list of link types
	globals->LinkTypes = MemPtrNew(sizeof(Char**)*MAX_LINKTYPES_NUM);
	for(i = 0; i < MAX_LINKTYPES_NUM; i++)
	{
		globals->LinkTypes[i] = MemPtrNew(sizeof(Char)*MAX_LINKTYPE_STRLEN);
	}
	
	
	if(!sublaunch)
	{
		globals->gCompanyList.Size=0;
		globals->gCompanyList.Pointer=MemPtrNew(MAX_COMPANIES*sizeof(Char**));
		globals->gRingerList.Size=0;
		globals->gRingerList.Pointer=MemPtrNew(MAX_RINGERS*sizeof(Char**));
		//init ringer list
		globals->gRingerStruct = 0;
		globals->gRingerCount = 0;
		if(globals->tonesLibRef)
		{
			UInt16 i, count;
			Err err;
			err = TonesLibGetToneList(globals->tonesLibRef, 0, &count);
			if (err == errNone && count)
			{
				globals->gRingerStruct = MemPtrNew(count * sizeof(ToneItemType));
				globals->gRingerCount = count;
				TonesLibGetToneList(globals->tonesLibRef, &(globals->gRingerStruct), &count);
				for(i = 0; i < count; i++)
				{
					Char* tEntry = globals->gRingerStruct[i].name;
					if(tEntry[0]==0)
						continue;
					if(StrLen(tEntry)==0)
						continue;
					globals->gRingerList.Pointer[globals->gRingerList.Size]=MemPtrNew(StrLen(tEntry)+1);
					StrCopy(globals->gRingerList.Pointer[globals->gRingerList.Size], tEntry);
					globals->gRingerList.Size++;
				}
			}
		}
		
		globals->gAddress=MemPtrNew(255);
		globals->gCompany=MemPtrNew(255);
		globals->gCity=MemPtrNew(255);
		globals->gZipCode=MemPtrNew(255);
		globals->gCountry=MemPtrNew(255);
		globals->gState=MemPtrNew(255);
		globals->gNewCompany=MemPtrNew(255);
		globals->gDomain=MemPtrNew(255);
		globals->gNewEMail=MemPtrNew(255);
	}	
	
	// Determime if secret records should be shown.
	globals->PrivateRecordVisualStatus = (privateRecordViewEnum)PrefGetPreference(prefShowPrivateRecords);
	if(globals->startupType == startupLookup)
	{
		if(globals->PrivateRecordVisualStatus == maskPrivateRecords)
			globals->PrivateRecordVisualStatus = hidePrivateRecords;
	}
	
//	LogWriteNum("xt_log", "Benchmark", TimGetTicks() , "Ticks9");
	mode = (globals->PrivateRecordVisualStatus == hidePrivateRecords) ?
		dmModeReadWrite : (dmModeReadWrite | dmModeShowSecret);

	globals->AddrDB = NULL;
	
	{
		AddrImportPreferenceType prefs;
		Int16 prefsVersion;
		UInt16 prefsSize;
		Boolean imported = false;
		prefsSize = sizeof (AddrImportPreferenceType);
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefImportID, &prefs, &prefsSize, true);
		
		if (prefsVersion > noPreferenceFound)
		{
			imported=prefs.idsImported;
		}	
	
		if(!imported)
		{
			ImportIDs();
			prefs.idsImported = true;
			PrefSetAppPreferences (CREATORID, addrPrefImportID, addrPrefVersionNum, &prefs,
						   prefsSize, true);	
		}
	}
		
#ifdef CONTACTS
	globals->AddrDB = DmOpenDatabase(0, DmFindDatabase(0, CONTACTS_DBNAME), mode);  
#else
	err = AddrDBGetDatabase(globals->adxtLibRef, &(globals->AddrDB), mode);
	if (err)
		return err;
#endif
//	LogWriteNum("xt_log", "Benchmark", TimGetTicks() , "Ticks10");
	recentDbId=DmFindDatabase(0, RECENTDB_DBNAME);
	if(recentDbId==0)
	{
		DmCreateDatabase(0, RECENTDB_DBNAME, CREATORID, 'DATA', false);
		recentDbId=DmFindDatabase(0, RECENTDB_DBNAME);
		if(recentDbId==0)
			return 1;
		DmDatabaseInfo(0, recentDbId, NULL, &attr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		attr|=dmHdrAttrBackup;
		DmSetDatabaseInfo(0, recentDbId, NULL, &attr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	globals->RecentDB=DmOpenDatabase(0, recentDbId, dmModeReadWrite); 
	
	
	appInfoPtr = univAddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	ErrFatalDisplayIf(appInfoPtr == NULL, "Missing app info block");
#ifdef CONTACTS
	ToolsInitPhoneLabelLetters(globals->adxtLibRef, appInfoPtr, globals->PhoneLabelLetters);
	MemPtrUnlock(appInfoPtr);
#else
	// Update the database to look and behave properly for the given country.
	if (appInfoPtr->country != PrefGetPreference(prefCountry))
		AddrDBChangeCountry(globals->adxtLibRef, appInfoPtr);
	ToolsInitPhoneLabelLetters(globals->adxtLibRef, appInfoPtr, globals->PhoneLabelLetters);
#endif
	

	CstOpenOrCreateDB(globals->adxtLibRef, LINKSDB_DBNAME, &(globals->linksDB));
	DeleteOrphaned(globals->adxtLibRef, globals->linksDB);

	CleanRecentDB(globals->adxtLibRef);
	
	//check if we should import birthdays from Contacts DB
/*#ifdef CONTACTS
	if(!sublaunch)
	{
		ImportBirthdays();
		CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &dbXTP);	
		UpdateAlarms(globals->adxtLibRef, dbXTP);
		DmCloseDatabase(dbXTP);
	}
#endif*/
	
	globals->SortByCompany = GetSortByCompany(globals->adxtLibRef);
	
	// Load the application preferences and fix them up if need be.	(BGT)
	
//	LogWriteNum("xt_log", "Benchmark", TimGetTicks() , "Ticks11");
#ifndef CONTACTS
	PrefsLoad(appInfoPtr);							
	PrefsExtLoad();
#else
	DmCloseDatabase(globals->AddrDB);
	AddrDBGetDatabase(globals->adxtLibRef, &(globals->AddrDB), mode);
	addrAppInfoPtr = (AddrAppInfoPtr) AddrDBAppInfoGetPtr(globals->adxtLibRef, globals->AddrDB);
	
	PrefsLoad(addrAppInfoPtr);	
	DmCloseDatabase(globals->AddrDB);
	
	globals->AddrDB = DmOpenDatabase(0, DmFindDatabase(0, CONTACTS_DBNAME), mode);  
	PrefsExtLoad();
#endif

	if(DmQueryRecord(globals->AddrDB, globals->gLastRecord)==NULL)
	{
		globals->gLastRecord=noRecord;
		globals->gAddrView=false;
	}

	if(globals->gLastRecord < DmNumRecords(globals->AddrDB))
	{
		if(DmQueryRecord(globals->AddrDB, globals->gLastRecord)==NULL)
		{
			globals->gLastRecord=noRecord;
			globals->gAddrView=false;
		}
	}
	
	if(globals->FiveWayUpDown==FIVEWAYCONTACTS)
	{
		if(globals->gLastRecord != noRecord)
			globals->gScrollMode=FIVEWAYRECORD;
		else
			globals->gScrollMode=FIVEWAYPAGE;		
	}
	if(globals->gLastRecord!=noRecord)
	{
		globals->ListViewSelectThisRecord=globals->gLastRecord;
	}
	// Initialize the default auto-fill databases
	
	if(!sublaunch)
	{
		AutoFillInitDB(titleDBType, sysFileCAddress, titleDBName, titleAFInitStr);
		AutoFillInitDB(companyDBType, sysFileCAddress, companyDBName, companyAFInitStr);
		AutoFillInitDB(cityDBType, sysFileCAddress, cityDBName, cityAFInitStr);
		AutoFillInitDB(stateDBType, sysFileCAddress, stateDBName, stateAFInitStr);
		AutoFillInitDB(countryDBType, sysFileCAddress, countryDBName, countryAFInitStr);
	}
	// Start watching the button pressed to get into this app.  If it's held down
	// long enough then we need to send the business card.
	globals->TickAppButtonPushed = TimGetTicks();

	// Mask off the key to avoid repeat keys causing clicking sounds
	KeySetMask(~KeyCurrentState());
//	LogWriteNum("xt_log", "Benchmark", TimGetTicks() , "Ticks12");
	
	return (err);
}

void PrvAppStopEx(Boolean sublaunch)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 lIndex;
	Err error;
	UInt32 *flagP;
	RGBColorType color;
	UInt16 i;
	Int16 j;
	UInt32 id;
	UInt16 prefsSize = sizeof(UInt32);
#ifdef CONTACTS
	UInt32 modTime;	
#endif
	UnloadSkin();
	// Write the preferences / saved-state information.
	PrefsSave();
	PrefsExtSave();
	//restore hardware map
	FtrGet(sysFtrCreator,sysFtrNumOEMDeviceID, &id);
	if(globals->gDeviceType == type_T5orTx)
	{
		PrefSetPreference(prefHard3CharAppCreator, globals->PrevCreatorID);	
	}
	else if(globals->gDeviceFlags.bits.treo)
	{
		if(globals->gDeviceFlags.bits.treoWithPhoneKeyOnly)
		{
			PrefSetPreference(prefHard1CharAppCreator, globals->gDialerCreatorID);
		}
		else
		{
			HsPrefSet(hsPrefHard11CharAppCreator, &globals->gDialerCreatorID, sizeof(UInt32));
		}		
	}
	else
	{
		PrefSetPreference(prefHard2CharAppCreator, globals->PrevCreatorID);
	}
	
	//free mem allocated in company
	if(!sublaunch)
	{
		MemPtrFree(globals->gAddress);
		MemPtrFree(globals->gCompany);
		MemPtrFree(globals->gCity);
		MemPtrFree(globals->gZipCode);
		MemPtrFree(globals->gCountry);
		MemPtrFree(globals->gState);
		MemPtrFree(globals->gNewCompany);
		MemPtrFree(globals->gDomain);
		MemPtrFree(globals->gNewEMail);
		if (globals->gRingerList.Size>0)
		{
			for(lIndex=0;lIndex<globals->gRingerList.Size;lIndex++)
				MemPtrFree(globals->gRingerList.Pointer[lIndex]);
		}
		if(globals->gRingerStruct)
			MemPtrFree(globals->gRingerStruct);
		
		MemPtrFree(globals->gRingerList.Pointer);	
		MemPtrFree(globals->RecentList);
		if (globals->gCompanyList.Size>0)
		{
			for(lIndex=0;lIndex<globals->gCompanyList.Size;lIndex++)
				MemPtrFree(globals->gCompanyList.Pointer[lIndex]);
		}
		MemPtrFree(globals->gCompanyList.Pointer);		
	}
	//free mem for link types popup
	for(i = 0; i < MAX_LINKTYPES_NUM; i++)
	{
		MemPtrFree(globals->LinkTypes[i]);
	}
	MemPtrFree(globals->LinkTypes);

	if(globals->tonesLibRef)
		TonesLibClose(globals->tonesLibRef);
	jpgClose();
	
	
	for(i = 0; i < connectoptions_num; i++)
	{
		for(j = 0; j < globals->ContactSettings[i].num; j++)
		{
			if(globals->ContactSettings[i].strings[j])
				MemPtrFree(globals->ContactSettings[i].strings[j]);
		}
		if(globals->ContactSettings[i].strings)
			MemPtrFree(globals->ContactSettings[i].strings);
		if(globals->ContactSettings[i].crIDs)
			MemPtrFree(globals->ContactSettings[i].crIDs);
	}

	
	/*if(globals->gScreen==CLIE_320x320)
	{
		//reset screen settings
		error = HRWinScreenMode (globals->refNum, winScreenModeSetToDefaults, NULL, NULL, NULL, NULL );
		error = HRClose(globals->refNum);
	}*/
	

	// Send a frmSave event to all the open forms.
	
	// Close all the open forms.
	if(!sublaunch)
	{
		FrmSaveAllForms ();
		FrmCloseAllForms ();
	}
	
	DmCloseDatabase (globals->linksDB);
	DmCloseDatabase (globals->AddrDB);
#ifdef CONTACTS
	if(DmDatabaseInfo(0, DmFindDatabase(0, CONTACTS_DBNAME), 0, 0, 0, 0, &modTime, 0, 0, 0, 0, 0, 0) == errNone)
	{
		PrefSetAppPreferences (CREATORID, addrPrefImportBirthdayID, addrPrefVersionNum, &modTime,
					   prefsSize, true);
	}
#endif
	
	DmCloseDatabase	(globals->RecentDB);
	
	
	//unregister notification
	SysNotifyUnregister(0, DmFindDatabase(0, APP_DBNAME), sysNotifyDisplayChangeEvent, sysNotifyNormalPriority);
	SysNotifyUnregister(0, DmFindDatabase(0, APP_DBNAME), sysNotifyDisplayResizedEvent, sysNotifyNormalPriority);
		
	globals->gColorBack=globals->gBackground;
	globals->gColorText=globals->gForeground;
	WinIndexToRGB(globals->gBackground, &color);
	UIColorSetTableEntry(UIFieldBackground, &color);
	WinIndexToRGB(globals->gFill, &color);
	UIColorSetTableEntry(UIFormFill, &color);
	WinIndexToRGB(globals->gAlert,&color);
	UIColorSetTableEntry(UIAlertFill, &color);
	WinIndexToRGB(globals->gForeground, &color);
	UIColorSetTableEntry(UIObjectForeground, &color);
	WinIndexToRGB(globals->gDialogFill, &color);
	UIColorSetTableEntry(UIDialogFill, &color);
	WinIndexToRGB(globals->gObjectFill, &color);
	UIColorSetTableEntry(UIObjectFill, &color);
	

	if(!sublaunch)
	{
#ifndef LIBDEBUG
		SysLibRemove(globals->adxtLibRef);
		SysLibRemove(globals->adxtLib2Ref);
#endif	
		DeleteGlobals();
	}	
	
#ifdef DEBUG
	LogWrite("xt_log", "main", "stop");
#endif
}

Err PrvAppStart(void)
{
	return PrvAppStartEx(false);
}

void PrvAppStop(void)
{
	PrvAppStopEx(false);
}


void PrvAppEventLoop (void)
{
	UInt16 error;
	EventType event;
	globalVars* globals = getGlobalsPtr();
	do
	{
		EvtGetEvent (&event, (globals->TickAppButtonPushed == 0) ? evtWaitForever : 2);
		if (! SysHandleEvent (&event) || (event.eType == keyDownEvent && event.data.keyDown.chr == vchrHard11))

			if (! PrvAppHandleKeyDown (&event))

				if (! MenuHandleEvent (0, &event, &error))

					if (! PrvAppHandleEvent (&event))

						FrmDispatchEvent (&event);
	}
	while (event.eType != appStopEvent);
}

Boolean PrvAppLookupEventLoop ()
{
	EventType event;
	
	while (true)
	{
		EvtGetEvent (&event, evtWaitForever);

		// Cancel if something is going to switch apps.
		if ( ((event.eType == keyDownEvent)
			 &&	(!TxtCharIsHardKey(	event.data.keyDown.modifiers, event.data.keyDown.chr))
			 &&	(EvtKeydownIsVirtual(&event))
			 &&	(event.data.keyDown.chr == vchrFind))
			 )
		{
			EvtAddEventToQueue(&event);
			return false;
		}
		
		if (event.eType == appStopEvent)
		{
			EvtAddEventToQueue(&event);
			return true;
		}
		
		if(event.eType == LOOKUP_END_EVENT)
		{
			return true;
		}

		if(SysHandleEvent(&event))
			continue;
		
		if (! PrvAppHandleEvent (&event))
			FrmDispatchEvent (&event);
	}
	return true;
}


Boolean PrvAppHandleKeyDown (EventType * event)
{
	globalVars* globals = getGlobalsPtr();
	// Check if a button being held down is released
	
	if (globals->TickAppButtonPushed != 0)
	{
		// This is the case when the button is let up
		if ((KeyCurrentState() & (keyBitHard1 | keyBitHard2 | keyBitHard3 | keyBitHard4)) == 0)
		{
			if (globals->BusinessCardSentForThisButtonPress)
			{
				globals->BusinessCardSentForThisButtonPress = false;

				globals->TickAppButtonPushed = 0;

				// Allow the masked off key to now send keyDownEvents.
				KeySetMask(keyBitsAll);
			}
			else if (event->eType == nilEvent)
			{
				// Send the keyDownEvent to the app.  It was stripped out
				// before but now it can be sent over the nullEvent.  It
				// may be nullChr from when the app was launched.  In that case
				// we don't need to send the app's key because the work expected,
				// which was switching to this app, has already been done.
				if (globals->AppButtonPushed != nullChr)
				{
					event->eType = keyDownEvent;
					event->data.keyDown.chr = globals->AppButtonPushed;
					event->data.keyDown.modifiers = globals->AppButtonPushedModifiers;
				}

				globals->TickAppButtonPushed = 0;

				// Allow the masked off key to now send keyDownEvents.
				KeySetMask(keyBitsAll);
			}
		}
		// This is the case when the button is depresed long enough to send the business card
		else if (globals->TickAppButtonPushed + AppButtonPushTimeout <= TimGetTicks() &&
				 !globals->BusinessCardSentForThisButtonPress)
		{
			globals->BusinessCardSentForThisButtonPress = true;
			ToolsAddrBeamBusinessCard(globals->AddrDB);
		}
	}


	else if (event->eType == keyDownEvent)
	{
		if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr) &&
			!(event->data.keyDown.modifiers & autoRepeatKeyMask))
		{
			if(!globals->gNavigation || (globals->gNavigation && IsFiveWayNavEvent(event) && ToolsNavKeyPressed(globals->adxtLibRef, event, Select)))
			{
#ifdef DEBUG
				LogWrite("xt_log", "syshandle", "keydown");
#endif
				// Remember which hard key is mapped to the Address Book
				// because it may need to be sent later.
				globals->AppButtonPushed = event->data.keyDown.chr;
				globals->AppButtonPushedModifiers = event->data.keyDown.modifiers;

				globals->TickAppButtonPushed = TimGetTicks();

				// Mask off the key to avoid repeat keys causing clicking sounds
				KeySetMask(~KeyCurrentState());

				// Don't process the key
				return true;
			}
		}
	}

	return false;
}

void RestoreDialer()
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gDeviceFlags.bits.treoWithPhoneKeyOnly)
		PrefSetPreference(prefHard1CharAppCreator, globals->gDialerCreatorID);
}

void SetDialer()
{
	globalVars* globals = getGlobalsPtr();
	if(globals->gDeviceFlags.bits.treoWithPhoneKeyOnly)
		PrefSetPreference(prefHard1CharAppCreator, CREATORID);
}

