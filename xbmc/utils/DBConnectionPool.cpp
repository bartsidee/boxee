

#include "StdString.h"
#include <map>
#include "log.h"

#include "DBConnectionPool.h"
#include "SqlitePoolMngr.h"

#include "SingleLock.h"

CDBConnectionPool::CDBConnectionPool()
{
}


CDBConnectionPool::~CDBConnectionPool()
{
  SqlitePoolMngrMap::iterator iter = begin();
  SqlitePoolMngrMap::iterator endIter = end();

  while( iter != endIter )
  {
    erase( iter++ );
  }
  clear();
}


CSqliteConnectionPoolObject* CDBConnectionPool::GetSqliteConnectionPoolObject( const CStdString& strDatabaseFile )
{
  CSingleLock lock(m_lock);
  
  SqlitePoolMngrMap::iterator iter = find( strDatabaseFile );
  CSqlitePoolMngr* pPool = NULL;

  if( iter != end() )
  {
    pPool = iter->second;
  }
  else
  {
    pPool = new CSqlitePoolMngr( strDatabaseFile );
    operator[]( strDatabaseFile ) = pPool;
  }

  lock.Leave();
  
  return pPool->GetFromPool();
}


void CDBConnectionPool::ReturnToPool( CSqliteConnectionPoolObject* pSqliteConnectionObject, const CStdString& strDatabaseFile )
{
  CSingleLock lock(m_lock);

  SqlitePoolMngrMap::iterator iter = find( strDatabaseFile );

  if( iter == end() )
  {
    return;
  }

  CSqlitePoolMngr* pPool = iter->second;

  if( !pPool )
  {
    return;
  }

  lock.Leave();
  
  pPool->ReturnToPool( pSqliteConnectionObject );
}
