#pragma once
#include <Form.h>
#include <Event.h>
#include "AddressRsc.h"
#include "AddrTools.h"

#define MAX_COMPANIES 2048
#define MAX_RINGERS 2048
#define INDEX_EMAIL 4



/************************************************************
 * Function Prototypes
 *************************************************************/
Boolean CompanyHandleEvent(EventType * event);
void CompanyInitForm();
void CompanySetScrollers(Boolean up, Boolean down);
void CompanyScroll(Int16 value);
void CompanyUpdateForm(Int16 direction);
void CompanyFillList();
void CompanyListAddEntry(const Char* tEntry);
Boolean CompanyListEntryFound(const Char* tEntry);
void CompanyOnTrigger();
Boolean CompanyChangeEMailDomain(const char* lSrc, const char* lDomain, char* lDst);

			
Int16 CompanyCompare(void *, void *, Int32 other);
void CompanyFillFields();