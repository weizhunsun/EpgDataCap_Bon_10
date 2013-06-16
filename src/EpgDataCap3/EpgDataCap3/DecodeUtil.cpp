﻿#include "StdAfx.h"
#include "DecodeUtil.h"

#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "ARIB8CharDecode.h"

CDecodeUtil::CDecodeUtil(void)
{
	this->epgDBUtil = NULL;

	this->patInfo = NULL;
	this->catInfo = NULL;
	this->nitActualInfo = NULL;
	this->sdtActualInfo = NULL;
	this->totInfo = NULL;
	this->tdtInfo = NULL;
	this->bitInfo = NULL;
	this->sitInfo = NULL;

	this->serviceListSize = 0;
	this->serviceList = NULL;

	this->delaySec = 0;
}

CDecodeUtil::~CDecodeUtil(void)
{
	Clear();
	SAFE_DELETE_ARRAY(this->serviceList);
}

void CDecodeUtil::SetEpgDB(CEpgDBUtil* epgDBUtil)
{
	this->epgDBUtil = epgDBUtil;
}

void CDecodeUtil::Clear()
{
	map<WORD, CTSBuffUtil*>::iterator itr;
	for( itr = this->buffUtilMap.begin(); itr != this->buffUtilMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->buffUtilMap.clear();

	SAFE_DELETE(this->patInfo);
	SAFE_DELETE(this->catInfo);

	map<WORD, CPMTTable*>::iterator itrPmt;
	for( itrPmt = this->pmtMap.begin(); itrPmt != this->pmtMap.end(); itrPmt++ ){
		SAFE_DELETE(itrPmt->second);
	}
	this->pmtMap.clear();

	SAFE_DELETE(this->nitActualInfo);
	SAFE_DELETE(this->sdtActualInfo);
	map<DWORD, CSDTTable*>::iterator itrSdt;
	for( itrSdt = this->sdtOtherMap.begin(); itrSdt != this->sdtOtherMap.end(); itrSdt++ ){
		SAFE_DELETE(itrSdt->second);
	}
	this->sdtOtherMap.clear();

	SAFE_DELETE(this->totInfo);
	SAFE_DELETE(this->tdtInfo);
	SAFE_DELETE(this->bitInfo);
	SAFE_DELETE(this->sitInfo);

	if( this->epgDBUtil != NULL ){
		this->epgDBUtil->SetStreamChangeEvent();
		this->epgDBUtil->ClearSectionStatus();
	}

	this->delaySec = 0;
}

void CDecodeUtil::ClearBuff(WORD noClearPid)
{
	map<WORD, CTSBuffUtil*>::iterator itr;

	itr = this->buffUtilMap.begin();
	while( itr != this->buffUtilMap.end() ){
		if( itr->first != noClearPid ){
			SAFE_DELETE(itr->second);
			this->buffUtilMap.erase(itr++);
		}else{
			itr++;
		}
	}
}

void CDecodeUtil::ChangeTSIDClear(WORD noClearPid)
{
	ClearBuff(noClearPid);

	SAFE_DELETE(this->patInfo);
	SAFE_DELETE(this->catInfo);

	map<WORD, CPMTTable*>::iterator itrPmt;
	for( itrPmt = this->pmtMap.begin(); itrPmt != this->pmtMap.end(); itrPmt++ ){
		SAFE_DELETE(itrPmt->second);
	}
	this->pmtMap.clear();

	SAFE_DELETE(this->nitActualInfo);
	SAFE_DELETE(this->sdtActualInfo);
	map<DWORD, CSDTTable*>::iterator itrSdt;
	for( itrSdt = this->sdtOtherMap.begin(); itrSdt != this->sdtOtherMap.end(); itrSdt++ ){
		SAFE_DELETE(itrSdt->second);
	}
	this->sdtOtherMap.clear();

	SAFE_DELETE(this->totInfo);
	SAFE_DELETE(this->tdtInfo);
	SAFE_DELETE(this->bitInfo);
	SAFE_DELETE(this->sitInfo);

	if( this->epgDBUtil != NULL ){
		this->epgDBUtil->SetStreamChangeEvent();
		this->epgDBUtil->ClearSectionStatus();
	}

	this->delaySec = 0;
}

DWORD CDecodeUtil::AddTSData(BYTE* data, DWORD dataSize)
{
	DWORD err = TRUE;

	for( DWORD i=0; i<dataSize; i+=188 ){
		CTSPacketUtil tsPacket;
		if( tsPacket.Set188TS(data + i, 188) == TRUE ){
			if( tsPacket.PID == 0x1FFF ){
				continue;
			}
			CTSBuffUtil* buffUtil = NULL;

			map<WORD, CTSBuffUtil*>::iterator itr;
			itr = this->buffUtilMap.find( tsPacket.PID );
			if( itr == this->buffUtilMap.end() ){
				//まだPIDがないので新規
				buffUtil = new CTSBuffUtil;
				if( this->buffUtilMap.insert(pair<WORD, CTSBuffUtil*>(tsPacket.PID, buffUtil)).second== false ){
					//追加に失敗？
					SAFE_DELETE(buffUtil);
					continue;
				}
			}else{
				buffUtil = itr->second;
			}
			if( buffUtil->Add188TS(&tsPacket) == TRUE ){
				BYTE* section = NULL;
				DWORD sectionSize = 0;
				while( buffUtil->GetSectionBuff( &section, &sectionSize ) == TRUE ){
					if( buffUtil->IsPES() == TRUE ){
						continue;
					}
					CTableUtil tableUtil;

					vector<TABLE_DATA*> tableList;
					DWORD decodeReadSize = 0;
					if( tableUtil.Decode(section, sectionSize, &tableList, &decodeReadSize) == TRUE ){
						for( size_t j=0; j<tableList.size(); j++ ){
							if( tableList[j]->PATTable != NULL ){
								if( CheckPAT(tsPacket.PID, tableList[j]->PATTable) == TRUE ){
									tableList[j]->PATTable = NULL;
								}
							}else if( tableList[j]->CATTable != NULL ){
								if( CheckCAT(tsPacket.PID, tableList[j]->CATTable) == TRUE ){
									tableList[j]->CATTable = NULL;
								}
							}else if( tableList[j]->PMTTable != NULL ){
								if( CheckPMT(tsPacket.PID, tableList[j]->PMTTable) == TRUE ){
									tableList[j]->PMTTable = NULL;
								}
							}else if( tableList[j]->NITTable != NULL ){
								if( CheckNIT(tsPacket.PID, tableList[j]->NITTable) == TRUE ){
									tableList[j]->NITTable = NULL;
								}
							}else if( tableList[j]->SDTTable != NULL ){
								if( CheckSDT(tsPacket.PID, tableList[j]->SDTTable) == TRUE ){
									tableList[j]->SDTTable = NULL;
								}
							}else if( tableList[j]->TOTTable != NULL ){
								if( CheckTOT(tsPacket.PID, tableList[j]->TOTTable) == TRUE ){
									tableList[j]->TOTTable = NULL;
								}
							}else if( tableList[j]->TDTTable != NULL ){
								if( CheckTDT(tsPacket.PID, tableList[j]->TDTTable) == TRUE ){
									tableList[j]->TDTTable = NULL;
								}
							}else if( tableList[j]->EITTable != NULL ){
								if( CheckEIT(tsPacket.PID, tableList[j]->EITTable) == TRUE ){
									tableList[j]->EITTable = NULL;
								}
							}else if( tableList[j]->CDTTable != NULL ){
								if( CheckCDT(tsPacket.PID, tableList[j]->CDTTable) == TRUE ){
									tableList[j]->CDTTable = NULL;
								}
							}else if( tableList[j]->SDTTTable != NULL ){
								if( CheckSDTT(tsPacket.PID, tableList[j]->SDTTTable) == TRUE ){
									tableList[j]->SDTTTable = NULL;
								}
							}else if( tableList[j]->BITTable != NULL ){
								if( CheckBIT(tsPacket.PID, tableList[j]->BITTable) == TRUE ){
									tableList[j]->BITTable = NULL;
								}
							}else if( tableList[j]->SITTable != NULL ){
								if( CheckSIT(tsPacket.PID, tableList[j]->SITTable) == TRUE ){
									tableList[j]->SITTable = NULL;
								}
							}
							SAFE_DELETE(tableList[j]);
						}
					}else{
						if( section[0] == 0 ){
							_OutputDebugString(L"★pid 0x%04X\r\n", tsPacket.PID);
						}
					}
				}
			}
		}
	}
	return err;
}

BOOL CDecodeUtil::CheckPAT(WORD PID, CPATTable* pat)
{
	if( pat == NULL ){
		return FALSE;
	}

	if( this->patInfo == NULL ){
		//初回
		this->patInfo = pat;
	}else{
		if( this->patInfo->transport_stream_id != pat->transport_stream_id ){
			//TSID変わったのでチャンネル変わった
			ChangeTSIDClear(PID);
			this->patInfo = pat;
		}else if(this->patInfo->version_number != pat->version_number){
			//バージョン変わった
			SAFE_DELETE(this->patInfo);
			this->patInfo = pat;
		}else{
			//変更なし
			return FALSE;
		}
	}
	return TRUE;
}

BOOL CDecodeUtil::CheckCAT(WORD PID, CCATTable* cat)
{
	if( cat == NULL ){
		return FALSE;
	}

	if( this->catInfo == NULL ){
		//初回
		this->catInfo = cat;
	}else{
		if(this->catInfo->version_number != cat->version_number){
			//バージョン変わった
			SAFE_DELETE(this->catInfo);
			this->catInfo = cat;
		}else{
			//変更なし
			return FALSE;
		}
	}
	return TRUE;
}

BOOL CDecodeUtil::CheckPMT(WORD PID, CPMTTable* pmt)
{
	if( pmt == NULL ){
		return FALSE;
	}

	map<WORD, CPMTTable*>::iterator itrPmt;
	itrPmt = this->pmtMap.find(PID);
	if( itrPmt == this->pmtMap.end() ){
		//初回
		this->pmtMap.insert(pair<WORD, CPMTTable*>(PID, pmt));
	}else{
		if( itrPmt->second->version_number != pmt->version_number ){
			//バージョン変わった
			SAFE_DELETE(itrPmt->second);
			this->pmtMap.erase(itrPmt);
			
			this->pmtMap.insert(pair<WORD, CPMTTable*>(PID, pmt));
		}else{
			//変更なし
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CDecodeUtil::CheckNIT(WORD PID, CNITTable* nit)
{
	if( nit == NULL ){
		return FALSE;
	}

	if( epgDBUtil != NULL ){
		epgDBUtil->AddServiceList(nit);
	}

	if( nit->table_id == 0x40 ){
		//自ネットワーク
		if( this->nitActualInfo == NULL ){
			//初回
			this->nitActualInfo = nit;
		}else{
			if( this->nitActualInfo->network_id != nit->network_id ){
				//NID変わったのでネットワーク変わった
				ChangeTSIDClear(PID);
				this->nitActualInfo = nit;
			}else if(this->nitActualInfo->version_number != nit->version_number){
				//バージョン変わった
				SAFE_DELETE(this->nitActualInfo);
				this->nitActualInfo = nit;
			}else{
				if( this->nitActualInfo->TSInfoList.size() != 0 || nit->TSInfoList.size() != 0 ){
					if( this->nitActualInfo->TSInfoList[0]->original_network_id != nit->TSInfoList[0]->original_network_id ){
						//ONID変わったのでネットワーク変わった
						ChangeTSIDClear(PID);
						this->nitActualInfo = nit;
					}else{
						if( this->nitActualInfo->TSInfoList[0]->transport_stream_id != nit->TSInfoList[0]->transport_stream_id ){
							//TSID変わったのでネットワーク変わった
							ChangeTSIDClear(PID);
							this->nitActualInfo = nit;
						}else{
							//変化なし
							return FALSE;
						}
					}
				}else{
					//変化なし
					return FALSE;
				}
			}
		}
		/*
//		_OutputDebugString(L"find NIT\r\n");
		for( size_t i=0; i<nit->descriptorList.size(); i++ ){
			if( nit->descriptorList[i]->networkName != NULL ){
				if( nit->descriptorList[i]->networkName->char_nameLength > 0 ){
					CARIB8CharDecode arib;
					string network_name = "";
					arib.PSISI((const BYTE*)nit->descriptorList[i]->networkName->char_name, nit->descriptorList[i]->networkName->char_nameLength, &network_name);
					wstring network_nameW = L"";
					AtoW(network_name, network_nameW);
//					_OutputDebugString(L"%s\r\n", network_nameW.c_str());
				}
			}
		}
		for( size_t i=0; i<nit->TSInfoList.size(); i++ ){
			for( size_t j=0; j<nit->TSInfoList[i]->descriptorList.size(); j++ ){
				if( nit->TSInfoList[i]->descriptorList[j]->TSInfo != NULL ){
					CTSInfoDesc* TSInfo = nit->TSInfoList[i]->descriptorList[j]->TSInfo;
					CARIB8CharDecode arib;
					wstring ts_nameW = L"";
					if( TSInfo->length_of_ts_name > 0 ){
						string ts_name = "";
						arib.PSISI((const BYTE*)TSInfo->ts_name_char, TSInfo->length_of_ts_name, &ts_name);
						AtoW(ts_name, ts_nameW);
					}
//					_OutputDebugString(L"remote_control_key_id %d , %s\r\n", TSInfo->remote_control_key_id, ts_nameW.c_str());
				}
			}
		}
		*/
	}else if( nit->table_id == 0x41 ){
		//他ネットワーク
		//特に扱う必要性なし
		return FALSE;
	}else{
		return FALSE;
	}

	return TRUE;
}

BOOL CDecodeUtil::CheckSDT(WORD PID, CSDTTable* sdt)
{
	if( sdt == NULL ){
		return FALSE;
	}

	if( epgDBUtil != NULL ){
		epgDBUtil->AddSDT(sdt);
	}

	if( sdt->table_id == 0x42 ){
		//自ストリーム
		if( this->sdtActualInfo == NULL ){
			//初回
			this->sdtActualInfo = sdt;
		}else{
			if( this->sdtActualInfo->original_network_id != sdt->original_network_id ){
				//ONID変わったのでネットワーク変わった
				ChangeTSIDClear(PID);
				this->sdtActualInfo = sdt;
			}else if( this->sdtActualInfo->transport_stream_id != sdt->transport_stream_id ){
				//TSID変わったのでチャンネル変わった
				ChangeTSIDClear(PID);
				this->sdtActualInfo = sdt;
			}else if( this->sdtActualInfo->version_number != sdt->version_number ){
				//バージョン変わった
				SAFE_DELETE(this->sdtActualInfo);
				this->sdtActualInfo = sdt;
			}else{
				//変化なし
				return FALSE;
			}
		}
//		_OutputDebugString(L"find SDT\r\n");
//		_OutputDebugString(L"ONID 0x%04X, TSID 0x%04X\r\n", sdt->original_network_id, sdt->transport_stream_id);
		for(size_t i=0; i<sdt->serviceInfoList.size(); i++ ){
//			_OutputDebugString(L"SID 0x%04X\r\n", sdt->serviceInfoList[i]->service_id);
			for( size_t j=0; j<sdt->serviceInfoList[i]->descriptorList.size(); j++ ){
				if( sdt->serviceInfoList[i]->descriptorList[j]->service != NULL ){
					CServiceDesc* service = sdt->serviceInfoList[i]->descriptorList[j]->service;
					CARIB8CharDecode arib;
					string service_provider_name = "";
					string service_name = "";
					if( service->service_provider_name_length > 0 ){
						arib.PSISI((const BYTE*)service->char_service_provider_name, service->service_provider_name_length, &service_provider_name);
					}
					if( service->service_name_length > 0 ){
						arib.PSISI((const BYTE*)service->char_service_name, service->service_name_length, &service_name);
					}
/*					wstring service_provider_nameW = L"";
					wstring service_nameW = L"";
					AtoW(service_provider_name, service_provider_nameW);
					AtoW(service_name, service_nameW);
					_OutputDebugString(L"type 0x%04X %s %s\r\n", service->service_type, service_provider_nameW.c_str(), service_nameW.c_str());
*/				}
				//logo_transmission
			}
		}

	}else if( sdt->table_id == 0x46 ){
		//他ストリーム
/*		_OutputDebugString(L"find SDT OTHER\r\n");
		_OutputDebugString(L"ONID 0x%04X, TSID 0x%04X\r\n", sdt->original_network_id, sdt->transport_stream_id);
		for(size_t i=0; i<sdt->serviceInfoList.size(); i++ ){
			_OutputDebugString(L"SID 0x%04X\r\n", sdt->serviceInfoList[i]->service_id);
			for( size_t j=0; j<sdt->serviceInfoList[i]->descriptorList.size(); j++ ){
				if( sdt->serviceInfoList[i]->descriptorList[j]->service != NULL ){
					CServiceDesc* service = sdt->serviceInfoList[i]->descriptorList[j]->service;
					CARIB8CharDecode arib;
					string service_provider_name = "";
					string service_name = "";
					if( service->service_provider_name_length > 0 ){
						arib.PSISI((const BYTE*)service->char_service_provider_name, service->service_provider_name_length, &service_provider_name);
					}
					if( service->service_name_length > 0 ){
						arib.PSISI((const BYTE*)service->char_service_name, service->service_name_length, &service_name);
					}
					wstring service_provider_nameW = L"";
					wstring service_nameW = L"";
					AtoW(service_provider_name, service_provider_nameW);
					AtoW(service_name, service_nameW);
					_OutputDebugString(L"type 0x%04X %s %s\r\n", service->service_type, service_provider_nameW.c_str(), service_nameW.c_str());
				}
				//logo_transmission
			}
		}
*/
		DWORD key = ((DWORD)sdt->original_network_id)<<16 | sdt->transport_stream_id;
		map<DWORD, CSDTTable*>::iterator itr;
		itr = sdtOtherMap.find(key);
		if( itr == sdtOtherMap.end() ){
			sdtOtherMap.insert(pair<DWORD, CSDTTable*>(key,sdt));
		}else{
			if( itr->second->version_number != sdt->version_number ){
				SAFE_DELETE(itr->second);
				itr->second = sdt;
			}else{
				//変化なし
				return FALSE;
			}
		}
	}else{
		return FALSE;
	}

	return TRUE;
}

BOOL CDecodeUtil::CheckTOT(WORD PID, CTOTTable* tot)
{
	if( tot == NULL ){
		return FALSE;
	}

	SAFE_DELETE(this->totInfo);
	this->totInfo = tot;

	__int64 nowTime = GetNowI64Time();
	__int64 streamTime = ConvertI64Time( tot->jst_time );

	this->delaySec = (int)((streamTime - nowTime)/I64_1SEC);

/*	_OutputDebugString(L"%d/%02d/%02d %02d:%02d:%02d\r\n",
		tot->jst_time.wYear, 
		tot->jst_time.wMonth, 
		tot->jst_time.wDay, 
		tot->jst_time.wHour, 
		tot->jst_time.wMinute, 
		tot->jst_time.wSecond 
		);
		*/

	return TRUE;
}

BOOL CDecodeUtil::CheckTDT(WORD PID, CTDTTable* tdt)
{
	if( tdt == NULL ){
		return FALSE;
	}

	SAFE_DELETE(this->tdtInfo);
	this->tdtInfo = tdt;

	__int64 nowTime = GetNowI64Time();
	__int64 streamTime = ConvertI64Time( tdt->jst_time );

	this->delaySec = (int)((streamTime - nowTime)/I64_1SEC);
	/*
	_OutputDebugString(L"%d/%02d/%02d %02d:%02d:%02d\r\n",
		tdt->jst_time.wYear, 
		tdt->jst_time.wMonth, 
		tdt->jst_time.wDay, 
		tdt->jst_time.wHour, 
		tdt->jst_time.wMinute, 
		tdt->jst_time.wSecond 
		);*/
		
	return TRUE;
}

BOOL CDecodeUtil::CheckEIT(WORD PID, CEITTable* eit)
{
	if( eit == NULL ){
		return FALSE;
	}
	
	if( epgDBUtil != NULL ){
		epgDBUtil->AddEIT(PID, eit);
	}
	return FALSE;
}

BOOL CDecodeUtil::CheckCDT(WORD PID, CCDTTable* cdt)
{
	if( cdt == NULL ){
		return FALSE;
	}

	return FALSE;
}

BOOL CDecodeUtil::CheckSDTT(WORD PID, CSDTTTable* sdtt)
{
	if( sdtt == NULL ){
		return FALSE;
	}
	/*
	for( size_t i=0; i<sdtt->contentInfoList.size(); i++ ){
		for( size_t j=0; j<sdtt->contentInfoList[i]->descriptorList.size(); j++ ){
			if( sdtt->contentInfoList[i]->descriptorList[j]->downloadContent != NULL ){
				if( sdtt->contentInfoList[i]->descriptorList[j]->downloadContent->text_char != NULL ){
					CARIB8CharDecode arib;
					string dec;
					arib.PSISI(sdtt->contentInfoList[i]->descriptorList[j]->downloadContent->text_char, sdtt->contentInfoList[i]->descriptorList[j]->downloadContent->text_length, &dec);
					dec+="\r\n";
					OutputDebugStringA(dec.c_str());
				}
			}
		}
	}
	*/
	return FALSE;
}

BOOL CDecodeUtil::CheckBIT(WORD PID, CBITTable* bit)
{
	if( bit == NULL ){
		return FALSE;
	}

	if( this->bitInfo == NULL ){
		//初回
		this->bitInfo = bit;
	}else{
		if( this->bitInfo->original_network_id != bit->original_network_id ){
			//ONID変わったのでネットワーク変わった
			ChangeTSIDClear(PID);
			this->bitInfo = bit;
		}else if( this->bitInfo->version_number != bit->version_number ){
			//バージョン変わった
			SAFE_DELETE(this->bitInfo);
			this->bitInfo = bit;
		}else{
			//変化なし
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CDecodeUtil::CheckSIT(WORD PID, CSITTable* sit)
{
	if( sit == NULL ){
		return FALSE;
	}

	//時間計算
	if( this->totInfo == NULL && this->tdtInfo == NULL ){
		for( size_t i=0; i<sit->descriptorList.size(); i++ ){
			if( sit->descriptorList[i]->partialTSTime != NULL ){
				if( sit->descriptorList[i]->partialTSTime->jst_time_flag == 1 ){
					__int64 nowTime = GetNowI64Time();
					__int64 streamTime = ConvertI64Time( sit->descriptorList[i]->partialTSTime->jst_time );

					this->delaySec = (int)((streamTime - nowTime)/I64_1SEC);
				}
			}
		}
	}

	if( epgDBUtil != NULL ){
		if( this->patInfo != NULL ){
			epgDBUtil->AddServiceList(this->patInfo->transport_stream_id, sit);
		}
	}

	if( this->sitInfo == NULL ){
		//初回
		this->sitInfo = sit;
	}else{
		if( this->sitInfo->version_number != sit->version_number ){
			//バージョン変わった
			SAFE_DELETE(this->sitInfo);
			this->sitInfo = sit;
		}else{
			//変化なし
			return FALSE;
		}
	}

	return TRUE;
}

//解析データの現在のストリームＩＤを取得する
//戻り値：
// エラーコード
// originalNetworkID		[OUT]現在のoriginalNetworkID
// transportStreamID		[OUT]現在のtransportStreamID
DWORD CDecodeUtil::GetTSID(
	WORD* originalNetworkID,
	WORD* transportStreamID
	)
{
	if( sdtActualInfo != NULL ){
		*originalNetworkID = sdtActualInfo->original_network_id;
		*transportStreamID = sdtActualInfo->transport_stream_id;
		return NO_ERR;
	}else if( this->sitInfo != NULL && this->patInfo != NULL ){
		//TSID
		*transportStreamID = this->patInfo->transport_stream_id;
		//ONID
		WORD ONID = 0xFFFF;
		for( size_t i=0; i<this->sitInfo->descriptorList.size(); i++ ){
			if( this->sitInfo->descriptorList[i]->networkIdentification != NULL ){
				*originalNetworkID = this->sitInfo->descriptorList[i]->networkIdentification->network_id;
				return NO_ERR;
			}
		}
	}
	return ERR_FALSE;
}

//自ストリームのサービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CDecodeUtil::GetServiceListActual(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	SAFE_DELETE_ARRAY(this->serviceList);
	this->serviceListSize = 0;

	if( serviceListSize == NULL || serviceList == NULL ){
		return ERR_INVALID_ARG;
	}

	if( this->nitActualInfo == NULL || this->sdtActualInfo == NULL ){
		if( GetServiceListSIT(serviceListSize, serviceList) != NO_ERR){
			return ERR_FALSE;
		}else{
			return NO_ERR;
		}
	}

	this->serviceListSize = (DWORD)this->sdtActualInfo->serviceInfoList.size();
	this->serviceList = new SERVICE_INFO[this->serviceListSize];


	wstring network_nameW = L"";
	wstring ts_nameW = L"";
	BYTE remote_control_key_id = 0;
	vector<WORD> partialServiceList;

	for( size_t i=0; i<this->nitActualInfo->descriptorList.size(); i++ ){
		if( this->nitActualInfo->descriptorList[i]->networkName != NULL ){
			CNetworkNameDesc* networkName = this->nitActualInfo->descriptorList[i]->networkName;
			if( networkName->char_nameLength > 0 ){
				CARIB8CharDecode arib;
				string network_name = "";
				arib.PSISI((const BYTE*)networkName->char_name, networkName->char_nameLength, &network_name);
				AtoW(network_name, network_nameW);
			}
		}
	}
	for( size_t i=0; i<this->nitActualInfo->TSInfoList.size(); i++ ){
		for( size_t j=0; j<this->nitActualInfo->TSInfoList[i]->descriptorList.size(); j++ ){
			if( this->nitActualInfo->TSInfoList[i]->descriptorList[j]->TSInfo != NULL ){
				CTSInfoDesc* TSInfo = this->nitActualInfo->TSInfoList[i]->descriptorList[j]->TSInfo;
				if( TSInfo->length_of_ts_name > 0 ){
					CARIB8CharDecode arib;
					string ts_name = "";
					arib.PSISI((const BYTE*)TSInfo->ts_name_char, TSInfo->length_of_ts_name, &ts_name);
					AtoW(ts_name, ts_nameW);
				}
				remote_control_key_id = TSInfo->remote_control_key_id;
			}
			if( this->nitActualInfo->TSInfoList[i]->descriptorList[j]->partialReception != NULL ){
				partialServiceList = this->nitActualInfo->TSInfoList[i]->descriptorList[j]->partialReception->service_idList;
			}
		}
	}

	for( DWORD i=0; i<this->serviceListSize; i++ ){
		this->serviceList[i].original_network_id = this->sdtActualInfo->original_network_id;
		this->serviceList[i].transport_stream_id = this->sdtActualInfo->transport_stream_id;
		this->serviceList[i].service_id = this->sdtActualInfo->serviceInfoList[i]->service_id;
		this->serviceList[i].extInfo = new SERVICE_EXT_INFO;

		for( size_t j=0; j<this->sdtActualInfo->serviceInfoList[i]->descriptorList.size(); j++ ){
			if( this->sdtActualInfo->serviceInfoList[i]->descriptorList[j]->service != NULL ){
				CServiceDesc* service = this->sdtActualInfo->serviceInfoList[i]->descriptorList[j]->service;
				CARIB8CharDecode arib;
				string service_provider_name = "";
				string service_name = "";
				if( service->service_provider_name_length > 0 ){
					arib.PSISI((const BYTE*)service->char_service_provider_name, service->service_provider_name_length, &service_provider_name);
				}
				if( service->service_name_length > 0 ){
					arib.PSISI((const BYTE*)service->char_service_name, service->service_name_length, &service_name);
				}
				wstring service_provider_nameW = L"";
				wstring service_nameW = L"";
				AtoW(service_provider_name, service_provider_nameW);
				AtoW(service_name, service_nameW);

				this->serviceList[i].extInfo->service_type = service->service_type;
				if( service_provider_nameW.size() > 0 ){
					this->serviceList[i].extInfo->service_provider_name = new WCHAR[service_provider_nameW.size()+1];
					wcscpy_s(this->serviceList[i].extInfo->service_provider_name, service_provider_nameW.size()+1, service_provider_nameW.c_str());
				}
				if( service_nameW.size() > 0 ){
					this->serviceList[i].extInfo->service_name = new WCHAR[service_nameW.size()+1];
					wcscpy_s(this->serviceList[i].extInfo->service_name, service_nameW.size()+1, service_nameW.c_str());
				}
			}
		}

		if( network_nameW.size() > 0 ){
			this->serviceList[i].extInfo->network_name = new WCHAR[network_nameW.size()+1];
			wcscpy_s(this->serviceList[i].extInfo->network_name, network_nameW.size()+1, network_nameW.c_str());
		}
		if( ts_nameW.size() > 0 ){
			this->serviceList[i].extInfo->ts_name = new WCHAR[ts_nameW.size()+1];
			wcscpy_s(this->serviceList[i].extInfo->ts_name, ts_nameW.size()+1, ts_nameW.c_str());
		}
		this->serviceList[i].extInfo->remote_control_key_id = remote_control_key_id;

		this->serviceList[i].extInfo->partialReceptionFlag = FALSE;
		for( size_t j=0; j<partialServiceList.size(); j++ ){
			if( partialServiceList[j] == this->serviceList[i].service_id ){
				this->serviceList[i].extInfo->partialReceptionFlag = TRUE;
			}
		}

	}

	*serviceListSize = this->serviceListSize;
	*serviceList = this->serviceList;


	return NO_ERR;
}

//自ストリームのサービス一覧をSITから取得する
//戻り値：
// エラーコード
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CDecodeUtil::GetServiceListSIT(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	if( this->sitInfo == NULL || this->patInfo == NULL ){
		return ERR_FALSE;
	}

	SAFE_DELETE_ARRAY(this->serviceList);
	this->serviceListSize = 0;

	//ONID
	WORD ONID = 0xFFFF;
	for( size_t i=0; i<this->sitInfo->descriptorList.size(); i++ ){
		if( this->sitInfo->descriptorList[i]->networkIdentification != NULL ){
			ONID = this->sitInfo->descriptorList[i]->networkIdentification->network_id;
		}
	}

	//TSID
	WORD TSID = 0xFFFF;
	TSID = this->patInfo->transport_stream_id;

	this->serviceListSize = (DWORD)this->sitInfo->serviceLoopList.size();
	this->serviceList = new SERVICE_INFO[this->serviceListSize];

	wstring network_nameW = L"";
	wstring ts_nameW = L"";
	BYTE remote_control_key_id = 0;

	//サービスリスト
	for( DWORD i=0; i<this->serviceListSize; i++ ){
		this->serviceList[i].original_network_id = ONID;
		this->serviceList[i].transport_stream_id = TSID;
		this->serviceList[i].service_id = this->sitInfo->serviceLoopList[i]->service_id;
		this->serviceList[i].extInfo = new SERVICE_EXT_INFO;

		for( size_t j=0; j<this->sitInfo->serviceLoopList[i]->descriptorList.size(); j++ ){
			if( this->sitInfo->serviceLoopList[i]->descriptorList[j]->service != NULL ){
				CServiceDesc* service = this->sitInfo->serviceLoopList[i]->descriptorList[j]->service;
				CARIB8CharDecode arib;
				string service_provider_name = "";
				string service_name = "";
				if( service->service_provider_name_length > 0 ){
					arib.PSISI((const BYTE*)service->char_service_provider_name, service->service_provider_name_length, &service_provider_name);
				}
				if( service->service_name_length > 0 ){
					arib.PSISI((const BYTE*)service->char_service_name, service->service_name_length, &service_name);
				}
				wstring service_provider_nameW = L"";
				wstring service_nameW = L"";
				AtoW(service_provider_name, service_provider_nameW);
				AtoW(service_name, service_nameW);

				this->serviceList[i].extInfo->service_type = service->service_type;
				if( service_provider_nameW.size() > 0 ){
					this->serviceList[i].extInfo->service_provider_name = new WCHAR[service_provider_nameW.size()+1];
					wcscpy_s(this->serviceList[i].extInfo->service_provider_name, service_provider_nameW.size()+1, service_provider_nameW.c_str());
				}
				if( service_nameW.size() > 0 ){
					this->serviceList[i].extInfo->service_name = new WCHAR[service_nameW.size()+1];
					wcscpy_s(this->serviceList[i].extInfo->service_name, service_nameW.size()+1, service_nameW.c_str());
				}
			}
		}

		if( network_nameW.size() > 0 ){
			this->serviceList[i].extInfo->network_name = new WCHAR[network_nameW.size()+1];
			wcscpy_s(this->serviceList[i].extInfo->network_name, network_nameW.size()+1, network_nameW.c_str());
		}
		if( ts_nameW.size() > 0 ){
			this->serviceList[i].extInfo->ts_name = new WCHAR[ts_nameW.size()+1];
			wcscpy_s(this->serviceList[i].extInfo->ts_name, ts_nameW.size()+1, ts_nameW.c_str());
		}
		this->serviceList[i].extInfo->remote_control_key_id = remote_control_key_id;

		this->serviceList[i].extInfo->partialReceptionFlag = FALSE;
	}


	*serviceListSize = this->serviceListSize;
	*serviceList = this->serviceList;

	return NO_ERR;
}

//ストリーム内の現在の時間情報を取得する
//戻り値：
// エラーコード
//引数：
// time				[OUT]ストリーム内の現在の時間
DWORD CDecodeUtil::GetNowTime(
	SYSTEMTIME* time
	)
{
	if( this->totInfo != NULL ){
		*time = this->totInfo->jst_time;
		return NO_ERR;
	}else if( this->tdtInfo != NULL ){
		*time = this->tdtInfo->jst_time;
		return NO_ERR;
	}else{
		if( this->sitInfo != NULL ){
			for( size_t i=0; i<this->sitInfo->descriptorList.size(); i++ ){
				if( this->sitInfo->descriptorList[i]->partialTSTime != NULL ){
					if( this->sitInfo->descriptorList[i]->partialTSTime->jst_time_flag == 1 ){
						*time = this->sitInfo->descriptorList[i]->partialTSTime->jst_time;
						return NO_ERR;
					}
				}
			}
		}
		return ERR_FALSE;
	}
}

//PC時計を元としたストリーム時間との差を取得する
//戻り値：
// 差の秒数
int CDecodeUtil::GetTimeDelay(
	)
{
	return this->delaySec;
}
