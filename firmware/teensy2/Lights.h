#ifndef _LIGHTS_H_
#define _LIGHTS_H_
	#define LED_PANELS 4
	#define PANEL_LEDS 8
	#define LED_COUNT LED_PANELS * PANEL_LEDS

    void Lights_Update();
	
	typedef struct
	{
	  uint8_t red, green, blue;
	} rgb_color;	
	
	typedef struct {
		int8_t sensorNumber;
		
		// We map from a Sensor number to a FROM-TO light number
		uint8_t fromLight;
		uint8_t toLight;
		
		rgb_color onColor;
		rgb_color offColor;
		
		rgb_color onFadeColor;
		rgb_color offFadeColor;
		
		bool fadeOn;
		bool fadeOff
		
    } __attribute__((packed)) LightRule;
	
	typedef struct
	{
		LightRule lightRules[MAX_LIGHT_RULES]
	} LightConfiguration;
	
	extern LightConfiguration lightConfiguration;

#endif