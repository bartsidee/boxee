
#include "ScriptDirectory.h"
#include "PluginDirectory.h"
#include "Util.h"
#include "lib/libPython/XBPython.h"
#include "GUIWindowManager.h"
#include "URL.h"
#include "utils/log.h"

namespace DIRECTORY
{

CScriptDirectory::CScriptDirectory()
{
}

CScriptDirectory::~CScriptDirectory()
{
}

bool CScriptDirectory::GetDirectory(const CStdString& _strPath, CFileItemList &items)
{
	
	CStdString strPath = _strPath;
	CURI url(strPath);

	// path is special://home/scripts/<path from here>
	CStdString pathToScript = "special://home/scripts/";
	CUtil::AddFileToFolder(pathToScript, url.GetHostName(), pathToScript);
	printf("\n\nRun Script: pathToScript = %s\n\n", pathToScript.c_str());
	fflush(stdout);
	
	CUtil::AddFileToFolder(pathToScript, "default.py", pathToScript);
	
	// setup our parameters to send the script
	CStdString strHandle;
	strHandle.Format("%i", -1);

	// run the script
	if (g_pythonParser.evalFile(pathToScript.c_str()) < 0)
		CLog::Log(LOGERROR, "Unable to run script %s", pathToScript.c_str());
	
	g_windowManager.PreviousWindow();
	
	
	return false;
}

bool CScriptDirectory::Exists(const char* strPath)
{
	return true;
}


}
