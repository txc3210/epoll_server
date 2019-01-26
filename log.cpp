#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include "config.h"
#include <iostream>


int LogD(const char * fmt,...)
{
	int ret = 0;
#if LOG_DEBUG_ENABLE	
	char str[1024];
	va_list va;
	va_start(va,fmt);	
	//ret = printf(fmt);
	vsnprintf(str, sizeof(str), fmt, va);
	syslog(LOG_DEBUG_FACILITY, str);
	va_end(va);
#endif	
	return ret;
}
/*
int LogD(const char * fmt,...)
{
	int ret = 0;
#if LOG_DEBUG_ENABLE	
	va_list va;
	va_start(va,fmt);	
	ret = printf(fmt);
	va_end(va);
#endif	
	return ret;
}
*/

int LogE(const char * fmt,...)
{
	int ret = 0;
#if LOG_ERROR	
	char str[1024];
	va_list va;
	va_start(va,fmt);	
	vsnprintf(str, sizeof(str), fmt, va);
	syslog(LOG_ERROR_FACILITY, str);
	//ret = fprintf(stderr, str); //cant not write as fprintf(stderr,fmt);
	va_end(va);
#endif	
	return ret;
}

int LogI(const char * fmt,...)
{
	int ret = 0;
	static bool open = false;
#if LOG_INFO_ENABLE	
	char str[1024];
	va_list va;
	va_start(va,fmt);	
	vsnprintf(str, sizeof(str), fmt, va);
	if(!open)
	{

		open = true;
	}

	//syslog(LOG_INFO_FACILITY, str);
	std::cout << str << std::endl
	va_end(va);	
#endif	
	return ret;
}



int LogF(const char * fmt,...)
{
	int ret = 0;
#if LOG_FILE	
	va_list va;
	va_start(va,fmt);	
	ret = printf(fmt);
	va_end(va);
#endif	
	return ret;
}

