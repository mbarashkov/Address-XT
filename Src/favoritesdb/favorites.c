// PhoneFavoritesDB (Treo600) & PhoneFavorites2DB (Treo650) support

/*
 sample usage:
 	
 	DbOpenRef fav_db;
 	
 	if(gTreo)
	{
		fav_db = favorites_open(false);
		if(fav_db)
		{
			// 4 - cell number (from 0), 'x' - hotkey
			favorites_write(fav_db, 4, APP_DBNAME, CREATORID, 'x');
			DmCloseDatabase(fav_db);
		}
	}
	
*/

#include <PalmTypes.h>
#include <DataMgr.h>
#include <MemoryMgr.h>
#include <StringMgr.h>

#include "../syslog.h"

#include "favorites.h"

//--

#define FAVORITES_DB "PhoneFavoritesDB"
#define FAVORITES_DB2 "PhoneFavorites2DB"

//-- 

enum fav_opcode
{
	FAV_APP_LAUNCH,
	FAV_SPEED_DIAL,
	FAV_MESSAGE,
	FAV_URL,
	// only for version 2, in Treo650:
	FAV_EMAIL = 7,
	FAV_SYSTEM = 8
};

// header - 1st 8bytes
struct favorites 
{
	UInt8 reserved1;
	// cell number
	UInt8 cell;
	// phone status: 00 - normal, 80 - need phone to be on
	UInt8 phone_status;
	// lock status, |02 - disable changing type, |10 - disable changing hotkey
	UInt8 lock;
	UInt8 reserved2;
	// operations
	enum fav_opcode operation;
	UInt8 reserved3;
	Char hot_key;
	// next part (body) - different for all types
};

//-- 

// open database
DmOpenRef favorites_open(Boolean readonly)
{
	LocalID loc_id;
	DmOpenRef db;
	
	loc_id = DmFindDatabase(0, FAVORITES_DB);
	if(loc_id == 0)
	{
		loc_id = DmFindDatabase(0, FAVORITES_DB2);
		if(loc_id == 0)
		{
#ifdef DEBUG
			LogWrite("xt_log", "fav", "not found");
#endif
			return 0;
		}
#ifdef DEBUG
			LogWrite("xt_log", "fav", "found v2");
#endif
	}
#ifdef DEBUG
	else
		LogWrite("xt_log", "fav", "found v1");
#endif

	db = DmOpenDatabase(0, loc_id, readonly ? dmModeReadOnly : dmModeReadWrite );
	
	// why did the simulator treo600 show error 'records left locked in unproteced db' ???
	// db = DmOpenDatabase(0, loc_id, dmModeReadOnly);
	// DmCloseDatabase(db);
	
	return db;
}


// find db record-num for Nth record in favorites
Int16 find_record(DmOpenRef db, UInt8 position, Boolean *create_new)
{
	UInt16 num_records, i;
	MemHandle rec_h;
	struct favorites *rec;
	
	num_records = DmNumRecordsInCategory(db, dmAllCategories );
	
	*create_new = false;
	for(i = 0; i < num_records; i++)
	{
		rec_h = DmQueryRecord(db, i);
		if(rec_h == NULL) break;
		rec = (struct favorites *)MemHandleLock(rec_h);
		if(rec->cell >= position)
		{
			MemHandleUnlock(rec_h);
			//DmReleaseRecord(db, i, false);
#ifdef DEBUG
	LogWriteNum("xt_log", "fav", i, "found recN");
#endif
			if(rec->cell > position)
			{
#ifdef DEBUG
				LogWriteNum("xt_log", "fav", i, "new");
#endif
				*create_new = true;
			}
			return i;
		}
		MemHandleUnlock(rec_h);
		//DmReleaseRecord(db, i, false);
	}

#ifdef DEBUG
	LogWrite("xt_log", "fav", "not found");
#endif
	
	return -1;
}


// check if our record already installed
Boolean self_check(DmOpenRef db, UInt16 index, UInt32 creator)
{
	MemHandle rec_h;
	struct favorites *rec;
	Boolean status = false;
	
	rec_h = DmQueryRecord(db, index);
	if(rec_h == NULL)
		return false;
	
	// skip strange record
	if( MemHandleSize(rec_h) < 12) 
		return false;
		
	rec = (struct favorites *)MemHandleLock(rec_h);
	if(rec == NULL)
	{
		//DmReleaseRecord(db, index, false);
		return false;
	}
	
	// if application launch
	if(rec->operation == FAV_APP_LAUNCH)
	{
		// check creator-id
		if( MemCmp( (UInt8 *)rec + MemHandleSize(rec_h) - 4, &creator, 4) == 0)
		{
			// already exists
			status = true;
		}
	}
	
	MemHandleUnlock(rec_h);
	//DmReleaseRecord(db, index, false);
	
	return status;
}


// create new record, copy old to new, also change cell-number
static Boolean copy_record(DmOpenRef db, UInt16 new_index, 
							UInt16 old_index, UInt8 new_cell)
{
	MemHandle old_h, new_h;
	Boolean status = false;
	MemPtr old_rec, new_rec;

	old_h = DmQueryRecord(db, old_index);
	
	if(old_h == NULL)
		return status;
		
	old_rec = MemHandleLock(old_h);
	if(old_rec == NULL)
		return status;
	
	new_h = DmNewRecord(db, &new_index, MemHandleSize(old_h));
	if(new_h == NULL)
	{
		MemHandleUnlock(old_h);
		return status;
	}
			
	new_rec = MemHandleLock(new_h);
	if(new_rec == NULL)
	{
		MemHandleUnlock(old_h);
		return status;
	}
			
	if( DmWrite(new_rec, 0, old_rec, MemHandleSize(old_h)) == errNone)
	{
		// change cell-num
		DmWrite(new_rec, 1, &new_cell, 1);
		
		MemHandleUnlock(new_h);
		
		status = true;
		DmReleaseRecord(db, new_index, true);
	}
	else
		MemHandleUnlock(new_rec);
		
	MemHandleUnlock(old_h);
	
	return status;
}

// copy record to first free cell in vaforites
static Boolean backup_record(DmOpenRef db, UInt16 index, UInt8 cell)
{
	UInt16 num_records, i;
	Boolean status = false;
	MemHandle rec_h;
	struct favorites *rec;
	UInt8 inc_cell;
	
	num_records = DmNumRecordsInCategory(db, dmAllCategories );
	
	i = index + 1;
	inc_cell = cell + 1;
	
	while(i < num_records)
	{
		rec_h = DmQueryRecord(db, i);
		if(rec_h == NULL)
			break;
		
		rec = (struct favorites *) MemHandleLock(rec_h);
		if(rec == NULL)
			break;
		
		if(rec->cell > inc_cell)
		{
			// we can insert new record into current index			
			MemHandleUnlock(rec_h);
			
			status = copy_record(db, i, index, inc_cell);
			
			break;
		}
		
		MemHandleUnlock(rec_h);
		i++;
		inc_cell++;
	}
	
	if(! status)
	{
		// add new record at the end of list
		status = copy_record(db, dmMaxRecordIndex, index, inc_cell);
	}
	
	return status;
}


// write app-launch into favorites
Boolean favorites_write(DmOpenRef db, UInt8 position, Char *label, UInt32 creator,
							Char hot_key)
{
	Int16 rec_n;
	UInt16 len, new_index;
	MemPtr new_record, rec;
	MemHandle rec_h;
	UInt32 new_size;
	Err err;
	Boolean create_new = false;
	
	// find Nth
	rec_n = find_record(db, position, &create_new);
	if(rec_n < 0)
		return false;
		
	if( self_check(db, rec_n, creator) )
	{
#ifdef DEBUG
		LogWrite("xt_log", "fav", "already");
#endif
		return true;
	}
	
	// compose record
	len = StrLen(label) + 1;
	// string aligned up to 2bytes !
	len += len % 2;
	
	// 8bytes header + label + 4bytes creator-id
	new_size = 8 + len + 4;
	new_record = MemPtrNew(new_size);
	MemSet(new_record, new_size, 0);
	
	// setup record
	( (UInt16 *)new_record )[0] = position;
	( (UInt8 *)new_record )[7] = hot_key;
	StrCopy( ((Char *)new_record) + 8, label);
	MemMove( ((Char *)new_record) + 8 + len, &creator, 4);
	
	if(create_new)
	{
		new_index = (UInt16)rec_n;
		rec_h = DmNewRecord(db, &new_index, new_size);
	}
	else
	{
		// try backup old record to first next free cell
		if(! backup_record(db, rec_n, position) )
		{
			MemPtrFree(new_record);
			return false;
		}
			
		rec_h = DmResizeRecord(db, rec_n, new_size);
	}
	if(rec_h == NULL)
	{
		MemPtrFree(new_record);
		return false;
	}
	
	rec = MemHandleLock(rec_h);
	if(rec == NULL)
	{
		MemPtrFree(new_record);
		return false;
	}
	
	err = DmWrite(rec, 0, new_record, new_size);
	MemPtrFree(new_record);
	if(err != errNone)
	{
		DmReleaseRecord(db, rec_n, false);
		return false;
	}
	
	DmReleaseRecord(db, rec_n, true);
#ifdef DEBUG
	LogWrite("xt_log", "fav", "saved");
#endif
	return true;
}
