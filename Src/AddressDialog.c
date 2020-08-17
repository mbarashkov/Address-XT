#include <PalmOS.h>
#include "AddrTools.h"
#include "AddrTools2.h"
#include "Address.h"
#include "AddressDialog.h"
#include "Mapopolis.h"
#include "globals.h"
#include "dia.h"

void AddressInitForm();
void AddressFree();
void AddressOpen();

void AddressOpen()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 cardNo;
	LocalID localID;
	if(MAPOPOLIS)
	{
		Err err;
		MapopolisLaunchParameterBlock * parameterBlock;
		LocalID id = DmFindDatabase(0, "Mapopolis");
		if(id == 0)
		{
			FrmAlert(MapFailedAlert);
			return;
		}
		
		if ( !(parameterBlock =
		MemPtrNew(sizeof(MapopolisLaunchParameterBlock))) )
		{
			FrmAlert(MapFailedAlert);
			return;
		}
		
		MemSet(parameterBlock, sizeof(*parameterBlock), 0);
		
		
		// build the block
		parameterBlock->interfaceVersion = 1;
		parameterBlock->command = 1;
		parameterBlock->licenseCode = 28424;
		// set the address
		StrCopy(parameterBlock->streetAddress,
		globals->AddressListPtr[globals->gAddressSel]->address);
		
		if(globals->AddressListPtr[globals->gAddressSel]->city != NULL)
			StrCopy(parameterBlock->cityName, globals->AddressListPtr[globals->gAddressSel]->city);
		
		if(globals->AddressListPtr[globals->gAddressSel]->state != NULL)
			StrCopy(parameterBlock->state, globals->AddressListPtr[globals->gAddressSel]->state);
		
		//if(AddressListPtr[gAddressSel]->zip != NULL)
		//	StrCopy(parameterBlock->zip, AddressListPtr[gAddressSel]->zip);

		// set app name and creator id to be called back
		StrCopy(parameterBlock->callbackAppName, "Address XT");
		parameterBlock->callbackCreatorID = 'adXT';
		
		// build the callback info block
		// set ownership of the paramter block
		MemPtrSetOwner(parameterBlock, 0);
		// launch Mapopolis
		err = SysUIAppSwitch(0, id, MapopolisLaunch,	parameterBlock);
		if ( err )
		{
			FrmAlert(MapFailedAlert);
			return;
		}
	}
	else if(globals->gTomTom)
	{
		Err err = VFSDirCreate(globals->gTomTomVolRef, "/TomTom/SdkFileCalls");
		FileRef fileRef;
		Char *nav;
		DmSearchStateType state; 
		UInt16 launchFlags=0;
		UInt32		*gotoInfoP;
		Char curr;
		//split address to number and address
		UInt16 lastspace = 0, firstspace = 0;
		UInt16 navSize = 22+1;
		
		globals->gTomTomNumber = CustomLstGetSelection(AddressTomTomList);	
		
		if(globals->AddressListPtr[globals->gAddressSel]->city)
			navSize += StrLen(globals->AddressListPtr[globals->gAddressSel]->city);
		if(globals->AddressListPtr[globals->gAddressSel]->address)
		 	navSize += StrLen(globals->AddressListPtr[globals->gAddressSel]->address);
		if(globals->AddressListPtr[globals->gAddressSel]->zip)
			navSize += StrLen(globals->AddressListPtr[globals->gAddressSel]->zip);
		
		nav = MemPtrNew(navSize);
		MemSet(nav, navSize, 0);
		if(globals->AddressListPtr[globals->gAddressSel]->address != NULL)
		{
			if(globals->gTomTomNumber == TOMTOMNUMBER_END)
			{
				if(StrLen(globals->AddressListPtr[globals->gAddressSel]->address) > 1)
				{
					UInt16 i;
					Boolean nonSpace = false;
					for(i = 0; i < StrLen(globals->AddressListPtr[globals->gAddressSel]->address) - 1; i++)
					{
						curr = globals->AddressListPtr[globals->gAddressSel]->address[StrLen(globals->AddressListPtr[globals->gAddressSel]->address)-i];
						if((curr == ' ') && nonSpace)
							break;
						else if(curr != 0)
							nonSpace = true;
					}
					if(i > 0)
						lastspace = StrLen(globals->AddressListPtr[globals->gAddressSel]->address) - i;
				}
			}
			else if(globals->gTomTomNumber == TOMTOMNUMBER_BEGIN)
			{
				if(StrLen(globals->AddressListPtr[globals->gAddressSel]->address) > 1)
				{
					UInt16 i;
					Boolean nonSpace = false;
					for(i = 0; i < StrLen(globals->AddressListPtr[globals->gAddressSel]->address) - 1; i++)
					{
						curr = globals->AddressListPtr[globals->gAddressSel]->address[i];
						if((curr == ' ') && nonSpace)
							break;
						else if(curr != 0)
							nonSpace = true;
					}
					if(i > 0)
						firstspace = i;
				}
			}
		}
		
		gotoInfoP = (UInt32*)MemPtrNew (sizeof(UInt32));
		MemPtrSetOwner(gotoInfoP, 0);			
			
		if(err == vfsErrVolumeBadRef || err == vfsErrNoFileSystem)
		{
			FrmAlert(alertTomTomCardRemoved);
			FrmReturnToForm(0);
			return;
		}
		else if(err == vfsErrVolumeFull) 
		{
			FrmAlert(alertTomTomCardFull);
			FrmReturnToForm(0);
			return;
		}
		err = VFSFileCreate(globals->gTomTomVolRef, "/TomTom/SdkFileCalls/PALM.TomTomNavigationServer.0.1.message");
		if(err == vfsErrVolumeFull) 
		{
			FrmAlert(alertTomTomCardFull);
			FrmReturnToForm(0);
			return;
		}
		VFSFileOpen(globals->gTomTomVolRef, "/TomTom/SdkFileCalls/PALM.TomTomNavigationServer.0.1.message", vfsModeWrite, &fileRef);
		
		StrCopy(nav, "NavigateToAddress|");
		if(globals->AddressListPtr[globals->gAddressSel]->city != NULL)
			StrCat(nav, globals->AddressListPtr[globals->gAddressSel]->city);
		StrCat(nav, "|");
		if(globals->AddressListPtr[globals->gAddressSel]->address != NULL)
		{
			if(globals->gTomTomNumber == TOMTOMNUMBER_END)
			{
				if(lastspace != 0)
				{
					UInt16 i;
					UInt16 len = StrLen(nav);
					for(i = 0; i < lastspace; i++)
					{
						nav[len + i] = globals->AddressListPtr[globals->gAddressSel]->address[i];
					}
				}
				else
					StrCat(nav, globals->AddressListPtr[globals->gAddressSel]->address);
			}
			else if(globals->gTomTomNumber == TOMTOMNUMBER_BEGIN)
			{
				if(firstspace != 0)
				{
					UInt16 i;
					UInt16 len = StrLen(nav);
					UInt16 lenAddr = StrLen(globals->AddressListPtr[globals->gAddressSel]->address);
					for(i = 0; i < lenAddr - firstspace - 1; i++)
					{
						nav[len + i] = globals->AddressListPtr[globals->gAddressSel]->address[firstspace + i + 1];
					}
				}
				else
					StrCat(nav, globals->AddressListPtr[globals->gAddressSel]->address);
			}
			else if(globals->gTomTomNumber == TOMTOMNUMBER_SKIP)
			{
				StrCat(nav, globals->AddressListPtr[globals->gAddressSel]->address);
			}
		}
		StrCat(nav, "|");
		if(globals->AddressListPtr[globals->gAddressSel]->address != NULL)
		{
			if(globals->gTomTomNumber == TOMTOMNUMBER_END)
			{
				if(lastspace != 0)
				{
					UInt16 i;
					UInt16 len = StrLen(nav);
					UInt16 lenAddr = StrLen(globals->AddressListPtr[globals->gAddressSel]->address);
					for(i = 0; i < lenAddr - lastspace - 1; i++)
					{
						nav[len + i] = globals->AddressListPtr[globals->gAddressSel]->address[lastspace + i + 1];
					}
				}
			}
			else if(globals->gTomTomNumber == TOMTOMNUMBER_BEGIN)
			{
				if(firstspace != 0)
				{
					UInt16 i;
					UInt16 len = StrLen(nav);
					for(i = 0; i < firstspace; i++)
					{
						nav[len + i] = globals->AddressListPtr[globals->gAddressSel]->address[i];
					}
				}
			}			
		}
		StrCat(nav, "|");
		if(globals->AddressListPtr[globals->gAddressSel]->zip != NULL)
			StrCat(nav, globals->AddressListPtr[globals->gAddressSel]->zip);
		StrCat(nav, "|");
		
		VFSFileWrite(fileRef, StrLen(nav), nav, NULL);
		VFSFileClose(fileRef);	
		err = VFSFileCreate(globals->gTomTomVolRef, "/TomTom/SdkFileCalls/PALM.TomTomNavigationServer.0.1.finished");
		if(err == vfsErrVolumeFull) 
		{
			FrmAlert(alertTomTomCardFull);
			FrmReturnToForm(0);
			return;
		}
		VFSFileOpen(globals->gTomTomVolRef, "/TomTom/SdkFileCalls/PALM.TomTomNavigationServer.0.1.finished", vfsModeWrite, &fileRef);
		StrCopy(nav, "finish\n\n");
		VFSFileWrite(fileRef, StrLen(nav), nav, NULL);
		VFSFileClose(fileRef);	
		
		MemPtrFree(nav);
		
		//FrmReturnToForm(0);
		
		if(DmGetNextDatabaseByTypeCreator(true, &state, 'appl', 'MniC', false, &cardNo, &localID) != errNone)
		{
			FrmAlert(MapFailedAlert);
			return;
		}
		
		err = SysUIAppSwitch(0, localID, sysAppLaunchCmdNormalLaunch,	gotoInfoP);
		if ( err )
		{
			FrmAlert(MapFailedAlert);
			return;
		}			
	}
}

void AddressFree()
{
	UInt16 cnt;
	globalVars* globals = getGlobalsPtr();
	for(cnt=0;cnt<globals->AddressCount;cnt++)
	{
		if(globals->AddressListPtr[cnt]->address != 0)
			MemPtrFree(globals->AddressListPtr[cnt]->address);
		if(globals->AddressListPtr[cnt]->city != 0)
			MemPtrFree(globals->AddressListPtr[cnt]->city);
		if(globals->AddressListPtr[cnt]->state != 0)
			MemPtrFree(globals->AddressListPtr[cnt]->state);
		if(globals->AddressListPtr[cnt]->zip != 0)
			MemPtrFree(globals->AddressListPtr[cnt]->zip);
		
	}	
	MemPtrFree(globals->AddressListPtr);
	MemPtrFree(globals->AddressListPtrStr);
}

void AddressInitForm()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 lSize, i;
	ListType *lListPtr=(ListType*)GetObjectPtrSmp(AddressList); 
	globals->AddressCount=LoadAddress(globals->CurrentRecord, NULL, true);
	if(globals->AddressCount==0)
		FrmReturnToForm(0);
	globals->AddressListPtr=MemPtrNew(globals->AddressCount*sizeof(AddressStruct*));
	globals->AddressListPtrStr=MemPtrNew(globals->AddressCount*sizeof(Char**));
	
	LoadAddress(globals->CurrentRecord, globals->AddressListPtr, false);
	//fill the list UI control with values
	lSize=globals->AddressCount;
	if(lSize>3)
		lSize=3;
	//LstSetHeight(lListPtr, lSize);
	for(i = 0; i < lSize; i++)
	{
		globals->AddressListPtrStr[i] = globals->AddressListPtr[i]->address;
	}
	
	if(MAPOPOLIS)
	{
		if(globals->AddressCount == 1)
		{
			globals->gAddressSel = 0;
			AddressFree();
			AddressOpen();
		}
	}
	if(globals->gTomTom)
	{
		CustomShowObjectSmp(AddressTomTomTrigger);
		CustomShowObjectSmp(AddressChooseTypeLabel);
	}
	
	LstSetListChoices(lListPtr,globals-> AddressListPtrStr, globals->AddressCount);	 
	LstDrawList(lListPtr);
	
	if(globals->gTomTom)
	{
		lListPtr=(ListType*)GetObjectPtrSmp(AddressTomTomList);
		StrCopy(globals->gTomTomNumberStr, LstGetSelectionText(lListPtr, globals->gTomTomNumber));
		CustomSetCtlLabelPtrSmp(AddressTomTomTrigger, globals->gTomTomNumberStr);
	}	
}

static Boolean AddressDialogOnCtlSelectEvent(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	switch (event->data.ctlSelect.controlID)
	{
	case AddressOkButton:
		globals->gAddressSel = CustomLstGetSelection(AddressList);		
		AddressFree();
		AddressOpen();
		handled = true;
		break;
	case AddressCancelButton:
		AddressFree();
		FrmReturnToForm(0);
		handled=true;
		break;
	default:
		break;
	}
	return handled;
}

static Boolean AddressDialogOnFrmOpenEvent()
{
	FormPtr frm = FrmGetFormPtr(AddressDialog);
	dia_save_state(); 
	dia_enable(frm, false);
	FrmDrawForm (frm);
	AddressInitForm();
	return true;
}

static Boolean  AddressDialogOnLstSelectEvent()
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frmP = FrmGetActiveForm();
	if(globals->gTomTom)
		return false;
	FrmSetFocus(frmP, FrmGetObjectIndex(frmP, AddressOkButton));
	if(globals->gNavigation)
	{
		FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frmP, AddressOkButton);	
	}
	return true;
}

Boolean AddressDialogHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch(event->eType)
	{
		case lstSelectEvent:
			handled = AddressDialogOnLstSelectEvent();
			break;
		case ctlSelectEvent:
			handled = AddressDialogOnCtlSelectEvent(event);
			break;
		case frmOpenEvent:
			handled = AddressDialogOnFrmOpenEvent();
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	return (handled);
}
