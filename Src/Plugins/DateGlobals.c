/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: DateGlobals.c
 *
 * Description:
 *	  Because of a bug in Metrowerks, we must compile the globals separately with
 *		PC-relative strings turned off. Otherwise, we get linker errors with
 *		pre-initialized structures.
 *
 * History:
 *		June 12, 1995	Created by Art Lamb
 *		09/09/97	frigino Added initial values to AlarmSoundXXXX vars
 *		03/05/99	grant	DetailsH replaced by DetailsP
 *		04/23/99	rbb	Added AlarmSnooze
 *		08/03/99	kwk	Removed initialization of font globals - they all
 *							have to be set up in StartApplication.
 *		11/04/99	jmp	Added InPhoneLookup global.
 *
 *****************************************************************************/

#include <PalmOS.h>

#define	NON_PORTABLE
//#include "MemoryPrv.h"

#include "Datebook.h"

