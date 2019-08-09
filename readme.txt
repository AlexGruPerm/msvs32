Cassandra cpp drivers

https://downloads.datastax.com/cpp-driver/windows/cassandra/
on 09.08.2019 last version is v2.13.0

This day I made decision to remove ts column in mts_src.ticks and rewrite dll also for this.

=============   STEPS DESCRIPTION =====================================

cqlsh 192.168.122.200
COPY MTS_SRC.TICKS(ticker_id,ddate,db_tsunx,ask,bid) TO '/root/ticks.csv' WITH HEADER=FALSE AND PAGETIMEOUT=40 AND PAGESIZE=20;

DROP TABLE MTS_SRC.TICKS;

CREATE TABLE MTS_SRC.TICKS(
	ticker_id int,
	ddate date,
	db_tsunx bigint,
	ask double,
	bid double,
	PRIMARY KEY (( ticker_id, ddate ), db_tsunx)
) WITH CLUSTERING ORDER BY (db_tsunx DESC);

cqlsh 192.168.122.200
COPY MTS_SRC.TICKS(ticker_id,ddate,db_tsunx,ask,bid) FROM '/root/ticks.csv' WITH HEADER=FALSE AND PAGETIMEOUT=40 AND PAGESIZE=20;

/*=============================================================================================*/
#property version "1.0"
#import "kernel32.dll"   
    
int GetModuleHandleA(string lpString);
int FreeLibrary(int hModule);  
int LoadLibraryA(string lpString);
 
#import "mt4cass.dll"  
void savetick(char&[],char&[], double pBid, double pAsk);
#import

char hosts[10240];
char cSymbol[10240];

int OnInit(){
   StringToCharArray("84.201.169.181", hosts);
   Print("OnInit Symbol = "+Symbol());
   StringToCharArray(Symbol(), cSymbol);
   return(INIT_SUCCEEDED);
}

void OnTick(){
 savetick(hosts,cSymbol,Bid,Ask);
}
/*=============================================================================================*/

