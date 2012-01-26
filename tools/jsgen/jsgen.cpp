#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <ctype.h>

using namespace std;

/* Include some helper IO functions */
#include "jsgenio.cpp"

#define SPECIAL_ARGUMENT_COUNT 100000

/* The variable struct used for variables and function/constructor arguments */
struct Variable {
	Variable() : _readonly(false),normal(false),_virtual(false),_const(false),_enum(false),_static(false),_special(NULL) {};
	string type;
	string name;
	bool _readonly;
	bool normal;
	bool _virtual;
	bool _const;
	bool _enum;
	bool _static;
	const char *_special;
};
typedef vector<Variable> VariableVector;

/* A list of variables that stores some statistical information about the contained variables */
struct VariableList : public VariableVector {
	VariableList() {
		nonStaticVars = 0;
		staticVars = 0;
		size = 0;
		setVars = 0;
	}
	int nonStaticVars;
	int staticVars;
	int size;
	int setVars;
};

/* The function struct used for functions and constructors */
struct Function {
	Function() : _static(false), argc(0), _has_argc(false), _has_argv(false) {}
	string name;
	string type;
	string code;
	bool _static;
	int argc;
	bool _has_argc;
	bool _has_argv;
	VariableVector arguments;
};

/* A vector of functions */
typedef vector<Function> FunctionList;

/* A map of function argument sizes to function lists */
typedef map<int, FunctionList> FunctionLength;

/* A struct that stores a group of functions of the same name and various statistical information about them. */
struct FunctionGroup {
	FunctionGroup() : minArguments(-1), nonStaticFuncs(0), staticFuncs(0), special(false) {};
	string name;
	int minArguments;
	int nonStaticFuncs;
	int staticFuncs;
	bool special;
	Function specialFunc;
	FunctionLength functions;
};

/* A STL map of function names to function groups */
typedef map<string, FunctionGroup> FunctionGroupMap;

/* The struct for managing a list of functions of different names */
struct FunctionMap : public FunctionGroupMap {
	FunctionMap() {
		nonStaticFuncs = 0;
		staticFuncs = 0;
		size = 0;
	}
	int size;
	int nonStaticFuncs;
	int staticFuncs;
};

/* A constructor map is just a function group (since all constructors have the same "name") */
typedef FunctionGroup ConstructorMap;

/* Include the code generation bits */
#include "jsgenjs.cpp"

/* Adds a variable to a variable list and updates the statistical information */
void addVariable(VariableList &variables, Variable &var) {
	if (var._static)
		variables.staticVars++;
	else
		variables.nonStaticVars++;
	if (!var._readonly && !var._const)
		variables.setVars++;

	variables.size++;
	variables.push_back(var);
}

/* Reads a string containing formatting variable definitions into a VariableList */
void readVariables(const string code, VariableList &variables) {
	istringstream in(code);
	
	while (!in.eof()) {
		Variable var;

		readWhiteSpace(in);
		
		istringstream command(readTo(in,";"));

		while (true) {
			command >> var.type;
			if (var.type=="static")
				var._static = true;
			else if (var.type=="virtual")
				var._virtual = true;
			else if (var.type=="const")
				var._const = true;
			else
				break;
		}
		
		command >> var.name;
		
		
		if (!var.name.length())
			continue;
			
		while (var.name[0]=='*' || var.name[0]=='&') {
			var.type += var.name[0];
			var.name = var.name.substr(1);
		}
		if (var.type[var.type.length()-1]=='&')
			var.type = var.type.substr(0,var.type.length()-1);
		
		while (!command.eof()) {
			string flag;
			command >> flag;
			
			if (flag=="readonly")
				var._readonly = true;
			else if (flag=="normal")
				var.normal = true;
			else if (flag=="const")
				var._const = true;
		}

		addVariable(variables,var);
	}
}

/* Reads a string containing the inner part of an enum into a VariableList as const static readonly normal variables. */
void readEnums(const string code, VariableList &variables) {
	istringstream in(code);
	
	while (!in.eof()) {
		Variable var;
		
		string e = readTo(in,",");
		
		istringstream in2(e);
		
		var.name = readWordTo(in2, "=");
		var.type = "int";
		var._const = true;
		var._static = true;
		var._readonly = true;
		var.normal = true;
		
		addVariable(variables,var);
	}
}

/* Reads a string containing arguments into a Function */
void readArguments( string str, Function &func) {
	istringstream in2(str);

	while (!in2.eof()) {
		Variable var;
		in2 >> var.type;
		var.name = readWordTo(in2,",");
		while (var.name[0]=='*' || var.name[0]=='&') {
			var.type += var.name[0];
			var.name = var.name.substr(1);
		}
		if (var.type[var.type.length()-1]=='&')
			var.type = var.type.substr(0,var.type.length()-1);
		if (!var.name.length())
			continue;
		
		if (var.type=="JSContext*")
			var._special = "cx";

		if (var.type=="uintN" && var.name=="argc") {
			func._has_argc = true;
			var._special = "argc";
		}

		if (var.type=="JSObject*" && var.name=="_this")
			var._special = "obj";

		if (var.type=="jsval*") {
			if (var.name == "argv") {
				var._special = "argv";
				func._has_argv = true;
			} else 
				var._special = "rval";
		}
		
		func.arguments.push_back(var);
		if (!var._special)
			func.argc++;
	}
}
void addFunctionToGroup(FunctionGroup &group, Function &func, bool constructor) {
	if (group.minArguments<0 || func.argc < group.minArguments)
		group.minArguments = func.argc;

	group.name = func.name;

	if (func._static)
		group.staticFuncs++;
	else
		group.nonStaticFuncs++;
	group.functions[(func._has_argv || func._has_argc) ? 2*SPECIAL_ARGUMENT_COUNT-func.argc : func.argc].push_back(func);
}
void readFunctions(const string code, FunctionMap &functions) {
	istringstream in_code(code); 
	
	while (!in_code.eof()) {
		Function func;
		try {
			readWhiteSpace(in_code);
		} catch (IOException e) {}
		func.code = readTo(in_code,";");
		
		istringstream in(func.code);
		
		while (true) {
			in >> func.type;
			if (func.type=="static")
				func._static = true;
			else if (func.type=="virtual")
				continue;
			else
				break;
		}
		
		
		func.name = readWordTo(in, "(");
		if (!func.name.length())
			continue;
		while (func.name[0]=='*' || func.name[0]=='&') {
			func.type += func.name[0];
			func.name = func.name.substr(1);
		}
		if (func.type[func.type.length()-1]=='&')
			func.type = func.type.substr(0,func.type.length()-1);
		
		string arguments = readTo(in, ")");
		
		readArguments(arguments,func);
		
		functions.size++;
		if (func._static)
			functions.staticFuncs++;
		else
			functions.nonStaticFuncs++;
			
		addFunctionToGroup(functions[func.name], func, false);
	}
}

void readConstructors(const string code, ConstructorMap &constructors) {
	istringstream in_code(code); 
	
	while (!in_code.eof()) {
		Function func;
		try {
			readWhiteSpace(in_code);
		} catch (IOException e) {}
		func.code = readTo(in_code,";");
		
		istringstream in(func.code);
		
		
		func.name = readWordTo(in, "(");
		if (!func.name.length())
			continue;
		
		string arguments = readTo(in, ")");
		
		readArguments(arguments,func);
				
		addFunctionToGroup(constructors, func, true);
	}
}



int main(int argc, char **argv) {
	if (argc<4) {
		cerr << "Not enough parameters." << endl;
		return -1;
	}
	ifstream in(argv[1]);
	ofstream out_h(argv[2]), out_cpp(argv[3]);
	
	if (!in.good() || !out_h.good() || !out_cpp.good()) {
		cerr << "Error opening file." << endl;
		return -1;
	}
	
	out_h << "// Generated source file -- DO NOT EDIT" << endl
		<< "// Javascript code from \"" << argv[1] << "\"" << endl 
		<< endl
		<< "public:" << endl
		<< endl
		<< endl;

	out_cpp << "// Generated source file -- DO NOT EDIT" << endl
		<< "// Javascript code from \"" << argv[1] << "\"" << endl 
		<< endl
		<< endl;
		
	VariableList variables;
	FunctionMap functions;
	ConstructorMap constructors;
	
	string className = "Unknown";
	string parentClass;
	
	try {
		do {
			readToWord("class",in);
			in >> className;
		} while (className[className.length()-1] == ';');
		
		string read;
		
		in >> read;
		if (read==":") {
			in >> read;
			if (read=="public" || read=="protected")
				in >> parentClass;
		}
		
		while (!in.eof()) {
			readToWord("/*",in);
			readWhiteSpace(in);
			readToWord("javascript ", in);
			
			string word;
			in >> word;
			if (word == "variables" || word=="vars" || word=="variable" || word=="var" || word=="v") {
				readVariables(readTo(in, "*/"), variables);
			} else if (word == "functions" || word=="function" || word=="methods" || word=="method" || word=="f" || word=="m") {
				readTo(in,"*/");
				readFunctions(readTo(in, "/* end */"), functions);
			} else if (word == "constructors" || word=="constructor" || word=="c") {
				readTo(in,"*/");
				readConstructors(readTo(in, "/* end */"), constructors);
			} else if (word == "enums" || word=="enum" || word=="e") {
				readTo(in,"*/");
				readEnums(readTo(in, "/* end */"), variables);
			}

		}
	} catch (IOException &ex) {
	}
	
	writeHeaderClass(out_h, className);
	
	writeHeaderMacros(out_h, className);

	writeHeaderInit(out_h, className);
	if (constructors.functions.size())
		writeHeaderConstructor(out_h, className);
	writeHeaderDestructor(out_h, className);
	
	writeHeaderGetObject(out_h, parentClass, className);

	writeHeaderVariablesEnum(out_h, variables);
	writeHeaderVariables(out_h, variables);	
	
	
	writeHeaderPropertySpec(out_h, variables, className);
	writeHeaderGetProperty(out_h, variables, className);
	writeHeaderSetProperty(out_h, variables, className);

	writeHeaderFunctionSpec(out_h, functions, className);
	writeHeaderFunctions(out_h, functions, className);	


	writeCodeClass(out_cpp, className, variables, functions);
	writeCodeInit(out_cpp, className, constructors, variables, functions, parentClass);
	if (constructors.functions.size())
		writeCodeConstructor(out_cpp, className, parentClass, constructors);
	writeCodeDestructor(out_cpp, className);
	
	writeCodeGetObject(out_cpp, className, parentClass);

	writeCodePropertySpec(out_cpp, variables, className);
	writeCodeGetProperty(out_cpp, variables, className);
	writeCodeSetProperty(out_cpp, variables, className);
		
	writeCodeFunctionSpec(out_cpp, functions, className);
	writeCodeFunctions(out_cpp, functions, className);
	
	return 0;	
}

