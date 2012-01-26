#include "boxee_md5.h"
#include "bxutils.h"
#include "logger.h"
#include <sstream>
#include <math.h>
#include "bxcurl.h"
#include "bxxmldocument.h"
#include "tinyxpath/xpath_static.h"
#include "lib/libpcre/pcre.h"
#include "bxexceptions.h"
#include "bxconfiguration.h"
#include "StringUtils.h"
#include "Util.h"
#include <json/writer.h>
#include "FileSystem/DllLibCurl.h"
#include "log.h"

#include <algorithm>
#ifdef WIN32
#include <io.h>
#endif

#include "boxee.h"

using namespace std;

#if defined(__APPLE__) || defined (_LINUX)
#define PATH_SEPARATOR_CHAR '/'
#define PATH_SEPARATOR_STR "/"
#else
#define PATH_SEPARATOR_CHAR '\\'
#define PATH_SEPARATOR_STR "\\"
#endif

namespace BOXEE
{

// HEX Values array
char hexVals[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

bool BXUtils::FileExists(std::string strFilePath) {
#ifdef WIN32
	return _access(strFilePath.c_str(), 0) == 0;
#else
	return access(strFilePath.c_str(), F_OK) == 0;
#endif
}

/**
 * Searches the original string for token1 and replaces it with token2 
 * whereever possible
 */
void BXUtils::StringReplace(string& strOrig, const string& token1, const string& token2) {

	int pos = strOrig.find(token1);

	while (pos != -1) {
		strOrig.replace(pos, token1.length(), token2);

		pos = strOrig.find(token1, pos);
	}
}

void BXUtils::StringTrim(std::string& strString)
{
	if (strString == "") return;
	string::size_type pos = strString.find_last_not_of(' ');
	if(pos != string::npos) {
		strString.erase(pos + 1);
		pos = strString.find_first_not_of(' ');
		if(pos != string::npos) strString.erase(0, pos);
	}
	else strString.erase(strString.begin(), strString.end());
}

vector<string> BXUtils::StringTokenize(const string& str,const string& delimiters)
{
	vector<string> tokens;

	string::size_type lastPos = 0, pos = 0;  
	int count = 0;

	if(str.length()<1)  return tokens;

	// skip delimiters at beginning.  
	lastPos = str.find_first_not_of(delimiters, 0);

	if((str.substr(0, lastPos-pos).length()) > 0)
	{   
		count = str.substr(0, lastPos-pos).length();    

		for(int i=0; i < count; i++)    
			tokens.push_back("");

		if(string::npos == lastPos)
			tokens.push_back("");
	}

	// find first "non-delimiter".
	pos = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos)
	{               
		// found a token, add it to the vector.
		tokens.push_back( str.substr(lastPos, pos - lastPos));

		// skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);             

		if((string::npos != pos) && (str.substr(pos, lastPos-pos).length() > 1))      
		{
			count = str.substr(pos, lastPos-pos).length();

			for(int i=0; i < count; i++)
				tokens.push_back("");
		}

		pos = str.find_first_of(delimiters, lastPos);
	}

	return tokens;
}

std::string BXUtils::IntToString(int iInt) {
	// TODO: Add error handling
	std::ostringstream stream;
	stream << iInt;
	return stream.str();
}

std::string BXUtils::LongToString(long iLong) {
  // TODO: Add error handling
  std::ostringstream stream;
  stream << iLong;
  return stream.str();
}

int BXUtils::StringToInt(std::string strString)
{
	// TODO: Add error handling
	int value(0);
	std::istringstream iss(strString);
	iss >> value;
	return value;
}

unsigned long BXUtils::StringToUnsignedLong(std::string strString)
{
  unsigned long value (0);

  std::istringstream iss(strString);
  iss >> value;
  return value;
}

std::string BXUtils::VectorTokened(const std::vector<std::string>& vecTokens , const std::string& separator)
{
	std::string strTokenString;

	for (size_t i=0; i < vecTokens.size(); i++)
	{
		strTokenString += vecTokens[i];

		if (i+1 < vecTokens.size())
		  strTokenString += separator;
	}

	return strTokenString;
}


std::string BXUtils::GetFolderName(std::string strPath)
{
	if (strPath.size() > 0 && strPath[strPath.size()-1] == PATH_SEPARATOR_CHAR)
		strPath += " "; // just a hack to get the tokenizer right in case its a folder

	std::vector<std::string> vecTokens = StringTokenize(strPath, PATH_SEPARATOR_STR);
	if (vecTokens.size() > 1) 
	{
		std::string strFolder = vecTokens[vecTokens.size() - 2];
		return strFolder;
	}

	return "";
}

std::string BXUtils::GetParentPath(const std::string& _strPath)
{
  std::string strPath = _strPath;
  BXUtils::RemoveSlashAtEnd(strPath);

  size_t iPos = strPath.find_last_of('/');
#ifndef _LINUX
  if (iPos == string::npos)
  {
    iPos = strPath.find_last_of('\\');
  }
#endif
  if (iPos != string::npos)
  {
    std::string parentPath = strPath.substr(0, iPos + 1);
    return parentPath;
  }

  return strPath;
}

std::string BXUtils::GetEffectiveFolderName(const std::string& strPath )
{
	std::string strFolderPath = GetEffectiveFolderPath(strPath);
	std::string strFolderName = GetFolderName(strFolderPath);

	return strFolderName;
}

std::string BXUtils::GetEffectiveParentFolderPath(const std::string& strPath ) {
	std::string strFolderPath = strPath;

	if (strFolderPath.size() > 0)
		strFolderPath = BXUtils::GetParentPath(strFolderPath);
	
	return GetEffectiveFolderPath(strFolderPath);

}

std::string BXUtils::GetEffectiveFolderPath(const std::string& strPath )
{
	bool bResolved = false;
	std::string strFolderPath = strPath;
	if (strFolderPath.size() > 0 && strFolderPath[strFolderPath.size() -1] != PATH_SEPARATOR_CHAR)
		strFolderPath = BXUtils::GetParentPath(strFolderPath);

	// The purpose of this expression is to filter out all unwanted folders
	std::string strExpression = "^ *cd.?[1-9] *$|^ *dis[kc].?[1-9] *$|^ *video_ts *$|^ *audio_ts *$|^[!]?sample$|^season|^.*\\.(rar|zip|RAR|ZIP)$";

	std::string strFolderName;
	while (!bResolved && Boxee::GetInstance().IsRunning())
	{

		strFolderName = BXUtils::GetFolderName(strFolderPath);
		std::string strlowerFolderName = strFolderName;
		transform (strlowerFolderName.begin(), strlowerFolderName.end(), strlowerFolderName.begin(), to_lower());
		
		if (BXUtils::RunRegExp(strlowerFolderName, strExpression, false) >= 0) 
		{
			std::string strKeepPath = strFolderPath;
			strFolderPath = BXUtils::GetParentPath(strFolderPath);

			if (strFolderPath == strKeepPath)
				break; // sanity

			continue;
		}

		bResolved = true;  
	}

	return strFolderPath;
}

void BXUtils::MergeByDateAdded(const std::vector<BXMetadata*> &vecA, const std::vector<BXMetadata*> &vecB, std::vector<BXMetadata*> &vecOut, int iItemLimit)
{
	int countA = 0;
	int countB = 0;
	vecOut.clear();

	// We assume that the input vectors are sorted, since they come from sequential copying of ordered database query results

	while (countA < (int)vecA.size() && countB < (int)vecB.size() && iItemLimit > 0)
	{
		if (vecA[countA]->m_iDateAdded > vecB[countB]->m_iDateAdded) {
			vecOut.push_back(new BXMetadata(*vecA[countA]));
			countA++;
		}
		else {
			vecOut.push_back(new BXMetadata(*vecB[countB]));
			countB++;
		}
		iItemLimit--;
	}

	// Add the rest of the items
	for (int i = countA; i < (int)vecA.size() && iItemLimit > 0; i++) {
		vecOut.push_back(new BXMetadata(*vecA[i]));
		iItemLimit--;
	}
	for (int i = countB; i < (int)vecB.size() && iItemLimit > 0; i++) {
		vecOut.push_back(new BXMetadata(*vecB[i]));
		iItemLimit--;
	}

}

void BXUtils::MergeByDateModified(const std::vector<BXMetadata*> &vecA, const std::vector<BXMetadata*> &vecB, std::vector<BXMetadata*> &vecOut, int iItemLimit)
{
	int countA = 0;
	int countB = 0;
	//vecOut.clear();

	// We assume that the input vectors are sorted, since they come from sequential copying of ordered database query results

	while (countA < (int)vecA.size() && countB < (int)vecB.size() && iItemLimit > 0)
	{
		if (vecA[countA]->m_iDateModified > vecB[countB]->m_iDateModified) {
			vecOut.push_back(new BXMetadata(*vecA[countA]));
			countA++;
		}
		else {
			vecOut.push_back(new BXMetadata(*vecB[countB]));
			countB++;
		}
		iItemLimit--;
	}

	// Add the rest of the items
	for (int i = countA; i < (int)vecA.size() && iItemLimit > 0; i++) {
		vecOut.push_back(new BXMetadata(*vecA[i]));
		iItemLimit--;
	}
	for (int i = countB; i < (int)vecB.size() && iItemLimit > 0; i++) {
		vecOut.push_back(new BXMetadata(*vecB[i]));
		iItemLimit--;
	}

}

void BXUtils::FreeFolderVec(std::vector<BXFolder*> &vec)
{
	size_t s = vec.size();
	for (size_t i=0; i < s; i++)
	{
		BXFolder *pData = vec[i];
		if (pData)
			delete pData;
	}
	vec.clear();
}

void BXUtils::FreeMetaDataVec(std::vector<BXMetadata*> &vec)
{
	size_t s = vec.size();
	for (size_t i=0; i < s; i++)
	{
		BXMetadata *pData = vec[i];
		if (pData)
			delete pData;
	}
	vec.clear();
}

// PURPOSE OF THIS FUNCTION IS TO CONVERT A GIVEN CHAR TO URL HEX FORM
std::string BXUtils::convert(char val) 
{
	std::string csRet;
	csRet += "%";
	csRet += decToHex(val, 16); 
	return  csRet;
}

// THIS IS A HELPER FUNCTION.
// PURPOSE OF THIS FUNCTION IS TO GENERATE A HEX REPRESENTATION OF GIVEN CHARACTER
std::string BXUtils::decToHex(char num, int radix)
{ 
	int temp=0; 
	std::string csTmp;
	int num_char;
	num_char = (int) num;

	// ISO-8859-1 
	// IF THE IF LOOP IS COMMENTED, THE CODE WILL FAIL TO GENERATE A 
	// PROPER URL ENCODE FOR THE CHARACTERS WHOSE RANGE IN 127-255(DECIMAL)
	if (num_char < 0)
		num_char = 256 + num_char;

	while (num_char >= radix)
	{
		temp = num_char % radix;
		num_char = (int)(num_char / radix);
		csTmp = hexVals[temp];
	}

	csTmp += hexVals[num_char];

	if(csTmp.length() < 2)
	{
		csTmp += '0';
	}

	std::string strdecToHex(csTmp);

	// Reverse the String  
	std::reverse(strdecToHex.begin(), strdecToHex.end());

	return strdecToHex;
}

std::string BXUtils::URLDecode(const std::string& strInput)
{
  std::string strRes=strInput;
  CURL_HANDLE* helperHandle = g_curlInterface.easy_init ();
  if (helperHandle)
	{
    char *unescaped = g_curlInterface.easy_unescape(helperHandle, strInput.c_str(), 0, NULL);
    if (unescaped)
		{ 
      strRes = unescaped;
      g_curlInterface.free_curl(unescaped);
		} 
    g_curlInterface.easy_cleanup(helperHandle);
	}
  return strRes;
}

std::string BXUtils::URLEncode(const std::string& pcsEncode)
{ 
  std::string strRes=pcsEncode;
  CURL_HANDLE* helperHandle = g_curlInterface.easy_init ();
  if (helperHandle)
	{
    char *escaped = g_curlInterface.easy_escape(helperHandle, pcsEncode.c_str(), pcsEncode.size());
    if (escaped)
		{
      strRes = escaped;
      g_curlInterface.free_curl(escaped);
		}   
    g_curlInterface.easy_cleanup(helperHandle);
		}
  return strRes;
		}

std::string BXUtils::RemoveSMBCredentials(const std::string& strPath)
{
  std::string strCleanPath = strPath;

  // Check if string starts with smb://
  if (strPath.find("smb://",0) == 0)
  {
    // Find first occurence of @, 
    size_t atPos = strPath.find_first_of('@');
    if (atPos != std::string::npos)
    {
      // Remove credentials, everything between the smb:// and the @
      strCleanPath = strCleanPath.replace(6, atPos - 5, "");
    }
  }

  return strCleanPath;
}

bool BXUtils::CheckPathFilter(const std::vector<std::string>& vecPathFilter, const std::string& strPath)
{
  CStdString str1 = strPath;

  if (strPath.find("rar://") != std::string::npos || strPath.find("zip://") != std::string::npos)
    str1 = URLDecode(strPath).substr(6);
  
  if(str1.find("upnp://") != std::string::npos)
    str1 = URLDecode(strPath);

  str1 = RemoveSMBCredentials(str1);
  transform (str1.begin(), str1.end(), str1.begin(), to_lower());

#ifdef _WIN32
  if (strPath.find("rar://") != std::string::npos || strPath.find("zip://") != std::string::npos)
  {
  str1.Replace('/','\\');
  }
#endif 
  // Check if the path is within filter
	for (int i = 0; i < (int)vecPathFilter.size(); i++) 
	{
	  // we need to consider different cases in which one path ends with PATH_SEPARATOR_CHAR and one path doesnt.
    // further more - on some cases empty password on smb will be handled as smb://user:@host and sometimes as smb://user@host
      
    std::string str2 = vecPathFilter[i];

    if(str2.find("upnp://") != std::string::npos)
    {
      std::string tmpStr = str2;
      str2 = URLDecode(tmpStr);
      RemoveSlashAtEnd(str2);
    }

    transform (str2.begin(), str2.end(), str2.begin(), to_lower());
    str2 = RemoveSMBCredentials(str2);
    std::string::size_type startPos = str1.find(str2, 0);
    
		if (startPos == 0)
		{
		  return true;
		}
	} 

	return false;
}

// Receives a list of shares and a path and returns the path of the folder in the path that is under the share
std::string BXUtils::GetSubShareFolderName(const std::vector<std::string>& vecPathFilter, const std::string& strPath) 
{
	for (int i = 0; i < (int)vecPathFilter.size(); i++) {
		std::string::size_type startPos = strPath.find(vecPathFilter[i], 0);
		if (startPos == 0) {
			std::string strSharePath = vecPathFilter[i];
			if (strSharePath == strPath) {
				// The given path is share, return it
				return strPath;
			}
			else {
				int iLength = strSharePath.length();

				int nextDelimiter = strPath.find(PATH_SEPARATOR_STR, iLength);
				return strPath.substr(0,nextDelimiter+1);
			}
		}
	} 

	return "";
}

int BXUtils::GetRandInt(int iLimit)
{
	return rand() % iLimit;
}

int64_t BXUtils::MAX_LONGLONG(int64_t x, int64_t y)
{  
	if((x) > (y)) 
		return x;
	else	
		return y;
}

uint64_t BXUtils::ComputeHash(std::ifstream& f)
{
	uint64_t hash, fsize;

	f.seekg(0, ios::end);
	fsize = f.tellg();
	f.seekg(0, ios::beg);

	hash = fsize;
	for(uint64_t tmp = 0, i = 0; i < 65536/sizeof(tmp) && f.read((char*)&tmp, sizeof(tmp)); i++, hash += tmp) {}
	f.seekg(MAX_LONGLONG(0, (uint64_t)fsize - 65536), ios::beg);
	for(uint64_t tmp = 0, i = 0; i < 65536/sizeof(tmp) && f.read((char*)&tmp, sizeof(tmp)); i++, hash += tmp) {}
	return hash;
} 

bool BXUtils::HasSlashAtEnd(const std::string& strFile) 
{
  if (strFile.size() == 0) return false;
  char kar = strFile.c_str()[strFile.size() - 1];

  if (kar == '/' || kar == '\\')
    return true;

  return false;
}

void BXUtils::RemoveSlashAtEnd(std::string& strFile)
{
	if (strFile.size() == 0) 
	{
		return;
	}

	// correct check for base url like smb://
	if (strFile.size() > 3 && strFile.substr(strFile.length()-3) == "://") 
		return;

	if (BXUtils::HasSlashAtEnd(strFile))
		strFile.erase(strFile.length() - 1);
}

bool BXUtils::GetArtistData(const std::string& strArtistName_, BXArtist* pArtist) {
	
  if (strArtistName_ == "")
    return false;

  if (Boxee::GetInstance().GetMetadataEngine().FindArtistInCache(strArtistName_, *pArtist))
    return true;

  // Modify artist name, put 'the' at the end to make it findable in the Discogs database
  std::string strArtistName = strArtistName_;
  strArtistName = FixArtistName(strArtistName);
	
  BXCurl curl;
  int index;
  while( (index=strArtistName.find_first_of(' ')) >= 0 ) {
    strArtistName.replace( index, 1, "+" );
  }

	std::string strRequest = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.ArtistThumb","http://app.boxee.tv");
	strRequest += "/api/artist_thumb?name=";
	strRequest += strArtistName;

	std::string strXML = curl.HttpGetString(strRequest.c_str());

	// Parse the returned XML
	BXXMLDocument doc;
	doc.LoadFromString(strXML);

	TiXmlElement* pRootElement = doc.GetRoot();
	
	if (!pRootElement)
	{
    // Add negative result to cache
    Boxee::GetInstance().GetMetadataEngine().AddArtistToCache(strArtistName_, *pArtist);

		return false;
	}

	if (strcmpi(pRootElement->Value(), "artist") != 0)
	{
	  return false;
	}

	std::string strValue;

	const TiXmlNode* pTag = 0;
	while ((pTag = pRootElement->IterateChildren(pTag)))
	{
	  if (pTag->ValueStr() == "thumb")
	  {
	    const TiXmlNode *pValue = pTag->FirstChild();
	    strValue = pValue->ValueStr();
	  }
	}

	pArtist->m_strPortrait = strValue;

  Boxee::GetInstance().GetMetadataEngine().AddArtistToCache(strArtistName_, *pArtist);

	return true;

}

bool BXUtils::ParseDiscogsXML(const std::string& strXML, std::string& strPrimaryThumb, std::string& strSecondaryThumb)
{
  BXCurl curl;
  BXXMLDocument doc;
  doc.LoadFromString(strXML);

  TiXmlElement* rootElement = doc.GetRoot();

  if (!rootElement)
  {
    return false;
  }

  int iImages = TinyXPath::i_xpath_int(rootElement, "count(/resp/artist/images/image)");

  for (int i = 0; i < iImages; i++)
  {
    char szQuery[1024];
    memset(szQuery,0,1024);
    snprintf(szQuery,1023,"/resp/artist/images/image[%d]/@type", i+1);
    std::string strPrimary = TinyXPath::S_xpath_string (rootElement, szQuery);

    if (strPrimary == "primary")
    {
      memset(szQuery,0,1024);
      snprintf(szQuery,1023,"/resp/artist/images/image[%d]/@uri", i+1);
      std::string strThumbUrl = TinyXPath::S_xpath_string (rootElement, szQuery);
      std::string extension = strThumbUrl.substr(strThumbUrl.find_last_of('.'), string::npos);
      if ((extension == ".jpg") || (extension == ".jpeg"))
      {
        // Check whether the thumb is available
        curl.HttpHEAD(strThumbUrl.c_str());
        int code = curl.GetLastRetCode();
        if (code >= 200 && code < 400)
        {
          strPrimaryThumb = strThumbUrl;
          break;
        }
      }
    }

    if (strPrimary == "secondary" && strSecondaryThumb == "")
    {
      memset(szQuery,0,1024);
      snprintf(szQuery,1023,"/resp/artist/images/image[%d]/@uri", i+1);
      std::string strThumbUrl = TinyXPath::S_xpath_string (rootElement, szQuery);
      std::string extension = strThumbUrl.substr(strThumbUrl.find_last_of('.'), string::npos);
      if ((extension == ".jpg") || (extension == ".jpeg"))
      {
        curl.HttpHEAD(strThumbUrl.c_str());
        int code = curl.GetLastRetCode();
        if (code >= 200 && code < 400)
        {
          strSecondaryThumb = strThumbUrl;
          break;
        }
      }
    }
  }

  return true;
}

// Function for fixing artist name (move "the" to the end of the name), in order to successfully bring artist thumb from Discogs.
// Algorithm: In case the artist name start with "The ", the function will pass the "The " to the end of the artist name with an addition comma.
// Exception: The name of the band "The The" will remain unchanged.
// For example: "The Velvet Underground" will be fix to "velvet underground, the".
std::string BXUtils::FixArtistName(const std::string& strArtistName)
{
  std::string artistName = strArtistName;

  StringTrim(artistName);

  transform(artistName.begin(), artistName.end(), artistName.begin(), to_lower());

  if(artistName != "the the")
  {
    if(artistName.find("the ",0) == 0)
    {
      // Remove the "the " from the start
      artistName.erase(0,4);
      
      artistName.append(", the");
    }
  }    

  return artistName;
}

// WARNING: This function is rather specific and should not be used everywhere, it is not generic enough
// Take a look at the function usage examples first
int BXUtils::RunRegExp(const std::string& strString, const std::string& strExpression, bool bCaseSensitive) 
{
	pcre* re;
	
	// Set options
	int iOptions;
	if (bCaseSensitive) {
		iOptions = PCRE_DOTALL;
	}
	else {
		iOptions = PCRE_DOTALL && PCRE_CASELESS;
	}
	
	const char *errMsg = NULL;
	int errOffset      = 0;
	const int OVECCOUNT=(20+1)*3;
	int iOvector[OVECCOUNT];

	// Compile regular expression 
	re = pcre_compile(strExpression.c_str(), iOptions, &errMsg, &errOffset, NULL);
	if (!re)
	{
		LOG(LOG_LEVEL_ERROR, "PCRE: %s. Compilation failed at offset %d in expression '%s'",	errMsg, errOffset, strExpression.c_str());
		return false;
	}
	
	// Execute the expression on the provided string
	int rc = pcre_exec(re, NULL, strString.c_str(), strlen(strString.c_str()), 0, 0, iOvector, OVECCOUNT);

	pcre_free(re);
	
	if (rc<1)
	{
		switch(rc)
		{
		case PCRE_ERROR_NOMATCH:
			return -1;

		case PCRE_ERROR_MATCHLIMIT:
			LOG(LOG_LEVEL_ERROR, "PCRE: Match limit reached");
			return -1;

		default:
			LOG(LOG_LEVEL_ERROR, "PCRE: Unknown error: %d", rc);
			return -1;
		}
	}
	
	return iOvector[0];
}

#define BASE16SYM			("0123456789ABCDEF")
#define BASE16VAL			("\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9|||||||\xA\xB\xC\xD\xE\xF")

#define BASE16_ENCODELO(b)		(BASE16SYM[((unsigned char)(b)) >> 4])
#define BASE16_ENCODEHI(b)		(BASE16SYM[((unsigned char)(b)) & 0xF])

#define BASE16_DECODELO(b)		(BASE16VAL[toupper(b) - '0'] << 4)
#define BASE16_DECODEHI(b)		(BASE16VAL[toupper(b) - '0'])

std::string BXUtils::BytesToHexString(unsigned char* bytes, size_t length)
{
  std::string result;
  char ch[2];
  
  for (size_t i = 0; i < length; i++)
  {
    ch[0] = BASE16_ENCODELO(*bytes);
    ch[1] = BASE16_ENCODEHI(*bytes);
    result += ch[0];
    result += ch[1];
    ++bytes;
  }

  return result;
}

size_t BXUtils::HexStringToBytes(std::string str, unsigned char* bytes, size_t maxLength)
{
  const char* source = str.c_str();
  size_t result = 0;
  
  while (*source != '\0' && maxLength <= result)
  {
   bytes[0]  = BASE16_DECODELO(source[0]);
   bytes[0] |= BASE16_DECODEHI(source[1]);

   source += 2;
   ++bytes;
   ++result;
  }
  
  return result;
}

std::string BXUtils::GetMD5Hex(const unsigned char* bytes, const size_t length)
{
  unsigned char digest[16];
  
  md5_state_t md5Ctx;
  BOXEE::md5_init(&md5Ctx);
  BOXEE::md5_append(&md5Ctx, (const md5_byte_t*) bytes, length);
  BOXEE::md5_finish(&md5Ctx, digest);
  
  return BytesToHexString(digest, 16);
}

std::string BXUtils::GetMD5Hex(const std::string str)
{
  return BXUtils::GetMD5Hex((const unsigned char*) str.c_str(), str.length());
}	

std::string& BXUtils::StringToUpper(std::string& s)
{
  transform(s.begin(), s.end(), s.begin(), to_upper());
  return s;
}

std::string& BXUtils::StringToLower(std::string& s)
{
  transform(s.begin(), s.end(), s.begin(), to_lower());
  return s;
}

bool BXUtils::PerformJSONGetRequest(const std::string& url, Json::Value& response, int& returnCode, bool useCache)
{
  if (url.empty())
  {
    return false;
  }

  std::string strUrl = url;

  BXCurl curl;

  curl.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());
  curl.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());

  ListHttpHeaders headers;
  headers.push_back("Connection: keep-alive");
  curl.HttpSetHeaders(headers);

  std::string strJson = curl.HttpGetString(strUrl.c_str(), useCache);

  Json::Reader reader;

  returnCode = curl.GetLastRetCode();
  if (returnCode != 200)
  {
    CLog::Log(LOGERROR, "Got error return code from server for url: <%s>. Code: %d", url.c_str(), returnCode);
    return false;
  }

  if (strJson.length() == 0)
  {
    CLog::Log(LOGERROR, "Failed to parse empty server response for url: <%s>", url.c_str());
    return false;
  }

  if (!reader.parse(strJson, response))
  {
    CLog::Log(LOGERROR, "Failed to parse server response for url: <%s>. Error: %s", url.c_str(), reader.getFormatedErrorMessages().c_str());
    return false;
  }

  return true;
}

class JsonRequestBG : public IRunnable
{
public:
  JsonRequestBG(const CStdString& url, Json::Value& response, int& returnCode, bool silent, bool useCache) : m_url(url), m_response(response), m_returnCode(returnCode)
  {
    m_bJobResult = false;
    m_IsSilent = silent;
    m_useCache = useCache;
  }

  virtual ~JsonRequestBG() { }

  virtual void Run();
  const CStdString m_url;
  Json::Value m_response;
  int& m_returnCode;
  bool m_useCache;
};

void JsonRequestBG::Run()
{
  m_bJobResult = BXUtils::PerformJSONGetRequest(m_url, m_response, m_returnCode, m_useCache);
}

Job_Result BXUtils::PerformJSONGetRequestInBG(const std::string& url, Json::Value& response, int& returnCode, bool silent, bool useCache)
{
  JsonRequestBG* job = new JsonRequestBG(url, response, returnCode, silent, useCache);
  Job_Result result = CUtil::RunInBG(job, false);
  if (result == JOB_SUCCEEDED)
  {
    response = job->m_response;
  }
  delete job;

  return result;
}

bool BXUtils::PerformJSONPostRequest(const std::string& url, Json::Value& body, Json::Value& response, int& returnCode)
{
  if (url.empty())
  {
    return false;
  }

  std::string strUrl = url;

  BXCurl curl;

  curl.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());
  curl.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());

  ListHttpHeaders headers;
  headers.push_back("Connection: keep-alive");
  headers.push_back("Content-Type: application/json");
  curl.HttpSetHeaders(headers);

  Json::FastWriter writer;
  std::string strPostData = writer.write(body);

  std::string strJson = curl.HttpPostString(strUrl.c_str(), strPostData);

  returnCode = curl.GetLastRetCode();

#if 0
  Json::StyledWriter writer2;
  std::string strPostData2 = writer2.write(body);
  printf("Posting to %s\n\n", strUrl.c_str());
  printf("%s", strPostData2.c_str());
  printf("Got: RC=%d Text=<%s>\n", returnCode, strJson.c_str());
#endif

  if (returnCode != 200)
  {
    CLog::Log(LOGERROR, "Got error return code from server for url: <%s>. Code: %d", url.c_str(), returnCode);
    return false;
  }

  if (strJson.length() == 0)
  {
    CLog::Log(LOGERROR, "Failed to parse empty server response for url: <%s>", url.c_str());
    return false;
  }

  Json::Reader reader;
  if (!reader.parse(strJson, response))
  {
    CLog::Log(LOGERROR, "Failed to parse server response for url: <%s>. Error: %s", url.c_str(), reader.getFormatedErrorMessages().c_str());
    return false;
  }

  return true;
}

bool BXUtils::PerformPostRequest(const std::string& url, std::string& body, std::string& response, int& returnCode)
{
  if (url.empty())
  {
    return false;
  }

  BXCurl curl;

  curl.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());
  curl.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());

  ListHttpHeaders headers;
  headers.push_back("Connection: keep-alive");
  curl.HttpSetHeaders(headers);

  response = curl.HttpPostString(url.c_str(), body);

  returnCode = curl.GetLastRetCode();

#if 0
  printf("Posting to %s\n\n", strUrl.c_str());
  printf("%s", body.c_str());
  printf("Got: RC=%d Text=<%s>\n", returnCode, strJson.c_str());
#endif

  if (returnCode != 200)
  {
    CLog::Log(LOGERROR, "Got error return code from server for url: <%s>. Code: %d", url.c_str(), returnCode);
    return false;
  }

  return true;
}

class HttpPostRequestBG : public IRunnable
{
public:
  HttpPostRequestBG(std::string url, std::string body, bool silent = true)
  {
    m_bJobResult = false;
    m_IsSilent = silent;
    m_url = url;
    m_body = body;
  }

  virtual ~HttpPostRequestBG() { }

  virtual void Run();
  std::string m_url;
  std::string m_response;
  int m_returnCode;
  std::string m_body;
};

void HttpPostRequestBG::Run()
{
  m_bJobResult = BXUtils::PerformPostRequest(m_url, m_body, m_response, m_returnCode);
}

Job_Result BXUtils::PerformPostRequestInBG(const std::string& url, std::string& body, std::string& response, int& returnCode, bool silent)
{
  HttpPostRequestBG* job = new HttpPostRequestBG(url, body, silent);

  Job_Result result = CUtil::RunInBG(job, false);
  if (result == JOB_SUCCEEDED)
  {
    response = job->m_response;
  }
  delete job;

  return result;
}

}

