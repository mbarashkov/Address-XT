#pragma once

#include <Event.h>
#include "AddrView.h"
#include "AddrList.h"
#include "AddrEdit.h"
#include "AddrDetails.h"
#include "AddrNote.h"
#include "Address.h"
#include "AddrDialList.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "AddressRsc.h"
#include "AddrDefines.h"
#include "AddressTransfer.h"
#include "AddressTransfer2.h"
#include "Links.h"
#include "globals.h"
#include "Plugins/ToDo.h"
#include "Plugins/ToDoDB.h"
#include "Plugins/DateDB.h"
#include "Plugins/DateDay.h"
#include "Plugins/MemoMain.h"
#include "Plugins/MemoDB.h"
#include "PalmContactsDB/ContactsDB.h"
#include "RecentDB.h"
#include "AddrXTDB.h"
//#include "Mapopolis.h"
#include "dia.h"

#include "pnoJpg/pnoJpeg.h"

#include <TextMgr.h>
#include <ErrorMgr.h>
#include <SoundMgr.h>
#include <StringMgr.h>
#include <AboutBox.h>
#include <Category.h>
#include <Menu.h>
#include <UIResources.h>
#include <FeatureMgr.h>

#include "syslog.h"


// number of record view lines to store
#define recordViewLinesMax		80
#define recordViewBlankLine		0xffff   // Half height if the next line.x == 0

// Resource type used to specify order of fields in Edit view.
#define	fieldMapRscType			'fmap'

/***********************************************************************
 *
 *   Internal Structures
 *
 ***********************************************************************/

// Info on how to draw the record view


typedef enum
{
    P1ContactsVirtualAddr1Label = P1ContactsaddressFieldsCount,
    P1ContactsVirtualAddr2Label,
    P1ContactsVirtualAddr3Label

} P1ContactsVirtualFields;

/************************************************************
 * Function Prototypes
 *************************************************************/


Boolean 	ViewHandleEvent (EventType * event);
void 		LinksDrawList(Int16 itemNum, RectangleType *tBounds, Char **itemsText); 
void		DisplayLinks();
void		PrvViewInit( FormType* frm );
void		PrvViewDrawBusinessCardIndicator (FormPtr formP);
void		PrvViewUpdate( FormType* frmP );
void		PrvViewClose(void);
