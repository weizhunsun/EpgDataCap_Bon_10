﻿#ifndef __PATH_UTIL_H__
#define __PATH_UTIL_H__

#include "CommonDef.h"

void GetDefSettingPath(wstring& strPath);

void GetSettingPath(wstring& strPath);

void GetModuleFolderPath(wstring& strPath);

void GetModuleIniPath(wstring& strPath);

void GetCommonIniPath(wstring& strPath);

void GetEpgTimerSrvIniPath(wstring& strPath);

void GetEpgSavePath(wstring& strPath);

void GetLogoSavePath(wstring& strPath);

void GetRecFolderPath(wstring& strPath);

void GetFileTitle(wstring strPath, wstring& strTitle);

void GetFileName(wstring strPath, wstring& strName);

void GetFileFolder(wstring strPath, wstring& strFolder);

BOOL IsExt(wstring filePath, wstring ext);

void CheckFileName(wstring& fileName);

#endif