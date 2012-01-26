#include <iostream>
#include "WidgetParser.h"
#include "Exception.h"
#include "XmlUtils.h"
#include "tinyxml.h"

using namespace std;

WidgetParser::WidgetParser()
{
}

WidgetParser::~WidgetParser()
{
}

Widget* WidgetParser::createWidget(const string& xmlFile) {

	cout << "Creating widget from " << xmlFile << endl;

	Widget* widget = new Widget();

	TiXmlDocument doc( xmlFile.c_str());
	bool loadOkay = doc.LoadFile();

	if ( !loadOkay )
	{
		cerr << "Could not load XML file, error: " << doc.ErrorDesc();
		return NULL;
	}

	TiXmlHandle hDoc(&doc);
	TiXmlElement* pWidgetElement;
	TiXmlHandle hRoot(0);


	pWidgetElement=hDoc.FirstChildElement().Element();
	// should always have a valid root but handle gracefully if it does not
	if (!pWidgetElement) { 
		return NULL;
	}

	try {
		// Create new widget and load it from the XML
		widget->LoadFromXml(pWidgetElement);
	}
	catch (Exception& exception) {
		cerr << "Error loading the widget from XML. Reason: " << exception.getReason() << endl;
	}



	//		const char* name=pWidgetElement->Value();
	//		
	//		
	//		
	//		printf("The root element name: %s\n", name);
	//
	//		// save this for later
	//		hRoot=TiXmlHandle(pWidgetElement);
	//		
	//		// Create window
	//		
	//		TiXmlElement* pWindowElement=hRoot.FirstChild( "window" ).Element();
	//		
	//		if (pWindowElement == NULL) {
	//			cerr << "No Window element detected, can't create element" << endl;
	//			return NULL;
	//		}
	//		
	//		const char* windowName = pWindowElement->Attribute("name");
	//		printf("The window element name: %s\n", windowName);
	//
	//		
	//		// Create window
	//		cout << "Creating window" << endl;
	//		
	//		// Extract widget elements
	//		
	//				
	//		// Extract the java script from the onTimerFired event
	//		TiXmlElement* pOnLoadEvent = (TiXmlElement*) pWidgetElement->FirstChild("action");
	//		const char * loadEventScript = pOnLoadEvent->GetText();
	//		
	//		printf("----------------------------------------------\n\n%s\n\n", loadEventScript);
	//		
	//		widget->setOnLoad(loadEventScript);




	cout << "Widget created" << endl;

	return widget;

}

