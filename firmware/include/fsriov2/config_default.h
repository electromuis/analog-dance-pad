#define DEFAULT_NAME "FSRIOv2"

const Configuration DEFAULT_CONFIGURATION = {
    .padConfiguration = {
		.sensors = {

            [0] = DEFAULT_SENSOR_CONFIG(0),
            [1] = DEFAULT_SENSOR_CONFIG(1),
            [2] = DEFAULT_SENSOR_CONFIG(2),
            [3] = DEFAULT_SENSOR_CONFIG(3),
            [4] = DEFAULT_SENSOR_CONFIG(4),
            [5] = DEFAULT_SENSOR_CONFIG(5),
            [6] = DEFAULT_SENSOR_CONFIG(6),
            [7] = DEFAULT_SENSOR_CONFIG(7),
            [8 ... SENSOR_COUNT - 1] = DEFAULT_SENSOR_CONFIG(0xFF)
		}
    },
    .nameAndSize = {
        .size = sizeof(DEFAULT_NAME) - 1,//, // we don't care about the null at the end.
        .name = DEFAULT_NAME
    },
	.lightConfiguration = {
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
	}
};