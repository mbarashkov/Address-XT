// DIA support for Sony devices
#include <PalmOS.h>
#include <SonyClie.h>

#include "../dia.h"
#include "../globals.h"
#include "sony.h"

#include "../syslog.h"
#include "../Address.h"
#include "../AddrTools.h"

//--
static void dia_sony_save_state();
static void dia_sony_restore_state();
//static void dia_sony_enable_form(FormPtr frm, Boolean can_resize);
//void dia_sony_move_form(FormPtr frm);
static Boolean dia_sony_resize_form(FormPtr frm, dia_resize_func mover);
//static void dia_sony_close();
//--


// check & init
Boolean dia_sony_init(struct dia_support *dia_object)
{
	Err error;
	globalVars* globals = getGlobalsPtr();
    // load library
	if ((error = SysLibFind(sonySysLibNameSilk, &(globals->silk_lib))))
	{
		if (error == sysErrLibNotFound)
		{
			/* couldn't find lib */
			error = SysLibLoad( 'libr', sonySysFileCSilkLib, &(globals->silk_lib));
			if(error == sysErrLibNotFound) 
			{
#ifdef DEBUG
				LogWrite("xt_log", "dia/sony", "no lib");
#endif
				return false;
			}
				
		}
	}
	
	if(error)
	{
#ifdef DEBUG
		LogWrite("xt_log", "dia/sony", "no lib");
#endif
		return false;
	}

	
	// silk lib version
	error = FtrGet(sonySysFtrCreator, sonySysFtrNumVskVersion, &(globals->vskVersion));
	
	if(error)
	{
		/* Version 1 is installed, only resize is available */
		if(SilkLibOpen(globals->silk_lib) == errNone)
		{
			SilkLibEnableResize(globals->silk_lib);
#ifdef DEBUG
			LogWrite("xt_log", "dia/sony", "found v1");
#endif
		}
		else
		{
#ifdef DEBUG
			LogWrite("xt_log", "dia/sony", "can't load");
#endif
			return false;
		}

	}
	else if (globals->vskVersion == vskVersionNum2)
	{
		/* Version 2 is installed */
		if(VskOpen(globals->silk_lib) == errNone)
		{
			VskSetState(globals->silk_lib, vskStateEnable, vskResizeVertically);
#ifdef DEBUG
			LogWrite("xt_log", "dia/sony", "found v2");
#endif
		}
		else
		{
#ifdef DEBUG
			LogWrite("xt_log", "dia/sony", "can't load");
#endif
			return false;
		}

	}
	else
	{
		/* Version 3 or up is installed, Horizontal screen is available */
		if(VskOpen(globals->silk_lib) == errNone)
		{
			VskSetState(globals->silk_lib, vskStateEnable, vskResizeHorizontally);
#ifdef DEBUG
			LogWrite("xt_log", "dia/sony", "found v3");
#endif
		}
		else
		{
#ifdef DEBUG
			LogWrite("xt_log", "dia/sony", "can't load");
#endif
			return false;
		}

	}

	// global
	//silkRefNum = silk_lib;

	//register to notification
#ifdef DEBUG
	LogWrite("xt_log", "dia/sony", "SysNotifyRegister");
#endif
	//SysNotifyRegister(0, DmFindDatabase(0, APP_DBNAME), sysNotifyDisplayChangeEvent, 
	//						NULL, sysNotifyNormalPriority, &gSilkState);
	SysNotifyRegister(0, DmFindDatabase(0, APP_DBNAME), sysNotifyDisplayChangeEvent, 
							NULL, sysNotifyNormalPriority, NULL);
	
	dia_object->has_dia = true;
	dia_object->changed = false;
	dia_object->skip_first = false;		
	// setup methods
	dia_object->save_state = dia_sony_save_state;
	dia_object->restore_state = dia_sony_restore_state;
	dia_object->enable_form = NULL; //dia_sony_enable_form;
	//dia_object->move_form = dia_sony_move_form;
	dia_object->resize_form = dia_sony_resize_form;
	dia_object->close = NULL; //dia_sony_close;
	
	return true;
}

//--- private ---

static void dia_sony_save_state() 
{ 	
	globalVars* globals = getGlobalsPtr();
	if(globals->vskVersion >= vskVersionNum2)
	{
		VskGetState(globals->silk_lib, vskStateResize, &(globals->area_state));
		VskGetState(globals->silk_lib, vskStateEnable, &(globals->pin_state));
	}
	// else - isn't possible
}

static void dia_sony_restore_state() 
{ 
	// enable dia
	globalVars* globals = getGlobalsPtr();
	if(globals->vskVersion >= vskVersionNum2)
	{
		VskSetState(globals->silk_lib, vskStateResize, globals->area_state);
		VskSetState(globals->silk_lib, vskStateEnable, globals->pin_state);
	}
	//else isn't possible
}

/*
static void dia_sony_enable_form(FormPtr frm, Boolean can_resize) 
{
#ifdef DEBUG
	LogWrite("xt_log", "dia/sony", "enable_form");
#endif
   
	// if(can_resize) return;
	//dia_sony_move_form(frm);
}
*/

/*
void dia_sony_move_form(FormPtr frm) 
{
    //WinHandle fwin;
    //RectangleType frm_b, win_b;
    
#ifdef DEBUG
	LogWrite("xt_log", "dia/sony", "move");
#endif

	// Contacts style: don't move anything
	return;

//    fwin = FrmGetWindowHandle(frm);
//    WinGetBounds(fwin, &frm_b);
//    WinGetBounds(WinGetDisplayWindow(), &win_b);
    
//    frm_b.topLeft.y = win_b.extent.y - frm_b.extent.y - 2;
//    WinSetBounds(fwin, &frm_b);
    
    // dirty hack: if open - don't let close
    //if( PINGetInputAreaState() == pinInputAreaOpen )
	//PINSetInputTriggerState(pinInputTriggerDisabled);
	
	/ * TODO - find & read manual for vsk* functions
	if(vskVersion == vskVersionNum2)
	{
		VskSetState(silk_lib, vskResizeNone, vskResizeVertically);
	}
	else if(vskVersion == vskVersionNum3)
	{
		VskSetState(silk_lib, vskResizeNone, vskResizeHorizontally);
	}
	else
	{
		SilkLibDisableResize(silk_lib);
	}
	* /
}
*/

Boolean dia_sony_resize_form(FormPtr frm, dia_resize_func mover) 
{
    WinHandle fwin;
    RectangleType frm_b, win_b;
    Boolean status = false;
    
#ifdef DEBUG
	LogWrite("xt_log", "dia/sony", "resize");
#endif
  
    fwin = FrmGetWindowHandle(frm);
    WinGetBounds(fwin, &frm_b);
    WinGetBounds(WinGetDisplayWindow(), &win_b);

    WinSetBounds(fwin, &win_b);
	
//#ifdef DEBUG
//	LogWriteNum("xt_log", "dia", win_b.extent.x - frm_b.extent.x, "dx");
//	LogWriteNum("xt_log", "dia", win_b.extent.y - frm_b.extent.y, "dy");
//#endif

	// call custom func - to move form objects
	if(mover != NULL)
		status = (*mover)(frm, win_b.extent.x - frm_b.extent.x, 
						win_b.extent.y - frm_b.extent.y);
	
	return status;	
}

/*
void dia_sony_close() 
{
	// not used?
}
*/
