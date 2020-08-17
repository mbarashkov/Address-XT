#include <PalmOS.h>

#include "AddrDefines.h"
#include "globals.h"
#include "AddressRsc.h"
#include "dia.h"
#include "syslog.h"
#include "AddrTools.h"
#include "Address.h"

#include "AddrFont.h"

static void AddrFontDeleteBitmaps(BitmapType *bmp, BitmapTypeV3 *bmpV3);
static Boolean AddrFontMakeHRBitmap(BitmapType *bmp, BitmapTypeV3 *bmpV3);
static Boolean AddrFontSelectLowRes(FontID currFontID, FontID *newFontID);
static Boolean AddrFontSelectHiRes(FontID currFontID, FontID *newFontID, Boolean *hires);
static void AddrFontDrawGadget(struct FormGadgetTypeInCallback *gadget, Boolean selected);
static Boolean FontLowResOnKeyDown(EventPtr event);
static Boolean FontHiResOnKeyDown(EventPtr event);
static Boolean FontHiResHandleEvent (EventType * event);
static Boolean FontLowResHandleEvent (EventType * event);


// public

Boolean AddressFontSelect(FontID currFontID, FontID *newFontID, Boolean *hires, Boolean withHires)
{
	globalVars* globals = getGlobalsPtr();
	Boolean changed = false;
	
	// if PALM_320x320 -- make other dialog
	if(globals->gScreen == PALM_320x320 && withHires)
		changed = AddrFontSelectHiRes(currFontID, newFontID, hires);
	else
		changed = AddrFontSelectLowRes(currFontID, newFontID);
	
	//changed = globals->fontDialogStatus;
	//*newFontID = globals->fontDialogFontID;
	//*hires = globals->fontDialogHiRes;
	
	return changed;
}


//===== private =====

// cut'&'paste from ToolsWinDrawCharsHD
static Boolean AddrFontMakeHRBitmap(BitmapType *bmp, BitmapTypeV3 *bmpV3)
{
	globalVars* globals = getGlobalsPtr();
	WinHandle offWindow, prevWindow;
	Boolean result = false;
	Err err = errNone;
	UInt16 bmpSize;
	void *bmpData;
	
	if(globals->gScreen == PALM_320x320)
	{
		WinPushDrawState();
					
		bmp = BmpCreate(FntCharWidth('W'), FntLineHeight(), globals->gDepth, NULL, &err);
		if(bmp == NULL) return false;
		bmpData = BmpGetBits(bmp);
		bmpSize = BmpBitsSize(bmp);
		//create bmp v3
		bmpV3 = BmpCreateBitmapV3(bmp, kDensityLow, bmpData, NULL);
		if(bmpV3 == NULL)
		{			
			AddrFontDeleteBitmaps(bmp, bmpV3);
			return false;
		}
		// draw on it
		offWindow = WinCreateBitmapWindow((BitmapType*)bmpV3, &err);
		if(offWindow == 0)
		{
			AddrFontDeleteBitmaps(bmp, bmpV3);
			return false;
		}
		
		prevWindow = WinSetDrawWindow(offWindow);
		
		WinSetCoordinateSystem(kCoordinatesStandard);
		WinDrawChar('W', 0, 0);
		BmpSetDensity((BitmapType*)bmpV3, kDensityDouble);
		
		WinSetDrawWindow(prevWindow);
		
		WinPopDrawState();
	}
	
	return result;
}

// clean
static void AddrFontDeleteBitmaps(BitmapType *bmp, BitmapTypeV3 *bmpV3)
{
	if(bmpV3) BmpDelete((BitmapType*)bmpV3);
	if(bmp) BmpDelete(bmp);
}



// util
static FontID AddrFontGetGadgetFont(UInt16 gadget_id)
{
	FontID font;
	
	switch(gadget_id)
	{
		case FontSelectHRFont1:
		case FontSelectHRFont5:
			font = stdFont;
			break;
		case FontSelectHRFont2:
		case FontSelectHRFont6:
			font = boldFont;
			break;
		case FontSelectHRFont3:
		case FontSelectHRFont7:
			font = largeFont;
			break;
		case FontSelectHRFont4:
		case FontSelectHRFont8:
			font = largeBoldFont;
			break;
	}
	
	return font;
}

// draw one gadget
static void AddrFontDrawGadget(struct FormGadgetTypeInCallback *gadget, Boolean selected)
{
	globalVars* globals = getGlobalsPtr();
	
	RectangleType rect;
	FontID prev_font, new_font;
	
	rect = gadget->rect;
	new_font = AddrFontGetGadgetFont(gadget->id);
			
	WinPushDrawState();
			
	prev_font = FntSetFont(new_font);
	if(selected)
	{
		WinSetBackColor( UIColorGetTableEntryIndex(UIObjectSelectedFill));
		
		WinSetForeColor( UIColorGetTableEntryIndex(UIObjectSelectedFill));
		WinDrawRectangle(&rect, 0);
		WinSetTextColor( UIColorGetTableEntryIndex(UIFormFill));
	}
	else 
	{
		WinSetBackColor( UIColorGetTableEntryIndex(UIObjectFill));
		WinSetForeColor( UIColorGetTableEntryIndex(UIObjectFill));
		WinDrawRectangle(&rect, 0);
		WinSetTextColor( UIColorGetTableEntryIndex(UIObjectForeground));
	}
				
	if(gadget->id > FontSelectHRFont4)
	{
		// standard
		WinDrawChar('A', rect.topLeft.x + (rect.extent.x - FntCharWidth('A')) / 2, 
						rect.topLeft.y + (rect.extent.y - FntCharHeight()) / 2 );
	}
	else 
	{
		// hi-res
		ToolsUnivWinDrawCharsHD(globals->adxtLibRef, "A", 1, rect.topLeft.x + (rect.extent.x - FntCharWidth('A') / 2) / 2, 
									rect.topLeft.y + (rect.extent.y - FntCharHeight() / 2) / 2 );
	}
	FntSetFont(prev_font);
			
	WinSetForeColor( UIColorGetTableEntryIndex(UIObjectFrame) );
	WinDrawRectangleFrame(simpleFrame, &rect);
			
	WinPopDrawState();	
}


	

// gadget's callback
static Boolean AddrFontCallback(struct FormGadgetTypeInCallback *gadget, UInt16 cmd, void *paramP)
{
	globalVars* globals = getGlobalsPtr();
	Boolean handled = false;
	EventType *event;
	Boolean hiRes = false;
	
	switch(cmd)
	{
		case formGadgetDrawCmd:			
			AddrFontDrawGadget(gadget, gadget->id == globals->selection);
			break;
		
		case formGadgetHandleEventCmd:
			event = (EventType *) paramP;

			if(event->eType == frmGadgetEnterEvent)
			{
				UInt16 ticksPerSec = SysTicksPerSecond();
				Boolean isPenDown = true;
				Int16 newPointX, newPointY;
				FormGadgetTypeInCallback selectedGadget;
				FormPtr frm;
				Boolean penInGadget;
				UInt16 focus = noFocus;
				//save focus ring info
				frm = FrmGetActiveForm();
				if(globals->gNavigation)
				{
					if(FrmGlueNavGetFocusRingInfo (globals->adxtLibRef, frm, &focus, NULL, NULL, NULL) != errNone)
       					focus = noFocus;
	 			}
				
				// PenDownEvent - wait for PenUpEvent
				
				selectedGadget.id = globals->selection;
				FrmGetObjectBounds(frm, FrmGetObjectIndex(frm, globals->selection), &selectedGadget.rect);
				
				if(gadget->id != globals->selection)
				{
					RectangleType rect;
					AddrFontDrawGadget(&selectedGadget, false);
					AddrFontDrawGadget(gadget, true);
					if(focus >= FontSelectHRFont1 && focus <= FontSelectHRFont8)
					{
						FrmGetObjectBounds (FrmGetActiveForm(),	FrmGetObjectIndex(FrmGetActiveForm(), focus), &rect); 
						FrmGlueNavDrawFocusRing(globals->adxtLibRef, FrmGetActiveForm(), focus, frmNavFocusRingNoExtraInfo, &rect, frmNavFocusRingStyleSquare, false);
					}
				}
				
				penInGadget = true;
				while(isPenDown)
				{ 
					EvtGetPen(&newPointX, &newPointY, &isPenDown);
					if( RctPtInRectangle(newPointX, newPointY, &(gadget->rect)) )
					{
						if(! penInGadget)
						{
							AddrFontDrawGadget(gadget, true);
							penInGadget = true;
						}
					}
					else if(penInGadget)
					{
						RectangleType rect;
						AddrFontDrawGadget(gadget, false);
						penInGadget = false;
						if(focus >= FontSelectHRFont1 && focus <= FontSelectHRFont8)
						{
							FrmGetObjectBounds (FrmGetActiveForm(),	FrmGetObjectIndex(FrmGetActiveForm(), focus), &rect); 
							FrmGlueNavDrawFocusRing(globals->adxtLibRef, FrmGetActiveForm(), focus, frmNavFocusRingNoExtraInfo, &rect, frmNavFocusRingStyleSquare, false);
						}
					}
				}
				
				if(penInGadget)
				{
					// tap
					globals->selection = gadget->id;				
				}
				else
				{
					if(gadget->id != globals->selection) 
						AddrFontDrawGadget(gadget, false);
					
					AddrFontDrawGadget(&selectedGadget, true);
				}	
				//restore focus ring info
				if(globals->gNavigation && focus != noFocus)
				{
					RectangleType rect;
					if(focus >= FontSelectHRFont1 && focus <= FontSelectHRFont8)
					{
						FrmGetObjectBounds (FrmGetActiveForm(),	FrmGetObjectIndex(FrmGetActiveForm(), focus), &rect); 
						FrmGlueNavDrawFocusRing(globals->adxtLibRef, FrmGetActiveForm(), focus, frmNavFocusRingNoExtraInfo, &rect, frmNavFocusRingStyleSquare, false);
					}
				}			
			}
			handled = true;
			break;
			
		default:
			break;
	}
	
	return handled;
}

static Boolean FontHiResOnFrmObjectFocusTakeEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 objID = event->data.frmObjectFocusTake.objectID;
	if(globals->gNavigation)
	{
		if(objID >= FontSelectHRFont1 && objID <= FontSelectHRFont8)
		{
			RectangleType rect;
			FrmGetObjectBounds (FrmGetActiveForm(),	FrmGetObjectIndex(FrmGetActiveForm(), objID), &rect); 
			FrmGlueNavDrawFocusRing(globals->adxtLibRef, FrmGetActiveForm(), objID, frmNavFocusRingNoExtraInfo, &rect, frmNavFocusRingStyleSquare, false);
		}
	}
	return false;
}

static Boolean FontLowResOnFrmObjectFocusTakeEvent(EventType* event)
{
	globalVars* globals = getGlobalsPtr();
	UInt16 objID = event->data.frmObjectFocusTake.objectID;
	if(globals->gNavigation)
	{
		if(objID >= FontSelectFont1 && objID <= FontSelectFont4)
		{
			RectangleType rect;
			FrmGetObjectBounds (FrmGetActiveForm(),	FrmGetObjectIndex(FrmGetActiveForm(), objID), &rect); 
			FrmGlueNavDrawFocusRing(globals->adxtLibRef, FrmGetActiveForm(), objID, frmNavFocusRingNoExtraInfo, &rect, frmNavFocusRingStyleSquare, false);
		}
	}
	return false;
}

static Boolean FontHiResOnKeyDown(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	if(IsFiveWayNavEvent(event) && globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			UInt16 focusIndex = FrmGetFocus(FrmGetActiveForm());
			UInt16 objID;
			if(focusIndex == noFocus)
				return false;
			objID = FrmGetObjectId(FrmGetActiveForm(), focusIndex);
			if(objID >= FontSelectHRFont1 && objID <= FontSelectHRFont8)
			{
				FormGadgetTypeInCallback selectedGadget;
				RectangleType rect;
				FrmGlueNavRemoveFocusRing(globals->adxtLibRef,  FrmGetActiveForm());
				
				FrmGetObjectBounds (FrmGetActiveForm(),	FrmGetObjectIndex(FrmGetActiveForm(), globals->selection), &rect); 
				
				selectedGadget.id = globals->selection;
				selectedGadget.rect = rect;
				AddrFontDrawGadget(&selectedGadget, false);
				
				globals->selection = objID;
				FrmGetObjectBounds (FrmGetActiveForm(),	FrmGetObjectIndex(FrmGetActiveForm(), globals->selection), &rect); 
				selectedGadget.id = globals->selection;
				selectedGadget.rect = rect;
				AddrFontDrawGadget(&selectedGadget, true);
				FrmGlueNavDrawFocusRing(globals->adxtLibRef, FrmGetActiveForm(), objID, frmNavFocusRingNoExtraInfo, &rect, frmNavFocusRingStyleSquare, false);
			}
		}
	}
	return false;
}

static Boolean FontLowResOnKeyDown(EventPtr event)
{
	globalVars* globals = getGlobalsPtr();
	if(IsFiveWayNavEvent(event) && globals->gNavigation)
	{
		if (ToolsNavKeyPressed(globals->adxtLibRef, event, Select))
		{
			UInt16 focusIndex = FrmGetFocus(FrmGetActiveForm());
			UInt16 objID;
			if(focusIndex == noFocus)
				return false;
			objID = FrmGetObjectId(FrmGetActiveForm(), focusIndex);
			if(objID >= FontSelectFont1 && objID <= FontSelectFont4)
			{
				FormGadgetTypeInCallback selectedGadget;
				RectangleType rect;
				FrmGlueNavRemoveFocusRing(globals->adxtLibRef,  FrmGetActiveForm());
				
				FrmGetObjectBounds (FrmGetActiveForm(),	FrmGetObjectIndex(FrmGetActiveForm(), globals->selection), &rect); 
				
				selectedGadget.id = globals->selection;
				selectedGadget.rect = rect;
				AddrFontDrawGadget(&selectedGadget, false);
				
				globals->selection = objID;
				FrmGetObjectBounds (FrmGetActiveForm(),	FrmGetObjectIndex(FrmGetActiveForm(), globals->selection), &rect); 
				selectedGadget.id = globals->selection;
				selectedGadget.rect = rect;
				AddrFontDrawGadget(&selectedGadget, true);
				FrmGlueNavDrawFocusRing(globals->adxtLibRef, FrmGetActiveForm(), objID, frmNavFocusRingNoExtraInfo, &rect, frmNavFocusRingStyleSquare, false);
			}
		}
	}
	return false;
}

Boolean FontHiResHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch (event->eType)
	{
		case frmObjectFocusTakeEvent:
			handled = FontHiResOnFrmObjectFocusTakeEvent(event);		
			break;
		case keyDownEvent:
			handled = FontHiResOnKeyDown(event);
			break;
		default:
			break;
	}
	return (handled);
}

Boolean FontLowResHandleEvent (EventType * event)
{
	Boolean handled = false;
	switch (event->eType)
	{
		case frmObjectFocusTakeEvent:
			handled = FontLowResOnFrmObjectFocusTakeEvent(event);		
			break;
		case keyDownEvent:
			handled = FontLowResOnKeyDown(event);
			break;
		default:
			break;
	}
	return (handled);
}

// low-res version
static Boolean AddrFontSelectLowRes(FontID currFontID, FontID *newFontID)
{
	FormPtr frm;
	FontID font = stdFont;
	UInt16 ctl, font_res, sel_index;
	Boolean status = false;

	switch(currFontID)
	{
		case stdFont:
			font_res = FontSelectFont1;
			break;
		case boldFont:
			font_res = FontSelectFont2;
			break;
		case largeFont:
			font_res = FontSelectFont3;
			break;
		case largeBoldFont:
			font_res = FontSelectFont4;
			break;
		default:
			font_res = FontSelectFont1;
			break;
	}
	
	frm = FrmInitForm(FontSelectDialog);
	FrmSetEventHandler(frm, FontLowResHandleEvent);
	dia_save_state(); 
	dia_enable(frm, false);
	// select current
	FrmSetControlGroupSelection(frm, FontSelectBtnGroup, font_res);
	
	ctl = FrmDoDialog(frm);
	
	dia_restore_state(); 

	if(ctl == FontSelectOKButton)
	{
		// what is selected
		sel_index = FrmGetControlGroupSelection(frm, FontSelectBtnGroup);
		if(sel_index != frmNoSelectedControl)
		{
			font_res = FrmGetObjectId(frm, sel_index);
			
			switch(font_res)
			{
				case FontSelectFont1:
					font = stdFont;
					break;
				case FontSelectFont2:
					font = boldFont;
					break;
				case FontSelectFont3:
					font = largeFont;
					break;
				case FontSelectFont4:
					font = largeBoldFont;
					break;
			}
			
			FntSetFont(font);
			
			*newFontID = font;
		}
		
		status = true;
	}
	
	// cleanup
	FrmDeleteForm(frm);
	
	return status;
}

// hi-res version
static Boolean AddrFontSelectHiRes(FontID currFontID, FontID *newFontID, Boolean *hires)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm;
	FontID font = currFontID;
	UInt16 ctl;
	UInt16 i;
	Boolean status = false;

	switch(font)
	{
		case stdFont:
			globals->selection = FontSelectHRFont1;
			break;
		case boldFont:
			globals->selection = FontSelectHRFont2;
			break;
		case largeFont:
			globals->selection = FontSelectHRFont3;
			break;
		case largeBoldFont:
			globals->selection = FontSelectHRFont4;
			break;
	}
	// small - the first
	if(! *hires)
		globals->selection += 4;
		
	frm = FrmInitForm(FontSelectHRDialog);
	for(i = FontSelectHRFont1; i <= FontSelectHRFont8; i++)
	{
		FrmSetGadgetHandler(frm, FrmGetObjectIndex(frm, i), AddrFontCallback);
	}	
	
	FrmSetEventHandler(frm, FontHiResHandleEvent);
	
	dia_save_state(); 
	dia_enable(frm, false);
	
	ctl = FrmDoDialog(frm);
	
	dia_restore_state(); 

	if(ctl == FontSelectHROKButton)
	{
		font = AddrFontGetGadgetFont(globals->selection);
		// small/big fonts
		if(globals->selection < FontSelectHRFont5)
			*hires = true;
		else
			*hires = false;
		status = true;
		*newFontID = font;
	}

	FrmDeleteForm(frm);
	return status;
}