#include "MonkeyEngine.h"


#include <iostream>

/* initialize the global class */
JSClass global_class = {
		"global",0,
		JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
		JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub,
		0, 0, 0, 0, 0, 0, 0, 0
};

JSBool JavaScript_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	uintN i, n;
	JSString *str;

	for (i = n = 0; i < argc; i++) {
		str = JS_ValueToString(cx, argv[i]);
		if (!str) return JS_FALSE;
		if (i)
			std::cout << " ";
		std::cout << JS_GetStringBytes(str);
	}
	n++;
	if (n)
		std::cout << std::endl;
	return JS_TRUE;
}
void JS_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report) {
	std::cerr << "JS Error in " << report->filename << " on line " << report->lineno << ": " << message << std::endl;
}

MonkeyEngine::MonkeyEngine()
{
}

MonkeyEngine::~MonkeyEngine()
{
	if (cx != NULL) {
		JS_DestroyContext(cx);

	}

	if (rt != NULL) {
		JS_DestroyRuntime(rt);
	}
}

void MonkeyEngine::init() {
	/* initialize the JS run time, and return result in rt */

	cout << "Initializing runtime" << endl;

	rt = JS_NewRuntime(8L * 1024L * 1024L);

	if (!rt) {
		cout << "Error creating new runtime" << endl;
		return;
	}

	cout << "Initializing runtime finished" << endl;


	/* Initialize new context */
	
	cout << "Initializing new context" << endl;

	cx = JS_NewContext(rt, 0x1000);

	cout << "Initializing new context finished" << endl;



	/* if cx does not have a value, end the program here */
	if (cx == NULL) {
		cerr << "Error creating new context" << endl;
		return;  
	}


	/* create the global object here */
	global = JS_NewObject(cx, &global_class, NULL, NULL);

	/* initialize the built-in JS objects and the global object */
	builtins = JS_InitStandardClasses(cx, global);
	JS_SetErrorReporter(cx, JS_ErrorReporter);

	cout << "Enigine initialization completed successfully" << endl;

}

int MonkeyEngine::createWidgetObject(Widget* widget) {

}

int MonkeyEngine::runScript(const string& script) {
	jsval rval; 
	uintN lineno = 0;

	cout << "Starting script evaluation" << endl;
	
	
	JSBool ok;
	try {
		ok = JS_EvaluateScript(cx, global, script.c_str(), script.length(), "script", lineno, &rval);
	}
	catch(...) {
		cerr << "Exception while evaluating script\n";
	}



	cout << "Script evaluation completed with status: " << (ok == JS_TRUE ? "OK" : "FAILED")<< endl;

	JSString *str = JS_ValueToString(cx, rval); 
	printf("Script result: %s\n", JS_GetStringBytes(str));

	return 0;
}
