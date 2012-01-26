/*
 * bxfeedfactory.cpp
 *
 *  Created on: Jul 16, 2009
 *      Author: yuvalt
 */

#include "tinyXML/tinyxml.h"
#include "bxfeedfactory.h"
#include "bxrssreader.h"
#include "bxatomreader.h"

namespace BOXEE {

IBXFeedReader* BXFeedFactory::Create(BXXMLDocument& document)
{
  if (document.GetDocument().FirstChild("rss"))
  {
    return new BXRSSReader(document);
  }
  else if (document.GetDocument().FirstChild("feed"))
  {
    return new BXAtomReader(document);
  }

  return NULL;
}

};
