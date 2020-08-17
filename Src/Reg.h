#pragma once

#ifndef LIBDEBUG
#include "..\SharedLibrary\AddressXTLib2\Src\AddressXTLib.h"
#endif

#include <PalmOS.h>
#include <System/DLServer.h>
#include "Address.h"
#include "globals.h"
#include "AddrPrefs.h"

#ifdef LIBDEBUG //moved to shared library
UInt32 CheckRegistration(UInt16 adxtRefNum);
#else
extern UInt32	CheckRegistration(UInt16 adxtRefNum)
					ADDRESSXTLIB2_LIB_TRAP(adxtLib2TrapCheckRegistration);
#endif