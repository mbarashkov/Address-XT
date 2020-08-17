// DIA support for G18-Xplore devices
#include <PalmOS.h>
// Zircon  SDK
#include <CSilkScreen.h>

#include "../dia.h"
#include "../globals.h"

#include "xplore.h"

#include "../syslog.h"
#include "../Address.h"
#include "../AddrTools.h"


//--
//static void dia_xplore_save_state();
//static void dia_xplore_restore_state();
static void dia_xplore_enable_form(FormPtr frm, Boolean can_resize);
//static void dia_xplore_move_form(FormPtr frm);
static Boolean dia_xplore_resize_form(FormPtr frm, dia_resize_func mover);
//static void dia_xplore_close();

//=== globals ===
// silk lib

// check & init
Boolean dia_xplore_init(struct dia_support *dia_object)
{
	globalVars* globals = getGlobalsPtr();
	Err error = errNone;
	
	// try to load library
	error = SysLibFind(CSilkScreenName, &(globals->silk_lib));
	if(error != errNone)
	{
		error = SysLibLoad(CSilkScreenTypeID, CSilkScreenCreatorID, &(globals->silk_lib));
		if(error)
		{
#ifdef DEBUG
			LogWrite("xt_log", "dia/xplore", "not found");
#endif
			return false;
		}
	}
	
	CSilkScreenEnable(globals->silk_lib, true);
	
#ifdef DEBUG
	LogWrite("xt_log", "dia/xplore", "found");
#endif
	
	// setub object
	dia_object->has_dia = true;
	dia_object->changed = false;
	dia_object->skip_first = false;		
	// setup methods
	dia_object->save_state = NULL; //dia_xplore_save_state;
	dia_object->restore_state = NULL; //dia_xplore_restore_state;
	dia_object->enable_form = NULL; //dia_xplore_enable_form;
	//dia_object->move_form = dia_xplore_move_form;
	dia_object->resize_form = dia_xplore_resize_form;
	dia_object->close = NULL; // dia_xplore_close;
	
	return true;
}

/*
static void dia_xplore_enable_form(FormPtr frm, Boolean can_resize) 
{
#ifdef DEBUG
	LogWrite("xt_log", "dia/xplore", "enable_form");
#endif
}
*/

//static void dia_xplore_save_state() {}
//static void dia_xplore_restore_state() {}
//static void dia_xplore_move_form(FormPtr frm) {}

static Boolean dia_xplore_resize_form(FormPtr frm, dia_resize_func mover)
{
    WinHandle fwin;
    RectangleType frm_b, win_b;
    Boolean status = false;
    
#ifdef DEBUG
	LogWrite("xt_log", "dia/xplore", "resize");
#endif
  
    fwin = FrmGetWindowHandle(frm);
    WinGetBounds(fwin, &frm_b);
    WinGetBounds(WinGetDisplayWindow(), &win_b);

    WinSetBounds(fwin, &win_b);
	

	// call custom func - to move form objects
	if(mover != NULL)
	{
		if(win_b.extent.x - frm_b.extent.x != 0 || win_b.extent.y - frm_b.extent.y != 0)
			WinEraseWindow();
			
		status = (*mover)(frm, win_b.extent.x - frm_b.extent.x, 
						win_b.extent.y - frm_b.extent.y);
	}
	return status;	
}

//static void dia_xplore_close() {}