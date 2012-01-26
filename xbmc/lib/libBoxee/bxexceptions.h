// Copyright © 2008 BOXEE. All rights reserved.
/*
 *  BoxeeExceptions.h
 *  libBoxee
 *
 *  Created by Michael Sasson on 6/2/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __BX_NET_EXCEPTION__
#define __BX_NET_EXCEPTION__
namespace BOXEE {

// Network exception
class BXNetworkException
{ 
	public:
		BXNetworkException(void);
		virtual ~BXNetworkException(void) { }
};

// Credentials exception
class BXCredentialsException
{
	public:
		BXCredentialsException(void);
		virtual ~BXCredentialsException(void) { }
};
}
#endif
