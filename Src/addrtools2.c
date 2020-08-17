#include "AddrTools2.h"
#include "AddrXTDB.h"
#include "AddrPrefs.h"
#include "AddressTransfer.h"
#include "AddressTransfer2.h"
#include "ColorOptions.h"
#include "Mapopolis.h"
#include "ContactsDB2.h"
#include "dia.h"
#include "AddrFont.h"
#include "syslog.h"
#include "Address.h"

/*Int16 LoadAddress(UInt16 recordIndex, AddressStruct ** list, Boolean countOnly)
{
	globalVars* globals = getGlobalsPtr();
	MemHandle recordH;
#ifndef CONTACTS
	AddrDBRecordType record;
#else
	P1ContactsDBRecordType contact;
#endif
	UInt16 addrCount=0;
	
	if(recordIndex==noRecord)
		return 0;	
	
#ifdef CONTACTS
	PrvP1ContactsDBGetRecord(globals->adxtLibRef, globals->AddrDB, recordIndex, &contact, &recordH);
	if(contact.fields[P1Contactsaddress]!=0)
	{
		if(contact.fields[P1Contactsaddress][0]!=0)
		{
			if(!countOnly)
			{
				list[addrCount] = MemPtrNew(sizeof(AddressStruct));

				list[addrCount]->address = NULL;
				list[addrCount]->city = NULL;
				list[addrCount]->state = NULL;
				list[addrCount]->zip = NULL;

				list[addrCount]->address=MemPtrNew((StrLen(contact.fields[P1Contactsaddress])+1)*sizeof(Char));
				StrCopy(list[addrCount]->address, contact.fields[P1Contactsaddress]);
				if(contact.fields[P1Contactscity]!=0)
				{
					if(contact.fields[P1Contactscity][0]!=0)
					{
						list[addrCount]->city=MemPtrNew((StrLen(contact.fields[P1Contactscity])+1)*sizeof(Char));
						StrCopy(list[addrCount]->city, contact.fields[P1Contactscity]);	
					}
				}
				if(contact.fields[P1Contactsstate]!=0)
				{
					if(contact.fields[P1Contactsstate][0]!=0)
					{
						list[addrCount]->state=MemPtrNew((StrLen(contact.fields[P1Contactsstate])+1)*sizeof(Char));
						StrCopy(list[addrCount]->state, contact.fields[P1Contactsstate]);	
					}
				}
				if(contact.fields[P1ContactszipCode]!=0)
				{
					if(contact.fields[P1ContactszipCode][0]!=0)
					{
						list[addrCount]->zip=MemPtrNew((StrLen(contact.fields[P1ContactszipCode])+1)*sizeof(Char));
						StrCopy(list[addrCount]->zip, contact.fields[P1ContactszipCode]);	
					}
				}
			}
			addrCount++;
		}
	}
	if(contact.fields[P1Contactsaddress2]!=0)
	{
		if(contact.fields[P1Contactsaddress2][0]!=0)
		{
			if(!countOnly)
			{
				list[addrCount] = MemPtrNew(sizeof(AddressStruct));

				list[addrCount]->address = NULL;
				list[addrCount]->city = NULL;
				list[addrCount]->state = NULL;
				list[addrCount]->zip = NULL;

				list[addrCount]->address=MemPtrNew((StrLen(contact.fields[P1Contactsaddress2])+1)*sizeof(Char));
				StrCopy(list[addrCount]->address, contact.fields[P1Contactsaddress2]);
				if(contact.fields[P1Contactscity2]!=0)
				{
					if(contact.fields[P1Contactscity2][0]!=0)
					{
						list[addrCount]->city=MemPtrNew((StrLen(contact.fields[P1Contactscity2])+1)*sizeof(Char));
						StrCopy(list[addrCount]->city, contact.fields[P1Contactscity2]);	
					}
				}
				if(contact.fields[P1Contactsstate2]!=0)
				{
					if(contact.fields[P1Contactsstate2][0]!=0)
					{
						list[addrCount]->state=MemPtrNew((StrLen(contact.fields[P1Contactsstate2])+1)*sizeof(Char));
						StrCopy(list[addrCount]->state, contact.fields[P1Contactsstate2]);	
					}
				}
				if(contact.fields[P1ContactszipCode2]!=0)
				{
					if(contact.fields[P1ContactszipCode2][0]!=0)
					{
						list[addrCount]->zip=MemPtrNew((StrLen(contact.fields[P1ContactszipCode2])+1)*sizeof(Char));
						StrCopy(list[addrCount]->zip, contact.fields[P1ContactszipCode2]);	
					}
				}
			}
			addrCount++;
		}
	}
	if(contact.fields[P1Contactsaddress3]!=0)
	{
		if(contact.fields[P1Contactsaddress3][0]!=0)
		{
			if(!countOnly)
			{
				list[addrCount] = MemPtrNew(sizeof(AddressStruct));

				list[addrCount]->address = NULL;
				list[addrCount]->city = NULL;
				list[addrCount]->state = NULL;
				list[addrCount]->zip = NULL;

				list[addrCount]->address=MemPtrNew((StrLen(contact.fields[P1Contactsaddress3])+1)*sizeof(Char));
				StrCopy(list[addrCount]->address, contact.fields[P1Contactsaddress3]);
				if(contact.fields[P1Contactscity3]!=0)
				{
					if(contact.fields[P1Contactscity3][0]!=0)
					{
						list[addrCount]->city=MemPtrNew((StrLen(contact.fields[P1Contactscity3])+1)*sizeof(Char));
						StrCopy(list[addrCount]->city, contact.fields[P1Contactscity3]);	
					}
				}
				if(contact.fields[P1Contactsstate3]!=0)
				{
					if(contact.fields[P1Contactsstate3][0]!=0)
					{
						list[addrCount]->state=MemPtrNew((StrLen(contact.fields[P1Contactsstate3])+1)*sizeof(Char));
						StrCopy(list[addrCount]->state, contact.fields[P1Contactsstate3]);	
					}
				}
				if(contact.fields[P1ContactszipCode3]!=0)
				{
					if(contact.fields[P1ContactszipCode3][0]!=0)
					{
						list[addrCount]->zip=MemPtrNew((StrLen(contact.fields[P1ContactszipCode3])+1)*sizeof(Char));
						StrCopy(list[addrCount]->zip, contact.fields[P1ContactszipCode3]);	
					}
				}
			}
			addrCount++;
		}
	}
	if(contact.fields[P1Contactscustom1]!=0 && MAPOPOLIS)
	{
		if(contact.fields[P1Contactscustom1][0]!=0)
		{
			if(!countOnly)
			{
				list[addrCount] = MemPtrNew(sizeof(AddressStruct));

				list[addrCount]->address = NULL;
				list[addrCount]->city = NULL;
				list[addrCount]->state = NULL;
				list[addrCount]->zip = NULL;

				list[addrCount]->address=MemPtrNew((StrLen(contact.fields[P1Contactscustom1])+1)*sizeof(Char));
				StrCopy(list[addrCount]->address, contact.fields[P1Contactscustom1]);
			}
			addrCount++;
		}
	}
#else
	AddrDBGetRecord(globals->adxtLibRef, globals->AddrDB, recordIndex, &record, &recordH);
	if(record.fields[address]!=0)
	{
		if(record.fields[address][0]!=0)
		{
			if(!countOnly)
			{
				list[addrCount] = MemPtrNew(sizeof(AddressStruct));
				
				list[addrCount]->address = NULL;
				list[addrCount]->city = NULL;
				list[addrCount]->state = NULL;
				list[addrCount]->zip = NULL;

				list[addrCount]->address=MemPtrNew((StrLen(record.fields[address])+1)*sizeof(Char));
				StrCopy(list[addrCount]->address, record.fields[address]);
				if(record.fields[city]!=0)
				{
					if(record.fields[city][0]!=0)
					{
						list[addrCount]->city=MemPtrNew((StrLen(record.fields[city])+1)*sizeof(Char));
						StrCopy(list[addrCount]->city, record.fields[city]);	
					}
				}
				if(record.fields[state]!=0)
				{
					if(record.fields[state][0]!=0)
					{
						list[addrCount]->state=MemPtrNew((StrLen(record.fields[state])+1)*sizeof(Char));
						StrCopy(list[addrCount]->state, record.fields[state]);	
					}
				}
				if(record.fields[zipCode]!=0)
				{
					if(record.fields[zipCode][0]!=0)
					{
						list[addrCount]->zip=MemPtrNew((StrLen(record.fields[zipCode])+1)*sizeof(Char));
						StrCopy(list[addrCount]->zip, record.fields[zipCode]);	
					}
				}
			}
			addrCount++;
		}
	}
	if(record.fields[custom1]!=0 && MAPOPOLIS)
	{
		if(record.fields[custom1][0]!=0)
		{
			if(!countOnly)
			{
				list[addrCount] = MemPtrNew(sizeof(AddressStruct));
				
				list[addrCount]->address = NULL;
				list[addrCount]->city = NULL;
				list[addrCount]->state = NULL;
				list[addrCount]->zip = NULL;

				list[addrCount]->address=MemPtrNew((StrLen(record.fields[custom1])+1)*sizeof(Char));
				StrCopy(list[addrCount]->address, record.fields[custom1]);
			}
			addrCount++;
		}
	}
#endif
	MemHandleUnlock(recordH);
		
	return addrCount;
}
*/
void StartPhone()
{
	globalVars* globals = getGlobalsPtr();
	DmSearchStateType srch; 
	UInt16 cardNo;
	UInt16 launchFlags=0;
	LocalID dbID;
	UInt32		*gotoInfoP;
	gotoInfoP = (UInt32*)MemPtrNew (sizeof(UInt32));
	MemPtrSetOwner(gotoInfoP, 0);
	
	
	if(DmGetNextDatabaseByTypeCreator(true, &srch, 'appl', globals->gDialerCreatorID, true, &cardNo, &dbID)!=errNone)
		return;
	SysUIAppSwitch(0, dbID, sysAppLaunchCmdNormalLaunch, gotoInfoP);
}

Boolean ConnectContact(Char* connect, UInt16 field)
{
	SysNotifyParamType param;
	HelperNotifyEventType details;
	HelperNotifyExecuteType execute;			
	HelperServiceEMailDetailsType emailDetails;
			
	AddrDialOptionsPreferenceType prefs;
	UInt16 prefsSize;
	Int16 prefsVersion;
	UInt32 crID = 0;
	Char subj[3] = "";
	Char msg[30] = "";
	Char name[10] = "";
	
	prefsSize = sizeof (AddrDialOptionsPreferenceType);
	prefsVersion =  PrefGetAppPreferences (CREATORID, addrPrefDialOptionsDataID, &prefs, &prefsSize, true);
	if(prefsVersion > noPreferenceFound)
	{
		crID = prefs.creatorID[field];
	}
#ifdef DEBUG
	LogWriteNum("xt_log", "e-mail", crID , "creatorID");
#endif	
	
#ifdef DEBUG
	LogWriteNum("xt_log", "default e-mail", crID , "creatorID");
#endif	

	param.notifyType = sysNotifyHelperEvent;
	param.broadcaster = CREATORID;
	param.notifyDetailsP = &details;
	param.handled = false;
				
	details.version = kHelperNotifyCurrentVersion;
	details.actionCode = kHelperNotifyActionCodeExecute;
	details.data.executeP = &execute;
	switch(field)
	{
		case connectoptions_email:
			execute.serviceClassID = kHelperServiceClassIDEMail;
			break;		
		case connectoptions_web:
			execute.serviceClassID = 'webH';
			break;		
		case connectoptions_im:
			execute.serviceClassID = 'imsH';
			break;		
		case connectoptions_map:
			execute.serviceClassID = 'mapH';
			break;		
	}
	execute.helperAppID = crID;
	execute.dataP = connect;
	execute.err = errNone;
	
	if(field == connectoptions_email)
	{
		execute.displayedName = name;
		emailDetails.version = 1;
		emailDetails.cc = subj;
		emailDetails.subject = subj;
		emailDetails.message = msg;
							
		execute.detailsP = &emailDetails;
	}
								
	SysNotifyBroadcast(&param);	
							
	return param.handled;
}

Boolean ConnectMap(Char* address, Char* city, Char* state, Char* zipcode, Char* country, UInt32 recid)
{
	SysNotifyParamType param;
	HelperNotifyEventType details;
	HelperNotifyExecuteType execute;			
	UInt16 prefsSize;
	Int16 prefsVersion;
	AddrDialOptionsPreferenceType prefs;
	UInt32 crID;
	prefsSize = sizeof (AddrDialOptionsPreferenceType);
	prefsVersion =  PrefGetAppPreferences (CREATORID, addrPrefDialOptionsDataID, &prefs, &prefsSize, true);
	if(prefsVersion > noPreferenceFound)
	{
		crID = prefs.creatorID[connectoptions_map];
	}
	
	//country = "US";
	if(crID == googleMapCreatorID)
	{
		UInt16 cardNo;
        LocalID dbID = 0;
        DmSearchStateType stateInfo;
        if (errNone == DmGetNextDatabaseByTypeCreator(true, &stateInfo,  
			sysFileTApplication, googleMapCreatorID, false, &cardNo, &dbID) && dbID)
        {
			Char* findStr;
			UInt16 length;
			GoToParamsType * cmdPBP = (GoToParamsType*)MemPtrNew(sizeof(GoToParamsType));
			length = StrLen(address);
			if(city)
				length += StrLen(city) + 1;
			if(state)
				length += StrLen(state) + 1;
			if(zipcode)
				length += StrLen(zipcode) + 1;
			if(country)
				length += StrLen(country) + 1;
			findStr = MemPtrNew(length*sizeof(Char));
			
			StrCopy(findStr, address);
			if(city)
			{
				StrCat(findStr, " ");
				StrCat(findStr, city);
			}
			if(state)
			{
				StrCat(findStr, " ");
				StrCat(findStr, state);
			}
			if(zipcode)
			{
				StrCat(findStr, " ");
				StrCat(findStr, zipcode);
			}
			if(country)
			{
				StrCat(findStr, " ");
				StrCat(findStr, country);
			}
			
			MemPtrSetOwner(cmdPBP, 0);
			MemSet(cmdPBP, sizeof(GoToParamsType), 0);
			cmdPBP->recordNum = googleMapFindLocation;     
			PrefSetAppPreferences(googleMapCreatorID, 
				googleMapPreferenceID, googleMapPreferenceVersion, findStr, StrLen(findStr), 
				false);
			MemPtrFree(findStr);
			SysUIAppSwitch(cardNo, dbID, sysAppLaunchCmdGoTo, cmdPBP);
			return true;
        }
        else
        	return false;
	}
	else
	{
		HelperServiceMapDetailsType mapDetails;
		mapDetails.version = 1;
		mapDetails.recid = recid;
		
		mapDetails.address = address;
		mapDetails.city = city;
		mapDetails.state = state;
		mapDetails.zipcode = zipcode;
		mapDetails.country = country;
		
		
		param.notifyType = sysNotifyHelperEvent;
		param.broadcaster = CREATORID;
		param.notifyDetailsP = &details;
		param.handled = false;
					
		details.version = kHelperNotifyCurrentVersion;
		details.actionCode = kHelperNotifyActionCodeExecute;
		details.data.executeP = &execute;
		execute.serviceClassID = 'mapH';
		execute.helperAppID = crID;
		execute.dataP = 0;
		execute.err = errNone;
		
		execute.detailsP = &mapDetails;

		SysNotifyBroadcast(&param);	
								
		return param.handled;
	}
}

Boolean ToolsVersaMailPresent()
{
	SysNotifyParamType param;
	HelperNotifyEventType details;
	HelperNotifyExecuteType execute;			
	HelperServiceEMailDetailsType emailDetails;
				
	Char subj[3] = "Hi";
	Char msg[30] = "This is a test Email";
	Char mail[15] = "john@doe.com";
	Char name[10] = "John Doe";
				
				
	param.notifyType = sysNotifyHelperEvent;
	param.broadcaster = CREATORID;
	param.notifyDetailsP = &details;
	param.handled = false;
				
	details.version = kHelperNotifyCurrentVersion;
	details.actionCode = kHelperNotifyActionCodeValidate;
	details.data.executeP = &execute;
	execute.serviceClassID = kHelperServiceClassIDEMail;
	execute.helperAppID = 0;
	execute.dataP = mail;
	execute.displayedName = name;
	execute.err = errNone;
	
	emailDetails.version = 1;
	emailDetails.cc = mail;
	emailDetails.subject = subj;
	emailDetails.message = msg;
							
	execute.detailsP = &emailDetails;
								
	SysNotifyBroadcast(&param);	
							

	return param.handled;
}

void CstGetField(DmOpenRef tDBRef, Char *tRes, UInt16 tRecord, UInt16 tField)
{
	MemHandle lH;
	Char *lP4;
	Char *lP2, lP[600];
	UInt16 lSize;
	UInt16 lCounter, lCounter2;
	UInt16 attr;
	DmRecordInfo(tDBRef, tRecord, &attr, NULL, NULL);
	if(attr & dmRecAttrDelete)
	{
		tRes[0]=0;
		return;
	}
	lH=DmQueryRecord(tDBRef, tRecord);
	lP4=(Char*)MemHandleLock(lH);
	lSize=MemHandleSize(lH);
	if(lSize==0)
	{
		tRes[0]=0;
		return;
	}
	MemSet(lP, 600, 0);
	for(lCounter=0;lCounter<lSize;lCounter++)
		lP[lCounter]=lP4[lCounter];
	MemHandleUnlock(lH);
	lCounter2=0;
	tRes[0]=0;
	for(lCounter=0;lCounter<lSize;lCounter++)
	{
		lP2=lP+lCounter;
		if(lCounter2==tField)
			break;
		if(*lP2==0)
			lCounter2++;
	}
	if(lCounter2==tField)
	{
		if(lP2!=0)
		{
			if(lP2[0]!=0)
			{
				StrCopy(tRes, lP2);
			}
		}
	}
	return;
}


void ImportIDs()
{
	globalVars* globals = getGlobalsPtr();
	DmOpenRef db=0, dbOld = 0;
	UInt16 dbSize;
#ifdef CONTACTS
	Err err;
	UInt16 index;
#endif
	UInt16 mode, attr;
	Char *lStrArray[4];
	UInt32 counter, counter2, counter3;
	LocalID lDBID;
	
	globals->PrivateRecordVisualStatus = (privateRecordViewEnum)PrefGetPreference(prefShowPrivateRecords);
	mode = (globals->PrivateRecordVisualStatus == hidePrivateRecords) ?
		dmModeReadWrite : (dmModeReadWrite | dmModeShowSecret);
	for(counter=0;counter<4;counter++)
	{
		lStrArray[counter]=MemPtrNew(255);	
	}
	
	lDBID = DmFindDatabase(0, ADDRXTDB_DBNAME_OLD);
	
	if(lDBID)
		dbOld = DmOpenDatabase(0, lDBID, dmModeReadWrite);

	if(dbOld)
	{
		dbSize = DmNumRecords(dbOld);
		if(dbSize != 0)
		{
			UInt32 id, seconds;
			UInt16 remind, passed;
			AddrXTDBRecordType rec;
			CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &db);
			if(db)
			{	
				for(counter2=0;counter2<dbSize;counter2++)
				{
					DmRecordInfo(dbOld, counter2, &attr, NULL, NULL);
					if(attr & dmRecAttrDelete )
						continue;
					for(counter3 = 0; counter3 < 4; counter3++)
					{
						CstGetField(dbOld, lStrArray[counter3], counter2, counter3);				
					}
					
					id = (UInt32)StrAToI(lStrArray[ADDRXTDB_ID]);
					seconds = (UInt32)StrAToI(lStrArray[ADDRXTDB_BDAY]);
					if(!StrCompare(lStrArray[ADDRXTDB_REMIND], REMIND_NONE_OLD))
						remind = REMIND_NONE;
					else
						remind = StrAToI(lStrArray[ADDRXTDB_REMIND]);
					passed = StrAToI(lStrArray[ADDRXTDB_PASSED]);
					
	#ifdef CONTACTS
					err = AddrDBGetDatabase(globals->adxtLibRef, &globals->AddrDB, mode);
					if(!err)
					{
						if(DmFindRecordByID(globals->AddrDB, id, &index) == 0)
						{
							DmCloseDatabase(globals->AddrDB);
							globals->AddrDB = DmOpenDatabase(0, DmFindDatabase(0, CONTACTS_DBNAME), mode);  
							if(globals->AddrDB != 0)
							{
								DmRecordInfo(globals->AddrDB, index, NULL, &id, NULL);
								DmCloseDatabase(globals->AddrDB);
							}
						}					
					}				
	#endif				
					rec.id = id;
					rec.bday = seconds;
					rec.remind = remind;
					rec.passed = passed;
					AddrXTDBNewRecord(globals->adxtLibRef, db, &rec);
				}
				DmCloseDatabase(db);
			}
				
		}
		DmCloseDatabase(dbOld);
	}

	lDBID = DmFindDatabase(0, RECENTDB_DBNAME_OLD);
	
	if(lDBID)
		dbOld = DmOpenDatabase(0, lDBID, dmModeReadWrite);

	if(dbOld)
	{
		dbSize = DmNumRecords(dbOld);
		if(dbSize != 0)
		{
			UInt32 id, seconds;
			RecentDBRecordType rec;
			CstOpenOrCreateDB(globals->adxtLibRef, RECENTDB_DBNAME, &db);
			if(db)
			{	
				for(counter2=0;counter2<dbSize;counter2++)
				{
					DmRecordInfo(dbOld, counter2, &attr, NULL, NULL);
					if(attr & dmRecAttrDelete )
						continue;
					for(counter3 = 0; counter3 < 2; counter3++)
					{
						CstGetField(dbOld, lStrArray[counter3], counter2, counter3);				
					}
					
					id = (UInt32)StrAToI(lStrArray[RECENTDB_ID]);
					seconds = (UInt32)StrAToI(lStrArray[RECENTDB_TIME]);
					
	#ifdef CONTACTS
					err = AddrDBGetDatabase(globals->adxtLibRef, &globals->AddrDB, mode);
					if(!err)
					{
						if(DmFindRecordByID(globals->AddrDB, id, &index) == 0)
						{
							DmCloseDatabase(globals->AddrDB);
							globals->AddrDB = DmOpenDatabase(0, DmFindDatabase(0, CONTACTS_DBNAME), mode);  
							if(globals->AddrDB != 0)
							{
								DmRecordInfo(globals->AddrDB, index, NULL, &id, NULL);
								DmCloseDatabase(globals->AddrDB);
							}
						}					
					}				
	#endif
					
					rec.id = id;
					rec.time = seconds;
					RecentDBNewRecord(globals->adxtLibRef, db, &rec);
				}
				DmCloseDatabase(db);
			}
				
		}
		DmCloseDatabase(dbOld);
	}
			
	CstOpenOrCreateDB(globals->adxtLibRef, LINKSDB_DBNAME, &db);
	dbSize = DmNumRecords(db);
	if(dbSize != 0)
	{
		LinkDBRecordPtr recPtr;
#ifdef CONTACTS
		LinkDBRecordType rec;
#endif
		UInt32 addrID, id;
		UInt32 linkID;
		UInt32 type;
		
		MemHandle mH;
		
		for(counter = 0; counter < dbSize; counter ++)
		{
			mH = DmQueryRecord(db, counter);
			recPtr = (LinkDBRecordPtr)MemHandleLock(mH);
			
			addrID = recPtr->addrID;
			linkID = recPtr->linkID;
			type = recPtr->type;
			
			MemHandleUnlock(mH);
			
			id = addrID;
#ifdef CONTACTS
			err = AddrDBGetDatabase(globals->adxtLibRef, &globals->AddrDB, mode);
			if(!err)
			{
				if(DmFindRecordByID(globals->AddrDB, id, &index) == 0)
				{
					DmCloseDatabase(globals->AddrDB);
					globals->AddrDB = DmOpenDatabase(0, DmFindDatabase(0, CONTACTS_DBNAME), mode);  
					if(globals->AddrDB != 0)
					{
						DmRecordInfo(globals->AddrDB, index, NULL, &id, NULL);
						addrID = id;
						DmCloseDatabase(globals->AddrDB);
					}
				}					
			}				
#endif
					
			if(recPtr->type == LINKTYPE_CONTACT)
			{
				id = linkID;
#ifdef CONTACTS
				err = AddrDBGetDatabase(globals->adxtLibRef, &globals->AddrDB, mode);
				if(!err)
				{
					if(DmFindRecordByID(globals->AddrDB, id, &index) == 0)
					{
						DmCloseDatabase(globals->AddrDB);
						globals->AddrDB = DmOpenDatabase(0, DmFindDatabase(0, CONTACTS_DBNAME), mode);  
						if(globals->AddrDB != 0)
						{
							DmRecordInfo(globals->AddrDB, index, NULL, &id, NULL);
							linkID = id;
							DmCloseDatabase(globals->AddrDB);
						}
					}					
				}	
#endif
			}
			
#ifdef CONTACTS
			//change LinksDB record
			rec.addrID = addrID;
			rec.linkID = linkID;
			rec.type = type;
			LinkDBChangeRecord(globals->adxtLibRef, db, &rec, counter);
#endif
		}		
	}
	DmCloseDatabase(db);
	
	for(counter=0;counter<4;counter++)
	{
		MemPtrFree(lStrArray[counter]);
	}
}

void ImportBirthdays()
{
	globalVars* globals = getGlobalsPtr();
	Boolean found;
	UInt8 remindDays=0;
	UInt32 id;
	DmOpenRef addrXTDB=0;
	UInt16 attr;
	UInt16 addrXTDBSize;
	UInt32 contactsDBSize=0, counter, counter2, seconds;
	DateTimeType date;
	LocalID localID;	
	P1ContactsDBRecordType contact;
	MemHandle mH;
	DateType bDay;
	UInt8 adt = 0;
	
	UInt32 savedModTime = 0, modTime = 0;
	Int16 prefsVersion;
	UInt16 prefsSize;
				
	CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &addrXTDB);
	
	if(addrXTDB == 0)
		return;
	
	addrXTDBSize = DmNumRecords(addrXTDB);
			
	contactsDBSize = DmNumRecords(globals->AddrDB);
	if(contactsDBSize==0)
	{
		DmCloseDatabase(addrXTDB);
		return;
	}
	if(globals->AddrDB==0)
	{
		DmCloseDatabase(addrXTDB);
		return;
	}		
	
	prefsSize = sizeof (UInt32);
	prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefImportBirthdayID, &savedModTime, &prefsSize, true);
	
	if (prefsVersion <= noPreferenceFound)
	{
		savedModTime = 0;
	}	
	
	localID = DmFindDatabase(0, CONTACTS_DBNAME);
	if(DmDatabaseInfo(0, localID, 0, 0, 0, 0, &modTime, 0, 0, 0, 0, 0, 0) == errNone)
	{
		if(savedModTime >= modTime)
		{
		//	DmCloseDatabase(addrXTDB);
		//	return;
		}
	}	
	
	for(counter2=0;counter2<addrXTDBSize;counter2++)
	{
		AddrXTDBRecordType rec;
		MemHandle mH;
		UInt32 passed;
								
		DmRecordInfo(addrXTDB, counter2, &attr, NULL, NULL);
		if(attr & dmRecAttrDelete )
			continue;
		
		if(AddrXTDBGetRecord(globals->adxtLibRef, addrXTDB, counter2, &rec, &mH) != 0)
			continue;
		
		passed = rec.passed;
		
		MemHandleUnlock(mH);
					
		if(passed == 0)
		{
			DmRemoveRecord(addrXTDB, counter2);
			addrXTDBSize = DmNumRecords(addrXTDB);
			counter2 = 0;			
		}
	}
	for(counter=0;counter<contactsDBSize;counter++)
	{
		PrvP1ContactsDBGetRecordBDOnly (globals->AddrDB, counter, &contact, &mH);
		
		if(mH == 0)
			continue;
		
		remindDays=ALARM_NONE;//impossible value;
		
		if(contact.birthdayInfo.birthdayDate.day == 0 && contact.birthdayInfo.birthdayDate.month == 0)
		{
			MemHandleUnlock(mH);
			continue;
		}
		
		
		bDay = contact.birthdayInfo.birthdayDate;
				
		DmRecordInfo(globals->AddrDB, counter, &attr, &id, NULL);				
		if(attr & dmRecAttrDelete )
		{
			MemHandleUnlock(mH);
			continue;
		}
		
		date.day=bDay.day;
		date.month=bDay.month;
		date.year=bDay.year + 1904;
		date.hour=0;
		date.minute=0;
		date.second=0;
		found=false;
		seconds=TimDateTimeToSeconds(&date);
		
		if(contact.birthdayInfo.birthdayMask.alarm == 0)
			remindDays = ALARM_NONE;
		else
			remindDays = contact.birthdayInfo.birthdayPreset;
		
		if(addrXTDBSize>0)
		{
			for(counter2=0;counter2<addrXTDBSize;counter2++)
			{
				AddrXTDBRecordType rec;
				MemHandle mAddrxtH;
				UInt32 recId;
										
				DmRecordInfo(addrXTDB, counter2, &attr, NULL, NULL);
				if(attr & dmRecAttrDelete )
					continue;
				
				if(AddrXTDBGetRecord(globals->adxtLibRef, addrXTDB, counter2, &rec, &mAddrxtH) != 0)
					continue;
				
				recId = rec.id;
				

							
				if(recId==id)
				{
					if(remindDays!=ALARM_NONE)
					{
						rec.remind = remindDays;
					}
					else
					{
						rec.remind = REMIND_NONE;
					}
					AddrXTDBChangeRecord(globals->adxtLibRef, addrXTDB, &rec, counter2);
					found=true;
					break;					
				}
				MemHandleUnlock(mAddrxtH);
			}
		}
		if(!found)//NEWEST
		{
			AddrXTDBRecordType rec;
			
			rec.id = id;
			rec.bday = seconds;
			
			if(remindDays!=ALARM_NONE)
			{
				rec.remind = remindDays;
			}
			else
			{
				rec.remind = REMIND_NONE;
			}
			rec.passed = 0;
			AddrXTDBNewRecord(globals->adxtLibRef, addrXTDB, &rec);
		}
		MemHandleUnlock(mH);
	}	
	prefsSize = sizeof (UInt32);
	PrefSetAppPreferences (CREATORID, addrPrefImportBirthdayID, addrPrefVersionNum, &modTime,
				   prefsSize, true);
	DmCloseDatabase(addrXTDB);
}


void ToolsDeleteRecord (Boolean archive)
{
	globalVars* globals = getGlobalsPtr();
	DmOpenRef addrXTDB=0, contactsDB = 0;
	UInt32 id;
	UInt16 lCnt;
	UInt16 addrXTDBSize;
	// Show the following record.  Users want to see where the record was and
	// they also want to return to the same location in the database because
	// they might be working their way through the records.  If there isn't
	// a following record show the prior record.  If there isn't a prior
	// record then don't show a record.
	CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &addrXTDB);
	
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, 0, &id, NULL);
	if(addrXTDB!=0)
	{
		addrXTDBSize = DmNumRecords(addrXTDB);
	}
	if(addrXTDB!=0 && addrXTDBSize>0)
	{
		for(lCnt=0; lCnt<addrXTDBSize; lCnt++)
		{
			AddrXTDBRecordType rec;
			MemHandle mH;
			UInt32 recId;
			
			if(AddrXTDBGetRecord(globals->adxtLibRef, addrXTDB, lCnt, &rec, &mH) != 0)
				continue;
			
			recId = rec.id;
			
			MemHandleUnlock(mH);
			
			if(recId==id)
			{
				deleteRecord(globals->adxtLibRef, addrXTDB, lCnt, archive);
				
				addrXTDBSize = DmNumRecords(addrXTDB);
				lCnt=0;
				continue;
			}		
		}	
	}
	while(AttnForgetIt(0, (LocalID)DmFindDatabase(0, APP_DBNAME), id))
	{
	};
	DeleteAllLinks(globals->adxtLibRef, globals->linksDB, id);
	
	globals->ListViewSelectThisRecord = globals->CurrentRecord;
	if (!ToolsSeekRecord(globals->adxtLibRef, &globals->ListViewSelectThisRecord, 1, dmSeekForward))
		if (!ToolsSeekRecord(globals->adxtLibRef, &globals->ListViewSelectThisRecord, 1, dmSeekBackward))
			globals->ListViewSelectThisRecord = noRecord;

	deleteRecord(globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, archive);
		
	// Since we just moved the globals->CurrentRecord to the end the
	// ListViewSelectThisRecord may have been moved up one position.
	if (globals->ListViewSelectThisRecord >= globals->CurrentRecord &&
		globals->ListViewSelectThisRecord != noRecord)
		globals->ListViewSelectThisRecord--;
	// Use whatever record we found to select.
	globals->CurrentRecord = globals->ListViewSelectThisRecord;
	
	if(addrXTDB!=0)
		DmCloseDatabase(addrXTDB);
		
	CleanRecentDB(globals->adxtLibRef);
	DeleteOrphaned(globals->adxtLibRef, globals->linksDB);
	
}

Boolean	ToolsIsPhoneIndexSupported( void* addrP, UInt16 phoneIndex)
{
	UInt16 label;
	UInt16	fieldIndex;
	
#ifdef CONTACTS
	if (phoneIndex == kDialListShowInListPhoneIndex)
	{
		phoneIndex = ((P1ContactsDBRecordPtr)addrP)->options.phones.displayPhoneForList;
	}	

	fieldIndex = P1ContactsfirstPhoneField + phoneIndex;

	if (!(((P1ContactsDBRecordPtr)addrP)->fields[fieldIndex]))
		return false;

	switch (fieldIndex)
	{
	case P1Contactsphone1:
		label = (P1ContactsPhoneLabels)(((P1ContactsDBRecordPtr)addrP)->options.phones.phone1);
		break;
	case P1Contactsphone2:
		label = (P1ContactsPhoneLabels)(((P1ContactsDBRecordPtr)addrP)->options.phones.phone2);
		break;
	case P1Contactsphone3:
		label = (P1ContactsPhoneLabels)(((P1ContactsDBRecordPtr)addrP)->options.phones.phone3);
		break;
	case P1Contactsphone4:
		label = (P1ContactsPhoneLabels)(((P1ContactsDBRecordPtr)addrP)->options.phones.phone4);
		break;
	case P1Contactsphone5:
		label = (P1ContactsPhoneLabels)(((P1ContactsDBRecordPtr)addrP)->options.phones.phone5);
		break;
	case P1Contactsphone6:
		label = (P1ContactsPhoneLabels)(((P1ContactsDBRecordPtr)addrP)->options.phones.phone6);
		break;
	case P1Contactsphone7:
		label = (P1ContactsPhoneLabels)(((P1ContactsDBRecordPtr)addrP)->options.phones.phone7);
		break;
	default:
		return false;
	}
	// Check whether this is a phone number and if so, whether it's one that's
	// appropriate to be dialed.
	switch (label)
	{
		// These are the phone numbers which are dialable.
		case P1ContactsworkLabel:
		case P1ContactshomeLabel:
		case P1ContactsotherLabel:
		case P1ContactsmainLabel:
		case P1ContactspagerLabel:
		case P1ContactsmobileLabel:
		case P1ContactsfaxLabel:
			return true;
			break;
		case P1ContactsemailLabel:
			if(ToolsVersaMailPresent())
			{
				return true;
			}
			break;
		default:
			return false;
			break;
	}
#else
	if (phoneIndex == kDialListShowInListPhoneIndex)
	{
		phoneIndex = ((AddrDBRecordPtr)addrP)->options.phones.displayPhoneForList;
	}	

	fieldIndex = firstPhoneField + phoneIndex;

	if (!(((AddrDBRecordPtr)addrP)->fields[fieldIndex]))
		return false;

	switch (fieldIndex)
	{
	case phone1:
		label = (AddressPhoneLabels)(((AddrDBRecordPtr)addrP)->options.phones.phone1);
		break;
	case phone2:
		label = (AddressPhoneLabels)(((AddrDBRecordPtr)addrP)->options.phones.phone2);
		break;
	case phone3:
		label = (AddressPhoneLabels)(((AddrDBRecordPtr)addrP)->options.phones.phone3);
		break;
	case phone4:
		label = (AddressPhoneLabels)(((AddrDBRecordPtr)addrP)->options.phones.phone4);
		break;
	case phone5:
		label = (AddressPhoneLabels)(((AddrDBRecordPtr)addrP)->options.phones.phone5);
		break;
	default:
		return false;
	}
	// Check whether this is a phone number and if so, whether it's one that's
	// appropriate to be dialed.
	switch (label)
	{
		// These are the phone numbers which are dialable.
		case workLabel:
			return true;
			break;
		case homeLabel:
			return true;
			break;
		case otherLabel:
			return true;
			break;
		case mainLabel:
			return true;
			break;
		case pagerLabel:
			return true;
			break;
		case mobileLabel:
			return true;
			break;
		case faxLabel:
			return true;
			break;
		case emailLabel:
			if(ToolsVersaMailPresent())
			{
				return true;
			}
			break;
		default:
			return false;
			break;
	}
#endif	
	return false;
}

UInt16 ToolsDuplicateCurrentRecord (UInt16 *numCharsToHilite, Boolean deleteCurrentRecord)
{
	globalVars* globals = getGlobalsPtr();
	univAddrDBRecordType recordToDup;
	UInt16 attr;
	Err err;
	UInt16 newRecordNum;
	MemHandle recordH;
	char *newFirstName = NULL;
	char duplicatedRecordIndicator [maxDuplicatedIndString + 1];
	UInt16 sizeToGet;
	UInt16 oldFirstNameLen;
	AddressFields fieldToAdd = firstName;
	DmOpenRef addrXTDB;
	UInt16 counter;
	Boolean found;
	UInt32 addrXTDBSize; UInt32 oldId, id;
	Char *lStrArray[4];
			
	univAddrDBGetRecord (globals->adxtLibRef, globals->AddrDB, globals->CurrentRecord, &recordToDup, &recordH);

	// Now we must add the "duplicated indicator" to the end of the First Name so that people
	// know that this was the duplicated record.
	ToolsGetStringResource (globals->adxtLibRef, DuplicatedRecordIndicatorStr, duplicatedRecordIndicator);
	*numCharsToHilite = StrLen (duplicatedRecordIndicator);

	// Find the first non-empty field from (first name, last name, company) to add "copy" to.
	fieldToAdd = univFirstName;
	if (recordToDup.fields[fieldToAdd] == NULL)
		fieldToAdd = univName;
	if (recordToDup.fields[fieldToAdd] == NULL)
		fieldToAdd = univCompany;
	// revert to last name if no relevant fields exist
	if (recordToDup.fields[fieldToAdd] == NULL)
		fieldToAdd = univName;

	if (recordToDup.fields[fieldToAdd] == NULL)
	{
		recordToDup.fields[fieldToAdd] = duplicatedRecordIndicator;
	}
	else
	{
		// Get enough space for current string, one blank and duplicated record
		// indicator string & end of string char.
		oldFirstNameLen = StrLen (recordToDup.fields[fieldToAdd]);
		sizeToGet = oldFirstNameLen + sizeOf7BitChar(spaceChr)+ StrLen (duplicatedRecordIndicator) + sizeOf7BitChar(nullChr);
		newFirstName = MemPtrNew (sizeToGet);

		if (newFirstName == NULL)
		{
			FrmAlert (DeviceFullAlert);
			newRecordNum = noRecord;
			goto Exit;
		}

		// make the new first name string with what was already there followed by
		// a space and the duplicate record indicator string.

		StrPrintF (newFirstName, "%s %s", recordToDup.fields[fieldToAdd], duplicatedRecordIndicator);

		recordToDup.fields[fieldToAdd] = newFirstName;
		// Must increment for the blank space that we add.
		(*numCharsToHilite)++;

		// Make sure that this string is less than or equal to the maximum allowed for
		// the field.
		if (StrLen (newFirstName) > MAXNAMELENGTH)
		{
			newFirstName [MAXNAMELENGTH] = '\0';
			(*numCharsToHilite) = MAXNAMELENGTH - oldFirstNameLen;
		}
	}

	globals->EditRowIDWhichHadFocus = fieldToAdd; //this is a lucky coincidence, that the first two
	//enums are the first two rows, but:
	if (globals->EditRowIDWhichHadFocus == company) //the third one's not
		globals->EditRowIDWhichHadFocus++;
	MemHandleUnlock(recordH);		
	
	// Make sure the attributes of the new record are the same.
	DmRecordInfo (globals->AddrDB, globals->CurrentRecord, &attr, &oldId, NULL);

	// If we are to delete the current record, then lets do that now.  We have
	// all the information from the record that we need to duplicate it correctly.
	if (deleteCurrentRecord)
	{
		ToolsDeleteRecord (false);
	}

	// Now create the new record that has been duplicated from the current record.
	err = univAddrDBNewRecord(globals->adxtLibRef, globals->AddrDB, &recordToDup, &newRecordNum);
	if (err)
	{
		FrmAlert(DeviceFullAlert);
		newRecordNum = noRecord;
		goto Exit;
	}

	// This includes the catagory so the catagories are the same between the original and
	// duplicated record.
	attr |= dmRecAttrDirty;
	DmSetRecordInfo (globals->AddrDB, newRecordNum, &attr, NULL);
	
	//dublicate birthday info
	CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &addrXTDB);
	for(counter=0;counter<4;counter++)
	{
		lStrArray[counter]=MemPtrNew(255);
	}
	
	addrXTDBSize = DmNumRecords(addrXTDB);
	//DmDatabaseSize(0, (LocalID)DmFindDatabase(0, ADDRXTDB_DBNAME), &addrXTDBSize, 0, 0);
	DmRecordInfo (globals->AddrDB, newRecordNum, NULL, &id, NULL);
	for(counter=0;counter<addrXTDBSize;counter++)
	{
		AddrXTDBRecordType rec;
		MemHandle mH;
		UInt32 recId, recSeconds, recPassed, recRemind;
		
		DmRecordInfo(addrXTDB, counter, &attr, NULL, NULL);
		if(attr & dmRecAttrDelete )
			continue;

		if(AddrXTDBGetRecord(globals->adxtLibRef, addrXTDB, counter, &rec, &mH) != 0)
			continue;
		
		recId = rec.id;
		recSeconds = rec.bday;
		recPassed = rec.passed;
		recRemind = rec.remind;
		
		MemHandleUnlock(mH);
		
		if(oldId==recId)
		{
			rec.id = id;
			
			AddrXTDBNewRecord(globals->adxtLibRef, addrXTDB, &rec);
			found=true;
			break;		
		}
	}
		
	DmCloseDatabase(addrXTDB);
	
	// This includes the catagory so the catagories are the same between the original and
	// duplicated record.
	attr |= dmRecAttrDirty;
	DmSetRecordInfo (globals->AddrDB, newRecordNum, &attr, NULL);

Exit:
	if (newFirstName)
	{
		MemPtrFree (newFirstName);
	}

	return (newRecordNum);
}


Int16 StrNumOfChrAdv(const char* lStr, WChar lChr, Boolean lRecursive)
{
	Char* lPointer;
	static Int16 rRes=0;
	if(!lRecursive)
		rRes=0;
	if(StrLen(lStr)==0)
		return rRes;
	lPointer=StrChr(lStr, lChr);
	if(lPointer)
		{
		rRes++;
		StrNumOfChrAdv(lPointer+1, lChr, true);
		}
	return rRes;
}

void ToolsDirtyRecord (UInt16 index)
{
	globalVars* globals = getGlobalsPtr();
	UInt16      attr;

	DmRecordInfo (globals->AddrDB, index, &attr, NULL, NULL);
	attr |= dmRecAttrDirty;
	DmSetRecordInfo (globals->AddrDB, index, &attr, NULL);
}

//-------


void CstSetFldEditable(UInt16 tObjID, UInt16 tEditable)
{
	FieldAttrType tAttr;
	FldGetAttributes(CustomGetObjectPtrSmp(tObjID), &tAttr);
	tAttr.editable=tEditable;
	FldSetAttributes(CustomGetObjectPtrSmp(tObjID), &tAttr);
}

void CstSetFldUnderlined(UInt16 tObjID, UInt16 tEditable)
{
	FieldAttrType tAttr;
	FldGetAttributes(CustomGetObjectPtrSmp(tObjID), &tAttr);
	tAttr.underlined=tEditable;
	FldSetAttributes(CustomGetObjectPtrSmp(tObjID), &tAttr);
}

UInt16 CustomFrmGetFocus(FormType *tFormP)
{
	if(FrmGetFocus(tFormP)!=noFocus)
		return FrmGetObjectId(tFormP, FrmGetFocus(tFormP));
	else
		return -1;
}
UInt16 CustomFrmGetFocusSmp()
{
	return CustomFrmGetFocus(FrmGetActiveForm());
}
void CstHitControl(UInt16 tObjID)
{
	CtlHitControl((ControlType*)FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), tObjID)));
}
void CstFldFreeSmp(UInt16 tObjID)
{
	FldFreeMemory(CustomGetObjectPtrSmp(tObjID)); 
	FldEraseField(CustomGetObjectPtrSmp(tObjID)); 
}

void CustomFldSetTextPtr(FormType *tFormP, UInt16 tFieldID, Char* tText)
{
	FldSetTextPtr(CustomGetObjectPtr(tFormP, tFieldID), tText);
}
void CustomFldSetTextPtrSmp(UInt16 tFieldID, Char* tText)
{
	FldSetTextPtr(CustomGetObjectPtrSmp(tFieldID), tText);
}

Char* CstCtlGetLabel(UInt16 tObjID)
{
	return (Char*)CtlGetLabel(CustomGetObjectPtrSmp(tObjID));
}
Int16 CustomGetIndexInList(const ListType *tListP, const Char* tText)
{
	int lIndex;
	if(tText==0)
		return -1;
	if(StrLen(tText)==0)
		return -1;
	for(lIndex=0;lIndex<LstGetNumberOfItems(tListP);lIndex++)
		if(!StrCompare(LstGetSelectionText(tListP, lIndex), tText))
			return lIndex;
	return -1;
}

Int16 CustomGetIndexInListSmp(UInt16 tListID, const Char* tText)
{
	return CustomGetIndexInList(CustomGetObjectPtrSmp(tListID), tText);
}
Boolean CustomFldIsEmpty(FormType *tFormP, UInt16 tObjID)
{
	Char* lPtr=CustomFldGetTextPtr(tFormP, tObjID);
	if(lPtr==0)
		return true;
	if(StrLen(lPtr)==0)
		return true;
	return false;
}


Boolean CustomFldIsEmptySmp(UInt16 tObjID)
{
	return CustomFldIsEmpty(FrmGetActiveForm(), tObjID);
}

Boolean FldEmptySmp(UInt16 tObjID)
{
	Char* lPtr=FldGetTextPtrSmp(tObjID);
	if(lPtr==0)
		return true;
	if(StrLen(lPtr)==0)
		return true;
	return false;
}

void FldSetTextPtrSmp(UInt16 tFieldID, Char* tText)
{
	MemHandle lHandle, lOldHandle;
	MemPtr lPtr;
	FieldType *lField;
	lField=(FieldType*)GetObjectPtrSmp(tFieldID);
	lOldHandle=FldGetTextHandle(lField);
	lHandle=MemHandleNew(StrLen(tText)+1);
	lPtr=MemHandleLock(lHandle);
	MemMove(lPtr, tText, StrLen(tText)+1);
	FldSetTextHandle(lField, lHandle);
	if(lOldHandle)
		MemHandleFree(lOldHandle);
	FldDrawField(lField);	
	MemHandleUnlock(lHandle);
	return;
}

void ToolsLeaveForm  ()
{
	FormPtr frm;

	frm = FrmGetActiveForm();
	FrmEraseForm (frm);
	FrmDeleteForm (frm);
	FrmSetActiveForm (FrmGetFirstForm ());
}

FontID ToolsSelectFontWithHires(FontID currFontID, Boolean withHires)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 formID;
	FontID font;
	Boolean hires = globals->AddrListHighRes;

	formID = FrmGetFormId( FrmGetActiveForm() );

	// OLD: Call the OS font selector to get the id of a font.
	//fontID = FontSelect (currFontID);	
	
	if( AddressFontSelect(currFontID, &font, &hires, withHires) )
	{
	
		if (font != currFontID || globals->AddrListHighRes != hires)
		{
			if(globals->AddrListHighRes != hires)
			{
				globals->AddrListHighRes = hires;
				FrmUpdateForm(ListView, updatePrefs);
			}
			else 
				FrmUpdateForm(formID, updateFontChanged);
		}
		
		return font;
	}
	else
		return currFontID;
}

FontID ToolsSelectFont(FontID currFontID)
{
	return ToolsSelectFontWithHires(currFontID, false);
}

Boolean ToolsAddrBeamBusinessCard (DmOpenRef dbP)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 recordNum;


	if (DmFindRecordByID (globals->AddrDB, globals->BusinessCardRecordID, &recordNum) == dmErrUniqueIDNotFound ||
		DmQueryRecord(dbP, recordNum) == 0)
	{
		FrmAlert(SendBusinessCardAlert);
	}
	
	else
	{
		TransferSendRecord(dbP, recordNum, exgBeamPrefix, NoDataToBeamAlert);
		return true;
	}

	return false;
}