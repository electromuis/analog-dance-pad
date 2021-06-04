#include <string.h>
#include <stdint.h>
#include <util/atomic.h>
#include <avr/eeprom.h>

#include "Config/DancePadConfig.h"
#include "Pad.h"
#include "ConfigStore.h"

// just some random bytes to figure out what we have in eeprom
// change these to reset configuration!
static const uint8_t magicBytes[5] = {9, 74, 9, 48, 99};

// where magic bytes (which indicate that a pad configuration is, in fact, stored) exist
#define MAGIC_BYTES_ADDRESS ((void *) 0x00)

// where actual configration is stored
#define CONFIGURATION_ADDRESS ((void *) (MAGIC_BYTES_ADDRESS + sizeof (magicBytes)))

#if defined(BOARD_TYPE_FSRMINIPAD)
	#define DEFAULT_NAME "FSR Mini pad"
#else
	#define DEFAULT_NAME "Untitled FSR Device"
#endif

#define DEFAULT_LIGHT_RULE_1(sensor, lfrom, lto) \
    {                                            \
        .sensorNumber = sensor,                  \
        .fromLight = lfrom,                      \
        .toLight = lto,                          \
        .onColor = {255, 255, 255},              \
        .offColor = {2, 0, 0},                   \
        .onFadeColor = {0, 0, 0},                \
        .offFadeColor = {255, 0, 0},             \
        .flags = LRF_FADE_OFF                    \
    }

#define DEFAULT_LIGHT_RULE_2(sensor, lfrom, lto) \
    {                                            \
        .sensorNumber = sensor,                  \
        .fromLight = lfrom,                      \
        .toLight = lto,                          \
        .onColor = {255, 255, 255},              \
        .offColor = {0, 0, 2},                   \
        .onFadeColor = {0, 0, 0},                \
        .offFadeColor = {0, 0, 255},             \
        .flags = LRF_FADE_OFF                    \
    }

static const Configuration DEFAULT_CONFIGURATION = {
    .padConfiguration = {
        .sensorThresholds = { [0 ... SENSOR_COUNT - 1] = 400 },
        .releaseMultiplier = 0.95,
        .sensorToButtonMapping = {
#if defined(BOARD_TYPE_FSRMINIPAD)
			[0 ... 1] = 0xFF,
			[2] = 0,
			[3] = 1,
			[4] = 2,
			[5] = 3,
			[6 ... SENSOR_COUNT - 1] = 0xFF
#else
			[0 ... SENSOR_COUNT - 1] = 0xFF 
#endif
		}
    },
    .nameAndSize = {
        .size = sizeof(DEFAULT_NAME) - 1, // we don't care about the null at the end.
        .name = DEFAULT_NAME
    },
	.lightConfiguration = {
        .lightRules =
		{	
#if defined(BOARD_TYPE_FSRMINIPAD)
            DEFAULT_LIGHT_RULE_1(2, PANEL_LEDS * 0, PANEL_LEDS * 1), // LEFT
            DEFAULT_LIGHT_RULE_2(5, PANEL_LEDS * 1, PANEL_LEDS * 2), // DOWN
            DEFAULT_LIGHT_RULE_2(3, PANEL_LEDS * 3, PANEL_LEDS * 4), // UP
            DEFAULT_LIGHT_RULE_1(4, PANEL_LEDS * 2, PANEL_LEDS * 3)  // RIGHT
#endif
        }
	}
};

void ConfigStore_LoadConfiguration(Configuration* conf) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        // see if we have magic bytes stored
        uint8_t magicByteBuffer[sizeof (magicBytes)];
        eeprom_read_block(magicByteBuffer, MAGIC_BYTES_ADDRESS, sizeof (magicBytes));

        if (memcmp(magicByteBuffer, magicBytes, sizeof (magicBytes)) == 0) {
            // we had magic bytes, let's load the configuration!
            eeprom_read_block(conf, CONFIGURATION_ADDRESS, sizeof (Configuration));
        } else {
            // we had some garbage on magic byte address, let's just use the default configuration
            ConfigStore_FactoryDefaults(conf);
        }
    }
}

void ConfigStore_StoreConfiguration(const Configuration* conf) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        eeprom_update_block(conf, CONFIGURATION_ADDRESS, sizeof (Configuration));
        eeprom_update_block(magicBytes, MAGIC_BYTES_ADDRESS, sizeof (magicBytes));
    }
}

void ConfigStore_FactoryDefaults (Configuration* conf) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        memcpy(conf, &DEFAULT_CONFIGURATION, sizeof(Configuration));
    }
}