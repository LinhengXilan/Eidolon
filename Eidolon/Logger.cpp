/**
 * @file Logger.cpp
 * @author LinhengXilan
 * @version 0.0.0.6
 * @date 2026-11-11
 */

#include <Pch.h>
#include <Logger.h>

void Logger::Log(const wchar* format, ...)
{
	wchar buffer[1024]{};
	va_list args;
	va_start(args, format);
	vswprintf_s(buffer, _countof(buffer), format, args);
	va_end(args);
	OutputDebugStringW(buffer);
	OutputDebugStringW(L"\n");
}

void Logger::LogHresult(const wchar* category, const wchar* what, HRESULT hr)
{
	WCHAR buffer[256];
	swprintf_s(buffer, _countof(buffer), L"[%s] %s 失败: 0x%08x\n", category, what, hr);
	OutputDebugStringW(buffer);
}