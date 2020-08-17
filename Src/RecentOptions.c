#include <PalmOS.h>
#include "RecentOptions.h"
#include "AddrPrefs.h"
#include "globals.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "Address.h"
#include "RecentDB.h"


void ROInitForm();
void ROSave();
/*void SetEnabled(UInt16 enabled);
void RecentOnEnabledSelector();
*/void ResizeRecentWindowToDisplay(WinHandle winH, Coord* dxP, Coord* dyP);
void RecentDIAInit(FormPtr frmP);

void ResizeRecentWindowToDisplay(WinHandle winH, Coord* dxP, Coord* dyP)
{
    RectangleType oldWinBounds, winBounds;
    ErrFatalDisplayIf(winH == NULL, "ResizeWindowToDisplay: null Window");

    WinGetBounds(winH, &oldWinBounds);
    WinGetBounds(WinGetDisplayWindow(), &winBounds);

	winBounds.topLeft.x=2+ (winBounds.extent.x - 160)/2;
	winBounds.topLeft.y=winBounds.extent.y-83;
	winBounds.extent.x=156;
	winBounds.extent.y=81;
		
    WinSetBounds(winH, &winBounds);
    if (dxP != NULL) { *dxP = winBounds.extent.x - oldWinBounds.extent.x; }
    if (dyP != NULL) { *dyP = winBounds.extent.y - oldWinBounds.extent.y; }
}


void RecentDIAInit(FormPtr frmP)
{
	WinHandle formWinH;
	RectangleType newBounds;
	UInt16 width, height;
	Int16 dx=0, dy=0;
    if(!DIA)
		return;
	
	// get the new display window bounds
	WinGetBounds(WinGetDisplayWindow(), &newBounds);
	width=newBounds.extent.x;
	height=newBounds.extent.y;
	
	if(width>220)
		width=225;
	if(height>220)
		height=225;
	 			
	formWinH = FrmGetWindowHandle (frmP);
	WinSetConstraintsSize(formWinH, 160, height , 225,160, width, 225);
	FrmSetDIAPolicyAttr(frmP, frmDIAPolicyCustom);
	PINSetInputTriggerState(pinInputTriggerDisabled);
	PINSetInputAreaState(pinInputAreaUser);
    ResizeRecentWindowToDisplay(FrmGetWindowHandle(frmP), &gSilkState.resizeAmount.x,&gSilkState.resizeAmount.y);
      	
    gSilkState.curResizableFormP = frmP;
    gSilkState.resizeAmount.x = gSilkState.resizeAmount.y = 0;       
}




void ROSave()
{
/*	UInt16 prefsSize, selection;
	Int16 prefsVersion;
	UInt32 numRecent=0;
	UInt16 enabled=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), RecentOptionsEnableCheckbox));
	UInt16 showAll=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), RecentOptionsShowAll));
	ListType *recentListPtr=(ListType*)GetObjectPtrSmp(RecentOptionsNumberList);
	AddrRecentPreferenceType prefs;
	
	// Write the preferences / saved-state information.
	prefsSize=0;
	PrefGetAppPreferences (CREATORID, addrPrefRcntID, NULL, &prefsSize, true);
	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefRcntID, &prefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	
	}
	if (prefsVersion == noPreferenceFound)
	{
		prefs.Number=0;
	}
	// Write the state information.
	prefs.Enabled=enabled;
	prefs.ShowAll=showAll;
	if(enabled)
	{
		selection=LstGetSelection(recentListPtr);
		switch(selection)
		{
			case 0:
				prefs.Number=5;
				break;
			case 1:
				prefs.Number=10;
				break;
			case 2:
				prefs.Number=20;
				break;
			case 3:
				prefs.Number=50;
				break;
			case 4:
				prefs.Number=100;
				break;		
		}	
	}
	prefsSize=sizeof(AddrRecentPreferenceType);
	gMaxRecent=prefs.Number;
	gEnabledRecent=prefs.Enabled;
	gShowAll=prefs.ShowAll;
	PrefSetAppPreferences (CREATORID, addrPrefRcntID, 2, &prefs,
						   prefsSize, true);
	TrimRecentDB(gMaxRecent);*/
}

/*void SetEnabled(UInt16 enabled)
{
	CtlSetValue(FrmGetObjectPtr (FrmGetFormPtr(RecentOptionsDialog), FrmGetObjectIndex(FrmGetFormPtr(RecentOptionsDialog), RecentOptionsEnableCheckbox)), enabled);
	if(enabled)
	{
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), RecentOptionsNumberLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), RecentOptionsNumberTrigger));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), RecentOptionsShowAll));
	}
	else
	{
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), RecentOptionsNumberLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), RecentOptionsNumberTrigger));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), RecentOptionsShowAll));
	}
}*/

void ROInitForm()
{
	/*Int16 prefsVersion;
	UInt16 prefsSize;
	Boolean showAll=false;
	AddrRecentPreferenceType prefs;
	Boolean enabled=false;
	UInt16 number=0;
	ListType *recentListPtr=(ListType*)GetObjectPtrSmp(RecentOptionsNumberList);
	
	prefsSize = 0;
	PrefGetAppPreferences (CREATORID, addrPrefRcntID, NULL, &prefsSize, true);
	if(prefsSize>0)
	{
	prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefRcntID, &prefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	}
	if (prefsVersion != noPreferenceFound)
	{
		enabled=prefs.Enabled;
		number=prefs.Number;
		if(prefsVersion==1)
		{
			showAll=false;
		}
		else
		{
			showAll=prefs.ShowAll;
		}
	}	
	
	
	if(number!=5 && number!=10 && number!=20 && number!=50 && number!=100)
		number=5; 
	StrIToA(gROStr, number);
	
	switch(number)
	{
		case 5:
			LstSetSelection(recentListPtr, 0);
			break;
		case 10:
			LstSetSelection(recentListPtr, 1);
			break;
		case 20:
			LstSetSelection(recentListPtr, 2);
			break;
		case 50:
			LstSetSelection(recentListPtr, 3);
			break;
		case 100:
			LstSetSelection(recentListPtr, 4);
			break;	
	}
	CustomSetCtlLabelPtrSmp(RecentOptionsNumberTrigger, gROStr);
	CtlSetValue(FrmGetObjectPtr (FrmGetFormPtr(RecentOptionsDialog), FrmGetObjectIndex(FrmGetFormPtr(RecentOptionsDialog), RecentOptionsShowAll)), showAll);
	
	
	SetEnabled(enabled);
	if(gTreo)
		PrefSetPreference(prefHard1CharAppCreator, gDialerCreatorID);*/			
}

/*void RecentOnEnabledSelector()
{
	UInt16 enabled=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), RecentOptionsEnableCheckbox));
	SetEnabled(enabled);
}
*/
Boolean ROHandleEvent (EventType * event)
{
	/*Boolean handled = false;
	FormPtr frm;

	if (event->eType == ctlSelectEvent)
	{
		switch (event->data.ctlSelect.controlID)
		{
		case RecentOptionsEnableCheckbox:
			RecentOnEnabledSelector();
			handled=true;
			break;
		case RecentOptionsOkButton:
			if(gTreo)
				PrefSetPreference(prefHard1CharAppCreator, CREATORID);
			ROSave();
			FrmUpdateForm (ListView, updateRecent);
			FrmReturnToForm(0);
			handled = true;
			break;
		case RecentOptionsCancelButton:
			if(gTreo)
				PrefSetPreference(prefHard1CharAppCreator, CREATORID);
			FrmReturnToForm(0);
			handled=true;
			break;
		default:
			break;
		}
	}
	else if(event->eType==keyDownEvent)
	{
		if(IsFiveWayNavEvent(event) && !gNavigation)
		{
			if (NavKeyPressed(event, Select))
			{
				ROSave();
				FrmUpdateForm (ListView, updateRecent);
				FrmReturnToForm(0);
				handled=true;
			}		   	 	
		}  
		else if (EvtKeydownIsVirtual(event))
		{
			switch (event->data.keyDown.chr)
			{
				case vchrJogBack:
					FrmReturnToForm(0);
					handled=true;
					break;
				case vchrJogPush:
					ROSave();
					FrmUpdateForm (ListView, updateRecent);
					FrmReturnToForm(0);
					handled = true;
					break;
				default:
					break;
			}
		}
	}
	else if (event->eType == frmOpenEvent)
	{
		frm = FrmGetFormPtr(RecentOptionsDialog);
		RecentDIAInit(frm);
		FrmDrawForm (frm);
		ROInitForm();
		handled = true;
	}
	
	return (handled);*/
}
