#pragma once
#include "Address.h"
#include "AddressDB.h"

#include "globals.h"
#include <Form.h>
#include <StringMgr.h>
#include <ErrorMgr.h>
#include <NotifyMgr.h>
#include <TextMgr.h>
#include <FontSelect.h>
#include <KeyMgr.h>
#include <TimeMgr.h>
#include <Helper.h>
#include <HelperServiceClass.h>
#include "CompanyData.h"
#include "RecentDB.h"
#include <PalmUtils.h>
#include <Bitmap.h>
#include <UIColor.h>
#include <Form.h>
#include <ExgMgr.h>
#include <System/DLServer.h>
#include "ContactsDB.h"
#include "Links.h"

#define MAXNAMELENGTH			255

Int16 			LoadAddress(UInt16 recordIndex, AddressStruct ** list, Boolean countOnly);
void 			StartPhone();
Boolean 		ConnectContact(Char* connect, UInt16 field);
Boolean 		ConnectMap(Char* address, Char* city, Char* state, Char* zipcode, Char* country, UInt32 recid);
Boolean 		ToolsVersaMailPresent();
void 			ImportBirthdays();
void 			SlkDisable();
void 			SlkEnable();
void 			ResizeWindowToDisplay(WinHandle winH, Coord* dxP, Coord* dyP);
void 			ResizeModalWindowToDisplay(WinHandle winH, Coord* dxP, Coord* dyP);
Err 			EnableSilkResize(UInt16 silkRefNum, UInt16 state);
void			ToolsDeleteRecord (Boolean archive);
Boolean			ToolsIsPhoneIndexSupported( void* addrP, UInt16 phoneIndex );
UInt16			ToolsDuplicateCurrentRecord (UInt16 *numCharsToHilite, Boolean deleteCurrentRecord);
void 			ImportIDs();
void 			CstGetField(DmOpenRef tDBRef, Char *tRes, UInt16 tRecord, UInt16 tField);
void 			CstHRPlusInit();
Err 			PrvHandleDisplayChange(SysNotifyParamType* notifyParamsP);
void 			CstDIAInit();
Err 			PrvHandleDIAChange();

/*extern UInt16 GetGSIIndex(const FormType* frmP);
extern void CstSetFldEditable(UInt16 tObjID, UInt16 tEditable);
extern void CstSetFldUnderlined(UInt16 tObjID, UInt16 tEditable);
extern UInt16 CustomFrmGetFocus(FormType *tFormP);
extern UInt16 CustomFrmGetFocusSmp();
extern void CustomEditableFldSetTextPtr(FormType *tFormPtr, UInt16 tFieldID, Char* tText);
extern Char* CustomFldGetTextPtr(FormType *tFormP, UInt16 tFieldID);
extern Char* CustomFldGetTextPtrSmp(UInt16 tObjID);
extern void CustomFldSetTextPtr(FormType *tFormP, UInt16 tFieldID, Char* tText);
extern void CustomFldSetTextPtrSmp(UInt16 tObjID, Char* tText);
extern Char* CstCtlGetLabel(UInt16 tObjID);
extern void CstFldFreeSmp(UInt16 tObjID);
extern void CustomSetCtlLabelPtr(const FormType *tFormP, UInt16 tObjID, Char *tText);
extern Int16 CustomGetIndexInList(const ListType *tListP, const Char* tText);
extern Boolean CustomFldIsEmpty(FormType *tFormP, UInt16 tObjID);
extern Int16 CustomGetIndexInListSmp(UInt16 tListID, const Char* tText);*/
Boolean CustomFldIsEmptySmp(UInt16 tObjID);
void 	CstHitControl(UInt16 tObjID);
void 	CustomFldSetTextPtrSmp(UInt16 tObjID, Char* tText);
Boolean FldEmptySmp(UInt16 tObjID);
void 	FldSetTextPtrSmp(UInt16 tFieldID, Char* tText);
void	ToolsLeaveForm();
FontID	ToolsSelectFont (FontID currFontID);
FontID	ToolsSelectFontWithHires(FontID currFontID, Boolean withHires);
Boolean	ToolsAddrBeamBusinessCard (DmOpenRef dbP);
void 	ToolsDirtyRecord (UInt16 index);
Int16 	StrNumOfChrAdv(const char* lStr, WChar lChr, Boolean lRecursive);

void 	CstSetFldEditable(UInt16 tObjID, UInt16 tEditable);
void 	CstSetFldUnderlined(UInt16 tObjID, UInt16 tEditable);
UInt16 	CustomFrmGetFocus(FormType *tFormP);
UInt16 	CustomFrmGetFocusSmp();
void 	CstFldFreeSmp(UInt16 tObjID);
void 	CustomFldSetTextPtr(FormType *tFormP, UInt16 tFieldID, Char* tText);
Char* 	CstCtlGetLabel(UInt16 tObjID);
Int16 	CustomGetIndexInList(const ListType *tListP, const Char* tText);
Int16 	CustomGetIndexInListSmp(UInt16 tListID, const Char* tText);
Boolean CustomFldIsEmpty(FormType *tFormP, UInt16 tObjID);






