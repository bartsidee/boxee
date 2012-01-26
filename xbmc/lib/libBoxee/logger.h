// Copyright © 2008 BOXEE. All rights reserved.
/*
* libBoxee
*
*
*
* --- property of boxee.tv
* 
*/
#ifndef BOXEELOGGER_H
#define BOXEELOGGER_H

#include <stdarg.h>
#include <string>

// only use SDL for locks (to maintain platform independence)
#include <SDL/SDL.h>

#ifdef _WIN32
#define snprintf _snprintf
#endif

#define LOG_LEVEL_DEBUG 	BOXEE::Logger::BX_LOG_DEBUG
#define LOG_LEVEL_INFO 		BOXEE::Logger::BX_LOG_INFO
#define LOG_LEVEL_WARNING 	BOXEE::Logger::BX_LOG_WARNING
#define LOG_LEVEL_ERROR 	BOXEE::Logger::BX_LOG_ERROR
#define LOG BOXEE::LogRecord(__FILE__,__FUNCTION__,__LINE__)

namespace BOXEE {

typedef void (*LoggerFunc)(const char *);

/**
logger for boxee package
*/

struct LogRecord;

class Logger{
public:
	typedef enum { BX_LOG_DEBUG=0, BX_LOG_INFO, BX_LOG_WARNING, BX_LOG_ERROR } BXLogLevel;

    ~Logger();
	static Logger &GetLogger();
	static std::string StrLogLevel(BXLogLevel level);

	void Log(BXLogLevel level, const LogRecord &logRecord, const char *fmt, va_list args);
	
	BXLogLevel GetLogLevel();
	void SetLogLevel(BXLogLevel level);
	
	// override the default logger function (dump to stdout) with a custom one
	void SetLoggerFunction(LoggerFunc func);

protected:
    Logger();
	static void DefaultLoggerFunction(const char *szLogMsg);
	
	BXLogLevel 	m_logLevel;
	LoggerFunc 	m_pLoggerFunction;
	SDL_mutex 	*m_lock;
};

struct LogRecord {
public:
	LogRecord() ;
	LogRecord(const char *szFile, const char *szFunc, int nLine) ;
	virtual ~LogRecord();

	int  		m_line;
	std::string	m_file;
	std::string	m_function;

	void operator()(Logger::BXLogLevel level, const char *fmt, ...);
};

}

#endif

