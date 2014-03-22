/* 
 * File:   defines.h
 * Author: s0457958
 *
 * Created on 20 March 2014, 10:18
 */

#ifndef DEFINES_H
#define	DEFINES_H

#include <stddef.h>
#include <cassert>
#include <iostream>
#include <sstream>

#define __packed__ __attribute__((packed))

enum LogLevels {
	ERROR,
	WARNING,
	INFO,
	DEBUG,
};

class Log {
public:
	Log(LogLevels level_ = INFO) : level(level_)
	{
	}

	~Log()
	{
		switch (level) {
		case ERROR:
			std::cerr << "error: ";
			break;
			
		case WARNING:
			std::cerr << "warning: ";
			break;
			
		case INFO:
			std::cerr << "info: ";
			break;
			
		case DEBUG:
			std::cerr << "debug: ";
			break;
		}
		
		std::cerr << os.str() << std::endl;
	}

	std::ostringstream& GetStream()
	{
		return os;
	}

private:
	LogLevels level;
	std::ostringstream os;

private:
	Log(const Log&); // DO NOT COPY
	Log& operator=(const Log&); // DO NOT ASSIGN
};

#define LOG(level) Log(level).GetStream()

#endif	/* DEFINES_H */
