#include "URL.h"

size_t write_data(void *ptr, size_t size, size_t nmemb, void *data)
{
	return ((URL*)data)->write_url_data(ptr,size,nmemb,data);
}

URL::URL(string url)
{
	m_url = url;
	
	CURI *curl;
	CURLcode res;

	// Initialize the cURL library
	curl = curl_easy_init();
	if(curl)
	{
		// set us up the URL
		curl_easy_setopt(curl, CURLOPT_URL,url.c_str() );
		/* send all data to this function  */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		/* we pass our 'chunk' struct to the callback function */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)this);
		res = curl_easy_perform(curl);
		/* always cleanup */
		curl_easy_cleanup(curl);
	}

}


URL::~URL()
{
}

size_t URL::write_url_data(void *ptr, size_t size, size_t nmemb, void *data) {
	 // What we will return
//	  int result = 0;
//
//	  // Is there anything in the buffer?
//	  if (buffer != NULL)
//	  {
//	    // Append the data to the buffer
//	    buffer->append(data, size * nmemb);
//
//	    // How much did we write?
//	    result = size * nmemb;
}

