/*
 * bxfeedfactory.h
 *
 *  Created on: Jul 16, 2009
 *      Author: yuvalt
 */

#ifndef BXFEEDFACTORY_H_
#define BXFEEDFACTORY_H_

#include <string>
#include "bxfeedreader.h"
#include "bxxmldocument.h"

namespace BOXEE
{

class BXFeedFactory
{
public:
  static IBXFeedReader* Create(BXXMLDocument& document);
};

};

#endif /* BXFEEDFACTORY_H_ */
