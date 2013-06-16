﻿#include "StdAfx.h"
#include "ReserveManager.h"
#include <process.h>
#include "../../Common/CtrlCmdDef.h"
#include "../../Common/SendCtrlCmd.h"
#include "CheckRecFile.h"

#include <tlhelp32.h> 
#include <shlwapi.h>
#include <algorithm>
#include <locale>

CReserveManager::CReserveManager(void)
{
	this->lockEvent = _CreateEvent(FALSE, TRUE, NULL);
	this->lockNotify = _CreateEvent(FALSE, TRUE, NULL);

	this->bankCheckThread = NULL;
	this->bankCheckStopEvent = _CreateEvent(FALSE, FALSE, NULL);

	this->notifyThread = NULL;
	this->notifyStopEvent = _CreateEvent(FALSE, FALSE, NULL);
	this->notifyStatusThread = NULL;
	this->notifyStatusStopEvent = _CreateEvent(FALSE, FALSE, NULL);
	this->notifyEpgReloadThread = NULL;
	this->notifyEpgReloadStopEvent = _CreateEvent(FALSE, FALSE, NULL);


	this->tunerManager.ReloadTuner();
	vector<DWORD> idList;
	this->tunerManager.GetEnumID( &idList );
	for( size_t i=0; i<idList.size(); i++){
		BANK_INFO* item = new BANK_INFO;
		item->tunerID = idList[i];
		this->bankMap.insert(pair<DWORD, BANK_INFO*>(item->tunerID, item));
	}

	this->tunerManager.GetEnumTunerBank(&this->tunerBankMap);

	this->backPriorityFlag = FALSE;
	this->sameChPriorityFlag = FALSE;

	this->defStartMargine = 5;
	this->defEndMargine = 2;

	this->enableSetSuspendMode = 0xFF;
	this->enableSetRebootFlag = 0xFF;

	this->epgCapCheckFlag = FALSE;

	this->autoDel = FALSE;

	this->eventRelay = FALSE;

	this->duraChgMarginMin = 0;
	this->notFindTuijyuHour = 6;
	this->noEpgTuijyuMin = 30;

	this->autoDelRecInfo = FALSE;
	this->autoDelRecInfoNum = 100;
	this->timeSync = FALSE;
	this->setTimeSync = FALSE;

	this->NWTVPID = 0;
	this->NWTVUDP = FALSE;
	this->NWTVTCP = FALSE;

	this->useTweet = FALSE;
	this->useProxy = FALSE;

	this->reloadBankMapAlgo = 0;
	ReloadSetting();
}


CReserveManager::~CReserveManager(void)
{
	if( this->bankCheckThread != NULL ){
		::SetEvent(this->bankCheckStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->bankCheckThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->bankCheckThread, 0xffffffff);
		}
		CloseHandle(this->bankCheckThread);
		this->bankCheckThread = NULL;
	}
	if( this->bankCheckStopEvent != NULL ){
		CloseHandle(this->bankCheckStopEvent);
		this->bankCheckStopEvent = NULL;
	}

	if( this->notifyEpgReloadThread != NULL ){
		::SetEvent(this->notifyEpgReloadStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->notifyEpgReloadThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->notifyEpgReloadThread, 0xffffffff);
		}
		CloseHandle(this->notifyEpgReloadThread);
		this->notifyEpgReloadThread = NULL;
	}
	if( this->notifyEpgReloadStopEvent != NULL ){
		CloseHandle(this->notifyEpgReloadStopEvent);
		this->notifyEpgReloadStopEvent = NULL;
	}

	if( this->notifyStatusThread != NULL ){
		::SetEvent(this->notifyStatusStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->notifyStatusThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->notifyStatusThread, 0xffffffff);
		}
		CloseHandle(this->notifyStatusThread);
		this->notifyStatusThread = NULL;
	}
	if( this->notifyStatusStopEvent != NULL ){
		CloseHandle(this->notifyStatusStopEvent);
		this->notifyStatusStopEvent = NULL;
	}

	if( this->notifyThread != NULL ){
		::SetEvent(this->notifyStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->notifyThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->notifyThread, 0xffffffff);
		}
		CloseHandle(this->notifyThread);
		this->notifyThread = NULL;
	}
	if( this->notifyStopEvent != NULL ){
		CloseHandle(this->notifyStopEvent);
		this->notifyStopEvent = NULL;
	}

	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		SAFE_DELETE(itrCtrl->second);
	}

	map<DWORD, BANK_INFO*>::iterator itrBank;
	for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
		map<DWORD, BANK_WORK_INFO*>::iterator itrWork;
		for( itrWork = itrBank->second->reserveList.begin(); itrWork != itrBank->second->reserveList.end(); itrWork++ ){
			SAFE_DELETE(itrWork->second);
		}
		SAFE_DELETE(itrBank->second);
	}

	map<DWORD, BANK_WORK_INFO*>::iterator itrNG;
	for( itrNG = this->NGReserveMap.begin(); itrNG != this->NGReserveMap.end(); itrNG++){
		SAFE_DELETE(itrNG->second);
	}

	map<DWORD, CReserveInfo*>::iterator itr;
	for( itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}

	if( this->lockNotify != NULL ){
		NotifyUnLock();
		CloseHandle(this->lockNotify);
		this->lockNotify = NULL;
	}

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}

}

BOOL CReserveManager::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	//if( log != NULL ){
	//	_OutputDebugString(L"◆%s",log);
	//}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			OutputDebugString(L"◆CReserveManager::Lock FALSE");
			if( log != NULL ){
				OutputDebugString(log);
			}
		return FALSE;
	}
	return TRUE;
}

void CReserveManager::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

BOOL CReserveManager::NotifyLock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockNotify == NULL ){
		return FALSE;
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
	DWORD dwRet = WaitForSingleObject(this->lockNotify, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			OutputDebugString(L"◆CReserveManager::NotifyLock FALSE");
		return FALSE;
	}
	return TRUE;
}

void CReserveManager::NotifyUnLock(LPCWSTR log)
{
	if( this->lockNotify != NULL ){
		SetEvent(this->lockNotify);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

void CReserveManager::SetRegistGUI(map<DWORD, DWORD> registGUIMap)
{
	if( Lock(L"SetRegistGUI") == FALSE ) return;

	if( this->notifyThread != NULL ){
		::SetEvent(this->notifyStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->notifyThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->notifyThread, 0xffffffff);
		}
		CloseHandle(this->notifyThread);
		this->notifyThread = NULL;
	}

	this->registGUIMap = registGUIMap;

	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		itrCtrl->second->SetRegistGUI(registGUIMap);
	}

	this->batManager.SetRegistGUI(registGUIMap);

	SendNotifyStatus(this->notifyStatus);

	UnLock();
}

void CReserveManager::SetRegistTCP(map<wstring, REGIST_TCP_INFO> registTCPMap)
{
	if( Lock(L"SetRegistTCP") == FALSE ) return;

	if( this->notifyThread != NULL ){
		::SetEvent(this->notifyStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->notifyThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->notifyThread, 0xffffffff);
		}
		CloseHandle(this->notifyThread);
		this->notifyThread = NULL;
	}

	this->registTCPMap = registTCPMap;

	SendNotifyStatus(this->notifyStatus);

	UnLock();
}

void CReserveManager::ReloadSetting()
{
	if( Lock(L"ReloadSetting") == FALSE ) return;

	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);

	wstring iniCommonPath = L"";
	GetCommonIniPath(iniCommonPath);

	this->BSOnly = GetPrivateProfileInt(L"SET", L"BSBasicOnly", 1, iniCommonPath.c_str());
	this->CS1Only = GetPrivateProfileInt(L"SET", L"CS1BasicOnly", 1, iniCommonPath.c_str());
	this->CS2Only = GetPrivateProfileInt(L"SET", L"CS2BasicOnly", 1, iniCommonPath.c_str());

	this->ngCapMin = GetPrivateProfileInt(L"SET", L"NGEpgCapTime", 20, iniAppPath.c_str());
	this->ngCapTunerMin = GetPrivateProfileInt(L"SET", L"NGEpgCapTunerTime", 20, iniAppPath.c_str());

	this->epgCapTimeList.clear();
	int count = GetPrivateProfileInt(L"EPG_CAP", L"Count", 0, iniAppPath.c_str());
	for( int i=0; i<count; i++ ){
		wstring selectKey;
		Format(selectKey, L"%dSelect", i);
		if( GetPrivateProfileInt(L"EPG_CAP", selectKey.c_str(), 0, iniAppPath.c_str()) == 1 ){
			wstring timeKey;
			Format(timeKey, L"%d", i);
			WCHAR buff[256] = L"";
			GetPrivateProfileString(L"EPG_CAP", timeKey.c_str(), L"", buff, 256, iniAppPath.c_str());
			wstring time = buff;
			if( time.size() > 0 ){
				wstring left = L"";
				wstring right = L"";
				Separate(time, L":", left, right);

				DWORD second = _wtoi(left.c_str()) * 60 * 60 + _wtoi(right.c_str()) * 60;
				this->epgCapTimeList.push_back(second);
			}
		}
	}

	this->wakeTime = GetPrivateProfileInt(L"SET", L"WakeTime", 5, iniAppPath.c_str());

	this->defSuspendMode = GetPrivateProfileInt(L"SET", L"RecEndMode", 0, iniAppPath.c_str());
	this->defRebootFlag = GetPrivateProfileInt(L"SET", L"Reboot", 0, iniAppPath.c_str());

	this->batMargin = GetPrivateProfileInt(L"SET", L"BatMargin", 10, iniAppPath.c_str());
	
	this->noStandbyExeList.clear();
	count = GetPrivateProfileInt(L"NO_SUSPEND", L"Count", 0, iniAppPath.c_str());
	if( count == 0 ){
		this->noStandbyExeList.push_back(L"EpgDataCap_Bon.exe");
	}else{
		for( int i=0; i<count; i++ ){
			wstring key;
			Format(key, L"%d", i);
			WCHAR buff[256] = L"";
			GetPrivateProfileString(L"NO_SUSPEND", key.c_str(), L"", buff, 256, iniAppPath.c_str());
			wstring exe = buff;
			if( exe.size() > 0 ){
				std::transform(exe.begin(), exe.end(), exe.begin(), tolower);
				this->noStandbyExeList.push_back(exe);
			}
		}
	}

	this->noStandbyTime = GetPrivateProfileInt(L"NO_SUSPEND", L"NoStandbyTime", 10, iniAppPath.c_str());
	this->autoDel = (BOOL)GetPrivateProfileInt(L"SET", L"AutoDel", 0, iniAppPath.c_str());

	this->delExtList.clear();
	count = GetPrivateProfileInt(L"DEL_EXT", L"Count", 0, iniAppPath.c_str());
	for( int i=0; i<count; i++ ){
		wstring key;
		Format(key, L"%d", i);
		WCHAR buff[512] = L"";
		GetPrivateProfileString(L"DEL_EXT", key.c_str(), L"", buff, 512, iniAppPath.c_str());
		wstring ext = buff;
		this->delExtList.push_back(ext);
	}
	this->delFolderList.clear();
	count = GetPrivateProfileInt(L"DEL_CHK", L"Count", 0, iniAppPath.c_str());
	for( int i=0; i<count; i++ ){
		wstring key;
		Format(key, L"%d", i);
		WCHAR buff[512] = L"";
		GetPrivateProfileString(L"DEL_CHK", key.c_str(), L"", buff, 512, iniAppPath.c_str());
		wstring folder = buff;
		this->delFolderList.push_back(folder);
	}

	this->backPriorityFlag = (BOOL)GetPrivateProfileInt(L"SET", L"BackPriority", 1, iniAppPath.c_str());
	this->sameChPriorityFlag = (BOOL)GetPrivateProfileInt(L"SET", L"SameChPriority", 0, iniAppPath.c_str());

	this->eventRelay = (BOOL)GetPrivateProfileInt(L"SET", L"EventRelay", 0, iniAppPath.c_str());

	wstring ch5Path = L"";
	GetSettingPath(ch5Path);
	ch5Path += L"\\ChSet5.txt";

	chUtil.ParseText(ch5Path.c_str());

	this->tvtestUseBon.clear();
	count = GetPrivateProfileInt(L"TVTEST", L"Num", 0, iniAppPath.c_str());
	for( int i=0; i<count; i++ ){
		wstring key;
		Format(key, L"%d", i);
		WCHAR buff[256] = L"";
		GetPrivateProfileString(L"TVTEST", key.c_str(), L"", buff, 256, iniAppPath.c_str());
		wstring bon = buff;
		if( bon.size() > 0 ){
			this->tvtestUseBon.push_back(bon);
		}
	}

	this->defStartMargine = GetPrivateProfileInt(L"SET", L"StartMargin", 5, iniAppPath.c_str());
	this->defEndMargine = GetPrivateProfileInt(L"SET", L"EndMargin", 2, iniAppPath.c_str());
	this->notFindTuijyuHour = GetPrivateProfileInt(L"SET", L"TuijyuHour", 6, iniAppPath.c_str());
	this->duraChgMarginMin = GetPrivateProfileInt(L"SET", L"DuraChgMarginMin", 0, iniAppPath.c_str());
	this->noEpgTuijyuMin = GetPrivateProfileInt(L"SET", L"NoEpgTuijyuMin", 30, iniAppPath.c_str());

	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		itrCtrl->second->ReloadSetting();
		itrCtrl->second->SetAutoDel(this->autoDel, &this->delExtList, &this->delFolderList); 
	}

	WCHAR buff[512] = L"";
	GetPrivateProfileString( L"SET", L"RecExePath", L"", buff, 512, iniCommonPath.c_str() );
	this->recExePath = buff;
	if( this->recExePath.size() == 0 ){
		GetModuleFolderPath(this->recExePath);
		this->recExePath += L"\\EpgDataCap_Bon.exe";
	}

	this->autoDelRecInfo = GetPrivateProfileInt(L"SET", L"AutoDelRecInfo", 0, iniAppPath.c_str());
	this->autoDelRecInfoNum = GetPrivateProfileInt(L"SET", L"AutoDelRecInfoNum", 100, iniAppPath.c_str());

	this->timeSync = GetPrivateProfileInt(L"SET", L"TimeSync", 0, iniAppPath.c_str());

	recInfoText.SetAutoDel(this->autoDelRecInfoNum, this->autoDelRecInfo);

	this->useTweet = GetPrivateProfileInt(L"TWITTER", L"use", 0, iniAppPath.c_str());
	this->useProxy = GetPrivateProfileInt(L"TWITTER", L"useProxy", 0, iniAppPath.c_str());
	ZeroMemory(buff, sizeof(WCHAR)*512);
	GetPrivateProfileStringW(L"TWITTER", L"ProxyServer", L"", buff, 512, iniAppPath.c_str());
	this->proxySrv = buff;
	ZeroMemory(buff, sizeof(WCHAR)*512);
	GetPrivateProfileStringW(L"TWITTER", L"ProxyID", L"", buff, 512, iniAppPath.c_str());
	this->proxyID = buff;
	ZeroMemory(buff, sizeof(WCHAR)*512);
	GetPrivateProfileStringW(L"TWITTER", L"ProxyPWD", L"", buff, 512, iniAppPath.c_str());
	this->proxyPWD = buff;

	USE_PROXY_INFO proxyInfo;
	if( this->useProxy == TRUE ){
		if( this->proxySrv.size() > 0 ){
			proxyInfo.serverName = new WCHAR[this->proxySrv.size()+1];
			wcscpy_s(proxyInfo.serverName, this->proxySrv.size()+1, this->proxySrv.c_str());
		}
		if( this->proxyID.size() > 0 ){
			proxyInfo.userName = new WCHAR[this->proxyID.size()+1];
			wcscpy_s(proxyInfo.userName, this->proxyID.size()+1, this->proxyID.c_str());
		}
		if( this->proxyPWD.size() > 0 ){
			proxyInfo.serverName = new WCHAR[this->proxyPWD.size()+1];
			wcscpy_s(proxyInfo.password, this->proxyPWD.size()+1, this->proxyPWD.c_str());
		}
	}
	this->twitterManager.SetProxy(this->useProxy, &proxyInfo);

	map<DWORD, CTunerBankCtrl*>::iterator itr;
	for(itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		itr->second->SetTwitterCtrl(&this->twitterManager);
	}

	this->reloadBankMapAlgo = GetPrivateProfileInt(L"SET", L"ReloadBankMapAlgo", 0, iniAppPath.c_str());

	UnLock();
}

void CReserveManager::SendNotifyUpdate()
{
	if( Lock(L"SendNotifyUpdate") == FALSE ) return;

	_SendNotifyUpdate();

	UnLock();
}

void CReserveManager::_SendNotifyUpdate()
{
	if( this->notifyThread != NULL ){
		if( ::WaitForSingleObject(this->notifyThread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(this->notifyThread);
			this->notifyThread = NULL;
		}
	}
	if( this->notifyThread == NULL ){
		ResetEvent(this->notifyStopEvent);
		this->notifyThread = (HANDLE)_beginthreadex(NULL, 0, SendNotifyThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->notifyThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->notifyThread);
	}
}


UINT WINAPI CReserveManager::SendNotifyThread(LPVOID param)
{
	CReserveManager* sys = (CReserveManager*)param;
	CSendCtrlCmd sendCtrl;
	map<DWORD,DWORD>::iterator itr;

	if( sys->NotifyLock() == FALSE ) return 0;

	vector<DWORD> errID;
	for( itr = sys->registGUIMap.begin(); itr != sys->registGUIMap.end(); itr++){
		if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		if( _FindOpenExeProcess(itr->first) == TRUE ){
			wstring pipe;
			wstring waitEvent;
			Format(pipe, L"%s%d", CMD2_GUI_CTRL_PIPE, itr->first);
			Format(waitEvent, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, itr->first);

			sendCtrl.SetPipeSetting(waitEvent, pipe);
			sendCtrl.SetConnectTimeOut(5*1000);
			if( sendCtrl.SendGUIUpdateReserve() != CMD_SUCCESS ){
				errID.push_back(itr->first);
			}
		}else{
			errID.push_back(itr->first);
		}
	}
	for( size_t i=0; i<errID.size(); i++ ){
		itr = sys->registGUIMap.find(errID[i]);
		if( itr != sys->registGUIMap.end() ){
			sys->registGUIMap.erase(itr);
		}
	}

	map<wstring, REGIST_TCP_INFO>::iterator itrTCP;
	vector<wstring> errIP;
	for( itrTCP = sys->registTCPMap.begin(); itrTCP != sys->registTCPMap.end(); itrTCP++){
		if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		sendCtrl.SetSendMode(TRUE);
		sendCtrl.SetNWSetting(itrTCP->second.ip, itrTCP->second.port);
		sendCtrl.SetConnectTimeOut(5*1000);
		if( sendCtrl.SendGUIUpdateReserve() != CMD_SUCCESS ){
			errIP.push_back(itrTCP->first);
		}
	}
	for( size_t i=0; i<errIP.size(); i++ ){
		itrTCP = sys->registTCPMap.find(errIP[i]);
		if( itrTCP != sys->registTCPMap.end() ){
			_OutputDebugString(L"notifyErr %s:%d", itrTCP->second.ip.c_str(), itrTCP->second.port);
			sys->registTCPMap.erase(itrTCP);
		}
	}

	sys->NotifyUnLock();

	return 0;
}

void CReserveManager::SendNotifyEpgReload()
{
	if( Lock(L"SendNotifyEpgReload") == FALSE ) return;

	_SendNotifyEpgReload();

	UnLock();
}

void CReserveManager::_SendNotifyEpgReload()
{
	if( this->notifyEpgReloadThread != NULL ){
		if( ::WaitForSingleObject(this->notifyEpgReloadThread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(this->notifyEpgReloadThread);
			this->notifyEpgReloadThread = NULL;
		}
	}
	if( this->notifyEpgReloadThread == NULL ){
		ResetEvent(this->notifyEpgReloadStopEvent);
		this->notifyEpgReloadThread = (HANDLE)_beginthreadex(NULL, 0, SendNotifyEpgReloadThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->notifyEpgReloadThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->notifyEpgReloadThread);
	}
}


UINT WINAPI CReserveManager::SendNotifyEpgReloadThread(LPVOID param)
{
	CReserveManager* sys = (CReserveManager*)param;
	CSendCtrlCmd sendCtrl;
	map<DWORD,DWORD>::iterator itr;

	if( sys->NotifyLock() == FALSE ) return 0;

	vector<DWORD> errID;
	for( itr = sys->registGUIMap.begin(); itr != sys->registGUIMap.end(); itr++){
		if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		if( _FindOpenExeProcess(itr->first) == TRUE ){
			wstring pipe;
			wstring waitEvent;
			Format(pipe, L"%s%d", CMD2_GUI_CTRL_PIPE, itr->first);
			Format(waitEvent, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, itr->first);

			sendCtrl.SetPipeSetting(waitEvent, pipe);
			sendCtrl.SetConnectTimeOut(5*1000);
			if( sendCtrl.SendGUIUpdateEpgData() != CMD_SUCCESS ){
				errID.push_back(itr->first);
			}
		}else{
			errID.push_back(itr->first);
		}
	}
	for( size_t i=0; i<errID.size(); i++ ){
		itr = sys->registGUIMap.find(errID[i]);
		if( itr != sys->registGUIMap.end() ){
			sys->registGUIMap.erase(itr);
		}
	}

	map<wstring, REGIST_TCP_INFO>::iterator itrTCP;
	vector<wstring> errIP;
	for( itrTCP = sys->registTCPMap.begin(); itrTCP != sys->registTCPMap.end(); itrTCP++){
		if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		sendCtrl.SetSendMode(TRUE);
		sendCtrl.SetNWSetting(itrTCP->second.ip, itrTCP->second.port);
		sendCtrl.SetConnectTimeOut(5*1000);
		if( sendCtrl.SendGUIUpdateEpgData() != CMD_SUCCESS ){
			errIP.push_back(itrTCP->first);
		}
	}
	for( size_t i=0; i<errIP.size(); i++ ){
		itrTCP = sys->registTCPMap.find(errIP[i]);
		if( itrTCP != sys->registTCPMap.end() ){
			_OutputDebugString(L"notifyErr %s:%d", itrTCP->second.ip.c_str(), itrTCP->second.port);
			sys->registTCPMap.erase(itrTCP);
		}
	}

	sys->NotifyUnLock();

	return 0;
}

void CReserveManager::SendNotifyStatus(WORD status)
{
	if( this->notifyStatusThread != NULL ){
		if( ::WaitForSingleObject(this->notifyStatusThread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(this->notifyStatusThread);
			this->notifyStatusThread = NULL;
		}
	}
	if( this->notifyStatusThread == NULL ){
		this->notifyStatus = status;
		ResetEvent(this->notifyStatusStopEvent);
		this->notifyStatusThread = (HANDLE)_beginthreadex(NULL, 0, SendNotifyStatusThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->notifyStatusThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->notifyStatusThread);
	}
}

UINT WINAPI CReserveManager::SendNotifyStatusThread(LPVOID param)
{
	CReserveManager* sys = (CReserveManager*)param;
	CSendCtrlCmd sendCtrl;
	map<DWORD,DWORD>::iterator itr;

	if( sys->NotifyLock() == FALSE ) return 0;

	vector<DWORD> errID;
	for( itr = sys->registGUIMap.begin(); itr != sys->registGUIMap.end(); itr++){
		if( ::WaitForSingleObject(sys->notifyStatusStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		if( _FindOpenExeProcess(itr->first) == TRUE ){
			wstring pipe;
			wstring waitEvent;
			Format(pipe, L"%s%d", CMD2_GUI_CTRL_PIPE, itr->first);
			Format(waitEvent, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, itr->first);

			sendCtrl.SetPipeSetting(waitEvent, pipe);
			sendCtrl.SetConnectTimeOut(5*1000);
			if( sendCtrl.SendGUIStatusChg(sys->notifyStatus) != CMD_SUCCESS ){
				errID.push_back(itr->first);
			}
		}else{
			errID.push_back(itr->first);
		}
	}
	for( size_t i=0; i<errID.size(); i++ ){
		itr = sys->registGUIMap.find(errID[i]);
		if( itr != sys->registGUIMap.end() ){
			sys->registGUIMap.erase(itr);
		}
	}
	
	map<wstring, REGIST_TCP_INFO>::iterator itrTCP;
	vector<wstring> errIP;
	for( itrTCP = sys->registTCPMap.begin(); itrTCP != sys->registTCPMap.end(); itrTCP++){
		if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		sendCtrl.SetSendMode(TRUE);
		sendCtrl.SetNWSetting(itrTCP->second.ip, itrTCP->second.port);
		sendCtrl.SetConnectTimeOut(5*1000);
		if( sendCtrl.SendGUIStatusChg(sys->notifyStatus) != CMD_SUCCESS ){
			errIP.push_back(itrTCP->first);
		}
	}
	for( size_t i=0; i<errIP.size(); i++ ){
		itrTCP = sys->registTCPMap.find(errIP[i]);
		if( itrTCP != sys->registTCPMap.end() ){
			_OutputDebugString(L"notifyErr %s:%d", itrTCP->second.ip.c_str(), itrTCP->second.port);
			sys->registTCPMap.erase(itrTCP);
		}
	}

	sys->NotifyUnLock();

	return 0;
}

BOOL CReserveManager::ReloadReserveData()
{
	if( Lock(L"ReloadReserveData") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	wstring reserveFilePath = L"";
	GetSettingPath(reserveFilePath);
	reserveFilePath += L"\\";
	reserveFilePath += RESERVE_TEXT_NAME;

	wstring recInfoFilePath = L"";
	GetSettingPath(recInfoFilePath);
	recInfoFilePath += L"\\";
	recInfoFilePath += REC_INFO_TEXT_NAME;


	map<DWORD, CReserveInfo*>::iterator itr;
	for( itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->reserveInfoMap.clear();
	this->reserveInfoIDMap.clear();

	this->recInfoText.ParseRecInfoText(recInfoFilePath.c_str());

	vector<DWORD> deleteList;
	LONGLONG nowTime = GetNowI64Time();

	ret = this->reserveText.ParseReserveText(reserveFilePath.c_str());
	if( ret == TRUE ){
		map<DWORD, RESERVE_DATA*>::iterator itrData;
		for( itrData = this->reserveText.reserveIDMap.begin(); itrData != this->reserveText.reserveIDMap.end(); itrData++){
			LONGLONG chkEndTime = GetSumTime(itrData->second->startTime, itrData->second->durationSecond);
			if( itrData->second->recSetting.useMargineFlag == 1){
				if( itrData->second->recSetting.endMargine < 0 ){
					chkEndTime += ((LONGLONG)itrData->second->recSetting.endMargine) * I64_1SEC;
				}
			}else{
				if( this->defEndMargine < 0 ){
					chkEndTime += ((LONGLONG)this->defEndMargine) * I64_1SEC;
				}
			}
			chkEndTime -= 60*I64_1SEC;

			if( nowTime < chkEndTime ){
				CReserveInfo* item = new CReserveInfo;
				item->SetData(itrData->second);

				//サービスサポートしてないチューナー検索
				vector<DWORD> idList;
				if( this->tunerManager.GetNotSupportServiceTuner(
					itrData->second->originalNetworkID,
					itrData->second->transportStreamID,
					itrData->second->serviceID,
					&idList ) == TRUE ){
						item->SetNGChTunerID(&idList);
				}
				
				this->reserveInfoMap.insert(pair<DWORD, CReserveInfo*>(itrData->second->reserveID, item));
				LONGLONG keyID = _Create64Key2(
					itrData->second->originalNetworkID,
					itrData->second->transportStreamID,
					itrData->second->serviceID,
					itrData->second->eventID);
				this->reserveInfoIDMap.insert(pair<LONGLONG, DWORD>(keyID, itrData->second->reserveID));

			}else{
				//時間過ぎているので失敗
				deleteList.push_back(itrData->second->reserveID);
				REC_FILE_INFO item;
				item = *itrData->second;
				if( itrData->second->recSetting.recMode != RECMODE_NO ){
					item.recStatus = REC_END_STATUS_START_ERR;
					item.comment = L"録画時間に起動していなかった可能性があります";
					this->recInfoText.AddRecInfo(&item);
				}
			}
		}
	}

	if( deleteList.size() > 0 ){
		for( size_t i=0; i<deleteList.size(); i++ ){
			this->reserveText.DelReserve(deleteList[i]);
		}
		this->reserveText.SaveReserveText();
		this->recInfoText.SaveRecInfoText();
	}

	if( this->bankCheckThread != NULL ){
		if( ::WaitForSingleObject(this->bankCheckThread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(this->bankCheckThread);
			this->bankCheckThread = NULL;
		}
	}
	if( this->bankCheckThread == NULL ){
		ResetEvent(this->bankCheckStopEvent);
		this->bankCheckThread = (HANDLE)_beginthreadex(NULL, 0, BankCheckThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->bankCheckThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->bankCheckThread);
	}

	UnLock();
	return ret;
}

BOOL CReserveManager::AddLoadReserveData()
{
	if( Lock(L"AddLoadReserveData") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	wstring filePath = L"";
	GetSettingPath(filePath);
	filePath += L"\\";
	filePath += RESERVE_TEXT_NAME;

	ret = this->reserveText.AddParseReserveText(filePath.c_str());
	if( ret == TRUE ){
		map<DWORD, RESERVE_DATA*>::iterator itrData;
		for( itrData = this->reserveText.reserveIDMap.begin(); itrData != this->reserveText.reserveIDMap.end(); itrData++){
			map<DWORD, CReserveInfo*>::iterator itrInfo;
			itrInfo = this->reserveInfoMap.find(itrData->second->reserveID);
			if( itrInfo == this->reserveInfoMap.end() ){
				CReserveInfo* item = new CReserveInfo;
				item->SetData(itrData->second);

				//サービスサポートしてないチューナー検索
				vector<DWORD> idList;
				if( this->tunerManager.GetNotSupportServiceTuner(
					itrData->second->originalNetworkID,
					itrData->second->transportStreamID,
					itrData->second->serviceID,
					&idList ) == TRUE ){
						item->SetNGChTunerID(&idList);
				}

				this->reserveInfoMap.insert(pair<DWORD, CReserveInfo*>(itrData->second->reserveID, item));
				LONGLONG keyID = _Create64Key2(
					itrData->second->originalNetworkID,
					itrData->second->transportStreamID,
					itrData->second->serviceID,
					itrData->second->eventID);
				this->reserveInfoIDMap.insert(pair<LONGLONG, DWORD>(keyID, itrData->second->reserveID));
			}
		}
	}

	_SendNotifyUpdate();

	UnLock();
	return ret;
}

//予約情報を取得する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// reserveList		[OUT]予約情報一覧（呼び出し元で解放する必要あり）
BOOL CReserveManager::GetReserveDataAll(
	vector<RESERVE_DATA*>* reserveList
	)
{
	if( Lock(L"GetReserveDataAll") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	map<DWORD, CReserveInfo*>::iterator itr;
	for( itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		RESERVE_DATA* item = new RESERVE_DATA;
		itr->second->GetData(item);
		reserveList->push_back(item);
	}

	UnLock();
	return ret;
}

//チューナー毎の予約情報を取得する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// reserveList		[OUT]予約情報一覧
BOOL CReserveManager::GetTunerReserveAll(
	vector<TUNER_RESERVE_INFO>* list
	)
{
	if( Lock(L"GetTunerReserveAll") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	map<DWORD, BANK_INFO*>::iterator itr;
	for( itr = this->bankMap.begin(); itr != this->bankMap.end(); itr++ ){
		TUNER_RESERVE_INFO item;
		item.tunerID = itr->second->tunerID;
		this->tunerManager.GetBonFileName(item.tunerID, item.tunerName);

		map<DWORD, BANK_WORK_INFO*>::iterator itrInfo;
		for( itrInfo = itr->second->reserveList.begin(); itrInfo != itr->second->reserveList.end(); itrInfo++){
			item.reserveList.push_back(itrInfo->second->reserveID);
		}
		list->push_back(item);
	}

	TUNER_RESERVE_INFO item;
	item.tunerID = 0xFFFFFFFF;
	item.tunerName = L"チューナー不足";
	map<DWORD, BANK_WORK_INFO*>::iterator itrNg;
	for( itrNg = this->NGReserveMap.begin(); itrNg != this->NGReserveMap.end(); itrNg++ ){
		item.reserveList.push_back(itrNg->second->reserveID);
	}
	list->push_back(item);

	UnLock();
	return ret;
}

//予約情報を取得する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id				[IN]予約ID
// reserveData		[OUT]予約情報
BOOL CReserveManager::GetReserveData(
	DWORD id,
	RESERVE_DATA* reserveData
	)
{
	if( Lock(L"GetReserveData") == FALSE ) return FALSE;
	BOOL ret = TRUE;


	map<DWORD, CReserveInfo*>::iterator itr;
	itr = this->reserveInfoMap.find(id);
	if( itr == this->reserveInfoMap.end() ){
		ret = FALSE;
	}else{
		itr->second->GetData(reserveData);
	}

	UnLock();
	return ret;
}

//予約情報を追加する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// reserveList		[IN]予約情報
BOOL CReserveManager::AddReserveData(
	vector<RESERVE_DATA>* reserveList,
	BOOL tweet
	)
{
	if( Lock(L"AddReserveData") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	//予約追加
	BOOL add = FALSE;
	for( size_t i=0; i<reserveList->size(); i++ ){
		if( _AddReserveData(&(*reserveList)[i], tweet) == TRUE ){
			add = TRUE;
		}
	}
	if( add == FALSE ){
		//追加成功したものがない
		UnLock();
		return FALSE;
	}

	wstring filePath = L"";
	GetSettingPath(filePath);
	filePath += L"\\";
	filePath += RESERVE_TEXT_NAME;

	this->reserveText.SaveReserveText(filePath.c_str());

	_ReloadBankMap();


	_SendNotifyUpdate();

	UnLock();
	return ret;
}

BOOL CReserveManager::_AddReserveData(RESERVE_DATA* reserve, BOOL tweet)
{
	BOOL ret = TRUE;

	DWORD reserveID = 0;
	if( this->reserveText.AddReserve(reserve, &reserveID) == FALSE ){
		return FALSE;
	}
	map<DWORD, RESERVE_DATA*>::iterator itrData;
	itrData = this->reserveText.reserveIDMap.find(reserveID);
	if( itrData != this->reserveText.reserveIDMap.end() ){
		if( this->reserveInfoMap.find(itrData->second->reserveID) == this->reserveInfoMap.end() ){
			CReserveInfo* item = new CReserveInfo;
			item->SetData(itrData->second);

			//サービスサポートしてないチューナー検索
			vector<DWORD> idList;
			if( this->tunerManager.GetNotSupportServiceTuner(
				itrData->second->originalNetworkID,
				itrData->second->transportStreamID,
				itrData->second->serviceID,
				&idList ) == TRUE ){
					item->SetNGChTunerID(&idList);
			}

			this->reserveInfoMap.insert(pair<DWORD, CReserveInfo*>(itrData->second->reserveID, item));
			LONGLONG keyID = _Create64Key2(
				itrData->second->originalNetworkID,
				itrData->second->transportStreamID,
				itrData->second->serviceID,
				itrData->second->eventID);
			this->reserveInfoIDMap.insert(pair<LONGLONG, DWORD>(keyID, itrData->second->reserveID));

			if( tweet == TRUE && itrData->second->recSetting.recMode != RECMODE_NO){
				_SendTweet(TW_ADD_RESERVE, itrData->second, NULL, NULL);
			}
		}
	}

	return ret;
}

//予約情報を変更する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// reserveList		[IN]予約情報
BOOL CReserveManager::ChgReserveData(
	vector<RESERVE_DATA>* reserveList
	)
{
	if( Lock(L"ChgReserveData") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	//予約変更
	BOOL chg = FALSE;
	for( size_t i=0; i<reserveList->size(); i++ ){
		if( _ChgReserveData(&(*reserveList)[i], FALSE) == TRUE ){
			chg = TRUE;
		}
	}
	if( chg == FALSE ){
		//変更成功したものがない
		UnLock();
		return FALSE;
	}

	wstring filePath = L"";
	GetSettingPath(filePath);
	filePath += L"\\";
	filePath += RESERVE_TEXT_NAME;

	this->reserveText.SaveReserveText(filePath.c_str());

	_ReloadBankMap();


	_SendNotifyUpdate();

	UnLock();
	return ret;
}

BOOL CReserveManager::_ChgReserveData(RESERVE_DATA* reserve, BOOL chgTime)
{
	if( reserve == NULL ){
		return FALSE;
	}
	map<DWORD, CReserveInfo*>::iterator itrInfo;
	itrInfo = this->reserveInfoMap.find(reserve->reserveID);
	if( itrInfo == this->reserveInfoMap.end() ){
		return FALSE;
	}
	RESERVE_DATA setData;
	itrInfo->second->GetData(&setData);

	BOOL recWaitFlag = FALSE;
	DWORD tunerID = 0;
	itrInfo->second->GetRecWaitMode(&recWaitFlag, &tunerID );

	BOOL chgCtrl = FALSE;
	if( recWaitFlag == TRUE ){
		//すでに録画中
		setData.recSetting.tuijyuuFlag = reserve->recSetting.tuijyuuFlag;
		setData.recSetting.batFilePath = reserve->recSetting.batFilePath;
		setData.recSetting.suspendMode = reserve->recSetting.suspendMode;
		setData.recSetting.rebootFlag = reserve->recSetting.rebootFlag;
		setData.recSetting.useMargineFlag = reserve->recSetting.useMargineFlag;
		setData.recSetting.endMargine = reserve->recSetting.endMargine;
		if( chgTime == TRUE ){
			//追従による時間変更
			setData.startTime = reserve->startTime;
			setData.durationSecond = reserve->durationSecond;
			setData.reserveStatus = reserve->reserveStatus;

			//コントロール経由で変更
			map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
			itrCtrl = this->tunerBankMap.find(tunerID);
			if( itrCtrl != this->tunerBankMap.end() ){
				itrCtrl->second->ChgReserve(&setData);
			}

			chgCtrl = TRUE;
		}else{
			if( reserve->eventID == 0xFFFF ){
				setData.durationSecond = reserve->durationSecond;
			}
		}
		
	}else{
		if( setData.eventID == 0xFFFF ){
			//プログラム予約の場合チャンネルが変わっている可能性あるためチェック
			if( setData.originalNetworkID != reserve->originalNetworkID ||
				setData.transportStreamID != reserve->transportStreamID ||
				setData.serviceID != reserve->serviceID ){
					//チャンネル変わってる
					itrInfo->second->ClearAddNGTuner();
					//サービスサポートしてないチューナー検索
					vector<DWORD> idList;
					if( this->tunerManager.GetNotSupportServiceTuner(
						reserve->originalNetworkID,
						reserve->transportStreamID,
						reserve->serviceID,
						&idList ) == TRUE ){
							itrInfo->second->SetNGChTunerID(&idList);
					}
			}
		}
		setData = *reserve;
	}

	this->reserveText.ChgReserve(&setData);
	if( chgCtrl == FALSE ){
		itrInfo->second->SetData(&setData);
	}

	return TRUE;
}

//予約情報を削除する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// reserveList		[IN]予約IDリスト
BOOL CReserveManager::DelReserveData(
	vector<DWORD>* reserveList
	)
{
	if( Lock(L"DelReserveData") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	//予約削除
	_DelReserveData(reserveList);

	wstring filePath = L"";
	GetSettingPath(filePath);
	filePath += L"\\";
	filePath += RESERVE_TEXT_NAME;

	this->reserveText.SaveReserveText(filePath.c_str());

	_ReloadBankMap();


	_SendNotifyUpdate();

	UnLock();
	return ret;
}

BOOL CReserveManager::_DelReserveData(
	vector<DWORD>* reserveList
)
{
	if( reserveList == NULL ){
		return FALSE;
	}
	for( size_t i=0; i<reserveList->size(); i++ ){
		map<DWORD, BANK_INFO*>::iterator itrBank;
		map<DWORD, BANK_WORK_INFO*>::iterator itrWork;
		for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++ ){
			itrWork = itrBank->second->reserveList.find((*reserveList)[i]);
			if( itrWork != itrBank->second->reserveList.end() ){

				map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
				itrCtrl = this->tunerBankMap.find(itrBank->second->tunerID);
				if( itrCtrl != this->tunerBankMap.end() ){
					itrCtrl->second->DeleteReserve((*reserveList)[i]);
				}
				SAFE_DELETE(itrWork->second);
				itrBank->second->reserveList.erase(itrWork);
			}
		}

		itrWork = this->NGReserveMap.find((*reserveList)[i]);
		if( itrWork != this->NGReserveMap.end() ){
			SAFE_DELETE(itrWork->second);
			this->NGReserveMap.erase(itrWork);
		}

		this->reserveText.DelReserve((*reserveList)[i]);

		map<DWORD, CReserveInfo*>::iterator itr;
		itr = this->reserveInfoMap.find((*reserveList)[i]);
		if( itr != this->reserveInfoMap.end() ){
			//IDリスト削除
			RESERVE_DATA data;
			itr->second->GetData(&data);
			map<LONGLONG, DWORD>::iterator itrID;
			LONGLONG keyID = _Create64Key2(data.originalNetworkID, data.transportStreamID, data.serviceID, data.eventID);
			itrID = this->reserveInfoIDMap.find(keyID);
			if( itrID != this->reserveInfoIDMap.end() ){
				this->reserveInfoIDMap.erase(itrID);
			}
			SAFE_DELETE(itr->second);
			this->reserveInfoMap.erase(itr);
		}
	}
	this->reserveText.SwapMap();
	map<DWORD, CReserveInfo*>(this->reserveInfoMap).swap(this->reserveInfoMap);
	return TRUE;
}

void CReserveManager::ReloadBankMap(BOOL notify)
{
	if( Lock(L"ReloadBankMap") == FALSE ) return ;

	_ReloadBankMap();
	if( notify == TRUE ){
		_SendNotifyUpdate();
	}
	
	UnLock();
}

void CReserveManager::_ReloadBankMap()
{
	OutputDebugString(L"Start _ReloadBankMap\r\n");
	DWORD time = GetTickCount();
	//まずバンクをクリア
	map<DWORD, BANK_INFO*>::iterator itrBank;
	for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
		map<DWORD, BANK_WORK_INFO*>::iterator itrWork;
		for( itrWork = itrBank->second->reserveList.begin(); itrWork != itrBank->second->reserveList.end(); itrWork++ ){
			SAFE_DELETE(itrWork->second);
		}
		itrBank->second->reserveList.clear();
	}
	map<DWORD, BANK_WORK_INFO*>::iterator itrNG;
	for( itrNG = this->NGReserveMap.begin(); itrNG != this->NGReserveMap.end(); itrNG++){
		SAFE_DELETE(itrNG->second);
	}
	this->NGReserveMap.clear();

	map<DWORD, BANK_INFO*>(this->bankMap).swap(this->bankMap);
	map<DWORD, BANK_WORK_INFO*>(this->NGReserveMap).swap(this->NGReserveMap);

	//待機状態に入っているもの以外クリア
	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++){
		itrCtrl->second->ClearNoCtrl();
	}

	//録画時間過ぎているものないかチェック
	CheckOverTimeReserve();

	if( this->autoDel == TRUE ){
		CCheckRecFile chkFile;
		chkFile.SetCheckFolder(&this->delFolderList);
		chkFile.SetDeleteExt(&this->delExtList);
		wstring defRecPath = L"";
		GetRecFolderPath(defRecPath);
		chkFile.CheckFreeSpace(&this->reserveInfoMap, defRecPath);
	}

	switch(this->reloadBankMapAlgo){
	case 0:
		_ReloadBankMapAlgo0();
		break;
	case 1:
		_ReloadBankMapAlgo1();
		break;
	default:
		_ReloadBankMapAlgo0();
		break;
	}


	Sleep(0);

	//予約情報を追加
	for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
		itrCtrl = this->tunerBankMap.find(itrBank->second->tunerID);
		if( itrCtrl != this->tunerBankMap.end() ){
			vector<CReserveInfo*> reserveInfo;
			map<DWORD, BANK_WORK_INFO*>::iterator itrItem;
			for( itrItem = itrBank->second->reserveList.begin(); itrItem != itrBank->second->reserveList.end(); itrItem++ ){
				reserveInfo.push_back(itrItem->second->reserveInfo);
			}
			if( reserveInfo.size() > 0 ){
				itrCtrl->second->AddReserve(&reserveInfo);
			}
		}
	}
	_OutputDebugString(L"End _ReloadBankMap %dmsec\r\n", GetTickCount()-time);
}

void CReserveManager::_ReloadBankMapAlgo0()
{
	map<DWORD, BANK_INFO*>::iterator itrBank;
	map<DWORD, BANK_WORK_INFO*>::iterator itrNG;

	//録画待機中のものをバンクに登録＆優先度と時間でソート
	map<DWORD, CReserveInfo*>::iterator itrInfo;
	multimap<LONGLONG, CReserveInfo*> sortTimeMap;
	for( itrInfo = this->reserveInfoMap.begin(); itrInfo != this->reserveInfoMap.end(); itrInfo++ ){
		BYTE recMode = 0;
		itrInfo->second->GetRecMode(&recMode);
		if( recMode != RECMODE_NO ){
			SYSTEMTIME time;
			itrInfo->second->GetStartTime(&time);
			sortTimeMap.insert(pair<LONGLONG, CReserveInfo*>(ConvertI64Time(time), itrInfo->second));
		}
	}
	multimap<wstring, BANK_WORK_INFO*> sortReserveMap;
	multimap<LONGLONG, CReserveInfo*>::iterator itrSortInfo;
	DWORD reserveNum = (DWORD)this->reserveInfoMap.size();
	DWORD reserveCount = 0;
	for( itrSortInfo = sortTimeMap.begin(); itrSortInfo != sortTimeMap.end(); itrSortInfo++ ){
		itrSortInfo->second->SetOverlapMode(0);
		BOOL recWaitFlag = FALSE;
		DWORD tunerID = 0;
		itrSortInfo->second->GetRecWaitMode(&recWaitFlag, &tunerID);
		if( recWaitFlag == TRUE ){
			//録画処理中なのでバンクに登録
			itrBank = this->bankMap.find(tunerID);
			if( itrBank != this->bankMap.end() ){
				BANK_WORK_INFO* item = new BANK_WORK_INFO;
				CreateWorkData(itrSortInfo->second, item, this->backPriorityFlag, reserveCount, reserveNum);
				itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(item->reserveID,item));
			}
		}else{
			//まだ録画処理されていないのでソートに追加
			BANK_WORK_INFO* item = new BANK_WORK_INFO;
			CreateWorkData(itrSortInfo->second, item, this->backPriorityFlag, reserveCount, reserveNum);
			sortReserveMap.insert(pair<wstring, BANK_WORK_INFO*>(item->sortKey, item));
		}
		reserveCount++;
	}

	Sleep(0);

	//予約の割り振り
	multimap<wstring, BANK_WORK_INFO*> tempMap;
	multimap<wstring, BANK_WORK_INFO*> tempNGMap;
	multimap<wstring, BANK_WORK_INFO*>::iterator itrSort;
	for( itrSort = sortReserveMap.begin(); itrSort !=  sortReserveMap.end(); itrSort++ ){
		BOOL insert = FALSE;
		if( itrSort->second->useTunerID == 0 ){
			//チューナー優先度より同一物理チャンネルで連続となるチューナーの使用を優先する
			if( this->sameChPriorityFlag == TRUE ){
				for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
					DWORD status = ChkInsertSameChStatus(itrBank->second, itrSort->second);
					if( status == 1 ){
						//問題なく追加可能
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
						break;
					}
				}
			}
			if( insert == FALSE ){
				for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
					DWORD status = ChkInsertStatus(itrBank->second, itrSort->second);
					if( status == 1 ){
						//問題なく追加可能
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
						break;
					}else if( status == 2 ){
						//追加可能だが終了時間と開始時間の重なった予約あり
						//仮追加
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						itrSort->second->preTunerID = itrBank->first;
						tempMap.insert(pair<wstring, BANK_WORK_INFO*>(itrSort->first, itrSort->second));
						insert = TRUE;
						break;
					}
				}
			}
		}else{
			//チューナー固定
			if( this->tunerManager.IsSupportService(itrSort->second->useTunerID, itrSort->second->ONID, itrSort->second->TSID, itrSort->second->SID) == TRUE ){
				map<DWORD, BANK_INFO*>::iterator itrManual;
				itrManual = this->bankMap.find(itrSort->second->useTunerID);
				if( itrManual != this->bankMap.end() ){
					itrManual->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
					insert = TRUE;
				}
			}
		}
		if( insert == FALSE ){
			//追加できなかった
			itrSort->second->reserveInfo->SetOverlapMode(2);
			this->NGReserveMap.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID, itrSort->second));

			tempNGMap.insert(pair<wstring, BANK_WORK_INFO*>(itrSort->first, itrSort->second));
		}
	}

	Sleep(0);

	//開始終了重なっている予約で、他のチューナーに回せるやつあるかチェック
	for( itrSort = tempMap.begin(); itrSort !=  tempMap.end(); itrSort++ ){
		BOOL insert = FALSE;
		for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
			if( itrBank->second->tunerID == itrSort->second->preTunerID ){
				if(ReChkInsertStatus(itrBank->second, itrSort->second) == 1 ){
					//前の予約移動した？このままでもOK
					break;
				}else{
					continue;
				}
			}
			DWORD status = ChkInsertStatus(itrBank->second, itrSort->second);
			if( status == 1 ){
				//問題なく追加可能
				itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
				insert = TRUE;
				break;
			}
		}
		if( insert == TRUE ){
			//仮追加を削除
			itrBank = this->bankMap.find(itrSort->second->preTunerID);
			if( itrBank != this->bankMap.end() ){
				map<DWORD, BANK_WORK_INFO*>::iterator itrDel;
				itrDel = itrBank->second->reserveList.find(itrSort->second->reserveID);
				if( itrDel != itrBank->second->reserveList.end() ){
					itrBank->second->reserveList.erase(itrDel);
				}
			}
		}
	}

	Sleep(0);

	multimap<wstring, BANK_WORK_INFO*>::iterator itrSortNG;
	//NGでチューナー入れ替えで録画できるものあるかチェック
	itrSortNG = tempNGMap.begin();
	while(itrSortNG != tempNGMap.end() ){
		if( itrSortNG->second->useTunerID != 0 ){
			//チューナー固定でNGになっているのは無視
			itrSortNG++;
			continue;
		}
		if( ChangeNGReserve(itrSortNG->second) == TRUE ){
			//登録できたのでNGから削除
			itrSortNG->second->reserveInfo->SetOverlapMode(0);
			itrNG = this->NGReserveMap.find(itrSortNG->second->reserveID);
			if( itrNG != this->NGReserveMap.end() ){
				this->NGReserveMap.erase(itrNG);
			}
			tempNGMap.erase(itrSortNG++);
		}else{
			itrSortNG++;
		}
	}
/*	for( itrSortNG = tempNGMap.begin(); itrSortNG != tempNGMap.end(); itrSortNG++){
		if( itrSortNG->second->useTunerID != 0 ){
			//チューナー固定でNGになっているのは無視
			continue;
		}
		if( ChangeNGReserve(itrSortNG->second) == TRUE ){
			//登録できたのでNGから削除
			itrSortNG->second->reserveInfo->SetOverlapMode(0);
			itrNG = this->NGReserveMap.find(itrSortNG->second->reserveID);
			if( itrNG != this->NGReserveMap.end() ){
				this->NGReserveMap.erase(itrNG);
			}
		}
	}*/

	//NGで少しでも録画できるかチェック
	for( itrSortNG = tempNGMap.begin(); itrSortNG != tempNGMap.end(); itrSortNG++){
		if( itrSortNG->second->useTunerID != 0 ){
			//チューナー固定でNGになっているのは無視
			continue;
		}
		DWORD maxDuration = 0;
		DWORD maxID = 0xFFFFFFFF;
		for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
			DWORD duration = ChkInsertNGStatus(itrBank->second, itrSortNG->second);
			if( maxDuration < duration && duration > 0){
				maxDuration = duration;
				maxID = itrBank->second->tunerID;
			}
		}
		if( maxDuration > 0 && maxID != 0xFFFFFFFF ){
			//少しでも録画できる場所あった
			itrBank = this->bankMap.find(maxID);
			if( itrBank != this->bankMap.end() ){
				itrSortNG->second->reserveInfo->SetOverlapMode(1);
				itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSortNG->second->reserveID,itrSortNG->second));

				//登録できたのでNGから削除
				itrNG = this->NGReserveMap.find(itrSortNG->second->reserveID);
				if( itrNG != this->NGReserveMap.end() ){
					this->NGReserveMap.erase(itrNG);
				}
			}
		}
	}
}

void CReserveManager::_ReloadBankMapAlgo1()
{
	map<DWORD, BANK_INFO*>::iterator itrBank;
	map<DWORD, BANK_WORK_INFO*>::iterator itrNG;

	//録画待機中のものをバンクに登録＆優先度と時間でソート
	map<DWORD, CReserveInfo*>::iterator itrInfo;
	multimap<LONGLONG, CReserveInfo*> sortTimeMap;
	for( itrInfo = this->reserveInfoMap.begin(); itrInfo != this->reserveInfoMap.end(); itrInfo++ ){
		BYTE recMode = 0;
		itrInfo->second->GetRecMode(&recMode);
		if( recMode != RECMODE_NO ){
			SYSTEMTIME time;
			itrInfo->second->GetStartTime(&time);
			sortTimeMap.insert(pair<LONGLONG, CReserveInfo*>(ConvertI64Time(time), itrInfo->second));
		}
	}
	multimap<wstring, BANK_WORK_INFO*> sortReserveMap;
	multimap<LONGLONG, CReserveInfo*>::iterator itrSortInfo;
	DWORD reserveNum = (DWORD)this->reserveInfoMap.size();
	DWORD reserveCount = 0;
	for( itrSortInfo = sortTimeMap.begin(); itrSortInfo != sortTimeMap.end(); itrSortInfo++ ){
		itrSortInfo->second->SetOverlapMode(0);
		BOOL recWaitFlag = FALSE;
		DWORD tunerID = 0;
		itrSortInfo->second->GetRecWaitMode(&recWaitFlag, &tunerID);
		if( recWaitFlag == TRUE ){
			//録画処理中なのでバンクに登録
			itrBank = this->bankMap.find(tunerID);
			if( itrBank != this->bankMap.end() ){
				BANK_WORK_INFO* item = new BANK_WORK_INFO;
				CreateWorkData(itrSortInfo->second, item, this->backPriorityFlag, reserveCount, reserveNum);
				itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(item->reserveID,item));
			}
		}else{
			//まだ録画処理されていないのでソートに追加
			BANK_WORK_INFO* item = new BANK_WORK_INFO;
			CreateWorkData(itrSortInfo->second, item, this->backPriorityFlag, reserveCount, reserveNum);
			sortReserveMap.insert(pair<wstring, BANK_WORK_INFO*>(item->sortKey, item));
		}
		reserveCount++;
	}

	Sleep(0);

	//予約の割り振り
	multimap<wstring, BANK_WORK_INFO*> tempNGMap1Pass;
	multimap<wstring, BANK_WORK_INFO*> tempMap2Pass;
	multimap<wstring, BANK_WORK_INFO*> tempNGMap2Pass;
	multimap<wstring, BANK_WORK_INFO*>::iterator itrSort;
	for( itrSort = sortReserveMap.begin(); itrSort !=  sortReserveMap.end(); itrSort++ ){
		BOOL insert = FALSE;
		if( itrSort->second->useTunerID == 0 ){
			//チューナー優先度より同一物理チャンネルで連続となるチューナーの使用を優先する
			if( this->sameChPriorityFlag == TRUE ){
				for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
					DWORD status = ChkInsertSameChStatus(itrBank->second, itrSort->second);
					if( status == 1 ){
						//問題なく追加可能
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
						break;
					}
				}
			}
			if( insert == FALSE ){
				for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
					DWORD status = ChkInsertStatus(itrBank->second, itrSort->second);
					if( status == 1 ){
						//問題なく追加可能
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
						break;
					}
				}
			}
		}else{
			//チューナー固定
			if( this->tunerManager.IsSupportService(itrSort->second->useTunerID, itrSort->second->ONID, itrSort->second->TSID, itrSort->second->SID) == TRUE ){
				map<DWORD, BANK_INFO*>::iterator itrManual;
				itrManual = this->bankMap.find(itrSort->second->useTunerID);
				if( itrManual != this->bankMap.end() ){
					itrManual->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
					insert = TRUE;
				}
			}
		}
		if( insert == FALSE ){
			//追加できなかった
			tempNGMap1Pass.insert(pair<wstring, BANK_WORK_INFO*>(itrSort->first, itrSort->second));
		}
	}

	//2Pass
	for( itrSort = tempNGMap1Pass.begin(); itrSort !=  tempNGMap1Pass.end(); itrSort++ ){
		BOOL insert = FALSE;
		if( itrSort->second->useTunerID == 0 ){
			//チューナー優先度より同一物理チャンネルで連続となるチューナーの使用を優先する
			if( this->sameChPriorityFlag == TRUE ){
				for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
					DWORD status = ChkInsertSameChStatus(itrBank->second, itrSort->second);
					if( status == 1 ){
						//問題なく追加可能
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
						break;
					}
				}
			}
			if( insert == FALSE ){
				for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
					DWORD status = ChkInsertStatus(itrBank->second, itrSort->second);
					if( status == 1 ){
						//問題なく追加可能
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
						break;
					}else if( status == 2 ){
						//追加可能だが終了時間と開始時間の重なった予約あり
						//仮追加
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						itrSort->second->preTunerID = itrBank->first;
						tempMap2Pass.insert(pair<wstring, BANK_WORK_INFO*>(itrSort->first, itrSort->second));
						insert = TRUE;
						break;
					}
				}
			}
		}else{
			//チューナー固定
			if( this->tunerManager.IsSupportService(itrSort->second->useTunerID, itrSort->second->ONID, itrSort->second->TSID, itrSort->second->SID) == TRUE ){
				map<DWORD, BANK_INFO*>::iterator itrManual;
				itrManual = this->bankMap.find(itrSort->second->useTunerID);
				if( itrManual != this->bankMap.end() ){
					itrManual->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
					insert = TRUE;
				}
			}
		}
		if( insert == FALSE ){
			//追加できなかった
			itrSort->second->reserveInfo->SetOverlapMode(2);
			this->NGReserveMap.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID, itrSort->second));

			tempNGMap2Pass.insert(pair<wstring, BANK_WORK_INFO*>(itrSort->first, itrSort->second));
		}
	}

	Sleep(0);

	//開始終了重なっている予約で、他のチューナーに回せるやつあるかチェック
	for( itrSort = tempMap2Pass.begin(); itrSort !=  tempMap2Pass.end(); itrSort++ ){
		BOOL insert = FALSE;
		for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
			if( itrBank->second->tunerID == itrSort->second->preTunerID ){
				if(ReChkInsertStatus(itrBank->second, itrSort->second) == 1 ){
					//前の予約移動した？このままでもOK
					break;
				}else{
					continue;
				}
			}
			DWORD status = ChkInsertStatus(itrBank->second, itrSort->second);
			if( status == 1 ){
				//問題なく追加可能
				itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
				insert = TRUE;
				break;
			}
		}
		if( insert == TRUE ){
			//仮追加を削除
			itrBank = this->bankMap.find(itrSort->second->preTunerID);
			if( itrBank != this->bankMap.end() ){
				map<DWORD, BANK_WORK_INFO*>::iterator itrDel;
				itrDel = itrBank->second->reserveList.find(itrSort->second->reserveID);
				if( itrDel != itrBank->second->reserveList.end() ){
					itrBank->second->reserveList.erase(itrDel);
				}
			}
		}
	}

	Sleep(0);

	multimap<wstring, BANK_WORK_INFO*>::iterator itrSortNG;
	//NGでチューナー入れ替えで録画できるものあるかチェック
	itrSortNG = tempNGMap2Pass.begin();
	while(itrSortNG != tempNGMap2Pass.end() ){
		if( itrSortNG->second->useTunerID != 0 ){
			//チューナー固定でNGになっているのは無視
			itrSortNG++;
			continue;
		}
		if( ChangeNGReserve(itrSortNG->second) == TRUE ){
			//登録できたのでNGから削除
			itrSortNG->second->reserveInfo->SetOverlapMode(0);
			itrNG = this->NGReserveMap.find(itrSortNG->second->reserveID);
			if( itrNG != this->NGReserveMap.end() ){
				this->NGReserveMap.erase(itrNG);
			}
			tempNGMap2Pass.erase(itrSortNG++);
		}else{
			itrSortNG++;
		}
	}

	//NGで少しでも録画できるかチェック
	for( itrSortNG = tempNGMap2Pass.begin(); itrSortNG != tempNGMap2Pass.end(); itrSortNG++){
		if( itrSortNG->second->useTunerID != 0 ){
			//チューナー固定でNGになっているのは無視
			continue;
		}
		DWORD maxDuration = 0;
		DWORD maxID = 0xFFFFFFFF;
		for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
			DWORD duration = ChkInsertNGStatus(itrBank->second, itrSortNG->second);
			if( maxDuration < duration && duration > 0){
				maxDuration = duration;
				maxID = itrBank->second->tunerID;
			}
		}
		if( maxDuration > 0 && maxID != 0xFFFFFFFF ){
			//少しでも録画できる場所あった
			itrBank = this->bankMap.find(maxID);
			if( itrBank != this->bankMap.end() ){
				itrSortNG->second->reserveInfo->SetOverlapMode(1);
				itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSortNG->second->reserveID,itrSortNG->second));

				//登録できたのでNGから削除
				itrNG = this->NGReserveMap.find(itrSortNG->second->reserveID);
				if( itrNG != this->NGReserveMap.end() ){
					this->NGReserveMap.erase(itrNG);
				}
			}
		}
	}
}

BOOL CReserveManager::ChangeNGReserve(BANK_WORK_INFO* inItem)
{
	BOOL ret = FALSE;

	if( inItem == NULL ){
		return FALSE;
	}

	map<DWORD, BANK_INFO*>::iterator itrBank;
	for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++ ){
		if( inItem->reserveInfo->IsNGTuner(itrBank->second->tunerID) == FALSE ){
			//NGじゃないバンクあり

			//時間のかぶる予約一覧取得
			vector<BANK_WORK_INFO*> chkReserve;
			map<DWORD, BANK_WORK_INFO*>::iterator itrWork;
			for( itrWork = itrBank->second->reserveList.begin(); itrWork != itrBank->second->reserveList.end(); itrWork++ ){
				if( itrWork->second->chID == inItem->chID ){
					//同一チャンネルなのでかぶりではない
					continue;
				}

				//時間かぶっている予約かチェック
				if( itrWork->second->startTime <= inItem->startTime && 
					inItem->startTime < itrWork->second->endTime){
					//開始時間が含まれている
						chkReserve.push_back(itrWork->second);
				}else
				if( itrWork->second->startTime < inItem->endTime &&
					inItem->endTime <= itrWork->second->endTime ){
					//終了時間が含まれている
						chkReserve.push_back(itrWork->second);
				}else
				if( inItem->startTime <= itrWork->second->startTime &&
					itrWork->second->startTime < inItem->endTime ){
					//開始から終了の間に含んでしまう
						chkReserve.push_back(itrWork->second);
				}else
				if( inItem->startTime < itrWork->second->endTime &&
					itrWork->second->endTime <= inItem->endTime ){
					//開始から終了の間に含んでしまう
						chkReserve.push_back(itrWork->second);
				}
			}

			//かぶった予約が別バンクで行えるかチェック
			BOOL moveOK = TRUE;
			vector<BANK_WORK_INFO*> tempIn;
			for( size_t i=0; i<chkReserve.size(); i++ ){
				BOOL inFlag = FALSE;

				//録画中の予約とかぶるとこのバンクは無理
				BOOL recWaitFlag = FALSE;
				DWORD tunerID = 0;
				chkReserve[i]->reserveInfo->GetRecWaitMode(&recWaitFlag, &tunerID);
				if( recWaitFlag == TRUE ){
					moveOK = FALSE;
					break;
				}

				map<DWORD, BANK_INFO*>::iterator itrBank2;
				//まず問題なく入る場所を探す
				for( itrBank2 = this->bankMap.begin(); itrBank2 != this->bankMap.end(); itrBank2++ ){
					if( itrBank2->first == itrBank->first ){
						continue;
					}
					if( ReChkInsertStatus(itrBank2->second, chkReserve[i]) == 1 ){
						inFlag = TRUE;
						//行えるのでバンク移動
						chkReserve[i]->preTunerID = itrBank->second->tunerID;

						itrBank2->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(chkReserve[i]->reserveID, chkReserve[i]));

						tempIn.push_back(chkReserve[i]);
						break;
					}
				}
				if(inFlag == FALSE ){
					//開始時間とか重なるものを探す
					for( itrBank2 = this->bankMap.begin(); itrBank2 != this->bankMap.end(); itrBank2++ ){
						if( itrBank2->first == itrBank->first ){
							continue;
						}
						if( ReChkInsertStatus(itrBank2->second, chkReserve[i]) != 0 ){
							inFlag = TRUE;
							//行えるのでバンク移動
							chkReserve[i]->preTunerID = itrBank->second->tunerID;

							itrBank2->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(chkReserve[i]->reserveID, chkReserve[i]));

							tempIn.push_back(chkReserve[i]);
							break;
						}
					}
				}
				if(inFlag == FALSE ){
					moveOK = FALSE;
					break;
				}
			}
			if(moveOK == FALSE ){
				//移動できなかったので復帰
				for( size_t i=0; i<tempIn.size(); i++ ){
					map<DWORD, BANK_INFO*>::iterator itrBank2;
					itrBank2 = this->bankMap.find(tempIn[i]->preTunerID);
					if( itrBank2 != this->bankMap.end() ){
						map<DWORD, BANK_WORK_INFO*>::iterator itrRes;
						itrRes = itrBank2->second->reserveList.find(tempIn[i]->reserveID);
						if( itrRes != itrBank2->second->reserveList.end() ){
							itrBank2->second->reserveList.erase(itrRes);
						}
					}
				}
			}else{
				//このバンクから移動したものを削除
				for( size_t i=0; i<tempIn.size(); i++ ){
					map<DWORD, BANK_WORK_INFO*>::iterator itrRes;
					itrRes = itrBank->second->reserveList.find(tempIn[i]->reserveID);
					if( itrRes != itrBank->second->reserveList.end() ){
						itrBank->second->reserveList.erase(itrRes);
					}
				}
				//このバンクにNG追加
				itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(inItem->reserveID, inItem));

				ret = TRUE;
				break;
			}
		}
	}

	return ret;
}

void CReserveManager::CheckOverTimeReserve()
{
	LONGLONG nowTime = GetNowI64Time();

	vector<DWORD> deleteList;
	map<DWORD, CReserveInfo*>::iterator itrInfo;
	for( itrInfo = this->reserveInfoMap.begin(); itrInfo != this->reserveInfoMap.end(); itrInfo++ ){
		BOOL recWaitFlag = FALSE;
		DWORD tunerID = 0;
		itrInfo->second->GetRecWaitMode(&recWaitFlag, &tunerID);
		if( recWaitFlag == FALSE ){
			//録画状態ではない
			RESERVE_DATA data;
			itrInfo->second->GetData(&data);

			int startMargine = 0;
			int endMargine = 0;
			if( data.recSetting.useMargineFlag == TRUE ){
				startMargine = data.recSetting.startMargine;
				endMargine = data.recSetting.endMargine;
			}else{
				startMargine = this->defStartMargine;
				endMargine = this->defEndMargine;
			}

			LONGLONG startTime = ConvertI64Time(data.startTime);
			if(startMargine < 0 ){
				//マージンマイナス値なら開始時間も考慮する
				startTime -= ((LONGLONG)startMargine) * I64_1SEC;
			}

			LONGLONG endTime = GetSumTime(data.startTime, data.durationSecond);
			if(endMargine < 0 ){
				//マージンマイナス値なら終了時間も考慮する
				endTime += ((LONGLONG)endMargine) * I64_1SEC;
			}

			if( endTime < nowTime ){
				//終了時間過ぎてしまっている
				deleteList.push_back(itrInfo->first);
				if( data.recSetting.recMode != RECMODE_NO ){
					//無効ものは結果に残さない
					REC_FILE_INFO item;
					item = data;
					if( data.overlapMode == 0 ){
						item.recStatus = REC_END_STATUS_START_ERR;
						item.comment = L"録画時間に起動していなかった可能性があります";
					}else{
						item.recStatus = REC_END_STATUS_NO_TUNER;
						item.comment = L"チューナー不足のため失敗しました";
					}
					this->recInfoText.AddRecInfo(&item);
				}
			}
		}
	}
	if( deleteList.size() > 0 ){
		_DelReserveData(&deleteList);
		this->reserveText.SaveReserveText();
		this->recInfoText.SaveRecInfoText();
	}
}

void CReserveManager::CreateWorkData(CReserveInfo* reserveInfo, BANK_WORK_INFO* workInfo, BOOL backPriority, DWORD reserveCount, DWORD reserveNum)
{
	workInfo->reserveInfo = reserveInfo;

	RESERVE_DATA data;
	reserveInfo->GetData(&data);

	DWORD tunerID = 0;
	reserveInfo->GetRecWaitMode(&workInfo->recWaitFlag, &tunerID);

	int startMargine = 0;
	int endMargine = 0;
	if( data.recSetting.useMargineFlag == TRUE ){
		startMargine = data.recSetting.startMargine;
		endMargine = data.recSetting.endMargine;
	}else{
		startMargine = this->defStartMargine;
		endMargine = this->defEndMargine;
	}
	workInfo->startTime = ConvertI64Time(data.startTime);
	if(startMargine < 0 ){
		//マージンマイナス値なら開始時間も考慮する
		workInfo->startTime -= ((LONGLONG)startMargine) * I64_1SEC;
	}

	workInfo->endTime = GetSumTime(data.startTime, data.durationSecond);
	if(endMargine < 0 ){
		//マージンマイナス値なら終了時間も考慮する
		workInfo->endTime += ((LONGLONG)endMargine) * I64_1SEC;
	}

	workInfo->priority = data.recSetting.priority;

	workInfo->reserveID = data.reserveID;

	workInfo->chID = ((DWORD)data.originalNetworkID)<<16 | data.transportStreamID;

	workInfo->useTunerID = data.recSetting.tunerID;

	workInfo->ONID = data.originalNetworkID;
	workInfo->TSID = data.transportStreamID;
	workInfo->SID = data.serviceID;

	BYTE tunerManual = 1;
	if( workInfo->useTunerID != 0 ){
		tunerManual = 0;
	}

	if( backPriority == TRUE ){
		//後の番組優先
		Format(workInfo->sortKey, L"%01d%01d%08I64x%05d", tunerManual, 9-workInfo->priority, workInfo->startTime*(-1), reserveNum-reserveCount);
	}else{
		//前の番組優先
		Format(workInfo->sortKey, L"%01d%01d%08I64x%05d", tunerManual, 9-workInfo->priority, workInfo->startTime, reserveCount);
	}
}

DWORD CReserveManager::ChkInsertSameChStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem)
{
	if( bank == NULL || inItem == NULL ){
		return 0;
	}
	if( inItem->reserveInfo->IsNGTuner(bank->tunerID) == TRUE ){
		return 0;
	}
	DWORD status = 0;
	map<DWORD, BANK_WORK_INFO*>::iterator itrBank;
	for( itrBank = bank->reserveList.begin(); itrBank != bank->reserveList.end(); itrBank++ ){
		if( itrBank->second->chID == inItem->chID ){
			//同一チャンネル
			if(( itrBank->second->startTime <= inItem->startTime && inItem->startTime <= itrBank->second->endTime ) ||
				( itrBank->second->startTime <= inItem->endTime && inItem->endTime <= itrBank->second->endTime ) ||
				( inItem->startTime <= itrBank->second->startTime && itrBank->second->startTime <= inItem->endTime ) ||
				( inItem->startTime <= itrBank->second->endTime && itrBank->second->endTime <= inItem->endTime ) 
				){
					//開始時間か終了時間が重なっている
					status = 1;
			}
			
		}else{
			//別チャンネルで開始時間と終了時間が重なっていないかチェック
			if( itrBank->second->startTime == inItem->endTime || itrBank->second->endTime == inItem->startTime ){
				//連続予約の可能性あり
				status = 2;
			}else if(( itrBank->second->startTime <= inItem->startTime && inItem->startTime <= itrBank->second->endTime ) ||
				( itrBank->second->startTime <= inItem->endTime && inItem->endTime <= itrBank->second->endTime ) ||
				( inItem->startTime <= itrBank->second->startTime && itrBank->second->startTime <= inItem->endTime ) ||
				( inItem->startTime <= itrBank->second->endTime && itrBank->second->endTime <= inItem->endTime ) 
				){
					//開始時間か終了時間が重なっている
					status = 0;
					break;
			}
		}
	}

	return status;
}

DWORD CReserveManager::ChkInsertStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem)
{
	if( bank == NULL || inItem == NULL ){
		return 0;
	}
	if( inItem->reserveInfo->IsNGTuner(bank->tunerID) == TRUE ){
		return 0;
	}
	DWORD status = 1;
	map<DWORD, BANK_WORK_INFO*>::iterator itrBank;
	for( itrBank = bank->reserveList.begin(); itrBank != bank->reserveList.end(); itrBank++ ){
		if( itrBank->second->chID == inItem->chID ){
			//同一チャンネルなのでOK
			continue;
		}

		//開始時間と終了時間が重なっていないかチェック
		if( itrBank->second->startTime == inItem->endTime || itrBank->second->endTime == inItem->startTime ){
			//連続予約の可能性あり
			status = 2;
		}else if(( itrBank->second->startTime <= inItem->startTime && inItem->startTime <= itrBank->second->endTime ) ||
			( itrBank->second->startTime <= inItem->endTime && inItem->endTime <= itrBank->second->endTime ) ||
			( inItem->startTime <= itrBank->second->startTime && itrBank->second->startTime <= inItem->endTime ) ||
			( inItem->startTime <= itrBank->second->endTime && itrBank->second->endTime <= inItem->endTime ) 
			){
				//開始時間か終了時間が重なっている
				status = 0;
				break;
		}
	}

	return status;
}

DWORD CReserveManager::ReChkInsertStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem)
{
	if( bank == NULL || inItem == NULL ){
		return 0;
	}
	if( inItem->reserveInfo->IsNGTuner(bank->tunerID) == TRUE ){
		return 0;
	}
	DWORD status = 1;
	map<DWORD, BANK_WORK_INFO*>::iterator itrBank;
	for( itrBank = bank->reserveList.begin(); itrBank != bank->reserveList.end(); itrBank++ ){
		if( itrBank->second->reserveID != inItem->reserveID ){
			if( itrBank->second->chID == inItem->chID ){
				//同一チャンネルなのでOK
				continue;
			}

			//開始時間と終了時間が重なっていないかチェック
			if( itrBank->second->startTime == inItem->endTime || itrBank->second->endTime == inItem->startTime ){
				//連続予約の可能性あり
				status = 2;
			}else if(( itrBank->second->startTime <= inItem->startTime && inItem->startTime <= itrBank->second->endTime ) ||
				( itrBank->second->startTime <= inItem->endTime && inItem->endTime <= itrBank->second->endTime ) ||
				( inItem->startTime <= itrBank->second->startTime && itrBank->second->startTime <= inItem->endTime ) ||
				( inItem->startTime <= itrBank->second->endTime && itrBank->second->endTime <= inItem->endTime ) 
				){
					//開始時間か終了時間が重なっている
					status = 0;
					break;
			}
		}
	}

	return status;
}

DWORD CReserveManager::ChkInsertNGStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem)
{
	if( bank == NULL || inItem == NULL ){
		return 0;
	}
	if( inItem->reserveInfo->IsNGTuner(bank->tunerID) == TRUE ){
		return 0;
	}

	LONGLONG chkStartTime = inItem->startTime;
	LONGLONG chkEndTime = inItem->endTime;
	map<DWORD, BANK_WORK_INFO*>::iterator itrBank;
	for( itrBank = bank->reserveList.begin(); itrBank != bank->reserveList.end(); itrBank++ ){
		if( itrBank->second->startTime == inItem->startTime && itrBank->second->endTime == inItem->endTime ){
			//時間完全に一緒ならこれによってNGになったはず
			return 0;
		}
		if( itrBank->second->startTime <= chkStartTime && chkStartTime <= itrBank->second->endTime && chkEndTime <= itrBank->second->endTime ){
			//開始時間が含まれ、終了時間まで含まれている
			if( itrBank->second->priority >= inItem->priority ){
				//優先度も高いのでNG
				return 0;
			}
		}else
		if( itrBank->second->startTime <= chkStartTime && chkStartTime <= itrBank->second->endTime && chkEndTime > itrBank->second->endTime ){
			//開始時間が含まれ、終了時間まで含まれない
			if( itrBank->second->priority >= inItem->priority ){
				//優先度も高いので開始時間削る
				chkStartTime = itrBank->second->endTime;
			}
		}else
		if( itrBank->second->startTime <= chkEndTime && chkEndTime <= itrBank->second->endTime && chkStartTime < itrBank->second->startTime ){
			//終了時間が含まれ、開始時間まで含まれない
			if( itrBank->second->priority >= inItem->priority ){
				//優先度も高いので終了時間削る
				chkEndTime = itrBank->second->startTime;
			}
		}else
		if( chkStartTime <= itrBank->second->startTime && itrBank->second->startTime <= chkEndTime && itrBank->second->endTime <= chkEndTime ){
			//開始から終了の間に含んでしまう
			if( itrBank->second->priority >= inItem->priority ){
				//優先度も高いので終了時間削る
				chkEndTime = itrBank->second->startTime;
			}
		}
		if( chkEndTime < chkStartTime ){
			//終了時間の方が早いとかおかしい
			return 0;
		}
	}

	DWORD duration = (DWORD)((chkEndTime - chkStartTime)/I64_1SEC);

	return duration;
}

UINT WINAPI CReserveManager::BankCheckThread(LPVOID param)
{
	CReserveManager* sys = (CReserveManager*)param;
	CSendCtrlCmd sendCtrl;
	DWORD wait = 1000;
	DWORD countTuijyuChk = 11;

	while(1){
		if( ::WaitForSingleObject(sys->bankCheckStopEvent, wait) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		//終了している予約の確認
		if( sys->Lock(L"BankCheckThread1") == TRUE){
			sys->CheckEndReserve();
			sys->UnLock();
		}

		//エラーの発生しているチューナーの確認
		if( sys->Lock(L"BankCheckThread2") == TRUE){
			sys->CheckErrReserve();
			sys->UnLock();
		}

		//追従の確認
		countTuijyuChk++;
		if( countTuijyuChk > 10 ){
			if( sys->Lock(L"BankCheckThread3") == TRUE){
				sys->CheckTuijyu();
				sys->UnLock();
			}
			countTuijyuChk = 0;
		}

		//バッチ処理の確認
		if( sys->Lock(L"BankCheckThread4") == TRUE){
			sys->CheckBatWork();
			sys->UnLock();
		}

		//自動削除の確認
		if( sys->autoDel == TRUE ){
			if( sys->Lock(L"BankCheckThread5") == TRUE){
				CCheckRecFile chkFile;
				chkFile.SetCheckFolder(&sys->delFolderList);
				chkFile.SetDeleteExt(&sys->delExtList);
				wstring defRecPath = L"";
				GetRecFolderPath(defRecPath);
				chkFile.CheckFreeSpace(&sys->reserveInfoMap, defRecPath);
				sys->UnLock();
			}
		}

		//EPG取得時間の確認
		if( sys->Lock(L"BankCheckThread6") == TRUE){
			LONGLONG capTime = 0;
			if( sys->GetNextEpgcapTime(&capTime, -1) == TRUE ){
				if( GetNowI64Time() > capTime ){
					//開始時間過ぎたので開始
					sys->_StartEpgCap();
				}
			}
			sys->UnLock();
		}

		//録画状態の通知
		if( sys->notifyStatus == 0 ){
			if( sys->Lock(L"BankCheckThread7") == TRUE){
				map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
				for( itrCtrl = sys->tunerBankMap.begin(); itrCtrl != sys->tunerBankMap.end(); itrCtrl++ ){
					if( itrCtrl->second->IsRecWork() == TRUE ){
						sys->SendNotifyStatus(1);
						break;
					}
				}
				sys->UnLock();
			}
		}else if( sys->notifyStatus == 1 ){
			if( sys->Lock(L"BankCheckThread8") == TRUE){
				map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
				BOOL noRec = TRUE;
				for( itrCtrl = sys->tunerBankMap.begin(); itrCtrl != sys->tunerBankMap.end(); itrCtrl++ ){
					if( itrCtrl->second->IsRecWork() == TRUE ){
						noRec = FALSE;
						break;
					}
				}
				if( noRec == TRUE ){
					sys->SendNotifyStatus(0);
				}
				sys->UnLock();
			}
		}

		//EPG取得状態のチェック
		if( sys->epgCapCheckFlag == TRUE ){
			if( sys->Lock(L"BankCheckThread9") == TRUE){
				if( sys->IsEpgCap() == FALSE ){
					//取得完了
					sys->SendNotifyStatus(0);
					sys->epgCapCheckFlag = FALSE;
					sys->EnableSuspendWork(0, 0, 1);
				}
				sys->UnLock();
			}
		}

	}
	return 0;
}

void CReserveManager::CheckEndReserve()
{
	BOOL needSave = FALSE;
	BYTE suspendMode = 0xFF;
	BYTE rebootFlag = 0xFF;
	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		map<DWORD, END_RESERVE_INFO*> reserveMap;
		itrCtrl->second->GetEndReserve(&reserveMap);

		if( reserveMap.size() > 0 ){
			needSave = TRUE;
			vector<DWORD> deleteList;

			map<DWORD, END_RESERVE_INFO*>::iterator itrEnd;
			for( itrEnd = reserveMap.begin(); itrEnd != reserveMap.end(); itrEnd++){
				//録画済みとして登録
				RESERVE_DATA data;
				itrEnd->second->reserveInfo->GetData(&data);
				REC_FILE_INFO item;
				item = data;
				item.recFilePath = itrEnd->second->recFilePath;
				item.drops = itrEnd->second->drop;
				item.scrambles = itrEnd->second->scramble;
				if( itrEnd->second->endType == REC_END_STATUS_NORMAL ){
					if( ConvertI64Time(data.startTime) != ConvertI64Time(data.startTimeEpg) ){
						item.recStatus = REC_END_STATUS_CHG_TIME;
						item.comment = L"開始時間が変更されました";
					}else{
						item.recStatus = REC_END_STATUS_NORMAL;
						if( data.recSetting.recMode == RECMODE_VIEW ){
							item.comment = L"終了";
						}else{
							item.comment = L"録画終了";
						}
					}
				}else if( itrEnd->second->endType == REC_END_STATUS_NOT_FIND_PF ){
					item.recStatus = REC_END_STATUS_NOT_FIND_PF;
					item.comment = L"録画中に番組情報を確認できませんでした";
				}else if( itrEnd->second->endType == REC_END_STATUS_NEXT_START_END ){
					item.recStatus = REC_END_STATUS_NEXT_START_END;
					item.comment = L"次の予約開始のためにキャンセルされました";
				}else if( itrEnd->second->endType == REC_END_STATUS_END_SUBREC ){
					item.recStatus = REC_END_STATUS_END_SUBREC;
					item.comment = L"録画終了（空き容量不足で別フォルダへの保存が発生）";
				}else if( itrEnd->second->endType == REC_END_STATUS_ERR_RECSTART ){
					item.recStatus = REC_END_STATUS_ERR_RECSTART;
					item.comment = L"録画開始処理に失敗しました（空き容量不足の可能性あり）";
				}else if( itrEnd->second->endType == REC_END_STATUS_NOT_START_HEAD ){
					item.recStatus = REC_END_STATUS_NOT_START_HEAD;
					item.comment = L"一部のみ録画が実行された可能性があります";
				}else if( itrEnd->second->endType == REC_END_STATUS_ERR_CH_CHG ){
					item.recStatus = REC_END_STATUS_ERR_CH_CHG;
					item.comment = L"指定チャンネルのデータがBonDriverから出力されなかった可能性があります";
				}else{
					item.recStatus = itrEnd->second->endType;
					item.comment = L"録画中にキャンセルされた可能性があります";
				}
				this->recInfoText.AddRecInfo(&item);
				_SendTweet(TW_REC_END, &item, NULL, NULL);

				//バッチ処理追加
				if(itrEnd->second->endType == REC_END_STATUS_NORMAL || itrEnd->second->endType == REC_END_STATUS_NEXT_START_END ){
					if( data.recSetting.batFilePath.size() > 0 && itrEnd->second->reserveInfo->IsContinueRec() == FALSE){
						BAT_WORK_INFO batInfo;
						batInfo.tunerID = itrEnd->second->tunerID;
						batInfo.reserveInfo = data;
						batInfo.recFileInfo = item;

						this->batManager.AddBatWork(&batInfo);
					}else{
						suspendMode = data.recSetting.suspendMode;
						rebootFlag = data.recSetting.rebootFlag;
						OutputDebugString(L"★Suspend　add");
					}
				}else{
					suspendMode = data.recSetting.suspendMode;
					rebootFlag = data.recSetting.rebootFlag;
						OutputDebugString(L"★Suspend　add2");
				}

				deleteList.push_back(itrEnd->second->reserveID);
				SAFE_DELETE(itrEnd->second);
			}
			//予約一覧から削除
			_DelReserveData(&deleteList);
		}
	}

	if( needSave == TRUE ){
		//情報ファイルの更新
		wstring filePath = L"";
		GetSettingPath(filePath);
		filePath += L"\\";
		filePath += RESERVE_TEXT_NAME;

		this->reserveText.SaveReserveText(filePath.c_str());

		wstring recFilePath = L"";
		GetSettingPath(recFilePath);
		recFilePath += L"\\";
		recFilePath += REC_INFO_TEXT_NAME;

		this->recInfoText.SaveRecInfoText(recFilePath.c_str());

		_SendNotifyUpdate();
	}
	if( suspendMode != 0xFF && rebootFlag != 0xFF ){
		EnableSuspendWork(suspendMode, rebootFlag, 0);
	}
}

void CReserveManager::CheckErrReserve()
{
	BOOL needNotify = FALSE;
	vector<BANK_WORK_INFO*> addReserve;
	vector<BANK_WORK_INFO*> NGAddReserve;

	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		if( itrCtrl->second->IsOpenErr() == TRUE ){
			vector<CReserveInfo*> reserveInfo;
			itrCtrl->second->GetOpenErrReserve(&reserveInfo);

			for( size_t i=0 ;i<reserveInfo.size(); i++ ){
				//バンクから削除
				RESERVE_DATA data;
				reserveInfo[i]->GetData(&data);

				map<DWORD, BANK_INFO*>::iterator itrBank;
				itrBank = this->bankMap.find(itrCtrl->first);
				if( itrBank != this->bankMap.end()){
					map<DWORD, BANK_WORK_INFO*>::iterator itrWork;
					itrWork = itrBank->second->reserveList.find(data.reserveID);
					if( itrWork != itrBank->second->reserveList.end()){
						SAFE_DELETE(itrWork->second);
						itrBank->second->reserveList.erase(itrWork);
					}
				}


				BANK_WORK_INFO* item = new BANK_WORK_INFO;
				CreateWorkData(reserveInfo[i], item, this->backPriorityFlag, 0, 0);
				reserveInfo[i]->AddNGTunerID(itrCtrl->first);
				reserveInfo[i]->SetRecWaitMode(FALSE, 0);

				BOOL insert = FALSE;
				//チューナー固定でエラーのものは空き探さない
				if( item->useTunerID == 0 ){

					//まずそのまま入るところ
					for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
						if( itrBank->second->tunerID == itrCtrl->first ){
							//チェックの必要なし
							continue;
						}
						DWORD status = ChkInsertStatus(itrBank->second, item);
						if( status == 1 ){
							//問題なく追加可能
							itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(item->reserveID,item));
							insert = TRUE;
							item->preTunerID = itrBank->second->tunerID;
							addReserve.push_back(item);
							break;
						}
					}
					if( insert == FALSE ){
						//次に開始と終了重なるもの
						for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
							if( itrBank->second->tunerID == itrCtrl->first ){
								//チェックの必要なし
								continue;
							}
							DWORD status = ChkInsertStatus(itrBank->second, item);
							if( status == 2 ){
								//開始と終了同じだけど可能
								itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(item->reserveID,item));
								insert = TRUE;
								item->preTunerID = itrBank->second->tunerID;
								addReserve.push_back(item);
								break;
							}
						}
					}
				}
				if( insert == FALSE ){
					NGAddReserve.push_back(item);
				}
				itrCtrl->second->DeleteReserve(item->reserveID);
			}
			
			itrCtrl->second->ResetOpenErr();
			needNotify = TRUE;
		}
	}

	//別チューナーに移動できるもの移動
	for( size_t i=0; i<addReserve.size(); i++ ){
		itrCtrl = this->tunerBankMap.find(addReserve[i]->preTunerID);
		if( itrCtrl != this->tunerBankMap.end() ){
			vector<CReserveInfo*> list;
			list.push_back(addReserve[i]->reserveInfo);
			itrCtrl->second->AddReserve(&list);
		}
	}

	//移動できないものエラーとして削除
	if( NGAddReserve.size() > 0 ){
		BYTE suspendMode = 0xFF;
		BYTE rebootFlag = 0xFF;

		vector<DWORD> deleteList;
		for( size_t i=0; i<NGAddReserve.size(); i++ ){
			deleteList.push_back(NGAddReserve[i]->reserveID);

			RESERVE_DATA data;
			NGAddReserve[i]->reserveInfo->GetData(&data);
			REC_FILE_INFO item;
			item = data;
			item.recStatus = REC_END_STATUS_OPEN_ERR;
			item.comment = L"チューナーのオープンに失敗しました";
			this->recInfoText.AddRecInfo(&item);
			_SendTweet(TW_REC_END, &item, NULL, NULL);

			SAFE_DELETE(NGAddReserve[i]);

			suspendMode = data.recSetting.suspendMode;
			rebootFlag = data.recSetting.rebootFlag;
		}
		_DelReserveData(&deleteList);

		//情報ファイルの更新
		wstring filePath = L"";
		GetSettingPath(filePath);
		filePath += L"\\";
		filePath += RESERVE_TEXT_NAME;

		this->reserveText.SaveReserveText(filePath.c_str());

		wstring recFilePath = L"";
		GetSettingPath(recFilePath);
		recFilePath += L"\\";
		recFilePath += REC_INFO_TEXT_NAME;

		this->recInfoText.SaveRecInfoText(recFilePath.c_str());

		needNotify = TRUE;

		if( suspendMode != 0xFF && rebootFlag != 0xFF ){
			EnableSuspendWork(suspendMode, rebootFlag, 0);
		}
	}

	if( needNotify == TRUE ){
		_ReloadBankMap();
		_SendNotifyUpdate();
	}
}

void CReserveManager::CheckBatWork()
{
	if( this->batManager.GetWorkCount() > 0 ){
		if( this->batMargin != 0 ){
			map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
			for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
				if( itrCtrl->second->IsOpenTuner() == TRUE ){
					//起動中なので予約処理中
					this->batManager.PauseWork();
					return ;
				}
			}

			LONGLONG chkTime = GetNowI64Time() + this->batMargin*60*I64_1SEC;
			multimap<wstring, RESERVE_DATA*>::iterator itr;
			for( itr = this->reserveText.reserveMap.begin(); itr != this->reserveText.reserveMap.end(); itr++ ){
				if( itr->second->recSetting.recMode != RECMODE_VIEW && itr->second->recSetting.recMode != RECMODE_NO ){
					LONGLONG startTime = ConvertI64Time(itr->second->startTime);
					LONGLONG endTime = GetSumTime(itr->second->startTime, itr->second->durationSecond);
					if( itr->second->recSetting.useMargineFlag == 1 ){
						startTime += ((LONGLONG)itr->second->recSetting.startMargine)*I64_1SEC;
						endTime += ((LONGLONG)itr->second->recSetting.endMargine)*I64_1SEC;
					}else{
						startTime += ((LONGLONG)this->defStartMargine)*I64_1SEC;
						endTime += ((LONGLONG)this->defEndMargine)*I64_1SEC;
					}

					if( startTime <= chkTime && chkTime < endTime ){
						//次の予約時間にかぶる
						this->batManager.PauseWork();
						return;
					}
					break;
				}
			}

		}

		if( this->batManager.IsWorking() == FALSE ){
			this->batManager.StartWork();
		}
	}else{
		if( this->batManager.IsWorking() == FALSE ){
			BYTE suspendMode = 0;
			BYTE rebootFlag = 0;
			if( this->batManager.GetLastWorkSuspend(&suspendMode, &rebootFlag) == TRUE ){
				//バッチ処理終わったのでサスペンド処理に挑戦
				EnableSuspendWork(suspendMode, rebootFlag, 0);
			}
		}
	}
}

void CReserveManager::CheckTuijyu()
{
	//録画処理中のチューナー一覧
	map<DWORD, CTunerBankCtrl*> chkTuner;
	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		DWORD chID = 0;
		if( itrCtrl->second->GetCurrentChID(&chID) == TRUE ){
			chkTuner.insert(pair<DWORD, CTunerBankCtrl*>(chID, itrCtrl->second));
		}
	}

	//予約の状態を確認する
	BOOL chgReserve = FALSE;
	map<DWORD, CReserveInfo*>::iterator itrRes;
	BOOL chk6h = FALSE;
	if( this->reserveInfoMap.size() > 100 ){
		chk6h = TRUE;
	}
	for( itrRes = this->reserveInfoMap.begin(); itrRes != this->reserveInfoMap.end(); itrRes++ ){
		Sleep(0);
		RESERVE_DATA data;
		itrRes->second->GetData(&data);

		if( data.recSetting.recMode == RECMODE_NO ){
			continue;
		}
		if( data.eventID == 0xFFFF ){
			continue;
		}
		if( data.recSetting.tuijyuuFlag == 0 ){
			continue;
		}
		if( chk6h == TRUE ){
			if( ConvertI64Time(data.startTime) > GetNowI64Time() + 6*60*60*I64_1SEC ){
				continue;
			}
		}
	
		DWORD chkChID = ((DWORD)data.originalNetworkID)<<16 | data.transportStreamID;

		itrCtrl = chkTuner.find(chkChID);
		if( itrCtrl != chkTuner.end() ){
			BOOL chgRes = FALSE;
			BOOL recWaitFlag = FALSE;
			DWORD tunerID = 0;
			itrRes->second->GetRecWaitMode(&recWaitFlag, &tunerID);
			if( recWaitFlag == 0 ){
				//通常チェック
				SEARCH_EPG_INFO_PARAM val;
				val.ONID = data.originalNetworkID;
				val.TSID = data.transportStreamID;
				val.SID = data.serviceID;
				val.eventID = data.eventID;
				val.pfOnlyFlag = 0;
				EPGDB_EVENT_INFO resVal;
				if( itrCtrl->second->SearchEpgInfo(
					&val,
					&resVal
					) == TRUE ){
						chgRes = CheckChgEvent(&resVal, &data);
						if( chgRes == TRUE ){
							//開始時間6時間以内ならEPG再読み込みで変更されないようにする
							if( GetNowI64Time() + 6*60*60*I64_1SEC > ConvertI64Time(data.startTime) ){
								if( data.reserveStatus == ADD_RESERVE_NORMAL ){
									data.reserveStatus = ADD_RESERVE_CHG_PF2;
								}
							}
						}
				}else{
					OutputDebugString( L"番組情報みつからず ");
				}
				if( chgRes == TRUE ){
					_ChgReserveData( &data, TRUE );
					chgReserve = TRUE;
				}
			}else{
				//録画中
				GET_EPG_PF_INFO_PARAM valPF;
				valPF.ONID = data.originalNetworkID;
				valPF.TSID = data.transportStreamID;
				valPF.SID = data.serviceID;
				valPF.pfNextFlag = 0;

				EPGDB_EVENT_INFO resNowVal;
				EPGDB_EVENT_INFO resNextVal;
				BOOL nowSuccess = itrCtrl->second->GetEventPF( &valPF, &resNowVal );
				valPF.pfNextFlag = 1;
				BOOL nextSuccess = itrCtrl->second->GetEventPF( &valPF, &resNextVal );

				BOOL findPF = FALSE;
				BOOL endRec = FALSE;
				if( nowSuccess == TRUE ){
					if( resNowVal.event_id == data.eventID ){
						findPF = TRUE;
						if( itrRes->second->IsChkPfInfo() == FALSE ){
							itrRes->second->SetChkPfInfo(TRUE);
						}
						if( resNowVal.StartTimeFlag == 1 && resNowVal.DurationFlag == 1 ){
							//通常チェック
							BYTE chgMode = 0;
							chgRes = CheckChgEvent(&resNowVal, &data, &chgMode);
							if( chgRes == TRUE && (chgMode&0x02) == 0x02 ){
								//総時間変更された
								data.durationSecond += (DWORD)this->duraChgMarginMin*60;
								//同一サービスで録画中になっているやつの開始時間変更してやる
								ChgDurationChk(&resNowVal);
							}
						}else{
							//時間未定
							if( resNowVal.StartTimeFlag == 1 ){
								data.startTime = resNowVal.start_time;
							}
							LONGLONG dureSec = GetNowI64Time() - ConvertI64Time(data.startTime) + 10*60*I64_1SEC;
							if( (DWORD)(dureSec/I64_1SEC) > data.durationSecond ){
								data.durationSecond = (DWORD)(dureSec/I64_1SEC);
								data.reserveStatus = ADD_RESERVE_UNKNOWN_END;
								chgRes = TRUE;
							}
							_OutputDebugString(L"●p/f 時間未定現在情報に存在 %d/%d/%d %d:%d:%d %dsec %s %s\r\n",
								data.startTime.wYear,
								data.startTime.wMonth,
								data.startTime.wDay,
								data.startTime.wHour,
								data.startTime.wMinute,
								data.startTime.wSecond,
								data.durationSecond,
								data.title.c_str(),
								data.stationName.c_str()
								);
						}
					}
				}
				if( nextSuccess == TRUE ){
					if( resNextVal.event_id == data.eventID ){
						findPF = TRUE;
						if( resNextVal.StartTimeFlag == 1 && resNextVal.DurationFlag == 1 ){
							//通常チェック
							chgRes = CheckChgEvent(&resNextVal, &data);
						}else{
							//総時間未定
							if( resNextVal.StartTimeFlag == 1 && resNowVal.StartTimeFlag == 1 ){
								if( ConvertI64Time(resNextVal.start_time) < ConvertI64Time(resNowVal.start_time) ){
									//次の方が開始時間早いとかおかしい
									endRec = TRUE;
									_OutputDebugString(L"●p/f 不正状態（現在開始＞次開始 %d/%d/%d %d:%d:%d %dsec %s %s\r\n",
										data.startTime.wYear,
										data.startTime.wMonth,
										data.startTime.wDay,
										data.startTime.wHour,
										data.startTime.wMinute,
										data.startTime.wSecond,
										data.durationSecond,
										data.title.c_str(),
										data.stationName.c_str()
										);
									if( CheckEventRelay(&resNextVal, &data, TRUE) == TRUE ){
										chgReserve = TRUE;
									}
								}
							}
							if( endRec == FALSE ){
								if( resNextVal.StartTimeFlag == 1 ){
									data.startTime = resNextVal.start_time;
								}
								LONGLONG dureSec = GetNowI64Time() - ConvertI64Time(data.startTime) + 10*60*I64_1SEC;
								if( (DWORD)(dureSec/I64_1SEC) > data.durationSecond ){
									data.durationSecond = (DWORD)(dureSec/I64_1SEC);
									data.reserveStatus = ADD_RESERVE_UNKNOWN_END;
									chgRes = TRUE;
								}
								_OutputDebugString(L"●p/f 次情報に存在 %d/%d/%d %d:%d:%d %dsec %s %s\r\n",
									data.startTime.wYear,
									data.startTime.wMonth,
									data.startTime.wDay,
									data.startTime.wHour,
									data.startTime.wMinute,
									data.startTime.wSecond,
									data.durationSecond,
									data.title.c_str(),
									data.stationName.c_str()
									);
							}
						}
					}
				}
				if( findPF == FALSE ){
					//現在or次ではない
					BOOL chkNormal = TRUE;
					if( nowSuccess == TRUE){
						if(resNowVal.StartTimeFlag != 1 || resNowVal.DurationFlag != 1){
							//時間未定なので6時間追従モードへ
							chkNormal = FALSE;
						}
					}
					if( nextSuccess == TRUE){
						if( resNextVal.StartTimeFlag != 1 || resNextVal.DurationFlag != 1 ){
							//時間未定なので6時間追従モードへ
							chkNormal = FALSE;
						}
					}
					if( chkNormal == FALSE ){
						//時間未定なので6時間追従モードへ
						if( CheckNotFindChgEvent(&data, itrCtrl->second) == TRUE ){
							chgReserve = TRUE;
						}
					}else{
						//p/f正常なので通常検索
						SEARCH_EPG_INFO_PARAM val;
						val.ONID = data.originalNetworkID;
						val.TSID = data.transportStreamID;
						val.SID = data.serviceID;
						val.eventID = data.eventID;
						val.pfOnlyFlag = 0;
						EPGDB_EVENT_INFO resVal;
						if( itrCtrl->second->SearchEpgInfo(
							&val,
							&resVal
							) == TRUE ){
								if( data.reserveStatus == ADD_RESERVE_NO_FIND ){
									if( resVal.StartTimeFlag == 1 ){
										if( ConvertI64Time(resVal.start_time) > ConvertI64Time(data.startTime) ){
											//開始時間後なので更新されたEPGのはず
											chgRes = CheckChgEvent(&resVal, &data);
										}else{
											//古いEPGなので何もしない
										}
									}
								}else{
									chgRes = CheckChgEvent(&resVal, &data);
								}
						}else{
							_OutputDebugString(L"●番組情報みつからず %d/%d/%d %d:%d:%d %dsec %s %s\r\n",
								data.startTime.wYear,
								data.startTime.wMonth,
								data.startTime.wDay,
								data.startTime.wHour,
								data.startTime.wMinute,
								data.startTime.wSecond,
								data.durationSecond,
								data.title.c_str(),
								data.stationName.c_str()
								);
							if( nowSuccess == FALSE && nextSuccess == FALSE ){
								if( data.reserveStatus == ADD_RESERVE_NORMAL ){
									LONGLONG delay = itrCtrl->second->DelayTime();
									LONGLONG nowTime = GetNowI64Time() + delay;
									LONGLONG endTime = GetSumTime(data.startTime, data.durationSecond);
									if( data.recSetting.useMargineFlag == 1 ){
										if( data.recSetting.endMargine < 0 ){
											endTime -= ((LONGLONG)data.recSetting.endMargine)*I64_1SEC;
										}
									}
									if( nowTime + 2*60*I64_1SEC > endTime ){
										//終了2分前だけどEPGなし
										data.reserveStatus = ADD_RESERVE_NO_EPG;
										data.durationSecond += this->noEpgTuijyuMin * 60;
										chgRes = TRUE;
									}
								}
							}
						}
					}
				}
				if( endRec == TRUE ){
					//誤差も加味
					LONGLONG delay = itrCtrl->second->DelayTime();
					chgRes = TRUE;
					LONGLONG dureSec = GetNowI64Time()+delay - ConvertI64Time(data.startTime);
					data.durationSecond = (DWORD)(dureSec/I64_1SEC);
					//録画時間過ぎている状態作るために終了マージンを-1分にしてやる
					data.recSetting.useMargineFlag = 1;
					data.recSetting.startMargine = 0;
					data.recSetting.endMargine = -60;
					_OutputDebugString(L"●情報確認できず終了 %d/%d/%d %d:%d:%d %dsec %s %s\r\n",
						data.startTime.wYear,
						data.startTime.wMonth,
						data.startTime.wDay,
						data.startTime.wHour,
						data.startTime.wMinute,
						data.startTime.wSecond,
						data.durationSecond,
						data.title.c_str(),
						data.stationName.c_str()
						);
				}
				if( chgRes == TRUE ){
					if( data.reserveStatus == ADD_RESERVE_NORMAL || data.reserveStatus == ADD_RESERVE_CHG_PF2 ){
						data.reserveStatus = ADD_RESERVE_CHG_PF;
					}
					_ChgReserveData( &data, TRUE );
					chgReserve = TRUE;
				}

				if( this->eventRelay == TRUE ){
					//イベントリレーのチェック
					if( nowSuccess == TRUE){
						if( CheckEventRelay(&resNowVal, &data) == TRUE ){
							chgReserve = TRUE;
						}
					}
				}
			}
		}
	}

	if( chgReserve == TRUE ){
		wstring filePath = L"";
		GetSettingPath(filePath);
		filePath += L"\\";
		filePath += RESERVE_TEXT_NAME;

		this->reserveText.SaveReserveText(filePath.c_str());

		_ReloadBankMap();

		_SendNotifyUpdate();
	}
}

BOOL CReserveManager::CheckChgEvent(EPGDB_EVENT_INFO* info, RESERVE_DATA* data, BYTE* chgMode)
{
	BOOL chgRes = FALSE;

	RESERVE_DATA oldData = *data;

	wstring log = L"";
	wstring timeLog1 = L"";
	wstring timeLog2 = L"";

	SYSTEMTIME oldEndTime;
	GetSumTime(data->startTime, data->durationSecond, &oldEndTime);
	Format(timeLog1, L"%d/%d/%d %d:%d:%d～%d:%d:%d",
		data->startTime.wYear,
		data->startTime.wMonth,
		data->startTime.wDay,
		data->startTime.wHour,
		data->startTime.wMinute,
		data->startTime.wSecond,
		oldEndTime.wHour,
		oldEndTime.wMinute,
		oldEndTime.wSecond);

	SYSTEMTIME endTime;
	if( info->StartTimeFlag == 1 && info->DurationFlag == 1){
		GetSumTime(info->start_time, info->durationSec, &endTime);
		Format(timeLog2, L"%d/%d/%d %d:%d:%d～%d:%d:%d",
			info->start_time.wYear,
			info->start_time.wMonth,
			info->start_time.wDay,
			info->start_time.wHour,
			info->start_time.wMinute,
			info->start_time.wSecond,
			endTime.wHour,
			endTime.wMinute,
			endTime.wSecond);
	}else if( info->StartTimeFlag == 1 && info->DurationFlag == 0){
		Format(timeLog2, L"%d/%d/%d %d:%d:%d～未定",
			info->start_time.wYear,
			info->start_time.wMonth,
			info->start_time.wDay,
			info->start_time.wHour,
			info->start_time.wMinute,
			info->start_time.wSecond);
	}else{
		timeLog2 = L"時間未定";
	}

	if( info->StartTimeFlag == 1 ){
		if( ConvertI64Time(data->startTime) != ConvertI64Time(info->start_time) ){
			//開始時間変わっている
			chgRes = TRUE;
			data->startTime = info->start_time;

			log += L"●追従：開始変更 ";
			if( chgMode != NULL ){
				*chgMode |= 0x01;
			}
		}
	}
	if( info->DurationFlag == 1 ){
		if( data->reserveStatus == ADD_RESERVE_CHG_PF ){
			//一度変わってるのでマージン追加されてるはず
			if( data->durationSecond - ((DWORD)this->duraChgMarginMin*60) != info->durationSec ){
				//総時間が変更されている
				chgRes = TRUE;
				data->durationSecond = info->durationSec;
				log += L"●追従：総時間変更 ";
				if( chgMode != NULL ){
					*chgMode |= 0x02;
				}
			}
		}else{
			if( data->durationSecond != info->durationSec ){
				//総時間が変更されている
				chgRes = TRUE;
				data->durationSecond = info->durationSec;
				log += L"●追従：総時間変更 ";
				if( chgMode != NULL ){
					*chgMode |= 0x02;
				}
			}
		}
	}

	if( chgRes == TRUE ){
		log += data->stationName;
		log += L" ";
		log += timeLog1;
		log += L" → ";
		log += timeLog2;
		log += L" ";
		log += data->title;
		log += L"\r\n";
		_SendTweet(TW_CHG_RESERVE_CHK_REC, &oldData, data, info);
	}else{
		log += data->stationName;
		log += L" ";
		log += timeLog2;
		log += L" ";
		log += data->title;
		log += L"\r\n";
	}

	OutputDebugString(log.c_str());
	return chgRes;
}

BOOL CReserveManager::CheckNotFindChgEvent(RESERVE_DATA* data, CTunerBankCtrl* ctrl)
{
	BOOL chgRes = FALSE;
	wstring log = L"";

	LONGLONG delay = ctrl->DelayTime();
	LONGLONG nowTime = GetNowI64Time()+delay;

	if( data->reserveStatus == ADD_RESERVE_RELAY ){
		//イベントリレーは追従の必要なし
	}else if(data->reserveStatus == ADD_RESERVE_NORMAL || data->reserveStatus == ADD_RESERVE_CHG_PF){
		//6時間追従用予約へ移行
		LONGLONG chkEndTime = 0;
		LONGLONG endMargine = 0;
		if( data->recSetting.useMargineFlag == 1 ){
			endMargine = ((LONGLONG)data->recSetting.endMargine)*I64_1SEC;
		}else{
			endMargine = ((LONGLONG)this->defEndMargine)*I64_1SEC;
		}
		chkEndTime = GetSumTime(data->startTime, data->durationSecond) + endMargine;
		LONGLONG delay = ctrl->DelayTime();
		LONGLONG nowTime = GetNowI64Time()+delay;
		//if( nowTime + 60*I64_1SEC > chkEndTime ){
			//終了1分前で番組情報みつからず
			OutputDebugString(L"●6時間追従用予約へ移行");

			//開始時間を延ばす
			LONGLONG chgStart = nowTime;
			if( data->recSetting.useMargineFlag == 1 ){
				if( data->recSetting.startMargine < 0 ){
					chgStart += ((LONGLONG)data->recSetting.startMargine)*I64_1SEC;
				}
			}else{
				if( this->defStartMargine < 0 ){
					chgStart += ((LONGLONG)this->defStartMargine)*I64_1SEC;
				}
			}
			chgStart -= 30*I64_1SEC;
			ConvertSystemTime( chgStart, &data->startTime);
			data->reserveStatus = ADD_RESERVE_NO_FIND;
			_ChgReserveData( data, TRUE );
			chgRes = TRUE;

			ctrl->ReRec(data->reserveID, FALSE);
		//}
	}else if(data->reserveStatus == ADD_RESERVE_NO_FIND){
		if( ConvertI64Time(data->startTimeEpg) + ((LONGLONG)this->notFindTuijyuHour)*60*60*I64_1SEC < nowTime ){
			OutputDebugString(L"●指定時間番組情報みつからず");
			REC_FILE_INFO item;
			item = *data;
			item.recFilePath = L"";
			item.drops = 0;
			item.scrambles = 0;
			item.recStatus = REC_END_STATUS_NOT_FIND_6H;
			item.comment = L"指定時間番組情報が見つかりませんでした";
			this->recInfoText.AddRecInfo(&item);
			_SendTweet(TW_REC_END, &item, NULL, NULL);

			vector<DWORD> deleteList;
			deleteList.push_back(data->reserveID);
			//予約一覧から削除
			_DelReserveData(&deleteList);
		}else{
			//開始時間を延ばす
			LONGLONG chgStart = nowTime;
			if( data->recSetting.useMargineFlag == 1 ){
				if( data->recSetting.startMargine < 0 ){
					chgStart += ((LONGLONG)data->recSetting.startMargine)*I64_1SEC;
				}
			}else{
				if( this->defStartMargine < 0 ){
					chgStart += ((LONGLONG)this->defStartMargine)*I64_1SEC;
				}
			}
			chgStart -= 30*I64_1SEC;
			ConvertSystemTime( chgStart, &data->startTime);
			_ChgReserveData( data, TRUE );

			ctrl->ReRec(data->reserveID, TRUE);
		}
		chgRes = TRUE;
	}

	return chgRes;
}

BOOL CReserveManager::CheckEventRelay(EPGDB_EVENT_INFO* info, RESERVE_DATA* data, BOOL errEnd)
{
	BOOL add = FALSE;
	if( info == NULL ){
		return add;
	}
	if( info->original_network_id != data->originalNetworkID ||
		info->transport_stream_id != data->transportStreamID ||
		info->service_id != data->serviceID ||
		info->event_id != data->eventID ){
			//イベントIDの確認
			return add;
	}
	if( info->eventRelayInfo != NULL ){
		if( errEnd == TRUE ){
			OutputDebugString(L"イベントリレーチェック　総時間異常終了");
		}else{
			if( info->StartTimeFlag == 0 || info->DurationFlag == 0 ){
				OutputDebugString(L"イベントリレーチェック　開始 or 総時間未定");
				return add;
			}
		}
		//イベントリレーあり
		for( size_t i=0; info->eventRelayInfo->eventDataList.size(); i++ ){
			LONGLONG chKey = _Create64Key(
				info->eventRelayInfo->eventDataList[i].original_network_id,
				info->eventRelayInfo->eventDataList[i].transport_stream_id,
				info->eventRelayInfo->eventDataList[i].service_id);

			map<LONGLONG, CH_DATA5>::iterator itrCh;
			itrCh = this->chUtil.chList.find(chKey);
			if( itrCh != this->chUtil.chList.end() ){
				//使用できるチャンネル発見

				//同一イベント予約済みかチェック
				BOOL find = FALSE;
				multimap<wstring, RESERVE_DATA*>::iterator itrRes;
				for( itrRes = this->reserveText.reserveMap.begin(); itrRes != this->reserveText.reserveMap.end(); itrRes++ ){
					if( itrRes->second->originalNetworkID == info->eventRelayInfo->eventDataList[i].original_network_id &&
						itrRes->second->transportStreamID == info->eventRelayInfo->eventDataList[i].transport_stream_id &&
						itrRes->second->serviceID == info->eventRelayInfo->eventDataList[i].service_id &&
						itrRes->second->eventID == info->eventRelayInfo->eventDataList[i].event_id ){
							//予約済み
							find = TRUE;
							//追加済みなら異常終了のために開始時間変更する必要なし
							if( errEnd == FALSE ){
								//時間変更必要かチェック
								SYSTEMTIME chkStart;
								GetSumTime(info->start_time, info->durationSec, &chkStart);
								if( ConvertI64Time(itrRes->second->startTime) != ConvertI64Time(chkStart) ){
									RESERVE_DATA chgData = *(itrRes->second);
									//開始時間変わっている
									add = TRUE;
									chgData.startTime = chkStart;
									chgData.startTimeEpg = chgData.startTime;
									_ChgReserveData( &chgData, TRUE );
									OutputDebugString(L"★イベントリレー開始変更");
								}
							}
							break;
					}
				}
				if( find == FALSE ){
					RESERVE_DATA addItem;
					if( data->title.find(L"(イベントリレー)") == string::npos ){
						addItem.title = L"(イベントリレー)";
					}
					addItem.title += data->title;
					if( errEnd == TRUE ){
						GetLocalTime(&addItem.startTime);
					}else{
						GetSumTime(info->start_time, info->durationSec, &addItem.startTime);
					}
					addItem.startTimeEpg = addItem.startTime;
					addItem.durationSecond = 10*60;
					addItem.stationName = itrCh->second.serviceName;
					addItem.originalNetworkID = info->eventRelayInfo->eventDataList[i].original_network_id;
					addItem.transportStreamID = info->eventRelayInfo->eventDataList[i].transport_stream_id;
					addItem.serviceID = info->eventRelayInfo->eventDataList[i].service_id;
					addItem.eventID = info->eventRelayInfo->eventDataList[i].event_id;

					addItem.recSetting = data->recSetting;
					addItem.reserveStatus = ADD_RESERVE_RELAY;
					_AddReserveData(&addItem);
					add = TRUE;
					OutputDebugString(L"★イベントリレー追加");
				}
				break;
			}
		}
	}
	return add;
}

BOOL CReserveManager::ChgDurationChk(EPGDB_EVENT_INFO* info)
{
	if( info == NULL ){
		return FALSE;
	}
	if( info->StartTimeFlag == 0 || info->DurationFlag == 0 ){
		return FALSE;
	}
	BOOL ret = FALSE;
	map<DWORD, CReserveInfo*>::iterator itrRes;
	for( itrRes = this->reserveInfoMap.begin(); itrRes != this->reserveInfoMap.end(); itrRes++ ){
		RESERVE_DATA data;
		itrRes->second->GetData(&data);

		if( data.recSetting.recMode == RECMODE_NO ){
			continue;
		}
		if( data.eventID == 0xFFFF ){
			continue;
		}
		if( data.recSetting.tuijyuuFlag == 0 ){
			continue;
		}
		BOOL recWaitFlag = FALSE;
		DWORD tunerID = 0;
		itrRes->second->GetRecWaitMode(&recWaitFlag, &tunerID);
		if( recWaitFlag == 0 ){
			continue;
		}
		//p/f確認できてるのに変更はおかしい
		if(itrRes->second->IsChkPfInfo() == TRUE){
			continue;
		}
		//未定で終わりかけのやつ除外
		if( data.reserveStatus == ADD_RESERVE_UNKNOWN_END ){
			continue;
		}

		if( data.originalNetworkID == info->original_network_id &&
			data.transportStreamID == info->transport_stream_id &&
			data.serviceID == info->service_id &&
			data.eventID != info->event_id ){
				if( ConvertI64Time(data.startTime) != GetSumTime(info->start_time, info->durationSec) ){
					//開始時間が違うので変更してやる
					GetSumTime(info->start_time, info->durationSec, &data.startTime);
					if( data.reserveStatus == ADD_RESERVE_NORMAL ){
						data.reserveStatus = ADD_RESERVE_CHG_PF2;
					}

					_ChgReserveData( &data, TRUE );
					ret = TRUE;
				}
		}
	}
	return ret;
}

void CReserveManager::EnableSuspendWork(BYTE suspendMode, BYTE rebootFlag, BYTE epgReload)
{
	BYTE setSuspendMode = suspendMode;
	BYTE setRebootFlag = rebootFlag;

	if( suspendMode == 0 ){
		setSuspendMode = this->defSuspendMode;
		setRebootFlag = this->defRebootFlag;
	}

	if( _IsSuspendOK(setRebootFlag) == TRUE ){
		this->enableSetSuspendMode = setSuspendMode;
		this->enableSetRebootFlag = setRebootFlag;
	}else{
		this->enableSetSuspendMode = 0xFF;
		this->enableSetRebootFlag = 0xFF;
		this->enableEpgReload = epgReload;
	}
}

BOOL CReserveManager::IsEnableSuspend(
	BYTE* suspendMode,
	BYTE* rebootFlag
	)
{
	if( Lock(L"IsEnableSuspend") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	if( this->enableSetSuspendMode == 0xFF && this->enableSetRebootFlag == 0xFF ){
		ret = FALSE;
	}else{
		if( this->twitterManager.GetTweetQue() == 0){
			*suspendMode = this->enableSetSuspendMode;
			*rebootFlag = this->enableSetRebootFlag;

			this->enableSetSuspendMode = 0xFF;
			this->enableSetRebootFlag = 0xFF;
		}else{
			ret = FALSE;
		}
	}

	UnLock();
	return ret;
}

BOOL CReserveManager::IsEnableReloadEPG(
	)
{
	if( Lock(L"IsEnableReloadEPG") == FALSE ) return FALSE;
	BOOL ret = FALSE;
	if( this->enableEpgReload == 1 ){
		ret = TRUE;
		this->enableEpgReload = 0;
	}
	UnLock();
	return ret;
}

BOOL CReserveManager::IsSuspendOK()
{
	if( Lock() == FALSE ) return FALSE;
	BOOL ret = _IsSuspendOK(FALSE);
	UnLock();
	return ret;
}

BOOL CReserveManager::_IsSuspendOK(BOOL rebootFlag)
{
	BOOL ret = TRUE;

	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		if( itrCtrl->second->IsSuspendOK() == FALSE ){
			//起動中なので予約処理中
			OutputDebugString(L"_IsSuspendOK tunerUse");
			return FALSE;
		}
	}

	LONGLONG wakeMargin = this->wakeTime;
	if( rebootFlag == TRUE ){
		wakeMargin += 5;
	}
	LONGLONG chkNoStandbyTime = GetNowI64Time() + ((LONGLONG)this->noStandbyTime)*60*I64_1SEC;

	LONGLONG chkWakeTime = GetNowI64Time() + wakeMargin*60*I64_1SEC;
	/*
	multimap<wstring, RESERVE_DATA*>::iterator itr;
	for( itr = this->reserveText.reserveMap.begin(); itr != this->reserveText.reserveMap.end(); itr++ ){
		if( itr->second->recSetting.recMode != RECMODE_VIEW && itr->second->recSetting.recMode != RECMODE_NO ){
			LONGLONG startTime = ConvertI64Time(itr->second->startTime);
			DWORD sec = itr->second->durationSecond;
			if( sec < (DWORD)wakeMargin*60 ){
				sec = (DWORD)wakeMargin*60;
			}
			LONGLONG endTime = GetSumTime(itr->second->startTime, sec);
			if( itr->second->recSetting.useMargineFlag == 1 ){
				startTime += ((LONGLONG)itr->second->recSetting.startMargine)*I64_1SEC;
				endTime += ((LONGLONG)itr->second->recSetting.endMargine)*I64_1SEC;
			}else{
				startTime += ((LONGLONG)this->defStartMargine)*I64_1SEC;
				endTime += ((LONGLONG)this->defEndMargine)*I64_1SEC;
			}

			if( startTime <= chkWakeTime && chkWakeTime < endTime ){
				OutputDebugString(L"_IsSuspendOK chkWakeTime");
				//次の予約時間にかぶる
				return FALSE;
			}
			break;
		}
	}
	*/
	//ソートされてないので全部回す必要あり
	map<DWORD, CReserveInfo*>::iterator itr;
	for( itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		RESERVE_DATA data;
		itr->second->GetData(&data);
		if( data.overlapMode == 0 ){
			if( data.recSetting.recMode != RECMODE_NO ){
				LONGLONG startTime = ConvertI64Time(data.startTime);
				DWORD sec = data.durationSecond;
				if( sec < (DWORD)wakeMargin*60 ){
					sec = (DWORD)wakeMargin*60;
				}
				LONGLONG endTime = GetSumTime(data.startTime, sec);
				if( data.recSetting.useMargineFlag == 1 ){
					startTime += ((LONGLONG)data.recSetting.startMargine)*I64_1SEC;
					endTime += ((LONGLONG)data.recSetting.endMargine)*I64_1SEC;
				}else{
					startTime += ((LONGLONG)this->defStartMargine)*I64_1SEC;
					endTime += ((LONGLONG)this->defEndMargine)*I64_1SEC;
				}

				if( startTime <= chkWakeTime && chkWakeTime < endTime ){
					OutputDebugString(L"_IsSuspendOK chkWakeTime");
					//次の予約時間にかぶる
					return FALSE;
				}
				if( startTime <= chkNoStandbyTime ){
					OutputDebugString(L"_IsSuspendOK chkNoStandbyTime");
					//次の予約時間にかぶる
					return FALSE;
				}

			}
		}
	}

	LONGLONG epgcapTime = 0;
	if( GetNextEpgcapTime(&epgcapTime, 0) == TRUE ){
		if( epgcapTime < chkWakeTime ){
			//EPG取得
			OutputDebugString(L"_IsSuspendOK EpgCapTime");
			return FALSE;
		}
	}

	if( this->batManager.IsWorking() == TRUE ){
		//バッチ処理中
		OutputDebugString(L"_IsSuspendOK IsBatWorking");
		return FALSE;
	}

	if( IsFindNoSuspendExe() == TRUE ){
		//バッチ処理中
		OutputDebugString(L"IsFindNoSuspendExe");
		return FALSE;
	}

	return ret;
}

BOOL CReserveManager::IsFindNoSuspendExe()
{
	OSVERSIONINFO VerInfo;
	VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx( &VerInfo );

	BOOL b2000 = FALSE;
	if( VerInfo.dwMajorVersion == 5 && VerInfo.dwMinorVersion == 0 ){
		b2000 = TRUE;
	}else{
		b2000 = FALSE;
	}

	HANDLE hSnapshot;
	PROCESSENTRY32 procent;

	BOOL bFind = FALSE;
	/* Toolhelpスナップショットを作成する */
	hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS,0 );
	if ( hSnapshot != (HANDLE)-1 ) {
		procent.dwSize = sizeof(PROCESSENTRY32);
		if ( Process32First( hSnapshot,&procent ) != FALSE )            {
			do {
				/* procent.szExeFileにプロセス名 */
				wstring strExe = procent.szExeFile;
				std::transform(strExe.begin(), strExe.end(), strExe.begin(), tolower);
				for( size_t i=0; i<this->noStandbyExeList.size(); i++ ){
					if( b2000 == TRUE ){
						if( strExe.find( this->noStandbyExeList[i].substr(0, 15).c_str()) == 0 ){
							bFind = TRUE;
							break;
						}
					}else{
						if( strExe.find( this->noStandbyExeList[i].c_str()) == 0 ){
							bFind = TRUE;
							break;
						}
					}
				}
				if( bFind == TRUE ){
					break;
				}
			} while ( Process32Next( hSnapshot,&procent ) != FALSE );
		}
		CloseHandle( hSnapshot );
	}
	return bFind;
}

BOOL CReserveManager::GetSleepReturnTime(
	LONGLONG* returnTime
	)
{
	if( returnTime == NULL ){
		return FALSE;
	}
	LONGLONG nextRec = 0;
	multimap<wstring, RESERVE_DATA*>::iterator itr;
	for( itr = this->reserveText.reserveMap.begin(); itr != this->reserveText.reserveMap.end(); itr++ ){
		if( itr->second->recSetting.recMode != RECMODE_NO ){
			LONGLONG startTime = ConvertI64Time(itr->second->startTime);
			LONGLONG endTime = GetSumTime(itr->second->startTime, itr->second->durationSecond);
			if( itr->second->recSetting.useMargineFlag == 1 ){
				startTime -= ((LONGLONG)itr->second->recSetting.startMargine)*I64_1SEC;
			}else{
				startTime -= ((LONGLONG)this->defStartMargine)*I64_1SEC;
			}

			nextRec = startTime;

			break;
		}
	}

	LONGLONG epgcapTime = 0;
	GetNextEpgcapTime(&epgcapTime, 0);


	if( nextRec == 0 && epgcapTime == 0 ){
		return FALSE;
	}else if( nextRec == 0 && epgcapTime != 0 ){
		*returnTime = epgcapTime;
	}else if( nextRec != 0 && epgcapTime == 0 ){
		*returnTime = nextRec;
	}else{
		if(nextRec < epgcapTime){
			*returnTime = nextRec;
		}else{
			*returnTime = epgcapTime;
		}
	}
	return TRUE;
}

BOOL CReserveManager::GetNextEpgcapTime(LONGLONG* capTime, LONGLONG chkMargineMin)
{
	if( capTime == NULL ){
		return FALSE;
	}

	wstring iniPath = L"";
	GetModuleIniPath(iniPath);

	SYSTEMTIME srcTime;
	GetLocalTime(&srcTime);
	srcTime.wHour = 0;
	srcTime.wMinute = 0;
	srcTime.wSecond = 0;
	srcTime.wHour = 0;

	map<LONGLONG,LONGLONG> timeList;

	for( size_t i=0; i<this->epgCapTimeList.size(); i++ ){
		LONGLONG chkTime = GetSumTime(srcTime, this->epgCapTimeList[i]);
		timeList.insert(pair<LONGLONG,LONGLONG>(chkTime,chkTime));
		chkTime = GetSumTime(srcTime, this->epgCapTimeList[i] + 24*60*60);
		timeList.insert(pair<LONGLONG,LONGLONG>(chkTime,chkTime));
	}

	if( timeList.size() == 0 ){
		return FALSE;
	}

	LONGLONG nowTime = GetNowI64Time() + (chkMargineMin*60*I64_1SEC);
	map<LONGLONG,LONGLONG>::iterator itr;
	for( itr = timeList.begin(); itr != timeList.end(); itr++){
		if( nowTime < itr->first ){
			*capTime = itr->first;
			break;
		}
	}

	return TRUE;
}

//録画済み情報一覧を取得する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// infoList			[OUT]録画済み情報一覧
BOOL CReserveManager::GetRecFileInfoAll(
	vector<REC_FILE_INFO>* infoList
	)
{
	if( Lock(L"GetRecFileInfoAll") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	multimap<wstring, REC_FILE_INFO*>::iterator itr;
	for( itr = this->recInfoText.recInfoMap.begin(); itr != this->recInfoText.recInfoMap.end(); itr++ ){
		REC_FILE_INFO item;
		item = *(itr->second);
		infoList->push_back(item);
	}

	UnLock();
	return ret;
}

//録画済み情報を削除する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// idList			[IN]IDリスト
BOOL CReserveManager::DelRecFileInfo(
	vector<DWORD>* idList
	)
{
	if( Lock(L"DelRecFileInfo") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	for( size_t i=0 ;i<idList->size(); i++){
		this->recInfoText.DelRecInfo((*idList)[i]);
	}

	wstring filePath = L"";
	GetSettingPath(filePath);
	filePath += L"\\";
	filePath += REC_INFO_TEXT_NAME;

	this->recInfoText.SaveRecInfoText(filePath.c_str());

	_SendNotifyUpdate();

	UnLock();
	return ret;
}

BOOL CReserveManager::StartEpgCap()
{
	if( Lock(L"StartEpgCap") == FALSE ) return FALSE;

	BOOL ret = _StartEpgCap();

	UnLock();
	return ret;
}

BOOL CReserveManager::_StartEpgCap()
{
	BOOL ret = TRUE;

	wstring chSet5Path = L"";
	GetSettingPath(chSet5Path);
	chSet5Path += L"\\ChSet5.txt";

	CParseChText5 chSet5;
	chSet5.ParseText(chSet5Path.c_str());

	//まず取得対象となるサービスの一覧を抽出
	map<DWORD, CH_DATA5> serviceList;
	map<LONGLONG, CH_DATA5>::iterator itrCh;
	for( itrCh = chSet5.chList.begin(); itrCh != chSet5.chList.end(); itrCh++ ){
		if( itrCh->second.epgCapFlag == TRUE ){
			DWORD key = ((DWORD)itrCh->second.originalNetworkID)<<16 | itrCh->second.transportStreamID;
			map<DWORD, CH_DATA5>::iterator itrIn;
			itrIn = serviceList.find(key);
			if( itrIn == serviceList.end() ){
				serviceList.insert(pair<DWORD, CH_DATA5>(key, itrCh->second));
			}
		}
	}

	//利用可能なチューナーの抽出
	vector<DWORD> tunerIDList;
	this->tunerManager.GetEnumEpgCapTuner(&tunerIDList);

	map<DWORD, CTunerBankCtrl*> epgCapCtrl;
	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( size_t i=0; i<tunerIDList.size(); i++ ){
		itrCtrl = this->tunerBankMap.find(tunerIDList[i]);
		if( itrCtrl != this->tunerBankMap.end() ){
			if( itrCtrl->second->IsEpgCapWorking() == FALSE ){
				itrCtrl->second->ClearEpgCapItem();
				if( ngCapMin != 0 ){
					if( itrCtrl->second->IsEpgCapOK(this->ngCapMin) == FALSE ){
						//実行しちゃいけない
						return FALSE;
					}
				}
				if( itrCtrl->second->IsEpgCapOK(this->ngCapTunerMin) == TRUE ){
					//使えるチューナー
					epgCapCtrl.insert(pair<DWORD, CTunerBankCtrl*>(itrCtrl->first, itrCtrl->second));
				}
			}
		}
	}
	if( epgCapCtrl.size() == 0 ){
		//実行できるものない
		return FALSE;
	}

	BOOL inBS = FALSE;
	BOOL inCS1 = FALSE;
	BOOL inCS2 = FALSE;
	//各チューナーに振り分け
	itrCtrl = epgCapCtrl.begin();
	map<DWORD, CH_DATA5>::iterator itrAdd;
	for( itrAdd = serviceList.begin(); itrAdd != serviceList.end(); itrAdd++ ){
		if( itrAdd->second.originalNetworkID == 4 && this->BSOnly == TRUE ){
			if( inBS == TRUE ){
				continue;
			}
		}
		if( itrAdd->second.originalNetworkID == 6 && this->CS1Only == TRUE ){
			if( inCS1 == TRUE ){
				continue;
			}
		}
		if( itrAdd->second.originalNetworkID == 7 && this->CS2Only == TRUE ){
			if( inCS2 == TRUE ){
				continue;
			}
		}
		DWORD startID = itrCtrl->first;
		BOOL add = FALSE;
		do{
			if( this->tunerManager.IsSupportService(
				itrCtrl->first,
				itrAdd->second.originalNetworkID,
				itrAdd->second.transportStreamID,
				itrAdd->second.serviceID
				) == TRUE ){
					SET_CH_INFO addItem;
					addItem.ONID = itrAdd->second.originalNetworkID;
					addItem.TSID = itrAdd->second.transportStreamID;
					addItem.SID = itrAdd->second.serviceID;
					addItem.useSID = TRUE;
					addItem.useBonCh = FALSE;
					itrCtrl->second->AddEpgCapItem(addItem);

					add = TRUE;

					if(itrAdd->second.originalNetworkID == 4){
						inBS = TRUE;
					}
					if(itrAdd->second.originalNetworkID == 6){
						inCS1 = TRUE;
					}
					if(itrAdd->second.originalNetworkID == 7){
						inCS2 = TRUE;
					}
			}

			itrCtrl++;
			if( itrCtrl == epgCapCtrl.end() ){
				itrCtrl = epgCapCtrl.begin();
			}
		}while(startID != itrCtrl->first && add == FALSE);
	}

	for( itrCtrl = epgCapCtrl.begin(); itrCtrl != epgCapCtrl.end(); itrCtrl++ ){
		itrCtrl->second->StartEpgCap();
	}
	this->epgCapCheckFlag = TRUE;

	SendNotifyStatus(2);
	this->setTimeSync = FALSE;

	return ret;
}

void CReserveManager::StopEpgCap()
{
	if( Lock(L"StopEpgCap") == FALSE ) return ;

	map<DWORD, CTunerBankCtrl*>::iterator itr;
	for( itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		itr->second->StopEpgCap();
	}
		 
	UnLock();
}

BOOL CReserveManager::IsEpgCap()
{
	BOOL ret = FALSE;
	map<DWORD, CTunerBankCtrl*>::iterator itr;
	for( itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		if(itr->second->IsEpgCapWorking() == TRUE){
			ret = TRUE;
			if( this->timeSync == TRUE && this->setTimeSync == FALSE){
				LONGLONG delay = itr->second->DelayTime();
				if( abs(delay) > 10*I64_1SEC ){
					LONGLONG local = GetNowI64Time();
					local += delay;
					SYSTEMTIME setTime;
					ConvertSystemTime(local, &setTime);
					if( SetLocalTime(&setTime) == FALSE ){
						_OutputDebugString(L"★SetLocalTime err %I64d", delay/I64_1SEC);
					}else{
						_OutputDebugString(L"★SetLocalTime %I64d",delay/I64_1SEC);
						this->setTimeSync = TRUE;
					}
				}
			}
			break;
		}
	}
		 
	return ret;
}

BOOL CReserveManager::IsFindReserve(
	WORD ONID,
	WORD TSID,
	WORD SID,
	WORD eventID
	)
{
	if( Lock(L"IsFindReserve") == FALSE ) return FALSE;

	BOOL ret = FALSE;
/*
	map<DWORD, CReserveInfo*>::iterator itr;
	for(itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		RESERVE_DATA info;
		itr->second->GetData(&info);
		if( info.originalNetworkID == ONID &&
			info.transportStreamID == TSID &&
			info.serviceID == SID &&
			info.eventID == eventID ){
				ret = TRUE;
				break;
		}
	}
*/
	map<LONGLONG, DWORD>::iterator itr;
	LONGLONG keyID = _Create64Key2(ONID, TSID, SID, eventID);
	itr = this->reserveInfoIDMap.find(keyID);
	if( itr != this->reserveInfoIDMap.end()){
		ret = TRUE;
	}

	UnLock();

	return ret;
}

BOOL CReserveManager::IsFindReserve(
	WORD ONID,
	WORD TSID,
	WORD SID,
	LONGLONG startTime,
	DWORD durationSec
	)
{
	if( Lock(L"IsFindReserve") == FALSE ) return FALSE;

	BOOL ret = FALSE;

	map<DWORD, CReserveInfo*>::iterator itr;
	for(itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		RESERVE_DATA info;
		itr->second->GetData(&info);
		if( info.originalNetworkID == ONID &&
			info.transportStreamID == TSID &&
			info.serviceID == SID &&
			ConvertI64Time(info.startTime) == startTime &&
			info.durationSecond == durationSec ){
				ret = TRUE;
				break;
		}
	}

	UnLock();

	return ret;
}

BOOL CReserveManager::GetTVTestChgCh(
	LONGLONG chID,
	TVTEST_CH_CHG_INFO* chInfo
	)
{
	if( Lock(L"GetTVTestChgCh") == FALSE ) return FALSE;

	BOOL ret = FALSE;
	
	WORD ONID = (WORD)((chID&0x0000FFFF00000000)>>32);
	WORD TSID = (WORD)((chID&0x00000000FFFF0000)>>16);
	WORD SID = (WORD)((chID&0x000000000000FFFF));

	vector<DWORD> idList;
	this->tunerManager.GetSupportServiceTuner(ONID, TSID, SID, &idList);

	for( int i=(int)idList.size() -1; i>=0; i-- ){
		wstring bonDriver = L"";
		this->tunerManager.GetBonFileName(idList[i], bonDriver);
		BOOL find = FALSE;
		for( size_t j=0; j<this->tvtestUseBon.size(); j++ ){
			if( CompareNoCase(this->tvtestUseBon[j], bonDriver) == 0 ){
				find = TRUE;
				break;
			}
		}
		if( find == TRUE ){
			map<DWORD, CTunerBankCtrl*>::iterator itr;
			itr = this->tunerBankMap.find(idList[i]);
			if( itr != this->tunerBankMap.end() ){
				if( itr->second->IsOpenTuner() == FALSE ){
					chInfo->bonDriver = bonDriver;
					chInfo->chInfo.useSID = TRUE;
					chInfo->chInfo.ONID = ONID;
					chInfo->chInfo.TSID = TSID;
					chInfo->chInfo.SID = SID;
					chInfo->chInfo.useBonCh = TRUE;
					this->tunerManager.GetCh(idList[i], ONID, TSID, SID, &chInfo->chInfo.space, &chInfo->chInfo.ch);
					ret = TRUE;
					break;
				}
			}
		}
	}

	UnLock();

	return ret;
}

BOOL CReserveManager::SetNWTVCh(
	SET_CH_INFO* chInfo
	)
{
	if( Lock(L"SetNWTVCh") == FALSE ) return FALSE;

	BOOL ret = FALSE;

	BOOL findCh = FALSE;
	SET_CH_INFO initCh;
	wstring bonDriver = L"";

	vector<DWORD> idList;
	this->tunerManager.GetSupportServiceTuner(chInfo->ONID, chInfo->TSID, chInfo->SID, &idList);

	for( int i=(int)idList.size() -1; i>=0; i-- ){
		this->tunerManager.GetBonFileName(idList[i], bonDriver);
		BOOL find = FALSE;
		for( size_t j=0; j<this->tvtestUseBon.size(); j++ ){
			if( CompareNoCase(this->tvtestUseBon[j], bonDriver) == 0 ){
				find = TRUE;
				break;
			}
		}
		if( find == TRUE ){
			map<DWORD, CTunerBankCtrl*>::iterator itr;
			itr = this->tunerBankMap.find(idList[i]);
			if( itr != this->tunerBankMap.end() ){
				if( itr->second->IsOpenTuner() == FALSE ){
					initCh.useSID = TRUE;
					initCh.ONID = chInfo->ONID;
					initCh.TSID = chInfo->TSID;
					initCh.SID = chInfo->SID;

					initCh.useBonCh = TRUE;
					this->tunerManager.GetCh(idList[i], chInfo->ONID, chInfo->TSID, chInfo->SID, &initCh.space, &initCh.ch);
					findCh = TRUE;
					break;
				}
			}
		}
	}

	if( findCh == FALSE ){
		UnLock();
		return FALSE;
	}

	BOOL findPID = FALSE;
	CTunerCtrl ctrl;
	vector<DWORD> pidList;
	ctrl.GetOpenExe(L"EpgDataCap_Bon.exe", &pidList);
	for( size_t i=0; i<pidList.size(); i++ ){
		if( pidList[i] == this->NWTVPID ){
			findPID = TRUE;
		}
	}
	if( findPID == FALSE ){
		this->NWTVPID = 0;
	}

	if( this->NWTVPID != 0 ){
		int id = -1;
		this->sendCtrlNWTV.SendViewGetID(&id);
		if( this->sendCtrlNWTV.SendViewGetID(&id) == CMD_SUCCESS ){
			if( id == -1 ){
				DWORD status = 0;
				if(this->sendCtrlNWTV.SendViewGetStatus(&status) == CMD_SUCCESS ){
					if( status == VIEW_APP_ST_NORMAL ){
						this->sendCtrlNWTV.SendViewSetBonDrivere(bonDriver);
						this->sendCtrlNWTV.SendViewSetCh(&initCh);
						ret = TRUE;
					}else{
						//録画とかしてそう
						this->NWTVPID = 0;
					}
				}else{
					//終了された？
					this->NWTVPID = 0;
				}
			}else{
				//録画用に奪われた？
				this->NWTVPID = 0;
			}
		}else{
			//終了された？
			this->NWTVPID = 0;
		}
	}
	if( this->NWTVPID == 0 ){

		ctrl.SetExePath(this->recExePath.c_str());
		DWORD PID = 0;
		BOOL noNW = FALSE;
		if( this->NWTVUDP == FALSE && this->NWTVTCP == FALSE ){
			noNW = TRUE;
		}
		if( ctrl.OpenExe(bonDriver, -1, TRUE, TRUE, noNW, this->registGUIMap, &PID, this->NWTVUDP, this->NWTVTCP, 3) == TRUE ){
			this->NWTVPID = PID;
			ret = TRUE;

			wstring pipeName = L"";
			wstring eventName = L"";
			Format(pipeName, L"%s%d", CMD2_VIEW_CTRL_PIPE, this->NWTVPID);
			Format(eventName, L"%s%d", CMD2_VIEW_CTRL_WAIT_CONNECT, this->NWTVPID);
			this->sendCtrlNWTV.SetPipeSetting(eventName, pipeName);
			this->sendCtrlNWTV.SendViewSetCh(&initCh);
		}
	}
	if( ret == FALSE ){
		this->NWTVPID = 0;
	}

	UnLock();

	return ret;
}

BOOL CReserveManager::CloseNWTV(
	)
{
	if( Lock(L"CloseNWTV") == FALSE ) return FALSE;

	BOOL ret = FALSE;

	if( this->NWTVPID != 0 ){
		int id = -1;
		if( this->sendCtrlNWTV.SendViewGetID(&id) == CMD_SUCCESS ){
			if( id == -1 ){
				this->sendCtrlNWTV.SendViewAppClose();
				ret = TRUE;
			}
		}
	}
	this->NWTVPID = 0;

	UnLock();

	return ret;
}

void CReserveManager::SetNWTVMode(
	DWORD mode
	)
{
	if( Lock(L"SetNWTVMode") == FALSE ) return ;

	if( mode == 1 ){
		this->NWTVUDP = TRUE;
		this->NWTVTCP = FALSE;
	}else if( mode == 2 ){
		this->NWTVUDP = FALSE;
		this->NWTVTCP = TRUE;
	}else if( mode == 3 ){
		this->NWTVUDP = TRUE;
		this->NWTVTCP = TRUE;
	}else{
		this->NWTVUDP = FALSE;
		this->NWTVTCP = FALSE;
	}
	UnLock();
}

void CReserveManager::SendTweet(
		SEND_TWEET_MODE mode,
		void* param1,
		void* param2,
		void* param3
	)
{
	if( Lock(L"SendTweet") == FALSE ) return ;

	_SendTweet(mode, param1, param2, param3);

	UnLock();
}

void CReserveManager::_SendTweet(
		SEND_TWEET_MODE mode,
		void* param1,
		void* param2,
		void* param3
	)
{
	if( this->useTweet == TRUE ){
		this->twitterManager.SendTweet(mode, param1, param2, param3);
	}
}

BOOL CReserveManager::GetRecFilePath(
	DWORD reserveID,
	wstring& filePath,
	DWORD* ctrlID,
	DWORD* processID
	)
{
	if( Lock(L"GetRecFilePath") == FALSE ) return FALSE;

	BOOL ret = FALSE;

	map<DWORD, CTunerBankCtrl*>::iterator itrBank;
	for( itrBank = this->tunerBankMap.begin(); itrBank != this->tunerBankMap.end(); itrBank++ ){
		if( itrBank->second->GetRecFilePath(reserveID, filePath, ctrlID, processID) == TRUE ){
			ret = TRUE;
			break;
		}
	}
	UnLock();
	return ret;
}

