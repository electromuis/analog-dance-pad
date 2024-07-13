#include "Structs.hpp"

#define DEFAULT_LIGHT_RULE_RED              \
    {                                       \
        .onColor = {100, 100, 100},         \
        .offColor = {2, 0, 0},              \
        .onFadeColor = {0, 0, 0},           \
        .offFadeColor = {255, 0, 0},        \
        .flags = LRF_ENABLED | LRF_FADE_OFF \
    }

#define DEFAULT_LIGHT_RULE_BLUE              \
    {                                        \
        .flags = LRF_ENABLED | LRF_FADE_OFF, \
        .onColor = {100, 100, 100},          \
        .offColor = {0, 0, 2},               \
        .onFadeColor = {0, 0, 0},            \
        .offFadeColor = {0, 0, 255},         \
    }

#define LIGHT_RULE_DISABLED() \
    {                         \
        .flags = 0,           \
    }

#define DEFAULT_LED_MAPPING(rule, sensor, lbegin, lend) \
    {                                                   \
        .flags = LMF_ENABLED,                           \
        .lightRuleIndex = rule,                         \
        .sensorIndex = sensor,                          \
        .ledIndexBegin = lbegin,                        \
        .ledIndexEnd = lend,                            \
    }

#define LED_MAPPING_DISABLED() \
    {                         \
        .flags = 0,           \
    }

#define DEFAULT_SENSOR_CONFIG(button) 	\
	{									\
	.threshold = 400,					\
	.releaseThreshold = 400,			\
	.buttonMapping = button,			\
	.resistorValue = 150,				\
	.flags = 0,							\
	.preload = 0						\
	}

#include "config_default.h"