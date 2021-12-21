#include <stdint.h>
#include <avr/io.h>

#include "Config/DancePadConfig.h"
#include "Pad.h"
#include "ADC.h"

// see page 308 of https://cdn.sparkfun.com/datasheets/Dev/Arduino/Boards/ATMega32U4.pdf for these
static const uint8_t sensorToAnalogPin[SENSOR_COUNT] = {
#if defined(BOARD_TYPE_FSRIO_2)
	0b000000, //ADC0
    0b000001, //ADC1
	0b000100, //ADC4
	0b000101, //ADC5
	0b000110, //ADC6
	0b000111, //ADC7
	0b100000, //ADC8
	0b100001, //ADC9
	
	0b111111,
	0b111111,
	0b111111,
	0b111111
#elif defined(BOARD_TYPE_FSRIO_1)	
    0b000111, //ADC7 A0
	0b000110, //ADC6 A1
	0b000101, //ADC5 A2
	0b000100, //ADC4 A3
	0b100000, //ADC8 A6
	0b100010, //ADC10 A7
	0b100011, //ADC11 A8
	0b100100, //ADC12 A9
	
	0b111111,
	0b111111,
	0b111111,
	0b111111
#elif defined(BOARD_TYPE_FSRMINIPAD)	
	0b111111,
    0b111111,
	0b000100, //ADC4 A3
	0b000101, //ADC5 A2
	0b000110, //ADC6 A1
	0b000111, //ADC7 A0
	0b111111,
	0b111111,
	0b111111,
	0b111111,
	0b111111,
	0b111111
#else
	0b000000, //ADC0 A5
    0b000001, //ADC1 A4
	0b000100, //ADC4 A3
	0b000101, //ADC5 A2
	0b000110, //ADC6 A1
	0b000111, //ADC7 A0
	0b100000, //ADC8 A6
	0b100001, //ADC9
	0b100010, //ADC10 A7
	0b100011, //ADC11 A8
	0b100100, //ADC12 A9
	0b100101  //ADC13 A10
#endif
};

void ADC_LoadPot(uint8_t sensor) {
	SensorConfig s = PAD_CONF.sensors[sensor];
	
	// PD1 PD0 PC6 PE6
	
	// Set the correct muxer output
	
	#if defined(BOARD_TYPE_FSRIO_1) && !defined(BOARD_TYPE_FSRIO_2)
		if(sensor & 1) {
			PORTD |= 1 << DDD1;
		}
		else {
			PORTD &= ~(1 << DDD1);
		}
		
		if(sensor & (1 << 1)) {
			PORTD |= 1 << DDD0;
		}
		else {
			PORTD &= ~(1 << DDD0);
		}
		
		if(sensor & (1 << 2)) {
			PORTC |= 1 << DDC6;
		}
		else {
			PORTC &= ~(1 << DDC6);
		}
		
		if(sensor & (1 << 3)) {
			PORTE |= 1 << DDE6;
		}
		else {
			PORTE &= ~(1 << DDE6);
		}
	#else
		PORTD = (PORTD & 0b11110000) | (sensor & 0b00001111);
	#endif
	
	// Set the digipot via SPI
	
	#if defined(BOARD_TYPE_FSRIO_1) && !defined(BOARD_TYPE_FSRIO_2)
		// Light pin is on the SPI register. Need to enable/disable SPI each time.
		SPCR = (1 << SPE) | (1 << MSTR);  // SPI enable, Master
		
		PORTB &= ~(1 << DDB6);
	#else
		PORTE &= ~(1 << DDE6);
	#endif
	
	
	
	SPDR = 0b00010001;
    while(! (SPSR & (1 << SPIF)) ) ;
	
	SPDR = s.resistorValue;	
    while(! (SPSR & (1 << SPIF)) ) ;
	
	
	
	#if defined(BOARD_TYPE_FSRIO_1) && !defined(BOARD_TYPE_FSRIO_2)
		PORTB |= 1 << DDB6;
	
		// Light pin is on the SPI register. Need to enable/disable SPI each time.
		SPCR = 0;
	#else
		PORTE |= 1 << DDE6;
	#endif
}

void ADC_Init(void) {
    // different prescalers change conversion speed. tinker! 111 is slowest, and not fast enough for many sensors.
    const uint8_t prescaler = (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0);

    ADCSRA = (1 << ADEN) | prescaler;
    ADMUX = (1 << REFS0);
    ADCSRB = (1 << ADHSM); // enable high speed mode
	
	#if defined(FEATURE_DIGIPOT_ENABLED)
		// Muxer outputs
		#if defined(BOARD_TYPE_FSRIO_1) && !defined(BOARD_TYPE_FSRIO_2)
			DDRD |= (1 << DDD0) | (1 << DDD1);
			DDRC |= 1 << DDC6;
			DDRE |= 1 << DDE6;
			DDRB |= (1 << DDB6) | (1 << DDB2) | (1 << DDB1); //spi pins on port b SS, MOSI, SCK outputs
		#else
			DDRD |= 0b00001111;
			DDRE |= 1 << DDE6;
			DDRB |= (1 << DDB2) | (1 << DDB1); //spi pins on port b MOSI, SCK outputs
		#endif

		SPCR = (1 << SPE) | (1 << MSTR);  // SPI enable, Master
	#endif
}

uint16_t ADC_Read(uint8_t sensor) {
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
