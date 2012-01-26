#ifndef WIDGETPARSER_H_
#define WIDGETPARSER_H_

#include <string>
#include "Widget.h"

using namespace std;

/**
 * The purpose of this class is to present an API for parsing and 
 * running Widgets
 */

class WidgetParser
{
public:
	WidgetParser();
	virtual ~WidgetParser();
	
	/**
	 * The function receives the path to xml file describing the widget
	 * and returns a pointer to fully created widget 
	 */
	Widget* createWidget(const string& xmlFile); 
};

#endif /*WIDGETPARSER_H_*/
