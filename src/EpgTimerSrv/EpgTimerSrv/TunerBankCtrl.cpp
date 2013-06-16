﻿#include "StdAfx.h"
#include "TunerBankCtrl.h"
#include "CheckRecFile.h"

#include <process.h>

#include "../../Common/ReNamePlugInUtil.h"

CTunerBankCtrl::CTunerBankCtrl(void)
{
	this->lockEvent = _CreateEvent(FALSE, TRUE, NULL);

	this->checkThread = NULL;
	this->checkStopEvent = _CreateEvent(FALSE, FALSE, NULL);

	this->openTuner = FALSE;
	this->processID = 0;
	this->openErrFlag = FALSE;
	this->useOpendTuner = FALSE;
	this->recMinWake = TRUE;
	this->recView = TRUE;
	this->recNW = FALSE;

	this->currentChID = 0xFFFFFFFF;

	this->sendCtrl.SetConnectTimeOut(5*1000);

	this->epgCapWork = FALSE;

	this->enableCaption = 1;
	this->enableData = 0;

	this->delayTime = 0;

	this->chkSpaceCount = 0;
	this->twitterManager = NULL;
	this->notifyManager = NULL;

	ReloadSetting();
}


CTunerBankCtrl::~CTunerBankCtrl(void)
{
	if( this->checkThread != NULL ){
		::SetEvent(this->checkStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->checkThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->checkThread, 0xffffffff);
		}
		CloseHandle(this->checkThread);
		this->checkThread = NULL;
	}
	if( this->checkStopEvent != NULL ){
		CloseHandle(this->checkStopEvent);
		this->checkStopEvent = NULL;
	}

	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->reserveWork.begin(); itr != this->reserveWork.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->reserveWork.clear();

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
}


BOOL CTunerBankCtrl::Lock(LPCWSTR log, DWORD timeOut)
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
			OutputDebugString(L"◆CTunerBankCtrl::Lock FALSE");
			if( log != NULL ){
				OutputDebugString(log);
			}
		return FALSE;
	}
	return TRUE;
}

void CTunerBankCtrl::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

void CTunerBankCtrl::SetTwitterCtrl(CTwitterManager* twitterManager)
{
	this->twitterManager = twitterManager;
}

void CTunerBankCtrl::ReloadSetting()
{
	wstring iniPath = L"";
	GetModuleIniPath(iniPath);

	wstring commonIniPath = L"";
	GetCommonIniPath(commonIniPath);

	wstring viewIniPath = L"";
	GetModuleFolderPath(viewIniPath);
	viewIniPath += L"\\EpgDataCap_Bon.ini";

	int sec = GetPrivateProfileInt(L"SET", L"StartMargin", 5, iniPath.c_str());
	this->defStartMargine = ((LONGLONG)sec) * I64_1SEC;
	sec = GetPrivateProfileInt(L"SET", L"EndMargin", 2, iniPath.c_str());
	this->defEndMargine = ((LONGLONG)sec) * I64_1SEC;
	this->recWakeTime = ((LONGLONG)GetPrivateProfileInt(L"SET", L"RecAppWakeTime", 2, iniPath.c_str())) * 60 * I64_1SEC;
	this->recMinWake = GetPrivateProfileInt(L"SET", L"RecMinWake", 1, iniPath.c_str());
	this->recView = GetPrivateProfileInt(L"SET", L"RecView", 1, iniPath.c_str());
	this->recNW = GetPrivateProfileInt(L"SET", L"RecNW", 0, iniPath.c_str());
	this->backPriority = GetPrivateProfileInt(L"SET", L"BackPriority", 1, iniPath.c_str());
	this->saveProgramInfo = GetPrivateProfileInt(L"SET", L"PgInfoLog", 0, iniPath.c_str());
	this->saveErrLog = GetPrivateProfileInt(L"SET", L"DropLog", 0, iniPath.c_str());

	this->recOverWrite = GetPrivateProfileInt(L"SET", L"RecOverWrite", 0, iniPath.c_str());
	this->useRecNamePlugIn = GetPrivateProfileInt(L"SET", L"RecNamePlugIn", 0, iniPath.c_str());

	WCHAR buff[512] = L"";
	GetPrivateProfileString(L"SET", L"RecNamePlugInFile", L"RecName_Macro.dll", buff, 512, iniPath.c_str());

	GetModuleFolderPath(this->recNamePlugInFilePath);
	this->recNamePlugInFilePath += L"\\RecName\\";
	this->recNamePlugInFilePath += buff;

	GetPrivateProfileString( L"SET", L"RecFolderPath0", L"", buff, 512, commonIniPath.c_str() );
	this->recFolderPath = buff;
	if( this->recFolderPath.size() == 0 ){
		GetDefSettingPath(this->recFolderPath);
	}
	GetPrivateProfileString( L"SET", L"RecWritePlugIn0", L"", buff, 512, commonIniPath.c_str() );
	this->recWritePlugIn = buff;

	GetPrivateProfileString( L"SET", L"RecExePath", L"", buff, 512, commonIniPath.c_str() );
	this->recExePath = buff;
	if( this->recExePath.size() == 0 ){
		GetModuleFolderPath(this->recExePath);
		this->recExePath += L"\\EpgDataCap_Bon.exe";
	}

	this->tunerCtrl.SetExePath(this->recExePath.c_str());

	this->enableCaption = GetPrivateProfileInt(L"SET", L"Caption", 1, viewIniPath.c_str());
	this->enableData = GetPrivateProfileInt(L"SET", L"Data", 0, viewIniPath.c_str());

	this->processPriority = (DWORD)GetPrivateProfileInt(L"SET", L"ProcessPriority", 3, iniPath.c_str());

}

void CTunerBankCtrl::SetAutoDel(
	BOOL autoDel,
	vector<wstring>* delExtList,
	vector<wstring>* delFolderList
	)
{
	if( Lock() == FALSE ) return;

	this->autoDel = autoDel;
	this->delExtList = *delExtList;
	this->delFolderList = *delFolderList;

	UnLock();
}

void CTunerBankCtrl::SetNotifyManager(CNotifyManager* manager)
{
	if( Lock(L"CTunerBankCtrl::SetNotifyManager") == FALSE ) return;
	this->notifyManager = manager;

	UnLock();
}
/*
void CTunerBankCtrl::SetRegistGUI(map<DWORD, DWORD> registGUIMap)
{
	if( Lock() == FALSE ) return;

	this->registGUIMap = registGUIMap;

	UnLock();
}
*/
void CTunerBankCtrl::SetTunerInfo(
	WORD bonID,
	WORD tunerID,
	wstring bonFileName,
	wstring chSet4FilePath
	)
{
	if( Lock() == FALSE ) return;

	this->tunerID = ((DWORD)bonID)<<16 | tunerID;
	this->bonFileName = bonFileName;
	this->chUtil.ParseText(chSet4FilePath.c_str());

	UnLock();
}

void CTunerBankCtrl::AddReserve(
	vector<CReserveInfo*>* reserveInfo
	)
{
	if( Lock() == FALSE ) return;

	if( this->checkThread != NULL ){
		if( ::WaitForSingleObject(this->checkThread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(this->checkThread);
			this->checkThread = NULL;
		}
	}
	
	for( size_t i=0; i<reserveInfo->size(); i++ ){
		RESERVE_DATA data;
		(*reserveInfo)[i]->GetData(&data);
		map<DWORD, RESERVE_WORK*>::iterator itr;
		itr = this->reserveWork.find(data.reserveID);
		if( itr == this->reserveWork.end() ){
			RESERVE_WORK* item = new RESERVE_WORK;
			item->reserveInfo = (*reserveInfo)[i];
			item->reserveID = data.reserveID;
			//item->ctrlID = 0;
			item->recStartFlag = FALSE;

			this->reserveWork.insert(pair<DWORD, RESERVE_WORK*>(item->reserveID, item));
		}
	}

	if( this->checkThread == NULL ){
		ResetEvent(this->checkStopEvent);
		this->checkThread = (HANDLE)_beginthreadex(NULL, 0, CheckReserveThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->checkThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->checkThread);
	}

	UnLock();
}

void CTunerBankCtrl::ChgReserve(
	RESERVE_DATA* reserve
	)
{
	if( Lock() == FALSE ) return;

	map<DWORD, RESERVE_WORK*>::iterator itr;
	itr = this->createCtrlList.find(reserve->reserveID);
	if( itr != this->createCtrlList.end() ){
		//起動中
		itr->second->reserveInfo->SetData(reserve);

		LONGLONG stratTime = ConvertI64Time(reserve->startTime);
		if( reserve->recSetting.useMargineFlag == TRUE ){
			stratTime -= ((LONGLONG)reserve->recSetting.useMargineFlag) * I64_1SEC;
		}else{
			stratTime -= this->defStartMargine;
		}

		if( GetNowI64Time() < stratTime ){
			//開始時間遅くなったのでコントロール削除の必要あり
			for( size_t i=0; i<itr->second->ctrlID.size(); i++ ){
				if( itr->second->recStartFlag == TRUE ){
					SET_CTRL_REC_STOP_PARAM param;
					param.ctrlID = itr->second->ctrlID[i];
					param.saveErrLog = this->saveErrLog;
					SET_CTRL_REC_STOP_RES_PARAM resVal;
					this->sendCtrl.SendViewStopRec(param, &resVal);
				}
				itr->second->recStartFlag = FALSE;
				this->sendCtrl.SendViewDeleteCtrl(itr->second->ctrlID[i]);
			}
			itr->second->ctrlID.clear();

			itr->second->reserveInfo->SetRecWaitMode(FALSE, 0);

			this->createCtrlList.erase(itr);
		}
	}else{
		itr = this->reserveWork.find(reserve->reserveID);
		if( itr != this->reserveWork.end() ){
			itr->second->reserveInfo->SetData(reserve);
		}
	}
	itr = this->openErrReserveList.find(reserve->reserveID);
	if( itr != this->openErrReserveList.end() ){
		this->openErrReserveList.erase(itr);
	}

	UnLock();
}

void CTunerBankCtrl::DeleteReserve(
	DWORD reserveID
	)
{
	if( Lock() == FALSE ) return;

	map<DWORD, RESERVE_WORK*>::iterator itr;

	//処理中なら停止
	itr = this->createCtrlList.find(reserveID);
	if( itr != this->createCtrlList.end() ){
		for( size_t i=0; i<itr->second->ctrlID.size(); i++ ){
			SET_CTRL_REC_STOP_PARAM param;
			param.ctrlID = itr->second->ctrlID[i];
			param.saveErrLog = this->saveErrLog;
			SET_CTRL_REC_STOP_RES_PARAM resVal;

			this->sendCtrl.SendViewStopRec(param, &resVal);

			this->sendCtrl.SendViewDeleteCtrl(itr->second->ctrlID[i]);
		}

		this->createCtrlList.erase(itr);
	}

	itr = this->reserveWork.find(reserveID);
	if( itr != this->reserveWork.end() ){
		SAFE_DELETE(itr->second);
		this->reserveWork.erase(itr);
	}

	itr = this->openErrReserveList.find(reserveID);
	if( itr != this->openErrReserveList.end() ){
		this->openErrReserveList.erase(itr);
	}

	UnLock();
}

void CTunerBankCtrl::ClearNoCtrl()
{
	if( Lock() == FALSE ) return;

	map<DWORD, RESERVE_WORK*>::iterator itr;
	itr = this->reserveWork.begin();
	while(itr != this->reserveWork.end() )
	{
		if( itr->second->ctrlID.size() == 0 ){
			SAFE_DELETE(itr->second);
			this->reserveWork.erase(itr++);
		}else{
			itr++;
		}
	}

	UnLock();
}

BOOL CTunerBankCtrl::IsOpenErr()
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = this->openErrFlag;

	UnLock();
	return ret;
}

void CTunerBankCtrl::GetOpenErrReserve(vector<CReserveInfo*>* reserveInfo)
{
	if( Lock() == FALSE ) return ;

	map<DWORD, RESERVE_WORK*>::iterator itr;
	for(itr = this->openErrReserveList.begin(); itr != this->openErrReserveList.end(); itr++ ){
		reserveInfo->push_back(itr->second->reserveInfo);
	}

	UnLock();
}

void CTunerBankCtrl::ResetOpenErr()
{
	if( Lock() == FALSE ) return ;

	this->openErrReserveList.clear();
	this->openErrFlag = FALSE;
	
	UnLock();
}

UINT WINAPI CTunerBankCtrl::CheckReserveThread(LPVOID param)
{
	CTunerBankCtrl* sys = (CTunerBankCtrl*)param;
	DWORD wait = 1000;
	LONGLONG delay = 0;

	BOOL startEpgCap = FALSE;

	while(1){
		if( ::WaitForSingleObject(sys->checkStopEvent, wait) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		if( sys->openErrFlag == FALSE ){
			if( sys->Lock() == TRUE ){
				multimap<LONGLONG, RESERVE_WORK*> sortList;
				sys->GetCheckList(&sortList);

				BOOL viewMode = FALSE;
				SET_CH_INFO initCh;
				BOOL needOpenTuner = sys->IsNeedOpenTuner(&sortList, &viewMode, &initCh);
				//起動チェック
				if( needOpenTuner == TRUE ){
					//EPG取得状態確認
					if( startEpgCap == TRUE ){
						//行っているのでキャンセル
					}
					//起動必要
					if( sys->openTuner == FALSE ){
						//まだ起動されてないので起動
						if( sys->OpenTuner(viewMode, &initCh) == FALSE ){
							//起動できなかった
							wait = 1000;
							multimap<LONGLONG, RESERVE_WORK*>::iterator itrErr;
							itrErr = sortList.begin();
							if( itrErr != sortList.end() ){
								itrErr->second->reserveInfo->AddNGTunerID(sys->tunerID);
								sys->openErrReserveList.insert(pair<DWORD, RESERVE_WORK*>(itrErr->second->reserveID, itrErr->second));
							}
							sys->openErrFlag = TRUE;
							sys->UnLock();
							continue;
						}else{
							sys->currentChID = ((DWORD)initCh.ONID) << 16 | initCh.TSID;
							if( sys->notifyManager != NULL ){
								sys->notifyManager->AddNotifyMsg(NOTIFY_UPDATE_PRE_REC_START, sys->bonFileName);
							}
						}
					}
				}else{
					if( startEpgCap ==FALSE ){
						if( sys->openTuner == TRUE ){
							sys->CloseTuner();
						}
						wait = 1000;
					}
				}

				if( sys->openTuner == TRUE && startEpgCap == FALSE ){
					//PC時計との誤差取得
					int delaySec = 0;
					if( sys->sendCtrl.SendViewGetDelay(&delaySec) == CMD_ERR_TIMEOUT ){
						//EXE消されたかも
						wait = 1000;
						sys->ErrStop();
						sys->UnLock();
						continue;
					}
					delay = ((LONGLONG)delaySec)*I64_1SEC;
					sys->delayTime = delay;

					//制御コントロールまだ作成されていないものを作成
					sys->CreateCtrl(&sortList, delay);

					BOOL needShortCheck = FALSE;
					//録画時間のチェック
					sys->CheckRec(delay, &needShortCheck);

					if( needShortCheck == TRUE ){
						wait = 100;
					}else{
						wait = 500;
					}
				}
				sys->UnLock();
			}

			if( sys->epgCapWork == TRUE ){
				if( startEpgCap == FALSE ){
					if( sys->openTuner == TRUE ){
						//予約用にすでに起動中なので終了
						sys->epgCapWork = FALSE;
					}else{
						//チューナー起動
						if( sys->Lock() == TRUE ){
							if( sys->OpenTuner(FALSE, NULL) == FALSE ){
								sys->epgCapWork = FALSE;
							}else{
								//EPG取得開始
								Sleep(1000);
								if(sys->sendCtrl.SendViewEpgCapStart(&sys->epgCapItem) != CMD_SUCCESS){
									sys->CloseTuner();
									sys->epgCapWork = FALSE;
								}else{
									startEpgCap = TRUE;
								}
							}
							sys->UnLock();
						}
					}
				}else{
					//ステータス確認
					DWORD status = 0;
					if( sys->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS ){
						if( status != VIEW_APP_ST_GET_EPG ){
						OutputDebugString(L"epg end");
							//取得終わった
							if( sys->openTuner == TRUE ){
								if( sys->Lock() == TRUE ){
									sys->sendCtrl.SendViewEpgCapStop();
									sys->CloseTuner();
									sys->UnLock();
								}
							}
							startEpgCap = FALSE;
							sys->epgCapWork = FALSE;
						}else{
							//PC時計との誤差取得
							int delaySec = 0;
							if( sys->sendCtrl.SendViewGetDelay(&delaySec) == CMD_SUCCESS ){
								delay = ((LONGLONG)delaySec)*I64_1SEC;
								sys->delayTime = delay;
							}
						}
					}else{
						//エラー？
						OutputDebugString(L"epg err");
						if( sys->openTuner == TRUE ){
							if( sys->Lock() == TRUE ){
								sys->CloseTuner();
								sys->UnLock();
							}
						}
						startEpgCap = FALSE;
						sys->epgCapWork = FALSE;
					}
				}
			}else{
				if( startEpgCap == TRUE ){
					OutputDebugString(L"epg cancel");
					//キャンセル？
					if( sys->openTuner == TRUE ){
						if( sys->Lock() == TRUE ){
							sys->sendCtrl.SendViewEpgCapStop();
							sys->CloseTuner();
							sys->UnLock();
						}
					}
					startEpgCap = FALSE;
				}
			}
		}
	}
	return 0;
}

void CTunerBankCtrl::GetCheckList(multimap<LONGLONG, RESERVE_WORK*>* sortList)
{
	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->reserveWork.begin(); itr != this->reserveWork.end(); itr++ ){
		RESERVE_DATA data;
		itr->second->reserveInfo->GetData(&data);
		if( data.recSetting.recMode == RECMODE_NO ){
			continue;
		}

		itr->second->stratTime = ConvertI64Time(data.startTime);
		itr->second->endTime = GetSumTime(data.startTime, data.durationSecond);

		if( data.recSetting.useMargineFlag == TRUE ){
			itr->second->startMargine = ((LONGLONG)data.recSetting.startMargine) * I64_1SEC;
			itr->second->endMargine = ((LONGLONG)data.recSetting.endMargine) * I64_1SEC;
		}else{
			itr->second->startMargine = this->defStartMargine;
			itr->second->endMargine = this->defEndMargine;
		}

		itr->second->chID = ((DWORD)data.originalNetworkID)<<16 | data.transportStreamID;
		itr->second->priority = data.recSetting.priority;


		if( data.recSetting.recMode == 2 || data.recSetting.recMode == 3 ){
			itr->second->enableScramble = 0;
		}else{
			itr->second->enableScramble = 1;
		}

		if( data.recSetting.serviceMode & RECSERVICEMODE_SET ){
			if( data.recSetting.serviceMode & RECSERVICEMODE_CAP ){
				itr->second->enableCaption = 1;
			}else{
				itr->second->enableCaption = 0;
			}
			if( data.recSetting.serviceMode & RECSERVICEMODE_DATA ){
				itr->second->enableData = 1;
			}else{
				itr->second->enableData = 0;
			}
		}else{
			itr->second->enableCaption = this->enableCaption;
			itr->second->enableData = this->enableData;
		}

		itr->second->partialRecFlag = data.recSetting.partialRecFlag;
		itr->second->continueRecFlag = data.recSetting.continueRecFlag;

		itr->second->ONID = data.originalNetworkID;
		itr->second->TSID = data.transportStreamID;
		itr->second->SID = data.serviceID;

		sortList->insert(pair<LONGLONG, RESERVE_WORK*>(itr->second->stratTime, itr->second));
	}
}

BOOL CTunerBankCtrl::IsNeedOpenTuner(multimap<LONGLONG, RESERVE_WORK*>* sortList, BOOL* viewMode, SET_CH_INFO* initCh)
{
	if( sortList == NULL ){
		return FALSE;
	}
	if( sortList->size() == 0 ){
		return FALSE;
	}

	LONGLONG nowTime = GetNowI64Time();

	BOOL ret = FALSE;
	multimap<LONGLONG, RESERVE_WORK*>::iterator itr;
	itr = sortList->begin();
	if( itr->second->stratTime - this->recWakeTime - itr->second->startMargine < nowTime ){
		ret =  TRUE;
		BYTE recMode=0;
		itr->second->reserveInfo->GetRecMode(&recMode);
		if( recMode == RECMODE_VIEW ){
			*viewMode = TRUE;
		}
		itr->second->reserveInfo->GetService(&(initCh->ONID), &(initCh->TSID), &(initCh->SID) );
		initCh->useSID = TRUE;
		initCh->useBonCh = FALSE;

		wstring searchKey;
		WIN32_FIND_DATA findData;
		HANDLE find;

		RESERVE_DATA data;
		itr->second->reserveInfo->GetData(&data);

		if( data.recSetting.recFolderList.size() == 0 ){
			searchKey = this->recFolderPath;
		}else{
			searchKey = data.recSetting.recFolderList[0].recFolder;
		}
		searchKey += L"\\*.*";

		//指定フォルダのファイル一覧取得
		find = FindFirstFile( searchKey.c_str(), &findData);
		if ( find != INVALID_HANDLE_VALUE ) {
			FindClose(find);
		}
	}

	return ret;
}


BOOL CTunerBankCtrl::OpenTuner(BOOL viewMode, SET_CH_INFO* initCh)
{
	BOOL noView = TRUE;
	BOOL noNW = TRUE;
	BOOL UDP = FALSE;
	BOOL TCP = FALSE;
	if( this->recView == TRUE && viewMode == TRUE ){
		noView = FALSE;
	}
	if( this->recNW == TRUE || viewMode == TRUE ){
		noNW = FALSE;
		UDP = TRUE;
		TCP = TRUE;
	}
	this->useOpendTuner = FALSE;
	map<DWORD, DWORD> registGUIMap;
	if( this->notifyManager != NULL ){
		this->notifyManager->GetRegistGUI(&registGUIMap);
	}

	BOOL ret = tunerCtrl.OpenExe(this->bonFileName, tunerID, this->recMinWake, noView, noNW, registGUIMap, &this->processID, UDP, TCP, this->processPriority);
	if( ret == FALSE ){
		Sleep(500);
		ret = tunerCtrl.OpenExe(this->bonFileName, tunerID, this->recMinWake, noView, noNW, registGUIMap, &this->processID, UDP, TCP, this->processPriority);
	}
	if( ret == TRUE ){
		wstring pipeName = L"";
		wstring eventName = L"";
		Format(pipeName, L"%s%d", CMD2_VIEW_CTRL_PIPE, this->processID);
		Format(eventName, L"%s%d", CMD2_VIEW_CTRL_WAIT_CONNECT, this->processID);
		this->sendCtrl.SetPipeSetting(eventName, pipeName);

		this->sendCtrl.SendViewSetID(this->tunerID);

		this->sendCtrl.SendViewSetStandbyRec(1);
		if( initCh != NULL ){
			this->sendCtrl.SendViewSetCh(initCh);
		}
	}else{
		//CHがNULLならEPG取得用
		//EPG取得では奪わない
		if( initCh != NULL ){
			//起動中で使えるもの探す
			vector<DWORD> IDList;
			tunerCtrl.GetOpenExe(&IDList);
			for(size_t i=0; i<IDList.size(); i++ ){
				wstring pipeName = L"";
				wstring eventName = L"";
				Format(pipeName, L"%s%d", CMD2_VIEW_CTRL_PIPE, IDList[i]);
				Format(eventName, L"%s%d", CMD2_VIEW_CTRL_WAIT_CONNECT, IDList[i]);
				this->sendCtrl.SetPipeSetting(eventName, pipeName);

				wstring bonDriver = L"";
				this->sendCtrl.SendViewGetBonDrivere(&bonDriver);
				if( bonDriver.size() > 0 && CompareNoCase(bonDriver, this->bonFileName) == 0 ){
					int id=0;
					if(this->sendCtrl.SendViewGetID(&id) == CMD_SUCCESS){
						if( id == -1 ){
							DWORD status = 0;
							if(this->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS){
								if( status == VIEW_APP_ST_NORMAL || status == VIEW_APP_ST_ERR_CH_CHG){
									this->sendCtrl.SendViewSetID(this->tunerID);

									this->sendCtrl.SendViewSetStandbyRec(1);

									if( initCh != NULL ){
										this->sendCtrl.SendViewSetCh(initCh);
									}
									this->processID = IDList[i];
									this->useOpendTuner = TRUE;
									ret = TRUE;
								
									break;
								}
							}
						}
					}
				}
			}
			if( this->useOpendTuner == FALSE ){
				//TVTestで使ってるものあるかチェック
				IDList.clear();
				tunerCtrl.GetOpenExe(L"tvtest.exe", &IDList);
				map<DWORD, DWORD> registGUIMap;
				if( this->notifyManager != NULL ){
					this->notifyManager->GetRegistGUI(&registGUIMap);
				}

				for(size_t i=0; i<IDList.size(); i++ ){
					CSendCtrlCmd send;
					wstring pipeName = L"";
					wstring eventName = L"";
					Format(pipeName, L"%s%d", CMD2_TVTEST_CTRL_PIPE, IDList[i]);
					Format(eventName, L"%s%d", CMD2_TVTEST_CTRL_WAIT_CONNECT, IDList[i]);
					send.SetPipeSetting(eventName, pipeName);
					send.SetConnectTimeOut(1000);

					wstring bonDriver = L"";
					if( send.SendViewGetBonDrivere(&bonDriver) == CMD_SUCCESS){
						if( bonDriver.size() > 0 && CompareNoCase(bonDriver, this->bonFileName) == 0 ){
							send.SendViewAppClose();
							Sleep(5000);
							ret = tunerCtrl.OpenExe(this->bonFileName, tunerID, this->recMinWake, noView, noNW, registGUIMap, &this->processID, UDP, TCP, this->processPriority);
							if( ret == FALSE ){
								Sleep(500);
								ret = tunerCtrl.OpenExe(this->bonFileName, tunerID, this->recMinWake, noView, noNW, registGUIMap, &this->processID, UDP, TCP, this->processPriority);
							}
							if( ret == TRUE ){
								wstring pipeName = L"";
								wstring eventName = L"";
								Format(pipeName, L"%s%d", CMD2_VIEW_CTRL_PIPE, this->processID);
								Format(eventName, L"%s%d", CMD2_VIEW_CTRL_WAIT_CONNECT, this->processID);
								this->sendCtrl.SetPipeSetting(eventName, pipeName);

								this->sendCtrl.SendViewSetID(this->tunerID);

								this->sendCtrl.SendViewSetStandbyRec(1);
								if( initCh != NULL ){
									this->sendCtrl.SendViewSetCh(initCh);
								}
								break;
							}
						}
					}
				}
			}
			//EPG取得中のもの奪う
			if( this->useOpendTuner == FALSE ){
				IDList.clear();
				tunerCtrl.GetOpenExe(&IDList);
				for(size_t i=0; i<IDList.size(); i++ ){
					wstring pipeName = L"";
					wstring eventName = L"";
					Format(pipeName, L"%s%d", CMD2_VIEW_CTRL_PIPE, IDList[i]);
					Format(eventName, L"%s%d", CMD2_VIEW_CTRL_WAIT_CONNECT, IDList[i]);
					this->sendCtrl.SetPipeSetting(eventName, pipeName);

					wstring bonDriver = L"";
					this->sendCtrl.SendViewGetBonDrivere(&bonDriver);
					if( bonDriver.size() > 0 && CompareNoCase(bonDriver, this->bonFileName) == 0 ){
						int id=0;
						if(this->sendCtrl.SendViewGetID(&id) == CMD_SUCCESS){
							if( id == -1 ){
								DWORD status = 0;
								if(this->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS){
									if( status == VIEW_APP_ST_GET_EPG ){
										this->sendCtrl.SendViewEpgCapStop();
										this->sendCtrl.SendViewSetID(this->tunerID);

										this->sendCtrl.SendViewSetStandbyRec(1);

										if( initCh != NULL ){
											this->sendCtrl.SendViewSetCh(initCh);
										}
										this->processID = IDList[i];
										this->useOpendTuner = TRUE;
										ret = TRUE;
								
										break;
									}
								}
							}
						}
					}
				}
			}
			//録画中のもの奪う
			if( this->useOpendTuner == FALSE ){
				IDList.clear();
				tunerCtrl.GetOpenExe(&IDList);
				for(size_t i=0; i<IDList.size(); i++ ){
					wstring pipeName = L"";
					wstring eventName = L"";
					Format(pipeName, L"%s%d", CMD2_VIEW_CTRL_PIPE, IDList[i]);
					Format(eventName, L"%s%d", CMD2_VIEW_CTRL_WAIT_CONNECT, IDList[i]);
					this->sendCtrl.SetPipeSetting(eventName, pipeName);

					wstring bonDriver = L"";
					this->sendCtrl.SendViewGetBonDrivere(&bonDriver);
					if( bonDriver.size() > 0 && CompareNoCase(bonDriver, this->bonFileName) == 0 ){
						int id=0;
						if(this->sendCtrl.SendViewGetID(&id) == CMD_SUCCESS){
							if( id == -1 ){
								DWORD status = 0;
								if(this->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS){
									if( status == VIEW_APP_ST_REC ){
										this->sendCtrl.SendViewStopRecAll();
										this->sendCtrl.SendViewSetID(this->tunerID);

										this->sendCtrl.SendViewSetStandbyRec(1);

										if( initCh != NULL ){
											this->sendCtrl.SendViewSetCh(initCh);
										}
										this->processID = IDList[i];
										this->useOpendTuner = TRUE;
										ret = TRUE;
								
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if( ret == TRUE ){
		this->openTuner = TRUE;
	}else{
		this->openTuner = FALSE;
	}

	return ret;
}

BOOL CTunerBankCtrl::FindPartialService(WORD ONID, WORD TSID, WORD SID, WORD* partialSID, wstring* serviceName)
{
	multimap<LONGLONG, CH_DATA4>::iterator itr;
	for( itr = this->chUtil.chList.begin(); itr != this->chUtil.chList.end(); itr++ ){
		if( itr->second.originalNetworkID == ONID && itr->second.transportStreamID == TSID && itr->second.partialFlag == TRUE ){
			if( itr->second.serviceID != SID ){
				if( partialSID != NULL ){
					*partialSID = itr->second.serviceID;
				}
				if( serviceName != NULL ){
					*serviceName = itr->second.serviceName;
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

void CTunerBankCtrl::CreateCtrl(multimap<LONGLONG, RESERVE_WORK*>* sortList, LONGLONG delay)
{
	LONGLONG nowTime = GetNowI64Time();
	nowTime += delay;

	multimap<LONGLONG, RESERVE_WORK*>::iterator itr;
	for( itr = sortList->begin(); itr != sortList->end(); itr++ ){
		if( itr->second->ctrlID.size() == 0 ){
			//録画開始１分前になったらコントロール作成
			LONGLONG chkStartTime = itr->second->stratTime - (60*I64_1SEC) - itr->second->startMargine;

			if( chkStartTime < nowTime ){
				BOOL createFlag = FALSE;
				if( this->currentChID == itr->second->chID ){
					//録画中のものに同一サービスで連続録画設定のものある？
					if( ContinueRec(itr->second) == FALSE ){
						CreateCtrl(itr->second);
						createFlag = TRUE;
					}
				}else{
					//チャンネル違うので録画時間考慮する必要あり
					createFlag = CheckOtherChCreate(nowTime, itr->second);

					if( createFlag == TRUE ){
						//作成タイミングになったので作成
						StopAllRec();

						SET_CH_INFO chgCh;
						itr->second->reserveInfo->GetService(&chgCh.ONID, &chgCh.TSID, &chgCh.SID);
						chgCh.useSID = TRUE;
						chgCh.useBonCh = FALSE;

						this->sendCtrl.SendViewSetStandbyRec(1);

						if( this->sendCtrl.SendViewSetCh(&chgCh) != CMD_SUCCESS ){
							//失敗時もう一度リトライ
							Sleep(200);
							this->sendCtrl.SendViewSetCh(&chgCh);
						}
						this->currentChID = ((DWORD)chgCh.ONID) << 16 | chgCh.TSID;

						//作成
						CreateCtrl(itr->second);
					}
				}

				if( this->notifyManager != NULL && createFlag == TRUE ){
					RESERVE_DATA data;
					itr->second->reserveInfo->GetData(&data);
					wstring msg;
					Format(msg, L"%s %04d/%02d/%02d %02d:%02d:%02d～ %s", 
						data.stationName.c_str(),
						data.startTime.wYear,
						data.startTime.wMonth,
						data.startTime.wDay,
						data.startTime.wHour,
						data.startTime.wMinute,
						data.startTime.wMilliseconds,
						data.title.c_str()
						);
					this->notifyManager->AddNotifyMsg(NOTIFY_UPDATE_PRE_REC_START, msg);
				}
			}
		}
	}
}

void CTunerBankCtrl::CreateCtrl(RESERVE_WORK* info)
{
	//作成
	DWORD newCtrlID = 0;
	BOOL createFull = TRUE;
	if( info->partialRecFlag == 1 || info->partialRecFlag == 2){
		//部分受信サービスも
		WORD partialSID = 0;
		if( FindPartialService(info->ONID, info->TSID, info->SID, &partialSID, NULL) == TRUE ){
			//部分受信
			if( sendCtrl.SendViewCreateCtrl(&newCtrlID) == CMD_SUCCESS){
				info->ctrlID.push_back(newCtrlID);
				//コントロールに対して設定
				SET_CTRL_MODE param;
				param.ctrlID = newCtrlID;
				param.SID = partialSID;
				param.enableScramble = info->enableScramble;
				param.enableCaption = info->enableCaption;
				param.enableData = info->enableData;

				if( this->sendCtrl.SendViewSetCtrlMode(param) != CMD_SUCCESS){
					//失敗時もう一度リトライ
					Sleep(200);
					this->sendCtrl.SendViewSetCtrlMode(param);
				}

				info->partialCtrlID = newCtrlID;
				if(info->partialRecFlag == 2){
					createFull = FALSE;
				}
			}
		}
	}
	if( createFull == FALSE ){
		info->mainCtrlID = newCtrlID;
		info->reserveInfo->SetRecWaitMode(TRUE, this->tunerID);
		this->createCtrlList.insert(pair<DWORD, RESERVE_WORK*>(info->reserveID,info));
	}else{
		//通常
		if( sendCtrl.SendViewCreateCtrl(&newCtrlID) == CMD_SUCCESS){
			info->ctrlID.push_back(newCtrlID);
			info->mainCtrlID = newCtrlID;
			info->reserveInfo->SetRecWaitMode(TRUE, this->tunerID);
			//コントロールに対して設定
			SET_CTRL_MODE param;
			param.ctrlID = newCtrlID;

			BYTE recMode = 0;
			info->reserveInfo->GetRecMode(&recMode);
			if( recMode == RECMODE_ALL || recMode == RECMODE_ALL_NOB25 ){
				param.SID = 0xFFFF;
			}else{
				info->reserveInfo->GetService(NULL,NULL,&param.SID);
			}
			param.enableScramble = info->enableScramble;
			param.enableCaption = info->enableCaption;
			param.enableData = info->enableData;

			if( this->sendCtrl.SendViewSetCtrlMode(param) != CMD_SUCCESS){
				//失敗時もう一度リトライ
				Sleep(200);
				this->sendCtrl.SendViewSetCtrlMode(param);
			}
						
			this->createCtrlList.insert(pair<DWORD, RESERVE_WORK*>(info->reserveID, info));
		}
	}
}

BOOL CTunerBankCtrl::ContinueRec(RESERVE_WORK* info)
{
	BOOL ret = FALSE;
	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->createCtrlList.begin(); itr != this->createCtrlList.end(); itr++ ){
		RESERVE_DATA data;
		itr->second->reserveInfo->GetData(&data);
		if( data.reserveStatus == ADD_RESERVE_NO_FIND ||
			data.reserveStatus == ADD_RESERVE_UNKNOWN_END
			){
				continue;
		}
		if( itr->second->reserveInfo->IsChkPfInfo() == FALSE && data.recSetting.tuijyuuFlag == 1 ){
			continue;
		}
		if( itr->second->ONID == info->ONID &&
			itr->second->TSID == info->TSID &&
			itr->second->SID == info->SID &&
			itr->second->continueRecFlag == 1 ){
				if( itr->second->stratTime < info->stratTime &&
					info->stratTime <= itr->second->endTime &&
					itr->second->endTime < info->endTime
					){
					info->ctrlID = itr->second->ctrlID;
					info->mainCtrlID = itr->second->mainCtrlID;
					info->reserveInfo->SetRecWaitMode(TRUE, this->tunerID);
					itr->second->reserveInfo->SetContinueRecFlag(TRUE);

					info->recStartFlag = TRUE;

					this->createCtrlList.insert(pair<DWORD, RESERVE_WORK*>(info->reserveID, info));


					ret = TRUE;
					break;
				}
		}
	}

	return ret;
}

BOOL CTunerBankCtrl::CheckOtherChCreate(LONGLONG nowTime, RESERVE_WORK* reserve)
{
	BOOL createFlag = FALSE;

	LONGLONG chkStartTime = reserve->stratTime;
	if( reserve->startMargine < 0 ){
		chkStartTime -= reserve->startMargine;
	}

	LONGLONG chgTimeHPriority = 0;
	LONGLONG chgTimeLPriority = 0;

	if( this->createCtrlList.size() != 0 ){
		map<DWORD, RESERVE_WORK*>::iterator itr;
		for( itr = this->createCtrlList.begin(); itr != this->createCtrlList.end(); itr++ ){
			LONGLONG chkEndTime = itr->second->endTime;
			if( reserve->endMargine < 0 ){
				chkEndTime += reserve->endMargine;
			}
			if( chkEndTime == chkStartTime ){
				//開始と終了が同じ
				if( itr->second->priority < reserve->priority ||
					(itr->second->priority == reserve->priority && this->backPriority == TRUE )){
						//後が優先なので終了15秒前に停止させる
						if( chgTimeHPriority > chkEndTime - 15*I64_1SEC || chgTimeHPriority == 0 ){
							chgTimeHPriority = chkEndTime - 15*I64_1SEC;
						}
				}else{
					//前の録画優先なので終わりまで待つ
					if( chgTimeLPriority < chkEndTime || chgTimeLPriority == 0 ){
						chgTimeLPriority = chkEndTime;
					}
				}
			}else if( chkEndTime > chkStartTime ){
				//開始と終了が違う
				if( itr->second->priority < reserve->priority ||
					(itr->second->priority == reserve->priority && this->backPriority == TRUE )){
					//後ろ優先なので開始15秒前に停止させる
					if( chgTimeHPriority > chkStartTime - 15*I64_1SEC || chgTimeHPriority == 0 ){
						chgTimeHPriority = chkStartTime - 15*I64_1SEC;
					}
				}else{
					//前の録画優先なので終わりまで待つ
					if( chgTimeLPriority < itr->second->endTime + reserve->endMargine || chgTimeLPriority == 0 ){
						chgTimeLPriority = itr->second->endTime + reserve->endMargine;
					}
				}
			}
		}

		LONGLONG chgTime = 0;
		if( chgTimeHPriority == 0 && chgTimeLPriority != 0 ){
			chgTime = chgTimeLPriority;
		}else if( chgTimeHPriority != 0 && chgTimeLPriority == 0 ){
			chgTime = chgTimeHPriority;
		}else if( chgTimeHPriority != 0 && chgTimeLPriority != 0 ){
			//自分より高い優先度存在するのでそちら優先
			chgTime = chgTimeLPriority;
		}

		if( chgTime != 0 && chgTime <= nowTime ){
			createFlag = TRUE;
		}
	}else{
		createFlag = TRUE;
	}

	return createFlag;
}

void CTunerBankCtrl::StopAllRec()
{
	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->createCtrlList.begin(); itr != this->createCtrlList.end(); itr++ ){
		for( size_t i=0; i<itr->second->ctrlID.size(); i++ ){
			SET_CTRL_REC_STOP_PARAM param;
			param.ctrlID = itr->second->ctrlID[i];
			param.saveErrLog = this->saveErrLog;
			SET_CTRL_REC_STOP_RES_PARAM resVal;

			this->sendCtrl.SendViewStopRec(param, &resVal);

			this->sendCtrl.SendViewDeleteCtrl(itr->second->ctrlID[i]);

			if( itr->second->ctrlID[i] == itr->second->mainCtrlID ){
				if( itr->second->endTime > GetNowI64Time() + 60*I64_1SEC ){
					AddEndReserve(itr->second, REC_END_STATUS_NEXT_START_END, resVal);
				}else{
					if( itr->second->notStartHeadFlag == FALSE ){
						AddEndReserve(itr->second, REC_END_STATUS_NORMAL, resVal);
					}else{
						AddEndReserve(itr->second, REC_END_STATUS_NOT_START_HEAD, resVal);
					}
				}
			}
		}
//		this->endReserveList.insert(pair<DWORD, RESERVE_WORK*>(itr->first, itr->second));
	}
	this->createCtrlList.clear();
}

void CTunerBankCtrl::ErrStop()
{
	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->createCtrlList.begin(); itr != this->createCtrlList.end(); itr++ ){
		SET_CTRL_REC_STOP_RES_PARAM resVal;
		resVal.drop = 0;
		resVal.scramble = 0;
		AddEndReserve(itr->second, REC_END_STATUS_ERR_END, resVal);
//		this->endReserveList.insert(pair<DWORD, RESERVE_WORK*>(itr->first, itr->second));
	}
	this->createCtrlList.clear();
	this->processID = 0;
	this->openTuner = FALSE;
}

void CTunerBankCtrl::CheckRec(LONGLONG delay, BOOL* needShortCheck)
{
	LONGLONG nowTime = GetNowI64Time();
	nowTime += delay;

	*needShortCheck = FALSE;
	vector<DWORD> endList;

	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->createCtrlList.begin(); itr != this->createCtrlList.end(); itr++ ){
		RESERVE_DATA data;
		itr->second->reserveInfo->GetData(&data);

		LONGLONG chkStartTime = 0;
		LONGLONG chkEndTime = 0;

		LONGLONG startMargine = 0;
		LONGLONG endMargine = 0;
		if( data.recSetting.useMargineFlag == TRUE ){
			startMargine = ((LONGLONG)data.recSetting.startMargine)*I64_1SEC;
			endMargine = ((LONGLONG)data.recSetting.endMargine)*I64_1SEC;
		}else{
			startMargine = this->defStartMargine;
			endMargine = this->defEndMargine;
		}

		chkStartTime = ConvertI64Time(data.startTime) - startMargine - I64_1SEC;
		chkEndTime = GetSumTime(data.startTime, data.durationSecond) + endMargine;

		if( nowTime < chkStartTime ){
			if( chkStartTime - 5*I64_1SEC < nowTime){
				//開始5秒前になったらチェック間隔を短くする
				*needShortCheck = TRUE;
			}else{
				//ステータス確認
				DWORD status = 0;
				if( this->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS ){
					if( status == VIEW_APP_ST_ERR_CH_CHG ){
						//チャンネル切り替え失敗してるようなのでリトライ
						SET_CH_INFO chgCh;
						itr->second->reserveInfo->GetService(&chgCh.ONID, &chgCh.TSID, &chgCh.SID);
						chgCh.useSID = TRUE;
						chgCh.useBonCh = FALSE;

						this->sendCtrl.SendViewSetCh(&chgCh);
						this->currentChID = ((DWORD)chgCh.ONID) << 16 | chgCh.TSID;
					}
				}
			}
		}else if( chkStartTime <= nowTime && nowTime < chkEndTime-30*I64_1SEC){
			if( itr->second->recStartFlag == FALSE ){
				//開始時間過ぎているので録画開始
				if( RecStart(nowTime, itr->second, TRUE) == TRUE ){
					itr->second->recStartFlag = TRUE;
					if( chkStartTime + 60*I64_1SEC < nowTime ){
						//途中から開始された
						itr->second->notStartHeadFlag = TRUE;
					}
				}else{
					//録画に失敗した？
					SET_CTRL_REC_STOP_RES_PARAM resVal;
					resVal.drop = 0;
					resVal.scramble = 0;
					AddEndReserve(itr->second, REC_END_STATUS_ERR_RECSTART, resVal);
				}
			}else{
				if( this->saveProgramInfo == TRUE && itr->second->savedPgInfo == FALSE){
					GET_EPG_PF_INFO_PARAM val;
					itr->second->reserveInfo->GetService(&val.ONID, &val.TSID, &val.SID);
					val.pfNextFlag = 0;

					EPGDB_EVENT_INFO resVal;
					if( this->sendCtrl.SendViewGetEventPF(&val, &resVal) == CMD_SUCCESS ){
						if( resVal.StartTimeFlag == 1 && resVal.DurationFlag == 1 ){
							if( ConvertI64Time(resVal.start_time) <= GetSumTime(data.startTime, 30) &&
								GetSumTime(data.startTime, 30) < GetSumTime(resVal.start_time, resVal.durationSec)
								){
								//開始時間から30秒は過ぎているのでこの番組情報が録画中のもののはず
								itr->second->savedPgInfo = TRUE;
								//録画ファイルのパス取得
								for(size_t i=0; i<itr->second->ctrlID.size(); i++ ){
									wstring recFilePath = L"";
									if( this->sendCtrl.SendViewGetRecFilePath(itr->second->ctrlID[i], &recFilePath) == CMD_SUCCESS ){
										//番組情報保存
										recFilePath += L".program.txt";
										SaveProgramInfo(recFilePath, &resVal, 0);
									}
								}
							}
						}
					}
				}
				//ステータス確認
				DWORD status = 0;
				if( this->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS ){
					if( status != VIEW_APP_ST_REC && data.recSetting.recMode != RECMODE_VIEW){
						//キャンセルされた？
						SET_CTRL_REC_STOP_RES_PARAM resVal;
						resVal.drop = 0;
						resVal.scramble = 0;
						if( status == VIEW_APP_ST_ERR_CH_CHG ){
							__int64 chkTime = ConvertI64Time(data.startTime);
							if( startMargine < 0 ){
								chkTime -= startMargine;
							}
							if( nowTime > chkTime + (60*I64_1SEC) ){
								AddEndReserve(itr->second, REC_END_STATUS_ERR_CH_CHG, resVal);
							}
						}else{
							AddEndReserve(itr->second, REC_END_STATUS_ERR_END, resVal);
						}
					}
				}
				if( this->autoDel == TRUE ){
					if( this->chkSpaceCount > 30 ){
						CCheckRecFile chkFile;
						chkFile.SetCheckFolder(&this->delFolderList);
						chkFile.SetDeleteExt(&this->delExtList);
						for(size_t i=0; i<itr->second->ctrlID.size(); i++ ){
							wstring recFilePath = L"";
							if( this->sendCtrl.SendViewGetRecFilePath(itr->second->ctrlID[i], &recFilePath) == CMD_SUCCESS ){
								wstring folderPath = L"";
								GetFileFolder(recFilePath, folderPath);
								chkFile.CheckFreeSpaceLive(&data, folderPath);
							}
						}
					}
				}
			}
		}else if( chkEndTime-5*I64_1SEC < nowTime && nowTime <chkEndTime ){
			//終了5秒前になったらチェック間隔を短くする
			*needShortCheck = TRUE;
		}else if( chkEndTime < nowTime){
			//終了時間過ぎている
			if( itr->second->reserveInfo->IsContinueRec() == FALSE ){
				for(size_t i=0; i<itr->second->ctrlID.size(); i++ ){
					SET_CTRL_REC_STOP_PARAM param;
					param.ctrlID = itr->second->ctrlID[i];
					param.saveErrLog = this->saveErrLog;
					SET_CTRL_REC_STOP_RES_PARAM resVal;
					BOOL errEnd = FALSE;
					if( this->sendCtrl.SendViewStopRec(param, &resVal) == CMD_ERR ){
						errEnd = TRUE;
					}

					this->sendCtrl.SendViewDeleteCtrl(itr->second->ctrlID[i]);
					if( itr->second->ctrlID[i] == itr->second->mainCtrlID ){
						DWORD endType = REC_END_STATUS_NORMAL;
						if( itr->second->notStartHeadFlag == TRUE ){
							endType = REC_END_STATUS_NOT_START_HEAD;
						}
						if( resVal.subRecFlag == 1 ){
							endType = REC_END_STATUS_END_SUBREC;
						}
						if( data.recSetting.tuijyuuFlag == 1 && data.eventID != 0xFFFF ){
							if( itr->second->reserveInfo->IsChkPfInfo() == FALSE ){
								endType = REC_END_STATUS_NOT_FIND_PF;
							}
						}
						if( errEnd == TRUE ){
							endType = REC_END_STATUS_ERR_END2;
						}
						AddEndReserve(itr->second, endType, resVal);
					}
				}
			}else{
				for(size_t i=0; i<itr->second->ctrlID.size(); i++ ){
					SET_CTRL_REC_STOP_RES_PARAM resVal;
					resVal.drop = 0;
					resVal.scramble = 0;
					resVal.recFilePath = L"";
					this->sendCtrl.SendViewGetRecFilePath(itr->second->mainCtrlID, &resVal.recFilePath);

					DWORD endType = REC_END_STATUS_NORMAL;
					if( resVal.subRecFlag == 1 ){
						endType = REC_END_STATUS_END_SUBREC;
					}
					if( data.recSetting.tuijyuuFlag == 1 && data.eventID != 0xFFFF ){
						if( itr->second->reserveInfo->IsChkPfInfo() == FALSE ){
							endType = REC_END_STATUS_NOT_FIND_PF;
						}
					}
					AddEndReserve(itr->second, endType, resVal);
				}
			}

			endList.push_back(itr->first);
		}
	}

	if( this->autoDel == TRUE ){
		if( this->chkSpaceCount > 30 ){
			this->chkSpaceCount = 0;
		}else{
			this->chkSpaceCount++;
		}
	}
	//終了リストに移行
	for( size_t i=0; i<endList.size(); i++ ){
		itr = this->createCtrlList.find(endList[i]);
		if( itr != this->createCtrlList.end()){
			this->createCtrlList.erase(itr);
		}
	}
}

void CTunerBankCtrl::SaveProgramInfo(wstring savePath, EPGDB_EVENT_INFO* info, BYTE mode)
{
	wstring outText = L"";
	wstring serviceName = L"";
	multimap<LONGLONG, CH_DATA4>::iterator itr;
	LONGLONG key = _Create64Key(info->original_network_id, info->transport_stream_id, info->service_id);
	itr = chUtil.chList.find(key);
	if( itr != chUtil.chList.end() ){
		serviceName = itr->second.serviceName;
	}
	_ConvertEpgInfoText2(info, outText, serviceName);

	string buff = "";
	WtoA(outText, buff);

	HANDLE file = _CreateFile2( savePath.c_str(), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( file != INVALID_HANDLE_VALUE ){
		DWORD dwWrite;
		WriteFile(file, buff.c_str(), (DWORD)buff.size(), &dwWrite, NULL);
		CloseHandle(file);
	}
}

BOOL CTunerBankCtrl::RecStart(LONGLONG nowTime, RESERVE_WORK* reserve, BOOL sendNoyify)
{
	RESERVE_DATA data;
	reserve->reserveInfo->GetData(&data);
	BOOL ret = TRUE;

	if( data.recSetting.recMode == RECMODE_VIEW ){
		this->sendCtrl.SendViewSetStandbyRec(2);
		if( this->recView == TRUE ){
			this->sendCtrl.SendViewExecViewApp();
		}
		return TRUE;
	}

	for( size_t i=0; i<reserve->ctrlID.size(); i++ ){
		SET_CTRL_REC_PARAM param;
		param.ctrlID = reserve->ctrlID[i];

		if( this->useRecNamePlugIn == TRUE ){
			CReNamePlugInUtil plugIn;
			if( plugIn.Initialize(this->recNamePlugInFilePath.c_str()) == TRUE ){
				WCHAR name[512] = L"";
				DWORD size = 512;
				PLUGIN_RESERVE_INFO info;

				info.startTime = data.startTime;
				info.durationSec = data.durationSecond;
				wcscpy_s(info.eventName, 512, data.title.c_str());
				info.ONID = data.originalNetworkID;
				info.TSID = data.transportStreamID;
				info.SID = data.serviceID;
				info.EventID = data.eventID;
				wcscpy_s(info.serviceName, 256, data.stationName.c_str());
				wcscpy_s(info.bonDriverName, 256, this->bonFileName.c_str());
				info.bonDriverID = (this->tunerID & 0xFFFF0000)>>16;
				info.tunerID = this->tunerID & 0x0000FFFF;

				if( plugIn.ConvertRecName(&info, name, &size) == TRUE ){
					param.fileName = name;
				}
			}
		}
		if( param.fileName.size() == 0 ){
			if( reserve->stratTime < nowTime ){
				SYSTEMTIME now;
				GetLocalTime(&now);
				//たぶん開始時間過ぎてる
				Format(param.fileName, L"%04d%02d%02d%02d%02d%02X%02X%02d-%s.ts",
					now.wYear,
					now.wMonth,
					now.wDay,
					now.wHour,
					now.wMinute,
					((this->tunerID & 0xFFFF0000)>>16),
					(this->tunerID & 0x0000FFFF),
					param.ctrlID,
					data.title.c_str()
					);
			}else{
				Format(param.fileName, L"%04d%02d%02d%02d%02d%02X%02X%02d-%s.ts",
					data.startTime.wYear,
					data.startTime.wMonth,
					data.startTime.wDay,
					data.startTime.wHour,
					data.startTime.wMinute,
					((this->tunerID & 0xFFFF0000)>>16),
					(this->tunerID & 0x0000FFFF),
					param.ctrlID,
					data.title.c_str()
					);
			}
		}

		if( param.ctrlID == reserve->partialCtrlID && reserve->partialCtrlID != 0 ){
			//部分受信同時録画用
			if( data.recSetting.partialRecFolder.size() == 0 ){
				REC_FILE_SET_INFO folderItem;
				folderItem.recFolder = this->recFolderPath;
				folderItem.writePlugIn = this->recWritePlugIn;

				param.saveFolder.push_back(folderItem);
			}else{
				WORD partialSID = 0;
				wstring partialName = L"";
				if( FindPartialService(data.originalNetworkID, data.transportStreamID, data.serviceID, &partialSID, &partialName) == FALSE ){
					partialSID = data.serviceID;
					partialName = data.stationName;
				}
				for( size_t j=0; j<data.recSetting.partialRecFolder.size(); j++ ){
					if( data.recSetting.partialRecFolder[j].recNamePlugIn.size() > 0 ){
						CReNamePlugInUtil plugIn;
						wstring plugInPath;
						GetModuleFolderPath(plugInPath);
						plugInPath += L"\\RecName\\";
						plugInPath += data.recSetting.partialRecFolder[j].recNamePlugIn;

						if( plugIn.Initialize(plugInPath.c_str()) == TRUE ){
							WCHAR name[512] = L"";
							DWORD size = 512;
							PLUGIN_RESERVE_INFO info;

							info.startTime = data.startTime;
							info.durationSec = data.durationSecond;
							wcscpy_s(info.eventName, 512, data.title.c_str());
							info.ONID = data.originalNetworkID;
							info.TSID = data.transportStreamID;
							info.SID = partialSID;
							info.EventID = 0xFFFF;
							wcscpy_s(info.serviceName, 256, partialName.c_str());
							wcscpy_s(info.bonDriverName, 256, this->bonFileName.c_str());
							info.bonDriverID = (this->tunerID & 0xFFFF0000)>>16;
							info.tunerID = this->tunerID & 0x0000FFFF;

							if( plugIn.ConvertRecName(&info, name, &size) == TRUE ){
								data.recSetting.partialRecFolder[j].recFileName = name;
							}
						}
					}
				}
				param.saveFolder = data.recSetting.partialRecFolder;
			}
		}else{
			//通常録画
			if( data.recSetting.recFolderList.size() == 0 ){
				REC_FILE_SET_INFO folderItem;
				folderItem.recFolder = this->recFolderPath;
				folderItem.writePlugIn = this->recWritePlugIn;

				param.saveFolder.push_back(folderItem);
			}else{
				for( size_t j=0; j<data.recSetting.recFolderList.size(); j++ ){
					if( data.recSetting.recFolderList[j].recNamePlugIn.size() > 0 ){
						CReNamePlugInUtil plugIn;
						wstring plugInPath;
						GetModuleFolderPath(plugInPath);
						plugInPath += L"\\RecName\\";
						plugInPath += data.recSetting.recFolderList[j].recNamePlugIn;

						if( plugIn.Initialize(plugInPath.c_str()) == TRUE ){
							WCHAR name[512] = L"";
							DWORD size = 512;
							PLUGIN_RESERVE_INFO info;

							info.startTime = data.startTime;
							info.durationSec = data.durationSecond;
							wcscpy_s(info.eventName, 512, data.title.c_str());
							info.ONID = data.originalNetworkID;
							info.TSID = data.transportStreamID;
							info.SID = data.serviceID;
							info.EventID = data.eventID;
							wcscpy_s(info.serviceName, 256, data.stationName.c_str());
							wcscpy_s(info.bonDriverName, 256, this->bonFileName.c_str());
							info.bonDriverID = (this->tunerID & 0xFFFF0000)>>16;
							info.tunerID = this->tunerID & 0x0000FFFF;

							if( plugIn.ConvertRecName(&info, name, &size) == TRUE ){
								data.recSetting.recFolderList[j].recFileName = name;
							}
						}
					}
				}
				param.saveFolder = data.recSetting.recFolderList;
			}
		}
		param.overWriteFlag = this->recOverWrite;
		param.pittariFlag = data.recSetting.pittariFlag;
		param.pittariONID = data.originalNetworkID;
		param.pittariTSID = data.transportStreamID;
		param.pittariSID = data.serviceID;
		param.pittariEventID = data.eventID;

		CheckFileName(param.fileName);

		DWORD durationSec = data.durationSecond;
		if( data.recSetting.continueRecFlag == 1 ){
			DWORD sumSec = 0;
			IsFindContinueReserve(reserve, &sumSec);
			durationSec += sumSec;
		}

		DWORD bitrate = 0;
		_GetBitrate(data.originalNetworkID, data.transportStreamID, data.serviceID, &bitrate);
		param.createSize = ((ULONGLONG)(bitrate/8)*1000) * durationSec;

		if( this->sendCtrl.SendViewStartRec(param) != CMD_SUCCESS){
			if( reserve->ctrlID[i] == reserve->mainCtrlID ){
				ret = FALSE;
			}
		}
	}

	if( this->twitterManager != NULL && sendNoyify == TRUE){
		this->twitterManager->SendTweet(TW_REC_START, &data, NULL, NULL);
	}

	if( this->notifyManager != NULL && sendNoyify == TRUE){
		wstring msg;
		Format(msg, L"%s %04d/%02d/%02d %02d:%02d:%02d\r\n%s", 
			data.stationName.c_str(),
			data.startTime.wYear,
			data.startTime.wMonth,
			data.startTime.wDay,
			data.startTime.wHour,
			data.startTime.wMinute,
			data.startTime.wSecond,
			data.title.c_str()
			);
		this->notifyManager->AddNotifyMsg(NOTIFY_UPDATE_REC_START, msg);
	}
	return ret;
}

BOOL CTunerBankCtrl::IsFindContinueReserve(RESERVE_WORK* reserve, DWORD* continueSec)
{
	if( reserve->continueRecFlag == 0 ){
		return FALSE;
	}
	BOOL ret = FALSE;

	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->reserveWork.begin(); itr != this->reserveWork.end(); itr++ ){
		if( itr->second->ONID == reserve->ONID &&
			itr->second->TSID == reserve->TSID &&
			itr->second->SID == reserve->SID &&
			itr->second->stratTime != reserve->stratTime
			){
				if( reserve->stratTime < itr->second->stratTime &&
					itr->second->stratTime <= reserve->endTime &&
					reserve->endTime < itr->second->endTime){
					//開始時間が現在の予約中にあるので連続
					RESERVE_DATA data;
					itr->second->reserveInfo->GetData(&data);
					*continueSec = data.durationSecond;
					if( itr->second->continueRecFlag == 1 ){
						DWORD sumSec = 0;
						IsFindContinueReserve(itr->second, &sumSec);
						*continueSec += sumSec;
					}
					ret = TRUE;
					break;
				}
		}
	}

	return ret;
}

BOOL CTunerBankCtrl::CloseTuner()
{
	this->tunerCtrl.CloseExe(this->processID);
	this->processID = 0;
	this->openTuner = FALSE;
	this->delayTime = 0;

	return TRUE;
}

void CTunerBankCtrl::AddEndReserve(RESERVE_WORK* reserve, DWORD endType, SET_CTRL_REC_STOP_RES_PARAM resVal)
{
	END_RESERVE_INFO* item = new END_RESERVE_INFO;
	item->reserveInfo = reserve->reserveInfo;
	item->tunerID = this->tunerID;
	item->reserveID = reserve->reserveID;
	item->endType = endType;
	item->recFilePath = resVal.recFilePath;
	item->drop = resVal.drop;
	item->scramble = resVal.scramble;

	endList.push_back(item);
}

void CTunerBankCtrl::GetEndReserve(map<DWORD, END_RESERVE_INFO*>* reserveMap)
{
	if( Lock() == FALSE ) return;

	for( size_t i=0; i<this->endList.size(); i++ ){

		reserveMap->insert(pair<DWORD, END_RESERVE_INFO*>(this->endList[i]->reserveID, this->endList[i]));
	}
	this->endList.clear();

	UnLock();
}

BOOL CTunerBankCtrl::IsRecWork()
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = FALSE;
	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->reserveWork.begin(); itr != this->reserveWork.end(); itr++ ){
		if( itr->second->recStartFlag == TRUE ){
			ret = TRUE;
			break;
		}
	}

	UnLock();

	return ret;
}

BOOL CTunerBankCtrl::IsOpenTuner()
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = this->openTuner;

	UnLock();

	return ret;
}

BOOL CTunerBankCtrl::GetCurrentChID(DWORD* currentChID)
{
	if( Lock() == FALSE ) return FALSE;

	DWORD ret = FALSE;
	if( this->openTuner == TRUE && this->epgCapWork == FALSE ){
		ret = TRUE;
		if( currentChID != NULL ){
			*currentChID = this->currentChID;
		}
	}

	UnLock();

	return ret;
}

BOOL CTunerBankCtrl::IsSuspendOK()
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = FALSE;

	multimap<LONGLONG, RESERVE_WORK*> sortList;
	this->GetCheckList(&sortList);

	BOOL viewMode = FALSE;
	SET_CH_INFO initCh;
	BOOL needOpenTuner = IsNeedOpenTuner(&sortList, &viewMode, &initCh);
	if( needOpenTuner == FALSE && this->epgCapWork == FALSE){
		ret = TRUE;
		if( this->openTuner == TRUE ){
			CloseTuner();
		}
	}
	UnLock();

	return ret;
}

BOOL CTunerBankCtrl::IsEpgCapOK(LONGLONG ngCapMin)
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = TRUE;
	if( this->openTuner == TRUE ){
		UnLock();
		return FALSE;
	}

	multimap<LONGLONG, RESERVE_WORK*> sortList;
	this->GetCheckList(&sortList);
	if( sortList.size() == 0 ){
		UnLock();
		return TRUE;
	}

	multimap<LONGLONG, RESERVE_WORK*>::iterator itr;
	itr = sortList.begin();
	LONGLONG startTime = itr->second->stratTime - itr->second->startMargine;

	if( startTime < GetNowI64Time() + (ngCapMin*60*I64_1SEC) ){
		ret = FALSE;
	}

	UnLock();

	return ret;
}

BOOL CTunerBankCtrl::IsEpgCapWorking()
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = this->epgCapWork;

	UnLock();
	return ret;
}

void CTunerBankCtrl::ClearEpgCapItem()
{
	if( Lock() == FALSE ) return;

	this->epgCapItem.clear();

	UnLock();
}

void CTunerBankCtrl::AddEpgCapItem(SET_CH_INFO info)
{
	if( Lock() == FALSE ) return;

	this->epgCapItem.push_back(info);

	UnLock();
}

void CTunerBankCtrl::StartEpgCap()
{
	if( Lock() == FALSE ) return;

	if( epgCapItem.size() == 0 ){
		UnLock();
		return ;
	}

	if( this->checkThread == NULL ){
		ResetEvent(this->checkStopEvent);
		this->checkThread = (HANDLE)_beginthreadex(NULL, 0, CheckReserveThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->checkThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->checkThread);
	}

	this->epgCapWork = TRUE;

	UnLock();
}

void CTunerBankCtrl::StopEpgCap()
{
	if( Lock() == FALSE ) return;

	this->epgCapWork = FALSE;

	UnLock();
}

//起動中のチューナーからEPGデータの検索
//戻り値：
// エラーコード
// val					[IN]取得番組
// resVal				[OUT]番組情報
BOOL CTunerBankCtrl::SearchEpgInfo(
	SEARCH_EPG_INFO_PARAM* val,
	EPGDB_EVENT_INFO* resVal
	)
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = FALSE;
	if( this->openTuner == TRUE && this->epgCapWork == FALSE ){
		if( this->sendCtrl.SendViewSearchEvent(val, resVal) == CMD_SUCCESS ){
			ret = TRUE;
		}
	}

	UnLock();

	return ret;
}

//起動中のチューナーから現在or次の番組情報を取得する
//戻り値：
// エラーコード
// val					[IN]取得番組
// resVal				[OUT]番組情報
DWORD CTunerBankCtrl::GetEventPF(
	GET_EPG_PF_INFO_PARAM* val,
	EPGDB_EVENT_INFO* resVal
	)
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = FALSE;
	if( this->openTuner == TRUE && this->epgCapWork == FALSE ){
		if( this->sendCtrl.SendViewGetEventPF(val, resVal) == CMD_SUCCESS ){
			ret = TRUE;
		}
	}

	UnLock();

	return ret;
}

LONGLONG CTunerBankCtrl::DelayTime()
{
	if( Lock() == FALSE ) return 0;

	LONGLONG ret = 0;
	if( this->openTuner == TRUE ){
		ret = this->delayTime;
	}

	UnLock();

	return ret;
}

BOOL CTunerBankCtrl::ReRec(DWORD reserveID, BOOL deleteFile)
{
	if( Lock() == FALSE ) return FALSE;

	LONGLONG nowTime = GetNowI64Time();
	nowTime += this->delayTime;

	BOOL ret = FALSE;
	map<DWORD, RESERVE_WORK*>::iterator itr;
	itr = this->createCtrlList.find(reserveID);
	if( itr != this->createCtrlList.end() ){
		for(size_t i=0; i<itr->second->ctrlID.size(); i++ ){
			SET_CTRL_REC_STOP_PARAM param;
			param.ctrlID = itr->second->ctrlID[i];
			param.saveErrLog = FALSE;
			SET_CTRL_REC_STOP_RES_PARAM resVal;
			this->sendCtrl.SendViewStopRec(param, &resVal);
			if( resVal.recFilePath.size() > 0 ){
				if(deleteFile == TRUE ){
					DeleteFile( resVal.recFilePath.c_str() );
				}
			}
		}
		//開始時間過ぎているので録画開始
		if( RecStart(nowTime, itr->second, FALSE) == TRUE ){
			itr->second->recStartFlag = TRUE;
		}
	}

	UnLock();

	return ret;
}

BOOL CTunerBankCtrl::GetRecFilePath(
	DWORD reserveID,
	wstring& filePath,
	DWORD* ctrlID,
	DWORD* processID
	)
{
	if( Lock() == FALSE ) return FALSE;
	BOOL ret = FALSE;

	map<DWORD, RESERVE_WORK*>::iterator itr;
	itr = this->createCtrlList.find(reserveID);
	if( itr != this->createCtrlList.end() ){
		wstring recFilePath = L"";
		if( this->sendCtrl.SendViewGetRecFilePath(itr->second->mainCtrlID, &recFilePath) == CMD_SUCCESS ){
			filePath = recFilePath;
			*ctrlID = itr->second->mainCtrlID;
			*processID = this->processID;
			ret = TRUE;
		}
	}
	UnLock();

	return ret;
}

