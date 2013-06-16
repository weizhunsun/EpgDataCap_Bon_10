﻿#pragma once

#include "../../Common/StructDef.h"
#include "EpgDBManager.h"

class CHTMLManager
{
public:
	CHTMLManager(void);
	~CHTMLManager(void);

	BOOL GetIndexPage(HTTP_STREAM* sendParam);
	BOOL GetReservePage(vector<RESERVE_DATA*>* list, int pageIndex, HTTP_STREAM* sendParam);
	BOOL GetReserveInfoPage(RESERVE_DATA* reserveData, wstring eventText, WORD preset, vector<TUNER_RESERVE_INFO>* tunerList, HTTP_STREAM* sendParam);
	BOOL GetReserveParam(RESERVE_DATA* reserveData, HTTP_STREAM* recvParam);
	BOOL GetReserveChgPage(HTTP_STREAM* sendParam, BOOL err = FALSE);
	BOOL GetReserveDelPage(HTTP_STREAM* sendParam, BOOL err = FALSE);

	BOOL GetRecInfoPage(vector<REC_FILE_INFO>* list, int pageIndex, HTTP_STREAM* sendParam);
	BOOL GetRecInfoDescPage(REC_FILE_INFO* recinfoData, HTTP_STREAM* sendParam);
	BOOL GetRecInfoDelPage(HTTP_STREAM* sendParam, BOOL err = FALSE);

	BOOL GetEpgPage(CEpgDBManager* epgDB, vector<RESERVE_DATA*>* reserveList, string param, HTTP_STREAM* sendParam);
	BOOL GetEpgInfoPage(CEpgDBManager* epgDB, vector<RESERVE_DATA*>* reserveList, vector<TUNER_RESERVE_INFO>* tunerList, string param, HTTP_STREAM* sendParam);
	BOOL GetAddReserveData(CEpgDBManager* epgDB, RESERVE_DATA* reserveData, string param);
	BOOL GetReserveAddPage(HTTP_STREAM* sendParam, BOOL err = FALSE);

	BOOL GetAddProgramReservePage(CEpgDBManager* epgDB, vector<TUNER_RESERVE_INFO>* tunerList, string param, HTTP_STREAM* sendParam);
	BOOL GetAddReservePgData(CEpgDBManager* epgDB, RESERVE_DATA* reserveData, string param);
protected:
	typedef struct _EVENT_ITEM{
		int colspan;
		int rowspan;
		string tableHtml;
		_EVENT_ITEM(void){
			colspan = 1;
			rowspan = 1;
			tableHtml = " ";
		}
		_EVENT_ITEM & operator= (const _EVENT_ITEM & o) {
			colspan=o.colspan;
			rowspan = o.rowspan;
			tableHtml = o.tableHtml;
			return *this;
		};
	}EVENT_TABLE;
	typedef struct _TIME_ITEM{
		SYSTEMTIME timeInfo;
		vector<EVENT_TABLE> eventTableList;
		_TIME_ITEM & operator= (const _TIME_ITEM & o) {
			timeInfo=o.timeInfo;
			eventTableList = o.eventTableList;
			return *this;
		};
	}TIME_TABLE;

protected:
	void LoadRecSetData(WORD preset, REC_SETTING_DATA* recSetData);
	void CreateRecSetForm(REC_SETTING_DATA* recSetData, vector<TUNER_RESERVE_INFO>* tunerList, string& htmlText);

	BOOL CreateDefEpgPage(CEpgDBManager* epgDB, vector<RESERVE_DATA*>* reserveList, int tab, int page, int date, string& htmlText);
	BOOL CreateCustEpgPage(CEpgDBManager* epgDB, vector<RESERVE_DATA*>* reserveList, int tab, int page, int date, string& htmlText);
	BOOL GetEpgErrPage(HTTP_STREAM* sendParam);

	BOOL CreateEpgMainTable(CEpgDBManager* epgDB, vector<LONGLONG>* viewServiceList, map<LONGLONG, TIME_TABLE>* timeMap, int minPx, int timeColumn, string& htmlText);
	BOOL CreateEpgWeekTable(vector<string>* dateList, map<LONGLONG, TIME_TABLE>* timeMap, int minPx, int timeColumn, string& htmlText);
	BOOL CreateHourTable(vector<EPGDB_EVENT_INFO*>* eventList, map<LONGLONG, RESERVE_DATA*>* reserveMap, map<int,string>* colorList, LONGLONG pageEndTime, int minPx, LONGLONG* startHour, EVENT_TABLE* eventTable);
};

