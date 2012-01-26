#ifndef XMLUTILS_H_
#define XMLUTILS_H_

#include "tinyxml.h"

class XmlUtils
{
public:
	XmlUtils();
	virtual ~XmlUtils();
	
	/**
	 * The purpose of this function is to extract the value of the 
	 * widget element attribute that can be read from either 
	 * the XML node attribute or the child element value
	 */
	static const char* GetWidgetElementAttribute(TiXmlElement* pElement, const char* szAttributeName);
	
	static const char * getIndent( unsigned int numIndents );
	static const char * getIndentAlt( unsigned int numIndents );
	static void dump_to_stdout( TiXmlNode* pParent, unsigned int indent = 0 );
	static void dump_to_stdout(const char* pFilename);
	static int dump_attribs_to_stdout(TiXmlElement* pElement, unsigned int indent);
};

#endif /*XMLUTILS_H_*/
