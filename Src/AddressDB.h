#pragma once

#ifndef LIBDEBUG
#include "..\SharedLibrary\AddressXTLib\Src\AddressXTLib.h"
#endif

#include "AddrDefines.h"

#include <AppLaunchCmd.h>
#include <CoreTraps.h>
#include <LocaleMgr.h>
#include <DataMgr.h>


/***********************************************************************
 *   Defines
 ***********************************************************************/

// Max length of a field name found in the FieldNamesStrList string list.
#define maxFieldName		31

#define LocalizedAppInfoStr	1000

// Extract the bit at position index from bitfield.  0 is the high bit.
#define BitAtPosition(pos)                ((UInt32)1 << (pos))
#define GetBitMacro(bitfield, index)      ((bitfield) & BitAtPosition(index))
#define RemoveBitMacro(bitfield, index)   ((bitfield) &= ~BitAtPosition(index))

#define sortKeyFieldBits   (BitAtPosition(name) | BitAtPosition(firstName) | BitAtPosition(company))

// Indexes into FieldNamesStrList string list.
enum {
	fieldNameStrListCity = 0,
	fieldNameStrListState,
	fieldNameStrListZip
};


#define firstAddressField			name
#define firstPhoneField				phone1
#define lastPhoneField				phone5
#define titleField					title
#define numPhoneFields				(lastPhoneField - firstPhoneField + 1)
#define numPhoneLabelsStoredFirst	numPhoneFields
#define numPhoneLabelsStoredSecond	(numPhoneLabels - numPhoneLabelsStoredFirst)

#define firstRenameableLabel		custom1
#define lastRenameableLabel			custom4
#define lastLabel					(addressFieldsCount + numPhoneLabelsStoredSecond)

#define IsPhoneLookupField(p)		(addrLookupWork <= (p) && (p) <= addrLookupMobile)

#define addrLabelLength				16

/***********************************************************************
 *   Internal Structures
 ***********************************************************************/

typedef union
{
	struct
	{
		unsigned reserved	:13;
		unsigned note		:1;	// set if record contains a note handle
		unsigned custom4	:1;	// set if record contains a custom4
		unsigned custom3	:1;	// set if record contains a custom3
		unsigned custom2	:1;	// set if record contains a custom2
		unsigned custom1	:1;	// set if record contains a custom1
		unsigned title		:1;	// set if record contains a title
		unsigned country	:1;	// set if record contains a birthday
		unsigned zipCode	:1;	// set if record contains a birthday
		unsigned state		:1;	// set if record contains a birthday
		unsigned city		:1;	// set if record contains a birthday
		unsigned address	:1;	// set if record contains a address
		unsigned phone5		:1;	// set if record contains a phone5
		unsigned phone4		:1;	// set if record contains a phone4
		unsigned phone3		:1;	// set if record contains a phone3
		unsigned phone2		:1;	// set if record contains a phone2
		unsigned phone1		:1;	// set if record contains a phone1
		unsigned company	:1;	// set if record contains a company
		unsigned firstName	:1;	// set if record contains a firstName
		unsigned name		:1;	// set if record contains a name (bit 0)

	} bits;
	UInt32 allBits;
} AddrDBRecordFlags;

typedef union
{
	struct
	{
		unsigned reserved	 :10;
		unsigned phone8      :1;	// set if phone8 label is dirty
		unsigned phone7      :1;	// set if phone7 label is dirty
		unsigned phone6      :1;	// set if phone6 label is dirty
		unsigned note        :1;	// set if note label is dirty
		unsigned custom4     :1;	// set if custom4 label is dirty
		unsigned custom3     :1;	// set if custom3 label is dirty
		unsigned custom2     :1;	// set if custom2 label is dirty
		unsigned custom1     :1;	// set if custom1 label is dirty
		unsigned title       :1;	// set if title label is dirty
		unsigned country	 :1;	// set if country label is dirty
		unsigned zipCode	 :1;	// set if zipCode label is dirty
		unsigned state		 :1;	// set if state label is dirty
		unsigned city		 :1;	// set if city label is dirty
		unsigned address     :1;	// set if address label is dirty
		unsigned phone5      :1;	// set if phone5 label is dirty
		unsigned phone4      :1;	// set if phone4 label is dirty
		unsigned phone3      :1;	// set if phone3 label is dirty
		unsigned phone2      :1;	// set if phone2 label is dirty
		unsigned phone1      :1;	// set if phone1 label is dirty
		unsigned company     :1;	// set if company label is dirty
		unsigned firstName   :1;	// set if firstName label is dirty
		unsigned name        :1;	// set if name label is dirty (bit 0)

	} bits;
	UInt32 allBits;
} AddrDBFieldLabelsDirtyFlags;


typedef struct
{
	//UInt8 reserved;//WAS unsigned reserved:7
	UInt16 sortByCompany;//WAS unsigned sortByCompany:1
} AddrDBMisc;

#define MAX_CONTACTS_ROWID 37
typedef enum
{
	name,
	firstName,
	company,
	phone1,
	phone2,
	phone3,
	phone4,
	phone5,
	address,
	city,
	state,
	zipCode,
	country,
	title,
	custom1,
	custom2,
	custom3,
	custom4,
	note,			// This field is assumed to be < 4K
	addressFieldsCount,
	birthday
} AddressFields;

// This structure is only for the exchange of address records.
typedef union
{
	struct
	{
		unsigned reserved		:8;

		// Typically only one of these are set
		unsigned email			:1;	// set if data is an email address
		unsigned fax			:1;	// set if data is a fax
		unsigned pager			:1;	// set if data is a pager
		unsigned voice			:1;	// set if data is a phone

		unsigned mobile			:1;	// set if data is a mobile phone

		// These are set in addition to other flags.
		unsigned work			:1;	// set if phone is at work
		unsigned home			:1;	// set if phone is at home

		// Set if this number is preferred over others.  May be preferred
		// over all others.  May be preferred over other emails.  One
		// preferred number should be listed next to the person's name.
		unsigned preferred   	:1;	// set if this phone is preferred (bit 0)
	} bits;
	UInt32 allBits;
} AddrDBPhoneFlags;

typedef enum
{
	workLabel,
	homeLabel,
	faxLabel,
	otherLabel,
	emailLabel,
	mainLabel,
	pagerLabel,
	mobileLabel
} AddressPhoneLabels;


typedef union
{
	struct
	{
		unsigned reserved:8;
		unsigned displayPhoneForList:4;	// The phone displayed for the list view 0 - 4
		unsigned phone5:4;				// Which phone (home, work, car, ...)
		unsigned phone4:4;
		unsigned phone3:4;
		unsigned phone2:4;
		unsigned phone1:4;
	} phones;
	UInt32 phoneBits;
} AddrOptionsType;

// AddrDBRecord.
//
// This is the unpacked record form as used by the app.  Pointers are
// either NULL or point to strings elsewhere on the card.  All strings
// are null character terminated.

typedef struct
{
	AddrOptionsType	options;        // Display by company or by name
	Char *			fields[addressFieldsCount];
} AddrDBRecordType;

typedef AddrDBRecordType *AddrDBRecordPtr;

// The labels for phone fields are stored specially.  Each phone field
// can use one of eight labels.  Part of those eight labels are stored
// where the phone field labels are.  The remainder (phoneLabelsStoredAtEnd)
// are stored after the labels for all the fields.

typedef char addressLabel[addrLabelLength];

typedef struct
{
	UInt16				renamedCategories;	// bitfield of categories with a different name
	char 					categoryLabels[dmRecNumCategories][dmCategoryLength];
	UInt8 				categoryUniqIDs[dmRecNumCategories];
	UInt8					lastUniqID;	// Uniq IDs generated by the device are between
	// 0 - 127.  Those from the PC are 128 - 255.
	UInt8					reserved1;	// from the compiler word aligning things
	UInt16				reserved2;
	AddrDBFieldLabelsDirtyFlags dirtyFieldLabels;
	addressLabel 		fieldLabels[addrNumFields + numPhoneLabelsStoredSecond];
	CountryType 		country;		// Country the database (labels) is formatted for
	//UInt8 				reserved;
	AddrDBMisc			misc;
} AddrAppInfoType;

typedef AddrAppInfoType *AddrAppInfoPtr;


/************************************************************
 * Function Prototypes
 *************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	AddrOptionsType		options;        // Display by company or by name
	AddrDBRecordFlags	flags;
	UInt8				companyFieldOffset;   // Offset from firstField
	char				firstField;
} PrvAddrPackedDBRecord;

#ifdef CONTACTS
	#define univAddressFields P1ContactsFields
	#define univAddressFieldsCount P1ContactsaddressFieldsCount
	#define univFirstPhoneField P1ContactsfirstPhoneField
	#define univLastPhoneField P1ContactslastPhoneField
	#define univIsPhoneField P1ContactsisPhoneField
	#define univAddrDBGetRecord PrvP1ContactsDBGetRecord
	#define univCompany P1Contactscompany
	#define univAddress P1Contactsaddress
	#define univCity P1Contactscity
	#define univName P1Contactsname
	#define univNote P1Contactsnote
	#define univCustom1 P1Contactscustom1
	#define univCustom2 P1Contactscustom2
	#define univCustom3 P1Contactscustom3
	#define univCustom4 P1Contactscustom4
	#define univFirstName P1ContactsfirstName
	#define univZipCode P1ContactszipCode
	#define univState P1Contactsstate
	#define univTitle P1Contactstitle
	#define univCountry P1Contactscountry
	#define univGetPhoneLabel P1ContactsGetPhoneLabel
	#define univPhone1 P1Contactsphone1
	#define univNumPhoneLabels P1ContactsnumPhoneLabels
	#define univAddrDBRecordType P1ContactsDBRecordType
	#define univAddrDBRecordPtr P1ContactsDBRecordType*
	#define univEmailLabel P1ContactsemailLabel
	#define univAdrDBAppInfoGetPtr P1ContactsDBAppInfoGetPtr
	#define univFirstRenameableLabel P1ContactsfirstRenameableLabel
	#define univLastRenameableLabel P1ContactslastRenameableLabel
	#define univAppInfoPtr P1ContactsAppInfoPtr
	#define univPrvAddrPackedDBRecord PrvP1ContactsPackedDBRecord
	#define univAddrDBAppInfoGetPtr P1ContactsDBAppInfoGetPtr
	#define univFaxLabel P1ContactsfaxLabel
	#define univPagerLabel P1ContactspagerLabel
	#define univMobileLabel P1ContactsmobileLabel
	#define univMainLabel P1ContactsmainLabel
	#define univHomeLabel P1ContactshomeLabel
	#define univWorkLabel P1ContactsworkLabel
	#define univOtherLabel P1ContactsotherLabel
	#define univAddrNumFields P1ContactsNumFields
	#define univNumPhoneLabelsStoredFirst P1ContactsnumPhoneLabelsStoredFirst
	#define univAddrDBNewRecord P1ContactsDBNewRecord
	#define univAddrPackedDBRecord PrvP1ContactsPackedDBRecord
	#define univAddrDBUnpack PrvP1ContactsDBUnpack
	#define univAddrDBRecordFlags P1ContactsDBRecordFlags
	#define univNumPhoneFields	P1ContactsnumPhoneFields
#else
	#define univAddressFields AddressFields
	#define univAddressFieldsCount addressFieldsCount
	#define univFirstPhoneField firstPhoneField
	#define univLastPhoneField lastPhoneField
	#define univIsPhoneField isPhoneField
	#define univAddrDBGetRecord AddrDBGetRecord
	#define univCompany company
	#define univAddress address
	#define univCity city
	#define univName name
	#define univNote note
	#define univCustom1 custom1
	#define univCustom2 custom2
	#define univCustom3 custom3
	#define univCustom4 custom4
	#define univFirstName firstName
	#define univZipCode zipCode
	#define univState state
	#define univTitle title
	#define univCountry country
	#define univGetPhoneLabel GetPhoneLabel
	#define univPhone1 phone1
	#define univNumPhoneLabels numPhoneLabels
	#define univAddrDBRecordType AddrDBRecordType
	#define univAddrDBRecordPtr AddrDBRecordType*
	#define univEmailLabel emailLabel
	#define univAdrDBAppInfoGetPtr AddrDBAppInfoGetPtr
	#define univFirstRenameableLabel firstRenameableLabel
	#define univLastRenameableLabel lastRenameableLabel
	#define univAppInfoPtr AddrAppInfoPtr	
	#define univPrvAddrPackedDBRecord PrvAddrPackedDBRecord
	#define univAddrDBAppInfoGetPtr AddrDBAppInfoGetPtr
	#define univFaxLabel faxLabel
	#define univPagerLabel pagerLabel
	#define univMobileLabel mobileLabel
	#define univMainLabel mainLabel
	#define univHomeLabel homeLabel
	#define univWorkLabel workLabel
	#define univOtherLabel otherLabel
	#define univAddrNumFields addrNumFields
	#define univNumPhoneLabelsStoredFirst numPhoneLabelsStoredFirst
	#define univAddrDBNewRecord AddrDBNewRecord
	#define univAddrPackedDBRecord PrvAddrPackedDBRecord
	#define univAddrDBUnpack PrvAddrDBUnpack
	#define univAddrDBRecordFlags AddrDBRecordFlags
	#define univNumPhoneFields	numPhoneFields

#endif


#ifdef LIBDEBUG
void			PrvAddrDBLocalizeAppInfo(UInt16 refNum, AddrAppInfoPtr appInfoP);
void 			UpdateAlarms(UInt16 refNum, DmOpenRef addrXTDB);
AddrAppInfoPtr	AddrDBAppInfoGetPtr(UInt16 refNum, DmOpenRef dbP);
Err				AddrDBChangeSortOrder(UInt16 refNum, DmOpenRef dbP, UInt16 sortByCompany);
Boolean			AddrDBLookupSeekRecord (UInt16 refNum, DmOpenRef dbP, UInt16 *indexP, Int16 *phoneP, Int16 offset, Int16 direction, AddressLookupFields field1, AddressLookupFields field2, AddressFields lookupFieldMap[]);
Boolean			AddrDBLookupString(UInt16 refNum, DmOpenRef dbP, Char *key, UInt16 sortByCompany, UInt16 category, UInt16 *recordP, Boolean *completeMatch,Boolean masked);
Boolean			AddrDBLookupLookupString(UInt16 refNum, DmOpenRef dbP, Char *key, UInt16 sortByCompany, AddressLookupFields field1, AddressLookupFields field2, UInt16 *recordP, Int16 *phoneP, AddressFields lookupFieldMap[],
				 	Boolean *completeMatch, Boolean *uniqueMatch);
Err				AddrDBGetDatabase (UInt16 refNum, DmOpenRef *addrPP, UInt16 mode);
UInt16 			PrvAddrDBFindSortPosition(UInt16 refNum, DmOpenRef dbP, void *newRecord);
void 			PrvAddrDBFindKey(UInt16 refNum, void *r, char **key, UInt16 *whichKey, UInt16 sortByCompany);

Err				AddrDBAppInfoInit(UInt16 refNum, DmOpenRef dbP);
Err				AddrDBNewRecord(UInt16 refNum, DmOpenRef dbP, AddrDBRecordPtr r, UInt16 *index);
Err				AddrDBGetRecord(UInt16 refNum, DmOpenRef dbP, UInt16 index, AddrDBRecordPtr recordP, MemHandle *recordH);
Int16			PrvAddrDBUnpackedSize(UInt16 refNum, AddrDBRecordPtr r);
void			PrvAddrDBPack(UInt16 refNum, AddrDBRecordPtr s, void * recordP);
void			PrvAddrDBUnpack(UInt16 refNum, PrvAddrPackedDBRecord *src, AddrDBRecordPtr dest);
void			AddrDBChangeCountry(UInt16 refNum, void* appInfoP);
UInt16 			PrvAddrDBStrCmpMatches(UInt16 refNum, const Char* s1, const Char* s2);
Boolean 		PrvAddrDBSeekVisibleRecordInCategory (UInt16 refNum, DmOpenRef dbR, UInt16 * indexP, UInt16 offset, Int16 direction, UInt16 category, Boolean masked);
#else
extern void		PrvAddrDBLocalizeAppInfo(UInt16 refNum, AddrAppInfoPtr appInfoP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvAddrDBLocalizeAppInfo);

extern void		UpdateAlarms(UInt16 refNum, DmOpenRef addrXTDB)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapUpdateAlarms);
extern AddrAppInfoPtr 	AddrDBAppInfoGetPtr(UInt16 refNum, DmOpenRef dbP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBAppInfoGetPtr);
extern Err		AddrDBChangeSortOrder(UInt16 refNum, DmOpenRef dbP, UInt16 sortByCompany)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBChangeSortOrder);
extern Boolean	AddrDBLookupSeekRecord(UInt16 refNum, DmOpenRef dbP, UInt16 *indexP, Int16 *phoneP, Int16 offset, Int16 direction, AddressLookupFields field1, AddressLookupFields field2, AddressFields lookupFieldMap[])
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBLookupSeekRecord);
extern Boolean	AddrDBLookupString(UInt16 refNum, DmOpenRef dbP, Char *key, UInt16 sortByCompany, UInt16 category, UInt16 *recordP, Boolean *completeMatch,Boolean masked)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBLookupString);
extern Boolean	AddrDBLookupLookupString(UInt16 refNum, DmOpenRef dbP, Char *key, UInt16 sortByCompany, AddressLookupFields field1, AddressLookupFields field2, UInt16 *recordP, Int16 *phoneP, AddressFields lookupFieldMap[],
				 	Boolean *completeMatch, Boolean *uniqueMatch)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBLookupLookupString);
extern Err		AddrDBGetDatabase(UInt16 refNum, DmOpenRef *addrPP, UInt16 mode)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBGetDatabase);
extern UInt16	PrvAddrDBFindSortPosition(UInt16 refNum, DmOpenRef dbP, void *newRecord)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvAddrDBFindSortPosition);
extern void		PrvAddrDBFindKey(UInt16 refNum, void *r, char **key, UInt16 *whichKey, UInt16 sortByCompany)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvAddrDBFindKey);

extern Err		AddrDBAppInfoInit(UInt16 refNum, DmOpenRef dbP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBAppInfoInit);
extern Err		AddrDBNewRecord(UInt16 refNum, DmOpenRef dbP, AddrDBRecordPtr r, UInt16 *index)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBNewRecord);
extern Err		AddrDBGetRecord(UInt16 refNum, DmOpenRef dbP, UInt16 index, AddrDBRecordPtr recordP, MemHandle *recordH)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBGetRecord);
extern Int16	PrvAddrDBUnpackedSize(UInt16 refNum, AddrDBRecordPtr r)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvAddrDBUnpackedSize);
extern void		PrvAddrDBPack(UInt16 refNum, AddrDBRecordPtr s, void * recordP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvAddrDBPack);
extern void		PrvAddrDBUnpack(UInt16 refNum, PrvAddrPackedDBRecord *src, AddrDBRecordPtr dest)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvAddrDBUnpack);
extern void		AddrDBChangeCountry(UInt16 refNum, void* appInfoP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapAddrDBChangeCountry);
extern UInt16	PrvAddrDBStrCmpMatches(UInt16 refNum, const Char* s1, const Char* s2)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvAddrDBStrCmpMatches);
extern Boolean	PrvAddrDBSeekVisibleRecordInCategory(UInt16 refNum, DmOpenRef dbR, UInt16 * indexP, UInt16 offset, Int16 direction, UInt16 category, Boolean masked)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvAddrDBSeekVisibleRecordInCategory);
#endif

Int16			PrvAddrDBComparePackedRecords(void *r1,
												void *r2,
												Int16 sortByCompany,
												SortRecordInfoPtr info1,
												SortRecordInfoPtr info2,
												MemHandle appInfoH);
												
Boolean AddrDBLookupStringEx(DmOpenRef dbP, Char * key, UInt16 sortByCompany, UInt16 category, UInt16 * recordP, Boolean *completeMatch, Boolean masked, UInt16 iWhichKey, UInt16 startRecord, WinDirectionType searchDir);
void 	PrvAddrDBFindKeyEx(void *r, char **key, UInt16 fieldIndex, UInt16 sortByCompany);

#ifdef __cplusplus
}
#endif
