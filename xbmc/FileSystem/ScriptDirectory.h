#ifndef SCRIPTDIRECTORY_H_
#define SCRIPTDIRECTORY_H_

#include "IDirectory.h"

namespace DIRECTORY
{

class CScriptDirectory : public IDirectory
{
public:
	CScriptDirectory();
	virtual ~CScriptDirectory();
	
	virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
	virtual bool Exists(const char* strPath);
};

} // namespace

#endif /*SCRIPTDIRECTORY_H_*/
