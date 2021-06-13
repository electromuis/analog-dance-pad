#ifndef _LIGHTS_H_
#define _LIGHTS_H_

#include "Config/DancePadConfig.h"

typedef struct
{
    uint8_t red, green, blue;
} __attribute__((packed)) rgb_color;

enum LightRuleFlags
{
    LRF_FADE_ON  = 0x1,
    LRF_FADE_OFF = 0x2,
};

typedef struct
{
    uint8_t lightRuleIndex;
    uint8_t sensorIndex;
    uint8_t ledIndexBegin;
    uint8_t ledIndexEnd;
} __attribute__((packed)) LedMapping;

typedef struct
{
	rgb_color onColor;
	rgb_color offColor;
	rgb_color onFadeColor;
	rgb_color offFadeColor;
	uint8_t flags;
} __attribute__((packed)) LightRule;

void Lights_UpdateConfiguration(const LightConfiguration* configuration);
void Lights_Update();

extern LightConfiguration LIGHT_CONF;

#endif
