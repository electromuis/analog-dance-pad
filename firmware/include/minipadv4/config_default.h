

const Configuration DEFAULT_CONFIGURATION = {
    .padConfiguration = {
		.sensors = {
            [0] = DEFAULT_SENSOR_CONFIG(0),
            [1] = DEFAULT_SENSOR_CONFIG(1),
            [2] = DEFAULT_SENSOR_CONFIG(2),
            [3] = DEFAULT_SENSOR_CONFIG(3)
		},
        .releaseMode = RELEASE_NONE,
    },
    .nameAndSize = {
        .size = sizeof(DEFAULT_NAME) - 1,//, // we don't care about the null at the end.
        .name = DEFAULT_NAME
    },
	.lightConfiguration = {
        .lightRules =
        {
            DEFAULT_LIGHT_RULE_BLUE,
            DEFAULT_LIGHT_RULE_RED,
            [2 ... MAX_LIGHT_RULES - 1] = LIGHT_RULE_DISABLED()
        },
        .ledMappings =
        {
            DEFAULT_LED_MAPPING(0, 0, 0, 16),
            DEFAULT_LED_MAPPING(1, 1, 16, 32),
            DEFAULT_LED_MAPPING(0, 2, 32, 48),
            DEFAULT_LED_MAPPING(1, 3, 48, 64),
            [4 ... MAX_LED_MAPPINGS - 1] = LED_MAPPING_DISABLED()
        }
	}
};
