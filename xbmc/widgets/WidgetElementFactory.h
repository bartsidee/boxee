#ifndef WIDGETELEMENTFACTORY_H_
#define WIDGETELEMENTFACTORY_H_

#include <string>

using namespace std;

class WidgetElement;

class WidgetElementFactory
{
public:
	WidgetElementFactory();
	virtual ~WidgetElementFactory();
	
	static WidgetElement* createWidgetElement(const string& type);
};

#endif /*WIDGETELEMENTFACTORY_H_*/
