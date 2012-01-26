// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxappboxpopularities
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BXAPPBOXPOPULARITIES_H
#define BXAPPBOXPOPULARITIES_H

#include <map>
#include "bxxmldocument.h"
#include "bxobject.h"

namespace BOXEE
{

class BXAppBoxPopularities : public BXXMLDocument
{
public:
  BXAppBoxPopularities();
  virtual ~BXAppBoxPopularities();
  virtual bool Parse() ;	
  
  int GetNumOfPopularities() const;
  std::string GetPopularityById(const std::string& id);
  
  unsigned long GetTimeStamp() const;
  
  void Clear();
  bool IsLoaded();
  void SetLoaded(bool bLoaded=true);

protected:
  
  std::map<std::string,BXObject>	m_popularities;
  time_t 	m_timestamp;
  bool	m_bLoaded;
};

}

#endif
