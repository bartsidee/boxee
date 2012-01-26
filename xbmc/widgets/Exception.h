#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <string>

using namespace std;

class Exception
{
	
private: 
	string m_strReason;
	
public:
	Exception(const string& reason);
	virtual ~Exception();
	
	const string& getReason();
};

#endif /*EXCEPTION_H_*/
