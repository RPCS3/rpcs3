#pragma once

#include <tchar.h>

extern void	CfgSetSettingsDir( const char* dir );

extern bool	CfgFindName( const TCHAR *Section, const TCHAR* Name);

extern void	CfgWriteBool(const TCHAR* Section, const TCHAR* Name, bool Value);
extern void	CfgWriteInt(const TCHAR* Section, const TCHAR* Name, int Value);
extern void	CfgWriteStr(const TCHAR* Section, const TCHAR* Name, const TCHAR *Data);

extern bool	CfgReadBool(const TCHAR *Section,const TCHAR* Name, bool Default);
extern void	CfgReadStr(const TCHAR* Section, const TCHAR* Name, TCHAR* Data, int DataSize, const TCHAR* Default);
extern int	CfgReadInt(const TCHAR* Section, const TCHAR* Name,int Default);
