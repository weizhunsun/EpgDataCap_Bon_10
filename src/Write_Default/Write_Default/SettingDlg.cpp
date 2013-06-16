﻿#include "StdAfx.h"
#include "SettingDlg.h"
#include "resource.h"

wstring g_size = L"";

CSettingDlg::CSettingDlg(void)
{
}


CSettingDlg::~CSettingDlg(void)
{
}

DWORD CSettingDlg::CreateSettingDialog(HINSTANCE hInstance, HWND parentWnd)
{
	DWORD ret = 0;

	g_size = this->size;
	ret = (DWORD)DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG_SET), parentWnd, (DLGPROC)DlgProc );
	if( ret == IDOK ){
		this->size = g_size;
	}

	return ret;
}

LRESULT CALLBACK CSettingDlg::DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
		case WM_KEYDOWN:
			if(wp == VK_RETURN){
				{
					WCHAR buff[1024] = L"";
					GetDlgItemText(hDlgWnd,IDC_EDIT_SIZE, buff, 1024);
					g_size = buff;
				}
				EndDialog(hDlgWnd, IDOK);
			}
			break;
		case WM_INITDIALOG:
			SetDlgItemText(hDlgWnd, IDC_EDIT_SIZE, g_size.c_str());
			return FALSE;
        case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
					{
						WCHAR buff[1024] = L"";
						GetDlgItemText(hDlgWnd,IDC_EDIT_SIZE, buff, 1024);
						g_size = buff;
					}
					EndDialog(hDlgWnd, IDOK);
					break;
				case IDCANCEL:
					EndDialog(hDlgWnd, IDCANCEL);
					break;
				default:
					return FALSE;
				}
		default:
			return FALSE;
	}
	return TRUE;
}

