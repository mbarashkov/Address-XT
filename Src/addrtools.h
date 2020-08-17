#pragma once

#ifndef LIBDEBUG
#include "..\SharedLibrary\AddressXTLib\Src\AddressXTLib.h"
#endif

#include "Address.h"
#include "AddressDB.h"
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

#define googleMapCreatorID              'GLM.'
#define googleMapPreferenceID           1
#define googleMapPreferenceVersion      1

typedef enum
{
        googleMapFindLocation = 0,
        googleMapFindBusiness = 1,
        googleMapDirectionsTo = 2,
        googleMapDirectionsFrom = 3
} googleMapFindType;

typedef struct
{
	UInt16 version;
	UInt32 recid;
	Char * address;
	Char * city;
	Char * state;
	Char * zipcode;
	Char * country;
} HelperServiceMapDetailsType;
	
#include <PalmUtils.h>
#include <Bitmap.h>

#include <UIColor.h>
#include <Form.h>
#include <ExgMgr.h>
#include <System/DLServer.h>
#include "ContactsDB.h"

typedef struct {
	UInt16	fields[3];
	UInt16	replacement1;
	UInt16 	replacement2;
} SortInfo;

typedef SortInfo* SortInfoPtr;


//Macros
#define FrmHideObjectSmp(obj) FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), obj))
#define FrmShowObjectSmp(obj) FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), obj))
#define GetObjectPtrSmp(obj) (void*)FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), obj))		
#define GetObjectIndexSmp(obj) FrmGetObjectIndex(FrmGetActiveForm(), obj)		
#define FldGetTextPtrSmp(obj) (Char*)(FldGetTextPtr(FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), obj))))
#define StrFldCopy(str, obj) if(!FldEmptySmp(obj)){StrCopy(str, FldGetTextPtrSmp(obj));}

// Max length for first name field

#define maxDuplicatedIndString	20

// Maximum label column width in Edit and Record views.
//  (Would be nice if this was based on window size or screen size, do 1/2 screen for now)
#define maxLabelColumnWidth		80

#define maxPhoneColumnWidth		82 // (415)-000-0000x...

#define IsSpaceOrDelim(attr,c)		(attr[(UInt8)(c)] & (charAttr_CN|charAttr_SP|charAttr_XS|charAttr_PU))

//5-Way Navigator macros
typedef enum
{
	Left = 1,
	Right,
	Up,
	Down,
	Select
}  NavigatorDirection;


/************************************************************
 * Function Prototypes
 *************************************************************/

#ifdef LIBDEBUG
Boolean
TxtFindStringEx(UInt16 refNum, const Char* inSourceStr,
				const Char* inTargetStr,
				UInt32* outPos,
				UInt16* outLength);
UInt16 GetSortByCompany(UInt16 refNum);
Boolean ToolsSeekRecordEx (UInt16 refNum, UInt16 * indexP, Int16 offset, Int16 direction);
void deleteRecord(UInt16 refNum, DmOpenRef dbP, UInt16 index, Boolean archive);
void CstOpenOrCreateDB(UInt16 refNum, Char* tDBName, DmOpenRef* tDBRef);
void	ToolsWinDrawCharsHD(UInt16 refNum, char* text, UInt16 length, UInt16 x, UInt16 y );
void	ToolsSetDBAttrBits(UInt16 refNum, DmOpenRef dbP, UInt16 attrBits);
Boolean ToolsDetermineRecordName (UInt16 refNum, void* recordP, Int16 *shortenedFieldWidth, 
Int16 *fieldSeparatorWidth, UInt16 sortByCompany, Char **name1, Int16 *name1Length, 
Int16 *name1Width, Char **name2, Int16 *name2Length, Int16 *name2Width,
Char **unnamedRecordStringPtr, MemHandle* unnamedRecordStringH, Int16 nameExtent, Boolean lowRes);
void	ToolsDrawRecordName (UInt16 refNum, Char *name1, Int16 name1Length, Int16 name1Width, Char *name2, Int16 name2Length, Int16 name2Width, Int16 nameExtent, Int16 *x, Int16 y, Int16 shortenedFieldWidth, Int16 fieldSeparatorWidth, Boolean center, 
Boolean priorityIsName1, Boolean inTitle, Boolean lowRes);
Int16 ToolsDrawRecordNameAndPhoneNumber(UInt16 refNum, univAddrDBRecordType* record, RectanglePtr bounds, Char * phoneLabelLetters, UInt16 sortByCompany, Char **unnamedRecordStringPtr, MemHandle* unnamedRecordStringH,
Boolean lowRes);
UInt16 	ToolsGetLabelColumnWidth (UInt16 refNum, void* appInfoPtr, FontID labelFontID);
Boolean	ToolsSeekRecord (UInt16 refNum, UInt16 * indexP, Int16 offset, Int16 direction);
Err		ToolsCustomAcceptBeamDialog(UInt16 refNum, DmOpenRef dbP, ExgAskParamPtr askInfoP);
void 	ToolsInitPhoneLabelLetters(UInt16 refNum, univAppInfoPtr appInfoPtr, Char * phoneLabelLetters);
char*	ToolsGetStringResource (UInt16 refNum, UInt16 stringResource, char * stringP);
UInt16	ToolsGetLineIndexAtOffset(UInt16 refNum, Char* textP, UInt16 offset );
void 	ToolsUnivWinDrawCharsHD(UInt16 refNum, Char* text, UInt16 length, UInt16 x, UInt16 y );
// enable/disable BT via panel app launch
Boolean ToolsEnableBluetooth(UInt16 refNum, Boolean on);
FontID ToolsFntSetFont(UInt16 refNum, UInt16 libRefNum, Boolean highRes, UInt16 screen, FontID font);
SortInfo ToolsGetSortInfo(UInt16 refNum, UInt16 sortByCompany);
void FrmGlueNavObjectTakeFocus (UInt16 refNum, 
   const FormType *formP,
   UInt16 objID
);
Err FrmGlueNavDrawFocusRing (UInt16 refNum, 
   FormType *formP,
   UInt16 objectID,
   Int16 extraInfo,
   RectangleType *boundsInsideRingP,
   FrmNavFocusRingStyleEnum ringStyle,
   Boolean forceRestore
);
Err FrmGlueNavRemoveFocusRing (UInt16 refNum, 
   FormType *formP
);
Err FrmGlueNavGetFocusRingInfo (UInt16 refNum, 
   const FormType *formP,
   UInt16 *objectIDP,
   Int16 *extraInfoP,
   RectangleType *boundsInsideRingP,
   FrmNavFocusRingStyleEnum *ringStyleP
);
Boolean ToolsIsFiveWayNavPalmEvent(UInt16 refNum, EventPtr eventP);                                                      
Boolean ToolsIsFiveWayNavEvent(UInt16 refNum, EventPtr eventP);														    
Boolean ToolsNavSelectHSPressed(UInt16 refNum, EventPtr eventP);														
Boolean ToolsNavSelectPalmPressed(UInt16 refNum, EventPtr eventP);													
Boolean ToolsNavSelectPressed(UInt16 refNum, EventPtr eventP);														
Boolean ToolsNavDirectionHSPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav);
Boolean ToolsNavDirectionPalmPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav);                                            
Boolean ToolsNavDirectionPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav);                                          
Boolean ToolsNavKeyPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav);
Boolean PrvToolsPhoneIsANumber(UInt16 refNum, Char* phone );
Char* 	GetDelimiterStr(UInt16 refNum, UInt8 delimiter);
UInt16 	GetDelimiterLen(UInt16 refNum, UInt8 delimiter);
void 	LoadColors(UInt16 refNum);
UInt16 	PrvDetermineRecordNameHelper(UInt16 refNum, UInt16 sortByCompany, univAddrDBRecordType* recordP, Char** name1, Char** name2);
#else
extern Boolean	TxtFindStringEx(UInt16 refNum, const Char* inSourceStr,
				const Char* inTargetStr,
				UInt32* outPos,
				UInt16* outLength)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapTxtFindStringEx);
extern UInt16	GetSortByCompany(UInt16 refNum)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapGetSortByCompany);
extern Boolean	ToolsSeekRecordEx (UInt16 refNum, UInt16 * indexP, Int16 offset, Int16 direction)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsSeekRecordEx);
extern void		deleteRecord (UInt16 refNum, DmOpenRef dbP, UInt16 index, Boolean archive)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapdeleteRecord);
extern void		CstOpenOrCreateDB(UInt16 refNum, Char* tDBName, DmOpenRef* tDBRef)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapCstOpenOrCreateDB);
extern void		ToolsWinDrawCharsHD(UInt16 refNum, char* text, UInt16 length, UInt16 x, UInt16 y )
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsWinDrawCharsHD);
extern void		ToolsSetDBAttrBits(UInt16 refNum, DmOpenRef dbP, UInt16 attrBits)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsSetDBAttrBits);
extern Boolean	ToolsDetermineRecordName (UInt16 refNum, void* recordP, Int16 *shortenedFieldWidth, 
					Int16 *fieldSeparatorWidth, UInt16 sortByCompany, Char **name1, Int16 *name1Length, 
					Int16 *name1Width, Char **name2, Int16 *name2Length, Int16 *name2Width,
					Char **unnamedRecordStringPtr, MemHandle* unnamedRecordStringH, Int16 nameExtent, Boolean lowRes)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsDetermineRecordName);
extern void		ToolsDrawRecordName (UInt16 refNum, Char *name1, Int16 name1Length, Int16 name1Width, Char *name2, Int16 name2Length, Int16 name2Width, Int16 nameExtent, Int16 *x, Int16 y, Int16 shortenedFieldWidth, Int16 fieldSeparatorWidth, Boolean center, 
					Boolean priorityIsName1, Boolean inTitle, Boolean lowRes)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsDrawRecordName);
extern Int16	ToolsDrawRecordNameAndPhoneNumber(UInt16 refNum, univAddrDBRecordType* record, RectanglePtr bounds, Char * phoneLabelLetters, UInt16 sortByCompany, Char **unnamedRecordStringPtr, MemHandle* unnamedRecordStringH,
					Boolean lowRes)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsDrawRecordNameAndPhoneNumber);
extern UInt16	ToolsGetLabelColumnWidth (UInt16 refNum, void* appInfoPtr, FontID labelFontID)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsGetLabelColumnWidth);
extern Boolean	ToolsSeekRecord (UInt16 refNum, UInt16 * indexP, Int16 offset, Int16 direction)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsSeekRecord);
extern Err		ToolsCustomAcceptBeamDialog(UInt16 refNum, DmOpenRef dbP, ExgAskParamPtr askInfoP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsCustomAcceptBeamDialog);
extern UInt16	ToolsInitPhoneLabelLetters(UInt16 refNum, univAppInfoPtr appInfoPtr, Char * phoneLabelLetters)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsInitPhoneLabelLetters);
extern char*	ToolsGetStringResource (UInt16 refNum, UInt16 stringResource, char * stringP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsGetStringResource);
extern UInt16	ToolsGetLineIndexAtOffset(UInt16 refNum, Char* textP, UInt16 offset )
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsGetLineIndexAtOffset);
extern void		ToolsUnivWinDrawCharsHD(UInt16 refNum, Char* text, UInt16 length, UInt16 x, UInt16 y )
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsUnivWinDrawCharsHD);
extern Boolean	ToolsEnableBluetooth(UInt16 refNum, Boolean on)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsEnableBluetooth);
extern FontID	ToolsFntSetFont(UInt16 refNum, UInt16 libRefNum, Boolean highRes, UInt16 screen, FontID font)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsFntSetFont);
extern SortInfo	ToolsGetSortInfo(UInt16 refNum, UInt16 sortByCompany)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsGetSortInfo);
extern void	FrmGlueNavObjectTakeFocus (UInt16 refNum, 
  					 const FormType *formP,
   					UInt16 objID)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapFrmGlueNavObjectTakeFocus);
extern Err 		FrmGlueNavDrawFocusRing (UInt16 refNum, 
					FormType *formP,
					UInt16 objectID,
					Int16 extraInfo,
					RectangleType *boundsInsideRingP,
					FrmNavFocusRingStyleEnum ringStyle,
					Boolean forceRestore)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapFrmGlueNavDrawFocusRing);
extern Err		FrmGlueNavRemoveFocusRing (UInt16 refNum, FormType *formP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapFrmGlueNavRemoveFocusRing);
extern Err		FrmGlueNavGetFocusRingInfo (UInt16 refNum, 
					const FormType *formP,
					UInt16 *objectIDP,
					Int16 *extraInfoP,
					RectangleType *boundsInsideRingP,
					FrmNavFocusRingStyleEnum *ringStyleP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapFrmGlueNavGetFocusRingInfo);
extern Boolean	ToolsIsFiveWayNavPalmEvent(UInt16 refNum, EventPtr eventP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsIsFiveWayNavPalmEvent);
extern Boolean	ToolsIsFiveWayNavEvent(UInt16 refNum, EventPtr eventP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsIsFiveWayNavEvent);
extern Boolean	ToolsNavSelectHSPressed(UInt16 refNum, EventPtr eventP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsNavSelectHSPressed);
extern Boolean	ToolsNavSelectPalmPressed(UInt16 refNum, EventPtr eventP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsNavSelectPalmPressed);
extern Boolean	ToolsNavSelectPressed(UInt16 refNum, EventPtr eventP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsNavSelectPressed);
extern Boolean	ToolsNavDirectionHSPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsNavDirectionHSPressed);
extern Boolean	ToolsNavDirectionPalmPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsNavDirectionPalmPressed);
extern Boolean	ToolsNavDirectionPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsNavDirectionPressed);
extern Boolean	ToolsNavKeyPressed(UInt16 refNum, EventPtr eventP, NavigatorDirection nav)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapToolsNavKeyPressed);
extern Boolean	PrvToolsPhoneIsANumber(UInt16 refNum, Char* phone )
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvToolsPhoneIsANumber);
extern Char*	GetDelimiterStr(UInt16 refNum, UInt8 delimiter)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapGetDelimiterStr);
extern UInt16	GetDelimiterLen(UInt16 refNum, UInt8 delimiter)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapGetDelimiterLen);
extern void		LoadColors(UInt16 refNum)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapLoadColors);
extern UInt16 	PrvDetermineRecordNameHelper(UInt16 refNum, UInt16 sortByCompany, univAddrDBRecordType* recordP, Char** name1, Char** name2)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvDetermineRecordNameHelper);
#endif

globalVars* 	getGlobalsPtr();
UInt16 GetGSIIndex(const FormType* frmP);
void MoveFormObject(FormType* frmP, UInt16 id, Coord dx, Coord dy);
void MoveFormObjectHide(FormPtr frm, UInt16 id, Coord dx, Coord dy);
void MoveFormGSI(FormPtr frm, Coord dx, Coord dy);
void CustomHideObject(FormType *tFormP, UInt16 tObjID);
void CustomHideObjectSmp(UInt16 tObjID);
void CustomShowObject(FormType *tFormP, UInt16 tObjID);
void CustomShowObjectSmp(UInt16 tObjID);
Err CstHighResInit(void);
UInt16 	GetWindowHeight();
UInt16 	GetWindowWidth();
void	ToolsChangeCategory (UInt16 category);
void 	CustomSetCtlLabelPtrSmp(UInt16 tObjID, Char *tText);
void 	CustomSetCtlLabelPtr(const FormType *tFormP, UInt16 tObjID, Char *tText);
Boolean	ToolsIsDialerPresent( void );
Char*	CustomFldGetTextPtrSmp(UInt16 tObjID);
Char*	CustomFldGetTextPtr(FormType *tFormP, UInt16 tFieldID);
void 	CustomEditableFldSetTextPtrSmp(UInt16 tFieldID, Char* tText);
void 	CustomEditableFldSetTextPtr(FormType *tFormPtr, UInt16 tFieldID, Char* tText);
void* 	CustomGetObjectPtr(const FormType *tFormP, UInt16 tObjID);
void* 	CustomGetObjectPtrSmp(UInt16 tObjID);
void	FillConnectOptions();
void 	SetDefaultConnectOptions(AddrDialOptionsPreferenceType* pPrefs);


