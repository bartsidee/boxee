#include "Exception.h"

#include <iostream>

Exception::Exception(const string& reason)
{
	m_strReason = reason;
	cerr << "Excception occured. Reason: " << m_strReason << endl; 
}

Exception::~Exception()
{
}

const string& Exception::getReason() {
	return m_strReason;
}
