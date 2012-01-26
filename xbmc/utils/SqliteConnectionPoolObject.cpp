
#include "StdString.h"
#include "SqliteConnectionPoolObject.h"
#include "Util.h"
#include "Settings.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "lib/sqLite/sqlitedataset.h"
#include "Database.h"

using namespace dbiplus;


CSqliteConnectionPoolObject::CSqliteConnectionPoolObject( const CStdString& strDatabaseName )
  : m_strDatabaseFile( strDatabaseName )
{
  m_pDB = NULL;
  m_pDS  = NULL;
  m_pDS2 = NULL;  
}

CSqliteConnectionPoolObject::~CSqliteConnectionPoolObject()
{
}


bool CSqliteConnectionPoolObject::Init()
{
  m_pDB = new SqliteDatabase();
  m_pDB->setDatabase( _P( m_strDatabaseFile ).c_str() );
  m_pDB->connect();
  m_pDS  = NULL;
  m_pDS2 = NULL;  
  return true;
}


bool CSqliteConnectionPoolObject::Release()
{
  if (m_pDS)
    delete m_pDS;
  m_pDS = NULL;
  
  if (m_pDS2)
    delete m_pDS2;
  m_pDS2 = NULL;

  if (m_pDB)
    delete m_pDB;
  m_pDB = NULL;
  
  return true;
}

bool CSqliteConnectionPoolObject::Validate()
{
  if (m_pDS)
    delete m_pDS;
  
  if (m_pDS2)
    delete m_pDS2;
  
  m_pDS = m_pDB->CreateDataset();
  m_pDS2 = m_pDB->CreateDataset();
  return true;
}
