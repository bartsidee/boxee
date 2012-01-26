// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxappboxrepositories
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BXAPPBOXREPOSITORIES_H
#define BXAPPBOXREPOSITORIES_H

#include <vector>
#include "bxxmldocument.h"
#include "bxobject.h"

namespace BOXEE
{

class BXAppBoxRepositories : public BXXMLDocument
{
public:
  BXAppBoxRepositories();
  virtual ~BXAppBoxRepositories();
  virtual bool Parse() ;	
  
  int GetNumOfRepositories() const;
  BXObject GetRepository(int index) const;
  
  unsigned long GetTimeStamp() const;
  
  void Clear();
  bool IsLoaded();
  void SetLoaded(bool bLoaded=true);

protected:
  
  std::vector<BXObject>	m_repositories;
  time_t 	m_timestamp;
  bool	m_bLoaded;
};

}

#endif
