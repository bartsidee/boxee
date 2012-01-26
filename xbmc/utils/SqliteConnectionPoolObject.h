#ifndef SQLITE_CONNECTION_POOL_OBJECT_H
#define SQLITE_CONNECTION_POOL_OBJECT_H

#include "StdString.h"
#include "lib/sqLite/sqlitedataset.h"

class CSqliteConnectionPoolObject
{
public:
  CSqliteConnectionPoolObject( const CStdString& strDatabaseFile="" );
  virtual ~CSqliteConnectionPoolObject();

  bool Init();
  bool Release();
  void SetDatabaseName( const CStdString& strDatabaseFile ) 
  { 
    m_strDatabaseFile = strDatabaseFile; 
  }
  bool Validate();

  dbiplus::SqliteDatabase* GetDatabase() const { return m_pDB;  }
  dbiplus::Dataset* GetFirstDataset() const    { return m_pDS;  }
  dbiplus::Dataset* GetSecondDataset() const   { return m_pDS2; }
private:
  dbiplus::SqliteDatabase* m_pDB;
  dbiplus::Dataset*        m_pDS;
  dbiplus::Dataset*        m_pDS2;
  CStdString m_strDatabaseFile;
  time_t m_time;	
};

#endif
