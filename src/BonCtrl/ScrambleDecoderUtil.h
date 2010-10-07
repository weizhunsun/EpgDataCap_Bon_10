﻿#pragma once

#include <Windows.h>

#include "../Common/Util.h"
#include "../Common/PathUtil.h"
#include "../Common/StringUtil.h"
#include "../Common/ErrDef.h"

#include "IB25Decoder.h"

class CScrambleDecoderUtil
{
public:
	CScrambleDecoderUtil(void);
	~CScrambleDecoderUtil(void);

	void UnLoadDll();

	void SetNetwork(WORD ONID, WORD TSID);

	BOOL Decode(BYTE* src, DWORD srcSize, BYTE** dest, DWORD* destSize);

	BOOL SetEmm(BOOL enable);

	DWORD GetEmmCount();

	BOOL GetLoadStatus(wstring& loadErrDll);

protected:
	wstring currentDll;
	wstring loadDll;

	IB25Decoder* decodeIF;
	IB25Decoder2* decodeIF2;
	HMODULE module;

	bool emmEnable;

protected:
	BOOL LoadDll(LPCWSTR dllPath);
};

