#pragma once

#include <PalmOS.h>
#include "globals.h"
#include "Address.h"

#define OLD_CALENDAR 'date'
#define OLD_TASKS 'todo'
#define OLD_MEMOS 'memo'
#define NEW_CALENDAR 'PDat'
#define NEW_TASKS 'PTod'
#define NEW_MEMOS 'PMem'

//Database structure

extern DmOpenRef LinksDB;

#define ERROR_GOTOLINK_PRIVATE 2

typedef enum
{
	link_memo = 0,
	link_address,
	link_note,
	link_datebook,
	link_todo,
	link_none
} LinkType;


typedef struct
{
	UInt32 addrID;
	UInt32 linkID;
	UInt32 type;
} LinkDBRecordType;

typedef LinkDBRecordType *LinkDBRecordPtr;

//exposed interface

#ifdef LIBDEBUG //moved to shared library
UInt32 	GetLinkCount(UInt16 refNum, DmOpenRef db, UInt32 addrID);
UInt16	FindLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type, UInt16 * foundIndex);
void 	UpdateLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type, UInt16 updIndex);
UInt16 	AddLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type);
void 	DeleteLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type);
void 	DeleteOrphaned(UInt16 refNum, DmOpenRef db);
void 	DeleteAllLinks(UInt16 refNum, DmOpenRef db, UInt32 addrID);
UInt16	GetLinkByIndex(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 index, LinkDBRecordPtr rec);
UInt16 	GoToLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 index);
Err 	LinkDBChangeRecord(UInt16 refNum, DmOpenRef dbP, LinkDBRecordPtr r, UInt16 index);
LocalID GetLinksLocalID(UInt16 refNum);
UInt16 	GetAllLinkCount(UInt16 refNum);
#else
extern UInt32	GetLinkCount(UInt16 refNum, DmOpenRef db, UInt32 addrID)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapGetLinkCount);
extern UInt16	FindLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type, UInt16 * foundIndex)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapFindLink);
extern void		UpdateLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type, UInt16 updIndex)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapUpdateLink);
extern UInt16	AddLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddLink);
extern void		DeleteLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 linkID, LinkType type)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapDeleteLink);
extern void		DeleteOrphaned(UInt16 refNum, DmOpenRef db)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapDeleteOrphaned);
extern void		DeleteAllLinks(UInt16 refNum, DmOpenRef db, UInt32 addrID)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapDeleteAllLinks);
extern UInt16	GetLinkByIndex(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 index, LinkDBRecordPtr rec)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapGetLinkByIndex);
extern UInt16	GoToLink(UInt16 refNum, DmOpenRef db, UInt32 addrID, UInt32 index)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapGoToLink);
extern Err		LinkDBChangeRecord(UInt16 refNum, DmOpenRef dbP, LinkDBRecordPtr r, UInt16 index)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapLinkDBChangeRecord);
extern LocalID	GetLinksLocalID(UInt16 refNum)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapGetLinksLocalID);
extern UInt16	GetAllLinkCount(UInt16 refNum)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapGetAllLinkCount);
#endif

#define LINK_NOERROR 0
#define LINK_ALREADYEXISTS 1 
#define LINK_ITSELF 2
