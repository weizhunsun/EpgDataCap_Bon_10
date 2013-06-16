﻿// EpgTimerPlugIn.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "EpgTimerPlugIn.h"

#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"

extern HINSTANCE g_hinstDLL;

// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CEpgTimerPlugIn;
}


CEpgTimerPlugIn::CEpgTimerPlugIn()
{
	this->nwMode = FALSE;
	this->ctrlDlg = NULL;
	this->fullScreen = FALSE;
	this->showNormal = TRUE;
}

CEpgTimerPlugIn::~CEpgTimerPlugIn()
{
	if( this->nwMode == TRUE ){
		this->cmd.SendNwPlayClose(this->nwModeInfo.ctrlID);
	}
	if( this->ctrlDlg != NULL ){
		this->ctrlDlg->CloseStreamCtrlDialog();
		SAFE_DELETE(this->ctrlDlg);
	}
}

// プラグインの情報を返す
bool CEpgTimerPlugIn::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"EpgTimer PlugIn";
	pInfo->pszCopyright   = L"りょうちん Copyright (C) 2010";
	pInfo->pszDescription = L"EpgTimerSrvからの制御用";
	return true;
}


// 初期化処理
bool CEpgTimerPlugIn::Initialize()
{
	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	if( this->ctrlDlg == NULL ){
		this->ctrlDlg = new CStreamCtrlDlg;
		this->ctrlDlg->CreateStreamCtrlDialog(g_hinstDLL, this->m_pApp->GetAppWindow());
	}

	return true;
}

void CEpgTimerPlugIn::EnablePlugin(BOOL enable)
{
	this->showNormal = TRUE;

	if( enable == TRUE ){
		OutputDebugString(L"EnablePlugin");
		if(this->m_pApp->SetWindowMessageCallback(WindowMsgeCallback, this)==false){
			OutputDebugString(L"●TVTest Version Err::SetWindowMessageCallback");
		}

		wstring pipeName = L"";
		wstring eventName = L"";

		Format(pipeName, L"%s%d", CMD2_TVTEST_CTRL_PIPE, GetCurrentProcessId());
		Format(eventName, L"%s%d", CMD2_TVTEST_CTRL_WAIT_CONNECT, GetCurrentProcessId());

		OutputDebugString(pipeName.c_str());
		OutputDebugString(eventName.c_str());
		this->pipeServer.StartServer(eventName.c_str(), pipeName.c_str(), CtrlCmdCallback, this, 0, GetCurrentProcessId());

		if( this->nwMode == TRUE ){
			this->ctrlDlg->SetCtrlCmd(&this->cmd, this->nwModeInfo.ctrlID, this->nwModeInfo.udpSend, this->nwModeInfo.tcpSend, FALSE, this->nwModeInfo.timeShiftMode);
		}
		if( this->m_pApp->GetFullscreen() == true ){
			this->fullScreen = TRUE;
			if( this->ctrlDlg != NULL ){
				this->ctrlDlg->StartFullScreenMouseChk();
			}
		}else{
			this->fullScreen = FALSE;
		}
		this->ResetStreamingCtrlView();
	}else{
		this->m_pApp->SetWindowMessageCallback(NULL, NULL);
		this->pipeServer.StopServer();

		if( this->ctrlDlg != NULL ){
			this->ctrlDlg->ShowCtrlDlg(SW_HIDE);
			this->ctrlDlg->StopTimer();
		}
	}
	return ;
}

// 終了処理
bool CEpgTimerPlugIn::Finalize()
{
	this->pipeServer.StopServer();

	if( this->ctrlDlg != NULL ){
		this->ctrlDlg->CloseStreamCtrlDialog();
		SAFE_DELETE(this->ctrlDlg);
	}
	return true;
}

// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CEpgTimerPlugIn::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CEpgTimerPlugIn *pThis=static_cast<CEpgTimerPlugIn*>(pClientData);
	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		if (lParam1!=0) {
			pThis->EnablePlugin(TRUE);
			return TRUE;
		}else{
			pThis->EnablePlugin(FALSE);
			return TRUE;
		}
		break;
	case TVTest::EVENT_FULLSCREENCHANGE:
		if (lParam1!=0) {
			pThis->fullScreen = TRUE;
			if( pThis->ctrlDlg != NULL ){
				pThis->ctrlDlg->StartFullScreenMouseChk();
			}
		}else{
			pThis->fullScreen = FALSE;
			if( pThis->ctrlDlg != NULL ){
				pThis->ctrlDlg->StopFullScreenMouseChk();
				pThis->ResetStreamingCtrlView();
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

int CALLBACK CEpgTimerPlugIn::CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam)
{
	CEpgTimerPlugIn* sys = (CEpgTimerPlugIn*)param;

	resParam->dataSize = 0;
	resParam->param = CMD_ERR;

	switch( cmdParam->param ){
	case CMD2_VIEW_APP_SET_BONDRIVER:
		OutputDebugString(L"TvTest:CMD2_VIEW_APP_SET_BONDRIVER");
		{
			wstring val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( sys->m_pApp->SetDriverName(val.c_str()) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_GET_BONDRIVER:
		OutputDebugString(L"TvTest:CMD2_VIEW_APP_GET_BONDRIVER");
		{
			WCHAR buff[512] = L"";
			sys->m_pApp->GetDriverFullPathName(buff, 512);
			wstring bonName;
			GetFileName(buff, bonName );
			if( bonName.size() > 0 ){
				resParam->dataSize = GetVALUESize(bonName);
				resParam->data = new BYTE[resParam->dataSize];
				if( WriteVALUE(bonName, resParam->data, resParam->dataSize, NULL) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CH:
		OutputDebugString(L"TvTest:CMD2_VIEW_APP_SET_CH");
		{
			SET_CH_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( val.useSID == TRUE && val.useBonCh == TRUE ){
					int space = 0;
					int ch = 0;
					TVTest::ChannelInfo chInfo;
					while(1){
						if( sys->m_pApp->GetChannelInfo(space, ch, &chInfo) == false ){
							if( ch == 0 ){
								break;
							}else{
								space++;
								ch = 0;
							}
						}else{
							if( chInfo.Space == val.space &&
								chInfo.Channel == val.ch )
							{
								if( sys->m_pApp->SetChannel(space, ch, val.SID) == true ){
									resParam->param = CMD_SUCCESS;
								}
								break;
							}
							ch++;
						}
					}
				}
			}
			if( sys->nwMode == TRUE ){
				sys->cmd.SendNwPlayClose(sys->nwModeInfo.ctrlID);
				sys->nwMode = FALSE;
				sys->ResetStreamingCtrlView();
			}
		}
		break;
	case CMD2_VIEW_APP_CLOSE:
		OutputDebugString(L"TvTest:CMD2_VIEW_APP_CLOSE");
		{
			if( sys->nwMode == TRUE ){
				sys->cmd.SendNwPlayClose(sys->nwModeInfo.ctrlID);
				sys->nwMode = FALSE;
			}
			sys->m_pApp->Close(1);
		}
		break;
	case CMD2_VIEW_APP_TT_SET_CTRL:
		OutputDebugString(L"TvTest:CMD2_VIEW_APP_TT_SET_CTRL");
		{
			if( sys->nwMode == TRUE ){
				sys->cmd.SendNwPlayClose(sys->nwModeInfo.ctrlID);
				if( sys->ctrlDlg != NULL ){
					sys->ctrlDlg->StopTimer();
				}
			}
			if( ReadVALUE(&sys->nwModeInfo, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				resParam->param = CMD_SUCCESS;

				if( sys->nwModeInfo.enableMode == TRUE ){
					sys->nwMode = TRUE;

					wstring ip = L"";
					Format(ip, L"%d.%d.%d.%d",
						(sys->nwModeInfo.serverIP&0xFF000000)>>24,
						(sys->nwModeInfo.serverIP&0x00FF0000)>>16,
						(sys->nwModeInfo.serverIP&0x0000FF00)>>8,
						(sys->nwModeInfo.serverIP&0x000000FF));

					sys->cmd.SetSendMode(TRUE);
					sys->cmd.SetNWSetting(ip,sys->nwModeInfo.serverPort);
					sys->cmd.SetConnectTimeOut(15*1000);

					if( sys->ctrlDlg != NULL ){
						sys->ctrlDlg->SetCtrlCmd(&sys->cmd, sys->nwModeInfo.ctrlID, sys->nwModeInfo.udpSend, sys->nwModeInfo.tcpSend, TRUE, sys->nwModeInfo.timeShiftMode);
					}
					sys->ResetStreamingCtrlView();
				}else{
					sys->nwMode = FALSE;
					if( sys->ctrlDlg != NULL ){
						sys->ctrlDlg->ShowCtrlDlg(SW_HIDE);
					}
				}
			}
		}
		break;
	default:
		_OutputDebugString(L"TvTest:err default cmd %d\r\n", cmdParam->param);
		resParam->param = CMD_NON_SUPPORT;
		break;
	}

	return 0;
}

void CEpgTimerPlugIn::ResetStreamingCtrlView()
{
	if( this->ctrlDlg == NULL ){
		return;
	}
	if( this->nwMode == FALSE ){
		this->ctrlDlg->ShowCtrlDlg(SW_HIDE);
		return ;
	}
	WINDOWPLACEMENT info;
	info.length = sizeof(WINDOWPLACEMENT);

	if( GetWindowPlacement(this->m_pApp->GetAppWindow(), &info) == FALSE ){
		_OutputDebugString(L"GetWindowPlacement err");
		return;
	}
	if( this->fullScreen == FALSE ){
		if( info.showCmd == SW_SHOWNORMAL ){
			RECT rc;
			GetWindowRect(this->m_pApp->GetAppWindow(), &rc);

			int x = rc.left;
			int y = rc.bottom+3;
			int cx = rc.right - rc.left;
			int cy = 65;

			this->ctrlDlg->ShowCtrlDlg(SW_SHOW);
			SetWindowPos(this->ctrlDlg->GetDlgHWND(), this->m_pApp->GetAppWindow(), x, y, cx, cy, SWP_SHOWWINDOW);
			this->showNormal = TRUE;
		}else if( info.showCmd == SW_SHOWMAXIMIZED ){
			RECT rc;
			GetWindowRect(this->m_pApp->GetAppWindow(), &rc);

			POINT pos;
			GetCursorPos(&pos);

			if( pos.y > rc.bottom - 65 ){
				int x = rc.left;
				int y = rc.bottom-65;
				int cx = rc.right - rc.left;
				int cy = 65;

				this->ctrlDlg->ShowCtrlDlg(SW_SHOW);
				SetWindowPos(this->ctrlDlg->GetDlgHWND(), HWND_TOPMOST, x, y, cx, cy, SWP_SHOWWINDOW);
			}else{
				this->ctrlDlg->ShowCtrlDlg(SW_HIDE);
			}
			this->showNormal = FALSE;
		}
	}
}

BOOL CALLBACK CEpgTimerPlugIn::WindowMsgeCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult,void *pUserData)
{
	CEpgTimerPlugIn* sys = (CEpgTimerPlugIn*)pUserData;
	if( sys->nwMode == TRUE ){
		switch(uMsg){
		case WM_SIZE:
			sys->ResetStreamingCtrlView();
			break;
		case WM_MOVE:
			sys->ResetStreamingCtrlView();
			break;
		case WM_MOUSEMOVE:
			if( sys->showNormal == FALSE ){
				sys->ResetStreamingCtrlView();
			}
			break;
		case WM_CHG_PORT:
			{
				WCHAR buff[512] = L"";
				sys->m_pApp->GetDriverFullPathName(buff, 512);
				wstring bonName;
				GetFileName(buff, bonName );
				if( lParam != 0){
					if( CompareNoCase(bonName, L"BonDriver_TCP.dll") != 0 ){
						sys->m_pApp->SetDriverName(L"BonDriver_TCP.dll");
					}
					DWORD ch = (DWORD)lParam-2230;
					sys->m_pApp->SetChannel(0, ch);
				}else
				if( wParam != 0){
					if( CompareNoCase(bonName, L"BonDriver_UDP.dll") != 0 ){
						sys->m_pApp->SetDriverName(L"BonDriver_UDP.dll");
					}
					DWORD ch = (DWORD)wParam-1234;
					sys->m_pApp->SetChannel(0, ch);
				}
			}
			break;
		default:
			break;
		}
	}
	return FALSE;
}

