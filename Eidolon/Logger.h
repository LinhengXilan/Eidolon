/**
 * @file Logger.h
 * @author LinhengXilan
 * @version 0.0.0.6
 * @date 2026-11-11
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Types.h>

class Logger
{
public:
	static void Log(const wchar* format, ...);
	static void LogHresult(const wchar* category, const wchar* what, HRESULT hr);
};

#endif