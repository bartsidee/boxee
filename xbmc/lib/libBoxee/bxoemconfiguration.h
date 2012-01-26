/*
 * bxoemconfiguration.h
 *
 *  Created on: Sep 1, 2010
 *      Author: shay
 */

#ifndef BXOEMCONFIGURATION_H_
#define BXOEMCONFIGURATION_H_

#include <string>
#include <map>
#include "system.h"
namespace BOXEE {

/**
*/
class BXOEMConfiguration{
public:
    virtual ~BXOEMConfiguration();

  static BXOEMConfiguration &GetInstance();

  bool  Load();

  bool  HasParam(const std::string &strKey);
  int   GetIntParam(const std::string &strKey, int nDefault=0);
  double  GetDoubleParam(const std::string &strKey, double dDefault=0.0);
  std::string GetStringParam(const std::string &strKey, const std::string strDefault="");
  std::string GetURLParam(const std::string &strKey, const std::string strDefault="");

  bool Getline(std::string& fLine,FILE *fFile);

protected:
    BXOEMConfiguration();

  std::map<std::string, std::string>  m_config;
};

}

#endif /* BXOEMCONFIGURATION_H_ */
