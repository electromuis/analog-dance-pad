#ifndef _ADC_H_
#define _ADC_H_
    #include <stdint.h>
	
	#define AREF_5 (1 << REFS0)
	#define AREF_3 ((1 << REFS0) | (1 << REFS1))
	
	enum AdcConfigFlags
	{
		ADC_DISABLED     = 0x1,
		ADC_SET_RESISTOR = 0x2,
		ADC_AREF_5		 = 0x4,
		ADC_AREF_3		 = 0x8
	};
	
	typedef struct
	{
		uint8_t flags;
		uint8_t resistorValue;
	} __attribute__((packed)) AdcConfig;
	
	typedef struct
	{
		AdcConfig adcConfig[SENSOR_COUNT];
		uint8_t selectedAdcConfigIndex;
	} __attribute__((packed)) AdcConfiguration;
    
	void Adc_UpdateConfiguration(const AdcConfiguration* adcConfiguration);
    void ADC_Init(void);
    uint16_t ADC_Read(uint8_t channel);
	
	extern AdcConfiguration ADC_CONF;
#endif
