#include "hal/hal_ADC.hpp"
#include "Modules/ModuleConfig.hpp"
#include <stdint.h>
#include <avr/io.h>
#include "adp_config.hpp"

void ADC_LoadPot(uint8_t sensor) {
	SensorConfig s = ModuleConfigInstance.GetSensorConfig(sensor);
	
	// PD1 PD0 PC6 PE6
	
	// Set the correct muxer output
	
	if(sensor & 1) {
		REG_MUX_1 |= 1 << PIN_MUX_1;
	}
	else {
		REG_MUX_1 &= ~(1 << PIN_MUX_1);
	}
	
	if(sensor & (1 << 1)) {
		REG_MUX_2 |= 1 << PIN_MUX_2;
	}
	else {
		REG_MUX_2 &= ~(1 << PIN_MUX_2);
	}
	
	if(sensor & (1 << 2)) {
		REG_MUX_3 |= 1 << PIN_MUX_3;
	}
	else {
		REG_MUX_3 &= ~(1 << PIN_MUX_3);
	}
	
	if(sensor & (1 << 3)) {
		REG_MUX_4 |= 1 << PIN_MUX_4;
	}
	else {
		REG_MUX_4 &= ~(1 << PIN_MUX_4);
	}
	
	// Set the digipot via SPI
	
	#if defined(LED_PIN_FIX)
		// Light pin is on the SPI register. Need to enable/disable SPI each time.
		SPCR = (1 << SPE) | (1 << MSTR);  // SPI enable, Master
	#endif
	
	REG_POT_CS &= ~(1 << PIN_POT_CS);
	
	SPDR = 0b00010001;
    while(! (SPSR & (1 << SPIF)) ) ;
	
	SPDR = s.resistorValue;	
    while(! (SPSR & (1 << SPIF)) ) ;
	
	REG_POT_CS |= 1 << PIN_POT_CS;
	
	#if defined(LED_PIN_FIX)
		// Light pin is on the SPI register. Need to enable/disable SPI each time.
		SPCR = 0;
	#endif
}

void HAL_ADC_Init()
{
    // different prescalers change conversion speed. tinker! 111 is slowest, and not fast enough for many sensors.
    const uint8_t prescaler = (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0);

    ADCSRA = (1 << ADEN) | prescaler;
    ADMUX = (1 << REFS0);
    ADCSRB = (1 << ADHSM); // enable high speed mode

	// Muxer outputs
	DDRD |= (1 << DDD0) | (1 << DDD1);
	DDRC |= 1 << DDC6;
	DDRE |= 1 << DDE6;
	
	#if defined(FEATURE_DIGIPOT_ENABLED)
		DDRB |= (1 << DDB6) | (1 << DDB2) | (1 << DDB1); //spi pins on port b SS, MOSI, SCK outputs
		SPCR = (1 << SPE) | (1 << MSTR);  // SPI enable, Master
	#endif
}

uint16_t HAL_ADC_ReadSensor(uint8_t sensor)
{
    uint8_t pin = sensorToAnalogPin[sensor];
	if(pin == 0b111111) {
		return 0;
	}
	
	#if defined(FEATURE_DIGIPOT_ENABLED)
		ADC_LoadPot(sensor);
	#endif

    // see: https://www.avrfreaks.net/comment/885267#comment-885267
    ADMUX = (ADMUX & 0xE0) | (pin & 0x1F);   //select channel (MUX0-4 bits)
	ADCSRB = (ADCSRB & 0xDF) | (pin & 0x20);   //select channel (MUX5 bit) 
	
	ADCSRA |= (1 << ADSC); // start conversion
	while (ADCSRA & (1 << ADSC)) {}; // wait until done
		
    return ADC;
}
