﻿#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include "../../Common/Util.h"
#include "../../Common/StringUtil.h"
#pragma comment(lib, "wsock32.lib")

class CSendTSTCPMain
{
public:
	CSendTSTCPMain(void);
	~CSendTSTCPMain(void);

	//DLLの初期化
	//戻り値：TRUE:成功、FALSE:失敗
	BOOL Initialize(
		);

	//DLLの開放
	//戻り値：なし
	void UnInitialize(
		);

	//送信先を追加
	//戻り値：エラーコード
	DWORD AddSendAddr(
		LPCWSTR lpcwszIP,
		DWORD dwPort
		);

	//送信先クリア
	//戻り値：エラーコード
	DWORD ClearSendAddr(
		);

	//データ送信を開始
	//戻り値：エラーコード
	DWORD StartSend(
		);

	//データ送信を停止
	//戻り値：エラーコード
	DWORD StopSend(
		);

	//データ送信を開始
	//戻り値：エラーコード
	DWORD AddSendData(
		BYTE* pbData,
		DWORD dwSize
		);

	//送信バッファをクリア
	//戻り値：エラーコード
	DWORD ClearSendBuff(
		);


protected:
	HANDLE m_hStopConnectEvent;
	HANDLE m_hConnectThread;
	HANDLE m_hStopSendEvent;
	HANDLE m_hSendThread;

	HANDLE m_hEvent;
	HANDLE m_hEvent2;

	typedef struct _TS_DATA{
		BYTE* pbBuff;
		DWORD dwSize;
		_TS_DATA(void){
			pbBuff = NULL;
			dwSize = 0;
		}
		~_TS_DATA(void){
			SAFE_DELETE_ARRAY(pbBuff);
		}
	}TS_DATA;
	vector<TS_DATA*> m_TSBuff;

	typedef struct _SEND_INFO{
		wstring strIP;
		DWORD dwIP;
		DWORD dwPort;
		SOCKET sock;
		struct sockaddr_in addr;
		BOOL bConnect;
	}SEND_INFO;
	map<wstring, SEND_INFO> m_SendList;

protected:
	BOOL Lock();
	void UnLock();
	BOOL Lock2();
	void UnLock2();

	static UINT WINAPI SendThread(LPVOID pParam);
	static UINT WINAPI ConnectThread(LPVOID pParam);

};
