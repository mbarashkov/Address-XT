
/******************************************************************************
 *
 * Copyright (c) 1995-2002 PalmSource, Inc. All rights reserved.
 *
 * File: AddrTools.c
 *
 * Release: Palm OS 5 SDK (68K) R3.
 *
 * Description:
 *  This is the place for all misc functions
 *
 *****************************************************************************/


#include <PalmVMLaunch.h>
#include <PalmOS.h>
#include <HsNavCommon.h>
#include <HsExt.h>
#include <BtPrefsPnlTypes.h>

#include "AddrCustom.h"
#include "AddressDB.h"
#include "RecentDB.h"
#include "AddrTools.h"
#include "Address.h"
#include "AddressTransfer.h"
#include "AddrDefines.h"
#include "AddrPrefs.h"
#include "AddrDetails.h"
#include "UIResources.h"
#include "Address.h"
#include "globals.h"
#include "CompanyData.h"
#include "Links.h"
#include "syslog.h"

#include "Plugins/MemoMain.h"
#include "Plugins/ToDo.h"
#include "Plugins/ContactSelList.h"
#include "Plugins/DateDay.h"
#include "Plugins/DateDB.h"
#include "Plugins/Datebook.h"

#ifdef LIBDEBUG //moved to shared library

/***********************************************************************
 *
 *   Defines
 *
 ***********************************************************************/

/**
 *  Delete or archive a record.
 *  @param dbP the database reference.
 *  @param index current sort index of the record.
 *  @param archive if true archive the record else delete it.
 */
 
void deleteRecord(UInt16 refNum, DmOpenRef dbP, UInt16 index, Boolean archive) {
 if (archive)
  DmArchiveRecord(dbP, index);
   else
  DmDeleteRecord(dbP, index);

 DmMoveRecord(dbP, index, DmNumRecords(dbP));
}

Char* GetDelimiterStr(UInt16 refNum, UInt8 delimiter)
{
	switch(delimiter)
	{
		case 0:
			return DELIMITER_0_STR;
			break;
		case 1:
			return DELIMITER_1_STR;
			break;
		default:
			return DELIMITER_0_STR;
			break;	
	}
}

UInt16 GetDelimiterLen(UInt16 refNum, UInt8 delimiter)
{
	switch(delimiter)
	{
		case 0:
			return DELIMITER_0_LEN;
			break;
		case 1:
			return DELIMITER_1_LEN;
			break;
		default:
			return DELIMITER_0_LEN;
			break;	
	}
}


UInt16 GetSortByCompany(UInt16 refNum)
{
	Int16 prefsVersion;
	UInt16 prefsSize;
	UInt16 sortByCompany;
	AddrExt2PreferenceType prefs;

	prefsSize = sizeof (AddrExt2PreferenceType);
	prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefExt2ID, &prefs, &prefsSize, true);
	if (prefsVersion > noPreferenceFound)
	{
		sortByCompany=prefs.sortByCompany;
	}
	else
	{
		sortByCompany=0;//invalid value
	}
	if(sortByCompany!=PreferencesLastName && sortByCompany!=PreferencesCompanyName && sortByCompany!=PreferencesFirstLast && sortByCompany!=PreferencesCompanyFirst && sortByCompany!=PreferencesCompanyTitle && sortByCompany!=PreferencesLastTitle)
		sortByCompany=PreferencesLastName;

	return sortByCompany;
}

void CstOpenOrCreateDB(UInt16 refNum, Char* tDBName, DmOpenRef* tDBRef)
{
	UInt16 attributes;
	
	LocalID lDBID=DmFindDatabase(0, tDBName);
	if(lDBID==0)
	{
		DmCreateDatabase(0, tDBName, CREATORID, 'DATA', false);
		lDBID=DmFindDatabase(0, tDBName);
	}
	*tDBRef=DmOpenDatabase(0, lDBID, dmModeReadWrite);
	attributes=dmHdrAttrBackup;
	DmSetDatabaseInfo(0, lDBID, NULL, &attributes, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

void ToolsUnivWinDrawCharsHD(UInt16 refNum, Char* text, UInt16 length, UInt16 x, UInt16 y )
{
	globalVars* globals = getGlobalsPtr();
	//if(globals->gScreen==CLIE_320x320)
	//{
	//	HRWinDrawChars(globals->refNum, text, length, x<<1, y<<1);
	//}
	//else if(globals->gScreen==PALM_320x320)
	//{
		ToolsWinDrawCharsHD(globals->refNum, text, length, x, y);
	//}

}
/***********************************************************************
 *
 * FUNCTION:
 *	ToolsWinDrawCharsHD
 *
 * DESCRIPTION:
 *	Output high-res text using HD feature set with current font
 *
 * PARAMETERS:
 *	char* - text to output, length - char length, x, y - coords (0-320)
 *
 * RETURNED:
 *  None
 *
 * REVISION HISTORY:
 *	Name		Date		Description
 *	----		----		-----------
 *	MB			06/02/04	Initial revision
 *
 ***********************************************************************/

void	ToolsWinDrawCharsHD(UInt16 refNum, Char* text, UInt16 length, UInt16 x, UInt16 y )
{
	//first, create an offscreen window
	BitmapType *bmp;
	BitmapTypeV3 *bmpV3;
	UInt16 err, bmpSize;
	void* bmpData;
	globalVars* globals = getGlobalsPtr();
	
	WinHandle offWindow, prevWindow;
	if(length==0)
		return;
	if(globals->gScreen==PALM_320x320)
	{
		WinPushDrawState();
		bmp=BmpCreate(FntCharsWidth(text, length), FntLineHeight(), globals->gDepth, NULL, &err);
		bmpData=BmpGetBits(bmp);
		bmpSize=BmpBitsSize(bmp);
		//create bmp v3
		bmpV3=BmpCreateBitmapV3(bmp, kDensityLow, bmpData, NULL);
		if(bmp!=NULL)
		{
			offWindow=WinCreateBitmapWindow((BitmapType*)bmpV3, &err);
			if(offWindow!=0)
			{
				//success
				prevWindow=WinSetDrawWindow(offWindow);
				
				WinSetCoordinateSystem(kCoordinatesStandard);
				WinDrawChars(text, length, 0, 0);
				BmpSetDensity((BitmapType*)bmpV3, kDensityDouble);
				WinSetDrawWindow(prevWindow);
				WinDrawBitmap((BitmapType*)bmpV3, x<<1, y<<1);
				WinDeleteWindow(offWindow, false);
				
			}
		}
		WinPopDrawState();
		BmpDelete((BitmapType*)bmpV3);
		BmpDelete(bmp);
	}

}


/***********************************************************************
 *
 * FUNCTION:     ToolsSetDBAttrBits
 *
 * DESCRIPTION:  This routine sets the backup bit on the given database.
 *					  This is to aid syncs with non Palm software.
 *					  If no DB is given, open the app's default database and set
 *					  the backup bit on it.
 *
 * PARAMETERS:   dbP -	the database to set backup bit,
 *								can be NULL to indicate app's default database
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	4/1/99	Initial Revision
 *			bhall	7/9/99	made non-static for access in AddressAutoFill.c
 *
 ***********************************************************************/
void ToolsSetDBAttrBits(UInt16 refNum, DmOpenRef dbP, UInt16 attrBits)
{
	DmOpenRef localDBP;
	LocalID dbID;
	UInt16 cardNo;
	UInt16 attributes;

	// Open database if necessary. If it doesn't exist, simply exit (don't create it).
	if (dbP == NULL)
	{
		localDBP = DmOpenDatabaseByTypeCreator (addrDBType, sysFileCAddress, dmModeReadWrite);
		if (localDBP == NULL)  return;
	}
	else
	{
		localDBP = dbP;
	}

	// now set the backup bit on localDBP
	DmOpenDatabaseInfo(localDBP, &dbID, NULL, NULL, &cardNo, NULL);
	DmDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
				   NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	attributes |= attrBits;
	DmSetDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
					  NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	// close database if necessary
	if (dbP == NULL)
	{
		DmCloseDatabase(localDBP);
	}
}

Boolean PrvToolsPhoneIsANumber(UInt16 refNum, Char* phone )
{
	UInt16	digitCount = 0;
	UInt16	charCount = 0;
	WChar	ch;
	UInt32 byteLength = TxtGetNextChar( phone, 0, &ch );

	while ( ch != 0 )
	{
		charCount++;
		if ( TxtCharIsDigit( ch ) ) digitCount++;
		byteLength += TxtGetNextChar( phone, byteLength, &ch );
	}

	return ( digitCount > ( charCount / 2 ) );
}


Boolean
TxtFindStringEx(UInt16 refNum, 	const Char* inSourceStr,
				const Char* inTargetStr,
				UInt32* outPos,
				UInt16* outLength)
{
	register UInt8 firstChr = *inTargetStr;
	register const UInt8 *s0 = (UInt8 *)inSourceStr;
	register UInt8 chr = *s0;

	ErrNonFatalDisplayIf(inSourceStr == NULL, "NULL source");
	ErrNonFatalDisplayIf(inTargetStr == NULL, "NULL target");
	ErrNonFatalDisplayIf((outPos == NULL) || (outLength == NULL), "NULL result ptr");

	// We know that for Latin the match length (in source) will always be
	// the same as the target string length (string to find), so set it up
	// now such that we can use it in our search loop below.
	
	while (chr != '\0') {
		
		// If the current character matches the first character of the 
		// string we're searching for, and the character is the start of 
		// a word, check for a word macth.
		
		if ((chr == firstChr)
		&& ((s0 == (UInt8 *)inSourceStr) )) {
			UInt8 * s1 = (UInt8 *)inTargetStr;
			const UInt8 * s2 = s0;
			UInt8	c1, c2;
			Int16 result;
			
			do {
				c1 = *s1++;
				c2 = *s2++;
				result = c1 - c2;
			} while ((result == 0) && (c1 != '\0'));
		
			// If the match is identical or all the characters in inTargetStr 
			// were found then we have a match.
			
			if ((c1 == '\0') || (result == 0)) {
				*outLength = StrLen(inTargetStr);
				*outPos = s0 - (UInt8 *)inSourceStr;
				return(true);
			}
		}
		
		chr = *(++s0);
	}
	
	*outLength = 0;
	return(false);
}

SortInfo ToolsGetSortInfo(UInt16 refNum, UInt16 sortByCompany)
{
	SortInfo res;
	switch(sortByCompany)
	{
		case PreferencesCompanyName:
			res.fields[0] = univCompany;
			res.fields[1] = univName;
			res.fields[2] = univFirstName;
			res.replacement1 = -1;		
			res.replacement2 = -1;		
			break;
		case PreferencesLastName:
			res.fields[0] = univName;
			res.fields[1] = univFirstName;
			res.fields[2] = univAddressFieldsCount;
			res.replacement1 = univCompany;		
			res.replacement2 = -1;		
			break;
		case PreferencesFirstLast:
			res.fields[0] = univFirstName;
			res.fields[1] = univName;
			res.fields[2] = univAddressFieldsCount;
			res.replacement1 = univCompany;		
			res.replacement2 = -1;	
			break;	
		case PreferencesCompanyFirst:
			res.fields[0] = univCompany;
			res.fields[1] = univFirstName;
			res.fields[2] = univName;
			res.replacement1 = univCompany;		
			res.replacement2 = -1;		
			break;
		case PreferencesCompanyTitle:
			res.fields[0] = univCompany;
			res.fields[1] = univTitle;
			res.fields[2] = univName;
			res.replacement1 = univName;		
			res.replacement2 = univFirstName;		
			break;
		case PreferencesLastTitle:
			res.fields[0] = univName;
			res.fields[1] = univTitle;
			res.fields[2] = univCompany;
			res.replacement1 = univFirstName;		
			res.replacement2 = -1;		
			break;
	}
	return res;
}

/***********************************************************************
 *
 * FUNCTION:    ToolsDetermineRecordName
 *
 * DESCRIPTION: Determines an address book record's name.  The name
 * varies based on which fields exist and what the sort order is.
 *
 * PARAMETERS:  name1, name2 - first and seconds names to draw
 *              name1Length, name2Length - length of the names in chars
 *              name1Width, name2Width - width of the names when drawn
 *              nameExtent - the space the names must be drawn in
 *              *x, y - where the names are drawn
 *              shortenedFieldWidth - the width in the current font
 *
 *
 * RETURNED:    x is set after the last char drawn
 *					 Boolean - name1/name2 priority based on sortByCompany
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			roger		06/20/95	Initial Revision
 *			frigino		08/13/97	Added priority return value
 *			fpa			11/02/00	Added unnamedRecordStringH parameter in order to prevent memory leaks
 *
 ***********************************************************************/
Boolean ToolsDetermineRecordName (UInt16 refNum, void* recordP, Int16 *shortenedFieldWidth, 
Int16 *fieldSeparatorWidth, UInt16 sortByCompany, Char **name1, Int16 *name1Length, 
Int16 *name1Width, Char **name2, Int16 *name2Length, Int16 *name2Width,
Char **unnamedRecordStringPtr, MemHandle* unnamedRecordStringH, Int16 nameExtent, Boolean lowRes)
{
	Boolean ignored;
	Boolean name1HasPriority = true;
	FontID font;
	UInt8 delimiter;
	globalVars* globals = getGlobalsPtr();
	
	delimiter = globals->gDelimiter;
	
	*shortenedFieldWidth = (FntCharWidth('.') * shortenedFieldLength);
	
	*fieldSeparatorWidth = FntCharsWidth (GetDelimiterStr(globals->adxtLibRef, delimiter), GetDelimiterLen(globals->adxtLibRef, delimiter));
	*name1 = NULL;
	*name2 = NULL;
	if ( unnamedRecordStringH != NULL )
		*unnamedRecordStringH = NULL;
	if((/*globals->gScreen==CLIE_320x320 ||*/ globals->gScreen==PALM_320x320) && globals->AddrListHighRes && !lowRes)
	{
		font=FntSetFont(globals->AddrListFont);
		*shortenedFieldWidth = (FntCharWidth('.') * shortenedFieldLength) >> 1;
		
		*fieldSeparatorWidth = FntCharsWidth (GetDelimiterStr(globals->adxtLibRef, delimiter), GetDelimiterLen(globals->adxtLibRef, delimiter)) >> 1;
		FntSetFont(font);
	}	
	
	// When sorting by company, always treat name2 as priority.
	if(sortByCompany == PreferencesCompanyName)
		name1HasPriority = false;
	if(PrvDetermineRecordNameHelper(globals->adxtLibRef, sortByCompany, recordP, name1, name2) > 1 && sortByCompany == PreferencesCompanyName)
		name1HasPriority = true;
		
	if (*name1)
	{
		// Only show text from the first line in the field
		*name1Length = nameExtent;            // longer than possible
		*name1Width = nameExtent;            // wider than possible
		if(lowRes || !globals->AddrListHighRes)
		{
			FntCharsInWidth (*name1, name1Width, name1Length, &ignored); //lint !e64
		}
		else if (/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320)
		{
			font=FntSetFont(globals->AddrListFont);
			(*name1Width)<<=1;
			FntCharsInWidth(*name1, name1Width, name1Length, &ignored);
			if((*name1Width)%2)
				(*name1Width)+=2;
			(*name1Width)>>=1;
			FntSetFont(font);
		}
	}
	else
	{
		// Set the name to the unnamed string
		if (*unnamedRecordStringPtr == NULL)
		{
			*unnamedRecordStringH = DmGetResource(strRsc, UnnamedRecordStr);
			*unnamedRecordStringPtr = MemHandleLock(*unnamedRecordStringH);
		}

		// The unnamed string is assumed to be well chosen to not need clipping.
		*name1 = *unnamedRecordStringPtr;
		*name1Length = StrLen(*unnamedRecordStringPtr);
		if(lowRes || !globals->AddrListHighRes)
		{
			*name1Width = FntCharsWidth (*unnamedRecordStringPtr, *name1Length);
		}
		else if (/*globals->gScreen==CLIE_320x320 ||*/ globals->gScreen==PALM_320x320)
		{
			font=FntSetFont(globals->AddrListFont);
			*name1Width = FntCharsWidth (*unnamedRecordStringPtr, *name1Length)>>1;
			FntSetFont(font);	
		}	
	}
	if (*name2)
	{
		// Only show text from the first line in the field
		*name2Length = nameExtent;            // longer than possible
		*name2Width = nameExtent;            // wider than possible
		if(lowRes || !globals->AddrListHighRes)
		{
			FntCharsInWidth (*name2, name2Width, name2Length, &ignored);//lint !e64
		}
		else if (/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320)
		{
			font=FntSetFont(globals->AddrListFont);
			(*name2Width)<<=1;
			FntCharsInWidth(*name2, name2Width, name2Length, &ignored);
			if((*name2Width)%2)
				(*name2Width)+=2;
			(*name2Width)>>=1;
			
			FntSetFont(font);		
		}		
	}
	else
	{
		*name2Length = 0;
		*name2Width = 0;
	}
	// Return priority status
	return name1HasPriority;
}


/***********************************************************************
 *
 * FUNCTION:    ToolsDrawRecordName
 *
 * DESCRIPTION: Draws an address book record name.  It is used
 * for the list view and note view.
 *
 * PARAMETERS:  name1, name2 - first and seconds names to draw
 *              name1Length, name2Length - length of the names in chars
 *              name1Width, name2Width - width of the names when drawn
 *              nameExtent - the space the names must be drawn in
 *              *x, y - where the names are drawn
 *              shortenedFieldWidth - the width in the current font
 *
 *
 * RETURNED:    x is set after the last char drawn
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			roger		6/20/95	Initial Revision
 *			frigino	970813	Rewritten. Now includes a variable ratio for
 *									name1/name2 width allocation, a prioritization
 *									parameter, and a word break search to allow
 *									reclaiming of space from the low priority name.
 *			MB			30/01/04	Modifications for Sony Clie
 *
 ***********************************************************************/
void ToolsDrawRecordName (UInt16 refNum, Char * name1, Int16 name1Length, Int16 name1Width, Char * name2, Int16 name2Length, Int16 name2Width, Int16 nameExtent, Int16 *x, Int16 y, Int16 shortenedFieldWidth, Int16 fieldSeparatorWidth, Boolean center, 
Boolean priorityIsName1, Boolean inTitle, Boolean lowRes)
{
	globalVars* globals = getGlobalsPtr();
	Int16		name1MaxWidth;
	Int16		name2MaxWidth;
	Boolean	ignored;
	Int16		totalWidth;
	Char *	lowPriName;
	Int16		highPriNameWidth;
	Int16		lowPriNameWidth;
	FontID		font;
	UInt8 delimiter;
	
	delimiter = globals->gDelimiter;
	
	
	if(inTitle && !lowRes)
	{
		name1Width<<=1;
		name1Width+=1;
		name2Width<<=1;
		name2Width+=1;
		fieldSeparatorWidth<<=1;
		*x<<=1;
	}
	// Check if both names fit
	totalWidth = name1Width + (name2 ? fieldSeparatorWidth : 0) + name2Width;

	// If we are supposed to center the names then move in the x position
	// by the amount that centers the text
	if (center && (nameExtent > totalWidth))
	{
		*x += (nameExtent - totalWidth) / 2;
	}

	// Special case if only name1 is given
	if (name2 == NULL)
	{
		// Draw name portion that fits in extent
		if(lowRes || inTitle)
		{
			FntCharsInWidth(name1, (Int16*)&nameExtent, &name1Length, &ignored);
		}
		else if (/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320)
		{
			font=FntSetFont(globals->AddrListFont);
			nameExtent<<=1;
			FntCharsInWidth(name1, (Int16*)&nameExtent, &name1Length, &ignored);
			if(nameExtent%2)
				nameExtent+=2;
			nameExtent>>=1;
			FntSetFont(font);
		}
		if (inTitle)
			WinDrawInvertedChars(name1, name1Length, *x, y);		
		else if(lowRes)
			WinDrawChars(name1, name1Length, *x, y);
		else
		{
			ToolsUnivWinDrawCharsHD(globals->adxtLibRef, name1,  name1Length, (*x), y);
		}
		// Add width of characters actually drawn
		if(lowRes || inTitle)
		{
			*x += FntCharsWidth(name1, name1Length);
		}
		else if(/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320)
		{
			font=FntSetFont(globals->AddrListFont);
			*x += FntCharsWidth(name1, name1Length)>>1;
			FntSetFont(font);
		}
		return;
	}

	// Remove name separator width
	nameExtent -= fieldSeparatorWidth;

	// Test if both names fit
	if ((name1Width + name2Width) <= nameExtent)
	{
		name1MaxWidth = name1Width;
		name2MaxWidth = name2Width;
	}
	else
	{
		// They dont fit. One or both needs truncation
		// Establish name priorities and their allowed widths
		// Change this to alter the ratio of the low and high priority name spaces
		Int16	highPriMaxWidth = (nameExtent * 2) / 3;	// 1/3 to low and 2/3 to high
		Int16	lowPriMaxWidth = nameExtent - highPriMaxWidth;

		// Save working copies of names and widths based on priority
		if (priorityIsName1)
		{
			// Priority is name1
			//			highPriName = name1;
			highPriNameWidth = name1Width;
			lowPriName = name2;
			lowPriNameWidth = name2Width;
		}
		else
		{
			// Priority is name2
			//			highPriName = name2;
			highPriNameWidth = name2Width;
			lowPriName = name1;
			lowPriNameWidth = name1Width;
		}

		// Does high priority name fit in high priority max width?
		if (highPriNameWidth > highPriMaxWidth)
		{
			// No. Look for word break in low priority name
			Char * spaceP = StrChr(lowPriName, spaceChr);
			if (spaceP != NULL)
			{
				if(lowRes || inTitle)
				{
					lowPriNameWidth = FntCharsWidth(lowPriName, spaceP - lowPriName);
				}
				else
				{
					if(/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320)
					{
						font=FntSetFont(globals->AddrListFont);
						lowPriNameWidth = FntCharsWidth(lowPriName, spaceP - lowPriName)>>1;
						FntSetFont(font);
					}
					// Reclaim width from low pri name width to low pri max width, if smaller
					if (lowPriNameWidth < lowPriMaxWidth)
					{
						lowPriMaxWidth = lowPriNameWidth;
						// Set new high pri max width
						highPriMaxWidth = nameExtent - lowPriMaxWidth;
					}
				}
			}
		}
		else
		{
			// Yes. Adjust maximum widths
			highPriMaxWidth = highPriNameWidth;
			lowPriMaxWidth = nameExtent - highPriMaxWidth;
		}

		// Convert priority widths back to name widths
		if (priorityIsName1)
		{
			// Priority is name1
			name1Width = highPriNameWidth;
			name2Width = lowPriNameWidth;
			name1MaxWidth = highPriMaxWidth;
			name2MaxWidth = lowPriMaxWidth;
		}
		else
		{
			// Priority is name2
			name1Width = lowPriNameWidth;
			name2Width = highPriNameWidth;
			name1MaxWidth = lowPriMaxWidth;
			name2MaxWidth = highPriMaxWidth;
		}
	}

	// Does name1 fit in its maximum width?
	if (name1Width > name1MaxWidth)
	{
		// No. Draw it to max width minus the ellipsis
		name1Width = name1MaxWidth - shortenedFieldWidth;
		if (inTitle || lowRes)
		{
			FntCharsInWidth(name1, &name1Width, &name1Length, &ignored);		
		}
		else
		{
			font=FntSetFont(globals->AddrListFont);
			name1Width<<=1;
			FntCharsInWidth(name1, &name1Width, &name1Length, &ignored);
			if(name1Width%2)
				name1Width+=2;
			name1Width>>=1;
			FntSetFont(font);
		}

		if (inTitle)
			WinDrawInvertedChars(name1, name1Length, *x, y);
		else
		{
			if(lowRes)
				WinDrawChars(name1, name1Length, *x, y);
			else
				ToolsUnivWinDrawCharsHD(globals->adxtLibRef, name1, name1Length, (*x), y);
		}
		*x += name1Width;

		// Draw ellipsis
		if (inTitle)
			WinDrawInvertedChars(shortenedFieldString, shortenedFieldLength, *x, y);
		else
		{
			if(lowRes)
				WinDrawChars(shortenedFieldString, shortenedFieldLength, *x, y);
			else
				ToolsUnivWinDrawCharsHD(globals->adxtLibRef, name1, name1Length, (*x), y);			
		}
		*x += shortenedFieldWidth;
	}
	else
	{
		if(lowRes)
		{
			FntCharsInWidth(name1, &name1Width, &name1Length, &ignored);
		}
		else
		{
			// Yes. Draw name1 within its width
			if(/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320)
			{
				font=FntSetFont(globals->AddrListFont);
				name1Width<<=1;
				FntCharsInWidth(name1, &name1Width, &name1Length, &ignored);
				if(name1Width%2)
					name1Width+=2;
				name1Width>>=1;
				FntSetFont(font);
			}		
		}
		if (inTitle)
			WinDrawInvertedChars(name1, name1Length, *x, y);
		else
		{
			if(lowRes)
				WinDrawChars(name1, name1Length, *x, y);
			else
				ToolsUnivWinDrawCharsHD(globals->adxtLibRef, name1, name1Length, (*x), y);
		}
		*x += name1Width;
	}

	// Draw name separator
	if (inTitle)
		WinDrawInvertedChars(GetDelimiterStr(globals->adxtLibRef, delimiter), GetDelimiterLen(globals->adxtLibRef, delimiter), *x, y);
	else
	{
		if(lowRes)
			WinDrawChars(GetDelimiterStr(globals->adxtLibRef, delimiter), GetDelimiterLen(globals->adxtLibRef, delimiter), *x, y);
		else
			ToolsUnivWinDrawCharsHD(globals->adxtLibRef, GetDelimiterStr(globals->adxtLibRef, delimiter), GetDelimiterLen(globals->adxtLibRef, delimiter), (*x), y);
	}
	*x += fieldSeparatorWidth;

	// Draw name2 within its maximum width
	if(lowRes)
	{
		FntCharsInWidth(name2, &name2MaxWidth, &name2Length, &ignored);
	}
	else
	{
		if(/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320)
		{
			font=FntSetFont(globals->AddrListFont);
			name2MaxWidth<<=1;
			FntCharsInWidth(name2, &name2MaxWidth, &name2Length, &ignored);
			if(name2MaxWidth%2)
				name2MaxWidth+=2;
			name2MaxWidth>>=1;
			FntSetFont(font);
		}		
	}	
	if (inTitle)
		WinDrawInvertedChars(name2, name2Length, *x, y);
	else
	{
		if(lowRes)
			WinDrawChars(name2, name2Length, *x, y);
		else
			ToolsUnivWinDrawCharsHD(globals->adxtLibRef, name2, name2Length, (*x), y);
	}
	*x += name2MaxWidth;
}


FontID ToolsFntSetFont(UInt16 refNum, UInt16 libRefNum, Boolean highRes, UInt16 screen, FontID font)
{
	/*if (screen==CLIE_320x320 && !highRes)
	{
		return HRFntSetFont(libRefNum, font);
	}
	else
	{
		*/return FntSetFont(font);
	//}
}



/***********************************************************************
 *
 * FUNCTION:    ToolsDrawRecordNameAndPhoneNumber
 *
 * DESCRIPTION: Draws the name and phone number (plus which phone)
 *					 within the screen bounds passed.
 *
 * PARAMETERS:  record - record to draw
 *              bounds - bounds of the draw region
 *              phoneLabelLetters - the first letter of each phone label
 *              sortByCompany - true if the database is sorted by company
 *              unnamedRecordStringPtr - string to use for unnamed records
 *
 * RETURNED:    x coordinate where phone number starts
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/21/95		Initial Revision
 *			peter	5/09/00		Added result and eliminated destructive change to bounds
 *			fpa		11/02/00	Added unnamedRecordStringH parameter in order to prevent memory leaks
 *			MB		30/01/04	Started to modify for Sony Clie compatibility
 *
 ***********************************************************************/
Int16 ToolsDrawRecordNameAndPhoneNumber(UInt16 refNum, univAddrDBRecordType* record, RectanglePtr bounds, Char * phoneLabelLetters, UInt16 sortByCompany, Char **unnamedRecordStringPtr, MemHandle* unnamedRecordStringH,
Boolean lowRes)
{
	Int16 x, y, phoneX, widthWithoutPhoneLabel;
	UInt16 phoneLabel;
	Int16 fieldSeparatorWidth;
	Int16 shortenedFieldWidth;
	Char * name1;
	Char * name2;
	Char * phone;
	Int16 name1Length;
	Int16 name2Length;
	Int16 phoneLength;
	Int16 name1Width;
	Int16 name2Width;
	Int16 phoneWidth;
	UInt16 nameExtent;
	FontID font;
	Boolean ignored;
	Boolean name1HasPriority;
	UInt8 phoneLabelWidth;
	UInt8 delimiter;
	const Int16 phoneColumnWidth = maxPhoneColumnWidth;
	globalVars* globals = getGlobalsPtr();
	x = bounds->topLeft.x;
	y = bounds->topLeft.y;
	
	delimiter = globals->gDelimiter;
	
	if(lowRes)
	{
		phoneLabelWidth = FntCharWidth('W') - 1;		// remove the blank trailing column
	}
	else if (/*globals->gScreen==CLIE_320x320 ||*/ globals->gScreen==PALM_320x320)
	{
		font=FntSetFont(globals->AddrListFont);
		phoneLabelWidth = FntCharWidth('W');		// remove the blank trailing column
		phoneLabelWidth>>=1;
		phoneLabelWidth++;
		FntSetFont(font);
	}
	
	widthWithoutPhoneLabel = bounds->extent.x - (phoneLabelWidth + 1);

	name1HasPriority = ToolsDetermineRecordName(globals->adxtLibRef, record, &shortenedFieldWidth, &fieldSeparatorWidth, sortByCompany, &name1, &name1Length, &name1Width, &name2, &name2Length, &name2Width, unnamedRecordStringPtr, unnamedRecordStringH, 
	widthWithoutPhoneLabel, lowRes);
	
	
	phone = record->fields[univPhone1 + record->options.phones.displayPhoneForList];
	if (phone && !globals->gShowNamesOnly)
	{
		// Only show text from the first line in the field
		phoneWidth = widthWithoutPhoneLabel;
		phoneLength = phoneWidth;         // more characters than we can expect
		if(lowRes)
		{
			FntCharsInWidth (phone, &phoneWidth, &phoneLength, &ignored);
		}
		else if (/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320)
		{
			phoneWidth<<=1;
			font=FntSetFont(globals->AddrListFont);
			FntCharsInWidth (phone, &phoneWidth, &phoneLength, &ignored);
			FntSetFont(font);
			if(phoneWidth%2)
				phoneWidth+=2;
			phoneWidth>>=1;
		}
	}
	else
	{
		phoneLength = 0;
		phoneWidth = 0;
	}
	
	phoneX = bounds->topLeft.x + widthWithoutPhoneLabel - phoneWidth;
	if (widthWithoutPhoneLabel >= name1Width + (name2 ? fieldSeparatorWidth : 0) +
		name2Width + (phone ? spaceBetweenNamesAndPhoneNumbers : 0) + phoneWidth)
	{
		// we can draw it all!
		if(lowRes)
		{
			WinDrawChars(name1, name1Length, x, y);
		}
		else
		{
			ToolsUnivWinDrawCharsHD(globals->adxtLibRef, name1, name1Length, x, y);
		}
		x += name1Width;
		// Is there a second name?
		if (name2)
		{
			if (name1)
			{
				if(lowRes)
				{
					WinDrawChars(GetDelimiterStr(globals->adxtLibRef, delimiter), GetDelimiterLen(globals->adxtLibRef, delimiter), x, y);
					x += fieldSeparatorWidth;
				}
				/*else if(globals->gScreen==CLIE_320x320)
				{
					HRWinDrawChars(globals->adxtLibRef, GetDelimiterStr(globals->adxtLibRef, delimiter), GetDelimiterLen(globals->adxtLibRef, delimiter), x<<1, y<<1);
					x += fieldSeparatorWidth<<1;
				}
				*/else if(globals->gScreen==PALM_320x320)
				{
					ToolsWinDrawCharsHD(globals->adxtLibRef, GetDelimiterStr(globals->adxtLibRef, delimiter), GetDelimiterLen(globals->adxtLibRef, delimiter), x, y);
					x += fieldSeparatorWidth;
				}
			}

			// draw name2
			if(lowRes)
			{
				WinDrawChars(name2, name2Length, x, y);
				x += name2Width;
			}
			/*else if(globals->gScreen==CLIE_320x320)
			{
				HRWinDrawChars(globals->adxtLibRef, name2, name2Length, x<<1, y<<1);
				x += name2Width<<1;
			}
			*/else if(globals->gScreen==PALM_320x320)
			{
				ToolsWinDrawCharsHD(globals->adxtLibRef, name2, name2Length, x, y);
				x += name2Width;
			}
		}
		if (phone)
		{
			if(lowRes)
			{
				WinDrawChars(phone, phoneLength, phoneX, y);
			}
			else 
			{
				ToolsUnivWinDrawCharsHD(globals->adxtLibRef, phone, phoneLength, phoneX, y);
			}
		}
	}
	else
	{
		// Shortened math (970812 maf)
		if(lowRes)
			nameExtent = widthWithoutPhoneLabel - min(phoneWidth, phoneColumnWidth);
		else if(/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320)
			nameExtent = widthWithoutPhoneLabel - min(phoneWidth, phoneColumnWidth/2);
		
		// Leave some space between names and numbers if there is a phone number
		if (phone)
			nameExtent -= spaceBetweenNamesAndPhoneNumbers;

		ToolsDrawRecordName (globals->adxtLibRef, name1, name1Length, name1Width, name2, name2Length, name2Width,
						nameExtent, &x, y, shortenedFieldWidth, fieldSeparatorWidth, false,
						name1HasPriority, false, lowRes);

		if (phone)
		{
			x += spaceBetweenNamesAndPhoneNumbers;
			nameExtent = x - bounds->topLeft.x;

			// Now draw the phone number
			if (widthWithoutPhoneLabel - nameExtent >= phoneWidth)
				// We can draw it all
			{
				if(lowRes)
				{
					WinDrawChars(phone, phoneLength, phoneX, y);
				}
				else
				{
					ToolsUnivWinDrawCharsHD(globals->adxtLibRef, phone, phoneLength, phoneX, y);
				}
			}
			else
			{
				// The phone number should be right justified instead of using
				// x from above because the string printed may be shorter
				// than we expect (CharsInWidth chops off space chars).
				phoneWidth = widthWithoutPhoneLabel - nameExtent - shortenedFieldWidth;
				if(lowRes)
				{
					FntCharsInWidth(phone, &phoneWidth, &phoneLength, &ignored);
				}
				else if(/*globals->gScreen==CLIE_320x320 || */globals->gScreen==PALM_320x320)
				{
					phoneWidth<<=1;
					font=FntSetFont(globals->AddrListFont);
					FntCharsInWidth (phone, &phoneWidth, &phoneLength, &ignored);
					FntSetFont(font);
					if(phoneWidth%2)
						phoneWidth+=2;
					phoneWidth>>=1;
				}
				phoneX = bounds->topLeft.x + widthWithoutPhoneLabel - shortenedFieldWidth - phoneWidth;
				if(lowRes)
				{
					WinDrawChars(phone, phoneLength, phoneX, y);
					WinDrawChars(shortenedFieldString, shortenedFieldLength,
							 bounds->topLeft.x + widthWithoutPhoneLabel - shortenedFieldWidth, y);
				}
				else 
				{
					ToolsUnivWinDrawCharsHD(globals->adxtLibRef, phone, phoneLength, phoneX, y);
					ToolsUnivWinDrawCharsHD(globals->adxtLibRef, shortenedFieldString, shortenedFieldLength,
							 bounds->topLeft.x + widthWithoutPhoneLabel - shortenedFieldWidth, y);
				}
			}
		}
	}		
	if (phone && !globals->gShowNamesOnly)
	{
		// Draw the first letter of the phone field label
		phoneLabel = univGetPhoneLabel(record, univFirstPhoneField +
									   record->options.phones.displayPhoneForList);
		if ( (phoneLabel != otherLabel) || PrvToolsPhoneIsANumber (globals->adxtLibRef, phone) )
		{
			if(lowRes)
			{
				UInt16 lTemp;
				lTemp=bounds->topLeft.x + widthWithoutPhoneLabel + 1 + ((phoneLabelWidth - FntCharWidth(phoneLabelLetters[phoneLabel]) + 1) >> 1);
				WinDrawChars (&phoneLabelLetters[phoneLabel], 1, lTemp , y);
			}
			else 
			{
				font=FntSetFont(globals->AddrListFont);
				ToolsUnivWinDrawCharsHD (globals->adxtLibRef, &phoneLabelLetters[phoneLabel], 1, bounds->topLeft.x + widthWithoutPhoneLabel + 1 + ((phoneLabelWidth - FntCharWidth(phoneLabelLetters[phoneLabel])>>1 + 1) >> 1) , y);
				FntSetFont(font);
			}			
		}
	}
	return phoneX;	
}


/***********************************************************************
 *
 * FUNCTION:    ToolsGetLabelColumnWidth
 *
 * DESCRIPTION: Calculate the width of the widest field label plus a ':'.
 *
 * PARAMETERS:  appInfoPtr  - pointer to the app info block for field labels
 *              labelFontID - font
 *
 * RETURNED:    width of the widest label.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   1/30/98   Initial Revision
 *
 ***********************************************************************/
UInt16 ToolsGetLabelColumnWidth (UInt16 refNum, void* appInfoPtr, FontID labelFontID)
{
	Int16		i;
	UInt16	labelWidth;     // Width of a field label
	UInt16	columnWidth;    // Width of the label column (fits all label)
	FontID	curFont;
	Char *	label;
	// Calculate column width of the label column which is used by the Record View and the
	// Edit View.
	curFont = FntSetFont (labelFontID);

	columnWidth = 0;

	
#ifdef CONTACTS
	for(i = 0; i < P1ContactsaddressFieldsCount; i++)
	{
		label = ((P1ContactsAppInfoPtr)appInfoPtr)->fieldLabels[i];
		labelWidth = FntCharsWidth(label, StrLen(label));
		columnWidth = max(columnWidth, labelWidth);
	}
#else
	for (i = firstAddressField; i < lastLabel; i ++)
	{
		label = ((AddrAppInfoPtr)appInfoPtr)->fieldLabels[i];
		labelWidth = FntCharsWidth(label, StrLen(label));
		columnWidth = max(columnWidth, labelWidth);
	}
#endif
	
	columnWidth += 1 + FntCharsWidth(": ", 2);


	FntSetFont (curFont);

	if (columnWidth > maxLabelColumnWidth)
		columnWidth = maxLabelColumnWidth;

	return columnWidth;
}


/***********************************************************************
 *
 * FUNCTION:    ToolsSeekRecord
 *
 * DESCRIPTION: Given the index of a to do record, this routine scans
 *              forewards or backwards for displayable to do records.
 *
 * PARAMETERS:  indexP  - pointer to the index of a record to start from;
 *                        the index of the record sought is returned in
 *                        this parameter.
 *
 *              offset  - number of records to skip:
 *                           0 - mean seek from the current record to the
 *                             next display record, if the current record is
 *                             a display record, its index is retuned.
 *                         1 - mean seek foreward, skipping one displayable
 *                             record
 *                        -1 - means seek backwards, skipping one
 *                             displayable record
 *
 *
 * RETURNED:    false is return if a displayable record was not found.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95      Initial Revision
 *
 ***********************************************************************/
Boolean ToolsSeekRecord (UInt16 refNum, UInt16 * indexP, Int16 offset, Int16 direction)
{
	globalVars* globals = getGlobalsPtr();
	DmSeekRecordInCategory (globals->AddrDB, indexP, offset, direction, globals->CurrentCategory);
	if (DmGetLastErr()) return (false);

	return (true);
}

Boolean ToolsSeekRecordEx (UInt16 refNum, UInt16 * indexP, Int16 offset, Int16 direction)
{
	globalVars* globals = getGlobalsPtr();
	PrvAddrDBSeekVisibleRecordInCategory(globals->adxtLibRef, globals->AddrDB, indexP, offset, direction, globals->CurrentCategory, (globals->PrivateRecordVisualStatus != showPrivateRecords));
	if (DmGetLastErr()) return (false);

	return (true);
}

/***********************************************************************
 *
 * FUNCTION:    ToolsCustomAcceptBeamDialog
 *
 * DESCRIPTION: This routine uses uses a new exchange manager function to
 *				Ask the user if they want to accept the data as well as set
 *				the category to put the data in. By default all data will go
 *				to the unfiled category, but the user can select another one.
 *				We store the selected category index in the appData field of
 *				the exchange socket so we have it at the when we get the receive
 *				data launch code later.
 *
 * PARAMETERS:  dbP - open database that holds category information
 *				askInfoP - structure passed on exchange ask launchcode
 *
 * RETURNED:    Error if any
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bhall	9/7/99	Initial Revision
 *			gavin   11/9/99  Rewritten to use new ExgDoDialog function
 *
 ***********************************************************************/
Err ToolsCustomAcceptBeamDialog(UInt16 refNum, DmOpenRef dbP, ExgAskParamPtr askInfoP)
{
	ExgDialogInfoType	exgInfo;
	Err err;
	Boolean result;

	// set default category to unfiled
	exgInfo.categoryIndex = dmUnfiledCategory;
	// Store the database ref into a gadget for use by the event handler
	exgInfo.db = dbP;

	// Let the exchange manager run the dialog for us
	result = ExgDoDialog(askInfoP->socketP, &exgInfo, &err);


	if (!err && result) {

		// pretend as if user hit OK, we'll now accept the data
		askInfoP->result = exgAskOk;

		// Stuff the category index into the appData field
		askInfoP->socketP->appData = exgInfo.categoryIndex;
	} else {
		// pretend as if user hit cancel, we won't accept the data
		askInfoP->result = exgAskCancel;
	}

	return err;
}




/***********************************************************************
 *
 * FUNCTION:     ToolsInitPhoneLabelLetters
 *
 * DESCRIPTION:  Init the list of first letters of phone labels.  Used
 * in the list view and for find.
 *
 * PARAMETERS:   appInfoPtr - contains the field labels
 *                 phoneLabelLetters - array of characters (one for each phone label)
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   7/24/95   Initial Revision
 *
 ***********************************************************************/
void ToolsInitPhoneLabelLetters(UInt16 refNum, univAppInfoPtr appInfoPtr, Char * phoneLabelLetters)
{
	UInt16 i;

	// Get the first char of the phone field labels for the list view.
	for (i = 0; i < univNumPhoneLabels; i++)
	{
		phoneLabelLetters[i] = appInfoPtr->fieldLabels[i +
													   ((i < univNumPhoneLabelsStoredFirst) ? univFirstPhoneField :
														(univAddressFieldsCount - univNumPhoneLabelsStoredFirst))][0];
	}
}



/*****************************************************************************
 * Function:			ToolsGetStringResource
 *
 * Description:			Reads the string associated with the resource into the passed
 *						in string.
 *
 * Notes:				None.
 *
 * Parameters:			stringResource:	(Input) The string resource to be read.
 *						stringP:		(Output) The string that represents the resource.
 *
 * Return Value(s):		The address of the string that contains a copy of the resource string.
 ******************************************************************************/
char* ToolsGetStringResource (UInt16 refNum, UInt16 stringResource, char * stringP)
{
	MemHandle 	nameH;

	nameH = DmGetResource(strRsc, stringResource);
	StrCopy (stringP, (Char *) MemHandleLock (nameH));
	MemHandleUnlock (nameH);
	DmReleaseResource (nameH);

	return (stringP);
}



/***********************************************************************
 *
 * FUNCTION:
 *	ToolsGetLineIndexAtOffset
 *
 * DESCRIPTION:
 *	This routine gets the line index of a string at a specified offset
 *
 * PARAMETERS:
 *	textP	IN	text to parse
 *	offset	IN	char offset
 *
 * RETURNED:
 *	index of the line
 *
 * REVISION HISTORY:
 *	Name		Date		Description
 *	----		----		-----------
 *	aro			6/27/00		Initial Revision
 *
 ***********************************************************************/
UInt16	ToolsGetLineIndexAtOffset(UInt16 refNum, Char* textP, UInt16 offset )
{
	Char* nextP;
	UInt16 lineIndex = 0;

	while (offset)
	{
		nextP = StrChr(textP, chrLineFeed);
		if (nextP)
		{
			UInt16 diff = nextP - textP;
			if (offset <= diff)
				break;
			else
			{
				offset -= (diff + 1);
				textP = nextP + 1;
				lineIndex++;
			}
		}
		else
			break;
	}
	return lineIndex;
}

void FrmGlueNavObjectTakeFocus (UInt16 refNum, 
   const FormType *formP,
   UInt16 objID
)
{
	UInt32 version;
	if (FtrGet (hsFtrCreator, hsFtrIDNavigationSupported, &version) == 0)
	{
		if (version == 1)
			HsNavObjectTakeFocus (formP, objID);
		else // if version >= 2
			FrmNavObjectTakeFocus (formP, objID);
	}
}

Err FrmGlueNavDrawFocusRing (UInt16 refNum, 
   FormType *formP,
   UInt16 objectID,
   Int16 extraInfo,
   RectangleType *boundsInsideRingP,
   FrmNavFocusRingStyleEnum ringStyle,
   Boolean forceRestore
)
{
	UInt32 version;
	if (FtrGet (hsFtrCreator, hsFtrIDNavigationSupported, &version) == 0)
	{
		if (version == 1)
			return HsNavDrawFocusRing (formP, objectID, extraInfo, boundsInsideRingP, ringStyle, forceRestore);
		else // if version >= 2
			return FrmNavDrawFocusRing (formP, objectID, extraInfo, boundsInsideRingP, ringStyle, forceRestore);
	}
	return 0;
}

Err FrmGlueNavRemoveFocusRing (UInt16 refNum, 
   FormType *formP
)
{
	UInt32 version;
	if (FtrGet (hsFtrCreator, hsFtrIDNavigationSupported, &version) == 0)
	{
		if (version == 1)
			return HsNavRemoveFocusRing (formP);
		else // if version >= 2
			return FrmNavRemoveFocusRing (formP);
	}
	return 0;
}

Err FrmGlueNavGetFocusRingInfo (UInt16 refNum, 
   const FormType *formP,
   UInt16 *objectIDP,
   Int16 *extraInfoP,
   RectangleType *boundsInsideRingP,
   FrmNavFocusRingStyleEnum *ringStyleP
)
{
	UInt32 version;
	if (FtrGet (hsFtrCreator, hsFtrIDNavigationSupported, &version) == 0)
	{
		if (version == 1)
			return HsNavGetFocusRingInfo (formP, objectIDP, extraInfoP, 
			boundsInsideRingP, ringStyleP);
		else // if version >= 2
			return FrmNavGetFocusRingInfo (formP, objectIDP, extraInfoP, 
			boundsInsideRingP, ringStyleP);
	}
	return 0;
}

// enable bluetooth
// http://news.palmos.com/read/messages?id=190255
Boolean ToolsEnableBluetooth(UInt16 refNum, Boolean on)
{
	SvcCalledFromAppPBType *p;
	DmSearchStateType stateInfo;
	UInt16 cardNo;
	LocalID dbID;
	UInt32 result;
	Err err;

#ifdef DEBUG
	//LogWrite("xt_log", "tool", "EnableBT");
#endif

	// create param
	p = MemPtrNew(sizeof(SvcCalledFromAppPBType));
	p->cmd = svcCFACmdSetBtOnOff;
	p->data.bValue = on;
	MemPtrSetOwner(p, 0);

	// find prefs app
	MemSet(&stateInfo, sizeof(stateInfo), 0);
	err = DmGetNextDatabaseByTypeCreator(1, &stateInfo, 'panl', sysFileCBluetoothPanel, true, &cardNo, &dbID);
	if (err != errNone)
	{
#ifdef DEBUG
		//LogWriteNum("xt_log", "tool", err, "panl not found");
#endif
		return false;
	}

	// run prefs app
	err = SysAppLaunch(cardNo, dbID, 0, sysAppLaunchCmdPanelCalledFromApp, p, &result);
	if (err != errNone)
	{
#ifdef DEBUG
		//LogWriteNum("xt_log", "tool", err, "launch failed");
#endif
		return false;
	}

	return true; 
}

//5-Way Navigator macros
Boolean ToolsIsFiveWayNavPalmEvent(UInt16 refNum, EventPtr eventP)                                                      
{                                                                                           
    return IsFiveWayNavPalmEvent(eventP);
}

Boolean ToolsIsFiveWayNavEvent(UInt16 refNum, EventPtr eventP)														    
{                                                                                          
    return IsFiveWayNavEvent(eventP);
}

Boolean ToolsNavSelectHSPressed(UInt16 refNum, EventPtr eventP)														
{																						
      return NavSelectHSPressed(eventP);
}

Boolean ToolsNavSelectPalmPressed(UInt16 refNum, EventPtr eventP)													
{																						
	return NavSelectPalmPressed(eventP);
}

Boolean ToolsNavSelectPressed(UInt16 refNum, EventPtr eventP)														
{																						
	return NavSelectPressed(eventP);                                            
}

Boolean ToolsNavDirectionHSPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav)
{
	switch(nav)
	{
		case Left:
			return NavDirectionHSPressed(eventP, Left);
			break;	
		case Right:
			return NavDirectionHSPressed(eventP, Right);
			break;	
		case Up:
			return NavDirectionHSPressed(eventP, Up);
			break;	
		case Down:
			return NavDirectionHSPressed(eventP, Down);
			break;	
		case Select:
			return NavDirectionHSPressed(eventP, Select);
			break;	
	}
	return false;
}                                                                                       

Boolean ToolsNavDirectionPalmPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav)                                            
{
	switch(nav)
	{
		case Left:
			return NavDirectionPalmPressed(eventP, Left);
			break;	
		case Right:
			return NavDirectionPalmPressed(eventP, Right);
			break;	
		case Up:
			return NavDirectionPalmPressed(eventP, Up);
			break;	
		case Down:
			return NavDirectionPalmPressed(eventP, Down);
			break;	
		case Select:
			return NavDirectionPalmPressed(eventP, Select);
			break;	
	}
	return false;
}                                                                                       

Boolean ToolsNavDirectionPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav)                                               
{
	switch(nav)
	{
		case Left:
			return NavDirectionPressed(eventP, Left);
			break;	
		case Right:
			return NavDirectionPressed(eventP, Right);
			break;	
		case Up:
			return NavDirectionPressed(eventP, Up);
			break;	
		case Down:
			return NavDirectionPressed(eventP, Down);
			break;	
		case Select:
			return NavDirectionPressed(eventP, Select);
			break;	
	}
	return false;
}                                                                                      

Boolean ToolsNavKeyPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav)
{
	switch(nav)
	{
		case Left:
			return NavKeyPressed(eventP, Left);
			break;	
		case Right:
			return NavKeyPressed(eventP, Right);
			break;	
		case Up:
			return NavKeyPressed(eventP, Up);
			break;	
		case Down:
			return NavKeyPressed(eventP, Down);
			break;	
		case Select:
			return NavKeyPressed(eventP, Select);
			break;	
	}
	return false;
}

void LoadColors(UInt16 refNum)
{
	globalVars* globals = getGlobalsPtr();
	Int16 prefsVersion;
	UInt16 prefsSize=0;
	Boolean defaults;
	AddrColorPreferenceType prefs;
	RGBColorType color;
	PrefGetAppPreferences (CREATORID, addrPrefColorID, NULL, &prefsSize, true);
	
	globals->gForeground=UIColorGetTableEntryIndex(UIObjectForeground);
	globals->gBackground=UIColorGetTableEntryIndex(UIFieldBackground);
	globals->gFill=UIColorGetTableEntryIndex(UIFormFill);
	globals->gAlert=UIColorGetTableEntryIndex(UIAlertFill);
	globals->gDialogFill=UIColorGetTableEntryIndex(UIDialogFill);
	globals->gObjectFill=UIColorGetTableEntryIndex(UIObjectFill);
	globals->gFieldText=UIColorGetTableEntryIndex(UIFieldText);
	globals->gObjSelText=UIColorGetTableEntryIndex(UIObjectSelectedFill);
	globals->gFldSelText=UIColorGetTableEntryIndex(UIFieldTextHighlightBackground);
	globals->gColorText=UIColorGetTableEntryIndex(UIObjectForeground);
	globals->gColorBack=UIColorGetTableEntryIndex(UIFieldBackground);
	globals->gColorSel=UIColorGetTableEntryIndex(UIObjectSelectedFill);
	globals->gInactiveSel=UIColorGetTableEntryIndex(UIObjectSelectedFill);
	globals->gEachOther=UIColorGetTableEntryIndex(UIObjectSelectedFill);
	globals->gEachOtherSelected=false;

	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefColorID, &prefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	}
	if (prefsVersion != noPreferenceFound)
	{
		globals->gColorText=prefs.textColor;
		globals->gColorBack=prefs.backColor;
		globals->gColorSel=prefs.selColor;
		globals->gInactiveSel=prefs.inactiveSelColor;
		globals->gEachOther=prefs.eachOtherColor;
		globals->gEachOtherSelected=prefs.eachOther;
		defaults=prefs.defaults;
	}
	else
	{
		if(globals->gDepth < 8)
		{
			globals->gColorText=UIColorGetTableEntryIndex(UIObjectForeground);
			globals->gColorBack=UIColorGetTableEntryIndex(UIFieldBackground);
			globals->gColorSel=UIColorGetTableEntryIndex(UIObjectSelectedFill);
			globals->gEachOther=UIColorGetTableEntryIndex(UIObjectSelectedFill);
			globals->gEachOtherSelected=false;
			defaults=true;
		}
		else
		{
			prefs.textColor = globals->gColorText = 255;
			prefs.selColor = globals->gColorSel = 77;
			prefs.eachOtherColor = globals->gEachOther = 110;
			prefs.backColor = globals->gColorBack = 0;
			prefs.inactiveSelColor = globals->gInactiveSel = 222;
			prefs.eachOther = globals->gEachOtherSelected=true;	
			prefs.defaults = 0;
			prefsSize=sizeof(AddrColorPreferenceType);
	
			PrefSetAppPreferences (CREATORID, addrPrefColorID, 2, &prefs,
						   prefsSize, true);
						   prefsVersion = 2;

		}
	}
	if(!defaults)
	{
		WinIndexToRGB(globals->gColorText, &color);
		UIColorSetTableEntry(UIObjectForeground, &color);
		UIColorSetTableEntry(UIFieldText, &color);
		WinIndexToRGB(globals->gColorBack, &color);
		UIColorSetTableEntry(UIFieldBackground, &color);
		UIColorSetTableEntry(UIFormFill, &color);
		UIColorSetTableEntry(UIAlertFill, &color);
		UIColorSetTableEntry(UIDialogFill, &color);
		UIColorSetTableEntry(UIObjectFill, &color);
		WinIndexToRGB(globals->gColorSel, &color);
		
		if(prefsVersion>1)
		{
			WinIndexToRGB(globals->gColorSel, &color);
			UIColorSetTableEntry(UIObjectSelectedFill, &color);
			UIColorSetTableEntry(UIFieldTextHighlightForeground, &color);
		}
		else
		{
			globals->gColorSel=UIColorGetTableEntryIndex(UIObjectSelectedFill);
			globals->gEachOtherSelected=false;
			globals->gEachOther=UIColorGetTableEntryIndex(UIObjectSelectedFill);
		}
		
	}
}

UInt16 PrvDetermineRecordNameHelper(UInt16 refNum, UInt16 sortByCompany, univAddrDBRecordType* recordP, Char** name1, Char** name2)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 fieldNameChoiceList[3];
	UInt16 fieldNameChoice;
	SortInfo info;
	info = ToolsGetSortInfo(globals->adxtLibRef, sortByCompany);
	
	fieldNameChoice = 0;
	
	fieldNameChoiceList[0] = info.fields[0];
	fieldNameChoiceList[1] = info.fields[1];
	fieldNameChoiceList[2] = info.fields[2];
	
	while (*name1 == NULL &&
		   fieldNameChoiceList[fieldNameChoice] != univAddressFieldsCount && fieldNameChoice < 3)
	{
		*name1 = recordP->fields[fieldNameChoiceList[fieldNameChoice++]];
	}
	if (*name1 == NULL)
	{
		if (*name1 == NULL && info.replacement1 != -1)
		{
			*name1 = recordP->fields[info.replacement1];
			if(*name1==NULL && info.replacement2 != -1)
				*name1= recordP->fields[P1ContactsfirstName];
			*name2 = NULL;
		}

	}
	else
	{
		while (*name2 == NULL &&
			   fieldNameChoiceList[fieldNameChoice] != univAddressFieldsCount && fieldNameChoice < 3)
		{
			*name2 = recordP->fields[fieldNameChoiceList[fieldNameChoice++]];
		}
	}
	return fieldNameChoice;
}

#endif

globalVars* getGlobalsPtr()
{
	MemPtr globals = NULL; 
	Err err = FtrGet(CREATORID, 1, (UInt32*)&globals);
	if(err != 0)
		return NULL;
	else
		return (globalVars*)globals;
}

UInt16 GetGSIIndex(const FormType* frmP)
{
    UInt16 numObjects;UInt16 i;
    ErrFatalDisplayIf(frmP == NULL, "GetGSIObjectIndex: null Form");

    numObjects = FrmGetNumberOfObjects(frmP);
    for (i = 0; i < numObjects; i++)
    {
        if (FrmGetObjectType(frmP, i) == frmGraffitiStateObj)
        {
            return i;
        }
    }

    return frmInvalidObjectId;
}


void MoveFormObject(FormType* frmP, UInt16 id, Coord dx, Coord dy)
{
    RectangleType objBounds;
    UInt16 index = FrmGetObjectIndex(frmP, id);
    ErrFatalDisplayIf(index == frmInvalidObjectId, "object not found");

    FrmGetObjectBounds(frmP, index, &objBounds);
    FrmSetObjectPosition(frmP, index, objBounds.topLeft.x + dx, objBounds.topLeft.y + dy);
}

// move object on form (hide/show)
void MoveFormObjectHide(FormPtr frm, UInt16 id, Coord dx, Coord dy)
{
	CustomHideObjectSmp(id);
	MoveFormObject(frm, id, dx, dy);
	CustomShowObjectSmp(id);
}

// move Graffity shift indicator
void MoveFormGSI(FormPtr frm, Coord dx, Coord dy)
{
	UInt16 index;
	RectangleType objBounds;
	
	// TODO - hide/show 
	index = GetGSIIndex(frm);

	FrmGetObjectBounds(frm, index, &objBounds);
    FrmSetObjectPosition(frm, index, objBounds.topLeft.x + dx,
                             objBounds.topLeft.y + dy);
}

void CustomHideObject(FormType *tFormP, UInt16 tObjID)
{
	FrmHideObject(tFormP, FrmGetObjectIndex(tFormP, tObjID));
}
void CustomHideObjectSmp(UInt16 tObjID)
{
	CustomHideObject(FrmGetActiveForm(), tObjID);
}
void CustomShowObject(FormType *tFormP, UInt16 tObjID)
{
	FrmShowObject(tFormP, FrmGetObjectIndex(tFormP, tObjID));
}
void CustomShowObjectSmp(UInt16 tObjID)
{
	CustomShowObject(FrmGetActiveForm(), tObjID);
}

/***********************************************************************
 *
 * FUNCTION:     CstHighResInit
 *
 * DESCRIPTION:  This routine detects and inits high-res mode on SonyClie 
 *               database.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     Err - standard error code
 *
 * REVISION HISTORY:
 * 			Name	Date		Description
 * 			----	----		-----------
 * 			MB      24/01/04	Initial revision - Sony Clie 320x320 only
 *
 ***********************************************************************/
Err CstHighResInit(void)
{
	globalVars* globals = getGlobalsPtr();
	Err error = 0;
	UInt16 version=0;
	UInt32 width,height,depth;
	Boolean color;
	UInt32 attr;
	UInt32 versionPalm;
	globals->gScreen=PALM_160x160;
	//try to init Palm High-Density API
	error = FtrGet(sysFtrCreator, sysFtrNumWinVersion, &versionPalm);
	if(versionPalm>=4)
	{
		//HighRes feature set is present supported
		WinScreenGetAttribute(winScreenDensity, &attr);
		if (attr == kDensityDouble)
		{
			globals->gScreen=PALM_320x320;
			WinScreenGetAttribute(winScreenDepth, &attr);
			globals->gDepth=attr;	
			WinScreenMode(winScreenModeGetSupportedDepths, 0, 0, &depth, 0);
			if(error==sysErrParamErr)
			{
				return error;
			}
			if(depth & 32768)
				depth=16;
			else if(depth&128)
				depth=8;
			else if(depth&8)
				depth=4;
			else if(depth&2)
				depth=2;
			else depth=1;
			
			WinScreenMode(winScreenModeSet, NULL, NULL, &depth, NULL);
			
			return error;
		}					
	}
	return error;	
}

UInt16 GetWindowHeight()
{
	RectangleType r;
	FormPtr frmP = FrmGetActiveForm ();
	WinHandle winH=FrmGetWindowHandle(frmP);
	WinGetBounds(winH, &r);
	return r.extent.y;
}

UInt16 GetWindowWidth()
{
	RectangleType r;
	FormPtr frmP = FrmGetActiveForm ();
	WinHandle winH=FrmGetWindowHandle(frmP);
	WinGetBounds(winH, &r);
	return r.extent.x;
}

void ToolsChangeCategory (UInt16 category)
{
	globalVars* globals = getGlobalsPtr();
	globals->CurrentCategory = category;
	globals->TopVisibleRecord = 0;
	globals->CurrentRecord = noRecord;
}

void CustomSetCtlLabelPtr(const FormType *tFormP, UInt16 tObjID, Char *tText)
{
	CtlSetLabel((ControlType*)FrmGetObjectPtr(tFormP, FrmGetObjectIndex(tFormP, tObjID)), tText);
}

void CustomSetCtlLabelPtrSmp(UInt16 tObjID, Char *tText)
{
	CustomSetCtlLabelPtr(FrmGetActiveForm(), tObjID, tText);
}

void* CustomGetObjectPtr(const FormType *tFormP, UInt16 tObjID)
{
	return (void*)FrmGetObjectPtr(tFormP, FrmGetObjectIndex(tFormP, tObjID));
}

void* CustomGetObjectPtrSmp(UInt16 tObjID)
{
	return (void*)CustomGetObjectPtr(FrmGetActiveForm(), tObjID);
}

Boolean	ToolsIsDialerPresent( void )
{
	globalVars* globals = getGlobalsPtr();
	SysNotifyParamType param;
	HelperNotifyEventType details;
	HelperNotifyValidateType validate;

	if(globals->gDeviceFlags.bits.treo)
	{
		globals->DialerPresent = true;
		globals->DialerPresentChecked = true;
	}
	else if (!globals->DialerPresentChecked)
	{
		/*if(globals->gKyocera)
		{
			globals->DialerPresent = true;
			globals->DialerPresentChecked = true;
			return globals->DialerPresent;
		}*/		
		
		param.notifyType = sysNotifyHelperEvent;
		param.broadcaster = sysFileCAddress;
		param.notifyDetailsP = &details;
		param.handled = false;

		details.version = kHelperNotifyCurrentVersion;
		details.actionCode = kHelperNotifyActionCodeValidate;
		details.data.validateP = &validate;

		validate.serviceClassID = kHelperServiceClassIDVoiceDial;
		validate.helperAppID = 0;

		SysNotifyBroadcast(&param);
		if (param.handled)
			globals->DialerPresent = true;
		else
			globals->DialerPresent = false;

		globals->DialerPresentChecked = true;
	}
	return globals->DialerPresent;
}

Char* CustomFldGetTextPtr(FormType *tFormP, UInt16 tFieldID)
{
	return (Char*)(FldGetTextPtr(FrmGetObjectPtr(tFormP, FrmGetObjectIndex(tFormP, tFieldID))));
}

Char* CustomFldGetTextPtrSmp(UInt16 tObjID)
{
	return (CustomFldGetTextPtr(FrmGetActiveForm(), tObjID));
}

void CustomEditableFldSetTextPtr(FormType *tFormPtr, UInt16 tFieldID, Char* tText)
{
	MemHandle lHandle, lOldHandle;
	MemPtr lPtr;
	FieldType *lField;
	lField=(FieldType*)CustomGetObjectPtr(tFormPtr, tFieldID);
	lOldHandle=FldGetTextHandle(lField);
	lHandle=MemHandleNew(StrLen(tText)+1);
	lPtr=MemHandleLock(lHandle);
	MemMove(lPtr, tText, StrLen(tText)+1);
	FldSetTextHandle(lField, lHandle);
	if(lOldHandle)
		MemHandleFree(lOldHandle);
	MemHandleUnlock(lHandle);
	return;
}

void CustomEditableFldSetTextPtrSmp(UInt16 tFieldID, Char* tText)
{
	CustomEditableFldSetTextPtr(FrmGetActiveForm(), tFieldID, tText);
}

void SetDefaultConnectOptions(AddrDialOptionsPreferenceType* pPrefs)
{
	UInt16 i;
	globalVars* globals = getGlobalsPtr();
	pPrefs->dialPrefixActive = false;
	pPrefs->dialPrefix[0] = 0;
	for(i = 0; i < connectoptions_num; i++)
	{
		pPrefs->creatorID[i] = globals->ContactSettings[i].crIDs[0];
	}
}

void FillConnectOptions()
{
	globalVars* globals = getGlobalsPtr();
	
	UInt32 serviceClassID[connectoptions_num] = 
	{
		kHelperServiceClassIDVoiceDial,
		kHelperServiceClassIDSMS,
		kHelperServiceClassIDEMail,
		'mapH',
		'webH',
		'imsH'
	};
	
	UInt16 count, i, optionIndex = 0;
	HelperNotifyEnumerateListType *nodeP, *oldNodeP;
   	SysNotifyParamType param; 
   	HelperNotifyEventType details; 
  	
	globals->listID[0] = ConnectOptionsDialList;
	globals->listID[1] = ConnectOptionsSMSList;
	globals->listID[2] = ConnectOptionsEmailList;
	globals->listID[3] = ConnectOptionsMapList;
	globals->listID[4] = ConnectOptionsWebList;
	globals->listID[5] = ConnectOptionsIMList;
		
  	MemSet(&param,sizeof(SysNotifyParamType),0);
  	param.notifyType = sysNotifyHelperEvent;
	param.broadcaster = 0;
	param.notifyDetailsP = &details;
	param.handled = false;

 	MemSet(&details,sizeof(HelperNotifyEventType),0);
	details.version = kHelperNotifyCurrentVersion;
	details.actionCode = kHelperNotifyActionCodeEnumerate;
	details.data.executeP = 0;
	details.data.enumerateP = 0;

	SysNotifyBroadcast(&param);
		
	for(i = 0; i < connectoptions_num; i++)
	{
		Int16 j;
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
	
	for(optionIndex = 0; optionIndex < connectoptions_num; optionIndex ++)
  	{
		Boolean googleMaps = false;
		nodeP = details.data.enumerateP;
		count = 0;
		while(nodeP)
		{
			if(nodeP->serviceClassID == serviceClassID[optionIndex])
				count++;
			nodeP = nodeP->nextP;
		}
		if(optionIndex == connectoptions_map)
		{
			DmSearchStateType stateInfo;
			UInt16 cardNo;
        	LocalID dbID;
			if (errNone == DmGetNextDatabaseByTypeCreator(true, &stateInfo,  
				sysFileTApplication, googleMapCreatorID, false, &cardNo, &dbID) && dbID)
        	{
				googleMaps = true;
				count++;
			}	
		}	
		
		globals->ContactSettings[optionIndex].strings = MemPtrNew(sizeof(Char*)*(count + 1));
		globals->ContactSettings[optionIndex].crIDs = MemPtrNew(sizeof(UInt32)*(count + 1));
		
		nodeP = details.data.enumerateP;
		i = 0;
		while(nodeP)
		{
			if(nodeP->serviceClassID == serviceClassID[optionIndex])
			{
				globals->ContactSettings[optionIndex].strings[i] = MemPtrNew(StrLen(nodeP->helperAppName) + 1);
				globals->ContactSettings[optionIndex].crIDs[i] = nodeP->helperAppID;
				StrCopy(globals->ContactSettings[optionIndex].strings[i], nodeP->helperAppName);
				i++;
			}
			nodeP = nodeP->nextP;
		}
		if(googleMaps)
		{
			globals->ContactSettings[optionIndex].strings[i] = MemPtrNew(StrLen("Google Maps") + 1);
			globals->ContactSettings[optionIndex].crIDs[i] =googleMapCreatorID;
			StrCopy(globals->ContactSettings[optionIndex].strings[i], "Google Maps");
			i++;
		}
		globals->ContactSettings[optionIndex].num = count+1;
		globals->ContactSettings[optionIndex].strings[count] = MemPtrNew(StrLen("- None -") + 1);
		globals->ContactSettings[optionIndex].crIDs[count] = 0;
		StrCopy(globals->ContactSettings[optionIndex].strings[count], "- None -");
	}
	nodeP = details.data.enumerateP;
	while(nodeP)
	{
		oldNodeP = nodeP;
		nodeP = nodeP->nextP;
		MemPtrFree(oldNodeP);		
	}
}