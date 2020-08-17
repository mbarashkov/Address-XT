// log data to palm-doc file
// $Id: syslog.c,v 1.3 2005/12/04 09:40:08 sm Exp $

#ifdef DEBUG

//#include <PalmOS.h>
#include <PalmTypes.h>
#include <DataMgr.h>
#include <StringMgr.h>
#include <MemoryMgr.h>

#include "syslog.h"

#define DOC_CREATOR    'REAd'
#define DOC_TYPE       'TEXt'
#define RECSIZE         4096

//#define DEBUGLOG

struct doc_header {
    UInt16 version;
    UInt16 unused;
    UInt32 length_uncompresed;
    UInt16 num_records;
    UInt16 max_rec_size;
    UInt32 position;
    // FIXME - we have only one record
    //UInt16 rec_size;
};

//Char *log_name = NULL;

void LogInit(Char *name) {
#ifdef DEBUG
#ifdef DEBUGLOG
    LocalID id;
    DmOpenRef db;
    MemHandle newH;
    struct doc_header rec0, *newP;
    UInt16 index = dmMaxRecordIndex;
    Err err;
    
    id = DmFindDatabase(0, name);
    if(id != 0) {
	// drop it
	err = DmDeleteDatabase(0, id);
	if(err) {
	    return;
	}
    }
/*--------------------------------------------------
*     else {
* 	err = DmGetLastErr();
* 	if(err != errNone) {
* 	    ui_perror(err, "DmFindDatabase");
* 	    return;
* 	}
*     }
*--------------------------------------------------*/
    
    //if( DmFindDatabase(0, name) == 0) {
	// create new
	DmCreateDatabase(0, name, DOC_CREATOR, DOC_TYPE, false);
	id = DmFindDatabase(0, name);
	db = DmOpenDatabase(0, id, dmModeWrite);
	newH = DmNewRecord(db, &index, sizeof(rec0));
	newP = (struct doc_header *) MemHandleLock(newH);

	MemSet(&rec0, sizeof(rec0), 0);

	rec0.version = 1;
	rec0.num_records = 1;
	rec0.length_uncompresed = 1;
	rec0.max_rec_size = RECSIZE;
	//rec0.rec_size = 1;
	
	
	DmWrite(newP, 0, &rec0, sizeof(rec0) );
	DmReleaseRecord(db, index, true);
	MemHandleUnlock(newH);

	// 1st
	index = dmMaxRecordIndex;
	newH = DmNewRecord(db, &index, 1);
        DmWrite(MemHandleLock(newH), 0, "", 1);
        DmReleaseRecord(db, index, true);
        MemHandleUnlock(newH);

        DmCloseDatabase(db);
    //}
    
    // XXX 
    //log_name = name;
#endif
#endif
}

// write message
//void LogWrite(Char *log, char *who,char *message) {

void LogWriteRaw(Char *log, Char *str) {
#ifdef DEBUG
#ifdef DEBUGLOG
    LocalID             id;
    DmOpenRef           ref;
    UInt16              index;
    MemHandle           newH, recH;
    char                *text;
    UInt32              size, newsize;
    struct doc_header rec0, *recP;

 
    if ((id = DmFindDatabase(0, log)) != 0)
    {
        ref = DmOpenDatabase(0,id,dmModeWrite);
        
        index = DmNumRecords(ref) - 1;
        newH = DmGetRecord(ref,index);
        text = MemHandleLock(newH);
        size = StrLen(text);
        newsize = size+StrLen(str)+1;
        MemHandleUnlock(newH);

        if (newsize < RECSIZE) {
            newH = DmResizeRecord(ref, index, newsize);
	    
	    recH = DmGetRecord(ref, 0);
	    recP = MemHandleLock(recH);
	    MemMove(&rec0, recP, sizeof(rec0));
	    // rec0.rec_size = newsize;
	    rec0.length_uncompresed = newsize;
	    DmWrite(recP, 0, &rec0, sizeof(rec0));
	    MemHandleUnlock(recH);
	    DmReleaseRecord(ref, 0, true);
	    
        } else { 
            recH = DmGetRecord(ref, 0);
            recP = MemHandleLock(recH);
            // update 0's record
            MemMove(&rec0, recP, sizeof(rec0));
            //rec0.wNumRecs++; 
	    rec0.num_records++;
            // rec0.wPartSize = newsize;
	    //rec0.rec_size = newsize;
	    rec0.length_uncompresed = newsize;
	    
            DmWrite(recP, 0, &rec0, sizeof(rec0));
	    MemHandleUnlock(recH);
            DmReleaseRecord(ref, 0, true);
	    
            // MemHandleUnlock(newH);
	    
	    // old
	    DmReleaseRecord(ref, index, false);
	    
            index = dmMaxRecordIndex;
            newH = DmNewRecord(ref, &index, StrLen(str) + 1 );
            size = 0;
        }
        text = MemHandleLock(newH);
        DmWrite(text,size,str,StrLen(str)+1);
        MemHandleUnlock(newH);
        DmReleaseRecord(ref,index,true);

        DmCloseDatabase(ref);
    }
#endif
#endif
}

void LogWrite(Char *log, Char *who, Char *message) {
#ifdef DEBUG
#ifdef DEBUGLOG
    Char *str;

    str = (Char *) MemPtrNew(StrLen(who) + StrLen(message) + 6);
    if(str == NULL) return;

    StrPrintF(str,"[%s] \n%s\n", who, message);
    
    LogWriteRaw(log, str);

    MemPtrFree(str);
#endif
#endif
}

// write: who, number, what
void LogWriteNum(Char *log, Char *who, UInt32 num,Char *message) {
#ifdef DEBUG
#ifdef DEBUGLOG
    Char *new_m;

    new_m = (Char *) MemPtrNew( StrLen(message) + maxStrIToALen + 5);
    StrIToA(new_m, num);
    StrCat(new_m, " - ");
    StrCat(new_m, message);
    StrCat(new_m, "\n");

    LogWrite(log, who, new_m);
    
    MemPtrFree(new_m);
#endif
#endif
}
// LogWriteNum


// TODO
// void LogInfo(int pri, int facility) { }

#endif // DEBUG