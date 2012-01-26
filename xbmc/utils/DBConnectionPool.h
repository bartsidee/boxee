#ifndef DB_CONNECTION_POOL_H
#define DB_CONNECTION_POOL_H

#include "CriticalSection.h"

class CSqlitePoolMngr;
class CSqliteConnectionPoolObject;

class CDBConnectionPool : public std::map< CStdString, CSqlitePoolMngr* >
{
typedef std::map< CStdString, CSqlitePoolMngr* > SqlitePoolMngrMap;
public:
  CDBConnectionPool();
  virtual ~CDBConnectionPool();

  CSqliteConnectionPoolObject* GetSqliteConnectionPoolObject( const CStdString& strDatabaseFile );
  void ReturnToPool( CSqliteConnectionPoolObject* pSqliteConnectionObject, const CStdString& strDatabaseFile );
  
protected:
  CCriticalSection m_lock;
};

#endif