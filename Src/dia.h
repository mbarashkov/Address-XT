#pragma once

typedef Boolean (*dia_resize_func)(FormPtr frmP, Coord dxP, Coord dyP);

struct dia_support {
	Boolean has_dia;
	// status
	Boolean changed;
	// skip 1st event (yes - for PalmOne, no - for Sony)
	Boolean skip_first;
  //=== methods ====
	// save area & trigger state
	void (*save_state)();
	// restore area & trigger state
	void (*restore_state)();
	// enable for form
	void (*enable_form)(FormPtr frm, Boolean can_resize);
	// move dialog form
	// void (*move_form)(FormPtr frm);
	// resize form
	Boolean (*resize_form)(FormPtr frm, dia_resize_func mover);
	// close area
	void (*close)();
	// open area
	void (*open)();
  //=== 
};

// init DIA for devices
void dia_init();

// close DI area
void dia_close();
void dia_open();

void dia_save_state();
void dia_restore_state();
//void dia_form(FormPtr frm);
//void dia_move(FormPtr frm);
void dia_enable(FormPtr frm, Boolean can_resize);
void dia_win_enter();
Boolean dia_resize(FormPtr frm, dia_resize_func mover);
void dia_display_changed();
Boolean dia_skip_first();

// main event-handler
Boolean dia_handle_event(EventType *event, dia_resize_func mover);
