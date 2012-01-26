// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeeapplications
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXBOXEEAPPLICATIONS_H
#define BOXEEBXBOXEEAPPLICATIONS_H

#include <vector>
#include <set>
#include "bxxmldocument.h"
#include "bxobject.h"

namespace BOXEE
{

class BXBoxeeApplications : public BXXMLDocument
{
public:
  BXBoxeeApplications();
  virtual ~BXBoxeeApplications();
  virtual bool Parse() ;	
  
  int GetNumOfApplications() const;
  BXObject GetApplication(int index) const;
  
  unsigned long GetTimeStamp() const;
  
  void Clear();
  bool IsLoaded();
  void SetLoaded(bool bLoaded=true);

  bool IsApplicationIdExist(std::string appId);

protected:
  
  std::vector<BXObject>	m_applications;
  std::set<std::string> m_applicationIdsMap;

  time_t 	m_timestamp;
  bool	m_bLoaded;
};

}

#endif
