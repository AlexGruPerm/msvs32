#include "stdafx.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cassandra.h"
#include <iostream> 
#include <map>

/*
Author:

Yakushev Aleksey
 ugr@bk.ru
 yakushevaleksey@gmail.com
 telegram = @AlexGruPerm
 vk       = https://vk.com/id11099676
 linkedin = https://www.linkedin.com/comm/in/yakushevaleksey
 gihub    = https://github.com/AlexGruPerm
 bigdata  = https://yakushev-bigdata.blogspot.com
 oracle   = http://yakushev-oracle.blogspot.com

*/


typedef std::map<std::string, cass_int32_t> MapOfTickerSymbol;

typedef std::map<cass_int32_t, CassStatement*> MapOftickerStatement;
typedef std::map<cass_int32_t, CassStatement*> MapOftickerStatementTotal;
typedef std::map<cass_int32_t, CassStatement*> MapOftickerStatementDates;

class Mt4Cass
{
private:
	static Mt4Cass* instance;

	Mt4Cass(const char* casshosts);
	~Mt4Cass();

public:
	static Mt4Cass* getInstance(const char* casshosts);
	CassSession*    session = NULL;
	CassCluster*    cluster = NULL;
	CassFuture*     prepared_future    = NULL;

	CassStatement*  savetick_statement  = NULL;
	CassStatement*  cnt_total_statement = NULL;
	CassStatement*  cnt_dates_statement = NULL;

	size_t          i = 0;

	const CassPrepared* savetick_prepared = NULL;
	const char*         savetick_query = "insert into mts_src.ticks(ticker_id, ddate, db_tsunx, bid, ask) values(?, toDate(now()), toUnixTimestamp(now()), ?, ?);";

	const CassPrepared* cnt_total_prepared = NULL;
	const char*         cnt_total_query = "update mts_src.ticks_count_total set ticks_count=ticks_count+1 where ticker_id=?;";

	const CassPrepared* cnt_dates_prepared = NULL;
	const char*         cnt_dates_query = "update mts_src.ticks_count_days set ticks_count=ticks_count+1 where ticker_id=? and ddate=toDate(now());";

	MapOfTickerSymbol    SymbolTickerMap;

	MapOftickerStatement      TickerMapStmt;
	MapOftickerStatementTotal TickersMapTotal;
	MapOftickerStatementDates TickersMapDates;

};

Mt4Cass* Mt4Cass::instance = 0;

Mt4Cass* Mt4Cass::getInstance(const char* casshosts)
{
	if (instance == 0)
	{
	 instance = new Mt4Cass(casshosts);
	}
	return instance;
}

Mt4Cass::Mt4Cass(const char* casshosts) {
	session = cass_session_new();
	cluster = cass_cluster_new();
	cass_cluster_set_contact_points(cluster, casshosts);
	CassFuture* connect_future = cass_session_connect(session, cluster);
	cass_future_wait(connect_future);

	// 1)  insert
	prepared_future = cass_session_prepare(session, savetick_query);
	if (cass_future_error_code(prepared_future) != CASS_OK) {
		const char* message;
		size_t message_length;
		cass_future_error_message(prepared_future, &message, &message_length);
		cass_future_free(prepared_future);
	}
	else {
		savetick_prepared = cass_future_get_prepared(prepared_future);
		cass_future_free(prepared_future);
		savetick_statement = cass_prepared_bind(savetick_prepared);
	}

	// 2)  prepare for updates query - total
	prepared_future = cass_session_prepare(session, cnt_total_query);
	if (cass_future_error_code(prepared_future) != CASS_OK) {
		const char* message;
		size_t message_length;
		cass_future_error_message(prepared_future, &message, &message_length);
		cass_future_free(prepared_future);
	}
	else {
		cnt_total_prepared = cass_future_get_prepared(prepared_future);
		cass_future_free(prepared_future);
		cnt_total_statement = cass_prepared_bind(cnt_total_prepared);
	}

	// 3)  prepare for updates query - by date
	prepared_future = cass_session_prepare(session, cnt_dates_query);
	if (cass_future_error_code(prepared_future) != CASS_OK) {
		const char* message;
		size_t message_length;
		cass_future_error_message(prepared_future, &message, &message_length);
		cass_future_free(prepared_future);
	}
	else {
		cnt_dates_prepared = cass_future_get_prepared(prepared_future);
		cass_future_free(prepared_future);
		cnt_dates_statement = cass_prepared_bind(cnt_dates_prepared);
	}


const char*  all_tickers = "select ticker_code,ticker_id from mts_meta.tickers;";
CassStatement* stmt = cass_statement_new(all_tickers, 0);
CassFuture* all_tickers_future = cass_session_execute(session, stmt);
cass_statement_free(stmt);	                                           
const CassResult* result = cass_future_get_result(all_tickers_future); 
CassIterator* row_iterator = cass_iterator_from_result(result);        

const char*   ticker_code;
size_t        ticker_code_length;
cass_int32_t  ticker_id;

while (cass_iterator_next(row_iterator)) {
	const CassRow* row = cass_iterator_get_row(row_iterator);
	const CassValue* value = cass_row_get_column_by_name(row, "ticker_code");
	cass_value_get_string(value, &ticker_code, &ticker_code_length);
	cass_value_get_int32(cass_row_get_column_by_name(row, "ticker_id"), &ticker_id);
	std::string scode = ticker_code;
	SymbolTickerMap[scode] = ticker_id;

	TickerMapStmt[ticker_id]   = cass_prepared_bind(savetick_prepared);
	TickersMapTotal[ticker_id] = cass_prepared_bind(cnt_total_prepared);
	TickersMapDates[ticker_id] = cass_prepared_bind(cnt_dates_prepared);
}

cass_iterator_free(row_iterator);
}

Mt4Cass::~Mt4Cass() {
	cass_cluster_free(cluster);
	cass_session_free(session);
	cass_future_free(prepared_future);
	cass_statement_free(savetick_statement);
	cass_statement_free(cnt_total_statement);
	cass_statement_free(cnt_dates_statement);
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
  return TRUE;
}

void on_insert_tick(CassFuture* future, void* data) {
	CassError code = cass_future_error_code(future);
	if (code != CASS_OK) {
		const char* message;
		size_t message_length;
		cass_future_error_message(future, &message, &message_length);
	}
}

void on_update_total(CassFuture* future, void* data) {
	CassError code = cass_future_error_code(future);
	if (code != CASS_OK) {
		const char* message;
		size_t message_length;
		cass_future_error_message(future, &message, &message_length);
	}
}

void on_update_dates(CassFuture* future, void* data) {
	CassError code = cass_future_error_code(future);
	if (code != CASS_OK) {
		const char* message;
		size_t message_length;
		cass_future_error_message(future, &message, &message_length);
	}
}

_DLLAPI void savetick(const char* casshosts, const char* csymbol, double bid, double ask) {
	Mt4Cass* cass = Mt4Cass::getInstance(casshosts);

	cass_statement_bind_int32(cass-> TickerMapStmt[cass->SymbolTickerMap[csymbol]], 0, cass->SymbolTickerMap[csymbol]);                      
	cass_statement_bind_double(cass->TickerMapStmt[cass->SymbolTickerMap[csymbol]], 1, bid);
	cass_statement_bind_double(cass->TickerMapStmt[cass->SymbolTickerMap[csymbol]], 2, ask);

	CassFuture* future = cass_session_execute(cass->session, cass->TickerMapStmt[cass->SymbolTickerMap[csymbol]]);

	//Async call
	cass_future_set_callback(future, on_insert_tick, cass->session);
	cass_future_free(future);

	//update total count by tciker_id
	cass_statement_bind_int32(cass->TickersMapTotal[cass->SymbolTickerMap[csymbol]], 0, cass->SymbolTickerMap[csymbol]);

	//update total count by ticker-id and date
	cass_statement_bind_int32(cass->TickersMapDates[cass->SymbolTickerMap[csymbol]], 0, cass->SymbolTickerMap[csymbol]);

	CassFuture* future_total = cass_session_execute(cass->session, cass->TickersMapTotal[cass->SymbolTickerMap[csymbol]]);
	cass_future_set_callback(future_total, on_update_total, cass->session);
	cass_future_free(future_total);
	//cass_future_wait(future_total);

	CassFuture* future_dates = cass_session_execute(cass->session, cass->TickersMapDates[cass->SymbolTickerMap[csymbol]]);
	cass_future_set_callback(future_dates, on_update_dates, cass->session);
	cass_future_free(future_dates);
}