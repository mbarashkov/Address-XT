#ifndef PNOJPEG_H_
#define PNOJPEG_H_

/* Palm OS common definitions */
#include <SystemMgr.h>

/* If we're actually compiling the library code, then we need to
 * eliminate the trap glue that would otherwise be generated from
 * this header file in order to prevent compiler errors in CW Pro 2. */
#ifdef BUILDING_PNOJPEG
	#define PNOJPEG_LIB_TRAP(trapNum)
#else
	#define PNOJPEG_LIB_TRAP(trapNum) SYS_TRAP(trapNum)
#endif


#define pno_srcNoSource 	0
#define pno_srcMemPtr 		1
#define pno_srcMemHandle	2
#define pno_srcVFS			3
#define pno_srcFileStream	4	// Mobilisoft 20041230

#define pnoResize			1
#define pnoResample			2
//extern UInt16 pnoJpegRefNum = 0;

typedef struct {
 UInt16 	source;
 Boolean 	freePtr;
 UInt32 	size;
 MemPtr 	dataPtr;
 Boolean 	grayScale;
 UInt16		scaleFactor;
} pnoJpegType, *pnoJpegPtr;


typedef struct {
 UInt16		version;
 UInt16 	source;
 Boolean 	freePtr;
 UInt32 	size;
 MemPtr 	dataPtr;
 Boolean 	grayScale;
 UInt16		scaleFactor;
 Coord		maxWidth;		// Mobilisoft 20041230
 Coord		maxHeight;		// Mobilisoft 20041230
} pnoJpeg2Type, *pnoJpeg2Ptr;

#define pnoJpegLibVersion	200		// Version 2.00

/*********************************************************************
 * Type and creator of Sample Library database
 *********************************************************************/

#define		pnoJpegCreatorID	'ajLi'
#define		pnoJpegTypeID		sysFileTLibrary

/*********************************************************************
 * Internal library name which can be passed to SysLibFind()
 *********************************************************************/

#define		pnoJpegName		"pnoJpegLib"

/*********************************************************************
 * pnoJpegLib result codes
 * (appErrorClass is reserved for 3rd party apps/libraries.
 * It is defined in SystemMgr.h)
 *********************************************************************/

/* invalid parameter */
#define pnoJpegErrParam		(appErrorClass | 1)		

/* library is not open */
#define pnoJpegErrNotOpen		(appErrorClass | 2)		

/* returned from pnoJpegClose() if the library is still open */
#define pnoJpegErrStillOpen	(appErrorClass | 3)		

/*********************************************************************
 * API Prototypes
 *********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Standard library open, close, sleep and wake functions */

extern Err pnoJpegOpen(UInt16 refNum)PNOJPEG_LIB_TRAP(sysLibTrapOpen);
extern Err pnoJpegClose(UInt16 refNum)PNOJPEG_LIB_TRAP(sysLibTrapClose);
extern Err pnoJpegSleep(UInt16 refNum)PNOJPEG_LIB_TRAP(sysLibTrapSleep);
extern Err pnoJpegWake(UInt16 refNum)PNOJPEG_LIB_TRAP(sysLibTrapWake);

/* pnoJpeg Version 1 API */

extern Err pnoJpegCreate(UInt16 refNum, pnoJpegPtr data)PNOJPEG_LIB_TRAP(sysLibTrapBase + 5);
extern Err pnoJpegLoadFromPtr(UInt16 refNum, pnoJpegPtr data, MemPtr dataPtr, UInt32 dataSize)PNOJPEG_LIB_TRAP(sysLibTrapBase + 6);
extern Err pnoJpegLoadFromVFS(UInt16 refNum, pnoJpegPtr data, UInt16 volRefNum, char *filePath)PNOJPEG_LIB_TRAP(sysLibTrapBase + 7);
extern Err pnoJpegLoadFromHandle(UInt16 refNum, pnoJpegPtr data, MemHandle hImageData)PNOJPEG_LIB_TRAP(sysLibTrapBase + 8);
extern Err pnoJpegGetInfo(UInt16 refNum, pnoJpegPtr data, Coord *width, Coord *height)PNOJPEG_LIB_TRAP(sysLibTrapBase + 9);
extern Err pnoJpegSetScaleFactor(UInt16 refNum, pnoJpegPtr data, UInt16 factor)PNOJPEG_LIB_TRAP(sysLibTrapBase + 10);
extern Err pnoJpegSetGrayScale(UInt16 refNum, pnoJpegPtr data, Boolean grayScale)PNOJPEG_LIB_TRAP(sysLibTrapBase + 11);
extern Err pnoJpegRead(UInt16 refNum, pnoJpegPtr data, BitmapPtr *bmpPtr)PNOJPEG_LIB_TRAP(sysLibTrapBase + 12);
extern Err pnoJpegBmp2DoubleDensity(UInt16 refNum, BitmapPtr *bmp)PNOJPEG_LIB_TRAP(sysLibTrapBase + 13);

/* pnoJpeg Version 2 API */

extern Err pnoJpeg2Create(UInt16 refNum, pnoJpeg2Ptr *data)PNOJPEG_LIB_TRAP(sysLibTrapBase + 14);
extern Err pnoJpeg2LoadFromPtr(UInt16 refNum, pnoJpeg2Ptr data, MemPtr dataPtr, UInt32 dataSize)PNOJPEG_LIB_TRAP(sysLibTrapBase + 15);
extern Err pnoJpeg2LoadFromVFS(UInt16 refNum, pnoJpeg2Ptr data, UInt16 volRefNum, char *filePath)PNOJPEG_LIB_TRAP(sysLibTrapBase + 16);
extern Err pnoJpeg2LoadFromHandle(UInt16 refNum, pnoJpeg2Ptr data, MemHandle hImageData)PNOJPEG_LIB_TRAP(sysLibTrapBase + 17);
extern Err pnoJpeg2LoadFromFileStream(UInt16 refNum, pnoJpeg2Ptr data, FileHand fh)PNOJPEG_LIB_TRAP(sysLibTrapBase + 18);
extern Err pnoJpeg2GetInfo(UInt16 refNum, pnoJpeg2Ptr data, Coord *width, Coord *height)PNOJPEG_LIB_TRAP(sysLibTrapBase + 19);
extern Err pnoJpeg2SetScaleFactor(UInt16 refNum, pnoJpeg2Ptr data, UInt16 factor)PNOJPEG_LIB_TRAP(sysLibTrapBase + 20);
extern Err pnoJpeg2SetGrayscale(UInt16 refNum, pnoJpeg2Ptr data, Boolean grayscale)PNOJPEG_LIB_TRAP(sysLibTrapBase + 21);
extern Err pnoJpeg2SetMaxDimensions(UInt16 refNum, pnoJpeg2Ptr data, Coord maxWidth, Coord maxHeight)PNOJPEG_LIB_TRAP(sysLibTrapBase + 22);
extern Err pnoJpeg2Read(UInt16 refNum, pnoJpeg2Ptr data, BitmapPtr *bmpPtr)PNOJPEG_LIB_TRAP(sysLibTrapBase + 23);
extern Err pnoJpeg2Free(UInt16 refNum, pnoJpeg2Ptr *data)PNOJPEG_LIB_TRAP(sysLibTrapBase + 24);
extern UInt16 pnoJpeg2Version(UInt16 refNum)PNOJPEG_LIB_TRAP(sysLibTrapBase + 25);

extern BitmapPtr pnoJpeg2Resize(UInt16 refNum, BitmapPtr inBmp, Coord newWidth, Coord newHeight, Err *error)PNOJPEG_LIB_TRAP(sysLibTrapBase + 26);
extern BitmapPtr pnoJpeg2Resample(UInt16 refNum, BitmapPtr inBmp, Coord newWidth, Coord newHeight, Err *error)PNOJPEG_LIB_TRAP(sysLibTrapBase + 27);
extern Err pnoJpeg2Bmp2DoubleDensity(UInt16 refNum, BitmapPtr inBmp, BitmapPtrV3 *outBmp)PNOJPEG_LIB_TRAP(sysLibTrapBase + 28);
extern BitmapPtr pnoJpeg2CreateThumbnail(UInt16 refNum, BitmapPtr bmp, Coord maxWidth, Coord maxHeight, int method)PNOJPEG_LIB_TRAP(sysLibTrapBase + 29);
extern Err GetJpegInfo(MemPtr dataPtr, UInt32 dataSize, Coord *x , Coord *y);

Err LoadJpegFromPtr(MemPtr dataPtr, UInt32 dataSize, Boolean resample, Coord width, Coord height, int x, int y);
extern Err switchToBestGraphicMode(void);
extern Err jpgInit();
extern void jpgClose(void);


#ifdef __cplusplus
}
#endif

/*
 * FUNCTION: pnoJpeg_OpenLibrary
 *
 * DESCRIPTION:
 *
 * User-level call to open the library.  This inline function
 * handles the messy task of finding or loading the library
 * and calling its open function, including handling cleanup
 * if the library could not be opened.
 * 
 * PARAMETERS:
 *
 * refNumP
 *		Pointer to UInt16 variable that will hold the new
 *      library reference number for use in later calls
 *
 * clientContextP
 *		pointer to variable for returning client context.  The client context is
 *		used to maintain client-specific data for multiple client support.  The 
 *		value returned here will be used as a parameter for other library 
 *		functions which require a client context.  
 *
 * CALLED BY: System
 *
 * RETURNS:
 *		errNone
 *		memErrNotEnoughSpace
 *      sysErrLibNotFound
 *      sysErrNoFreeRAM
 *      sysErrNoFreeLibSlots
 *
 * SIDE EFFECTS:
 *		*clientContextP will be set to client context on success, or zero on
 *      error.
 */
 
__inline Err pnoJpeg_OpenLibrary(UInt16 *refNumP)
{
	Err error;
	Boolean loaded = false;
	
	/* first try to find the library */
	error = SysLibFind(pnoJpegName, refNumP);
	
	if (error == sysErrLibNotFound)
	{
		error = SysLibLoad(pnoJpegTypeID, pnoJpegCreatorID, refNumP);
		loaded = true;
	}
	
	if (error == errNone)
	{
		error = pnoJpegOpen(*refNumP);
		if (error != errNone)
		{
			if (loaded)
			{
				SysLibRemove(*refNumP);
			}

			*refNumP = sysInvalidRefNum;
		}
	}
	
	return error;
}

/*
 * FUNCTION: pnoJpeg_CloseLibrary
 *
 * DESCRIPTION:	
 *
 * User-level call to closes the shared library.  This handles removal
 * of the library from system if there are no users remaining.
 *
 * PARAMETERS:
 *
 * refNum
 *		Library reference number obtained from pnoJpeg_OpenLibrary().
 *
 * clientContext
 *		client context (as returned by the open call)
 *
 * CALLED BY: Whoever wants to close the library
 *
 * RETURNS:
 *		errNone
 *		sysErrParamErr
 */

__inline Err pnoJpeg_CloseLibrary(UInt16 refNum)
{
	Err error;
	
	if (refNum == sysInvalidRefNum)
	{
		return sysErrParamErr;
	}

	error = pnoJpegClose(refNum);

	if (error == errNone)
	{
		/* no users left, so unload library */
		SysLibRemove(refNum);
	} 
	else if (error == pnoJpegErrStillOpen)
	{
		/* don't unload library, but mask "still open" from caller  */
		error = errNone;
	}
	
	return error;
}

#endif /* PNOJPEG_H_ */
