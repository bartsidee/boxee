#ifndef SQLITE_POOL_MNGR_H
#define SQLITE_POOL_MNGR_H

#include "SqliteConnectionPoolObject.h"
#include "PoolMngr.h"

class CSqlitePoolMngr : public CPoolMngr< CSqliteConnectionPoolObject >
{
public:
  CSqlitePoolMngr( const CStdString& strDatabaseFile );
protected:
  virtual void InitializeNewObject( CSqliteConnectionPoolObject* pObj );
private:
  CStdString m_strDatabaseFile;
};

#endif