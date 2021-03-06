#include "DatabaseCursor.h"
#include "Query.h"
#include "sqlite/sqlite3.h"
#include "Server.h"

DatabaseCursor::DatabaseCursor(CQuery *query, int *timeoutms)
	: query(query), transaction_lock(false), tries(60), timeoutms(timeoutms), lastErr(SQLITE_OK), _has_error(false)
{
	query->setupStepping(timeoutms);

#ifdef LOG_READ_QUERIES
	active_query=new ScopedAddActiveQuery(query);
#endif
}

DatabaseCursor::~DatabaseCursor(void)
{
	query->shutdownStepping(lastErr, timeoutms, transaction_lock);

#ifdef LOG_READ_QUERIES
	delete active_query;
#endif
}

bool DatabaseCursor::next(db_single_result &res)
{
	res.clear();
	do
	{
		bool reset=false;
		lastErr=query->step(res, timeoutms, tries, transaction_lock, reset);
		//TODO handle reset (should not happen in WAL mode)
		if(lastErr==SQLITE_ROW)
		{
			return true;
		}
	}
	while(query->resultOkay(lastErr));

	if(lastErr!=SQLITE_DONE)
	{
		Server->Log("SQL Error: "+query->getErrMsg()+ " Stmt: ["+query->getStatement()+"]", LL_ERROR);
		_has_error=true;
	}

	return false;
}

bool DatabaseCursor::has_error(void)
{
	return _has_error;
}