/*
 * bxboxeewebfavorites.h
 *
 *  Created on: Feb 10, 2011
 *      Author: shayyizhak
 */

#ifndef BXBOXEEWEBFAVORITES_H_
#define BXBOXEEWEBFAVORITES_H_

#include <vector>
#include "bxxmldocument.h"
#include "bxobject.h"

namespace BOXEE
{

class BXBoxeeWebFavorites : public BXXMLDocument
{
public:
  BXBoxeeWebFavorites();
  virtual ~BXBoxeeWebFavorites();

  int GetNumOfWebFavorites() const;
  BXObject GetWebFavorites(int index) const;

  unsigned long GetTimeStamp() const;

  virtual bool Parse();
  void Clear();
  bool IsLoaded();
  void SetLoaded(bool bLoaded=true);

protected:

  std::vector<BXObject> m_webFavorites;
  time_t  m_timestamp;
  bool  m_bLoaded;
};

}

#endif /* BXBOXEEWEBFAVORITES_H_ */
