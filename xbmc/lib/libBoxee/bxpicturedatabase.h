// Copyright © 2008 BOXEE. All rights reserved.
#ifndef BXPICTUREDATABASE_H_
#define BXPICTUREDATABASE_H_

#include "bxdatabase.h"

namespace BOXEE
{

class BXMetadata;
class BXFolder;

class BXPictureDatabase : public BOXEE::BXDatabase
{
public:
	BXPictureDatabase();
	virtual ~BXPictureDatabase();
	
	int AddPictureFolder(const BXFolder* pPictureFolder);
	int AddPicture(const BXMetadata* pMetadata);
	int GetPictures(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iOrder, int iItemLimit);
	int GetPictureFolders(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iOrder, int iItemLimit);
	int GetPictureFiles(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iOrder, int iItemLimit);
	void RemoveDuplicates(std::vector<BXMetadata*> &vecA, std::vector<BXMetadata*> &vecB);
	
};

}

#endif /*BXPICTUREDATABASE_H_*/
