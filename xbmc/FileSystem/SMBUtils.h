#ifndef SMBUTILS_H
#define SMBUTILS_H

#include "../../guilib/StdString.h"
#include "../FileItem.h"

namespace DIRECTORY
{
class SMBUtils {
public:
	static bool ProcessPath(const CStdString& strPath, CFileItemList &items, bool allow_prompt=false);
	static CStdString TranslatePath(const CStdString& strPath);
private:
	static CStdString GetComputerSMBName(const CStdString& computer);
	static bool GetComputers(CFileItemList &items);
	static bool GetComputerDevices(const CStdString& strSmbPath,const CStdString& selectedComp, CFileItemList &items, bool allow_prompt=false);
};
}
#endif	// #ifndef SMBUTILS_H
