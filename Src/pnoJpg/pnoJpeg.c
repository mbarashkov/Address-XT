/*
 * pnoJpeg2Test.c
 *
 * main file for pnoJpeg2Test
 *
 * This wizard-generated code is based on code adapted from the
 * stationery files distributed as part of the Palm OS SDK 4.0.
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */
 
#include <PalmOS.h>

#include "pnoJpeg.h"

/*********************************************************************
 * Entry Points
 *********************************************************************/

/*********************************************************************
 * Global variables
 *********************************************************************/
UInt16 pnoJpegRefNum;

Err GetJpegInfo(MemPtr dataPtr, UInt32 dataSize, Coord *width , Coord *height)
{
 Err 			err 		= errNone;
 pnoJpeg2Ptr	jpegData 	= NULL;
 BitmapPtr		bmp			= NULL;
 
 *width = 0;
 *height = 0;
 
 err = pnoJpeg2Create(pnoJpegRefNum, &jpegData);
 if(err != errNone)
 {
  ErrAlert(err);
  return(err);
 }
 

 err = pnoJpeg2LoadFromPtr(pnoJpegRefNum, jpegData, dataPtr, dataSize);
 if(err != errNone)
 {
  ErrAlert(err);
  return(err);
 }
 err = pnoJpeg2Read(pnoJpegRefNum, jpegData, &bmp);
 if(err != errNone)
 {
  ErrAlert(err);
  return(err);
 }

 err = pnoJpeg2Free(pnoJpegRefNum, &jpegData);
 if(err != errNone)
 {
  ErrAlert(err);
  return(err);
 }
 

 if(bmp)
 {
  BmpGetDimensions(bmp, width, height, NULL);
  BmpDelete(bmp);
 }
 return errNone;
}


Err LoadJpegFromPtr(MemPtr dataPtr, UInt32 dataSize, Boolean resample, Coord width, Coord height, int x , int y)
{
 Err 			err 		= errNone;
 pnoJpeg2Ptr	jpegData 	= NULL;
 BitmapPtr		bmp			= NULL;
 BitmapPtrV3	bmpV3		= NULL;
 
 
 err = pnoJpeg2Create(pnoJpegRefNum, &jpegData);
 if(err != errNone)
 {
  ErrAlert(err);
  return(err);
 }
 

 err = pnoJpeg2LoadFromPtr(pnoJpegRefNum, jpegData, dataPtr, dataSize);
 if(err != errNone)
 {
  ErrAlert(err);
  return(err);
 }

 err = pnoJpeg2Read(pnoJpegRefNum, jpegData, &bmp);
 if(err != errNone)
 {
  ErrAlert(err);
  return(err);
 }

 err = pnoJpeg2Free(pnoJpegRefNum, &jpegData);
 if(err != errNone)
 {
  ErrAlert(err);
  return(err);
 }
 
 if(bmp)
 {
  if(resample)
  {
   bmp = pnoJpeg2Resample(pnoJpegRefNum, bmp, width, height, &err);
   if(err != errNone)
   {
	 ErrAlert(err);
	 return(err);   
   }

  }
 
  err = pnoJpeg2Bmp2DoubleDensity(pnoJpegRefNum, bmp, &bmpV3);
	if(err != errNone)
	{
	 ErrAlert(err);
	 return(err);
	}
   
  if(bmpV3)
  {
   WinDrawBitmap((BitmapPtr)bmpV3, x, y);
   BmpDelete((BitmapPtr)bmpV3);
  } 
  
  BmpDelete(bmp);
 }
 

 return(errNone);
}

Err switchToBestGraphicMode(void)
{
 UInt32 	depth 	= 0;
 Boolean 	color 	= false;
 Err 		err 	= errNone;
 
 
 err = WinScreenMode(winScreenModeGetSupportsColor, NULL, NULL, NULL, &color);

 err = WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &depth, &color);
 if(depth & 0x8000)
 {
  depth = 16;
  err = WinScreenMode(winScreenModeSet, NULL, NULL, &depth, &color);
 } else
 if(depth & 0x80)
 {
  depth = 8;
  err = WinScreenMode(winScreenModeSet, NULL, NULL, &depth, &color); 
 } else
 if(depth & 0x8)
 {
  depth = 4;
  err = WinScreenMode(winScreenModeSet, NULL, NULL, &depth, &color); 
 } else
 if(depth & 0x2)
 {
  depth = 2;
  err = WinScreenMode(winScreenModeSet, NULL, NULL, &depth, &color); 
 } else
 {
  err = WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);
 }

 return(err);
}

Err jpgInit()
{
 Err err = errNone;
  err = pnoJpeg_OpenLibrary(&pnoJpegRefNum);
 if(err != errNone)
 {
  //ErrAlert(err);
  return(err);
 }

 switchToBestGraphicMode();

 return errNone;
}

void jpgClose(void)
{
 Err err = errNone;    
 if(pnoJpegRefNum != sysInvalidRefNum )	
 	err = pnoJpegClose(pnoJpegRefNum);
}