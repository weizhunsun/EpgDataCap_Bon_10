﻿#pragma once

#include "EpgDBUtil.h"
#include "../../Common/TSPacketUtil.h"
#include "../../Common/TSBuffUtil.h"
#include "../../Common/Util.h"
#include "../../Common/ErrDef.h"
#include "../../Common/EpgDataCap3Def.h"

#include "./Table/TableUtil.h"

class CDecodeUtil
{
public:
	CDecodeUtil(void);
	~CDecodeUtil(void);

	void SetEpgDB(CEpgDBUtil* epgDBUtil);
	DWORD AddTSData(BYTE* data, DWORD dataSize);

	//解析データの現在のストリームＩＤを取得する
	//戻り値：
	// エラーコード
	//引数：
	// originalNetworkID		[OUT]現在のoriginalNetworkID
	// transportStreamID		[OUT]現在のtransportStreamID
	DWORD GetTSID(
		WORD* originalNetworkID,
		WORD* transportStreamID
		);

	//自ストリームのサービス一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetServiceListActual(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList
		);

	//ストリーム内の現在の時間情報を取得する
	//戻り値：
	// エラーコード
	//引数：
	// time				[OUT]ストリーム内の現在の時間
	DWORD GetNowTime(
		SYSTEMTIME* time
		);

	//PC時計を元としたストリーム時間との差を取得する
	//戻り値：
	// 差の秒数
	int GetTimeDelay(
		);

protected:
	CEpgDBUtil* epgDBUtil;

	//PID毎のバッファリング
	//キー PID
	map<WORD, CTSBuffUtil*> buffUtilMap;

	CPATTable* patInfo;
	CCATTable* catInfo;
	map<WORD, CPMTTable*> pmtMap;
	CNITTable* nitActualInfo;
	CSDTTable* sdtActualInfo;
	map<DWORD, CSDTTable*> sdtOtherMap;
	CTOTTable* totInfo;
	CTDTTable* tdtInfo;
	CBITTable* bitInfo;
	CSITTable* sitInfo;


	DWORD serviceListSize;
	SERVICE_INFO* serviceList;

	int delaySec;

protected:
	void Clear();
	void ClearBuff(WORD noClearPid);
	void ChangeTSIDClear(WORD noClearPid);

	BOOL CheckPAT(WORD PID, CPATTable* pat);
	BOOL CheckCAT(WORD PID, CCATTable* pat);
	BOOL CheckPMT(WORD PID, CPMTTable* pmt);
	BOOL CheckNIT(WORD PID, CNITTable* nit);
	BOOL CheckSDT(WORD PID, CSDTTable* sdt);
	BOOL CheckTOT(WORD PID, CTOTTable* tot);
	BOOL CheckTDT(WORD PID, CTDTTable* tdt);
	BOOL CheckEIT(WORD PID, CEITTable* eit);
	BOOL CheckCDT(WORD PID, CCDTTable* cdt);
	BOOL CheckSDTT(WORD PID, CSDTTTable* sdtt);
	BOOL CheckBIT(WORD PID, CBITTable* bit);
	BOOL CheckSIT(WORD PID, CSITTable* sit);

	//自ストリームのサービス一覧をSITから取得する
	//戻り値：
	// エラーコード
	//引数：
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetServiceListSIT(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList
		);

};
