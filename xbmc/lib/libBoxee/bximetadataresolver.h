// Copyright © 2008 BOXEE. All rights reserved.
#ifndef BXIMETADATARESOLVER_H_
#define BXIMETADATARESOLVER_H_

#include <string>
#include <vector>

namespace BOXEE {

class BXFolder;
class BXMetadataScannerJob;

#define RESOLVER_SUCCESS 1
#define RESOLVER_FAILED  2
#define RESOLVER_ABORTED 3
#define RESOLVER_CANT_ACCESS 4

#define SHARE_TIMESTAMP_NOT_SCANNED 0
#define SHARE_TIMESTAMP_RESOLVING 1

/**
 * Interface class that should be implemented in order to
 * provide metadata for media files to boxee scanners
 */
class BXIMetadataResolver
{
public:
	BXIMetadataResolver();
	virtual ~BXIMetadataResolver();
	
	virtual int Resolve(BOXEE::BXMetadataScannerJob * pJob, std::vector<BOXEE::BXFolder*>& vecResults ) = 0;
	
	// This function verifies whether the provided path is available and located on one of existing shares
	virtual bool CheckPath(const std::string& strPath) = 0;
	
	// Stops the resolution process
	virtual bool Stop()=0;
	
	// Returns the id string of the resolver 
	virtual std::string GetId() = 0;
	
	// NOTE: All functions in the resolver interface receive some input parameters
	// regarding the media file that they are to resolve and some output parameters
	// that will contain the resolved details. The functions return true of the resolution
	// was successful and false otherwise
	
};

}

#endif /*BXIMETADATARESOLVER_H_*/
