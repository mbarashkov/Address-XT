// logging to palm-doc file
// $Id: syslog.h,v 1.2 2005/12/04 09:40:08 sm Exp $

#pragma once

// it must be defined in compiler setting, but try it

void LogInit(char *name);
void LogWriteRaw(Char *log, Char *str);
void LogWrite(Char *log, Char *who, Char *message);
void LogWriteNum(Char *log, Char *who, UInt32 num,Char *message);
//void LogInfo(int pri, int facility);
