#ifndef MONKEYENGINE_H_
#define MONKEYENGINE_H_

#define XP_UNIX

#include "jsapi.h"
#include "jsautocfg.h"

#include <string>

using namespace std;


class Widget;


/**
 * This class provides the wrapper for the SpiderMonkey API
 */
class MonkeyEngine
{

private:

	JSVersion version;
	JSRuntime *rt;
	JSContext *cx;
	JSBool builtins;
	JSObject *global;

public:
	MonkeyEngine();
	virtual ~MonkeyEngine();

	/**
	 * Initialize the JS engine
	 */
	void init();

	/**
	 * Run the provided script
	 */
	int runScript(const string& script);
	
	int createWidgetObject(Widget* widget);
	
	// Auxiliary getters and setters
	JSContext* getContext() {
		return cx;
	}
	
	JSObject* getGlobalObject() {
		return global;
	}

	
};

#endif /*MONKEYENGINE_H_*/
