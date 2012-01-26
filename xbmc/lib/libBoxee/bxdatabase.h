// Copyright © 2008 BOXEE. All rights reserved.
#ifndef BXDATABASE_H_
#define BXDATABASE_H_

#include <string>
#include "../sqLite/sqlitedataset.h"

// Set of responses for data 
#define MEDIA_DATABASE_OK   1 // required operation was successful
#define MEDIA_DATABASE_ERROR -1 // some error has occured during the operation
#define MEDIA_DATABASE_NOT_FOUND -2 // item could not be found in the database
#define MEDIA_DATABSE_LOCK_RETRIES  5 // number of times to retry a transaction when database is locked

#define MEDIA_FILE_EXISTS   2
#define MEDIA_FILE_MODIFIED 3
#define MEDIA_FILE_NEW      4

class CSqliteConnectionPoolObject;
namespace BOXEE {

class BXDatabase {

protected:
  //static const std::string m_strDatabaseFile;

  dbiplus::SqliteDatabase* m_pDatabase;

  bool m_bOpen;

public:
  BXDatabase();
  virtual ~BXDatabase();
  
  virtual bool CreateTables();
  virtual bool UpdateTables() { return true; }
  
  int GetLastErrorCode();
  const char* GetLastErrorMessage();
	
//  void SetDatabaseFile(std::string strDatabaseFile) {
//    m_strDatabaseFile = strDatabaseFile;
//  }
//
//  std::string GetDatabaseFile() {
//    return m_strDatabaseFile;
//  }

  virtual bool Open();
  bool IsOpen();
  bool FileExists();
  virtual bool TablesExist();
  void Close();
  dbiplus::Dataset* Query(const char* query, ...);
  dbiplus::Dataset* UnQuotedQuery(const char* query, ...);
  dbiplus::Dataset* QuotedQuery(bool shouldQuote, const char* query, va_list argList);

  dbiplus::Dataset* Exec(const char* query, ...);
  int Insert(const char* query, ...);
  bool Create(const char *query, ...);

  void StartTransaction();
  bool CommitTransaction();
  void RollbackTransaction();
  bool InTransaction();
  std::string PrepareSQLStatement(std::string strStmt, ...);
	
	virtual std::string getDatabaseFilePath();
	std::string mDatabaseFilePath;
	
  CSqliteConnectionPoolObject *m_pDBPoolObject;
	static int refCount;
};

}

#endif /*BXDATABASE_H_*/
