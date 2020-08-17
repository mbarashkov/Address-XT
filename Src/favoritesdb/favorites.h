#pragma once

// manage favorites db in Treo 600 & 650

DmOpenRef favorites_open(Boolean readonly);

Boolean favorites_write(DmOpenRef db, UInt8 position, Char *label, UInt32 creator, Char hot_key);

Boolean self_check(DmOpenRef db, UInt16 index, UInt32 creator);

Int16 find_record(DmOpenRef db, UInt8 position, Boolean *create_new);