#ifndef _LIGHTS_H_
#define _LIGHTS_H_
	#define LED_PANELS 4
	#define PANEL_LEDS 8
	#define LED_COUNT (LED_PANELS * PANEL_LEDS)
	
	typedef struct
	{
	  uint8_t red, green, blue;
	} __attribute__((packed)) rgb_color;

    enum LightRuleFlags
    {
        LRF_FADE_ON  = 0x1,
        LRF_FADE_OFF = 0x2,
    };
	
	typedef struct {
		int8_t sensorNumber;
		
		// We map from a Sensor number to a FROM-TO light number
		uint8_t fromLight;
		uint8_t toLight;
		
		rgb_color onColor;
		rgb_color offColor;
		
		rgb_color onFadeColor;
		rgb_color offFadeColor;
		
		uint8_t flags;
		
    } __attribute__((packed)) LightRule;
	
	typedef struct
	{
        LightRule lightRules[MAX_LIGHT_RULES];
	} __attribute__((packed)) LightConfiguration;

    void Lights_UpdateConfiguration(const LightConfiguration* configuration);
    void Lights_Update();

	extern LightConfiguration LIGHT_CONF;

#endif
