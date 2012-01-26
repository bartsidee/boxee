#ifndef URL_H_
#define URL_H_

#include <string>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

using namespace std;



class URL
{
private:
	
	// Write all expected data in here
	static string buffer;
	
	string m_url;
	
public:
	URL(string url);
	virtual ~URL();
	
	size_t write_url_data(void *ptr, size_t size, size_t nmemb, void *data);
};

#endif /*URL_H_*/
