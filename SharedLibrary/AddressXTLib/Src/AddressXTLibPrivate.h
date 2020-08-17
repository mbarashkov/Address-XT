/*
 * AddressXTLibPrivate.h
 *
 * header for implementation for shared library
 *
 * This wizard-generated code is based on code adapted from the
 * SampleLib project distributed as part of the Palm OS SDK 4.0.
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */
 
#ifndef ADDRESSXTLIBPRIVATE_H_
#define ADDRESSXTLIBPRIVATE_H_

/*********************************************************************
 * Private Structures
 *********************************************************************/

/* Library globals */

typedef struct AddressXTLibGlobalsType
{
	/* our library reference number (for convenience and debugging) */
	UInt16 thisLibRefNum;
	
	/* library open count */
	Int16 openCount;
	
	/* number of context in existence (for debugging) */
	Int16 contextCount;
	
	/* TODO: add other library globals here */

} AddressXTLibGlobalsType;

/* Client context structure for storing each client-specific data */

typedef struct AddressXTLibClientContextType
{
	/* signature for validating the context based on
	 * address of AddressXTLibOpen routine */
	void * libSignature;

	/* TODO: add other per-client variables here */

} AddressXTLibClientContextType;

/*********************************************************************
 * Private Macros
 *********************************************************************/

#endif /* ADDRESSXTLIBPRIVATE_H_ */
