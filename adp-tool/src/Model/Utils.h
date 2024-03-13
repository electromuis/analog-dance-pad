#pragma once

#include <string>
#include <cstdint>

namespace adp {

// Constants to subtract from decoded UTF-8 codepoint to cancel out non-codepoint bits.
// Indexed by number of trailing bytes (0-5).
extern const uint32_t utf8MultibyteResidu[6];

// Indicates how many trailing bytes follow a leading byte of UTF-8 codepoint.
// Indexed by the leading byte (0-255).
extern const uint8_t utf8TrailingBytes[256];

// Constants to add to the leading byte of UTF-8 codepoint to indicate trailing bytes.
// Indexed by the total number of bytes (1-6).
extern const uint8_t utf8EncodeConstants[7];

// Converts a c-style wide string (UTF-16/UTF-32) to a string (UTF-8).
std::string narrow(const wchar_t* str, size_t len);

// Converts a string (UTF-8) to a wide string (UTF-16/UTF-32).
std::wstring widen(const char* str, size_t len);

}; // namespace adp.
