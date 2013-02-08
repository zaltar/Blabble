/**********************************************************\
Original Author: Andrew Ofisher (zaltar)

License:    GNU General Public License, version 3.0
            http://www.gnu.org/licenses/gpl-3.0.txt

Copyright 2012 Andrew Ofisher
\**********************************************************/

#ifndef H_BlabbleLoggingPLUGIN
#define H_BlabbleLoggingPLUGIN

#include <string>
#include <sstream>
#include "log4cplus/logger.h"
#include "log4cplus/loggingmacros.h"

namespace BlabbleLogging {

	/*! @Brief Initilize log4cplus
	 *  Sets up log4cplus with a rolling file appender. 
	 *  Currently it is hardcoded to keep 5 files of 10MB each.
	 *
	 *  @ToDo Make this more configurable.
	 */
	void initLogging();
	
	/*! This is used by PJSIP to log via log4plus.
	 */
	void blabbleLog(int level, const char* data, int len);

	/*! @Brief Return the path to the log file
	 *  Currently the log file will be stored in the userprofile path on windows, 
	 *  directly on the C drive if that fails, or under /tmp on unix platforms.
	 *
	 *  @ToDo Make this more configureable.
	 */
	std::wstring getLogFilename();

	/*! @Brief Map between PJSIP log levels and log4cplus
	 */
	log4cplus::LogLevel mapPJSIPLogLevel(int pjsipLevel);

	extern bool logging_started;
	extern log4cplus::Logger blabble_logger;
	extern log4cplus::Logger js_logger;
}

#define BLABBLE_LOG_TRACE(what)								\
	do {													\
			if (BlabbleLogging::logging_started) {			\
				LOG4CPLUS_TRACE(							\
					BlabbleLogging::blabble_logger, what);	\
			}												\
	} while(0)

#define BLABBLE_LOG_DEBUG(what)								\
	do {													\
			if (BlabbleLogging::logging_started) {			\
				LOG4CPLUS_DEBUG(							\
					BlabbleLogging::blabble_logger, what);	\
			}												\
	} while(0)

#define BLABBLE_LOG_WARN(what)								\
	do {													\
			if (BlabbleLogging::logging_started) {			\
				LOG4CPLUS_WARN(								\
					BlabbleLogging::blabble_logger, what);	\
			}												\
	} while(0)

#define BLABBLE_LOG_ERROR(what)								\
	do {													\
			if (BlabbleLogging::logging_started) {			\
				LOG4CPLUS_ERROR(							\
					BlabbleLogging::blabble_logger, what);	\
			}												\
	} while(0)

#define BLABBLE_JS_LOG(level, what)							\
    do {													\
		log4cplus::LogLevel log4level =						\
			BlabbleLogging::mapPJSIPLogLevel(level);		\
        if(BlabbleLogging::js_logger.isEnabledFor(log4level)) {		\
            log4cplus::tostringstream _log4cplus_buf;		\
            _log4cplus_buf << what;							\
            BlabbleLogging::js_logger.						\
				forcedLog(log4level,						\
                _log4cplus_buf.str(), __FILE__, __LINE__);	\
        }													\
    } while (0)

#endif // H_BlabbleLoggingPLUGIN
