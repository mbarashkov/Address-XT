// DIA common func
#include <PalmOS.h>

// winResizeEvent
//#include <CSilkScreen.h>

#include "dia.h"
#include "syslog.h"

#include "dia/palm.h"
#include "../Address.h"
#include "../AddrTools.h"

// main object

// check & init DIA support
void dia_init()
{
	globalVars* globals = getGlobalsPtr();
	globals->dia_object.has_dia = false;
	
	if(dia_palm_init(&globals->dia_object))
		return;
}

// presave DIA state - usefull before system dialogs
void dia_save_state() 
{
 	globalVars* globals = getGlobalsPtr();
    if(! globals->dia_object.has_dia || !globals->dia_object.save_state) return;
    globals->dia_object.save_state();
}

// restore DIA state from last dia_save_state
void dia_restore_state() 
{
	globalVars* globals = getGlobalsPtr();
	if(! globals->dia_object.has_dia || !globals->dia_object.restore_state ) return;
#ifdef DEBUG
	LogWrite("xt_log", "dia", "restore_state");
#endif
    globals->dia_object.restore_state();
}

// close DIA area
void dia_close()
{
	globalVars* globals = getGlobalsPtr();
	if(! globals->dia_object.has_dia || !globals->dia_object.close) return;
	globals->dia_object.close();
}

// open DIA area
void dia_open()
{
	globalVars* globals = getGlobalsPtr();
	if(! globals->dia_object.has_dia || !globals->dia_object.open) return;
	globals->dia_object.open();
}


// resize form (object moved by own function)
Boolean dia_resize(FormPtr frm, dia_resize_func mover)
{
	globalVars* globals = getGlobalsPtr();
	if(! globals->dia_object.has_dia || !globals->dia_object.resize_form) return false;
	globals->dia_object.changed = false;
	return globals->dia_object.resize_form(frm, mover);
	//return false;
}

// enable DIA for form (call on frmOpenEvent or frmLoadEvent)
void dia_enable(FormPtr frm, Boolean can_resize) 
{
	globalVars* globals = getGlobalsPtr();
    if(! globals->dia_object.has_dia || !globals->dia_object.enable_form) return;
    globals->dia_object.enable_form(frm, can_resize);
}


// common winEnterEvent handler -- repost it as winDisplayChangedEvent
void dia_win_enter() 
{
	globalVars* globals = getGlobalsPtr();
	EventType resizedEvent; 
	
    if(! globals->dia_object.has_dia) return;
    
    if(globals->dia_object.changed)
    {
/*#ifdef DEBUG
		LogWrite("xt_log", "dia", "winEnterEvent");
#endif*/
		
		MemSet(&resizedEvent, sizeof(EventType), 0); 
		resizedEvent.eType = winDisplayChangedEvent;
		EvtAddUniqueEventToQueue(&resizedEvent, 0, true);
/*#ifdef DEBUG
		LogWrite("xt_log", "dia", "queue winDisplayChangedEvent");
#endif*/
		globals->dia_object.changed = false;
	}
}

// common winDisplayChangedEvent handler
// save status for dialogs -- for resize later
void dia_display_changed()
{
	globalVars* globals = getGlobalsPtr();
//#ifdef DEBUG
//	LogWrite("xt_log", "dia", "winDisplayChangedEvent");
//#endif
	if(! globals->dia_object.has_dia) return;
	globals->dia_object.changed = true;
}

// does we need to skip 1st winDisplayChangedEvent (see struct comments)
Boolean dia_skip_first()
{
	globalVars* globals = getGlobalsPtr();
	if(! globals->dia_object.has_dia) return false;
	return globals->dia_object.skip_first;
}

// process DIA-related events
Boolean dia_handle_event(EventType *event, dia_resize_func mover)
{
	globalVars* globals = getGlobalsPtr();
	FormPtr frm;
	Boolean handled = false;
	
	if(! globals->dia_object.has_dia) return handled;

	switch(event->eType)
	{
		case winDisplayChangedEvent:
#ifdef DEBUG
			LogWrite("xt_log", "dia", "winDisplayChangedEvent");
#endif
			if(mover != NULL)
			{
				frm = FrmGetActiveForm();
				if( dia_resize(frm, mover) )
				{
          			/*dia_save_state();
#ifdef DEBUG
					LogWrite("xt_log", "dia", "post frmUpdateEvent");
#endif
			*/
					FrmUpdateForm(FrmGetActiveFormID(), frmRedrawUpdateCode);
          		
          		}
			}
			else
			{
				dia_display_changed();
#ifdef DEBUG
				LogWrite("xt_log", "dia", "post frmUpdateEvent");
#endif
			
				FrmUpdateForm(FrmGetActiveFormID(), frmRedrawUpdateCode);
			}
			handled = true;
			break;
			
		default:
			break;
	}
	
	return handled;
}
