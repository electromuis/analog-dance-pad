#include <string.h>
#include <stdint.h>
#include <util/atomic.h>
#include <avr/eeprom.h>

#include "Config/DancePadConfig.h"
#include "Pad.h"
#include "ConfigStore.h"
#include "Lights.h"

// just some random bytes to figure out what we have in eeprom
// change these to reset configuration!
static const uint8_t magicBytes[5] = {9, 63, 9, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR};

// where magic bytes (which indicate that a pad configuration is, in fact, stored) exist
#define MAGIC_BYTES_ADDRESS ((void *) 0x00)

// where actual configration is stored
#define CONFIGURATION_ADDRESS ((void *) (MAGIC_BYTES_ADDRESS + sizeof (magicBytes)))

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

#define DEFAULT_LED_MAPPING(rule, sensor, lbegin, lend) \
    {                                                   \
        .flags = LMF_ENABLED,                           \
        .lightRuleIndex = rule,                         \
        .sensorIndex = sensor,                          \
        .ledIndexBegin = lbegin,                        \
        .ledIndexEnd = lend,                            \
    }

#define DEFAULT_SENSOR_CONFIG(button) 	\
	{									\
	.threshold = 450,					\
	.releaseThreshold = 450 * 0.95,		\
	.buttonMapping = button,			\
	.resistorValue = 150,				\
	.flags = 0,							\
	.preload = 0						\
	}

Configuration configuration;

static const Configuration DEFAULT_CONFIGURATION = {
    .padConfiguration = {
		.sensors = {

            #if CONFIG_LAYOUT == CONFIG_LAYOUT_PAD4
                [0] = DEFAULT_SENSOR_CONFIG(0),
				[1] = DEFAULT_SENSOR_CONFIG(1),
				[2] = DEFAULT_SENSOR_CONFIG(2),
				[3] = DEFAULT_SENSOR_CONFIG(3),
                [4 ... SENSOR_COUNT - 1] = DEFAULT_SENSOR_CONFIG(0xFF)
            #elif CONFIG_LAYOUT == CONFIG_LAYOUT_BOARD8
                [0] = DEFAULT_SENSOR_CONFIG(0),
				[1] = DEFAULT_SENSOR_CONFIG(1),
				[2] = DEFAULT_SENSOR_CONFIG(2),
				[3] = DEFAULT_SENSOR_CONFIG(3),
				[4] = DEFAULT_SENSOR_CONFIG(4),
				[5] = DEFAULT_SENSOR_CONFIG(5),
				[6] = DEFAULT_SENSOR_CONFIG(6),
				[7] = DEFAULT_SENSOR_CONFIG(7),
				[8 ... SENSOR_COUNT - 1] = DEFAULT_SENSOR_CONFIG(0xFF)
            #else
                [0 ... SENSOR_COUNT - 1] = DEFAULT_SENSOR_CONFIG(0xFF)
            #endif
		
		}
    },
    .nameAndSize = {
        .size = sizeof(DEFAULT_NAME) - 1, // we don't care about the null at the end.
        .name = DEFAULT_NAME
    },
	.lightConfiguration = {
        .selectedLightRuleIndex = 0,
        .selectedLedMappingIndex = 0,

    #if CONFIG_LAYOUT == CONFIG_LAYOUT_PAD4
        .lightRules =
        {
            DEFAULT_LIGHT_RULE_RED, // LEFT & RIGHT
            DEFAULT_LIGHT_RULE_BLUE, // DOWN & UP
        },
        .ledMappings =
        {
            DEFAULT_LED_MAPPING(1, 0, PANEL_LEDS * 0, PANEL_LEDS * 1), // Up
            DEFAULT_LED_MAPPING(0, 1, PANEL_LEDS * 1, PANEL_LEDS * 2), // Right
            DEFAULT_LED_MAPPING(1, 2, PANEL_LEDS * 2, PANEL_LEDS * 3), // Down
            DEFAULT_LED_MAPPING(0, 3, PANEL_LEDS * 3, PANEL_LEDS * 4)  // Left
        }
    #elif CONFIG_LAYOUT == CONFIG_LAYOUT_BOARD8
        .lightRules =
        {
            DEFAULT_LIGHT_RULE_BLUE
        },
        .ledMappings =
        {
            DEFAULT_LED_MAPPING(0, 0, 0, 1),
            DEFAULT_LED_MAPPING(0, 1, 1, 2),
            DEFAULT_LED_MAPPING(0, 2, 2, 3),
            DEFAULT_LED_MAPPING(0, 3, 3, 4),
            DEFAULT_LED_MAPPING(0, 4, 4, 5),
            DEFAULT_LED_MAPPING(0, 5, 5, 6),
            DEFAULT_LED_MAPPING(0, 6, 6, 7),
            DEFAULT_LED_MAPPING(0, 7, 7, 8)
        }
    #endif
	}
};

void SetupConfiguration()
{
	Pad_Initialize(&configuration.padConfiguration);
    Lights_UpdateConfiguration(&configuration.lightConfiguration);
}

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
