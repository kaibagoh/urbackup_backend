#include "../Interface/Database.h"
#include "../Interface/Server.h"
#include "../Interface/DatabaseCursor.h"
#include "database.h"
#include "server_settings.h"
#include "MDBFileCache.h"
#include "SQLiteFileCache.h"
#include "../stringtools.h"
#include "../urbackupcommon/os_functions.h"
#include "serverinterface/helper.h"

namespace
{

std::wstring get_files_cache_type(void)
{
	IDatabase *db=Server->getDatabase(Server->getThreadID(), URBACKUPDB_SERVER);
	db_results res=db->Read("SELECT tvalue FROM misc WHERE tkey='files_cache'");

	if(!res.empty())
	{
		return res[0][L"tvalue"];
	}
	return std::wstring();
}

void update_files_cache_type(std::string new_type)
{
	IDatabase *db=Server->getDatabase(Server->getThreadID(), URBACKUPDB_SERVER);
	IQuery *q=db->Prepare("UPDATE misc SET tvalue=? WHERE tkey='files_cache'");
	if(q!=NULL)
	{
		q->Bind(new_type);
		q->Write();
	}
}

struct SCallbackData
{
	IDatabaseCursor* cur;
	int64 pos;
	SStartupStatus* status;
};

db_results create_callback(size_t n_done, void *userdata)
{
	SCallbackData *data=(SCallbackData*)userdata;

	data->status->processed_file_entries=n_done;
	
	db_results ret;
	db_single_result res;
	
	if(data->cur->next(res))
	{
		ret.push_back(res);
	}
	
	return ret;
}

bool create_files_cache_common(FileCache& filecache, SStartupStatus& status)
{
	IDatabase *db=Server->getDatabase(Server->getThreadID(), URBACKUPDB_SERVER);

	db_results cache_res;
	if(db->getEngineName()=="sqlite")
	{
		cache_res=db->Read("PRAGMA cache_size");
		db->Write("PRAGMA cache_size = -"+nconvert(500*1024));
	}

	status.creating_filescache=true;
	Server->Log("Creating file entry cache. This might take a while...", LL_WARNING);

	IQuery *q_read=db->Prepare("SELECT shahash, filesize, fullpath, hashpath FROM files ORDER BY shahash ASC, filesize ASC, created DESC");

	SCallbackData data;
	data.cur=q_read->Cursor();
	data.pos=0;
	data.status=&status;

	filecache.create(create_callback, &data);

	if(!cache_res.empty())
	{
		db->Write("PRAGMA cache_size = "+wnarrow(cache_res[0][L"cache_size"]));
		db->Write("PRAGMA shrink_memory");
	}

	status.creating_filescache=false;

	if(data.cur->has_error())
	{
		return false;
	}

	return true;
}

bool setup_lmdb_files_cache(size_t map_size, SStartupStatus& status)
{
	MDBFileCache filecache(map_size);
	if(filecache.has_error())
	{
		Server->Log("Error creating file cache", LL_ERROR);
		return false;
	}

	return create_files_cache_common(filecache, status);
}

bool setup_sqlite_files_cache(SStartupStatus& status)
{
	os_create_dir(L"urbackup/cache");

	if(!Server->openDatabase("urbackup/cache/backup_server_files_cache.db", URBACKUPDB_FILES_CACHE))
	{
		Server->Log("Failed to open SQLite file entry cache database", LL_ERROR);
		return false;
	}
	else
	{
		IDatabase *db=Server->getDatabase(Server->getThreadID(), URBACKUPDB_FILES_CACHE);
		db->Write("PRAGMA journal_mode=WAL");
		db->Write("CREATE TABLE files_cache ( key BLOB, value BLOB)");

		SQLiteFileCache filecache;

		return create_files_cache_common(filecache, status);
	}
}

bool filecache_opened_ok=false;

}

void delete_file_caches(void)
{
	Server->deleteFile("urbackup/cache/backup_server_files_cache.lmdb");
	Server->deleteFile("urbackup/cache/backup_server_files_cache.lmdb-lock");
	Server->deleteFile("urbackup/cache/backup_server_files_cache.db");
	Server->deleteFile("urbackup/cache/backup_server_files_cache.db-shm");
	Server->deleteFile("urbackup/cache/backup_server_files_cache.db-wal");
}

void create_files_cache(SStartupStatus& status)
{
	IDatabase *db=Server->getDatabase(Server->getThreadID(), URBACKUPDB_SERVER);
	ServerSettings settings(db);

	if(settings.getSettings()->filescache_type=="lmdb")
	{
		filecache_opened_ok=true;
		if(get_files_cache_type()!=L"lmdb" && settings.getSettings()->filescache_type=="lmdb")
		{
			delete_file_caches();

			if(!setup_lmdb_files_cache(static_cast<size_t>(settings.getSettings()->filescache_size), status))
			{
				Server->Log("Setting up files cache failed", LL_ERROR);
				filecache_opened_ok=false;
			}
		}
		else if(!FileExists("urbackup/cache/backup_server_files_cache.lmdb"))
		{
			delete_file_caches();

			update_files_cache_type("none");
			if(!setup_lmdb_files_cache(static_cast<size_t>(settings.getSettings()->filescache_size), status))
			{
				Server->Log("Setting up files cache failed", LL_ERROR);
				filecache_opened_ok=false;
			}
		}
		update_files_cache_type(settings.getSettings()->filescache_type);
		MDBFileCache::initFileCache(static_cast<size_t>(settings.getSettings()->filescache_size));
	}
	else if(settings.getSettings()->filescache_type=="sqlite")
	{
		bool has_error=false;
		if(get_files_cache_type()!=L"sqlite" && settings.getSettings()->filescache_type=="sqlite")
		{
			delete_file_caches();

			if(!setup_sqlite_files_cache(status))
			{
				Server->Log("Setting up files cache failed", LL_ERROR);
				has_error=true;
			}
		}
		else if(!FileExists("urbackup/cache/backup_server_files_cache.db"))
		{
			delete_file_caches();

			update_files_cache_type("none");
			if(!setup_sqlite_files_cache(status))
			{
				Server->Log("Setting up files cache failed", LL_ERROR);
				has_error=true;
			}
		}
		else
		{
			if(!Server->openDatabase("urbackup/cache/backup_server_files_cache.db", URBACKUPDB_FILES_CACHE))
			{
				Server->Log("Failed to open SQLite file entry cache database", LL_ERROR);
				has_error=true;
			}
		}
		if(!has_error)
		{
			update_files_cache_type(settings.getSettings()->filescache_type);
		}
		filecache_opened_ok=!has_error;
		SQLiteFileCache::initFileCache();
	}

	if(settings.getSettings()->filescache_type=="none")
	{
		if(FileExists("urbackup/cache/backup_server_files_cache.lmdb"))
		{
			delete_file_caches();
		}
		if(FileExists("urbackup/cache/backup_server_files_cache.db"))
		{
			delete_file_caches();
		}

		update_files_cache_type(settings.getSettings()->filescache_type);
	}
}

FileCache* create_lmdb_files_cache(void)
{
	if(filecache_opened_ok)
	{
		return new MDBFileCache(0);
	}
	else
	{
		return NULL;
	}
}

FileCache* create_sqlite_files_cache(void)
{
	if(filecache_opened_ok)
	{
		return new SQLiteFileCache();
	}
	else
	{
		return NULL;
	}
}