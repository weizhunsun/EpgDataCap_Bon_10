﻿// RecName_Macro.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"


#include "RecName_PlugIn.h"
#include "SettingDlg.h"
#include "ConvertMacro.h"
#include <tchar.h>

#define PLUGIN_NAME L"マクロ PlugIn"

extern HINSTANCE g_instance;

//PlugInの名前を取得する
//nameがNULL時は必要なサイズをnameSizeで返す
//通常nameSize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// name						[OUT]名称
// nameSize					[IN/OUT]nameのサイズ(WCHAR単位)
BOOL WINAPI GetPlugInName(
	WCHAR* name,
	DWORD* nameSize
	)
{
	if( name == NULL ){
		if( nameSize == NULL ){
			return FALSE;
		}else{
			*nameSize = (DWORD)wcslen(PLUGIN_NAME)+1;
		}
	}else{
		if( nameSize == NULL ){
			return FALSE;
		}else{
			if( *nameSize < (DWORD)wcslen(PLUGIN_NAME)+1 ){
				*nameSize = (DWORD)wcslen(PLUGIN_NAME)+1;
				return FALSE;
			}else{
				wcscpy_s(name, *nameSize, PLUGIN_NAME);
			}
		}
	}
	return TRUE;
}

//PlugInで設定が必要な場合、設定用のダイアログなどを表示する
//引数：
// parentWnd				[IN]親ウインドウ
void WINAPI Setting(
	HWND parentWnd
	)
{
	WCHAR dllPath[512] = L"";
	GetModuleFileName(g_instance, dllPath, 512);

	wstring iniPath = dllPath;
	iniPath += L".ini";

	WCHAR buff[1024] = L"";
	GetPrivateProfileString(L"SET", L"Macro", L"$Title$.ts", buff, 1024, iniPath.c_str());

	CSettingDlg dlg;
	dlg.macro = buff;
	if( dlg.CreateSettingDialog(g_instance, parentWnd) == IDOK ){
		WritePrivateProfileString(L"SET", L"Macro", dlg.macro.c_str(), iniPath.c_str());
	}
}

//入力された予約情報を元に、録画時のファイル名を作成する（拡張子含む）
//recNameがNULL時は必要なサイズをrecNamesizeで返す
//通常recNamesize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// info						[IN]予約情報
// recName					[OUT]名称
// recNamesize				[IN/OUT]nameのサイズ(WCHAR単位)
BOOL WINAPI ConvertRecName(
	PLUGIN_RESERVE_INFO* info,
	WCHAR* recName,
	DWORD* recNamesize
	)
{
	if( recNamesize == NULL ){
		return FALSE;
	}
	WCHAR dllPath[512] = L"";
	GetModuleFileName(g_instance, dllPath, 512);

	wstring iniPath = dllPath;
	iniPath += L".ini";

	WCHAR buff[1024] = L"";
	GetPrivateProfileString(L"SET", L"Macro", L"$Title$.ts", buff, 1024, iniPath.c_str());

	wstring convert = L"";
	CConvertMacro convertMacro;
	BOOL ret = convertMacro.Convert(buff, info, convert);
	if( recName == NULL ){
		*recNamesize = (DWORD)convert.size()+1;
	}else{
		if( *recNamesize < (DWORD)convert.size()+1 ){
			*recNamesize = (DWORD)convert.size()+1;
			ret = FALSE;
		}else{
			wcscpy_s(recName, *recNamesize, convert.c_str());
		}
	}

	return ret;
}
