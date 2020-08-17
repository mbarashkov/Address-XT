#pragma once

#include <IMCUtils.h>
#include <ExgMgr.h>

#define memoDBName						"MemoDB"
#define memoDBType						'DATA'
#define memoMaxLength					8192		// note: must be same as tFLD 1109 max length!!!


/************************************************************
 * Function Prototypes
 *************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


typedef UInt32 ReadFunctionF (const void * stream, Char * bufferP, UInt32 length);
typedef UInt32 WriteFunctionF (void * stream, const Char * const bufferP, Int32 length);


Boolean MemoListViewHandleEvent (EventPtr event);
void DrawMemoTitle (Char * memo, Int16 x, Int16 y, Int16 width);
void MemoListViewDisplayMask (RectanglePtr bounds);

