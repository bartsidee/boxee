// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeeapps
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXBOXEESERVICES_H
#define BOXEEBXBOXEESERVICES_H

#include <vector>
#include "bxxmldocument.h"
#include "bxobject.h"

namespace BOXEE {

/**
*/
class BXBoxeeServices : public BXXMLDocument
{
public:
  BXBoxeeServices();
  virtual ~BXBoxeeServices();
  virtual bool Parse() ;	
  
  int GetNumOfServices() const;
  BXObject GetService(int index) const;
  
  unsigned long GetTimeStamp() const;
  
  void Clear();
  bool IsLoaded();
  void SetLoaded(bool bLoaded=true);

protected:
  
  std::vector<BXObject>	m_services;
  time_t 	m_timestamp;
  bool	m_bLoaded;
};

}

#endif
