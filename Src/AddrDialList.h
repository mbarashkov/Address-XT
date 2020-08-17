/******************************************************************************
 *
 * Copyright (c) 1995-2002 PalmSource, Inc. All rights reserved.
 *
 * File: AddrDialList.h
 *
 * Release: Palm OS 5 SDK (68K) R3.
 *
 *****************************************************************************/

#ifndef ADDRDIALLIST_H
#define ADDRDIALLIST_H

#include <Event.h>
#include <Chars.h>
#include <ErrorMgr.h>
#include <NotifyMgr.h>
#include <StringMgr.h>
#include <SysUtils.h>
#include <UIResources.h>
#include <DataMgr.h>
#include <LocaleMgr.h>
#include <Form.h>
#include <Helper.h>
#include <HelperServiceClass.h>
#include <TextMgr.h>
#include <PalmUtils.h>
#include <BtLib.h>
//#include <pdQCore.h>
#include "PalmContactsDB/ContactsDB.h"
#include "AddressDB.h"

#include "AddrTools.h"
#include "AddrTools2.h"
#include "AddressRsc.h"
#include "AddrDefines.h"
#include "globals.h"
#include "dia.h"
#include "syslog.h"




/************************************************************
 * Function Prototypes
 *************************************************************/

Boolean DialListShowDialog( UInt16 recordIndex, UInt16 phoneIndex, UInt16 lineIndex );
Boolean DialListHandleEvent( EventType * event );
Boolean AddressDialable(UInt16 recordIndex, UInt16 phoneIndex);


#endif // ADDRDIALLIST_H
