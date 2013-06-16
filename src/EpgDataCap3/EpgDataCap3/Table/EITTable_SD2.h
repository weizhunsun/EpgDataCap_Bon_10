﻿#pragma once

#include "../../../Common/Util.h"
#include "../Descriptor/DescriptorDef.h"

class CEITTable_SD2
{
public:
	typedef struct _EVENT_MAP_DATA{
		WORD event_id;
		BYTE hour;
		BYTE minute;
		DWORD duration;
		WORD a4table_eventID;
	}EVENT_MAP_DATA;
	typedef struct _EVENT_MAP_INFO{
		WORD descriptor_length;
		SYSTEMTIME start_day;
		vector<EVENT_MAP_DATA> eventList;
	} EVENT_MAP_INFO;
	BYTE table_id;
	BYTE section_syntax_indicator;
	WORD section_length;
	WORD service_id;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	WORD service_id2;
	WORD original_network_id;
	SYSTEMTIME start_time;
	SYSTEMTIME end_time;
	vector<EVENT_MAP_INFO*> eventMapList;
	DWORD crc32;

public:
	CEITTable_SD2(void);
	~CEITTable_SD2(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
protected:
	void Clear();


};
