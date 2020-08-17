#include <PalmOS.h>
#include "ColorOptions.h"
#include "AddrPrefs.h"
#include "Address.h"
#include "globals.h"
#include "AddrTools.h"
#include "AddrTools2.h"
#include "AddrDefines.h"
#include "dia.h"
#include "syslog.h"

#define THEME_ADDRESSXT 0
#define THEME_AUTUMN 1
#define THEME_BRICK 2
#define THEME_GRAYSCALE 3
#define THEME_INVERTED 4
#define THEME_MILD 5
#define THEME_MILITARY 6
#define THEME_CUSTOM 7


// public
Boolean COHandleEvent (EventType *event);

// private
static void COInitForm();
static void COSave();
static void ColorOnTextSelector();
static void ColorOnBackSelector();
static void ColorOnDefaultsSelector();
static void ColorOnEachOtherSelector();
static void DrawColorRects();
static void ColorOnSelSelector();
static void ColorOnInactiveSelSelector();
static void ColorOnEachOther();
static void COSelectTheme(UInt16 theme);
static void SetThemeLabel();


static void COSelectTheme(UInt16 theme)
{
	globalVars* globals = getGlobalsPtr();
	
	globals->gTheme = theme;

	CtlSetLabel(CustomGetObjectPtrSmp(ColorThemeTrigger), LstGetSelectionText(CustomGetObjectPtrSmp(ColorThemeList), theme));
	
	if(theme == THEME_CUSTOM)
		return;

	FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorTextLabel));
	FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorBackLabel));
	FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorSelLabel));
	FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorTextSelector));
	FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorBackSelector));
	FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorSelSelector));
	FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOtherLabel));
	FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOtherSelector));
	FrmSetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorDefaults), false);
	FrmSetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOther), true);
	switch(theme)
	{
		case THEME_ADDRESSXT:
			globals->gColorText = 255;
			globals->gColorSel = 77;
			globals->gEachOther = 110;
			globals->gColorBack = 0;
			globals->gInactiveSel = 222;
			break;
		case THEME_AUTUMN:
			globals->gColorText = 255;
			globals->gColorSel = 111;
			globals->gEachOther = 108;
			globals->gColorBack = 30;
			globals->gInactiveSel = 219;
			break;
		case THEME_BRICK:
			globals->gColorText = 255;
			globals->gColorSel = 119;
			globals->gEachOther = 147;
			globals->gColorBack = 6;
			globals->gInactiveSel = 222;
			break;
		case THEME_GRAYSCALE:
			globals->gColorText = 255;
			globals->gColorSel = 217;
			globals->gEachOther = 222;
			globals->gColorBack = 223;
			globals->gInactiveSel = 220;
			break;
		case THEME_INVERTED:
			globals->gColorText = 0;
			globals->gColorSel = 64;
			globals->gEachOther = 218;
			globals->gColorBack = 255;
			globals->gInactiveSel = 216;
			break;
		case THEME_MILD:
			globals->gColorText = 255;
			globals->gColorSel = 14;
			globals->gEachOther = 13;
			globals->gColorBack = 6;
			globals->gInactiveSel = 222;
			break;
		case THEME_MILITARY:
			globals->gColorText = 255;
			globals->gColorSel = 183;
			globals->gEachOther = 153;
			globals->gColorBack = 164;
			globals->gInactiveSel = 222;
			break;
	}	
	DrawColorRects();
}

static void SetThemeLabel()
{
	globalVars* globals = getGlobalsPtr();
	EventType event;
	
	if(globals->gColorText == 255 && globals->gColorSel == 77
	 && globals->gEachOther == 110 && globals->gColorBack == 0 
	 && globals->gInactiveSel == 222)
		globals->gTheme = THEME_ADDRESSXT;
	else if(globals->gColorText == 255 && globals->gColorSel == 111
	 && globals->gEachOther == 108 && globals->gColorBack == 30
	  && globals->gInactiveSel == 219)
		globals->gTheme = THEME_AUTUMN;	
	else if(globals->gColorText == 255 && globals->gColorSel == 119
	 && globals->gEachOther == 147 && globals->gColorBack == 6
	  && globals->gInactiveSel == 222)
		globals->gTheme = THEME_BRICK;	
	else if(globals->gColorText == 255 && globals->gColorSel == 217
	 && globals->gEachOther == 222 && globals->gColorBack == 223
	  && globals->gInactiveSel == 220)
		globals->gTheme = THEME_GRAYSCALE;	
	else if(globals->gColorText == 0 && globals->gColorSel == 64
	 && globals->gEachOther == 218 && globals->gColorBack == 255
	  && globals->gInactiveSel == 216)
		globals->gTheme = THEME_INVERTED;	
	else if(globals->gColorText == 255 && globals->gColorSel == 14
	 && globals->gEachOther == 13 && globals->gColorBack == 6
	  && globals->gInactiveSel == 222)
		globals->gTheme = THEME_MILD;	
	else if(globals->gColorText == 255 && globals->gColorSel == 183
	 && globals->gEachOther == 153 && globals->gColorBack == 164
	  && globals->gInactiveSel == 222)
		globals->gTheme = THEME_MILITARY;	
	else
		globals->gTheme = THEME_CUSTOM;
	
	
	event.eType = popSelectEvent;
	event.data.popSelect.controlID = ColorThemeTrigger;
	event.data.popSelect.controlP = CustomGetObjectPtrSmp(ColorThemeTrigger);
	event.data.popSelect.listID = ColorThemeList;
	event.data.popSelect.listP = CustomGetObjectPtrSmp(ColorThemeList);
	event.data.popSelect.selection = globals->gTheme;
	event.data.popSelect.priorSelection = globals->gTheme;
	
	EvtAddEventToQueue(&event);
	
	LstSetSelection(CustomGetObjectPtrSmp(ColorThemeList), globals->gTheme);	
}

static void DrawColorRects()
{
	globalVars* globals = getGlobalsPtr();
	RectangleType rect;
	IndexedColorType colPrev;
	Boolean defaults=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorDefaults));
	Boolean eachOther=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOther));
	
	WinSetDrawWindow(FrmGetWindowHandle(FrmGetFormPtr(ColorOptionsDialog)));
	if(!defaults)
	{
		FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorBackSelector), &rect);
		colPrev=WinSetForeColor(globals->gColorBack);
		WinDrawRectangle(&rect, 0);
		WinSetForeColor(colPrev);
		FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorTextSelector), &rect);
		WinSetForeColor(globals->gColorText);
		WinDrawRectangle(&rect, 0);
		WinSetForeColor(colPrev);
		FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorSelSelector), &rect);
		WinSetForeColor(globals->gColorSel);
		WinDrawRectangle(&rect, 0);
		WinSetForeColor(colPrev);
		FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorInactiveSelector), &rect);
		WinSetForeColor(globals->gInactiveSel);
		WinDrawRectangle(&rect, 0);
		WinSetForeColor(colPrev);
	}
	if(eachOther)
	{
		FrmGetObjectBounds(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOtherSelector), &rect);
		colPrev=WinSetForeColor(globals->gEachOther);
		WinDrawRectangle(&rect, 0);
		WinSetForeColor(colPrev);
	}
}


static void ColorOnDefaultsSelector()
{
	globalVars* globals = getGlobalsPtr();
	Boolean defaults=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorDefaults));
	FrmDrawForm(FrmGetActiveForm());
	DrawColorRects();
	if(defaults)
	{
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorTextLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorBackLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorSelLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorInactiveLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorTextSelector));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorBackSelector));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorSelSelector));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorInactiveSelector));
	}
	else
	{
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorTextLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorBackLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorSelLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorInactiveLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorTextSelector));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorBackSelector));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorSelSelector));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorInactiveSelector));
		{
			DrawColorRects();
		}	
	}
	globals->gTheme = THEME_CUSTOM;
	LstSetSelection(CustomGetObjectPtrSmp(ColorThemeList), globals->gTheme);
	CustomSetCtlLabelPtrSmp(ColorThemeTrigger, LstGetSelectionText(CustomGetObjectPtrSmp(ColorThemeList), globals->gTheme));
}

static void ColorOnEachOther()
{
	globalVars* globals = getGlobalsPtr();
	Boolean eachOther=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOther));
	FrmDrawForm(FrmGetActiveForm());
	DrawColorRects();
	if(!eachOther)
	{
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOtherLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOtherSelector));
	}
	else
	{
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOtherLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOtherSelector));
		DrawColorRects();
	}
	globals->gTheme = THEME_CUSTOM;
	LstSetSelection(CustomGetObjectPtrSmp(ColorThemeList), globals->gTheme);
	CustomSetCtlLabelPtrSmp(ColorThemeTrigger, LstGetSelectionText(CustomGetObjectPtrSmp(ColorThemeList), globals->gTheme));	
}

// common code - for color selection dialog
static void ColorSelection(IndexedColorType *gColor, Char *label)
{
	globalVars* globals = getGlobalsPtr();
	RGBColorType color;
	RectangleType rectAll;
	Boolean colSet;
	UInt16 err;
	WinHandle offScr;
	
	// save window (why?)
	WinSetDrawWindow(WinGetDisplayWindow());
	WinGetBounds(WinGetDisplayWindow(), &rectAll);
	offScr = WinSaveBits(&rectAll, &err);
	
	colSet = UIPickColor(gColor, NULL, UIPickColorStartPalette, label, NULL);

	if(!err)
	{
		WinSetDrawWindow(WinGetDisplayWindow());
		WinRestoreBits(offScr, 0, 0);
	}
	
	if(!colSet)
	{
		DrawColorRects();
		// need return?
	}
	
	// set global vars
	
	WinIndexToRGB(globals->gSavedColorText, &color);
	
	UIColorSetTableEntry(UIObjectForeground, &color);
	UIColorSetTableEntry(UIFieldText, &color);
	
	WinIndexToRGB(globals->gSavedColorBack, &color);
	
	UIColorSetTableEntry(UIFieldBackground, &color);
	UIColorSetTableEntry(UIFormFill, &color);
	UIColorSetTableEntry(UIAlertFill, &color);
	UIColorSetTableEntry(UIDialogFill, &color);
	UIColorSetTableEntry(UIObjectFill, &color);
	
	WinIndexToRGB(globals->gSavedColorSel, &color);
	
	UIColorSetTableEntry(UIObjectSelectedFill, &color);
	UIColorSetTableEntry(UIFieldTextHighlightForeground, &color);
	
	if(colSet)
	{
		EventType event;
		event.eType = popSelectEvent;
		event.data.popSelect.controlID = ColorThemeTrigger;
		event.data.popSelect.controlP = CustomGetObjectPtrSmp(ColorThemeTrigger);
		event.data.popSelect.listID = ColorThemeList;
		event.data.popSelect.listP = CustomGetObjectPtrSmp(ColorThemeList);
		event.data.popSelect.selection = THEME_CUSTOM;
		event.data.popSelect.priorSelection = THEME_CUSTOM - 1;
		
		globals->gTheme = THEME_CUSTOM;
		
		EvtAddEventToQueue(&event);
		
		FrmDrawForm(FrmGetActiveForm());
		DrawColorRects();
		
		LstSetSelection(CustomGetObjectPtrSmp(ColorThemeList), globals->gTheme);
	}
}



static void ColorOnSelSelector()
{
	globalVars* globals = getGlobalsPtr();
	ColorSelection(&(globals->gColorSel), "Pick selection color");
}

static void ColorOnInactiveSelSelector()
{
	globalVars* globals = getGlobalsPtr();
	ColorSelection(&(globals->gInactiveSel), "Pick inactive selection color");
}

static void ColorOnEachOtherSelector()
{
	globalVars* globals = getGlobalsPtr();
	ColorSelection(&(globals->gEachOther), "Pick each other row color");
}

static void ColorOnBackSelector()
{
	globalVars* globals = getGlobalsPtr();
	ColorSelection(&(globals->gColorBack), "Pick background color");
}
	
static void ColorOnTextSelector()
{
	globalVars* globals = getGlobalsPtr();
	ColorSelection(&(globals->gColorText), "Pick text color");
}
					
static void COSave()
{
	globalVars* globals = getGlobalsPtr();
	UInt16 prefsSize;
	Boolean defaults=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorDefaults));
	Boolean eachOther=FrmGetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOther));
	AddrColorPreferenceType prefs;
	RGBColorType color;
	
	globals->gEachOtherSelected=eachOther;
	
	if(defaults)
	{
		globals->gColorBack=globals->gBackground;
		globals->gColorText=globals->gFieldText;
		globals->gColorSel=globals->gObjSelText;
		globals->gInactiveSel=globals->gObjSelText;
		WinIndexToRGB(globals->gBackground, &color);
		UIColorSetTableEntry(UIFieldBackground, &color);
		WinIndexToRGB(globals->gFill, &color);
		UIColorSetTableEntry(UIFormFill, &color);
		WinIndexToRGB(globals->gAlert,&color);
		UIColorSetTableEntry(UIAlertFill, &color);
		WinIndexToRGB(globals->gForeground, &color);
		UIColorSetTableEntry(UIObjectForeground, &color);
		WinIndexToRGB(globals->gDialogFill, &color);
		UIColorSetTableEntry(UIDialogFill, &color);
		WinIndexToRGB(globals->gObjectFill, &color);
		UIColorSetTableEntry(UIObjectFill, &color);
		WinIndexToRGB(globals->gFieldText, &color);
		UIColorSetTableEntry(UIFieldText, &color);
		WinIndexToRGB(globals->gObjSelText, &color);
		UIColorSetTableEntry(UIObjectSelectedFill, &color);
		WinIndexToRGB(globals->gFldSelText, &color);
		UIColorSetTableEntry(UIFieldTextHighlightBackground , &color);
	}
	else
	{
		WinIndexToRGB(globals->gColorText, &color);
		UIColorSetTableEntry(UIObjectForeground, &color);
		UIColorSetTableEntry(UIFieldText, &color);
		WinIndexToRGB(globals->gColorBack, &color);
		UIColorSetTableEntry(UIFieldBackground, &color);
		UIColorSetTableEntry(UIFormFill, &color);
		UIColorSetTableEntry(UIAlertFill, &color);
		UIColorSetTableEntry(UIDialogFill, &color);
		UIColorSetTableEntry(UIObjectFill, &color);
		WinIndexToRGB(globals->gColorSel, &color);
		
		UIColorSetTableEntry(UIObjectSelectedFill, &color);
		UIColorSetTableEntry(UIFieldTextHighlightForeground, &color);
	}
	// Write the state information.
	prefs.backColor=globals->gColorBack;
	prefs.textColor=globals->gColorText;
	prefs.selColor=globals->gColorSel;
	prefs.inactiveSelColor=globals->gInactiveSel;
	if(!eachOther)
	{
		prefs.eachOtherColor=globals->gEachOther;
		prefs.eachOther=eachOther;
	}
	else
	{
		prefs.eachOtherColor=globals->gEachOther;
		prefs.eachOther=eachOther;
	}
	prefs.defaults=defaults;
	prefs.eachOther=eachOther;
	prefs.recentColor=0;
	prefs.reserv6=0;
	prefs.reserv7=0;
	
	prefsSize=sizeof(AddrColorPreferenceType);
	
	PrefSetAppPreferences (CREATORID, addrPrefColorID, 2, &prefs,
						   prefsSize, true);
	
	FrmUpdateForm(globals->gColorFormID, updateColorsChanged);
}

static void COInitForm()
{
	globalVars* globals = getGlobalsPtr();
	Int16 prefsVersion;
	UInt16 prefsSize=0;
	Boolean defaults, eachOther;
	AddrColorPreferenceType prefs;
	
	PrefGetAppPreferences (CREATORID, addrPrefColorID, NULL, &prefsSize, true);
	if(prefsSize>0)
	{
		prefsVersion = PrefGetAppPreferences (CREATORID, addrPrefColorID, &prefs, &prefsSize, true);
	}
	else
	{
		prefsVersion=noPreferenceFound;
	}
	if (prefsVersion != noPreferenceFound)
	{
		globals->gSavedColorText = globals->gColorText=prefs.textColor;
		globals->gSavedColorBack = globals->gColorBack=prefs.backColor;
		globals->gSavedColorSel = globals->gColorSel=prefs.selColor;
		globals->gSavedInactiveSel = globals->gInactiveSel=prefs.inactiveSelColor;
		globals->gSavedEachOther = globals->gEachOther=prefs.eachOtherColor;
		defaults=prefs.defaults;
		eachOther=prefs.eachOther;
	}
	else
	{
		globals->gSavedColorText = globals->gColorText=UIColorGetTableEntryIndex(UIObjectForeground);
		globals->gSavedColorBack = globals->gColorBack=UIColorGetTableEntryIndex(UIFieldBackground);
		globals->gSavedColorSel = globals->gColorSel=UIColorGetTableEntryIndex(UIObjectSelectedFill);
		globals->gSavedEachOther = globals->gEachOther=UIColorGetTableEntryIndex(UIObjectSelectedFill);
		globals->gSavedInactiveSel = globals->gInactiveSel=UIColorGetTableEntryIndex(UIObjectSelectedFill);
		defaults=true;
		eachOther=false;
	}
	if(prefsVersion==1)
	{
		globals->gSavedColorSel = globals->gColorSel=globals->gObjSelText;
		eachOther=false;
		globals->gSavedEachOther = globals->gEachOther=globals->gColorSel;
	}
	if(globals->gDepth > 4)
	{
		CustomShowObjectSmp(ColorThemeLabel);
		CustomShowObjectSmp(ColorThemeTrigger);		
		SetThemeLabel();
	}
	else
	{
		CustomHideObjectSmp(ColorThemeLabel);
		CustomHideObjectSmp(ColorThemeTrigger);		
	}
	if(defaults)
	{
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorTextLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorBackLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorSelLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorInactiveLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorTextSelector));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorBackSelector));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorSelSelector));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorInactiveSelector));
	}
	else
	{
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorTextLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorBackLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorSelLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorInactiveLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorTextSelector));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorBackSelector));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorSelSelector));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorInactiveSelector));
	}
	if(eachOther)
	{
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOtherLabel));
		FrmShowObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOtherSelector));
	}
	else
	{
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOtherLabel));
		FrmHideObject(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOtherSelector));
	}
	
	
	FrmSetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorDefaults), defaults);
	FrmSetControlValue(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), ColorEachOther), eachOther);
	DrawColorRects();
	RestoreDialer();	

}

static Boolean ColorOptionsOnCtlSelectEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;

	switch (event->data.ctlSelect.controlID)
	{
		case ColorDefaults:
			ColorOnDefaultsSelector();
			handled=true;
			break;
		case ColorEachOther:
			ColorOnEachOther();
			handled=true;
			break;
		case ColorTextSelector:
			ColorOnTextSelector();
			handled=true;
			break;
		case ColorSelSelector:
			ColorOnSelSelector();
			handled=true;
			break;
		case ColorInactiveSelector:
			ColorOnInactiveSelSelector();
			handled=true;
			break;
		case ColorEachOtherSelector:
			ColorOnEachOtherSelector();
			handled=true;
			break;
		case ColorBackSelector:
			ColorOnBackSelector();
			handled=true;
			break;
		case ColorOptionsOkButton:
			COSave();
			SetDialer();
			FrmReturnToForm(0);
			dia_restore_state();
#ifdef DEBUG
			LogWrite("xt_log", "color", "ok btn");
#endif
			handled = true;
			break;
		case ColorOptionsCancelButton:
			SetDialer();
			{
				RGBColorType color;
				globals->gEachOther = globals->gSavedEachOther;
				globals->gColorBack = globals->gSavedColorBack;
				globals->gColorText = globals->gSavedColorText;
				globals->gColorSel = globals->gSavedColorSel;
				WinIndexToRGB(globals->gColorText, &color);
				UIColorSetTableEntry(UIObjectForeground, &color);
				UIColorSetTableEntry(UIFieldText, &color);
				WinIndexToRGB(globals->gColorBack, &color);
				UIColorSetTableEntry(UIFieldBackground, &color);
				UIColorSetTableEntry(UIFormFill, &color);
				UIColorSetTableEntry(UIAlertFill, &color);
				UIColorSetTableEntry(UIDialogFill, &color);
				UIColorSetTableEntry(UIObjectFill, &color);
				WinIndexToRGB(globals->gColorSel, &color);
				
				UIColorSetTableEntry(UIObjectSelectedFill, &color);
				UIColorSetTableEntry(UIFieldTextHighlightForeground, &color);
			}
			FrmReturnToForm(0);
			dia_restore_state();
			handled=true;
			break;		
		default:
			break;
	}
	return handled;
}

static Boolean ColorOptionsOnFrmOpenEvent()
{
	FormPtr frm = FrmGetFormPtr(ColorOptionsDialog);
	dia_save_state(); 
	dia_enable(frm, false);
	FrmDrawForm(frm);
	COInitForm();
	return true;
}

static Boolean ColorOptionsOnKeyDownEvent(EventType* event)
{
	Boolean handled = false;
	globalVars* globals = getGlobalsPtr();
	if(IsFiveWayNavEvent(event))
	{
		if (NavKeyPressed(event, Select) && !globals->gNavigation)
		{
			COSave();
			FrmReturnToForm(0);
			dia_restore_state();
			return true;
		}		   	 	
	}  
	return handled;
}

Boolean COHandleEvent (EventType * event)
{
	Boolean handled = false;

	switch(event->eType)
	{
		case ctlSelectEvent:
			handled = ColorOptionsOnCtlSelectEvent(event);
			break;
		case frmUpdateEvent:
			handled=true;
			FrmDrawForm(FrmGetActiveForm());
			DrawColorRects();
			break;
		case popSelectEvent:
			COSelectTheme(event->data.popSelect.selection);
			handled = false;
			break;
		case frmOpenEvent:
			handled = ColorOptionsOnFrmOpenEvent();
			break;
		case keyDownEvent:
			handled = ColorOptionsOnKeyDownEvent(event);
			break;
		default:
			handled = dia_handle_event(event, NULL);
			break;
	}
	
	return (handled);
}
