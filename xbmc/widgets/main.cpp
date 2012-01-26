
#include <iostream>

#define XP_UNIX


#include "tinyxml.h"
#include "WidgetParser.h"
#include "WidgetEngine.h"
#include "MonkeyEngine.h"
#include "URL.h"
#include "MyVideoEngine.h"

#include "JSCustomer.h"
#include "JSImage.h"

#include <jsapi.h>
#include <jsautocfg.h>


using namespace std;

void xmlTest();
void spiderMonkeyTest();

int main() {
	
	// Test 
	spiderMonkeyTest();
	

//	cout << "Loading widget engine..." << endl;
//
//	WidgetEngine * pWidgetEngine = new WidgetEngine();
//	
//	pWidgetEngine->LoadWidget("Dilbert Daily.kon");
//	
//	pWidgetEngine->Init();
//	
//	pWidgetEngine->Run();
	
	
	
}	

void spiderMonkeyTest() {
	// loading the spider monkey

	cout << "Starting Spider Monkey test" << endl;


	MonkeyEngine* engine = new MonkeyEngine();

	engine->init();

	// Create an object
	
//	Customer * myCustomer = new Customer();
//	myCustomer->SetName("Alexis");
//	myCustomer->SetAge(30);
//	
//	JSCustomer myJSCustomer

	JSCustomer::JSInit(engine->getContext(), engine->getGlobalObject());
	//JSImage::JSInit(engine->getContext(), engine->getGlobalObject());

	JSObject * customerObject = JS_DefineObject(engine->getContext(), 
			engine->getGlobalObject(), 
			"customer", 
			&JSCustomer::Customer_class, 
			NULL, 
			0);
	
	JSCustomer *priv = new JSCustomer();
	Customer* customer = new Customer();
	priv->setCustomer(customer);
	JS_SetPrivate(engine->getContext(), customerObject, (void *) priv);
	
	//JSCustomer::JSConstructor(engine->getContext(), customerObject, 0, NULL, NULL);
	
//	JSObject * imageObject = JS_DefineObject(engine->getContext(), 
//				engine->getGlobalObject(), 
//				"result", 
//				&JSImage::_class, 
//				NULL, 
//				0);
	
//	string str = "Alexx";
//	JSString* jsString = JS_NewStringCopyZ(engine->getContext(), str.c_str());
//	jsval val = STRING_TO_JSVAL(jsString);
	
	jsval val = INT_TO_JSVAL(50);
	
	JS_SetProperty(engine->getContext(), customerObject, "age", &val);
	
	string str = "Alexx";
	JSString* jsString = JS_NewStringCopyZ(engine->getContext(), str.c_str());
	jsval val2 = STRING_TO_JSVAL(jsString);
	JS_SetProperty(engine->getContext(), customerObject, "name", &val2);
	
	//JS_SetProperty(engine->getContext(), imageObject, "src", &val);

	//std::string script = myWidget->getOnLoad();
	//std::string script = "print \"Hello World\"; ";
	//std::string script = "var url = new URL(); var urldata = url.fetch(\"http://www.tapestrycomics.com/dilbert.xml\"); result.name = urldata;";
	//std::string script = "var url = new URL();";
	//std::string script = "var c = new Customer(); c.name = \"Franky\"; c.age = 50; c.computeReduction(); 14;";
	std::string script = "customer.age = 32; customer.name";
	//std::string script = "result.src;";
	//std::string script = "var c = new Customer(); c.name = \"Franky\"; c.age = 32; var reduction = c.compute();";
	//std::string script = "var c = new Customer(); c.name = \"Hello\";  c.age = 50;";

	engine->runScript(script);
	jsval rval; 
	JSBool result  = JS_GetProperty(engine->getContext(), customerObject, "name", &rval);
	//JSBool result  = JS_GetProperty(engine->getContext(), imageObject, "src", &rval);

	if (result == JS_TRUE) {

		if (JSVAL_IS_OBJECT(rval)) {
			cerr << "Value is object" << endl;
			JSObject* obj = JSVAL_TO_OBJECT(rval);
		}

		JSString *str = JS_ValueToString(engine->getContext(), rval); 
		printf("Got value: %s\n", JS_GetStringBytes(str));
	}
	else {
		cerr << "Could not retreive property" << endl;
	}
	
	cout << "New Age: " << customer->GetAge() << endl;
}
