﻿#pragma once

#include "../../Common/Util.h"
#include "ReserveInfo.h"

class CCheckRecFile
{
public:
	CCheckRecFile(void);
	~CCheckRecFile(void);

	void SetCheckFolder(vector<wstring>* chkFolder);
	void SetDeleteExt(vector<wstring>* delExt);

	void CheckFreeSpace(map<DWORD, CReserveInfo*>* chkReserve, wstring defRecFolder);
	void CheckFreeSpaceLive(RESERVE_DATA* reserve, wstring recFolder);
	
protected:
	vector<wstring> chkFolder;
	vector<wstring> delExt;

	typedef struct _TS_FILE_INFO{
		LONGLONG fileSize;
		LONGLONG fileTime;
		wstring filePath;
	}TS_FILE_INFO;

protected:
	void FindTsFileList(wstring findFolder, map<LONGLONG, TS_FILE_INFO>* findList);
};

