/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: DateDay.h
 *
 * Description:
 *	  This file defines the Datebook's Main modual's functions anf globals.
 *
 * History:
 *		August 10, 1995	Created by Art Lamb
 *		09/09/97	frigino	Moved DatebookPreferenceType into here
 *		04/22/99	rbb	Added support for snooze feature
 *		08/03/99	kwk	Deleted defaultApptDescFont & defaultNoteFont.
 *
 *****************************************************************************/

#include <Event.h>

Boolean RepeatHandleEvent (EventType* event);
Boolean DetailsHandleEvent (EventType* event);
Boolean NoteViewHandleEvent (EventType* event);
Boolean DayViewHandleEvent (EventType* event);
Err DateGetDatabase (DmOpenRef *dbPP, UInt16 mode);

