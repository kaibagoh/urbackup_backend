#ifndef INTERFACE_DATABASE_H
#define INTERFACE_DATABASE_H

#include <vector>
#include <string>
#include <map>
#include "Query.h"
#include "Object.h"

class IDatabase : public IObject
{
public:
	virtual db_nresults ReadN(std::string pQuery)=0; 
	virtual db_results Read(std::string pQuery)=0; 
	virtual bool Write(std::string pQuery)=0;

	virtual void BeginTransaction(void)=0;
	virtual bool EndTransaction(void)=0;

	virtual IQuery* Prepare(std::string pQuery, bool autodestroy=true)=0;
	virtual void destroyQuery(IQuery *q)=0;
	virtual void destroyAllQueries(void)=0;

	virtual _i64 getLastInsertID(void)=0;

	virtual bool Import(const std::string &pFile)=0;
	virtual bool Dump(const std::string &pFile)=0;
	
	virtual std::string getEngineName(void)=0;

	virtual void DetachDBs(void)=0;
	virtual void AttachDBs(void)=0;

	virtual bool Backup(const std::string &pFile)=0;

	virtual void freeMemory()=0;

	virtual int getLastChanges()=0;
};

class DBScopedFreeMemory
{
public:
	DBScopedFreeMemory(IDatabase* db)
		: db(db) {}
	~DBScopedFreeMemory() {
		db->freeMemory();
	}
private:
	IDatabase* db;
};

#endif
