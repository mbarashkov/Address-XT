#include "Reg.h"

UInt32 CheckRegistration()
{
	Char lStr[255], lCode[255];
	UInt32 res = 0;
	UInt32 i, code, seconds, temp, curSeconds, diffSeconds;
	Char userName[dlkUserNameBufSize], userNameLower[dlkUserNameBufSize];
	UInt16 prefsSize=0;
	DateTimeType date;
	UInt16 len;
	gTrialExpired=false;
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
	//DlkGetSyncInfo(0, 0, 0, userName, 0, 0);
	StrCopy(userName, "Delta Data Systems");
	code=userName[0];
	temp=1;
	len = StrLen(userName);
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
			gTrialDays=date.day;
			
			if((date.year>1904)||(date.month>1)||(date.day>30))
			{
				gTrialExpired=true;
			}	
		}	
		return false;
	}
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
		for (i=StrLen(userNameLower)-1; (i >0) && (userNameLower[i] != '\0'); i--)
		{
			code *= (((userNameLower[i] & 0xD2) << 7) | (userNameLower[i] & 0x27));
			code = ((code << 5) | ((code & 0x7200) >> 12));
		}
		
		code*=611;
		
		if(code==0 && StrLen(userNameLower)>2)
		{
			code=userNameLower[0];
			for (i=0; (i < StrLen(userNameLower)-1) && (userNameLower[i] != '\0'); i++)
			{
				code += (((userNameLower[i] & 0xA1) << 7) | (userNameLower[i] & 0x31));
				code = ((code << 6) | ((code & 0x7000) >> 12));
			}
			for (i=StrLen(userNameLower)-2; (i >0) && (userNameLower[i] != '\0'); i--)
			{
				code *= (((userNameLower[i] & 0xD2) << 7) | (userNameLower[i] & 0x27));
				code = ((code << 5) | ((code & 0x7200) >> 12));
			}
			code*=611;	
		}
		
		if(code==0)
		{
			for (i=0; (i < StrLen(userNameLower)) && (userNameLower[i] != '\0'); i++)
			{
				code += (((userNameLower[i] & 0xD1) << 5) | (userNameLower[i] & 0x21));
				code = ((code << 3) | ((code & 0x3321) >> 12));
			}
			for (i=StrLen(userNameLower)-1; (i >0) && (userNameLower[i] != '\0'); i--)
			{
				code *= (((userNameLower[i] & 0xB2) << 6) | (userNameLower[i] & 0x37));
				code = ((code << 5) | ((code & 0x7631) >> 13));
			}
			code*=511;
		}
		if(code==0)
		{
			for (i=0; (i < StrLen(userNameLower)) && (userNameLower[i] != '\0'); i++)
			{
				code += (((userNameLower[i] & 0xD1) << 5) | (userNameLower[i] & 0x21));
				code = ((code << 3) | ((code & 0x3321) >> 12));
			}
			for (i=StrLen(userNameLower)-1; (i >0) && (userNameLower[i] != '\0'); i--)
			{
				code *= (((userNameLower[i] & 0xB2) << 6) | (userNameLower[i] & 0x37));
				code = ((code << 5) | ((code & 0x7631) >> 13));
			}		
			code*=511;
		}	
		
		if(code==0)
		{
			for (i=0; (i < StrLen(userNameLower)) && (userNameLower[i] != '\0'); i++)
			{
				code += (((userNameLower[i] & 0xD3) << 7) | (userNameLower[i] & 0x22));
				code += ((code << 2) | ((code & 0x2311) >> 10));
			}
			for (i=StrLen(userNameLower)-1; (i >0) && (userNameLower[i] != '\0'); i--)
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
			gTrialDays=date.day;
			if((date.year>1904)||(date.month>1)||(date.day>30))
			{
				gTrialExpired=true;
			}		
		}	
	}
	return res;
}
