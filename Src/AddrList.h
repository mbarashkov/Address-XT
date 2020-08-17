#pragma once
#include <Event.h>

#include <Event.h>
#include "AddrView.h"
#include "AddrView2.h"
#include "AddrEdit.h"
#include "AddrDialList.h"
#include "Address.h"
#include "AddressDB.h"
#include "AddressDB2.h"
#include "RecentDB.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "AddrNote.h"
#include "Reg.h"
#include "AddrPrefs.h"
#include "AddrDefines.h"
#include "AddressRsc.h"
#include "AddressTransfer.h"
#include "AddressTransfer2.h"
#include "AddrPrefs.h"
#include "CompanyData.h"
#include "ColorOptions.h"
#include "ConnectOptions.h"
#include "Links.h"
#include "Plugins/ToDoDB.h"
#include "Plugins/MemoMain.h"
#include "Plugins/MemoDB.h"
#include <PmSysGadgetLib.h>
#include <PmSysGadgetLibCommon.h>
#include <HsExt.h>

#include "globals.h"
#include <TextMgr.h>
#include <ErrorMgr.h>
#include <SoundMgr.h>
#include <TimeMgr.h>
#include <AboutBox.h>
#include <Category.h>
#include <Menu.h>
#include <UIResources.h>

#include "globals.h"
#include "dia.h"
#include "syslog.h"

#define LINKSICON_WIDTH 8

/************************************************************
 * Function Prototypes
 *************************************************************/

Boolean 	ListHandleEvent (EventType * event);
UInt32 		ListCheckRegistration();
void		PrvListLoadTable( FormType* frmP, Boolean refresh );
void		PrvListUpdateDisplay( UInt16 updateCode );
void		SetCategoryLabel(UInt16 category, Boolean changeCat);
void		PrvListClearLookupString ();
void		PrvListInit( FormType* frmP );
void 		ShowSkin();
void		LoadSkin();
void		UnloadSkin();
void 		PrvListDrawFormAndSkin(FormPtr frmP);
Boolean 	PrvListDeleteRecord (void);
Boolean 	ManageXTPresent();
Boolean 	ListOnMenuOpenEvent();
Boolean 	ListOnMenuCmdBarOpenEvent(EventType* event);
Boolean 	ListOnMenuEvent(EventType* event);
Boolean 	PrvListPurgeCategory(UInt16 categoryNum);
void 		Lookup(AddrLookupParamsType * params);
