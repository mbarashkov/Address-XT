#include "Reg.h"
#include "Address.h"
#include "AddressRsc.h"
#include "AddrTools.h"

#ifdef LIBDEBUG //moved to shared library

UInt32 CheckRegistration(UInt16 adxtRefNum)
{
	globalVars* globals = getGlobalsPtr();
	Char lStr[255], lCode[255];
	UInt32 res = 0;
	UInt16 len;
	UInt32 i, code, code2, seconds, temp, curSeconds, diffSeconds;
	Char userName[dlkUserNameBufSize], userNameLower[dlkUserNameBufSize];
	UInt16 prefsSize=0;
	DateTimeType date;
	globals->gTrialExpired=false;
	MemSet(lStr, 255, 0);
	PrefGetAppPreferences(CREATORID, addrPrefRegID, 0, &prefsSize, true);
	if(prefsSize==0)
	{
		res=0;
	}
	else
	{
		PrefGetAppPreferences(CREATORID, addrPrefRegID, &lStr, &prefsSize, true);
	}
	//Get HotSync User name
	DlkGetSyncInfo(0, 0, 0, userName, 0, 0);
	code=userName[0];
	temp=1;
	len = StrLen(userName);
	//FrmCustomAlert(alertDebug, userName, 0, 0);
	if(len==0)
	{
		MemSet(lStr, 255, 0);
		//check if installation date is recorded
		prefsSize=0;
		PrefGetAppPreferences(CREATORID, addrInstalledRegID, 0, &prefsSize, true);
		if(prefsSize==0)
		{
			seconds=TimGetSeconds();
			StrIToA(lStr, seconds);
			PrefSetAppPreferences(CREATORID, addrInstalledRegID, 0, &lStr, StrLen(lStr), true);	
		}
		else
		{
			PrefGetAppPreferences(CREATORID, addrInstalledRegID, &lStr, &prefsSize, true);
			seconds=StrAToI(lStr);
			curSeconds=TimGetSeconds();
				diffSeconds=curSeconds-seconds;
			TimSecondsToDateTime(diffSeconds, &date);
			globals->gTrialDays=date.day;
			
			if((date.year>1904)||(date.month>1)||(date.day>30))
			{
				globals->gTrialExpired=true;
			}	
		}	
		return false;
	}
	//FrmCustomAlert(alertDebug, lStr, 0, 0);
	for (i=0; (i < len) && (userName[i] != '\0'); i++)
	{
		code += (((userName[i] & 0xA1) << 7) | (userName[i] & 0x31));
		code = ((code << 6) | ((code & 0x7000) >> 12));
	}
	for (i=len-1; (i >0) && (userName[i] != '\0'); i--)
	{
		code *= (((userName[i] & 0xD2) << 7) | (userName[i] & 0x27));
		code = ((code << 5) | ((code & 0x7200) >> 12));
	}
	
	code*=611;
	
	if(code==0 && len>2)
	{
		code=userName[0];
		for (i=0; (i < len-1) && (userName[i] != '\0'); i++)
		{
			code += (((userName[i] & 0xA1) << 7) | (userName[i] & 0x31));
			code = ((code << 6) | ((code & 0x7000) >> 12));
		}
		for (i=len-2; (i >0) && (userName[i] != '\0'); i--)
		{
			code *= (((userName[i] & 0xD2) << 7) | (userName[i] & 0x27));
			code = ((code << 5) | ((code & 0x7200) >> 12));
		}
		code*=611;	
	}
	
	if(code==0)
	{
		for (i=0; (i < len) && (userName[i] != '\0'); i++)
		{
			code += (((userName[i] & 0xD1) << 5) | (userName[i] & 0x21));
			code = ((code << 3) | ((code & 0x3321) >> 12));
		}
		for (i=len-1; (i >0) && (userName[i] != '\0'); i--)
		{
			code *= (((userName[i] & 0xB2) << 6) | (userName[i] & 0x37));
			code = ((code << 5) | ((code & 0x7631) >> 13));
		}
		code*=511;
	}
	if(code==0)
	{
		for (i=0; (i < len) && (userName[i] != '\0'); i++)
		{
			code += (((userName[i] & 0xD1) << 5) | (userName[i] & 0x21));
			code = ((code << 3) | ((code & 0x3321) >> 12));
		}
		for (i=len-1; (i >0) && (userName[i] != '\0'); i--)
		{
			code *= (((userName[i] & 0xB2) << 6) | (userName[i] & 0x37));
			code = ((code << 5) | ((code & 0x7631) >> 13));
		}		
		code*=511;
	}	
	
	if(code==0)
	{
		for (i=0; (i < len) && (userName[i] != '\0'); i++)
		{
			code += (((userName[i] & 0xD3) << 7) | (userName[i] & 0x22));
			code += ((code << 2) | ((code & 0x2311) >> 10));
		}
		for (i=len-1; (i >0) && (userName[i] != '\0'); i--)
		{
			if((((userName[i] & 0xA2) << 4) | (userName[i] & 0x17))!=0)
				code *= (((userName[i] & 0xA2) << 4) | (userName[i] & 0x17));
			code -= ((code << 3) | ((code & 0x2641) >> 11));
		}		
		code*=157;
	}	
	StrIToA(lCode, code);
	if(code == StrAToI(lStr))
		res=1;
	else
	{
		StrToLower(userNameLower, userName);
		code=userNameLower[0];
		for (i=0; (i < len) && (userNameLower[i] != '\0'); i++)
		{
			code += (((userNameLower[i] & 0xA1) << 7) | (userNameLower[i] & 0x31));
			code = ((code << 6) | ((code & 0x7000) >> 12));
		}
		for (i=len-1; (i >0) && (userNameLower[i] != '\0'); i--)
		{
			code *= (((userNameLower[i] & 0xD2) << 7) | (userNameLower[i] & 0x27));
			code = ((code << 5) | ((code & 0x7200) >> 12));
		}
		
		code*=611;
		
		if(code==0 && len>2)
		{
			code=userNameLower[0];
			for (i=0; (i < len-1) && (userNameLower[i] != '\0'); i++)
			{
				code += (((userNameLower[i] & 0xA1) << 7) | (userNameLower[i] & 0x31));
				code = ((code << 6) | ((code & 0x7000) >> 12));
			}
			for (i=len-2; (i >0) && (userNameLower[i] != '\0'); i--)
			{
				code *= (((userNameLower[i] & 0xD2) << 7) | (userNameLower[i] & 0x27));
				code = ((code << 5) | ((code & 0x7200) >> 12));
			}
			code*=611;	
		}
		
		if(code==0)
		{
			for (i=0; (i < len) && (userNameLower[i] != '\0'); i++)
			{
				code += (((userNameLower[i] & 0xD1) << 5) | (userNameLower[i] & 0x21));
				code = ((code << 3) | ((code & 0x3321) >> 12));
			}
			for (i=len-1; (i >0) && (userNameLower[i] != '\0'); i--)
			{
				code *= (((userNameLower[i] & 0xB2) << 6) | (userNameLower[i] & 0x37));
				code = ((code << 5) | ((code & 0x7631) >> 13));
			}
			code*=511;
		}
		if(code==0)
		{
			for (i=0; (i < len) && (userNameLower[i] != '\0'); i++)
			{
				code += (((userNameLower[i] & 0xD1) << 5) | (userNameLower[i] & 0x21));
				code = ((code << 3) | ((code & 0x3321) >> 12));
			}
			for (i=len-1; (i >0) && (userNameLower[i] != '\0'); i--)
			{
				code *= (((userNameLower[i] & 0xB2) << 6) | (userNameLower[i] & 0x37));
				code = ((code << 5) | ((code & 0x7631) >> 13));
			}		
			code*=511;
		}	
		
		if(code==0)
		{
			for (i=0; (i < len) && (userNameLower[i] != '\0'); i++)
			{
				code += (((userNameLower[i] & 0xD3) << 7) | (userNameLower[i] & 0x22));
				code += ((code << 2) | ((code & 0x2311) >> 10));
			}
			for (i=len-1; (i >0) && (userNameLower[i] != '\0'); i--)
			{
				if((((userNameLower[i] & 0xA2) << 4) | (userNameLower[i] & 0x17))!=0)
					code *= (((userNameLower[i] & 0xA2) << 4) | (userNameLower[i] & 0x17));
				code -= ((code << 3) | ((code & 0x2641) >> 11));
			}		
			code*=157;
		}			
	}	
	
	
	if((Int32)code == StrAToI(lStr))
		res=1;
	if(res == 0 && len > 0) // Now check for Address XT + Manage XT bundle serial.
	{
		code2=userName[0];
		for (i=0; (i < len) && (userName[i] != '\0'); i++)
		{
			code2 += (((userName[i] & 0xA1) << 7) | (userName[i] & 0x31));
			code2 = ((code2 << 4) | ((code2 & 0x7321) >> 11));
		}
		for (i=len-1; (i >0) && (userName[i] != '\0'); i--)
		{
			code2 *= (((userName[i] & 0xD2) << 7) | (userName[i] & 0x27));
			code2 = ((code2 << 4) | ((code2 & 0x7331) >> 14));
		}
		
		code2*=547;
	
		if(code2==0 && len>2)
		{
			code2=userName[0];
			for (i=0; (i < len-1) && (userName[i] != '\0'); i++)
			{
				code2 += (((userName[i] & 0xA3) << 5) | (userName[i] & 0x34));
				code2 = ((code2 << 3) | ((code2 & 0x7320) >> 12));
			}
			for (i=len-2; (i >0) && (userName[i] != '\0'); i--)
			{
				code2 *= (((userName[i] & 0xD2) << 7) | (userName[i] & 0x27));
				code2 = ((code2 << 5) | ((code2 & 0x7200) >> 12));
			}
			code2*=611;	
		}
		if(code2==0)
		{
			for (i=0; (i < len) && (userName[i] != '\0'); i++)
			{
				code2 += (((userName[i] & 0xD1) << 5) | (userName[i] & 0x21));
				code2 = ((code2 << 2) | ((code2 & 0x3321) >> 11));
			}
			for (i=len-1; (i >0) && (userName[i] != '\0'); i--)
			{
				code2 *= (((userName[i] & 0xB2) << 6) | (userName[i] & 0x37));
				code2 = ((code2 << 3) | ((code2 & 0x7631) >> 10));
			}		
			code2*=411;			
		}
		if(code2==0)
		{		
			for (i=0; (i < len) && (userName[i] != '\0'); i++)
			{
				code2 += (((userName[i] & 0xD1) << 4) | (userName[i] & 0x13));
				code2 = ((code2 << 1) | ((code2 & 0x3321) >> 10));
			}
			for (i=len-1; (i >0) && (userName[i] != '\0'); i--)
			{
				code2 *= ((((userName[i] & 0xB2) << 4) | (userName[i] & 0x21))+3);
				code2 = ((code2 << 2) | ((code2 & 0x7631) >> 9));
			}		
			code2*=311;
		}
		if((Int32)(code+code2) == StrAToI(lStr))
			res=1;
		if(res == 0)
		{
			StrToLower(userNameLower, userName);
			code2=userNameLower[0];
			for (i=0; (i < len) && (userNameLower[i] != '\0'); i++)
			{
				code2 += (((userNameLower[i] & 0xA1) << 7) | (userNameLower[i] & 0x31));
				code2 = ((code2 << 4) | ((code2 & 0x7321) >> 11));
			}
			for (i=len-1; (i >0) && (userNameLower[i] != '\0'); i--)
			{
				code2 *= (((userNameLower[i] & 0xD2) << 7) | (userNameLower[i] & 0x27));
				code2 = ((code2 << 4) | ((code2 & 0x7331) >> 14));
			}
			
			code2*=547;
		
			if(code2==0 && len>2)
			{
				code2=userNameLower[0];
				for (i=0; (i < len-1) && (userNameLower[i] != '\0'); i++)
				{
					code2 += (((userNameLower[i] & 0xA3) << 5) | (userNameLower[i] & 0x34));
					code2 = ((code2 << 3) | ((code2 & 0x7320) >> 12));
				}
				for (i=len-2; (i >0) && (userNameLower[i] != '\0'); i--)
				{
					code2 *= (((userNameLower[i] & 0xD2) << 7) | (userNameLower[i] & 0x27));
					code2 = ((code2 << 5) | ((code2 & 0x7200) >> 12));
				}
				code2*=611;	
			}
			if(code2==0)
			{
				for (i=0; (i < len) && (userNameLower[i] != '\0'); i++)
				{
					code2 += (((userNameLower[i] & 0xD1) << 5) | (userNameLower[i] & 0x21));
					code2 = ((code2 << 2) | ((code2 & 0x3321) >> 11));
				}
				for (i=len-1; (i >0) && (userNameLower[i] != '\0'); i--)
				{
					code2 *= (((userNameLower[i] & 0xB2) << 6) | (userNameLower[i] & 0x37));
					code2 = ((code2 << 3) | ((code2 & 0x7631) >> 10));
				}		
				code2*=411;			
			}
			if(code2==0)
			{		
				for (i=0; (i < len) && (userNameLower[i] != '\0'); i++)
				{
					code2 += (((userNameLower[i] & 0xD1) << 4) | (userNameLower[i] & 0x13));
					code2 = ((code2 << 1) | ((code2 & 0x3321) >> 10));
				}
				for (i=len-1; (i >0) && (userNameLower[i] != '\0'); i--)
				{
					code2 *= ((((userNameLower[i] & 0xB2) << 4) | (userNameLower[i] & 0x21))+3);
					code2 = ((code2 << 2) | ((code2 & 0x7631) >> 9));
				}		
				code2*=311;
			}	
		}
		if((Int32)(code+code2) == StrAToI(lStr))
			res=1;		
	}

	//PRINTDEBUG(code - 7235);
	if(res==0)
	{
		MemSet(lStr, 255, 0);
		//check if installation date is recorded
		prefsSize=0;
		PrefGetAppPreferences(CREATORID, addrInstalledRegID, 0, &prefsSize, true);
		if(prefsSize==0)
		{
			seconds=TimGetSeconds();
			StrIToA(lStr, seconds);
			PrefSetAppPreferences(CREATORID, addrInstalledRegID, 0, &lStr, StrLen(lStr), true);	
		}
		else
		{
			PrefGetAppPreferences(CREATORID, addrInstalledRegID, &lStr, &prefsSize, true);
			seconds=StrAToI(lStr);
			curSeconds=TimGetSeconds();
				diffSeconds=curSeconds-seconds;
			TimSecondsToDateTime(diffSeconds, &date);
			globals->gTrialDays=date.day;
			if((date.year>1904)||(date.month>1)||(date.day>30))
			{
				globals->gTrialExpired=true;
			}		
		}	
	}
	return res;
}

#endif