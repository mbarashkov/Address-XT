/******************************************************************************
 * Copyright (c) 1995-2005 palmOne, Inc. or its subsidiaries.
 * All rights reserved.
 *****************************************************************************/
/**
 * @defgroup PIM PIM Database Record Structures
 *
 * @{
 * @}
 */
/**
 * @ingroup PIM
 */

/**
 * @file AddressDB.h
 *
 * @brief Contains database record type and constants for Contacts application.
 *
 * Contacts application uses a different record format than the old AddressBook
 * application due to some feature enhancement and new data fields. This file
 * contains the structure of the record in Contacts DB and can be used to access
 * the database directly. One way to utilize this header file is to combine it
 * with the old Address Book source code so that the record packing and
 * unpacking routines are adjusted for the new structure.
 *
 * Please note that accessing the database directly is generally not recommended
 * and this is offered as a temporary solution for 3rd party developers. The
 * structure might change again in the future.
 *
 * <hr>
 */

#pragma once

#ifndef LIBDEBUG
#include "..\SharedLibrary\AddressXTLib\Src\AddressXTLib.h"
#endif

#include "ContactsDefines.h"

#include <AppLaunchCmd.h>
#include <DataMgr.h>
#include <DateTime.h>
#include "globals.h"

/** Contacts DB version number */
#define addrDBVersionNum        1

#ifndef appErrVersionIncompatible
#define appErrVersionIncompatible   (appErrorClass | 1) /**< Error code */
#endif

#define P1ContactsnumPhoneLabels          8 /**< Number of phone labels */
#define P1ContactsnumAddressLabels        3 /**< Number of address labels */
#define P1ContactsnumChatLabels           5 /**< Number of chat labels */

#define ALARM_NONE 255
 
/** Total number of fields related to birthday */
#define numBirthdayFields       3

#define addrLabelLength             16
typedef struct
{
    unsigned reserved:7;
    unsigned sortByCompany  :1;
} P1ContactsDBMisc;


/** Record field label string. */
typedef char P1ContactsaddressLabel[addrLabelLength];



/**
 *  Indices for fields member of AddrDBRecordType (record fields)
 **/
typedef enum
{
    P1Contactsname,
    P1ContactsfirstName,
    P1Contactscompany,
    P1Contactstitle,

    P1Contactsphone1,         /**< Phone fields. */
    P1Contactsphone2,
    P1Contactsphone3,
    P1Contactsphone4,
    P1Contactsphone5,
    P1Contactsphone6,
    P1Contactsphone7,

    P1Contactschat1,          /**< Instant message id and service. */
    P1Contactschat2,

    P1Contactswebpage,

    P1Contactscustom1,        /**< Custom fields. */
    P1Contactscustom2,
    P1Contactscustom3,
    P1Contactscustom4,
    P1Contactscustom5,
    P1Contactscustom6,
    P1Contactscustom7,
    P1Contactscustom8,
    P1Contactscustom9,

    P1Contactsaddress,        /**< Address fields. */
    P1Contactscity,
    P1Contactsstate,
    P1ContactszipCode,
    P1Contactscountry,
    P1Contactsaddress2,
    P1Contactscity2,
    P1Contactsstate2,
    P1ContactszipCode2,
    P1Contactscountry2,
    P1Contactsaddress3,
    P1Contactscity3,
    P1Contactsstate3,
    P1ContactszipCode3,
    P1Contactscountry3,

    P1Contactsnote,           /**< The note field must not be more than ~32K. */

    P1ContactsbirthdayDate,   /**< Birthday info. */
    P1ContactsbirthdayMask,   /**< Holds AddressDBBirthdayFlags type. */
    P1ContactsbirthdayPreset,


    /***************************************************************************
     * All fields starting from picture will be added as blob
     * Each blob is: 4 bytes id, 2 bytes length and data
     * App blobs will have reserved id (1 to 64k) to keep them separate from
     * third party blobs.
     **************************************************************************/

    P1ContactspictureInfo,    /**< holds picture blob: id --> 1, data  -->  2 bytes
    picture dirty flag, picture data */

    P1ContactsaddressFieldsCount,
    P1ContactsanniversaryDate,
    P1Contactsringer

} P1ContactsFields;

typedef struct
{
    union
    {
        struct
        {
            UInt32 P1Contactsreserved     :4;

            UInt32 P1Contactscountry      :1; /**< Set if country label is dirty. */
            UInt32 P1ContactszipCode      :1; /**< Set if zipcode label is dirty. */
            UInt32 P1Contactsstate        :1; /**< Set if state label is dirty. */
            UInt32 P1Contactscity         :1; /**< Set if city label is dirty. */
            UInt32 P1Contactsaddress      :1; /**< Set if address label is dirty. */

            UInt32 P1Contactscustom9      :1; /**< Set if custom/renameable label is
            dirty. */
            UInt32 P1Contactscustom8      :1;
            UInt32 P1Contactscustom7      :1;
            UInt32 P1Contactscustom6      :1;
            UInt32 P1Contactscustom5      :1;
            UInt32 P1Contactscustom4      :1;
            UInt32 P1Contactscustom3      :1;
            UInt32 P1Contactscustom2      :1;
            UInt32 P1Contactscustom1      :1;

            UInt32 P1Contactswebpage      :1; /**< Set if webpage label is dirty. */

            UInt32 P1Contactschat2        :1; /**< Set if instant message id is dirty. */
            UInt32 P1Contactschat1        :1;

            UInt32 P1Contactsphone7       :1; /**< Set if phone label is dirty. */
            UInt32 P1Contactsphone6       :1;
            UInt32 P1Contactsphone5       :1;
            UInt32 P1Contactsphone4       :1;
            UInt32 P1Contactsphone3       :1;
            UInt32 P1Contactsphone2       :1;
            UInt32 P1Contactsphone1       :1;
            UInt32 P1Contactstitle        :1; /**< Set if title label is dirty. */
            UInt32 P1Contactscompany      :1; /**< Set if company label is dirty. */
            UInt32 P1ContactsfirstName    :1; /**< Set if firstName label is dirty. */
            UInt32 P1Contactsname         :1; /**< Set if name label is dirty (bit 0). */

        } bits;
        UInt32 allBits;
    };
    union
    {
        struct
        {
            UInt32 P1Contactsreserved1        :12;
            UInt32 P1Contactsreserved2        :7;

            UInt32 P1ContactspictureInfo      :1; /**< Set if picture label is dirty. */

            UInt32 P1ContactsbirthdayDate     :1; /**< Set if birthday label is dirty. */

            UInt32 P1Contactsnote             :1; /**< Set if note field is dirty. */

            UInt32 P1Contactscountry3         :1; /**< Set if address fields are dirty. */
            UInt32 P1ContactszipCode3         :1;
            UInt32 P1Contactsstate3           :1;
            UInt32 P1Contactscity3            :1;
            UInt32 P1Contactsaddress3         :1;

            UInt32 P1Contactscountry2         :1;
            UInt32 P1ContactszipCode2         :1;
            UInt32 P1Contactsstate2           :1;
            UInt32 P1Contactscity2            :1;
            UInt32 P1Contactsaddress2         :1;

        } bits2;
        UInt32 allBits2;
    };
} P1ContactsDBFieldLabelsDirtyFlags;


/** Total number of fields related to birthday */
#define numBirthdayFields       3

/** Total number of fields in Contacts application */
#define P1ContactsNumFields           43

/** Number of fields that are non-string */
#define P1ContactsNumNonStringFields  4

/** Number of fields that will show up in UI */
#define P1ContactsaddrNumDisplayFields    P1ContactsNumFields - (numBirthdayFields -1)

/** Number of fields that are of type string */
#define P1ContactsNumStringFields     P1ContactsNumFields - P1ContactsNumNonStringFields

#define P1ContactsfirstAddressField           P1Contactsname    /**< First index of address field */
#define P1ContactsfirstPhoneField             P1Contactsphone1  /**< First index of phone field */
#define P1ContactslastPhoneField              P1Contactsphone7  /**< Last index of phone field */

/** Total number of phone fields */
#define P1ContactsnumPhoneFields              (P1ContactslastPhoneField - P1ContactsfirstPhoneField + 1)

#define firstChatField              P1Contactschat1   /**< First index of chat field */
#define lastChatField               P1Contactschat2   /**< Last index of chat field */

#define firstWebField               P1Contactswebpage /**< First index of web field */
#define lastWebField                P1Contactswebpage /**< Last index of web field */

/**
 *  Field label sub-indices relating to the position of the field
 *  label within a popup list of labels.
 *  e.g.: the phone label popup list first selection is "Work", the
 *        second is "Home", etc.
 **/

typedef enum
{
    P1ContactsworkLabel,
    P1ContactshomeLabel,
    P1ContactsfaxLabel,
    P1ContactsotherLabel,
    P1ContactsemailLabel,
    P1ContactsmainLabel,
    P1ContactspagerLabel,
    P1ContactsmobileLabel
} P1ContactsPhoneLabels;

/**
 *  Field label sub-indices relating to the position of the field
 *  label within a popup list of labels.
 *  e.g.: the phone label popup list first selection is "Work", the
 *        second is "Home", etc.
 **/

typedef enum
{
    otherChatLabel,
    aimChatLabel,
    msnChatLabel,
    yahooChatLabel,
    icqChatLabel

} AddressChatLabels;

/**
 *  Field label sub-indices relating to the position of the field
 *  label within a popup list of labels.
 *  e.g.: the phone label popup list first selection is "Work", the
 *        second is "Home", etc.
 **/

typedef enum
{
     P1ContactsworkAddressLabel,
     P1ContactshomeAddressLabel,
     P1ContactsotherAddressLabel
}  P1ContactsAddressStreetAddressLabels;

/** Maximum length of a field label, excluding NULL terminator: */

/** Phone labels: */
#define P1ContactsnumPhoneLabelsStoredFirst    P1ContactsnumPhoneFields
#define  P1ContactsnumPhoneLabelsStoredSecond  ( P1ContactsnumPhoneLabels -  P1ContactsnumPhoneLabelsStoredFirst)

/** Labels preceding the address field groups (work, home, other): */
/*@{*/
#define Addr1FieldGroupLabel        phone1
#define Addr2FieldGroupLabel        phone2
#define Addr3FieldGroupLabel        phone4
/*@}*/

/** The first and last custom/renameable labels: */
/*@{*/
#define  P1ContactsfirstRenameableLabel         P1Contactscustom1
#define  P1ContactslastRenameableLabel          P1Contactscustom9
/*@}*/

/** Last label in the first set of labels: */
#define P1ContactslastLabel                   P1ContactsaddrNumDisplayFields

/** Indices to start of second set of phone, address and chat labels: */
/*@{*/
#define  P1ContactsphoneLabelSecondStart       P1ContactsaddrNumDisplayFields -1
#define  P1ContactschatLabelstart              P1ContactsaddrNumDisplayFields
#define  P1ContactswebLabelStart               13
/*@}*/

/** Labels for the "Add field" list of fields: */
/*@{*/
#define  P1ContactsnumAddFieldLabels           3
#define  P1ContactsAddFieldLabelStart           P1ContactsaddrNumDisplayFields            \
                                    +  P1ContactsnumPhoneLabelsStoredSecond    \
                                    +  P1ContactsnumChatLabels
/*@}*/

/** Total field label count: */
/*@{*/
#define P1ContactsnumPictureFieldLabels       3
#define P1ContactsnumFieldLabels              P1ContactsaddrNumDisplayFields            \
                                    + P1ContactsnumPhoneLabelsStoredSecond    \
                                    + P1ContactsnumChatLabels                 \
                                    + P1ContactsnumAddFieldLabels             \
                                    + P1ContactsnumPictureFieldLabels
/*@}*/

typedef struct
{
    /** Bitfield of categories with a different name. */
    UInt16 renamedCategories;

    char   categoryLabels[dmRecNumCategories][dmCategoryLength];

    UInt8  categoryUniqIDs[dmRecNumCategories];

    /**
     * Unique IDs generated by the device are between 0 - 127.
     * Those from the PC are 128 - 255.
     */
    UInt8  lastUniqID;

    /** From the compiler word aligning things. */
    UInt8  reserved1;

    /**
     * Whether category colors were edited since last sync.
     * Least significant bit first.
     */
    UInt16 categoryColorsEdited;

    /**
     * Added as part of the Mullet version of this application,
     * so that we can later support color categories without
     * breaking the conduits.
     */
    UInt8  categoryColors[dmRecNumCategories];

    UInt16 reserved2;

    /** Labels that have changed. */
    P1ContactsDBFieldLabelsDirtyFlags dirtyFieldLabels;

    /** Labels displayed for record fields. */
    P1ContactsaddressLabel               fieldLabels[P1ContactsnumFieldLabels];

    /** Country for which the database (labels) is formatted. */
    CountryType                 country;

    UInt8                       reserved;

    P1ContactsDBMisc                  misc;

} P1ContactsAppInfoType;
typedef P1ContactsAppInfoType *P1ContactsAppInfoPtr;

/***********************************************************************
 *  Application Packed Record Format Related Data
 *
 *  Note: Records are stored in the database in packed format to
 *        conserve space.  When retrieving a record from the database,
 *        it is unpacked into the AddrDBRecordType.  AddrDBGetRecord()
 *        does the necessary unpacking for you.  When creating a new
 *        record, AddrDBNewRecord(), or saving a record, AddrDBChangeRecord(),
 *        the packing is handled for you.
 ***********************************************************************/

/** @brief Packed record flags.
 *
 * AddrDBRecordFlags indicates the address record bits.  It is used
 * to indicate field changes or fields contained in a packed record.
 * Since there is no 64-bit bitfield, a struct with two unsigned
 * longs is used. Please be sure to use the bit macros (defined
 * later below) to extract the bits, as the bit order may change in
 * future.
 */
typedef struct
{
    union
    {
        struct
        {
            UInt32 reserved     :4;

            UInt32 country      :1; /**< Set if record contains a country. */
            UInt32 zipCode      :1; /**< Set if record contains a zipCode. */
            UInt32 state        :1; /**< Set if record contains a state. */
            UInt32 city         :1; /**< Set if record contains a city. */
            UInt32 address      :1; /**< Set if record contains a address. */

            UInt32 custom9      :1; /**< Set if record contains a custom9. */
            UInt32 custom8      :1; /**< Set if record contains a custom8. */
            UInt32 custom7      :1; /**< Set if record contains a custom7. */
            UInt32 custom6      :1; /**< Set if record contains a custom6. */
            UInt32 custom5      :1; /**< Set if record contains a custom5. */
            UInt32 custom4      :1; /**< Set if record contains a custom4. */
            UInt32 custom3      :1; /**< Set if record contains a custom3. */
            UInt32 custom2      :1; /**< Set if record contains a custom2. */
            UInt32 custom1      :1; /**< Set if record contains a custom1. */

            UInt32 webpage      :1; /**< Set if record contains a URL. */

            UInt32 chat2        :1; /**< Set if record contains an instant
                                         message id 2. */
            UInt32 chat1        :1; /**< Set if record contains an instant
                                         message id 1. */

            UInt32 phone7       :1; /**< Set if record contains a phone7. */
            UInt32 phone6       :1; /**< Set if record contains a phone6. */
            UInt32 phone5       :1; /**< Set if record contains a phone5. */
            UInt32 phone4       :1; /**< Set if record contains a phone4. */
            UInt32 phone3       :1; /**< Set if record contains a phone3. */
            UInt32 phone2       :1; /**< Set if record contains a phone2. */
            UInt32 phone1       :1; /**< Set if record contains a phone1. */
            UInt32 title        :1; /**< Set if record contains a title. */
            UInt32 company      :1; /**< Set if record contains a company. */
            UInt32 firstName    :1; /**< Set if record contains a firstName. */
            UInt32 name         :1; /**< Set if record contains a name (bit 0).
            */

        } bits;
        UInt32 allBits;
    };
    union
    {
        struct
        {
            UInt32 reserved2        :6;
            UInt32 reserved         :12;

            UInt32 birthdayPreset   :1; /**< Set if record contains birthday
                                             preset. */
            UInt32 birthdayMask     :1; /**< Set if record contains birthday
                                             mask. */
            UInt32 birthdayDate     :1; /**< Set if record contains birthday
                                             date. */

            UInt32 note             :1; /**< Set if record contains a note
                                             handle. */

            UInt32 country3         :1; /**< Set if record contains a country3.
                                             */
            UInt32 zipCode3         :1; /**< Set if record contains a zipCode3.
                                             */
            UInt32 state3           :1; /**< Set if record contains a state3. */
            UInt32 city3            :1; /**< Set if record contains a city3. */
            UInt32 address3         :1; /**< Set if record contains a address3.
                                             */

            UInt32 country2         :1; /**< Set if record contains a country2.
                                             */
            UInt32 zipCode2         :1; /**< Set if record contains a zipCode2.
                                             */
            UInt32 state2           :1; /**< Set if record contains a state2. */
            UInt32 city2            :1; /**< Set if record contains a city2. */
            UInt32 address2         :1; /**< Set if record contains a address2.
                                             */


        } bits2;
        UInt32 allBits2;
    };
} P1ContactsDBRecordFlags;

/***********************************************************************
 *  Application Unpacked Record Format Related Data
 ***********************************************************************/

/** @brief Label types.
 *
 * This describes the label types for those user selectable label
 * fields: phone, instant messenger, and address.
 */
typedef struct
{
    union
    {
        struct
        {
            unsigned displayPhoneForList:4; /**< The phone displayed for the
                                                 List View, corresponding to the
                                                 item in the AddressPhoneLabels
                                                 enum. */
            unsigned phone7:4;              /**< Which phone label (work, home,
                                                 fax, other, ...) for this phone
                                                 field, value is an
                                                 AddressPhoneLabels enum. */
            unsigned phone6:4;
            unsigned phone5:4;
            unsigned phone4:4;
            unsigned phone3:4;
            unsigned phone2:4;
            unsigned phone1:4;
        } phones;
        UInt32 phoneBits;
    };

    union
    {
        struct
        {
            unsigned int reserved : 4;
            unsigned int address3 : 4;  /**< Which address label (work, home,
                                             other) for this address field,
                                             value is an
                                             AddressStreetAddressLabels enum. */
            unsigned int address2 : 4;
            unsigned int address1 : 4;
        }addresses;
        UInt16 addressBits;
    };

    union
    {
        struct
        {
            unsigned int reserved:8;
            unsigned int chat2:4;       /**< Which chat label (other, aim, msn,
                                          ...) for this chat field, value is an
                                             AddressChatLabels enum. */
            unsigned int chat1:4;

        }chatIds;
        UInt16 chatBits;
    };

} P1ContactsOptionsType;

/** @brief Birthday data flags
 *
 * On/off flags for birthday features.
 */
typedef struct
{
    unsigned short reserved     :15;
    unsigned short alarm        :1; /**< Indicates if a reminder is set for this
                                         birthday. */
}
AddressDBBirthdayFlags;

/** @brief Birthday format
 *
 * This is the format of the birthday data in the record.
 */
typedef struct
{
    DateType                birthdayDate;   /**< Contains birthdate. Year should
                                                 be considered if year mask it
                                                 set. */
    AddressDBBirthdayFlags  birthdayMask;
    UInt8                   birthdayPreset; /**< Number of days for advance
                                                 reminder. */
} BirthdayInfo;

/** @brief Picture info
 *
 * AddressDBPictureInfo is a temp struct to hold the picture data.Within the
 * address record picture is stored as blob data.
 */
typedef struct
{
    UInt16  pictureDirty;   /**< set when a picture is edited or added. */
    UInt16  pictureSize;    /**< size of the image in bytes */
    void*   pictureData;    /**< image data in jpeg format, limit is 4k */
} AddressDBPictureInfo;

/** Total number of blobs a record is allowed to have */
#define apptMaxBlobs                    10

/** @brief Format of each blob attached to a record. */
typedef struct
{
    UInt32                  creatorID;
    UInt16                  size;       /**< Excludes space used by blob ID
                                             and size. */
    void *                  content;    /**< Remainder of blob is content */
} BlobType;

typedef BlobType * BlobPtr;

/** @brief Record format
 *
 * This is the unpacked record format as used by the app.  Pointers are
 * either NULL or point to strings elsewhere on the card.  All strings
 * are NULL character terminated.
 */
typedef struct
{
    /** Display by company or by name */
    P1ContactsOptionsType         options;

    /** Container for all fields of type string */
    Char *                  fields[P1ContactsaddrNumStringFields];

    /** Container to hold birthday information */
    BirthdayInfo            birthdayInfo;

    /**
     * Container to hold picture information: dirty flags, followed
     * by size and the actual image */
    AddressDBPictureInfo    pictureInfo;

    /** Number of blobs the record has excluding the picture blob */
    UInt16                  numBlobs;

    /** Container to hold blobs and the metadata */
    BlobType                blobs[apptMaxBlobs];

} P1ContactsDBRecordType;

typedef P1ContactsDBRecordType *P1ContactsDBRecordPtr;

/***********************************************************************
 *  Contacts Application Info Block Related Data
 ***********************************************************************/



/** @brief Miscellaneous application data
 *
 * This includes any miscellaneous application wide data.
 */


/** Macro to determine whether a lookup field is for phone lookup */
#define P1ContactsIsPhoneLookupField(p)   (addrLookupWork <= (p) && (p) <= \
                                 addrLookupMobile)

#define firstRecordFlagBits2Field   P1Contactsaddress2

/**
 * Get a bit mask for the given bit position.
 *
 * The pos argument is used to indicate what bit position
 * should be set in the 4-byte bit mask, so a pos = 2 yields:
 * 0000 0000 0000 0010
 *
 * @param pos   Position of the bit to be set in the 4 byte
 *                  bit mask.
 * @return      A 4-byte bit mask.
 */
#define P1ContactsBitAtPosition(pos)  ((UInt32)1 <<                               \
                            ((pos < firstRecordFlagBits2Field) ?        \
                              pos:(pos - firstRecordFlagBits2Field)) )

/**
 * Get the record flag value for the given flag index.
 *
 * @param recFlags  A AddrDBRecordFlags struct.
 * @param index     The flag index. i.e.: the flag bit position.
 *
 * @return          The value of the record flag for the given flag index.
 */
#define P1ContactsGetBitMacro(recFlags, index)    ((index < firstRecordFlagBits2Field) ? \
                                        ((recFlags).allBits) & P1ContactsBitAtPosition(  \
                                          index):((recFlags).allBits2) &       \
                                          P1ContactsBitAtPosition(index))

/**
 * Set the record flag value at the given flag index.
 *
 * @param recFlags  A AddrDBRecordFlags struct.
 * @param index     The index of the flag to set. i.e.:
 *                      the bit position of the flag to set.
 *
 * @return          Nothing.
 */
#define P1ContactsSetBitMacro(recFlags, index)    if(index < firstRecordFlagBits2Field)\
                                          (recFlags).allBits |= P1ContactsBitAtPosition(index);  else \
                                          (recFlags).allBits2 |=  P1ContactsBitAtPosition(index)

/**
 * Clear the record flag value at the given flag index.
 * This clears or sets the flag value to zero (0).
 *
 * @param recFlags  A AddrDBRecordFlags struct.
 * @param index     The index of the flag to clear. i.e.:
 *                      the bit position of the flag to set.
 *
 * @retval          Nothing.
 */
#define P1ContactsRemoveBitMacro(recFlags, index) if(index < firstRecordFlagBits2Field)  \
                                        ((recFlags).allBits) &= ~P1ContactsBitAtPosition \
                                        (index); else                          \
                                        ((recFlags).allBits2) &=               \
                                        ~P1ContactsBitAtPosition(index)

#define P1ContactsSetBitMacroBits(bitField, index) (bitField |= ~P1ContactsBitAtPosition(index))

#define PictureFieldListStart   addrNumDisplayFields +          \
                                numPhoneLabelsStoredSecond +    \
                                numChatLabels +                 \
                                numAddFieldLabels

#define phoneLabelNone          -1

#define noPhoneField            0x0000

// <chg 20-sep-99 dia> These are phone fields that you might want to make
// a voice phone call to...
#define IsVoiceLabel(f) (((f) != P1ctFaxPhone) && ((f) != P1ctEmailPhone))

#define IsPhoneLookupField(p) (addrLookupWork <= (p) && (p) <= addrLookupMobile)

#define IsSMSLabel(f) (((f) == P1ctMobilePhone) || ((f) ==P1ctEmailPhone) ||   \
                       ((f) == P1ctOtherPhone))

#define IsSMSLabelNoEmail(f) (((f) == P1ctMobilePhone) ||   \
                              ((f) == P1ctOtherPhone))

/******************************************************************************
 * BLOB ID definition
 * Internal blob id will start from 'Bd00'.
 * Creator ids from 'Bd00 - 'Bd09' is registered for this purpose.
 ******************************************************************************/

/** picture blob id */
#define addrPictureBlobId                       'Bd00'

// Need this constant to hack the fix for a Mullet Outlook conduit bug, which is
// already shipped.
#define appInfoBlockSizeMullet  1092

#define kCallerIDMaxPhoneNumberDigits           63

// This is the number of digits in a phone number that are significant...
// Only check the last 8 chars to allow number to be stored in international
// format to still match if bufferLen is greater than key len just use the key
// length
#define kCallerIDSignificantPhoneNumberDigits   8

#define kStrLenRequiredForMatch                 6

// This structure is only for the exchange of address records.
typedef union
{
    struct
    {
        unsigned reserved       :8;

        // Typically only one of these are set

        unsigned email          :1; /**< set if data is an email address */
        unsigned fax            :1; /**< set if data is a fax */
        unsigned pager          :1; /**< set if data is a pager */
        unsigned voice          :1; /**< set if data is a phone */

        unsigned mobile         :1; /**< set if data is a mobile phone */

        // These are set in addition to other flags.

        unsigned work           :1; /**< set if phone is at work */
        unsigned home           :1; /**< set if phone is at home */

        // Set if this number is preferred over others.  May be preferred
        // over all others.  May be preferred over other emails.  One
        // preferred number should be listed next to the person's name.

        unsigned preferred      :1; /**< set if this phone is preferred (bit 0)
                                         */
    } bits;
    UInt32 allBits;
} P1ContactsDBPhoneFlags;

typedef struct {
	P1ContactsOptionsType		options;            // Display by company or by name
	P1ContactsDBRecordFlags	flags;
	UInt8				companyFieldOffset; // Offset from firstField
	char				firstField;
} PrvP1ContactsPackedDBRecord;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


#ifdef LIBDEBUG
Err PrvP1ContactsDBChangeRecord(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 *index, P1ContactsDBRecordPtr r,
                       P1ContactsDBRecordFlags changedFields, UInt16 BlobDirty);
Int16 PrvP1ContactsDBUnpackedSize(UInt16 adxtRefNum, P1ContactsDBRecordPtr r);
void PrvP1ContactsDBUnpack(UInt16 adxtRefNum, PrvP1ContactsPackedDBRecord *src, P1ContactsDBRecordPtr dest);
void PrvP1ContactsDBPack(UInt16 adxtRefNum, P1ContactsDBRecordPtr s, void * recordP);
P1ContactsAppInfoPtr   P1ContactsDBAppInfoGetPtr(UInt16 adxtRefNum, DmOpenRef dbP);
Err P1ContactsDBNewRecord(UInt16 adxtRefNum, DmOpenRef dbP, P1ContactsDBRecordPtr r, UInt16 *index);
Err 	PrvP1ContactsDBGetRecord(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 index, P1ContactsDBRecordPtr recordP,
				  MemHandle *recordH);
#else
extern Err		PrvP1ContactsDBChangeRecord(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 *index, P1ContactsDBRecordPtr r,
                       P1ContactsDBRecordFlags changedFields, UInt16 BlobDirty)
				ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvP1ContactsDBChangeRecord);
extern Int16		PrvP1ContactsDBUnpackedSize(UInt16 adxtRefNum, P1ContactsDBRecordPtr r)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvP1ContactsDBUnpackedSize);
extern void		PrvP1ContactsDBUnpack(UInt16 adxtRefNum, PrvP1ContactsPackedDBRecord *src, P1ContactsDBRecordPtr dest)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvP1ContactsDBUnpack);
extern void		PrvP1ContactsDBPack(UInt16 adxtRefNum, P1ContactsDBRecordPtr s, void * recordP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvP1ContactsDBPack);
extern P1ContactsAppInfoPtr		P1ContactsDBAppInfoGetPtr(UInt16 adxtRefNum, DmOpenRef dbP)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapP1ContactsDBAppInfoGetPtr);
extern Err		P1ContactsDBNewRecord(UInt16 adxtRefNum, DmOpenRef dbP, P1ContactsDBRecordPtr r, UInt16 *index)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapP1ContactsDBNewRecord);
extern Err		PrvP1ContactsDBGetRecord(UInt16 adxtRefNum, DmOpenRef dbP, UInt16 index, P1ContactsDBRecordPtr recordP,
				  MemHandle *recordH)
					ADDRESSXTLIB_LIB_TRAP(adxtLibTrapPrvP1ContactsDBGetRecord);
#endif

#ifdef __cplusplus
}
#endif
