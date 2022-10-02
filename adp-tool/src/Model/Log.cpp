#include <Adp.h>

#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include <memory>
#include <string>

#include "Log.h"

using namespace std;

namespace adp {

static vector<string>* messages = nullptr;

void Log::Init()
{
	messages = new vector<string>();
}

void Log::Shutdown()
{
	delete messages;
	messages = nullptr;
}

void Log::Write(const char* message)
{
	messages->emplace_back(message);
}

void Log::Writef(const char* format, ...)
{
	char buffer[256];
	va_list args;
	va_start(args, format);

	size_t len = vsprintf_s(buffer, 256, format, args);
	messages->emplace_back((const char*)buffer, len);
	
	va_end (args);
}

int Log::NumMessages()
{
	return (int)messages->size();
}

const string& Log::Message(int index)
{
	return messages->at(index);
}

}; // namespace adp.
