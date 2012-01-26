
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "logger.h"

using namespace std;
 
namespace BOXEE {

LogRecord::LogRecord() :m_line(0) {
}

LogRecord::LogRecord(const char *szFile, const char *szFunc, int nLine) {
	m_file = szFile;
	m_function = szFunc;
	m_line = nLine;
}

LogRecord::~LogRecord() {
}

void LogRecord::operator()(Logger::BXLogLevel level, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	Logger::GetLogger().Log(level, *this, fmt, args);
	va_end(args);
}

Logger::Logger() : m_logLevel(Logger::BX_LOG_DEBUG), m_pLoggerFunction(DefaultLoggerFunction)
{
	m_lock = SDL_CreateMutex();
}


Logger::~Logger()
{
	SDL_DestroyMutex(m_lock);
}

Logger &Logger::GetLogger() {
	static Logger instance;
	return instance;
}

void Logger::Log(BXLogLevel level, const LogRecord &logRecord, const char *fmt, va_list args) 
{
  if (level >= m_logLevel) 
  {
    
    SDL_mutexP(m_lock);
    
    static char szMsg[1024] = { 0 };
    static char szTime[256] = { 0 } ;
    static char szCompleteMsg[2048] = { 0 };
    
    vsnprintf(szMsg, 1023, fmt, args);
    
    time_t now = time(NULL);
    strftime(szTime,255,"%d/%m/%y %H:%M:%S", gmtime(&now));
    
    snprintf(szCompleteMsg, 2047, "%s#%s#%s:%d(%s)#%s\n", szTime, 
             StrLogLevel(level).c_str(), logRecord.m_file.c_str(),
             logRecord.m_line, logRecord.m_function.c_str(), szMsg);
    
    m_pLoggerFunction(szCompleteMsg);
    
    SDL_mutexV(m_lock);
  }
}

void Logger::DefaultLoggerFunction(const char *szLogMsg) {
	fprintf(stdout,szLogMsg);
}

string Logger::StrLogLevel(BXLogLevel level) {
	string strRet = "UNKNOWN";
	switch(level){
		case BX_LOG_DEBUG:
			strRet="DEBUG";
			break;
		case BX_LOG_INFO:
			strRet="DEBUG";
			break;
		case BX_LOG_WARNING:
			strRet="WARNING";
			break;
		case BX_LOG_ERROR:
			strRet="ERROR";
			break;
	
	}

	return strRet;
}

Logger::BXLogLevel Logger::GetLogLevel() {
	return m_logLevel;
}

void Logger::SetLogLevel(BXLogLevel level) {
	m_logLevel = level;
}

void Logger::SetLoggerFunction(LoggerFunc func)
{
	m_pLoggerFunction = func;
}

} // namespace BOXEE

