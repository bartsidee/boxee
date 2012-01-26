#include "WidgetElementFactory.h"
#include "WidgetElement.h"
#include "Image.h"
#include "Window.h"
#include "Text.h"
#include "Action.h"

WidgetElementFactory::WidgetElementFactory()
{
}

WidgetElementFactory::~WidgetElementFactory()
{
}

WidgetElement* WidgetElementFactory::createWidgetElement(const string& type) {
	
	cout << "Creating widget of type: " << type << endl;
	
	if (type == "image") {
		return new Image();
	}
	else if (type == "text") {
		return new Text();
	}
	else if (type == "window") {
		return new Window();
	}
	else if (type == "action") {
		return new Action();
	}
	else 
		return NULL;
}
