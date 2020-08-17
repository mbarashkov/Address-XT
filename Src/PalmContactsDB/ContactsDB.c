#ifdef LIBDEBUG //moved to shared library

/******************************************************************************
 *
 * Copyright (c) 1995-2002 PalmSource, Inc. All rights reserved.
 *
 * File: AddressDB.c
 *
 * Release: Palm OS 5 SDK (68K) R3.
 *
 * Description:
 *      Address Manager routines
 *
 *****************************************************************************/

#include "ContactsDB.h"
#include "AddressDB.h"
#include "../AddressRsc.h"
#include "../Address.h"
#include "../AddrTools.h"
#include "ContactsDefines.h"

#include <PalmOS.h>
#include <DateTime.h>

#include <UIResources.h>
#include <SysUtils.h>
#include <ErrorMgr.h>
#include <StringMgr.h>
#include <TextMgr.h>
#include <PalmUtils.h>
 
#include <FeatureMgr.h>
#include <AddressSortLib.h>

// The following structure doesn't really exist.  The first field
// varies depending on the data present.  However, it is convient
// (and less error prone) to use when accessing the other information.

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/

P1ContactsAppInfoPtr   P1ContactsDBAppInfoGetPtr(UInt16 adxtRefNum, DmOpenRef dbP)
{
	UInt16     cardNo;
	LocalID    dbID;
	LocalID    appInfoID;

	if (DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL))
		return NULL;
	if (DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL))
		return NULL;

	if (appInfoID == 0)
		return NULL;
	else
		return ((P1ContactsAppInfoType*) MemLocalIDToLockedPtr(appInfoID, cardNo));

}


Err PrvP1ContactsDBGetRecord(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 index, P1ContactsDBRecordPtr recordP,
				  MemHandle *recordH)
{
	globalVars* globals = getGlobalsPtr();
	PrvP1ContactsPackedDBRecord *src;

	*recordH = DmQueryRecord(dbP, index);
	
	if (*recordH == NULL)
		return dmErrIndexOutOfRange;
		
	src = (PrvP1ContactsPackedDBRecord *) MemHandleLock(*recordH);
	if (src == NULL)
		return dmErrIndexOutOfRange;

	PrvP1ContactsDBUnpack(globals->adxtLibRef, src, recordP);

	return 0;
}

Err PrvP1ContactsDBChangeRecord(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 *index, P1ContactsDBRecordPtr r, P1ContactsDBRecordFlags changedFields, UInt16 blobDirty)
{
	globalVars* globals = getGlobalsPtr();
	P1ContactsDBRecordType		src;
	MemHandle          		srcH;
	Err            			result, err;
	MemHandle          		recordH=0;
	MemHandle          		oldH;
	Int16              		i;
	P1ContactsDBRecordFlags	changes;
	
	PrvP1ContactsPackedDBRecord* 	recordP;
	UInt16				    Counter = 0;
	
	MemHandle packedH;
	
	MemMove(&changes, &changedFields, sizeof(changedFields));

	// We do not assume that r is completely valid so we get a valid
	// AddrDBRecordPtr...
	if ((result = PrvP1ContactsDBGetRecord(globals->adxtLibRef, dbP, *index, &src, &srcH)) != 0)
		return result;
	
	// Get handle to packed record, we need to figure out if there is blob data
	packedH = DmQueryRecord(dbP, *index);
	
	// And we apply the changes to it.
	MemMove(&(src.options), (&r->options), sizeof(P1ContactsOptionsType));
	
	for (i = P1ContactsfirstAddressField; i < P1ContactsaddrNumStringFields; i++)
	{
		// If the flag is set, point to the string else NULL
		if (P1ContactsGetBitMacro(changes, i) != 0)
		{
			src.fields[i] = r->fields[i];
			P1ContactsRemoveBitMacro(changes, i);
		}
		
		if ((changes.allBits == 0) && (changes.allBits2 == 0))
			break;  // no more changes
	}
	
	// Fill in birthday info only if the birth date is present.
	if(P1ContactsGetBitMacro(changes, P1ContactsbirthdayDate))
	{
		MemHandle cacheRecordH;
		DmOpenRef BDCache;
		UInt16 bdIndex;
		UInt32 size;
		UInt32 curID;
		//now update the ContactsDBIndex-PAdd
		DmRecordInfo(dbP, *index, 0, &curID, 0);
		BDCache = DmOpenDatabase(0, DmFindDatabase(0, "ContactsBDIndex-PAdd"), dmModeReadWrite); 
		if(BDCache)
		{
			UInt32 days = 0;
			UInt16 monthIndex;
			if(src.birthdayInfo.birthdayDate.month || src.birthdayInfo.birthdayDate.day)
			{
				for(monthIndex = 1; monthIndex < src.birthdayInfo.birthdayDate.month; monthIndex++)
				{
					days += DaysInMonth(monthIndex, src.birthdayInfo.birthdayDate.year);
				}
				days += src.birthdayInfo.birthdayDate.day;
				bdIndex = days - 1;
				//first, get a record for current birthday
				cacheRecordH = DmQueryRecord(BDCache, bdIndex);
				if(cacheRecordH)
				{
					UInt32 iNew;
					UInt32* p = (UInt32*) MemHandleLock(cacheRecordH);
					Boolean found = false;
					MemHandle changedH;
					void* newCacheRecordP;
					size = MemHandleSize(cacheRecordH);
					
					if(size/sizeof(UInt32) > 1)
					{
						for(i = 1; i < size/sizeof(UInt32); i++)
						{
							if(p[i] == curID)
							{
								found = true;
								break;
							}
						}
					}
					
					if(found)
					{
						changedH = DmNewHandle(BDCache, size - sizeof(UInt32));
						
						newCacheRecordP = (UInt32*)MemHandleLock(changedH);
						
						iNew = 0;
						for(i = 0; i < size/sizeof(UInt32); i++)
						{
							if(i == 0)
							{
								UInt32 num = p[i] - 1;
								DmWrite(newCacheRecordP, iNew*sizeof(UInt32), &num, sizeof(UInt32));
								//newCacheRecordP[iNew] = p[i] - 1;
								iNew ++;
							}
							else if(p[i] != curID)
							{
								DmWrite(newCacheRecordP, iNew*sizeof(UInt32), &p[i], sizeof(UInt32));
								//newCacheRecordP[iNew] = p[i];
								iNew++;
							}
						}
						
						MemPtrUnlock(newCacheRecordP);

						err = DmAttachRecord(BDCache, &bdIndex, changedH, &cacheRecordH);
						if (err)
							MemHandleFree(changedH);
					}
					
					MemHandleUnlock(cacheRecordH);
				}
			}
			//now create a new DBCache entry
			days = 0;
			for(monthIndex = 1; monthIndex < r->birthdayInfo.birthdayDate.month; monthIndex++)
			{
				days += DaysInMonth(monthIndex, r->birthdayInfo.birthdayDate.year);
			}
			days += r->birthdayInfo.birthdayDate.day;
			bdIndex = days - 1;
			cacheRecordH = DmQueryRecord(BDCache, bdIndex);
			if(cacheRecordH)
			{
				UInt32 i, iNew;
				UInt32* p = (UInt32*) MemHandleLock(cacheRecordH);
				Boolean found = false;
				MemHandle changedH;
				void* newCacheRecordP;
				size = MemHandleSize(cacheRecordH);
				
				//PRINTDEBUG(size);
				
				if(size/sizeof(UInt32) > 1)
				{
					for(i = 1; i < size/sizeof(UInt32); i++)
					{
						if(p[i] == curID)
						{
							found = true;
							break;
						}
					}
				}
				
				if(!found)
				{
					changedH = DmNewHandle(BDCache, size + sizeof(UInt32));
					
					newCacheRecordP = MemHandleLock(changedH);
					
					for(i = 0; i < size/sizeof(UInt32); i++)
					{
						if(i == 0)
						{
							UInt32 num = p[i] + 1;
							DmWrite(newCacheRecordP, i*sizeof(UInt32), &num, sizeof(UInt32));
							//newCacheRecordP[i] = p[i] + 1;
						}
						else
						{
							DmWrite(newCacheRecordP, i*sizeof(UInt32), &p[i], sizeof(UInt32));
							//newCacheRecordP[i] = p[i];
						}
					}
					DmWrite(newCacheRecordP, size, &curID, sizeof(UInt32));
							
					//newCacheRecordP[size/sizeof(UInt32)] = curID;
					
					MemPtrUnlock(newCacheRecordP);

					err = DmAttachRecord(BDCache, &bdIndex, changedH, &cacheRecordH);
					if (err)
						MemHandleFree(changedH);
				}
				
				MemHandleUnlock(cacheRecordH);
			}
			DmCloseDatabase(BDCache); 
		}
		src.birthdayInfo.birthdayDate = r->birthdayInfo.birthdayDate;
		P1ContactsRemoveBitMacro(changes, P1ContactsbirthdayDate);
		
	}
	
	if(P1ContactsGetBitMacro(changes, P1ContactsbirthdayMask))
	{
		src.birthdayInfo.birthdayMask = r->birthdayInfo.birthdayMask;
		P1ContactsRemoveBitMacro(changes, P1ContactsbirthdayMask);
	}
	
	if(P1ContactsGetBitMacro(changes, P1ContactsbirthdayPreset))
	{
		src.birthdayInfo.birthdayPreset = r->birthdayInfo.birthdayPreset;
		P1ContactsRemoveBitMacro(changes, P1ContactsbirthdayPreset);
	}
	
	if(r->pictureInfo.pictureDirty)
	{
		src.pictureInfo.pictureDirty = 1;
		src.pictureInfo.pictureSize = r->pictureInfo.pictureSize;
		src.pictureInfo.pictureData = r->pictureInfo.pictureData;		
	}

	if(blobDirty)
	{
		for(Counter = 0;Counter< r->numBlobs  ;Counter++)
		{
			src.blobs[Counter] = r->blobs[Counter]; 
		} 
		src.numBlobs = r->numBlobs;
	}

	// 1) and 2) (make a new chunk with the correct size)
	recordH = DmNewHandle(dbP, PrvP1ContactsDBUnpackedSize(globals->adxtLibRef, &src));
	if (recordH == NULL)
	{
		MemHandleUnlock(srcH);  // undo lock from AddrGetRecord above
		return dmErrMemError;
	}
	recordP = (PrvP1ContactsPackedDBRecord*) MemHandleLock(recordH);

	// 3) Copy the data from the unpacked record to the packed one.
	PrvP1ContactsDBPack(globals->adxtLibRef, &src, recordP);

	// The original record is copied and no longer needed.
	MemHandleUnlock(srcH);

	
	// The record isn't in the right position.  Move it.
	i = PrvAddrDBFindSortPosition(globals->adxtLibRef, dbP, recordP);
	DmMoveRecord(dbP, *index, i);
	if (i > *index) i--;
	*index = i;                  // return new position
	
	result = DmAttachRecord(dbP, index, recordH, &oldH);
	MemPtrUnlock(recordP);
	if (result) return result;

	MemHandleFree(oldH);
	return 0;
}

Int16 PrvP1ContactsDBUnpackedSize(UInt16 adxtRefNum, P1ContactsDBRecordPtr r)
{
	Int16 size;
	Int16   index;

	size = sizeof (PrvP1ContactsPackedDBRecord) - sizeof (char);   // correct
	for (index = P1ContactsfirstAddressField; index < P1ContactsaddrNumStringFields; index++)
	{
		 if (r->fields[index] != NULL)
			size += StrLen(r->fields[index]) + 1;
	}
	
	//calculate length for birthday info
	if(r->birthdayInfo.birthdayDate.month && r->birthdayInfo.birthdayDate.day)
	{
		size += sizeof(DateType);
		size += sizeof(AddressDBBirthdayFlags);
		if(r->birthdayInfo.birthdayMask.alarm)
		{
			size += sizeof(UInt8); //preset			
		}
	}
	if(r->pictureInfo.pictureSize && r->pictureInfo.pictureData)
	{
		size += sizeof(UInt32); //blob id
		size+=sizeof(r->pictureInfo.pictureSize); //blob size, same as sizeof(picture size)
		size+= sizeof(r->pictureInfo.pictureDirty);  		
		size+= r->pictureInfo.pictureSize;
	}

	for (index = 0; index < r->numBlobs; index++)
 	{
 		size += sizeof(r->blobs[index].creatorID);
 		size += sizeof(r->blobs[index].size);
 		size += r->blobs[index].size;
 	}
	return size;
}

void PrvP1ContactsDBPack(UInt16 adxtRefNum, P1ContactsDBRecordPtr s, void * recordP)
{
	Int32                offset;
	P1ContactsDBRecordFlags    flags;
	Int16                index;
	PrvP1ContactsPackedDBRecord*  d=0;
	UInt16                len;
	void *               srcP;
	UInt8               companyFieldOffset;
	UInt16				blobCount = 0;
 	UInt16				size = 0;

	flags.allBits = 0;
	flags.allBits2 = 0;

	DmWrite(recordP, (Int32)&d->options, &s->options, sizeof(s->options));
	offset = (Int32)&d->firstField;

	for (index = P1ContactsfirstAddressField; index < P1ContactsaddrNumStringFields; index++)
	{
		if (s->fields[index] != NULL)
		{		
			// Skip fields with empty strings
			if ((s->fields[index][0] != '\0') || (index == P1Contactsnote))
			{
				srcP = s->fields[index];
				len = StrLen((Char*)srcP) + 1;
				DmWrite(recordP, offset, srcP, len);
				offset += len;
				P1ContactsSetBitMacro(flags, index);
			}
		}
	}
	
	//write birthday info
	len = sizeof(DateType); 						
	
	//fill in birthday info only if the birth date is present
	if(s->birthdayInfo.birthdayDate.month && s->birthdayInfo.birthdayDate.day)
	{
		DmWrite(recordP, offset, &(s->birthdayInfo.birthdayDate), len);
		offset += len;
		P1ContactsSetBitMacro(flags, P1ContactsbirthdayDate);
		
		//write other birthday info only if birthdate is valid				
		//write birthday mask to determine if year is set.
		len = sizeof(AddressDBBirthdayFlags);				
		DmWrite(recordP, offset, &(s->birthdayInfo.birthdayMask), len);
		offset += len;
		P1ContactsSetBitMacro(flags, P1ContactsbirthdayMask);
			
		//set preset and text only if Date Book reminder option is set
		if(s->birthdayInfo.birthdayMask.alarm)
		{
			len = sizeof(UInt8);
			DmWrite(recordP, offset, &(s->birthdayInfo.birthdayPreset), len);
			offset += len;
			P1ContactsSetBitMacro(flags, P1ContactsbirthdayPreset);
		}	
	}
	
	//set picture blob information
	if(s->pictureInfo.pictureSize && s->pictureInfo.pictureData)
	{
		UInt32 blobId = addrPictureBlobId;
		UInt16 blobSize = 0;
		
		//set 4 byte id
		len = sizeof(blobId);	
		DmWrite(recordP, offset, &blobId, len);
		offset += len;
		
		//set 2 bytes len: blob size = 2 bytes picture dirty flag + picture size
		len = sizeof(blobSize);
		blobSize = s->pictureInfo.pictureSize + sizeof(s->pictureInfo.pictureDirty);
		DmWrite(recordP, offset, &blobSize, len);
		offset += len;		
		
		//set dirty flag and picture
		len = sizeof(s->pictureInfo.pictureDirty);	
		DmWrite(recordP, offset, &(s->pictureInfo.pictureDirty), len);
		offset += len;
		
		DmWrite(recordP, offset, (s->pictureInfo.pictureData), s->pictureInfo.pictureSize);
		offset += s->pictureInfo.pictureSize;		
		
		blobCount++;
 	}

 	// Include any other blobs we don't understand.
 	ErrNonFatalDisplayIf(blobCount + s->numBlobs > apptMaxBlobs, "Too many blobs");
 	// Include any other blobs we don't understand.
	for (index = 0; index < s->numBlobs; index++)
	{
		size = sizeof(s->blobs[index].creatorID);
		DmWrite(recordP, offset, &(s->blobs[index].creatorID), size);
		offset += size;
		size = sizeof(s->blobs[index].size);
		DmWrite(recordP, offset, &(s->blobs[index].size), size);
		offset += size;
		DmWrite(recordP, offset, s->blobs[index].content, s->blobs[index].size);
		offset += s->blobs[index].size;
	}

 	ErrNonFatalDisplayIf(offset > MemHandleSize(MemPtrRecoverHandle(recordP)), "Not enough room for packed record");

	// Set the flags indicating which fields are used
	DmWrite(recordP, (Int32)&d->flags.allBits, &flags.allBits, sizeof(flags.allBits));
	DmWrite(recordP, (Int32)&d->flags.allBits2, &flags.allBits2, sizeof(flags.allBits2));

	// Set the companyFieldOffset or clear it
	if (s->fields[P1Contactscompany] == NULL)
		companyFieldOffset = 0;
	else {
		index = 1;
		if (s->fields[P1Contactsname] != NULL)
			index += StrLen(s->fields[P1Contactsname]) + 1;
		if (s->fields[P1ContactsfirstName] != NULL)
			index += StrLen(s->fields[P1ContactsfirstName]) + 1;
		companyFieldOffset = (UInt8) index;
	}
	DmWrite(recordP, (Int32)(&d->companyFieldOffset), &companyFieldOffset, sizeof(companyFieldOffset));
}

Err P1ContactsDBNewRecord(UInt16 adxtRefNum, DmOpenRef dbP, P1ContactsDBRecordPtr r, UInt16 *index)
{
	globalVars* globals = getGlobalsPtr();
	MemHandle               recordH;
	Err                   	err;
	PrvP1ContactsPackedDBRecord*  recordP;
	UInt16                  newIndex;

	// 1) and 2) (make a new chunk with the correct size)
	recordH = DmNewHandle(dbP,(Int32) PrvP1ContactsDBUnpackedSize(globals->adxtLibRef, r));
	if (recordH == NULL)
		return dmErrMemError;

	
	// 3) Copy the data from the unpacked record to the packed one.
	recordP = (PrvP1ContactsPackedDBRecord*) MemHandleLock(recordH);
	
	PrvP1ContactsDBPack(globals->adxtLibRef, r, recordP);
	
	// Get the index
	newIndex = PrvAddrDBFindSortPosition(globals->adxtLibRef, dbP, recordP);
	
	MemPtrUnlock(recordP);

	// 4) attach in place
	err = DmAttachRecord(dbP, &newIndex, recordH, 0);
	if (err)
		MemHandleFree(recordH);
	else
		*index = newIndex;

	return err;
}

void PrvP1ContactsDBUnpack(UInt16 adxtRefNum, PrvP1ContactsPackedDBRecord *src, P1ContactsDBRecordPtr dest)
{
	globalVars* globals = getGlobalsPtr();
	Int16   				index;
	P1ContactsDBRecordFlags flags;
	char 					*p;
 	UInt16					recordSize = 0;
 	Char 					*blobStart;
 	Char 					*blobEnd;
 	UInt32					blobId;
 	UInt16 					blobSize;

	MemSet(dest, sizeof(P1ContactsDBRecordType), 0);
	
	MemMove(&(dest->options), &(src->options), sizeof(P1ContactsOptionsType));
	MemMove(&flags, &(src->flags), sizeof(P1ContactsDBRecordFlags));
	p = &src->firstField;

	for (index = P1ContactsfirstAddressField; index < P1ContactsaddrNumStringFields; index++)
	{
		// If the flag is set, point to the string else NULL
		if (P1ContactsGetBitMacro(flags, index) != 0)
		{
			dest->fields[index] = p;
			p += StrLen(p) + 1;
		}
		else
			dest->fields[index] = NULL;
	}
	
	// Unpack birthday info
	MemSet(&(dest->birthdayInfo), sizeof(BirthdayInfo), 0 );
	
	if(P1ContactsGetBitMacro(flags, P1ContactsbirthdayDate))
	{
		MemMove(&(dest->birthdayInfo.birthdayDate), p, sizeof(DateType));
		p += sizeof(DateType);
	}
	
	if(P1ContactsGetBitMacro(flags, P1ContactsbirthdayMask))
	{
		MemMove(&(dest->birthdayInfo.birthdayMask), p, sizeof(AddressDBBirthdayFlags));
		p += sizeof(AddressDBBirthdayFlags);
	}
	
	if(P1ContactsGetBitMacro(flags, P1ContactsbirthdayPreset))
	{
		dest->birthdayInfo.birthdayPreset = *((UInt8*)p);		
		p += sizeof(UInt8);
	}
	
	// Get picture info	
	dest->pictureInfo.pictureDirty = 0;
	dest->pictureInfo.pictureSize = 0;
	dest->pictureInfo.pictureData = NULL;
	dest->numBlobs = 0;	

 	// Then iterate through the blobs, ignoring any we don't understand.
 	blobStart = p;   // First blob starts where last non-blob data ends.
 	recordSize = MemPtrSize(src);
 	while (blobStart < (Char *)src + recordSize)
   	{	
   		p = blobStart;
		ErrNonFatalDisplayIf((Char *)src + recordSize - blobStart <= sizeof (blobId) + sizeof (blobSize),"Invalid blob encountered");
 		MemMove(&blobId, p, sizeof (blobId));
 		p += sizeof (blobId);
 		MemMove(&blobSize, p, sizeof (blobSize));		
 		p += sizeof (blobSize);
 		blobEnd = p + blobSize; // Blob size excludes space to store ID and size of blob.
 		if(blobSize > 1024 && blobId != addrPictureBlobId)//corrupted blob entry
 			break; 		
 		switch (blobId)
   		{
	   		case addrPictureBlobId:
			{
				UInt16  pictureDirtySize;
				pictureDirtySize = sizeof(dest->pictureInfo.pictureDirty);
				dest->pictureInfo.pictureSize = blobSize - pictureDirtySize;
				MemMove(&(dest->pictureInfo.pictureDirty), p, pictureDirtySize);
				p += pictureDirtySize;
					
				if(dest->pictureInfo.pictureSize)
					dest->pictureInfo.pictureData = p;

				p += dest->pictureInfo.pictureSize;
				break;				
			}
			
			default:
			{

				ErrNonFatalDisplayIf (dest->numBlobs >= apptMaxBlobs, "Too many blobs");
				dest->blobs[dest->numBlobs].creatorID = blobId;
				dest->blobs[dest->numBlobs].size = blobSize;
				dest->blobs[dest->numBlobs].content = p;				
	
				dest->numBlobs++;

				p = blobEnd;
				break;
	 		}		

   		}

		ErrNonFatalDisplayIf(p != blobEnd, "Blob size does not agree with contents");
		blobStart = blobEnd;    // Next blob starts where last blob ends.
		//break;
	}
	
	ErrNonFatalDisplayIf(blobStart != (Char *)src + recordSize,
	    "Last blob not aligned with end of record - don't let fields edit records directly!");
}

#endif