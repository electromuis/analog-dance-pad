#include "Adp.h"

#include "Model/Utils.h"

using namespace std;

namespace adp {

const uint32_t utf8MultibyteResidu[6] =
{
	0x00000000, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080
};

const uint8_t utf8TrailingBytes[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
};

const uint8_t utf8EncodeConstants[7] =
{
	0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};


static uint32_t ReadU8(const uint8_t*& p, const uint8_t* end)
{
	uint32_t c = *p; ++p;
	if (c >= 0x80)
	{
		int numBytes = utf8TrailingBytes[c];
		if (p + numBytes <= end)
		{
			switch (numBytes)
			{
			case 5: c <<= 6; c += *p; ++p; [[fallthrough]];
			case 4: c <<= 6; c += *p; ++p; [[fallthrough]];
			case 3: c <<= 6; c += *p; ++p; [[fallthrough]];
			case 2: c <<= 6; c += *p; ++p; [[fallthrough]];
			case 1: c <<= 6; c += *p; ++p;
			};
			c -= utf8MultibyteResidu[numBytes];
		}
		else c = '~', p = end;
	}
	return c;
}

static uint32_t ReadU16(const uint16_t*& p, const uint16_t* end)
{
	uint32_t c = *p; ++p;
	if (c >= 0xD800 && c <= 0xDBFF && p < end)
	{
		c = ((c - 0xD800) << 10) + (*p - 0xDC00) + 0x0010000; ++p;
	}
	return c;
}

static void WriteU8(string& str, uint32_t c)
{
	if (c < 0x80)
	{
		str.push_back((char)c);
	}
	else if (c > 0x10FFFF)
	{
		str.push_back('~');
	}
	else
	{
		int numBytes = (c < 0x800) ? 2 : ((c < 0x10000) ? 3 : 4);
		uint8_t buffer[4];
		switch (numBytes)
		{
		case 4: buffer[3] = (c | 0x80) & 0xBF; c >>= 6; [[fallthrough]];
		case 3: buffer[2] = (c | 0x80) & 0xBF; c >>= 6; [[fallthrough]];
		case 2: buffer[1] = (c | 0x80) & 0xBF; c >>= 6;
		};
		buffer[0] = (uint8_t)c | utf8EncodeConstants[numBytes];
		str.append((const char*)buffer, numBytes);
	}
}

static void WriteU16(wstring& str, uint32_t c)
{
	if (c < 0xFFFF)
	{
		str.push_back((uint16_t)c);
	}
	else if (c > 0x10FFFF)
	{
		str.push_back(L'~');
	}
	else
	{
		c -= 0x10000;
		str.push_back((uint16_t)(c >> 10) + 0xD800);
		str.push_back((uint16_t)(c & 0x3FFUL) + 0xDC00);
	}
}

string narrow(const wchar_t* s, size_t len)
{
	string out;
	if (sizeof(wchar_t) == sizeof(uint16_t))
	{
		for (auto p = (const uint16_t*)s, end = p + len; p != end;)
			WriteU8(out, ReadU16(p, end));
	}
	else if (sizeof(wchar_t) == sizeof(uint32_t))
	{
		for (auto p = (const uint32_t*)s, end = p + len; p != end;)
			WriteU8(out, *p);
	}
	return out;
}

wstring widen(const char* s, size_t len)
{
	wstring out;
	if (sizeof(wchar_t) == sizeof(uint16_t))
	{
		for (auto p = (const uint8_t*)s, end = p + len; p != end;)
			WriteU16(out, ReadU8(p, end));
	}
	else if (sizeof(wchar_t) == sizeof(uint32_t))
	{
		for (auto p = (const uint8_t*)s, end = p + len; p != end;)
			out.push_back(ReadU8(p, end));
	}
	return out;
}

}; // namespace adp.