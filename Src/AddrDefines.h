#pragma once

#include <Event.h>

// Update codes, used to determine how the address list view should
// be redrawn.
#define updateRedrawAll						0x01
#define updateGrabFocus						0x02
#define updateItemHide						0x04
#define updateCategoryChanged				0x08
#define updateFontChanged					0x10
#define updateListViewPhoneChanged			0x20
#define updateCustomFieldLabelChanged		0x40
#define updateSelectCurrentRecord			0x80
#define updateColorsChanged					0x100
#define updateRecent						0x200
#define updateBirthday						0x200
#define updatePrefs							0x400
#define updateDisplayOptsChanged			0x800
// AutoFill database types and names
// Note that we prefix with "Address" to avoid name conflicts with Expense app
#define titleDBType							'titl'
#define titleDBName							"AddressTitlesDB"

#define companyDBType						'cmpy'
#define companyDBName						"AddressCompaniesDB"

#define cityDBType							'city'
#define cityDBName							"AddressCitiesDB"

#define stateDBType							'stat'
#define stateDBName							"AddressStatesDB"

#define countryDBType						'cnty'
#define countryDBName						"AddressCountriesDB"

#define addrDBName							"AddressDB"
#define addrDBType							'DATA'

#define shortenedFieldString				"..."
#define shortenedFieldLength				3
#define fieldSeparatorString				", "
#define fieldSeparatorLength				2
#define spaceBetweenNamesAndPhoneNumbers	6

#define editFirstFieldIndex					0

#define	kDialListShowInListPhoneIndex		((UInt16)(-1))

#define addrNumFields						19
#define numPhoneLabels						8

#define noRecord							0xffff

#define GetPhoneLabel(r, p)					(((r)->options.phoneBits >> (((p) - firstPhoneField) << 2)) & 0xF)

#define SetPhoneLabel(r, p, pl)				((r)->options.phoneBits = \
											((r)->options.phoneBits & ~((UInt32) 0x0000000F << (((p) - firstPhoneField) << 2))) | \
											((UInt32) pl << (((p) - firstPhoneField) << 2)))

#define kFrmCustomUpdateEvent				firstUserEvent