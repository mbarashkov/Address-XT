// DIA funcs for PalmOne devices
#include <PalmOS.h>

#include "../globals.h"
#include "../dia.h"
#include "palm.h"
#include "../syslog.h"
#include "../Address.h"
#include "../AddrTools.h"
//-- private
static void dia_palm_save_state();
static void dia_palm_restore_state();
static void dia_palm_enable_form(FormPtr frm, Boolean can_resize);
//static void dia_palm_move_form(FormPtr frm);
static Boolean dia_palm_resize_form(FormPtr frm, dia_resize_func mover);
static void dia_palm_close();
static void dia_palm_open();


// check & init
Boolean dia_palm_init(struct dia_support *dia_object)
{
	UInt32 version;
	Err err;
		
	err = FtrGet(pinCreator, pinFtrAPIVersion, &version);
	if(!err && version)
	{
		//PINS exists
		if(version == pinAPIVersion1_0)
		{
			// need to register only for 1.0
			SysNotifyRegister(0, DmFindDatabase(0, APP_DBNAME), sysNotifyDisplayResizedEvent, 
								NULL, sysNotifyNormalPriority, NULL);		
#ifdef DEBUG
			LogWrite("xt_log", "dia/palm", "found v1.0");
#endif		
		}
		else
		{
			// we will receive winDisplayChangedEvent already
#ifdef DEBUG
			LogWrite("xt_log", "dia/palm", "found v1.1");
#endif		
		}
		
		dia_object->has_dia = true;
		dia_object->changed = false;
		// now - false?
		dia_object->skip_first = false;
		// FIXME - it's false for Garmin
		//dia_object.skip_first = false;
		// setup methods
		dia_object->save_state = dia_palm_save_state;
		dia_object->restore_state = dia_palm_restore_state;
		dia_object->enable_form = dia_palm_enable_form;
		//dia_object->move_form = dia_palm_move_form;
		dia_object->resize_form = dia_palm_resize_form;
		dia_object->close = dia_palm_close;
		dia_object->open = dia_palm_open;
		
		return true;		
	}
	
#ifdef DEBUG
	LogWrite("xt_log", "dia/palm", "not found");
#endif
	
	return false;
}

//--- private ---

static void dia_palm_save_state()
{
	globalVars* globals = getGlobalsPtr();
	globals->pin_state = PINGetInputAreaState();
    globals->trigger_state = PINGetInputTriggerState();
}

static void dia_palm_restore_state()
{
	globalVars* globals = getGlobalsPtr();
	PINSetInputAreaState(globals->pin_state);
    PINSetInputTriggerState(globals->trigger_state);
}

// close area
static void dia_palm_close()
{
	PINSetInputAreaState(pinInputAreaClosed);
	PINSetInputTriggerState(pinInputTriggerEnabled);
	dia_palm_save_state();
}

// open area
static void dia_palm_open()
{
	globalVars* globals = getGlobalsPtr();
	PINSetInputAreaState(pinInputAreaOpen);
	PINSetInputTriggerState(pinInputTriggerEnabled);
	// ? dia_palm_save_state();
}


static void dia_palm_enable_form(FormPtr frm, Boolean can_resize)
{
    WinHandle fwin;
    
    fwin = FrmGetWindowHandle(frm);
    
    WinSetConstraintsSize(fwin, 80, 160, 225, 160, 160, 225);
    
    FrmSetDIAPolicyAttr( frm, frmDIAPolicyCustom );
	PINSetInputTriggerState( pinInputTriggerEnabled );
	// optional
	PINSetInputAreaState(pinInputAreaUser);

    if(can_resize) return;
}

/*
void dia_palm_move_form(FormPtr frm)
{
    WinHandle fwin;
    RectangleType frm_b, win_b;
    
    // Contacts style - dont move anything
    //return;
    
#ifdef DEBUG
	LogWrite("xt_log", "dia/palm", "move");
#endif

    fwin = FrmGetWindowHandle(frm);
    WinGetBounds(fwin, &frm_b);
    WinGetBounds(WinGetDisplayWindow(), &win_b);
 
#ifdef DEBUG
	LogWriteNum("xt_log", "dia", frm_b.topLeft.x, "f.left-x");
	LogWriteNum("xt_log", "dia", frm_b.topLeft.y, "f.left-y");
	LogWriteNum("xt_log", "dia", win_b.topLeft.x, "d.left-x");
	LogWriteNum("xt_log", "dia", win_b.topLeft.y, "d.left-y");	
#endif
    
//    frm_b.topLeft.y = win_b.extent.y - frm_b.extent.y - 2;
//    WinSetBounds(fwin, &frm_b);
    
    // dirty hack: if open - don't let close
//    if( PINGetInputAreaState() == pinInputAreaOpen )
//	PINSetInputTriggerState(pinInputTriggerDisabled);
}
*/

static Boolean dia_palm_resize_form(FormPtr frm, dia_resize_func mover)
{
	globalVars* globals = getGlobalsPtr();
	WinHandle fwin;
    RectangleType frm_b, win_b;
    Boolean status = false;
 	UInt16 focus = noFocus;   
  
    fwin = FrmGetWindowHandle(frm);

    WinGetBounds(fwin, &frm_b);
    WinGetBounds(WinGetDisplayWindow(), &win_b);
	
	// call custom func - to move form objects
	if(globals->gNavigation)
       	{
       		if(FrmGlueNavGetFocusRingInfo (globals->adxtLibRef, frm, &focus, NULL, NULL, NULL) != errNone)
       			focus = noFocus;
       }
      	
   	if(mover != NULL)
		status = (*mover)(frm, win_b.extent.x - frm_b.extent.x, 
						win_b.extent.y - frm_b.extent.y);
	
	    WinSetBounds(fwin, &win_b);

	if(globals->gNavigation && focus != noFocus)
       	{
       	   		FrmGlueNavObjectTakeFocus(globals->adxtLibRef, frm, focus);
       	}
       	
   	return status;
}
