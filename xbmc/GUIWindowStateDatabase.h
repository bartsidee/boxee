#ifndef GUIWINDOWSTATEDATABASE_H_
#define GUIWINDOWSTATEDATABASE_H_

#include "bxdatabase.h"

class CGUIWindowStateDatabase : public BOXEE::BXDatabase
{
public:
    CGUIWindowStateDatabase();
    virtual ~CGUIWindowStateDatabase();

    bool SaveState(int iWindow, const std::string& strSelectedCategory, int iView, const std::string& strSortMethod, int iSelectedItem);
    bool LoadState(int iWindow, const std::string& strSelectedCategory, int& iView, std::string& strSortMethod, int& iSelectedItem);

    bool SetDefaultCategory(int iWindow, const std::string& strSelectedCategory);
    bool GetDefaultCategory(int iWindow, std::string& strSelectedCategory);
    bool SetUserSetting(const std::string& strKey, const std::string& strValue);
    bool GetUserSetting(const std::string& strKey, std::string& strValue);

    bool GetUserEnabledServices(std::vector<std::string>& vecEnabledServices);
    bool GetUserService(const std::string& serviceId, std::string& serviceName, bool& sendShare);
    bool AddUserService(const std::string& serviceId, const std::string& serviceName, bool sendShare);
    bool SetUserServiceState(const std::string& serviceId, bool sendShare);
    bool RemoveUserService(const std::string& serviceId);

    bool Init();

    virtual bool TablesExist();
    virtual bool CreateTables();
    virtual bool UpdateTables();
    bool DropTables();

private:
    
};

#endif

