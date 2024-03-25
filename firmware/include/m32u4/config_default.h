
const Configuration DEFAULT_CONFIGURATION = {
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
				[7] = DEFAULT_SENSOR_CONFIG(7)//,
				//[8 ... SENSOR_COUNT - 1] = DEFAULT_SENSOR_CONFIG(0xFF)
            #else
                [0 ... SENSOR_COUNT - 1] = DEFAULT_SENSOR_CONFIG(0xFF)
            #endif
		
		}
    },
    .nameAndSize = {
        .size = sizeof(DEFAULT_NAME) - 1,//, // we don't care about the null at the end.
        .name = DEFAULT_NAME
    },
	.lightConfiguration = {
    #if CONFIG_LAYOUT == CONFIG_LAYOUT_PAD4
        .lightRules =
        {
            DEFAULT_LIGHT_RULE_RED, // LEFT & RIGHT
            DEFAULT_LIGHT_RULE_BLUE, // DOWN & UP
        },
        .ledMappings =
        {
            DEFAULT_LED_MAPPING(0, 5, PANEL_LEDS * 0, PANEL_LEDS * 1), // LEFT
            DEFAULT_LED_MAPPING(1, 4, PANEL_LEDS * 1, PANEL_LEDS * 2), // DOWN
            DEFAULT_LED_MAPPING(1, 2, PANEL_LEDS * 3, PANEL_LEDS * 4), // UP
            DEFAULT_LED_MAPPING(0, 3, PANEL_LEDS * 2, PANEL_LEDS * 3)  // RIGHT
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
