#pragma once

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

namespace adp {

#define ADP_VERSION_MAJOR 2
#define ADP_VERSION_MINOR 0

#define ADP_VERSION_STR_1(x) #x
#define ADP_VERSION_STR_2(a,b) ADP_VERSION_STR_1(a) "." ADP_VERSION_STR_1(b)
#define ADP_VERSION_STR ADP_VERSION_STR_2(ADP_VERSION_MAJOR, ADP_VERSION_MINOR)

}; // namespace adp.
