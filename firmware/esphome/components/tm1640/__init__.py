import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import time as time_
from esphome.const import CONF_ID, PLATFORM_ESP32

ESP_PLATFORMS = [PLATFORM_ESP32]

CONF_CLK_PIN = "clk_pin"
CONF_DIO_PIN = "dio_pin"
CONF_BRIGHTNESS = "brightness"
CONF_DRIVER_BRIGHTNESS = "driver_brightness"
CONF_TIME_ID = "time_id"
CONF_TWELVE_HOUR = "twelve_hour"
CONF_BLINK_COLON = "blink_colon"
CONF_RED = "red"
CONF_GREEN = "green"
CONF_BLUE = "blue"


tm1640_ns = cg.esphome_ns.namespace("tm1640")
Tm1640 = tm1640_ns.class_("Tm1640", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Tm1640),
        cv.Required(CONF_CLK_PIN): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_DIO_PIN): pins.internal_gpio_output_pin_schema,
        cv.Optional(CONF_BRIGHTNESS, default=255): cv.int_range(min=0, max=255),
        cv.Optional(CONF_DRIVER_BRIGHTNESS, default=7): cv.int_range(min=0, max=7),
        cv.Optional(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
        cv.Optional(CONF_TWELVE_HOUR, default=True): cv.boolean,
        cv.Optional(CONF_BLINK_COLON, default=True): cv.boolean,
        # RGB channels are on/off; common brightness maps to the chip's global
        # 0..7 duty setting, capped by driver_brightness.
        cv.Optional(CONF_RED, default=255): cv.int_range(min=0, max=255),
        cv.Optional(CONF_GREEN, default=255): cv.int_range(min=0, max=255),
        cv.Optional(CONF_BLUE, default=255): cv.int_range(min=0, max=255),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    clk = await cg.gpio_pin_expression(config[CONF_CLK_PIN])
    dio = await cg.gpio_pin_expression(config[CONF_DIO_PIN])
    cg.add(var.set_pins(clk, dio))
    cg.add(var.set_driver_brightness(config[CONF_DRIVER_BRIGHTNESS]))
    cg.add(var.set_twelve_hour(config[CONF_TWELVE_HOUR]))
    cg.add(var.set_blink_colon(config[CONF_BLINK_COLON]))
    cg.add(
        var.set_rgb(
            config[CONF_RED],
            config[CONF_GREEN],
            config[CONF_BLUE],
            config[CONF_BRIGHTNESS],
        )
    )

    if CONF_TIME_ID in config:
        time = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time(time))
