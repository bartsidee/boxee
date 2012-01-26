#ifndef WIDGET_H_
#define WIDGET_H_

#include <string>
#include <vector>
#include "tinyxml.h"

#include "WidgetElement.h"

using namespace std;

class Action;

/**
 * This class represents a single widget
 */

class Widget
{
	
public:
	
	
	
private:
	
	vector<WidgetElement*> m_arrWidgetElements;
	
	// Actions
	Action * m_pOnLoadAction;
	
public:
	Widget();
	virtual ~Widget();
	
	void LoadFromXml(TiXmlElement* pWidgetElement);

	
private: // functions
	
	// A function that validates the child elements of the widget
	// TODO: Move this to more complex validation engine
	// 
	bool validateChildElement(const string& type);
};

#endif /*WIDGET_H_*/
