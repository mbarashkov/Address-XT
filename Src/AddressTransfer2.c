#include "Address.h"
#include "AddressTransfer2.h"
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

static Err	PrvTransferSendRecordTryCatch (DmOpenRef dbP, Int16 recordNum, void* recordP, UDAWriterType* media);
static Err	PrvTransferSendCategoryTryCatch (DmOpenRef dbP, UInt16 categoryNum, UDAWriterType*  media, UInt16 index);

/***********************************************************************
 *
 * FUNCTION:    TransferSendRecord
 *
 * DESCRIPTION: Send a record.
 *
 * PARAMETERS:	dbP - pointer to the database to add the record to
 * 				recordNum - the record to send
 *				prefix - the scheme with ":" suffix and optional "?" prefix
 *				noDataAlertID - alert to put up if there is nothing to send
 *
 * RETURNED:    true if the record is found and sent
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   5/9/97   Initial Revision
 *         dje     4/24/00  Don't specify target creator ID
 *         ABa     6/20/00  Integrate Pdi library
 *
 ***********************************************************************/
void TransferSendRecord (DmOpenRef dbP, Int16 recordNum, const Char * const prefix, UInt16 noDataAlertID)
{
	globalVars* globals = getGlobalsPtr();
	univAddrDBRecordType record;
	MemHandle recordH;
	MemHandle descriptionH;
	UInt16 descriptionSize = 0;
	Int16 descriptionWidth;
	Boolean descriptionFit;
	UInt16 newDescriptionSize;
	MemHandle nameH;
	MemHandle resourceH;
	Char *resourceP;
	Err error;
	ExgSocketType exgSocket;
	UDAWriterType* media;
	UInt8 schemeLength;
	Boolean hasData;

	// important to init structure to zeros...
	MemSet(&exgSocket, sizeof(exgSocket), 0);
		
	// Form a description of what's being sent.  This will be displayed
	// by the system send dialog on the sending and receiving devices.
	error = univAddrDBGetRecord (globals->adxtLibRef, dbP, recordNum, &record, &recordH);
		
	ErrNonFatalDisplayIf(error, "Can't get record");

	hasData = AddrDBRecordContainsData(&record);
	
	if (hasData)
	{
		// Figure out whether a person's name or company should be displayed.
		descriptionH = NULL;
		exgSocket.description = NULL;
		if (record.fields[univName] && record.fields[univFirstName])
			descriptionSize = sizeOf7BitChar(' ') + sizeOf7BitChar('\0');
		else
			descriptionSize = sizeOf7BitChar('\0');

		if (record.fields[univName])
			descriptionSize += StrLen(record.fields[univName]);

		if (record.fields[univFirstName])
			descriptionSize += StrLen(record.fields[univFirstName]);


		descriptionH = MemHandleNew(descriptionSize);
		if (descriptionH)
		{
			exgSocket.description = MemHandleLock(descriptionH);

			if (record.fields[univFirstName])
			{
				StrCopy(exgSocket.description, record.fields[univFirstName]);
				if (record.fields[univName])
					StrCat(exgSocket.description, " ");
			}
			else
				exgSocket.description[0] = '\0';

			if (record.fields[univName])
			{
				StrCat(exgSocket.description, record.fields[univName]);
			}
		}
		
		// Truncate the description if too long
		if (descriptionSize > 0)
		{
			// Make sure the description isn't too long.
			newDescriptionSize = descriptionSize;
			WinGetDisplayExtent(&descriptionWidth, NULL);
			FntCharsInWidth (exgSocket.description, &descriptionWidth, (Int16 *)&newDescriptionSize, &descriptionFit);

			if (newDescriptionSize > 0)
			{
				if (newDescriptionSize != descriptionSize)
				{
					exgSocket.description[newDescriptionSize] = nullChr;
					MemHandleUnlock(descriptionH);
					MemHandleResize(descriptionH, newDescriptionSize + sizeOf7BitChar('\0'));
					exgSocket.description = MemHandleLock(descriptionH);
				}
			}
			else
			{
				MemHandleFree(descriptionH);
			}
			descriptionSize = newDescriptionSize;
		}

		// Make a filename
		schemeLength = StrLen(prefix);
		if (descriptionSize > 0)
		{
			// Now make a filename from the description
			nameH = MemHandleNew(schemeLength + imcFilenameLength);
			exgSocket.name = MemHandleLock(nameH);
			StrCopy(exgSocket.name, prefix);
			StrNCat(exgSocket.name, exgSocket.description,
					schemeLength + imcFilenameLength - addrFilenameExtensionLength - sizeOf7BitChar('.'));
			StrCat(exgSocket.name, ".");
			StrCat(exgSocket.name, addrFilenameExtension);
		}
		else
		{
			// A description is needed.  Either there never was one or the first line wasn't usable.
			descriptionH = DmGetResource(strRsc, BeamDescriptionStr);
			exgSocket.description = MemHandleLock(descriptionH);

			resourceH = DmGetResource(strRsc, BeamFilenameStr);
			resourceP = MemHandleLock(resourceH);
			nameH = MemHandleNew(schemeLength + StrLen(resourceP) + 1);
			exgSocket.name = MemHandleLock(nameH);
			StrCopy(exgSocket.name, prefix);
			StrCat(exgSocket.name, resourceP);
			MemHandleUnlock(resourceH);
			DmReleaseResource(resourceH);
		}

		//ABa: remove superfluous '.' characters
		PrvTransferCleanFileName(globals->adxtLibRef, exgSocket.name);
		exgSocket.length = MemHandleSize(recordH) + 100;		// rough guess
		//exgSocket.target = sysFileCAddress;		// commented out 4/24/00 dje
		exgSocket.type = (Char *)addrMIMEType;
		error = ExgPut(&exgSocket);   // put data to destination


		// ABa: Changes to use new streaming mechanism
		media = UDAExchangeWriterNew(&exgSocket,  512);
		if (!error)
		{
			if (media)
			{
				error = PrvTransferSendRecordTryCatch(dbP, recordNum, &record, media);				
			}
			else
				error = exgMemError;
				
			ExgDisconnect(&exgSocket, error);
		}
		
		if (media)
			UDADelete(media);

		// Clean up
		if (descriptionH)
		{
			MemHandleUnlock (descriptionH);
			if (MemHandleDataStorage (descriptionH))
				DmReleaseResource(descriptionH);
			else
				MemHandleFree(descriptionH);
		}
		if (nameH)
		{
			MemHandleUnlock (nameH);
			if (MemHandleDataStorage (nameH))
				DmReleaseResource(nameH);
			else
				MemHandleFree(nameH);
		}
	}
	else
	{
		FrmAlert(noDataAlertID);
	}

	MemHandleUnlock(recordH);
	
	return;
}




/***********************************************************************
 *
 * FUNCTION:    TransferSendCategory
 *
 * DESCRIPTION: Send all visible records in a category.
 *
 * PARAMETERS:	dbP - pointer to the database to add the record to
 * 				categoryNum - the category of records to send
 *				prefix - the scheme with ":" suffix and optional "?" prefix
 *				noDataAlertID - alert to put up if there is nothing to send
 *
 * RETURNED:    true if any records are found and sent
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   5/9/97   Initial Revision
 *         dje     4/24/00  Don't specify target creator ID
 *
 ***********************************************************************/
void TransferSendCategory (DmOpenRef dbP, UInt16 categoryNum, const Char * const prefix, UInt16 noDataAlertID)
{
	globalVars* globals = getGlobalsPtr();
	Err error;
	Char description[dmCategoryLength];
	UInt16 index;
	Boolean foundAtLeastOneRecord;
	ExgSocketType exgSocket;
	UInt16 mode;
	LocalID dbID;
	UInt16 cardNo;
	Boolean databaseReopened;
	UDAWriterType* media;
	
	// If the database was opened to show secret records, reopen it to not see
	// secret records.  The idea is that secret records are not sent when a
	// category is sent.  They must be explicitly sent one by one.
	DmOpenDatabaseInfo(dbP, &dbID, NULL, &mode, &cardNo, NULL);
	if (mode & dmModeShowSecret)
	{
		dbP = DmOpenDatabase(cardNo, dbID, dmModeReadOnly);
		databaseReopened = true;
	}
	else
		databaseReopened = false;


	// important to init structure to zeros...
	MemSet(&exgSocket, sizeof(exgSocket), 0);


	// Make sure there is at least one record in the category.
	index = 0;
	foundAtLeastOneRecord = false;
	while (true)
	{
		if (DmSeekRecordInCategory(dbP, &index, 0, dmSeekForward, categoryNum) != 0)
			break;

		foundAtLeastOneRecord = DmQueryRecord(dbP, index) != 0;
		if (foundAtLeastOneRecord)
			break;


		index++;
	}


	// We should send the category because there's at least one record to send.
	if (foundAtLeastOneRecord)
	{
		// Form a description of what's being sent.  This will be displayed
		// by the system send dialog on the sending and receiving devices.
		CategoryGetName (dbP, categoryNum, description);
		exgSocket.description = description;

		// Now form a file name
		exgSocket.name = MemPtrNew(StrLen(prefix) + StrLen(description) + sizeOf7BitChar('.') + StrLen(addrFilenameExtension) + sizeOf7BitChar('\0'));
		if (exgSocket.name)
		{
			StrCopy(exgSocket.name, prefix);
			StrCat(exgSocket.name, description);
			StrCat(exgSocket.name, ".");
			StrCat(exgSocket.name, addrFilenameExtension);
		}
		// ABa: remove superfluous '.' chars
		PrvTransferCleanFileName(globals->adxtLibRef, exgSocket.name);
		exgSocket.length = 0;		// rough guess
		//exgSocket.target = sysFileCAddress;		// commented out 4/24/00 dje
		exgSocket.type = (Char *)addrMIMEType;
		error = ExgPut(&exgSocket);   // put data to destination
		media = UDAExchangeWriterNew(&exgSocket, 512);
		if (!error)
		{
			if (media)
				error = PrvTransferSendCategoryTryCatch (dbP, categoryNum, media, index);
			else
				error = exgMemError;
				
			ExgDisconnect(&exgSocket, error);
		}

		// Release file name
		if (exgSocket.name)
			MemPtrFree(exgSocket.name);

		if (media)
			UDADelete(media);
	}
	else
	{
		FrmAlert(noDataAlertID);
	}
	if (databaseReopened)
		DmCloseDatabase(dbP);

	return;
}

/************************************************************
 *
 * FUNCTION: TransferExportVCard
 *
 * DESCRIPTION: Export a record as a Imc VCard record
 *
 * PARAMETERS:
 *			dbP - pointer to the database to export the records from
 *			index - the record number to export
 *			recordP - whether the begin statement has been read
 *			outputStream - pointer to where to export the record to
 *			outputFunc - function to send output to the stream
 *			writeUniqueIDs - true to write the record's unique id
 *
 * RETURNS: nothing
 *
 *	HISTORY:
 *		08/06/97	rsf	Created by Roger Flores
 *		06/09/99	grant	Ensure that phone numbers labeled "other" aren't
 *							tagged as ";WORK" or ";HOME".
 *		08/12/99	bhall	moved category beaming code from gromit codeline
 *		10/30/99	kwk	Use TxtGetChar before calling TxtCharIsDigit.
 *         ABa     6/20/00  Integrate Pdi library
 *
 *************************************************************/

void TransferExportVCard(DmOpenRef dbP, Int16 index, univAddrDBRecordType* recordP, UInt16 pdiRefNum, PdiWriterType* writer, Boolean writeUniqueIDs)
{
	globalVars* globals = getGlobalsPtr();	
	int			i;
	UInt32			uid;
	AddrAppInfoPtr appInfoP= NULL;
	P1ContactsAppInfoPtr contactsInfoP= NULL;
	Char uidString[12];
	Boolean personOnlyAtHome = false;
	Boolean personOnlyAtWork = false;
#ifndef CONTACTS
	AddressPhoneLabels phoneLabel;
#else
	P1ContactsPhoneLabels P1phoneLabel;
#endif
	MemHandle unnamedRecordStrH;
	Char * unnamedRecordStr;
	Char* fields[7];
	
	UInt32 id;
	UInt16 addrXTDBSize;
	Char lStr[31], lStr2[31];
	DmOpenRef addrXTDB;
	UInt16 counter, m, d, y;
	DateTimeType date;
	DmRecordInfo (globals->AddrDB, index, NULL, &id, NULL);

	PdiWriteBeginObject(pdiRefNum, writer, kPdiPRN_BEGIN_VCARD);
	PdiWriteProperty(pdiRefNum, writer, kPdiPRN_VERSION);
	PdiWritePropertyValue(pdiRefNum, writer, (Char*)"2.1", kPdiWriteData);

	PdiWritePropertyStr(pdiRefNum, writer, "X-PALM", kPdiNoFields, 1);
	PdiWritePropertyValue(pdiRefNum, writer, (Char*) kVObjectVersion, kPdiWriteData);

	PdiWriteProperty(pdiRefNum, writer, kPdiPRN_N);

#ifdef CONTACTS
	if (recordP->fields[P1Contactsname] != NULL ||
		(recordP->fields[P1ContactsfirstName] != NULL))
	{
		fields[0] = recordP->fields[P1Contactsname];
		fields[1] = recordP->fields[P1ContactsfirstName];
		PdiWritePropertyFields(pdiRefNum, writer, fields, 2, kPdiWriteText);
	}
	else if (recordP->fields[P1Contactscompany] != NULL)
		// no name field, so try emitting company in N: field
	{
		PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[P1Contactscompany], kPdiWriteText);
	}
	else
		// no company name either, so emit unnamed identifier
	{
		unnamedRecordStrH = DmGetResource(strRsc, UnnamedRecordStr);
		unnamedRecordStr = MemHandleLock(unnamedRecordStrH);

		PdiWritePropertyValue(pdiRefNum, writer, unnamedRecordStr, kPdiWriteText);

		MemHandleUnlock(unnamedRecordStrH);
		DmReleaseResource(unnamedRecordStrH);
	}
	//im's
	if (recordP->fields[P1Contactschat1] != NULL)
	{
		switch(recordP->options.chatIds.chat1)
		{
			case icqChatLabel:
				PdiWritePropertyStr(pdiRefNum, writer, "X-ICQ", kPdiNoFields, 1);
				break;
			case otherChatLabel:
				PdiWritePropertyStr(pdiRefNum, writer, "X-PALMSG-IM", kPdiNoFields, 1);
				break;
			case aimChatLabel:
				PdiWritePropertyStr(pdiRefNum, writer, "X-AIM", kPdiNoFields, 1);
				break;
			case msnChatLabel:
				PdiWritePropertyStr(pdiRefNum, writer, "X-MSN", kPdiNoFields, 1);
				break;
			case yahooChatLabel:
				PdiWritePropertyStr(pdiRefNum, writer, "X-YAHOO", kPdiNoFields, 1);
				break;
		}
		PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[P1Contactschat1], kPdiWriteText);			
	}
	if (recordP->fields[P1Contactschat2] != NULL)
	{
		switch(recordP->options.chatIds.chat2)
		{
			case icqChatLabel:
				PdiWritePropertyStr(pdiRefNum, writer, "X-ICQ", kPdiNoFields, 1);
				break;
			case otherChatLabel:
				PdiWritePropertyStr(pdiRefNum, writer, "X-PALMSG-IM", kPdiNoFields, 1);
				break;
			case aimChatLabel:
				PdiWritePropertyStr(pdiRefNum, writer, "X-AIM", kPdiNoFields, 1);
				break;
			case msnChatLabel:
				PdiWritePropertyStr(pdiRefNum, writer, "X-MSN", kPdiNoFields, 1);
				break;
			case yahooChatLabel:
				PdiWritePropertyStr(pdiRefNum, writer, "X-YAHOO", kPdiNoFields, 1);
				break;
		}
		PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[P1Contactschat2], kPdiWriteText);			
	}
	
	if(recordP->pictureInfo.pictureData != NULL && ((P1ContactsDBRecordPtr)recordP)->pictureInfo.pictureSize > 0)
	{
		PdiWritePropertyStr(pdiRefNum, writer, "PHOTO;JPEG", kPdiNoFields, 1);
		//PdiWriteProperty(pdiRefNum, writer, kPdiPRN_PHOTO);
		PdiSetEncoding(pdiRefNum, writer, kPdiB64Encoding);
		PdiWritePropertyBinaryValue(pdiRefNum, writer, recordP->pictureInfo.pictureData, ((P1ContactsDBRecordPtr)recordP)->pictureInfo.pictureSize, kPdiWriteData);
	}

	for(i = 0; i < 3; i++)
	{
		if (recordP->fields[P1Contactsaddress + i*(P1Contactsaddress2-P1Contactsaddress)] != NULL ||
			recordP->fields[P1Contactscity + i*(P1Contactsaddress2-P1Contactsaddress)] != NULL ||
			recordP->fields[P1Contactsstate + i*(P1Contactsaddress2-P1Contactsaddress)] != NULL ||
			recordP->fields[P1ContactszipCode + i*(P1Contactsaddress2-P1Contactsaddress)] != NULL ||
			recordP->fields[P1Contactscountry + i*(P1Contactsaddress2-P1Contactsaddress)] != NULL)
		{
			
			unsigned int addrType;
			
			PdiWriteProperty(pdiRefNum, writer, kPdiPRN_ADR);
			
			if(i == 0)
				addrType = recordP->options.addresses.address1;
			else if(i == 1)
				addrType = recordP->options.addresses.address2;
			else if(i == 2)
				addrType = recordP->options.addresses.address3;
			
			if(addrType == P1ContactsworkAddressLabel)
		 		PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_WORK, false);
			else if(addrType == P1ContactshomeAddressLabel)
		 		PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_HOME, false);
			else if(addrType == P1ContactsotherAddressLabel)
		 		PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_DOM, false);
						
			
			fields[0] = fields[1] = NULL;
			fields[2] = recordP->fields[P1Contactsaddress + i*(P1Contactsaddress2-P1Contactsaddress)];
			fields[3] = recordP->fields[P1Contactscity + i*(P1Contactsaddress2-P1Contactsaddress)];
			fields[4] = recordP->fields[P1Contactsstate + i*(P1Contactsaddress2-P1Contactsaddress)];
			fields[5] = recordP->fields[P1ContactszipCode + i*(P1Contactsaddress2-P1Contactsaddress)];
			fields[6] = recordP->fields[P1Contactscountry + i*(P1Contactsaddress2-P1Contactsaddress)];
			PdiWritePropertyFields(pdiRefNum, writer, fields, 7, kPdiWriteText);
		}
	}
	
	if (recordP->fields[P1Contactscompany] != NULL)
	{
		PdiWriteProperty(pdiRefNum, writer, kPdiPRN_ORG);
		PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[P1Contactscompany], kPdiWriteText);
	}
	// Emit a title
	if (recordP->fields[P1Contactstitle] != NULL)
	{
		// We want to encode ';' with quoted-printable because we convert
		// non encoded ';' into '\n'. This change fixes the bug on ';' in TITLE
		PdiWritePropertyStr(pdiRefNum, writer, "TITLE", kPdiSemicolonFields, 1);
		PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[P1Contactstitle], kPdiWriteText);
	}
	// Emit a note
	if (recordP->fields[P1Contactsnote] != NULL)
	{
		PdiWriteProperty(pdiRefNum, writer, kPdiPRN_NOTE);
		PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[P1Contactsnote], kPdiWriteMultiline);
	}

	for (i = P1ContactsfirstPhoneField; i <= P1ContactslastPhoneField; i++)
	{
		if (recordP->fields[i] != NULL)
		{
			P1phoneLabel = (P1ContactsPhoneLabels) P1ContactsGetPhoneLabel(recordP, i);
			if (P1phoneLabel == homeLabel)
			{
				if (personOnlyAtWork)
				{
					personOnlyAtWork = false;
					break;
				}
				else
				{
					personOnlyAtHome = true;
				}
			}
			else if (P1phoneLabel == workLabel)
			{
				if (personOnlyAtHome)
				{
					personOnlyAtHome = false;
					break;
				}
				else
				{
					personOnlyAtWork = true;
				}
			}

		}
	}

	// Now emit the phone fields
	for (i = P1ContactsfirstPhoneField; i <= P1ContactslastPhoneField; i++)
	{
		if (recordP->fields[i] != NULL)
		{
			P1phoneLabel = (P1ContactsPhoneLabels) P1ContactsGetPhoneLabel(recordP, i);
			if (P1phoneLabel != emailLabel)
			{
				// The item
				PdiWriteProperty(pdiRefNum, writer, kPdiPRN_TEL);

				// Is this prefered?  Assume so if listed in the list view.
				if (recordP->options.phones.displayPhoneForList == i - P1ContactsfirstPhoneField)
				{
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_PREF, false);
				}

				// Add a home or work tag, unless this field is labeled "other".
				// We don't want "other" phone numbers to be tagged as ";WORK" or
				// ";HOME", because then they are interpreted as "work" or "home" numbers
				// on the receiving end.
				if (P1phoneLabel != P1ContactsotherLabel)
				{
					if (personOnlyAtHome || P1phoneLabel == P1ContactshomeLabel)
						PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_HOME, false);
					else if (personOnlyAtWork || P1phoneLabel == P1ContactsworkLabel)
						PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_WORK, false);
				}

				switch (P1phoneLabel)
				{
				case P1ContactsfaxLabel:
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_FAX, false);
					break;

				case P1ContactspagerLabel:
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_PAGER, false);
					break;

				case P1ContactsmobileLabel:
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_CELL, false);
					break;

				case P1ContactsmainLabel:
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_X_X_PALM_MAIN, false);
					
				case P1ContactsworkLabel:
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_WORK, false);
					break;
				case P1ContactshomeLabel:
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_HOME, false);
					break;

				case P1ContactsotherLabel:
					break;
				}

				PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[i], kPdiWriteText);
			}
			else
			{
				// The item
				PdiWriteProperty(pdiRefNum, writer, kPdiPRN_EMAIL);

				// Is this prefered?  Assume so if listed in the list view.
				if (recordP->options.phones.displayPhoneForList == i - P1ContactsfirstPhoneField)
				{
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_PREF, false);
				}

				if (personOnlyAtHome || P1phoneLabel == P1ContactshomeLabel)
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_HOME, false);
				else if (personOnlyAtWork || P1phoneLabel == P1ContactsworkLabel)
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_WORK, false);

				// Now try to identify the email type by it's syntax.
				// A '@' indicates a probable internet address.
				if (StrChr(recordP->fields[i], '@') == NULL)
				{
					if (TxtCharIsDigit(TxtGetChar(recordP->fields[i], 0)))
					{
						if (StrChr(recordP->fields[i], ',') != NULL)
							PdiWriteParameterStr(pdiRefNum, writer, "", "CIS");
						// We know that a hyphen is never part of a multi-byte char.
						else if (recordP->fields[i][3] == '-')
						{
							PdiWriteParameterStr(pdiRefNum, writer, "", "MCIMail");
						}
					}
				}
				else
				{
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_INTERNET, false);
				}
				PdiWritePropertyValue(pdiRefNum, writer, ((P1ContactsDBRecordPtr)recordP)->fields[i], kPdiWriteText);
			}
		}
	}

	if (recordP->fields[P1Contactswebpage] != NULL)
	{
		PdiWriteProperty(pdiRefNum, writer, kPdiPRN_URL);
		PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[P1Contactswebpage], kPdiWriteMultiline);
	}

	for (i = P1ContactsfirstRenameableLabel; i <= P1ContactslastRenameableLabel; i++)
	{
		if (recordP->fields[i] != NULL)
		{
			PdiWriteProperty(pdiRefNum, writer, kPdiPRN_X_PALM_CUSTOM);

			// Emit the custom field number
			StrIToA(uidString, i - P1ContactsfirstRenameableLabel + 1);
			PdiWriteParameterStr(pdiRefNum, writer, "", uidString);

			if (contactsInfoP == NULL)
				contactsInfoP = P1ContactsDBAppInfoGetPtr(globals->adxtLibRef, dbP);

			// Emit the custom label name if used.  This will enable smart matching.
			if (contactsInfoP->fieldLabels[i][0] != nullChr)
			{
				PdiWriteParameterStr(pdiRefNum, writer, "", contactsInfoP->fieldLabels[i]);
			}
			PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[i], kPdiWriteText);
		}
	}
#else
	if (recordP->fields[name] != NULL ||
		recordP->fields[firstName] != NULL)
	{
		fields[0] = recordP->fields[name];
		fields[1] = recordP->fields[firstName];
		PdiWritePropertyFields(pdiRefNum, writer, fields, 2, kPdiWriteText);
	}
	else if (recordP->fields[company] != NULL)
		// no name field, so try emitting company in N: field
	{
		PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[company], kPdiWriteText);
	}
	else
		// no company name either, so emit unnamed identifier
	{
		unnamedRecordStrH = DmGetResource(strRsc, UnnamedRecordStr);
		unnamedRecordStr = MemHandleLock(unnamedRecordStrH);

		PdiWritePropertyValue(pdiRefNum, writer, unnamedRecordStr, kPdiWriteText);

		MemHandleUnlock(unnamedRecordStrH);
		DmReleaseResource(unnamedRecordStrH);
	}

	if (recordP->fields[address] != NULL ||
		recordP->fields[city] != NULL ||
		recordP->fields[state] != NULL ||
		recordP->fields[zipCode] != NULL ||
		recordP->fields[country] != NULL)
	{
		PdiWriteProperty(pdiRefNum, writer, kPdiPRN_ADR);
		if (!recordP->fields[country])
		{
			PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_DOM, false);
		}
		// ABa: Add a tag HOME or WORK
		if (recordP->fields[company])
		{
			// if there's a company name, assume work address
			PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_WORK, false);
		}
		else
		{
			// If no company name, assume home address
			PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_HOME, false);
		}
		fields[0] = fields[1] = NULL;
		fields[2] = recordP->fields[address];
		fields[3] = recordP->fields[city];
		fields[4] = recordP->fields[state];
		fields[5] = recordP->fields[zipCode];
		fields[6] = recordP->fields[country];
		PdiWritePropertyFields(pdiRefNum, writer, fields, 7, kPdiWriteText);
	}

	if (recordP->fields[company] != NULL)
	{
		PdiWriteProperty(pdiRefNum, writer, kPdiPRN_ORG);
		PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[company], kPdiWriteText);
	}
	// Emit a title
	if (recordP->fields[title] != NULL)
	{
		// We want to encode ';' with quoted-printable because we convert
		// non encoded ';' into '\n'. This change fixes the bug on ';' in TITLE
		PdiWritePropertyStr(pdiRefNum, writer, "TITLE", kPdiSemicolonFields, 1);
		PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[title], kPdiWriteText);
	}
	// Emit a note
	if (recordP->fields[note] != NULL)
	{
		PdiWriteProperty(pdiRefNum, writer, kPdiPRN_NOTE);
		PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[note], kPdiWriteMultiline);
	}

	for (i = firstPhoneField; i <= lastPhoneField; i++)
	{
		if (recordP->fields[i] != NULL)
		{
			phoneLabel = (AddressPhoneLabels) GetPhoneLabel(recordP, i);
			if (phoneLabel == homeLabel)
			{
				if (personOnlyAtWork)
				{
					personOnlyAtWork = false;
					break;
				}
				else
				{
					personOnlyAtHome = true;
				}
			}
			else if (phoneLabel == workLabel)
			{
				if (personOnlyAtHome)
				{
					personOnlyAtHome = false;
					break;
				}
				else
				{
					personOnlyAtWork = true;
				}
			}

		}
	}

	// Now emit the phone fields
	for (i = firstPhoneField; i <= lastPhoneField; i++)
	{
		if (recordP->fields[i] != NULL)
		{
			phoneLabel = (AddressPhoneLabels) GetPhoneLabel(recordP, i);
			if (phoneLabel != emailLabel)
			{
				// The item
				PdiWriteProperty(pdiRefNum, writer, kPdiPRN_TEL);

				// Is this prefered?  Assume so if listed in the list view.
				if (recordP->options.phones.displayPhoneForList == i - firstPhoneField)
				{
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_PREF, false);
				}

				// Add a home or work tag, unless this field is labeled "other".
				// We don't want "other" phone numbers to be tagged as ";WORK" or
				// ";HOME", because then they are interpreted as "work" or "home" numbers
				// on the receiving end.
				if (phoneLabel != otherLabel)
				{
					if (personOnlyAtHome || phoneLabel == homeLabel)
						PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_HOME, false);
					else if (personOnlyAtWork || phoneLabel == workLabel)
						PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_WORK, false);
				}

				switch (phoneLabel)
				{
				case faxLabel:
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_FAX, false);
					break;

				case pagerLabel:
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_PAGER, false);
					break;

				case mobileLabel:
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_CELL, false);
					break;

				case mainLabel:
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_X_X_PALM_MAIN, false);
					
				case workLabel:
				case homeLabel:
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_VOICE, false);
					break;

				case otherLabel:
					break;
				}

				PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[i], kPdiWriteText);
			}
			else
			{
				// The item
				PdiWriteProperty(pdiRefNum, writer, kPdiPRN_EMAIL);

				// Is this prefered?  Assume so if listed in the list view.
				if (recordP->options.phones.displayPhoneForList == i - firstPhoneField)
				{
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_PREF, false);
				}

				if (personOnlyAtHome || phoneLabel == homeLabel)
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_HOME, false);
				else if (personOnlyAtWork || phoneLabel == workLabel)
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_WORK, false);

				// Now try to identify the email type by it's syntax.
				// A '@' indicates a probable internet address.
				if (StrChr(recordP->fields[i], '@') == NULL)
				{
					if (TxtCharIsDigit(TxtGetChar(recordP->fields[i], 0)))
					{
						if (StrChr(recordP->fields[i], ',') != NULL)
							PdiWriteParameterStr(pdiRefNum, writer, "", "CIS");
						// We know that a hyphen is never part of a multi-byte char.
						else if (recordP->fields[i][3] == '-')
						{
							PdiWriteParameterStr(pdiRefNum, writer, "", "MCIMail");
						}
					}
				}
				else
				{
					PdiWriteParameter(pdiRefNum, writer, kPdiPAV_TYPE_INTERNET, false);
				}
				PdiWritePropertyValue(pdiRefNum, writer, ((AddrDBRecordPtr)recordP)->fields[i], kPdiWriteText);
			}
		}
	}



	for (i = firstRenameableLabel; i <= lastRenameableLabel; i++)
	{
		if (recordP->fields[i] != NULL)
		{
			PdiWriteProperty(pdiRefNum, writer, kPdiPRN_X_PALM_CUSTOM);

			// Emit the custom field number
			StrIToA(uidString, i - firstRenameableLabel + 1);
			PdiWriteParameterStr(pdiRefNum, writer, "", uidString);

			if (appInfoP == NULL)
				appInfoP = AddrDBAppInfoGetPtr(globals->adxtLibRef, dbP);

			// Emit the custom label name if used.  This will enable smart matching.
			if (appInfoP->fieldLabels[i][0] != nullChr)
			{
				PdiWriteParameterStr(pdiRefNum, writer, "", appInfoP->fieldLabels[i]);
			}
			PdiWritePropertyValue(pdiRefNum, writer, recordP->fields[i], kPdiWriteText);
		}
	}
#endif

	//export birthday, if any
	CstOpenOrCreateDB(globals->adxtLibRef, ADDRXTDB_DBNAME, &addrXTDB);
	addrXTDBSize = DmNumRecords(addrXTDB);
	//DmDatabaseSize(0, (LocalID)DmFindDatabase(0, ADDRXTDB_DBNAME), &addrXTDBSize, 0, 0);
	for(counter=0;counter<addrXTDBSize;counter++)
	{
		AddrXTDBRecordType rec;
		MemHandle mH;
		UInt32 idRec, seconds;
		
		if(AddrXTDBGetRecord(globals->adxtLibRef, addrXTDB, counter, &rec, &mH) != 0)
			continue;
		
		idRec = rec.id;
		seconds = rec.bday;
		
		MemHandleUnlock(mH);
			
		if(id == idRec)
		{
			Char lStr3[3];
			TimSecondsToDateTime(seconds, &date);
			d=date.day;
			m=date.month;
			y=date.year;
			StrIToA(lStr2, y);
			if(m<10)
			{
				StrCopy(lStr, "0");
				StrIToA(lStr3, m);
				StrCat(lStr, lStr3);
			}
			else
			{
				StrIToA(lStr, m); 
			}
			StrCat(lStr2, lStr);
			
			if(d<10)
			{
				StrCopy(lStr, "0");
				StrIToA(lStr3, d);
				StrCat(lStr, lStr3);
			}
			else
			{
				StrIToA(lStr, d); 
			}
			StrCat(lStr2, lStr);
			PdiWriteProperty(pdiRefNum, writer, kPdiPRN_BDAY);
			PdiWritePropertyValue(pdiRefNum, writer, lStr2, kPdiWriteText);
	
		
			break;
		}			
	}	
	DmCloseDatabase(addrXTDB);
	
	
#ifdef CONTACTS
	if(contactsInfoP != NULL)
		MemPtrUnlock(contactsInfoP);
#else
	if (appInfoP != NULL)
		MemPtrUnlock(appInfoP);
#endif

	// Emit an unique id
	if (writeUniqueIDs)
	{
		// Get the record's unique id and append to the string.
		DmRecordInfo(dbP, index, NULL, &uid, NULL);
		StrIToA(uidString, uid);
		PdiWriteProperty(pdiRefNum, writer, kPdiPRN_UID);
		PdiWritePropertyValue(pdiRefNum, writer, uidString, kPdiWriteData);
	}

#ifdef VCARD_CATEGORIES
	// Emit category
	{
		Char description[dmCategoryLength];
		UInt16 attr;
		UInt16 category;

		// Get category name
		DmRecordInfo (dbP, index, &attr, NULL, NULL);
		category = attr & dmRecAttrCategoryMask;
		CategoryGetName(dbP, category, description);

		PdiWriteProperty(pdiRefNum, writer, kPdiPRN_X_PALM_CATEGORY);

		PdiWritePropertyValue(pdiRefNum, writer, description, kPdiWriteText);
	}

#endif

	PdiWriteEndObject(pdiRefNum, writer, kPdiPRN_END_VCARD);
	
	if (writer->error)
		ErrThrow(writer->error);
}

/***********************************************************************
 *
 * FUNCTION:    PrvTransferSendRecordTryCatch
 *
 * DESCRIPTION: Send a record.
 *
 * PARAMETERS:	dbP - pointer to the database to add the record to
 * 				recordNum - the record number to send
 * 				recordP - pointer to the record to send
 * 				outputStream - place to send the data
 *
 * RETURNED:    0 if there's no error
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  12/11/97  Initial Revision
 *         ABa    4/10/00   Load Pdi library
 *
 ***********************************************************************/
Err PrvTransferSendRecordTryCatch (DmOpenRef dbP, Int16 recordNum, void* recordP, UDAWriterType* media)
{
	globalVars* globals = getGlobalsPtr();
	Err error = 0;
	UInt16 pdiRefNum;
	Boolean loaded;

	PdiWriterType* writer;

	if ((error = PrvTransferPdiLibLoad(globals->adxtLibRef, &pdiRefNum, &loaded)))
	{
		ErrNonFatalDisplay("Can't load Pdi library");
		return error;
	}

	writer = PdiWriterNew(pdiRefNum, media, kPdiPalmCompatibility);
	if (writer)
	{

		// An error can happen anywhere during the send process.  It's easier just to
		// catch the error.  If an error happens, we must pass it into ExgDisconnect.
		// It will then cancel the send and display appropriate ui.
		ErrTry
		{
			TransferExportVCard(dbP, recordNum, recordP, pdiRefNum, writer, true);
			error = UDAWriterFlush(media);
		}


		ErrCatch(inErr)
		{
			error = inErr;
		} ErrEndCatch

		PdiWriterDelete(pdiRefNum, &writer);
	}
	else
	{
		error = exgMemError;
	}
	PrvTransferPdiLibUnload(globals->adxtLibRef, pdiRefNum, loaded);
	
	return error;
}

/***********************************************************************
 *
 * FUNCTION:    PrvTransferSendCategoryTryCatch
 *
 * DESCRIPTION: Send all visible records in a category.
 *
 * PARAMETERS:	dbP - pointer to the database to add the record to
 * 				categoryNum - the category of records to send
 * 				exgSocketP - the exchange socket used to send
 * 				index - the record number of the first record in the category to send
 *
 * RETURNED:    0 if there's no error
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   5/9/97   Initial Revision
 *         ABa     6/20/00  Integrate Pdi library
 *
 ***********************************************************************/
Err PrvTransferSendCategoryTryCatch (DmOpenRef dbP, UInt16 categoryNum, UDAWriterType*  media, UInt16 index)
{
	globalVars* globals = getGlobalsPtr();
	volatile Err error = 0;
	volatile MemHandle outRecordH = 0;
	univAddrDBRecordType outRecord;
	//P1ContactsDBRecordType outContact;
	UInt16 pdiRefNum;
	PdiWriterType* writer;
	Boolean loaded;
	
	if ((error = PrvTransferPdiLibLoad(globals->adxtLibRef, &pdiRefNum, &loaded)))
		return error;
	
	writer = PdiWriterNew(pdiRefNum, media, kPdiPalmCompatibility);
	if (writer)
	{
		// An error can happen anywhere during the send process.  It's easier just to
		// catch the error.  If an error happens, we must pass it into ExgDisconnect.
		// It will then cancel the send and display appropriate ui.
		ErrTry
		{
			// Loop through all records in the category.
			while (DmSeekRecordInCategory(dbP, &index, 0, dmSeekForward, categoryNum) == 0)
			{
				// Emit the record.  If the record is private do not emit it.
				if (univAddrDBGetRecord(globals->adxtLibRef, dbP, index, &outRecord, (MemHandle*)&outRecordH) == 0)
				{
					TransferExportVCard(dbP, index, &outRecord, pdiRefNum, writer, true);
					MemHandleUnlock(outRecordH);
				}
				index++;
			}
			error = UDAWriterFlush(media);
		}

		ErrCatch(inErr)
		{
			error = inErr;
			if (outRecordH)
				MemHandleUnlock(outRecordH);
		} ErrEndCatch

		PdiWriterDelete(pdiRefNum, &writer);
	}
	else
	{
		error = exgMemError;
	}
	PrvTransferPdiLibUnload(globals->adxtLibRef, pdiRefNum, loaded);
	
	return error;
}



