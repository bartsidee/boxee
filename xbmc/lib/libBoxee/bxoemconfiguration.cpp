/*
 * bxoemconfiguration.cpp
 *
 *  Created on: Sep 1, 2010
 *      Author: shay
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "bxoemconfiguration.h"
#include "SpecialProtocol.h"
#include "log.h"

namespace BOXEE {

BXOEMConfiguration::BXOEMConfiguration()
{
}


BXOEMConfiguration::~BXOEMConfiguration()
{
}

BXOEMConfiguration &BXOEMConfiguration::GetInstance() {
  static BXOEMConfiguration instance;
  return instance;
}

bool BXOEMConfiguration::Getline(std::string& fLine,FILE *fFile)
{
  char line[1025];
  memset(line, 0, 1025);

  while(fgets(line, 1024, fFile) != NULL)
  {
    if(strlen(line) < 1024)
    {
            // Case of successful read of line
      fLine += line;
      return true;
    }
  }

  // Case we exit the fgets with NULL
  // we need to check if we read any data
  // and if so append it to fLine
  fLine += line;
  return false;
}

bool BXOEMConfiguration::Load()
{
#ifdef HAS_EMBEDDED
  CStdString fileName = "special://xbmc/system/oem.config";
  fileName = _P(fileName);
  CLog::Log(LOGINFO, "BXOEMConfiguration::Load - Loading OEM config file: %s", fileName.c_str());
  FILE *fConfig = fopen(fileName.c_str(), "r");
#else
  FILE *fConfig = NULL;
#endif
  if (!fConfig)
  {
    return false;
  }

  // read first line
  std::string strLine;
  bool readLine = Getline(strLine,fConfig);

  while (readLine == true)
  {
    if (!(strLine[0] == '#' || strLine[0] == '\r' || strLine[0] == '\n'))
    {
      int nSep = strLine.find("=");
      std::string strKey, strValue;
      if (nSep > 0)
      {
        strKey  = strLine.substr(0,nSep);
        strValue = strLine.substr(nSep+1);

        while (strKey.size() > 0 && (strKey[0] == ' ' || strKey[0] == '\t'))
          strKey.erase(0,1);
        while (strKey.size() > 0 && (strKey[strKey.size()-1] == ' ' || strKey[strKey.size()-1] == '\t'))
          strKey.erase(strKey.size()-1,1);

        while (strValue.size() > 0 && (strValue[0] == ' ' || strValue[0] == '\t'))
          strValue.erase(0,1);

        if (strValue[strValue.size()-1] == '\n')
          strValue.erase(strValue.size()-1,1);
        if (strValue[strValue.size()-1] == '\r')
          strValue.erase(strValue.size()-1,1);

        m_config[strKey] = strValue;
      }
    }

    // read next line
    strLine = "";
    readLine = Getline(strLine,fConfig);
  }

  fclose(fConfig);
  return true;
}

bool BXOEMConfiguration::HasParam(const std::string &strKey) {
  return m_config.find(strKey) != m_config.end();
}

int BXOEMConfiguration::GetIntParam(const std::string &strKey, int nDefault) {
  if (!HasParam(strKey))
    return nDefault;

  return atoi(m_config[strKey].c_str());
}

double  BXOEMConfiguration::GetDoubleParam(const std::string &strKey, double dDefault) {
  if (!HasParam(strKey))
    return dDefault;

  return atof(m_config[strKey].c_str());
}

std::string BXOEMConfiguration::GetStringParam(const std::string &strKey, const std::string strDefault) {
  if (!HasParam(strKey))
    return strDefault;

  return m_config[strKey];
}

std::string BXOEMConfiguration::GetURLParam(const std::string &strKey, const std::string strDefault) {
  if (!HasParam(strKey))
    return strDefault;

  std::string strResult = m_config[strKey];

  // partial URLs are prefixed by the Boxee.Server param
  if (strResult.find("://") <= 0)
    strResult = m_config["Boxee.Server"] + strResult;

  return strResult;
}

}
