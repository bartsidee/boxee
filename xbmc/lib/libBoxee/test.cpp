
#include <stdio.h>
#include <stdlib.h>


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <cstdlib>

#include "logger.h"
#include "boxee.h"
#include "bxcurl.h"
#include "bxrssreader.h"
#include "bxcredentials.h"
#include "bxconfiguration.h"
#include "bxfriendslist.h"
#include "bxboxeefeed.h"

using namespace std;
using namespace BOXEE;

void TestLoggerFunction(const char *szMsg) {
	printf("Test: %s",szMsg);
}

int main(int argc, char *argv[])
{
	Boxee::GetInstance().Start();
	
	Logger::GetLogger().SetLogLevel(LOG_LEVEL_DEBUG);	
	Logger::GetLogger().SetLoggerFunction(TestLoggerFunction);

	Boxee &boxee = Boxee::GetInstance();
	BXCredentials creds;	
	creds.SetUserName("vulkan");
	creds.SetPassword("nickcave");

	boxee.SetCredentials(creds);
	boxee.SetVerbose(true);

	BXFriendsList list;
	boxee.RetrieveFriendsList(list);

	
	BXGeneralMessage msg;
	msg.SetMessageType("recommend");
	msg.SetValue("movie_id","1232423");
	msg.SetValue("user_id","vulkan");
	msg.SetValue("name","the movie");

	boxee.SetUserAction(msg);


	BXBoxeeFeed feed;
	if (boxee.GetBoxeeFeed(feed)){
		printf("time: %lu\n", feed.GetTimeStamp());
 	}

	Boxee::GetInstance().Stop();
 	
	return EXIT_SUCCESS;
}
