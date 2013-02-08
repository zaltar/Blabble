#include "BlabbleLogging.h"
#include "log4cplus/config/defines.hxx"
#include "log4cplus/loglevel.h"
#include "log4cplus/layout.h"
#include "log4cplus/ndc.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/hierarchy.h"
#include "utf8_tools.h"

#ifdef WIN32
#include <Windows.h>
#endif

namespace BlabbleLogging {
	/*! Keep track of whether or not logging has been initilized
	 */
	bool logging_started = false;

	/*! Keep the logger around for reference
	 */
	log4cplus::Logger blabble_logger, js_logger;
}

void BlabbleLogging::initLogging()
{
	if (BlabbleLogging::logging_started)
        return;

	log4cplus::Logger logger = log4cplus::Logger::getDefaultHierarchy().getRoot();
	log4cplus::SharedAppenderPtr fileAppender(new log4cplus::RollingFileAppender(BlabbleLogging::getLogFilename(), 10*1024*1024, 5));
	std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(L"%d{%d-%m-%Y %H:%M:%S,%q} [%t] %c - %m%n"));
    fileAppender->setLayout(layout);
    logger.addAppender(fileAppender);
    
	BlabbleLogging::blabble_logger = log4cplus::Logger::getInstance(L"Blabble");
	BlabbleLogging::js_logger = log4cplus::Logger::getInstance(L"JavaScript");

    BlabbleLogging::logging_started = true;
}

std::wstring BlabbleLogging::getLogFilename()
{
#if defined(XP_WIN)
	WCHAR profilepath[256];
	int profileLen = ExpandEnvironmentStringsW(L"%userprofile%", profilepath, 255);
	if (profileLen > 0)
	{
		return std::wstring(profilepath, 0, profileLen) + L"\\blabble.log";
	}
    return L"C:\\blabble.log";
#elif defined(XP_UNIX)
    return L"/tmp/blabble.log";
#endif
}

void BlabbleLogging::blabbleLog(int level, const char* data, int len)
{
	if (BlabbleLogging::logging_started)
	{
		wchar_t* wdata = new wchar_t[len + 1];
		for (int i = 0; i < len; i++) 
		{
			wdata[i] = static_cast<wchar_t>(data[i]);
		}
		wdata[len] = 0;
		LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance(L"Blabble"), std::wstring(wdata, 0, len));
		delete[] wdata;
	}
}

log4cplus::LogLevel BlabbleLogging::mapPJSIPLogLevel(int pjsipLevel) {
	switch (pjsipLevel) {
		case 0:
			return log4cplus::FATAL_LOG_LEVEL;
		case 1:
			return log4cplus::ERROR_LOG_LEVEL;
		case 2:
			return log4cplus::WARN_LOG_LEVEL;
		case 3:
			return log4cplus::INFO_LOG_LEVEL;
		case 4:
		case 5:
			return log4cplus::DEBUG_LOG_LEVEL;
		default:
			return log4cplus::TRACE_LOG_LEVEL;
	}
}