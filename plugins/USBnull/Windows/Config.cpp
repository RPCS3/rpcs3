#include <stdlib.h>

#include "../USB.h"

extern HINSTANCE hInst;
void SaveConfig() 
{

    Config *Conf1 = &conf;
	char *szTemp;
	char szIniFile[256], szValue[256];

	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return;
	strcpy(szTemp, "\\inis\\usbnull.ini");
	sprintf(szValue,"%u",Conf1->Log);
    WritePrivateProfileString("Interface", "Logging",szValue,szIniFile);

}

void LoadConfig() {
   FILE *fp;


    Config *Conf1 = &conf;
	char *szTemp;
	char szIniFile[256], szValue[256];
  
	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return ;
	strcpy(szTemp, "\\inis\\usbnull.ini");
    fp=fopen("inis\\usbnull.ini","rt");//check if usbnull.ini really exists
	if (!fp)
	{
		CreateDirectory("inis",NULL); 
        memset(&conf, 0, sizeof(conf));
        conf.Log = 0;//default value
		SaveConfig();//save and return
		return ;
	}
	fclose(fp);
	GetPrivateProfileString("Interface", "Logging", NULL, szValue, 20, szIniFile);
	Conf1->Log = strtoul(szValue, NULL, 10);
	return ;

}

