	#define BitAtPosition(pos)                ((UInt32)1 << (pos))
	#define SetBitMacro(bitfield, index)      ((bitfield) |= BitAtPosition(index))
	
	
	//#define CLIE_320x320	0
	//#define CLIE_320x480	1
	#define PALM_320x320	2
	#define PALM_320x480	3
	#define PALM_160x160	4
	#define ROW_STD_SMALL	7

	#define CREATORID		'adXT'
	#define PHONECREATORID	'HsPh'
	
	#define ADDRXTDB_DBNAME "AddrXTDB2"
	#define RECENTDB_DBNAME "RecentDB2"
	#define LINKSDB_DBNAME "LinksDB"
	
	#define REMIND_NONE		9999
	#define REMIND_NONE_OLD	"NONE"
	
	#define APP_DBNAME		"Address XT"
	
	#define CONTACTS_DBNAME "ContactsDB-PAdd"

	//#define MAPOPOLIS_APPNAME "Mapopolis"

	#define CONTACTSAPP_DBNAME "Contacts-PAdd"
	
	#define sort_ascending	-1
	#define sort_descending 1
	
	#define RECENT_MAX		20
	
	#define MAX_RECORD 512
	
	#define LINKTYPE_CONTACT	0
	#define LINKTYPE_MEMO		1
	#define LINKTYPE_TODO		2
	#define LINKTYPE_DATE		3
	
	#define PRINTDEBUG(x) {Char lStr[255];StrIToA(lStr, x);FrmCustomAlert(alertDebug, lStr, 0, 0);}
	
	extern UInt16 pnoJpegRefNum;
	
	//#define CONTACTS (DmFindDatabase(0, CONTACTS_DBNAME)!=0)
	
	#define DELIMITER_DEFAULT 0
	
	#define DELIMITER_0_STR ", "
	#define DELIMITER_0_LEN 2
	#define DELIMITER_1_STR " "
	#define DELIMITER_1_LEN 1
	
	#define DELIMITER_COUNT 2
	
	//deprecated
	#define RECENTDB_ID		0
	#define RECENTDB_TIME	1
	
	#define ADDRXTDB_ID		0
	#define ADDRXTDB_BDAY	1
	#define ADDRXTDB_REMIND 2
	#define ADDRXTDB_PASSED 3

	#define ADDRXTDB_DBNAME_OLD "AddrXTDB"
	#define RECENTDB_DBNAME_OLD "RecentDB"
	
	#define ADVANCEDFIND_OFF 1
	#define ADVANCEDFIND_ON 2
	
	#define TOMTOMNUMBER_BEGIN 0
	#define TOMTOMNUMBER_END 1
	#define TOMTOMNUMBER_SKIP 2
	
	
