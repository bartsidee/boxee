
const char *isJSValType(string type) {
	if (type == "int" || type == "float" || type=="double")
		return "JSVAL_IS_NUMBER(";
	if (type == "bool")
		return "JSVAL_IS_BOOLEAN(";
	if (type == "string" || type == "JSString*")
		return "JSVAL_IS_STRING(";
	if (type == "jsval")
		return "JSVAL_IS_JSVAL(";
	return "JSVAL_IS_OBJECT(";
}

string convertJSValTo(string destType) {
	if (destType=="jsval")
		return string("(");
	if (destType=="JSObject*")
		return string("JSVAL_TO_OBJECT(");
	if (destType=="JSString*")
		return string("JSVAL_TO_STRING(");
	string s;
	if (destType=="int" || destType=="float" || destType=="double" || destType=="bool" || destType=="string") {
		s = "__JSVal_TO_";
		s += destType;
		s += "(";
	} else {
		s += "__JSVal_TO_object";
		if (destType!="JSObject*" && destType[destType.length()-1]=='*')
			s += "p";
		s += "(cx,";
		s += destType;
		s += ",";
	}
	return s;
}



string convertToJSVal(string sourceType, string &className) {
	if (sourceType=="jsval")
		return string("(");
	if (sourceType=="JSObject*")
		return string("OBJECT_TO_JSVAL(");
	if (sourceType=="JSString*")
		return string("STRING_TO_JSVAL(");
	string s;
	if (sourceType=="int" || sourceType=="float" || sourceType=="double" || sourceType=="bool" || sourceType=="string") {
	  s += "__";
		s += sourceType;
		s += "_TO_JSVal(cx,";
	} else {
	  string sourceType2 = sourceType;
	  if (sourceType[sourceType.length()-1]=='*')
	  {
	    sourceType2.erase(sourceType.length()-1);
	  }
    s += sourceType2;
    s += "__";
		s += "object";
		if (sourceType!="JSObject*" && sourceType[sourceType.length()-1]=='*')
			s += "p";
		s += "_TO_JSVal(cx,";
	}
	return s;
}

string getUpperCaseFirst(string s) {
	s[0] = toupper(s[0]);
	return s;
}

void writeHeaderMacros(ostream &out, string &className) {
	out << "///// JavaScript Conversion Macros" << endl
		<< "#ifndef __JSVal_MACROS" << endl
		<< "#define __JSVal_MACROS" << endl
		<< endl
		<< "#define JSVAL_IS_JSVAL(v)			(true)" << endl
		<< endl
		<< "#define __JSVal_TO_int(v)			(JSVAL_IS_DOUBLE(v)?(int)(*(JSVAL_TO_DOUBLE(v))):JSVAL_TO_INT(v))" << endl
		<< "#define __JSVal_TO_bool(v)			JSVAL_TO_BOOLEAN(v)" << endl
		<< "#define __JSVal_TO_double(v)		(JSVAL_IS_INT(v)?JSVAL_TO_INT(v):(double)(*(JSVAL_TO_DOUBLE(v))))" << endl
		<< "#define __JSVal_TO_float(v)			(JSVAL_IS_INT(v)?JSVAL_TO_INT(v):(float)(*(JSVAL_TO_DOUBLE(v))))" << endl
		<< "#define __JSVal_TO_string(v)		string(JS_GetStringBytes(JSVAL_TO_STRING(v)))" << endl
		<< "#define __JSVal_TO_object(cx,t,v)	*((t*)(JS_GetPrivate(cx,JSVAL_TO_OBJECT(v))))" << endl
		<< "#define __JSVal_TO_objectp(cx,t,v)	((t)(JS_GetPrivate(cx,JSVAL_TO_OBJECT(v))))" << endl
		<< endl
		<< "#define __int_TO_JSVal(cx,v)		INT_TO_JSVAL(v)" << endl
		<< "#define __bool_TO_JSVal(cx,v)		BOOLEAN_TO_JSVAL(v)" << endl
		<< "#define __double_TO_JSVal(cx,v)		DOUBLE_TO_JSVAL(JS_NewDouble(cx, v))" << endl
		<< "#define __float_TO_JSVal(cx,v)		DOUBLE_TO_JSVAL(JS_NewDouble(cx, v))" << endl
		<< "#define __string_TO_JSVal(cx,v)		STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (v).c_str()))" << endl
    << "#endif" << endl
		<< "#define " << className << "__object_TO_JSVal(cx,v)		OBJECT_TO_JSVAL((v)." << className << "getJSObject(cx))" << endl
		<< "#define " << className << "__objectp_TO_JSVal(cx,v)	OBJECT_TO_JSVAL((v)->" << className << "getJSObject(cx))" << endl
		<< endl;
}

void writeHeaderFunctionSpec(ostream &out, FunctionMap &functions, string &className) {
	if (functions.nonStaticFuncs)
		out << "static JSFunctionSpec " << className << "_JSFunctionSpec[];" << endl
			<< endl;
	if (functions.staticFuncs)
		out << "static JSFunctionSpec " << className << "_JSFunctionSpec_static[];" << endl
			<< endl;
}


void writeCodeFunctionSpec(ostream &out, FunctionMap &functions, string &className) {
	if (!functions.size) return;
	out << "///// JavaScript Function Table" << endl;
	if (functions.nonStaticFuncs) {
		out << "JSFunctionSpec " << className << "::" << className << "_JSFunctionSpec[] = {" << endl;
		for (FunctionMap::iterator i=functions.begin(); i!=functions.end(); ++i) {
			out << "	{ \"" << i->first << "\", " << className << "JSFUNC_" << i->first << ", " << i->second.minArguments << ", 0, 0 }," << endl;
		}
		out << "	{ 0, 0, 0, 0, 0 }" << endl
			<< "};" << endl 
			<< endl;
	} 
	if (functions.staticFuncs) {
		out << "JSFunctionSpec " << className << "::" << className << "_JSFunctionSpec_static[] = {" << endl;
		for (FunctionMap::iterator i=functions.begin(); i!=functions.end(); ++i) {
			if (i->second.staticFuncs)
				out << "	{ \"" << i->first << "\", " << className << "JSFUNC_" << i->first << ", " << i->second.minArguments << ", 0, 0 }," << endl;
		}
		out << "	{ 0, 0, 0, 0, 0 }" << endl
			<< "};" << endl 
			<< endl;
	}
}

void writeHeaderPropertySpec(ostream &out, VariableList &variables, string &className) {
	if (variables.nonStaticVars)
		out << "static JSPropertySpec " << className << "_JSPropertySpec[];" << endl
			<< endl;
	if (variables.staticVars)
		out << "static JSPropertySpec " << className << "_JSPropertySpec_static[];" << endl
			<< endl;
}

void writeCodePropertySpec(ostream &out, VariableList &variables, string &className) {
	if (!variables.size) return;
	out << "///// JavaScript Variable Table" << endl;
	if (variables.nonStaticVars) {
		out << "JSPropertySpec " << className << "::" << className << "_JSPropertySpec[] = {" << endl;
		for (int i=0; i<variables.size; i++) {
			Variable var = variables[i];
			out << "	{ \"" << var.name << "\", JSVAR_" << var.name << ", JSPROP_ENUMERATE";
			if (var._readonly)
				out << " | JSPROP_READONLY";
			out << ", 0, 0}," << endl;
		}
		out << "	{ 0, 0, 0, 0, 0 }" << endl
			<< "};" << endl 
			<< endl;
	}
	if (variables.staticVars) {
		out << "JSPropertySpec " << className << "::" << className << "_JSPropertySpec_static[] = {" << endl;
		for (int i=0; i<variables.size; i++) {
			Variable var = variables[i];
			if (var._static) {
				out << "	{ \"" << var.name << "\", JSVAR_" << var.name << ", JSPROP_ENUMERATE";
				if (var._readonly)
					out << " | JSPROP_READONLY";
				out << ", " << className << "::" << className << "JSGetProperty, ";
				if (!var._readonly)
					out << className << "::" << className << "JSSetProperty";
				else
					out << "0";
				out << "}," << endl;
			}
		}
		out << "	{ 0, 0, 0, 0, 0 }" << endl
			<< "};" << endl 
			<< endl;
	}
}
		
void writeHeaderDestructor(ostream &out, string &className) {
	out << "static void " << className << "JSDestructor(JSContext *cx, JSObject *obj);" << endl
		<< endl;
}

void writeCodeDestructor(ostream &out, string &className) {
	out << "///// JavaScript Destructor" << endl;
	out << "void " << className << "::" << className << "JSDestructor(JSContext *cx, JSObject *obj) {" << endl
		<< "	" << className << " *p = (" << className << "*)JS_GetPrivate(cx, obj);" << endl
		<< "	if (p) delete p;" << endl
		<< "}" << endl
		<< endl;
}

void writeFunctionGroup(ostream &out, string &className, FunctionGroup &functions, bool constructor) {
	// Check the argument count against our minimum argument count
	if (functions.minArguments>0)
		out << "	if (argc < " << functions.minArguments << ") return JS_FALSE;" << endl;

	for (FunctionLength::iterator l=functions.functions.begin(); l!=functions.functions.end(); ++l) {
		out << "	";
		// First check the number of arguments, for argc/argv statements, this should only be a minimum
		out << "if (";
		
		if (constructor && l!=functions.functions.begin())
			out << "!p && ";
		
		// Functions with argv/argc are special case (variable number of arguments allowed)
		if (l->first >= SPECIAL_ARGUMENT_COUNT) {
			int realCount = 2*SPECIAL_ARGUMENT_COUNT - l->first;
			if (realCount==0)
				out << "true";
			else
				out << "argc >= " << realCount; // Get the *real* argument minimum
		} else
			out << "argc == " << l->first;
		out << ") {" << endl;
		
		// Loop through the functions for this many of arguments
		for (size_t i=0; i<l->second.size(); i++) {
			Function func = l->second[i];
			out << "		/* " << (constructor ? "Constructor" : "Function") << ": " << func.code << " */" << endl;

			// If we have any non-special, we need to check for the right parameters
			if (func.argc > 0) {
				out << "		";
				// Avoid calling multiple functions if multiple parameter sets match
				if (i>0)
					out << "else ";
				out << "if (";
				int c=0;
				for (size_t j=0; j<func.arguments.size(); j++) {
					// Skip special arguments, since they are not from argv
					if (func.arguments[j]._special)
						continue;
					// Logical AND multiple arguments
					if (c>0)
						out << " && ";
					out << isJSValType(func.arguments[j].type) << "argv[" << c << "])";
					c++;
				}
				out << ") {" << endl;
			}
			out << "			";
			if (constructor) {
				// Constructors are simple
				out << "p = new " << className;
			} else {				
				// Functions are a bit more complicated, with more checks...
				if (func.type != "void") {
					if (func.type=="JSBool")
						out << "return (";
					else
						out << "*rval = " << convertToJSVal(func.type, className);
				}

				// Static functions don't use the pointer
				if (func._static)
					out << className << "::";
				else
					out << "p->";
				
				// All functions need parentheses
				out << func.name << "(";
			}
			if (func.arguments.size()) {
				// Constructors don't need parentheses for no arguments
				if (constructor) 
					out << "(";
				// Multiline
				if (func.arguments.size()>1)
					out << endl;
				for (size_t j=0; j<func.arguments.size(); j++) {							
					
					// Indent for multiline parameters if more than 1 argument
					if (func.arguments.size()>1)
						out << "				";
						

					Variable var = func.arguments[j];
					// Print out special parameter
					if (var._special)
						out << var._special;
					// Or print out converted argument
					else
						out << convertJSValTo(var.type) << "argv[" << j << "])";
					
					// Not last argument
					if (j+1 < func.arguments.size())
						out << ",";
					
					// Multiline parameters if more than 1 argument
					if (func.arguments.size()>1)
						out << endl;
				}
				if (func.arguments.size()>1)
					out << "			";
				if (constructor) out << ")";
			}
			// For functions we need to close up parentheses...
			if (!constructor) {
				out << ")";
				// And close up the conversion macro if we're returning
				if (func.type != "void")
					out << ")";
			}
			// Finish off with an endline...
			out << ";" << endl;
			// For functions
			if (!constructor && func.type != "JSBool")
				out << "			return JS_TRUE;" << endl;
			// Close the check for specific parameter types
			if (func.argc>0) {
				out << "		}" << endl;
			}
		}
		// Close the check for number of arguments
		out << "	}" << endl;
	}
	out << endl
		<< endl;
}
	
void writeHeaderConstructor(ostream &out, string &className) {
	out << "static JSBool " << className << "JSConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);" << endl
		<< endl;
}

void writeCodeConstructor(ostream &out, string &className, string &parentClass, ConstructorMap &constructors) {
	out << "///// JavaScript Constructor" << endl;
	out << "JSBool " << className << "::" << className << "JSConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {" << endl;
	out << "	" << className << " *p = NULL;" << endl;
	writeFunctionGroup(out, className, constructors, true);

	out << "	if (!p || !JS_SetPrivate(cx, obj, p)) return JS_FALSE;" << endl
		<< "	p->" << className << "_JSinternal.o = obj;" << endl;
	if (parentClass.length())
		out << "	JS_SetPrototype(cx,obj," << parentClass << "::" << parentClass << "newJSObject(cx));" << endl;
		
	out << "	*rval = OBJECT_TO_JSVAL(obj);" << endl
		<< "	return JS_TRUE;" << endl
		<< "}" << endl 
		<< endl;
}

void writeHeaderGetObject(ostream &out, string &parentClass, string &className) {
	out << "///// JavaScript Object Linking" << endl
		<< "struct " << className << "_JSinternalStruct {" << endl
		<< "	JSObject *o;" << endl
		<< "	" << className << "_JSinternalStruct() : o(NULL) {};" << endl
		<< "	~" << className << "_JSinternalStruct() { if (o) JS_SetPrivate(NULL,o, NULL); };" << endl
		<< "};" << endl
		<< className << "_JSinternalStruct " << className << "_JSinternal;" << endl
		<< "JSObject *" << className << "getJSObject(JSContext *cx);" << endl
		<< "static JSObject *" << className << "newJSObject(JSContext *cx);" << endl
		<< endl;
}
void writeCodeGetObject(ostream &out, string &className, string &parentClass) {
	out << "///// JavaScript Object Linking" << endl
		<< "JSObject *" << className << "::" << className << "getJSObject(JSContext *cx) {" << endl
		<< "	if (!cx) return NULL;" << endl
		<< "	if (!" << className << "_JSinternal.o) {" << endl
		<< "		" << className << "_JSinternal.o = " << className << "newJSObject(cx);" << endl
		<< "		if (!JS_SetPrivate(cx, " << className << "_JSinternal.o, this)) return NULL;" << endl
		<< "	}" << endl
		<< "	return " << className << "_JSinternal.o;" << endl
		<< "}" << endl
		<< endl;
	out << "JSObject *" << className << "::" << className << "newJSObject(JSContext *cx) {" << endl
		<< "	return JS_NewObject(cx, &" << className << "::" << className << "_jsClass, ";
		
	if (parentClass.length())
		out << parentClass << "::" << parentClass << "newJSObject(cx)";
	else
		out << "NULL";
	out << ", NULL);" << endl
		<< "}" << endl
		<< endl;
}

void writeHeaderClass(ostream &out, string &className) {
	out << "static JSClass " << className << "_jsClass;" << endl
		<< endl;
}

void writeCodeClass(ostream &out, string &className, VariableList &variables, FunctionMap &functions) {
	out << "///// JavaScript Class Definition" << endl;
	out << "JSClass " << className << "::" << className << "_jsClass = {" << endl 
		<< "	\"" << className << "\", JSCLASS_HAS_PRIVATE," << endl
		<< "	JS_PropertyStub, JS_PropertyStub," << endl
		<< "	";
		if (variables.size)
			out << className << "::" << className << "JSGetProperty, ";
		else
			out << "JS_PropertyStub, ";
		if (variables.setVars)
			out << className << "::" << className << "JSSetProperty," << endl;
		else
			out << "JS_PropertyStub," << endl;
		
	out << "	JS_EnumerateStub, JS_ResolveStub," << endl
		<< "	JS_ConvertStub, " << className << "::" << className << "JSDestructor," << endl
		<< "	0, 0, 0, 0, " << endl
		<< "	0, 0, 0, 0" << endl
		<< "};" << endl
		<< endl;
}
	
void writeHeaderInit(ostream &out, string &className) {
	out << "static JSObject *" << className << "JSInit(JSContext *cx, JSObject *obj = NULL);" << endl
		<< endl;
}

void writeCodeInit(ostream &out, string &className, ConstructorMap &constructors, VariableList &variables, FunctionMap &functions, string &parentClass) {
	out << "///// JavaScript Initialization Method" << endl
		<< "JSObject *" << className << "::" << className << "JSInit(JSContext *cx, JSObject *obj) {" << endl
		<< "	if (obj==NULL)" << endl
		<< "		obj = JS_GetGlobalObject(cx);" << endl
		<< "	jsval oldobj;" << endl
		<< "	if (JS_TRUE == JS_LookupProperty(cx, obj, " << className << "::" << className << "_jsClass.name, &oldobj) && JSVAL_IS_OBJECT(oldobj))" << endl
		<< "		return JSVAL_TO_OBJECT(oldobj);" << endl
		<< "	return JS_InitClass(cx, obj, ";
	if (parentClass.length()) out << parentClass << "::" << parentClass << "JSInit(cx,obj)";
	else out << "NULL";
	out << ", &" << className << "::" << className << "_jsClass," << endl
		<< "    	                                 ";

	if (constructors.functions.size()) out << className << "::" << className << "JSConstructor, " << constructors.minArguments << "," << endl;
	else out << "NULL, 0," << endl;
	
	out << "    	                                 ";
		
	if (variables.nonStaticVars) out << className << "::" << className << "JSPropertySpec, ";
	else out << "NULL, ";
		
	if (functions.nonStaticFuncs) out << className << "::" << className << "_JSFunctionSpec," << endl;
	else out << "NULL," << endl;
	
	out << "    	                                 ";
	
	if (variables.staticVars) out << className << "::" << className << "_JSPropertySpec_static, ";
	else out << "NULL, ";
		
	if (functions.staticFuncs) out << className << "::" << className << "_JSFunctionSpec_static);" << endl;
	else out << "NULL);" << endl;
		
	out << "}" << endl
		<< endl;
}


void writeHeaderVariablesEnum(ostream &out, VariableList &variables) {
	out << "///// JavaScript Class Variable IDs" << endl;
	out << "enum {" << endl;
	for (int i=0; i<variables.size; i++) {
		Variable var = variables[i];
		out << "	JSVAR_" << var.name << "," << endl;
	}
	out << "	JSVAR_LASTENUM" << endl
		<< "};" << endl 
		<< endl;
}
void writeHeaderVariables(ostream &out, VariableList &variables) {
	if (!variables.size) return;
	
	out << "///// JavaScript Variable Definitions" << endl;
	for (int i=0; i<variables.size; i++) {		
		Variable var = variables[i];
		if (var.normal) {
			if (!var._const) {
				if (var._static)
					out << "static ";
				out << var.type << " " << var.name << ";" << endl;
			}
		} else {
			if (var._static)
				out << "static ";
			else if (var._virtual)
				out << "virtual ";
			out << var.type << " get" << getUpperCaseFirst(var.name) << "()";
			if (var._const)
				out << " const";
			out << ";" << endl;
			if (!var._readonly) {
				if (var._static)
					out << "static ";
				else if (var._virtual)
					out << "virtual ";
				out << "void set" << getUpperCaseFirst(var.name) << "(" << var.type << " " << var.name << ");" << endl;
			}
			out << endl;
		}
	}
	out <<  endl;
}
void writeHeaderSetProperty(ostream &out, VariableList &variables, string& className) {
	if (!variables.setVars) return;
	out << "static JSBool " << className << "JSSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);" << endl
		<< endl;
}
void writeCodeSetProperty(ostream &out, VariableList &variables, string &className) {
	if (!variables.setVars) return;
	out << "///// JavaScript Set Property Wrapper" << endl;
	
	out << "JSBool " << className << "::" << className << "JSSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {" << endl
		<< "	if (JSVAL_IS_INT(id)) {" << endl;
	if (variables.nonStaticVars)
		out << "		" << className << " *priv;" << endl;
	out << "		switch(JSVAL_TO_INT(id)) {" << endl;


	for (int i=0; i<variables.size; i++) {
		Variable var = variables[i];
		if (!var._readonly) {
			out << "			case JSVAR_" << var.name << ":" << endl;
			if (var._static)
				out << "				" << className << "::";
			else {
				out << "				priv = (" << className << "*)JS_GetPrivate(cx, obj);" << endl
					<< "				if (priv!=NULL)" << endl;
				out << "					priv->";
			}
			if (var.normal)
				out << var.name << " = " << convertJSValTo(var.type) << "*vp);" << endl;
			else
				out << "set" << getUpperCaseFirst(var.name) << "(" << convertJSValTo(var.type) << "*vp));" << endl;
			out << "				break;" << endl
				<< endl;
		}
	}
	out << "		}" << endl
		<< "	}" << endl
		<< "	return JS_TRUE;" << endl
		<< "}" << endl
		<< endl;
}

void writeHeaderGetProperty(ostream &out, VariableList &variables, string& className) {
	if (!variables.size) return;
	out << "static JSBool " << className << "JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);" << endl
		<< endl;
	
}
void writeCodeGetProperty(ostream &out, VariableList &variables, string &className) {
	if (!variables.size) return;
	out << "///// JavaScript Get Property Wrapper" << endl;
	out << "JSBool " << className << "::" << className << "JSGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {" << endl
		<< "	if (JSVAL_IS_INT(id)) {" << endl;
	if (variables.nonStaticVars)
		out << "		" << className << " *priv;" << endl;
	out << "		switch(JSVAL_TO_INT(id)) {" << endl;
	
	for (int i=0; i<variables.size; i++) {
		Variable var = variables[i];
		out << "			case JSVAR_" << var.name << ":" << endl;
		if (!var._static) {
			out << "				priv = (" << className << "*)JS_GetPrivate(cx, obj);" << endl
				<< "				if (priv==NULL)" << endl 
				<< "					*vp = JSVAL_NULL;" << endl
				<< "				else" << endl
				<< "	";
		}
		out << "				*vp = " << convertToJSVal(var.type, className); 
		if (var._static)
			out << className << "::";
		else
			out << "priv->";
		if (var.normal) {
			out << var.name << ");" << endl;
		} else {
			out << "get" << getUpperCaseFirst(var.name) << "());" << endl;
		}
		out << "				break;" << endl 
			<< endl;
	}
	out << "		}" << endl
		<< "	}" << endl
		<< "	return JS_TRUE;" << endl
		<< "}" << endl
		<< endl;
}

void writeHeaderFunctions(ostream &out, FunctionMap &functions, string &className) {
	if (!functions.size) return;
	out << "///// JavaScript Function Wrapper Prototypes" << endl;
	for (FunctionMap::iterator i=functions.begin(); i!=functions.end(); ++i) {
		for (FunctionLength::iterator l=i->second.functions.begin(); l!=i->second.functions.end(); ++l) {
			for (size_t j=0; j<l->second.size(); j++) {
				out << "/* Function: " << l->second[j].code << " */" << endl;
			}
		}
		out << "static JSBool " << className << "JSFUNC_" << i->first << "(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);" << endl
			<< endl;
	}
	out << endl;
}

void writeCodeFunctions(ostream &out, FunctionMap &functions, string &className) {
	if (!functions.size) return;
	out << "///// JavaScript Function Wrappers" << endl;
	for (FunctionMap::iterator g=functions.begin(); g!=functions.end(); ++g) {
		out << "JSBool " << className << "::" << className << "JSFUNC_" << g->first << "(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {" << endl;
		FunctionGroup &group = g->second;
		if (group.nonStaticFuncs)
			out << "	" << className << " *p = (" << className << "*)JS_GetPrivate(cx, obj);" << endl;

		writeFunctionGroup(out, className, group, false);
			
		out << "	return JS_FALSE;" << endl
			<< "}" << endl
			<< endl;
	}
	out << endl;
}



