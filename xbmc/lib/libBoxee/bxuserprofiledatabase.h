
#ifndef BXUSERPROFILEDATABASE_H_
#define BXUSERPROFILEDATABASE_H_

#include "bxdatabase.h"
#include "boxee.h"
#include "SpecialProtocol.h"

namespace BOXEE
{

  class BXUserProfileDatabase : public BOXEE::BXDatabase
  {
    public:
      BXUserProfileDatabase(void);
      virtual ~BXUserProfileDatabase(void);
      bool Init();

      void MarkAsWatched(const std::string& strPath, const std::string& strBoxeeId, double iLastPosition);
      void MarkAsUnWatched(const std::string& strPath, const std::string& strBoxeeId);
      bool IsWatchedByPath(const std::string& strPath);
      bool IsWatchedById(const std::string& strBoxeeId);
      bool CopyDataFromMediaDatabase();

      bool GetTimeWatchedByPath(const std::string& strPath, double& out_Time );
      bool IsWatchedById(const std::string& strBoxeeId , double& out_Time );

      virtual bool TablesExist();
      virtual bool CreateTables();
  private:
    static bool m_bTableCheck; //indicates if the app checked if the table exist on startup
  };

}
#endif

