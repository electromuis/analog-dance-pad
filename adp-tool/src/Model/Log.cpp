#include <Adp.h>

#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

#include "Log.h"

using namespace std;

namespace adp {

static vector<string>* messages = nullptr;
static bool enabled = true;

void Log::Init()
{
	messages = new vector<string>();
}

void Log::SetEnabled(bool enabled)
{
	enabled = enabled;
}

void Log::Shutdown()
{
	delete messages;
	messages = nullptr;
}

void Log::Write(const char* message)
{
	if (!enabled)
		return;

	messages->emplace_back(message);
	cout << message << endl;
}

void Log::Writef(const char* format, ...)
{
	if (!enabled)
		return;

	char buffer[10000];
	va_list args;
	va_start(args, format);

	#if defined(_MSC_VER)
	size_t len = vsprintf_s(buffer, 10000, format, args);
	#else
	size_t len = vsnprintf(buffer, 10000, format, args);
	#endif

	messages->emplace_back((const char*)buffer, len);
	
	va_end (args);

	for(int i=0 ; i<len ; ++i)
	{
		cout << buffer[i];
	}

	cout << endl;
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
