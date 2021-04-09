#pragma once

#include "logging.h"

#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include <memory>
#include <string>

using namespace std;

namespace logging {

static vector<wstring>* messages = nullptr;

void Log::Init()
{
	messages = new vector<wstring>();
}

void Log::Shutdown()
{
	delete messages;
	messages = nullptr;
}

void Log::Write(const wchar_t* message)
{
	messages->emplace_back(message);
}

void Log::Writef(const wchar_t* format, ...)
{
	wchar_t buffer[256];
	va_list args;
	va_start(args, format);
	size_t len = vswprintf_s(buffer, format, args);
	messages->emplace_back((const wchar_t*)buffer, len);
}

int Log::NumMessages()
{
	return (int)messages->size();
}

const wstring& Log::Message(int index)
{
	return messages->at(index);
}

}; // namespace logging.