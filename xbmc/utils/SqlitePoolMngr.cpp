
#include "StdString.h"
#include "SqlitePoolMngr.h"
#include "log.h"


CSqlitePoolMngr::CSqlitePoolMngr( const CStdString& strDatabaseFile ) 
  : m_strDatabaseFile( strDatabaseFile )
{
}


void CSqlitePoolMngr::InitializeNewObject( CSqliteConnectionPoolObject* pObj )
{
  pObj->SetDatabaseName( m_strDatabaseFile );
}
