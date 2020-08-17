/******************************************************************************
 *
 * Copyright (c) 1995-2002 PalmSource, Inc. All rights reserved.
 *
 * File: AddressRsc.h
 *
 * Release: Palm OS 5 SDK (68K) R3.
 *
 *****************************************************************************/

#pragma once

#define	kFrmNavHeaderFlagsObjectFocusStartState  0x00000001
#define	kFrmNavHeaderFlagsAppFocusStartState     0x00000002

//New Link Dialog
#define NewLinkDialog						6100
#define NewLinkList							6107
#define NewLinkOkButton						6103
#define NewLinkCancelButton					6104


//Links List
#define LinksDialog							6000
#define LinksDone							6003
#define LinksNew							6005
#define LinksDelete							6006
#define LinksGoTo							6007
#define LinksTable							6001
#define LinksScrollbar						6002

// List View
#define ListView							1000
#define ListCategoryTrigger					1003
#define ListCategoryList					1004
#define ListTable							1005
#define ListLookupLabel						1006
#define ListLookupField						1007
#define ListNewButton						1008
#define ListUpButton						1009
#define ListDownButton						1010
#define ListNoteBmpPalmHRStd				5100
#define ListNoteBmpPalmHRSmall				5101
#define ListNoteBmpStd						5102
#define ListNoteBmpSmall					5103

#define ListLinkBmpPalmHRStd				5200
#define ListLinkBmpPalmHRSmall				5201
#define ListLinkBmpStd						5202
#define ListLinkBmpSmall					5203

#define ListRecentButton					1001

#define ListRecentList						1002
#define ListLinksList						1012

#define ListSignalGadget					1013
#define ListBatteryGadget					1014
#define ListLookupAdd						1015
#define ListLookupCancel					1016
//Skin elements
#define	SkinMainTitleBarStd					6101
#define	SkinMainTitleBarHR					6100
#define	SkinMainTitleBarSonyHR				6201

#define BmpRecentHr							7200
#define BmpRecentHrPressed					7300

#define BmpMemoStd							7400
#define BmpMemoHr							7500
#define BmpMemoSonyHr						7600

#define BmpTodoStd							7700
#define BmpTodoHr							7800
#define BmpTodoSonyHr						7900

#define BmpAddrStd							8000
#define BmpAddrHr							8100
#define BmpAddrSonyHr						8200

#define BmpDateStd							8300
#define BmpDateHr							8400
#define BmpDateSonyHr						8500

//Serial number dialog
#define SerialNumberDialog					5200
#define SNOK								5201
#define SNCancel							5202
#define SNField								5204

//Address List options dialog
#define AddressListOptionsDialog			5300
#define AddressListOptionsOkButton			5301
#define AddressListOptionsCancelButton		5302
#define AddressListOptionsOneHanded			5304
#define AddressListOptionsAdvancedFind		5318
#define AddressListOptionsSortList			5307
#define AddressListOptionsDelimiterList		5316
#define AddressListOptionsSortTrigger		5308
#define AddressListOptionsDelimiterTrigger	5317
#define AddressListOptionsResortNow			5314
#define AddressListOptionsShowNameOnly		5319
#define AddressListOptionsTouchMode			5320
#define AddressListOptionsTapDialingCheckbox	5305

#define AddressListOptionsRecentEnableCheckbox	5309
#define AddressListOptionsRecentNumberLabel		5310
#define AddressListOptionsRecentNumberTrigger	5311
#define AddressListOptionsRecentNumberList		5312	
#define AddressListOptionsRecentShowAll			5313		

//Color Options
#define ColorOptionsDialog					5500
#define ColorOptionsOkButton				5501
#define ColorOptionsCancelButton			5502 
#define ColorTextSelector					5508
#define ColorBackSelector					5506
#define ColorSelSelector					5509
#define ColorDefaults						5505
#define ColorTextLabel						5503
#define ColorBackLabel						5504
#define ColorSelLabel						5507
#define ColorEachOtherLabel					5510
#define ColorEachOtherSelector				5511
#define ColorEachOther						5512
#define ColorThemeLabel						5514
#define ColorThemeTrigger					5513
#define ColorThemeList						5515
#define ColorInactiveLabel					5516
#define ColorInactiveSelector				5517

//Button Options
#define ButtonOptionsDialog					5400
#define ButtonOptionsOkButton				5401
#define ButtonOptionsCancelButton			5402 
#define ButtonFiveWayList					5405
#define ButtonFiveWayLabel					5403
/*#define ButtonJogDialList					5407
#define ButtonJogDialLabel					5406
#define ButtonStdList						5410
#define ButtonStdLabel						5409
*/
#define ButtonFiveWayTrigger				5404
/*#define ButtonJogDialTrigger				5408
#define ButtonStdTrigger					5411
*/

#define PreferencesMap						5412

// Details Dialog Box
#define DetailsDialog						1200
#define DetailsCategoryTrigger				1204
#define DetailsCategoryList					1205
#define DetailsSecretCheckbox				1207
#define DetailsOkButton						1208
#define DetailsCancelButton					1209
#define DetailsDeleteButton					1210
#define DetailsNoteButton					1211
#define DetailsPhoneList					1213
#define DetailsPhoneTrigger					1214

// Birthday Dialog Box
#define BirthdayDialog						6700
#define BirthdayDateSelector				6701
#define BirthdayOkButton					6702
#define BirthdayCancelButton				6703
#define BirthdayDateClear					6704
#define BirthdayRemindDays					6705
#define BirthdayRemindCheck 				6706

// Options Dialog
#define OptionsDialog						1400
#define OptionsSortByPriority				1403
#define OptionsSortByDueDate				1404
#define OptionsShowCompleted				1405
#define OptionsShowDueItems					1407
#define OptionsShowDueDates					1409
#define OptionsShowPriorities				1411
#define OptionsOkButton						1413
#define OptionsCancelButton					1414

// Address Dialog
#define AddressDialog						6200
#define AddressList							6207
#define AddressTomTomTrigger				6202
#define AddressTomTomList					6205
#define AddressOkButton						6203
#define AddressCancelButton					6204
#define AddressChooseTypeLabel				6201

// Detele Completed Dialog
#define DeleteCompletedDialog				1500
#define DeleteCompletedSaveBackup			1504
#define DeleteCompletedOk					1506
#define DeleteCompletedCancel				1507

// Delete Addr Dialog
#define DeleteAddrDialog					1600
#define DeleteAddrSaveBackup				1604
#define DeleteAddrOk						1606
#define DeleteAddrCancel					1607
//Purge Category Dialog
#define PurgeCatDialog						2400
#define PurgeCatSaveBackup					2404
#define PurgeCatOk							2406
#define PurgeCatCancel						2407

// Address Record View
#define RecordView							1700
#define RecordCategoryLabel					1702
#define RecordDoneButton					1704
#define RecordEditButton					1705
#define RecordNewButton						1706
#define RecordUpButton						1707
#define RecordDownButton					1708
#define RecordViewDisplay					1709
#define RecordViewBusinessCardBmp			1710
#define RecordDialButton					1701

#define RecordLinksButton					1703
#define RecordLinksList						1711


// Edit Address View
#define EditView							1800
#define EditCategoryTrigger					1803
#define EditCategoryList					1804
#define EditTable							1805
#define EditDoneButton						1806
#define EditDetailsButton					1807
#define EditUpButton						1808
#define EditDownButton						1809
#define EditPhoneList						1810
#define EditIMList							1831
#define EditAddressList						1832
#define EditNoteButton						1812
#define EditViewBusinessCardBmp				1813
#define EditBirthdaySelector				1833
#define EditAnnivSelector					1834
#define EditRingtoneList					1835
#define EditRingerTrigger					1836
#define EditPhotoMenu						1837
//Company data form
#define CompanyData		                    3000
#define CompanyDataCancelButton		        3002
#define CompanyDataOkButton	                3003
#define CompanyScrollUp				        3020
#define CompanyScrollDown			        3021

#define CompanyDataPrefDialog				5700
#define CompanyDataPrefOK					5701
#define CompanyDataPrefCancel				5702
#define CompanyDataEmptyCheck				5703


#define CompanyCompanyNameField   	       3009
#define CompanyAddressField     	         3011
#define CompanyCityField       	          3013
#define CompanyZipCodeField   	           3015
#define CompanyStateField                3017
#define CompanyCountryField              3019
#define CompanyEmaildomainField          3023

#define CompanyCompanyLabel         3008
#define CompanyAddressLabel         3010
#define CompanyCityLabel            3012
#define CompanyZipCodeLabel        3014
#define CompanyStateLabel           3016
#define CompanyCountryLabel         3018
#define CompanyEmaildomainLabel     3022

#define CompanyList					3024

#define OldCompanyField 3005 

#define HeadingLabel 3007 

#define SelectorTrigger        3006
#define FieldnotenteredAlert                      1200 
#define NothingtodoAlert                          1300
#define SendMailFailedAlert								4996
#define CompleteAlert 1100
#define ConfirmAlert 1000
#define PurgeAlert 4999
#define PalmOSVersionAlert						4998
#define FontsAlert 4997
#define WelcomeAlert	5396


//About form
#define AboutForm  4000 
#define AboutOkButton 4001
#define AboutVersionField 4003
#define AboutHotSyncNameField 4009
#define AboutSNButton 4010
#define AboutTrialDay 4012
#define alertExpiredTrial 5098
#define AboutReg1 4005
#define AboutReg2 4008
#define AboutReg3 4009
#define AboutReg4 4011
#define AboutReg5 4010








// Custom Edit
#define CustomEditDialog					1900
#define CustomEditFirstField				1903

#define CustomEditOkButton					1901
#define CustomEditCancelButton				1902

#define alertDebug							2096
// Preferences
#define PreferencesDialog						2000
#define PreferencesRememberCategoryCheckbox		2001
#define PreferencesRememberRecordCheckbox		2016
#define PreferencesAutoBluetooth				2015
#define PreferencesDefaultCalendar				2012
#define PreferencesDefaultTasks					2013
#define PreferencesDefaultMemos					2014
#define PreferencesCalendarCrID					2007
#define PreferencesTasksCrID					2009
#define PreferencesMemosCrID					2011
#define PreferencesCrIDAlert					2050
//
#define PreferencesLastName						2003
#define PreferencesCompanyName					2004
#define PreferencesOkButton						2005
#define PreferencesCancelButton					2006
#define PreferencesEnableTapDialingHeightGadget	2007
#define PreferencesEnableHRGadget				2012
#define PreferencesFirstLast					2010
#define PreferencesCompanyFirst					2011
#define PreferencesCompanyTitle					2013
#define PreferencesLastTitle					2014
#define PreferencesFullText						2002

#define BirthdaysDialog							5900
#define BirthdaysMonthSelector					5905
#define BirthdaysTable							5901
#define BirthdaysScrollbar						5902
#define BirthdaysOkButton						5903
// Lookup View
#define LookupView							2100
#define LookupTitle							2101
#define LookupTable							2102
#define LookupLookupField					2104
#define LookupPasteButton					2105
#define LookupCancelButton					2106
#define LookupUpButton						2107
#define LookupDownButton					2108

// Sorting Message
#define SortingMessageDialog				2200
#define SortingMessageLabel					2201

// Addr Dial list
#define DialListDialog                      2300
#define DialListDialButton                  2301
#define DialListCancelButton                2302
#define DialListNumberField                 2305
#define DialListDescriptionGadget           2304
#define DialListPhoneRectangleGadget        2307
#define DialListNumberToDialLabel           2306
#define DialListList                        2303

// Delete Note Alert
#define DeleteNoteAlert						2001
#define DeleteNoteYes						0
#define DeleteNoteNo           			  	1

#define NameNeededAlert						2003

// Select Business Card Alert
#define SelectBusinessCardAlert				2004
#define SelectBusinessCardYes				0
#define SelectBusinessCardNo        		1

// Send Business Card Alert
#define SendBusinessCardAlert				2005
// Menus
#define ListViewMenuBar						1000
#define ListViewTreoMenuBar					1500
#define RecordViewMenuBar					1100
#define EditViewMenuBar						1200
#define EditViewMenuBarTreo					1600
#define CompanyDataMenuBarTreo				1700
#define GeneralOptionsMenuBarTreo			1800

// Menu commands
#define ListRecordDeleteRecordMenuCmd		100
#define ListRecordDuplicateAddressCmd		101
#define ListRecordDialCmd					102
#define ListRecordSeparator1				103

#define ListRecordBeamCategoryCmd			108
#define ListRecordSendCategoryCmd			109
#define ListRecordBeamBusinessCardCmd		110
#define ListRecordCompanyData				112
#define ListRecordManageXTCmd				113
#define ListRecordPurgeCategory				106
#define ListRecordBirthdays					104

// The below two menu commands aren't actually on the menu anymore, but
// it is easiest to pretend that they are, for the command bar's sake.

#define ListRecordDeleteRecordCmd			114
#define ListRecordBeamRecordCmd				115

#define ListEditUndoCmd						10000
#define ListEditCutCmd						10001
#define ListEditCopyCmd						10002
#define ListEditPasteCmd					10003
#define ListEditSelectAllCmd				10004
#define ListEditSeparator					10005
#define ListEditKeyboardCmd					10006
#define ListEditGraffitiLegacyHelpCmd		10007
#define ListEditGraffitiHelpCmd	    		10008




#define ListOptionsFontCmd					300
#define ListOptionsListByCmd				301
#define ListOptionsConnect					302
#define ListOptionsSearch					303
#define ListOptionsButton					304
#define ListOptionsColor					305

#define ListOptionsEditCustomFldsCmd		307
#define ListOptionsSecurityCmd				308
#define ListOptionsAboutCmd					309

#define ListHelpTips						1300

#define RecordRecordDeleteRecordCmd			100
#define RecordRecordDuplicateAddressCmd		101
#define RecordRecordBeamRecordCmd			102
#define RecordRecordSendRecordCmd			103
#define RecordRecordDialCmd					104
#define RecordRecordMapCmd					106
#define RecordRecordSeparator1				107
#define RecordRecordAttachNoteCmd			108
#define RecordRecordDeleteNoteCmd			109
#define RecordRecordSeparator2				110
#define RecordRecordSelectBusinessCardCmd 	111
#define RecordRecordBeamBusinessCardCmd 	112

#define RecordOptionsFontCmd				200
#define RecordOptionsGeneral				201
#define RecordOptionsConnect				202
#define RecordOptionsColors					203
#define RecordOptionsEditCustomFldsCmd		204
#define RecordOptionsAboutCmd				206

#define RecordHelpTips						1400

#define EditRecordDeleteRecordCmd			100
#define EditRecordDuplicateAddressCmd		101
#define EditRecordBeamRecordCmd				102
#define EditRecordSendRecordCmd				103
#define EditRecordDialCmd					104
#define EditRecordMapCmd					106
#define EditRecordLinksCmd 					107
#define EditRecordSeparator1				108
#define EditRecordAttachNoteCmd				109
#define EditRecordDeleteNoteCmd				110
#define EditRecordSeparator2				111
#define EditRecordSelectBusinessCardCmd 	112
#define EditRecordBeamBusinessCardCmd 		113

#define EditEditUndoCmd						10000
#define EditEditCutCmd						10001
#define EditEditCopyCmd						10002
#define EditEditPasteCmd					10003
#define EditEditSelectAllCmd				10004
#define EditSeparator						10005
#define EditEditKeyboardCmd					10006
#define EditEditGraffitiHelpCmd				10007

#define EditOptionsFontCmd					300
#define EditOptionsGeneral					301
#define EditOptionsConnect					302
#define EditOptionsColors					303
#define EditOptionsEditCustomFldsCmd		304

#define EditOptionsAboutCmd					306

#define EditHelpTips						1500

#define CompanyDataPreferences				1600
#define CompanyDataHelpTips					1700


// Strings
#define FindAddrHeaderStr					100
#define UnnamedRecordStr					1000
#define BeamDescriptionStr					1001
#define BeamFilenameStr						1002
#define DuplicatedRecordIndicatorStr		1003
#define ZipCodePrefixStr					1004
#define DeleteRecordStr						1005
#define BeamRecordStr						1006
#define ExgDescriptionStr					1007
#define ExgMultipleDescriptionStr			1008

#define AddressListHelp						9000
#define AddressViewHelp						9002
#define AddressEditHelp						9001
#define CompanyDataHelp						9003


// String lists
#define FieldNamesStrList					1000	// Reserved range up to 1255!!!

// Field mapping
#define FieldMapID							1000


#define titleAFInitStr						200
#define companyAFInitStr					201
#define cityAFInitStr						202
#define stateAFInitStr						203
#define countryAFInitStr					204


#define MemoListView						1100

#define MemoListCategoryTrigger				1103
#define MemoListCategoryList				1104
#define MemoListOK							1105
#define MemoListTable						1108
#define MemoListScrollBar					1109
#define MemoListCancel						1101

#define ToDoListView						2500
#define ToDoListCategoryTrigger				2503
#define ToDoListCategoryList				2504
#define ToDoListUpButton					2508
#define ToDoListDownButton					2509
#define ToDoListTable						2510


#define ToDoListOK							2505
#define ToDoListCancel						2506

#define alertDeleteLink						1400

// List View
#define ContactSelListView					2600
#define ContactSelListCategoryTrigger		2603
#define ListCategoryContactSelList			2604
#define ContactSelListTable					2605
#define ContactSelListLookupField			2607
#define ContactSelListOKButton				2612
#define ContactSelListCancelButton			2601
#define ContactSelListUpButton				2609
#define ContactSelListDownButton			2610

#define alertGoToPrivateLink				1500

#define alertLowMemory						1600
#define alertLibNotFound					5198
#define alertIntendedForNonContacts			5298
#define alertIntendedForContacts			5398

#define MapFailedAlert						5096
#define alertLinkExists						5196
#define alertLinkItself						5296

#define alertTomTomCardRemoved				5496
#define alertTomTomCardFull					5596
#define alertTomTomCopied					5696

// Custom Font Select Dialog
#define FontSelectDialog					6300
#define FontSelectOKButton					6301
#define FontSelectCancelButton				6302
#define FontSelectBtnGroup					1
#define FontSelectFont1						6303
#define FontSelectFont2						6304
#define FontSelectFont3						6305
#define FontSelectFont4						6306

// with HiRes fonts (small)
#define FontSelectHRDialog					6400
#define FontSelectHROKButton				6401
#define FontSelectHRCancelButton			6402
#define FontSelectHRFont1					6404
#define FontSelectHRFont2					6405
#define FontSelectHRFont3					6406
#define FontSelectHRFont4					6407
#define FontSelectHRFont5					6408
#define FontSelectHRFont6					6409
#define FontSelectHRFont7					6410
#define FontSelectHRFont8					6411
//
// Autogenerated symbols - 01.10.2006 21:52:40
// Falch.net PilRC Designer
//
#define	AT	6500
//
// Autogenerated symbols - 01.10.2006 21:54:30
// Falch.net PilRC Designer
//
#define	ConnectOptionsDialog	6501
#define	ConnectOptionsOKButton	6502
//
// Autogenerated symbols - 01.10.2006 22:09:26
// Falch.net PilRC Designer
//
#define	ConnectOptionsCancelButton	6503
//
// Autogenerated symbols - 01.10.2006 22:56:08
// Falch.net PilRC Designer
//
#define	DialListMessageButton	6504
#define	ConnectOptionsEmailLabel	6505
#define	ConnectOptionsDialLabel	6506
#define	ConnectOptionsSMSLabel	6507
#define	ConnectOptionsMapLabel	6508
#define	ConnectOptionsWebLabel	6509
//
// Autogenerated symbols - 01.10.2006 23:27:49
// Falch.net PilRC Designer
//
#define	ConnectOptionsIMLabel	6510
#define	ConnectOptionsNumberPrefix	6522
#define	ConnectOptionsNumberPrefixField	6523
#define	ConnectOptionsDialTrigger	6524
#define	ConnectOptionsSMSTrigger	6525
#define	ConnectOptionsEmailTrigger	6526
#define	ConnectOptionsMapTrigger	6527
#define	ConnectOptionsWebTrigger	6528
#define	ConnectOptionsIMTrigger	6529
#define	ConnectOptionsDialList	6530
#define	ConnectOptionsSMSList	6531
#define	ConnectOptionsEmailList	6532
#define	ConnectOptionsMapList	6533
#define	ConnectOptionsWebList	6534
#define	ConnectOptionsIMList	6535
