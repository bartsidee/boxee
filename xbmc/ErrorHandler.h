#include <stdlib.h>
#include <new>
#include <exception>
#include "utils/log.h"

#ifdef _WIN32_ALREADY_IN_WIN32EXCEPTION
#include "StackWalker.h"
#include <eh.h>

class BoxeeStackWalker : public StackWalker
{
public:
  BoxeeStackWalker() : StackWalker() {}
  BoxeeStackWalker(DWORD dwProcessId, HANDLE hProcess) : StackWalker(dwProcessId, hProcess) {}
  virtual void OnOutput(LPCSTR szText) {
    CLog::Log(LOGFATAL, "An exception occured. Stack Trace is:\n%s", szText);
  }
};

void boxee_se_translator(unsigned int u, EXCEPTION_POINTERS* pExp)
{
  BoxeeStackWalker sw;
  sw.ShowCallstack(GetCurrentThread(), pExp->ContextRecord);
  throw std::exception("SE Exception");
}

#endif

void boxee_new_handler() {
  //CLog::Log(LOGFATAL, "New handler called (no memory on the heap)");	// Causes stack-overflow in the call to new-handler, since the log function is trying to allocate additional memory.
  //abort();
  exit(1);
}

void boxee_terminate_handler() {
  CLog::Log(LOGFATAL, "Terminate handler called (abnormal termination)");
  //abort();  // forces abnormal termination
  exit(2);
}

void boxee_pure_call_handler()
{
  CLog::Log(LOGFATAL, "Pure call handler called (pure virtual function was called)");
  //abort();
  exit(3);
}

class StaticErrorHandler {
public:

  StaticErrorHandler() {
#ifdef _WIN32
    //m_prev_se_func = _set_se_translator(boxee_se_translator);	// Already in Win32Exception.cpp
    m_purecall_handler = _set_purecall_handler(boxee_pure_call_handler);
#endif
    m_prev_terminate_handler = std::set_terminate(boxee_terminate_handler);
    m_prev_new_handler = std::set_new_handler(boxee_new_handler);
  }

  ~StaticErrorHandler() {
#ifdef _WIN32
    //_set_se_translator( m_prev_se_func );
    _set_purecall_handler(m_purecall_handler);
#endif
    std::set_terminate(m_prev_terminate_handler);
    std::set_new_handler(m_prev_new_handler);
  }

private:

#ifdef _WIN32
  //_se_translator_function m_prev_se_func;
  _purecall_handler m_purecall_handler;
#endif
  std::terminate_handler m_prev_terminate_handler;
  std::new_handler m_prev_new_handler;
};

class GlobalErrorHandler {
public:
  static StaticErrorHandler& get_instance() {
    static StaticErrorHandler s_err_handlr;
    return s_err_handlr;
  }
  GlobalErrorHandler() {
    (void) get_instance();  // Force the static constructor.
  }
};
