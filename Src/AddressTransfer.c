#ifdef LIBDEBUG //moved to shared library

/******************************************************************************
 *
 * Copyright (c) 1995-2002 PalmSource, Inc. All rights reserved.
 *
 * File: AddressTransfer.c
 *
 * Release: Palm OS 5 SDK (68K) R3.
 *
 * Description:
 *      Address Book routines to transfer records.
 *
 *****************************************************************************/

#include "Address.h"
#include "AddressTransfer.h"
#include "AddressRsc.h"
#include "AddrDefines.h"
#include "AddrTools.h"
#include "globals.h"
#include "AddrXTDB.h"
#include "AddressDB2.h"
#include <PdiLib.h>
#include <UDAMgr.h>
#include <ErrorMgr.h>
#include <UIResources.h>
#include <StringMgr.h>
#include <Category.h>
#include <FeatureMgr.h>


/***********************************************************************
 *
 * FUNCTION:		TransferRegisterData
 *
 * DESCRIPTION:		Register with the exchange manager to receive data
 * with a certain name extension.
 *
 * PARAMETERS:		nothing
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rsf		12/2/97		Created
 *
 ***********************************************************************/
void TransferRegisterData (UInt16 adxtRefNum)
{
	MemHandle resH = DmGetResource(strRsc, ExgDescriptionStr);
	void *desc = MemHandleLock(resH);
	UInt32 romVersion, creator;
	Char toDoCreatorStr[5];
	UInt32 creatorID;
	
	
	Err err = FtrGet(sysFtrCreator,	sysFtrNumROMVersion, &romVersion);
	
	creatorID = CREATORID;
	
	if(romVersion>=0x04003000)
	{
		ExgRegisterDatatype(creatorID, exgRegExtensionID, addrFilenameExtension, desc, 0);
		ExgRegisterDatatype(creatorID, exgRegTypeID, addrMIMEType, desc, 0);
	}
	creator=CREATORID;
	ExgSetDefaultApplication(creatorID, exgRegExtensionID, addrFilenameExtension);
	ExgSetDefaultApplication(creatorID, exgRegTypeID, addrMIMEType);
	ExgGetDefaultApplication(&creator, exgRegExtensionID, addrFilenameExtension);
	MemHandleUnlock(resH);
	
	DmReleaseResource(resH);
	creator=sysFileCAddress;
	MemMove(toDoCreatorStr, &creator, 4);
	toDoCreatorStr[4] = chrNull;
	ExgRegisterDatatype(creatorID, exgRegCreatorID, toDoCreatorStr, NULL, 0);
	
}

/***********************************************************************
 *
 * FUNCTION:		ReceiveData
 *
 * DESCRIPTION:		Receives data into the output field using the Exg API
 *
 * PARAMETERS:		exgSocketP, socket from the app code
 *						 sysAppLaunchCmdExgReceiveData
 *
 * RETURNED:		error code or zero for no error.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         ABa     6/20/00  Integrate Pdi library
 *
 ***********************************************************************/
Err TransferReceiveData(UInt16 adxtRefNum, DmOpenRef dbP, ExgSocketPtr exgSocketP)
{
	volatile Err err;
	UInt16 pdiRefNum = sysInvalidRefNum;
	PdiReaderType* reader = NULL;
	UDAReaderType* stream = NULL;
	Boolean loaded;
	
	globalVars* globals = getGlobalsPtr();
	// accept will open a progress dialog and wait for your receive commands
	if ((err = ExgAccept(exgSocketP)) != 0)
		return err;

	if ((err = PrvTransferPdiLibLoad(globals->adxtLibRef, &pdiRefNum, &loaded)))
	{
		pdiRefNum = sysInvalidRefNum;
		goto errorDisconnect;
	}	
		
	if ((stream = UDAExchangeReaderNew(exgSocketP)) == NULL)
	{
		err = exgMemError;
		goto errorDisconnect;
	}
		
	if ((reader = PdiReaderNew(pdiRefNum, stream, kPdiOpenParser)) == NULL)
	{
		err = exgMemError;
		goto errorDisconnect;
	}
	
	reader->appData = exgSocketP;
	
	ErrTry
	{
		// Keep importing records until it can't
		while(TransferImportVCard(globals->adxtLibRef, dbP, pdiRefNum, reader, false, false)){};
	}
	ErrCatch(inErr)
	{
		err = inErr;
	} ErrEndCatch

	// Aba: A record has been added in the Database iff the GoTo
	// uniqueID parameter != 0.
	// In the case no record is added, return an error
	if (err == errNone && exgSocketP->goToParams.uniqueID == 0)
		err = exgErrBadData;
	
errorDisconnect:
	if (reader)
		PdiReaderDelete(pdiRefNum, &reader);

	if (stream)
		UDADelete(stream);
	
	if (pdiRefNum != sysInvalidRefNum)
		PrvTransferPdiLibUnload(globals->adxtLibRef, pdiRefNum, loaded);
		
	ExgDisconnect(exgSocketP, err); // closes transfer dialog
	err = errNone;	// error was reported, so don't return it

	return err;
}


/************************************************************
 *
 * FUNCTION: TransferImportVCard
 *
 * DESCRIPTION: Import a VCard record.
 *
 * PARAMETERS:
 *			dbP - pointer to the database to add the record to
 *			inputStream	- pointer to where to import the record from
 *			inputFunc - function to get input from the stream
 *			obeyUniqueIDs - true to obey any unique ids if possible
 *			beginAlreadyRead - whether the begin statement has been read
 *
 * RETURNS: true if the input was read
 *
 *	REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rsf		4/24/97		Initial Revision
 *			bhall	8/12/99		moved category beaming code from gromit codeline
 *          ABa     6/20/00  	Integrate Pdi library
 *
 *************************************************************/


Boolean TransferImportVCard(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 pdiRefNum, PdiReaderType* reader, Boolean obeyUniqueIDs, Boolean beginAlreadyRead)
{
	univAddrDBRecordType 		newRecord;
	AddressPhoneLabels 			phoneLabel;
	UInt16						phoneField;
	UInt16 						indexNew;
	UInt16						addrNum = 0;
	UInt16						chatNum = 0;
	UInt16 						indexOld;
	UInt32 						uid;
	Err 						err;
	UInt32 						uniqueID;
	volatile Err 				error = 0;
	UInt16						property;
	Char* 						addressBufferP = NULL;
	Char* 						nameBufferP = NULL;
	UInt16						i;
	Char 						bday[31];
	UInt16						categoryID; UInt32 id;
	Char 						categoryName[dmCategoryLength];
#ifdef CONTACTS
	Char* 						buffer;
	Char*						propName;
#endif
	Boolean bdayRcvd=false;
	Boolean lastPhone;
	Boolean changed = false;
	Boolean knownFormat=false;
#ifdef CONTACTS
	unsigned int chatType;
#endif
	globalVars* globals = getGlobalsPtr();

	MemSet(bday, 31, 0);
	
	// if we have ExgManager socket
	if (reader->appData != NULL)
	{
		categoryID = ((ExgSocketPtr)(reader->appData))->appData;
	}

	// Initialize a new record
#ifdef CONTACTS
		for (i=0; i < P1ContactsNumFields; i++)
			newRecord.fields[i] = NULL;	// clear the record
		newRecord.options.phones.phone1 = P1ContactsworkLabel;
		newRecord.options.phones.phone2 = P1ContactshomeLabel;
		newRecord.options.phones.phone3 = P1ContactsmobileLabel;
		newRecord.options.phones.phone4 = P1ContactsemailLabel;
		newRecord.options.phones.phone5 = P1ContactsmainLabel;
		newRecord.options.phones.phone6 = P1ContactspagerLabel;
		newRecord.options.phones.phone7 = P1ContactsotherLabel;
		
		newRecord.options.chatIds.reserved = 0;
		newRecord.options.chatIds.chat1 = aimChatLabel;
		newRecord.options.chatIds.chat2 = msnChatLabel;
		
		newRecord.options.addresses.address1 = P1ContactsworkAddressLabel;
		newRecord.options.addresses.address2 = P1ContactshomeAddressLabel;
		newRecord.options.addresses.address3 = P1ContactsotherAddressLabel;

		newRecord.options.phones.displayPhoneForList = P1Contactsphone1 - P1ContactsfirstPhoneField;
#else
		for (i=0; i < addrNumFields; i++)
			newRecord.fields[i] = NULL;	// clear the record
		newRecord.options.phones.phone1 = 0;	// Work
		newRecord.options.phones.phone2 = 1;	// Home
		newRecord.options.phones.phone3 = 2;	// Fax
		newRecord.options.phones.phone4 = 7;	// Other
		newRecord.options.phones.phone5 = 3;	// Email
		newRecord.options.phones.displayPhoneForList = phone1 - firstPhoneField;
#endif
	
	uid = 0;

	ErrTry
	{
		phoneField = univFirstPhoneField;

		if (!beginAlreadyRead)
		{
			PdiReadProperty(pdiRefNum, reader);
			beginAlreadyRead = reader->property == kPdiPRN_BEGIN_VCARD;
		}
		
		// if not "BEGIN:VCARD"
		if (!beginAlreadyRead)
			ErrThrow(exgErrBadData);
			
		PdiEnterObject(pdiRefNum, reader);
		PdiDefineResizing(pdiRefNum, reader, 16, tableMaxTextItemSize);

		while (PdiReadProperty(pdiRefNum, reader) == 0 && (property = reader->property) != kPdiPRN_END_VCARD)
		{
			switch(property)
			{
			case kPdiPRN_N:
				PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[univName], kPdiResizableBuffer, kPdiDefaultFields);
				PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[univFirstName], kPdiResizableBuffer, kPdiDefaultFields);
				break;

			case kPdiPRN_BDAY:
				{
					Char *bdayP = NULL;
					err = PdiReadPropertyField(pdiRefNum, reader, &bdayP, kPdiResizableBuffer, kPdiNoFields);
					if (bdayP != NULL)
					{
#ifdef CONTACTS
						UInt32 seconds; DateTimeType date;
						UInt16 m, d, y; 
#endif

						StrNCopy(bday, bdayP, 8);
						MemPtrFree(bdayP);
						bdayRcvd=true;
#ifdef CONTACTS			
						if(StrLen(bday)==8)
						{
							//parse bday in format: yyyymmdd
							Char yStr[5], dStr[3], mStr[3];
							yStr[0]=bday[0];
							yStr[1]=bday[1];
							yStr[2]=bday[2];
							yStr[3]=bday[3];
							yStr[4]=0;
							
							mStr[0]=bday[4];
							mStr[1]=bday[5];
							mStr[2]=0;
							
							dStr[0]=bday[6];
							dStr[1]=bday[7];
							dStr[2]=0;
							
							m=StrAToI(mStr);
							d=StrAToI(dStr);
							y=StrAToI(yStr);	
							
							date.year=y;
							date.month=m;
							date.day=d;
							date.hour=0;
							date.minute=0;
							date.second=0;
							
							seconds=TimDateTimeToSeconds(&date);		
							
							knownFormat=true;													
						}
						else if (StrLen(bday)==10)
						{
							//parse bday in format: yyyy-mm-dd
							Char yStr[5], dStr[3], mStr[3];
							yStr[0]=bday[0];
							yStr[1]=bday[1];
							yStr[2]=bday[2];
							yStr[3]=bday[3];
							yStr[4]=0;
							
							mStr[0]=bday[5];
							mStr[1]=bday[6];
							mStr[2]=0;
							
							dStr[0]=bday[8];
							dStr[1]=bday[9];
							dStr[2]=0;
							
							m=StrAToI(mStr);
							d=StrAToI(dStr);
							y=StrAToI(yStr);	
							
							date.year=y;
							date.month=m;
							date.day=d;
							date.hour=0;
							date.minute=0;
							date.second=0;
							
							seconds=TimDateTimeToSeconds(&date);		
							
							knownFormat=true;									
						}

						if(knownFormat)
						{
							DateType day;
							AddressDBBirthdayFlags flag;
							flag.alarm = 0;
							flag.reserved = 0;
							newRecord.birthdayInfo.birthdayMask = flag;
							day.day = d;
							day.month = m;
							day.year = y + 16;
							newRecord.birthdayInfo.birthdayDate = day;					
						}						
#endif		
						
					}
				}
				break;
			
			case kPdiPRN_ADR:
				{
					UInt8 n = 0;
					UInt16 totalSize;
					Char* addressPostOfficeP = NULL;
					Char* addressExtendedP = NULL;
					Char* strings[3];

#ifdef CONTACTS
					unsigned int phoneType;
					
					PdiReadPropertyField(pdiRefNum, reader, &addressPostOfficeP, kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, &addressExtendedP, kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[P1Contactsaddress + 5*addrNum], kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[P1Contactscity + 5*addrNum], kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[P1Contactsstate + 5*addrNum], kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[P1ContactszipCode + 5*addrNum], kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[P1Contactscountry + 5*addrNum], kPdiResizableBuffer, kPdiDefaultFields);
					
					if (PdiParameterPairTest(reader, kPdiPAV_TYPE_WORK))
						phoneType = P1ContactsworkAddressLabel;
					else if (PdiParameterPairTest(reader, kPdiPAV_TYPE_HOME))
						phoneType = P1ContactshomeAddressLabel;
					else if (PdiParameterPairTest(reader, kPdiPAV_TYPE_DOM))
						phoneType = P1ContactsotherAddressLabel;
						
					if(addrNum == 0)
						newRecord.options.addresses.address1 = phoneType;
					else if(addrNum == 1)
						newRecord.options.addresses.address2 = phoneType;
					else if(addrNum == 2)
						newRecord.options.addresses.address3 = phoneType;
						
					
					if (newRecord.fields[P1Contactsaddress + 5*addrNum] != NULL)
					{
						strings[n++] = newRecord.fields[P1Contactsaddress + 5*addrNum];
						totalSize += StrLen(newRecord.fields[P1Contactsaddress + 5*addrNum]);
					}
					if (addressPostOfficeP != NULL)
					{
						strings[n++] = addressPostOfficeP;
						totalSize += StrLen(addressPostOfficeP);
					}
					if (addressExtendedP != NULL)
					{
						strings[n++] = addressExtendedP;
						totalSize += StrLen(addressExtendedP);
					}

					if (addressPostOfficeP != NULL || addressExtendedP != NULL)
					{
						Char* result = NULL;

						totalSize += (n - 1) * (sizeOf7BitChar(linefeedChr) + sizeOf7BitChar(spaceChr)) + sizeOf7BitChar(nullChr);
						if (totalSize > tableMaxTextItemSize)
						{
							totalSize = tableMaxTextItemSize;
						}
						result = MemHandleLock(MemHandleNew(totalSize));
						if (result != NULL)
						{
							*result = 0;
							while (n > 0)
							{
								n--;
								StrNCat(result, strings[n], totalSize);
								if (n != 0)
								{
									StrNCat(result, "\n ", totalSize);
								}
							}
						}
						if (addressPostOfficeP != NULL)
						{
							MemPtrFree(addressPostOfficeP);
						}
						if (addressExtendedP != NULL)
						{
							MemPtrFree(addressExtendedP);
						}
						if (newRecord.fields[P1Contactsaddress + 5*addrNum] != NULL && result != NULL)
						{
							MemPtrFree(newRecord.fields[P1Contactsaddress + 5*addrNum]);
						}
						if (result != NULL)
						{
							newRecord.fields[P1Contactsaddress + 5*addrNum] = result;
						}
					}
					addrNum++;
#else
					PdiReadPropertyField(pdiRefNum, reader, &addressPostOfficeP, kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, &addressExtendedP, kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[address], kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[city], kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[state], kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[zipCode], kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[country], kPdiResizableBuffer, kPdiDefaultFields);

					if (newRecord.fields[address] != NULL)
					{
						strings[n++] = newRecord.fields[address];
						totalSize += StrLen(newRecord.fields[address]);
					}
					if (addressPostOfficeP != NULL)
					{
						strings[n++] = addressPostOfficeP;
						totalSize += StrLen(addressPostOfficeP);
					}
					if (addressExtendedP != NULL)
					{
						strings[n++] = addressExtendedP;
						totalSize += StrLen(addressExtendedP);
					}

					if (addressPostOfficeP != NULL || addressExtendedP != NULL)
					{
						Char* result = NULL;

						totalSize += (n - 1) * (sizeOf7BitChar(linefeedChr) + sizeOf7BitChar(spaceChr)) + sizeOf7BitChar(nullChr);
						if (totalSize > tableMaxTextItemSize)
						{
							totalSize = tableMaxTextItemSize;
						}
						result = MemHandleLock(MemHandleNew(totalSize));
						if (result != NULL)
						{
							*result = 0;
							while (n > 0)
							{
								n--;
								StrNCat(result, strings[n], totalSize);
								if (n != 0)
								{
									StrNCat(result, "\n ", totalSize);
								}
							}
						}
						if (addressPostOfficeP != NULL)
						{
							MemPtrFree(addressPostOfficeP);
						}
						if (addressExtendedP != NULL)
						{
							MemPtrFree(addressExtendedP);
						}
						if (newRecord.fields[address] != NULL && result != NULL)
						{
							MemPtrFree(newRecord.fields[address]);
						}
						if (result != NULL)
						{
							newRecord.fields[address] = result;
						}
					}					
#endif
				}
				break;

			case kPdiPRN_FN:
				// Take care of FN iff no name nor first name.
				if (newRecord.fields[univName] == NULL && newRecord.fields[univFirstName] == NULL)
				{
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[univName], kPdiResizableBuffer, kPdiDefaultFields);
				}		
				break;

			case kPdiPRN_NICKNAME:
				// Take care of nickname iff no first name.
				if (newRecord.fields[univFirstName] == NULL)
				{
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[univFirstName], kPdiResizableBuffer, kPdiDefaultFields);
				}				
				break;

#ifdef CONTACTS
			case kPdiPRN_URL:
				PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[P1Contactswebpage], kPdiResizableBuffer, kPdiDefaultFields);
				break;
#endif
			case kPdiPRN_ORG:
				PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[univCompany], kPdiResizableBuffer, kPdiConvertSemicolon);
				break;

			case kPdiPRN_TITLE:
				PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[univTitle], kPdiResizableBuffer, kPdiConvertSemicolon);
				break;

			case kPdiPRN_NOTE:
				PdiDefineResizing(pdiRefNum, reader, 16, noteViewMaxLength);
				PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[univNote], kPdiResizableBuffer, kPdiNoFields);			
				PdiDefineResizing(pdiRefNum, reader, 16, tableMaxTextItemSize);
				break;

			case kPdiPRN_X_PALM_CATEGORY:
				{
					Char* categoryStringP = NULL;
					PdiReadPropertyField(pdiRefNum, reader, &categoryStringP, kPdiResizableBuffer, kPdiNoFields);
					if (categoryStringP != NULL)
					{
						// Make a copy
						StrNCopy(categoryName, categoryStringP, dmCategoryLength);
						// Free the string (Imc routines allocate the space)
						MemPtrFree(categoryStringP);
						// If we ever decide to use vCard 3.0 CATEGORIES, we would need to skip additional ones here
					}
				}
				break;

			case kPdiPRN_TEL:
			case kPdiPRN_EMAIL:
				lastPhone = !(phoneField <= univLastPhoneField);
				
				if (!lastPhone)
				{
					if (reader->property == kPdiPRN_EMAIL)
					{
						phoneLabel = univEmailLabel;
					}
					else if (PdiParameterPairTest(reader, kPdiPAV_TYPE_FAX))
					{
						phoneLabel = univFaxLabel;
					}
					else if (PdiParameterPairTest(reader, kPdiPAV_TYPE_PAGER))
					{
						phoneLabel = univPagerLabel;
					}
					else if (PdiParameterPairTest(reader, kPdiPAV_TYPE_CAR) || PdiParameterPairTest(reader, kPdiPAV_TYPE_CELL))
					{
						phoneLabel = univMobileLabel;
					}
					else if (PdiParameterPairTest(reader, kPdiPAV_X_X_PALM_MAIN))
					{
						phoneLabel = univMainLabel;
					}
					else if (PdiParameterPairTest(reader, kPdiPAV_TYPE_HOME))
					{
						phoneLabel = univHomeLabel;
					}
					else if (PdiParameterPairTest(reader, kPdiPAV_TYPE_WORK))
					{
						phoneLabel = univWorkLabel;
					}
					else
					{
						phoneLabel = univOtherLabel;
					}
#ifdef CONTACTS
					SetPhoneLabel(&newRecord, phoneField - 1, phoneLabel);
#else
					SetPhoneLabel(&newRecord, phoneField, phoneLabel);
#endif
					SetPhoneLabel(&newRecord, phoneField, phoneLabel);
					if (PdiParameterPairTest(reader, kPdiPAV_TYPE_PREF))
					{
						newRecord.options.phones.displayPhoneForList = phoneField - univFirstPhoneField;
					}
					PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[phoneField], kPdiResizableBuffer, kPdiNoFields);
					if (newRecord.fields[phoneField] != NULL)
					{
						phoneField++;
					}
				}
				break;

			case kPdiPRN_X_PALM_CUSTOM:
				ErrNonFatalDisplayIf(reader->customFieldNumber >= (univLastRenameableLabel - univFirstRenameableLabel + 1),
								 "Invalid Custom Field");
				PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[univCustom1 + reader->customFieldNumber],
									 kPdiResizableBuffer, kPdiNoFields);
				break;

			case kPdiPRN_BEGIN_VCARD:
				TransferImportVCard(globals->adxtLibRef, dbP, pdiRefNum, reader, obeyUniqueIDs, true);
				break;
			case kPdiPRN_PHOTO:
#ifdef CONTACTS
				buffer = NULL;
				newRecord.pictureInfo.pictureData = NULL;
				err = PdiReadPropertyField(pdiRefNum, reader, &buffer, kPdiResizableBuffer, kPdiSemicolonFields);
				newRecord.pictureInfo.pictureData = buffer;
				newRecord.pictureInfo.pictureDirty = 1;
				newRecord.pictureInfo.pictureSize = MemPtrSize(buffer);
#endif
				break;	
			default:
#ifdef CONTACTS
				propName = reader->propertyName;
				if(propName == NULL)
					break;
				if(StrLen(propName) == NULL)
					break;
				if(!StrCompare(propName, "X-ICQ"))
					chatType = icqChatLabel;
				else if(!StrCompare(propName, "X-AIM"))
					chatType = aimChatLabel;
				else if(!StrCompare(propName, "X-MSN"))
					chatType = msnChatLabel;
				else if(!StrCompare(propName, "X-YAHOO"))
					chatType = yahooChatLabel;
				else if(!StrCompare(propName, "X-PALMSG-IM"))
					chatType = otherChatLabel;
				else
					break;
				
				if(chatNum == 0)
				{
					newRecord.options.chatIds.chat1 = chatType;					
				}
				else if (chatNum == 1)
				{
					newRecord.options.chatIds.chat2 = chatType;					
				}
				PdiReadPropertyField(pdiRefNum, reader, (Char **) &newRecord.fields[P1Contactschat1 + chatNum],
							 kPdiResizableBuffer, kPdiNoFields);
				chatNum++;						
#endif
				break;

			}
		} // end while

		// We don't have to search for the pref phone, because we are sure it is not null
		// (or it is but there was no TEL or EMAIL property in the vcard and so the
		// displayList is firstPhoneField)

		// if the company and name fields are identical, assume company only
		if (newRecord.fields[univName] != NULL
			&& newRecord.fields[univCompany] != NULL
			&& newRecord.fields[univFirstName] == NULL
			&& StrCompare(newRecord.fields[univName], newRecord.fields[univCompany]) == 0)
			{
				MemPtrFree(newRecord.fields[univName]);
				newRecord.fields[name] = NULL;
			}
		// Before adding the record verify that one field at least in non NULL
		
		for (i = 0; i < univAddrNumFields; i++)
		{
			if (newRecord.fields[i] != NULL)
				goto addRecord;
		}
		
		// All fields are NULL: not really an error but we must not add the record
		ErrThrow(errNone);

addRecord:
		
		err = univAddrDBNewRecord(globals->adxtLibRef, dbP, &newRecord, &indexNew);
		// Memory error ? 
		if (err)
			ErrThrow(exgMemError);
		//add birthday info to addrxtdb
		DmRecordInfo (dbP, indexNew, NULL, &id, NULL);		
		
		if(bdayRcvd)
		{
			UInt32 seconds; DateTimeType date;
			DmOpenRef addrXTDB;
			UInt16 m, d, y; 
	
			if(StrLen(bday)==8)
			{
				//parse bday in format: yyyymmdd
				Char yStr[5], dStr[3], mStr[3];
				yStr[0]=bday[0];
				yStr[1]=bday[1];
				yStr[2]=bday[2];
				yStr[3]=bday[3];
				yStr[4]=0;
				
				mStr[0]=bday[4];
				mStr[1]=bday[5];
				mStr[2]=0;
				
				dStr[0]=bday[6];
				dStr[1]=bday[7];
				dStr[2]=0;
				
				m=StrAToI(mStr);
				d=StrAToI(dStr);
				y=StrAToI(yStr);	
				
				date.year=y;
				date.month=m;
				date.day=d;
				date.hour=0;
				date.minute=0;
				date.second=0;
				
				seconds=TimDateTimeToSeconds(&date);		
				
				knownFormat=true;													
			}
			else if (StrLen(bday)==10)
			{
				//parse bday in format: yyyy-mm-dd
				Char yStr[5], dStr[3], mStr[3];
				yStr[0]=bday[0];
				yStr[1]=bday[1];
				yStr[2]=bday[2];
				yStr[3]=bday[3];
				yStr[4]=0;
				
				mStr[0]=bday[5];
				mStr[1]=bday[6];
				mStr[2]=0;
				
				dStr[0]=bday[8];
				dStr[1]=bday[9];
				dStr[2]=0;
				
				m=StrAToI(mStr);
				d=StrAToI(dStr);
				y=StrAToI(yStr);	
				
				date.year=y;
				date.month=m;
				date.day=d;
				date.hour=0;
				date.minute=0;
				date.second=0;
				
				seconds=TimDateTimeToSeconds(&date);		
				
				knownFormat=true;									
			}
			
			if(knownFormat)
			{
				AddrXTDBRecordType rec;
				
				CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &addrXTDB);
					
				rec.id = id;
				rec.bday = seconds;
				rec.remind = REMIND_NONE;
				rec.passed = 0;

				AddrXTDBNewRecord(globals->adxtLibRef, addrXTDB, &rec);
					
				DmCloseDatabase(addrXTDB);
			}		
		}
		
#ifdef VCARD_CATEGORIES
		// If a category was included, try to use it
		if (categoryName[0])
		{
			UInt16	categoryID;
			UInt16	attr;
			Err		err;

			// Get the category ID
			categoryID = CategoryFind(dbP, categoryName);

			// If it doesn't exist, and we have room, create it
			if (categoryID == dmAllCategories)
			{
				// Find the first unused category
				categoryID = CategoryFind(dbP, "");

				// If there is a slot, fill it with the name we were given
				if (categoryID != dmAllCategories)
				{
					CategorySetName(dbP, categoryID, categoryName);
				}
			}

			// Set the category for the record
			if (categoryID != dmAllCategories)
			{
				// Get the attributes
				err = DmRecordInfo(dbP, indexNew, &attr, NULL, NULL);

				// Set them to include the category, and mark the record dirty
				if ((attr & dmRecAttrCategoryMask) != categoryID)
				{
					attr &= ~dmRecAttrCategoryMask;
					attr |= categoryID | dmRecAttrDirty;
					err = DmSetRecordInfo(dbP, indexNew, &attr, NULL);
				}
			}
		}
#endif

		// Set the category for the record
		if (categoryID)
		{
			UInt16	attr;
			Err		err;

			// Get the attributes
			//PRINTDEBUG(3);	
			err = DmRecordInfo(dbP, indexNew, &attr, NULL, NULL);

			// Set them to include the category, and mark the record dirty
			if ((attr & dmRecAttrCategoryMask) != categoryID)
			{
				attr &= ~dmRecAttrCategoryMask;
				attr |= categoryID | dmRecAttrDirty;
				err = DmSetRecordInfo(dbP, indexNew, &attr, NULL);
			}
		}

		// If uid was set then a unique id was passed to be used.
		if (uid != 0 && obeyUniqueIDs)
		{
			// We can't simply remove any old record using the unique id and
			// then add the new record because removing the old record could
			// move the new one.  So, we find any old record, change the new
			// record, and then remove the old one.
			indexOld = indexNew;

			// Find any record with this uid.  indexOld changes only if
			// such a record is found.
			DmFindRecordByID (dbP, uid, &indexOld);

			// Change this record to this uid.  The dirty bit is set from
			// newly making this record.
			DmSetRecordInfo(dbP, indexNew, NULL, &uid);

			// Now remove any old record.
			if (indexOld != indexNew)
			{
				DmRemoveRecord(dbP, indexOld);
			}
		}


		// Store the information necessary to navigate to the record inserted.
		DmRecordInfo(dbP, indexNew, NULL, &uniqueID, NULL);

#if EMULATION_LEVEL != EMULATION_NONE
		// Don't call PrvTransferSetGoToParams for shell commands.  Do this by seeing which
		// input function is passed - the one for shell commands or the local one for exchange.
		if (reader->appData != NULL)
#endif
			PrvTransferSetGoToParams (globals->adxtLibRef, dbP, (ExgSocketPtr)(reader->appData), uniqueID);

		;
	} //end of ErrTry


	ErrCatch(inErr)
	{
		// Throw the error after the memory is cleaned up.
		error = inErr;
	} ErrEndCatch

	// Free any temporary buffers used to store the incoming data.
#ifndef CONTACTS
	for (i=0; i < addrNumFields; i++)
	{
		if (newRecord.fields[i] != NULL)
		{
			MemPtrFree(newRecord.fields[i]);
			newRecord.fields[i] = NULL;	// clear the record
		}
	}
#endif
	
		// misformed vCard (no BEGIN:xxx): we must stop the import
	if (error == exgErrBadData)
		return false;

	// in other case (typically a memory error) we must throw up the error
	if (error != errNone)
		ErrThrow(error);

	// if no error we must inform caller to continue iff not EOF
	return ((reader->events & kPdiEOFEventMask) == 0);
}


/***********************************************************************
 *
 * FUNCTION:		TransferPreview
 *
 * DESCRIPTION:	Create a short string preview of the data coming in.
 *
 * PARAMETERS:		infoP - the preview info from the command parameter block
 *						        of the sysAppLaunchCmdExgPreview launch
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         dje    8/31/00   Created
 *
 ***********************************************************************/
void TransferPreview(UInt16 adxtRefNum, ExgPreviewInfoType *infoP)
{
	Err err;
	UInt16 pdiRefNum;
	PdiReaderType* reader;
	UDAReaderType* stream;
	Boolean loaded;
	Char *lastNameP = NULL, *firstNameP = NULL, *companyP = NULL;
	Boolean multiple = false;
	globalVars* globals = getGlobalsPtr();

	if (infoP->op == exgPreviewQuery)
	{
		infoP->types = exgPreviewShortString;
		return;
	}
	if (infoP->op != exgPreviewShortString)
	{
		infoP->error = exgErrNotSupported;
		return;
	}

	// if we have a description we don't have to parse the vObject
	if (infoP->socketP->description && *infoP->socketP->description)
	{
		StrNCopy(infoP->string, infoP->socketP->description, infoP->size - 1);
		infoP->string[infoP->size - 1] = 0;
		infoP->error = errNone;
		return;
	}

	err = ExgAccept(infoP->socketP);
	if (!err)
	{
		err = PrvTransferPdiLibLoad(globals->adxtLibRef, &pdiRefNum, &loaded);
	}
	if (!err)
	{
		stream = UDAExchangeReaderNew(infoP->socketP);
		reader = PdiReaderNew(pdiRefNum, stream, kPdiOpenParser);
		reader->appData = infoP->socketP;
		if (reader)
		{
			PdiReadProperty(pdiRefNum, reader);
			if  (reader->property != kPdiPRN_BEGIN_VCARD)
				goto ParseError;
			PdiEnterObject(pdiRefNum, reader);
			PdiDefineResizing(pdiRefNum, reader, 16, tableMaxTextItemSize);

			while (PdiReadProperty(pdiRefNum, reader) == 0)
			{
				if (reader->property == kPdiPRN_BEGIN_VCARD)
				{
					multiple = true;
					break;
				}
				switch (reader->property)
				{
				case kPdiPRN_N:
					PdiReadPropertyField(pdiRefNum, reader, &lastNameP, kPdiResizableBuffer, kPdiDefaultFields);
					PdiReadPropertyField(pdiRefNum, reader, &firstNameP, kPdiResizableBuffer, kPdiDefaultFields);
					break;
				case kPdiPRN_FN:
					// Take care of FN iff no name nor first name.
					if (lastNameP == NULL && firstNameP == NULL)
						PdiReadPropertyField(pdiRefNum, reader, &lastNameP, kPdiResizableBuffer, kPdiDefaultFields);
					break;
				case kPdiPRN_NICKNAME:
					// Take care of nickname iff no first name.
					if (firstNameP == NULL)
						PdiReadPropertyField(pdiRefNum, reader, &firstNameP, kPdiResizableBuffer, kPdiDefaultFields);
					break;
				case kPdiPRN_ORG:
					PdiReadPropertyField(pdiRefNum, reader, &companyP, kPdiResizableBuffer, kPdiConvertSemicolon);
					break;
					// ignore other properties
				}
			}

			if (multiple)
			{
				MemHandle resH = DmGetResource(strRsc, ExgMultipleDescriptionStr);
				void *desc = MemHandleLock(resH);

				StrNCopy(infoP->string, desc, infoP->size);
				infoP->string[infoP->size - 1] = chrNull;
				MemHandleUnlock(resH);
				DmReleaseResource(resH);
			}
			else
			{
				// if the company and last name fields are identical, assume company only
				if (lastNameP != NULL
					&& companyP != NULL
					&& firstNameP == NULL
					&& StrCompare(lastNameP, companyP) == 0)
				{
					MemPtrFree(lastNameP);
					lastNameP = NULL;
				}

				// write the short string preview
				if (firstNameP || lastNameP)
				{
					infoP->string[0] = chrNull;
					if (firstNameP)
						StrNCat(infoP->string, firstNameP, infoP->size);
					if (firstNameP && lastNameP)
						StrNCat(infoP->string, " ", infoP->size);
					if (lastNameP)
						StrNCat(infoP->string, lastNameP, infoP->size);
				}
				else if (companyP)
				{
					StrNCopy(infoP->string, companyP, infoP->size);
					infoP->string[infoP->size - 1] = chrNull;
				}
				else
				{
				ParseError:
					err = exgErrBadData;
				}
			}

			// clean up
			if (lastNameP)
				MemPtrFree(lastNameP);
			if (firstNameP)
				MemPtrFree(firstNameP);
			if (companyP)
				MemPtrFree(companyP);

			ExgDisconnect(infoP->socketP, err);
		}
		PdiReaderDelete(pdiRefNum, &reader);
		UDADelete(stream);
		PrvTransferPdiLibUnload(globals->adxtLibRef, pdiRefNum, loaded);
	}
	infoP->error = err;
	
}

#pragma mark -

/***********************************************************************
 *
 * FUNCTION:		PrvTransferPdiLibLoad
 *
 * DESCRIPTION:		Load Pdi library
 * PARAMETERS:		a pointer to an integer: the refNum of the libary
 *						return whether the library had to be loaded (and therefore
 *								needs to be unloaded)
 *
 * RETURNED:		An error if library is not found
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ABa		4/10/00		Created
 *
 ***********************************************************************/
Err PrvTransferPdiLibLoad(UInt16 adxtRefNum, UInt16* refNum, Boolean *loadedP)
{
	Err	error;

	// Load the Pdi library

	// Check if the library was pre-loaded (this is useful if we can
	// be called from another app via an action code and want to use an existing
	// instance of the library in case our caller has already loaded it)
	*loadedP = false;
	error = SysLibFind(kPdiLibName, refNum);
	if (error != 0)
	{
		error = SysLibLoad(sysResTLibrary, sysFileCPdiLib, refNum);
		if (! error)
			*loadedP = true;
	}
	if (error)
	{
		// We're here because the Pdi library failed to load.
		// Inform the user or do something else defensive here.
		ErrNonFatalDisplay(kPdiLibName " not found");
		return error;
	}
	error = PdiLibOpen(*refNum);

	return error;
}

/***********************************************************************
 *
 * FUNCTION:		PrvTransferPdiLibUnload
 *
 * DESCRIPTION:		Unload Pdi library
 * PARAMETERS:		The refnum of the pdi library
 *						Whether the library was loaded (and therefore needs to be unloaded)
 *
 * RETURNED:		An error if library is not found
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ABa		4/10/00		Created
 *
 ***********************************************************************/
void PrvTransferPdiLibUnload(UInt16 adxtRefNum, UInt16 refNum, Boolean loaded)
{
	if (PdiLibClose(refNum) == 0)
	{
		if (loaded)
			SysLibRemove(refNum);
	}
}


/***********************************************************************
 *
 * FUNCTION:		PrvTransferCleanFileName
 *
 * DESCRIPTION:		Remove dot characters in file name but not the least
 * PARAMETERS:		a pointer to a string
 *
 * RETURNED:		String parameter doesn't contains superfluous dot characters
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ABa		7/28/00		Created
 *
 ***********************************************************************/
void PrvTransferCleanFileName(UInt16 adxtRefNum, Char* ioFileName)
{
	Char* 	mayBeLastDotP;
	Char*  	lastDotP;
	UInt32	chrFullStopSize = TxtCharSize(chrFullStop);	
    
	// prevent NULL & empty string
	if (ioFileName == NULL || *ioFileName == 0)
		return;

	// remove dot but not the last one
	mayBeLastDotP = StrChr(ioFileName, 	chrFullStop);
	while ((lastDotP = StrChr(mayBeLastDotP + chrFullStopSize, chrFullStop)))
	{
		// remove the dot
		StrCopy(mayBeLastDotP, mayBeLastDotP + chrFullStopSize);
		mayBeLastDotP = lastDotP - chrFullStopSize;
	}
}


/***********************************************************************
 *
 * FUNCTION:		PrvTransferSetGoToParams
 *
 * DESCRIPTION:	Store the information necessary to navigate to the
 *                record inserted into the launch code's parameter block.
 *
 * PARAMETERS:		dbP        - pointer to the database to add the record to
 *					exgSocketP - parameter block passed with the launch code
 *					uniqueID   - unique id of the record inserted
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		10/17/97	Created
 *
 ***********************************************************************/
void PrvTransferSetGoToParams (UInt16 adxtRefNum, DmOpenRef dbP, ExgSocketPtr exgSocketP, UInt32 uniqueID)
{
	UInt16		recordNum;
	UInt16		cardNo;
	LocalID 	dbID;


	if (! uniqueID) return;

	DmOpenDatabaseInfo (dbP, &dbID, NULL, NULL, &cardNo, NULL);

	// The this the the first record inserted, save the information
	// necessary to navigate to the record.
	if (! exgSocketP->goToParams.uniqueID)
	{
		DmFindRecordByID (dbP, uniqueID, &recordNum);

		exgSocketP->goToCreator = CREATORID;
		exgSocketP->goToParams.uniqueID = uniqueID;
		exgSocketP->goToParams.dbID = dbID;
		exgSocketP->goToParams.dbCardNo = cardNo;
		exgSocketP->goToParams.recordNum = recordNum;
	}

	// If we already have a record then make sure the record index
	// is still correct.  Don't update the index if the record is not
	// in your the app's database.
	else if (dbID == exgSocketP->goToParams.dbID &&
			 cardNo == exgSocketP->goToParams.dbCardNo)
	{
		DmFindRecordByID (dbP, exgSocketP->goToParams.uniqueID, &recordNum);

		exgSocketP->goToParams.recordNum = recordNum;
	}
}

#endif