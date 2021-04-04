#pragma once

#include <string>

namespace logging {

class Log
{
public:
	static void Init();

	static void Shutdown();

	static void Write(const wchar_t* message);

	static void Writef(const wchar_t* format, ...);

	static int NumMessages();

	static const std::wstring& Message(int index);
};

}; // namespace logging.