#include "../Interface/Database.h"
#include "../Interface/Thread.h"
#include "../Interface/Types.h"
#include "../Interface/Mutex.h"
#include "../Interface/Condition.h"
#include <vector>
#include "dao/ServerCleanupDao.h"
#include "dao/ServerBackupDao.h"
#include <sstream>

class ServerSettings;

enum ECleanupAction
{
	ECleanupAction_None,
	ECleanupAction_FreeMinspace,
	ECleanupAction_DeleteFilebackup,
	ECleanupAction_RemoveUnknown
};

struct CleanupAction
{
	//None
	CleanupAction(void)
		: action(ECleanupAction_None)
	{
	}

	//Delete file backup
	CleanupAction(std::wstring backupfolder, int clientid, int backupid, bool force_remove)
		: action(ECleanupAction_DeleteFilebackup), backupfolder(backupfolder), clientid(clientid), backupid(backupid),
		  force_remove(force_remove)
	{

	}

	//Free minspace
	CleanupAction(int64 minspace, bool *result, bool switch_to_wal)
		: action(ECleanupAction_FreeMinspace), minspace(minspace), result(result), switch_to_wal(switch_to_wal)
	{
	}

	//Remove unknown data
	CleanupAction(ECleanupAction action)
		: action(action)
	{
	}

	ECleanupAction action;
	
	std::wstring backupfolder;
	int clientid;
	int backupid;
	bool force_remove;
	bool switch_to_wal;

	int64 minspace;
	bool *result;
};


class ServerCleanupThread : public IThread
{
public:
	ServerCleanupThread(CleanupAction action);
	~ServerCleanupThread(void);

	void operator()(void);

	static bool cleanupSpace(int64 minspace, bool switch_to_wal=false);

	static void removeUnknown(void);

	static void updateStats(bool interruptible);
	static void initMutex(void);
	static void destroyMutex(void);

	static void doQuit(void);

private:

	void do_cleanup(void);
	bool do_cleanup(int64 minspace, bool switch_to_wal=false);

	void do_remove_unknown(void);

	bool correct_target(const std::wstring& backupfolder, std::wstring& target);

	bool correct_poolname(const std::wstring& backupfolder, const std::wstring& clientname, const std::wstring& pool_name, std::wstring& pool_path);

	void check_symlinks(const ServerCleanupDao::SClientInfo& client_info, const std::wstring& backupfolder);

	bool cleanup_images_client(int clientid, int64 minspace, std::vector<int> &imageids);

	void cleanup_images(int64 minspace=-1);

	void cleanup_files(int64 minspace=-1);

	bool cleanup_one_filebackup_client(int clientid, int64 minspace, int& filebid);

	void cleanup_other();

	void rewrite_history(const std::wstring& back_start, const std::wstring& back_stop, const std::wstring& date_grouping);

	void cleanup_client_hist();

	size_t getImagesFullNum(int clientid, int &backupid_top, const std::vector<int> &notit);
	size_t getImagesIncrNum(int clientid, int &backupid_top, const std::vector<int> &notit);

	size_t getFilesFullNum(int clientid, int &backupid_top);
	size_t getFilesIncrNum(int clientid, int &backupid_top);

	bool removeImage(int backupid, bool update_stat=true, int64 size_correction=0, bool force_remove=false);
	bool findUncompleteImageRef(int backupid);

	void removeClient(int clientid);

	bool deleteFileBackup(const std::wstring &backupfolder, int clientid, int backupid, bool force_remove=false);

	void deletePendingClients(void);

	void backup_database(void);

	bool deleteAndTruncateFile(std::wstring path);
	bool deleteImage(std::wstring path);
	int64 getImageSize(int backupid);

	int hasEnoughFreeSpace(int64 minspace, ServerSettings *settings);

	bool truncate_files_recurisve(std::wstring path);

	void enforce_quotas(void);

	bool enforce_quota(int clientid, std::ostringstream& log);

	void delete_incomplete_file_backups(void);

	IDatabase *db;

	static IMutex *mutex;
	static ICondition *cond;

	static IMutex *a_mutex;

	static bool update_stats;
	static bool update_stats_interruptible;
	
	std::vector<int> removeerr;

	static volatile bool do_quit;

	CleanupAction cleanup_action;

	ServerCleanupDao* cleanupdao;
	ServerBackupDao* backupdao;
};
