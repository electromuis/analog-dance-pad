#include <stdint.h>
#include <avr/io.h>

#include "Config/DancePadConfig.h"
#include "twi/twi_master.h"

#define MCP4725_ADDR	     0x60
#define MCP4725_CMD_WRITEDAC 0x40

// see page 308 of https://cdn.sparkfun.com/datasheets/Dev/Arduino/Boards/ATMega32U4.pdf for these
static const uint8_t sensorToAnalogPin[SENSOR_COUNT] = {
    0b000000,
    0b000001,
    0b000100,
    0b000101,
    0b000110,
    0b000111,
    0b100000,
    0b100001,
    0b100010,
    0b100011,
    0b100100,
    0b100101
};

void ADC_Init(void) {
    // different prescalers change conversion speed. tinker! 111 is slowest, and not fast enough for many sensors.
    const uint8_t prescaler = (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0);

    ADCSRA = (1 << ADEN) | prescaler;
    ADMUX = 0; // analog reference = AREF, on which our MCP4725 ADC is connected
    ADCSRB = (1 << ADHSM); // enable high speed mode
	
	tw_init(TW_FREQ_400K, false); // set I2C Frequency
	
	uint16_t output = 4095;
	
	uint8_t packet[3];
	
	packet[0] = MCP4725_CMD_WRITEDAC;
	packet[1] = output / 16;        // Upper data bits (D11.D10.D9.D8.D7.D6.D5.D4)
	packet[2] = (output % 16) << 4; // Lower data bits (D3.D2.D1.D0.x.x.x.x)
	
	tw_master_transmit(MCP4725_ADDR, packet, sizeof(packet), false);
}

uint16_t ADC_Read(uint8_t sensor) {
    uint8_t pin = sensorToAnalogPin[sensor];
	
	if(pin == 0b111111) {
		return 0;
	}

    // see: https://www.avrfreaks.net/comment/885267#comment-885267
    ADMUX = (ADMUX & 0xE0) | (pin & 0x1F); // select channel (MUX0-4 bits)
    ADCSRB = (ADCSRB & 0xDF) | (pin & 0x20); // select channel (MUX5 bit) 

    ADCSRA |= (1 << ADSC); // start conversion
    while (ADCSRA & (1 << ADSC)) {}; // wait until done

    return ADC;
}
