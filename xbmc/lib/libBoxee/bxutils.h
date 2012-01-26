// Copyright © 2008 BOXEE. All rights reserved.
#ifndef BXUTILS_H_
#define BXUTILS_H_

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "bxmetadata.h"

namespace BOXEE
{

class to_lower
{
public:
    char operator() (char c) const       
    {
        return tolower(c);
    }
};

class to_upper
{
public:
    char operator() (char c) const
    {
        return toupper(c);
    }
};

class BXUtils
{
public:
	
	/**
	 * Check that the file with the specified
	 * path can be open for reading. If not 
	 * return false.
	 */
	static bool FileExists(std::string strFilePath);
	
	static void StringReplace(std::string& strOrig, const std::string& token1, const std::string& token2);
	static void StringTrim(std::string& strString);
	static std::vector<std::string> StringTokenize(const std::string& str,const std::string& delimiters);
	static std::string IntToString(int iInt);
	static int StringToInt(std::string strString);
	static std::string& StringToUpper(std::string& strString);
	static std::string& StringToLower(std::string& strString);
	
	static std::string GetFolderName(std::string strPath);
	static std::string GetParentPath(const std::string& strPath);
	static std::string GetEffectiveFolderName(const std::string& strPath );
	static std::string GetEffectiveFolderPath(const std::string& strPath );
	static std::string GetEffectiveParentFolderPath(const std::string& strPath );
	static std::string GetSubShareFolderName(const std::vector<std::string>& vecPathFilter, const std::string& strPath);
	static void MergeByDateAdded(const std::vector<BXMetadata*> &vecA, const std::vector<BXMetadata*> &vecB, std::vector<BXMetadata*> &vecOut, int iItemLimit);
	static void MergeByDateModified(const std::vector<BXMetadata*> &vecA, const std::vector<BXMetadata*> &vecB, std::vector<BXMetadata*> &vecOut, int iItemLimit);
	static bool CheckPathFilter(const std::vector<std::string>& vecPathFilter, const std::string& strPath);
	static std::string RemoveSMBCredentials(const std::string& strPath);
	
	static bool HasSlashAtEnd(const std::string& strFile);
	static void RemoveSlashAtEnd(std::string& strFile);
	
	static void FreeMetaDataVec(std::vector<BXMetadata*> &vec);
	static void FreeFolderVec(std::vector<BXFolder*> &vec);
	static int GetRandInt(int iLimit);
	
	static std::string URLEncode(const std::string& pcsEncode);
	static std::string URLDecode(const std::string& strInput);
	static bool GetArtistData(const std::string& strArtistName, BXArtist* pArtist);
	static int RunRegExp(const std::string& strString, const std::string& strExpression, bool bCaseSensitive = true);
	
	static int64_t MAX_LONGLONG(int64_t x, int64_t y);
	static uint64_t ComputeHash(std::ifstream& f);
	
	static std::string BytesToHexString(unsigned char* bytes, size_t length);
	static size_t HexStringToBytes(std::string str, unsigned char* bytes, size_t maxLength);

  static std::string GetMD5Hex(const unsigned char* bytes, const size_t length);
  static std::string GetMD5Hex(const std::string str);
		
private:
  static std::string csUnsafeString;
  static std::string decToHex(char num, int radix);
  static std::string convert(char val);
  static std::string FixArtistName(const std::string& strArtistName);
  static bool ParseDiscogsXML(const std::string& strXML, std::string& strPrimaryThumb, std::string& strSecondaryThumb);

public:
  

	/**
	 * Prints vector of strings to log (DEBUG level)
	 */
	static void PrintTokens(std::vector<std::string> vecTokens);
};

}

#endif /*BXUTILS_H_*/
