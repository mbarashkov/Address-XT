#ifndef MAPOPOLIS_H
#define MAPOPOLIS_H

#define MapopolisLaunch 51001
#define MapopolisCallbackLaunch 51002
// the definition of the parameter block that is sent to Mapopolis
typedef struct
{
	unsigned char interfaceVersion;
	unsigned char command;
	unsigned int licenseCode;
	char streetAddress[50];
	char cityName[25];
	char state[3];
	char zip[6];
	char zipExtension[5];
	char callbackAppName[33];
	unsigned long callbackCreatorID;
	char callbackParameterBlock[100];
} MapopolisLaunchParameterBlock;


typedef struct
{
	int parameter1;
	char parameter2[10];
} MapopolisCallbackInfoBlock;

#define MAPOPOLIS (DmFindDatabase(0, MAPOPOLIS_APPNAME)!=0)

#endif
