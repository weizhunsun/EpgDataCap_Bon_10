﻿#include "StdAfx.h"
#include "TSOut.h"

#include "../Common/TimeUtil.h"

CTSOut::CTSOut(void)
{
	this->lockEvent = _CreateEvent(FALSE, TRUE, NULL);

	this->chChangeFlag = FALSE;
	this->chChangeErr = FALSE;
	this->chChangeTime = 0;
	this->lastONID = 0xFFFF;
	this->lastTSID = 0xFFFF;

	this->epgUtil.Initialize(FALSE);

	this->decodeBuff = 0;
	this->decodeBuffSize = 0;
	this->deocdeBuffWriteSize = 0;

	this->enableDecodeFlag = TRUE;
	this->emmEnableFlag = FALSE;
	this->serviceOnlyFlag = FALSE;


	this->catUtil = NULL;

	this->nextCtrlID = 1;

	this->epgFile = NULL;
}


CTSOut::~CTSOut(void)
{
	if( this->epgFile != NULL ){
		CloseHandle(this->epgFile);
		DeleteFile(this->epgTempFilePath.c_str());
	}
	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}

	map<DWORD, COneServiceUtil*>::iterator itr;
	for( itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->serviceUtilMap.clear();

	map<WORD, CPMTUtil*>::iterator itrPmt;
	for( itrPmt = this->pmtUtilMap.begin(); itrPmt != this->pmtUtilMap.end(); itrPmt++ ){
		SAFE_DELETE(itrPmt->second);
	}
	this->pmtUtilMap.clear();

	SAFE_DELETE(this->catUtil);

	this->epgUtil.UnInitialize();

	SAFE_DELETE_ARRAY(this->decodeBuff);
}

BOOL CTSOut::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			OutputDebugString(L"◆CTSOut::Lock FALSE");
		return FALSE;
	}
	return TRUE;
}

void CTSOut::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

DWORD CTSOut::SetChChangeEvent(BOOL resetEpgUtil)
{
	if( Lock() == FALSE ) return ERR_FALSE;

	this->chChangeFlag = TRUE;
	this->chChangeErr = FALSE;
	this->chChangeTime = GetTimeCount();

	this->decodeUtil.UnLoadDll();

	SAFE_DELETE(this->catUtil);

	if( resetEpgUtil == TRUE ){
		this->epgUtil.UnInitialize();
		this->epgUtil.Initialize(FALSE);
	}

	UnLock();
	return NO_ERR;
}

BOOL CTSOut::IsChChanging(BOOL* chChgErr)
{
	if( Lock() == FALSE ) return ERR_FALSE;

	BOOL ret = this->chChangeFlag;
	if( chChgErr != NULL ){
		*chChgErr = this->chChangeErr;
	}

	if( this->chChangeTime == 0 ){
		this->chChangeTime = GetTimeCount();
	}else if( GetTimeCount() > this->chChangeTime + 15 ){
		ret = FALSE;
		if( this->chChangeFlag == TRUE ){
			if( chChgErr != NULL ){
				*chChgErr = TRUE;
			}
		}
	}

	UnLock();
	return ret;
}

void CTSOut::ResetChChange()
{
	this->lastONID = 0xFFFF;
	this->lastTSID = 0xFFFF;
}

BOOL CTSOut::GetStreamID(WORD* ONID, WORD* TSID)
{
	if( Lock() == FALSE ) return ERR_FALSE;

	BOOL ret = TRUE;
	if( this->chChangeFlag == TRUE ){
		ret = FALSE;
	}else{
		ret = this->epgUtil.GetTSID(ONID, TSID);
		if( ret == NO_ERR ){
			this->lastONID = *ONID;
			this->lastTSID = *TSID;
		}
	}
	UnLock();
	return ret;
}

DWORD CTSOut::AddTSBuff(TS_DATA* data)
{
	if( Lock() == FALSE ) return ERR_FALSE;

	if( data->size > this->decodeBuffSize ){
		SAFE_DELETE_ARRAY(this->decodeBuff);
		this->decodeBuff = new BYTE[data->size*2];
		this->decodeBuffSize = data->size*2;
	}
	this->deocdeBuffWriteSize = 0;

	BYTE* decodeData = NULL;
	DWORD decodeSize = 0;
	
	try{
		for( DWORD i=0; i<data->size; i+=188 ){
			CTSPacketUtil packet;
			if( packet.Set188TS(data->data + i, 188) == TRUE ){
				if( this->chChangeFlag == TRUE ){
					//チャンネル切り替え中
					//簡易パケット解析
					if( packet.transport_scrambling_control != 0 ){
						//スクランブルパケットなので解析できない
						continue;
					}
					if( packet.data_byteSize < 3 ){
						//サイズが小さすぎる
						continue;
					}
					if(packet.payload_unit_start_indicator == 1 && 
						packet.data_byte[0] == 0x00 &&
						packet.data_byte[1] == 0x00 &&
						packet.data_byte[2] == 0x01){
						//PES
						continue;
					}
					this->epgUtil.AddTSPacket(data->data + i, 188);

					WORD onid = 0xFFFF;
					WORD tsid = 0xFFFF;
					if( this->epgUtil.GetTSID(&onid, &tsid) == NO_ERR ){
						if( onid != this->lastONID || tsid != this->lastTSID ){
							OutputDebugString(L"★Ch Change Complete\r\n");
						_OutputDebugString(L"★Ch 0x%04X 0x%04X => 0x%04X 0x%04X\r\n", onid, tsid, this->lastONID, this->lastTSID);
							this->chChangeFlag = FALSE;
							this->chChangeErr = FALSE;
							this->lastONID = onid;
							this->lastTSID = tsid;
							this->epgUtil.ClearSectionStatus();
							if( this->decodeUtil.SetNetwork(onid, tsid) == FALSE ){
								OutputDebugString(L"★★Decode DLL load err\r\n");
								Sleep(100);
								this->decodeUtil.SetNetwork(onid, tsid);
							}
							this->decodeUtil.SetEmm(this->emmEnableFlag);
							ResetErrCount();
						}else if( GetTimeCount() > this->chChangeTime + 15 ){
							if( this->lastONID == onid && this->lastTSID == tsid ){
								this->chChangeFlag = FALSE;
								this->chChangeErr = FALSE;
								this->lastONID = onid;
								this->lastTSID = tsid;
								this->epgUtil.ClearSectionStatus();
								if( this->decodeUtil.SetNetwork(onid, tsid) == FALSE ){
									OutputDebugString(L"★★Decode DLL load err\r\n");
									Sleep(100);
									this->decodeUtil.SetNetwork(onid, tsid);
								}
								this->decodeUtil.SetEmm(this->emmEnableFlag);
								ResetErrCount();
							}else{
//								OutputDebugString(L"★Ch Change Err\r\n");
								//10秒以上たってるなら切り替わったとする
								this->chChangeErr = TRUE;
								//this->chChangeFlag = FALSE;
								this->lastONID = onid;
								this->lastTSID = tsid;
							}
							//this->epgUtil.ClearSectionStatus();
							//this->decodeUtil.SetNetwork(onid, tsid);
							//this->decodeUtil.SetEmm(this->emmEnableFlag);
							//ResetErrCount();
						}
					}
					else{
						if( GetTimeCount() > this->chChangeTime + 15 ){
							//15秒以上たってるなら切り替わったとする
							OutputDebugString(L"★GetTSID Err\r\n");
							//this->chChangeFlag = FALSE;
							this->chChangeErr = TRUE;
							this->lastONID = onid;
							this->lastTSID = tsid;
							//this->epgUtil.ClearSectionStatus();
							//this->decodeUtil.SetNetwork(onid, tsid);
							//this->decodeUtil.SetEmm(this->emmEnableFlag);
							//ResetErrCount();
						}
					}

				}else{
					//指定サービスに必要なPIDを解析
					if( packet.transport_scrambling_control == 0 ){
						//CAT
						if( packet.PID == 0x0001 ){
							if( this->catUtil == NULL ){
								this->catUtil = new CCATUtil;
							}
							if(this->catUtil->AddPacket(&packet) == TRUE){
								CheckNeedPID();
							}
						}
						//PMT
						if( packet.payload_unit_start_indicator == 1 && packet.data_byteSize > 0){
							BYTE pointer = packet.data_byte[0];
							if( pointer+1 < packet.data_byteSize ){
								if( packet.data_byte[1+pointer] == 0x02 ){
									//PMT
									map<WORD, CPMTUtil*>::iterator itrPmt;
									itrPmt = this->pmtUtilMap.find(packet.PID);
									if( itrPmt == this->pmtUtilMap.end() ){
										CPMTUtil* util = new CPMTUtil;
										this->pmtUtilMap.insert(pair<WORD, CPMTUtil*>(packet.PID, util));
										if( util->AddPacket(&packet) == TRUE ){
											CheckNeedPID();
										}
									}else{
										if( itrPmt->second->AddPacket(&packet) == TRUE ){
											CheckNeedPID();
										}
									}
								}
							}
						}else{
							//PMTの2パケット目かチェック
							map<WORD, CPMTUtil*>::iterator itrPmt;
							itrPmt = this->pmtUtilMap.find(packet.PID);
							if( itrPmt != this->pmtUtilMap.end() ){
								if( itrPmt->second->AddPacket(&packet) == TRUE ){
									CheckNeedPID();
								}
							}
						}
					}

					//デコード用のバッファ作成
					if( this->serviceOnlyFlag == FALSE ){
						//全サービス
						memcpy(this->decodeBuff + this->deocdeBuffWriteSize, data->data + i, 188);
						this->deocdeBuffWriteSize += 188;
					}else{
						//指定サービス
						if( IsNeedPID(&packet) == TRUE ){
							if( packet.PID == 0x0000 ){
								//PATなので必要なサービスのみに絞る
								BYTE* patBuff = NULL;
								DWORD patBuffSize = 0;
								if( patUtil.GetPacket(&patBuff, &patBuffSize) == TRUE ){
									memcpy(this->decodeBuff + this->deocdeBuffWriteSize, patBuff, patBuffSize);
									this->deocdeBuffWriteSize += patBuffSize;
								}
							}else{
								memcpy(this->decodeBuff + this->deocdeBuffWriteSize, data->data + i, 188);
								this->deocdeBuffWriteSize += 188;
							}
						}
					}
					if( this->epgFile != NULL ){
						if( packet.PID <= 0x0030 ){
							DWORD write=0;
							WriteFile(this->epgFile, data->data + i, 188, &write, NULL);
						}
					}
				}
			}else{
				if( this->chChangeFlag == TRUE ){
					if( GetTimeCount() > this->chChangeTime + 15 ){
						//15秒以上たってるなら切り替わったとする
						//OutputDebugString(L"★Ch Change Err NoPacket\r\n");
						//this->chChangeFlag = FALSE;
						this->chChangeErr = TRUE;
						this->lastONID = 0xFFFF;
						this->lastTSID = 0xFFFF;
						//this->epgUtil.ClearSectionStatus();
						//this->decodeUtil.SetNetwork(onid, tsid);
						//this->decodeUtil.SetEmm(this->emmEnableFlag);
						//ResetErrCount();
					}
				}
			}
		}
	}catch(...){
		_OutputDebugString(L"★★CTSOut::AddTSBuff Exception1");
		UnLock();
		return ERR_FALSE;
	}
	try{
		if( this->deocdeBuffWriteSize > 0 ){
			if( this->enableDecodeFlag == TRUE){
				//デコード必要

				if( decodeUtil.Decode(this->decodeBuff, this->deocdeBuffWriteSize, &decodeData, &decodeSize) == FALSE ){
					//デコード失敗
					decodeData = this->decodeBuff;
					decodeSize = this->deocdeBuffWriteSize;
				}else{
					if( decodeData == NULL || decodeSize == 0 ){
						decodeData = NULL;
						decodeSize = 0;
					}
				}
			}else{
				//デコードの必要なし
				decodeData = this->decodeBuff;
				decodeSize = this->deocdeBuffWriteSize;
			}
		}
	}catch(...){
		_OutputDebugString(L"★★CTSOut::AddTSBuff Exception2");
		UnLock();
		return ERR_FALSE;
	}
	
	//デコード済みのデータを解析させる
	try{
		for( DWORD i=0; i<decodeSize; i+=188 ){
			this->epgUtil.AddTSPacket(decodeData + i, 188);
		}
	}catch(...){
		_OutputDebugString(L"★★CTSOut::AddTSBuff Exception3");
		UnLock();
		return ERR_FALSE;
	}

	//各サービス処理にデータ渡す
	try{
		map<DWORD, COneServiceUtil*>::iterator itrService;
		for( itrService = serviceUtilMap.begin(); itrService != serviceUtilMap.end(); itrService++ ){
			itrService->second->AddTSBuff(decodeData, decodeSize);
		}
	}catch(...){
		_OutputDebugString(L"★★CTSOut::AddTSBuff Exception4");
		UnLock();
		return ERR_FALSE;
	}
	UnLock();
	return NO_ERR;
}

void CTSOut::CheckNeedPID()
{
	this->needPIDMap.clear();

	//PAT作成用のPMTリスト
	map<WORD, CCreatePATPacket::PROGRAM_PID_INFO> PIDMap;
	//NITのPID追加しておく
	CCreatePATPacket::PROGRAM_PID_INFO item;
	item.PMTPID = 0x10;
	item.SID = 0;
	PIDMap.insert(pair<WORD, CCreatePATPacket::PROGRAM_PID_INFO>(item.PMTPID,item));

	//各サービスのPMTを探す
	map<DWORD, COneServiceUtil*>::iterator itrService;
	for( itrService = serviceUtilMap.begin(); itrService != serviceUtilMap.end(); itrService++ ){
		if( itrService->second->GetSID() == 0xFFFF ){
			//全サービス対象
			map<WORD, CPMTUtil*>::iterator itrPmt;
			for( itrPmt = pmtUtilMap.begin(); itrPmt != pmtUtilMap.end(); itrPmt++ ){
				//PAT作成用のPMTリスト作成
				CCreatePATPacket::PROGRAM_PID_INFO item;
				item.PMTPID = itrPmt->first;
				item.SID = itrPmt->second->program_number;
				PIDMap.insert(pair<WORD, CCreatePATPacket::PROGRAM_PID_INFO>(item.PMTPID,item));

				//PMT記載のPIDを登録
				this->needPIDMap.insert(pair<WORD,WORD>(itrPmt->first, 0));
				map<WORD,WORD>::iterator itrPID;
				for( itrPID = itrPmt->second->PIDList.begin(); itrPID != itrPmt->second->PIDList.end(); itrPID++ ){
					this->needPIDMap.insert(pair<WORD,WORD>(itrPID->first, itrPID->second));
				}
			}
		}else{
			map<WORD, CPMTUtil*>::iterator itrPmt;
			for( itrPmt = pmtUtilMap.begin(); itrPmt != pmtUtilMap.end(); itrPmt++ ){
				if( itrService->second->GetSID() == itrPmt->second->program_number ){
					//PMT発見
					itrService->second->SetPmtPID(this->lastTSID, itrPmt->first);
					if( catUtil != NULL ){
						itrService->second->SetEmmPID(&catUtil->PIDList);
					}


					//PAT作成用のPMTリスト作成
					CCreatePATPacket::PROGRAM_PID_INFO item;
					item.PMTPID = itrPmt->first;
					item.SID = itrPmt->second->program_number;
					PIDMap.insert(pair<WORD, CCreatePATPacket::PROGRAM_PID_INFO>(item.PMTPID,item));

					//PMT記載のPIDを登録
					this->needPIDMap.insert(pair<WORD,WORD>(itrPmt->first, 0));
					map<WORD,WORD>::iterator itrPID;
					for( itrPID = itrPmt->second->PIDList.begin(); itrPID != itrPmt->second->PIDList.end(); itrPID++ ){
						this->needPIDMap.insert(pair<WORD,WORD>(itrPID->first, itrPID->second));
					}
				}
			}
		}
	}
	this->patUtil.SetParam(this->lastTSID, &PIDMap);

	//EMMのPID
	if( catUtil != NULL ){
		map<WORD,WORD>::iterator itrPID;
		for( itrPID = catUtil->PIDList.begin(); itrPID != catUtil->PIDList.end(); itrPID++ ){
			this->needPIDMap.insert(pair<WORD,WORD>(itrPID->first, itrPID->second));
		}
	}
}

BOOL CTSOut::IsNeedPID(CTSPacketUtil* packet)
{
	if( packet == NULL ){
		return FALSE;
	}
	if( packet->PID <= 0x30 ){
		return TRUE;
	}
	map<WORD,WORD>::iterator itrPID;
	itrPID = this->needPIDMap.find(packet->PID);
	if( itrPID != this->needPIDMap.end() ){
		return TRUE;
	}

	return FALSE;
}

//EPGデータの保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CTSOut::StartSaveEPG(
	wstring epgFilePath
	)
{
	if( Lock() == FALSE ) return FALSE;
	if( this->epgFile != NULL ){
		UnLock();
		return FALSE;
	}
	this->epgFilePath = epgFilePath;
	this->epgTempFilePath = epgFilePath;
	this->epgTempFilePath += L".tmp";

	_OutputDebugString(L"★%s\r\n", this->epgFilePath.c_str());
	_OutputDebugString(L"★%s\r\n", this->epgTempFilePath.c_str());

	BOOL ret = TRUE;
	this->epgFile = _CreateFile2(this->epgTempFilePath.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( this->epgFile == INVALID_HANDLE_VALUE ){
		this->epgFile = NULL;
		ret = FALSE;
		OutputDebugString(L"err\r\n");
	}

	UnLock();
	return ret;
}

//EPGデータの保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CTSOut::StopSaveEPG(
	BOOL copy
	)
{
	if( Lock() == FALSE ) return FALSE;
	if( this->epgFile == NULL ){
		UnLock();
		return FALSE;
	}

	CloseHandle(this->epgFile);
	this->epgFile = NULL;

	if( copy == TRUE ){
		CopyFile(this->epgTempFilePath.c_str(), this->epgFilePath.c_str(), FALSE );
	}
	DeleteFile(this->epgTempFilePath.c_str());

	this->epgFilePath = L"";
	this->epgTempFilePath = L"";

	UnLock();
	return TRUE;
}

//EPGデータの蓄積状態をリセットする
void CTSOut::ClearSectionStatus()
{
	if( Lock() == FALSE ) return ;

	this->epgUtil.ClearSectionStatus();

	UnLock();
	return ;
}

//EPGデータの蓄積状態を取得する
//戻り値：
// ステータス
//引数：
// l_eitFlag		[IN]L-EITのステータスを取得
EPG_SECTION_STATUS CTSOut::GetSectionStatus(
	BOOL l_eitFlag
	)
{
	if( Lock() == FALSE ) return EpgNoData;

	EPG_SECTION_STATUS status = this->epgUtil.GetSectionStatus(l_eitFlag);

	UnLock();
	return status;
}

//EMM処理の動作設定
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// enable		[IN] TRUE（処理する）、FALSE（処理しない）
BOOL CTSOut::SetEmm(
	BOOL enable
	)
{
	if( Lock() == FALSE ) return FALSE;

	BOOL err = this->decodeUtil.SetEmm(enable);
	this->emmEnableFlag = enable;

	UnLock();
	return err;
}

//EMM処理を行った数
//戻り値：
// 処理数
DWORD CTSOut::GetEmmCount()
{
	if( Lock() == FALSE ) return 0;

	DWORD count = this->decodeUtil.GetEmmCount();

	UnLock();
	return count;
}

//DLLのロード状態を取得
//戻り値：
// TRUE（ロードに成功している）、FALSE（ロードに失敗している）
//引数：
// loadErrDll		[OUT]ロードに失敗したDLLファイル名
BOOL CTSOut::GetLoadStatus(
	wstring& loadErrDll
	)
{
	if( Lock() == FALSE ) return 0;

	BOOL err = this->decodeUtil.GetLoadStatus(loadErrDll);

	UnLock();
	return err;
}

//自ストリームのサービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CTSOut::GetServiceListActual(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	if( Lock() == FALSE ) return 0;

	DWORD err = this->epgUtil.GetServiceListActual(serviceListSize, serviceList);

	UnLock();
	return err;
}

//次に使用する制御IDを取得する
//戻り値：
// id
DWORD CTSOut::GetNextID()
{
	DWORD nextID = 0xFFFFFFFF;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = this->serviceUtilMap.find(this->nextCtrlID);
	if( itr == this->serviceUtilMap.end() ){
		//存在しないIDなのでそのまま使用
		nextID = this->nextCtrlID;
		this->nextCtrlID++;
		if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
			this->nextCtrlID = 1;
		}
	}else{
		//一周した？
		for( DWORD i=1; i<0xFFFFFFFF; i++ ){
			//１から順番に存在しないIDを確認
			itr = this->serviceUtilMap.find(this->nextCtrlID);
			if( itr == this->serviceUtilMap.end() ){
				nextID = this->nextCtrlID;
				this->nextCtrlID++;
				if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
					this->nextCtrlID = 1;
				}
				break;
			}else{
				this->nextCtrlID++;
			}
			if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
				this->nextCtrlID = 1;
			}
		}
	}

	return nextID;
}

//TSストリーム制御用コントロールを作成する
//戻り値：
// エラーコード
//引数：
// id			[OUT]制御識別ID
BOOL CTSOut::CreateServiceCtrl(
	DWORD* id
	)
{
	if( Lock() == FALSE ) return FALSE;

	COneServiceUtil* serviceUtil = new COneServiceUtil;
	*id = GetNextID();

	serviceUtil->SetEpgUtil(&this->epgUtil);
	serviceUtil->SetID(*id);
	serviceUtil->SetBonDriver(bonFile);

	serviceUtilMap.insert(pair<DWORD, COneServiceUtil*>(*id, serviceUtil));

	UnLock();
	return TRUE;
}

//TSストリーム制御用コントロールを作成する
//戻り値：
// エラーコード
//引数：
// id			[IN]制御識別ID
BOOL CTSOut::DeleteServiceCtrl(
	DWORD id
	)
{
	if( Lock() == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	SAFE_DELETE(itr->second);
	serviceUtilMap.erase(itr);

	UnLock();
	return TRUE;
}

//制御対象のサービスを設定する
//戻り値：
// TRUE（成功）、FALSE（失敗
//引数：
// id			[IN]制御識別ID
// serviceID	[IN]対象サービスID
BOOL CTSOut::SetServiceID(
	DWORD id,
	WORD serviceID
	)
{
	if( Lock() == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	itr->second->SetSID(serviceID);
	CheckNeedPID();

	UnLock();
	return TRUE;
}

//UDPで送信を行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id			[IN]制御識別ID
// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
BOOL CTSOut::SendUdp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	if( Lock() == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	itr->second->SendUdp(sendList);

	UnLock();
	return TRUE;
}

//TCPで送信を行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id			[IN]制御識別ID
// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
BOOL CTSOut::SendTcp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	if( Lock() == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	itr->second->SendTcp(sendList);

	UnLock();
	return TRUE;
}

//EPGデータをコピーする
BOOL CTSOut::ConvertEpgInfo(EPG_EVENT_INFO* src, EPGDB_EVENT_INFO* dest)
{
	if( src == NULL || dest == NULL ){
		return FALSE;
	}

	dest->event_id = src->event_id;
	dest->StartTimeFlag = src->StartTimeFlag;
	dest->start_time = src->start_time;
	dest->DurationFlag = src->DurationFlag;
	dest->durationSec = src->durationSec;
	dest->freeCAFlag = src->freeCAFlag;

	if( src->shortInfo != NULL ){
		dest->shortInfo = new EPGDB_SHORT_EVENT_INFO;
		dest->shortInfo->event_name = src->shortInfo->event_name;
		dest->shortInfo->text_char = src->shortInfo->text_char;
	}
	if( src->extInfo != NULL ){
		dest->extInfo = new EPGDB_EXTENDED_EVENT_INFO;
		dest->extInfo->text_char = src->extInfo->text_char;
	}
	if( src->contentInfo != NULL ){
		dest->contentInfo = new EPGDB_CONTEN_INFO;
		for( BYTE i=0; i<src->contentInfo->listSize; i++ ){
			EPGDB_CONTENT_DATA item;
			item.content_nibble_level_1 = src->contentInfo->nibbleList[i].content_nibble_level_1;
			item.content_nibble_level_2 = src->contentInfo->nibbleList[i].content_nibble_level_2;
			item.user_nibble_1 = src->contentInfo->nibbleList[i].user_nibble_1;
			item.user_nibble_2 = src->contentInfo->nibbleList[i].user_nibble_2;
			dest->contentInfo->nibbleList.push_back(item);
		}
	}
	if( src->componentInfo != NULL ){
		dest->componentInfo = new EPGDB_COMPONENT_INFO;
		dest->componentInfo->stream_content = src->componentInfo->stream_content;
		dest->componentInfo->component_type = src->componentInfo->component_type;
		dest->componentInfo->component_tag = src->componentInfo->component_tag;
		dest->componentInfo->text_char = src->componentInfo->text_char;
	}
	if( src->audioInfo != NULL ){
		dest->audioInfo = new EPGDB_AUDIO_COMPONENT_INFO;
		for( WORD i=0; i<src->audioInfo->listSize; i++ ){
			EPGDB_AUDIO_COMPONENT_INFO_DATA item;
			item.stream_content = src->audioInfo->audioList[i].stream_content;
			item.component_type = src->audioInfo->audioList[i].component_type;
			item.component_tag = src->audioInfo->audioList[i].component_tag;
			item.simulcast_group_tag = src->audioInfo->audioList[i].simulcast_group_tag;
			item.ES_multi_lingual_flag = src->audioInfo->audioList[i].ES_multi_lingual_flag;
			item.main_component_flag = src->audioInfo->audioList[i].main_component_flag;
			item.quality_indicator = src->audioInfo->audioList[i].quality_indicator;
			item.sampling_rate = src->audioInfo->audioList[i].sampling_rate;
			item.text_char = src->audioInfo->audioList[i].text_char;
			dest->audioInfo->componentList.push_back(item);
		}
	}
	if( src->eventGroupInfo != NULL ){
		dest->eventGroupInfo = new EPGDB_EVENTGROUP_INFO;
		dest->eventGroupInfo->group_type = src->eventGroupInfo->group_type;
		for( BYTE i = 0; i<src->eventGroupInfo->event_count; i++ ){
			EPGDB_EVENT_DATA item;
			item.original_network_id = src->eventGroupInfo->eventDataList[i].original_network_id;
			item.transport_stream_id = src->eventGroupInfo->eventDataList[i].transport_stream_id;
			item.service_id = src->eventGroupInfo->eventDataList[i].service_id;
			item.event_id = src->eventGroupInfo->eventDataList[i].event_id;
			dest->eventGroupInfo->eventDataList.push_back(item);
		}
	}
	if( src->eventRelayInfo != NULL ){
		dest->eventRelayInfo = new EPGDB_EVENTGROUP_INFO;
		dest->eventRelayInfo->group_type = src->eventRelayInfo->group_type;
		for( BYTE i = 0; i<src->eventRelayInfo->event_count; i++ ){
			EPGDB_EVENT_DATA item;
			item.original_network_id = src->eventRelayInfo->eventDataList[i].original_network_id;
			item.transport_stream_id = src->eventRelayInfo->eventDataList[i].transport_stream_id;
			item.service_id = src->eventRelayInfo->eventDataList[i].service_id;
			item.event_id = src->eventRelayInfo->eventDataList[i].event_id;
			dest->eventRelayInfo->eventDataList.push_back(item);
		}
	}
	return TRUE;
}

//指定サービスの現在or次のEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CTSOut::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	EPGDB_EVENT_INFO* epgInfo
	)
{
	if( Lock() == FALSE ) return FALSE;

	EPG_EVENT_INFO* _epgInfo;
	DWORD err = this->epgUtil.GetEpgInfo(originalNetworkID, transportStreamID, serviceID, nextFlag, &_epgInfo);
	if( err == NO_ERR ){
		ConvertEpgInfo(_epgInfo, epgInfo);
		epgInfo->original_network_id = originalNetworkID;
		epgInfo->transport_stream_id = transportStreamID;
		epgInfo->service_id = serviceID;
	}

	UnLock();
	return err;
}

//指定イベントのEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// eventID					[IN]取得対象のEventID
// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CTSOut::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPGDB_EVENT_INFO* epgInfo
	)
{
	if( Lock() == FALSE ) return FALSE;

	EPG_EVENT_INFO* _epgInfo;
	DWORD err = this->epgUtil.SearchEpgInfo(originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, &_epgInfo);
	if( err == NO_ERR ){
		ConvertEpgInfo(_epgInfo, epgInfo);
		epgInfo->original_network_id = originalNetworkID;
		epgInfo->transport_stream_id = transportStreamID;
		epgInfo->service_id = serviceID;
	}

	UnLock();
	return err;
}

//PC時計を元としたストリーム時間との差を取得する
//戻り値：
// 差の秒数
int CTSOut::GetTimeDelay(
	)
{
	if( Lock() == FALSE ) return 0;

	int delay = this->epgUtil.GetTimeDelay();

	UnLock();
	return delay;
}

//録画中かどうか
//戻り値：
// TRUE（録画中）、FALSE（していない）
BOOL CTSOut::IsRec()
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = FALSE;
	map<DWORD, COneServiceUtil*>::iterator itr;
	for( itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		if( itr->second->IsRec() == TRUE ){
			ret = TRUE;
			break;
		}
	}

	UnLock();
	return ret;
}

//ファイル保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id					[IN]制御識別ID
// fileName				[IN]保存ファイルパス
// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
// pittariFlag			[IN]ぴったりモード（TRUE：する、FALSE：しない）
// pittariONID			[IN]ぴったりモードで録画するONID
// pittariTSID			[IN]ぴったりモードで録画するTSID
// pittariSID			[IN]ぴったりモードで録画するSID
// pittariEventID		[IN]ぴったりモードで録画するイベントID
// createSize			[IN]ファイル作成時にディスクに予約する容量
// saveFolder			[IN]使用するフォルダ一覧
// saveFolderSub		[IN]HDDの空きがなくなった場合に一時的に使用するフォルダ
BOOL CTSOut::StartSave(
	DWORD id,
	wstring fileName,
	BOOL overWriteFlag,
	BOOL pittariFlag,
	WORD pittariONID,
	WORD pittariTSID,
	WORD pittariSID,
	WORD pittariEventID,
	ULONGLONG createSize,
	vector<REC_FILE_SET_INFO>* saveFolder,
	vector<wstring>* saveFolderSub
)
{
	if( Lock() == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	BOOL ret = itr->second->StartSave(fileName, overWriteFlag, pittariFlag, pittariONID, pittariTSID, pittariSID, pittariEventID, createSize, saveFolder, saveFolderSub);

	UnLock();
	return ret;
}

//ファイル保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id			[IN]制御識別ID
BOOL CTSOut::EndSave(
	DWORD id
	)
{
	if( Lock() == FALSE ) return FALSE;
	BOOL ret = TRUE;
	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	ret = itr->second->EndSave();

	UnLock();
	return ret;
}

//スクランブル解除処理の動作設定
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// enable		[IN] TRUE（処理する）、FALSE（処理しない）
BOOL CTSOut::SetScramble(
	DWORD id,
	BOOL enable
	)
{
	if( Lock() == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	itr->second->SetScramble(enable);

	BOOL enableScramble = FALSE;
	for( itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		if( itr->second->GetScramble() == TRUE ){
			enableScramble = TRUE;
			break;
		}
	}

	this->enableDecodeFlag = enableScramble;

	UnLock();
	return TRUE;
}

//字幕とデータ放送含めるかどうか
//引数：
// id					[IN]制御識別ID
// enableCaption		[IN]字幕を TRUE（含める）、FALSE（含めない）
// enableData			[IN]データ放送を TRUE（含める）、FALSE（含めない）
void CTSOut::SetServiceMode(
	DWORD id,
	BOOL enableCaption,
	BOOL enableData
	)
{
	if( Lock() == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->SetServiceMode(enableCaption, enableData);

	UnLock();
}

//エラーカウントをクリアする
//引数：
// id				[IN]制御識別ID
void CTSOut::ClearErrCount(
	DWORD id
	)
{
	if( Lock() == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->ClearErrCount();

	UnLock();
}

//ドロップとスクランブルのカウントを取得する
//引数：
// id				[IN]制御識別ID
// drop				[OUT]ドロップ数
// scramble			[OUT]スクランブル数
void CTSOut::GetErrCount(
	DWORD id,
	ULONGLONG* drop,
	ULONGLONG* scramble
	)
{
	if( Lock() == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->GetErrCount(drop, scramble);

	UnLock();
}


//録画中のファイルの出力サイズを取得する
//引数：
// id					[IN]制御識別ID
// writeSize			[OUT]保存ファイル名
void CTSOut::GetRecWriteSize(
	DWORD id,
	__int64* writeSize
	)
{
	if( Lock() == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->GetRecWriteSize(writeSize);

	UnLock();
}

void CTSOut::ResetErrCount()
{
	map<DWORD, COneServiceUtil*>::iterator itr;
	for( itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->ClearErrCount();
	}
}

//録画中のファイルのファイルパスを取得する
//引数：
// id					[IN]制御識別ID
// filePath				[OUT]保存ファイル名
// subRecFlag			[OUT]サブ録画が発生したかどうか
void CTSOut::GetSaveFilePath(
	DWORD id,
	wstring* filePath,
	BOOL* subRecFlag
	)
{
	if( Lock() == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->GetSaveFilePath(filePath, subRecFlag);

	UnLock();
}

//ドロップとスクランブルのカウントを保存する
//引数：
// id					[IN]制御識別ID
// filePath				[IN]保存ファイル名
void CTSOut::SaveErrCount(
	DWORD id,
	wstring filePath
	)
{
	if( Lock() == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->SaveErrCount(filePath);

	UnLock();
}

void CTSOut::SetSignalLevel(
	float signalLv
	)
{
	if( Lock() == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	for( itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->SetSignalLevel(signalLv);
	}

	UnLock();
}


void CTSOut::SetBonDriver(
	wstring bonDriver
	)
{
	if( Lock() == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	for( itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->SetBonDriver(bonDriver);
	}
	bonFile = bonDriver;

	UnLock();
}

