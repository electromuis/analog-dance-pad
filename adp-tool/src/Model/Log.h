#pragma once

#include <string>

namespace adp {

class Log
{
public:
	static void Init();

	static void Shutdown();

	static void Write(const char* message);

	static void Writef(const char* format, ...);

	static int NumMessages();

	static const std::string& Message(int index);

	static void SetEnabled(bool enabled);
};

}; // namespace adp.