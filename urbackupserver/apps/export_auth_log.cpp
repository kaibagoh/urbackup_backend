#include "app.h"
#include <fstream>
#include "../serverinterface/login.h"
#include "../../stringtools.h"
#include "../../urbackupcommon/os_functions.h"

int export_auth_log()
{
	bool use_berkeleydb;
	open_server_database(use_berkeleydb, true);
	open_settings_database(use_berkeleydb);

	IDatabase *db=Server->getDatabase(Server->getThreadID(), URBACKUPDB_SERVER);
	if(db==NULL)
	{
		Server->Log("Could not open main database", LL_ERROR);
		return 1;
	}

	db_results res = db->Read("SELECT strftime('%Y-%m-%d %H:%M', logintime, 'localtime') AS iso_logintime, username, ip, method FROM settings_db.login_access_log ORDER BY logintime DESC");

	Server->deleteFile("urbackup/auth_log.csv");
	std::fstream out("urbackup/auth_log.csv", std::ios::out|std::ios::binary);
	if(!out.is_open())
	{
		Server->Log(L"Error opening \""+Server->getServerWorkingDir()+os_file_sep()+L"urbackup/auth_log.csv\" for writing", LL_ERROR);
		return 1;
	}

	for(size_t i=0;i<res.size();++i)
	{
		LoginMethod loginMethod = static_cast<LoginMethod>(watoi(res[i][L"method"]));
		out << Server->ConvertToUTF8(res[i][L"iso_logintime"]) << ";"
			<< Server->ConvertToUTF8(res[i][L"username"]) << ";"
			<< Server->ConvertToUTF8(res[i][L"ip"]) << ";";

		switch(loginMethod)
		{
		case LoginMethod_Webinterface:
			out << "webinterface"; break;
		case LoginMethod_RestoreCD:
			out << "restorecd"; break;
		}

		out << "\r\n";
	}

	out.close();
	Server->Log(L"Auth log has been written to \""+Server->getServerWorkingDir()+os_file_sep()+L"urbackup/auth_log.csv\"", LL_INFO);


	return 0;
}