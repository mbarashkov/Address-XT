#pragma once
#include "AddrView.h"

Boolean ViewOnMenuCmdBarOpenEvent(EventType* event);
Boolean ViewOnMenuOpenEvent();
Boolean PrvViewDoCommand (UInt16 command);
void 	LinksDrawList(Int16 itemNum, RectangleType *bounds, Char **itemsText);
void 	DisplayLinks();